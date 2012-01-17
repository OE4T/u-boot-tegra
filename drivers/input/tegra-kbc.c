/*
 *  (C) Copyright 2011
 *  NVIDIA Corporation <www.nvidia.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <stdio_dev.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <tegra-kbc.h>
#include <fdt_decode.h>
#if defined(CONFIG_TEGRA2)
#include <asm/arch/pinmux.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#define DEVNAME "tegra-kbc"

enum {
	KBC_MAX_GPIO	= 24,
	KBC_MAX_KPENT	= 8,
};

/* KBC row scan time and delay for beginning the row scan. */
#define KBC_ROW_SCAN_TIME	16
#define KBC_ROW_SCAN_DLY	5

/*  uses a 32KHz clock so a cycle = 1/32Khz */
#define KBC_CYCLE_IN_USEC	DIV_ROUND_UP(1000, 32)

#define KBC_FIFO_TH_CNT_SHIFT(cnt)	(cnt << 14)
#define KBC_DEBOUNCE_CNT_SHIFT(cnt)	(cnt << 4)
#define KBC_CONTROL_FIFO_CNT_INT_EN	(1 << 3)
#define KBC_CONTROL_KBC_EN		(1 << 0)
#define KBC_INT_FIFO_CNT_INT_STATUS	(1 << 2)
#define KBC_KPENT_VALID	(1 << 7)

#define KBC_RPT_DLY	20
#define KBC_RPT_RATE	4

/* kbc globals */
static unsigned int kbc_repoll_time;
static unsigned int kbc_init_dly;
static unsigned long kbc_start_time;

/*
 * Use a simple FIFO to convert some keys into escape sequences and to handle
 * testc vs getc.  The FIFO length must be a power of two.  Currently, four
 * characters is the smallest power of two that is large enough to contain all
 * generated escape sequences.
 */
#define KBC_FIFO_LENGTH	(1 << 2)

static int kbc_fifo[KBC_FIFO_LENGTH];
static int kbc_fifo_read;
static int kbc_fifo_write;

/* These are key maps for each modifier: each has KBC_KEY_COUNT entries */
u8 *kbc_plain_keycode;
u8 *kbc_fn_keycode;
u8 *kbc_shift_keycode;
u8 *kbc_ctrl_keycode;

#ifdef CONFIG_OF_CONTROL
struct fdt_kbc config;	/* Our keyboard config */
#if (FDT_KBC_KEY_COUNT) != (KBC_KEY_COUNT)
#error definition mismatch
#endif
#endif

static int tegra_kbc_keycode(int r, int c, int modifier)
{
	int entry = r * KBC_MAX_COL + c;

	debug("tegra_kbc_keycode: r = %d, c = %d, modifier = %d\n",
		r, c, modifier);

	if (modifier == KEY_FN) {
		debug("Function key seen %02X\n", modifier);
		return kbc_fn_keycode[entry];
	}

	/* Put ctrl keys ahead of shift */
	else if ((modifier == KEY_LEFT_CTRL || modifier == KEY_RIGHT_CTRL)
			&& kbc_ctrl_keycode) {
		debug("Ctrl key seen! entry = %d\n", entry);
		return kbc_ctrl_keycode[entry];
	} else if (modifier == KEY_SHIFT) {
		debug("Shift key seen! entry = %d\n", entry);
		return kbc_shift_keycode[entry];
	} else {
		debug("Plain key seen! entry = %d\n", entry);
		return kbc_plain_keycode[entry];
	}
}

/* determines if current keypress configuration can cause key ghosting */
static int is_ghost_key_config(int *rows_val, int *cols_val, int valid)
{
	int i, j, key_in_same_col = 0, key_in_same_row = 0;

	debug("is_ghost_key_config: valid = %d\n", valid);
	/*
	 * Matrix keyboard designs are prone to keyboard ghosting.
	 * Ghosting occurs if there are 3 keys such that -
	 * any 2 of the 3 keys share a row, and any 2 of them share a column.
	 */
	for (i = 0; i < valid; i++) {
		/*
		 * Find 2 keys such that one key is in the same row
		 * and the other is in the same column as the i-th key.
		 */
		for (j = i + 1; j < valid; j++) {
			if (cols_val[j] == cols_val[i])
				key_in_same_col = 1;
			if (rows_val[j] == rows_val[i])
				key_in_same_row = 1;
		}
	}

	if (key_in_same_col && key_in_same_row)
		return 1;
	else
		return 0;
}

/* reads the keyboard fifo for current keypresses. */
static int tegra_kbc_find_keys(int *fifo)
{
	struct kbc_tegra *kbc = (struct kbc_tegra *)NV_PA_KBC_BASE;
	int rows_val[KBC_MAX_KPENT], cols_val[KBC_MAX_KPENT];
	u32 kp_ent_val[(KBC_MAX_KPENT + 3) / 4];
	u32 *kp_ents = kp_ent_val;
	u32 kp_ent = 0;
	int i, j, k, valid = 0;
	int modifier = 0;

	debug("tegra_kbc_find_keys\n");
	for (i = 0; i < ARRAY_SIZE(kp_ent_val); i++)
		kp_ent_val[i] = readl(&kbc->kp_ent[i]);

	valid = 0;
	for (i = 0; i < KBC_MAX_KPENT; i++) {
		if (!(i&3))
			kp_ent = *kp_ents++;

		if (kp_ent & KBC_KPENT_VALID) {
			cols_val[valid] = kp_ent & 0x7;
			rows_val[valid++] = (kp_ent >> 3) & 0xf;
		}
		kp_ent >>= 8;
	}
	for (i = 0; i < valid; i++) {
		k = tegra_kbc_keycode(rows_val[i], cols_val[i], 0);

		if (KEY_IS_MODIFIER(k)) {
			modifier = k;
			break;
		}
	}

	/* For a ghost key config, ignore the keypresses for this iteration. */
	if ((valid >= 3) && is_ghost_key_config(rows_val, cols_val, valid))
		return KBC_MAX_KPENT + 1;

	j = 0;
	for (i = 0; i < valid; i++) {
		k = tegra_kbc_keycode(rows_val[i], cols_val[i], modifier);
		/* Key modifiers (Fn and Shift) will not be part of the fifo */
		if (k && (k != -1))
			fifo[j++] = k;
	}

	return j;
}

/* processes all the keypresses in fifo and returns a single key */
static int tegra_kbc_get_single_char(u32 fifo_cnt)
{
	int i, cnt, j;
	int fifo[KBC_MAX_KPENT] = {0};
	static int prev_fifo[KBC_MAX_KPENT];
	static int prev_cnt;
	static int prev_key;
	char key = 0;

	debug("tegra_kbc_get_single_char: fifo_cnt = %ul\n", fifo_cnt);

	if (fifo_cnt) {
		do {
			cnt = tegra_kbc_find_keys(fifo);
			/* If this is a ghost key combo report no change. */
			key = prev_key;
			if (cnt <= KBC_MAX_KPENT) {
				int prev_key_in_fifo = 0;
				/*
				 * If multiple keys are pressed such that it
				 * results in * 2 or more unmodified keys, we
				 * will determine the newest key and report
				 * that to the upper layer.
				 */
				for (i = 0; i < cnt; i++) {
					for (j = 0; j < prev_cnt; j++) {
						if (fifo[i] == prev_fifo[j])
							break;
					}
					/* Found a new key. */
					if (j == prev_cnt) {
						key = fifo[i];
						break;
					}
					if (prev_key == fifo[i])
						prev_key_in_fifo = 1;
				}
				/*
				 * Keys were released and FIFO does not contain
				 * the previous reported key. So report a null
				 * key.
				 */
				if (i == cnt && !prev_key_in_fifo)
					key = 0;

				for (i = 0; i < cnt; i++)
					prev_fifo[i] = fifo[i];
				prev_cnt = cnt;
				prev_key = key;
			}
		} while (!key && --fifo_cnt);
	} else {
		prev_cnt = 0;
		prev_key = 0;
	}
	udelay((fifo_cnt == 1) ? kbc_repoll_time : 1000);

	return key;
}

/* manages keyboard hardware registers on keypresses and returns a key.*/
static unsigned char tegra_kbc_get_char(void)
{
	struct kbc_tegra *kbc = (struct kbc_tegra *)NV_PA_KBC_BASE;
	u32 val, ctl;
	char key = 0;

	debug("tegra_kbc_get_char\n");
	/*
	 * Until all keys are released, defer further processing to
	 * the polling loop.
	 */
	ctl = readl(&kbc->control);
	ctl &= ~KBC_CONTROL_FIFO_CNT_INT_EN;
	writel(ctl, &kbc->control);

	/*
	 * Quickly bail out & reenable interrupts if the interrupt source
	 * wasn't fifo count threshold
	 */
	val = readl(&kbc->interrupt);
	writel(val, &kbc->interrupt);

	if (val & KBC_INT_FIFO_CNT_INT_STATUS)
		key = tegra_kbc_get_single_char((val >> 4) & 0xf);

	ctl |= KBC_CONTROL_FIFO_CNT_INT_EN;
	writel(ctl, &kbc->control);

	return key;
}

/* handles key repeat delay and key repeat rate.*/
static int kbd_fetch_char(int test)
{
	unsigned char key;
	static unsigned char prev_key;
	static unsigned int rpt_dly = KBC_RPT_DLY;

	debug("kbd_fetch_char: test = %d\n", test);

	do {
		key = tegra_kbc_get_char();
		if (!key) {
			prev_key = 0;
			if (test)
				break;
			else
				continue;
		}

		/* This logic takes care of the repeat rate */
		if ((key != prev_key) || !(rpt_dly--))
			break;
	} while (1);

	if (key == prev_key) {
		rpt_dly = KBC_RPT_RATE;
	} else {
		rpt_dly = KBC_RPT_DLY;
		prev_key = key;
	}

	return key;
}

/*
 * Return true if there are no characters in the FIFO.
 */
static int kbd_fifo_empty(void)
{
	debug("kbd_fifo_empty\n");

	return kbc_fifo_read == kbc_fifo_write;
}

/*
 * Insert a character into the FIFO.  Calling this function when the FIFO is
 * full will overwrite the oldest character in the FIFO.
 */
static void kbd_fifo_insert(int key)
{
	int index = kbc_fifo_write & (KBC_FIFO_LENGTH - 1);

	debug("kbd_fifo_insert: key = %d\n", key);

	kbc_fifo[index] = key;

	kbc_fifo_write++;
}

/*
 * Remove a character from the FIFO, it is an error to call this function when
 * the FIFO is empty.
 */
static int kbd_fifo_remove(void)
{
	int index = kbc_fifo_read & (KBC_FIFO_LENGTH - 1);
	int key   = kbc_fifo[index];

	debug("kbd_fifo_remove\n");

	assert(!kbd_fifo_empty());

	kbc_fifo_read++;

	return key;
}

/*
 * Given a keycode, convert it to the sequence of characters that U-Boot expects
 * to recieve from its input devices and insert the characters into the FIFO.
 * If the keycode is 0, ignore it.  Currently this function will only ever add
 * at most three characters to the FIFO, so it is always safe to call when the
 * FIFO is empty.
 */
static void kbd_fifo_refill(int key)
{
	debug("kbd_fifo_refill: key = %d\n", key);

	switch (key) {

	/*
	 * We need to deal with a zero keycode value here because the
	 * testc call can call us with zero if no key is pressed.
	 */
	case 0x00:
		break;

	/*
	 * Generate escape sequences for arrow keys.  Additional escape
	 * sequences can be added to the switch statements, but it may
	 * be better in the future to use the top bit of the keycode to
	 * indicate a key that generates an escape sequence.  Then the
	 * outer switch could turn into an "if (key & 0x80)".
	 */
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		kbd_fifo_insert(0x1B); /* Escape */
		kbd_fifo_insert('[');

		switch (key) {

		case 0x1C:
			kbd_fifo_insert('D'); break;
		case 0x1D:
			kbd_fifo_insert('C'); break;
		case 0x1E:
			kbd_fifo_insert('A'); break;
		case 0x1F:
			kbd_fifo_insert('B'); break;
		}
		break;

	/*
	 * All other keycodes can be treated as characters as is and
	 * are inserted into the FIFO directly.
	 */
	default:
		kbd_fifo_insert(key);
		break;
	}
}

static void kbd_wait_for_fifo_init(void)
{
	unsigned long elapsed_time;
	long delay;
	static unsigned int kbc_initialized;

	debug("kbd_wait_for_fifo_init\n");

	if (kbc_initialized)
		return;
	/*
	 * In order to detect keys pressed on boot, wait for the hardware to
	 * complete scanning the keys. This includes time to transition from
	 * Wkup mode to Continous polling mode and the repoll time. We can
	 * deduct the time thats already elapsed.
	 */
	elapsed_time = timer_get_us() - kbc_start_time;
	delay = kbc_init_dly + kbc_repoll_time - elapsed_time;
	if (delay > 0)
		udelay(delay);

	kbc_initialized = 1;
}

static int kbd_testc(void)
{
	debug("kbd_testc\n");

	kbd_wait_for_fifo_init();

	if (kbd_fifo_empty())
		kbd_fifo_refill(kbd_fetch_char(1));

	return !kbd_fifo_empty();
}

static int kbd_getc(void)
{
	debug("kbd_getc\n");

	if (kbd_fifo_empty())
		kbd_fifo_refill(kbd_fetch_char(0));

	return kbd_fifo_remove();
}

/* configures keyboard GPIO registers to use the rows and columns */
static void config_kbc(void)
{
	struct kbc_tegra *kbc = (struct kbc_tegra *)NV_PA_KBC_BASE;
	int i;

	debug("config_kbc\n");

	for (i = 0; i < KBC_MAX_GPIO; i++) {
		u32 row_cfg, col_cfg;
		u32 r_shift = 5 * (i % 6);
		u32 c_shift = 4 * (i % 8);
		u32 r_mask = 0x1f << r_shift;
		u32 c_mask = 0xf << c_shift;
		u32 r_offs = i / 6;
		u32 c_offs = i / 8;

		row_cfg = readl(&kbc->row_cfg[r_offs]);
		col_cfg = readl(&kbc->col_cfg[c_offs]);

		row_cfg &= ~r_mask;
		col_cfg &= ~c_mask;

		if (i < KBC_MAX_ROW)
			row_cfg |= ((i << 1) | 1) << r_shift;
		else
			col_cfg |= (((i - KBC_MAX_ROW) << 1) | 1) << c_shift;

		writel(row_cfg, &kbc->row_cfg[r_offs]);
		writel(col_cfg, &kbc->col_cfg[c_offs]);
	}
}

static int tegra_kbc_open(void)
{
	struct kbc_tegra *kbc = (struct kbc_tegra *)NV_PA_KBC_BASE;
	unsigned int scan_time_rows, debounce_cnt, rpt_cnt;
	u32 val = 0;

	debug("tegra_kbc_open\n");

	config_kbc();

	/*
	 * The time delay between two consecutive reads of the FIFO is
	 * the sum of the repeat time and the time taken for scanning
	 * the rows. There is an additional delay before the row scanning
	 * starts. The repoll delay is computed in microseconds.
	 */
	rpt_cnt = 5 * DIV_ROUND_UP(1000, KBC_CYCLE_IN_USEC);
	debounce_cnt = 2;
	scan_time_rows = (KBC_ROW_SCAN_TIME + debounce_cnt) * KBC_MAX_ROW;
	kbc_repoll_time = KBC_ROW_SCAN_DLY + scan_time_rows + rpt_cnt;
	kbc_repoll_time = kbc_repoll_time * KBC_CYCLE_IN_USEC;

	writel(rpt_cnt, &kbc->rpt_dly);

	val = KBC_DEBOUNCE_CNT_SHIFT(debounce_cnt);
	val |= KBC_FIFO_TH_CNT_SHIFT(1); /* set fifo interrupt threshold to 1 */
	val |= KBC_CONTROL_FIFO_CNT_INT_EN;  /* interrupt on FIFO threshold */
	val |= KBC_CONTROL_KBC_EN;     /* enable */

	writel(val, &kbc->control);

	kbc_init_dly = readl(&kbc->init_dly) * KBC_CYCLE_IN_USEC;
	kbc_start_time = timer_get_us();

	return 0;
}

#if defined(CONFIG_TEGRA2)
void config_kbc_pinmux(void)
{
	enum pmux_pingrp pingrp[] = {PINGRP_KBCA, PINGRP_KBCB, PINGRP_KBCC,
				     PINGRP_KBCD, PINGRP_KBCE, PINGRP_KBCF};
	int i;

	for (i = 0; i < (sizeof(pingrp)/sizeof(pingrp[0])); i++) {
		pinmux_tristate_disable(pingrp[i]);
		pinmux_set_func(pingrp[i], PMUX_FUNC_KBC);
		pinmux_set_pullupdown(pingrp[i], PMUX_PULL_UP);
	}
}
#endif

int drv_keyboard_init(void)
{
	int error;
	struct stdio_dev kbddev;
	char *stdinname;

	debug("drv_keyboard_init\n");

#ifdef CONFIG_OF_CONTROL
	int	node, upto = 0;

	node = fdt_decode_next_alias(gd->blob, "kbc",
				  COMPAT_NVIDIA_TEGRA250_KBC, &upto);
	if (node < 0) {
		debug("node = %d, returning node\n", node);
		return node;
	}

	if (fdt_decode_kbc(gd->blob, node, &config)) {
		debug("fdt_decode_kbc failed, returning -1\n");
		return -1;
	}

	kbc_plain_keycode = config.plain_keycode;
	kbc_shift_keycode = config.shift_keycode;
	kbc_fn_keycode    = config.fn_keycode;
	kbc_ctrl_keycode  = config.ctrl_keycode;
#else
	kbc_plain_keycode = board_keyboard_config.plain_keycode;
	kbc_shift_keycode = board_keyboard_config.shift_keycode;
	kbc_fn_keycode    = board_keyboard_config.fn_keycode;
#endif
#if defined(CONFIG_TEGRA2)
	config_kbc_pinmux();
#endif
	debug("drv_keyboard_init - enabling clock\n");

	/*
	 * All of the Tegra board use the same clock configuration for now.
	 * This can be moved to the board specific configuration if that
	 * changes.
	 */
	clock_enable(PERIPH_ID_KBC);

	stdinname = getenv("stdin");
	memset(&kbddev, 0, sizeof(kbddev));
	strcpy(kbddev.name, DEVNAME);
	kbddev.flags = DEV_FLAGS_INPUT | DEV_FLAGS_SYSTEM;
	kbddev.putc = NULL;
	kbddev.puts = NULL;
	kbddev.getc = kbd_getc;
	kbddev.tstc = kbd_testc;
	kbddev.start = tegra_kbc_open;

	error = stdio_register(&kbddev);
	if (!error) {
		/* check if this is the standard input device */
		if (strcmp(stdinname, DEVNAME) == 0) {
			/* reassign the console */
			if (OVERWRITE_CONSOLE)
				return 1;

			error = console_assign(stdin, DEVNAME);
			if (!error)
				return 0;
			else
				return error;
		}
		return 1;
	}

	return error;
}

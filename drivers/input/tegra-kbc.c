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
#include <malloc.h>
#include <stdio_dev.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/pinmux.h>
#include <tegra-kbc.h>

#define DEVNAME "tegra-kbc"

#define KBC_MAX_GPIO	24
#define KBC_MAX_KPENT	8
#define KBC_MAX_ROW	16
#define KBC_MAX_COL	8

#define KBC_MAX_KEY	(KBC_MAX_ROW * KBC_MAX_COL)

#define TEGRA2_KBC_BASE	0x7000E200

/* KBC row scan time and delay for beginning the row scan. */
#define KBC_ROW_SCAN_TIME	16
#define KBC_ROW_SCAN_DLY	5

/*  uses a 32KHz clock so a cycle = 1/32Khz */
#define KBC_CYCLE_USEC	32

#define KBC_FIFO_TH_CNT_SHIFT(cnt)	(cnt << 14)
#define KBC_DEBOUNCE_CNT_SHIFT(cnt)	(cnt << 4)
#define KBC_CONTROL_FIFO_CNT_INT_EN	(1 << 3)
#define KBC_CONTROL_KBC_EN		(1 << 0)
#define KBC_INT_FIFO_CNT_INT_STATUS	(1 << 2)
#define KBC_KPENT_VALID	(1 << 7)

#define KBC_RPT_DLY	20
#define KBC_RPT_RATE	4

/* kbc globals */
unsigned int kbc_repoll_time;
int kbc_last_keypress;
int *kbc_plain_keycode;
int *kbc_fn_keycode;
int *kbc_shift_keycode;

static int tegra_kbc_keycode(int r, int c, int modifier)
{
	if (modifier == KEY_FN)
		return kbc_fn_keycode[(r * KBC_MAX_COL) + c];
	else if (modifier == KEY_SHIFT)
		return kbc_shift_keycode[(r * KBC_MAX_COL) + c];
	else
		return kbc_plain_keycode[(r * KBC_MAX_COL) + c];
}

/* determines if current keypress configuration can cause key ghosting */
static int is_ghost_key_config(int *rows_val, int *cols_val, int valid)
{
	int i, j, key_in_same_col = 0, key_in_same_row = 0;

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
	struct kbc_tegra *kbc = (struct kbc_tegra *)TEGRA2_KBC_BASE;
	int rows_val[KBC_MAX_KPENT], cols_val[KBC_MAX_KPENT];
	u32 kp_ent_val[(KBC_MAX_KPENT + 3) / 4];
	u32 *kp_ents = kp_ent_val;
	u32 kp_ent = 0;
	int i, j, k, valid = 0;
	int modifier = 0;

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
		if ((k == KEY_FN) || (k == KEY_SHIFT)) {
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

	if (fifo_cnt) {
		cnt = tegra_kbc_find_keys(fifo);
		/* If this is a ghost key combination report no change. */
		key = prev_key;
		if (cnt <= KBC_MAX_KPENT) {
			int prev_key_in_fifo = 0;
			/*
			 * If multiple keys are pressed such that it results in
			 * 2 or more unmodified keys, we will determine the
			 * newest key and report that to the upper layer.
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
			 * Keys were released and FIFO does not contain the
			 * previous reported key. So report a null key.
			 */
			if (i == cnt && !prev_key_in_fifo)
				key = 0;

			for (i = 0; i < cnt; i++)
				prev_fifo[i] = fifo[i];
			prev_cnt = cnt;
			prev_key = key;
		}
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
	struct kbc_tegra *kbc = (struct kbc_tegra *)TEGRA2_KBC_BASE;
	u32 val, ctl;
	char key = 0;

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

	if (!(val & KBC_INT_FIFO_CNT_INT_STATUS)) {
		ctl |= KBC_CONTROL_FIFO_CNT_INT_EN;
		writel(ctl, &kbc->control);
		return 0;
	}

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

static int kbd_testc(void)
{
	unsigned char key = kbd_fetch_char(1);

	if (key)
		kbc_last_keypress = key;

	return (key != 0);
}

static int kbd_getc(void)
{
	unsigned char key;

	if (kbc_last_keypress) {
		key = kbc_last_keypress;
		kbc_last_keypress = 0;
	} else {
		key = kbd_fetch_char(0);
	}

	return key;
}

/* configures keyboard GPIO registers to use the rows and columns */
static void config_kbc(void)
{
	struct kbc_tegra *kbc = (struct kbc_tegra *)TEGRA2_KBC_BASE;
	int i;

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
	struct kbc_tegra *kbc = (struct kbc_tegra *)TEGRA2_KBC_BASE;
	unsigned int scan_time_rows, debounce_cnt, rpt_cnt;
	u32 val = 0;

	config_kbc();

	/*
	 * The time delay between two consecutive reads of the FIFO is
	 * the sum of the repeat time and the time taken for scanning
	 * the rows. There is an additional delay before the row scanning
	 * starts. The repoll delay is computed in microseconds.
	 */
	rpt_cnt = 5 * (1000 / KBC_CYCLE_USEC);
	debounce_cnt = 2;
	scan_time_rows = (KBC_ROW_SCAN_TIME + debounce_cnt) * KBC_MAX_ROW;
	kbc_repoll_time = KBC_ROW_SCAN_DLY + scan_time_rows + rpt_cnt;
	kbc_repoll_time = (kbc_repoll_time * KBC_CYCLE_USEC) + 999;

	writel(rpt_cnt, &kbc->rpt_dly);

	val = KBC_DEBOUNCE_CNT_SHIFT(debounce_cnt);
	val |= KBC_FIFO_TH_CNT_SHIFT(1); /* set fifo interrupt threshold to 1 */
	val |= KBC_CONTROL_FIFO_CNT_INT_EN;  /* interrupt on FIFO threshold */
	val |= KBC_CONTROL_KBC_EN;     /* enable */

	writel(val, &kbc->control);

	/*
	 * Atomically clear out any remaining entries in the key FIFO
	 * and enable keyboard interrupts.
	 */
	while (1) {
		val = readl(&kbc->interrupt);
		val >>= 4;
		if (val) {
			readl(kbc->kp_ent[0]);
			readl(kbc->kp_ent[1]);
		} else {
			break;
		}
	}
	writel(0x7, &kbc->interrupt);

	return 0;
}

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

int drv_keyboard_init(void)
{
	int error;
	struct stdio_dev kbddev;
	char *stdinname;

	config_kbc_pinmux();

	/*
	 * All of the Tegra board use the same clock configuration for now.
	 * This can be moved to the board specific configuration if that
	 * changes.
	 */
	clock_enable(PERIPH_ID_KBC);

	kbc_plain_keycode = board_keyboard_config.plain_keycode;
	kbc_shift_keycode = board_keyboard_config.shift_keycode;
	kbc_fn_keycode    = board_keyboard_config.function_keycode;

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

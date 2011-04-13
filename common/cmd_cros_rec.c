/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Debug commands for Chrome OS recovery mode firmware */

#include <common.h>
#include <command.h>
#include <fat.h>
#include <lcd.h>
#include <malloc.h>
#include <mmc.h>
#include <usb.h>
#include <chromeos/firmware_storage.h>
#include <chromeos/load_firmware_helper.h>
#include <chromeos/gbb_bmpblk.h>
#include <chromeos/hardware_interface.h>
#include <chromeos/os_storage.h>
#include <chromeos/vboot_nvstorage_helper.h>

#include <bmpblk_header.h>
#include <gbb_header.h>
#include <load_kernel_fw.h>
#include <vboot_nvstorage.h>

DECLARE_GLOBAL_DATA_PTR;

#define PREFIX "cros_rec: "

#ifdef VBOOT_DEBUG
#define WARN_ON_FAILURE(action) do { \
	int return_code = (action); \
	if (return_code != 0) \
		debug(PREFIX "%s failed, returning %d\n", \
				#action, return_code); \
} while (0)
#else
#define WARN_ON_FAILURE(action) action
#endif

/* MMC dev number of SD card */
#define MMC_DEV_NUM_SD		1
#define MMC_DEV_NUM_SD_STR	"1"

#define WAIT_MS_BETWEEN_PROBING	400
#define WAIT_MS_SHOW_ERROR	2000

#define STACK_MARGIN		256
#define MAX_EXCLUDED_REGIONS	16

/* This is the data structure of the to-be-cleared memory region list. */
struct MemRegion {
	uint32_t start;
	uint32_t end;
} g_memory_region, g_excluded_regions[MAX_EXCLUDED_REGIONS];
int g_excluded_size = 0;

uint8_t *g_gbb_base = NULL;
uint64_t g_gbb_size = 0;
int g_is_dev = 0;
ScreenIndex g_cur_scr = SCREEN_BLANK;

static int is_mmc_storage_present(void)
{
	return mmc_legacy_init(MMC_DEV_NUM_SD) == 0;
}

static int is_usb_storage_present(void)
{
	int i;

	/* TODO: Seek a better way to probe USB instead of restart it */
	usb_stop();
	i = usb_init();
#ifdef CONFIG_USB_STORAGE
	if (i >= 0) {
		/* Scanning bus for storage devices, mode = 1. */
		return usb_stor_scan(1) == 0;
	}
#endif
	return 1;
}

static int write_log(void)
{
	/* TODO: Implement it when Chrome OS firmware logging is ready. */
	return 0;
}

static uint32_t get_cur_stack_addr(void)
{
	uint32_t local_var;
	uint32_t addr = (uint32_t)&local_var;

	return addr;
}

/*
 * Initializes the memory region that needs to be cleared.
 */
static void init_mem_region(uint32_t start, uint32_t end)
{
	g_memory_region.start = start;
	g_memory_region.end = end;
	g_excluded_size = 0;
}

/*
 * Excludes a memory region from the to-be-cleared region.
 * The function returns 0 on success; otherwise -1.
 */
static int exclude_mem_region(uint32_t start, uint32_t end)
{
	int i = g_excluded_size;

	if (g_excluded_size >= MAX_EXCLUDED_REGIONS) {
		debug(PREFIX "the number of excluded regions reaches"
				"the maximum: %d\n", MAX_EXCLUDED_REGIONS);
		return -1;
	}

	while (i > 0 && g_excluded_regions[i - 1].start > start) {
		g_excluded_regions[i].start = g_excluded_regions[i - 1].start;
		g_excluded_regions[i].end = g_excluded_regions[i - 1].end;
		i--;
	}
	g_excluded_regions[i].start = start;
	g_excluded_regions[i].end = end;
	g_excluded_size++;

	return 0;
}

static void zero_mem(uint32_t start, uint32_t end)
{
	if (end > start) {
		debug(PREFIX "\t[0x%08x, 0x%08x)\n", start, end);
		memset((void *)start, '\0', (size_t)(end - start));
	}
}

/*
 * Clears the memory not in excluded regions.
 */
static void clear_mem_regions(void)
{
	int i;
	uint32_t addr = g_memory_region.start;

	debug(PREFIX "clear memory regions:\n");
	for (i = 0; i < g_excluded_size; ++i) {
		zero_mem(addr, g_excluded_regions[i].start);
		if (g_excluded_regions[i].end > addr)
			addr = g_excluded_regions[i].end;
	}
	zero_mem(addr, g_memory_region.end);
}

#ifdef TEST_CLEAR_MEM_REGIONS
static int test_clear_mem_regions(void)
{
	int i;
	char s[10] = "ABCDEFGHIJ";
	char r[10] = "\0BCDEFGH\0J";
	uint32_t base = (uint32_t)s;

	init_mem_region(base, base + 10);
	/* Result: ---------- */
	exclude_mem_region(base + 1, base + 2);
	/* Result: -B-------- */
	exclude_mem_region(base + 5, base + 7);
	/* Result: -B---FG--- */
	exclude_mem_region(base + 2, base + 3);
	/* Result: -BC--FG--- */
	exclude_mem_region(base + 9, base + 10);
	/* Result: -BC--FG--J */
	exclude_mem_region(base + 4, base + 6);
	/* Result: -BC-EFG--J */
	exclude_mem_region(base + 3, base + 5);
	/* Result: -BCDEFG--J */
	exclude_mem_region(base + 2, base + 8);
	/* Result: -BCDEFGH-J */
	clear_mem_regions();
	for (i = 0; i < 10; ++i) {
		if (s[i] != r[i]) {
			debug(PREFIX "test_clear_mem_regions FAILED!\n");
			return -1;
		}
	}
	return 0;
}
#endif

/*
 * Clears the memory regions that the recovery firmware not used.
 * We do that to prevent cold boot attacks.
 */
static void clear_ram_not_in_use(void)
{
	uint32_t stack_top = get_cur_stack_addr();

	init_mem_region(0, PHYS_SDRAM_1_SIZE);

	/* Excludes the firmware text + data + bss regions. */
	exclude_mem_region(TEXT_BASE,
			TEXT_BASE + (_bss_end - _armboot_start));

	/* TODO: Remove the GBB exclusion when we store it in SPI flash. */
	exclude_mem_region(TEXT_BASE + CONFIG_OFFSET_GBB,
			TEXT_BASE + CONFIG_OFFSET_GBB + CONFIG_LENGTH_GBB);

	/* Excludes the global data. */
	exclude_mem_region((uint32_t)gd, (uint32_t)gd + sizeof(*gd));
	exclude_mem_region((uint32_t)gd->bd,
			(uint32_t)gd->bd + sizeof(*gd->bd));

	/* Excludes the whole heap. */
	exclude_mem_region(CONFIG_STACKBASE - CONFIG_SYS_MALLOC_LEN,
			CONFIG_STACKBASE);

	/* Excludes the used stack. Leave a margin for safe. */
	exclude_mem_region(stack_top - STACK_MARGIN,
			CONFIG_STACKBASE + CONFIG_STACKSIZE);

	/* Excludes the framebuffer. */
	exclude_mem_region(gd->fb_base, gd->fb_base + panel_info.vl_col *
			panel_info.vl_row * NBITS(panel_info.vl_bpix) / 8);

	clear_mem_regions();
}

static int load_and_boot_kernel(void)
{
	firmware_storage_t file;
	char *devtype;
	int devnum;
	uint64_t boot_flags;
	LoadKernelParams par;
	VbNvContext nvcxt;
	int i, status;

	devtype = getenv("devtype");
	devnum = (int)simple_strtoul(getenv("devnum"), NULL, 10);

	/* TODO move to u-boot-config */
	run_command("setenv console console=ttyS0,115200n8", 0);
	run_command("setenv bootargs ${console} ${platform_extras}", 0);

        debug(PREFIX "set_bootdev %s %x:0\n", devtype, devnum);
        if (set_bootdev(devtype, devnum, 0)) {
                debug(PREFIX "set_bootdev fail\n");
                return -1;
        }

	/* Prepare to load kernel */
	boot_flags = BOOT_FLAG_RECOVERY;
	if (g_is_dev) {
		boot_flags |= BOOT_FLAG_DEVELOPER;
	}

	if (firmware_storage_init(&file) || read_nvcontext(&file, &nvcxt)) {
		/*
		 * If we fail to initialize firmware storage or fail to read
		 * VbNvContext, we might not be able to clear the recovery
		 * cookie. As a result, next time we will still boot recovery
		 * firmware!
		 */
		debug(PREFIX "init firmware storage or clear recovery cookie "
				"failed\n");
	}

        debug(PREFIX "call load_kernel_wrapper with boot_flags: %lld\n",
			boot_flags);
        status = load_kernel_wrapper(&par, g_gbb_base, g_gbb_size,
			boot_flags, &nvcxt, NULL);

	/* Should only clear recovery cookie if successfully load kernel */
	if (status == LOAD_KERNEL_SUCCESS &&
			VbNvSet(&nvcxt, VBNV_RECOVERY_REQUEST,
				VBNV_RECOVERY_NOT_REQUESTED)) {
		/*
		 * We might still boot recovery firmware next time we boot for
		 * the same reason above.
		 */
		debug(PREFIX "fail to clear recovery cookie\n");
	}

	if (nvcxt.raw_changed && write_nvcontext(&file, &nvcxt)) {
		/*
		 * We might still boot recovery firmware next time we boot for
		 * the same reason above.
		 */
		debug(PREFIX "fail to write nvcontext\n");
	}

	file.close(file.context);

	switch (status) {
	case LOAD_KERNEL_SUCCESS:
		printf(PREFIX "Success; good kernel found on device\n");
		printf(PREFIX "partition_number: %lld\n",
				par.partition_number);
		printf(PREFIX "bootloader_address: 0x%llx\n",
				par.bootloader_address);
		printf(PREFIX "bootloader_size: 0x%llx\n", par.bootloader_size);
		printf(PREFIX "partition_guid: ");
		for (i = 0; i < 16; i++)
			printf("%02x", par.partition_guid[i]);
		putc('\n');

		source(CONFIG_LOADADDR + par.bootloader_address -
			0x100000, NULL);
		run_command("bootm ${loadaddr}", 0);
		break;
	case LOAD_KERNEL_NOT_FOUND:
		printf(PREFIX "No kernel found on device\n");
		break;
	case LOAD_KERNEL_INVALID:
		printf(PREFIX "Only invalid kernels found on device\n");
		break;
	case LOAD_KERNEL_RECOVERY:
		printf(PREFIX "Internal error; reboot to recovery mode\n");
		break;
	case LOAD_KERNEL_REBOOT:
		printf(PREFIX "Internal error; reboot to current mode\n");
		break;
	default:
		printf(PREFIX "Unexpected return status from LoadKernel: %d\n",
				status);
		return 1;
	}

	return 0;
}

static int boot_recovery_image_in_mmc(void)
{
	char buffer[CONFIG_SYS_CBSIZE];

        setenv("devtype", "mmc");
	sprintf(buffer, "mmcblk%dp", MMC_DEV_NUM_SD);
	setenv("devname", buffer);
	setenv("devnum", MMC_DEV_NUM_SD_STR);
	return load_and_boot_kernel();
}

static int boot_recovery_image_in_usb(void)
{
	/* TODO: Find the correct dev num of USB storage instead of always 0 */
        setenv("devtype", "usb");
	setenv("devname", "sda");
	setenv("devnum", "0");
	return load_and_boot_kernel();
}

static int init_gbb_in_ram(void)
{
	firmware_storage_t file;
	void *gbb_base = NULL;

	if (firmware_storage_init(&file)) {
		debug(PREFIX "init firmware storage failed\n");
		return -1;
	}

	if (load_gbb(&file, &gbb_base, &g_gbb_size)) {
		debug(PREFIX "Unable to load gbb\n");
		return -1;
	}

	g_gbb_base = gbb_base;
	return 0;
}

static int clear_screen(void)
{
	g_cur_scr = SCREEN_BLANK;
	return lcd_clear();
}

static int show_screen(ScreenIndex scr)
{
	if (g_cur_scr == scr) {
		/* No need to update. Do nothing and return success. */
		return 0;
	} else {
		g_cur_scr = scr;
		return display_screen_in_bmpblk(g_gbb_base, scr);
	}
}

int do_cros_rec(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int is_mmc, is_usb;

	WARN_ON_FAILURE(write_log());

#ifdef TEST_CLEAR_MEM_REGIONS
	test_clear_mem_regions();
#endif

	clear_ram_not_in_use();

	WARN_ON_FAILURE(init_gbb_in_ram());

	g_is_dev = is_developer_mode_gpio_asserted();

	if (!g_is_dev) {
		/* Wait for user to unplug SD card and USB storage device */
		while (is_mmc_storage_present() || is_usb_storage_present()) {
			show_screen(SCREEN_RECOVERY_MODE);
			wait_ms(WAIT_MS_BETWEEN_PROBING);
		}
	}

	for (;;) {
		/* Wait for user to plug in SD card or USB storage device */
		is_mmc = is_usb = 0;
		do {
			is_mmc = is_mmc_storage_present();
			if (!is_mmc) {
				is_usb = is_usb_storage_present();
			}
			if (!is_mmc && !is_usb) {
				show_screen(SCREEN_RECOVERY_NO_OS);
				wait_ms(WAIT_MS_BETWEEN_PROBING);
			}
		} while (!is_mmc && !is_usb);

		clear_screen();

		if (is_mmc) {
			WARN_ON_FAILURE(boot_recovery_image_in_mmc());
			/* Wait for user to unplug SD card */
			do {
				show_screen(SCREEN_RECOVERY_MISSING_OS);
				wait_ms(WAIT_MS_SHOW_ERROR);
			} while (is_mmc_storage_present());
		} else if (is_usb) {
			WARN_ON_FAILURE(boot_recovery_image_in_usb());
			/* Wait for user to unplug USB storage device */
			do {
				show_screen(SCREEN_RECOVERY_MISSING_OS);
				wait_ms(WAIT_MS_SHOW_ERROR);
			} while (is_usb_storage_present());
		}
	}

	/* This point is never reached */
	return 0;
}

U_BOOT_CMD(cros_rec, 1, 1, do_cros_rec, "recovery mode firmware", NULL);

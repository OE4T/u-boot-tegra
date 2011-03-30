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

#define ESP_PART		0xc
#define SCRIPT_PATH		"/u-boot/boot.scr.uimg"
#define SCRIPT_LOAD_ADDRESS	0x408000

#define WAIT_MS_BETWEEN_PROBING	400
#define WAIT_MS_SHOW_ERROR	2000

uint8_t *g_gbb_base = NULL;
uint64_t g_gbb_size = 0;
int g_is_dev = 0;
ScreenIndex g_cur_scr = SCREEN_BLANK;

static void sleep_ms(int msecond)
{
	const ulong start = get_timer(0);
	const ulong delay = msecond * CONFIG_SYS_HZ / 1000;

	while (!ctrlc() && get_timer(start) < delay)
		udelay(100);
}

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

static int clear_ram_not_in_use(void)
{
	/* TODO: Implement it when the memory layout is defined. */
	return 0;
}

static int load_and_boot_kernel(uint8_t *load_addr, size_t load_size)
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

        /* FIXME: Should not execute scripts in EFI system partition */
	if (fat_fsload(devtype, devnum, ESP_PART, SCRIPT_PATH,
			(void*) SCRIPT_LOAD_ADDRESS, 0) < 0) {
                debug(PREFIX "fail to load %s from %s %x:%x to 0x%08x\n",
				SCRIPT_PATH, devtype, devnum,
				ESP_PART, SCRIPT_LOAD_ADDRESS);
                return -1;
        }
        if (source(SCRIPT_LOAD_ADDRESS, NULL)) {
                debug(PREFIX "fail to execute script at 0x%08x\n",
                                SCRIPT_LOAD_ADDRESS);
                return -1;
        }

        debug(PREFIX "set_bootdev %s %x:0\n", DEVICE_TYPE, DEVICE_NUMBER);
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
	return load_and_boot_kernel((uint8_t *)CONFIG_LOADADDR, 0x01000000);
}

static int boot_recovery_image_in_usb(void)
{
	/* TODO: Find the correct dev num of USB storage instead of always 0 */
        setenv("devtype", "usb");
	setenv("devname", "sda");
	setenv("devnum", "0");
	return load_and_boot_kernel((uint8_t *)CONFIG_LOADADDR, 0x01000000);
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
	WARN_ON_FAILURE(clear_ram_not_in_use());
	WARN_ON_FAILURE(init_gbb_in_ram());

	g_is_dev = is_developer_mode_gpio_asserted();

	if (!g_is_dev) {
		/* Wait for user to plug out SD card and USB storage device */
		while (is_mmc_storage_present() || is_usb_storage_present()) {
			show_screen(SCREEN_RECOVERY_MODE);
			sleep_ms(WAIT_MS_BETWEEN_PROBING);
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
				sleep_ms(WAIT_MS_BETWEEN_PROBING);
			}
		} while (!is_mmc && !is_usb);

		clear_screen();

		if (is_mmc) {
			WARN_ON_FAILURE(boot_recovery_image_in_mmc());
			/* Wait for user to plug out SD card */
			do {
				show_screen(SCREEN_RECOVERY_MISSING_OS);
				sleep_ms(WAIT_MS_SHOW_ERROR);
			} while (is_mmc_storage_present());
		} else if (is_usb) {
			WARN_ON_FAILURE(boot_recovery_image_in_usb());
			/* Wait for user to plug out USB storage device */
			do {
				show_screen(SCREEN_RECOVERY_MISSING_OS);
				sleep_ms(WAIT_MS_SHOW_ERROR);
			} while (is_usb_storage_present());
		}
	}

	/* This point is never reached */
	return 0;
}

U_BOOT_CMD(cros_rec, 1, 1, do_cros_rec, "recovery mode firmware", NULL);

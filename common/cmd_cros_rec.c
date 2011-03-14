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
#include <lcd.h>
#include <malloc.h>
#include <mmc.h>
#include <usb.h>

#include <bmpblk_header.h>
#include <chromeos/gbb_bmpblk.h>
#include <chromeos/hardware_interface.h>

#include <gbb_header.h>
#include <load_firmware_fw.h>
#include <load_kernel_fw.h>
#include <chromeos/boot_device_impl.h>

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
#define MMC_DEV_NUM_SD 1

#define WAIT_MS_BETWEEN_PROBING 200

uint8_t *g_gbb_base = NULL;
int g_is_dev = 0;

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
#else
	return i;
#endif
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
	LoadKernelParams par;
	block_dev_desc_t *dev_desc;
	VbNvContext vnc;
	int i, status;
	GoogleBinaryBlockHeader *gbbh = (GoogleBinaryBlockHeader *)g_gbb_base;
	char buffer[CONFIG_SYS_CBSIZE];

	if ((dev_desc = get_bootdev()) == NULL) {
		printf(PREFIX "No boot device set yet\n");
		return 1;
	}

	par.gbb_data = g_gbb_base;
	par.gbb_size = CONFIG_LENGTH_GBB;
	par.shared_data_blob = NULL;
	par.shared_data_size = 0;
	par.bytes_per_lba = (uint64_t) dev_desc->blksz;
	par.ending_lba = (uint64_t) get_limit() - 1;
	par.kernel_buffer = load_addr;
	par.kernel_buffer_size = load_size;
	par.boot_flags = BOOT_FLAG_RECOVERY | BOOT_FLAG_SKIP_ADDR_CHECK;
	/* TODO: load vnc.raw from NV storage */
	par.nv_context = &vnc;

	if (g_is_dev) {
		par.boot_flags |= BOOT_FLAG_DEVELOPER;
	}

	status = LoadKernel(&par);

	if (vnc.raw_changed) {
		/* TODO: save vnc.raw to NV storage */
	}

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

		lcd_clear();

		strcpy(buffer, "console=ttyS0,115200n8 ");
		strcat(buffer, getenv("platform_extras"));
		setenv("bootargs", buffer);
		sprintf(buffer, "%lld", par.partition_number + 1);
		setenv("rootpart", buffer);

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

static int load_recovery_image_in_mmc(void)
{
	char buffer[CONFIG_SYS_CBSIZE];
	sprintf(buffer, "mmcblk%dp", MMC_DEV_NUM_SD);
	setenv("devname", buffer);
	set_bootdev("mmc", MMC_DEV_NUM_SD, 0);
	return load_and_boot_kernel((uint8_t *)CONFIG_LOADADDR, 0x01000000);
}

static int load_recovery_image_in_usb(void)
{
	/* TODO: Find the correct dev num of USB storage instead of always 0 */
	setenv("devname", "sda");
	set_bootdev("usb", 0, 0);
	return load_and_boot_kernel((uint8_t *)CONFIG_LOADADDR, 0x01000000);
}

static int init_gbb_in_ram(void)
{
	firmware_storage_t file;
	if (init_firmware_storage(&file)) {
		debug(PREFIX "init_firmware_storage failed\n");
		return -1;
	}
	g_gbb_base = malloc(CONFIG_LENGTH_GBB);
	if (!g_gbb_base) {
		debug(PREFIX "Unable to malloc g_gbb_base\n");
		return -1;
	}
	if (read_firmware_device(&file, CONFIG_OFFSET_GBB, g_gbb_base,
			CONFIG_LENGTH_GBB)) {
		debug(PREFIX "Unable to read firmware to g_gbb_base\n");
		return -1;
	}
	return 0;
}

static int show_screen(ScreenIndex scr)
{
	static ScreenIndex cur_scr;
	if (cur_scr == scr) {
		return 0;
	} else {
		cur_scr = scr;
		return display_screen_in_bmpblk(g_gbb_base, scr);
	}
}

int do_cros_rec(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
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
		while (!is_mmc_storage_present() && !is_usb_storage_present()) {
			show_screen(SCREEN_RECOVERY_NO_OS);
			sleep_ms(WAIT_MS_BETWEEN_PROBING);
		}
		if (is_mmc_storage_present()) {
			WARN_ON_FAILURE(load_recovery_image_in_mmc());
			/* Wait for user to plug out SD card */
			while (is_mmc_storage_present()) {
				show_screen(SCREEN_RECOVERY_MISSING_OS);
				sleep_ms(WAIT_MS_BETWEEN_PROBING);
			}
		} else if (is_usb_storage_present()) {
			WARN_ON_FAILURE(load_recovery_image_in_usb());
			/* Wait for user to plug out USB storage device */
			while (is_usb_storage_present()) {
				show_screen(SCREEN_RECOVERY_MISSING_OS);
				sleep_ms(WAIT_MS_BETWEEN_PROBING);
			}
		}
	}

	/* This point is never reached */
	return 0;
}

U_BOOT_CMD(cros_rec, 1, 1, do_cros_rec, "recovery mode firmware", NULL);

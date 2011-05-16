/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of developer firmware of Chrome OS Verify Boot */

#include <common.h>
#include <command.h>
#include <mmc.h>
#include <chromeos/firmware_storage.h>
#include <chromeos/gbb_bmpblk.h>
#include <chromeos/gpio.h>
#include <chromeos/load_firmware_helper.h>
#include <chromeos/load_kernel_helper.h>
#include <chromeos/os_storage.h>
#include <chromeos/vboot_nvstorage_helper.h>

#include <bmpblk_header.h>

#define PREFIX "cros_developer_firmware: "

/* TODO(clchiou): get this number from FDT? */
#define MMC_EMMC_DEVNUM	0
#define MMC_SD_DEVNUM	1

/* ASCII codes of characters */
#define CTRL_D 0x04
#define CTRL_U 0x15
#define ESCAPE 0x1b

int do_cros_normal_firmware(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[]);

static void beep(void)
{
	/* TODO: implement beep */
	debug(PREFIX "beep\n");
}

static int unblocked_getc(int *c)
{
	/* TODO(crosbug/15243): Cannot read control-* from keyboard */

	/* does not check whether we have console or not */
	if (tstc()) {
		*c = getc();
		return 1;
	} else
		return 0;
}

/* This function only returns when verification failed */
static void load_and_boot_kernel(void *gbb_data, uint64_t gbb_size,
		uint64_t boot_flags)
{
	LoadKernelParams params;
	VbNvContext nvcxt;
	int status;

	if (read_nvcontext(&nvcxt)) {
		/*
		 * Even if we can't read nvcxt, we continue anyway because this
		 * is developer firmware
		 */
		debug(PREFIX "fail to read nvcontext\n");
	}

	prepare_bootargs();

	status = load_kernel_wrapper(&params, gbb_data, gbb_size,
			boot_flags, &nvcxt, NULL);

	debug(PREFIX "load_kernel_wrapper returns %d\n", status);

	if (VbNvTeardown(&nvcxt) ||
			(nvcxt.raw_changed && write_nvcontext(&nvcxt))) {
		/*
		 * Even if we can't read nvcxt, we continue anyway because this
		 * is developer firmware
		 */
		debug(PREFIX "fail to write nvcontext\n");
	}

	if (status == LOAD_KERNEL_SUCCESS)
		boot_kernel(&params); /* this function never returns */
}

int do_cros_developer_firmware(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	firmware_storage_t file;
	void *gbb_data = NULL;
	uint64_t gbb_size = 0,
		 boot_flags = BOOT_FLAG_DEV_FIRMWARE | BOOT_FLAG_DEVELOPER;
	ulong start = 0, time = 0, last_time = 0;
	int c, is_after_20_seconds = 0;

	if (!is_developer_mode_gpio_asserted()) {
		debug(PREFIX "developer switch is off; reboot to recovery!\n");
		reboot_to_recovery_mode(NULL, VBNV_RECOVERY_RW_DEV_MISMATCH);
	}

	if (firmware_storage_init(&file) ||
			load_gbb(&file, &gbb_data, &gbb_size)) {
		/*
		 * FIXME: We can't read gbb and so can't show a face on screen;
		 * how should we do when this happen? Trap in infinite loop?
		 */
		debug(PREFIX "cannot load gbb\n");
		while (1);
	}

	/* we don't care whether close operation fails */
	if (file.close(file.context)) {
		debug(PREFIX "close firmware storage failed\n");
	}

	if (display_screen_in_bmpblk(gbb_data, SCREEN_DEVELOPER_MODE)) {
		debug(PREFIX "cannot display stuff on LCD screen\n");
	}

	start = get_timer(0);
	while (1) {
		udelay(100);

		last_time = time;
		time = get_timer(start);
#ifdef DEBUG
		/* only output when time advances at least 1 second */
		if (time / CONFIG_SYS_HZ > last_time / CONFIG_SYS_HZ)
			debug(PREFIX "time ~ %d sec\n", time / CONFIG_SYS_HZ);
#endif

		/* Beep twice when time > 20 seconds */
		if (!is_after_20_seconds && time > 20 * CONFIG_SYS_HZ) {
			beep(); udelay(500); beep();
			is_after_20_seconds = 1;
			continue;
		}

		/* Boot from internal storage after 30 seconds */
		if (time > 30 * CONFIG_SYS_HZ)
			break;

		if (!unblocked_getc(&c))
			continue;

		debug(PREFIX "unblocked_getc() == 0x%x\n", c);

		/* Load and boot kernel from USB or SD card */
		if (CTRL_U == c) {
			/*
			 * TODO(waihong): Find the correct dev num of USB
			 * storage instead of always 0.
			 */
			if (is_usb_storage_present() &&
					!set_bootdev("usb", 0, 0)) {
				debug(PREFIX "Probed usb\n");
				load_and_boot_kernel(gbb_data, gbb_size,
						boot_flags);
			} else if (is_mmc_storage_present(MMC_SD_DEVNUM) &&
					!set_bootdev("mmc", MMC_SD_DEVNUM, 0)) {
				debug(PREFIX "Probed mmc\n");
				load_and_boot_kernel(gbb_data, gbb_size,
						boot_flags);
			} else {
				debug(PREFIX "Fail to probe usb and mmc\n");
			}

			beep();
		}

		/* Boot from internal storage when user pressed Ctrl-D */
		if (CTRL_D == c)
			break;

		if (c == ' ' || c == '\r' || c == '\n' || c == ESCAPE) {
			debug(PREFIX "reboot to recovery mode\n");
			reboot_to_recovery_mode(NULL,
					VBNV_RECOVERY_RW_DEV_SCREEN);
		}
	}

	debug(PREFIX "boot from internal storage\n");
	if (is_mmc_storage_present(MMC_EMMC_DEVNUM) &&
			!set_bootdev("mmc", MMC_EMMC_DEVNUM, 0))
		load_and_boot_kernel(gbb_data, gbb_size, boot_flags);

	printf(PREFIX "cannot boot from internal storage!\n");
	while (1);

	return 0;
}

U_BOOT_CMD(cros_developer_firmware, 1, 1, do_cros_developer_firmware,
		"verified boot developer firmware", NULL);

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
#include <chromeos/load_firmware_helper.h>
#include <chromeos/load_kernel_helper.h>
#include <chromeos/os_storage.h>
#include <chromeos/vboot_nvstorage_helper.h>

#include <bmpblk_header.h>

#define PREFIX "cros_developer_firmware: "

int do_cros_normal_firmware(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[]);

static void beep(void)
{
	/* TODO: implement beep */
	debug(PREFIX "beep\n");
}

static int is_ctrlu(int c)
{
	return 0;
}

static int is_ctrld(int c)
{
	return 0;
}

static int is_escape(int c)
{
	return 0;
}

static int is_space(int c)
{
	return 0;
}

static int is_enter(int c)
{
	return 0;
}

int do_cros_developer_firmware(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	firmware_storage_t file;
	void *gbb_data = NULL;
	uint64_t gbb_size = 0;
	ulong start = 0, time = 0;
	int c, is_after_20_seconds = 0;

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
	file.close(file.context);

	if (display_screen_in_bmpblk(gbb_data, SCREEN_DEVELOPER_MODE)) {
		debug(PREFIX "cannot display stuff on LCD screen\n");
	}

	start = get_timer(0);
	while (1) {
		time = get_timer(start);

		/* Beep twice when time > 20 seconds */
		if (!is_after_20_seconds && time > 20 * CONFIG_SYS_HZ) {
			beep(); udelay(500); beep();
			is_after_20_seconds = 1;
			continue;
		}

		/* Fall back to normal firmware when time > 30 seconds */
		if (time > 30 * CONFIG_SYS_HZ)
			break;

		c = getc();
		debug(PREFIX "getc() == 0x%x\n", c);

		if (is_ctrlu(c)) {
			/* TODO: load and boot kernel from USB or SD card */
		}

		/* Fall back to normal firmware when user pressed Ctrl-D */
		if (is_ctrld(c))
			break;

		if (is_space(c) || is_enter(c) || is_escape(c))
			reboot_to_recovery_mode(NULL,
					VBNV_RECOVERY_RW_DEV_SCREEN);

		udelay(100);
	}

	return do_cros_normal_firmware(NULL, 0, 0, NULL);
}

U_BOOT_CMD(cros_developer_firmware, 1, 1, do_cros_developer_firmware,
		"verified boot developer firmware", NULL);

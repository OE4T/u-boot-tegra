/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of boot stub of Chrome OS Verify Boot firmware */

#include <common.h>
#include <command.h>
#include <chromeos/hardware_interface.h>

/* Verify Boot interface */
#include <gbb_header.h>
#include <load_firmware_fw.h>

#define PREFIX "cros_bootstub: "

#ifdef DEBUG
#define WARN_ON_FAILURE(action) do { \
	int return_code = (action); \
	if (return_code != 0) \
		debug(PREFIX "%s failed, returning %d\n", \
				#action, return_code); \
} while (0)
#else
#define WARN_ON_FAILURE(action) action
#endif

#define FIRMWARE_RECOVERY	0
#define FIRMWARE_A		1
#define FIRMWARE_B		2

int do_cros_bootstub(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int firmware_index, primary_firmware;

	/* Boot Stub ******************************************************** */

	/* TODO Start initializing chipset */

	if (is_firmware_write_protect_gpio_asserted())
		WARN_ON_FAILURE(lock_down_eeprom());

	WARN_ON_FAILURE(initialize_tpm());

	if (is_s3_resume() && !is_debug_reset_mode_field_containing_cookie()) {
		WARN_ON_FAILURE(lock_tpm());
		/* TODO Jump to S3 resume pointer and should never return */
		return 0;
	}

	if (is_recovery_mode_gpio_asserted() ||
	    is_recovery_mode_field_containing_cookie()) {
		firmware_index = FIRMWARE_RECOVERY;
	} else {
		if (is_try_firmware_b_field_containing_cookie())
			primary_firmware = FIRMWARE_B;
		else
			primary_firmware = FIRMWARE_A;

		/* TODO call LoadFirmware */
	}

	debug(PREFIX "load firmware of index %d\n", firmware_index);

	WARN_ON_FAILURE(lock_tpm_rewritable_firmware_index());

	if (firmware_index == FIRMWARE_A) {
		/* TODO Jump to firmware A and should never return */
	}

	if (firmware_index == FIRMWARE_B) {
		/* TODO Jump to firmware B and should never return */
	}

	/* assert(firmware_index == FIRMWARE_RECOVERY); */

	/* Recovery Firmware ************************************************ */

	return 0;
}

U_BOOT_CMD(cros_bootstub, 1, 1, do_cros_bootstub, NULL, NULL);

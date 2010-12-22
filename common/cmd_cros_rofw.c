/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of read-only part of Chrome OS Verify Boot firmware */

#include <common.h>
#include <command.h>
#include <chromeos/firmware_storage.h>
#include <chromeos/boot_device_impl.h>

/* Verify Boot interface */
#include <boot_device.h>
#include <gbb_header.h>
#include <load_firmware_fw.h>
#include <load_kernel_fw.h>

#ifdef VBOOT_DEBUG
#define WARN_ON_FAILURE(action) do { \
	int return_code = (action); \
	if (return_code != 0) \
		debug("Boot Stub: %s failed, returning %d\n", \
				#action, return_code); \
} while (0)
#else
#define WARN_ON_FAILURE(action) action
#endif

/* TODO: Replace dummy functions below with real implementation. */

/* Returns 0 if false, nonzero if true */
static int is_firmware_write_protect_gpio_asserted(void) { return 0; }
static int is_recovery_mode_gpio_asserted(void) { return 0; }
static int is_s3_resume(void) { return 0; }
static int is_debug_reset_mode_field_containing_cookie(void) { return 0; }
static int is_recovery_mode_field_containing_cookie(void) { return 0; }
static int is_try_firmware_b_field_containing_cookie(void) { return 0; }
static int is_developer_mode_gpio_asserted(void) { return 0; }

/* Returns 0 if success, nonzero if error. */
static int lock_down_eeprom(void) { return 0; }
static int initialize_tpm(void) { return 0; }
static int lock_tpm(void) { return 0; }
static int lock_tpm_rewritable_firmware_index(void) { return 0; }

#define FIRMWARE_RECOVERY	0
#define FIRMWARE_A		1
#define FIRMWARE_B		2

int do_cros_rofw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
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

	debug("Boot Stub: Load firmware of index %d\n", firmware_index);

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

U_BOOT_CMD(cros_rofw, 0, 1, do_cros_rofw, NULL, NULL);

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

/*
 * verified_firmware_t is used for storing returned data of LoadFirmware() and
 * GetFirmwareBody().
 */
typedef struct {
	/*
	 * This field is output of LoadFirmware(). See
	 * vboot_reference/firmware/include/load_firmware_fw.h for
	 * documentation.
	 */
	int firmware_index;

	/* Rewritable firmware data loaded by GetFirmwareBody(). */
	uint8_t *firmware_body[2];
} verified_firmware_t;

/*
 * Load and verify rewritable firmware. A wrapper of LoadFirmware() function.
 *
 * Returns what is returned by LoadFirmware().
 *
 * For documentation of return values of LoadFirmware(), <primary_firmware>, and
 * <boot_flags>, please refer to
 * vboot_reference/firmware/include/load_firmware_fw.h
 *
 * The kernel key found in firmware image is stored in <kernel_sign_key_blob>
 * and <kernel_sign_key_size>.
 *
 * Output of LoadFirmware() and GetFirmwareBody() is stored in
 * struct pointed by <verified_firmware>.
 */
int load_firmware(int primary_firmware, int boot_flags,
		void *kernel_sign_key_blob, uint64_t kernel_sign_key_size,
		verified_firmware_t *verified_firmware)
{
	return LOAD_FIRMWARE_RECOVERY;
}

void jump_to_firmware(void (*rwfw_entry_point)(void))
{
	debug(PREFIX "jump to firmware %p\n", rwfw_entry_point);
}

int do_cros_bootstub(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int status = LOAD_FIRMWARE_RECOVERY;
	int primary_firmware, boot_flags = 0;
	verified_firmware_t vf;
	uint8_t *kernel_sign_key_blob = (uint8_t *) CONFIG_KERNEL_SIGN_KEY_BLOB;
	uint64_t kernel_sign_key_size = CONFIG_KERNEL_SIGN_KEY_SIZE;

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
		debug(PREFIX "boot recovery firmware\n");
	} else {
		if (is_try_firmware_b_field_containing_cookie())
			primary_firmware = FIRMWARE_B;
		else
			primary_firmware = FIRMWARE_A;

		if (is_developer_mode_gpio_asserted())
			boot_flags |= BOOT_FLAG_DEVELOPER;

		status = load_firmware(primary_firmware, boot_flags,
				kernel_sign_key_blob, kernel_sign_key_size,
				&vf);
	}

	WARN_ON_FAILURE(lock_tpm_rewritable_firmware_index());

	if (status == LOAD_FIRMWARE_SUCCESS) {
		debug(PREFIX "jump to rewritable firmware %d "
				"and never return\n",
				vf.firmware_index);
		jump_to_firmware((void (*)(void))
				vf.firmware_body[vf.firmware_index]);

		debug(PREFIX "error: should never reach here! "
				"jump to recovery firmware\n");
	}

	/* TODO Jump to recovery firmware and never return */

	debug(PREFIX "error: should never reach here!\n");
	return 1;
}

U_BOOT_CMD(cros_bootstub, 1, 1, do_cros_bootstub, "verified boot stub firmware",
		NULL);

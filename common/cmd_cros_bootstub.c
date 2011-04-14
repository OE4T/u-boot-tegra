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
#include <malloc.h>
#include <chromeos/firmware_storage.h>
#include <chromeos/hardware_interface.h>
#include <chromeos/load_firmware_helper.h>
#include <chromeos/vboot_nvstorage_helper.h>

/* Verify Boot interface */
#include <gbb_header.h>
#include <load_firmware_fw.h>
#include <vboot_nvstorage.h>
#include <vboot_struct.h>

#define PREFIX "cros_bootstub: "

#define WARN_ON_FAILURE(action) do { \
	int return_code = (action); \
	if (return_code != 0) \
		printf(PREFIX "%s failed, returning %d\n", \
				#action, return_code); \
} while (0)

/*
 * Read recovery firmware into <recovery_firmware_buffer>.
 *
 * Return 0 on success, non-zero on error.
 */
int load_recovery_firmware(firmware_storage_t *file,
		uint8_t *recovery_firmware_buffer)
{
	int retval;

	retval = firmware_storage_read(file,
			CONFIG_OFFSET_RECOVERY, CONFIG_LENGTH_RECOVERY,
			recovery_firmware_buffer);
	if (retval) {
		debug(PREFIX "cannot load recovery firmware\n");
	}

	return retval;
}

void jump_to_firmware(void (*firmware_entry_point)(void))
{
	debug(PREFIX "jump to firmware %p\n", firmware_entry_point);

	cleanup_before_linux();

	/* should never return! */
	firmware_entry_point();

	/* FIXME(clchiou) Bring up a sad face as boot has failed */
	enable_interrupts();
	debug(PREFIX "error: firmware returns\n");
	while (1);
}

int do_cros_bootstub(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int status = LOAD_FIRMWARE_RECOVERY;
	firmware_storage_t file;
	VbNvContext nvcxt;
	uint64_t boot_flags = 0;
	uint32_t debug_reset_mode = 0;
	uint32_t recovery_request = 0;
	uint32_t reason = VBNV_RECOVERY_RO_UNSPECIFIED;
	uint8_t *firmware_data;

	if (firmware_storage_init(&file)) {
		/* FIXME(clchiou) Bring up a sad face as boot has failed */
		debug(PREFIX "init_firmware_storage fail\n");
		while (1);
	}

	if (is_firmware_write_protect_gpio_asserted())
		WARN_ON_FAILURE(file.lock_device(file.context));

	if (initialize_tpm()) {
		debug(PREFIX "init tpm failed\n");
		reason = VBNV_RECOVERY_RO_TPM_ERROR;
		goto RECOVERY;
	}

	if (read_nvcontext(&file, &nvcxt) ||
			VbNvGet(&nvcxt, VBNV_DEBUG_RESET_MODE,
				&debug_reset_mode) ||
			VbNvGet(&nvcxt, VBNV_RECOVERY_REQUEST,
				&recovery_request)) {
		debug(PREFIX "fail to read nvcontext\n");
		goto RECOVERY;
	}

	/* clear VBNV_DEBUG_RESET_MODE after read */
	if (VbNvSet(&nvcxt, VBNV_DEBUG_RESET_MODE, 0)) {
		debug(PREFIX "fail to write nvcontext\n");
		goto RECOVERY;
	}

	if (is_s3_resume() && !debug_reset_mode) {
		if (lock_tpm()) {
			debug(PREFIX "lock tpm failed\n");
			reason = VBNV_RECOVERY_RO_TPM_ERROR;
			goto RECOVERY;
		}

		/* TODO Jump to S3 resume pointer and should never return */
		return 0;
		/* reason = VBNV_RECOVERY_RO_S3_RESUME; */
		/* goto RECOVERY; */
	}

	if (recovery_request != VBNV_RECOVERY_NOT_REQUESTED) {
		debug(PREFIX "boot recovery cookie set\n");
		reason = recovery_request;
		goto RECOVERY;
	}

	if (is_recovery_mode_gpio_asserted()) {
		debug(PREFIX "recovery button pressed\n");
		reason = VBNV_RECOVERY_RO_MANUAL;
		goto RECOVERY;
	}

	if (is_developer_mode_gpio_asserted())
		boot_flags |= BOOT_FLAG_DEVELOPER;

	status = load_firmware_wrapper(&file,
			boot_flags, &nvcxt, NULL, &firmware_data);

	if (nvcxt.raw_changed && write_nvcontext(&file, &nvcxt)) {
		debug(PREFIX "fail to write nvcontext\n");
		goto RECOVERY;
        }

	if (lock_tpm_rewritable_firmware_index()) {
		debug(PREFIX "lock tpm rewritable firmware index failed\n");
		reason = VBNV_RECOVERY_RO_TPM_ERROR;
		goto RECOVERY;
	}

	if (status == LOAD_FIRMWARE_SUCCESS) {
		jump_to_firmware((void (*)(void)) firmware_data);
	} else {
		reason = VBNV_RECOVERY_RO_INVALID_RW;
	}

RECOVERY:
	debug(PREFIX "write to recovery cookie\n");

	if (VbNvSet(&nvcxt, VBNV_RECOVERY_REQUEST, reason) ||
			VbNvTeardown(&nvcxt) ||
			(nvcxt.raw_changed && write_nvcontext(&file, &nvcxt))) {
		/* FIXME: bring up a sad face? */
		debug(PREFIX "error: cannot write recovery cookie");
		while (1);
	}

	debug(PREFIX "jump to recovery firmware and never return\n");

	firmware_data = malloc(CONFIG_LENGTH_RECOVERY);
	WARN_ON_FAILURE(load_recovery_firmware(&file, firmware_data));
	jump_to_firmware((void (*)(void)) firmware_data);

	/* never reach here */
	return 1;
}

U_BOOT_CMD(cros_bootstub, 1, 1, do_cros_bootstub, "verified boot stub firmware",
		NULL);

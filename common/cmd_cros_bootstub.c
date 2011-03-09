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
#include <chromeos/hardware_interface.h>

/* Verify Boot interface */
#include <gbb_header.h>
#include <load_firmware_fw.h>

#define PREFIX "cros_bootstub: "

#define WARN_ON_FAILURE(action) do { \
	int return_code = (action); \
	if (return_code != 0) \
		printf(PREFIX "%s failed, returning %d\n", \
				#action, return_code); \
} while (0)

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
 * Read <count> bytes, starting from <offset>, from firmware storage device <f>
 * into buffer <buf>.
 *
 * Return 0 on success, non-zero on error.
 */
int read_firmware_device(firmware_storage_t *f, off_t offset, void *buf,
		size_t count)
{
	ssize_t size;

	if (f->seek(f->context, offset, SEEK_SET) < 0) {
		debug(PREFIX "seek to address 0x%08x fail\n", offset);
		return -1;
	}

	size = 0;
	while (count > 0 && (size = f->read(f->context, buf, count)) > 0) {
		count -= size;
		buf += size;
	}

	if (size < 0) {
		debug(PREFIX "an error occur when read firmware: %d\n", size);
		return -1;
	}

	if (count > 0) {
		debug(PREFIX "cannot read all data: %d\n", count);
		return -1;
	}

	return 0;
}

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

/*
 * Read recovery firmware into <recovery_firmware_buffer>.
 *
 * Return 0 on success, non-zero on error.
 */
int load_recovery_firmware(uint8_t *recovery_firmware_buffer)
{
	firmware_storage_t f;
	int retval;

	if (init_firmware_storage(&f)) {
		debug(PREFIX "init_firmware_storage fail\n");
		return -1;
	}

	retval = read_firmware_device(&f, CONFIG_OFFSET_RECOVERY,
			recovery_firmware_buffer, CONFIG_LENGTH_RECOVERY);
	if (retval) {
		debug(PREFIX "cannot load recovery firmware\n");
	}

	release_firmware_storage(&f);
	return retval;
}

void jump_to_firmware(void (*firmware_entry_point)(void))
{
	debug(PREFIX "jump to firmware %p\n", firmware_entry_point);

	cleanup_before_linux();

	/* should never return! */
	firmware_entry_point();

	enable_interrupts();
	debug(PREFIX "error: firmware returns\n");

	/* FIXME(clchiou) Bring up a sad face as boot has failed */
	while (1);
}

int do_cros_bootstub(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int status = LOAD_FIRMWARE_RECOVERY;
	int primary_firmware, boot_flags = 0;
	verified_firmware_t vf;
	uint8_t *recovery_firmware;
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

	debug(PREFIX "jump to recovery firmware and never return\n");

	recovery_firmware = malloc(CONFIG_LENGTH_RECOVERY);
	WARN_ON_FAILURE(load_recovery_firmware(recovery_firmware));
	jump_to_firmware((void (*)(void)) recovery_firmware);

	debug(PREFIX "error: should never reach here!\n");
	return 1;
}

U_BOOT_CMD(cros_bootstub, 1, 1, do_cros_bootstub, "verified boot stub firmware",
		NULL);

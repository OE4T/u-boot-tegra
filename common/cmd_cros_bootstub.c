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
#include <vboot_struct.h>

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
 * Read <count> bytes, starting from <offset>, from firmware storage
 * device <file> into buffer <buf>.
 *
 * Return 0 on success, non-zero on error.
 */
int read_firmware_device(firmware_storage_t *file, off_t offset, void *buf,
		size_t count)
{
	ssize_t size;

	if (file->seek(file->context, offset, SEEK_SET) < 0) {
		debug(PREFIX "seek to address 0x%08x fail\n", offset);
		return -1;
	}

	size = 0;
	while (count > 0 &&
			(size = file->read(file->context, buf, count)) > 0) {
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
 * Read verification block on <vblock_offset> from <file>.
 *
 * The pointer to verification block is stored in <vblock_ptr>, and the size of
 * the block is stored in <vblock_size_ptr>.
 *
 * On success, the space for storing verification block is malloc()'ed, and has
 * to be free()'ed by the caller.
 *
 * Return 0 if success, non-zero if fail.
 */
int read_verification_block(firmware_storage_t *file, off_t vblock_offset,
		void **vblock_ptr, uint64_t *vblock_size_ptr)
{
	VbKeyBlockHeader kbh;
	VbFirmwarePreambleHeader fph;
	uint64_t key_block_size, vblock_size;
	void *vblock;

	/* read key block header */
	if (read_firmware_device(file, vblock_offset, &kbh, sizeof(kbh))) {
		debug(PREFIX "read key block fail\n");
		return -1;
	}
	key_block_size = kbh.key_block_size;

	/* read firmware preamble header */
	if (read_firmware_device(file, vblock_offset + key_block_size, &fph,
				sizeof(fph))) {
		debug(PREFIX "read preamble fail\n");
		return -1;
	}
	vblock_size = key_block_size + fph.preamble_size;

	vblock = malloc(vblock_size);
	if (!vblock) {
		debug(PREFIX "malloc verification block fail\n");
		return -1;
	}

	if (read_firmware_device(file, vblock_offset, vblock, vblock_size)) {
		debug(PREFIX "read verification block %d fail\n");
		free(vblock);
		return -1;
	}

	*vblock_ptr = vblock;
	*vblock_size_ptr = vblock_size;

	return 0;
}

/*
 * Initialize <params> for LoadFirmware().
 *
 * Return 0 if success, non-zero if error.
 */
int init_params(LoadFirmwareParams *params, firmware_storage_t *file,
		const off_t vblock_offset[], int boot_flags,
		void *vb_shared_data_blob, uint64_t vb_shared_data_size)
{
	memset(params, '\0', sizeof(*params));

	/* read gbb */
	params->gbb_size = CONFIG_LENGTH_GBB;
	params->gbb_data = malloc(CONFIG_LENGTH_GBB);
	if (!params->gbb_data) {
		debug(PREFIX "cannot malloc gbb\n");
		return -1;
	}
	if (read_firmware_device(file, CONFIG_OFFSET_GBB,
				params->gbb_data,
				params->gbb_size)) {
		debug(PREFIX "read gbb fail\n");
		return -1;
	}

	/* read verification block 0 and 1 */
	if (read_verification_block(file, vblock_offset[0],
				&params->verification_block_0,
				&params->verification_size_0)) {
		debug(PREFIX "read verification block 0 fail\n");
		return -1;
	}
	if (read_verification_block(file, vblock_offset[1],
				&params->verification_block_1,
				&params->verification_size_1)) {
		debug(PREFIX "read verification block 1 fail\n");
		return -1;
	}

	params->boot_flags = boot_flags;
	params->caller_internal = file;
	params->shared_data_blob = vb_shared_data_blob;
	params->shared_data_size = vb_shared_data_size;

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
 * The shared data blob for the firmware image is stored in
 * <vb_shared_data_blob> and <vb_shared_data_size>.
 *
 * Pointer to loaded firmware is stored in <firmware_data_ptr>.
 */
int load_firmware(uint8_t **firmware_data_ptr,
		int primary_firmware, int boot_flags,
		void *vb_shared_data_blob, uint64_t vb_shared_data_size)
{
	/*
	 * Offsets of verification blocks are
	 * vblock_offset[primary_firmware][verification_block_index].
	 *
	 * Offsets of firmware data are
	 * data_offset[primary_firmware][firmware_data_index].
	 */
	const off_t vblock_offset[2][2] = {
		{ CONFIG_OFFSET_FW_A_KEY, CONFIG_OFFSET_FW_B_KEY },
		{ CONFIG_OFFSET_FW_B_KEY, CONFIG_OFFSET_FW_A_KEY },
	};
	const off_t data_offset[2][2] = {
		{ CONFIG_OFFSET_FW_A_DATA, CONFIG_OFFSET_FW_B_DATA },
		{ CONFIG_OFFSET_FW_B_DATA, CONFIG_OFFSET_FW_A_DATA },
	};
	int status = LOAD_FIRMWARE_RECOVERY;
	LoadFirmwareParams params;
	firmware_storage_t file;

	if (init_firmware_storage(&file)) {
		debug(PREFIX "init_firmware_storage fail\n");
		return FIRMWARE_RECOVERY;
	}
	GetFirmwareBody_setup(&file, data_offset[primary_firmware][0],
			data_offset[primary_firmware][1]);

	if (init_params(&params, &file, vblock_offset[primary_firmware],
				boot_flags, vb_shared_data_blob,
				vb_shared_data_size)) {
		debug(PREFIX "init LoadFirmware parameters fail\n");
	} else
		status = LoadFirmware(&params);

	if (status == LOAD_FIRMWARE_SUCCESS) {
		debug(PREFIX "will jump to rewritable firmware %d\n",
				(int) params.firmware_index);
		*firmware_data_ptr = file.firmware_body[params.firmware_index];

		/* set to null so that *firmware_data_ptr is not disposed */
		file.firmware_body[params.firmware_index] = NULL;
	}

	release_firmware_storage(&file);
	GetFirmwareBody_dispose(&file);

	if (params.gbb_data)
		free(params.gbb_data);
	if (params.verification_block_0)
		free(params.verification_block_0);
	if (params.verification_block_1)
		free(params.verification_block_1);

	return status;
}

/*
 * Read recovery firmware into <firmware_data_buffer>.
 *
 * Return 0 on success, non-zero on error.
 */
int load_firmware_data(uint8_t *firmware_data_buffer)
{
	firmware_storage_t file;
	int retval;

	if (init_firmware_storage(&file)) {
		debug(PREFIX "init_firmware_storage fail\n");
		return -1;
	}

	retval = read_firmware_device(&file, CONFIG_OFFSET_RECOVERY,
			firmware_data_buffer, CONFIG_LENGTH_RECOVERY);
	if (retval) {
		debug(PREFIX "cannot load recovery firmware\n");
	}

	release_firmware_storage(&file);
	return retval;
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
	uint8_t *firmware_data;
	uint8_t *vb_shared_data_blob = (uint8_t *) CONFIG_VB_SHARED_DATA_BLOB;
	uint64_t vb_shared_data_size = CONFIG_VB_SHARED_DATA_SIZE;

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

		status = load_firmware(&firmware_data,
				primary_firmware, boot_flags,
				vb_shared_data_blob, vb_shared_data_size);
	}

	WARN_ON_FAILURE(lock_tpm_rewritable_firmware_index());

	if (status == LOAD_FIRMWARE_SUCCESS) {
		jump_to_firmware((void (*)(void)) firmware_data);

		debug(PREFIX "error: should never reach here! "
				"jump to recovery firmware\n");
	}

	debug(PREFIX "jump to recovery firmware and never return\n");

	firmware_data = malloc(CONFIG_LENGTH_RECOVERY);
	WARN_ON_FAILURE(load_firmware_data(firmware_data));
	jump_to_firmware((void (*)(void)) firmware_data);

	debug(PREFIX "error: should never reach here!\n");
	return 1;
}

U_BOOT_CMD(cros_bootstub, 1, 1, do_cros_bootstub, "verified boot stub firmware",
		NULL);

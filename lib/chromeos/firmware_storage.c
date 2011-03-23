/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <malloc.h>
#include <chromeos/firmware_storage.h>

#include <gbb_header.h>
#include <load_firmware_fw.h>
#include <vboot_nvstorage.h>
#include <vboot_struct.h>

#define PREFIX "firmware_storage: "

int lock_down_firmware_storage(void)
{
	/* TODO Implement this function */
	return 0;
}

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

int is_debug_reset_mode_field_containing_cookie(void)
{
	/* TODO Implement this function */
	return 0;
}

int is_recovery_mode_field_containing_cookie(void)
{
	/* TODO Implement this function */
	return 0;
}

int is_try_firmware_b_field_containing_cookie(void)
{
	/* TODO Implement this function */
	return 0;
}

#undef PREFIX
#define PREFIX "load_gbb: "

#define EMSG_INIT_FIRMWARE_STORAGE \
	PREFIX "error: initialize_firmware_storage() fail\n"
#define EMSG_MALLOC \
	PREFIX "error: malloc(%d) fail\n"
#define EMSG_READ_GBB \
	PREFIX "error: read gbb fail\n"
#define EMSG_RELEASE_FIRMWARE_STORAGE \
	PREFIX "error: release_firmware_storage() fail\n"

int load_gbb(firmware_storage_t *file, void **gbb_data_ptr,
		uint64_t *gbb_size_ptr)
{
	int retcode = -1;
	void *gbb_data;
	uint64_t gbb_size;
	firmware_storage_t myfile;

	gbb_data = malloc(CONFIG_LENGTH_GBB);
	gbb_size = CONFIG_LENGTH_GBB;
	if (!gbb_data) {
		debug(EMSG_MALLOC, CONFIG_LENGTH_GBB);
		goto CLEANUP_GBB;
	}

	if (!file) {
		file = &myfile;
		if (init_firmware_storage(file)) {
			debug(EMSG_INIT_FIRMWARE_STORAGE);
			goto CLEANUP_FILE;
		}
	}

	if (read_firmware_device(file, CONFIG_OFFSET_GBB, gbb_data,
				gbb_size)) {
		debug(EMSG_READ_GBB);
	} else
		retcode = 0;

CLEANUP_FILE:
	if (file == &myfile && release_firmware_storage(file)) {
		debug(EMSG_RELEASE_FIRMWARE_STORAGE);
		retcode = -1;
	}

CLEANUP_GBB:
	if (!retcode) {
		*gbb_data_ptr = gbb_data;
		*gbb_size_ptr = gbb_size;
	} else
		free(gbb_data);

	return retcode;
}

#undef PREFIX
#define PREFIX "read_verification_block: "

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
static int read_verification_block(firmware_storage_t *file,
		off_t vblock_offset,
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

#undef PREFIX
#define PREFIX "load_firmware_wrapper: "

int load_firmware_wrapper(firmware_storage_t *file,
		int primary_firmware, int boot_flags, void *shared_data_blob,
		uint8_t **firmware_data_ptr)
{
	/*
	 * Offsets of verification blocks are
	 * vblock_offset[primary_firmware][verification_block_index].
	 *
	 * Offsets of firmware data are
	 * data_offset[primary_firmware][firmware_data_index].
	 */
	const off_t vblock_offset[2][2] = {
		{ CONFIG_OFFSET_VBLOCK_A, CONFIG_OFFSET_VBLOCK_B },
		{ CONFIG_OFFSET_VBLOCK_B, CONFIG_OFFSET_VBLOCK_A },
	};
	const off_t data_offset[2][2] = {
		{ CONFIG_OFFSET_FW_MAIN_A, CONFIG_OFFSET_FW_MAIN_B },
		{ CONFIG_OFFSET_FW_MAIN_B, CONFIG_OFFSET_FW_MAIN_A },
	};

	int status = LOAD_FIRMWARE_RECOVERY;
	LoadFirmwareParams params;
        VbNvContext vnc;

	GetFirmwareBody_setup(file, data_offset[primary_firmware][0],
			data_offset[primary_firmware][1]);

	memset(&params, '\0', sizeof(params));

	if (load_gbb(file, &params.gbb_data, &params.gbb_size)) {
		debug(PREFIX "error: read gbb fail\n");
		goto CLEANUP;
	}

	if (read_verification_block(file, vblock_offset[primary_firmware][0],
				&params.verification_block_0,
				&params.verification_size_0)) {
		debug(PREFIX "error: read verification block 0 fail\n");
		goto CLEANUP;
	}

	if (read_verification_block(file, vblock_offset[primary_firmware][1],
				&params.verification_block_1,
				&params.verification_size_1)) {
		debug(PREFIX "read verification block 1 fail\n");
		goto CLEANUP;
	}

        /* TODO: load vnc.raw from NV storage */
        params.nv_context = &vnc;

	params.boot_flags = boot_flags;
	params.shared_data_blob = shared_data_blob ? shared_data_blob :
		(uint8_t*) CONFIG_VB_SHARED_DATA_BLOB;
	params.shared_data_size = VB_SHARED_DATA_REC_SIZE;
	params.caller_internal = file;

	status = LoadFirmware(&params);

	if (vnc.raw_changed) {
		/* TODO: save vnc.raw to NV storage */
        }

	if (status == LOAD_FIRMWARE_SUCCESS) {
		debug(PREFIX "will jump to rewritable firmware %lld\n",
				params.firmware_index);
		*firmware_data_ptr = file->firmware_body[params.firmware_index];

		/* set to null so that *firmware_data_ptr is not disposed */
		file->firmware_body[params.firmware_index] = NULL;
	}

CLEANUP:
	GetFirmwareBody_dispose(file);

	free(params.gbb_data);
	free(params.verification_block_0);
	free(params.verification_block_1);

	return status;
}

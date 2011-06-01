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
#include <chromeos/common.h>
#include <chromeos/get_firmware_body.h>
#include <chromeos/kernel_shared_data.h>
#include <chromeos/load_firmware_helper.h>

#include <gbb_header.h>
#include <load_firmware_fw.h>
#include <vboot_nvstorage.h>
#include <vboot_struct.h>

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

	gbb_data = malloc(CONFIG_LENGTH_GBB);
	gbb_size = CONFIG_LENGTH_GBB;
	if (!gbb_data) {
		VBDEBUG(EMSG_MALLOC, CONFIG_LENGTH_GBB);
		goto EXIT;
	}

	if (firmware_storage_read(file, CONFIG_OFFSET_GBB, gbb_size, gbb_data))
		VBDEBUG(EMSG_READ_GBB);
	else
		retcode = 0;

EXIT:
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
		const off_t vblock_offset,
		void **vblock_ptr, uint64_t *vblock_size_ptr)
{
	VbKeyBlockHeader kbh;
	VbFirmwarePreambleHeader fph;
	uint64_t key_block_size, vblock_size;
	void *vblock;

	/* read key block header */
	if (firmware_storage_read(file, vblock_offset, sizeof(kbh), &kbh)) {
		VBDEBUG(PREFIX "read key block fail\n");
		return -1;
	}
	key_block_size = kbh.key_block_size;

	/* read firmware preamble header */
	if (firmware_storage_read(file, vblock_offset + key_block_size,
				sizeof(fph), &fph)) {
		VBDEBUG(PREFIX "read preamble fail\n");
		return -1;
	}
	vblock_size = key_block_size + fph.preamble_size;

	vblock = malloc(vblock_size);
	if (!vblock) {
		VBDEBUG(PREFIX "malloc verification block fail\n");
		return -1;
	}

	if (firmware_storage_read(file, vblock_offset, vblock_size, vblock)) {
		VBDEBUG(PREFIX "read verification block fail\n");
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
		const uint64_t boot_flags,
		VbNvContext *nvcxt,
		void *shared_data_blob,
		uint8_t **firmware_data_ptr)
{
	int status = LOAD_FIRMWARE_RECOVERY;
	LoadFirmwareParams params;
	get_firmware_body_internal_t gfbi;

	memset(&params, '\0', sizeof(params));

	get_firmware_body_internal_setup(&gfbi,
			file, CONFIG_OFFSET_FW_MAIN_A, CONFIG_OFFSET_FW_MAIN_B);

	if (load_gbb(file, &params.gbb_data, &params.gbb_size)) {
		VBDEBUG(PREFIX "error: read gbb fail\n");
		goto EXIT;
	}

	if (read_verification_block(file, CONFIG_OFFSET_VBLOCK_A,
				&params.verification_block_0,
				&params.verification_size_0)) {
		VBDEBUG(PREFIX "error: read verification block 0 fail\n");
		goto EXIT;
	}

	if (read_verification_block(file, CONFIG_OFFSET_VBLOCK_B,
				&params.verification_block_1,
				&params.verification_size_1)) {
		VBDEBUG(PREFIX "read verification block 1 fail\n");
		goto EXIT;
	}

	params.nv_context = nvcxt;

	params.boot_flags = boot_flags;
	params.shared_data_blob = shared_data_blob ? shared_data_blob :
		(uint8_t*) CONFIG_VB_SHARED_DATA_BLOB;
	params.shared_data_size = VB_SHARED_DATA_REC_SIZE;
	params.caller_internal = &gfbi;

	status = LoadFirmware(&params);

	if (status == LOAD_FIRMWARE_SUCCESS) {
		/* Fill in the firmware ID based on the index. */
		KernelSharedDataType *sd = get_kernel_shared_data();
		if (params.firmware_index == FIRMWARE_A) {
			if (firmware_storage_read(file,
					(off_t)CONFIG_OFFSET_RW_FWID_A,
					(size_t)CONFIG_LENGTH_RW_FWID_A,
					sd->fwid)) {
				VBDEBUG(PREFIX "error: read fwid fail\n");
				goto EXIT;
			}
		} else if (params.firmware_index == FIRMWARE_B) {
			if (firmware_storage_read(file,
					(off_t)CONFIG_OFFSET_RW_FWID_B,
					(size_t)CONFIG_LENGTH_RW_FWID_B,
					sd->fwid)) {
				VBDEBUG(PREFIX "error: read fwid fail\n");
				goto EXIT;
			}
		} else {
			VBDEBUG(PREFIX "wrong firmware_index %lld\n",
					params.firmware_index);
			goto EXIT;
		}

		VBDEBUG(PREFIX "will jump to rewritable firmware %lld\n",
				params.firmware_index);
		*firmware_data_ptr = gfbi.firmware_body[params.firmware_index];

		/* set to null so that *firmware_data_ptr is not disposed */
		gfbi.firmware_body[params.firmware_index] = NULL;
	}

EXIT:
	get_firmware_body_internal_dispose(&gfbi);

	free(params.gbb_data);
	free(params.verification_block_0);
	free(params.verification_block_1);

	return status;
}

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

#include <load_firmware_fw.h>
#include <vboot_struct.h>

/* Amount of bytes we read each time we call read() */
#define BLOCK_SIZE 512
#define PREFIX "GetFirmwareBody: "

int get_firmware_body_internal_setup(get_firmware_body_internal_t *gfbi,
		firmware_storage_t *file,
		const off_t firmware_data_offset_0,
		const off_t firmware_data_offset_1)
{
	gfbi->file = file;
	gfbi->firmware_data_offset[0] = firmware_data_offset_0;
	gfbi->firmware_data_offset[1] = firmware_data_offset_1;
	gfbi->firmware_body[0] = NULL;
	gfbi->firmware_body[1] = NULL;
	return 0;
}

int get_firmware_body_internal_dispose(get_firmware_body_internal_t *gfbi)
{
	int i;
	for (i = 0; i < 2; i ++)
		free(gfbi->firmware_body[i]);
	return 0;
}

/*
 * See vboot_reference/firmware/include/load_firmware_fw.h for documentation of
 * this function.
 */
int GetFirmwareBody(LoadFirmwareParams *params, uint64_t index)
{
	get_firmware_body_internal_t *gfbi;
	firmware_storage_t *file;
	void *block;
	VbKeyBlockHeader *kbh;
	VbFirmwarePreambleHeader *fph;
	off_t data_offset;
	int64_t leftover;
	ssize_t n;
	uint8_t *firmware_body;

	VBDEBUG(PREFIX "firmware index: %d\n", index);

	/* index = 0: firmware A; 1: firmware B; anything else: invalid */
	if (index != 0 && index != 1) {
		VBDEBUG(PREFIX "incorrect index: %d\n", index);
		return 1;
	}

	gfbi = (get_firmware_body_internal_t*) params->caller_internal;
	file = gfbi->file;

	if (index == 0) {
		block = params->verification_block_0;
	} else {
		block = params->verification_block_1;
	}
	data_offset = gfbi->firmware_data_offset[index];
	gfbi->firmware_body[index] = malloc(MAX(CONFIG_LENGTH_FW_MAIN_A,
				CONFIG_LENGTH_FW_MAIN_B));

	kbh = (VbKeyBlockHeader *) block;
	fph = (VbFirmwarePreambleHeader *) (block + kbh->key_block_size);

	VBDEBUG(PREFIX "key block address: %p\n", kbh);
	VBDEBUG(PREFIX "preamble address: %p\n", fph);
	VBDEBUG(PREFIX "firmware body offset: %08lx\n", data_offset);

	if (file->seek(file->context, data_offset, SEEK_SET) < 0) {
		VBDEBUG(PREFIX "seek to firmware data failed\n");
		return 1;
	}

	VBDEBUG(PREFIX "body size: %08llx\n", fph->body_signature.data_size);

	/*
	 * This loop feeds firmware body into UpdateFirmwareBodyHash.
	 * Variable <leftover> book-keeps the remaining number of bytes
	 */
	firmware_body = gfbi->firmware_body[index];
	for (leftover = fph->body_signature.data_size;
			leftover > 0; leftover -= n) {
		n = BLOCK_SIZE < leftover ? BLOCK_SIZE : leftover;
		n = file->read(file->context, firmware_body, n);
		if (n <= 0) {
			VBDEBUG(PREFIX "an error has occured "
					"while reading firmware: %d\n", n);
			return 1;
		}

		/*
		 * See vboot_reference/firmware/include/load_firmware_fw.h for
		 * documentation of this function.
		 */
		UpdateFirmwareBodyHash(params, firmware_body, n);
		firmware_body += n;
	}

	return 0;
}

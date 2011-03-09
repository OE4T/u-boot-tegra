/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <vboot_struct.h>
#include <chromeos/firmware_storage.h>

#include <load_firmware_fw.h>

/* Amount of bytes we read each time we call read() */
#define BLOCK_SIZE 512
#define PREFIX "GetFirmwareBody: "

void GetFirmwareBody_setup(firmware_storage_t *f,
		off_t firmware_data_offset_0, off_t firmware_data_offset_1)
{
	f->firmware_data_offset[0] = firmware_data_offset_0;
	f->firmware_data_offset[1] = firmware_data_offset_1;
	f->firmware_body[0] = NULL;
	f->firmware_body[1] = NULL;
}

void GetFirmwareBody_dispose(firmware_storage_t *f)
{
	int i;
	for (i = 0; i < 2; i ++)
		if (f->firmware_body[i])
			free(f->firmware_body[i]);
}

/*
 * See vboot_reference/firmware/include/load_firmware_fw.h for documentation of
 * this function.
 */
int GetFirmwareBody(LoadFirmwareParams *params, uint64_t index)
{
	firmware_storage_t *f;
	void *block;
	VbKeyBlockHeader *kbh;
	VbFirmwarePreambleHeader *fph;
	off_t data_offset;
	int64_t leftover;
	ssize_t n;
	uint8_t *firmware_body;

	debug(PREFIX "firmware index: %d\n", index);

	/* index = 0: firmware A; 1: firmware B; anything else: invalid */
	if (index != 0 && index != 1) {
		debug(PREFIX "incorrect index: %d\n", index);
		return 1;
	}

	f = (firmware_storage_t *) params->caller_internal;

	if (index == 0) {
		block = params->verification_block_0;
	} else {
		block = params->verification_block_1;
	}
	data_offset = f->firmware_data_offset[index];
	f->firmware_body[index] = malloc(MAX(CONFIG_LENGTH_FW_A_DATA,
				CONFIG_LENGTH_FW_B_DATA));

	kbh = (VbKeyBlockHeader *) block;
	fph = (VbFirmwarePreambleHeader *) (block + kbh->key_block_size);

	debug(PREFIX "key block address: %p\n", kbh);
	debug(PREFIX "preamble address: %p\n", fph);
	debug(PREFIX "firmware body offset: %08lx\n", data_offset);

	if (f->seek(f->context, data_offset, SEEK_SET) < 0) {
		debug(PREFIX "seek to firmware data failed\n");
		return 1;
	}

	debug(PREFIX "body size: %08llx\n", fph->body_signature.data_size);

	/*
	 * This loop feeds firmware body into UpdateFirmwareBodyHash.
	 * Variable <leftover> book-keeps the remaining number of bytes
	 */
	firmware_body = f->firmware_body[index];
	for (leftover = fph->body_signature.data_size;
			leftover > 0; leftover -= n) {
		n = BLOCK_SIZE < leftover ? BLOCK_SIZE : leftover;
		n = f->read(f->context, firmware_body, n);
		if (n <= 0) {
			debug(PREFIX "an error has occured "
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

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

#define BUF_SIZE 256
#define PREFIX "GetFirmwareBody: "

int GetFirmwareBody(LoadFirmwareParams *params, uint64_t index)
{
	caller_internal_t *ci;
	void *block;
	VbKeyBlockHeader *kbh;
	VbFirmwarePreambleHeader *fph;
	off_t data_offset;
	int64_t leftover;
	ssize_t n;
	uint8_t buf[BUF_SIZE];

	debug(PREFIX "firmware index: %d\n", index);

	/* index = 0: firmware A; 1: firmware B; anything else: invalid */
	if (index != 0 && index != 1) {
		debug(PREFIX "incorrect index: %d\n", index);
		return 1;
	}

	ci = (caller_internal_t *) params->caller_internal;

	if (index == 0) {
		block = params->verification_block_0;
		data_offset = CONFIG_OFFSET_FW_A_DATA;
	} else {
		block = params->verification_block_1;
		data_offset = CONFIG_OFFSET_FW_B_DATA;
	}

	kbh = (VbKeyBlockHeader *) block;
	fph = (VbFirmwarePreambleHeader *) (block + kbh->key_block_size);

	debug(PREFIX "key block address: %p\n", kbh);
	debug(PREFIX "preamble address: %p\n", fph);
	debug(PREFIX "firmware body offset: %08lx\n", data_offset);

	if (ci->seek(ci->context, data_offset, SEEK_SET) < 0) {
		debug(PREFIX "seek to firmware data failed\n");
		return 1;
	}

	debug(PREFIX "body size: %08llx\n", fph->body_signature.data_size);

	for (leftover = fph->body_signature.data_size;
			leftover > 0; leftover -= n) {
		n = sizeof(buf) < leftover ? sizeof(buf) : leftover;
		n = ci->read(ci->context, buf, n);
		if (n <= 0) {
			debug(PREFIX "an error has occured "
					"while reading firmware: %d\n", n);
			return 1;
		}

		UpdateFirmwareBodyHash(params, buf, n);
	}

	return 0;
}

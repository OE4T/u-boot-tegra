/*
 * Copyright 2011, Google Inc.
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

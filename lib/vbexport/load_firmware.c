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
#include <vboot/firmware_cache.h>
#include <vboot/firmware_storage.h>

/* Import the header files from vboot_reference. */
#include <vboot_api.h>

#define PREFIX			"load_firmware: "

/* Amount of bytes we read each time we call read() */
#define BLOCK_SIZE		512

VbError_t VbExHashFirmwareBody(VbCommonParams* cparams,
			       uint32_t firmware_index)
{
	firmware_cache_t *cache = (firmware_cache_t *)cparams->caller_context;
	firmware_storage_t *file = cache->file;
	firmware_info_t *info;
	int index;
	uint8_t *buffer;
	size_t leftover, n, got_n;

	if (firmware_index != VB_SELECT_FIRMWARE_A &&
			firmware_index != VB_SELECT_FIRMWARE_B) {
		VbExDebug(PREFIX "Incorrect firmware index: %lu\n",
				firmware_index);
		return 1;
	}

	index = (firmware_index == VB_SELECT_FIRMWARE_A ? 0 : 1);
	info = &cache->infos[index];
	buffer = info->buffer;

	if (file->seek(file->context, info->offset, SEEK_SET)
			< 0) {
		VbExDebug(PREFIX "seek to firmware data failed\n");
		return 1;
	}

	/*
	 * This loop feeds firmware body into VbUpdateFirmwareBodyHash.
	 * Variable leftover keeps the remaining number of bytes.
	 */
	for (leftover = info->size; leftover > 0; leftover -= n, buffer += n) {
		n = min(BLOCK_SIZE, leftover);
		got_n = file->read(file->context, buffer, n);
		if (got_n != n) {
			VbExDebug(PREFIX "an error has occured "
					"while reading firmware: %d\n", n);
			return 1;
		}

		/*
		 * See vboot_reference/firmware/include/vboot_api.h for
		 * documentation of this function.
		 */
		VbUpdateFirmwareBodyHash(cparams, buffer, n);
	}

	return 0;
}

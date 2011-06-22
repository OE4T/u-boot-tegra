/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/*
 * Firmware cache is the internal context for firmware/VbExHashFirmwareBody().
 * It keeps the RW firmware A/B data which is loaded in VbExHashFirmwareBody().
 * After VbSelectFirmware() returns, we can directly jump to the loaded
 * firmware without another load from flash.
 */

#ifndef VBOOT_FIRMWARE_CACHE_H_
#define VBOOT_FIRMWARE_CACHE_H_

#include <linux/types.h>
#include <vboot/firmware_storage.h>

typedef struct {
	/*
	 * These fields are the offsets of firmware data sections,
	 * from beginning of firmware storage device.
	 */
	off_t offset;

	/*
	 * These fields are the sizes of firmware data sections.
	 */
	size_t size;

	/*
	 * These pointers will point to firmware data bodies loaded by
	 * VbExHashFirmwareBody().
	 */
	uint8_t *buffer;
} firmware_info_t;

typedef struct {
	firmware_storage_t *file;
	firmware_info_t infos[2];
} firmware_cache_t;

/* Initialize fields of firmware cache. */
void init_firmware_cache(firmware_cache_t *cache, firmware_storage_t *file,
		off_t firmware_a_offset, size_t firmware_a_size,
		off_t firmware_b_offset, size_t firmware_b_size);

/* Release fields of firmware cache. */
void free_firmware_cache(firmware_cache_t *cache);

#endif /* VBOOT_FIRMWARE_CACHE_H */

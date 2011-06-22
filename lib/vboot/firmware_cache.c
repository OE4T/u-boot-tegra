/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <vboot/firmware_cache.h>

/* Import the header files from vboot_reference. */
#include <vboot_api.h>

void init_firmware_cache(firmware_cache_t *cache, firmware_storage_t *file,
                off_t firmware_a_offset, size_t firmware_a_size,
                off_t firmware_b_offset, size_t firmware_b_size)
{
	cache->file = file;
	cache->infos[0].offset = firmware_a_offset;
	cache->infos[1].offset = firmware_b_offset;
	cache->infos[0].size = firmware_a_size;
	cache->infos[1].size = firmware_b_size;
	cache->infos[0].buffer = VbExMalloc(firmware_a_size);
	cache->infos[1].buffer = VbExMalloc(firmware_b_size);
}

void free_firmware_cache(firmware_cache_t *cache)
{
	VbExFree(cache->infos[0].buffer);
	VbExFree(cache->infos[1].buffer);
}

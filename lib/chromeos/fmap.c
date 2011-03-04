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
#include <chromeos/fmap.h>

off_t fmap_find(const uint8_t *image, size_t image_len)
{
	off_t offset;
	uint64_t sig;

	memcpy(&sig, FMAP_SIGNATURE, strlen(FMAP_SIGNATURE));

	/* Find 4-byte aligned FMAP signature within image */
	for (offset = 0; offset < image_len; offset += 4) {
		if (!memcmp(&image[offset], &sig, sizeof(sig))) {
			break;
		}
	}

	if (offset < image_len)
		return offset;
	else
		return -1;
}

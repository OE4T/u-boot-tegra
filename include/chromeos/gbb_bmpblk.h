/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __CHROMEOS_GBB_BMPBLK_H__
#define __CHROMEOS_GBB_BMPBLK_H__

enum {
	BMPBLK_OK = 0,
	BMPBLK_UNSUPPORTED_COMPRESSION,
	BMPBLK_LZMA_DECOMPRESS_FAILED,
	BMPBLK_BMP_DISPLAY_FAILED,
};

/* Print the screen info in BMP block. */
int print_screen_info_in_bmpblk(uint8_t *gbb_start, int index);

/* Display the screen on LCD. */
int display_screen_in_bmpblk(uint8_t *gbb_start, int index);

#endif /* __CHROMEOS_GBB_BMPBLK_H__ */

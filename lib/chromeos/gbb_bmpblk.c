/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Library for Chrome OS GBB BMP block. */

#include <common.h>
#include <lcd.h>
#include <malloc.h>
#include <chromeos/common.h>
#include <chromeos/gbb_bmpblk.h>
#include <lzma/LzmaTypes.h>
#include <lzma/LzmaDec.h>
#include <lzma/LzmaTools.h>

/* headers of vboot_reference */
#include <bmpblk_header.h>
#include <gbb_header.h>

/* defined in common/lcd.c */
extern int lcd_display_bitmap (ulong, int, int);

static uint8_t *uncompress_lzma(uint8_t *in_addr, SizeT in_size,
				SizeT out_size)
{
        uint8_t *out_addr = malloc(out_size);
        SizeT lzma_len = out_size;
        int ret;

        ret = lzmaBuffToBuffDecompress(out_addr, &lzma_len, in_addr, in_size);
        if (ret != SZ_OK) {
		free(out_addr);
		out_addr = NULL;
        }
        return out_addr;
}

int print_screen_info_in_bmpblk(uint8_t *gbb_start, int index)
{
	GoogleBinaryBlockHeader *gbbh;
	BmpBlockHeader *bmph;
	ScreenLayout *screen;
	ImageInfo *info;
	int i;

	gbbh = (GoogleBinaryBlockHeader *)gbb_start;
	bmph = (BmpBlockHeader *)(gbb_start + gbbh->bmpfv_offset);
	screen = (ScreenLayout *)(bmph + 1);

	assert(index <
	       bmph->number_of_localizations * bmph->number_of_screenlayouts);
	screen += index;

	printf("screens[%d] info:\n", index);
	for (i = 0;
	     i < MAX_IMAGE_IN_LAYOUT && screen->images[i].image_info_offset;
	     ++i) {
		info = (ImageInfo *)((uint8_t *)bmph +
				     screen->images[i].image_info_offset);
		printf("\t- %s (%d x %d) showed on (%d, %d)\n",
			(info->format==FORMAT_BMP)?"BMP":"Unknown Image",
			info->width,
			info->height,
			screen->images[i].x,
			screen->images[i].y);
	}
	return BMPBLK_OK;
}

int display_screen_in_bmpblk(uint8_t *gbb_start, int index)
{
	GoogleBinaryBlockHeader *gbbh;
	BmpBlockHeader *bmph;
	ScreenLayout *screen;
	ImageInfo *info;
	uint8_t *data;
	int i;
	int ret;

	gbbh = (GoogleBinaryBlockHeader *)gbb_start;
	bmph = (BmpBlockHeader *)(gbb_start + gbbh->bmpfv_offset);
	screen = (ScreenLayout *)(bmph + 1);

	assert(index <
	       bmph->number_of_localizations * bmph->number_of_screenlayouts);
	screen += index;

	for (i = 0;
	     i < MAX_IMAGE_IN_LAYOUT && screen->images[i].image_info_offset;
	     ++i) {
		info = (ImageInfo *)((uint8_t *)bmph +
				     screen->images[i].image_info_offset);

		if (info->compression != COMPRESS_NONE &&
				info->compression != COMPRESS_LZMA1)
			return BMPBLK_UNSUPPORTED_COMPRESSION;

		data = (uint8_t *)(info + 1);
		if (info->compression == COMPRESS_LZMA1) {
			data = uncompress_lzma(data,
				(SizeT)info->compressed_size,
				(SizeT)info->original_size);
		}
		if (!data)
			return BMPBLK_LZMA_DECOMPRESS_FAILED;

		ret = lcd_display_bitmap((ulong)data,
				screen->images[i].x,
				screen->images[i].y);

		if (info->compression == COMPRESS_LZMA1)
			free(data);

		if (ret)
			return BMPBLK_BMP_DISPLAY_FAILED;
	}
	return BMPBLK_OK;
}

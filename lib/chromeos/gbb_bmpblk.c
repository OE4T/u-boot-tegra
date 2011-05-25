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

#include <bmpblk_header.h>
#include <gbb_header.h>

#ifdef CONFIG_LZMA
#include <lzma/LzmaTypes.h>
#include <lzma/LzmaDec.h>
#include <lzma/LzmaTools.h>
#endif /* CONFIG_LZMA */

#ifdef CONFIG_CMD_BMP
extern int lcd_display_bitmap (ulong, int, int);
#endif /* CONFIG_CMD_BMP */

#ifdef CONFIG_LZMA
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
#endif /* CONFIG_LZMA */

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
	int needed_free;
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
		data = NULL;
		needed_free = 0;
		if (info->compression == COMPRESS_NONE) {
			data = (uint8_t *)(info + 1);
		}
#ifdef CONFIG_LZMA
		if (info->compression == COMPRESS_LZMA1) {
			data = uncompress_lzma(
				(uint8_t *)(info + 1),
				(SizeT)info->compressed_size,
				(SizeT)info->original_size);
			if (data) needed_free = 1;
			else return BMPBLK_LZMA_DECOMPRESS_FAILED;
		}
#endif /* CONFIG_LZMA */
		if (data) {
			ret = -1;
#ifdef CONFIG_CMD_BMP
			ret = lcd_display_bitmap((ulong)data,
				screen->images[i].x,
				screen->images[i].y);
#endif /* CONFIG_CMD_BMP */
			if (ret) {
				if (needed_free) free(data);
				return BMPBLK_BMP_DISPLAY_FAILED;
			}
		} else {
			return BMPBLK_UNSUPPORTED_COMPRESSION;
		}
		if (needed_free) free(data);
	}
	return BMPBLK_OK;
}

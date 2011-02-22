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

/* Library for Chrome OS GBB BMP block. */

#include <common.h>
#include <lcd.h>
#include <malloc.h>
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

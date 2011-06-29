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
#include <fdt_decode.h>
#include <lcd.h>
#include <lzma/LzmaTypes.h>
#include <lzma/LzmaDec.h>
#include <lzma/LzmaTools.h>

/* Import the header files from vboot_reference */
#include <vboot_api.h>

#define PRINT_MAX_ROW	20
#define PRINT_MAX_COL	80

DECLARE_GLOBAL_DATA_PTR;

/* Defined in common/lcd.c */
extern int lcd_display_bitmap (ulong, int, int);

VbError_t VbExDisplayInit(uint32_t *width, uint32_t *height)
{
	struct fdt_lcd config;

	/* Get LCD details from FDT */
	if (fdt_decode_lcd(gd->blob, &config)) {
		VbExDebug("No LCD information in device tree.\n");
		return 1;
	}

	*width = config.width;
	*height = config.height;

	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayBacklight(uint8_t enable)
{
	/* TODO(waihong@chromium.org) Implement it later */
	return VBERROR_SUCCESS;
}

/* Print the message on the center of LCD. */
static void print_on_center(const char *message)
{
	char result[PRINT_MAX_ROW * (PRINT_MAX_COL + 1) + 1] = "\0";
	int i, j, left_space, right_space;

	for (i = 0; i < PRINT_MAX_ROW / 2 - 1; i++) {
		for (j = 0; j < PRINT_MAX_COL; j++)
			strcat(result, ".");
		strcat(result, "\n");
	}
	
	left_space = (PRINT_MAX_COL - strlen(message)) / 2;
	for (i = 0; i < left_space; i++)
		strcat(result, ".");
	strcat(result, message);

	right_space = PRINT_MAX_COL - strlen(message) - left_space;
	for (i = 0; i < right_space; i++)
		strcat(result, ".");
	strcat(result, "\n");

	for (i = 0; i < PRINT_MAX_ROW - PRINT_MAX_ROW / 2; i++) {
		for (j = 0; j < PRINT_MAX_COL; j++)
			strcat(result, ".");
		strcat(result, "\n");
	}

	VbExDisplayDebugInfo(result);
}

VbError_t VbExDisplayScreen(uint32_t screen_type)
{
	/*
	 * Show the debug messages for development. It is a backup method
	 * when GBB does not contain a full set of bitmaps.
	 */
	switch (screen_type) {
		case VB_SCREEN_BLANK:
			print_on_center("");
			break;
		case VB_SCREEN_DEVELOPER_WARNING:
			print_on_center("developer mode warning");
			break;
		case VB_SCREEN_DEVELOPER_EGG:
			print_on_center("easter egg");
			break;
		case VB_SCREEN_RECOVERY_REMOVE:
			print_on_center("remove inserted devices");
			break;
		case VB_SCREEN_RECOVERY_INSERT:
			print_on_center("insert recovery image");
			break;
		case VB_SCREEN_RECOVERY_NO_GOOD:
			print_on_center("insert image invalid");
			break;
		default:
			VbExDebug("Not a valid screen type: 0x%lx.\n",
					screen_type);
			return 1;
	}
	return VBERROR_SUCCESS;
}

static uint8_t *uncompress_lzma(uint8_t *in_addr, SizeT in_size,
                                SizeT out_size)
{
	uint8_t *out_addr = VbExMalloc(out_size);
	SizeT lzma_len = out_size;
	int ret;

	ret = lzmaBuffToBuffDecompress(out_addr, &lzma_len, in_addr, in_size);
	if (ret != SZ_OK) {
		VbExFree(out_addr);
		out_addr = NULL;
	}
	return out_addr;
}

VbError_t VbExDisplayImage(uint32_t x, uint32_t y, const ImageInfo *info,
                           const void *buffer)
{
	int ret;
	uint8_t *raw_data;

	switch (info->compression) {
	case COMPRESS_NONE:
		raw_data = (uint8_t *)buffer;
		break;

	case COMPRESS_LZMA1:
		raw_data = uncompress_lzma((uint8_t *)buffer,
				(SizeT)info->compressed_size,
				(SizeT)info->original_size);
		if (!raw_data) {
			VbExDebug("LZMA decompress failed.\n");
			return 1;
		}
		break;

	default:
		VbExDebug("Unsupported compression format: %lu\n",
				info->compression);
		return 1;
	}

	ret = lcd_display_bitmap((ulong)raw_data, x, y);

	if (info->compression == COMPRESS_LZMA1)
		VbExFree(raw_data);

	if (ret) {
		VbExDebug("LCD display error.\n");
		return 1;
	}

	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayDebugInfo(const char *info_str)
{
	/* Show the debug message on the upper left corner */
	console_col = 0;
	console_row = 0;
	lcd_puts(info_str);

	return VBERROR_SUCCESS;
}

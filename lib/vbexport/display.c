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

/* Import the header files from vboot_reference */
#include <vboot_api.h>

DECLARE_GLOBAL_DATA_PTR;

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

static void print_msg(const char *name, const char *message)
{
	int i, left_space, right_space;
	printf("Showing a %s screen\n", name);
	printf("+------------------------------+\n");

	printf("|");
	left_space = (30 - strlen(message)) / 2;
	for (i = 0; i < left_space; i++)
		printf("-");

	printf(message);

	right_space = 30 - strlen(message) - left_space;
	for (i = 0; i < right_space; i++)
		printf("-");
	printf("|\n");

	printf("+------------------------------+\n");
}

VbError_t VbExDisplayScreen(uint32_t screen_type)
{
	/*
	 * Show the debug messages for development. It is a backup method
	 * when GBB does not contain a full set of bitmaps.
	 */
	switch (screen_type) {
		case VB_SCREEN_BLANK:
			print_msg("BLANK", "");
			break;
		case VB_SCREEN_DEVELOPER_WARNING:
			print_msg("DEVELOPER_WARNING", "warning");
			break;
		case VB_SCREEN_DEVELOPER_EGG:
			print_msg("DEVELOPER_EGG", "easter egg");
			break;
		case VB_SCREEN_RECOVERY_REMOVE:
			print_msg("RECOVERY_REMOVE", "remove inserted devices");
			break;
		case VB_SCREEN_RECOVERY_INSERT:
			print_msg("RECOVERY_INSERT", "insert recovery image");
			break;
		case VB_SCREEN_RECOVERY_NO_GOOD:
			print_msg("RECOVERY_NO_GOOD", "insert image invalid");
			break;
		default:
			VbExDebug("Not a valid screen type: 0x%lx.\n",
					screen_type);
			return 1;
	}
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayImage(uint32_t x, uint32_t y, const ImageInfo *info,
                           const void *buffer)
{
	/* TODO(waihong@chromium.org) Implement it later. */
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayDebugInfo(const char *info_str)
{
	/* TODO(waihong@chromium.org) Implement it later. */
	return VBERROR_SUCCESS;
}

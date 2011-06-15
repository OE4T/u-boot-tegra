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
#include <malloc.h>
#include <part.h>
#include <chromeos/common.h>
#include <chromeos/vboot_nvstorage_helper.h>
#include <linux/string.h>

#define PREFIX "vboot_nvstorage_helper: "

/**
 * This reads the VbNvContext from internal (mmc) storage device. If it
 * succeeds, caller must free buf_ptr. If it fails, both dev_desc_ptr and
 * buf_ptr will be set to NULL.
 *
 * @param dev_desc_ptr stores the pointer to the device descriptor
 * @param buf_ptr stores the pointer to a buffer holding loaded VbNvContext data
 * @return 0 if it succeeds; non-zero if it fails
 */
static int load_nvcxt(block_dev_desc_t **dev_desc_ptr, uint8_t **buf_ptr)
{
	block_dev_desc_t *dev_desc = NULL;
	uint8_t *buf = NULL;

	*dev_desc_ptr = NULL;
	*buf_ptr = NULL;

	dev_desc = get_dev("mmc", MMC_INTERNAL_DEVICE);
	if (dev_desc == NULL) {
		VBDEBUG(PREFIX "get_dev(mmc, MMC_INTERNAL_DEVICE) fail\n");
		return -1;
	}

	buf = memalign(CACHE_LINE_SIZE, 512);
	if (!buf) {
		VBDEBUG(PREFIX "memalign(%d, 512) == NULL\n", CACHE_LINE_SIZE);
		return -1;
	}

	if (dev_desc->block_read(dev_desc->dev, NVCONTEXT_LBA, 1, buf) < 0) {
		VBDEBUG(PREFIX "block_read fail\n");
		free(buf);
		return -1;
	}

	*dev_desc_ptr = dev_desc;
	*buf_ptr = buf;
	return 0;
}

int read_nvcontext(VbNvContext *nvcxt)
{
	block_dev_desc_t *dev_desc;
	uint8_t *buf;

	if (load_nvcxt(&dev_desc, &buf))
		return -1;

	memcpy(nvcxt->raw, buf, VBNV_BLOCK_SIZE);

	free(buf);
	return 0;
}

int write_nvcontext(VbNvContext *nvcxt)
{
	block_dev_desc_t *dev_desc;
	uint8_t *buf;
	int ret = 0;

	if (load_nvcxt(&dev_desc, &buf))
		return -1;

	memcpy(buf, nvcxt->raw, VBNV_BLOCK_SIZE);

	ret = dev_desc->block_write(dev_desc->dev, NVCONTEXT_LBA, 1, buf);
	free(buf);

	if (ret < 0)
		VBDEBUG(PREFIX "block_write fail\n");

	return ret;
}

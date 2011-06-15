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
#include <chromeos/common.h>
#include <chromeos/power_management.h>
#include <chromeos/vboot_nvstorage_helper.h>

/* TODO: temporary hack for factory bring up; remove/rewrite when necessary */
#include <malloc.h>
#include <part.h>
#include <chromeos/os_storage.h> /* for initialize_mmc_device */
#include <linux/string.h>

/*
 * TODO: So far (2011/04/12) kernel does not have a SPI flash driver. That
 * means kernel cannot access cookies in SPI flash. (In addition, there is
 * an on-going discussion about the storage device for these cookies. So the
 * final storage of the cookies might even not be SPI flash after a couple of
 * months.) Here is a temporary workaround for factory bring up that stores
 * the cookies in mmc device where we are certain that kernel can access.
 */

#define PREFIX "vboot_nvstorage_helper: "

/* FIXME: remove this if we do not store nvcontext in mmc device */
uint64_t get_nvcxt_lba(void)
{
	/* we store nvcontext in mbr */
	return 0;
}

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
	const uint64_t nvcxt_lba = get_nvcxt_lba();
	block_dev_desc_t *dev_desc = NULL;
	uint8_t *buf = NULL;

	*dev_desc_ptr = NULL;
	*buf_ptr = NULL;

	if (initialize_mmc_device(MMC_INTERNAL_DEVICE)) {
		VBDEBUG(PREFIX "init MMC_INTERNAL_DEVICE fail\n");
		return -1;
	}

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

	if (dev_desc->block_read(dev_desc->dev, nvcxt_lba, 1, buf) < 0) {
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
	const uint64_t nvcxt_lba = get_nvcxt_lba();
	block_dev_desc_t *dev_desc;
	uint8_t *buf;
	int ret = 0;

	if (load_nvcxt(&dev_desc, &buf))
		return -1;

	memcpy(buf, nvcxt->raw, VBNV_BLOCK_SIZE);

	ret = dev_desc->block_write(dev_desc->dev, nvcxt_lba, 1, buf);
	free(buf);

	if (ret < 0)
		VBDEBUG(PREFIX "block_write fail\n");

	return ret;
}

int clear_recovery_request(void)
{
	VbNvContext nvcxt;

	if (read_nvcontext(&nvcxt) || VbNvSetup(&nvcxt)) {
		VBDEBUG(PREFIX "cannot read nvcxt\n");
		return 1;
	}

	if (VbNvSet(&nvcxt, VBNV_RECOVERY_REQUEST,
				VBNV_RECOVERY_NOT_REQUESTED)) {
		VBDEBUG(PREFIX "cannot clear VBNV_RECOVERY_REQUEST\n");
		return 1;
	}

	if (VbNvTeardown(&nvcxt) ||
			(nvcxt.raw_changed && write_nvcontext(&nvcxt))) {
		VBDEBUG(PREFIX "cannot write nvcxt\n");
		return 1;
	}

	return 0;
}

void reboot_to_recovery_mode(uint32_t reason)
{
	VbNvContext *nvcxt, nvcontext;

	nvcxt = &nvcontext;
	if (read_nvcontext(nvcxt) || VbNvSetup(nvcxt)) {
		VBDEBUG(PREFIX "cannot read nvcxt\n");
		goto FAIL;
	}

	VBDEBUG(PREFIX "store recovery cookie in recovery field\n");
	if (VbNvSet(nvcxt, VBNV_RECOVERY_REQUEST, reason) ||
			VbNvTeardown(nvcxt) ||
			(nvcxt->raw_changed && write_nvcontext(nvcxt))) {
		VBDEBUG(PREFIX "cannot write back nvcxt");
		goto FAIL;
	}

	VBDEBUG(PREFIX "reboot to recovery mode\n");
	cold_reboot();

	VBDEBUG(PREFIX "error: cold_reboot() returned\n");
FAIL:
	/* FIXME: bring up a sad face? */
	printf("Please reset and press recovery button when reboot.\n");
	while (1);
}

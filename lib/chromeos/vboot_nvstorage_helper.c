/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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

#include <common.h>
#include <chromeos/common.h>
#include <chromeos/power_management.h>
#include <chromeos/vboot_nvstorage_helper.h>

/* TODO: temporary hack for factory bring up; remove/rewrite when necessary */
#include <mmc.h>
#include <part.h>
#include <asm/arch/nv_sdmmc.h>
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

static int set_mmc_device(int new_device_index, int *last_device_index_ptr)
{
	int cur_dev = mmc_get_current_dev_index();

	if (last_device_index_ptr)
		*last_device_index_ptr = cur_dev;

	if (cur_dev == new_device_index)
		return 0;

	return initialize_mmc_device(new_device_index);
}

static int prepare_access_nvcontext(block_dev_desc_t **dev_desc_ptr,
		uint64_t *nvcxt_lba_ptr)
{
	block_dev_desc_t *dev_desc = NULL;
	uint8_t buf[512];

	dev_desc = get_dev("mmc", 0);
	if (dev_desc == NULL) {
		VBDEBUG(PREFIX "get_dev(0) fail\n");
		return -1;
	}

	if (dev_desc->block_read(dev_desc->dev, 1, 1, buf) < 0) {
		VBDEBUG(PREFIX "read primary GPT table fail\n");
		return -1;
	}

	*dev_desc_ptr = dev_desc;
	*nvcxt_lba_ptr = 0; /* Store cookie in MBR */
	return 0;
}

static int access_nvcontext(VbNvContext *nvcxt, int is_read)
{
	int retval = -1;
	int last_dev;
	block_dev_desc_t *dev_desc;
	uint64_t nvcxt_lba;
	uint8_t buf[512];

	if (set_mmc_device(0, &last_dev)) {
		VBDEBUG(PREFIX "set_mmc_device(0) fail\n");
		return -1;
	}

	VBDEBUG(PREFIX "last_dev: %d\n", last_dev);

	if (prepare_access_nvcontext(&dev_desc, &nvcxt_lba)) {
		VBDEBUG(PREFIX "prepare_access_nvcontext fail\n");
		goto EXIT;
	}

	if (dev_desc->block_read(dev_desc->dev, nvcxt_lba, 1, buf) < 0) {
		VBDEBUG(PREFIX "block_read fail\n");
		goto EXIT;
	}

	if (is_read)
		memcpy(nvcxt->raw, buf, VBNV_BLOCK_SIZE);
	else {
		memcpy(buf, nvcxt->raw, VBNV_BLOCK_SIZE);
		if (dev_desc->block_write(dev_desc->dev,
					nvcxt_lba, 1, buf) < 0) {
			VBDEBUG(PREFIX "block_write fail\n");
			goto EXIT;
		}
	}

	retval = 0;
EXIT:
	/* restore previous device */
	if (last_dev != -1 && last_dev != 0)
		set_mmc_device(last_dev, NULL);

	return retval;
}

uint64_t get_nvcxt_lba(void)
{
	int last_dev;
	block_dev_desc_t *dev_desc;
	uint64_t nvcxt_lba = ~0ULL;

	if (set_mmc_device(0, &last_dev)) {
		VBDEBUG(PREFIX "set_mmc_device(0) fail\n");
		return ~0ULL;
	}

	if (prepare_access_nvcontext(&dev_desc, &nvcxt_lba))
		VBDEBUG(PREFIX "prepare_access_nvcontext fail\n");

	/* restore previous device */
	if (last_dev != -1 && last_dev != 0)
		set_mmc_device(last_dev, NULL);

	return nvcxt_lba;
}

int read_nvcontext(VbNvContext *nvcxt)
{
	return access_nvcontext(nvcxt, 1);
}

int write_nvcontext(VbNvContext *nvcxt)
{
	return access_nvcontext(nvcxt, 0);
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

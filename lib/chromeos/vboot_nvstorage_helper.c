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
#include <chromeos/vboot_nvstorage_helper.h>

/* TODO: temporary hack for factory bring up; remove/rewrite when necessary */
#include <mmc.h>
#include <part.h>
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

#define BACKUP_LBA_OFFSET 0x20

uint64_t get_nvcxt_lba(void)
{
	int cur_dev = get_mmc_current_device();
	block_dev_desc_t *dev_desc = NULL;
	uint64_t nvcxt_lba;
	uint8_t buf[512];

	/* use side effect of "mmc init 0" to set current device for us */
	if (cur_dev == -1)
		run_command("mmc init 0", 0);
	else if (cur_dev != 0)
		initialize_mmc_device(0);

	if ((dev_desc = get_dev("mmc", 0)) == NULL) {
		debug(PREFIX "get_dev fail\n");
		return ~0ULL;
	}

	if (dev_desc->block_read(dev_desc->dev, 1, 1, buf) < 0) {
		debug(PREFIX "read primary GPT table fail\n");
		return ~0ULL;
	}

	nvcxt_lba = 1 + *(uint64_t*) (buf + BACKUP_LBA_OFFSET);

	/* restore previous device */
	if (cur_dev != -1 && cur_dev != 0)
		initialize_mmc_device(cur_dev);

	return nvcxt_lba;
}

static int access_nvcontext(firmware_storage_t *file, VbNvContext *nvcxt,
		int is_read)
{
	int cur_dev = get_mmc_current_device();
	block_dev_desc_t *dev_desc = NULL;
	uint64_t nvcxt_lba;
	uint8_t buf[512];

	debug(PREFIX "cur_dev: %d\n", cur_dev);

	/* use side effect of "mmc init 0" to set current device for us */
	if (cur_dev == -1)
		run_command("mmc init 0", 0);
	else if (cur_dev != 0)
		initialize_mmc_device(0);

	if ((dev_desc = get_dev("mmc", 0)) == NULL) {
		debug(PREFIX "get_dev fail\n");
		return -1;
	}

	if (dev_desc->block_read(dev_desc->dev, 1, 1, buf) < 0) {
		debug(PREFIX "read primary GPT table fail\n");
		return -1;
	}

	nvcxt_lba = 1 + *(uint64_t*) (buf + BACKUP_LBA_OFFSET);

	if (is_read) {
		if (dev_desc->block_read(dev_desc->dev,
					nvcxt_lba, 1, buf) < 0) {
			debug(PREFIX "block_read fail\n");
			return -1;
		}
		memcpy(nvcxt->raw, buf, VBNV_BLOCK_SIZE);
	} else {
		memcpy(buf, nvcxt->raw, VBNV_BLOCK_SIZE);
		if (dev_desc->block_write(dev_desc->dev,
					nvcxt_lba, 1, buf) < 0) {
			debug(PREFIX "block_write fail\n");
			return -1;
		}
	}

	/* restore previous device */
	if (cur_dev != -1 && cur_dev != 0)
		initialize_mmc_device(cur_dev);

	return 0;
}

int read_nvcontext(firmware_storage_t *file, VbNvContext *nvcxt)
{
	return access_nvcontext(file, nvcxt, 1);
}

int write_nvcontext(firmware_storage_t *file, VbNvContext *nvcxt)
{
	return access_nvcontext(file, nvcxt, 0);
}

#if 0
/*
 * TODO It should averagely distributed erase/write operation to entire flash
 * memory section allocated for VBNVCONTEXT to increase maximal lifetime.
 *
 * But since VbNvContext gets written infrequently enough, this is likely
 * an overkill.
 */

int read_nvcontext(firmware_storage_t *file, VbNvContext *nvcxt)
{
	if (firmware_storage_read(file,
			CONFIG_OFFSET_VBNVCONTEXT, VBNV_BLOCK_SIZE,
			nvcxt->raw)) {
		debug(PREFIX "read_nvcontext fail\n");
		return -1;
	}

	if (VbNvSetup(nvcxt)) {
		debug(PREFIX "setup nvcontext fail\n");
		return -1;
	}

	return 0;
}

int write_nvcontext(firmware_storage_t *file, VbNvContext *nvcxt)
{
	if (firmware_storage_write(file,
			CONFIG_OFFSET_VBNVCONTEXT, VBNV_BLOCK_SIZE,
			nvcxt->raw)) {
		debug(PREFIX "write_nvcontext fail\n");
		return -1;
	}

	return 0;
}
#endif

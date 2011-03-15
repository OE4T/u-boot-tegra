/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <part.h>
#include <boot_device.h>

#define PREFIX "boot_device: "

#define BACKUP_LBA_OFFSET 0x20

static struct {
	block_dev_desc_t *dev_desc;
	ulong offset, limit;
} bootdev_config = {
	.dev_desc = NULL,
	.offset = 0u,
	.limit = 0u
};

block_dev_desc_t *get_bootdev(void)
{
	return bootdev_config.dev_desc;
}

ulong get_offset(void)
{
	return bootdev_config.offset;
}

ulong get_limit(void)
{
	return bootdev_config.limit;
}

uint64_t get_bytes_per_lba(void)
{
	if (!bootdev_config.dev_desc) {
		debug(PREFIX "get_bytes_per_lba: not dev_desc set\n");
		return ~0ULL;
	}

	return (uint64_t) bootdev_config.dev_desc->blksz;
}

uint64_t get_ending_lba(void)
{
	uint8_t static_buf[512];
	uint8_t *buf = NULL;
	uint64_t ret = ~0ULL, bytes_per_lba = ~0ULL;

	bytes_per_lba = get_bytes_per_lba();
	if (bytes_per_lba == ~0ULL) {
		debug(PREFIX "get_ending_lba: get bytes_per_lba fail\n");
		goto EXIT;
	}

	if (bytes_per_lba > sizeof(static_buf))
		buf = malloc(bytes_per_lba);
	else
		buf = static_buf;

	if (BootDeviceReadLBA(1, 1, buf)) {
		debug(PREFIX "get_ending_lba: read primary GPT header fail\n");
		goto EXIT;
	}

	ret = *(uint64_t*) (buf + BACKUP_LBA_OFFSET);

EXIT:
	if (buf != static_buf)
		free(buf);

	return ret;
}

int set_bootdev(char *ifname, int dev, int part)
{
	disk_partition_t part_info;

	if ((bootdev_config.dev_desc = get_dev(ifname, dev)) == NULL)
		goto cleanup; /* block device not supported */

	if (part == 0) {
		bootdev_config.offset = 0;
		bootdev_config.limit = bootdev_config.dev_desc->lba;
		return 0;
	}

	if (get_partition_info(bootdev_config.dev_desc, part, &part_info))
		goto cleanup; /* cannot find partition */

	bootdev_config.offset = part_info.start;
	bootdev_config.limit = part_info.size;

	return 0;

cleanup:
	bootdev_config.dev_desc = NULL;
	bootdev_config.offset = 0;
	bootdev_config.limit = 0;

	return 1;
}

int BootDeviceReadLBA(uint64_t lba_start, uint64_t lba_count, void *buffer)
{
	block_dev_desc_t *dev_desc;

	if ((dev_desc = bootdev_config.dev_desc) == NULL)
		return 1; /* No boot device configured */

	if (lba_start + lba_count > bootdev_config.limit)
		return 1; /* read out of range */

	if (dev_desc->block_read(dev_desc->dev,
				bootdev_config.offset + lba_start, lba_count,
				buffer) < 0)
		return 1; /* error reading blocks */

	return 0;
}

int BootDeviceWriteLBA(uint64_t lba_start, uint64_t lba_count,
		const void *buffer)
{
	block_dev_desc_t *dev_desc;

	if ((dev_desc = bootdev_config.dev_desc) == NULL)
		return 1; /* No boot device configured */

	if (lba_start + lba_count > bootdev_config.limit)
		return 1; /* read out of range */

	if (dev_desc->block_write(dev_desc->dev,
				bootdev_config.offset + lba_start, lba_count,
				buffer) < 0)
		return 1; /* error reading blocks */

	return 0;
}

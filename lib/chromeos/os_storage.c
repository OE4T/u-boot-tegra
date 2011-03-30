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
#include <chromeos/os_storage.h>

#include <boot_device.h>
#include <load_kernel_fw.h>
#include <vboot_nvstorage.h>
#include <vboot_struct.h>

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

#undef PREFIX
#define PREFIX "load_kernel_wrapper: "

int load_kernel_wrapper(LoadKernelParams *params,
		void *gbb_data, uint64_t gbb_size, uint64_t boot_flags,
		uint8_t *shared_data_blob)
{
	int status = LOAD_KERNEL_NOT_FOUND;
	block_dev_desc_t *dev_desc;
	VbNvContext vnc;

	memset(params, '\0', sizeof(*params));

	dev_desc = get_bootdev();
	if (!dev_desc) {
		debug(PREFIX "get_bootdev fail\n");
		goto EXIT;
	}

	params->gbb_data = gbb_data;
	params->gbb_size = gbb_size;

	params->boot_flags = boot_flags;
	params->shared_data_blob = shared_data_blob ? shared_data_blob :
			(uint8_t *) CONFIG_VB_SHARED_DATA_BLOB;
	params->shared_data_size = CONFIG_VB_SHARED_DATA_SIZE;

	params->bytes_per_lba = get_bytes_per_lba();
	params->ending_lba = get_ending_lba();

	params->kernel_buffer = (uint8_t *) CONFIG_LOADADDR;
	params->kernel_buffer_size = CONFIG_MAX_KERNEL_SIZE;

        /* TODO: load vnc.raw from NV storage */
	params->nv_context = &vnc;

	debug(PREFIX "call LoadKernel() with parameters...\n");
	debug(PREFIX "shared_data_blob:     0x%p\n",
			params->shared_data_blob);
	debug(PREFIX "bytes_per_lba:        %d\n",
			(int) params->bytes_per_lba);
	debug(PREFIX "ending_lba:           0x%08x\n",
			(int) params->ending_lba);
	debug(PREFIX "kernel_buffer:        0x%p\n",
			params->kernel_buffer);
	debug(PREFIX "kernel_buffer_size:   0x%08x\n",
			(int) params->kernel_buffer_size);
	debug(PREFIX "boot_flags:           0x%08x\n",
			(int) params->boot_flags);

	status = LoadKernel(params);

	if (vnc.raw_changed) {
		/* TODO: save vnc.raw to NV storage */
	}

EXIT:
	debug(PREFIX "LoadKernel status: %d\n", status);
	if (status == LOAD_KERNEL_SUCCESS) {
		debug(PREFIX "partition_number:   0x%08x\n",
				(int) params->partition_number);
		debug(PREFIX "bootloader_address: 0x%08x\n",
				(int) params->bootloader_address);
		debug(PREFIX "bootloader_size:    0x%08x\n",
				(int) params->bootloader_size);
	}

	return status;
}

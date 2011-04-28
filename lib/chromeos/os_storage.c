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

/* TODO For load fmap; remove when not used */
#include <chromeos/firmware_storage.h>

/* TODO For strcpy; remove when not used */
#include <linux/string.h>

/* TODO remove when not used */
extern uint64_t get_nvcxt_lba(void);

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

int load_kernel_wrapper_core(LoadKernelParams *params,
			     void *gbb_data, uint64_t gbb_size,
			     uint64_t boot_flags, VbNvContext *nvcxt,
			     uint8_t *shared_data_blob,
			     int bypass_load_kernel)
{
	/*
	 * TODO(clchiou): Hack for bringing up factory; preserve recovery
	 * reason before LoadKernel destroys it. Remove when not needed.
	 */
	uint32_t reason = 0;
	VbNvGet(nvcxt, VBNV_RECOVERY_REQUEST, &reason);

	int status = LOAD_KERNEL_NOT_FOUND;
	block_dev_desc_t *dev_desc;

	memset(params, '\0', sizeof(*params));

	if (!bypass_load_kernel) {
		dev_desc = get_bootdev();
		if (!dev_desc) {
			debug(PREFIX "get_bootdev fail\n");
			goto EXIT;
		}
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

	params->nv_context = nvcxt;

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

	if (!bypass_load_kernel) {
		status = LoadKernel(params);
	} else {
		status = LOAD_KERNEL_SUCCESS;
		params->partition_number = 2;
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

		if (params->partition_number == 2) {
			setenv("kernelpart", "2");
			setenv("rootpart", "3");
		} else if (params->partition_number == 4) {
			setenv("kernelpart", "4");
			setenv("rootpart", "5");
		} else {
			debug(PREFIX "unknown kernel partition: %d\n",
					(int) params->partition_number);
			status = LOAD_KERNEL_NOT_FOUND;
		}
	}

	/*
	 * TODO(clchiou): This is an urgent hack for bringing up factory. We
	 * fill in data that will be used by kernel at last 1MB space.
	 *
	 * Rewrite this part after the protocol specification between
	 * Chrome OS firmware and kernel is finalized.
	 */
	if (status == LOAD_KERNEL_SUCCESS) {
		DECLARE_GLOBAL_DATA_PTR;

		void *kernel_shared_data = (void*)
			gd->bd->bi_dram[CONFIG_NR_DRAM_BANKS-1].start +
			gd->bd->bi_dram[CONFIG_NR_DRAM_BANKS-1].size - SZ_1M;

		struct {
			uint32_t	total_size;
			uint8_t		signature[12];
			uint64_t	nvcxt_lba;
			uint8_t		gpio[11];
			uint8_t		binf[5];
			uint8_t		nvcxt_cache[VBNV_BLOCK_SIZE];
			uint32_t	chsw;
			uint8_t		hwid[256];
			uint8_t		fwid[256];
			uint8_t		frid[256];
			uint32_t	vbnv[2];
			uint8_t		shared_data_body[CONFIG_LENGTH_FMAP];
		} __attribute__((packed)) *sd = kernel_shared_data;

		int i;

		debug(PREFIX "kernel shared data at %p\n", kernel_shared_data);

		memset(sd, '\0', sizeof(*sd));

		strcpy((char*) sd->signature, "CHROMEOS");

		/*
		 * chsw bit value
		 *   bit 0x00000002 : recovery button pressed
		 *   bit 0x00000020 : developer mode enabled
		 *   bit 0x00000200 : firmware write protect disabled
		 */
		if (params->boot_flags & BOOT_FLAG_RECOVERY)
			sd->chsw |= 0x002;
		if (params->boot_flags & BOOT_FLAG_DEVELOPER)
			sd->chsw |= 0x020;
		sd->chsw |= 0x200; /* so far write protect is disabled */

		strcpy((char*) sd->hwid, CONFIG_CHROMEOS_HWID);
		strcpy((char*) sd->fwid, "ARM Firmware ID");
		strcpy((char*) sd->frid, "ARM Read-Only Firmware ID");

		sd->binf[0] = 0; /* boot reason; always 0 */
		if (params->boot_flags & BOOT_FLAG_RECOVERY) {
			sd->binf[1] = 0; /* active main firmware */
			sd->binf[3] = 0; /* active firmware type */
		} else {
			sd->binf[1] = 1; /* active main firmware */
			sd->binf[3] = 1; /* active firmware type */
		}
		sd->binf[2] = 0; /* active EC firmware */
		sd->binf[4] = reason;

		/* sd->gpio[i] == 1 if it is active-high */
		sd->gpio[1] = 1; /* only developer mode gpio is active high */

		sd->vbnv[0] = 0;
		sd->vbnv[1] = VBNV_BLOCK_SIZE;

		firmware_storage_t file;
		firmware_storage_init(&file);
		firmware_storage_read(&file,
				CONFIG_OFFSET_FMAP, CONFIG_LENGTH_FMAP,
				sd->shared_data_body);
		file.close(file.context);

		sd->total_size = sizeof(*sd);

		sd->nvcxt_lba = get_nvcxt_lba();

		memcpy(sd->nvcxt_cache,
				params->nv_context->raw, VBNV_BLOCK_SIZE);

		debug(PREFIX "chsw     %08x\n", sd->chsw);
		for (i = 0; i < 5; i++)
			debug(PREFIX "binf[%2d] %08x\n", i, sd->binf[i]);
		for (i = 0; i < 11; i++)
			debug(PREFIX "gpio[%2d] %08x\n", i, sd->gpio[i]);
		debug(PREFIX "vbnv[ 0] %08x\n", sd->vbnv[0]);
		debug(PREFIX "vbnv[ 1] %08x\n", sd->vbnv[1]);
		debug(PREFIX "fmap     %08llx\n", sd->fmap_start_address);
		debug(PREFIX "nvcxt    %08llx\n", sd->nvcxt_lba);
		debug(PREFIX "nvcxt_c  ");
		for (i = 0; i < VBNV_BLOCK_SIZE; i++)
			debug("%02x", sd->nvcxt_cache[i]);
		putc('\n');
	}

	return status;
}

int load_kernel_wrapper(LoadKernelParams *params,
			void *gbb_data, uint64_t gbb_size,
			uint64_t boot_flags, VbNvContext *nvcxt,
			uint8_t *shared_data_blob)
{
	return load_kernel_wrapper_core(params, gbb_data, gbb_size, boot_flags,
					nvcxt, shared_data_blob, 0);
}

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
#include <chromeos/boot_device_impl.h>
#include <chromeos/firmware_storage.h>
#include <chromeos/load_util.h>

/* vboot_reference interface */
#include <vboot_struct.h>

#define PREFIX "load_kernel_wrapper: "

void *load_gbb(firmware_storage_t *file, uint64_t *gbb_size_ptr)
{
	void *gbb_data;
	uint64_t gbb_size;

	gbb_size = CONFIG_LENGTH_GBB;
	gbb_data = malloc(CONFIG_LENGTH_GBB);
	if (gbb_data) {
		debug(PREFIX "cannot malloc gbb\n");
		return NULL;
	}

	if (read_firmware_device(file, CONFIG_OFFSET_GBB, gbb_data, gbb_size)) {
		debug(PREFIX "read gbb fail\n");
		free(gbb_data);
		return NULL;
	}

	*gbb_size_ptr = gbb_size;
	return gbb_data;
}

int load_kernel_wrapper(LoadKernelParams *params,
		void *gbb_data, uint64_t gbb_size, uint64_t boot_flags)
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

	if (boot_flags & BOOT_FLAG_RECOVERY) {
		params->shared_data_blob = NULL;
		params->shared_data_size = 0;
	} else {
		params->shared_data_blob =
			(uint8_t *) CONFIG_VB_SHARED_DATA_BLOB;
		params->shared_data_size = CONFIG_VB_SHARED_DATA_SIZE;
	}

	params->bytes_per_lba = (uint64_t) dev_desc->blksz;
	params->ending_lba = (uint64_t) get_limit() - 1;

	params->kernel_buffer = (uint8_t *) CONFIG_LOADADDR;
	params->kernel_buffer_size = CONFIG_MAX_KERNEL_SIZE;

        /* TODO: load vnc.raw from NV storage */
	params->nv_context = &vnc;

	debug(PREFIX "call LoadKernel() with parameters...\n");
	debug(PREFIX "header_sign_key_blob: 0x%p\n",
			params.header_sign_key_blob);
	debug(PREFIX "bytes_per_lba:        %d\n",
			(int) params.bytes_per_lba);
	debug(PREFIX "ending_lba:           0x%08x\n",
			(int) params.ending_lba);
	debug(PREFIX "kernel_buffer:        0x%p\n",
			params.kernel_buffer);
	debug(PREFIX "kernel_buffer_size:   0x%08x\n",
			(int) params.kernel_buffer_size);
	debug(PREFIX "boot_flags:           0x%08x\n",
			(int) params.boot_flags);

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

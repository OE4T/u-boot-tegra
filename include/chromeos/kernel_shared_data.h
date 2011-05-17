/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __CHROMEOS_KERNEL_SHARED_DATA_H__
#define __CHROMEOS_KERNEL_SHARED_DATA_H__

#include <vboot_nvstorage.h>

/* U-Boot version string with build datetime */
extern const char version_string[];

#define ID_LEN		256

typedef struct {
	uint32_t	total_size;
	uint8_t		signature[10];
	uint16_t	version;
	uint64_t	nvcxt_lba;
	uint16_t	vbnv[2];
	uint8_t		nvcxt_cache[VBNV_BLOCK_SIZE];
	uint8_t		write_protect_sw;
	uint8_t		recovery_sw;
	uint8_t		developer_sw;
	uint8_t		binf[5];
	uint32_t	chsw;
	uint8_t		hwid[ID_LEN];
	uint8_t		fwid[ID_LEN];
	uint8_t		frid[ID_LEN];
	uint32_t	fmap_base;
	uint8_t		shared_data_body[CONFIG_LENGTH_FMAP];
} __attribute__((packed)) KernelSharedDataType;

/* Returns the kernel shared data point. */
KernelSharedDataType *get_kernel_shared_data(void);

/* Clears the kernel shared data content. */
void clear_kernel_shared_data(void);

#endif /* __CHROMEOS_KERNEL_SHARED_DATA_H__ */

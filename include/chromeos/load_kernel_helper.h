/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_LOAD_KERNEL_HELPER_H_
#define CHROMEOS_LOAD_KERNEL_HELPER_H_

#include <linux/types.h>
#include <vboot_nvstorage.h>

/**
 * This loads and verifies the kernel image on storage device that is specified
 * by set_bootdev. The caller of this functions must have called set_bootdev
 * first.
 *
 * @param boot_flags are bitwise-or'ed of flags in load_kernel_fw.h
 * @param gbb_data points to a GBB blob
 * @param gbb_size is the size of the GBB blob
 * @param vbshared_data points to VbSharedData blob
 * @param vbshared_size is the size of the VbSharedData blob
 * @param nvcxt points to a VbNvContext object
 * @return error codes, e.g., LOAD_KERNEL_INVALID if either the verification
 *         fails or the kernel is not bootable; otherwise, this function
 *         boots the kernel and never returns to its caller
 */
int boot_kernel(uint64_t boot_flags,
		void *gbb_data, uint32_t gbb_size,
		void *vbshared_data, uint32_t vbshared_size,
		VbNvContext *nvcxt);

#endif /* CHROMEOS_LOAD_KERNEL_HELPER_H_ */

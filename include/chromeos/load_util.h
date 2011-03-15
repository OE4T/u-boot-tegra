/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CHROMEOS_LOAD_UTIL__
#define __CHROMEOS_LOAD_UTIL__

#include <linux/types.h>
#include <chromeos/firmware_storage.h>

#include <load_kernel_fw.h>

/*
 * Helper function for loading GBB
 *
 * On success, return a pointer to a malloc'ed area storing GBB, and caller is
 * responsible to free this area. On error, return NULL.
 */
void *load_gbb(firmware_storage_t *file, uint64_t *gbb_size_ptr);

/*
 * Wrapper of LoadKernel() function. Returns the return value of LoadKernel().
 *
 * See vboot_reference/firmware/include/load_kernel_fw.h for documentation.
 */
int load_kernel_wrapper(LoadKernelParams *params,
		void *gbb_data, uint64_t gbb_size, uint64_t boot_flags);

#endif /* __CHROMEOS_LOAD_UTIL__ */

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

#include <load_kernel_fw.h>
#include <vboot_nvstorage.h>

/* Prepare bootargs environment variable that holds kernel parameter */
void prepare_bootargs(void);

/*
 * Given the bootloader_addres, load the kernel config as bootargs.
 * Return zero if success and non-zero if error.
 */
int load_kernel_config(uint64_t bootloader_address);

/*
 * Wrapper of LoadKernel() function. Returns the return value of LoadKernel().
 *
 * See vboot_reference/firmware/include/load_kernel_fw.h for documentation.
 *
 * If <shared_data_blob> is NULL, it will use a system default location.
 */
int load_kernel_wrapper(LoadKernelParams *params,
		void *gbb_data, uint64_t gbb_size, uint64_t boot_flags,
		VbNvContext *nvcxt, uint8_t *shared_data_blob);

/*
 * Actual wrapper implementation. Most callers invoke it with
 * 'bypass_load_kernel' set to False. If it is set to True - the shared memory
 * is intialized, but the actual kernel load function is not invoked. This
 * facilitates debugging when the kernel is loaded by some other means (for
 * instance netbooted)
 */
int load_kernel_wrapper_core(LoadKernelParams *params, void *gbb_data,
			     uint64_t gbb_size, uint64_t boot_flags,
			     VbNvContext *nvcxt, uint8_t *shared_data_blob,
			     int bypass_load_kernel);

/*
 * Boot kernel specified at the address in <params>. This function never
 * returns.
 */
void boot_kernel(LoadKernelParams *params);

#endif /* CHROMEOS_LOAD_KERNEL_HELPER_H_ */

/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Helper functions for LoadFirmware */

#ifndef CHROMEOS_LOAD_FIRMWARE_HELPER_H_
#define CHROMEOS_LOAD_FIRMWARE_HELPER_H_

#include <linux/types.h>
#include <chromeos/firmware_storage.h>

#include <vboot_nvstorage.h>

#define FIRMWARE_A		0
#define FIRMWARE_B		1
#define FIRMWARE_RECOVERY	2

/*
 * Helper function for loading GBB
 *
 * On success, it malloc an area for storing GBB, and returns 0.
 * On error, it returns non-zero value, and gbb_size_ptr and gbb_size_ptr are
 * untouched.
 */
int load_gbb(firmware_storage_t *file,
		void **gbb_data_ptr, uint64_t *gbb_size_ptr);

/*
 * Load and verify rewritable firmware. A wrapper of LoadFirmware() function.
 *
 * Returns what is returned by LoadFirmware().
 *
 * For documentation of return values of LoadFirmware(), <primary_firmware>, and
 * <boot_flags>, please refer to
 * vboot_reference/firmware/include/load_firmware_fw.h
 *
 * The shared data blob for the firmware image is stored in
 * <shared_data_blob>. When pass NULL, use system default location.
 *
 * Pointer to loaded firmware is malloc()'ed and stored in <firmware_data_ptr>
 * when success. Otherwise, <firmware_data_ptr> is untouched.
 */
int load_firmware_wrapper(firmware_storage_t *file,
		const int primary_firmware,
		const uint64_t boot_flags,
		VbNvContext *nvcxt,
		void *shared_data_blob,
		uint8_t **firmware_data_ptr);

#endif /* CHROMEOS_LOAD_FIRMWARE_HELPER_H_ */

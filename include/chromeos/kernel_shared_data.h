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

#define ID_LEN		256

#define RECOVERY_FIRMWARE	0
#define REWRITABLE_FIRMWARE_A	1
#define REWRITABLE_FIRMWARE_B	2

#define RECOVERY_TYPE		0
#define NORMAL_TYPE		1
#define DEVELOPER_TYPE		2

/**
 * This returns the pointer the last 1MB space of RAM. It is where the data blob
 * that firmware shares with kernel _temporarily_. This blob will be embedded in
 * a device tree. By then, this function will be removed.
 *
 * @return pointer to the last 1MB space of RAM.
 */
void *get_last_1mb_of_ram(void);

/**
 * This initializes the data blob that we will pass to kernel, and later be
 * used by crossystem. Note that:
 * - It does not initialize information of the main firmware, e.g., fwid. This
 *   information must be initialized in subsequent calls to the setters below.
 * - The recovery reason is default to VBNV_RECOVERY_NOT_REQUESTED.
 *
 * @param kernel_shared_data is the data blob that firmware shares with kernel
 * @param frid r/o firmware id; a zero-terminated string shorter than ID_LEN
 * @param fmap_data points to fmap blob
 * @param fmap_size is the size of the fmap blob
 * @param gbb_data points to gbb blob
 * @param gbb_size is the size of the gbb blob
 * @param nvcxt points to a VbNvContext object
 * @param write_protect_sw stores the value of write protect gpio
 * @param recovery_sw stores the value of recovery mode gpio
 * @param developer_sw stores the value of developer mode gpio
 * @return 0 if it succeeds; non-zero if it fails
 */
int cros_ksd_init(void *kernel_shared_data, uint8_t *frid,
		void *fmap_data, uint32_t fmap_size,
		void *gbb_data, uint32_t gbb_size,
		VbNvContext *nvcxt,
		int write_protect_sw, int recovery_sw, int developer_sw);

/**
 * This sets rewritable firmware id. It should only be called in non-recovery
 * boots.
 *
 * @param kernel_shared_data is the data blob that firmware shares with kernel
 * @param fwid r/w firmware id; a zero-terminated string shorter than ID_LEN
 * @return 0 if it succeeds; non-zero if it fails
 */
int cros_ksd_set_fwid(void *kernel_shared_data, uint8_t *fwid);

/**
 * This sets active main firmware.
 *
 * @param kernel_shared_data is the data blob that firmware shares with kernel
 * @param which - recovery: 0; rewritable A: 1; rewritable B: 2
 * @return 0 if it succeeds; non-zero if it fails
 */
int cros_ksd_set_active_main_firmware(void *kernel_shared_data, int which);

/**
 * This sets active main firmware type.
 *
 * @param kernel_shared_data is the data blob that firmware shares with kernel
 * @param type - recovery: 0; normal: 1; developer: 2
 * @return 0 if it succeeds; non-zero if it fails
 */
int cros_ksd_set_active_firmware_type(void *kernel_shared_data, int type);

/**
 * This sets recovery reason.
 *
 * @param kernel_shared_data is the data blob that firmware shares with kernel
 * @param reason - the recovery reason
 * @return 0 if it succeeds; non-zero if it fails
 */
int cros_ksd_set_recovery_reason(void *kernel_shared_data, uint32_t reason);

/**
 * This prints out the data blob content to debug output.
 */
void cros_ksd_dump(void *kernel_shared_data);

#endif /* __CHROMEOS_KERNEL_SHARED_DATA_H__ */

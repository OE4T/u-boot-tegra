/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation details of GetFirmwareBody */

#ifndef CHROMEOS_GET_FIRMWARE_BODY_H_
#define CHROMEOS_GET_FIRMWARE_BODY_H_

#include <linux/types.h>
#include <chromeos/firmware_storage.h>

typedef struct {
	firmware_storage_t *file;

	/*
	 * This field is offset of firmware data sections, from the beginning
	 * of firmware storage device.
	 */
	off_t firmware_data_offset[2];

	/*
	 * These pointers will point to firmware data loaded by
	 * GetFirmwareBody().
	 */
	uint8_t *firmware_body[2];
} get_firmware_body_internal_t;

/* Initialize fields for talking to GetFirmwareBody(). */
int get_firmware_body_internal_setup(get_firmware_body_internal_t *gfbi,
		firmware_storage_t *file,
		const off_t firmware_data_offset_0,
		const off_t firmware_data_offset_1);

/* Dispose fields that are used for communicating with GetFirmwareBody(). */
int get_firmware_body_internal_dispose(get_firmware_body_internal_t *gfbi);

#endif /* CHROMEOS_GET_FIRMWARE_BODY_H_ */

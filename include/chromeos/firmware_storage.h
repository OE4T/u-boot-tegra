/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/*
 * The firmware_storage provides a interface for GetFirmware to interact with
 * board-specific firmware storage device.
 */

#ifndef __FIRMWARE_STORAGE_H__
#define __FIRMWARE_STORAGE_H__

#include <linux/types.h>

enum whence_t { SEEK_SET, SEEK_CUR, SEEK_END };

/*
 * This struct is the interface for accessing firmware storage.
 * It is also used as the internal data for caller of LoadFirmware() to talk to
 * GetFirmwareBody().
 */
typedef struct {
	/*
	 * The semantic (argument and return value) is similar with system
	 * call lseek(2) and read(2), except that file description is replaced
	 * by <context>.
	 *
	 * <context> stores current status of the storage device and
	 * local states of the driver.
	 *
	 * A storage device driver that implements this interface must provide
	 * methods to initialize <context> to caller of LoadFirmware().
	 *
	 * For example, common/cmd_cros.c implements a driver of RAM.
	 */
	off_t (*seek)(void *context, off_t offset, enum whence_t whence);
	ssize_t (*read)(void *context, void *buf, size_t count);
	void *context;

	/*
	 * This field is offset of firmware data sections, from the beginning
	 * of firmware storage device.
	 *
	 * This field is only used by GetFirmwareBody(). Caller of
	 * LoadFirmware() must initialize this field.
	 */
	off_t firmware_data_offset[2];
} caller_internal_t;

#endif /* __FIRMWARE_STORAGE_H_ */

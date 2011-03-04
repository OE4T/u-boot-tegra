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

/* Internal data for caller of LoadFirmware() to talk to GetFirmwareBody() */
struct caller_internal_s {
	off_t (*seek)(void *context, off_t offset, enum whence_t whence);
	ssize_t (*read)(void *context, void *buf, size_t count);
	void *context;
};

typedef struct caller_internal_s caller_internal_t;

#endif /* __FIRMWARE_STORAGE_H_ */

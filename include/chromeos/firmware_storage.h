/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Interface for access various storage device */

#ifndef CHROMEOS_FIRMWARE_STORAGE_H_
#define CHROMEOS_FIRMWARE_STORAGE_H_

#include <linux/types.h>

enum whence_t { SEEK_SET, SEEK_CUR, SEEK_END };

/*
 * The semantic (argument and return value) is similar with system
 * call lseek(2), read(2) and write(2), except that file description
 * is replaced by <context>.
 *
 * <context> stores current status of the storage device and
 * local states of the driver.
 */
typedef struct {
	off_t (*seek)(void *context, off_t offset, enum whence_t whence);
	ssize_t (*read)(void *context, void *buf, size_t count);
	ssize_t (*write)(void *context, const void *buf, size_t count);
	int (*close)(void *context);

	/* Returns 0 if success, nonzero if error. */
	int (*lock_device)(void *context);

	void *context;
} firmware_storage_t;

/* Returns 0 if success, nonzero if error. */
int firmware_storage_init_nand(firmware_storage_t *file);
int firmware_storage_init_spi(firmware_storage_t *file);
int firmware_storage_init_ram(firmware_storage_t *file, void *beg, void *end);

extern int (*const firmware_storage_init)(firmware_storage_t *file);

/*
 * Read/write <count> bytes, starting from <offset>, from/to firmware storage
 * device <file> into/from buffer <buf>.
 *
 * Return 0 on success, non-zero on error.
 */
int firmware_storage_read(firmware_storage_t *file,
		const off_t offset, const size_t count, void *buf);
int firmware_storage_write(firmware_storage_t *file,
		const off_t offset, const size_t count, const void *buf);

#endif /* CHROMEOS_FIRMWARE_STORAGE_H_ */

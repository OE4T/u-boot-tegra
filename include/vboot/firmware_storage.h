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

#ifndef VBOOT_FIRMWARE_STORAGE_H_
#define VBOOT_FIRMWARE_STORAGE_H_

#include <linux/types.h>

enum whence_t { SEEK_SET, SEEK_CUR, SEEK_END };

/*
 * The semantic (argument and return value) is similar with system
 * call lseek(2), read(2) and write(2), except that file descriptor
 * is replaced by [context].
 */
typedef struct {
	off_t (*seek)(void *context, off_t offset, enum whence_t whence);
	ssize_t (*read)(void *context, void *buf, size_t count);
	ssize_t (*write)(void *context, const void *buf, size_t count);
	int (*close)(void *context);

	/**
	 * This locks the storage device, i.e., enables write protect.
	 *
	 * @return 0 if it succeeds, non-zero if it fails
	 */
	int (*lock_device)(void *context);

	void *context;
} firmware_storage_t;

/**
 * This opens SPI flash device
 *
 * @param file - the opened SPI flash device
 * @return 0 if it succeeds, non-zero if it fails
 */
int firmware_storage_open_spi(firmware_storage_t *file);

/**
 * These read or write [count] bytes starting from [offset] of storage into or
 * from the [buf]. These are really wrappers of file->{seek,read,write}.
 *
 * @param file - storage device you access
 * @param offset - where on the device you access
 * @param count - the amount of data in byte you access
 * @param buf - the data that these functions read from or write to
 * @return 0 if it succeeds, non-zero if it fails
 */
int firmware_storage_read(firmware_storage_t *file,
		const off_t offset, const size_t count, void *buf);
int firmware_storage_write(firmware_storage_t *file,
		const off_t offset, const size_t count, const void *buf);

#endif /* CHROMEOS_FIRMWARE_STORAGE_H_ */

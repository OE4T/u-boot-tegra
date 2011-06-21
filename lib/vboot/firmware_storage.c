/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <vboot/firmware_storage.h>

/* Import the header files from vboot_reference. */
#include <vboot_api.h>

#define PREFIX "firmware_storage: "

int firmware_storage_read(firmware_storage_t *file,
		const off_t offset, const size_t count, void *buf)
{
	ssize_t size;
	size_t remain;

	VbExDebug(PREFIX "read\n");

	if (file->seek(file->context, offset, SEEK_SET) < 0) {
		VbExDebug(PREFIX "seek to address 0x%08lx fail\n", offset);
		return -1;
	}

	size = 0;
	remain = count;
	while (remain > 0 &&
			(size = file->read(file->context, buf, remain)) > 0) {
		remain -= size;
		buf += size;
	}

	if (size < 0) {
		VbExDebug(PREFIX "an error occur when read firmware: %u\n",
				size);
		return -1;
	}

	if (remain > 0) {
		VbExDebug(PREFIX "cannot read all data: %u\n", remain);
		return -1;
	}

	return 0;
}

int firmware_storage_write(firmware_storage_t *file,
		const off_t offset, const size_t count, const void *buf)
{
	ssize_t size;
	size_t remain;

	VbExDebug(PREFIX "write\n");

	if (file->seek(file->context, offset, SEEK_SET) < 0) {
		VbExDebug(PREFIX "seek to address 0x%08lx fail\n", offset);
		return -1;
	}

	size = 0;
	remain = count;
	while (remain > 0 &&
			(size = file->write(file->context, buf, remain)) > 0) {
		remain -= size;
		buf += size;
	}

	if (size < 0) {
		VbExDebug(PREFIX "an error occur when write firmware: %d\n",
				size);
		return -1;
	}

	if (remain > 0) {
		VbExDebug(PREFIX "cannot write all data: %u\n", remain);
		return -1;
	}

	return 0;
}

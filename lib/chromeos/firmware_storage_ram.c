/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of firmware storage access interface for NAND */

#include <common.h>
#include <malloc.h>
#include <chromeos/firmware_storage.h>

struct context_t {
	void *beg, *cur, *end;
};

static off_t seek_ram(void *context, off_t offset, enum whence_t whence)
{
	struct context_t *cxt = (struct context_t *) context;
	void *cur;

	if (whence == SEEK_SET)
		cur = cxt->beg + offset;
	else if (whence == SEEK_CUR)
		cur = cxt->cur + offset;
	else if (whence == SEEK_END)
		cur = cxt->end + offset;
	else {
		debug("seek_ram: unknown whence value: %d\n", whence);
		return -1;
	}

	if (cur < cxt->beg) {
		debug("seek_ram: offset underflow: %p < %p\n", cur, cxt->beg);
		return -1;
	}

	if (cur >= cxt->end) {
		debug("seek_ram: offset exceeds size: %p >= %p\n", cur,
				cxt->end);
		return -1;
	}

	cxt->cur = cur;
	return cur - cxt->beg;
}

static ssize_t read_ram(void *context, void *buf, size_t count)
{
	struct context_t *cxt = (struct context_t *) context;

	if (count == 0)
		return 0;

	if (count > cxt->end - cxt->cur)
		count = cxt->end - cxt->cur;

	memcpy(buf, cxt->cur, count);
	cxt->cur += count;

	return count;
}

static ssize_t write_ram(void *context, const void *buf, size_t count)
{
	struct context_t *cxt = (struct context_t *) context;

	if (count == 0)
		return 0;

	if (count > cxt->end - cxt->cur)
		count = cxt->end - cxt->cur;

	memcpy(cxt->cur, buf, count);
	cxt->cur += count;

	return count;
}

static int close_ram(void *context)
{
	free(context);
	return 0;
}

static int lock_ram(void *context)
{
	/* TODO Implement lock device */
	return 0;
}

int firmware_storage_init_ram(firmware_storage_t *file, void *beg, void *end)
{
	struct context_t *cxt;

	cxt = malloc(sizeof(*cxt));
	cxt->beg = beg;
	cxt->cur = beg;
	cxt->end = end;

	file->seek = seek_ram;
	file->read = read_ram;
	file->write = write_ram;
	file->close = close_ram;
	file->lock_device = lock_ram;
	file->context = (void*) cxt;

	return 0;
}

/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board EEPROM driver functions */

#include <common.h>
#include <malloc.h>
#include <chromeos/hardware_interface.h>

/* TODO Replace dummy functions below with real implementation. */

int lock_down_eeprom(void) { return 0; }

/* TODO Replace mem_seek/read with spi_seek/read when moving firmware storage
 * from NAND to SPI Flash */

struct context_t {
	void *begin, *cur, *end;
};

static off_t mem_seek(void *context, off_t offset, enum whence_t whence)
{
	void *begin, *cur, *end;

	begin = ((struct context_t *) context)->begin;
	cur = ((struct context_t *) context)->cur;
	end = ((struct context_t *) context)->end;

	if (whence == SEEK_SET)
		cur = begin + offset;
	else if (whence == SEEK_CUR)
		cur = cur + offset;
	else if (whence == SEEK_END)
		cur = end + offset;
	else {
		debug("mem_seek: unknown whence value: %d\n", whence);
		return -1;
	}

	if (cur < begin) {
		debug("mem_seek: offset underflow: %p < %p\n", cur, begin);
		return -1;
	}

	if (cur >= end) {
		debug("mem_seek: offset exceeds size: %p >= %p\n", cur, end);
		return -1;
	}

	((struct context_t *) context)->cur = cur;
	return cur - begin;
}

static ssize_t mem_read(void *context, void *buf, size_t count)
{
	void *begin, *cur, *end;

	if (count == 0)
		return 0;

	begin = ((struct context_t *) context)->begin;
	cur = ((struct context_t *) context)->cur;
	end = ((struct context_t *) context)->end;

	if (count > end - cur)
		count = end - cur;

	memcpy(buf, cur, count);
	((struct context_t *) context)->cur += count;

	return count;
}

int init_firmware_storage(firmware_storage_t *f)
{
	struct context_t *context;

	context = (struct context_t *) malloc(sizeof(struct context_t));
	context->begin = (void *) TEXT_BASE;
	context->cur = (void *) TEXT_BASE;
	context->end = (void *) TEXT_BASE + CONFIG_FIRMWARE_SIZE;

	f->seek = mem_seek;
	f->read = mem_read;
	f->context = (void *) context;

	return 0;
}

int release_firmware_storage(firmware_storage_t *f)
{
	free(f->context);
	return 0;
}

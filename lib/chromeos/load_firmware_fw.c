/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
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

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <chromeos/firmware_storage.h>
#include <load_firmware_fw.h>

#define BUF_SIZE	256

int lookup_area(struct fmap *fmap, const char *name)
{
	size_t len = strlen(name);
	int i;
	for (i = 0; i < fmap->nareas; i++)
		if (!strncmp((const char *) fmap->areas[i].name, name, len))
			return i;
	return -1;
}

int GetFirmwareBody(LoadFirmwareParams *params, uint64_t index)
{
	caller_internal_t	*ci;
	firmware_storage_t	*f;
	const char		*name;
	int			i;
	struct fmap_area	*area;
	uint8_t			buf[BUF_SIZE];
	ssize_t			count;
	size_t			size;

	/* index = 0: firmware A; 1: firmware B; anything else: invalid */
	if (index != 0 && index != 1)
		return 1;

	ci = (caller_internal_t *) params->caller_internal;
	f = &ci->firmware_storage;

	/* TODO(clchiou): Consider look up by special flags? */
	name = (index == 0) ? "Firmware A Data" : "Firmware B Data";
	if ((i = lookup_area(f->fmap, name)) < 0)
		return 1;
	area = f->fmap->areas + i;

	if (ci->cached_image == NULL) {
		ci->cached_image = malloc(area->size);
	} else if (ci->size < area->size) {
		free(ci->cached_image);
		ci->cached_image = malloc(area->size);
	} /* else: reuse allocated space */
	ci->index = index;
	ci->size = area->size;

	for (size = 0; size < area->size; size += count) {
		if ((count = f->read(f, i, buf, sizeof(buf))) <= 0)
			return 1;
		UpdateFirmwareBodyHash(params, buf, count);
		memcpy(ci->cached_image + size, buf, count);
	}

	return 0;
}

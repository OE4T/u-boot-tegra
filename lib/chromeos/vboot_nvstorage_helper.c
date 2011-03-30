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

#include <common.h>
#include <chromeos/vboot_nvstorage_helper.h>

/*
 * TODO It should averagely distributed erase/write operation to entire flash
 * memory section allocated for VBNVCONTEXT to increase maximal lifetime.
 *
 * But since VbNvContext gets written infrequently enough, this is likely
 * an overkill.
 */

#define PREFIX "vboot_nvstorage_helper: "

int read_nvcontext(firmware_storage_t *file, VbNvContext *nvcxt)
{
	if (firmware_storage_read(file,
			CONFIG_OFFSET_VBNVCONTEXT, VBNV_BLOCK_SIZE,
			nvcxt->raw)) {
		debug(PREFIX "read_nvcontext fail\n");
		return -1;
	}

	if (VbNvSetup(nvcxt)) {
		debug(PREFIX "setup nvcontext fail\n");
		return -1;
	}

	return 0;
}

int write_nvcontext(firmware_storage_t *file, VbNvContext *nvcxt)
{
	if (firmware_storage_write(file,
			CONFIG_OFFSET_VBNVCONTEXT, VBNV_BLOCK_SIZE,
			nvcxt->raw)) {
		debug(PREFIX "write_nvcontext fail\n");
		return -1;
	}

	return 0;
}

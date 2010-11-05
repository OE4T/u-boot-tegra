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

/* Implementation of helper functions/wrappers for memory allocations,
 * manipulation and comparison for vboot library.
 */

#include <config.h>
#include <common.h>
#include <malloc.h>

/* HACK: Get rid of U-Boots debug and assert macros */
#undef error
#undef debug
#undef assert

/* HACK: We want to use malloc, free, memcmp, memcpy, memset */
#define _STUB_IMPLEMENTATION_

/* Import interface of vboot's helper functions */
#include <utility.h>

/* Is it defined in lib/string.c? */
int memcmp(const void *cs, const void *ct, size_t count);

void abort(void)
{
	reset_cpu(0);
}

#define exit(retcode) abort()

void error(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	puts("ERROR: ");
	vprintf(format, ap);
	va_end(ap);
	exit(1);
}

void debug(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	puts("DEBUG: ");
	vprintf(format, ap);
	va_end(ap);
}

void *Malloc(size_t size)
{
	void *p = malloc(size);
	if (!p) {
		/* Fatal Error. We must abort. */
		abort();
	}
	return p;
}

void Free(void *p)
{
	free(p);
}

int Memcmp(const void *src1, const void *src2, size_t n)
{
	return memcmp(src1, src2, n);
}

void *Memcpy(void *dest, const void *src, uint64_t n)
{
	return memcpy(dest, src, (size_t) n);
}

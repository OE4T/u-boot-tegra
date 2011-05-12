/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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
#include <chromeos/power_management.h>

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

/* Append underscore to prevent name conflict with abort() in
 * cpu/arm_cortexa9/tegra2/board.c (which is empty)
 */
void _abort(void)
{
	cold_reboot();
}

#define exit(retcode) _abort()

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
		_abort();
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

uint64_t VbGetTimer(void) {
	/* TODO: hook this up to a real timer; see utility.h */
	return 0;
}

uint64_t VbGetTimerMaxFreq(void) {
	/* TODO: return an approximate max timer frequency; see utility.h */
	return UINT64_C(2000000000);
}

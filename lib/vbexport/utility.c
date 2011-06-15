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
 * Implementation of APIs provided by firmware and exported to vboot_reference.
 * They includes debug output, memory allocation, timer and delay, etc.
 */

#include <common.h>
#include <malloc.h>
#include <chromeos/power_management.h>

/* Import the definition of vboot_wrapper interfaces. */
#include <vboot_api.h>

#define UINT32_MAX		(~0UL)
#define TICKS_PER_MSEC		(CONFIG_SYS_HZ / 1000)
#define MAX_MSEC_PER_LOOP	((UINT32_MAX / TICKS_PER_MSEC) / 2)

static void system_abort(void)
{
	/* Wait for 3 seconds to let users see error messages and reboot. */
	VbExSleepMs(3000);
	cold_reboot();
}

void VbExError(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	system_abort();
}

void VbExDebug(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
}

void *VbExMalloc(size_t size)
{
	void *ptr = memalign(CACHE_LINE_SIZE, size);
	if (!ptr) {
		VbExError("Internal malloc error.");
	}
	return ptr;
}

void VbExFree(void *ptr)
{
	free(ptr);
}

uint64_t VbExGetTimer(void)
{
	return timer_get_us();
}

void VbExSleepMs(uint32_t msec)
{
	uint32_t delay, start;

	/*
	 * Can't use entire UINT32_MAX range in the max delay, because it
	 * pushes get_timer() too close to wraparound. So use /2.
	 */
	while(msec > MAX_MSEC_PER_LOOP) {
		VbExSleepMs(MAX_MSEC_PER_LOOP);
		msec -= MAX_MSEC_PER_LOOP;
	}

	delay = msec * TICKS_PER_MSEC;
	start = get_timer(0);

	while (get_timer(start) < delay)
		udelay(100);
}

void VbExBeep(uint32_t msec, uint32_t frequency)
{
	/* TODO Implement it later. */
	VbExSleepMs(msec);
	VbExDebug("Beep!\n");
}

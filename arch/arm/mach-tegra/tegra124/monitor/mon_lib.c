/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include "mon.h"

void mon_text mon_memcpy(void *dest, const void *src, size_t n)
{
	if ((((u32)dest) & 3) || (((u32)src) & 3) || (n & 3)) {
		u8 *d = dest;
		const u8 *s = src;

		while (n--)
			*d++ = *s++;
	} else {
		u32 *d = dest;
		const u32 *s = src;

		n /= 4;
		while (n--)
			*d++ = *s++;
	}
}

void * mon_text mon_memset(void *dest, int c, size_t n)
{
	if ((((u32)dest) & 3) || (n & 3)) {
		u8 *d = dest;

		while (n--)
			*d++ = c;
	} else {
		u32 *d = dest;
		const u32 val = (c << 24) | (c << 16) | (c << 8) | c;

		n /= 4;
		while (n--)
			*d++ = val;
	}

	return dest;
}

void mon_text mon_printf(const char *s, ...)
{
	// FIXME: This is an extremely lame printf() implementation.
	mon_puts(MON_STR("MON: "));
	mon_puts(s);
	mon_putc('\n');
}

void mon_text mon_delay_usecs(u32 n)
{
	u32 start = readl(TEGRA_TMRUS_BASE);
	while ((readl(TEGRA_TMRUS_BASE) - start) < (n + 1))
		;
}

void mon_text mon_noreturn mon_error(void)
{
	mon_puts(MON_STR("MON: ERR: Hanging\n"));
	while (1)
		;
}

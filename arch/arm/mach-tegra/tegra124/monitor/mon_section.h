/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _MON_SECTION_H_
#define _MON_SECTION_H_

#include <stddef.h>

#define mon_data	__attribute__((section("._secure.data")))
#define mon_rodata	__attribute__((section("._secure.rodata")))
#define mon_text	__attribute__((section("._secure.text")))

#define MON_STR(s) (__extension__({static const char __mon_str[] mon_rodata = \
			(s); &__mon_str[0];}))

#define MON_SYM(x) mon_##x
#define MON_SYM_STR3(x) #x
#define MON_SYM_STR2(x) MON_SYM_STR3(x)
#define MON_SYM_STR(x) MON_SYM_STR2(MON_SYM(x))

/* mon_lib.h */
extern void mon_memcpy(void *dest, const void *src, size_t n);
extern void *mon_memset(void *dest, int c, size_t n);
extern void mon_printf(const char *s, ...);

#endif

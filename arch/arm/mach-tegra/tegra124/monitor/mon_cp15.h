/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _MON_CP15_H_
#define _MON_CP15_H_

static inline u32 mon_text mon_read_midr(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %[val], c0, c0, 0 \n\t" : [val] "=r" (val));
	return val;
}

static inline u32 mon_text mon_read_mpidr(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %[val], c0, c0, 5 \n\t" : [val] "=r" (val));
	return val;
}

static inline void mon_text mon_write_sctlr(u32 val)
{
	asm volatile("mcr p15, 0, %[val], c1, c0, 0 \n\t" : : [val] "r" (val));
	isb();
}

static inline u32 mon_text mon_read_actlr(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %[val], c1, c0, 1 \n\t" : [val] "=r" (val));
	return val;
}

static inline void mon_text mon_write_actlr(u32 val)
{
	asm volatile("mcr p15, 0, %[val], c1, c0, 1 \n\t" : : [val] "r" (val));
	isb();
}

static inline u32 mon_text mon_read_scr(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %[val], c1, c1, 0 \n\t" : [val] "=r" (val));
	return val;
}

static inline void mon_text mon_write_scr(u32 val)
{
	asm volatile("mcr p15, 0, %[val], c1, c1, 0 \n\t" : : [val] "r" (val));
	isb();
}

static inline u32 mon_text mon_read_nsacr(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %[val], c1, c1, 2 \n\t" : [val] "=r" (val));
	return val;
}

static inline void mon_text mon_write_nsacr(u32 val)
{
	asm volatile("mcr p15, 0, %[val], c1, c1, 2 \n\t" : : [val] "r" (val));
}

static inline void mon_text mon_dummy_dvm_op(void)
{
	asm volatile("mcr p15, 0, %[val], c7, c5, 6 \n\t" : : [val] "r" (0));
}

static inline void mon_text mon_dccimvac(u32 val)
{
	asm volatile("mcr p15, 0, %[val], c7, c14, 1\n\t" : : [val] "r" (val));
}

static inline void mon_text mon_write_cntfrq(u32 val)
{
	asm volatile("mcr p15, 0, %[val], c14, c0, 0\n\t" : : [val] "r" (val));
}

static inline u32 mon_text mon_read_l2ctlr(void)
{
	u32 val;
	asm volatile("mrc p15, 1, %[val], c9, c0, 2 \n\t" : [val] "=r" (val));
	return val;
}

static inline void mon_text mon_write_l2ctlr(u32 val)
{
	asm volatile("mcr p15, 1, %[val], c9, c0, 2 \n\t" : : [val] "r" (val));
}

static inline u32 mon_text mon_read_l2actlr(void)
{
	u32 val;
	asm volatile("mrc p15, 1, %[val], c15, c0, 0 \n\t" : [val] "=r" (val));
	return val;
}

static inline void mon_text mon_write_l2actlr(u32 val)
{
	asm volatile("mcr p15, 1, %[val], c15, c0, 0 \n\t" : : [val] "r" (val));
}

static inline u32 mon_text mon_read_l2pfr(void)
{
	u32 val;
	asm volatile("mrc p15, 1, %[val], c15, c0, 3 \n\t" : [val] "=r" (val));
	return val;
}

static inline void mon_text mon_write_l2pfr(u32 val)
{
	asm volatile("mcr p15, 1, %[val], c15, c0, 3 \n\t" : : [val] "r" (val));
}

static inline u32 mon_text mon_read_actlr2(void)
{
	u32 val;
	asm volatile("mrc p15, 1, %[val], c15, c0, 4 \n\t" : [val] "=r" (val));
	return val;
}

static inline void mon_text mon_write_actlr2(u32 val)
{
	asm volatile("mcr p15, 1, %[val], c15, c0, 4 \n\t" : : [val] "r" (val));
}

static inline u32 mon_text mon_read_cbar(void)
{
	u32 val;
	asm volatile("mrc p15, 4, %[val], c15, c0, 0 \n\t" : [val] "=r" (val));
	return val;
}

static inline void mon_text mon_clear_cntvoff(void)
{
	asm volatile("mcrr p15, 4, %[val], %[val], c14 \n\t" : : [val] "r" (0));
}

#endif

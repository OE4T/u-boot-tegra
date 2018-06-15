/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Derived from code:
 * Copyright (c) 2013 Andre Przywara <andre.przywara//linaro.org>
 */

#ifndef _MON_GIC_H_
#define _MON_GIC_H_

#include <common.h>
#include <asm/gic.h>
#include <asm/io.h>
#include "mon.h"

static inline u32 mon_text mon_gicd_addr(void)
{
	u32 cbar = mon_read_cbar();
	return (cbar & ~0xffff) + GIC_DIST_OFFSET;
}

static inline u32 mon_text mon_gicc_addr(void)
{
	u32 cbar = mon_read_cbar();
	return (cbar & ~0xffff) + GIC_CPU_OFFSET_A15;
}

static inline void mon_text mon_init_gicd(void)
{
	u32 group;
	u32 gicd = mon_gicd_addr();
	u32 icdictr = readl(gicd + GICD_TYPER);
	u32 groups = (icdictr & 0x1f) + 1;
	for (group = 0; group < groups; group++)
		writel(0xffffffff, gicd + GICD_IGROUPRn + (group * 4));
}

static inline void mon_text mon_init_gicc(void)
{
	u32 gicc = mon_gicc_addr();
	writel(BIT(1), gicc + GICC_CTLR);
	writel(0xff, gicc + GICC_PMR);
}

static inline void mon_text mon_init_gic(void)
{
	mon_init_gicd();
	mon_init_gicc();
}

static inline void mon_text mon_disable_gic(void)
{
	u32 gicc = mon_gicc_addr();
	writel(0, gicc + GICC_CTLR);
}

#endif

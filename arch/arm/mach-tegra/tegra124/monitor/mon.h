/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _MON_H_
#define _MON_H_

#define DEBUG 0 // 0 or 1
#define DEBUG_HIGH_VOLUME 0 // 0 or 1

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define mon_noreturn	__attribute__ ((noreturn))

#define L1_CACHE_SHIFT			6

#define ACTLR_SMP			BIT(6)
#define ACTLR_EN_INV_BTB		BIT(0)

#define SCTLR_TE			BIT(30)
#define SCTLR_NMFI			BIT(27)
#define SCTLR_EE			BIT(25)
#define SCTLR_V				BIT(13)
#define SCTLR_I				BIT(12)
#define SCTLR_Z				BIT(11)
#define SCTLR_C				BIT(2)
#define SCTLR_A				BIT(1)
#define SCTLR_M				BIT(0)

#define SCR_SIF				BIT(9)
#define SCR_HCE				BIT(8)
#define SCR_AW				BIT(5)
#define SCR_FW				BIT(4)
#define SCR_NS				BIT(0)

#define TEGRA_SB_CSR			0x6000c200
#define TEGRA_SB_CSR_NS_RST_VEC_WR_DIS	BIT(1)

#define TEGRA_RESET_EXCEPTION_VECTOR	0x6000f100

#define TEGRA_PMC_BASE			0x7000E400

#define TEGRA_CLK_RESET_BASE		0x60006000

#define TEGRA_FLOW_CTLR_BASE		0x60007000

#define TEGRA_TMRUS_BASE		0x60005010

#define TEGRA_EMC_BASE			0x7001B000

#define TEGRA_IRAM_BASE			0x40000000

#define MON_LP0_RESUME_IRAM_ADDR	NV_WB_RUN_ADDRESS // 0x40020000
#define MON_LP0_RESUME_MAX_SIZE		0x1000

#define MON_LP0_LP1_ENTRY_IRAM_ADDR	0x40010000
#define MON_LP0_LP1_ENTRY_MAX_SIZE	0x1000

#ifndef __ASSEMBLY__
#include "mon_section.h"
#include "mon_cp15.h"
#include "mon_gic.h"

/* mon_cache.S */
extern void mon_disable_dcache_clean_l1(void);

/* mon_dbg.c */
extern void mon_putc(char c);
extern void mon_puts(const char *s);
extern void mon_puthex(u32 val, bool zfill);
extern void mon_put_cpuid(void);

/* mon_entry.c */
extern void mon_entry_initial(void);
extern void mon_entry_cpu_on(void);
extern void mon_entry_lp2_resume(void);
extern void mon_entry_unexpected(void);

/* mon_lib.c */
void mon_delay_usecs(u32 n);
void mon_noreturn mon_error(void);

/* mon_psci.c */
extern void mon_init_psci_soc(void);
extern void mon_smc_psci_notify_booted(void);
extern u32 mon_smc_psci(u32 func, u32 arg0, u32 arg1, u32 arg2);

/* mon_sip.c */
extern u32 mon_smc_sip(u32 func, u32 arg0, u32 arg1, u32 arg2, u32 *ns_regs);

/* mon_sleep.S */
extern void mon_cpu_shutdown(u32 for_hotplug);

/* mon_smc.c */
void mon_smc(u32 func, u32 arg0, u32 arg1, u32 arg2, u32 *ns_regs);

/* mon_vecs.S */
extern u32 mon_entry_handlers[];
extern u32 mon_ns_entry_points[];
extern u32 mon_context_ids[];
extern void mon_vectors(void);
extern void mon_entry(void);
extern void mon_reentry(void);
#endif
#endif

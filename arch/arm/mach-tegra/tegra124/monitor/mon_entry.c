/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Derived from code:
 * Copyright (C) 2013,2014 - ARM Ltd
 * Copyright (C) 2015, Siemens AG
 * Copyright (c) 2013 Andre Przywara <andre.przywara//linaro.org>
 */

#include <common.h>
#include <asm/arch/clock.h>
#include <asm/arch/mc.h>
#include <asm/arch/sysctr.h>
#include <asm/arch-tegra/clk_rst.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/io.h>
#include "mon.h"

static u32 mon_text mon_get_osc_freq(void)
{
	struct clk_rst_ctlr *clkrst =
			(struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	u32 val, osc_freq;

	val = readl(&clkrst->crc_osc_ctrl);
	osc_freq = (val & OSC_FREQ_MASK) >> OSC_FREQ_SHIFT;
	switch (osc_freq) {
	case 0:
		return 13000000;
	case 1:
		return 16800000;
	case 4:
		return 19200000;
	case 5:
		return 38400000;
	case 8:
		return 12000000;
	case 9:
		return 48000000;
	case 12:
		return 26000000;
	default:
		mon_error();
		break;
	}
}

static void mon_text mon_init_secure_ram(void)
{
	struct mc_ctlr *mc = (struct mc_ctlr *)NV_PA_MC_BASE;

	/* Must be MB aligned */
	BUILD_BUG_ON(CONFIG_ARMV7_SECURE_BASE & 0xFFFFF);
	BUILD_BUG_ON(CONFIG_ARMV7_SECURE_RESERVE_SIZE & 0xFFFFF);

	writel(CONFIG_ARMV7_SECURE_BASE, &mc->mc_security_cfg0);
	writel(CONFIG_ARMV7_SECURE_RESERVE_SIZE >> 20, &mc->mc_security_cfg1);
}

static void mon_text mon_init_evp(void)
{
	u32 val;

	// Set the SoC CCPLEX reset entry point
	writel((u32)&mon_reentry, TEGRA_RESET_EXCEPTION_VECTOR);

	// Lock reset vector for non-secure
	val = readl(TEGRA_SB_CSR);
	val |= TEGRA_SB_CSR_NS_RST_VEC_WR_DIS;
	writel(val, TEGRA_SB_CSR);
}

static void mon_text mon_init_sysctr_ctlr(void)
{
	struct sysctr_ctlr *sysctr = (struct sysctr_ctlr *)NV_PA_TSC_BASE;
	u32 val;

	writel(mon_get_osc_freq(), &sysctr->cntfid0);

	val = readl(&sysctr->cntcr);
	val |= TSC_CNTCR_ENABLE | TSC_CNTCR_HDBG;
	writel(val, &sysctr->cntcr);
}

static void mon_text mon_init_smmu(void)
{
	struct mc_ctlr *mc = (struct mc_ctlr *)NV_PA_MC_BASE;
	u32 val;

	// Enable translation for all clients. The kernel will use the per-
	// SWGROUP enable bits to enable or disable translations.
	writel(0xffffffff, &mc->mc_smmu_translation_enable_0);
	writel(0xffffffff, &mc->mc_smmu_translation_enable_1);
	writel(0xffffffff, &mc->mc_smmu_translation_enable_2);
	writel(0xffffffff, &mc->mc_smmu_translation_enable_3);

	// Global SMMU enable
	val = readl(&mc->mc_smmu_config);
	val |= TEGRA_MC_SMMU_CONFIG_ENABLE;
	writel(val, &mc->mc_smmu_config);

	// Flush SMMU
	readl(&mc->mc_smmu_config);
}

static void mon_text mon_init_vpr(void)
{
	struct mc_ctlr *mc = (struct mc_ctlr *)NV_PA_MC_BASE;

	// Turn VPR off
	writel(0, &mc->mc_video_protect_size_mb);
	writel(TEGRA_MC_VIDEO_PROTECT_REG_WRITE_ACCESS_DISABLED,
	       &mc->mc_video_protect_reg_ctrl);
	// read back to ensure the write went through
	readl(&mc->mc_video_protect_reg_ctrl);
}

static void mon_text mon_init_soc(void)
{
	mon_init_psci_soc();
	mon_init_secure_ram();
	mon_init_evp();
	mon_init_sysctr_ctlr();
	mon_init_smmu();
	mon_init_vpr();
}

static void mon_text mon_init_actlr(void)
{
	u32 actlr = mon_read_actlr();
	actlr |= ACTLR_EN_INV_BTB;
	mon_write_actlr(actlr);
}

static void mon_text mon_init_arch_timer(void)
{
	mon_write_cntfrq(mon_get_osc_freq());
	mon_clear_cntvoff();
}

static void mon_text mon_init_copros(void)
{
	u32 nsacr = mon_read_nsacr();
	nsacr |= BIT(18) | 0x3fff;
	mon_write_nsacr(nsacr);
}

static void mon_text mon_init_ns_sctlr(void)
{
	u32 sctlr_ns = 0; // Lazy, but works
	mon_write_sctlr(sctlr_ns);
}

static void mon_text mon_switch_to_ns_banked(void)
{
	u32 scr = mon_read_scr();
	scr |= SCR_NS;
	mon_write_scr(scr);
}

static void mon_text mon_switch_to_s_banked(void)
{
	u32 scr = mon_read_scr();
	scr &= ~SCR_NS;
	mon_write_scr(scr);
}

static void mon_text mon_init_cpu_for_ns(void)
{
	mon_init_actlr();
	mon_switch_to_ns_banked();
	mon_init_arch_timer();
	mon_init_gic();
	mon_init_copros();
	mon_init_ns_sctlr();
	mon_switch_to_s_banked();
}

/* Called by mon_reentry() */
void mon_text mon_init_l2(void)
{
	u32 l2pfr, midr, variant, actlr2, mpidr, cluster;
	u32 l2ctlr, data_latency, l2actlr;

	// L2PFR: Disable throttling
	l2pfr = mon_read_l2pfr();
	if (!(l2pfr & BIT(13))) {
		l2pfr |= BIT(13);
		mon_write_l2pfr(l2pfr);
	}

	// ACTLR2: Enable regional clock gates
	midr = mon_read_midr();
	variant = (midr >> 20) & 0xf;
	if (variant >= 3) {
		actlr2 = mon_read_actlr2();
		actlr2 |= BIT(31);
		mon_write_actlr2(actlr2);
	}

	// L2CTLR: Fix data RAM latency
	// L2ACTLR: Set hazard detection timeout
	mpidr = mon_read_mpidr();
	cluster = (mpidr >> 8) & 0xf;
	if (cluster == 0) {
		l2ctlr = mon_read_l2ctlr();
		data_latency = l2ctlr & 7;
		if (data_latency != 2) {
			l2ctlr &= ~7;
			l2ctlr |= 2;
			mon_write_l2ctlr(l2ctlr);
			l2actlr = mon_read_l2actlr();
			l2actlr |= BIT(7);
			mon_write_l2actlr(l2actlr);
		}
	}
}

void mon_text mon_init_clear_pmc_scratch(void)
{
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	int i;

	/* SCRATCH0 is initialized by the boot ROM and shouldn't be cleared */
	for (i = 1; i < 24; i++)
		writel(0, &pmc->pmc_scratch0 + i);
}

void mon_text mon_entry_initial(void)
{
	mon_puts(MON_STR("MON: First boot on CPU "));
	mon_put_cpuid();
	mon_putc('\n');

	mon_init_clear_pmc_scratch();
	mon_init_soc();
	mon_init_cpu_for_ns();
}

void mon_text mon_entry_cpu_on(void)
{
	mon_puts(MON_STR("MON: CPU_ON on CPU "));
	mon_put_cpuid();
	mon_putc('\n');

	mon_init_cpu_for_ns();
}

void mon_text mon_entry_lp2_resume(void)
{
#if DEBUG_HIGH_VOLUME
	mon_puts(MON_STR("MON: LP2 resume CPU "));
	mon_put_cpuid();
	mon_putc('\n');
#endif

	mon_init_cpu_for_ns();
}

void mon_text mon_entry_unexpected(void)
{
	mon_puts(MON_STR("MON: Unexpected entry on CPU "));
	mon_put_cpuid();
	mon_putc('\n');

	mon_error();
}

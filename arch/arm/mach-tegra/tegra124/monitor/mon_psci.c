/*
 * Copyright (c) 2010-2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Derived from code:
 * Copyright (C) 2010-2015, NVIDIA Corporation. All rights reserved.
 * Copyright (C) 2002-2014 ARM Ltd.
 * Copyright (c) 2011, Google, Inc.
 * Copyright (c) 2013 Andre Przywara <andre.przywara//linaro.org>
 * Copyright (C) 2015, Siemens AG
 */

#include <common.h>
#include <asm/arch-tegra/clk_rst.h>
#include <asm/arch/flow.h>
#include <asm/io.h>
#include <asm/psci.h>

#include "mon.h"
#include <asm/arch-tegra/warmboot.h>

#define TEGRA_PMC_CNTRL				0x0
#define TEGRA_PMC_CNTRL_MAIN_RST		BIT(4)

#define TEGRA_PMC_PWRGATE_TOGGLE		0x30
#define TEGRA_PMC_PWRGATE_TOGGLE_START		0x100

#define TEGRA_POWERGATE_CPU0			14
#define TEGRA_POWERGATE_CPU1			9
#define TEGRA_CPU_POWERGATE_ID(cpu) \
	((cpu == 0) ? TEGRA_POWERGATE_CPU0 : (TEGRA_POWERGATE_CPU1 + cpu - 1))

static u32 mon_data mon_psci_state[4] = {
	PSCI_AFFINITY_LEVEL_ON,
	PSCI_AFFINITY_LEVEL_OFF,
	PSCI_AFFINITY_LEVEL_OFF,
	PSCI_AFFINITY_LEVEL_OFF,
};

static bool mon_data mon_unpowergated[4] = {
	false,
	false,
	false,
	false,
};

static u32 mon_text mon_pmc_readl(u32 reg)
{
	reg += NV_PA_PMC_BASE;
	return readl(reg);
}

static void mon_text mon_pmc_writel(u32 val, u32 reg)
{
	reg += NV_PA_PMC_BASE;
	writel(val, reg);
}

static u32 * mon_text mon_flow_csr_addr(u32 cpu)
{
	struct flow_ctlr *flow = (struct flow_ctlr *)NV_PA_FLOW_BASE;

	switch (cpu) {
	case 0:
		return &(flow->cpu_csr);
	case 1:
		return &(flow->cpu1_csr);
	case 2:
		return &(flow->cpu2_csr);
	case 3:
		return &(flow->cpu3_csr);
	default:
		mon_error();
	}
}

static u32 * mon_text mon_flow_halt_events_addr(u32 cpu)
{
	struct flow_ctlr *flow = (struct flow_ctlr *)NV_PA_FLOW_BASE;

	switch (cpu) {
	case 0:
		return &(flow->halt_cpu_events);
	case 1:
		return &(flow->halt_cpu1_events);
	case 2:
		return &(flow->halt_cpu2_events);
	case 3:
		return &(flow->halt_cpu3_events);
	default:
		mon_error();
	}
}

static void mon_text mon_disable_smp(void)
{
	u32 actlr = mon_read_actlr();
	actlr &= ~ACTLR_SMP;
	mon_write_actlr(actlr);
	isb();
	/*
	 * Issue a Dummy DVM op to make subsequent DSB issue a DVM_SYNC in A15.
	 * This is for a bug where DSB-lite(with no DVM_SYNC component)
	 * doesn't trigger the logic required to drain all other DSBs.
	 */
	mon_dummy_dvm_op();
	dsb();
}

static void mon_text mon_check_cur_cpu_is_on(void)
{
	u32 cur_cpu_id = mon_read_mpidr() & 3;

	if (mon_psci_state[cur_cpu_id] != PSCI_AFFINITY_LEVEL_ON) {
		mon_puts(MON_STR("MON: ERR: Bad CPU state\n"));
		mon_error();
	}
}

static u32 mon_text mon_noreturn mon_smc_psci_cpu_off(void)
{
	u32 cur_cpu_id = mon_read_mpidr() & 3;

	mon_puts(MON_STR("MON: PSCI: CPU_OFF for "));
	mon_put_cpuid();
	mon_putc('\n');

	mon_check_cur_cpu_is_on();

	mon_disable_gic();
	mon_disable_dcache_clean_l1();
	mon_disable_smp();
	mon_psci_state[cur_cpu_id] = PSCI_AFFINITY_LEVEL_OFF;
	mon_cpu_shutdown(1);

	/* This should not be reached */
	mon_puts(MON_STR("MON: ERR: CPU did not power down!\n"));
	mon_error();
}

static u32 mon_text mon_smc_psci_cpu_on(u32 target_cpu,
	u32 entry_point_address, u32 context_id)
{
	u32 *csr, iter, val;

	mon_puts(MON_STR("MON: PSCI: CPU_ON for 0x"));
	mon_puthex(target_cpu, false);
	mon_putc('\n');

	mon_check_cur_cpu_is_on();

	if (target_cpu >= 4) {
		mon_puts(MON_STR("MON: ERR: Bad CPU ID\n"));
		return ARM_PSCI_RET_INVAL;
	}

	switch (mon_psci_state[target_cpu]) {
	case PSCI_AFFINITY_LEVEL_ON:
		mon_puts(MON_STR("MON: ERR: CPU already on\n"));
		return ARM_PSCI_RET_ALREADY_ON;
	case PSCI_AFFINITY_LEVEL_OFF:
		break;
	case PSCI_AFFINITY_LEVEL_ON_PENDING:
		mon_puts(MON_STR("MON: ERR: CPU already on pending\n"));
		return ARM_PSCI_RET_ON_PENDING;
	default:
		mon_puts(MON_STR("MON: ERR: Bad CPU state\n"));
		mon_error();
	}

	if (mon_unpowergated[target_cpu]) {
		// Wait for any outstanding power off to complete
		csr = mon_flow_csr_addr(target_cpu);
		for (iter = 0; iter < 100000; iter++) {
			if (readl(csr) & CSR_PWR_OFF_STS)
				break;
		}
		if (!(readl(csr) & CSR_PWR_OFF_STS)) {
			mon_puts(MON_STR("MON: ERR: Previous CPU_OFF did not complete\n"));
			mon_error();
		}
	}

	mon_entry_handlers[target_cpu] = (u32)&mon_entry_cpu_on;
	mon_ns_entry_points[target_cpu] = entry_point_address;
	mon_context_ids[target_cpu] = context_id;
	mon_psci_state[target_cpu] = PSCI_AFFINITY_LEVEL_ON_PENDING;
	dsb();

	if (!mon_unpowergated[target_cpu]) {
		mon_unpowergated[target_cpu] = true;
		val = TEGRA_PMC_PWRGATE_TOGGLE_START |
			TEGRA_CPU_POWERGATE_ID(target_cpu);
		mon_pmc_writel(val, TEGRA_PMC_PWRGATE_TOGGLE);
		mon_pmc_readl(TEGRA_PMC_PWRGATE_TOGGLE);
	} else {
		u32 *csr = mon_flow_csr_addr(target_cpu);
		u32 *halt_events = mon_flow_halt_events_addr(target_cpu);

		writel(1, csr);
		writel(0x48000000, halt_events);
	}

	return 0;
}

static u32 mon_text mon_smc_psci_affinity_info(u32 target_affinity,
	u32 lowest_affinity_level)
{
	u32 info;

#if DEBUG_HIGH_VOLUME
	mon_puts(MON_STR("MON: PSCI: AFFINITY_INFO for "));
	mon_puthex(target_affinity, false);
	mon_puts(MON_STR(", "));
	mon_puthex(lowest_affinity_level, false);
	mon_putc('\n');
#endif

	mon_check_cur_cpu_is_on();

	// This is technically not compliant with PSCI v0.2, but is with 1.0.
	// In practice, the kernel only uses this affinity_level value.
	if (lowest_affinity_level != 0)
		return ARM_PSCI_RET_INVAL;

	if (target_affinity >= 4)
		return ARM_PSCI_RET_INVAL;

	info = mon_psci_state[target_affinity];
	if (info == PSCI_AFFINITY_LEVEL_OFF) {
		u32 *csr = mon_flow_csr_addr(target_affinity);
		if (!(readl(csr) & CSR_PWR_OFF_STS))
			info = PSCI_AFFINITY_LEVEL_ON;
	}

	return info;
}

static void mon_text mon_noreturn mon_smc_psci_system_reset(void)
{
	mon_puts(MON_STR("MON: PSCI: System reset\n"));

	mon_check_cur_cpu_is_on();

	u32 val = mon_pmc_readl(TEGRA_PMC_CNTRL);
	val |= TEGRA_PMC_CNTRL_MAIN_RST;
	mon_pmc_writel(val, TEGRA_PMC_CNTRL);

	/* Shouldn't get here */
	mon_puts(MON_STR("MON: ERR: System reset failed!\n"));
	mon_error();
}

void mon_text mon_init_psci_soc(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(mon_unpowergated); i++)
		mon_unpowergated[i] = false;
}

u32 mon_text mon_smc_psci(u32 func, u32 arg0, u32 arg1, u32 arg2)
{
	switch (func) {
	case ARM_PSCI_0_2_FN_PSCI_VERSION:
		return 2;
	case ARM_PSCI_0_2_FN_CPU_OFF:
		return mon_smc_psci_cpu_off();
	case ARM_PSCI_0_2_FN_CPU_ON:
		return mon_smc_psci_cpu_on(arg0, arg1, arg2);
	case ARM_PSCI_0_2_FN_AFFINITY_INFO:
		return mon_smc_psci_affinity_info(arg0, arg1);
	case ARM_PSCI_0_2_FN_SYSTEM_RESET:
		mon_smc_psci_system_reset();
	default:
		mon_puts(MON_STR("MON: ERR: Unknown PSCI SMC 0x"));
		mon_puthex(func, true);
		mon_putc('\n');
		return ARM_PSCI_RET_NI;
	}
}

void mon_text mon_smc_psci_notify_booted(void)
{
	mon_psci_state[mon_read_mpidr() & 3] = PSCI_AFFINITY_LEVEL_ON;
}

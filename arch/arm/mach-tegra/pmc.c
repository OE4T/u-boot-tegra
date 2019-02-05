/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier:     GPL-2.0
 */

#include <common.h>
#include <asm/io.h>
#include <fdtdec.h>
#include <asm/arch-tegra/cboot.h>
#include <asm/arch-tegra/pmc.h>

#ifdef CONFIG_ACCESS_PMC_VIA_SMC
#ifndef CONFIG_ARM64
#error CONFIG_ACCESS_PMC_VIA_SMC only supported with CONFIG_ARM64
#endif
#endif

DECLARE_GLOBAL_DATA_PTR;

#define NR_SMC_REGS 6

#define PMC_READ		0xaa
#define PMC_WRITE		0xbb
#define TEGRA_SIP_PMC_COMMAND_FID 0xC2FFFE00

struct pmc_smc_regs {
	u64 args[NR_SMC_REGS];
};

#if defined(CONFIG_ACCESS_PMC_VIA_SMC)
static void send_smc(u32 func, struct pmc_smc_regs *regs)
{
	u32 ret = func;
	asm volatile(
		"mov x0, %0\n"
		"ldp x1, x2, [%1, #16 * 0]\n"
		"ldp x3, x4, [%1, #16 * 1]\n"
		"ldp x5, x6, [%1, #16 * 2]\n"
		"smc #0\n"
		"mov %0, x0\n"
		"stp x1, x2, [%1, #16 * 0]\n"
		: "+r" (ret)
		: "r" (regs)
		: "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8",
		  "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17");
	if (ret)
		printf("%s: failed (ret=%d)\n", __func__, ret);
}
#endif

#ifdef CONFIG_ACCESS_PMC_VIA_SMC
static bool get_secure_pmc_setting(void)
{
	int node_offset;
	const void *fdt;
	static bool secure_pmc;
	static bool initialized;

	if (!initialized) {
		initialized = true;
		fdt = (void *)cboot_boot_x0;
		node_offset = fdt_node_offset_by_compatible(fdt,
			      -1, "nvidia,tegra210-pmc");
		if (node_offset < 0) {
			printf("%s: compatible node not found\n", __func__);
			secure_pmc = false;
			return secure_pmc;
		}
		if (!fdt_getprop(fdt, node_offset, "nvidia,secure-pmc", NULL))
			secure_pmc = false;
		else
			secure_pmc = true;
	}
	return secure_pmc;
}
#endif

uint32_t tegra_pmc_readl(unsigned long offset)
{
#ifdef CONFIG_ACCESS_PMC_VIA_SMC
	struct pmc_smc_regs regs;

	if (get_secure_pmc_setting()) {
		regs.args[0] = PMC_READ;
		regs.args[1] = offset;
		regs.args[2] = 0;
		regs.args[3] = 0;
		regs.args[4] = 0;
		regs.args[5] = 0;
		send_smc(TEGRA_SIP_PMC_COMMAND_FID, &regs);
		return (u32)regs.args[0];
	}
#endif
	return readl(NV_PA_PMC_BASE + offset);
}

void tegra_pmc_writel(u32 value, unsigned long offset)
{
#ifdef CONFIG_ACCESS_PMC_VIA_SMC
	struct pmc_smc_regs regs;

	if (get_secure_pmc_setting()) {
		regs.args[0] = PMC_WRITE;
		regs.args[1] = offset;
		regs.args[2] = value;
		regs.args[3] = 0;
		regs.args[4] = 0;
		regs.args[5] = 0;
		send_smc(TEGRA_SIP_PMC_COMMAND_FID, &regs);
	}
#endif
	writel(value, NV_PA_PMC_BASE + offset);
}

void reset_cpu(ulong addr)
{
	u32 val;

	val = tegra_pmc_readl(offsetof(struct pmc_ctlr, pmc_cntrl));
	val |= (1 << MAIN_RST_BIT);
	tegra_pmc_writel(val, offsetof(struct pmc_ctlr, pmc_cntrl));
}

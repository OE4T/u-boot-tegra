// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch-tegra/pmc.h>

#if defined(CONFIG_TARGET_L4T_MONITOR) && !defined(CONFIG_SPL_BUILD)
#define NR_SMC_REGS			6
#define PMC_READ			0xaa
#define PMC_WRITE			0xbb
#define TEGRA_SIP_PMC_COMMAND_FID	0x82fffe00

struct pmc_smc_regs {
	u32 args[NR_SMC_REGS];
};

static void send_smc(u32 func, struct pmc_smc_regs *regs)
{
	u32 ret = func;
	asm volatile(
		"mov r0, %0\n"
		"ldr r1, [%1, #4 * 0]\n"
		"ldr r2, [%1, #4 * 1]\n"
		"ldr r3, [%1, #4 * 2]\n"
		"ldr r4, [%1, #4 * 3]\n"
		"ldr r5, [%1, #4 * 4]\n"
		"ldr r6, [%1, #4 * 5]\n"
		"smc #0\n"
		"mov %0, r0\n"
		"str r1, [%1, #4 * 0]\n"
		"str r2, [%1, #4 * 1]\n"
		: "+r" (ret)
		: "r" (regs)
		: "r0", "r1", "r2", "r3", "r4", "r5", "r6");
	if (ret)
		printf("%s: failed (ret=%d)\n", __func__, ret);
}

u32 tegra_pmc_readl(unsigned long offset)
{
	struct pmc_smc_regs regs;

	regs.args[0] = PMC_READ;
	regs.args[1] = offset;
	regs.args[2] = 0;
	regs.args[3] = 0;
	regs.args[4] = 0;
	regs.args[5] = 0;
	send_smc(TEGRA_SIP_PMC_COMMAND_FID, &regs);
	return (u32)regs.args[0];
}

void tegra_pmc_writel(u32 value, unsigned long offset)
{
	struct pmc_smc_regs regs;

	regs.args[0] = PMC_WRITE;
	regs.args[1] = offset;
	regs.args[2] = value;
	regs.args[3] = 0;
	regs.args[4] = 0;
	regs.args[5] = 0;
	send_smc(TEGRA_SIP_PMC_COMMAND_FID, &regs);
}
#else
u32 tegra_pmc_readl(unsigned long offset)
{
	return readl(NV_PA_PMC_BASE + offset);
}

void tegra_pmc_writel(u32 value, unsigned long offset)
{
	writel(value, NV_PA_PMC_BASE + offset);
}
#endif

void reset_cpu(ulong addr)
{
	u32 val;

	val = tegra_pmc_readl(offsetof(struct pmc_ctlr, pmc_cntrl));
	val |= (1 << MAIN_RST_BIT);
	tegra_pmc_writel(val, offsetof(struct pmc_ctlr, pmc_cntrl));
}

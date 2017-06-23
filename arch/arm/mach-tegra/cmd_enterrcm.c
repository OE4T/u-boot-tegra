/*
 * Copyright (c) 2012,2017 NVIDIA CORPORATION.  All rights reserved.
 *
 * Derived from code (arch/arm/lib/reset.c) that is:
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
 *
 * (C) Copyright 2004
 * DAVE Srl
 * http://www.dave-tech.it
 * http://www.wawnet.biz
 * mailto:info@wawnet.biz
 *
 * (C) Copyright 2004 Texas Insturments
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/tegra.h>
#include <asm/arch-tegra/pmc.h>

static int do_enterrcm(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	u32 *pmc_scratch0;

	puts("Entering RCM...\n");
	udelay(50000);

#if defined(CONFIG_TEGRA186)
	/*
	 * T186 has moved the scratch regs. Address the reg directly here
	 * rather than creating a new pmc reg struct for just one register.
	 */
	pmc_scratch0 = (u32 *)(NV_PA_PMC_BASE + 0x32000);
#else
	struct pmc_ctlr *pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	pmc_scratch0 = &pmc->pmc_scratch0;
#endif
	writel(2, pmc_scratch0);	/* SCRATCH0, bit 1 */
	disable_interrupts();
	reset_cpu(0);

	return 0;
}

U_BOOT_CMD(
	enterrcm, 1, 0, do_enterrcm,
	"reset Tegra and enter USB Recovery Mode",
	""
);

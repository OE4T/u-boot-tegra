/*
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch/gp_padctrl.h>
#include <asm/arch/clock.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/arch-tegra/warmboot.h>
#include <asm/arch-tegra/sdram_param.h>

int warmboot_save_sdram_params(void)
{
	u32 ram_code;
	struct sdram_params sdram;
	struct pmc_ctlr *pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;

	/* get ram code that is used as index to array _params in BCT */
	ram_code = (readl(&pmc->pmc_strap_opt_a) & STRAP_OPT_A_RAM_CODE_MASK)
		   >> STRAP_OPT_A_RAM_CODE_SHIFT;
	/* ram_code[1:0] selects SDRAM configuration set within the BCT */
	ram_code &= 3;

	memcpy(&sdram,
	       (char *)((struct sdram_params *)SDRAM_PARAMS_BASE + ram_code),
	       sizeof(sdram));

	return t1x4_wb_save_sdram_params(&sdram);
}

int warmboot_prepare_code(u32 seg_address, u32 seg_length)
{
	return t1x4_wb_prepare_code(CHIPID_TEGRA124, seg_address, seg_length);
}

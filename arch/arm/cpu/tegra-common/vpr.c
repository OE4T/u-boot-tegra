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

/* Tegra vpr routines */

#include <common.h>
#include <asm/io.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch-tegra/mc.h>
#include <asm/arch/gp_padctrl.h>

/* Configures VPR.  Right now, all we do is turn it off. */
void config_vpr(void)
{
	struct apb_misc_gp_ctlr *gp =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;
	struct mc_ctlr *mc = (struct mc_ctlr *)NV_PA_MC_BASE;
	u32 reg = 0;

	/* VPR is only in T114 and T124 */
	reg = (readl(&gp->hidrev) & HIDREV_CHIPID_MASK);
	reg = (reg >> HIDREV_CHIPID_SHIFT) & 0xFF;
	if ((reg == CHIPID_TEGRA114) || (reg == CHIPID_TEGRA124)) {
		/* Turn off VPR */
		writel(0x00000000, &mc->mc_video_protect_size_mb);
		writel(0x00000001, &mc->mc_video_protect_reg_ctrl);
	}
}

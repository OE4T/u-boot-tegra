/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* Tegra2 power control functions */

#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/clock.h>
#include <asm/arch/pmc.h>
#include <asm/arch/power.h>
#include <common.h>

void power_enable_partition(enum power_partition_id id)
{
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	enum power_partition_id clamp_id = id;

	/* The order of these operations is important */
	reset_set_enable(id, 1);

	/* Turn the power gate on if it's not on already */
	if (!(readl(&pmc->pmc_pwrgate_status) & (1 << id)))
		writel(PWRGATE_ENABLE | id, &pmc->pmc_pwrgate_toggle);

	clock_enable(id);
	udelay(10);

	/* The SOC switches these IDs for clamping */
	if (clamp_id == POWERP_PCI)
		clamp_id = POWERP_VIDEO_DEC;
	else if (clamp_id == POWERP_VIDEO_DEC)
		clamp_id = POWERP_PCI;
	writel(1 << clamp_id, &pmc->pmc_remove_clamping);

	reset_set_enable(id, 0);
}

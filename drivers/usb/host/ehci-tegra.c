/*
 * Copyright (c) 2009 NVIDIA Corporation
 *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <usb.h>

#include "ehci.h"
#include "ehci-core.h"

#include <asm/errno.h>
#include <asm/arch/usb.h>


/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(void)
{
	/* EHCI registers start at offset 0x100. For now support only port 0*/
	hccr = (struct ehci_hccr *)(CONFIG_TEGRA2_USB0 + 0x100);
	hcor = (struct ehci_hcor *)((uint32_t) hccr
		+ HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int ehci_hcd_stop(void)
{
	usb_set_host_mode();
	ehci_writel(&hcor->or_usbcmd, 0);
	udelay(1000);
	ehci_writel(&hcor->or_usbcmd, 2);
	udelay(1000);
	return 0;
}

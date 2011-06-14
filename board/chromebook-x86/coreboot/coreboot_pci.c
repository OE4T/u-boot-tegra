/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * (C) Copyright 2008,2009
 * Graeme Russ, <graeme.russ@gmail.com>
 *
 * (C) Copyright 2002
 * Daniel Engström, Omicron Ceti AB, <daniel@omicron.se>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <pci.h>
#include <asm/pci.h>

static struct pci_controller coreboot_hose;

#define X86_PCI_CONFIG_ADDR 0xCF8
#define X86_PCI_CONFIG_DATA 0xCFC

static void config_pci_bridge(struct pci_controller *hose, pci_dev_t dev,
                              struct pci_config_table *table)
{
	u8 secondary;
	hose->read_byte(hose, dev, PCI_SECONDARY_BUS, &secondary);
	hose->last_busno = max(hose->last_busno, secondary);
	pci_hose_scan_bus(hose, secondary);
}

static struct pci_config_table pci_coreboot_config_table[] = {
	/* vendor, device, class, bus, dev, func */
	{ PCI_ANY_ID, PCI_ANY_ID, PCI_CLASS_BRIDGE_PCI,
	  PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, &config_pci_bridge},
	{}
};

void pci_init_board(void)
{
	coreboot_hose.config_table = pci_coreboot_config_table;
	coreboot_hose.first_busno = 0;
	coreboot_hose.last_busno = 0;
	coreboot_hose.region_count = 0;

	pci_setup_type1(&coreboot_hose, X86_PCI_CONFIG_ADDR,
		X86_PCI_CONFIG_DATA);

	pci_register_hose(&coreboot_hose);

	pci_hose_scan(&coreboot_hose);
}

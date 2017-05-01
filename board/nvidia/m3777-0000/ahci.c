/*
 * Copyright (c) 2017, NVIDIA CORPORATION
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <asm/io.h>
#include <ahci.h>

#define NV_PA_SATA_BASE	0x03500000
#define NV_PA_SATA_BAR5	0x03507000

#define WAIT_MS_LINKUP	200
#define msleep(a) udelay(a * 1000)

int ahci_link_up(struct ahci_probe_ent *probe_ent, u8 port)
{
	u32 tmp;
	int j = 0;
	void __iomem *port_mmio = probe_ent->port[port].port_mmio;

	/*
	 * Small delay needed here or I/F doesn't come
	 * up and we'll get timeouts in data_io.
	 */
	msleep(50);

	/*
	 * Bring up SATA link.
	 * SATA link bringup time is usually less than 1 ms; only very
	 * rarely has it taken between 1-2 ms. Never seen it above 2 ms.
	 */
	while (j < WAIT_MS_LINKUP) {
		tmp = readl(port_mmio + PORT_SCR_STAT);
		tmp &= PORT_SCR_STAT_DET_MASK;
		if (tmp == PORT_SCR_STAT_DET_PHYRDY)
			return 0;
		udelay(1000);
		j++;
	}
	return 1;
}

void scsi_init(void)
{
	debug("%s: calling AHCI_INIT w/base = 0x%08lX\n", __func__, (ulong)NV_PA_SATA_BAR5);
	ahci_init((void __iomem *)NV_PA_SATA_BAR5);
}

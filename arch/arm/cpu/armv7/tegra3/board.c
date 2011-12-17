/*
 *  (C) Copyright 2010,2011
 *  NVIDIA Corporation <www.nvidia.com>
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
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/tegra.h>
#include <asm/arch/pmc.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Boot ROM initializes the odmdata in APBDEV_PMC_SCRATCH20_0,
 * so we are using this value to identify memory size.
 */

unsigned int query_sdram_size(void)
{
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	u32 reg;

	debug("query_sdram_size entry\n");
	reg = readl(&pmc->pmc_scratch20);
	debug("\tpmc->pmc_scratch20 (ODMData) = 0x%08X\n", reg);

	/* bits 31:28 in OdmData are used for RAM size  */
	switch ((reg) >> 28) {
	case 1:
		return 0x10000000;	/* 256 MB */
	case 2:
		return 0x20000000;	/* 512 MB */
	case 4:
		return 0x40000000;	/* 1GB */
	case 8:
		return 0x7ff00000;	/* 2GB - 1MB */
	default:
		return 0x40000000;	/* 1GB */
	}
}

int dram_init(void)
{
	unsigned long rs;

	debug("dram_init entry\n");
	debug("\tgd = %lX\n", (ulong)gd);
	debug("\tgd->bd = %lX\n", (ulong)gd->bd);

	/* We do not initialise DRAM here. We just query the size */
	gd->ram_size = query_sdram_size();
	debug("\tdramsize = %lx\n", gd->ram_size);

	/*
	 * Since function get_ram_size() can only check with memory size
	 * given by a 32bit word with one bit set, for examples:
	 *	0x10000000	256MB
	 *	0x20000000	512MB
	 *	0x40000000	1GB
	 *	0x80000000	2GB
	 * For tegra3, its max size is (2GB-1MB), ie, 0x7ff00000.
	 * When multiple bits are set to 1, this function will return
	 * with incorrect size. so, we may just by-pass this function.
	 */
	/* Now check it dynamically */
	if (gd->ram_size != 0x7ff00000) {
		rs = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE, gd->ram_size);
		debug("\tdynamic dramsize (rs) = %lx\n", rs);
		if (rs)
			gd->ram_size = rs;
	}
	debug("dram_init exit\n");
	return 0;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	const char* board_name;
#ifdef CONFIG_OF_CONTROL
	board_name = get_board_name();
#else
	board_name = sysinfo.board_string;
#endif
	printf("Board: %s\n", board_name);
	return 0;
}
#endif	/* CONFIG_DISPLAY_BOARDINFO */

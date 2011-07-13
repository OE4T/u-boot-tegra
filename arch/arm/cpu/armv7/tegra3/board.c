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
#define DEBUG			//tcw
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
/* t30 bringup */
#if 0
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
	case 3:
	default:
		return 0x40000000;	/* 1GB */
	}
#else /* t30 bringup */
	return 0x40000000;
#endif
}

/* t30 bringup */
void PostCc(char c);
void NvBlPrintU32(unsigned int);
void uart_post(char c);

int dram_init(void)
{
	unsigned long rs;
	/* !!!! TODO: // jz
	     error: gd->bd has not been initialized yet. 
	*/

	if (!gd->bd)
		gd->bd = (bd_t *)0x40002000;	//TCW USE IRAM FOR EARLY INIT

	debug("dram_init entry\n");
	debug("\tgd = %lX\n", (ulong)gd);
	debug("\tgd->bd = %lX\n", (ulong)gd->bd);

	/* We do not initialise DRAM here. We just query the size */
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = gd->ram_size = query_sdram_size();
	debug("\tsdram size = %lX\n", gd->bd->bi_dram[0].size);

	/* Now check it dynamically */
	rs = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE, gd->ram_size);
	debug("\tdynamic dramsize (rs) = %lx\n", rs);
	if (rs)
		gd->bd->bi_dram[0].size = gd->ram_size = rs;
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

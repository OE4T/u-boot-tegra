/*
 * (C) Copyright 2010,2011
 * NVIDIA Corporation <www.nvidia.com>
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

#ifndef _TEGRA11X_H_
#define _TEGRA11X_H_

#define t11x_port 1

#define NV_PA_SDRAM_BASE	0x80000000
#define NV_PA_ARM_PERIPHBASE	0x50040000
#define NV_PA_PG_UP_BASE	0x60000000
#define NV_PA_TMRUS_BASE	0x60005010
#define NV_PA_CLK_RST_BASE	0x60006000
#define NV_PA_FLOW_BASE		0x60007000
#define NV_PA_AHB_BASE		0x6000C000
#define NV_PA_GPIO_BASE		0x6000D000
#define NV_PA_EVP_BASE		0x6000F000
#define NV_PA_APB_MISC_BASE	0x70000000
#define NV_PA_APB_MISC_GP_BASE	(NV_PA_APB_MISC_BASE + 0x0800)
#define NV_PA_APB_UARTA_BASE	(NV_PA_APB_MISC_BASE + 0x6000)
#define NV_PA_APB_UARTB_BASE	(NV_PA_APB_MISC_BASE + 0x6040)
#define NV_PA_APB_UARTC_BASE	(NV_PA_APB_MISC_BASE + 0x6200)
#define NV_PA_APB_UARTD_BASE	(NV_PA_APB_MISC_BASE + 0x6300)
#define NV_PA_APB_UARTE_BASE	(NV_PA_APB_MISC_BASE + 0x6400)
#define NV_PA_I2C1_BASE		0x7000C000
#define NV_PA_I2C2_BASE		0x7000C400
#define NV_PA_I2C3_BASE		0x7000C500
#define NV_PA_I2C4_BASE		0x7000C700
#define NV_PA_I2C5_BASE		0x7000D000
#define NV_PA_KBC_BASE		0x7000E200
#define NV_PA_PMC_BASE		0x7000E400
#define NV_PA_EMC_BASE		0x7000F400
#define NV_PA_FUSE_BASE		0x7000F800
#define NV_PA_CSITE_BASE	0x70040000
#define NV_PA_SDMMC1_BASE	0x78000000
#define NV_PA_SDMMC2_BASE	0x78000200
#define NV_PA_SDMMC3_BASE	0x78000400
#define NV_PA_SDMMC4_BASE	0x78000600
#define NV_PA_USB1_BASE		0x7D000000
#define NV_PA_USB2_BASE		0x7D004000
#define NV_PA_USB3_BASE		0x7D008000

#define TEGRA_SDRC_CS0		NV_PA_SDRAM_BASE
#define LOW_LEVEL_SRAM_STACK	0x4000FFFC
#define EARLY_AVP_STACK		(NV_PA_SDRAM_BASE + 0x20000)
#define EARLY_CPU_STACK		(EARLY_AVP_STACK - 4096)
#define PG_UP_TAG_AVP		0xAAAAAAAA

#ifndef __ASSEMBLY__
struct timerus {
	unsigned int cntr_1us;
};
#else  /* __ASSEMBLY__ */
#define PRM_RSTCTRL		NV_PA_PMC_BASE
#endif

#define SKU_ID_T20		0x8
#define SKU_ID_T25SE		0x14
#define SKU_ID_AP25		0x17
#define SKU_ID_T25		0x18
#define SKU_ID_AP25E		0x1b
#define SKU_ID_T25E		0x1c

#define TEGRA_SOC_UNKNOWN	(-1)
#define TEGRA_SOC_T20		(0)
#define TEGRA_SOC_T25		(1)
#define TEGRA_SOC_COUNT		(2)

/* Address at which WB code runs, it must not overlap Bootrom's IRAM usage */
#define T30_WB_RUN_ADDRESS	0x40020000

#define NVBOOTINFOTABLE_BCTSIZE	0x48	/* BCT size in BIT in IRAM */
#define NVBOOTINFOTABLE_BCTPTR	0x4C	/* BCT pointer in BIT in IRAM */
#define BCT_ODMDATA_OFFSET	1752

#define round_up(x, y) ((x + (y-1)) & (~(y-1)))
#define TEGRA_LINEAR_PITCH_ALIGNMENT	16

#endif	/* TEGRA11X_H */

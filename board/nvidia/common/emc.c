/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/tegra2.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/pmu.h>
#include <asm/arch/emc.h>
#include "board.h"

static const struct tegra_emc_table *tegra_emc_table;
static int tegra_emc_table_size;

static inline void emc_writel(u32 val, unsigned long addr)
{
	writel(val, (NV_PA_EMC_BASE + addr));
}

static inline u32 emc_readl(unsigned long addr)
{
	return readl((NV_PA_EMC_BASE + addr));
}

static const unsigned long emc_reg_addr[TEGRA_EMC_NUM_REGS] = {
	0x2c,	/* RC */
	0x30,	/* RFC */
	0x34,	/* RAS */
	0x38,	/* RP */
	0x3c,	/* R2W */
	0x40,	/* W2R */
	0x44,	/* R2P */
	0x48,	/* W2P */
	0x4c,	/* RD_RCD */
	0x50,	/* WR_RCD */
	0x54,	/* RRD */
	0x58,	/* REXT */
	0x5c,	/* WDV */
	0x60,	/* QUSE */
	0x64,	/* QRST */
	0x68,	/* QSAFE */
	0x6c,	/* RDV */
	0x70,	/* REFRESH */
	0x74,	/* BURST_REFRESH_NUM */
	0x78,	/* PDEX2WR */
	0x7c,	/* PDEX2RD */
	0x80,	/* PCHG2PDEN */
	0x84,	/* ACT2PDEN */
	0x88,	/* AR2PDEN */
	0x8c,	/* RW2PDEN */
	0x90,	/* TXSR */
	0x94,	/* TCKE */
	0x98,	/* TFAW */
	0x9c,	/* TRPAB */
	0xa0,	/* TCLKSTABLE */
	0xa4,	/* TCLKSTOP */
	0xa8,	/* TREFBW */
	0xac,	/* QUSE_EXTRA */
	0x114,	/* FBIO_CFG6 */
	0xb0,	/* ODT_WRITE */
	0xb4,	/* ODT_READ */
	0x104,	/* FBIO_CFG5 */
	0x2bc,	/* CFG_DIG_DLL */
	0x2c0,	/* DLL_XFORM_DQS */
	0x2c4,	/* DLL_XFORM_QUSE */
	0x2e0,	/* ZCAL_REF_CNT */
	0x2e4,	/* ZCAL_WAIT_CNT */
	0x2a8,	/* AUTO_CAL_INTERVAL */
	0x2d0,	/* CFG_CLKTRIM_0 */
	0x2d4,	/* CFG_CLKTRIM_1 */
	0x2d8,	/* CFG_CLKTRIM_2 */
};

/* The EMC registers have shadow registers.  When the EMC clock is updated
 * in the clock controller, the shadow registers are copied to the active
 * registers, allowing glitchless memory bus frequency changes.
 * This function updates the shadow registers for a new clock frequency,
 * and relies on the clock lock on the emc clock to avoid races between
 * multiple frequency changes */
#define EMC_SDRAM_RATE_T20	(333000*2*1000)
#define EMC_SDRAM_RATE_T25	(380000*2*1000)

static int tegra_emc_set_rate(unsigned long rate)
{
	int	i;
	int	j;

	if (!tegra_emc_table)
		return -1;

	/* The EMC clock rate is twice the bus rate, and the bus rate is
	 * measured in kHz */
	rate = rate / 2 / 1000;

	for (i = 0; i < tegra_emc_table_size; i++)
		if (tegra_emc_table[i].rate == rate)
			break;

	if (i >= tegra_emc_table_size)
		return -1;

	for (j = 0; j < TEGRA_EMC_NUM_REGS; j++)
		emc_writel(tegra_emc_table[i].regs[j], emc_reg_addr[j]);

	/*
	 * trigger emc with new settings by updating clk_rst's source EMC
	 * Set PLLM_OUT0 (bits: 31:30= 0) and divisor bits 7:0 = 0 (ie 1)
	 */
	clock_ll_set_source_divisor(PERIPH_ID_EMC, 0, 0);
	udelay(1);
	return 0;
}

static int tegra_set_emc(const struct tegra_emc_table *table, int table_size)
{
	unsigned long rate;

	if (!table) {
		tegra_emc_table = NULL;
		tegra_emc_table_size = 0;
		return -1;
	}

	tegra_emc_table = table;
	tegra_emc_table_size = table_size;

	switch (tegra_get_chip_type()) {
	case TEGRA_SOC_T20:
		rate  = EMC_SDRAM_RATE_T20;
		break;
	case TEGRA_SOC_T25:
		rate  = EMC_SDRAM_RATE_T25;
		break;
	default:
		/* unknown chip type, no clk change*/
		return -1;
	}

	tegra_emc_set_rate(rate);

	return 0;
}

struct tegra_board_emc_table {
	int				id; /* Boot strap ID */
	const struct tegra_emc_table	*table;
	const int			table_size;
	const char			*name;
};


static const struct tegra_emc_table seaboard_emc_tables_hynix_333Mhz[] = {
	{
		.rate = 166500,   /* SDRAM frequency */
		.regs = {
			0x0000000a,   /* RC */
			0x00000021,   /* RFC */
			0x00000008,   /* RAS */
			0x00000003,   /* RP */
			0x00000004,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x000004df,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000003,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000006,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x0000000f,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xa04004ae,   /* CFG_DIG_DLL */
			0x007fd010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}, {
		.rate = 333000,   /* SDRAM frequency */
		.regs = {
			0x00000014,   /* RC */
			0x00000041,   /* RFC */
			0x0000000f,   /* RAS */
			0x00000005,   /* RP */
			0x00000004,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000005,   /* RD_RCD */
			0x00000005,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x000009ff,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000005,   /* PCHG2PDEN */
			0x00000005,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000f,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000c,   /* TFAW */
			0x00000006,   /* TRPAB */
			0x0000000f,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xe034048b,   /* CFG_DIG_DLL */
			0x007e8010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

static const struct tegra_emc_table seaboard_emc_tables_hynix_380Mhz[] = {
	{
		.rate = 190000,   /* SDRAM frequency */
		.regs = {
			0x0000000c,   /* RC */
			0x00000026,   /* RFC */
			0x00000009,   /* RAS */
			0x00000003,   /* RP */
			0x00000004,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x0000059f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000003,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000b,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000007,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x0000000f,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xa06204ae,   /* CFG_DIG_DLL */
			0x007dc010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}, {
		.rate = 380000,   /* SDRAM frequency */
		.regs = {
			0x00000017,   /* RC */
			0x0000004b,   /* RFC */
			0x00000012,   /* RAS */
			0x00000006,   /* RP */
			0x00000004,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000006,   /* RD_RCD */
			0x00000006,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x00000b5f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000006,   /* PCHG2PDEN */
			0x00000006,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x00000011,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000e,   /* TFAW */
			0x00000007,   /* TRPAB */
			0x0000000f,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xe044048b,   /* CFG_DIG_DLL */
			0x007d8010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

void seaboard_emc_init(void)
{
	switch (tegra_get_chip_type()) {
	case TEGRA_SOC_T20:
		tegra_set_emc(seaboard_emc_tables_hynix_333Mhz,
			ARRAY_SIZE(seaboard_emc_tables_hynix_333Mhz));
		break;
	case TEGRA_SOC_T25:
		tegra_set_emc(seaboard_emc_tables_hynix_380Mhz,
			ARRAY_SIZE(seaboard_emc_tables_hynix_380Mhz));
		break;
	default:
		/* unknown chip type, no clk change*/
		tegra_set_emc(NULL, 0);
		break;
	}
}

static const struct tegra_emc_table kaen_emc_tables_Nanya_333Mhz[] = {
	{
		.rate = 166500,   /* SDRAM frequency */
		.regs = {
			0x0000000a,   /* RC */
			0x00000016,   /* RFC */
			0x00000008,   /* RAS */
			0x00000003,   /* RP */
			0x00000004,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x0000000a,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000001,   /* REXT */
			0x00000003,   /* WDV */
			0x00000004,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000c,   /* RDV */
			0x000004df,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000003,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x00000009,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000007,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xa06e04ae,   /* CFG_DIG_DLL */
			0x007e2010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}, {
		.rate = 333000,   /* SDRAM frequency */
		.regs = {
			0x00000014,   /* RC */
			0x0000002b,   /* RFC */
			0x0000000f,   /* RAS */
			0x00000005,   /* RP */
			0x00000004,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x0000000a,   /* W2P */
			0x00000005,   /* RD_RCD */
			0x00000005,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000001,   /* REXT */
			0x00000003,   /* WDV */
			0x00000004,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000c,   /* RDV */
			0x000009ff,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000005,   /* PCHG2PDEN */
			0x00000005,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000e,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000d,   /* TFAW */
			0x00000006,   /* TRPAB */
			0x00000006,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xe04e048b,   /* CFG_DIG_DLL */
			0x007e2010,   /* DLL_XFORM_DQS */
			0x007f8417,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

static const struct tegra_emc_table kaen_emc_tables_Nanya_380Mhz[] = {
	{
		.rate = 190000,   /* SDRAM frequency */
		.regs = {
			0x0000000b,   /* RC */
			0x00000019,   /* RFC */
			0x00000009,   /* RAS */
			0x00000003,   /* RP */
			0x00000004,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x0000000b,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000001,   /* REXT */
			0x00000003,   /* WDV */
			0x00000004,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000c,   /* RDV */
			0x0000059f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000003,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000007,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000008,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xa06204ae,   /* CFG_DIG_DLL */
			0x007fd010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}, {
		.rate = 380000,   /* SDRAM frequency */
		.regs = {
			0x00000016,   /* RC */
			0x00000031,   /* RFC */
			0x00000012,   /* RAS */
			0x00000006,   /* RP */
			0x00000004,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x0000000b,   /* W2P */
			0x00000005,   /* RD_RCD */
			0x00000005,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000001,   /* REXT */
			0x00000003,   /* WDV */
			0x00000004,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000c,   /* RDV */
			0x00000b5f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000006,   /* PCHG2PDEN */
			0x00000005,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x00000010,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000e,   /* TFAW */
			0x00000007,   /* TRPAB */
			0x00000008,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000004,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xe044048b,   /* CFG_DIG_DLL */
			0x007e4010,   /* DLL_XFORM_DQS */
			0x00016617,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

static const struct tegra_emc_table kaen_emc_tables_Samsung_333Mhz[] = {
	{
		.rate = 166500,   /* SDRAM frequency */
		.regs = {
			0x0000000a,   /* RC */
			0x00000016,   /* RFC */
			0x00000008,   /* RAS */
			0x00000003,   /* RP */
			0x00000004,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x000004df,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000003,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000006,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000008,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xa06e04ae,   /* CFG_DIG_DLL */
			0x007e2010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}, {
		.rate = 333000,   /* SDRAM frequency */
		.regs = {
			0x00000014,   /* RC */
			0x0000002b,   /* RFC */
			0x0000000f,   /* RAS */
			0x00000005,   /* RP */
			0x00000004,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000005,   /* RD_RCD */
			0x00000005,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x000009ff,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000005,   /* PCHG2PDEN */
			0x00000005,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000f,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000c,   /* TFAW */
			0x00000006,   /* TRPAB */
			0x00000008,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xe04e048b,   /* CFG_DIG_DLL */
			0x007de010,   /* DLL_XFORM_DQS */
			0x00022015,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

static const struct tegra_emc_table kaen_emc_tables_Samsung_380Mhz[] = {
	{
		.rate = 190000,   /* SDRAM frequency */
		.regs = {
			0x0000000c,   /* RC */
			0x00000019,   /* RFC */
			0x00000009,   /* RAS */
			0x00000003,   /* RP */
			0x00000004,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x0000059f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000003,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000b,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000007,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x00000008,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xa06204ae,   /* CFG_DIG_DLL */
			0x007e0010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}, {
		.rate = 380000,   /* SDRAM frequency */
		.regs = {
			0x00000017,   /* RC */
			0x00000031,   /* RFC */
			0x00000012,   /* RAS */
			0x00000006,   /* RP */
			0x00000004,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x0000000c,   /* W2P */
			0x00000006,   /* RD_RCD */
			0x00000006,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000001,   /* REXT */
			0x00000004,   /* WDV */
			0x00000005,   /* QUSE */
			0x00000004,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000d,   /* RDV */
			0x00000b5f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000006,   /* PCHG2PDEN */
			0x00000006,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x00000011,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000e,   /* TFAW */
			0x00000007,   /* TRPAB */
			0x00000008,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000002,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xe044048b,   /* CFG_DIG_DLL */
			0x007e0010,   /* DLL_XFORM_DQS */
			0x00023215,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

struct tegra_board_emc_table kaen_emc[] = {
	{
		.table		= kaen_emc_tables_Samsung_333Mhz,
		.table_size	= ARRAY_SIZE(kaen_emc_tables_Samsung_333Mhz),
		.name		= "Samsung 333MHz",
	},
	{
		.table		= kaen_emc_tables_Nanya_333Mhz,
		.table_size	= ARRAY_SIZE(kaen_emc_tables_Nanya_333Mhz),
		.name		= "Nanya 333MHz",
	},
	{
		.table		= kaen_emc_tables_Samsung_380Mhz,
		.table_size	= ARRAY_SIZE(kaen_emc_tables_Samsung_380Mhz),
		.name		= "Samsung 380MHz",
	},
	{
		.table		= kaen_emc_tables_Nanya_380Mhz,
		.table_size	= ARRAY_SIZE(kaen_emc_tables_Nanya_380Mhz),
		.name		= "Nanya 380MHz",
	},
};

#define STRAP_OPT 0x008
#define GMI_AD0 (1 << 4)
#define GMI_AD1 (1 << 5)
#define RAM_ID_MASK (GMI_AD0 | GMI_AD1)
#define RAM_CODE_SHIFT 4

void kaen_emc_init(void)
{
	u32 reg;
	int ram_id;

	reg = readl(NV_PA_APB_MISC_BASE + STRAP_OPT);
	ram_id = (reg & RAM_ID_MASK) >> RAM_CODE_SHIFT;

	if (ram_id >= ARRAY_SIZE(kaen_emc) || !kaen_emc[ram_id].table) {
		tegra_set_emc(NULL, 0);
	} else {
		tegra_set_emc(kaen_emc[ram_id].table,
			       kaen_emc[ram_id].table_size);
	}
}

static const struct tegra_emc_table aebl_emc_tables[] = {
	{
		.rate = 190000,   /* SDRAM frequency */
		.regs = {
			0x0000000b,   /* RC */
			0x00000026,   /* RFC */
			0x00000008,   /* RAS */
			0x00000003,   /* RP */
			0x00000004,   /* R2W */
			0x00000004,   /* W2R */
			0x00000002,   /* R2P */
			0x0000000b,   /* W2P */
			0x00000003,   /* RD_RCD */
			0x00000003,   /* WR_RCD */
			0x00000002,   /* RRD */
			0x00000001,   /* REXT */
			0x00000003,   /* WDV */
			0x00000004,   /* QUSE */
			0x00000005,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000c,   /* RDV */
			0x0000059f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000003,   /* PCHG2PDEN */
			0x00000003,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000a,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x00000007,   /* TFAW */
			0x00000004,   /* TRPAB */
			0x0000000f,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xa06204ae,   /* CFG_DIG_DLL */
			0x007e8010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}, {
		.rate = 380000,   /* SDRAM frequency */
		.regs = {
			0x00000015,   /* RC */
			0x0000004c,   /* RFC */
			0x00000010,   /* RAS */
			0x00000005,   /* RP */
			0x00000004,   /* R2W */
			0x00000005,   /* W2R */
			0x00000003,   /* R2P */
			0x0000000b,   /* W2P */
			0x00000005,   /* RD_RCD */
			0x00000005,   /* WR_RCD */
			0x00000003,   /* RRD */
			0x00000001,   /* REXT */
			0x00000003,   /* WDV */
			0x00000004,   /* QUSE */
			0x00000003,   /* QRST */
			0x00000009,   /* QSAFE */
			0x0000000c,   /* RDV */
			0x00000b5f,   /* REFRESH */
			0x00000000,   /* BURST_REFRESH_NUM */
			0x00000003,   /* PDEX2WR */
			0x00000003,   /* PDEX2RD */
			0x00000005,   /* PCHG2PDEN */
			0x00000005,   /* ACT2PDEN */
			0x00000001,   /* AR2PDEN */
			0x0000000f,   /* RW2PDEN */
			0x000000c8,   /* TXSR */
			0x00000003,   /* TCKE */
			0x0000000e,   /* TFAW */
			0x00000006,   /* TRPAB */
			0x0000000f,   /* TCLKSTABLE */
			0x00000002,   /* TCLKSTOP */
			0x00000000,   /* TREFBW */
			0x00000000,   /* QUSE_EXTRA */
			0x00000003,   /* FBIO_CFG6 */
			0x00000000,   /* ODT_WRITE */
			0x00000000,   /* ODT_READ */
			0x00000083,   /* FBIO_CFG5 */
			0xe044048b,   /* CFG_DIG_DLL */
			0x007e0010,   /* DLL_XFORM_DQS */
			0x00000000,   /* DLL_XFORM_QUSE */
			0x00000000,   /* ZCAL_REF_CNT */
			0x00000000,   /* ZCAL_WAIT_CNT */
			0x00000000,   /* AUTO_CAL_INTERVAL */
			0x00000000,   /* CFG_CLKTRIM_0 */
			0x00000000,   /* CFG_CLKTRIM_1 */
			0x00000000,   /* CFG_CLKTRIM_2 */
		}
	}
};

void aebl_emc_init(void)
{
	tegra_set_emc(aebl_emc_tables, ARRAY_SIZE(aebl_emc_tables));
}

struct emc_init {
	unsigned int	id;		/* board id */
	void		(*init)(void);
};

static struct emc_init board_table[] = {
	{
		.id	= MACH_TYPE_AEBL,
		.init	= aebl_emc_init,
	},
	{
		.id	= MACH_TYPE_HARMONY,
		.init	= NULL,
	},
	{
		.id	= MACH_TYPE_KAEN,
		.init	= kaen_emc_init,
	},
	{
		.id	= MACH_TYPE_SEABOARD,
		.init	= seaboard_emc_init,
	},
	{
		.id	= MACH_TYPE_WARIO,
		.init	= NULL,
	},
};

int board_emc_init(void)
{
	int	i;
	DECLARE_GLOBAL_DATA_PTR;

	/* if voltage has not been set properly, return */
	if (!tegra2_pmu_is_voltage_nominal())
		return -1;

	for (i = 0; i < ARRAY_SIZE(board_table); i++) {
		if (board_table[i].id == gd->bd->bi_arch_number) {
			if (board_table[i].init) {
				board_table[i].init();
				return 0;
			} else
				return -1;
		}
	}
	return -1;
}

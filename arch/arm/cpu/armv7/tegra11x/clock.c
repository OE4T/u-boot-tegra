/*
 * Copyright (c) 2011-2012 The Chromium OS Authors.
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

/* Tegra3 Clock control functions */

#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/timer.h>
#include <asm/arch/tegra.h>
#include <common.h>
#include <div64.h>

/*
 * This is our record of the current clock rate of each clock. We don't
 * fill all of these in since we are only really interested in clocks which
 * we use as parents.
 */
static unsigned pll_rate[CLOCK_ID_COUNT];

/*
 * The oscillator frequency is fixed to one of four set values. Based on this
 * the other clocks are set up appropriately.
 */
static unsigned osc_freq[CLOCK_OSC_FREQ_COUNT] = {
	13000000,
	19200000,
	12000000,
	26000000,
};

/*
 * Clock types that we can use as a source. The Tegra3 has muxes for the
 * peripheral clocks, and in most cases there are four options for the clock
 * source. This gives us a clock 'type' and exploits what commonality exists
 * in the device.
 *
 * Letters are obvious, except for T which means CLK_M, and S which means the
 * clock derived from 32KHz. Beware that CLK_M (also called OSC in the
 * datasheet) and PLL_M are different things. The former is the basic
 * clock supplied to the SOC from an external oscillator. The latter is the
 * memory clock PLL.
 *
 * See definitions in clock_id in the header file.
 */
enum clock_type_id {
	CLOCK_TYPE_AXPT,	/* PLL_A, PLL_X, PLL_P, CLK_M */
	CLOCK_TYPE_MCPA,	/* and so on */
	CLOCK_TYPE_MCPT,
	CLOCK_TYPE_PCM,
	CLOCK_TYPE_PCMT,
	CLOCK_TYPE_PDCT,
	CLOCK_TYPE_ACPT,
	CLOCK_TYPE_ASPTE,
	CLOCK_TYPE_PMDACD2T,
	CLOCK_TYPE_PCST,

	CLOCK_TYPE_PC2CC3M,
	CLOCK_TYPE_PC2CC3S_T,
	CLOCK_TYPE_PC2CC3M_T,
	CLOCK_TYPE_MC2CC3P_A,
	CLOCK_TYPE_M,
	CLOCK_TYPE_MCPTM2C2C3,
	CLOCK_TYPE_PC2CC3T_S,
	CLOCK_TYPE_AC2CC3P_TS2,

	CLOCK_TYPE_COUNT,
	CLOCK_TYPE_NONE = -1,	/* invalid clock type */
};

/* return 1 if a peripheral ID is in range */
#define clock_type_id_isvalid(id) ((id) >= 0 && \
		(id) < CLOCK_TYPE_COUNT)


enum {
	CLOCK_MAX_MUX	= 8	/* number of source options for each clock */
};

enum {
	MASK_BITS_31_30	= 2,	/* num of bits used to specify clock source */
	MASK_BITS_31_29,
	MASK_BITS_29_28,
};

/*
 * Clock source mux for each clock type. This just converts our enum into
 * a list of mux sources for use by the code.
 *
 * Note:
 *  The extra column in each clock source array is used to store the mask
 *  bits in its register for the source.
 */
#define CLK(x) CLOCK_ID_ ## x
static enum clock_id clock_source[CLOCK_TYPE_COUNT] [CLOCK_MAX_MUX+1] = {
	{ CLK(AUDIO),	CLK(XCPU),	CLK(PERIPH),	CLK(OSC),
		CLK(NONE),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_31_30},
	{ CLK(MEMORY),	CLK(CGENERAL),	CLK(PERIPH),	CLK(AUDIO),
		CLK(NONE),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_31_30},
	{ CLK(MEMORY),	CLK(CGENERAL),	CLK(PERIPH),	CLK(OSC),
		CLK(NONE),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_31_30},
	{ CLK(PERIPH),	CLK(CGENERAL),	CLK(MEMORY),	CLK(NONE),
		CLK(NONE),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_31_30},
	{ CLK(PERIPH),	CLK(CGENERAL),	CLK(MEMORY),	CLK(OSC),
		CLK(NONE),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_31_30},
	{ CLK(PERIPH),	CLK(DISPLAY),	CLK(CGENERAL),	CLK(OSC),
		CLK(NONE),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_31_30},
	{ CLK(AUDIO),	CLK(CGENERAL),	CLK(PERIPH),	CLK(OSC),
		CLK(NONE),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_31_30},
	{ CLK(AUDIO),	CLK(SFROM32KHZ),	CLK(PERIPH),	CLK(OSC),
		CLK(EPCI),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_31_29},
	{ CLK(PERIPH),	CLK(MEMORY),	CLK(DISPLAY),	CLK(AUDIO),
		CLK(CGENERAL),	CLK(DISPLAY2_RESERVED),	CLK(OSC), CLK(NONE),
		MASK_BITS_31_29},
	{ CLK(PERIPH),	CLK(CGENERAL),	CLK(SFROM32KHZ),	CLK(OSC),
		CLK(NONE),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_29_28},

	/* CLOCK_TYPE_PC2CC3M */
	{ CLK(PERIPH),	CLK(CGENERAL2_RESERVED),	CLK(CGENERAL),
		CLK(CGENERAL3_RESERVED),	CLK(MEMORY),	CLK(NONE),
		CLK(NONE),	CLK(NONE),
		MASK_BITS_31_29},

	/* CLOCK_TYPE_PC2CC3S_T */
	{ CLK(PERIPH),	CLK(CGENERAL2_RESERVED),	CLK(CGENERAL),
		CLK(CGENERAL3_RESERVED),	CLK(SFROM32KHZ),
		CLK(NONE),	CLK(OSC),
		CLK(NONE),	MASK_BITS_31_29},

	/* CLOCK_TYPE_PC2CC3M_T */
	{ CLK(PERIPH),	CLK(CGENERAL2_RESERVED),	CLK(CGENERAL),
		CLK(CGENERAL3_RESERVED),	CLK(MEMORY),	CLK(NONE),
		CLK(OSC),	CLK(NONE),
		MASK_BITS_31_29},

	/* CLOCK_TYPE_MC2CC3P_A */
	{ CLK(MEMORY),	CLK(CGENERAL2_RESERVED),	CLK(CGENERAL),
		CLK(CGENERAL3_RESERVED),	CLK(PERIPH),	CLK(NONE),
		CLK(AUDIO),	CLK(NONE),
		MASK_BITS_31_29},

	/* CLOCK_TYPE_M */
	{ CLK(MEMORY),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		CLK(NONE),	CLK(NONE),	CLK(NONE),	CLK(NONE),
		MASK_BITS_31_30},

	/* CLOCK_TYPE_MCPTM2C2C3 */
	{ CLK(MEMORY),	CLK(CGENERAL),	CLK(PERIPH),	CLK(OSC),
		CLK(MEMORY2_RESERVED),	CLK(CGENERAL2_RESERVED),
		CLK(CGENERAL3_RESERVED),	CLK(NONE),
		MASK_BITS_31_29},

	/* CLOCK_TYPE_PC2CC3T_S */
	{ CLK(PERIPH),	CLK(CGENERAL2_RESERVED),	CLK(CGENERAL),
		CLK(CGENERAL3_RESERVED),	CLK(OSC),	CLK(NONE),
		CLK(SFROM32KHZ),	CLK(NONE),
		MASK_BITS_31_29},

	/* CLOCK_TYPE_AC2CC3P_TS2 */
	{ CLK(AUDIO),	CLK(CGENERAL2_RESERVED),	CLK(CGENERAL),
		CLK(CGENERAL3_RESERVED),	CLK(PERIPH),	CLK(NONE),
		CLK(OSC),	CLK(SRC2_RESERVED),
		MASK_BITS_31_29},
};

/* return 1 if a periphc_internal_id is in range */
#define periphc_internal_id_isvalid(id) ((id) >= 0 && \
		(id) < PERIPHC_COUNT)

/*
 * Clock type for each peripheral clock source. We put the name in each
 * record just so it is easy to match things up
 */
#define TYPE(name, type) type
static enum clock_type_id clock_periph_type[PERIPHC_COUNT] = {
	/* 0x00 */
	TYPE(PERIPHC_I2S1,	CLOCK_TYPE_AXPT),
	TYPE(PERIPHC_I2S2,	CLOCK_TYPE_AXPT),
	TYPE(PERIPHC_SPDIF_OUT,	CLOCK_TYPE_AXPT),
	TYPE(PERIPHC_SPDIF_IN,	CLOCK_TYPE_PC2CC3M),
	TYPE(PERIPHC_PWM,	CLOCK_TYPE_PC2CC3S_T),
	TYPE(PERIPHC_05h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_SBC2,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_SBC3,	CLOCK_TYPE_PC2CC3M_T),

	/* 0x08 */
	TYPE(PERIPHC_08h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_I2C1,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_I2C5,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_0bh,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_0ch,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_SBC1,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_DISP1,	CLOCK_TYPE_PMDACD2T),
	TYPE(PERIPHC_DISP2,	CLOCK_TYPE_PMDACD2T),

	/* 0x10 */
	TYPE(PERIPHC_10h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_11h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_VI,	CLOCK_TYPE_MC2CC3P_A),
	TYPE(PERIPHC_13h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_SDMMC1,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_SDMMC2,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_G3D,	CLOCK_TYPE_MC2CC3P_A),
	TYPE(PERIPHC_G2D,	CLOCK_TYPE_MC2CC3P_A),

	/* 0x18 */
	TYPE(PERIPHC_NDFLASH,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_SDMMC4,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_VFIR,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_EPP,	CLOCK_TYPE_MC2CC3P_A),
	TYPE(PERIPHC_MPE,	CLOCK_TYPE_M),
	TYPE(PERIPHC_HSI,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_UART1,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_UART2,	CLOCK_TYPE_PC2CC3M_T),

	/* 0x20 */
	TYPE(PERIPHC_HOST1X,	CLOCK_TYPE_MC2CC3P_A),
	TYPE(PERIPHC_21h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_22h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_HDMI,	CLOCK_TYPE_PMDACD2T),
	TYPE(PERIPHC_24h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_25h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_I2C2,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_EMC,	CLOCK_TYPE_MCPTM2C2C3),

	/* 0x28 */
	TYPE(PERIPHC_UART3,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_29h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_VI_SENSOR,	CLOCK_TYPE_MC2CC3P_A),
	TYPE(PERIPHC_2bh,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_2ch,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_SBC4,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_I2C3,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_SDMMC3,	CLOCK_TYPE_PC2CC3M_T),

	/* 0x30 */
	TYPE(PERIPHC_UART4,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_UART5,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_VDE,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_OWR,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_NOR,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_CSITE,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_I2S0,	CLOCK_TYPE_AXPT),
	TYPE(PERIPHC_DTV,	CLOCK_TYPE_NONE),

	/* 0x38h */
	TYPE(PERIPHC_38h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_39h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_3ah,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_3bh,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_MSENC,	CLOCK_TYPE_MC2CC3P_A),
	TYPE(PERIPHC_TSEC,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_3eh,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_OSC,	CLOCK_TYPE_NONE),

	/* 0x40 */
	TYPE(PERIPHC_G3D2,	CLOCK_TYPE_MC2CC3P_A),	/* start with 0x3b0 */
	TYPE(PERIPHC_MSELECT,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_TSENSOR,	CLOCK_TYPE_PC2CC3T_S),
	TYPE(PERIPHC_I2S3,	CLOCK_TYPE_AXPT),
	TYPE(PERIPHC_I2S4,	CLOCK_TYPE_AXPT),
	TYPE(PERIPHC_I2C4,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_SBC5,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_SBC6,	CLOCK_TYPE_PC2CC3M_T),

	/* 0x48 */
	TYPE(PERIPHC_AUDIO,	CLOCK_TYPE_AC2CC3P_TS2),
	TYPE(PERIPHC_49h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_DAM0,	CLOCK_TYPE_AC2CC3P_TS2),
	TYPE(PERIPHC_DAM1,	CLOCK_TYPE_AC2CC3P_TS2),
	TYPE(PERIPHC_DAM2,	CLOCK_TYPE_AC2CC3P_TS2),
	TYPE(PERIPHC_HDA2CODEC2X, CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_ACTMON,	CLOCK_TYPE_PC2CC3S_T),
	TYPE(PERIPHC_EXTPERIPH1, CLOCK_TYPE_ASPTE),

	/* 0x50 */
	TYPE(PERIPHC_EXTPERIPH2, CLOCK_TYPE_ASPTE),
	TYPE(PERIPHC_EXTPERIPH3, CLOCK_TYPE_ASPTE),
	TYPE(PERIPHC_NANDSPEED,	CLOCK_TYPE_PC2CC3M_T),
	TYPE(PERIPHC_I2CSLOW,	CLOCK_TYPE_PC2CC3S_T),
	TYPE(PERIPHC_SYS,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_55h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_56h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_57h,	CLOCK_TYPE_NONE),

	/* 0x58 */
	TYPE(PERIPHC_58h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_59h,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_5ah,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_5bh,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_SATAOOB,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_SATA,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_HDA,	CLOCK_TYPE_PC2CC3M_T),
};

/*
 * This array translates a periph_id to a periphc_internal_id
 */
#define NONE(name) (-1)
#define OFFSET(name, value) PERIPHC_ ## name
static s8 periph_id_to_internal_id[PERIPH_ID_COUNT] = {
	/* Low word: 31:0 */
	NONE(CPU),
	NONE(COP),
	NONE(TRIGSYS),
	NONE(RESERVED3),
	NONE(RESERVED4),
	NONE(TMR),
	PERIPHC_UART1,
	PERIPHC_UART2,	/* and vfir 0x68 */

	/* 8 */
	NONE(GPIO),
	PERIPHC_SDMMC2,
	PERIPHC_SPDIF_IN,
	PERIPHC_I2S1,
	PERIPHC_I2C1,
	PERIPHC_NDFLASH,
	PERIPHC_SDMMC1,
	PERIPHC_SDMMC4,

	/* 16 */
	NONE(RESERVED16),
	PERIPHC_PWM,
	PERIPHC_I2S2,
	PERIPHC_EPP,
	PERIPHC_VI,
	PERIPHC_G2D,
	NONE(USBD),
	NONE(ISP),

	/* 24 */
	PERIPHC_G3D,
	NONE(RESERVED25),
	PERIPHC_DISP2,
	PERIPHC_DISP1,
	PERIPHC_HOST1X,
	NONE(VCP),
	PERIPHC_I2S0,
	NONE(CACHE2),

	/* Middle word: 63:32 */
	NONE(MEM),
	NONE(AHBDMA),
	NONE(APBDMA),
	NONE(RESERVED35),
	NONE(RESERVED36),
	NONE(STAT_MON),
	NONE(RESERVED38),
	NONE(RESERVED39),

	/* 40 */
	NONE(KFUSE),
	NONE(SBC1),	/* SBC1, 0x34, is this SPI1? */
	PERIPHC_NOR,
	NONE(RESERVED43),
	PERIPHC_SBC2,
	NONE(RESERVED45),
	PERIPHC_SBC3,
	PERIPHC_I2C5,

	/* 48 */
	NONE(DSI),
	NONE(RESERVED49),
	PERIPHC_HSI,
	PERIPHC_HDMI,
	NONE(CSI),
	NONE(RESERVED53),
	PERIPHC_I2C2,
	PERIPHC_UART3,

	/* 56 */
	NONE(MIPI_CAL),
	PERIPHC_EMC,
	NONE(USB2),
	NONE(USB3),
	PERIPHC_MPE,
	PERIPHC_VDE,
	NONE(BSEA),
	NONE(BSEV),

	/* Upper word 95:64 */
	NONE(RESERVED64),
	PERIPHC_UART4,
	PERIPHC_UART5,
	PERIPHC_I2C3,
	PERIPHC_SBC4,
	PERIPHC_SDMMC3,
	NONE(PCIE),
	PERIPHC_OWR,

	/* 72 */
	NONE(AFI),
	PERIPHC_CSITE,
	NONE(PCIEXCLK),
	NONE(AVPUCQ),
	NONE(RESERVED76),
	NONE(TRACECLKIN),
	NONE(SOC_THERM),
	NONE(DTV),

	/* 80 */
	PERIPHC_NANDSPEED,
	PERIPHC_I2CSLOW,
	NONE(DSIB),
	PERIPHC_TSEC,
	NONE(RESERVED84),
	NONE(RESERVED85),
	NONE(RESERVED86),
	NONE(EMUCIF),

	/* 88 */
	NONE(RESERVED88),
	NONE(XUSB_HOST),
	NONE(RESERVED90),
	PERIPHC_MSENC,
	NONE(RESERVED92),
	NONE(RESERVED93),
	NONE(RESERVED94),
	NONE(XUSB_DEV),

	/* V word: 31:0 */
	NONE(CPUG),
	NONE(CPULP),
	PERIPHC_G3D2,
	PERIPHC_MSELECT,
	NONE(V_RESERVED4),
	PERIPHC_I2S3,
	PERIPHC_I2S4,
	PERIPHC_I2C4,

	/* 08 */
	PERIPHC_SBC5,
	PERIPHC_SBC6,
	PERIPHC_AUDIO,
	NONE(APBIF),
	PERIPHC_DAM0,
	PERIPHC_DAM1,
	PERIPHC_DAM2,
	PERIPHC_HDA2CODEC2X,

	/* 16 */
	NONE(ATOMICS),
	NONE(V_RESERVED17),
	NONE(V_RESERVED18),
	NONE(V_RESERVED19),
	NONE(V_RESERVED20),
	NONE(V_RESERVED21),
	NONE(V_RESERVED22),
	PERIPHC_ACTMON,

	/* 24 */
	NONE(V_RESERVED24),
	NONE(V_RESERVED25),
	NONE(V_RESERVED26),
	NONE(V_RESERVED27),
	PERIPHC_SATA,
	PERIPHC_HDA,
	NONE(V_RESERVED30),
	NONE(V_RESERVED31),

	/* W word: 31:0 */
	NONE(HDA2HDMICODEC),
	NONE(SATACOLD),
	NONE(W_RESERVED2),
	NONE(W_RESERVED3),
	NONE(W_RESERVED4),
	NONE(W_RESERVED5),
	NONE(W_RESERVED6),
	NONE(W_RESERVED7),

	/* 40 */
	NONE(CEC),
	NONE(W_RESERVED9),
	NONE(W_RESERVED10),
	NONE(W_RESERVED11),
	NONE(W_RESERVED12),
	NONE(W_RESERVED13),
	NONE(XUSB_PADCTL),
	NONE(W_RESERVED15),

	/* 48 */
	NONE(W_RESERVED16),
	NONE(W_RESERVED17),
	NONE(W_RESERVED18),
	NONE(W_RESERVED19),
	NONE(W_RESERVED20),
	NONE(ENTROPY),
	NONE(W_RESERVED22),
	NONE(W_RESERVED23),

	/* 56 */
	NONE(W_RESERVED24),
	NONE(AMX0),
	NONE(ADX0),
	NONE(DVFS),
	NONE(XUSB_SS),
	NONE(W_RESERVED29),
	NONE(MC1),
	NONE(EMC1),
};

/*
 * Get the oscillator frequency, from the corresponding hardware configuration
 * field.
 */
enum clock_osc_freq clock_get_osc_freq(void)
{
	struct clk_rst_ctlr *clkrst =
			(struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	u32 reg;

	reg = readl(&clkrst->crc_osc_ctrl);
	return bf_unpack(OSC_FREQ, reg);
}

/* Returns a pointer to the registers of the given pll */
static struct clk_pll *get_pll(enum clock_id clkid)
{
	struct clk_rst_ctlr *clkrst =
			(struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;

	assert(clock_id_isvalid(clkid));
	return &clkrst->crc_pll[clkid];
}

unsigned long clock_start_pll(enum clock_id clkid, u32 divm, u32 divn,
		u32 divp, u32 cpcon, u32 lfcon)
{
	struct clk_pll *pll = get_pll(clkid);
	u32 data;

	/*
	 * We cheat by treating all PLL (except PLLU) in the same fashion.
	 * This works only because:
	 * - same fields are always mapped at same offsets, except DCCON
	 * - DCCON is always 0, doesn't conflict
	 * - M,N, P of PLLP values are ignored for PLLP
	 */

	data = bf_pack(PLL_CPCON, cpcon) |
		bf_pack(PLL_LFCON, lfcon);
	writel(data, &pll->pll_misc);

	data = bf_pack(PLL_DIVM, divm) |
		bf_pack(PLL_DIVN, divn) |
		bf_pack(PLL_BYPASS, 0) |
		bf_pack(PLL_ENABLE, 1);

	if (clkid == CLOCK_ID_USB)
		data |= bf_pack(PLL_VCO_FREQ, divp);
	else
		data |= bf_pack(PLL_DIVP, divp);
	writel(data, &pll->pll_base);

	/* calculate the stable time */
	return timer_get_future_us(CLOCK_PLL_STABLE_DELAY_US);
}

/* Returns a pointer to the clock source register for a peripheral */
static u32 *get_periph_source_reg(enum periph_id periph_id)
{
	struct clk_rst_ctlr *clkrst =
			(struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	enum periphc_internal_id internal_id;

	assert(periph_id >= PERIPH_ID_FIRST && periph_id < PERIPH_ID_COUNT);
	internal_id = periph_id_to_internal_id[periph_id];
	assert(internal_id != -1);
	if (internal_id >= PERIPHC_VW_FIRST) {
		internal_id -= PERIPHC_VW_FIRST;
		return &clkrst->crc_clk_src_vw[internal_id];
	} else
		return &clkrst->crc_clk_src[internal_id];
}

static int get_periph_clock_source_mux_bits(enum periph_id periph_id)
{
	enum clock_type_id type;
	enum periphc_internal_id internal_id;

	assert(clock_periph_id_isvalid(periph_id));

	internal_id = periph_id_to_internal_id[periph_id];
	assert(periphc_internal_id_isvalid(internal_id));

	type = clock_periph_type[internal_id];
	assert(clock_type_id_isvalid(type));

	return clock_source[type][CLOCK_MAX_MUX];
}

static u32 update_periph_clock_source(int mux_bits, u32 value, int source)
{
	switch (mux_bits) {
	case MASK_BITS_31_30:
		bf_update(OUT_CLK_SOURCE, value, source);
		break;

	case MASK_BITS_31_29:
		bf_update(OUT_CLK_SOURCE3, value, source);
		break;

	case MASK_BITS_29_28:
		bf_update(OUT_CLK_SOURCE42, value, source);
		break;
	}
	return value;
}

/**
 * Set periph clock source and divisor
 *
 * @param periph_id	peripherals defined in CAR L_H_U_V_W registers
 * @param source	the clock source field value defined in TRM
 *			(not an enum clock_id value)
 * @param divisor	the clock divisor field value defined in TRM
 */
void clock_ll_set_source_divisor(enum periph_id periph_id, int source,
			      unsigned divisor)
{
	u32 *reg = get_periph_source_reg(periph_id);
	u32 value;
	int mux_bits;

	value = readl(reg);

	/* update source */
	mux_bits = get_periph_clock_source_mux_bits(periph_id);
	value = update_periph_clock_source(mux_bits, value, source);

	/* update divisor */
	bf_update(OUT_CLK_DIVISOR, value, divisor);

	writel(value, reg);
}

/**
 * Set periph clock source
 *
 * @param periph_id	peripherals defined in CAR L_H_U_V_W registers
 * @param source	the clock source value defined in TRM
 *			(not an enum clock_id value)
 */
void clock_ll_set_source(enum periph_id periph_id, int source)
{
	u32 *reg = get_periph_source_reg(periph_id);
	u32 value;
	int mux_bits;

	value = readl(reg);

	/* update source */
	mux_bits = get_periph_clock_source_mux_bits(periph_id);
	value = update_periph_clock_source(mux_bits, value, source);

	writel(value, reg);
}

/**
 * Given the parent's rate and the required rate for the children, this works
 * out the peripheral clock divider to use, in 7.1 binary format.
 *
 * @param parent_rate	clock rate of parent clock in Hz
 * @param rate		required clock rate for this clock
 * @return divider which should be used
 */
static int clk_div7_1_get_divider(unsigned long parent_rate,
				  unsigned long rate)
{
	u64 divider = parent_rate * 2;

	divider += rate - 1;
	do_div(divider, rate);

	if ((s64)divider - 2 < 0)
		return 0;

	if ((s64)divider - 2 > 255)
		return -1;

	return divider - 2;
}

/**
 * Given the parent's rate and the divider in 7.1 format, this works out the
 * resulting peripheral clock rate.
 *
 * @param parent_rate	clock rate of parent clock in Hz
 * @param divider which should be used in 7.1 format
 * @return effective clock rate of peripheral
 */
static unsigned long get_rate_from_divider(unsigned long parent_rate,
					   int divider)
{
	u64 rate;

	rate = (u64)parent_rate * 2;
	do_div (rate, divider + 2);
	return rate;
}

unsigned long clock_get_periph_rate(enum periph_id periph_id,
		enum clock_id parent)
{
	u32 *reg = get_periph_source_reg(periph_id);

	return get_rate_from_divider(pll_rate[parent],
				     bf_readl(OUT_CLK_DIVISOR, reg));
}

/**
 * Find the best available 7.1 format divisor given a parent clock rate and
 * required child clock rate. This function assumes that a second-stage
 * divisor is available which can divide by powers of 2 from 1 to 256.
 *
 * @param parent_rate	clock rate of parent clock in Hz
 * @param rate		required clock rate for this clock
 * @param extra_div	value for the second-stage divisor (not set if this
 *			function returns -1.
 * @return divider which should be used, or -1 if nothing is valid
 *
 */
static int find_best_divider(unsigned long parent_rate, unsigned long rate,
		int *extra_div)
{
	int shift;
	int best_divider = -1;
	int best_error = rate;

	/* try dividers from 1 to 256 and find closest match */
	for (shift = 0; shift <= 8 && best_error > 0; shift++) {
		unsigned divided_parent = parent_rate >> shift;
		int divider = clk_div7_1_get_divider(divided_parent, rate);
		unsigned effective_rate = get_rate_from_divider(divided_parent,
						       divider);
		int error = rate - effective_rate;

		/* Given a valid divider, look for the lowest error */
		if (divider != -1 && error < best_error) {
			best_error = error;
			*extra_div = 1 << shift;
			best_divider = divider;
		}
	}

	/* return what we found - *extra_div will already be set */
	return best_divider;
}

/**
 * Given a peripheral ID and the required source clock, this returns which
 * value should be programmed into the source mux for that peripheral.
 *
 * There is special code here to handle the one source type with 5 sources.
 *
 * @param periph_id	peripheral to start
 * @param source	PLL id of required parent clock
 * @param mux_bits	Set to number of bits in mux register: 2,3 or 4
 * @return mux value (0-6, or -1 if not found)
 */
static int get_periph_clock_source(enum periph_id periph_id,
		enum clock_id parent, int *mux_bits)
{
	enum clock_type_id type;
	enum periphc_internal_id internal_id;
	int mux;

	assert(clock_periph_id_isvalid(periph_id));

	internal_id = periph_id_to_internal_id[periph_id];
	assert(periphc_internal_id_isvalid(internal_id));

	type = clock_periph_type[internal_id];
	assert(clock_type_id_isvalid(type));

	*mux_bits = clock_source[type][CLOCK_MAX_MUX];

	for (mux = 0; mux < CLOCK_MAX_MUX; mux++)
		if (clock_source[type][mux] == parent)
			return mux;

	/* if we get here, either us or the caller has made a mistake */
	printf("Caller requested bad clock: periph=%d, parent=%d\n", periph_id,
		parent);
	return -1;
}

/**
 * Adjust peripheral PLL to use the given divider and source.
 *
 * @param periph_id	peripheral to adjust
 * @param parent	Required parent clock (for source mux)
 * @param divider	Required divider in 7.1 format
 * @return 0 if ok, -1 on error (requesting a parent clock which is not valid
 *		for this peripheral)
 */
static int adjust_periph_pll(enum periph_id periph_id,
		enum clock_id parent, int divider)
{
	u32 *reg = get_periph_source_reg(periph_id);
	int source, mux_bits;

	bf_writel(OUT_CLK_DIVISOR, divider, reg);
	udelay(1);

	/* work out the source clock and set it */
	source = get_periph_clock_source(periph_id, parent, &mux_bits);
	if (source < 0)
		return -1;

	switch (mux_bits) {
	case MASK_BITS_31_30:
		bf_writel(OUT_CLK_SOURCE, source, reg);
		break;

	case MASK_BITS_31_29:
		bf_writel(OUT_CLK_SOURCE3, source, reg);
		break;

	case MASK_BITS_29_28:
		bf_writel(OUT_CLK_SOURCE42, source, reg);
		break;
	default:
		/* should not be here. */
		printf("Error: unknown mux_id: %d\n", mux_bits);
		break;
	}
	udelay(2);
	return 0;
}

unsigned clock_adjust_periph_pll_div(enum periph_id periph_id,
		enum clock_id parent, unsigned rate, int *extra_div)
{
	unsigned effective_rate;
	int divider;

	if (extra_div)
		divider = find_best_divider(pll_rate[parent], rate, extra_div);
	else
		divider = clk_div7_1_get_divider(pll_rate[parent], rate);
	assert(divider >= 0);
	if (adjust_periph_pll(periph_id, parent, divider))
		return -1U;
	debug("periph %d, rate=%d, reg=%p = %x\n", periph_id, rate,
		get_periph_source_reg(periph_id),
		readl(get_periph_source_reg(periph_id)));

	/* Check what we ended up with. This shouldn't matter though */
	effective_rate = clock_get_periph_rate(periph_id, parent);
	if (extra_div)
		effective_rate /= *extra_div;
	if (rate != effective_rate)
		debug("Requested clock rate %u not honored (got %u)\n",
		       rate, effective_rate);
	return effective_rate;
}

unsigned clock_start_periph_pll(enum periph_id periph_id,
		enum clock_id parent, unsigned rate)
{
	unsigned effective_rate;

	reset_set_enable(periph_id, 1);
	clock_enable(periph_id);

	effective_rate = clock_adjust_periph_pll_div(periph_id, parent, rate,
						 NULL);

	reset_set_enable(periph_id, 0);
	return effective_rate;
}

void clock_set_enable(enum periph_id periph_id, int enable)
{
	struct clk_rst_ctlr *clkrst =
			(struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	u32 *clk;
	u32 reg;

	/* Enable/disable the clock to this peripheral */
	assert(clock_periph_id_isvalid(periph_id));
	if ((int)periph_id < (int)PERIPH_ID_VW_FIRST)
		clk = &clkrst->crc_clk_out_enb[PERIPH_REG(periph_id)];
	else
		clk = &clkrst->crc_clk_out_enb_vw[PERIPH_REG(periph_id)];
	reg = readl(clk);
	if (enable)
		reg |= PERIPH_MASK(periph_id);
	else
		reg &= ~PERIPH_MASK(periph_id);
	writel(reg, clk);
}

void clock_enable(enum periph_id clkid)
{
	clock_set_enable(clkid, 1);
}

void clock_disable(enum periph_id clkid)
{
	clock_set_enable(clkid, 0);
}

void reset_set_enable(enum periph_id periph_id, int enable)
{
	struct clk_rst_ctlr *clkrst =
			(struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	u32 *reset;
	u32 reg;

	/* Enable/disable reset to the peripheral */
	assert(clock_periph_id_isvalid(periph_id));
	if (periph_id < PERIPH_ID_VW_FIRST)
		reset = &clkrst->crc_rst_dev[PERIPH_REG(periph_id)];
	else
		reset = &clkrst->crc_rst_dev_vw[PERIPH_REG(periph_id)];
	reg = readl(reset);
	if (enable)
		reg |= PERIPH_MASK(periph_id);
	else
		reg &= ~PERIPH_MASK(periph_id);
	writel(reg, reset);
}

void reset_periph(enum periph_id periph_id, int us_delay)
{
	/* Put peripheral into reset */
	reset_set_enable(periph_id, 1);
	udelay(us_delay);

	/* Remove reset */
	reset_set_enable(periph_id, 0);

	udelay(us_delay);
}

void reset_cmplx_set_enable(int cpu, int which, int reset)
{
	struct clk_rst_ctlr *clkrst =
			(struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	u32 mask;

	/* Form the mask, which depends on the cpu chosen. Tegra3 has 4 */
	assert(cpu >= 0 && cpu < 4);
	mask = which << cpu;

	/* either enable or disable those reset for that CPU */
	if (reset)
		writel(mask, &clkrst->crc_rst_cpu_cmplx_set);
	else
		writel(mask, &clkrst->crc_rst_cpu_cmplx_clr);
}

unsigned clock_get_rate(enum clock_id clkid)
{
	struct clk_pll *pll;
	u32 base;
	u32 divm;
	u64 parent_rate;
	u64 rate;

	parent_rate = osc_freq[clock_get_osc_freq()];
	if (clkid == CLOCK_ID_OSC)
		return parent_rate;

	pll = get_pll(clkid);
	base = readl(&pll->pll_base);
	rate = parent_rate * bf_unpack(PLL_DIVN, base);
	divm = bf_unpack(PLL_DIVM, base);
	if (clkid == CLOCK_ID_USB)
		divm <<= bf_unpack(PLLU_VCO_FREQ, base);
	else
		divm <<= bf_unpack(PLL_DIVP, base);
	do_div(rate, divm);
	return rate;
}

/**
 * Set the output frequency you want for each PLL clock.
 * PLL output frequencies are programmed by setting their N, M and P values.
 * The governing equations are:
 *     VCO = (Fi / m) * n, Fo = VCO / (2^p)
 *     where Fo is the output frequency from the PLL.
 * Example: Set the output frequency to 216Mhz(Fo) with 12Mhz OSC(Fi)
 *     216Mhz = ((12Mhz / m) * n) / (2^p) so n=432,m=12,p=1
 * Please see Tegra TRM PLL Programming section to get the detail
 *
 * @param n PLL feedback divider(DIVN)
 * @param m PLL input divider(DIVN)
 * @param p post divider(DIVP)
 * @param cpcon base PLL charge pump(CPCON)
 * @return 0 if ok, -1 on error (the requested PLL cannot be overriden)
 */
static int clock_set_rate(enum clock_id clkid, u32 n, u32 m, u32 p, u32 cpcon)
{
	u32 base_reg;
	u32 misc_reg;
	struct clk_pll *pll;

	pll = get_pll(clkid);

	base_reg = readl(&pll->pll_base);

	if(clkid == CLOCK_ID_PERIPH)
		if(base_reg & bf_mask(PLL_BASE_OVRRIDE))
			return -1;

	/* Set BYPASS, m, n and p to PLL_BASE */
	bf_update(PLL_BYPASS, base_reg, 1);
	if(clkid == CLOCK_ID_PERIPH)
		bf_update(PLL_BASE_OVRRIDE, base_reg, 1);
	bf_update(PLL_DIVM, base_reg, m);
	bf_update(PLL_DIVN, base_reg, n);
	bf_update(PLL_DIVP, base_reg, p);
	writel(base_reg, &pll->pll_base);

	/* Set cpcon to PLL_MISC */
	misc_reg = readl(&pll->pll_misc);
	bf_update(PLL_CPCON, misc_reg, cpcon);
	writel(misc_reg, &pll->pll_misc);

	/* Enable PLL */
	bf_update(PLL_ENABLE, base_reg, 1);
	writel(base_reg, &pll->pll_base);

	/* Disable BYPASS */
	bf_update(PLL_BYPASS, base_reg, 0);
	writel(base_reg, &pll->pll_base);

	/*
	 * TODO: add next two steps for setting pll
	 */
	/* Enable Lock */

	/* Check lock status (bit 27 in base reg) */
	return 0;
}

void common_pll_init(void)
{
	/*
	 * PLLP output frequency set to 216Mh or 408Mh
	 * PLLC output frequency set to 600Mh
	 * PLLD output frequecy set to 925Mh
	 */
	switch (clock_get_rate(CLOCK_ID_OSC)) {
	case 12000000: /* OSC is 12Mhz */
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
		clock_set_rate(CLOCK_ID_PERIPH, 408, 12, 0, 8);
#else
		clock_set_rate(CLOCK_ID_PERIPH, 432, 12, 1, 8);
#endif

		clock_set_rate(CLOCK_ID_CGENERAL, 600, 12, 0, 8);
		clock_set_rate(CLOCK_ID_DISPLAY, 925, 12, 0, 12);
		break;
	case 13000000: /* OSC is 13Mhz */
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
		clock_set_rate(CLOCK_ID_PERIPH, 408, 13, 0, 8);
#else
		clock_set_rate(CLOCK_ID_PERIPH, 432, 13, 1, 8);
#endif
		clock_set_rate(CLOCK_ID_CGENERAL, 600, 13, 0, 8);
		clock_set_rate(CLOCK_ID_DISPLAY, 925, 13, 0, 12);
		break;
	/* TBD - handle other OSC freqs (19.2, 26, etc.) */
	}

	/*
	 * Set PLLC_MISC2 and MISC
	 *
	 * TODO: replace direct setting with functions
	 */
	writel(0x00561600, 0x60006088);	/* PLLC_MISC2: dynramp_stepA/B */
	writel(0x01000800, 0x6000608c);	/* PLLC_MISC: LOCK_ENABLE */
	udelay(2);

	/*
	 * Set PLLD_MISC
	 */
	writel(0x40000c10, 0x600060dc);	/* PLLD: LOCK_ENABLE */
					/* CPCON: 12, LFCON: 1 */
	udelay(2);
}

void clock_init(void)
{
	pll_rate[CLOCK_ID_MEMORY] = clock_get_rate(CLOCK_ID_MEMORY);
	pll_rate[CLOCK_ID_PERIPH] = clock_get_rate(CLOCK_ID_PERIPH);
	pll_rate[CLOCK_ID_CGENERAL] = clock_get_rate(CLOCK_ID_CGENERAL);
	pll_rate[CLOCK_ID_OSC] = clock_get_rate(CLOCK_ID_OSC);
	pll_rate[CLOCK_ID_SFROM32KHZ] = 32768;
	debug("Osc = %d\n", pll_rate[CLOCK_ID_OSC]);
	debug("PLLM = %d\n", pll_rate[CLOCK_ID_MEMORY]);
	debug("PLLP = %d\n", pll_rate[CLOCK_ID_PERIPH]);
}

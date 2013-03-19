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

/* Tegra2 Clock control functions */

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
 * Clock types that we can use as a source. The Tegra2 has muxes for the
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
	CLOCK_TYPE_PCXTS,
	CLOCK_TYPE_PDCT,

	CLOCK_TYPE_COUNT,
	CLOCK_TYPE_NONE = -1,	/* invalid clock type */
};

/* return 1 if a peripheral ID is in range */
#define clock_type_id_isvalid(id) ((id) >= 0 && \
		(id) < CLOCK_TYPE_COUNT)


enum {
	CLOCK_MAX_MUX	= 4	/* number of source options for each clock */
};

/*
 * Clock source mux for each clock type. This just converts our enum into
 * a list of mux sources for use by the code. Note that CLOCK_TYPE_PCXTS
 * is special as it has 5 sources. Since it also has a different number of
 * bits in its register for the source, we just handle it with a special
 * case in the code.
 */
#define CLK(x) CLOCK_ID_ ## x
static enum clock_id clock_source[CLOCK_TYPE_COUNT] [CLOCK_MAX_MUX] = {
	{ CLK(AUDIO),	CLK(XCPU),	CLK(PERIPH),	CLK(OSC)	},
	{ CLK(MEMORY),	CLK(CGENERAL),	CLK(PERIPH),	CLK(AUDIO)	},
	{ CLK(MEMORY),	CLK(CGENERAL),	CLK(PERIPH),	CLK(OSC)	},
	{ CLK(PERIPH),	CLK(CGENERAL),	CLK(MEMORY),	CLK(NONE)	},
	{ CLK(PERIPH),	CLK(CGENERAL),	CLK(MEMORY),	CLK(OSC)	},
	{ CLK(PERIPH),	CLK(CGENERAL),	CLK(XCPU),	CLK(OSC)	},
	{ CLK(PERIPH),	CLK(DISPLAY),	CLK(CGENERAL),	CLK(OSC)	},
};

/*
 * Clock peripheral IDs which sadly don't match up with PERIPH_ID. This is
 * not in the header file since it is for purely internal use - we want
 * callers to use the PERIPH_ID for all access to peripheral clocks to avoid
 * confusion bewteen PERIPH_ID_... and PERIPHC_...
 *
 * We don't call this CLOCK_PERIPH_ID or PERIPH_CLOCK_ID as it would just be
 * confusing.
 *
 * Note to SOC vendors: perhaps define a unified numbering for peripherals and
 * use it for reset, clock enable, clock source/divider and even pinmuxing
 * if you can.
 */
enum periphc_internal_id {
	/* 0x00 */
	PERIPHC_I2S1,
	PERIPHC_I2S2,
	PERIPHC_SPDIF_OUT,
	PERIPHC_SPDIF_IN,
	PERIPHC_PWM,
	PERIPHC_SPI1,
	PERIPHC_SPI2,
	PERIPHC_SPI3,

	/* 0x08 */
	PERIPHC_XIO,
	PERIPHC_I2C1,
	PERIPHC_DVC_I2C,
	PERIPHC_TWC,
	PERIPHC_0c,
	PERIPHC_10,	/* PERIPHC_SPI1, what is this really? */
	PERIPHC_DISP1,
	PERIPHC_DISP2,

	/* 0x10 */
	PERIPHC_CVE,
	PERIPHC_IDE0,
	PERIPHC_VI,
	PERIPHC_1c,
	PERIPHC_SDMMC1,
	PERIPHC_SDMMC2,
	PERIPHC_G3D,
	PERIPHC_G2D,

	/* 0x18 */
	PERIPHC_NDFLASH,
	PERIPHC_SDMMC4,
	PERIPHC_VFIR,
	PERIPHC_EPP,
	PERIPHC_MPE,
	PERIPHC_MIPI,
	PERIPHC_UART1,
	PERIPHC_UART2,

	/* 0x20 */
	PERIPHC_HOST1X,
	PERIPHC_21,
	PERIPHC_TVO,
	PERIPHC_HDMI,
	PERIPHC_24,
	PERIPHC_TVDAC,
	PERIPHC_I2C2,
	PERIPHC_EMC,

	/* 0x28 */
	PERIPHC_UART3,
	PERIPHC_29,
	PERIPHC_VI_SENSOR,
	PERIPHC_2b,
	PERIPHC_2c,
	PERIPHC_SPI4,
	PERIPHC_I2C3,
	PERIPHC_SDMMC3,

	/* 0x30 */
	PERIPHC_UART4,
	PERIPHC_UART5,
	PERIPHC_VDE,
	PERIPHC_OWR,
	PERIPHC_NOR,
	PERIPHC_CSITE,

	PERIPHC_COUNT,

	PERIPHC_NONE = -1,
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
	TYPE(PERIPHC_SPDIF_IN,	CLOCK_TYPE_PCM),
	TYPE(PERIPHC_PWM,	CLOCK_TYPE_PCXTS),
	TYPE(PERIPHC_SPI1,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_SPI22,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_SPI3,	CLOCK_TYPE_PCMT),

	/* 0x08 */
	TYPE(PERIPHC_XIO,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_I2C1,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_DVC_I2C,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_TWC,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_NONE,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_SPI1,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_DISP1,	CLOCK_TYPE_PDCT),
	TYPE(PERIPHC_DISP2,	CLOCK_TYPE_PDCT),

	/* 0x10 */
	TYPE(PERIPHC_CVE,	CLOCK_TYPE_PDCT),
	TYPE(PERIPHC_IDE0,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_VI,	CLOCK_TYPE_MCPA),
	TYPE(PERIPHC_NONE,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_SDMMC1,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_SDMMC2,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_G3D,	CLOCK_TYPE_MCPA),
	TYPE(PERIPHC_G2D,	CLOCK_TYPE_MCPA),

	/* 0x18 */
	TYPE(PERIPHC_NDFLASH,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_SDMMC4,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_VFIR,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_EPP,	CLOCK_TYPE_MCPA),
	TYPE(PERIPHC_MPE,	CLOCK_TYPE_MCPA),
	TYPE(PERIPHC_MIPI,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_UART1,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_UART2,	CLOCK_TYPE_PCMT),

	/* 0x20 */
	TYPE(PERIPHC_HOST1X,	CLOCK_TYPE_MCPA),
	TYPE(PERIPHC_NONE,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_TVO,	CLOCK_TYPE_PDCT),
	TYPE(PERIPHC_HDMI,	CLOCK_TYPE_PDCT),
	TYPE(PERIPHC_NONE,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_TVDAC,	CLOCK_TYPE_PDCT),
	TYPE(PERIPHC_I2C2,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_EMC,	CLOCK_TYPE_MCPT),

	/* 0x28 */
	TYPE(PERIPHC_UART3,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_NONE,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_VI,	CLOCK_TYPE_MCPA),
	TYPE(PERIPHC_NONE,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_NONE,	CLOCK_TYPE_NONE),
	TYPE(PERIPHC_SPI4,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_I2C3,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_SDMMC3,	CLOCK_TYPE_PCMT),

	/* 0x30 */
	TYPE(PERIPHC_UART4,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_UART5,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_VDE,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_OWR,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_NOR,	CLOCK_TYPE_PCMT),
	TYPE(PERIPHC_CSITE,	CLOCK_TYPE_PCMT),
};

/*
 * This array translates a periph_id to a periphc_internal_id
 *
 * Not present/matched up:
 *	uint vi_sensor;	 _VI_SENSOR_0,		0x1A8
 * 	SPDIF - which is both 0x08 and 0x0c
 *
 */
#define NONE(name) (-1)
#define OFFSET(name, value) PERIPHC_ ## name
static s8 periph_id_to_internal_id[PERIPH_ID_COUNT] = {
	/* Low word: 31:0 */
	NONE(CPU),
	NONE(RESERVED1),
	NONE(RESERVED2),
	NONE(AC97),
	NONE(RTC),
	NONE(TMR),
	PERIPHC_UART1,
	PERIPHC_UART2,	/* and vfir 0x68 */

	/* 0x08 */
	NONE(GPIO),
	PERIPHC_SDMMC2,
	NONE(SPDIF),		/* 0x08 and 0x0c, unclear which to use */
	PERIPHC_I2S1,
	PERIPHC_I2C1,
	PERIPHC_NDFLASH,
	PERIPHC_SDMMC1,
	PERIPHC_SDMMC4,

	/* 0x10 */
	PERIPHC_TWC,
	PERIPHC_PWM,
	PERIPHC_I2S2,
	PERIPHC_EPP,
	PERIPHC_VI,
	PERIPHC_G2D,
	NONE(USBD),
	NONE(ISP),

	/* 0x18 */
	PERIPHC_G3D,
	PERIPHC_IDE0,
	PERIPHC_DISP2,
	PERIPHC_DISP1,
	PERIPHC_HOST1X,
	NONE(VCP),
	NONE(RESERVED30),
	NONE(CACHE2),

	/* Middle word: 63:32 */
	NONE(MEM),
	NONE(AHBDMA),
	NONE(APBDMA),
	NONE(RESERVED35),
	NONE(KBC),
	NONE(STAT_MON),
	NONE(PMC),
	NONE(FUSE),

	/* 0x28 */
	NONE(KFUSE),
	NONE(SBC1),	/* SBC1, 0x34, is this SPI1? */
	PERIPHC_NOR,
	PERIPHC_SPI1,
	PERIPHC_SPI2,
	PERIPHC_XIO,
	PERIPHC_SPI3,
	PERIPHC_DVC_I2C,

	/* 0x30 */
	NONE(DSI),
	PERIPHC_TVO,	/* also CVE 0x40 */
	PERIPHC_MIPI,
	PERIPHC_HDMI,
	PERIPHC_CSITE,
	PERIPHC_TVDAC,
	PERIPHC_I2C2,
	PERIPHC_UART3,

	/* 0x38 */
	NONE(RESERVED56),
	PERIPHC_EMC,
	NONE(USB2),
	NONE(USB3),
	PERIPHC_MPE,
	PERIPHC_VDE,
	NONE(BSEA),
	NONE(BSEV),

	/* Upper word 95:64 */
	NONE(SPEEDO),
	PERIPHC_UART4,
	PERIPHC_UART5,
	PERIPHC_I2C3,
	PERIPHC_SPI4,
	PERIPHC_SDMMC3,
	NONE(PCIE),
	PERIPHC_OWR,

	/* 0x48 */
	NONE(AFI),
	NONE(CORESIGHT),
	NONE(RESERVED74),
	NONE(AVPUCQ),
	NONE(RESERVED76),
	NONE(RESERVED77),
	NONE(RESERVED78),
	NONE(RESERVED79),

	/* 0x50 */
	NONE(RESERVED80),
	NONE(RESERVED81),
	NONE(RESERVED82),
	NONE(RESERVED83),
	NONE(IRAMA),
	NONE(IRAMB),
	NONE(IRAMC),
	NONE(IRAMD),

	/* 0x58 */
	NONE(CRAM2),
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

#ifdef DEBUG
/* return 1 if a peripheral ID is in range and valid */
static int clock_periph_id_isvalid(enum periph_id id)
{
	if (id < PERIPH_ID_FIRST || id >= PERIPH_ID_COUNT)
		printf("Peripheral id %d out of range\n", id);
	else switch (id) {
	case PERIPH_ID_RESERVED1:
	case PERIPH_ID_RESERVED2:
	case PERIPH_ID_RESERVED30:
	case PERIPH_ID_RESERVED35:
	case PERIPH_ID_RESERVED56:
	case PERIPH_ID_RESERVED74:
	case PERIPH_ID_RESERVED76:
	case PERIPH_ID_RESERVED77:
	case PERIPH_ID_RESERVED78:
	case PERIPH_ID_RESERVED79:
	case PERIPH_ID_RESERVED80:
	case PERIPH_ID_RESERVED81:
	case PERIPH_ID_RESERVED82:
	case PERIPH_ID_RESERVED83:
		printf("Peripheral id %d is reserved\n", id);
		break;
	default:
		return 1;
	}
	return 0;
}
#endif

/* Returns a pointer to the clock source register for a peripheral */
static u32 *get_periph_source_reg(enum periph_id periph_id)
{
	struct clk_rst_ctlr *clkrst =
			(struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	enum periphc_internal_id internal_id;

	assert(clock_periph_id_isvalid(periph_id));
	internal_id = periph_id_to_internal_id[periph_id];
	assert(internal_id != -1);
	return &clkrst->crc_clk_src[internal_id];
}

void clock_ll_set_source_divisor(enum periph_id periph_id, int source,
			      unsigned divisor)
{
	u32 *reg = get_periph_source_reg(periph_id);
	u32 value;

	value = readl(reg);
	bf_update(OUT_CLK_SOURCE, value, source);
	bf_update(OUT_CLK_DIVISOR, value, divisor);
	writel(value, reg);
}

void clock_ll_set_source(enum periph_id periph_id, int source)
{
	u32 *reg = get_periph_source_reg(periph_id);

	bf_writel(OUT_CLK_SOURCE, source, reg);
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
 * @param mux_bits	Set to number of bits in mux register: 2 or 4
 * @return mux value (0-4, or -1 if not found)
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

	/* Special case here for the clock with a 4-bit source mux */
	if (type == CLOCK_TYPE_PCXTS)
		*mux_bits = 4;
	else
		*mux_bits = 2;

	for (mux = 0; mux < CLOCK_MAX_MUX; mux++)
		if (clock_source[type][mux] == parent)
			return mux;

	/*
	 * Not found: it might be looking for the 'S' in CLOCK_TYPE_PCXTS
	 * which is not in our table. If not, then they are asking for a
	 * source which this peripheral can't access through its mux.
	 */
	assert(type == CLOCK_TYPE_PCXTS);
	assert(parent == CLOCK_ID_SFROM32KHZ);
	if (type == CLOCK_TYPE_PCXTS && parent == CLOCK_ID_SFROM32KHZ);
		return 4;	/* mux value for this clock */

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
	if (mux_bits == 4)
		bf_writel(OUT_CLK_SOURCE4, source, reg);
	else
		bf_writel(OUT_CLK_SOURCE, source, reg);
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
	u32 *clk = &clkrst->crc_clk_out_enb[PERIPH_REG(periph_id)];
	u32 reg;

	/* Enable/disable the clock to this peripheral */
	assert(clock_periph_id_isvalid(periph_id));
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
	u32 *reset = &clkrst->crc_rst_dev[PERIPH_REG(periph_id)];
	u32 reg;

	/* Enable/disable reset to the peripheral */
	assert(clock_periph_id_isvalid(periph_id));
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

	/* Form the mask, which depends on the cpu chosen. Tegra2 has 2 */
	assert(cpu >= 0 && cpu < 2);
	mask = which << cpu;

	/* either enable or disable those reset for that CPU */
	if (reset)
		writel(mask, &clkrst->crc_cpu_cmplx_set);
	else
		writel(mask, &clkrst->crc_cpu_cmplx_clr);
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
 * Please see Tegra TRM section 5.3 to get the detail for PLL Programming
 *
 * @param n PLL feedback divider(DIVN)
 * @param m PLL input divider(DIVN)
 * @param p post divider(DIVP)
 * @param cpcon base PLL charge pump(CPCON)
 * @param outa PLLx_OUT1 & PLLx_OUT2 config (0 = no driven clock)
 * @parma outb PLLx_OUT3 & PLLx_OUT4 config (0 = no driven clock)
 * @return 0 if ok, -1 on error (the requested PLL cannot be overriden)
 */
static int clock_set_rate(enum clock_id clkid, u32 n, u32 m, u32 p, u32 cpcon,
			u32 outa, u32 outb)
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

	/* OUTA */
	if (outa)
		writel(outa, &pll->pll_out);

	/* OUTB */
	if (outb)
		writel(outb, &pll->reserved);

	/* Disable BYPASS */
	bf_update(PLL_BYPASS, base_reg, 0);
	writel(base_reg, &pll->pll_base);

	return 0;
}

/**
 * Set the lock for each PLL clock.
 *
 * @parma misc_locken LOCK enable bit mask in pll misc reg.
 * @parma base_stat LOCK status bit mask in pll base reg.
 * @return 0 if ok, -1 on error (the requested PLL cannot be overriden)
 */
static int clock_set_lock(enum clock_id clkid, u32 misc_locken, u32 base_stat)
{
	u32 base_reg;
	u32 misc_reg;
	struct clk_pll *pll;

	if ((pll = get_pll(clkid)) == NULL)
		return -1;

	misc_reg = readl(&pll->pll_misc);
	misc_reg |= misc_locken;
	writel(misc_reg, &pll->pll_misc);
	do {
		base_reg = readl(&pll->pll_base);
	} while ((base_reg & base_stat) == 0);

	return 0;
}

#define PLLC_LOCK_STATE			(1 << 27)
#define PLLC_LOCK_ENABLE		(1 << 18)
#define PLLP_LOCK_STATE			(1 << 27)
#define PLLP_LOCK_ENABLE		(1 << 18)

#define PLL_OUT_RSTN_RESET_DISABLE	(1 << 0)
#define PLL_OUT_CLKEN_ENABLE		(1 << 1)
#define PLL_OUT_OVERRIDE_ENABLE		(1 << 2)
#define PLL_OUT_U71_RATIO_15		(0x1c << 8)
#define PLL_OUT_U71_RATIO_9		(0x10 << 8)
#define PLL_OUT_U71_RATIO_6		(0x08 << 8)
#define PLL_OUT_U71_RATIO_4		(0x06 << 8)
#define PLL_OUT_U71_RATIO_2_5		(0x03 << 8)

#define PLLP_OUT1_RSTN_RESET_DISABLE	(PLL_OUT_RSTN_RESET_DISABLE <<  0)
#define PLLP_OUT1_CLKEN_ENABLE		(PLL_OUT_CLKEN_ENABLE       <<  0)
#define PLLP_OUT1_OVERRIDE_ENABLE	(PLL_OUT_OVERRIDE_ENABLE    <<  0)
#define PLLP_OUT1_RATIO_15		(PLL_OUT_U71_RATIO_15       <<  0)

#define PLLP_OUT2_RSTN_RESET_DISABLE	(PLL_OUT_RSTN_RESET_DISABLE << 16)
#define PLLP_OUT2_CLKEN_ENABLE		(PLL_OUT_CLKEN_ENABLE       << 16)
#define PLLP_OUT2_OVERRIDE_ENABLE	(PLL_OUT_OVERRIDE_ENABLE    << 16)
#define PLLP_OUT2_RATIO_9		(PLL_OUT_U71_RATIO_9        << 16)

#define PLLP_OUT3_RSTN_RESET_DISABLE	(PLL_OUT_RSTN_RESET_DISABLE <<  0)
#define PLLP_OUT3_CLKEN_ENABLE		(PLL_OUT_CLKEN_ENABLE       <<  0)
#define PLLP_OUT3_OVERRIDE_ENABLE	(PLL_OUT_OVERRIDE_ENABLE    <<  0)
#define PLLP_OUT3_RATIO_6		(PLL_OUT_U71_RATIO_6        <<  0)

#define PLLP_OUT4_RSTN_RESET_DISABLE	(PLL_OUT_RSTN_RESET_DISABLE << 16)
#define PLLP_OUT4_CLKEN_ENABLE		(PLL_OUT_CLKEN_ENABLE       << 16)
#define PLLP_OUT4_OVERRIDE_ENABLE	(PLL_OUT_OVERRIDE_ENABLE    << 16)
#define PLLP_OUT4_RATIO_4		(PLL_OUT_U71_RATIO_4        << 16)

#define PLLC_OUT1_RSTN_RESET_DISABLE	(PLL_OUT_RSTN_RESET_DISABLE <<  0)
#define PLLC_OUT1_CLKEN_ENABLE		(PLL_OUT_CLKEN_ENABLE       <<  0)
#define PLLC_OUT1_RATIO_2_5		(PLL_OUT_U71_RATIO_2_5      <<  0)

void common_pll_init(void)
{
	u32 couta, pouta, poutb;

	/*
	 * Vco = (Fi/M)*N
	 * Fo = Vco / (2**P)
	 *    = Fi * N / M / 2**P
	 *
	 * PLLC output frequency set to 600Mhz
	 *	- PLLC_OUT1 to 240Mhz yields 2.5(=0x3) as divisor value
	 * PLLP output frequency set to 216Mhz: refclk is always 432MHz
	 *	- PLLP_OUT1 to  28.8 Mhz: divisor = 15 = 0x1c
	 *	- PLLP_OUT2 to 216.0 Mhz: divisor =  9 = 0x10
	 *	- PLLP_OUT3 to  72.0 Mhz: divisor =  6 = 0x0a
	 *	- PLLP_OUT4 to 	24.0 Mhz: divisor =  4 = 0x06
	 */
	pouta = PLLP_OUT1_RSTN_RESET_DISABLE | PLLP_OUT1_CLKEN_ENABLE |
		PLLP_OUT1_OVERRIDE_ENABLE    | PLLP_OUT1_RATIO_15 |
		PLLP_OUT2_RSTN_RESET_DISABLE | PLLP_OUT2_CLKEN_ENABLE |
		PLLP_OUT2_OVERRIDE_ENABLE    | PLLP_OUT2_RATIO_9;
	poutb = PLLP_OUT3_RSTN_RESET_DISABLE | PLLP_OUT3_CLKEN_ENABLE |
		PLLP_OUT3_OVERRIDE_ENABLE    | PLLP_OUT3_RATIO_6 |
		PLLP_OUT4_RSTN_RESET_DISABLE | PLLP_OUT4_CLKEN_ENABLE |
		PLLP_OUT4_OVERRIDE_ENABLE    | PLLP_OUT4_RATIO_4;
	couta = PLLC_OUT1_RSTN_RESET_DISABLE | PLLC_OUT1_CLKEN_ENABLE |
		PLLC_OUT1_RATIO_2_5;

	switch (clock_get_rate(CLOCK_ID_OSC)) {
	case 13000000: /* OSC is 13Mhz */
		clock_set_rate(CLOCK_ID_PERIPH,   432, 13, 1, 8, pouta, poutb);
		clock_set_rate(CLOCK_ID_CGENERAL, 600, 13, 0, 8, couta, 0);
		break;
	case 19200000: /* OSC is 19.2Mhz. Just best approximation. */
		clock_set_rate(CLOCK_ID_PERIPH,    90,  4, 1, 1, pouta, poutb);
		clock_set_rate(CLOCK_ID_CGENERAL, 600, 19, 0, 8, couta, 0);
		break;
	case 12000000: /* OSC is 12Mhz */
		clock_set_rate(CLOCK_ID_PERIPH,   432, 12, 1, 8, pouta, poutb);
		clock_set_rate(CLOCK_ID_CGENERAL, 600, 12, 0, 8, couta, 0);
		break;
	case 26000000: /* OSC is 26Mhz */
		clock_set_rate(CLOCK_ID_PERIPH,   432, 26, 1, 8, pouta, poutb);
		clock_set_rate(CLOCK_ID_CGENERAL, 600, 26, 0, 8, couta, 0);
		break;
	}

	clock_set_lock(CLOCK_ID_PERIPH,   PLLP_LOCK_ENABLE, PLLP_LOCK_STATE);
	clock_set_lock(CLOCK_ID_CGENERAL, PLLC_LOCK_ENABLE, PLLC_LOCK_STATE);
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

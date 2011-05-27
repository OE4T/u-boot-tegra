/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2010,2011 NVIDIA Corporation <www.nvidia.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/tegra2.h>
#include <asm/arch/sys_proto.h>

#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/uart.h>
#include <asm/arch/gpio.h>
#include <asm/arch/usb.h>


/* The fields for USB PLLU configuration parameters */
struct usb_pll_params {
	unsigned divn; 		/* PLL feedback divider */
	unsigned divm; 		/* PLL input divider */
	unsigned divp; 		/* post divider (2^n) */
	unsigned cpcon;		/* Base PLLC charge pump setup control */
	unsigned lfcon;		/* Base PLLC loop filter setup control */
	u8 enable_delay_count;	/* Pll-U Enable Delay Count */
	u8 stable_count;	/* PLL-U Stable count */
	u8 active_delay_count;	/* Pll-U Active delay count */
	u8 xtal_freq_count;	/* PLL-U Xtal frequency count */
	unsigned debounce_a_time; /* 10ms delay for BIAS_DEBOUNCE_A */
	unsigned bias_time;  	/* 20us delay after bias cell op */
};

/*
 * This table has USB timing parameters for each Oscillator frequency we
 * support. There are four sets of values:
 *
 * 1. PLLU configuration information (reference clock is osc/clk_m and
 * PLLU-FOs are fixed at 12MHz/60MHz/480MHz).
 *
 *  Reference frequency     13.0MHz      19.2MHz      12.0MHz      26.0MHz
 *  ----------------------------------------------------------------------
 *      DIVN                960 (0x3c0)  200 (0c8)    960 (3c0h)   960 (3c0)
 *      DIVM                13 (0d)      4 (04)       12 (0c)      26 (1a)
 * Filter frequency (MHz)   1            4.8          6            2
 * CPCON                    1100b        0011b        1100b        1100b
 * LFCON0                   0            0            0            0
 *
 * 2. PLL CONFIGURATION & PARAMETERS for different clock generators:
 *
 * Reference frequency     13.0MHz         19.2MHz         12.0MHz     26.0MHz
 * ---------------------------------------------------------------------------
 * PLLU_ENABLE_DLY_COUNT   02 (0x02)       03 (03)         02 (02)     04 (04)
 * PLLU_STABLE_COUNT       51 (33)         75 (4B)         47 (2F)    102 (66)
 * PLL_ACTIVE_DLY_COUNT    05 (05)         06 (06)         04 (04)     09 (09)
 * XTAL_FREQ_COUNT        127 (7F)        187 (BB)        118 (76)    254 (FE)
 *
 * 3. Debounce values IdDig, Avalid, Bvalid, VbusValid, VbusWakeUp, and
 * SessEnd. Each of these signals have their own debouncer and for each of
 * those one out of two debouncing times can be chosen (BIAS_DEBOUNCE_A or
 * BIAS_DEBOUNCE_B).
 *
 * The values of DEBOUNCE_A and DEBOUNCE_B are calculated as follows:
 *    0xffff -> No debouncing at all
 *    <n> ms = <n> *1000 / (1/19.2MHz) / 4
 *
 * So to program a 1 ms debounce for BIAS_DEBOUNCE_A, we have:
 * BIAS_DEBOUNCE_A[15:0] = 1000 * 19.2 / 4  = 4800 = 0x12c0
 *
 * We need to use only DebounceA for BOOTROM. We dont need the DebounceB
 * values, so we can keep those to default.
 *
 * 4. The 20 microsecond delay after bias cell operation.
 */
static const struct usb_pll_params usb_pll[CLOCK_OSC_FREQ_COUNT] = {
	/* DivN, DivM, DivP, CPCON, LFCON, Delays             Debounce, Bias */
	{ 0x3C0, 0x0D, 0x00, 0xC,   0,  0x02, 0x33, 0x05, 0x7F, 0x7EF4, 5 },
	{ 0x0C8, 0x04, 0x00, 0x3,   0,  0x03, 0x4B, 0x06, 0xBB, 0xBB80, 7 },
	{ 0x3C0, 0x0C, 0x00, 0xC,   0,  0x02, 0x2F, 0x04, 0x76, 0x7530, 5 },
	{ 0x3C0, 0x1A, 0x00, 0xC,   0,  0x04, 0x66, 0x09, 0xFE, 0xFDE8, 9 }
};

/* UTMIP Idle Wait Delay */
static const u8 utmip_idle_wait_delay = 17;

/* UTMIP Elastic limit */
static const u8 utmip_elastic_limit = 16;

/* UTMIP High Speed Sync Start Delay */
static const u8 utmip_hs_sync_start_delay = 9;


void usb1_set_host_mode(void)
{
	struct usb_ctlr *usbctlr = (struct usb_ctlr *)NV_PA_USB1_BASE;

	/* Check whether remote host from USB1 is driving VBus */
	if (bf_readl(VBUS_VLD_STS, &usbctlr->phy_vbus_sensors))
		return;

	/*
	 * If not driving, we set GPIO USB1_VBus_En. Seaboard platform uses
	 * PAD SLXK (GPIO D.00) as USB1_VBus_En Config as GPIO
	 */
	gpio_direction_output(GPIO_PD0, 1);

	/* Z_SLXK = 0, normal, not tristate */
	pinmux_tristate_disable(PINGRP_SLXK);
}

void usbf_reset_controller(enum periph_id id, struct usb_ctlr *usbctlr)
{
	/* Reset the USB controller with 2us delay */
	reset_periph(id, 2);

	/*
	 * Set USB1_NO_LEGACY_MODE to 1, Registers are accessible under
	 * base address
	 */
	if (id == PERIPH_ID_USBD)
		bf_writel(USB1_NO_LEGACY_MODE, NO_LEGACY_MODE,
			&usbctlr->usb1_legacy_ctrl);

	/* Put UTMIP1/3 in reset */
	bf_writel(UTMIP_RESET, 1, &usbctlr->susp_ctrl);

	/* Set USB3 to use UTMIP PHY */
	if (id == PERIPH_ID_USB3)
		bf_writel(UTMIP_PHY_ENB, 1, &usbctlr->susp_ctrl);

	/*
	 * TODO: where do we take the USB1 out of reset? The old code would
	 * take USB3 out of reset, but not USB1. This code doesn't do either.
	 */
}

/* set up the USB controller with the parameters provided */
static void init_usb_controller(enum periph_id id, struct usb_ctlr *usbctlr,
		const struct usb_pll_params *params)
{
	u32 val;
	int loop_count;

	clock_enable(id);

	/* Reset the usb controller */
	usbf_reset_controller(id, usbctlr);

	/* Stop crystal clock by setting UTMIP_PHY_XTAL_CLOCKEN low */
	bf_clearl(UTMIP_PHY_XTAL_CLOCKEN, &usbctlr->utmip_misc_cfg1);

	/* Follow the crystal clock disable by >100ns delay */
	udelay(1);

	/*
	 * To Use the A Session Valid for cable detection logic, VBUS_WAKEUP
	 * mux must be switched to actually use a_sess_vld threshold.
	 */
	if (id == PERIPH_ID_USBD)
		bf_enum_writel(VBUS_SENSE_CTL, A_SESS_VLD,
				&usbctlr->usb1_legacy_ctrl);

	/*
	 * PLL Delay CONFIGURATION settings. The following parameters control
	 * the bring up of the plls.
	 */
	val = readl(&usbctlr->utmip_misc_cfg1);
	bf_update(UTMIP_PLLU_STABLE_COUNT, val, params->stable_count);
	bf_update(UTMIP_PLL_ACTIVE_DLY_COUNT, val,
		     params->active_delay_count);
	writel(val, &usbctlr->utmip_misc_cfg1);

	/* Set PLL enable delay count and crystal frequency count */
	val = readl(&usbctlr->utmip_pll_cfg1);
	bf_update(UTMIP_PLLU_ENABLE_DLY_COUNT, val,
		     params->enable_delay_count);
	bf_update(UTMIP_XTAL_FREQ_COUNT, val, params->xtal_freq_count);
	writel(val, &usbctlr->utmip_pll_cfg1);

	/* Setting the tracking length time */
	bf_writel(UTMIP_BIAS_PDTRK_COUNT, params->bias_time,
			&usbctlr->utmip_bias_cfg1);

	/* Program debounce time for VBUS to become valid */
	bf_writel(UTMIP_DEBOUNCE_CFG0, params->debounce_a_time,
			&usbctlr->utmip_debounce_cfg0);

	/* Set UTMIP_FS_PREAMBLE_J to 1 */
	bf_writel(UTMIP_FS_PREAMBLE_J, 1, &usbctlr->utmip_tx_cfg0);

	/* Disable battery charge enabling bit */
	bf_writel(UTMIP_PD_CHRG, 1, &usbctlr->utmip_bat_chrg_cfg0);

	/* Set UTMIP_XCVR_LSBIAS_SEL to 0 */
	bf_writel(UTMIP_XCVR_LSBIAS_SE, 0, &usbctlr->utmip_xcvr_cfg0);

	/* Set bit 3 of UTMIP_SPARE_CFG0 to 1 */
	bf_writel(FUSE_SETUP_SEL, 1, &usbctlr->utmip_spare_cfg0);

	/*
	 * Configure the UTMIP_IDLE_WAIT and UTMIP_ELASTIC_LIMIT
	 * Setting these fields, together with default values of the
	 * other fields, results in programming the registers below as
	 * follows:
	 *         UTMIP_HSRX_CFG0 = 0x9168c000
	 *         UTMIP_HSRX_CFG1 = 0x13
	 */

	/* Set PLL enable delay count and Crystal frequency count */
	val = readl(&usbctlr->utmip_hsrx_cfg0);
	bf_update(UTMIP_IDLE_WAIT, val, utmip_idle_wait_delay);
	bf_update(UTMIP_ELASTIC_LIMIT, val, utmip_elastic_limit);
	writel(val, &usbctlr->utmip_hsrx_cfg0);

	/* Configure the UTMIP_HS_SYNC_START_DLY */
	bf_writel(UTMIP_HS_SYNC_START_DLY, utmip_hs_sync_start_delay,
			&usbctlr->utmip_hsrx_cfg1);

	/* Preceed the crystal clock disable by >100ns delay. */
	udelay(1);

	/* Resuscitate crystal clock by setting UTMIP_PHY_XTAL_CLOCKEN */
	bf_writel(UTMIP_PHY_XTAL_CLOCKEN, 1, &usbctlr->utmip_misc_cfg1);

	/* Finished the per-controller init. */

	/* De-assert UTMIP_RESET to bring out of reset. */
	bf_clearl(UTMIP_RESET, &usbctlr->susp_ctrl);

	/* Wait for the phy clock to become valid in 100 ms */
	for (loop_count = 100000; loop_count != 0; loop_count--) {
		if (bf_readl(USB_PHY_CLK_VALID, &usbctlr->susp_ctrl))
			break;
		udelay(1);
	}
}

static void power_up_port(struct usb_ctlr *usbctlr)
{
	u32 val;

	/* Deassert power down state */
	val = readl(&usbctlr->utmip_xcvr_cfg0);
	bf_update(UTMIP_FORCE_PD_POWERDOWN, val, 0);
	bf_update(UTMIP_FORCE_PD2_POWERDOWN, val, 0);
	bf_update(UTMIP_FORCE_PDZI_POWERDOWN, val, 0);
	writel(val, &usbctlr->utmip_xcvr_cfg0);

	val = readl(&usbctlr->utmip_xcvr_cfg1);
	bf_update(UTMIP_FORCE_PDDISC_POWERDOWN, val, 0);
	bf_update(UTMIP_FORCE_PDCHRP_POWERDOWN, val, 0);
	bf_update(UTMIP_FORCE_PDDR_POWERDOWN, val, 0);
	writel(val, &usbctlr->utmip_xcvr_cfg1);
}

void board_usb_init(void)
{
	enum clock_osc_freq freq;
	const struct usb_pll_params *params;
	struct usb_ctlr *usbctlr;
	unsigned stable_time;

	/* Get the Oscillator frequency */
	freq = clock_get_osc_freq();

	/* Enable PLL U for USB */
	params = &usb_pll[freq];
	stable_time = clock_start_pll(CLOCK_ID_USB,
		params->divm, params->divn, params->divp, params->cpcon,
		params->lfcon);
	/* TODO: what should we do with stable_time? */

	/* Set up our two ports */
	usbctlr = (struct usb_ctlr *)NV_PA_USB1_BASE;
	init_usb_controller(PERIPH_ID_USBD, usbctlr, params);

	usbctlr = (struct usb_ctlr *)NV_PA_USB3_BASE;
	init_usb_controller(PERIPH_ID_USB3, usbctlr, params);

	/* Disable ICUSB FS/LS transceiver */
	bf_writel(IC_ENB1, 0, &usbctlr->icusb_ctrl);

	/* Select UTMI parallel interface */
	bf_writel(PTS, PTS_UTMI, &usbctlr->port_sc1);
	bf_writel(STS, 0, &usbctlr->port_sc1);

	power_up_port(usbctlr);

#ifdef CONFIG_TEGRA2_USB1_HOST
	usb1_set_host_mode();
#endif
}


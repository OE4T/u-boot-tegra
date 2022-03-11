// SPDX-License-Identifier: GPL-2.0+
/* Copyright (c) 2021 NVIDIA Corporation */

/* Tegra210 XUSB pad ctl init, move to padctl driver later */

#include <common.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch-tegra/clk_rst.h>
#include <linux/delay.h>
#include "xusb-macros.h"
#include "tegra_xhci.h"

int tegra_uphy_pll_enable(void)
{
	u32 val;
	unsigned long start;

	debug("%s: entry\n", __func__);

	/* UPHY PLL init from TRM, page 1391 */
	/* 1. Deassert PLL/Lane resets */
	reset_set_enable(PERIPH_ID_PEX_USB_UPHY, 0);

	/* Set cal values and overrides
	 * (0x364, b27:4)- UPHY_PLL_P0_CTL_2_0[PLL0_CAL_CTRL] = 0x136
	 * (0x370, b23:16)- UPHY_PLL_P0_CTL_5_0[PLL0_DCO_CTRL] = 0x2A
	 * (0x360, b4)- UPHY_PLL_P0_CTL_1_0[PLL0_PWR_OVRD] = 1 (Default)
	 * (0x364, b2)- UPHY_PLL_P0_CTL_2_0[PLL0_CAL_OVRD] = 1
	 * (0x37C, b15)- UPHY_PLL_P0_CTL_8_0[PLL0_RCAL_OVRD] = 1
	 */
	val = (0x136 << 4);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_2, val);

	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_5, val);
	val &= 0x0000FFFF;
	val |= (0x2A << 16);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_5, val);

	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_1, val);
	val |= (1 << 4);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_1, val);

	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_2, val);
	val |= (1 << 2);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_2, val);

	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_8, val);
	val |= (1 << 15);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_8, val);

	/* 2. Other reg bits (P0_CTL4, P0_CTL_1 MDIV/NDIV/PSDIV) defaults OK
	 * (0x360, b0)- UPHY_PLL_P0_CTL_1_0[PLL0_IDDQ] = 0
	 * (0x360, b2:1)- UPHY_PLL_P0_CTL_1_0[PLL0_SLEEP] = 00
	 */
	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_1, val);
	val &= ~(7 << 0);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_1, val);

	/* 3. Wait 100 ns */
	udelay(1);

	/* 4. Calibration
	 * (0x364, b0)- UPHY_PLL_P0_CTL_2_0[PLL0_CAL_EN] = 1
	 * (0x364, b1)- Wait for UPHY_PLL_P0_CTL_2_0[PLL0_CAL_DONE] == 1
	 * (0x364, b0)- UPHY_PLL_P0_CTL_2_0[PLL0_CAL_EN] = 0
	 * (0x364), b1- Wait for UPHY_PLL_P0_CTL_2_0[PLL0_CAL_DONE] == 0
	 */
	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_2, val);
	val |= (1 << 0);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_2, val);

	debug("%s: Waiting for calibration done ...", __func__);
	start = get_timer(0);

	while (get_timer(start) < 250) {
		NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_2, val);
		if (val & XUSB_PADCTL_UPHY_PLL_P0_CTL2_CAL_DONE)
			break;
	}

	if (!(val & XUSB_PADCTL_UPHY_PLL_P0_CTL2_CAL_DONE)) {
		debug("  timeout\n");
		return -ETIMEDOUT;
	}
	debug("done\n");

	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_2, val);
	val &= ~(1 << 0);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_2, val);

	debug("%s: Waiting for calibration done == 0 ...", __func__);
	start = get_timer(0);

	while (get_timer(start) < 250) {
		NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_2, val);
		if ((val & XUSB_PADCTL_UPHY_PLL_P0_CTL2_CAL_DONE) == 0)
			break;
	}

	if (val & XUSB_PADCTL_UPHY_PLL_P0_CTL2_CAL_DONE) {
		debug("  timeout\n");
		return -ETIMEDOUT;
	}
	debug("done\n");

	/* 5. Enable the PLL (20 µs Lock time)
	 * (0x360, b3)- UPHY_PLL_P0_CTL_1_0[PLL0_ENABLE] = 1
	 * (0x360, b15)- Wait for UPHY_PLL_P0_CTL_1_0[PLL0_LOCKDET_STATUS] == 1
	 */
	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_1, val);
	val |= (1 << 3);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_1, val);

	debug("%s: Waiting for lock detect ...", __func__);
	start = get_timer(0);

	while (get_timer(start) < 250) {
		NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_1, val);
		if (val & XUSB_PADCTL_UPHY_PLL_P0_CTL1_PLL0_LOCKDET)
			break;
	}

	if (!(val & XUSB_PADCTL_UPHY_PLL_P0_CTL1_PLL0_LOCKDET)) {
		debug("  timeout\n");
		return -ETIMEDOUT;
	}
	debug("done\n");

	/* 6. RCAL
	 * (0x37C, b12)- UPHY_PLL_P0_CTL_8_0[PLL0_RCAL_EN] = 1
	 * (0x37C, b13)- UPHY_PLL_P0_CTL_8_0[PLL0_RCAL_CLK_EN] = 1
	 * (0x37C, b31)- Wait for UPHY_PLL_P0_CTL_8_0[PLL0_RCAL_DONE] == 1
	 * (0x37C, b12)- UPHY_PLL_P0_CTL_8_0[PLL0_RCAL_EN] = 0
	 * (0x37C, b31)- Wait for UPHY_PLL_P0_CTL_8_0[PLL0_RCAL_DONE] == 0
	 * (0x37C, b13)- UPHY_PLL_P0_CTL_8_0[PLL0_RCAL_CLK_EN] = 0
	 */
	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_8, val);
	val |= (3 << 12);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_8, val);

	debug("%s: Waiting for rcal done ...", __func__);
	start = get_timer(0);

	while (get_timer(start) < 250) {
		NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_8, val);
		if (val & XUSB_PADCTL_UPHY_PLL_P0_CTL8_PLL0_RCAL_DONE)
			break;
	}

	if (!(val & XUSB_PADCTL_UPHY_PLL_P0_CTL8_PLL0_RCAL_DONE)) {
		debug("  timeout\n");
		return -ETIMEDOUT;
	}
	debug("done\n");

	val &= ~(1 << 12);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_8, val);

	debug("%s: Waiting for rcal done == 0 ...", __func__);
	start = get_timer(0);

	while (get_timer(start) < 250) {
		NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_8, val);
		if ((val & XUSB_PADCTL_UPHY_PLL_P0_CTL8_PLL0_RCAL_DONE) == 0)
			break;
	}

	if (val & XUSB_PADCTL_UPHY_PLL_P0_CTL8_PLL0_RCAL_DONE) {
		debug("  timeout\n");
		return -ETIMEDOUT;
	}
	debug("done\n");

	val &= ~(1 << 13);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_8, val);

	/* Finally, lift PADCTL overrides
	 * (0x360, b4)- UPHY_PLL_P0_CTL_1_0[PLL0_PWR_OVRD] = 0
	 * (0x364, b2)- UPHY_PLL_P0_CTL_2_0[PLL0_CAL_OVRD] = 0
	 * (0x37C, b15)- UPHY_PLL_P0_CTL_8_0[PLL0_RCAL_OVRD] = 0
	 */
	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_1, val);
	val &= ~(1 << 4);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_1, val);

	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_2, val);
	val &= ~(1 << 2);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_2, val);

	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_8, val);
	val &= ~(1 << 15);
	NV_XUSB_PADCTL_WRITE(UPHY_PLL_P0_CTL_8, val);

	return 0;
}

void padctl_usb3_port_init(int board_id)
{
	int i, port;
	u32 reg, val;
	debug("%s: entry\n", __func__);

	/* Set SS_PORT_MAP, SS PORT0 maps to USB2 PORT1 on all boards */
	reg = XUSB_PADCTL_SS_PORT_MAP_0;
	NV_XUSB_PADCTL_READ(SS_PORT_MAP, val);
	debug("%s: reg %X: val read = %X\n", __func__, reg, val);
	val &= ~SS_PORT_MAP(0, SS_PORT_MAP_PORT_DISABLED);
	val |= SS_PORT_MAP(0, 1);
	/* Set SS PORT1 maps to USB2 PORT2 on TX1 */
	if (board_id == 2371) {
		val &= ~SS_PORT_MAP(1, SS_PORT_MAP_PORT_DISABLED);
		val |= SS_PORT_MAP(1, 2);
	}
	NV_XUSB_PADCTL_WRITE(SS_PORT_MAP, val);
	debug("%s: Wrote 0x%08X to 0x7009F%03X\n", __func__, val, reg);

	/* Set ELPG_PROGRAM_1 clamp bits next */
	reg = XUSB_PADCTL_ELPG_PROGRAM_1_0;
	NV_XUSB_PADCTL_READ(ELPG_PROGRAM_1, val);
	debug("%s: reg %X: val read = %X\n", __func__, reg, val);

	/* Unclamp SSP0 & SSP3 */
	port = 0;
	val &= ~SSPX_ELPG_CLAMP_EN(port);
	val &= ~SSPX_ELPG_CLAMP_EN_EARLY(port);
	val &= ~SSPX_ELPG_VCORE_DOWN(port);
	port = 3;
	val &= ~SSPX_ELPG_CLAMP_EN(port);
	val &= ~SSPX_ELPG_CLAMP_EN_EARLY(port);
	val &= ~SSPX_ELPG_VCORE_DOWN(port);
	/* Unclamp PORT1 on TX1 */
	if (board_id == 2371) {
		port = 1;
		val &= ~SSPX_ELPG_CLAMP_EN(port);
		val &= ~SSPX_ELPG_CLAMP_EN_EARLY(port);
		val &= ~SSPX_ELPG_VCORE_DOWN(port);
	}
	val &= ~AUX_MUX_LP0_CLAMP_EN;
	val &= ~AUX_MUX_LP0_CLAMP_EN_EARLY;
	val &= ~AUX_MUX_LP0_VCORE_DOWN;
	NV_XUSB_PADCTL_WRITE(ELPG_PROGRAM_1, val);
	debug("%s: Wrote 0x%08X to 0x7009F%03X\n", __func__, val, reg);

	/* Set USB3_PAD_MUX, XUSB_PADCTL reset clears PCIE IDDQ bits 8-0 */
	reg = XUSB_PADCTL_USB3_PAD_MUX_0;
	NV_XUSB_PADCTL_READ(USB3_PAD_MUX, val);
	debug("%s: reg %X: val read = %X\n", __func__, reg, val);
	for (i = 0; i < 7; i++)
		val |= FORCE_PCIE_PAD_IDDQ_DISABLE(i);
	val |= FORCE_SATA_PAD_IDDQ_DISABLE(0);
	NV_XUSB_PADCTL_WRITE(USB3_PAD_MUX, val);
	debug("%s: Wrote 0x%08X to 0x7009F%03X\n", __func__, val, reg);
}

void pad_trk_init(u32 hs_squelch)
{
	u32 val;

	debug("%s: entry\n", __func__);

	/* Enable tracking clock */
	clock_enable(PERIPH_ID_USB2_TRK);

	NV_XUSB_PADCTL_READ(USB2_PAD_MUX, val);
	val &= ~(3 << USB2_BIAS_PAD);
	val |= (1 << USB2_BIAS_PAD);			/* 1 = XUSB */
	NV_XUSB_PADCTL_WRITE(USB2_PAD_MUX, val);

	NV_XUSB_PADCTL_READ(USB2_BIAS_PAD_CTL_1, val);
	val &= ~USB2_TRK_START_TIMER(~0);
	val |= USB2_TRK_START_TIMER(0x1E);		/* START TIMER = 30 */
	val &= ~USB2_TRK_DONE_RESET_TIMER(~0);
	val |= USB2_TRK_DONE_RESET_TIMER(0x0A);		/* DONE TIMER = 10 */
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_1, val);

	NV_XUSB_PADCTL_READ(USB2_BIAS_PAD_CTL_0, val);

	val &= ~BIAS_PAD_PD;				/* PD = 0 */
	val &= ~HS_DISCON_LEVEL(~0);
	val |= HS_DISCON_LEVEL(0x7);
	val &= ~HS_SQUELCH_LEVEL(~0);
	val |= HS_SQUELCH_LEVEL(hs_squelch);
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_0, val);

	udelay(1);

	NV_XUSB_PADCTL_READ(USB2_BIAS_PAD_CTL_1, val);
	val &= ~USB2_PD_TRK;				/* Enable tracking */
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_1, val);
}

void do_padctl_usb2_config(void)
{
	int i, index;
	u32 reg, val;
	u32 *xusbpadctl = (u32 *)NV_ADDRESS_MAP_APB_XUSB_PADCTL_BASE;

	u32 hs_curr_level[4];
	u32 hs_curr_level_offset = 0;
	u32 hs_squelch, hs_term_range_adj, rpd_ctrl;
	u32 fuse_calib, fuse_calib_ext;

	debug("%s: entry\n", __func__);

	/* Set USB2 BIAS pad and USB2 OTG Port0 pad to 'XUSB' */
	reg = XUSB_PADCTL_USB2_PAD_MUX_0;
	NV_XUSB_PADCTL_READ(USB2_PAD_MUX, val);
	debug("%s: reg %X: val read = %X\n", __func__, reg, val);
	val &= ~(XUSB_PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_MASK <<
		  XUSB_PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_SHIFT);
	val |= XUSB_PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_XUSB <<
		XUSB_PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_SHIFT;

	val &= ~(XUSB_PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_MASK <<
		  XUSB_PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_SHIFT);
	val |= XUSB_PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_XUSB <<
		XUSB_PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_SHIFT;
	NV_XUSB_PADCTL_WRITE(USB2_PAD_MUX, val);
	debug("%s: Wrote 0x%08X to 0x7009F%03X\n", __func__, val, reg);

	/* Clear all SS/USB2/HSIC wakeup event bits */
	val = 0x41E00780;
	reg = XUSB_PADCTL_ELPG_PROGRAM_0_0;
	NV_XUSB_PADCTL_WRITE(ELPG_PROGRAM_0, val);
	debug("%s: Wrote 0x%08X to 0x7009F%03X\n", __func__, val, reg);

	/* Set up USB2 pads */
	reg = XUSB_PADCTL_USB2_PORT_CAP_0;
	NV_XUSB_PADCTL_READ(USB2_PORT_CAP, val);
	debug("%s: reg %X: val read = %X\n", __func__, reg, val);

	/* PORT0 = OTG */
	index = 0;
	val &= ~XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_MASK(index);
	val |= XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_OTG(index);
	/* PORT1 = HOST */
	index++;
	val &= ~XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_MASK(index);
	val |= XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_HOST(index);
	/* PORT2 = HOST */
	index++;
	val &= ~XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_MASK(index);
	val |= XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_HOST(index);
	/* PORT3 = DISABLED */
	index++;
	val &= ~XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_MASK(index);
	val |= XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_DISABLED(index);
	NV_XUSB_PADCTL_WRITE(USB2_PORT_CAP, val);
	debug("%s: Wrote 0x%08X to 0x7009F%03X\n", __func__, val, reg);

	/* Read USB calib fuse & ext values */
	fuse_calib = readl(FUSE_USB_CALIB_0);
	fuse_calib_ext = readl(FUSE_USB_CALIB_EXT_0);
	debug("%s: FUSE_CALIB regs = 0x%08X/0x%08X\n", __func__, fuse_calib, fuse_calib_ext);

	for (i = 0; i < ARRAY_SIZE(hs_curr_level); i++) {
		hs_curr_level[i] = (fuse_calib >> HS_CURR_LEVEL_PADX_SHIFT(i)) &
			HS_CURR_LEVEL_PAD_MASK;
	}

	hs_squelch = (fuse_calib >> HS_SQUELCH_SHIFT) & HS_SQUELCH_MASK;
	hs_term_range_adj =
		(fuse_calib >> HS_TERM_RANGE_ADJ_SHIFT) & HS_TERM_RANGE_ADJ_MASK;
	rpd_ctrl = (fuse_calib_ext >> RPD_CTRL_SHIFT) & RPD_CTRL_MASK;

	debug("%s: HS_SQUELCH = 0x%X, HS_TERM_RANGE_ADJ = 0x%X\n", __func__, hs_squelch, hs_term_range_adj);
	debug(" RPD_CTRL = 0x%X\n", rpd_ctrl);

	for (i = 0; i < ARRAY_SIZE(hs_curr_level); i++)
		debug("HS_CURR_LEVEL[%d] = 0x%02X\n", i, hs_curr_level[i]);

	for (index = 0; index < 3; index++) {
		reg = XUSB_PADCTL_USB2_OTG_PADX_CTL_0(index);
		val = readl(xusbpadctl + (reg / 4));
		val &= ~USB2_OTG_PD;
		val &= ~USB2_OTG_PD_ZI;
		val &= ~HS_CURR_LEVEL(~0);
		val |= HS_CURR_LEVEL(hs_curr_level[index] + hs_curr_level_offset);
		writel(val, xusbpadctl + (reg / 4));
		debug("%s: %d: Wrote 0x%08X to 0x7009F%03X\n", __func__, index, val, reg);

		reg = XUSB_PADCTL_USB2_OTG_PADX_CTL_1(index);
		val = readl(xusbpadctl + (reg / 4));
		val &= ~TERM_RANGE_ADJ(~0);
		val &= ~RPD_CTRL(~0);
		val |= TERM_RANGE_ADJ(hs_term_range_adj);
		val |= RPD_CTRL(rpd_ctrl);
		val &= ~USB2_OTG_PD_DR;
		writel(val, xusbpadctl + (reg / 4));
		debug("%s: %d: Wrote 0x%08X to 0x7009F%03X\n", __func__, index, val, reg);
	}

	/* Setup pad tracking */
	pad_trk_init(hs_squelch);
}

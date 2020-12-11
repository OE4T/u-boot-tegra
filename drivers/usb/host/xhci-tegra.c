// SPDX-License-Identifier: GPL-2.0+
/* Copyright (c) 2020 NVIDIA Corporation */

#include <common.h>
#include <dm.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch-tegra/usb.h>
#include <asm/arch-tegra/clk_rst.h>
#include <asm/arch/powergate.h>
#include <usb.h>
#include <linux/libfdt.h>
#include <fdtdec.h>
#include <malloc.h>
#include <watchdog.h>
#include <asm/gpio.h>
#include <linux/compat.h>
#include <usb/xhci.h>

#include "tegra_xhci_fw.h"
#include "xusb-macros.h"

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

struct tegra_xhci_platdata {
	fdt_addr_t hcd_base;		/* Host controller base address */
	struct gpio_desc vbus_gpio;	/* VBUS for GPIO enable */
	fdt_addr_t fw_addr;		/* XUSB firmware address in memory */
};

/**
 * Contains pointers to register base addresses for the usb controller.
 */
struct tegra_xhci {
	struct usb_platdata usb_plat;
	struct xhci_ctrl ctrl;
	struct xhci_hccr *hcd;
};

static int tegra_xhci_usb_ofdata_to_platdata(struct udevice *dev)
{
	struct tegra_xhci_platdata *plat = dev_get_platdata(dev);
	int ret = 0;

	debug("%s: entry\n", __func__);

	/* Get the base address for XHCI controller from the device node */
	plat->hcd_base = dev_read_addr(dev);
	debug("%s: dev_get_addr returns HCD base of 0x%08X\n", __func__,
	      (u32)plat->hcd_base);
	if (plat->hcd_base == FDT_ADDR_T_NONE) {
		pr_err("Can't get the XHCI register base address!\n");
		ret = -ENXIO;
		goto errout;
	}

	/* Vbus gpio */
	gpio_request_by_name(dev, "nvidia,vbus-gpio", 0, &plat->vbus_gpio,
			     GPIOD_IS_OUT);

	/* Get the XUSB firmware address that CBoot saved to DTB, now in env */
	plat->fw_addr = env_get_hex("xusb_fw_addr", 0);

	if (plat->fw_addr) {
		debug("%s: XUSB FW is @ 0x%08X\n", __func__,
		      (u32)plat->fw_addr);
	} else {
		pr_err("Can't get the XUSB firmware load address!\n");
		ret = -ENXIO;
	}
errout:

	return ret;
}

static int tegra_uphy_pll_enable(void)
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

static void tegra_pllu_enable(void)
{
	debug("%s: entry\n", __func__);
	/* TODO: Enable PLLU here */
}

static void padctl_usb3_port_init(int board_id)
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

static void pad_trk_init(u32 hs_squelch)
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
	val &= ~(0x7F << TRK_START_TIMER);		/* START TIMER = 30 */
	val |= (0x1E << TRK_START_TIMER);		/* DONE TIMER = 10 */
	val &= ~(0x7F << TRK_DONE_RESET_TIMER);
	val |= (0x0A << TRK_DONE_RESET_TIMER);		/* DONE TIMER = 10 */
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_1, val);

	NV_XUSB_PADCTL_READ(USB2_BIAS_PAD_CTL_0, val);
	val &= ~(1 << PD);				/* PD = 0 */
	val &= ~BIAS_PAD_PD;
	val &= ~HS_DISCON_LEVEL(~0);
	val |= HS_DISCON_LEVEL(0x7);
	val &= ~HS_SQUELCH_LEVEL(~0);
	val |= HS_SQUELCH_LEVEL(hs_squelch);
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_0, val);

	udelay(1);

	NV_XUSB_PADCTL_READ(USB2_BIAS_PAD_CTL_1, val);
	val &= ~(1 << PD_TRK);				/* Enable tracking */
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_1, val);
}

static void do_padctl_usb2_config(void)
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

/* Simple hacky way to get board ID (3450, 3541, etc). */
static int get_board_id(void)
{
	/* for ex, CONFIG_TEGRA_BOARD_STRING = "NVIDIA P3450-0000" */
	const char *board = CONFIG_TEGRA_BOARD_STRING;
	int board_num;

	board_num = (int) simple_strtol(board+8, NULL, 10);
	debug("%s: board = %d\n", __func__, board_num);

	return board_num;
}

void do_tegra_xusb_padctl_init(void)
{
	int board_id;
	debug("%s: entry\n", __func__);

	board_id = get_board_id();
	debug("%s: Board ID = %d\n", __func__, board_id);

	/* Set up USB2 pads (MUX, CTLx, CAP, etc.) */
	do_padctl_usb2_config();

	/* Set up SS port map */
	padctl_usb3_port_init(board_id);
}

void xhci_clk_rst_init(struct tegra_xhci *tegra)
{
	u32 reg_data;
	u32 base = (uintptr_t)tegra->hcd;
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;

	debug("%s: entry, XHCI base @ 0x%08X\n", __func__, base);

	clock_enable(PERIPH_ID_XUSB);
	clock_enable(PERIPH_ID_XUSB_HOST);

	/* enable the clocks to individual XUSB partitions */
	clock_enable(PERIPH_ID_XUSB_DEV);
	/* wait stabilization time (always) */
	udelay(NVBOOT_RESET_STABILIZATION_DELAY);

	clock_enable(PERIPH_ID_XUSB_SS);
	udelay(NVBOOT_RESET_STABILIZATION_DELAY);

	/* disable PLLU OUT1 divider reset */
	reg_data = readl(&clkrst->crc_pll[CLOCK_ID_USB].pll_out[0]);
	reg_data |= (1 << 0);		/* bit 0 = OUT1_RSTN reset disable */
	writel(reg_data, &clkrst->crc_pll[CLOCK_ID_USB].pll_out[0]);

	/* Wait 2 us before using FO_48M clock */
	udelay(2);

	/*
	 * Set  XUSB_CORE_HOST_CLK_SRC to 'PLLP_OUT0', divisor to 6
	 * Set  XUSB_FALCON_CLK_SRC to 'PLLP_OUT0', divisor to 2
	 * Set  XUSB_FS_CLK_SRC to 'FO_48M', divisor to 0
	 * Set  XUSB_SS_CLK_SRC to 'HSIC_480', divisor to 6
	 */
	adjust_periph_pll(CLK_SRC_XUSB_CORE_HOST, 1, MASK_BITS_31_29, 6);

	adjust_periph_pll(CLK_SRC_XUSB_FALCON, 1, MASK_BITS_31_29, 2);

	adjust_periph_pll(CLK_SRC_XUSB_FS, 2, MASK_BITS_31_29, 0);

	/* Maintain default HS_HSICP clock @ 120Mhz */
	adjust_periph_pll(CLK_SRC_XUSB_SS, 3, MASK_BITS_31_29, 6);

	/*
	 * Clear SWR_XUSB_HOST_RST bit to release the reset to the XUSB host
	 *  block. The reset to XUSB host block should only be deasserted
	 *  after the port and pad programming and the clock programming are
	 *  both completed.
	 */
	reset_set_enable(PERIPH_ID_XUSB_HOST, 0);
	udelay(NVBOOT_RESET_STABILIZATION_DELAY);

	reset_set_enable(PERIPH_ID_XUSB_DEV, 0);
	udelay(NVBOOT_RESET_STABILIZATION_DELAY);

	reset_set_enable(PERIPH_ID_XUSB_SS, 0);
	udelay(NVBOOT_RESET_STABILIZATION_DELAY);

	reset_set_enable(PERIPH_ID_USBD, 0);
	udelay(NVBOOT_RESET_STABILIZATION_DELAY);

	reset_set_enable(PERIPH_ID_USB2, 0);
	udelay(NVBOOT_RESET_STABILIZATION_DELAY);

	/* Host controller identification and enumeration */
	NV_XUSB_HOST_READ(CONFIGURATION, reg_data);
	reg_data |= EN_FPCI;
	NV_XUSB_HOST_WRITE(CONFIGURATION, reg_data);

	NV_XUSB_CFG_WRITE(4, base);

	NV_XUSB_CFG_READ(1, reg_data);
	reg_data |= (BUS_MASTER + MEMORY_SPACE);
	NV_XUSB_CFG_WRITE(1, reg_data);

	NV_XUSB_HOST_READ(INTR_MASK, reg_data);
	reg_data |= IP_INT_MASK;
	NV_XUSB_HOST_WRITE(INTR_MASK, reg_data);

	NV_XUSB_HOST_WRITE(CLKGATE_HYSTERESIS, CLK_DISABLE_CNT);
}

static int tegra_xhci_core_init(struct tegra_xhci *tegra, u32 fw_addr)
{
	u32 val, val2;
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	int err;

	debug("%s: entry\n", __func__);

	/* Setup USB clocks, etc. */
	xhci_clk_rst_init(tegra);

	/* Reset PADCTL (need to setup PADCTL via slams) */
	reset_set_enable(PERIPH_ID_XUSB_PADCTL, 1);
	mdelay(1);
	reset_set_enable(PERIPH_ID_XUSB_PADCTL, 0);

	/* Setup PADCTL registers w/default values per board (TX1/Nano) */
	do_tegra_xusb_padctl_init();

	/* Check PLLU, set up if not enabled & locked */
	val = readl(&clkrst->crc_pll[CLOCK_ID_USB].pll_base);
	if (val & ((1 << 27) | (1 << 30))) {
		debug("%s: PLLU locked and enabled, skipping init ..\n",
		      __func__);
	} else {
		debug("%s: enabling PLLU ...\n", __func__);
		tegra_pllu_enable();
	}

	/* Setup UTMIPLL (XTAL_FREQ_COUNT = 3, rest of regs s/b default) */
	val = readl(&clkrst->crc_utmip_pll_cfg1);
	val &= 0xFFFFF000;			/* bits 11:0 */
	val |= 3;
	writel(val, &clkrst->crc_utmip_pll_cfg1);
	debug("%s: UTMIPLL: wrote %X to CFG1 reg ...\n", __func__, val);

	/* Check PLLE, set up if not enabled & locked */
	val = readl(&clkrst->crc_pll_simple[SIMPLE_PLLE].pll_base);
	val2 = readl(&clkrst->crc_pll_simple[SIMPLE_PLLE].pll_misc);
	if ((val & (1 << 31)) && (val2 & (1 << 11))) {
		debug("%s: PLLE locked and enabled, skipping init ..\n",
		      __func__);
	} else {
		debug("%s: enabling PLLE ...\n", __func__);
		tegra_plle_enable();
	}

	/* Set up PEX_PAD PLL in PADCTL (skip if enabled/locked) */
	NV_XUSB_PADCTL_READ(UPHY_PLL_P0_CTL_1, val);
	if (val & ((1 << 15) | (1 << 3))) {
		debug("%s: UPHY PLL locked and enabled, skipping init ..\n",
		      __func__);
	} else {
		debug("%s: enabling UPHY PLL ...\n", __func__);
		tegra_uphy_pll_enable();
	}

	/* Load XUSB FW (RP4, see CBoot FW load code) */
	err = xhci_load_firmware((u64)fw_addr);
	if (err < 0) {
		if (err == -EEXIST) {
			/* Firmware already loaded/running */
			printf("XUSB firmware already running: %d\n", err);
		} else {
			printf("Failed to load XUSB firmware!: %d\n", err);
			return err;
		}
	}

	/* Done, start XHCI and hub/storage drivers */

	return 0;
}

static void tegra_xhci_core_exit(struct tegra_xhci *tegra)
{
	u32 val;

	debug("%s: entry\n", __func__);

	/* Assert reset to XUSB partitions */
	reset_set_enable(PERIPH_ID_XUSB_HOST, 1);
	reset_set_enable(PERIPH_ID_XUSB_DEV, 1);
	reset_set_enable(PERIPH_ID_XUSB_SS, 1);

	reset_set_enable(PERIPH_ID_USBD, 1);
	reset_set_enable(PERIPH_ID_USB2, 1);

	/* XUSB_PADCTL_USB2_BIAS_PAD_CTL_0_0, power down the bias pad */
	NV_XUSB_PADCTL_READ(USB2_BIAS_PAD_CTL_0, val);
	val |= (1 << PD);
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_0, val);

	/* Assert UPHY reset */
	reset_set_enable(PERIPH_ID_PEX_USB_UPHY, 1);

	/* Reset PADCTL (debug fix for HID on Batuu?) */
	debug("%s: Resetting PADCTL block ...\n", __func__);
	reset_set_enable(PERIPH_ID_XUSB_PADCTL, 1);
	mdelay(1);
	reset_set_enable(PERIPH_ID_XUSB_PADCTL, 0);

	/* Set CLK_SRC_XUSB_FS to 'CLK_M' or LS/FS devices won't work in kernel */
	adjust_periph_pll(CLK_SRC_XUSB_FS, 0, MASK_BITS_31_29, 0);

	debug("%s: done\n", __func__);
}

static int tegra_xhci_usb_probe(struct udevice *dev)
{
	struct tegra_xhci_platdata *plat = dev_get_platdata(dev);
	struct tegra_xhci *ctx = dev_get_priv(dev);
	struct xhci_hcor *hcor;
	int ret, len;

	debug("%s: entry\n", __func__);

	ctx->hcd = (struct xhci_hccr *)plat->hcd_base;

	/* setup the Vbus gpio here */
	if (dm_gpio_is_valid(&plat->vbus_gpio)) {
		debug("%s: Setting GPIO %d to 1\n", __func__,
		      plat->vbus_gpio.offset);
		dm_gpio_set_value(&plat->vbus_gpio, 1);
	}

	ret = tegra_xhci_core_init(ctx, plat->fw_addr);
	if (ret) {
		puts("XHCI: failed to initialize controller\n");
		return -EINVAL;
	}

	len = HC_LENGTH(xhci_readl(&ctx->hcd->cr_capbase));
	hcor = (struct xhci_hcor *)((u64)ctx->hcd + len);

	debug("XHCI-TEGRA init hccr %lX and hcor %lX hc_length %d\n",
	      (uintptr_t)ctx->hcd, (uintptr_t)hcor, len);

	return xhci_register(dev, ctx->hcd, hcor);
}

static int tegra_xhci_usb_remove(struct udevice *dev)
{
	struct tegra_xhci *ctx = dev_get_priv(dev);
	int ret;

	ret = xhci_deregister(dev);
	if (ret)
		return ret;

	tegra_xhci_core_exit(ctx);

	return 0;
}

static const struct udevice_id tegra_xhci_usb_ids[] = {
	{ .compatible = "nvidia,tegra210-xhci" },
	{ }
};

U_BOOT_DRIVER(usb_xhci) = {
	.name	= "xhci_tegra",
	.id	= UCLASS_USB,
	.of_match = tegra_xhci_usb_ids,
	.ofdata_to_platdata = tegra_xhci_usb_ofdata_to_platdata,
	.probe = tegra_xhci_usb_probe,
	.remove = tegra_xhci_usb_remove,
	.ops	= &xhci_usb_ops,
	.platdata_auto_alloc_size = sizeof(struct tegra_xhci_platdata),
	.priv_auto_alloc_size = sizeof(struct tegra_xhci),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};

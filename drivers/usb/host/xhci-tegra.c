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

void tegra_xusb_slam_padctl_init(void)
{
	/*
	 * FIXME: SLAM PADCTL REGS HERE USING KERNEL VALUES FOR NANO/TX1
	 * Essentially does this:
	 *  assign USB2 pads to XUSB (PADCTL)
	 *  assign port caps for USB2 ports owned by XUSB (PADCTL)
	 *  program PADCTL regs to keep UPHY in IDDQ
	 *  assign UPHY lanes to XUSB (PADCTL)
	 *  take UPHY out of IDDQ (PADCTL)
	 *  assign static UPHY PAD/PLL params lanes (PADCTL)
	 *  assign static UPHY PAD params of ports owned by XUSB (PADCTL)
	 *  assign static USB2 PAD params to ports owned by XUSB (PADCTL)
	 *  disable powerdown of USB2 ports owned by XUSB (PADCTL)
	 *  release the XUSB SS wake logic state latches (PADCTL)
	 *  bring lanes of UPHY out of IDDQ (only used lanes) (PADCTL)
	 *  release always-on PAD muxing logic state latching (PADCTL)
	 * This is a WAR because the U-Boot XUSB padctl driver isn't
	 *  setting things up correctly at this time, need to debug & fix.
	 */

	int i;
	u32 reg, val;
	u32 *xusbpadctl = (u32 *)NV_ADDRESS_MAP_APB_XUSB_PADCTL_BASE;
	u32 padctl_regs[] = {
		/* reg		data */
		0x0004, 0x00040055,		// USB2_PAD_MUX
		0x0008, 0x00000113,		// USB2_PORT_CAP
		0x0014, 0x00001CE1,		// SS_PORT_MAP
		0x0020, 0x00C00100,		// ELPG_PROGRAM_0
		0x0024, 0x000001F8,		// ELPG_PROGRAM_1
		0x0028, 0x817FC0FE,		// USB3_PAD_MUX
		0x0088, 0x00CD100E,		// USB2_OTG_PAD0_CTL_0
		0x008C, 0x12100040,		// USB2_OTG_PAD0_CTL_1
		0x00C8, 0x00CD1011,		// USB2_OTG_PAD1_CTL_0
		0x00CC, 0x12100040,		// USB2_OTG_PAD1_CTL_1
		0x0108, 0x00CD100E,		// USB2_OTG_PAD2_CTL_0
		0x010C, 0x10100040,		// USB2_OTG_PAD2_CTL_1
		0x0284, 0x260E003A,		// USB2_BIAS_PAD_CTL_0
		0x0288, 0x0451E8E5,		// USB2_BIAS_PAD_CTL_1
		0x0460, 0x01000080,		// UPHY_MISC_PAD_P0_CTL_1
		0x04A0, 0x11000000,		// UPHY_MISC_PAD_P1_CTL_1
		0x04E0, 0x11000000,		// UPHY_MISC_PAD_P2_CTL_1
		0x0520, 0x11000000,		// UPHY_MISC_PAD_P3_CTL_1
		0x0560, 0x11000000,		// UPHY_MISC_PAD_P4_CTL_1
		0x0AA0, 0x0020001F,		// UPHY_USB3_PAD1_ECTL_1
		0x0AA4, 0x000000FC,		// UPHY_USB3_PAD1_ECTL_2
		0x0AA8, 0xC0077F1F,		// UPHY_USB3_PAD1_ECTL_3
		0x0AB4, 0xFCF01368,		// UPHY_USB3_PAD1_ECTL_6
		0x0C60, 0x0021505B,		// USB2_VBUS_ID
	};

	debug("%s: entry\n", __func__);

	for (i = 0; i < sizeof(padctl_regs) / 4; i += 2) {
		reg = padctl_regs[i];
		val = padctl_regs[i + 1];
		writel(val, xusbpadctl + (reg / 4));
		debug("%s: Wrote 0x%08X to 0x7009F%03X\n", __func__, val, reg);
	}
}

static void pad_trk_init(void)
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
	val &= ~(0x7F << TRK_START_TIMER);		/* START TIMER = 0 */
	val &= ~(0x7F << TRK_DONE_RESET_TIMER);
	val |= (0x0A << TRK_DONE_RESET_TIMER);		/* DONE TIMER = 10 */
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_1, val);

	NV_XUSB_PADCTL_READ(USB2_BIAS_PAD_CTL_0, val);
	val &= ~(1 << PD);				/* PD = 0 */
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_0, val);

	udelay(1);

	NV_XUSB_PADCTL_READ(USB2_BIAS_PAD_CTL_1, val);
	val &= ~(1 << PD_TRK);				/* Enable tracking */
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_1, val);
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

	/* Slam PADCTL registers w/default values per board (TX1/Nano) */
	tegra_xusb_slam_padctl_init();

	/* Setup pad tracking */
	pad_trk_init();

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

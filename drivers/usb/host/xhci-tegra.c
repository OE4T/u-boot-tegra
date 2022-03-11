// SPDX-License-Identifier: GPL-2.0+
/* Copyright (c) 2020-2021 NVIDIA Corporation */

#include <common.h>
#include <dm.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm-generic/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch-tegra/usb.h>
#include <asm/arch-tegra/clk_rst.h>
#include <asm/arch/powergate.h>
#include <usb.h>
#include <linux/libfdt.h>
#include <linux/sizes.h>
#include <fdtdec.h>
#include <malloc.h>
#include <watchdog.h>
#include <asm/gpio.h>
#include <linux/compat.h>
#include <linux/delay.h>
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
	struct usb_plat usb_plat;
	struct xhci_ctrl ctrl;
	struct xhci_hccr *hcd;
};

int find_rp4_data(char *fwbuffer, long size);

static int tegra_xhci_usb_ofdata_to_platdata(struct udevice *dev)
{
	struct tegra_xhci_platdata *plat = dev_get_plat(dev);
	int ret = 0;
	long size = SZ_128K;		/* size of RP4 blob */
	char *fwbuf;

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
#ifdef CBOOT_RP4_LOAD
	/* Get the XUSB firmware address that CBoot saved to DTB, now in env */
	plat->fw_addr = env_get_hex("xusb_fw_addr", 0);

	if (plat->fw_addr) {
		debug("%s: XUSB FW is @ 0x%08X\n", __func__,
		      (u32)plat->fw_addr);
	} else {
		pr_err("Can't get the XUSB firmware load address!\n");
		ret = -ENXIO;
	}

#else	/* !CBOOT_RP4_LOAD */

	/* allocate 128KB for RP4 FW, pass to find_rp4_data */
	fwbuf = malloc(size);
	if (!fwbuf) {
		pr_err("Could not allocate %ld byte RP4 buffer!\n", size);
		ret = -ENOMEM;
	}

	/* Search QSPI (or eMMC) for RP4 blob, load it to fwbuf */
	ret = find_rp4_data(fwbuf, size);
	if (!ret) {
		debug("%s: Got some RP4 data!\n", __func__ );
		debug("[U-Boot] found XUSB FW @ 0x%p\n", fwbuf);
		plat->fw_addr = (u64)fwbuf;
		plat->fw_addr += 0x28C;
		debug(" plat->fw_addr is now 0x%lld!\n", plat->fw_addr);
	} else {
		pr_err("Cannot read the RP4 data!\n");
		free(fwbuf);
		ret = -ENXIO;
	}
#endif	/* CBOOT_RP4_LOAD */

errout:
	return ret;
}


static void tegra_pllu_enable(void)
{
	debug("%s: entry\n", __func__);
	/* TODO: Enable PLLU here */
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
	val |= BIAS_PAD_PD;
	NV_XUSB_PADCTL_WRITE(USB2_BIAS_PAD_CTL_0, val);

	/* Assert UPHY reset */
	reset_set_enable(PERIPH_ID_PEX_USB_UPHY, 1);

	/* Reset PADCTL */
	debug("%s: Resetting PADCTL block ...\n", __func__);
	reset_set_enable(PERIPH_ID_XUSB_PADCTL, 1);
	mdelay(1);
	reset_set_enable(PERIPH_ID_XUSB_PADCTL, 0);

	/* Set CLK_SRC_XUSB_FS to 'CLK_M' or LS/FS devices fail in the kernel */
	adjust_periph_pll(CLK_SRC_XUSB_FS, 0, MASK_BITS_31_29, 0);

	debug("%s: done\n", __func__);
}

static int tegra_xhci_usb_probe(struct udevice *dev)
{
	struct tegra_xhci_platdata *plat = dev_get_plat(dev);
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
	.of_to_plat = tegra_xhci_usb_ofdata_to_platdata,
	.probe = tegra_xhci_usb_probe,
	.remove = tegra_xhci_usb_remove,
	.ops	= &xhci_usb_ops,
	.plat_auto = sizeof(struct tegra_xhci_platdata),
	.priv_auto = sizeof(struct tegra_xhci),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};

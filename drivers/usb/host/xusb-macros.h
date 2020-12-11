/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) 2020 NVIDIA Corporation */

/*
 * Internal definitions and macros for using XHCI/XUSB as a boot device.
 */

#ifndef __XUSB_MACROS_H__
#define __XUSB_MACROS_H__

#ifndef _MK_ADDR_CONST
#define _MK_ADDR_CONST(_constant_) _constant_
#endif

#ifndef _MK_ENUM_CONST
#define _MK_ENUM_CONST(_constant_) (_constant_ ## UL)
#endif

#define NV_ADDRESS_MAP_CAR_BASE			0x60006000
#define NV_XUSB_HOST_APB_DFPCI_CFG		0x70098000
#define NV_XUSB_HOST_IPFS_REGS			0x70099000
#define NV_ADDRESS_MAP_APB_XUSB_PADCTL_BASE	0x7009F000

#define CLK_SRC_XUSB_CORE_HOST			152
#define CLK_SRC_XUSB_FALCON			153
#define CLK_SRC_XUSB_FS				154
#define CLK_SRC_XUSB_CORE_DEV			155
#define CLK_SRC_XUSB_SS				156

#define NVBOOT_RESET_STABILIZATION_DELAY	0x2
#define NVBOOT_CLOCKS_CLOCK_STABILIZATION_TIME	0x2

#define XUSB_HOST_INTR_MASK_0			0x188
#define XUSB_HOST_CLKGATE_HYSTERESIS_0		0x1BC

#define XUSB_HOST_CONFIGURATION_0		_MK_ADDR_CONST(0x0180)

#define XUSB_CFG_1_0				_MK_ADDR_CONST(0x0004)
#define XUSB_CFG_4_0				_MK_ADDR_CONST(0x0010)

// TODO: Move to XUSB PADCTL driver!!
/* FUSE USB_CALIB registers */
#define FUSE_USB_CALIB_0	(NV_PA_FUSE_BASE+0x1F0)
#define   HS_CURR_LEVEL_PADX_SHIFT(x)	((x) ? (11 + ((x) - 1) * 6) : 0)
#define   HS_CURR_LEVEL_PAD_MASK	(0x3f)
#define   HS_TERM_RANGE_ADJ_SHIFT	(7)
#define   HS_TERM_RANGE_ADJ_MASK	(0xf)
#define   HS_SQUELCH_SHIFT		(29)
#define   HS_SQUELCH_MASK		(0x7)
/* FUSE_USB_CALIB_EXT_0 */
#define FUSE_USB_CALIB_EXT_0	(NV_PA_FUSE_BASE+0x350)
#define   RPD_CTRL_SHIFT		(0)
#define   RPD_CTRL_MASK			(0x1f)

#define XUSB_PADCTL_USB2_PAD_MUX_0	(0x004)
#define   XUSB_PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_SHIFT	(0)
#define   XUSB_PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_MASK	(0x3)
#define   XUSB_PADCTL_USB2_PAD_MUX_USB2_OTG_PAD_PORT0_XUSB	(0x1)
#define   XUSB_PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_SHIFT		(18)
#define   XUSB_PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_MASK		(0x3)
#define   XUSB_PADCTL_USB2_PAD_MUX_USB2_BIAS_PAD_XUSB		(0x1)

#define XUSB_PADCTL_USB2_PORT_CAP_0	(0x008)
#define   XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_DISABLED(x)  (0x0 << ((x) * 4))
#define   XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_HOST(x)	(0x1 << ((x) * 4))
#define   XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_DEVICE(x)	(0x2 << ((x) * 4))
#define   XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_OTG(x)	(0x3 << ((x) * 4))
#define   XUSB_PADCTL_USB2_PORT_CAP_PORTX_CAP_MASK(x)	(0x3 << ((x) * 4))

#define XUSB_PADCTL_SS_PORT_MAP_0		(0x014)
#define   SS_PORT_MAP(_ss, _usb2)		(((_usb2) & 0x7) << ((_ss) * 5))
#define   SS_PORT_MAP_PORT_DISABLED		(0x7)

#define XUSB_PADCTL_ELPG_PROGRAM_0_0		(0x020)
#define XUSB_PADCTL_ELPG_PROGRAM_1_0		(0x024)
#define   SSPX_ELPG_CLAMP_EN(x)			BIT(0 + (x) * 3)
#define   SSPX_ELPG_CLAMP_EN_EARLY(x)		BIT(1 + (x) * 3)
#define   SSPX_ELPG_VCORE_DOWN(x)		BIT(2 + (x) * 3)
#define   AUX_MUX_LP0_CLAMP_EN			BIT(29)
#define   AUX_MUX_LP0_CLAMP_EN_EARLY		BIT(30)
#define   AUX_MUX_LP0_VCORE_DOWN		BIT(31)

#define XUSB_PADCTL_USB3_PAD_MUX_0		(0x028)
#define   FORCE_PCIE_PAD_IDDQ_DISABLE(x)	(1 << (1 + (x)))
#define   FORCE_SATA_PAD_IDDQ_DISABLE(x)	(1 << (8 + (x)))

#define XUSB_PADCTL_USB2_OTG_PADX_CTL_0(x)	(0x088 + (x) * 0x40)
#define   HS_CURR_LEVEL(x)			((x) & 0x3f)
#define   TERM_SEL				BIT(25)
#define   USB2_OTG_PD				BIT(26)
#define   USB2_OTG_PD2				BIT(27)
#define   USB2_OTG_PD2_OVRD_EN			BIT(28)
#define   USB2_OTG_PD_ZI			BIT(29)

#define XUSB_PADCTL_USB2_OTG_PADX_CTL_1(x)	(0x8c + (x) * 0x40)
#define   USB2_OTG_PD_DR			BIT(2)
#define   TERM_RANGE_ADJ(x)			(((x) & 0xf) << 3)
#define   RPD_CTRL(x)				(((x) & 0x1f) << 26)
#define   RPD_CTRL_VALUE(x)			(((x) >> 26) & 0x1f)

#define XUSB_PADCTL_USB2_BIAS_PAD_CTL_0_0	(0x284)
#define   BIAS_PAD_PD				BIT(11)
#define   HS_DISCON_LEVEL(x)			(((x) & 0x7) << 3)
#define   HS_SQUELCH_LEVEL(x)			(((x) & 0x7) << 0)

#define XUSB_PADCTL_USB2_BIAS_PAD_CTL_1_0	(0x288)
#define   TCTRL_VALUE(x)			(((x) & 0x3f) >> 0)
#define   PCTRL_VALUE(x)			(((x) >> 6) & 0x3f)
#define   USB2_TRK_START_TIMER(x)		(((x) & 0x7f) << 12)
#define   USB2_TRK_DONE_RESET_TIMER(x)		(((x) & 0x7f) << 19)
#define   USB2_PD_TRK				BIT(26)

#define XUSB_PADCTL_UPHY_PLL_P0_CTL_1_0		_MK_ADDR_CONST(0x0360)
#define XUSB_PADCTL_UPHY_PLL_P0_CTL_2_0		_MK_ADDR_CONST(0x0364)
#define XUSB_PADCTL_UPHY_PLL_P0_CTL_5_0		_MK_ADDR_CONST(0x0370)
#define XUSB_PADCTL_UPHY_PLL_P0_CTL_8_0		_MK_ADDR_CONST(0x037C)
#define XUSB_PADCTL_UPHY_PLL_P0_CTL1_PLL0_LOCKDET	BIT(15)
#define XUSB_PADCTL_UPHY_PLL_P0_CTL2_CAL_DONE		BIT(1)
#define XUSB_PADCTL_UPHY_PLL_P0_CTL8_PLL0_RCAL_DONE	BIT(31)

#define XUSB_PADCTL_UPHY_MISC_PAD_PX_CTL_1(x)	(0x460 + (x) * 0x40)
#define XUSB_PADCTL_UPHY_MISC_PAD_S0_CTL_1	0x960
#define   AUX_TX_IDDQ				BIT(0)
#define   AUX_TX_IDDQ_OVRD			BIT(1)
#define   AUX_RX_MODE_OVRD			BIT(13)
#define   AUX_RX_TERM_EN			BIT(18)
#define   AUX_RX_IDLE_MODE(x)			(((x) & 0x3) << 20)
#define   AUX_RX_IDLE_EN			BIT(22)
#define   AUX_RX_IDLE_TH(x)			(((x) & 0x3) << 24)

#define XUSB_PADCTL_USB2_VBUS_ID_0		(0xc60)
#define  VBUS_OVERRIDE_VBUS_ON			BIT(14)
#define  ID_OVERRIDE(x)				(((x) & 0xf) << 18)
#define  ID_OVERRIDE_GROUNDED			ID_OVERRIDE(0)
#define  ID_OVERRIDE_FLOATING			ID_OVERRIDE(8)

#define NV_XUSB_PADCTL_READ(reg, value) \
	value = readl((NV_ADDRESS_MAP_APB_XUSB_PADCTL_BASE \
	+ XUSB_PADCTL_##reg##_0))

#define NV_XUSB_PADCTL_WRITE(reg, value) \
	writel(value, (NV_ADDRESS_MAP_APB_XUSB_PADCTL_BASE \
	+ XUSB_PADCTL_##reg##_0))

#define NV_XUSB_HOST_READ(reg, value) \
	value = readl((NV_XUSB_HOST_IPFS_REGS + XUSB_HOST_##reg##_0))

#define NV_XUSB_HOST_WRITE(reg, value) \
	writel(value, (NV_XUSB_HOST_IPFS_REGS + XUSB_HOST_##reg##_0))

#define NV_XUSB_CFG_READ(reg, value) \
	value = readl((NV_XUSB_HOST_APB_DFPCI_CFG + XUSB_CFG_##reg##_0))

#define NV_XUSB_CFG_WRITE(reg, value) \
	writel(value, (NV_XUSB_HOST_APB_DFPCI_CFG + XUSB_CFG_##reg##_0))

#endif /* __XUSB_MACROS_H__ */

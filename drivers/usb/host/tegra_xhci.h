/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) 2020, NVIDIA CORPORATION */

#ifndef _TEGRA_XHCI_H_
#define _TEGRA_XHCI_H_

#define FW_MAJOR_VERSION(x)                     (((x) >> 24) & 0xff)
#define FW_MINOR_VERSION(x)                     (((x) >> 16) & 0xff)

enum build_info_log {
	LOG_NONE = 0,
	LOG_MEMORY
};

/* T210-specific */
#define XHCI_PCI_ID                             (0x0fac10de)
#define PCIE_DEVICE_CONFIG_BASE                 0x70098000ull

/* FPCI CFG registers */
#define XUSB_CFG_0                              0x000
#define XUSB_CFG_ARU_C11_CSBRANGE               0x41c
#define XUSB_CFG_CSB_BASE_ADDR                  0x800

#define CSB_PAGE_SELECT_MASK                    0x7fffff
#define CSB_PAGE_SELECT_SHIFT                   9
#define CSB_PAGE_OFFSET_MASK                    0x1ff
#define CSB_PAGE_SELECT(addr)                   ((addr) >> (CSB_PAGE_SELECT_SHIFT) & \
						 CSB_PAGE_SELECT_MASK)
#define CSB_PAGE_OFFSET(addr)                   ((addr) & CSB_PAGE_OFFSET_MASK)

/* Falcon CSB registers */
#define XUSB_FALC_CPUCTL                        0x100
#define  CPUCTL_STARTCPU                        BIT(1)
#define  CPUCTL_STATE_HALTED                    BIT(4)
#define  CPUCTL_STATE_STOPPED                   BIT(5)
#define XUSB_FALC_BOOTVEC                       0x104
#define XUSB_FALC_DMACTL                        0x10c
#define XUSB_FALC_IMFILLRNG1                    0x154
#define  IMFILLRNG1_TAG_MASK                    0xffff
#define  IMFILLRNG1_TAG_LO_SHIFT                0
#define  IMFILLRNG1_TAG_HI_SHIFT                16
#define XUSB_FALC_IMFILLCTL                     0x158

/* MP CSB registers */
#define XUSB_CSB_MP_ILOAD_ATTR                  0x101a00
#define XUSB_CSB_MP_ILOAD_BASE_LO               0x101a04
#define XUSB_CSB_MP_ILOAD_BASE_HI               0x101a08
#define XUSB_CSB_MP_L2IMEMOP_SIZE               0x101a10
#define  L2IMEMOP_SIZE_SRC_OFFSET_SHIFT         8
#define  L2IMEMOP_SIZE_SRC_OFFSET_MASK          0x3ff
#define  L2IMEMOP_SIZE_SRC_COUNT_SHIFT          24
#define  L2IMEMOP_SIZE_SRC_COUNT_MASK           0xff
#define XUSB_CSB_MP_L2IMEMOP_TRIG               0x101a14
#define  L2IMEMOP_ACTION_SHIFT                  24
#define  L2IMEMOP_INVALIDATE_ALL                (0x40 << L2IMEMOP_ACTION_SHIFT)
#define  L2IMEMOP_LOAD_LOCKED_RESULT            (0x11 << L2IMEMOP_ACTION_SHIFT)
#define XUSB_CSB_MEMPOOL_L2IMEMOP_RESULT        0x00101A18
#define  L2IMEMOP_RESULT_VLD                    BIT(31)
#define XUSB_CSB_MP_APMAP                       0x10181C
#define  APMAP_BOOTPATH                         BIT(31)

#define IMEM_BLOCK_SIZE                         256
#define DIV_ROUND_UP(n, d)                      (((n) + (d) - 1) / (d))

#define FW_IOCTL_LOG_DEQUEUE_LOW                4
#define FW_IOCTL_LOG_DEQUEUE_HIGH               5
#define FW_IOCTL_DATA_SHIFT                     0
#define FW_IOCTL_DATA_MASK                      0x00ffffff
#define FW_IOCTL_TYPE_SHIFT                     24
#define FW_IOCTL_TYPE_MASK                      0xff000000

#define SIMPLE_PLLE		(CLOCK_ID_EPCI - CLOCK_ID_FIRST_SIMPLE)

#define USB2_BIAS_PAD				18

#define EN_FPCI					BIT(0)
#define BUS_MASTER				BIT(2)
#define MEMORY_SPACE				BIT(1)
#define IP_INT_MASK				BIT(16)
#define CLK_DISABLE_CNT				0x80

struct tegra_xhci_softc {
	u32	fw_size;
	void	*fw_data;
	u64	fw_dma_addr;
	bool	xhci_inited;
	char	*fw_name;
};

int tegra_uphy_pll_enable(void);
void padctl_usb3_port_init(int);
void pad_trk_init(u32);
void do_padctl_usb2_config(void);

#endif /* _TEGRA_XHCI_H_ */

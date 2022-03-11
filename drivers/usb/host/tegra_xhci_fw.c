// SPDX-License-Identifier: GPL-2.0+
/* Copyright (c) 2020 NVIDIA Corporation */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <asm/system.h>
#include <linux/delay.h>
#include "tegra_xhci_fw.h"

#define FW_FROM_PARTITION 0

static inline u32 fpci_to_phys(u32 fpci_addr)
{
	return PCIE_DEVICE_CONFIG_BASE + fpci_addr;
}

u32 fpci_readl(u32 addr)
{
	u64 phys_addr = (u64)fpci_to_phys(addr);
	u32 val = *(volatile u32 *)phys_addr;

	debug("%s addr 0x%x val 0x%x\n", __func__, addr, val);

	return val;
}

void fpci_writel(u32 val, u32 addr)
{
	u64 phys_addr = (u64)fpci_to_phys(addr);

	debug("%s addr 0x%x val 0x%x\n", __func__, addr, val);
	*(volatile u32 *)phys_addr = val;
}

static u32 csb_readl(u32 addr)
{
	u32 page, offset;
	u32 val;

	page = CSB_PAGE_SELECT(addr);
	offset = CSB_PAGE_OFFSET(addr);
	fpci_writel(page, XUSB_CFG_ARU_C11_CSBRANGE);
	val = fpci_readl(XUSB_CFG_CSB_BASE_ADDR + offset);

	debug("%s addr 0x%x val 0x%x\n", __func__, addr, val);

	return val;
}

static void csb_writel(u32 val, u32 addr)
{
	u32 page, offset;

	debug("%s addr 0x%x val 0x%x\n", __func__, addr, val);

	page = CSB_PAGE_SELECT(addr);
	offset = CSB_PAGE_OFFSET(addr);
	fpci_writel(page, XUSB_CFG_ARU_C11_CSBRANGE);
	fpci_writel(val, XUSB_CFG_CSB_BASE_ADDR + offset);
}

#if FW_FROM_PARTITION
static void *get_firmware_blob(u32 *size)
{
	/*
	 * Eventually we should make U-Boot find the RP4 partition and read
	 *  it in to decouple us from CBoot. TBD in a future patch.
	 */
	*size = 124416;
	return env_get_hex("xusb_fw_addr", 0);
}
#endif

int xhci_load_firmware(u64 fw_addr)
{
#if FW_FROM_PARTITION
	u8 *fw_buffer = NULL;
	u32 fw_size = 0;
#endif
	struct xhci_firmware_cfgtbl *cfgtbl;
	struct tegra_xhci_softc *sc;
	u32 val, code_tag_blocks, code_size_blocks;
	u64 fw_base;
	int timeout, err = 0;

	sc = (struct tegra_xhci_softc *)malloc(sizeof(struct tegra_xhci_softc));

#if FW_FROM_PARTITION
	fw_buffer = get_firmware_blob(&fw_size);
	if (!fw_buffer) {
		printf("No USB firmware found!!\n");
		err = -ENODEV;
		goto error;
	}

	debug("XUSB blob size %d @ %p\n", fw_size, fw_buffer);
#endif

	val = fpci_readl(XUSB_CFG_0);
	if (val == XHCI_PCI_ID) {
#if FW_FROM_PARTITION
		cfgtbl = (struct xhci_firmware_cfgtbl *)fw_buffer;
#else
		cfgtbl = (struct xhci_firmware_cfgtbl *)fw_addr;
		debug("XUSB blob size %d @ %X\n", cfgtbl->fwimg_len,
		      (u32)fw_addr);
#endif
	} else {
		printf("Unknown controller 0x%08x\n", val);
		err = -ENODEV;
		goto error;
	}

	printf("\nFirmware size %d\n", cfgtbl->fwimg_len);
	sc->fw_size = cfgtbl->fwimg_len;
	sc->fw_data = (void *)noncached_alloc(sc->fw_size, 256);
	sc->fw_dma_addr = (u64)sc->fw_data;

	if (!sc->fw_data) {
		printf("Alloc memory for firmware failed\n");
		err = -ENOMEM;
		goto error;
	}
	memcpy(sc->fw_data, cfgtbl, sc->fw_size);
	cfgtbl = sc->fw_data;

	if (csb_readl(XUSB_CSB_MP_ILOAD_BASE_LO) != 0) {
		printf("Firmware already loaded, Falcon state %#x\n",
		       csb_readl(XUSB_FALC_CPUCTL));
		err = -EEXIST;
		goto error;
	}

	/* Program the size of DFI into ILOAD_ATTR. */
	csb_writel(sc->fw_size, XUSB_CSB_MP_ILOAD_ATTR);

	/*
	 * Boot code of the firmware reads the ILOAD_BASE registers
	 * to get to the start of the DFI in system memory.
	 */
	fw_base = sc->fw_dma_addr + sizeof(*cfgtbl);
	csb_writel(fw_base, XUSB_CSB_MP_ILOAD_BASE_LO);
	csb_writel(fw_base >> 32, XUSB_CSB_MP_ILOAD_BASE_HI);

	/* Set BOOTPATH to 1 in APMAP. */
	csb_writel(APMAP_BOOTPATH, XUSB_CSB_MP_APMAP);

	/* Invalidate L2IMEM. */
	csb_writel(L2IMEMOP_INVALIDATE_ALL, XUSB_CSB_MP_L2IMEMOP_TRIG);

	/*
	 * Initiate fetch of bootcode from system memory into L2IMEM.
	 * Program bootcode location and size in system memory.
	 */
	code_tag_blocks = DIV_ROUND_UP(cfgtbl->boot_codetag, IMEM_BLOCK_SIZE);
	code_size_blocks = DIV_ROUND_UP(cfgtbl->boot_codesize, IMEM_BLOCK_SIZE);

	val = ((code_tag_blocks & L2IMEMOP_SIZE_SRC_OFFSET_MASK) <<
			L2IMEMOP_SIZE_SRC_OFFSET_SHIFT) |
		((code_size_blocks & L2IMEMOP_SIZE_SRC_COUNT_MASK) <<
			L2IMEMOP_SIZE_SRC_COUNT_SHIFT);
	csb_writel(val, XUSB_CSB_MP_L2IMEMOP_SIZE);

	/* Trigger L2IMEM Load operation. */
	csb_writel(L2IMEMOP_LOAD_LOCKED_RESULT, XUSB_CSB_MP_L2IMEMOP_TRIG);

	/* Setup Falcon Auto-fill. */
	csb_writel(code_size_blocks, XUSB_FALC_IMFILLCTL);

	val = ((code_tag_blocks & IMFILLRNG1_TAG_MASK) <<
			IMFILLRNG1_TAG_LO_SHIFT) |
		(((code_size_blocks + code_tag_blocks) & IMFILLRNG1_TAG_MASK) <<
			IMFILLRNG1_TAG_HI_SHIFT);
	csb_writel(val, XUSB_FALC_IMFILLRNG1);

	printf("Firmware timestamp: 0x%08x, Version: %2x.%02x %s\n\n",
	       cfgtbl->fwimg_created_time,
	       FW_MAJOR_VERSION(cfgtbl->version_id),
	       FW_MINOR_VERSION(cfgtbl->version_id),
	       (cfgtbl->build_log == LOG_MEMORY) ? "debug" : "release");

	csb_writel(0, XUSB_FALC_DMACTL);
	mdelay(50);

	/* wait for RESULT_VLD to get set */
	timeout = 1000;
	do {
		val = csb_readl(XUSB_CSB_MEMPOOL_L2IMEMOP_RESULT);
		if (val & L2IMEMOP_RESULT_VLD)
			break;
		udelay(10);
	} while (timeout--);

	if (!(val & L2IMEMOP_RESULT_VLD))
		printf("DMA controller not ready 0x08%x\n", val);

	csb_writel(cfgtbl->boot_codetag, XUSB_FALC_BOOTVEC);

	/* Start Falcon CPU */
	csb_writel(CPUCTL_STARTCPU, XUSB_FALC_CPUCTL);
	udelay(2000);

	/* wait for Falcon to enter STOPPED mode */
	timeout = 200;
	do {
		val = csb_readl(XUSB_FALC_CPUCTL);
		if (val & CPUCTL_STATE_STOPPED)
			break;
		mdelay(1);
	} while (timeout--);

	val = csb_readl(XUSB_FALC_CPUCTL);
	if (!(val & CPUCTL_STATE_STOPPED)) {
		printf("Falcon not stopped. Falcon state: 0x%x\n",
		       csb_readl(XUSB_FALC_CPUCTL));
		err = -EIO;
	}

error:
	free(sc);
	return err;
}

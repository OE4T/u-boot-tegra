/*
 * (C) Copyright 2018-2019
 * NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _P3450_PORG_H
#define _P3450_PORG_H

#include <linux/sizes.h>

#include "tegra210-common.h"

/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"NVIDIA P3450-PORG"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTA

/* I2C */
#define CONFIG_SYS_I2C_TEGRA
#define CONFIG_SYS_VI_I2C_TEGRA

/* SD/MMC */
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_TEGRA_MMC

#ifdef CONFIG_NANO_EMMC
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)

#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		2
#define CONFIG_ENV_OFFSET		(-CONFIG_ENV_SIZE)

#define DEFAULT_MMC_DEV "0"
#else
/* Only MMC1/PXE/DHCP for now, add USB back in later when supported */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 1) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)

/* Environment at end of QSPI, in the VER partition */
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_SPI_MAX_HZ		48000000
#define CONFIG_ENV_SPI_MODE		SPI_MODE_0
#define CONFIG_ENV_SECT_SIZE		SZ_4K
/* 4MB flash, environment located as high as possible */
#define CONFIG_ENV_OFFSET		(SZ_4M - CONFIG_ENV_SIZE)

/* SPI */
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_SF_DEFAULT_SPEED		24000000
#define CONFIG_SPI_FLASH_SIZE		(4 << 20)

#define DEFAULT_MMC_DEV "1"
#endif /* CONFIG_NANO_EMMC */

/* USB2.0 Host support */
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_TEGRA

/* PCI host support */
#define CONFIG_PCI
#define CONFIG_PCI_PNP
#define CONFIG_CMD_PCI

#define CONFIG_PREBOOT

#define BOARD_EXTRA_ENV_SETTINGS \
	"preboot=if test -e mmc " DEFAULT_MMC_DEV ":1 /u-boot-preboot.scr; then " \
		"load mmc " DEFAULT_MMC_DEV ":1 ${scriptaddr} /u-boot-preboot.scr; " \
		"source ${scriptaddr}; " \
	"fi\0"

/* General networking support */

#include "tegra-common-usb-gadget.h"
#include "tegra-common-post.h"

/* Crystal is 38.4MHz. clk_m runs at half that rate */
#define COUNTER_FREQUENCY	19200000

/* Turn SDMMC1 off in early init on Porg/Nano */
#define CONFIG_DISABLE_SDMMC1_EARLY

#endif /* _P3450_PORG_H */

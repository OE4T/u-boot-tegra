/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2018-2020 NVIDIA Corporation.
 */

#ifndef _P3450_0000_H
#define _P3450_0000_H

#include <linux/sizes.h>

#include "tegra210-common.h"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTA

#ifdef CONFIG_P3450_EMMC
#define CONFIG_TEGRA_BOARD_STRING	"NVIDIA P3450-0002"
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(USB, usb, 0) \
	func(NVME, nvme, 0) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		2
#define DEFAULT_MMC_DEV			"0"
#else
/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"NVIDIA P3450-0000"

/* Only MMC/PXE/DHCP for now, add USB back in later when supported */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 1) \
	func(USB, usb, 0) \
	func(NVME, nvme, 0) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)

/* Environment at end of QSPI, in the VER partition */
#define CONFIG_ENV_SPI_MAX_HZ		48000000
#define CONFIG_ENV_SPI_MODE		SPI_MODE_0
#define CONFIG_SPI_FLASH_SIZE		(4 << 20)
#define DEFAULT_MMC_DEV			"1"
#endif

#define CONFIG_PREBOOT

#ifdef CONFIG_FIT
#undef CONFIG_BOOTCOMMAND
/* FIT Image environment settings */
#define FITIMAGE_ENV_SETTINGS \
	"mmcdev=0\0" \
	"mmcpart=1\0" \
	"devnum=" DEFAULT_MMC_DEV "\0" \
	"fitpart=1\0" \
	"prefix=/boot\0" \
	"mmcfit_name=fitImage\0" \
	"fit_addr=0x90000000\0" \
	"mmcloadfit=load mmc ${devnum}:${fitpart} ${fit_addr} " \
		"${prefix}/${mmcfit_name}\0" \
	"mmcargs=setenv bootargs ${cbootargs} " \
		"root=/dev/mmcblk${mmcdev}p${mmcpart} rw rootwait\0" \
	"mmc_mmc_fit=run mmcloadfit;run mmcargs; bootm ${fit_addr} - ${fdt_addr}\0"

#define CONFIG_BOOTCOMMAND "run mmc_mmc_fit"
#else
#define FITIMAGE_ENV_SETTINGS
#endif /* CONFIG_FIT */

#define BOARD_EXTRA_ENV_SETTINGS \
	"preboot=if test -e mmc " DEFAULT_MMC_DEV ":1 /u-boot-preboot.scr; then " \
		"load mmc " DEFAULT_MMC_DEV ":1 ${scriptaddr} /u-boot-preboot.scr; " \
		"source ${scriptaddr}; " \
	"fi\0" \
	FITIMAGE_ENV_SETTINGS

/* General networking support */
#include "tegra-common-usb-gadget.h"
#include "tegra-common-post.h"

/* Crystal is 38.4MHz. clk_m runs at half that rate */
#define COUNTER_FREQUENCY	19200000

#endif /* _P3450_0000_H */

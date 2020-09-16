/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2018-2019 NVIDIA Corporation.
 */

#ifndef _P3541_0000_H
#define _P3541_0000_H

#include <linux/sizes.h>

#include "tegra210-common.h"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTA

/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"NVIDIA P3541-0000"

/* Nano2GB doesn't have eMMC or NVMe, just SD/USB/Net */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 1) \
	func(USB, usb, 0) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)

/* Environment at end of QSPI, in the VER partition */
#define CONFIG_ENV_SPI_MAX_HZ		48000000
#define CONFIG_ENV_SPI_MODE		SPI_MODE_0
#define CONFIG_SPI_FLASH_SIZE		(4 << 20)
#define DEFAULT_MMC_DEV			"1"

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

#endif /* _P3541_0000_H */

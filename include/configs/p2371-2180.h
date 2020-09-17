/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2013-2020
 * NVIDIA Corporation <www.nvidia.com>
 */

#ifndef _P2371_2180_H
#define _P2371_2180_H

#include <linux/sizes.h>

#include "tegra210-common.h"

/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"NVIDIA P2371-2180"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTA

/* SD, eMMC, USB, NVME, PXE, DHCP */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 0) \
	func(USB, usb, 0) \
	func(NVME, nvme, 0) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)

/* Environment in eMMC, at the end of 2nd "boot sector" */

/* SPI */
#define CONFIG_SPI_FLASH_SIZE		(4 << 20)

#include "tegra-common-usb-gadget.h"
#include "tegra-common-post.h"

/* Crystal is 38.4MHz. clk_m runs at half that rate */
#define COUNTER_FREQUENCY	19200000

#endif /* _P2371_2180_H */

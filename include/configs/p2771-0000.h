/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2013-2020, NVIDIA CORPORATION.
 */

#ifndef _P2771_0000_H
#define _P2771_0000_H

#include <linux/sizes.h>

#include "tegra186-common.h"

/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"NVIDIA P2771-0000"

/* Environment in eMMC, at the end of 2nd "boot sector" */

/* SD, eMMC, USB, NVME, PXE, DHCP */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 0) \
	func(USB, usb, 0) \
	func(NVME, nvme, 0) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)

#include "tegra-common-post.h"

#endif

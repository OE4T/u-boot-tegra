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
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		2

#ifdef CONFIG_FIT
#undef CONFIG_BOOTCOMMAND
#undef BOARD_EXTRA_ENV_SETTINGS
/* FIT Image environment settings */
#define FITIMAGE_ENV_SETTINGS \
	"mmcdev=0\0" \
	"mmcpart=1\0" \
	"devnum=0\0" \
	"fitpart=1\0" \
	"prefix=/boot\0" \
	"mmcfit_name=fitImage\0" \
	"fit_addr=0x90000000\0" \
	"mmcloadfit=load mmc ${devnum}:${fitpart} ${fit_addr} " \
		"${prefix}/${mmcfit_name}\0" \
	"mmcargs=setenv bootargs ${cbootargs} " \
		"root=/dev/mmcblk${mmcdev}p${mmcpart} rw rootwait\0" \
	"mmc_mmc_fit=run mmcloadfit;run mmcargs; bootm ${fit_addr} - ${fdt_addr}\0"

#define BOARD_EXTRA_ENV_SETTINGS \
	FITIMAGE_ENV_SETTINGS

#define CONFIG_BOOTCOMMAND "run mmc_mmc_fit"
#endif /* CONFIG_FIT */

#include "tegra-common-post.h"

/* Crystal is 38.4MHz. clk_m runs at half that rate */
#define COUNTER_FREQUENCY	19200000

#endif

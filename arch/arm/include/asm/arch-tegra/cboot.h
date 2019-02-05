/*
 * Copyright (c) 2019, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _TEGRA_CBOOT_H_
#define _TEGRA_CBOOT_H_

extern unsigned long cboot_boot_x0;

int set_ethaddr_from_cboot(const void *fdt);

#endif

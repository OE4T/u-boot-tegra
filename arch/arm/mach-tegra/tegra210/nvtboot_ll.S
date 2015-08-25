/*
 * Save nvtboot-related boot-time CPU state
 *
 * (C) Copyright 2015 NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <linux/linkage.h>

.globl	nvtboot_boot_x5
nvtboot_boot_x5:
	.dword 0

ENTRY(save_boot_params)
	adr	x0, nvtboot_boot_x5
	str	x5, [x0]
	b	save_boot_params_ret
ENDPROC(save_boot_params)

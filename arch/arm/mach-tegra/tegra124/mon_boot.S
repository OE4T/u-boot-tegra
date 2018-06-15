/*
 * (C) Copyright 2018 NVIDIA CORPORATION
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <config.h>
#include <linux/linkage.h>

ENTRY(boot_monitor)
	ldr	sp, =CONFIG_ARMV7_SECURE_BASE

	ldr	r0, =CONFIG_ARMV7_SECURE_BASE
	mov	r1, #0
	ldr	r2, =CONFIG_ARMV7_SECURE_RESERVE_SIZE
	blx	memset

	ldr	r0, =CONFIG_ARMV7_SECURE_BASE
	ldr	r1, =__secure_start
	ldr	r2, =__secure_end
	sub	r2, r2, r1
	blx	memcpy

	ldr	r0, =boot_monitor_ret
	ldr	lr, =mon_entry
	mov	pc, lr
ENDPROC(boot_monitor)

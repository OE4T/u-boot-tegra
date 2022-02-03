/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  Copyright (c) 2013-2021, NVIDIA CORPORATION. All rights reserved.
 */

#ifndef _TEGRA210_COMMON_H_
#define _TEGRA210_COMMON_H_

#include "tegra-common.h"

/*
 * NS16550 Configuration
 */
#define V_NS16550_CLK		408000000	/* 408MHz (pllp_out0) */

/*
 * Memory layout for where various images get loaded by boot scripts:
 *
 * scriptaddr can be pretty much anywhere that doesn't conflict with something
 *   else. Put it above BOOTMAPSZ to eliminate conflicts.
 *
 * pxefile_addr_r can be pretty much anywhere that doesn't conflict with
 *   something else. Put it above BOOTMAPSZ to eliminate conflicts.
 *
 * kernel_addr_r must be within the first 128M of RAM in order for the
 *   kernel's CONFIG_AUTO_ZRELADDR option to work. Since the kernel will
 *   decompress itself to 0x8000 after the start of RAM, kernel_addr_r
 *   should not overlap that area, or the kernel will have to copy itself
 *   somewhere else before decompression. Similarly, the address of any other
 *   data passed to the kernel shouldn't overlap the start of RAM. Pushing
 *   this up to 16M allows for a sizable kernel to be decompressed below the
 *   compressed load address.
 *
 * fdt_addr_r simply shouldn't overlap anything else. Choosing 32M allows for
 *   the compressed kernel to be up to 16M too.
 *
 * fdtoverlay_addr_r is used for DTB overlays from extlinux.conf. It's placed
 *   just above pxefile_addr_r.
 *
 * ramdisk_addr_r simply shouldn't overlap anything else. Choosing 33M allows
 *   for the FDT/DTB to be up to 1M, which is hopefully plenty.
 */
/*
 * NOTE: fdt_addr (from CBoot) is @ 0x83100000. fdt_addr_r is also from CBoot
 * and can't be moved. To accomodate a 128MB kernel (for gcov, trace, debug,
 * etc.), kernel_addr_r is moved to 0x84000000, above fdt/ramdisk and below
 * pxe/script addresses.
 */
#define MEM_LAYOUT_ENV_SETTINGS \
	"scriptaddr=0x90000000\0" \
	"pxefile_addr_r=0x90100000\0" \
	"fdtoverlay_addr_r=0x90200000\0" \
	"kernel_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"fdtfile=" FDTFILE "\0" \
	"fdt_addr_r=0x83000000\0" \
	"ramdisk_addr_r=0x87200000\0" \
	"fdt_copy_node_paths=" \
		"/chosen/plugin-manager:" \
		"/chosen/reset:" \
		"/chosen/display-board:" \
		"/chosen/proc-board:" \
		"/chosen/pmu-board:" \
		"/external-memory-controller@7001b000:" \
		"/memory@80000000\0" \
	"fdt_copy_prop_paths=" \
		"/bpmp/carveout-start:" \
		"/bpmp/carveout-size:" \
		"/chosen/eks_info:" \
		"/chosen/nvidia,bluetooth-mac:" \
		"/chosen/nvidia,ethernet-mac:" \
		"/chosen/nvidia,wifi-mac:" \
		"/chosen/uuid:" \
		"/chosen/linux,initrd-start:" \
		"/chosen/linux,initrd-end:" \
		"/serial-number:" \
		"/psci/nvidia,system-lp0-disable\0"

/* For USB EHCI controller */
#define CONFIG_USB_EHCI_TXFIFO_THRESH	0x10

/* GPU needs setup */
#define CONFIG_TEGRA_GPU

#define CONFIG_SYS_BOOTM_LEN   (64 << 20)      /* Increase max gunzip size */

#endif /* _TEGRA210_COMMON_H_ */

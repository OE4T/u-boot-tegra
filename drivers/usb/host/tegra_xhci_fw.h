/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) 2020 NVIDIA Corporation */

#ifndef __TEGRA_XHCI_FW_H__
#define __TEGRA_XHCI_FW_H__

#include "tegra_xhci.h"

struct xhci_firmware_cfgtbl {
	u32 boot_loadaddr_in_imem;
	u32 boot_codedfi_offset;
	u32 boot_codetag;
	u32 boot_codesize;
	u32 phys_memaddr;
	u16 reqphys_memsize;
	u16 alloc_phys_memsize;
	u32 rodata_img_offset;
	u32 rodata_section_start;
	u32 rodata_section_end;
	u32 main_fnaddr;
	u32 fwimg_cksum;
	u32 fwimg_created_time;
	u32 imem_resident_start;
	u32 imem_resident_end;
	u32 idirect_start;
	u32 idirect_end;
	u32 l2_imem_start;
	u32 l2_imem_end;
	u32 version_id;
	u8 init_ddirect;
	u8 reserved[3];
	u32 phys_addr_log_buffer;
	u32 total_log_entries;
	u32 dequeue_ptr;
	u32 dummy_var[2];
	u32 fwimg_len;
	u8 magic[8];
	u32 ss_low_power_entry_timeout;
	u8 num_hsic_port;
	u8 ss_portmap;
	u8 build_log:4;
	u8 build_type:4;
	u8 chip_revision[3];
	u8 reserved2[2];
	u32 falcon_clock_freq;
	u32 slots_in_each_vf;
	u32 port_owner[2];		/* 0: ss, 1: hs */
	u8 padding[116];		/* Padding to make 256-byte cfgtbl */
};

int xhci_load_firmware(u64 fw_addr);

#endif	/* __TEGRA_XHCI_FW_H__ */

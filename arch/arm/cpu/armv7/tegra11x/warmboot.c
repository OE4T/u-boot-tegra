/*
 * (C) Copyright 2013
 * NVIDIA Corporation <www.nvidia.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/pmc.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/tegra.h>
#include <asm/arch/fuse.h>
#include <asm/arch/emc.h>
#include <asm/arch/gp_padctrl.h>
#include <asm/arch/warmboot.h>
#include <asm/arch/sdram_param.h>
#include <asm/arch/sdmmc.h>
#include "ap20.h"
#include <asm/arch/t11x/nvbl_memmap_nvap.h>

#define BCT_SDRAM_PARAMS_OFFSET	(0x928)
#define SDRAM_PARAMS_BASE	(NVAP_BASE_PA_SRAM + BCT_SDRAM_PARAMS_OFFSET)

enum field_type {
	TYPE_SDRAM,
	TYPE_PLLM,
	TYPE_CONST,
};

#define sdram_offset(member) offsetof(struct sdram_params, member)
#define pmc_offset(member) offsetof(struct pmc_ctlr, member)
#define pllm_offset(member) offsetof(struct clk_pllm, member)

struct encode_fields {
	enum field_type encode_type;
	u32 src_offset;
	u32 op1_mask;
	u32 op1_shift;
	u32 op2_mask;
	u32 op2_shift;
	u32 dst_offset;
	u32 dst_shift;
};

/*
 * encode: set dst_off.dst_bit to 1 if (src_off.op1_bits > src_off.op2_bits)
 *         else set dst_off.dst_bit to 0.
 */
#define encode(type, src_off, op1_bits, op2_bits, dst_off, dst_bit)	\
	{								\
		.encode_type = type,					\
		.src_offset = src_off,					\
		.op1_mask = 0xfffffffful >>				\
			(31 - ((1 ? op1_bits) - (0 ? op1_bits))),	\
		.op1_shift = 0 ? op1_bits,				\
		.op2_mask = 0xfffffffful >>				\
			(31 - ((1 ? op2_bits) - (0 ? op2_bits))),	\
		.op2_shift = 0 ? op2_bits,				\
		.dst_offset = dst_off,					\
		.dst_shift = dst_bit,					\
	}

struct encode_fields encode_list[] = {
	encode(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte0), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 0),
	encode(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte1), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 1),
	encode(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte2), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 2),
	encode(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte3), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 3),
	encode(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte0), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 4),
	encode(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte1), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 5),
	encode(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte2), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 6),
	encode(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte3), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 7),
	encode(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte0), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 8),
	encode(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte1), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 9),
	encode(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte2), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 10),
	encode(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte3), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 11),
	encode(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte0), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 12),
	encode(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte1), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 13),
	encode(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte2), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 14),
	encode(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte3), 26:24, 30:28, sdram_offset(swizzle_rank_byte_encode), 15),
};

struct pack_fields {
	enum field_type src_type;
	u32 src_offset;		/* for TYPE_CONST, this field is a constant */
	u32 src_mask;
	u32 src_shift;
	enum field_type dst_type;
	u32 dst_offset;
	u32 dst_mask;
	u32 dst_shift;
};

/*
 * pack: copy from src bits to dst bits.
 * For example,
 *   pack(TYPE_SDRAM, sdram_offset(emc_clock_source), 7:0, pmc_offset(pmc_scratch6), 15:8)
 *     describes to:
 *         copy bits 7:0 of sdram_offset.emc_clock_source to
 *         bits 15:8 of pmc_scratch6 register.
 */
#define pack(_src_type, src_off, src_bits, dst_off, dst_bits)		\
	{								\
		.src_type = _src_type,					\
		.src_offset = src_off,					\
		.src_mask = 0xfffffffful >>				\
			(31 - ((1 ? src_bits) - (0 ? src_bits))),	\
		.src_shift = 0 ? src_bits,				\
		.dst_offset = dst_off,					\
		.dst_mask = 0xfffffffful >>				\
			(31 - ((1 ? dst_bits) - (0 ? dst_bits))),	\
		.dst_shift = 0 ? dst_bits,				\
	}

struct pack_fields pack_list_1[] = {
	pack(TYPE_SDRAM, sdram_offset(emc_clock_source), 7:0, pmc_offset(pmc_scratch6), 15:8),
	pack(TYPE_SDRAM, sdram_offset(emc_clock_source), 31:29, pmc_offset(pmc_scratch6), 18:16),
	pack(TYPE_SDRAM, sdram_offset(emc_clock_source), 26:26, pmc_offset(pmc_scratch6), 19:19),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl2), 18:16, pmc_offset(pmc_scratch6), 22:20),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl2), 22:20, pmc_offset(pmc_scratch6), 25:23),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl2), 26:24, pmc_offset(pmc_scratch6), 28:26),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl2), 30:28, pmc_offset(pmc_scratch6), 31:29),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl2), 18:16, pmc_offset(pmc_scratch7), 22:20),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl2), 22:20, pmc_offset(pmc_scratch7), 25:23),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl2), 26:24, pmc_offset(pmc_scratch7), 28:26),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl2), 30:28, pmc_offset(pmc_scratch7), 31:29),
	pack(TYPE_SDRAM, sdram_offset(emc_txsrdll), 11:0, pmc_offset(pmc_scratch8), 31:20),
	pack(TYPE_SDRAM, sdram_offset(emc_dsr_vttgen_drv), 5:0, pmc_offset(pmc_scratch9), 25:20),
	pack(TYPE_SDRAM, sdram_offset(emc_dsr_vttgen_drv), 18:16, pmc_offset(pmc_scratch9), 28:26),
	pack(TYPE_SDRAM, sdram_offset(emc_dsr_vttgen_drv), 26:24, pmc_offset(pmc_scratch9), 31:29),
	pack(TYPE_CONST, 0, 31:0, pmc_offset(pmc_scratch10), 31:0),
	pack(TYPE_SDRAM, sdram_offset(emc_txdsrvttgen), 11:0, pmc_offset(pmc_scratch10), 31:20),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_spare), 31:24, pmc_offset(pmc_scratch11), 7:0),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_spare), 23:16, pmc_offset(pmc_scratch11), 15:8),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_spare), 15:8, pmc_offset(pmc_scratch11), 23:16),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_spare), 7:0, pmc_offset(pmc_scratch11), 31:24),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_fbio_spare), 31:24, pmc_offset(pmc_scratch12), 7:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_fbio_spare), 23:16, pmc_offset(pmc_scratch12), 15:8),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_fbio_spare), 15:8, pmc_offset(pmc_scratch12), 23:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_fbio_spare), 7:0, pmc_offset(pmc_scratch12), 31:24),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_rsv), 7:0, pmc_offset(pmc_scratch13), 7:0),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_rsv), 15:8, pmc_offset(pmc_scratch13), 15:8),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_rsv), 23:16, pmc_offset(pmc_scratch13), 23:16),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_rsv), 31:24, pmc_offset(pmc_scratch13), 31:24),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 0:0, pmc_offset(pmc_scratch14), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 1:1, pmc_offset(pmc_scratch14), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 2:2, pmc_offset(pmc_scratch14), 2:2),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 3:3, pmc_offset(pmc_scratch14), 3:3),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 4:4, pmc_offset(pmc_scratch14), 4:4),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 5:5, pmc_offset(pmc_scratch14), 5:5),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 6:6, pmc_offset(pmc_scratch14), 6:6),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 7:7, pmc_offset(pmc_scratch14), 7:7),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 8:8, pmc_offset(pmc_scratch14), 8:8),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 9:9, pmc_offset(pmc_scratch14), 9:9),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 10:10, pmc_offset(pmc_scratch14), 10:10),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 11:11, pmc_offset(pmc_scratch14), 11:11),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 12:12, pmc_offset(pmc_scratch14), 12:12),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 13:13, pmc_offset(pmc_scratch14), 13:13),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 14:14, pmc_offset(pmc_scratch14), 14:14),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 15:15, pmc_offset(pmc_scratch14), 15:15),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 16:16, pmc_offset(pmc_scratch14), 16:16),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 17:17, pmc_offset(pmc_scratch14), 17:17),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 18:18, pmc_offset(pmc_scratch14), 18:18),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 19:19, pmc_offset(pmc_scratch14), 19:19),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 20:20, pmc_offset(pmc_scratch14), 20:20),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 21:21, pmc_offset(pmc_scratch14), 21:21),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 22:22, pmc_offset(pmc_scratch14), 22:22),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 23:23, pmc_offset(pmc_scratch14), 23:23),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 24:24, pmc_offset(pmc_scratch14), 24:24),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 25:25, pmc_offset(pmc_scratch14), 25:25),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 26:26, pmc_offset(pmc_scratch14), 26:26),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 27:27, pmc_offset(pmc_scratch14), 27:27),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 28:28, pmc_offset(pmc_scratch14), 28:28),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 29:29, pmc_offset(pmc_scratch14), 29:29),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 30:30, pmc_offset(pmc_scratch14), 30:30),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl2), 31:31, pmc_offset(pmc_scratch14), 31:31),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_da_turns), 7:0, pmc_offset(pmc_scratch15), 7:0),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_da_turns), 15:8, pmc_offset(pmc_scratch15), 15:8),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_da_turns), 23:16, pmc_offset(pmc_scratch15), 23:16),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_da_turns), 31:24, pmc_offset(pmc_scratch15), 31:24),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 0:0, pmc_offset(pmc_scratch17), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 2:2, pmc_offset(pmc_scratch17), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 3:3, pmc_offset(pmc_scratch17), 2:2),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 4:4, pmc_offset(pmc_scratch17), 3:3),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 5:5, pmc_offset(pmc_scratch17), 4:4),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 7:6, pmc_offset(pmc_scratch17), 6:5),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 11:8, pmc_offset(pmc_scratch17), 10:7),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 13:12, pmc_offset(pmc_scratch17), 12:11),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 14:14, pmc_offset(pmc_scratch17), 13:13),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 15:15, pmc_offset(pmc_scratch17), 14:14),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 25:16, pmc_offset(pmc_scratch17), 24:15),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 27:27, pmc_offset(pmc_scratch17), 25:25),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 29:28, pmc_offset(pmc_scratch17), 27:26),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 30:30, pmc_offset(pmc_scratch17), 28:28),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 31:31, pmc_offset(pmc_scratch17), 29:29),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc0), 7:0, pmc_offset(pmc_scratch18), 7:0),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc0), 14:8, pmc_offset(pmc_scratch18), 14:8),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc0), 20:16, pmc_offset(pmc_scratch18), 19:15),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc0), 26:21, pmc_offset(pmc_scratch18), 25:20),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc0), 27:27, pmc_offset(pmc_scratch18), 26:26),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc0), 30:28, pmc_offset(pmc_scratch18), 29:27),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl), 4:0, pmc_offset(pmc_scratch19), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl), 12:8, pmc_offset(pmc_scratch19), 9:5),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl), 18:14, pmc_offset(pmc_scratch19), 14:10),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl), 23:19, pmc_offset(pmc_scratch19), 19:15),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl), 27:24, pmc_offset(pmc_scratch19), 23:20),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl), 31:28, pmc_offset(pmc_scratch19), 27:24),
	pack(TYPE_SDRAM, sdram_offset(emc_rdr), 3:0, pmc_offset(pmc_scratch19), 31:28),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl), 4:0, pmc_offset(pmc_scratch22), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl), 12:8, pmc_offset(pmc_scratch22), 9:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl), 18:14, pmc_offset(pmc_scratch22), 14:10),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl), 23:19, pmc_offset(pmc_scratch22), 19:15),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl), 27:24, pmc_offset(pmc_scratch22), 23:20),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl), 31:28, pmc_offset(pmc_scratch22), 27:24),
	pack(TYPE_SDRAM, sdram_offset(emc_rext), 3:0, pmc_offset(pmc_scratch22), 31:28),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl), 8:4, pmc_offset(pmc_scratch23), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl), 13:9, pmc_offset(pmc_scratch23), 9:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl), 18:14, pmc_offset(pmc_scratch23), 14:10),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl), 23:19, pmc_offset(pmc_scratch23), 19:15),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl), 27:24, pmc_offset(pmc_scratch23), 23:20),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2dqpadctrl), 31:28, pmc_offset(pmc_scratch23), 27:24),
	pack(TYPE_SDRAM, sdram_offset(emc_wdv), 3:0, pmc_offset(pmc_scratch23), 31:28),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl), 8:4, pmc_offset(pmc_scratch24), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl), 13:9, pmc_offset(pmc_scratch24), 9:5),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl), 18:14, pmc_offset(pmc_scratch24), 14:10),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl), 23:19, pmc_offset(pmc_scratch24), 19:15),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl), 27:24, pmc_offset(pmc_scratch24), 23:20),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqpadctrl), 31:28, pmc_offset(pmc_scratch24), 27:24),
	pack(TYPE_SDRAM, sdram_offset(emc_burst_refresh_num), 3:0, pmc_offset(pmc_scratch24), 31:28),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 0:0, pmc_offset(pmc_scratch25), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 1:1, pmc_offset(pmc_scratch25), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 2:2, pmc_offset(pmc_scratch25), 2:2),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 6:6, pmc_offset(pmc_scratch25), 3:3),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 7:7, pmc_offset(pmc_scratch25), 4:4),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 9:8, pmc_offset(pmc_scratch25), 6:5),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 10:10, pmc_offset(pmc_scratch25), 7:7),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 11:11, pmc_offset(pmc_scratch25), 8:8),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 12:12, pmc_offset(pmc_scratch25), 9:9),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 13:13, pmc_offset(pmc_scratch25), 10:10),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 14:14, pmc_offset(pmc_scratch25), 11:11),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 15:15, pmc_offset(pmc_scratch25), 12:12),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 17:16, pmc_offset(pmc_scratch25), 14:13),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 21:20, pmc_offset(pmc_scratch25), 16:15),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 22:22, pmc_offset(pmc_scratch25), 17:17),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 23:23, pmc_offset(pmc_scratch25), 18:18),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 24:24, pmc_offset(pmc_scratch25), 19:19),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 25:25, pmc_offset(pmc_scratch25), 20:20),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 27:26, pmc_offset(pmc_scratch25), 22:21),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 28:28, pmc_offset(pmc_scratch25), 23:23),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 29:29, pmc_offset(pmc_scratch25), 24:24),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 30:30, pmc_offset(pmc_scratch25), 25:25),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg2), 31:31, pmc_offset(pmc_scratch25), 26:26),
	pack(TYPE_SDRAM, sdram_offset(emc_r2w), 4:0, pmc_offset(pmc_scratch25), 31:27),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2comppadctrl), 3:0, pmc_offset(pmc_scratch26), 3:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2comppadctrl), 7:5, pmc_offset(pmc_scratch26), 6:4),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2comppadctrl), 8:8, pmc_offset(pmc_scratch26), 7:7),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2comppadctrl), 9:9, pmc_offset(pmc_scratch26), 8:8),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2comppadctrl), 10:10, pmc_offset(pmc_scratch26), 9:9),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2comppadctrl), 11:11, pmc_offset(pmc_scratch26), 10:10),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2comppadctrl), 16:12, pmc_offset(pmc_scratch26), 15:11),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2comppadctrl), 24:20, pmc_offset(pmc_scratch26), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2comppadctrl), 31:28, pmc_offset(pmc_scratch26), 24:21),
	pack(TYPE_SDRAM, sdram_offset(emc_rc), 6:0, pmc_offset(pmc_scratch26), 31:25),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config), 4:0, pmc_offset(pmc_scratch27), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config), 12:8, pmc_offset(pmc_scratch27), 9:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config), 18:16, pmc_offset(pmc_scratch27), 12:10),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config), 25:20, pmc_offset(pmc_scratch27), 18:13),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config), 27:27, pmc_offset(pmc_scratch27), 19:19),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config), 28:28, pmc_offset(pmc_scratch27), 20:20),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config), 29:29, pmc_offset(pmc_scratch27), 21:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config), 30:30, pmc_offset(pmc_scratch27), 22:22),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config), 31:31, pmc_offset(pmc_scratch27), 23:23),
	pack(TYPE_SDRAM, sdram_offset(emc_quse_extra), 5:0, pmc_offset(pmc_scratch27), 29:24),
	pack(TYPE_SDRAM, sdram_offset(emc_quse_extra), 8:8, pmc_offset(pmc_scratch27), 30:30),
	pack(TYPE_SDRAM, sdram_offset(emc_quse_extra), 9:9, pmc_offset(pmc_scratch27), 31:31),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 4:0, pmc_offset(pmc_scratch28), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 12:8, pmc_offset(pmc_scratch28), 9:5),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 18:16, pmc_offset(pmc_scratch28), 12:10),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 25:20, pmc_offset(pmc_scratch28), 18:13),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 27:27, pmc_offset(pmc_scratch28), 19:19),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 28:28, pmc_offset(pmc_scratch28), 20:20),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 29:29, pmc_offset(pmc_scratch28), 21:21),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 30:30, pmc_offset(pmc_scratch28), 22:22),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 31:31, pmc_offset(pmc_scratch28), 23:23),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte_cfg), 1:0, pmc_offset(pmc_scratch28), 25:24),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte_cfg), 3:2, pmc_offset(pmc_scratch28), 27:26),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte_cfg), 5:4, pmc_offset(pmc_scratch28), 29:28),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte_cfg), 7:6, pmc_offset(pmc_scratch28), 31:30),
	pack(TYPE_SDRAM, sdram_offset(emc_zcal_interval), 23:10, pmc_offset(pmc_scratch29), 13:0),
	pack(TYPE_SDRAM, sdram_offset(emc_zcal_interval), 9:0, pmc_offset(pmc_scratch29), 23:14),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte_cfg), 1:0, pmc_offset(pmc_scratch29), 25:24),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte_cfg), 3:2, pmc_offset(pmc_scratch29), 27:26),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte_cfg), 5:4, pmc_offset(pmc_scratch29), 29:28),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte_cfg), 7:6, pmc_offset(pmc_scratch29), 31:30),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_da_covers), 7:0, pmc_offset(pmc_scratch30), 7:0),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_da_covers), 15:8, pmc_offset(pmc_scratch30), 15:8),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_da_covers), 23:16, pmc_offset(pmc_scratch30), 23:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte_cfg), 1:0, pmc_offset(pmc_scratch30), 25:24),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte_cfg), 3:2, pmc_offset(pmc_scratch30), 27:26),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte_cfg), 5:4, pmc_offset(pmc_scratch30), 29:28),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte_cfg), 7:6, pmc_offset(pmc_scratch30), 31:30),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 0:0, pmc_offset(pmc_scratch31), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 1:1, pmc_offset(pmc_scratch31), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 2:2, pmc_offset(pmc_scratch31), 2:2),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 3:3, pmc_offset(pmc_scratch31), 3:3),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 4:4, pmc_offset(pmc_scratch31), 4:4),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 5:5, pmc_offset(pmc_scratch31), 5:5),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 6:6, pmc_offset(pmc_scratch31), 6:6),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 7:7, pmc_offset(pmc_scratch31), 7:7),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 8:8, pmc_offset(pmc_scratch31), 8:8),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 9:9, pmc_offset(pmc_scratch31), 9:9),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 10:10, pmc_offset(pmc_scratch31), 10:10),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 11:11, pmc_offset(pmc_scratch31), 11:11),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 12:12, pmc_offset(pmc_scratch31), 12:12),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 13:13, pmc_offset(pmc_scratch31), 13:13),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 14:14, pmc_offset(pmc_scratch31), 14:14),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 15:15, pmc_offset(pmc_scratch31), 15:15),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 17:16, pmc_offset(pmc_scratch31), 17:16),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 19:18, pmc_offset(pmc_scratch31), 19:18),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 21:20, pmc_offset(pmc_scratch31), 21:20),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl2), 24:24, pmc_offset(pmc_scratch31), 22:22),
	pack(TYPE_SDRAM, sdram_offset(emc_rfc), 8:0, pmc_offset(pmc_scratch31), 31:23),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl), 0:0, pmc_offset(pmc_scratch32), 0:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl), 2:2, pmc_offset(pmc_scratch32), 1:1),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl), 3:3, pmc_offset(pmc_scratch32), 2:2),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl), 4:4, pmc_offset(pmc_scratch32), 3:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl), 7:7, pmc_offset(pmc_scratch32), 4:4),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl), 18:14, pmc_offset(pmc_scratch32), 9:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl), 23:19, pmc_offset(pmc_scratch32), 14:10),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl), 27:24, pmc_offset(pmc_scratch32), 18:15),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl), 31:28, pmc_offset(pmc_scratch32), 22:19),
	pack(TYPE_SDRAM, sdram_offset(emc_ar2pden), 8:0, pmc_offset(pmc_scratch32), 31:23),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl), 0:0, pmc_offset(pmc_scratch33), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl), 2:2, pmc_offset(pmc_scratch33), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl), 3:3, pmc_offset(pmc_scratch33), 2:2),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl), 4:4, pmc_offset(pmc_scratch33), 3:3),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl), 7:7, pmc_offset(pmc_scratch33), 4:4),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl), 18:14, pmc_offset(pmc_scratch33), 9:5),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl), 23:19, pmc_offset(pmc_scratch33), 14:10),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl), 27:24, pmc_offset(pmc_scratch33), 18:15),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl), 31:28, pmc_offset(pmc_scratch33), 22:19),
	pack(TYPE_SDRAM, sdram_offset(emc_rfc_slr), 8:0, pmc_offset(pmc_scratch33), 31:23),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl3), 0:0, pmc_offset(pmc_scratch40), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl3), 5:5, pmc_offset(pmc_scratch40), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl3), 12:8, pmc_offset(pmc_scratch40), 6:2),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl3), 18:14, pmc_offset(pmc_scratch40), 11:7),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl3), 24:20, pmc_offset(pmc_scratch40), 16:12),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl3), 30:26, pmc_offset(pmc_scratch40), 21:17),
	pack(TYPE_SDRAM, sdram_offset(emc_txsr), 9:0, pmc_offset(pmc_scratch40), 31:22),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl3), 0:0, pmc_offset(pmc_scratch42), 0:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl3), 5:5, pmc_offset(pmc_scratch42), 1:1),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl3), 12:8, pmc_offset(pmc_scratch42), 6:2),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl3), 18:14, pmc_offset(pmc_scratch42), 11:7),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl3), 24:20, pmc_offset(pmc_scratch42), 16:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl3), 30:26, pmc_offset(pmc_scratch42), 21:17),
	pack(TYPE_SDRAM, sdram_offset(emc_mc2emcq), 2:0, pmc_offset(pmc_scratch42), 24:22),
	pack(TYPE_SDRAM, sdram_offset(emc_mc2emcq), 10:8, pmc_offset(pmc_scratch42), 27:25),
	pack(TYPE_SDRAM, sdram_offset(emc_mc2emcq), 27:24, pmc_offset(pmc_scratch42), 31:28),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_cfg), 8:0, pmc_offset(pmc_scratch44), 8:0),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_cfg), 20:16, pmc_offset(pmc_scratch44), 13:9),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_cfg), 27:24, pmc_offset(pmc_scratch44), 17:14),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_cfg), 31:28, pmc_offset(pmc_scratch44), 21:18),
	pack(TYPE_SDRAM, sdram_offset(emc_zcal_wait_cnt), 9:0, pmc_offset(pmc_scratch44), 31:22),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_interval), 20:0, pmc_offset(pmc_scratch45), 20:0),
	pack(TYPE_SDRAM, sdram_offset(emc_odt_write), 2:0, pmc_offset(pmc_scratch45), 23:21),
	pack(TYPE_SDRAM, sdram_offset(emc_odt_write), 4:4, pmc_offset(pmc_scratch45), 24:24),
	pack(TYPE_SDRAM, sdram_offset(emc_odt_write), 5:5, pmc_offset(pmc_scratch45), 25:25),
	pack(TYPE_SDRAM, sdram_offset(emc_odt_write), 11:8, pmc_offset(pmc_scratch45), 29:26),
	pack(TYPE_SDRAM, sdram_offset(emc_odt_write), 30:30, pmc_offset(pmc_scratch45), 30:30),
	pack(TYPE_SDRAM, sdram_offset(emc_odt_write), 31:31, pmc_offset(pmc_scratch45), 31:31),
	pack(TYPE_SDRAM, sdram_offset(emc_mrs_wait_cnt2), 9:0, pmc_offset(pmc_scratch46), 9:0),
	pack(TYPE_SDRAM, sdram_offset(emc_mrs_wait_cnt2), 25:16, pmc_offset(pmc_scratch46), 19:10),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc1), 19:19, pmc_offset(pmc_scratch46), 20:20),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc1), 20:20, pmc_offset(pmc_scratch46), 21:21),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc1), 23:21, pmc_offset(pmc_scratch46), 24:22),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc1), 25:24, pmc_offset(pmc_scratch46), 26:25),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc1), 27:27, pmc_offset(pmc_scratch46), 27:27),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc1), 31:28, pmc_offset(pmc_scratch46), 31:28),
	pack(TYPE_SDRAM, sdram_offset(emc_mrs_wait_cnt), 9:0, pmc_offset(pmc_scratch47), 9:0),
	pack(TYPE_SDRAM, sdram_offset(emc_mrs_wait_cnt), 25:16, pmc_offset(pmc_scratch47), 19:10),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_outstanding_req), 8:0, pmc_offset(pmc_scratch47), 28:20),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_outstanding_req), 30:30, pmc_offset(pmc_scratch47), 29:29),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_outstanding_req), 31:31, pmc_offset(pmc_scratch47), 30:30),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl4), 22:18, pmc_offset(pmc_scratch48), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl4), 16:12, pmc_offset(pmc_scratch48), 9:5),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl4), 10:6, pmc_offset(pmc_scratch48), 14:10),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2dqspadctrl4), 4:0, pmc_offset(pmc_scratch48), 19:15),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl2), 4:0, pmc_offset(pmc_scratch48), 24:20),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2clkpadctrl2), 12:8, pmc_offset(pmc_scratch48), 29:25),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl4), 22:18, pmc_offset(pmc_scratch50), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl4), 16:12, pmc_offset(pmc_scratch50), 9:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl4), 10:6, pmc_offset(pmc_scratch50), 14:10),
	pack(TYPE_SDRAM, sdram_offset(ch1_cmc_xm2dqspadctrl4), 4:0, pmc_offset(pmc_scratch50), 19:15),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl2), 4:0, pmc_offset(pmc_scratch50), 24:20),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2clkpadctrl2), 12:8, pmc_offset(pmc_scratch50), 29:25),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config2), 4:0, pmc_offset(pmc_scratch51), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config2), 12:8, pmc_offset(pmc_scratch51), 9:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config2), 20:16, pmc_offset(pmc_scratch51), 14:10),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config2), 28:24, pmc_offset(pmc_scratch51), 19:15),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config3), 4:0, pmc_offset(pmc_scratch51), 24:20),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_auto_cal_config3), 12:8, pmc_offset(pmc_scratch51), 29:25),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config2), 4:0, pmc_offset(pmc_scratch56), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config2), 12:8, pmc_offset(pmc_scratch56), 9:5),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config2), 20:16, pmc_offset(pmc_scratch56), 14:10),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config2), 28:24, pmc_offset(pmc_scratch56), 19:15),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config3), 4:0, pmc_offset(pmc_scratch56), 24:20),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_config3), 12:8, pmc_offset(pmc_scratch56), 29:25),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 1:1, pmc_offset(pmc_scratch57), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 2:2, pmc_offset(pmc_scratch57), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 3:3, pmc_offset(pmc_scratch57), 2:2),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 4:4, pmc_offset(pmc_scratch57), 3:3),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 5:5, pmc_offset(pmc_scratch57), 4:4),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 6:6, pmc_offset(pmc_scratch57), 5:5),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 18:18, pmc_offset(pmc_scratch57), 6:6),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 19:19, pmc_offset(pmc_scratch57), 7:7),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 20:20, pmc_offset(pmc_scratch57), 8:8),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 21:21, pmc_offset(pmc_scratch57), 9:9),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 22:22, pmc_offset(pmc_scratch57), 10:10),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 23:23, pmc_offset(pmc_scratch57), 11:11),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 24:24, pmc_offset(pmc_scratch57), 12:12),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 25:25, pmc_offset(pmc_scratch57), 13:13),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 28:28, pmc_offset(pmc_scratch57), 14:14),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 29:29, pmc_offset(pmc_scratch57), 15:15),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 30:30, pmc_offset(pmc_scratch57), 16:16),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg), 31:31, pmc_offset(pmc_scratch57), 17:17),
	pack(TYPE_SDRAM, sdram_offset(emc_trefbw), 13:0, pmc_offset(pmc_scratch57), 31:18),
	pack(TYPE_SDRAM, sdram_offset(emc_zcal_mrw_cmd), 7:0, pmc_offset(pmc_scratch58), 7:0),
	pack(TYPE_SDRAM, sdram_offset(emc_zcal_mrw_cmd), 23:16, pmc_offset(pmc_scratch58), 15:8),
	pack(TYPE_SDRAM, sdram_offset(emc_zcal_mrw_cmd), 31:30, pmc_offset(pmc_scratch58), 17:16),
	pack(TYPE_SDRAM, sdram_offset(emc_puterm_extra), 5:0, pmc_offset(pmc_scratch58), 23:18),
	pack(TYPE_SDRAM, sdram_offset(emc_puterm_extra), 8:8, pmc_offset(pmc_scratch58), 24:24),
	pack(TYPE_SDRAM, sdram_offset(emc_puterm_extra), 9:9, pmc_offset(pmc_scratch58), 25:25),
	pack(TYPE_SDRAM, sdram_offset(emc_puterm_extra), 21:16, pmc_offset(pmc_scratch58), 31:26),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl2), 18:14, pmc_offset(pmc_scratch59), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl2), 23:19, pmc_offset(pmc_scratch59), 9:5),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl2), 27:24, pmc_offset(pmc_scratch59), 13:10),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl2), 31:28, pmc_offset(pmc_scratch59), 17:14),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2vttgenpadctrl), 2:2, pmc_offset(pmc_scratch59), 18:18),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2vttgenpadctrl), 10:8, pmc_offset(pmc_scratch59), 21:19),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2vttgenpadctrl), 14:12, pmc_offset(pmc_scratch59), 24:22),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2vttgenpadctrl), 18:16, pmc_offset(pmc_scratch59), 27:25),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2vttgenpadctrl), 26:24, pmc_offset(pmc_scratch59), 30:28),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl2), 18:14, pmc_offset(pmc_scratch60), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl2), 23:19, pmc_offset(pmc_scratch60), 9:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl2), 27:24, pmc_offset(pmc_scratch60), 13:10),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl2), 31:28, pmc_offset(pmc_scratch60), 17:14),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 27:27, pmc_offset(pmc_scratch60), 18:18),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 26:26, pmc_offset(pmc_scratch60), 19:19),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 24:24, pmc_offset(pmc_scratch60), 20:20),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 16:16, pmc_offset(pmc_scratch60), 21:21),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 10:10, pmc_offset(pmc_scratch60), 22:22),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 7:7, pmc_offset(pmc_scratch60), 23:23),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 6:6, pmc_offset(pmc_scratch60), 24:24),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 5:5, pmc_offset(pmc_scratch60), 25:25),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 4:4, pmc_offset(pmc_scratch60), 26:26),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 3:3, pmc_offset(pmc_scratch60), 27:27),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 2:2, pmc_offset(pmc_scratch60), 28:28),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 1:1, pmc_offset(pmc_scratch60), 29:29),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 0:0, pmc_offset(pmc_scratch60), 30:30),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 6:6, pmc_offset(pmc_scratch60), 31:31),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte0), 2:0, pmc_offset(pmc_scratch61), 2:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte0), 6:4, pmc_offset(pmc_scratch61), 5:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte0), 10:8, pmc_offset(pmc_scratch61), 8:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte0), 14:12, pmc_offset(pmc_scratch61), 11:9),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte0), 18:16, pmc_offset(pmc_scratch61), 14:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte0), 22:20, pmc_offset(pmc_scratch61), 17:15),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_ring1_throttle), 4:0, pmc_offset(pmc_scratch61), 22:18),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_ring1_throttle), 20:16, pmc_offset(pmc_scratch61), 27:23),
	pack(TYPE_SDRAM, sdram_offset(emc_wext), 3:0, pmc_offset(pmc_scratch61), 31:28),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte0), 2:0, pmc_offset(pmc_scratch62), 2:0),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte0), 6:4, pmc_offset(pmc_scratch62), 5:3),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte0), 10:8, pmc_offset(pmc_scratch62), 8:6),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte0), 14:12, pmc_offset(pmc_scratch62), 11:9),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte0), 18:16, pmc_offset(pmc_scratch62), 14:12),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte0), 22:20, pmc_offset(pmc_scratch62), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_ctt_term_ctrl), 2:0, pmc_offset(pmc_scratch62), 20:18),
	pack(TYPE_SDRAM, sdram_offset(emc_ctt_term_ctrl), 12:8, pmc_offset(pmc_scratch62), 25:21),
	pack(TYPE_SDRAM, sdram_offset(emc_ctt_term_ctrl), 31:31, pmc_offset(pmc_scratch62), 26:26),
	pack(TYPE_SDRAM, sdram_offset(emc_w2r), 4:0, pmc_offset(pmc_scratch62), 31:27),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte1), 2:0, pmc_offset(pmc_scratch63), 2:0),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte1), 6:4, pmc_offset(pmc_scratch63), 5:3),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte1), 10:8, pmc_offset(pmc_scratch63), 8:6),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte1), 14:12, pmc_offset(pmc_scratch63), 11:9),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte1), 18:16, pmc_offset(pmc_scratch63), 14:12),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte1), 22:20, pmc_offset(pmc_scratch63), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 2:2, pmc_offset(pmc_scratch63), 18:18),
	pack(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 3:3, pmc_offset(pmc_scratch63), 19:19),
	pack(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 4:4, pmc_offset(pmc_scratch63), 20:20),
	pack(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 5:5, pmc_offset(pmc_scratch63), 21:21),
	pack(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 8:8, pmc_offset(pmc_scratch63), 22:22),
	pack(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 9:9, pmc_offset(pmc_scratch63), 23:23),
	pack(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 18:16, pmc_offset(pmc_scratch63), 26:24),
	pack(TYPE_SDRAM, sdram_offset(emc_r2p), 4:0, pmc_offset(pmc_scratch63), 31:27),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte1), 2:0, pmc_offset(pmc_scratch64), 2:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte1), 6:4, pmc_offset(pmc_scratch64), 5:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte1), 10:8, pmc_offset(pmc_scratch64), 8:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte1), 14:12, pmc_offset(pmc_scratch64), 11:9),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte1), 18:16, pmc_offset(pmc_scratch64), 14:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte1), 22:20, pmc_offset(pmc_scratch64), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte_cfg), 1:0, pmc_offset(pmc_scratch64), 19:18),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte_cfg), 3:2, pmc_offset(pmc_scratch64), 21:20),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte_cfg), 5:4, pmc_offset(pmc_scratch64), 23:22),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte_cfg), 7:6, pmc_offset(pmc_scratch64), 25:24),
	pack(TYPE_SDRAM, sdram_offset(emc_ras), 5:0, pmc_offset(pmc_scratch64), 31:26),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte2), 2:0, pmc_offset(pmc_scratch65), 2:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte2), 6:4, pmc_offset(pmc_scratch65), 5:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte2), 10:8, pmc_offset(pmc_scratch65), 8:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte2), 14:12, pmc_offset(pmc_scratch65), 11:9),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte2), 18:16, pmc_offset(pmc_scratch65), 14:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte2), 22:20, pmc_offset(pmc_scratch65), 17:15),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_rsv), 7:0, pmc_offset(pmc_scratch65), 25:18),
	pack(TYPE_SDRAM, sdram_offset(emc_rp), 5:0, pmc_offset(pmc_scratch65), 31:26),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte2), 2:0, pmc_offset(pmc_scratch66), 2:0),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte2), 6:4, pmc_offset(pmc_scratch66), 5:3),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte2), 10:8, pmc_offset(pmc_scratch66), 8:6),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte2), 14:12, pmc_offset(pmc_scratch66), 11:9),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte2), 18:16, pmc_offset(pmc_scratch66), 14:12),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte2), 22:20, pmc_offset(pmc_scratch66), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_w2p), 5:0, pmc_offset(pmc_scratch66), 31:26),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte3), 2:0, pmc_offset(pmc_scratch67), 2:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte3), 6:4, pmc_offset(pmc_scratch67), 5:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte3), 10:8, pmc_offset(pmc_scratch67), 8:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte3), 14:12, pmc_offset(pmc_scratch67), 11:9),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte3), 18:16, pmc_offset(pmc_scratch67), 14:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank0_byte3), 22:20, pmc_offset(pmc_scratch67), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_rd_rcd), 5:0, pmc_offset(pmc_scratch67), 31:26),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte3), 2:0, pmc_offset(pmc_scratch68), 2:0),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte3), 6:4, pmc_offset(pmc_scratch68), 5:3),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte3), 10:8, pmc_offset(pmc_scratch68), 8:6),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte3), 14:12, pmc_offset(pmc_scratch68), 11:9),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte3), 18:16, pmc_offset(pmc_scratch68), 14:12),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank0_byte3), 22:20, pmc_offset(pmc_scratch68), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_wr_rcd), 5:0, pmc_offset(pmc_scratch68), 31:26),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte0), 2:0, pmc_offset(pmc_scratch69), 2:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte0), 6:4, pmc_offset(pmc_scratch69), 5:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte0), 10:8, pmc_offset(pmc_scratch69), 8:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte0), 14:12, pmc_offset(pmc_scratch69), 11:9),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte0), 18:16, pmc_offset(pmc_scratch69), 14:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte0), 22:20, pmc_offset(pmc_scratch69), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_quse), 5:0, pmc_offset(pmc_scratch69), 31:26),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte0), 2:0, pmc_offset(pmc_scratch70), 2:0),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte0), 6:4, pmc_offset(pmc_scratch70), 5:3),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte0), 10:8, pmc_offset(pmc_scratch70), 8:6),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte0), 14:12, pmc_offset(pmc_scratch70), 11:9),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte0), 18:16, pmc_offset(pmc_scratch70), 14:12),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte0), 22:20, pmc_offset(pmc_scratch70), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_qrst), 5:0, pmc_offset(pmc_scratch70), 31:26),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte1), 2:0, pmc_offset(pmc_scratch71), 2:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte1), 6:4, pmc_offset(pmc_scratch71), 5:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte1), 10:8, pmc_offset(pmc_scratch71), 8:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte1), 14:12, pmc_offset(pmc_scratch71), 11:9),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte1), 18:16, pmc_offset(pmc_scratch71), 14:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte1), 22:20, pmc_offset(pmc_scratch71), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_tfaw), 6:0, pmc_offset(pmc_scratch71), 24:18),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dli_trim_txdqs0), 6:0, pmc_offset(pmc_scratch71), 31:25),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte1), 2:0, pmc_offset(pmc_scratch72), 2:0),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte1), 6:4, pmc_offset(pmc_scratch72), 5:3),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte1), 10:8, pmc_offset(pmc_scratch72), 8:6),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte1), 14:12, pmc_offset(pmc_scratch72), 11:9),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte1), 18:16, pmc_offset(pmc_scratch72), 14:12),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte1), 22:20, pmc_offset(pmc_scratch72), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs0), 6:0, pmc_offset(pmc_scratch72), 24:18),
	pack(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs1), 6:0, pmc_offset(pmc_scratch72), 31:25),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte2), 2:0, pmc_offset(pmc_scratch73), 2:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte2), 6:4, pmc_offset(pmc_scratch73), 5:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte2), 10:8, pmc_offset(pmc_scratch73), 8:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte2), 14:12, pmc_offset(pmc_scratch73), 11:9),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte2), 18:16, pmc_offset(pmc_scratch73), 14:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte2), 22:20, pmc_offset(pmc_scratch73), 17:15),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dli_trim_txdqs1), 6:0, pmc_offset(pmc_scratch73), 24:18),
	pack(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs2), 6:0, pmc_offset(pmc_scratch73), 31:25),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte2), 2:0, pmc_offset(pmc_scratch74), 2:0),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte2), 6:4, pmc_offset(pmc_scratch74), 5:3),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte2), 10:8, pmc_offset(pmc_scratch74), 8:6),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte2), 14:12, pmc_offset(pmc_scratch74), 11:9),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte2), 18:16, pmc_offset(pmc_scratch74), 14:12),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte2), 22:20, pmc_offset(pmc_scratch74), 17:15),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dli_trim_txdqs2), 6:0, pmc_offset(pmc_scratch74), 24:18),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dli_trim_txdqs3), 6:0, pmc_offset(pmc_scratch74), 31:25),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte3), 2:0, pmc_offset(pmc_scratch75), 2:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte3), 6:4, pmc_offset(pmc_scratch75), 5:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte3), 10:8, pmc_offset(pmc_scratch75), 8:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte3), 14:12, pmc_offset(pmc_scratch75), 11:9),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte3), 18:16, pmc_offset(pmc_scratch75), 14:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_swizzle_rank1_byte3), 22:20, pmc_offset(pmc_scratch75), 17:15),
	pack(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs3), 6:0, pmc_offset(pmc_scratch75), 24:18),
	pack(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs4), 6:0, pmc_offset(pmc_scratch75), 31:25),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte3), 2:0, pmc_offset(pmc_scratch76), 2:0),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte3), 6:4, pmc_offset(pmc_scratch76), 5:3),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte3), 10:8, pmc_offset(pmc_scratch76), 8:6),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte3), 14:12, pmc_offset(pmc_scratch76), 11:9),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte3), 18:16, pmc_offset(pmc_scratch76), 14:12),
	pack(TYPE_SDRAM, sdram_offset(emc_swizzle_rank1_byte3), 22:20, pmc_offset(pmc_scratch76), 17:15),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dli_trim_txdqs4), 6:0, pmc_offset(pmc_scratch76), 24:18),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dli_trim_txdqs5), 6:0, pmc_offset(pmc_scratch76), 31:25),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl3), 18:14, pmc_offset(pmc_scratch77), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl3), 23:19, pmc_offset(pmc_scratch77), 9:5),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl3), 27:24, pmc_offset(pmc_scratch77), 13:10),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl3), 31:28, pmc_offset(pmc_scratch77), 17:14),
	pack(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs5), 6:0, pmc_offset(pmc_scratch77), 24:18),
	pack(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs6), 6:0, pmc_offset(pmc_scratch77), 31:25),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl3), 18:14, pmc_offset(pmc_scratch78), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl3), 23:19, pmc_offset(pmc_scratch78), 9:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl3), 27:24, pmc_offset(pmc_scratch78), 13:10),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl3), 31:28, pmc_offset(pmc_scratch78), 17:14),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dli_trim_txdqs6), 6:0, pmc_offset(pmc_scratch78), 24:18),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dli_trim_txdqs7), 6:0, pmc_offset(pmc_scratch78), 31:25),
	pack(TYPE_SDRAM, sdram_offset(emc_dyn_self_ref_control), 15:0, pmc_offset(pmc_scratch79), 15:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dyn_self_ref_control), 31:31, pmc_offset(pmc_scratch79), 16:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs7), 6:0, pmc_offset(pmc_scratch79), 23:17),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_rp), 6:0, pmc_offset(pmc_scratch79), 30:24),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 7:7, pmc_offset(pmc_scratch79), 31:31),
	pack(TYPE_SDRAM, sdram_offset(emc_refresh), 5:0, pmc_offset(pmc_scratch80), 5:0),
	pack(TYPE_SDRAM, sdram_offset(emc_refresh), 15:6, pmc_offset(pmc_scratch80), 15:6),
	pack(TYPE_SDRAM, sdram_offset(emc_cmdq), 4:0, pmc_offset(pmc_scratch80), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_cmdq), 10:8, pmc_offset(pmc_scratch80), 23:21),
	pack(TYPE_SDRAM, sdram_offset(emc_cmdq), 14:12, pmc_offset(pmc_scratch80), 26:24),
	pack(TYPE_SDRAM, sdram_offset(emc_cmdq), 28:24, pmc_offset(pmc_scratch80), 31:27),
	pack(TYPE_SDRAM, sdram_offset(emc_acpd_control), 15:0, pmc_offset(pmc_scratch81), 15:0),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll_period), 15:0, pmc_offset(pmc_scratch81), 31:16),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 3:3, pmc_offset(pmc_scratch82), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 4:4, pmc_offset(pmc_scratch82), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 5:5, pmc_offset(pmc_scratch82), 2:2),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 6:6, pmc_offset(pmc_scratch82), 3:3),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 7:7, pmc_offset(pmc_scratch82), 4:4),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 9:9, pmc_offset(pmc_scratch82), 5:5),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 10:10, pmc_offset(pmc_scratch82), 6:6),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 14:12, pmc_offset(pmc_scratch82), 9:7),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 18:16, pmc_offset(pmc_scratch82), 12:10),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl), 22:20, pmc_offset(pmc_scratch82), 15:13),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs0), 4:0, pmc_offset(pmc_scratch82), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs0), 22:12, pmc_offset(pmc_scratch82), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqso), 4:0, pmc_offset(pmc_scratch83), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqso), 22:12, pmc_offset(pmc_scratch83), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs1), 4:0, pmc_offset(pmc_scratch83), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs1), 22:12, pmc_offset(pmc_scratch83), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs1), 4:0, pmc_offset(pmc_scratch84), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs1), 22:12, pmc_offset(pmc_scratch84), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs2), 4:0, pmc_offset(pmc_scratch84), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs2), 22:12, pmc_offset(pmc_scratch84), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs2), 4:0, pmc_offset(pmc_scratch85), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs2), 22:12, pmc_offset(pmc_scratch85), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs3), 4:0, pmc_offset(pmc_scratch85), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs3), 22:12, pmc_offset(pmc_scratch85), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs3), 4:0, pmc_offset(pmc_scratch86), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs3), 22:12, pmc_offset(pmc_scratch86), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs4), 4:0, pmc_offset(pmc_scratch86), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs4), 22:12, pmc_offset(pmc_scratch86), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs4), 4:0, pmc_offset(pmc_scratch87), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs4), 22:12, pmc_offset(pmc_scratch87), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs5), 4:0, pmc_offset(pmc_scratch87), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs5), 22:12, pmc_offset(pmc_scratch87), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs5), 4:0, pmc_offset(pmc_scratch88), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs5), 22:12, pmc_offset(pmc_scratch88), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs6), 4:0, pmc_offset(pmc_scratch88), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs6), 22:12, pmc_offset(pmc_scratch88), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs6), 4:0, pmc_offset(pmc_scratch89), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs6), 22:12, pmc_offset(pmc_scratch89), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs7), 4:0, pmc_offset(pmc_scratch89), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs7), 22:12, pmc_offset(pmc_scratch89), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs7), 4:0, pmc_offset(pmc_scratch90), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dqs7), 22:12, pmc_offset(pmc_scratch90), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse0), 4:0, pmc_offset(pmc_scratch90), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse0), 22:12, pmc_offset(pmc_scratch90), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse0), 4:0, pmc_offset(pmc_scratch91), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse0), 22:12, pmc_offset(pmc_scratch91), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse1), 4:0, pmc_offset(pmc_scratch91), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse1), 22:12, pmc_offset(pmc_scratch91), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse1), 4:0, pmc_offset(pmc_scratch92), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse1), 22:12, pmc_offset(pmc_scratch92), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse2), 4:0, pmc_offset(pmc_scratch92), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse2), 22:12, pmc_offset(pmc_scratch92), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse2), 4:0, pmc_offset(pmc_scratch93), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse2), 22:12, pmc_offset(pmc_scratch93), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse3), 4:0, pmc_offset(pmc_scratch93), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse3), 22:12, pmc_offset(pmc_scratch93), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse3), 4:0, pmc_offset(pmc_scratch94), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse3), 22:12, pmc_offset(pmc_scratch94), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse4), 4:0, pmc_offset(pmc_scratch94), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse4), 22:12, pmc_offset(pmc_scratch94), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse4), 4:0, pmc_offset(pmc_scratch95), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse4), 22:12, pmc_offset(pmc_scratch95), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse5), 4:0, pmc_offset(pmc_scratch95), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse5), 22:12, pmc_offset(pmc_scratch95), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse5), 4:0, pmc_offset(pmc_scratch96), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse5), 22:12, pmc_offset(pmc_scratch96), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse6), 4:0, pmc_offset(pmc_scratch96), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse6), 22:12, pmc_offset(pmc_scratch96), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse6), 4:0, pmc_offset(pmc_scratch97), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse6), 22:12, pmc_offset(pmc_scratch97), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse7), 4:0, pmc_offset(pmc_scratch97), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_quse7), 22:12, pmc_offset(pmc_scratch97), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse7), 4:0, pmc_offset(pmc_scratch98), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse7), 22:12, pmc_offset(pmc_scratch98), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq0), 4:0, pmc_offset(pmc_scratch98), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq0), 22:12, pmc_offset(pmc_scratch98), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dq0), 4:0, pmc_offset(pmc_scratch99), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dq0), 22:12, pmc_offset(pmc_scratch99), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq1), 4:0, pmc_offset(pmc_scratch99), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq1), 22:12, pmc_offset(pmc_scratch99), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dq1), 4:0, pmc_offset(pmc_scratch100), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dq1), 22:12, pmc_offset(pmc_scratch100), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq2), 4:0, pmc_offset(pmc_scratch100), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq2), 22:12, pmc_offset(pmc_scratch100), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dq2), 4:0, pmc_offset(pmc_scratch101), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dq2), 22:12, pmc_offset(pmc_scratch101), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq3), 4:0, pmc_offset(pmc_scratch101), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq3), 22:12, pmc_offset(pmc_scratch101), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dq3), 4:0, pmc_offset(pmc_scratch102), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_dq3), 22:12, pmc_offset(pmc_scratch102), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_pre_refresh_req_cnt), 15:0, pmc_offset(pmc_scratch102), 31:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_cdb_cntl_1), 1:0, pmc_offset(pmc_scratch103), 1:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_cdb_cntl_1), 4:3, pmc_offset(pmc_scratch103), 3:2),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_cdb_cntl_1), 7:6, pmc_offset(pmc_scratch103), 5:4),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_cdb_cntl_1), 10:9, pmc_offset(pmc_scratch103), 7:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_cdb_cntl_1), 13:12, pmc_offset(pmc_scratch103), 9:8),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_cdb_cntl_1), 16:15, pmc_offset(pmc_scratch103), 11:10),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_cdb_cntl_1), 19:18, pmc_offset(pmc_scratch103), 13:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_cdb_cntl_1), 22:21, pmc_offset(pmc_scratch103), 15:14),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl1), 1:0, pmc_offset(pmc_scratch103), 17:16),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl1), 4:3, pmc_offset(pmc_scratch103), 19:18),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl1), 7:6, pmc_offset(pmc_scratch103), 21:20),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl1), 10:9, pmc_offset(pmc_scratch103), 23:22),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl1), 13:12, pmc_offset(pmc_scratch103), 25:24),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl1), 16:15, pmc_offset(pmc_scratch103), 27:26),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl1), 19:18, pmc_offset(pmc_scratch103), 29:28),
	pack(TYPE_SDRAM, sdram_offset(emc_cdb_cntl1), 22:21, pmc_offset(pmc_scratch103), 31:30),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_addr0), 4:0, pmc_offset(pmc_scratch104), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_addr0), 22:12, pmc_offset(pmc_scratch104), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_addr0), 4:0, pmc_offset(pmc_scratch104), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_addr0), 22:12, pmc_offset(pmc_scratch104), 31:21),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_addr1), 4:0, pmc_offset(pmc_scratch105), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_addr1), 22:12, pmc_offset(pmc_scratch105), 15:5),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_addr1), 4:0, pmc_offset(pmc_scratch105), 20:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_addr1), 22:12, pmc_offset(pmc_scratch105), 31:21),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_addr2), 4:0, pmc_offset(pmc_scratch106), 4:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_dll_xform_addr2), 22:12, pmc_offset(pmc_scratch106), 15:5),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_addr2), 4:0, pmc_offset(pmc_scratch106), 20:16),
	pack(TYPE_SDRAM, sdram_offset(emc_dll_xform_addr2), 22:12, pmc_offset(pmc_scratch106), 31:21),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_rc), 6:0, pmc_offset(pmc_scratch108), 6:0),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_faw), 6:0, pmc_offset(pmc_scratch108), 13:7),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_wap2pre), 6:0, pmc_offset(pmc_scratch108), 20:14),
	pack(TYPE_SDRAM, sdram_offset(emc_rdv), 5:0, pmc_offset(pmc_scratch108), 26:21),
	pack(TYPE_SDRAM, sdram_offset(emc_qsafe), 4:0, pmc_offset(pmc_scratch108), 31:27),
	pack(TYPE_SDRAM, sdram_offset(emv_pdex2wr), 5:0, pmc_offset(pmc_scratch109), 5:0),
	pack(TYPE_SDRAM, sdram_offset(emc_pdex2rd), 5:0, pmc_offset(pmc_scratch109), 11:6),
	pack(TYPE_SDRAM, sdram_offset(emc_pchg2pden), 5:0, pmc_offset(pmc_scratch109), 17:12),
	pack(TYPE_SDRAM, sdram_offset(emc_act2pden), 5:0, pmc_offset(pmc_scratch109), 23:18),
	pack(TYPE_SDRAM, sdram_offset(emc_rw2pden), 5:0, pmc_offset(pmc_scratch109), 29:24),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 1:0, pmc_offset(pmc_scratch109), 31:30),
	pack(TYPE_SDRAM, sdram_offset(emc_tcke), 5:0, pmc_offset(pmc_scratch110), 5:0),
	pack(TYPE_SDRAM, sdram_offset(emc_trpab), 5:0, pmc_offset(pmc_scratch110), 11:6),
	pack(TYPE_SDRAM, sdram_offset(emc_ctt), 5:0, pmc_offset(pmc_scratch110), 17:12),
	pack(TYPE_SDRAM, sdram_offset(emc_einput), 5:0, pmc_offset(pmc_scratch110), 23:18),
	pack(TYPE_SDRAM, sdram_offset(emc_tckesr), 5:0, pmc_offset(pmc_scratch110), 29:24),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 3:2, pmc_offset(pmc_scratch110), 31:30),
	pack(TYPE_SDRAM, sdram_offset(emc_tpd), 5:0, pmc_offset(pmc_scratch111), 5:0),
	pack(TYPE_SDRAM, sdram_offset(emc_rdv_mask), 5:0, pmc_offset(pmc_scratch111), 11:6),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2vttgenpadctrl2), 5:0, pmc_offset(pmc_scratch111), 17:12),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl4), 0:0, pmc_offset(pmc_scratch111), 18:18),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl4), 2:2, pmc_offset(pmc_scratch111), 19:19),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl4), 4:4, pmc_offset(pmc_scratch111), 20:20),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl4), 6:6, pmc_offset(pmc_scratch111), 21:21),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl4), 8:8, pmc_offset(pmc_scratch111), 22:22),
	pack(TYPE_SDRAM, sdram_offset(emc_xm2cmdpadctrl4), 10:10, pmc_offset(pmc_scratch111), 23:23),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl4), 0:0, pmc_offset(pmc_scratch111), 24:24),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl4), 2:2, pmc_offset(pmc_scratch111), 25:25),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl4), 4:4, pmc_offset(pmc_scratch111), 26:26),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl4), 6:6, pmc_offset(pmc_scratch111), 27:27),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl4), 8:8, pmc_offset(pmc_scratch111), 28:28),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_xm2cmdpadctrl4), 10:10, pmc_offset(pmc_scratch111), 29:29),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 8:8, pmc_offset(pmc_scratch111), 30:30),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 9:9, pmc_offset(pmc_scratch111), 31:31),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_rcd), 5:0, pmc_offset(pmc_scratch112), 5:0),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_ras), 5:0, pmc_offset(pmc_scratch112), 11:6),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_rap2pre), 5:0, pmc_offset(pmc_scratch112), 17:12),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_r2w), 5:0, pmc_offset(pmc_scratch112), 23:18),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_w2r), 5:0, pmc_offset(pmc_scratch112), 29:24),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 10:10, pmc_offset(pmc_scratch112), 30:30),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 11:11, pmc_offset(pmc_scratch112), 31:31),
	pack(TYPE_SDRAM, sdram_offset(emc_tclkstable), 4:0, pmc_offset(pmc_scratch113), 4:0),
	pack(TYPE_SDRAM, sdram_offset(emc_tclkstop), 4:0, pmc_offset(pmc_scratch113), 9:5),
	pack(TYPE_SDRAM, sdram_offset(emc_ibdly), 4:0, pmc_offset(pmc_scratch113), 14:10),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_r2r), 4:0, pmc_offset(pmc_scratch113), 19:15),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_w2w), 4:0, pmc_offset(pmc_scratch113), 24:20),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_channel_mask_propag), 4:0, pmc_offset(pmc_scratch113), 29:25),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 12:12, pmc_offset(pmc_scratch113), 30:30),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 16:16, pmc_offset(pmc_scratch113), 31:31),
	pack(TYPE_SDRAM, sdram_offset(emc_clken_override), 1:1, pmc_offset(pmc_scratch114), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_clken_override), 2:2, pmc_offset(pmc_scratch114), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_clken_override), 3:3, pmc_offset(pmc_scratch114), 2:2),
	pack(TYPE_SDRAM, sdram_offset(emc_clken_override), 6:6, pmc_offset(pmc_scratch114), 3:3),
	pack(TYPE_SDRAM, sdram_offset(emc_r2r), 3:0, pmc_offset(pmc_scratch114), 7:4),
	pack(TYPE_SDRAM, sdram_offset(emc_w2w), 3:0, pmc_offset(pmc_scratch114), 11:8),
	pack(TYPE_SDRAM, sdram_offset(emc_wdv_mask), 3:0, pmc_offset(pmc_scratch114), 15:12),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_arb_timing_rrd), 3:0, pmc_offset(pmc_scratch114), 19:16),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg6), 2:0, pmc_offset(pmc_scratch114), 22:20),
	pack(TYPE_SDRAM, sdram_offset(emc_einput_duration), 2:0, pmc_offset(pmc_scratch114), 25:23),
	pack(TYPE_SDRAM, sdram_offset(emc_ctt_duration), 2:0, pmc_offset(pmc_scratch114), 28:26),
	pack(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 15:13, pmc_offset(pmc_scratch115), 5:3),
	pack(TYPE_SDRAM, sdram_offset(bootrom_patch_data), 31:0, pmc_offset(pmc_scratch16), 31:0),
	pack(TYPE_SDRAM, sdram_offset(emc_dev_select), 1:0, pmc_offset(pmc_scratch17), 31:30),
	pack(TYPE_SDRAM, sdram_offset(emc_zcal_warmcoldboot_enables), 1:0, pmc_offset(pmc_scratch18), 31:30),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_extra_modereg_write_en), 0:0, pmc_offset(pmc_scratch47), 31:31),
	pack(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll_period_warmboot), 1:0, pmc_offset(pmc_scratch48), 31:30),
	pack(TYPE_SDRAM, sdram_offset(mc_emc_reg_mode), 1:0, pmc_offset(pmc_scratch50), 31:30),
	pack(TYPE_SDRAM, sdram_offset(emc_ca_training_enable), 0:0, pmc_offset(pmc_scratch51), 30:30),
	pack(TYPE_SDRAM, sdram_offset(mc_clken_override_all_warmboot), 0:0, pmc_offset(pmc_scratch51), 31:31),
	pack(TYPE_SDRAM, sdram_offset(emc_clken_override_all_warmboot), 0:0, pmc_offset(pmc_scratch56), 30:30),
	pack(TYPE_SDRAM, sdram_offset(emc_mrs_warmboot_enable), 0:0, pmc_offset(pmc_scratch56), 31:31),
	pack(TYPE_SDRAM, sdram_offset(ahb_arb_xbar_ctrl_mem_init_done), 0:0, pmc_offset(pmc_scratch59), 31:31),
	pack(TYPE_SDRAM, sdram_offset(emc_timing_control_wait), 7:0, pmc_offset(pmc_scratch66), 25:18),
	pack(TYPE_SDRAM, sdram_offset(emc_zcal_warmboot_wait), 7:0, pmc_offset(pmc_scratch67), 25:18),
	pack(TYPE_SDRAM, sdram_offset(emc_auto_cal_wait), 7:0, pmc_offset(pmc_scratch68), 25:18),
	pack(TYPE_SDRAM, sdram_offset(warmboot_wait), 7:0, pmc_offset(pmc_scratch69), 25:18),
	pack(TYPE_SDRAM, sdram_offset(emc_pin_program_wait), 7:0, pmc_offset(pmc_scratch70), 25:18),
	pack(TYPE_SDRAM, sdram_offset(bootrom_patch_control), 15:0, pmc_offset(pmc_scratch107), 15:0),
	pack(TYPE_SDRAM, sdram_offset(swizzle_rank_byte_encode), 15:0, pmc_offset(pmc_scratch107), 31:16),
	pack(TYPE_SDRAM, sdram_offset(emc_extra_refresh_num), 2:0, pmc_offset(pmc_scratch114), 31:29),
	pack(TYPE_SDRAM, sdram_offset(memory_type), 2:0, pmc_offset(pmc_scratch115), 2:0),
};

struct pack_fields pack_list_ddr3[] = {
	pack(TYPE_SDRAM, sdram_offset(emc_mrs), 13:0, pmc_offset(pmc_scratch5), 13:0),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs), 13:0, pmc_offset(pmc_scratch5), 27:14),
	pack(TYPE_SDRAM, sdram_offset(emc_mrs), 21:20, pmc_offset(pmc_scratch5), 29:28),
	pack(TYPE_SDRAM, sdram_offset(emc_mrs), 31:30, pmc_offset(pmc_scratch5), 31:30),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs2), 13:0, pmc_offset(pmc_scratch7), 13:0),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs), 21:20, pmc_offset(pmc_scratch7), 15:14),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs), 31:30, pmc_offset(pmc_scratch7), 17:16),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs2), 21:20, pmc_offset(pmc_scratch7), 19:18),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs3), 13:0, pmc_offset(pmc_scratch8), 13:0),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs2), 31:30, pmc_offset(pmc_scratch8), 15:14),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs3), 21:20, pmc_offset(pmc_scratch8), 17:16),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs3), 31:30, pmc_offset(pmc_scratch8), 19:18),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrs_extra), 13:0, pmc_offset(pmc_scratch9), 13:0),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrs_extra), 31:30, pmc_offset(pmc_scratch9), 15:14),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrs_extra), 21:20, pmc_offset(pmc_scratch9), 17:16),
	pack(TYPE_SDRAM, sdram_offset(emc_zq_cal_ddr3_warmboot), 31:30, pmc_offset(pmc_scratch9), 19:18),
	pack(TYPE_SDRAM, sdram_offset(emc_mrs), 26:26, pmc_offset(pmc_scratch10), 0:0),
	pack(TYPE_SDRAM, sdram_offset(emc_mrs), 27:27, pmc_offset(pmc_scratch10), 1:1),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs), 26:26, pmc_offset(pmc_scratch10), 2:2),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs), 27:27, pmc_offset(pmc_scratch10), 3:3),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs2), 26:26, pmc_offset(pmc_scratch10), 4:4),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs2), 27:27, pmc_offset(pmc_scratch10), 5:5),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs3), 26:26, pmc_offset(pmc_scratch10), 6:6),
	pack(TYPE_SDRAM, sdram_offset(emc_emrs3), 27:27, pmc_offset(pmc_scratch10), 7:7),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrs_extra), 27:27, pmc_offset(pmc_scratch10), 8:8),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrs_extra), 26:26, pmc_offset(pmc_scratch10), 9:9),
	pack(TYPE_SDRAM, sdram_offset(emc_zq_cal_ddr3_warmboot), 0:0, pmc_offset(pmc_scratch10), 10:10),
	pack(TYPE_SDRAM, sdram_offset(emc_zq_cal_ddr3_warmboot), 4:4, pmc_offset(pmc_scratch10), 11:11),
	pack(TYPE_CONST, 0, 31:0, pmc_offset(pmc_scratch116), 31:0),
	pack(TYPE_CONST, 0, 31:0, pmc_offset(pmc_scratch117), 31:0),
};

struct pack_fields pack_list_lpddr2[] = {
	pack(TYPE_SDRAM, sdram_offset(emc_mrw_lpddr2_zcal_warmboot), 23:16, pmc_offset(pmc_scratch5), 7:0),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw_lpddr2_zcal_warmboot), 7:0, pmc_offset(pmc_scratch5), 15:8),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrw_extra), 23:16, pmc_offset(pmc_scratch5), 23:16),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrw_extra), 7:0, pmc_offset(pmc_scratch5), 31:24),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw_lpddr2_zcal_warmboot), 31:30, pmc_offset(pmc_scratch6), 1:0),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrw_extra), 31:30, pmc_offset(pmc_scratch6), 3:2),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw_lpddr2_zcal_warmboot), 26:26, pmc_offset(pmc_scratch6), 4:4),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw_lpddr2_zcal_warmboot), 27:27, pmc_offset(pmc_scratch6), 5:5),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrw_extra), 26:26, pmc_offset(pmc_scratch6), 6:6),
	pack(TYPE_SDRAM, sdram_offset(emc_warmboot_mrw_extra), 27:27, pmc_offset(pmc_scratch6), 7:7),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw1), 7:0, pmc_offset(pmc_scratch7), 7:0),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw1), 23:16, pmc_offset(pmc_scratch7), 15:8),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw1), 26:26, pmc_offset(pmc_scratch7), 16:16),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw1), 27:27, pmc_offset(pmc_scratch7), 17:17),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw1), 31:30, pmc_offset(pmc_scratch7), 19:18),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw2), 7:0, pmc_offset(pmc_scratch8), 7:0),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw2), 23:16, pmc_offset(pmc_scratch8), 15:8),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw2), 26:26, pmc_offset(pmc_scratch8), 16:16),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw2), 27:27, pmc_offset(pmc_scratch8), 17:17),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw2), 31:30, pmc_offset(pmc_scratch8), 19:18),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw3), 7:0, pmc_offset(pmc_scratch9), 7:0),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw3), 23:16, pmc_offset(pmc_scratch9), 15:8),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw3), 26:26, pmc_offset(pmc_scratch9), 16:16),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw3), 27:27, pmc_offset(pmc_scratch9), 17:17),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw3), 31:30, pmc_offset(pmc_scratch9), 19:18),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw4), 7:0, pmc_offset(pmc_scratch10), 7:0),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw4), 23:16, pmc_offset(pmc_scratch10), 15:8),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw4), 26:26, pmc_offset(pmc_scratch10), 16:16),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw4), 27:27, pmc_offset(pmc_scratch10), 17:17),
	pack(TYPE_SDRAM, sdram_offset(emc_mrw4), 31:30, pmc_offset(pmc_scratch10), 19:18),
};

struct pack_fields pack_list_2[] = {
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1a), 3:0, pmc_offset(pmc_secure_scratch8), 3:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1a), 7:4, pmc_offset(pmc_secure_scratch8), 7:4),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1a), 11:8, pmc_offset(pmc_secure_scratch8), 11:8),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1a), 19:16, pmc_offset(pmc_secure_scratch8), 15:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1a), 23:20, pmc_offset(pmc_secure_scratch8), 19:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1a), 27:24, pmc_offset(pmc_secure_scratch8), 23:20),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1a), 31:28, pmc_offset(pmc_secure_scratch8), 27:24),
	pack(TYPE_SDRAM, sdram_offset(emc_adr_cfg), 0:0, pmc_offset(pmc_secure_scratch8), 28:28),
	pack(TYPE_SDRAM, sdram_offset(emc_adr_cfg), 7:7, pmc_offset(pmc_secure_scratch8), 29:29),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg), 0:0, pmc_offset(pmc_secure_scratch8), 30:30),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_write_access), 0:0, pmc_offset(pmc_secure_scratch8), 31:31),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1a), 3:0, pmc_offset(pmc_secure_scratch9), 3:0),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1a), 7:4, pmc_offset(pmc_secure_scratch9), 7:4),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1a), 11:8, pmc_offset(pmc_secure_scratch9), 11:8),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1a), 19:16, pmc_offset(pmc_secure_scratch9), 15:12),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1a), 23:20, pmc_offset(pmc_secure_scratch9), 19:16),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1a), 27:24, pmc_offset(pmc_secure_scratch9), 23:20),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1a), 31:28, pmc_offset(pmc_secure_scratch9), 27:24),
	pack(TYPE_SDRAM, sdram_offset(mc_sec_carveout_protect_write_access), 0:0, pmc_offset(pmc_secure_scratch9), 28:28),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2a), 3:0, pmc_offset(pmc_secure_scratch10), 3:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2a), 7:4, pmc_offset(pmc_secure_scratch10), 7:4),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2a), 11:8, pmc_offset(pmc_secure_scratch10), 11:8),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2a), 15:12, pmc_offset(pmc_secure_scratch10), 15:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2a), 19:16, pmc_offset(pmc_secure_scratch10), 19:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2a), 23:20, pmc_offset(pmc_secure_scratch10), 23:20),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2a), 27:24, pmc_offset(pmc_secure_scratch10), 27:24),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2a), 3:0, pmc_offset(pmc_secure_scratch11), 3:0),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2a), 7:4, pmc_offset(pmc_secure_scratch11), 7:4),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2a), 11:8, pmc_offset(pmc_secure_scratch11), 11:8),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2a), 15:12, pmc_offset(pmc_secure_scratch11), 15:12),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2a), 19:16, pmc_offset(pmc_secure_scratch11), 19:16),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2a), 23:20, pmc_offset(pmc_secure_scratch11), 23:20),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2a), 27:24, pmc_offset(pmc_secure_scratch11), 27:24),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_bank_mask0), 0:0, pmc_offset(pmc_secure_scratch12), 0:0),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_bank_mask0), 1:1, pmc_offset(pmc_secure_scratch12), 1:1),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_bank_mask0), 31:10, pmc_offset(pmc_secure_scratch12), 23:2),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_channel_mask), 11:5, pmc_offset(pmc_secure_scratch12), 30:24),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_bank_mask1), 31:10, pmc_offset(pmc_secure_scratch13), 21:0),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev0), 2:0, pmc_offset(pmc_secure_scratch13), 24:22),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev0), 9:8, pmc_offset(pmc_secure_scratch13), 26:25),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev0), 19:16, pmc_offset(pmc_secure_scratch13), 30:27),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_bank_mask2), 31:10, pmc_offset(pmc_secure_scratch14), 21:0),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev1), 2:0, pmc_offset(pmc_secure_scratch14), 24:22),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev1), 9:8, pmc_offset(pmc_secure_scratch14), 26:25),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev1), 19:16, pmc_offset(pmc_secure_scratch14), 30:27),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1b), 3:0, pmc_offset(pmc_secure_scratch15), 3:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1b), 7:4, pmc_offset(pmc_secure_scratch15), 7:4),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1b), 11:8, pmc_offset(pmc_secure_scratch15), 11:8),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1b), 15:12, pmc_offset(pmc_secure_scratch15), 15:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack1b), 19:16, pmc_offset(pmc_secure_scratch15), 19:16),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2b), 3:0, pmc_offset(pmc_secure_scratch15), 23:20),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2b), 7:4, pmc_offset(pmc_secure_scratch15), 27:24),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack2b), 11:8, pmc_offset(pmc_secure_scratch15), 31:28),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1b), 3:0, pmc_offset(pmc_secure_scratch16), 3:0),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1b), 7:4, pmc_offset(pmc_secure_scratch16), 7:4),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1b), 11:8, pmc_offset(pmc_secure_scratch16), 11:8),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1b), 15:12, pmc_offset(pmc_secure_scratch16), 15:12),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack1b), 19:16, pmc_offset(pmc_secure_scratch16), 19:16),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2b), 3:0, pmc_offset(pmc_secure_scratch16), 23:20),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2b), 7:4, pmc_offset(pmc_secure_scratch16), 27:24),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack2b), 11:8, pmc_offset(pmc_secure_scratch16), 31:28),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 1:1, pmc_offset(pmc_secure_scratch17), 0:0),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 2:2, pmc_offset(pmc_secure_scratch17), 1:1),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 3:3, pmc_offset(pmc_secure_scratch17), 2:2),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 4:4, pmc_offset(pmc_secure_scratch17), 3:3),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 5:5, pmc_offset(pmc_secure_scratch17), 4:4),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 6:6, pmc_offset(pmc_secure_scratch17), 5:5),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 7:7, pmc_offset(pmc_secure_scratch17), 6:6),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 8:8, pmc_offset(pmc_secure_scratch17), 7:7),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 9:9, pmc_offset(pmc_secure_scratch17), 8:8),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 10:10, pmc_offset(pmc_secure_scratch17), 9:9),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 11:11, pmc_offset(pmc_secure_scratch17), 10:10),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 12:12, pmc_offset(pmc_secure_scratch17), 11:11),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 14:14, pmc_offset(pmc_secure_scratch17), 12:12),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 16:16, pmc_offset(pmc_secure_scratch17), 13:13),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 17:17, pmc_offset(pmc_secure_scratch17), 14:14),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 19:19, pmc_offset(pmc_secure_scratch17), 15:15),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 20:20, pmc_offset(pmc_secure_scratch17), 16:16),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 21:21, pmc_offset(pmc_secure_scratch17), 17:17),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 22:22, pmc_offset(pmc_secure_scratch17), 18:18),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_vpr_override), 23:23, pmc_offset(pmc_secure_scratch17), 19:19),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_bom), 31:20, pmc_offset(pmc_secure_scratch17), 31:20),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack3), 2:0, pmc_offset(pmc_secure_scratch18), 2:0),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack3), 6:4, pmc_offset(pmc_secure_scratch18), 5:3),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack3), 10:8, pmc_offset(pmc_secure_scratch18), 8:6),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack3), 14:12, pmc_offset(pmc_secure_scratch18), 11:9),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack3), 18:16, pmc_offset(pmc_secure_scratch18), 14:12),
	pack(TYPE_SDRAM, sdram_offset(emc_addr_swizzle_stack3), 22:20, pmc_offset(pmc_secure_scratch18), 17:15),
	pack(TYPE_SDRAM, sdram_offset(mc_emem_cfg), 12:0, pmc_offset(pmc_secure_scratch18), 30:18),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack3), 2:0, pmc_offset(pmc_secure_scratch19), 2:0),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack3), 6:4, pmc_offset(pmc_secure_scratch19), 5:3),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack3), 10:8, pmc_offset(pmc_secure_scratch19), 8:6),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack3), 14:12, pmc_offset(pmc_secure_scratch19), 11:9),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack3), 18:16, pmc_offset(pmc_secure_scratch19), 14:12),
	pack(TYPE_SDRAM, sdram_offset(ch1_emc_addr_swizzle_stack3), 22:20, pmc_offset(pmc_secure_scratch19), 17:15),
	pack(TYPE_SDRAM, sdram_offset(mc_video_protect_size_mb), 11:0, pmc_offset(pmc_secure_scratch19), 29:18),
	pack(TYPE_SDRAM, sdram_offset(mc_sec_carveout_bom), 31:20, pmc_offset(pmc_secure_scratch20), 11:0),
	pack(TYPE_SDRAM, sdram_offset(mc_sec_carveout_size_mb), 11:0, pmc_offset(pmc_secure_scratch20), 23:12),

	pack(TYPE_CONST, 0x1555555, 25:0, pmc_offset(pmc_sec_disable2), 25:0),
	pack(TYPE_CONST, 0xff, 7:0, pmc_offset(pmc_sec_disable), 19:12),

	pack(TYPE_CONST, 0, 31:0, pmc_offset(pmc_scratch2), 31:0),
	pack(TYPE_PLLM, pllm_offset(pllm_base), 7:0, pmc_offset(pmc_scratch2), 7:0),
	pack(TYPE_PLLM, pllm_offset(pllm_base), 15:8, pmc_offset(pmc_scratch2), 15:8),
	pack(TYPE_PLLM, pllm_offset(pllm_base), 20:20, pmc_offset(pmc_scratch2), 16:16),
	pack(TYPE_PLLM, pllm_offset(pllm_misc2), 0:0, pmc_offset(pmc_scratch2), 17:17),
	pack(TYPE_PLLM, pllm_offset(pllm_misc2), 2:1, pmc_offset(pmc_scratch2), 19:18),

	pack(TYPE_CONST, 0, 31:0, pmc_offset(pmc_scratch35), 31:0),
	pack(TYPE_PLLM, pllm_offset(pllm_misc1), 23:0, pmc_offset(pmc_scratch35), 23:0),
	pack(TYPE_PLLM, pllm_offset(pllm_misc1), 28:28, pmc_offset(pmc_scratch35), 28:28),
	pack(TYPE_PLLM, pllm_offset(pllm_misc1), 29:29, pmc_offset(pmc_scratch35), 29:29),
	pack(TYPE_PLLM, pllm_offset(pllm_misc1), 30:30, pmc_offset(pmc_scratch35), 30:30),

	pack(TYPE_CONST, 0, 31:0, pmc_offset(pmc_scratch3), 31:0),
	pack(TYPE_SDRAM, sdram_offset(pllm_input_divider), 7:0, pmc_offset(pmc_scratch3), 7:0),
	pack(TYPE_CONST, 0x3e, 7:0, pmc_offset(pmc_scratch3), 15:8),
	pack(TYPE_CONST, 0, 3:0, pmc_offset(pmc_scratch3), 19:16),
	pack(TYPE_SDRAM, sdram_offset(pllm_kvco), 0:0, pmc_offset(pmc_scratch3), 20:20),
	pack(TYPE_SDRAM, sdram_offset(pllm_kcp), 1:0, pmc_offset(pmc_scratch3), 22:21),

	pack(TYPE_CONST, 0, 31:0, pmc_offset(pmc_scratch36), 31:0),
	pack(TYPE_SDRAM, sdram_offset(pllm_setup_control), 23:0, pmc_offset(pmc_scratch36), 23:0),

	pack(TYPE_CONST, 0, 31:0, pmc_offset(pmc_scratch4), 31:0),
	pack(TYPE_SDRAM, sdram_offset(pllm_stable_time), 9:0, pmc_offset(pmc_scratch4), 9:0),
	pack(TYPE_SDRAM, sdram_offset(pllm_stable_time), 9:0, pmc_offset(pmc_scratch4), 19:10),

	pack(TYPE_SDRAM, sdram_offset(pllm_select_div2), 0:0, pmc_offset(pmc_pllm_wb0_override2), 27:27),
	pack(TYPE_CONST, 1, 0:0, pmc_offset(pmc_pllp_wb0_override), 11:11),
};

void do_encode(struct encode_fields *encode, struct sdram_params *sdram)
{
	u32 val, op1, op2;

	if (encode->encode_type != TYPE_SDRAM) {
		debug("error!!! encode for TYPE_SDRAM only\n");
		return;
	}
	val = readl((char *)sdram + encode->src_offset);
	op1 = val >> encode->op1_shift;
	op1 &= encode->op1_mask;
	op2 = val >> encode->op2_shift;
	op2 &= encode->op2_mask;

	val = readl((char *)sdram + encode->dst_offset);
	val &= ~(1 << encode->dst_shift);
	if (op1 > op2)
		val |= (1 << encode->dst_shift);
	writel(val, (char *)sdram + encode->dst_offset);
}

void do_encode_list(struct encode_fields *encode_list,  u32 num_list,
		       struct sdram_params *sdram)
{
	struct encode_fields *encode;
	u32 i;

	for (i = 0, encode = encode_list; i < num_list; ++i, ++encode)
		do_encode(encode, sdram);
}

void do_pack(struct pack_fields *pack, struct sdram_params *sdram)
{
	u32 val, offset, type, reg;
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;

	offset = pack->src_offset;
	type = pack->src_type;

	switch (type) {
	case TYPE_SDRAM:
		val = readl((char *)sdram + offset);
		break;
	case TYPE_PLLM:
		val = readl((char *)&clkrst->crc_pll[CLOCK_ID_MEMORY]
			+ offset);
		break;
	case TYPE_CONST:
		val = offset;
		break;
	default:
		debug("src_type (%u) is not supported\n", type);
		return;
	}

	val >>= pack->src_shift;
	val &= pack->src_mask;
	val <<= pack->dst_shift;

	reg = readl(NV_PA_PMC_BASE + pack->dst_offset);
	reg &= ~(pack->dst_mask << pack->dst_shift);
	reg |= val;
	writel(reg, NV_PA_PMC_BASE + pack->dst_offset);
}

void do_pack_list(struct pack_fields *pack_list, u32 num_list,
		   struct sdram_params *sdram)
{
	struct pack_fields *pack;
	u32 i;

	for (pack = pack_list, i = 0; i < num_list; ++i, ++pack)
		do_pack(pack, sdram);
}

void warmboot_save_sdram_params(void)
{
	u32 ram_code;
	struct sdram_params sdram;
	struct pmux_tri_ctlr *pmt = (struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;

	/* get ram code that is used as index to array _params in BCT */
	ram_code = bf_readl(STRAP_OPT_A_RAM_CODE, &pmt->pmt_strap_opt_a) & 3;
	memcpy(&sdram,
	       (char *)((struct sdram_params *)SDRAM_PARAMS_BASE + ram_code),
	       sizeof(sdram));

	/* encode BIT6_GT_BIT7 bits in sdram.swizzle_rank_byte_encode */
	do_encode_list(encode_list, ARRAY_SIZE(encode_list), &sdram);

	do_pack_list(pack_list_1, ARRAY_SIZE(pack_list_1), &sdram);

	if (sdram.memory_type == MEMORY_TYPE_LPDDR2)
		do_pack_list(pack_list_lpddr2, ARRAY_SIZE(pack_list_lpddr2),
			     &sdram);
	else if (sdram.memory_type == MEMORY_TYPE_DDR3)
		do_pack_list(pack_list_ddr3, ARRAY_SIZE(pack_list_ddr3),
			     &sdram);

	do_pack_list(pack_list_2, ARRAY_SIZE(pack_list_2), &sdram);
}

/*
 * NOTE: If more than one of the following is enabled, only one of them will
 *	 actually be used. RANDOM takes precedence over PATTERN and ZERO, and
 *	 PATTERN takes precedence overy ZERO.
 *
 *	 RANDOM_AES_BLOCK_IS_PATTERN is to define a 32-bit PATTERN.
 */
#undef RANDOM_AES_BLOCK_IS_RANDOM	/* to randomize the header */
#undef RANDOM_AES_BLOCK_IS_PATTERN	/* to patternize the header */
#define RANDOM_AES_BLOCK_IS_ZERO	/* to clear the header */

static u32 get_major_version(void)
{
	u32 major_id;
	struct apb_misc_gp_ctlr *gp =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;

	major_id = bf_readl(HIDREV_MAJORPREV, &gp->hidrev);
	return major_id;
}

static int is_production_mode_fuse_set(struct fuse_regs *fuse)
{
	return readl(&fuse->production_mode);
}

static int is_odm_production_mode_fuse_set(struct fuse_regs *fuse)
{
	return readl(&fuse->security_mode);
}

static int is_failure_analysis_mode(struct fuse_regs *fuse)
{
	return readl(&fuse->fa);
}

static int is_odm_production_mode(void)
{
	struct fuse_regs *fuse = (struct fuse_regs *)NV_PA_FUSE_BASE;

	if (!is_failure_analysis_mode(fuse) &&
	    is_odm_production_mode_fuse_set(fuse))
		return 1;
	else
		return 0;
}

static int is_production_mode(void)
{
	struct fuse_regs *fuse = (struct fuse_regs *)NV_PA_FUSE_BASE;

	if (get_major_version() == 0)
		return 1;

	if (!is_failure_analysis_mode(fuse) &&
	    is_production_mode_fuse_set(fuse) &&
	    !is_odm_production_mode_fuse_set(fuse))
		return 1;
	else
		return 0;
}

static enum fuse_operating_mode fuse_get_operation_mode(void)
{
	u32 chip_id;
	struct apb_misc_gp_ctlr *gp =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;

	chip_id = bf_readl(HIDREV_CHIPID, &gp->hidrev);
	if (chip_id == CHIPID_TEGRA114) {
		if (is_odm_production_mode()) {
			printf("!! odm_production_mode is not supported !!\n");
			return MODE_UNDEFINED;
		} else
			if (is_production_mode())
				return MODE_PRODUCTION;
			else
				return MODE_PREPRODUCTION;
	}
	return MODE_UNDEFINED;
}

#if defined(RANDOM_AES_BLOCK_IS_RANDOM)
/* Currently, this routine returns a 32-bit all 0 seed. */
static u32 query_random_seed(void)
{
	return 0;
}
#endif

static void determine_crypto_options(int *is_encrypted, int *is_signed,
				     int *use_zero_key)
{
	switch (fuse_get_operation_mode()) {
	case MODE_PREPRODUCTION:
	case MODE_PRODUCTION:
		*is_encrypted = 0;
		*is_signed = 1;
		*use_zero_key = 1;
		break;
	case MODE_UNDEFINED:
	default:
		*is_encrypted = 0;
		*is_signed = 0;
		*use_zero_key  = 0;
		break;
	}
}

static int sign_wb_code(u32 start, u32 length, int use_zero_key)
{
	int err;
	u8 *source;		/* Pointer to source */
	u8 *hash;

	/* Calculate AES block parameters. */
	source = (u8 *)(start + offsetof(struct wb_header, random_aes_block));
	length -= offsetof(struct wb_header, random_aes_block);
	hash = (u8 *)(start + offsetof(struct wb_header, hash_signature.hash));
	err = sign_data_block(source, length, hash);

	return err;
}

int warmboot_prepare_code(u32 seg_address, u32 seg_length)
{
	int err = 0;
	u32 length;			/* length of the signed/encrypt code */
	struct wb_header *dst_header;	/* Pointer to dest WB header */
	int is_encrypted;		/* Segment is encrypted */
	int is_signed;			/* Segment is signed */
	int use_zero_key;		/* Use key of all zeros */

	/* Determine crypto options. */
	determine_crypto_options(&is_encrypted, &is_signed, &use_zero_key);

	/* Get the actual code limits. */
	length = roundup(((u32)wb_end - (u32)wb_start), 16);

	/*
	 * The region specified by seg_address must not be in IRAM and must be
	 * nonzero in length.
	 */
	if ((seg_length == 0) || (seg_address == 0) ||
	    ((seg_address >= NVAP_BASE_PA_SRAM) &&
	     (seg_address < (NVAP_BASE_PA_SRAM + NVAP_BASE_PA_SRAM_SIZE)))) {
		err = -EFAULT;
		goto fail;
	}

	/* Things must be 16-byte aligned. */
	if ((seg_length & 0xF) || (seg_address & 0xF)) {
		err = -EINVAL;
		goto fail;
	}

	/* Will the code fit? (destination includes wb_header + wb code) */
	if (seg_length < (length + sizeof(struct wb_header))) {
		err = -EINVAL;
		goto fail;
	}

	dst_header = (struct wb_header *)seg_address;
	memset((char *)dst_header, 0, sizeof(struct wb_header));

	/* Populate the random_aes_block as requested. */
	{
		u32 *aes_block = (u32 *)&(dst_header->random_aes_block);
		u32 *end = (u32 *)(((u32)aes_block) +
				   sizeof(dst_header->random_aes_block));

		do {
#if defined(RANDOM_AES_BLOCK_IS_RANDOM)
			*aes_block++ = query_random_seed();
#elif defined(RANDOM_AES_BLOCK_IS_PATTERN)
			*aes_block++ = RANDOM_AES_BLOCK_IS_PATTERN;
#elif defined(RANDOM_AES_BLOCK_IS_ZERO)
			*aes_block++ = 0;
#else
			printf("None of RANDOM_AES_BLOCK_IS_XXX is defined; ");
			printf("Default to pattern 0.\n");
			*aes_block++ = 0;
#endif
		} while (aes_block < end);
	}

	/* Populate the header. */
	dst_header->length_in_secure = length + sizeof(struct wb_header);
	dst_header->length_secure = length + sizeof(struct wb_header);
	dst_header->destination = T11X_WB_RUN_ADDRESS;
	dst_header->entry_point = T11X_WB_RUN_ADDRESS;
	dst_header->code_length = length;

	if (is_encrypted) {
		printf("!!!! Encryption is not supported !!!!\n");
		dst_header->length_in_secure = 0;
		err = -EACCES;
		goto fail;
	} else
		/* copy the wb code directly following dst_header. */
		memcpy((char *)(dst_header+1), (char *)wb_start, length);

	if (is_signed)
		err = sign_wb_code(seg_address, dst_header->length_in_secure,
				   use_zero_key);

fail:
	if (err)
		printf("WB code not copied to LP0 location! (error=%d)\n", err);

	return err;
}

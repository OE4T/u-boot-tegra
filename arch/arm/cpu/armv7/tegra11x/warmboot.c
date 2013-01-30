/*
 * (C) Copyright 2010 - 2011
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

/* t11x bringup */
#include <asm/arch/t11x/nvbl_memmap_nvap.h>

#define BCT_OFFSET		0x100		/* BCT starts at 0x100 */
#define BCT_SDRAM_PARAMS_OFFSET	(BCT_OFFSET + 0x148)
#define SDRAM_PARAMS_BASE	(NVAP_BASE_PA_SRAM + BCT_SDRAM_PARAMS_OFFSET)

enum field_type {
	TYPE_SDRAM,
	TYPE_PLLX,
	TYPE_CONST,
};

struct pmc_scratch_field {
	enum field_type field_type;
	u32 offset_val;
	u32 mask;
	u32 shift_src;
	u32 shift_dst;
};

#define pllx_offset(member) offsetof(struct clk_pll_simple, member)
#define sdram_offset(member) offsetof(struct sdram_params, member)

#define field(type, param, field, dst)					\
	{								\
		.field_type = type,					\
		.offset_val = param,					\
		.mask = 0xfffffffful >> (31 - ((1 ? field) - (0 ? field))), \
		.shift_src = 0 ? field,					\
		.shift_dst = 0 ? dst,					\
	}

#define scratch(num, field_list)					\
	{								\
		.scratch_off = offsetof(struct pmc_ctlr, num),		\
		.fields = field_list,					\
		.num_fields = ARRAY_SIZE(field_list),			\
	}

static const struct pmc_scratch_field scratch_3[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_PLLX, pllx_offset(pll_base), 4 : 0, 4 : 0),
	field(TYPE_PLLX, pllx_offset(pll_base), 17 : 8, 14 : 5),
	field(TYPE_PLLX, pllx_offset(pll_base), 22 : 20, 17 : 15),
	field(TYPE_PLLX, pllx_offset(pll_misc), 7 : 4, 21 : 18),
	field(TYPE_PLLX, pllx_offset(pll_base), 11 : 8, 25 : 22),
	field(TYPE_CONST, 0, 0 : 0, 26 : 26),
};

static const struct pmc_scratch_field scratch_4[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_CONST, -1, 9 : 0, 19 : 10),
	field(TYPE_CONST, 1, 0 : 0, 31 : 31),
};

static const struct pmc_scratch_field scratch_5_ddr3[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_emr3), 13 : 0, 13 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_emr2), 13 : 0, 27 : 14),
	field(TYPE_SDRAM, sdram_offset(emc_wb_emr3), 21 : 20, 29 : 28),
	field(TYPE_SDRAM, sdram_offset(emc_wb_emr2), 21 : 20, 31 : 30),
};

static const struct pmc_scratch_field scratch_5_lpddr2[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_mrw_lpddr2_zcal_wb), 23 : 16, 7 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_mrw_lpddr2_zcal_wb), 7 : 0, 15 : 8),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrw1), 23 : 16, 23 : 16),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrw1), 7 : 0, 31 : 24),
};

static const struct pmc_scratch_field scratch_6_ddr3[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrs_extra), 13 : 0, 13 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrs), 13 : 0, 27 : 14),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrs_extra), 21 : 20, 29 : 28),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrs), 21 : 20, 31 : 30),
};
static const struct pmc_scratch_field scratch_6_lpddr2[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrw3), 23 : 16, 7 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrw3), 7 : 0, 15 : 8),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrw2), 23 : 16, 23 : 16),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrw2), 7 : 0, 31 : 24),
};

static const struct pmc_scratch_field scratch_7_ddr3[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_emrs), 13 : 0, 13 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_emrs), 21 : 20, 15 : 14),
	field(TYPE_SDRAM, sdram_offset(emc_wb_emr3), 26 : 26, 16 : 16),
	field(TYPE_SDRAM, sdram_offset(emc_wb_emr2), 26 : 26, 17 : 17),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrs_extra), 26 : 26, 18 : 18),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrs), 26 : 26, 19 : 19),
	field(TYPE_SDRAM, sdram_offset(emc_wb_emrs), 26 : 26, 20 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_zq_cal_ddr3_wb), 0 : 0, 21 : 21),
	field(TYPE_SDRAM, sdram_offset(emc_zq_cal_ddr3_wb), 4 : 4, 22 : 22),
	field(TYPE_SDRAM, sdram_offset(emc_rfc), 8 : 0, 31 : 23),
};
static const struct pmc_scratch_field scratch_7_lpddr2[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrw_extra), 23 : 16, 7 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wb_mrw_extra), 7 : 0, 15 : 8),
};

static const struct pmc_scratch_field scratch_8[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 0 : 0, 0 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 2 : 2, 1 : 1),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 4 : 4, 2 : 2),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 5 : 5, 3 : 3),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 7 : 6, 5 : 4),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 11 : 8, 9 : 6),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 25 : 16, 19 : 10),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 27 : 27, 20 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll), 29 : 28, 22 : 21),
	field(TYPE_SDRAM, sdram_offset(emc_xm2cmd_pad_ctrl), 5 : 5, 23 : 23),
	field(TYPE_SDRAM, sdram_offset(emc_xm2cmd_pad_ctrl), 10 : 10, 24 : 24),
	field(TYPE_SDRAM, sdram_offset(emc_xm2cmd_pad_ctrl), 22 : 16, 31 : 25),
};

static const struct pmc_scratch_field scratch_9[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs0), 4 : 0, 4 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs0), 22 : 8, 19 : 5),
	field(TYPE_SDRAM, sdram_offset(emc_xm2clk_pad_ctrl), 0 : 0, 20 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_xm2clk_pad_ctrl), 2 : 2, 21 : 21),
	field(TYPE_SDRAM, sdram_offset(emc_xm2clk_pad_ctrl), 18 : 14, 26 : 22),
	field(TYPE_SDRAM, sdram_offset(emc_xm2clk_pad_ctrl), 23 : 19, 31 : 27),
};

static const struct pmc_scratch_field scratch_10[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse0), 4 : 0, 4 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse0), 22 : 8, 19 : 5),
	field(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 4 : 0, 24 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 12 : 8, 29 : 25),
	field(TYPE_SDRAM, sdram_offset(emc_auto_cal_config), 30 : 30, 30 : 30),
	field(TYPE_SDRAM, sdram_offset(emc_cfg), 21 : 21, 31 : 31),
};

static const struct pmc_scratch_field scratch_11[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq0), 4 : 0, 4 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq0), 22 : 8, 19 : 5),
	field(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 5 : 0, 25 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 9 : 8, 27 : 26),
	field(TYPE_SDRAM, sdram_offset(emc_sel_dpd_ctrl), 18 : 16, 30 : 28),
	field(TYPE_SDRAM, sdram_offset(emc_adr_cfg), 7 : 7, 31 : 31)
};

static const struct pmc_scratch_field scratch_12[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_cfg2), 14 : 7, 7 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_cfg2), 19 : 18, 9 : 8),
	field(TYPE_SDRAM, sdram_offset(emc_cfg2), 22 : 22, 10 : 10),
	field(TYPE_SDRAM, sdram_offset(emc_cfg2), 30 : 24, 17 : 11),
	field(TYPE_SDRAM, sdram_offset(emc_trefbw), 13 : 0, 31 : 18),
};

static const struct pmc_scratch_field scratch_13[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_zcal_mrw_cmd), 7 : 0, 7 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_zcal_mrw_cmd), 23 : 16, 15 : 8),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs1), 22 : 8, 30 : 16),
	field(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg), 0 : 0, 31 : 31),
};

static const struct pmc_scratch_field scratch_14[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs2), 22 : 8, 14 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs3), 22 : 8, 29 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_dev_select), 1 : 0, 31 : 30),
};

static const struct pmc_scratch_field scratch_15[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs4), 22 : 8, 14 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs5), 22 : 8, 29 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_dig_dll_period_wb),
					1 : 0, 31 : 30),
};

static const struct pmc_scratch_field scratch_16[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs6), 22 : 8, 14 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dqs7), 22 : 8, 29 : 15),
	field(TYPE_SDRAM, sdram_offset(mc_emem_arb_misc0), 27 : 27, 30 : 30),
	field(TYPE_SDRAM, sdram_offset(emc_wb_ext_mode_reg_enable),
					0 : 0, 31 : 31),
};

static const struct pmc_scratch_field scratch_17[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse1), 22 : 8, 14 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse2), 22 : 8, 29 : 15),
	field(TYPE_SDRAM, sdram_offset(mc_clken_override_all_wb),
					0 : 0, 30 : 30),
	field(TYPE_SDRAM, sdram_offset(emc_clken_override_all_wb),
					0 : 0, 31 : 31),
};

static const struct pmc_scratch_field scratch_18[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse3), 22 : 8, 14 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse4), 22 : 8, 29 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_mrs_wb_enable), 0 : 0, 30 : 30),
	field(TYPE_SDRAM, sdram_offset(emc_zcal_wb_enable), 0 : 0, 31 : 31),
};

static const struct pmc_scratch_field scratch_19[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse5), 22 : 8, 14 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse6), 22 : 8, 29 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 1 : 0, 31 : 30),
};

static const struct pmc_scratch_field scratch_22[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_quse7), 22 : 8, 14 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq1), 22 : 8, 29 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 3 : 2, 31 : 30),
};

static const struct pmc_scratch_field scratch_23[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq2), 22 : 8, 14 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dll_xform_dq3), 22 : 8, 29 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_clock_source), 1 : 0, 31 : 30),
};

static const struct pmc_scratch_field scratch_24[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_zcal_interval), 23 : 10, 13 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 0 : 0, 14 : 14),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 2 : 2, 15 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 3 : 3, 16 : 16),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 4 : 4, 17 : 17),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 5 : 5, 18 : 18),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 9 : 9, 19 : 19),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 10 : 10, 20 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 11 : 11, 21 : 21),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 12 : 12, 22 : 22),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 13 : 13, 23 : 23),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl2), 27 : 24, 27 : 24),
	field(TYPE_SDRAM, sdram_offset(emc_rrd), 3 : 0, 31 : 28),
};

static const struct pmc_scratch_field scratch_25[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2vttgen_pad_ctrl), 0 : 0, 0 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2vttgen_pad_ctrl), 10 : 8, 3 : 1),
	field(TYPE_SDRAM, sdram_offset(emc_xm2vttgen_pad_ctrl), 14 : 12, 6 : 4),
	field(TYPE_SDRAM, sdram_offset(emc_xm2vttgen_pad_ctrl), 18 : 16, 9 : 7),
	field(TYPE_SDRAM, sdram_offset(emc_xm2vttgen_pad_ctrl),
					26 : 24, 12 : 10),
	field(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 27 : 27, 13 : 13),
	field(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 26 : 26, 14 : 14),
	field(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 24 : 24, 15 : 15),
	field(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 16 : 16, 16 : 16),
	field(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 10 : 10, 17 : 17),
	field(TYPE_SDRAM, sdram_offset(mc_emem_arb_override), 7 : 0, 25 : 18),
	field(TYPE_SDRAM, sdram_offset(emc_ras), 5 : 0, 31 : 26),
};

static const struct pmc_scratch_field scratch_26[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_refresh), 15 : 6, 9 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_zcal_wait_cnt), 9 : 0, 19 : 10),
	field(TYPE_SDRAM, sdram_offset(emc_xm2vttgen_pad_ctrl2),
					2 : 0, 22 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_xm2vttgen_pad_ctrl2),
					27 : 26, 24 : 23),
	field(TYPE_SDRAM, sdram_offset(emc_xm2vttgen_pad_ctrl2),
					29 : 28, 26 : 25),
	field(TYPE_SDRAM, sdram_offset(emc_xm2vttgen_pad_ctrl2),
					31 : 30, 28 : 27),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_cfg6), 2 : 0, 31 : 29),
};

static const struct pmc_scratch_field scratch_27[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2quse_pad_ctrl), 0 : 0, 0 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2quse_pad_ctrl), 2 : 2, 1 : 1),
	field(TYPE_SDRAM, sdram_offset(emc_xm2quse_pad_ctrl), 3 : 3, 2 : 2),
	field(TYPE_SDRAM, sdram_offset(emc_xm2quse_pad_ctrl), 4 : 4, 3 : 3),
	field(TYPE_SDRAM, sdram_offset(emc_xm2quse_pad_ctrl), 5 : 5, 4 : 4),
	field(TYPE_SDRAM, sdram_offset(emc_xm2quse_pad_ctrl), 27 : 24, 8 : 5),
	field(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev0), 2 : 0, 11 : 9),
	field(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev0), 9 : 8, 13 : 12),
	field(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev0), 19 : 16, 17 : 14),
	field(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev1), 2 : 0, 20 : 18),
	field(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev1), 9 : 8, 22 : 21),
	field(TYPE_SDRAM, sdram_offset(mc_emem_adr_cfg_dev1), 19 : 16, 26 : 23),
	field(TYPE_SDRAM, sdram_offset(emc_r2w), 4 : 0, 31 : 27),
};

static const struct pmc_scratch_field scratch_28[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_spare), 31 : 24, 7 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_rsv), 7 : 0, 15 : 8),
	field(TYPE_SDRAM, sdram_offset(emc_xm2comp_pad_ctrl), 3 : 0, 19 : 16),
	field(TYPE_SDRAM, sdram_offset(emc_xm2comp_pad_ctrl), 7 : 5, 22 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_xm2comp_pad_ctrl), 10 : 10, 23 : 23),
	field(TYPE_SDRAM, sdram_offset(mc_emem_arb_rsv), 7 : 0, 31 : 24),
};

static const struct pmc_scratch_field scratch_29[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_timing_control_wait), 7 : 0, 7 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_zcal_wb_wait), 7 : 0, 15 : 8),
	field(TYPE_SDRAM, sdram_offset(emc_auto_cal_wait), 7 : 0, 23 : 16),
	field(TYPE_SDRAM, sdram_offset(wb_wait), 7 : 0, 31 : 24),
};

static const struct pmc_scratch_field scratch_30[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_pin_program_wait), 7 : 0, 7 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_rc), 6 : 0, 14 : 8),
	field(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs0), 6 : 0, 21 : 5),
	field(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs1), 6 : 0, 28 : 22),
	field(TYPE_SDRAM, sdram_offset(emc_extra_refresh_num) , 2 : 0, 31 : 29),
};

static const struct pmc_scratch_field scratch_31[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs2), 6 : 0, 6 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs3), 6 : 0, 13 : 7),
	field(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs4), 6 : 0, 20 : 14),
	field(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs5), 6 : 0, 27 : 21),
	field(TYPE_SDRAM, sdram_offset(emc_rext), 3 : 0, 31 : 28),
};

static const struct pmc_scratch_field scratch_32[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs6), 6 : 0, 6 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_dli_trim_txdqs7), 6 : 0, 13 : 7),
	field(TYPE_SDRAM, sdram_offset(emc_rp), 5 : 0, 19 : 14),
	field(TYPE_SDRAM, sdram_offset(emc_w2p), 5 : 0, 25 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_rd_rcd), 5 : 0, 31 : 26),
};

static const struct pmc_scratch_field scratch_33[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wr_rcd), 5 : 0, 5 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_tfaw), 5 : 0, 11 : 6),
	field(TYPE_SDRAM, sdram_offset(emc_trpab), 5 : 0, 17 : 12),
	field(TYPE_SDRAM, sdram_offset(emc_odt_write), 2 : 0, 20 : 18),
	field(TYPE_SDRAM, sdram_offset(emc_odt_write), 4 : 4, 21 : 21),
	field(TYPE_SDRAM, sdram_offset(emc_odt_write), 30 : 30, 22 : 22),
	field(TYPE_SDRAM, sdram_offset(emc_odt_write), 31 : 31, 23 : 23),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl3), 0 : 0, 24 : 24),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl3), 5 : 5, 25 : 25),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl3), 27 : 24, 29 : 26),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 7 : 7, 31 : 31),
	field(TYPE_SDRAM, sdram_offset(ahb_arbitration_xbar_ctrl_mem_init_done),
					0 : 0, 30 : 30),
};

static const struct pmc_scratch_field scratch_40[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_w2r), 4 : 0, 4 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_r2p), 4 : 0, 9 : 5),
	field(TYPE_SDRAM, sdram_offset(emc_quse), 4 : 0, 14 : 10),
	field(TYPE_SDRAM, sdram_offset(emc_qrst), 4 : 0, 19 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_rdv), 4 : 0, 24 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_ctt), 4 : 0, 29 : 25),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 8 : 8, 30 : 30),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 9 : 9, 31 : 31),
};

static const struct pmc_scratch_field scratch_42[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_wdv), 3 : 0, 3 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_qsafe), 3 : 0, 7 : 4),
	field(TYPE_SDRAM, sdram_offset(emc_wext), 3 : 0, 11 : 8),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 15 : 13, 25 : 23),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_cfg5), 12 : 12, 26 : 26),
	field(TYPE_SDRAM, sdram_offset(emc_quse_extra), 3 : 0, 31 : 28),
	field(TYPE_SDRAM, sdram_offset(memory_type), 2 : 0, 14 : 12),
	field(TYPE_SDRAM, sdram_offset(emc_clock_divider), 7 : 0, 22 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_clock_use_pllmud), 0 : 0, 27 : 27),
};

static const struct pmc_scratch_field scratch_44[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2cmd_pad_ctrl), 6 : 6, 0 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2cmd_pad_ctrl), 8 : 8, 1 : 1),
	field(TYPE_SDRAM, sdram_offset(emc_ar2pden), 8 : 0, 10 : 2),
	field(TYPE_SDRAM, sdram_offset(emc_fbio_spare), 23 : 16, 18 : 11),
	field(TYPE_SDRAM, sdram_offset(emc_cfg_rsv), 15 : 8, 26 : 19),
	field(TYPE_SDRAM, sdram_offset(emc_ctt_term_ctrl), 12 : 8, 31 : 27),
};

static const struct pmc_scratch_field scratch_45[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_pchg2pden), 5 : 0, 5 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_act2pden), 5 : 0, 11 : 6),
	field(TYPE_SDRAM, sdram_offset(emc_rw2pden), 5 : 0, 17 : 12),
	field(TYPE_SDRAM, sdram_offset(emc_xm2cmd_pad_ctrl2), 18 : 14, 22 : 18),
	field(TYPE_SDRAM, sdram_offset(emc_xm2cmd_pad_ctrl2), 23 : 19, 27 : 23),
	field(TYPE_SDRAM, sdram_offset(emc_xm2cmd_pad_ctrl2), 27 : 24, 31 : 28),
};

static const struct pmc_scratch_field scratch_46[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl), 4 : 0, 4 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl), 12 : 8, 9 : 5),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl), 18 : 14, 14 : 10),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl), 23 : 19, 19 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dq_pad_ctrl), 18 : 14, 24 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dq_pad_ctrl), 23 : 19, 29 : 25),
	field(TYPE_SDRAM, sdram_offset(emc_cfg2), 17 : 16, 31 : 30),
};

static const struct pmc_scratch_field scratch_47[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2cmd_pad_ctrl2), 31 : 28, 3 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl), 27 : 24, 7 : 4),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dqs_pad_ctrl), 31 : 28, 11 : 8),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dq_pad_ctrl), 27 : 24, 15 : 12),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dq_pad_ctrl), 31 : 28, 19 : 16),
	field(TYPE_SDRAM, sdram_offset(emc_xm2clk_pad_ctrl), 27 : 24, 23 : 20),
	field(TYPE_SDRAM, sdram_offset(emc_xm2clk_pad_ctrl), 31 : 28, 27 : 24),
	field(TYPE_SDRAM, sdram_offset(emc_ctt_term_ctrl), 2 : 0, 30 : 28),
	field(TYPE_SDRAM, sdram_offset(emc_cfg), 22 : 22, 31 : 31),
};

static const struct pmc_scratch_field scratch_48[] = {
	field(TYPE_CONST, 0, 31 : 0, 31 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dq_pad_ctrl2), 18 : 16, 2 : 0),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dq_pad_ctrl2), 22 : 20, 5 : 3),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dq_pad_ctrl2), 26 : 24, 8 : 6),
	field(TYPE_SDRAM, sdram_offset(emc_xm2dq_pad_ctrl2), 30 : 28, 11 : 9),
	field(TYPE_SDRAM, sdram_offset(emc_xm2clk_pad_ctrl), 13 : 11, 14 : 12),
	field(TYPE_SDRAM, sdram_offset(emc_cfg2), 21 : 20, 16 : 15),
	field(TYPE_SDRAM, sdram_offset(emc_cfg), 23 : 23, 17 : 17),
};

struct pmc_scratch_reg {
	const struct pmc_scratch_field *fields;
	u32 scratch_off;
	u32 num_fields;
};

static const struct pmc_scratch_reg scratch[] = {
	scratch(pmc_scratch3, scratch_3),
	scratch(pmc_scratch4, scratch_4),
	scratch(pmc_scratch5, scratch_5_ddr3),
	scratch(pmc_scratch6, scratch_6_ddr3),
	scratch(pmc_scratch7, scratch_7_ddr3),
	scratch(pmc_scratch8, scratch_8),
	scratch(pmc_scratch9, scratch_9),
	scratch(pmc_scratch10, scratch_10),
	scratch(pmc_scratch11, scratch_11),
	scratch(pmc_scratch12, scratch_12),
	scratch(pmc_scratch13, scratch_13),
	scratch(pmc_scratch14, scratch_14),
	scratch(pmc_scratch15, scratch_15),
	scratch(pmc_scratch16, scratch_16),
	scratch(pmc_scratch17, scratch_17),
	scratch(pmc_scratch18, scratch_18),
	scratch(pmc_scratch19, scratch_19),
	scratch(pmc_scratch22, scratch_22),
	scratch(pmc_scratch23, scratch_23),
	scratch(pmc_scratch24, scratch_24),
	scratch(pmc_scratch25, scratch_25),
	scratch(pmc_scratch26, scratch_26),
	scratch(pmc_scratch27, scratch_27),
	scratch(pmc_scratch28, scratch_28),
	scratch(pmc_scratch29, scratch_29),
	scratch(pmc_scratch30, scratch_30),
	scratch(pmc_scratch31, scratch_31),
	scratch(pmc_scratch32, scratch_32),
	scratch(pmc_scratch33, scratch_33),
	scratch(pmc_scratch40, scratch_40),
	scratch(pmc_scratch42, scratch_42),
	scratch(pmc_scratch44, scratch_44),
	scratch(pmc_scratch45, scratch_45),
	scratch(pmc_scratch46, scratch_46),
	scratch(pmc_scratch47, scratch_47),
	scratch(pmc_scratch48, scratch_48),
};

static const struct pmc_scratch_reg scratch_lpddr2[] = {
	scratch(pmc_scratch5, scratch_5_lpddr2),
	scratch(pmc_scratch6, scratch_6_lpddr2),
	scratch(pmc_scratch7, scratch_7_lpddr2),
};

void set_scratches(struct pmc_scratch_reg scratches, struct sdram_params *sdram)
{
	u32 j, scratch_off;
	u32 reg = 0;
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;

	for (j = 0; j < scratches.num_fields; j++) {
		u32 val, offset, type;

		offset = scratches.fields[j].offset_val;
		type = scratches.fields[j].field_type;

		switch (type) {
		case TYPE_SDRAM:
			val = readl((char *)sdram + offset);
			break;
		case TYPE_PLLX:
			val =
			readl((char *)&clkrst->crc_pll_simple[CLOCK_ID_XCPU]
				+ offset);
			break;
		case TYPE_CONST:
			val = offset;
			break;
		default:
			continue;
		}
		val >>= scratches.fields[j].shift_src;
		val &= scratches.fields[j].mask;
		val <<= scratches.fields[j].shift_dst;
		reg |= val;
	}

	scratch_off = scratches.scratch_off;
	writel(reg, NV_PA_PMC_BASE + scratch_off);
}

void warmboot_save_sdram_params(void)
{
	u32 ram_code;
	struct sdram_params sdram;
	struct pmux_tri_ctlr *pmt = (struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	struct pmc_ctlr *pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;

	u32 reg, i;

	/* get ram code that is used as index to array _params in BCT */
	ram_code = bf_readl(STRAP_OPT_A_RAM_CODE, &pmt->pmt_strap_opt_a) & 3;

	memcpy(&sdram,
	       (char *)((struct sdram_params *)SDRAM_PARAMS_BASE + ram_code),
	       sizeof(sdram));

	for (i = 0; i < ARRAY_SIZE(scratch); i++)
		set_scratches(scratch[i], &sdram);

	if (sdram.memory_type == MEMORY_TYPE_LPDDR2) {
		for (i = 0; i < ARRAY_SIZE(scratch_lpddr2); i++)
			set_scratches(scratch_lpddr2[i], &sdram);
	}

	writel(0xffffffff, &pmc->pmc_scratch2);

	reg = readl(&pmc->pmc_pllp_wb0_override);
	reg |= PLLM_OVERRIDE_ENABLE;
	writel(reg, &pmc->pmc_pllp_wb0_override);
}

/*
 * NOTE: If more than one of the following is enabled, only one of them will
 *	 actually be used. RANDOM takes precedence over PATTERN and ZERO, and
 *	 PATTERN takes precedence overy ZERO.
 *
 *	 RANDOM_AES_BLOCK_IS_PATTERN is to define a 32-bit PATTERN.
 */
#define RANDOM_AES_BLOCK_IS_RANDOM	/* to randomize the header */
#undef RANDOM_AES_BLOCK_IS_PATTERN	/* to patternize the header */
#undef RANDOM_AES_BLOCK_IS_ZERO		/* to clear the header */

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
				return MODE_UNDEFINED;
	}
	return MODE_UNDEFINED;
}

/* Currently, this routine returns a 32-bit all 0 seed. */
static u32 query_random_seed(void)
{
	return 0;
}

static void determine_crypto_options(int *is_encrypted, int *is_signed,
				     int *use_zero_key)
{
	switch (fuse_get_operation_mode()) {
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
	hash = (u8 *)(start + offsetof(struct wb_header, hash));
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
	dst_header->destination = T30_WB_RUN_ADDRESS;
	dst_header->entry_point = T30_WB_RUN_ADDRESS;
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

/*
 *  (C) Copyright 2010,2011,2012
 *  NVIDIA Corporation <www.nvidia.com>
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

#ifndef _CLK_RST_H_
#define _CLK_RST_H_


/* PLL registers - there are several PLLs in the clock controller */
struct clk_pll {
	uint pll_base;	/* the control register */
	uint pll_out;		/* output control */
	uint pll_out_b;	/* some clock has output B control */
	uint pll_misc;	/* other misc things */
};

/* PLL registers - there are several PLLs in the clock controller */
struct clk_pll_simple {
	uint pll_base;		/* the control register */
	uint pll_misc;		/* other misc things */
};

/* RST_DEV_(L,H,U,V,W)_(SET,CLR) and CLK_ENB_(L,H,U,V,W)_(SET,CLR) */
struct clk_set_clr {
	uint set;
	uint clr;
};

/*
 * Most PLLs use the clk_pll structure, but some have a simpler two-member
 * structure for which we use clk_pll_simple. The reason for this non-
 * othogonal setup is not stated.
 */
enum {
	TEGRA_CLK_PLLS		= 6,	/* Number of normal PLLs */
	TEGRA_CLK_SIMPLE_PLLS	= 3,	/* Number of simple PLLs */
	TEGRA_CLK_REGS		= 3,	/* Number of clock enable regs L/H/U */
	TEGRA_CLK_SOURCES	= 64,	/* Number of ppl clock sources L/H/U */
	TEGRA_CLK_REGS_VW	= 2,	/* Number of clock enable regs V/W */
	TEGRA_CLK_SOURCES_VW	= 32,	/* Number of ppl clock sources V/W*/
};

/* Clock/Reset Controller (CLK_RST_CONTROLLER_) regs */
struct clk_rst_ctlr {
	uint crc_rst_src;			/* _RST_SOURCE_0,0x00 */
	uint crc_rst_dev[TEGRA_CLK_REGS];	/* _RST_DEVICES_L/H/U_0 */
	uint crc_clk_out_enb[TEGRA_CLK_REGS];	/* _CLK_OUT_ENB_L/H/U_0 */
	uint crc_reserved0;		/* reserved_0,		0x1C */
	uint crc_cclk_brst_pol;		/* _CCLK_BURST_POLICY_0, 0x20 */
	uint crc_super_cclk_div;	/* _SUPER_CCLK_DIVIDER_0,0x24 */
	uint crc_sclk_brst_pol;		/* _SCLK_BURST_POLICY_0, 0x28 */
	uint crc_super_sclk_div;	/* _SUPER_SCLK_DIVIDER_0,0x2C */
	uint crc_clk_sys_rate;		/* _CLK_SYSTEM_RATE_0,	0x30 */
	uint crc_reserved01;		/* reserved_0_1,	0x34 */
	uint crc_reserved02;		/* reserved_0_2,	0x38 */
	uint crc_reserved1;		/* reserved_1,		0x3C */
	uint crc_cop_clk_skip_plcy;	/* _COP_CLK_SKIP_POLICY_0,0x40 */
	uint crc_clk_mask_arm;		/* _CLK_MASK_ARM_0,	0x44 */
	uint crc_misc_clk_enb;		/* _MISC_CLK_ENB_0,	0x48 */
	uint crc_clk_cpu_cmplx;		/* _CLK_CPU_CMPLX_0,	0x4C */
	uint crc_osc_ctrl;		/* _OSC_CTRL_0,		0x50 */
	uint crc_pll_lfsr;		/* _PLL_LFSR_0,		0x54 */
	uint crc_osc_freq_det;		/* _OSC_FREQ_DET_0,	0x58 */
	uint crc_osc_freq_det_stat;	/* _OSC_FREQ_DET_STATUS_0,0x5C */
	uint crc_reserved2[8];		/* reserved_2[8],	0x60-7C */

	struct clk_pll crc_pll[TEGRA_CLK_PLLS];	/* PLLs from 0x80 to 0xdc */

	/* PLLs from 0xe0 to 0xf4    */
	struct clk_pll_simple crc_pll_simple[TEGRA_CLK_SIMPLE_PLLS];

	uint crc_reserved10;		/* _reserved_10,	0xF8 */
	uint crc_reserved11;		/* _reserved_11,	0xFC */

	uint crc_clk_src[TEGRA_CLK_SOURCES]; /*_I2S1_0...	0x100-1fc */

	uint crc_reserved20[32];	/* _reserved_20,	0x200-27c */

	uint crc_clk_out_enb_x;		/* _CLK_OUT_ENB_X_0,	0x280 */
	uint crc_clk_enb_x_set;		/* _CLK_ENB_X_SET_0,	0x284 */
	uint crc_clk_enb_x_clr;		/* _CLK_ENB_X_CLR_0,	0x288 */

	uint crc_rst_devices_x;		/* _RST_DEVICES_X_0,	0x28c */
	uint crc_rst_dev_x_set;		/* _RST_DEV_X_SET_0,	0x290 */
	uint crc_rst_dev_x_clr;		/* _RST_DEV_X_CLR_0,	0x294 */

	uint crc_reserved21[23];	/* _reserved_21,	0x298-2f0 */

	uint crc_dfll_base;		/* _DFLL_BASE_0,	0x2f4 */

	uint crc_reserved22[2];		/* _reserved_22,	0x2f8-2fc */

	/* _RST_DEV_L/H/U_SET_0 0x300 ~ 0x314 */
	struct clk_set_clr crc_rst_dev_ex[TEGRA_CLK_REGS];

	uint crc_reserved30[2];		/* _reserved_30,	0x318, 0x31c */

	/* _CLK_ENB_L/H/U_CLR_0 0x320 ~ 0x334 */
	struct clk_set_clr crc_clk_enb_ex[TEGRA_CLK_REGS];

	uint crc_reserved31;		/* _reserved_31,	0x338 */

	uint crc_ccplex_pg_sm_ovrd;	/* _CCPLEX_PG_SM_OVRD_0,    0x33c */

	uint crc_rst_cpu_cmplx_set;	/* _RST_CPU_CMPLX_SET_0,    0x340 */
	uint crc_rst_cpu_cmplx_clr;	/* _RST_CPU_CMPLX_CLR_0,    0x344 */
	uint crc_clk_cpu_cmplx_set;	/* _CLK_CPU_CMPLX_SET_0,    0x348 */
	uint crc_clk_cpu_cmplx_clr;	/* _CLK_CPU_CMPLX_SET_0,    0x34c */

	uint crc_reserved32[2];		/* _reserved_32,      0x350,0x354 */

	uint crc_rst_dev_vw[TEGRA_CLK_REGS_VW]; /* _RST_DEVICES_V/W_0 */
	uint crc_clk_out_enb_vw[TEGRA_CLK_REGS_VW]; /* _CLK_OUT_ENB_V/W_0 */
	uint crc_cclkg_brst_pol;	/* _CCLKG_BURST_POLICY_0,   0x368 */
	uint crc_super_cclkg_div;	/* _SUPER_CCLKG_DIVIDER_0,  0x36C */
	uint crc_cclklp_brst_pol;	/* _CCLKLP_BURST_POLICY_0,  0x370 */
	uint crc_super_cclkp_div;	/* _SUPER_CCLKLP_DIVIDER_0, 0x374 */
	uint crc_clk_cpug_cmplx;	/* _CLK_CPUG_CMPLX_0,       0x378 */
	uint crc_clk_cpulp_cmplx;	/* _CLK_CPULP_CMPLX_0,      0x37C */
	uint crc_cpu_softrst_ctrl;	/* _CPU_SOFTRST_CTRL_0,     0x380 */
	uint crc_cpu_softrst_ctrl1;	/* _CPU_SOFTRST_CTRL1_0,    0x384 */
	uint crc_cpu_softrst_ctrl2;	/* _CPU_SOFTRST_CTRL2_0,    0x388 */
	uint crc_reserved33[9];		/* _reserved_33,        0x38c-3ac */
	uint crc_clk_src_vw[TEGRA_CLK_SOURCES_VW]; /* _G3D2_0..., 0x3b0-0x42c */
	/* _RST_DEV_V/W_SET_0 0x430 ~ 0x43c */
	struct clk_set_clr crc_rst_dev_ex_vw[TEGRA_CLK_REGS_VW];
	/* _CLK_ENB_V/W_CLR_0 0x440 ~ 0x44c */
	struct clk_set_clr crc_clk_enb_ex_vw[TEGRA_CLK_REGS_VW];
	uint crc_reserved40[12];	/* _reserved_40,	0x450-47C */
	uint crc_pll_cfg0;		/* _PLL_CFG0_0,		0x480 */
	uint crc_pll_cfg1;		/* _PLL_CFG1_0,		0x484 */
	uint crc_pll_cfg2;		/* _PLL_CFG2_0,		0x488 */
};

/* CLK_RST_CONTROLLER_CLK_CPU_CMPLX_0 */
#define CPU1_CLK_STP_RANGE	9:9
#define CPU0_CLK_STP_RANGE	8:8

/* CLK_RST_CONTROLLER_PLLx_BASE_0 */
#define PLL_BYPASS_RANGE	31:31
#define PLL_ENABLE_RANGE	30:30
#define PLL_BASE_OVRRIDE_RANGE	28:28
#define PLL_DIVP_RANGE		22:20
#define PLL_DIVN_RANGE		17:8
#define PLL_DIVM_RANGE		4:0

/* CLK_RST_CONTROLLER_PLLx_MISC_0 */
#define PLL_CPCON_RANGE		11:8
#define PLL_LFCON_RANGE		7:4
#define PLLU_VCO_FREQ_RANGE	20:20
#define PLL_VCO_FREQ_RANGE	3:0

/* CLK_RST_CONTROLLER_OSC_CTRL_0 */
#define OSC_FREQ_RANGE		31:30

/* CLK_RST_CONTROLLER_CLK_SOURCE_x_OUT_0 */
#define OUT_CLK_DIVISOR_RANGE	7:0
#define OUT_CLK_SOURCE_RANGE	31:30
#define OUT_CLK_SOURCE3_RANGE	31:29
#define OUT_CLK_SOURCE42_RANGE	29:28

/* CLK_RST_CONTROLLER_UTMIP_PLL_CFG1_0 */
#define UTMIP_PLLU_ENABLE_DLY_COUNT_RANGE		31:27
#define UTMIP_PLL_SETUP_RANGE				26:18
#define UTMIP_FORCE_PLLU_POWERUP_RANGE			17:17
#define UTMIP_FORCE_PLLU_POWERDOWN_RANGE		16:16
#define UTMIP_FORCE_PLL_ENABLE_POWERUP_RANGE		15:15
#define UTMIP_FORCE_PLL_ENABLE_POWERDOWN_RANGE		14:14
#define UTMIP_FORCE_PLL_ACTIVE_POWERUP_RANGE		13:13
#define UTMIP_FORCE_PLL_ACTIVE_POWERDOWN_RANGE		12:12
#define UTMIP_XTAL_FREQ_COUNT_RANGE			11:0

/* CLK_RST_CONTROLLER_UTMIP_PLL_CFG2_0 */
#define UTMIP_PHY_XTAL_CLOCKEN_RANGE			30:30
#define UTMIP_FORCE_PD_CLK60_POWERUP_RANGE		29:29
#define UTMIP_FORCE_PD_CLK60_POWERDOWN_RANGE		28:28
#define UTMIP_FORCE_PD_CLK48_POWERUP_RANGE		27:27
#define UTMIP_FORCE_PD_CLK48_POWERDOWN_RANGE		26:26
#define UTMIP_PLL_ACTIVE_DLY_COUNT_RANGE		23:18
#define UTMIP_PLLU_STABLE_COUNT_RANGE			17:6
#define UTMIP_FORCE_PD_SAMP_C_POWERUP_RANGE		5:5
#define UTMIP_FORCE_PD_SAMP_C_POWERDOWN_RANGE		4:4
#define UTMIP_FORCE_PD_SAMP_B_POWERUP_RANGE		3:3
#define UTMIP_FORCE_PD_SAMP_B_POWERDOWN_RANGE		2:2
#define UTMIP_FORCE_PD_SAMP_A_POWERUP_RANGE		1:1
#define UTMIP_FORCE_PD_SAMP_A_POWERDOWN_RANGE		0:0
#endif	/* CLK_RST_H */

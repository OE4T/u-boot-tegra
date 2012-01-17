/*
 * (C) Copyright 2010, 2011
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

#ifndef _WARMBOOT_AVP_H_
#define _WARMBOOT_AVP_H_

#define TEGRA_DEV_L		0
#define TEGRA_DEV_H		1
#define TEGRA_DEV_U		2
#define TEGRA_DEV_V		0
#define TEGRA_DEV_W		1

#define SIMPLE_PLLX			(CLOCK_ID_XCPU - CLOCK_ID_FIRST_SIMPLE)
#define SIMPLE_PLLE			(CLOCK_ID_EPCI - CLOCK_ID_FIRST_SIMPLE)

#define TIMER_USEC_CNTR			(NV_PA_TMRUS_BASE + 0)
#define TIMER_USEC_CFG			(NV_PA_TMRUS_BASE + 4)

#define USEC_CFG_DIVISOR_MASK		0xffff

#define CONFIG_CTL_TBE			(1 << 7)
#define CONFIG_CTL_JTAG			(1 << 6)

#define CPU_RST				(1 << 0)
#define CLK_ENB_CPU			(1 << 0)
#define SWR_TRIG_SYS_RST		(1 << 2)
#define SWR_CSITE_RST			(1 << 9)
#define CLK_ENB_CSITE			(1 << 9)

#define CPU_CMPLX_CPU_BRIDGE_CLKDIV_4	(3 << 0)
#define CPU_CMPLX_CPU0_CLK_STP_STOP	(1 << 8)
#define CPU_CMPLX_CPU0_CLK_STP_RUN	(0 << 8)
#define CPU_CMPLX_CPU1_CLK_STP_STOP	(1 << 9)
#define CPU_CMPLX_CPU1_CLK_STP_RUN	(0 << 9)

#define CPU_CMPLX_CPURESET0		(1 << 0)
#define CPU_CMPLX_CPURESET1		(1 << 1)
#define CPU_CMPLX_CPURESET2		(1 << 2)
#define CPU_CMPLX_CPURESET3		(1 << 3)
#define CPU_CMPLX_DERESET0		(1 << 4)
#define CPU_CMPLX_DERESET1		(1 << 5)
#define CPU_CMPLX_DERESET2		(1 << 6)
#define CPU_CMPLX_DERESET3		(1 << 7)
#define CPU_CMPLX_DBGRESET0		(1 << 12)
#define CPU_CMPLX_DBGRESET1		(1 << 13)
#define CPU_CMPLX_DBGRESET2		(1 << 14)
#define CPU_CMPLX_DBGRESET3		(1 << 15)

#define PLLM_OUT1_RSTN_RESET_DISABLE	(1 << 0)
#define PLLM_OUT1_CLKEN_ENABLE		(1 << 1)
#define PLLM_OUT1_RATIO_VAL_8		(8 << 8)

/* CRC_CCLK_BURST_POLICY_0 20h */
#define CCLK_PLLP_BURST_POLICY		0x20004444

/* CRC_SUPER_CCLK_DIVIDER_0 24h */
#define SUPER_CDIV_ENB_ENABLE		(1 << 31)

/* CRC_SCLK_BURST_POLICY_0 28h */
#define SCLK_SYS_STATE_IDLE		(1 << 28)
#define SCLK_SYS_STATE_RUN		(2 << 28)
#define SCLK_SWAKE_FIQ_SRC_CLKM		(0 << 12)
#define SCLK_SWAKE_IRQ_SRC_CLKM		(0 << 8)
#define SCLK_SWAKE_RUN_SRC_CLKM		(0 << 4)
#define SCLK_SWAKE_IDLE_SRC_CLKM	(0 << 0)
#define SCLK_SWAKE_FIQ_SRC_PLLM_OUT1	(7 << 12)
#define SCLK_SWAKE_IRQ_SRC_PLLM_OUT1	(7 << 8)
#define SCLK_SWAKE_RUN_SRC_PLLM_OUT1	(7 << 4)
#define SCLK_SWAKE_IDLE_SRC_PLLM_OUT1	(7 << 0)

/* CRC_CLK_CPU_CMPLX_CLR_0 34ch */
#define CPU_CMPLX_CLR_CPU0_CLK_STP	(1 << 8)

/* CRC_MISC_CLK_ENN_0 48h */
#define MISC_CLK_ENB_EN_PPSB_STOPCLK_ENABLE	(1 << 0)

/* CRC_OSC_CTRL_0 50h */
#define OSC_CTRL_XOE		(1 << 0)
#define OSC_CTRL_XOE_ENABLE	(1 << 0)
#define OSC_CTRL_XOFS		(0x3f << 4)
#define OSC_CTRL_XOFS_4		(0x4 << 4)
#define OSC_CTRL_XODS		(0xf << 16)
#define OSC_CTRL_XODS_4		(0x4 << 16)
#define OSC_CTRL_OSC_FREQ	(0xf << 28)
#define OSC_CTRL_OSC_FREQ_SHIFT	28
#define OSC_FREQ_OSC13			0	/* 13.0MHz */
#define OSC_FREQ_OSC19P2		4	/* 19.2MHz */
#define OSC_FREQ_OSC12			8	/* 12.0MHz */
#define OSC_FREQ_OSC26			12	/* 26.0MHz */
#define OSC_FREQ_OSC16P8		1	/* 16.8MHz */
#define OSC_FREQ_OSC38P4		5	/* 38.4MHz */
#define OSC_FREQ_OSC48			9	/* 48.0MHz */

/* CRC_PLLP_BASE_0 a0h */
#define PLLP_BASE_PLLP_DIVM_SHIFT		0
#define PLLP_BASE_PLLP_DIVN_SHIFT		8
#define PLLP_BASE_PLLP_LOCK_LOCK		(1 << 27)
#define PLLP_BASE_OVRRIDE_ENABLE		(1 << 28)
#define PLLP_BASE_PLLP_ENABLE_ENABLE		(1 << 30)

/* CRC_PLLP_OUTA_0 a4h */
#define PLLP_OUTA_OUT1_RSTN_RESET_DISABLE	(1 << 0)
#define PLLP_OUTA_OUT1_CLKEN_ENABLE		(1 << 1)
#define PLLP_OUTA_OUT1_OVRRIDE_ENABLE		(1 << 2)
#define PLLP_OUTA_OUT1_RATIO_83			(83 << 8)
#define PLLP_OUTA_OUT2_RSTN_RESET_DISABLE	(1 << 16)
#define PLLP_OUTA_OUT2_CLKEN_ENABLE		(1 << 17)
#define PLLP_OUTA_OUT2_OVRRIDE_ENABLE		(1 << 18)
#define PLLP_OUTA_OUT2_RATIO_15			(15 << 24)
#define PLLP_408_OUTA (PLLP_OUTA_OUT2_RATIO_15 |		\
			PLLP_OUTA_OUT2_OVRRIDE_ENABLE |	\
			PLLP_OUTA_OUT2_CLKEN_ENABLE |	\
			PLLP_OUTA_OUT2_RSTN_RESET_DISABLE |	\
			PLLP_OUTA_OUT1_RATIO_83 |	\
			PLLP_OUTA_OUT1_OVRRIDE_ENABLE |	\
			PLLP_OUTA_OUT1_CLKEN_ENABLE |	\
			PLLP_OUTA_OUT1_RSTN_RESET_DISABLE)

/* CRC_PLLP_OUTB_0 a8h */
#define PLLP_OUTA_OUT3_RSTN_RESET_DISABLE	(1 << 0)
#define PLLP_OUTA_OUT3_CLKEN_ENABLE		(1 << 1)
#define PLLP_OUTA_OUT3_OVRRIDE_ENABLE		(1 << 2)
#define PLLP_OUTA_OUT3_RATIO_6			(6 << 8)
#define PLLP_OUTA_OUT4_RSTN_RESET_DISABLE	(1 << 16)
#define PLLP_OUTA_OUT4_CLKEN_ENABLE		(1 << 17)
#define PLLP_OUTA_OUT4_OVRRIDE_ENABLE		(1 << 18)
#define PLLP_OUTA_OUT4_RATIO_6			(6 << 24)
#define PLLP_408_OUTB (PLLP_OUTA_OUT4_RATIO_6 |		\
			PLLP_OUTA_OUT4_OVRRIDE_ENABLE |	\
			PLLP_OUTA_OUT4_CLKEN_ENABLE |	\
			PLLP_OUTA_OUT4_RSTN_RESET_DISABLE |	\
			PLLP_OUTA_OUT3_RATIO_6 |	\
			PLLP_OUTA_OUT3_OVRRIDE_ENABLE |	\
			PLLP_OUTA_OUT3_CLKEN_ENABLE |	\
			PLLP_OUTA_OUT3_RSTN_RESET_DISABLE)

/* CRC_PLLP_MISC_0 ach */
#define PLLP_MISC_PLLP_CPCON_8			(8 << 8)
#define PLLP_MISC_PLLP_LOCK_ENABLE_ENABLE	(1 << 18)

/* CRC_PLLU_BASE_0 c0h */
#define PLLU_BYPASS_ENABLE		(1 << 31)
#define PLLU_ENABLE_ENABLE		(1 << 30)
#define PLLU_REF_DIS_REF_DISABLE	(1 << 29)

/* CRC_PLLU_MISC_0 cch */
#define PLLU_LOCK_ENABLE_ENABLE		(1 << 22)

/* CRC_RST_DEV_L_CLR_0 304h */
#define CLR_SDMMC4_RST_ENABLE		(1 << 15)

/* CRC_CLK_ENB_L_SET_0 320h */
#define SET_CLK_ENB_SDMMC4_ENABLE	(1 << 15)

/* CRC_CLK_ENB_L_CLR_0 324h */
#define CLR_CLK_ENB_SDMMC4_ENABLE	(1 << 15)

/* CRC_CLK_CPU_CMPLX_SET_0 348h */
#define SET_CPU0_CLK_STP_ENABLE		(1 << 8)
#define SET_CPU1_CLK_STP_ENABLE		(1 << 9)
#define SET_CPU2_CLK_STP_ENABLE		(1 << 10)
#define SET_CPU3_CLK_STP_ENABLE		(1 << 11)

/* CRC_CLK_SOURCE_MSELECT_0 3b4 */
#define MSELECT_CLK_SRC_PLLP_OUT0	(0 << 30)

/* CRC_RST_DEV_V_SET_0 430h */
#define SET_MSELECT_RST_ENABLE		(1 << 3)

/* CRC_CLK_ENB_V_SET_0 440h */
#define SET_CLK_ENB_MSELECT_ENABLE	(1 << 3)

/* FLOW_CTLR_HALT_COP_EVENTS_0 04h */
#define EVENT_ZERO_VAL_10		(10 << 0)
#define EVENT_ZERO_VAL_20		(20 << 0)
#define EVENT_ZERO_VAL_250		(250 << 0)
#define EVENT_MSEC			(1 << 24)
#define EVENT_USEC			(1 << 25)
#define EVENT_JTAG			(1 << 28)
#define EVENT_MODE_STOP			(2 << 29)

/* FLOW_CTLR_CLUSTER_CONTROL_0 2ch */
#define ACTIVE_LP			(1 << 0)

/* AHB_ARBITRATION_XBAR_CTRL_0 e0h */
#define ARBITRATION_XBAR_CTRL_PPSB_ENABLE_ENABLE (1 << 2)

/* SDMMC_VENDOR_CLOCK_CNTRL_0 100h */
#define HW_RSTN_OVERRIDE_OVERRIDE	(1 << 4)

/* APBDEV_PMC_PWRGATE_TOGGLE_0 30h */
#define PWRGATE_TOGGLE_PARTID_CP	(0 << 0)
#define PWRGATE_TOGGLE_PARTID_A9LP	(12 << 0)
#define PWRGATE_TOGGLE_START		(1 << 8)

/* APBDEV_PMC_REMOVE_CLAMPING_CMD_0 34h */
#define REMOVE_CLAMPING_CMD_CPU_ENABLE	(1 << 0)
#define REMOVE_CLAMPING_CMD_A9LP_ENABLE	(1 << 12)

/* APBDEV_PMC_PWRGATE_STATUS_0 38 */
#define PWRGATE_STATUS_CPU_ENABLE	(1 << 0)
#define PWRGATE_STATUS_A9LP_ENABLE	(1 << 12)



#define CPU_WAKEUP_CLUSTER		(1 << 31)

#endif


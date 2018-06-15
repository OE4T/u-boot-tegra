/*
 * (C) Copyright 2010-2018 NVIDIA CORPORATION
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 * Derived from code:
 * Copyright (c) 2010-2014, NVIDIA CORPORATION.  All rights reserved.
 */

#include <linux/linkage.h>
#include "mon.h"

	.pushsection ._secure.text, "ax"

#define EMC_CFG					0xc
#define EMC_ADR_CFG				0x10
#define EMC_TIMING_CONTROL			0x28
#define EMC_SELF_REF				0xe0
#define EMC_MRW					0xe8
#define EMC_REQ_CTRL				0x2b0
#define EMC_EMC_STATUS				0x2b4
#define EMC_FBIO_CFG5				0x104
#define EMC_AUTO_CAL_CONFIG			0x2a4
#define EMC_AUTO_CAL_INTERVAL			0x2a8
#define EMC_AUTO_CAL_STATUS			0x2ac
#define EMC_CFG_DIG_DLL				0x2bc
#define EMC_ZCAL_INTERVAL			0x2e0
#define EMC_ZQ_CAL				0x2ec
#define EMC_XM2VTTGENPADCTRL			0x310
#define EMC_XM2VTTGENPADCTRL2			0x314
#define EMC_SEL_DPD_CTRL			0x3d8

#define PMC_CTRL				0x0
#define PMC_CTRL_SIDE_EFFECT_LP0		(1 << 14)  /* enter LP0 when CPU pwr gated */
/* PMC_SCRATCH2 is used for PLLM boot state if PLLM auto-restart is enabled */
#define PMC_SCRATCH2				0x58
#define PMC_SCRATCH38				0x134
#define PMC_IO_DPD_REQ				0x1b8
#define PMC_IO_DPD_STATUS			0x1bc
#define PMC_POR_DPD_CTRL			0x264
#define PMC_IO_DPD3_REQ				0x45c
#define PMC_IO_DPD3_STATUS			0x460

#define CLK_RESET_CCLK_BURST			0x20
#define CCLK_BURST_PLLX_DIV2_BYPASS_LP		(1<<16)
#define CLK_RESET_CCLK_DIVIDER			0x24
#define CLK_RESET_SCLK_BURST			0x28
#define CLK_RESET_SCLK_DIVIDER			0x2c

#define CLK_RESET_PLLC_BASE			0x80
#define CLK_RESET_PLLM_BASE			0x90
#define CLK_RESET_PLLP_BASE			0xa0
#define CLK_RESET_PLLA_BASE			0xb0
#define CLK_RESET_PLLX_BASE			0xe0

#define CLK_RESET_PLLP_RESHIFT			0x528
#define CLK_RESET_PLLP_RESHIFT_DEFAULT		0x3b
#define CLK_RESET_PLLP_RESHIFT_ENABLE		0x3

#define CLK_RESET_PLLC_MISC			0x8c
#define CLK_RESET_PLLM_MISC			0x9c
#define CLK_RESET_PLLP_MISC			0xac
#define CLK_RESET_PLLX_MISC3			0x518
#define CLK_RESET_PLLM_MISC_IDDQ		5
#define CLK_RESET_PLLC_MISC_IDDQ		26
#define CLK_RESET_PLLX_MISC3_IDDQ		3

#define PMC_PLLP_WB0_OVERRIDE			0xf8
#define PMC_PLLM_WB0_OVERRIDE			0x1dc

#define CLK_RESET_CLK_SOURCE_MSELECT		0x3b4
#define CLK_RESET_CLK_ENB_W_SET			0x448
#define CLK_RESET_CLK_ENB_W_CLR			0x44c

#define MSELECT_CLKM				(0x3 << 30)

#define FLOW_CONTROL_CLUSTER_CONTROL		0x2c

#define FLOW_CTLR_HALT_CPU_EVENTS		0x00
#define FLOW_CTLR_HALT_EVENTS_MODE_STOP_UNTIL_IRQ	(4 << 29)
#define FLOW_CTLR_HALT_EVENTS_MODE_WAITEVENT	(2 << 29)
#define FLOW_CTLR_HALT_EVENTS_LIC_IRQ		BIT(11)
#define FLOW_CTLR_HALT_EVENTS_LIC_FIQ		BIT(10)
#define FLOW_CTLR_HALT_EVENTS_GIC_IRQ		BIT(9)
#define FLOW_CTLR_HALT_EVENTS_GIC_FIQ		BIT(8)

#define FLOW_CTLR_CPU_CSR			0x08
#define FLOW_CTLR_CSR_INTR_FLAG			BIT(15)
#define FLOW_CTLR_CSR_EVENT_FLAG		BIT(14)
#define FLOW_CTLR_CSR_IMMEDIATE_WAKE		BIT(3)
#define FLOW_CTLR_CSR_ENABLE			BIT(0)

#define FLOW_CTLR_HALT_CPU1_EVENTS		0x14

#define FLOW_CTLR_CPU1_CSR			0x18

#define PLL_POST_LOCK_DELAY			10  /* Safety delay after lock is detected */
#define LOCK_DELAY 				PLL_POST_LOCK_DELAY

/* waits until the microsecond counter (base) ticks, for exact timing loops */
.macro  wait_for_us, rd, base, tmp
	ldr    \rd, [\base]
1001:   ldr    \tmp, [\base]
	cmp    \rd, \tmp
	beq    1001b
	mov    \tmp, \rd
.endm

/* waits until the microsecond counter (base) is > rn */
.macro	wait_until, rn, base, tmp
	add	\rn, \rn, #1
1002:	ldr	\tmp, [\base]
	sub	\tmp, \tmp, \rn
	ands	\tmp, \tmp, #0x80000000
	dmb	sy
	bne	1002b
.endm

/* Returns the ID of the current processor */
.macro cpu_id, rd
	mrc	p15, 0, \rd, c0, c0, 5
	and	\rd, \rd, #0xF
.endm

/* loads a 32-bit value into a register without a data access */
.macro mov32, reg, val
	movw	\reg, #:lower16:\val
	movt	\reg, #:upper16:\val
.endm

/* Returns the offset of the flow controller csr register for a cpu */
.macro cpu_to_csr_reg rd, rcpu
	cmp	\rcpu, #0
	mov	\rd, #FLOW_CTLR_CPU_CSR
	beq	1001f
	sub	\rd, \rcpu, #1
	lsl	\rd, \rd, #3 // log2(CPU2_CSR - CPU1_CSR)
	add	\rd, \rd, #FLOW_CTLR_CPU1_CSR
1001:
.endm

/* Returns the offset of the flow controller halt register for a cpu */
.macro cpu_to_halt_reg rd, rcpu
	cmp	\rcpu, #0
	mov	\rd, #FLOW_CTLR_HALT_CPU_EVENTS
	beq	1001f
	sub	\rd, \rcpu, #1
	lsl	\rd, \rd, #3 // log2(HALT_CPU2_EVENTS - HALT_CPU1_EVENTS)
	add	\rd, \rd, #FLOW_CTLR_HALT_CPU1_EVENTS
1001:
.endm

.macro emc_device_mask, rd, base
	ldr	\rd, [\base, #EMC_ADR_CFG]
	tst     \rd, #0x1
	moveq	\rd, #(0x1<<8)		@ just 1 device
	movne	\rd, #(0x3<<8)		@ 2 devices
.endm

.macro emc_timing_update, rd, base
	mov	\rd, #1
	str	\rd, [\base, #EMC_TIMING_CONTROL]
1001:
	ldr	\rd, [\base, #EMC_EMC_STATUS]
	tst	\rd, #(0x1<<23)		@ wait until EMC_STATUS_TIMING_UPDATE_STALLED is clear
	bne	1001b
.endm

/*
 * void mon_cpu_shutdown(u32 for_hotplug)
 *
 * Puts the current CPU in wait-for-event mode on the flow controller
 * and powergates it -- flags (in R0) indicate the request type.
 * Must never be called for CPU 0.
 *
 * corrupts r0-r4, r12
 */
ENTRY(mon_cpu_shutdown)
	cpu_id	r3

	ldr	r12, =TEGRA_FLOW_CTLR_BASE
	cpu_to_csr_reg r1, r3
	add	r1, r1, r12	@ CSR address for this CPU
	cpu_to_halt_reg r2, r3
	add	r2, r2, r12	@ HALT_EVENTS address for this CPU

	/*
	 * Clear this CPU's "event" and "interrupt" flags and power gate
	 * it when halting but not before it is in the "WFE" state.
	 */
	movw	r12, \
		FLOW_CTLR_CSR_INTR_FLAG | FLOW_CTLR_CSR_EVENT_FLAG | \
		FLOW_CTLR_CSR_ENABLE
	mov	r4, #(1 << 8)			@ wfi bitmap
	orr	r12, r12, r4, lsl r3
	str	r12, [r1]

	/* Halt this CPU. */
	mov	r3, #0x400
mon_cpu_shutdown_delay:
	subs	r3, r3, #1			@ delay as a part of wfe war.
	bge	mon_cpu_shutdown_delay
	cpsid	a				@ disable imprecise aborts.
	ldr	r3, [r1]			@ read CSR
	str	r3, [r1]			@ clear CSR
	cmp	r0, #0
	mov	r3, #FLOW_CTLR_HALT_EVENTS_MODE_WAITEVENT
	orreq	r3, r3, #FLOW_CTLR_HALT_EVENTS_GIC_IRQ
	orreq	r3, r3, #FLOW_CTLR_HALT_EVENTS_GIC_FIQ
	str	r3, [r2]
	ldr	r0, [r2]
	b	mon_cpu_shutdown_wfe_war

mon_cpu_shutdown_reset_again:
	dsb
	.align 5
	wfi					@ CPU should be power gated here
mon_cpu_shutdown_wfe_war:
	b	mon_cpu_shutdown_reset_again

	/*
	 * 38 nop's, which fills reset of wfe cache line and
	 * 4 more cachelines with nop
	 */
	.rept 38
	nop
	.endr
	b	.				@ should never get here
ENDPROC(mon_cpu_shutdown)

/*
 * mon_cluster_shutdown
 *
 * uses flow controller to enter sleep state
 * This copy is only used for cluster switching; there's another copy that's
 * executed from IRAM for LP0/LP1.
 * executes from SDRAM with target state is LP2
 */
ENTRY(mon_cluster_shutdown)
	ldr	r6, =TEGRA_FLOW_CTLR_BASE

	dsb
	cpu_id	r1

	cpu_to_csr_reg	r2, r1
	ldr	r0, [r6, r2]
	orr	r0, r0, #FLOW_CTLR_CSR_INTR_FLAG | FLOW_CTLR_CSR_EVENT_FLAG
	orr	r0, r0, #FLOW_CTLR_CSR_ENABLE
	str	r0, [r6, r2]

	tst	r0, #FLOW_CTLR_CSR_IMMEDIATE_WAKE
	movne	r0, #FLOW_CTLR_HALT_EVENTS_MODE_WAITEVENT
	moveq	r0, #FLOW_CTLR_HALT_EVENTS_MODE_STOP_UNTIL_IRQ
	orr	r0, r0, #FLOW_CTLR_HALT_EVENTS_LIC_IRQ | FLOW_CTLR_HALT_EVENTS_LIC_FIQ
	cpu_to_halt_reg r2, r1
	str	r0, [r6, r2]
	dsb
	ldr	r0, [r6, r2] /* memory barrier */

1:
	isb
	dsb
	wfi	/* CPU should be power gated here */

	/* !!!FIXME!!! Implement halt failure handler */
	b	1b
ENDPROC(mon_cluster_shutdown)

ENTRY(mon_lp1_shutdown)
	/* preload all the address literals that are needed for the
	 * CPU power-gating process, to avoid loads from SDRAM (which are
	 * not supported once SDRAM is put into self-refresh.
	 * LP0 / LP1 use physical address, since the MMU needs to be
	 * disabled before putting SDRAM into self-refresh to avoid
	 * memory access due to page table walks */
	mov32	r4, TEGRA_PMC_BASE
	mov32	r5, TEGRA_CLK_RESET_BASE
	mov32	r6, TEGRA_FLOW_CTLR_BASE
	mov32	r7, TEGRA_TMRUS_BASE

	ldr	r1, =tegra3_tear_down_core
	adr	r2, tegra3_iram_start
	sub	r1, r1, r2
	mov32	r2, MON_LP0_LP1_ENTRY_IRAM_ADDR
	add	r1, r1, r2

	// Disable MMU (taken from tegra_shut_off_mmu)
	mrc	p15, 0, r3, c1, c0, 0
	movw	r2, #SCTLR_I | SCTLR_Z | SCTLR_C | SCTLR_M
	bic	r3, r3, r2
	dsb
	mcr	p15, 0, r3, c1, c0, 0
	isb
	mov	pc, r1
ENDPROC(mon_lp1_shutdown)

	.ltorg

	/* START OF ROUTINES COPIED TO IRAM */

	.align L1_CACHE_SHIFT
	.globl tegra3_iram_start
tegra3_iram_start:
/*
 * tegra3_lp1_reset
 *
 * reset vector for LP1 restore; copied into IRAM during suspend.
 * brings the system back up to a safe starting point (SDRAM out of
 * self-refresh, PLLC, PLLM and PLLP reenabled, CPU running on PLLP,
 * system clock running on the same PLL that it suspended at), and
 * jumps to mon_reentry to resume the monitor.
 *
 * NOTE: THIS *MUST* BE RELOCATED TO MON_LP0_LP1_ENTRY_IRAM_ADDR AND MUST BE FIRST.
 */
.macro pll_enable, rd, car, base, misc
	ldr	\rd, [\car, #\base]
	tst	\rd, #(1<<30)
	orreq	\rd, \rd, #(1<<30)
	streq	\rd, [\car, #\base]
	.if	\misc
	ldr	\rd, [\car, #\misc]
	bic	\rd, \rd, #(1<<18)
	str	\rd, [\car, #\misc]
	ldr	\rd, [\car, #\misc]
	ldr	\rd, [\car, #\misc]
	orr	\rd, \rd, #(1<<18)
	str	\rd, [\car, #\misc]
	.endif
.endm

.macro pll_locked, rd, car, base
1:
	ldr	\rd, [\car, #\base]
	tst	\rd, #(1<<27)
	beq	1b
.endm

.macro pll_iddq_exit, rd, car, iddq, iddq_bit
	ldr	\rd, [\car, #\iddq]
	bic	\rd, \rd, #(1<<\iddq_bit)
	str	\rd, [\car, #\iddq]
.endm

.macro pll_iddq_entry, rd, car, iddq, iddq_bit
	ldr	\rd, [\car, #\iddq]
	orr	\rd, \rd, #(1<<\iddq_bit)
	str	\rd, [\car, #\iddq]
.endm

ENTRY(tegra3_lp1_reset)
	/* the CPU and system bus are running from CLKM and executing from
	 * IRAM when this code is executed
	 * switch all SCLK/CCLK clocks to CLKM and set non STDBY clock source
	 * enable PLLP, PLLM, PLLC, and PLLX. */

	mov32	r0, TEGRA_CLK_RESET_BASE

	/* secure code handles 32KHz to CLKM/OSC clock switch */
	mov	r1, #(1<<28)
	str	r1, [r0, #CLK_RESET_SCLK_BURST]
	str	r1, [r0, #CLK_RESET_CCLK_BURST]
	mov	r1, #0
	str	r1, [r0, #CLK_RESET_SCLK_DIVIDER]
	str	r1, [r0, #CLK_RESET_CCLK_DIVIDER]

	pll_iddq_exit r1, r0, CLK_RESET_PLLM_MISC, CLK_RESET_PLLM_MISC_IDDQ
	pll_iddq_exit r1, r0, CLK_RESET_PLLC_MISC, CLK_RESET_PLLC_MISC_IDDQ
	pll_iddq_exit r1, r0, CLK_RESET_PLLX_MISC3, CLK_RESET_PLLX_MISC3_IDDQ

	mov32	r7, TEGRA_TMRUS_BASE
	ldr	r1, [r7]
	add	r1, r1, #2
	wait_until r1, r7, r3

	/* enable PLLM via PMC */
	mov32	r2, TEGRA_PMC_BASE
	ldr	r1, [r2, #PMC_PLLP_WB0_OVERRIDE]
	orr	r1, r1, #(1<<12)
	str	r1, [r2, #PMC_PLLP_WB0_OVERRIDE]

	pll_enable r1, r0, CLK_RESET_PLLM_BASE, 0
	pll_enable r1, r0, CLK_RESET_PLLC_BASE, 0
	pll_enable r1, r0, CLK_RESET_PLLX_BASE, 0
	pll_enable r1, r0, CLK_RESET_PLLP_BASE, CLK_RESET_PLLP_MISC

	pll_locked r1, r0, CLK_RESET_PLLM_BASE
	pll_locked r1, r0, CLK_RESET_PLLP_BASE
	pll_locked r1, r0, CLK_RESET_PLLC_BASE
	pll_locked r1, r0, CLK_RESET_PLLX_BASE

	ldr	r1, [r0, #CLK_RESET_PLLP_BASE]
	bic	r1, r1, #(1<<31)                /* disable PllP bypass */
	str	r1, [r0, #CLK_RESET_PLLP_BASE]

	mov	r1, #CLK_RESET_PLLP_RESHIFT_DEFAULT
	str	r1, [r0, #CLK_RESET_PLLP_RESHIFT]

	mov32	r7, TEGRA_TMRUS_BASE
	ldr	r1, [r7]
	add	r1, r1, #LOCK_DELAY
	wait_until r1, r7, r3

	/* re-enable cl_dvfs logic clock (if dfll running, it's in open loop) */
	mov	r4, #(1 << 27)
	str	r4, [r0, #CLK_RESET_CLK_ENB_W_SET]

	/* re-enable cl_dvfs logic clock (if dfll running, it's in open loop) */
	mov	r4, #(1 << 27)
	str	r4, [r0, #CLK_RESET_CLK_ENB_W_SET]

	add	r5, pc, #tegra3_sdram_pad_save-(.+8)	@ r5 --> saved data

	ldr	r4, [r5, #0x18]
	str	r4, [r0, #CLK_RESET_CLK_SOURCE_MSELECT]

	ldr	r4, [r5, #0x1C]
	str	r4, [r0, #CLK_RESET_SCLK_BURST]
	/* first restore PLLX div2 state, 2us delay, then CPU clock source */
	ldr	r4, [r5, #0x20]
	tst	r4, #CCLK_BURST_PLLX_DIV2_BYPASS_LP
	ldr	r1, [r0, #CLK_RESET_CCLK_BURST]
	biceq	r1, r1, #CCLK_BURST_PLLX_DIV2_BYPASS_LP
	orrne	r1, r1, #CCLK_BURST_PLLX_DIV2_BYPASS_LP
	str	r1, [r0, #CLK_RESET_CCLK_BURST]
	ldr	r1, [r7]
	add	r1, r1, #2
	wait_until r1, r7, r3
	str	r4, [r0, #CLK_RESET_CCLK_BURST]

emc_exit_selfrefresh:
	mov32	r0, TEGRA_EMC_BASE		@ r0 reserved for emc base
	add	r5, pc, #tegra3_sdram_pad_save-(.+8)	@ r5 --> saved data

	/* Take BGBIAS pads out of DPD */
	mov32	r1, 0x40020000
	str	r1, [r2, #PMC_IO_DPD3_REQ]

dram_exit_sr_wait4:
	ldr	r1, [r2, #PMC_IO_DPD3_STATUS]
	tst	r1, #(1 << 17)
	bne	dram_exit_sr_wait4

	/* Take VTTGEN pads out of DPD */
	mov32	r1, 0x4CD00000
	str	r1, [r2, #PMC_IO_DPD3_REQ]

dram_exit_sr_wait3:
	ldr	r1, [r2, #PMC_IO_DPD3_STATUS]
	tst	r1, #(1 << 20)
	bne	dram_exit_sr_wait3

	/* Take func pads out of dpd explicitly */
	mov32	r1, 0x430DFFFF
	str	r1, [r2, #PMC_IO_DPD3_REQ]

dram_exit_sr_wait2:
	ldr	r1, [r2, #PMC_IO_DPD3_STATUS]
	tst	r1, #(1 << 18)
	bne	dram_exit_sr_wait2

	mov32	r1, 0x40400000
	str	r1, [r2, #PMC_IO_DPD_REQ]

dram_exit_sr_wait1:
	ldr	r1, [r2, #PMC_IO_DPD_STATUS]
	tst	r1, #(1 << 22)
	bne	dram_exit_sr_wait1

	ldr	r1, [r2, #PMC_POR_DPD_CTRL]
	bic	r1, r1, #0x80000003
	str	r1, [r2, #PMC_POR_DPD_CTRL]

	ldr	r1, [r5, #0x14]	@ PMC_IO_DPD_STATUS
	mvn	r1, r1
	bic	r1, r1, #(0x1<<31)
	orr	r1, r1, #(0x1<<30)
	str	r1, [r2, #PMC_IO_DPD_REQ]

	ldr	r1, [r5, #0x24]	@ PMC_IO_DPD3_STATUS
	mvn	r1, r1
	bic	r1, r1, #(0x1<<31)
	orr	r1, r1, #(0x1<<30)
	str	r1, [r2, #PMC_IO_DPD3_REQ]

exit_self_refresh:
	ldr	r1, [r5, #0xC]
	str	r1, [r0, #EMC_XM2VTTGENPADCTRL]
	ldr	r1, [r5, #0x10]
	str	r1, [r0, #EMC_XM2VTTGENPADCTRL2]
	ldr	r1, [r5, #0x8]
	str	r1, [r0, #EMC_AUTO_CAL_INTERVAL]

	ldr	r1, [r0, #EMC_CFG_DIG_DLL]
	orr	r1, r1, #(0x1<<30)		@ set DLL_RESET
	str	r1, [r0, #EMC_CFG_DIG_DLL]

	emc_timing_update r1, r0

	ldr	r1, [r0, #EMC_AUTO_CAL_CONFIG]
	orr	r1, r1, #(0x1<<31)		@ set AUTO_CAL_ACTIVE
	str	r1, [r0, #EMC_AUTO_CAL_CONFIG]

emc_wait_audo_cal_onetime:
	ldr	r1, [r0, #EMC_AUTO_CAL_STATUS]
	tst	r1, #(0x1<<31)		@ wait until AUTO_CAL_ACTIVE is clear
	bne	emc_wait_audo_cal_onetime

	ldr	r1, [r0, #EMC_CFG]
	bic	r1, r1, #(1<<31)	@ disable DRAM_CLK_STOP
	str	r1, [r0, #EMC_CFG]

	mov	r1, #0
	str	r1, [r0, #EMC_SELF_REF]	@ take DRAM out of self refresh
	mov	r1, #1

	emc_device_mask r1, r0

exit_selfrefresh_loop:
	ldr	r2, [r0, #EMC_EMC_STATUS]
	ands	r2, r2, r1
	bne	exit_selfrefresh_loop

	lsr	r1, r1, #8		@ devSel, bit0:dev0 bit1:dev1

	mov32	r7, TEGRA_TMRUS_BASE
	ldr	r2, [r0, #EMC_FBIO_CFG5]

	and	r2, r2, #3
	cmp	r2, #2
	beq	emc_lpddr2

	mov32	r2, 0x80000011
	str	r2, [r0, #EMC_ZQ_CAL]
	ldr	r2, [r7]
	add	r2, r2, #10
	wait_until r2, r7, r3

	tst	r1, #2
	beq zcal_done

	mov32	r2, 0x40000011
	str	r2, [r0, #EMC_ZQ_CAL]
	ldr	r2, [r7]
	add	r2, r2, #10
	wait_until r2, r7, r3
	b zcal_done

emc_lpddr2:

	mov32	r2, 0x800A00AB
	str	r2, [r0, #EMC_MRW]
	ldr	r2, [r7]
	add	r2, r2, #1
	wait_until r2, r7, r3

	tst	r1, #2
	beq zcal_done

	mov32	r2, 0x400A00AB
	str	r2, [r0, #EMC_MRW]
	ldr	r2, [r7]
	add	r2, r2, #1
	wait_until r2, r7, r3

zcal_done:

	mov	r1, #0
	str	r1, [r0, #EMC_REQ_CTRL]
	ldr	r1, [r5, #0x4]
	str	r1, [r0, #EMC_ZCAL_INTERVAL]
	ldr	r1, [r5, #0x0]
	str	r1, [r0, #EMC_CFG]

	emc_timing_update r1, r0

	ldr	r2, [r7]
	add	r2, r2, #5
	wait_until r2, r7, r3

	mov32	r0, TEGRA_PMC_BASE
	ldr	r0, =mon_reentry
	mov	pc, r0
ENDPROC(tegra3_lp1_reset)

	.align	L1_CACHE_SHIFT
	.type	tegra3_sdram_pad_save, %object
tegra3_sdram_pad_save:
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0

tegra3_sdram_pad_address:
	.word	TEGRA_EMC_BASE + EMC_CFG				@0x0
	.word	TEGRA_EMC_BASE + EMC_ZCAL_INTERVAL			@0x4
	.word	TEGRA_EMC_BASE + EMC_AUTO_CAL_INTERVAL			@0x8
	.word	TEGRA_EMC_BASE + EMC_XM2VTTGENPADCTRL			@0xc
	.word	TEGRA_EMC_BASE + EMC_XM2VTTGENPADCTRL2			@0x10
	.word	TEGRA_PMC_BASE + PMC_IO_DPD_STATUS			@0x14
	.word	TEGRA_CLK_RESET_BASE + CLK_RESET_CLK_SOURCE_MSELECT	@0x18
	.word	TEGRA_CLK_RESET_BASE + CLK_RESET_SCLK_BURST		@0x1c
	.word	TEGRA_CLK_RESET_BASE + CLK_RESET_CCLK_BURST		@0x20
	.word	TEGRA_PMC_BASE + PMC_IO_DPD3_STATUS			@0x24

pllm_state:
	.word	0

/* tegra3_tear_down_core
 *
 * LP0 entry check conditions w.r.t BB take place here
 */
ENTRY(tegra3_tear_down_core)
	bl	tegra3_save_config
	bl	tegra3_sdram_self_refresh
	bl	tegra3_cpu_clk32k
	b	tegra3_enter_sleep
ENDPROC(tegra3_tear_down_core)

/*
 * tegra3_cpu_clk32k
 *
 * In LP0 and LP1 all plls will be turned off.  Switch the system clock
 * to the 32khz clock (clks) and CPU to clkm.
 * r4 = TEGRA_PMC_BASE
 * r5 = TEGRA_CLK_RESET_BASE
 * r6 = TEGRA_FLOW_CTLR_BASE
 * r7 = TEGRA_TMRUS_BASE
 */
tegra3_cpu_clk32k:
	ldr	r0, [r4, #PMC_CTRL]
	tst	r0, #PMC_CTRL_SIDE_EFFECT_LP0
	beq	lp1_clocks_prepare

	/* enable PLLM auto-restart via PMC in LP0; restore override settings */
	ldr	r0, [r4, #PMC_SCRATCH2]
	str	r0, [r4, #PMC_PLLM_WB0_OVERRIDE]
	ldr	r0, [r4, #PMC_PLLP_WB0_OVERRIDE]
	orr	r0, r0, #((1 << 12) | (1 << 11))
	str	r0, [r4, #PMC_PLLP_WB0_OVERRIDE]

	mov	pc, lr

lp1_clocks_prepare:
	/* start by jumping to clkm to safely disable PLLs, then jump
	 * to clks */
	mov	r0, #(1 << 28)
	str	r0, [r5, #CLK_RESET_SCLK_BURST]
	/* 2 us delay between changing sclk and cclk */
	wait_for_us r1, r7, r9
	add	r1, r1, #2
	wait_until r1, r7, r9
	mov	r0, #(1 << 28)
	str	r0, [r5, #CLK_RESET_CCLK_BURST]
	mov	r0, #0
	str	r0, [r5, #CLK_RESET_CCLK_DIVIDER]
	str	r0, [r5, #CLK_RESET_SCLK_DIVIDER]

	/* switch the clock source for mselect to be CLK_M */
	ldr	r0, [r5, #CLK_RESET_CLK_SOURCE_MSELECT]
	orr	r0, r0, #MSELECT_CLKM
	str	r0, [r5, #CLK_RESET_CLK_SOURCE_MSELECT]

	/* disable cl_dvfs logic clock (if dfll running, it's in open loop) */
	mov	r0, #(1 << 27)
	str	r0, [r5, #CLK_RESET_CLK_ENB_W_CLR]
	/* 2 us delay between changing sclk and disabling PLLs */
	wait_for_us r1, r7, r9
	add	r1, r1, #2
	wait_until r1, r7, r9

	/* disable PLLM via PMC in LP1 */
	ldr	r0, [r4, #PMC_PLLP_WB0_OVERRIDE]
	bic	r0, r0, #(1 << 12)
	str	r0, [r4, #PMC_PLLP_WB0_OVERRIDE]

powerdown_pll_pacx:
	ldr	r0, [r6, #FLOW_CONTROL_CLUSTER_CONTROL]
	tst	r0, #1
	ldr	r0, [r5, #CLK_RESET_PLLP_BASE]
	bic	r0, r0, #(1<<30)
	orreq   r0, r0, #(1<<31)                @ enable PllP bypass on fast
	str	r0, [r5, #CLK_RESET_PLLP_BASE]
	mov	r0, #CLK_RESET_PLLP_RESHIFT_ENABLE
	str	r0, [r5, #CLK_RESET_PLLP_RESHIFT]
	ldr	r0, [r5, #CLK_RESET_PLLA_BASE]
	bic	r0, r0, #(1<<30)
	str	r0, [r5, #CLK_RESET_PLLA_BASE]

powerdown_pll_cx:
	ldr	r0, [r5, #CLK_RESET_PLLC_BASE]
	bic	r0, r0, #(1<<30)
	str	r0, [r5, #CLK_RESET_PLLC_BASE]
powerdown_pll_x:
	ldr	r0, [r5, #CLK_RESET_PLLX_BASE]
	bic	r0, r0, #(1<<30)
	str	r0, [r5, #CLK_RESET_PLLX_BASE]
	/*
	 * FIXME: put PLLM/C into IDDQ (need additional testing)
	 * pll_iddq_entry r1, r5, CLK_RESET_PLLM_MISC, CLK_RESET_PLLM_MISC_IDDQ
	 * pll_iddq_entry r1, r5, CLK_RESET_PLLC_MISC, CLK_RESET_PLLC_MISC_IDDQ
	 */
	pll_iddq_entry r1, r5, CLK_RESET_PLLX_MISC3, CLK_RESET_PLLX_MISC3_IDDQ

	/*
	 * Switch to STDBY clock (CLKS), bits 28:31 == 0
	 * Enable burst on CPU IRQ (bit 24)
	 * Set clock sources to CLKM (clock source 0)
	 */
	mov	r0, #(1 << 24)
	str	r0, [r5, #CLK_RESET_SCLK_BURST]
	mov	pc, lr

/*
 * tegra3_enter_sleep
 *
 * uses flow controller to enter sleep state
 * executes from IRAM with SDRAM in selfrefresh when target state is LP0 or LP1
 * executes from SDRAM with target state is LP2
 * r4 = TEGRA_PMC_BASE
 * r5 = TEGRA_CLK_RESET_BASE
 * r6 = TEGRA_FLOW_CTLR_BASE
 * r7 = TEGRA_TMRUS_BASE
 */
tegra3_enter_sleep:
	ldr	r1, [r7]
	str	r1, [r4, #PMC_SCRATCH38]
	dsb
	cpu_id	r1

	cpu_to_csr_reg	r2, r1
	ldr	r0, [r6, r2]
	orr	r0, r0, #FLOW_CTLR_CSR_INTR_FLAG | FLOW_CTLR_CSR_EVENT_FLAG
	orr	r0, r0, #FLOW_CTLR_CSR_ENABLE
	str	r0, [r6, r2]

	tst	r0, #FLOW_CTLR_CSR_IMMEDIATE_WAKE
	movne	r0, #FLOW_CTLR_HALT_EVENTS_MODE_WAITEVENT
	moveq	r0, #FLOW_CTLR_HALT_EVENTS_MODE_STOP_UNTIL_IRQ
	orr	r0, r0, #FLOW_CTLR_HALT_EVENTS_LIC_IRQ | FLOW_CTLR_HALT_EVENTS_LIC_FIQ
	cpu_to_halt_reg r2, r1
	str	r0, [r6, r2]
	dsb
	ldr	r0, [r6, r2] /* memory barrier */

	/* Set the Debug OS Double Lock for Debug Arch v7.1 or greater.
	   With this lock set, the debugger is completely locked out.
	   Disable this to debug WFI/powergating failures.
	*/
	mrc	p15, 0, r3, c0, c1, 2	@ ID_DFR0
	and	r3, r3, #0xF		@ coprocessor debug model
	cmp	r3, #5			@ debug arch >= v7.1?

	mov32	r1, 0xC5ACCE55
	mcrge	p14, 0, r1, c1, c3, 4	@ DBGOSDLR

1:
	isb
	dsb
	wfi	/* CPU should be power gated here */

	/* !!!FIXME!!! Implement halt failure handler */
	b	1b

/*
 * tegra3_sdram_self_refresh
 *
 * called with MMU off and caches disabled
 * puts sdram in self refresh
 * must execute from IRAM
 * r4 = TEGRA_PMC_BASE
 * r5 = TEGRA_CLK_RESET_BASE
 * r6 = TEGRA_FLOW_CTLR_BASE
 * r7 = TEGRA_TMRUS_BASE
 */

tegra3_save_config:
	adr	r2, tegra3_sdram_pad_address
	adr	r8, tegra3_sdram_pad_save
	mov	r9, r2

padsave:
	ldr	r0, [r2], #4			@ r0 is emc register address

	ldr	r1, [r0]
	str	r1, [r8], #4			@ save emc register

	cmp	r8, r9
	bne	padsave
padsave_done:

	dsb
	mov	pc, lr

tegra3_sdram_self_refresh:
	mov32	r0, TEGRA_EMC_BASE		@ r0 reserved for emc base
enter_self_refresh:
	/* Enable SEL_DPD */
	ldr	r1, [r0, #EMC_SEL_DPD_CTRL]
	orr	r1, r1, #0xF
	orr	r1, r1, #0xF0
	orr	r1, r1, #0x100
	str	r1, [r0, #EMC_SEL_DPD_CTRL]
	mov	r1, #0
	str	r1, [r0, #EMC_ZCAL_INTERVAL]
	str	r1, [r0, #EMC_AUTO_CAL_INTERVAL]
	ldr	r1, [r0, #EMC_CFG]
	bic	r1, r1, #(1<<28)
	bic	r1, r1, #(1<<29)
	str	r1, [r0, #EMC_CFG]		@ disable DYN_SELF_REF

	emc_timing_update r1, r0

	ldr	r1, [r7]
	add	r1, r1, #5
	wait_until r1, r7, r2

emc_wait_audo_cal:
	ldr	r1, [r0, #EMC_AUTO_CAL_STATUS]
	tst	r1, #(0x1<<31)		@ wait until AUTO_CAL_ACTIVE is clear
	bne	emc_wait_audo_cal

	mov	r1, #3
	str	r1, [r0, #EMC_REQ_CTRL]		@ stall incoming DRAM requests

emcidle:
	ldr	r1, [r0, #EMC_EMC_STATUS]
	tst	r1, #4
	beq	emcidle

	mov	r1, #1
	str	r1, [r0, #EMC_SELF_REF]

	emc_device_mask r1, r0

emcself:
	ldr	r2, [r0, #EMC_EMC_STATUS]
	and	r2, r2, r1
	cmp	r2, r1
	bne	emcself				@ loop until DDR in self-refresh

	ldr	r1, [r0, #EMC_XM2VTTGENPADCTRL]
	mov32	r2, 0xF8F8FFFF		@ clear XM2VTTGEN_DRVUP and XM2VTTGEN_DRVDN
	and	r1, r1, r2
	str	r1, [r0, #EMC_XM2VTTGENPADCTRL]
	ldr	r1, [r0, #EMC_XM2VTTGENPADCTRL2]
	orr	r1, r1, #0x3f			@ set E_NO_VTTGEN
	str	r1, [r0, #EMC_XM2VTTGENPADCTRL2]

	emc_timing_update r1, r0

	ldr	r1, [r4, #PMC_CTRL]
	tst	r1, #PMC_CTRL_SIDE_EFFECT_LP0
	bne	pmc_io_dpd_skip

	ldr	r1, [r4, #PMC_POR_DPD_CTRL]
	orr	r1, r1, #0x80000003
	str	r1, [r4, #PMC_POR_DPD_CTRL]

	/* Put func pads in dpd explicitly */
	mov32	r1, 0x80400000
	str	r1, [r4, #PMC_IO_DPD_REQ]

dram_sr_wait1:
	ldr	r1, [r4, #PMC_IO_DPD_STATUS]
	tst	r1, #(1 << 22)
	beq	dram_sr_wait1

	mov32	r1, 0x830DFFFF
	str	r1, [r4, #PMC_IO_DPD3_REQ]

dram_sr_wait2:
	ldr	r1, [r4, #PMC_IO_DPD3_STATUS]
	tst	r1, #(1 << 18)
	beq	dram_sr_wait2

	/* Put VTTGEN pads in DPD */
	mov32	r1, 0x8CD00000
	str	r1, [r4, #PMC_IO_DPD3_REQ]

dram_sr_wait3:
	ldr	r1, [r4, #PMC_IO_DPD3_STATUS]
	tst	r1, #(1 << 20)
	beq	dram_sr_wait3

	/* Put BGBIAS pads in DPD */
	mov32	r1, 0x80020000
	str	r1, [r4, #PMC_IO_DPD3_REQ]

dram_sr_wait4:
	ldr	r1, [r4, #PMC_IO_DPD3_STATUS]
	tst	r1, #(1 << 17)
	beq	dram_sr_wait4

	dsb
	mov	pc, lr

pmc_io_dpd_skip:
	dsb
	mov	pc, lr

	.ltorg
/* dummy symbol for end of IRAM */
	.align L1_CACHE_SHIFT
	.globl tegra3_iram_end
tegra3_iram_end:
	b	.

	.popsection

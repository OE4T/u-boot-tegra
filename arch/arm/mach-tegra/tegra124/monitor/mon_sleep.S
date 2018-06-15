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

	.popsection

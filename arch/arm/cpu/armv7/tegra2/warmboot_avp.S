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

#include <asm/arch/tegra2.h>
#include "warmboot_avp.h"

/* Set non-zero to skip resetting CoreSight */
#define DEBUG_DO_NOT_RESET_CORESIGHT	0

	.section	.text
	.align		4

/* ------------------------------------------------------
 *   void wb_start(void)
 *
 * Input:
 *
 * Output:
 *   None
 *
 * Registers Used:
 *   ALL
 *
 * Description:
 *   This function restarts the CPU and then waits for the AVP driver to
 *   tell it where to transfer control to finish restoration of the AVP state.
 * ---------------------------------------------------------------------------
 */

	.globl	wb_start
wb_start:

	.word	0, 0, 0, 0
	.word	0, 0, 0, 0
	.word	0, 0, 0, 0
	.word	0, 0, 0, 0

start:
	ldr	r0, =NV_PA_APB_MISC_BASE	/* R0 = MISC PA base address */
	ldr	r1, =APB_MISC_PP_CONFIG_CTL_JTAG_ENABLE \
		| APB_MISC_PP_CONFIG_CTL_TBE_ENABLE
	str	r1, [r0, #APB_MISC_PP_CONFIG_CTL_OFFSET]

	/* -----------------------------------------------------------------
	 * Load up the base addresses for the register blocks.
	 * -----------------------------------------------------------------
	 */
	ldr	r5, =NV_PA_PMC_BASE	/* R5 = PMC PA base address */
	ldr	r6, =NV_PA_FLOW_BASE	/* R6 = FLOW PA base address */
	ldr	r7, =NV_PA_TMRUS_BASE	/* R7 = TIMERUS PA base address */
	ldr	r8, =NV_PA_CLK_RST_BASE	/* R8 = CLK PA base address */
	ldr	r9, =NV_PA_EVP_BASE	/* R9 = EVP PA base address */
	ldr	r10,=NV_PA_CSITE_BASE	/* R10 = CSITE base address */

	/* -----------------------------------------------------------------
	 * Are we running where we're supposed to be?
	 * -----------------------------------------------------------------
	 */

	ldr	r0, =AP20_WB_RUN_ADDRESS	/* R0 = expected load address */
	add	r1, pc, #here-(.+8)		/* R1 = &here */

here:
	sub	r1, r1, #(here-start)		/* R1 = actual load address */
	ldr	r2, =NV_PA_PG_UP_BASE		/* R2 = PG PA base address */
	ldr	r3, [r2, #PG_UP_TAG_OFFSET]	/* R3 = processor tag */
	ldr	r2, =PG_UP_TAG_AVP		/* R2 = AVP processor tag */
	cmp	r0, r1				/* Addresses match? */
	cmpeq	r2, r3				/* Processor tags match? */
	bne	do_reset			/* No -- reset the chip */


	/* -----------------------------------------------------------------
	 * Get a snapshot of the Usec count. This is a good indicator of
	 * the overhead of BOOTROM after a wake-up event.
	 * -----------------------------------------------------------------
	 */

	ldr	r11, [r7, #TIMERUS_CNTR_1US_OFFSET]

	/* ==================================================================
	 * BEGIN CPU COMPLEX INITIALIZATON
	 * ==================================================================
	 */

#if	!DEBUG_DO_NOT_RESET_CORESIGHT

	/* ----------------------------------------------------------------
	 * Assert CoreSight reset.
	 * ----------------------------------------------------------------
	 */

	mov	r1, #CLK_RST_DEV_U_SET_SET_CSITE_RST
	str	r1, [r8, #CLK_RST_CTLR_RST_DEV_U_SET_OFFSET]

#endif /*!DEBUG_DO_NOT_RESET_CORESIGHT */

	/* Set the drive strength */
	ldr	r1, [r8, #CLK_RST_CTLR_OSC_CTRL_OFFSET]
	ldr	r3, =CLK_RST_CTLR_OSC_CTRL_XOFS_MASK \
		| CLK_RST_CTLR_OSC_CTRL_XOE_MASK
	bic 	r1, r1, r3
	ldr	r3, =CLK_RST_CTLR_OSC_CTRL_XOFS_VAL_4 \
		| CLK_RST_CTLR_OSC_CTRL_XOE_ENABLE
	orr 	r3, r1, r3
	str	r3, [r8, #CLK_RST_CTLR_OSC_CTRL_OFFSET]

	/* -----------------------------------------------------------------
	 * Power up the CPU complex if necessary.
	 * -----------------------------------------------------------------
	 */

	ldr	r3, [r5, #PMC_PWRGATE_STATUS_OFFSET]
	tst	r3, #PMC_PWRGATE_STATUS_CPU_MASK

	ldreq	r2, =PMC_PWRGATE_TOGGLE_PARTID_CPU \
		| PMC_PWRGATE_TOGGLE_START_ENABLE
	streq	r2, [r5, #PMC_PWRGATE_TOGGLE_OFFSET]

is_cpu_on:
	ldr	r3, [r5, #PMC_PWRGATE_STATUS_OFFSET]
	tst	r3, #PMC_PWRGATE_STATUS_CPU_MASK
	beq	is_cpu_on

	/* -----------------------------------------------------------------
	 * Remove the I/O clamps from the CPU power partition.
	 * -----------------------------------------------------------------
	 */

	mov	r3, #PMC_REMOVE_CLAMPING_CMD_CPU_ENABLE
	str	r3, [r5, #PMC_REMOVE_CLAMPING_CMD_OFFSET]

	ldr	r3, =FLOW_CTLR_HALT_COP_EVENTS_ZERO_VAL_20 \
		| FLOW_CTLR_HALT_COP_EVENTS_MSEC \
		| FLOW_CTLR_HALT_COP_EVENTS_MODE_STOP
	str	r3, [r6, #FLOW_CTLR_HALT_COP_EVENTS_OFFSET]

	/* ------------------------------------------------------------------
	 * Assert CPU complex reset.
	 * ------------------------------------------------------------------
	 */

	mov	r1, #CLK_RST_CTLR_RST_DEV_L_SET_CPU_RST
	str	r1, [r8, #CLK_RST_CTLR_RST_DEV_L_SET_OFFSET]

	/* ------------------------------------------------------------------
	 * Hold both CPUs in reset.
	 * ------------------------------------------------------------------
	 */

	ldr	r3, =CLK_RST_CTLR_RST_CPU_CMPLX_SET_CPURESET0 \
		| CLK_RST_CTLR_RST_CPU_CMPLX_SET_CPURESET1 \
		| CLK_RST_CTLR_RST_CPU_CMPLX_SET_DERESET0 \
		| CLK_RST_CTLR_RST_CPU_CMPLX_SET_DERESET1 \
		| CLK_RST_CTLR_RST_CPU_CMPLX_SET_DBGRESET0 \
		| CLK_RST_CTLR_RST_CPU_CMPLX_SET_DBGRESET1
	str	r3, [r8, #CLK_RST_CTLR_RST_CPU_CMPLX_SET_OFFSET]

	/* ------------------------------------------------------------------
	 * Halt CPU1 at the flow controller for uni-processor configurations.
	 * ------------------------------------------------------------------
	 */

	mov	r3, #FLOW_CTLR_HALT_CPU1_EVENTS_MODE_FLOW_MODE_STOP
	str	r3, [r6, #FLOW_CTLR_HALT_CPU1_EVENTS_OFFSET]

	/* -----------------------------------------------------------------
	 * Set the CPU reset vector. SCRATCH41 contains the physical
	 * address of the CPU-side restoration code.
	 * -----------------------------------------------------------------
	 */

	ldr	r3, [r5, #PMC_SCRATCH41_OFFSET]
	str	r3, [r9, #EVP_CPU_RESET_VECTOR_OFFSET]

	/* ------------------------------------------------------------------
	 * Select CPU complex clock source.
	 * ------------------------------------------------------------------
	 */
#define CPU_CLK_SRC	4	/* 4 = PLLP_OUT0 */
	ldr	r3, =(CPU_CLK_SRC << CLK_RST_CTLR_CCLK_CWAKE_FIQ_SOURCE_SHIFT) \
		| (CPU_CLK_SRC << CLK_RST_CTLR_CCLK_CWAKE_IRQ_SOURCE_SHIFT)  \
		| (CPU_CLK_SRC << CLK_RST_CTLR_CCLK_CWAKE_RUN_SOURCE_SHIFT)  \
		| (CPU_CLK_SRC << CLK_RST_CTLR_CCLK_CWAKE_IDLE_SOURCE_SHIFT) \
		| CLK_RST_CTLR_CCLK_CPU_STATE_RUN
	str	r3, [r8, #CLK_RST_CTLR_CCLK_BURST_POLICY_OFFSET]
#undef CPU_CLK_SRC

	/* ------------------------------------------------------------------
	 * Start the CPU0 clock and stop the CPU1 clock.
	 * ------------------------------------------------------------------
	 */

	ldr	r3, =CLK_RST_CTLR_CPU_CMPLX_CPU_BRIDGE_CLKDIV_BY_4 \
		| CLK_RST_CTLR_CPU_CMPLX_CPU0_CLK_STP_RUN \
		| CLK_RST_CTLR_CPU_CMPLX_CPU1_CLK_STP_STOP
	str	r3, [r8, #CLK_RST_CTLR_CPU_CMPLX_OFFSET]

	/* ------------------------------------------------------------------
	 * Enable the CPU complex clock.
	 * ------------------------------------------------------------------
	 */

	mov	r3, #CLK_RST_CTLR_CLK_ENB_L_SET_CLK_ENB_CPU
	str	r3, [r8, #CLK_RST_CTLR_CLK_ENB_L_SET_OFFSET]

	/* -----------------------------------------------------------------
	 * Make sure the resets were held for at least 2 microseconds.
	 * -----------------------------------------------------------------
	 */

	add	r3, r11, #2

wait:
	ldr	r2, [r7, #TIMERUS_CNTR_1US_OFFSET]
	cmp	r2, r3
	ble	wait

#if	!DEBUG_DO_NOT_RESET_CORESIGHT

	/* -----------------------------------------------------------------
	 * De-assert CoreSight reset.
	 * NOTE: We're leaving the CoreSight clock on the oscillator for
	 *	now. It will be restored to its original clock source
	 *	when the CPU-side restoration code runs.
	 * -----------------------------------------------------------------
	 */

	mov	r1, #CLK_RST_CTLR_RST_DEV_U_CLR_CSITE_RST_MASK
	str	r1, [r8, #CLK_RST_CTLR_RST_DEV_U_CLR_OFFSET]

#endif /*!DEBUG_DO_NOT_RESET_CORESIGHT */

	ldr	r1, =0xC5ACCE55			/* R0 = CoreSight unlock value*/
	ldr	r2, =CSITE_CPUDBG0_LAR_OFFSET	/* R1 = CPU0 lock offset */
	ldr	r3, =CSITE_CPUDBG1_LAR_OFFSET	/* R2 = CPU1 lock offset */
	str	r1, [r10, r2]			/* Unlock CPU0 */
	str	r1, [r10, r3]			/* Unlock CPU1 */

	/* -----------------------------------------------------------------
	 * Sample the microsecond timestamp again. This is the time we must
	 * use when returning from LP0 for PLL stabilization delays.
	 * ----------------------------------------------------------------
	 */

	ldr	r11, [r7, #TIMERUS_CNTR_1US_OFFSET]
	str	r11, [r5, #PMC_SCRATCH1_OFFSET]

	/* -----------------------------------------------------------------
	 * Get the oscillator frequency. For 19.2 MHz, just use 19 to
	 * make the calculations easier.
	 * -----------------------------------------------------------------
	 */

	ldr	r4, [r7, #TIMERUS_USEC_CFG_OFFSET]
	and	r4, r4, #TIMERUS_USEC_CFG_USEC_DIVISOR_MASK
	add	r4, r4, #1
	cmp	r4, #26
	MOVGT	r4, #19

	/* PLLX_BASE.PLLX_DIVM */
	ldr	r0, [r5, #PMC_SCRATCH3_OFFSET]
	and	r2, r0, #PMC_SCRATCH3_PLLX_BASE_DIVM_MASK
	cmp	r2, r4
	moveq	r4, #0
	movne	r4, #1

	/* PLLX_BASE.PLLX_DIVN */
	mov	r0, r0, ASR #(PMC_SCRATCH3_PLLX_BASE_DIVN_SHIFT - \
				PMC_SCRATCH3_PLLX_BASE_DIVM_SHIFT)
	ldr	r3, =PMC_SCRATCH3_PLLX_BASE_DIVN_MASK
	and	r1, r0, r3
	orr	r2, r2, r1, LSL #CLK_RST_CTLR_PLLX_BASE_DIVN_SHIFT
	mov	r4, r1, LSL r4

	/* PLLX_BASE.PLLX_DIVP */
	mov	r0, r0, ASR #(PMC_SCRATCH3_PLLX_BASE_DIVP_SHIFT - \
				PMC_SCRATCH3_PLLX_BASE_DIVN_SHIFT)
	and	r1, r0, #PMC_SCRATCH3_PLLX_BASE_DIVP_MASK
	orr	r2, r2, r1, LSL #CLK_RST_CTLR_PLLX_BASE_DIVP_SHIFT
	mov	r4, r4, ASR r1

	/* PLLX_BASE.PLLX_BYPASS_ENABLE | PLLX_BASE.PLLX_ENABLE_DISABLE |
	 * PLLX_BASE.PLLX_REF_DIS_REF_ENABLE
	 */
	orr	r2, r2, #CLK_RST_CTLR_PLLX_BASE_BYPASS_ENABLE \
		| CLK_RST_CTLR_PLLX_BASE_ENABLE_DISABLE \
		| CLK_RST_CTLR_PLLX_BASE_REF_DIS_REF_ENABLE

	/* PLLX_MISC_DCCON must be set for frequencies > 600 MHz. */
	cmp	r4, #600
	movlt	r3, #0
	movge	r3, #CLK_RST_CTLR_PLLX_MISC_DCCON_DEFAULT

	/* PLLX_MISC_LFCON */
	mov	r0, r0, ASR #(PMC_SCRATCH3_PLLX_MISC_LFCON_SHIFT - \
				PMC_SCRATCH3_PLLX_BASE_DIVP_SHIFT)
	and	r1, r0, #PMC_SCRATCH3_PLLX_MISC_LFCON_MASK
	orr	r3, r3, r1, LSL #CLK_RST_CTLR_PLLX_MISC_LFCON_SHIFT

	/* PLLX_MISC_CPCON */
	mov	r0, r0, ASR #(PMC_SCRATCH3_PLLX_MISC_CPCON_SHIFT - \
				PMC_SCRATCH3_PLLX_MISC_LFCON_SHIFT)
	and	r1, r0, #PMC_SCRATCH3_PLLX_MISC_LFCON_MASK
	orr	r3, r3, r1, LSL #CLK_RST_CTLR_PLLX_MISC_CPCON_SHIFT

	str	r3, [r8, #CLK_RST_CTLR_PLLX_MISC_OFFSET]
	str	r2, [r8, #CLK_RST_CTLR_PLLX_BASE_OFFSET]
	orr	r2, r2, #CLK_RST_CTLR_PLLX_BASE_ENABLE_ENABLE
	str	r2, [r8, #CLK_RST_CTLR_PLLX_BASE_OFFSET]
	BIC	r2, r2, #CLK_RST_CTLR_PLLX_BASE_BYPASS_ENABLE
	str	r2, [r8, #CLK_RST_CTLR_PLLX_BASE_OFFSET]

	mov	r3, #0
	str	r3, [r6, #FLOW_CTLR_HALT_CPU_EVENTS_OFFSET]

	ldr	r3, =CLK_RST_CTLR_RST_CPU_CMPLX_CLR_CPURESET0 \
		| CLK_RST_CTLR_RST_CPU_CMPLX_CLR_DBGRESET0 \
		| CLK_RST_CTLR_RST_CPU_CMPLX_CLR_DERESET0
	str	r3, [r8, #CLK_RST_CTLR_RST_CPU_CMPLX_CLR_OFFSET]

	ldr	r1, = CLK_RST_CTLR_PLLM_OUT1_RSTN_RESET_DISABLE \
		| CLK_RST_CTLR_PLLM_OUT1_CLKEN_ENABLE \
		| CLK_RST_CTLR_PLLM_OUT1_RATIO_VAL_8
	str	r1, [r8, #CLK_RST_CTLR_PLLM_OUT_OFFSET]

	ldr	r2, =CLK_RST_CTLR_SCLK_SWAKE_FIQ_SOURCE_PLLM_OUT1 \
		| CLK_RST_CTLR_SCLK_SWAKE_IRQ_SOURCE_PLLM_OUT1 \
		| CLK_RST_CTLR_SCLK_SWAKE_RUN_SOURCE_PLLM_OUT1 \
		| CLK_RST_CTLR_SCLK_SWAKE_IDLE_SOURCE_PLLM_OUT1 \
		| CLK_RST_CTLR_SCLK_SYS_STATE_IDLE
	str	r2, [r8, #CLK_RST_CTLR_SCLK_BURST_POLICY_OFFSET]
	b	avp_resume

	.ltorg
	.align	4

avp_resume:

	mov	r1, #CLK_RST_CTLR_RST_DEV_L_CLR_CPU_RST
	str	r1, [r8, #CLK_RST_CTLR_RST_DEV_L_CLR_OFFSET]

avp_halt:

	mov	r3, #FLOW_CTLR_HALT_COP_EVENTS_MODE_STOP
	orr	r3, r3, #FLOW_CTLR_HALT_COP_EVENTS_JTAG
	str	r3, [r6, #FLOW_CTLR_HALT_COP_EVENTS_OFFSET]
	b	avp_halt

/* ---------------------------------------------------------------------------
 * Prototype:
 *   do_reset
 *
 * Input:
 *   None
 *
 * Output:
 *   None
 *
 * Registers Used:
 *   All
 *
 * Description:
 *   Execution comes here it something goes wrong. The chip is reset and a
 *   cold boot is performed.
 * ---------------------------------------------------------------------------
 */

do_reset:

	mov	r0, #CLK_RST_CTLR_RST_DEVICES_L_SWR_TRIG_SYS_RST
	str	r0, [r8, #CLK_RST_CTLR_RST_DEVICES_L_OFFSET]
	b	.

	.ltorg

	.globl		wb_end
wb_end:

	.end



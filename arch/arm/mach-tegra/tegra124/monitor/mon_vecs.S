/*
 * (C) Copyright 2018 NVIDIA CORPORATION
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 * Derived from code:
 * Copyright (C) 2013,2014 - ARM Ltd
 * Copyright (C) 2015, Siemens AG
 * Copyright (c) 2013 Andre Przywara <andre.przywara//linaro.org>
 */

#include <config.h>
#include <linux/linkage.h>
#include <asm/psci.h>
#include <asm/ptrace.h>
#include "mon.h"

	.pushsection ._secure.text, "ax"

	.align	5
ENTRY(mon_vectors)
	b	mon_exc_reset		// reset
	b	mon_exc_undef		// undef
	b	mon_exc_smc		// smc
	b	mon_exc_pabort		// pabort
	b	mon_exc_dabort		// dabort
	b	mon_exc_hyp		// hyp
	b	mon_exc_irq		// irq
	b	mon_exc_fiq		// fiq
ENDPROC(mon_vectors)

mon_txt_exc:
	.asciz "MON: EXC: "

ENTRY(mon_exc)
	// Save ELR
	mov	r6, lr

	// Get CPU ID
	mrc p15, 0, r4, c0, c0, 5	// MPIDR
	and r4, r4, #0x3		// Aff0 (masked to T124 CPU count)

	// Force use of secure stack
	// stack top = __secure_stack_end - (cpuid << ARM_PSCI_STACK_SHIFT)
	ldr	r5, =__secure_stack_end
	sub	r5, r5, r4, LSL #ARM_PSCI_STACK_SHIFT
	mov	sp, r5

	// Print general exception message
	mov	r4, r0
	adr	r0, mon_txt_exc
	bl	mon_puts

	// Print per-type exception message
	mov	r0, r4
	bl	mon_puts

	// Print ELR
	mov	r0, r6
	mov	r1, #1
	bl	mon_puthex

	// Terminate line
	mov	r0, #'\n'
	bl	mon_putc

	// Hang
	b	mon_error
ENDPROC(mon_exc)

mon_txt_exc_reset:
	.asciz "RESET @"

ENTRY(mon_exc_reset)
	adr	r0, mon_txt_exc_reset
	b	mon_exc
ENDPROC(mon_exc_reset)

mon_txt_exc_undef:
	.asciz "UNDEF @"

ENTRY(mon_exc_undef)
	adr	r0, mon_txt_exc_undef
	b	mon_exc
ENDPROC(mon_exc_undef)

ENTRY(mon_exc_smc)
	// Save all registers; while we should only need to save r4-r14, the
	// kernel SMC code assumes we save everything. We also save everything
	// so that the SMC handler can easily modify any NS state.
	push	{r0-r12, lr}
	// SCR: secure world banked access
	mrc	p15, 0, r5, c1, c1, 0
	bic	r5, r5, #SCR_NS
	mcr	p15, 0, r5, c1, c1, 0
	// Handle the SMC call
	push	{sp} // Pointer to saved NS regs
	bl	mon_smc
	add	sp, sp, #4
	// SCR: non-secure world banked access
	mrc	p15, 0, r5, c1, c1, 0
	orr	r5, r5, #SCR_NS
	mcr	p15, 0, r5, c1, c1, 0
	// Restore saved (possibly modified) registers
	pop	{r0-r12, lr}
	// ERET to NS SW
	movs	pc, lr
ENDPROC(mon_exc_smc)

mon_txt_exc_pabort:
	.asciz "PABORT @"

ENTRY(mon_exc_pabort)
	adr	r0, mon_txt_exc_pabort
	b	mon_exc
ENDPROC(mon_exc_pabort)

mon_txt_exc_dabort:
	.asciz "DABORT @"

ENTRY(mon_exc_dabort)
	adr	r0, mon_txt_exc_dabort
	b	mon_exc
ENDPROC(mon_exc_dabort)

mon_txt_exc_hyp:
	.asciz "HYP @"

ENTRY(mon_exc_hyp)
	adr	r0, mon_txt_exc_hyp
	b	mon_exc
ENDPROC(mon_exc_hyp)

mon_txt_exc_irq:
	.asciz "IRQ @"

ENTRY(mon_exc_irq)
	adr	r0, mon_txt_exc_irq
	b	mon_exc
ENDPROC(mon_exc_irq)

mon_txt_exc_fiq:
	.asciz "FIQ @"

ENTRY(mon_exc_fiq)
	adr	r0, mon_txt_exc_fiq
	b	mon_exc
ENDPROC(mon_exc_fiq)

	.pushsection ._secure.data, "aw"

	.globl mon_entry_handlers
mon_entry_handlers:
	.word mon_entry_initial
	.word mon_entry_unexpected
	.word mon_entry_unexpected
	.word mon_entry_unexpected

	.globl mon_ns_entry_points
mon_ns_entry_points:
	.word 0
	.word 0
	.word 0
	.word 0

	.globl mon_context_ids
mon_context_ids:
	.word 0
	.word 0
	.word 0
	.word 0

	.globl mon_translation_table_ptr
mon_translation_table_ptr:
	.word 0

	.globl mon_lp0_lp1_entry_iram_save_ptr
mon_lp0_lp1_entry_iram_save_ptr:
	.word 0

	.popsection

// One-time monitor startup logic
ENTRY(mon_entry)
	// Save entry r0 in mon_ns_entry_points[cpu_id]
	mrc p15, 0, r4, c0, c0, 5	// MPIDR
	and r4, r4, #0x3		// Aff0 (masked to T124 CPU count)
	ldr	r5, =mon_ns_entry_points
	str	r0, [r5, r4, LSL #2]
	bl	mon_dynamic_alloc
	bl	mon_create_page_tables
	b	mon_reentry
ENDPROC(mon_entry)

// Every-time monitor startup logic
ENTRY(mon_reentry)
	// Switch to monitor mode
	cpsid aif, #MON_MODE

	// SCR: secure world banked access for initial setup
	mov	r5, #(SCR_SIF | SCR_HCE | SCR_AW | SCR_FW)
	mcr	p15, 0, r5, c1, c1, 0

	// SCTLR (S)
	mrc	p15, 0, r5, c1, c0, 0
	bic	r5, r5, #(SCTLR_TE | SCTLR_NMFI | SCTLR_EE)
	bic	r5, r5, #SCTLR_V
	orr	r5, r5, #SCTLR_A
	mcr	p15, 0, r5, c1, c0, 0

	// MON_sp
	// stack top = __secure_stack_end - (cpuid << ARM_PSCI_STACK_SHIFT)
	mrc p15, 0, r4, c0, c0, 5	// MPIDR
	and r4, r4, #0x3		// Aff0 (masked to T124 CPU count)
	ldr	r5, =__secure_stack_end
	sub	r5, r5, r4, LSL #ARM_PSCI_STACK_SHIFT
	mov	sp, r5

	// *VBAR*.
	adr	r5, mon_vectors
	mcr	p15, 0, r5, c12, c0, 1 // MVBAR
	mcr	p15, 0, r5, c12, c0, 0 // VBAR (S)

	// Cache, SMP, MMU enable
	bl	mon_init_l2

	// Enable SMP/coherence
	mrc	p15, 0, r5, c1, c0, 1
	orr	r5, r5, #ACTLR_SMP
	mcr	p15, 0, r5, c1, c0, 1
	isb

	// Enable MMU
	// Any DRAM writes made before this point are not guaranteed to be seen
	// after the MMU is enabled. That's because before this point, the MMU
	// is off, so all accesses are strongly ordered, so will read/write
	// DRAM directly completely ignoring the cach, so are guaranteed not to
	// read or update the cache, and so can leave any cache content
	// incoherent with DRAM content. After this point, DRAM accesses will
	// hit the cache if present in the cache. This incoherence never
	// happens on first boot of a cluster (and hence the monitor as a
	// whole), since the cache has just been powered on and is clean. Thus,
	// the logic in mon_entry to save the boot r0 and create the page
	// tables is safe. This matters when powering on a new CPU within a
	// cluster where the L2 could already hold lines. Thus, it's definitely
	// unsafe to push to the stack before this point and pop afterwards,
	// since that will happen every time through this code, and be exposed
	// to the cluster L2 issue on LP2/hotplug.

	// Translation table address
	ldr	r5, =mon_translation_table_ptr
	ldr	r5, [r5]
	// TTBCR
	mov	r0, #0
	mcr	p15, 0, r0, c2, c0, 2
	// TTBR0
	orr	r0, r5, #0x13
	mcr	p15, 0, r0, c2, c0, 0
	// DACR
	mvn	r0, #0
	mcr	p15, 0, r0, c3, c0, 0
	// Enable MMU
	// SCTLR (S)
	mrc	p15, 0, r5, c1, c0, 0
	orr	r5, r5, #SCTLR_I
	orr	r5, r5, #(SCTLR_C | SCTLR_M)
	mcr	p15, 0, r5, c1, c0, 0
	isb

	// Call per-CPU boot/resume handler
	mrc p15, 0, r4, c0, c0, 5	// MPIDR
	and r4, r4, #0x3		// Aff0 (masked to T124 CPU count)
	ldr	r5, =mon_entry_handlers
	ldr	r6, [r5, r4, LSL #2]
	ldr	r7, =mon_entry_unexpected
	str	r7, [r5, r4, LSL #2]
	blx	r6

	blx	mon_smc_psci_notify_booted

	// Get NS world target PC and context ID
	mrc p15, 0, r4, c0, c0, 5	// MPIDR
	and r4, r4, #0x3		// Aff0 (masked to T124 CPU count)
	ldr	r5, =mon_ns_entry_points
	ldr	lr, [r5, r4, LSL #2]
	ldr	r5, =mon_context_ids
	ldr	r0, [r5, r4, LSL #2]

	// Set SPSR for return to NS world SW
	mov	r5, #(A_BIT | I_BIT | F_BIT)	// Set A, I and F
	tst	lr, #1				// Check for Thumb PC
	orrne	r5, r5, #T_BIT			// Set T if Thumb
	orr	r5, r5, #HYP_MODE		// Slot target mode in
	msr	spsr_cxfs, r5			// Set full SPSR

	// SCR: non-secure world banked access
	mrc	p15, 0, r5, c1, c1, 0
	orr	r5, r5, #SCR_NS
	mcr	p15, 0, r5, c1, c1, 0

	// Clear registers for NS SW
	mov	r1, #0
	mov	r2, #0
	mov	r3, #0
	mov	r4, #0
	mov	r5, #0
	mov	r6, #0
	mov	r7, #0
	mov	r8, #0
	mov	r9, #0
	mov	r10, #0
	mov	r11, #0
	mov	r12, #0

	// ERET to NS world SW
	movs	pc, lr
ENDPROC(mon_reentry)

#define S_DRAM		(1 << 16)
#define TEX_DRAM	(5 << 12)
#define XN_DRAM		(0 << 4)
#define C_DRAM		(0 << 3)
#define B_DRAM		(1 << 2)
#define PXN_DRAM	(0 << 0)

#define S_IRAM		(1 << 16)
#define TEX_IRAM	(5 << 12)
#define XN_IRAM		(1 << 4)
#define C_IRAM		(0 << 3)
#define B_IRAM		(1 << 2)
#define PXN_IRAM	(1 << 0)

#define S_DEV		(0 << 16)
#define TEX_DEV		(0 << 12)
#define XN_DEV		(1 << 4)
#define C_DEV		(0 << 3)
#define B_DEV		(0 << 2)
#define PXN_DEV		(1 << 0)

#define TT_SECTION	BIT(1)

ENTRY(mon_dynamic_alloc)
	// Allocate translation table
	// ... Base address
	ldr	r5, =__secure_stack_end
	//    16k-aligned
	add	r5, r5, #0x3f00
	add	r5, r5, #0x00ff
	bic	r5, r5, #0x3f00
	bic	r5, r5, #0x00ff
	// ... Save the address
	ldr	r1, =mon_translation_table_ptr
	str	r5, [r1]
	// ... Translation table size
	ldr	r6, =(16 * 1024)
	add	r5, r5, r6
	// Allocate IRAM save buffer for LP0/1 entry code
	ldr	r1, =mon_lp0_lp1_entry_iram_save_ptr
	str	r5, [r1]
	add	r5, r5, #MON_LP0_LP1_ENTRY_MAX_SIZE
	bx	lr
ENDPROC(mon_dynamic_alloc)

ENTRY(mon_create_page_tables)
	ldr	r5, =mon_translation_table_ptr
	ldr	r5, [r5]

	// Clear all entries
	ldr	r6, =(16 * 1024)
	mov	r7, r5
	mov	r0, #0
tt_fill_loop:
	str	r0, [r7]
	add	r7, r7, #4
	subs	r6, r6, #4
	bne	tt_fill_loop

	// Translation table entries
	// ... for secure DRAM
#define BASE CONFIG_ARMV7_SECURE_BASE
	ldr	r0, =(BASE & 0xfff00000)
	orr	r0, r0, #(S_DRAM | TEX_DRAM)
	orr	r0, r0, #(XN_DRAM | C_DRAM | B_DRAM | TT_SECTION | PXN_DRAM)
	ldr	r1, =((BASE >> 18) & ~3)
	str	r0, [r5, r1]
#undef BASE

	// ... for IRAM
#define BASE 0x40000000
	ldr	r0, =(BASE & 0xfff00000)
	orr	r0, r0, #(S_IRAM | TEX_IRAM)
	orr	r0, r0, #(XN_IRAM | C_IRAM | B_IRAM | TT_SECTION | PXN_IRAM)
	ldr	r1, =((BASE >> 18) & ~3)
	str	r0, [r5, r1]
#undef BASE

	// ... for peripherals:
	// GIC			0x50040000
#define BASE 0x50040000
	ldr	r0, =(BASE & 0xfff00000)
	orr	r0, r0, #(S_DEV | TEX_DEV)
	orr	r0, r0, #(XN_DEV | C_DEV | B_DEV | TT_SECTION | PXN_DEV)
	ldr	r1, =((BASE >> 18) & ~3)
	str	r0, [r5, r1]
#undef BASE

	// ... for peripherals:
	// EVP			0x6000f100
	// Flow Controller	0x60007000
#define BASE 0x60000000
	ldr	r0, =(BASE & 0xfff00000)
	orr	r0, r0, #(S_DEV | TEX_DEV)
	orr	r0, r0, #(XN_DEV | C_DEV | B_DEV | TT_SECTION | PXN_DEV)
	ldr	r1, =((BASE >> 18) & ~3)
	str	r0, [r5, r1]
#undef BASE

	// ... for peripherals:
	// Debug UART		0x70006300
	// PMC			0x7000e400
#define BASE 0x70000000
	ldr	r0, =(BASE & 0xfff00000)
	orr	r0, r0, #(S_DEV | TEX_DEV)
	orr	r0, r0, #(XN_DEV | C_DEV | B_DEV | TT_SECTION | PXN_DEV)
	ldr	r1, =((BASE >> 18) & ~3)
	str	r0, [r5, r1]
#undef BASE

	bx lr
ENDPROC(mon_create_page_tables)

	.popsection

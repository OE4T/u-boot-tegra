/*
 * (C) Copyright 2010-2018 NVIDIA CORPORATION
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 * Derived from code:
 * Copyright (c) 2010-2014, NVIDIA CORPORATION.  All rights reserved.
 * Copyright (c) 2011, Google, Inc.
 * Copyright (C) 2001 Deep Blue Solutions Ltd.
 * Copyright (C) 2005 ARM Ltd.
 */

#include <linux/linkage.h>
#include "mon.h"

	.pushsection ._secure.text, "ax"

/*
 * void mon_disable_dcache_clean_l1(void)
 *
 * clean & invalidate the L1 cache
 *
 * The flush_cache_all flushes all caches within level of coherence, this
 * may not be desired if all we need is to flush L1 only. Therefore this
 * function is implemented to flush the L1 cache only.
 *
 * Disable is needed before flush to prevent allocations during flush
 * When cache is disabled, we cannot push to stack.
 */
ENTRY(mon_disable_dcache_clean_l1)
	stmfd	sp!, {r4-r5, r7, r9-r11, lr}
	dmb					@ ensure ordering with previous memory accesses

	/*
	 * Disable the S data cache. Don't need to disable NS cache since it's
	 * not running and can't allocate.
	 */
	mrc	p15, 0, r2, c1, c0, 0
	bic	r2, r2, #SCTLR_C
	dsb
	mcr	p15, 0, r2, c1, c0, 0

	/* Clean S and NS lines from the cache. */
	mov	r10, #0
	mcr	p15, 2, r10, c0, c0, 0		@ select cache level 0
	isb
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
1001:
	mov	r9, r4				@ create working copy of max way size
1002:
	orr	r11, r10, r9, lsl r5		@ factor way and cache number into r11
	orr	r11, r11, r7, lsl r2		@ factor index number into r11
	mcr	p15, 0, r11, c7, c14, 2		@ op=c10/c14, clean/flush by set/way
	subs	r9, r9, #1			@ decrement the way
	bge	1002b
	subs	r7, r7, #1			@ decrement the index
	bge	1001b
	mcr	p15, 2, r10, c0, c0, 0		@ restore cache level 0
	isb
	dsb

	ldmfd	sp!, {r4-r5, r7, r9-r11, lr}
	mov	pc, lr
ENDPROC(mon_disable_dcache_clean_l1)

/*
 *	v7_op_dcache_all op
 *
 *	op=c14, Flush the whole D-cache.
 *	op=c10, Clean the whole D-cache.
 *
 *	Corrupted registers: r0-r7, r9-r11 (r6 only in Thumb mode)
 */
.macro v7_op_dcache_all op			@ op=c10 clean, op=c14 flush
	dmb					@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	ands	r3, r0, #0x7000000		@ extract loc from clidr
	mov	r3, r3, lsr #23			@ left align loc bit field
	beq	1005f				@ if loc is 0, then no need to clean
	mov	r10, #0				@ start clean at cache level 0
1001:
	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt	1004f				@ skip if no cache, or just i-cache
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
1002:
	mov	r9, r4				@ create working copy of max way size
1003:
	orr	r11, r10, r9, lsl r5		@ factor way and cache number into r11
	orr	r11, r11, r7, lsl r2		@ factor index number into r11
	mcr	p15, 0, r11, c7, \op, 2		@ op=c10/c14, clean/flush by set/way
	subs	r9, r9, #1			@ decrement the way
	bge	1003b
	subs	r7, r7, #1			@ decrement the index
	bge	1002b
1004:
	add	r10, r10, #2			@ increment cache number
	cmp	r3, r10
	bgt	1001b
1005:
	mov	r10, #0				@ swith back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb
	isb
	mov	pc, lr
.endm

ENTRY(mon_disable_dcache_clean_all)
	stmfd	sp!, {r4-r5, r7, r9-r11, lr}
	dmb					@ ensure ordering

	/*
	 * Disable the S data cache. Don't need to disable NS cache since it's
	 * not running and can't allocate.
	 */
	mrc	p15, 0, r2, c1, c0, 0
	bic	r2, r2, #SCTLR_C
	dsb
	mcr	p15, 0, r2, c1, c0, 0

	/* Clean S and NS lines from the cache. */
	v7_op_dcache_all c14

	ldmfd	sp!, {r4-r5, r7, r9-r11, lr}
	mov	pc, lr
ENDPROC(mon_disable_dcache_clean_all)

	.popsection

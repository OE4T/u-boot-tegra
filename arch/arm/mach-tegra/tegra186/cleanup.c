/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <asm/system.h>

#define SMC_SIP_INVOKE_MCE	0x82FFFF00
#define MCE_SMC_ROC_FLUSH_CACHE	11

void board_post_cleanup_before_linux(void)
{
	struct pt_regs regs;

	isb();

	regs.regs[0] = SMC_SIP_INVOKE_MCE | MCE_SMC_ROC_FLUSH_CACHE;
	smc_call(&regs);
}

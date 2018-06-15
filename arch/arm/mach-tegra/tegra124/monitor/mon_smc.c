/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <asm/io.h>
#include <asm/psci.h>
#include "mon.h"

void mon_text mon_smc(u32 func, u32 arg0, u32 arg1, u32 arg2, u32 *ns_regs)
{
	u32 ret;

#if DEBUG_HIGH_VOLUME
	mon_puts(MON_STR("MON: SMC: "));
	mon_puthex(func, true);
	mon_putc(' ');
	mon_puthex(arg0, true);
	mon_putc(' ');
	mon_puthex(arg1, true);
	mon_putc(' ');
	mon_puthex(arg2, true);
	mon_putc('\n');
#endif

	if (!(func & 0x80000000)) {
		mon_puts(MON_STR("MON: ERR: Yielding SMC\n"));
		ret = ARM_PSCI_RET_NI;
		goto set_ret;
	}

	if (func & 0x40000000) {
		mon_puts(MON_STR("MON: ERR: SMC64\n"));
		ret = ARM_PSCI_RET_NI;
		goto set_ret;
	}

	switch ((func >> 24) & 0x3f) {
	case 2:
		ret = mon_smc_sip(func, arg0, arg1, arg2, ns_regs);
		break;
	case 4:
		ret = mon_smc_psci(func, arg0, arg1, arg2);
		break;
	default:
		mon_puts(MON_STR("MON: ERR: Unknown SMC call range 0x"));
		mon_puthex(func, true);
		mon_putc('\n');
		ret = ARM_PSCI_RET_NI;
	}

set_ret:
	ns_regs[0] = ret;
}

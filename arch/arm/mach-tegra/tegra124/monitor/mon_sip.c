/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <asm/io.h>
#include "mon.h"

#define APBDEV_PMC_SIZE			0x800

#define PMC_READ			0xaa
#define PMC_WRITE			0xbb

#define TEGRA_SIP_PMC_COMMANDS		0x82FFFE00

#define APBDEV_PMC_SEC_DISABLE_0	0x4
#define APBDEV_PMC_PMC_SWRST_0		0x8
#define APBDEV_PMC_SCRATCH1_0		0x54
#define APBDEV_PMC_SCRATCH2_0		0x58
#define APBDEV_PMC_SCRATCH19_0		0x9c
#define APBDEV_PMC_SCRATCH22_0		0xa8
#define APBDEV_PMC_SCRATCH23_0		0xac
#define APBDEV_PMC_SECURE_SCRATCH0_0	0xb0
#define APBDEV_PMC_SECURE_SCRATCH5_0	0xc4
#define APBDEV_PMC_SCRATCH24_0		0xfc
#define APBDEV_PMC_SCRATCH33_0		0x120
#define APBDEV_PMC_SCRATCH35_0		0x128
#define APBDEV_PMC_SCRATCH36_0		0x12c
#define APBDEV_PMC_SCRATCH40_0		0x13c
#define APBDEV_PMC_SCRATCH41_0		0x140
#define APBDEV_PMC_SCRATCH42_0		0x144
#define APBDEV_PMC_SECURE_SCRATCH6_0	0x224
#define APBDEV_PMC_SECURE_SCRATCH7_0	0x228
#define APBDEV_PMC_SCRATCH44_0		0x230
#define APBDEV_PMC_SCRATCH48_0		0x240
#define APBDEV_PMC_SCRATCH50_0		0x248
#define APBDEV_PMC_SCRATCH51_0		0x24c
#define APBDEV_PMC_SEC_DISABLE2_0	0x2c4
#define APBDEV_PMC_SEC_DISABLE3_0	0x2d8
#define APBDEV_PMC_SECURE_SCRATCH8_0	0x300
#define APBDEV_PMC_SECURE_SCRATCH35_0	0x36c
#define APBDEV_PMC_SCRATCH56_0		0x600
#define APBDEV_PMC_SCRATCH108_0		0x6d0
#define APBDEV_PMC_SCRATCH116_0		0x6f0
#define APBDEV_PMC_SCRATCH117_0		0x6f4

static u32 mon_text mon_sip_pmc_commands(u32 cmd, u32 addr, u32 val, u32 *ns_regs)
{
	/* Check the address is within PMC range and is 4byte aligned */
	if ((addr >= APBDEV_PMC_SIZE) || (addr & 0x3)) {
		mon_puts(MON_STR("MON: ERR: Invalid PMC offset 0x"));
		mon_puthex(addr, true);
		mon_putc('\n');
		return -1;
	}

	switch (addr) {
	// Location of LP0 resume code
	case APBDEV_PMC_SCRATCH1_0:
	// Where LP0 resume code jumps to after completion
	case APBDEV_PMC_SCRATCH41_0:
	// Configuration used by boot ROM during LP0 resume
	case APBDEV_PMC_SCRATCH2_0 ... APBDEV_PMC_SCRATCH19_0:
	case APBDEV_PMC_SCRATCH22_0 ... APBDEV_PMC_SCRATCH23_0:
	case APBDEV_PMC_SCRATCH24_0 ... APBDEV_PMC_SCRATCH33_0:
	case APBDEV_PMC_SCRATCH35_0 ... APBDEV_PMC_SCRATCH36_0:
	case APBDEV_PMC_SCRATCH40_0:
	case APBDEV_PMC_SCRATCH42_0:
	case APBDEV_PMC_SCRATCH44_0 ... APBDEV_PMC_SCRATCH48_0:
	case APBDEV_PMC_SCRATCH50_0 ... APBDEV_PMC_SCRATCH51_0:
	case APBDEV_PMC_SCRATCH56_0 ... APBDEV_PMC_SCRATCH108_0:
	case APBDEV_PMC_SCRATCH116_0 ... APBDEV_PMC_SCRATCH117_0:
	// All secure scratch registers
	// These mostly aren't used, but protecting them pro-actively in case
	// the monitor is enhanced to use these.
	// (Some used by boot ROM during LP0 resume)
	case APBDEV_PMC_SECURE_SCRATCH0_0 ... APBDEV_PMC_SECURE_SCRATCH5_0:
	case APBDEV_PMC_SECURE_SCRATCH6_0 ... APBDEV_PMC_SECURE_SCRATCH7_0:
	case APBDEV_PMC_SECURE_SCRATCH8_0 ... APBDEV_PMC_SECURE_SCRATCH35_0:
	// Configuration of TZ access to registers
	// (Some used by boot ROM during LP0 resume)
	case APBDEV_PMC_SEC_DISABLE_0:
	case APBDEV_PMC_SEC_DISABLE2_0:
	case APBDEV_PMC_SEC_DISABLE3_0:
	// PMC-only reset (documented as a no-op)
	// Presumably resets PMC_SCRATCH1 to 0
	case APBDEV_PMC_PMC_SWRST_0:
		mon_puts(MON_STR("MON: ERR: Disallowed PMC offset 0x"));
		mon_puthex(addr, true);
		mon_putc('\n');
		return -1;
	default:
		// All other registers are OK
		break;
	}

	/* Perform PMC read/write */
	if (cmd == PMC_READ) {
		ns_regs[1] = readl(NV_PA_PMC_BASE + addr);
		return 0;
	} else if (cmd == PMC_WRITE) {
		writel(val, NV_PA_PMC_BASE + addr);
		return 0;
	} else {
		mon_puts(MON_STR("MON: ERR: Unknown PMC command 0x"));
		mon_puthex(cmd, true);
		mon_putc('\n');
		return -1;
	}
}

u32 mon_text mon_smc_sip(u32 func, u32 arg0, u32 arg1, u32 arg2, u32 *ns_regs)
{
	switch (func) {
	case TEGRA_SIP_PMC_COMMANDS:
		return mon_sip_pmc_commands(arg0, arg1, arg2, ns_regs);
	default:
		mon_puts(MON_STR("MON: ERR: Unknown SIP 0x"));
		mon_puthex(func, true);
		mon_putc('\n');
		return -1;
	}
}

/*
 * Copyright (c) 2018, NVIDIA CORPORATION.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch-tegra/clk_rst.h>
#include "mon.h"

#if DEBUG
#define UART_BASE	0x70006300
#define UART_TX		0
#define UART_DLL	0
#define UART_DLM	1
#define UART_LCR	3
#define UART_LCR_DLAB	0x80
#define UART_LCR_WLEN8	0x03
#define UART_LSR	5
#define UART_LSR_TEMT	0x40
#define UART_LSR_THRE	0x20
#define UART_REG(r)	(UART_BASE + ((r) * 4))

void mon_text mon_putc(char c)
{
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	u32 val;
	u32 lsr_mask = UART_LSR_TEMT | UART_LSR_THRE;

	// These early return conditions occur during LP0 resume. The monitor
	// can't resume the UART during LP0 resume, since that requires clearing
	// IO DPD, and that relies on full pinmux resume first, and only the OS
	// has enough information to do that.

	// UART in reset? If so, can't touch it.
	val = readl(&clkrst->crc_rst_dev[2]);
	if (val & BIT(1))
		return;

	// UART in clock? If so, can't touch it.
	val = readl(&clkrst->crc_clk_out_enb[2]);
	if (!(val & BIT(1)))
		return;

	while ((readl(UART_REG(UART_LSR)) & lsr_mask) != lsr_mask)
		;
	if (c == '\n')
		writel('\r', UART_REG(UART_TX));
	writel(c, UART_REG(UART_TX));
}

void mon_text mon_puts(const char *s)
{
	while (*s)
		mon_putc(*s++);
}

void mon_text mon_puthex(u32 val, bool zfill)
{
	int shift = 28;
	bool seen_nz = false;
	while (shift >= 0) {
		u32 digit = (val >> shift) & 0xf;
		if (digit)
			seen_nz = true;
		if (seen_nz) {
			u32 c;
			if (digit < 10)
				c = digit + '0';
			else
				c = (digit - 10) + 'A';
			mon_putc(c);
		}
		shift -= 4;
		if (shift == 0)
			seen_nz = true;
	}
}

void mon_text mon_put_cpuid(void)
{
	u32 cpu_id = mon_read_mpidr();
	mon_puthex((cpu_id >> 8) & 0xff, false);
	mon_putc('.');
	mon_puthex(cpu_id & 0xff, false);
}
#else
void mon_text mon_putc(char c)
{
}

void mon_text mon_puts(const char *s)
{
}

void mon_text mon_puthex(u32 val, bool zfill)
{
}

void mon_text mon_put_cpuid(void)
{
}
#endif

/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
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
 *
 * OXPCIe 952 support
 */

#include <common.h>
#include <config.h>
#include <linux/types.h>
#include <serial.h>
#include <asm/io.h>
#include <ns16550.h>

DECLARE_GLOBAL_DATA_PTR;

static int OXPCIe952_init(void);
static int OXPCIe952_uninit(void);
static void OXPCIe952_setbrg(void);
static int OXPCIe952_getc(void);
static int OXPCIe952_tstc(void);
static void OXPCIe952_putc(const char c);
static void OXPCIe952_puts(const char *s);

struct serial_device oxpcie952_device = {
	"oxpcie952_0",
	"OXUART0",
	OXPCIe952_init,
	OXPCIe952_uninit,
	OXPCIe952_setbrg,
	OXPCIe952_getc,
	OXPCIe952_tstc,
	OXPCIe952_putc,
	OXPCIe952_puts,
	NULL
};

static struct NS16550 *uart = (struct NS16550 *)(uintptr_t)(0xe0401000);

static int OXPCIe952_init(void)
{
	return 0;
}

static int OXPCIe952_uninit(void)
{
	return 0;
}

static void OXPCIe952_setbrg(void)
{
}

static int OXPCIe952_havechar(void)
{
	return readb(&uart->lsr) & UART_LSR_DR;
}

static int OXPCIe952_getc(void)
{
	while (!OXPCIe952_havechar());
	return (int)readb(&uart->rbr);
}

static int OXPCIe952_tstc(void)
{
	return OXPCIe952_havechar();
}

static void OXPCIe952_putc(const char c)
{
	if (c == '\n') {
		OXPCIe952_putc('\r');
	}
	while ((readb(&uart->lsr) & UART_LSR_THRE) == 0);
	writeb(c, &uart->rbr);
}

static void OXPCIe952_puts(const char *s)
{
	while (*s) OXPCIe952_putc(*s++);
}

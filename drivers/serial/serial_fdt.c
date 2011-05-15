/*
 * Copyright (c) 2011 The Chromium OS Authors.
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

/*
 * This is a serial port provided by the flattened device tree. It works
 * by selecting a compiled in driver according to the setting in the FDT.
 */

#include <common.h>
#include <fdt_decode.h>
#include <ns16550.h>
#include <serial.h>
#include <serial_fdt.h>


DECLARE_GLOBAL_DATA_PTR;

/*
 * We need these structures to be outside BSS since they are accessed before
 * relocation.
 */
static struct serial_device console = {
	"not_in_bss"
};

struct fdt_uart console_uart = {
	-1U
};

/* Access the console - this may need to be a function */
#define DECLARE_CONSOLE	struct fdt_uart *uart = &console_uart

/* Initialize the serial port */
static int fserial_init(void)
{
	DECLARE_CONSOLE;

	switch (uart->compat) {
#ifdef CONFIG_SYS_NS16550
	case COMPAT_SERIAL_NS16550:
		NS16550_init((NS16550_t)uart->reg, uart->divisor);
		break;
#endif
	default:
		break;
	}
	return 0;
}

static void fserial_putc(const char c)
{
	DECLARE_CONSOLE;

	if (c == '\n')
		fserial_putc('\r');
	switch (uart->compat) {
#ifdef CONFIG_SYS_NS16550
	case COMPAT_SERIAL_NS16550 :
		NS16550_putc((NS16550_t)uart->reg, c);
		break;
#endif
	default:
		break;
	}
}

static void fserial_puts(const char *s)
{
	while (*s)
		fserial_putc(*s++);
}

static int fserial_getc(void)
{
	DECLARE_CONSOLE;

	switch (uart->compat) {
#ifdef CONFIG_SYS_NS16550
	case COMPAT_SERIAL_NS16550 :
		return NS16550_getc((NS16550_t)uart->reg, uart->id);
#endif
	default:
		break;
	}
	/* hang */
	for (;;) ;
}

static int fserial_tstc(void)
{
	DECLARE_CONSOLE;

	switch (uart->compat) {
#ifdef CONFIG_SYS_NS16550
	case COMPAT_SERIAL_NS16550 :
		return NS16550_tstc((NS16550_t)uart->reg, uart->id);
#endif
	default:
		break;
	}
	return 0;
}

static void fserial_setbrg(void)
{
	DECLARE_CONSOLE;

	uart->baudrate = gd->baudrate;
	fdt_decode_uart_calc_divisor(uart);
	switch (uart->compat) {
#ifdef CONFIG_SYS_NS16550
	case COMPAT_SERIAL_NS16550 :
		NS16550_reinit((NS16550_t)uart->reg, uart->divisor);
		break;
#endif
	default:
		break;
	}
}

static struct serial_device *get_console(void)
{
	if (fdt_decode_uart_console(gd->blob, &console_uart,
			gd->baudrate))
		return NULL;
	strcpy(console.name, "serial");
	strcpy(console.ctlr, "fdt");
	console.init = fserial_init;
	console.uninit = NULL;
	console.setbrg = fserial_setbrg;
	console.getc = fserial_getc;
	console.tstc = fserial_tstc;
	console.putc = fserial_putc;
	console.puts = fserial_puts;
	return &console;
}

struct serial_device *serial_fdt_get_console_f(void)
{
	/* if the uart isn't already set up, do it now */
	if (console_uart.reg == -1U)
		return get_console();

	/* otherwise just return the current information */
	return &console;
}


struct serial_device *serial_fdt_get_console_r(void)
{
	/*
	 * Relocation moves all our function pointers, so we need to set up
	 * things again. This function will only be called once.
	 *
	 * We cannot do the -1 check as in serial_fdt_get_console_f
	 * because it will be -1 if that function has been ever been called.
	 * However, the function pointers set up by serial_fdt_get_console_f
	 * will be pre-relocation values, so we must re-calculate them.
	 */
	return get_console();
}

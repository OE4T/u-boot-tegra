/*
 * Copyright (c) 2011 The Chromium OS Authors.
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

#include <common.h>
#include <asm/io.h>
#include <ns16550.h>
#include <libfdt.h>
#include <fdt_decode.h>
#include <fdt_support.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/tegra2.h>
#include <asm/arch/tegra2_spi.h>
#include "uart-spi-fix.h"


/* position of the UART/SPI select switch */
enum spi_uart_switch {
	SWITCH_UNKNOWN,
	SWITCH_SPI,
	SWITCH_UART,
	SWITCH_BOTH
};

static struct fdt_spi_uart local;
static enum spi_uart_switch switch_pos; /* Current switch position */

static void get_config(const void *blob, struct fdt_spi_uart *config)
{
	config->gpio = -1; /* just in case the config does not exist */

#ifdef CONFIG_OF_CONTROL
	fdt_decode_get_spi_switch(blob, config);
#elif defined CONFIG_SPI_CORRUPTS_UART
	config->gpio = UART_DISABLE_GPIO;
	config->regs = (NS16550_t)CONFIG_SPI_CORRUPTS_UART;
	config->port = CONFIG_SPI_CORRUPTS_UART_NR;
#endif
}

/*
 * Init the UART / SPI switch. This can be called before relocation so we must
 * not access BSS.
 */
void gpio_early_init_uart(const void *blob)
{
	struct fdt_spi_uart config;

	get_config(blob, &config);
	if (config.gpio != -1)
		gpio_direction_output(config.gpio, 0);
}

/*
 * Configure the UART / SPI switch.
 */
void gpio_config_uart(const void *blob)
{
	get_config(blob, &local);
	switch_pos = SWITCH_BOTH;
	if (local.gpio != -1) {
		gpio_direction_output(local.gpio, 0);
		switch_pos = SWITCH_UART;
	}
}

#ifdef CONFIG_SPI_UART_SWITCH

static void spi_uart_switch(struct fdt_spi_uart *config,
			      enum spi_uart_switch new_pos)
{
	if (switch_pos == SWITCH_BOTH || new_pos == switch_pos)
		return;

	/* if the UART was selected, allow it to drain */
	if (switch_pos == SWITCH_UART)
		NS16550_drain(config->regs, config->port);

	/* We need to dynamically change the pinmux, shared w/UART RXD/CTS */
	pinmux_set_func(PINGRP_GMC, new_pos == SWITCH_SPI ?
				PMUX_FUNC_SFLASH : PMUX_FUNC_UARTD);

	/*
	* On Seaboard, MOSI/MISO are shared w/UART.
	* Use GPIO I3 (UART_DISABLE) to tristate UART during SPI activity.
	* Enable UART later (cs_deactivate) so we can use it for U-Boot comms.
	*/
	gpio_direction_output(config->gpio, new_pos == SWITCH_SPI);
	switch_pos = new_pos;

	/* if the SPI was selected, clear any junk bytes in the UART */
	if (switch_pos == SWITCH_UART) {
		/* TODO: What if it is part-way through clocking in junk? */
		udelay(100);
		NS16550_clear(config->regs, config->port);
	}
}

void uart_enable(NS16550_t regs)
{
	/* Also prevents calling spi_uart_switch() before relocation */
	if (regs == local.regs)
		spi_uart_switch(&local, SWITCH_UART);
}

void spi_enable(void)
{
	spi_uart_switch(&local, SWITCH_SPI);
}

#endif

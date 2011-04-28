/*
 *  (C) Copyright 2010,2011
 *  NVIDIA Corporation <www.nvidia.com>
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
#include <asm/arch/bitfield.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/tegra2.h>
#include <asm/arch/tegra2_spi.h>

static enum spi_uart_switch switch_pos;

/*
 * Routine: gpio_config_uart
 * Description: Force GPIO_PI3 low on Seaboard so UART4 works.
 */
void gpio_config_uart(void)
{
	gpio_direction_output(UART_DISABLE_GPIO, 0);
	switch_pos = SWITCH_UART;
}

#ifdef CONFIG_SPI_CORRUPTS_UART

void seaboard_switch_spi_uart(enum spi_uart_switch new_pos)
{
	struct pmux_tri_ctlr *pmt = (struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;

	if (new_pos == switch_pos)
		return;

	/* if the UART was selected, allow it to drain */
	if (switch_pos == SWITCH_UART)
		NS16550_drain((NS16550_t)CONFIG_SPI_CORRUPTS_UART,
			      CONFIG_SPI_CORRUPTS_UART_NR);

	/* We need to dynamically change the pinmux, shared w/UART RXD/CTS */
	bf_writel(GMC_SEL_SFLASH, new_pos == SWITCH_SPI ? 3 : 0,
			&pmt->pmt_ctl_b);

	/*
	* On Seaboard, MOSI/MISO are shared w/UART.
	* Use GPIO I3 (UART_DISABLE) to tristate UART during SPI activity.
	* Enable UART later (cs_deactivate) so we can use it for U-Boot comms.
	*/
	gpio_direction_output(UART_DISABLE_GPIO, new_pos == SWITCH_SPI);
	switch_pos = new_pos;

	/* if the SPI was selected, clear any junk bytes in the UART */
	if (switch_pos == SWITCH_UART) {
		/* TODO: What if it is part-way through clocking in junk? */
		udelay(100);
		NS16550_clear((NS16550_t)CONFIG_SPI_CORRUPTS_UART,
			      CONFIG_SPI_CORRUPTS_UART_NR);
	}
}

void enable_uart(void)
{
	seaboard_switch_spi_uart(SWITCH_UART);
}

#endif

/*
 * Copyright (c) 2010-2011 NVIDIA Corporation
 * With help from the mpc8xxx SPI driver
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

#include <malloc.h>
#include <spi.h>
#include <asm/clocks.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/pinmux.h>
#if !defined(CONFIG_USE_SLINK)
#include <ns16550.h>		/* for NS16550_drain and NS16550_clear */
#include <asm/arch/gpio.h>
#include "uart-spi-fix.h"
#endif
#include "tegra_spi.h"

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	/* Tegra SPI - only 1 device ('bus/cs') */
	if (bus > 0 && cs != 0)
		return 0;
	else
		return 1;
}


struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode)
{
	struct spi_slave *slave;

	if (!spi_cs_is_valid(bus, cs))
		return NULL;

	slave = malloc(sizeof(struct spi_slave));
	if (!slave)
		return NULL;

	slave->bus = bus;
	slave->cs = cs;

	/*
	 * Currently, Tegra SPI uses mode 0 & a 24MHz clock.
	 * Use 'mode' and 'maz_hz' to change that here, if needed.
	 */

	return slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	free(slave);
}

void spi_init(void)
{
	struct spi_tegra *spi = (struct spi_tegra *)TEGRA_SPI_BASE;
	u32 reg;
	enum periph_id id;

#if defined(CONFIG_USE_SLINK)
	id = PERIPH_ID_SBC4;
#else
	id = PERIPH_ID_SPI1;
#endif
	/* Change SPI clock to 24MHz, PLLP_OUT0 source */
	clock_start_periph_pll(id, CLOCK_ID_PERIPH, CLK_24M);

	/* Clear stale status here */
	reg = SPI_STAT_RDY | SPI_STAT_RXF_FLUSH | SPI_STAT_TXF_FLUSH | \
		SPI_STAT_RXF_UNR | SPI_STAT_TXF_OVF;
	writel(reg, &spi->status);
	debug("spi_init: STATUS = %08x\n", readl(&spi->status));

#if defined(CONFIG_USE_SLINK)
	/* Set master mode */
	reg = readl(&spi->command);
	writel(reg | SPI_CMD_M_S, &spi->command);
	debug("spi_init: COMMAND = %08x\n", readl(&spi->command));
#endif	/* SLINK */

	/*
	 * Use sw-controlled CS, so we can clock in data after ReadID, etc.
	 */

	reg = readl(&spi->command);
	writel(reg | SPI_CMD_CS_SOFT, &spi->command);
	debug("spi_init: COMMAND = %08x\n", readl(&spi->command));

#if !defined(CONFIG_USE_SLINK)
	/*
	 * SPI pins on Tegra2 Seaboard are muxed - change pinmux last due
	 * to UART issue.
	 */
	pinmux_set_func(PINGRP_GMD, PMUX_FUNC_SFLASH);
	pinmux_tristate_disable(PINGRP_LSPI);

	/*
	 * NOTE:
	 * Don't set PinMux bits 3:2 to SPI here or subsequent UART data
	 * won't go out! It'll be correctly set in spi_uart_switch().
	 */
#endif	/* !SLINK */
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	/*
	 * We can't release UART_DISABLE and set pinmux to UART4 here since
	 * some code (e,g, spi_flash_probe) uses printf() while the SPI
	 * bus is held. That is arguably bad, but it has the advantage of
	 * already being in the source tree.
	 */
}

void spi_cs_activate(struct spi_slave *slave)
{
	struct spi_tegra *spi = (struct spi_tegra *)TEGRA_SPI_BASE;
	u32 val;

#if !defined(CONFIG_USE_SLINK)
	spi_enable();
#endif
	/* CS is negated on Tegra, so drive a 1 to get a 0 */
	val = readl(&spi->command);
	writel(val | SPI_CMD_CS_VAL, &spi->command);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	struct spi_tegra *spi = (struct spi_tegra *)TEGRA_SPI_BASE;
	u32 val;

	/* CS is negated on Tegra, so drive a 0 to get a 1 */
	val = readl(&spi->command);
	writel(val & ~SPI_CMD_CS_VAL, &spi->command);
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
	struct spi_tegra *spi = (struct spi_tegra *)TEGRA_SPI_BASE;
	unsigned int status;
	int num_bytes = (bitlen + 7) / 8;
	int i, ret, tm, bytes, bits, isRead = 0;
	u32 reg, tmpdout, tmpdin = 0;

	debug("spi_xfer: slave %u:%u dout %08X din %08X bitlen %u\n",
	      slave->bus, slave->cs, *(u8 *)dout, *(u8 *)din, bitlen);

	ret = tm = 0;

	status = readl(&spi->status);
	writel(status, &spi->status);	/* Clear all SPI events via R/W */
	debug("spi_xfer entry: STATUS = %08x\n", status);
#if defined(CONFIG_USE_SLINK)
	reg = readl(&spi->status2);
	writel(reg, &spi->status2);	/* Clear all STATUS2 events via R/W */
	debug("spi_xfer entry: STATUS2 = %08x\n", reg);

	debug("spi_xfer entry: COMMAND = %08x\n", readl(&spi->command));
	reg = readl(&spi->command2);
	writel((reg |= (SPI_CMD2_TXEN | SPI_CMD2_RXEN)), &spi->command2);
	writel((reg |= SPI_CMD2_SS_EN), &spi->command2);
	debug("spi_xfer: COMMAND2 = %08x\n", readl(&spi->command2));
#else	/* SPIFLASH a la Tegra2 */
	reg = readl(&spi->command);
	writel((reg |= (SPI_CMD_TXEN | SPI_CMD_RXEN)), &spi->command);
	debug("spi_xfer: COMMAND = %08x\n", readl(&spi->command));
#endif
	if (flags & SPI_XFER_BEGIN)
		spi_cs_activate(slave);

	/* handle data in 32-bit chunks */
	while (num_bytes > 0) {
		tmpdout = 0;
		bytes = (num_bytes >  4) ?  4 : num_bytes;
		bits  = (bitlen   > 32) ? 32 : bitlen;

		if (dout != NULL) {
			for (i = 0; i < bytes; ++i)
				tmpdout = (tmpdout << 8) | ((u8 *) dout)[i];
		}

		num_bytes -= bytes;
		if (dout)
			dout += bytes;
		bitlen -= bits;

		reg = readl(&spi->command);
		reg &= ~SPI_CMD_BIT_LENGTH_MASK;
		reg |= (bits - 1);
		writel(reg, &spi->command);

		/* Write data to FIFO and initiate transfer */
		writel(tmpdout, &spi->tx_fifo);
		writel((reg |= SPI_CMD_GO), &spi->command);

		/*
		 * Wait for SPI transmit FIFO to empty, or to time out.
		 * The RX FIFO status will be read and cleared last
		 */
		for (tm = 0, isRead = 0; tm < SPI_TIMEOUT; ++tm) {
			status = readl(&spi->status);

			while (status & SPI_STAT_BSY) {
				status = readl(&spi->status);

				tm++;
				if (tm > SPI_TIMEOUT) {
					tm = 0;
					break;
				}
			}

			while (!(status & SPI_STAT_RDY)) {
				status = readl(&spi->status);

				tm++;
				if (tm > SPI_TIMEOUT) {
					tm = 0;
					break;
				}
			}

			if (!(status & SPI_STAT_RXF_EMPTY)) {
				tmpdin = readl(&spi->rx_fifo);
				isRead = 1;
				status = readl(&spi->status);

				/* swap bytes read in */
				if (din != NULL) {
					for (i = bytes - 1; i >= 0; --i) {
						((u8 *)din)[i] =
							(tmpdin & 0xff);
						tmpdin >>= 8;
					}
					din += bytes;
				}
			}

			/* We can exit when we've had both RX and TX activity */
			status = readl(&spi->status);
			if (isRead && (status & SPI_STAT_TXF_EMPTY))
				break;
		}

		if (tm >= SPI_TIMEOUT)
			ret = -1;

		status = readl(&spi->status);
		writel(status, &spi->status);	/* ACK RDY, etc. bits */
	}

	if (flags & SPI_XFER_END)
		spi_cs_deactivate(slave);

	debug("spi_xfer: transfer ended. Value=%08x, status = %08x\n",
	 tmpdin, status);

	if (ret == -1)
		printf("spi_xfer: timeout during SPI transfer, tm = %d\n", tm);

	return ret;
}

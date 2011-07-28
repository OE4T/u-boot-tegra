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
#include "tegra_slink.h"

/* NOTE: This is currently hard-coded for SPI4, aka SBC4 on T30 */

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	/* Tegra SLINK - only 1 device ('bus/cs') */
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
	 * Currently, Tegra SLINK uses mode 0 & a 24MHz clock.
	 * Use 'mode' and 'max_hz' to change that here, if needed.
	 */

	return slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	free(slave);
}

void spi_init(void)
{
	struct slink_tegra *spi = (struct slink_tegra *)TEGRA_SLINK4_BASE;
	u32 reg;
	debug("spi_init entry\n");

	/* Change SPI clock to 6MHz (for DEBUG!), PLLP_OUT0 source */
	clock_start_periph_pll(PERIPH_ID_SBC4, CLOCK_ID_PERIPH, (CLK_24M/4));

	/* Clear stale status here */
	reg = SLINK_STAT_RDY | SLINK_STAT_RXF_FLUSH | SLINK_STAT_TXF_FLUSH | \
		SLINK_STAT_RXF_UNR | SLINK_STAT_TXF_OVF;
	writel(reg, &spi->status);
	debug("spi_init: STATUS = %08x\n", readl(&spi->status));

	/* Set master mode */
	reg = readl(&spi->command);
	writel(reg | SLINK_CMD_M_S, &spi->command);
	debug("spi_init: COMMAND = %08x\n", readl(&spi->command));

	/*
	 * Use sw-controlled CS, so we can clock in data after ReadID, etc.
	 */

	reg = readl(&spi->command);
	writel(reg | SLINK_CMD_CS_SOFT, &spi->command);
	debug("spi_init: COMMAND = %08x\n", readl(&spi->command));

	/*
	 * Pinmuxs on Tegra3 are done globally @ init, no need to do it here.
	 */
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
}

void spi_cs_activate(struct spi_slave *slave)
{
	struct slink_tegra *spi = (struct slink_tegra *)TEGRA_SLINK4_BASE;
	u32 val;

	/* CS is negated on Tegra, so drive a 1 to get a 0 */
	val = readl(&spi->command);
	writel(val | SLINK_CMD_CS_VAL, &spi->command);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	struct slink_tegra *spi = (struct slink_tegra *)TEGRA_SLINK4_BASE;
	u32 val;

	/* CS is negated on Tegra, so drive a 0 to get a 1 */
	val = readl(&spi->command);
	writel(val & ~SLINK_CMD_CS_VAL, &spi->command);
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
	struct slink_tegra *spi = (struct slink_tegra *)TEGRA_SLINK4_BASE;
	unsigned int status, status2;
	int num_bytes = (bitlen + 7) / 8;
	int i, ret, tm, bytes, bits, isRead = 0;
	u32 reg, tmpdout, tmpdin = 0;
	debug("spi_xfer entry\n");

	ret = tm = 0;

	status = readl(&spi->status);
	writel(status, &spi->status);	/* Clear all SPI events via R/W */
	status2 = readl(&spi->status2);
	writel(status2, &spi->status2);	/* Clear all STATUS2 events via R/W */

	debug("spi_xfer entry: STATUS = %08x\n", status);
	debug("spi_xfer entry: STATUS2 = %08x\n", status2);

	debug("spi_xfer entry: COMMAND = %08x\n", readl(&spi->command));

	reg = readl(&spi->command2);
	writel((reg |= (SLINK_CMD2_TXEN | SLINK_CMD2_RXEN)), &spi->command2);
	writel((reg |= SLINK_CMD2_SS_EN), &spi->command2);
	debug("spi_xfer: COMMAND2 = %08x\n", readl(&spi->command2));

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
		debug("spi_xfer: TmpDout = %08X\n", tmpdout);

		num_bytes -= bytes;
		if (dout)
			dout += bytes;
		bitlen -= bits;

		reg = readl(&spi->command);
		reg &= ~SLINK_CMD_BIT_LENGTH_MASK;
		reg |= (bits - 1);
		writel(reg, &spi->command);
		debug("spi_xfer: bitlength = %d, COMMAND = %08x\n",
			bits, readl(&spi->command));

		/* Write data to FIFO and initiate transfer */
		writel(tmpdout, &spi->tx_fifo);
		debug("spi_xfer: wrote TX FIFO, address %08X\n", &spi->tx_fifo);
		writel((reg |= SLINK_CMD_GO), &spi->command);
		debug("spi_xfer: Sent GO command\n");

		/*
		 * Wait for SPI transmit FIFO to empty, or to time out.
		 * The RX FIFO status will be read and cleared last
		 */
		for (tm = 0, isRead = 0; tm < SLINK_TIMEOUT; ++tm) {
			status = readl(&spi->status);

			while (status & SLINK_STAT_BSY) {
				status = readl(&spi->status);
				debug("spi_xfer: BUSY: %08x\n", status);

				tm++;
				if (tm > SLINK_TIMEOUT) {
					tm = 0;
					break;
				}
			}

			while (!(status & SLINK_STAT_RDY)) {
				status = readl(&spi->status);
				debug("spi_xfer: !READY: %08x\n", status);

				tm++;
				if (tm > SLINK_TIMEOUT) {
					tm = 0;
					break;
				}
			}

			if (!(status & SLINK_STAT_RXF_EMPTY)) {
				tmpdin = readl(&spi->rx_fifo);
				debug("spi_xfer: RXFIFO data: %08X\n", tmpdin);
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
			if (isRead && (status & SLINK_STAT_TXF_EMPTY))
				break;
		}

		if (tm >= SLINK_TIMEOUT)
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

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
#include <asm/io.h>
#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/tegra2_spi.h>

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	/* Tegra2 SPI-Flash - only 1 device ('bus/cs') */
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
	 * Currently, Tegra2 SFLASH uses mode 0 & a 24MHz clock.
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
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	struct pmux_tri_ctlr *pmt = (struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	struct spi_tegra *spi = (struct spi_tegra *)TEGRA2_SPI_BASE;
	u32 reg;

	/* Enable UART via GPIO_PI3 so serial console works */
	gpio_direction_output(UART_DISABLE_GPIO, 0);

	/*
	 * SPI reset/clocks init - reset SPI, set clocks, release from reset
	 */
	reset_set_enable(PERIPH_ID_SPI1, 1);
	clock_enable(PERIPH_ID_SPI1);

	/* Change SPI clock to 24MHz, PLLP_OUT0 source */
	reg = readl(&clkrst->crc_clk_src_spi1);
	reg &= 0x3FFFFF00;		/* src = PLLP_OUT0 */
	reg |= ((9-1) << 1);		/* div = 9 in 7.1 format */
	writel(reg, &clkrst->crc_clk_src_spi1);
	debug("spi_init: ClkSrc = %08x\n", reg);

	/* wait for 2us */
	udelay(2);

	reset_set_enable(PERIPH_ID_SPI1, 0);

	/* Clear stale status here */

	reg = SPI_STAT_RDY | SPI_STAT_RXF_FLUSH | SPI_STAT_TXF_FLUSH | \
		SPI_STAT_RXF_UNR | SPI_STAT_TXF_OVF;
	writel(reg, &spi->status);
	debug("spi_init: STATUS = %08x\n", readl(&spi->status));

	/*
	 * Use sw-controlled CS, so we can clock in data after ReadID, etc.
	 */

	reg = readl(&spi->command);
	writel((reg | SPI_CMD_CS_SOFT), &spi->command);
	debug("spi_init: COMMAND = %08x\n", readl(&spi->command));

	/*
	 * SPI pins on Tegra2 are muxed - change pinmux last due to UART issue
	 */
	reg = readl(&pmt->pmt_ctl_c);
	reg |= GMD_SEL_SFLASH;		/* GMD_SEL [31:30] = (3) SFLASH */
	writel(reg, &pmt->pmt_ctl_c);
	debug("spi_init: PinMuxRegC = %08x\n", reg);

	pinmux_tristate_disable(PIN_LSPI);

	/*
	 * NOTE:
	 * Don't set PinMux bits 3:2 to SPI here or subsequent UART data
	 * won't go out! It'll be correctly set in the actual SPI driver
	 * before/after any transactions (cs_activate/_deactivate).
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
	struct pmux_tri_ctlr *pmt = (struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	struct spi_tegra *spi = (struct spi_tegra *)TEGRA2_SPI_BASE;
	u32 val;

	/*
	 * Delay here to clean up comms - spurious chars seen around SPI xfers.
	 * Fine-tune later.
	 */
	udelay(1000);

	/* We need to dynamically change the pinmux, shared w/UART RXD/CTS */
	val = readl(&pmt->pmt_ctl_b);
	val |= GMC_SEL_SFLASH;		/* GMC_SEL [3:2] = (3) SFLASH */
	writel(val, &pmt->pmt_ctl_b);

	/*
	 * On Seaboard, MOSI/MISO are shared w/UART.
	 * Use GPIO I3 (UART_DISABLE) to tristate UART during SPI activity.
	 * Enable UART later (cs_deactivate) so we can use it for U-Boot comms.
	 */
	gpio_direction_output(UART_DISABLE_GPIO, 1);

	/* CS is negated on Tegra, so drive a 1 to get a 0 */
	val = readl(&spi->command);
	writel((val |= SPI_CMD_CS_VAL), &spi->command);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	struct pmux_tri_ctlr *pmt = (struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	struct spi_tegra *spi = (struct spi_tegra *)TEGRA2_SPI_BASE;
	u32 val;

	/*
	 * Delay here to clean up comms - spurious chars seen around SPI xfers.
	 * Fine-tune later.
	 */
	udelay(1000);

	/* We need to dynamically change the pinmux, shared w/UART RXD/CTS */
	val = readl(&pmt->pmt_ctl_b);
	val &= ~GMC_SEL_SFLASH;		/* GMC_SEL [3:2] = (0) UARTD */
	writel(val, &pmt->pmt_ctl_b);

	/*
	 * On Seaboard, MOSI/MISO are shared w/UART.
	 * GPIO I3 (UART_DISABLE) is used to tristate UART in cs_activate.
	 * Enable UART here by setting that GPIO to 0 so we can do U-Boot comms.
	 */
	gpio_direction_output(UART_DISABLE_GPIO, 0);

	/* CS is negated on Tegra, so drive a 0 to get a 1 */
	val = readl(&spi->command);
	writel((val &= ~SPI_CMD_CS_VAL), &spi->command);
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
	struct spi_tegra *spi = (struct spi_tegra *)TEGRA2_SPI_BASE;
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

	reg = readl(&spi->command);
	writel((reg |= (SPI_CMD_TXEN | SPI_CMD_RXEN)), &spi->command);
	debug("spi_xfer: COMMAND = %08x\n", readl(&spi->command));

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
		dout     += bytes;
		bitlen   -= bits;

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
					for (i = 0; i < bytes; ++i) {
						((u8 *)din)[i] = (tmpdin >> 24);
						tmpdin <<= 8;
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

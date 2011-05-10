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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#if defined(CONFIG_SPI_UART_SWITCH)

#ifndef __ASSEMBLY__
#include <ns16550.h>
/*
 * Signal that we are about to use the UART. This unfortunate hack is
 * required by Seaboard, which cannot use its console and SPI at the same
 * time! If the board file provides this, the board config will declare it.
 * Let this be a lesson for others.
 */
void uart_enable(NS16550_t regs);

/*
 * Signal that we are about the use the SPI bus.
 */
void spi_enable(void);
#endif

#else /* not CONFIG_SPI_UART_SWITCH */

static inline void uart_enable(NS16550_t regs) {}
static inline void spi_enable(void) {}

#endif

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

#ifndef __CONFIG_H
#define __CONFIG_H

#include <asm/sizes.h>
#include "tegra2-common.h"

/* High-level configuration options */
#define TEGRA2_SYSMEM		"mem=384M@0M nvmem=128M@384M mem=512M@512M"
#define V_PROMPT		"Tegra2 (SeaBoard) # "
#define CONFIG_TEGRA2_BOARD_STRING	"NVIDIA Seaboard"

/* Board-specific serial config */
#define CONFIG_SERIAL_MULTI
#define CONFIG_TEGRA2_ENABLE_UARTD
#define CONFIG_SYS_NS16550_COM1		NV_PA_APB_UARTD_BASE
#define CONFIG_NS16550_BUFFER_READS

/* Seaboard SPI activity corrupts the first UART */
#define CONFIG_SPI_CORRUPTS_UART	NV_PA_APB_UARTD_BASE
#define CONFIG_SPI_CORRUPTS_UART_NR	0

#define CONFIG_MACH_TYPE		MACH_TYPE_SEABOARD
#define CONFIG_SYS_BOARD_ODMDATA	0x300d8011 /* lp1, 1GB */

#define CONFIG_BOARD_EARLY_INIT_F

/* GPIO */
#define CONFIG_TEGRA2_GPIO
#define CONFIG_CMD_TEGRA2_GPIO_INFO

/* SPI */
#define CONFIG_TEGRA2_SPI
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_CMD_SPI
#define CONFIG_CMD_SF

/* Environment in SPI */
#define CONFIG_ENV_IS_IN_SPI_FLASH

#define CONFIG_ENV_SECT_SIZE    CONFIG_ENV_SIZE
#define CONFIG_ENV_OFFSET       (SZ_4M - CONFIG_ENV_SECT_SIZE)

/* To select the order in which U-Boot sees USB ports */
#define CONFIG_TEGRA2_USB0	NV_PA_USB3_BASE
#define CONFIG_TEGRA2_USB1	NV_PA_USB1_BASE
#define CONFIG_TEGRA2_USB2	0
#define CONFIG_TEGRA2_USB3	0

/* Put USB1 in host mode */
#define CONFIG_TEGRA2_USB1_HOST

#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_EXTRA_ENV_SETTINGS_COMMON \
	"board=seaboard\0" \

/* On Seaboard: GPIO_PI3 = Port I = 8, bit = 3 */
#define UART_DISABLE_GPIO	GPIO_PI3

#if defined(CONFIG_SPI_CORRUPTS_UART) && !defined(__ASSEMBLY__)

/* position of the UART/SPI select switch */
enum spi_uart_switch {
	SWITCH_UNKNOWN,
	SWITCH_SPI,
	SWITCH_UART
};

/* Move the SPI/UART switch - we can only use one at a time! */
void seaboard_switch_spi_uart(enum spi_uart_switch new_pos);

static inline void uart_enable(void) { seaboard_switch_spi_uart(SWITCH_UART); }
static inline void spi_enable(void) { seaboard_switch_spi_uart(SWITCH_SPI); }

#endif


#endif /* __CONFIG_H */

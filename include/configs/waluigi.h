/*
 *  (C) Copyright 2010,2011
 *  NVIDIA Corporation <www.nvidia.com>
 * Portions Copyright (c) 2011 The Chromium OS Authors
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

/* High-level configuration options */
#define TEGRA3_SYSMEM		"mem=1023M@2048M vmalloc=128M"
#define V_PROMPT		"Tegra3 # "

#define CONFIG_TEGRA2_LP0

#define CONFIG_TEGRA3_WALUIGI
#define CONFIG_SYS_SKIP_ARM_RELOCATION
#define CONFIG_SYS_PLLP_BASE_IS_408MHZ

#define z_pause()	\
{			\
	while (1)	\
		;	\
}

#define CONFIG_EXTRA_BOOTARGS \
	"panel=lvds " \
	"tegraid=30.1.2.0.0 " \
	"debug_uartport=lsport " \
	"pmuboard=0x0c08:0x0a01:0x04:0x41:0x08\0" \

#include "tegra3-common.h"

#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_EXTRA_ENV_SETTINGS_COMMON \
	"board=waluigi\0" \

#define CONFIG_DEFAULT_DEVICE_TREE "tegra3-waluigi"

#define CONFIG_SERIAL_TAG		/* enable passing serial# (board id) */
#define CONFIG_TEGRA_SERIAL_HIGH	0x0c5b0a00
#define CONFIG_TEGRA_SERIAL_LOW		0x02430300

#ifndef CONFIG_OF_CONTROL
/* Things in here are defined by the device tree now. Let it grow! */

#define CONFIG_TEGRA2_ENABLE_UARTD
#define CONFIG_SYS_NS16550_COM1		NV_PA_APB_UARTD_BASE

/* To select the order in which U-Boot sees USB ports */
#define CONFIG_TEGRA2_USB0      NV_PA_USB3_BASE
#define CONFIG_TEGRA2_USB1      NV_PA_USB1_BASE
#define CONFIG_TEGRA2_USB2      0
#define CONFIG_TEGRA2_USB3      0

/* Put USB1 in host mode */
#define CONFIG_TEGRA2_USB1_HOST
#define CONFIG_MACH_TYPE	MACH_TYPE_WALUIGI

#endif /* CONFIG_OF_CONTROL not defined ^^^^^^^ */

#define CONFIG_CONSOLE_MUX
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_STD_DEVICES_SETTINGS	"stdin=serial,tegra-kbc\0" \
					"stdout=serial,lcd\0" \
					"stderr=serial,lcd\0"

/* TBD: Waluigi is supposed to have 2GB, but it hangs w/odmdata = 0x800xxxxx ??? */
#define CONFIG_SYS_BOARD_ODMDATA	0x400D8105	/* 1GB, UARTD, etc */

#define CONFIG_ENV_SECT_SIZE    CONFIG_ENV_SIZE
#define CONFIG_ENV_OFFSET       (SZ_4M - CONFIG_ENV_SECT_SIZE)

#define TEGRA_MMC_DEFAULT_DEVICE	"1"	/* TCW SD-card for now */

/* GPIO */
#define CONFIG_TEGRA2_GPIO
#define CONFIG_CMD_TEGRA2_GPIO_INFO

/* SPI */
#define CONFIG_TEGRA_SPI
#define CONFIG_USE_SLINK	/* Cardhu SPI chip is on SBC4 */
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_CMD_SPI
#define CONFIG_CMD_SF
/* Environment in SPI */
#define CONFIG_ENV_IS_IN_SPI_FLASH

/* I2C */
#define CONFIG_TEGRA2_I2C
#define CONFIG_SYS_I2C_INIT_BOARD
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_MAX_I2C_BUS		5
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_CMD_I2C

/* SD/MMC */
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_TEGRA2_MMC
#define CONFIG_CMD_MMC

#define CONFIG_DOS_PARTITION
#define CONFIG_EFI_PARTITION
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_FAT

#define CONFIG_TEGRA_KEYBOARD
#define CONFIG_KEYBOARD

/*
 *  LCDC configuration
 */
#define CONFIG_LCD
#define CONFIG_VIDEO_TEGRA2

/* TODO: This needs to be configurable at run-time */
#define LCD_BPP	LCD_COLOR16
#define CONFIG_SYS_WHITE_ON_BLACK       /*Console colors*/

#endif /* __CONFIG_H */

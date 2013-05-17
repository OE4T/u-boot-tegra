/*
 *  (C) Copyright 2010-2013
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
#define V_PROMPT	"Dalmore #"

#define CONFIG_TEGRA2_LP0

#define CONFIG_TEGRA11X_DALMORE
#define CONFIG_SYS_PLLP_BASE_IS_408MHZ

#define z_pause()	\
{			\
	while (1)	\
		;	\
}

#define CONFIG_EXTRA_BOOTARGS \
	"tegraid=35.1.1.0.0 " \
	"mem=1980M@2048M vpr=32M@4063M tsec=32M@4031M " \
	"otf_key=0 commchip_id=0 " \
	"console=tty1 " \
	"tegra_fbmem=18554880@0xad013000 " \
	"pmuboard=0x8028:0x81c8:0x31:0x80:0x3c " \
	"displayboard=0x065b:0x03e8:0x02:0x43:0x03 " \
	"display_panel=0 " \
	"audio_codec=rt5640 " \
	"debug_uartport=lsport,3\0" \

#include "tegra11x-common.h"

#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_EXTRA_ENV_SETTINGS_COMMON \
	"board=dalmore\0" \

#define CONFIG_DEFAULT_DEVICE_TREE "t114-dalmore"

#define CONFIG_SERIAL_TAG	/* enable passing serial# (board id) */
//#define TEGRA3_BOOT_TRACE
//#define TEGRA3_BOOT_TRACE_MORE

#define CONFIG_TEGRA_SERIAL_HIGH	0x064b03e8
#define CONFIG_TEGRA_SERIAL_LOW		0x02450300

/* The following are used to retrieve the board id from an eeprom */
#define CONFIG_SERIAL_EEPROM
#define EEPROM_I2C_BUS		1
#define EEPROM_I2C_ADDRESS	0x56
#define EEPROM_SERIAL_OFFSET	0x04
#define NUM_SERIAL_ID_BYTES	8

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
#define CONFIG_MACH_TYPE	MACH_TYPE_CARDHU

#endif /* CONFIG_OF_CONTROL not defined ^^^^^^^ */

#define CONFIG_CONSOLE_MUX
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_STD_DEVICES_SETTINGS	"stdin=serial,tegra-kbc\0" \
					"stdout=serial,lcd\0" \
					"stderr=serial,lcd\0"

#define CONFIG_ENV_SECT_SIZE		CONFIG_ENV_SIZE
#ifdef CONFIG_L4T
#define CONFIG_ENV_OFFSET		SZ_16M
#define CONFIG_FIND_MMC_ENV_OFFSET
#else /* CONFIG_L4T */
#define CONFIG_ENV_OFFSET		(SZ_4M - CONFIG_ENV_SECT_SIZE)
#endif /* CONFIG_L4T */

#define TEGRA_MMC_DEFAULT_DEVICE	"0"

/* GPIO */
#define CONFIG_TEGRA2_GPIO
#define CONFIG_CMD_TEGRA2_GPIO_INFO

/* SPI */
#define CONFIG_TEGRA_SPI
#define CONFIG_USE_SLINK	/* Cardhu SPI chip is on SBC4 */
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_ATMEL
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_CMD_SPI
#define CONFIG_CMD_SF
#ifndef CONFIG_L4T
/* Environment in SPI */
#define CONFIG_ENV_IS_IN_SPI_FLASH
#endif	/* !CONFIG_L4T */

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
#ifdef CONFIG_L4T
/* Environment in MMC partition */
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV	0
#endif /* CONFIG_L4T */

#define CONFIG_DOS_PARTITION
#define CONFIG_EFI_PARTITION
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_FAT

#if 0 /* tcw - disable kbd for now */
#define CONFIG_TEGRA_KEYBOARD
#define CONFIG_KEYBOARD
#endif /* tcw if 0 kbd */

#if 0 /* tcw - disable LCD for now */
/*
 *  LCDC configuration
 */
#define CONFIG_LCD
#define CONFIG_VIDEO_TEGRA2
/* TODO: This needs to be configurable at run-time */
#define LCD_BPP	LCD_COLOR16
#define CONFIG_SYS_WHITE_ON_BLACK       /*Console colors*/
#endif /* tcw if 0 LCD */

#endif /* __CONFIG_H */

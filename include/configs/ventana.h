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

#define CONFIG_TEGRA2_LP0

/* High-level configuration options */
#define TEGRA2_SYSMEM		"mem=1024M@0M vmalloc=128M"
#define V_PROMPT		"Tegra2 (Ventana) # "

#define CONFIG_EXTRA_BOOTARGS \
	"usbcore.old_scheme_first=1 " \
	"tegraid=20.1.2.0.0 " \
	"no_console_suspend=1 " \
	"debug_uartport=lsport,3\0" \

#include "tegra2-common.h"

#define CONFIG_SERIAL_TAG	/* enable passing serial# (board id) */
/* ventana a03 board */
#define CONFIG_BOARD_ID_HIGH_A03	0x024b0a00
#define CONFIG_BOARD_ID_LOW_A03 	0x03420500

#define CONFIG_TEGRA_SERIAL_HIGH	CONFIG_BOARD_ID_HIGH_A03
#define CONFIG_TEGRA_SERIAL_LOW 	CONFIG_BOARD_ID_LOW_A03

/* The following are used to retrieve the board id from an eeprom */
#define CONFIG_SERIAL_EEPROM
#define EEPROM_I2C_BUS		0
#define EEPROM_I2C_ADDRESS	0x50
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

#endif /* CONFIG_OF_CONTROL not defined ^^^^^^^ */

#define CONFIG_CONSOLE_MUX
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_STD_DEVICES_SETTINGS	"stdin=serial,tegra-kbc\0" \
					"stdout=serial,lcd\0" \
					"stderr=serial,lcd\0"

#ifdef CONFIG_L4T
#define CONFIG_ENV_SECT_SIZE    CONFIG_ENV_SIZE
#define CONFIG_ENV_OFFSET	SZ_16M
#define CONFIG_FIND_MMC_ENV_OFFSET
#endif /* CONFIG_L4T */

/* GPIO */
#define CONFIG_TEGRA2_GPIO
#define CONFIG_CMD_TEGRA2_GPIO_INFO

/* I2C */
#define CONFIG_TEGRA2_I2C
#define CONFIG_SYS_I2C_INIT_BOARD
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_MAX_I2C_BUS		4
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_CMD_I2C

/* pin-mux settings for ventana */
#define CONFIG_I2CP_PIN_MUX		1
#define CONFIG_I2C1_PIN_MUX		1
#define CONFIG_I2C2_PIN_MUX		2
#define CONFIG_I2C3_PIN_MUX		1

#define CONFIG_ENABLE_EMC_INIT		/* voltage control over I2C */

/* SD/MMC */
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_TEGRA2_MMC
#define CONFIG_CMD_MMC
#ifdef CONFIG_L4T
/* Environment in MMC partition */
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#endif /* CONFIG_L4T */

#define CONFIG_DOS_PARTITION
#define CONFIG_EFI_PARTITION
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_FAT

#define TEGRA2_MMC_DEFAULT_DEVICE	"0"

#ifndef CONFIG_L4T
/* Environment not stored */
#define CONFIG_ENV_IS_NOWHERE
#endif /* CONFIG_L4T */

/*
 *  LCDC configuration
 */
#define CONFIG_LCD
#define CONFIG_VIDEO_TEGRA2

/* TODO: This needs to be configurable at run-time */
#define LCD_BPP             LCD_COLOR32
#define CONFIG_SYS_WHITE_ON_BLACK       /*Console colors*/

#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_EXTRA_ENV_SETTINGS_COMMON \
	"board=ventana\0" \

#define CONFIG_DEFAULT_DEVICE_TREE "tegra2-ventana"

#endif /* __CONFIG_H */

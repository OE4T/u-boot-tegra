/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) Copyright 2013-2014
 * NVIDIA Corporation <www.nvidia.com>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#include "tegra124-common.h"

#define CONFIG_SERIAL_TAG
#define CONFIG_TEGRA_SERIAL_HIGH	0x01770000
#define CONFIG_TEGRA_SERIAL_LOW		0x034200FF

/* The following are used to retrieve the board id from an eeprom */
#define CONFIG_SERIAL_EEPROM
#define EEPROM_I2C_BUS         1
#define EEPROM_I2C_ADDRESS     0x56
#define EEPROM_SERIAL_OFFSET   0x04
#define NUM_SERIAL_ID_BYTES    8

/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"NVIDIA Jetson TK1"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTD
#define CONFIG_SYS_NS16550_COM1		NV_PA_APB_UARTD_BASE

/* I2C */
#define CONFIG_SYS_I2C_TEGRA

/* Environment in eMMC, at the end of 2nd "boot sector" */
#define CONFIG_ENV_OFFSET		(-CONFIG_ENV_SIZE)
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		2

/* SPI */
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_SF_DEFAULT_SPEED		24000000
#define CONFIG_SPI_FLASH_SIZE		(4 << 20)

/* USB Host support */
#define CONFIG_USB_EHCI_TEGRA

/* USB networking support */

/* PCI host support */

/* General networking support */

#include "tegra-common-usb-gadget.h"
#include "tegra-common-post.h"

/* Reserve top 1M for secure RAM */
#define CONFIG_ARMV7_SECURE_BASE		0xfff00000
#define CONFIG_ARMV7_SECURE_RESERVE_SIZE	0x00100000

#endif /* __CONFIG_H */

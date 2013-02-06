/*
 * Copyright (c) 2013 The Chromium OS Authors.
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

/* Tegra3 pin multiplexing functions */

#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/tegra.h>
#include <asm/arch/pinmux.h>
#include <common.h>

struct tegra_pingroup_desc {
	const char *name;
	enum pmux_func funcs[4];
	enum pmux_func func_safe;
	enum pmux_vddio vddio;
	enum pmux_pin_io io;
};

#define PMUX_MUXCTL_SHIFT	0
#define PMUX_PULL_SHIFT		2
#define PMUX_TRISTATE_SHIFT	4
#define PMUX_TRISTATE_MASK	(1 << PMUX_TRISTATE_SHIFT)
#define PMUX_IO_SHIFT		5
#define PMUX_OD_SHIFT		6
#define PMUX_LOCK_SHIFT		7
#define PMUX_IO_RESET_SHIFT	8
#define PMUX_RCV_SEL_SHIFT	9

/* Convenient macro for defining pin group properties */
#define PIN(pg_name, vdd, f0, f1, f2, f3, f_safe, iod)	\
	{						\
		.vddio = PMUX_VDDIO_ ## vdd,		\
		.funcs = {				\
			PMUX_FUNC_ ## f0,		\
			PMUX_FUNC_ ## f1,		\
			PMUX_FUNC_ ## f2,		\
			PMUX_FUNC_ ## f3,		\
		},					\
		.func_safe = PMUX_FUNC_ ## f_safe,	\
		.io = PMUX_PIN_ ## iod,			\
	}

/* A pin group number which is not used */
#define PIN_RESERVED \
	PIN(NONE, NONE, INVALID, INVALID, INVALID, INVALID, INVALID, NONE)

const struct tegra_pingroup_desc tegra_soc_pingroups[PINGRP_COUNT] = {
	/*  NAME	VDD	f0	f1	f2	f3	fSafe	io */
	PIN(ULPI_DATA0,	BB,	    SPI3,	HSI,	    UARTA,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_DATA1,	BB,	    SPI3,	HSI,	    UARTA,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_DATA2,	BB,	    SPI3,	HSI,	    UARTA,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_DATA3,	BB,	    SPI3,	HSI,	    UARTA,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_DATA4,	BB,	    SPI2,	HSI,	    UARTA,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_DATA5,	BB,	    SPI2,	HSI,	    UARTA,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_DATA6,	BB,	    SPI2,	HSI,	    UARTA,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_DATA7,	BB,	    SPI2,	HSI,	    UARTA,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_CLK,	BB,	    SPI1,	SPI5,	    UARTD,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_DIR,	BB,	    SPI1,	SPI5,	    UARTD,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_NXT,	BB,	    SPI1,	SPI5,	    UARTD,	ULPI,	     RSVD,	INPUT),
	PIN(ULPI_STP,	BB,	    SPI1,	SPI5,	    UARTD,	ULPI,	     RSVD,	INPUT),
	PIN(DAP3_FS,	BB,	    I2S2,	SPI5,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP3_DIN,	BB,	    I2S2,	SPI5,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP3_DOUT,	BB,	    I2S2,	SPI5,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP3_SCLK,	BB,	    I2S2,	SPI5,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PV0,	BB,	    USB,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PV1,	BB,	    RSVD0,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC1_CLK,	SDMMC1,     SDMMC1,	CLK12,	    RSVD2,	RSVD3,       RSVD,	INPUT),
	PIN(SDMMC1_CMD,	SDMMC1,     SDMMC1,	SPDIF,	    SPI4,	UARTA,       RSVD,	INPUT),
	PIN(SDMMC1_DAT3,	SDMMC1,     SDMMC1,	SPDIF,	    SPI4,	UARTA,       RSVD,	INPUT),
	PIN(SDMMC1_DAT2,	SDMMC1,     SDMMC1,	PWM0,	    SPI4,	UARTA,       RSVD,	INPUT),
	PIN(SDMMC1_DAT1,	SDMMC1,     SDMMC1,	PWM1,	    SPI4,	UARTA,       RSVD,	INPUT),
	PIN(SDMMC1_DAT0,	SDMMC1,     SDMMC1,	RSVD1,	    SPI4,	UARTA,       RSVD,	INPUT),
	PIN_RESERVED,	/* Reserved by t114: 0x3060 - 0x3064 */
	PIN_RESERVED,
	PIN(CLK2_OUT,	SDMMC1,     EXTPERIPH2,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(CLK2_REQ,	SDMMC1,     DAP,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN_RESERVED,	/* Reserved by t114: 0x3070 - 0x310c */
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN(HDMI_INT,	LCD,	    RSVD0,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DDC_SCL,	LCD,	    I2C4,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DDC_SDA,	LCD,	    I2C4,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN_RESERVED,	/* Reserved by t114: 0x311c - 0x3160 */
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN(UART2_RXD,	UART,	    IRDA,	SPDIF,	    UARTA,	SPI4,	     RSVD,	INPUT),
	PIN(UART2_TXD,	UART,	    IRDA,	SPDIF,	    UARTA,	SPI4,	     RSVD,	INPUT),
	PIN(UART2_RTS_N,	UART,	    UARTA,	UARTB,	    RSVD2,	SPI4,	     RSVD,	INPUT),
	PIN(UART2_CTS_N,	UART,	    UARTA,	UARTB,	    RSVD2,	SPI4,	     RSVD,	INPUT),
	PIN(UART3_TXD,	UART,	    UARTC,	RSVD1,	    RSVD2,	SPI4,	     RSVD,	INPUT),
	PIN(UART3_RXD,	UART,	    UARTC,	RSVD1,	    RSVD2,	SPI4,	     RSVD,	INPUT),
	PIN(UART3_CTS_N,	UART,	    UARTC,	SDMMC1,	    DTV,	SPI4,	     RSVD,	INPUT),
	PIN(UART3_RTS_N,	UART,	    UARTC,	PWM0,	    DTV,	DISPLAYA,    RSVD,	INPUT),
	PIN(GPIO_PU0,	UART,	    RSVD0,	UARTA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PU1,	UART,	    RSVD0,	UARTA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PU2,	UART,	    RSVD0,	UARTA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PU3,	UART,	    PWM0,	UARTA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PU4,	UART,	    PWM1,	UARTA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PU5,	UART,	    PWM2,	UARTA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PU6,	UART,	    PWM3,	UARTA,	    USB,	RSVD3,	     RSVD,	INPUT),
	PIN(GEN1_I2C_SDA,	UART,	    I2C1,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GEN1_I2C_SCL,	UART,	    I2C1,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP4_FS,	UART,	    I2S3,	RSVD1,	    DTV,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP4_DIN,	UART,	    I2S3,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP4_DOUT,	UART,	    I2S3,	RSVD1,	    DTV,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP4_SCLK,	UART,	    I2S3,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(CLK3_OUT,	UART,	    EXTPERIPH3,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(CLK3_REQ,	UART,	    DEV3,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GMI_WP_N,	GMI,	    RSVD0,	NAND,	    GMI,	GMI_ALT,     RSVD,	INPUT),
	PIN(GMI_IORDY,	GMI,	    SDMMC2,	RSVD1,	    GMI,	RSVD,	     RSVD,	INPUT),
	PIN(GMI_WAIT,	GMI,	    SPI4,	NAND,	    GMI,	DTV,	     RSVD,	INPUT),
	PIN(GMI_ADV_N,	GMI,	    RSVD0,	NAND,	    GMI,	RSVD,	     RSVD,	INPUT),
	PIN(GMI_CLK,	GMI,	    SDMMC2,	NAND,	    GMI,	RSVD,	     RSVD,	INPUT),
	PIN(GMI_CS0_N,	GMI,	    RSVD0,	NAND,	    GMI,	USB,	     RSVD,	INPUT),
	PIN(GMI_CS1_N,	GMI,	    RSVD0,	NAND,	    GMI,	SOC,	     RSVD,	INPUT),
	PIN(GMI_CS2_N,	GMI,	    SDMMC2,	NAND,	    GMI,	RSVD,	     RSVD,	INPUT),
	PIN(GMI_CS3_N,	GMI,	    SDMMC2,	NAND,	    GMI,	GMI_ALT,     RSVD,	INPUT),
	PIN(GMI_CS4_N,	GMI,	    USB,	NAND,	    GMI,	RSVD,	     RSVD,	INPUT),
	PIN(GMI_CS6_N,	GMI,	    NAND,	NAND_ALT,   GMI,	SPI4,	     RSVD,	INPUT),
	PIN(GMI_CS7_N,	GMI,	    NAND,	NAND_ALT,   GMI,	SDMMC2,	     RSVD,	INPUT),
	PIN(GMI_AD0,	GMI,	    RSVD0,	NAND,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(GMI_AD1,	GMI,	    RSVD0,	NAND,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(GMI_AD2,	GMI,	    RSVD0,	NAND,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(GMI_AD3,	GMI,	    RSVD0,	NAND,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(GMI_AD4,	GMI,	    RSVD0,	NAND,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(GMI_AD5,	GMI,	    RSVD0,	NAND,	    GMI,	SPI4,	     RSVD,	INPUT),
	PIN(GMI_AD6,	GMI,	    RSVD0,	NAND,	    GMI,	SPI4,	     RSVD,	INPUT),
	PIN(GMI_AD7,	GMI,	    RSVD0,	NAND,	    GMI,	SPI4,	     RSVD,	INPUT),
	PIN(GMI_AD8,	GMI,	    PWM0,	NAND,	    GMI,	DTV,	     RSVD,	INPUT),
	PIN(GMI_AD9,	GMI,	    PWM1,	NAND,	    GMI,	CLDVFS,	     RSVD,	INPUT),
	PIN(GMI_AD10,	GMI,	    PWM2,	NAND,	    GMI,	CLDVFS,	     RSVD,	INPUT),
	PIN(GMI_AD11,	GMI,	    PWM3,	NAND,	    GMI,	USB,	     RSVD,	INPUT),
	PIN(GMI_AD12,	GMI,	    SDMMC2,	NAND,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(GMI_AD13,	GMI,	    SDMMC2,	NAND,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(GMI_AD14,	GMI,	    SDMMC2,	NAND,	    GMI,	DTV,	     RSVD,	INPUT),
	PIN(GMI_AD15,	GMI,	    SDMMC2,	NAND,	    GMI,	DTV,	     RSVD,	INPUT),
	PIN(GMI_A16,	GMI,	    UARTD,	RSVD,	    GMI,	GMI_ALT,     RSVD,	INPUT),
	PIN(GMI_A17,	GMI,	    UARTD,	RSVD1,	    GMI,	RSVD,	     RSVD,	INPUT),
	PIN(GMI_A18,	GMI,	    UARTD,	RSVD1,	    GMI,	RSVD,	     RSVD,	INPUT),
	PIN(GMI_A19,	GMI,	    UARTD,	SPI4,	    GMI,	RSVD,	     RSVD,	INPUT),
	PIN(GMI_WR_N,	GMI,	    RSVD0,	NAND,	    GMI,	SPI4,	     RSVD,	INPUT),
	PIN(GMI_OE_N,	GMI,	    RSVD0,	NAND,	    GMI,	SOC,	     RSVD,	INPUT),
	PIN(GMI_DQS_P,	GMI,	    SDMMC2,	NAND,	    GMI,	RSVD,	     RSVD,	INPUT),
	PIN(GMI_RST_N,	GMI,	    NAND,	NAND_ALT,   GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(GEN2_I2C_SCL,	GMI,	    I2C2,	RSVD1,      GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(GEN2_I2C_SDA,	GMI,	    I2C2,	RSVD1,      GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_CLK,		SDMMC4,     SDMMC4,	RSVD1,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_CMD,		SDMMC4,     SDMMC4,	RSVD1,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_DAT0,	SDMMC4,     SDMMC4,	SPI3,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_DAT1,	SDMMC4,     SDMMC4,	SPI3,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_DAT2,	SDMMC4,     SDMMC4,	SPI3,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_DAT3,	SDMMC4,     SDMMC4,	SPI3,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_DAT4,	SDMMC4,     SDMMC4,	SPI3,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_DAT5,	SDMMC4,     SDMMC4,	SPI3,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_DAT6,	SDMMC4,     SDMMC4,	SPI3,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC4_DAT7,	SDMMC4,     SDMMC4,	RSVD1,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN_RESERVED,	/* Reserved by t114: 0x3280 */
	PIN(CAM_MCLK,	CAM,	    VI,		VI_ALT1,    VI_ALT3,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PCC1,	CAM,	    I2S4,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PBB0,	CAM,	    I2S4,	VI,	    VI_ALT1,	VI_ALT3,     RSVD,	INPUT),
	PIN(CAM_I2C_SCL,	CAM,	    VGP1,	I2C3,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(CAM_I2C_SDA,	CAM,	    VGP2,	I2C3,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PBB3,	CAM,	    VGP3,	RSVD1,	RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PBB4,	CAM,	    VGP4,	RSVD1,	RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PBB5,	CAM,	    VGP5,	RSVD1, 	RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PBB6,	CAM,	    VGP6,	RSVD1,	RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PBB7,	CAM,	    I2S4,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_PCC2,	CAM,	    I2S4,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(JTAG_RTCK,	SYS,	    RTCK,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(PWR_I2C_SCL,	SYS,	    I2CPWR,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(PWR_I2C_SDA,	SYS,	    I2CPWR,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(KB_ROW0,	SYS,	    KBC,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(KB_ROW1,	SYS,	    KBC,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(KB_ROW2,	SYS,	    KBC,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(KB_ROW3,	SYS,	    KBC,	RSVD1,      RSVD2,	RSVD3,       RSVD,	INPUT),
	PIN(KB_ROW4,	SYS,	    KBC,	RSVD1,      SPI2,	RSVD3,       RSVD,	INPUT),
	PIN(KB_ROW5,	SYS,	    KBC,	RSVD1,      SPI2,	RSVD3,       RSVD,	INPUT),
	PIN(KB_ROW6,	SYS,	    KBC,	RSVD1,      RSVD2,	RSVD3,       RSVD,	INPUT),
	PIN(KB_ROW7,	SYS,	    KBC,	RSVD1,	    CLDVFS,	UARTA,	     RSVD,	INPUT),
	PIN(KB_ROW8,	SYS,	    KBC,	RSVD1,	    CLDVFS,	UARTA,	     RSVD,	INPUT),
	PIN(KB_ROW9,	SYS,	    KBC,	RSVD1,	    RSVD2,	UARTA,	     RSVD,	INPUT),
	PIN(KB_ROW10,	SYS,	    KBC,	RSVD1,	    RSVD2,	UARTA,	     RSVD,	INPUT),
	PIN_RESERVED,	/* Reserved by t114: 0x32e8 - 0x32f8 */
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN(KB_COL0,	SYS,	    KBC,	USB,	    SPI2,	RSVD3,	     RSVD,	INPUT),
	PIN(KB_COL1,	SYS,	    KBC,	RSVD1,	    SPI2,	RSVD3,	     RSVD,	INPUT),
	PIN(KB_COL2,	SYS,	    KBC,	RSVD1,	    SPI2,	RSVD,	     RSVD,	INPUT),
	PIN(KB_COL3,	SYS,	    KBC,	RSVD1,	    PWM2,	UARTA,	     RSVD,	INPUT),
	PIN(KB_COL4,	SYS,	    KBC,	RSVD1,	    SDMMC3,	UARTA,	     RSVD,	INPUT),
	PIN(KB_COL5,	SYS,	    KBC,	RSVD1,	    SDMMC1,	RSVD3,	     RSVD,	INPUT),
	PIN(KB_COL6,	SYS,	    KBC,	RSVD1,	    SPI2,	RSVD3,	     RSVD,	INPUT),
	PIN(KB_COL7,	SYS,	    KBC,	RSVD1,	    SPI2,	RSVD3,	     RSVD,	INPUT),
	PIN(CLK_32K_OUT,	SYS,	    BLINK,	SOC,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(SYS_CLK_REQ,	SYS,	    SYSCLK,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(CORE_PWR_REQ,	SYS,	    PWRON,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(CPU_PWR_REQ,	SYS,	    CPU,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(PWR_INT_N,	SYS,	    PMI,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(CLK_32K_IN,	SYS,	    CLK,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(OWR,	SYS,	    OWR,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP1_FS,	AUDIO,      I2S0,	HDA,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP1_DIN,	AUDIO,      I2S0,	HDA,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP1_DOUT,	AUDIO,      I2S0,	HDA,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP1_SCLK,	AUDIO,      I2S0,	HDA,	    GMI,	RSVD3,	     RSVD,	INPUT),
	PIN(CLK1_REQ,	AUDIO,      DAP,	DAP1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(CLK1_OUT,	AUDIO,      EXTPERIPH1,	DAP2,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(SPDIF_IN,	AUDIO,      SPDIF,	USB,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(SPDIF_OUT,	AUDIO,      SPDIF,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP2_FS,	AUDIO,      I2S1,	HDA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP2_DIN,	AUDIO,      I2S1,	HDA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP2_DOUT,	AUDIO,      I2S1,	HDA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DAP2_SCLK,	AUDIO,      I2S1,	HDA,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DVFS_PWM,	AUDIO,      SPI6,	CLDVFS,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_X1_AUD,	AUDIO,      SPI6,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_X3_AUD,	AUDIO,      SPI6,	SPI1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(DVFS_CLK,	AUDIO,      SPI6,	CLDVFS,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_X4_AUD,	AUDIO,      RSVD0,	SPI1,	    SPI2,	DAP2,	     RSVD,	INPUT),
	PIN(GPIO_X5_AUD,	AUDIO,      RSVD0,	SPI1,	    SPI2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_X6_AUD,	AUDIO,      SPI6,	SPI1,	    SPI2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_X7_AUD,	AUDIO,      RSVD0,	SPI1,	    SPI2,	RSVD3,	     RSVD,	INPUT),
	PIN_RESERVED,	/* Reserved by t114: 0x3388 - 0x3338c */
	PIN_RESERVED,
	PIN(SDMMC3_CLK,	SDMMC3,     SDMMC3,	RSVD1,	    RSVD2,	SPI3,	     RSVD,	INPUT),
	PIN(SDMMC3_CMD,	SDMMC3,     SDMMC3,	PWM3,	    UARTA,	SPI3,	     RSVD,	INPUT),
	PIN(SDMMC3_DAT0,	SDMMC3,     SDMMC3,	RSVD1,	    RSVD2,	SPI3,	     RSVD,	INPUT),
	PIN(SDMMC3_DAT1,	SDMMC3,     SDMMC3,	PWM2,	    UARTA,	SPI3,	     RSVD,	INPUT),
	PIN(SDMMC3_DAT2,	SDMMC3,     SDMMC3,	PWM1,	    DISPLAYA,	SPI3,	     RSVD,	INPUT),
	PIN(SDMMC3_DAT3,	SDMMC3,     SDMMC3,	PWM0,	    DISPLAYB,	SPI3,	     RSVD,	INPUT),
	PIN_RESERVED,	/* Reserved by t114: 0x33a8 - 0x33dc */
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN_RESERVED,
	PIN(HDMI_CEC,	SYS,        CEC,	SDMMC3,	    RSVD2,	SOC,	     RSVD,	INPUT),
	PIN(SDMMC1_WP_N,	SDMMC1,     SDMMC1,	CLK12,	    SPI4,	UARTA,	     RSVD,	INPUT),
	PIN(SDMMC3_CD_N,	SYS,	    SDMMC3,	OWR,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(GPIO_W2_AUD,	AUDIO,      SPI6,	RSVD1,	    SPI2,	I2C1,	     RSVD,	INPUT),
	PIN(GPIO_W3_AUD,	AUDIO,      SPI6,	SPI1,	    SPI2,	I2C1,	     RSVD,	INPUT),
	PIN(USB_VBUS_EN0,	LCD,        USB,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(USB_VBUS_EN1,	LCD,        USB,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC3_CLK_LB_OUT,	SDMMC3,     SDMMC3,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN(SDMMC3_CLK_LB_IN,	SDMMC3,     SDMMC3,	RSVD1,	    RSVD2,	RSVD3,	     RSVD,	INPUT),
	PIN_RESERVED,	/* Reserved by t114: 0x3404 */
	PIN(RESET_OUT_N,	SYS,        RSVD0,	RSVD1,	    RSVD2,	RESET_OUT_N, RSVD,	OUTPUT),

};

void pinmux_set_tristate(enum pmux_pingrp pin, int enable)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *tri = &pmt->pmt_ctl[pin];
	u32 reg;

	/* Error check on pin */
	assert(pmux_pingrp_isvalid(pin));

	reg = readl(tri);
	if (enable)
		reg |= PMUX_TRISTATE_MASK;
	else
		reg &= ~PMUX_TRISTATE_MASK;
	writel(reg, tri);
}

void pinmux_tristate_enable(enum pmux_pingrp pin)
{
	pinmux_set_tristate(pin, 1);
}

void pinmux_tristate_disable(enum pmux_pingrp pin)
{
	pinmux_set_tristate(pin, 0);
}

void pinmux_set_pullupdown(enum pmux_pingrp pin, enum pmux_pull pupd)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *pull = &pmt->pmt_ctl[pin];
	u32 reg;

	/* Error check on pin and pupd */
	assert(pmux_pingrp_isvalid(pin));
	assert(pmux_pin_pupd_isvalid(pupd));

	reg = readl(pull);
	reg &= ~(0x3 << PMUX_PULL_SHIFT);
	reg |= (pupd << PMUX_PULL_SHIFT);
	writel(reg, pull);
}

void pinmux_set_func(enum pmux_pingrp pin, enum pmux_func func)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *muxctl = &pmt->pmt_ctl[pin];
	int i, mux = -1;
	u32 reg;

	/* Error check on pin and func */
	assert(pmux_pingrp_isvalid(pin));
	assert(pmux_func_isvalid(func));

	/* Handle special values */
	if (func == PMUX_FUNC_SAFE)
		func = tegra_soc_pingroups[pin].func_safe;

	if (func & PMUX_FUNC_RSVD) {
		mux = func & 0x3;
	} else {
		/* Search for the appropriate function */
		for (i = 0; i < 4; i++) {
			if (tegra_soc_pingroups[pin].funcs[i] == func) {
				mux = i;
				break;
			}
		}
	}
	assert(mux != -1);

	reg = readl(muxctl);
	reg &= ~(0x3 << PMUX_MUXCTL_SHIFT);
	reg |= (mux << PMUX_MUXCTL_SHIFT);
	writel(reg, muxctl);

}

void pinmux_set_io(enum pmux_pingrp pin, enum pmux_pin_io io)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *pin_io = &pmt->pmt_ctl[pin];
	u32 reg;

	/* Error check on pin and io */
	assert(pmux_pingrp_isvalid(pin));
	assert(pmux_pin_io_isvalid(io));

	reg = readl(pin_io);
	reg &= ~(0x1 << PMUX_IO_SHIFT);
	reg |= (io & 0x1) << PMUX_IO_SHIFT;
	writel(reg, pin_io);
}

static int pinmux_set_lock(enum pmux_pingrp pin, enum pmux_pin_lock lock)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *pin_lock = &pmt->pmt_ctl[pin];
	u32 reg;

	/* Error check on pin and lock */
	assert(pmux_pingrp_isvalid(pin));
	assert(pmux_pin_lock_isvalid(lock));

	if (lock == PMUX_PIN_LOCK_DEFAULT)
		return 0;

	reg = readl(pin_lock);
	reg &= ~(0x1 << PMUX_LOCK_SHIFT);
	if (lock == PMUX_PIN_LOCK_ENABLE)
		reg |= (0x1 << PMUX_LOCK_SHIFT);
	writel(reg, pin_lock);

	return 0;
}

static int pinmux_set_od(enum pmux_pingrp pin, enum pmux_pin_od od)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *pin_od = &pmt->pmt_ctl[pin];
	u32 reg;

	/* Error check on pin and od */
	assert(pmux_pingrp_isvalid(pin));
	assert(pmux_pin_od_isvalid(od));

	if (od == PMUX_PIN_OD_DEFAULT)
		return 0;

	reg = readl(pin_od);
	reg &= ~(0x1 << PMUX_OD_SHIFT);
	if (od == PMUX_PIN_OD_ENABLE)
		reg |= (0x1 << PMUX_OD_SHIFT);
	writel(reg, pin_od);

	return 0;
}

static int pinmux_set_ioreset(enum pmux_pingrp pin,
				enum pmux_pin_ioreset ioreset)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *pin_ioreset = &pmt->pmt_ctl[pin];
	u32 reg;

	/* Error check on pin and ioreset */
	assert(pmux_pingrp_isvalid(pin));
	assert(pmux_pin_ioreset_isvalid(ioreset));

	if (ioreset == PMUX_PIN_IO_RESET_DEFAULT)
		return 0;

	reg = readl(pin_ioreset);
	reg &= ~(0x1 << PMUX_IO_RESET_SHIFT);
	if (ioreset == PMUX_PIN_IO_RESET_ENABLE)
		reg |= (0x1 << PMUX_IO_RESET_SHIFT);
	writel(reg, pin_ioreset);

	return 0;
}

static int pinmux_set_rcv_sel(enum pmux_pingrp pin,
				enum pmux_pin_rcv_sel rcv_sel)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *pin_rcv_sel = &pmt->pmt_ctl[pin];
	u32 reg;

	/* Error check on pin and rcv_sel */
	assert(pmux_pingrp_isvalid(pin));
	assert(pmux_pin_rcv_sel_isvalid(rcv_sel));

	if (rcv_sel == PMUX_PIN_RCV_SEL_DEFAULT)
		return 0;

	reg = readl(pin_rcv_sel);
	reg &= ~(0x1 << PMUX_RCV_SEL_SHIFT);
	if (rcv_sel == PMUX_PIN_RCV_SEL_HIGH)
		reg |= (0x1 << PMUX_RCV_SEL_SHIFT);
	writel(reg, pin_rcv_sel);

	return 0;
}

void pinmux_config_pingroup(struct pingroup_config *config)
{
	enum pmux_pingrp pin = config->pingroup;

	pinmux_set_func(pin, config->func);
	pinmux_set_pullupdown(pin, config->pull);
	pinmux_set_tristate(pin, config->tristate);
	pinmux_set_io(pin, config->io);
	pinmux_set_lock(pin, config->lock);
	pinmux_set_od(pin, config->od);
	pinmux_set_ioreset(pin, config->ioreset);
	pinmux_set_rcv_sel(pin, config->rcv_sel);
}

void pinmux_config_table(struct pingroup_config *config, int len)
{
	int i;

	for (i = 0; i < len; i++)
		pinmux_config_pingroup(&config[i]);
}

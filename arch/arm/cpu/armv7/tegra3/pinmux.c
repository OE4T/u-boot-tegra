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

/* Tegra2 pin multiplexing functions */

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

#define MUXCTL_SHIFT	0
#define PUPD_SHIFT	2
#define TRISTATE_SHIFT	4
#define TRISTATE_MASK	(1 << TRISTATE_SHIFT)
#define IO_SHIFT	5
#define OD_SHIFT	6
#define LOCK_SHIFT	7
#define IO_RESET_SHIFT	8

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
	PIN(NONE, NONE, NONE, NONE, NONE, NONE, NONE)

const struct tegra_pingroup_desc tegra_soc_pingroups[PINGRP_COUNT] = {
	/*  NAME	VDD	f0	f1	f2	f3	fSafe	io */
	PIN(ULPI_DATA0,	  BB,	    SPI3,	HSI,	    UARTA,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_DATA1,	  BB,	    SPI3,	HSI,	    UARTA,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_DATA2,	  BB,	    SPI3,	HSI,	    UARTA,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_DATA3,	  BB,	    SPI3,	HSI,	    UARTA,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_DATA4,	  BB,	    SPI2,	HSI,	    UARTA,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_DATA5,	  BB,	    SPI2,	HSI,	    UARTA,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_DATA6,	  BB,	    SPI2,	HSI,	    UARTA,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_DATA7,	  BB,	    SPI2,	HSI,	    UARTA,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_CLK,	  BB,	    SPI1,	RSVD,	    UARTD,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_DIR,	  BB,	    SPI1,	RSVD,	    UARTD,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_NXT,	  BB,	    SPI1,	RSVD,	    UARTD,	ULPI,	    RSVD,	INPUT),
	PIN(ULPI_STP,	  BB,	    SPI1,	RSVD,	    UARTD,	ULPI,	    RSVD,	INPUT),
	PIN(DAP3_FS,	  BB,	    I2S2,	RSVD1,	    DISPLAYA,	DISPLAYB,   RSVD,	INPUT),
	PIN(DAP3_DIN,	  BB,	    I2S2,	RSVD1,	    DISPLAYA,	DISPLAYB,   RSVD,	INPUT),
	PIN(DAP3_DOUT,	  BB,	    I2S2,	RSVD1,	    DISPLAYA,	DISPLAYB,   RSVD,	INPUT),
	PIN(DAP3_SCLK,	  BB,	    I2S2,	RSVD1,	    DISPLAYA,	DISPLAYB,   RSVD,	INPUT),
	PIN(GPIO_PV0,	  BB,	    RSVD,	RSVD,	    RSVD,	RSVD,	    RSVD,	INPUT),
	PIN(GPIO_PV1,	  BB,	    RSVD,	RSVD,	    RSVD,	RSVD,	    RSVD,	INPUT),
	PIN(SDMMC1_CLK,	  SDMMC1,   SDMMC1,	RSVD1,	    RSVD2,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC1_CMD,	  SDMMC1,   SDMMC1,	RSVD1,	    RSVD2,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC1_DAT3,	  SDMMC1,   SDMMC1,	RSVD1,	    UARTE,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC1_DAT2,	  SDMMC1,   SDMMC1,	RSVD1,	    UARTE,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC1_DAT1,	  SDMMC1,   SDMMC1,	RSVD1,	    UARTE,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC1_DAT0,	  SDMMC1,   SDMMC1,	RSVD1,	    UARTE,	INVALID,    RSVD,	INPUT),
	PIN(GPIO_PV2,	  SDMMC1,   OWR,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(GPIO_PV3,	  SDMMC1,   INVALID,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(CLK2_OUT,	  SDMMC1,   EXTPERIPH2,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(CLK2_REQ,	  SDMMC1,   DAP,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(LCD_PWR1,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_PWR2,	  LCD,	    DISPLAYA,	DISPLAYB,   SPI5,	INVALID,    RSVD,	OUTPUT),
	PIN(LCD_SDIN,	  LCD,	    DISPLAYA,	DISPLAYB,   SPI5,	RSVD,	    RSVD,	OUTPUT),
	PIN(LCD_SDOUT,	  LCD,	    DISPLAYA,	DISPLAYB,   SPI5,	INVALID,    RSVD,	OUTPUT),
	PIN(LCD_WR_N,	  LCD,	    DISPLAYA,	DISPLAYB,   SPI5,	INVALID,    RSVD,	OUTPUT),
	PIN(LCD_CS0_N,	  LCD,	    DISPLAYA,	DISPLAYB,   SPI5,	RSVD,	    RSVD,	OUTPUT),
	PIN(LCD_DC0,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_SCK,	  LCD,	    DISPLAYA,	DISPLAYB,   SPI5,	INVALID,    RSVD,	OUTPUT),
	PIN(LCD_PWR0,	  LCD,	    DISPLAYA,	DISPLAYB,   SPI5,	INVALID,    RSVD,	OUTPUT),
	PIN(LCD_PCLK,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_DE,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_HSYNC,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_VSYNC,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D0,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D1,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D2,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D3,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D4,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D5,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D6,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D7,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D8,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D9,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D10,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D11,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D12,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D13,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D14,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D15,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D16,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D17,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D18,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D19,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D20,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D21,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D22,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_D23,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_CS1_N,	  LCD,	    DISPLAYA,	DISPLAYB,   SPI5,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_M1,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(LCD_DC1,	  LCD,	    DISPLAYA,	DISPLAYB,   RSVD1,	RSVD2,	    RSVD,	OUTPUT),
	PIN(HDMI_INT,	  LCD,	    RSVD,	RSVD,	    RSVD,	RSVD,	    RSVD,	INPUT),
	PIN(DDC_SCL,	  LCD,	    I2C4,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(DDC_SDA,	  LCD,	    I2C4,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(CRT_HSYNC,	  LCD,	    CRT,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(CRT_VSYNC,	  LCD,	    CRT,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(VI_D0,		  VI,	    INVALID,	RSVD1,	    VI,		RSVD2,	    RSVD,	INPUT),
	PIN(VI_D1,		  VI,	    INVALID,	SDMMC2,	    VI,		RSVD1,	    RSVD,	INPUT),
	PIN(VI_D2,		  VI,	    INVALID,	SDMMC2,	    VI,		RSVD1,	    RSVD,	INPUT),
	PIN(VI_D3,		  VI,	    INVALID,	SDMMC2,	    VI,		RSVD1,	    RSVD,	INPUT),
	PIN(VI_D4,		  VI,	    INVALID,	SDMMC2,	    VI,		RSVD1,	    RSVD,	INPUT),
	PIN(VI_D5,		  VI,	    INVALID,	SDMMC2,	    VI,		RSVD1,	    RSVD,	INPUT),
	PIN(VI_D6,		  VI,	    INVALID,	SDMMC2,	    VI,		RSVD1,	    RSVD,	INPUT),
	PIN(VI_D7,		  VI,	    INVALID,	SDMMC2,	    VI,		RSVD1,	    RSVD,	INPUT),
	PIN(VI_D8,		  VI,	    INVALID,	SDMMC2,	    VI,		RSVD1,	    RSVD,	INPUT),
	PIN(VI_D9,		  VI,	    INVALID,	SDMMC2,	    VI,		RSVD1,	    RSVD,	INPUT),
	PIN(VI_D10,	  VI,	    INVALID,	RSVD1,	    VI,		RSVD2,	    RSVD,	INPUT),
	PIN(VI_D11,	  VI,	    INVALID,	RSVD1,	    VI,		RSVD2,	    RSVD,	INPUT),
	PIN(VI_PCLK,	  VI,	    RSVD1,	SDMMC2,	    VI,		RSVD2,	    RSVD,	INPUT),
	PIN(VI_MCLK,	  VI,	    VI,		INVALID,    INVALID,	INVALID,    RSVD,	INPUT),
	PIN(VI_VSYNC,	  VI,	    INVALID,	RSVD1,	    VI,		RSVD2,	    RSVD,	INPUT),
	PIN(VI_HSYNC,	  VI,	    INVALID,	RSVD1,	    VI,		RSVD2,	    RSVD,	INPUT),
	PIN(UART2_RXD,	  UART,	    IRDA,	SPDIF,	    UARTA,	SPI4,	    RSVD,	INPUT),
	PIN(UART2_TXD,	  UART,	    IRDA,	SPDIF,	    UARTA,	SPI4,	    RSVD,	INPUT),
	PIN(UART2_RTS_N,	  UART,	    UARTA,	UARTB,	    GMI,	SPI4,	    RSVD,	INPUT),
	PIN(UART2_CTS_N,	  UART,	    UARTA,	UARTB,	    GMI,	SPI4,	    RSVD,	INPUT),
	PIN(UART3_TXD,	  UART,	    UARTC,	RSVD1,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(UART3_RXD,	  UART,	    UARTC,	RSVD1,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(UART3_CTS_N,	  UART,	    UARTC,	RSVD1,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(UART3_RTS_N,	  UART,	    UARTC,	PWM0,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GPIO_PU0,	  UART,	    OWR,	UARTA,	    GMI,	RSVD1,	    RSVD,	INPUT),
	PIN(GPIO_PU1,	  UART,	    RSVD1,	UARTA,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GPIO_PU2,	  UART,	    RSVD1,	UARTA,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GPIO_PU3,	  UART,	    PWM0,	UARTA,	    GMI,	RSVD1,	    RSVD,	INPUT),
	PIN(GPIO_PU4,	  UART,	    PWM1,	UARTA,	    GMI,	RSVD1,	    RSVD,	INPUT),
	PIN(GPIO_PU5,	  UART,	    PWM2,	UARTA,	    GMI,	RSVD1,	    RSVD,	INPUT),
	PIN(GPIO_PU6,	  UART,	    PWM3,	UARTA,	    GMI,	RSVD1,	    RSVD,	INPUT),
	PIN(GEN1_I2C_SDA,	  UART,	    I2C1,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(GEN1_I2C_SCL,	  UART,	    I2C1,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(DAP4_FS,	  UART,	    I2S3,	RSVD1,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(DAP4_DIN,	  UART,	    I2S3,	RSVD1,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(DAP4_DOUT,	  UART,	    I2S3,	RSVD1,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(DAP4_SCLK,	  UART,	    I2S3,	RSVD1,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(CLK3_OUT,	  UART,	    EXTPERIPH3,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(CLK3_REQ,	  UART,	    DEV3,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(GMI_WP_N,	  GMI,	    RSVD1,	NAND,	    GMI,	GMI_ALT,    RSVD,	INPUT),
	PIN(GMI_IORDY,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_WAIT,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_ADV_N,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_CLK,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_CS0_N,	  GMI,	    RSVD1,	NAND,	    GMI,	INVALID,    RSVD,	INPUT),
	PIN(GMI_CS1_N,	  GMI,	    RSVD1,	NAND,	    GMI,	DTV,	    RSVD,	INPUT),
	PIN(GMI_CS2_N,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_CS3_N,	  GMI,	    RSVD1,	NAND,	    GMI,	GMI_ALT,    RSVD,	INPUT),
	PIN(GMI_CS4_N,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_CS6_N,	  GMI,	    NAND,	NAND_ALT,   GMI,	SATA,	    RSVD,	INPUT),
	PIN(GMI_CS7_N,	  GMI,	    NAND,	NAND_ALT,   GMI,	GMI_ALT,    RSVD,	INPUT),
	PIN(GMI_AD0,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD1,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD2,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD3,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD4,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD5,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD6,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD7,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD8,	  GMI,	    PWM0,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD9,	  GMI,	    PWM1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD10,	  GMI,	    PWM2,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD11,	  GMI,	    PWM3,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD12,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD13,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD14,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_AD15,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD2,	    RSVD,	INPUT),
	PIN(GMI_A16,	  GMI,	    UARTD,	SPI4,	    GMI,	GMI_ALT,    RSVD,	INPUT),
	PIN(GMI_A17,	  GMI,	    UARTD,	SPI4,	    GMI,	INVALID,    RSVD,	INPUT),
	PIN(GMI_A18,	  GMI,	    UARTD,	SPI4,	    GMI,	INVALID,    RSVD,	INPUT),
	PIN(GMI_A19,	  GMI,	    UARTD,	SPI4,	    GMI,	RSVD3,	    RSVD,	INPUT),
	PIN(GMI_WR_N,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD3,	    RSVD,	INPUT),
	PIN(GMI_OE_N,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD3,	    RSVD,	INPUT),
	PIN(GMI_DQS,	  GMI,	    RSVD1,	NAND,	    GMI,	RSVD3,	    RSVD,	INPUT),
	PIN(GMI_RST_N,	  GMI,	    NAND,	NAND_ALT,   GMI,	RSVD3,	    RSVD,	INPUT),
	PIN(GEN2_I2C_SCL,	  GMI,	    I2C2,	INVALID,    GMI,	RSVD3,	    RSVD,	INPUT),
	PIN(GEN2_I2C_SDA,	  GMI,	    I2C2,	INVALID,    GMI,	RSVD3,	    RSVD,	INPUT),
	PIN(SDMMC4_CLK,	  SDMMC4,   INVALID,	NAND,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_CMD,	  SDMMC4,   I2C3,	NAND,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_DAT0,	  SDMMC4,   UARTE,	SPI3,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_DAT1,	  SDMMC4,   UARTE,	SPI3,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_DAT2,	  SDMMC4,   UARTE,	SPI3,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_DAT3,	  SDMMC4,   UARTE,	SPI3,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_DAT4,	  SDMMC4,   I2C3,	I2S4,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_DAT5,	  SDMMC4,   VGP3,	I2S4,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_DAT6,	  SDMMC4,   VGP4,	I2S4,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_DAT7,	  SDMMC4,   VGP5,	I2S4,	    GMI,	SDMMC4,	    RSVD,	INPUT),
	PIN(SDMMC4_RST_N,	  SDMMC4,   VGP6,	RSVD1,	    RSVD2,	POPSDMMC4,  RSVD,	INPUT),
	PIN(CAM_MCLK,	  CAM,	    VI,		INVALID,    VI_ALT2,	POPSDMMC4,  RSVD,	INPUT),
	PIN(GPIO_PCC1,	  CAM,	    I2S4,	RSVD1,	    RSVD2,	POPSDMMC4,  RSVD,	INPUT),
	PIN(GPIO_PBB0,	  CAM,	    I2S4,	RSVD1,	    RSVD2,	POPSDMMC4,  RSVD,	INPUT),
	PIN(CAM_I2C_SCL,	  CAM,	    INVALID,	I2C3,	    RSVD2,	POPSDMMC4,  RSVD,	INPUT),
	PIN(CAM_I2C_SDA,	  CAM,	    INVALID,	I2C3,	    RSVD2,	POPSDMMC4,  RSVD,	INPUT),
	PIN(GPIO_PBB3,	  CAM,	    VGP3,	DISPLAYA,   DISPLAYB,	POPSDMMC4,  RSVD,	INPUT),
	PIN(GPIO_PBB4,	  CAM,	    VGP4,	DISPLAYA,   DISPLAYB,	POPSDMMC4,  RSVD,	INPUT),
	PIN(GPIO_PBB5,	  CAM,	    VGP5,	DISPLAYA,   DISPLAYB,	POPSDMMC4,  RSVD,	INPUT),
	PIN(GPIO_PBB6,	  CAM,	    VGP6,	DISPLAYA,   DISPLAYB,	POPSDMMC4,  RSVD,	INPUT),
	PIN(GPIO_PBB7,	  CAM,	    I2S4,	RSVD1,	    RSVD2,	POPSDMMC4,  RSVD,	INPUT),
	PIN(GPIO_PCC2,	  CAM,	    I2S4,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(JTAG_RTCK,	  SYS,	    RTCK,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PWR_I2C_SCL,	  SYS,	    I2CPWR,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PWR_I2C_SDA,	  SYS,	    I2CPWR,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(KB_ROW0,	  SYS,	    KBC,	INVALID,    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(KB_ROW1,	  SYS,	    KBC,	INVALID,    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(KB_ROW2,	  SYS,	    KBC,	INVALID,    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(KB_ROW3,	  SYS,	    KBC,	INVALID,    RSVD2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW4,	  SYS,	    KBC,	INVALID,    TRACE,	RSVD3,	    RSVD,	INPUT),
	PIN(KB_ROW5,	  SYS,	    KBC,	INVALID,    TRACE,	OWR,	    RSVD,	INPUT),
	PIN(KB_ROW6,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW7,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW8,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW9,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW10,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW11,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW12,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW13,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW14,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_ROW15,	  SYS,	    KBC,	INVALID,    SDMMC2,	INVALID,    RSVD,	INPUT),
	PIN(KB_COL0,	  SYS,	    KBC,	INVALID,    TRACE,	INVALID,    RSVD,	INPUT),
	PIN(KB_COL1,	  SYS,	    KBC,	INVALID,    TRACE,	INVALID,    RSVD,	INPUT),
	PIN(KB_COL2,	  SYS,	    KBC,	INVALID,    TRACE,	RSVD,	    RSVD,	INPUT),
	PIN(KB_COL3,	  SYS,	    KBC,	INVALID,    TRACE,	RSVD,	    RSVD,	INPUT),
	PIN(KB_COL4,	  SYS,	    KBC,	INVALID,    TRACE,	RSVD,	    RSVD,	INPUT),
	PIN(KB_COL5,	  SYS,	    KBC,	INVALID,    TRACE,	RSVD,	    RSVD,	INPUT),
	PIN(KB_COL6,	  SYS,	    KBC,	INVALID,    TRACE,	INVALID,    RSVD,	INPUT),
	PIN(KB_COL7,	  SYS,	    KBC,	INVALID,    TRACE,	INVALID,    RSVD,	INPUT),
	PIN(CLK_32K_OUT,	  SYS,	    BLINK,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(SYS_CLK_REQ,	  SYS,	    SYSCLK,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(CORE_PWR_REQ,	  SYS,	    RSVD,	RSVD,	    RSVD,	RSVD,	    RSVD,	INPUT),
	PIN(CPU_PWR_REQ,	  SYS,	    RSVD,	RSVD,	    RSVD,	RSVD,	    RSVD,	INPUT),
	PIN(PWR_INT_N,	  SYS,	    RSVD,	RSVD,	    RSVD,	RSVD,	    RSVD,	INPUT),
	PIN(CLK_32K_IN,	  SYS,	    RSVD,	RSVD,	    RSVD,	RSVD,	    RSVD,	INPUT),
	PIN(OWR,		  SYS,	    OWR,	RSVD,	    RSVD,	RSVD,	    RSVD,	INPUT),
	PIN(DAP1_FS,	  AUDIO,    I2S0,	HDA,	    GMI,	SDMMC2,	    RSVD,	INPUT),
	PIN(DAP1_DIN,	  AUDIO,    I2S0,	HDA,	    GMI,	SDMMC2,	    RSVD,	INPUT),
	PIN(DAP1_DOUT,	  AUDIO,    I2S0,	HDA,	    GMI,	SDMMC2,	    RSVD,	INPUT),
	PIN(DAP1_SCLK,	  AUDIO,    I2S0,	HDA,	    GMI,	SDMMC2,	    RSVD,	INPUT),
	PIN(CLK1_REQ,	  AUDIO,    DAP,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(CLK1_OUT,	  AUDIO,    EXTPERIPH1,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(SPDIF_IN,	  AUDIO,    SPDIF,	HDA,	    INVALID,	DAPSDMMC2,  RSVD,	INPUT),
	PIN(SPDIF_OUT,	  AUDIO,    SPDIF,	RSVD1,	    INVALID,	DAPSDMMC2,  RSVD,	INPUT),
	PIN(DAP2_FS,	  AUDIO,    I2S1,	HDA,	    RSVD2,	GMI,	    RSVD,	INPUT),
	PIN(DAP2_DIN,	  AUDIO,    I2S1,	HDA,	    RSVD2,	GMI,	    RSVD,	INPUT),
	PIN(DAP2_DOUT,	  AUDIO,    I2S1,	HDA,	    RSVD2,	GMI,	    RSVD,	INPUT),
	PIN(DAP2_SCLK,	  AUDIO,    I2S1,	HDA,	    RSVD2,	GMI,	    RSVD,	INPUT),
	PIN(SPI2_MOSI,	  AUDIO,    SPI6,	SPI2,	    INVALID,	GMI,	    RSVD,	INPUT),
	PIN(SPI2_MISO,	  AUDIO,    SPI6,	SPI2,	    INVALID,	GMI,	    RSVD,	INPUT),
	PIN(SPI2_CS0_N,	  AUDIO,    SPI6,	SPI2,	    INVALID,	GMI,	    RSVD,	INPUT),
	PIN(SPI2_SCK,	  AUDIO,    SPI6,	SPI2,	    INVALID,	GMI,	    RSVD,	INPUT),
	PIN(SPI1_MOSI,	  AUDIO,    SPI2,	SPI1,	    INVALID,	GMI,	    RSVD,	INPUT),
	PIN(SPI1_SCK,	  AUDIO,    SPI2,	SPI1,	    INVALID,	GMI,	    RSVD,	INPUT),
	PIN(SPI1_CS0_N,	  AUDIO,    SPI2,	SPI1,	    INVALID,	GMI,	    RSVD,	INPUT),
	PIN(SPI1_MISO,	  AUDIO,    INVALID,	SPI1,	    INVALID,	RSVD3,	    RSVD,	INPUT),
	PIN(SPI2_CS1_N,	  AUDIO,    INVALID,	SPI2,	    INVALID,	INVALID,    RSVD,	INPUT),
	PIN(SPI2_CS2_N,	  AUDIO,    INVALID,	SPI2,	    INVALID,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_CLK,	  SDMMC3,   UARTA,	PWM2,	    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_CMD,	  SDMMC3,   UARTA,	PWM3,	    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_DAT0,	  SDMMC3,   RSVD0,	RSVD1,	    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_DAT1,	  SDMMC3,   RSVD0,	RSVD1,	    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_DAT2,	  SDMMC3,   RSVD0,	PWM1,	    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_DAT3,	  SDMMC3,   RSVD0,	PWM0,	    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_DAT4,	  SDMMC3,   PWM1,	INVALID,    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_DAT5,	  SDMMC3,   PWM0,	INVALID,    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_DAT6,	  SDMMC3,   SPDIF,	INVALID,    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(SDMMC3_DAT7,	  SDMMC3,   SPDIF,	INVALID,    SDMMC3,	INVALID,    RSVD,	INPUT),
	PIN(PEX_L0_PRSNT_N,  PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PEX_L0_RST_N,	  PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PEX_L0_CLKREQ_N, PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PEX_WAKE_N,	  PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PEX_L1_PRSNT_N,  PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PEX_L1_RST_N,	  PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PEX_L1_CLKREQ_N, PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PEX_L2_PRSNT_N,  PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PEX_L2_RST_N,	  PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(PEX_L2_CLKREQ_N, PEXCTL,   PCIE,	HDA,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
	PIN(HDMI_CEC, 	  SYS,      CEC,	RSVD1,	    RSVD2,	RSVD3,	    RSVD,	INPUT),
};

void pinmux_set_tristate(enum pmux_pingrp pin, int enable)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *tri = &pmt->pmt_ctl[pin];
	u32 reg;

	/* TODO: error check on pin */

	reg = readl(tri);
	if (enable)
		reg |= TRISTATE_MASK;
	else
		reg &= ~TRISTATE_MASK;
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

	/* TODO: error check on pin and pupd, jz */

	reg = readl(pull);
	reg &= ~(0x3 << PUPD_SHIFT);
	reg |= pupd << PUPD_SHIFT;
	writel(reg, pull);
}

void pinmux_set_func(enum pmux_pingrp pin, enum pmux_func func)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *muxctl = &pmt->pmt_ctl[pin];
	int i, mux = -1;
	u32 reg;

	/* TODO: error check on pin and func, jz */
#if 0
	assert(pmux_func_isvalid(func));
#endif

	/* Handle special values */
	if (func == PMUX_FUNC_SAFE)
		func = tegra_soc_pingroups[pin].func_safe;

#if 0 /* t20 u-boot pinmux code */
	if (func >= PMUX_FUNC_RSVD1) {
		mux = (func - PMUX_FUNC_RSVD1) & 0x3;
	} else {
#else /* t30 kernel pinmux code */
	if (func & PMUX_FUNC_RSVD) {
		mux = func & 0x3;
	} else {
#endif
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
	reg &= ~(0x3 << MUXCTL_SHIFT);
	reg |= mux << MUXCTL_SHIFT;
	writel(reg, muxctl);
}

void pinmux_set_io(enum pmux_pingrp pin, enum pmux_pin_io io)
{
	struct pmux_tri_ctlr *pmt =
			(struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	u32 *pin_io = &pmt->pmt_ctl[pin];
	u32 reg;

	/* TODO: error check on pin and io, jz */

	reg = readl(pin_io);
	reg &= ~(0x3 << IO_SHIFT);
	reg |= (io & 0x1) << IO_SHIFT;
	writel(reg, pin_io);
}

void pinmux_config_pingroup(struct pingroup_config *config)
{
	enum pmux_pingrp pin = config->pingroup;

	pinmux_set_func(pin, config->func);
	pinmux_set_pullupdown(pin, config->pull);
	pinmux_set_tristate(pin, config->tristate);
	pinmux_set_io(pin, config->io);
}

void pinmux_config_table(struct pingroup_config *config, int len)
{
	int i;

	for (i = 0; i < len; i++)
		pinmux_config_pingroup(&config[i]);
}


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

#include <common.h>
#include <ns16550.h>
#include <asm/clocks.h>
#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/tegra.h>
#include <asm/arch/sys_proto.h>

#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/gp_padctrl.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/uart.h>
#include <asm/arch/usb.h>
#include <asm/arch/pmc.h>
#include <asm/arch/fuse.h>
#include <asm/arch/emc.h>
#include <spi.h>
#include <fdt_decode.h>
#include <i2c.h>
#include "board.h"

#ifdef CONFIG_TEGRA2_MMC
#include <asm/arch/pmu.h>
#include <mmc.h>
#endif
#if defined(CONFIG_TEGRA3) || defined(CONFIG_TEGRA11X)
#include <asm/arch/sdmmc.h>
#include <asm/arch/gp_padctrl.h>
#endif
#ifdef CONFIG_OF_CONTROL
#include <libfdt.h>
#endif

#include <asm/arch/warmboot.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_OF_CONTROL
const struct tegra2_sysinfo sysinfo = {
	CONFIG_TEGRA2_BOARD_STRING
};
#endif

enum {
	/* UARTs which we can enable */
	UARTA	= 1 << 0,
	UARTB	= 1 << 1,
	UARTD	= 1 << 3,
};

/*
 * Routine: timer_init
 * Description: init the timestamp and lastinc value
 */
int timer_init(void)
{
	reset_timer();
	return 0;
}

static void enable_uart(enum periph_id pid)
{
	/* Assert UART reset and enable clock */
	reset_set_enable(pid, 1);
	clock_enable(pid);
	clock_ll_set_source(pid, 0);	/* UARTx_CLK_SRC = 00, PLLP_OUT0 */

	/* wait for 2us */
	udelay(2);

	/* De-assert reset to UART */
	reset_set_enable(pid, 0);
}

/*
 * Routine: clock_init_uart
 * Description: init clock for the UART(s)
 */
static void clock_init_uart(int uart_ids)
{
	if (uart_ids & UARTA)
		enable_uart(PERIPH_ID_UART1);
	if (uart_ids & UARTB)
		enable_uart(PERIPH_ID_UART2);
	if (uart_ids & UARTD)
		enable_uart(PERIPH_ID_UART4);
}

#if ((!defined(CONFIG_TEGRA3)) && (!defined(CONFIG_TEGRA11X)))
/*
 * Routine: pin_mux_uart
 * Description: setup the pin muxes/tristate values for the UART(s)
 */
static void pin_mux_uart(int uart_ids)
{
	if (uart_ids & UARTA) {
		pinmux_set_func(PINGRP_IRRX, PMUX_FUNC_UARTA);
		pinmux_set_func(PINGRP_IRTX, PMUX_FUNC_UARTA);
		pinmux_tristate_disable(PINGRP_IRRX);
		pinmux_tristate_disable(PINGRP_IRTX);
	}
	if (uart_ids & UARTB) {
		pinmux_set_func(PINGRP_UAD, PMUX_FUNC_IRDA);
		pinmux_tristate_disable(PINGRP_UAD);
	}
	if (uart_ids & UARTD) {
		pinmux_set_func(PINGRP_GMC, PMUX_FUNC_UARTD);
		pinmux_tristate_disable(PINGRP_GMC);
	}
}
#endif

#if defined(CONFIG_TEGRA3) || defined(CONFIG_TEGRA11X)
static void enable_clock(enum periph_id pid, int src, unsigned divisor)
{
	/* Assert reset and enable clock */
	reset_set_enable(pid, 1);
	clock_enable(pid);

	/* Use 'src' if provided, else use default */
	if (src != -1) {
		if (divisor != 0)
			clock_ll_set_source_divisor(pid, src, (divisor-1)<<1);
		else
			clock_ll_set_source(pid, src);
	}

	/* wait for 2us */
	udelay(2);

	/* De-assert reset */
	reset_set_enable(pid, 0);
}

/* Init misc clocks for kernel booting */
static void clock_init_misc(void)
{
#if defined(CONFIG_TEGRA3)
	/* 0 = PLLA_OUT0, -1 = CLK_M (default) */
	enable_clock(PERIPH_ID_I2S0, -1, 0);
	enable_clock(PERIPH_ID_I2S1, 0, 0);
	enable_clock(PERIPH_ID_I2S2, 0, 0);
	enable_clock(PERIPH_ID_I2S3, 0, 0);
	enable_clock(PERIPH_ID_I2S4, -1, 0);
	enable_clock(PERIPH_ID_SPDIF, -1, 0);
#endif

#if defined(CONFIG_TEGRA11X)
	/*
	 * init of clocks that are needed by kernel
	 *
	 */
	/* set to power on default: source: CLK_M, divisor: 1 */
	enable_clock(PERIPH_ID_I2S0, 3, 1);
	enable_clock(PERIPH_ID_I2S1, 3, 1);
	enable_clock(PERIPH_ID_I2S2, 3, 1);
	enable_clock(PERIPH_ID_I2S3, 3, 1);
	enable_clock(PERIPH_ID_I2S4, 3, 1);

	/* spdif: source: PLLP_OUT0, divisor: 9 */
	enable_clock(PERIPH_ID_SPDIF, 0, 9);

	/* host1x: source: PLLP_OUT0, divisor: 4 */
	enable_clock(PERIPH_ID_HOST1X, 4, 4);

	/* 3d: source: PLLM_OUT0, divisor: 6 */
	enable_clock(PERIPH_ID_3D, 0, 6);

	/* 2d: source: PLLM_OUT0, divisor: 6 */
	enable_clock(PERIPH_ID_2D, 0, 6);

	/* 3d2: source: PLLM_OUT0, divisor: 6 */
	enable_clock(PERIPH_ID_3D2, 0, 6);

	/* epp: source: PLLM_OUT0, divisor: 6 */
	enable_clock(PERIPH_ID_EPP, 0, 6);

	/* msenc: source: PLLM_OUT0, divisor: 6 */
	enable_clock(PERIPH_ID_MSENC, 0, 6);

	/* tsec: source: PLLP_OUT0, divisor: 4 */
	enable_clock(PERIPH_ID_TSEC, 0, 4);

	/* vi: source: PLLM_OUT0, divisor: 6 */
	enable_clock(PERIPH_ID_VI, 0, 6);

	/* vde: source: PLLC_OUT0, divisor: 3 */
	enable_clock(PERIPH_ID_VDE, 2, 3);

	/*
	 * RST_V: de-assert reset on:
	 *
	 * 14-12: DAM2-0
	 * 11: APBIF
	 * 10: AUDIO
	 */
	reset_set_enable(PERIPH_ID_AUDIO, 0);
	reset_set_enable(PERIPH_ID_APBIF, 0);
	reset_set_enable(PERIPH_ID_DAM0, 0);
	reset_set_enable(PERIPH_ID_DAM1, 0);
	reset_set_enable(PERIPH_ID_DAM2, 0);
	udelay(2);

	/*
	 * RST_W: de-assert reset on:
	 *
	 * 26: ADX0
	 * 25: AMX0
	 */
	reset_set_enable(PERIPH_ID_AMX0, 0);
	reset_set_enable(PERIPH_ID_ADX0, 0);
	udelay(2);
#endif
}
#endif

#ifdef CONFIG_TEGRA2_MMC
/*
 * Routine: pin_mux_mmc
 * Description: setup the pin muxes/tristate values for the SDMMC(s)
 */
static void pin_mux_mmc(void)
{
#if ((!defined(CONFIG_TEGRA3)) && (!defined(CONFIG_TEGRA11X)))
	/* SDMMC4: config 3, x8 on 2nd set of pins */
	pinmux_set_func(PINGRP_ATB, PMUX_FUNC_SDIO4);
	pinmux_set_func(PINGRP_GMA, PMUX_FUNC_SDIO4);
	pinmux_set_func(PINGRP_GME, PMUX_FUNC_SDIO4);

	pinmux_tristate_disable(PINGRP_ATB);
	pinmux_tristate_disable(PINGRP_GMA);
	pinmux_tristate_disable(PINGRP_GME);

	/* SDMMC3: SDIO3_CLK, SDIO3_CMD, SDIO3_DAT[3:0] */
	pinmux_set_func(PINGRP_SDB, PMUX_FUNC_SDIO3);
	pinmux_set_func(PINGRP_SDC, PMUX_FUNC_SDIO3);
	pinmux_set_func(PINGRP_SDD, PMUX_FUNC_SDIO3);

	pinmux_tristate_disable(PINGRP_SDC);
	pinmux_tristate_disable(PINGRP_SDD);
	pinmux_tristate_disable(PINGRP_SDB);
#endif
}
#endif

#if defined(CONFIG_TEGRA3)
	#include "../cardhu/pinmux-config-common.h"
#endif
#if defined(CONFIG_TEGRA11X)
	#include "../dalmore/pinmux-config-common.h"
#endif

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
static void pinmux_init(int uart_ids)
{
#if defined(CONFIG_TEGRA2)
	pin_mux_uart(uart_ids);
#endif

#if defined(CONFIG_TEGRA3)
	pinmux_config_table(tegra3_pinmux_common,
				ARRAY_SIZE(tegra3_pinmux_common));

	pinmux_config_table(unused_pins_lowpower,
				ARRAY_SIZE(unused_pins_lowpower));
#endif

#if defined(CONFIG_TEGRA11X)
	pinmux_config_table(tegra114_pinmux_set_nontristate,
				ARRAY_SIZE(tegra114_pinmux_set_nontristate));

	pinmux_config_table(tegra114_pinmux_common,
				ARRAY_SIZE(tegra114_pinmux_common));

	pinmux_config_table(unused_pins_lowpower,
				ARRAY_SIZE(unused_pins_lowpower));
#endif
}

/**
 * Do individual peripheral GPIO configs
 *
 * @param blob	FDT blob with configuration information
 */
static void gpio_init(const void *blob)
{
#ifdef CONFIG_SPI_UART_SWITCH
	gpio_early_init_uart(blob);
#endif
}

/*
 * Do I2C/PMU writes to bring up SD card bus power
 *
 */
static void board_sdmmc_voltage_init(void)
{
#if defined(CONFIG_TEGRA3) && defined(CONFIG_TEGRA2_MMC)
	uchar reg, data_buffer[1];
	int i;

	i2c_set_bus_num(0);		/* PMU is on bus 0 */

	data_buffer[0] = 0x65;
	reg = 0x32;

	for (i = 0; i < MAX_I2C_RETRY; ++i) {
		if (i2c_write(PMU_I2C_ADDRESS, reg, 1, data_buffer, 1))
			udelay(100);
	}

	data_buffer[0] = 0x09;
	reg = 0x67;

	for (i = 0; i < MAX_I2C_RETRY; ++i) {
		if (i2c_write(PMU_I2C_ADDRESS, reg, 1, data_buffer, 1))
			udelay(100);
	}
#endif
}

/*
 * Routine: power_det_init
 * Description: turn off power detects
 */
static void power_det_init(void)
{
#if ((!defined(CONFIG_TEGRA3)) && (!defined(CONFIG_TEGRA11X)))
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;

	/* turn off power detects */
	writel(0, &pmc->pmc_pwr_det_latch);
	writel(0, &pmc->pmc_pwr_det);
#endif	/* Tegra3*/
}

/*
 * Routine: board_init
 * Description: Early hardware init.
 */
int board_init(void)
{
	/* boot param addr */
	gd->bd->bi_boot_params = (NV_PA_SDRAM_BASE + 0x100);
	/* board id for Linux */

#ifdef CONFIG_OF_CONTROL
	gd->bd->bi_arch_number = get_arch_number();
#else
	gd->bd->bi_arch_number = CONFIG_MACH_TYPE;
#endif
	clock_init();

#ifdef CONFIG_SPI_UART_SWITCH
	gpio_config_uart(gd->blob);
#endif
#ifdef CONFIG_USB_EHCI_TEGRA
	board_usb_init(gd->blob);
#endif
#ifdef CONFIG_SPI_FLASH
	spi_init();
#endif
	power_det_init();
#ifdef CONFIG_TEGRA2_I2C
	i2c_init_board();

#ifdef CONFIG_ENABLE_EMC_INIT
	pmu_set_nominal();
	board_emc_init();
#endif
	board_sdmmc_voltage_init();
#endif

#ifdef CONFIG_TEGRA2_LP0
	/* prepare the WB code to LP0 location */
	warmboot_prepare_code(TEGRA_LP0_ADDR, TEGRA_LP0_SIZE);
#endif


	return 0;
}

#ifdef CONFIG_BOARD_EARLY_INIT_F

int board_early_init_f(void)
{
	extern void cpu_init_crit(void);
	int uart_ids = 0;	/* bit mask of which UART ids to enable */

#ifdef CONFIG_OF_CONTROL
	struct fdt_uart uart;

	if (!fdt_decode_uart_console(gd->blob, &uart, gd->baudrate))
		uart_ids = 1 << uart.id;
#else
#ifdef CONFIG_TEGRA2_ENABLE_UARTA
	uart_ids |= UARTA;
#endif
#ifdef CONFIG_TEGRA2_ENABLE_UARTB
	uart_ids |= UARTB;
#endif
#ifdef CONFIG_TEGRA2_ENABLE_UARTD
	uart_ids |= UARTD;
#endif
#endif /* CONFIG_OF_CONTROL */

	/* We didn't do this init in start.S, so do it now */
	cpu_init_crit();

	/* Initialize essential common plls */
	common_pll_init();

	/* Initialize UART clocks */
	clock_init_uart(uart_ids);

#if defined(CONFIG_TEGRA3) || defined(CONFIG_TEGRA11X)
	/* Initialize misc clocks for kernel booting */
	clock_init_misc();
#endif
	/* Initialize periph pinmuxes */
	pinmux_init(uart_ids);

	/* Initialize periph GPIOs */
	gpio_init(gd->blob);

#ifdef CONFIG_VIDEO_TEGRA2
	/* Get LCD panel size */
	lcd_early_init(gd->blob);
#endif

	return 0;
}
#endif	/* EARLY_INIT */

#ifdef CONFIG_ARCH_CPU_INIT
/*
 * Note this function is executed by the ARM7TDMI AVP. It does not return
 * in this case. It is also called once the A9 starts up, but does nothing in
 * that case.
 */
int arch_cpu_early_init(void)
{
	/* Fire up the Cortex A9 */
	tegra2_start();
	return 0;
}
int arch_cpu_init(void)
{
	/* Fire up the Cortex A9 */
//	tegra2_start();
	return 0;
}
#endif

void pad_init_mmc(struct tegra2_mmc *reg)
{
#if defined(CONFIG_TEGRA3) || defined(CONFIG_TEGRA11X)
	struct apb_misc_gp_ctlr *const gpc =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;
	struct sdmmc_ctlr *const sdmmc = (struct sdmmc_ctlr *)reg;
	u32 val, offset = (unsigned int)reg;
	u32 padcfg, padmask;

	debug("%s: sdmmc address = %08x\n", __func__, (unsigned int)sdmmc);

	/* Set the pad drive strength for SDMMC1 or 3 only */
	if (offset != NV_PA_SDMMC1_BASE && offset != NV_PA_SDMMC3_BASE) {
		debug("%s: settings are only valid for SDMMC1/SDMMC3!\n",
			__func__);
		return;
	}

	/*
	 * Set pads as per T30 TRM, section 24.6.1.2
	 * or per T114 TRM, section 27.5.1
	 */
	padcfg = (GP_SDIOCFG_DRVUP_SLWF | GP_SDIOCFG_DRVDN_SLWR | \
		GP_SDIOCFG_DRVUP | GP_SDIOCFG_DRVDN);
	padmask = 0x00000FFF;

#if defined(CONFIG_TEGRA11X)
	val = padcfg | (GP_SDIOCFG_SCHMITT | GP_SDIOCFG_HSM);
#endif
	if (offset == NV_PA_SDMMC1_BASE) {
#if defined(CONFIG_TEGRA3)
		val = readl(&gpc->sdio1cfg);
		val &= padmask;
		val |= padcfg;
#endif
		writel(val, &gpc->sdio1cfg);
	} else {				/* SDMMC3 */
#if defined(CONFIG_TEGRA3)
		val = readl(&gpc->sdio3cfg);
		val &= padmask;
		val |= padcfg;
#endif
		writel(val, &gpc->sdio3cfg);
	}

	val = readl(&sdmmc->sdmmc_sdmemcomp_pad_ctrl);
	val &= 0xFFFFFFF0;
	val |= MEMCOMP_PADCTRL_VREF;
	writel(val, &sdmmc->sdmmc_sdmemcomp_pad_ctrl);

#if defined(CONFIG_TEGRA3)
	val = readl(&sdmmc->sdmmc_auto_cal_config);
	val &= 0xFFFF0000;
	val |= AUTO_CAL_PU_OFFSET | AUTO_CAL_PD_OFFSET | AUTO_CAL_ENABLED;
	writel(val, &sdmmc->sdmmc_auto_cal_config);
#endif

#endif
}

#ifdef CONFIG_TEGRA2_MMC
/* this is a weak define that we are overriding */
int board_mmc_init(bd_t *bd)
{
	debug("board_mmc_init called\n");

	/* Enable muxes, etc. for SDMMC controllers */
	pin_mux_mmc();

	tegra2_mmc_init(gd->blob);

	return 0;
}
#endif

int tegra_get_chip_type(void)
{
	uint tegra_sku_id;

	struct fuse_regs *fuse = (struct fuse_regs *)NV_PA_FUSE_BASE;

	tegra_sku_id = readl(&fuse->sku_info) & 0xff;

	switch (tegra_sku_id) {
	case SKU_ID_T20:
		return TEGRA_SOC_T20;
	case SKU_ID_T25SE:
	case SKU_ID_AP25:
	case SKU_ID_T25:
	case SKU_ID_AP25E:
	case SKU_ID_T25E:
		return TEGRA_SOC_T25;
	default:
		/* unknown sku id */
		return TEGRA_SOC_UNKNOWN;
	}
}

#ifdef CONFIG_OF_CONTROL

const char* get_board_name(void)
{
	int len;
	const char *board_name;
	int node = fdt_node_offset_by_compatible(gd->blob, -1,
						 CONFIG_COMPAT_STRING);

	board_name = fdt_getprop(gd->blob, node, "model", &len);
	if (!board_name)
		board_name = "<not defined>";

	return board_name;
}

struct arch_name_map {
	const char* board_name;
	ulong arch_number;
};

static struct arch_name_map name_map[] = {
	{"Google Aebl", MACH_TYPE_AEBL},
	{"NVIDIA Cardhu", MACH_TYPE_CARDHU},
	{"Google Kaen", MACH_TYPE_KAEN},
	{"NVIDIA Seaboard", MACH_TYPE_SEABOARD},
	{"NVIDIA Ventana", MACH_TYPE_VENTANA},
	{"NVIDIA Dalmore", MACH_TYPE_DALMORE},
	{"Google Waluigi", MACH_TYPE_WALUIGI},
	{"<not defined>", MACH_TYPE_SEABOARD} /* this is the default */
};

ulong get_arch_number(void)
{
	int i;
	const char* board_name = get_board_name();

	for (i = 0; i < ARRAY_SIZE(name_map); i++)
		if (!strcmp(name_map[i].board_name, board_name))
			return name_map[i].arch_number;

	/* should never happen */
	puts("Failed to find arch number");
	return 0;
}
#endif

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	/* pass board id to kernel */
	serialnr->high = CONFIG_TEGRA_SERIAL_HIGH;
	serialnr->low = CONFIG_TEGRA_SERIAL_LOW;
	/* TODO: use FDT */

	debug("Config file serialnr->high = %08X, ->low = %08X\n",
		serialnr->high, serialnr->low);

	/*
	 * Check if we can read the EEPROM serial number
	 *  and if so, use it instead.
	 */

#if !defined(CONFIG_SERIAL_EEPROM)
	debug("No serial EEPROM onboard, using default values\n");
	return;
#else
	uchar data_buffer[NUM_SERIAL_ID_BYTES];

	i2c_set_bus_num(EEPROM_I2C_BUS);

	if (i2c_read(EEPROM_I2C_ADDRESS, EEPROM_SERIAL_OFFSET,
		1, data_buffer, NUM_SERIAL_ID_BYTES)) {
		printf("%s: I2C read of bus %d chip %d addr %d failed!\n",
			__func__, EEPROM_I2C_BUS, EEPROM_I2C_ADDRESS,
			EEPROM_SERIAL_OFFSET);
	} else {
#ifdef DEBUG
		int i;
		printf("get_board_serial: Got ");
		for (i = 0; i < NUM_SERIAL_ID_BYTES; i++)
			printf("%02X:", data_buffer[i]);
		printf("\n");
#endif
		serialnr->high = data_buffer[2];
		serialnr->high |= (u32)data_buffer[3] << 8;
		serialnr->high |= (u32)data_buffer[0] << 16;
		serialnr->high |= (u32)data_buffer[1] << 24;

		serialnr->low = data_buffer[7];
		serialnr->low |= (u32)data_buffer[6] << 8;
		serialnr->low |= (u32)data_buffer[5] << 16;
		serialnr->low |= (u32)data_buffer[4] << 24;

		debug("SEEPROM serialnr->high = %08X, ->low = %08X\n",
			serialnr->high, serialnr->low);
	}
#endif	/* SERIAL_EEPROM */
}
#endif	/* SERIAL_TAG */

u32 get_minor_rev(void)
{
	u32 minor_rev;
	struct apb_misc_gp_ctlr *gp =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;

	minor_rev = readl(&gp->hidrev);
	minor_rev >>= 16;			/* MINORREV = bits 19:16 */
	minor_rev &= 0x0F;

	debug("Tegra chip minor rev = %X\n", minor_rev);
	return minor_rev;
}

#ifdef	CONFIG_MISC_INIT_R
int misc_init_r(void)
{
#if defined(CONFIG_TEGRA3) || defined(CONFIG_TEGRA11X)
	char buf[255], *s, *maxptr;

	/*
	 * We need to differentiate between T30 and T33 by passing
	 * 'max_cpu_cur_ma=10000' to the kernel if we're on a T33,
	 * as per NVBug 932291.  T30 = 0x02, T33 = 0x03.
	 */
	s = getenv("extra_bootargs");
	debug("old kernel cmd line is %s\n", s);

	if (get_minor_rev() == 0x03) {
		if (s) {
			maxptr = strstr(s, "max_cpu_cur_ma");
			if (maxptr == NULL) {
				sprintf(buf, "%s %s", s,
					"max_cpu_cur_ma=10000");
				setenv("extra_bootargs", buf);
				debug("new kernel cmd line is %s\n", buf);
			} else {
				debug("max_cpu_cur_ma already present!\n");
			}
		}
	}
#endif
	return 0;
}
#endif

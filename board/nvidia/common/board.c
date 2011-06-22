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
#include <asm/arch/tegra2.h>
#include <asm/arch/sys_proto.h>

#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
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
#include <mmc.h>
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

#ifdef CONFIG_TEGRA2_MMC
/*
 * Routine: pin_mux_mmc
 * Description: setup the pin muxes/tristate values for the SDMMC(s)
 */
static void pin_mux_mmc(void)
{
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
}
#endif

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
static void pinmux_init(int uart_ids)
{
	pin_mux_uart(uart_ids);
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
 * Routine: power_det_init
 * Description: turn off power detects
 */
static void power_det_init(void)
{
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;

	/* turn off power detects */
	writel(0, &pmc->pmc_pwr_det_latch);
	writel(0, &pmc->pmc_pwr_det);
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
#ifdef CONFIG_TEGRA2_SPI
	spi_init();
#endif
	power_det_init();
#ifdef CONFIG_TEGRA2_I2C
	i2c_init_board();

	pmu_set_nominal();

	board_emc_init();
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
int arch_cpu_init(void)
{
	/* Fire up the Cortex A9 */
	tegra2_start();
	return 0;
}
#endif

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
	ulong	    arch_number;
};

static struct arch_name_map name_map[] = {
	{"Google Kaen", MACH_TYPE_KAEN},
	{"NVIDIA Seaboard", MACH_TYPE_SEABOARD},
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

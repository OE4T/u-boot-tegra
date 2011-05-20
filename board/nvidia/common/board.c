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
#include <asm/io.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/tegra2.h>
#include <asm/arch/sys_proto.h>

#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/uart.h>
#include <asm/arch/usb.h>
#include <spi.h>
#include "board.h"

DECLARE_GLOBAL_DATA_PTR;

const struct tegra2_sysinfo sysinfo = {
	CONFIG_TEGRA2_BOARD_STRING
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

/*
 * Routine: clock_init_uart
 * Description: init the PLL and clock for the UART(s)
 */
static void clock_init_uart(void)
{
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	struct clk_pll *pll = &clkrst->crc_pll[CLOCK_PLL_ID_PERIPH];
	u32 reg;

	reg = readl(&pll->pll_base);
	if (!(reg & bf_mask(PLL_BASE_OVRRIDE))) {
		/* Override pllp setup for 216MHz operation. */
		reg = bf_mask(PLL_BYPASS) | bf_mask(PLL_BASE_OVRRIDE) |
			bf_pack(PLL_DIVP, 1) | bf_pack(PLL_DIVM, 0xc);
		reg |= bf_pack(PLL_DIVN, NVRM_PLLP_FIXED_FREQ_KHZ / 500);
		writel(reg, &pll->pll_base);

		reg |= bf_mask(PLL_ENABLE);
		writel(reg, &pll->pll_base);

		reg &= ~bf_mask(PLL_BYPASS);
		writel(reg, &pll->pll_base);
	}

#if defined(CONFIG_TEGRA2_ENABLE_UARTA)
	/* Assert UART reset and enable clock */
	reset_set_enable(PERIPH_ID_UART1, 1);
	clock_enable(PERIPH_ID_UART1);

	/* Enable pllp_out0 to UART */
	reg = readl(&clkrst->crc_clk_src_uarta);
	reg &= 0x3FFFFFFF;	/* UARTA_CLK_SRC = 00, PLLP_OUT0 */
	writel(reg, &clkrst->crc_clk_src_uarta);

	/* wait for 2us */
	udelay(2);

	/* De-assert reset to UART */
	reset_set_enable(PERIPH_ID_UART1, 0);
#endif	/* CONFIG_TEGRA2_ENABLE_UARTA */
#if defined(CONFIG_TEGRA2_ENABLE_UARTD)
	/* Assert UART reset and enable clock */
	reset_set_enable(PERIPH_ID_UART4, 1);
	clock_enable(PERIPH_ID_UART4);

	/* Enable pllp_out0 to UART */
	reg = readl(&clkrst->crc_clk_src_uartd);
	reg &= 0x3FFFFFFF;	/* UARTD_CLK_SRC = 00, PLLP_OUT0 */
	writel(reg, &clkrst->crc_clk_src_uartd);

	/* wait for 2us */
	udelay(2);

	/* De-assert reset to UART */
	reset_set_enable(PERIPH_ID_UART4, 0);
#endif	/* CONFIG_TEGRA2_ENABLE_UARTD */
}

/*
 * Routine: pin_mux_uart
 * Description: setup the pin muxes/tristate values for the UART(s)
 */
static void pin_mux_uart(void)
{
#if defined(CONFIG_TEGRA2_ENABLE_UARTA)
	pinmux_set_func(PINGRP_IRRX, PMUX_FUNC_UARTA);
	pinmux_set_func(PINGRP_IRTX, PMUX_FUNC_UARTA);
	pinmux_tristate_disable(PINGRP_IRRX);
	pinmux_tristate_disable(PINGRP_IRTX);
#endif	/* CONFIG_TEGRA2_ENABLE_UARTA */
#if defined(CONFIG_TEGRA2_ENABLE_UARTD)
	pinmux_set_func(PINGRP_GMC, PMUX_FUNC_UARTD);
	pinmux_tristate_disable(PINGRP_GMC);
#endif	/* CONFIG_TEGRA2_ENABLE_UARTD */
}

/*
 * Routine: clock_init
 * Description: Do individual peripheral clock reset/enables
 */
static void clock_init(void)
{
	clock_init_uart();
}

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
static void pinmux_init(void)
{
	pin_mux_uart();
}

/*
 * Routine: gpio_init
 * Description: Do individual peripheral GPIO configs
 */
static void gpio_init(void)
{
	gpio_config_uart();
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
	gd->bd->bi_arch_number = CONFIG_MACH_TYPE;

#ifdef CONFIG_USB_EHCI_TEGRA
	board_usb_init();
#endif
#ifdef CONFIG_TEGRA2_SPI
	spi_init();
#endif

	return 0;
}

#ifdef CONFIG_BOARD_EARLY_INIT_F

int board_early_init_f(void)
{
	extern void cpu_init_crit(void);

	/* We didn't do this init in start.S, so do it now */
	cpu_init_crit();

	/* Initialize periph clocks */
	clock_init();

	/* Initialize periph pinmuxes */
	pinmux_init();

	/* Initialize periph GPIOs */
	gpio_init();

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

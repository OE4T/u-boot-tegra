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
#include <asm/arch/gpio.h>
#ifdef CONFIG_TEGRA2_MMC
#include <mmc.h>
#endif

/*
 * Routine: gpio_config_mmc
 * Description: Set GPIOs for SDMMC3 SDIO slot.
 */
void gpio_config_mmc(void)
{
	/* Set EN_VDDIO_SD (GPIO I6) */
	gpio_direction_output(GPIO_PI6, 1);

	/* Config pin as GPI for Card Detect (GPIO I5) */
	gpio_direction_input(GPIO_PI5);
}

#ifdef CONFIG_TEGRA2_MMC
/* this is a weak define that we are overriding */
int board_mmc_getcd(u8 *cd, struct mmc *mmc)
{
	debug("board_mmc_getcd called\n");
	*cd = 1;			/* Assume card is inserted, or eMMC */

	if (IS_SD(mmc)) {
		/* Seaboard SDMMC3 = SDIO3_CD = GPIO_PI5 */
		if (gpio_get_value(GPIO_PI5))
			*cd = 0;
	}

	return 0;
}
#endif

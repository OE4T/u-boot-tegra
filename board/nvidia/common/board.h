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

#ifndef _BOARD_H_
#define _BOARD_H_

#define DSI_PANEL_BL_EN_GPIO		GPIO_PH2

void tegra2_start(void);
void gpio_config_uart(const void *blob);
void gpio_early_init_uart(const void *blob);
void gpio_config_mmc(void);
int tegra2_mmc_init(const void *blob);
void lcd_early_init(const void *blob);
int tegra_get_chip_type(void);
int pmu_set_nominal(void);
u32 get_minor_rev(void);
int misc_init_r(void);

#endif	/* BOARD_H */

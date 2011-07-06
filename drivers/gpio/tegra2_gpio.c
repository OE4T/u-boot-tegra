/*
 * drivers/gpio/tegra2_gpio.c
 *
 * NVIDIA Tegra2 GPIO handling.
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

/*
 * Based on (mostly copied from) kw_gpio.c based Linux 2.6 kernel driver.
 * Tom Warren (twarren@nvidia.com)
 */

#include <common.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/arch/tegra.h>
#include <asm/arch/gpio.h>

enum {
	TEGRA2_CMD_INFO,
	TEGRA2_CMD_PORT,
	TEGRA2_CMD_OUTPUT,
	TEGRA2_CMD_INPUT,
};

/* Config port 'gp' as GPIO or SFPIO, based on 'type' */
void __set_config(int gp, int type)
{
	struct gpio_ctlr *gpio = (struct gpio_ctlr *)NV_PA_GPIO_BASE;
	struct gpio_ctlr_bank *bank = &gpio->gpio_bank[GPIO_BANK(gp)];
	u32 u;

	debug("__set_config: port = %d, bit = %d, %s\n",
		GPIO_PORT8(gp), GPIO_BIT(gp), type ? "GPIO" : "SFPIO");

	u = readl(&bank->gpio_config[GPIO_PORT(gp)]);
	if (type)				/* GPIO */
		u |= 1 << GPIO_BIT(gp);
	else
		u &= ~(1 << GPIO_BIT(gp));
	writel(u, &bank->gpio_config[GPIO_PORT(gp)]);
}

/* Config GPIO port 'gp' as input or output (OE) as per 'output' */
void __set_direction(int gp, int output)
{
	struct gpio_ctlr *gpio = (struct gpio_ctlr *)NV_PA_GPIO_BASE;
	struct gpio_ctlr_bank *bank = &gpio->gpio_bank[GPIO_BANK(gp)];
	u32 u;

	debug("__set_direction: port = %d, bit = %d, %s\n",
		GPIO_PORT8(gp), GPIO_BIT(gp), output ? "OUT" : "IN");

	u = readl(&bank->gpio_dir_out[GPIO_PORT(gp)]);
	if (output)
		u |= 1 << GPIO_BIT(gp);
	else
		u &= ~(1 << GPIO_BIT(gp));
	writel(u, &bank->gpio_dir_out[GPIO_PORT(gp)]);
}

/* set GPIO port 'gp' output bit as 0 or 1 as per 'high' */
void __set_level(int gp, int high)
{
	struct gpio_ctlr *gpio = (struct gpio_ctlr *)NV_PA_GPIO_BASE;
	struct gpio_ctlr_bank *bank = &gpio->gpio_bank[GPIO_BANK(gp)];
	u32 u;

	debug("__set_level: port = %d, bit %d == %d\n",
		GPIO_PORT8(gp), GPIO_BIT(gp), high);

	u = readl(&bank->gpio_out[GPIO_PORT(gp)]);
	if (high)
		u |= 1 << GPIO_BIT(gp);
	else
		u &= ~(1 << GPIO_BIT(gp));
	writel(u, &bank->gpio_out[GPIO_PORT(gp)]);
}

/*
 * GENERIC_GPIO primitives.
 */

/* set GPIO port 'gp' as an input */
int gpio_direction_input(int gp)
{
	debug("gpio_direction_input: offset = %d (port %d:bit %d)\n",
		gp, GPIO_PORT8(gp), GPIO_BIT(gp));

	/* Configure as a GPIO */
	__set_config(gp, 1);

	/* Configure GPIO direction as input. */
	__set_direction(gp, 0);

	return 0;
}

/* set GPIO port 'gp' as an output, with polarity 'value' */
int gpio_direction_output(int gp, int value)
{
	debug("gpio_direction_output: offset = %d (port %d:bit %d) = %s\n",
		gp, GPIO_PORT8(gp), GPIO_BIT(gp), value ? "HIGH" : "LOW");

	/* Configure as a GPIO */
	__set_config(gp, 1);

	/* Configure GPIO output value. */
	__set_level(gp, value);

	/* Configure GPIO direction as output. */
	__set_direction(gp, 1);

	return 0;
}

/* read GPIO IN value of port 'gp' */
int gpio_get_value(int gp)
{
	struct gpio_ctlr *gpio = (struct gpio_ctlr *)NV_PA_GPIO_BASE;
	struct gpio_ctlr_bank *bank = &gpio->gpio_bank[GPIO_BANK(gp)];
	int val;

	debug("gpio_get_value: offset = %d (port %d:bit %d)\n",
		gp, GPIO_PORT8(gp), GPIO_BIT(gp));

	val = readl(&bank->gpio_in[GPIO_PORT(gp)]);

	return (val >> GPIO_BIT(gp)) & 1;
}

/* write GPIO OUT value to port 'gp' */
void gpio_set_value(int gp, int value)
{
	debug("gpio_set_value: offset = %d (port %d:bit %d), value = %d\n",
		gp, GPIO_PORT8(gp), GPIO_BIT(gp), value);

	/* Configure GPIO output value. */
	__set_level(gp, value);
}

#ifdef CONFIG_CMD_TEGRA2_GPIO_INFO
/*
 * Display Tegra GPIO information
 */
static int gpio_info(int gp)
{
	struct gpio_ctlr *gpio = (struct gpio_ctlr *)NV_PA_GPIO_BASE;
	struct gpio_ctlr_bank *bank = &gpio->gpio_bank[GPIO_BANK(gp)];
	int i, port;
	u32 data;

	port = GPIO_PORT8(gp);		/* 8-bit port # */

	printf("Tegra2 GPIO port %d:\n\n", port);
	printf("gpio bits: 76543210\n");
	printf("-------------------\n");

	if (port < 0 || port > 27)
		return -1;

	printf("GPIO_CNF:  ");
	data = readl(&bank->gpio_config[GPIO_PORT(gp)]);
	for (i = 7; i >= 0; i--)
		printf("%c", data & (1 << i) ? 'g' : 's');
	printf("\n");

	printf("GPIO_OE:   ");
	data = readl(&bank->gpio_dir_out[GPIO_PORT(gp)]);
	for (i = 7; i >= 0; i--)
		printf("%c", data & (1 << i) ? 'o' : 'i');
	printf("\n");

	printf("GPIO_IN:   ");
	data = readl(&bank->gpio_in[GPIO_PORT(gp)]);
	for (i = 7; i >= 0; i--)
		printf("%c", data & (1 << i) ? '1' : '0');
	printf("\n");

	printf("GPIO_OUT:  ");
	data = readl(&bank->gpio_out[GPIO_PORT(gp)]);
	for (i = 7; i >= 0; i--)
		printf("%c", data & (1 << i) ? '1' : '0');
	printf("\n");

	return 0;
}
#endif /* CONFIG_CMD_TEGRA2_GPIO_INFO */

cmd_tbl_t cmd_gpio[] = {
	U_BOOT_CMD_MKENT(offset2port, 3, 0, (void *)TEGRA2_CMD_PORT, "", ""),
	U_BOOT_CMD_MKENT(output, 4, 0, (void *)TEGRA2_CMD_OUTPUT, "", ""),
	U_BOOT_CMD_MKENT(input, 3, 0, (void *)TEGRA2_CMD_INPUT, "", ""),
#ifdef CONFIG_CMD_TEGRA2_GPIO_INFO
	U_BOOT_CMD_MKENT(info, 3, 0, (void *)TEGRA2_CMD_INFO, "", ""),
#endif
};

int do_gpio(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	static uint8_t offset;
	int val, port, bit;
	ulong ul_arg2, ul_arg3, ul_arg4;
	cmd_tbl_t *c;

	ul_arg2 = ul_arg3 = ul_arg4 = 0;

	c = find_cmd_tbl(argv[1], cmd_gpio, ARRAY_SIZE(cmd_gpio));

	/* All commands but "port" require 'maxargs' arguments */
	if (!c || !((argc == (c->maxargs)) ||
		(((int)c->cmd == TEGRA2_CMD_PORT) &&
		 (argc == (c->maxargs - 1))))) {
		cmd_usage(cmdtp);
		return 1;
	}

	/* arg2 used as offset */
	if (argc > 2)
		ul_arg2 = simple_strtoul(argv[2], NULL, 10);

	/* arg3 used as output value, 0 or 1 */
	if (argc > 3)
		ul_arg3 = simple_strtoul(argv[3], NULL, 10) & 0x1;

	switch ((int)c->cmd) {
#ifdef CONFIG_CMD_TEGRA2_GPIO_INFO
	case TEGRA2_CMD_INFO:
		if (argc == 3)
			offset = (uint8_t)ul_arg2;
		return gpio_info(offset);

#endif
	case TEGRA2_CMD_PORT:
		if (argc == 3)
			offset = (uint8_t)ul_arg2;
		port = GPIO_PORT8(offset);
		bit = GPIO_BIT(offset);
		printf("Offset %d = port # %d, bit # %d\n",
			offset, port, bit);
		return 0;

	case TEGRA2_CMD_INPUT:
		/* arg2 = offset */
		port = GPIO_PORT8(offset);
		bit = GPIO_BIT(offset);
		gpio_direction_input(ul_arg2);
		val = gpio_get_value(ul_arg2);

		printf("Offset %lu (port:bit %d:%d) = %d\n",
			ul_arg2, port, bit, val);
		return 0;

	case TEGRA2_CMD_OUTPUT:
		/* args = offset, value */
		port = GPIO_PORT8(offset);
		bit = GPIO_BIT(offset);
		gpio_direction_output(ul_arg2, ul_arg3);
		printf("Offset %lu (port:bit %d:%d) = %ld\n",
			ul_arg2, port, bit, ul_arg3);
		return 0;

	default:
		/* We should never get here */
		return 1;
	}
}

U_BOOT_CMD(
	gpio,	5,	1,	do_gpio,
	"GPIO access",
	"offset2port offset\n"
	"	- show GPIO port:bit based on 'offset'\n"
#ifdef CONFIG_CMD_TEGRA2_GPIO_INFO
	"     info offset\n"
	"	- display info for all bits in port based on 'offset'\n"
#endif
	"     output offset 0|1\n"
	"	- using 'offset', set port:bit as output and drive low or high\n"
	"     input offset\n"
	"	- using 'offset', set port:bit as input and read value"
);

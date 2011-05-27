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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <serial.h>
#include <libfdt.h>
#include <fdt_support.h>
#include <fdt_decode.h>

/* we need a generic GPIO interface here */
#include <asm/arch/gpio.h>

/*
 * Here are the type we know about. One day we might allow drivers to
 * register. For now we just put them here.
 */
static struct fdt_compat compat_types[] = {
	{ COMPAT_UNKNOWN, "<none>" },
	{ COMPAT_SPI_UART_SWITCH, "spi-uart-select" },
	{ COMPAT_SERIAL_NS16550, "ns16550" },
};

/**
 * Look in the FDT for an alias with the given name and return its node.
 *
 * @param blob	FDT blob
 * @param name	alias name to look up
 * @return node offset if found, or an error code < 0 otherwise
 */
static int find_alias_node(const void *blob, const char *name)
{
	const char *path;
	int alias_node;

	alias_node = fdt_path_offset(blob, "/aliases");
	if (alias_node < 0)
		return alias_node;
	path = fdt_getprop(blob, alias_node, name, NULL);
	if (!path)
		return -FDT_ERR_NOTFOUND;
	return fdt_path_offset(blob, path);
}

/**
 * Look up an address property in a node and return it as an address.
 * The property must hold exactly one address with no trailing data.
 * This is only tested on 32-bit machines.
 *
 * @param blob	FDT blob
 * @param node	node to examine
 * @param prop_name	name of property to find
 * @return address, if found, or ADDR_T_NONE if not
 */
static addr_t get_addr(const void *blob, int node, const char *prop_name)
{
	const addr_t *cell;
	int len;

	cell = fdt_getprop(blob, node, prop_name, &len);
	if (cell && len != sizeof(addr_t))
		return addr_to_cpu(*cell);
	return ADDR_T_NONE;
}

/**
 * Look up a 32-bit integer property in a node and return it. The property
 * must have at least 4 bytes of data. The value of the first cell is
 * returned.
 *
 * @param blob	FDT blob
 * @param node	node to examine
 * @param prop_name	name of property to find
 * @param default_val	default value to return if the property is not found
 * @return integer value, if found, or default_val if not
 */
static s32 get_int(const void *blob, int node, const char *prop_name,
		s32 default_val)
{
	const s32 *cell;
	int len;

	cell = fdt_getprop(blob, node, prop_name, &len);
	if (cell && len >= sizeof(s32))
		return fdt32_to_cpu(cell[0]);
	return default_val;
}

/**
 * Look up a phandle and follow it to its node. Then return the offset
 * of that node.
 *
 * @param blob		FDT blob
 * @param node		node to examine
 * @param prop_name	name of property to find
 * @return node offset if found, -ve error code on error
 */
static int lookup_phandle(const void *blob, int node, const char *prop_name)
{
	const u32 *phandle;
	int lookup;

	phandle = fdt_getprop(blob, node, prop_name, NULL);
	if (!phandle)
		return -FDT_ERR_NOTFOUND;

	lookup = fdt_node_offset_by_phandle(blob, fdt32_to_cpu(*phandle));
	return lookup;
}

/**
 * Checks whether a node is enabled.
 * This looks for a 'status' property. If this exists, then returns 1 if
 * the status is 'ok' and 0 otherwise. If there is no status property,
 * it returns the default value.
 *
 * @param blob	FDT blob
 * @param node	node to examine
 * @param default_val	default value to return if no 'status' property exists
 * @return integer value 0/1, if found, or default_val if not
 */
static int get_is_enabled(const void *blob, int node, int default_val)
{
	const char *cell;

	cell = fdt_getprop(blob, node, "status", NULL);
	if (cell)
		return 0 == strcmp(cell, "ok");
	return default_val;
}

void fdt_decode_uart_calc_divisor(struct fdt_uart *uart)
{
	if (uart->multiplier && uart->baudrate)
		uart->divisor = (uart->clock_freq +
				(uart->baudrate * (uart->multiplier / 2))) /
			(uart->multiplier * uart->baudrate);
}

int fdt_decode_uart_console(const void *blob, struct fdt_uart *uart,
		int default_baudrate)
{
	int node;

	node = find_alias_node(blob, "console");
	if (node < 0)
		return node;
	uart->reg = get_addr(blob, node, "reg");
	uart->id = get_int(blob, node, "id", 0);
	uart->reg_shift = get_int(blob, node, "reg_shift", 2);
	uart->baudrate = get_int(blob, node, "baudrate", default_baudrate);
	uart->clock_freq = get_int(blob, node, "clock-frequency", -1);
	uart->multiplier = get_int(blob, node, "multiplier", 16);
	uart->divisor = get_int(blob, node, "divisor", -1);
	uart->enabled = get_is_enabled(blob, node, 1);
	uart->interrupt = get_int(blob, node, "interrupts", -1);
	uart->silent = get_int(blob, node, "silent", 0);
	uart->compat = fdt_decode_lookup(blob, node);

	/* Calculate divisor if required */
	if (uart->divisor == -1)
		fdt_decode_uart_calc_divisor(uart);
	return 0;
}

enum fdt_compat_id fdt_decode_lookup(const void *blob, int node)
{
	enum fdt_compat_id id;

	/* Search our drivers */
	for (id = COMPAT_UNKNOWN; id < COMPAT_COUNT; id++)
		if (0 == fdt_node_check_compatible(blob, node,
				compat_types[id].name))
			return id;
	return COMPAT_UNKNOWN;
}

int fdt_decode_get_spi_switch(const void *blob, struct fdt_spi_uart *config)
{
	int node, uart_node;
	const u32 *gpio;

	node = fdt_node_offset_by_compatible(blob, 0,
					     "nvidia,spi-uart-switch");
	if (node < 0)
		return node;

	uart_node = lookup_phandle(blob, node, "uart");
	if (uart_node < 0)
		return uart_node;
	config->port = get_int(blob, uart_node, "id", -1);
	if (config->port == -1)
		return -FDT_ERR_NOTFOUND;
	config->gpio = -1;
	config->regs = (NS16550_t)get_addr(blob, uart_node, "reg");
	gpio = fdt_getprop(blob, node, "gpios", NULL);
	if (gpio)
		config->gpio = fdt32_to_cpu(gpio[1]);
	return 0;
}

/**
 * Decode a list of GPIOs from an FDT. This creates a list of GPIOs with the
 * last one being GPIO_NONE
 *
 * @param blob		FDT blob to use
 * @param node		Node to look at
 * @param property	Node property name
 * @param gpio		Array of gpio elements to fill from FDT
 * @param max_count	Maximum number of elements allowed
 * @return 0 if ok, -FDT_ERR_BADLAYOUT if max_count would be exceeded
 */
static int decode_gpios(const void *blob, int node, const char *property,
		 struct fdt_gpio_state *gpio, int max_count)
{
	const u32 *cell;
	int len, i;

	cell = fdt_getprop(blob, node, property, &len);
	len /= sizeof(u32) * 3;		/* 3 cells per GPIO record */
	if (len >= max_count) {
		printf("FDT: decode_gpios: too many GPIOs\n");
		return -FDT_ERR_BADLAYOUT;
	}
	for (i = 0; i < len; i++, cell += 3) {
		gpio[i].gpio = fdt32_to_cpu(cell[1]);
		gpio[i].high = fdt32_to_cpu(cell[2]);
	}

	/* terminate the list */
	gpio[len].gpio = FDT_GPIO_NONE;
	return 0;
}

void fdt_setup_gpios(struct fdt_gpio_state *gpio_list)
{
	struct fdt_gpio_state *gpio;
	int i;

	for (i = 0, gpio = gpio_list; fdt_gpio_isvalid(gpio); i++, gpio++) {
		if (i > FDT_GPIO_MAX) {
			/* Something may have gone horribly wrong */
			printf("FDT: fdt_setup_gpios: too many GPIOs\n");
			return;
		}
		gpio_direction_output(gpio->gpio, 1);
		gpio_set_value(gpio->gpio, gpio->high);
	}
}

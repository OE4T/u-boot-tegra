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
	int node, i;

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

	/* Calculate divisor if required */
	if (uart->divisor == -1)
		fdt_decode_uart_calc_divisor(uart);
	return 0;
}

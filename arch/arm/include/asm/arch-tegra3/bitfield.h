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

#ifndef __TEGRA2_BITFIELD_H
#define __TEGRA2_BITFIELD_H

/*
 * Macros for working with bit fields. To use these, set things up like this:
 *
 * #define UART_PA_START 0x67000000	Physical address of UART
 * #define UART_FBCON_RANGE  5:3	Bit range for the FBCON field
 * enum {				An enum with allowed values
 *	UART_FBCON_OFF,
 *	UART_FBCON_ON,
 *	UART_FBCON_MULTI,
 *	UART_FBCON_SLAVE,
 * };
 * struct uart_ctlr *uart = (struct uart_ctlr *)UART_PA_START;
 *
 * This defines a bit field of 3 bits starting at bit 5 and extending down
 * to bit 3, i.e. 5:3
 *
 * Then:
 *    bf_unpack(UART_FBCON)
 *	- return the value of bits 5:3 (shifted down to bits 2:0)
 *
 *    bf_pack(UART_FBCON, 4)
 *	- return a word with that field set to 4 (so in this case (4 << 3))
 *
 *    bf_update(UART_FBCON, word, val)
 *	- update a field within word so that its value is val.
 *
 *    bf_enum_writel(UART_FBCON, MULTI, &uart->fbcon)
 *	- set the UART's FBCON field to MULTI
 *
 *
 * Why have bitfield macros?
 *   1. Reability
 *   2. Maintainability
 *   3. Less error prone
 *
 * For example, this:
 *
 *	int RegVal = 0;
 *	RegVal= readl(UsbBase+USB_SUSP_CTRL);
 *	RegVal |= Bit11;
 *	writel(RegVal, UsbBase+USB_SUSP_CTRL);
 *	if(UsbBase == NV_ADDRESS_MAP_USB3_BASE)
 *	{
 *      	RegVal = readl(UsbBase+USB_SUSP_CTRL);
 *		RegVal |= Bit12;
 *		writel(RegVal, UsbBase+USB_SUSP_CTRL);
 *	}
 *
 * becomes this:
 *
 *	bitfield_writel(UTMIP_RESET, 1, &usbctlr->susp_ctrl);
 *	if (id == PERIPH_ID_USB3)
 *		bitfield_writel(UTMIP_PHY_ENB, 1, &usbctlr->susp_ctrl);
 */

/* Returns the low bit of the bitfield */
#define BITFIELD_LOWBIT(range)	((0 ? range) & 0x1f)

/* Returns the high bit of the bitfield */
#define BITFIELD_HIGHBIT(range)	(1 ? range)

/* Returns the width of the bitfield (in bits) */
#define BITFIELD_WIDTH(range)	\
	(BITFIELD_HIGHBIT(range) - BITFIELD_LOWBIT(range) + 1)


/*
 * Returns the number of bits the bitfield needs to be shifted left to pack it.
 * This is just the same as the low bit.
 */
#define bf_shift(field)	BITFIELD_LOWBIT(field ## _RANGE)

/* Returns the unshifted mask for the field (i.e. LSB of mask is bit 0) */
#define bf_rawmask(field) (0xfffffffful >> \
		(32 - BITFIELD_WIDTH(field ## _RANGE)))

/* Returns the mask for a field. Clear these bits to zero the field */
#define bf_mask(field) \
		(bf_rawmask(field) << (bf_shift(field)))

/* Unpacks and returns a value extracted from a field */
#define bf_unpack(field, word) \
	(((unsigned)(word) >> bf_shift(field)) & bf_rawmask(field))

/*
 * Packs a value into a field - this masks the value to ensure it does not
 * overflow into another field.
 */
#define bf_pack(field, value) \
	((((unsigned)(value)) & bf_rawmask(field)) \
		<< bf_shift(field))

/* Sets the value of a field in a word to the given value */
#define bf_update(field, word, value)		\
	((word) = ((word) & ~bf_mask(field)) |	\
		bf_pack(field, value))

/*
 * Sets the value of a field in a register to the given value using
 * readl/writel
 */
#define bf_writel(field, value, reg) ({		\
	u32 *__reg = (u32 *)(reg);			\
	u32 __oldval = readl(__reg); 			\
	bf_update(field, __oldval, value);		\
	writel(__oldval, __reg);				\
	})

/* Unpacks a field from a register using readl */
#define bf_readl(field, reg) 			\
	bf_unpack(field, readl(reg))

/*
 * Clears a field in a register using writel - like
 * bf_writel(field, 0, reg)
 */
#define bf_clearl(field, reg) bf_writel(field, 0, reg)

/*
 * Sets the value of a field in a register to the given enum.
 *
 * The enum should have the field as a prefix.
 */
#define bf_enum(field, _enum) bf_pack(field, field ## _ ## _enum)

/*
 * Sets the value of a field in a register to the given enum.
 *
 * The enum should have the field as a prefix.
 */
#define bf_enum_writel(field, _enum, reg) \
		bf_writel(field, field ## _ ## _enum, reg)

/*
 * Return a word with the bitfield set to all ones.
 */
#define bf_ones(field) bf_mask(field)

#endif

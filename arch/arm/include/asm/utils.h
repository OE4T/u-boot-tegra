/*
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *
 * Aneesh V <aneesh@ti.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef	_UTILS_H_
#define	_UTILS_H_

/* extract a bit field from a bit vector */
#define get_bit_field(nr, start, mask)\
	(((nr) & (mask)) >> (start))

/* Set a field in a bit vector */
#define set_bit_field(nr, start, mask, val)\
	do { \
		(nr) = ((nr) & ~(mask)) | (((val) << (start)) & (mask));\
	} while (0);

/*
 * Utility macro for read-modify-write of a hardware register
 *	addr - address of the register
 *	shift - starting bit position of the field to be modified
 *	msk - mask for the field
 *	val - value to be shifted masked and written to the field
 */
#define modify_reg_32(addr, shift, msk, val) \
	do {\
		writel(((readl(addr) & ~(msk))|(((val) << (shift)) & (msk))),\
		       (addr));\
	} while (0);

static inline s32 log_2_n_round_up(u32 n)
{
	s32 log2n = -1;
	u32 temp = n;

	while (temp) {
		log2n++;
		temp >>= 1;
	}

	if (n & (n - 1))
		return log2n + 1; /* not power of 2 - round up */
	else
		return log2n; /* power of 2 */
}

static inline s32 log_2_n_round_down(u32 n)
{
	s32 log2n = -1;
	u32 temp = n;

	while (temp) {
		log2n++;
		temp >>= 1;
	}

	return log2n;
}

#endif /* _OMAP_COMMON_H_ */

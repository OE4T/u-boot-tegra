/*
 *  (C) Copyright 2010 - 2013
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

#ifndef _FUSE_H_
#define _FUSE_H_

/* FUSE registers */
struct fuse_regs {
	u32 reserved0[9];		/* 0x00 - 0x20: */
	u32 fuse_bypass;		/* 0x24: FUSE_FUSEBYPASS */
	u32 private_key_disable;	/* 0x28: FUSE_PRIVATEKEYDISABLE */
	u32 disable_reg_program;	/* 0x2C:  FUSE_DISABLEREGPROGRAM */
	u32 write_access_sw;		/* 0x30:  FUSE_WRITE_ACCESS_SW */
	u32 reserved01[51];		/* 0x34 - 0xFC: */
	u32 production_mode;		/* 0x100: FUSE_PRODUCTION_MODE */
	u32 reserved1[3];		/* 0x104 - 0x10c: */
	u32 sku_info;			/* 0x110 */
	u32 reserved2[13];		/* 0x114 - 0x144: */
	u32 fa;				/* 0x148: FUSE_FA */
	u32 reserved3[21];		/* 0x14C - 0x19C: */
	u32 security_mode;		/* 0x1A0 - FUSE_SECURITY_MODE */
	u32 reserved4[55];		/* 0x1A4 - 0x27C */
	u32 fuse_spare_bit_0;		/* 0x280 - FUSE_SPARE_BIT_0 */
	u32 fuse_spare_bit_1;		/* 0x284 - FUSE_SPARE_BIT_1 */
	u32 fuse_spare_bit_2;		/* 0x288 - FUSE_SPARE_BIT_2 */
	u32 fuse_spare_bit_3;		/* 0x28C - FUSE_SPARE_BIT_3 */
	u32 fuse_spare_bit_4;		/* 0x290 - FUSE_SPARE_BIT_4 */
	u32 fuse_spare_bit_5;		/* 0x294 - FUSE_SPARE_BIT_5 */
	u32 fuse_spare_bit_6;		/* 0x298 - FUSE_SPARE_BIT_6 */
	u32 fuse_spare_bit_7;		/* 0x29C - FUSE_SPARE_BIT_7 */
	u32 fuse_spare_bit_8;		/* 0x2A0 - FUSE_SPARE_BIT_8 */
	u32 fuse_spare_bit_9;		/* 0x2A4 - FUSE_SPARE_BIT_9 */
	u32 fuse_spare_bit_10;		/* 0x2A8 - FUSE_SPARE_BIT_10 */
	u32 fuse_spare_bit_11;		/* 0x2AC - FUSE_SPARE_BIT_11 */
};

#endif	/* ifndef _FUSE_H_ */

/*
 *  (C) Copyright 2010 - 2011
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

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

/* Lengths, in bytes */
#define KEY_LENGTH (128/8)

#define ICEIL(a, b) (((a) + (b) - 1)/(b))

#define AES_CMAC_CONST_RB 0x87  /* from RFC 4493, Figure 2.2 */

enum security_mode {
	SECURITY_MODE_NONE = 0,
	SECURITY_MODE_PLAINTEXT,
	SECURITY_MODE_CHECKSUM,
	SECURITY_MODE_ENCRYPTED,
	SECURITY_MODE_MAX,
	SECURITY_MODE_FORCE32 = 0x7fffffff
};

int sign_data_block(u8 *source, u32 length, u8 *signature);

#endif /* #ifndef _CRYPTO_H_ */

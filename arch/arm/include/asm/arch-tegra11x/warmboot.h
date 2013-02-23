/*
 * (C) Copyright 2010, 2011
 * NVIDIA Corporation <www.nvidia.com>
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

#ifndef _WARM_BOOT_H_
#define _WARM_BOOT_H_

/* bit fields definitions for APB_MISC_GP_HIDREV register */
#define HIDREV_MINOPREV_RANGE			19 : 16
#define HIDREV_CHIPID_RANGE			15 : 8
#define HIDREV_MAJORPREV_RANGE			7 : 4
#define HIDREV_HIDFAM_RANGE			3 : 0

/* CHIPID field returned from APB_MISC_GP_HIDREV register */
#define CHIPID_TEGRA114				0x35

#define STRAP_OPT_A_RAM_CODE_RANGE		7 : 4

/* Defines the supported operating modes */
enum fuse_operating_mode {
	MODE_PREPRODUCTION = 2,
	MODE_PRODUCTION = 3,
	MODE_UNDEFINED,
};

/* Defines the CMAC-AES-128 hash length in 32 bit words. (128 bits = 4 words) */
enum {
	HASH_LENGTH = 4
};

/* Defines the storage for a hash value (128 bits) */
struct hash {
	u32 hash[HASH_LENGTH];
};

/*
 * Defines the code header information for the boot rom.
 *
 * The code immediately follows the code header.
 *
 * Note that the code header needs to be 16 bytes aligned to preserve
 * the alignment of relevant data for hash and decryption computations without
 * requiring extra copies to temporary memory areas.
 */
struct wb_header {
	u32 length_in_secure;	/* length of the code header */
	u32 reserved[3];
	u32 modules[64];	/* RsaKeyModulus */
	union {
		struct hash hash;  /* hash of header+code, starts next field */
		u32 signature[64]; /* The RSA-PSS signature*/
	} hash_signature;
	struct hash random_aes_block;	/* a data block to aid security. */
	u32 length_secure;	/* length of the code header */
	u32 destination;	/* destination address to put the wb code */
	u32 entry_point;	/* execution address of the wb code */
	u32 code_length;	/* length of the code */
};

void warmboot_save_sdram_params(void);
int warmboot_prepare_code(u32 seg_address, u32 seg_length);
int sign_data_block(u8 *source, u32 length, u8 *signature);
void wb_start(void);	/* Start of WB assembly code */
void wb_end(void);	/* End of WB assembly code */

#endif

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

#include <common.h>
#include <asm/errno.h>
#include "crypto.h"
#include "aes_ref.h"

static u8 zero_key[16];

#ifdef DEBUG
static void debug_print_vector(char *name, u32 num_bytes, u8 *data)
{
	u32 i;

	printf("%s [%d] @0x%08x", name, num_bytes, (u32)data);
	for (i = 0; i < num_bytes; i++) {
		if (i % 16 == 0)
			printf(" = ");
		printf("%02x", data[i]);
		if ((i+1) % 16 != 0)
			printf(" ");
	}
	printf("\n");
}
#else
#define debug_print_vector(name, num_bytes, data)
#endif

static void apply_cbc_chain_data(u8 *cbc_chain_data, u8 *src, u8 *dst)
{
	int i;

	for (i = 0; i < 16; i++)
		*dst++ = *src++ ^ *cbc_chain_data++;
}

static int determine_crypto_ops(enum security_mode security, int *encrypt,
				int *sign)
{
	int err = 0;

	switch (security) {
	case SECURITY_MODE_PLAINTEXT:
		*encrypt = 0;
		*sign = 0;
		break;

	case SECURITY_MODE_CHECKSUM:
		*encrypt = 0;
		*sign = 1;
		break;

	case SECURITY_MODE_ENCRYPTED:
		*encrypt = 1;
		*sign = 1;
		break;

	default:
		printf("determine_crypto_ops: security mode not set.\n");
		err = -EINVAL;
	}

	return err;
}

static void generate_key_schedule(u8 *key, u8 *key_schedule, int encrypt_data)
{
	/* Expand the key to produce a key schedule. */
	if (encrypt_data)
		/* Expand the provided key. */
		aes_expand_key(key, key_schedule);
	else
		/*
		 * The only need for a key is for signing/checksum purposes, so
		 * expand a key of 0's.
		 */
		aes_expand_key(zero_key, key_schedule);
}

static void encrypt_object(u8 *key_schedule, u8 *src, u8 *dst,
			   u32 num_aes_blocks)
{
	u32 i;
	u8 *cbc_chain_data;
	u8  tmp_data[KEY_LENGTH];

	cbc_chain_data = zero_key;	/* Convenient array of 0's for IV */

	for (i = 0; i < num_aes_blocks; i++) {
		debug("encrypt_object: block %d of %d\n", i, num_aes_blocks);
		debug_print_vector("AES Src", KEY_LENGTH, src);

		/* Apply the chain data */
		apply_cbc_chain_data(cbc_chain_data, src, tmp_data);
		debug_print_vector("AES Xor", KEY_LENGTH, tmp_data);

		/* encrypt the AES block */
		aes_encrypt(tmp_data, key_schedule, dst);
		debug_print_vector("AES Dst", KEY_LENGTH, dst);

		/* Update pointers for next loop. */
		cbc_chain_data = dst;
		src += KEY_LENGTH;
		dst += KEY_LENGTH;
	}
}

static void left_shift_vector(u8 *in, u8 *out, u32 size)
{
	s32 i;
	u8 carry = 0;

	for (i = size - 1; i >= 0; i--) {
		out[i] = (in[i] << 1) | carry;
		carry = in[i] >> 7;	/* get most significant bit */
	}
}

static void sign_object(u8 *key, u8 *key_schedule, u8 *src, u8 *dst,
			u32 num_aes_blocks)
{
	u32 i;
	u8 *cbc_chain_data;

	u8 L[KEY_LENGTH];
	u8 K1[KEY_LENGTH];
	u8 tmp_data[KEY_LENGTH];

	cbc_chain_data = zero_key;	/* Convenient array of 0's for IV */

	/* compute K1 constant needed by AES-CMAC calculation */
	for (i = 0; i < KEY_LENGTH; i++)
		tmp_data[i] = 0;

	encrypt_object(key_schedule, tmp_data, L, 1);
	debug_print_vector("AES(key, nonce)", KEY_LENGTH, L);

	left_shift_vector(L, K1, sizeof(L));
	debug_print_vector("L", KEY_LENGTH, L);

	if ((L[0] >> 7) != 0) /* get MSB of L */
		K1[KEY_LENGTH-1] ^= AES_CMAC_CONST_RB;
	debug_print_vector("K1", KEY_LENGTH, K1);

	/* compute the AES-CMAC value */
	for (i = 0; i < num_aes_blocks; i++) {
		/* Apply the chain data */
		apply_cbc_chain_data(cbc_chain_data, src, tmp_data);

		/* for the final block, XOR K1 into the IV */
		if (i == num_aes_blocks-1)
			apply_cbc_chain_data(tmp_data, K1, tmp_data);

		/* encrypt the AES block */
		aes_encrypt(tmp_data, key_schedule, (u8 *)dst);

		debug("sign_obj: block %d of %d\n", i, num_aes_blocks);
		debug_print_vector("AES-CMAC Src", KEY_LENGTH, src);
		debug_print_vector("AES-CMAC Xor", KEY_LENGTH, tmp_data);
		debug_print_vector("AES-CMAC Dst", KEY_LENGTH, (u8 *)dst);

		/* Update pointers for next loop. */
		cbc_chain_data = (u8 *)dst;
		src += KEY_LENGTH;
	}

	debug_print_vector("AES-CMAC Hash", KEY_LENGTH, (u8 *)dst);
}

static int encrypt_and_sign(u8 *key, enum security_mode security, u8 *src,
			    u32 length, u8 *sig_dst)
{
	int encrypt_data;
	int sign_data;
	u32 num_aes_blocks;
	u8 key_schedule[4 * AES_STATECOLS * (AES_ROUNDS+1)];
	int err;

	err = determine_crypto_ops(security, &encrypt_data, &sign_data);
	if (err)
		return err;

	debug("encrypt_and_sign: Length = %d\n", length);
	debug_print_vector("AES key", KEY_LENGTH, key);

	generate_key_schedule(key, key_schedule, encrypt_data);

	num_aes_blocks = ICEIL(length, KEY_LENGTH);

	if (encrypt_data) {
		debug("encrypt_and_sign: begin encryption\n");

		/* Perform this in place, resulting in src being encrypted. */
		encrypt_object(key_schedule, src, src, num_aes_blocks);

		debug("encrypt_and_sign: end encryption\n");
	}

	if (sign_data) {
		debug("encrypt_and_sign: begin signing\n");

		/* encrypt the data, overwriting the result in signature. */
		sign_object(key, key_schedule, src, sig_dst, num_aes_blocks);

		debug("encrypt_and_sign: end signing\n");
	}

	return err;
}

int sign_data_block(u8 *source, u32 length, u8 *signature)
{
	return encrypt_and_sign(zero_key, SECURITY_MODE_CHECKSUM, source,
				length, signature);
}

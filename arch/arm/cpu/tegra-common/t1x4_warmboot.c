/*
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch-tegra/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/tegra.h>
#include <asm/arch-tegra/fuse.h>
#include <asm/arch/gp_padctrl.h>
#include <asm/arch-tegra/warmboot.h>
#include <asm/arch-tegra/sdram_param.h>
#include <asm/arch/wb_param.h>

/*
 * do_encode():
 *
 * sdram->dst.dst_bit = (sdram->src.op1_bits > sdram->src.op2_bits);
 *   where src, dst, dst_bit, op1_bits, op2_bits are specified in encode_fields.
 *
 * @param encode	pointer to struct encode_fields to specify all needed
 *			fields to do encoding
 * @param sdram		pointer to struct sdram_params for SDRAM parameters
 */
void do_encode(struct encode_fields *encode, struct sdram_params *sdram)
{
	u32 val, op1, op2;

	val = readl((char *)sdram + encode->src_offset);
	debug("%s: sdram[%#x] => %#x\n", __func__, encode->src_offset, val);
	op1 = val >> encode->op1_shift;
	op1 &= encode->op1_mask;
	op2 = val >> encode->op2_shift;
	op2 &= encode->op2_mask;

	val = readl((char *)sdram + encode->dst_offset);
	val &= ~(1 << encode->dst_shift);
	if (op1 > op2)
		val |= (1 << encode->dst_shift);
	debug("%s: sdram[%#x] <= %#x\n", __func__, encode->dst_offset, val);
	writel(val, (char *)sdram + encode->dst_offset);
}

/*
 * do_encode_list(): call do_encode() for each encode in encode_list
 *
 * @param encode_list	pointer to struct encode_fields which contains list of
 *			pointers of encode
 * @param num_list	# of pointers of struct encode_fields in encode_list
 * @param sdram		pointer to struct sdram_params for SDRAM parameters
 */
void do_encode_list(struct encode_fields *encode_list,  u32 num_list,
		       struct sdram_params *sdram)
{
	struct encode_fields *encode;
	u32 i;

	for (i = 0, encode = encode_list; i < num_list; ++i, ++encode)
		do_encode(encode, sdram);
}

/*
 * do_pack():
 *
 * PMC.dst_offset.dst_bits = src_type.src_offset.src_bits;
 *
 *   src_type, src_offset, src_bits, dst_offset, and dst_bits are specified
 *   in pack_fields.
 *
 *   src_type can be SDRAM, PLLM registers, or CONSTant.
 *   The destination is always PMC registers.
 *
 * @param pack		pointer to struct pack_fields to specify all needed
 *			fields to do packing
 * @param sdram		pointer to struct sdram_params for SDRAM parameters
 */
void do_pack(struct pack_fields *pack, struct sdram_params *sdram)
{
	u32 val, offset, type, reg;
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;

	offset = pack->src_offset;
	type = pack->src_type;

	switch (type) {
	case TYPE_SDRAM:
		val = readl((char *)sdram + offset);
		break;
	case TYPE_PLLM:
		val = readl((char *)&clkrst->crc_pll[CLOCK_ID_MEMORY]
			+ offset);
		break;
	case TYPE_CONST:
		val = offset;
		break;
	default:
		debug("src_type (%u) is not supported\n", type);
		return;
	}

	val >>= pack->src_shift;
	val &= pack->src_mask;
	val <<= pack->dst_shift;

	reg = readl(NV_PA_PMC_BASE + pack->dst_offset);
	reg &= ~(pack->dst_mask << pack->dst_shift);
	reg |= val;
	writel(reg, NV_PA_PMC_BASE + pack->dst_offset);
}

/*
 * do_pack_list() : call do_pack() for each pack in pack_list.
 *
 * @param pack_list	pointer to struct pack_fields which contains list of
 *			pointers of pack
 * @param num_list	# of pointers of struct pack_fields in pack_list
 * @param sdram		pointer to struct sdram_params for SDRAM parameters
 */
void do_pack_list(struct pack_fields *pack_list, u32 num_list,
		   struct sdram_params *sdram)
{
	struct pack_fields *pack;
	u32 i;

	for (pack = pack_list, i = 0; i < num_list; ++i, ++pack)
		do_pack(pack, sdram);
}

/*
 * t1x4_wb_save_sdram_params():
 *
 * save sdram parameters to scratch registers so sdram parameters can be
 * restored when system resumes from LP0.
 *
 * common routine for both T114 and T124 SOCs, but based on each SOC's
 * encode_list and pack_lists (defined in wb_param.h)
 *
 * @param sdram		pointer to struct sdram_params for SDRAM parameters
 * @return		0 always
 */
int t1x4_wb_save_sdram_params(struct sdram_params *sdram)
{
	/* encode BIT6_GT_BIT7 bits in sdram.swizzle_rank_byte_encode */
	do_encode_list(encode_list, ARRAY_SIZE(encode_list), sdram);

	do_pack_list(pack_list_1, ARRAY_SIZE(pack_list_1), sdram);

	if (sdram->memory_type == MEMORY_TYPE_LPDDR2)
		do_pack_list(pack_list_lpddr2, ARRAY_SIZE(pack_list_lpddr2),
			     sdram);
	else if (sdram->memory_type == MEMORY_TYPE_DDR3)
		do_pack_list(pack_list_ddr3, ARRAY_SIZE(pack_list_ddr3),
			     sdram);

	do_pack_list(pack_list_2, ARRAY_SIZE(pack_list_2), sdram);

	return 0;
}

/*
 * NOTE: If more than one of the following is enabled, only one of them will
 *	 actually be used. RANDOM takes precedence over PATTERN and ZERO, and
 *	 PATTERN takes precedence overy ZERO.
 *
 *	 RANDOM_AES_BLOCK_IS_PATTERN is to define a 32-bit PATTERN.
 */
#undef RANDOM_AES_BLOCK_IS_RANDOM	/* to randomize the header */
#undef RANDOM_AES_BLOCK_IS_PATTERN	/* to patternize the header */
#define RANDOM_AES_BLOCK_IS_ZERO	/* to clear the header */

static u32 get_major_version(void)
{
	u32 major_id;
	struct apb_misc_gp_ctlr *gp =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;

	major_id = (readl(&gp->hidrev) & HIDREV_MAJORPREV_MASK) >>
			HIDREV_MAJORPREV_SHIFT;
	return major_id;
}

static int is_production_mode_fuse_set(struct fuse_regs *fuse)
{
	return readl(&fuse->production_mode);
}

static int is_odm_production_mode_fuse_set(struct fuse_regs *fuse)
{
	return readl(&fuse->security_mode);
}

static int is_failure_analysis_mode(struct fuse_regs *fuse)
{
	return readl(&fuse->fa);
}

static int is_odm_production_mode(void)
{
	struct fuse_regs *fuse = (struct fuse_regs *)NV_PA_FUSE_BASE;

	if (!is_failure_analysis_mode(fuse) &&
	    is_odm_production_mode_fuse_set(fuse))
		return 1;
	else
		return 0;
}

static int is_production_mode(void)
{
	struct fuse_regs *fuse = (struct fuse_regs *)NV_PA_FUSE_BASE;

	if (get_major_version() == 0)
		return 1;

	if (!is_failure_analysis_mode(fuse) &&
	    is_production_mode_fuse_set(fuse) &&
	    !is_odm_production_mode_fuse_set(fuse))
		return 1;
	else
		return 0;
}

static enum fuse_operating_mode fuse_get_operation_mode(u32 tegra_id)
{
	u32 chip_id;
	struct apb_misc_gp_ctlr *gp =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;

	chip_id = readl(&gp->hidrev);
	chip_id = (chip_id & HIDREV_CHIPID_MASK) >> HIDREV_CHIPID_SHIFT;
	if (chip_id == tegra_id) {
		if (is_odm_production_mode()) {
			printf("!! odm_production_mode is not supported !!\n");
			return MODE_UNDEFINED;
		} else {
			if (is_production_mode())
				return MODE_PRODUCTION;
			else
				return MODE_PREPRODUCTION;
		}
	}
	return MODE_UNDEFINED;
}

#if defined(RANDOM_AES_BLOCK_IS_RANDOM)
/* Currently, this routine returns a 32-bit all 0 seed. */
static u32 query_random_seed(void)
{
	return 0;
}
#endif

static void determine_crypto_options(u32 tegra_id, int *is_encrypted,
				     int *is_signed, int *use_zero_key)
{
	switch (fuse_get_operation_mode(tegra_id)) {
	case MODE_PREPRODUCTION:
	case MODE_PRODUCTION:
		*is_encrypted = 0;
		*is_signed = 1;
		*use_zero_key = 1;
		break;
	case MODE_UNDEFINED:
	default:
		*is_encrypted = 0;
		*is_signed = 0;
		*use_zero_key  = 0;
		break;
	}
}

static int sign_wb_code(u32 start, u32 length, int use_zero_key)
{
	int err;
	u8 *source;		/* Pointer to source */
	u8 *hash;

	/* Calculate AES block parameters. */
	source = (u8 *)(start + offsetof(struct wb_header, random_aes_block));
	length -= offsetof(struct wb_header, random_aes_block);
	hash = (u8 *)(start + offsetof(struct wb_header, hash_signature.hash));
	err = sign_data_block(source, length, hash);

	return err;
}

/*
 * t1x4_wb_prepare_code():
 *
 * prepare WB code, which will be executed by AVP when system resumes from LP0.
 *
 * common routine for both T114 and T124 SOCs, except the caller must pass the
 * T114 or T124 chip ID for checking.
 *
 * @param tegra_id	SOC ID: CHIPID_TEGRA114 or CHIPID_TEGRA124
 * @param seg_address	address where the WB code to be stored
 * @param seg_length	# of bytes allocated to store the WB code
 * @return		0 if success
 *			!0 (error code) if failed
 */
int t1x4_wb_prepare_code(u32 tegra_id, u32 seg_address, u32 seg_length)
{
	int err = 0;
	u32 length;			/* length of the signed/encrypt code */
	struct wb_header *dst_header;	/* Pointer to dest WB header */
	int is_encrypted;		/* Segment is encrypted */
	int is_signed;			/* Segment is signed */
	int use_zero_key;		/* Use key of all zeros */

	/* Determine crypto options. */
	determine_crypto_options(tegra_id, &is_encrypted, &is_signed,
				 &use_zero_key);

	/* Get the actual code limits. */
	length = roundup(((u32)wb_end - (u32)wb_start), 16);

	/*
	 * The region specified by seg_address must not be in IRAM and must be
	 * nonzero in length.
	 */
	if ((seg_length == 0) || (seg_address == 0) ||
	    ((seg_address >= NV_PA_BASE_SRAM) &&
	     (seg_address < (NV_PA_BASE_SRAM + NV_PA_BASE_SRAM_SIZE)))) {
		err = -EFAULT;
		goto fail;
	}

	/* Things must be 16-byte aligned. */
	if ((seg_length & 0xF) || (seg_address & 0xF)) {
		err = -EINVAL;
		goto fail;
	}

	/* Will the code fit? (destination includes wb_header + wb code) */
	if (seg_length < (length + sizeof(struct wb_header))) {
		err = -EINVAL;
		goto fail;
	}

	dst_header = (struct wb_header *)seg_address;
	memset((char *)dst_header, 0, sizeof(struct wb_header));

	/* Populate the random_aes_block as requested. */
	{
		u32 *aes_block = (u32 *)&(dst_header->random_aes_block);
		u32 *end = (u32 *)(((u32)aes_block) +
				   sizeof(dst_header->random_aes_block));

		do {
#if defined(RANDOM_AES_BLOCK_IS_RANDOM)
			*aes_block++ = query_random_seed();
#elif defined(RANDOM_AES_BLOCK_IS_PATTERN)
			*aes_block++ = RANDOM_AES_BLOCK_IS_PATTERN;
#elif defined(RANDOM_AES_BLOCK_IS_ZERO)
			*aes_block++ = 0;
#else
			printf("None of RANDOM_AES_BLOCK_IS_XXX is defined; ");
			printf("Default to pattern 0.\n");
			*aes_block++ = 0;
#endif
		} while (aes_block < end);
	}

	/* Populate the header. */
	dst_header->length_insecure = length + sizeof(struct wb_header);
	dst_header->length_secure = length + sizeof(struct wb_header);
	dst_header->destination = NV_WB_RUN_ADDRESS;
	dst_header->entry_point = NV_WB_RUN_ADDRESS;
	dst_header->code_length = length;

	if (is_encrypted) {
		printf("!!!! Encryption is not supported !!!!\n");
		dst_header->length_insecure = 0;
		err = -EACCES;
		goto fail;
	} else {
		/* copy the wb code directly following dst_header. */
		memcpy((char *)(dst_header+1), (char *)wb_start, length);
	}

	if (is_signed)
		err = sign_wb_code(seg_address, dst_header->length_insecure,
				   use_zero_key);

fail:
	if (err)
		printf("WB code not copied to LP0 location! (error=%d)\n", err);

	return err;
}

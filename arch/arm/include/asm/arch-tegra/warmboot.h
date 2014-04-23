/*
 * (C) Copyright 2010, 2011
 * NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _WARM_BOOT_H_
#define _WARM_BOOT_H_

#include <asm/arch-tegra/sdram_param.h>

#define STRAP_OPT_A_RAM_CODE_SHIFT	4
#define STRAP_OPT_A_RAM_CODE_MASK	(0xf << STRAP_OPT_A_RAM_CODE_SHIFT)

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
	u32 length_insecure;	/* length of the code header */
	u32 reserved[3];
#ifdef CONFIG_TEGRA124
	u32 modules[64];	/* RsaKeyModulus */
	struct {
		struct hash hash;  /* hash of header+code, starts next field */
		u32 signature[64]; /* The RSA-PSS signature*/
	} hash_signature;
#else
	struct hash hash;	/* hash of header+code, starts next field*/
#endif
	struct hash random_aes_block;	/* a data block to aid security. */
	u32 length_secure;	/* length of the code header */
	u32 destination;	/* destination address to put the wb code */
	u32 entry_point;	/* execution address of the wb code */
	u32 code_length;	/* length of the code */
};

/*
 * The warm boot code needs direct access to these registers since it runs in
 * SRAM and cannot call other U-Boot code.
 */
union osc_ctrl_reg {
	struct {
		u32 xoe:1;
		u32 xobp:1;
		u32 reserved0:2;
		u32 xofs:6;
		u32 reserved1:2;
		u32 xods:5;
		u32 reserved2:3;
		u32 oscfi_spare:8;
		u32 pll_ref_div:2;
		u32 osc_freq:2;
	};
	u32 word;
};

union pllx_base_reg {
	struct {
		u32 divm:5;
		u32 reserved0:3;
		u32 divn:10;
		u32 reserved1:2;
		u32 divp:3;
		u32 reserved2:4;
		u32 lock:1;
		u32 reserved3:1;
		u32 ref_dis:1;
		u32 enable:1;
		u32 bypass:1;
	};
	u32 word;
};

union pllx_misc_reg {
	struct {
		u32 vcocon:4;
		u32 lfcon:4;
		u32 cpcon:4;
		u32 lock_sel:6;
		u32 reserved0:1;
		u32 lock_enable:1;
		u32 reserved1:1;
		u32 dccon:1;
		u32 pts:2;
		u32 reserved2:6;
		u32 out1_div_byp:1;
		u32 out1_inv_clk:1;
	};
	u32 word;
};

/*
 * TODO: This register is not documented in the TRM yet. We could move this
 * into the EMC and give it a proper interface, but not while it is
 * undocumented.
 */
union scratch3_reg {
	struct {
		u32 pllx_base_divm:5;
		u32 pllx_base_divn:10;
		u32 pllx_base_divp:3;
		u32 pllx_misc_lfcon:4;
		u32 pllx_misc_cpcon:4;
	};
	u32 word;
};

enum field_type {
	TYPE_SDRAM,
	TYPE_PLLM,
	TYPE_CONST,
	TYPE_COUNT,
};

#define sdram_offset(member) offsetof(struct sdram_params, member)
#define pmc_offset(member) offsetof(struct pmc_ctlr, member)
#define pllm_offset(member) offsetof(struct clk_pllm, member)

struct encode_fields {
	u32 src_offset;
	u32 op1_mask;
	u32 op1_shift;
	u32 op2_mask;
	u32 op2_shift;
	u32 dst_offset;
	u32 dst_shift;
};

/*
 * encode: set dst_off.dst_bit to 1 if (src_off.op1_bits > src_off.op2_bits)
 *         else set dst_off.dst_bit to 0.
 */
#define encode(src_off, op1_bits, op2_bits, dst_off, dst_bit)		\
	{								\
		.src_offset = src_off,					\
		.op1_mask = 0xfffffffful >>				\
			(31 - ((1 ? op1_bits) - (0 ? op1_bits))),	\
		.op1_shift = 0 ? op1_bits,				\
		.op2_mask = 0xfffffffful >>				\
			(31 - ((1 ? op2_bits) - (0 ? op2_bits))),	\
		.op2_shift = 0 ? op2_bits,				\
		.dst_offset = dst_off,					\
		.dst_shift = dst_bit,					\
	}

struct pack_fields {
	enum field_type src_type;
	u32 src_offset;		/* for TYPE_CONST, this field is a constant */
	u32 src_mask;
	u32 src_shift;
	enum field_type dst_type;
	u32 dst_offset;
	u32 dst_mask;
	u32 dst_shift;
};

/*
 * pack: a macro to copy from bits in src to bits in dst.
 *
 * For example,
 *   pack(TYPE_SDRAM, sdram_offset(emc_clock_source), 7:0,
 *	  pmc_offset(pmc_scratch6), 15:8)
 *     is to:
 *         copy bits 7:0 of sdram_offset.emc_clock_source to
 *         bits 15:8 of pmc_scratch6 register.
 *
 *   The first parameter of pack determines the type of src:
 *     TYPE_SDRAM: src is of SDRAM parameter,
 *     TYPE_PLLM:  src is of PLLM,
 *     TYPE_CONST: src is a constant.
 */
#define pack(_src_type, src_off, src_bits, dst_off, dst_bits)		\
	{								\
		.src_type = _src_type,					\
		.src_offset = src_off,					\
		.src_mask = 0xfffffffful >>				\
			(31 - ((1 ? src_bits) - (0 ? src_bits))),	\
		.src_shift = 0 ? src_bits,				\
		.dst_offset = dst_off,					\
		.dst_mask = 0xfffffffful >>				\
			(31 - ((1 ? dst_bits) - (0 ? dst_bits))),	\
		.dst_shift = 0 ? dst_bits,				\
	}


/**
 * Save warmboot memory settings for a later resume
 *
 * @return 0 if ok, -1 on error
 */
int warmboot_save_sdram_params(void);

int warmboot_prepare_code(u32 seg_address, u32 seg_length);
int sign_data_block(u8 *source, u32 length, u8 *signature);
void wb_start(void);	/* Start of WB assembly code */
void wb_end(void);	/* End of WB assembly code */

/* Common routines for T114 and T124 SOCs */

/*
 * t1x4_wb_save_sdram_params():
 * save sdram parameters to scratch registers so sdram parameters can be
 * restored when system resumes from LP0
 */
int t1x4_wb_save_sdram_params(struct sdram_params *sdram);

/*
 * t1x4_wb_prepare_code():
 * prepare WB code, which will be executed by AVP when system resumes from LP0
 */
int t1x4_wb_prepare_code(u32 tegra_id, u32 seg_address, u32 seg_length);
#endif

/*
 * (C) Copyright 2010 - 2011
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

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/pmc.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/tegra.h>
#include <asm/arch/fuse.h>
#include <asm/arch/emc.h>
#include <asm/arch/gp_padctrl.h>
#include <asm/arch/warmboot.h>
#include <asm/arch/sdram_param.h>
#include "ap20.h"

#define BCT_OFFSET		0x100		/* BCT starts at 0x100 */
#define BCT_SDRAM_PARAMS_OFFSET	(BCT_OFFSET + 0x88)
#define SDRAM_PARAMS_BASE	(AP20_BASE_PA_SRAM + BCT_SDRAM_PARAMS_OFFSET)

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

union pll_base_reg {
	struct {
		u32 pll_divm:5;
		u32 reserved0:3;
		u32 pll_divn:10;
		u32 reserved1:2;
		u32 pll_divp:3;
		u32 reserved2:4;
		u32 pll_lock:1;
		u32 reserved3:1;
		u32 pll_ref_dis:1;
		u32 pll_enable:1;
		u32 pll_bypass:1;
	};
	u32 word;
};

union pll_misc_reg {
	struct {
		u32 pll_vcocon:4;
		u32 pll_lfcon:4;
		u32 pll_cpcon:4;
		u32 pll_lock_sel:6;
		u32 reserved0:1;
		u32 pll_lock_enable:1;
		u32 reserved1:1;
		u32 pll_dccon:1;
		u32 pll_pts:2;
		u32 reserved2:6;
		u32 pll_out1_div_byp:1;
		u32 pll_out1_inv_clk:1;
	};
	u32 word;
};

union xm2cfga_reg {
	struct {
		u32 reserved0:2;
		u32 hsm_en:1;
		u32 reserved1:2;
		u32 preemp_en:1;
		u32 vref_en:1;
		u32 reserved2:5;
		u32 cal_drvdn:5;
		u32 reserved3:3;
		u32 cal_drvup:5;
		u32 reserved4:3;
		u32 cal_drvdn_slwr:2;
		u32 cal_drvup_slwf:2;
	};
	u32 word;
};

union xm2cfgd_reg {
	struct {
		u32 reserved0:2;
		u32 hsm_en:1;
		u32 schmt_en:1;
		u32 lpmd:2;
		u32 vref_en:1;
		u32 reserved1:5;
		u32 cal_drvdn:5;
		u32 reserved2:3;
		u32 cal_drvup:5;
		u32 reserved3:3;
		u32 cal_drvdn_slwr:2;
		u32 cal_drvup_slwf:2;
	};
	u32 word;
};

union fbio_spare_reg {
	struct {
		u32 reserved:24;
		u32 cfg_wb0:8;
	};
	u32 word;
};

union scratch2_reg {
	struct {
		u32 pllm_base_divm:5;
		u32 pllm_base_divn:10;
		u32 pllm_base_divp:3;
		u32 pllm_misc_lfcon:4;
		u32 pllm_misc_cpcon:4;
		u32 gp_xm2cfga_padctrl_preemp:1;
		u32 gp_xm2cfgd_padctrl_schmt:1;
		u32 osc_ctrl_xobp:1;
		u32 memory_type:3;
	};
	u32 word;
};

union scratch4_reg {
	struct {
		u32 emc_clock_divider:8;
		u32 pllm_stable_time:8;
		u32 pllx_stable_time:8;
		u32 emc_fbio_spare_cfg_wb0:8;
	};
	u32 word;
};

union scratch24_reg {
	struct {
		u32 emc_auto_cal_wait:8;
		u32 emc_pin_program_wait:8;
		u32 warmboot_wait:8;
		u32 reserved:8;
	};
	u32 word;
};

void warmboot_save_sdram_params(void)
{
	u32 ram_code;
	struct sdram_params sdram;
	struct pmux_tri_ctlr *pmt = (struct pmux_tri_ctlr *)NV_PA_APB_MISC_BASE;
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	struct pmc_ctlr *pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	struct apb_misc_gp_ctlr *gp =
			(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;
	struct emc_ctlr *emc = (struct emc_ctlr *)NV_PA_EMC_BASE;
	union osc_ctrl_reg osc_ctrl;
	union pll_base_reg pllm_base;
	union pll_misc_reg pllm_misc;
	union scratch2_reg scratch2;
	union scratch4_reg scratch4;
	union scratch24_reg scratch24;
	union xm2cfga_reg xm2cfga;
	union xm2cfgd_reg xm2cfgd;
	union fbio_spare_reg fbio_spare;

	/* get ram code that is used as index to array sdram_params in BCT */
	ram_code = bf_readl(STRAP_OPT_A_RAM_CODE, &pmt->pmt_strap_opt_a) & 3;
	memcpy(&sdram,
	       (char *)((struct sdram_params *)SDRAM_PARAMS_BASE + ram_code),
	       sizeof(sdram));

	osc_ctrl.word = readl(&clkrst->crc_osc_ctrl);
	pllm_base.word = readl(&clkrst->crc_pll[CLOCK_ID_MEMORY].pll_base);
	pllm_misc.word = readl(&clkrst->crc_pll[CLOCK_ID_MEMORY].pll_misc);
	xm2cfga.word = readl(&gp->xm2cfga);
	xm2cfgd.word = readl(&gp->xm2cfgd);

	scratch2.word = 0;
	scratch2.osc_ctrl_xobp = osc_ctrl.xobp;
	scratch2.pllm_base_divm = pllm_base.pll_divm;
	scratch2.pllm_base_divn = pllm_base.pll_divn;
	scratch2.pllm_base_divp = pllm_base.pll_divp;
	scratch2.pllm_misc_cpcon = pllm_misc.pll_cpcon;
	scratch2.pllm_misc_lfcon = pllm_misc.pll_lfcon;
	scratch2.gp_xm2cfga_padctrl_preemp = xm2cfga.preemp_en;
	scratch2.gp_xm2cfgd_padctrl_schmt = xm2cfgd.schmt_en;
	scratch2.memory_type = sdram.memory_type;
	writel(scratch2.word, &pmc->pmc_scratch2);

	/* collect data from various sources for pmc_scratch4 */
	fbio_spare.word = readl(&emc->fbio_spare);
	scratch4.word = 0;
	scratch4.emc_fbio_spare_cfg_wb0 = fbio_spare.cfg_wb0;
	scratch4.emc_clock_divider = sdram.emc_clock_divider;
	scratch4.pllm_stable_time = -1;
	scratch4.pllx_stable_time = -1;
	writel(scratch4.word, &pmc->pmc_scratch4);

	/* collect various data from sdram for pmc_scratch24 */
	scratch24.word = 0;
	scratch24.emc_pin_program_wait = sdram.emc_pin_program_wait;
	scratch24.emc_auto_cal_wait = sdram.emc_auto_cal_wait;
	scratch24.warmboot_wait = sdram.warm_boot_wait;
	writel(scratch24.word, &pmc->pmc_scratch24);
}

/*
 * NOTE: If more than one of the following is enabled, only one of them will
 *	 actually be used. RANDOM takes precedence over PATTERN and ZERO, and
 *	 PATTERN takes precedence overy ZERO.
 *
 *	 RANDOM_AES_BLOCK_IS_PATTERN is to define a 32-bit PATTERN.
 */
#define RANDOM_AES_BLOCK_IS_RANDOM	/* to randomize the header */
#undef RANDOM_AES_BLOCK_IS_PATTERN	/* to patternize the header */
#undef RANDOM_AES_BLOCK_IS_ZERO		/* to clear the header */

static u32 get_major_version(void)
{
	u32 major_id;
	struct apb_misc_gp_ctlr *gp =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;

	major_id = bf_readl(HIDREV_MAJORPREV, &gp->hidrev);
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

static int ap20_is_odm_production_mode(void)
{
	struct fuse_regs *fuse = (struct fuse_regs *)NV_PA_FUSE_BASE;

	if (!is_failure_analysis_mode(fuse) &&
	    is_odm_production_mode_fuse_set(fuse))
		return 1;
	else
		return 0;
}

static int ap20_is_production_mode(void)
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

static enum fuse_operating_mode fuse_get_operation_mode(void)
{
	u32 chip_id;
	struct apb_misc_gp_ctlr *gp =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;

	chip_id = bf_readl(HIDREV_CHIPID, &gp->hidrev);
	if (chip_id == CHIPID_TEGRA2) {
		if (ap20_is_odm_production_mode()) {
			printf("!! odm_production_mode is not supported !!\n");
			return MODE_UNDEFINED;
		} else
			if (ap20_is_production_mode())
				return MODE_PRODUCTION;
			else
				return MODE_UNDEFINED;
	}
	return MODE_UNDEFINED;
}

/* Currently, this routine returns a 32-bit all 0 seed. */
static u32 query_random_seed(void)
{
	return 0;
}

static void determine_crypto_options(int *is_encrypted, int *is_signed,
				     int *use_zero_key)
{
	switch (fuse_get_operation_mode()) {
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
	hash = (u8 *)(start + offsetof(struct wb_header, hash));
	err = sign_data_block(source, length, hash);

	return err;
}

int warmboot_prepare_code(u32 seg_address, u32 seg_length)
{
	int err = 0;
	u32 length;			/* length of the signed/encrypt code */
	struct wb_header *dst_header;	/* Pointer to dest WB header */
	int is_encrypted;		/* Segment is encrypted */
	int is_signed;			/* Segment is signed */
	int use_zero_key;		/* Use key of all zeros */

	/* Determine crypto options. */
	determine_crypto_options(&is_encrypted, &is_signed, &use_zero_key);

	/* Get the actual code limits. */
	length = roundup(((u32)wb_end - (u32)wb_start), 16);

	/*
	 * The region specified by seg_address must not be in IRAM and must be
	 * nonzero in length.
	 */
	if ((seg_length == 0) || (seg_address == 0) ||
	    (seg_address >= AP20_BASE_PA_SRAM)) {
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
	dst_header->length_in_secure = length + sizeof(struct wb_header);
	dst_header->length_secure = length + sizeof(struct wb_header);
	dst_header->destination = AP20_WB_RUN_ADDRESS;
	dst_header->entry_point = AP20_WB_RUN_ADDRESS;
	dst_header->code_length = length;

	if (is_encrypted) {
		printf("!!!! Encryption is not supported !!!!\n");
		dst_header->length_in_secure = 0;
		err = -EACCES;
		goto fail;
	} else
		/* copy the wb code directly following dst_header. */
		memcpy((char *)(dst_header+1), (char *)wb_start, length);

	if (is_signed)
		err = sign_wb_code(seg_address, dst_header->length_in_secure,
				   use_zero_key);

fail:
	if (err)
		printf("WB code not copied to LP0 location! (error=%d)\n", err);

	return err;
}

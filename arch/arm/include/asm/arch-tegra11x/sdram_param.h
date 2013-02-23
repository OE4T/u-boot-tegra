/*
 *  (C) Copyright 2013
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

#ifndef _SDRAM_PARAM_H_
#define _SDRAM_PARAM_H_

enum memory_type {
	MEMORY_TYPE_NONE = 0,
	MEMORY_TYPE_LPDDR2,
	MEMORY_TYPE_DDR3,
	MEMORY_TYPE_NUM,
	MEMORY_TYPE_FORCE32 = 0x7FFFFFFF
};

/**
 * Defines the SDRAM parameter structure
 */
struct sdram_params {
	enum memory_type memory_type;
	u32 pllm_input_divider;
	u32 pllm_feedback_divider;
	u32 pllm_stable_time;
	u32 pllm_setup_control;
	u32 pllm_select_div2;
	u32 pllm_pdl_shift_ph45;
	u32 pllm_pdl_shift_ph90;
	u32 pllm_pdl_shift_ph135;
	u32 pllm_kcp;
	u32 pllm_kvco;
	u32 emc_bct_spare0;
	u32 emc_clock_source;
	u32 emc_auto_cal_interval;
	u32 emc_auto_cal_config;
	u32 emc_auto_cal_config2;
	u32 emc_auto_cal_config3;
	u32 emc_auto_cal_wait;
	u32 emc_adr_cfg;
	u32 emc_pin_program_wait;
	u32 emc_pin_extra_wait;
	u32 emc_timing_control_wait;
	u32 emc_rc;
	u32 emc_rfc;
	u32 emc_rfc_slr;
	u32 emc_ras;
	u32 emc_rp;
	u32 emc_r2r;
	u32 emc_w2w;
	u32 emc_r2w;
	u32 emc_w2r;
	u32 emc_r2p;
	u32 emc_w2p;
	u32 emc_rd_rcd;
	u32 emc_wr_rcd;
	u32 emc_rdr;
	u32 emc_rext;
	u32 emc_wext;
	u32 emc_wdv;
	u32 emc_wdv_mask;
	u32 emc_quse;
	u32 emc_ibdly;
	u32 emc_einput;
	u32 emc_einput_duration;
	u32 emc_puterm_extra;
	u32 emc_cdb_cntl1;
	u32 emc_cdb_cntl2;
	u32 emc_qrst;
	u32 emc_qsafe;
	u32 emc_rdv;
	u32 emc_rdv_mask;
	u32 emc_ctt;
	u32 emc_ctt_duration;
	u32 emc_refresh;
	u32 emc_burst_refresh_num;
	u32 emc_pre_refresh_req_cnt;
	u32 emv_pdex2wr;
	u32 emc_pdex2rd;
	u32 emc_pchg2pden;
	u32 emc_act2pden;
	u32 emc_ar2pden;
	u32 emc_rw2pden;
	u32 emc_txsr;
	u32 emc_txsrdll;
	u32 emc_tcke;
	u32 emc_tckesr;
	u32 emc_tpd;
	u32 emc_tfaw;
	u32 emc_trpab;
	u32 emc_tclkstable;
	u32 emc_tclkstop;
	u32 emc_trefbw;
	u32 emc_quse_extra;
	u32 emc_fbio_cfg5;
	u32 emc_fbio_cfg6;
	u32 emc_fbio_spare;
	u32 emc_cfg_rsv;
	u32 emc_mrs;
	u32 emc_emrs;
	u32 emc_emrs2;
	u32 emc_emrs3;
	u32 emc_mrw1;
	u32 emc_mrw2;
	u32 emc_mrw3;
	u32 emc_mrw4;
	u32 emc_mrw_extra;
	u32 emc_warmboot_mrw_extra;
	u32 emc_warmboot_extra_modereg_write_en;
	u32 emc_extra_modereg_write_enable;
	u32 emc_mrw_reset_command;
	u32 emc_mrw_reset_ninit_wait;
	u32 emc_mrs_wait_cnt;
	u32 emc_mrs_wait_cnt2;
	u32 emc_cfg;
	u32 emc_cfg2;
	u32 emc_dbg;
	u32 emc_cmdq;
	u32 emc_mc2emcq;
	u32 emc_dyn_self_ref_control;
	u32 ahb_arb_xbar_ctrl_mem_init_done;
	u32 emc_cfg_dig_dll;
	u32 emc_cfg_dig_dll_period;
	u32 emc_dev_select;
	u32 emc_sel_dpd_ctrl;
	u32 emc_dll_xform_dqso;
	u32 emc_dll_xform_dqs1;
	u32 emc_dll_xform_dqs2;
	u32 emc_dll_xform_dqs3;
	u32 emc_dll_xform_dqs4;
	u32 emc_dll_xform_dqs5;
	u32 emc_dll_xform_dqs6;
	u32 emc_dll_xform_dqs7;
	u32 emc_dll_xform_quse0;
	u32 emc_dll_xform_quse1;
	u32 emc_dll_xform_quse2;
	u32 emc_dll_xform_quse3;
	u32 emc_dll_xform_quse4;
	u32 emc_dll_xform_quse5;
	u32 emc_dll_xform_quse6;
	u32 emc_dll_xform_quse7;
	u32 emc_dll_xform_addr0;
	u32 emc_dll_xform_addr1;
	u32 emc_dll_xform_addr2;
	u32 emc_dli_trim_txdqs0;
	u32 emc_dli_trim_txdqs1;
	u32 emc_dli_trim_txdqs2;
	u32 emc_dli_trim_txdqs3;
	u32 emc_dli_trim_txdqs4;
	u32 emc_dli_trim_txdqs5;
	u32 emc_dli_trim_txdqs6;
	u32 emc_dli_trim_txdqs7;
	u32 emc_dll_xform_dq0;
	u32 emc_dll_xform_dq1;
	u32 emc_dll_xform_dq2;
	u32 emc_dll_xform_dq3;
	u32 warmboot_wait;
	u32 emc_ctt_term_ctrl;
	u32 emc_odt_write;
	u32 emc_odt_read;
	u32 emc_zcal_interval;
	u32 emc_zcal_wait_cnt;
	u32 emc_zcal_mrw_cmd;
	u32 emc_mrs_rest_dll;
	u32 emc_zcal_init_dev0;
	u32 emc_zcal_init_dev1;
	u32 emc_zcal_init_wait;
	u32 emc_zcal_warmcoldboot_enables;
	u32 emc_mrw_lpddr2_zcal_warmboot;
	u32 emc_zq_cal_ddr3_warmboot;
	u32 emc_zcal_warmboot_wait;
	u32 emc_mrs_warmboot_enable;
	u32 emc_mrs_reset_dll_wait;
	u32 emc_mrs_extra;
	u32 emc_warmboot_mrs_extra;
	u32 emc_emrs_ddr2_dll_enable;
	u32 emc_mrs_ddr2_dll_reset;
	u32 emc_emrs_ddr2_ocd_calib;
	u32 emc_ddr2_wait;
	u32 emc_clken_override;
	u32 emc_extra_refresh_num;
	u32 emc_clken_override_all_warmboot;
	u32 mc_clken_override_all_warmboot;
	u32 emc_cfg_dig_dll_period_warmboot;
	u32 pmc_vddp_sel;
	u32 pmc_ddr_pwr;
	u32 pmc_ddr_cfg;
	u32 pmc_io_dpd_req;
	u32 pmc_io_dpd2_req;
	u32 pmc_reg_short;
	u32 pmc_e_no_vttgen;
	u32 pmc_no_iopower;
	u32 emc_xm2cmdpadctrl;
	u32 emc_xm2cmdpadctrl2;
	u32 emc_xm2cmdpadctrl3;
	u32 emc_xm2cmdpadctrl4;
	u32 emc_xm2dqspadctrl;
	u32 emc_xm2dqspadctrl2;
	u32 emc_xm2dqspadctrl3;
	u32 emc_xm2dqspadctrl4;
	u32 emc_xm2dqpadctrl;
	u32 emc_xm2dqpadctrl2;
	u32 emc_xm2clkpadctrl;
	u32 emc_xm2clkpadctrl2;
	u32 emc_xm2comppadctrl;
	u32 emc_xm2vttgenpadctrl;
	u32 emc_xm2vttgenpadctrl2;
	u32 emc_acpd_control;
	u32 emc_swizzle_rank0_byte_cfg;
	u32 emc_swizzle_rank0_byte0;
	u32 emc_swizzle_rank0_byte1;
	u32 emc_swizzle_rank0_byte2;
	u32 emc_swizzle_rank0_byte3;
	u32 emc_swizzle_rank1_byte_cfg;
	u32 emc_swizzle_rank1_byte0;
	u32 emc_swizzle_rank1_byte1;
	u32 emc_swizzle_rank1_byte2;
	u32 emc_swizzle_rank1_byte3;
	u32 emc_addr_swizzle_stack1a;
	u32 emc_addr_swizzle_stack1b;
	u32 emc_addr_swizzle_stack2a;
	u32 emc_addr_swizzle_stack2b;
	u32 emc_addr_swizzle_stack3;
	u32 emc_dsr_vttgen_drv;
	u32 emc_txdsrvttgen;
	u32 mc_emem_adr_cfg;
	u32 mc_emem_adr_cfg_dev0;
	u32 mc_emem_adr_cfg_dev1;
	u32 mc_emem_adr_cfg_channel_mask;
	u32 mc_emem_adr_cfg_channel_mask_propag;
	u32 mc_emem_adr_cfg_bank_mask0;
	u32 mc_emem_adr_cfg_bank_mask1;
	u32 mc_emem_adr_cfg_bank_mask2;
	u32 mc_emem_cfg;
	u32 mc_emem_arb_cfg;
	u32 mc_emem_arb_outstanding_req;
	u32 mc_emem_arb_timing_rcd;
	u32 mc_emem_arb_timing_rp;
	u32 mc_emem_arb_timing_rc;
	u32 mc_emem_arb_timing_ras;
	u32 mc_emem_arb_timing_faw;
	u32 mc_emem_arb_timing_rrd;
	u32 mc_emem_arb_timing_rap2pre;
	u32 mc_emem_arb_timing_wap2pre;
	u32 mc_emem_arb_timing_r2r;
	u32 mc_emem_arb_timing_w2w;
	u32 mc_emem_arb_timing_r2w;
	u32 mc_emem_arb_timing_w2r;
	u32 mc_emem_arb_da_turns;
	u32 mc_emem_arb_da_covers;
	u32 mc_emem_arb_misc0;
	u32 mc_emem_arb_misc1;
	u32 mc_emem_arb_ring1_throttle;
	u32 mc_emem_arb_override;
	u32 mc_emem_arb_rsv;
	u32 mc_clken_override;
	u32 mc_emc_reg_mode;
	u32 mc_video_protect_bom;
	u32 mc_video_protect_size_mb;
	u32 mc_video_protect_vpr_override;
	u32 mc_sec_carveout_bom;
	u32 mc_sec_carveout_size_mb;
	u32 mc_video_protect_write_access;
	u32 mc_sec_carveout_protect_write_access;
	u32 emc_ca_training_enable;
	u32 emc_ca_training_timing_cntl1;
	u32 emc_ca_training_timing_cntl2;
	u32 swizzle_rank_byte_encode;
	u32 bootrom_patch_control;
	u32 bootrom_patch_data;
	u32 ch1_emc_dll_xform_dqs0;
	u32 ch1_emc_dll_xform_dqs1;
	u32 ch1_emc_dll_xform_dqs2;
	u32 ch1_emc_dll_xform_dqs3;
	u32 ch1_emc_dll_xform_dqs4;
	u32 ch1_emc_dll_xform_dqs5;
	u32 ch1_emc_dll_xform_dqs6;
	u32 ch1_emc_dll_xform_dqs7;
	u32 ch1_emc_dll_xform_quse0;
	u32 ch1_emc_dll_xform_quse1;
	u32 ch1_emc_dll_xform_quse2;
	u32 ch1_emc_dll_xform_quse3;
	u32 ch1_emc_dll_xform_quse4;
	u32 ch1_emc_dll_xform_quse5;
	u32 ch1_emc_dll_xform_quse6;
	u32 ch1_emc_dll_xform_quse7;
	u32 ch1_emc_dli_trim_txdqs0;
	u32 ch1_emc_dli_trim_txdqs1;
	u32 ch1_emc_dli_trim_txdqs2;
	u32 ch1_emc_dli_trim_txdqs3;
	u32 ch1_emc_dli_trim_txdqs4;
	u32 ch1_emc_dli_trim_txdqs5;
	u32 ch1_emc_dli_trim_txdqs6;
	u32 ch1_emc_dli_trim_txdqs7;
	u32 ch1_emc_dll_xform_dq0;
	u32 ch1_emc_dll_xform_dq1;
	u32 ch1_emc_dll_xform_dq2;
	u32 ch1_emc_dll_xform_dq3;
	u32 ch1_emc_swizzle_rank0_byte_cfg;
	u32 ch1_emc_swizzle_rank0_byte0;
	u32 ch1_emc_swizzle_rank0_byte1;
	u32 ch1_emc_swizzle_rank0_byte2;
	u32 ch1_emc_swizzle_rank0_byte3;
	u32 ch1_emc_swizzle_rank1_byte_cfg;
	u32 ch1_emc_swizzle_rank1_byte0;
	u32 ch1_emc_swizzle_rank1_byte1;
	u32 ch1_emc_swizzle_rank1_byte2;
	u32 ch1_emc_swizzle_rank1_byte3;
	u32 ch1_emc_addr_swizzle_stack1a;
	u32 ch1_emc_addr_swizzle_stack1b;
	u32 ch1_emc_addr_swizzle_stack2a;
	u32 ch1_emc_addr_swizzle_stack2b;
	u32 ch1_emc_addr_swizzle_stack3;
	u32 ch1_emc_auto_cal_config;
	u32 ch1_emc_auto_cal_config2;
	u32 ch1_emc_auto_cal_config3;
	u32 ch1_emc_cdb_cntl_1;
	u32 ch1_emc_dll_xform_addr0;
	u32 ch1_emc_dll_xform_addr1;
	u32 ch1_emc_dll_xform_addr2;
	u32 ch1_emc_fbio_spare;
	u32 ch1_emc_xm2clkpadctrl;
	u32 ch1_emc_xm2clkpadctrl2;
	u32 ch1_emc_xm2cmdpadctrl2;
	u32 ch1_emc_xm2cmdpadctrl3;
	u32 ch1_emc_xm2cmdpadctrl4;
	u32 ch1_emc_xm2dqpadctrl;
	u32 ch1_emc_xm2dqpadctrl2;
	u32 ch1_cmc_xm2dqspadctrl;
	u32 ch1_cmc_xm2dqspadctrl3;
	u32 ch1_cmc_xm2dqspadctrl4;
};
#endif

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Rockchip VDPU381 Video Decoder driver registers description
 *
 * Copyright (C) 2024 Collabora, Ltd.
 *  Detlev Casanova <detlev.casanova@collabora.com>
 */

#include <linux/types.h>

#ifndef _RKVDEC_REGS_H_
#define _RKVDEC_REGS_H_

#define OFFSET_COMMON_REGS		(8 * sizeof(u32))
#define OFFSET_CODEC_PARAMS_REGS	(64 * sizeof(u32))
#define OFFSET_COMMON_ADDR_REGS		(128 * sizeof(u32))
#define OFFSET_CODEC_ADDR_REGS		(160 * sizeof(u32))
#define OFFSET_POC_HIGHBIT_REGS		(200 * sizeof(u32))

#define VDPU381_MODE_HEVC	0
#define VDPU381_MODE_H264	1
#define VDPU381_MODE_VP9	2
#define VDPU381_MODE_AVS2	3

#define MAX_SLICE_NUMBER	0x3fff

#define RKVDEC_1080P_PIXELS		(1920 * 1080)
#define RKVDEC_4K_PIXELS		(4096 * 2304)
#define RKVDEC_8K_PIXELS		(7680 * 4320)
#define RKVDEC_TIMEOUT_1080p		(0xefffff)
#define RKVDEC_TIMEOUT_4K		(0x2cfffff)
#define RKVDEC_TIMEOUT_8K		(0x4ffffff)
#define RKVDEC_TIMEOUT_MAX		(0xffffffff)

#define VDPU381_REG_DEC_E		0x028
#define VDPU381_DEC_E_BIT		1

#define VDPU381_REG_IMPORTANT_EN	0x02c
#define VDPU381_DEC_IRQ_DISABLE	BIT(4)

#define VDPU381_REG_STA_INT		0x380
#define VDPU381_STA_INT_DEC_RDY_STA	BIT(2)
#define VDPU381_STA_INT_ERROR		BIT(4)
#define VDPU381_STA_INT_TIMEOUT		BIT(5)
#define VDPU381_STA_INT_SOFTRESET_RDY	BIT(9)

/* base: OFFSET_COMMON_REGS */
struct rkvdec_vdpu381_regs_common {
	struct rkvdec_vdpu381_in_out {
		u32 in_endian		: 1;
		u32 in_swap32_e		: 1;
		u32 in_swap64_e		: 1;
		u32 str_endian		: 1;
		u32 str_swap32_e	: 1;
		u32 str_swap64_e	: 1;
		u32 out_endian		: 1;
		u32 out_swap32_e	: 1;
		u32 out_cbcr_swap	: 1;
		u32 out_swap64_e	: 1;
		u32 reserved		: 22;
	} reg008;

	struct rkvdec_vdpu381_dec_mode {
		u32 dec_mode	: 10;
		u32 reserved	: 22;
	} reg009;

	struct rkvdec_vdpu381_dec_e {
		u32 dec_e	: 1;
		u32 reserved	: 31;
	} reg010;

	struct rkvdec_vdpu381_important_en {
		u32 reserved			: 1;
		u32 dec_clkgate_e		: 1;
		u32 dec_e_strmd_clkgate_dis	: 1;
		u32 reserved0			: 1;

		u32 dec_irq_dis			: 1;
		u32 dec_timeout_e		: 1;
		u32 buf_empty_en		: 1;
		u32 reserved1			: 3;

		u32 dec_e_rewrite_valid		: 1;
		u32 reserved2			: 9;
		u32 softrst_en_p		: 1;
		u32 force_softreset_valid	: 1;
		u32 reserved3			: 2;
		u32 pix_range_detection_e	: 1;
		u32 reserved4			: 7;
	} reg011;

	struct rkvdec_vdpu381_sencodary_en {
		u32 wr_ddr_align_en		: 1;
		u32 colmv_compress_en		: 1;
		u32 fbc_e			: 1;
		u32 reserved0			: 1;

		u32 buspr_slot_disable		: 1;
		u32 error_info_en		: 1;
		u32 info_collect_en		: 1;
		u32 wait_reset_en		: 1;

		u32 scanlist_addr_valid_en	: 1;
		u32 scale_down_en		: 1;
		u32 error_cfg_wr_disable	: 1;
		u32 reserved1			: 21;
	} reg012;

	struct rkvdec_vdpu381_en_mode_set {
		u32 timeout_mode		: 1;
		u32 req_timeout_rst_sel		: 1;
		u32 reserved0			: 1;
		u32 dec_commonirq_mode		: 1;
		u32 reserved1			: 2;
		u32 stmerror_waitdecfifo_empty	: 1;
		u32 reserved2			: 2;
		u32 h26x_streamd_error_mode	: 1;
		u32 reserved3			: 2;
		u32 allow_not_wr_unref_bframe	: 1;
		u32 fbc_output_wr_disable	: 1;
		u32 reserved4			: 1;
		u32 colmv_error_mode		: 1;

		u32 reserved5			: 2;
		u32 h26x_error_mode		: 1;
		u32 reserved6			: 2;
		u32 ycacherd_prior		: 1;
		u32 reserved7			: 2;
		u32 cur_pic_is_idr		: 1;
		u32 reserved8			: 1;
		u32 right_auto_rst_disable	: 1;
		u32 frame_end_err_rst_flag	: 1;
		u32 rd_prior_mode		: 1;
		u32 rd_ctrl_prior_mode		: 1;
		u32 reserved9			: 1;
		u32 filter_outbuf_mode		: 1;
	} reg013;

	struct rkvdec_vdpu381_fbc_param_set {
		u32 fbc_force_uncompress	: 1;

		u32 reserved0			: 2;
		u32 allow_16x8_cp_flag		: 1;
		u32 reserved1			: 2;

		u32 fbc_h264_exten_4or8_flag	: 1;
		u32 reserved2			: 25;
	} reg014;

	struct rkvdec_vdpu381_stream_param_set {
		u32 rlc_mode_direct_write	: 1;
		u32 rlc_mode			: 1;
		u32 reserved0			: 3;

		u32 strm_start_bit		: 7;
		u32 reserved1			: 20;
	} reg015;

	u32 stream_len;

	struct rkvdec_vdpu381_slice_number {
		u32 slice_num	: 25;
		u32 reserved	: 7;
	} reg017;

	struct rkvdec_vdpu381_y_hor_stride {
		u32 y_hor_virstride	: 16;
		u32 reserved		: 16;
	} reg018;

	struct rkvdec_vdpu381_uv_hor_stride {
		u32 uv_hor_virstride	: 16;
		u32 reserved		: 16;
	} reg019;

	struct rkvdec_vdpu381_y_stride {
		u32 y_virstride		: 28;
		u32 reserved		: 4;
	} reg020;

	struct rkvdec_vdpu381_error_ctrl_set {
		u32 inter_error_prc_mode		: 1;
		u32 error_intra_mode			: 1;
		u32 error_deb_en			: 1;
		u32 picidx_replace			: 5;
		u32 error_spread_e			: 1;
		u32 reserved0				: 3;
		u32 error_inter_pred_cross_slice	: 1;
		u32 reserved1				: 11;
		u32 roi_error_ctu_cal_en		: 1;
		u32 reserved2				: 7;
	} reg021;

	struct rkvdec_vdpu381_err_roi_ctu_offset_start {
		u32 roi_x_ctu_offset_st	: 12;
		u32 reserved0		: 4;
		u32 roi_y_ctu_offset_st	: 12;
		u32 reserved1		: 4;
	} reg022;

	struct rkvdec_vdpu381_err_roi_ctu_offset_end {
		u32 roi_x_ctu_offset_end	: 12;
		u32 reserved0			: 4;
		u32 roi_y_ctu_offset_end	: 12;
		u32 reserved1			: 4;
	} reg023;

	struct rkvdec_vdpu381_cabac_error_en_lowbits {
		u32 cabac_err_en_lowbits	: 32;
	} reg024;

	struct rkvdec_vdpu381_cabac_error_en_highbits {
		u32 cabac_err_en_highbits	: 30;
		u32 reserved			: 2;
	} reg025;

	struct rkvdec_vdpu381_block_gating_en {
		u32 swreg_block_gating_e	: 20;
		u32 reserved			: 11;
		u32 reg_cfg_gating_en		: 1;
	} reg026;

	struct SW027_CORE_SAFE_PIXELS {
		u32 core_safe_x_pixels	: 16;
		u32 core_safe_y_pixels	: 16;
	} reg027;

	struct rkvdec_vdpu381_multiply_core_ctrl {
		u32 swreg_vp9_wr_prob_idx	: 3;
		u32 reserved0			: 1;
		u32 swreg_vp9_rd_prob_idx	: 3;
		u32 reserved1			: 1;

		u32 swreg_ref_req_advance_flag	: 1;
		u32 sw_colmv_req_advance_flag	: 1;
		u32 sw_poc_only_highbit_flag	: 1;
		u32 sw_poc_arb_flag		: 1;

		u32 reserved2			: 4;
		u32 sw_film_idx			: 10;
		u32 reserved3			: 2;
		u32 sw_pu_req_mismatch_dis	: 1;
		u32 sw_colmv_req_mismatch_dis	: 1;
		u32 reserved4			: 2;
	} reg028;

	struct SW029_SCALE_DOWN_CTRL {
		u32 scale_down_hor_ratio	: 2;
		u32 reserved0			: 6;
		u32 scale_down_vrz_ratio	: 2;
		u32 reserved1			: 22;
	} reg029;

	struct SW032_Y_SCALE_DOWN_TILE8x8_HOR_STRIDE {
		u32 y_scale_down_hor_stride	: 20;
		u32 reserved0			: 12;
	} reg030;

	struct SW031_UV_SCALE_DOWN_TILE8x8_HOR_STRIDE {
		u32 uv_scale_down_hor_stride	: 20;
		u32 reserved0			: 12;
	} reg031;

	u32 timeout_threshold;
} __packed;

/* base: OFFSET_COMMON_ADDR_REGS */
struct rkvdec_vdpu381_regs_common_addr {
	u32 rlc_base;
	u32 rlcwrite_base;
	u32 decout_base;
	u32 colmv_cur_base;
	u32 error_ref_base;
	u32 rcb_base[10];
} __packed;

struct rkvdec_vdpu381_h26x_set {
	u32 h26x_frame_orslice		: 1;
	u32 h26x_rps_mode		: 1;
	u32 h26x_stream_mode		: 1;
	u32 h26x_stream_lastpacket	: 1;
	u32 h264_firstslice_flag	: 1;
	u32 reserved			: 27;
} __packed;

/* base: OFFSET_CODEC_PARAMS_REGS */
struct rkvdec_vdpu381_regs_h264_params {
	struct rkvdec_vdpu381_h26x_set reg064;

	u32 cur_top_poc;
	u32 cur_bot_poc;
	u32 ref_pocs[32];

	struct rkvdec_vdpu381_h264_info {
		struct rkvdec_vdpu381_h264_ref_info {
			u32 ref_field		: 1;
			u32 ref_topfield_used	: 1;
			u32 ref_botfield_used	: 1;
			u32 ref_colmv_use_flag	: 1;
			u32 ref_reserved	: 4;
		} __packed ref_info[4];
	} __packed ref_info_regs[4];

	u32 reserved_103_111[9];

	struct rkvdec_vdpu381_error_ref_info {
		u32 avs2_ref_error_field	: 1;
		u32 avs2_ref_error_topfield	: 1;
		u32 ref_error_topfield_used	: 1;
		u32 ref_error_botfield_used	: 1;
		u32 reserved			: 28;
	} reg112;
} __packed;

struct rkvdec_vdpu381_regs_hevc_params {
	struct rkvdec_vdpu381_h26x_set reg064;

	u32 cur_top_poc;
	u32 cur_bot_poc;

	u32 reg067_082_ref_poc[16];

	u32 reserved_083_098[16];

	struct rkvdec_vdpu381_hevc_ref_valid {
		u32 hevc_ref_valid_0    : 1;
		u32 hevc_ref_valid_1    : 1;
		u32 hevc_ref_valid_2    : 1;
		u32 hevc_ref_valid_3    : 1;
		u32 reserve0            : 4;
		u32 hevc_ref_valid_4    : 1;
		u32 hevc_ref_valid_5    : 1;
		u32 hevc_ref_valid_6    : 1;
		u32 hevc_ref_valid_7    : 1;
		u32 reserve1            : 4;
		u32 hevc_ref_valid_8    : 1;
		u32 hevc_ref_valid_9    : 1;
		u32 hevc_ref_valid_10   : 1;
		u32 hevc_ref_valid_11   : 1;
		u32 reserve2            : 4;
		u32 hevc_ref_valid_12   : 1;
		u32 hevc_ref_valid_13   : 1;
		u32 hevc_ref_valid_14   : 1;
		u32 reserve3            : 5;
	} reg099;

	u32 reserved_100_102[3];

	struct rkvdec_vdpu381_hevc_mvc0 {
		u32 ref_pic_layer_same_with_cur : 16;
		u32 reserve                     : 16;
	} reg103;

	struct rkvdec_vdpu381_hevc_mvc1 {
		u32 poc_lsb_not_present_flag        : 1;
		u32 num_direct_ref_layers           : 6;
		u32 reserve0                        : 1;

		u32 num_reflayer_pics               : 6;
		u32 default_ref_layers_active_flag  : 1;
		u32 max_one_active_ref_layer_flag   : 1;

		u32 poc_reset_info_present_flag     : 1;
		u32 vps_poc_lsb_aligned_flag        : 1;
		u32 mvc_poc15_valid_flag            : 1;
		u32 reserve1                        : 13;
	} reg104;

	u32 reserved_105_111[7];

	struct rkvdec_vdpu381_hevc_ref_info {
		u32 avs2_ref_error_field        : 1;
		u32 avs2_ref_error_topfield     : 1;
		u32 ref_error_topfield_used     : 1;
		u32 ref_error_botfield_used     : 1;
		u32 reserve                     : 28;
	} reg112;

} __packed;

/* base: OFFSET_CODEC_ADDR_REGS */
struct rkvdec_vdpu381_regs_h26x_addr {
	u32 reserved_160;
	u32 pps_base;
	u32 reserved_162;
	u32 rps_base;
	u32 ref_base[16];
	u32 scanlist_addr;
	u32 colmv_base[16];
	u32 cabactbl_base;
} __packed;

struct rkvdec_vdpu381_regs_h26x_highpoc {
	struct rkvdec_vdpu381_ref_poc_highbit {
		u32 ref0_poc_highbit	: 4;
		u32 ref1_poc_highbit	: 4;
		u32 ref2_poc_highbit	: 4;
		u32 ref3_poc_highbit	: 4;
		u32 ref4_poc_highbit	: 4;
		u32 ref5_poc_highbit	: 4;
		u32 ref6_poc_highbit	: 4;
		u32 ref7_poc_highbit	: 4;
	} reg200[4];
	struct rkvdec_vdpu381_cur_poc_highbit {
		u32 cur_poc_highbit	: 4;
		u32 reserved		: 28;
	} reg204;
} __packed;

struct rkvdec_vdpu381_regs_h264 {
	struct rkvdec_vdpu381_regs_common		common;
	struct rkvdec_vdpu381_regs_h264_params		h264_param;
	struct rkvdec_vdpu381_regs_common_addr		common_addr;
	struct rkvdec_vdpu381_regs_h26x_addr		h264_addr;
	struct rkvdec_vdpu381_regs_h26x_highpoc		h264_highpoc;
} __packed;

struct rkvdec_vdpu381_regs_hevc {
	struct rkvdec_vdpu381_regs_common		common;
	struct rkvdec_vdpu381_regs_hevc_params		hevc_param;
	struct rkvdec_vdpu381_regs_common_addr		common_addr;
	struct rkvdec_vdpu381_regs_h26x_addr		hevc_addr;
	struct rkvdec_vdpu381_regs_h26x_highpoc		hevc_highpoc;
} __packed;

#endif /* __RKVDEC_REGS_H__ */

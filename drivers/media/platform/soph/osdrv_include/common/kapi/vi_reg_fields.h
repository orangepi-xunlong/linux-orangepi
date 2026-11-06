/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name:vi_reg_fields.h
 * Description:HW register description
 */

#ifndef _VI_REG_FIELDS_H_
#define _VI_REG_FIELDS_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           module definition            */
/******************************************/
union reg_pre_raw_be_top_ctrl {
	uint32_t raw;
	struct {
		uint32_t bayer_type_le                   : 4;
		uint32_t bayer_type_se                   : 4;
		uint32_t rgbir_en                        : 1;
		uint32_t ch_num                          : 1;
		uint32_t post_rgbir_bayer_type_le        : 4;
		uint32_t post_rgbir_bayer_type_se        : 4;
		uint32_t _rsv_18                         : 13;
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_pre_raw_be_up_pq_en {
	uint32_t raw;
	struct {
		uint32_t up_pq_en                        : 1;
	} bits;
};

union reg_pre_raw_be_img_size_le {
	uint32_t raw;
	struct {
		uint32_t frame_widthm1                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t frame_heightm1                  : 14;
	} bits;
};

union reg_pre_raw_be_pre_raw_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy_rw                        : 16;
		uint32_t dummy_ro                        : 16;
	} bits;
};

union reg_pre_raw_be_debug_info {
	uint32_t raw;
	struct {
		uint32_t ch0_crop_done                   : 1;
		uint32_t ch0_blc_done                    : 1;
		uint32_t ch0_af_done                     : 1;
		uint32_t ch0_dpc_done                    : 1;
		uint32_t ch0_async_done                  : 1;
		uint32_t ch0_pre_wdma_done               : 1;
		uint32_t ch0_rgbir_done                  : 1;
		uint32_t _rsv_7                          : 9;
		uint32_t ch1_crop_done                   : 1;
		uint32_t ch1_blc_done                    : 1;
		uint32_t ch1_dpc_done                    : 1;
		uint32_t ch1_async_done                  : 1;
		uint32_t ch1_pre_wdma_done               : 1;
		uint32_t ch1_rgbir_done                  : 1;
	} bits;
};

union reg_pre_raw_be_dma_idle_info {
	uint32_t raw;
	struct {
		uint32_t af_dma_idle                     : 1;
		uint32_t pre_wdma0_idle                  : 1;
		uint32_t pre_wdma1_idle                  : 1;
		uint32_t ch0_rgbir_wdma_idle             : 1;
		uint32_t ch1_rgbir_wdma_idle             : 1;
	} bits;
};

union reg_pre_raw_be_ip_idle_info {
	uint32_t raw;
	struct {
		uint32_t ch0_crop_idle                   : 1;
		uint32_t ch0_blc_idle                    : 1;
		uint32_t ch0_dpc_idle                    : 1;
		uint32_t ch0_af_idle                     : 1;
		uint32_t ch0_async_idle                  : 1;
		uint32_t ch0_pre_wdma_idle               : 1;
		uint32_t ch0_rgbir_idle                  : 1;
		uint32_t _rsv_7                          : 9;
		uint32_t ch1_crop_idle                   : 1;
		uint32_t ch1_blc_idle                    : 1;
		uint32_t ch1_dpc_idle                    : 1;
		uint32_t ch1_async_idle                  : 1;
		uint32_t ch1_pre_wdma_idle               : 1;
		uint32_t ch1_rgbir_idle                  : 1;
	} bits;
};

union reg_pre_raw_be_line_balance_ctrl {
	uint32_t raw;
	struct {
		uint32_t pass_sel                        : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t pass_cnt_m1                     : 8;
	} bits;
};

union reg_pre_raw_be_debug_enable {
	uint32_t raw;
	struct {
		uint32_t debug_en                        : 1;
	} bits;
};

union reg_pre_raw_be_tvalid_status {
	uint32_t raw;
	struct {
		uint32_t ip_tvalid                       : 17;
	} bits;
};

union reg_pre_raw_be_tready_status {
	uint32_t raw;
	struct {
		uint32_t ip_tready                       : 17;
	} bits;
};

union reg_pre_raw_be_patgen1 {
	uint32_t raw;
	struct {
		uint32_t x_curser                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t y_curser                        : 14;
		uint32_t curser_en                       : 1;
		uint32_t pg_enable                       : 1;
	} bits;
};

union reg_pre_raw_be_patgen2 {
	uint32_t raw;
	struct {
		uint32_t curser_value                    : 16;
	} bits;
};

union reg_pre_raw_be_patgen3 {
	uint32_t raw;
	struct {
		uint32_t value_report                    : 32;
	} bits;
};

union reg_pre_raw_be_patgen4 {
	uint32_t raw;
	struct {
		uint32_t xcnt_rpt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ycnt_rpt                        : 14;
	} bits;
};

union reg_pre_raw_be_chksum_enable {
	uint32_t raw;
	struct {
		uint32_t lexp_chksum_enable              : 1;
		uint32_t sexp_chksum_enable              : 1;
	} bits;
};

union reg_pre_raw_be_chksum {
	uint32_t raw;
	struct {
		uint32_t um_lexp                         : 12;
		uint32_t um_sexp                         : 16;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_dma_ctl_sys_control {
	uint32_t raw;
	struct {
		uint32_t qos_sel                         : 1;
		uint32_t sw_qos                          : 1;
		uint32_t enable_inv                      : 1;
		uint32_t _rsv_3                          : 5;
		uint32_t baseh                           : 8;
		uint32_t base_sel                        : 1;
		uint32_t stride_sel                      : 1;
		uint32_t seglen_sel                      : 1;
		uint32_t segnum_sel                      : 1;
		uint32_t slice_enable                    : 1;
		uint32_t update_base_addr                : 1;
		uint32_t inv_sel                         : 1;
		uint32_t _rsv_23                         : 5;
		uint32_t dbg_sel                         : 3;
	} bits;
};

union reg_isp_dma_ctl_base_addr {
	uint32_t raw;
	struct {
		uint32_t basel                           : 32;
	} bits;
};

union reg_isp_dma_ctl_dma_seglen {
	uint32_t raw;
	struct {
		uint32_t seglen                          : 28;
	} bits;
};

union reg_isp_dma_ctl_dma_stride {
	uint32_t raw;
	struct {
		uint32_t stride                          : 28;
	} bits;
};

union reg_isp_dma_ctl_dma_segnum {
	uint32_t raw;
	struct {
		uint32_t segnum                          : 14;
	} bits;
};

union reg_isp_dma_ctl_dma_status {
	uint32_t raw;
	struct {
		uint32_t status                          : 32;
	} bits;
};

union reg_isp_dma_ctl_dma_slicesize {
	uint32_t raw;
	struct {
		uint32_t slice_size                      : 6;
	} bits;
};

union reg_isp_dma_ctl_dma_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy                           : 16;
		uint32_t perf_patch_enable               : 1;
		uint32_t seglen_less16_enable            : 1;
		uint32_t sync_patch_enable               : 1;
		uint32_t trig_patch_enable               : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_ae_hist_ae_hist_status {
	uint32_t raw;
	struct {
		uint32_t ae_hist_status                  : 32;
	} bits;
};

union reg_isp_ae_hist_ae_hist_grace_reset {
	uint32_t raw;
	struct {
		uint32_t ae_hist_grace_reset             : 1;
	} bits;
};

union reg_isp_ae_hist_ae_hist_monitor {
	uint32_t raw;
	struct {
		uint32_t ae_hist_monitor                 : 32;
	} bits;
};

union reg_isp_ae_hist_ae_hist_bypass {
	uint32_t raw;
	struct {
		uint32_t ae_hist_bypass                  : 1;
		uint32_t _rsv_1                          : 19;
		uint32_t hist_zeroing_enable             : 1;
		uint32_t _rsv_21                         : 7;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_ae_hist_ae_kickoff {
	uint32_t raw;
	struct {
		uint32_t ae_zero_ae_sum                  : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t ae_wbgain_apply                 : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t loadshadowreg                   : 1;
		uint32_t _rsv_5                          : 1;
		uint32_t hist_zerohistogram              : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t hist_wbgain_apply               : 1;
		uint32_t _rsv_9                          : 1;
		uint32_t ae_hist_shadow_select           : 1;
		uint32_t _rsv_11                         : 5;
		uint32_t ae_face_enable                  : 4;
	} bits;
};

union reg_isp_ae_hist_sts_ae0_hist_enable {
	uint32_t raw;
	struct {
		uint32_t sts_ae0_hist_enable             : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t ae0_gain_enable                 : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t hist0_enable                    : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t hist0_gain_enable               : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t ir_ae_enable                    : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t ir_ae_gain_enable               : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t ir_hist_enable                  : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t ir_hist_gain_enable             : 1;
	} bits;
};

union reg_isp_ae_hist_sts_ae_offsetx {
	uint32_t raw;
	struct {
		uint32_t sts_ae0_offsetx                 : 13;
	} bits;
};

union reg_isp_ae_hist_sts_ae_offsety {
	uint32_t raw;
	struct {
		uint32_t sts_ae0_offsety                 : 13;
	} bits;
};

union reg_isp_ae_hist_sts_ae_numxm1 {
	uint32_t raw;
	struct {
		uint32_t sts_ae0_numxm1                  : 6;
	} bits;
};

union reg_isp_ae_hist_sts_ae_numym1 {
	uint32_t raw;
	struct {
		uint32_t sts_ae0_numym1                  : 6;
	} bits;
};

union reg_isp_ae_hist_sts_ae_width {
	uint32_t raw;
	struct {
		uint32_t sts_ae0_width                   : 8;
	} bits;
};

union reg_isp_ae_hist_sts_ae_height {
	uint32_t raw;
	struct {
		uint32_t sts_ae0_height                  : 8;
	} bits;
};

union reg_isp_ae_hist_sts_ae_sts_div {
	uint32_t raw;
	struct {
		uint32_t sts_ae0_sts_div                 : 13;
	} bits;
};

union reg_isp_ae_hist_sts_hist_mode {
	uint32_t raw;
	struct {
		uint32_t sts_hist0_mode                  : 2;
	} bits;
};

union reg_isp_ae_hist_ae_hist_monitor_select {
	uint32_t raw;
	struct {
		uint32_t ae_hist_monitor_select          : 32;
	} bits;
};

union reg_isp_ae_hist_ae_hist_location {
	uint32_t raw;
	struct {
		uint32_t ae_hist_location                : 32;
	} bits;
};

union reg_isp_ae_hist_sts_ir_ae_offsetx {
	uint32_t raw;
	struct {
		uint32_t sts_ir_ae_offsetx               : 13;
	} bits;
};

union reg_isp_ae_hist_sts_ir_ae_offsety {
	uint32_t raw;
	struct {
		uint32_t sts_ir_ae_offsety               : 13;
	} bits;
};

union reg_isp_ae_hist_sts_ir_ae_numxm1 {
	uint32_t raw;
	struct {
		uint32_t sts_ir_ae_numxm1                : 5;
	} bits;
};

union reg_isp_ae_hist_sts_ir_ae_numym1 {
	uint32_t raw;
	struct {
		uint32_t sts_ir_ae_numym1                : 5;
	} bits;
};

union reg_isp_ae_hist_sts_ir_ae_width {
	uint32_t raw;
	struct {
		uint32_t sts_ir_ae_width                 : 10;
	} bits;
};

union reg_isp_ae_hist_sts_ir_ae_height {
	uint32_t raw;
	struct {
		uint32_t sts_ir_ae_height                : 10;
	} bits;
};

union reg_isp_ae_hist_sts_ir_ae_sts_div {
	uint32_t raw;
	struct {
		uint32_t sts_ir_ae_sts_div               : 3;
	} bits;
};

union reg_isp_ae_hist_ae_hist_bayer_starting {
	uint32_t raw;
	struct {
		uint32_t ae_hist_bayer_starting          : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t force_bayer_enable              : 1;
	} bits;
};

union reg_isp_ae_hist_ae_hist_dummy {
	uint32_t raw;
	struct {
		uint32_t ae_hist_dummy                   : 16;
	} bits;
};

union reg_isp_ae_hist_ae_hist_checksum {
	uint32_t raw;
	struct {
		uint32_t ae_hist_checksum                : 32;
	} bits;
};

union reg_isp_ae_hist_wbg_4 {
	uint32_t raw;
	struct {
		uint32_t ae0_wbg_rgain                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ae0_wbg_ggain                   : 14;
	} bits;
};

union reg_isp_ae_hist_wbg_5 {
	uint32_t raw;
	struct {
		uint32_t ae0_wbg_bgain                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ae1_wbg_bgain                   : 14;
	} bits;
};

union reg_isp_ae_hist_wbg_6 {
	uint32_t raw;
	struct {
		uint32_t ae1_wbg_rgain                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ae1_wbg_ggain                   : 14;
	} bits;
};

union reg_isp_ae_hist_wbg_7 {
	uint32_t raw;
	struct {
		uint32_t ae0_wbg_vgain                   : 14;
	} bits;
};

union reg_isp_ae_hist_dmi_enable {
	uint32_t raw;
	struct {
		uint32_t dmi_enable                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t dmi_qos                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t force_dma_disable               : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t ir_dmi_enable                   : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t ir_dmi_qos                      : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t ir_force_dma_disable            : 1;
	} bits;
};

union reg_isp_ae_hist_ae_face0_location {
	uint32_t raw;
	struct {
		uint32_t ae_face0_offset_x               : 13;
		uint32_t ae_face0_offset_y               : 13;
	} bits;
};

union reg_isp_ae_hist_ae_face1_location {
	uint32_t raw;
	struct {
		uint32_t ae_face1_offset_x               : 13;
		uint32_t ae_face1_offset_y               : 13;
	} bits;
};

union reg_isp_ae_hist_ae_face2_location {
	uint32_t raw;
	struct {
		uint32_t ae_face2_offset_x               : 13;
		uint32_t ae_face2_offset_y               : 13;
	} bits;
};

union reg_isp_ae_hist_ae_face3_location {
	uint32_t raw;
	struct {
		uint32_t ae_face3_offset_x               : 13;
		uint32_t ae_face3_offset_y               : 13;
	} bits;
};

union reg_isp_ae_hist_ae_face0_size {
	uint32_t raw;
	struct {
		uint32_t ae_face0_size_minus1_x          : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t ae_face0_size_minus1_y          : 8;
	} bits;
};

union reg_isp_ae_hist_ae_face1_size {
	uint32_t raw;
	struct {
		uint32_t ae_face1_size_minus1_x          : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t ae_face1_size_minus1_y          : 8;
	} bits;
};

union reg_isp_ae_hist_ae_face2_size {
	uint32_t raw;
	struct {
		uint32_t ae_face2_size_minus1_x          : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t ae_face2_size_minus1_y          : 8;
	} bits;
};

union reg_isp_ae_hist_ae_face3_size {
	uint32_t raw;
	struct {
		uint32_t ae_face3_size_minus1_x          : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t ae_face3_size_minus1_y          : 8;
	} bits;
};

union reg_isp_ae_hist_ir_ae_face0_location {
	uint32_t raw;
	struct {
		uint32_t ir_ae_face0_offset_x            : 16;
		uint32_t ir_ae_face0_offset_y            : 16;
	} bits;
};

union reg_isp_ae_hist_ir_ae_face1_location {
	uint32_t raw;
	struct {
		uint32_t ir_ae_face1_offset_x            : 16;
		uint32_t ir_ae_face1_offset_y            : 16;
	} bits;
};

union reg_isp_ae_hist_ir_ae_face2_location {
	uint32_t raw;
	struct {
		uint32_t ir_ae_face2_offset_x            : 16;
		uint32_t ir_ae_face2_offset_y            : 16;
	} bits;
};

union reg_isp_ae_hist_ir_ae_face3_location {
	uint32_t raw;
	struct {
		uint32_t ir_ae_face3_offset_x            : 16;
		uint32_t ir_ae_face3_offset_y            : 16;
	} bits;
};

union reg_isp_ae_hist_ir_ae_face0_size {
	uint32_t raw;
	struct {
		uint32_t ir_ae_face0_size_minus1_x       : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t ir_ae_face0_size_minus1_y       : 7;
	} bits;
};

union reg_isp_ae_hist_ir_ae_face1_size {
	uint32_t raw;
	struct {
		uint32_t ir_ae_face1_size_minus1_x       : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t ir_ae_face1_size_minus1_y       : 7;
	} bits;
};

union reg_isp_ae_hist_ir_ae_face2_size {
	uint32_t raw;
	struct {
		uint32_t ir_ae_face2_size_minus1_x       : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t ir_ae_face2_size_minus1_y       : 7;
	} bits;
};

union reg_isp_ae_hist_ir_ae_face3_size {
	uint32_t raw;
	struct {
		uint32_t ir_ae_face3_size_minus1_x       : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t ir_ae_face3_size_minus1_y       : 7;
	} bits;
};

union reg_isp_ae_hist_ae_face0_enable {
	uint32_t raw;
	struct {
		uint32_t ae_face0_enable                 : 1;
		uint32_t ae_face1_enable                 : 1;
		uint32_t ae_face2_enable                 : 1;
		uint32_t ae_face3_enable                 : 1;
	} bits;
};

union reg_isp_ae_hist_ae_face0_sts_div {
	uint32_t raw;
	struct {
		uint32_t ae_face0_sts_div                : 13;
	} bits;
};

union reg_isp_ae_hist_ae_face1_sts_div {
	uint32_t raw;
	struct {
		uint32_t ae_face1_sts_div                : 13;
	} bits;
};

union reg_isp_ae_hist_ae_face2_sts_div {
	uint32_t raw;
	struct {
		uint32_t ae_face2_sts_div                : 13;
	} bits;
};

union reg_isp_ae_hist_ae_face3_sts_div {
	uint32_t raw;
	struct {
		uint32_t ae_face3_sts_div                : 13;
	} bits;
};

union reg_isp_ae_hist_sts_enable {
	uint32_t raw;
	struct {
		uint32_t sts_awb_enable                  : 1;
	} bits;
};

union reg_isp_ae_hist_ae_algo_enable {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 1;
		uint32_t ae_algo_enable                  : 1;
	} bits;
};

union reg_isp_ae_hist_ae_hist_low {
	uint32_t raw;
	struct {
		uint32_t ae_hist_low                     : 8;
	} bits;
};

union reg_isp_ae_hist_ae_hist_high {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 8;
		uint32_t ae_hist_high                    : 8;
	} bits;
};

union reg_isp_ae_hist_ae_top {
	uint32_t raw;
	struct {
		uint32_t ae_awb_top                      : 12;
	} bits;
};

union reg_isp_ae_hist_ae_bot {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 12;
		uint32_t ae_awb_bot                      : 12;
	} bits;
};

union reg_isp_ae_hist_ae_overexp_thr {
	uint32_t raw;
	struct {
		uint32_t ae_overexp_thr                  : 10;
	} bits;
};

union reg_isp_ae_hist_ae_num_gapline {
	uint32_t raw;
	struct {
		uint32_t ae_num_gapline                  : 1;
	} bits;
};

union reg_isp_ae_hist_ae_simple2a_result_luma {
	uint32_t raw;
	struct {
		uint32_t simple2a_result_luma            : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t e_counter_0                     : 16;
	} bits;
};

union reg_isp_ae_hist_ae_simple2a_result_rgain {
	uint32_t raw;
	struct {
		uint32_t simple2a_result_rgain           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t e_counter_1                     : 16;
	} bits;
};

union reg_isp_ae_hist_ae_simple2a_result_bgain {
	uint32_t raw;
	struct {
		uint32_t simple2a_result_bgain           : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t e_counter_2                     : 16;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_00 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_00                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_01 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_01                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_02 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_02                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_03 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_03                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_04 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_04                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_05 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_05                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_06 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_06                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_07 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_07                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_08 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_08                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_09 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_09                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_10 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_10                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_11 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_11                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_12 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_12                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_13 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_13                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_14 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_14                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_15 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_15                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_16 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_16                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_17 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_17                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_18 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_18                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_19 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_19                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_20 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_20                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_21 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_21                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_22 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_22                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_23 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_23                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_24 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_24                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_25 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_25                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_26 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_26                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_27 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_27                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_28 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_28                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_29 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_29                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_30 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_30                       : 32;
	} bits;
};

union reg_isp_ae_hist_ae_wgt_31 {
	uint32_t raw;
	struct {
		uint32_t ae_wgt_31                       : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_gms_status {
	uint32_t raw;
	struct {
		uint32_t gms_status                      : 32;
	} bits;
};

union reg_isp_gms_grace_reset {
	uint32_t raw;
	struct {
		uint32_t gms_grace_reset                 : 1;
	} bits;
};

union reg_isp_gms_monitor {
	uint32_t raw;
	struct {
		uint32_t gms_monitor                     : 32;
	} bits;
};

union reg_isp_gms_enable {
	uint32_t raw;
	struct {
		uint32_t gms_enable                      : 1;
		uint32_t out_shiftbit                    : 3;
		uint32_t _rsv_4                          : 12;
		uint32_t force_bayer_enable              : 1;
		uint32_t _rsv_17                         : 11;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_gms_flow {
	uint32_t raw;
	struct {
		uint32_t gms_zerogmsogram                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t gms_shadow_select               : 1;
	} bits;
};

union reg_isp_gms_start_x {
	uint32_t raw;
	struct {
		uint32_t gms_start_x                     : 13;
	} bits;
};

union reg_isp_gms_start_y {
	uint32_t raw;
	struct {
		uint32_t gms_start_y                     : 13;
	} bits;
};

union reg_isp_gms_location {
	uint32_t raw;
	struct {
		uint32_t gms_location                    : 32;
	} bits;
};

union reg_isp_gms_x_sizem1 {
	uint32_t raw;
	struct {
		uint32_t gms_x_sizem1                    : 10;
	} bits;
};

union reg_isp_gms_y_sizem1 {
	uint32_t raw;
	struct {
		uint32_t gms_y_sizem1                    : 10;
	} bits;
};

union reg_isp_gms_x_gap {
	uint32_t raw;
	struct {
		uint32_t gms_x_gap                       : 10;
	} bits;
};

union reg_isp_gms_y_gap {
	uint32_t raw;
	struct {
		uint32_t gms_y_gap                       : 10;
	} bits;
};

union reg_isp_gms_dummy {
	uint32_t raw;
	struct {
		uint32_t gms_dummy                       : 16;
	} bits;
};

union reg_isp_gms_mem_sw_mode {
	uint32_t raw;
	struct {
		uint32_t gms_mem_sw_mode                 : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t gms_mem_sel                     : 6;
	} bits;
};

union reg_isp_gms_mem_sw_raddr {
	uint32_t raw;
	struct {
		uint32_t gms_sw_raddr                    : 11;
	} bits;
};

union reg_isp_gms_mem_sw_rdata {
	uint32_t raw;
	struct {
		uint32_t gms_rdata_r                     : 31;
		uint32_t gms_sw_r                        : 1;
	} bits;
};

union reg_isp_gms_monitor_select {
	uint32_t raw;
	struct {
		uint32_t gms_monitor_select              : 32;
	} bits;
};

union reg_isp_gms_dmi_enable {
	uint32_t raw;
	struct {
		uint32_t dmi_enable                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t dmi_qos                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t force_dma_disable               : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_af_status {
	uint32_t raw;
	struct {
		uint32_t af_status                       : 32;
	} bits;
};

union reg_isp_af_grace_reset {
	uint32_t raw;
	struct {
		uint32_t af_grace_reset                  : 1;
	} bits;
};

union reg_isp_af_monitor {
	uint32_t raw;
	struct {
		uint32_t af_monitor                      : 32;
	} bits;
};

union reg_isp_af_bypass {
	uint32_t raw;
	struct {
		uint32_t af_bypass                       : 1;
	} bits;
};

union reg_isp_af_kickoff {
	uint32_t raw;
	struct {
		uint32_t af_enable                       : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t af_wbgain_apply                 : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t af_revert_exposure              : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t af_gain_enable                  : 1;
	} bits;
};

union reg_isp_af_enables {
	uint32_t raw;
	struct {
		uint32_t af_horizon_0_enable             : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t af_horizon_1_enable             : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t af_vertical_0_enable            : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t af_gamma_enable                 : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t af_dpc_enable                   : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t af_hlc_enable                   : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t af_shadow_select                : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_af_offset_x {
	uint32_t raw;
	struct {
		uint32_t af_offset_x                     : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t af_offset_y                     : 13;
	} bits;
};

union reg_isp_af_mxn_image_width_m1 {
	uint32_t raw;
	struct {
		uint32_t af_mxn_image_width              : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t af_mxn_image_height             : 13;
	} bits;
};

union reg_isp_af_block_width {
	uint32_t raw;
	struct {
		uint32_t af_block_width                  : 8;
	} bits;
};

union reg_isp_af_block_height {
	uint32_t raw;
	struct {
		uint32_t af_block_height                 : 8;
	} bits;
};

union reg_isp_af_block_num_x {
	uint32_t raw;
	struct {
		uint32_t af_block_num_x                  : 5;
	} bits;
};

union reg_isp_af_block_num_y {
	uint32_t raw;
	struct {
		uint32_t af_block_num_y                  : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t force_bayer_enable              : 1;
	} bits;
};

union reg_isp_af_hor_low_pass_value_shift {
	uint32_t raw;
	struct {
		uint32_t af_hor_low_pass_value_shift     : 4;
	} bits;
};

union reg_isp_af_corning_offset_horizontal_0 {
	uint32_t raw;
	struct {
		uint32_t af_corning_offset_horizontal_0  : 8;
	} bits;
};

union reg_isp_af_corning_offset_horizontal_1 {
	uint32_t raw;
	struct {
		uint32_t af_corning_offset_horizontal_1  : 8;
	} bits;
};

union reg_isp_af_corning_offset_vertical {
	uint32_t raw;
	struct {
		uint32_t af_corning_offset_vertical      : 8;
	} bits;
};

union reg_isp_af_high_y_thre {
	uint32_t raw;
	struct {
		uint32_t af_high_y_thre                  : 8;
	} bits;
};

union reg_isp_af_low_pass_horizon {
	uint32_t raw;
	struct {
		uint32_t af_low_pass_horizon_0           : 6;
		uint32_t af_low_pass_horizon_1           : 6;
		uint32_t af_low_pass_horizon_2           : 6;
		uint32_t af_low_pass_horizon_3           : 6;
		uint32_t af_low_pass_horizon_4           : 6;
	} bits;
};

union reg_isp_af_location {
	uint32_t raw;
	struct {
		uint32_t af_location                     : 32;
	} bits;
};

union reg_isp_af_high_pass_horizon_0 {
	uint32_t raw;
	struct {
		uint32_t af_high_pass_horizon_0_0        : 6;
		uint32_t af_high_pass_horizon_0_1        : 6;
		uint32_t af_high_pass_horizon_0_2        : 6;
		uint32_t af_high_pass_horizon_0_3        : 6;
		uint32_t af_high_pass_horizon_0_4        : 6;
	} bits;
};

union reg_isp_af_high_pass_horizon_1 {
	uint32_t raw;
	struct {
		uint32_t af_high_pass_horizon_1_0        : 6;
		uint32_t af_high_pass_horizon_1_1        : 6;
		uint32_t af_high_pass_horizon_1_2        : 6;
		uint32_t af_high_pass_horizon_1_3        : 6;
		uint32_t af_high_pass_horizon_1_4        : 6;
	} bits;
};

union reg_isp_af_high_pass_vertical_0 {
	uint32_t raw;
	struct {
		uint32_t af_high_pass_vertical_0_0       : 6;
		uint32_t af_high_pass_vertical_0_1       : 6;
		uint32_t af_high_pass_vertical_0_2       : 6;
	} bits;
};

union reg_isp_af_mem_sw_mode {
	uint32_t raw;
	struct {
		uint32_t af_mem_sw_mode                  : 1;
		uint32_t af_r_mem_sel                    : 1;
		uint32_t af_g_mem_sel                    : 1;
		uint32_t af_b_mem_sel                    : 1;
		uint32_t af_blk_div_mem_sel              : 1;
		uint32_t af_gamma_g_mem_sel              : 1;
		uint32_t af_magfactor_mem_sel            : 1;
		uint32_t af_blk_div_dff_sel              : 1;
		uint32_t af_gamma_g_dff_sel              : 1;
		uint32_t af_magfactor_dff_sel            : 1;
	} bits;
};

union reg_isp_af_monitor_select {
	uint32_t raw;
	struct {
		uint32_t af_monitor_select               : 32;
	} bits;
};

union reg_isp_af_image_width {
	uint32_t raw;
	struct {
		uint32_t af_image_width                  : 16;
	} bits;
};

union reg_isp_af_dummy {
	uint32_t raw;
	struct {
		uint32_t af_dummy                        : 16;
	} bits;
};

union reg_isp_af_mem_sw_raddr {
	uint32_t raw;
	struct {
		uint32_t af_sw_raddr                     : 7;
	} bits;
};

union reg_isp_af_mem_sw_rdata {
	uint32_t raw;
	struct {
		uint32_t af_rdata                        : 31;
		uint32_t af_sw_read                      : 1;
	} bits;
};

union reg_isp_af_mxn_border {
	uint32_t raw;
	struct {
		uint32_t af_mxn_border                   : 2;
	} bits;
};

union reg_isp_af_th_low    {
	uint32_t raw;
	struct {
		uint32_t af_th_low                       : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t af_th_high                      : 8;
	} bits;
};

union reg_isp_af_gain_low  {
	uint32_t raw;
	struct {
		uint32_t af_gain_low                     : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t af_gain_high                    : 8;
	} bits;
};

union reg_isp_af_slop_low {
	uint32_t raw;
	struct {
		uint32_t af_slop_low                     : 4;
		uint32_t af_slop_high                    : 4;
	} bits;
};

union reg_isp_af_dmi_enable {
	uint32_t raw;
	struct {
		uint32_t dmi_enable                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t dmi_qos                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t force_dma_disable               : 1;
	} bits;
};

union reg_isp_af_square_enable {
	uint32_t raw;
	struct {
		uint32_t af_square_enable                : 1;
	} bits;
};

union reg_isp_af_outshift {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 4;
		uint32_t af_outshift                     : 3;
	} bits;
};

union reg_isp_af_num_gapline {
	uint32_t raw;
	struct {
		uint32_t af_num_gapline                  : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_pre_raw_fe_pre_raw_ctrl {
	uint32_t raw;
	struct {
		uint32_t bayer_type_le                   : 4;
		uint32_t bayer_type_se                   : 4;
		uint32_t post_blc_bayer_type_le          : 4;
		uint32_t post_blc_bayer_type_se          : 4;
		uint32_t _rsv_16                         : 2;
		uint32_t up_pq_en                        : 1;
		uint32_t _rsv_19                         : 1;
		uint32_t rgbir_en                        : 1;
		uint32_t _rsv_21                         : 10;
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_pre_raw_fe_pre_raw_frame_size {
	uint32_t raw;
	struct {
		uint32_t frame_widthm1                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t frame_heightm1                  : 14;
	} bits;
};

union reg_pre_raw_fe_le_rgbmap_grid_number {
	uint32_t raw;
	struct {
		uint32_t le_rgbmp_h_grid_numm1           : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t le_rgbmp_h_grid_size            : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t le_rgbmp_v_grid_numm1           : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t le_rgbmp_v_grid_size            : 3;
	} bits;
};

union reg_pre_raw_fe_se_rgbmap_grid_number {
	uint32_t raw;
	struct {
		uint32_t se_rgbmp_h_grid_numm1           : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t se_rgbmp_h_grid_size            : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t se_rgbmp_v_grid_numm1           : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t se_rgbmp_v_grid_size            : 3;
	} bits;
};

union reg_pre_raw_fe_pre_raw_post_no_rspd_cyc {
	uint32_t raw;
	struct {
		uint32_t post_no_rspd_cyc                : 32;
	} bits;
};

union reg_pre_raw_fe_pre_raw_post_rgbmap_no_rspd_cyc {
	uint32_t raw;
	struct {
		uint32_t post_rgbmap_no_rspd_cyc         : 32;
	} bits;
};

union reg_pre_raw_fe_pre_raw_frame_vld {
	uint32_t raw;
	struct {
		uint32_t fe_frame_vld_ch0                : 1;
		uint32_t fe_frame_vld_ch1                : 1;
		uint32_t fe_frame_vld_ch2                : 1;
		uint32_t fe_frame_vld_ch3                : 1;
		uint32_t fe_pq_vld_ch0                   : 1;
		uint32_t fe_pq_vld_ch1                   : 1;
		uint32_t fe_pq_vld_ch2                   : 1;
		uint32_t fe_pq_vld_ch3                   : 1;
		uint32_t _rsv_8                          : 8;
		uint32_t post_raw_idle                   : 1;
	} bits;
};

union reg_pre_raw_fe_pre_raw_debug_state {
	uint32_t raw;
	struct {
		uint32_t pre_raw_fe_idle                 : 32;
	} bits;
};

union reg_pre_raw_fe_pre_raw_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy_rw                        : 16;
		uint32_t dummy_ro                        : 16;
	} bits;
};

union reg_pre_raw_fe_pre_raw_debug_info {
	uint32_t raw;
	struct {
		uint32_t ip_frame_done_sts               : 32;
	} bits;
};

union reg_pre_raw_fe_idle_info {
	uint32_t raw;
	struct {
		uint32_t ip_dma_idle                     : 32;
	} bits;
};

union reg_pre_raw_fe_check_sum {
	uint32_t raw;
	struct {
		uint32_t lexp_chksum_enable              : 1;
		uint32_t sexp_chksum_enable              : 1;
	} bits;
};

union reg_pre_raw_fe_check_sum_value {
	uint32_t raw;
	struct {
		uint32_t lexp_chksum_value               : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t sexp_chksum_value               : 8;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_csi_bdg_dvp_bdg_top_ctrl {
	uint32_t raw;
	struct {
		uint32_t csi_mode                        : 2;
		uint32_t csi_in_format                   : 1;
		uint32_t csi_in_yuv_format               : 1;
		uint32_t ch_num                          : 2;
		uint32_t ch0_dma_wr_enable               : 1;
		uint32_t _rsv_7                          : 2;
		uint32_t y_only                          : 1;
		uint32_t pxl_data_sel                    : 1;
		uint32_t vs_pol                          : 1;
		uint32_t hs_pol                          : 1;
		uint32_t reset_mode                      : 1;
		uint32_t vs_mode                         : 1;
		uint32_t abort_mode                      : 1;
		uint32_t reset                           : 1;
		uint32_t abort                           : 1;
		uint32_t _rsv_18                         : 2;
		uint32_t yuv_pack_mode                   : 1;
		uint32_t multi_ch_frame_sync_en          : 1;
		uint32_t ch0_dma_420_wr_enable           : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t mcsi_enable                      : 1;
		uint32_t tgen_enable                     : 1;
		uint32_t yuv2bay_enable                  : 1;
		uint32_t _rsv_27                         : 1;
		uint32_t shdw_read_sel                   : 1;
		uint32_t _rsv_29                         : 2;
		uint32_t csi_up_reg                      : 1;
	} bits;
};

union reg_isp_csi_bdg_dvp_bdg_interrupt_ctrl {
	uint32_t raw;
	struct {
		uint32_t ch0_vs_int_en                   : 1;
		uint32_t ch0_trig_int_en                 : 1;
		uint32_t ch0_drop_int_en                 : 1;
		uint32_t ch0_size_error_int_en           : 1;
		uint32_t _rsv_4                          : 24;
		uint32_t slice_line_intp_en              : 1;
		uint32_t dma_error_intp_en               : 1;
		uint32_t line_intp_en                    : 1;
		uint32_t fifo_overflow_int_en            : 1;
	} bits;
};

union reg_isp_csi_bdg_dvp_bdg_dma_dpcm_mode {
	uint32_t raw;
	struct {
		uint32_t dma_st_dpcm_mode                : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t dpcm_mipi_opt                   : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t drop_mode                       : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t avg_mode                        : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t dpcm_xstr                       : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_bdg_dma_ld_dpcm_mode {
	uint32_t raw;
	struct {
		uint32_t dma_ld_dpcm_mode                : 3;
		uint32_t _rsv_3                          : 13;
		uint32_t dpcm_rx_xstr                    : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_ch0_size {
	uint32_t raw;
	struct {
		uint32_t ch0_frame_widthm1               : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t ch0_frame_heightm1              : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_ch0_crop_en {
	uint32_t raw;
	struct {
		uint32_t ch0_crop_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_dvp_ch0_horz_crop {
	uint32_t raw;
	struct {
		uint32_t ch0_horz_crop_start             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t ch0_horz_crop_end               : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_ch0_vert_crop {
	uint32_t raw;
	struct {
		uint32_t ch0_vert_crop_start             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t ch0_vert_crop_end               : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_ch0_blc_sum {
	uint32_t raw;
	struct {
		uint32_t ch0_blc_sum                     : 32;
	} bits;
};

union reg_isp_csi_bdg_dvp_pat_gen_ctrl {
	uint32_t raw;
	struct {
		uint32_t pat_en                          : 1;
		uint32_t gra_inv                         : 1;
		uint32_t auto_en                         : 1;
		uint32_t dith_en                         : 1;
		uint32_t snow_en                         : 1;
		uint32_t fix_mc                          : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t dith_md                         : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t bayer_id                        : 2;
	} bits;
};

union reg_isp_csi_bdg_dvp_pat_idx_ctrl {
	uint32_t raw;
	struct {
		uint32_t pat_prd                         : 8;
		uint32_t pat_idx                         : 5;
	} bits;
};

union reg_isp_csi_bdg_dvp_pat_color_0 {
	uint32_t raw;
	struct {
		uint32_t pat_r                           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t pat_g                           : 12;
	} bits;
};

union reg_isp_csi_bdg_dvp_pat_color_1 {
	uint32_t raw;
	struct {
		uint32_t pat_b                           : 12;
	} bits;
};

union reg_isp_csi_bdg_dvp_background_color_0 {
	uint32_t raw;
	struct {
		uint32_t fde_r                           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t fde_g                           : 12;
	} bits;
};

union reg_isp_csi_bdg_dvp_background_color_1 {
	uint32_t raw;
	struct {
		uint32_t fde_b                           : 12;
	} bits;
};

union reg_isp_csi_bdg_dvp_fix_color_0 {
	uint32_t raw;
	struct {
		uint32_t mde_r                           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mde_g                           : 12;
	} bits;
};

union reg_isp_csi_bdg_dvp_fix_color_1 {
	uint32_t raw;
	struct {
		uint32_t mde_b                           : 12;
	} bits;
};

union reg_isp_csi_bdg_dvp_mde_v_size {
	uint32_t raw;
	struct {
		uint32_t vmde_str                        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t vmde_stp                        : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_mde_h_size {
	uint32_t raw;
	struct {
		uint32_t hmde_str                        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t hmde_stp                        : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_fde_v_size {
	uint32_t raw;
	struct {
		uint32_t vfde_str                        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t vfde_stp                        : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_fde_h_size {
	uint32_t raw;
	struct {
		uint32_t hfde_str                        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t hfde_stp                        : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_hsync_ctrl {
	uint32_t raw;
	struct {
		uint32_t hs_str                          : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t hs_stp                          : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_vsync_ctrl {
	uint32_t raw;
	struct {
		uint32_t vs_str                          : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t vs_stp                          : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_tgen_tt_size {
	uint32_t raw;
	struct {
		uint32_t htt                             : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t vtt                             : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_line_intp_height_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_line_intp_heightm1          : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_ch0_debug_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_pxl_cnt                     : 32;
	} bits;
};

union reg_isp_csi_bdg_dvp_ch0_debug_1 {
	uint32_t raw;
	struct {
		uint32_t ch0_line_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_dvp_ch0_debug_2 {
	uint32_t raw;
	struct {
		uint32_t ch0_vs_cnt                      : 16;
		uint32_t ch0_trig_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_dvp_ch0_debug_3 {
	uint32_t raw;
	struct {
		uint32_t ch0_tot_blk_idle                : 1;
		uint32_t ch0_tot_dma_idle                : 1;
		uint32_t ch0_bdg_dma_idle                : 1;
	} bits;
};

union reg_isp_csi_bdg_dvp_interrupt_status_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_frame_drop_int              : 1;
		uint32_t ch0_vs_int                      : 1;
		uint32_t ch0_trig_int                    : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t ch0_frame_width_gt_int          : 1;
		uint32_t ch0_frame_width_ls_int          : 1;
		uint32_t ch0_frame_height_gt_int         : 1;
		uint32_t ch0_frame_height_ls_int         : 1;
	} bits;
};

union reg_isp_csi_bdg_dvp_interrupt_status_1 {
	uint32_t raw;
	struct {
		uint32_t fifo_overflow_int               : 1;
		uint32_t frame_resolution_over_max_int   : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t dma_error_int                   : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t ch0_line_intp_int               : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t ch0_slice_line_intp_int         : 1;
	} bits;
};

union reg_isp_csi_bdg_dvp_bdg_debug {
	uint32_t raw;
	struct {
		uint32_t ring_buff_idle                  : 1;
	} bits;
};

union reg_isp_csi_bdg_dvp_out_vsync_line_delay {
	uint32_t raw;
	struct {
		uint32_t out_vsync_line_delay            : 12;
	} bits;
};

union reg_isp_csi_bdg_dvp_wr_urgent_ctrl {
	uint32_t raw;
	struct {
		uint32_t wr_near_overflow_threshold      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t wr_safe_threshold               : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_rd_urgent_ctrl {
	uint32_t raw;
	struct {
		uint32_t rd_near_overflow_threshold      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t rd_safe_threshold               : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy_in                        : 16;
		uint32_t dummy_out                       : 16;
	} bits;
};

union reg_isp_csi_bdg_dvp_slice_line_intp_height_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_slice_line_intp_heightm1    : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_wdma_ch0_crop_en {
	uint32_t raw;
	struct {
		uint32_t st_ch0_crop_en                  : 1;
	} bits;
};

union reg_isp_csi_bdg_dvp_wdma_ch0_horz_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch0_horz_crop_start          : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t st_ch0_horz_crop_end            : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_wdma_ch0_vert_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch0_vert_crop_start          : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t st_ch0_vert_crop_end            : 13;
	} bits;
};

union reg_isp_csi_bdg_dvp_trig_dly_control_0 {
	uint32_t raw;
	struct {
		uint32_t trig_dly_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_dvp_trig_dly_control_1 {
	uint32_t raw;
	struct {
		uint32_t trig_dly_value                  : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_crop_0 {
	uint32_t raw;
	struct {
		uint32_t crop_enable                     : 1;
		uint32_t dma_enable                      : 1;
		uint32_t shaw_read_sel                   : 1;
		uint32_t dmi_qos                         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t dpcm_mode                       : 3;
		uint32_t _rsv_11                         : 5;
		uint32_t dpcm_xstr                       : 13;
		uint32_t _rsv_29                         : 2;
		uint32_t dmi16b_en                       : 1;
	} bits;
};

union reg_crop_1 {
	uint32_t raw;
	struct {
		uint32_t crop_start_y                    : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_end_y                      : 14;
	} bits;
};

union reg_crop_2 {
	uint32_t raw;
	struct {
		uint32_t crop_start_x                    : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_end_x                      : 14;
	} bits;
};

union reg_crop_3 {
	uint32_t raw;
	struct {
		uint32_t in_widthm1                      : 16;
		uint32_t in_heightm1                     : 16;
	} bits;
};

union reg_crop_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy                           : 32;
	} bits;
};

union reg_crop_debug {
	uint32_t raw;
	struct {
		uint32_t force_clk_enable                : 1;
		uint32_t force_dma_disable               : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_blc_0 {
	uint32_t raw;
	struct {
		uint32_t blc_bypass                      : 1;
	} bits;
};

union reg_isp_blc_1 {
	uint32_t raw;
	struct {
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_isp_blc_2 {
	uint32_t raw;
	struct {
		uint32_t blc_enable                      : 1;
		uint32_t _rsv_1                          : 27;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_blc_3 {
	uint32_t raw;
	struct {
		uint32_t blc_offset_r                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t blc_offset_gr                   : 12;
	} bits;
};

union reg_isp_blc_4 {
	uint32_t raw;
	struct {
		uint32_t blc_offset_gb                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t blc_offset_b                    : 12;
	} bits;
};

union reg_isp_blc_5 {
	uint32_t raw;
	struct {
		uint32_t blc_gain_r                      : 16;
		uint32_t blc_gain_gr                     : 16;
	} bits;
};

union reg_isp_blc_6 {
	uint32_t raw;
	struct {
		uint32_t blc_gain_gb                     : 16;
		uint32_t blc_gain_b                      : 16;
	} bits;
};

union reg_isp_blc_7 {
	uint32_t raw;
	struct {
		uint32_t blc_checksum                    : 32;
	} bits;
};

union reg_isp_blc_8 {
	uint32_t raw;
	struct {
		uint32_t blc_int                         : 1;
		uint32_t _rsv_1                          : 15;
		uint32_t force_bayer_enable              : 1;
	} bits;
};

union reg_isp_blc_dummy {
	uint32_t raw;
	struct {
		uint32_t blc_dummy                       : 16;
	} bits;
};

union reg_isp_blc_location {
	uint32_t raw;
	struct {
		uint32_t blc_location                    : 32;
	} bits;
};

union reg_isp_blc_9 {
	uint32_t raw;
	struct {
		uint32_t blc_2ndoffset_r                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t blc_2ndoffset_gr                : 12;
	} bits;
};

union reg_isp_blc_a {
	uint32_t raw;
	struct {
		uint32_t blc_2ndoffset_gb                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t blc_2ndoffset_b                 : 12;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_lmap_lmp_0 {
	uint32_t raw;
	struct {
		uint32_t lmap_enable                     : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t lmap_y_mode                     : 2;
		uint32_t lmap_thd_l                      : 8;
		uint32_t lmap_thd_h                      : 8;
		uint32_t _rsv_22                         : 8;
		uint32_t lmap_softrst                    : 1;
		uint32_t force_dma_disable               : 1;
	} bits;
};

union reg_isp_lmap_lmp_1 {
	uint32_t raw;
	struct {
		uint32_t lmap_crop_widthm1               : 13;
		uint32_t lmap_crop_heightm1              : 13;
		uint32_t _rsv_26                         : 1;
		uint32_t lmap_bayer_id                   : 4;
		uint32_t lmap_shdw_sel                   : 1;
	} bits;
};

union reg_isp_lmap_lmp_2 {
	uint32_t raw;
	struct {
		uint32_t lmap_w_grid_num                 : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t lmap_w_bit                      : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t lmap_h_grid_num                 : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t lmap_h_bit                      : 3;
		uint32_t lmap_out_sel                    : 1;
	} bits;
};

union reg_isp_lmap_lmp_debug_0 {
	uint32_t raw;
	struct {
		uint32_t lmap_debug_0                    : 32;
	} bits;
};

union reg_isp_lmap_lmp_debug_1 {
	uint32_t raw;
	struct {
		uint32_t lmap_debug_1                    : 32;
	} bits;
};

union reg_isp_lmap_dummy {
	uint32_t raw;
	struct {
		uint32_t lmap_dummy                      : 32;
	} bits;
};

union reg_isp_lmap_lmp_debug_2 {
	uint32_t raw;
	struct {
		uint32_t lmap_debug_2                    : 32;
	} bits;
};

union reg_isp_lmap_lmp_3 {
	uint32_t raw;
	struct {
		uint32_t debug_data_sel                  : 2;
	} bits;
};

union reg_isp_lmap_lmp_4 {
	uint32_t raw;
	struct {
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_lmap_lmp_set_sel {
	uint32_t raw;
	struct {
		uint32_t set_sel                         : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_rgbmap_0 {
	uint32_t raw;
	struct {
		uint32_t rgbmap_enable                   : 1;
		uint32_t rgbmap_w_bit                    : 3;
		uint32_t rgbmap_h_bit                    : 3;
		uint32_t img_bayerid                     : 4;
		uint32_t rgbmap_w_grid_num               : 10;
		uint32_t rgbmap_h_grid_num               : 10;
		uint32_t rgbmap_softrst                  : 1;
	} bits;
};

union reg_isp_rgbmap_1 {
	uint32_t raw;
	struct {
		uint32_t img_widthm1                     : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t img_heightm1                    : 13;
		uint32_t rgbmap_shdw_sel                 : 1;
	} bits;
};

union reg_isp_rgbmap_debug_0 {
	uint32_t raw;
	struct {
		uint32_t rgbmap_debug_0                  : 32;
	} bits;
};

union reg_isp_rgbmap_debug_1 {
	uint32_t raw;
	struct {
		uint32_t rgbmap_debug_1                  : 32;
	} bits;
};

union reg_isp_rgbmap_dummy {
	uint32_t raw;
	struct {
		uint32_t rgbmap_dummy                    : 32;
	} bits;
};

union reg_isp_rgbmap_2 {
	uint32_t raw;
	struct {
		uint32_t force_dma_disable               : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t debug_data_sel                  : 2;
	} bits;
};

union reg_isp_rgbmap_debug_2 {
	uint32_t raw;
	struct {
		uint32_t rgbmap_debug_2                  : 32;
	} bits;
};

union reg_isp_rgbmap_3 {
	uint32_t raw;
	struct {
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_rgbmap_set_sel {
	uint32_t raw;
	struct {
		uint32_t set_sel                         : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_wbg_0 {
	uint32_t raw;
	struct {
		uint32_t wbg_bypass                      : 1;
	} bits;
};

union reg_isp_wbg_1 {
	uint32_t raw;
	struct {
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_isp_wbg_2 {
	uint32_t raw;
	struct {
		uint32_t wbg_enable                      : 1;
		uint32_t _rsv_1                          : 27;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_wbg_4 {
	uint32_t raw;
	struct {
		uint32_t wbg_rgain                       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t wbg_ggain                       : 14;
	} bits;
};

union reg_isp_wbg_5 {
	uint32_t raw;
	struct {
		uint32_t wbg_bgain                       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t wbg_irgain                      : 14;
	} bits;
};

union reg_isp_wbg_6 {
	uint32_t raw;
	struct {
		uint32_t wbg_checksum                    : 32;
	} bits;
};

union reg_isp_wbg_7 {
	uint32_t raw;
	struct {
		uint32_t wbg_int                         : 1;
	} bits;
};

union reg_isp_wbg_img_bayerid {
	uint32_t raw;
	struct {
		uint32_t img_bayerid                     : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t force_bayer_enable              : 1;
	} bits;
};

union reg_isp_wbg_dummy {
	uint32_t raw;
	struct {
		uint32_t wbg_dummy                       : 32;
	} bits;
};

union reg_isp_wbg_location {
	uint32_t raw;
	struct {
		uint32_t wbg_location                    : 32;
	} bits;
};

union reg_isp_wbg_34 {
	uint32_t raw;
	struct {
		uint32_t rgain_fraction                  : 24;
	} bits;
};

union reg_isp_wbg_38 {
	uint32_t raw;
	struct {
		uint32_t ggain_fraction                  : 24;
	} bits;
};

union reg_isp_wbg_3c {
	uint32_t raw;
	struct {
		uint32_t bgain_fraction                  : 24;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_dpc_0 {
	uint32_t raw;
	struct {
		uint32_t prog_hdk_dis                    : 1;
		uint32_t cont_en                         : 1;
		uint32_t softrst                         : 1;
		uint32_t dbg_en                          : 1;
		uint32_t ch_nm                           : 1;
	} bits;
};

union reg_isp_dpc_1 {
	uint32_t raw;
	struct {
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_isp_dpc_2 {
	uint32_t raw;
	struct {
		uint32_t dpc_enable                      : 1;
		uint32_t ge_enable                       : 1;
		uint32_t dpc_dynamicbpc_enable           : 1;
		uint32_t dpc_staticbpc_enable            : 1;
		uint32_t delay                           : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t force_clk_enable                : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t dpc_cluster_size                : 2;
	} bits;
};

union reg_isp_dpc_3 {
	uint32_t raw;
	struct {
		uint32_t dpc_r_bright_pixel_ratio        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t dpc_g_bright_pixel_ratio        : 10;
	} bits;
};

union reg_isp_dpc_4 {
	uint32_t raw;
	struct {
		uint32_t dpc_b_bright_pixel_ratio        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t dpc_r_dark_pixel_ratio          : 10;
	} bits;
};

union reg_isp_dpc_5 {
	uint32_t raw;
	struct {
		uint32_t dpc_g_dark_pixel_ratio          : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t dpc_b_dark_pixel_ratio          : 10;
	} bits;
};

union reg_isp_dpc_6 {
	uint32_t raw;
	struct {
		uint32_t dpc_r_dark_pixel_mindiff        : 8;
		uint32_t dpc_g_dark_pixel_mindiff        : 8;
		uint32_t dpc_b_dark_pixel_mindiff        : 8;
	} bits;
};

union reg_isp_dpc_7 {
	uint32_t raw;
	struct {
		uint32_t dpc_r_bright_pixel_upboud_ratio : 8;
		uint32_t dpc_g_bright_pixel_upboud_ratio : 8;
		uint32_t dpc_b_bright_pixel_upboud_ratio : 8;
	} bits;
};

union reg_isp_dpc_8 {
	uint32_t raw;
	struct {
		uint32_t dpc_flat_thre_min_rb            : 8;
		uint32_t dpc_flat_thre_min_g             : 8;
	} bits;
};

union reg_isp_dpc_9 {
	uint32_t raw;
	struct {
		uint32_t dpc_flat_thre_r                 : 8;
		uint32_t dpc_flat_thre_g                 : 8;
		uint32_t dpc_flat_thre_b                 : 8;
	} bits;
};

union reg_isp_dpc_10 {
	uint32_t raw;
	struct {
		uint32_t ge_strength                     : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t ge_combineweight                : 4;
	} bits;
};

union reg_isp_dpc_11 {
	uint32_t raw;
	struct {
		uint32_t ge_thre1                        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ge_thre2                        : 12;
	} bits;
};

union reg_isp_dpc_12 {
	uint32_t raw;
	struct {
		uint32_t ge_thre3                        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ge_thre4                        : 12;
	} bits;
};

union reg_isp_dpc_13 {
	uint32_t raw;
	struct {
		uint32_t ge_thre11                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ge_thre21                       : 12;
	} bits;
};

union reg_isp_dpc_14 {
	uint32_t raw;
	struct {
		uint32_t ge_thre31                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ge_thre41                       : 12;
	} bits;
};

union reg_isp_dpc_15 {
	uint32_t raw;
	struct {
		uint32_t ge_thre12                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ge_thre22                       : 12;
	} bits;
};

union reg_isp_dpc_16 {
	uint32_t raw;
	struct {
		uint32_t ge_thre32                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ge_thre42                       : 12;
	} bits;
};

union reg_isp_dpc_17 {
	uint32_t raw;
	struct {
		uint32_t dpc_mem0_img0_addr              : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t dpc_mem0_img1_addr              : 11;
		uint32_t _rsv_27                         : 3;
		uint32_t dpc_mem0_img_addr_sel           : 1;
		uint32_t dpc_mem_prog_mode               : 1;
	} bits;
};

union reg_isp_dpc_18 {
	uint32_t raw;
	struct {
		uint32_t dpc_sw_raddr                    : 12;
	} bits;
};

union reg_isp_dpc_19 {
	uint32_t raw;
	struct {
		uint32_t dpc_rdata_r                     : 24;
		uint32_t _rsv_24                         : 7;
		uint32_t dpc_sw_r                        : 1;
	} bits;
};

union reg_isp_dpc_mem_w0 {
	uint32_t raw;
	struct {
		uint32_t dpc_bp_mem_d                    : 26;
		uint32_t _rsv_26                         : 5;
		uint32_t dpc_bp_mem_w                    : 1;
	} bits;
};

union reg_isp_dpc_window {
	uint32_t raw;
	struct {
		uint32_t img_wd                          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t img_ht                          : 12;
	} bits;
};

union reg_isp_dpc_mem_st_addr {
	uint32_t raw;
	struct {
		uint32_t dpc_bp_mem_st_addr              : 11;
		uint32_t _rsv_11                         : 20;
		uint32_t dpc_bp_mem_st_addr_w            : 1;
	} bits;
};

union reg_isp_dpc_checksum {
	uint32_t raw;
	struct {
		uint32_t dpc_checksum                    : 32;
	} bits;
};

union reg_isp_dpc_int {
	uint32_t raw;
	struct {
		uint32_t dpc_int                         : 1;
	} bits;
};

union reg_isp_dpc_20 {
	uint32_t raw;
	struct {
		uint32_t prob_out_sel                    : 4;
		uint32_t prob_perfmt                     : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t prob_fmt                        : 6;
	} bits;
};

union reg_isp_dpc_21 {
	uint32_t raw;
	struct {
		uint32_t prob_line                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t prob_pix                        : 12;
	} bits;
};

union reg_isp_dpc_22 {
	uint32_t raw;
	struct {
		uint32_t dpc_dbg0                        : 32;
	} bits;
};

union reg_isp_dpc_23 {
	uint32_t raw;
	struct {
		uint32_t dpc_dbg1                        : 32;
	} bits;
};

union reg_isp_dpc_24 {
	uint32_t raw;
	struct {
		uint32_t dpc_ir_bright_pixel_ratio       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t dpc_ir_dark_pixel_ratio         : 10;
	} bits;
};

union reg_isp_dpc_25 {
	uint32_t raw;
	struct {
		uint32_t dpc_ir_dark_pixel_mindiff       : 8;
		uint32_t dpc_ir_bright_pixel_upboud_ratio: 8;
		uint32_t dpc_flat_thre_min_ir            : 8;
		uint32_t dpc_flat_thre_ir                : 8;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_raw_top_raw_0 {
	uint32_t raw;
	struct {
		uint32_t svn_version                     : 32;
	} bits;
};

union reg_raw_top_read_sel {
	uint32_t raw;
	struct {
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_raw_top_raw_1 {
	uint32_t raw;
	struct {
		uint32_t timestamp                       : 32;
	} bits;
};

union reg_raw_top_ctrl {
	uint32_t raw;
	struct {
		uint32_t ls_crop_dst_sel                 : 1;
		uint32_t _rsv_1                          : 15;
		uint32_t frame_done_sel_tail0_all1       : 1;
	} bits;
};

union reg_raw_top_up_pq_en {
	uint32_t raw;
	struct {
		uint32_t up_pq_en                        : 1;
		uint32_t _rsv_1                          : 15;
		uint32_t chk_sum_en                      : 1;
	} bits;
};

union reg_raw_top_raw_2 {
	uint32_t raw;
	struct {
		uint32_t img_widthm1                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_heightm1                    : 14;
	} bits;
};

union reg_raw_top_dummy {
	uint32_t raw;
	struct {
		uint32_t raw_top_dummy                   : 32;
	} bits;
};

union reg_raw_top_raw_4 {
	uint32_t raw;
	struct {
		uint32_t yuv_in_mode                     : 1;
	} bits;
};

union reg_raw_top_status {
	uint32_t raw;
	struct {
		uint32_t raw_top_status                  : 32;
	} bits;
};

union reg_raw_top_debug {
	uint32_t raw;
	struct {
		uint32_t raw_top_debug                   : 32;
	} bits;
};

union reg_raw_top_debug_select {
	uint32_t raw;
	struct {
		uint32_t raw_top_debug_select            : 32;
	} bits;
};

union reg_raw_top_raw_bayer_type_topleft {
	uint32_t raw;
	struct {
		uint32_t bayer_type_topleft              : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t rgbir_enable                    : 1;
	} bits;
};

union reg_raw_top_rdmi_enable {
	uint32_t raw;
	struct {
		uint32_t rdmi_en                         : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t pass_sel                        : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t pass_cnt_m1                     : 8;
		uint32_t ch_num                          : 1;
	} bits;
};

union reg_raw_top_rdma_size {
	uint32_t raw;
	struct {
		uint32_t rdmi_widthm1                    : 16;
		uint32_t rdmi_heightm1                   : 16;
	} bits;
};

union reg_raw_top_dpcm_mode {
	uint32_t raw;
	struct {
		uint32_t dpcm_mode                       : 3;
		uint32_t _rsv_3                          : 5;
		uint32_t dpcm_xstr                       : 13;
	} bits;
};

union reg_raw_top_stvalid_status {
	uint32_t raw;
	struct {
		uint32_t stvalid_status                  : 32;
	} bits;
};

union reg_raw_top_stready_status {
	uint32_t raw;
	struct {
		uint32_t stready_status                  : 32;
	} bits;
};

union reg_raw_top_patgen1 {
	uint32_t raw;
	struct {
		uint32_t x_curser                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t y_curser                        : 14;
		uint32_t curser_en                       : 1;
		uint32_t pg_enable                       : 1;
	} bits;
};

union reg_raw_top_patgen2 {
	uint32_t raw;
	struct {
		uint32_t curser_value                    : 16;
	} bits;
};

union reg_raw_top_patgen3 {
	uint32_t raw;
	struct {
		uint32_t value_report_le                 : 32;
	} bits;
};

union reg_raw_top_patgen4 {
	uint32_t raw;
	struct {
		uint32_t xcnt_rpt_le                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ycnt_rpt_le                     : 14;
		uint32_t pg_lcac_enable                  : 1;
		uint32_t curser_lcac_en                  : 1;
	} bits;
};

union reg_raw_top_ro_idle {
	uint32_t raw;
	struct {
		uint32_t raw_top_ro_idle                 : 32;
	} bits;
};

union reg_raw_top_ro_done {
	uint32_t raw;
	struct {
		uint32_t raw_top_ro_done                 : 32;
	} bits;
};

union reg_raw_top_dma_idle {
	uint32_t raw;
	struct {
		uint32_t raw_top_dma_idle                : 32;
	} bits;
};

union reg_raw_top_le_lmap_grid_number {
	uint32_t raw;
	struct {
		uint32_t le_lmp_h_grid_numm1             : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t le_lmp_h_grid_size              : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t le_lmp_v_grid_numm1             : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t le_lmp_v_grid_size              : 3;
	} bits;
};

union reg_raw_top_se_lmap_grid_number {
	uint32_t raw;
	struct {
		uint32_t se_lmp_h_grid_numm1             : 10;
		uint32_t _rsv_10                         : 2;
		uint32_t se_lmp_h_grid_size              : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t se_lmp_v_grid_numm1             : 10;
		uint32_t _rsv_26                         : 2;
		uint32_t se_lmp_v_grid_size              : 3;
	} bits;
};

union reg_raw_top_checksum_0 {
	uint32_t raw;
	struct {
		uint32_t raw_top_checksum_0              : 32;
	} bits;
};

union reg_raw_top_checksum_1 {
	uint32_t raw;
	struct {
		uint32_t raw_top_checksum_1              : 32;
	} bits;
};

union reg_raw_top_patgen5 {
	uint32_t raw;
	struct {
		uint32_t xcnt_rpt_se                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ycnt_rpt_se                     : 14;
		uint32_t pg_lcac_enable                  : 1;
		uint32_t curser_lcac_en                  : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_rgbcac_ctrl {
	uint32_t raw;
	struct {
		uint32_t rgbcac_enable                   : 1;
		uint32_t rgbcac_out_sel                  : 2;
		uint32_t rgbcac_shdw_sel                 : 1;
		uint32_t force_clk_enable                : 1;
		uint32_t softrst                         : 1;
	} bits;
};

union reg_isp_rgbcac_purple_th {
	uint32_t raw;
	struct {
		uint32_t rgbcac_purple_th_le             : 8;
		uint32_t rgbcac_purple_th_se             : 8;
		uint32_t rgbcac_correct_strength_le      : 8;
		uint32_t rgbcac_correct_strength_se      : 8;
	} bits;
};

union reg_isp_rgbcac_purple_cbcr {
	uint32_t raw;
	struct {
		uint32_t rgbcac_purple_cb                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbcac_purple_cr                : 12;
	} bits;
};

union reg_isp_rgbcac_purple_cbcr2 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_purple_cb2               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbcac_purple_cr2               : 12;
	} bits;
};

union reg_isp_rgbcac_purple_cbcr3 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_purple_cb3               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbcac_purple_cr3               : 12;
	} bits;
};

union reg_isp_rgbcac_green_cbcr {
	uint32_t raw;
	struct {
		uint32_t rgbcac_green_cb                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbcac_green_cr                 : 12;
	} bits;
};

union reg_isp_rgbcac_edge_coring {
	uint32_t raw;
	struct {
		uint32_t rgbcac_edge_coring              : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbcac_edge_scale               : 12;
	} bits;
};

union reg_isp_rgbcac_depurple_str_ratio_min {
	uint32_t raw;
	struct {
		uint32_t rgbcac_depurple_str_ratio_min_le: 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbcac_depurple_str_ratio_min_se: 12;
	} bits;
};

union reg_isp_rgbcac_depurple_str_ratio_max {
	uint32_t raw;
	struct {
		uint32_t rgbcac_depurple_str_ratio_max_le: 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbcac_depurple_str_ratio_max_se: 12;
	} bits;
};

union reg_isp_rgbcac_edge_wgt_lut0 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_edge_wgt_lut_00          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t rgbcac_edge_wgt_lut_01          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t rgbcac_edge_wgt_lut_02          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t rgbcac_edge_wgt_lut_03          : 6;
	} bits;
};

union reg_isp_rgbcac_edge_wgt_lut1 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_edge_wgt_lut_04          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t rgbcac_edge_wgt_lut_05          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t rgbcac_edge_wgt_lut_06          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t rgbcac_edge_wgt_lut_07          : 6;
	} bits;
};

union reg_isp_rgbcac_edge_wgt_lut2 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_edge_wgt_lut_08          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t rgbcac_edge_wgt_lut_09          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t rgbcac_edge_wgt_lut_10          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t rgbcac_edge_wgt_lut_11          : 6;
	} bits;
};

union reg_isp_rgbcac_edge_wgt_lut3 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_edge_wgt_lut_12          : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t rgbcac_edge_wgt_lut_13          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t rgbcac_edge_wgt_lut_14          : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t rgbcac_edge_wgt_lut_15          : 6;
	} bits;
};

union reg_isp_rgbcac_edge_wgt_lut4 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_edge_wgt_lut_16          : 6;
	} bits;
};

union reg_isp_rgbcac_luma {
	uint32_t raw;
	struct {
		uint32_t rgbcac_luma_scale               : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t rgbcac_luma2                    : 12;
	} bits;
};

union reg_isp_rgbcac_luma_blend {
	uint32_t raw;
	struct {
		uint32_t rgbcac_luma_blend_wgt           : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t rgbcac_luma_blend_wgt2          : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t rgbcac_luma_blend_wgt3          : 6;
	} bits;
};

union reg_isp_rgbcac_luma_filter0 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_luma_filter_00           : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t rgbcac_luma_filter_01           : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t rgbcac_luma_filter_02           : 7;
	} bits;
};

union reg_isp_rgbcac_luma_filter1 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_luma_filter_03           : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t rgbcac_luma_filter_04           : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t rgbcac_luma_filter_05           : 7;
	} bits;
};

union reg_isp_rgbcac_var_filter0 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_var_filter_00            : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t rgbcac_var_filter_01            : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t rgbcac_var_filter_02            : 7;
	} bits;
};

union reg_isp_rgbcac_var_filter1 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_var_filter_03            : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t rgbcac_var_filter_04            : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t rgbcac_var_filter_05            : 7;
	} bits;
};

union reg_isp_rgbcac_chroma_filter0 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_chroma_filter_00         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t rgbcac_chroma_filter_01         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t rgbcac_chroma_filter_02         : 7;
	} bits;
};

union reg_isp_rgbcac_chroma_filter1 {
	uint32_t raw;
	struct {
		uint32_t rgbcac_chroma_filter_03         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t rgbcac_chroma_filter_04         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t rgbcac_chroma_filter_05         : 7;
	} bits;
};

union reg_isp_rgbcac_cbcr_str {
	uint32_t raw;
	struct {
		uint32_t rgbcac_cb_str_le                : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t rgbcac_cr_str_le                : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t rgbcac_cb_str_se                : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t rgbcac_cr_str_se                : 5;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_bnr_shadow_rd_sel {
	uint32_t raw;
	struct {
		uint32_t shadow_rd_sel                   : 1;
	} bits;
};

union reg_isp_bnr_out_sel {
	uint32_t raw;
	struct {
		uint32_t bnr_out_sel                     : 4;
	} bits;
};

union reg_isp_bnr_index_clr {
	uint32_t raw;
	struct {
		uint32_t bnr_index_clr                   : 1;
	} bits;
};

union reg_isp_bnr_ns_luma_th_r    {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_luma_th_r                : 10;
	} bits;
};

union reg_isp_bnr_ns_slope_r      {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_slope_r                  : 10;
	} bits;
};

union reg_isp_bnr_ns_offset0_r    {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_low_offset_r             : 10;
	} bits;
};

union reg_isp_bnr_ns_offset1_r    {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_high_offset_r            : 10;
	} bits;
};

union reg_isp_bnr_ns_luma_th_gr   {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_luma_th_gr               : 10;
	} bits;
};

union reg_isp_bnr_ns_slope_gr     {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_slope_gr                 : 10;
	} bits;
};

union reg_isp_bnr_ns_offset0_gr   {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_low_offset_gr            : 10;
	} bits;
};

union reg_isp_bnr_ns_offset1_gr   {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_high_offset_gr           : 10;
	} bits;
};

union reg_isp_bnr_ns_luma_th_gb   {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_luma_th_gb               : 10;
	} bits;
};

union reg_isp_bnr_ns_slope_gb     {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_slope_gb                 : 10;
	} bits;
};

union reg_isp_bnr_ns_offset0_gb   {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_low_offset_gb            : 10;
	} bits;
};

union reg_isp_bnr_ns_offset1_gb   {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_high_offset_gb           : 10;
	} bits;
};

union reg_isp_bnr_ns_luma_th_b    {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_luma_th_b                : 10;
	} bits;
};

union reg_isp_bnr_ns_slope_b      {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_slope_b                  : 10;
	} bits;
};

union reg_isp_bnr_ns_offset0_b    {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_low_offset_b             : 10;
	} bits;
};

union reg_isp_bnr_ns_offset1_b    {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_high_offset_b            : 10;
	} bits;
};

union reg_isp_bnr_ns_gain         {
	uint32_t raw;
	struct {
		uint32_t bnr_ns_gain                     : 8;
	} bits;
};

union reg_isp_bnr_strength_mode   {
	uint32_t raw;
	struct {
		uint32_t bnr_strength_mode               : 8;
	} bits;
};

union reg_isp_bnr_intensity_sel {
	uint32_t raw;
	struct {
		uint32_t bnr_intensity_sel_00            : 5;
	} bits;
};

union reg_isp_bnr_weight_intra_0  {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_intra_0              : 3;
	} bits;
};

union reg_isp_bnr_weight_intra_1  {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_intra_1              : 3;
	} bits;
};

union reg_isp_bnr_weight_intra_2  {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_intra_2              : 3;
	} bits;
};

union reg_isp_bnr_weight_norm_1   {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_norm_1               : 7;
	} bits;
};

union reg_isp_bnr_weight_norm_2   {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_norm_2               : 8;
	} bits;
};

union reg_isp_bnr_var_th          {
	uint32_t raw;
	struct {
		uint32_t bnr_var_th                      : 8;
	} bits;
};

union reg_isp_bnr_weight_lut {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_lut                  : 5;
	} bits;
};

union reg_isp_bnr_weight_sm       {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_smooth               : 5;
	} bits;
};

union reg_isp_bnr_weight_v        {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_v                    : 5;
	} bits;
};

union reg_isp_bnr_weight_h        {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_h                    : 5;
	} bits;
};

union reg_isp_bnr_weight_d45      {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_d45                  : 5;
	} bits;
};

union reg_isp_bnr_weight_d135     {
	uint32_t raw;
	struct {
		uint32_t bnr_weight_d135                 : 5;
	} bits;
};

union reg_isp_bnr_neighbor_max    {
	uint32_t raw;
	struct {
		uint32_t bnr_flag_neighbor_max           : 1;
	} bits;
};

union reg_isp_bnr_res_k_smooth    {
	uint32_t raw;
	struct {
		uint32_t bnr_res_ratio_k_smooth          : 9;
	} bits;
};

union reg_isp_bnr_res_k_texture   {
	uint32_t raw;
	struct {
		uint32_t bnr_res_ratio_k_texture         : 9;
	} bits;
};

union reg_isp_bnr_res_max {
	uint32_t raw;
	struct {
		uint32_t bnr_res_max                     : 12;
	} bits;
};

union reg_isp_bnr_dummy           {
	uint32_t raw;
	struct {
		uint32_t bnr_dummy                       : 16;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_ca_00 {
	uint32_t raw;
	struct {
		uint32_t cacp_enable                     : 1;
		uint32_t cacp_mode                       : 1;
		uint32_t cacp_dbg_mode                   : 1;
		uint32_t cacp_mem_sw_mode                : 1;
		uint32_t cacp_shdw_read_sel              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t prog_hdk_dis                    : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t cacp_iso_ratio                  : 11;
	} bits;
};

union reg_ca_04 {
	uint32_t raw;
	struct {
		uint32_t cacp_mem_d                      : 24;
		uint32_t _rsv_24                         : 7;
		uint32_t cacp_mem_w                      : 1;
	} bits;
};

union reg_ca_08 {
	uint32_t raw;
	struct {
		uint32_t cacp_mem_st_addr                : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t cacp_mem_st_addr_w              : 1;
	} bits;
};

union reg_ca_0c {
	uint32_t raw;
	struct {
		uint32_t cacp_mem_sw_raddr               : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t cacp_mem_sw_r                   : 1;
	} bits;
};

union reg_ca_10 {
	uint32_t raw;
	struct {
		uint32_t cacp_mem_sw_rdata_r             : 24;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_ccm_00 {
	uint32_t raw;
	struct {
		uint32_t ccm_00                          : 14;
	} bits;
};

union reg_isp_ccm_01 {
	uint32_t raw;
	struct {
		uint32_t ccm_01                          : 14;
	} bits;
};

union reg_isp_ccm_02 {
	uint32_t raw;
	struct {
		uint32_t ccm_02                          : 14;
	} bits;
};

union reg_isp_ccm_10 {
	uint32_t raw;
	struct {
		uint32_t ccm_10                          : 14;
	} bits;
};

union reg_isp_ccm_11 {
	uint32_t raw;
	struct {
		uint32_t ccm_11                          : 14;
	} bits;
};

union reg_isp_ccm_12 {
	uint32_t raw;
	struct {
		uint32_t ccm_12                          : 14;
	} bits;
};

union reg_isp_ccm_20 {
	uint32_t raw;
	struct {
		uint32_t ccm_20                          : 14;
	} bits;
};

union reg_isp_ccm_21 {
	uint32_t raw;
	struct {
		uint32_t ccm_21                          : 14;
	} bits;
};

union reg_isp_ccm_22 {
	uint32_t raw;
	struct {
		uint32_t ccm_22                          : 14;
	} bits;
};

union reg_isp_ccm_ctrl {
	uint32_t raw;
	struct {
		uint32_t ccm_shdw_sel                    : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t ccm_enable                      : 1;
	} bits;
};

union reg_isp_ccm_dbg {
	uint32_t raw;
	struct {
		uint32_t force_clk_enable                : 1;
		uint32_t softrst                         : 1;
	} bits;
};

union reg_isp_ccm_dmy0 {
	uint32_t raw;
	struct {
		uint32_t dmy_def0                        : 32;
	} bits;
};

union reg_isp_ccm_dmy1 {
	uint32_t raw;
	struct {
		uint32_t dmy_def1                        : 32;
	} bits;
};

union reg_isp_ccm_dmy_r {
	uint32_t raw;
	struct {
		uint32_t dmy_ro                          : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_mmap_00 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_enable                   : 1;
		uint32_t mmap_1_enable                   : 1;
		uint32_t mmap_mrg_mode                   : 1;
		uint32_t mm_double_buf_sel               : 1;
		uint32_t on_the_fly                      : 1;
		uint32_t first_frame_reset               : 1;
		uint32_t reg_2_tap_en                    : 1;
		uint32_t mirror_mode_en                  : 1;
		uint32_t mmap_mrg_alph                   : 8;
		uint32_t guard_cnt                       : 8;
		uint32_t bypass                          : 1;
		uint32_t inter_1_en                      : 1;
		uint32_t inter_2_en                      : 1;
		uint32_t inter_3_en                      : 1;
		uint32_t inter_4_en                      : 1;
		uint32_t dma_sel                         : 1;
		uint32_t rgbmap_sw_crop                  : 1;
	} bits;
};

union reg_isp_mmap_04 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_lpf_00                   : 3;
		uint32_t mmap_0_lpf_01                   : 3;
		uint32_t mmap_0_lpf_02                   : 3;
		uint32_t mmap_0_lpf_10                   : 3;
		uint32_t mmap_0_lpf_11                   : 3;
		uint32_t mmap_0_lpf_12                   : 3;
		uint32_t mmap_0_lpf_20                   : 3;
		uint32_t mmap_0_lpf_21                   : 3;
		uint32_t mmap_0_lpf_22                   : 3;
		uint32_t _rsv_27                         : 2;
		uint32_t force_clk_en                    : 1;
		uint32_t reg_8bit_rgbmap_mode            : 1;
		uint32_t wh_sw_mode                      : 1;
	} bits;
};

union reg_isp_mmap_08 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_map_coring               : 8;
		uint32_t mmap_0_map_gain                 : 8;
		uint32_t mmap_0_map_thd_l                : 8;
		uint32_t mmap_0_map_thd_h                : 8;
	} bits;
};

union reg_isp_mmap_0c {
	uint32_t raw;
	struct {
		uint32_t mmap_0_luma_adapt_lut_in_0      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_0_luma_adapt_lut_in_1      : 12;
	} bits;
};

union reg_isp_mmap_10 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_luma_adapt_lut_in_2      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_0_luma_adapt_lut_in_3      : 12;
	} bits;
};

union reg_isp_mmap_14 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_luma_adapt_lut_out_0     : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t mmap_0_luma_adapt_lut_out_1     : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t mmap_0_luma_adapt_lut_out_2     : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t mmap_0_luma_adapt_lut_out_3     : 6;
	} bits;
};

union reg_isp_mmap_18 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_luma_adapt_lut_slope_0   : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t mmap_0_luma_adapt_lut_slope_1   : 11;
	} bits;
};

union reg_isp_mmap_1c {
	uint32_t raw;
	struct {
		uint32_t mmap_0_luma_adapt_lut_slope_2   : 11;
		uint32_t _rsv_11                         : 1;
		uint32_t mmap_0_map_dshift_bit           : 3;
	} bits;
};

union reg_isp_mmap_20 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_iir_prtct_lut_in_0       : 8;
		uint32_t mmap_0_iir_prtct_lut_in_1       : 8;
		uint32_t mmap_0_iir_prtct_lut_in_2       : 8;
		uint32_t mmap_0_iir_prtct_lut_in_3       : 8;
	} bits;
};

union reg_isp_mmap_24 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_iir_prtct_lut_out_0      : 4;
		uint32_t mmap_0_iir_prtct_lut_out_1      : 4;
		uint32_t mmap_0_iir_prtct_lut_out_2      : 4;
		uint32_t mmap_0_iir_prtct_lut_out_3      : 4;
	} bits;
};

union reg_isp_mmap_28 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_iir_prtct_lut_slope_0    : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t mmap_0_iir_prtct_lut_slope_1    : 9;
	} bits;
};

union reg_isp_mmap_2c {
	uint32_t raw;
	struct {
		uint32_t mmap_0_iir_prtct_lut_slope_2    : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t mmap_0_mh_wgt                   : 4;
	} bits;
};

union reg_isp_mmap_30 {
	uint32_t raw;
	struct {
		uint32_t img_widthm1_sw                  : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_heightm1_sw                 : 14;
	} bits;
};

union reg_isp_mmap_34 {
	uint32_t raw;
	struct {
		uint32_t v_thd_l                         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t v_thd_h                         : 12;
	} bits;
};

union reg_isp_mmap_38 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_map_coring               : 8;
	} bits;
};

union reg_isp_mmap_3c {
	uint32_t raw;
	struct {
		uint32_t v_wgt_slp                       : 19;
		uint32_t _rsv_19                         : 5;
		uint32_t motion_ls_mode                  : 1;
		uint32_t motion_ls_sel                   : 1;
		uint32_t motion_yv_ls_mode               : 1;
		uint32_t motion_yv_ls_sel                : 1;
	} bits;
};

union reg_isp_mmap_40 {
	uint32_t raw;
	struct {
		uint32_t v_wgt_max                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t v_wgt_min                       : 9;
	} bits;
};

union reg_isp_mmap_44 {
	uint32_t raw;
	struct {
		uint32_t mmap_med_wgt                    : 9;
		uint32_t _rsv_9                          : 6;
		uint32_t mmap_med_enable                 : 1;
	} bits;
};

union reg_isp_mmap_4c {
	uint32_t raw;
	struct {
		uint32_t mmap_1_luma_adapt_lut_in_0      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_1_luma_adapt_lut_in_1      : 12;
	} bits;
};

union reg_isp_mmap_50 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_luma_adapt_lut_in_2      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_1_luma_adapt_lut_in_3      : 12;
	} bits;
};

union reg_isp_mmap_54 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_luma_adapt_lut_out_0     : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t mmap_1_luma_adapt_lut_out_1     : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t mmap_1_luma_adapt_lut_out_2     : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t mmap_1_luma_adapt_lut_out_3     : 6;
	} bits;
};

union reg_isp_mmap_58 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_luma_adapt_lut_slope_0   : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t mmap_1_luma_adapt_lut_slope_1   : 11;
	} bits;
};

union reg_isp_mmap_5c {
	uint32_t raw;
	struct {
		uint32_t mmap_1_luma_adapt_lut_slope_2   : 11;
	} bits;
};

union reg_isp_mmap_60 {
	uint32_t raw;
	struct {
		uint32_t rgbmap_w_bit                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t rgbmap_h_bit                    : 3;
	} bits;
};

union reg_isp_mmap_64 {
	uint32_t raw;
	struct {
		uint32_t sram_wdata_0                    : 32;
	} bits;
};

union reg_isp_mmap_68 {
	uint32_t raw;
	struct {
		uint32_t sram_wdata_1                    : 32;
	} bits;
};

union reg_isp_mmap_6c {
	uint32_t raw;
	struct {
		uint32_t sram_wadd                       : 7;
		uint32_t sram_wen                        : 1;
		uint32_t force_dma_disable               : 8;
		uint32_t manr_debug                      : 16;
	} bits;
};

union reg_isp_mmap_70 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_gain_ratio_r             : 16;
		uint32_t mmap_0_gain_ratio_g             : 16;
	} bits;
};

union reg_isp_mmap_74 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_gain_ratio_b             : 16;
	} bits;
};

union reg_isp_mmap_78 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_ns_slope_r               : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_0_ns_slope_g               : 10;
	} bits;
};

union reg_isp_mmap_7c {
	uint32_t raw;
	struct {
		uint32_t mmap_0_ns_slope_b               : 10;
	} bits;
};

union reg_isp_mmap_80 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_ns_luma_th0_r            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_0_ns_luma_th0_g            : 12;
	} bits;
};

union reg_isp_mmap_84 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_ns_luma_th0_b            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_0_ns_low_offset_r          : 12;
	} bits;
};

union reg_isp_mmap_88 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_ns_low_offset_g          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_0_ns_low_offset_b          : 12;
	} bits;
};

union reg_isp_mmap_8c {
	uint32_t raw;
	struct {
		uint32_t mmap_0_ns_high_offset_r         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_0_ns_high_offset_g         : 12;
	} bits;
};

union reg_isp_mmap_90 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_ns_high_offset_b         : 12;
	} bits;
};

union reg_isp_mmap_a0 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_gain_ratio_r             : 16;
		uint32_t mmap_1_gain_ratio_g             : 16;
	} bits;
};

union reg_isp_mmap_a4 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_gain_ratio_b             : 16;
	} bits;
};

union reg_isp_mmap_a8 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_ns_slope_r               : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_1_ns_slope_g               : 10;
	} bits;
};

union reg_isp_mmap_ac {
	uint32_t raw;
	struct {
		uint32_t mmap_1_ns_slope_b               : 10;
	} bits;
};

union reg_isp_mmap_b0 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_ns_luma_th0_r            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_1_ns_luma_th0_g            : 12;
	} bits;
};

union reg_isp_mmap_b4 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_ns_luma_th0_b            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_1_ns_low_offset_r          : 12;
	} bits;
};

union reg_isp_mmap_b8 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_ns_low_offset_g          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_1_ns_low_offset_b          : 12;
	} bits;
};

union reg_isp_mmap_bc {
	uint32_t raw;
	struct {
		uint32_t mmap_1_ns_high_offset_r         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_1_ns_high_offset_g         : 12;
	} bits;
};

union reg_isp_mmap_c0 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_ns_high_offset_b         : 12;
	} bits;
};

union reg_isp_mmap_c4 {
	uint32_t raw;
	struct {
		uint32_t img_width_crop                  : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_height_crop                 : 14;
		uint32_t _rsv_30                         : 1;
		uint32_t crop_enable                     : 1;
	} bits;
};

union reg_isp_mmap_c8 {
	uint32_t raw;
	struct {
		uint32_t crop_w_str                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_w_end                      : 14;
	} bits;
};

union reg_isp_mmap_cc {
	uint32_t raw;
	struct {
		uint32_t crop_h_str                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_h_end                      : 14;
	} bits;
};

union reg_isp_mmap_d0 {
	uint32_t raw;
	struct {
		uint32_t img_width_crop_scalar           : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_height_crop_scalar          : 14;
		uint32_t _rsv_30                         : 1;
		uint32_t crop_enable_scalar              : 1;
	} bits;
};

union reg_isp_mmap_d4 {
	uint32_t raw;
	struct {
		uint32_t crop_w_str_scalar               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_w_end_scalar               : 14;
	} bits;
};

union reg_isp_mmap_d8 {
	uint32_t raw;
	struct {
		uint32_t crop_h_str_scalar               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_h_end_scalar               : 14;
	} bits;
};

union reg_isp_mmap_dc {
	uint32_t raw;
	struct {
		uint32_t coef_r                          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t coef_g                          : 11;
	} bits;
};

union reg_isp_mmap_e0 {
	uint32_t raw;
	struct {
		uint32_t coef_b                          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t coef_i                          : 11;
	} bits;
};

union reg_isp_mmap_e4 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_gain_ratio_i             : 16;
		uint32_t mmap_0_ns_slope_i               : 10;
	} bits;
};

union reg_isp_mmap_e8 {
	uint32_t raw;
	struct {
		uint32_t mmap_0_ns_luma_th0_i            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_0_ns_low_offset_i          : 12;
	} bits;
};

union reg_isp_mmap_ec {
	uint32_t raw;
	struct {
		uint32_t mmap_0_ns_high_offset_i         : 12;
	} bits;
};

union reg_isp_mmap_f0 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_gain_ratio_i             : 16;
		uint32_t mmap_1_ns_slope_i               : 10;
	} bits;
};

union reg_isp_mmap_f4 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_ns_luma_th0_i            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mmap_1_ns_low_offset_i          : 12;
	} bits;
};

union reg_isp_mmap_f8 {
	uint32_t raw;
	struct {
		uint32_t mmap_1_ns_high_offset_i         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t history_sel_0                   : 1;
		uint32_t history_sel_1                   : 1;
		uint32_t history_sel_2                   : 1;
		uint32_t history_sel_3                   : 1;
	} bits;
};

union reg_isp_mmap_fc {
	uint32_t raw;
	struct {
		uint32_t manr_status                     : 28;
		uint32_t manr_status_mux                 : 4;
	} bits;
};

union reg_isp_mmap_100 {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_centerx                : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t mmap_lsc_centery                : 13;
	} bits;
};

union reg_isp_mmap_104 {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_norm                   : 15;
		uint32_t _rsv_15                         : 1;
		uint32_t mmap_lsc_dy_gain                : 8;
	} bits;
};

union reg_isp_mmap_108 {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_comp_gain_lut_00       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_lsc_comp_gain_lut_01       : 10;
	} bits;
};

union reg_isp_mmap_10c {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_comp_gain_lut_02       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_lsc_comp_gain_lut_03       : 10;
	} bits;
};

union reg_isp_mmap_110 {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_comp_gain_lut_04       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_lsc_comp_gain_lut_05       : 10;
	} bits;
};

union reg_isp_mmap_114 {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_comp_gain_lut_06       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_lsc_comp_gain_lut_07       : 10;
	} bits;
};

union reg_isp_mmap_118 {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_comp_gain_lut_08       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_lsc_comp_gain_lut_09       : 10;
	} bits;
};

union reg_isp_mmap_11c {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_comp_gain_lut_10       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_lsc_comp_gain_lut_11       : 10;
	} bits;
};

union reg_isp_mmap_120 {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_comp_gain_lut_12       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_lsc_comp_gain_lut_13       : 10;
	} bits;
};

union reg_isp_mmap_124 {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_comp_gain_lut_14       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t mmap_lsc_comp_gain_lut_15       : 10;
	} bits;
};

union reg_isp_mmap_128 {
	uint32_t raw;
	struct {
		uint32_t mmap_lsc_comp_gain_lut_16       : 10;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_gamma_ctrl {
	uint32_t raw;
	struct {
		uint32_t gamma_enable                    : 1;
		uint32_t gamma_shdw_sel                  : 1;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_gamma_prog_ctrl {
	uint32_t raw;
	struct {
		uint32_t gamma_wsel                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t gamma_rsel                      : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t gamma_prog_en                   : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t gamma_prog_1to3_en              : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t gamma_prog_mode                 : 2;
	} bits;
};

union reg_isp_gamma_prog_st_addr {
	uint32_t raw;
	struct {
		uint32_t gamma_st_addr                   : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t gamma_st_w                      : 1;
	} bits;
};

union reg_isp_gamma_prog_data {
	uint32_t raw;
	struct {
		uint32_t gamma_data_e                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t gamma_data_o                    : 12;
		uint32_t _rsv_28                         : 3;
		uint32_t gamma_w                         : 1;
	} bits;
};

union reg_isp_gamma_prog_max {
	uint32_t raw;
	struct {
		uint32_t gamma_max                       : 13;
	} bits;
};

union reg_isp_gamma_mem_sw_raddr {
	uint32_t raw;
	struct {
		uint32_t gamma_sw_raddr                  : 8;
		uint32_t _rsv_8                          : 4;
		uint32_t gamma_sw_r_mem_sel              : 1;
	} bits;
};

union reg_isp_gamma_mem_sw_rdata {
	uint32_t raw;
	struct {
		uint32_t gamma_rdata_r                   : 12;
		uint32_t _rsv_12                         : 19;
		uint32_t gamma_sw_r                      : 1;
	} bits;
};

union reg_isp_gamma_mem_sw_rdata_bg {
	uint32_t raw;
	struct {
		uint32_t gamma_rdata_g                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t gamma_rdata_b                   : 12;
	} bits;
};

union reg_isp_gamma_dbg {
	uint32_t raw;
	struct {
		uint32_t prog_hdk_dis                    : 1;
		uint32_t softrst                         : 1;
	} bits;
};

union reg_isp_gamma_dmy0 {
	uint32_t raw;
	struct {
		uint32_t dmy_def0                        : 32;
	} bits;
};

union reg_isp_gamma_dmy1 {
	uint32_t raw;
	struct {
		uint32_t dmy_def1                        : 32;
	} bits;
};

union reg_isp_gamma_dmy_r {
	uint32_t raw;
	struct {
		uint32_t dmy_ro                          : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_clut_ctrl {
	uint32_t raw;
	struct {
		uint32_t clut_enable                     : 1;
		uint32_t clut_shdw_sel                   : 1;
		uint32_t force_clk_enable                : 1;
		uint32_t prog_en                         : 1;
	} bits;
};

union reg_isp_clut_prog_addr {
	uint32_t raw;
	struct {
		uint32_t sram_r_idx                      : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t sram_g_idx                      : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t sram_b_idx                      : 5;
	} bits;
};

union reg_isp_clut_prog_data {
	uint32_t raw;
	struct {
		uint32_t sram_wdata                      : 30;
		uint32_t _rsv_30                         : 1;
		uint32_t sram_wr                         : 1;
	} bits;
};

union reg_isp_clut_prog_rdata {
	uint32_t raw;
	struct {
		uint32_t sram_rdata                      : 30;
		uint32_t _rsv_30                         : 1;
		uint32_t sram_rd                         : 1;
	} bits;
};

union reg_isp_clut_dbg {
	uint32_t raw;
	struct {
		uint32_t prog_hdk_dis                    : 1;
	} bits;
};

union reg_isp_clut_dmy0 {
	uint32_t raw;
	struct {
		uint32_t dmy_def0                        : 32;
	} bits;
};

union reg_isp_clut_dmy1 {
	uint32_t raw;
	struct {
		uint32_t dmy_def1                        : 32;
	} bits;
};

union reg_isp_clut_dmy_r {
	uint32_t raw;
	struct {
		uint32_t dmy_ro                          : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_dehaze_dhz_smooth {
	uint32_t raw;
	struct {
		uint32_t dehaze_w                        : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_th_smooth                : 10;
	} bits;
};

union reg_isp_dehaze_dhz_skin {
	uint32_t raw;
	struct {
		uint32_t dehaze_skin_cb                  : 8;
		uint32_t dehaze_skin_cr                  : 8;
	} bits;
};

union reg_isp_dehaze_dhz_wgt {
	uint32_t raw;
	struct {
		uint32_t dehaze_a_luma_wgt               : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t dehaze_blend_wgt                : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t dehaze_tmap_scale               : 8;
		uint32_t dehaze_d_wgt                    : 5;
	} bits;
};

union reg_isp_dehaze_dhz_bypass {
	uint32_t raw;
	struct {
		uint32_t dehaze_enable                   : 1;
		uint32_t dehaze_luma_lut_enable          : 1;
		uint32_t dehaze_skin_lut_enable          : 1;
		uint32_t dehaze_shdw_sel                 : 1;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_dehaze_0 {
	uint32_t raw;
	struct {
		uint32_t softrst                         : 1;
		uint32_t _rsv_1                          : 4;
		uint32_t dbg_en                          : 1;
		uint32_t _rsv_6                          : 10;
		uint32_t check_sum                       : 16;
	} bits;
};

union reg_isp_dehaze_1 {
	uint32_t raw;
	struct {
		uint32_t dehaze_cum_th                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t dehaze_hist_th                  : 14;
	} bits;
};

union reg_isp_dehaze_2 {
	uint32_t raw;
	struct {
		uint32_t dehaze_sw_dc_th                 : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t dehaze_sw_aglobal_r             : 12;
		uint32_t dehaze_sw_dc_trig               : 1;
		uint32_t _rsv_29                         : 2;
		uint32_t dehaze_sw_dc_aglobal_trig       : 1;
	} bits;
};

union reg_isp_dehaze_28 {
	uint32_t raw;
	struct {
		uint32_t dehaze_sw_aglobal_g             : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t dehaze_sw_aglobal_b             : 12;
	} bits;
};

union reg_isp_dehaze_2c {
	uint32_t raw;
	struct {
		uint32_t dehaze_aglobal_max              : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t dehaze_aglobal_min              : 12;
	} bits;
};

union reg_isp_dehaze_3 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_min                 : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t dehaze_tmap_max                 : 13;
	} bits;
};

union reg_isp_dehaze_5 {
	uint32_t raw;
	struct {
		uint32_t fmt_st                          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t fmt_end                         : 12;
		uint32_t tile_nm                         : 4;
	} bits;
};

union reg_isp_dehaze_6 {
	uint32_t raw;
	struct {
		uint32_t dbg_sel                         : 3;
	} bits;
};

union reg_isp_dehaze_7 {
	uint32_t raw;
	struct {
		uint32_t dhz_dbg0                        : 32;
	} bits;
};

union reg_isp_dehaze_8 {
	uint32_t raw;
	struct {
		uint32_t dhz_dbg1                        : 32;
	} bits;
};

union reg_isp_dehaze_9 {
	uint32_t raw;
	struct {
		uint32_t dehaze_luma_lut00               : 8;
		uint32_t dehaze_luma_lut01               : 8;
		uint32_t dehaze_luma_lut02               : 8;
		uint32_t dehaze_luma_lut03               : 8;
	} bits;
};

union reg_isp_dehaze_10 {
	uint32_t raw;
	struct {
		uint32_t dehaze_luma_lut04               : 8;
		uint32_t dehaze_luma_lut05               : 8;
		uint32_t dehaze_luma_lut06               : 8;
		uint32_t dehaze_luma_lut07               : 8;
	} bits;
};

union reg_isp_dehaze_11 {
	uint32_t raw;
	struct {
		uint32_t dehaze_luma_lut08               : 8;
		uint32_t dehaze_luma_lut09               : 8;
		uint32_t dehaze_luma_lut10               : 8;
		uint32_t dehaze_luma_lut11               : 8;
	} bits;
};

union reg_isp_dehaze_12 {
	uint32_t raw;
	struct {
		uint32_t dehaze_luma_lut12               : 8;
		uint32_t dehaze_luma_lut13               : 8;
		uint32_t dehaze_luma_lut14               : 8;
		uint32_t dehaze_luma_lut15               : 8;
	} bits;
};

union reg_isp_dehaze_17 {
	uint32_t raw;
	struct {
		uint32_t dehaze_skin_lut00               : 8;
		uint32_t dehaze_skin_lut01               : 8;
		uint32_t dehaze_skin_lut02               : 8;
		uint32_t dehaze_skin_lut03               : 8;
	} bits;
};

union reg_isp_dehaze_18 {
	uint32_t raw;
	struct {
		uint32_t dehaze_skin_lut04               : 8;
		uint32_t dehaze_skin_lut05               : 8;
		uint32_t dehaze_skin_lut06               : 8;
		uint32_t dehaze_skin_lut07               : 8;
	} bits;
};

union reg_isp_dehaze_19 {
	uint32_t raw;
	struct {
		uint32_t dehaze_skin_lut08               : 8;
		uint32_t dehaze_skin_lut09               : 8;
		uint32_t dehaze_skin_lut10               : 8;
		uint32_t dehaze_skin_lut11               : 8;
	} bits;
};

union reg_isp_dehaze_20 {
	uint32_t raw;
	struct {
		uint32_t dehaze_skin_lut12               : 8;
		uint32_t dehaze_skin_lut13               : 8;
		uint32_t dehaze_skin_lut14               : 8;
		uint32_t dehaze_skin_lut15               : 8;
	} bits;
};

union reg_isp_dehaze_25 {
	uint32_t raw;
	struct {
		uint32_t aglobal_r                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t aglobal_g                       : 12;
	} bits;
};

union reg_isp_dehaze_26 {
	uint32_t raw;
	struct {
		uint32_t aglobal_b                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t dc_th                           : 10;
	} bits;
};

union reg_isp_dehaze_tmap_00 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut000         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut001         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut002         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut003         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_01 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut004         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut005         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut006         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut007         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_02 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut008         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut009         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut010         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut011         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_03 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut012         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut013         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut014         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut015         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_04 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut016         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut017         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut018         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut019         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_05 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut020         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut021         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut022         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut023         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_06 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut024         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut025         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut026         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut027         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_07 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut028         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut029         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut030         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut031         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_08 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut032         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut033         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut034         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut035         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_09 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut036         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut037         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut038         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut039         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_10 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut040         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut041         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut042         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut043         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_11 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut044         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut045         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut046         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut047         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_12 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut048         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut049         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut050         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut051         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_13 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut052         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut053         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut054         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut055         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_14 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut056         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut057         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut058         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut059         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_15 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut060         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut061         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut062         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut063         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_16 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut064         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut065         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut066         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut067         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_17 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut068         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut069         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut070         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut071         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_18 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut072         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut073         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut074         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut075         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_19 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut076         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut077         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut078         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut079         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_20 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut080         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut081         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut082         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut083         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_21 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut084         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut085         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut086         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut087         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_22 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut088         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut089         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut090         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut091         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_23 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut092         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut093         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut094         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut095         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_24 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut096         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut097         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut098         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut099         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_25 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut100         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut101         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut102         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut103         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_26 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut104         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut105         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut106         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut107         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_27 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut108         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut109         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut110         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut111         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_28 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut112         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut113         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut114         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut115         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_29 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut116         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut117         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut118         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut119         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_30 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut120         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut121         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut122         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut123         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_31 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut124         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t dehaze_tmap_gain_lut125         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t dehaze_tmap_gain_lut126         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t dehaze_tmap_gain_lut127         : 7;
	} bits;
};

union reg_isp_dehaze_tmap_32 {
	uint32_t raw;
	struct {
		uint32_t dehaze_tmap_gain_lut128         : 7;
	} bits;
};

union reg_isp_dehaze_o_dc_th_up_mode {
	uint32_t raw;
	struct {
		uint32_t o_dc_th_up_mode                 : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_csc_0 {
	uint32_t raw;
	struct {
		uint32_t csc_enable                      : 1;
		uint32_t r2y4_shdw_sel                   : 1;
	} bits;
};

union reg_isp_csc_1 {
	uint32_t raw;
	struct {
		uint32_t op_start                        : 1;
		uint32_t cont_en                         : 1;
		uint32_t r2y4_bypass                     : 1;
		uint32_t softrst                         : 1;
		uint32_t auto_update_en                  : 1;
		uint32_t dbg_en                          : 1;
		uint32_t _rsv_6                          : 10;
		uint32_t check_sum                       : 16;
	} bits;
};

union reg_isp_csc_2 {
	uint32_t raw;
	struct {
		uint32_t shdw_update_req                 : 1;
	} bits;
};

union reg_isp_csc_3 {
	uint32_t raw;
	struct {
		uint32_t dmy0                            : 32;
	} bits;
};

union reg_isp_csc_4 {
	uint32_t raw;
	struct {
		uint32_t coeff_00                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t coeff_01                        : 14;
	} bits;
};

union reg_isp_csc_5 {
	uint32_t raw;
	struct {
		uint32_t coeff_02                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t coeff_10                        : 14;
	} bits;
};

union reg_isp_csc_6 {
	uint32_t raw;
	struct {
		uint32_t coeff_11                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t coeff_12                        : 14;
	} bits;
};

union reg_isp_csc_7 {
	uint32_t raw;
	struct {
		uint32_t coeff_20                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t coeff_21                        : 14;
	} bits;
};

union reg_isp_csc_8 {
	uint32_t raw;
	struct {
		uint32_t coeff_22                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t offset_0                        : 11;
	} bits;
};

union reg_isp_csc_9 {
	uint32_t raw;
	struct {
		uint32_t offset_1                        : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t offset_2                        : 11;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_rgb_dither_rgb_dither {
	uint32_t raw;
	struct {
		uint32_t rgb_dither_enable               : 1;
		uint32_t rgb_dither_mod_en               : 1;
		uint32_t rgb_dither_histidx_en           : 1;
		uint32_t rgb_dither_fmnum_en             : 1;
		uint32_t rgb_dither_shdw_sel             : 1;
		uint32_t rgb_dither_softrst              : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t crop_widthm1                    : 12;
		uint32_t crop_heightm1                   : 12;
	} bits;
};

union reg_isp_rgb_dither_rgb_dither_debug0 {
	uint32_t raw;
	struct {
		uint32_t rgb_dither_debug0               : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_dci_status {
	uint32_t raw;
	struct {
		uint32_t dci_status                      : 32;
	} bits;
};

union reg_isp_dci_grace_reset {
	uint32_t raw;
	struct {
		uint32_t dci_grace_reset                 : 1;
	} bits;
};

union reg_isp_dci_monitor {
	uint32_t raw;
	struct {
		uint32_t dci_monitor                     : 32;
	} bits;
};

union reg_isp_dci_enable {
	uint32_t raw;
	struct {
		uint32_t dci_enable                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t dci_hist_enable                 : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t dci_uv_gain_enable              : 1;
		uint32_t _rsv_9                          : 19;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_dci_map_enable {
	uint32_t raw;
	struct {
		uint32_t dci_map_enable                  : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t dci_per1sample_enable           : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t dci_histo_big_endian            : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t dci_roi_enable                  : 1;
		uint32_t _rsv_13                         : 7;
		uint32_t dci_zeroing_enable              : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t dci_shift_enable                : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t dci_index_enable                : 1;
	} bits;
};

union reg_isp_dci_flow {
	uint32_t raw;
	struct {
		uint32_t dci_zerodciogram                : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t dci_shadow_select               : 1;
	} bits;
};

union reg_isp_dci_demo_mode {
	uint32_t raw;
	struct {
		uint32_t dci_demo_mode                   : 1;
	} bits;
};

union reg_isp_dci_monitor_select {
	uint32_t raw;
	struct {
		uint32_t dci_monitor_select              : 32;
	} bits;
};

union reg_isp_dci_location {
	uint32_t raw;
	struct {
		uint32_t dci_location                    : 32;
	} bits;
};

union reg_isp_dci_prog_data {
	uint32_t raw;
	struct {
		uint32_t dci_prog_data                   : 32;
	} bits;
};

union reg_isp_dci_prog_ctrl {
	uint32_t raw;
	struct {
		uint32_t dci_prog_ctrl                   : 32;
	} bits;
};

union reg_isp_dci_prog_max {
	uint32_t raw;
	struct {
		uint32_t dci_prog_max                    : 32;
	} bits;
};

union reg_isp_dci_ctrl {
	uint32_t raw;
	struct {
		uint32_t dci_ctrl                        : 32;
	} bits;
};

union reg_isp_dci_mem_sw_mode {
	uint32_t raw;
	struct {
		uint32_t dci_mem_sw_mode                 : 32;
	} bits;
};

union reg_isp_dci_mem_raddr {
	uint32_t raw;
	struct {
		uint32_t dci_mem_raddr                   : 32;
	} bits;
};

union reg_isp_dci_mem_rdata {
	uint32_t raw;
	struct {
		uint32_t dci_mem_rdata                   : 32;
	} bits;
};

union reg_isp_dci_debug {
	uint32_t raw;
	struct {
		uint32_t dci_debug                       : 32;
	} bits;
};

union reg_isp_dci_dummy {
	uint32_t raw;
	struct {
		uint32_t dci_dummy                       : 32;
	} bits;
};

union reg_isp_dci_img_widthm1 {
	uint32_t raw;
	struct {
		uint32_t img_widthm1                     : 16;
	} bits;
};

union reg_isp_dci_lut_order_select {
	uint32_t raw;
	struct {
		uint32_t dci_lut_order_select            : 1;
	} bits;
};

union reg_isp_dci_roi_start {
	uint32_t raw;
	struct {
		uint32_t dci_roi_start_x                 : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t dci_roi_start_y                 : 14;
	} bits;
};

union reg_isp_dci_roi_geo {
	uint32_t raw;
	struct {
		uint32_t dci_roi_widthm1                 : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t dci_roi_heightm1                : 14;
	} bits;
};

union reg_isp_dci_uv_gain_max {
	uint32_t raw;
	struct {
		uint32_t dci_uv_gain_max                 : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t dci_uv_gain_min                 : 7;
	} bits;
};

union reg_isp_dci_map_dbg {
	uint32_t raw;
	struct {
		uint32_t prog_hdk_dis                    : 1;
		uint32_t cont_en                         : 1;
		uint32_t softrst                         : 1;
		uint32_t dbg_en                          : 1;
		uint32_t _rsv_4                          : 12;
		uint32_t check_sum                       : 16;
	} bits;
};

union reg_isp_dci_bayer_starting {
	uint32_t raw;
	struct {
		uint32_t dci_bayer_starting              : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t force_bayer_enable              : 1;
	} bits;
};

union reg_isp_dci_dmi_enable {
	uint32_t raw;
	struct {
		uint32_t dmi_enable                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t dmi_qos                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t force_dma_disable               : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_hist_edge_v_status {
	uint32_t raw;
	struct {
		uint32_t status                          : 32;
	} bits;
};

union reg_isp_hist_edge_v_sw_ctl {
	uint32_t raw;
	struct {
		uint32_t sw_reset                        : 1;
		uint32_t clr_sram                        : 1;
		uint32_t _rsv_2                          : 1;
		uint32_t shaw_sel                        : 1;
		uint32_t tile_nm                         : 4;
	} bits;
};

union reg_isp_hist_edge_v_bypass {
	uint32_t raw;
	struct {
		uint32_t bypass                          : 1;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_hist_edge_v_ip_config {
	uint32_t raw;
	struct {
		uint32_t hist_edge_v_enable              : 1;
		uint32_t hist_edge_v_luma_mode           : 1;
	} bits;
};

union reg_isp_hist_edge_v_hist_edge_v_offsetx {
	uint32_t raw;
	struct {
		uint32_t hist_edge_v_offsetx             : 13;
	} bits;
};

union reg_isp_hist_edge_v_hist_edge_v_offsety {
	uint32_t raw;
	struct {
		uint32_t hist_edge_v_offsety             : 13;
	} bits;
};

union reg_isp_hist_edge_v_monitor {
	uint32_t raw;
	struct {
		uint32_t monitor                         : 32;
	} bits;
};

union reg_isp_hist_edge_v_monitor_select {
	uint32_t raw;
	struct {
		uint32_t monitor_sel                     : 32;
	} bits;
};

union reg_isp_hist_edge_v_location {
	uint32_t raw;
	struct {
		uint32_t location                        : 32;
	} bits;
};

union reg_isp_hist_edge_v_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy                           : 16;
	} bits;
};

union reg_isp_hist_edge_v_dmi_enable {
	uint32_t raw;
	struct {
		uint32_t dmi_enable                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t dmi_qos                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t force_dma_disable               : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_fusion_fs_ctrl_0 {
	uint32_t raw;
	struct {
		uint32_t fs_enable                       : 1;
		uint32_t force_clk_enable                : 1;
		uint32_t se_in_sel                       : 1;
		uint32_t force_pclk_enable               : 1;
		uint32_t fs_mc_enable                    : 1;
		uint32_t fs_dc_mode                      : 1;
		uint32_t fs_luma_mode                    : 1;
		uint32_t fs_lmap_guide_dc_mode           : 1;
		uint32_t fs_lmap_guide_luma_mode         : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t fs_s_max                        : 20;
	} bits;
};

union reg_fusion_fs_se_gain {
	uint32_t raw;
	struct {
		uint32_t fs_ls_gain                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t fs_out_sel                      : 4;
	} bits;
};

union reg_fusion_fs_luma_thd {
	uint32_t raw;
	struct {
		uint32_t fs_luma_thd_l                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t fs_luma_thd_h                   : 12;
	} bits;
};

union reg_fusion_fs_wgt {
	uint32_t raw;
	struct {
		uint32_t fs_wgt_max                      : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t fs_wgt_min                      : 9;
	} bits;
};

union reg_fusion_fs_wgt_slope {
	uint32_t raw;
	struct {
		uint32_t fs_wgt_slp                      : 19;
	} bits;
};

union reg_fusion_fs_shdw_read_sel {
	uint32_t raw;
	struct {
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_fusion_fs_motion_lut_in {
	uint32_t raw;
	struct {
		uint32_t fs_motion_lut_in_0              : 8;
		uint32_t fs_motion_lut_in_1              : 8;
		uint32_t fs_motion_lut_in_2              : 8;
		uint32_t fs_motion_lut_in_3              : 8;
	} bits;
};

union reg_fusion_fs_motion_lut_out_0 {
	uint32_t raw;
	struct {
		uint32_t fs_motion_lut_out_0             : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t fs_motion_lut_out_1             : 9;
	} bits;
};

union reg_fusion_fs_motion_lut_out_1 {
	uint32_t raw;
	struct {
		uint32_t fs_motion_lut_out_2             : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t fs_motion_lut_out_3             : 9;
	} bits;
};

union reg_fusion_fs_motion_lut_slope_0 {
	uint32_t raw;
	struct {
		uint32_t fs_motion_lut_slope_0           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t fs_motion_lut_slope_1           : 12;
	} bits;
};

union reg_fusion_fs_motion_lut_slope_1 {
	uint32_t raw;
	struct {
		uint32_t fs_motion_lut_slope_2           : 12;
	} bits;
};

union reg_fusion_fs_ctrl_1 {
	uint32_t raw;
	struct {
		uint32_t le_in_sel                       : 1;
		uint32_t _rsv_1                          : 5;
		uint32_t fs_fusion_type                  : 2;
		uint32_t _rsv_8                          : 8;
		uint32_t fs_fusion_lwgt                  : 9;
	} bits;
};

union reg_fusion_fs_calib_ctrl_0 {
	uint32_t raw;
	struct {
		uint32_t fs_calib_luma_low_th            : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t fs_calib_luma_high_th           : 12;
	} bits;
};

union reg_fusion_fs_calib_ctrl_1 {
	uint32_t raw;
	struct {
		uint32_t fs_calib_dif_th                 : 12;
	} bits;
};

union reg_fusion_fs_se_fix_offset_0 {
	uint32_t raw;
	struct {
		uint32_t fs_se_fix_offset_r              : 21;
	} bits;
};

union reg_fusion_fs_se_fix_offset_1 {
	uint32_t raw;
	struct {
		uint32_t fs_se_fix_offset_g              : 21;
	} bits;
};

union reg_fusion_fs_se_fix_offset_2 {
	uint32_t raw;
	struct {
		uint32_t fs_se_fix_offset_b              : 21;
	} bits;
};

union reg_fusion_fs_calib_out_0 {
	uint32_t raw;
	struct {
		uint32_t fs_cal_pxl_num                  : 20;
	} bits;
};

union reg_fusion_fs_calib_out_1 {
	uint32_t raw;
	struct {
		uint32_t fs_pxl_diff_sum_r               : 32;
	} bits;
};

union reg_fusion_fs_calib_out_2 {
	uint32_t raw;
	struct {
		uint32_t fs_pxl_diff_sum_g               : 32;
	} bits;
};

union reg_fusion_fs_calib_out_3 {
	uint32_t raw;
	struct {
		uint32_t fs_pxl_diff_sum_b               : 32;
	} bits;
};

union reg_fusion_fs_lmap_dark_thd {
	uint32_t raw;
	struct {
		uint32_t fs_lmap_guide_dark_thd_l        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t fs_lmap_guide_dark_thd_h        : 12;
	} bits;
};

union reg_fusion_fs_lmap_dark_wgt {
	uint32_t raw;
	struct {
		uint32_t fs_lmap_guide_dark_wgt_l        : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t fs_lmap_guide_dark_wgt_h        : 9;
	} bits;
};

union reg_fusion_fs_lmap_dark_wgt_slope {
	uint32_t raw;
	struct {
		uint32_t fs_lmap_guide_dark_wgt_slp      : 19;
	} bits;
};

union reg_fusion_fs_lmap_brit_thd {
	uint32_t raw;
	struct {
		uint32_t fs_lmap_guide_brit_thd_l        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t fs_lmap_guide_brit_thd_h        : 12;
	} bits;
};

union reg_fusion_fs_lmap_brit_wgt {
	uint32_t raw;
	struct {
		uint32_t fs_lmap_guide_brit_wgt_l        : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t fs_lmap_guide_brit_wgt_h        : 9;
	} bits;
};

union reg_fusion_fs_lmap_brit_wgt_slope {
	uint32_t raw;
	struct {
		uint32_t fs_lmap_guide_brit_wgt_slp      : 19;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_ltm_h00 {
	uint32_t raw;
	struct {
		uint32_t ltm_enable                      : 1;
		uint32_t ltm_dark_enh_enable             : 1;
		uint32_t ltm_brit_enh_enable             : 1;
		uint32_t _rsv_3                          : 2;
		uint32_t shdw_read_sel                   : 1;
		uint32_t _rsv_6                          : 4;
		uint32_t force_pclk_enable               : 1;
		uint32_t _rsv_11                         : 2;
		uint32_t ltm_dbg_mode                    : 3;
		uint32_t force_dma_disable               : 2;
		uint32_t dark_tone_wgt_refine_enable     : 1;
		uint32_t brit_tone_wgt_refine_enable     : 1;
		uint32_t _rsv_20                         : 11;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_ltm_h04 {
	uint32_t raw;
	struct {
		uint32_t img_widthm1_sw                  : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_heightm1_sw                 : 14;
		uint32_t first_frame_reset               : 1;
		uint32_t wh_sw_mode                      : 1;
	} bits;
};

union reg_ltm_h08 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 8;
		uint32_t ltm_be_strth_dshft              : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t ltm_be_strth_gain               : 11;
	} bits;
};

union reg_ltm_h0c {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 8;
		uint32_t ltm_de_strth_dshft              : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t ltm_de_strth_gain               : 11;
	} bits;
};

union reg_ltm_h14 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 16;
		uint32_t ltm_be_rng                      : 4;
		uint32_t _rsv_20                         : 4;
		uint32_t ltm_de_rng                      : 4;
	} bits;
};

union reg_ltm_h18 {
	uint32_t raw;
	struct {
		uint32_t ltm_bri_in_thd_l                : 8;
		uint32_t ltm_bri_in_thd_h                : 8;
		uint32_t _rsv_16                         : 7;
		uint32_t ltm_bri_out_thd_l               : 9;
	} bits;
};

union reg_ltm_h1c {
	uint32_t raw;
	struct {
		uint32_t ltm_bri_out_thd_h               : 9;
		uint32_t ltm_bri_in_gain_slop            : 17;
	} bits;
};

union reg_ltm_h20 {
	uint32_t raw;
	struct {
		uint32_t ltm_dar_in_thd_l                : 8;
		uint32_t ltm_dar_in_thd_h                : 8;
		uint32_t _rsv_16                         : 7;
		uint32_t ltm_dar_out_thd_l               : 9;
	} bits;
};

union reg_ltm_h24 {
	uint32_t raw;
	struct {
		uint32_t ltm_dar_out_thd_h               : 9;
		uint32_t ltm_dar_in_gain_slop            : 17;
	} bits;
};

union reg_ltm_h28 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 4;
		uint32_t ltm_b_curve_quan_bit            : 4;
		uint32_t _rsv_8                          : 4;
		uint32_t ltm_g_curve_1_quan_bit          : 4;
	} bits;
};

union reg_ltm_h2c {
	uint32_t raw;
	struct {
		uint32_t ltm_be_dist_wgt_00              : 5;
		uint32_t ltm_be_dist_wgt_01              : 5;
		uint32_t ltm_be_dist_wgt_02              : 5;
		uint32_t ltm_be_dist_wgt_03              : 5;
		uint32_t ltm_be_dist_wgt_04              : 5;
		uint32_t ltm_be_dist_wgt_05              : 5;
	} bits;
};

union reg_ltm_h30 {
	uint32_t raw;
	struct {
		uint32_t ltm_de_dist_wgt_00              : 5;
		uint32_t ltm_de_dist_wgt_01              : 5;
		uint32_t ltm_de_dist_wgt_02              : 5;
		uint32_t ltm_de_dist_wgt_03              : 5;
		uint32_t ltm_de_dist_wgt_04              : 5;
		uint32_t ltm_de_dist_wgt_05              : 5;
	} bits;
};

union reg_ltm_h34 {
	uint32_t raw;
	struct {
		uint32_t lut_dbg_raddr                   : 10;
		uint32_t _rsv_10                         : 4;
		uint32_t lut_dbg_rsel                    : 1;
		uint32_t lut_dbg_read_en_1t              : 1;
		uint32_t lut_prog_en_bright              : 1;
		uint32_t lut_prog_en_dark                : 1;
		uint32_t lut_prog_en_global              : 1;
	} bits;
};

union reg_ltm_h38 {
	uint32_t raw;
	struct {
		uint32_t lut_wdata                       : 32;
	} bits;
};

union reg_ltm_h3c {
	uint32_t raw;
	struct {
		uint32_t lut_wstaddr                     : 10;
		uint32_t _rsv_10                         : 4;
		uint32_t lut_wsel                        : 1;
		uint32_t lut_wdata_trig_1t               : 1;
		uint32_t lut_wstaddr_trig_1t             : 1;
	} bits;
};

union reg_ltm_h40 {
	uint32_t raw;
	struct {
		uint32_t bright_lut_max                  : 16;
	} bits;
};

union reg_ltm_h44 {
	uint32_t raw;
	struct {
		uint32_t dark_lut_max                    : 16;
	} bits;
};

union reg_ltm_h48 {
	uint32_t raw;
	struct {
		uint32_t global_lut_max                  : 16;
	} bits;
};

union reg_ltm_h4c {
	uint32_t raw;
	struct {
		uint32_t lut_dbg_rdata                   : 32;
	} bits;
};

union reg_ltm_h50 {
	uint32_t raw;
	struct {
		uint32_t dummy_rw                        : 16;
		uint32_t dummy_ro                        : 16;
	} bits;
};

union reg_ltm_h54 {
	uint32_t raw;
	struct {
		uint32_t crop_w_str_scalar               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_w_end_scalar               : 14;
		uint32_t _rsv_30                         : 1;
		uint32_t crop_enable_scalar              : 1;
	} bits;
};

union reg_ltm_h58 {
	uint32_t raw;
	struct {
		uint32_t crop_h_str_scalar               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_h_end_scalar               : 14;
	} bits;
};

union reg_ltm_h5c {
	uint32_t raw;
	struct {
		uint32_t img_height_crop_scalar          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_width_crop_scalar           : 14;
	} bits;
};

union reg_ltm_h60 {
	uint32_t raw;
	struct {
		uint32_t hw_mem_sel                      : 1;
		uint32_t inter_1_bypass                  : 1;
		uint32_t inter_2_bypass                  : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t lmap_assign                     : 1;
		uint32_t lmap_debug                      : 1;
		uint32_t phase_comp                      : 1;
		uint32_t scaler_push_on                  : 1;
		uint32_t _rsv_8                          : 16;
		uint32_t lmap_debug_value                : 8;
	} bits;
};

union reg_ltm_h64 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_enable                   : 1;
		uint32_t ltm_ee_total_gain               : 8;
		uint32_t ltm_ee_luma_gain_enable         : 1;
		uint32_t ltm_ee_luma_mode                : 1;
		uint32_t ltm_ee_soft_clamp_enable        : 1;
	} bits;
};

union reg_ltm_h68 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_luma_gain_lut_00         : 8;
		uint32_t ltm_ee_luma_gain_lut_01         : 8;
		uint32_t ltm_ee_luma_gain_lut_02         : 8;
		uint32_t ltm_ee_luma_gain_lut_03         : 8;
	} bits;
};

union reg_ltm_h6c {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_luma_gain_lut_04         : 8;
		uint32_t ltm_ee_luma_gain_lut_05         : 8;
		uint32_t ltm_ee_luma_gain_lut_06         : 8;
		uint32_t ltm_ee_luma_gain_lut_07         : 8;
	} bits;
};

union reg_ltm_h70 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_luma_gain_lut_08         : 8;
		uint32_t ltm_ee_luma_gain_lut_09         : 8;
		uint32_t ltm_ee_luma_gain_lut_10         : 8;
		uint32_t ltm_ee_luma_gain_lut_11         : 8;
	} bits;
};

union reg_ltm_h74 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_luma_gain_lut_12         : 8;
		uint32_t ltm_ee_luma_gain_lut_13         : 8;
		uint32_t ltm_ee_luma_gain_lut_14         : 8;
		uint32_t ltm_ee_luma_gain_lut_15         : 8;
	} bits;
};

union reg_ltm_h78 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_luma_gain_lut_16         : 8;
		uint32_t ltm_ee_luma_gain_lut_17         : 8;
		uint32_t ltm_ee_luma_gain_lut_18         : 8;
		uint32_t ltm_ee_luma_gain_lut_19         : 8;
	} bits;
};

union reg_ltm_h7c {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_luma_gain_lut_20         : 8;
		uint32_t ltm_ee_luma_gain_lut_21         : 8;
		uint32_t ltm_ee_luma_gain_lut_22         : 8;
		uint32_t ltm_ee_luma_gain_lut_23         : 8;
	} bits;
};

union reg_ltm_h80 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_luma_gain_lut_24         : 8;
		uint32_t ltm_ee_luma_gain_lut_25         : 8;
		uint32_t ltm_ee_luma_gain_lut_26         : 8;
		uint32_t ltm_ee_luma_gain_lut_27         : 8;
	} bits;
};

union reg_ltm_h84 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_luma_gain_lut_28         : 8;
		uint32_t ltm_ee_luma_gain_lut_29         : 8;
		uint32_t ltm_ee_luma_gain_lut_30         : 8;
		uint32_t ltm_ee_luma_gain_lut_31         : 8;
	} bits;
};

union reg_ltm_h88 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_luma_gain_lut_32         : 8;
	} bits;
};

union reg_ltm_h8c {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 12;
		uint32_t lmap_w_bit                      : 3;
		uint32_t _rsv_15                         : 13;
		uint32_t lmap_h_bit                      : 3;
	} bits;
};

union reg_ltm_h90 {
	uint32_t raw;
	struct {
		uint32_t ltm_v_blend_thd_l               : 10;
		uint32_t ltm_v_blend_thd_h               : 10;
	} bits;
};

union reg_ltm_h94 {
	uint32_t raw;
	struct {
		uint32_t ltm_v_blend_wgt_min             : 9;
		uint32_t ltm_v_blend_wgt_max             : 9;
	} bits;
};

union reg_ltm_h98 {
	uint32_t raw;
	struct {
		uint32_t ltm_v_blend_wgt_slope           : 19;
	} bits;
};

union reg_ltm_h9c {
	uint32_t raw;
	struct {
		uint32_t ltm_de_lmap_lut_in_0            : 8;
		uint32_t ltm_de_lmap_lut_in_1            : 8;
		uint32_t ltm_de_lmap_lut_in_2            : 8;
		uint32_t ltm_de_lmap_lut_in_3            : 8;
	} bits;
};

union reg_ltm_ha0 {
	uint32_t raw;
	struct {
		uint32_t ltm_de_lmap_lut_out_0           : 8;
		uint32_t ltm_de_lmap_lut_out_1           : 8;
		uint32_t ltm_de_lmap_lut_out_2           : 8;
		uint32_t ltm_de_lmap_lut_out_3           : 8;
	} bits;
};

union reg_ltm_ha4 {
	uint32_t raw;
	struct {
		uint32_t ltm_de_lmap_lut_slope_0         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ltm_de_lmap_lut_slope_1         : 12;
	} bits;
};

union reg_ltm_ha8 {
	uint32_t raw;
	struct {
		uint32_t ltm_de_lmap_lut_slope_2         : 12;
	} bits;
};

union reg_ltm_hac {
	uint32_t raw;
	struct {
		uint32_t ltm_be_lmap_lut_in_0            : 8;
		uint32_t ltm_be_lmap_lut_in_1            : 8;
		uint32_t ltm_be_lmap_lut_in_2            : 8;
		uint32_t ltm_be_lmap_lut_in_3            : 8;
	} bits;
};

union reg_ltm_hb0 {
	uint32_t raw;
	struct {
		uint32_t ltm_be_lmap_lut_out_0           : 8;
		uint32_t ltm_be_lmap_lut_out_1           : 8;
		uint32_t ltm_be_lmap_lut_out_2           : 8;
		uint32_t ltm_be_lmap_lut_out_3           : 8;
	} bits;
};

union reg_ltm_hb4 {
	uint32_t raw;
	struct {
		uint32_t ltm_be_lmap_lut_slope_0         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ltm_be_lmap_lut_slope_1         : 12;
	} bits;
};

union reg_ltm_hb8 {
	uint32_t raw;
	struct {
		uint32_t ltm_be_lmap_lut_slope_2         : 12;
	} bits;
};

union reg_ltm_hbc {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_motion_lut_in_0          : 8;
		uint32_t ltm_ee_motion_lut_in_1          : 8;
		uint32_t ltm_ee_motion_lut_in_2          : 8;
		uint32_t ltm_ee_motion_lut_in_3          : 8;
	} bits;
};

union reg_ltm_hc0 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_motion_lut_out_0         : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t ltm_ee_motion_lut_out_1         : 9;
	} bits;
};

union reg_ltm_hc4 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_motion_lut_out_2         : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t ltm_ee_motion_lut_out_3         : 9;
	} bits;
};

union reg_ltm_hc8 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_motion_lut_slope_0       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ltm_ee_motion_lut_slope_1       : 12;
	} bits;
};

union reg_ltm_hcc {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_motion_lut_slope_2       : 12;
	} bits;
};

union reg_ltm_hd0 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_total_oshtthrd           : 8;
		uint32_t ltm_ee_total_ushtthrd           : 8;
		uint32_t ltm_ee_shtctrl_oshtgain         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ltm_ee_shtctrl_ushtgain         : 6;
	} bits;
};

union reg_ltm_hd4 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_total_oshtthrd_clp       : 8;
		uint32_t ltm_ee_total_ushtthrd_clp       : 8;
	} bits;
};

union reg_ltm_hd8 {
	uint32_t raw;
	struct {
		uint32_t ltm_ee_upper_bound_left_diff    : 8;
		uint32_t ltm_ee_lower_bound_right_diff   : 8;
	} bits;
};

union reg_ltm_hdc {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 16;
		uint32_t ltm_ee_min_y                    : 12;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_ca_lite_00 {
	uint32_t raw;
	struct {
		uint32_t ca_lite_enable                  : 1;
		uint32_t ca_lite_shdw_read_sel           : 1;
	} bits;
};

union reg_ca_lite_04 {
	uint32_t raw;
	struct {
		uint32_t ca_lite_lut_in_0                : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t ca_lite_lut_in_1                : 9;
	} bits;
};

union reg_ca_lite_08 {
	uint32_t raw;
	struct {
		uint32_t ca_lite_lut_in_2                : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t ca_lite_lut_in_3                : 9;
	} bits;
};

union reg_ca_lite_0c {
	uint32_t raw;
	struct {
		uint32_t ca_lite_lut_in_4                : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t ca_lite_lut_in_5                : 9;
	} bits;
};

union reg_ca_lite_10 {
	uint32_t raw;
	struct {
		uint32_t ca_lite_lut_out_0               : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ca_lite_lut_out_1               : 11;
	} bits;
};

union reg_ca_lite_14 {
	uint32_t raw;
	struct {
		uint32_t ca_lite_lut_out_2               : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ca_lite_lut_out_3               : 11;
	} bits;
};

union reg_ca_lite_18 {
	uint32_t raw;
	struct {
		uint32_t ca_lite_lut_out_4               : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ca_lite_lut_out_5               : 11;
	} bits;
};

union reg_ca_lite_1c {
	uint32_t raw;
	struct {
		uint32_t ca_lite_lut_slp_0               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ca_lite_lut_slp_1               : 12;
	} bits;
};

union reg_ca_lite_20 {
	uint32_t raw;
	struct {
		uint32_t ca_lite_lut_slp_2               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ca_lite_lut_slp_3               : 12;
	} bits;
};

union reg_ca_lite_24 {
	uint32_t raw;
	struct {
		uint32_t ca_lite_lut_slp_4               : 12;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_444_422_0 {
	uint32_t raw;
	struct {
		uint32_t op_start                        : 1;
		uint32_t cont_en                         : 1;
		uint32_t bypass_en                       : 1;
		uint32_t softrst                         : 1;
		uint32_t auto_update_en                  : 1;
		uint32_t dbg_en                          : 1;
		uint32_t _rsv_6                          : 1;
		uint32_t force_clk_en                    : 1;
	} bits;
};

union reg_isp_444_422_1 {
	uint32_t raw;
	struct {
		uint32_t shdw_update_req                 : 1;
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_isp_444_422_2 {
	uint32_t raw;
	struct {
		uint32_t fd_int                          : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t frame_overflow                  : 1;
	} bits;
};

union reg_isp_444_422_3 {
	uint32_t raw;
	struct {
		uint32_t checksum                        : 32;
	} bits;
};

union reg_isp_444_422_4 {
	uint32_t raw;
	struct {
		uint32_t reg_422_444                     : 1;
		uint32_t swap                            : 2;
	} bits;
};

union reg_isp_444_422_5 {
	uint32_t raw;
	struct {
		uint32_t first_frame_reset               : 1;
		uint32_t tdnr_enable                     : 2;
		uint32_t dma_crop_enable                 : 1;
		uint32_t force_mono_enable               : 1;
		uint32_t debug_status_en                 : 1;
		uint32_t reg_3dnr_comp_gain_enable       : 1;
		uint32_t reg_3dnr_lumaref_lpf_enable     : 1;
	} bits;
};

union reg_isp_444_422_6 {
	uint32_t raw;
	struct {
		uint32_t tdnr_debug_status               : 32;
	} bits;
};

union reg_isp_444_422_8 {
	uint32_t raw;
	struct {
		uint32_t guard_cnt                       : 8;
		uint32_t force_dma_disable               : 6;
		uint32_t uv_rounding_type_sel            : 1;
		uint32_t tdnr_pixel_lp                   : 1;
		uint32_t tdnr_debug_sel                  : 16;
	} bits;
};

union reg_isp_444_422_9 {
	uint32_t raw;
	struct {
		uint32_t dma_write_sel_y                 : 1;
		uint32_t dma_write_sel_c                 : 1;
		uint32_t dma_sel                         : 1;
		uint32_t _rsv_3                          : 2;
		uint32_t avg_mode_read                   : 1;
		uint32_t avg_mode_write                  : 1;
		uint32_t drop_mode_write                 : 1;
	} bits;
};

union reg_isp_444_422_10 {
	uint32_t raw;
	struct {
		uint32_t img_width_crop                  : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_height_crop                 : 14;
		uint32_t _rsv_30                         : 1;
		uint32_t crop_enable                     : 1;
	} bits;
};

union reg_isp_444_422_11 {
	uint32_t raw;
	struct {
		uint32_t crop_w_str                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_w_end                      : 14;
	} bits;
};

union reg_isp_444_422_12 {
	uint32_t raw;
	struct {
		uint32_t crop_h_str                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t crop_h_end                      : 14;
	} bits;
};

union reg_isp_444_422_13 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_y_lut_in_0             : 8;
		uint32_t reg_3dnr_y_lut_in_1             : 8;
		uint32_t reg_3dnr_y_lut_in_2             : 8;
		uint32_t reg_3dnr_y_lut_in_3             : 8;
	} bits;
};

union reg_isp_444_422_14 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_y_lut_out_0            : 8;
		uint32_t reg_3dnr_y_lut_out_1            : 8;
		uint32_t reg_3dnr_y_lut_out_2            : 8;
		uint32_t reg_3dnr_y_lut_out_3            : 8;
	} bits;
};

union reg_isp_444_422_15 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_y_lut_slope_0          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_y_lut_slope_1          : 12;
	} bits;
};

union reg_isp_444_422_16 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_y_lut_slope_2          : 12;
		uint32_t _rsv_12                         : 3;
		uint32_t motion_sel                      : 1;
		uint32_t reg_3dnr_y_beta_max             : 8;
	} bits;
};

union reg_isp_444_422_17 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_u_lut_in_0             : 8;
		uint32_t reg_3dnr_u_lut_in_1             : 8;
		uint32_t reg_3dnr_u_lut_in_2             : 8;
		uint32_t reg_3dnr_u_lut_in_3             : 8;
	} bits;
};

union reg_isp_444_422_18 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_u_lut_out_0            : 8;
		uint32_t reg_3dnr_u_lut_out_1            : 8;
		uint32_t reg_3dnr_u_lut_out_2            : 8;
		uint32_t reg_3dnr_u_lut_out_3            : 8;
	} bits;
};

union reg_isp_444_422_19 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_u_lut_slope_0          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_u_lut_slope_1          : 12;
	} bits;
};

union reg_isp_444_422_20 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_u_lut_slope_2          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_u_beta_max             : 8;
	} bits;
};

union reg_isp_444_422_21 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_v_lut_in_0             : 8;
		uint32_t reg_3dnr_v_lut_in_1             : 8;
		uint32_t reg_3dnr_v_lut_in_2             : 8;
		uint32_t reg_3dnr_v_lut_in_3             : 8;
	} bits;
};

union reg_isp_444_422_22 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_v_lut_out_0            : 8;
		uint32_t reg_3dnr_v_lut_out_1            : 8;
		uint32_t reg_3dnr_v_lut_out_2            : 8;
		uint32_t reg_3dnr_v_lut_out_3            : 8;
	} bits;
};

union reg_isp_444_422_23 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_v_lut_slope_0          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_v_lut_slope_1          : 12;
	} bits;
};

union reg_isp_444_422_24 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_v_lut_slope_2          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_v_beta_max             : 8;
	} bits;
};

union reg_isp_444_422_25 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_motion_y_lut_in_0      : 8;
		uint32_t reg_3dnr_motion_y_lut_in_1      : 8;
		uint32_t reg_3dnr_motion_y_lut_in_2      : 8;
		uint32_t reg_3dnr_motion_y_lut_in_3      : 8;
	} bits;
};

union reg_isp_444_422_26 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_motion_y_lut_out_0     : 8;
		uint32_t reg_3dnr_motion_y_lut_out_1     : 8;
		uint32_t reg_3dnr_motion_y_lut_out_2     : 8;
		uint32_t reg_3dnr_motion_y_lut_out_3     : 8;
	} bits;
};

union reg_isp_444_422_27 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_motion_y_lut_slope_0   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_motion_y_lut_slope_1   : 12;
	} bits;
};

union reg_isp_444_422_28 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_motion_y_lut_slope_2   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_motion_c_lut_slope_0   : 12;
	} bits;
};

union reg_isp_444_422_29 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_motion_c_lut_slope_1   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_motion_c_lut_slope_2   : 12;
	} bits;
};

union reg_isp_444_422_30 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_motion_c_lut_in_0      : 8;
		uint32_t reg_3dnr_motion_c_lut_in_1      : 8;
		uint32_t reg_3dnr_motion_c_lut_in_2      : 8;
		uint32_t reg_3dnr_motion_c_lut_in_3      : 8;
	} bits;
};

union reg_isp_444_422_31 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_motion_c_lut_out_0     : 8;
		uint32_t reg_3dnr_motion_c_lut_out_1     : 8;
		uint32_t reg_3dnr_motion_c_lut_out_2     : 8;
		uint32_t reg_3dnr_motion_c_lut_out_3     : 8;
	} bits;
};

union reg_isp_444_422_80 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_ee_comp_gain           : 9;
	} bits;
};

union reg_isp_444_422_84 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_luma_comp_gain_lut_00  : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_luma_comp_gain_lut_01  : 10;
	} bits;
};

union reg_isp_444_422_88 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_luma_comp_gain_lut_02  : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_luma_comp_gain_lut_03  : 10;
	} bits;
};

union reg_isp_444_422_8c {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_luma_comp_gain_lut_04  : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_luma_comp_gain_lut_05  : 10;
	} bits;
};

union reg_isp_444_422_90 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_luma_comp_gain_lut_06  : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_luma_comp_gain_lut_07  : 10;
	} bits;
};

union reg_isp_444_422_94 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_luma_comp_gain_lut_08  : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_luma_comp_gain_lut_09  : 10;
	} bits;
};

union reg_isp_444_422_98 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_luma_comp_gain_lut_10  : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_luma_comp_gain_lut_11  : 10;
	} bits;
};

union reg_isp_444_422_9c {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_luma_comp_gain_lut_12  : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_luma_comp_gain_lut_13  : 10;
	} bits;
};

union reg_isp_444_422_a0 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_luma_comp_gain_lut_14  : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_luma_comp_gain_lut_15  : 10;
	} bits;
};

union reg_isp_444_422_a4 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_luma_comp_gain_lut_16  : 10;
	} bits;
};

union reg_isp_444_422_a8 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_centerx            : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t reg_3dnr_lsc_centery            : 13;
	} bits;
};

union reg_isp_444_422_ac {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_norm               : 15;
		uint32_t _rsv_15                         : 1;
		uint32_t reg_3dnr_lsc_dy_gain            : 8;
	} bits;
};

union reg_isp_444_422_b0 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_comp_gain_lut_00   : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_lsc_comp_gain_lut_01   : 10;
	} bits;
};

union reg_isp_444_422_b4 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_comp_gain_lut_02   : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_lsc_comp_gain_lut_03   : 10;
	} bits;
};

union reg_isp_444_422_b8 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_comp_gain_lut_04   : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_lsc_comp_gain_lut_05   : 10;
	} bits;
};

union reg_isp_444_422_bc {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_comp_gain_lut_06   : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_lsc_comp_gain_lut_07   : 10;
	} bits;
};

union reg_isp_444_422_c0 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_comp_gain_lut_08   : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_lsc_comp_gain_lut_09   : 10;
	} bits;
};

union reg_isp_444_422_c4 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_comp_gain_lut_10   : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_lsc_comp_gain_lut_11   : 10;
	} bits;
};

union reg_isp_444_422_c8 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_comp_gain_lut_12   : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_lsc_comp_gain_lut_13   : 10;
	} bits;
};

union reg_isp_444_422_cc {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_comp_gain_lut_14   : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t reg_3dnr_lsc_comp_gain_lut_15   : 10;
	} bits;
};

union reg_isp_444_422_d0 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_lsc_comp_gain_lut_16   : 10;
	} bits;
};

union reg_isp_444_422_d4 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_y_gain_lut_in_0        : 8;
		uint32_t reg_3dnr_y_gain_lut_in_1        : 8;
		uint32_t reg_3dnr_y_gain_lut_in_2        : 8;
		uint32_t reg_3dnr_y_gain_lut_in_3        : 8;
	} bits;
};

union reg_isp_444_422_d8 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_y_gain_lut_out_0       : 8;
		uint32_t reg_3dnr_y_gain_lut_out_1       : 8;
		uint32_t reg_3dnr_y_gain_lut_out_2       : 8;
		uint32_t reg_3dnr_y_gain_lut_out_3       : 8;
	} bits;
};

union reg_isp_444_422_dc {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_y_gain_lut_slope_0     : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_y_gain_lut_slope_1     : 12;
	} bits;
};

union reg_isp_444_422_e0 {
	uint32_t raw;
	struct {
		uint32_t reg_3dnr_y_gain_lut_slope_2     : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t reg_3dnr_y_motion_max           : 8;
		uint32_t reg_3dnr_c_motion_max           : 8;
	} bits;
};

union reg_isp_444_422_e4 {
	uint32_t raw;
	struct {
		uint32_t mot_debug_lut_in_0              : 8;
		uint32_t mot_debug_lut_in_1              : 8;
		uint32_t mot_debug_lut_in_2              : 8;
		uint32_t mot_debug_lut_in_3              : 8;
	} bits;
};

union reg_isp_444_422_e8 {
	uint32_t raw;
	struct {
		uint32_t mot_debug_lut_out_0             : 8;
		uint32_t mot_debug_lut_out_1             : 8;
		uint32_t mot_debug_lut_out_2             : 8;
		uint32_t mot_debug_lut_out_3             : 8;
	} bits;
};

union reg_isp_444_422_ec {
	uint32_t raw;
	struct {
		uint32_t mot_debug_lut_slope_0           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mot_debug_lut_slope_1           : 12;
	} bits;
};

union reg_isp_444_422_f0 {
	uint32_t raw;
	struct {
		uint32_t mot_debug_lut_slope_2           : 12;
		uint32_t mot_debug_switch                : 1;
		uint32_t reg_3dnr_y_pix_gain_enable      : 1;
		uint32_t reg_3dnr_c_pix_gain_enable      : 1;
		uint32_t reg_3dnr_pix_gain_mode          : 1;
	} bits;
};

union reg_isp_444_422_f4 {
	uint32_t raw;
	struct {
		uint32_t tdnr_jnd_ths                    : 8;
		uint32_t tdnr_jnd_sat                    : 8;
		uint32_t _rsv_16                         : 4;
		uint32_t tdnr_jnd_gain_rsb               : 3;
		uint32_t tdnr_max_tf_num                 : 4;
	} bits;
};

union reg_isp_444_422_f8 {
	uint32_t raw;
	struct {
		uint32_t tdnr_max_noise_level            : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t tdnr_min_noise_level            : 8;
	} bits;
};

union reg_isp_444_422_fc {
	uint32_t raw;
	struct {
		uint32_t tdnr_pixel_diff_motionout       : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t tdnr_vec1_0                     : 3;
		uint32_t _rsv_19                         : 3;
		uint32_t tdnr_vec2_0                     : 3;
		uint32_t _rsv_25                         : 3;
		uint32_t tdnr_vec3_0                     : 3;
	} bits;
};

union reg_isp_444_422_100 {
	uint32_t raw;
	struct {
		uint32_t bypass_h                        : 1;
		uint32_t _rsv_1                          : 15;
		uint32_t bypass_v                        : 1;
	} bits;
};
/******************************************/
/*           module definition            */
/******************************************/
union reg_fbce_00 {
	uint32_t raw;
	struct {
		uint32_t fbce_en                         : 1;
		uint32_t shd_rd                          : 1;
		uint32_t force_cke                       : 1;
		uint32_t _rsv_3                          : 5;
		uint32_t debug                           : 16;
		uint32_t dummy                           : 8;
	} bits;
};

union reg_fbce_10 {
	uint32_t raw;
	struct {
		uint32_t y_lossless                      : 1;
		uint32_t y_base_qdpcm_q                  : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t y_base_pcm_bd_minus2            : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t y_default_gr_k                  : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t y_cplx_shift                    : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t y_pen_pos_shift                 : 3;
	} bits;
};

union reg_fbce_14 {
	uint32_t raw;
	struct {
		uint32_t y_min_cu_bit                    : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t y_max_cu_bit                    : 7;
	} bits;
};

union reg_fbce_18 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 6;
		uint32_t y_bit_stream_size               : 22;
	} bits;
};

union reg_fbce_1c {
	uint32_t raw;
	struct {
		uint32_t y_total_line_bit_budget         : 17;
	} bits;
};

union reg_fbce_20 {
	uint32_t raw;
	struct {
		uint32_t c_lossless                      : 1;
		uint32_t c_base_qdpcm_q                  : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t c_base_pcm_bd_minus2            : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t c_default_gr_k                  : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t c_cplx_shift                    : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t c_pen_pos_shift                 : 3;
	} bits;
};

union reg_fbce_24 {
	uint32_t raw;
	struct {
		uint32_t c_min_cu_bit                    : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t c_max_cu_bit                    : 7;
	} bits;
};

union reg_fbce_28 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 6;
		uint32_t c_bit_stream_size               : 22;
	} bits;
};

union reg_fbce_2c {
	uint32_t raw;
	struct {
		uint32_t c_total_line_bit_budget         : 17;
	} bits;
};

union reg_fbce_30 {
	uint32_t raw;
	struct {
		uint32_t y_total_first_line_bit_budget   : 17;
	} bits;
};

union reg_fbce_34 {
	uint32_t raw;
	struct {
		uint32_t c_total_first_line_bit_budget   : 17;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_fbcd_00 {
	uint32_t raw;
	struct {
		uint32_t fbcd_en                         : 1;
		uint32_t ff_drop                         : 1;
		uint32_t fbcd_auto                       : 1;
		uint32_t force_cke                       : 1;
	} bits;
};

union reg_fbcd_0c {
	uint32_t raw;
	struct {
		uint32_t y_bit_stream_size               : 32;
	} bits;
};

union reg_fbcd_10 {
	uint32_t raw;
	struct {
		uint32_t c_bit_stream_size               : 32;
	} bits;
};

union reg_fbcd_14 {
	uint32_t raw;
	struct {
		uint32_t debug_sel                       : 4;
	} bits;
};

union reg_fbcd_18 {
	uint32_t raw;
	struct {
		uint32_t debug_bus                       : 32;
	} bits;
};

union reg_fbcd_20 {
	uint32_t raw;
	struct {
		uint32_t dummy                           : 8;
		uint32_t shd_rd                          : 1;
	} bits;
};

union reg_fbcd_24 {
	uint32_t raw;
	struct {
		uint32_t y_lossless                      : 1;
		uint32_t y_base_qdpcm_q                  : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t y_base_pcm_bd_minus2            : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t y_default_gr_k                  : 3;
	} bits;
};

union reg_fbcd_28 {
	uint32_t raw;
	struct {
		uint32_t c_lossless                      : 1;
		uint32_t c_base_qdpcm_q                  : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t c_base_pcm_bd_minus2            : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t c_default_gr_k                  : 3;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_yuv_dither_y_dither {
	uint32_t raw;
	struct {
		uint32_t y_dither_enable                 : 1;
		uint32_t y_dither_mod_enable             : 1;
		uint32_t y_dither_histidx_enable         : 1;
		uint32_t y_dither_fmnum_enable           : 1;
		uint32_t y_dither_shdw_sel               : 1;
		uint32_t y_dither_softrst                : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t y_dither_heightm1               : 12;
		uint32_t y_dither_widthm1                : 12;
	} bits;
};

union reg_isp_yuv_dither_uv_dither {
	uint32_t raw;
	struct {
		uint32_t uv_dither_enable                : 1;
		uint32_t uv_dither_mod_enable            : 1;
		uint32_t uv_dither_histidx_enable        : 1;
		uint32_t uv_dither_fmnum_enable          : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t uv_dither_heightm1              : 12;
		uint32_t uv_dither_widthm1               : 12;
	} bits;
};

union reg_isp_yuv_dither_debug_00 {
	uint32_t raw;
	struct {
		uint32_t uv_dither_debug0                : 32;
	} bits;
};

union reg_isp_yuv_dither_debug_01 {
	uint32_t raw;
	struct {
		uint32_t y_dither_debug0                 : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_ynr_shadow_rd_sel   {
	uint32_t raw;
	struct {
		uint32_t shadow_rd_sel                   : 1;
	} bits;
};

union reg_isp_ynr_out_sel         {
	uint32_t raw;
	struct {
		uint32_t ynr_out_sel                     : 4;
	} bits;
};

union reg_isp_ynr_index_clr {
	uint32_t raw;
	struct {
		uint32_t ynr_index_clr                   : 1;
	} bits;
};

union reg_isp_ynr_ns0_luma_th_00 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_luma_th_00              : 8;
	} bits;
};

union reg_isp_ynr_ns0_luma_th_01 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_luma_th_01              : 8;
	} bits;
};

union reg_isp_ynr_ns0_luma_th_02 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_luma_th_02              : 8;
	} bits;
};

union reg_isp_ynr_ns0_luma_th_03 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_luma_th_03              : 8;
	} bits;
};

union reg_isp_ynr_ns0_luma_th_04 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_luma_th_04              : 8;
	} bits;
};

union reg_isp_ynr_ns0_luma_th_05 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_luma_th_05              : 8;
	} bits;
};

union reg_isp_ynr_ns0_slope_00       {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_slope_00                : 11;
	} bits;
};

union reg_isp_ynr_ns0_slope_01 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_slope_01                : 11;
	} bits;
};

union reg_isp_ynr_ns0_slope_02 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_slope_02                : 11;
	} bits;
};

union reg_isp_ynr_ns0_slope_03 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_slope_03                : 11;
	} bits;
};

union reg_isp_ynr_ns0_slope_04 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_slope_04                : 11;
	} bits;
};

union reg_isp_ynr_ns0_offset_00 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_offset_00               : 8;
	} bits;
};

union reg_isp_ynr_ns0_offset_01 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_offset_01               : 8;
	} bits;
};

union reg_isp_ynr_ns0_offset_02 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_offset_02               : 8;
	} bits;
};

union reg_isp_ynr_ns0_offset_03 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_offset_03               : 8;
	} bits;
};

union reg_isp_ynr_ns0_offset_04 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_offset_04               : 8;
	} bits;
};

union reg_isp_ynr_ns0_offset_05 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns0_offset_05               : 8;
	} bits;
};

union reg_isp_ynr_ns1_luma_th_00 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_luma_th_00              : 8;
	} bits;
};

union reg_isp_ynr_ns1_luma_th_01 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_luma_th_01              : 8;
	} bits;
};

union reg_isp_ynr_ns1_luma_th_02 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_luma_th_02              : 8;
	} bits;
};

union reg_isp_ynr_ns1_luma_th_03 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_luma_th_03              : 8;
	} bits;
};

union reg_isp_ynr_ns1_luma_th_04 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_luma_th_04              : 8;
	} bits;
};

union reg_isp_ynr_ns1_luma_th_05 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_luma_th_05              : 8;
	} bits;
};

union reg_isp_ynr_ns1_slope_00 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_slope_00                : 11;
	} bits;
};

union reg_isp_ynr_ns1_slope_01 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_slope_01                : 11;
	} bits;
};

union reg_isp_ynr_ns1_slope_02 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_slope_02                : 11;
	} bits;
};

union reg_isp_ynr_ns1_slope_03 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_slope_03                : 11;
	} bits;
};

union reg_isp_ynr_ns1_slope_04 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_slope_04                : 11;
	} bits;
};

union reg_isp_ynr_ns1_offset_00 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_offset_00               : 8;
	} bits;
};

union reg_isp_ynr_ns1_offset_01 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_offset_01               : 8;
	} bits;
};

union reg_isp_ynr_ns1_offset_02 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_offset_02               : 8;
	} bits;
};

union reg_isp_ynr_ns1_offset_03 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_offset_03               : 8;
	} bits;
};

union reg_isp_ynr_ns1_offset_04 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_offset_04               : 8;
	} bits;
};

union reg_isp_ynr_ns1_offset_05 {
	uint32_t raw;
	struct {
		uint32_t ynr_ns1_offset_05               : 8;
	} bits;
};

union reg_isp_ynr_ns_gain         {
	uint32_t raw;
	struct {
		uint32_t ynr_ns_gain                     : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_00 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_00               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_01 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_01               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_02 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_02               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_03 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_03               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_04 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_04               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_05 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_05               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_06 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_06               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_07 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_07               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_08 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_08               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_09 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_09               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_10 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_10               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_11 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_11               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_12 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_12               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_13 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_13               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_14 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_14               : 8;
	} bits;
};

union reg_isp_ynr_motion_lut_15 {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_lut_15               : 8;
	} bits;
};

union reg_isp_ynr_weight_intra_0  {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_intra_0              : 3;
	} bits;
};

union reg_isp_ynr_weight_intra_1  {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_intra_1              : 3;
	} bits;
};

union reg_isp_ynr_weight_intra_2  {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_intra_2              : 3;
	} bits;
};

union reg_isp_ynr_weight_norm_1   {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_norm_1               : 7;
	} bits;
};

union reg_isp_ynr_weight_norm_2   {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_norm_2               : 8;
	} bits;
};

union reg_isp_ynr_alpha_gain      {
	uint32_t raw;
	struct {
		uint32_t ynr_alpha_gain                  : 10;
	} bits;
};

union reg_isp_ynr_var_th          {
	uint32_t raw;
	struct {
		uint32_t ynr_var_th                      : 8;
	} bits;
};

union reg_isp_ynr_weight_sm       {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_smooth               : 5;
	} bits;
};

union reg_isp_ynr_weight_v        {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_v                    : 5;
	} bits;
};

union reg_isp_ynr_weight_h        {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_h                    : 5;
	} bits;
};

union reg_isp_ynr_weight_d45      {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_d45                  : 5;
	} bits;
};

union reg_isp_ynr_weight_d135     {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_d135                 : 5;
	} bits;
};

union reg_isp_ynr_neighbor_max    {
	uint32_t raw;
	struct {
		uint32_t ynr_flag_neighbor_max_weight    : 1;
	} bits;
};

union reg_isp_ynr_res_k_smooth    {
	uint32_t raw;
	struct {
		uint32_t ynr_res_ratio_k_smooth          : 9;
	} bits;
};

union reg_isp_ynr_res_k_texture   {
	uint32_t raw;
	struct {
		uint32_t ynr_res_ratio_k_texture         : 9;
	} bits;
};

union reg_isp_ynr_filter_mode_en {
	uint32_t raw;
	struct {
		uint32_t ynr_filter_mode_enable          : 1;
	} bits;
};

union reg_isp_ynr_filter_mode_alpha {
	uint32_t raw;
	struct {
		uint32_t ynr_filter_mode_alpha           : 9;
	} bits;
};

union reg_isp_ynr_res_mot_lut_00 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_00              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_01 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_01              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_02 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_02              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_03 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_03              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_04 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_04              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_05 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_05              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_06 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_06              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_07 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_07              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_08 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_08              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_09 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_09              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_10 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_10              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_11 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_11              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_12 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_12              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_13 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_13              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_14 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_14              : 8;
	} bits;
};

union reg_isp_ynr_res_mot_lut_15 {
	uint32_t raw;
	struct {
		uint32_t ynr_res_mot_lut_15              : 8;
	} bits;
};

union reg_isp_ynr_res_max {
	uint32_t raw;
	struct {
		uint32_t ynr_res_max                     : 8;
	} bits;
};

union reg_isp_ynr_res_motion_max {
	uint32_t raw;
	struct {
		uint32_t ynr_res_motion_max              : 8;
	} bits;
};

union reg_isp_ynr_motion_ns_clip_max {
	uint32_t raw;
	struct {
		uint32_t ynr_motion_ns_clip_max          : 8;
	} bits;
};

union reg_isp_ynr_weight_lut      {
	uint32_t raw;
	struct {
		uint32_t ynr_weight_lut                  : 5;
	} bits;
};

union reg_isp_ynr_dummy           {
	uint32_t raw;
	struct {
		uint32_t ynr_dummy                       : 16;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_cnr_enable {
	uint32_t raw;
	struct {
		uint32_t cnr_enable                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t pfc_enable                      : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t cnr_diff_shift_val              : 8;
		uint32_t cnr_ratio                       : 8;
		uint32_t cnr_out_sel                     : 2;
	} bits;
};

union reg_isp_cnr_strength_mode {
	uint32_t raw;
	struct {
		uint32_t cnr_strength_mode               : 8;
		uint32_t cnr_fusion_intensity_weight     : 4;
		uint32_t _rsv_12                         : 4;
		uint32_t cnr_weight_inter_sel            : 4;
		uint32_t cnr_var_th                      : 9;
		uint32_t _rsv_29                         : 1;
		uint32_t cnr_flag_neighbor_max_weight    : 1;
		uint32_t cnr_shdw_sel                    : 1;
	} bits;
};

union reg_isp_cnr_purple_th {
	uint32_t raw;
	struct {
		uint32_t cnr_purple_th                   : 8;
		uint32_t cnr_correct_strength            : 8;
		uint32_t cnr_diff_gain                   : 4;
		uint32_t _rsv_20                         : 4;
		uint32_t cnr_motion_enable               : 1;
	} bits;
};

union reg_isp_cnr_purple_cb {
	uint32_t raw;
	struct {
		uint32_t cnr_purple_cb                   : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t cnr_purple_cr                   : 8;
	} bits;
};

union reg_isp_cnr_green_cb {
	uint32_t raw;
	struct {
		uint32_t cnr_green_cb                    : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t cnr_green_cr                    : 8;
	} bits;
};

union reg_isp_cnr_weight_lut_inter_cnr_00 {
	uint32_t raw;
	struct {
		uint32_t weight_lut_inter_cnr_00         : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t weight_lut_inter_cnr_01         : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t weight_lut_inter_cnr_02         : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t weight_lut_inter_cnr_03         : 5;
	} bits;
};

union reg_isp_cnr_weight_lut_inter_cnr_04 {
	uint32_t raw;
	struct {
		uint32_t weight_lut_inter_cnr_04         : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t weight_lut_inter_cnr_05         : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t weight_lut_inter_cnr_06         : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t weight_lut_inter_cnr_07         : 5;
	} bits;
};

union reg_isp_cnr_weight_lut_inter_cnr_08 {
	uint32_t raw;
	struct {
		uint32_t weight_lut_inter_cnr_08         : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t weight_lut_inter_cnr_09         : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t weight_lut_inter_cnr_10         : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t weight_lut_inter_cnr_11         : 5;
	} bits;
};

union reg_isp_cnr_weight_lut_inter_cnr_12 {
	uint32_t raw;
	struct {
		uint32_t weight_lut_inter_cnr_12         : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t weight_lut_inter_cnr_13         : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t weight_lut_inter_cnr_14         : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t weight_lut_inter_cnr_15         : 5;
	} bits;
};

union reg_isp_cnr_motion_lut_0 {
	uint32_t raw;
	struct {
		uint32_t cnr_motion_lut_0                : 8;
		uint32_t cnr_motion_lut_1                : 8;
		uint32_t cnr_motion_lut_2                : 8;
		uint32_t cnr_motion_lut_3                : 8;
	} bits;
};

union reg_isp_cnr_motion_lut_4 {
	uint32_t raw;
	struct {
		uint32_t cnr_motion_lut_4                : 8;
		uint32_t cnr_motion_lut_5                : 8;
		uint32_t cnr_motion_lut_6                : 8;
		uint32_t cnr_motion_lut_7                : 8;
	} bits;
};

union reg_isp_cnr_motion_lut_8 {
	uint32_t raw;
	struct {
		uint32_t cnr_motion_lut_8                : 8;
		uint32_t cnr_motion_lut_9                : 8;
		uint32_t cnr_motion_lut_10               : 8;
		uint32_t cnr_motion_lut_11               : 8;
	} bits;
};

union reg_isp_cnr_motion_lut_12 {
	uint32_t raw;
	struct {
		uint32_t cnr_motion_lut_12               : 8;
		uint32_t cnr_motion_lut_13               : 8;
		uint32_t cnr_motion_lut_14               : 8;
		uint32_t cnr_motion_lut_15               : 8;
	} bits;
};

union reg_isp_cnr_purple_cb2 {
	uint32_t raw;
	struct {
		uint32_t cnr_purple_cb2                  : 8;
		uint32_t cnr_purple_cr2                  : 8;
		uint32_t cnr_purple_cb3                  : 8;
		uint32_t cnr_purple_cr3                  : 8;
	} bits;
};

union reg_isp_cnr_mask {
	uint32_t raw;
	struct {
		uint32_t cnr_mask                        : 8;
	} bits;
};

union reg_isp_cnr_dummy {
	uint32_t raw;
	struct {
		uint32_t cnr_dummy                       : 32;
	} bits;
};

union reg_isp_cnr_edge_scale {
	uint32_t raw;
	struct {
		uint32_t cnr_edge_scale                  : 8;
		uint32_t cnr_edge_coring                 : 8;
		uint32_t cnr_edge_min                    : 8;
		uint32_t cnr_edge_max                    : 8;
	} bits;
};

union reg_isp_cnr_edge_ratio_speed {
	uint32_t raw;
	struct {
		uint32_t cnr_ratio_speed                 : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t cnr_cb_str                      : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t cnr_cr_str                      : 5;
	} bits;
};

union reg_isp_cnr_depurple_weight_th {
	uint32_t raw;
	struct {
		uint32_t cnr_depurple_weight_th          : 8;
		uint32_t cnr_depurple_str_min_ratio      : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t cnr_depurple_str_max_ratio      : 7;
	} bits;
};

union reg_isp_cnr_coring_motion_lut_0 {
	uint32_t raw;
	struct {
		uint32_t cnr_coring_motion_lut_00        : 8;
		uint32_t cnr_coring_motion_lut_01        : 8;
		uint32_t cnr_coring_motion_lut_02        : 8;
		uint32_t cnr_coring_motion_lut_03        : 8;
	} bits;
};

union reg_isp_cnr_coring_motion_lut_4 {
	uint32_t raw;
	struct {
		uint32_t cnr_coring_motion_lut_04        : 8;
		uint32_t cnr_coring_motion_lut_05        : 8;
		uint32_t cnr_coring_motion_lut_06        : 8;
		uint32_t cnr_coring_motion_lut_07        : 8;
	} bits;
};

union reg_isp_cnr_coring_motion_lut_8 {
	uint32_t raw;
	struct {
		uint32_t cnr_coring_motion_lut_08        : 8;
		uint32_t cnr_coring_motion_lut_09        : 8;
		uint32_t cnr_coring_motion_lut_10        : 8;
		uint32_t cnr_coring_motion_lut_11        : 8;
	} bits;
};

union reg_isp_cnr_coring_motion_lut_12 {
	uint32_t raw;
	struct {
		uint32_t cnr_coring_motion_lut_12        : 8;
		uint32_t cnr_coring_motion_lut_13        : 8;
		uint32_t cnr_coring_motion_lut_14        : 8;
		uint32_t cnr_coring_motion_lut_15        : 8;
	} bits;
};

union reg_isp_cnr_edge_scale_lut_0 {
	uint32_t raw;
	struct {
		uint32_t cnr_edge_scale_lut_00           : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cnr_edge_scale_lut_01           : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cnr_edge_scale_lut_02           : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cnr_edge_scale_lut_03           : 6;
	} bits;
};

union reg_isp_cnr_edge_scale_lut_4 {
	uint32_t raw;
	struct {
		uint32_t cnr_edge_scale_lut_04           : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cnr_edge_scale_lut_05           : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cnr_edge_scale_lut_06           : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cnr_edge_scale_lut_07           : 6;
	} bits;
};

union reg_isp_cnr_edge_scale_lut_8 {
	uint32_t raw;
	struct {
		uint32_t cnr_edge_scale_lut_08           : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cnr_edge_scale_lut_09           : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cnr_edge_scale_lut_10           : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cnr_edge_scale_lut_11           : 6;
	} bits;
};

union reg_isp_cnr_edge_scale_lut_12 {
	uint32_t raw;
	struct {
		uint32_t cnr_edge_scale_lut_12           : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cnr_edge_scale_lut_13           : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cnr_edge_scale_lut_14           : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cnr_edge_scale_lut_15           : 6;
	} bits;
};

union reg_isp_cnr_edge_scale_lut_16 {
	uint32_t raw;
	struct {
		uint32_t cnr_edge_scale_lut_16           : 6;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_ycurv_ycur_ctrl {
	uint32_t raw;
	struct {
		uint32_t ycur_enable                     : 1;
		uint32_t ycur_shdw_sel                   : 1;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_ycurv_ycur_prog_ctrl {
	uint32_t raw;
	struct {
		uint32_t ycur_wsel                       : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t ycur_rsel                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t ycur_prog_en                    : 1;
	} bits;
};

union reg_isp_ycurv_ycur_prog_st_addr {
	uint32_t raw;
	struct {
		uint32_t ycur_st_addr                    : 6;
		uint32_t _rsv_6                          : 25;
		uint32_t ycur_st_w                       : 1;
	} bits;
};

union reg_isp_ycurv_ycur_prog_data {
	uint32_t raw;
	struct {
		uint32_t ycur_data_e                     : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t ycur_data_o                     : 8;
		uint32_t _rsv_24                         : 7;
		uint32_t ycur_w                          : 1;
	} bits;
};

union reg_isp_ycurv_ycur_prog_max {
	uint32_t raw;
	struct {
		uint32_t ycur_max                        : 9;
	} bits;
};

union reg_isp_ycurv_ycur_mem_sw_mode {
	uint32_t raw;
	struct {
		uint32_t ycur_sw_raddr                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ycur_sw_r_mem_sel               : 1;
	} bits;
};

union reg_isp_ycurv_ycur_mem_sw_rdata {
	uint32_t raw;
	struct {
		uint32_t ycur_rdata_r                    : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t ycur_sw_r                       : 1;
	} bits;
};

union reg_isp_ycurv_ycur_dbg {
	uint32_t raw;
	struct {
		uint32_t prog_hdk_dis                    : 1;
		uint32_t softrst                         : 1;
	} bits;
};

union reg_isp_ycurv_ycur_dmy0 {
	uint32_t raw;
	struct {
		uint32_t dmy_def0                        : 32;
	} bits;
};

union reg_isp_ycurv_ycur_dmy1 {
	uint32_t raw;
	struct {
		uint32_t dmy_def1                        : 32;
	} bits;
};

union reg_isp_ycurv_ycur_dmy_r {
	uint32_t raw;
	struct {
		uint32_t dmy_ro                          : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_top_int_event0 {
	uint32_t raw;
	struct {
		uint32_t frame_done_fe0                  : 4;
		uint32_t frame_done_fe1                  : 4;
		uint32_t frame_done_fe2                  : 2;
		uint32_t frame_done_be                   : 2;
		uint32_t frame_done_raw                  : 1;
		uint32_t frame_done_rgb                  : 1;
		uint32_t frame_done_yuv                  : 1;
		uint32_t frame_done_post                 : 1;
		uint32_t shaw_done_fe0                   : 4;
		uint32_t shaw_done_fe1                   : 4;
		uint32_t shaw_done_fe2                   : 2;
		uint32_t shaw_done_be                    : 2;
		uint32_t shaw_done_raw                   : 1;
		uint32_t shaw_done_rgb                   : 1;
		uint32_t shaw_done_yuv                   : 1;
		uint32_t shaw_done_post                  : 1;
	} bits;
};

union reg_isp_top_int_event1 {
	uint32_t raw;
	struct {
		uint32_t pq_done_fe0                     : 4;
		uint32_t pq_done_fe1                     : 4;
		uint32_t pq_done_fe2                     : 2;
		uint32_t pq_done_be                      : 2;
		uint32_t pq_done_raw                     : 1;
		uint32_t pq_done_rgb                     : 1;
		uint32_t pq_done_yuv                     : 1;
		uint32_t pq_done_post                    : 1;
	} bits;
};

union reg_isp_top_int_event2 {
	uint32_t raw;
	struct {
		uint32_t frame_start_fe0                 : 4;
		uint32_t frame_start_fe1                 : 4;
		uint32_t frame_start_fe2                 : 2;
		uint32_t frame_err                       : 1;
		uint32_t pchk_err                        : 1;
		uint32_t cmdq_int                        : 1;
		uint32_t line_intp_fe0                   : 1;
		uint32_t line_intp_fe1                   : 1;
		uint32_t line_intp_fe2                   : 1;
		uint32_t line_intp_post                  : 1;
		uint32_t int_bdg0_lite                   : 1;
		uint32_t int_bdg1_lite                   : 1;
		uint32_t int_dma_err                     : 1;
		uint32_t frame_err_line_spliter_fe0      : 1;
		uint32_t frame_err_line_spliter_fe1      : 1;
	} bits;
};

union reg_isp_top_error_sts {
	uint32_t raw;
	struct {
		uint32_t pchk0_err_fe0                   : 1;
		uint32_t pchk0_err_fe1                   : 1;
		uint32_t pchk0_err_fe2                   : 1;
		uint32_t pchk0_err_be                    : 1;
		uint32_t pchk0_err_raw                   : 1;
		uint32_t pchk0_err_rgb                   : 1;
		uint32_t pchk0_err_yuv                   : 1;
		uint32_t pchk1_err_fe0                   : 1;
		uint32_t pchk1_err_fe1                   : 1;
		uint32_t pchk1_err_fe2                   : 1;
		uint32_t pchk1_err_be                    : 1;
		uint32_t pchk1_err_raw                   : 1;
		uint32_t pchk1_err_rgb                   : 1;
		uint32_t pchk1_err_yuv                   : 1;
	} bits;
};

union reg_isp_top_int_event0_en {
	uint32_t raw;
	struct {
		uint32_t frame_done_enable_fe0           : 4;
		uint32_t frame_done_enable_fe1           : 4;
		uint32_t frame_done_enable_fe2           : 2;
		uint32_t frame_done_enable_be            : 2;
		uint32_t frame_done_enable_raw           : 1;
		uint32_t frame_done_enable_rgb           : 1;
		uint32_t frame_done_enable_yuv           : 1;
		uint32_t frame_done_enable_post          : 1;
		uint32_t shaw_done_enable_fe0            : 4;
		uint32_t shaw_done_enable_fe1            : 4;
		uint32_t shaw_done_enable_fe2            : 2;
		uint32_t shaw_done_enable_be             : 2;
		uint32_t shaw_done_enable_raw            : 1;
		uint32_t shaw_done_enable_rgb            : 1;
		uint32_t shaw_done_enable_yuv            : 1;
		uint32_t shaw_done_enable_post           : 1;
	} bits;
};

union reg_isp_top_int_event1_en {
	uint32_t raw;
	struct {
		uint32_t pq_done_enable_fe0              : 4;
		uint32_t pq_done_enable_fe1              : 4;
		uint32_t pq_done_enable_fe2              : 2;
		uint32_t pq_done_enable_be               : 2;
		uint32_t pq_done_enable_raw              : 1;
		uint32_t pq_done_enable_rgb              : 1;
		uint32_t pq_done_enable_yuv              : 1;
		uint32_t pq_done_enable_post             : 1;
	} bits;
};

union reg_isp_top_int_event2_en {
	uint32_t raw;
	struct {
		uint32_t frame_start_enable_fe0          : 4;
		uint32_t frame_start_enable_fe1          : 4;
		uint32_t frame_start_enable_fe2          : 2;
		uint32_t frame_err_enable                : 1;
		uint32_t pchk_err_enable                 : 1;
		uint32_t cmdq_int_enable                 : 1;
		uint32_t line_intp_enable_fe0            : 1;
		uint32_t line_intp_enable_fe1            : 1;
		uint32_t line_intp_enable_fe2            : 1;
		uint32_t line_intp_enable_post           : 1;
		uint32_t int_bdg0_lite_enable            : 1;
		uint32_t int_bdg1_lite_enable            : 1;
		uint32_t int_dma_err_enable              : 1;
		uint32_t frame_err_line_spliter_enable_fe0: 1;
		uint32_t frame_err_line_spliter_enable_fe1: 1;
	} bits;
};

union reg_isp_top_sw_ctrl_0 {
	uint32_t raw;
	struct {
		uint32_t trig_str_fe0                    : 4;
		uint32_t trig_str_fe1                    : 4;
		uint32_t trig_str_fe2                    : 2;
		uint32_t trig_str_be                     : 2;
		uint32_t trig_str_raw                    : 1;
		uint32_t trig_str_post                   : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t shaw_up_fe0                     : 4;
		uint32_t shaw_up_fe1                     : 4;
		uint32_t shaw_up_fe2                     : 2;
		uint32_t shaw_up_be                      : 2;
		uint32_t shaw_up_raw                     : 1;
		uint32_t shaw_up_post                    : 1;
	} bits;
};

union reg_isp_top_sw_ctrl_1 {
	uint32_t raw;
	struct {
		uint32_t pq_up_fe0                       : 4;
		uint32_t pq_up_fe1                       : 4;
		uint32_t pq_up_fe2                       : 2;
		uint32_t pq_up_be                        : 2;
		uint32_t pq_up_raw                       : 1;
		uint32_t pq_up_post                      : 1;
	} bits;
};

union reg_isp_top_ctrl_mode_sel0 {
	uint32_t raw;
	struct {
		uint32_t trig_str_sel_fe0                : 4;
		uint32_t trig_str_sel_fe1                : 4;
		uint32_t trig_str_sel_fe2                : 2;
		uint32_t trig_str_sel_be                 : 2;
		uint32_t trig_str_sel_raw                : 1;
		uint32_t trig_str_sel_post               : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t shaw_up_sel_fe0                 : 4;
		uint32_t shaw_up_sel_fe1                 : 4;
		uint32_t shaw_up_sel_fe2                 : 2;
		uint32_t shaw_up_sel_be                  : 2;
		uint32_t shaw_up_sel_raw                 : 1;
		uint32_t shaw_up_sel_post                : 1;
	} bits;
};

union reg_isp_top_ctrl_mode_sel1 {
	uint32_t raw;
	struct {
		uint32_t pq_up_sel_fe0                   : 4;
		uint32_t pq_up_sel_fe1                   : 4;
		uint32_t pq_up_sel_fe2                   : 2;
		uint32_t pq_up_sel_be                    : 2;
		uint32_t pq_up_sel_raw                   : 1;
		uint32_t pq_up_sel_post                  : 1;
	} bits;
};

union reg_isp_top_scenarios_ctrl {
	uint32_t raw;
	struct {
		uint32_t dst2sc                          : 1;
		uint32_t dst2dma                         : 1;
		uint32_t pre2be_l_enable                 : 1;
		uint32_t pre2be_s_enable                 : 1;
		uint32_t pre2yuv_422_enable              : 1;
		uint32_t be2raw_l_enable                 : 1;
		uint32_t be2raw_s_enable                 : 1;
		uint32_t be_rdma_l_enable                : 1;
		uint32_t be_rdma_s_enable                : 1;
		uint32_t be_wdma_l_enable                : 1;
		uint32_t be_wdma_s_enable                : 1;
		uint32_t _rsv_11                         : 2;
		uint32_t af_raw0yuv1                     : 1;
		uint32_t rgbmp_online_l_enable           : 1;
		uint32_t rgbmp_online_s_enable           : 1;
		uint32_t raw2yuv_422_enable              : 1;
		uint32_t hdr_enable                      : 1;
		uint32_t hw_auto_enable                  : 1;
		uint32_t hw_auto_iso                     : 2;
		uint32_t dci_rgb0yuv1                    : 1;
		uint32_t fe_dma_share_enable             : 1;
		uint32_t fe2_dma_share_enable            : 1;
		uint32_t be_src_sel                      : 3;
	} bits;
};

union reg_isp_top_sw_rst {
	uint32_t raw;
	struct {
		uint32_t isp_rst                         : 1;
		uint32_t csi0_rst                        : 1;
		uint32_t csi1_rst                        : 1;
		uint32_t csi_be_rst                      : 1;
		uint32_t csi2_rst                        : 1;
		uint32_t bdg0_lite_rst                   : 1;
		uint32_t bdg1_lite_rst                   : 1;
		uint32_t axi_rst                         : 1;
		uint32_t cmdq_rst                        : 1;
		uint32_t apb_rst                         : 1;
		uint32_t raw_rst                         : 1;
	} bits;
};

union reg_isp_top_blk_idle {
	uint32_t raw;
	struct {
		uint32_t fe0_blk_idle                    : 1;
		uint32_t fe1_blk_idle                    : 1;
		uint32_t fe2_blk_idle                    : 1;
		uint32_t be_blk_idle                     : 1;
		uint32_t raw_blk_idle                    : 1;
		uint32_t rgb_blk_idle                    : 1;
		uint32_t yuv_blk_idle                    : 1;
		uint32_t rdma0_idle                      : 1;
		uint32_t wdma0_idle                      : 1;
		uint32_t wdma1_idle                      : 1;
	} bits;
};

union reg_isp_top_blk_idle_enable {
	uint32_t raw;
	struct {
		uint32_t blk_idle_csi0_en                : 1;
		uint32_t blk_idle_csi1_en                : 1;
		uint32_t blk_idle_csi2_en                : 1;
		uint32_t blk_idle_bdg0_lite_en           : 1;
		uint32_t blk_idle_bdg1_lite_en           : 1;
		uint32_t blk_idle_be_en                  : 1;
		uint32_t blk_idle_post_en                : 1;
		uint32_t blk_idle_apb_en                 : 1;
		uint32_t blk_idle_axi_en                 : 1;
		uint32_t blk_idle_cmdq_en                : 1;
		uint32_t blk_idle_raw_en                 : 1;
		uint32_t blk_idle_rgb_en                 : 1;
		uint32_t blk_idle_yuv_en                 : 1;
	} bits;
};

union reg_isp_top_dbus0 {
	uint32_t raw;
	struct {
		uint32_t dbus0                           : 32;
	} bits;
};

union reg_isp_top_dbus1 {
	uint32_t raw;
	struct {
		uint32_t dbus1                           : 32;
	} bits;
};

union reg_isp_top_dbus2 {
	uint32_t raw;
	struct {
		uint32_t dbus2                           : 32;
	} bits;
};

union reg_isp_top_dbus3 {
	uint32_t raw;
	struct {
		uint32_t dbus3                           : 32;
	} bits;
};

union reg_isp_top_force_int {
	uint32_t raw;
	struct {
		uint32_t force_isp_int                   : 1;
		uint32_t force_isp_int_en                : 1;
	} bits;
};

union reg_isp_top_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy                           : 28;
		uint32_t dbus_sel                        : 4;
	} bits;
};

union reg_isp_top_ip_enable0 {
	uint32_t raw;
	struct {
		uint32_t fe0_rgbmap_l_enable             : 1;
		uint32_t fe0_rgbmap_s_enable             : 1;
		uint32_t fe0_blc_l_enable                : 1;
		uint32_t fe0_blc_s_enable                : 1;
		uint32_t fe1_rgbmap_l_enable             : 1;
		uint32_t fe1_rgbmap_s_enable             : 1;
		uint32_t fe1_blc_l_enable                : 1;
		uint32_t fe1_blc_s_enable                : 1;
		uint32_t fe2_rgbmap_l_enable             : 1;
		uint32_t fe2_rgbmap_s_enable             : 1;
		uint32_t fe2_blc_l_enable                : 1;
		uint32_t fe2_blc_s_enable                : 1;
		uint32_t _rsv_12                         : 4;
		uint32_t be_blc_l_enable                 : 1;
		uint32_t be_blc_s_enable                 : 1;
		uint32_t be_crop_l_enable                : 1;
		uint32_t be_crop_s_enable                : 1;
		uint32_t be_rgbir_l_enable               : 1;
		uint32_t be_rgbir_s_enable               : 1;
		uint32_t be_dpc_l_enable                 : 1;
		uint32_t be_dpc_s_enable                 : 1;
		uint32_t be_af_enable                    : 1;
	} bits;
};

union reg_isp_top_ip_enable1 {
	uint32_t raw;
	struct {
		uint32_t raw_crop_l_enable               : 1;
		uint32_t raw_crop_s_enable               : 1;
		uint32_t raw_bnr_l_enable                : 1;
		uint32_t raw_bnr_s_enable                : 1;
		uint32_t raw_cfa_l_enable                : 1;
		uint32_t raw_cfa_s_enable                : 1;
		uint32_t raw_lscm_l_enable               : 1;
		uint32_t raw_lscm_s_enable               : 1;
		uint32_t raw_wbg_l_enable                : 1;
		uint32_t raw_wbg_s_enable                : 1;
		uint32_t raw_lmp_l_enable                : 1;
		uint32_t raw_lmp_s_enable                : 1;
		uint32_t raw_ae_l_enable                 : 1;
		uint32_t raw_ae_s_enable                 : 1;
		uint32_t raw_gms_enable                  : 1;
		uint32_t raw_rgbcac_l_enable             : 1;
		uint32_t raw_rgbcac_s_enable             : 1;
		uint32_t raw_lcac_l_enable               : 1;
		uint32_t raw_lcac_s_enable               : 1;
		uint32_t rgb_fusion_enable               : 1;
		uint32_t rgb_ltm_enable                  : 1;
		uint32_t rgb_manr_enable                 : 1;
		uint32_t rgb_histv_enable                : 1;
		uint32_t rgb_gamma_enable                : 1;
		uint32_t rgb_dhz_enable                  : 1;
		uint32_t rgb_rgbdither_enable            : 1;
		uint32_t rgb_clut_enable                 : 1;
		uint32_t rgb_r2y4_enable                 : 1;
		uint32_t rgb_user_gamma_enable           : 1;
		uint32_t rgb_ccm_0_enable                : 1;
		uint32_t rgb_ccm_1_enable                : 1;
		uint32_t rgb_fusion_csc_enable           : 1;
	} bits;
};

union reg_isp_top_ip_enable2 {
	uint32_t raw;
	struct {
		uint32_t yuv_preyee_enable               : 1;
		uint32_t yuv_dither_enable               : 1;
		uint32_t yuv_3dnr_enable                 : 1;
		uint32_t yuv_ynr_enable                  : 1;
		uint32_t yuv_cnr_enable                  : 1;
		uint32_t yuv_ee_enable                   : 1;
		uint32_t yuv_crop_y_enable               : 1;
		uint32_t yuv_crop_c_enable               : 1;
		uint32_t yuv_ycurve_enable               : 1;
		uint32_t yuv_ca2_enable                  : 1;
		uint32_t yuv_ca_enable                   : 1;
		uint32_t yuv_dci_enable                  : 1;
		uint32_t yuv_ldci_enable                 : 1;
	} bits;
};

union reg_isp_top_cmdq_ctrl {
	uint32_t raw;
	struct {
		uint32_t cmdq_tsk_en                     : 8;
		uint32_t cmdq_flag_sel                   : 2;
		uint32_t cmdq_task_sel                   : 2;
	} bits;
};

union reg_isp_top_cmdq_trig {
	uint32_t raw;
	struct {
		uint32_t cmdq_tsk_trig                   : 8;
	} bits;
};

union reg_isp_top_trig_cnt {
	uint32_t raw;
	struct {
		uint32_t trig_str_cnt                    : 4;
		uint32_t vsync_delay                     : 4;
	} bits;
};

union reg_isp_top_svn_version {
	uint32_t raw;
	struct {
		uint32_t svn_revision                    : 32;
	} bits;
};

union reg_isp_top_timestamp {
	uint32_t raw;
	struct {
		uint32_t unix_timestamp                  : 32;
	} bits;
};

union reg_isp_top_sclie_enable {
	uint32_t raw;
	struct {
		uint32_t slice_enable_main_lexp          : 1;
		uint32_t slice_enable_main_sexp          : 1;
		uint32_t slice_enable_sub_lexp           : 1;
		uint32_t slice_enable_sub_sexp           : 1;
	} bits;
};

union reg_isp_top_w_slice_thresh_main {
	uint32_t raw;
	struct {
		uint32_t w_slice_thr_main_lexp           : 16;
		uint32_t w_slice_thr_main_sexp           : 16;
	} bits;
};

union reg_isp_top_w_slice_thresh_sub_curr {
	uint32_t raw;
	struct {
		uint32_t w_slice_thr_sub_cur_lexp        : 16;
		uint32_t w_slice_thr_sub_cur_sexp        : 16;
	} bits;
};

union reg_isp_top_w_slice_thresh_sub_prv {
	uint32_t raw;
	struct {
		uint32_t w_slice_thr_sub_prv_lexp        : 16;
		uint32_t w_slice_thr_sub_prv_sexp        : 16;
	} bits;
};

union reg_isp_top_r_slice_thresh_main {
	uint32_t raw;
	struct {
		uint32_t r_slice_thr_main_lexp           : 16;
		uint32_t r_slice_thr_main_sexp           : 16;
	} bits;
};

union reg_isp_top_r_slice_thresh_sub_curr {
	uint32_t raw;
	struct {
		uint32_t r_slice_thr_sub_cur_lexp        : 16;
		uint32_t r_slice_thr_sub_cur_sexp        : 16;
	} bits;
};

union reg_isp_top_r_slice_thresh_sub_prv {
	uint32_t raw;
	struct {
		uint32_t r_slice_thr_sub_prv_lexp        : 16;
		uint32_t r_slice_thr_sub_prv_sexp        : 16;
	} bits;
};

union reg_isp_top_raw_frame_valid {
	uint32_t raw;
	struct {
		uint32_t raw_frame_vld                   : 1;
		uint32_t raw_pq_vld                      : 1;
	} bits;
};

union reg_isp_top_first_frame {
	uint32_t raw;
	struct {
		uint32_t first_frame_sw                  : 5;
		uint32_t first_frame_top                 : 1;
	} bits;
};

union reg_isp_top_int_event0_fe345 {
	uint32_t raw;
	struct {
		uint32_t frame_done_fe3                  : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t frame_done_fe4                  : 2;
		uint32_t frame_done_fe5                  : 2;
		uint32_t _rsv_8                          : 8;
		uint32_t shaw_done_fe3                   : 2;
		uint32_t _rsv_18                         : 2;
		uint32_t shaw_done_fe4                   : 2;
		uint32_t shaw_done_fe5                   : 2;
		uint32_t line_spliter_dma_done_fe0       : 4;
		uint32_t line_spliter_dma_done_fe1       : 4;
	} bits;
};

union reg_isp_top_int_event1_fe345 {
	uint32_t raw;
	struct {
		uint32_t pq_done_fe3                     : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t pq_done_fe4                     : 2;
		uint32_t pq_done_fe5                     : 2;
	} bits;
};

union reg_isp_top_int_event2_fe345 {
	uint32_t raw;
	struct {
		uint32_t frame_start_fe3                 : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t frame_start_fe4                 : 2;
		uint32_t frame_start_fe5                 : 2;
		uint32_t _rsv_8                          : 3;
		uint32_t line_intp_fe3                   : 1;
		uint32_t line_intp_fe4                   : 1;
		uint32_t line_intp_fe5                   : 1;
	} bits;
};

union reg_isp_top_error_sts_fe345 {
	uint32_t raw;
	struct {
		uint32_t pchk0_err_fe3                   : 1;
		uint32_t pchk0_err_fe4                   : 1;
		uint32_t pchk0_err_fe5                   : 1;
		uint32_t _rsv_3                          : 4;
		uint32_t pchk1_err_fe3                   : 1;
		uint32_t pchk1_err_fe4                   : 1;
		uint32_t pchk1_err_fe5                   : 1;
	} bits;
};

union reg_isp_top_int_event0_en_fe345 {
	uint32_t raw;
	struct {
		uint32_t frame_done_enable_fe3           : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t frame_done_enable_fe4           : 2;
		uint32_t frame_done_enable_fe5           : 2;
		uint32_t _rsv_8                          : 8;
		uint32_t shaw_done_enable_fe3            : 2;
		uint32_t _rsv_18                         : 2;
		uint32_t shaw_done_enable_fe4            : 2;
		uint32_t shaw_done_enable_fe5            : 2;
		uint32_t line_spliter_dma_done_enable_fe0: 4;
		uint32_t line_spliter_dma_done_enable_fe1: 4;
	} bits;
};

union reg_isp_top_int_event1_en_fe345 {
	uint32_t raw;
	struct {
		uint32_t pq_done_enable_fe3              : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t pq_done_enable_fe4              : 2;
		uint32_t pq_done_enable_fe5              : 2;
	} bits;
};

union reg_isp_top_int_event2_en_fe345 {
	uint32_t raw;
	struct {
		uint32_t frame_start_enable_fe3          : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t frame_start_enable_fe4          : 2;
		uint32_t frame_start_enable_fe5          : 2;
		uint32_t _rsv_8                          : 3;
		uint32_t line_intp_enable_fe3            : 1;
		uint32_t line_intp_enable_fe4            : 1;
		uint32_t line_intp_enable_fe5            : 1;
	} bits;
};

union reg_isp_top_sw_ctrl_0_fe345 {
	uint32_t raw;
	struct {
		uint32_t trig_str_fe3                    : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t trig_str_fe4                    : 2;
		uint32_t trig_str_fe5                    : 2;
		uint32_t _rsv_8                          : 8;
		uint32_t shaw_up_fe3                     : 2;
		uint32_t _rsv_18                         : 2;
		uint32_t shaw_up_fe4                     : 2;
		uint32_t shaw_up_fe5                     : 2;
	} bits;
};

union reg_isp_top_sw_ctrl_1_fe345 {
	uint32_t raw;
	struct {
		uint32_t pq_up_fe3                       : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t pq_up_fe4                       : 2;
		uint32_t pq_up_fe5                       : 2;
	} bits;
};

union reg_isp_top_ctrl_mode_sel0_fe345 {
	uint32_t raw;
	struct {
		uint32_t trig_str_sel_fe3                : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t trig_str_sel_fe4                : 2;
		uint32_t trig_str_sel_fe5                : 2;
		uint32_t _rsv_8                          : 8;
		uint32_t shaw_up_sel_fe3                 : 2;
		uint32_t _rsv_18                         : 2;
		uint32_t shaw_up_sel_fe4                 : 2;
		uint32_t shaw_up_sel_fe5                 : 2;
	} bits;
};

union reg_isp_top_ctrl_mode_sel1_fe345 {
	uint32_t raw;
	struct {
		uint32_t pq_up_sel_fe3                   : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t pq_up_sel_fe4                   : 2;
		uint32_t pq_up_sel_fe5                   : 2;
	} bits;
};

union reg_isp_top_sw_rst_fe345 {
	uint32_t raw;
	struct {
		uint32_t csi3_rst                        : 1;
		uint32_t csi4_rst                        : 1;
		uint32_t csi5_rst                        : 1;
	} bits;
};

union reg_isp_top_blk_idle_1 {
	uint32_t raw;
	struct {
		uint32_t fe3_blk_idle                    : 1;
		uint32_t fe4_blk_idle                    : 1;
		uint32_t fe5_blk_idle                    : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t rdma1_idle                      : 1;
		uint32_t _rsv_5                          : 2;
		uint32_t wdma2_idle                      : 1;
		uint32_t wdma3_idle                      : 1;
		uint32_t line_spliter_fe0_dma_idle       : 4;
		uint32_t line_spliter_fe1_dma_idle       : 4;
		uint32_t line_spliter_fe0_fifo_overflow_status: 1;
		uint32_t line_spliter_fe1_fifo_overflow_status: 1;
	} bits;
};

union reg_isp_top_blk_idle_enable_fe345 {
	uint32_t raw;
	struct {
		uint32_t blk_idle_csi3_en                : 1;
		uint32_t blk_idle_csi4_en                : 1;
		uint32_t blk_idle_csi5_en                : 1;
	} bits;
};

union reg_isp_top_ip_enable_fe345 {
	uint32_t raw;
	struct {
		uint32_t fe3_rgbmap_l_enable             : 1;
		uint32_t fe3_rgbmap_s_enable             : 1;
		uint32_t fe3_blc_l_enable                : 1;
		uint32_t fe3_blc_s_enable                : 1;
		uint32_t fe4_rgbmap_l_enable             : 1;
		uint32_t fe4_rgbmap_s_enable             : 1;
		uint32_t fe4_blc_l_enable                : 1;
		uint32_t fe4_blc_s_enable                : 1;
		uint32_t fe5_rgbmap_l_enable             : 1;
		uint32_t fe5_rgbmap_s_enable             : 1;
		uint32_t fe5_blc_l_enable                : 1;
		uint32_t fe5_blc_s_enable                : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_wdma_core_shadow_rd_sel   {
	uint32_t raw;
	struct {
		uint32_t shadow_rd_sel                   : 1;
		uint32_t abort_mode                      : 1;
	} bits;
};

union reg_wdma_core_ip_disable {
	uint32_t raw;
	struct {
		uint32_t ip_disable                      : 32;
	} bits;
};

union reg_wdma_core_disable_seglen {
	uint32_t raw;
	struct {
		uint32_t seglen_disable                  : 32;
	} bits;
};

union reg_wdma_core_up_ring_base {
	uint32_t raw;
	struct {
		uint32_t up_ring_base                    : 32;
	} bits;
};

union reg_wdma_core_norm_status0 {
	uint32_t raw;
	struct {
		uint32_t abort_done                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t error_axi                       : 1;
		uint32_t error_dmi                       : 1;
		uint32_t slot_full                       : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t error_id                        : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t dma_version                     : 16;
	} bits;
};

union reg_wdma_core_norm_status1 {
	uint32_t raw;
	struct {
		uint32_t id_idle                         : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_en {
	uint32_t raw;
	struct {
		uint32_t ring_enable                     : 32;
	} bits;
};

union reg_wdma_core_norm_perf  {
	uint32_t raw;
	struct {
		uint32_t bwlwin                          : 10;
		uint32_t bwltxn                          : 6;
		uint32_t qoso_th                         : 4;
		uint32_t qoso_en                         : 1;
	} bits;
};

union reg_wdma_core_ring_patch_enable {
	uint32_t raw;
	struct {
		uint32_t ring_patch_enable               : 32;
	} bits;
};

union reg_wdma_core_set_ring_base {
	uint32_t raw;
	struct {
		uint32_t set_ring_base                   : 32;
	} bits;
};

union reg_wdma_core_ring_base_addr_l {
	uint32_t raw;
	struct {
		uint32_t ring_base_l                     : 32;
	} bits;
};

union reg_wdma_core_ring_base_addr_h {
	uint32_t raw;
	struct {
		uint32_t ring_base_h                     : 8;
	} bits;
};

union reg_wdma_core_ring_buffer_size0 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size0                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size1 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size1                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size2 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size2                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size3 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size3                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size4 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size4                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size5 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size5                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size6 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size6                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size7 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size7                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size8 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size8                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size9 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size9                      : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size10 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size10                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size11 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size11                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size12 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size12                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size13 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size13                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size14 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size14                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size15 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size15                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size16 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size16                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size17 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size17                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size18 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size18                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size19 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size19                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size20 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size20                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size21 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size21                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size22 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size22                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size23 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size23                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size24 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size24                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size25 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size25                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size26 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size26                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size27 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size27                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size28 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size28                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size29 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size29                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size30 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size30                     : 32;
	} bits;
};

union reg_wdma_core_ring_buffer_size31 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size31                     : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts0 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr0                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts1 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr1                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts2 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr2                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts3 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr3                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts4 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr4                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts5 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr5                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts6 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr6                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts7 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr7                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts8 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr8                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts9 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr9                  : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts10 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr10                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts11 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr11                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts12 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr12                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts13 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr13                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts14 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr14                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts15 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr15                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts16 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr16                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts17 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr17                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts18 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr18                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts19 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr19                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts20 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr20                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts21 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr21                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts22 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr22                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts23 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr23                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts24 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr24                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts25 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr25                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts26 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr26                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts27 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr27                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts28 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr28                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts29 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr29                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts30 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr30                 : 32;
	} bits;
};

union reg_wdma_core_next_dma_addr_sts31 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr31                 : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_rdma_core_shadow_rd_sel   {
	uint32_t raw;
	struct {
		uint32_t shadow_rd_sel                   : 1;
		uint32_t abort_mode                      : 1;
		uint32_t _rsv_2                          : 6;
		uint32_t max_ostd                        : 8;
		uint32_t ostd_sw_en                      : 1;
	} bits;
};

union reg_rdma_core_ip_disable {
	uint32_t raw;
	struct {
		uint32_t ip_disable                      : 32;
	} bits;
};

union reg_rdma_core_up_ring_base {
	uint32_t raw;
	struct {
		uint32_t up_ring_base                    : 32;
	} bits;
};

union reg_rdma_core_norm_status0 {
	uint32_t raw;
	struct {
		uint32_t abort_done                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t error_axi                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t error_id                        : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t dma_version                     : 16;
	} bits;
};

union reg_rdma_core_norm_status1 {
	uint32_t raw;
	struct {
		uint32_t id_idle                         : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_en {
	uint32_t raw;
	struct {
		uint32_t ring_enable                     : 32;
	} bits;
};

union reg_rdma_core_norm_perf  {
	uint32_t raw;
	struct {
		uint32_t bwlwin                          : 10;
		uint32_t bwltxn                          : 6;
	} bits;
};

union reg_rdma_core_ar_priority_sel {
	uint32_t raw;
	struct {
		uint32_t ar_priority_sel                 : 1;
		uint32_t qos_priority_sel                : 1;
		uint32_t arb_hist_disable                : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t arb_usage_th                    : 4;
	} bits;
};

union reg_rdma_core_ring_patch_enable {
	uint32_t raw;
	struct {
		uint32_t ring_patch_enable               : 32;
	} bits;
};

union reg_rdma_core_set_ring_base {
	uint32_t raw;
	struct {
		uint32_t set_ring_base                   : 32;
	} bits;
};

union reg_rdma_core_ring_base_addr_l {
	uint32_t raw;
	struct {
		uint32_t ring_base_l                     : 32;
	} bits;
};

union reg_rdma_core_ring_base_addr_h {
	uint32_t raw;
	struct {
		uint32_t ring_base_h                     : 8;
	} bits;
};

union reg_rdma_core_ring_buffer_size0 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size0                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size1 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size1                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size2 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size2                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size3 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size3                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size4 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size4                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size5 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size5                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size6 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size6                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size7 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size7                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size8 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size8                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size9 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size9                      : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size10 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size10                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size11 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size11                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size12 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size12                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size13 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size13                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size14 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size14                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size15 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size15                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size16 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size16                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size17 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size17                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size18 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size18                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size19 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size19                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size20 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size20                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size21 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size21                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size22 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size22                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size23 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size23                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size24 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size24                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size25 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size25                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size26 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size26                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size27 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size27                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size28 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size28                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size29 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size29                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size30 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size30                     : 32;
	} bits;
};

union reg_rdma_core_ring_buffer_size31 {
	uint32_t raw;
	struct {
		uint32_t rbuf_size31                     : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts0 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr0                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts1 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr1                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts2 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr2                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts3 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr3                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts4 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr4                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts5 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr5                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts6 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr6                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts7 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr7                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts8 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr8                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts9 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr9                  : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts10 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr10                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts11 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr11                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts12 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr12                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts13 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr13                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts14 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr14                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts15 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr15                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts16 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr16                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts17 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr17                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts18 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr18                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts19 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr19                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts20 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr20                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts21 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr21                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts22 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr22                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts23 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr23                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts24 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr24                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts25 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr25                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts26 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr26                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts27 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr27                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts28 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr28                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts29 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr29                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts30 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr30                 : 32;
	} bits;
};

union reg_rdma_core_next_dma_addr_sts31 {
	uint32_t raw;
	struct {
		uint32_t next_dma_addr31                 : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_csi_bdg_lite_bdg_top_ctrl {
	uint32_t raw;
	struct {
		uint32_t csi_mode                        : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t ch_num                          : 2;
		uint32_t ch0_dma_wr_enable               : 1;
		uint32_t ch1_dma_wr_enable               : 1;
		uint32_t ch2_dma_wr_enable               : 1;
		uint32_t y_only                          : 1;
		uint32_t _rsv_10                         : 1;
		uint32_t vs_pol                          : 1;
		uint32_t hs_pol                          : 1;
		uint32_t reset_mode                      : 1;
		uint32_t vs_mode                         : 1;
		uint32_t abort_mode                      : 1;
		uint32_t reset                           : 1;
		uint32_t abort                           : 1;
		uint32_t ch3_dma_wr_enable               : 1;
		uint32_t ch3_dma_420_wr_enable           : 1;
		uint32_t _rsv_20                         : 2;
		uint32_t ch0_dma_420_wr_enable           : 1;
		uint32_t ch1_dma_420_wr_enable           : 1;
		uint32_t mcsi_enable                      : 1;
		uint32_t _rsv_25                         : 2;
		uint32_t ch2_dma_420_wr_enable           : 1;
		uint32_t shdw_read_sel                   : 1;
		uint32_t _rsv_29                         : 2;
		uint32_t csi_up_reg                      : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_bdg_interrupt_ctrl_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_vs_int_en                   : 1;
		uint32_t ch0_trig_int_en                 : 1;
		uint32_t ch0_drop_int_en                 : 1;
		uint32_t ch0_size_error_int_en           : 1;
		uint32_t ch0_frame_done_en               : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t ch1_vs_int_en                   : 1;
		uint32_t ch1_trig_int_en                 : 1;
		uint32_t ch1_drop_int_en                 : 1;
		uint32_t ch1_size_error_int_en           : 1;
		uint32_t ch1_frame_done_en               : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t ch2_vs_int_en                   : 1;
		uint32_t ch2_trig_int_en                 : 1;
		uint32_t ch2_drop_int_en                 : 1;
		uint32_t ch2_size_error_int_en           : 1;
		uint32_t ch2_frame_done_en               : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t ch3_vs_int_en                   : 1;
		uint32_t ch3_trig_int_en                 : 1;
		uint32_t ch3_drop_int_en                 : 1;
		uint32_t ch3_size_error_int_en           : 1;
		uint32_t ch3_frame_done_en               : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_bdg_interrupt_ctrl_1 {
	uint32_t raw;
	struct {
		uint32_t line_intp_en                    : 1;
		uint32_t fifo_overflow_int_en            : 1;
		uint32_t dma_error_intp_en               : 1;
		uint32_t _rsv_3                          : 5;
		uint32_t drop_mode                       : 4;
		uint32_t avg_mode                        : 4;
	} bits;
};

union reg_isp_csi_bdg_lite_frame_vld {
	uint32_t raw;
	struct {
		uint32_t frame_vld_ch0                   : 1;
		uint32_t frame_vld_ch1                   : 1;
		uint32_t frame_vld_ch2                   : 1;
		uint32_t frame_vld_ch3                   : 1;
		uint32_t e_vld_clr_ch0                   : 1;
		uint32_t e_vld_clr_ch1                   : 1;
		uint32_t e_vld_clr_ch2                   : 1;
		uint32_t e_vld_clr_ch3                   : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_ch0_size {
	uint32_t raw;
	struct {
		uint32_t ch0_frame_widthm1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch0_frame_heightm1              : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch1_size {
	uint32_t raw;
	struct {
		uint32_t ch1_frame_widthm1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch1_frame_heightm1              : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch2_size {
	uint32_t raw;
	struct {
		uint32_t ch2_frame_widthm1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch2_frame_heightm1              : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch3_size {
	uint32_t raw;
	struct {
		uint32_t ch3_frame_widthm1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch3_frame_heightm1              : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch0_crop_en {
	uint32_t raw;
	struct {
		uint32_t ch0_crop_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_ch0_horz_crop {
	uint32_t raw;
	struct {
		uint32_t ch0_horz_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch0_horz_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch0_vert_crop {
	uint32_t raw;
	struct {
		uint32_t ch0_vert_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch0_vert_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch1_crop_en {
	uint32_t raw;
	struct {
		uint32_t ch1_crop_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_ch1_horz_crop {
	uint32_t raw;
	struct {
		uint32_t ch1_horz_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch1_horz_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch1_vert_crop {
	uint32_t raw;
	struct {
		uint32_t ch1_vert_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch1_vert_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch2_crop_en {
	uint32_t raw;
	struct {
		uint32_t ch2_crop_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_ch2_horz_crop {
	uint32_t raw;
	struct {
		uint32_t ch2_horz_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch2_horz_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch2_vert_crop {
	uint32_t raw;
	struct {
		uint32_t ch2_vert_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch2_vert_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch3_crop_en {
	uint32_t raw;
	struct {
		uint32_t ch3_crop_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_ch3_horz_crop {
	uint32_t raw;
	struct {
		uint32_t ch3_horz_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch3_horz_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch3_vert_crop {
	uint32_t raw;
	struct {
		uint32_t ch3_vert_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch3_vert_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_line_intp_height {
	uint32_t raw;
	struct {
		uint32_t line_intp_heightm1              : 14;
	} bits;
};

union reg_isp_csi_bdg_lite_ch0_debug_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_pxl_cnt                     : 32;
	} bits;
};

union reg_isp_csi_bdg_lite_ch0_debug_1 {
	uint32_t raw;
	struct {
		uint32_t ch0_line_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_lite_ch0_debug_2 {
	uint32_t raw;
	struct {
		uint32_t ch0_vs_cnt                      : 16;
		uint32_t ch0_trig_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_lite_ch0_debug_3 {
	uint32_t raw;
	struct {
		uint32_t ch0_tot_blk_idle                : 1;
		uint32_t ch0_tot_dma_idle                : 1;
		uint32_t ch0_bdg_dma_idle                : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_ch1_debug_0 {
	uint32_t raw;
	struct {
		uint32_t ch1_pxl_cnt                     : 32;
	} bits;
};

union reg_isp_csi_bdg_lite_ch1_debug_1 {
	uint32_t raw;
	struct {
		uint32_t ch1_line_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_lite_ch1_debug_2 {
	uint32_t raw;
	struct {
		uint32_t ch1_vs_cnt                      : 16;
		uint32_t ch1_trig_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_lite_ch1_debug_3 {
	uint32_t raw;
	struct {
		uint32_t ch1_tot_blk_idle                : 1;
		uint32_t ch1_tot_dma_idle                : 1;
		uint32_t ch1_bdg_dma_idle                : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_ch2_debug_0 {
	uint32_t raw;
	struct {
		uint32_t ch2_pxl_cnt                     : 32;
	} bits;
};

union reg_isp_csi_bdg_lite_ch2_debug_1 {
	uint32_t raw;
	struct {
		uint32_t ch2_line_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_lite_ch2_debug_2 {
	uint32_t raw;
	struct {
		uint32_t ch2_vs_cnt                      : 16;
		uint32_t ch2_trig_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_lite_ch2_debug_3 {
	uint32_t raw;
	struct {
		uint32_t ch2_tot_blk_idle                : 1;
		uint32_t ch2_tot_dma_idle                : 1;
		uint32_t ch2_bdg_dma_idle                : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_ch3_debug_0 {
	uint32_t raw;
	struct {
		uint32_t ch3_pxl_cnt                     : 32;
	} bits;
};

union reg_isp_csi_bdg_lite_ch3_debug_1 {
	uint32_t raw;
	struct {
		uint32_t ch3_line_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_lite_ch3_debug_2 {
	uint32_t raw;
	struct {
		uint32_t ch3_vs_cnt                      : 16;
		uint32_t ch3_trig_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_lite_ch3_debug_3 {
	uint32_t raw;
	struct {
		uint32_t ch3_tot_blk_idle                : 1;
		uint32_t ch3_tot_dma_idle                : 1;
		uint32_t ch3_bdg_dma_idle                : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_interrupt_status_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_frame_drop_int              : 1;
		uint32_t ch0_vs_int                      : 1;
		uint32_t ch0_trig_int                    : 1;
		uint32_t ch0_frame_done                  : 1;
		uint32_t ch0_frame_width_gt_int          : 1;
		uint32_t ch0_frame_width_ls_int          : 1;
		uint32_t ch0_frame_height_gt_int         : 1;
		uint32_t ch0_frame_height_ls_int         : 1;
		uint32_t ch1_frame_drop_int              : 1;
		uint32_t ch1_vs_int                      : 1;
		uint32_t ch1_trig_int                    : 1;
		uint32_t ch1_frame_done                  : 1;
		uint32_t ch1_frame_width_gt_int          : 1;
		uint32_t ch1_frame_width_ls_int          : 1;
		uint32_t ch1_frame_height_gt_int         : 1;
		uint32_t ch1_frame_height_ls_int         : 1;
		uint32_t ch2_frame_drop_int              : 1;
		uint32_t ch2_vs_int                      : 1;
		uint32_t ch2_trig_int                    : 1;
		uint32_t ch2_frame_done                  : 1;
		uint32_t ch2_frame_width_gt_int          : 1;
		uint32_t ch2_frame_width_ls_int          : 1;
		uint32_t ch2_frame_height_gt_int         : 1;
		uint32_t ch2_frame_height_ls_int         : 1;
		uint32_t ch3_frame_drop_int              : 1;
		uint32_t ch3_vs_int                      : 1;
		uint32_t ch3_trig_int                    : 1;
		uint32_t ch3_frame_done                  : 1;
		uint32_t ch3_frame_width_gt_int          : 1;
		uint32_t ch3_frame_width_ls_int          : 1;
		uint32_t ch3_frame_height_gt_int         : 1;
		uint32_t ch3_frame_height_ls_int         : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_interrupt_status_1 {
	uint32_t raw;
	struct {
		uint32_t fifo_overflow_int               : 1;
		uint32_t frame_resolution_over_max_int   : 1;
		uint32_t _rsv_2                          : 1;
		uint32_t line_intp_int                   : 1;
		uint32_t dma_error_int                   : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_bdg_debug {
	uint32_t raw;
	struct {
		uint32_t ring_buff_idle                  : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_wr_urgent_ctrl {
	uint32_t raw;
	struct {
		uint32_t wr_near_overflow_threshold      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t wr_safe_threshold               : 13;
	} bits;
};

union reg_isp_csi_bdg_lite_rd_urgent_ctrl {
	uint32_t raw;
	struct {
		uint32_t rd_near_overflow_threshold      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t rd_safe_threshold               : 13;
	} bits;
};

union reg_isp_csi_bdg_lite_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy_in                        : 16;
		uint32_t dummy_out                       : 16;
	} bits;
};

union reg_isp_csi_bdg_lite_trig_dly_control_0 {
	uint32_t raw;
	struct {
		uint32_t trig_dly_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_lite_trig_dly_control_1 {
	uint32_t raw;
	struct {
		uint32_t trig_dly_value                  : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_raw_rdma_ctrl_read_sel {
	uint32_t raw;
	struct {
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_raw_rdma_ctrl_config {
	uint32_t raw;
	struct {
		uint32_t le_rdma_en                      : 1;
		uint32_t se_rdma_en                      : 1;
	} bits;
};

union reg_raw_rdma_ctrl_rdma_size {
	uint32_t raw;
	struct {
		uint32_t rdmi_widthm1                    : 16;
		uint32_t rdmi_heightm1                   : 16;
	} bits;
};

union reg_raw_rdma_ctrl_dpcm_mode {
	uint32_t raw;
	struct {
		uint32_t dpcm_mode                       : 3;
		uint32_t _rsv_3                          : 5;
		uint32_t dpcm_xstr                       : 13;
	} bits;
};

union reg_raw_rdma_ctrl_rdma_enable {
	uint32_t raw;
	struct {
		uint32_t raw_rdma_rproc_enable           : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t raw_rdma_crop_enable            : 1;
	} bits;
};

union reg_raw_rdma_ctrl_rdma_crop {
	uint32_t raw;
	struct {
		uint32_t raw_rdma_crop_img_width         : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t raw_rdma_crop_img_height        : 14;
	} bits;
};

union reg_raw_rdma_ctrl_rdma_crop_h {
	uint32_t raw;
	struct {
		uint32_t raw_rdma_crop_w_str             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t raw_rdma_crop_w_end             : 14;
	} bits;
};

union reg_raw_rdma_ctrl_rdma_crop_w {
	uint32_t raw;
	struct {
		uint32_t raw_rdma_crop_h_str             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t raw_rdma_crop_h_end             : 14;
	} bits;
};

union reg_raw_rdma_ctrl_pass {
	uint32_t raw;
	struct {
		uint32_t pass_cnt_m1                     : 8;
		uint32_t pass_sel                        : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_ldci_enable {
	uint32_t raw;
	struct {
		uint32_t ldci_enable                     : 1;
		uint32_t ldci_stats_enable               : 1;
		uint32_t ldci_map_enable                 : 1;
		uint32_t ldci_uv_gain_enable             : 1;
		uint32_t _rsv_4                          : 2;
		uint32_t ldci_first_frame_enable         : 1;
		uint32_t ldci_zeroing_enable             : 1;
		uint32_t ldci_image_size_div_by_16x12    : 1;
		uint32_t _rsv_9                          : 19;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_ldci_strength {
	uint32_t raw;
	struct {
		uint32_t ldci_strength                   : 9;
	} bits;
};

union reg_isp_ldci_luma_wgt_max {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_max               : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t ldci_luma_wgt_min               : 8;
	} bits;
};

union reg_isp_ldci_idx_iir_alpha {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_iir_alpha              : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_var_iir_alpha              : 10;
	} bits;
};

union reg_isp_ldci_edge_scale {
	uint32_t raw;
	struct {
		uint32_t ldci_edge_scale                 : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_edge_coring                : 8;
	} bits;
};

union reg_isp_ldci_edge_clamp {
	uint32_t raw;
	struct {
		uint32_t ldci_var_map_max                : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t ldci_var_map_min                : 8;
	} bits;
};

union reg_isp_ldci_idx_filter_norm {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_filter_norm            : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_var_filter_norm            : 9;
	} bits;
};

union reg_isp_ldci_tone_curve_idx_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_idx_00          : 4;
		uint32_t ldci_tone_curve_idx_01          : 4;
		uint32_t ldci_tone_curve_idx_02          : 4;
		uint32_t ldci_tone_curve_idx_03          : 4;
		uint32_t ldci_tone_curve_idx_04          : 4;
		uint32_t ldci_tone_curve_idx_05          : 4;
		uint32_t ldci_tone_curve_idx_06          : 4;
		uint32_t ldci_tone_curve_idx_07          : 4;
	} bits;
};

union reg_isp_ldci_blk_size_x {
	uint32_t raw;
	struct {
		uint32_t ldci_blk_size_x                 : 16;
		uint32_t ldci_blk_size_y                 : 16;
	} bits;
};

union reg_isp_ldci_blk_size_x1 {
	uint32_t raw;
	struct {
		uint32_t ldci_blk_size_x1                : 16;
		uint32_t ldci_blk_size_y1                : 16;
	} bits;
};

union reg_isp_ldci_subblk_size_x {
	uint32_t raw;
	struct {
		uint32_t ldci_subblk_size_x              : 16;
		uint32_t ldci_subblk_size_y              : 16;
	} bits;
};

union reg_isp_ldci_subblk_size_x1 {
	uint32_t raw;
	struct {
		uint32_t ldci_subblk_size_x1             : 16;
		uint32_t ldci_subblk_size_y1             : 16;
	} bits;
};

union reg_isp_ldci_interp_norm_lr {
	uint32_t raw;
	struct {
		uint32_t ldci_interp_norm_lr             : 16;
		uint32_t ldci_interp_norm_ud             : 16;
	} bits;
};

union reg_isp_ldci_sub_interp_norm_lr {
	uint32_t raw;
	struct {
		uint32_t ldci_sub_interp_norm_lr         : 16;
		uint32_t ldci_sub_interp_norm_ud         : 16;
	} bits;
};

union reg_isp_ldci_mean_norm_x {
	uint32_t raw;
	struct {
		uint32_t ldci_mean_norm_x                : 15;
		uint32_t _rsv_15                         : 1;
		uint32_t ldci_mean_norm_y                : 14;
	} bits;
};

union reg_isp_ldci_var_norm_y {
	uint32_t raw;
	struct {
		uint32_t ldci_var_norm_y                 : 14;
	} bits;
};

union reg_isp_ldci_uv_gain_max {
	uint32_t raw;
	struct {
		uint32_t ldci_uv_gain_max                : 7;
		uint32_t _rsv_7                          : 9;
		uint32_t ldci_uv_gain_min                : 7;
	} bits;
};

union reg_isp_ldci_img_widthm1 {
	uint32_t raw;
	struct {
		uint32_t ldci_img_widthm1                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ldci_img_heightm1               : 14;
	} bits;
};

union reg_isp_ldci_status {
	uint32_t raw;
	struct {
		uint32_t ldci_status                     : 32;
	} bits;
};

union reg_isp_ldci_grace_reset {
	uint32_t raw;
	struct {
		uint32_t ldci_grace_reset                : 1;
	} bits;
};

union reg_isp_ldci_monitor {
	uint32_t raw;
	struct {
		uint32_t ldci_monitor                    : 32;
	} bits;
};

union reg_isp_ldci_flow {
	uint32_t raw;
	struct {
		uint32_t ldci_zerodciogram               : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t ldci_shadow_select              : 1;
	} bits;
};

union reg_isp_ldci_monitor_select {
	uint32_t raw;
	struct {
		uint32_t ldci_monitor_select             : 32;
	} bits;
};

union reg_isp_ldci_location {
	uint32_t raw;
	struct {
		uint32_t ldci_location                   : 32;
	} bits;
};

union reg_isp_ldci_debug {
	uint32_t raw;
	struct {
		uint32_t ldci_debug                      : 32;
	} bits;
};

union reg_isp_ldci_dummy {
	uint32_t raw;
	struct {
		uint32_t ldci_dummy                      : 32;
	} bits;
};

union reg_isp_ldci_dmi_enable {
	uint32_t raw;
	struct {
		uint32_t dmi_enable                      : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t dmi_qos                         : 2;
		uint32_t _rsv_6                          : 2;
		uint32_t force_dma_disable               : 2;
	} bits;
};

union reg_isp_ldci_dci_bayer_starting {
	uint32_t raw;
	struct {
		uint32_t dci_bayer_starting              : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t force_bayer_enable              : 1;
	} bits;
};

union reg_isp_ldci_idx_filter_lut_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_filter_lut_00          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ldci_idx_filter_lut_01          : 11;
	} bits;
};

union reg_isp_ldci_idx_filter_lut_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_filter_lut_02          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ldci_idx_filter_lut_03          : 11;
	} bits;
};

union reg_isp_ldci_idx_filter_lut_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_filter_lut_04          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ldci_idx_filter_lut_05          : 11;
	} bits;
};

union reg_isp_ldci_idx_filter_lut_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_filter_lut_06          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ldci_idx_filter_lut_07          : 11;
	} bits;
};

union reg_isp_ldci_idx_filter_lut_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_filter_lut_08          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ldci_idx_filter_lut_09          : 11;
	} bits;
};

union reg_isp_ldci_idx_filter_lut_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_filter_lut_10          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ldci_idx_filter_lut_11          : 11;
	} bits;
};

union reg_isp_ldci_idx_filter_lut_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_filter_lut_12          : 11;
		uint32_t _rsv_11                         : 5;
		uint32_t ldci_idx_filter_lut_13          : 11;
	} bits;
};

union reg_isp_ldci_idx_filter_lut_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_idx_filter_lut_14          : 11;
	} bits;
};

union reg_isp_ldci_interp_norm_lr1 {
	uint32_t raw;
	struct {
		uint32_t ldci_interp_norm_lr1            : 16;
		uint32_t ldci_interp_norm_ud1            : 16;
	} bits;
};

union reg_isp_ldci_sub_interp_norm_lr1 {
	uint32_t raw;
	struct {
		uint32_t ldci_sub_interp_norm_lr1        : 16;
		uint32_t ldci_sub_interp_norm_ud1        : 16;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_00_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_00_00       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_00_01       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_00_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_00_02       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_00_03       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_00_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_00_04       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_00_05       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_00_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_00_06       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_00_07       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_00_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_00_08       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_00_09       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_00_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_00_10       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_00_11       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_00_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_00_12       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_00_13       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_00_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_00_14       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_00_15       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_01_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_01_00       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_01_01       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_01_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_01_02       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_01_03       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_01_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_01_04       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_01_05       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_01_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_01_06       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_01_07       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_01_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_01_08       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_01_09       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_01_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_01_10       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_01_11       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_01_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_01_12       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_01_13       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_01_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_01_14       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_01_15       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_02_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_02_00       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_02_01       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_02_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_02_02       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_02_03       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_02_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_02_04       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_02_05       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_02_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_02_06       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_02_07       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_02_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_02_08       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_02_09       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_02_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_02_10       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_02_11       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_02_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_02_12       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_02_13       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_02_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_02_14       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_02_15       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_03_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_03_00       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_03_01       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_03_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_03_02       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_03_03       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_03_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_03_04       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_03_05       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_03_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_03_06       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_03_07       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_03_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_03_08       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_03_09       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_03_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_03_10       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_03_11       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_03_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_03_12       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_03_13       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_03_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_03_14       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_03_15       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_04_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_04_00       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_04_01       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_04_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_04_02       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_04_03       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_04_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_04_04       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_04_05       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_04_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_04_06       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_04_07       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_04_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_04_08       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_04_09       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_04_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_04_10       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_04_11       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_04_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_04_12       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_04_13       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_04_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_04_14       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_04_15       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_05_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_05_00       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_05_01       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_05_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_05_02       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_05_03       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_05_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_05_04       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_05_05       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_05_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_05_06       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_05_07       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_05_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_05_08       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_05_09       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_05_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_05_10       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_05_11       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_05_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_05_12       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_05_13       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_05_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_05_14       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_05_15       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_06_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_06_00       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_06_01       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_06_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_06_02       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_06_03       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_06_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_06_04       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_06_05       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_06_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_06_06       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_06_07       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_06_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_06_08       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_06_09       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_06_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_06_10       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_06_11       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_06_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_06_12       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_06_13       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_06_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_06_14       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_06_15       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_07_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_07_00       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_07_01       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_07_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_07_02       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_07_03       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_07_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_07_04       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_07_05       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_07_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_07_06       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_07_07       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_07_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_07_08       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_07_09       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_07_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_07_10       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_07_11       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_07_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_07_12       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_07_13       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_07_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_07_14       : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_07_15       : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_p_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_p_00        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_p_01        : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_p_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_p_02        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_p_03        : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_p_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_p_04        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_p_05        : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_p_06 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_p_06        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_p_07        : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_p_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_p_08        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_p_09        : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_p_10 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_p_10        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_p_11        : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_p_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_p_12        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_p_13        : 10;
	} bits;
};

union reg_isp_ldci_tone_curve_lut_p_14 {
	uint32_t raw;
	struct {
		uint32_t ldci_tone_curve_lut_p_14        : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_tone_curve_lut_p_15        : 10;
	} bits;
};

union reg_isp_ldci_luma_wgt_lut_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_lut_00            : 8;
		uint32_t ldci_luma_wgt_lut_01            : 8;
		uint32_t ldci_luma_wgt_lut_02            : 8;
		uint32_t ldci_luma_wgt_lut_03            : 8;
	} bits;
};

union reg_isp_ldci_luma_wgt_lut_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_lut_04            : 8;
		uint32_t ldci_luma_wgt_lut_05            : 8;
		uint32_t ldci_luma_wgt_lut_06            : 8;
		uint32_t ldci_luma_wgt_lut_07            : 8;
	} bits;
};

union reg_isp_ldci_luma_wgt_lut_08 {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_lut_08            : 8;
		uint32_t ldci_luma_wgt_lut_09            : 8;
		uint32_t ldci_luma_wgt_lut_10            : 8;
		uint32_t ldci_luma_wgt_lut_11            : 8;
	} bits;
};

union reg_isp_ldci_luma_wgt_lut_12 {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_lut_12            : 8;
		uint32_t ldci_luma_wgt_lut_13            : 8;
		uint32_t ldci_luma_wgt_lut_14            : 8;
		uint32_t ldci_luma_wgt_lut_15            : 8;
	} bits;
};

union reg_isp_ldci_luma_wgt_lut_16 {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_lut_16            : 8;
		uint32_t ldci_luma_wgt_lut_17            : 8;
		uint32_t ldci_luma_wgt_lut_18            : 8;
		uint32_t ldci_luma_wgt_lut_19            : 8;
	} bits;
};

union reg_isp_ldci_luma_wgt_lut_20 {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_lut_20            : 8;
		uint32_t ldci_luma_wgt_lut_21            : 8;
		uint32_t ldci_luma_wgt_lut_22            : 8;
		uint32_t ldci_luma_wgt_lut_23            : 8;
	} bits;
};

union reg_isp_ldci_luma_wgt_lut_24 {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_lut_24            : 8;
		uint32_t ldci_luma_wgt_lut_25            : 8;
		uint32_t ldci_luma_wgt_lut_26            : 8;
		uint32_t ldci_luma_wgt_lut_27            : 8;
	} bits;
};

union reg_isp_ldci_luma_wgt_lut_28 {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_lut_28            : 8;
		uint32_t ldci_luma_wgt_lut_29            : 8;
		uint32_t ldci_luma_wgt_lut_30            : 8;
		uint32_t ldci_luma_wgt_lut_31            : 8;
	} bits;
};

union reg_isp_ldci_luma_wgt_lut_32 {
	uint32_t raw;
	struct {
		uint32_t ldci_luma_wgt_lut_32            : 8;
	} bits;
};

union reg_isp_ldci_var_filter_lut_00 {
	uint32_t raw;
	struct {
		uint32_t ldci_var_filter_lut_00          : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_var_filter_lut_01          : 10;
	} bits;
};

union reg_isp_ldci_var_filter_lut_02 {
	uint32_t raw;
	struct {
		uint32_t ldci_var_filter_lut_02          : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_var_filter_lut_03          : 10;
	} bits;
};

union reg_isp_ldci_var_filter_lut_04 {
	uint32_t raw;
	struct {
		uint32_t ldci_var_filter_lut_04          : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t ldci_var_filter_lut_05          : 10;
	} bits;
};

union reg_isp_ldci_block_crop_size {
	uint32_t raw;
	struct {
		uint32_t block_img_width_crop            : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t block_img_height_crop           : 14;
	} bits;
};

union reg_isp_ldci_block_x_crop_size {
	uint32_t raw;
	struct {
		uint32_t block_crop_w_str                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t block_crop_w_end                : 14;
	} bits;
};

union reg_isp_ldci_block_y_crop_size {
	uint32_t raw;
	struct {
		uint32_t block_crop_h_str                : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t block_crop_h_end                : 14;
	} bits;
};

union reg_isp_ldci_block_crop_enable {
	uint32_t raw;
	struct {
		uint32_t block_crop_enable               : 1;
	} bits;
};

union reg_isp_ldci_block_x_offset {
	uint32_t raw;
	struct {
		uint32_t block_x_offset1                 : 4;
		uint32_t block_x_offset2                 : 4;
	} bits;
};
/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_rgb_top_0 {
	uint32_t raw;
	struct {
		uint32_t rgbtop_bayer_type               : 4;
		uint32_t rgbtop_rgbir_enable             : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t up_pq_en                        : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t prog_hdk_dis                    : 1;
	} bits;
};

union reg_isp_rgb_top_1 {
	uint32_t raw;
	struct {
		uint32_t rgb_shdw_sel                    : 1;
	} bits;
};

union reg_isp_rgb_top_2 {
	uint32_t raw;
	struct {
		uint32_t shdw_dmy                        : 32;
	} bits;
};

union reg_isp_rgb_top_3 {
	uint32_t raw;
	struct {
		uint32_t dmy                             : 32;
	} bits;
};

union reg_isp_rgb_top_4 {
	uint32_t raw;
	struct {
		uint32_t prob_out_sel                    : 5;
		uint32_t prob_perfmt                     : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t prob_fmt                        : 6;
	} bits;
};

union reg_isp_rgb_top_5 {
	uint32_t raw;
	struct {
		uint32_t prob_line                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t prob_pix                        : 12;
	} bits;
};

union reg_isp_rgb_top_6 {
	uint32_t raw;
	struct {
		uint32_t prob_r                          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t prob_g                          : 12;
	} bits;
};

union reg_isp_rgb_top_7 {
	uint32_t raw;
	struct {
		uint32_t prob_b                          : 12;
	} bits;
};

union reg_isp_rgb_top_8 {
	uint32_t raw;
	struct {
		uint32_t force_clk_enable                : 1;
		uint32_t dbg_en                          : 1;
	} bits;
};

union reg_isp_rgb_top_9 {
	uint32_t raw;
	struct {
		uint32_t rgbtop_imgw_m1                  : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t rgbtop_imgh_m1                  : 14;
	} bits;
};

union reg_isp_rgb_top_10 {
	uint32_t raw;
	struct {
		uint32_t ir_bit_mode                     : 1;
		uint32_t ir_sw_mode                      : 1;
		uint32_t ir_dmi_enable                   : 1;
		uint32_t ir_crop_enable                  : 1;
		uint32_t ir_dmi_num_sw                   : 14;
	} bits;
};

union reg_isp_rgb_top_11 {
	uint32_t raw;
	struct {
		uint32_t ir_img_width                    : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ir_img_height                   : 14;
	} bits;
};

union reg_isp_rgb_top_12 {
	uint32_t raw;
	struct {
		uint32_t ir_crop_w_str                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ir_crop_w_end                   : 14;
	} bits;
};

union reg_isp_rgb_top_13 {
	uint32_t raw;
	struct {
		uint32_t ir_crop_h_str                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ir_crop_h_end                   : 14;
	} bits;
};

union reg_isp_rgb_top_14 {
	uint32_t raw;
	struct {
		uint32_t irm_enable                      : 1;
		uint32_t irm_hw_rqos                     : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t ir_blending_wgt                 : 9;
		uint32_t _rsv_13                         : 3;
		uint32_t ir_dmi_num                      : 14;
	} bits;
};

union reg_isp_rgb_top_dbg_ip_s_vld {
	uint32_t raw;
	struct {
		uint32_t ip_s_tvalid                     : 31;
		uint32_t ip_dbg_en                       : 1;
	} bits;
};

union reg_isp_rgb_top_dbg_ip_s_rdy {
	uint32_t raw;
	struct {
		uint32_t ip_s_tready                     : 31;
	} bits;
};

union reg_isp_rgb_top_dbg_dmi_vld {
	uint32_t raw;
	struct {
		uint32_t ip_dmi_valid                    : 16;
	} bits;
};

union reg_isp_rgb_top_dbg_dmi_rdy {
	uint32_t raw;
	struct {
		uint32_t ip_dmi_ready                    : 16;
	} bits;
};

union reg_isp_rgb_top_patgen1 {
	uint32_t raw;
	struct {
		uint32_t x_curser                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t y_curser                        : 14;
		uint32_t curser_en                       : 1;
		uint32_t pg_enable                       : 1;
	} bits;
};

union reg_isp_rgb_top_patgen2 {
	uint32_t raw;
	struct {
		uint32_t curser_value                    : 16;
	} bits;
};

union reg_isp_rgb_top_patgen3 {
	uint32_t raw;
	struct {
		uint32_t value_report                    : 32;
	} bits;
};

union reg_isp_rgb_top_patgen4 {
	uint32_t raw;
	struct {
		uint32_t xcnt_rpt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ycnt_rpt                        : 14;
	} bits;
};

union reg_isp_rgb_top_chk_sum {
	uint32_t raw;
	struct {
		uint32_t chk_sum_y                       : 8;
		uint32_t chk_sum_u                       : 8;
		uint32_t chk_sum_v                       : 8;
		uint32_t chk_sum_en                      : 1;
	} bits;
};

union reg_isp_rgb_top_dma_idle {
	uint32_t raw;
	struct {
		uint32_t ip_dma_idle                     : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_pre_wdma_ctrl {
	uint32_t raw;
	struct {
		uint32_t wdmi_en_le                      : 1;
		uint32_t wdmi_en_se                      : 1;
		uint32_t dma_wr_mode                     : 1;
		uint32_t dma_wr_msb                      : 1;
		uint32_t _rsv_4                          : 27;
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_pre_wdma_ctrl_pre_raw_be_rdmi_dpcm {
	uint32_t raw;
	struct {
		uint32_t dpcm_mode                       : 3;
		uint32_t mipi_opt                        : 1;
		uint32_t _rsv_4                          : 12;
		uint32_t dpcm_xstr                       : 13;
	} bits;
};

union reg_pre_wdma_ctrl_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy_rw                        : 16;
		uint32_t dummy_ro                        : 16;
	} bits;
};

union reg_pre_wdma_ctrl_debug_info {
	uint32_t raw;
	struct {
		uint32_t wdma_done_le                    : 1;
		uint32_t wdma_done_se                    : 1;
	} bits;
};

union reg_pre_wdma_ctrl_wdma_enable {
	uint32_t raw;
	struct {
		uint32_t pre_dma_wproc_enable_le         : 1;
		uint32_t pre_dma_wproc_enable_se         : 1;
		uint32_t pre_raw_wdma_crop_enable        : 1;
	} bits;
};

union reg_pre_wdma_ctrl_wdma_insert {
	uint32_t raw;
	struct {
		uint32_t pre_raw_wdma_crop_img_width     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t pre_raw_wdma_crop_img_height    : 14;
	} bits;
};

union reg_pre_wdma_ctrl_wdma_insert_h {
	uint32_t raw;
	struct {
		uint32_t pre_raw_wdma_crop_w_str         : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t pre_raw_wdma_crop_w_end         : 14;
	} bits;
};

union reg_pre_wdma_ctrl_wdma_insert_w {
	uint32_t raw;
	struct {
		uint32_t pre_raw_wdma_crop_h_str         : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t pre_raw_wdma_crop_h_end         : 14;
	} bits;
};
/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_ee_00 {
	uint32_t raw;
	struct {
		uint32_t ee_enable                       : 1;
		uint32_t ee_shadow_sel                   : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t ee_debug_mode                   : 4;
		uint32_t ee_total_coring                 : 8;
		uint32_t ee_total_motion_coring          : 8;
		uint32_t ee_total_gain                   : 8;
	} bits;
};

union reg_isp_ee_04 {
	uint32_t raw;
	struct {
		uint32_t ee_total_oshtthrd               : 8;
		uint32_t ee_total_ushtthrd               : 8;
		uint32_t ee_debug_shift_bit              : 3;
		uint32_t _rsv_19                         : 12;
		uint32_t ee_pre_proc_enable              : 1;
	} bits;
};

union reg_isp_ee_0c {
	uint32_t raw;
	struct {
		uint32_t ee_lumaref_lpf_en               : 1;
		uint32_t ee_luma_coring_en               : 1;
		uint32_t _rsv_2                          : 4;
		uint32_t ee_luma_adptctrl_en             : 1;
		uint32_t ee_delta_adptctrl_en            : 1;
		uint32_t ee_delta_adptctrl_shift         : 2;
		uint32_t ee_chromaref_lpf_en             : 1;
		uint32_t ee_chroma_adptctrl_en           : 1;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_mf_core_gain                 : 8;
	} bits;
};

union reg_isp_ee_10 {
	uint32_t raw;
	struct {
		uint32_t hf_blend_wgt                    : 8;
		uint32_t mf_blend_wgt                    : 8;
	} bits;
};

union reg_isp_ee_a4 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_coring_lut_in_0         : 8;
		uint32_t ee_luma_coring_lut_in_1         : 8;
		uint32_t ee_luma_coring_lut_in_2         : 8;
		uint32_t ee_luma_coring_lut_in_3         : 8;
	} bits;
};

union reg_isp_ee_a8 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_coring_lut_out_0        : 8;
		uint32_t ee_luma_coring_lut_out_1        : 8;
		uint32_t ee_luma_coring_lut_out_2        : 8;
		uint32_t ee_luma_coring_lut_out_3        : 8;
	} bits;
};

union reg_isp_ee_ac {
	uint32_t raw;
	struct {
		uint32_t ee_luma_coring_lut_slope_0      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_luma_coring_lut_slope_1      : 12;
	} bits;
};

union reg_isp_ee_b0 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_coring_lut_slope_2      : 12;
	} bits;
};

union reg_isp_ee_b4 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_coring_lut_in_0       : 8;
		uint32_t ee_motion_coring_lut_in_1       : 8;
		uint32_t ee_motion_coring_lut_in_2       : 8;
		uint32_t ee_motion_coring_lut_in_3       : 8;
	} bits;
};

union reg_isp_ee_b8 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_coring_lut_out_0      : 8;
		uint32_t ee_motion_coring_lut_out_1      : 8;
		uint32_t ee_motion_coring_lut_out_2      : 8;
		uint32_t ee_motion_coring_lut_out_3      : 8;
	} bits;
};

union reg_isp_ee_bc {
	uint32_t raw;
	struct {
		uint32_t ee_motion_coring_lut_slope_0    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_motion_coring_lut_slope_1    : 12;
	} bits;
};

union reg_isp_ee_c0 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_coring_lut_slope_2    : 12;
	} bits;
};

union reg_isp_ee_c4 {
	uint32_t raw;
	struct {
		uint32_t ee_mcore_gain_lut_in_0          : 8;
		uint32_t ee_mcore_gain_lut_in_1          : 8;
		uint32_t ee_mcore_gain_lut_in_2          : 8;
		uint32_t ee_mcore_gain_lut_in_3          : 8;
	} bits;
};

union reg_isp_ee_c8 {
	uint32_t raw;
	struct {
		uint32_t ee_mcore_gain_lut_out_0         : 8;
		uint32_t ee_mcore_gain_lut_out_1         : 8;
		uint32_t ee_mcore_gain_lut_out_2         : 8;
		uint32_t ee_mcore_gain_lut_out_3         : 8;
	} bits;
};

union reg_isp_ee_hcc {
	uint32_t raw;
	struct {
		uint32_t ee_mcore_gain_lut_slope_0       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_mcore_gain_lut_slope_1       : 12;
	} bits;
};

union reg_isp_ee_hd0 {
	uint32_t raw;
	struct {
		uint32_t ee_mcore_gain_lut_slope_2       : 12;
	} bits;
};

union reg_isp_ee_130 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_00         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_01         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_02         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_03         : 6;
	} bits;
};

union reg_isp_ee_134 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_04         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_05         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_06         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_07         : 6;
	} bits;
};

union reg_isp_ee_138 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_08         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_09         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_10         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_11         : 6;
	} bits;
};

union reg_isp_ee_13c {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_12         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_13         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_14         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_15         : 6;
	} bits;
};

union reg_isp_ee_140 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_16         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_17         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_18         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_19         : 6;
	} bits;
};

union reg_isp_ee_144 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_20         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_21         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_22         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_23         : 6;
	} bits;
};

union reg_isp_ee_148 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_24         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_25         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_26         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_27         : 6;
	} bits;
};

union reg_isp_ee_14c {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_28         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_29         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_30         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_31         : 6;
	} bits;
};

union reg_isp_ee_150 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_32         : 6;
	} bits;
};

union reg_isp_ee_154 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_00        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_01        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_02        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_03        : 6;
	} bits;
};

union reg_isp_ee_158 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_04        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_05        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_06        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_07        : 6;
	} bits;
};

union reg_isp_ee_15c {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_08        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_09        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_10        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_11        : 6;
	} bits;
};

union reg_isp_ee_160 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_12        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_13        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_14        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_15        : 6;
	} bits;
};

union reg_isp_ee_164 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_16        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_17        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_18        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_19        : 6;
	} bits;
};

union reg_isp_ee_168 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_20        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_21        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_22        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_23        : 6;
	} bits;
};

union reg_isp_ee_16c {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_24        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_25        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_26        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_27        : 6;
	} bits;
};

union reg_isp_ee_170 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_28        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_29        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_30        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_31        : 6;
	} bits;
};

union reg_isp_ee_174 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_32        : 6;
	} bits;
};

union reg_isp_ee_178 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_00       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_01       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_02       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_03       : 6;
	} bits;
};

union reg_isp_ee_17c {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_04       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_05       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_06       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_07       : 6;
	} bits;
};

union reg_isp_ee_180 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_08       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_09       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_10       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_11       : 6;
	} bits;
};

union reg_isp_ee_184 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_12       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_13       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_14       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_15       : 6;
	} bits;
};

union reg_isp_ee_188 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_16       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_17       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_18       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_19       : 6;
	} bits;
};

union reg_isp_ee_18c {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_20       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_21       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_22       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_23       : 6;
	} bits;
};

union reg_isp_ee_190 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_24       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_25       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_26       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_27       : 6;
	} bits;
};

union reg_isp_ee_194 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_28       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_29       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_30       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_31       : 6;
	} bits;
};

union reg_isp_ee_198 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_32       : 6;
	} bits;
};

union reg_isp_ee_19c {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_amp_lut_in_0          : 8;
		uint32_t ee_chroma_amp_lut_in_1          : 8;
		uint32_t ee_chroma_amp_lut_in_2          : 8;
		uint32_t ee_chroma_amp_lut_in_3          : 8;
	} bits;
};

union reg_isp_ee_1a0 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_amp_lut_out_0         : 8;
		uint32_t ee_chroma_amp_lut_out_1         : 8;
		uint32_t ee_chroma_amp_lut_out_2         : 8;
		uint32_t ee_chroma_amp_lut_out_3         : 8;
	} bits;
};

union reg_isp_ee_1a4 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_amp_lut_slope_0       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_chroma_amp_lut_slope_1       : 12;
	} bits;
};

union reg_isp_ee_1a8 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_amp_lut_slope_2       : 12;
	} bits;
};

union reg_isp_ee_1c4 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 16;
		uint32_t ee_shtctrl_oshtgain             : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_shtctrl_ushtgain             : 6;
	} bits;
};

union reg_isp_ee_1c8 {
	uint32_t raw;
	struct {
		uint32_t ee_total_oshtthrd_clp           : 8;
		uint32_t ee_total_ushtthrd_clp           : 8;
	} bits;
};

union reg_isp_ee_1cc {
	uint32_t raw;
	struct {
		uint32_t ee_motion_lut_in_0              : 8;
		uint32_t ee_motion_lut_in_1              : 8;
		uint32_t ee_motion_lut_in_2              : 8;
		uint32_t ee_motion_lut_in_3              : 8;
	} bits;
};

union reg_isp_ee_1d0 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_lut_out_0             : 8;
		uint32_t ee_motion_lut_out_1             : 8;
		uint32_t ee_motion_lut_out_2             : 8;
		uint32_t ee_motion_lut_out_3             : 8;
	} bits;
};

union reg_isp_ee_1d4 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_lut_slope_0           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_motion_lut_slope_1           : 12;
	} bits;
};

union reg_isp_ee_1d8 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_lut_slope_2           : 12;
	} bits;
};

union reg_isp_ee_1dc {
	uint32_t raw;
	struct {
		uint32_t hf_coef_0                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t hf_coef_1                       : 9;
	} bits;
};

union reg_isp_ee_1e0 {
	uint32_t raw;
	struct {
		uint32_t hf_coef_2                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t hf_coef_3                       : 9;
	} bits;
};

union reg_isp_ee_1e4 {
	uint32_t raw;
	struct {
		uint32_t hf_coef_4                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t hf_coef_5                       : 9;
	} bits;
};

union reg_isp_ee_1e8 {
	uint32_t raw;
	struct {
		uint32_t mf_coef_0                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t mf_coef_1                       : 9;
	} bits;
};

union reg_isp_ee_1ec {
	uint32_t raw;
	struct {
		uint32_t mf_coef_2                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t mf_coef_3                       : 9;
	} bits;
};

union reg_isp_ee_1f0 {
	uint32_t raw;
	struct {
		uint32_t mf_coef_4                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t mf_coef_5                       : 9;
	} bits;
};

union reg_isp_ee_1f4 {
	uint32_t raw;
	struct {
		uint32_t mf_coef_6                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t mf_coef_7                       : 9;
	} bits;
};

union reg_isp_ee_1f8 {
	uint32_t raw;
	struct {
		uint32_t mf_coef_8                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t mf_coef_9                       : 9;
	} bits;
};

union reg_isp_ee_1fc {
	uint32_t raw;
	struct {
		uint32_t ee_soft_clamp_enable            : 1;
		uint32_t ee_cbcr_switch                  : 1;
		uint32_t _rsv_2                          : 6;
		uint32_t ee_upper_bound_left_diff        : 8;
		uint32_t ee_lower_bound_right_diff       : 8;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_ygamma_gamma_ctrl {
	uint32_t raw;
	struct {
		uint32_t ygamma_enable                   : 1;
		uint32_t gamma_shdw_sel                  : 1;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_ygamma_gamma_prog_ctrl {
	uint32_t raw;
	struct {
		uint32_t gamma_wsel                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t gamma_rsel                      : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t gamma_prog_en                   : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t gamma_prog_1to3_en              : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t gamma_prog_mode                 : 2;
		uint32_t _rsv_18                         : 6;
		uint32_t gamma_w                         : 1;
	} bits;
};

union reg_ygamma_gamma_prog_st_addr {
	uint32_t raw;
	struct {
		uint32_t gamma_st_addr                   : 8;
		uint32_t _rsv_8                          : 23;
		uint32_t gamma_st_w                      : 1;
	} bits;
};

union reg_ygamma_gamma_prog_data {
	uint32_t raw;
	struct {
		uint32_t gamma_data_e                    : 16;
		uint32_t gamma_data_o                    : 16;
	} bits;
};

union reg_ygamma_gamma_prog_max {
	uint32_t raw;
	struct {
		uint32_t ygamma_max                      : 17;
	} bits;
};

union reg_ygamma_gamma_mem_sw_raddr {
	uint32_t raw;
	struct {
		uint32_t gamma_sw_raddr                  : 8;
		uint32_t _rsv_8                          : 4;
		uint32_t gamma_sw_r_mem_sel              : 1;
	} bits;
};

union reg_ygamma_gamma_mem_sw_rdata {
	uint32_t raw;
	struct {
		uint32_t gamma_rdata_r                   : 16;
		uint32_t _rsv_16                         : 15;
		uint32_t gamma_sw_r                      : 1;
	} bits;
};

union reg_ygamma_gamma_mem_sw_rdata_bg {
	uint32_t raw;
	struct {
		uint32_t gamma_rdata_g                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t gamma_rdata_b                   : 12;
	} bits;
};

union reg_ygamma_gamma_dbg {
	uint32_t raw;
	struct {
		uint32_t prog_hdk_dis                    : 1;
		uint32_t softrst                         : 1;
	} bits;
};

union reg_ygamma_gamma_dmy0 {
	uint32_t raw;
	struct {
		uint32_t dmy_def0                        : 32;
	} bits;
};

union reg_ygamma_gamma_dmy1 {
	uint32_t raw;
	struct {
		uint32_t dmy_def1                        : 32;
	} bits;
};

union reg_ygamma_gamma_dmy_r {
	uint32_t raw;
	struct {
		uint32_t dmy_ro                          : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_preyee_00 {
	uint32_t raw;
	struct {
		uint32_t ee_enable                       : 1;
		uint32_t ee_shadow_sel                   : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t ee_debug_mode                   : 4;
		uint32_t ee_total_coring                 : 8;
		uint32_t ee_total_motion_coring          : 8;
		uint32_t ee_total_gain                   : 8;
	} bits;
};

union reg_isp_preyee_04 {
	uint32_t raw;
	struct {
		uint32_t ee_total_oshtthrd               : 8;
		uint32_t ee_total_ushtthrd               : 8;
		uint32_t ee_debug_shift_bit              : 3;
		uint32_t _rsv_19                         : 12;
		uint32_t ee_pre_proc_enable              : 1;
	} bits;
};

union reg_isp_preyee_0c {
	uint32_t raw;
	struct {
		uint32_t ee_lumaref_lpf_en               : 1;
		uint32_t ee_luma_coring_en               : 1;
		uint32_t _rsv_2                          : 4;
		uint32_t ee_luma_adptctrl_en             : 1;
		uint32_t ee_delta_adptctrl_en            : 1;
		uint32_t ee_delta_adptctrl_shift         : 2;
		uint32_t ee_chromaref_lpf_en             : 1;
		uint32_t ee_chroma_adptctrl_en           : 1;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_mf_core_gain                 : 8;
	} bits;
};

union reg_isp_preyee_10 {
	uint32_t raw;
	struct {
		uint32_t hf_blend_wgt                    : 8;
		uint32_t mf_blend_wgt                    : 8;
	} bits;
};

union reg_isp_preyee_a4 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_coring_lut_in_0         : 8;
		uint32_t ee_luma_coring_lut_in_1         : 8;
		uint32_t ee_luma_coring_lut_in_2         : 8;
		uint32_t ee_luma_coring_lut_in_3         : 8;
	} bits;
};

union reg_isp_preyee_a8 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_coring_lut_out_0        : 8;
		uint32_t ee_luma_coring_lut_out_1        : 8;
		uint32_t ee_luma_coring_lut_out_2        : 8;
		uint32_t ee_luma_coring_lut_out_3        : 8;
	} bits;
};

union reg_isp_preyee_ac {
	uint32_t raw;
	struct {
		uint32_t ee_luma_coring_lut_slope_0      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_luma_coring_lut_slope_1      : 12;
	} bits;
};

union reg_isp_preyee_b0 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_coring_lut_slope_2      : 12;
	} bits;
};

union reg_isp_preyee_b4 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_coring_lut_in_0       : 8;
		uint32_t ee_motion_coring_lut_in_1       : 8;
		uint32_t ee_motion_coring_lut_in_2       : 8;
		uint32_t ee_motion_coring_lut_in_3       : 8;
	} bits;
};

union reg_isp_preyee_b8 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_coring_lut_out_0      : 8;
		uint32_t ee_motion_coring_lut_out_1      : 8;
		uint32_t ee_motion_coring_lut_out_2      : 8;
		uint32_t ee_motion_coring_lut_out_3      : 8;
	} bits;
};

union reg_isp_preyee_bc {
	uint32_t raw;
	struct {
		uint32_t ee_motion_coring_lut_slope_0    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_motion_coring_lut_slope_1    : 12;
	} bits;
};

union reg_isp_preyee_c0 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_coring_lut_slope_2    : 12;
	} bits;
};

union reg_isp_preyee_c4 {
	uint32_t raw;
	struct {
		uint32_t ee_mcore_gain_lut_in_0          : 8;
		uint32_t ee_mcore_gain_lut_in_1          : 8;
		uint32_t ee_mcore_gain_lut_in_2          : 8;
		uint32_t ee_mcore_gain_lut_in_3          : 8;
	} bits;
};

union reg_isp_preyee_c8 {
	uint32_t raw;
	struct {
		uint32_t ee_mcore_gain_lut_out_0         : 8;
		uint32_t ee_mcore_gain_lut_out_1         : 8;
		uint32_t ee_mcore_gain_lut_out_2         : 8;
		uint32_t ee_mcore_gain_lut_out_3         : 8;
	} bits;
};

union reg_isp_preyee_hcc {
	uint32_t raw;
	struct {
		uint32_t ee_mcore_gain_lut_slope_0       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_mcore_gain_lut_slope_1       : 12;
	} bits;
};

union reg_isp_preyee_hd0 {
	uint32_t raw;
	struct {
		uint32_t ee_mcore_gain_lut_slope_2       : 12;
	} bits;
};

union reg_isp_preyee_130 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_00         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_01         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_02         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_03         : 6;
	} bits;
};

union reg_isp_preyee_134 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_04         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_05         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_06         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_07         : 6;
	} bits;
};

union reg_isp_preyee_138 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_08         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_09         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_10         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_11         : 6;
	} bits;
};

union reg_isp_preyee_13c {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_12         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_13         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_14         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_15         : 6;
	} bits;
};

union reg_isp_preyee_140 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_16         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_17         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_18         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_19         : 6;
	} bits;
};

union reg_isp_preyee_144 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_20         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_21         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_22         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_23         : 6;
	} bits;
};

union reg_isp_preyee_148 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_24         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_25         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_26         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_27         : 6;
	} bits;
};

union reg_isp_preyee_14c {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_28         : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_luma_adptctrl_lut_29         : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_luma_adptctrl_lut_30         : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_luma_adptctrl_lut_31         : 6;
	} bits;
};

union reg_isp_preyee_150 {
	uint32_t raw;
	struct {
		uint32_t ee_luma_adptctrl_lut_32         : 6;
	} bits;
};

union reg_isp_preyee_154 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_00        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_01        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_02        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_03        : 6;
	} bits;
};

union reg_isp_preyee_158 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_04        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_05        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_06        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_07        : 6;
	} bits;
};

union reg_isp_preyee_15c {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_08        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_09        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_10        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_11        : 6;
	} bits;
};

union reg_isp_preyee_160 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_12        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_13        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_14        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_15        : 6;
	} bits;
};

union reg_isp_preyee_164 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_16        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_17        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_18        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_19        : 6;
	} bits;
};

union reg_isp_preyee_168 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_20        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_21        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_22        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_23        : 6;
	} bits;
};

union reg_isp_preyee_16c {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_24        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_25        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_26        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_27        : 6;
	} bits;
};

union reg_isp_preyee_170 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_28        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_delta_adptctrl_lut_29        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_delta_adptctrl_lut_30        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_delta_adptctrl_lut_31        : 6;
	} bits;
};

union reg_isp_preyee_174 {
	uint32_t raw;
	struct {
		uint32_t ee_delta_adptctrl_lut_32        : 6;
	} bits;
};

union reg_isp_preyee_178 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_00       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_01       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_02       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_03       : 6;
	} bits;
};

union reg_isp_preyee_17c {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_04       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_05       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_06       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_07       : 6;
	} bits;
};

union reg_isp_preyee_180 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_08       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_09       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_10       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_11       : 6;
	} bits;
};

union reg_isp_preyee_184 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_12       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_13       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_14       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_15       : 6;
	} bits;
};

union reg_isp_preyee_188 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_16       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_17       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_18       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_19       : 6;
	} bits;
};

union reg_isp_preyee_18c {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_20       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_21       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_22       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_23       : 6;
	} bits;
};

union reg_isp_preyee_190 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_24       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_25       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_26       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_27       : 6;
	} bits;
};

union reg_isp_preyee_194 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_28       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t ee_chroma_adptctrl_lut_29       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t ee_chroma_adptctrl_lut_30       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_chroma_adptctrl_lut_31       : 6;
	} bits;
};

union reg_isp_preyee_198 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_adptctrl_lut_32       : 6;
	} bits;
};

union reg_isp_preyee_19c {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_amp_lut_in_0          : 8;
		uint32_t ee_chroma_amp_lut_in_1          : 8;
		uint32_t ee_chroma_amp_lut_in_2          : 8;
		uint32_t ee_chroma_amp_lut_in_3          : 8;
	} bits;
};

union reg_isp_preyee_1a0 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_amp_lut_out_0         : 8;
		uint32_t ee_chroma_amp_lut_out_1         : 8;
		uint32_t ee_chroma_amp_lut_out_2         : 8;
		uint32_t ee_chroma_amp_lut_out_3         : 8;
	} bits;
};

union reg_isp_preyee_1a4 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_amp_lut_slope_0       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_chroma_amp_lut_slope_1       : 12;
	} bits;
};

union reg_isp_preyee_1a8 {
	uint32_t raw;
	struct {
		uint32_t ee_chroma_amp_lut_slope_2       : 12;
	} bits;
};

union reg_isp_preyee_1c4 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 16;
		uint32_t ee_shtctrl_oshtgain             : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t ee_shtctrl_ushtgain             : 6;
	} bits;
};

union reg_isp_preyee_1c8 {
	uint32_t raw;
	struct {
		uint32_t ee_total_oshtthrd_clp           : 8;
		uint32_t ee_total_ushtthrd_clp           : 8;
	} bits;
};

union reg_isp_preyee_1cc {
	uint32_t raw;
	struct {
		uint32_t ee_motion_lut_in_0              : 8;
		uint32_t ee_motion_lut_in_1              : 8;
		uint32_t ee_motion_lut_in_2              : 8;
		uint32_t ee_motion_lut_in_3              : 8;
	} bits;
};

union reg_isp_preyee_1d0 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_lut_out_0             : 8;
		uint32_t ee_motion_lut_out_1             : 8;
		uint32_t ee_motion_lut_out_2             : 8;
		uint32_t ee_motion_lut_out_3             : 8;
	} bits;
};

union reg_isp_preyee_1d4 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_lut_slope_0           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ee_motion_lut_slope_1           : 12;
	} bits;
};

union reg_isp_preyee_1d8 {
	uint32_t raw;
	struct {
		uint32_t ee_motion_lut_slope_2           : 12;
	} bits;
};

union reg_isp_preyee_1dc {
	uint32_t raw;
	struct {
		uint32_t hf_coef_0                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t hf_coef_1                       : 9;
	} bits;
};

union reg_isp_preyee_1e0 {
	uint32_t raw;
	struct {
		uint32_t hf_coef_2                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t hf_coef_3                       : 9;
	} bits;
};

union reg_isp_preyee_1e4 {
	uint32_t raw;
	struct {
		uint32_t hf_coef_4                       : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t hf_coef_5                       : 9;
	} bits;
};

union reg_isp_preyee_1e8 {
	uint32_t raw;
	struct {
		uint32_t mf_coef_0                       : 9;
		uint32_t _rsv_9                          : 3;
		uint32_t mf_coef_1_0                     : 9;
		uint32_t _rsv_21                         : 1;
		uint32_t mf_coef_1_1                     : 9;
	} bits;
};

union reg_isp_preyee_1ec {
	uint32_t raw;
	struct {
		uint32_t mf_coef_2_0                     : 9;
		uint32_t _rsv_9                          : 3;
		uint32_t mf_coef_2_1                     : 9;
		uint32_t _rsv_21                         : 1;
		uint32_t mf_coef_3                       : 9;
	} bits;
};

union reg_isp_preyee_1f0 {
	uint32_t raw;
	struct {
		uint32_t mf_coef_4_0                     : 9;
		uint32_t _rsv_9                          : 3;
		uint32_t mf_coef_4_1                     : 9;
		uint32_t _rsv_21                         : 1;
		uint32_t mf_coef_5                       : 9;
	} bits;
};

union reg_isp_preyee_1fc {
	uint32_t raw;
	struct {
		uint32_t ee_soft_clamp_enable            : 1;
		uint32_t ee_cbcr_switch                  : 1;
		uint32_t _rsv_2                          : 6;
		uint32_t ee_upper_bound_left_diff        : 8;
		uint32_t ee_lower_bound_right_diff       : 8;
	} bits;
};

union reg_isp_preyee_200 {
	uint32_t raw;
	struct {
		uint32_t ee_dark_luma_thr                : 20;
	} bits;
};

union reg_isp_preyee_204 {
	uint32_t raw;
	struct {
		uint32_t ee_bright_luma_thr              : 20;
	} bits;
};

union reg_isp_preyee_208 {
	uint32_t raw;
	struct {
		uint32_t ee_dark_edge_thr                : 20;
	} bits;
};

union reg_isp_preyee_20c {
	uint32_t raw;
	struct {
		uint32_t ee_dark_edge_strength           : 20;
	} bits;
};

union reg_isp_preyee_210 {
	uint32_t raw;
	struct {
		uint32_t ee_bright_edge_thr              : 20;
	} bits;
};

union reg_isp_preyee_214 {
	uint32_t raw;
	struct {
		uint32_t ee_bright_edge_strength         : 20;
	} bits;
};

union reg_isp_preyee_218 {
	uint32_t raw;
	struct {
		uint32_t ee_line_thr                     : 20;
	} bits;
};

union reg_isp_preyee_21c {
	uint32_t raw;
	struct {
		uint32_t ee_line_strength                : 20;
	} bits;
};

union reg_isp_preyee_220 {
	uint32_t raw;
	struct {
		uint32_t ee_texture_mode                 : 1;
		uint32_t ee_shoot_mode                   : 1;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_csi_bdg_top_ctrl {
	uint32_t raw;
	struct {
		uint32_t csi_mode                        : 2;
		uint32_t csi_in_format                   : 1;
		uint32_t csi_in_yuv_format               : 1;
		uint32_t ch_num                          : 2;
		uint32_t ch0_dma_wr_enable               : 1;
		uint32_t ch1_dma_wr_enable               : 1;
		uint32_t ch2_dma_wr_enable               : 1;
		uint32_t y_only                          : 1;
		uint32_t pxl_data_sel                    : 1;
		uint32_t vs_pol                          : 1;
		uint32_t hs_pol                          : 1;
		uint32_t reset_mode                      : 1;
		uint32_t vs_mode                         : 1;
		uint32_t abort_mode                      : 1;
		uint32_t reset                           : 1;
		uint32_t abort                           : 1;
		uint32_t ch3_dma_wr_enable               : 1;
		uint32_t ch3_dma_420_wr_enable           : 1;
		uint32_t yuv_pack_mode                   : 1;
		uint32_t multi_ch_frame_sync_en          : 1;
		uint32_t ch0_dma_420_wr_enable           : 1;
		uint32_t ch1_dma_420_wr_enable           : 1;
		uint32_t mcsi_enable                      : 1;
		uint32_t tgen_enable                     : 1;
		uint32_t yuv2bay_enable                  : 1;
		uint32_t ch2_dma_420_wr_enable           : 1;
		uint32_t shdw_read_sel                   : 1;
		uint32_t _rsv_29                         : 2;
		uint32_t csi_up_reg                      : 1;
	} bits;
};

union reg_isp_csi_bdg_interrupt_ctrl {
	uint32_t raw;
	struct {
		uint32_t ch0_vs_int_en                   : 1;
		uint32_t ch0_trig_int_en                 : 1;
		uint32_t ch0_drop_int_en                 : 1;
		uint32_t ch0_size_error_int_en           : 1;
		uint32_t ch1_vs_int_en                   : 1;
		uint32_t ch1_trig_int_en                 : 1;
		uint32_t ch1_drop_int_en                 : 1;
		uint32_t ch1_size_error_int_en           : 1;
		uint32_t ch2_vs_int_en                   : 1;
		uint32_t ch2_trig_int_en                 : 1;
		uint32_t ch2_drop_int_en                 : 1;
		uint32_t ch2_size_error_int_en           : 1;
		uint32_t ch3_vs_int_en                   : 1;
		uint32_t ch3_trig_int_en                 : 1;
		uint32_t ch3_drop_int_en                 : 1;
		uint32_t ch3_size_error_int_en           : 1;
		uint32_t _rsv_16                         : 12;
		uint32_t slice_line_intp_en              : 1;
		uint32_t dma_error_intp_en               : 1;
		uint32_t line_intp_en                    : 1;
		uint32_t fifo_overflow_int_en            : 1;
	} bits;
};

union reg_isp_csi_bdg_dma_dpcm_mode {
	uint32_t raw;
	struct {
		uint32_t dma_st_dpcm_mode                : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t dpcm_mipi_opt                   : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t drop_mode                       : 4;
		uint32_t avg_mode                        : 4;
		uint32_t dpcm_xstr                       : 13;
	} bits;
};

union reg_isp_csi_bdg_dma_ld_dpcm_mode {
	uint32_t raw;
	struct {
		uint32_t dma_ld_dpcm_mode                : 3;
		uint32_t _rsv_3                          : 13;
		uint32_t dpcm_rx_xstr                    : 13;
	} bits;
};

union reg_isp_csi_bdg_ch0_size {
	uint32_t raw;
	struct {
		uint32_t ch0_frame_widthm1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch0_frame_heightm1              : 14;
	} bits;
};

union reg_isp_csi_bdg_ch1_size {
	uint32_t raw;
	struct {
		uint32_t ch1_frame_widthm1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch1_frame_heightm1              : 14;
	} bits;
};

union reg_isp_csi_bdg_ch2_size {
	uint32_t raw;
	struct {
		uint32_t ch2_frame_widthm1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch2_frame_heightm1              : 14;
	} bits;
};

union reg_isp_csi_bdg_ch3_size {
	uint32_t raw;
	struct {
		uint32_t ch3_frame_widthm1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch3_frame_heightm1              : 14;
	} bits;
};

union reg_isp_csi_bdg_ch0_crop_en {
	uint32_t raw;
	struct {
		uint32_t ch0_crop_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_ch0_horz_crop {
	uint32_t raw;
	struct {
		uint32_t ch0_horz_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch0_horz_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_ch0_vert_crop {
	uint32_t raw;
	struct {
		uint32_t ch0_vert_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch0_vert_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_ch0_blc_sum {
	uint32_t raw;
	struct {
		uint32_t ch0_blc_sum                     : 32;
	} bits;
};

union reg_isp_csi_bdg_ch1_crop_en {
	uint32_t raw;
	struct {
		uint32_t ch1_crop_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_ch1_horz_crop {
	uint32_t raw;
	struct {
		uint32_t ch1_horz_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch1_horz_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_ch1_vert_crop {
	uint32_t raw;
	struct {
		uint32_t ch1_vert_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch1_vert_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_ch1_blc_sum {
	uint32_t raw;
	struct {
		uint32_t ch1_blc_sum                     : 32;
	} bits;
};

union reg_isp_csi_bdg_ch2_crop_en {
	uint32_t raw;
	struct {
		uint32_t ch2_crop_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_ch2_horz_crop {
	uint32_t raw;
	struct {
		uint32_t ch2_horz_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch2_horz_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_ch2_vert_crop {
	uint32_t raw;
	struct {
		uint32_t ch2_vert_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch2_vert_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_ch2_blc_sum {
	uint32_t raw;
	struct {
		uint32_t ch2_blc_sum                     : 32;
	} bits;
};

union reg_isp_csi_bdg_ch3_crop_en {
	uint32_t raw;
	struct {
		uint32_t ch3_crop_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_ch3_horz_crop {
	uint32_t raw;
	struct {
		uint32_t ch3_horz_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch3_horz_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_ch3_vert_crop {
	uint32_t raw;
	struct {
		uint32_t ch3_vert_crop_start             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch3_vert_crop_end               : 14;
	} bits;
};

union reg_isp_csi_bdg_ch3_blc_sum {
	uint32_t raw;
	struct {
		uint32_t ch3_blc_sum                     : 32;
	} bits;
};

union reg_isp_csi_bdg_pat_gen_ctrl {
	uint32_t raw;
	struct {
		uint32_t pat_en                          : 1;
		uint32_t gra_inv                         : 1;
		uint32_t auto_en                         : 1;
		uint32_t dith_en                         : 1;
		uint32_t snow_en                         : 1;
		uint32_t fix_mc                          : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t dith_md                         : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t bayer_id                        : 2;
	} bits;
};

union reg_isp_csi_bdg_pat_idx_ctrl {
	uint32_t raw;
	struct {
		uint32_t pat_prd                         : 8;
		uint32_t pat_idx                         : 5;
	} bits;
};

union reg_isp_csi_bdg_pat_color_0 {
	uint32_t raw;
	struct {
		uint32_t pat_r                           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t pat_g                           : 12;
	} bits;
};

union reg_isp_csi_bdg_pat_color_1 {
	uint32_t raw;
	struct {
		uint32_t pat_b                           : 12;
	} bits;
};

union reg_isp_csi_bdg_background_color_0 {
	uint32_t raw;
	struct {
		uint32_t fde_r                           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t fde_g                           : 12;
	} bits;
};

union reg_isp_csi_bdg_background_color_1 {
	uint32_t raw;
	struct {
		uint32_t fde_b                           : 12;
	} bits;
};

union reg_isp_csi_bdg_fix_color_0 {
	uint32_t raw;
	struct {
		uint32_t mde_r                           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t mde_g                           : 12;
	} bits;
};

union reg_isp_csi_bdg_fix_color_1 {
	uint32_t raw;
	struct {
		uint32_t mde_b                           : 12;
	} bits;
};

union reg_isp_csi_bdg_mde_v_size {
	uint32_t raw;
	struct {
		uint32_t vmde_str                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t vmde_stp                        : 14;
	} bits;
};

union reg_isp_csi_bdg_mde_h_size {
	uint32_t raw;
	struct {
		uint32_t hmde_str                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t hmde_stp                        : 14;
	} bits;
};

union reg_isp_csi_bdg_fde_v_size {
	uint32_t raw;
	struct {
		uint32_t vfde_str                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t vfde_stp                        : 14;
	} bits;
};

union reg_isp_csi_bdg_fde_h_size {
	uint32_t raw;
	struct {
		uint32_t hfde_str                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t hfde_stp                        : 14;
	} bits;
};

union reg_isp_csi_bdg_hsync_ctrl {
	uint32_t raw;
	struct {
		uint32_t hs_str                          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t hs_stp                          : 14;
	} bits;
};

union reg_isp_csi_bdg_vsync_ctrl {
	uint32_t raw;
	struct {
		uint32_t vs_str                          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t vs_stp                          : 14;
	} bits;
};

union reg_isp_csi_bdg_tgen_tt_size {
	uint32_t raw;
	struct {
		uint32_t htt                             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t vtt                             : 14;
	} bits;
};

union reg_isp_csi_bdg_line_intp_height_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_line_intp_heightm1          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch1_line_intp_heightm1          : 14;
	} bits;
};

union reg_isp_csi_bdg_ch0_debug_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_pxl_cnt                     : 32;
	} bits;
};

union reg_isp_csi_bdg_ch0_debug_1 {
	uint32_t raw;
	struct {
		uint32_t ch0_line_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_ch0_debug_2 {
	uint32_t raw;
	struct {
		uint32_t ch0_vs_cnt                      : 16;
		uint32_t ch0_trig_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_ch0_debug_3 {
	uint32_t raw;
	struct {
		uint32_t ch0_tot_blk_idle                : 1;
		uint32_t ch0_tot_dma_idle                : 1;
		uint32_t ch0_bdg_dma_idle                : 1;
	} bits;
};

union reg_isp_csi_bdg_ch1_debug_0 {
	uint32_t raw;
	struct {
		uint32_t ch1_pxl_cnt                     : 32;
	} bits;
};

union reg_isp_csi_bdg_ch1_debug_1 {
	uint32_t raw;
	struct {
		uint32_t ch1_line_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_ch1_debug_2 {
	uint32_t raw;
	struct {
		uint32_t ch1_vs_cnt                      : 16;
		uint32_t ch1_trig_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_ch1_debug_3 {
	uint32_t raw;
	struct {
		uint32_t ch1_tot_blk_idle                : 1;
		uint32_t ch1_tot_dma_idle                : 1;
		uint32_t ch1_bdg_dma_idle                : 1;
	} bits;
};

union reg_isp_csi_bdg_ch2_debug_0 {
	uint32_t raw;
	struct {
		uint32_t ch2_pxl_cnt                     : 32;
	} bits;
};

union reg_isp_csi_bdg_ch2_debug_1 {
	uint32_t raw;
	struct {
		uint32_t ch2_line_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_ch2_debug_2 {
	uint32_t raw;
	struct {
		uint32_t ch2_vs_cnt                      : 16;
		uint32_t ch2_trig_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_ch2_debug_3 {
	uint32_t raw;
	struct {
		uint32_t ch2_tot_blk_idle                : 1;
		uint32_t ch2_tot_dma_idle                : 1;
		uint32_t ch2_bdg_dma_idle                : 1;
	} bits;
};

union reg_isp_csi_bdg_ch3_debug_0 {
	uint32_t raw;
	struct {
		uint32_t ch3_pxl_cnt                     : 32;
	} bits;
};

union reg_isp_csi_bdg_ch3_debug_1 {
	uint32_t raw;
	struct {
		uint32_t ch3_line_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_ch3_debug_2 {
	uint32_t raw;
	struct {
		uint32_t ch3_vs_cnt                      : 16;
		uint32_t ch3_trig_cnt                    : 16;
	} bits;
};

union reg_isp_csi_bdg_ch3_debug_3 {
	uint32_t raw;
	struct {
		uint32_t ch3_tot_blk_idle                : 1;
		uint32_t ch3_tot_dma_idle                : 1;
		uint32_t ch3_bdg_dma_idle                : 1;
	} bits;
};

union reg_isp_csi_bdg_interrupt_status_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_frame_drop_int              : 1;
		uint32_t ch0_vs_int                      : 1;
		uint32_t ch0_trig_int                    : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t ch0_frame_width_gt_int          : 1;
		uint32_t ch0_frame_width_ls_int          : 1;
		uint32_t ch0_frame_height_gt_int         : 1;
		uint32_t ch0_frame_height_ls_int         : 1;
		uint32_t ch1_frame_drop_int              : 1;
		uint32_t ch1_vs_int                      : 1;
		uint32_t ch1_trig_int                    : 1;
		uint32_t _rsv_11                         : 1;
		uint32_t ch1_frame_width_gt_int          : 1;
		uint32_t ch1_frame_width_ls_int          : 1;
		uint32_t ch1_frame_height_gt_int         : 1;
		uint32_t ch1_frame_height_ls_int         : 1;
		uint32_t ch2_frame_drop_int              : 1;
		uint32_t ch2_vs_int                      : 1;
		uint32_t ch2_trig_int                    : 1;
		uint32_t _rsv_19                         : 1;
		uint32_t ch2_frame_width_gt_int          : 1;
		uint32_t ch2_frame_width_ls_int          : 1;
		uint32_t ch2_frame_height_gt_int         : 1;
		uint32_t ch2_frame_height_ls_int         : 1;
		uint32_t ch3_frame_drop_int              : 1;
		uint32_t ch3_vs_int                      : 1;
		uint32_t ch3_trig_int                    : 1;
		uint32_t _rsv_27                         : 1;
		uint32_t ch3_frame_width_gt_int          : 1;
		uint32_t ch3_frame_width_ls_int          : 1;
		uint32_t ch3_frame_height_gt_int         : 1;
		uint32_t ch3_frame_height_ls_int         : 1;
	} bits;
};

union reg_isp_csi_bdg_interrupt_status_1 {
	uint32_t raw;
	struct {
		uint32_t fifo_overflow_int               : 1;
		uint32_t frame_resolution_over_max_int   : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t dma_error_int                   : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t ch0_line_intp_int               : 1;
		uint32_t ch1_line_intp_int               : 1;
		uint32_t ch2_line_intp_int               : 1;
		uint32_t ch3_line_intp_int               : 1;
		uint32_t ch0_slice_line_intp_int         : 1;
		uint32_t ch1_slice_line_intp_int         : 1;
		uint32_t ch2_slice_line_intp_int         : 1;
		uint32_t ch3_slice_line_intp_int         : 1;
	} bits;
};

union reg_isp_csi_bdg_debug {
	uint32_t raw;
	struct {
		uint32_t ring_buff_idle                  : 1;
	} bits;
};

union reg_isp_csi_bdg_out_vsync_line_delay {
	uint32_t raw;
	struct {
		uint32_t out_vsync_line_delay            : 12;
	} bits;
};

union reg_isp_csi_bdg_wr_urgent_ctrl {
	uint32_t raw;
	struct {
		uint32_t wr_near_overflow_threshold      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t wr_safe_threshold               : 13;
	} bits;
};

union reg_isp_csi_bdg_rd_urgent_ctrl {
	uint32_t raw;
	struct {
		uint32_t rd_near_overflow_threshold      : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t rd_safe_threshold               : 13;
	} bits;
};

union reg_isp_csi_bdg_dummy {
	uint32_t raw;
	struct {
		uint32_t dummy_in                        : 16;
		uint32_t dummy_out                       : 16;
	} bits;
};

union reg_isp_csi_bdg_line_intp_height_1 {
	uint32_t raw;
	struct {
		uint32_t ch2_line_intp_heightm1          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch3_line_intp_heightm1          : 14;
	} bits;
};

union reg_isp_csi_bdg_slice_line_intp_height_0 {
	uint32_t raw;
	struct {
		uint32_t ch0_slice_line_intp_heightm1    : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch1_slice_line_intp_heightm1    : 14;
	} bits;
};

union reg_isp_csi_bdg_slice_line_intp_height_1 {
	uint32_t raw;
	struct {
		uint32_t ch2_slice_line_intp_heightm1    : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ch3_slice_line_intp_heightm1    : 14;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch0_crop_en {
	uint32_t raw;
	struct {
		uint32_t st_ch0_crop_en                  : 1;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch0_horz_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch0_horz_crop_start          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t st_ch0_horz_crop_end            : 14;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch0_vert_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch0_vert_crop_start          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t st_ch0_vert_crop_end            : 14;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch1_crop_en {
	uint32_t raw;
	struct {
		uint32_t st_ch1_crop_en                  : 1;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch1_horz_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch1_horz_crop_start          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t st_ch1_horz_crop_end            : 14;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch1_vert_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch1_vert_crop_start          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t st_ch1_vert_crop_end            : 14;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch2_crop_en {
	uint32_t raw;
	struct {
		uint32_t st_ch2_crop_en                  : 1;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch2_horz_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch2_horz_crop_start          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t st_ch2_horz_crop_end            : 14;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch2_vert_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch2_vert_crop_start          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t st_ch2_vert_crop_end            : 14;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch3_crop_en {
	uint32_t raw;
	struct {
		uint32_t st_ch3_crop_en                  : 1;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch3_horz_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch3_horz_crop_start          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t st_ch3_horz_crop_end            : 14;
	} bits;
};

union reg_isp_csi_bdg_wdma_ch3_vert_crop {
	uint32_t raw;
	struct {
		uint32_t st_ch3_vert_crop_start          : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t st_ch3_vert_crop_end            : 14;
	} bits;
};

union reg_isp_csi_bdg_trig_dly_control_0 {
	uint32_t raw;
	struct {
		uint32_t trig_dly_en                     : 1;
	} bits;
};

union reg_isp_csi_bdg_trig_dly_control_1 {
	uint32_t raw;
	struct {
		uint32_t trig_dly_value                  : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_lcac_reg00 {
	uint32_t raw;
	struct {
		uint32_t lcac_enable                     : 1;
		uint32_t lcac_shdw_sel                   : 1;
		uint32_t force_clk_enable                : 1;
		uint32_t lcac_out_sel                    : 3;
	} bits;
};

union reg_isp_lcac_reg04 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_str_r1                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t lcac_lti_str_b1                 : 12;
	} bits;
};

union reg_isp_lcac_reg08 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_str_r2_le              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t lcac_lti_str_b2_le              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t lcac_lti_wgt_r_le               : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t lcac_lti_wgt_b_le               : 7;
	} bits;
};

union reg_isp_lcac_reg0c {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_str_r2_se              : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t lcac_lti_str_b2_se              : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t lcac_lti_wgt_r_se               : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t lcac_lti_wgt_b_se               : 7;
	} bits;
};

union reg_isp_lcac_reg10 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_kernel_r0              : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t lcac_lti_kernel_r1              : 10;
	} bits;
};

union reg_isp_lcac_reg14 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_kernel_r2              : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t lcac_lti_kernel_b0              : 10;
	} bits;
};

union reg_isp_lcac_reg18 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_kernel_b1              : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t lcac_lti_kernel_b2              : 10;
	} bits;
};

union reg_isp_lcac_reg1c {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_edge_scale_r_le        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t lcac_lti_edge_scale_g_le        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t lcac_lti_edge_scale_b_le        : 6;
	} bits;
};

union reg_isp_lcac_reg20 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_edge_scale_r_se        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t lcac_lti_edge_scale_g_se        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t lcac_lti_edge_scale_b_se        : 6;
	} bits;
};

union reg_isp_lcac_reg24 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_edge_coring_r          : 8;
		uint32_t lcac_lti_edge_coring_g          : 8;
		uint32_t lcac_lti_edge_coring_b          : 8;
	} bits;
};

union reg_isp_lcac_reg28 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_wgt_max_r              : 8;
		uint32_t lcac_lti_wgt_min_r              : 8;
		uint32_t lcac_lti_wgt_max_b              : 8;
		uint32_t lcac_lti_wgt_min_b              : 8;
	} bits;
};

union reg_isp_lcac_reg2c {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_var_wgt_r              : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t lcac_lti_var_wgt_b              : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t lcac_filter_scale               : 4;
		uint32_t _rsv_20                         : 4;
		uint32_t lcac_fcf_luma_blend_wgt         : 7;
	} bits;
};

union reg_isp_lcac_reg30 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_edge_scale_r_le        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t lcac_fcf_edge_scale_g_le        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t lcac_fcf_edge_scale_b_le        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t lcac_fcf_edge_scale_y_le        : 6;
	} bits;
};

union reg_isp_lcac_reg34 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_edge_scale_r_se        : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t lcac_fcf_edge_scale_g_se        : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t lcac_fcf_edge_scale_b_se        : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t lcac_fcf_edge_scale_y_se        : 6;
	} bits;
};

union reg_isp_lcac_reg38 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_edge_coring_r          : 8;
		uint32_t lcac_fcf_edge_coring_g            : 8;
		uint32_t lcac_fcf_edge_coring_b            : 8;
		uint32_t lcac_fcf_edge_coring_y            : 8;
	} bits;
};

union reg_isp_lcac_reg3c {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_wgt_max_r                : 8;
		uint32_t lcac_fcf_wgt_min_r                : 8;
		uint32_t lcac_fcf_wgt_max_b                : 8;
		uint32_t lcac_fcf_wgt_min_b                : 8;
	} bits;
};

union reg_isp_lcac_reg40 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_var_wgt_r                : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t lcac_fcf_var_wgt_g                : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t lcac_fcf_var_wgt_b                : 5;
	} bits;
};

union reg_isp_lcac_reg44 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_filter_kernel_00         : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t lcac_fcf_filter_kernel_01         : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t lcac_fcf_filter_kernel_02         : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t lcac_fcf_filter_kernel_03         : 7;
	} bits;
};

union reg_isp_lcac_reg48 {
	uint32_t raw;
	struct {
		uint32_t lcac_luma_kernel_00               : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t lcac_luma_kernel_01               : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t lcac_luma_kernel_02               : 7;
		uint32_t _rsv_23                         : 1;
		uint32_t lcac_luma_kernel_03               : 7;
	} bits;
};

union reg_isp_lcac_reg4c {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_filter_kernel_04       : 7;
		uint32_t _rsv_7                          : 1;
		uint32_t lcac_luma_kernel_04             : 7;
	} bits;
};

union reg_isp_lcac_reg50 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_luma_lut_00            : 8;
		uint32_t lcac_lti_luma_lut_01            : 8;
		uint32_t lcac_lti_luma_lut_02            : 8;
		uint32_t lcac_lti_luma_lut_03            : 8;
	} bits;
};

union reg_isp_lcac_reg54 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_luma_lut_04            : 8;
		uint32_t lcac_lti_luma_lut_05            : 8;
		uint32_t lcac_lti_luma_lut_06            : 8;
		uint32_t lcac_lti_luma_lut_07            : 8;
	} bits;
};

union reg_isp_lcac_reg58 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_luma_lut_08            : 8;
		uint32_t lcac_lti_luma_lut_09            : 8;
		uint32_t lcac_lti_luma_lut_10            : 8;
		uint32_t lcac_lti_luma_lut_11            : 8;
	} bits;
};

union reg_isp_lcac_reg5c {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_luma_lut_12            : 8;
		uint32_t lcac_lti_luma_lut_13            : 8;
		uint32_t lcac_lti_luma_lut_14            : 8;
		uint32_t lcac_lti_luma_lut_15            : 8;
	} bits;
};

union reg_isp_lcac_reg60 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_luma_lut_16            : 8;
		uint32_t lcac_lti_luma_lut_17            : 8;
		uint32_t lcac_lti_luma_lut_18            : 8;
		uint32_t lcac_lti_luma_lut_19            : 8;
	} bits;
};

union reg_isp_lcac_reg64 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_luma_lut_20            : 8;
		uint32_t lcac_lti_luma_lut_21            : 8;
		uint32_t lcac_lti_luma_lut_22            : 8;
		uint32_t lcac_lti_luma_lut_23            : 8;
	} bits;
};

union reg_isp_lcac_reg68 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_luma_lut_24            : 8;
		uint32_t lcac_lti_luma_lut_25            : 8;
		uint32_t lcac_lti_luma_lut_26            : 8;
		uint32_t lcac_lti_luma_lut_27            : 8;
	} bits;
};

union reg_isp_lcac_reg6c {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_luma_lut_28            : 8;
		uint32_t lcac_lti_luma_lut_29            : 8;
		uint32_t lcac_lti_luma_lut_30            : 8;
		uint32_t lcac_lti_luma_lut_31            : 8;
	} bits;
};

union reg_isp_lcac_reg70 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_luma_lut_00            : 8;
		uint32_t lcac_fcf_luma_lut_01            : 8;
		uint32_t lcac_fcf_luma_lut_02            : 8;
		uint32_t lcac_fcf_luma_lut_03            : 8;
	} bits;
};

union reg_isp_lcac_reg74 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_luma_lut_04            : 8;
		uint32_t lcac_fcf_luma_lut_05            : 8;
		uint32_t lcac_fcf_luma_lut_06            : 8;
		uint32_t lcac_fcf_luma_lut_07            : 8;
	} bits;
};

union reg_isp_lcac_reg78 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_luma_lut_08            : 8;
		uint32_t lcac_fcf_luma_lut_09            : 8;
		uint32_t lcac_fcf_luma_lut_10            : 8;
		uint32_t lcac_fcf_luma_lut_11            : 8;
	} bits;
};

union reg_isp_lcac_reg7c {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_luma_lut_12            : 8;
		uint32_t lcac_fcf_luma_lut_13            : 8;
		uint32_t lcac_fcf_luma_lut_14            : 8;
		uint32_t lcac_fcf_luma_lut_15            : 8;
	} bits;
};

union reg_isp_lcac_reg80 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_luma_lut_16            : 8;
		uint32_t lcac_fcf_luma_lut_17            : 8;
		uint32_t lcac_fcf_luma_lut_18            : 8;
		uint32_t lcac_fcf_luma_lut_19            : 8;
	} bits;
};

union reg_isp_lcac_reg84 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_luma_lut_20            : 8;
		uint32_t lcac_fcf_luma_lut_21            : 8;
		uint32_t lcac_fcf_luma_lut_22            : 8;
		uint32_t lcac_fcf_luma_lut_23            : 8;
	} bits;
};

union reg_isp_lcac_reg88 {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_luma_lut_24            : 8;
		uint32_t lcac_fcf_luma_lut_25            : 8;
		uint32_t lcac_fcf_luma_lut_26            : 8;
		uint32_t lcac_fcf_luma_lut_27            : 8;
	} bits;
};

union reg_isp_lcac_reg8c {
	uint32_t raw;
	struct {
		uint32_t lcac_fcf_luma_lut_28            : 8;
		uint32_t lcac_fcf_luma_lut_29            : 8;
		uint32_t lcac_fcf_luma_lut_30            : 8;
		uint32_t lcac_fcf_luma_lut_31            : 8;
	} bits;
};

union reg_isp_lcac_reg90 {
	uint32_t raw;
	struct {
		uint32_t lcac_lti_luma_lut_32            : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t lcac_fcf_luma_lut_32            : 8;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_cfa_00 {
	uint32_t raw;
	struct {
		uint32_t cfa_shdw_sel                    : 1;
		uint32_t cfa_enable                      : 1;
		uint32_t _rsv_2                          : 1;
		uint32_t cfa_ymoire_enable               : 1;
		uint32_t delay                           : 1;
		uint32_t force_clk_enable                : 1;
		uint32_t cfa_force_dir_enable            : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t cfa_force_dir_sel               : 2;
	} bits;
};

union reg_isp_cfa_04 {
	uint32_t raw;
	struct {
		uint32_t cfa_out_sel                     : 1;
		uint32_t cont_en                         : 1;
		uint32_t _rsv_2                          : 1;
		uint32_t softrst                         : 1;
		uint32_t _rsv_4                          : 1;
		uint32_t dbg_en                          : 1;
		uint32_t _rsv_6                          : 10;
		uint32_t cfa_edgee_thd2                  : 12;
	} bits;
};

union reg_isp_cfa_0c {
	uint32_t raw;
	struct {
		uint32_t cfa_edgee_thd                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_sige_thd                    : 12;
	} bits;
};

union reg_isp_cfa_10 {
	uint32_t raw;
	struct {
		uint32_t cfa_gsig_tol                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_rbsig_tol                   : 12;
	} bits;
};

union reg_isp_cfa_14 {
	uint32_t raw;
	struct {
		uint32_t cfa_edge_tol                    : 12;
	} bits;
};

union reg_isp_cfa_18 {
	uint32_t raw;
	struct {
		uint32_t cfa_ghp_thd                     : 16;
	} bits;
};

union reg_isp_cfa_1c {
	uint32_t raw;
	struct {
		uint32_t cfa_rb_vt_enable                : 1;
	} bits;
};

union reg_isp_cfa_20 {
	uint32_t raw;
	struct {
		uint32_t cfa_rbsig_luma_thd              : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_vw_thd                      : 12;
	} bits;
};

union reg_isp_cfa_30 {
	uint32_t raw;
	struct {
		uint32_t cfa_ghp_lut00                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cfa_ghp_lut01                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cfa_ghp_lut02                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cfa_ghp_lut03                   : 6;
	} bits;
};

union reg_isp_cfa_34 {
	uint32_t raw;
	struct {
		uint32_t cfa_ghp_lut04                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cfa_ghp_lut05                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cfa_ghp_lut06                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cfa_ghp_lut07                   : 6;
	} bits;
};

union reg_isp_cfa_38 {
	uint32_t raw;
	struct {
		uint32_t cfa_ghp_lut08                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cfa_ghp_lut09                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cfa_ghp_lut10                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cfa_ghp_lut11                   : 6;
	} bits;
};

union reg_isp_cfa_3c {
	uint32_t raw;
	struct {
		uint32_t cfa_ghp_lut12                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cfa_ghp_lut13                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cfa_ghp_lut14                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cfa_ghp_lut15                   : 6;
	} bits;
};

union reg_isp_cfa_40 {
	uint32_t raw;
	struct {
		uint32_t cfa_ghp_lut16                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cfa_ghp_lut17                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cfa_ghp_lut18                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cfa_ghp_lut19                   : 6;
	} bits;
};

union reg_isp_cfa_44 {
	uint32_t raw;
	struct {
		uint32_t cfa_ghp_lut20                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cfa_ghp_lut21                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cfa_ghp_lut22                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cfa_ghp_lut23                   : 6;
	} bits;
};

union reg_isp_cfa_48 {
	uint32_t raw;
	struct {
		uint32_t cfa_ghp_lut24                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cfa_ghp_lut25                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cfa_ghp_lut26                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cfa_ghp_lut27                   : 6;
	} bits;
};

union reg_isp_cfa_4c {
	uint32_t raw;
	struct {
		uint32_t cfa_ghp_lut28                   : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t cfa_ghp_lut29                   : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t cfa_ghp_lut30                   : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t cfa_ghp_lut31                   : 6;
	} bits;
};

union reg_isp_cfa_70 {
	uint32_t raw;
	struct {
		uint32_t dir_readcnt_from_line0          : 5;
	} bits;
};

union reg_isp_cfa_74 {
	uint32_t raw;
	struct {
		uint32_t prob_out_sel                    : 4;
		uint32_t prob_perfmt                     : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t prob_fmt                        : 6;
	} bits;
};

union reg_isp_cfa_78 {
	uint32_t raw;
	struct {
		uint32_t prob_line                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t prob_pix                        : 12;
	} bits;
};

union reg_isp_cfa_7c {
	uint32_t raw;
	struct {
		uint32_t cfa_dbg0                        : 32;
	} bits;
};

union reg_isp_cfa_80 {
	uint32_t raw;
	struct {
		uint32_t cfa_dbg1                        : 32;
	} bits;
};

union reg_isp_cfa_90 {
	uint32_t raw;
	struct {
		uint32_t cfa_ymoire_ref_maxg_only        : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t cfa_ymoire_np                   : 8;
	} bits;
};

union reg_isp_cfa_94 {
	uint32_t raw;
	struct {
		uint32_t cfa_ymoire_detail_th            : 8;
		uint32_t cfa_ymoire_detail_low           : 9;
	} bits;
};

union reg_isp_cfa_98 {
	uint32_t raw;
	struct {
		uint32_t cfa_ymoire_detail_high          : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t cfa_ymoire_detail_slope         : 15;
	} bits;
};

union reg_isp_cfa_9c {
	uint32_t raw;
	struct {
		uint32_t cfa_ymoire_edge_th              : 8;
		uint32_t cfa_ymoire_edge_low             : 9;
	} bits;
};

union reg_isp_cfa_a0 {
	uint32_t raw;
	struct {
		uint32_t cfa_ymoire_edge_high            : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t cfa_ymoire_edge_slope           : 15;
	} bits;
};

union reg_isp_cfa_a4 {
	uint32_t raw;
	struct {
		uint32_t cfa_ymoire_lut_th               : 10;
		uint32_t _rsv_10                         : 6;
		uint32_t cfa_ymoire_lut_low              : 9;
	} bits;
};

union reg_isp_cfa_a8 {
	uint32_t raw;
	struct {
		uint32_t cfa_ymoire_lut_high             : 9;
		uint32_t _rsv_9                          : 7;
		uint32_t cfa_ymoire_lut_slope            : 15;
	} bits;
};

union reg_isp_cfa_110 {
	uint32_t raw;
	struct {
		uint32_t cfa_ymoire_lpf_w                : 8;
		uint32_t cfa_ymoire_dc_w                 : 8;
	} bits;
};

union reg_isp_cfa_120 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_enable               : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t cfa_cmoire_strth                : 8;
	} bits;
};

union reg_isp_cfa_124 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_sat_x0               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_cmoire_sat_y0               : 12;
	} bits;
};

union reg_isp_cfa_128 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_sat_slp0             : 18;
	} bits;
};

union reg_isp_cfa_12c {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_sat_x1               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_cmoire_sat_y1               : 12;
	} bits;
};

union reg_isp_cfa_130 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_ptclr_x0             : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_cmoire_ptclr_y0             : 12;
	} bits;
};

union reg_isp_cfa_134 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_ptclr_slp0           : 18;
	} bits;
};

union reg_isp_cfa_138 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_ptclr_x1             : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_cmoire_ptclr_y1             : 12;
	} bits;
};

union reg_isp_cfa_13c {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_protclr1_enable      : 1;
		uint32_t cfa_cmoire_protclr2_enable      : 1;
		uint32_t cfa_cmoire_protclr3_enable      : 1;
	} bits;
};

union reg_isp_cfa_140 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_protclr1             : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_cmoire_protclr2             : 12;
	} bits;
};

union reg_isp_cfa_144 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_protclr3             : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_cmoire_pd_x0                : 12;
	} bits;
};

union reg_isp_cfa_148 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_pd_y0                : 12;
	} bits;
};

union reg_isp_cfa_14c {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_pd_slp0              : 18;
	} bits;
};

union reg_isp_cfa_150 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_pd_x1                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_cmoire_pd_y1                : 12;
	} bits;
};

union reg_isp_cfa_154 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_edge_x0              : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_cmoire_edge_y0              : 12;
	} bits;
};

union reg_isp_cfa_158 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_edge_slp0            : 18;
	} bits;
};

union reg_isp_cfa_15c {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_edge_x1              : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t cfa_cmoire_edge_y1              : 12;
	} bits;
};

union reg_isp_cfa_160 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_lumagain_enable      : 1;
	} bits;
};

union reg_isp_cfa_164 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_lumatg               : 12;
	} bits;
};

union reg_isp_cfa_168 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_edge_d0c0            : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t cfa_cmoire_edge_d0c1            : 13;
	} bits;
};

union reg_isp_cfa_16c {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_edge_d0c2            : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t cfa_cmoire_edge_d45c0           : 13;
	} bits;
};

union reg_isp_cfa_170 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_edge_d45c1           : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t cfa_cmoire_edge_d45c2           : 13;
	} bits;
};

union reg_isp_cfa_174 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_edge_d45c3           : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t cfa_cmoire_edge_d45c4           : 13;
	} bits;
};

union reg_isp_cfa_178 {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_edge_d45c5           : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t cfa_cmoire_edge_d45c6           : 13;
	} bits;
};

union reg_isp_cfa_17c {
	uint32_t raw;
	struct {
		uint32_t cfa_cmoire_edge_d45c7           : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t cfa_cmoire_edge_d45c8           : 13;
	} bits;
};

union reg_isp_cfa_180 {
	uint32_t raw;
	struct {
		uint32_t _cfa_shpn_enable                : 1;
		uint32_t _cfa_shpn_pre_proc_enable       : 1;
		uint32_t _rsv_2                          : 6;
		uint32_t _cfa_shpn_min_y                 : 12;
	} bits;
};

union reg_isp_cfa_184 {
	uint32_t raw;
	struct {
		uint32_t _cfa_shpn_min_gain              : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t _cfa_shpn_max_gain              : 12;
	} bits;
};

union reg_isp_cfa_188 {
	uint32_t raw;
	struct {
		uint32_t cfa_shpn_mf_core_gain           : 8;
		uint32_t cfa_shpn_hf_blend_wgt           : 8;
		uint32_t cfa_shpn_mf_blend_wgt           : 8;
	} bits;
};

union reg_isp_cfa_18c {
	uint32_t raw;
	struct {
		uint32_t cfa_shpn_core_value             : 12;
	} bits;
};

union reg_isp_cfa_190 {
	uint32_t raw;
	struct {
		uint32_t cfa_shpn_gain_value             : 8;
	} bits;
};
/******************************************/
/*           module definition            */
/******************************************/
union reg_pre_raw_vi_sel_0 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_reset                 : 1;
		uint32_t ring_buff_monitor_en            : 1;
		uint32_t dma_ld_dpcm_mode                : 3;
		uint32_t _rsv_5                          : 11;
		uint32_t dpcm_rx_xstr                    : 13;
	} bits;
};

union reg_pre_raw_vi_sel_1 {
	uint32_t raw;
	struct {
		uint32_t frame_widthm1                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t frame_heightm1                  : 14;
	} bits;
};

union reg_pre_raw_vi_sel_2 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_threshold_0_le        : 16;
		uint32_t ring_buff_threshold_1_le        : 16;
	} bits;
};

union reg_pre_raw_vi_sel_3 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_threshold_2_le        : 16;
		uint32_t ring_buff_threshold_3_le        : 16;
	} bits;
};

union reg_pre_raw_vi_sel_4 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_above_th_0_le         : 32;
	} bits;
};

union reg_pre_raw_vi_sel_5 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_above_th_1_le         : 32;
	} bits;
};

union reg_pre_raw_vi_sel_6 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_above_th_2_le         : 32;
	} bits;
};

union reg_pre_raw_vi_sel_7 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_above_th_3_le         : 32;
	} bits;
};

union reg_pre_raw_vi_sel_8 {
	uint32_t raw;
	struct {
		uint32_t ip_dma_idle                     : 32;
	} bits;
};

union reg_pre_raw_vi_sel_9 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_threshold_0_se        : 16;
		uint32_t ring_buff_threshold_1_se        : 16;
	} bits;
};

union reg_pre_raw_vi_sel_10 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_threshold_2_se        : 16;
		uint32_t ring_buff_threshold_3_se        : 16;
	} bits;
};

union reg_pre_raw_vi_sel_11 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_above_th_0_se         : 32;
	} bits;
};

union reg_pre_raw_vi_sel_12 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_above_th_1_se         : 32;
	} bits;
};

union reg_pre_raw_vi_sel_13 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_above_th_2_se         : 32;
	} bits;
};

union reg_pre_raw_vi_sel_14 {
	uint32_t raw;
	struct {
		uint32_t ring_buff_above_th_3_se         : 32;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_yuv_top_yuv_0 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 8;
		uint32_t yuv_top_sel                     : 1;
	} bits;
};

union reg_yuv_top_yuv_2 {
	uint32_t raw;
	struct {
		uint32_t fd_int                          : 1;
		uint32_t dma_int                         : 1;
		uint32_t frame_overflow                  : 1;
	} bits;
};

union reg_yuv_top_yuv_3 {
	uint32_t raw;
	struct {
		uint32_t _rsv_0                          : 2;
		uint32_t yonly_en                        : 1;
	} bits;
};

union reg_yuv_top_yuv_debug_0 {
	uint32_t raw;
	struct {
		uint32_t debug_bus                       : 32;
	} bits;
};

union reg_yuv_top_yuv_4 {
	uint32_t raw;
	struct {
		uint32_t dummy                           : 32;
	} bits;
};

union reg_yuv_top_yuv_debug_state {
	uint32_t raw;
	struct {
		uint32_t ma_idle                         : 16;
		uint32_t _rsv_16                         : 15;
		uint32_t idle                            : 1;
	} bits;
};

union reg_yuv_top_yuv_5 {
	uint32_t raw;
	struct {
		uint32_t dis_uv2dram                     : 1;
		uint32_t line_thres_en                   : 1;
		uint32_t _rsv_2                          : 6;
		uint32_t line_thres                      : 14;
		uint32_t _rsv_22                         : 2;
		uint32_t pg2_enable                      : 1;
	} bits;
};

union reg_yuv_top_yuv_ctrl {
	uint32_t raw;
	struct {
		uint32_t checksum_enable                 : 1;
		uint32_t sc_dma_switch                   : 1;
		uint32_t avg_mode                        : 1;
		uint32_t bypass_h                        : 1;
		uint32_t bypass_v                        : 1;
		uint32_t drop_mode                       : 1;
		uint32_t yc_swap                         : 1;
		uint32_t curser2_en                      : 1;
		uint32_t guard_cnt                       : 8;
	} bits;
};

union reg_yuv_top_imgw_m1 {
	uint32_t raw;
	struct {
		uint32_t yuv_top_imgw_m1                 : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t yuv_top_imgh_m1                 : 14;
	} bits;
};

union reg_yuv_top_stvalid_status {
	uint32_t raw;
	struct {
		uint32_t stvalid_status                  : 32;
	} bits;
};

union reg_yuv_top_stready_status {
	uint32_t raw;
	struct {
		uint32_t stready_status                  : 32;
	} bits;
};

union reg_yuv_top_patgen1 {
	uint32_t raw;
	struct {
		uint32_t x_curser                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t y_curser                        : 14;
		uint32_t curser_en                       : 1;
		uint32_t pg_enable                       : 1;
	} bits;
};

union reg_yuv_top_patgen2 {
	uint32_t raw;
	struct {
		uint32_t curser_value                    : 16;
	} bits;
};

union reg_yuv_top_patgen3 {
	uint32_t raw;
	struct {
		uint32_t value_report                    : 32;
	} bits;
};

union reg_yuv_top_patgen4 {
	uint32_t raw;
	struct {
		uint32_t xcnt_rpt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ycnt_rpt                        : 14;
	} bits;
};

union reg_yuv_top_check_sum {
	uint32_t raw;
	struct {
		uint32_t k_sum                           : 32;
	} bits;
};

union reg_yuv_top_ai_isp_rdma_ctrl {
	uint32_t raw;
	struct {
		uint32_t ai_isp_rdma_enable              : 3;
		uint32_t ai_isp_crop_enable              : 1;
		uint32_t ai_isp_enable                   : 1;
		uint32_t ai_isp_mask                     : 1;
	} bits;
};

union reg_yuv_top_ai_isp_img_size_y {
	uint32_t raw;
	struct {
		uint32_t ai_isp_img_width_crop_0         : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ai_isp_img_height_crop_0        : 14;
	} bits;
};

union reg_yuv_top_ai_isp_w_crop_y {
	uint32_t raw;
	struct {
		uint32_t ai_isp_crop_w_str_0             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ai_isp_crop_w_end_0             : 14;
	} bits;
};

union reg_yuv_top_ai_isp_h_crop_y {
	uint32_t raw;
	struct {
		uint32_t ai_isp_crop_h_str_0             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ai_isp_crop_h_end_0             : 14;
	} bits;
};

union reg_yuv_top_ai_isp_img_size_uv {
	uint32_t raw;
	struct {
		uint32_t ai_isp_img_width_crop_1         : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ai_isp_img_height_crop_1        : 14;
	} bits;
};

union reg_yuv_top_ai_isp_w_crop_uv {
	uint32_t raw;
	struct {
		uint32_t ai_isp_crop_w_str_1             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ai_isp_crop_w_end_1             : 14;
	} bits;
};

union reg_yuv_top_ai_isp_h_crop_uv {
	uint32_t raw;
	struct {
		uint32_t ai_isp_crop_h_str_1             : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ai_isp_crop_h_end_1             : 14;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_lsc_status {
	uint32_t raw;
	struct {
		uint32_t lsc_status                      : 32;
	} bits;
};

union reg_isp_lsc_grace_reset {
	uint32_t raw;
	struct {
		uint32_t lsc_grace_reset                 : 1;
	} bits;
};

union reg_isp_lsc_monitor {
	uint32_t raw;
	struct {
		uint32_t lsc_monitor                     : 32;
	} bits;
};

union reg_isp_lsc_enable {
	uint32_t raw;
	struct {
		uint32_t lsc_enable                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t lsc_gain_3p9_0_4p8_1            : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t lsc_gain_bicubic_0_bilinear_1   : 1;
		uint32_t lsc_boundary_interpolation_mode : 1;
		uint32_t _rsv_10                         : 2;
		uint32_t lsc_renormalize_enable          : 1;
		uint32_t _rsv_13                         : 3;
		uint32_t lsc_hdr_enable                  : 1;
		uint32_t _rsv_17                         : 3;
		uint32_t lsc_blocking_gain_update_enable : 1;
		uint32_t _rsv_21                         : 3;
		uint32_t lsc_35tile_enable               : 1;
		uint32_t _rsv_25                         : 3;
		uint32_t force_clk_enable                : 1;
	} bits;
};

union reg_isp_lsc_kickoff {
	uint32_t raw;
	struct {
		uint32_t lsc_kickoff                     : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t lsc_gainmover_enable            : 1;
		uint32_t _rsv_5                          : 7;
		uint32_t lsc_shadow_select               : 1;
	} bits;
};

union reg_isp_lsc_strength {
	uint32_t raw;
	struct {
		uint32_t lsc_strength                    : 12;
	} bits;
};

union reg_isp_lsc_img_bayerid {
	uint32_t raw;
	struct {
		uint32_t img_bayerid                     : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t force_bayer_enable              : 1;
	} bits;
};

union reg_isp_lsc_monitor_select {
	uint32_t raw;
	struct {
		uint32_t lsc_monitor_select              : 32;
	} bits;
};

union reg_isp_lsc_dmi_widthm1 {
	uint32_t raw;
	struct {
		uint32_t lsc_dmi_widthm1                 : 13;
	} bits;
};

union reg_isp_lsc_dmi_heightm1 {
	uint32_t raw;
	struct {
		uint32_t lsc_dmi_heightm1                : 13;
	} bits;
};

union reg_isp_lsc_gain_base {
	uint32_t raw;
	struct {
		uint32_t lsc_gain_base                   : 2;
	} bits;
};

union reg_isp_lsc_xstep {
	uint32_t raw;
	struct {
		uint32_t lsc_xstep                       : 15;
	} bits;
};

union reg_isp_lsc_ystep {
	uint32_t raw;
	struct {
		uint32_t lsc_ystep                       : 15;
	} bits;
};

union reg_isp_lsc_imgx0 {
	uint32_t raw;
	struct {
		uint32_t lsc_imgx0                       : 22;
	} bits;
};

union reg_isp_lsc_imgy0 {
	uint32_t raw;
	struct {
		uint32_t lsc_imgy0                       : 22;
	} bits;
};

union reg_isp_lsc_initx0 {
	uint32_t raw;
	struct {
		uint32_t lsc_initx0                      : 22;
	} bits;
};

union reg_isp_lsc_inity0 {
	uint32_t raw;
	struct {
		uint32_t lsc_inity0                      : 22;
	} bits;
};

union reg_isp_lsc_kernel_table_write {
	uint32_t raw;
	struct {
		uint32_t lsc_kernel_table_write          : 1;
	} bits;
};

union reg_isp_lsc_kernel_table_data {
	uint32_t raw;
	struct {
		uint32_t lsc_kernel_table_data           : 32;
	} bits;
};

union reg_isp_lsc_kernel_table_ctrl {
	uint32_t raw;
	struct {
		uint32_t lsc_kernel_table_start          : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t lsc_kernel_table_w              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t lsc_kernel_table_done           : 1;
	} bits;
};

union reg_isp_lsc_dummy {
	uint32_t raw;
	struct {
		uint32_t lsc_dummy                       : 16;
		uint32_t lsc_debug                       : 16;
	} bits;
};

union reg_isp_lsc_location {
	uint32_t raw;
	struct {
		uint32_t lsc_location                    : 32;
	} bits;
};

union reg_isp_lsc_1st_runhit {
	uint32_t raw;
	struct {
		uint32_t lsc_1st_runhit                  : 32;
	} bits;
};

union reg_isp_lsc_compare_value {
	uint32_t raw;
	struct {
		uint32_t lsc_compare_value               : 32;
	} bits;
};

union reg_isp_lsc_mem_sw_mode {
	uint32_t raw;
	struct {
		uint32_t lsc_mem_sw_mode                 : 1;
		uint32_t _rsv_1                          : 4;
		uint32_t lsc_cubic_kernel_mem_sel        : 1;
	} bits;
};

union reg_isp_lsc_mem_sw_raddr {
	uint32_t raw;
	struct {
		uint32_t lsc_sw_raddr                    : 7;
	} bits;
};

union reg_isp_lsc_mem_sw_rdata {
	uint32_t raw;
	struct {
		uint32_t lsc_rdata                       : 31;
		uint32_t lsc_sw_read                     : 1;
	} bits;
};

union reg_isp_lsc_interpolation {
	uint32_t raw;
	struct {
		uint32_t lsc_boundary_interpolation_lf_range: 6;
		uint32_t _rsv_6                          : 2;
		uint32_t lsc_boundary_interpolation_up_range: 6;
		uint32_t _rsv_14                         : 2;
		uint32_t lsc_boundary_interpolation_rt_range: 6;
		uint32_t _rsv_22                         : 2;
		uint32_t lsc_boundary_interpolation_dn_range: 6;
	} bits;
};

union reg_isp_lsc_dmi_enable {
	uint32_t raw;
	struct {
		uint32_t dmi_enable                      : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t dmi_qos                         : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t force_dma_disable               : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t dmi_pull_after_done             : 1;
	} bits;
};

union reg_isp_lsc_bld {
	uint32_t raw;
	struct {
		uint32_t lsc_bldratio_enable             : 1;
		uint32_t _rsv_1                          : 15;
		uint32_t lsc_bldratio                    : 9;
	} bits;
};

union reg_isp_lsc_intp_gain_max {
	uint32_t raw;
	struct {
		uint32_t lsc_intp_gain_max               : 26;
	} bits;
};

union reg_isp_lsc_intp_gain_min {
	uint32_t raw;
	struct {
		uint32_t lsc_intp_gain_min               : 26;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_rgbir_ctrl {
	uint32_t raw;
	struct {
		uint32_t rgbir2rggb_enable               : 1;
		uint32_t force_clk_enable                : 1;
		uint32_t se_in_sel                       : 1;
		uint32_t force_pclk_enable               : 1;
		uint32_t rgbir2rggb_comp_enable          : 1;
		uint32_t rgbir2rggb_ir_wdma_mode         : 1;
		uint32_t softrst                         : 1;
	} bits;
};

union reg_isp_rgbir_gain_offset_1 {
	uint32_t raw;
	struct {
		uint32_t rgbir2rggb_rec_gbr_gain         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbir2rggb_rec_gbr_offset       : 12;
	} bits;
};

union reg_isp_rgbir_gain_offset_2 {
	uint32_t raw;
	struct {
		uint32_t rgbir2rggb_rec_gir_gain         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbir2rggb_rec_gir_offset       : 12;
	} bits;
};

union reg_isp_rgbir_gain_offset_3 {
	uint32_t raw;
	struct {
		uint32_t rgbir2rggb_rec_rg_gain          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbir2rggb_rec_rg_offset        : 12;
	} bits;
};

union reg_isp_rgbir_gain_offset_4 {
	uint32_t raw;
	struct {
		uint32_t rgbir2rggb_rec_bg_gain          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t rgbir2rggb_rec_bg_offset        : 12;
	} bits;
};

union reg_isp_rgbir_shdw_read_sel {
	uint32_t raw;
	struct {
		uint32_t shdw_read_sel                   : 1;
	} bits;
};

union reg_isp_rgbir_comp_gain {
	uint32_t raw;
	struct {
		uint32_t rgbir2rggb_g_comp_gain          : 4;
		uint32_t rgbir2rggb_r_comp_gain          : 4;
		uint32_t rgbir2rggb_b_comp_gain          : 4;
	} bits;
};

union reg_isp_rgbir_wdma_ctl {
	uint32_t raw;
	struct {
		uint32_t rgbir2rggb_dma_enable           : 2;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_isp_line_spliter_img_size_fe0 {
	uint32_t raw;
	struct {
		uint32_t img_in_w_m1                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_in_h_m1                     : 14;
	} bits;
};

union reg_isp_line_spliter_img_width_nbld {
	uint32_t raw;
	struct {
		uint32_t nbld_w_str                      : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t nbld_w_end                      : 14;
	} bits;
};

union reg_isp_line_spliter_img_width_bld {
	uint32_t raw;
	struct {
		uint32_t bld_w_str                       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t bld_w_end                       : 14;
	} bits;
};

union reg_isp_line_spliter_img_size_nbld {
	uint32_t raw;
	struct {
		uint32_t img_width_nb                    : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_height_nb                   : 14;
	} bits;
};

union reg_isp_line_spliter_img_size_bld {
	uint32_t raw;
	struct {
		uint32_t img_width_b                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_height_b                    : 14;
	} bits;
};

union reg_isp_line_spliter_img_size_fe1 {
	uint32_t raw;
	struct {
		uint32_t img_width_fe1                   : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t img_height_fe1                  : 14;
	} bits;
};

union reg_isp_line_spliter_frame_size_fe0 {
	uint32_t raw;
	struct {
		uint32_t frame_widthm1_fe0               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t frame_heightm1_fe0              : 14;
	} bits;
};

union reg_isp_line_spliter_frame_size_fe1 {
	uint32_t raw;
	struct {
		uint32_t frame_widthm1_fe1               : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t frame_heightm1_fe1              : 14;
	} bits;
};

union reg_isp_line_spliter_sel_ctrl {
	uint32_t raw;
	struct {
		uint32_t fe0_sel                         : 1;
		uint32_t fe1_sel                         : 1;
		uint32_t fe0_rdma_sel                    : 1;
		uint32_t fe1_rdma_sel                    : 1;
	} bits;
};

union reg_isp_line_spliter_dma_ctrl {
	uint32_t raw;
	struct {
		uint32_t fe0_wdma_l_enable               : 1;
		uint32_t fe0_wdma_s_enable               : 1;
		uint32_t fe1_wdma_l_enable               : 1;
		uint32_t fe1_wdma_s_enable               : 1;
		uint32_t rdma_l_enable_fe0               : 1;
		uint32_t rdma_s_enable_fe0               : 1;
		uint32_t rdma_l_enable_fe1               : 1;
		uint32_t rdma_s_enable_fe1               : 1;
	} bits;
};

union reg_isp_line_spliter_yuv_mode {
	uint32_t raw;
	struct {
		uint32_t yuv_format_fe0                  : 1;
		uint32_t yuv_format_fe1                  : 1;
	} bits;
};

union reg_isp_line_spliter_enable {
	uint32_t raw;
	struct {
		uint32_t line_spliter_enable             : 1;
	} bits;
};

union reg_isp_line_spliter_lese_arbiter_ctrl {
	uint32_t raw;
	struct {
		uint32_t ch_num_fe0                      : 1;
		uint32_t pass_sel_fe0                    : 1;
		uint32_t pass_cnt_m1_fe0                 : 8;
		uint32_t ch_num_fe1                      : 1;
		uint32_t pass_sel_fe1                    : 1;
		uint32_t pass_cnt_m1_fe1                 : 8;
	} bits;
};

union reg_isp_line_spliter_vs_sw_ctrl {
	uint32_t raw;
	struct {
		uint32_t fe0_vs_sel                      : 1;
		uint32_t fe1_vs_sel                      : 1;
		uint32_t fe0_sw_vs_w1p                   : 1;
		uint32_t fe1_sw_vs_w1p                   : 1;
	} bits;
};

union reg_isp_line_spliter_hdr_ctrl {
	uint32_t raw;
	struct {
		uint32_t hdr_fe0                         : 1;
		uint32_t hdr_fe1                         : 1;
	} bits;
};

union reg_isp_line_spliter_frame_vld_ctrl {
	uint32_t raw;
	struct {
		uint32_t spliter_frame_vld_fe0           : 1;
		uint32_t spliter_frame_vld_fe1           : 1;
		uint32_t spliter_frame_drop_clr_fe0      : 1;
		uint32_t spliter_frame_drop_clr_fe1      : 1;
		uint32_t spliter_reset_mode_fe0          : 1;
		uint32_t spliter_reset_mode_fe1          : 1;
		uint32_t vs_mode_fe0                     : 1;
		uint32_t vs_mode_fe1                     : 1;
	} bits;
};

union reg_isp_line_spliter_pol_ctrl {
	uint32_t raw;
	struct {
		uint32_t vs_pol_fe0                      : 1;
		uint32_t hs_pol_fe0                      : 1;
		uint32_t vs_pol_fe1                      : 1;
		uint32_t hs_pol_fe1                      : 1;
	} bits;
};

union reg_isp_line_spliter_status {
	uint32_t raw;
	struct {
		uint32_t spliter_frame_drop_fe0_ch0      : 1;
		uint32_t spliter_frame_vld_fe0_ch0       : 1;
		uint32_t spliter_frame_drop_fe0_ch1      : 1;
		uint32_t spliter_frame_vld_fe0_ch1       : 1;
		uint32_t spliter_frame_drop_fe1_ch0      : 1;
		uint32_t spliter_frame_vld_fe1_ch0       : 1;
		uint32_t spliter_frame_drop_fe1_ch1      : 1;
		uint32_t spliter_frame_vld_fe1_ch1       : 1;
	} bits;
};

#ifdef __cplusplus
}
#endif

#endif /* _VI_REG_FIELDS_H_ */

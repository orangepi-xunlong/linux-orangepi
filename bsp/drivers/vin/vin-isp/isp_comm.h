/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * isp_comm.h for all v4l2 subdev manage
 *
 * Copyright (c) 2022 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Ma YiFei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BSP__ISP__COMM__H
#define __BSP__ISP__COMM__H
#include "isp_type.h"
#define USE_ENCPP

#define ISP_REG_TBL_LENGTH 33
#define ISP_REG1_TBL_LENGTH 17
#define ISP_WDR_TBL_SIZE 24
#define ISP_CEM_TBL0_SIZE 0x0cc0
#define ISP_CEM_TBL1_SIZE 0x0a40
#define ISP_CEM_MEM_SIZE (ISP_CEM_TBL0_SIZE + ISP_CEM_TBL1_SIZE)

#define ISP_WDR_SPLIT_CFG_MAX 21
#define ISP_WDR_COMM_CFG_MAX 13
#define ISP_SHARP_MAX 45
#define ENCPP_SHARP_MAX 38
#define ENCODER_DENOISE_MAX 11
#define ISP_DENOISE_MAX 27
#define ISP_BLC_MAX 4
#define ISP_DPC_MAX 8
#define ISP_PLTM_DYNAMIC_MAX 12
#define ISP_CEM_MAX 5
#define ISP_TDF_MAX 35
#define ISP_EXP_CFG_MAX 19
#define ISP_GTM_HEQ_MAX 9
#define ISP_LCA_MAX 11
#define ISP_WDR_CFG_MAX 28
#define ISP_CFA_MAX 3
#define ISP_RAW_CH_MAX 4
#define GTM_LUM_IDX_NUM 9
#define GTM_VAR_IDX_NUM 9

#define ISP_LSC_TEMP_NUM 6
#define ISP_MSC_TEMP_NUM 6
#define ISP_LSC_TBL_LENGTH (3*256)
#define ISP_MSC_TBL_LENGTH (3*484)
#define ISP_MSC_TBL_LUT_SIZE 11
#define ISP_GAMMA_TRIGGER_POINTS 5
#define ISP_GAMMA_TBL_LENGTH (3*1024)
#define ISP_CCM_TEMP_NUM 3
#define ISP_GCA_MAX 9
#define ISP_PLTM_MAX 45
#define ISP_SHARP_COMM_MAX 10
#define ISP_DENOISE_COMM_MAX 33
#define ISP_TDF_COMM_MAX 45
#define ENCPP_SHARP_COMM_MAX 10
#define TEMP_COMP_MAX 12

#define ISP_MSC_TBL_SIZE 484
#define ISP_AE_ROW 18
#define ISP_AE_COL 24

#define PLTM_MAX_STAGE 4
struct isp_rgb2rgb_gain_offset {
	HW_S16 matrix[3][3];
	HW_S16 offset[3];
};

struct encpp_static_sharp_config {
	unsigned char ss_shp_ratio;
	unsigned char ls_shp_ratio;
	unsigned short ss_dir_ratio;
	unsigned short ls_dir_ratio;
	unsigned short ss_crc_stren;
	unsigned char ss_crc_min;
	unsigned short wht_sat_ratio;
	unsigned short blk_sat_ratio;
	unsigned char wht_slp_bt;
	unsigned char blk_slp_bt;

	unsigned short sharp_ss_value[ISP_REG_TBL_LENGTH];
	unsigned short sharp_ls_value[ISP_REG_TBL_LENGTH];
	unsigned short sharp_hsv[46];
};

struct encpp_dynamic_sharp_config {
	unsigned short ss_ns_lw;
	unsigned short ss_ns_hi;
	unsigned short ls_ns_lw;
	unsigned short ls_ns_hi;
	unsigned char ss_lw_cor;
	unsigned char ss_hi_cor;
	unsigned char ls_lw_cor;
	unsigned char ls_hi_cor;
	unsigned short ss_blk_stren;
	unsigned short ss_wht_stren;
	unsigned short ls_blk_stren;
	unsigned short ls_wht_stren;
	unsigned char ss_avg_smth;
	unsigned char ss_dir_smth;
	unsigned char dir_smth[4];
	unsigned char hfr_smth_ratio;
	unsigned short hfr_hf_wht_stren;
	unsigned short hfr_hf_blk_stren;
	unsigned short hfr_mf_wht_stren;
	unsigned short hfr_mf_blk_stren;
	unsigned char hfr_hf_cor_ratio;
	unsigned char hfr_mf_cor_ratio;
	unsigned short hfr_hf_mix_ratio;
	unsigned short hfr_mf_mix_ratio;
	unsigned char hfr_hf_mix_min_ratio;
	unsigned char hfr_mf_mix_min_ratio;
	unsigned short hfr_hf_wht_clp;
	unsigned short hfr_hf_blk_clp;
	unsigned short hfr_mf_wht_clp;
	unsigned short hfr_mf_blk_clp;
	unsigned short wht_clp_para;
	unsigned short blk_clp_para;
	unsigned char wht_clp_slp;
	unsigned char blk_clp_slp;
	unsigned char max_clp_ratio;

	unsigned short sharp_edge_lum[ISP_REG_TBL_LENGTH];
};

struct encoder_3dnr_config {
	unsigned char enable_3d_fliter;
	unsigned char adjust_pix_level_enable; // adjustment of coef pix level enable
	unsigned char smooth_filter_enable;    //* 3x3 smooth filter enable
	unsigned char max_pix_diff_th;         //* range[0~31]: maximum threshold of pixel difference
	unsigned char max_mad_th;              //* range[0~63]: maximum threshold of mad
	unsigned char max_mv_th;               //* range[0~63]: maximum threshold of motion vector
	unsigned char min_coef;                //* range[0~16]: minimum weight of 3d filter
	unsigned char max_coef;                //* range[0~16]: maximum weight of 3d filter,
};

struct encoder_2dnr_config {
	unsigned char enable_2d_fliter;
	unsigned char filter_strength_uv; //* range[0~255], 0 means close 2d filter, advice: 32
	unsigned char filter_strength_y;  //* range[0~255], 0 means close 2d filter, advice: 32
	unsigned char filter_th_uv;       //* range[0~15], advice: 2
	unsigned char filter_th_y;        //* range[0~15], advice: 2
};

struct isp_encpp_cfg_attr_data {
	signed char encpp_en;
	struct encpp_static_sharp_config encpp_static_sharp_cfg;
	struct encpp_dynamic_sharp_config encpp_dynamic_sharp_cfg;
	struct encoder_3dnr_config encoder_3dnr_cfg;
	struct encoder_2dnr_config encoder_2dnr_cfg;
};

typedef enum {
	/*isp_ctrl*/
	ISP_CTRL_MODULE_EN = 0,
	ISP_CTRL_DIGITAL_GAIN,
	ISP_CTRL_PLTMWDR_STR,
	ISP_CTRL_DN_STR,
	ISP_CTRL_3DN_STR,
	ISP_CTRL_HIGH_LIGHT,
	ISP_CTRL_BACK_LIGHT,
	ISP_CTRL_WB_MGAIN,
	ISP_CTRL_AGAIN_DGAIN,
	ISP_CTRL_COLOR_EFFECT,
	ISP_CTRL_AE_ROI,
	ISP_CTRL_AF_METERING,
	ISP_CTRL_COLOR_TEMP,
	ISP_CTRL_EV_IDX,
	ISP_CTRL_MAX_EV_IDX,
	ISP_CTRL_PLTM_HARDWARE_STR,
	ISP_CTRL_ISO_LUM_IDX,
	ISP_CTRL_COLOR_SPACE,
	ISP_CTRL_VENC2ISP_PARAM,
	ISP_CTRL_NPU_NR_PARAM,
	ISP_CTRL_TOTAL_GAIN,
	ISP_CTRL_AE_EV_LV,
	ISP_CTRL_AE_EV_LV_ADJ,
	ISP_CTRL_AE_WEIGHT_LUM,
	ISP_CTRL_AE_LOCK,
	ISP_CTRL_AE_TABLE,
	ISP_CTRL_AE_STATS,
	ISP_CTRL_IR_STATUS,
	ISP_CTRL_IR_AWB_GAIN,
	ISP_CTRL_READ_BIN_PARAM,
	ISP_CTRL_AE_ROI_TARGET,
} hw_isp_ctrl_cfg_ids;

struct ae_table {
	unsigned int min_exp;
	unsigned int max_exp;
	unsigned int min_gain;
	unsigned int max_gain;
	unsigned int min_iris;
	unsigned int max_iris;
};

struct ae_table_info {
	struct ae_table ae_tbl[10];
	int length;
	int ev_step;
	int shutter_shift;
};

enum set_bin_step {
	SET_BIN_TEST_3A_TUNING = 0,
	SET_BIN_ISO_TRIGER = 1,
	SET_BIN_ISO_OTHER = 2,
	SET_BIN_TUNING_UPDATE = 3,
};

#endif

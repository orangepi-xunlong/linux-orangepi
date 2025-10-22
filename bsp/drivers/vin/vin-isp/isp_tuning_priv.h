/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 *
 * Authors:  Yang Feng <yangfeng@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ISP_TUNING_H_
#define _ISP_TUNING_H_
#include "isp_type.h"
#include "isp_comm.h"

struct isp_test_param {
	/*isp test param */
	HW_S8 isp_test_mode;

	HW_S32 isp_test_exptime;
	HW_S32 exp_line_start;
	HW_S32 exp_line_step;
	HW_S32 exp_line_end;
	HW_S8 exp_change_interval;

	HW_S32 isp_test_gain;
	HW_S32 gain_start;
	HW_S32 gain_step;
	HW_S32 gain_end;
	HW_S8 gain_change_interval;

	HW_S32 isp_test_focus;
	HW_S32 focus_start;
	HW_S32 focus_step;
	HW_S32 focus_end;
	HW_S8 focus_change_interval;

	HW_S32 isp_log_param;
	HW_S32 isp_gain;
	HW_S32 isp_exp_line;
	HW_S32 isp_color_temp;
	HW_S32 ae_forced;
	HW_S32 lum_forced;

	/*isp enable param */
	HW_S8 manual_en;
	HW_S8 afs_en;
	HW_S8 ae_en;
	HW_S8 af_en;
	HW_S8 awb_en;
	HW_S8 hist_en;
	HW_S8 wdr_split_en;
	HW_S8 wdr_stitch_en;
	HW_S8 otf_dpc_en;
	HW_S8 ctc_en;
	HW_S8 gca_en;
	HW_S8 nrp_en;
	HW_S8 denoise_en;
	HW_S8 tdf_en;
	HW_S8 blc_en;
	HW_S8 wb_en;
	HW_S8 dig_gain_en;
	HW_S8 lsc_en;
	HW_S8 msc_en;
	HW_S8 pltm_en;
	HW_S8 cfa_en;
	HW_S8 lca_en;
	HW_S8 sharp_en;
	HW_S8 ccm_en;
	HW_S8 defog_en;
	HW_S8 cnr_en;
	HW_S8 drc_en;
	HW_S8 gtm_en;
	HW_S8 gamma_en;
	HW_S8 cem_en;
	HW_S8 encpp_en;
	HW_S8 enc_3dnr_en;
	HW_S8 enc_2dnr_en;
};

struct isp_3a_param {
	/*isp ae param */
	HW_U8 define_ae_table;
	HW_U16 ae_max_lv;
	HW_U8 ae_table_preview_length;
	HW_U8 ae_table_capture_length;
	HW_U8 ae_table_video_length;
	HW_S32 ae_table_preview[42];
	HW_S32 ae_table_capture[42];
	HW_S32 ae_table_video[42];
	HW_S32 ae_win_weight[64];
	HW_S32 ae_gain_favor;
	HW_S32 ae_analog_gain_range[2];
	HW_S32 ae_digital_gain_range[2];
	HW_S32 ae_gain_range[4];
	HW_U8 ae_hist_mod_en;
	HW_U8 ae_hist0_sel;
	HW_U8 ae_hist1_sel;
	HW_U8 ae_stat_sel;
	HW_U8 ae_ev_step;
	HW_U8 ae_ConvDataIndex;
	HW_U8 ae_blowout_pre_en;
	HW_U8 ae_blowout_attr;
	HW_U8 ae_reserve_0;
	HW_U8 ae_reserve_1;
	HW_U8 ae_reserve_2;
	HW_U8 ae_reserve_3;
	HW_U8 ae_reserve_4;
	HW_U8 ae_delay_frame;
	HW_U8 exp_delay_frame;
	HW_U8 gain_delay_frame;
	HW_U8 exp_comp_step;
	HW_U8 ae_touch_dist_ind;
	HW_U8 ae_iso2gain_ratio;
	HW_S32 ae_fno_step[16];
	HW_U16 wdr_split_cfg[ISP_WDR_SPLIT_CFG_MAX];
	HW_U16 wdr_comm_cfg[ISP_WDR_COMM_CFG_MAX];
	HW_S32 gain_ratio;

	/*isp awb param */
	HW_U8 awb_interval;
	HW_U8 awb_speed;
	HW_U8 awb_stat_sel;
	HW_U16 awb_color_temper_low;
	HW_U16 awb_color_temper_high;
	HW_U16 awb_base_temper;
	HW_U16 awb_green_zone_dist;
	HW_U16 awb_blue_sky_dist;
	HW_U8 awb_light_num;
	HW_U8 awb_ext_light_num;
	HW_U8 awb_skin_color_num;
	HW_U8 awb_special_color_num;
	HW_S32 awb_light_info[320];
	HW_S32 awb_ext_light_info[320];
	HW_S32 awb_skin_color_info[160];
	HW_S32 awb_special_color_info[320];
	HW_S32 awb_preset_gain[22];
	HW_S16 awb_rgain_favor;
	HW_S16 awb_bgain_favor;

	/*isp af param */
	HW_U8 af_use_otp;
	HW_S16 vcm_min_code;
	HW_S16 vcm_max_code;
	HW_S16 af_interval_time;
	HW_S16 af_speed_ind; //0~5
	HW_U8 af_auto_fine_en;
	HW_U8 af_single_fine_en;
	HW_U8 af_fine_step;
	HW_U8 af_reserve_0;
	HW_U8 af_reserve_1;
	HW_U8 af_reserve_2;
	HW_U8 af_reserve_3;
	HW_U8 af_move_cnt;
	HW_U8 af_still_cnt;
	HW_U8 af_move_monitor_cnt;
	HW_U8 af_still_monitor_cnt;
	HW_S16 af_stable_min;
	HW_S16 af_stable_max;
	HW_S16 af_low_light_lv;
	HW_U8 af_near_tolerance;
	HW_U8 af_far_tolerance;
	HW_U8 af_tolerance_off;
	HW_S16 af_peak_th;
	HW_S16 af_dir_th;
	HW_S16 af_change_ratio;
	HW_S16 af_move_minus;
	HW_S16 af_still_minus;
	HW_S16 af_scene_motion_th;

	HW_U8 af_tolerance_tbl_len;
	HW_S32 af_std_code_tbl[20];
	HW_S32 af_tolerance_value_tbl[20];
};

struct isp_dynamic_config {
	/*param*/
	HW_S16 sharp_cfg[ISP_SHARP_MAX];
#ifdef USE_ENCPP
	HW_S16 encpp_sharp_cfg[ENCPP_SHARP_MAX];
	HW_S16 encoder_denoise_cfg[ENCODER_DENOISE_MAX];
#endif
	HW_S16 denoise_cfg[ISP_DENOISE_MAX];
	HW_S16 black_level[ISP_BLC_MAX];
	HW_S16 dpc_cfg[ISP_DPC_MAX];
	HW_S16 pltm_dynamic_cfg[ISP_PLTM_DYNAMIC_MAX];
	HW_S16 defog_value;
	HW_S16 brightness;
	HW_S16 contrast;
	HW_S16 cem_cfg[ISP_CEM_MAX];
	HW_S16 tdf_cfg[ISP_TDF_MAX];
	HW_S16 color_denoise;
	HW_S16 ae_cfg[ISP_EXP_CFG_MAX];
	HW_S16 gtm_cfg[ISP_GTM_HEQ_MAX];
	HW_S16 lca_cfg[ISP_LCA_MAX];
	HW_S16 wdr_cfg[ISP_WDR_CFG_MAX];
	HW_S16 cfa_cfg[ISP_CFA_MAX];
	HW_S16 shading_comp;

	/*Curve*/
	HW_S16 d2d_lp0_th[ISP_REG_TBL_LENGTH];
	HW_S16 d2d_lp1_th[ISP_REG_TBL_LENGTH];
	HW_S16 d2d_lp2_th[ISP_REG_TBL_LENGTH];
	HW_S16 d2d_lp3_th[ISP_REG_TBL_LENGTH];
	HW_S16 d3d_flt0_thr_vc[ISP_REG_TBL_LENGTH];
	HW_S16 sharp_edge_lum[ISP_REG_TBL_LENGTH];
#ifdef USE_ENCPP
	HW_S16 encpp_sharp_edge_lum[ISP_REG_TBL_LENGTH];
#endif
};

enum isp_triger_type {
	ISP_TRIGER_BY_LUM_IDX, //ISP_TRIGER_BY_ISO_IDX
	ISP_TRIGER_BY_GAIN_IDX,
	/*ISP_TRIGER_BY_COLOR_TEMPERATURE,*/

	ISP_TRIGER_MAX,
};

typedef struct isp_param_triger {
	enum isp_triger_type sharp_triger;
#ifdef USE_ENCPP
	enum isp_triger_type encpp_sharp_triger;
	enum isp_triger_type encoder_denoise_triger;
#endif
	enum isp_triger_type denoise_triger;
	enum isp_triger_type black_level_triger;
	enum isp_triger_type dpc_triger;
	enum isp_triger_type defog_value_triger;
	enum isp_triger_type pltm_dynamic_triger;
	enum isp_triger_type brightness_triger;
	enum isp_triger_type gcontrast_triger;
	enum isp_triger_type cem_triger;
	enum isp_triger_type tdf_triger;
	enum isp_triger_type color_denoise_triger;
	enum isp_triger_type ae_cfg_triger;
	enum isp_triger_type gtm_cfg_triger;
	enum isp_triger_type lca_cfg_triger;
	enum isp_triger_type wdr_cfg_triger;
	enum isp_triger_type cfa_triger;
	enum isp_triger_type shading_triger;
} isp_dynamic_triger_t;

struct isp_dynamic_param {
	/*isp denoise param */
	isp_dynamic_triger_t triger;
	HW_S32 isp_lum_mapping_point[14];
	HW_S32 isp_gain_mapping_point[14];
	struct isp_dynamic_config isp_dynamic_cfg[14];
};

struct isp_tunning_param {
	HW_S32 flash_gain;
	HW_S32 flash_delay_frame;
	HW_S32 flicker_type;
	HW_S32 flicker_ratio;
	HW_S32 hor_visual_angle;
	HW_S32 ver_visual_angle;
	HW_S32 focus_length;
	HW_S32 gamma_num;
	HW_S32 rolloff_ratio;

	/*isp gtm param */
	HW_S8 gtm_hist_sel;
	HW_S32 gtm_type;
	HW_S32 gamma_type;
	HW_S32 auto_alpha_en;
	HW_S32 hist_pix_cnt;
	HW_S32 dark_minval;
	HW_S32 bright_minval;
	HW_S16 plum_var[GTM_LUM_IDX_NUM][GTM_VAR_IDX_NUM];

	/*cfa param*/
	HW_S16 grad_th;
	HW_S16 dir_v_th;
	HW_S16 dir_h_th;
	HW_S16 res_smth_high;
	HW_S16 res_smth_low;
	HW_S16 res_high_th;
	HW_S16 res_low_th;
	HW_S16 res_dir_a;
	HW_S16 res_dir_d;
	HW_S16 res_dir_v;
	HW_S16 res_dir_h;

	/*dpc*/
	HW_S8 dpc_remove_mode;
	HW_S8 dpc_sup_twinkle_en;

	/*cross talk param*/
	HW_U16 ctc_th_max;
	HW_U16 ctc_th_min;
	HW_U16 ctc_th_slope;
	HW_U16 ctc_dir_wt;
	HW_U16 ctc_dir_th;

	/*isp tune param */
	HW_S32 bayer_gain[ISP_RAW_CH_MAX];

	/*rsc*/
	HW_S8 lsc_mode;
	HW_S8 ff_mod;
	HW_S16 lsc_center_x;
	HW_S16 lsc_center_y;
	HW_U16 lsc_tbl[ISP_LSC_TEMP_NUM +ISP_LSC_TEMP_NUM][ISP_LSC_TBL_LENGTH];
	HW_U16 lsc_trig_cfg[ISP_LSC_TEMP_NUM]; //Color temp trigger points

	/*msc*/
	HW_S8 msc_mode;
	HW_S8 mff_mod;
	HW_S16 msc_blw_lut[ISP_MSC_TBL_LUT_SIZE];
	HW_S16 msc_blh_lut[ISP_MSC_TBL_LUT_SIZE];
	HW_U16 msc_trig_cfg[ISP_MSC_TEMP_NUM]; //Color temp trigger points
	HW_U16 msc_tbl[ISP_MSC_TEMP_NUM+ISP_MSC_TEMP_NUM][ISP_MSC_TBL_LENGTH];

	/*gamma*/
	HW_U16 gamma_tbl_ini[ISP_GAMMA_TRIGGER_POINTS][ISP_GAMMA_TBL_LENGTH]; // form bin file
	HW_U16 gamma_trig_cfg[ISP_GAMMA_TRIGGER_POINTS]; //LV trigger points

	/*ccm*/
	struct isp_rgb2rgb_gain_offset color_matrix_ini[ISP_CCM_TEMP_NUM];
	HW_U16 ccm_trig_cfg[ISP_CCM_TEMP_NUM]; //Color temp trigger points

	/*gca*/
	HW_S16 gca_cfg[ISP_GCA_MAX];

	/*pltm*/
	HW_S32 pltm_cfg[ISP_PLTM_MAX];

	/*sharp*/
	HW_S16 sharp_comm_cfg[ISP_SHARP_COMM_MAX];

	/*denoise*/
	HW_S16 denoise_comm_cfg[ISP_DENOISE_COMM_MAX];

	/*tdf*/
	HW_S16 tdf_comm_cfg[ISP_TDF_COMM_MAX];

#ifdef USE_ENCPP
	/* encpp sharp */
	HW_S16 encpp_sharp_comm_cfg[ENCPP_SHARP_COMM_MAX];
#endif

	/*sensor*/
	HW_S16 sensor_temp[14*TEMP_COMP_MAX]; // 14 * 12

	/*reg table*/
	HW_U8 isp_tdnf_df_shape[ISP_REG1_TBL_LENGTH];
	HW_U8 isp_tdnf_ratio_amp[ISP_REG1_TBL_LENGTH];
	HW_U8 isp_tdnf_k_dlt_bk[ISP_REG1_TBL_LENGTH];
	HW_U8 isp_tdnf_ct_rt_bk[ISP_REG1_TBL_LENGTH];
	HW_U8 isp_tdnf_dtc_hf_bk[ISP_REG1_TBL_LENGTH];
	HW_U8 isp_tdnf_dtc_mf_bk[ISP_REG1_TBL_LENGTH];
	HW_U8 isp_tdnf_lay0_d2d0_rt_br[ISP_REG1_TBL_LENGTH];
	HW_U8 isp_tdnf_lay1_d2d0_rt_br[ISP_REG1_TBL_LENGTH];
	HW_U8 isp_tdnf_lay0_nrd_rt_br[ISP_REG1_TBL_LENGTH];
	HW_U8 isp_tdnf_lay1_nrd_rt_br[ISP_REG1_TBL_LENGTH];
	HW_U8 lca_pf_satu_lut[ISP_REG_TBL_LENGTH];
	HW_U8 lca_gf_satu_lut[ISP_REG_TBL_LENGTH];
	HW_U16 isp_sharp_ss_value[ISP_REG_TBL_LENGTH];
	HW_U16 isp_sharp_ls_value[ISP_REG_TBL_LENGTH];
	HW_U16 isp_sharp_hsv[46];
#ifdef USE_ENCPP
	HW_U16 encpp_sharp_ss_value[ISP_REG_TBL_LENGTH];
	HW_U16 encpp_sharp_ls_value[ISP_REG_TBL_LENGTH];
	HW_U16 encpp_sharp_hsv[46];
#endif
	HW_U8 isp_wdr_de_purpl_hsv_tbl[ISP_WDR_TBL_SIZE];
	HW_U8 isp_pltm_stat_gd_cv[PLTM_MAX_STAGE][15];
	HW_U8 isp_pltm_df_cv[PLTM_MAX_STAGE][ISP_REG_TBL_LENGTH];

	/*dram table*/
	HW_U8 isp_cem_table[ISP_CEM_MEM_SIZE];
	HW_U8 isp_cem_table1[ISP_CEM_MEM_SIZE];
	HW_U8 isp_pltm_lum_map_cv[PLTM_MAX_STAGE][128];
	HW_U16 isp_pltm_gtm_tbl[512];
};

struct isp_param_config {
	struct isp_test_param isp_test_settings;
	struct isp_3a_param isp_3a_settings;
	struct isp_tunning_param isp_tunning_settings;
	struct isp_dynamic_param isp_iso_settings;
};

#endif /*_ISP_TUNING_H_*/


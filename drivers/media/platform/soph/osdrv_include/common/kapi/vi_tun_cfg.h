/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: vi_tun_cfg.h
 * Description:
 */

#ifndef _U_VI_TUN_CFG_H_
#define _U_VI_TUN_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vi_reg_fields.h"

#define TUNING_NODE_NUM  2

enum sop_isp_raw {
	ISP_PRERAW0,
	ISP_PRERAW1,
	ISP_PRERAW2,
	ISP_PRERAW3,
	ISP_PRERAW4,
	ISP_PRERAW5,
	ISP_PRERAW_LITE0,
	ISP_PRERAW_LITE1,
	ISP_PRERAW_MAX,
};

enum sop_isp_fe_chn_num {
	ISP_FE_CH0,
	ISP_FE_CH1,
	ISP_FE_CH2,
	ISP_FE_CH3,
	ISP_FE_CHN_MAX,
};

enum sop_isp_be_chn_num {
	ISP_BE_CH0,
	ISP_BE_CH1,
	ISP_BE_CHN_MAX,
};

struct sop_vip_isp_blc_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  bypass;
	__u16 roffset;
	__u16 groffset;
	__u16 gboffset;
	__u16 boffset;
	__u16 roffset_2nd;
	__u16 groffset_2nd;
	__u16 gboffset_2nd;
	__u16 boffset_2nd;
	__u16 rgain;
	__u16 grgain;
	__u16 gbgain;
	__u16 bgain;
};

struct sop_vip_isp_wbg_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  bypass;
	__u16 rgain;
	__u16 ggain;
	__u16 bgain;
	__u32 rgain_fraction;
	__u32 ggain_fraction;
	__u32 bgain_fraction;
};

/* struct sop_vip_isp_ccm_config
 * @enable: ccm module enable or not
 * @coef: s3.10, (2's complement)
 */
struct sop_vip_isp_ccm_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u16 coef[3][3];
};

struct sop_vip_isp_cacp_config {
	__u8  update;
	__u8  enable;
	__u8  mode;
	__u16 iso_ratio;
	__u16 ca_y_ratio_lut[256];
	__u8  cp_y_lut[256];
	__u8  cp_u_lut[256];
	__u8  cp_v_lut[256];
};

struct sop_vip_isp_ca2_config {
	__u8  update;
	__u8  enable;
	__u16 lut_in[6];
	__u16 lut_out[6];
	__u16 lut_slp[5];
};

struct sop_vip_isp_ygamma_config {
	__u8  update;
	__u8  enable;
	__u32 max;
	__u16 lut[256];
};

struct sop_vip_isp_gamma_config {
	__u8  update;
	__u8  enable;
	__u16 max;
	__u16 lut[256];
};

struct sop_isp_demosiac_tun_cfg {
	union reg_isp_cfa_0c                     reg_0c;
	union reg_isp_cfa_10                     reg_10;
	union reg_isp_cfa_14                     reg_14;
	union reg_isp_cfa_18                     reg_18;
	union reg_isp_cfa_1c                     reg_1c;
};

struct sop_isp_demosiac_tun_1_cfg {
	union reg_isp_cfa_120                   reg_120;
	union reg_isp_cfa_124                   reg_124;
	union reg_isp_cfa_128                   reg_128;
	union reg_isp_cfa_12c                   reg_12c;
	union reg_isp_cfa_130                   reg_130;
	union reg_isp_cfa_134                   reg_134;
	union reg_isp_cfa_138                   reg_138;
	union reg_isp_cfa_13c                   reg_13c;
	union reg_isp_cfa_140                   reg_140;
	union reg_isp_cfa_144                   reg_144;
	union reg_isp_cfa_148                   reg_148;
	union reg_isp_cfa_14c                   reg_14c;
	union reg_isp_cfa_150                   reg_150;
	union reg_isp_cfa_154                   reg_154;
	union reg_isp_cfa_158                   reg_158;
	union reg_isp_cfa_15c                   reg_15c;
	union reg_isp_cfa_160                   reg_160;
	union reg_isp_cfa_164                   reg_164;
	union reg_isp_cfa_168                   reg_168;
	union reg_isp_cfa_16c                   reg_16c;
	union reg_isp_cfa_170                   reg_170;
	union reg_isp_cfa_174                   reg_174;
	union reg_isp_cfa_178                   reg_178;
	union reg_isp_cfa_17c                   reg_17c;
	union reg_isp_cfa_180                   reg_180;
	union reg_isp_cfa_184                   reg_184;
	union reg_isp_cfa_188                   reg_188;
	union reg_isp_cfa_18c                   reg_18c;
	union reg_isp_cfa_190                   reg_190;
};

struct sop_isp_demosiac_tun_2_cfg {
	union reg_isp_cfa_90                    reg_90;
	union reg_isp_cfa_94                    reg_94;
	union reg_isp_cfa_98                    reg_98;
	union reg_isp_cfa_9c                    reg_9c;
	union reg_isp_cfa_a0                    reg_a0;
	union reg_isp_cfa_a4                    reg_a4;
	union reg_isp_cfa_a8                    reg_a8;
};

struct sop_vip_isp_demosiac_config {
	__u8  update;
	__u8  inst;
	__u8  cfa_enable;
	__u16 cfa_edgee_thd2;
	__u8  cfa_out_sel;
	__u8  cfa_force_dir_enable;
	__u8  cfa_force_dir_sel;
	__u16 cfa_rbsig_luma_thd;
	__u8  cfa_ghp_lut[32];
	__u8  cfa_ymoire_enable;
	__u8  cfa_ymoire_dc_w;
	__u8  cfa_ymoire_lpf_w;
	struct sop_isp_demosiac_tun_cfg demosiac_cfg;
	struct sop_isp_demosiac_tun_1_cfg demosiac_1_cfg;
	struct sop_isp_demosiac_tun_2_cfg demosiac_2_cfg;
};

struct sop_vip_isp_lsc_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u16 strength;
	__u8  debug;
	__u8  gain_base;
	__u8  gain_3p9_0_4p8_1;
	__u8  renormalize_enable;
	__u8  gain_bicubic_0_bilinear_1;
	__u8  boundary_interpolation_mode;
	__u8  boundary_interpolation_lf_range;
	__u8  boundary_interpolation_up_range;
	__u8  boundary_interpolation_rt_range;
	__u8  boundary_interpolation_dn_range;
	__u8  bldratio_enable;
	__u16 bldratio;
	__u32 intp_gain_max;
	__u32 intp_gain_min;
};

struct sop_isp_bnr_tun_1_cfg {
	union reg_isp_bnr_ns_luma_th_r          ns_luma_th_r;
	union reg_isp_bnr_ns_slope_r            ns_slope_r;
	union reg_isp_bnr_ns_offset0_r          ns_offset0_r;
	union reg_isp_bnr_ns_offset1_r          ns_offset1_r;
	union reg_isp_bnr_ns_luma_th_gr         ns_luma_th_gr;
	union reg_isp_bnr_ns_slope_gr           ns_slope_gr;
	union reg_isp_bnr_ns_offset0_gr         ns_offset0_gr;
	union reg_isp_bnr_ns_offset1_gr         ns_offset1_gr;
	union reg_isp_bnr_ns_luma_th_gb         ns_luma_th_gb;
	union reg_isp_bnr_ns_slope_gb           ns_slope_gb;
	union reg_isp_bnr_ns_offset0_gb         ns_offset0_gb;
	union reg_isp_bnr_ns_offset1_gb         ns_offset1_gb;
	union reg_isp_bnr_ns_luma_th_b          ns_luma_th_b;
	union reg_isp_bnr_ns_slope_b            ns_slope_b;
	union reg_isp_bnr_ns_offset0_b          ns_offset0_b;
	union reg_isp_bnr_ns_offset1_b          ns_offset1_b;
	union reg_isp_bnr_ns_gain               ns_gain;
	union reg_isp_bnr_strength_mode         strength_mode;
};

struct sop_isp_bnr_tun_2_cfg {
	union reg_isp_bnr_var_th                var_th;
	union reg_isp_bnr_weight_lut            weight_lut;
	union reg_isp_bnr_weight_sm             weight_sm;
	union reg_isp_bnr_weight_v              weight_v;
	union reg_isp_bnr_weight_h              weight_h;
	union reg_isp_bnr_weight_d45            weight_d45;
	union reg_isp_bnr_weight_d135           weight_d135;
	union reg_isp_bnr_neighbor_max          neighbor_max;
};

struct sop_vip_isp_bnr_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  out_sel;
	__u8  weight_lut[256];
	__u8  intensity_sel[8];
	__u8  weight_intra_0;
	__u8  weight_intra_1;
	__u8  weight_intra_2;
	__u8  weight_norm_1;
	__u8  weight_norm_2;
	__u16 k_smooth;
	__u16 k_texture;
	struct sop_isp_bnr_tun_1_cfg bnr_1_cfg;
	struct sop_isp_bnr_tun_2_cfg bnr_2_cfg;
};

struct sop_vip_isp_clut_config {
	__u8  update;
	__u8  enable;
	__u8  is_update_partial;
	__u8  tbl_idx;
	__u16 r_lut[4913];
	__u16 g_lut[4913];
	__u16 b_lut[4913];
	__u16 update_length;
	__u32 lut[1024][2]; //0:addr, 1:value
};

struct sop_isp_drc_tun_1_cfg {
	union reg_ltm_h90                       reg_h90;
	union reg_ltm_h94                       reg_h94;
	union reg_ltm_h98                       reg_h98;
	union reg_ltm_h9c                       reg_h9c;
	union reg_ltm_ha0                       reg_ha0;
	union reg_ltm_ha4                       reg_ha4;
	union reg_ltm_ha8                       reg_ha8;
	union reg_ltm_hac                       reg_hac;
	union reg_ltm_hb0                       reg_hb0;
	union reg_ltm_hb4                       reg_hb4;
	union reg_ltm_hb8                       reg_hb8;
	union reg_ltm_hbc                       reg_hbc;
	union reg_ltm_hc0                       reg_hc0;
	union reg_ltm_hc4                       reg_hc4;
	union reg_ltm_hc8                       reg_hc8;
	union reg_ltm_hcc                       reg_hcc;
	union reg_ltm_hd0                       reg_hd0;
	union reg_ltm_hd4                       reg_hd4;
	union reg_ltm_hd8                       reg_hd8;
	union reg_ltm_hdc                       reg_hdc;
};

struct sop_isp_drc_tun_2_cfg {
	union reg_ltm_h14                       reg_h14;
	union reg_ltm_h18                       reg_h18;
	union reg_ltm_h1c                       reg_h1c;
	union reg_ltm_h20                       reg_h20;
	union reg_ltm_h24                       reg_h24;
	union reg_ltm_h28                       reg_h28;
	union reg_ltm_h2c                       reg_h2c;
	union reg_ltm_h30                       reg_h30;
};

struct sop_isp_drc_tun_3_cfg {
	union reg_ltm_h64                       reg_h64;
	union reg_ltm_h68                       reg_h68;
	union reg_ltm_h6c                       reg_h6c;
	union reg_ltm_h70                       reg_h70;
	union reg_ltm_h74                       reg_h74;
	union reg_ltm_h78                       reg_h78;
	union reg_ltm_h7c                       reg_h7c;
	union reg_ltm_h80                       reg_h80;
	union reg_ltm_h84                       reg_h84;
	union reg_ltm_h88                       reg_h88;
};

struct sop_vip_isp_drc_config {
	__u8  update;
	__u8  ltm_enable;
	__u8  dark_enh_en;
	__u8  brit_enh_en;
	__u8  dbg_mode;
	__u8  dark_tone_wgt_refine_en;
	__u8  brit_tone_wgt_refine_en;
	__u16 global_lut[769];
	__u16 dark_lut[257];
	__u16 brit_lut[513];
	__u8  lmap_enable;
	__u8  lmap_w_bit;
	__u8  lmap_h_bit;
	__u16 lmap_thd_l;
	__u16 lmap_thd_h;
	__u8  lmap_y_mode;
	__u8  de_strth_dshft;
	__u16 de_strth_gain;
	__u8  be_strth_dshft;
	__u16 be_strth_gain;
	struct sop_isp_drc_tun_1_cfg drc_1_cfg;
	struct sop_isp_drc_tun_2_cfg drc_2_cfg;
	struct sop_isp_drc_tun_3_cfg drc_3_cfg;
};

struct sop_isp_ynr_tun_1_cfg {
	union reg_isp_ynr_ns0_luma_th_00        ns0_luma_th_00;
	union reg_isp_ynr_ns0_luma_th_01        ns0_luma_th_01;
	union reg_isp_ynr_ns0_luma_th_02        ns0_luma_th_02;
	union reg_isp_ynr_ns0_luma_th_03        ns0_luma_th_03;
	union reg_isp_ynr_ns0_luma_th_04        ns0_luma_th_04;
	union reg_isp_ynr_ns0_luma_th_05        ns0_luma_th_05;
	union reg_isp_ynr_ns0_slope_00          ns0_slope_00;
	union reg_isp_ynr_ns0_slope_01          ns0_slope_01;
	union reg_isp_ynr_ns0_slope_02          ns0_slope_02;
	union reg_isp_ynr_ns0_slope_03          ns0_slope_03;
	union reg_isp_ynr_ns0_slope_04          ns0_slope_04;
	union reg_isp_ynr_ns0_offset_00         ns0_offset_00;
	union reg_isp_ynr_ns0_offset_01         ns0_offset_01;
	union reg_isp_ynr_ns0_offset_02         ns0_offset_02;
	union reg_isp_ynr_ns0_offset_03         ns0_offset_03;
	union reg_isp_ynr_ns0_offset_04         ns0_offset_04;
	union reg_isp_ynr_ns0_offset_05         ns0_offset_05;
	union reg_isp_ynr_ns1_luma_th_00        ns1_luma_th_00;
	union reg_isp_ynr_ns1_luma_th_01        ns1_luma_th_01;
	union reg_isp_ynr_ns1_luma_th_02        ns1_luma_th_02;
	union reg_isp_ynr_ns1_luma_th_03        ns1_luma_th_03;
	union reg_isp_ynr_ns1_luma_th_04        ns1_luma_th_04;
	union reg_isp_ynr_ns1_luma_th_05        ns1_luma_th_05;
	union reg_isp_ynr_ns1_slope_00          ns1_slope_00;
	union reg_isp_ynr_ns1_slope_01          ns1_slope_01;
	union reg_isp_ynr_ns1_slope_02          ns1_slope_02;
	union reg_isp_ynr_ns1_slope_03          ns1_slope_03;
	union reg_isp_ynr_ns1_slope_04          ns1_slope_04;
	union reg_isp_ynr_ns1_offset_00         ns1_offset_00;
	union reg_isp_ynr_ns1_offset_01         ns1_offset_01;
	union reg_isp_ynr_ns1_offset_02         ns1_offset_02;
	union reg_isp_ynr_ns1_offset_03         ns1_offset_03;
	union reg_isp_ynr_ns1_offset_04         ns1_offset_04;
	union reg_isp_ynr_ns1_offset_05         ns1_offset_05;
	union reg_isp_ynr_ns_gain               ns_gain;
};

struct sop_isp_ynr_tun_2_cfg {
	union reg_isp_ynr_motion_lut_00         motion_lut_00;
	union reg_isp_ynr_motion_lut_01         motion_lut_01;
	union reg_isp_ynr_motion_lut_02         motion_lut_02;
	union reg_isp_ynr_motion_lut_03         motion_lut_03;
	union reg_isp_ynr_motion_lut_04         motion_lut_04;
	union reg_isp_ynr_motion_lut_05         motion_lut_05;
	union reg_isp_ynr_motion_lut_06         motion_lut_06;
	union reg_isp_ynr_motion_lut_07         motion_lut_07;
	union reg_isp_ynr_motion_lut_08         motion_lut_08;
	union reg_isp_ynr_motion_lut_09         motion_lut_09;
	union reg_isp_ynr_motion_lut_10         motion_lut_10;
	union reg_isp_ynr_motion_lut_11         motion_lut_11;
	union reg_isp_ynr_motion_lut_12         motion_lut_12;
	union reg_isp_ynr_motion_lut_13         motion_lut_13;
	union reg_isp_ynr_motion_lut_14         motion_lut_14;
	union reg_isp_ynr_motion_lut_15         motion_lut_15;
};

struct sop_isp_ynr_tun_3_cfg {
	union reg_isp_ynr_alpha_gain            alpha_gain;
	union reg_isp_ynr_var_th                var_th;
	union reg_isp_ynr_weight_sm             weight_sm;
	union reg_isp_ynr_weight_v              weight_v;
	union reg_isp_ynr_weight_h              weight_h;
	union reg_isp_ynr_weight_d45            weight_d45;
	union reg_isp_ynr_weight_d135           weight_d135;
	union reg_isp_ynr_neighbor_max          neighbor_max;
	union reg_isp_ynr_res_k_smooth          res_k_smooth;
	union reg_isp_ynr_res_k_texture         res_k_texture;
	union reg_isp_ynr_filter_mode_en        filter_mode_en;
	union reg_isp_ynr_filter_mode_alpha     filter_mode_alpha;
};

struct sop_isp_ynr_tun_4_cfg {
	union reg_isp_ynr_res_mot_lut_00        res_mot_lut_00;
	union reg_isp_ynr_res_mot_lut_01        res_mot_lut_01;
	union reg_isp_ynr_res_mot_lut_02        res_mot_lut_02;
	union reg_isp_ynr_res_mot_lut_03        res_mot_lut_03;
	union reg_isp_ynr_res_mot_lut_04        res_mot_lut_04;
	union reg_isp_ynr_res_mot_lut_05        res_mot_lut_05;
	union reg_isp_ynr_res_mot_lut_06        res_mot_lut_06;
	union reg_isp_ynr_res_mot_lut_07        res_mot_lut_07;
	union reg_isp_ynr_res_mot_lut_08        res_mot_lut_08;
	union reg_isp_ynr_res_mot_lut_09        res_mot_lut_09;
	union reg_isp_ynr_res_mot_lut_10        res_mot_lut_10;
	union reg_isp_ynr_res_mot_lut_11        res_mot_lut_11;
	union reg_isp_ynr_res_mot_lut_12        res_mot_lut_12;
	union reg_isp_ynr_res_mot_lut_13        res_mot_lut_13;
	union reg_isp_ynr_res_mot_lut_14        res_mot_lut_14;
	union reg_isp_ynr_res_mot_lut_15        res_mot_lut_15;
};

struct sop_vip_isp_ynr_config {
	__u8  update;
	__u8  enable;
	__u8  out_sel;
	__u8  weight_intra_0;
	__u8  weight_intra_1;
	__u8  weight_intra_2;
	__u8  weight_norm_1;
	__u8  weight_norm_2;
	__u8  res_max;
	__u8  res_motion_max;
	__u8  motion_ns_clip_max;
	__u8  weight_lut_h[64];
	struct sop_isp_ynr_tun_1_cfg ynr_1_cfg;
	struct sop_isp_ynr_tun_2_cfg ynr_2_cfg;
	struct sop_isp_ynr_tun_3_cfg ynr_3_cfg;
	struct sop_isp_ynr_tun_4_cfg ynr_4_cfg;
};

struct sop_vip_isp_cnr_config {
	__u8  update;
	__u8  enable;
	__u8  strength_mode;
	__u8  diff_shift_val;
	__u8  diff_gain;
	__u8  ratio;
	__u8  fusion_intensity_weight;
	__u8  flag_neighbor_max_weight;
	__u8  weight_lut_inter[16];
	__u8  motion_enable;
	__u8  coring_motion_lut[16];
	__u8  motion_lut[16];
};

struct sop_isp_tnr_tun_cfg {
	union reg_isp_mmap_0c                     reg_0c;
	union reg_isp_mmap_10                     reg_10;
	union reg_isp_mmap_14                     reg_14;
	union reg_isp_mmap_18                     reg_18;
};

struct sop_isp_tnr_tun_5_cfg {
	union reg_isp_mmap_20                     reg_20;
	union reg_isp_mmap_24                     reg_24;
	union reg_isp_mmap_28                     reg_28;
	union reg_isp_mmap_2c                     reg_2c;
};

struct sop_isp_tnr_tun_1_cfg {
	union reg_isp_mmap_4c                     reg_4c;
	union reg_isp_mmap_50                     reg_50;
	union reg_isp_mmap_54                     reg_54;
	union reg_isp_mmap_58                     reg_58;
	union reg_isp_mmap_5c                     reg_5c;
};

struct sop_isp_tnr_tun_2_cfg {
	union reg_isp_mmap_70                     reg_70;
	union reg_isp_mmap_74                     reg_74;
	union reg_isp_mmap_78                     reg_78;
	union reg_isp_mmap_7c                     reg_7c;
	union reg_isp_mmap_80                     reg_80;
	union reg_isp_mmap_84                     reg_84;
	union reg_isp_mmap_88                     reg_88;
	union reg_isp_mmap_8c                     reg_8c;
	union reg_isp_mmap_90                     reg_90;
};

struct sop_isp_tnr_tun_3_cfg {
	union reg_isp_mmap_a0                     reg_a0;
	union reg_isp_mmap_a4                     reg_a4;
	union reg_isp_mmap_a8                     reg_a8;
	union reg_isp_mmap_ac                     reg_ac;
	union reg_isp_mmap_b0                     reg_b0;
	union reg_isp_mmap_b4                     reg_b4;
	union reg_isp_mmap_b8                     reg_b8;
	union reg_isp_mmap_bc                     reg_bc;
	union reg_isp_mmap_c0                     reg_c0;
};

struct sop_isp_tnr_tun_4_cfg {
	union reg_isp_444_422_13                reg_13;
	union reg_isp_444_422_14                reg_14;
	union reg_isp_444_422_15                reg_15;
	union reg_isp_444_422_16                reg_16;
	union reg_isp_444_422_17                reg_17;
	union reg_isp_444_422_18                reg_18;
	union reg_isp_444_422_19                reg_19;
	union reg_isp_444_422_20                reg_20;
	union reg_isp_444_422_21                reg_21;
	union reg_isp_444_422_22                reg_22;
	union reg_isp_444_422_23                reg_23;
	union reg_isp_444_422_24                reg_24;
	union reg_isp_444_422_25                reg_25;
	union reg_isp_444_422_26                reg_26;
	union reg_isp_444_422_27                reg_27;
	union reg_isp_444_422_28                reg_28;
	union reg_isp_444_422_29                reg_29;
	union reg_isp_444_422_30                reg_30;
	union reg_isp_444_422_31                reg_31;
};

struct sop_isp_tnr_tun_6_cfg {
	union reg_isp_444_422_84                reg_84;
	union reg_isp_444_422_88                reg_88;
	union reg_isp_444_422_8c                reg_8c;
	union reg_isp_444_422_90                reg_90;
	union reg_isp_444_422_94                reg_94;
	union reg_isp_444_422_98                reg_98;
	union reg_isp_444_422_9c                reg_9c;
	union reg_isp_444_422_a0                reg_a0;
	union reg_isp_444_422_a4                reg_a4;
	union reg_isp_444_422_a8                reg_a8;
	union reg_isp_444_422_ac                reg_ac;
	union reg_isp_444_422_b0                reg_b0;
	union reg_isp_444_422_b4                reg_b4;
	union reg_isp_444_422_b8                reg_b8;
	union reg_isp_444_422_bc                reg_bc;
	union reg_isp_444_422_c0                reg_c0;
	union reg_isp_444_422_c4                reg_c4;
	union reg_isp_444_422_c8                reg_c8;
	union reg_isp_444_422_cc                reg_cc;
	union reg_isp_444_422_d0                reg_d0;
	union reg_isp_444_422_d4                reg_d4;
	union reg_isp_444_422_d8                reg_d8;
	union reg_isp_444_422_dc                reg_dc;
	union reg_isp_444_422_e0                reg_e0;
	union reg_isp_444_422_e4                reg_e4;
	union reg_isp_444_422_e8                reg_e8;
	union reg_isp_444_422_ec                reg_ec;
	union reg_isp_444_422_f0                reg_f0;
	union reg_isp_444_422_f4                reg_f4;
	union reg_isp_444_422_f8                reg_f8;
	union reg_isp_444_422_fc                reg_fc;
};

struct sop_isp_tnr_tun_7_cfg {
	union reg_isp_mmap_100                  reg_100;
	union reg_isp_mmap_104                  reg_104;
	union reg_isp_mmap_108                  reg_108;
	union reg_isp_mmap_10c                  reg_10c;
	union reg_isp_mmap_110                  reg_110;
	union reg_isp_mmap_114                  reg_114;
	union reg_isp_mmap_118                  reg_118;
	union reg_isp_mmap_11c                  reg_11c;
	union reg_isp_mmap_120                  reg_120;
	union reg_isp_mmap_124                  reg_124;
	union reg_isp_mmap_128                  reg_128;
};

struct sop_vip_isp_tnr_config {
	__u8  update;
	__u8  manr_enable;
	__u8  rgbmap_w_bit;
	__u8  rgbmap_h_bit;
	__u8  mh_wgt;
	__u8  lpf[3][3];
	__u8  map_gain;
	__u8  map_thd_l;
	__u8  map_thd_h;
	__u8  uv_rounding_type_sel;
	__u8  history_sel_0;
	__u8  history_sel_1;
	__u8  history_sel_3;
	__u16 tdnr_debug_sel;
	__s16 luma_adapt_lut_slope_2;
	__u8  med_enable;
	__u16 med_wgt;
	__u8  mtluma_mode;
	__u8  avg_mode_write;
	__u8  drop_mode_write;
	__u8  tdnr_pixel_lp;
	__u8  tdnr_comp_gain_enable;
	__u16 tdnr_ee_comp_gain;
	struct sop_isp_tnr_tun_cfg      tnr_cfg;
	struct sop_isp_tnr_tun_1_cfg    tnr_1_cfg;
	struct sop_isp_tnr_tun_2_cfg    tnr_2_cfg;
	struct sop_isp_tnr_tun_3_cfg    tnr_3_cfg;
	struct sop_isp_tnr_tun_4_cfg    tnr_4_cfg;
	struct sop_isp_tnr_tun_5_cfg    tnr_5_cfg;
	struct sop_isp_tnr_tun_6_cfg    tnr_6_cfg;
	struct sop_isp_tnr_tun_7_cfg    tnr_7_cfg;
};

struct sop_isp_ee_tun_1_cfg {
	union reg_isp_ee_a4                     reg_a4;
	union reg_isp_ee_a8                     reg_a8;
	union reg_isp_ee_ac                     reg_ac;
	union reg_isp_ee_b0                     reg_b0;
	union reg_isp_ee_b4                     reg_b4;
	union reg_isp_ee_b8                     reg_b8;
	union reg_isp_ee_bc                     reg_bc;
	union reg_isp_ee_c0                     reg_c0;
	union reg_isp_ee_c4                     reg_c4;
	union reg_isp_ee_c8                     reg_c8;
	union reg_isp_ee_hcc                    reg_hcc;
	union reg_isp_ee_hd0                    reg_hd0;
};

struct sop_isp_ee_tun_2_cfg {
	union reg_isp_ee_19c                    reg_19c;
	union reg_isp_ee_1a0                    reg_1a0;
	union reg_isp_ee_1a4                    reg_1a4;
	union reg_isp_ee_1a8                    reg_1a8;
};

struct sop_isp_ee_tun_3_cfg {
	union reg_isp_ee_1c4                    reg_1c4;
	union reg_isp_ee_1c8                    reg_1c8;
	union reg_isp_ee_1cc                    reg_1cc;
	union reg_isp_ee_1d0                    reg_1d0;
	union reg_isp_ee_1d4                    reg_1d4;
	union reg_isp_ee_1d8                    reg_1d8;
};

struct sop_isp_ee_tun_4_cfg {
	union reg_isp_ee_1dc                    reg_1dc;
	union reg_isp_ee_1e0                    reg_1e0;
	union reg_isp_ee_1e4                    reg_1e4;
	union reg_isp_ee_1e8                    reg_1e8;
	union reg_isp_ee_1ec                    reg_1ec;
	union reg_isp_ee_1f0                    reg_1f0;
	union reg_isp_ee_1f4                    reg_1f4;
	union reg_isp_ee_1f8                    reg_1f8;
};

struct sop_vip_isp_ee_config {
	__u8  update;
	__u8  enable;
	__u8  dbg_mode;
	__u8  total_coring;
	__u8  total_motion_coring;
	__u8  total_gain;
	__u8  total_oshtthrd;
	__u8  total_ushtthrd;
	__u8  pre_proc_enable;
	__u8  lumaref_lpf_en;
	__u8  luma_coring_en;
	__u8  luma_adptctrl_en;
	__u8  delta_adptctrl_en;
	__u8  delta_adptctrl_shift;
	__u8  chromaref_lpf_en;
	__u8  chroma_adptctrl_en;
	__u8  mf_core_gain;
	__u8  hf_blend_wgt;
	__u8  mf_blend_wgt;
	__u8  soft_clamp_enable;
	__u8  upper_bound_left_diff;
	__u8  lower_bound_right_diff;
	__u8  luma_adptctrl_lut[33];
	__u8  delta_adptctrl_lut[33];
	__u8  chroma_adptctrl_lut[33];
	struct sop_isp_ee_tun_1_cfg ee_1_cfg;
	struct sop_isp_ee_tun_2_cfg ee_2_cfg;
	struct sop_isp_ee_tun_3_cfg ee_3_cfg;
	struct sop_isp_ee_tun_4_cfg ee_4_cfg;
};

struct sop_isp_preyee_tun_1_cfg {
	union reg_isp_preyee_a4                     reg_a4;
	union reg_isp_preyee_a8                     reg_a8;
	union reg_isp_preyee_ac                     reg_ac;
	union reg_isp_preyee_b0                     reg_b0;
	union reg_isp_preyee_b4                     reg_b4;
	union reg_isp_preyee_b8                     reg_b8;
	union reg_isp_preyee_bc                     reg_bc;
	union reg_isp_preyee_c0                     reg_c0;
	union reg_isp_preyee_c4                     reg_c4;
	union reg_isp_preyee_c8                     reg_c8;
	union reg_isp_preyee_hcc                    reg_hcc;
	union reg_isp_preyee_hd0                    reg_hd0;
};

struct sop_isp_preyee_tun_2_cfg {
	union reg_isp_preyee_19c                    reg_19c;
	union reg_isp_preyee_1a0                    reg_1a0;
	union reg_isp_preyee_1a4                    reg_1a4;
	union reg_isp_preyee_1a8                    reg_1a8;
};

struct sop_isp_preyee_tun_3_cfg {
	union reg_isp_preyee_1c4                    reg_1c4;
	union reg_isp_preyee_1c8                    reg_1c8;
	union reg_isp_preyee_1cc                    reg_1cc;
	union reg_isp_preyee_1d0                    reg_1d0;
	union reg_isp_preyee_1d4                    reg_1d4;
	union reg_isp_preyee_1d8                    reg_1d8;
};

struct sop_isp_preyee_tun_4_cfg {
	union reg_isp_preyee_1dc                    reg_1dc;
	union reg_isp_preyee_1e0                    reg_1e0;
	union reg_isp_preyee_1e4                    reg_1e4;
	union reg_isp_preyee_1e8                    reg_1e8;
	union reg_isp_preyee_1ec                    reg_1ec;
	union reg_isp_preyee_1f0                    reg_1f0;
};

struct sop_isp_preyee_tun_5_cfg {
	union reg_isp_preyee_200                    reg_200;
	union reg_isp_preyee_204                    reg_204;
	union reg_isp_preyee_208                    reg_208;
	union reg_isp_preyee_20c                    reg_20c;
	union reg_isp_preyee_210                    reg_210;
	union reg_isp_preyee_214                    reg_214;
	union reg_isp_preyee_218                    reg_218;
	union reg_isp_preyee_21c                    reg_21c;
	union reg_isp_preyee_220                    reg_220;
};

struct sop_vip_isp_pre_ee_config {
	__u8  update;
	__u8  enable;
	__u8  dbg_mode;
	__u8  total_coring;
	__u8  total_motion_coring;
	__u8  total_gain;
	__u8  total_oshtthrd;
	__u8  total_ushtthrd;
	__u8  pre_proc_enable;
	__u8  lumaref_lpf_en;
	__u8  luma_coring_en;
	__u8  luma_adptctrl_en;
	__u8  delta_adptctrl_en;
	__u8  delta_adptctrl_shift;
	__u8  chromaref_lpf_en;
	__u8  chroma_adptctrl_en;
	__u8  mf_core_gain;
	__u8  hf_blend_wgt;
	__u8  mf_blend_wgt;
	__u8  soft_clamp_enable;
	__u8  upper_bound_left_diff;
	__u8  lower_bound_right_diff;
	__u8  luma_adptctrl_lut[33];
	__u8  delta_adptctrl_lut[33];
	__u8  chroma_adptctrl_lut[33];
	struct sop_isp_preyee_tun_1_cfg pre_ee_1_cfg;
	struct sop_isp_preyee_tun_2_cfg pre_ee_2_cfg;
	struct sop_isp_preyee_tun_3_cfg pre_ee_3_cfg;
	struct sop_isp_preyee_tun_4_cfg pre_ee_4_cfg;
	struct sop_isp_preyee_tun_5_cfg pre_ee_5_cfg;
};

struct sop_isp_fswdr_tun_cfg {
	union reg_fusion_fs_se_gain             fs_se_gain;
	union reg_fusion_fs_luma_thd            fs_luma_thd;
	union reg_fusion_fs_wgt                 fs_wgt;
	union reg_fusion_fs_wgt_slope           fs_wgt_slope;
};

struct sop_isp_fswdr_tun_2_cfg {
	union reg_fusion_fs_motion_lut_in       fs_motion_lut_in;
	union reg_fusion_fs_motion_lut_out_0    fs_motion_lut_out_0;
	union reg_fusion_fs_motion_lut_out_1    fs_motion_lut_out_1;
	union reg_fusion_fs_motion_lut_slope_0  fs_motion_lut_slope_0;
	union reg_fusion_fs_motion_lut_slope_1  fs_motion_lut_slope_1;
};

struct sop_isp_fswdr_tun_3_cfg {
	union reg_fusion_fs_calib_ctrl_0        fs_calib_ctrl_0;
	union reg_fusion_fs_calib_ctrl_1        fs_calib_ctrl_1;
	union reg_fusion_fs_se_fix_offset_0     fs_se_fix_offset_0;
	union reg_fusion_fs_se_fix_offset_1     fs_se_fix_offset_1;
	union reg_fusion_fs_se_fix_offset_2     fs_se_fix_offset_2;
	union reg_fusion_fs_calib_out_0         fs_calib_out_0;
	union reg_fusion_fs_calib_out_1         fs_calib_out_1;
	union reg_fusion_fs_calib_out_2         fs_calib_out_2;
	union reg_fusion_fs_calib_out_3         fs_calib_out_3;
	union reg_fusion_fs_lmap_dark_thd       fs_lmap_dark_thd;
	union reg_fusion_fs_lmap_dark_wgt       fs_lmap_dark_wgt;
	union reg_fusion_fs_lmap_dark_wgt_slope  fs_lmap_dark_wgt_slope;
	union reg_fusion_fs_lmap_brit_thd       fs_lmap_brit_thd;
	union reg_fusion_fs_lmap_brit_wgt       fs_lmap_brit_wgt;
	union reg_fusion_fs_lmap_brit_wgt_slope  fs_lmap_brit_wgt_slope;
};

struct sop_vip_isp_fswdr_config {
	__u8  update;
	__u8  enable;
	__u8  mc_enable;
	__u8  dc_mode;
	__u8  luma_mode;
	__u8  lmap_guide_dc_mode;
	__u8  lmap_guide_luma_mode;
	__u32 s_max;
	__u8  fusion_type;
	__u16 fusion_lwgt;
	__u8  motion_ls_mode;
	__u8  mmap_mrg_mode;
	__u8  mmap_1_enable;
	__u8  motion_ls_sel;
	__u8  mmap_mrg_alph;
	__u8  history_sel_2;
	__u16 mmap_v_thd_l;
	__u16 mmap_v_thd_h;
	__u16 mmap_v_wgt_min;
	__u16 mmap_v_wgt_max;
	__s32 mmap_v_wgt_slp;
	__u8  le_in_sel;
	__u8  se_in_sel;
	struct sop_isp_fswdr_tun_cfg fswdr_cfg;
	struct sop_isp_fswdr_tun_2_cfg fswdr_2_cfg;
	struct sop_isp_fswdr_tun_3_cfg fswdr_3_cfg;
};

struct sop_vip_isp_fswdr_report {
	uint32_t cal_pix_num;
	int32_t diff_sum_r;
	int32_t diff_sum_g;
	int32_t diff_sum_b;
};

struct sop_vip_isp_ldci_tun_1_cfg {
	union reg_isp_ldci_luma_wgt_max         ldci_luma_wgt_max;
	union reg_isp_ldci_idx_iir_alpha        ldci_idx_iir_alpha;
	union reg_isp_ldci_edge_scale           ldci_edge_scale;
	union reg_isp_ldci_edge_clamp           ldci_edge_clamp;
	union reg_isp_ldci_idx_filter_norm      ldci_idx_filter_norm;
	union reg_isp_ldci_tone_curve_idx_00    ldci_tone_curve_idx_00;
};

struct sop_vip_isp_ldci_tun_2_cfg {
	union reg_isp_ldci_blk_size_x           ldci_blk_size_x;
	union reg_isp_ldci_blk_size_x1          ldci_blk_size_x1;
	union reg_isp_ldci_subblk_size_x        ldci_subblk_size_x;
	union reg_isp_ldci_subblk_size_x1       ldci_subblk_size_x1;
	union reg_isp_ldci_interp_norm_lr       ldci_interp_norm_lr;
	union reg_isp_ldci_sub_interp_norm_lr   ldci_sub_interp_norm_lr;
	union reg_isp_ldci_mean_norm_x          ldci_mean_norm_x;
	union reg_isp_ldci_var_norm_y           ldci_var_norm_y;
	union reg_isp_ldci_uv_gain_max          ldci_uv_gain_max;
};

struct sop_vip_isp_ldci_tun_3_cfg {
	union reg_isp_ldci_idx_filter_lut_00    ldci_idx_filter_lut_00;
	union reg_isp_ldci_idx_filter_lut_02    ldci_idx_filter_lut_02;
	union reg_isp_ldci_idx_filter_lut_04    ldci_idx_filter_lut_04;
	union reg_isp_ldci_idx_filter_lut_06    ldci_idx_filter_lut_06;
	union reg_isp_ldci_idx_filter_lut_08    ldci_idx_filter_lut_08;
	union reg_isp_ldci_idx_filter_lut_10    ldci_idx_filter_lut_10;
	union reg_isp_ldci_idx_filter_lut_12    ldci_idx_filter_lut_12;
	union reg_isp_ldci_idx_filter_lut_14    ldci_idx_filter_lut_14;
	union reg_isp_ldci_interp_norm_lr1      ldci_interp_norm_lr1;
	union reg_isp_ldci_sub_interp_norm_lr1  ldci_sub_interp_norm_lr1;
	union reg_isp_ldci_tone_curve_lut_00_00  ldci_tone_curve_lut_00_00;
	union reg_isp_ldci_tone_curve_lut_00_02  ldci_tone_curve_lut_00_02;
	union reg_isp_ldci_tone_curve_lut_00_04  ldci_tone_curve_lut_00_04;
	union reg_isp_ldci_tone_curve_lut_00_06  ldci_tone_curve_lut_00_06;
	union reg_isp_ldci_tone_curve_lut_00_08  ldci_tone_curve_lut_00_08;
	union reg_isp_ldci_tone_curve_lut_00_10  ldci_tone_curve_lut_00_10;
	union reg_isp_ldci_tone_curve_lut_00_12  ldci_tone_curve_lut_00_12;
	union reg_isp_ldci_tone_curve_lut_00_14  ldci_tone_curve_lut_00_14;
	union reg_isp_ldci_tone_curve_lut_01_00  ldci_tone_curve_lut_01_00;
	union reg_isp_ldci_tone_curve_lut_01_02  ldci_tone_curve_lut_01_02;
	union reg_isp_ldci_tone_curve_lut_01_04  ldci_tone_curve_lut_01_04;
	union reg_isp_ldci_tone_curve_lut_01_06  ldci_tone_curve_lut_01_06;
	union reg_isp_ldci_tone_curve_lut_01_08  ldci_tone_curve_lut_01_08;
	union reg_isp_ldci_tone_curve_lut_01_10  ldci_tone_curve_lut_01_10;
	union reg_isp_ldci_tone_curve_lut_01_12  ldci_tone_curve_lut_01_12;
	union reg_isp_ldci_tone_curve_lut_01_14  ldci_tone_curve_lut_01_14;
	union reg_isp_ldci_tone_curve_lut_02_00  ldci_tone_curve_lut_02_00;
	union reg_isp_ldci_tone_curve_lut_02_02  ldci_tone_curve_lut_02_02;
	union reg_isp_ldci_tone_curve_lut_02_04  ldci_tone_curve_lut_02_04;
	union reg_isp_ldci_tone_curve_lut_02_06  ldci_tone_curve_lut_02_06;
	union reg_isp_ldci_tone_curve_lut_02_08  ldci_tone_curve_lut_02_08;
	union reg_isp_ldci_tone_curve_lut_02_10  ldci_tone_curve_lut_02_10;
	union reg_isp_ldci_tone_curve_lut_02_12  ldci_tone_curve_lut_02_12;
	union reg_isp_ldci_tone_curve_lut_02_14  ldci_tone_curve_lut_02_14;
	union reg_isp_ldci_tone_curve_lut_03_00  ldci_tone_curve_lut_03_00;
	union reg_isp_ldci_tone_curve_lut_03_02  ldci_tone_curve_lut_03_02;
	union reg_isp_ldci_tone_curve_lut_03_04  ldci_tone_curve_lut_03_04;
	union reg_isp_ldci_tone_curve_lut_03_06  ldci_tone_curve_lut_03_06;
	union reg_isp_ldci_tone_curve_lut_03_08  ldci_tone_curve_lut_03_08;
	union reg_isp_ldci_tone_curve_lut_03_10  ldci_tone_curve_lut_03_10;
	union reg_isp_ldci_tone_curve_lut_03_12  ldci_tone_curve_lut_03_12;
	union reg_isp_ldci_tone_curve_lut_03_14  ldci_tone_curve_lut_03_14;
	union reg_isp_ldci_tone_curve_lut_04_00  ldci_tone_curve_lut_04_00;
	union reg_isp_ldci_tone_curve_lut_04_02  ldci_tone_curve_lut_04_02;
	union reg_isp_ldci_tone_curve_lut_04_04  ldci_tone_curve_lut_04_04;
	union reg_isp_ldci_tone_curve_lut_04_06  ldci_tone_curve_lut_04_06;
	union reg_isp_ldci_tone_curve_lut_04_08  ldci_tone_curve_lut_04_08;
	union reg_isp_ldci_tone_curve_lut_04_10  ldci_tone_curve_lut_04_10;
	union reg_isp_ldci_tone_curve_lut_04_12  ldci_tone_curve_lut_04_12;
	union reg_isp_ldci_tone_curve_lut_04_14  ldci_tone_curve_lut_04_14;
	union reg_isp_ldci_tone_curve_lut_05_00  ldci_tone_curve_lut_05_00;
	union reg_isp_ldci_tone_curve_lut_05_02  ldci_tone_curve_lut_05_02;
	union reg_isp_ldci_tone_curve_lut_05_04  ldci_tone_curve_lut_05_04;
	union reg_isp_ldci_tone_curve_lut_05_06  ldci_tone_curve_lut_05_06;
	union reg_isp_ldci_tone_curve_lut_05_08  ldci_tone_curve_lut_05_08;
	union reg_isp_ldci_tone_curve_lut_05_10  ldci_tone_curve_lut_05_10;
	union reg_isp_ldci_tone_curve_lut_05_12  ldci_tone_curve_lut_05_12;
	union reg_isp_ldci_tone_curve_lut_05_14  ldci_tone_curve_lut_05_14;
	union reg_isp_ldci_tone_curve_lut_06_00  ldci_tone_curve_lut_06_00;
	union reg_isp_ldci_tone_curve_lut_06_02  ldci_tone_curve_lut_06_02;
	union reg_isp_ldci_tone_curve_lut_06_04  ldci_tone_curve_lut_06_04;
	union reg_isp_ldci_tone_curve_lut_06_06  ldci_tone_curve_lut_06_06;
	union reg_isp_ldci_tone_curve_lut_06_08  ldci_tone_curve_lut_06_08;
	union reg_isp_ldci_tone_curve_lut_06_10  ldci_tone_curve_lut_06_10;
	union reg_isp_ldci_tone_curve_lut_06_12  ldci_tone_curve_lut_06_12;
	union reg_isp_ldci_tone_curve_lut_06_14  ldci_tone_curve_lut_06_14;
	union reg_isp_ldci_tone_curve_lut_07_00  ldci_tone_curve_lut_07_00;
	union reg_isp_ldci_tone_curve_lut_07_02  ldci_tone_curve_lut_07_02;
	union reg_isp_ldci_tone_curve_lut_07_04  ldci_tone_curve_lut_07_04;
	union reg_isp_ldci_tone_curve_lut_07_06  ldci_tone_curve_lut_07_06;
	union reg_isp_ldci_tone_curve_lut_07_08  ldci_tone_curve_lut_07_08;
	union reg_isp_ldci_tone_curve_lut_07_10  ldci_tone_curve_lut_07_10;
	union reg_isp_ldci_tone_curve_lut_07_12  ldci_tone_curve_lut_07_12;
	union reg_isp_ldci_tone_curve_lut_07_14  ldci_tone_curve_lut_07_14;
	union reg_isp_ldci_tone_curve_lut_p_00  ldci_tone_curve_lut_p_00;
	union reg_isp_ldci_tone_curve_lut_p_02  ldci_tone_curve_lut_p_02;
	union reg_isp_ldci_tone_curve_lut_p_04  ldci_tone_curve_lut_p_04;
	union reg_isp_ldci_tone_curve_lut_p_06  ldci_tone_curve_lut_p_06;
	union reg_isp_ldci_tone_curve_lut_p_08  ldci_tone_curve_lut_p_08;
	union reg_isp_ldci_tone_curve_lut_p_10  ldci_tone_curve_lut_p_10;
	union reg_isp_ldci_tone_curve_lut_p_12  ldci_tone_curve_lut_p_12;
	union reg_isp_ldci_tone_curve_lut_p_14  ldci_tone_curve_lut_p_14;
};

struct sop_vip_isp_ldci_tun_4_cfg {
	union reg_isp_ldci_luma_wgt_lut_00      ldci_luma_wgt_lut_00;
	union reg_isp_ldci_luma_wgt_lut_04      ldci_luma_wgt_lut_04;
	union reg_isp_ldci_luma_wgt_lut_08      ldci_luma_wgt_lut_08;
	union reg_isp_ldci_luma_wgt_lut_12      ldci_luma_wgt_lut_12;
	union reg_isp_ldci_luma_wgt_lut_16      ldci_luma_wgt_lut_16;
	union reg_isp_ldci_luma_wgt_lut_20      ldci_luma_wgt_lut_20;
	union reg_isp_ldci_luma_wgt_lut_24      ldci_luma_wgt_lut_24;
	union reg_isp_ldci_luma_wgt_lut_28      ldci_luma_wgt_lut_28;
	union reg_isp_ldci_luma_wgt_lut_32      ldci_luma_wgt_lut_32;
};

struct sop_vip_isp_ldci_tun_5_cfg {
	union reg_isp_ldci_var_filter_lut_00    ldci_var_filter_lut_00;
	union reg_isp_ldci_var_filter_lut_02    ldci_var_filter_lut_02;
	union reg_isp_ldci_var_filter_lut_04    ldci_var_filter_lut_04;
};

struct sop_vip_isp_ldci_config {
	__u8  update;
	__u8  enable;
	__u8  stats_enable;
	__u8  map_enable;
	__u8  uv_gain_enable;
	__u8  first_frame_enable;
	__u8  image_size_div_by_16x12;
	__u16 strength;
	struct sop_vip_isp_ldci_tun_1_cfg ldci_1_cfg;
	struct sop_vip_isp_ldci_tun_2_cfg ldci_2_cfg;
	struct sop_vip_isp_ldci_tun_3_cfg ldci_3_cfg;
	struct sop_vip_isp_ldci_tun_4_cfg ldci_4_cfg;
	struct sop_vip_isp_ldci_tun_5_cfg ldci_5_cfg;
};

struct sop_vip_isp_ycur_config {
	__u8  update;
	__u8  enable;
	__u8  lut[64];
	__u16 lut_256;
};

struct sop_vip_isp_dci_config {
	__u8  update;
	__u8  enable;
	__u8  map_enable;
	__u8  demo_mode;
	__u16 map_lut[256];
	__u8  per1sample_enable;
	__u8  hist_enable;
};

struct sop_vip_isp_dhz_luma_tun_cfg {
	union reg_isp_dehaze_9  luma_00;
	union reg_isp_dehaze_10 luma_04;
	union reg_isp_dehaze_11 luma_08;
	union reg_isp_dehaze_12 luma_12;
};

struct sop_vip_isp_dhz_skin_tun_cfg {
	union reg_isp_dehaze_17 skin_00;
	union reg_isp_dehaze_18 skin_04;
	union reg_isp_dehaze_19 skin_08;
	union reg_isp_dehaze_20 skin_12;
};

struct sop_vip_isp_dhz_tmap_tun_cfg {
	union reg_isp_dehaze_tmap_00 tmap_00;
	union reg_isp_dehaze_tmap_01 tmap_01;
	union reg_isp_dehaze_tmap_02 tmap_02;
	union reg_isp_dehaze_tmap_03 tmap_03;
	union reg_isp_dehaze_tmap_04 tmap_04;
	union reg_isp_dehaze_tmap_05 tmap_05;
	union reg_isp_dehaze_tmap_06 tmap_06;
	union reg_isp_dehaze_tmap_07 tmap_07;
	union reg_isp_dehaze_tmap_08 tmap_08;
	union reg_isp_dehaze_tmap_09 tmap_09;
	union reg_isp_dehaze_tmap_10 tmap_10;
	union reg_isp_dehaze_tmap_11 tmap_11;
	union reg_isp_dehaze_tmap_12 tmap_12;
	union reg_isp_dehaze_tmap_13 tmap_13;
	union reg_isp_dehaze_tmap_14 tmap_14;
	union reg_isp_dehaze_tmap_15 tmap_15;
	union reg_isp_dehaze_tmap_16 tmap_16;
	union reg_isp_dehaze_tmap_17 tmap_17;
	union reg_isp_dehaze_tmap_18 tmap_18;
	union reg_isp_dehaze_tmap_19 tmap_19;
	union reg_isp_dehaze_tmap_20 tmap_20;
	union reg_isp_dehaze_tmap_21 tmap_21;
	union reg_isp_dehaze_tmap_22 tmap_22;
	union reg_isp_dehaze_tmap_23 tmap_23;
	union reg_isp_dehaze_tmap_24 tmap_24;
	union reg_isp_dehaze_tmap_25 tmap_25;
	union reg_isp_dehaze_tmap_26 tmap_26;
	union reg_isp_dehaze_tmap_27 tmap_27;
	union reg_isp_dehaze_tmap_28 tmap_28;
	union reg_isp_dehaze_tmap_29 tmap_29;
	union reg_isp_dehaze_tmap_30 tmap_30;
	union reg_isp_dehaze_tmap_31 tmap_31;
	union reg_isp_dehaze_tmap_32 tmap_32;
};

/* struct sop_vip_isp_dhz_config
 * @param strength:  (0~127) dehaze strength
 * @param th_smooth: (0~1023) threshold for edge/smooth classification.
 */
struct sop_vip_isp_dhz_config {
	__u8  update;
	__u8  enable;
	__u8  strength;
	__u16 cum_th;
	__u16 hist_th;
	__u16 tmap_min;
	__u16 tmap_max;
	__u16 th_smooth;
	__u8  luma_lut_enable;
	__u8  skin_lut_enable;
	__u8  a_luma_wgt;
	__u8  blend_wgt;
	__u8  tmap_scale;
	__u8  d_wgt;
	__u16 sw_dc_th;
	__u16 sw_aglobal_r;
	__u16 sw_aglobal_g;
	__u16 sw_aglobal_b;
	__u16 aglobal_max;
	__u16 aglobal_min;
	__u8  skin_cb;
	__u8  skin_cr;
	struct sop_vip_isp_dhz_luma_tun_cfg luma_cfg;
	struct sop_vip_isp_dhz_skin_tun_cfg skin_cfg;
	struct sop_vip_isp_dhz_tmap_tun_cfg tmap_cfg;
};

struct sop_isp_rgbcac_tun_cfg {
	union reg_isp_rgbcac_purple_th          rgbcac_purple_th;
	union reg_isp_rgbcac_purple_cbcr        rgbcac_purple_cbcr;
	union reg_isp_rgbcac_purple_cbcr2       rgbcac_purple_cbcr2;
	union reg_isp_rgbcac_purple_cbcr3       rgbcac_purple_cbcr3;
	union reg_isp_rgbcac_green_cbcr         rgbcac_green_cbcr;
	union reg_isp_rgbcac_edge_coring        rgbcac_edge_coring;
	union reg_isp_rgbcac_depurple_str_ratio_min  rgbcac_depurple_str_ratio_min;
	union reg_isp_rgbcac_depurple_str_ratio_max  rgbcac_depurple_str_ratio_max;
	union reg_isp_rgbcac_edge_wgt_lut0      rgbcac_edge_wgt_lut0;
	union reg_isp_rgbcac_edge_wgt_lut1      rgbcac_edge_wgt_lut1;
	union reg_isp_rgbcac_edge_wgt_lut2      rgbcac_edge_wgt_lut2;
	union reg_isp_rgbcac_edge_wgt_lut3      rgbcac_edge_wgt_lut3;
	union reg_isp_rgbcac_edge_wgt_lut4      rgbcac_edge_wgt_lut4;
	union reg_isp_rgbcac_luma               rgbcac_luma;
	union reg_isp_rgbcac_luma_blend         rgbcac_luma_blend;
	union reg_isp_rgbcac_luma_filter0       rgbcac_luma_filter0;
	union reg_isp_rgbcac_luma_filter1       rgbcac_luma_filter1;
	union reg_isp_rgbcac_var_filter0        rgbcac_var_filter0;
	union reg_isp_rgbcac_var_filter1        rgbcac_var_filter1;
	union reg_isp_rgbcac_chroma_filter0     rgbcac_chroma_filter0;
	union reg_isp_rgbcac_chroma_filter1     rgbcac_chroma_filter1;
	union reg_isp_rgbcac_cbcr_str           rgbcac_cbcr_str;
};

struct sop_vip_isp_rgbcac_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  out_sel;
	struct sop_isp_rgbcac_tun_cfg rgbcac_cfg;
};

struct sop_isp_cac_tun_cfg {
	union reg_isp_cnr_purple_cb             cnr_purple_cb;
	union reg_isp_cnr_green_cb              cnr_green_cb;
};

struct sop_isp_cac_2_tun_cfg {
	union reg_isp_cnr_edge_scale            cnr_edge_scale;
	union reg_isp_cnr_edge_ratio_speed      cnr_edge_ratio_speed;
	union reg_isp_cnr_depurple_weight_th    cnr_depurple_weight_th;
};

struct sop_isp_cac_3_tun_cfg {
	union reg_isp_cnr_edge_scale_lut_0      cnr_edge_scale_lut_0;
	union reg_isp_cnr_edge_scale_lut_4      cnr_edge_scale_lut_4;
	union reg_isp_cnr_edge_scale_lut_8      cnr_edge_scale_lut_8;
	union reg_isp_cnr_edge_scale_lut_12     cnr_edge_scale_lut_12;
	union reg_isp_cnr_edge_scale_lut_16     cnr_edge_scale_lut_16;
};

struct sop_vip_isp_cac_config {
	__u8  update;
	__u8  enable;
	__u8  out_sel;
	__u8  purple_th;
	__u8  correct_strength;
	__u8  purple_cb2;
	__u8  purple_cr2;
	__u8  purple_cb3;
	__u8  purple_cr3;
	struct sop_isp_cac_tun_cfg cac_cfg;
	struct sop_isp_cac_2_tun_cfg cac_2_cfg;
	struct sop_isp_cac_3_tun_cfg cac_3_cfg;
};

struct sop_isp_lcac_tun_cfg {
	union reg_isp_lcac_reg04                reg04;
	union reg_isp_lcac_reg08                reg08;
	union reg_isp_lcac_reg0c                reg0c;
	union reg_isp_lcac_reg10                reg10;
	union reg_isp_lcac_reg14                reg14;
	union reg_isp_lcac_reg18                reg18;
	union reg_isp_lcac_reg1c                reg1c;
	union reg_isp_lcac_reg20                reg20;
	union reg_isp_lcac_reg24                reg24;
	union reg_isp_lcac_reg28                reg28;
	union reg_isp_lcac_reg2c                reg2c;
	union reg_isp_lcac_reg30                reg30;
	union reg_isp_lcac_reg34                reg34;
	union reg_isp_lcac_reg38                reg38;
	union reg_isp_lcac_reg3c                reg3c;
	union reg_isp_lcac_reg40                reg40;
	union reg_isp_lcac_reg44                reg44;
	union reg_isp_lcac_reg48                reg48;
	union reg_isp_lcac_reg4c                reg4c;
};

struct sop_isp_lcac_2_tun_cfg {
	union reg_isp_lcac_reg50                reg50;
	union reg_isp_lcac_reg54                reg54;
	union reg_isp_lcac_reg58                reg58;
	union reg_isp_lcac_reg5c                reg5c;
	union reg_isp_lcac_reg60                reg60;
	union reg_isp_lcac_reg64                reg64;
	union reg_isp_lcac_reg68                reg68;
	union reg_isp_lcac_reg6c                reg6c;
};

struct sop_isp_lcac_3_tun_cfg {
	union reg_isp_lcac_reg70                reg70;
	union reg_isp_lcac_reg74                reg74;
	union reg_isp_lcac_reg78                reg78;
	union reg_isp_lcac_reg7c                reg7c;
	union reg_isp_lcac_reg80                reg80;
	union reg_isp_lcac_reg84                reg84;
	union reg_isp_lcac_reg88                reg88;
	union reg_isp_lcac_reg8c                reg8c;
};

struct sop_vip_isp_lcac_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  out_sel;
	__u8  lti_luma_lut_32;
	__u8  fcf_luma_lut_32;
	struct sop_isp_lcac_tun_cfg lcac_cfg;
	struct sop_isp_lcac_2_tun_cfg lcac_2_cfg;
	struct sop_isp_lcac_3_tun_cfg lcac_3_cfg;
};

struct sop_vip_isp_csc_config {
	__u8  update;
	__u8  enable;
	__s16 coeff[9];
	__s16 offset[3];
};

struct sop_isp_rgbir_tun_cfg {
	union reg_isp_rgbir_gain_offset_1       gain_offset_1;
	union reg_isp_rgbir_gain_offset_2       gain_offset_2;
	union reg_isp_rgbir_gain_offset_3       gain_offset_3;
	union reg_isp_rgbir_gain_offset_4       gain_offset_4;
	union reg_isp_rgbir_comp_gain           comp_gain;
};

struct sop_vip_isp_rgbir_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  comp_enable;
	struct sop_isp_rgbir_tun_cfg rgbir_cfg;
};

struct sop_isp_dpc_tun_cfg {
	union reg_isp_dpc_3                     dpc_3;
	union reg_isp_dpc_4                     dpc_4;
	union reg_isp_dpc_5                     dpc_5;
	union reg_isp_dpc_6                     dpc_6;
	union reg_isp_dpc_7                     dpc_7;
	union reg_isp_dpc_8                     dpc_8;
	union reg_isp_dpc_9                     dpc_9;
};

struct sop_vip_isp_dpc_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  staticbpc_enable;
	__u8  dynamicbpc_enable;
	__u8  cluster_size;
	__u32 bp_tbl[2047];
	__u16 bp_cnt;
	struct sop_isp_dpc_tun_cfg dpc_cfg;
};

struct sop_isp_ae_tun_cfg {
	union reg_isp_ae_hist_ae_face0_enable   ae_face0_enable;
	union reg_isp_ae_hist_ae_face0_sts_div  ae_face0_sts_div;
	union reg_isp_ae_hist_ae_face1_sts_div  ae_face1_sts_div;
	union reg_isp_ae_hist_ae_face2_sts_div  ae_face2_sts_div;
	union reg_isp_ae_hist_ae_face3_sts_div  ae_face3_sts_div;
	union reg_isp_ae_hist_sts_enable        sts_enable;
	union reg_isp_ae_hist_ae_algo_enable    ae_algo_enable;
	union reg_isp_ae_hist_ae_hist_low       ae_hist_low;
	union reg_isp_ae_hist_ae_hist_high      ae_hist_high;
	union reg_isp_ae_hist_ae_top            ae_top;
	union reg_isp_ae_hist_ae_bot            ae_bot;
	union reg_isp_ae_hist_ae_overexp_thr    ae_overexp_thr;
	union reg_isp_ae_hist_ae_num_gapline    ae_num_gapline;
};

struct sop_isp_ae_2_tun_cfg {
	union reg_isp_ae_hist_ae_wgt_00         ae_wgt_00;
	union reg_isp_ae_hist_ae_wgt_01         ae_wgt_01;
	union reg_isp_ae_hist_ae_wgt_02         ae_wgt_02;
	union reg_isp_ae_hist_ae_wgt_03         ae_wgt_03;
	union reg_isp_ae_hist_ae_wgt_04         ae_wgt_04;
	union reg_isp_ae_hist_ae_wgt_05         ae_wgt_05;
	union reg_isp_ae_hist_ae_wgt_06         ae_wgt_06;
	union reg_isp_ae_hist_ae_wgt_07         ae_wgt_07;
	union reg_isp_ae_hist_ae_wgt_08         ae_wgt_08;
	union reg_isp_ae_hist_ae_wgt_09         ae_wgt_09;
	union reg_isp_ae_hist_ae_wgt_10         ae_wgt_10;
	union reg_isp_ae_hist_ae_wgt_11         ae_wgt_11;
	union reg_isp_ae_hist_ae_wgt_12         ae_wgt_12;
	union reg_isp_ae_hist_ae_wgt_13         ae_wgt_13;
	union reg_isp_ae_hist_ae_wgt_14         ae_wgt_14;
	union reg_isp_ae_hist_ae_wgt_15         ae_wgt_15;
	union reg_isp_ae_hist_ae_wgt_16         ae_wgt_16;
	union reg_isp_ae_hist_ae_wgt_17         ae_wgt_17;
	union reg_isp_ae_hist_ae_wgt_18         ae_wgt_18;
	union reg_isp_ae_hist_ae_wgt_19         ae_wgt_19;
	union reg_isp_ae_hist_ae_wgt_20         ae_wgt_20;
	union reg_isp_ae_hist_ae_wgt_21         ae_wgt_21;
	union reg_isp_ae_hist_ae_wgt_22         ae_wgt_22;
	union reg_isp_ae_hist_ae_wgt_23         ae_wgt_23;
	union reg_isp_ae_hist_ae_wgt_24         ae_wgt_24;
	union reg_isp_ae_hist_ae_wgt_25         ae_wgt_25;
	union reg_isp_ae_hist_ae_wgt_26         ae_wgt_26;
	union reg_isp_ae_hist_ae_wgt_27         ae_wgt_27;
	union reg_isp_ae_hist_ae_wgt_28         ae_wgt_28;
	union reg_isp_ae_hist_ae_wgt_29         ae_wgt_29;
	union reg_isp_ae_hist_ae_wgt_30         ae_wgt_30;
	union reg_isp_ae_hist_ae_wgt_31         ae_wgt_31;
};

struct sop_vip_isp_ae_config {
	__u8  update;
	__u8  inst;
	__u8  ae_enable;
	__u16 ae_offsetx;
	__u16 ae_offsety;
	__u8  ae_numx;
	__u8  ae_numy;
	__u16 ae_width;
	__u16 ae_height;
	__u16 ae_sts_div;
	__u16 ae_face_offset_x[4];
	__u16 ae_face_offset_y[4];
	__u8  ae_face_size_minus1_x[4];
	__u8  ae_face_size_minus1_y[4];
	struct sop_isp_ae_tun_cfg ae_cfg;
	struct sop_isp_ae_2_tun_cfg ae_2_cfg;
};

struct sop_isp_ge_tun_cfg {
	union reg_isp_dpc_10                    dpc_10;
	union reg_isp_dpc_11                    dpc_11;
	union reg_isp_dpc_12                    dpc_12;
	union reg_isp_dpc_13                    dpc_13;
	union reg_isp_dpc_14                    dpc_14;
	union reg_isp_dpc_15                    dpc_15;
	union reg_isp_dpc_16                    dpc_16;
};

struct sop_vip_isp_ge_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	struct sop_isp_ge_tun_cfg ge_cfg;
};

struct sop_vip_isp_af_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  dpc_enable;
	__u8  hlc_enable;
	__u8  square_enable;
	__u8  outshift;
	__u8  num_gapline;
	__u16 offsetx;
	__u16 offsety;
	__u16 block_width;
	__u16 block_height;
	__u8  block_numx;
	__u8  block_numy;
	__u8  h_low_pass_value_shift;
	__u32 h_corning_offset_0;
	__u32 h_corning_offset_1;
	__u16 v_corning_offset;
	__u16 high_luma_threshold;
	__u8  h_low_pass_coef[5];
	__u8  h_high_pass_coef_0[5];
	__u8  h_high_pass_coef_1[5];
	__u8  v_high_pass_coef[3];
	__u8  th_low;
	__u8  th_high;
	__u8  gain_low;
	__u8  gain_high;
	__u8  slop_low;
	__u8  slop_high;
};

struct sop_vip_isp_hist_v_config {
	__u8  update;
	__u8  enable;
	__u8  luma_mode;
	__u16 offset_x;
	__u16 offset_y;
};

struct sop_vip_isp_gms_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u16 offset_x;
	__u16 offset_y;
	__u16 x_section_size;
	__u16 y_section_size;
	__u8  x_gap;
	__u8  y_gap;
};

struct sop_vip_isp_mono_config {
	__u8  update;
	__u8  force_mono_enable;
};

#if 0
struct sop_vip_isp_3dlut_config {
	__u8  update;
	__u8  enable;
	__u8  h_clamp_wrap_opt;
	__u16 h_lut[3276];
	__u16 s_lut[3276];
	__u16 v_lut[3276];
};

struct sop_vip_isp_lscr_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u16 strength;
	__u16 strength_ir;
	__u16 norm;
	__u16 norm_ir;
	__u16 centerx;
	__u16 centery;
	__u16 gain_lut[32];
	__u16 gain_lut1[32];
	__u16 gain_lut2[32];
	__u16 gain_lut_ir[32];
};

struct sop_vip_isp_awb_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__u8  bayer_start;
	__u16 awb_offsetx;
	__u16 awb_offsety;
	__u16 awb_sub_win_w;
	__u16 awb_sub_win_h;
	__u8  awb_numx;
	__u8  awb_numy;
	__u8  corner_avg_en;
	__u8  corner_size;
	__u8  awb_sts_div;
	__u16 r_lower_bound;
	__u16 r_upper_bound;
	__u16 g_lower_bound;
	__u16 g_upper_bound;
	__u16 b_lower_bound;
	__u16 b_upper_bound;
};

struct sop_vip_isp_hsv_config {
	__u8  update;
	__u8  enable;
	__u8  htune_enable;
	__u8  stune_enable;
	__u8  hsgain_enable;
	__u8  hvgain_enable;
	__u16 h_lut[769];
	__u16 s_lut[513];
	__u16 sgain_lut[769];
	__u16 vgain_lut[769];
};

struct sop_vip_isp_preproc_config {
	__u8  update;
	__u8  inst;
	__u8  enable;
	__s16 r_ir_ratio[128];
	__s16 g_ir_ratio[128];
	__s16 b_ir_ratio[128];
	__u8  w_lut[128];
};

#endif

struct sop_vip_teaisp_bnr_config {
	__u8  update;
	__u32 blc;
	__u32 coeff_a;
	__u32 coeff_b;
	__u32 filter_motion_str_2d;
	__u32 filter_static_str_2d;
	__u32 filter_str_3d;
};

struct sop_vip_isp_fe_tun_cfg {
	struct sop_vip_isp_blc_config	blc_cfg[2];
	struct sop_vip_isp_wbg_config	wbg_cfg[2];
	struct sop_vip_teaisp_bnr_config bnr;
};

struct sop_vip_isp_be_tun_cfg {
	struct sop_vip_isp_blc_config		blc_cfg[2];
	struct sop_vip_isp_rgbir_config		rgbir_cfg[2];
	struct sop_vip_isp_dpc_config		dpc_cfg[2];
	struct sop_vip_isp_ge_config		ge_cfg[2];
	struct sop_vip_isp_af_config		af_cfg;
};

struct sop_vip_isp_post_tun_cfg {
	struct sop_vip_isp_bnr_config		bnr_cfg[2];
	struct sop_vip_isp_lsc_config		lsc_cfg[2];
	struct sop_vip_isp_ae_config		ae_cfg[2];
	struct sop_vip_isp_rgbcac_config	rgbcac_cfg[2];
	struct sop_vip_isp_lcac_config		lcac_cfg[2];
	struct sop_vip_isp_wbg_config		wbg_cfg[2];
	struct sop_vip_isp_ccm_config		ccm_cfg[2];
	struct sop_vip_isp_ygamma_config	ygamma_cfg;
	struct sop_vip_isp_gamma_config		gamma_cfg;
	struct sop_vip_isp_dhz_config		dhz_cfg;
	struct sop_vip_isp_csc_config		csc_cfg;
	struct sop_vip_isp_dci_config		dci_cfg;
	struct sop_vip_isp_ldci_config		ldci_cfg;
	struct sop_vip_isp_pre_ee_config	pre_ee_cfg;
	struct sop_vip_isp_tnr_config		tnr_cfg;
	struct sop_vip_isp_cnr_config		cnr_cfg;
	struct sop_vip_isp_cac_config		cac_cfg;
	struct sop_vip_isp_ynr_config		ynr_cfg;
	struct sop_vip_isp_ee_config		ee_cfg;
	struct sop_vip_isp_cacp_config		cacp_cfg;
	struct sop_vip_isp_ca2_config		ca2_cfg;
	struct sop_vip_isp_ycur_config		ycur_cfg;
	struct sop_vip_isp_demosiac_config	demosiac_cfg[2];
	struct sop_vip_isp_clut_config		clut_cfg;
	struct sop_vip_isp_drc_config		drc_cfg;
	struct sop_vip_isp_fswdr_config		fswdr_cfg;
	struct sop_vip_isp_hist_v_config	hist_v_cfg;
	struct sop_vip_isp_gms_config		gms_cfg;
	struct sop_vip_isp_mono_config		mono_cfg;
};

struct sop_vip_isp_fe_cfg {
	uint8_t tun_update[TUNING_NODE_NUM];
	uint8_t tun_idx;
	struct sop_vip_isp_fe_tun_cfg tun_cfg[TUNING_NODE_NUM];
};

struct sop_vip_isp_be_cfg {
	uint8_t tun_update[TUNING_NODE_NUM];
	uint8_t tun_idx;
	struct sop_vip_isp_be_tun_cfg tun_cfg[TUNING_NODE_NUM];
};

struct sop_vip_isp_post_cfg {
	uint8_t tun_update[TUNING_NODE_NUM];
	uint8_t tun_idx;
	struct sop_vip_isp_post_tun_cfg tun_cfg[TUNING_NODE_NUM];
};

struct isp_tuning_cfg {
	uint64_t  fe_addr[ISP_PRERAW_MAX];
	void	  *fe_vir[ISP_PRERAW_MAX];
#ifdef __arm__
	__u32	  fe_padding[ISP_PRERAW_MAX];
#endif
	uint64_t  be_addr[ISP_PRERAW_MAX];
	void	  *be_vir[ISP_PRERAW_MAX];
#ifdef __arm__
	__u32	  be_padding[ISP_PRERAW_MAX];
#endif
	uint64_t  post_addr[ISP_PRERAW_MAX];
	void      *post_vir[ISP_PRERAW_MAX];
#ifdef __arm__
	__u32	  post_padding[ISP_PRERAW_MAX];
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* _U_VI_TUN_CFG_H_ */

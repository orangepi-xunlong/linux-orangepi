/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_fields_csi_mac.h
 * Description:HW register description
 */

#ifndef _REG_FIELDS_CSI_MAC_H_
#define _REG_FIELDS_CSI_MAC_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           module definition            */
/******************************************/
union reg_sensor_mac_00 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_mode                 : 3;
		uint32_t bt_demux_enable                 : 1;
		uint32_t csi_ctrl_enable                 : 1;
		uint32_t csi_vs_inv                      : 1;
		uint32_t csi_hs_inv                      : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t sublvds_ctrl_enable             : 1;
		uint32_t sublvds_vs_inv                  : 1;
		uint32_t sublvds_hs_inv                  : 1;
		uint32_t sublvds_hdr_inv                 : 1;
		uint32_t slvsec_ctrl_enable              : 1;
		uint32_t slvsec_vs_inv                   : 1;
		uint32_t slvsec_hs_inv                   : 1;
		uint32_t _rsv_15                         : 1;
		uint32_t mask_up                         : 1;
		uint32_t shrd_sel                        : 1;
		uint32_t sw_up                           : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t dbg_sel                         : 8;
	} bits;
};

union reg_sensor_mac_10 {
	uint32_t raw;
	struct {
		uint32_t ttl_ip_en                       : 1;
		uint32_t ttl_sensor_bit                  : 2;
		uint32_t _rsv_3                          : 1;
		uint32_t ttl_bt_fmt_out                  : 2;
		uint32_t _rsv_6                          : 2;
		uint32_t ttl_fmt_in                      : 4;
		uint32_t ttl_bt_data_seq                 : 2;
		uint32_t ttl_vs_inv                      : 1;
		uint32_t ttl_hs_inv                      : 1;
	} bits;
};

union reg_sensor_mac_14 {
	uint32_t raw;
	struct {
		uint32_t ttl_vs_bp                       : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_hs_bp                       : 12;
	} bits;
};

union reg_sensor_mac_18 {
	uint32_t raw;
	struct {
		uint32_t ttl_img_wd                      : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_img_ht                      : 12;
	} bits;
};

union reg_sensor_mac_1c {
	uint32_t raw;
	struct {
		uint32_t ttl_sync_0                      : 16;
		uint32_t ttl_sync_1                      : 16;
	} bits;
};

union reg_sensor_mac_20 {
	uint32_t raw;
	struct {
		uint32_t ttl_sync_2                      : 16;
	} bits;
};

union reg_sensor_mac_24 {
	uint32_t raw;
	struct {
		uint32_t ttl_sav_vld                     : 16;
		uint32_t ttl_sav_blk                     : 16;
	} bits;
};

union reg_sensor_mac_28 {
	uint32_t raw;
	struct {
		uint32_t ttl_eav_vld                     : 16;
		uint32_t ttl_eav_blk                     : 16;
	} bits;
};

union reg_sensor_mac_30 {
	uint32_t raw;
	struct {
		uint32_t vi_sel                          : 3;
		uint32_t vi_from                         : 1;
		uint32_t vi_clk_inv                      : 1;
		uint32_t vi_v_sel_vs                     : 1;
		uint32_t vi_vs_dbg                       : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t pad_vi0_clk_inv                 : 1;
		uint32_t pad_vi1_clk_inv                 : 1;
		uint32_t pad_vi2_clk_inv                 : 1;
	} bits;
};

union reg_sensor_mac_34 {
	uint32_t raw;
	struct {
		uint32_t vi_vs_dly                       : 5;
		uint32_t _rsv_5                          : 1;
		uint32_t vi_vs_dly_en                    : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t vi_hs_dly                       : 5;
		uint32_t _rsv_13                         : 1;
		uint32_t vi_hs_dly_en                    : 1;
		uint32_t _rsv_15                         : 1;
		uint32_t vi_vde_dly                      : 5;
		uint32_t _rsv_21                         : 1;
		uint32_t vi_vde_dly_en                   : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t vi_hde_dly                      : 5;
		uint32_t _rsv_29                         : 1;
		uint32_t vi_hde_dly_en                   : 1;
	} bits;
};

union reg_sensor_mac_40 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_hdr_en               : 1;
		uint32_t sensor_mac_hdr_vsinv            : 1;
		uint32_t sensor_mac_hdr_hsinv            : 1;
		uint32_t sensor_mac_hdr_deinv            : 1;
		uint32_t sensor_mac_hdr_hdr0inv          : 1;
		uint32_t sensor_mac_hdr_hdr1inv          : 1;
		uint32_t sensor_mac_hdr_blcinv           : 1;
		uint32_t _rsv_7                          : 1;
		uint32_t sensor_mac_hdr_mode             : 1;
	} bits;
};

union reg_sensor_mac_44 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_hdr_shift            : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t sensor_mac_hdr_vsize            : 13;
	} bits;
};

union reg_sensor_mac_48 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_info_line_num        : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t sensor_mac_rm_info_line         : 1;
	} bits;
};

union reg_sensor_mac_4c {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_hdr_line_cnt         : 14;
	} bits;
};

union reg_sensor_mac_50 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_blc0_en              : 1;
		uint32_t sensor_mac_blc1_en              : 1;
	} bits;
};

union reg_sensor_mac_54 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_blc0_start           : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t sensor_mac_blc0_size            : 13;
	} bits;
};

union reg_sensor_mac_58 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_blc1_start           : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t sensor_mac_blc1_size            : 13;
	} bits;
};

union reg_sensor_mac_60 {
	uint32_t raw;
	struct {
		uint32_t vi_vs_sel                       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t vi_hs_sel                       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t vi_vde_sel                      : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t vi_hde_sel                      : 6;
	} bits;
};

union reg_sensor_mac_64 {
	uint32_t raw;
	struct {
		uint32_t vi_d0_sel                       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t vi_d1_sel                       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t vi_d2_sel                       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t vi_d3_sel                       : 6;
	} bits;
};

union reg_sensor_mac_68 {
	uint32_t raw;
	struct {
		uint32_t vi_d4_sel                       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t vi_d5_sel                       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t vi_d6_sel                       : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t vi_d7_sel                       : 6;
	} bits;
};

union reg_sensor_mac_6c {
	uint32_t raw;
	struct {
		uint32_t vi_d8_sel                       : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t vi_d9_sel                       : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t vi_d10_sel                      : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t vi_d11_sel                      : 6;
	} bits;
};

union reg_sensor_mac_70 {
	uint32_t raw;
	struct {
		uint32_t vi_d12_sel                      : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t vi_d13_sel                      : 6;
		uint32_t _rsv_14                         : 2;
		uint32_t vi_d14_sel                      : 6;
		uint32_t _rsv_22                         : 2;
		uint32_t vi_d15_sel                      : 6;
	} bits;
};

union reg_sensor_mac_74 {
	uint32_t raw;
	struct {
		uint32_t vi_bt_d0_sel                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t vi_bt_d1_sel                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t vi_bt_d2_sel                    : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t vi_bt_d3_sel                    : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t vi_bt_d4_sel                    : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t vi_bt_d5_sel                    : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t vi_bt_d6_sel                    : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t vi_bt_d7_sel                    : 3;
	} bits;
};

union reg_sensor_mac_80 {
	uint32_t raw;
	struct {
		uint32_t bt_clr_sync_lost_1t             : 1;
		uint32_t bt_ip_en                        : 1;
		uint32_t bt_ddr_mode                     : 1;
		uint32_t bt_hs_gate_by_vde               : 1;
		uint32_t bt_vs_inv                       : 1;
		uint32_t bt_hs_inv                       : 1;
		uint32_t bt_vs_as_vde                    : 1;
		uint32_t bt_hs_as_hde                    : 1;
		uint32_t bt_sw_en_clk                    : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t bt_demux_ch                     : 2;
		uint32_t _rsv_18                         : 2;
		uint32_t bt_fmt_sel                      : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t bt_sync_lost                    : 1;
	} bits;
};

union reg_sensor_mac_84 {
	uint32_t raw;
	struct {
		uint32_t bt_v_ctrl_dly                   : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t bt_h_ctrl_dly                   : 5;
	} bits;
};

union reg_sensor_mac_88 {
	uint32_t raw;
	struct {
		uint32_t bt_img_wd_m1                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t bt_img_ht_m1                    : 12;
	} bits;
};

union reg_sensor_mac_8c {
	uint32_t raw;
	struct {
		uint32_t bt_vs_bp_m1                     : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t bt_hs_bp_m1                     : 12;
	} bits;
};

union reg_sensor_mac_90 {
	uint32_t raw;
	struct {
		uint32_t bt_vs_fp_m1                     : 8;
		uint32_t bt_hs_fp_m1                     : 8;
	} bits;
};

union reg_sensor_mac_94 {
	uint32_t raw;
	struct {
		uint32_t bt_sync_0                       : 8;
		uint32_t bt_sync_1                       : 8;
		uint32_t bt_sync_2                       : 8;
	} bits;
};

union reg_sensor_mac_98 {
	uint32_t raw;
	struct {
		uint32_t bt_sav_vld_0                    : 8;
		uint32_t bt_sav_blk_0                    : 8;
		uint32_t bt_eav_vld_0                    : 8;
		uint32_t bt_eav_blk_0                    : 8;
	} bits;
};

union reg_sensor_mac_9c {
	uint32_t raw;
	struct {
		uint32_t bt_sav_vld_1                    : 8;
		uint32_t bt_sav_blk_1                    : 8;
		uint32_t bt_eav_vld_1                    : 8;
		uint32_t bt_eav_blk_1                    : 8;
	} bits;
};

union reg_sensor_mac_a0 {
	uint32_t raw;
	struct {
		uint32_t bt_sav_vld_2                    : 8;
		uint32_t bt_sav_blk_2                    : 8;
		uint32_t bt_eav_vld_2                    : 8;
		uint32_t bt_eav_blk_2                    : 8;
	} bits;
};

union reg_sensor_mac_a4 {
	uint32_t raw;
	struct {
		uint32_t bt_sav_vld_3                    : 8;
		uint32_t bt_sav_blk_3                    : 8;
		uint32_t bt_eav_vld_3                    : 8;
		uint32_t bt_eav_blk_3                    : 8;
	} bits;
};

union reg_sensor_mac_a8 {
	uint32_t raw;
	struct {
		uint32_t bt_yc_inv                       : 4;
	} bits;
};

union reg_sensor_mac_b0 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_crop_start_x         : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t sensor_mac_crop_end_x           : 13;
		uint32_t _rsv_29                         : 2;
		uint32_t sensor_mac_crop_en              : 1;
	} bits;
};

union reg_sensor_mac_b4 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_crop_start_y         : 13;
		uint32_t _rsv_13                         : 3;
		uint32_t sensor_mac_crop_end_y           : 13;
	} bits;
};

union reg_sensor_mac_b8 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_swapuv_en            : 1;
		uint32_t sensor_mac_swapyc_en            : 1;
	} bits;
};

union reg_sensor_mac_bc {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_dbg_htotal_max       : 16;
		uint32_t sensor_mac_dbg_en               : 1;
	} bits;
};

union reg_sensor_mac_c0 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_dbg_vtotal_max       : 32;
	} bits;
};

union reg_sensor_mac_c4 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_dbg_htotal           : 16;
	} bits;
};

union reg_sensor_mac_c8 {
	uint32_t raw;
	struct {
		uint32_t sensor_mac_dbg_vtotal           : 32;
	} bits;
};

union reg_sensor_mac_d0 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_enable             : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t ttl_as_slvds_bit_mode           : 2;
		uint32_t ttl_as_slvds_data_reverse       : 1;
		uint32_t _rsv_11                         : 1;
		uint32_t ttl_as_slvds_hdr_mode           : 1;
		uint32_t ttl_as_slvds_hdr_pattern        : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t ttl_as_slvds_vfporch            : 10;
	} bits;
};

union reg_sensor_mac_d4 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_sync_1st           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_sync_2nd           : 12;
	} bits;
};

union reg_sensor_mac_d8 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_sync_3rd           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_norm_bk_sav        : 12;
	} bits;
};

union reg_sensor_mac_dc {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_norm_bk_eav        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_norm_sav           : 12;
	} bits;
};

union reg_sensor_mac_e0 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_norm_eav           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_n0_bk_sav          : 12;
	} bits;
};

union reg_sensor_mac_e4 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_n0_bk_eav          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_n1_bk_sav          : 12;
	} bits;
};

union reg_sensor_mac_e8 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_n1_bk_eav          : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_n0_lef_sav         : 12;
	} bits;
};

union reg_sensor_mac_ec {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_n0_lef_eav         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_n0_sef_sav         : 12;
	} bits;
};

union reg_sensor_mac_f0 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_n0_sef_eav         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_n1_lef_sav         : 12;
	} bits;
};

union reg_sensor_mac_f4 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_n1_lef_eav         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_n1_sef_sav         : 12;
	} bits;
};

union reg_sensor_mac_f8 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_n1_sef_eav         : 12;
	} bits;
};

union reg_sensor_mac_fc {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_vs_gen_sync_code   : 12;
		uint32_t ttl_as_slvds_vs_gen_by_sync_code: 1;
	} bits;
};

union reg_sensor_mac_100 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_n0_lsef_sav        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_n0_lsef_eav        : 12;
	} bits;
};

union reg_sensor_mac_104 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_n1_lsef_sav        : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_slvds_n1_lsef_eav        : 12;
	} bits;
};

union reg_sensor_mac_108 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_slvds_hdr_p2_hsize       : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t ttl_as_slvds_hdr_p2_hblank      : 14;
	} bits;
};

union reg_sensor_mac_110 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_hispi_mode               : 1;
		uint32_t ttl_as_hispi_use_hsize          : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t ttl_as_hispi_hdr_psp_mode       : 1;
	} bits;
};

union reg_sensor_mac_114 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_hispi_norm_sof           : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_hispi_norm_eof           : 12;
	} bits;
};

union reg_sensor_mac_118 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_hispi_hdr_t1_sof         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_hispi_hdr_t1_eof         : 12;
	} bits;
};

union reg_sensor_mac_11c {
	uint32_t raw;
	struct {
		uint32_t ttl_as_hispi_hdr_t1_sol         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_hispi_hdr_t1_eol         : 12;
	} bits;
};

union reg_sensor_mac_120 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_hispi_hdr_t2_sof         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_hispi_hdr_t2_eof         : 12;
	} bits;
};

union reg_sensor_mac_124 {
	uint32_t raw;
	struct {
		uint32_t ttl_as_hispi_hdr_t2_sol         : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t ttl_as_hispi_hdr_t2_eol         : 12;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_sublvds_ctrl_top_00 {
	uint32_t raw;
	struct {
		uint32_t slvds_enable                    : 8;
		uint32_t slvds_bit_mode                  : 2;
		uint32_t slvds_data_reverse              : 1;
		uint32_t _rsv_11                         : 1;
		uint32_t slvds_hdr_mode                  : 1;
		uint32_t slvds_hdr_pattern               : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t slvds_vfporch                   : 10;
	} bits;
};

union reg_sublvds_ctrl_top_04 {
	uint32_t raw;
	struct {
		uint32_t slvds_sync_1st                  : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_sync_2nd                  : 12;
	} bits;
};

union reg_sublvds_ctrl_top_08 {
	uint32_t raw;
	struct {
		uint32_t slvds_sync_3rd                  : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_norm_bk_sav               : 12;
	} bits;
};

union reg_sublvds_ctrl_top_0c {
	uint32_t raw;
	struct {
		uint32_t slvds_norm_bk_eav               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_norm_sav                  : 12;
	} bits;
};

union reg_sublvds_ctrl_top_10 {
	uint32_t raw;
	struct {
		uint32_t slvds_norm_eav                  : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n0_bk_sav                 : 12;
	} bits;
};

union reg_sublvds_ctrl_top_14 {
	uint32_t raw;
	struct {
		uint32_t slvds_n0_bk_eav                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n1_bk_sav                 : 12;
	} bits;
};

union reg_sublvds_ctrl_top_18 {
	uint32_t raw;
	struct {
		uint32_t slvds_n1_bk_eav                 : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n0_lef_sav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_1c {
	uint32_t raw;
	struct {
		uint32_t slvds_n0_lef_eav                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n0_sef_sav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_20 {
	uint32_t raw;
	struct {
		uint32_t slvds_n0_sef_eav                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n1_lef_sav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_24 {
	uint32_t raw;
	struct {
		uint32_t slvds_n1_lef_eav                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n1_sef_sav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_28 {
	uint32_t raw;
	struct {
		uint32_t slvds_n1_sef_eav                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_2c {
	uint32_t raw;
	struct {
		uint32_t vs_gen_sync_code                : 12;
		uint32_t vs_gen_by_sync_code             : 1;
	} bits;
};

union reg_sublvds_ctrl_top_30 {
	uint32_t raw;
	struct {
		uint32_t slvds_lane_mode                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t slvds_sync_source               : 8;
		uint32_t slvds_fifo_clr                  : 1;
	} bits;
};

union reg_sublvds_ctrl_top_40 {
	uint32_t raw;
	struct {
		uint32_t slvds_fifo_full                 : 1;
	} bits;
};

union reg_sublvds_ctrl_top_50 {
	uint32_t raw;
	struct {
		uint32_t slvds_n0_lsef_sav               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n0_lsef_eav               : 12;
	} bits;
};

union reg_sublvds_ctrl_top_54 {
	uint32_t raw;
	struct {
		uint32_t slvds_n1_lsef_sav               : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_n1_lsef_eav               : 12;
	} bits;
};

union reg_sublvds_ctrl_top_58 {
	uint32_t raw;
	struct {
		uint32_t slvds_hdr_p2_hsize              : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t slvds_hdr_p2_hblank             : 14;
	} bits;
};

union reg_sublvds_ctrl_top_60 {
	uint32_t raw;
	struct {
		uint32_t hispi_mode                      : 1;
		uint32_t hispi_use_hsize                 : 1;
		uint32_t _rsv_2                          : 2;
		uint32_t hispi_hdr_psp_mode              : 1;
	} bits;
};

union reg_sublvds_ctrl_top_64 {
	uint32_t raw;
	struct {
		uint32_t hispi_norm_sof                  : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_norm_eof                  : 12;
	} bits;
};

union reg_sublvds_ctrl_top_68 {
	uint32_t raw;
	struct {
		uint32_t hispi_hdr_t1_sof                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_hdr_t1_eof                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_6c {
	uint32_t raw;
	struct {
		uint32_t hispi_hdr_t1_sol                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_hdr_t1_eol                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_70 {
	uint32_t raw;
	struct {
		uint32_t hispi_hdr_t2_sof                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_hdr_t2_eof                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_74 {
	uint32_t raw;
	struct {
		uint32_t hispi_hdr_t2_sol                : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t hispi_hdr_t2_eol                : 12;
	} bits;
};

union reg_sublvds_ctrl_top_80 {
	uint32_t raw;
	struct {
		uint32_t dbg_sel                         : 8;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_csi_ctrl_top_00 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_mode                   : 3;
		uint32_t csi_ignore_ecc                  : 1;
		uint32_t csi_vc_check                    : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t csi_vc_set                      : 4;
		uint32_t csi_line_start_sent             : 1;
		uint32_t csi_format_frc                  : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t csi_format_set                  : 8;
	} bits;
};

union reg_csi_ctrl_top_04 {
	uint32_t raw;
	struct {
		uint32_t csi_intr_mask                   : 8;
		uint32_t csi_intr_clr                    : 8;
		uint32_t csi_hdr_en                      : 1;
		uint32_t csi_hdr_mode                    : 1;
		uint32_t csi_id_rm_else                  : 1;
		uint32_t csi_id_rm_ob                    : 1;
	} bits;
};

union reg_csi_ctrl_top_08 {
	uint32_t raw;
	struct {
		uint32_t csi_n0_ob_lef                   : 16;
		uint32_t csi_n0_ob_sef                   : 16;
	} bits;
};

union reg_csi_ctrl_top_0c {
	uint32_t raw;
	struct {
		uint32_t csi_n0_lef                      : 16;
		uint32_t csi_n1_ob_lef                   : 16;
	} bits;
};

union reg_csi_ctrl_top_10 {
	uint32_t raw;
	struct {
		uint32_t csi_n1_ob_sef                   : 16;
		uint32_t csi_n1_lef                      : 16;
	} bits;
};

union reg_csi_ctrl_top_14 {
	uint32_t raw;
	struct {
		uint32_t csi_blc_dt                      : 6;
		uint32_t _rsv_6                          : 2;
		uint32_t csi_blc_en                      : 1;
		uint32_t _rsv_9                          : 3;
		uint32_t csi_blc_format_set              : 3;
	} bits;
};

union reg_csi_ctrl_top_18 {
	uint32_t raw;
	struct {
		uint32_t csi_vc_map_ch00                 : 4;
		uint32_t csi_vc_map_ch01                 : 4;
		uint32_t csi_vc_map_ch10                 : 4;
		uint32_t csi_vc_map_ch11                 : 4;
	} bits;
};

union reg_csi_ctrl_top_1c {
	uint32_t raw;
	struct {
		uint32_t csi_n0_sef                      : 16;
		uint32_t csi_n1_sef                      : 16;
	} bits;
};

union reg_csi_ctrl_top_20 {
	uint32_t raw;
	struct {
		uint32_t csi_n0_sef2                     : 16;
		uint32_t csi_n1_sef2                     : 16;
	} bits;
};

union reg_csi_ctrl_top_24 {
	uint32_t raw;
	struct {
		uint32_t csi_n0_ob_sef2                  : 16;
		uint32_t csi_n1_ob_sef2                  : 16;
	} bits;
};

union reg_csi_ctrl_top_30 {
	uint32_t raw;
	struct {
		uint32_t csi_ecc_ph_dbg                  : 32;
	} bits;
};

union reg_csi_ctrl_top_34 {
	uint32_t raw;
	struct {
		uint32_t csi_ecc_clr_ph_dbg              : 1;
	} bits;
};

union reg_csi_ctrl_top_40 {
	uint32_t raw;
	struct {
		uint32_t csi_ecc_no_error                : 1;
		uint32_t csi_ecc_corrected_error         : 1;
		uint32_t csi_ecc_error                   : 1;
		uint32_t _rsv_3                          : 1;
		uint32_t csi_crc_error                   : 1;
		uint32_t csi_wc_error                    : 1;
		uint32_t _rsv_6                          : 2;
		uint32_t csi_fifo_full                   : 1;
		uint32_t _rsv_9                          : 7;
		uint32_t csi_decode_format               : 6;
	} bits;
};

union reg_csi_ctrl_top_48 {
	uint32_t raw;
	struct {
		uint32_t short_pkt_rsv1                  : 16;
		uint32_t short_pkt_rsv2                  : 16;
	} bits;
};

union reg_csi_ctrl_top_4c {
	uint32_t raw;
	struct {
		uint32_t short_pkt_rsv3                  : 16;
		uint32_t short_pkt_rsv4                  : 16;
	} bits;
};

union reg_csi_ctrl_top_50 {
	uint32_t raw;
	struct {
		uint32_t generic_short_pkt1              : 16;
		uint32_t generic_short_pkt2              : 16;
	} bits;
};

union reg_csi_ctrl_top_54 {
	uint32_t raw;
	struct {
		uint32_t generic_short_pkt3              : 16;
		uint32_t generic_short_pkt4              : 16;
	} bits;
};

union reg_csi_ctrl_top_58 {
	uint32_t raw;
	struct {
		uint32_t generic_short_pkt5              : 16;
		uint32_t generic_short_pkt6              : 16;
	} bits;
};

union reg_csi_ctrl_top_5c {
	uint32_t raw;
	struct {
		uint32_t generic_short_pkt7              : 16;
		uint32_t generic_short_pkt8              : 16;
	} bits;
};

union reg_csi_ctrl_top_60 {
	uint32_t raw;
	struct {
		uint32_t csi_intr_status                 : 8;
	} bits;
};

union reg_csi_ctrl_top_64 {
	uint32_t raw;
	struct {
		uint32_t csi_dbg_sel                     : 8;
	} bits;
};

union reg_csi_ctrl_top_70 {
	uint32_t raw;
	struct {
		uint32_t csi_vs_gen_mode                 : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t csi_vs_gen_by_vcset             : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t csi_vs_delay_sel                : 2;
		uint32_t csi_hs_delay_sel                : 2;
	} bits;
};

union reg_csi_ctrl_top_74 {
	uint32_t raw;
	struct {
		uint32_t csi_hdr_dt_mode                 : 1;
		uint32_t _rsv_1                          : 3;
		uint32_t csi_hdr_dt_format               : 6;
		uint32_t _rsv_10                         : 2;
		uint32_t csi_hdr_dt_lef                  : 6;
		uint32_t _rsv_18                         : 2;
		uint32_t csi_hdr_dt_sef                  : 6;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_sensor_mac_vi_00 {
	uint32_t raw;
	struct {
		uint32_t bt_demux_enable                 : 1;
		uint32_t _rsv_1                          : 9;
		uint32_t pad_clk_inv                     : 1;
		uint32_t _rsv_11                         : 13;
		uint32_t dbg_sel                         : 8;
	} bits;
};

union reg_sensor_mac_vi_10 {
	uint32_t raw;
	struct {
		uint32_t vi_bt_d0_sel                    : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t vi_bt_d1_sel                    : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t vi_bt_d2_sel                    : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t vi_bt_d3_sel                    : 5;
	} bits;
};

union reg_sensor_mac_vi_14 {
	uint32_t raw;
	struct {
		uint32_t vi_bt_d4_sel                    : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t vi_bt_d5_sel                    : 5;
		uint32_t _rsv_13                         : 3;
		uint32_t vi_bt_d6_sel                    : 5;
		uint32_t _rsv_21                         : 3;
		uint32_t vi_bt_d7_sel                    : 5;
	} bits;
};

union reg_sensor_mac_vi_20 {
	uint32_t raw;
	struct {
		uint32_t bt_clr_sync_lost_1t             : 1;
		uint32_t bt_ip_en                        : 1;
		uint32_t bt_ddr_mode                     : 1;
		uint32_t bt_hs_gate_by_vde               : 1;
		uint32_t bt_vs_inv                       : 1;
		uint32_t bt_hs_inv                       : 1;
		uint32_t bt_vs_as_vde                    : 1;
		uint32_t bt_hs_as_hde                    : 1;
		uint32_t bt_sw_en_clk                    : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t bt_demux_ch                     : 2;
		uint32_t _rsv_18                         : 2;
		uint32_t bt_fmt_sel                      : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t bt_sync_lost                    : 1;
	} bits;
};

union reg_sensor_mac_vi_24 {
	uint32_t raw;
	struct {
		uint32_t bt_v_ctrl_dly                   : 5;
		uint32_t _rsv_5                          : 3;
		uint32_t bt_h_ctrl_dly                   : 5;
	} bits;
};

union reg_sensor_mac_vi_28 {
	uint32_t raw;
	struct {
		uint32_t bt_img_wd_m1                    : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t bt_img_ht_m1                    : 12;
	} bits;
};

union reg_sensor_mac_vi_2c {
	uint32_t raw;
	struct {
		uint32_t bt_vs_bp_m1                     : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t bt_hs_bp_m1                     : 12;
	} bits;
};

union reg_sensor_mac_vi_30 {
	uint32_t raw;
	struct {
		uint32_t bt_vs_fp_m1                     : 8;
		uint32_t bt_hs_fp_m1                     : 8;
	} bits;
};

union reg_sensor_mac_vi_34 {
	uint32_t raw;
	struct {
		uint32_t bt_sync_0                       : 8;
		uint32_t bt_sync_1                       : 8;
		uint32_t bt_sync_2                       : 8;
	} bits;
};

union reg_sensor_mac_vi_38 {
	uint32_t raw;
	struct {
		uint32_t bt_sav_vld_0                    : 8;
		uint32_t bt_sav_blk_0                    : 8;
		uint32_t bt_eav_vld_0                    : 8;
		uint32_t bt_eav_blk_0                    : 8;
	} bits;
};

union reg_sensor_mac_vi_3c {
	uint32_t raw;
	struct {
		uint32_t bt_sav_vld_1                    : 8;
		uint32_t bt_sav_blk_1                    : 8;
		uint32_t bt_eav_vld_1                    : 8;
		uint32_t bt_eav_blk_1                    : 8;
	} bits;
};

union reg_sensor_mac_vi_40 {
	uint32_t raw;
	struct {
		uint32_t bt_sav_vld_2                    : 8;
		uint32_t bt_sav_blk_2                    : 8;
		uint32_t bt_eav_vld_2                    : 8;
		uint32_t bt_eav_blk_2                    : 8;
	} bits;
};

union reg_sensor_mac_vi_44 {
	uint32_t raw;
	struct {
		uint32_t bt_sav_vld_3                    : 8;
		uint32_t bt_sav_blk_3                    : 8;
		uint32_t bt_eav_vld_3                    : 8;
		uint32_t bt_eav_blk_3                    : 8;
	} bits;
};

union reg_sensor_mac_vi_48 {
	uint32_t raw;
	struct {
		uint32_t bt_yc_inv                       : 4;
	} bits;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_FIELDS_CSI_MAC_H_ */

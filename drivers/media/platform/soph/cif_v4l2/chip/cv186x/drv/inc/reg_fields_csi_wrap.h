/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name:reg_fields_csi_wrap.h
 * Description:HW register description
 */

#ifndef _REG_FIELDS_CSI_WRAP_H_
#define _REG_FIELDS_CSI_WRAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/*           module definition            */
/******************************************/
union reg_sensor_phy_top_00 {
	uint32_t raw;
	struct {
		uint32_t sensor_phy_mode                 : 3;
	} bits;
};

union reg_sensor_phy_top_04 {
	uint32_t raw;
	struct {
		uint32_t en_clkrx_source                 : 18;
	} bits;
};

union reg_sensor_phy_top_08 {
	uint32_t raw;
	struct {
		uint32_t en_rxbus_clk                    : 18;
	} bits;
};

union reg_sensor_phy_top_10 {
	uint32_t raw;
	struct {
		uint32_t pd_mipi_lane                    : 18;
		uint32_t _rsv_18                         : 2;
		uint32_t pd_pll                          : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t pd_ref_lane                     : 3;
	} bits;
};

union reg_sensor_phy_top_14 {
	uint32_t raw;
	struct {
		uint32_t en_mipi_lpcd                    : 18;
	} bits;
};

union reg_sensor_phy_top_18 {
	uint32_t raw;
	struct {
		uint32_t en_mipi_lptx                    : 18;
	} bits;
};

union reg_sensor_phy_top_1c {
	uint32_t raw;
	struct {
		uint32_t en_mipi_lprx                    : 18;
	} bits;
};

union reg_sensor_phy_top_20 {
	uint32_t raw;
	struct {
		uint32_t en_demux                        : 18;
	} bits;
};

union reg_sensor_phy_top_24 {
	uint32_t raw;
	struct {
		uint32_t en_preamp                       : 18;
	} bits;
};

union reg_sensor_phy_top_28 {
	uint32_t raw;
	struct {
		uint32_t en_vcm_det                      : 18;
	} bits;
};

union reg_sensor_phy_top_2c {
	uint32_t raw;
	struct {
		uint32_t en_hvcmi                        : 18;
	} bits;
};

union reg_sensor_phy_top_30 {
	uint32_t raw;
	struct {
		uint32_t sel_mipi_iq                     : 18;
	} bits;
};

union reg_sensor_phy_top_34 {
	uint32_t raw;
	struct {
		uint32_t en_lvds                         : 18;
	} bits;
};

union reg_sensor_phy_top_38 {
	uint32_t raw;
	struct {
		uint32_t en_lvds_ldo                     : 18;
	} bits;
};

union reg_sensor_phy_top_3c {
	uint32_t raw;
	struct {
		uint32_t en_sublvds                      : 3;
	} bits;
};

union reg_sensor_phy_top_40 {
	uint32_t raw;
	struct {
		uint32_t en_mipi_drv                     : 18;
	} bits;
};

union reg_sensor_phy_top_44 {
	uint32_t raw;
	struct {
		uint32_t en_mipi_ldo                     : 18;
	} bits;
};

union reg_sensor_phy_top_48 {
	uint32_t raw;
	struct {
		uint32_t en_mipi_data_ser                : 18;
	} bits;
};

union reg_sensor_phy_top_4c {
	uint32_t raw;
	struct {
		uint32_t en_clkbusl                      : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t en_clkbusr                      : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t en_clkbusl_to_extl              : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t en_clkbusr_to_extr              : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t en_extl_to_clkbusl              : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t en_extr_to_clkbusr              : 3;
	} bits;
};

union reg_sensor_phy_top_50 {
	uint32_t raw;
	struct {
		uint32_t en_tst                          : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t tst_bypass_vref                 : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t tst_mipi_clkiq_inv              : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t tst_vcmi_det                    : 18;
	} bits;
};

union reg_sensor_phy_top_54 {
	uint32_t raw;
	struct {
		uint32_t en_lckdet                       : 3;
		uint32_t _rsv_3                          : 13;
		uint32_t mipi_txpll_lock                 : 3;
	} bits;
};

union reg_sensor_phy_top_58 {
	uint32_t raw;
	struct {
		uint32_t div_out_sel_0                   : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t div_sel_0                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t disp_divsel_0                   : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t sel_mipi_txpll_ictrl_0          : 4;
		uint32_t loop_div_0                      : 2;
	} bits;
};

union reg_sensor_phy_top_5c {
	uint32_t raw;
	struct {
		uint32_t div_out_sel_1                   : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t div_sel_1                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t disp_divsel_1                   : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t sel_mipi_txpll_ictrl_1          : 4;
		uint32_t loop_div_1                      : 2;
	} bits;
};

union reg_sensor_phy_top_60 {
	uint32_t raw;
	struct {
		uint32_t div_out_sel_2                   : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t div_sel_2                       : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t disp_divsel_2                   : 7;
		uint32_t _rsv_15                         : 1;
		uint32_t sel_mipi_txpll_ictrl_2          : 4;
		uint32_t loop_div_2                      : 2;
	} bits;
};

union reg_sensor_phy_top_64 {
	uint32_t raw;
	struct {
		uint32_t csel_preamp0                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t csel_preamp1                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t csel_preamp2                    : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t csel_preamp3                    : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t csel_preamp4                    : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t csel_preamp5                    : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t csel_preamp6                    : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t csel_preamp7                    : 3;
	} bits;
};

union reg_sensor_phy_top_68 {
	uint32_t raw;
	struct {
		uint32_t csel_preamp8                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t csel_preamp9                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t csel_preamp10                   : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t csel_preamp11                   : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t csel_preamp12                   : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t csel_preamp13                   : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t csel_preamp14                   : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t csel_preamp15                   : 3;
	} bits;
};

union reg_sensor_phy_top_6c {
	uint32_t raw;
	struct {
		uint32_t csel_preamp16                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t csel_preamp17                   : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t swap_rxdata_inv                 : 18;
		uint32_t _rsv_26                         : 2;
		uint32_t en_preamp_hspeed0               : 2;
		uint32_t en_preamp_hspeed1               : 2;
	} bits;
};

union reg_sensor_phy_top_70 {
	uint32_t raw;
	struct {
		uint32_t en_preamp_hspeed2               : 2;
		uint32_t en_preamp_hspeed3               : 2;
		uint32_t en_preamp_hspeed4               : 2;
		uint32_t en_preamp_hspeed5               : 2;
		uint32_t en_preamp_hspeed6               : 2;
		uint32_t en_preamp_hspeed7               : 2;
		uint32_t en_preamp_hspeed8               : 2;
		uint32_t en_preamp_hspeed9               : 2;
		uint32_t en_preamp_hspeed10              : 2;
		uint32_t en_preamp_hspeed11              : 2;
		uint32_t en_preamp_hspeed12              : 2;
		uint32_t en_preamp_hspeed13              : 2;
		uint32_t en_preamp_hspeed14              : 2;
		uint32_t en_preamp_hspeed15              : 2;
		uint32_t en_preamp_hspeed16              : 2;
		uint32_t en_preamp_hspeed17              : 2;
	} bits;
};

union reg_sensor_phy_top_74 {
	uint32_t raw;
	struct {
		uint32_t rsel_preamp0                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t rsel_preamp1                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t rsel_preamp2                    : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t rsel_preamp3                    : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t rsel_preamp4                    : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t rsel_preamp5                    : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t rsel_preamp6                    : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t rsel_preamp7                    : 3;
	} bits;
};

union reg_sensor_phy_top_78 {
	uint32_t raw;
	struct {
		uint32_t rsel_preamp8                    : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t rsel_preamp9                    : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t rsel_preamp10                   : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t rsel_preamp11                   : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t rsel_preamp12                   : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t rsel_preamp13                   : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t rsel_preamp14                   : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t rsel_preamp15                   : 3;
	} bits;
};

union reg_sensor_phy_top_7c {
	uint32_t raw;
	struct {
		uint32_t rsel_preamp16                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t rsel_preamp17                   : 3;
	} bits;
};

union reg_sensor_phy_top_80 {
	uint32_t raw;
	struct {
		uint32_t sel_rxclk_skew0                 : 4;
		uint32_t sel_rxclk_skew1                 : 4;
		uint32_t sel_rxclk_skew2                 : 4;
		uint32_t sel_rxclk_skew3                 : 4;
		uint32_t sel_rxclk_skew4                 : 4;
		uint32_t sel_rxclk_skew5                 : 4;
		uint32_t sel_rxclk_skew6                 : 4;
		uint32_t sel_rxclk_skew7                 : 4;
	} bits;
};

union reg_sensor_phy_top_84 {
	uint32_t raw;
	struct {
		uint32_t sel_rxclk_skew8                 : 4;
		uint32_t sel_rxclk_skew9                 : 4;
		uint32_t sel_rxclk_skew10                : 4;
		uint32_t sel_rxclk_skew11                : 4;
		uint32_t sel_rxclk_skew12                : 4;
		uint32_t sel_rxclk_skew13                : 4;
		uint32_t sel_rxclk_skew14                : 4;
		uint32_t sel_rxclk_skew15                : 4;
	} bits;
};

union reg_sensor_phy_top_88 {
	uint32_t raw;
	struct {
		uint32_t sel_rxclk_skew16                : 4;
		uint32_t sel_rxclk_skew17                : 4;
	} bits;
};

union reg_sensor_phy_top_90 {
	uint32_t raw;
	struct {
		uint32_t gpo_ds0_p                       : 18;
	} bits;
};

union reg_sensor_phy_top_94 {
	uint32_t raw;
	struct {
		uint32_t gpo_ds1_p                       : 18;
	} bits;
};

union reg_sensor_phy_top_98 {
	uint32_t raw;
	struct {
		uint32_t gpo_ds0_n                       : 18;
	} bits;
};

union reg_sensor_phy_top_9c {
	uint32_t raw;
	struct {
		uint32_t gpo_ds1_n                       : 18;
	} bits;
};

union reg_sensor_phy_top_a0 {
	uint32_t raw;
	struct {
		uint32_t gpi_rpu4p7_p                    : 18;
	} bits;
};

union reg_sensor_phy_top_a4 {
	uint32_t raw;
	struct {
		uint32_t gpi_rpu4p7_n                    : 18;
	} bits;
};

union reg_sensor_phy_top_a8 {
	uint32_t raw;
	struct {
		uint32_t gpi_st_p                        : 18;
	} bits;
};

union reg_sensor_phy_top_ac {
	uint32_t raw;
	struct {
		uint32_t gpi_st_n                        : 18;
	} bits;
};

union reg_sensor_phy_top_b0 {
	uint32_t raw;
	struct {
		uint32_t en_mipi_trim0                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t en_mipi_trim1                   : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t en_mipi_trim2                   : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t en_mipi_trim3                   : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t en_mipi_trim4                   : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t en_mipi_trim5                   : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t en_mipi_trim6                   : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t en_mipi_trim7                   : 3;
	} bits;
};

union reg_sensor_phy_top_b4 {
	uint32_t raw;
	struct {
		uint32_t en_mipi_trim8                   : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t en_mipi_trim9                   : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t en_mipi_trim10                  : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t en_mipi_trim11                  : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t en_mipi_trim12                  : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t en_mipi_trim13                  : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t en_mipi_trim14                  : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t en_mipi_trim15                  : 3;
	} bits;
};

union reg_sensor_phy_top_b8 {
	uint32_t raw;
	struct {
		uint32_t en_mipi_trim16                  : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t en_mipi_trim17                  : 3;
		uint32_t _rsv_7                          : 21;
		uint32_t en_mipi_de_drv0                 : 2;
		uint32_t en_mipi_de_drv1                 : 2;
	} bits;
};

union reg_sensor_phy_top_bc {
	uint32_t raw;
	struct {
		uint32_t en_mipi_de_drv2                 : 2;
		uint32_t en_mipi_de_drv3                 : 2;
		uint32_t en_mipi_de_drv4                 : 2;
		uint32_t en_mipi_de_drv5                 : 2;
		uint32_t en_mipi_de_drv6                 : 2;
		uint32_t en_mipi_de_drv7                 : 2;
		uint32_t en_mipi_de_drv8                 : 2;
		uint32_t en_mipi_de_drv9                 : 2;
		uint32_t en_mipi_de_drv10                : 2;
		uint32_t en_mipi_de_drv11                : 2;
		uint32_t en_mipi_de_drv12                : 2;
		uint32_t en_mipi_de_drv13                : 2;
		uint32_t en_mipi_de_drv14                : 2;
		uint32_t en_mipi_de_drv15                : 2;
		uint32_t en_mipi_de_drv16                : 2;
		uint32_t en_mipi_de_drv17                : 2;
	} bits;
};

union reg_sensor_phy_top_c0 {
	uint32_t raw;
	struct {
		uint32_t sel_vref_ldo_0                  : 2;
		uint32_t sel_vref_ldo_1                  : 2;
		uint32_t sel_vref_ldo_2                  : 2;
		uint32_t _rsv_6                          : 2;
		uint32_t en_mipi_ulps                    : 18;
	} bits;
};

union reg_sensor_phy_top_c4 {
	uint32_t raw;
	struct {
		uint32_t ad_clk_inv                      : 18;
	} bits;
};

union reg_sensor_phy_top_d0 {
	uint32_t raw;
	struct {
		uint32_t ad_d0_data                      : 8;
		uint32_t ad_d1_data                      : 8;
		uint32_t ad_d2_data                      : 8;
		uint32_t ad_d3_data                      : 8;
	} bits;
};

union reg_sensor_phy_top_d4 {
	uint32_t raw;
	struct {
		uint32_t ad_d4_data                      : 8;
		uint32_t ad_d5_data                      : 8;
		uint32_t ad_d6_data                      : 8;
		uint32_t ad_d7_data                      : 8;
	} bits;
};

union reg_sensor_phy_top_d8 {
	uint32_t raw;
	struct {
		uint32_t ad_d8_data                      : 8;
		uint32_t ad_d9_data                      : 8;
		uint32_t ad_d10_data                     : 8;
		uint32_t ad_d11_data                     : 8;
	} bits;
};

union reg_sensor_phy_top_dc {
	uint32_t raw;
	struct {
		uint32_t ad_d12_data                     : 8;
		uint32_t ad_d13_data                     : 8;
		uint32_t ad_d14_data                     : 8;
		uint32_t ad_d15_data                     : 8;
	} bits;
};

union reg_sensor_phy_top_e0 {
	uint32_t raw;
	struct {
		uint32_t ad_d16_data                     : 8;
		uint32_t ad_d17_data                     : 8;
	} bits;
};

union reg_sensor_phy_top_e4 {
	uint32_t raw;
	struct {
		uint32_t ad_lpoutn                       : 18;
	} bits;
};

union reg_sensor_phy_top_e8 {
	uint32_t raw;
	struct {
		uint32_t ad_lpoutp                       : 18;
	} bits;
};

union reg_sensor_phy_top_ec {
	uint32_t raw;
	struct {
		uint32_t force_deskew_code0              : 1;
		uint32_t force_deskew_code1              : 1;
		uint32_t force_deskew_code2              : 1;
		uint32_t force_deskew_code3              : 1;
		uint32_t force_deskew_code4              : 1;
		uint32_t force_deskew_code5              : 1;
		uint32_t force_deskew_code6              : 1;
		uint32_t force_deskew_code7              : 1;
		uint32_t force_deskew_code8              : 1;
		uint32_t force_deskew_code9              : 1;
		uint32_t force_deskew_code10             : 1;
		uint32_t force_deskew_code11             : 1;
		uint32_t force_deskew_code12             : 1;
		uint32_t force_deskew_code13             : 1;
		uint32_t force_deskew_code14             : 1;
		uint32_t force_deskew_code15             : 1;
		uint32_t force_deskew_code16             : 1;
		uint32_t force_deskew_code17             : 1;
	} bits;
};

union reg_sensor_phy_top_f0 {
	uint32_t raw;
	struct {
		uint32_t deskew_code0                    : 8;
		uint32_t deskew_code1                    : 8;
		uint32_t deskew_code2                    : 8;
		uint32_t deskew_code3                    : 8;
	} bits;
};

union reg_sensor_phy_top_f4 {
	uint32_t raw;
	struct {
		uint32_t deskew_code4                    : 8;
		uint32_t deskew_code5                    : 8;
		uint32_t deskew_code6                    : 8;
		uint32_t deskew_code7                    : 8;
	} bits;
};

union reg_sensor_phy_top_f8 {
	uint32_t raw;
	struct {
		uint32_t deskew_code8                    : 8;
		uint32_t deskew_code9                    : 8;
		uint32_t deskew_code10                   : 8;
		uint32_t deskew_code11                   : 8;
	} bits;
};

union reg_sensor_phy_top_fc {
	uint32_t raw;
	struct {
		uint32_t deskew_code12                   : 8;
		uint32_t deskew_code13                   : 8;
		uint32_t deskew_code14                   : 8;
		uint32_t deskew_code15                   : 8;
	} bits;
};

union reg_sensor_phy_top_100 {
	uint32_t raw;
	struct {
		uint32_t deskew_code16                   : 8;
		uint32_t deskew_code17                   : 8;
	} bits;
};

union reg_sensor_phy_top_104 {
	uint32_t raw;
	struct {
		uint32_t force_rterm                     : 18;
	} bits;
};

union reg_sensor_phy_top_108 {
	uint32_t raw;
	struct {
		uint32_t en_rterm                        : 18;
	} bits;
};

union reg_sensor_phy_top_10c {
	uint32_t raw;
	struct {
		uint32_t force_rterm_cap                 : 18;
	} bits;
};

union reg_sensor_phy_top_110 {
	uint32_t raw;
	struct {
		uint32_t en_rterm_cap                    : 18;
	} bits;
};

union reg_sensor_phy_top_120 {
	uint32_t raw;
	struct {
		uint32_t cam0_vtt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam0_vs_str                     : 14;
	} bits;
};

union reg_sensor_phy_top_124 {
	uint32_t raw;
	struct {
		uint32_t cam0_vs_stp                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam0_htt                        : 14;
	} bits;
};

union reg_sensor_phy_top_128 {
	uint32_t raw;
	struct {
		uint32_t cam0_hs_str                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam0_hs_stp                     : 14;
	} bits;
};

union reg_sensor_phy_top_12c {
	uint32_t raw;
	struct {
		uint32_t cam0_vs_pol                     : 1;
		uint32_t cam0_hs_pol                     : 1;
		uint32_t cam0_tgen_en                    : 1;
	} bits;
};

union reg_sensor_phy_top_130 {
	uint32_t raw;
	struct {
		uint32_t cam1_vtt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam1_vs_str                     : 14;
	} bits;
};

union reg_sensor_phy_top_134 {
	uint32_t raw;
	struct {
		uint32_t cam1_vs_stp                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam1_htt                        : 14;
	} bits;
};

union reg_sensor_phy_top_138 {
	uint32_t raw;
	struct {
		uint32_t cam1_hs_str                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam1_hs_stp                     : 14;
	} bits;
};

union reg_sensor_phy_top_13c {
	uint32_t raw;
	struct {
		uint32_t cam1_vs_pol                     : 1;
		uint32_t cam1_hs_pol                     : 1;
		uint32_t cam1_tgen_en                    : 1;
	} bits;
};

union reg_sensor_phy_top_140 {
	uint32_t raw;
	struct {
		uint32_t cam2_vtt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam2_vs_str                     : 14;
	} bits;
};

union reg_sensor_phy_top_144 {
	uint32_t raw;
	struct {
		uint32_t cam2_vs_stp                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam2_htt                        : 14;
	} bits;
};

union reg_sensor_phy_top_148 {
	uint32_t raw;
	struct {
		uint32_t cam2_hs_str                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam2_hs_stp                     : 14;
	} bits;
};

union reg_sensor_phy_top_14c {
	uint32_t raw;
	struct {
		uint32_t cam2_vs_pol                     : 1;
		uint32_t cam2_hs_pol                     : 1;
		uint32_t cam2_tgen_en                    : 1;
	} bits;
};

union reg_sensor_phy_top_150 {
	uint32_t raw;
	struct {
		uint32_t cam3_vtt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam3_vs_str                     : 14;
	} bits;
};

union reg_sensor_phy_top_154 {
	uint32_t raw;
	struct {
		uint32_t cam3_vs_stp                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam3_htt                        : 14;
	} bits;
};

union reg_sensor_phy_top_158 {
	uint32_t raw;
	struct {
		uint32_t cam3_hs_str                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam3_hs_stp                     : 14;
	} bits;
};

union reg_sensor_phy_top_15c {
	uint32_t raw;
	struct {
		uint32_t cam3_vs_pol                     : 1;
		uint32_t cam3_hs_pol                     : 1;
		uint32_t cam3_tgen_en                    : 1;
	} bits;
};

union reg_sensor_phy_top_160 {
	uint32_t raw;
	struct {
		uint32_t cam4_vtt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam4_vs_str                     : 14;
	} bits;
};

union reg_sensor_phy_top_164 {
	uint32_t raw;
	struct {
		uint32_t cam4_vs_stp                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam4_htt                        : 14;
	} bits;
};

union reg_sensor_phy_top_168 {
	uint32_t raw;
	struct {
		uint32_t cam4_hs_str                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam4_hs_stp                     : 14;
	} bits;
};

union reg_sensor_phy_top_16c {
	uint32_t raw;
	struct {
		uint32_t cam4_vs_pol                     : 1;
		uint32_t cam4_hs_pol                     : 1;
		uint32_t cam4_tgen_en                    : 1;
	} bits;
};

union reg_sensor_phy_top_170 {
	uint32_t raw;
	struct {
		uint32_t cam5_vtt                        : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam5_vs_str                     : 14;
	} bits;
};

union reg_sensor_phy_top_174 {
	uint32_t raw;
	struct {
		uint32_t cam5_vs_stp                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam5_htt                        : 14;
	} bits;
};

union reg_sensor_phy_top_178 {
	uint32_t raw;
	struct {
		uint32_t cam5_hs_str                     : 14;
		uint32_t _rsv_14                         : 2;
		uint32_t cam5_hs_stp                     : 14;
	} bits;
};

union reg_sensor_phy_top_17c {
	uint32_t raw;
	struct {
		uint32_t cam5_vs_pol                     : 1;
		uint32_t cam5_hs_pol                     : 1;
		uint32_t cam5_tgen_en                    : 1;
	} bits;
};

union reg_sensor_phy_top_dft_180 {
	uint32_t raw;
	struct {
		uint32_t ro_deskew_code0                 : 8;
		uint32_t ro_deskew_code1                 : 8;
		uint32_t ro_deskew_code2                 : 8;
		uint32_t ro_deskew_code3                 : 8;
	} bits;
};

union reg_sensor_phy_top_dft_184 {
	uint32_t raw;
	struct {
		uint32_t ro_deskew_code4                 : 8;
		uint32_t ro_deskew_code5                 : 8;
		uint32_t ro_deskew_code6                 : 8;
		uint32_t ro_deskew_code7                 : 8;
	} bits;
};

union reg_sensor_phy_top_dft_188 {
	uint32_t raw;
	struct {
		uint32_t ro_deskew_code8                 : 8;
		uint32_t ro_deskew_code9                 : 8;
		uint32_t ro_deskew_code10                : 8;
		uint32_t ro_deskew_code11                : 8;
	} bits;
};

union reg_sensor_phy_top_dft_18c {
	uint32_t raw;
	struct {
		uint32_t ro_deskew_code12                : 8;
		uint32_t ro_deskew_code13                : 8;
		uint32_t ro_deskew_code14                : 8;
		uint32_t ro_deskew_code15                : 8;
	} bits;
};

union reg_sensor_phy_top_dft_190 {
	uint32_t raw;
	struct {
		uint32_t ro_deskew_code16                : 8;
		uint32_t ro_deskew_code17                : 8;
	} bits;
};

union reg_sensor_phy_top_dbg_1f0 {
	uint32_t raw;
	struct {
		uint32_t dbg_sel                         : 16;
		uint32_t dbg_ck_sel                      : 8;
	} bits;
};

union reg_sensor_phy_top_200 {
	uint32_t raw;
	struct {
		uint32_t force_gpio_p                    : 18;
	} bits;
};

union reg_sensor_phy_top_204 {
	uint32_t raw;
	struct {
		uint32_t force_gpio_n                    : 18;
	} bits;
};

union reg_sensor_phy_top_208 {
	uint32_t raw;
	struct {
		uint32_t gpi_ie_p                        : 18;
	} bits;
};

union reg_sensor_phy_top_20c {
	uint32_t raw;
	struct {
		uint32_t gpi_ie_n                        : 18;
	} bits;
};

union reg_sensor_phy_top_210 {
	uint32_t raw;
	struct {
		uint32_t gpo_oen_p                       : 18;
	} bits;
};

union reg_sensor_phy_top_214 {
	uint32_t raw;
	struct {
		uint32_t gpo_oen_n                       : 18;
	} bits;
};

union reg_sensor_phy_top_218 {
	uint32_t raw;
	struct {
		uint32_t gpo_i_p                         : 18;
	} bits;
};

union reg_sensor_phy_top_21c {
	uint32_t raw;
	struct {
		uint32_t gpo_i_n                         : 18;
	} bits;
};

union reg_sensor_phy_top_220 {
	uint32_t raw;
	struct {
		uint32_t sw_up                           : 1;
		uint32_t en_ssc                          : 1;
		uint32_t ssc_mode                        : 2;
		uint32_t syn_mode                        : 1;
	} bits;
};

union reg_sensor_phy_top_224 {
	uint32_t raw;
	struct {
		uint32_t set                             : 32;
	} bits;
};

union reg_sensor_phy_top_228 {
	uint32_t raw;
	struct {
		uint32_t span                            : 16;
	} bits;
};

union reg_sensor_phy_top_22c {
	uint32_t raw;
	struct {
		uint32_t step                            : 24;
	} bits;
};

union reg_sensor_phy_top_230 {
	uint32_t raw;
	struct {
		uint32_t mipi_tx_prbs9_en                : 18;
	} bits;
};

union reg_sensor_phy_top_234 {
	uint32_t raw;
	struct {
		uint32_t force_tx_data                   : 18;
	} bits;
};

union reg_sensor_phy_top_238 {
	uint32_t raw;
	struct {
		uint32_t tx_data0                        : 8;
		uint32_t tx_data1                        : 8;
		uint32_t tx_data2                        : 8;
		uint32_t tx_data3                        : 8;
	} bits;
};

union reg_sensor_phy_top_23c {
	uint32_t raw;
	struct {
		uint32_t tx_data4                        : 8;
		uint32_t tx_data5                        : 8;
		uint32_t tx_data6                        : 8;
		uint32_t tx_data7                        : 8;
	} bits;
};

union reg_sensor_phy_top_240 {
	uint32_t raw;
	struct {
		uint32_t tx_data8                        : 8;
		uint32_t tx_data9                        : 8;
		uint32_t tx_data10                       : 8;
		uint32_t tx_data11                       : 8;
	} bits;
};

union reg_sensor_phy_top_244 {
	uint32_t raw;
	struct {
		uint32_t tx_data12                       : 8;
		uint32_t tx_data13                       : 8;
		uint32_t tx_data14                       : 8;
		uint32_t tx_data15                       : 8;
	} bits;
};

union reg_sensor_phy_top_248 {
	uint32_t raw;
	struct {
		uint32_t tx_data16                       : 8;
		uint32_t tx_data17                       : 8;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_sensor_phy_8l_00 {
	uint32_t raw;
	struct {
		uint32_t sensor_mode                     : 2;
	} bits;
};

union reg_sensor_phy_8l_04 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_d0_sel                 : 4;
		uint32_t csi_lane_d1_sel                 : 4;
		uint32_t csi_lane_d2_sel                 : 4;
		uint32_t csi_lane_d3_sel                 : 4;
		uint32_t csi_lane_d4_sel                 : 4;
		uint32_t csi_lane_d5_sel                 : 4;
		uint32_t csi_lane_d6_sel                 : 4;
		uint32_t csi_lane_d7_sel                 : 4;
	} bits;
};

union reg_sensor_phy_8l_08 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_ck_sel                 : 4;
		uint32_t csi_lane_ck_pnswap              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t csi_lane_d0_pnswap              : 1;
		uint32_t csi_lane_d1_pnswap              : 1;
		uint32_t csi_lane_d2_pnswap              : 1;
		uint32_t csi_lane_d3_pnswap              : 1;
		uint32_t csi_lane_d4_pnswap              : 1;
		uint32_t csi_lane_d5_pnswap              : 1;
		uint32_t csi_lane_d6_pnswap              : 1;
		uint32_t csi_lane_d7_pnswap              : 1;
		uint32_t csi_ck_phase                    : 8;
	} bits;
};

union reg_sensor_phy_8l_0c {
	uint32_t raw;
	struct {
		uint32_t deskew_lane_en                  : 8;
		uint32_t _rsv_8                          : 8;
		uint32_t prbs9_test_period               : 16;
	} bits;
};

union reg_sensor_phy_8l_10 {
	uint32_t raw;
	struct {
		uint32_t t_hs_settle                     : 8;
		uint32_t t_all_zero                      : 8;
		uint32_t auto_ignore                     : 1;
		uint32_t auto_sync                       : 1;
	} bits;
};

union reg_sensor_phy_8l_14 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_d0_sel_h               : 1;
		uint32_t csi_lane_d1_sel_h               : 1;
		uint32_t csi_lane_d2_sel_h               : 1;
		uint32_t csi_lane_d3_sel_h               : 1;
		uint32_t csi_lane_d4_sel_h               : 1;
		uint32_t csi_lane_d5_sel_h               : 1;
		uint32_t csi_lane_d6_sel_h               : 1;
		uint32_t csi_lane_d7_sel_h               : 1;
		uint32_t csi_lane_ck_sel_h               : 1;
	} bits;
};

union reg_sensor_phy_8l_20 {
	uint32_t raw;
	struct {
		uint32_t slvds_inv_en                    : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t slvds_bit_mode                  : 2;
		uint32_t slvds_lane_en                   : 8;
		uint32_t slvds_force_resync              : 1;
		uint32_t slvds_resync                    : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t slvds_sav_1st                   : 12;
	} bits;
};

union reg_sensor_phy_8l_24 {
	uint32_t raw;
	struct {
		uint32_t slvds_sav_2nd                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_sav_3rd                   : 12;
	} bits;
};

union reg_sensor_phy_8l_28 {
	uint32_t raw;
	struct {
		uint32_t slvds_d0_sync_state             : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t slvds_d1_sync_state             : 2;
		uint32_t _rsv_6                          : 2;
		uint32_t slvds_d2_sync_state             : 2;
		uint32_t _rsv_10                         : 2;
		uint32_t slvds_d3_sync_state             : 2;
		uint32_t _rsv_14                         : 2;
		uint32_t slvds_d4_sync_state             : 2;
		uint32_t _rsv_18                         : 2;
		uint32_t slvds_d5_sync_state             : 2;
		uint32_t _rsv_22                         : 2;
		uint32_t slvds_d6_sync_state             : 2;
		uint32_t _rsv_26                         : 2;
		uint32_t slvds_d7_sync_state             : 2;
	} bits;
};

union reg_sensor_phy_8l_30 {
	uint32_t raw;
	struct {
		uint32_t slvsec_lane_en                  : 8;
		uint32_t slvsec_skew_cnt_en              : 1;
		uint32_t slvsec_train_seq_chk_en         : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t slvsec_skew_cons                : 5;
		uint32_t slvsec_force_resync             : 1;
		uint32_t slvsec_resync                   : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t slvsec_unstable_skew_cnt        : 8;
	} bits;
};

union reg_sensor_phy_8l_34 {
	uint32_t raw;
	struct {
		uint32_t slvsec_sync_symbol              : 9;
		uint32_t _rsv_9                          : 1;
		uint32_t slvsec_standby_symbol           : 9;
		uint32_t _rsv_19                         : 1;
		uint32_t slvsec_deskew_symbol            : 9;
	} bits;
};

union reg_sensor_phy_8l_38 {
	uint32_t raw;
	struct {
		uint32_t slvsec_prbs9_test_period        : 16;
	} bits;
};

union reg_sensor_phy_8l_3c {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_clr             : 32;
	} bits;
};

union reg_sensor_phy_8l_40 {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_mask            : 32;
	} bits;
};

union reg_sensor_phy_8l_44 {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_status          : 32;
	} bits;
};

union reg_sensor_phy_8l_48 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d0_test_pat_en           : 1;
		uint32_t slvsec_d0_clr_test_pat_err      : 1;
		uint32_t slvsec_d0_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_8l_4c {
	uint32_t raw;
	struct {
		uint32_t slvsec_d0_test_pat_err_cnt      : 16;
		uint32_t slvsec_d0_test_pat_err          : 1;
		uint32_t slvsec_d0_test_pat_pass         : 1;
		uint32_t slvsec_d0_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d0_start_code_err        : 1;
		uint32_t slvsec_d0_end_code_err          : 1;
		uint32_t slvsec_d0_deskew_code_err       : 1;
		uint32_t slvsec_d0_standby_code_err      : 1;
		uint32_t slvsec_d0_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_8l_50 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d1_test_pat_en           : 1;
		uint32_t slvsec_d1_clr_test_pat_err      : 1;
		uint32_t slvsec_d1_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_8l_54 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d1_test_pat_err_cnt      : 16;
		uint32_t slvsec_d1_test_pat_err          : 1;
		uint32_t slvsec_d1_test_pat_pass         : 1;
		uint32_t slvsec_d1_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d1_start_code_err        : 1;
		uint32_t slvsec_d1_end_code_err          : 1;
		uint32_t slvsec_d1_deskew_code_err       : 1;
		uint32_t slvsec_d1_standby_code_err      : 1;
		uint32_t slvsec_d1_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_8l_58 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d2_test_pat_en           : 1;
		uint32_t slvsec_d2_clr_test_pat_err      : 1;
		uint32_t slvsec_d2_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_8l_5c {
	uint32_t raw;
	struct {
		uint32_t slvsec_d2_test_pat_err_cnt      : 16;
		uint32_t slvsec_d2_test_pat_err          : 1;
		uint32_t slvsec_d2_test_pat_pass         : 1;
		uint32_t slvsec_d2_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d2_start_code_err        : 1;
		uint32_t slvsec_d2_end_code_err          : 1;
		uint32_t slvsec_d2_deskew_code_err       : 1;
		uint32_t slvsec_d2_standby_code_err      : 1;
		uint32_t slvsec_d2_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_8l_60 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d3_test_pat_en           : 1;
		uint32_t slvsec_d3_clr_test_pat_err      : 1;
		uint32_t slvsec_d3_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_8l_64 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d3_test_pat_err_cnt      : 16;
		uint32_t slvsec_d3_test_pat_err          : 1;
		uint32_t slvsec_d3_test_pat_pass         : 1;
		uint32_t slvsec_d3_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d3_start_code_err        : 1;
		uint32_t slvsec_d3_end_code_err          : 1;
		uint32_t slvsec_d3_deskew_code_err       : 1;
		uint32_t slvsec_d3_standby_code_err      : 1;
		uint32_t slvsec_d3_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_8l_68 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d4_test_pat_en           : 1;
		uint32_t slvsec_d4_clr_test_pat_err      : 1;
		uint32_t slvsec_d4_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_8l_6c {
	uint32_t raw;
	struct {
		uint32_t slvsec_d4_test_pat_err_cnt      : 16;
		uint32_t slvsec_d4_test_pat_err          : 1;
		uint32_t slvsec_d4_test_pat_pass         : 1;
		uint32_t slvsec_d4_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d4_start_code_err        : 1;
		uint32_t slvsec_d4_end_code_err          : 1;
		uint32_t slvsec_d4_deskew_code_err       : 1;
		uint32_t slvsec_d4_standby_code_err      : 1;
		uint32_t slvsec_d4_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_8l_70 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d5_test_pat_en           : 1;
		uint32_t slvsec_d5_clr_test_pat_err      : 1;
		uint32_t slvsec_d5_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_8l_74 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d5_test_pat_err_cnt      : 16;
		uint32_t slvsec_d5_test_pat_err          : 1;
		uint32_t slvsec_d5_test_pat_pass         : 1;
		uint32_t slvsec_d5_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d5_start_code_err        : 1;
		uint32_t slvsec_d5_end_code_err          : 1;
		uint32_t slvsec_d5_deskew_code_err       : 1;
		uint32_t slvsec_d5_standby_code_err      : 1;
		uint32_t slvsec_d5_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_8l_78 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d6_test_pat_en           : 1;
		uint32_t slvsec_d6_clr_test_pat_err      : 1;
		uint32_t slvsec_d6_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_8l_7c {
	uint32_t raw;
	struct {
		uint32_t slvsec_d6_test_pat_err_cnt      : 16;
		uint32_t slvsec_d6_test_pat_err          : 1;
		uint32_t slvsec_d6_test_pat_pass         : 1;
		uint32_t slvsec_d6_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d6_start_code_err        : 1;
		uint32_t slvsec_d6_end_code_err          : 1;
		uint32_t slvsec_d6_deskew_code_err       : 1;
		uint32_t slvsec_d6_standby_code_err      : 1;
		uint32_t slvsec_d6_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_8l_80 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d7_test_pat_en           : 1;
		uint32_t slvsec_d7_clr_test_pat_err      : 1;
		uint32_t slvsec_d7_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_8l_84 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d7_test_pat_err_cnt      : 16;
		uint32_t slvsec_d7_test_pat_err          : 1;
		uint32_t slvsec_d7_test_pat_pass         : 1;
		uint32_t slvsec_d7_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d7_start_code_err        : 1;
		uint32_t slvsec_d7_end_code_err          : 1;
		uint32_t slvsec_d7_deskew_code_err       : 1;
		uint32_t slvsec_d7_standby_code_err      : 1;
		uint32_t slvsec_d7_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_8l_dbg_90 {
	uint32_t raw;
	struct {
		uint32_t ck_hs_state                     : 1;
		uint32_t ck_ulps_state                   : 1;
		uint32_t ck_stopstate                    : 1;
		uint32_t ck_err_state                    : 1;
		uint32_t deskew_state                    : 2;
	} bits;
};

union reg_sensor_phy_8l_dbg_94 {
	uint32_t raw;
	struct {
		uint32_t d0_datahs_state                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t d1_datahs_state                 : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t d2_datahs_state                 : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t d3_datahs_state                 : 3;
		uint32_t _rsv_15                         : 1;
		uint32_t d4_datahs_state                 : 3;
		uint32_t _rsv_19                         : 1;
		uint32_t d5_datahs_state                 : 3;
		uint32_t _rsv_23                         : 1;
		uint32_t d6_datahs_state                 : 3;
		uint32_t _rsv_27                         : 1;
		uint32_t d7_datahs_state                 : 3;
	} bits;
};

union reg_sensor_phy_8l_status_98 {
	uint32_t raw;
	struct {
		uint32_t ck_lp_status_clr                : 8;
		uint32_t d0_lp_status_clr                : 8;
		uint32_t d1_lp_status_clr                : 8;
		uint32_t d2_lp_status_clr                : 8;
	} bits;
};

union reg_sensor_phy_8l_status_9c {
	uint32_t raw;
	struct {
		uint32_t d3_lp_status_clr                : 8;
		uint32_t d4_lp_status_clr                : 8;
		uint32_t d5_lp_status_clr                : 8;
		uint32_t d6_lp_status_clr                : 8;
	} bits;
};

union reg_sensor_phy_8l_status_a0 {
	uint32_t raw;
	struct {
		uint32_t d7_lp_status_clr                : 8;
	} bits;
};

union reg_sensor_phy_8l_status_a4 {
	uint32_t raw;
	struct {
		uint32_t ck_lp_status_out                : 8;
		uint32_t d0_lp_status_out                : 8;
		uint32_t d1_lp_status_out                : 8;
		uint32_t d2_lp_status_out                : 8;
	} bits;
};

union reg_sensor_phy_8l_status_a8 {
	uint32_t raw;
	struct {
		uint32_t d3_lp_status_out                : 8;
		uint32_t d4_lp_status_out                : 8;
		uint32_t d5_lp_status_out                : 8;
		uint32_t d6_lp_status_out                : 8;
	} bits;
};

union reg_sensor_phy_8l_status_ac {
	uint32_t raw;
	struct {
		uint32_t d7_lp_status_out                : 8;
	} bits;
};

union reg_sensor_phy_8l_d0_0 {
	uint32_t raw;
	struct {
		uint32_t d0_prbs9_en                     : 1;
		uint32_t d0_prbs9_clr_err                : 1;
		uint32_t d0_prbs9_source                 : 1;
		uint32_t d0_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d0_calib_max                    : 8;
		uint32_t d0_calib_step                   : 8;
		uint32_t d0_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_8l_d0_1 {
	uint32_t raw;
	struct {
		uint32_t d0_calib_en                     : 1;
		uint32_t d0_calib_source                 : 1;
		uint32_t d0_calib_mode                   : 1;
		uint32_t d0_calib_ignore                 : 1;
		uint32_t d0_calib_settle                 : 3;
		uint32_t d0_calib_phase_no_shift         : 1;
		uint32_t d0_calib_set_phase              : 8;
		uint32_t d0_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_8l_d0_2 {
	uint32_t raw;
	struct {
		uint32_t d0_prbs9_rx_err                 : 1;
		uint32_t d0_prbs9_test_done              : 1;
		uint32_t d0_prbs9_test_pass              : 1;
		uint32_t d0_skew_calib_done              : 1;
		uint32_t d0_skew_calib_fail              : 1;
		uint32_t d0_datalp_state                 : 4;
		uint32_t d0_datalp_lpreq2err             : 1;
		uint32_t d0_datalp_dataesc2err           : 1;
		uint32_t d0_datalp_rsttri2err            : 1;
		uint32_t d0_datalp_hstest2err            : 1;
		uint32_t d0_datalp_esculp2err            : 1;
		uint32_t d0_datalp_hs2err                : 1;
		uint32_t d0_data_exist_1st_byte          : 1;
		uint32_t d0_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_8l_d0_3 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_8l_d0_4 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_8l_d0_5 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_8l_d0_6 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_8l_d0_7 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_8l_d0_8 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_8l_d0_9 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_8l_d0_a {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_8l_d0_b {
	uint32_t raw;
	struct {
		uint32_t d0_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d0_calib_threshold              : 8;
		uint32_t d0_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_8l_d1_0 {
	uint32_t raw;
	struct {
		uint32_t d1_prbs9_en                     : 1;
		uint32_t d1_prbs9_clr_err                : 1;
		uint32_t d1_prbs9_source                 : 1;
		uint32_t d1_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d1_calib_max                    : 8;
		uint32_t d1_calib_step                   : 8;
		uint32_t d1_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_8l_d1_1 {
	uint32_t raw;
	struct {
		uint32_t d1_calib_en                     : 1;
		uint32_t d1_calib_source                 : 1;
		uint32_t d1_calib_mode                   : 1;
		uint32_t d1_calib_ignore                 : 1;
		uint32_t d1_calib_settle                 : 3;
		uint32_t d1_calib_phase_no_shift         : 1;
		uint32_t d1_calib_set_phase              : 8;
		uint32_t d1_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_8l_d1_2 {
	uint32_t raw;
	struct {
		uint32_t d1_prbs9_rx_err                 : 1;
		uint32_t d1_prbs9_test_done              : 1;
		uint32_t d1_prbs9_test_pass              : 1;
		uint32_t d1_skew_calib_done              : 1;
		uint32_t d1_skew_calib_fail              : 1;
		uint32_t d1_datalp_state                 : 4;
		uint32_t d1_datalp_lpreq2err             : 1;
		uint32_t d1_datalp_dataesc2err           : 1;
		uint32_t d1_datalp_rsttri2err            : 1;
		uint32_t d1_datalp_hstest2err            : 1;
		uint32_t d1_datalp_esculp2err            : 1;
		uint32_t d1_datalp_hs2err                : 1;
		uint32_t d1_data_exist_1st_byte          : 1;
		uint32_t d1_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_8l_d1_3 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_8l_d1_4 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_8l_d1_5 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_8l_d1_6 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_8l_d1_7 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_8l_d1_8 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_8l_d1_9 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_8l_d1_a {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_8l_d1_b {
	uint32_t raw;
	struct {
		uint32_t d1_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d1_calib_threshold              : 8;
		uint32_t d1_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_8l_d2_0 {
	uint32_t raw;
	struct {
		uint32_t d2_prbs9_en                     : 1;
		uint32_t d2_prbs9_clr_err                : 1;
		uint32_t d2_prbs9_source                 : 1;
		uint32_t d2_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d2_calib_max                    : 8;
		uint32_t d2_calib_step                   : 8;
		uint32_t d2_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_8l_d2_1 {
	uint32_t raw;
	struct {
		uint32_t d2_calib_en                     : 1;
		uint32_t d2_calib_source                 : 1;
		uint32_t d2_calib_mode                   : 1;
		uint32_t d2_calib_ignore                 : 1;
		uint32_t d2_calib_settle                 : 3;
		uint32_t d2_calib_phase_no_shift         : 1;
		uint32_t d2_calib_set_phase              : 8;
		uint32_t d2_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_8l_d2_2 {
	uint32_t raw;
	struct {
		uint32_t d2_prbs9_rx_err                 : 1;
		uint32_t d2_prbs9_test_done              : 1;
		uint32_t d2_prbs9_test_pass              : 1;
		uint32_t d2_skew_calib_done              : 1;
		uint32_t d2_skew_calib_fail              : 1;
		uint32_t d2_datalp_state                 : 4;
		uint32_t d2_datalp_lpreq2err             : 1;
		uint32_t d2_datalp_dataesc2err           : 1;
		uint32_t d2_datalp_rsttri2err            : 1;
		uint32_t d2_datalp_hstest2err            : 1;
		uint32_t d2_datalp_esculp2err            : 1;
		uint32_t d2_datalp_hs2err                : 1;
		uint32_t d2_data_exist_1st_byte          : 1;
		uint32_t d2_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_8l_d2_3 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_8l_d2_4 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_8l_d2_5 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_8l_d2_6 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_8l_d2_7 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_8l_d2_8 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_8l_d2_9 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_8l_d2_a {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_8l_d2_b {
	uint32_t raw;
	struct {
		uint32_t d2_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d2_calib_threshold              : 8;
		uint32_t d2_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_8l_d3_0 {
	uint32_t raw;
	struct {
		uint32_t d3_prbs9_en                     : 1;
		uint32_t d3_prbs9_clr_err                : 1;
		uint32_t d3_prbs9_source                 : 1;
		uint32_t d3_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d3_calib_max                    : 8;
		uint32_t d3_calib_step                   : 8;
		uint32_t d3_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_8l_d3_1 {
	uint32_t raw;
	struct {
		uint32_t d3_calib_en                     : 1;
		uint32_t d3_calib_source                 : 1;
		uint32_t d3_calib_mode                   : 1;
		uint32_t d3_calib_ignore                 : 1;
		uint32_t d3_calib_settle                 : 3;
		uint32_t d3_calib_phase_no_shift         : 1;
		uint32_t d3_calib_set_phase              : 8;
		uint32_t d3_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_8l_d3_2 {
	uint32_t raw;
	struct {
		uint32_t d3_prbs9_rx_err                 : 1;
		uint32_t d3_prbs9_test_done              : 1;
		uint32_t d3_prbs9_test_pass              : 1;
		uint32_t d3_skew_calib_done              : 1;
		uint32_t d3_skew_calib_fail              : 1;
		uint32_t d3_datalp_state                 : 4;
		uint32_t d3_datalp_lpreq2err             : 1;
		uint32_t d3_datalp_dataesc2err           : 1;
		uint32_t d3_datalp_rsttri2err            : 1;
		uint32_t d3_datalp_hstest2err            : 1;
		uint32_t d3_datalp_esculp2err            : 1;
		uint32_t d3_datalp_hs2err                : 1;
		uint32_t d3_data_exist_1st_byte          : 1;
		uint32_t d3_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_8l_d3_3 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_8l_d3_4 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_8l_d3_5 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_8l_d3_6 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_8l_d3_7 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_8l_d3_8 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_8l_d3_9 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_8l_d3_a {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_8l_d3_b {
	uint32_t raw;
	struct {
		uint32_t d3_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d3_calib_threshold              : 8;
		uint32_t d3_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_8l_d4_0 {
	uint32_t raw;
	struct {
		uint32_t d4_prbs9_en                     : 1;
		uint32_t d4_prbs9_clr_err                : 1;
		uint32_t d4_prbs9_source                 : 1;
		uint32_t d4_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d4_calib_max                    : 8;
		uint32_t d4_calib_step                   : 8;
		uint32_t d4_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_8l_d4_1 {
	uint32_t raw;
	struct {
		uint32_t d4_calib_en                     : 1;
		uint32_t d4_calib_source                 : 1;
		uint32_t d4_calib_mode                   : 1;
		uint32_t d4_calib_ignore                 : 1;
		uint32_t d4_calib_settle                 : 3;
		uint32_t d4_calib_phase_no_shift         : 1;
		uint32_t d4_calib_set_phase              : 8;
		uint32_t d4_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_8l_d4_2 {
	uint32_t raw;
	struct {
		uint32_t d4_prbs9_rx_err                 : 1;
		uint32_t d4_prbs9_test_done              : 1;
		uint32_t d4_prbs9_test_pass              : 1;
		uint32_t d4_skew_calib_done              : 1;
		uint32_t d4_skew_calib_fail              : 1;
		uint32_t d4_datalp_state                 : 4;
		uint32_t d4_datalp_lpreq2err             : 1;
		uint32_t d4_datalp_dataesc2err           : 1;
		uint32_t d4_datalp_rsttri2err            : 1;
		uint32_t d4_datalp_hstest2err            : 1;
		uint32_t d4_datalp_esculp2err            : 1;
		uint32_t d4_datalp_hs2err                : 1;
		uint32_t d4_data_exist_1st_byte          : 1;
		uint32_t d4_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_8l_d4_3 {
	uint32_t raw;
	struct {
		uint32_t d4_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_8l_d4_4 {
	uint32_t raw;
	struct {
		uint32_t d4_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_8l_d4_5 {
	uint32_t raw;
	struct {
		uint32_t d4_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_8l_d4_6 {
	uint32_t raw;
	struct {
		uint32_t d4_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_8l_d4_7 {
	uint32_t raw;
	struct {
		uint32_t d4_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_8l_d4_8 {
	uint32_t raw;
	struct {
		uint32_t d4_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_8l_d4_9 {
	uint32_t raw;
	struct {
		uint32_t d4_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_8l_d4_a {
	uint32_t raw;
	struct {
		uint32_t d4_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_8l_d4_b {
	uint32_t raw;
	struct {
		uint32_t d4_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d4_calib_threshold              : 8;
		uint32_t d4_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_8l_d5_0 {
	uint32_t raw;
	struct {
		uint32_t d5_prbs9_en                     : 1;
		uint32_t d5_prbs9_clr_err                : 1;
		uint32_t d5_prbs9_source                 : 1;
		uint32_t d5_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d5_calib_max                    : 8;
		uint32_t d5_calib_step                   : 8;
		uint32_t d5_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_8l_d5_1 {
	uint32_t raw;
	struct {
		uint32_t d5_calib_en                     : 1;
		uint32_t d5_calib_source                 : 1;
		uint32_t d5_calib_mode                   : 1;
		uint32_t d5_calib_ignore                 : 1;
		uint32_t d5_calib_settle                 : 3;
		uint32_t d5_calib_phase_no_shift         : 1;
		uint32_t d5_calib_set_phase              : 8;
		uint32_t d5_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_8l_d5_2 {
	uint32_t raw;
	struct {
		uint32_t d5_prbs9_rx_err                 : 1;
		uint32_t d5_prbs9_test_done              : 1;
		uint32_t d5_prbs9_test_pass              : 1;
		uint32_t d5_skew_calib_done              : 1;
		uint32_t d5_skew_calib_fail              : 1;
		uint32_t d5_datalp_state                 : 4;
		uint32_t d5_datalp_lpreq2err             : 1;
		uint32_t d5_datalp_dataesc2err           : 1;
		uint32_t d5_datalp_rsttri2err            : 1;
		uint32_t d5_datalp_hstest2err            : 1;
		uint32_t d5_datalp_esculp2err            : 1;
		uint32_t d5_datalp_hs2err                : 1;
		uint32_t d5_data_exist_1st_byte          : 1;
		uint32_t d5_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_8l_d5_3 {
	uint32_t raw;
	struct {
		uint32_t d5_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_8l_d5_4 {
	uint32_t raw;
	struct {
		uint32_t d5_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_8l_d5_5 {
	uint32_t raw;
	struct {
		uint32_t d5_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_8l_d5_6 {
	uint32_t raw;
	struct {
		uint32_t d5_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_8l_d5_7 {
	uint32_t raw;
	struct {
		uint32_t d5_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_8l_d5_8 {
	uint32_t raw;
	struct {
		uint32_t d5_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_8l_d5_9 {
	uint32_t raw;
	struct {
		uint32_t d5_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_8l_d5_a {
	uint32_t raw;
	struct {
		uint32_t d5_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_8l_d5_b {
	uint32_t raw;
	struct {
		uint32_t d5_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d5_calib_threshold              : 8;
		uint32_t d5_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_8l_d6_0 {
	uint32_t raw;
	struct {
		uint32_t d6_prbs9_en                     : 1;
		uint32_t d6_prbs9_clr_err                : 1;
		uint32_t d6_prbs9_source                 : 1;
		uint32_t d6_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d6_calib_max                    : 8;
		uint32_t d6_calib_step                   : 8;
		uint32_t d6_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_8l_d6_1 {
	uint32_t raw;
	struct {
		uint32_t d6_calib_en                     : 1;
		uint32_t d6_calib_source                 : 1;
		uint32_t d6_calib_mode                   : 1;
		uint32_t d6_calib_ignore                 : 1;
		uint32_t d6_calib_settle                 : 3;
		uint32_t d6_calib_phase_no_shift         : 1;
		uint32_t d6_calib_set_phase              : 8;
		uint32_t d6_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_8l_d6_2 {
	uint32_t raw;
	struct {
		uint32_t d6_prbs9_rx_err                 : 1;
		uint32_t d6_prbs9_test_done              : 1;
		uint32_t d6_prbs9_test_pass              : 1;
		uint32_t d6_skew_calib_done              : 1;
		uint32_t d6_skew_calib_fail              : 1;
		uint32_t d6_datalp_state                 : 4;
		uint32_t d6_datalp_lpreq2err             : 1;
		uint32_t d6_datalp_dataesc2err           : 1;
		uint32_t d6_datalp_rsttri2err            : 1;
		uint32_t d6_datalp_hstest2err            : 1;
		uint32_t d6_datalp_esculp2err            : 1;
		uint32_t d6_datalp_hs2err                : 1;
		uint32_t d6_data_exist_1st_byte          : 1;
		uint32_t d6_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_8l_d6_3 {
	uint32_t raw;
	struct {
		uint32_t d6_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_8l_d6_4 {
	uint32_t raw;
	struct {
		uint32_t d6_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_8l_d6_5 {
	uint32_t raw;
	struct {
		uint32_t d6_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_8l_d6_6 {
	uint32_t raw;
	struct {
		uint32_t d6_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_8l_d6_7 {
	uint32_t raw;
	struct {
		uint32_t d6_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_8l_d6_8 {
	uint32_t raw;
	struct {
		uint32_t d6_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_8l_d6_9 {
	uint32_t raw;
	struct {
		uint32_t d6_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_8l_d6_a {
	uint32_t raw;
	struct {
		uint32_t d6_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_8l_d6_b {
	uint32_t raw;
	struct {
		uint32_t d6_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d6_calib_threshold              : 8;
		uint32_t d6_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_8l_d7_0 {
	uint32_t raw;
	struct {
		uint32_t d7_prbs9_en                     : 1;
		uint32_t d7_prbs9_clr_err                : 1;
		uint32_t d7_prbs9_source                 : 1;
		uint32_t d7_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d7_calib_max                    : 8;
		uint32_t d7_calib_step                   : 8;
		uint32_t d7_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_8l_d7_1 {
	uint32_t raw;
	struct {
		uint32_t d7_calib_en                     : 1;
		uint32_t d7_calib_source                 : 1;
		uint32_t d7_calib_mode                   : 1;
		uint32_t d7_calib_ignore                 : 1;
		uint32_t d7_calib_settle                 : 3;
		uint32_t d7_calib_phase_no_shift         : 1;
		uint32_t d7_calib_set_phase              : 8;
		uint32_t d7_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_8l_d7_2 {
	uint32_t raw;
	struct {
		uint32_t d7_prbs9_rx_err                 : 1;
		uint32_t d7_prbs9_test_done              : 1;
		uint32_t d7_prbs9_test_pass              : 1;
		uint32_t d7_skew_calib_done              : 1;
		uint32_t d7_skew_calib_fail              : 1;
		uint32_t d7_datalp_state                 : 4;
		uint32_t d7_datalp_lpreq2err             : 1;
		uint32_t d7_datalp_dataesc2err           : 1;
		uint32_t d7_datalp_rsttri2err            : 1;
		uint32_t d7_datalp_hstest2err            : 1;
		uint32_t d7_datalp_esculp2err            : 1;
		uint32_t d7_datalp_hs2err                : 1;
		uint32_t d7_data_exist_1st_byte          : 1;
		uint32_t d7_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_8l_d7_3 {
	uint32_t raw;
	struct {
		uint32_t d7_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_8l_d7_4 {
	uint32_t raw;
	struct {
		uint32_t d7_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_8l_d7_5 {
	uint32_t raw;
	struct {
		uint32_t d7_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_8l_d7_6 {
	uint32_t raw;
	struct {
		uint32_t d7_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_8l_d7_7 {
	uint32_t raw;
	struct {
		uint32_t d7_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_8l_d7_8 {
	uint32_t raw;
	struct {
		uint32_t d7_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_8l_d7_9 {
	uint32_t raw;
	struct {
		uint32_t d7_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_8l_d7_a {
	uint32_t raw;
	struct {
		uint32_t d7_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_8l_d7_b {
	uint32_t raw;
	struct {
		uint32_t d7_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d7_calib_threshold              : 8;
		uint32_t d7_calib_gp_count               : 9;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_sensor_phy_4l_00 {
	uint32_t raw;
	struct {
		uint32_t sensor_mode                     : 2;
	} bits;
};

union reg_sensor_phy_4l_04 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_d0_sel                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t csi_lane_d1_sel                 : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t csi_lane_d2_sel                 : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t csi_lane_d3_sel                 : 3;
	} bits;
};

union reg_sensor_phy_4l_08 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_ck_sel                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t csi_lane_ck_pnswap              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t csi_lane_d0_pnswap              : 1;
		uint32_t csi_lane_d1_pnswap              : 1;
		uint32_t csi_lane_d2_pnswap              : 1;
		uint32_t csi_lane_d3_pnswap              : 1;
		uint32_t _rsv_12                         : 4;
		uint32_t csi_ck_phase                    : 8;
	} bits;
};

union reg_sensor_phy_4l_0c {
	uint32_t raw;
	struct {
		uint32_t deskew_lane_en                  : 4;
		uint32_t _rsv_4                          : 12;
		uint32_t prbs9_test_period               : 16;
	} bits;
};

union reg_sensor_phy_4l_10 {
	uint32_t raw;
	struct {
		uint32_t t_hs_settle                     : 8;
		uint32_t t_all_zero                      : 8;
		uint32_t auto_ignore                     : 1;
		uint32_t auto_sync                       : 1;
	} bits;
};

union reg_sensor_phy_4l_20 {
	uint32_t raw;
	struct {
		uint32_t slvds_inv_en                    : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t slvds_bit_mode                  : 2;
		uint32_t slvds_lane_en                   : 4;
		uint32_t _rsv_8                          : 4;
		uint32_t slvds_force_resync              : 1;
		uint32_t slvds_resync                    : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t slvds_sav_1st                   : 12;
	} bits;
};

union reg_sensor_phy_4l_24 {
	uint32_t raw;
	struct {
		uint32_t slvds_sav_2nd                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_sav_3rd                   : 12;
	} bits;
};

union reg_sensor_phy_4l_28 {
	uint32_t raw;
	struct {
		uint32_t slvds_d0_sync_state             : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t slvds_d1_sync_state             : 2;
		uint32_t _rsv_6                          : 2;
		uint32_t slvds_d2_sync_state             : 2;
		uint32_t _rsv_10                         : 2;
		uint32_t slvds_d3_sync_state             : 2;
	} bits;
};

union reg_sensor_phy_4l_30 {
	uint32_t raw;
	struct {
		uint32_t slvsec_lane_en                  : 4;
		uint32_t _rsv_4                          : 4;
		uint32_t slvsec_skew_cnt_en              : 1;
		uint32_t slvsec_train_seq_chk_en         : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t slvsec_skew_cons                : 5;
		uint32_t slvsec_force_resync             : 1;
		uint32_t slvsec_resync                   : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t slvsec_unstable_skew_cnt        : 8;
	} bits;
};

union reg_sensor_phy_4l_34 {
	uint32_t raw;
	struct {
		uint32_t slvsec_sync_symbol              : 9;
		uint32_t _rsv_9                          : 1;
		uint32_t slvsec_standby_symbol           : 9;
		uint32_t _rsv_19                         : 1;
		uint32_t slvsec_deskew_symbol            : 9;
	} bits;
};

union reg_sensor_phy_4l_38 {
	uint32_t raw;
	struct {
		uint32_t slvsec_prbs9_test_period        : 16;
	} bits;
};

union reg_sensor_phy_4l_3c {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_clr             : 16;
	} bits;
};

union reg_sensor_phy_4l_40 {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_mask            : 16;
	} bits;
};

union reg_sensor_phy_4l_44 {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_status          : 16;
	} bits;
};

union reg_sensor_phy_4l_48 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d0_test_pat_en           : 1;
		uint32_t slvsec_d0_clr_test_pat_err      : 1;
		uint32_t slvsec_d0_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_4l_4c {
	uint32_t raw;
	struct {
		uint32_t slvsec_d0_test_pat_err_cnt      : 16;
		uint32_t slvsec_d0_test_pat_err          : 1;
		uint32_t slvsec_d0_test_pat_pass         : 1;
		uint32_t slvsec_d0_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d0_start_code_err        : 1;
		uint32_t slvsec_d0_end_code_err          : 1;
		uint32_t slvsec_d0_deskew_code_err       : 1;
		uint32_t slvsec_d0_standby_code_err      : 1;
		uint32_t slvsec_d0_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_4l_50 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d1_test_pat_en           : 1;
		uint32_t slvsec_d1_clr_test_pat_err      : 1;
		uint32_t slvsec_d1_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_4l_54 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d1_test_pat_err_cnt      : 16;
		uint32_t slvsec_d1_test_pat_err          : 1;
		uint32_t slvsec_d1_test_pat_pass         : 1;
		uint32_t slvsec_d1_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d1_start_code_err        : 1;
		uint32_t slvsec_d1_end_code_err          : 1;
		uint32_t slvsec_d1_deskew_code_err       : 1;
		uint32_t slvsec_d1_standby_code_err      : 1;
		uint32_t slvsec_d1_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_4l_58 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d2_test_pat_en           : 1;
		uint32_t slvsec_d2_clr_test_pat_err      : 1;
		uint32_t slvsec_d2_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_4l_5c {
	uint32_t raw;
	struct {
		uint32_t slvsec_d2_test_pat_err_cnt      : 16;
		uint32_t slvsec_d2_test_pat_err          : 1;
		uint32_t slvsec_d2_test_pat_pass         : 1;
		uint32_t slvsec_d2_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d2_start_code_err        : 1;
		uint32_t slvsec_d2_end_code_err          : 1;
		uint32_t slvsec_d2_deskew_code_err       : 1;
		uint32_t slvsec_d2_standby_code_err      : 1;
		uint32_t slvsec_d2_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_4l_60 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d3_test_pat_en           : 1;
		uint32_t slvsec_d3_clr_test_pat_err      : 1;
		uint32_t slvsec_d3_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_4l_64 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d3_test_pat_err_cnt      : 16;
		uint32_t slvsec_d3_test_pat_err          : 1;
		uint32_t slvsec_d3_test_pat_pass         : 1;
		uint32_t slvsec_d3_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d3_start_code_err        : 1;
		uint32_t slvsec_d3_end_code_err          : 1;
		uint32_t slvsec_d3_deskew_code_err       : 1;
		uint32_t slvsec_d3_standby_code_err      : 1;
		uint32_t slvsec_d3_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_4l_dbg_90 {
	uint32_t raw;
	struct {
		uint32_t ck_hs_state                     : 1;
		uint32_t ck_ulps_state                   : 1;
		uint32_t ck_stopstate                    : 1;
		uint32_t ck_err_state                    : 1;
		uint32_t deskew_state                    : 2;
	} bits;
};

union reg_sensor_phy_4l_dbg_94 {
	uint32_t raw;
	struct {
		uint32_t d0_datahs_state                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t d1_datahs_state                 : 3;
		uint32_t _rsv_7                          : 1;
		uint32_t d2_datahs_state                 : 3;
		uint32_t _rsv_11                         : 1;
		uint32_t d3_datahs_state                 : 3;
	} bits;
};

union reg_sensor_phy_4l_status_98 {
	uint32_t raw;
	struct {
		uint32_t ck_lp_status_clr                : 8;
		uint32_t d0_lp_status_clr                : 8;
		uint32_t d1_lp_status_clr                : 8;
		uint32_t d2_lp_status_clr                : 8;
	} bits;
};

union reg_sensor_phy_4l_status_9c {
	uint32_t raw;
	struct {
		uint32_t d3_lp_status_clr                : 8;
	} bits;
};

union reg_sensor_phy_4l_status_a4 {
	uint32_t raw;
	struct {
		uint32_t ck_lp_status_out                : 8;
		uint32_t d0_lp_status_out                : 8;
		uint32_t d1_lp_status_out                : 8;
		uint32_t d2_lp_status_out                : 8;
	} bits;
};

union reg_sensor_phy_4l_status_a8 {
	uint32_t raw;
	struct {
		uint32_t d3_lp_status_out                : 8;
	} bits;
};

union reg_sensor_phy_4l_d0_0 {
	uint32_t raw;
	struct {
		uint32_t d0_prbs9_en                     : 1;
		uint32_t d0_prbs9_clr_err                : 1;
		uint32_t d0_prbs9_source                 : 1;
		uint32_t d0_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d0_calib_max                    : 8;
		uint32_t d0_calib_step                   : 8;
		uint32_t d0_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_4l_d0_1 {
	uint32_t raw;
	struct {
		uint32_t d0_calib_en                     : 1;
		uint32_t d0_calib_source                 : 1;
		uint32_t d0_calib_mode                   : 1;
		uint32_t d0_calib_ignore                 : 1;
		uint32_t d0_calib_settle                 : 3;
		uint32_t d0_calib_phase_no_shift         : 1;
		uint32_t d0_calib_set_phase              : 8;
		uint32_t d0_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_4l_d0_2 {
	uint32_t raw;
	struct {
		uint32_t d0_prbs9_rx_err                 : 1;
		uint32_t d0_prbs9_test_done              : 1;
		uint32_t d0_prbs9_test_pass              : 1;
		uint32_t d0_skew_calib_done              : 1;
		uint32_t d0_skew_calib_fail              : 1;
		uint32_t d0_datalp_state                 : 4;
		uint32_t d0_datalp_lpreq2err             : 1;
		uint32_t d0_datalp_dataesc2err           : 1;
		uint32_t d0_datalp_rsttri2err            : 1;
		uint32_t d0_datalp_hstest2err            : 1;
		uint32_t d0_datalp_esculp2err            : 1;
		uint32_t d0_datalp_hs2err                : 1;
		uint32_t d0_data_exist_1st_byte          : 1;
		uint32_t d0_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_4l_d0_3 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_4l_d0_4 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_4l_d0_5 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_4l_d0_6 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_4l_d0_7 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_4l_d0_8 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_4l_d0_9 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_4l_d0_a {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_4l_d0_b {
	uint32_t raw;
	struct {
		uint32_t d0_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d0_calib_threshold              : 8;
		uint32_t d0_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_4l_d1_0 {
	uint32_t raw;
	struct {
		uint32_t d1_prbs9_en                     : 1;
		uint32_t d1_prbs9_clr_err                : 1;
		uint32_t d1_prbs9_source                 : 1;
		uint32_t d1_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d1_calib_max                    : 8;
		uint32_t d1_calib_step                   : 8;
		uint32_t d1_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_4l_d1_1 {
	uint32_t raw;
	struct {
		uint32_t d1_calib_en                     : 1;
		uint32_t d1_calib_source                 : 1;
		uint32_t d1_calib_mode                   : 1;
		uint32_t d1_calib_ignore                 : 1;
		uint32_t d1_calib_settle                 : 3;
		uint32_t d1_calib_phase_no_shift         : 1;
		uint32_t d1_calib_set_phase              : 8;
		uint32_t d1_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_4l_d1_2 {
	uint32_t raw;
	struct {
		uint32_t d1_prbs9_rx_err                 : 1;
		uint32_t d1_prbs9_test_done              : 1;
		uint32_t d1_prbs9_test_pass              : 1;
		uint32_t d1_skew_calib_done              : 1;
		uint32_t d1_skew_calib_fail              : 1;
		uint32_t d1_datalp_state                 : 4;
		uint32_t d1_datalp_lpreq2err             : 1;
		uint32_t d1_datalp_dataesc2err           : 1;
		uint32_t d1_datalp_rsttri2err            : 1;
		uint32_t d1_datalp_hstest2err            : 1;
		uint32_t d1_datalp_esculp2err            : 1;
		uint32_t d1_datalp_hs2err                : 1;
		uint32_t d1_data_exist_1st_byte          : 1;
		uint32_t d1_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_4l_d1_3 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_4l_d1_4 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_4l_d1_5 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_4l_d1_6 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_4l_d1_7 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_4l_d1_8 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_4l_d1_9 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_4l_d1_a {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_4l_d1_b {
	uint32_t raw;
	struct {
		uint32_t d1_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d1_calib_threshold              : 8;
		uint32_t d1_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_4l_d2_0 {
	uint32_t raw;
	struct {
		uint32_t d2_prbs9_en                     : 1;
		uint32_t d2_prbs9_clr_err                : 1;
		uint32_t d2_prbs9_source                 : 1;
		uint32_t d2_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d2_calib_max                    : 8;
		uint32_t d2_calib_step                   : 8;
		uint32_t d2_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_4l_d2_1 {
	uint32_t raw;
	struct {
		uint32_t d2_calib_en                     : 1;
		uint32_t d2_calib_source                 : 1;
		uint32_t d2_calib_mode                   : 1;
		uint32_t d2_calib_ignore                 : 1;
		uint32_t d2_calib_settle                 : 3;
		uint32_t d2_calib_phase_no_shift         : 1;
		uint32_t d2_calib_set_phase              : 8;
		uint32_t d2_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_4l_d2_2 {
	uint32_t raw;
	struct {
		uint32_t d2_prbs9_rx_err                 : 1;
		uint32_t d2_prbs9_test_done              : 1;
		uint32_t d2_prbs9_test_pass              : 1;
		uint32_t d2_skew_calib_done              : 1;
		uint32_t d2_skew_calib_fail              : 1;
		uint32_t d2_datalp_state                 : 4;
		uint32_t d2_datalp_lpreq2err             : 1;
		uint32_t d2_datalp_dataesc2err           : 1;
		uint32_t d2_datalp_rsttri2err            : 1;
		uint32_t d2_datalp_hstest2err            : 1;
		uint32_t d2_datalp_esculp2err            : 1;
		uint32_t d2_datalp_hs2err                : 1;
		uint32_t d2_data_exist_1st_byte          : 1;
		uint32_t d2_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_4l_d2_3 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_4l_d2_4 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_4l_d2_5 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_4l_d2_6 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_4l_d2_7 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_4l_d2_8 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_4l_d2_9 {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_4l_d2_a {
	uint32_t raw;
	struct {
		uint32_t d2_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_4l_d2_b {
	uint32_t raw;
	struct {
		uint32_t d2_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d2_calib_threshold              : 8;
		uint32_t d2_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_4l_d3_0 {
	uint32_t raw;
	struct {
		uint32_t d3_prbs9_en                     : 1;
		uint32_t d3_prbs9_clr_err                : 1;
		uint32_t d3_prbs9_source                 : 1;
		uint32_t d3_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d3_calib_max                    : 8;
		uint32_t d3_calib_step                   : 8;
		uint32_t d3_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_4l_d3_1 {
	uint32_t raw;
	struct {
		uint32_t d3_calib_en                     : 1;
		uint32_t d3_calib_source                 : 1;
		uint32_t d3_calib_mode                   : 1;
		uint32_t d3_calib_ignore                 : 1;
		uint32_t d3_calib_settle                 : 3;
		uint32_t d3_calib_phase_no_shift         : 1;
		uint32_t d3_calib_set_phase              : 8;
		uint32_t d3_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_4l_d3_2 {
	uint32_t raw;
	struct {
		uint32_t d3_prbs9_rx_err                 : 1;
		uint32_t d3_prbs9_test_done              : 1;
		uint32_t d3_prbs9_test_pass              : 1;
		uint32_t d3_skew_calib_done              : 1;
		uint32_t d3_skew_calib_fail              : 1;
		uint32_t d3_datalp_state                 : 4;
		uint32_t d3_datalp_lpreq2err             : 1;
		uint32_t d3_datalp_dataesc2err           : 1;
		uint32_t d3_datalp_rsttri2err            : 1;
		uint32_t d3_datalp_hstest2err            : 1;
		uint32_t d3_datalp_esculp2err            : 1;
		uint32_t d3_datalp_hs2err                : 1;
		uint32_t d3_data_exist_1st_byte          : 1;
		uint32_t d3_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_4l_d3_3 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_4l_d3_4 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_4l_d3_5 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_4l_d3_6 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_4l_d3_7 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_4l_d3_8 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_4l_d3_9 {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_4l_d3_a {
	uint32_t raw;
	struct {
		uint32_t d3_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_4l_d3_b {
	uint32_t raw;
	struct {
		uint32_t d3_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d3_calib_threshold              : 8;
		uint32_t d3_calib_gp_count               : 9;
	} bits;
};

/******************************************/
/*           module definition            */
/******************************************/
union reg_sensor_phy_2l_00 {
	uint32_t raw;
	struct {
		uint32_t sensor_mode                     : 2;
	} bits;
};

union reg_sensor_phy_2l_04 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_d0_sel                 : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t csi_lane_d1_sel                 : 2;
	} bits;
};

union reg_sensor_phy_2l_08 {
	uint32_t raw;
	struct {
		uint32_t csi_lane_ck_sel                 : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t csi_lane_ck_pnswap              : 1;
		uint32_t _rsv_5                          : 3;
		uint32_t csi_lane_d0_pnswap              : 1;
		uint32_t csi_lane_d1_pnswap              : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t csi_ck_phase                    : 8;
	} bits;
};

union reg_sensor_phy_2l_0c {
	uint32_t raw;
	struct {
		uint32_t deskew_lane_en                  : 2;
		uint32_t _rsv_2                          : 14;
		uint32_t prbs9_test_period               : 16;
	} bits;
};

union reg_sensor_phy_2l_10 {
	uint32_t raw;
	struct {
		uint32_t t_hs_settle                     : 8;
		uint32_t t_all_zero                      : 8;
		uint32_t auto_ignore                     : 1;
		uint32_t auto_sync                       : 1;
	} bits;
};

union reg_sensor_phy_2l_20 {
	uint32_t raw;
	struct {
		uint32_t slvds_inv_en                    : 1;
		uint32_t _rsv_1                          : 1;
		uint32_t slvds_bit_mode                  : 2;
		uint32_t slvds_lane_en                   : 2;
		uint32_t _rsv_6                          : 6;
		uint32_t slvds_force_resync              : 1;
		uint32_t slvds_resync                    : 1;
		uint32_t _rsv_14                         : 2;
		uint32_t slvds_sav_1st                   : 12;
	} bits;
};

union reg_sensor_phy_2l_24 {
	uint32_t raw;
	struct {
		uint32_t slvds_sav_2nd                   : 12;
		uint32_t _rsv_12                         : 4;
		uint32_t slvds_sav_3rd                   : 12;
	} bits;
};

union reg_sensor_phy_2l_28 {
	uint32_t raw;
	struct {
		uint32_t slvds_d0_sync_state             : 2;
		uint32_t _rsv_2                          : 2;
		uint32_t slvds_d1_sync_state             : 2;
	} bits;
};

union reg_sensor_phy_2l_30 {
	uint32_t raw;
	struct {
		uint32_t slvsec_lane_en                  : 2;
		uint32_t _rsv_2                          : 6;
		uint32_t slvsec_skew_cnt_en              : 1;
		uint32_t slvsec_train_seq_chk_en         : 1;
		uint32_t _rsv_10                         : 6;
		uint32_t slvsec_skew_cons                : 5;
		uint32_t slvsec_force_resync             : 1;
		uint32_t slvsec_resync                   : 1;
		uint32_t _rsv_23                         : 1;
		uint32_t slvsec_unstable_skew_cnt        : 8;
	} bits;
};

union reg_sensor_phy_2l_34 {
	uint32_t raw;
	struct {
		uint32_t slvsec_sync_symbol              : 9;
		uint32_t _rsv_9                          : 1;
		uint32_t slvsec_standby_symbol           : 9;
		uint32_t _rsv_19                         : 1;
		uint32_t slvsec_deskew_symbol            : 9;
	} bits;
};

union reg_sensor_phy_2l_38 {
	uint32_t raw;
	struct {
		uint32_t slvsec_prbs9_test_period        : 16;
	} bits;
};

union reg_sensor_phy_2l_3c {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_clr             : 16;
	} bits;
};

union reg_sensor_phy_2l_40 {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_mask            : 16;
	} bits;
};

union reg_sensor_phy_2l_44 {
	uint32_t raw;
	struct {
		uint32_t slvsec_phy_intr_status          : 16;
	} bits;
};

union reg_sensor_phy_2l_48 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d0_test_pat_en           : 1;
		uint32_t slvsec_d0_clr_test_pat_err      : 1;
		uint32_t slvsec_d0_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_2l_4c {
	uint32_t raw;
	struct {
		uint32_t slvsec_d0_test_pat_err_cnt      : 16;
		uint32_t slvsec_d0_test_pat_err          : 1;
		uint32_t slvsec_d0_test_pat_pass         : 1;
		uint32_t slvsec_d0_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d0_start_code_err        : 1;
		uint32_t slvsec_d0_end_code_err          : 1;
		uint32_t slvsec_d0_deskew_code_err       : 1;
		uint32_t slvsec_d0_standby_code_err      : 1;
		uint32_t slvsec_d0_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_2l_50 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d1_test_pat_en           : 1;
		uint32_t slvsec_d1_clr_test_pat_err      : 1;
		uint32_t slvsec_d1_test_stop_when_done   : 1;
	} bits;
};

union reg_sensor_phy_2l_54 {
	uint32_t raw;
	struct {
		uint32_t slvsec_d1_test_pat_err_cnt      : 16;
		uint32_t slvsec_d1_test_pat_err          : 1;
		uint32_t slvsec_d1_test_pat_pass         : 1;
		uint32_t slvsec_d1_test_pat_done         : 1;
		uint32_t _rsv_19                         : 5;
		uint32_t slvsec_d1_start_code_err        : 1;
		uint32_t slvsec_d1_end_code_err          : 1;
		uint32_t slvsec_d1_deskew_code_err       : 1;
		uint32_t slvsec_d1_standby_code_err      : 1;
		uint32_t slvsec_d1_sync_code_err         : 1;
	} bits;
};

union reg_sensor_phy_2l_dbg_90 {
	uint32_t raw;
	struct {
		uint32_t ck_hs_state                     : 1;
		uint32_t ck_ulps_state                   : 1;
		uint32_t ck_stopstate                    : 1;
		uint32_t ck_err_state                    : 1;
		uint32_t deskew_state                    : 2;
	} bits;
};

union reg_sensor_phy_2l_dbg_94 {
	uint32_t raw;
	struct {
		uint32_t d0_datahs_state                 : 3;
		uint32_t _rsv_3                          : 1;
		uint32_t d1_datahs_state                 : 3;
	} bits;
};

union reg_sensor_phy_2l_status_98 {
	uint32_t raw;
	struct {
		uint32_t ck_lp_status_clr                : 8;
		uint32_t d0_lp_status_clr                : 8;
		uint32_t d1_lp_status_clr                : 8;
	} bits;
};

union reg_sensor_phy_2l_status_9c {
	uint32_t raw;
	struct {
		uint32_t ck_lp_status_out                : 8;
		uint32_t d0_lp_status_out                : 8;
		uint32_t d1_lp_status_out                : 8;
	} bits;
};

union reg_sensor_phy_2l_d0_0 {
	uint32_t raw;
	struct {
		uint32_t d0_prbs9_en                     : 1;
		uint32_t d0_prbs9_clr_err                : 1;
		uint32_t d0_prbs9_source                 : 1;
		uint32_t d0_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d0_calib_max                    : 8;
		uint32_t d0_calib_step                   : 8;
		uint32_t d0_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_2l_d0_1 {
	uint32_t raw;
	struct {
		uint32_t d0_calib_en                     : 1;
		uint32_t d0_calib_source                 : 1;
		uint32_t d0_calib_mode                   : 1;
		uint32_t d0_calib_ignore                 : 1;
		uint32_t d0_calib_settle                 : 3;
		uint32_t d0_calib_phase_no_shift         : 1;
		uint32_t d0_calib_set_phase              : 8;
		uint32_t d0_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_2l_d0_2 {
	uint32_t raw;
	struct {
		uint32_t d0_prbs9_rx_err                 : 1;
		uint32_t d0_prbs9_test_done              : 1;
		uint32_t d0_prbs9_test_pass              : 1;
		uint32_t d0_skew_calib_done              : 1;
		uint32_t d0_skew_calib_fail              : 1;
		uint32_t d0_datalp_state                 : 4;
		uint32_t d0_datalp_lpreq2err             : 1;
		uint32_t d0_datalp_dataesc2err           : 1;
		uint32_t d0_datalp_rsttri2err            : 1;
		uint32_t d0_datalp_hstest2err            : 1;
		uint32_t d0_datalp_esculp2err            : 1;
		uint32_t d0_datalp_hs2err                : 1;
		uint32_t d0_data_exist_1st_byte          : 1;
		uint32_t d0_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_2l_d0_3 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_4 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_5 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_6 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_7 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_8 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_9 {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_a {
	uint32_t raw;
	struct {
		uint32_t d0_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_2l_d0_b {
	uint32_t raw;
	struct {
		uint32_t d0_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d0_calib_threshold              : 8;
		uint32_t d0_calib_gp_count               : 9;
	} bits;
};

union reg_sensor_phy_2l_d1_0 {
	uint32_t raw;
	struct {
		uint32_t d1_prbs9_en                     : 1;
		uint32_t d1_prbs9_clr_err                : 1;
		uint32_t d1_prbs9_source                 : 1;
		uint32_t d1_prbs9_stop_when_done         : 1;
		uint32_t _rsv_4                          : 4;
		uint32_t d1_calib_max                    : 8;
		uint32_t d1_calib_step                   : 8;
		uint32_t d1_calib_pattern                : 8;
	} bits;
};

union reg_sensor_phy_2l_d1_1 {
	uint32_t raw;
	struct {
		uint32_t d1_calib_en                     : 1;
		uint32_t d1_calib_source                 : 1;
		uint32_t d1_calib_mode                   : 1;
		uint32_t d1_calib_ignore                 : 1;
		uint32_t d1_calib_settle                 : 3;
		uint32_t d1_calib_phase_no_shift         : 1;
		uint32_t d1_calib_set_phase              : 8;
		uint32_t d1_calib_cycle                  : 16;
	} bits;
};

union reg_sensor_phy_2l_d1_2 {
	uint32_t raw;
	struct {
		uint32_t d1_prbs9_rx_err                 : 1;
		uint32_t d1_prbs9_test_done              : 1;
		uint32_t d1_prbs9_test_pass              : 1;
		uint32_t d1_skew_calib_done              : 1;
		uint32_t d1_skew_calib_fail              : 1;
		uint32_t d1_datalp_state                 : 4;
		uint32_t d1_datalp_lpreq2err             : 1;
		uint32_t d1_datalp_dataesc2err           : 1;
		uint32_t d1_datalp_rsttri2err            : 1;
		uint32_t d1_datalp_hstest2err            : 1;
		uint32_t d1_datalp_esculp2err            : 1;
		uint32_t d1_datalp_hs2err                : 1;
		uint32_t d1_data_exist_1st_byte          : 1;
		uint32_t d1_prbs9_err_cnt                : 16;
	} bits;
};

union reg_sensor_phy_2l_d1_3 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_0          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_4 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_1          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_5 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_2          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_6 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_3          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_7 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_4          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_8 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_5          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_9 {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_6          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_a {
	uint32_t raw;
	struct {
		uint32_t d1_skew_calib_result_7          : 32;
	} bits;
};

union reg_sensor_phy_2l_d1_b {
	uint32_t raw;
	struct {
		uint32_t d1_calib_option                 : 1;
		uint32_t _rsv_1                          : 7;
		uint32_t d1_calib_threshold              : 8;
		uint32_t d1_calib_gp_count               : 9;
	} bits;
};

#ifdef __cplusplus
}
#endif

#endif /* _REG_FIELDS_CSI_WRAP_H_ */

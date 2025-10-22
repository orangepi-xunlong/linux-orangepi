/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*
 * Detail information of registers
 */
#include <linux/phy/phy.h>
#include <drm/drm_print.h>

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN65IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define DISPLL_LINEAR_FREQ
#endif
struct sunxi_dphy_lcd {
	int dphy_index;
	volatile struct dphy_lcd_reg *reg;
	struct combophy_config *phy_config;
};

union dphy_ctl_reg_t {
	u32 dwval;
	struct {
		u32 module_en:1;
		u32 res0:3;
		u32 lane_num:2;
		u32 res1:26;
	} bits;
};

union dphy_tx_ctl_reg_t {
	u32 dwval;
	struct {
		u32 tx_d0_force:1;
		u32 tx_d1_force:1;
		u32 tx_d2_force:1;
		u32 tx_d3_force:1;
		u32 tx_clk_force:1;
		u32 res0:3;
		u32 lptx_endian:1;
		u32 hstx_endian:1;
		u32 lptx_8b9b_en:1;
		u32 hstx_8b9b_en:1;
		u32 force_lp11:1;
		u32 res1:3;
		u32 ulpstx_data0_exit:1;
		u32 ulpstx_data1_exit:1;
		u32 ulpstx_data2_exit:1;
		u32 ulpstx_data3_exit:1;
		u32 ulpstx_clk_exit:1;
		u32 res2:3;
		u32 hstx_data_exit:1;
		u32 hstx_clk_exit:1;
		u32 res3:2;
		u32 hstx_clk_cont:1;
		u32 ulpstx_enter:1;
		u32 res4:2;
	} bits;
};

union dphy_rx_ctl_reg_t {
	u32 dwval;
	struct {
		u32 res0:8;
		u32 lprx_endian:1;
		u32 hsrx_endian:1;
		u32 lprx_8b9b_en:1;
		u32 hsrx_8b9b_en:1;
		u32 hsrx_sync:1;
		u32 res1:3;
		u32 lprx_trnd_mask:4;
		u32 rx_d0_force:1;
		u32 rx_d1_force:1;
		u32 rx_d2_force:1;
		u32 rx_d3_force:1;
		u32 rx_clk_force:1;
		u32 res2:6;
		u32 dbc_en:1;
	} bits;
};

union dphy_tx_time0_reg_t {
	u32 dwval;
	struct {
		u32 lpx_tm_set:8;
		u32 dterm_set:8;
		u32 hs_pre_set:8;
		u32 hs_trail_set:8;
	} bits;
};

union dphy_tx_time1_reg_t {
	u32 dwval;
	struct {
		u32 ck_prep_set:8;
		u32 ck_zero_set:8;
		u32 ck_pre_set:8;
		u32 ck_post_set:8;
	} bits;
};

union dphy_tx_time2_reg_t {
	u32 dwval;
	struct {
		u32 ck_trail_set:8;
		u32 hs_dly_set:16;
		u32 res0:4;
		u32 hs_dly_mode:1;
		u32 res1:3;
	} bits;
};

union dphy_tx_time3_reg_t {
	u32 dwval;
	struct {
		u32 lptx_ulps_exit_set:20;
		u32 res0:12;
	} bits;
};

union dphy_tx_time4_reg_t {
	u32 dwval;
	struct {
		u32 hstx_ana0_set:8;
		u32 hstx_ana1_set:8;
		u32 res0:16;
	} bits;
};

union dphy_rx_time0_reg_t {
	u32 dwval;
	struct {
		u32 lprx_to_en:1;
		u32 freq_cnt_en:1;
		u32 res0:2;
		u32 hsrx_clk_miss_en:1;
		u32 hsrx_sync_err_to_en:1;
		u32 res1:2;
		u32 lprx_to:8;
		u32 hsrx_clk_miss:8;
		u32 hsrx_sync_err_to:8;
	} bits;
};

union dphy_rx_time1_reg_t {
	u32 dwval;
	struct {
		u32 lprx_ulps_wp:20;
		u32 rx_dly:12;
	} bits;
};

union dphy_rx_time2_reg_t {
	u32 dwval;
	struct {
		u32 hsrx_ana0_set:8;
		u32 hsrx_ana1_set:8;
		u32 res0:16;
	} bits;
};

union dphy_rx_time3_reg_t {
	u32 dwval;
	struct {
		u32 freq_cnt:16;
		u32 res0:8;
		u32 lprst_dly:8;
	} bits;
};

union dphy_ana0_reg_t {
	u32 dwval;
	struct {
		u32 reg_lptx_setr:3;
		u32 res0:1;
		u32 reg_lptx_setc:3;
		u32 res1:1;
		u32 reg_preemph3:4;
		u32 reg_preemph2:4;
		u32 reg_preemph1:4;
		u32 reg_preemph0:4;
		u32 res2:8;
	} bits;
};

union dphy_ana1_reg_t {
	u32 dwval;
	struct {
		u32 reg_stxck:1;
		u32 res0:3;
		u32 reg_svdl0:4;
		u32 reg_svdl1:4;
		u32 reg_svdl2:4;
		u32 reg_svdl3:4;
		u32 reg_svdlc:4;
		u32 reg_svtt:4;
		u32 reg_csmps:2;
		u32 res1:1;
		u32 reg_vttmode:1;
	} bits;
};

union dphy_ana2_reg_t {
	u32 dwval;
	struct {
		u32 ana_cpu_en:1;
		u32 enib:1;
		u32 enrvs:1;
		u32 res0:1;
		u32 enck_cpu:1;
		u32 entxc_cpu:1;
		u32 enckq_cpu:1;
		u32 res1:1;
		u32 entx_cpu:4;
		u32 res2:1;
		u32 entermc_cpu:1;
		u32 enrxc_cpu:1;
		u32 res3:1;
		u32 enterm_cpu:4;
		u32 enrx_cpu:4;
		u32 enp2s_cpu:4;
		u32 res4:4;
	} bits;
};

union dphy_ana3_reg_t {
	u32 dwval;
	struct {
		u32 enlptx_cpu:4;
		u32 enlprx_cpu:4;
		u32 enlpcd_cpu:4;
		u32 enlprxc_cpu:1;
		u32 enlptxc_cpu:1;
		u32 enlpcdc_cpu:1;
		u32 res0:1;
		u32 entest:1;
		u32 enckdbg:1;
		u32 enldor:1;
		u32 res1:5;
		u32 enldod:1;
		u32 enldoc:1;
		u32 endiv:1;
		u32 envttc:1;
		u32 envttd:4;
	} bits;
};

union dphy_ana4_reg_t {
	u32 dwval;
	struct {
		u32 reg_soft_rcal:5;
		u32 en_soft_rcal:1;
		u32 on_rescal:1;
		u32 en_rescal:1;
		u32 reg_vlv_set:3;
		u32 res0:1;
		u32 reg_vlptx_set:3;
		u32 res1:1;
		u32 reg_vtt_set:3;
		u32 res2:1;
		u32 reg_vres_set:3;
		u32 reg_vref_source:1;
		u32 reg_ib:3;
		u32 res3:1;
		u32 reg_comtest:2;
		u32 en_comtest:1;
		u32 en_mipi:1;
	} bits;
};

union dphy_int_en0_reg_t {
	u32 dwval;
	struct {
		u32 sot_d0_int:1;
		u32 sot_d1_int:1;
		u32 sot_d2_int:1;
		u32 sot_d3_int:1;
		u32 sot_err_d0_int:1;
		u32 sot_err_d1_int:1;
		u32 sot_err_d2_int:1;
		u32 sot_err_d3_int:1;
		u32 sot_sync_err_d0_int:1;
		u32 sot_sync_err_d1_int:1;
		u32 sot_sync_err_d2_int:1;
		u32 sot_sync_err_d3_int:1;
		u32 rx_alg_err_d0_int:1;
		u32 rx_alg_err_d1_int:1;
		u32 rx_alg_err_d2_int:1;
		u32 rx_alg_err_d3_int:1;
		u32 res0:6;
		u32 cd_lp0_err_clk_int:1;
		u32 cd_lp1_err_clk_int:1;
		u32 cd_lp0_err_d0_int:1;
		u32 cd_lp1_err_d0_int:1;
		u32 cd_lp0_err_d1_int:1;
		u32 cd_lp1_err_d1_int:1;
		u32 cd_lp0_err_d2_int:1;
		u32 cd_lp1_err_d2_int:1;
		u32 cd_lp0_err_d3_int:1;
		u32 cd_lp1_err_d3_int:1;
	} bits;
};

union dphy_int_en1_reg_t {
	u32 dwval;
	struct {
		u32 ulps_d0_int:1;
		u32 ulps_d1_int:1;
		u32 ulps_d2_int:1;
		u32 ulps_d3_int:1;
		u32 ulps_wp_d0_int:1;
		u32 ulps_wp_d1_int:1;
		u32 ulps_wp_d2_int:1;
		u32 ulps_wp_d3_int:1;
		u32 ulps_clk_int:1;
		u32 ulps_wp_clk_int:1;
		u32 res0:2;
		u32 lpdt_d0_int:1;
		u32 rx_trnd_d0_int:1;
		u32 tx_trnd_err_d0_int:1;
		u32 undef1_d0_int:1;
		u32 undef2_d0_int:1;
		u32 undef3_d0_int:1;
		u32 undef4_d0_int:1;
		u32 undef5_d0_int:1;
		u32 rst_d0_int:1;
		u32 rst_d1_int:1;
		u32 rst_d2_int:1;
		u32 rst_d3_int:1;
		u32 esc_cmd_err_d0_int:1;
		u32 esc_cmd_err_d1_int:1;
		u32 esc_cmd_err_d2_int:1;
		u32 esc_cmd_err_d3_int:1;
		u32 false_ctl_d0_int:1;
		u32 false_ctl_d1_int:1;
		u32 false_ctl_d2_int:1;
		u32 false_ctl_d3_int:1;
	} bits;
};

union dphy_int_en2_reg_t {
	u32 dwval;
	struct {
		u32 res0;
	} bits;
};

union dphy_int_pd0_reg_t {
	u32 dwval;
	struct {
		u32 sot_d0_pd:1;
		u32 sot_d1_pd:1;
		u32 sot_d2_pd:1;
		u32 sot_d3_pd:1;
		u32 sot_err_d0_pd:1;
		u32 sot_err_d1_pd:1;
		u32 sot_err_d2_pd:1;
		u32 sot_err_d3_pd:1;
		u32 sot_sync_err_d0_pd:1;
		u32 sot_sync_err_d1_pd:1;
		u32 sot_sync_err_d2_pd:1;
		u32 sot_sync_err_d3_pd:1;
		u32 rx_alg_err_d0_pd:1;
		u32 rx_alg_err_d1_pd:1;
		u32 rx_alg_err_d2_pd:1;
		u32 rx_alg_err_d3_pd:1;
		u32 res0:6;
		u32 cd_lp0_err_clk_pd:1;
		u32 cd_lp1_err_clk_pd:1;
		u32 cd_lp0_err_d1_pd:1;
		u32 cd_lp1_err_d1_pd:1;
		u32 cd_lp0_err_d0_pd:1;
		u32 cd_lp1_err_d0_pd:1;
		u32 cd_lp0_err_d2_pd:1;
		u32 cd_lp1_err_d2_pd:1;
		u32 cd_lp0_err_d3_pd:1;
		u32 cd_lp1_err_d3_pd:1;
	} bits;
};

union dphy_int_pd1_reg_t {
	u32 dwval;
	struct {
		u32 ulps_d0_pd:1;
		u32 ulps_d1_pd:1;
		u32 ulps_d2_pd:1;
		u32 ulps_d3_pd:1;
		u32 ulps_wp_d0_pd:1;
		u32 ulps_wp_d1_pd:1;
		u32 ulps_wp_d2_pd:1;
		u32 ulps_wp_d3_pd:1;
		u32 ulps_clk_pd:1;
		u32 ulps_wp_clk_pd:1;
		u32 res0:2;
		u32 lpdt_d0_pd:1;
		u32 rx_trnd_d0_pd:1;
		u32 tx_trnd_err_d0_pd:1;
		u32 undef1_d0_pd:1;
		u32 undef2_d0_pd:1;
		u32 undef3_d0_pd:1;
		u32 undef4_d0_pd:1;
		u32 undef5_d0_pd:1;
		u32 rst_d0_pd:1;
		u32 rst_d1_pd:1;
		u32 rst_d2_pd:1;
		u32 rst_d3_pd:1;
		u32 esc_cmd_err_d0_pd:1;
		u32 esc_cmd_err_d1_pd:1;
		u32 esc_cmd_err_d2_pd:1;
		u32 esc_cmd_err_d3_pd:1;
		u32 false_ctl_d0_pd:1;
		u32 false_ctl_d1_pd:1;
		u32 false_ctl_d2_pd:1;
		u32 false_ctl_d3_pd:1;
	} bits;
};

union dphy_int_pd2_reg_t {
	u32 dwval;
	struct {
		u32 res0;
	} bits;
};

union dphy_dbg0_reg_t {
	u32 dwval;
	struct {
		u32 lptx_sta_d0:3;
		u32 res0:1;
		u32 lptx_sta_d1:3;
		u32 res1:1;
		u32 lptx_sta_d2:3;
		u32 res2:1;
		u32 lptx_sta_d3:3;
		u32 res3:1;
		u32 lptx_sta_clk:3;
		u32 res4:5;
		u32 rcal_flag:1;
		u32 rcal_cmpo:1;
		u32 lock:1;
		u32 res5:1;
		u32 direction:1;
		u32 res6:3;
	} bits;
};

union dphy_dbg1_reg_t {
	u32 dwval;
	struct {
		u32 lptx_dbg_en:1;
		u32 hstx_dbg_en:1;
		u32 res0:2;
		u32 lptx_set_d0:2;
		u32 lptx_set_d1:2;
		u32 lptx_set_d2:2;
		u32 lptx_set_d3:2;
		u32 lptx_set_ck:2;
		u32 res1:18;
	} bits;
};

union dphy_dbg2_reg_t {
	u32 dwval;
	struct {
		u32 hstx_data;
	} bits;
};

union dphy_dbg3_reg_t {
	u32 dwval;
	struct {
		u32 lprx_sta_d0:4;
		u32 lprx_sta_d1:4;
		u32 lprx_sta_d2:4;
		u32 lprx_sta_d3:4;
		u32 lprx_sta_clk:4;
		u32 res0:12;
	} bits;
};

union dphy_dbg4_reg_t {
	u32 dwval;
	struct {
		u32 lprx_phy_d0:2;
		u32 lprx_phy_d1:2;
		u32 lprx_phy_d2:2;
		u32 lprx_phy_d3:2;
		u32 lprx_phy_clk:2;
		u32 res0:6;
		u32 lpcd_phy_d0:2;
		u32 lpcd_phy_d1:2;
		u32 lpcd_phy_d2:2;
		u32 lpcd_phy_d3:2;
		u32 lpcd_phy_clk:2;
		u32 res1:6;
	} bits;
};

union dphy_dbg5_reg_t {
	u32 dwval;
	struct {
		u32 hsrx_data;
	} bits;
};

union dphy_reservd_reg_t {
	u32 dwval;
	struct {
		u32 res0;
	} bits;
};
union combo_phy_reg0_t {
	__u32 dwval;
	struct {
		__u32 en_cp               :  1 ;    /* default: 0; */
		__u32 en_comboldo         :  1 ;    /* default: 0; */
		__u32 en_lvds             :  1 ;    /* default: 0; */
		__u32 en_mipi             :  1 ;    /* default: 0; */
		__u32 en_test_0p8         :  1 ;    /* default: 0; */
		__u32 en_test_comboldo    :  1 ;    /* default: 0; */
		__u32 res0                :  26;    /* default: 0; */
	} bits;
};

union combo_phy_reg1_t {
	__u32 dwval;
	struct {
		__u32 reg_vref0p8         :  3 ;    /* default: 0; */
		__u32 res0                :  1 ;    /* default: 0; */
		__u32 reg_vref1p6         :  3 ;    /* default: 0; */
		__u32 res1                :  25;    /* default: 0; */
	} bits;
};

union combo_phy_reg2_t {
	__u32 dwval;
	struct {
		__u32 hs_stop_dly         :  8 ;    /* default: 0; */
		__u32 res0                :  24;    /* default: 0; */
	} bits;
};

union dphy_tx_skew_reg0_t {
	__u32 dwavl;
	struct {
		__u32 reg_skewcal_sync     :  8 ;    /* default: 0; */
		__u32 reg_skewcal          :  8 ;    /* default: 0; */
		__u32 skewcal_trail_set    :  8 ;    /* default: 0; */
		__u32 skewcal_zero_set     :  8 ;    /* default: 0; */
	} bits;
};

union dphy_tx_skew_reg1_t {
	__u32 dwval;
	struct {
		__u32 skewcal_init_set      : 16 ;    /* default: 0; */
		__u32 skewcal_pedic_set     :  8 ;    /* default: 0; */
		__u32 skewcal_sync_set      :  8 ;    /* default: 0; */
	} bits;
};

union dphy_tx_skew_reg2_t {
	__u32 dwval;
	struct {
		__u32 skewcal_prepare_lp00   :  8 ;    /* default: 0; */
		__u32 skewcal_trail_inv      :  1 ;    /* default: 0; */
		__u32 en_skewcal_perdic      :  1 ;    /* default: 0; */
		__u32 en_skewcal_init        :  1 ;    /* default: 0; */
		__u32 res0                   : 21 ;    /* default: 0; */
	} bits;
};

union dphy_pll_reg0_t {
	__u32 dwval;
	struct {
		__u32 m1                      :  4 ;    /* default: 0x3; */
		__u32 m0                      :  2 ;    /* default: 0; */
		__u32 tdiv                    :  1 ;    /* default: 0; */
		__u32 ndet                    :  1 ;    /* default: 0x1; */
		__u32 n                       :  8 ;    /* default: 0x32; */
		__u32 p                       :  4 ;    /* default: 0; */
		__u32 pll_en                  :  1 ;    /* default: 0x1; */
		__u32 en_lvs                  :  1 ;    /* default: 0x1; */
		__u32 ldo_en                  :  1 ;    /* default: 0x1; */
		__u32 cp36_en                 :  1 ;    /* default: 0x1; */
		__u32 m3                      :  4 ;
		__u32 m2                      :  2 ;
		__u32 res0                    :  1 ;    /* default: 0; */
		__u32 reg_update	          :  1 ;
	} bits;
};

union dphy_pll_reg1_t {
	__u32 dwval;
	struct {
		__u32 test_en                  :  1 ;    /* default: 0x1; */
		__u32 atest_sel                :  2 ;    /* default: 0; */
		__u32 icp_sel                  :  2 ;    /* default: 0; */
		__u32 lpf_sw                   :  1 ;    /* default: 0; */
		__u32 vsetd                    :  3 ;    /* default: 0x2; */
		__u32 vseta                    :  3 ;    /* default: 0x2; */
		__u32 lockdet_en               :  1 ;    /* default: 0; */
		__u32 lockmdsel                :  1 ;    /* default: 0; */
		__u32 unlock_mdsel             :  2 ;    /* default: 0; */
		__u32 pll_cp                   :  5 ;
		__u32 ls_gating                :  1 ;
		__u32 hs_gating                :  1 ;
		__u32 res0                     :  9 ;    /* default: 0; */
	} bits;
};

union dphy_pll_reg2_t {
	__u32 dwval;
	struct {
		__u32 frac                      : 12 ;    /* default: 0x800; */
		__u32 ss_int                    :  8 ;    /* default: 0x32; */
		__u32 ss_frac                   :  9 ;    /* default: 0; */
		__u32 ss_en                     :  1 ;    /* default: 0; */
		__u32 ff_en                     :  1 ;    /* default: 0; */
		__u32 sdm_en                    :  1 ;    /* default: 0x1; */
	} bits;
};

union dphy_pll_reg3_t {
	__u32 dwval;
	struct {
		__u32 res0                    :  30 ;    /* default: 0; */
		__u32 ssc_clk_mux             :  1 ;     /* default: 0; */
		__u32 ssc_rstn_back_up        :  1 ;     /* default: 1; */
	} bits;
};


union dphy_pll_pat0_t {
	__u32 dwval;
	struct {
		__u32 sdm_bot                 :  17 ;    /* default: 0x3; */
		__u32 frequency		      :  2;
		__u32 sdm_dir		      :  1;
		__u32 sdm_step                :  9 ;    /* default: 0; */
		__u32 sdm_mode                :  2 ;    /* default: 0; */
		__u32 sdm_en                  :  1 ;    /* default: 0x1; */
	} bits;
};

union dphy_pll_pat1_t {
	__u32 dwval;
	struct {
		__u32 frc_in                  :  17 ;    /* default: 0x0; */
		__u32 frc_en                  :  1 ;    /* default: 0; */
		__u32 dith                    :  1 ;    /* default: 0; */
		__u32 res0                    :  1 ;    /* default: 0x0; */
		__u32 sdm_dir	              :  1 ;    /* default: 0x0; */
		__u32 res1                    :  1 ;    /* default: 0; */
		__u32 sdm_cycle               :  10 ;    /* default: 0x0; */
	} bits;
};

union dphy_pll_ssc_t {
	__u32 dwval;
	struct {
		__u32 pll_step                  :  4 ;    /* default: 0x0; */
		__u32 pll_phase_compensate      :  3 ;    /* default: 0; */
		__u32 res0                      :  5 ;    /* default: 0; */
		__u32 pll_ssc                   :  17 ;    /* default: 0xccca; */
		__u32 res1                      :  2 ;    /* default: 0x0; */
		__u32 pll_mode                  :  1 ;    /* default: 0x0; */
	} bits;
};

/* dphy register define */
struct dphy_lcd_reg {
	/* 0x00 - 0x0c */
	union dphy_ctl_reg_t dphy_gctl;
	union dphy_tx_ctl_reg_t dphy_tx_ctl;
	union dphy_rx_ctl_reg_t dphy_rx_ctl;
	union dphy_reservd_reg_t dphy_reg00c;
	/* 0x10 - 0x1c */
	union dphy_tx_time0_reg_t dphy_tx_time0;
	union dphy_tx_time1_reg_t dphy_tx_time1;
	union dphy_tx_time2_reg_t dphy_tx_time2;
	union dphy_tx_time3_reg_t dphy_tx_time3;
	/* 0x20 - 0x2c */
	union dphy_tx_time4_reg_t dphy_tx_time4;
	union dphy_reservd_reg_t dphy_reg024[3];
	/* 0x30 - 0x3c */
	union dphy_rx_time0_reg_t dphy_rx_time0;
	union dphy_rx_time1_reg_t dphy_rx_time1;
	union dphy_rx_time2_reg_t dphy_rx_time2;
	union dphy_reservd_reg_t dphy_reg03c;
	/* 0x40 - 0x4c */
	union dphy_rx_time3_reg_t dphy_rx_time3;
	union dphy_reservd_reg_t dphy_reg044[2];
	union dphy_ana0_reg_t dphy_ana0;
	/* 0x50 - 0x5c */
	union dphy_ana1_reg_t dphy_ana1;
	union dphy_ana2_reg_t dphy_ana2;
	union dphy_ana3_reg_t dphy_ana3;
	union dphy_ana4_reg_t dphy_ana4;
	/* 0x60 - 0x6c */
	union dphy_int_en0_reg_t dphy_int_en0;
	union dphy_int_en1_reg_t dphy_int_en1;
	union dphy_int_en2_reg_t dphy_int_en2;
	union dphy_reservd_reg_t dphy_reg06c;
	/* 0x70 - 0x7c */
	union dphy_int_pd0_reg_t dphy_int_pd0;
	union dphy_int_pd1_reg_t dphy_int_pd1;
	union dphy_int_pd2_reg_t dphy_int_pd2;
	union dphy_reservd_reg_t dphy_reg07c;
	/* 0x80 - 0xdc */
	union dphy_reservd_reg_t dphy_reg080[24];
	/* 0xe0 - 0xec */
	union dphy_dbg0_reg_t dphy_dbg0;
	union dphy_dbg1_reg_t dphy_dbg1;
	union dphy_dbg2_reg_t dphy_dbg2;
	union dphy_dbg3_reg_t dphy_dbg3;
	/* 0xf0 - 0xfc */
	union dphy_dbg4_reg_t dphy_dbg4;
	union dphy_dbg5_reg_t dphy_dbg5;
	union dphy_tx_skew_reg0_t       dphy_tx_skew_reg0;  /* 0xf8 */
	union dphy_tx_skew_reg1_t       dphy_tx_skew_reg1;  /* 0xfc */
	union dphy_tx_skew_reg2_t       dphy_tx_skew_reg2;  /* 0x100 */
	union dphy_pll_reg0_t           dphy_pll_reg0;      /* 0x104 */
	union dphy_pll_reg1_t           dphy_pll_reg1;      /* 0x108 */
	union dphy_pll_reg2_t           dphy_pll_reg2;      /* 0x10c */
	union combo_phy_reg0_t          combo_phy_reg0;     /* 0x110 */
	union combo_phy_reg1_t          combo_phy_reg1;     /* 0x114 */
	union combo_phy_reg2_t          combo_phy_reg2;     /* 0x118 */
	union dphy_reservd_reg_t        dphy_reg11c[2];
	union dphy_pll_reg3_t           dphy_pll_reg3;      /* 0x124 */
	union dphy_reservd_reg_t        dphy_reg128[2];
	union dphy_pll_pat0_t           dphy_pll_pat0;      /* 0x130 */
	union dphy_pll_pat1_t           dphy_pll_pat1;      /* 0x134 */
	union dphy_pll_ssc_t            dphy_pll_ssc;       /* 0x138 */
};

struct __disp_dsi_dphy_timing_t {
	unsigned int lp_clk_div;
	unsigned int hs_prepare;
	unsigned int hs_trail;
	unsigned int clk_prepare;
	unsigned int clk_zero;
	unsigned int clk_pre;
	unsigned int clk_post;
	unsigned int clk_trail;
	unsigned int hs_dly_mode;
	unsigned int hs_dly;
	unsigned int lptx_ulps_exit;
	unsigned int hstx_ana0;
	unsigned int hstx_ana1;
};

struct displl_div {
	unsigned int            n;
	unsigned int            p;
	unsigned int            m0;
	unsigned int            m1;
	unsigned int            m2;
	unsigned int            m3;
};

struct freq_range {
	unsigned long lvl_min;
	unsigned long lvl_max;
};

struct combophy_config {
	struct freq_range               freq_lvl;
	union dphy_tx_time0_reg_t       dphy_tx_time0;
	union dphy_ana0_reg_t           dphy_ana0;
	union dphy_ana4_reg_t           dphy_ana4;
	union combo_phy_reg0_t          combo_phy_reg0;
	union combo_phy_reg1_t          combo_phy_reg1;
};

unsigned long get_displl_vco(struct sunxi_dphy_lcd *dphy, unsigned long prate);
void displl_clk_set(struct sunxi_dphy_lcd *dphy, struct displl_div *div);
void displl_clk_get(struct sunxi_dphy_lcd *dphy, struct displl_div *div);
void displl_clk_enable(struct sunxi_dphy_lcd *dphy);
void displl_clk_disable(struct sunxi_dphy_lcd *dphy);
int sunxi_dsi_combo_phy_set_reg_base(struct sunxi_dphy_lcd *dphy, uintptr_t base);
int sunxi_dsi_combophy_configure_dsi(struct sunxi_dphy_lcd *dphy, enum phy_mode mode, struct phy_configure_opts_mipi_dphy *config);
int sunxi_dsi_combophy_set_dsi_mode(struct sunxi_dphy_lcd *dphy, int mode);
int sunxi_dsi_combophy_set_lvds_mode(struct sunxi_dphy_lcd *dphy, bool enable);
u32 phy_displl_ssc(struct sunxi_dphy_lcd *dphy, u32 precent, u32 dcxo_rate);

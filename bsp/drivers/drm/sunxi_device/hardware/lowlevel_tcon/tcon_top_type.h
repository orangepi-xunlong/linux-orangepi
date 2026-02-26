/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_LCD_TYPE_H__
#define __DE_LCD_TYPE_H__

#include <linux/types.h>

union tcon_tv_setup_reg_t {
	u32 dwval;
	struct {
		u32 res0:1;
		u32 tv0_clk_src:1;
		u32 res1:3;
		u32 tv1_clk_src:1;
		u32 res2:10;
		u32 rgb0_src_sel:2; /* use in sun50iw6 */
		//tv0_out tv1_out is not exist
		u32 tv0_out:1;
		u32 tv1_out:1;
		u32 res3:12;
	} bits;
	struct {
		u32 res0:1;
		u32 tv0_edp_hdmi_sel:1;
		u32 res1:1;
		u32 tv0_hdmiphy_ccu_sel:1;
		u32 res2:1;
		u32 tv1_edp_hdmi_sel:1;
		u32 res3:1;
		u32 tv1_hdmiphy_ccu_sel:1;
		u32 res4:7;
		u32 edp_i2s1_src_sel:1;
		u32 res5:16;
	} sun60i_bits;
};

union tcon_de_perh_reg_t {
	u32 dwval;
	struct {
		u32 de_port0_perh:4;
		u32 de_port1_perh:4;
		u32 res0:24;
	} bits;
};

union tcon_dsi_data_skew_reg_t {
	u32 dwval;
	struct {
		u32 dsi0_skew_num:4;
		u32 dsi0_skew_en:1;
		u32 res0:3;
		u32 dsi1_skew_num:4;
		u32 dsi1_skew_en:1;
		u32 res1:19;
	} bits;
};

union tcon_clk_src_reg_t {
	u32 dwval;
	struct {
		u32 lcd0_clk_src:1;
		u32 lcd1_clk_src:1;
		u32 lcd2_clk_src:1;
		u32 lcd3_clk_src:1;
		u32 combo_phy0_clk_src:1;
		u32 combo_phy1_clk_src:1;
		u32 res0:26;
	} bits;
};

union tcon_dsi_dbg_out_src_reg_t {
	u32 dwval;
	struct {
		u32 dsi_debug_out_src:1;
		u32 res0:3;
		u32 dsi_ccu_div_bypass0:1;
		u32 dsi_ccu_div_bypass1:1;
		u32 res2:26;
	} bits;
};

union tcon_lvds_phy_src_reg_t {
	u32 dwval;
	struct {
		u32 lvds_phy3_in_src:1;
		u32 res0:31;
	} bits;
};

union tcon_lvds_ctrl_src_reg_t {
	u32 dwval;
	struct {
		u32 lvds_ctrl2_src:1;
		u32 res0:31;
	} bits;
};

union tcon_clk_gate_reg_t {
	u32 dwval;
	struct {
		u32 res4:16;
		u32 dsi0_clk_gate:1;
		u32 dsi1_clk_gate:1;
		u32 res3:2;
		u32 tv0_clk_gate:1;
		u32 tv1_clk_gate:1;
		u32 tv2_clk_gate:1;
		u32 res2:5;
		u32 hdmi_src:2;
		u32 res0:1;
		u32 dp_sync_dpss:1;
	} bits;
	struct {
		u32 res2:20;
		u32 tv0_clk_gate:1;
		u32 tv1_clk_gate:1;
		u32 res1:6;
		u32 tv0_hdmi_gate:1;
		u32 res0:2;
		u32 dp_sync_dpss:1;
	} sun60i_bits;
};

union dsc_top_ctrl_reg_t {
	u32 dwval;
	struct {
		u32 dsc_enable:1;
		u32 dsc_slice_sel:1;
		u32 dsc_flush:1;
		u32 clk_dsc_div:5;
		u32 res0:24;
	} bits;
};

union dsc_int_reg_t {
	u32 dwval;
	struct {
		u32 res0:13;
		u32 dwc_dsc_iready_int_flag:1;
		u32 res1:1;
		u32 dwc_dsc_err_int_flag:1;
		u32 res2:13;
		u32 dwc_dsc_irady_int_en:1;
		u32 res3:1;
		u32 dwc_dsc_err_int_en:1;
	} bits;
};

union dsi_src_select_reg_t {
	u32 dwval;
	struct {
		u32 dsi0_src_sel:1;
		u32 res0:3;
		u32 dsi1_src_sel:1;
		u32 lcd0_ch_sel:1;
		u32 res1:26;
	} bits;
};

union tv_data_io_pol0_reg_t {
	u32 dwval;
	struct {
		u32 g_y_ch_data_inv:10;
		u32 res0:6;
		u32 r_cb_ch_data_inv:10;
		u32 res1:6;
	} bits;
};

union tv_data_io_pol1_reg_t {
	u32 dwval;
	struct {
		u32 res0:16;
		u32 b_cr_ch_data_inv:10;
		u32 res1:6;
	} bits;
};

union tv_data_io_tri0_reg_t {
	u32 dwval;
	struct {
		u32 g_y_ch_data_out_tri_en:10;
		u32 res0:6;
		u32 r_cb_ch_data_out_tri_en:10;
		u32 res1:6;
	} bits;
};

union tv_data_io_tri1_reg_t {
	u32 dwval;
	struct {
		u32 res0:16;
		u32 b_cr_ch_data_out_tri_en:10;
		u32 res1:6;
	} bits;
};

union tv_pixel_depth_mode_reg_t {
	u32 dwval;
	struct {
		u32 pixel_depth:1;
		u32 res0:31;
	} bits;
};

struct __tcon_top_dev_t {
	/* 0x00 - 0x0c */
	union tcon_tv_setup_reg_t tcon_tv_setup;
	union dsi_src_select_reg_t dsi_src_select;
	union tcon_dsi_data_skew_reg_t dsi_skew;
	union tcon_clk_src_reg_t tcon_clk_src;
	/* 0x10 - 0x1c */
	union tcon_dsi_dbg_out_src_reg_t dsi_dbg_src;
	union tcon_lvds_phy_src_reg_t lvds_phy_src;
	union tcon_lvds_ctrl_src_reg_t lvds_src;
	union tcon_de_perh_reg_t tcon_de_perh;
	/* 0x20 - 0x2c */
	union tcon_clk_gate_reg_t tcon_clk_gate;
	union dsc_top_ctrl_reg_t dsc_top_ctrl;
	union dsc_int_reg_t dsc_int_reg;
};

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2022 Copyright (c)
 *
 *  File name   :       de_dci_type.h
 *
 *  Description :       display engine 35x basic function declaration
 *
 *  History     :       2021/10/20  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_DCI_TYPE_H_
#define _DE_DCI_TYPE_H_

#include "linux/types.h"

/*offset:0x0000*/
union dci_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:15;
		u32 demo_en:1;
		u32 res1:14;
		u32 chroma_comp_en:1;
	} bits;
};

/*offset:0x0004*/
union dci_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0008*/
union dci_in_color_range_reg {
	u32 dwval;
	struct {
		u32 input_color_space:1;
		u32 res0:31;
	} bits;
};

/*offset:0x000c*/
union dci_skin_protect_reg {
	u32 dwval;
	struct {
		u32 skin_en:1;
		u32 res0:15;
		u32 skin_darken_w:8;
		u32 skin_brighten_w:8;
	} bits;
};

/*offset:0x0010*/
union dci_pdf_radius_reg {
	u32 dwval;
	struct {
		u32 pdf_x_radius:12;
		u32 res0:4;
		u32 pdf_y_radius:12;
		u32 res1:4;
	} bits;
};

/*offset:0x0014*/
union dci_count_bound_reg {
	u32 dwval;
	struct {
		u32 black_bound:10;
		u32 res0:6;
		u32 white_bound:10;
		u32 res1:6;
	} bits;
};

/*offset:0x0018*/
union dci_border_map_mode_reg {
	u32 dwval;
	struct {
		u32 border_mapping_mode:1;
		u32 res0:31;
	} bits;
};

/*offset:0x0020*/
union dci_brighten_level_reg0 {
	u32 dwval;
	struct {
		u32 brighten_level_0:6;
		u32 res0:2;
		u32 brighten_level_1:6;
		u32 res1:2;
		u32 brighten_level_2:6;
		u32 res2:2;
		u32 brighten_level_3:6;
		u32 res3:2;
	} bits;
};

/*offset:0x0024*/
union dci_brighten_level_reg1 {
	u32 dwval;
	struct {
		u32 brighten_level_4:6;
		u32 res0:2;
		u32 brighten_level_5:6;
		u32 res1:2;
		u32 brighten_level_6:6;
		u32 res2:2;
		u32 brighten_level_7:6;
		u32 res3:2;
	} bits;
};

/*offset:0x0028*/
union dci_brighten_level_reg2 {
	u32 dwval;
	struct {
		u32 brighten_level_8:6;
		u32 res0:2;
		u32 brighten_level_9:6;
		u32 res1:2;
		u32 brighten_level_10:6;
		u32 res2:2;
		u32 brighten_level_11:6;
		u32 res3:2;
	} bits;
};

/*offset:0x002c*/
union dci_brighten_level_reg3 {
	u32 dwval;
	struct {
		u32 brighten_level_12:6;
		u32 res0:2;
		u32 brighten_level_13:6;
		u32 res1:2;
		u32 brighten_level_14:6;
		u32 res2:10;
	} bits;
};

/*offset:0x0030*/
union dci_darken_level_reg0 {
	u32 dwval;
	struct {
		u32 darken_level_0:6;
		u32 res0:2;
		u32 darken_level_1:6;
		u32 res1:2;
		u32 darken_level_2:6;
		u32 res2:2;
		u32 darken_level_3:6;
		u32 res3:2;
	} bits;
};

/*offset:0x0034*/
union dci_darken_level_reg1 {
	u32 dwval;
	struct {
		u32 darken_level_4:6;
		u32 res0:2;
		u32 darken_level_5:6;
		u32 res1:2;
		u32 darken_level_6:6;
		u32 res2:2;
		u32 darken_level_7:6;
		u32 res3:2;
	} bits;
};

/*offset:0x0038*/
union dci_darken_level_reg2 {
	u32 dwval;
	struct {
		u32 darken_level_8:6;
		u32 res0:2;
		u32 darken_level_9:6;
		u32 res1:2;
		u32 darken_level_10:6;
		u32 res2:2;
		u32 darken_level_11:6;
		u32 res3:2;
	} bits;
};

/*offset:0x003c*/
union dci_darken_level_reg3 {
	u32 dwval;
	struct {
		u32 darken_level_12:6;
		u32 res0:2;
		u32 darken_level_13:6;
		u32 res1:2;
		u32 darken_level_14:6;
		u32 res2:10;
	} bits;
};

/*offset:0x0040*/
union dci_chroma_comp_br_th_reg0 {
	u32 dwval;
	struct {
		u32 c_comp_br_th0:8;
		u32 c_comp_br_th1:8;
		u32 c_comp_br_th2:8;
		u32 c_comp_br_th3:8;
	} bits;
};

/*offset:0x0044*/
union dci_chroma_comp_br_th_reg1 {
	u32 dwval;
	struct {
		u32 c_comp_br_th4:8;
		u32 c_comp_br_th5:8;
		u32 c_comp_br_th6:8;
		u32 c_comp_br_th7:8;
	} bits;
};

/*offset:0x0048*/
union dci_chroma_comp_br_gain_reg0 {
	u32 dwval;
	struct {
		u32 c_comp_br_gain0:8;
		u32 c_comp_br_gain1:8;
		u32 c_comp_br_gain2:8;
		u32 c_comp_br_gain3:8;
	} bits;
};

/*offset:0x004c*/
union dci_chroma_comp_br_gain_reg1 {
	u32 dwval;
	struct {
		u32 c_comp_br_gain4:8;
		u32 c_comp_br_gain5:8;
		u32 c_comp_br_gain6:8;
		u32 c_comp_br_gain7:8;
	} bits;
};

/*offset:0x0050*/
union dci_chroma_comp_br_slope_reg0 {
	u32 dwval;
	struct {
		u32 c_comp_br_slope0:8;
		u32 c_comp_br_slope1:8;
		u32 c_comp_br_slope2:8;
		u32 c_comp_br_slope3:8;
	} bits;
};

/*offset:0x0054*/
union dci_chroma_comp_br_slope_reg1 {
	u32 dwval;
	struct {
		u32 c_comp_br_slope4:8;
		u32 c_comp_br_slope5:8;
		u32 c_comp_br_slope6:8;
		u32 c_comp_br_slope7:8;
	} bits;
};

/*offset:0x0060*/
union dci_chroma_comp_dark_th_reg0 {
	u32 dwval;
	struct {
		u32 c_comp_dk_th0:8;
		u32 c_comp_dk_th1:8;
		u32 c_comp_dk_th2:8;
		u32 c_comp_dk_th3:8;
	} bits;
};

/*offset:0x0064*/
union dci_chroma_comp_dark_th_reg1 {
	u32 dwval;
	struct {
		u32 c_comp_dk_th4:8;
		u32 c_comp_dk_th5:8;
		u32 c_comp_dk_th6:8;
		u32 c_comp_dk_th7:8;
	} bits;
};

/*offset:0x0068*/
union dci_chroma_comp_dark_gain_reg0 {
	u32 dwval;
	struct {
		u32 c_comp_dk_gain0:8;
		u32 c_comp_dk_gain1:8;
		u32 c_comp_dk_gain2:8;
		u32 c_comp_dk_gain3:8;
	} bits;
};

/*offset:0x006c*/
union dci_chroma_comp_dark_gain_reg1 {
	u32 dwval;
	struct {
		u32 c_comp_dk_gain4:8;
		u32 c_comp_dk_gain5:8;
		u32 c_comp_dk_gain6:8;
		u32 c_comp_dk_gain7:8;
	} bits;
};

/*offset:0x0070*/
union dci_chroma_comp_dark_slope_reg0 {
	u32 dwval;
	struct {
		u32 c_comp_dk_slope0:8;
		u32 c_comp_dk_slope1:8;
		u32 c_comp_dk_slope2:8;
		u32 c_comp_dk_slope3:8;
	} bits;
};

/*offset:0x0074*/
union dci_chroma_comp_dark_slope_reg1 {
	u32 dwval;
	struct {
		u32 c_comp_dk_slope4:8;
		u32 c_comp_dk_slope5:8;
		u32 c_comp_dk_slope6:8;
		u32 c_comp_dk_slope7:8;
	} bits;
};

/*offset:0x0080*/
union dci_demo_horz_reg {
	u32 dwval;
	struct {
		u32 demo_horz_start:13;
		u32 res0:3;
		u32 demo_horz_end:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0084*/
union dci_demo_vert_reg {
	u32 dwval;
	struct {
		u32 demo_vert_start:13;
		u32 res0:3;
		u32 demo_vert_end:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0088*/
union dci_inter_frame_para_reg {
	u32 dwval;
	struct {
		u32 lpf_pdf_en:1;
		u32 res0:31;
	} bits;
};

/*offset:0x0090*/
union dci_ftd_hue_thr_reg {
	u32 dwval;
	struct {
		u32 ftd_hue_low_thr:9;
		u32 res0:7;
		u32 ftd_hue_high_thr:9;
		u32 res1:7;
	} bits;
};

/*offset:0x0094*/
union dci_ftd_chroma_thr_reg {
	u32 dwval;
	struct {
		u32 ftd_chr_low_thr:9;
		u32 res0:7;
		u32 ftd_chr_high_thr:9;
		u32 res1:7;
	} bits;
};

/*offset:0x0098*/
union dci_ftd_slp_reg {
	u32 dwval;
	struct {
		u32 ftd_hue_low_slp:4;
		u32 res0:4;
		u32 ftd_hue_high_slp:4;
		u32 res1:4;
		u32 ftd_chr_low_slp:4;
		u32 res3:4;
		u32 ftd_chr_high_slp:4;
		u32 res4:4;
	} bits;
};

/*offset:0x0100 + N*0x40 + bin*4*/
union dci_cdf_config_reg {
	u32 dwval;
	struct {
		u32 coef:10;
		u32 res0:22;
	} bits;
};

/*offset:0x0500 + N*0x40 + bin*4*/
union dci_pdf_stats_reg {
	u32 dwval;
	struct {
		u32 coef:10;
		u32 res0:22;
	} bits;
};

struct dci_reg {
    /*0x0000*/
    union dci_ctl_reg ctl;
    /*0x0004*/
    union dci_size_reg size;
    /*0x0008*/
    union dci_in_color_range_reg color_range;
    /*0x000c*/
    union dci_skin_protect_reg skin_protect;
    /*0x0010*/
    union dci_pdf_radius_reg pdf_radius;
    /*0x0014*/
    union dci_count_bound_reg count_bound;
    /*0x0018*/
    union dci_border_map_mode_reg border_map_mode;
    u32 res0;
    /*0x0020*/
    union dci_brighten_level_reg0 brighten_level0;
    union dci_brighten_level_reg1 brighten_level1;
    union dci_brighten_level_reg2 brighten_level2;
    union dci_brighten_level_reg3 brighten_level3;
    /*0x0030*/
    union dci_darken_level_reg0 darken_level0;
    union dci_darken_level_reg1 darken_level1;
    union dci_darken_level_reg2 darken_level2;
    union dci_darken_level_reg3 darken_level3;
    /*0x0040*/
    union dci_chroma_comp_br_th_reg0 chroma_comp_br_th0;
    /*0x0044*/
    union dci_chroma_comp_br_th_reg1 chroma_comp_br_th1;
    /*0x0048*/
    union dci_chroma_comp_br_gain_reg0 chroma_comp_br_gain0;
    /*0x004c*/
    union dci_chroma_comp_br_gain_reg1 chroma_comp_br_gain1;
    /*0x0050*/
    union dci_chroma_comp_br_slope_reg0 chroma_comp_br_slope0;
    /*0x0054*/
    union dci_chroma_comp_br_slope_reg1 chroma_comp_br_slope1;
    u32 res1[2];
    /*0x0060*/
    union dci_chroma_comp_dark_th_reg0 chroma_comp_dark_th0;
    /*0x0064*/
    union dci_chroma_comp_dark_th_reg1 chroma_comp_dark_th1;
    /*0x0068*/
    union dci_chroma_comp_dark_gain_reg0 chroma_comp_dark_gain0;
    /*0x006c*/
    union dci_chroma_comp_dark_gain_reg1 chroma_comp_dark_gain1;
    /*0x0070*/
    union dci_chroma_comp_dark_slope_reg0 chroma_comp_dark_slope0;
    /*0x0074*/
    union dci_chroma_comp_dark_slope_reg1 chroma_comp_dark_slope1;
    u32 res2[2];
    /*0x0080*/
    union dci_demo_horz_reg demo_horz;
    /*0x0084*/
    union dci_demo_vert_reg demo_vert;
    /*0x0088*/
    union dci_inter_frame_para_reg inter_frame_para;
    u32 res3;
    /*0x0090*/
    union dci_ftd_hue_thr_reg ftd_hue_thr;
    /*0x0094*/
    union dci_ftd_chroma_thr_reg ftd_chroma_thr;
    /*0x0098*/
    union dci_ftd_slp_reg ftd_slp;
    u32 res4[25];
    /*0x0100*/
    union dci_cdf_config_reg cdf_config[256];
    /*0x0500*/
    union dci_pdf_stats_reg pdf_stats[256];
};

struct dci_status {
	/* Frame number of dci run */
	u32 runtime;
	/* dci enabled */
	u32 isenable;
};

struct dci_config {
    u8 contrast_level;
};

#endif /* #ifndef _DE_DCI_TYPE_H_ */

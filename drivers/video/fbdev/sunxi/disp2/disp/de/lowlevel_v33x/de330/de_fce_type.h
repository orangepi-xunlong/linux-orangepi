/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *  File name   :   de_fce_type.h
 *
 *  Description :   display engine 2.0 fce struct declaration
 *
 *  History     :   2014/04/01  vito cheng  v0.1  Initial version
 *                  2014/04/29      vito cheng  v0.2  Add __fce_config_data
 ******************************************************************************/

#ifndef _DE_FCE_TYPE_H_
#define _DE_FCE_TYPE_H_

#include "linux/types.h"
#include "de_rtmx.h"

/*
 * 0x0: do hist in even frame;
 * 0x1, do hist in odd frame;
 * 0x2, do hist in all frames
 */
#define HIST_FRAME_MASK  0x00000002

/*
 * 0x0: do CE in even frame;
 * 0x1, do CE in odd frame;
 * 0x2, do CE in all frames
 */
#define CE_FRAME_MASK    0x00000002

#define LCE_PARA_NUM  2
#define LCE_MODE_NUM  2

#define AUTOCE_PARA_NUM  5
#define AUTOCE_MODE_NUM  3

#define CE_PARA_NUM  2
#define CE_MODE_NUM  2

#define FTC_PARA_NUM  2
#define FTC_MODE_NUM  2

#define AVG_NUM 8

union fce_gctrl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:15;
		u32 hist_en:1;
		u32 ce_en:1;
		u32 ftc_en:1;
		u32 res1:12;
		u32 win_en:1;
	} bits;
};

union fce_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union fce_win0_reg {
	u32 dwval;
	struct {
		u32 win_left:13;
		u32 res0:3;
		u32 win_top:13;
		u32 res1:3;
	} bits;
};

union fce_win1_reg {
	u32 dwval;
	struct {
		u32 win_right:13;
		u32 res0:3;
		u32 win_bot:13;
		u32 res1:3;
	} bits;
};

union LCE_GAIN_REG {
	u32 dwval;
	struct {
		u32 lce_gain:6;
		u32 res0:2;
		u32 lce_blend:8;
		u32 res1:16;
	} bits;
};

union hist_sum_reg {
	u32 dwval;
	struct {
		u32 sum:32;
	} bits;
};

union HIST_STATUS_REG {
	u32 dwval;
	struct {
		u32 hist_valid:1;
		u32 res0:31;
	} bits;
};

union CE_STATUS_REG {
	u32 dwval;
	struct {
		u32 celut_access_switch:1;
		u32 res0:31;
	} bits;
};

union CE_CC_REG {
	u32 dwval;
	struct {
		u32 chroma_compen_en:1;
		u32 res0:31;
	} bits;
};

union FTC_GAIN_REG {
	u32 dwval;
	struct {
		u32 ftc_gain_y:8;
		u32 res0:8;
		u32 ftc_gain_r:8;
		u32 res1:8;
	} bits;
};

union FTD_HUE_THR_REG {
	u32 dwval;
	struct {
		u32 ftd_hue_low_thr:9;
		u32 res0:7;
		u32 ftd_hue_high_thr:9;
		u32 res1:7;
	} bits;
};

union FTD_CHR_THR_REG {
	u32 dwval;
	struct {
		u32 ftd_chr_low_thr:8;
		u32 res0:8;
		u32 ftd_chr_high_thr:8;
		u32 res1:8;
	} bits;
};

union FTD_SLP_REG {
	u32 dwval;
	struct {
		u32 ftd_hue_low_slp:4;
		u32 res0:4;
		u32 ftd_hue_high_slp:4;
		u32 res1:4;
		u32 ftd_chr_low_slp:4;
		u32 res2:4;
		u32 ftd_chr_high_slp:4;
		u32 res3:4;
	} bits;
};

union fce_csc_en_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:31;
	} bits;
};

union fce_csc_const_reg {
	u32 dwval;
	struct {
		u32 d:10;
		u32 res0:22;
	} bits;
};

union fce_csc_coeff_reg {
	u32 dwval;
	struct {
		u32 c:20;
		u32 res0:12;
	} bits;
};

union CE_LUT_REG {
	u32 dwval;
	struct {
		u32 lut0:10;
		u32 res0:6;
		u32 lut1:10;
		u32 res1:6;
	} bits;
};

union HIST_CNT_REG {
	u32 dwval;
	struct {
		u32 hist:22;
		u32 res0:10;
	} bits;
};

struct fce_reg {
	union fce_gctrl_reg ctrl; /* 0x0000 */
	union fce_size_reg size; /* 0x0004 */
	union fce_win0_reg win0; /* 0x0008 */
	union fce_win1_reg win1; /* 0x000c */
	u32 res0[4]; /* 0x0010-0x001c */
	union hist_sum_reg histsum; /* 0x0020 */
	union HIST_STATUS_REG histstauts; /* 0x0024 */
	union CE_STATUS_REG cestatus; /* 0x0028 */
	union CE_CC_REG cecc; /* 0x002c */
	union FTC_GAIN_REG ftcgain; /* 0x0030 */
	union FTD_HUE_THR_REG ftdhue; /* 0x0034 */
	union FTD_CHR_THR_REG ftdchr; /* 0x0038 */
	union FTD_SLP_REG ftdslp; /* 0x003c */
	union fce_csc_en_reg csc_en;
	union fce_csc_const_reg csc_d0;
	union fce_csc_const_reg csc_d1;
	union fce_csc_const_reg csc_d2;
	union fce_csc_coeff_reg csc_c0[4];
	union fce_csc_coeff_reg csc_c1[4];
	union fce_csc_coeff_reg csc_c2[4];
	u32 res3[32]; /* 0x0080-0x00fc */
	u32 res4[64]; /* 0x0100-0x01fc */
	union CE_LUT_REG celut[128]; /* 0x0200-0x03fc */
	union HIST_CNT_REG hist[256]; /* 0x0400-0x07fc */
};

struct __fce_config_data {

	/* contrast */
	u32 contrast_level; /* contrast level */

	/* brightness */
	u32 bright_level; /* brightness level */

	/* size */
	s32 outw; /* overlay scale width */
	s32 outh; /* overlay scale height */
};

struct fce_hist_status {
	/* Frame number of Histogram run */
	u32 runtime;
	/* Histogram enabled */
	u32 isenable;
	/* Already get histogram of two frames */
	u32 twohistready;
};

struct __ce_status_t {
	u32 isenable;	  /* CE enabled */
	u32 b_automode;  /* 0: Constant CE ; 1: Auto CE */
	u32 width;
	u32 height;
	/* alg */
	u32 up_precent_thr;
	u32 down_precent_thr;
	u32 update_diff_thr;
	u32 slope_black_lmt;
	u32 slope_white_lmt;
	s32 brightness;
	u32 bls_lvl;
	u32 wls_lvl;
};

struct hist_data {
	u32 hist_mean;
	u32 old_hist_mean;
	s32 hist_mean_diff;
	u32 avg_mean_saved[AVG_NUM];
	u32 avg_mean_idx;
	u32 avg_mean;
	u32 counter;
	u32 diff_coeff;

	u32 black_thr0;
	u32 black_thr1;
	u32 white_thr0;
	u32 white_thr1;

	u32 black_slp0;
	u32 black_slp1;
	u32 white_slp0;
	u32 white_slp1;

};

#endif /* #ifndef _DE_FCE_TYPE_H_ */

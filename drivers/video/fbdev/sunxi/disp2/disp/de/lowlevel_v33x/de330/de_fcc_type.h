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
 *  File name   :       de_fcc_type.h
 *
 *  Description :       display engine 2.0 fcc base struct declaration
 *
 *  History     :       2014/03/28  iptang  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_FCC_TYPE_
#define _DE_FCC_TYPE_

#include "de_rtmx.h"

union __fcc_ctrl_reg_t {
	u32 dwval;
	struct {
		u32 en:1;
		u32 r0:3;
		u32 skin_en:1;
		u32 sat_en:1;
		u32 light_en:1;
		u32 r1:1;
		u32 win_en:1;
		u32 r2:23;
	} bits;
};

union __fcc_size_reg_t {
	u32 dwval;
	struct {
		u32 width:13;
		u32 r0:3;
		u32 height:13;
		u32 r1:3;
	} bits;
};

union __fcc_win0_reg_t {
	u32 dwval;
	struct {
		u32 left:13;
		u32 r0:3;
		u32 top:13;
		u32 r1:3;
	} bits;
};

union __fcc_win1_reg_t {
	u32 dwval;
	struct {
		u32 right:13;
		u32 r0:3;
		u32 bot:13;
		u32 r1:3;
	} bits;
};

union __fcc_hue_range_reg_t {
	u32 dwval;
	struct {
		u32 hmin:12;
		u32 r0:4;
		u32 hmax:12;
		u32 r1:4;
	} bits;
};

union __fcc_hs_gain_reg_t {
	u32 dwval;
	struct {
		u32 sgain:9;
		u32 r0:7;
		u32 hgain:9;
		u32 r1:7;
	} bits;
};

union __fcc_sat_gain_reg_t {
	u32 dwval;
	struct {
		u32 sgain:9;
		u32 r0:23;
	} bits;
};

union __fcc_color_gain_reg_t {
	u32 dwval;
	struct {
		u32 sb:4;
		u32 sg:4;
		u32 sr:4;
		u32 r0:20;
	} bits;
};

union __fcc_lut_ctrl_reg_t {
	u32 dwval;
	struct {
		u32 access:1;
		u32 r0:31;
	} bits;
};

union __fcc_light_ctrl_reg_t {
	u32 dwval;
	struct {
		u32 th0:9;
		u32 r0:7;
		u32 th1:9;
		u32 r1:7;
	} bits;
};

union fcc_csc_const_reg {
	u32 dwval;
	struct {
		u32 d:10;
		u32 res0:22;
	} bits;
};

union fcc_csc_coeff_reg {
	u32 dwval;
	struct {
		u32 c:20;
		u32 res0:12;
	} bits;
};

struct fcc_reg {
	union __fcc_ctrl_reg_t ctl;	                /* 0x00      */
	union __fcc_size_reg_t size;	                /* 0x04      */
	union __fcc_win0_reg_t win0;	                /* 0x08      */
	union __fcc_win1_reg_t win1;	                /* 0x0c      */
	union __fcc_hue_range_reg_t hue_range[6];       /* 0x10-0x24 */
	u32 r0[2];	                        /* 0x28-0x2c */
	union __fcc_hs_gain_reg_t hue_gain[6];	        /* 0x30-0x44 */
	union __fcc_sat_gain_reg_t sat_gain;	        /* 0x48      */
	union __fcc_color_gain_reg_t color_gain;	/* 0x4c      */
	union __fcc_lut_ctrl_reg_t lut_ctrl;	        /* 0x50      */
	union __fcc_light_ctrl_reg_t light_ctrl;	/* 0x54      */
	u32 res1[3];
	union fcc_csc_const_reg icsc_d0;
	union fcc_csc_const_reg icsc_d1;
	union fcc_csc_const_reg icsc_d2;
	union fcc_csc_coeff_reg icsc_c0[4];
	union fcc_csc_coeff_reg icsc_c1[4];
	union fcc_csc_coeff_reg icsc_c2[4];
	u32 res2;
	union fcc_csc_const_reg ocsc_d0;
	union fcc_csc_const_reg ocsc_d1;
	union fcc_csc_const_reg ocsc_d2;
	union fcc_csc_coeff_reg ocsc_c0[4];
	union fcc_csc_coeff_reg ocsc_c1[4];
	union fcc_csc_coeff_reg ocsc_c2[4];
};

union __fcc_gain_lut_reg_t {
	u32 dwval;
	struct {
		u32 lut0:10;
		u32 r0:6;
		u32 lut1:10;
		u32 r1:6;
	} bits;
};

struct fcc_lut_reg {
	union __fcc_gain_lut_reg_t lut[256];	        /* 0x100      */
};

struct __fcc_config_data {

	u32 level;
};

#endif /* #ifndef _DE_FCC_TYPE_ */

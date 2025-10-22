/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :       de_fcm_type.h
 *
 *  Description :       display engine 350 basic function declaration
 *
 *  History     :       2021/10/29  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_FCM_TYPE_H_
#define _DE_FCM_TYPE_H_

#include <linux/types.h>
#include "de_csc_table.h"

/*offset:0x0000*/
union fcm_ctrl_reg {
	u32 dwval;
	struct {
		u32 fcm_en:1;
		u32 res0:3;
		u32 window_en:1;
		u32 res1:27;
	} bits;
};

/*offset:0x0008*/
union fcm_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x000c*/
union fcm_win0_reg {
	u32 dwval;
	struct {
		u32 win_left:13;
		u32 res0:3;
		u32 win_top:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0010*/
union fcm_win1_reg {
	u32 dwval;
	struct {
		u32 win_right:13;
		u32 res0:3;
		u32 win_bot:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0020+n*4*/
union fcm_angle_hue_lut_reg {
	u32 dwval;
	struct {
		u32 angle_hue_lut:14;
		u32 res0:18;
	} bits;
};

/*offset:0x00a0+n*4*/
union fcm_angle_sat_lut_reg {
	u32 dwval;
	struct {
		u32 angle_sat_lut:12;
		u32 res0:20;
	} bits;
};

/*offset:0x00e0+n*4*/
union fcm_angle_lum_lut_reg {
	u32 dwval;
	struct {
		u32 angle_lum_lut:10;
		u32 res0:22;
	} bits;
};

/*offset:0x0120+n*4*/
union fcm_hbh_hue_lut_reg {
	u32 dwval;
	struct {
		u32 hbh_hue_lut_low:15;
		u32 res0:1;
		u32 hbh_hue_lut_high:15;
		u32 res1:1;
	} bits;
};

/*offset:0x01a0+n*4*/
union fcm_sbh_hue_lut_reg {
	u32 dwval;
	struct {
		u32 sbh_hue_lut_low:13;
		u32 res0:3;
		u32 sbh_hue_lut_high:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0220+n*4*/
union fcm_vbh_hue_lut_reg {
	u32 dwval;
	struct {
		u32 vbh_hue_lut_low:11;
		u32 res0:5;
		u32 vbh_hue_lut_high:11;
		u32 res1:5;
	} bits;
};

/*offset:0x0300+n*4*/
union fcm_hbh_sat_lut_reg {
	u32 dwval;
	struct {
		u32 hbh_sat_lut0:8;
		u32 hbh_sat_lut1:8;
		u32 hbh_sat_lut2:8;
		u32 hbh_sat_lut3:8;
	} bits;
};

/*offset:0x0900+n*4*/
union fcm_sbh_sat_lut_reg {
	u32 dwval;
	struct {
		u32 sbh_sat_lut0:8;
		u32 sbh_sat_lut1:8;
		u32 sbh_sat_lut2:8;
		u32 sbh_sat_lut3:8;
	} bits;
};

/*offset:0x0f00+n*4*/
union fcm_vbh_sat_lut_reg {
	u32 dwval;
	struct {
		u32 vbh_sat_lut0:8;
		u32 vbh_sat_lut1:8;
		u32 vbh_sat_lut2:8;
		u32 vbh_sat_lut3:8;
	} bits;
};

/*offset:0x1500+n*4*/
union fcm_hbh_lum_lut_reg {
	u32 dwval;
	struct {
		u32 hbh_lum_lut0:8;
		u32 hbh_lum_lut1:8;
		u32 hbh_lum_lut2:8;
		u32 hbh_lum_lut3:8;
	} bits;
};

/*offset:0x1b00+n*4*/
union fcm_sbh_lum_lut_reg {
	u32 dwval;
	struct {
		u32 sbh_lum_lut0:8;
		u32 sbh_lum_lut1:8;
		u32 sbh_lum_lut2:8;
		u32 sbh_lum_lut3:8;
	} bits;
};

/*offset:0x2100+n*4*/
union fcm_vbh_lum_lut_reg {
	u32 dwval;
	struct {
		u32 vbh_lum_lut0:8;
		u32 vbh_lum_lut1:8;
		u32 vbh_lum_lut2:8;
		u32 vbh_lum_lut3:8;
	} bits;
};

/*offset:0x3000*/
union fcm_csc_ctl_reg {
	u32 dwval;
	struct {
		u32 csc_en:1;
		u32 res0:31;
	} bits;
};

/*offset:0x3004*/
union fcm_csc0_d0_reg {
	u32 dwval;
	struct {
		u32 d0:10;
		u32 res0:22;
	} bits;
};

/*offset:0x3008*/
union fcm_csc0_d1_reg {
	u32 dwval;
	struct {
		u32 d1:10;
		u32 res0:22;
	} bits;
};

/*offset:0x300c*/
union fcm_csc0_d2_reg {
	u32 dwval;
	struct {
		u32 d2:10;
		u32 res0:22;
	} bits;
};

/*offset:0x3010*/
union fcm_csc0_c00_reg {
	u32 dwval;
	struct {
		u32 c00:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3014*/
union fcm_csc0_c01_reg {
	u32 dwval;
	struct {
		u32 c01:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3018*/
union fcm_csc0_c02_reg {
	u32 dwval;
	struct {
		u32 c02:20;
		u32 res0:12;
	} bits;
};

/*offset:0x301c*/
union fcm_csc0_c03_reg {
	u32 dwval;
	struct {
		u32 c03:10;
		u32 res0:22;
	} bits;
};

/*offset:0x3020*/
union fcm_csc0_c10_reg {
	u32 dwval;
	struct {
		u32 c10:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3024*/
union fcm_csc0_c11_reg {
	u32 dwval;
	struct {
		u32 c11:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3028*/
union fcm_csc0_c12_reg {
	u32 dwval;
	struct {
		u32 c12:20;
		u32 res0:12;
	} bits;
};

/*offset:0x302c*/
union fcm_csc0_c13_reg {
	u32 dwval;
	struct {
		u32 c13:10;
		u32 res0:22;
	} bits;
};

/*offset:0x3030*/
union fcm_csc0_c20_reg {
	u32 dwval;
	struct {
		u32 c20:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3034*/
union fcm_csc0_c21_reg {
	u32 dwval;
	struct {
		u32 c21:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3038*/
union fcm_csc0_c22_reg {
	u32 dwval;
	struct {
		u32 c22:20;
		u32 res0:12;
	} bits;
};

/*offset:0x303c*/
union fcm_csc0_c23_reg {
	u32 dwval;
	struct {
		u32 c23:10;
		u32 res0:22;
	} bits;
};

/*offset:0x3040*/
union fcm_csc1_d0_reg {
	u32 dwval;
	struct {
		u32 d1:10;
		u32 res0:22;
	} bits;
};

/*offset:0x3044*/
union fcm_csc1_d1_reg {
	u32 dwval;
	struct {
		u32 d1:10;
		u32 res0:22;
	} bits;
};

/*offset:0x3048*/
union fcm_csc1_d2_reg {
	u32 dwval;
	struct {
		u32 d2:10;
		u32 res0:22;
	} bits;
};

/*offset:0x304c*/
union fcm_csc1_c00_reg {
	u32 dwval;
	struct {
		u32 c00:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3050*/
union fcm_csc1_c01_reg {
	u32 dwval;
	struct {
		u32 c01:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3054*/
union fcm_csc1_c02_reg {
	u32 dwval;
	struct {
		u32 c02:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3058*/
union fcm_csc1_c03_reg {
	u32 dwval;
	struct {
		u32 c03:10;
		u32 res0:22;
	} bits;
};

/*offset:0x305c*/
union fcm_csc1_c10_reg {
	u32 dwval;
	struct {
		u32 c10:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3060*/
union fcm_csc1_c11_reg {
	u32 dwval;
	struct {
		u32 c11:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3064*/
union fcm_csc1_c12_reg {
	u32 dwval;
	struct {
		u32 c12:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3068*/
union fcm_csc1_c13_reg {
	u32 dwval;
	struct {
		u32 c13:10;
		u32 res0:22;
	} bits;
};

/*offset:0x306c*/
union fcm_csc1_c20_reg {
	u32 dwval;
	struct {
		u32 c20:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3070*/
union fcm_csc1_c21_reg {
	u32 dwval;
	struct {
		u32 c21:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3074*/
union fcm_csc1_c22_reg {
	u32 dwval;
	struct {
		u32 c22:20;
		u32 res0:12;
	} bits;
};

/*offset:0x3078*/
union fcm_csc1_c23_reg {
	u32 dwval;
	struct {
		u32 c23:10;
		u32 res0:22;
	} bits;
};

struct fcm_reg {
    /*offset:0x0000*/
    union fcm_ctrl_reg ctl;
    u32 res0;
    union fcm_size_reg size;
    union fcm_win0_reg win0;
    /*offset:0x0010*/
    union fcm_win1_reg win1;
    u32 res1[3];
    /*offset:0x0020*/
    union fcm_angle_hue_lut_reg angle_hue_lut[28];
    u32 res2[4];
    /*offset:0x00a0*/
    union fcm_angle_sat_lut_reg angle_sat_lut[13];
    u32 res3[3];
    /*offset:0x00e0*/
    union fcm_angle_lum_lut_reg angle_lum_lut[13];
    u32 res4[3];
    /*offset:0x0120*/
    union fcm_hbh_hue_lut_reg hbh_hue_lut[28];
    u32 res5[4];
    /*offset:0x01a0*/
    union fcm_sbh_hue_lut_reg sbh_hue_lut[28];
    u32 res6[4];
    /*offset:0x0220*/
    union fcm_vbh_hue_lut_reg vbh_hue_lut[28];
    u32 res7[28];
    /*offset:0x0300*/
    union fcm_hbh_sat_lut_reg hbh_sat_lut[364];
    u32 res8[20];
    /*offset:0x0900*/
    union fcm_sbh_sat_lut_reg sbh_sat_lut[364];
    u32 res9[20];
    /*offset:0x0f00*/
    union fcm_vbh_sat_lut_reg vbh_sat_lut[364];
    u32 res10[20];
    /*offset:0x1500*/
    union fcm_hbh_lum_lut_reg hbh_lum_lut[364];
    u32 res11[20];
    /*offset:0x1b00*/
    union fcm_sbh_lum_lut_reg sbh_lum_lut[364];
    u32 res12[20];
    /*offset:0x2100*/
    union fcm_vbh_lum_lut_reg vbh_lum_lut[364];
    u32 res13[596];
    /*offset:0x3000*/
    union fcm_csc_ctl_reg csc_ctl;
    union fcm_csc0_d0_reg csc0_d0;
    union fcm_csc0_d1_reg csc0_d1;
    union fcm_csc0_d2_reg csc0_d2;
    /*offset:0x3010*/
    union fcm_csc0_c00_reg csc0_c00;
    union fcm_csc0_c01_reg csc0_c01;
    union fcm_csc0_c02_reg csc0_c02;
    union fcm_csc0_c03_reg csc0_c03;
    /*offset:0x3020*/
    union fcm_csc0_c10_reg csc0_c10;
    union fcm_csc0_c11_reg csc0_c11;
    union fcm_csc0_c12_reg csc0_c12;
    union fcm_csc0_c13_reg csc0_c13;
    /*offset:0x3030*/
    union fcm_csc0_c20_reg csc0_c20;
    union fcm_csc0_c21_reg csc0_c21;
    union fcm_csc0_c22_reg csc0_c22;
    union fcm_csc0_c23_reg csc0_c23;
    /*offset:0x3040*/
    union fcm_csc1_d0_reg csc1_d0;
    union fcm_csc1_d1_reg csc1_d1;
    union fcm_csc1_d2_reg csc1_d2;
    union fcm_csc1_c00_reg csc1_c00;
    /*offset:0x3050*/
    union fcm_csc1_c01_reg csc1_c01;
    union fcm_csc1_c02_reg csc1_c02;
    union fcm_csc1_c03_reg csc1_c03;
    union fcm_csc1_c10_reg csc1_c10;
    /*offset:0x3060*/
    union fcm_csc1_c11_reg csc1_c11;
    union fcm_csc1_c12_reg csc1_c12;
    union fcm_csc1_c13_reg csc1_c13;
    union fcm_csc1_c20_reg csc1_c20;
    /*offset:0x3070*/
    union fcm_csc1_c21_reg csc1_c21;
    union fcm_csc1_c22_reg csc1_c22;
    union fcm_csc1_c23_reg csc1_c23;
};

struct fcm_config_data {
    u32 level;
};

typedef struct fcm_hardware_data {
	char name[32];
	u32 lut_id;

	s32 hbh_hue[28];
	s32 sbh_hue[28];
	s32 ybh_hue[28];

	s32 angle_hue[28];
	s32 angle_sat[13];
	s32 angle_lum[13];

	s32 hbh_sat[364];
	s32 sbh_sat[364];
	s32 ybh_sat[364];

	s32 hbh_lum[364];
	s32 sbh_lum[364];
	s32 ybh_lum[364];
} fcm_hardware_data_t;

struct fcm_info {
	int cmd;/* 0 write_with update, 1 write without update, 2 read*/
	fcm_hardware_data_t fcm_data;
};

int de_fcm_set_csc(u32 disp, u32 chn, struct de_csc_info *in_info, struct de_csc_info *out_info);

#endif /* #ifndef _DE_FCM_TYPE_H_ */

/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_GAMMA_TYPE_H_
#define _DE_GAMMA_TYPE_H_

#include "linux/types.h"

union gamma_cm_en_reg {
	u32 dwval;
	struct {
		u32 cm_en:1;
		u32 res0:31;
	} bits;
};

union gamma_cm_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union gamma_cm_const_reg {
	u32 dwval;
	struct {
		u32 d:20;
		u32 res0:12;
	} bits;
};

union gamma_cm_coeff_reg {
	u32 dwval;
	struct {
		u32 c:20;
		u32 res0:12;
	} bits;
};

/*color mode:
 * 0,use 0x44 color;
 * 1,vertical gradient;
 * 2,horizontal gradient;
 * 3,colorbar;
 * 4, 16 gray;
 * other,reserved*/
/*offset:0x0040*/
union gamma_ctl_reg {
	u32 dwval;
	struct {
		u32 gamma_en:1;
		u32 color_mode:3;
		u32 blue_en:1;
		u32 res0:27;
	} bits;
};

union gamma_blue_color_reg {
	u32 dwval;
	struct {
		u32 b:10;
		u32 g:10;
		u32 r:10;
		u32 res0:2;
	} bits;
};

union gamma_tab_reg {
	u32 dwval;
	struct {
		u32 tab_b:10;
		u32 tab_g:10;
		u32 tab_r:10;
		u32 res0:2;
	} bits;
};

union ctc_ctl_reg {
	u32 dwval;
	struct {
		u32 ctc_en:1;
		u32 res0:31;
	} bits;
};

union ctc_gain_reg {
	u32 dwval;
	struct {
		u32 gain:10;
		u32 res0:22;
	} bits;
};

union ctc_offset_reg {
	u32 dwval;
	struct {
		u32 offset:9;
		u32 res0:22;
	} bits;
};

union gamma_demo_ctrl_reg {
	u32 dwval;
	struct {
		u32 demo_en:1;
		u32 res0:31;
	} bits;
};

union gamma_demo_win_reg {
	u32 dwval;
	struct {
		u32 start:13;
		u32 res0:3;
		u32 end:13;
		u32 res1:3;
	} bits;
};

union gamma_skin_protect_reg {
	u32 dwval;
	struct {
		u32 skin_en:1;
		u32 res0:15;
		u32 skin_darken_w:8;
		u32 skin_brighten_w:8;
	} bits;
};

struct gamma_reg {
	union gamma_cm_en_reg cm_en;			/* 0x0    */
	union gamma_cm_size_reg cm_size;		/* 0x4    */
	u32 res0[2];
	union gamma_cm_coeff_reg cm_c00;		/* 0x10   */
	union gamma_cm_coeff_reg cm_c01;
	union gamma_cm_coeff_reg cm_c02;
	union gamma_cm_const_reg cm_c03;

	union gamma_cm_coeff_reg cm_c10;		/* 0x20   */
	union gamma_cm_coeff_reg cm_c11;
	union gamma_cm_coeff_reg cm_c12;
	union gamma_cm_coeff_reg cm_c13;

	union gamma_cm_coeff_reg cm_c20;		/* 0x30   */
	union gamma_cm_coeff_reg cm_c21;
	union gamma_cm_coeff_reg cm_c22;
	union gamma_cm_coeff_reg cm_c23;

	union gamma_ctl_reg ctl;			/* 0x40   */
	union gamma_blue_color_reg blue_color;		/* 0x44   */
	u32 res1[2];
	union ctc_ctl_reg crc_ctrl;			/* 0x50   */
	union ctc_gain_reg r_gain;			/* 0x54   */
	union ctc_gain_reg g_gain;			/* 0x58   */
	union ctc_gain_reg b_gain;			/* 0x5c   */
	union ctc_offset_reg r_offset;			/* 0x60   */
	union ctc_offset_reg g_offset;			/* 0x64   */
	union ctc_offset_reg b_offset;			/* 0x68   */
	u32 res2[1];
	union gamma_demo_ctrl_reg demo_ctrl;		/* 0x70   */
	union gamma_demo_win_reg hori_win;		/* 0x74   */
	union gamma_demo_win_reg vert_win;		/* 0x78   */
	union gamma_skin_protect_reg skin_prot;		/* 0x7c   */
	u32 res3[32];
	union gamma_tab_reg tab[1024];			/* 0x100  */
};

#endif /* #ifndef _DE_GAMMA_TYPE_H_ */

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
 *  File name   :       de_gamma_type.h
 *
 *  Description :       display engine 35x vep basic function declaration
 *
 *  History     :       2021/10/29 v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_GAMMA_TYPE_H_
#define _DE_GAMMA_TYPE_H_

#include "linux/types.h"

/*offset:0x0000*/
union gamma_cm_en_reg {
	u32 dwval;
	struct {
		u32 cm_en:1;
		u32 res0:31;
	} bits;
};

/*offset:0x0004*/
union gamma_cm_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0010*/
union gamma_cm_c00_reg {
	u32 dwval;
	struct {
		u32 c00:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0014*/
union gamma_cm_c01_reg {
	u32 dwval;
	struct {
		u32 c01:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0018*/
union gamma_cm_c02_reg {
	u32 dwval;
	struct {
		u32 c02:20;
		u32 res0:12;
	} bits;
};

/*offset:0x001c*/
union gamma_cm_c03_reg {
	u32 dwval;
	struct {
		u32 c03:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0020*/
union gamma_cm_c10_reg {
	u32 dwval;
	struct {
		u32 c10:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0024*/
union gamma_cm_c11_reg {
	u32 dwval;
	struct {
		u32 c11:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0028*/
union gamma_cm_c12_reg {
	u32 dwval;
	struct {
		u32 c12:20;
		u32 res0:12;
	} bits;
};

/*offset:0x002c*/
union gamma_cm_c13_reg {
	u32 dwval;
	struct {
		u32 c13:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0030*/
union gamma_cm_c20_reg {
	u32 dwval;
	struct {
		u32 c20:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0034*/
union gamma_cm_c21_reg {
	u32 dwval;
	struct {
		u32 c21:20;
		u32 res0:12;
	} bits;
};

/*offset:0x0038*/
union gamma_cm_c22_reg {
	u32 dwval;
	struct {
		u32 c22:20;
		u32 res0:12;
	} bits;
};

/*offset:0x003c*/
union gamma_cm_c23_reg {
	u32 dwval;
	struct {
		u32 c23:20;
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

/*offset:0x0044*/
union gamma_blue_color_reg {
	u32 dwval;
	struct {
		u32 b:10;
		u32 g:10;
		u32 r:10;
		u32 res0:2;
	} bits;
};

/*offset:0x0100+n*4*/
union gamma_tab_reg {
	u32 dwval;
	struct {
		u32 tab_b:10;
		u32 tab_g:10;
		u32 tab_r:10;
		u32 res0:2;
	} bits;
};

struct gamma_reg {
    /*offset:0x0000*/
    union gamma_cm_en_reg cm_en;
    union gamma_cm_size_reg cm_size;
    u32 res0[2];
    /*offset:0x0010*/
    union gamma_cm_c00_reg cm_c00;
    union gamma_cm_c01_reg cm_c01;
    union gamma_cm_c02_reg cm_c02;
    union gamma_cm_c03_reg cm_c03;
    /*offset:0x0020*/
    union gamma_cm_c10_reg cm_c10;
    union gamma_cm_c11_reg cm_c11;
    union gamma_cm_c12_reg cm_c12;
    union gamma_cm_c13_reg cm_c13;
    /*offset:0x0030*/
    union gamma_cm_c20_reg cm_c20;
    union gamma_cm_c21_reg cm_c21;
    union gamma_cm_c22_reg cm_c22;
    union gamma_cm_c23_reg cm_c23;
    /*offset:0x0040*/
    union gamma_ctl_reg ctl;
    union gamma_blue_color_reg blue_color;
    u32 res1[46];
    /*offset:0x0100*/
    union gamma_tab_reg tab[1024];
};

struct gamma_data {
	u8 brighten_level;
};

#endif /* #ifndef _DE_GAMMA_TYPE_H_ */

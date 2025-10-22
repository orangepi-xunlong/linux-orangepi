/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DE_CDC_TYPE_H_
#define _DE_CDC_TYPE_H_

#include <linux/types.h>

union cdc_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:3;
		u32 gtm_en:1;
		u32 res1:17;
	} bits;
};

union cdc_dynamic_mode_reg {
	u32 dwval;
	struct {
		u32 dynamic_mode_en:1;
		u32 res0:31;
	} bits;
};

union cdc_gtm_hl_th_reg {
	u32 dwval;
	struct {
		u32 hl_th:10;
		u32 res0:22;
	} bits;
};

union cdc_gtm_hl_val_reg {
	u32 dwval;
	struct {
		u32 hl_val:32;
	} bits;
};

union cdc_gtm_hl_ratio_reg {
	u32 dwval;
	struct {
		u32 hl_ratio:8;
		u32 res0:8;
		u32 th_low_pct:8;
		u32 th_hig_pct:8;
	} bits;
};

union cdc_gtm_src_nit_range_reg {
	u32 dwval;
	struct {
		u32 src_nit_low:14;
		u32 res0:2;
		u32 src_nit_hig:14;
		u32 res1:2;
	} bits;
};

union cdc_gtm_src_max_nit_reg {
	u32 dwval;
	struct {
		u32 src_max_nit:14;
		u32 res0:2;
		u32 max_nit_idx:14;
		u32 res1:2;
	} bits;
};

union cdc_gtm_gamut_fraction_reg {
	u32 dwval;
	struct {
		u32 fraction_bit:4;
		u32 res0:28;
	} bits;
};

union cdc_gtm_gamut_coef_reg {
	u32 dwval;
	struct {
		u32 gamut_coef:16;
		u32 res0:16;
	} bits;
};

union cdc_gtm_curve_reg {
	u32 dwval;
	struct {
		u32 low_curve_val:10;
		u32 res0:6;
		u32 hig_curve_val:10;
		u32 res1:6;
	} bits;
};

union cdc_size_reg {
	u32 dwval;
	struct {
		u32 cdc_width:13;
		u32 res0:3;
		u32 cdc_height:13;
		u32 res1:3;
	} bits;
};

union cdc_csc_const_reg {
	u32 dwval;
	struct {
		u32 val:10;
		u32 res0:22;
	} bits;
};

union cdc_csc_coeff_reg {
	u32 dwval;
	struct {
		u32 val:20;
		u32 res0:12;
	} bits;
};

union cdc_lut_coeff_reg {
	u32 dwval;
	struct {
		u32 coeff:30;
		u32 res0:2;
	} bits;
};

struct cdc_reg {
	union cdc_ctl_reg ctl;
	union cdc_size_reg cdc_size;
	u32 res0[3];
	union cdc_csc_const_reg in_d[3];
	union cdc_csc_coeff_reg in_c0[3];
	union cdc_csc_const_reg in_c03;
	union cdc_csc_coeff_reg in_c1[3];
	union cdc_csc_const_reg in_c13;
	union cdc_csc_coeff_reg in_c2[3];
	union cdc_csc_const_reg in_c23;
	u32 res1;
	union cdc_csc_const_reg out_d[3];
	union cdc_csc_coeff_reg out_c0[3];
	union cdc_csc_const_reg out_c03;
	union cdc_csc_coeff_reg out_c1[3];
	union cdc_csc_const_reg out_c13;
	union cdc_csc_coeff_reg out_c2[3];
	union cdc_csc_const_reg out_c23;
	u32 res20[28];
	union cdc_dynamic_mode_reg dynamic_mode; /* 100 */
	union cdc_gtm_hl_th_reg gtm_hl_th;
	union cdc_gtm_hl_val_reg gtm_hl_val;
	union cdc_gtm_hl_ratio_reg gtm_hl_ratio;
	union cdc_gtm_src_nit_range_reg gtm_src_nit_range;
	union cdc_gtm_src_max_nit_reg gtm_src_max_nit;
	union cdc_gtm_gamut_fraction_reg gtm_gamut_fration;
	union cdc_gtm_gamut_coef_reg gtm_gamut_coef[9];
	u32 res21[944];

	/* lut */
	union cdc_lut_coeff_reg lut0[729]; /* 0x1000 */
	u32 res3[39];
	union cdc_lut_coeff_reg lut1[648]; /* 0x1C00 */
	u32 res4[120];
	union cdc_lut_coeff_reg lut2[648]; /* 0x2800 */
	u32 res5[120];
	union cdc_lut_coeff_reg lut3[576]; /* 0x3400 */
	u32 res6[192];
	union cdc_lut_coeff_reg lut4[648]; /* 0x4000 */
	u32 res7[120];
	union cdc_lut_coeff_reg lut5[576]; /* 0x4C00 */
	u32 res8[192];
	union cdc_lut_coeff_reg lut6[576]; /* 0x5800 */
	u32 res9[192];
	union cdc_lut_coeff_reg lut7[512]; /* 0x6400 */

	u32 res10[256];
	union cdc_gtm_curve_reg curve[1024]; /* 0x7000 */
};

#endif /* #ifndef _DE_CDC_TYPE_H_ */

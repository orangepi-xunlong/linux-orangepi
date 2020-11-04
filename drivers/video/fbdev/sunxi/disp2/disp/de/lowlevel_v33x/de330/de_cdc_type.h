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
		u32 res0:31;
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
	u32 res0[4];
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
	u32 res2[988];

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
};

#endif /* #ifndef _DE_CDC_TYPE_H_ */

/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2016 Copyright (c)
 *
 *  File name   :   de_bls_type.h
 *
 *  Description :   display engine 3.0 bls struct declaration
 *
 *  History     :   2016-3-3 zzz  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_BLS_TYPE_H_
#define _DE_BLS_TYPE_H_

#include "de_rtmx.h"

union bls_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 win_en:1;
		u32 res0:2;
		u32 input_csc_en:1;
		u32 output_csc_en:1;
		u32 res1:2;
		u32 res2:24;
	} bits;
};

union bls_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union bls_pos_reg {
	u32 dwval;
	struct {
		u32 u_offset:8;
		u32 res0:8;
		u32 v_offset:8;
		u32 res1:8;
	} bits;
};

union bls_win0_reg {
	u32 dwval;
	struct {
		u32 win_left:13;
		u32 res0:3;
		u32 win_top:13;
		u32 res1:3;
	} bits;
};

union bls_win1_reg {
	u32 dwval;
	struct {
		u32 win_right:13;
		u32 res0:3;
		u32 win_bot:13;
		u32 res1:3;
	} bits;
};

union bls_attlut_reg {
	u32 dwval;
	struct {
		u32 k0:4;
		u32 k1:4;
		u32 k2:4;
		u32 k3:4;
		u32 k4:4;
		u32 k5:4;
		u32 k6:4;
		u32 k7:4;
	} bits;
};

union BLS_LUTCTRL_REG {
	u32 dwval;
	struct {
		u32 zone_lut_row_sel:5;
		u32 res0:11;
		u32 res1:15;
		u32 zone_lut_access:1;
	} bits;
};

union bls_gainlut_reg {
	u32 dwval;
	struct {
		u32 gain0:8;
		u32 gain1:8;
		u32 gain2:8;
		u32 gain3:8;
	} bits;
};

struct bls_reg {
	union bls_ctl_reg ctrl;           /* 0x00 */
	union bls_size_reg size;           /* 0x04 */
	union bls_win0_reg win0;           /* 0x08 */
	union bls_win1_reg win1;           /* 0x0c */
	union bls_attlut_reg bls_attlut[4];    /* 0x10 */
	union bls_pos_reg bls_pos;             /* 0x20 */
	u32 res0[3];                  /* 0x24-0x2c */
	union bls_gainlut_reg bls_gainlut[4];  /* 0x30 */
};

struct __bls_config_data {
	/* return info */
	u32 mod_en; /* return mod en info */

	/* parameter */
	u32 level; /* user level */
};

#endif /* #ifndef _DE_BLS_TYPE_H_ */

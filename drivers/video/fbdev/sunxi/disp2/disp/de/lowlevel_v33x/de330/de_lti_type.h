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
 *  File name   :   de_lti_type.h
 *
 *  Description :   display engine 2.0 lti struct declaration
 *
 *  History     :   2016-3-3  zzz  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_LTI_TYPE_H_
#define _DE_LTI_TYPE_H_

#include "de_rtmx.h"

union lti_en {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:7;
		u32 sel:1;
		u32 res1:7;
		u32 nonl_en:1;
		u32 res2:7;
		u32 win_en:1;
		u32 res3:7;

	} bits;
};

union lti_size {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union lti_fir_coff0 {
	u32 dwval;
	struct {
		u32 c0:8;
		u32 res0:8;
		u32 c1:8;
		u32 res1:8;
	} bits;
};
union lti_fir_coff1 {
	u32 dwval;
	struct {
		u32 c2:8;
		u32 res0:8;
		u32 c3:8;
		u32 res1:8;
	} bits;
};
union lti_fir_coff2 {
	u32 dwval;
	struct {
		u32 c4:8;
		u32 c5:8;
		u32 c6:8;
		u32 c7:8;
	} bits;
};

union lti_fir_gain {
	u32 dwval;
	struct {
		u32 lti_fil_gain:4;
		u32 res0:28;

	} bits;
};

union lti_cor_th {
	u32 dwval;
	struct {
		u32 lti_cor_th:10;
		u32 res0:22;
	} bits;
};

union lti_diff_ctl {
	u32 dwval;
	struct {
		u32 offset:8;
		u32 res0:8;
		u32 slope:5;
		u32 res1:11;
	} bits;
};

union lti_edge_gain {
	u32 dwval;
	struct {
		u32 edge_gain:5;
		u32 res0:27;
	} bits;
};

union lti_os_con {
	u32 dwval;
	struct {
		u32 core_x:8;
		u32 res0:8;
		u32 clip:8;
		u32 res1:4;
		u32 peak_limit:3;
		u32 res2:1;
	} bits;
};

union lti_win_expansion {
	u32 dwval;
	struct {
		u32 win_range:8;
		u32 cmp_win_sel:2;
		u32 res0:22;
	} bits;
};

union lti_edge_elvel_th {
	u32 dwval;
	struct {
		u32 elvel_th:8;
		u32 res0:24;
	} bits;
};

union lti_win0_reg {
	u32 dwval;
	struct {
		u32 win_left:13;
		u32 res0:3;
		u32 win_top:13;
		u32 res1:3;
	} bits;
};

union lti_win1_reg {
	u32 dwval;
	struct {
		u32 win_right:13;
		u32 res0:3;
		u32 win_bot:13;
		u32 res1:3;
	} bits;
};

union ctl_clmprt_reg {
	u32 dwval;
	struct {
		u32 clm_thr:8;
		u32 clm_lmt:4;
		u32 res0:4;
		u32 res1:16;
	} bits;
};

struct lti_reg {
	union lti_en ctrl;			             /* 0x0000 */
	u32 res0[2];		       /* 0x0004-0x0008 */
	union lti_size size;			           /* 0x000c */
	union lti_fir_coff0 coef0;		       /* 0x0010 */
	union lti_fir_coff1 coef1;		       /* 0x0014 */
	union lti_fir_coff2 coef2;		       /* 0x0018 */
	union lti_fir_gain gain;		         /* 0x001c */
	union lti_cor_th corth;		           /* 0x0020 */
	union lti_diff_ctl diff;		         /* 0x0024 */
	union lti_edge_gain edge_gain;		   /* 0x0028 */
	union lti_os_con os_con;		         /* 0x002c */
	union lti_win_expansion win_range;	 /* 0x0030 */
	union lti_edge_elvel_th elvel_th;		 /* 0x0034 */
	union lti_win0_reg win0;		         /* 0x0038 */
	union lti_win1_reg win1;		         /* 0x003c */
	union ctl_clmprt_reg clm;            /* 0x0040 */
};

struct __lti_config_data {
	/* return info */
	u32 mod_en; /* return mod en info */

	/* parameter */
	u32 level; /* user level */
	u32 inw; /* lti win select */
	u32 inh;
	u32 outw;
	u32 outh;
};

#endif /* #ifndef _DE_LTI_TYPE_H_ */

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
 *  File name   :   de_peak_type.h
 *
 *  Description :   display engine 3.0 peak struct declaration
 *
 *  History     :   2016-3-3 zzz  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_PEAK_TYPE_H_
#define _DE_PEAK_TYPE_H_

#include <linux/types.h>
#include "de_rtmx.h"

union lp_ctrl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:7;
		u32 win_en:1;
		u32 res1:23;
	} bits;
};

union lp_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union lp_win0_reg {
	u32 dwval;
	struct {
		u32 win_left:13;
		u32 res0:3;
		u32 win_top:13;
		u32 res1:3;
	} bits;
};

union lp_win1_reg {
	u32 dwval;
	struct {
		u32 win_right:13;
		u32 res0:3;
		u32 win_bot:13;
		u32 res1:3;
	} bits;
};

union lp_filter_reg {
	u32 dwval;
	struct {
		u32 bp1_ratio:6;
		u32 res0:2;
		u32 bp0_ratio:6;
		u32 res1:2;
		u32 hp_ratio:6;
		u32 res2:9;
		u32 filter_sel:1;
	} bits;
};

union lp_cstm_filter0_reg {
	u32 dwval;
	struct {
		u32 c0:9;
		u32 res0:7;
		u32 c1:9;
		u32 res1:7;
	} bits;
};

union lp_cstm_filter1_reg {
	u32 dwval;
	struct {
		u32 c2:9;
		u32 res0:7;
		u32 c3:9;
		u32 res1:7;
	} bits;
};

union lp_cstm_filter2_reg {
	u32 dwval;
	struct {
		u32 c4:9;
		u32 res0:23;
	} bits;
};

union lp_gain_reg {
	u32 dwval;
	struct {
		u32 gain:8;
		u32 res0:24;
	} bits;
};

union lp_gainctrl_reg {
	u32 dwval;
	struct {
		u32 beta:5;
		u32 res0:11;
		u32 dif_up:8;
		u32 res1:8;
	} bits;
};

union lp_shootctrl_reg {
	u32 dwval;
	struct {
		u32 neg_gain:6;
		u32 res0:26;
	} bits;
};

union lp_coring_reg {
	u32 dwval;
	struct {
		u32 corthr:8;
		u32 res0:24;
	} bits;
};

struct peak_reg {
	union lp_ctrl_reg ctrl;		                /* 0x0000 */
	union lp_size_reg size;		                /* 0x0004 */
	union lp_win0_reg win0;		                /* 0x0008 */
	union lp_win1_reg win1;		                /* 0x000c */
	union lp_filter_reg filter;		            /* 0x0010 */
	union lp_cstm_filter0_reg cfilter0;		    /* 0x0014 */
	union lp_cstm_filter1_reg cfilter1;		    /* 0x0018 */
	union lp_cstm_filter2_reg cfilter2;		    /* 0x001c */
	union lp_gain_reg gain;		                /* 0x0020 */
	union lp_gainctrl_reg gainctrl;		        /* 0x0024 */
	union lp_shootctrl_reg shootctrl;		      /* 0x0028 */
	union lp_coring_reg coring;		            /* 0x002c */
};

struct __peak_config_data {
	u32 mod_en; /* return mod en info */

	/* parameter */
	u32 level; /* user level */
	u32 peak2d_exist;
	u32 inw; /* peak en select */
	u32 inh;
	u32 outw;
	u32 outh;
};

#endif /* #ifndef _DE_PEAK_TYPE_H_ */

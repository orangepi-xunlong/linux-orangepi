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
 *  File name   :       de_deband_type.h
 *
 *  Description :       display engine 35x  basic function declaration
 *
 *  History     :       2021/10/29  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_DEBAND_TYPE_H_
#define _DE_DEBAND_TYPE_H_

#include "linux/types.h"

/*offset:0x0000*/
union deband_ctl_reg {
	u32 dwval;
	struct {
		u32 hdeband_en:1;
		u32 res0:3;
		u32 c_hdeband_en:1;
		u32 res1:3;
		u32 vdeband_en:1;
		u32 res2:3;
		u32 c_vdeband_en:1;
		u32 res3:3;
		u32 demo_en:1;
		u32 res4:15;
	} bits;
};

/*offset:0x0004*/
union deband_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0008*/
union deband_in_color_space_reg {
	u32 dwval;
	struct {
		u32 input_color_space:2;
		u32 res0:30;
	} bits;
};

/*offset:0x000c*/
union deband_output_bits_reg {
	u32 dwval;
	struct {
		u32 output_bits:4;
		u32 res0:28;
	} bits;
};

/*offset:0x0010*/
union deband_step_th_reg {
	u32 dwval;
	struct {
		u32 step_th:6;
		u32 res0:10;
		u32 c_step_th:6;
		u32 res1:10;
	} bits;
};

/*offset:0x0014*/
union deband_edge_th_reg {
	u32 dwval;
	struct {
		u32 edge_th:10;
		u32 res0:6;
		u32 c_edge_th:10;
		u32 res1:6;
	} bits;
};

/*offset:0x0020*/
union deband_vmin_steps_reg {
	u32 dwval;
	struct {
		u32 vmin_steps:4;
		u32 res0:4;
		u32 c_vmin_steps:4;
		u32 res1:4;
		u32 vmin_ratio:4;
		u32 res2:4;
		u32 c_vmin_ratio:4;
		u32 res3:4;
	} bits;
};

/*offset:0x0024*/
union deband_vfilt_paras0_reg {
	u32 dwval;
	struct {
		u32 max_step_diff:4;
		u32 c_max_step_diff:4;
		u32 max_epsilon:4;
		u32 c_max_epsilon:4;
		u32 weight_delta:4;
		u32 c_weight_delta:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0028*/
union deband_vfilt_paras1_reg {
	u32 dwval;
	struct {
		u32 adp_w_up:5;
		u32 res0:3;
		u32 c_adp_w_up:5;
		u32 res1:3;
		u32 vsigma:8;
		u32 c_vsigma:8;
	} bits;
};

/*offset:0x0030*/
union deband_hmin_steps_reg {
	u32 dwval;
	struct {
		u32 hmin_steps:4;
		u32 res0:4;
		u32 c_hmin_steps:4;
		u32 res1:4;
		u32 hmin_ratio:4;
		u32 res2:4;
		u32 c_hmin_ratio:4;
		u32 res3:4;
	} bits;
};

/*offset:0x0034*/
union deband_hfilt_paras0_reg {
	u32 dwval;
	struct {
		u32 kmax:4;
		u32 res0:4;
		u32 c_kmax:4;
		u32 res1:4;
		u32 hsigma:4;
		u32 c_hsigma:4;
	} bits;
};

/*offset:0x0038*/
union deband_hfilt_paras1_reg {
	u32 dwval;
	struct {
		u32 eth:10;
		u32 res0:6;
		u32 c_eth:10;
		u32 res1:6;
	} bits;
};

/*offset:0x0040*/
union deband_err_diffuse_w_reg {
	u32 dwval;
	struct {
		u32 errw_r:4;
		u32 res0:4;
		u32 errw_dr:4;
		u32 res1:4;
		u32 errw_d:4;
		u32 res2:4;
		u32 errw_dl:4;
		u32 res3:4;
	} bits;
};

/*offset:0x0050*/
union deband_rand_dither_reg {
	u32 dwval;
	struct {
		u32 rand_num_bits:3;
		u32 res0:28;
		u32 rand_dither_en:1;
	} bits;
};

/*offset:0x0054*/
union deband_random_generator_reg0 {
	u32 dwval;
	struct {
		u32 rand_generator0:32;
	} bits;
};

/*offset:0x0058*/
union deband_random_generator_reg1 {
	u32 dwval;
	struct {
		u32 rand_generator1:32;
	} bits;
};

/*offset:0x005c*/
union deband_random_generator_reg2 {
	u32 dwval;
	struct {
		u32 rand_generator2:32;
	} bits;
};

/*offset:0x0060*/
union deband_demo_horz_reg {
	u32 dwval;
	struct {
		u32 demo_horz_start:13;
		u32 res0:3;
		u32 demo_horz_end:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0064*/
union deband_demo_vert_reg {
	u32 dwval;
	struct {
		u32 demo_vert_start:13;
		u32 res0:3;
		u32 demo_vert_end:13;
		u32 res1:3;
	} bits;
};

struct deband_reg {
    /*offset:0x0000*/
    union deband_ctl_reg ctl;
    union deband_size_reg size;
    union deband_in_color_space_reg cs;
    union deband_output_bits_reg output_bits;
    /*offset:0x0010*/
    union deband_step_th_reg step;
    union deband_edge_th_reg edge;
    u32 res1[2];
    /*offset:0x0020*/
    union deband_vmin_steps_reg vmin_steps;
    union deband_vfilt_paras0_reg vfilt_para0;
    union deband_vfilt_paras1_reg vfilt_para1;
    u32 res2;
    /*offset:0x0030*/
    union deband_hmin_steps_reg hmin_steps;
    union deband_hfilt_paras0_reg hfilt_para0;
    union deband_hfilt_paras1_reg hfilt_para1;
    u32 res3;
    /*offset:0x0040*/
    union deband_err_diffuse_w_reg err_diffuse_w;
    u32 res4[3];
    /*offset:0x0050*/
    union deband_rand_dither_reg rand_dither;
    union deband_random_generator_reg0 random_gen0;
    union deband_random_generator_reg1 random_gen1;
    union deband_random_generator_reg2 random_gen2;
    /*offset:0x0060*/
    union deband_demo_horz_reg demo_horz;
    union deband_demo_vert_reg demo_vert;
};

#endif /* #ifndef _DE_DEBAND_TYPE_H_ */

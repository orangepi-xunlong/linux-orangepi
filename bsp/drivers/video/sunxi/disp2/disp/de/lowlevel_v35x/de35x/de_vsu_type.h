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

#ifndef _DE_VSU_TYPE_H_
#define _DE_VSU_TYPE_H_

#include <linux/types.h>

union vsu8_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:29;
		u32 reset:1;
		u32 res1:1;
	} bits;
};

union vsu8_scale_mode_reg {
	u32 dwval;
	struct {
		u32 mode:1;
		u32 res0:31;
	} bits;
};

union vsu8_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union vsu8_glb_alpha_reg {
	u32 dwval;
	struct {
		u32 alpha:8;
		u32 res0:24;
	} bits;
};

#define VSU8_STEP_VALID_START_BIT 1
#define VSU8_STEP_FRAC_BITWIDTH 19
#define VSU8_STEP_FIXED_BITWIDTH 4
union vsu8_step_reg {
	u32 dwval;
	struct {
		u32 res0:VSU8_STEP_VALID_START_BIT;
		u32 frac:VSU8_STEP_FRAC_BITWIDTH;
		u32 fixed:VSU8_STEP_FIXED_BITWIDTH;
		u32 res1:8;
	} bits;
};

#define VSU8_PHASE_VALID_START_BIT 1
#define VSU8_PHASE_RRAC_BITWIDTH 19
#define VSU8_PHASE_FIXED_BITWIDTH 4
union vsu8_phase_reg {
	u32 dwval;
	struct {
		u32 res0:VSU8_PHASE_VALID_START_BIT;
		u32 frac:VSU8_PHASE_RRAC_BITWIDTH;
		u32 fixed:VSU8_PHASE_FIXED_BITWIDTH;
		u32 res1:8;
	} bits;
};

union vsu8_filter_coeff_reg {
	u32 dwval;
	struct {
		u32 coeff0:8;
		u32 coeff1:8;
		u32 coeff2:8;
		u32 coeff3:8;
	} bits;
};

struct vsu8_reg {
	union vsu8_ctl_reg ctl;
	u32 res0[3];
	union vsu8_scale_mode_reg scale_mode;
	u32 res1[11];
	union vsu8_size_reg out_size;
	union vsu8_glb_alpha_reg glb_alpha;
	u32 res2[14];
	union vsu8_size_reg y_in_size;
	u32 res3;
	union vsu8_step_reg y_hstep;
	union vsu8_step_reg y_vstep;
	union vsu8_phase_reg y_hphase;
	u32 res4;
	union vsu8_phase_reg y_vphase;
	u32 res5[9];
	union vsu8_size_reg c_in_size;
	u32 res6;
	union vsu8_step_reg c_hstep;
	union vsu8_step_reg c_vstep;
	union vsu8_phase_reg c_hphase;
	u32 res7;
	union vsu8_phase_reg c_vphase; /* 0xD8 */
	u32 res8[73];

	union vsu8_filter_coeff_reg y_hori_coeff[32]; /* 0x200 */
	u32 res9[96];

	union vsu8_filter_coeff_reg y_vert_coeff[32]; /* 0x400 */
	u32 res10[96];

	union vsu8_filter_coeff_reg c_hori_coeff[32]; /* 0x600 */
};

/* ********************************************************** */

union vsu10_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:29;
		u32 reset:1;
		u32 res1:1;
	} bits;
};

union vsu10_status_reg {
	u32 dwval;
	struct {
		u32 res0:4;
		u32 busy:1;
		u32 res1:11;
		u32 line_cnt:12;
		u32 res2:4;
	} bits;
};

union vsu10_field_ctl_reg {
	u32 dwval;
	struct {
		u32 vphase_en:1;
		u32 res0:3;
		u32 filed_reverse:1;
		u32 sync_reverse:1;
		u32 res1:26;
	} bits;
};

union vsu10_scale_mode_reg {
	u32 dwval;
	struct {
		u32 mode:2;
		u32 res0:30;
	} bits;
};

union vsu10_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union vsu10_glb_alpha_reg {
	u32 dwval;
	struct {
		u32 alpha:8;
		u32 res0:24;
	} bits;
};

#define VSU10_STEP_VALID_START_BIT 1
#define VSU10_STEP_FRAC_BITWIDTH 19
#define VSU10_STEP_FIXED_BITWIDTH 4
union vsu10_step_reg {
	u32 dwval;
	struct {
		u32 res0:1;
		u32 frac:19;
		u32 fixed:4;
		u32 res1:8;
	} bits;
};

#define VSU10_PHASE_VALID_START_BIT 1
#define VSU10_PHASE_RRAC_BITWIDTH 19
#define VSU10_PHASE_FIXED_BITWIDTH 4
union vsu10_phase_reg {
	u32 dwval;
	struct {
		u32 res0:VSU10_PHASE_VALID_START_BIT;
		u32 frac:VSU10_PHASE_RRAC_BITWIDTH;
		u32 fixed:VSU10_PHASE_FIXED_BITWIDTH;
		u32 res1:8;
	} bits;
};

union vsu10_filter_coeff_reg {
	u32 dwval;
	struct {
		u32 coeff0:8;
		u32 coeff1:8;
		u32 coeff2:8;
		u32 coeff3:8;
	} bits;
};

struct vsu10_reg {
	union vsu10_ctl_reg ctl;
	u32 res0;
	union vsu10_status_reg status;
	union vsu10_field_ctl_reg field_ctl;
	union vsu10_scale_mode_reg scale_mode;
	u32 res1[11];
	union vsu10_size_reg out_size;
	union vsu10_glb_alpha_reg glb_alpha;
	u32 res2[14];
	union vsu10_size_reg y_in_size;
	u32 res3;
	union vsu10_step_reg y_hstep;
	union vsu10_step_reg y_vstep;
	union vsu10_phase_reg y_hphase;
	u32 res4;
	union vsu10_phase_reg y_vphase0;
	union vsu10_phase_reg y_vphase1;
	u32 res5[8];
	union vsu10_size_reg c_in_size;
	u32 res6;
	union vsu10_step_reg c_hstep;
	union vsu10_step_reg c_vstep;
	union vsu10_phase_reg c_hphase;
	u32 res7;
	union vsu10_phase_reg c_vphase0;
	union vsu10_phase_reg c_vphase1;
	u32 res8[72];
	union vsu10_filter_coeff_reg y_hori_coeff0[32];
	u32 res9[32];
	union vsu10_filter_coeff_reg y_hori_coeff1[32];
	u32 res10[32];
	union vsu10_filter_coeff_reg y_vert_coeff[32];
	u32 res11[96];
	union vsu10_filter_coeff_reg c_hori_coeff0[32];
	u32 res12[32];
	union vsu10_filter_coeff_reg c_hori_coeff1[32];
	u32 res13[32];
	union vsu10_filter_coeff_reg c_vert_coeff[32];
};

/* ********************************************************** */

union vsu_ed_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:29;
		u32 core_rest:1;
		u32 res1:1;
	} bits;
};

union vsu_ed_status_reg {
	u32 dwval;
	struct {
		u32 res0:4;
		u32 busy:1;
		u32 res1:11;
		u32 line_cnt:12;
		u32 res2:4;
	} bits;
};

union vsu_ed_field_ctl_reg {
	u32 dwval;
	struct {
		u32 vphase_sel:1;
		u32 res0:3;
		u32 filed_reverse:1;
		u32 sync_reverse:1;
		u32 res1:26;
	} bits;
};

union vsu_ed_scale_mode_reg {
	u32 dwval;
	struct {
		u32 mode:2;
		u32 res0:30;
	} bits;
};

union vsu_ed_direction_thr_reg {
	u32 dwval;
	struct {
		u32 vert_dir_thr:8;
		u32 horz_dir_thr:8;
		u32 zero_dir_thr:8;
		u32 sub_zero_dir_thr:8;
	} bits;
};

union vsu_ed_edge_thr_reg {
	u32 dwval;
	struct {
		u32 edge_offset:8;
		u32 res0:8;
		u32 edge_shift:4;
		u32 res1:12;
	} bits;
};

union vsu_ed_direction_ctl_reg {
	u32 dwval;
	struct {
		u32 clamp:1;
		u32 res0:31;
	} bits;
};

union vsu_ed_angle_thr_reg {
	u32 dwval;
	struct {
		u32 angle_offset:8;
		u32 res0:8;
		u32 angle_shift:4;
		u32 res1:12;
	} bits;
};

union vsu_ed_sharp_en_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:31;
	} bits;
};

union vsu_ed_sharp_coring_reg {
	u32 dwval;
	struct {
		u32 corth:10;
		u32 res0:22;
	} bits;
};

union vsu_ed_sharp_gain0_reg {
	u32 dwval;
	struct {
		u32 dipthr0:10;
		u32 res0:6;
		u32 dipthr1:10;
		u32 res1:6;
	} bits;
};

union vsu_ed_sharp_gain1_reg {
	u32 dwval;
	struct {
		u32 gain:8;
		u32 res0:8;
		u32 neggain:5;
		u32 res1:3;
		u32 beta:5;
		u32 res2:3;
	} bits;
};

union vsu_ed_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union vsu_ed_glb_alpha_reg {
	u32 dwval;
	struct {
		u32 alpha:8;
		u32 res0:24;
	} bits;
};

#define VSU_ED_STEP_VALID_START_BIT 1
#define VSU_ED_STEP_FRAC_BITWIDTH 19
#define VSU_ED_STEP_FIXED_BITWIDTH 4
union vsu_ed_step_reg {
	u32 dwval;
	struct {
		u32 res0:VSU_ED_STEP_VALID_START_BIT;
		u32 frac:VSU_ED_STEP_FRAC_BITWIDTH;
		u32 fixed:VSU_ED_STEP_FIXED_BITWIDTH;
		u32 res1:8;
	} bits;
};

#define VSU_ED_PHASE_VALID_START_BIT 1
#define VSU_ED_PHASE_RRAC_BITWIDTH 19
#define VSU_ED_PHASE_FIXED_BITWIDTH 4
union vsu_ed_phase_reg {
	u32 dwval;
	struct {
		u32 res0:VSU_ED_PHASE_VALID_START_BIT;
		u32 frac:VSU_ED_PHASE_RRAC_BITWIDTH;
		u32 fixed:VSU_ED_PHASE_FIXED_BITWIDTH;
		u32 res1:8;
	} bits;
};

union vsu_ed_filter_coeff_reg {
	u32 dwval;
	struct {
		u32 coeff0:8;
		u32 coeff1:8;
		u32 coeff2:8;
		u32 coeff3:8;
	} bits;
};

struct vsu_ed_reg {
	union vsu_ed_ctl_reg ctl;
	u32 res0;
	union vsu_ed_status_reg status;
	union vsu_ed_field_ctl_reg field_ctl;
	union vsu_ed_scale_mode_reg scale_mode;
	u32 res1[3];
	union vsu_ed_direction_thr_reg dir_thr;
	union vsu_ed_edge_thr_reg edge_thr;
	union vsu_ed_direction_ctl_reg dir_ctl;
	union vsu_ed_angle_thr_reg angle_thr;
	union vsu_ed_sharp_en_reg sharp_en;
	union vsu_ed_sharp_coring_reg sharp_coring;
	union vsu_ed_sharp_gain0_reg sharp_gain0;
	union vsu_ed_sharp_gain1_reg sharp_gain1;
	union vsu_ed_size_reg out_size;
	union vsu_ed_glb_alpha_reg glb_alpha;
	u32 res2[14];
	union vsu_ed_size_reg y_in_size;
	u32 res3;
	union vsu_ed_step_reg y_hstep;
	union vsu_ed_step_reg y_vstep;
	union vsu_ed_phase_reg y_hphase;
	u32 res4;
	union vsu_ed_phase_reg y_vphase0;
	union vsu_ed_phase_reg y_vphase1;
	u32 res5[8];
	union vsu_ed_size_reg c_in_size;
	u32 res6;
	union vsu_ed_step_reg c_hstep;
	union vsu_ed_step_reg c_vstep;
	union vsu_ed_phase_reg c_hphase;
	u32 res7;
	union vsu_ed_phase_reg c_vphase0;
	union vsu_ed_phase_reg c_vphase1;
	u32 res8[72];
	union vsu_ed_filter_coeff_reg y_hori_coeff0[32];
	u32 res9[32];
	union vsu_ed_filter_coeff_reg y_hori_coeff1[32];
	u32 res10[32];
	union vsu_ed_filter_coeff_reg y_vert_coeff[32];
	u32 res11[96];
	union vsu_ed_filter_coeff_reg c_hori_coeff0[32];
	u32 res12[32];
	union vsu_ed_filter_coeff_reg c_hori_coeff1[32];
	u32 res13[32];
	union vsu_ed_filter_coeff_reg c_vert_coeff[32];
};

/*offset:0x0000*/
union asu_ctl_reg {
	u32 dwval;
	struct {
		u32 y_en:1;
		u32 res0:3;
		u32 c_en:1;
		u32 res1:3;
		u32 win_en:1;
		u32 res2:23;
	} bits;
};

/*offset:0x0004*/
union asu_global_alpha_reg {
	u32 dwval;
	struct {
		u32 alpha:8;
		u32 res0:24;
	} bits;
};

/*offset:0x0008*/
union asu_status_reg {
	u32 dwval;
	struct {
		u32 res0:4;
		u32 busy:1;
		u32 res1:11;
		u32 line_cnt:12;
		u32 res2:4;
	} bits;
};

/*offset:0x000c*/
union asu_field_ctrl_reg {
	u32 dwval;
	struct {
		u32 field_sel_vphase:1;
		u32 res0:3;
		u32 lcd_field_reverse:1;
		u32 res1:27;
	} bits;
};

/*offset:0x0010*/
union asu_scale_mode_reg {
	u32 dwval;
	struct {
		u32 scale_mode:2;
		u32 res0:14;
		u32 lb_mode:1;
		u32 res1:15;
	} bits;
};

/*offset:0x0020*/
union asu_diretion_thr_reg {
	u32 dwval;
	struct {
		u32 th_62_63_switch:8;
		u32 th_sub:8;
		u32 th_flat:8;
		u32 res0:8;
	} bits;
};

/*offset:0x0030*/
union asu_peaking_en_reg {
	u32 dwval;
	struct {
		u32 peaking_en:1;
		u32 res0:15;
		u32 cpeaking_en:1;
		u32 res1:14;
		u32 local_clamp_en:1;
	} bits;
};

/*offset:0x0034*/
union asu_peaking_strength_reg {
	u32 dwval;
	struct {
		u32 peaking_strength:8;
		u32 res0:8;
		u32 cpeaking_strength:8;
		u32 res1:8;
	} bits;
};

/*offset:0x0038*/
union asu_peaking_range_limit_reg {
	u32 dwval;
	struct {
		u32 peaking_range_limit:10;
		u32 res0:6;
		u32 cpeaking_range_limit:10;
		u32 res1:6;
	} bits;
};

/*offset:0x003c*/
union asu_peaking_para_reg {
	u32 dwval;
	struct {
		u32 chroma_white:10;
		u32 res0:6;
		u32 th_strong_edge:4;
		u32 res1:4;
		u32 peak_weights_strength:4;
		u32 res2:4;
	} bits;
};

/*offset:0x0040*/
union asu_out_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0044*/
union asu_crop_horz_reg {
	u32 dwval;
	struct {
		u32 crop_left:13;
		u32 res0:3;
		u32 crop_right:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0048*/
union asu_demo_horz_reg {
	u32 dwval;
	struct {
		u32 win_left:13;
		u32 res0:3;
		u32 win_right:13;
		u32 res1:3;
	} bits;
};

/*offset:0x004c*/
union asu_demo_vert_reg {
	u32 dwval;
	struct {
		u32 win_top:13;
		u32 res0:3;
		u32 win_bot:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0050*/
union asu_y_size_reg {
	u32 dwval;
	struct {
		u32 y_width:13;
		u32 res0:3;
		u32 y_height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0054*/
union asu_y_hstep_reg {
	u32 dwval;
	struct {
		u32 y_hstep_frac:20;
		u32 y_hstep_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0058*/
union asu_y_vstep_reg {
	u32 dwval;
	struct {
		u32 y_vstep_frac:20;
		u32 y_vstep_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0060*/
union asu_y_hphase_reg {
	u32 dwval;
	struct {
		u32 y_hphase_frac:20;
		u32 y_hphase_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0064*/
union asu_y_vphase0_reg {
	u32 dwval;
	struct {
		u32 y_vphase0_frac:20;
		u32 y_vphase0_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0068*/
union asu_y_vphase1_reg {
	u32 dwval;
	struct {
		u32 y_vphase1_frac:20;
		u32 y_vphase1_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0070*/
union asu_c_size_reg {
	u32 dwval;
	struct {
		u32 c_width:13;
		u32 res0:3;
		u32 c_height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0074*/
union asu_c_hstep_reg {
	u32 dwval;
	struct {
		u32 c_hstep_frac:20;
		u32 c_hstep_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0078*/
union asu_c_vstep_reg {
	u32 dwval;
	struct {
		u32 c_vstep_frac:20;
		u32 c_vstep_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0080*/
union asu_c_hphase_reg {
	u32 dwval;
	struct {
		u32 c_hphase_frac:20;
		u32 c_hphase_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0084*/
union asu_c_vphase0_reg {
	u32 dwval;
	struct {
		u32 c_vphase0_frac:20;
		u32 c_vphase0_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0088*/
union asu_c_vphase1_reg {
	u32 dwval;
	struct {
		u32 c_vphase1_frac:20;
		u32 c_vphase1_int:4;
		u32 res0:8;
	} bits;
};

/*offset:0x0100 + N*4*/
union asu_y_hcoef0_reg {
	u32 dwval;
	struct {
		u32 coef0:8;
		u32 coef1:8;
		u32 coef2:8;
		u32 coef3:8;
	} bits;
};

/*offset:0x0200 + N*4*/
union asu_y_hcoef1_reg {
	u32 dwval;
	struct {
		u32 coef4:8;
		u32 coef5:8;
		u32 coef6:8;
		u32 coef7:8;
	} bits;
};

/*offset:0x0300 + N*4*/
union asu_y_vcoef_reg {
	u32 dwval;
	struct {
		u32 coef0:8;
		u32 coef1:8;
		u32 coef2:8;
		u32 coef3:8;
	} bits;
};

/*offset:0x0400 + N*4*/
union asu_c_hcoef0_reg {
	u32 dwval;
	struct {
		u32 coef0:8;
		u32 coef1:8;
		u32 coef2:8;
		u32 coef3:8;
	} bits;
};

/*offset:0x0500 + N*4*/
union asu_c_hcoef1_reg {
	u32 dwval;
	struct {
		u32 coef4:8;
		u32 coef5:8;
		u32 coef6:8;
		u32 coef7:8;
	} bits;
};

/*offset:0x0600 + N*4*/
union asu_c_vcoef_reg {
	u32 dwval;
	struct {
		u32 coef0:8;
		u32 coef1:8;
		u32 coef2:8;
		u32 coef3:8;
	} bits;
};


struct asu_reg {
    /*0x0000*/
    union asu_ctl_reg ctl;
    /*0x0004*/
    union asu_global_alpha_reg g_alpha;
    /*0x0008*/
    union asu_status_reg status;
    /*0x000c*/
    union asu_field_ctrl_reg field_ctl;
    /*0x0010*/
    union asu_scale_mode_reg scale_mode;
    u32 res0[3];
    /*0x0020*/
    union asu_diretion_thr_reg diret_thr;
    u32 res1[3];
    /*0x0030*/
    union asu_peaking_en_reg peaking_en;
    /*0x0034*/
    union asu_peaking_strength_reg peaking_stren;
    /*0x0038*/
    union asu_peaking_range_limit_reg peaking_limit;
    /*0x003c*/
    union asu_peaking_para_reg peaking_para;
    /*0x0040*/
    union asu_out_size_reg out_size;
    /*0x0044*/
    union asu_crop_horz_reg crop_horz;
    /*0x0048*/
    union asu_demo_horz_reg demo_horz;
    /*0x004c*/
    union asu_demo_vert_reg demo_vert;
    /*0x0050*/
    union asu_y_size_reg y_size;
    /*0x0054*/
    union asu_y_hstep_reg y_hstep;
    /*0x0058*/
    union asu_y_vstep_reg y_vstep;
    u32 res2;
    /*0x0060*/
    union asu_y_hphase_reg y_hphase;
    /*0x0064*/
    union asu_y_vphase0_reg y_vphase0;
    /*0x0068*/
    union asu_y_vphase1_reg y_vphase1;
    u32 res3;
    /*0x0070*/
    union asu_c_size_reg c_size;
    /*0x0074*/
    union asu_c_hstep_reg c_hstep;
    /*0x0078*/
    union asu_c_vstep_reg c_vstep;
    u32 res4;
    /*0x0080*/
    union asu_c_hphase_reg c_hphase;
    /*0x0084*/
    union asu_c_vphase0_reg c_vphase0;
    /*0x0088*/
    union asu_c_vphase1_reg c_vphase1;
    u32 res5[29];
    /*0x0100+n*4*/
    union asu_y_hcoef0_reg y_hcoef0[32];
    u32 res6[32];
    /*0x0200+n*4*/
    union asu_y_hcoef1_reg y_hcoef1[32];
    u32 res7[32];
    /*0x0300+n*4*/
    union asu_y_vcoef_reg y_vcoef[32];
    u32 res8[32];
    /*0x0400+n*4*/
    union asu_c_hcoef0_reg c_hcoef0[32];
    u32 res9[32];
    /*0x0500+n*4*/
    union asu_c_hcoef1_reg c_hcoef1[32];
    u32 res10[32];
    /*0x0600+n*4*/
    union asu_c_vcoef_reg c_vcoef[32];
};

#endif /* #ifndef _DE_VSU_TYPE_H_ */

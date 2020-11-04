/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DE_BLD_TYPE_H_
#define _DE_BLD_TYPE_H_

#include <linux/types.h>

union bld_en_ctl_reg {
	u32 dwval;
	struct {
		u32 pipe0_fcolor_en:1;
		u32 pipe1_fcolor_en:1;
		u32 pipe2_fcolor_en:1;
		u32 pipe3_fcolor_en:1;
		u32 pipe4_fcolor_en:1;
		u32 pipe5_fcolor_en:1;
		u32 res0:2;
		u32 pipe0_en:1;
		u32 pipe1_en:1;
		u32 pipe2_en:1;
		u32 pipe3_en:1;
		u32 pipe4_en:1;
		u32 pipe5_en:1;
		u32 res1:18;
	} bits;
};

union bld_fcolor_reg {
	u32 dwval;
	struct {
		u32 blue:8;
		u32 green:8;
		u32 red:8;
		u32 alpha:8;
	} bits;
};

union bld_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union bld_coord_reg {
	u32 dwval;
	struct {
		u32 xcoord:16;
		u32 ycoord:16;
	} bits;
};

union bld_rout_ctl_reg {
	u32 dwval;
	struct {
		u32 pipe0_rout:4;
		u32 pipe1_rout:4;
		u32 pipe2_rout:4;
		u32 pipe3_rout:4;
		u32 pipe4_rout:4;
		u32 pipe5_rout:4;
		u32 res0:8;
	} bits;
};

union bld_premul_ctl_reg {
	u32 dwval;
	struct {
		u32 pipe0_is_premul:1;
		u32 pipe1_is_premul:1;
		u32 pipe2_is_premul:1;
		u32 pipe3_is_premul:1;
		u32 pipe4_is_premul:1;
		u32 pipe5_is_premul:1;
		u32 res0:26;
	} bits;
};

union bld_bg_color_reg {
	u32 dwval;
	struct {
		u32 blue:8;
		u32 green:8;
		u32 red:8;
		u32 res0:8;
	} bits;
};

union bld_blend_ctl_reg {
	u32 dwval;
	struct {
		u32 pfs:4;
		u32 res0:4;
		u32 pfd:4;
		u32 res1:4;
		u32 afs:4;
		u32 res2:4;
		u32 afd:4;
		u32 res3:4;
	} bits;
};

union bld_colorkey_ctl_reg {
	u32 dwval;
	struct {
		u32 key0_en:1;
		u32 key0_match_dir:2;
		u32 res0:1;
		u32 key1_en:1;
		u32 key1_match_dir:2;
		u32 res1:1;
		u32 key2_en:1;
		u32 key2_match_dir:2;
		u32 res2:1;
		u32 key3_en:1;
		u32 key3_match_dir:2;
		u32 res3:1;
		u32 key4_en:1;
		u32 key4_match_dir:2;
		u32 res4:1;
		u32 res5:12;
	} bits;
};

union bld_colorkey_cfg0_reg {
	u32 dwval;
	struct {
		u32 key0_match_b:1;
		u32 key0_match_g:1;
		u32 key0_match_r:1;
		u32 res0:5;
		u32 key1_match_b:1;
		u32 key1_match_g:1;
		u32 key1_match_r:1;
		u32 res1:5;
		u32 key2_match_b:1;
		u32 key2_match_g:1;
		u32 key2_match_r:1;
		u32 res2:5;
		u32 key3_match_b:1;
		u32 key3_match_g:1;
		u32 key3_match_r:1;
		u32 res3:5;
	} bits;
};

union bld_colorkey_cfg1_reg {
	u32 dwval;
	struct {
		u32 key4_match_b:1;
		u32 key4_match_g:1;
		u32 key4_match_r:1;
		u32 res4:29;
	} bits;
};

union bld_colorkey_max_reg {
	u32 dwval;
	struct {
		u32 max_b:8;
		u32 max_g:8;
		u32 max_r:8;
		u32 res0:8;
	} bits;
};

union bld_colorkey_min_reg {
	u32 dwval;
	struct {
		u32 min_b:8;
		u32 min_g:8;
		u32 min_r:8;
		u32 res0:8;
	} bits;
};

union bld_out_color_ctl_reg {
	u32 dwval;
	struct {
		u32 premul_en:1;
		u32 interlace_en:1;
		u32 res0:6;
		u32 fmt_space:2;
		u32 res1:22;
	} bits;
};

struct bld_pipe_attr {
	u32 res0;
	union bld_fcolor_reg fcolor;
	union bld_size_reg in_size;
	union bld_coord_reg in_coord;
};

struct bld_reg {
	union {
		union bld_en_ctl_reg en;
		struct bld_pipe_attr attr[6];
	} pipe;
	u32 res0[8];
	union bld_rout_ctl_reg rout_ctl;
	union bld_premul_ctl_reg premul_ctl;
	union bld_bg_color_reg bg_color;
	union bld_size_reg out_size;
	union bld_blend_ctl_reg blend_ctl[5];
	u32 res1[3];
	union bld_colorkey_ctl_reg colorkey_ctl;
	union bld_colorkey_cfg0_reg colorkey_cfg0;
	union bld_colorkey_cfg1_reg colorkey_cfg1;
	u32 res2;
	union bld_colorkey_max_reg colorkey_max[5];
	u32 res3[3];
	union bld_colorkey_min_reg colorkey_min[5];
	u32 res4[2];
	union bld_out_color_ctl_reg out_ctl;

};

#endif /* #ifndef _DE_BLD_TYPE_H_ */

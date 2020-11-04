/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_FBD_ATW_TYPE_H_
#define _DE_FBD_ATW_TYPE_H_

#include <linux/types.h>

enum fbd_atw_type {
	FBD_ATW_TYPE_INVALID = 0,
	FBD_ATW_TYPE_VI,
	FBD_TYPE_UI,
};

enum fbd_format {
	FBD_RGBA4444    =  0X0e,
	FBD_RGB565      =  0X0a,
	FBD_RGBA5551    =  0X12,
	FBD_RGB888      =  0X08,
	FBD_RGBA8888    =  0X02,
	FBD_RGBA1010102 =  0X16,

	FBD_YUV420      =  0X2a,
	FBD_YUV422      =  0X26,
	FBD_YUV420B10   =  0X30,
	FBD_YUV422B10   =  0X32,
};

struct de_fbd_info {
	enum fbd_format  fmt;
	u32 compbits[4];
	u32 sbs[2];
	u32 yuv_tran;
};

union fbd_v_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 fcolor_en:1;
		u32 alpha_mode:2;
		u32 auto_clk_gate:1;
		u32 res0:19;
		u32 glb_alpha:8;
	} bits;
};

union fbd_v_img_size_reg {
	u32 dwval;
	struct {
		u32 width:12;
		u32 res0:4;
		u32 height:12;
		u32 res1:4;
	} bits;
};

union fbd_v_blk_size_reg {
	u32 dwval;
	struct {
		u32 width:10;
		u32 res0:6;
		u32 height:10;
		u32 res1:6;
	} bits;
};

union fbd_v_src_crop_reg {
	u32 dwval;
	struct {
		u32 left:4;
		u32 res0:12;
		u32 top:4;
		u32 res1:12;
	} bits;
};

union fbd_v_lay_crop_reg {
	u32 dwval;
	struct {
		u32 left:12;
		u32 res0:4;
		u32 top:12;
		u32 res1:4;
	} bits;
};

union fbd_v_fmt_reg {
	u32 dwval;
	struct {
		u32 in_fmt:7;
		u32 yuv_tran:1;
		u32 res0:8;
		u32 sbs_0:2;
		u32 sbs_1:2;
		u32 res1:12;
	} bits;
};

union fbd_v_header_laddr_reg {
	u32 dwval;
	struct {
		u32 addr:32;
	} bits;
};

union fbd_v_header_haddr_reg {
	u32 dwval;
	struct {
		u32 addr:8;
		u32 res0:24;
	} bits;
};

union fbd_atw_v_size_reg {
	u32 dwval;
	struct {
		u32 ovl_width:13;
		u32 res0:3;
		u32 ovl_height:13;
		u32 res1:3;
	} bits;
};

union fbd_atw_v_coor_reg {
	u32 dwval;
	struct {
		u32 ovl_xcoor:13;
		u32 res0:3;
		u32 ovl_ycoor:13;
		u32 res1:3;
	} bits;
};

union fbd_v_bg_color_reg {
	u32 dwval;
	struct {
		u32 color:32;
	} bits;
};

union fbd_v_fcolor_reg {
	u32 dwval;
	struct {
		u32 color:32;
	} bits;
};

union fbd_v_color0_reg {
	u32 dwval;
	struct {
		u32 y_r:16;
		u32 alpha:16;
	} bits;
};

union fbd_v_color1_reg {
	u32 dwval;
	struct {
		u32 v_b:16;
		u32 u_g:16;
	} bits;
};

union fbd_atw_v_rotate_reg {
	u32 dwval;
	struct {
		u32 mode:3;
		u32 res0:29;
	} bits;
};

union atw_v_attr_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 auto_clk_gate:1;
		u32 res0:2;
		u32 eye_buf_mode:4;
		u32 res1:24;
	} bits;
};

union atw_v_data_bit_reg {
	u32 dwval;
	struct {
		u32 bits:4;
		u32 res0:28;
	} bits;
};

union atw_v_out_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union atw_v_blk_step_reg {
	u32 dwval;
	struct {
		u32 step_x:16;
		u32 step_y:16;
	} bits;
};

union atw_v_coeff_laddr_reg {
	u32 dwval;
	struct {
		u32 addr:32;
	} bits;
};

union atw_v_coeff_haddr_reg {
	u32 dwval;
	struct {
		u32 addr:8;
		u32 res0:24;
	} bits;
};

union atw_v_blk_number_reg {
	u32 dwval;
	struct {
		u32 m:8;
		u32 n:8;
		u32 res0:16;
	} bits;
};

struct fbd_atw_v_reg {
	union fbd_v_ctl_reg fbd_ctl;
	u32 res0;
	union fbd_v_img_size_reg fbd_img_size;
	union fbd_v_blk_size_reg fbd_blk_size;
	union fbd_v_src_crop_reg fbd_src_crop;
	union fbd_v_lay_crop_reg fbd_lay_crop;
	union fbd_v_fmt_reg fbd_fmt;
	u32 res1;
	union fbd_v_header_laddr_reg fbd_hd_laddr;
	union fbd_v_header_haddr_reg fbd_hd_haddr;
	u32 res2[2];
	union fbd_atw_v_size_reg fbd_ovl_size;
	union fbd_atw_v_coor_reg fbd_ovl_coor;
	union fbd_v_bg_color_reg fbd_bg_color;
	union fbd_v_fcolor_reg fbd_fcolor;
	u32 res3[4];
	union fbd_v_color0_reg fbd_color0;
	union fbd_v_color1_reg fbd_color1;

	union fbd_atw_v_rotate_reg atw_rotate;
	u32 res4;
	u32 res5[40];

	union atw_v_attr_reg atw_attr;
	u32 res6[3];
	union atw_v_data_bit_reg atw_data_bit;
	u32 res7[3];
	u32 res8[2];
	union atw_v_out_size_reg atw_out_size;
	u32 res9;
	union atw_v_blk_step_reg atw_blk_step;
	union atw_v_coeff_laddr_reg atw_coeff_laddr;
	union atw_v_coeff_haddr_reg atw_coeff_haddr;
	union atw_v_blk_number_reg atw_blk_number;
};

/* ****************************************************** */

union fbd_u_ctl_reg {
	u32 dwval;
	struct {
		u32 fbd_en:1;
		u32 fbd_fcolor_en:1;
		u32 alpha_mode:2;
		u32 auto_clk_gate:1;
		u32 res0:19;
		u32 glb_alpha:8;
	} bits;
};

union fbd_u_img_size_reg {
	u32 dwval;
	struct {
		u32 width:12;
		u32 res0:4;
		u32 height:12;
		u32 res1:4;
	} bits;
};

union fbd_u_blk_size_reg {
	u32 dwval;
	struct {
		u32 width:10;
		u32 res0:6;
		u32 height:10;
		u32 res1:6;
	} bits;
};

union fbd_u_src_crop_reg {
	u32 dwval;
	struct {
		u32 left:4;
		u32 res0:12;
		u32 top:4;
		u32 res1:12;
	} bits;
};

union fbd_u_lay_crop_reg {
	u32 dwval;
	struct {
		u32 left:12;
		u32 res0:4;
		u32 top:12;
		u32 res1:4;
	} bits;
};

union fbd_u_fmt_reg {
	u32 dwval;
	struct {
		u32 fmt:7;
		u32 yuv_tran:1;
		u32 res0:8;
		u32 sbs0:2;
		u32 sbs1:2;
		u32 res1:12;
	} bits;
};

union fbd_u_header_laddr_reg {
	u32 dwval;
	struct {
		u32 addr:32;
	} bits;
};

union fbd_u_header_haddr_reg {
	u32 dwval;
	struct {
		u32 addr:8;
		u32 res0:24;
	} bits;
};

union fbd_u_ovl_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union fbd_u_ovl_coor_reg {
	u32 dwval;
	struct {
		u32 xcoor:13;
		u32 res0:3;
		u32 ycoor:13;
		u32 res1:3;
	} bits;
};

union fbd_u_bg_color_reg {
	u32 dwval;
	struct {
		u32 color:32;
	} bits;
};

union fbd_u_fcolor_reg {
	u32 dwval;
	struct {
		u32 color:32;
	} bits;
};

union fbd_u_color0_reg {
	u32 dwval;
	struct {
		u32 y_r:16;
		u32 alpha:16;
	} bits;
};

union fbd_u_color1_reg {
	u32 dwval;
	struct {
		u32 v_b:16;
		u32 u_g:16;
	} bits;
};

struct fbd_u_reg {
	union fbd_u_ctl_reg fbd_ctl;
	u32 res0;
	union fbd_u_img_size_reg fbd_img_size;
	union fbd_u_blk_size_reg fbd_blk_size;
	union fbd_u_src_crop_reg fbd_src_crop;
	union fbd_u_lay_crop_reg fbd_lay_crop;
	union fbd_u_fmt_reg fbd_fmt;
	u32 res1;
	union fbd_u_header_laddr_reg fbd_hd_laddr;
	union fbd_u_header_haddr_reg fbd_hd_haddr;
	u32 res2[2];
	union fbd_u_ovl_size_reg fbd_ovl_size;
	union fbd_u_ovl_coor_reg fbd_ovl_coor;
	union fbd_u_bg_color_reg fbd_bg_color;
	union fbd_u_fcolor_reg fbd_fcolor;
	u32 res3[4];
	union fbd_u_color0_reg fbd_color0;
	union fbd_u_color0_reg fbd_color1;
};

#endif /* #ifndef _DE_FBD_ATW_TYPE_H_ */

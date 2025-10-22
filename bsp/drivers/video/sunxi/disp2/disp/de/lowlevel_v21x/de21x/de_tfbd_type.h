/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#ifndef _DE_TFBD_TYPE_H_
#define _DE_TFBD_TYPE_H_

#include <linux/types.h>


union ovl_v_tfbd_ctrl {
	u32 dwval;
	struct {
		u32 en:1;
		u32 fc_en:1;
		u32 alpha_mode:2;
		u32 res0:1;
		u32 lossy_rate:2;
		u32 res1:1;
		u32 fmt:7;
		u32 res2:9;
		u32 glb_alpha:8;
	} bits;
};

union ovl_v_tfbd_src_size {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union ovl_v_tfbd_crop_coor {
	u32 dwval;
	struct {
		u32 left:13;
		u32 res0:3;
		u32 top:13;
		u32 res1:3;
	} bits;
};

union ovl_v_tfbd_crop_size {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union ovl_v_tfbd_y_laddr {
	u32 dwval;
	struct {
		u32 laddr:32;
	} bits;
};

union ovl_v_tfbd_uv_laddr {
	u32 dwval;
	struct {
		u32 laddr:32;
	} bits;
};

union ovl_v_tfbd_haddr {
	u32 dwval;
	struct {
		u32 y_haddr:8;
		u32 uv_haddr:8;
		u32 res0:16;
	} bits;
};

union ovl_v_ovl_size {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union ovl_v_ovl_coor {
	u32 dwval;
	struct {
		u32 x:13;
		u32 res0:3;
		u32 y:13;
		u32 res1:3;
	} bits;
};

union ovl_v_tfbd_fc {
	u32 dwval;
	struct {
		u32 fc:32;
	} bits;
};

union ovl_v_tfbd_bgc {
	u32 dwval;
	struct {
		u32 bgc:32;
	} bits;
};

union ovl_v_tfbd_output_fmt_seq {
	u32 dwval;
	struct {
		u32 a:2;
		u32 res0:2;
		u32 b:2;
		u32 res1:2;
		u32 g:2;
		u32 res2:2;
		u32 r:2;
		u32 res3:2;
		u32 uv:1;
		u32 res4:15;
	} bits;
};

struct ovl_v_tfbd_reg {
	union ovl_v_tfbd_ctrl ctrl;
	union ovl_v_tfbd_src_size src_size;
	union ovl_v_tfbd_crop_coor crop_coor;
	union ovl_v_tfbd_crop_size crop_size;
	union ovl_v_tfbd_y_laddr y_laddr;
	union ovl_v_tfbd_uv_laddr uv_laddr;
	union ovl_v_tfbd_haddr haddr;
	union ovl_v_ovl_size ovl_size;
	union ovl_v_ovl_coor ovl_coor;
	union ovl_v_tfbd_fc fc;
	union ovl_v_tfbd_bgc bgc;
	u32 res0;
	union ovl_v_tfbd_output_fmt_seq fmt_seq;
};

#endif /* #ifndef _DE_TFBD_TYPE_H_ */

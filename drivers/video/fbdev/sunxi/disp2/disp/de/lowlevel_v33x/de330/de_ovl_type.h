/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_OVL_TYPE_H_
#define _DE_OVL_TYPE_H_

#include <linux/types.h>

enum ovl_type {
	OVL_TYPE_INVALID = 0,
	OVL_TYPE_VI,
	OVL_TYPE_UI,
};

union ovl_v_attctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 alpha_mode:2;
		u32 res0:1;
		u32 fcolor_en:1;
		u32 res1:3;
		u32 fmt:5;
		u32 res2:2;
		u32 vi_ui_sel:1;
		u32 premul_ctl:2;
		u32 res3:2;
		u32 brust_len:2;
		u32 res4:1;
		u32 top_bot_addr_en:1;
		u32 glb_alpha:8;
	} bits;
};

union ovl_v_mbsize_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union ovl_v_coor_reg {
	u32 dwval;
	struct {
		u32 xcoor:16;
		u32 ycoor:16;
	} bits;
};

union ovl_v_pitch_reg {
	u32 dwval;
	struct {
		u32 pitch:32;
	} bits;
};

union ovl_v_laddr_reg {
	u32 dwval;
	struct {
		u32 laddr:32;
	} bits;
};

union ovl_v_haddr_reg {
	u32 dwval;
	struct {
		u32 haddr_lay0:8;
		u32 haddr_lay1:8;
		u32 haddr_lay2:8;
		u32 haddr_lay3:8;
	} bits;
};

union ovl_v_fcolor_reg {
	u32 dwval;
	struct {
		u32 v_b:8;
		u32 u_g:8;
		u32 y_r:8;
		u32 alpha:8;
	} bits;
};

union ovl_v_win_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/* dsctl: down sampling cotrol */
union ovl_v_dsctl_reg {
	u32 dwval;
	struct {
		u32 m:14;
		u32 res0:2;
		u32 n:14;
		u32 res1:2;
	} bits;
};

struct ovl_v_lay_reg {
	union ovl_v_attctl_reg ctl;
	union ovl_v_mbsize_reg mbsize;
	union ovl_v_coor_reg mbcoor;
	union ovl_v_pitch_reg pitch0;
	union ovl_v_pitch_reg pitch1;
	union ovl_v_pitch_reg pitch2;
	union ovl_v_laddr_reg top_laddr0;
	union ovl_v_laddr_reg top_laddr1;
	union ovl_v_laddr_reg top_laddr2;
	union ovl_v_laddr_reg bot_laddr0;
	union ovl_v_laddr_reg bot_laddr1;
	union ovl_v_laddr_reg bot_laddr2;
};

struct ovl_v_reg {
	struct ovl_v_lay_reg lay[4];
	union ovl_v_fcolor_reg fcolor[4];
	union ovl_v_haddr_reg top_haddr0;
	union ovl_v_haddr_reg top_haddr1;
	union ovl_v_haddr_reg top_haddr2;
	union ovl_v_haddr_reg bot_haddr0;
	union ovl_v_haddr_reg bot_haddr1;
	union ovl_v_haddr_reg bot_haddr2;
	union ovl_v_win_size_reg win_size;
	u32 res0;
	union ovl_v_dsctl_reg hori_ds[2];
	union ovl_v_dsctl_reg vert_ds[2];
};

/* ***************************************************************** */

union ovl_u_attctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 alpha_mode:2;
		u32 res0:1;
		u32 fcolor_en:1;
		u32 res1:3;
		u32 fmt:5;
		u32 res2:3;
		u32 premul_ctl:2;
		u32 res3:2;
		u32 brust_len:2;
		u32 res4:1;
		u32 top_bot_addr_en:1;
		u32 glb_alpha:8;
	} bits;
};

union ovl_u_mbsize_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union ovl_u_coor_reg {
	u32 dwval;
	struct {
		u32 xcoor:16;
		u32 ycoor:16;
	} bits;
};

union ovl_u_pitch_reg {
	u32 dwval;
	struct {
		u32 pitch:32;
	} bits;
};

union ovl_u_laddr_reg {
	u32 dwval;
	struct {
		u32 laddr:32;
	} bits;
};

union ovl_u_fcolor_reg {
	u32 dwval;
	struct {
		u32 b:8;
		u32 g:8;
		u32 r:8;
		u32 alpha:8;
	} bits;
};

union ovl_u_haddr_reg {
	u32 dwval;
	struct {
		u32 haddr_lay0:8;
		u32 haddr_lay1:8;
		u32 haddr_lay2:8;
		u32 haddr_lay3:8;
	} bits;
};

union ovl_u_win_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/* dsctl: down sampling cotrol */
union ovl_u_dsctl_reg {
	u32 dwval;
	struct {
		u32 m:14;
		u32 res0:2;
		u32 n:14;
		u32 res1:2;
	} bits;
};

struct ovl_u_lay_reg {
	union ovl_u_attctl_reg ctl;
	union ovl_u_mbsize_reg mbsize;
	union ovl_u_coor_reg mbcoor;
	union ovl_u_pitch_reg pitch;
	union ovl_u_laddr_reg top_laddr;
	union ovl_u_laddr_reg bot_laddr;
	union ovl_u_fcolor_reg fcolor;
	u32 res0;
};

struct ovl_u_reg {
	struct ovl_u_lay_reg lay[4];
	union ovl_u_haddr_reg top_haddr;
	union ovl_u_haddr_reg bot_haddr;
	union ovl_u_win_size_reg win_size;
	u32 res0;
	u32 res1[24];
	union ovl_u_dsctl_reg hori_ds;
	u32 res2;
	union ovl_u_dsctl_reg vert_ds;
	u32 res3;
};


#endif /* #ifndef _DE_OVL_TYPE_H_ */

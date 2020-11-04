/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_DNS_TYPE_H_
#define _DE_DNS_TYPE_H_

#include <linux/types.h>
#include "de_rtmx.h"

union dns_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 winsz_sel:1;
		u32 res0:29;
		u32 win_en:1;
	} bits;
};

union dns_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

union dns_win0_reg {
	u32 dwval;
	struct {
		u32 win_left:13;
		u32 res0:3;
		u32 win_top:13;
		u32 res1:3;
	} bits;
};

union dns_win1_reg {
	u32 dwval;
	struct {
		u32 win_right:13;
		u32 res0:3;
		u32 win_bot:13;
		u32 res1:3;
	} bits;
};

/* lft: lumFilter */
union dns_lft_para0_reg {
	u32 dwval;
	struct {
		u32 lsig:3;
		u32 res0:5;
		u32 lsig2:8;
		u32 lsig3:8;
		u32 ldir_rsig_gain2:8;
	} bits;
};

union dns_lft_para1_reg {
	u32 dwval;
	struct {
		u32 ldir_cen:8;
		u32 ldir_rsig_gain:8;
		u32 ldir_thrlow:8;
		u32 ldir_thrhigh:8;
	} bits;
};

union dns_lft_para2_reg {
	u32 dwval;
	struct {
		u32 lbben:1;
		u32 res0:7;
		u32 lbgain:8;
		u32 lbyst:4;
		u32 lbxst:4;
		u32 lbw:4;
		u32 lbh:4;
	} bits;
};

union dns_lft_para3_reg {
	u32 dwval;
	struct {
		u32 lsig_cen:8;
		u32 res0:24;
	} bits;
};

union iqa_ctl_reg {
	u32 dwval;
	struct {
		u32 res0:1;
		u32 res1:15;
		u32 max:10;
		u32 res2:6;
	} bits;
};

union iqa_sum_reg {
	u32 dwval;
	struct {
		u32 sum;
	} bits;
};

union iqa_sta_reg {
	u32 dwval;
	struct {
		u32 sta;
	} bits;
};

union iqa_blkdt_para0_reg {
	u32 dwval;
	struct {
		u32 dt_thrlow:8;
		u32 dt_thrhigh:8;
		u32 res0:16;
	} bits;
};

union iqa_blkdt_sum_reg {
	u32 dwval;
	struct {
		u32 dt_blksum:27;
		u32 res0:5;
	} bits;
};

union iqa_blkdt_num_reg {
	u32 dwval;
	struct {
		u32 dt_blknum:19;
		u32 res0:13;
	} bits;
};

struct dns_reg {
	union dns_ctl_reg ctrl;         /* 0x0000 */
	union dns_size_reg size;        /* 0x0004 */
	union dns_win0_reg win0;        /* 0x0008 */
	union dns_win1_reg win1;        /* 0x000c */
	union dns_lft_para0_reg lpara0; /* 0x0010 */
	union dns_lft_para1_reg lpara1; /* 0x0014 */
	union dns_lft_para2_reg lpara2; /* 0x0018 */
	union dns_lft_para3_reg lpara3; /* 0x001c */
};

struct iqa_reg {
	union iqa_ctl_reg iqa_ctl;			   /* 0x0100 */
	union iqa_sum_reg iqa_sum;			   /* 0x0104 */
	union iqa_sta_reg iqa_sta[13];		   /* 0x0108~0x0138 */
	union iqa_blkdt_para0_reg blk_dt_para; /* 0x013c */
	union iqa_blkdt_sum_reg blk_dt_sum;    /* 0x0140 */
	union iqa_blkdt_num_reg blk_dt_num;    /* 0x0144 */
};

struct de_dns_para {
	s32 w;
	s32 h;

	s32 win_en;
	s32 win_left;
	s32 win_top;
	s32 win_right;
	s32 win_bot;

	s32 en;
	s32 autolvl;
	s32 winsz_sel;
	s32 sig;
	s32 sig2;
	s32 sig3;
	s32 rsig_cen;
	s32 dir_center;
	s32 dir_thrhigh;
	s32 dir_thrlow;

	s32 dir_rsig_gain;
	s32 dir_rsig_gain2;

	s32 bgain;
	s32 xst;
	s32 yst;

};

struct __dns_config_data {
	u32 mod_en; /* return mod en info */

	/* parameter */
	u32 level; /* user level */
	u32 inw;
	u32 inh;
	struct disp_rect croprect;
};

#endif /* #ifndef _DE_DNS_TYPE_H_ */

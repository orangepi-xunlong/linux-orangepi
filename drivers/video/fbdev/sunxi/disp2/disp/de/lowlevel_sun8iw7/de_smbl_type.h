/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/**
 *	All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *	File name   :       de_smbl_type.h
 *
 *	Description :       display engine 2.0 smbl struct declaration
 *
 *	History     :       2014/05/13  vito cheng  v0.1  Initial version
 *
 */
#ifndef __DE_SMBL_TYPE_H__
#define __DE_SMBL_TYPE_H__

#include "de_rtmx.h"

#define SMBL_FRAME_MASK 0x00000002
/*
* 0x0: do SMBL in even frame;
* 0x1, do SMBL in odd frame;
* 0x2, do SMBL in all frames
*/

#define IEP_LH_INTERVAL_NUM 8
#define IEP_LH_PWRSV_NUM 24


typedef union {
	__u32 dwval;
	struct {
		__u32	en				:1;
		__u32   incsc_en		:1;
		__u32	r0				:2;
		__u32   coef_switch_en  :1;
		__u32   r1				:3;
		__u32	mod				:2;
		__u32	r2				:21;
		__u32	bist_en			:1;
	} bits;
} __imgehc_gnectl_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32 disp_w			:13;
		__u32 r0				:3;
		__u32 disp_h			:13;
		__u32 r1				:3;
	} bits;
} __imgehc_drcsize_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	db_en			:1;
		__u32	r0				:7;
		__u32	win_en			:1;
		__u32   hsv_en			:1;
		__u32	r1				:22;
	} bits;
} __imgehc_drcctl_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	lgc_abslumshf	:1;
		__u32	adjust_en		:1;
		__u32	r0				:6;
		__u32	lgc_abslumperval:8;
		__u32	r1				:16;
	} bits;
} __imgehc_drc_set_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	win_left        :12;
		__u32	r0				:4;
		__u32	win_top			:12;
		__u32	r1				:4;
	} bits;
} __imgehc_drc_wp_reg0_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	win_right		:12;
		__u32	r0				:4;
		__u32	win_bottom		:12;
		__u32	r1				:4;
	} bits;
} __imgehc_drc_wp_reg1_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	lh_rec_clr		:1;
		__u32	lh_mod			:1;
		__u32	r0				:30;
	} bits;
} __imgehc_lhctl_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	lh_thres_val1	:8;
		__u32	lh_thres_val2	:8;
		__u32	lh_thres_val3	:8;
		__u32	lh_thres_val4	:8;
	} bits;
} __imgehc_lhthr_reg0_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	lh_thres_val5	:8;
		__u32	lh_thres_val6	:8;
		__u32	lh_thres_val7	:8;
		__u32	r0				:8;
	} bits;
} __imgehc_lhthr_reg1_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	lh_lum_data		:32;
	} bits;
} __imgehc_lhslum_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	lh_cnt_data		:32;
	} bits;
} __imgehc_lhscnt_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	csc_yg_coff		:13;
		__u32	r0				:19;
	} bits;
} __imgehc_cscygcoff_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	csc_yg_con		:14;
		__u32	r0				:18;
	} bits;
} __imgehc_cscygcon_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	csc_ur_coff		:13;
		__u32	r0				:19;
	} bits;
} __imgehc_cscurcoff_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	csc_ur_con		:14;
		__u32	r0				:18;
	} bits;
} __imgehc_cscurcon_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	csc_vb_coff		:13;
		__u32	r0				:19;
	} bits;
} __imgehc_cscvbcoff_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	csc_vb_con		:14;
		__u32	r0				:18;
	} bits;
} __imgehc_cscvbcon_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	spa_coff0		:8;
		__u32	spa_coff1		:8;
		__u32	spa_coff2		:8;
		__u32	r0				:8;
	} bits;
} __imgehc_drcspacoff_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	inten_coff0		:8;
		__u32	inten_coff1		:8;
		__u32	inten_coff2		:8;
		__u32	inten_coff3		:8;
	} bits;
} __imgehc_drcintcoff_reg_t;

typedef union {
	__u32 dwval;
	struct {
		__u32	lumagain_coff0	:16;
		__u32	lumagain_coff1	:16;
	} bits;
} __imgehc_drclgcoff_reg_t;


typedef struct {
	__imgehc_gnectl_reg_t			gnectl;
	__imgehc_drcsize_reg_t          drcsize;
	__u32							r0[2];
	__imgehc_drcctl_reg_t			drcctl;
	__u32							r1;
	__imgehc_drc_set_reg_t			drc_set;
	__imgehc_drc_wp_reg0_t			drc_wp0;
	__imgehc_drc_wp_reg1_t			drc_wp1;
	__u32                           r5[3];
	__imgehc_lhctl_reg_t			lhctl;
	__imgehc_lhthr_reg0_t			lhthr0;
	__imgehc_lhthr_reg1_t			lhthr1;
	__u32							r2;
	__imgehc_lhslum_reg_t			lhslum[8];
	__imgehc_lhscnt_reg_t			lhscnt[8];
	__imgehc_cscygcoff_reg_t		incscycoff[3];
	__imgehc_cscygcon_reg_t			incscycon;
	__imgehc_cscurcoff_reg_t		incscucoff[3];
	__imgehc_cscurcon_reg_t			incscucon;
	__imgehc_cscvbcoff_reg_t		incscvcoff[3];
	__imgehc_cscvbcon_reg_t			incscvcon;
	__u32							r6[4];
	__imgehc_cscygcoff_reg_t		cscrcoff[3];
	__imgehc_cscygcon_reg_t			cscrcon;
	__imgehc_cscurcoff_reg_t		cscgcoff[3];
	__imgehc_cscurcon_reg_t			cscgcon;
	__imgehc_cscvbcoff_reg_t		cscbcoff[3];
	__imgehc_cscvbcon_reg_t			cscbcon;
	__imgehc_drcspacoff_reg_t		drcspacoff[3];
	__u32							r4;
	__imgehc_drcintcoff_reg_t		drcintcoff[64];
	__imgehc_drclgcoff_reg_t		drclgcoff[128];
} __smbl_reg_t;

typedef struct {
	unsigned int IsEnable;
	unsigned int Runtime;
	unsigned int backlight;
	unsigned int dimming;
	unsigned char min_adj_index_hist[IEP_LH_PWRSV_NUM];
	unsigned int size;
} __smbl_status_t;

#endif

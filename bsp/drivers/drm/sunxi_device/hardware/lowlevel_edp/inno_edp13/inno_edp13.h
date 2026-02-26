/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * inno_edp13.h
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __INNO_EDP13_LOWLEVEL_H__
#define __INNO_EDP13_LOWLEVEL_H__

#include <linux/types.h>

/*INNO EDP 1.3 register*/
#define REG_EDP_HPD_SCALE        0x0018
#define REG_EDP_RESET            0x001c
#define REG_EDP_HPD_EVENT        0x0080
#define REG_EDP_HPD_INT          0x0084
#define REG_EDP_HPD_PLUG         0x0088
#define REG_EDP_HPD_EN           0x008c
#define REG_EDP_CAPACITY         0x0100
#define REG_EDP_ANA_PLL_FBDIV    0x0180
#define REG_EDP_ANA_PLL_FRAC     0x0184
#define REG_EDP_ANA_PLL_POSDIV   0x0188
#define REG_EDP_ANA_AUX_CLOCK    0x018c
#define REG_EDP_ANA_PIXPLL_FBDIV 0x0190
#define REG_EDP_ANA_PIXPLL_DIV   0x0194
#define REG_EDP_ANA_PIXPLL_FRAC  0x019c
#define REG_EDP_AUX_FILTTER      0x01a0
#define REG_EDP_TX32_ISEL_DRV    0x01a4
#define REG_EDP_TX_MAINSEL       0x01a8
#define REG_EDP_TX_POSTSEL       0x01ac
#define REG_EDP_TX_PRESEL        0x01b0
#define REG_EDP_TX_RCAL_SEL      0x01c0
#define REG_EDP_TX_RTM           0x01c4
#define REG_EDP_VIDEO_STREAM_EN  0x0200
#define REG_EDP_SYNC_POLARITY    0x020c
#define REG_EDP_HACTIVE_BLANK    0x0210
#define REG_EDP_VACTIVE_BLANK    0x0214
#define REG_EDP_HSW_FRONT_PORCH  0x0218
#define REG_EDP_VSW_FRONT_PORCH  0x021c
#define REG_EDP_FRAME_UNIT       0x0220
#define REG_EDP_SYNC_START       0x0224
#define REG_EDP_MSA_MISC0        0x0228
#define REG_EDP_MSA_MISC1        0x022c
#define REG_EDP_HBLANK_LINK_CYC  0x0230
#define REG_EDP_AUDIO            0x0300
#define REG_EDP_PHY_AUX          0x0400
#define REG_EDP_AUX_TIMEOUT      0x0404
#define REG_EDP_AUX_DATA0        0x0408
#define REG_EDP_AUX_DATA1        0x040c
#define REG_EDP_AUX_DATA2        0x0410
#define REG_EDP_AUX_DATA3        0x0414
#define REG_EDP_AUX_START        0x0418
#define REG_EDP_AUDIO_VBLANK_EN  0x0500
#define REG_EDP_AUDIO_HBLANK_EN  0x0504
#define REG_EDP_BIST_CFG         0x2010
#define REG_EDP_RES1000_CFG      0x2014

/* not define in spec, inno tell it by Wechat, record in email */
#define REG_EDP_AUX_ISEL_MAINSET 0x01a0

#define TRAIN_CNT 4
#define VOL_SWING_LEVEL_NUM 4
#define PRE_EMPHASIS_LEVEL_NUM 4

#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	(((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	(((reg) & CLRMASK(width, shift)) | (val << (shift)))
#define CLR_BITS(shift, width, reg) \
	(((reg) & CLRMASK(width, shift)))

struct training_param {
	u32 sw_lv;
	u32 pre_lv;
};

struct recommand_corepll {
	u32 prediv;			/* "DMA packet" head descriptor */
	u32 fbdiv_h4;
	u32 fbdiv_l8;			/* current descriptor */
	u32 postdiv;
	u32 frac_pd;			/* flags, remaining buflen */
	u32 frac_h8;
	u32 frac_m8;			/* unused */
	u32 frac_l8;
};

struct recommand_pixpll {
	u32 pixel_clk;
	u32 prediv;			/* "DMA packet" head descriptor */
	u32 fbdiv_h4;
	u32 fbdiv_l8;			/* current descriptor */
	u32 plldiv_a;
	u32 plldiv_b;
	u32 plldiv_c;
	u32 frac_pd;			/* flags, remaining buflen */
	u32 frac_h8;
	u32 frac_m8;			/* unused */
	u32 frac_l8;
};

#endif /*End of file*/

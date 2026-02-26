/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * api for inno edp tx based on edp_1.3 hardware operation
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "../edp_core/edp_core.h"
#include "inno_edp13_lowlevel.h"
#include "edp_lowlevel.h"
#include "../edp_configs.h"
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <video/sunxi_edp.h>

/*link symbol per TU*/
#define LS_PER_TU 64

static void __iomem *edp_base[2];
static u64 g_bit_rate;
static u32 cur_request;

struct mutex aux_lock;

/* training_param_array [param_type][sw][pre]
 * param_type as follow:
 *
 * 0: low swing for 200mv-350mv
 * 1: high swing for 300mv-450mv
 * 2: balanced swing for 200mv-500mv
 *
 * */
static struct training_param training_param_table[3][4][4] = {
	{
		{
			{.sw_lv = 0x1, .pre_lv = 0x0,},
			{.sw_lv = 0x3, .pre_lv = 0x4,},
			{.sw_lv = 0x5, .pre_lv = 0x7,},
			{.sw_lv = 0x5, .pre_lv = 0x7,},
		},
		{
			{.sw_lv = 0x2, .pre_lv = 0x0,},
			{.sw_lv = 0x5, .pre_lv = 0x4,},
			{.sw_lv = 0x7, .pre_lv = 0x7,},
			{.sw_lv = 0x7, .pre_lv = 0x7,},
		},
		{
			{.sw_lv = 0x3, .pre_lv = 0x0,},
			{.sw_lv = 0x6, .pre_lv = 0x4,},
			{.sw_lv = 0xa, .pre_lv = 0x7,},
			{.sw_lv = 0xa, .pre_lv = 0x7,},
		},
		{
			{.sw_lv = 0x5, .pre_lv = 0x0,},
			{.sw_lv = 0x8, .pre_lv = 0x4,},
			{.sw_lv = 0xf, .pre_lv = 0x7,},
			{.sw_lv = 0xf, .pre_lv = 0x7,},
		},
	},
	{
		{
			{.sw_lv = 0x3, .pre_lv = 0x0,},
			{.sw_lv = 0x6, .pre_lv = 0x4,},
			{.sw_lv = 0xa, .pre_lv = 0x7,},
			{.sw_lv = 0xa, .pre_lv = 0x7,},
		},
		{
			{.sw_lv = 0x5, .pre_lv = 0x0,},
			{.sw_lv = 0x8, .pre_lv = 0x4,},
			{.sw_lv = 0xf, .pre_lv = 0x7,},
			{.sw_lv = 0xf, .pre_lv = 0x7,},
		},
		{
			{.sw_lv = 0x6, .pre_lv = 0x0,},
			{.sw_lv = 0xb, .pre_lv = 0x4,},
			{.sw_lv = 0xb, .pre_lv = 0x4,},
			{.sw_lv = 0xb, .pre_lv = 0x4,},
		},
		{
			{.sw_lv = 0x8, .pre_lv = 0x0,},
			{.sw_lv = 0x8, .pre_lv = 0x0,},
			{.sw_lv = 0x8, .pre_lv = 0x0,},
			{.sw_lv = 0x8, .pre_lv = 0x0,},
		},
	},
	{
		{
			{.sw_lv = 0x1, .pre_lv = 0x0,},
			{.sw_lv = 0x3, .pre_lv = 0x4,},
			{.sw_lv = 0x5, .pre_lv = 0x7,},
			{.sw_lv = 0x5, .pre_lv = 0x7,},
		},
		{
			{.sw_lv = 0x3, .pre_lv = 0x0,},
			{.sw_lv = 0x6, .pre_lv = 0x4,},
			{.sw_lv = 0xa, .pre_lv = 0x7,},
			{.sw_lv = 0xa, .pre_lv = 0x7,},
		},
		{
			{.sw_lv = 0x6, .pre_lv = 0x0,},
			{.sw_lv = 0xb, .pre_lv = 0x4,},
			{.sw_lv = 0xb, .pre_lv = 0x4,},
			{.sw_lv = 0xb, .pre_lv = 0x4,},
		},
		{
			{.sw_lv = 0x9, .pre_lv = 0x0,},
			{.sw_lv = 0x9, .pre_lv = 0x0,},
			{.sw_lv = 0x9, .pre_lv = 0x0,},
			{.sw_lv = 0x9, .pre_lv = 0x0,},
		},
	},
};

static struct recommand_corepll recom_corepll[] = {
	/* 1.62G*/
	{
		.prediv = 0x2,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x87,
		.postdiv = 0x1,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,
	},
	/*2.7G*/
	{
		.prediv = 0x2,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0xe1,
		.postdiv = 0x1,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,
	},
	{},
};

/*0:voltage_mode  1:cureent_mode*/
static void edp_mode_init(u32 sel, u32 mode)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_TX_PRESEL);
	if (mode) {
		reg_val = SET_BITS(24, 4, reg_val, 0xf);
	} else {
		reg_val = SET_BITS(24, 4, reg_val, 0x0);
	}
	writel(reg_val, edp_base[sel] + REG_EDP_TX_PRESEL);
}

static void edp_resistance_init(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_RES1000_CFG);
	reg_val = SET_BITS(8, 1, reg_val, 0x1);
	reg_val = SET_BITS(0, 6, reg_val, 0x0);
	writel(reg_val, edp_base[sel] + REG_EDP_RES1000_CFG);
}

void edp_aux_16m_config(u32 sel, u64 bit_rate)
{
	u32 bit_clock = 0;
	u32 div_16m = 0;
	u32 reg_val;

	if (bit_rate == BIT_RATE_1G62)
		bit_clock = 810;
	else if (bit_rate == BIT_RATE_2G7)
		bit_clock = 1350;
	else {
		EDP_ERR("no bit_clock match for bitrate:%lld\n", bit_rate);
		return;
	}

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_AUX_CLOCK);
	div_16m = (bit_clock / 8) / 8;
	reg_val = SET_BITS(0, 5, reg_val, div_16m);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_AUX_CLOCK);

	/*set main isel*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
	reg_val = SET_BITS(16, 5, reg_val, 0x14);
	reg_val = SET_BITS(24, 5, reg_val, 0x14);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_MAINSEL);

	reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
	reg_val = SET_BITS(0, 5, reg_val, 0x14);
	reg_val = SET_BITS(8, 5, reg_val, 0x14);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);

	/*set pre isel*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_PRESEL);
	reg_val = SET_BITS(0, 15, reg_val, 0x0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_PRESEL);

	/* set aux channel swing level to max, enhance compatibility */
	reg_val = readl(edp_base[sel] + REG_EDP_AUX_ISEL_MAINSET);
	/* AUX ISEL */
	reg_val = SET_BITS(4, 4, reg_val, 0xf);
	/* AUX MAINSET */
	reg_val = SET_BITS(8, 4, reg_val, 0xf);
	writel(reg_val, edp_base[sel] + REG_EDP_AUX_ISEL_MAINSET);
}

void edp_corepll_config(u32 sel, u64 bit_rate)
{
	u32 reg_val;
	u32 index;

	if (bit_rate == BIT_RATE_1G62)
		index = 0;
	else
		index = 1;
	g_bit_rate = bit_rate;

	/*turnoff corepll*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll prediv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(8, 6, reg_val, recom_corepll[index].prediv);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll fbdiv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(16, 4, reg_val, recom_corepll[index].fbdiv_h4);
	reg_val = SET_BITS(24, 8, reg_val, recom_corepll[index].fbdiv_l8);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll postdiv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_POSDIV);
	reg_val = SET_BITS(2, 2, reg_val, recom_corepll[index].postdiv);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_POSDIV);

	/*config corepll frac_pd*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(4, 2, reg_val, recom_corepll[index].frac_pd);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll frac*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FRAC);
	reg_val = SET_BITS(0, 8, reg_val, recom_corepll[index].frac_h8);
	reg_val = SET_BITS(8, 8, reg_val, recom_corepll[index].frac_m8);
	reg_val = SET_BITS(16, 8, reg_val, recom_corepll[index].frac_l8);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FRAC);

	/*turnon corepll*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
}

s32 pixpll_cal(u32 pixel_clk, struct recommand_pixpll *pixpll)
{
	u32 pre_div = 0, fbdiv = 0;
	u32 frac_div = 0, frac = 0, try_cnt = 0;
	u32 pclk_div, pclk_diva, pclk_divc, pclk_divb = 0;
	u32 i = 0, match_flag = 0;

	pclk_div = 0;
	pre_div = 1;
	pclk_diva = 2;

	/* try to match non-frac param */
	while (try_cnt < 31) {
		pclk_div = pclk_div + pclk_diva;

		/* fixme */
		/* should keep fvco in 1GHz-3GHz range */
		if (((pixel_clk * 2 * pclk_div) < 1000000000) || ((pixel_clk * 2 * pclk_div) > 3000000000))
			continue;

		fbdiv = pixel_clk * 2 * pclk_div * pre_div / 24000000;
		frac = pixel_clk * 2 * pclk_div * pre_div % 24000000;
		frac = frac / 100000;
		frac_div = frac * 1024 * 1024 * 16 / 240;

		if (!frac) {
			match_flag = 1;
			break;
		}
		try_cnt++;
	}

	/* param non-frac is not match, try frac param */
	if (!match_flag) {
		for (i = 1; i < 31; i++) {
			pclk_div = pclk_diva * i;
			/* should keep fvco in 1GHz-3GHz range */
			if (((pixel_clk * 2 * pclk_div) > 1000000000) && ((pixel_clk * 2 * pclk_div) < 3000000000))
				break;

			if (i >= 30) {
				EDP_ERR("pixel_clk param calculate fail!\n");
				return RET_FAIL;
			}
		}
		fbdiv = pixel_clk * 2 * pclk_div * pre_div / 24000000;
		frac = pixel_clk * 2 * pclk_div * pre_div % 24000000;
		frac = frac / 100000;
		frac_div = frac * 1024 * 1024 * 16 / 240;
	}

	pclk_divc = pclk_div / pclk_diva;

	pixpll->prediv = pre_div;
	pixpll->fbdiv_h4 = (fbdiv >> 8) & 0xff;
	pixpll->fbdiv_l8 = fbdiv & 0xff;
	pixpll->plldiv_a = pclk_diva;
	pixpll->plldiv_b = pclk_divb;
	pixpll->plldiv_c = pclk_divc;
	pixpll->frac_pd = match_flag ? 0x3 : 0x0;
	pixpll->frac_h8 = (frac_div >> 16) & 0xff;
	pixpll->frac_m8 = (frac_div >> 8) & 0xff;
	pixpll->frac_l8 = frac_div & 0xff;
	EDP_LOW_DBG("<%s>-<%d>: pixclk:%d prediv:0x%x fbdiv_h4:0x%x\n",\
		__func__, __LINE__, pixel_clk, pixpll->prediv, pixpll->fbdiv_h4);
	EDP_LOW_DBG("fbdiv_l8:0x%x plldiv_a:0x%x plldiv_b:0x%x plldiv_c:0x%x\n",\
		pixpll->fbdiv_l8, pixpll->plldiv_a, pixpll->plldiv_b, pixpll->plldiv_c);
	EDP_LOW_DBG("frac_pd:0x%x frac_h8:%x frac_m8:0x%x frac_l8:0x%x\n",\
		pixpll->frac_pd, pixpll->frac_h8, pixpll->frac_m8, pixpll->frac_l8);

	return RET_OK;

}

s32 edp_pixpll_cfg(u32 sel, u32 pixel_clk)
{
	u32 reg_val;
	s32 ret = 0;
	struct recommand_pixpll pixpll;

	memset(&pixpll, 0, sizeof(struct recommand_pixpll));

	ret = pixpll_cal(pixel_clk, &pixpll);
	if (ret)
		return ret;

	/*turnoff pixpll*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll prediv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(8, 6, reg_val, pixpll.prediv);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll fbdiv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(16, 4, reg_val, pixpll.fbdiv_h4);
	reg_val = SET_BITS(24, 8, reg_val, pixpll.fbdiv_l8);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll divabc*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_DIV);
	reg_val = SET_BITS(0, 5, reg_val, pixpll.plldiv_a);
	reg_val = SET_BITS(8, 2, reg_val, pixpll.plldiv_b);
	reg_val = SET_BITS(16, 5, reg_val, pixpll.plldiv_c);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_DIV);

	/*config pixpll frac_pd*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(4, 2, reg_val, pixpll.frac_pd);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll frac*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FRAC);
	reg_val = SET_BITS(0, 8, reg_val, pixpll.frac_h8);
	reg_val = SET_BITS(8, 8, reg_val, pixpll.frac_m8);
	reg_val = SET_BITS(16, 8, reg_val, pixpll.frac_l8);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FRAC);

	/*turnon pixpll*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);

	return RET_OK;
}

void edp_set_misc(u32 sel, u32 misc0_val, u32 misc1_val)
{
	u32 reg_val;

	/*misc0 setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_MSA_MISC0);
	reg_val = SET_BITS(24, 8, reg_val, misc0_val);
	writel(reg_val, edp_base[sel] + REG_EDP_MSA_MISC0);

	/*misc1 setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_MSA_MISC1);
	reg_val = SET_BITS(24, 8, reg_val, misc1_val);
	writel(reg_val, edp_base[sel] + REG_EDP_MSA_MISC1);
}


void edp_video_stream_enable(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(5, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
}

void edp_video_stream_disable(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(5, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
}


void edp_hal_set_training_pattern(u32 sel, u32 pattern)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	reg_val = SET_BITS(0, 4, reg_val, pattern);
	writel(reg_val, edp_base[sel] + REG_EDP_CAPACITY);
}


void edp_audio_stream_vblank_setting(u32 sel, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
	reg_val = SET_BITS(1, 1, reg_val, enable);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
}

void edp_audio_timestamp_vblank_setting(u32 sel, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
	reg_val = SET_BITS(0, 1, reg_val, enable);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
}

void edp_audio_stream_hblank_setting(u32 sel, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
	reg_val = SET_BITS(1, 1, reg_val, enable);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
}

void edp_audio_timestamp_hblank_setting(u32 sel, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
	reg_val = SET_BITS(0, 1, reg_val, enable);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
}

void edp_audio_interface_config(u32 sel, u32 interface)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = SET_BITS(0, 1, reg_val, interface);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO);
}

void edp_audio_channel_config(u32 sel, u32 chn_num)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);

	switch (chn_num) {
	case 1:
		reg_val = SET_BITS(12, 3, reg_val, 0x0);
		reg_val = SET_BITS(1, 4, reg_val, 0x1);
		break;
	case 2:
		reg_val = SET_BITS(12, 3, reg_val, 0x1);
		reg_val = SET_BITS(1, 4, reg_val, 0x1);
		break;
	case 8:
		reg_val = SET_BITS(12, 3, reg_val, 0x7);
		reg_val = SET_BITS(1, 4, reg_val, 0xf);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO);
}

void edp_audio_mute_config(u32 sel, bool mute)
{

	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = SET_BITS(15, 1, reg_val, mute);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO);
}

void edp_audio_data_width_config(u32 sel, u32 data_width)
{

	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	switch (data_width) {
	case 16:
		reg_val = SET_BITS(5, 5, reg_val, 0x10);
		break;
	case 20:
		reg_val = SET_BITS(5, 5, reg_val, 0x14);
		break;
	case 24:
		reg_val = SET_BITS(5, 5, reg_val, 0x18);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO);
}

void edp_audio_soft_reset(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_RESET);
	reg_val = SET_BITS(3, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_RESET);
	reg_val = SET_BITS(3, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_RESET);
}

void edp_set_input_video_mapping(u32 sel, enum edp_video_mapping_e mapping)
{
	u32 reg_val;
	u32 mapping_val;
	u32 misc0_val = 0;
	u32 misc1_val = 0;

	switch (mapping) {
	case RGB_6BIT:
		mapping_val = 0;
		misc0_val = (0 << 5);
		break;
	case RGB_8BIT:
		mapping_val = 1;
		misc0_val = (1 << 5);
		break;
	case RGB_10BIT:
		mapping_val = 2;
		misc0_val = (2 << 5);
		break;
	case RGB_12BIT:
		mapping_val = 3;
		misc0_val = (3 << 5);
		break;
	case RGB_16BIT:
		mapping_val = 4;
		misc0_val = (4 << 5);
		break;
	case YCBCR444_8BIT:
		mapping_val = 5;
		misc0_val = (1 << 5) | (1 << 2);
		break;
	case YCBCR444_10BIT:
		mapping_val = 6;
		misc0_val = (2 << 5) | (1 << 2);
		break;
	case YCBCR444_12BIT:
		mapping_val = 7;
		misc0_val = (3 << 5) | (1 << 2);
		break;
	case YCBCR444_16BIT:
		mapping_val = 8;
		misc0_val = (4 << 5) | (1 << 2);
		break;
	case YCBCR422_8BIT:
		mapping_val = 9;
		misc0_val = (1 << 5) | (1 << 1);
		break;
	case YCBCR422_10BIT:
		mapping_val = 10;
		misc0_val = (2 << 5) | (1 << 1);
		break;
	case YCBCR422_12BIT:
		mapping_val = 11;
		misc0_val = (3 << 5) | (1 << 1);
		break;
	case YCBCR422_16BIT:
		mapping_val = 12;
		misc0_val = (4 << 5) | (1 << 1);
		break;
	}

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(16, 5, reg_val, mapping_val);
	writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);

	edp_set_misc(sel, misc0_val, misc1_val);
}

s32 edp_bist_test(u32 sel, struct edp_tx_core *edp_core)
{
	u32 reg_val;
	s32 ret;

	/*set bist_test_sel to 1*/
	reg_val = readl(edp_base[sel] + REG_EDP_BIST_CFG);
	reg_val = SET_BITS(0, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_BIST_CFG);

	/*assert reset pin*/
	if (gpio_is_valid(edp_core->rst_gpio)) {
		gpio_direction_output(edp_core->rst_gpio, 0);
	}

	/*set bist_test_en to 1*/
	reg_val = readl(edp_base[sel] + REG_EDP_BIST_CFG);
	reg_val = SET_BITS(1, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_BIST_CFG);

	usleep_range(5, 10);

	/*wait for bist_test_done to 1*/
	while (1) {
		reg_val = readl(edp_base[sel] + REG_EDP_BIST_CFG);
		reg_val = GET_BITS(4, 1, reg_val);
		if (reg_val == 1)
			break;
	}

	/*checke bist_test_done*/
	reg_val = readl(edp_base[sel] + REG_EDP_BIST_CFG);
	reg_val = GET_BITS(5, 1, reg_val);
	if (reg_val == 1)
		ret = RET_OK;
	else
		ret = RET_FAIL;

	/*deassert reset pin and disable bist_test_sel to exit bist*/
	if (gpio_is_valid(edp_core->rst_gpio)) {
		gpio_direction_output(edp_core->rst_gpio, 1);
	}
	writel(0x0, edp_base[sel] + REG_EDP_BIST_CFG);

	return RET_OK;
}

void edp_phy_soft_reset(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_RESET);
	reg_val = SET_BITS(1, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_RESET);
	usleep_range(5, 10);
	reg_val = SET_BITS(1, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_RESET);
}

void edp_main_link_reset(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	reg_val = SET_BITS(0, 12, reg_val, 0);
	reg_val = SET_BITS(26, 3, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_CAPACITY);
}

void edp_hpd_irq_enable(u32 sel)
{
	writel(0x1, edp_base[sel] + REG_EDP_HPD_INT);
	writel(0x6, edp_base[sel] + REG_EDP_HPD_EN);
}

void edp_hpd_irq_disable(u32 sel)
{
	writel(0x0, edp_base[sel] + REG_EDP_HPD_INT);
	writel(0x0, edp_base[sel] + REG_EDP_HPD_EN);
}

void edp_hpd_enable(u32 sel)
{
	u32 reg_val;
	u32 reg_val1;

	reg_val = readl(edp_base[sel] + REG_EDP_HPD_SCALE);
	reg_val = SET_BITS(3, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_HPD_SCALE);

	/* only hpd enable need, irq is not necesssary*/
	reg_val1 = readl(edp_base[sel] + REG_EDP_HPD_EN);
	reg_val1 = SET_BITS(1, 2, reg_val1, 3);
	writel(reg_val1, edp_base[sel] + REG_EDP_HPD_EN);
	edp_hpd_irq_enable(sel);
}

void edp_hpd_disable(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_HPD_SCALE);
	reg_val = SET_BITS(3, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_HPD_SCALE);
	edp_hpd_irq_disable(sel);
}


bool edp_hal_get_hotplug_change(u32 sel)
{
	u32 int_event_en = 0;
	u32 int_en = 0;
	u32 reg_val = 0;

	int_event_en = readl(edp_base[sel] + REG_EDP_HPD_INT);
	int_en = readl(edp_base[sel] + REG_EDP_HPD_EN);

	if ((int_event_en & (1 << 0)) &&
	    ((int_en & (1 << 1)) || (int_en & (1 << 2)))) {
		reg_val =  readl(edp_base[sel] + REG_EDP_HPD_PLUG) & ((1 << 1) | (1 << 2));
		return reg_val ? true : false;
	}

	return false;
}

s32 edp_hal_get_hotplug_state(u32 sel)
{
	bool hpd_plugin;
	bool hpd_plugout;

	/* hpd plugin interrupt */
	hpd_plugin = readl(edp_base[sel] + REG_EDP_HPD_PLUG) & (1 << 1);
	if (hpd_plugin == true) {
		writel((1 << 1), edp_base[sel] + REG_EDP_HPD_PLUG);
		return 1;;
	}

	/* hpd plugout interrupt */
	hpd_plugout = readl(edp_base[sel] + REG_EDP_HPD_PLUG) & (1 << 2);
	if (hpd_plugout == true) {
		writel((1 << 2), edp_base[sel] + REG_EDP_HPD_PLUG);
		return 0;
	}

	return 0;
}


void edp_hal_irq_handler(u32 sel, struct edp_tx_core *edp_core)
{
}


s32 edp_hal_irq_enable(u32 sel, u32 irq_id)
{
	edp_hpd_irq_enable(sel);
	return 0;
}

s32 edp_hal_irq_disable(u32 sel, u32 irq_id)
{
	/*fixme: irq is not need?*/
	//edp_hpd_irq_disable(sel);
	return 0;
}

s32 edp_hal_irq_query(u32 sel)
{
	return 0;
}

s32 edp_hal_irq_clear(u32 sel)
{
	return 0;
}

s32 edp_aux_read(u32 sel, s32 addr, s32 len, char *buf, bool retry)
{
	u32 reg_val[4];
	u32 regval = 0;
	s32 i;
	u32 timeout = 0;
	s32 ret = 0;

	if ((len > 16) || (len <= 0)) {
		EDP_ERR("aux read out of len:%d\n", len);
		return RET_FAIL;
	}

	memset(buf, 0, 16);

	mutex_lock(&aux_lock);
	if (!retry) {
		writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA0);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA1);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA2);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA3);
		writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
		regval = readl(edp_base[sel] + REG_EDP_HPD_EVENT);
		regval |= (1 << 1);
		writel(regval, edp_base[sel] + REG_EDP_HPD_EVENT);

		regval = 0;
		/* aux read request*/
		regval |= (len - 1);
		regval = SET_BITS(8, 20, regval, addr);
		regval = SET_BITS(28, 4, regval, NATIVE_READ);
		cur_request = regval;
		EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
		writel(regval, edp_base[sel] + REG_EDP_PHY_AUX);
		udelay(1);
		writel(1, edp_base[sel] + REG_EDP_AUX_START);
	} else {
		writel(1, edp_base[sel] + REG_EDP_AUX_START);
	}

	/* wait aux reply event */
	while (!(readl(edp_base[sel] + REG_EDP_HPD_EVENT) & (1 << 1))) {
		if (timeout >= 50000) {
			EDP_LOW_DBG("edp_aux_read wait AUX_REPLY event timeout, request:0x%x\n", cur_request);
			mutex_unlock(&aux_lock);
			return RET_AUX_TIMEOUT;
		}
		timeout++;
	}

	/* wait for AUX_REPLY*/
	//fixme
	regval = readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT);
	while (((readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
		if (timeout >= 50000) {
			EDP_LOW_DBG("edp_aux_read wait AUX_REPLY timeout, request:0x%x\n", cur_request);
			ret = RET_AUX_TIMEOUT;
			goto CLR_EVENT;
		}
		timeout++;
	}

	regval = readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT);
	/* not ensure, need confirm from inno */
	if (((regval >> 24) & 0xf) == 0xe) {
		EDP_LOW_DBG("edp_aux_read recieve without STOP, request:0x%x\n", cur_request);
		ret = RET_AUX_NO_STOP;
		goto CLR_EVENT;
	}
	regval &= 0xf0;
	if ((regval >> 4) == AUX_REPLY_NACK) {
		EDP_LOW_DBG("edp_aux_read recieve AUX_REPLY_NACK, request:0x%x\n", cur_request);
		ret = RET_AUX_NACK;
		goto CLR_EVENT;
	} else if ((regval >> 4) == AUX_REPLY_DEFER) {
		EDP_LOW_DBG("edp_aux_read recieve AUX_REPLY_DEFER, request:0x%x\n", cur_request);
		ret = RET_AUX_DEFER;
		goto CLR_EVENT;
	}

	/* aux read reply*/
	for (i = 0; i < 4; i++) {
		reg_val[i] = readl(edp_base[sel] + REG_EDP_AUX_DATA0 + i * 0x4);
		usleep_range(10, 20);
		EDP_LOW_DBG("[%s] result: reg_val[%d] = 0x%x\n", __func__, i, reg_val[i]);
	}

	for (i = 0; i < len; i++) {
		buf[i] = GET_BITS((i % 4) * 8, 8, reg_val[i / 4]);
	}

CLR_EVENT:
	regval = readl(edp_base[sel] + REG_EDP_HPD_EVENT);
	regval |= (1 << 1);
	writel(regval, edp_base[sel] + REG_EDP_HPD_EVENT);
	mutex_unlock(&aux_lock);

	return ret;
}

s32 edp_aux_write(u32 sel, s32 addr, s32 len, char *buf, bool retry)
{
	u32 reg_val[4];
	u32 regval = 0;
	u32 timeout = 0;
	s32 i;
	s32 ret = 0;

	if ((len > 16) || (len <= 0)) {
		EDP_ERR("aux read out of len:%d\n", len);
		return RET_FAIL;
	}

	memset(reg_val, 0, sizeof(reg_val));

	mutex_lock(&aux_lock);
	if (!retry) {
		writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA0);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA1);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA2);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA3);
		writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
		regval = readl(edp_base[sel] + REG_EDP_HPD_EVENT);
		regval |= (1 << 1);
		writel(regval, edp_base[sel] + REG_EDP_HPD_EVENT);

		for (i = 0; i < len; i++) {
			reg_val[i / 4] = SET_BITS((i % 4) * 8, 8, reg_val[i / 4], buf[i]);
		}

		for (i = 0; i < (1 + ((len - 1) / 4)); i++) {
			writel(reg_val[i], edp_base[sel] + REG_EDP_AUX_DATA0 + (i * 0x4));
			usleep_range(10, 20);
			EDP_LOW_DBG("[%s]: date: reg_val[%d] = 0x%x", __func__, i, reg_val[i]);
		}

		/* aux write request*/
		regval = 0;
		regval |= (len - 1);
		regval = SET_BITS(8, 20, regval, addr);
		regval = SET_BITS(28, 4, regval, NATIVE_WRITE);
		cur_request = regval;
		EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
		writel(regval, edp_base[sel] + REG_EDP_PHY_AUX);

		udelay(1);
		writel(1, edp_base[sel] + REG_EDP_AUX_START);
	} else {
		writel(1, edp_base[sel] + REG_EDP_AUX_START);
	}

	/* wait aux reply event */
	while (!(readl(edp_base[sel] + REG_EDP_HPD_EVENT) & (1 << 1))) {
		if (timeout >= 50000) {
			EDP_LOW_DBG("edp_aux_write wait AUX_REPLY event timeout, request:0x%x\n", cur_request);
			mutex_unlock(&aux_lock);
			return RET_AUX_TIMEOUT;
		}
		timeout++;
	}

	/* wait for AUX_REPLY*/
	while (((readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
		if (timeout >= 50000) {
			EDP_LOW_DBG("edp_aux_write wait AUX_REPLY timeout, request:0x%x\n", cur_request);
			ret = RET_AUX_TIMEOUT;
			goto CLR_EVENT;
		}
		timeout++;
	}

	regval = readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT);
	/* not ensure, need confirm from inno */
	if (((regval >> 24) & 0xf) == 0xe) {
		EDP_LOW_DBG("edp_aux_read recieve without STOP, request:0x%x\n", cur_request);
		ret = RET_AUX_NO_STOP;
		goto CLR_EVENT;
	}
	regval &= 0xf0;
	if ((regval >> 4) == AUX_REPLY_NACK) {
		EDP_LOW_DBG("edp_aux_write recieve AUX_NACK, request:0x%x\n", cur_request);
		ret = RET_AUX_NACK;
		goto CLR_EVENT;
	} else if ((regval >> 4) == AUX_REPLY_DEFER) {
		EDP_LOW_DBG("edp_aux_write recieve AUX_REPLY_DEFER, request:0x%x\n", cur_request);
		ret = RET_AUX_DEFER;
		goto CLR_EVENT;
	}

CLR_EVENT:
	regval = readl(edp_base[sel] + REG_EDP_HPD_EVENT);
	regval |= (1 << 1);
	writel(regval, edp_base[sel] + REG_EDP_HPD_EVENT);
	mutex_unlock(&aux_lock);

	return ret;
}

s32 edp_aux_i2c_read(u32 sel, s32 addr, s32 len, char *buf, bool retry)
{
	u32 reg_val[4];
	u32 regval = 0;
	s32 i;
	u32 timeout = 0;
	s32 ret = 0;

	if ((len > 16) || (len <= 0)) {
		EDP_ERR("aux read out of len:%d\n", len);
		return RET_FAIL;
	}

	memset(buf, 0, 16);

	mutex_lock(&aux_lock);
	if (!retry) {
		writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA0);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA1);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA2);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA3);
		writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
		regval = readl(edp_base[sel] + REG_EDP_HPD_EVENT);
		regval |= (1 << 1);
		writel(regval, edp_base[sel] + REG_EDP_HPD_EVENT);

		/* aux read request*/
		regval = 0;
		regval |= (len - 1);
		regval = SET_BITS(8, 20, regval, addr);
		regval = SET_BITS(28, 4, regval, AUX_I2C_READ);
		cur_request = regval;
		EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
		writel(regval, edp_base[sel] + REG_EDP_PHY_AUX);
		udelay(1);
		writel(1, edp_base[sel] + REG_EDP_AUX_START);
	} else {
		writel(1, edp_base[sel] + REG_EDP_AUX_START);
	}

	/* wait aux reply event */
	while (!(readl(edp_base[sel] + REG_EDP_HPD_EVENT) & (1 << 1))) {
		if (timeout >= 50000) {
			EDP_LOW_DBG("edp_aux_i2c_read wait AUX_REPLY event timeout, request:0x%x\n", cur_request);
			mutex_unlock(&aux_lock);
			return RET_AUX_TIMEOUT;
		}
		timeout++;
	}

	/* wait for AUX_REPLY*/
	while (((readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
		if (timeout >= 50000) {
			EDP_LOW_DBG("edp_aux_i2c_read wait AUX_REPLY timeout, request:0x%x\n", cur_request);
			ret = RET_AUX_TIMEOUT;
			goto CLR_EVENT;
		}
		timeout++;
	}

	regval = readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT);
	/* not ensure, need confirm from inno */
	if (((regval >> 24) & 0xf) == 0xe) {
		EDP_LOW_DBG("edp_aux_read recieve without STOP, request:0x%x\n", cur_request);
		ret = RET_AUX_NO_STOP;
		goto CLR_EVENT;
	}
	regval &= 0xf0;
	if ((regval >> 4) == AUX_REPLY_I2C_NACK) {
		EDP_LOW_DBG("edp_aux_i2c_read recieve AUX_REPLY_NACK, request:0x%x\n", cur_request);
		ret = RET_AUX_NACK;
		goto CLR_EVENT;
	} else if ((regval >> 4) == AUX_REPLY_I2C_DEFER) {
		EDP_LOW_DBG("edp_aux_i2c_read recieve AUX_REPLY_I2C_DEFER, request:0x%x\n", cur_request);
		ret = RET_AUX_DEFER;
		goto CLR_EVENT;
	}

	/* aux read reply*/
	for (i = 0; i < 4; i++) {
		reg_val[i] = readl(edp_base[sel] + REG_EDP_AUX_DATA0 + i * 0x4);
		EDP_LOW_DBG("[%s]: data: reg_val[%d] = 0x%x", __func__, i, reg_val[i]);
	}

	for (i = 0; i < len; i++) {
		buf[i] = GET_BITS((i % 4) * 8, 8, reg_val[i / 4]);
	}

CLR_EVENT:
	regval = readl(edp_base[sel] + REG_EDP_HPD_EVENT);
	regval |= (1 << 1);
	writel(regval, edp_base[sel] + REG_EDP_HPD_EVENT);
	mutex_unlock(&aux_lock);

	return ret;
}

s32 edp_aux_i2c_write(u32 sel, s32 addr, s32 len, char *buf, bool retry)
{
	u32 reg_val[4];
	u32 regval = 0;
	s32 i;
	u32 timeout = 0;
	s32 ret = 0;

	if ((len > 16) || (len <= 0)) {
		EDP_ERR("aux read out of len:%d\n", len);
		return RET_FAIL;
	}

	mutex_lock(&aux_lock);
	if (!retry) {
		writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA0);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA1);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA2);
		writel(0, edp_base[sel] + REG_EDP_AUX_DATA3);
		writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
		regval = readl(edp_base[sel] + REG_EDP_HPD_EVENT);
		regval |= (1 << 1);
		writel(regval, edp_base[sel] + REG_EDP_HPD_EVENT);


		for (i = 0; i < len; i++) {
			reg_val[i / 4] = SET_BITS(i % 4, 8, reg_val[i / 4], buf[i]);
		}

		for (i = 0; i < 4; i++) {
			writel(reg_val[i], edp_base[sel] + REG_EDP_AUX_DATA0 + i * 0x4);
			EDP_LOW_DBG("[%s]: data: reg_val[%d] = 0x%x", __func__, i, reg_val[i]);
		}

		/* aux write request*/
		regval = 0;
		regval |= (len - 1);
		regval = SET_BITS(8, 20, regval, addr);
		regval = SET_BITS(28, 4, regval, AUX_I2C_WRITE);
		cur_request = regval;
		EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
		writel(regval, edp_base[sel] + REG_EDP_PHY_AUX);

		udelay(1);
		writel(1, edp_base[sel] + REG_EDP_AUX_START);
	} else {
		writel(1, edp_base[sel] + REG_EDP_AUX_START);
	}

	/* wait aux reply event */
	while (!(readl(edp_base[sel] + REG_EDP_HPD_EVENT) & (1 << 1))) {
		if (timeout >= 50000) {
			EDP_LOW_DBG("edp_aux_i2c_write wait AUX_REPLY event timeout, request:0x%x\n", cur_request);
			mutex_unlock(&aux_lock);
			return RET_AUX_TIMEOUT;
		}
		timeout++;
	}

	/* wait for AUX_REPLY*/
	while (((readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
		if (timeout >= 50000) {
			EDP_LOW_DBG("edp_aux_i2c_write wait AUX_REPLY timeout, request:0x%x\n", cur_request);
			ret = RET_AUX_TIMEOUT;
			goto CLR_EVENT;
		}
		timeout++;
	}

	regval = readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT);
	/* not ensure, need confirm from inno */
	if (((regval >> 24) & 0xf) == 0xe) {
		EDP_LOW_DBG("edp_aux_read recieve without STOP, request:0x%x\n", cur_request);
		ret = RET_AUX_NO_STOP;
		goto CLR_EVENT;
	}
	regval &= 0xf0;
	if ((regval >> 4) == AUX_REPLY_I2C_NACK) {
		EDP_LOW_DBG("edp_aux_i2c_write recieve AUX_REPLY_NACK, request:0x%x\n", cur_request);
		ret = RET_AUX_NACK;
		goto CLR_EVENT;
	} else if ((regval >> 4) == AUX_REPLY_I2C_DEFER) {
		EDP_LOW_DBG("edp_aux_i2c_write recieve AUX_REPLY_I2C_DEFER, request:0x%x\n", cur_request);
		ret = RET_AUX_DEFER;
		goto CLR_EVENT;
	}

CLR_EVENT:
	regval = readl(edp_base[sel] + REG_EDP_HPD_EVENT);
	regval |= (1 << 1);
	writel(regval, edp_base[sel] + REG_EDP_HPD_EVENT);
	mutex_unlock(&aux_lock);

	return ret;
}

s32 edp_hal_aux_read(u32 sel, s32 addr, s32 len, char *buf, bool retry)
{
	return edp_aux_read(sel, addr, len, buf, retry);
}

s32 edp_hal_aux_write(u32 sel, s32 addr, s32 len, char *buf, bool retry)
{
	return edp_aux_write(sel, addr, len, buf, retry);
}

s32 edp_hal_aux_i2c_read(u32 sel, s32 addr, s32 len, char *buf, bool retry)
{
	return edp_aux_i2c_read(sel, addr, len, buf, retry);
}

s32 edp_hal_aux_i2c_write(u32 sel, s32 addr, s32 len, char *buf, bool retry)
{
	return edp_aux_i2c_write(sel, addr, len, buf, retry);
}

s32 edp_hal_aux_read_ext(u32 sel, s32 addr, s32 len, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;
	while (retry_cnt < 7) {
		ret = edp_hal_aux_read(sel, addr, len, buf, retry_cnt ? true : false);
		/*
		 * for CTS 4.2.1.1, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP
		 */
		if ((ret != RET_AUX_NACK) &&
		    (ret != RET_AUX_TIMEOUT) &&
		    (ret != RET_AUX_DEFER) &&
		    (ret != RET_AUX_NO_STOP))
			break;
		/* at least 400us between two request is request in dp cts */
		usleep_range(500, 550);
		retry_cnt++;
	}

	return ret;
}

s32 edp_hal_aux_i2c_read_ext(u32 sel, s32 addr, s32 len, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;
	while (retry_cnt < 7) {
		ret = edp_hal_aux_i2c_read(sel, addr, len, buf, retry_cnt ? true : false);
		/*
		 * for CTS 4.2.1.1, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP
		 */
		if ((ret != RET_AUX_NACK) &&
		    (ret != RET_AUX_TIMEOUT) &&
		    (ret != RET_AUX_DEFER) &&
		    (ret != RET_AUX_NO_STOP))
			break;
		/* at least 400us between two request is request in dp cts */
		usleep_range(500, 550);
		retry_cnt++;
	}

	return ret;
}

s32 edp_hal_aux_i2c_write_ext(u32 sel, s32 addr, s32 len, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;
	while (retry_cnt < 7) {
		ret = edp_hal_aux_i2c_write(sel, addr, len, buf, retry_cnt ? true : false);
		/*
		 * for CTS 4.2.1.1, add retry when AUX_NACK, AUX_DEFER,
		 * AUX_TIMEOUT, AUX_NO_STOP
		 */
		if ((ret != RET_AUX_NACK) &&
		    (ret != RET_AUX_TIMEOUT) &&
		    (ret != RET_AUX_DEFER) &&
		    (ret != RET_AUX_NO_STOP))
			break;
		/* at least 400us between two request is request in dp cts */
		usleep_range(500, 550);
		retry_cnt++;
	}

	return ret;
}

s32 edp_transfer_unit_config(u32 sel, u32 bpp, u32 lane_cnt, u64 bit_rate, u32 pixel_clk)
{
	u32 reg_val;
	u32 pack_data_rate;
	u32 valid_symbol;
	u32 hblank;
	u32 bandwidth;
	u32 pre_div = 1000;

	reg_val = readl(edp_base[sel] + REG_EDP_HACTIVE_BLANK);
	hblank = GET_BITS(2, 14, reg_val);

	/*
	 * avg valid syobol per TU: pack_data_rate / bandwidth * LS_PER_TU
	 * pack_data_rate = (bpp / 8bit) * pix_clk / lane_cnt (1 symbol is 8 bit)
	 */

	pixel_clk = pixel_clk / 1000;
	bandwidth = bit_rate / 10000000;

	pack_data_rate = (bpp * pixel_clk / 8) / lane_cnt;
	EDP_LOW_DBG("[edp_transfer_unit]: pack_data_rate:%d\n", pack_data_rate);
	valid_symbol = LS_PER_TU * (pack_data_rate / bandwidth);

	if (valid_symbol > (62 * pre_div)) {
		EDP_ERR("valid symbol now: %d, should less than 62.\n", (valid_symbol / pre_div));
		EDP_ERR("Try to enlarge lane count or lane rate!\n");
		return RET_FAIL;
	}

	EDP_LOW_DBG("[edp_transfer_unit]: bpp:%d valid_symbol:%d\n", bpp, valid_symbol);

	reg_val = readl(edp_base[sel] + REG_EDP_FRAME_UNIT);
	reg_val = SET_BITS(0, 7, reg_val, valid_symbol / pre_div);
	reg_val = SET_BITS(16, 4, reg_val, (valid_symbol % pre_div) / 100);

	if ((valid_symbol / pre_div) < 6)
		reg_val = SET_BITS(7, 7, reg_val, 32);
	else {
		if (hblank < 80)
			reg_val = SET_BITS(7, 7, reg_val, 12);
		else
			reg_val = SET_BITS(7, 7, reg_val, 16);
	}
	writel(reg_val, edp_base[sel] + REG_EDP_FRAME_UNIT);

	return RET_OK;
}


void edp_set_link_clk_cyc(u32 sel, u32 lane_cnt, u64 bit_rate, u32 pixel_clk)
{
	u32 reg_val;
	u32 hblank;
	u32 symbol_clk;
	u32 link_cyc;

	/*hblank_link_cyc = hblank * (symbol_clk / 4) / pixclk*/
	reg_val = readl(edp_base[sel] + REG_EDP_HACTIVE_BLANK);
	hblank = GET_BITS(2, 14, reg_val);

	symbol_clk = bit_rate / 10000000;


	link_cyc = 1000 * hblank * (symbol_clk / lane_cnt) / (pixel_clk / 1000);
	EDP_LOW_DBG("link_cyc:%d hblank:%d symbol_clk:%d pixel_clk:%d\n", link_cyc, hblank, symbol_clk, pixel_clk);

	reg_val = readl(edp_base[sel] + REG_EDP_HBLANK_LINK_CYC);
	reg_val = SET_BITS(0, 16, reg_val, link_cyc);
	writel(reg_val, edp_base[sel] + REG_EDP_HBLANK_LINK_CYC);
}

s32 edp_hal_init_early(u32 sel)
{
	mutex_init(&aux_lock);

	return RET_OK;
}

s32 edp_hal_phy_init(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = 0;

	/* reserved for debug */
	//ret = edp_bist_test(sel, edp_core);
	if (ret < 0)
		return ret;
	edp_phy_soft_reset(sel);
	edp_hpd_enable(sel);
	edp_mode_init(sel, 1);
	edp_resistance_init(sel);

	/* use 2.7G bitrate to configurate, ensure aux can be accessed
	 * to get sink capability such as timing and lane parameter
	 */
	edp_aux_16m_config(sel, BIT_RATE_2G7);
	edp_corepll_config(sel, BIT_RATE_2G7);
	usleep_range(500, 1000);

	return ret;
}

void edp_hal_set_reg_base(u32 sel, uintptr_t base)
{
	edp_base[sel] = (void __iomem *)(base);
}


s32 edp_hal_enable(u32 sel, struct edp_tx_core *edp_core)
{
	u64 bit_rate;

	bit_rate = edp_core->lane_para.bit_rate;
	if (bit_rate == g_bit_rate)
		return RET_OK;

	edp_aux_16m_config(sel, bit_rate);
	edp_corepll_config(sel, bit_rate);
	usleep_range(500, 1000);

	return RET_OK;
}

s32 edp_hal_disable(u32 sel, struct edp_tx_core *edp_core)
{
	edp_video_stream_disable(sel);
	edp_main_link_reset(sel);

	return 0;
}

u64 edp_hal_get_max_rate(u32 sel)
{
	return BIT_RATE_2G7;
}

u32 edp_hal_get_max_lane(u32 sel)
{
	return 4;
}

bool edp_hal_support_tps3(u32 sel)
{
	return false;
}

bool edp_hal_support_fast_train(u32 sel)
{
	return false;
}

bool edp_hal_support_audio(u32 sel)
{
	return true;
}

bool edp_hal_support_psr(u32 sel)
{
	return false;
}

bool edp_hal_support_assr(u32 sel)
{
	return true;
}

bool edp_hal_support_ssc(u32 sel)
{
	return true;
}

bool edp_hal_support_mst(u32 sel)
{
	return false;
}

s32 edp_read_edid(u32 sel, struct edid *edid)
{
	s32 i;
	s32 ret;

	for (i = 0; i < (EDID_LENGTH / 16); i++) {
		ret = edp_hal_aux_i2c_read_ext(sel, EDID_ADDR, 16, (char *)(edid) + (i * 16));
		if (ret < 0)
			return ret;

	}
	for (i = 0; i < 128; i++)
		EDP_EDID_DBG("edid[%d] = 0x%x\n", i, *((char *)(edid) + i));

	return 0;
}

s32 edp_hal_read_edid(u32 sel, struct edid *edid)
{
	char g_tx_buf[16];

	memset(g_tx_buf, 0, sizeof(g_tx_buf));

	edp_hal_aux_i2c_write_ext(sel, EDID_ADDR, 1, &g_tx_buf[0]);

	return edp_read_edid(sel, edid);
}

s32 edp_hal_read_edid_ext(u32 sel, u32 block_cnt, struct edid *edid)
{
	s32 i, j, ret;
	struct edid *edid_ext;

	for (i = 1; i < block_cnt; i++) {
		edid_ext = edid + i;
		ret = edp_read_edid(sel, edid_ext);
		if (ret < 0)
			return ret;

		for (j = 0; j < 128; j++) {
			EDP_EDID_DBG("edid_ext[%d] = 0x%x\n", j, *((char *)(edid_ext) + j));
		}
	}

	return 0;
}

void edp_set_video_timings(u32 sel, struct disp_video_timings *timings)
{
	u32 reg_val;

	/*hsync/vsync polarity setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_SYNC_POLARITY);
	reg_val = SET_BITS(1, 1, reg_val, timings->hor_sync_polarity);
	reg_val = SET_BITS(0, 1, reg_val, timings->ver_sync_polarity);
	writel(reg_val, edp_base[sel] + REG_EDP_SYNC_POLARITY);

	/*h/vactive h/vblank setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_HACTIVE_BLANK);
	reg_val = SET_BITS(16, 16, reg_val, timings->x_res);
	reg_val = SET_BITS(2, 14, reg_val, (timings->hor_total_time - timings->x_res));
	writel(reg_val, edp_base[sel] + REG_EDP_HACTIVE_BLANK);

	reg_val = readl(edp_base[sel] + REG_EDP_VACTIVE_BLANK);
	reg_val = SET_BITS(0, 16, reg_val, timings->y_res);
	reg_val = SET_BITS(16, 16, reg_val, (timings->ver_total_time - timings->y_res));
	writel(reg_val, edp_base[sel] + REG_EDP_VACTIVE_BLANK);

	/*h/vstart setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_SYNC_START);
	reg_val = SET_BITS(0, 16, reg_val, (timings->hor_sync_time + timings->hor_back_porch));
	reg_val = SET_BITS(16, 16, reg_val, (timings->ver_sync_time + timings->ver_back_porch));
	writel(reg_val, edp_base[sel] + REG_EDP_SYNC_START);

	/*hs/vswidth  h/v_front_porch setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_HSW_FRONT_PORCH);
	reg_val = SET_BITS(16, 16, reg_val, timings->hor_sync_time);
	reg_val = SET_BITS(0, 16, reg_val, timings->hor_front_porch);
	writel(reg_val, edp_base[sel] + REG_EDP_HSW_FRONT_PORCH);

	reg_val = readl(edp_base[sel] + REG_EDP_VSW_FRONT_PORCH);
	reg_val = SET_BITS(16, 16, reg_val, timings->ver_sync_time);
	reg_val = SET_BITS(0, 16, reg_val, timings->ver_front_porch);
	writel(reg_val, edp_base[sel] + REG_EDP_VSW_FRONT_PORCH);
}

s32 edp_hal_set_video_format(u32 sel, struct edp_tx_core *edp_core)
{
	u32 colordepth;
	u32 color_fmt;
	u32 video_map;

	colordepth = edp_core->lane_para.colordepth;
	color_fmt = edp_core->lane_para.color_fmt;

	/* 0:RGB  1:YUV444  2:YUV422*/
	if (color_fmt == 0) {
		switch (colordepth) {
		case 6:
			video_map = RGB_6BIT;
			break;
		case 10:
			video_map = RGB_10BIT;
			break;
		case 12:
			video_map = RGB_12BIT;
			break;
		case 16:
			video_map = RGB_16BIT;
			break;
		case 8:
		default:
			video_map = RGB_8BIT;
			break;
		}
	} else if (color_fmt == 1) {
		switch (colordepth) {
		case 10:
			video_map = YCBCR444_10BIT;
			break;
		case 12:
			video_map = YCBCR444_12BIT;
			break;
		case 16:
			video_map = YCBCR444_16BIT;
			break;
		case 8:
		default:
			video_map = YCBCR444_8BIT;
			break;
		}
	} else if (color_fmt == 2) {
		switch (colordepth) {
		case 10:
			video_map = YCBCR422_10BIT;
			break;
		case 12:
			video_map = YCBCR422_12BIT;
			break;
		case 16:
			video_map = YCBCR422_16BIT;
			break;
		case 8:
		default:
			video_map = YCBCR422_8BIT;
			break;
		}
	} else {
		EDP_ERR("color format is not support!");
		return RET_FAIL;
	}

	edp_set_input_video_mapping(sel, (enum edp_video_mapping_e) video_map);

	return RET_OK;

}

s32 edp_hal_set_video_timings(u32 sel, struct disp_video_timings *tmgs)
{
	u32 pixel_clk = tmgs->pixel_clk;
	s32 ret = 0;

	edp_set_video_timings(sel, tmgs);
	ret = edp_pixpll_cfg(sel, pixel_clk);
	if (ret) {
		EDP_ERR("pixclk pll param calculate error!\n");
		return RET_FAIL;
	}

	return RET_OK;
}

s32 edp_hal_set_transfer_config(u32 sel, struct edp_tx_core *edp_core)
{
	struct disp_video_timings *tmgs = &edp_core->timings;
	u32 pixel_clk = tmgs->pixel_clk;
	u64 bit_rate = edp_core->lane_para.bit_rate;
	u32 lane_cnt = edp_core->lane_para.lane_cnt;
	u32 bpp = edp_core->lane_para.bpp;
	s32 ret;

	ret = edp_transfer_unit_config(sel, bpp, lane_cnt, bit_rate, pixel_clk);
	if (ret)
		return ret;

	edp_set_link_clk_cyc(sel, lane_cnt, bit_rate, pixel_clk);

	return RET_OK;
}

s32 edp_hal_audio_enable(u32 sel)
{
	edp_audio_timestamp_hblank_setting(sel, true);
	edp_audio_timestamp_vblank_setting(sel, true);
	edp_audio_stream_hblank_setting(sel, true);
	edp_audio_stream_vblank_setting(sel, true);
	edp_audio_soft_reset(sel);

	return RET_OK;
}

s32 edp_hal_audio_disable(u32 sel)
{
	edp_audio_timestamp_hblank_setting(sel, false);
	edp_audio_timestamp_vblank_setting(sel, false);
	edp_audio_stream_hblank_setting(sel, false);
	edp_audio_stream_vblank_setting(sel, false);

	return RET_OK;
}


s32 edp_hal_audio_set_para(u32 sel, edp_audio_t *para)
{
	edp_audio_interface_config(sel, para->interface);
	edp_audio_channel_config(sel, para->chn_cnt);
	edp_audio_data_width_config(sel, para->data_width);
	edp_audio_mute_config(sel, para->mute);

	return RET_OK;
}

s32 edp_hal_ssc_enable(u32 sel, bool enable)
{
	u32 reg_val;
	u32 index;

	if (g_bit_rate == BIT_RATE_1G62)
		index = 0;
	else
		index = 1;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	if (enable) {
		reg_val = SET_BITS(4, 2, reg_val, 0);
		reg_val = SET_BITS(21, 1, reg_val, 0);
	} else {
		reg_val = SET_BITS(4, 2, reg_val, recom_corepll[index].frac_pd);
		reg_val = SET_BITS(21, 1, reg_val, 1);
	}
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	return RET_OK;
}

bool edp_hal_ssc_is_enabled(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = GET_BITS(21, 1, reg_val);

	if (!reg_val)
		return true;

	return false;
}

s32 edp_hal_ssc_get_mode(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = GET_BITS(20, 1, reg_val);

	return reg_val;
}

s32 edp_hal_ssc_set_mode(u32 sel, u32 mode)
{
	u32 reg_val;
	u32 reg_val1;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val1 = readl(edp_base[sel] + REG_EDP_ANA_PLL_POSDIV);

	switch (mode) {
	case SSC_CENTER_MODE:
		reg_val = SET_BITS(20, 1, reg_val, 0);
		break;
	case SSC_DOWNSPR_MODE:
	default:
		reg_val = SET_BITS(20, 1, reg_val, 1);
		break;
	}

	reg_val1 = SET_BITS(24, 4, reg_val1, 3);
	reg_val1 = SET_BITS(28, 3, reg_val1, 4);

	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	writel(reg_val1, edp_base[sel] + REG_EDP_ANA_PLL_POSDIV);

	return RET_OK;
}


s32 edp_hal_psr_enable(u32 sel, bool enable)
{
	EDP_ERR("psr isn't support\n");
	return RET_FAIL;
}

bool edp_hal_psr_is_enabled(u32 sel)
{
	EDP_ERR("psr isn't support\n");
	return false;
}

s32 edp_hal_assr_enable(u32 sel, bool enable)
{
	u32 reg_val;

	if (enable) {
		reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
		reg_val = SET_BITS(24, 1, reg_val, 1);
		writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	} else {
		reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
		reg_val = SET_BITS(24, 1, reg_val, 0);
		writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	}

	return RET_OK;
}

s32 edp_hal_set_pattern(u32 sel, u32 pattern)
{
	if (pattern > 6)
		pattern = 6;

	switch (pattern) {
	case PRBS7:
		edp_hal_set_training_pattern(sel, 6);
		break;
	default:
		edp_hal_set_training_pattern(sel, pattern);
		break;
	}
	return RET_OK;
}

s32 edp_hal_get_color_fmt(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);

	reg_val = GET_BITS(16, 5, reg_val);

	switch (reg_val) {
	case 0:
		return RGB_6BIT;
	case 1:
		return RGB_8BIT;
	case 2:
		return RGB_10BIT;
	case 3:
		return RGB_12BIT;
	case 4:
		return RGB_16BIT;
	case 5:
		return YCBCR444_8BIT;
	case 6:
		return YCBCR444_10BIT;
	case 7:
		return YCBCR444_12BIT;
	case 8:
		return YCBCR444_16BIT;
	case 9:
		return YCBCR422_8BIT;
	case 10:
		return YCBCR422_10BIT;
	case 11:
		return YCBCR422_12BIT;
	case 12:
		return YCBCR422_16BIT;
	}

	return RET_FAIL;
}


s32 edp_hal_get_pixclk(u32 sel)
{
	u32 reg_val;
	u32 fb_div;
	u32 pre_div;
	u32 pll_divb;
	u32 pll_divc;
	u32 pixclk;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	fb_div = GET_BITS(24, 8, reg_val);
	pre_div = GET_BITS(8, 6, reg_val);

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_DIV);
	pll_divc = GET_BITS(16, 5, reg_val);
	reg_val = GET_BITS(8, 2, reg_val);
	if (reg_val == 1)
		pll_divb = 2;
	else if (reg_val == 2)
		pll_divb = 3;
	else if (reg_val == 3)
		pll_divb = 5;
	else
		pll_divb = 1;

	pixclk = (24 * fb_div) / pre_div;
	pixclk = pixclk / (2 * pll_divb * pll_divc);

	return pixclk * 1000000;
}

s32 edp_hal_get_train_pattern(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);

	reg_val = GET_BITS(0, 4, reg_val);

	return reg_val;
}

s32 edp_hal_get_lane_para(u32 sel, struct edp_lane_para *tmp_lane_para)
{
	u32 reg_val;
	u32 regval;

	/* bit rate */
	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	regval = GET_BITS(26, 3, reg_val);
	if (regval == 1)
		tmp_lane_para->bit_rate = 2160000000;
	else if (regval == 2)
		tmp_lane_para->bit_rate = 2430000000;
	else {
		regval = GET_BITS(4, 2, reg_val);
		if (regval == 0)
			tmp_lane_para->bit_rate = 1620000000;
		else if (regval == 1)
			tmp_lane_para->bit_rate = 2700000000;
		else
			tmp_lane_para->bit_rate = 0;
	}

	/* lane count */
	regval = GET_BITS(6, 2, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_cnt = 1;
	else if (regval == 1)
		tmp_lane_para->lane_cnt = 2;
	else if (regval == 2)
		tmp_lane_para->lane_cnt = 4;
	else
		tmp_lane_para->lane_cnt = 0;

	/*sw*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
	regval = GET_BITS(0, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane0_sw = 0;
	if (regval == 2)
		tmp_lane_para->lane0_sw = 1;
	if (regval == 4)
		tmp_lane_para->lane0_sw = 2;
	if (regval == 6)
		tmp_lane_para->lane0_sw = 3;

	regval = GET_BITS(4, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane1_sw = 0;
	if (regval == 2)
		tmp_lane_para->lane1_sw = 1;
	if (regval == 4)
		tmp_lane_para->lane1_sw = 2;
	if (regval == 6)
		tmp_lane_para->lane1_sw = 3;

	reg_val = readl(edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
	regval = GET_BITS(24, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane2_sw = 0;
	if (regval == 2)
		tmp_lane_para->lane2_sw = 1;
	if (regval == 4)
		tmp_lane_para->lane2_sw = 2;
	if (regval == 6)
		tmp_lane_para->lane2_sw = 3;

	regval = GET_BITS(28, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane3_sw = 0;
	if (regval == 2)
		tmp_lane_para->lane3_sw = 1;
	if (regval == 4)
		tmp_lane_para->lane3_sw = 2;
	if (regval == 6)
		tmp_lane_para->lane3_sw = 3;


	/*pre*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
	regval = GET_BITS(24, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane0_pre = 0;
	if (regval == 1)
		tmp_lane_para->lane0_pre = 1;
	if (regval == 2)
		tmp_lane_para->lane0_pre = 2;
	if (regval == 3)
		tmp_lane_para->lane0_pre = 3;

	regval = GET_BITS(28, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane1_pre = 0;
	if (regval == 1)
		tmp_lane_para->lane1_pre = 1;
	if (regval == 2)
		tmp_lane_para->lane1_pre = 2;
	if (regval == 3)
		tmp_lane_para->lane1_pre = 3;

	regval = GET_BITS(16, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane2_pre = 0;
	if (regval == 1)
		tmp_lane_para->lane2_pre = 1;
	if (regval == 2)
		tmp_lane_para->lane2_pre = 2;
	if (regval == 3)
		tmp_lane_para->lane2_pre = 3;

	regval = GET_BITS(20, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane3_pre = 0;
	if (regval == 1)
		tmp_lane_para->lane3_pre = 1;
	if (regval == 2)
		tmp_lane_para->lane3_pre = 2;
	if (regval == 3)
		tmp_lane_para->lane3_pre = 3;

	return RET_OK;
}

s32 edp_hal_get_tu_size(u32 sel)
{
	return LS_PER_TU;
}

s32 edp_hal_get_valid_symbol_per_tu(u32 sel)
{
	u32 reg_val;
	u32 regval;
	u32 valid_symbol;

	reg_val = readl(edp_base[sel] + REG_EDP_FRAME_UNIT);
	regval = GET_BITS(0, 7, reg_val);
	valid_symbol = regval * 10;

	regval = GET_BITS(16, 4, reg_val);
	valid_symbol += regval;

	return valid_symbol;
}

bool edp_hal_audio_is_enabled(u32 sel)
{
	u32 reg_val;
	u32 regval0;
	u32 regval1;
	u32 regval2;
	u32 regval3;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
	regval0 = GET_BITS(0, 1, reg_val);
	regval1 = GET_BITS(1, 1, reg_val);

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
	regval2 = GET_BITS(0, 1, reg_val);
	regval3 = GET_BITS(1, 1, reg_val);

	if (regval0 && regval1 && regval2 && regval3)
		return true;

	return false;
}

s32 edp_hal_get_audio_if(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = GET_BITS(0, 1, reg_val);

	return reg_val;
}

s32 edp_hal_audio_is_mute(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = GET_BITS(15, 1, reg_val);

	return reg_val;
}

s32 edp_hal_get_audio_chn_cnt(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = GET_BITS(12, 3, reg_val);

	if (reg_val == 0)
		return 1;
	else if (reg_val == 1)
		return 2;
	else
		return 8;
}

s32 edp_hal_get_audio_date_width(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = GET_BITS(5, 5, reg_val);

	if (reg_val == 0x10)
		return 16;
	else if (reg_val == 0x14)
		return 20;
	else
		return 24;
}

s32 edp_hal_get_cur_line(u32 sel)
{
	return 0;
}

s32 edp_hal_get_start_dly(u32 sel)
{
	return 0;
}

void edp_hal_show_builtin_patten(u32 sel, u32 pattern)
{
}


void edp_hal_clean_hpd_status(u32 sel)
{
	writel((1 << 1), edp_base[sel] + REG_EDP_HPD_PLUG);
}


s32 edp_hal_link_start(u32 sel)
{
	edp_video_stream_enable(sel);

	return RET_OK;
}

s32 edp_hal_link_stop(u32 sel)
{
	edp_video_stream_disable(sel);
	edp_main_link_reset(sel);

	return RET_OK;
}

s32 edp_hal_query_transfer_unit(u32 sel, struct edp_tx_core *edp_core,
				struct disp_video_timings *tmgs)
{
	u32 pack_data_rate;
	u32 valid_symbol;
	u32 pixel_clk;
	u32 bandwidth;
	u32 pre_div = 1000;
	u64 bit_rate = edp_core->lane_para.bit_rate;
	u32 lane_cnt =  edp_core->lane_para.lane_cnt;
	//fixme
	//fix me if color_fmt and colordepth can be configured
	u32 bpp = edp_core->lane_para.bpp;

	/*
	 * avg valid syobol per TU: pack_data_rate / bandwidth * LS_PER_TU
	 * pack_data_rate = (bpp / 8bit) * pix_clk / lane_cnt (1 symbol is 8 bit)
	 */

	pixel_clk = tmgs->pixel_clk;
	pixel_clk = pixel_clk / 1000;

	bandwidth = bit_rate / 10000000;

	if ((bit_rate == 0) || (lane_cnt == 0) || (pixel_clk == 0)) {
		EDP_ERR("edp param is zero, mode not support! pixclk:%d, bitrate:%lld lane_cnt:%d\n",
			pixel_clk * 1000, bit_rate, lane_cnt);
		return RET_FAIL;
	}

	pack_data_rate = (bpp * pixel_clk / 8) / lane_cnt;
	valid_symbol = LS_PER_TU * (pack_data_rate / bandwidth);

	if (valid_symbol > (62 * pre_div)) {
		EDP_ERR("out of valid symbol limit(lane:%d bit_rate:%lld pixel_clk:%d symbol_limit:62 symbol_now:%d\n",
				lane_cnt, bit_rate, pixel_clk, valid_symbol / 1000);
		EDP_ERR("check if lane_cnt or lane_rate can be enlarged!\n");

		return RET_FAIL;
	}

	return RET_OK;
}

/* define 3 different voltage level param type
 * 0: low voltage level, for normal edp panel
 * 1: high voltage level, for dp display that may follow with voltage attenuation
 * 2: width scope voltage, cover some low and high voltahe
 */
void edp_hal_set_lane_sw_pre(u32 sel, u32 lane_id, u32 sw, u32 pre, u32 param_type)
{
	u32 sw_lv = training_param_table[param_type][sw][pre].sw_lv;
	u32 pre_lv = training_param_table[param_type][sw][pre].pre_lv;
	u32 reg_val;

	if (lane_id == 0) {
		/* lane0 swing voltage level*/
		reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
		reg_val = SET_BITS(0, 4, reg_val, sw_lv);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_MAINSEL);

		/* lane0 pre emphasis level */
		reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
		reg_val = SET_BITS(24, 4, reg_val, pre_lv);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);
	} else if (lane_id == 1) {
		/* lane1 swing voltage level*/
		reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
		reg_val = SET_BITS(4, 4, reg_val, sw_lv);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_MAINSEL);

		/* lane1 pre emphasis level */
		reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
		reg_val = SET_BITS(28, 4, reg_val, pre_lv);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);
	} else if (lane_id == 2) {
		/* lane2 swing voltage level*/
		reg_val = readl(edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
		reg_val = SET_BITS(24, 4, reg_val, sw_lv);
		writel(reg_val, edp_base[sel] + REG_EDP_TX32_ISEL_DRV);

		/* lane2 pre emphasis level */
		reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
		reg_val = SET_BITS(16, 4, reg_val, pre_lv);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);
	} else if (lane_id == 3) {
		/* lane3 swing voltage level*/
		reg_val = readl(edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
		reg_val = SET_BITS(28, 4, reg_val, sw_lv);
		writel(reg_val, edp_base[sel] + REG_EDP_TX32_ISEL_DRV);

		/* lane3 pre emphasis level */
		reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
		reg_val = SET_BITS(20, 4, reg_val, pre_lv);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);
	} else
		EDP_WRN("%s: lane number is not support!\n", __func__);
}

void edp_hal_set_lane_rate(u32 sel, u64 bit_rate)
{
	u32 reg_val;

	/*config lane bit rate*/
	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	switch (bit_rate) {
	case BIT_RATE_1G62:
		reg_val = SET_BITS(26, 3, reg_val, 0x0);
		reg_val = SET_BITS(4, 2, reg_val, 0x0);
		break;
	case BIT_RATE_2G7:
	default:
		reg_val = SET_BITS(26, 3, reg_val, 0x0);
		reg_val = SET_BITS(4, 2, reg_val, 0x1);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_CAPACITY);
}

void edp_hal_set_lane_cnt(u32 sel, u32 lane_cnt)
{
	u32 reg_val;

	if ((lane_cnt < 0) || (lane_cnt > 4)) {
		EDP_WRN("unsupport lane number!\n");
	}

	/*config lane number*/
	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	switch (lane_cnt) {
	case 0:
	case 3:
		EDP_WRN("edp lane number can not be configed to 0/3!\n");
		break;
	case 1:
		reg_val = SET_BITS(6, 2, reg_val, 0x0);
		reg_val = SET_BITS(8, 4, reg_val, 0x01);
		break;
	case 2:
		reg_val = SET_BITS(6, 2, reg_val, 0x1);
		reg_val = SET_BITS(8, 4, reg_val, 0x3);
		break;
	case 4:
		reg_val = SET_BITS(6, 2, reg_val, 0x2);
		reg_val = SET_BITS(8, 4, reg_val, 0xf);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_CAPACITY);
}

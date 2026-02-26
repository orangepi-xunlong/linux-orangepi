/* SPDX-License-Identifier: GPL-2.0-or-later */
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

#include "../../../sunxi_edp.h"
#include "inno_edp13.h"
//#include "../edp_lowlevel.h"
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

/*link symbol per TU*/
#define LS_PER_TU 64

/* REG88[BIT0]: set 1 when a narrow hpd plus between 0.25s-2ms in hpd pin*/
/* REG1A4[BIT16-BIT23]: default 0xff, value would change if ESD overvoltage occurs */
/* we can use these bits to judge if ESD overvoltage happen */
#define REG_HPD_NARROW_PLUSE_DEF 0x0
#define REG_ESD_DEF 0xff

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
static void edp_mode_init(struct sunxi_edp_hw_desc *edp_hw, u32 mode)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_TX_PRESEL);
	if (mode) {
		reg_val = SET_BITS(24, 4, reg_val, 0xf);
	} else {
		reg_val = SET_BITS(24, 4, reg_val, 0x0);
	}
	writel(reg_val, edp_hw->reg_base + REG_EDP_TX_PRESEL);
	reg_val = readl(edp_hw->reg_base + REG_EDP_AUX_FILTTER);
	reg_val = SET_BITS(14, 2, reg_val, 0x2);
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUX_FILTTER);
}

/*0:edp_mode   1:dp_mode*/
static void edp_controller_mode_init(struct sunxi_edp_hw_desc *edp_hw, u32 mode)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_HPD_SCALE);
	if (mode)
		/* dp_mode*/
		reg_val = SET_BITS(27, 1, reg_val, 0);
	else
		/* edp_mode*/
		reg_val = SET_BITS(27, 1, reg_val, 1);

	writel(reg_val, edp_hw->reg_base + REG_EDP_HPD_SCALE);
}


static void edp_resistance_init(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_RES1000_CFG);
	reg_val = SET_BITS(8, 1, reg_val, 0x1);
	reg_val = SET_BITS(0, 6, reg_val, 0x0);
	writel(reg_val, edp_hw->reg_base + REG_EDP_RES1000_CFG);
}

void edp_aux_16m_config(struct sunxi_edp_hw_desc *edp_hw, u64 bit_rate)
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

	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_AUX_CLOCK);
	div_16m = (bit_clock / 8) / 8;
	reg_val = SET_BITS(0, 5, reg_val, div_16m);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_AUX_CLOCK);

	/*set main isel*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_TX_MAINSEL);
	reg_val = SET_BITS(16, 5, reg_val, 0x14);
	reg_val = SET_BITS(24, 5, reg_val, 0x14);
	writel(reg_val, edp_hw->reg_base + REG_EDP_TX_MAINSEL);

	reg_val = readl(edp_hw->reg_base + REG_EDP_TX_POSTSEL);
	reg_val = SET_BITS(0, 5, reg_val, 0x14);
	reg_val = SET_BITS(8, 5, reg_val, 0x14);
	writel(reg_val, edp_hw->reg_base + REG_EDP_TX_POSTSEL);

	/*set pre isel*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_TX_PRESEL);
	reg_val = SET_BITS(0, 15, reg_val, 0x0);
	writel(reg_val, edp_hw->reg_base + REG_EDP_TX_PRESEL);

	/* set aux channel swing level to max, enhance compatibility */
	reg_val = readl(edp_hw->reg_base + REG_EDP_AUX_ISEL_MAINSET);
	/* AUX ISEL */
	reg_val = SET_BITS(4, 4, reg_val, 0xf);
	/* AUX MAINSET */
	reg_val = SET_BITS(8, 4, reg_val, 0xf);
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUX_ISEL_MAINSET);
}

void edp_corepll_config(struct sunxi_edp_hw_desc *edp_hw, u64 bit_rate)
{
	u32 reg_val;
	u32 index;

	if (bit_rate == BIT_RATE_1G62)
		index = 0;
	else
		index = 1;
	edp_hw->cur_bit_rate = bit_rate;

	/*turnoff corepll*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 1);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll prediv*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(8, 6, reg_val, recom_corepll[index].prediv);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll fbdiv*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(16, 4, reg_val, recom_corepll[index].fbdiv_h4);
	reg_val = SET_BITS(24, 8, reg_val, recom_corepll[index].fbdiv_l8);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll postdiv*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_POSDIV);
	reg_val = SET_BITS(2, 2, reg_val, recom_corepll[index].postdiv);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PLL_POSDIV);

	/*config corepll frac_pd*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(4, 2, reg_val, recom_corepll[index].frac_pd);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll frac*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FRAC);
	reg_val = SET_BITS(0, 8, reg_val, recom_corepll[index].frac_h8);
	reg_val = SET_BITS(8, 8, reg_val, recom_corepll[index].frac_m8);
	reg_val = SET_BITS(16, 8, reg_val, recom_corepll[index].frac_l8);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PLL_FRAC);

	/*turnon corepll*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 0);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
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

s32 edp_pixpll_cfg(struct sunxi_edp_hw_desc *edp_hw, u32 pixel_clk)
{
	u32 reg_val;
	s32 ret = 0;
	struct recommand_pixpll pixpll;

	memset(&pixpll, 0, sizeof(struct recommand_pixpll));

	ret = pixpll_cal(pixel_clk, &pixpll);
	if (ret)
		return ret;

	/*turnoff pixpll*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 1);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll prediv*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(8, 6, reg_val, pixpll.prediv);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll fbdiv*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(16, 4, reg_val, pixpll.fbdiv_h4);
	reg_val = SET_BITS(24, 8, reg_val, pixpll.fbdiv_l8);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll divabc*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_DIV);
	reg_val = SET_BITS(0, 5, reg_val, pixpll.plldiv_a);
	reg_val = SET_BITS(8, 2, reg_val, pixpll.plldiv_b);
	reg_val = SET_BITS(16, 5, reg_val, pixpll.plldiv_c);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PIXPLL_DIV);

	/*config pixpll frac_pd*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(4, 2, reg_val, pixpll.frac_pd);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll frac*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FRAC);
	reg_val = SET_BITS(0, 8, reg_val, pixpll.frac_h8);
	reg_val = SET_BITS(8, 8, reg_val, pixpll.frac_m8);
	reg_val = SET_BITS(16, 8, reg_val, pixpll.frac_l8);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FRAC);

	/*turnon pixpll*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 0);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);

	return RET_OK;
}

void edp_set_misc(struct sunxi_edp_hw_desc *edp_hw, u32 misc0_val, u32 misc1_val)
{
	u32 reg_val;

	/*misc0 setting*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_MSA_MISC0);
	reg_val = SET_BITS(24, 8, reg_val, misc0_val);
	writel(reg_val, edp_hw->reg_base + REG_EDP_MSA_MISC0);

	/*misc1 setting*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_MSA_MISC1);
	reg_val = SET_BITS(24, 8, reg_val, misc1_val);
	writel(reg_val, edp_hw->reg_base + REG_EDP_MSA_MISC1);
}

void edp_video_stream_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(5, 1, reg_val, 1);
	writel(reg_val, edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);
}

void edp_video_stream_disable(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(5, 1, reg_val, 0);
	writel(reg_val, edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);
}

void inno_set_training_pattern(struct sunxi_edp_hw_desc *edp_hw, u32 pattern)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_CAPACITY);
	reg_val = SET_BITS(0, 4, reg_val, pattern);
	writel(reg_val, edp_hw->reg_base + REG_EDP_CAPACITY);
}

void edp_audio_stream_vblank_setting(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO_VBLANK_EN);
	reg_val = SET_BITS(1, 1, reg_val, enable);
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUDIO_VBLANK_EN);
}

void edp_audio_timestamp_vblank_setting(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO_VBLANK_EN);
	reg_val = SET_BITS(0, 1, reg_val, enable);
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUDIO_VBLANK_EN);
}

void edp_audio_stream_hblank_setting(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO_HBLANK_EN);
	reg_val = SET_BITS(1, 1, reg_val, enable);
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUDIO_HBLANK_EN);
}

void edp_audio_timestamp_hblank_setting(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO_HBLANK_EN);
	reg_val = SET_BITS(0, 1, reg_val, enable);
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUDIO_HBLANK_EN);
}

void edp_audio_interface_config(struct sunxi_edp_hw_desc *edp_hw, u32 interface)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO);
	reg_val = SET_BITS(0, 1, reg_val, interface);
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUDIO);
}

void edp_audio_channel_config(struct sunxi_edp_hw_desc *edp_hw, u32 chn_num)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO);

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
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUDIO);
}

void edp_audio_mute_config(struct sunxi_edp_hw_desc *edp_hw, bool mute)
{

	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO);
	reg_val = SET_BITS(15, 1, reg_val, mute);
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUDIO);
}

void edp_audio_data_width_config(struct sunxi_edp_hw_desc *edp_hw, u32 data_width)
{

	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO);
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
	writel(reg_val, edp_hw->reg_base + REG_EDP_AUDIO);
}

void edp_audio_soft_reset(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_RESET);
	reg_val = SET_BITS(3, 1, reg_val, 1);
	writel(reg_val, edp_hw->reg_base + REG_EDP_RESET);
	reg_val = SET_BITS(3, 1, reg_val, 0);
	writel(reg_val, edp_hw->reg_base + REG_EDP_RESET);
}

void edp_set_input_video_mapping(struct sunxi_edp_hw_desc *edp_hw, enum edp_video_mapping_e mapping)
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
	default:
		mapping_val = 1;
		misc0_val = (1 << 5);
		break;
	}

	reg_val = readl(edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(16, 5, reg_val, mapping_val);
	writel(reg_val, edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);

	edp_set_misc(edp_hw, misc0_val, misc1_val);
}

s32 edp_bist_test(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	u32 reg_val;
	s32 ret;

	/*set bist_test_sel to 1*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_BIST_CFG);
	reg_val = SET_BITS(0, 1, reg_val, 1);
	writel(reg_val, edp_hw->reg_base + REG_EDP_BIST_CFG);

	/*assert reset pin*/
	//if (gpio_is_valid(edp_core->rst_gpio)) {
	//	gpio_direction_output(edp_core->rst_gpio, 0);
	//}

	/*set bist_test_en to 1*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_BIST_CFG);
	reg_val = SET_BITS(1, 1, reg_val, 1);
	writel(reg_val, edp_hw->reg_base + REG_EDP_BIST_CFG);

	usleep_range(5, 10);

	/*wait for bist_test_done to 1*/
	while (1) {
		reg_val = readl(edp_hw->reg_base + REG_EDP_BIST_CFG);
		reg_val = GET_BITS(4, 1, reg_val);
		if (reg_val == 1)
			break;
	}

	/*checke bist_test_done*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_BIST_CFG);
	reg_val = GET_BITS(5, 1, reg_val);
	if (reg_val == 1)
		ret = RET_OK;
	else
		ret = RET_FAIL;

	/*deassert reset pin and disable bist_test_sel to exit bist*/
	//if (gpio_is_valid(edp_core->rst_gpio)) {
	//	gpio_direction_output(edp_core->rst_gpio, 1);
	//}
	writel(0x0, edp_hw->reg_base + REG_EDP_BIST_CFG);

	return RET_OK;
}

void edp_controller_soft_reset(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_RESET);
	reg_val = SET_BITS(1, 1, reg_val, 1);
	writel(reg_val, edp_hw->reg_base + REG_EDP_RESET);
	usleep_range(5, 10);
	reg_val = SET_BITS(1, 1, reg_val, 0);
	writel(reg_val, edp_hw->reg_base + REG_EDP_RESET);
}

void edp_main_link_reset(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_CAPACITY);
	reg_val = SET_BITS(0, 12, reg_val, 0);
	reg_val = SET_BITS(26, 3, reg_val, 0);
	writel(reg_val, edp_hw->reg_base + REG_EDP_CAPACITY);
}

void edp_hpd_irq_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	writel(0x1, edp_hw->reg_base + REG_EDP_HPD_INT);
	writel(0x6, edp_hw->reg_base + REG_EDP_HPD_EN);
}

void edp_hpd_irq_disable(struct sunxi_edp_hw_desc *edp_hw)
{
	writel(0x0, edp_hw->reg_base + REG_EDP_HPD_INT);
	writel(0x0, edp_hw->reg_base + REG_EDP_HPD_EN);
}

void edp_hpd_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;
	u32 reg_val1;

	reg_val = readl(edp_hw->reg_base + REG_EDP_HPD_SCALE);
	reg_val = SET_BITS(3, 1, reg_val, 1);
	writel(reg_val, edp_hw->reg_base + REG_EDP_HPD_SCALE);

	/* only hpd enable need, irq is not necesssary*/
	reg_val1 = readl(edp_hw->reg_base + REG_EDP_HPD_EN);
	reg_val1 = SET_BITS(1, 2, reg_val1, 3);
	writel(reg_val1, edp_hw->reg_base + REG_EDP_HPD_EN);
	edp_hpd_irq_enable(edp_hw);
}

void edp_hpd_disable(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_HPD_SCALE);
	reg_val = SET_BITS(3, 1, reg_val, 0);
	writel(reg_val, edp_hw->reg_base + REG_EDP_HPD_SCALE);
	edp_hpd_irq_disable(edp_hw);
}

bool inno_get_hotplug_change(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 int_event_en = 0;
	u32 int_en = 0;
	u32 reg_val = 0;

	int_event_en = readl(edp_hw->reg_base + REG_EDP_HPD_INT);
	int_en = readl(edp_hw->reg_base + REG_EDP_HPD_EN);

	if ((int_event_en & (1 << 0)) &&
	    ((int_en & (1 << 1)) || (int_en & (1 << 2)))) {
		reg_val =  readl(edp_hw->reg_base + REG_EDP_HPD_PLUG) & ((1 << 1) | (1 << 2));
		return reg_val ? true : false;
	}

	return false;
}

s32 inno_get_hotplug_state(struct sunxi_edp_hw_desc *edp_hw)
{
	bool hpd_plugin;
	bool hpd_plugout;

	/* hpd plugin interrupt */
	hpd_plugin = readl(edp_hw->reg_base + REG_EDP_HPD_PLUG) & (1 << 1);
	if (hpd_plugin == true) {
		writel((1 << 1), edp_hw->reg_base + REG_EDP_HPD_PLUG);
		return 1;;
	}

	/* hpd plugout interrupt */
	hpd_plugout = readl(edp_hw->reg_base + REG_EDP_HPD_PLUG) & (1 << 2);
	if (hpd_plugout == true) {
		writel((1 << 2), edp_hw->reg_base + REG_EDP_HPD_PLUG);
		return 0;
	}

	return 0;
}


void inno_irq_handle(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
}

s32 inno_irq_enable(struct sunxi_edp_hw_desc *edp_hw, u32 irq_id)
{
	edp_hpd_irq_enable(edp_hw);
	return 0;
}

s32 inno_irq_disable(struct sunxi_edp_hw_desc *edp_hw, u32 irq_id)
{
	/*fixme: irq is not need?*/
	//edp_hpd_irq_disable();
	return 0;
}

s32 edp_aux_read(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf)
{
	u32 reg_val[4];
	u32 regval = 0;
	s32 i, j;
	u32 timeout = 0;
	s32 ret = 0;
	u32 block_len = 16;
	u32 ext = (len % block_len) ? 1 : 0;

	mutex_lock(&edp_hw->aux_lock);
	for (j = 0; j < (len / block_len + ext); j++) {
		writel(0, edp_hw->reg_base + REG_EDP_PHY_AUX);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA0);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA1);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA2);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA3);
		writel(0, edp_hw->reg_base + REG_EDP_PHY_AUX);
		regval = readl(edp_hw->reg_base + REG_EDP_HPD_EVENT);
		regval |= (1 << 1);
		writel(regval, edp_hw->reg_base + REG_EDP_HPD_EVENT);

		regval = 0;
		/* aux read request*/
		regval |= (block_len - 1);
		regval = SET_BITS(8, 20, regval, (addr + (j * block_len)));
		regval = SET_BITS(28, 4, regval, NATIVE_READ);
		edp_hw->cur_aux_request = regval;
		EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
		writel(regval, edp_hw->reg_base + REG_EDP_PHY_AUX);
		udelay(1);
		writel(1, edp_hw->reg_base + REG_EDP_AUX_START);

		/* wait aux reply event */
		/*
		while (!(readl(edp_hw->reg_base + REG_EDP_HPD_EVENT) & (1 << 1))) {
			if (timeout >= 50000) {
				EDP_LOW_DBG("edp_aux_read wait AUX_REPLY event timeout, request:0x%x\n",
					    edp_hw->cur_aux_request);
				ret = RET_AUX_TIMEOUT;
				goto CLR_EVENT;
			}
			timeout++;
		}
		*/
		/* wait for AUX_REPLY*/
		//fixme
		regval = readl(edp_hw->reg_base + REG_EDP_AUX_TIMEOUT);
		while (((readl(edp_hw->reg_base + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
			if (timeout >= 50000) {
				EDP_LOW_DBG("edp_aux_read wait AUX_REPLY timeout, request:0x%x\n",
					    edp_hw->cur_aux_request);
				ret = RET_AUX_TIMEOUT;
				goto CLR_EVENT;
			}
			timeout++;
		}

		regval = readl(edp_hw->reg_base + REG_EDP_AUX_TIMEOUT);
		/* not ensure, need confirm from inno */
		if (((regval >> 24) & 0xf) == 0xe) {
			EDP_LOW_DBG("edp_aux_read recieve without STOP, request:0x%x\n",
				    edp_hw->cur_aux_request);
			ret = RET_AUX_NO_STOP;
			goto CLR_EVENT;
		}
		regval &= 0xf0;
		if ((regval >> 4) == AUX_REPLY_NACK) {
			EDP_LOW_DBG("edp_aux_read recieve AUX_REPLY_NACK, request:0x%x\n",
				    edp_hw->cur_aux_request);
			ret = RET_AUX_NACK;
			goto CLR_EVENT;
		} else if ((regval >> 4) == AUX_REPLY_DEFER) {
			EDP_LOW_DBG("edp_aux_read recieve AUX_REPLY_DEFER, request:0x%x\n",
				    edp_hw->cur_aux_request);
			ret = RET_AUX_DEFER;
			goto CLR_EVENT;
		}

		/* aux read reply*/
		for (i = 0; i < 4; i++) {
			reg_val[i] = readl(edp_hw->reg_base + REG_EDP_AUX_DATA0 + i * 0x4);
			usleep_range(10, 20);
			EDP_LOW_DBG("[%s] result: reg_val[%d] = 0x%x\n", __func__, i, reg_val[i]);
		}

		for (i = 0; i < block_len; i++) {
			if ((block_len * j + i) <= len)
				buf[j * block_len + i] = GET_BITS((i % 4) * 8, 8, reg_val[i / 4]);
		}
	}

CLR_EVENT:
	regval = readl(edp_hw->reg_base + REG_EDP_HPD_EVENT);
	regval |= (1 << 1);
	writel(regval, edp_hw->reg_base + REG_EDP_HPD_EVENT);
	mutex_unlock(&edp_hw->aux_lock);

	return ret;
}

s32 edp_aux_write(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf)
{
	u32 reg_val[4];
	u32 regval = 0;
	u32 timeout = 0;
	s32 i, j;
	s32 ret = 0;
	u32 block_len = 16;
	u32 data_len;
	u32 ext = (len % block_len) ? 1 : 0;

	memset(reg_val, 0, sizeof(reg_val));

	mutex_lock(&edp_hw->aux_lock);
	for (j = 0; j < (len / block_len + ext); j++) {
		if (len <= 16)
			data_len = len;
		else if ((16 * j) < (len - 16))
			data_len = 16;
		else
			/* this is the last one transmit */
			data_len = len - 16;

		writel(0, edp_hw->reg_base + REG_EDP_PHY_AUX);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA0);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA1);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA2);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA3);
		writel(0, edp_hw->reg_base + REG_EDP_PHY_AUX);
		regval = readl(edp_hw->reg_base + REG_EDP_HPD_EVENT);
		regval |= (1 << 1);
		writel(regval, edp_hw->reg_base + REG_EDP_HPD_EVENT);

		for (i = 0; i < data_len; i++) {
			reg_val[i / 4] = SET_BITS((i % 4) * 8, 8, reg_val[i / 4], buf[j * block_len + i]);
		}

		for (i = 0; i < (1 + ((data_len - 1) / 4)); i++) {
			writel(reg_val[i], edp_hw->reg_base + REG_EDP_AUX_DATA0 + (i * 0x4));
			usleep_range(10, 20);
			EDP_LOW_DBG("[%s]: date: reg_val[%d] = 0x%x", __func__, i, reg_val[i]);
		}

		/* aux write request*/
		regval = 0;
		regval |= (data_len - 1);
		regval = SET_BITS(8, 20, regval, (addr + (j * block_len)));
		regval = SET_BITS(28, 4, regval, NATIVE_WRITE);
		edp_hw->cur_aux_request = regval;
		EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
		writel(regval, edp_hw->reg_base + REG_EDP_PHY_AUX);

		udelay(1);
		writel(1, edp_hw->reg_base + REG_EDP_AUX_START);

		/* wait aux reply event */
		/*
		while (!(readl(edp_hw->reg_base + REG_EDP_HPD_EVENT) & (1 << 1))) {
			if (timeout >= 50000) {
				EDP_LOW_DBG("edp_aux_write wait AUX_REPLY event timeout, request:0x%x\n",
					    edp_hw->cur_aux_request);
				ret = RET_AUX_TIMEOUT;
				goto CLR_EVENT;
			}
			timeout++;
		}
		*/

		/* wait for AUX_REPLY*/
		while (((readl(edp_hw->reg_base + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
			if (timeout >= 50000) {
				EDP_LOW_DBG("edp_aux_write wait AUX_REPLY timeout, request:0x%x\n",
					    edp_hw->cur_aux_request);
				ret = RET_AUX_TIMEOUT;
				goto CLR_EVENT;
			}
			timeout++;
		}

		regval = readl(edp_hw->reg_base + REG_EDP_AUX_TIMEOUT);
		/* not ensure, need confirm from inno */
		if (((regval >> 24) & 0xf) == 0xe) {
			EDP_LOW_DBG("edp_aux_read recieve without STOP, request:0x%x\n",
				    edp_hw->cur_aux_request);
			ret = RET_AUX_NO_STOP;
			goto CLR_EVENT;
		}
		regval &= 0xf0;
		if ((regval >> 4) == AUX_REPLY_NACK) {
			EDP_LOW_DBG("edp_aux_write recieve AUX_NACK, request:0x%x\n",
				    edp_hw->cur_aux_request);
			ret = RET_AUX_NACK;
			goto CLR_EVENT;
		} else if ((regval >> 4) == AUX_REPLY_DEFER) {
			EDP_LOW_DBG("edp_aux_write recieve AUX_REPLY_DEFER, request:0x%x\n",
				    edp_hw->cur_aux_request);
			ret = RET_AUX_DEFER;
			goto CLR_EVENT;
		}
	}

CLR_EVENT:
	regval = readl(edp_hw->reg_base + REG_EDP_HPD_EVENT);
	regval |= (1 << 1);
	writel(regval, edp_hw->reg_base + REG_EDP_HPD_EVENT);
	mutex_unlock(&edp_hw->aux_lock);

	return ret;
}

s32 edp_aux_i2c_read(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf)
{
	u32 reg_val[4];
	u32 regval = 0;
	s32 i, j;
	u32 timeout = 0;
	s32 ret = 0;
	u32 block_len = 16;
	u32 ext = (len % block_len) ? 1 : 0;

	mutex_lock(&edp_hw->aux_lock);
	for (j = 0; j < (len / block_len + ext); j++) {
		writel(0, edp_hw->reg_base + REG_EDP_PHY_AUX);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA0);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA1);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA2);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA3);
		writel(0, edp_hw->reg_base + REG_EDP_PHY_AUX);
		regval = readl(edp_hw->reg_base + REG_EDP_HPD_EVENT);
		regval |= (1 << 1);
		writel(regval, edp_hw->reg_base + REG_EDP_HPD_EVENT);

		/* aux read request*/
		regval = 0;
		regval |= (block_len - 1);
		regval = SET_BITS(8, 20, regval, addr);
		regval = SET_BITS(28, 4, regval, AUX_I2C_READ);
		edp_hw->cur_aux_request = regval;
		EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
		writel(regval, edp_hw->reg_base + REG_EDP_PHY_AUX);
		udelay(1);
		writel(1, edp_hw->reg_base + REG_EDP_AUX_START);

		/* wait aux reply event */
		/*
		while (!(readl(edp_hw->reg_base + REG_EDP_HPD_EVENT) & (1 << 1))) {
			if (timeout >= 50000) {
				EDP_LOW_DBG("edp_aux_i2c_read wait AUX_REPLY event timeout, request:0x%x\n", edp_hw->cur_aux_request);
				ret = RET_AUX_TIMEOUT;
				goto CLR_EVENT;
			}
			timeout++;
		}
		*/

		/* wait for AUX_REPLY*/
		while (((readl(edp_hw->reg_base + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
			if (timeout >= 50000) {
				EDP_LOW_DBG("edp_aux_i2c_read wait AUX_REPLY timeout, request:0x%x\n", edp_hw->cur_aux_request);
				ret = RET_AUX_TIMEOUT;
				goto CLR_EVENT;
			}
			timeout++;
		}

		regval = readl(edp_hw->reg_base + REG_EDP_AUX_TIMEOUT);
		/* not ensure, need confirm from inno */
		if (((regval >> 24) & 0xf) == 0xe) {
			EDP_LOW_DBG("edp_aux_read recieve without STOP, request:0x%x\n", edp_hw->cur_aux_request);
			ret = RET_AUX_NO_STOP;
			goto CLR_EVENT;
		}
		regval &= 0xf0;
		if ((regval >> 4) == AUX_REPLY_I2C_NACK) {
			EDP_LOW_DBG("edp_aux_i2c_read recieve AUX_REPLY_NACK, request:0x%x\n", edp_hw->cur_aux_request);
			ret = RET_AUX_NACK;
			goto CLR_EVENT;
		} else if ((regval >> 4) == AUX_REPLY_I2C_DEFER) {
			EDP_LOW_DBG("edp_aux_i2c_read recieve AUX_REPLY_I2C_DEFER, request:0x%x\n", edp_hw->cur_aux_request);
			ret = RET_AUX_DEFER;
			goto CLR_EVENT;
		}

		/* aux read reply*/
		for (i = 0; i < 4; i++) {
			reg_val[i] = readl(edp_hw->reg_base + REG_EDP_AUX_DATA0 + i * 0x4);
			usleep_range(10, 20);
			EDP_LOW_DBG("[%s]: data: reg_val[%d] = 0x%x", __func__, i, reg_val[i]);
		}

		for (i = 0; i < block_len; i++) {
			if ((block_len * j + i) <= len)
				buf[j * block_len + i] = GET_BITS((i % 4) * 8, 8, reg_val[i / 4]);
		}
	}

CLR_EVENT:
	regval = readl(edp_hw->reg_base + REG_EDP_HPD_EVENT);
	regval |= (1 << 1);
	writel(regval, edp_hw->reg_base + REG_EDP_HPD_EVENT);
	mutex_unlock(&edp_hw->aux_lock);

	return ret;
}

s32 edp_aux_i2c_write(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf)
{
	u32 reg_val[4];
	u32 regval = 0;
	s32 i, j;
	u32 timeout = 0;
	s32 ret = 0;
	u32 block_len = 16;
	u32 data_len;
	u32 ext = (len % block_len) ? 1 : 0;

	mutex_lock(&edp_hw->aux_lock);
	for (j = 0; j < (len / block_len + ext); j++) {
		if (len <= 16)
			data_len = len;
		else if ((16 * j) < (len - 16))
			data_len = 16;
		else
			/* this is the last one transmit */
			data_len = len - 16;
		writel(0, edp_hw->reg_base + REG_EDP_PHY_AUX);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA0);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA1);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA2);
		writel(0, edp_hw->reg_base + REG_EDP_AUX_DATA3);
		writel(0, edp_hw->reg_base + REG_EDP_PHY_AUX);
		regval = readl(edp_hw->reg_base + REG_EDP_HPD_EVENT);
		regval |= (1 << 1);
		writel(regval, edp_hw->reg_base + REG_EDP_HPD_EVENT);


		for (i = 0; i < data_len; i++) {
			reg_val[i / 4] = SET_BITS(i % 4, 8, reg_val[i / 4], buf[j * block_len + i]);
		}

		for (i = 0; i < 4; i++) {
			writel(reg_val[i], edp_hw->reg_base + REG_EDP_AUX_DATA0 + i * 0x4);
			usleep_range(10, 20);
			EDP_LOW_DBG("[%s]: data: reg_val[%d] = 0x%x", __func__, i, reg_val[i]);
		}

		/* aux write request*/
		regval = 0;
		regval |= (data_len - 1);
		regval = SET_BITS(8, 20, regval, addr);
		regval = SET_BITS(28, 4, regval, AUX_I2C_WRITE);
		edp_hw->cur_aux_request = regval;
		EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
		writel(regval, edp_hw->reg_base + REG_EDP_PHY_AUX);

		udelay(1);
		writel(1, edp_hw->reg_base + REG_EDP_AUX_START);

		/* wait aux reply event */
		/*
		while (!(readl(edp_hw->reg_base + REG_EDP_HPD_EVENT) & (1 << 1))) {
			if (timeout >= 50000) {
				EDP_LOW_DBG("edp_aux_i2c_write wait AUX_REPLY event timeout, request:0x%x\n", edp_hw->cur_aux_request);
				ret = RET_AUX_TIMEOUT;
				goto CLR_EVENT;
			}
			timeout++;
		}
		*/

		/* wait for AUX_REPLY*/
		while (((readl(edp_hw->reg_base + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
			if (timeout >= 50000) {
				EDP_LOW_DBG("edp_aux_i2c_write wait AUX_REPLY timeout, request:0x%x\n", edp_hw->cur_aux_request);
				ret = RET_AUX_TIMEOUT;
				goto CLR_EVENT;
			}
			timeout++;
		}

		regval = readl(edp_hw->reg_base + REG_EDP_AUX_TIMEOUT);
		/* not ensure, need confirm from inno */
		if (((regval >> 24) & 0xf) == 0xe) {
			EDP_LOW_DBG("edp_aux_read recieve without STOP, request:0x%x\n", edp_hw->cur_aux_request);
			ret = RET_AUX_NO_STOP;
			goto CLR_EVENT;
		}
		regval &= 0xf0;
		if ((regval >> 4) == AUX_REPLY_I2C_NACK) {
			EDP_LOW_DBG("edp_aux_i2c_write recieve AUX_REPLY_NACK, request:0x%x\n", edp_hw->cur_aux_request);
			ret = RET_AUX_NACK;
			goto CLR_EVENT;
		} else if ((regval >> 4) == AUX_REPLY_I2C_DEFER) {
			EDP_LOW_DBG("edp_aux_i2c_write recieve AUX_REPLY_I2C_DEFER, request:0x%x\n", edp_hw->cur_aux_request);
			ret = RET_AUX_DEFER;
			goto CLR_EVENT;
		}
	}

CLR_EVENT:
	regval = readl(edp_hw->reg_base + REG_EDP_HPD_EVENT);
	regval |= (1 << 1);
	writel(regval, edp_hw->reg_base + REG_EDP_HPD_EVENT);
	mutex_unlock(&edp_hw->aux_lock);

	return ret;
}

s32 inno_aux_read(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf)
{
	return edp_aux_read(edp_hw, addr, len, buf);
}

s32 inno_aux_write(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf)
{
	return edp_aux_write(edp_hw, addr, len, buf);
}

s32 inno_aux_i2c_read(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 addr, s32 len, char *buf)
{
	return edp_aux_i2c_read(edp_hw, i2c_addr, len, buf);
}

s32 inno_aux_i2c_write(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 addr, s32 len, char *buf)
{
	return edp_aux_i2c_write(edp_hw, i2c_addr, len, buf);
}

s32 inno_aux_read_ext(struct sunxi_edp_hw_desc *edp_hw, s32 addr, s32 len, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;
	while (retry_cnt < 7) {
		ret = inno_aux_read(edp_hw, addr, len, buf);
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

s32 inno_aux_i2c_read_ext(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 len, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;
	while (retry_cnt < 7) {
		ret = inno_aux_i2c_read(edp_hw, i2c_addr, i2c_addr, len, buf);
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

s32 inno_aux_i2c_write_ext(struct sunxi_edp_hw_desc *edp_hw, s32 i2c_addr, s32 len, char *buf)
{
	u32 retry_cnt = 0;
	s32 ret = 0;
	while (retry_cnt < 7) {
		ret = inno_aux_i2c_write(edp_hw, i2c_addr, i2c_addr, len, buf);
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


s32 edp_transfer_unit_config(struct sunxi_edp_hw_desc *edp_hw, u32 bpp, u32 lane_cnt, u64 bit_rate, u32 pixel_clk)
{
	u32 reg_val;
	u32 pack_data_rate;
	u32 valid_symbol;
	u32 hblank;
	u32 bandwidth;
	u32 pre_div = 1000;

	reg_val = readl(edp_hw->reg_base + REG_EDP_HACTIVE_BLANK);
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
		EDP_ERR("valid symbol now: %d, should less than 62\n", (valid_symbol / pre_div));
		EDP_ERR("Try to enlarge lane count or lane rate!\n");
		return RET_FAIL;
	}

	EDP_LOW_DBG("[edp_transfer_unit]: bpp:%d valid_symbol:%d\n", bpp, valid_symbol);

	reg_val = readl(edp_hw->reg_base + REG_EDP_FRAME_UNIT);
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
	writel(reg_val, edp_hw->reg_base + REG_EDP_FRAME_UNIT);

	return RET_OK;
}


void edp_set_link_clk_cyc(struct sunxi_edp_hw_desc *edp_hw, u32 lane_cnt, u64 bit_rate, u32 pixel_clk)
{
	u32 reg_val;
	u32 hblank;
	u32 symbol_clk;
	u32 link_cyc;

	/*hblank_link_cyc = hblank * (symbol_clk / 4) / pixclk*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_HACTIVE_BLANK);
	hblank = GET_BITS(2, 14, reg_val);

	symbol_clk = bit_rate / 10000000;


	link_cyc = 1000 * hblank * (symbol_clk / lane_cnt) / (pixel_clk / 1000);
	EDP_LOW_DBG("link_cyc:%d hblank:%d symbol_clk:%d pixel_clk:%d\n", link_cyc, hblank, symbol_clk, pixel_clk);

	reg_val = readl(edp_hw->reg_base + REG_EDP_HBLANK_LINK_CYC);
	reg_val = SET_BITS(0, 16, reg_val, link_cyc);
	writel(reg_val, edp_hw->reg_base + REG_EDP_HBLANK_LINK_CYC);
}

s32 inno_init_early(struct sunxi_edp_hw_desc *edp_hw)
{
	mutex_init(&edp_hw->aux_lock);

	return RET_OK;
}

s32 inno_controller_init(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	s32 ret = 0;

	/* reserved for debug */
	//ret = edp_bist_test(edp_core);
	if (ret < 0)
		return ret;
	edp_controller_soft_reset(edp_hw);
	edp_hpd_enable(edp_hw);
	edp_mode_init(edp_hw, 1);
	edp_controller_mode_init(edp_hw, edp_core->controller_mode);
	edp_resistance_init(edp_hw);

	/* use 2.7G bitrate to configurate, ensure aux can be accessed
	 * to get sink capability such as timing and lane parameter
	 */
	edp_aux_16m_config(edp_hw, BIT_RATE_2G7);
	edp_corepll_config(edp_hw, BIT_RATE_2G7);
	usleep_range(500, 1000);

	return ret;
}


s32 inno_enable(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	u64 bit_rate;

	bit_rate = edp_core->lane_para.bit_rate;
	if (bit_rate == edp_hw->cur_bit_rate)
		return RET_OK;

	edp_aux_16m_config(edp_hw, bit_rate);

	edp_corepll_config(edp_hw, bit_rate);

	usleep_range(500, 1000);

	return RET_OK;
}

s32 inno_disable(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	edp_video_stream_disable(edp_hw);
	edp_main_link_reset(edp_hw);

	return 0;
}

void inno_scrambling_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
}

u64 inno_get_max_rate(struct sunxi_edp_hw_desc *edp_hw)
{
	return BIT_RATE_2G7;
}

u32 inno_get_max_lane(struct sunxi_edp_hw_desc *edp_hw)
{
	return 4;
}

bool inno_support_tps3(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool inno_support_fast_train(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool inno_support_audio(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool inno_support_psr(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool inno_support_psr2(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool inno_support_ssc(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool inno_support_assr(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool inno_support_mst(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool inno_support_fec(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool inno_support_hdcp1x(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool inno_support_hw_hdcp1x(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool inno_support_hdcp2x(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool inno_support_hw_hdcp2x(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

s32 edp_read_edid(struct sunxi_edp_hw_desc *edp_hw,  u8 *edid, size_t len)
{
	s32 i;
	s32 ret;

	for (i = 0; i < (len / 16); i++) {
		ret = inno_aux_i2c_read_ext(edp_hw, EDID_ADDR, 16, (char *)(edid) + (i * 16));
		if (ret < 0)
			return ret;

	}
	for (i = 0; i < len; i++)
		EDP_EDID_DBG("edid[%d] = 0x%x\n", i, *((char *)(edid) + i));

	return 0;
}

s32 inno_read_edid_block(struct sunxi_edp_hw_desc *edp_hw,
			 u8 *raw_edid, unsigned int block_id, size_t len)
{
	char g_tx_buf[16];

	memset(g_tx_buf, 0, sizeof(g_tx_buf));

	if (block_id == 0)
		inno_aux_i2c_write_ext(edp_hw, EDID_ADDR, 1, &g_tx_buf[0]);

	return edp_read_edid(edp_hw, raw_edid, len);

}

void edp_set_video_timings(struct sunxi_edp_hw_desc *edp_hw,
			   struct disp_video_timings *timings)
{
	u32 reg_val;

	/*hsync/vsync polarity setting*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_SYNC_POLARITY);
	reg_val = SET_BITS(1, 1, reg_val, timings->hor_sync_polarity);
	reg_val = SET_BITS(0, 1, reg_val, timings->ver_sync_polarity);
	writel(reg_val, edp_hw->reg_base + REG_EDP_SYNC_POLARITY);

	/*h/vactive h/vblank setting*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_HACTIVE_BLANK);
	reg_val = SET_BITS(16, 16, reg_val, timings->x_res);
	reg_val = SET_BITS(2, 14, reg_val, (timings->hor_total_time - timings->x_res));
	writel(reg_val, edp_hw->reg_base + REG_EDP_HACTIVE_BLANK);

	reg_val = readl(edp_hw->reg_base + REG_EDP_VACTIVE_BLANK);
	reg_val = SET_BITS(0, 16, reg_val, timings->y_res);
	reg_val = SET_BITS(16, 16, reg_val, (timings->ver_total_time - timings->y_res));
	writel(reg_val, edp_hw->reg_base + REG_EDP_VACTIVE_BLANK);

	/*h/vstart setting*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_SYNC_START);
	reg_val = SET_BITS(0, 16, reg_val, (timings->hor_sync_time + timings->hor_back_porch));
	reg_val = SET_BITS(16, 16, reg_val, (timings->ver_sync_time + timings->ver_back_porch));
	writel(reg_val, edp_hw->reg_base + REG_EDP_SYNC_START);

	/*hs/vswidth  h/v_front_porch setting*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_HSW_FRONT_PORCH);
	reg_val = SET_BITS(16, 16, reg_val, timings->hor_sync_time);
	reg_val = SET_BITS(0, 16, reg_val, timings->hor_front_porch);
	writel(reg_val, edp_hw->reg_base + REG_EDP_HSW_FRONT_PORCH);

	reg_val = readl(edp_hw->reg_base + REG_EDP_VSW_FRONT_PORCH);
	reg_val = SET_BITS(16, 16, reg_val, timings->ver_sync_time);
	reg_val = SET_BITS(0, 16, reg_val, timings->ver_front_porch);
	writel(reg_val, edp_hw->reg_base + REG_EDP_VSW_FRONT_PORCH);
}

s32 inno_set_video_format(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
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

	edp_set_input_video_mapping(edp_hw, (enum edp_video_mapping_e) video_map);

	return RET_OK;

}

s32 inno_set_video_timings(struct sunxi_edp_hw_desc *edp_hw,
			   struct disp_video_timings *tmgs)
{
	u32 pixel_clk = tmgs->pixel_clk;
	s32 ret = 0;

	edp_set_video_timings(edp_hw, tmgs);
	ret = edp_pixpll_cfg(edp_hw, pixel_clk);
	if (ret) {
		EDP_ERR("pixclk pll param calculate error!\n");
		return RET_FAIL;
	}

	return RET_OK;
}

s32 inno_set_transfer_config(struct sunxi_edp_hw_desc *edp_hw,
			     struct edp_tx_core *edp_core)
{
	struct disp_video_timings *tmgs = &edp_core->timings;
	u32 pixel_clk = tmgs->pixel_clk;
	u64 bit_rate = edp_core->lane_para.bit_rate;
	u32 lane_cnt = edp_core->lane_para.lane_cnt;
	u32 bpp = edp_core->lane_para.bpp;
	s32 ret;

	ret = edp_transfer_unit_config(edp_hw, bpp, lane_cnt, bit_rate, pixel_clk);
	if (ret)
		return ret;

	edp_set_link_clk_cyc(edp_hw, lane_cnt, bit_rate, pixel_clk);

	return RET_OK;
}

s32 inno_audio_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	edp_audio_timestamp_hblank_setting(edp_hw, true);
	edp_audio_timestamp_vblank_setting(edp_hw, true);
	edp_audio_stream_hblank_setting(edp_hw, true);
	edp_audio_stream_vblank_setting(edp_hw, true);
	edp_audio_soft_reset(edp_hw);

	return RET_OK;
}

s32 inno_audio_disable(struct sunxi_edp_hw_desc *edp_hw)
{
	edp_audio_timestamp_hblank_setting(edp_hw, false);
	edp_audio_timestamp_vblank_setting(edp_hw, false);
	edp_audio_stream_hblank_setting(edp_hw, false);
	edp_audio_stream_vblank_setting(edp_hw, false);

	return RET_OK;
}

s32 inno_audio_config(struct sunxi_edp_hw_desc *edp_hw, int interface,
		      int chn_cnt, int data_width, int data_rate)
{
	edp_audio_interface_config(edp_hw, interface);
	edp_audio_channel_config(edp_hw, chn_cnt);
	edp_audio_data_width_config(edp_hw, data_width);

	return RET_OK;
}

s32 inno_audio_mute(struct sunxi_edp_hw_desc *edp_hw, bool enable, int direction)
{
	edp_audio_mute_config(edp_hw, enable);

	return RET_OK;
}


/*
s32 edp_hal_audio_set_para(edp_audio_t *para)
{
	edp_audio_interface_config(para->interface);
	edp_audio_channel_config(para->chn_cnt);
	edp_audio_data_width_config(para->data_width);
	edp_audio_mute_config(para->mute);

	return RET_OK;
}
*/

s32 inno_ssc_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	u32 reg_val;
	u32 index;

	if (edp_hw->cur_bit_rate == BIT_RATE_1G62)
		index = 0;
	else
		index = 1;

	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	if (enable) {
		reg_val = SET_BITS(4, 2, reg_val, 0);
		reg_val = SET_BITS(21, 1, reg_val, 0);
	} else {
		reg_val = SET_BITS(4, 2, reg_val, recom_corepll[index].frac_pd);
		reg_val = SET_BITS(21, 1, reg_val, 1);
	}
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);

	return RET_OK;
}

bool inno_ssc_is_enabled(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	reg_val = GET_BITS(21, 1, reg_val);

	if (!reg_val)
		return true;

	return false;
}

s32 inno_ssc_get_mode(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	reg_val = GET_BITS(20, 1, reg_val);

	return reg_val;
}

s32 inno_ssc_set_mode(struct sunxi_edp_hw_desc *edp_hw, u32 mode)
{
	u32 reg_val;
	u32 reg_val1;

	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	reg_val1 = readl(edp_hw->reg_base + REG_EDP_ANA_PLL_POSDIV);

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

	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PLL_FBDIV);
	writel(reg_val1, edp_hw->reg_base + REG_EDP_ANA_PLL_POSDIV);

	return RET_OK;
}


s32 inno_psr_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	EDP_ERR("psr isn't support\n");
	return RET_FAIL;
}

bool inno_psr_is_enabled(struct sunxi_edp_hw_desc *edp_hw)
{
	EDP_ERR("psr isn't support\n");
	return false;
}

s32 inno_assr_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	u32 reg_val;

	if (enable) {
		reg_val = readl(edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);
		reg_val = SET_BITS(24, 1, reg_val, 1);
		writel(reg_val, edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);
	} else {
		reg_val = readl(edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);
		reg_val = SET_BITS(24, 1, reg_val, 0);
		writel(reg_val, edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);
	}

	return RET_OK;
}

s32 inno_set_pattern(struct sunxi_edp_hw_desc *edp_hw,
		     u32 pattern, u32 lane_cnt)
{
	if (pattern > 6)
		pattern = 6;

	switch (pattern) {
	case PRBS7:
		inno_set_training_pattern(edp_hw, 6);
		break;
	default:
		inno_set_training_pattern(edp_hw, pattern);
		break;
	}
	return RET_OK;
}

s32 inno_get_color_fmt(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_VIDEO_STREAM_EN);

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


u32 inno_get_pixclk(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;
	u32 fb_div;
	u32 pre_div;
	u32 pll_divb;
	u32 pll_divc;
	u32 pixclk;

	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);
	fb_div = GET_BITS(24, 8, reg_val);
	pre_div = GET_BITS(8, 6, reg_val);

	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_DIV);
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

u32 inno_get_pattern(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_CAPACITY);

	reg_val = GET_BITS(0, 4, reg_val);

	if (reg_val == 0x6)
		reg_val = PRBS7;

	return reg_val;
}

s32 inno_get_lane_para(struct sunxi_edp_hw_desc *edp_hw,
		       struct edp_lane_para *tmp_lane_para)
{
	u32 reg_val;
	u32 regval;

	/* bit rate */
	reg_val = readl(edp_hw->reg_base + REG_EDP_CAPACITY);
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
	reg_val = readl(edp_hw->reg_base + REG_EDP_TX_MAINSEL);
	regval = GET_BITS(0, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_sw[0] = 0;
	if (regval == 2)
		tmp_lane_para->lane_sw[0] = 1;
	if (regval == 4)
		tmp_lane_para->lane_sw[0] = 2;
	if (regval == 6)
		tmp_lane_para->lane_sw[0] = 3;

	regval = GET_BITS(4, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_sw[1] = 0;
	if (regval == 2)
		tmp_lane_para->lane_sw[1] = 1;
	if (regval == 4)
		tmp_lane_para->lane_sw[1] = 2;
	if (regval == 6)
		tmp_lane_para->lane_sw[1] = 3;

	reg_val = readl(edp_hw->reg_base + REG_EDP_TX32_ISEL_DRV);
	regval = GET_BITS(24, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_sw[2] = 0;
	if (regval == 2)
		tmp_lane_para->lane_sw[2] = 1;
	if (regval == 4)
		tmp_lane_para->lane_sw[2] = 2;
	if (regval == 6)
		tmp_lane_para->lane_sw[2] = 3;

	regval = GET_BITS(28, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_sw[3] = 0;
	if (regval == 2)
		tmp_lane_para->lane_sw[3] = 1;
	if (regval == 4)
		tmp_lane_para->lane_sw[3] = 2;
	if (regval == 6)
		tmp_lane_para->lane_sw[3] = 3;


	/*pre*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_TX_POSTSEL);
	regval = GET_BITS(24, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_pre[0] = 0;
	if (regval == 1)
		tmp_lane_para->lane_pre[0] = 1;
	if (regval == 2)
		tmp_lane_para->lane_pre[0] = 2;
	if (regval == 3)
		tmp_lane_para->lane_pre[0] = 3;

	regval = GET_BITS(28, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_pre[1] = 0;
	if (regval == 1)
		tmp_lane_para->lane_pre[1] = 1;
	if (regval == 2)
		tmp_lane_para->lane_pre[1] = 2;
	if (regval == 3)
		tmp_lane_para->lane_pre[1] = 3;

	regval = GET_BITS(16, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_pre[2] = 0;
	if (regval == 1)
		tmp_lane_para->lane_pre[2] = 1;
	if (regval == 2)
		tmp_lane_para->lane_pre[2] = 2;
	if (regval == 3)
		tmp_lane_para->lane_pre[2] = 3;

	regval = GET_BITS(20, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_pre[3] = 0;
	if (regval == 1)
		tmp_lane_para->lane_pre[3] = 1;
	if (regval == 2)
		tmp_lane_para->lane_pre[3] = 2;
	if (regval == 3)
		tmp_lane_para->lane_pre[3] = 3;

	return RET_OK;
}

u32 inno_get_tu_size(struct sunxi_edp_hw_desc *edp_hw)
{
	return LS_PER_TU;
}

u32 inno_get_tu_valid_symbol(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;
	u32 regval;
	u32 valid_symbol;

	reg_val = readl(edp_hw->reg_base + REG_EDP_FRAME_UNIT);
	regval = GET_BITS(0, 7, reg_val);
	valid_symbol = regval * 10;

	regval = GET_BITS(16, 4, reg_val);
	valid_symbol += regval;

	return valid_symbol;
}

bool inno13_audio_is_enabled(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;
	u32 regval0;
	u32 regval1;
	u32 regval2;
	u32 regval3;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO_HBLANK_EN);
	regval0 = GET_BITS(0, 1, reg_val);
	regval1 = GET_BITS(1, 1, reg_val);

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO_VBLANK_EN);
	regval2 = GET_BITS(0, 1, reg_val);
	regval3 = GET_BITS(1, 1, reg_val);

	if (regval0 && regval1 && regval2 && regval3)
		return true;

	return false;
}

u32 inno_get_audio_if(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO);
	reg_val = GET_BITS(0, 1, reg_val);

	return reg_val;
}

bool inno_audio_is_muted(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO);
	reg_val = GET_BITS(15, 1, reg_val);

	return reg_val ? true : false;
}

u32 inno_get_audio_max_channel(struct sunxi_edp_hw_desc *edp_hw)
{
	return 8;
}

u32 inno_get_audio_chn_cnt(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO);
	reg_val = GET_BITS(12, 3, reg_val);

	if (reg_val == 0)
		return 1;
	else if (reg_val == 1)
		return 2;
	else
		return 8;
}

u32 inno_get_audio_date_width(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = readl(edp_hw->reg_base + REG_EDP_AUDIO);
	reg_val = GET_BITS(5, 5, reg_val);

	if (reg_val == 0x10)
		return 16;
	else if (reg_val == 0x14)
		return 20;
	else
		return 24;
}

s32 inno_link_start(struct sunxi_edp_hw_desc *edp_hw)
{
	edp_video_stream_enable(edp_hw);

	return RET_OK;
}

s32 inno_link_stop(struct sunxi_edp_hw_desc *edp_hw)
{
	edp_video_stream_disable(edp_hw);
	edp_main_link_reset(edp_hw);

	return RET_OK;
}

s32 inno_query_transfer_unit(struct sunxi_edp_hw_desc *edp_hw,
				struct edp_tx_core *edp_core,
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
void inno_set_lane_sw_pre(struct sunxi_edp_hw_desc *edp_hw, u32 lane_id, u32 sw, u32 pre, u32 param_type)
{
	u32 sw_lv = training_param_table[param_type][sw][pre].sw_lv;
	u32 pre_lv = training_param_table[param_type][sw][pre].pre_lv;
	u32 reg_val;

	if (lane_id == 0) {
		/* lane0 swing voltage level*/
		reg_val = readl(edp_hw->reg_base + REG_EDP_TX_MAINSEL);
		reg_val = SET_BITS(0, 4, reg_val, sw_lv);
		writel(reg_val, edp_hw->reg_base + REG_EDP_TX_MAINSEL);

		/* lane0 pre emphasis level */
		reg_val = readl(edp_hw->reg_base + REG_EDP_TX_POSTSEL);
		reg_val = SET_BITS(24, 4, reg_val, pre_lv);
		writel(reg_val, edp_hw->reg_base + REG_EDP_TX_POSTSEL);
	} else if (lane_id == 1) {
		/* lane1 swing voltage level*/
		reg_val = readl(edp_hw->reg_base + REG_EDP_TX_MAINSEL);
		reg_val = SET_BITS(4, 4, reg_val, sw_lv);
		writel(reg_val, edp_hw->reg_base + REG_EDP_TX_MAINSEL);

		/* lane1 pre emphasis level */
		reg_val = readl(edp_hw->reg_base + REG_EDP_TX_POSTSEL);
		reg_val = SET_BITS(28, 4, reg_val, pre_lv);
		writel(reg_val, edp_hw->reg_base + REG_EDP_TX_POSTSEL);
	} else if (lane_id == 2) {
		/* lane2 swing voltage level*/
		reg_val = readl(edp_hw->reg_base + REG_EDP_TX32_ISEL_DRV);
		reg_val = SET_BITS(24, 4, reg_val, sw_lv);
		writel(reg_val, edp_hw->reg_base + REG_EDP_TX32_ISEL_DRV);

		/* lane2 pre emphasis level */
		reg_val = readl(edp_hw->reg_base + REG_EDP_TX_POSTSEL);
		reg_val = SET_BITS(16, 4, reg_val, pre_lv);
		writel(reg_val, edp_hw->reg_base + REG_EDP_TX_POSTSEL);
	} else if (lane_id == 3) {
		/* lane3 swing voltage level*/
		reg_val = readl(edp_hw->reg_base + REG_EDP_TX32_ISEL_DRV);
		reg_val = SET_BITS(28, 4, reg_val, sw_lv);
		writel(reg_val, edp_hw->reg_base + REG_EDP_TX32_ISEL_DRV);

		/* lane3 pre emphasis level */
		reg_val = readl(edp_hw->reg_base + REG_EDP_TX_POSTSEL);
		reg_val = SET_BITS(20, 4, reg_val, pre_lv);
		writel(reg_val, edp_hw->reg_base + REG_EDP_TX_POSTSEL);
	} else
		EDP_WRN("%s: lane number is not support!\n", __func__);
}

void inno_set_lane_rate(struct sunxi_edp_hw_desc *edp_hw, u64 bit_rate)
{
	u32 reg_val;

	/*config lane bit rate*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_CAPACITY);
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
	writel(reg_val, edp_hw->reg_base + REG_EDP_CAPACITY);
}

void inno_set_lane_cnt(struct sunxi_edp_hw_desc *edp_hw, u32 lane_cnt)
{
	u32 reg_val;

	if ((lane_cnt < 0) || (lane_cnt > 4)) {
		EDP_WRN("unsupport lane number!\n");
	}

	/*config lane number*/
	reg_val = readl(edp_hw->reg_base + REG_EDP_CAPACITY);
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
	writel(reg_val, edp_hw->reg_base + REG_EDP_CAPACITY);
}

bool inno_check_controller_error(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val, reg_val1;

	reg_val = readl(edp_hw->reg_base + REG_EDP_TX32_ISEL_DRV);
	reg_val = GET_BITS(16, 8, reg_val);

	reg_val1 = readl(edp_hw->reg_base + REG_EDP_HPD_PLUG);
	reg_val1 = GET_BITS(0, 1, reg_val1);

	if ((reg_val != REG_ESD_DEF) || (reg_val1 != REG_HPD_NARROW_PLUSE_DEF)) {
		EDP_LOW_DBG("reg[0x1a4][bit16:bit23]: original_val:0x%x, cur_value:0x%x\n",
				REG_ESD_DEF, reg_val);
		EDP_LOW_DBG("reg[0x88][bit0]: original_val:0x%x, cur_value:0x%x\n",
				REG_HPD_NARROW_PLUSE_DEF, reg_val1);
		return true;
	} else
		return false;
}

void inno_enhance_frame_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	u32 reg_val;

	if (enable) {
		reg_val = readl(edp_hw->reg_base + REG_EDP_HPD_SCALE);
		reg_val = SET_BITS(1, 1, reg_val, 1);
		writel(reg_val, edp_hw->reg_base + REG_EDP_HPD_SCALE);
	} else {
		reg_val = readl(edp_hw->reg_base + REG_EDP_HPD_SCALE);
		reg_val = SET_BITS(1, 1, reg_val, 0);
		writel(reg_val, edp_hw->reg_base + REG_EDP_HPD_SCALE);
	}
}

bool inno_support_enhance_frame(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool inno_set_tcon_tv_use_edp_inner_clk(struct sunxi_edp_hw_desc *edp_hw, u32 bypass)
{
	u32 reg_val;
	reg_val = readl(edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(1, 1, reg_val, bypass);
	writel(reg_val, edp_hw->reg_base + REG_EDP_ANA_PIXPLL_FBDIV);
	return true;
}


static struct sunxi_edp_hw_video_ops inno_edp13_video_ops = {
	.check_controller_error = inno_check_controller_error,
	.assr_enable = inno_assr_enable,
	.psr_enable = inno_psr_enable,
	.psr_is_enabled = inno_psr_is_enabled,
	.get_tu_size = inno_get_tu_size,
	.get_pixel_clk = inno_get_pixclk,
	.get_color_format = inno_get_color_fmt,
	.get_lane_para = inno_get_lane_para,
	.get_tu_valid_symbol = inno_get_tu_valid_symbol,
	.get_hotplug_change = inno_get_hotplug_change,
	.get_hotplug_state = inno_get_hotplug_state,
	.get_pattern = inno_get_pattern,
	.set_pattern = inno_set_pattern,
	.ssc_enable = inno_ssc_enable,
	.ssc_is_enabled = inno_ssc_is_enabled,
	.ssc_set_mode = inno_ssc_set_mode,
	.ssc_get_mode = inno_ssc_get_mode,
	.aux_read = inno_aux_read_ext,
	.aux_write = inno_aux_write,
	.aux_i2c_read = inno_aux_i2c_read,
	.aux_i2c_write = inno_aux_i2c_write,
	.read_edid_block = inno_read_edid_block,
	.irq_enable = inno_irq_enable,
	.irq_disable = inno_irq_disable,
	.irq_handle = inno_irq_handle,
	.main_link_start = inno_link_start,
	.main_link_stop = inno_link_stop,
	.scrambling_enable = inno_scrambling_enable,
	.set_lane_sw_pre = inno_set_lane_sw_pre,
	.set_lane_cnt = inno_set_lane_cnt,
	.set_lane_rate = inno_set_lane_rate,
	.init_early = inno_init_early,
	.init = inno_controller_init,
	.enable = inno_enable,
	.disable = inno_disable,
	.set_video_timings = inno_set_video_timings,
	.set_video_format = inno_set_video_format,
	.config_tu = inno_set_transfer_config,
	.query_tu_capability = inno_query_transfer_unit,
	.support_max_rate = inno_get_max_rate,
	.support_max_lane = inno_get_max_lane,
	.support_tps3 = inno_support_tps3,
	.support_fast_training = inno_support_fast_train,
	.support_psr = inno_support_psr,
	.support_psr2 = inno_support_psr2,
	.support_ssc = inno_support_ssc,
	.support_mst = inno_support_mst,
	.support_fec = inno_support_fec,
	.support_assr = inno_support_assr,
	.enhance_frame_enable = inno_enhance_frame_enable,
	.support_enhance_frame = inno_support_enhance_frame,
	.set_use_inner_clk = inno_set_tcon_tv_use_edp_inner_clk,
};

struct sunxi_edp_hw_video_ops *sunxi_edp_get_hw_video_ops(void)
{
	return &inno_edp13_video_ops;
}

static struct sunxi_edp_hw_audio_ops inno_edp13_audio_ops = {
	.support_audio = inno_support_audio,
	.audio_is_enabled = inno13_audio_is_enabled,
	.get_audio_if = inno_get_audio_if,
	.audio_is_muted = inno_audio_is_muted,
	.audio_enable = inno_audio_enable,
	.audio_disable = inno_audio_disable,
	.audio_mute = inno_audio_mute,
	.audio_config = inno_audio_config,
	.get_audio_data_width = inno_get_audio_date_width,
	.get_audio_chn_cnt = inno_get_audio_chn_cnt,
	.get_audio_max_channel = inno_get_audio_max_channel,
};

struct sunxi_edp_hw_audio_ops *sunxi_edp_get_hw_audio_ops(void)
{
	return &inno_edp13_audio_ops;
}

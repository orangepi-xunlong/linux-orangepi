// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-ahub"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_pcm.h"
#include "snd_sunxi_common.h"
#include "snd_sunxi_ahub.h"
#include "snd_sunxi_adapter.h"

#define DRV_NAME	"sunxi-snd-plat-ahub"

static void sunxi_ahub_get_dai_ucfmt(struct snd_notifier_block *snd_nb)
{
	struct sunxi_ahub *ahub;
	struct snd_sunxi_dai_ucfmt *i2s_dai_fmt;

	if (!snd_nb) {
		SND_LOG_ERR("snd_nb is null\n");
		return;
	}

	ahub = snd_nb->cb_data;
	i2s_dai_fmt = snd_nb->tx_data;

	ahub->data_late = i2s_dai_fmt->data_late;
	ahub->tx_lsb_first = i2s_dai_fmt->tx_lsb_first;
	ahub->rx_lsb_first = i2s_dai_fmt->rx_lsb_first;
}

static void sunxi_ahub_get_hdmi_fmt(struct snd_notifier_block *snd_nb)
{
	struct sunxi_ahub *ahub;
	enum HDMI_FORMAT *hdmi_fmt;

	if (!snd_nb) {
		SND_LOG_ERR("snd_nb is null\n");
		return;
	}

	ahub = snd_nb->cb_data;
	hdmi_fmt = snd_nb->tx_data;

	if (ahub->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
		ahub->playback_dma_param.hdmi_fmt = *hdmi_fmt;
		SND_LOG_DEBUG("hdmi fmt -> %d\n", ahub->playback_dma_param.hdmi_fmt);
	}
}

static int sunxi_ahub_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num;

	SND_LOG_DEBUG("\n");

	regmap = ahub->mem.regmap;
	apb_num = ahub->dts.apb_num;
	tdm_num = ahub->dts.tdm_num;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_dai_set_dma_data(dai, substream, &ahub->playback_dma_param);
	} else {
		snd_soc_dai_set_dma_data(dai, substream, &ahub->capture_dma_param);
	}

	/* APBIF & I2S of RST and GAT */
	if (tdm_num > 3 || apb_num > 2) {
		SND_LOG_ERR("unspport tdm num or apbif num\n");
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST,
				   0x1 << (APBIF_TXDIF0_RST - apb_num),
				   0x1 << (APBIF_TXDIF0_RST - apb_num));
		regmap_update_bits(regmap, SUNXI_AHUB_GAT,
				   0x1 << (APBIF_TXDIF0_GAT - apb_num),
				   0x1 << (APBIF_TXDIF0_GAT - apb_num));
	} else {
		regmap_update_bits(regmap, SUNXI_AHUB_RST,
				   0x1 << (APBIF_RXDIF0_RST - apb_num),
				   0x1 << (APBIF_RXDIF0_RST - apb_num));
		regmap_update_bits(regmap, SUNXI_AHUB_GAT,
				   0x1 << (APBIF_RXDIF0_GAT - apb_num),
				   0x1 << (APBIF_RXDIF0_GAT - apb_num));
	}

	return 0;
}

static int sunxi_ahub_dai_set_pll(struct snd_soc_dai *dai,
				  int pll_id, int source,
				  unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct sunxi_ahub_clk *clk = NULL;

	SND_LOG_DEBUG("stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out);

	if (IS_ERR_OR_NULL(ahub)) {
		SND_LOG_ERR("ahub is null.\n");
		return -ENOMEM;
	}
	clk = &ahub->clk;

	if (freq_in > 24576000) {
		if (clk_set_parent(clk->clk_module, clk->clk_pllx4)) {
			SND_LOG_ERR("set parent of clk_module to pllx4 failed\n");
			return -EINVAL;
		}
		if (clk_set_rate(clk->clk_pllx4, freq_in)) {
			SND_LOG_ERR("freq : %u pllx4 clk unsupport\n", freq_in);
			return -EINVAL;
		}
	} else {
		if (clk_set_parent(clk->clk_module, clk->clk_pll)) {
			SND_LOG_ERR("set parent of clk_module to pll failed\n");
			return -EINVAL;
		}
		if (clk_set_rate(clk->clk_pll, freq_in)) {
			SND_LOG_ERR("freq : %u pll clk unsupport\n", freq_in);
			return -EINVAL;
		}
	}
	if (clk_set_rate(clk->clk_module, freq_out)) {
		SND_LOG_ERR("freq : %u module clk unsupport\n", freq_out);
		return -EINVAL;
	}

	ahub->pllclk_freq = freq_in;
	ahub->mclk_freq = freq_out;

	return 0;
}

static int sunxi_ahub_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,
				     unsigned int freq, int dir)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num;
	unsigned int mclk_ratio, mclk_ratio_map;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(ahub)) {
		SND_LOG_ERR("ahub is null.\n");
		return -ENOMEM;
	}
	regmap = ahub->mem.regmap;
	tdm_num = ahub->dts.tdm_num;

	if (freq == 0) {
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CLKD(tdm_num),
				   0x1 << I2S_CLKD_MCLK, 0x0 << I2S_CLKD_MCLK);
		SND_LOG_DEBUG("mclk freq: 0\n");
		return 0;
	}
	if (ahub->pllclk_freq == 0) {
		SND_LOG_ERR("pllclk freq is invalid\n");
		return -ENOMEM;
	}
	mclk_ratio = ahub->pllclk_freq / freq;

	switch (mclk_ratio) {
	case 1:
		mclk_ratio_map = 1;
		break;
	case 2:
		mclk_ratio_map = 2;
		break;
	case 4:
		mclk_ratio_map = 3;
		break;
	case 6:
		mclk_ratio_map = 4;
		break;
	case 8:
		mclk_ratio_map = 5;
		break;
	case 12:
		mclk_ratio_map = 6;
		break;
	case 16:
		mclk_ratio_map = 7;
		break;
	case 24:
		mclk_ratio_map = 8;
		break;
	case 32:
		mclk_ratio_map = 9;
		break;
	case 48:
		mclk_ratio_map = 10;
		break;
	case 64:
		mclk_ratio_map = 11;
		break;
	case 96:
		mclk_ratio_map = 12;
		break;
	case 128:
		mclk_ratio_map = 13;
		break;
	case 176:
		mclk_ratio_map = 14;
		break;
	case 192:
		mclk_ratio_map = 15;
		break;
	default:
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CLKD(tdm_num),
				   0x1 << I2S_CLKD_MCLK, 0x0 << I2S_CLKD_MCLK);
		SND_LOG_ERR("mclk freq div unsupport\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CLKD(tdm_num),
			   0xf << I2S_CLKD_MCLKDIV,
			   mclk_ratio_map << I2S_CLKD_MCLKDIV);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CLKD(tdm_num),
			   0x1 << I2S_CLKD_MCLK, 0x1 << I2S_CLKD_MCLK);

	return 0;
}

static int sunxi_ahub_dai_set_bclk_ratio(struct snd_soc_dai *dai, unsigned int ratio)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num;
	unsigned int bclk_ratio;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(ahub)) {
		SND_LOG_ERR("ahub is null.\n");
		return -ENOMEM;
	}
	regmap = ahub->mem.regmap;
	tdm_num = ahub->dts.tdm_num;

	/* ratio -> cpudai pllclk / pcm rate */
	switch (ratio) {
	case 1:
		bclk_ratio = 1;
		break;
	case 2:
		bclk_ratio = 2;
		break;
	case 4:
		bclk_ratio = 3;
		break;
	case 6:
		bclk_ratio = 4;
		break;
	case 8:
		bclk_ratio = 5;
		break;
	case 12:
		bclk_ratio = 6;
		break;
	case 16:
		bclk_ratio = 7;
		break;
	case 24:
		bclk_ratio = 8;
		break;
	case 32:
		bclk_ratio = 9;
		break;
	case 48:
		bclk_ratio = 10;
		break;
	case 64:
		bclk_ratio = 11;
		break;
	case 96:
		bclk_ratio = 12;
		break;
	case 128:
		bclk_ratio = 13;
		break;
	case 176:
		bclk_ratio = 14;
		break;
	case 192:
		bclk_ratio = 15;
		break;
	default:
		SND_LOG_ERR("bclk freq div unsupport\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CLKD(tdm_num),
			   0xf << I2S_CLKD_BCLKDIV,
			   bclk_ratio << I2S_CLKD_BCLKDIV);

	return 0;
}

static int sunxi_ahub_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num, tx_pin, rx_pin;
	unsigned int mode;
	unsigned int lrck_polarity, brck_polarity;

	SND_LOG_DEBUG("\n");

	ahub->fmt = fmt;

	if (IS_ERR_OR_NULL(ahub)) {
		SND_LOG_ERR("ahub is null.\n");
		return -ENOMEM;
	}
	regmap = ahub->mem.regmap;
	tdm_num = ahub->dts.tdm_num;
	tx_pin = ahub->dts.tx_pin;
	rx_pin = ahub->dts.rx_pin;

	/* set TDM format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		mode = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		mode = 2;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		mode = 1;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		mode = 0;
		/* L data MSB after FRM LRC (short frame) */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
				   0x1 << I2S_FMT0_LRCK_WIDTH,
				   0x0 << I2S_FMT0_LRCK_WIDTH);
		break;
	case SND_SOC_DAIFMT_DSP_B:
		mode = 0;
		/* L data MSB during FRM LRC (long frame) */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
				   0x1 << I2S_FMT0_LRCK_WIDTH,
				   0x1 << I2S_FMT0_LRCK_WIDTH);
		break;
	default:
		SND_LOG_ERR("format setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x3 << I2S_CTL_MODE, mode << I2S_CTL_MODE);
	/* regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, tx_pin),
	 *		   0x3 << I2S_OUT_OFFSET, offset << I2S_OUT_OFFSET);
	 */
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 0),
			   0x3 << I2S_OUT_OFFSET, ahub->data_late << I2S_OUT_OFFSET);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 1),
			   0x3 << I2S_OUT_OFFSET, ahub->data_late << I2S_OUT_OFFSET);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 2),
			   0x3 << I2S_OUT_OFFSET, ahub->data_late << I2S_OUT_OFFSET);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 3),
			   0x3 << I2S_OUT_OFFSET, ahub->data_late << I2S_OUT_OFFSET);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_IN_SLOT(tdm_num),
			   0x3 << I2S_IN_OFFSET, ahub->data_late << I2S_IN_OFFSET);

	if (!ahub->rx_lsb_first && !ahub->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT1(tdm_num),
				   1 << I2S_FMT1_RX_LSB, 0 << I2S_FMT1_RX_LSB);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT1(tdm_num),
				   1 << I2S_FMT1_TX_LSB, 0 << I2S_FMT1_TX_LSB);
	} else if (ahub->rx_lsb_first && !ahub->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT1(tdm_num),
				   1 << I2S_FMT1_RX_LSB, 1 << I2S_FMT1_RX_LSB);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT1(tdm_num),
				   1 << I2S_FMT1_TX_LSB, 0 << I2S_FMT1_TX_LSB);
	} else if (!ahub->rx_lsb_first && ahub->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT1(tdm_num),
				   1 << I2S_FMT1_RX_LSB, 0 << I2S_FMT1_RX_LSB);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT1(tdm_num),
				   1 << I2S_FMT1_TX_LSB, 1 << I2S_FMT1_TX_LSB);
	} else {
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT1(tdm_num),
				   1 << I2S_FMT1_RX_LSB, 1 << I2S_FMT1_RX_LSB);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT1(tdm_num),
				   1 << I2S_FMT1_TX_LSB, 1 << I2S_FMT1_TX_LSB);
	}

	/* set lrck & bclk polarity */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		lrck_polarity = 0;
		brck_polarity = 0;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		lrck_polarity = 1;
		brck_polarity = 0;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		lrck_polarity = 0;
		brck_polarity = 1;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		lrck_polarity = 1;
		brck_polarity = 1;
		break;
	default:
		SND_LOG_ERR("invert clk setting failed\n");
		return -EINVAL;
	}

	lrck_polarity ^= !mode;

	regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
			   0x1 << I2S_FMT0_LRCK_POLARITY,
			   lrck_polarity << I2S_FMT0_LRCK_POLARITY);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
			   0x1 << I2S_FMT0_BCLK_POLARITY,
			   brck_polarity << I2S_FMT0_BCLK_POLARITY);

	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		/* lrck & bclk dir input */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   0x1 << I2S_CTL_CLK_OUT, 0x0 << I2S_CTL_CLK_OUT);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		/* lrck & bclk dir output */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   0x1 << I2S_CTL_CLK_OUT, 0x1 << I2S_CTL_CLK_OUT);
		break;
	default:
		SND_LOG_ERR("unknown master/slave format\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_ahub_dai_set_tdm_slot(struct snd_soc_dai *dai,
				       unsigned int tx_mask, unsigned int rx_mask,
				       int slots, int slot_width)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num, tx_pin, rx_pin;
	unsigned int slot_width_map, lrck_width_map;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(ahub)) {
		SND_LOG_ERR("ahub is null\n");
		return -ENOMEM;
	}
	regmap = ahub->mem.regmap;
	tdm_num = ahub->dts.tdm_num;
	tx_pin = ahub->dts.tx_pin;
	rx_pin = ahub->dts.rx_pin;

	switch (slot_width) {
	case 8:
		slot_width_map = 1;
		break;
	case 12:
		slot_width_map = 2;
		break;
	case 16:
		slot_width_map = 3;
		break;
	case 20:
		slot_width_map = 4;
		break;
	case 24:
		slot_width_map = 5;
		break;
	case 28:
		slot_width_map = 6;
		break;
	case 32:
		slot_width_map = 7;
		break;
	default:
		SND_LOG_ERR("unknown slot width\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
			   0x7 << I2S_FMT0_SW, slot_width_map << I2S_FMT0_SW);

	/* bclk num of per channel
	 * I2S/RIGHT_J/LEFT_J	-> lrck long total is lrck_width_map * 2
	 * DSP_A/DAP_B		-> lrck long total is lrck_width_map * 1
	 */
	switch (ahub->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		slots /= 2;
		break;
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		break;
	default:
		SND_LOG_ERR("unsupoort format\n");
		return -EINVAL;
	}
	lrck_width_map = slots * slot_width - 1;
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
			   0x3ff << I2S_FMT0_LRCK_PERIOD,
			   lrck_width_map << I2S_FMT0_LRCK_PERIOD);

	return 0;
}

static int sunxi_ahub_dai_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params,
				    struct snd_soc_dai *dai)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num, tx_pin, rx_pin;
	unsigned int channels;
	unsigned int channels_en[16] = {
		0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
		0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
	};
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd),
								       substream);

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(ahub)) {
		SND_LOG_ERR("ahub is null.\n");
		return -ENOMEM;
	}
	regmap = ahub->mem.regmap;
	apb_num = ahub->dts.apb_num;
	tdm_num = ahub->dts.tdm_num;
	tx_pin = ahub->dts.tx_pin;
	rx_pin = ahub->dts.rx_pin;

	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		/* apbifn bits */
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (ahub->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
				if (dma_params->hdmi_fmt > HDMI_FMT_PCM) {
					regmap_update_bits(regmap,
							   SUNXI_AHUB_APBIF_TX_CTL(apb_num),
							   0x7 << APBIF_TX_WS,
							   0x7 << APBIF_TX_WS);
					regmap_update_bits(regmap,
							   SUNXI_AHUB_APBIF_TXFIFO_CTL(apb_num),
							   0x1 << APBIF_TX_TXIM,
							   0x0 << APBIF_TX_TXIM);
				} else {
					regmap_update_bits(regmap,
							   SUNXI_AHUB_APBIF_TX_CTL(apb_num),
							   0x7 << APBIF_TX_WS,
							   0x3 << APBIF_TX_WS);
					regmap_update_bits(regmap,
							   SUNXI_AHUB_APBIF_TXFIFO_CTL(apb_num),
							   0x1 << APBIF_TX_TXIM,
							   0x1 << APBIF_TX_TXIM);
				}
			} else {
				regmap_update_bits(regmap,
						   SUNXI_AHUB_APBIF_TX_CTL(apb_num),
						   0x7 << APBIF_TX_WS,
						   0x3 << APBIF_TX_WS);
				regmap_update_bits(regmap,
						   SUNXI_AHUB_APBIF_TXFIFO_CTL(apb_num),
						   0x1 << APBIF_TX_TXIM,
						   0x1 << APBIF_TX_TXIM);
			}
		} else {
			regmap_update_bits(regmap,
					   SUNXI_AHUB_APBIF_RX_CTL(apb_num),
					   0x7 << APBIF_RX_WS,
					   0x3 << APBIF_RX_WS);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_APBIF_RXFIFO_CTL(apb_num),
					   0x3 << APBIF_RX_RXOM,
					   0x1 << APBIF_RX_RXOM);
		}
		/* tdmn bits */
		if (ahub->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
			if (dma_params->hdmi_fmt > HDMI_FMT_PCM) {
				regmap_update_bits(regmap,
						   SUNXI_AHUB_I2S_FMT0(tdm_num),
						   0x7 << I2S_FMT0_SR,
						   0x5 << I2S_FMT0_SR);
			} else {
				regmap_update_bits(regmap,
						   SUNXI_AHUB_I2S_FMT0(tdm_num),
						   0x7 << I2S_FMT0_SR,
						   0x3 << I2S_FMT0_SR);
			}
		} else {
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_FMT0(tdm_num),
					   0x7 << I2S_FMT0_SR,
					   0x3 << I2S_FMT0_SR);
		}
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TX_CTL(apb_num),
					   0x7 << APBIF_TX_WS, 0x5 << APBIF_TX_WS);
			regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TXFIFO_CTL(apb_num),
					   0x1 << APBIF_TX_TXIM, 0x1 << APBIF_TX_TXIM);
		} else {
			regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RX_CTL(apb_num),
					   0x7 << APBIF_RX_WS, 0x5 << APBIF_RX_WS);
			regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RXFIFO_CTL(apb_num),
					   0x3 << APBIF_RX_RXOM, 0x1 << APBIF_RX_RXOM);
		}
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
				   0x7 << I2S_FMT0_SR, 0x5 << I2S_FMT0_SR);
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TX_CTL(apb_num),
					   0x7 << APBIF_TX_WS, 0x7 << APBIF_TX_WS);
			regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TXFIFO_CTL(apb_num),
					   0x1 << APBIF_TX_TXIM, 0x1 << APBIF_TX_TXIM);
		} else {
			regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RX_CTL(apb_num),
					   0x7 << APBIF_RX_WS, 0x7 << APBIF_RX_WS);
			regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RXFIFO_CTL(apb_num),
					   0x3 << APBIF_RX_RXOM, 0x1 << APBIF_RX_RXOM);
		}
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
				   0x7 << I2S_FMT0_SR, 0x7 << I2S_FMT0_SR);
		break;
	default:
		SND_LOG_ERR("unrecognized format bits\n");
		return -EINVAL;
	}

	/* set channels */
	channels = params_channels(params);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* apbifn channels */
		regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TX_CTL(apb_num),
				   0xf << APBIF_TX_CHAN_NUM,
				   (channels - 1) << APBIF_TX_CHAN_NUM);
		/* tdmn channels */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CHCFG(tdm_num),
				   0xf << I2S_CHCFG_TX_CHANNUM,
				   (channels - 1) << I2S_CHCFG_TX_CHANNUM);
		if (ahub->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
			regmap_write(regmap,
				     SUNXI_AHUB_I2S_OUT_CHMAP0(tdm_num, 0),
				     0x10);
			if (dma_params->hdmi_fmt > HDMI_FMT_PCM) {
				regmap_write(regmap,
					     SUNXI_AHUB_I2S_OUT_CHMAP0(tdm_num, 1),
					     0x32);
				regmap_write(regmap,
					     SUNXI_AHUB_I2S_OUT_CHMAP0(tdm_num, 2),
					     0x54);
				regmap_write(regmap,
					     SUNXI_AHUB_I2S_OUT_CHMAP0(tdm_num, 3),
					     0x76);
			} else {
				if (channels > 2) {
					regmap_write(regmap,
						     SUNXI_AHUB_I2S_OUT_CHMAP0(tdm_num, 1),
						     0x23);

					/* only 5.1 & 7.1 */
					if (channels > 4) {
						/* 5.1 hit this */
						if (channels == 6) {
							regmap_write(regmap,
								     SUNXI_AHUB_I2S_OUT_CHMAP0(tdm_num, 2),
								     0x54);
						} else {
							regmap_write(regmap,
								     SUNXI_AHUB_I2S_OUT_CHMAP0(tdm_num, 2),
								     0x76);
						}
					}
					if (channels > 6) {
						regmap_write(regmap,
							     SUNXI_AHUB_I2S_OUT_CHMAP0(tdm_num, 3),
							     0x54);
					}
				}
			}

			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 0),
					   0xf << I2S_OUT_SLOT_NUM,
					   0x1 << I2S_OUT_SLOT_NUM);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 0),
					   0xffff << I2S_OUT_SLOT_EN,
					   0x3 << I2S_OUT_SLOT_EN);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 1),
					   0xf << I2S_OUT_SLOT_NUM,
					   0x1 << I2S_OUT_SLOT_NUM);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 1),
					   0xffff << I2S_OUT_SLOT_EN,
					   0x3 << I2S_OUT_SLOT_EN);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 2),
					   0xf << I2S_OUT_SLOT_NUM,
					   0x1 << I2S_OUT_SLOT_NUM);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 2),
					   0xffff << I2S_OUT_SLOT_EN,
					   0x3 << I2S_OUT_SLOT_EN);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 3),
					   0xf << I2S_OUT_SLOT_NUM,
					   0x1 << I2S_OUT_SLOT_NUM);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 3),
					   0xffff << I2S_OUT_SLOT_EN,
					   0x3 << I2S_OUT_SLOT_EN);

		} else {
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, tx_pin),
					   0xf << I2S_OUT_SLOT_NUM,
					   (channels - 1) << I2S_OUT_SLOT_NUM);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, tx_pin),
					   0xffff << I2S_OUT_SLOT_EN,
					   channels_en[channels - 1] << I2S_OUT_SLOT_EN);
		}
	} else {
		/* apbifn channels */
		regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RX_CTL(apb_num),
				   0xf << APBIF_RX_CHAN_NUM,
				   (channels - 1) << APBIF_RX_CHAN_NUM);
		/* tdmn channels */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CHCFG(tdm_num),
				   0xf << I2S_CHCFG_RX_CHANNUM,
				   (channels - 1) << I2S_CHCFG_RX_CHANNUM);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_IN_SLOT(tdm_num),
				   0xf << I2S_IN_SLOT_NUM,
				   (channels - 1) << I2S_IN_SLOT_NUM);
	}

	sunxi_ahub_dam_ctrl(true, apb_num, tdm_num, channels);

	return 0;
}

static int sunxi_ahub_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	sunxi_ahub_dam_ctrl(false, ahub->dts.apb_num, ahub->dts.tdm_num, 0);

	return 0;
}

static void sunxi_ahub_dai_tx_route(struct sunxi_ahub *ahub, bool enable)
{
	struct regmap *regmap = NULL;
	unsigned int tdm_num, tx_pin;
	unsigned int apb_num;

	SND_LOG_DEBUG("%s\n", enable ? "on" : "off");

	regmap = ahub->mem.regmap;
	tdm_num = ahub->dts.tdm_num;
	tx_pin = ahub->dts.tx_pin;
	apb_num = ahub->dts.apb_num;

	if (enable)
		goto tx_route_enable;
	else
		goto tx_route_disable;

tx_route_enable:
	if (ahub->dts.dai_type != SUNXI_DAI_HDMI_TYPE) {
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   0x1 << (I2S_CTL_SDO0_EN + tx_pin),
				   0x1 << (I2S_CTL_SDO0_EN + tx_pin));
	}
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << I2S_CTL_TXEN, 0x1 << I2S_CTL_TXEN);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << I2S_CTL_OUT_MUTE, 0x0 << I2S_CTL_OUT_MUTE);
	/* start apbif tx */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TX_CTL(apb_num),
			   0x1 << APBIF_TX_START, 0x1 << APBIF_TX_START);
	/* enable tx drq */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TX_IRQ_CTL(apb_num),
			   0x1 << APBIF_TX_DRQ, 0x1 << APBIF_TX_DRQ);
	return;

tx_route_disable:
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << I2S_CTL_OUT_MUTE, 0x1 << I2S_CTL_OUT_MUTE);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << I2S_CTL_TXEN, 0x0 << I2S_CTL_TXEN);
	if (ahub->dts.dai_type != SUNXI_DAI_HDMI_TYPE) {
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   0x1 << (I2S_CTL_SDO0_EN + tx_pin),
				   0x0 << (I2S_CTL_SDO0_EN + tx_pin));
	}
	/* stop apbif tx */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TX_CTL(apb_num),
			   0x1 << APBIF_TX_START, 0x0 << APBIF_TX_START);
	/* disable tx drq */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TX_IRQ_CTL(apb_num),
			   0x1 << APBIF_TX_DRQ, 0x0 << APBIF_TX_DRQ);
	return;
}

static void sunxi_ahub_dai_rx_route(struct sunxi_ahub *ahub, bool enable)
{
	struct regmap *regmap = NULL;
	unsigned int tdm_num, rx_pin;
	unsigned int apb_num;

	SND_LOG_DEBUG("%s\n", enable ? "on" : "off");

	regmap = ahub->mem.regmap;
	tdm_num = ahub->dts.tdm_num;
	rx_pin = ahub->dts.rx_pin;
	apb_num = ahub->dts.apb_num;

	if (enable)
		goto rx_route_enable;
	else
		goto rx_route_disable;

rx_route_enable:
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << (I2S_CTL_SDI0_EN + rx_pin),
			   0x1 << (I2S_CTL_SDI0_EN + rx_pin));
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << I2S_CTL_RXEN, 0x1 << I2S_CTL_RXEN);
	/* start apbif rx */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RX_CTL(apb_num),
			   0x1 << APBIF_RX_START, 0x1 << APBIF_RX_START);
	/* enable rx drq */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RX_IRQ_CTL(apb_num),
			   0x1 << APBIF_RX_DRQ, 0x1 << APBIF_RX_DRQ);
	return;

rx_route_disable:
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << I2S_CTL_RXEN, 0x0 << I2S_CTL_RXEN);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << (I2S_CTL_SDI0_EN + rx_pin),
			   0x0 << (I2S_CTL_SDI0_EN + rx_pin));
	/* stop apbif rx */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RX_CTL(apb_num),
			   0x1 << APBIF_RX_START, 0x0 << APBIF_RX_START);
	/* disable rx drq */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RX_IRQ_CTL(apb_num),
			   0x1 << APBIF_RX_DRQ, 0x0 << APBIF_RX_DRQ);
	return;
}

static int sunxi_ahub_dai_trigger(struct snd_pcm_substream *substream,
				  int cmd,
				  struct snd_soc_dai *dai)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(ahub)) {
		SND_LOG_ERR("ahub is null.\n");
		return -ENOMEM;
	}

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_ahub_dai_tx_route(ahub, true);
		} else {
			sunxi_ahub_dai_rx_route(ahub, true);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_ahub_dai_tx_route(ahub, false);
		} else {
			sunxi_ahub_dai_rx_route(ahub, false);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_ahub_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int apb_num;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(ahub)) {
		SND_LOG_ERR("ahub is null.\n");
		return -ENOMEM;
	}
	regmap = ahub->mem.regmap;
	apb_num = ahub->dts.apb_num;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* clear txfifo */
		regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TXFIFO_CTL(apb_num),
				   0x1 << APBIF_TX_FTX, 0x1 << APBIF_TX_FTX);
		/* clear tx o/u irq */
		regmap_write(regmap, SUNXI_AHUB_APBIF_TX_IRQ_STA(apb_num),
			     (0x1 << APBIF_TX_OV_PEND) | (0x1 << APBIF_TX_EM_PEND));
		/* clear tx fifo cnt */
		regmap_write(regmap, SUNXI_AHUB_APBIF_TXFIFO_CNT(apb_num), 0);
	} else {
		/* clear rxfifo */
		regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RXFIFO_CTL(apb_num),
				   0x1 << APBIF_RX_FRX, 0x1 << APBIF_RX_FRX);
		/* clear rx o/u irq */
		regmap_write(regmap, SUNXI_AHUB_APBIF_RX_IRQ_STA(apb_num),
			     (0x1 << APBIF_RX_UV_PEND) | (0x1 << APBIF_RX_AV_PEND));
		/* clear rx fifo cnt */
		regmap_write(regmap, SUNXI_AHUB_APBIF_RXFIFO_CNT(apb_num), 0);
	}

	return 0;
}

static void sunxi_ahub_dai_shutdown(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num;

	SND_LOG_DEBUG("\n");

	regmap = ahub->mem.regmap;
	apb_num = ahub->dts.apb_num;
	tdm_num = ahub->dts.tdm_num;

	/* APBIF & I2S of RST and GAT */
	if (tdm_num > 3 || apb_num > 2) {
		SND_LOG_ERR("unspport tdm num or apbif num\n");
		return;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST,
				   0x1 << (APBIF_TXDIF0_RST - apb_num),
				   0x0 << (APBIF_TXDIF0_RST - apb_num));
		regmap_update_bits(regmap, SUNXI_AHUB_GAT,
				   0x1 << (APBIF_TXDIF0_GAT - apb_num),
				   0x0 << (APBIF_TXDIF0_GAT - apb_num));
	} else {
		regmap_update_bits(regmap, SUNXI_AHUB_RST,
				   0x1 << (APBIF_RXDIF0_RST - apb_num),
				   0x0 << (APBIF_RXDIF0_RST - apb_num));
		regmap_update_bits(regmap, SUNXI_AHUB_GAT,
				   0x1 << (APBIF_RXDIF0_GAT - apb_num),
				   0x0 << (APBIF_RXDIF0_GAT - apb_num));
	}
}

static struct snd_soc_dai_ops sunxi_ahub_dai_ops = {
	/* call by machine */
	.set_pll	= sunxi_ahub_dai_set_pll,	/* set pllclk */
	.set_sysclk	= sunxi_ahub_dai_set_sysclk,	/* set mclk */
	.set_bclk_ratio	= sunxi_ahub_dai_set_bclk_ratio,/* set bclk freq */
	.set_tdm_slot	= sunxi_ahub_dai_set_tdm_slot,	/* set slot num and width */
	.set_fmt	= sunxi_ahub_dai_set_fmt,	/* set tdm fmt */
	/* call by asoc */
	.startup	= sunxi_ahub_dai_startup,
	.hw_params	= sunxi_ahub_dai_hw_params,	/* set hardware params */
	.hw_free	= sunxi_ahub_dai_hw_free,
	.prepare	= sunxi_ahub_dai_prepare,	/* clean irq and fifo */
	.trigger	= sunxi_ahub_dai_trigger,	/* set drq */
	.shutdown	= sunxi_ahub_dai_shutdown,
};

static void snd_soc_sunxi_ahub_init(struct sunxi_ahub *ahub)
{
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num, tx_pin, rx_pin;
	unsigned int reg_val = 0;
	unsigned int rx_pin_map = 0;
	unsigned int tdm_to_apb = 0;
	unsigned int apb_to_tdm = 0;

	SND_LOG_DEBUG("\n");

	regmap = ahub->mem.regmap;
	apb_num = ahub->dts.apb_num;
	tdm_num = ahub->dts.tdm_num;
	tx_pin = ahub->dts.tx_pin;
	rx_pin = ahub->dts.rx_pin;

	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << I2S_CTL_GEN, 0x1 << I2S_CTL_GEN);
	regmap_update_bits(regmap, SUNXI_AHUB_RST,
			   0x1 << (I2S0_RST - tdm_num),
			   0x1 << (I2S0_RST - tdm_num));
	regmap_update_bits(regmap, SUNXI_AHUB_GAT,
			   0x1 << (I2S0_GAT - tdm_num),
			   0x1 << (I2S0_GAT - tdm_num));

	/* tdm tx channels map */
	regmap_write(regmap, SUNXI_AHUB_I2S_OUT_CHMAP0(tdm_num, tx_pin), 0x76543210);
	regmap_write(regmap, SUNXI_AHUB_I2S_OUT_CHMAP1(tdm_num, tx_pin), 0xFEDCBA98);

	/* tdm rx channels map */
	rx_pin_map = (rx_pin << 4) | (rx_pin << 12) | (rx_pin << 20) | (rx_pin << 28);
	reg_val = 0x03020100 | rx_pin_map;
	regmap_write(regmap, SUNXI_AHUB_I2S_IN_CHMAP0(tdm_num), reg_val);
	reg_val = 0x07060504 | rx_pin_map;
	regmap_write(regmap, SUNXI_AHUB_I2S_IN_CHMAP1(tdm_num), reg_val);
	reg_val = 0x0B0A0908 | rx_pin_map;
	regmap_write(regmap, SUNXI_AHUB_I2S_IN_CHMAP2(tdm_num), reg_val);
	reg_val = 0x0F0E0D0C | rx_pin_map;
	regmap_write(regmap, SUNXI_AHUB_I2S_IN_CHMAP3(tdm_num), reg_val);

	/* tdm tx & rx data fmt
	 * 1. MSB first
	 * 2. transfer 0 after each sample in each slot
	 * 3. linear PCM
	 */
	regmap_write(regmap, SUNXI_AHUB_I2S_FMT1(tdm_num), 0x30);

	/* apbif tx & rx data fmt
	 * 1. MSB first
	 * 2. trigger level tx -> 0x20, rx -> 0x40
	 */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TXFIFO_CTL(apb_num),
			   0x1 << APBIF_TX_TXIM, 0x0 << APBIF_TX_TXIM);
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TXFIFO_CTL(apb_num),
			   0x3f << APBIF_TX_LEVEL, 0x20 << APBIF_TX_LEVEL);
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RXFIFO_CTL(apb_num),
			   0x3 << APBIF_RX_RXOM, 0x0 << APBIF_RX_RXOM);
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_RXFIFO_CTL(apb_num),
			   0x7f << APBIF_RX_LEVEL, 0x40 << APBIF_RX_LEVEL);

	/* apbif <-> tdm */
	switch (tdm_num) {
	case 0:
		tdm_to_apb = APBIF_RX_I2S0_TXDIF;
		break;
	case 1:
		tdm_to_apb = APBIF_RX_I2S1_TXDIF;
		break;
	case 2:
		tdm_to_apb = APBIF_RX_I2S2_TXDIF;
		break;
	case 3:
		tdm_to_apb = APBIF_RX_I2S3_TXDIF;
		break;
	default:
		SND_LOG_ERR("unspport tdm num\n");
		return;
	}
	regmap_write(regmap, SUNXI_AHUB_APBIF_RXFIFO_CONT(apb_num), 0x1 << tdm_to_apb);

	switch (apb_num) {
	case 0:
		apb_to_tdm = I2S_RX_APBIF_TXDIF0;
		break;
	case 1:
		apb_to_tdm = I2S_RX_APBIF_TXDIF1;
		break;
	case 2:
		apb_to_tdm = I2S_RX_APBIF_TXDIF2;
		break;
	default:
		SND_LOG_ERR("unspport apb num\n");
		return;
	}
	regmap_write(regmap, SUNXI_AHUB_I2S_RXCONT(tdm_num), 0x1 << apb_to_tdm);

	/* default setting HDMIAUDIO clk from ahub */
	if (ahub->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
		regmap_write(regmap, SUNXI_AHUB_CTL, 0x1 << HDMI_SRC_SEL);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   1 << I2S_CTL_SDO0_EN, 1 << I2S_CTL_SDO0_EN);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   1 << I2S_CTL_SDO1_EN, 1 << I2S_CTL_SDO1_EN);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   1 << I2S_CTL_SDO2_EN, 1 << I2S_CTL_SDO2_EN);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   1 << I2S_CTL_SDO3_EN, 1 << I2S_CTL_SDO3_EN);
	}

	return;
}

static int sunxi_ahub_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, &ahub->playback_dma_param, &ahub->capture_dma_param);

	snd_soc_sunxi_ahub_init(ahub);

	return 0;
}

static int sunxi_ahub_dai_remove(struct snd_soc_dai *dai)
{
	struct sunxi_ahub *ahub = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num;

	SND_LOG_DEBUG("\n");

	regmap = ahub->mem.regmap;
	tdm_num = ahub->dts.tdm_num;

	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << I2S_CTL_GEN, 0x0 << I2S_CTL_GEN);
	regmap_update_bits(regmap, SUNXI_AHUB_RST,
			   0x1 << (I2S0_RST - tdm_num),
			   0x0 << (I2S0_RST - tdm_num));
	regmap_update_bits(regmap, SUNXI_AHUB_GAT,
			   0x1 << (I2S0_GAT - tdm_num),
			   0x0 << (I2S0_GAT - tdm_num));

	return 0;
}

static struct snd_soc_dai_driver sunxi_ahub_dai = {
	.name		= "ahub_plat",
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
};

static int sunxi_ahub_probe(struct snd_soc_component *component)
{
	int ret;
	struct sunxi_ahub *ahub = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_extparam_register_cb(component->card->name, EXTPARAM_ID_DAI_UCFMT,
					     sunxi_ahub_get_dai_ucfmt, (void *)ahub);
	if (ret) {
		SND_LOG_ERR("dai ucfmt callback register failed\n");
		return -EINVAL;
	}
	ret = snd_sunxi_extparam_register_cb(component->card->name, EXTPARAM_ID_HDMI_FMT,
					     sunxi_ahub_get_hdmi_fmt, (void *)ahub);
	if (ret) {
		SND_LOG_ERR("hdmi fmt callback register failed\n");
		return -EINVAL;
	}

	return 0;
}

static void sunxi_ahub_remove(struct snd_soc_component *component)
{
	SND_LOG_DEBUG("\n");

	snd_sunxi_extparam_unregister_cb(component->card->name, EXTPARAM_ID_DAI_UCFMT,
					 sunxi_ahub_get_dai_ucfmt);
	snd_sunxi_extparam_unregister_cb(component->card->name, EXTPARAM_ID_HDMI_FMT,
					 sunxi_ahub_get_hdmi_fmt);
}

static int sunxi_ahub_suspend(struct snd_soc_component *component)
{
	struct sunxi_ahub *ahub = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num;

	SND_LOG_DEBUG("\n");

	regmap = ahub->mem.regmap;
	apb_num = ahub->dts.apb_num;
	tdm_num = ahub->dts.tdm_num;

	if (ahub->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   1 << I2S_CTL_SDO0_EN, 0 << I2S_CTL_SDO0_EN);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   1 << I2S_CTL_SDO1_EN, 0 << I2S_CTL_SDO1_EN);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   1 << I2S_CTL_SDO2_EN, 0 << I2S_CTL_SDO2_EN);
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   1 << I2S_CTL_SDO3_EN, 0 << I2S_CTL_SDO3_EN);
	}

	return 0;
}

static int sunxi_ahub_resume(struct snd_soc_component *component)
{
	struct sunxi_ahub *ahub = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	snd_soc_sunxi_ahub_init(ahub);

	return 0;
}

int sunxi_loopback_debug_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	unsigned int reg_val;
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct sunxi_ahub *ahub = snd_soc_component_get_drvdata(component);
	struct sunxi_ahub_mem *mem = &ahub->mem;
	struct sunxi_ahub_dts *dts = &ahub->dts;

	regmap_read(mem->regmap, SUNXI_AHUB_I2S_CTL(dts->tdm_num), &reg_val);
	ucontrol->value.integer.value[0] = ((reg_val & (1 << I2S_CTL_LOOP0)) ? 1 : 0);

	return 0;
}

int sunxi_loopback_debug_set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct sunxi_ahub *ahub = snd_soc_component_get_drvdata(component);
	struct sunxi_ahub_mem *mem = &ahub->mem;
	struct sunxi_ahub_dts *dts = &ahub->dts;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(mem->regmap,
				   SUNXI_AHUB_I2S_CTL(dts->tdm_num),
				   1 << I2S_CTL_LOOP0, 0 << I2S_CTL_LOOP0);
		break;
	case 1:
		regmap_update_bits(mem->regmap,
				   SUNXI_AHUB_I2S_CTL(dts->tdm_num),
				   1 << I2S_CTL_LOOP0, 1 << I2S_CTL_LOOP0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_kcontrol_new sunxi_ahub_controls[] = {
	SOC_SINGLE_EXT("loopback debug", SND_SOC_NOPM, 0, 1, 0,
		       sunxi_loopback_debug_get, sunxi_loopback_debug_set),
};

static struct snd_soc_component_driver sunxi_ahub_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_ahub_probe,
	.remove		= sunxi_ahub_remove,
	.suspend	= sunxi_ahub_suspend,
	.resume		= sunxi_ahub_resume,
	.controls	= sunxi_ahub_controls,
	.num_controls	= ARRAY_SIZE(sunxi_ahub_controls),
};

/*******************************************************************************
 * *** kernel source ***
 * @0 regmap (get from ahub_dam)
 * @1 clk (get from ahub_dam)
 * @2 regulator
 * @3 dts params
 ******************************************************************************/
static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_ahub_rglt *rglt)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	rglt->rglt_name = NULL;
	if (of_property_read_string(np, "ahub-regulator", &rglt->rglt_name)) {
		SND_LOG_DEBUG("regulator missing\n");
		rglt->ahub_rglt = NULL;
		return 0;
	}

	rglt->ahub_rglt = regulator_get(NULL, rglt->rglt_name);
	if (IS_ERR_OR_NULL(rglt->ahub_rglt)) {
		SND_LOG_ERR("get duaido vcc-pin failed\n");
		ret = -EFAULT;
		goto err_regulator_get;
	}
	ret = regulator_set_voltage(rglt->ahub_rglt, 3300000, 3300000);
	if (ret < 0) {
		SND_LOG_ERR("set duaido voltage failed\n");
		ret = -EFAULT;
		goto err_regulator_set_vol;
	}
	ret = regulator_enable(rglt->ahub_rglt);
	if (ret < 0) {
		SND_LOG_ERR("enable duaido vcc-pin failed\n");
		ret = -EFAULT;
		goto err_regulator_enable;
	}

	return 0;

err_regulator_enable:
err_regulator_set_vol:
	if (rglt->ahub_rglt)
		regulator_put(rglt->ahub_rglt);
err_regulator_get:
	return ret;
};

static void snd_sunxi_rglt_exit(struct platform_device *pdev, struct sunxi_ahub_rglt *rglt)
{
	SND_LOG_DEBUG("\n");

	if (rglt->ahub_rglt)
		if (!IS_ERR_OR_NULL(rglt->ahub_rglt)) {
			regulator_disable(rglt->ahub_rglt);
			regulator_put(rglt->ahub_rglt);
		}
}

static int snd_sunxi_pin_init(struct platform_device *pdev, struct sunxi_ahub_pinctl *pin)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	if (of_property_read_bool(np, "pinctrl-used")) {
		pin->pinctrl_used = 1;
	} else {
		pin->pinctrl_used = 0;
		SND_LOG_DEBUG("unused pinctrl\n");
		return 0;
	}

	pin->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin->pinctrl)) {
		SND_LOG_ERR("pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin->pinstate = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin->pinstate)) {
		SND_LOG_ERR("pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin->pinstate_sleep = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin->pinstate_sleep)) {
		SND_LOG_ERR("pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin->pinctrl, pin->pinstate);
	if (ret < 0) {
		SND_LOG_ERR("i2s set pinctrl default state fail\n");
		ret = -EBUSY;
		goto err_pinctrl_select_default;
	}

	return 0;

err_pinctrl_select_default:
err_loopup_pin_sleep:
err_loopup_pinstate:
	devm_pinctrl_put(pin->pinctrl);
	return ret;
}

static void snd_sunxi_pin_exit(struct platform_device *pdev, struct sunxi_ahub_pinctl *pin)
{
	SND_LOG_DEBUG("\n");

	if (pin->pinctrl_used)
		devm_pinctrl_put(pin->pinctrl);
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_ahub_dts *dts)
{
	int ret = 0;
	unsigned int temp_val = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* get dma params */
	ret = of_property_read_u32(np, "playback-cma", &temp_val);
	if (ret < 0) {
		dts->playback_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("playback-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts->playback_cma = temp_val;
	}
	ret = of_property_read_u32(np, "capture-cma", &temp_val);
	if (ret != 0) {
		dts->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("capture-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts->capture_cma = temp_val;
	}
	ret = of_property_read_u32(np, "tx-fifo-size", &temp_val);
	if (ret != 0) {
		dts->playback_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("tx-fifo-size miss, using default value\n");
	} else {
		dts->playback_fifo_size = temp_val;
	}
	ret = of_property_read_u32(np, "rx-fifo-size", &temp_val);
	if (ret != 0) {
		dts->capture_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("rx-fifo-size miss,using default value\n");
	} else {
		dts->capture_fifo_size = temp_val;
	}

	/* get tdm fmt of apb_num & tdm_num & tx/rx_pin */
	ret = of_property_read_u32(np, "apb-num", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("apb-num config missing\n");
		dts->apb_num = 0;
	} else {
		if (temp_val > 2) {	/* APBIFn (n = 0~2) */
			dts->apb_num = 0;
			SND_LOG_WARN("apb-num config invalid\n");
		} else {
			dts->apb_num = temp_val;
		}
	}
	ret = of_property_read_u32(np, "tdm-num", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("tdm-num config missing\n");
		dts->tdm_num = 0;
	} else {
		if (temp_val > 3) {	/* I2Sn (n = 0~3) */
			dts->tdm_num = 0;
			SND_LOG_WARN("tdm-num config invalid\n");
		} else {
			dts->tdm_num = temp_val;
		}
	}
	ret = of_property_read_u32(np, "tx-pin", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("tx-pin config missing\n");
		dts->tx_pin = 0;
	} else {
		if (temp_val > 3) {	/* I2S_DOUTn (n = 0~3) */
			dts->tx_pin = 0;
			SND_LOG_WARN("tx-pin config invalid\n");
		} else {
			dts->tx_pin = temp_val;
		}
	}
	ret = of_property_read_u32(np, "rx-pin", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("rx-pin config missing\n");
		dts->rx_pin = 0;
	} else {
		if (temp_val > 3) {	/* I2S_DINTn (n = 0~3) */
			dts->rx_pin = 0;
			SND_LOG_WARN("rx-pin config invalid\n");
		} else {
			dts->rx_pin = temp_val;
		}
	}
	ret = snd_sunxi_hdmi_get_dai_type(np, &dts->dai_type);
	if (ret)
		dts->dai_type = SUNXI_DAI_I2S_TYPE;

	if (dts->dai_type == SUNXI_DAI_HDMI_TYPE)
		SND_LOG_DEBUG("dai-type     : HDMI\n");
	else
		SND_LOG_DEBUG("dai-type     : I2S\n");
	SND_LOG_DEBUG("playback-cma : %zu\n", dts->playback_cma);
	SND_LOG_DEBUG("capture-cma  : %zu\n", dts->capture_cma);
	SND_LOG_DEBUG("tx-fifo-size : %zu\n", dts->playback_fifo_size);
	SND_LOG_DEBUG("rx-fifo-size : %zu\n", dts->capture_fifo_size);
	SND_LOG_DEBUG("apb-num      : %u\n", dts->apb_num);
	SND_LOG_DEBUG("tdm-num      : %u\n", dts->tdm_num);
	SND_LOG_DEBUG("tx-pin       : %u\n", dts->tx_pin);
	SND_LOG_DEBUG("rx-pin       : %u\n", dts->rx_pin);
};

static void snd_sunxi_dma_params_init(struct sunxi_ahub *ahub)
{
	struct resource *res = ahub->mem.res;
	struct sunxi_ahub_dts *dts = &ahub->dts;

	SND_LOG_DEBUG("\n");

	ahub->playback_dma_param.src_maxburst = 4;
	ahub->playback_dma_param.dst_maxburst = 4;
	ahub->playback_dma_param.dma_addr = res->start + SUNXI_AHUB_APBIF_TXFIFO(dts->apb_num);
	ahub->playback_dma_param.cma_kbytes = dts->playback_cma;
	ahub->playback_dma_param.fifo_size = dts->playback_fifo_size;
	ahub->playback_dma_param.hdmi_fmt = HDMI_FMT_PCM;

	ahub->capture_dma_param.src_maxburst = 4;
	ahub->capture_dma_param.dst_maxburst = 4;
	ahub->capture_dma_param.dma_addr = res->start + SUNXI_AHUB_APBIF_RXFIFO(dts->apb_num);
	ahub->capture_dma_param.cma_kbytes = dts->capture_cma;
	ahub->capture_dma_param.fifo_size = dts->capture_fifo_size;
};

static int sunxi_ahub_dev_probe(struct platform_device *pdev)
{
	struct sunxi_adapt_dai_ops_priv priv;
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_ahub *ahub = NULL;
	struct sunxi_ahub_mem *mem = NULL;
	struct sunxi_ahub_clk *clk = NULL;
	struct sunxi_ahub_pinctl *pin = NULL;
	struct sunxi_ahub_dts *dts = NULL;
	struct sunxi_ahub_rglt *rglt = NULL;

	SND_LOG_DEBUG("\n");

	ahub = devm_kzalloc(&pdev->dev, sizeof(*ahub), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ahub)) {
		SND_LOG_ERR("alloc sunxi_ahub failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(&pdev->dev, ahub);
	ahub->dev = &pdev->dev;
	mem = &ahub->mem;
	clk = &ahub->clk;
	pin = &ahub->pin;
	dts = &ahub->dts;
	rglt = &ahub->rglt;

	ret = snd_sunxi_ahub_mem_get(mem);
	if (ret) {
		SND_LOG_ERR("remap get failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_ahub_mem_get;
	}

	ret = snd_sunxi_ahub_clk_get(clk);
	if (ret) {
		SND_LOG_ERR("clk get failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_ahub_clk_get;
	}

	ret = snd_sunxi_rglt_init(pdev, rglt);
	if (ret) {
		SND_LOG_ERR("regulator init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_rglt_init;
	}

	snd_sunxi_dts_params_init(pdev, dts);

	if (dts->dai_type != SUNXI_DAI_HDMI_TYPE) {
		ret = snd_sunxi_pin_init(pdev, pin);
		if (ret) {
			SND_LOG_ERR("pinctrl init failed\n");
			ret = -EINVAL;
			goto err_snd_sunxi_pin_init;
		}
	}

	snd_sunxi_dma_params_init(ahub);

	priv.probe = sunxi_ahub_dai_probe;
	priv.remove = sunxi_ahub_dai_remove;
	sunxi_adpt_set_dai_ops(&sunxi_ahub_dai, &sunxi_ahub_dai_ops, &priv);

	ret = snd_soc_register_component(&pdev->dev, &sunxi_ahub_dev, &sunxi_ahub_dai, 1);
	if (ret) {
		SND_LOG_ERR("component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_sunxi_dma_platform_register(&pdev->dev);
	if (ret) {
		SND_LOG_ERR("register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_platform_register;
	}

	SND_LOG_DEBUG("register ahub platform success\n");

	return 0;

err_snd_sunxi_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
	snd_sunxi_pin_exit(pdev, pin);
err_snd_sunxi_pin_init:
	snd_sunxi_rglt_exit(pdev, rglt);
err_snd_sunxi_rglt_init:
err_snd_sunxi_ahub_clk_get:
err_snd_sunxi_ahub_mem_get:
	devm_kfree(&pdev->dev, ahub);
err_devm_kzalloc:
	of_node_put(np);
	return ret;
}

static int sunxi_ahub_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_ahub *ahub = dev_get_drvdata(dev);
	struct sunxi_ahub_pinctl *pin = &ahub->pin;
	struct sunxi_ahub_rglt *rglt = &ahub->rglt;

	SND_LOG_DEBUG("\n");

	snd_sunxi_dma_platform_unregister(dev);

	snd_soc_unregister_component(dev);

	snd_sunxi_pin_exit(pdev, pin);
	snd_sunxi_rglt_exit(pdev, rglt);

	devm_kfree(dev, ahub);
	of_node_put(np);

	SND_LOG_DEBUG("unregister ahub platform success\n");

	return 0;
}

static const struct of_device_id sunxi_ahub_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_ahub_of_match);

static struct platform_driver sunxi_ahub_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_ahub_of_match,
	},
	.probe	= sunxi_ahub_dev_probe,
	.remove	= sunxi_ahub_dev_remove,
};

int __init sunxi_ahub_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_ahub_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_ahub_dev_exit(void)
{
	platform_driver_unregister(&sunxi_ahub_driver);
}

late_initcall(sunxi_ahub_dev_init);
module_exit(sunxi_ahub_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.2");
MODULE_DESCRIPTION("sunxi soundcard platform of ahub");

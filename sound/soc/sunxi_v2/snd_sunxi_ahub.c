/*
 * sound\soc\sunxi\snd_sunxi_ahub.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
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
#include <sound/dmaengine_pcm.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_ahub.h"

#define HLOG		"AHUB"
#define DRV_NAME	"sunxi-snd-plat-ahub"

static int sunxi_ahub_dai_startup(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num;

	SND_LOG_DEBUG(HLOG, "\n");

	regmap = ahub_info->mem_info.regmap;
	apb_num = ahub_info->dts_info.apb_num;
	tdm_num = ahub_info->dts_info.tdm_num;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_dai_set_dma_data(dai, substream,
					 &ahub_info->playback_dma_param);
	} else {
		snd_soc_dai_set_dma_data(dai, substream,
					 &ahub_info->capture_dma_param);
	}

	/* APBIF & I2S of RST and GAT */
	if (tdm_num > 3 || apb_num > 2) {
		SND_LOG_ERR(HLOG, "unspport tdm num or apbif num\n");
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
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct sunxi_ahub_clk_info *clk_info = NULL;

	SND_LOG_DEBUG(HLOG, "stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out);

	if (IS_ERR_OR_NULL(ahub_info)) {
		SND_LOG_ERR(HLOG, "ahub_info is null.\n");
		return -ENOMEM;
	}
	clk_info = &ahub_info->clk_info;

	if (freq_in > 24576000) {
		//if (clk_set_parent(clk_info->clk_module, clk_info->clk_pllx4)) {
		//	SND_LOG_ERR(HLOG, "set parent of clk_module to pllx4 failed\n");
		//	return -EINVAL;
		//}

		if (clk_set_rate(clk_info->clk_pll, freq_in)) {
			SND_LOG_ERR(HLOG, "freq : %u pllx4 clk unsupport\n", freq_in);
			return -EINVAL;
		}
	} else {
		//if (clk_set_parent(clk_info->clk_module, clk_info->clk_pll)) {
		//	SND_LOG_ERR(HLOG, "set parent of clk_module to pll failed\n");
		//	return -EINVAL;
		//}
		if (clk_set_rate(clk_info->clk_pll, freq_in)) {
			SND_LOG_ERR(HLOG, "freq : %u pll clk unsupport\n", freq_in);
			return -EINVAL;
		}
	}
	if (clk_set_rate(clk_info->clk_module, freq_out / 2)) {
		SND_LOG_ERR(HLOG, "freq : %u module clk unsupport\n", freq_out);
		return -EINVAL;
	}

	ahub_info->pllclk_freq = freq_in;
	ahub_info->mclk_freq = freq_out;

	return 0;
}

static int sunxi_ahub_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,
				     unsigned int freq, int dir)
{
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num;
	unsigned int mclk_ratio, mclk_ratio_map;

	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(ahub_info)) {
		SND_LOG_ERR(HLOG, "ahub_info is null.\n");
		return -ENOMEM;
	}
	regmap = ahub_info->mem_info.regmap;
	tdm_num = ahub_info->dts_info.tdm_num;

	if (freq == 0) {
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CLKD(tdm_num),
				   0x1 << I2S_CLKD_MCLK, 0x0 << I2S_CLKD_MCLK);
		SND_LOG_DEBUG(HLOG, "mclk freq: 0\n");
		return 0;
	}
	if (ahub_info->pllclk_freq == 0) {
		SND_LOG_ERR(HLOG, "pllclk freq is invalid\n");
		return -ENOMEM;
	}
	mclk_ratio = ahub_info->pllclk_freq / freq;

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
		SND_LOG_ERR(HLOG, "mclk freq div unsupport\n");
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
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num;
	unsigned int bclk_ratio;

	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(ahub_info)) {
		SND_LOG_ERR(HLOG, "ahub_info is null.\n");
		return -ENOMEM;
	}
	regmap = ahub_info->mem_info.regmap;
	tdm_num = ahub_info->dts_info.tdm_num;

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
		SND_LOG_ERR(HLOG, "bclk freq div unsupport\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CLKD(tdm_num),
			   0xf << I2S_CLKD_BCLKDIV,
			   bclk_ratio << I2S_CLKD_BCLKDIV);

	return 0;
}

static int sunxi_ahub_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num, tx_pin, rx_pin;
	unsigned int mode, offset;
	unsigned int lrck_polarity, brck_polarity;

	SND_LOG_DEBUG(HLOG, "\n");

	ahub_info->fmt = fmt;

	if (IS_ERR_OR_NULL(ahub_info)) {
		SND_LOG_ERR(HLOG, "ahub_info is null.\n");
		return -ENOMEM;
	}
	regmap = ahub_info->mem_info.regmap;
	tdm_num = ahub_info->dts_info.tdm_num;
	tx_pin = ahub_info->dts_info.tx_pin;
	rx_pin = ahub_info->dts_info.rx_pin;

	/* set TDM format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case	SND_SOC_DAIFMT_I2S:
		mode = 1;
		offset = 1;
		break;
	case	SND_SOC_DAIFMT_RIGHT_J:
		mode = 2;
		offset = 0;
		break;
	case	SND_SOC_DAIFMT_LEFT_J:
		mode = 1;
		offset = 0;
		break;
	case	SND_SOC_DAIFMT_DSP_A:
		mode = 0;
		offset = 1;
		/* L data MSB after FRM LRC (short frame) */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
				   0x1 << I2S_FMT0_LRCK_WIDTH,
				   0x0 << I2S_FMT0_LRCK_WIDTH);
		break;
	case	SND_SOC_DAIFMT_DSP_B:
		mode = 0;
		offset = 0;
		/* L data MSB during FRM LRC (long frame) */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
				   0x1 << I2S_FMT0_LRCK_WIDTH,
				   0x1 << I2S_FMT0_LRCK_WIDTH);
		break;
	default:
		SND_LOG_ERR(HLOG, "format setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x3 << I2S_CTL_MODE, mode << I2S_CTL_MODE);
	/* regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, tx_pin),
	 *		   0x3 << I2S_OUT_OFFSET, offset << I2S_OUT_OFFSET);
	 */
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 0),
			   0x3 << I2S_OUT_OFFSET, offset << I2S_OUT_OFFSET);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 1),
			   0x3 << I2S_OUT_OFFSET, offset << I2S_OUT_OFFSET);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 2),
			   0x3 << I2S_OUT_OFFSET, offset << I2S_OUT_OFFSET);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, 3),
			   0x3 << I2S_OUT_OFFSET, offset << I2S_OUT_OFFSET);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_IN_SLOT(tdm_num),
			   0x3 << I2S_IN_OFFSET, offset << I2S_IN_OFFSET);

	/* set lrck & bclk polarity */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case	SND_SOC_DAIFMT_NB_NF:
		lrck_polarity = 0;
		brck_polarity = 0;
		break;
	case	SND_SOC_DAIFMT_NB_IF:
		lrck_polarity = 1;
		brck_polarity = 0;
		break;
	case	SND_SOC_DAIFMT_IB_NF:
		lrck_polarity = 0;
		brck_polarity = 1;
		break;
	case	SND_SOC_DAIFMT_IB_IF:
		lrck_polarity = 1;
		brck_polarity = 1;
		break;
	default:
		SND_LOG_ERR(HLOG, "invert clk setting failed\n");
		return -EINVAL;
	}
	if (((fmt & SND_SOC_DAIFMT_FORMAT_MASK) == SND_SOC_DAIFMT_DSP_A) ||
	    ((fmt & SND_SOC_DAIFMT_FORMAT_MASK) == SND_SOC_DAIFMT_DSP_B))
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
				   0x1 << I2S_FMT0_LRCK_POLARITY,
				   (lrck_polarity^1) << I2S_FMT0_LRCK_POLARITY);
	else
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
				   0x1 << I2S_FMT0_LRCK_POLARITY,
				   lrck_polarity << I2S_FMT0_LRCK_POLARITY);
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
			   0x1 << I2S_FMT0_BCLK_POLARITY,
			   brck_polarity << I2S_FMT0_BCLK_POLARITY);

	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case	SND_SOC_DAIFMT_CBM_CFM:
		/* lrck & bclk dir output */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   0x1 << I2S_CTL_CLK_OUT, 0x0 << I2S_CTL_CLK_OUT);
		break;
	case	SND_SOC_DAIFMT_CBS_CFS:
		/* lrck & bclk dir input */
		regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
				   0x1 << I2S_CTL_CLK_OUT, 0x1 << I2S_CTL_CLK_OUT);
		break;
	default:
		SND_LOG_ERR(HLOG, "unknown master/slave format\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_ahub_dai_set_tdm_slot(struct snd_soc_dai *dai,
				       unsigned int tx_mask, unsigned int rx_mask,
				       int slots, int slot_width)
{
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num, tx_pin, rx_pin;
	unsigned int slot_width_map, lrck_width_map;

	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(ahub_info)) {
		SND_LOG_ERR(HLOG, "ahub_info is null\n");
		return -ENOMEM;
	}
	regmap = ahub_info->mem_info.regmap;
	tdm_num = ahub_info->dts_info.tdm_num;
	tx_pin = ahub_info->dts_info.tx_pin;
	rx_pin = ahub_info->dts_info.rx_pin;

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
		SND_LOG_ERR(HLOG, "unknown slot width\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_FMT0(tdm_num),
			   0x7 << I2S_FMT0_SW, slot_width_map << I2S_FMT0_SW);

	/* bclk num of per channel
	 * I2S/RIGHT_J/LEFT_J	-> lrck long total is lrck_width_map * 2
	 * DSP_A/DAP_B		-> lrck long total is lrck_width_map * 1
	 */
	switch (ahub_info->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case    SND_SOC_DAIFMT_I2S:
	case    SND_SOC_DAIFMT_RIGHT_J:
	case    SND_SOC_DAIFMT_LEFT_J:
		slots /= 2;
		break;
	case    SND_SOC_DAIFMT_DSP_A:
	case    SND_SOC_DAIFMT_DSP_B:
		break;
	default:
		SND_LOG_ERR(HLOG, "unsupoort format\n");
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
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num, tx_pin, rx_pin;
	unsigned int channels;
	unsigned int channels_en[16] = {
		0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
		0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
	};

	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(ahub_info)) {
		SND_LOG_ERR(HLOG, "ahub_info is null.\n");
		return -ENOMEM;
	}
	regmap = ahub_info->mem_info.regmap;
	apb_num = ahub_info->dts_info.apb_num;
	tdm_num = ahub_info->dts_info.tdm_num;
	tx_pin = ahub_info->dts_info.tx_pin;
	rx_pin = ahub_info->dts_info.rx_pin;

	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		/* apbifn bits */
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap,
					   SUNXI_AHUB_APBIF_TX_CTL(apb_num),
					   0x7 << APBIF_TX_WS,
					   0x3 << APBIF_TX_WS);
			regmap_update_bits(regmap,
					   SUNXI_AHUB_APBIF_TXFIFO_CTL(apb_num),
					   0x1 << APBIF_TX_TXIM,
					   0x1 << APBIF_TX_TXIM);
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

		regmap_update_bits(regmap,
				   SUNXI_AHUB_I2S_FMT0(tdm_num),
				   0x7 << I2S_FMT0_SR,
				   0x3 << I2S_FMT0_SR);
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
		SND_LOG_ERR(HLOG, "unrecognized format bits\n");
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

		regmap_update_bits(regmap,
				   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, tx_pin),
				   0xf << I2S_OUT_SLOT_NUM,
				   (channels - 1) << I2S_OUT_SLOT_NUM);
		regmap_update_bits(regmap,
				   SUNXI_AHUB_I2S_OUT_SLOT(tdm_num, tx_pin),
				   0xffff << I2S_OUT_SLOT_EN,
				   channels_en[channels - 1] << I2S_OUT_SLOT_EN);
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

	return 0;
}

static void sunxi_ahub_dai_tx_route(struct sunxi_ahub_info *ahub_info,
				    bool enable)
{
	struct regmap *regmap = NULL;
	unsigned int tdm_num, tx_pin;
	unsigned int apb_num;

	SND_LOG_DEBUG(HLOG, "%s\n", enable ? "on" : "off");

	regmap = ahub_info->mem_info.regmap;
	tdm_num = ahub_info->dts_info.tdm_num;
	tx_pin = ahub_info->dts_info.tx_pin;
	apb_num = ahub_info->dts_info.apb_num;

	if (enable)
		goto tx_route_enable;
	else
		goto tx_route_disable;

tx_route_enable:
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << (I2S_CTL_SDO0_EN + tx_pin),
			   0x1 << (I2S_CTL_SDO0_EN + tx_pin));
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
	regmap_update_bits(regmap, SUNXI_AHUB_I2S_CTL(tdm_num),
			   0x1 << (I2S_CTL_SDO0_EN + tx_pin),
			   0x0 << (I2S_CTL_SDO0_EN + tx_pin));
	/* stop apbif tx */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TX_CTL(apb_num),
			   0x1 << APBIF_TX_START, 0x0 << APBIF_TX_START);
	/* disable tx drq */
	regmap_update_bits(regmap, SUNXI_AHUB_APBIF_TX_IRQ_CTL(apb_num),
			   0x1 << APBIF_TX_DRQ, 0x0 << APBIF_TX_DRQ);
	return;
}

static void sunxi_ahub_dai_rx_route(struct sunxi_ahub_info *ahub_info,
				    bool enable)
{
	struct regmap *regmap = NULL;
	unsigned int tdm_num, rx_pin;
	unsigned int apb_num;

	SND_LOG_DEBUG(HLOG, "%s\n", enable ? "on" : "off");

	regmap = ahub_info->mem_info.regmap;
	tdm_num = ahub_info->dts_info.tdm_num;
	rx_pin = ahub_info->dts_info.rx_pin;
	apb_num = ahub_info->dts_info.apb_num;

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
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(ahub_info)) {
		SND_LOG_ERR(HLOG, "ahub_info is null.\n");
		return -ENOMEM;
	}

	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_ahub_dai_tx_route(ahub_info, true);
		} else {
			sunxi_ahub_dai_rx_route(ahub_info, true);
		}
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_ahub_dai_tx_route(ahub_info, false);
		} else {
			sunxi_ahub_dai_rx_route(ahub_info, false);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_ahub_dai_prepare(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int apb_num;

	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(ahub_info)) {
		SND_LOG_ERR(HLOG, "ahub_info is null.\n");
		return -ENOMEM;
	}
	regmap = ahub_info->mem_info.regmap;
	apb_num = ahub_info->dts_info.apb_num;

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
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num;

	SND_LOG_DEBUG(HLOG, "\n");

	regmap = ahub_info->mem_info.regmap;
	apb_num = ahub_info->dts_info.apb_num;
	tdm_num = ahub_info->dts_info.tdm_num;

	/* APBIF & I2S of RST and GAT */
	if (tdm_num > 3 || apb_num > 2) {
		SND_LOG_ERR(HLOG, "unspport tdm num or apbif num\n");
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

static const struct snd_soc_dai_ops sunxi_ahub_dai_ops = {
	/* call by machine */
	.set_pll	= sunxi_ahub_dai_set_pll,	// set pllclk
	.set_sysclk	= sunxi_ahub_dai_set_sysclk,	// set mclk
	.set_bclk_ratio	= sunxi_ahub_dai_set_bclk_ratio,// set bclk freq
	.set_tdm_slot	= sunxi_ahub_dai_set_tdm_slot,	// set slot num and width
	.set_fmt	= sunxi_ahub_dai_set_fmt,	// set tdm fmt
	/* call by asoc */
	.startup	= sunxi_ahub_dai_startup,
	.hw_params	= sunxi_ahub_dai_hw_params,	// set hardware params
	.prepare	= sunxi_ahub_dai_prepare,	// clean irq and fifo
	.trigger	= sunxi_ahub_dai_trigger,	// set drq
	.shutdown	= sunxi_ahub_dai_shutdown,
};

static void snd_soc_sunxi_ahub_init(struct sunxi_ahub_info *ahub_info)
{
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num, tx_pin, rx_pin;
	unsigned int reg_val = 0;
	unsigned int rx_pin_map = 0;
	unsigned int tdm_to_apb = 0;
	unsigned int apb_to_tdm = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	regmap = ahub_info->mem_info.regmap;
	apb_num = ahub_info->dts_info.apb_num;
	tdm_num = ahub_info->dts_info.tdm_num;
	tx_pin = ahub_info->dts_info.tx_pin;
	rx_pin = ahub_info->dts_info.rx_pin;

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
	switch (tdm_num)
	{
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
		SND_LOG_ERR(HLOG, "unspport tdm num\n");
		return;
	}
	regmap_write(regmap, SUNXI_AHUB_APBIF_RXFIFO_CONT(apb_num), 0x1 << tdm_to_apb);

	switch (apb_num)
	{
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
		SND_LOG_ERR(HLOG, "unspport apb num\n");
		return;
	}
	regmap_write(regmap, SUNXI_AHUB_I2S_RXCONT(tdm_num), 0x1 << apb_to_tdm);

	return;
}

static int sunxi_ahub_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai,
				  &ahub_info->playback_dma_param,
				  &ahub_info->capture_dma_param);

	snd_soc_sunxi_ahub_init(ahub_info);

	return 0;
}

static int sunxi_ahub_dai_remove(struct snd_soc_dai *dai)
{
	struct sunxi_ahub_info *ahub_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = NULL;
	unsigned int tdm_num;

	SND_LOG_DEBUG(HLOG, "\n");

	regmap = ahub_info->mem_info.regmap;
	tdm_num = ahub_info->dts_info.tdm_num;

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
	.probe		= sunxi_ahub_dai_probe,
	.remove		= sunxi_ahub_dai_remove,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_ahub_dai_ops,
};

static int sunxi_ahub_probe(struct snd_soc_component *component)
{
	SND_LOG_DEBUG(HLOG, "\n");

	return 0;
}

static int sunxi_ahub_suspend(struct snd_soc_component *component)
{
	struct sunxi_ahub_info *ahub_info = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = NULL;
	unsigned int apb_num, tdm_num;

	SND_LOG_DEBUG(HLOG, "\n");

	regmap = ahub_info->mem_info.regmap;
	apb_num = ahub_info->dts_info.apb_num;
	tdm_num = ahub_info->dts_info.tdm_num;

	return 0;
}

static int sunxi_ahub_resume(struct snd_soc_component *component)
{
	struct sunxi_ahub_info *ahub_info = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG(HLOG, "\n");

	snd_soc_sunxi_ahub_init(ahub_info);

	return 0;
}

int sunxi_loopback_debug_get(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	unsigned int reg_val;
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct sunxi_ahub_info *ahub_info = snd_soc_component_get_drvdata(component);
	struct sunxi_ahub_mem_info *mem_info = &ahub_info->mem_info;
	struct sunxi_ahub_dts_info *dts_info = &ahub_info->dts_info;

	regmap_read(mem_info->regmap, SUNXI_AHUB_I2S_CTL(dts_info->tdm_num), &reg_val);
	ucontrol->value.integer.value[0] = ((reg_val & (1 << I2S_CTL_LOOP0)) ? 1 : 0);

	return 0;
}

int sunxi_loopback_debug_set(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct sunxi_ahub_info *ahub_info = snd_soc_component_get_drvdata(component);
	struct sunxi_ahub_mem_info *mem_info = &ahub_info->mem_info;
	struct sunxi_ahub_dts_info *dts_info = &ahub_info->dts_info;

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		regmap_update_bits(mem_info->regmap,
				   SUNXI_AHUB_I2S_CTL(dts_info->tdm_num),
				   1 << I2S_CTL_LOOP0, 0 << I2S_CTL_LOOP0);
		break;
	case	1:
		regmap_update_bits(mem_info->regmap,
				   SUNXI_AHUB_I2S_CTL(dts_info->tdm_num),
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

static struct snd_soc_component_driver sunxi_ahub_component = {
	.name		= DRV_NAME,
	.probe		= sunxi_ahub_probe,
	.suspend	= sunxi_ahub_suspend,
	.resume		= sunxi_ahub_resume,
	.controls	= sunxi_ahub_controls,
	.num_controls	= ARRAY_SIZE(sunxi_ahub_controls),
};

/*******************************************************************************
 * for kernel source
 ******************************************************************************/
static int snd_soc_sunxi_ahub_pin_init(struct platform_device *pdev,
				       struct device_node *np,
				       struct sunxi_ahub_pinctl_info *pin_info)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	if (of_property_read_bool(np, "pinctrl_used")) {
		pin_info->pinctrl_used = 1;
	} else {
		pin_info->pinctrl_used = 0;
		SND_LOG_DEBUG(HLOG, "unused pinctrl\n");
		return 0;
	}

	pin_info->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin_info->pinctrl)) {
		SND_LOG_ERR(HLOG, "pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin_info->pinstate = pinctrl_lookup_state(pin_info->pinctrl,
						  PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin_info->pinstate)) {
		SND_LOG_ERR(HLOG, "pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin_info->pinstate_sleep = pinctrl_lookup_state(pin_info->pinctrl,
							PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin_info->pinstate_sleep)) {
		SND_LOG_ERR(HLOG, "pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "daudio set pinctrl default state fail\n");
		ret = -EBUSY;
		goto err_pinctrl_select_default;
	}

	return 0;

err_pinctrl_select_default:
err_loopup_pin_sleep:
err_loopup_pinstate:
	devm_pinctrl_put(pin_info->pinctrl);
	return ret;
}

static int snd_soc_sunxi_ahub_dts_params_init(struct platform_device *pdev,
					      struct device_node *np,
					      struct sunxi_ahub_dts_info *dts_info)
{
	int ret = 0;
	unsigned int temp_val = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get tdm fmt of apb_num & tdm_num & tx/rx_pin */
	ret = of_property_read_u32(np, "apb_num", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "apb_num config missing\n");
		dts_info->apb_num = 0;
	} else {
		if (temp_val > 2) {	/* APBIFn (n = 0~2) */
			dts_info->apb_num = 0;
			SND_LOG_WARN(HLOG, "apb_num config invalid\n");
		} else {
			dts_info->apb_num = temp_val;
		}
	}
	ret = of_property_read_u32(np, "tdm_num", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "tdm_num config missing\n");
		dts_info->tdm_num = 0;
	} else {
		if (temp_val > 3) {	/* I2Sn (n = 0~3) */
			dts_info->tdm_num = 0;
			SND_LOG_WARN(HLOG, "tdm_num config invalid\n");
		} else {
			dts_info->tdm_num = temp_val;
		}
	}
	ret = of_property_read_u32(np, "tx_pin", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "tx_pin config missing\n");
		dts_info->tx_pin = 0;
	} else {
		if (temp_val > 3) {	/* I2S_DOUTn (n = 0~3) */
			dts_info->tx_pin = 0;
			SND_LOG_WARN(HLOG, "tx_pin config invalid\n");
		} else {
			dts_info->tx_pin = temp_val;
		}
	}
	ret = of_property_read_u32(np, "rx_pin", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "rx_pin config missing\n");
		dts_info->rx_pin = 0;
	} else {
		if (temp_val > 3) {	/* I2S_DINTn (n = 0~3) */
			dts_info->rx_pin = 0;
			SND_LOG_WARN(HLOG, "rx_pin config invalid\n");
		} else {
			dts_info->rx_pin = temp_val;
		}
	}

	SND_LOG_DEBUG(HLOG, "playback_cma : %lu\n", dts_info->playback_cma);
	SND_LOG_DEBUG(HLOG, "capture_cma  : %lu\n", dts_info->capture_cma);
	SND_LOG_DEBUG(HLOG, "tx_fifo_size : %lu\n", dts_info->playback_fifo_size);
	SND_LOG_DEBUG(HLOG, "rx_fifo_size : %lu\n", dts_info->capture_fifo_size);
	SND_LOG_DEBUG(HLOG, "apb_num      : %u\n", dts_info->apb_num);
	SND_LOG_DEBUG(HLOG, "tdm_num      : %u\n", dts_info->tdm_num);
	SND_LOG_DEBUG(HLOG, "tx_pin       : %u\n", dts_info->tx_pin);
	SND_LOG_DEBUG(HLOG, "rx_pin       : %u\n", dts_info->rx_pin);

	return 0;
};

static int snd_soc_sunxi_ahub_regulator_init(struct platform_device *pdev,
					     struct device_node *np,
					     struct sunxi_ahub_regulator_info *regulator_info)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	regulator_info->regulator_name = NULL;
	if (of_property_read_string(np, "ahub_regulator", &regulator_info->regulator_name)) {
		SND_LOG_DEBUG(HLOG, "regulator missing\n");
		regulator_info->regulator = NULL;
		return 0;
	}

	regulator_info->regulator = regulator_get(NULL, regulator_info->regulator_name);
	if (IS_ERR_OR_NULL(regulator_info->regulator)) {
		SND_LOG_ERR(HLOG, "get duaido vcc-pin failed\n");
		ret = -EFAULT;
		goto err_regulator_get;
	}
	ret = regulator_set_voltage(regulator_info->regulator, 3300000, 3300000);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "set duaido voltage failed\n");
		ret = -EFAULT;
		goto err_regulator_set_vol;
	}
	ret = regulator_enable(regulator_info->regulator);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "enable duaido vcc-pin failed\n");
		ret = -EFAULT;
		goto err_regulator_enable;
	}

	return 0;

err_regulator_enable:
err_regulator_set_vol:
	if (regulator_info->regulator)
		regulator_put(regulator_info->regulator);
err_regulator_get:
	return ret;
};

static void snd_soc_sunxi_dma_params_init(struct sunxi_ahub_info *ahub_info)
{
	struct resource *res = ahub_info->mem_info.res;
	struct sunxi_ahub_dts_info *dts_info = &ahub_info->dts_info;

	SND_LOG_DEBUG(HLOG, "\n");

	ahub_info->playback_dma_param.addr =
		res->start + SUNXI_AHUB_APBIF_TXFIFO(dts_info->apb_num);
	ahub_info->playback_dma_param.maxburst = 8;
	ahub_info->playback_dma_param.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

	ahub_info->capture_dma_param.addr =
		res->start + SUNXI_AHUB_APBIF_RXFIFO(dts_info->apb_num);
	ahub_info->capture_dma_param.maxburst = 8;
	ahub_info->capture_dma_param.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
};

static int sunxi_ahub_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_ahub_info *ahub_info = NULL;
	struct sunxi_ahub_mem_info *mem_info = NULL;
	struct sunxi_ahub_clk_info *clk_info = NULL;
	struct sunxi_ahub_pinctl_info *pin_info = NULL;
	struct sunxi_ahub_dts_info *dts_info = NULL;
	struct sunxi_ahub_regulator_info *regulator_info = NULL;

	SND_LOG_DEBUG(HLOG, "\n");

	ahub_info = devm_kzalloc(&pdev->dev,
				  sizeof(struct sunxi_ahub_info),
				  GFP_KERNEL);
	if (IS_ERR_OR_NULL(ahub_info)) {
		SND_LOG_ERR(HLOG, "alloc sunxi_ahub_info failed\n");
		ret = -ENOMEM;
		goto err_devm_malloc_sunxi_daudio;
	}
	dev_set_drvdata(&pdev->dev, ahub_info);
	ahub_info->dev = &pdev->dev;
	mem_info = &ahub_info->mem_info;
	clk_info = &ahub_info->clk_info;
	pin_info = &ahub_info->pin_info;
	dts_info = &ahub_info->dts_info;
	regulator_info = &ahub_info->regulator_info;

	ret = snd_soc_sunxi_ahub_mem_get(mem_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "remap get failed\n");
		ret = -EINVAL;
		goto err_snd_soc_sunxi_ahub_mem_get;
	}

	ret = snd_soc_sunxi_ahub_clk_get(clk_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk get failed\n");
		ret = -EINVAL;
		goto err_snd_soc_sunxi_ahub_clk_get;
	}

	ret = snd_soc_sunxi_ahub_dts_params_init(pdev, np, dts_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "dts init failed\n");
		ret = -EINVAL;
		goto err_snd_soc_sunxi_ahub_dts_params_init;
	}

	ret = snd_soc_sunxi_ahub_pin_init(pdev, np, pin_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "pinctrl init failed\n");
		ret = -EINVAL;
		goto err_snd_soc_sunxi_ahub_pin_init;
	}

	ret = snd_soc_sunxi_ahub_regulator_init(pdev, np, regulator_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "regulator_info init failed\n");
		ret = -EINVAL;
		goto err_snd_soc_sunxi_ahub_regulator_init;
	}

	snd_soc_sunxi_dma_params_init(ahub_info);

	ret = snd_soc_register_component(&pdev->dev,
					 &sunxi_ahub_component,
					 &sunxi_ahub_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

        ret = devm_snd_dmaengine_pcm_register(&pdev->dev, NULL, 0);
        if (ret) {
		SND_LOG_ERR(HLOG, "register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_sunxi_dma_platform_register;
        }

	SND_LOG_DEBUG(HLOG, "register ahub platform success\n");

	return 0;

err_snd_soc_sunxi_dma_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
err_snd_soc_sunxi_ahub_regulator_init:
err_snd_soc_sunxi_ahub_dts_params_init:
err_snd_soc_sunxi_ahub_pin_init:
err_snd_soc_sunxi_ahub_clk_get:
err_snd_soc_sunxi_ahub_mem_get:
	devm_kfree(&pdev->dev, ahub_info);
err_devm_malloc_sunxi_daudio:
	of_node_put(np);
	return ret;
}

static int sunxi_ahub_dev_remove(struct platform_device *pdev)
{
	struct sunxi_ahub_info *ahub_info = dev_get_drvdata(&pdev->dev);
	struct sunxi_ahub_pinctl_info *pin_info = &ahub_info->pin_info;
	struct sunxi_ahub_regulator_info *regulator_info = &ahub_info->regulator_info;

	SND_LOG_DEBUG(HLOG, "\n");

	snd_soc_unregister_component(&pdev->dev);

	if (regulator_info->regulator) {
		if (!IS_ERR_OR_NULL(regulator_info->regulator)) {
			regulator_disable(regulator_info->regulator);
			regulator_put(regulator_info->regulator);
		}
	}
	if (pin_info->pinctrl_used) {
		devm_pinctrl_put(pin_info->pinctrl);
	}

	devm_kfree(&pdev->dev, ahub_info);

	SND_LOG_DEBUG(HLOG, "unregister ahub platform success\n");

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
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
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
MODULE_DESCRIPTION("sunxi soundcard platform of ahub");

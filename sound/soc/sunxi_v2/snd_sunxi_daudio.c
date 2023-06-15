/*
 * sound\soc\sunxi\snd_sunxi_daudio.c
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
#include <linux/platform_device.h>
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
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_daudio.h"

#define HLOG		"DAUDIO"
#define DRV_NAME	"sunxi-snd-plat-daudio"

/* for reg debug */
#define REG_LABEL(constant)	{#constant, constant, 0}
#define REG_LABEL_END		{NULL, 0, 0}

struct audio_reg_label {
	const char *name;
	const unsigned int address;
	unsigned int value;
};

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DAUDIO_CTL),
	REG_LABEL(SUNXI_DAUDIO_FMT0),
	REG_LABEL(SUNXI_DAUDIO_FMT1),
	REG_LABEL(SUNXI_DAUDIO_INTSTA),
	/* REG_LABEL(SUNXI_DAUDIO_RXFIFO), */
	REG_LABEL(SUNXI_DAUDIO_FIFOCTL),
	REG_LABEL(SUNXI_DAUDIO_FIFOSTA),
	REG_LABEL(SUNXI_DAUDIO_INTCTL),
	/* REG_LABEL(SUNXI_DAUDIO_TXFIFO), */
	REG_LABEL(SUNXI_DAUDIO_CLKDIV),
	REG_LABEL(SUNXI_DAUDIO_TXCNT),
	REG_LABEL(SUNXI_DAUDIO_RXCNT),
	REG_LABEL(SUNXI_DAUDIO_CHCFG),
	REG_LABEL(SUNXI_DAUDIO_TX0CHSEL),
	REG_LABEL(SUNXI_DAUDIO_TX1CHSEL),
	REG_LABEL(SUNXI_DAUDIO_TX2CHSEL),
	REG_LABEL(SUNXI_DAUDIO_TX3CHSEL),
	REG_LABEL(SUNXI_DAUDIO_TX0CHMAP0),
	REG_LABEL(SUNXI_DAUDIO_TX0CHMAP1),
	REG_LABEL(SUNXI_DAUDIO_TX1CHMAP0),
	REG_LABEL(SUNXI_DAUDIO_TX1CHMAP1),
	REG_LABEL(SUNXI_DAUDIO_TX2CHMAP0),
	REG_LABEL(SUNXI_DAUDIO_TX2CHMAP1),
	REG_LABEL(SUNXI_DAUDIO_TX3CHMAP0),
	REG_LABEL(SUNXI_DAUDIO_TX3CHMAP1),
	REG_LABEL(SUNXI_DAUDIO_RXCHSEL),
	REG_LABEL(SUNXI_DAUDIO_RXCHMAP0),
	REG_LABEL(SUNXI_DAUDIO_RXCHMAP1),
	REG_LABEL(SUNXI_DAUDIO_RXCHMAP2),
	REG_LABEL(SUNXI_DAUDIO_RXCHMAP3),
	REG_LABEL(SUNXI_DAUDIO_DEBUG),
	REG_LABEL(SUNXI_DAUDIO_REV),
	REG_LABEL_END,
};

static struct regmap_config g_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DAUDIO_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static int snd_sunxi_save_reg(struct regmap *regmap, struct audio_reg_label *reg_labels);
static int snd_sunxi_echo_reg(struct regmap *regmap, struct audio_reg_label *reg_labels);
static int snd_sunxi_clk_init(struct platform_device *pdev,
			      struct sunxi_daudio_clk_info *clk_info);
static void snd_sunxi_clk_exit(struct platform_device *pdev,
			       struct sunxi_daudio_clk_info *clk_info);
static int snd_sunxi_clk_enable(struct platform_device *pdev,
				struct sunxi_daudio_clk_info *clk_info);
static void snd_sunxi_clk_disable(struct platform_device *pdev,
				  struct sunxi_daudio_clk_info *clk_info);
static int snd_sunxi_regulator_init(struct platform_device *pdev,
				    struct sunxi_daudio_regulator_info *rglt_info);
static void snd_sunxi_regulator_exit(struct platform_device *pdev,
				     struct sunxi_daudio_regulator_info *rglt_info);
static int snd_sunxi_regulator_enable(struct platform_device *pdev,
				      struct sunxi_daudio_regulator_info *rglt_info);
static void snd_sunxi_regulator_disable(struct platform_device *pdev,
					struct sunxi_daudio_regulator_info *rglt_info);

static void sunxi_rx_sync_enable(void *data, bool enable);

static int sunxi_daudio_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
				    unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_clk_info *clk_info = &daudio_info->clk_info;

	SND_LOG_DEBUG(HLOG, "\n");

	if (clk_set_parent(clk_info->clk_i2s, clk_info->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "set parent of clk_i2s to pll failed\n");
		return -EINVAL;
	}
	if (clk_set_rate(clk_info->clk_pll_audio, freq_in)) {
		SND_LOG_ERR(HLOG, "freq : %u pll clk unsupport\n", freq_in);
		return -EINVAL;
	}

	if (clk_set_rate(clk_info->clk_i2s, freq_out)) {
		SND_LOG_ERR(HLOG, "freq : %u module clk unsupport\n", freq_out);
		return -EINVAL;
	}

	daudio_info->pllclk_freq = freq_in;
	daudio_info->moduleclk_freq = freq_out;

	return 0;
}

static int sunxi_daudio_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,
				       unsigned int freq, int dir)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio_info->mem_info.regmap;
	unsigned int mclk_ratio, mclk_ratio_map;

	SND_LOG_DEBUG(HLOG, "\n");

	if (freq == 0) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV, 1 << MCLKOUT_EN, 0 << MCLKOUT_EN);
		return 0;
	}

	if (daudio_info->pllclk_freq == 0) {
		SND_LOG_ERR(HLOG, "pllclk freq is invalid\n");
		return -ENOMEM;
	}
	mclk_ratio = daudio_info->pllclk_freq / freq;

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
		regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV, 1 << MCLKOUT_EN, 0 << MCLKOUT_EN);
		SND_LOG_ERR(HLOG, "mclk freq div unsupport\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV,
			   0xf << MCLK_DIV, mclk_ratio_map << MCLK_DIV);
	regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV, 1 << MCLKOUT_EN, 1 << MCLKOUT_EN);

	return 0;
}

static int sunxi_daudio_dai_set_bclk_ratio(struct snd_soc_dai *dai, unsigned int ratio)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio_info->mem_info.regmap;
	unsigned int bclk_ratio;

	SND_LOG_DEBUG(HLOG, "\n");

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

	regmap_update_bits(regmap, SUNXI_DAUDIO_CLKDIV, 0xf << BCLK_DIV, bclk_ratio << BCLK_DIV);

	return 0;
}

static int sunxi_daudio_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio_info->mem_info.regmap;
	unsigned int mode, offset;
	unsigned int lrck_polarity, brck_polarity;

	SND_LOG_DEBUG(HLOG, "dai fmt -> 0x%x\n", fmt);

	daudio_info->fmt = fmt;

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
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0, 1 << LRCK_WIDTH, 0 << LRCK_WIDTH);
		break;
	case	SND_SOC_DAIFMT_DSP_B:
		mode = 0;
		offset = 0;
		/* L data MSB during FRM LRC (long frame) */
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0, 1 << LRCK_WIDTH, 1 << LRCK_WIDTH);
		break;
	default:
		SND_LOG_ERR(HLOG, "format setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 3 << MODE_SEL, mode << MODE_SEL);
	regmap_update_bits(regmap, SUNXI_DAUDIO_TX0CHSEL, 3 << TX_OFFSET, offset << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_DAUDIO_TX1CHSEL, 3 << TX_OFFSET, offset << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_DAUDIO_TX2CHSEL, 3 << TX_OFFSET, offset << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_DAUDIO_TX3CHSEL, 3 << TX_OFFSET, offset << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_DAUDIO_RXCHSEL, 3 << RX_OFFSET, offset << RX_OFFSET);

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
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
				   1 << LRCK_POLARITY,
				   (lrck_polarity^1) << LRCK_POLARITY);
	else
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
				   1 << LRCK_POLARITY,
				   lrck_polarity << LRCK_POLARITY);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
			   1 << BRCK_POLARITY,
			   brck_polarity << BRCK_POLARITY);

	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	/* bclk & lrck dir input */
	case	SND_SOC_DAIFMT_CBM_CFM:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << BCLK_OUT, 0 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << LRCK_OUT, 0 << LRCK_OUT);
		break;
	case	SND_SOC_DAIFMT_CBS_CFM:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << BCLK_OUT, 1 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << LRCK_OUT, 0 << LRCK_OUT);
		break;
	case	SND_SOC_DAIFMT_CBM_CFS:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << BCLK_OUT, 0 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << LRCK_OUT, 1 << LRCK_OUT);
		break;
	case	SND_SOC_DAIFMT_CBS_CFS:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << BCLK_OUT, 1 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << LRCK_OUT, 1 << LRCK_OUT);
		break;
	default:
		SND_LOG_ERR(HLOG, "unknown master/slave format\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_daudio_dai_set_tdm_slot(struct snd_soc_dai *dai,
					 unsigned int tx_mask, unsigned int rx_mask,
					 int slots, int slot_width)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio_info->mem_info.regmap;
	unsigned int slot_width_map, lrck_width_map;

	SND_LOG_DEBUG(HLOG, "\n");

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
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
			   7 << SLOT_WIDTH, slot_width_map << SLOT_WIDTH);

	/* bclk num of per channel
	 * I2S/RIGHT_J/LEFT_J	-> lrck long total is lrck_width_map * 2
	 * DSP_A/DAP_B		-> lrck long total is lrck_width_map * 1
	 */
	switch (daudio_info->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
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
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
			   0x3ff << LRCK_PERIOD, lrck_width_map << LRCK_PERIOD);

	return 0;
}

static int sunxi_daudio_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream, &daudio_info->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream, &daudio_info->capture_dma_param);

	return 0;
}

static void sunxi_daudio_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG(HLOG, "\n");

	return;
}

static int sunxi_daudio_dai_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params,
				      struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio_info->mem_info.regmap;
	unsigned int channels;
	unsigned int channels_en[16] = {
		0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
		0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
	};


	SND_LOG_DEBUG(HLOG, "\n");

	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
				   SUNXI_DAUDIO_SR_MASK << DAUDIO_SAMPLE_RESOLUTION,
				   SUNXI_DAUDIO_SR_16BIT << DAUDIO_SAMPLE_RESOLUTION);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   SUNXI_DAUDIO_TXIM_MASK << TXIM,
					   SUNXI_DAUDIO_TXIM_VALID_LSB << TXIM);
		} else {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   SUNXI_DAUDIO_RXOM_MASK << RXOM,
					   SUNXI_DAUDIO_RXOM_EXPH << RXOM);
		}
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
				   SUNXI_DAUDIO_SR_MASK << DAUDIO_SAMPLE_RESOLUTION,
				   SUNXI_DAUDIO_SR_24BIT << DAUDIO_SAMPLE_RESOLUTION);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   SUNXI_DAUDIO_TXIM_MASK << TXIM,
					   SUNXI_DAUDIO_TXIM_VALID_LSB << TXIM);
		} else {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   SUNXI_DAUDIO_RXOM_MASK << RXOM,
					   SUNXI_DAUDIO_RXOM_EXPH << RXOM);
		}
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(regmap, SUNXI_DAUDIO_FMT0,
				   SUNXI_DAUDIO_SR_MASK << DAUDIO_SAMPLE_RESOLUTION,
				   SUNXI_DAUDIO_SR_32BIT << DAUDIO_SAMPLE_RESOLUTION);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   SUNXI_DAUDIO_TXIM_MASK << TXIM,
					   SUNXI_DAUDIO_TXIM_VALID_LSB << TXIM);
		} else {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   SUNXI_DAUDIO_RXOM_MASK << RXOM,
					   SUNXI_DAUDIO_RXOM_EXPH << RXOM);
		}
		break;
	default:
		SND_LOG_ERR(HLOG, "unrecognized format\n");
		return -EINVAL;
	}

	/* set channels */
	channels = params_channels(params);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CHCFG,
				   SUNXI_DAUDIO_TX_SLOT_MASK << TX_SLOT_NUM,
				   (channels - 1) << TX_SLOT_NUM);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX0CHSEL,
				   SUNXI_DAUDIO_TX_CHSEL_MASK << TX_CHSEL,
				   (channels - 1) << TX_CHSEL);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX1CHSEL,
				   SUNXI_DAUDIO_TX_CHSEL_MASK << TX_CHSEL,
				   (channels - 1) << TX_CHSEL);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX2CHSEL,
				   SUNXI_DAUDIO_TX_CHSEL_MASK << TX_CHSEL,
				   (channels - 1) << TX_CHSEL);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX3CHSEL,
				   SUNXI_DAUDIO_TX_CHSEL_MASK << TX_CHSEL,
				   (channels - 1) << TX_CHSEL);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX0CHSEL,
				   SUNXI_DAUDIO_TX_CHEN_MASK << TX_CHEN,
				   channels_en[channels - 1] << TX_CHEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX1CHSEL,
				   SUNXI_DAUDIO_TX_CHEN_MASK << TX_CHEN,
				   channels_en[channels - 1] << TX_CHEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX2CHSEL,
				   SUNXI_DAUDIO_TX_CHEN_MASK << TX_CHEN,
				   channels_en[channels - 1] << TX_CHEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_TX3CHSEL,
				   SUNXI_DAUDIO_TX_CHEN_MASK << TX_CHEN,
				   channels_en[channels - 1] << TX_CHEN);
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CHCFG,
				   SUNXI_DAUDIO_RX_SLOT_MASK << RX_SLOT_NUM,
				   (channels - 1) << RX_SLOT_NUM);
		regmap_update_bits(regmap, SUNXI_DAUDIO_RXCHSEL,
				   SUNXI_DAUDIO_RX_CHSEL_MASK << RX_CHSEL,
				   (channels - 1) << RX_CHSEL);
	}

	return 0;
}

static int sunxi_daudio_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio_info->mem_info.regmap;
	unsigned int i;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0 ; i < SUNXI_DAUDIO_FTX_TIMES ; i++) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
			mdelay(1);
		}
		regmap_write(regmap, SUNXI_DAUDIO_TXCNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
				   1 << FIFO_CTL_FRX, 1 << FIFO_CTL_FRX);
		regmap_write(regmap, SUNXI_DAUDIO_RXCNT, 0);
	}

	return 0;
}

static void sunxi_daudio_dai_tx_route(struct sunxi_daudio_info *daudio_info, bool enable)
{
	struct sunxi_daudio_dts_info *dts_info = &daudio_info->dts_info;
	struct regmap *regmap = daudio_info->mem_info.regmap;
	unsigned int reg_val;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_INTCTL, 1 << TXDRQEN, 1 << TXDRQEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL,
				   1 << (SDO0_EN + dts_info->tx_pin),
				   1 << (SDO0_EN + dts_info->tx_pin));
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_TXEN, 1 << CTL_TXEN);
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_INTCTL, 1 << TXDRQEN, 0 << TXDRQEN);

		/* add this to avoid the i2s pop */
		while (1) {
			regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
					   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
			regmap_write(regmap, SUNXI_DAUDIO_TXCNT, 0);
			regmap_read(regmap, SUNXI_DAUDIO_FIFOSTA, &reg_val);
			reg_val = ((reg_val & 0xFF0000) >> 16);
			if (reg_val == 0x80)
				break;
		}
		udelay(250);

		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_TXEN, 0 << CTL_TXEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL,
				   1 << (SDO0_EN + dts_info->tx_pin),
				   0 << (SDO0_EN + dts_info->tx_pin));
	}
}

static void sunxi_daudio_dai_rx_route(struct sunxi_daudio_info *daudio_info, bool enable)
{
	struct regmap *regmap = daudio_info->mem_info.regmap;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_RXEN, 1 << CTL_RXEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_INTCTL, 1 << RXDRQEN, 1 << RXDRQEN);
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_INTCTL, 1 << RXDRQEN, 0 << RXDRQEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_RXEN, 0 << CTL_RXEN);
	}
}

static int sunxi_daudio_dai_trigger(struct snd_pcm_substream *substream,
				    int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_dts_info *dts_info = &daudio_info->dts_info;

	SND_LOG_DEBUG(HLOG, "\n");

	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_daudio_dai_tx_route(daudio_info, true);
		} else {
			sunxi_daudio_dai_rx_route(daudio_info, true);
			if (dts_info->rx_sync_en && dts_info->rx_sync_ctl)
				sunxi_rx_sync_control(dts_info->rx_sync_domain,
						      dts_info->rx_sync_id, true);
		}
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_daudio_dai_tx_route(daudio_info, false);
		} else {
			sunxi_daudio_dai_rx_route(daudio_info, false);
			if (dts_info->rx_sync_en && dts_info->rx_sync_ctl)
				sunxi_rx_sync_control(dts_info->rx_sync_domain,
						      dts_info->rx_sync_id, false);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_daudio_dai_ops = {
	/* call by machine */
	.set_pll	= sunxi_daudio_dai_set_pll,		/* set pllclk */
	.set_sysclk	= sunxi_daudio_dai_set_sysclk,		/* set mclk */
	.set_bclk_ratio	= sunxi_daudio_dai_set_bclk_ratio,	/* set bclk freq */
	.set_fmt	= sunxi_daudio_dai_set_fmt,		/* set tdm fmt */
	.set_tdm_slot	= sunxi_daudio_dai_set_tdm_slot,	/* set slot num and width */
	/* call by asoc */
	.startup	= sunxi_daudio_dai_startup,
	.hw_params	= sunxi_daudio_dai_hw_params,
	.prepare	= sunxi_daudio_dai_prepare,
	.trigger	= sunxi_daudio_dai_trigger,
	.shutdown	= sunxi_daudio_dai_shutdown,
};

static int sunxi_daudio_init(struct sunxi_daudio_info *daudio_info)
{
	struct regmap *regmap = daudio_info->mem_info.regmap;

	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 1 << TX_MLS, 0 << TX_MLS);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 1 << RX_MLS, 0 << RX_MLS);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 3 << SEXT, 0 << SEXT);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 3 << TX_PDM, 0 << TX_PDM);
	regmap_update_bits(regmap, SUNXI_DAUDIO_FMT1, 3 << RX_PDM, 0 << RX_PDM);

	regmap_write(regmap, SUNXI_DAUDIO_TX0CHMAP0, SUNXI_DEFAULT_TXCHMAP0);
	regmap_write(regmap, SUNXI_DAUDIO_TX0CHMAP1, SUNXI_DEFAULT_TXCHMAP1);
	regmap_write(regmap, SUNXI_DAUDIO_TX1CHMAP0, SUNXI_DEFAULT_TXCHMAP0);
	regmap_write(regmap, SUNXI_DAUDIO_TX1CHMAP1, SUNXI_DEFAULT_TXCHMAP1);
	regmap_write(regmap, SUNXI_DAUDIO_TX2CHMAP0, SUNXI_DEFAULT_TXCHMAP0);
	regmap_write(regmap, SUNXI_DAUDIO_TX2CHMAP1, SUNXI_DEFAULT_TXCHMAP1);
	regmap_write(regmap, SUNXI_DAUDIO_TX3CHMAP0, SUNXI_DEFAULT_TXCHMAP0);
	regmap_write(regmap, SUNXI_DAUDIO_TX3CHMAP1, SUNXI_DEFAULT_TXCHMAP1);

	regmap_write(regmap, SUNXI_DAUDIO_RXCHMAP0, SUNXI_DEFAULT_RXCHMAP0);
	regmap_write(regmap, SUNXI_DAUDIO_RXCHMAP1, SUNXI_DEFAULT_RXCHMAP1);
	regmap_write(regmap, SUNXI_DAUDIO_RXCHMAP2, SUNXI_DEFAULT_RXCHMAP2);
	regmap_write(regmap, SUNXI_DAUDIO_RXCHMAP3, SUNXI_DEFAULT_RXCHMAP3);

	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << RX_SYNC_EN, 0 << RX_SYNC_EN);
	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << GLOBAL_EN, 1 << GLOBAL_EN);

	return 0;
}

static int sunxi_daudio_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai,
				  &daudio_info->playback_dma_param,
				  &daudio_info->capture_dma_param);

	sunxi_daudio_init(daudio_info);

	return 0;
}

static int sunxi_daudio_dai_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_clk_info *clk_info = &daudio_info->clk_info;
	struct sunxi_daudio_regulator_info *rglt_info = &daudio_info->rglt_info;
	struct regmap *regmap = daudio_info->mem_info.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	/* save reg value */
	snd_sunxi_save_reg(regmap, sunxi_reg_labels);

	/* disable clk & regulator */
	snd_sunxi_regulator_disable(daudio_info->pdev, rglt_info);
	snd_sunxi_clk_disable(daudio_info->pdev, clk_info);

	return 0;
}

static int sunxi_daudio_dai_resume(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_clk_info *clk_info = &daudio_info->clk_info;
	struct sunxi_daudio_regulator_info *rglt_info = &daudio_info->rglt_info;
	struct regmap *regmap = daudio_info->mem_info.regmap;
	int ret;
	int i;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = snd_sunxi_clk_enable(daudio_info->pdev, clk_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		return ret;
	}
	ret = snd_sunxi_regulator_enable(daudio_info->pdev, rglt_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "regulator enable failed\n");
		return ret;
	}

	/* for daudio init */
	sunxi_daudio_init(daudio_info);

	/* resume reg value */
	snd_sunxi_echo_reg(regmap, sunxi_reg_labels);

	/* for clear TX fifo */
	for (i = 0 ; i < SUNXI_DAUDIO_FTX_TIMES ; i++) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL,
				   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
		mdelay(1);
	}
	regmap_write(regmap, SUNXI_DAUDIO_TXCNT, 0);

	/* for clear RX fifo */
	regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 1 << FIFO_CTL_FRX, 1 << FIFO_CTL_FRX);
	regmap_write(regmap, SUNXI_DAUDIO_RXCNT, 0);

	return 0;
}

static int sunxi_daudio_dai_remove(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = daudio_info->mem_info.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 0x1 << GLOBAL_EN, 0x0 << GLOBAL_EN);

	return 0;
}

static struct snd_soc_dai_driver sunxi_daudio_dai = {
	.probe		= sunxi_daudio_dai_probe,
	.suspend	= sunxi_daudio_dai_suspend,
	.resume		= sunxi_daudio_dai_resume,
	.remove		= sunxi_daudio_dai_remove,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
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
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_daudio_dai_ops,
};

/*******************************************************************************
 * *** sound card & component function source ***
 * @0 sound card probe
 * @1 component function kcontrol register
 ******************************************************************************/
static void sunxi_rx_sync_enable(void *data, bool enable)
{
	struct sunxi_daudio_info *daudio_info = data;
	struct regmap *regmap = daudio_info->mem_info.regmap;

	SND_LOG_DEBUG(HLOG, "%s\n", enable ? "on" : "off");

	if (enable) {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL,
				   0x1 << RX_SYNC_EN_START, 0x1 << RX_SYNC_EN_START);
	} else {
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL,
				   0x1 << RX_SYNC_EN_START, 0x0 << RX_SYNC_EN_START);
	}

	return;
}

static int sunxi_get_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio_info *daudio_info = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = daudio_info->mem_info.regmap;

	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAUDIO_FIFOCTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_set_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio_info *daudio_info = snd_soc_component_get_drvdata(component);
	struct sunxi_daudio_dts_info *dts_info = &daudio_info->dts_info;
	struct regmap *regmap = daudio_info->mem_info.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_TXEN, 0 << CTL_TXEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL,
				   1 << (SDO0_EN + dts_info->tx_pin),
				   0 << (SDO0_EN + dts_info->tx_pin));
		regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 1 << HUB_EN, 0 << HUB_EN);
		break;
	case	1:
		regmap_update_bits(regmap, SUNXI_DAUDIO_FIFOCTL, 1 << HUB_EN, 1 << HUB_EN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << CTL_TXEN, 1 << CTL_TXEN);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL,
				   1 << (SDO0_EN + dts_info->tx_pin),
				   1 << (SDO0_EN + dts_info->tx_pin));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_get_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio_info *daudio_info = snd_soc_component_get_drvdata(component);
	struct sunxi_daudio_dts_info *dts_info = &daudio_info->dts_info;

	ucontrol->value.integer.value[0] = dts_info->rx_sync_ctl;

	return 0;
}

static int sunxi_set_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio_info *daudio_info = snd_soc_component_get_drvdata(component);
	struct sunxi_daudio_dts_info *dts_info = &daudio_info->dts_info;
	struct regmap *regmap = daudio_info->mem_info.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		dts_info->rx_sync_ctl = 0;
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << RX_SYNC_EN, 0 << RX_SYNC_EN);
		sunxi_rx_sync_shutdown(dts_info->rx_sync_domain, dts_info->rx_sync_id);
		break;
	case	1:
		sunxi_rx_sync_startup(dts_info->rx_sync_domain, dts_info->rx_sync_id,
				      (void *)regmap, sunxi_rx_sync_enable);
		regmap_update_bits(regmap, SUNXI_DAUDIO_CTL, 1 << RX_SYNC_EN, 1 << RX_SYNC_EN);
		dts_info->rx_sync_ctl = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *sunxi_switch_text[] = {"Off", "On"};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};
static const struct snd_kcontrol_new sunxi_daudio_controls[] = {
	SOC_SINGLE("loopback debug", SUNXI_DAUDIO_CTL, LOOP_EN, 1, 0),
};

static int sunxi_daudio_probe(struct snd_soc_component *component)
{
	struct sunxi_daudio_info *daudio_info = snd_soc_component_get_drvdata(component);
	struct sunxi_daudio_dts_info *dts_info = &daudio_info->dts_info;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	/* component kcontrols -> tx_hub */
	if (dts_info->tx_hub_en) {
		ret = snd_soc_add_component_controls(component, sunxi_tx_hub_controls,
						     ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add tx_hub kcontrols failed\n");
	}

	/* component kcontrols -> rx_sync */
	if (dts_info->rx_sync_en) {
		ret = snd_soc_add_component_controls(component, sunxi_rx_sync_controls,
						     ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add rx_sync kcontrols failed\n");
	}

	return 0;
}

static struct snd_soc_component_driver sunxi_daudio_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_daudio_probe,
	.controls	= sunxi_daudio_controls,
	.num_controls	= ARRAY_SIZE(sunxi_daudio_controls),
};

/*******************************************************************************
 * *** kernel source ***
 * @0 reg debug
 * @1 regmap
 * @2 clk
 * @3 regulator
 * @4 dts params
 ******************************************************************************/
static int snd_sunxi_save_reg(struct regmap *regmap, struct audio_reg_label *reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (reg_labels[i].name != NULL) {
		regmap_read(regmap, reg_labels[i].address, &(reg_labels[i].value));
		i++;
	}

	return i;
}

static int snd_sunxi_echo_reg(struct regmap *regmap, struct audio_reg_label *reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (reg_labels[i].name != NULL) {
		regmap_write(regmap, reg_labels[i].address, reg_labels[i].value);
		i++;
	}

	return i;
}

static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_daudio_mem_info *mem_info)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = of_address_to_resource(np, 0, &mem_info->res);
	if (ret) {
		SND_LOG_ERR(HLOG, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem_info->memregion = devm_request_mem_region(&pdev->dev, mem_info->res.start,
						      resource_size(&mem_info->res),
						      DRV_NAME);
	if (IS_ERR_OR_NULL(mem_info->memregion)) {
		SND_LOG_ERR(HLOG, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem_info->membase = devm_ioremap(&pdev->dev, mem_info->memregion->start,
					 resource_size(mem_info->memregion));
	if (IS_ERR_OR_NULL(mem_info->membase)) {
		SND_LOG_ERR(HLOG, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem_info->regmap = devm_regmap_init_mmio(&pdev->dev, mem_info->membase, &g_regmap_config);
	if (IS_ERR_OR_NULL(mem_info->regmap)) {
		SND_LOG_ERR(HLOG, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem_info->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_daudio_mem_info *mem_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	devm_iounmap(&pdev->dev, mem_info->membase);
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
}

static int snd_sunxi_clk_init(struct platform_device *pdev,
			      struct sunxi_daudio_clk_info *clk_info)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get rst clk */
	clk_info->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk_info->clk_rst)) {
		SND_LOG_ERR(HLOG, "clk rst get failed\n");
		ret =  PTR_ERR(clk_info->clk_rst);
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk_info->clk_bus = of_clk_get_by_name(np, "clk_bus_i2s");
	if (IS_ERR_OR_NULL(clk_info->clk_bus)) {
		SND_LOG_ERR(HLOG, "clk bus get failed\n");
		ret = PTR_ERR(clk_info->clk_bus);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk_info->clk_pll_audio = of_clk_get_by_name(np, "clk_pll_audio");
	if (IS_ERR_OR_NULL(clk_info->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "clk pll get failed\n");
		ret = PTR_ERR(clk_info->clk_pll_audio);
		goto err_get_clk_pll_audio;
	}

	/* get module clk */
	clk_info->clk_i2s = of_clk_get_by_name(np, "clk_i2s");
	if (IS_ERR_OR_NULL(clk_info->clk_i2s)) {
		SND_LOG_ERR(HLOG, "clk i2s get failed\n");
		ret = PTR_ERR(clk_info->clk_i2s);
		goto err_get_clk_i2s;
	}

	if (clk_set_parent(clk_info->clk_i2s, clk_info->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "set parent of clk_i2s to pll_audio failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	ret = snd_sunxi_clk_enable(pdev, clk_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		ret = -EINVAL;
		goto err_clk_enable;
	}

	return 0;

err_clk_enable:
err_set_parent:
	clk_put(clk_info->clk_i2s);
err_get_clk_i2s:
	clk_put(clk_info->clk_pll_audio);
err_get_clk_pll_audio:
	clk_put(clk_info->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	return ret;
}

static void snd_sunxi_clk_exit(struct platform_device *pdev,
			       struct sunxi_daudio_clk_info *clk_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_clk_disable(pdev, clk_info);
	clk_put(clk_info->clk_i2s);
	clk_put(clk_info->clk_pll_audio);
	clk_put(clk_info->clk_bus);
}

static int snd_sunxi_clk_enable(struct platform_device *pdev,
				struct sunxi_daudio_clk_info *clk_info)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	if (reset_control_deassert(clk_info->clk_rst)) {
		SND_LOG_ERR(HLOG, "clk rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk_info->clk_bus)) {
		SND_LOG_ERR(HLOG, "clk bus enable failed\n");
		goto err_enable_clk_bus;
	}

	if (clk_prepare_enable(clk_info->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "pll_audio enable failed\n");
		goto err_enable_clk_pll_audio;
	}

	if (clk_prepare_enable(clk_info->clk_i2s)) {
		SND_LOG_ERR(HLOG, "clk_i2s enable failed\n");
		goto err_enable_clk_i2s;
	}

	return 0;

err_enable_clk_i2s:
	clk_disable_unprepare(clk_info->clk_pll_audio);
err_enable_clk_pll_audio:
	clk_disable_unprepare(clk_info->clk_bus);
err_enable_clk_bus:
	reset_control_assert(clk_info->clk_rst);
err_deassert_rst:
	return ret;
}

static void snd_sunxi_clk_disable(struct platform_device *pdev,
				  struct sunxi_daudio_clk_info *clk_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	clk_disable_unprepare(clk_info->clk_i2s);
	clk_disable_unprepare(clk_info->clk_pll_audio);
	clk_disable_unprepare(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);
}

static int snd_sunxi_regulator_init(struct platform_device *pdev,
				    struct sunxi_daudio_regulator_info *rglt_info)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	rglt_info->regulator_name = NULL;
	ret = of_property_read_string(np, "daudio_regulator", &rglt_info->regulator_name);
	if (ret < 0) {
		SND_LOG_DEBUG(HLOG, "regulator missing or invalid\n");
		rglt_info->daudio_regulator = NULL;
		return 0;
	}

	rglt_info->daudio_regulator = regulator_get(NULL, rglt_info->regulator_name);
	if (IS_ERR_OR_NULL(rglt_info->daudio_regulator)) {
		SND_LOG_ERR(HLOG, "get duaido vcc-pin failed\n");
		ret = -EFAULT;
		goto err_regulator_get;
	}
	ret = regulator_set_voltage(rglt_info->daudio_regulator, 3300000, 3300000);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "set duaido voltage failed\n");
		ret = -EFAULT;
		goto err_regulator_set_vol;
	}
	ret = regulator_enable(rglt_info->daudio_regulator);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "enable duaido vcc-pin failed\n");
		ret = -EFAULT;
		goto err_regulator_enable;
	}

	return 0;

err_regulator_enable:
err_regulator_set_vol:
	if (rglt_info->daudio_regulator)
		regulator_put(rglt_info->daudio_regulator);
err_regulator_get:
	return ret;
}

static void snd_sunxi_regulator_exit(struct platform_device *pdev,
				     struct sunxi_daudio_regulator_info *rglt_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt_info->daudio_regulator)
		if (!IS_ERR_OR_NULL(rglt_info->daudio_regulator)) {
			regulator_disable(rglt_info->daudio_regulator);
			regulator_put(rglt_info->daudio_regulator);
		}
}

static int snd_sunxi_regulator_enable(struct platform_device *pdev,
				      struct sunxi_daudio_regulator_info *rglt_info)
{
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt_info->daudio_regulator)
		if (!IS_ERR_OR_NULL(rglt_info->daudio_regulator)) {
			ret = regulator_enable(rglt_info->daudio_regulator);
			if (ret) {
				SND_LOG_ERR(HLOG, "enable daudio_regulator failed\n");
				return -1;
			}
		}

	return 0;
}

static void snd_sunxi_regulator_disable(struct platform_device *pdev,
					struct sunxi_daudio_regulator_info *rglt_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt_info->daudio_regulator)
		if (!IS_ERR_OR_NULL(rglt_info->daudio_regulator))
			regulator_disable(rglt_info->daudio_regulator);
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev,
				      struct sunxi_daudio_dts_info *dts_info)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get dma params */
	ret = of_property_read_u32(np, "playback_cma", &temp_val);
	if (ret < 0) {
		dts_info->playback_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN(HLOG, "playback_cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts_info->playback_cma = temp_val;
	}
	ret = of_property_read_u32(np, "capture_cma", &temp_val);
	if (ret != 0) {
		dts_info->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN(HLOG, "capture_cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts_info->capture_cma = temp_val;
	}
	ret = of_property_read_u32(np, "tx_fifo_size", &temp_val);
	if (ret != 0) {
		dts_info->playback_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN(HLOG, "tx_fifo_size miss, using default value\n");
	} else {
		dts_info->playback_fifo_size = temp_val;
	}
	ret = of_property_read_u32(np, "rx_fifo_size", &temp_val);
	if (ret != 0) {
		dts_info->capture_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN(HLOG, "rx_fifo_size miss,using default value\n");
	} else {
		dts_info->capture_fifo_size = temp_val;
	}

	ret = of_property_read_u32(np, "tdm_num", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "tdm_num config missing\n");
		dts_info->tdm_num = 0;
	} else {
		dts_info->tdm_num = temp_val;
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

	SND_LOG_DEBUG(HLOG, "playback_cma : %zu\n", dts_info->playback_cma);
	SND_LOG_DEBUG(HLOG, "capture_cma  : %zu\n", dts_info->capture_cma);
	SND_LOG_DEBUG(HLOG, "tx_fifo_size : %zu\n", dts_info->playback_fifo_size);
	SND_LOG_DEBUG(HLOG, "rx_fifo_size : %zu\n", dts_info->capture_fifo_size);
	SND_LOG_DEBUG(HLOG, "tx_pin       : %u\n", dts_info->tx_pin);
	SND_LOG_DEBUG(HLOG, "rx_pin       : %u\n", dts_info->rx_pin);

	/* tx_hub */
	dts_info->tx_hub_en = of_property_read_bool(np, "tx_hub_en");

	/* components func -> rx_sync */
	dts_info->rx_sync_en = of_property_read_bool(np, "rx_sync_en");
	if (dts_info->rx_sync_en) {
		dts_info->rx_sync_ctl = false;
		dts_info->rx_sync_domain = RX_SYNC_SYS_DOMAIN;
		dts_info->rx_sync_id = sunxi_rx_sync_probe(dts_info->rx_sync_domain);
		if (dts_info->rx_sync_id < 0) {
			SND_LOG_ERR(HLOG, "sunxi_rx_sync_probe failed\n");
		} else {
			SND_LOG_DEBUG(HLOG, "sunxi_rx_sync_probe successful. domain=%d, id=%d\n",
				      dts_info->rx_sync_domain, dts_info->rx_sync_id);
		}
	}
}

static int snd_sunxi_pin_init(struct platform_device *pdev,
			      struct sunxi_daudio_pinctl_info *pin_info)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

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

static void snd_sunxi_pin_exit(struct platform_device *pdev,
			       struct sunxi_daudio_pinctl_info *pin_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (pin_info->pinctrl_used)
		devm_pinctrl_put(pin_info->pinctrl);
}

static void snd_sunxi_dma_params_init(struct sunxi_daudio_info *daudio_info)
{
	struct resource *res = &daudio_info->mem_info.res;
	struct sunxi_daudio_dts_info *dts_info = &daudio_info->dts_info;

	SND_LOG_DEBUG(HLOG, "\n");

	daudio_info->playback_dma_param.src_maxburst = 4;
	daudio_info->playback_dma_param.dst_maxburst = 4;
	daudio_info->playback_dma_param.dma_addr = res->start + SUNXI_DAUDIO_TXFIFO;
	daudio_info->playback_dma_param.cma_kbytes = dts_info->playback_cma;
	daudio_info->playback_dma_param.fifo_size = dts_info->playback_fifo_size;

	daudio_info->capture_dma_param.src_maxburst = 4;
	daudio_info->capture_dma_param.dst_maxburst = 4;
	daudio_info->capture_dma_param.dma_addr = res->start + SUNXI_DAUDIO_RXFIFO;
	daudio_info->capture_dma_param.cma_kbytes = dts_info->capture_cma;
	daudio_info->capture_dma_param.fifo_size = dts_info->capture_fifo_size;
};

/* sysfs debug */
static ssize_t snd_sunxi_debug_show_reg(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	struct sunxi_daudio_info *daudio_info = dev_get_drvdata(dev);
	struct regmap *regmap = daudio_info->mem_info.regmap;
	size_t count = 0, i = 0;
	unsigned int reg_val;
	unsigned int size = ARRAY_SIZE(sunxi_reg_labels);

	while ((i < size) && (sunxi_reg_labels[i].name != NULL)) {
		regmap_read(regmap, sunxi_reg_labels[i].address, &reg_val);
		count += sprintf(buf + count, "%-24s [0x%03x]: 0x%8x save_val:0x%x\n",
				 sunxi_reg_labels[i].name,
				 sunxi_reg_labels[i].address, reg_val,
				 sunxi_reg_labels[i].value);
		i++;
	}

	return count;
}

static ssize_t snd_sunxi_debug_store_reg(struct device *dev, struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct sunxi_daudio_info *daudio_info = dev_get_drvdata(dev);
	struct regmap *regmap = daudio_info->mem_info.regmap;
	int scanf_cnt;
	unsigned int reg_val;
	unsigned int input_reg_val = 0;
	unsigned int input_reg_offset = 0;

	scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	if (scanf_cnt == 0 || scanf_cnt > 2) {
		pr_err("usage read : echo 0x > audio_reg\n");
		pr_err("usage write: echo 0x 0x > audio_reg\n");
		return count;
	}

	if (input_reg_offset > SUNXI_DAUDIO_MAX_REG) {
		pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_DAUDIO_MAX_REG);
		return count;
	}

	if (scanf_cnt == 1) {
		regmap_read(regmap, input_reg_offset, &reg_val);
		pr_err("reg[0x%03x]: 0x%x\n", input_reg_offset, reg_val);
		return count;
	} else if (scanf_cnt == 2) {
		regmap_read(regmap, input_reg_offset, &reg_val);
		pr_err("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, reg_val);
		regmap_write(regmap, input_reg_offset, input_reg_val);
		regmap_read(regmap, input_reg_offset, &reg_val);
		pr_err("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, reg_val);
	}

	return count;
}

static DEVICE_ATTR(audio_reg, 0644, snd_sunxi_debug_show_reg, snd_sunxi_debug_store_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group debug_attr = {
	.name	= "audio_debug",
	.attrs	= audio_debug_attrs,
};

static int sunxi_daudio_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_daudio_info *daudio_info;
	struct sunxi_daudio_mem_info *mem_info;
	struct sunxi_daudio_clk_info *clk_info;
	struct sunxi_daudio_pinctl_info *pin_info;
	struct sunxi_daudio_dts_info *dts_info;
	struct sunxi_daudio_regulator_info *rglt_info;

	SND_LOG_DEBUG(HLOG, "\n");

	/* sunxi daudio info */
	daudio_info = devm_kzalloc(dev, sizeof(struct sunxi_daudio_info), GFP_KERNEL);
	if (IS_ERR_OR_NULL(daudio_info)) {
		SND_LOG_ERR(HLOG, "alloc sunxi_daudio_info failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, daudio_info);
	daudio_info->pdev = pdev;
	mem_info = &daudio_info->mem_info;
	clk_info = &daudio_info->clk_info;
	pin_info = &daudio_info->pin_info;
	dts_info = &daudio_info->dts_info;
	rglt_info = &daudio_info->rglt_info;

	ret = snd_sunxi_mem_init(pdev, mem_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "remap init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_mem_init;
	}
	ret = snd_sunxi_clk_init(pdev, clk_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_clk_init;
	}
	ret = snd_sunxi_regulator_init(pdev, rglt_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "regulator init failed\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_regulator_init;
	}

	snd_sunxi_dts_params_init(pdev, dts_info);

	ret = snd_sunxi_pin_init(pdev, pin_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "pinctrl init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_pin_init;
	}

	snd_sunxi_dma_params_init(daudio_info);

	ret = snd_soc_register_component(&pdev->dev, &sunxi_daudio_dev, &sunxi_daudio_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_soc_sunxi_dma_platform_register(&pdev->dev, 0);
	if (ret) {
		SND_LOG_ERR(HLOG, "register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_sunxi_dma_platform_register;
	}

	ret = snd_sunxi_sysfs_create_group(pdev, &debug_attr);
	if (ret)
		SND_LOG_WARN(HLOG, "sysfs debug create failed\n");

	SND_LOG_DEBUG(HLOG, "register daudio platform success\n");

	return 0;

err_snd_soc_sunxi_dma_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
err_snd_sunxi_pin_init:
	snd_sunxi_regulator_exit(pdev, rglt_info);
err_snd_sunxi_regulator_init:
	snd_sunxi_clk_exit(pdev, clk_info);
err_snd_sunxi_clk_init:
	snd_sunxi_mem_exit(pdev, mem_info);
err_snd_sunxi_mem_init:
	devm_kfree(dev, daudio_info);
err_devm_kzalloc:
	of_node_put(np);

	return ret;
}

static int sunxi_daudio_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_daudio_info *daudio_info = dev_get_drvdata(&pdev->dev);
	struct sunxi_daudio_mem_info *mem_info = &daudio_info->mem_info;
	struct sunxi_daudio_clk_info *clk_info = &daudio_info->clk_info;
	struct sunxi_daudio_pinctl_info *pin_info = &daudio_info->pin_info;
	struct sunxi_daudio_dts_info *dts_info = &daudio_info->dts_info;
	struct sunxi_daudio_regulator_info *rglt_info = &daudio_info->rglt_info;

	SND_LOG_DEBUG(HLOG, "\n");

	/* remove components */
	snd_sunxi_sysfs_remove_group(pdev, &debug_attr);
	if (dts_info->rx_sync_en)
		sunxi_rx_sync_remove(dts_info->rx_sync_domain);

	snd_soc_sunxi_dma_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	snd_sunxi_pin_exit(pdev, pin_info);
	snd_sunxi_clk_exit(pdev, clk_info);
	snd_sunxi_mem_exit(pdev, mem_info);
	snd_sunxi_regulator_exit(pdev, rglt_info);

	devm_kfree(dev, daudio_info);
	of_node_put(np);

	SND_LOG_DEBUG(HLOG, "unregister daudio platform success\n");

	return 0;
}

static const struct of_device_id sunxi_daudio_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_daudio_of_match);

static struct platform_driver sunxi_daudio_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_daudio_of_match,
	},
	.probe	= sunxi_daudio_dev_probe,
	.remove	= sunxi_daudio_dev_remove,
};

int __init sunxi_daudio_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_daudio_driver);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_daudio_dev_exit(void)
{
	platform_driver_unregister(&sunxi_daudio_driver);
}

late_initcall(sunxi_daudio_dev_init);
module_exit(sunxi_daudio_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of daudio");

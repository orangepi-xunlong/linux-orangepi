// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2021, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-dmic"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/slab.h>
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

#include "snd_sunxi_dmic.h"
#include "snd_sunxi_adapter.h"

#define DRV_NAME	"sunxi-snd-plat-dmic"

/* for sample rate conver */
struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

static const struct sample_rate sample_rate_conv[] = {
	{44100, 0x0},
	{48000, 0x0},
	{22050, 0x2},
	/* KNOT support */
	{24000, 0x2},
	{11025, 0x4},
	{12000, 0x4},
	{32000, 0x1},
	{16000, 0x3},
	{8000,  0x5},
};

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DMIC_EN),
	REG_LABEL(SUNXI_DMIC_SR),
	REG_LABEL(SUNXI_DMIC_CTR),
	/* REG_LABEL(SUNXI_DMIC_DATA), */
	REG_LABEL(SUNXI_DMIC_INTC),
	REG_LABEL(SUNXI_DMIC_INTS),
	REG_LABEL(SUNXI_DMIC_FIFO_CTR),
	REG_LABEL(SUNXI_DMIC_FIFO_STA),
	REG_LABEL(SUNXI_DMIC_CH_NUM),
	REG_LABEL(SUNXI_DMIC_CH_MAP),
	REG_LABEL(SUNXI_DMIC_CNT),
	REG_LABEL(SUNXI_DMIC_DATA0_1_VOL),
	REG_LABEL(SUNXI_DMIC_DATA2_3_VOL),
	REG_LABEL(SUNXI_DMIC_HPF_CTRL),
	REG_LABEL(SUNXI_DMIC_HPF_COEF),
	REG_LABEL(SUNXI_DMIC_HPF_GAIN),
	REG_LABEL(SUNXI_DMIC_REV),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

static struct regmap_config g_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DMIC_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

static void sunxi_rx_sync_enable(void *data, bool enable);

static int sunxi_dmic_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
				  unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_dts *dts = &dmic->dts;

	freq_out = freq_out / dts->pll_fs;

	SND_LOG_DEBUG("stream -> %s, freq_in ->%u, freq_out ->%u, pll_fs ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out, dts->pll_fs);

	if (snd_dmic_clk_rate(dmic->clk, freq_in, freq_out)) {
		SND_LOG_ERR("clk set rate failed\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_dmic_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		snd_soc_dai_set_dma_data(dai, substream, &dmic->capture_dma_param);

	return 0;
}

static void sunxi_dmic_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	(void)substream;
	(void)dai;
}

static int sunxi_dmic_dai_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params,
				    struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_dts *dts = &dmic->dts;
	struct regmap *regmap = dmic->mem.regmap;
	unsigned int channels;
	unsigned int channels_en;
	int i;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		SND_LOG_ERR_STD(E_DMIC_SWARG_HW_PARAMS, "unsupport playback\n");
		return -EINVAL;
	}

	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
				   0x1 << DMIC_SAMPLE_RESOLUTION,
				   0x0 << DMIC_SAMPLE_RESOLUTION);
		regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
				   0x1 << DMIC_FIFO_MODE,
				   0x1 << DMIC_FIFO_MODE);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
				   0x1 << DMIC_SAMPLE_RESOLUTION,
				   0x1 << DMIC_SAMPLE_RESOLUTION);
		regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
				   0x1 << DMIC_FIFO_MODE,
				   0x0 << DMIC_FIFO_MODE);
		break;
	default:
		SND_LOG_ERR_STD(E_DMIC_SWARG_HW_PARAMS, "unrecognized format\n");
		return -EINVAL;
	}

	/* set rate */
	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == params_rate(params)) {
			if (sample_rate_conv[i].samplerate > 48000)
				return -EINVAL;
			regmap_update_bits(regmap, SUNXI_DMIC_SR, 0x7 << DMIC_SR,
					   sample_rate_conv[i].rate_bit << DMIC_SR);
		}
	}

	/* oversamplerate adjust */
	if (params_rate(params) >= 24000)
		regmap_update_bits(regmap, SUNXI_DMIC_CTR,
				   1 << DMIC_OVERSAMPLE_RATE, 1 << DMIC_OVERSAMPLE_RATE);
	else
		regmap_update_bits(regmap, SUNXI_DMIC_CTR,
				   1 << DMIC_OVERSAMPLE_RATE, 0 << DMIC_OVERSAMPLE_RATE);

	/* set channels */
	channels = params_channels(params);
	regmap_update_bits(regmap, SUNXI_DMIC_CH_NUM, 0x7 << DMIC_CH_NUM,
			   (channels - 1) << DMIC_CH_NUM);
	for (i = 0; i < channels; i++) {
		channels_en = (dts->rx_chmap >> 4 * i) & 0xf;
		regmap_update_bits(regmap, SUNXI_DMIC_EN, 0x1 << (DATA_CH_EN + channels_en),
				   0x1 << (DATA_CH_EN + channels_en));
		/* enabled HPF */
		regmap_update_bits(regmap, SUNXI_DMIC_HPF_CTRL,
				   0x1 << (HPF_DATA0_CHL_EN + channels_en),
				   0x1 << (HPF_DATA0_CHL_EN + channels_en));
	}

	/* enable clk after set clk rate */
	if (snd_dmic_clk_enable(dmic->clk)) {
		SND_LOG_ERR("clk enable failed\n");
		return -EINVAL;
	} else {
		dmic->clk_sta = SND_SUNXI_CLK_OPEN;
	}

	return 0;
}

static int sunxi_dmic_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	/* prevent the closed clks from being closed again */
	if (dmic->clk_sta == SND_SUNXI_CLK_OPEN) {
		snd_dmic_clk_disable(dmic->clk);
		dmic->clk_sta = SND_SUNXI_CLK_CLOSE;
	}

	return 0;
}

static int sunxi_dmic_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = dmic->mem.regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR, 1 << DMIC_FIFO_FLUSH, 1 << DMIC_FIFO_FLUSH);

	regmap_write(regmap, SUNXI_DMIC_INTS,
		     (1 << FIFO_OVERRUN_IRQ_PENDING) | (1 << FIFO_DATA_IRQ_PENDING));
	regmap_write(regmap, SUNXI_DMIC_CNT, 0x0);

	return 0;
}

static void sunxi_dmic_dai_rx_route(struct sunxi_dmic *dmic, bool enable)
{
	struct regmap *regmap = dmic->mem.regmap;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_DMIC_INTC, 0x1 << FIFO_DRQ_EN, 0x1 << FIFO_DRQ_EN);
		regmap_update_bits(regmap, SUNXI_DMIC_EN, 0x1 << GLOBE_EN, 0x1 << GLOBE_EN);
	} else {
		regmap_update_bits(regmap, SUNXI_DMIC_EN, 0x1 << GLOBE_EN, 0x0 << GLOBE_EN);
		regmap_update_bits(regmap, SUNXI_DMIC_INTC, 0x1 << FIFO_DRQ_EN, 0x0 << FIFO_DRQ_EN);
	}
}

static int sunxi_dmic_dai_trigger(struct snd_pcm_substream *substream,
				  int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_dts *dts = &dmic->dts;

	SND_LOG_DEBUG("\n");

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			sunxi_dmic_dai_rx_route(dmic, true);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			sunxi_dmic_dai_rx_route(dmic, false);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct snd_soc_dai_ops sunxi_dmic_dai_ops = {
	/* call by machine */
	.set_pll	= sunxi_dmic_dai_set_pll,
	/* call by asoc */
	.startup	= sunxi_dmic_dai_startup,
	.hw_params	= sunxi_dmic_dai_hw_params,
	.prepare	= sunxi_dmic_dai_prepare,
	.trigger	= sunxi_dmic_dai_trigger,
	.hw_free	= sunxi_dmic_dai_hw_free,
	.shutdown	= sunxi_dmic_dai_shutdown,
};

static int sunxi_dmic_init(struct sunxi_dmic *dmic)
{
	struct sunxi_dmic_dts *dts = &dmic->dts;
	struct regmap *regmap = dmic->mem.regmap;
	unsigned int rx_dtime_map;

	SND_LOG_DEBUG("\n");

	/* set rx channel map */
	regmap_write(regmap, SUNXI_DMIC_CH_MAP, dts->rx_chmap);

	/* set rxfifo delay time */
	switch (dts->rx_dtime) {
	case 5:
		rx_dtime_map = 0;
		break;
	case 10:
		rx_dtime_map = 1;
		break;
	case 20:
		rx_dtime_map = 2;
		break;
	case 30:
		rx_dtime_map = 3;
		break;
	case 0:
	default:
		break;
	}
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 0x3 << DMICFDT, rx_dtime_map << DMICFDT);
	if (dts->rx_dtime)
		regmap_update_bits(regmap, SUNXI_DMIC_CTR, 0x1 << DMICDFEN, 0x1 << DMICDFEN);
	else
		regmap_update_bits(regmap, SUNXI_DMIC_CTR, 0x1 << DMICDFEN, 0x0 << DMICDFEN);

	/* disable rx_sync_en default */
	regmap_update_bits(regmap, SUNXI_DMIC_EN, 0x1 << RX_SYNC_EN, 0x0 << RX_SYNC_EN);

	/* diabsle LR SWAP default */
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 1 << DATA0_LR_SWAP_EN, 0 << DATA0_LR_SWAP_EN);
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 1 << DATA1_LR_SWAP_EN, 0 << DATA1_LR_SWAP_EN);
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 1 << DATA2_LR_SWAP_EN, 0 << DATA2_LR_SWAP_EN);
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 1 << DATA3_LR_SWAP_EN, 0 << DATA3_LR_SWAP_EN);

	/* set the digital volume */
	regmap_update_bits(regmap, SUNXI_DMIC_DATA0_1_VOL,
			   (0xFF << DATA0L_VOL) | (0xFF << DATA0R_VOL),
			   (dts->data_vol << DATA0L_VOL) | (dts->data_vol << DATA0R_VOL));
	regmap_update_bits(regmap, SUNXI_DMIC_DATA0_1_VOL,
			   (0xFF << DATA1L_VOL) | (0xFF << DATA1R_VOL),
			   (dts->data_vol << DATA1L_VOL) | (dts->data_vol << DATA1R_VOL));

	regmap_update_bits(regmap, SUNXI_DMIC_DATA2_3_VOL,
			   (0xFF << DATA2L_VOL) | (0xFF << DATA2R_VOL),
			   (dts->data_vol << DATA2L_VOL) | (dts->data_vol << DATA2R_VOL));
	regmap_update_bits(regmap, SUNXI_DMIC_DATA2_3_VOL,
			   (0xFF << DATA3L_VOL) | (0xFF << DATA3R_VOL),
			   (dts->data_vol << DATA3L_VOL) | (dts->data_vol << DATA3R_VOL));

	return 0;
}

static int sunxi_dmic_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, NULL, &dmic->capture_dma_param);

	sunxi_dmic_init(dmic);

	return 0;
}

static struct snd_soc_dai_driver sunxi_dmic_dai = {
	.name = DRV_NAME,
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= SNDRV_PCM_RATE_8000_48000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
	},
};

/*******************************************************************************
 * *** sound card & component function source ***
 * @0 sound card probe
 * @1 component function kcontrol register
 ******************************************************************************/
static void sunxi_rx_sync_enable(void *data, bool enable)
{
	struct regmap *regmap = data;

	SND_LOG_DEBUG("%s\n", enable ? "on" : "off");

	if (enable)
		regmap_update_bits(regmap, SUNXI_DMIC_EN,
			0x1 << RX_SYNC_EN_START, 0x1 << RX_SYNC_EN_START);
	else
		regmap_update_bits(regmap, SUNXI_DMIC_EN,
			0x1 << RX_SYNC_EN_START, 0x0 << RX_SYNC_EN_START);
}

static int sunxi_get_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_dmic *dmic = snd_soc_component_get_drvdata(component);
	struct sunxi_dmic_dts *dts = &dmic->dts;

	ucontrol->value.integer.value[0] = dts->rx_sync_ctl;

	return 0;
}

static int sunxi_set_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_dmic *dmic = snd_soc_component_get_drvdata(component);
	struct sunxi_dmic_dts *dts = &dmic->dts;
	struct regmap *regmap = dmic->mem.regmap;

	if (dts->rx_sync_ctl == ucontrol->value.integer.value[0])
		return 0;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dts->rx_sync_ctl = 0;
		regmap_update_bits(regmap, SUNXI_DMIC_EN, 0x1 << RX_SYNC_EN, 0x0 << RX_SYNC_EN);
		sunxi_rx_sync_shutdown(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DMIC_EN, 0x1 << RX_SYNC_EN, 0x1 << RX_SYNC_EN);
		dts->rx_sync_ctl = 1;
		sunxi_rx_sync_startup(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *sunxi_switch_text[] = {"Off", "On"};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};

static const DECLARE_TLV_DB_SCALE(digital_tlv, -12000, 75, 7125);
static const struct snd_kcontrol_new sunxi_dmic_controls[] = {
	/* Digital Volume */
	SOC_SINGLE_TLV("L0 volume", SUNXI_DMIC_DATA0_1_VOL, DATA0L_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("R0 volume", SUNXI_DMIC_DATA0_1_VOL, DATA0R_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("L1 volume", SUNXI_DMIC_DATA0_1_VOL, DATA1L_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("R1 volume", SUNXI_DMIC_DATA0_1_VOL, DATA1R_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("L2 volume", SUNXI_DMIC_DATA2_3_VOL, DATA2L_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("R2 volume", SUNXI_DMIC_DATA2_3_VOL, DATA2R_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("L3 volume", SUNXI_DMIC_DATA2_3_VOL, DATA3L_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("R3 volume", SUNXI_DMIC_DATA2_3_VOL, DATA3R_VOL, 0xFF, 0, digital_tlv),
};

static int sunxi_dmic_component_probe(struct snd_soc_component *component)
{
	struct sunxi_dmic *dmic = snd_soc_component_get_drvdata(component);
	struct sunxi_dmic_dts *dts = &dmic->dts;
	struct regmap *regmap = dmic->mem.regmap;
	int ret;

	SND_LOG_DEBUG("\n");

	/* component kcontrols -> rx_sync */
	if (dts->rx_sync_en) {
		ret = snd_soc_add_component_controls(component, sunxi_rx_sync_controls,
						     ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR_STD(E_DMIC_SWSYS_COMP_PROBE, "add rx_sync kcontrols failed\n");

		dts->rx_sync_ctl = false;
		dts->rx_sync_domain = RX_SYNC_SYS_DOMAIN;
		dts->rx_sync_id = sunxi_rx_sync_probe(dts->rx_sync_domain);
		if (dts->rx_sync_id < 0) {
			SND_LOG_ERR("sunxi_rx_sync_probe failed\n");
		} else {
			SND_LOG_DEBUG("sunxi_rx_sync_probe successful. domain=%d, id=%d\n",
				      dts->rx_sync_domain, dts->rx_sync_id);
			ret = sunxi_rx_sync_register_cb(dts->rx_sync_domain, dts->rx_sync_id,
							(void *)regmap, sunxi_rx_sync_enable);
			if (ret)
				SND_LOG_ERR("callback register failed\n");
		}
	}

	return 0;
}

static void sunxi_dmic_component_remove(struct snd_soc_component *component)
{
	struct sunxi_dmic *dmic = snd_soc_component_get_drvdata(component);
	struct sunxi_dmic_dts *dts = &dmic->dts;
	SND_LOG_DEBUG("\n");

	if (dts->rx_sync_en)
		sunxi_rx_sync_unregister_cb(dts->rx_sync_domain, dts->rx_sync_id);
}

static int sunxi_dmic_component_suspend(struct snd_soc_component *component)
{
	struct sunxi_dmic *dmic = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = dmic->mem.regmap;

	SND_LOG_DEBUG("\n");

	/* save reg value */
	snd_sunxi_save_reg(regmap, &sunxi_reg_group);

	/* disable clk & regulator */
	snd_dmic_clk_bus_disable(dmic->clk);
	snd_sunxi_regulator_disable(dmic->rglt);

	return 0;
}

static int sunxi_dmic_component_resume(struct snd_soc_component *component)
{
	struct sunxi_dmic *dmic = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = dmic->mem.regmap;
	int ret;

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_regulator_enable(dmic->rglt);
	if (ret) {
		SND_LOG_ERR("regulator enable failed\n");
		return ret;
	}
	ret = snd_dmic_clk_bus_enable(dmic->clk);
	if (ret) {
		SND_LOG_ERR("clk_bus and clk_rst enable failed\n");
		return ret;
	}

	/* for dmic init */
	sunxi_dmic_init(dmic);

	/* resume reg value */
	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);

	/* for clear RX fifo */
	regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
			   (1 << DMIC_FIFO_FLUSH), (1 << DMIC_FIFO_FLUSH));
	regmap_write(regmap, SUNXI_DMIC_INTS,
		     (1 << FIFO_OVERRUN_IRQ_PENDING) | (1 << FIFO_DATA_IRQ_PENDING));
	regmap_write(regmap, SUNXI_DMIC_CNT, 0x0);

	return 0;
}

static struct snd_soc_component_driver sunxi_dmic_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_dmic_component_probe,
	.remove		= sunxi_dmic_component_remove,
	.suspend	= sunxi_dmic_component_suspend,
	.resume		= sunxi_dmic_component_resume,
	.controls	= sunxi_dmic_controls,
	.num_controls	= ARRAY_SIZE(sunxi_dmic_controls),
};

/*******************************************************************************
 * *** kernel source ***
 * @0 regmap
 * @1 clk
 * @2 dts params
 ******************************************************************************/
static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_dmic_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	ret = of_address_to_resource(np, 0, &mem->res);
	if (ret) {
		SND_LOG_ERR_STD(E_DMIC_SWDEP_MEM_INIT, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res), DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR_STD(E_DMIC_SWDEP_MEM_INIT, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev, mem->memregion->start,
				    resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR_STD(E_DMIC_SWDEP_MEM_INIT, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &g_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR_STD(E_DMIC_SWDEP_MEM_INIT, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_dmic_mem *mem)
{
	SND_LOG_DEBUG("\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_dmic_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* get dma params */
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
	ret = of_property_read_u32(np, "rx-fifo-size", &temp_val);
	if (ret != 0) {
		dts->capture_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("rx-fifo-size miss,using default value\n");
	} else {
		dts->capture_fifo_size = temp_val;
	}

	ret = of_property_read_u32(np, "rx-chmap", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("rx-chmap config missing\n");
		dts->rx_chmap = 0x76543210;
	} else {
		dts->rx_chmap = temp_val;
	}
	ret = of_property_read_u32(np, "data-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("data-vol config missing\n");
		dts->data_vol = 0xA0;
	} else {
		if (temp_val > 0xFF)
			temp_val = 0XFF;
		dts->data_vol = temp_val;
	}
	ret = of_property_read_u32(np, "rxdelaytime", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("rxdelaytime config missing\n");
		dts->rx_dtime = 0;
	} else {
		switch (temp_val) {
		case 0:
		case 5:
		case 10:
		case 20:
		case 30:
			dts->rx_dtime = temp_val;
			break;
		default:
			SND_LOG_WARN("rx delay time supoort only 0,5,10,20,30ms\n");
			dts->rx_dtime = 0;
			break;
		}
	}

	SND_LOG_DEBUG("capture-cma  : %zu\n", dts->capture_cma);
	SND_LOG_DEBUG("rx-fifo-size : %zu\n", dts->capture_fifo_size);

	/* components func -> rx_sync */
	dts->rx_sync_en = of_property_read_bool(np, "rx-sync-en");

	/* clk fs */
	ret = of_property_read_u32(np, "pll-fs", &temp_val);
	if (ret < 0) {
		dts->pll_fs = 1;
	} else {
		dts->pll_fs = temp_val;
	}
}

static int snd_sunxi_pin_init(struct platform_device *pdev, struct sunxi_dmic_pinctl *pin)
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
		SND_LOG_ERR_STD(E_DMIC_SWDEP_PIN_INIT, "pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin->pinstate = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin->pinstate)) {
		SND_LOG_ERR_STD(E_DMIC_SWDEP_PIN_INIT, "pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin->pinstate_sleep = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin->pinstate_sleep)) {
		SND_LOG_ERR_STD(E_DMIC_SWDEP_PIN_INIT, "pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin->pinctrl, pin->pinstate);
	if (ret < 0) {
		SND_LOG_ERR_STD(E_DMIC_SWDEP_PIN_INIT, "dmic set pinctrl default state fail\n");
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
static void snd_sunxi_dma_params_init(struct sunxi_dmic *dmic)
{
	struct resource *res = &dmic->mem.res;
	struct sunxi_dmic_dts *dts = &dmic->dts;

	SND_LOG_DEBUG("\n");

	dmic->capture_dma_param.dma_addr = res->start + SUNXI_DMIC_DATA;
	dmic->capture_dma_param.src_maxburst = 8;
	dmic->capture_dma_param.dst_maxburst = 8;
	dmic->capture_dma_param.cma_kbytes = dts->capture_cma;
	dmic->capture_dma_param.fifo_size = dts->capture_fifo_size;
};

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
/* sysfs debug */
static void snd_sunxi_dump_version(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_dmic *dmic = (struct sunxi_dmic *)priv;

	if (!dmic) {
		SND_LOG_ERR_STD(E_DMIC_SWARG_DUMP_VER, "priv to dmic failed\n");
		return;
	}
	if (dmic->pdev)
		if (dmic->pdev->dev.driver)
			if (dmic->pdev->dev.driver->owner)
				goto module_version;
	return;

module_version:
	dmic->module_version = dmic->pdev->dev.driver->owner->version;
	count_tmp += sprintf(buf + count_tmp, "%s\n", dmic->module_version);

	*count = count_tmp;
}

static void snd_sunxi_dump_help(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;

	count_tmp += sprintf(buf + count_tmp, "1. reg read : echo {num} > dump && cat dump\n");
	count_tmp += sprintf(buf + count_tmp, "num: 0(all)\n");
	count_tmp += sprintf(buf + count_tmp, "2. reg write: echo {reg} {value} > dump\n");
	count_tmp += sprintf(buf + count_tmp, "eg. echo 0x00 0xaa > dump\n");

	*count = count_tmp;
}

static int snd_sunxi_dump_show(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_dmic *dmic = (struct sunxi_dmic *)priv;
	unsigned int i = 0;
	unsigned int output_reg_val;
	struct regmap *regmap;

	if (!dmic) {
		SND_LOG_ERR_STD(E_DMIC_SWARG_DUMP_SHOW, "priv to dmic failed\n");
		return -1;
	}
	if (!dmic->show_reg_all)
		return 0;
	else
		dmic->show_reg_all = false;

	regmap = dmic->mem.regmap;
	for (i = 0; i < ARRAY_SIZE(sunxi_reg_labels); ++i) {
		regmap_read(regmap, sunxi_reg_labels[i].address, &output_reg_val);
		count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
				     sunxi_reg_labels[i].address, output_reg_val);
	}

	*count = count_tmp;

	return 0;
}

static int snd_sunxi_dump_store(void *priv, const char *buf, size_t count)
{
	struct sunxi_dmic *dmic = (struct sunxi_dmic *)priv;
	int scanf_cnt;
	unsigned int input_reg_offset, input_reg_val, output_reg_val;
	struct regmap *regmap;

	if (count <= 1)	/* null or only "\n" */
		return 0;
	if (!dmic) {
		SND_LOG_ERR_STD(E_DMIC_SWARG_DUMP_STORE, "priv to dmic failed\n");
		return -1;
	}
	regmap = dmic->mem.regmap;

	if (!strcmp(buf, "0\n")) {
		dmic->show_reg_all = true;
		return 0;
	}

	scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	if (scanf_cnt != 2) {
		pr_err("wrong format: %s\n", buf);
		return -1;
	}
	if (input_reg_offset > SUNXI_DMIC_REG_MAX) {
		pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_DMIC_REG_MAX);
		return -1;
	}
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
	regmap_write(regmap, input_reg_offset, input_reg_val);
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);

	return 0;
}
#endif

static int sunxi_dmic_dev_probe(struct platform_device *pdev)
{
	struct sunxi_adapt_dai_ops_priv priv;
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_dmic *dmic;
	struct sunxi_dmic_mem *mem;
	struct sunxi_dmic_pinctl *pin;
	struct sunxi_dmic_dts *dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump;
#endif

	SND_LOG_DEBUG("\n");

	/* sunxi dmic info */
	dmic = devm_kzalloc(dev, sizeof(*dmic), GFP_KERNEL);
	if (IS_ERR_OR_NULL(dmic)) {
		SND_LOG_ERR_STD(E_DMIC_SWSYS_PLAT_PROBE, "alloc sunxi_dmic failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, dmic);
	mem = &dmic->mem;
	pin = &dmic->pin;
	dts = &dmic->dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	dump = &dmic->dump;
#endif
	dmic->pdev = pdev;

	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR("remap init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_mem_init;
	}
	dmic->clk = snd_dmic_clk_init(pdev);
	if (!dmic->clk) {
		SND_LOG_ERR("clk init failed\n");
		ret = -EINVAL;
		goto err_snd_dmic_clk_init;
	}
	ret = snd_dmic_clk_bus_enable(dmic->clk);
	if (ret) {
		SND_LOG_ERR("clk_bus and clk_rst enable failed\n");
		ret = -EINVAL;
		goto err_clk_bus_enable;
	}

	dmic->rglt = snd_sunxi_regulator_init(dev);
	if (!dmic->rglt) {
		SND_LOG_ERR("rglt init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_rglt_init;
	}

	snd_sunxi_dts_params_init(pdev, dts);

	ret = snd_sunxi_pin_init(pdev, pin);
	if (ret) {
		SND_LOG_ERR("pinctrl init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_pin_init;
	}

	snd_sunxi_dma_params_init(dmic);

	priv.probe = sunxi_dmic_dai_probe;
	priv.remove = NULL;
	sunxi_adpt_set_dai_ops(&sunxi_dmic_dai, &sunxi_dmic_dai_ops, &priv);

	ret = snd_soc_register_component(&pdev->dev, &sunxi_dmic_dev, &sunxi_dmic_dai, 1);
	if (ret) {
		SND_LOG_ERR_STD(E_DMIC_SWSYS_PLAT_PROBE, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_sunxi_dma_platform_register(&pdev->dev);
	if (ret) {
		SND_LOG_ERR("register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_platform_register;
	}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snprintf(dmic->module_name, 32, "%s", "DMIC");
	dump->name = dmic->module_name;
	dump->priv = dmic;
	dump->dump_version = snd_sunxi_dump_version;
	dump->dump_help = snd_sunxi_dump_help;
	dump->dump_show = snd_sunxi_dump_show;
	dump->dump_store = snd_sunxi_dump_store;
	ret = snd_sunxi_dump_register(dump);
	if (ret)
		SND_LOG_WARN("snd_sunxi_dump_register failed\n");
#endif

	SND_LOG_DEBUG("register dmic platform success\n");

	return 0;

err_snd_sunxi_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
err_snd_sunxi_pin_init:
	snd_sunxi_regulator_exit(dmic->rglt);
err_snd_sunxi_rglt_init:
err_clk_bus_enable:
	snd_dmic_clk_exit(dmic->clk);
err_snd_dmic_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_snd_sunxi_mem_init:
	devm_kfree(dev, dmic);
err_devm_kzalloc:
	of_node_put(np);

	return ret;
}

static int sunxi_dmic_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_dmic *dmic = dev_get_drvdata(&pdev->dev);
	struct sunxi_dmic_mem *mem = &dmic->mem;
	struct sunxi_dmic_pinctl *pin = &dmic->pin;
	struct sunxi_dmic_dts *dts = &dmic->dts;

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump = &dmic->dump;
#endif

	/* remove components */
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snd_sunxi_dump_unregister(dump);
#endif
	if (dts->rx_sync_en)
		sunxi_rx_sync_remove(dts->rx_sync_domain);

	snd_sunxi_dma_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	snd_sunxi_regulator_exit(dmic->rglt);
	snd_dmic_clk_bus_disable(dmic->clk);
	snd_dmic_clk_exit(dmic->clk);
	snd_sunxi_mem_exit(pdev, mem);
	if (pin->pinctrl_used)
		devm_pinctrl_put(pin->pinctrl);

	devm_kfree(dev, dmic);
	of_node_put(np);

	SND_LOG_DEBUG("unregister dmic platform success\n");

	return 0;
}

static const struct of_device_id sunxi_dmic_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_dmic_of_match);

static struct platform_driver sunxi_dmic_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_dmic_of_match,
	},
	.probe	= sunxi_dmic_dev_probe,
	.remove	= sunxi_dmic_dev_remove,
};

int __init sunxi_dmic_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_dmic_driver);
	if (ret != 0) {
		SND_LOG_ERR_STD(E_DMIC_SWSYS_MOD_INIT, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_dmic_dev_exit(void)
{
	platform_driver_unregister(&sunxi_dmic_driver);
}

late_initcall(sunxi_dmic_dev_init);
module_exit(sunxi_dmic_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.10");
MODULE_DESCRIPTION("sunxi soundcard platform of dmic");

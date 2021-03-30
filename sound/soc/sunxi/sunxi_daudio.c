/* ound\soc\sunxi\sunxi_daudio.c
 * (C) Copyright 2015-2017
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wolfgang huang <huangjinhui@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/dma/sunxi-dma.h>
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "sunxi_daudio.h"
#include "sunxi_dma.h"

#define	DRV_NAME	"sunxi_daudio"

#define	SUNXI_DAUDIO_EXTERNAL_TYPE	1
#define	SUNXI_DAUDIO_TDMHDMI_TYPE	2

struct sunxi_daudio_platform_data {
	unsigned int daudio_type;
	unsigned int external_type;
	unsigned int daudio_master;
	unsigned int pcm_lrck_period;
	unsigned int pcm_lrckr_period;
	unsigned int slot_width_select;
	unsigned int tx_data_mode;
	unsigned int rx_data_mode;
	unsigned int audio_format;
	unsigned int signal_inversion;
	unsigned int frame_type;
	unsigned int tdm_config;
	unsigned int tdm_num;
	unsigned int mclk_div;
};

struct sunxi_daudio_info {
	struct device *dev;
	struct regmap *regmap;
	struct clk *pllclk;
	struct clk *moduleclk;
	struct mutex mutex;
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;
	struct sunxi_daudio_platform_data *pdata;
	unsigned int hub_mode;
	unsigned int hdmi_en;
};

static bool daudio_loop_en;
module_param(daudio_loop_en, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(daudio_loop_en, "SUNXI Digital audio loopback debug(Y=enable, N=disable)");

static int sunxi_daudio_get_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct snd_soc_dai *dai = card->rtd->cpu_dai;
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	unsigned int reg_val;

	regmap_read(sunxi_daudio->regmap, SUNXI_DAUDIO_FIFOCTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (1<<HUB_EN)) ? 2 : 1);
	return 0;
}

static int sunxi_daudio_set_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct snd_soc_dai *dai = card->rtd->cpu_dai;
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
	case	1:
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1<<HUB_EN), (0<<HUB_EN));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (0<<CTL_TXEN));
		break;
	case	2:
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1<<HUB_EN), (1<<HUB_EN));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (1<<CTL_TXEN));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *daudio_format_function[] = {"null", "hub_disable", "hub_enable"};
static const struct soc_enum daudio_format_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(daudio_format_function),
			daudio_format_function),
};

/* dts pcm Audio Mode Select */
static const struct snd_kcontrol_new sunxi_spdif_controls[] = {
	SOC_ENUM_EXT("sunxi daudio audio hub mode", daudio_format_enum[0],
		sunxi_daudio_get_hub_mode, sunxi_daudio_set_hub_mode),
};

static void sunxi_daudio_txctrl_enable(struct sunxi_daudio_info *sunxi_daudio,
					int enable)
{
	pr_debug("Enter %s, enable %d\n", __func__, enable);
	if (enable) {
		/* HDMI audio Transmit Clock just enable at startup */
		if (sunxi_daudio->pdata->daudio_type
			!= SUNXI_DAUDIO_TDMHDMI_TYPE)
			regmap_update_bits(sunxi_daudio->regmap,
					SUNXI_DAUDIO_CTL,
					(1<<CTL_TXEN), (1<<CTL_TXEN));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_INTCTL,
					(1<<TXDRQEN), (1<<TXDRQEN));
	} else {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_INTCTL,
					(1<<TXDRQEN), (0<<TXDRQEN));
		if (sunxi_daudio->pdata->daudio_type
			!= SUNXI_DAUDIO_TDMHDMI_TYPE)
			regmap_update_bits(sunxi_daudio->regmap,
					SUNXI_DAUDIO_CTL,
					(1<<CTL_TXEN), (0<<CTL_TXEN));
	}
	pr_debug("End %s, enable %d\n", __func__, enable);
}

static void sunxi_daudio_rxctrl_enable(struct sunxi_daudio_info *sunxi_daudio,
					int enable)
{
	if (enable) {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_RXEN), (1<<CTL_RXEN));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_INTCTL,
				(1<<RXDRQEN), (1<<RXDRQEN));
	} else {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_INTCTL,
				(1<<RXDRQEN), (0<<RXDRQEN));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_RXEN), (0<<CTL_RXEN));
	}
}

static int sunxi_daudio_global_enable(struct sunxi_daudio_info *sunxi_daudio,
					int enable)
{
	if (enable) {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<SDO0_EN), (1<<SDO0_EN));
		if (sunxi_daudio->hdmi_en) {
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO1_EN), (1<<SDO1_EN));
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO2_EN), (1<<SDO2_EN));
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO3_EN), (1<<SDO3_EN));
		}
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<GLOBAL_EN), (1<<GLOBAL_EN));
	} else {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<GLOBAL_EN), (0<<GLOBAL_EN));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<SDO0_EN), (0<<SDO0_EN));
		if (sunxi_daudio->hdmi_en) {
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO1_EN), (0<<SDO1_EN));
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO2_EN), (0<<SDO2_EN));
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO3_EN), (0<<SDO3_EN));
		}
	}
	return 0;
}

static int sunxi_daudio_mclk_setting(struct sunxi_daudio_info *sunxi_daudio)
{
	unsigned int mclk_div;

	if (sunxi_daudio->pdata->mclk_div) {
		switch (sunxi_daudio->pdata->mclk_div) {
		case	1:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_1;
			break;
		case	2:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_2;
			break;
		case	4:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_3;
			break;
		case	6:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_4;
			break;
		case	8:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_5;
			break;
		case	12:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_6;
			break;
		case	16:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_7;
			break;
		case	24:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_8;
			break;
		case	32:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_9;
			break;
		case	48:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_10;
			break;
		case	64:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_11;
			break;
		case	96:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_12;
			break;
		case	128:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_13;
			break;
		case	176:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_14;
			break;
		case	192:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_15;
			break;
		default:
			dev_err(sunxi_daudio->dev, "unsupport  mclk_div\n");
			return -EINVAL;
		}
		/* setting Mclk as external codec input clk */
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CLKDIV,
			(SUNXI_DAUDIO_MCLK_DIV_MASK<<MCLK_DIV),
			(mclk_div<<MCLK_DIV));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CLKDIV,
				(1<<MCLKOUT_EN), (1<<MCLKOUT_EN));
	} else {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CLKDIV,
				(1<<MCLKOUT_EN), (0<<MCLKOUT_EN));
	}
	return 0;
}

static int sunxi_daudio_init_fmt(struct sunxi_daudio_info *sunxi_daudio,
				unsigned int fmt)
{
	unsigned int offset, mode;
	unsigned int lrck_polarity, brck_polarity;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case	SND_SOC_DAIFMT_CBM_CFM:
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(SUNXI_DAUDIO_LRCK_OUT_MASK<<LRCK_OUT),
				(SUNXI_DAUDIO_LRCK_OUT_DISABLE<<LRCK_OUT));
		break;
	case	SND_SOC_DAIFMT_CBS_CFS:
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(SUNXI_DAUDIO_LRCK_OUT_MASK<<LRCK_OUT),
				(SUNXI_DAUDIO_LRCK_OUT_ENABLE<<LRCK_OUT));
		break;
	default:
		dev_err(sunxi_daudio->dev, "unknown maser/slave format\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case	SND_SOC_DAIFMT_I2S:
		offset = SUNXI_DAUDIO_TX_OFFSET_1;
		mode = SUNXI_DAUDIO_MODE_CTL_I2S;
		break;
	case	SND_SOC_DAIFMT_RIGHT_J:
		offset = SUNXI_DAUDIO_TX_OFFSET_0;
		mode = SUNXI_DAUDIO_MODE_CTL_RIGHT;
		break;
	case	SND_SOC_DAIFMT_LEFT_J:
		offset = SUNXI_DAUDIO_TX_OFFSET_0;
		mode = SUNXI_DAUDIO_MODE_CTL_LEFT;
		break;
	case	SND_SOC_DAIFMT_DSP_A:
		offset = SUNXI_DAUDIO_TX_OFFSET_1;
		mode = SUNXI_DAUDIO_MODE_CTL_PCM;
		break;
	case	SND_SOC_DAIFMT_DSP_B:
		offset = SUNXI_DAUDIO_TX_OFFSET_0;
		mode = SUNXI_DAUDIO_MODE_CTL_PCM;
		break;
	default:
		dev_err(sunxi_daudio->dev, "format setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
			(SUNXI_DAUDIO_MODE_CTL_MASK<<MODE_SEL),
			(mode<<MODE_SEL));
	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_TX0CHSEL,
			(SUNXI_DAUDIO_TX_OFFSET_MASK<<TX_OFFSET),
			(offset<<TX_OFFSET));
	if (sunxi_daudio->hdmi_en) {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_TX1CHSEL,
			(SUNXI_DAUDIO_TX_OFFSET_MASK<<TX_OFFSET),
			(offset<<TX_OFFSET));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_TX2CHSEL,
			(SUNXI_DAUDIO_TX_OFFSET_MASK<<TX_OFFSET),
			(offset<<TX_OFFSET));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_TX3CHSEL,
			(SUNXI_DAUDIO_TX_OFFSET_MASK<<TX_OFFSET),
			(offset<<TX_OFFSET));
	}
#ifdef	CONFIG_ARCH_SUN8IW10
	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_RXCHSEL,
			(SUNXI_DAUDIO_RX_OFFSET_MASK<<RX_OFFSET),
			(offset<<RX_OFFSET));
#else
	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_RXCHSEL,
			(SUNXI_DAUDIO_RX_OFFSET_MASK<<RX_OFFSET),
			(offset<<RX_OFFSET));
#endif

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case	SND_SOC_DAIFMT_NB_NF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_NOR;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_NOR;
		break;
	case	SND_SOC_DAIFMT_NB_IF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_INV;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_NOR;
		break;
	case	SND_SOC_DAIFMT_IB_NF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_NOR;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_INV;
		break;
	case	SND_SOC_DAIFMT_IB_IF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_INV;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_INV;
		break;
	default:
		dev_err(sunxi_daudio->dev, "invert clk setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT0,
			(1<<LRCK_POLARITY), (lrck_polarity<<LRCK_POLARITY));
	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT0,
			(1<<BRCK_POLARITY), (brck_polarity<<BRCK_POLARITY));
	return 0;
}

static int sunxi_daudio_init(struct sunxi_daudio_info *sunxi_daudio)
{
	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT0,
			(1<<LRCK_WIDTH),
			(sunxi_daudio->pdata->frame_type<<LRCK_WIDTH));
	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT0,
			(SUNXI_DAUDIO_LRCK_PERIOD_MASK)<<LRCK_PERIOD,
			((sunxi_daudio->pdata->pcm_lrck_period-1)<<LRCK_PERIOD));

	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT0,
			(SUNXI_DAUDIO_SLOT_WIDTH_MASK<<SLOT_WIDTH),
			(((sunxi_daudio->pdata->slot_width_select>>2) - 1)<<SLOT_WIDTH));

	/*
	 * MSB on the transmit format, always be first.
	 * default using Linear-PCM, without no companding.
	 * A-law<Eourpean standard> or U-law<US-Japan> not working ok.
	 */
	regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT1, SUNXI_DAUDIO_FMT1_DEF);

	sunxi_daudio_init_fmt(sunxi_daudio, (sunxi_daudio->pdata->audio_format
		| (sunxi_daudio->pdata->signal_inversion<<SND_SOC_DAIFMT_SIG_SHIFT)
		| (sunxi_daudio->pdata->daudio_master<<SND_SOC_DAIFMT_MASTER_SHIFT)));

	return sunxi_daudio_mclk_setting(sunxi_daudio);
}

static int sunxi_daudio_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct sunxi_hdmi_priv *sunxi_hdmi = snd_soc_card_get_drvdata(card);
#ifndef	CONFIG_ARCH_SUN8IW10
	unsigned int reg_val;
#endif

	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		/*
		 * Special procesing for hdmi, HDMI card name is
		 * "sndhdmi" or sndhdmiraw. if card not HDMI,
		 * strstr func just return NULL, jump to right section.
		 * Not HDMI card, sunxi_hdmi maybe a NULL pointer.
		 */
		if (sunxi_daudio->pdata->daudio_type
			== SUNXI_DAUDIO_TDMHDMI_TYPE
			&& (sunxi_hdmi->hdmi_format > 1)) {
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK<<SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_24BIT<<SAMPLE_RESOLUTION));
			regmap_update_bits(sunxi_daudio->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK<<TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_MSB<<TXIM));
		} else {
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK<<SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_16BIT<<SAMPLE_RESOLUTION));
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
				regmap_update_bits(sunxi_daudio->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK<<TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_LSB<<TXIM));
			else
				regmap_update_bits(sunxi_daudio->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_RXOM_MASK<<RXOM),
					(SUNXI_DAUDIO_RXOM_EXPH<<RXOM));
		}
		break;
	case	SNDRV_PCM_FORMAT_S20_3LE:
	case	SNDRV_PCM_FORMAT_S24_LE:
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK<<SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_24BIT<<SAMPLE_RESOLUTION));
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_daudio->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK<<TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_LSB<<TXIM));
		else
			regmap_update_bits(sunxi_daudio->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_RXOM_MASK<<RXOM),
					(SUNXI_DAUDIO_RXOM_EXPH<<RXOM));
		break;
	case	SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK<<SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_32BIT<<SAMPLE_RESOLUTION));
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_daudio->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK<<TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_LSB<<TXIM));
		else
			regmap_update_bits(sunxi_daudio->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_RXOM_MASK<<RXOM),
					(SUNXI_DAUDIO_RXOM_EXPH<<RXOM));
		break;
	default:
		dev_err(sunxi_daudio->dev, "unrecognized format\n");
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CHCFG,
				(SUNXI_DAUDIO_TX_SLOT_MASK<<TX_SLOT_NUM),
				((params_channels(params)-1)<<TX_SLOT_NUM));
		if (sunxi_daudio->hdmi_en == 0) {
#ifdef CONFIG_ARCH_SUN8IW10
			regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_TX0CHMAP0, SUNXI_DEFAULT_CHMAP1);
			regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_TX0CHMAP1, SUNXI_DEFAULT_CHMAP0);
#else
			regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_TX0CHMAP0, SUNXI_DEFAULT_CHMAP0);
#endif
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_TX0CHSEL,
					(SUNXI_DAUDIO_TX_CHSEL_MASK<<TX_CHSEL),
					((params_channels(params)-1)<<TX_CHSEL));
			regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_TX0CHSEL,
					(SUNXI_DAUDIO_TX_CHEN_MASK<<TX_CHEN),
					((1<<params_channels(params))-1)<<TX_CHEN);
		} else {
#ifndef CONFIG_ARCH_SUN8IW10
			pr_info("GYY:channel num is %d\n", params_channels(params));
			regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_TX0CHMAP0, 0x10);
			if (params_channels(params) - 2 > 0)
				regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_TX1CHMAP0, 0x23);
			if (params_channels(params) - 4 > 0) {
				if (params_channels(params) == 6)
					regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_TX2CHMAP0, 0x54);
				else
					regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_TX2CHMAP0, 0x54);
			}
			if (params_channels(params) - 6 > 0)
				regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_TX3CHMAP0, 0x76);
			regmap_update_bits(sunxi_daudio->regmap , SUNXI_DAUDIO_TX0CHSEL,
					0x01 << TX_CHSEL, 0x01 << TX_CHSEL);
			regmap_update_bits(sunxi_daudio->regmap , SUNXI_DAUDIO_TX0CHSEL,
					0x03 << TX_CHEN, 0x03 << TX_CHEN);

			regmap_update_bits(sunxi_daudio->regmap , SUNXI_DAUDIO_TX1CHSEL,
					0x01 << TX_CHSEL, 0x01 << TX_CHSEL);
			regmap_update_bits(sunxi_daudio->regmap , SUNXI_DAUDIO_TX1CHSEL,
					(0x03)<<TX_CHEN, 0x03 << TX_CHEN);

			regmap_update_bits(sunxi_daudio->regmap , SUNXI_DAUDIO_TX2CHSEL,
					0x01 << TX_CHSEL, 0x01 << TX_CHSEL);
			regmap_update_bits(sunxi_daudio->regmap , SUNXI_DAUDIO_TX2CHSEL,
					(0x03)<<TX_CHEN, 0x03 << TX_CHEN);

			regmap_update_bits(sunxi_daudio->regmap , SUNXI_DAUDIO_TX3CHSEL,
					0x01 << TX_CHSEL, 0x01 << TX_CHSEL);
			regmap_update_bits(sunxi_daudio->regmap , SUNXI_DAUDIO_TX3CHSEL,
					(0x03)<<TX_CHEN, 0x03 << TX_CHEN);
#endif
		}
	} else {
#ifdef CONFIG_ARCH_SUN8IW10
		regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_RXCHMAP0, SUNXI_DEFAULT_CHMAP1);
		regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_RXCHMAP1, SUNXI_DEFAULT_CHMAP0);
#else
		regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_RXCHMAP, SUNXI_DEFAULT_CHMAP);
#endif
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CHCFG,
				(SUNXI_DAUDIO_RX_SLOT_MASK<<RX_SLOT_NUM),
				((params_channels(params)-1)<<RX_SLOT_NUM));
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_RXCHSEL,
				(SUNXI_DAUDIO_RX_CHSEL_MASK<<RX_CHSEL),
				((params_channels(params)-1)<<RX_CHSEL));
	}
#ifndef	CONFIG_ARCH_SUN8IW10
	/* Special processing for HDMI hub playback to enable hdmi module */
	if (sunxi_daudio->pdata->daudio_type == SUNXI_DAUDIO_TDMHDMI_TYPE) {
		mutex_lock(&sunxi_daudio->mutex);
		regmap_read(sunxi_daudio->regmap,
				SUNXI_DAUDIO_FIFOCTL, &reg_val);
		sunxi_daudio->hub_mode = (reg_val & (1<<HUB_EN));
		if (sunxi_daudio->hub_mode) {
			sndhdmi_hw_params(substream, params, NULL);
			sndhdmi_prepare(substream, NULL);
		}
		mutex_unlock(&sunxi_daudio->mutex);
	}
#endif

	return 0;
}

static int sunxi_daudio_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	sunxi_daudio_init_fmt(sunxi_daudio, fmt);
	return 0;
}

static int sunxi_daudio_set_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	if (clk_set_rate(sunxi_daudio->pllclk, freq)) {
		dev_err(sunxi_daudio->dev, "set pllclk rate failed\n");
		return -EBUSY;
	}
	return 0;
}

static int sunxi_daudio_set_clkdiv(struct snd_soc_dai *dai,
				int clk_id, int clk_div)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	unsigned int bclk_div, div_ratio;

	if (sunxi_daudio->pdata->tdm_config)
		/* I2S/TDM two channel mode */
		div_ratio = clk_div / (2 * sunxi_daudio->pdata->pcm_lrck_period);
	else
		/* PCM mode */
		div_ratio = clk_div / sunxi_daudio->pdata->pcm_lrck_period;

	switch (div_ratio) {
	case	1:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_1;
		break;
	case	2:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_2;
		break;
	case	4:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_3;
		break;
	case	6:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_4;
		break;
	case	8:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_5;
		break;
	case	12:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_6;
		break;
	case	16:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_7;
		break;
	case	24:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_8;
		break;
	case	32:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_9;
		break;
	case	48:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_10;
		break;
	case	64:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_11;
		break;
	case	96:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_12;
		break;
	case	128:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_13;
		break;
	case	176:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_14;
		break;
	case	192:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_15;
		break;
	default:
		dev_err(sunxi_daudio->dev, "unsupport clk_div\n");
		return -EINVAL;
	}

	/* setting bclk to driver external codec bit clk */
	regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CLKDIV,
			(SUNXI_DAUDIO_BCLK_DIV_MASK<<BCLK_DIV),
			(bclk_div<<BCLK_DIV));
	return 0;
}

static int sunxi_daudio_dai_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	/* FIXME: As HDMI module to play audio, it need at least 1100ms to sync.
	 * if we not wait we lost audio data to playback, or we wait for 1100ms
	 * to playback, user experience worst than you can imagine. So we need
	 * to cutdown that sync time by keeping clock signal on. we just enable
	 * it at startup and resume, cutdown it at remove and suspend time.
	 */
	if (sunxi_daudio->pdata->daudio_type == SUNXI_DAUDIO_TDMHDMI_TYPE)
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (1<<CTL_TXEN));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_daudio->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_daudio->capture_dma_param);

	return 0;
}

static int sunxi_daudio_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (daudio_loop_en)
				regmap_update_bits(sunxi_daudio->regmap,
						SUNXI_DAUDIO_CTL,
						(1<<LOOP_EN), (1<<LOOP_EN));
			else
				regmap_update_bits(sunxi_daudio->regmap,
						SUNXI_DAUDIO_CTL,
						(1<<LOOP_EN), (0<<LOOP_EN));
			sunxi_daudio_txctrl_enable(sunxi_daudio, 1);
		} else {
			sunxi_daudio_rxctrl_enable(sunxi_daudio, 1);
		}
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			sunxi_daudio_txctrl_enable(sunxi_daudio, 0);
		else
			sunxi_daudio_rxctrl_enable(sunxi_daudio, 0);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_daudio_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1<<FIFO_CTL_FTX), (1<<FIFO_CTL_FTX));
		regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_TXCNT, 0);
	} else {
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1<<FIFO_CTL_FRX), (1<<FIFO_CTL_FRX));
		regmap_write(sunxi_daudio->regmap, SUNXI_DAUDIO_RXCNT, 0);
	}
	return 0;
}

static int sunxi_daudio_probe(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	int ret;

	mutex_init(&sunxi_daudio->mutex);

	ret = snd_soc_add_card_controls(dai->card, sunxi_spdif_controls,
					ARRAY_SIZE(sunxi_spdif_controls));
	if (ret)
		dev_warn(sunxi_daudio->dev, "Failed to register hub mode control, will continue without it.\n");

	sunxi_daudio_init(sunxi_daudio);
	return 0;
}

static void sunxi_daudio_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	/* Special processing for HDMI hub playback to shutdown hdmi module */
	if (sunxi_daudio->pdata->daudio_type == SUNXI_DAUDIO_TDMHDMI_TYPE) {
		mutex_lock(&sunxi_daudio->mutex);
		if (sunxi_daudio->hub_mode)
			sndhdmi_shutdown(substream, NULL);
		mutex_unlock(&sunxi_daudio->mutex);
	}
}

static int sunxi_daudio_remove(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	if (sunxi_daudio->pdata->daudio_type == SUNXI_DAUDIO_TDMHDMI_TYPE)
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (0<<CTL_TXEN));
	return 0;
}

static int sunxi_daudio_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	pr_debug("[daudio] suspend .%s\n", dev_name(sunxi_daudio->dev));

	/* Global disable I2S/TDM module */
	sunxi_daudio_global_enable(sunxi_daudio, 0);

	if (sunxi_daudio->pdata->daudio_type == SUNXI_DAUDIO_TDMHDMI_TYPE)
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (0<<CTL_TXEN));

	clk_disable_unprepare(sunxi_daudio->moduleclk);
	clk_disable_unprepare(sunxi_daudio->pllclk);

	if (sunxi_daudio->pdata->external_type) {
		ret = pinctrl_select_state(sunxi_daudio->pinctrl,
				sunxi_daudio->pinstate_sleep);
		if (ret) {
			pr_warn("[daudio]select pin sleep state failed\n");
			return ret;
		}
		devm_pinctrl_put(sunxi_daudio->pinctrl);
	}

	return 0;
}

static int sunxi_daudio_resume(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	int ret;

	pr_debug("[daudio] resume .%s\n", dev_name(sunxi_daudio->dev));

	if (clk_prepare_enable(sunxi_daudio->pllclk)) {
		dev_err(sunxi_daudio->dev, "pllclk resume failed\n");
		ret = -EBUSY;
		goto err_resume_out;
	}

	if (clk_prepare_enable(sunxi_daudio->moduleclk)) {
		dev_err(sunxi_daudio->dev, "moduleclk resume failed\n");
		ret = -EBUSY;
		goto err_pllclk_disable;
	}

	if (sunxi_daudio->pdata->external_type) {
		sunxi_daudio->pinctrl = devm_pinctrl_get(sunxi_daudio->dev);
		if (IS_ERR_OR_NULL(sunxi_daudio)) {
			dev_err(sunxi_daudio->dev, "pinctrl resume get failed\n");
			ret = -ENOMEM;
			goto err_moduleclk_disable;
		}

		sunxi_daudio->pinstate = pinctrl_lookup_state(sunxi_daudio->pinctrl,
							PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinstate)) {
			dev_err(sunxi_daudio->dev, "pinctrl default state get failed\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}

		sunxi_daudio->pinstate_sleep = pinctrl_lookup_state(sunxi_daudio->pinctrl,
							PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinstate_sleep)) {
			dev_err(sunxi_daudio->dev, "pinctrl sleep state get failed\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}

		ret = pinctrl_select_state(sunxi_daudio->pinctrl, sunxi_daudio->pinstate);
		if (ret)
			dev_warn(sunxi_daudio->dev,
				"digital audio set pinctrl default state failed\n");
	}

	sunxi_daudio_init(sunxi_daudio);

	/* Global enable I2S/TDM module */
	sunxi_daudio_global_enable(sunxi_daudio, 1);

	if (sunxi_daudio->pdata->daudio_type == SUNXI_DAUDIO_TDMHDMI_TYPE)
		regmap_update_bits(sunxi_daudio->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (1<<CTL_TXEN));

	return 0;

err_pinctrl_put:
	devm_pinctrl_put(sunxi_daudio->pinctrl);
err_moduleclk_disable:
	clk_disable_unprepare(sunxi_daudio->moduleclk);
err_pllclk_disable:
	clk_disable_unprepare(sunxi_daudio->pllclk);
err_resume_out:
	return ret;
}

#define	SUNXI_DAUDIO_RATES	(SNDRV_PCM_RATE_8000_192000 \
				| SNDRV_PCM_RATE_KNOT)

static struct snd_soc_dai_ops sunxi_daudio_dai_ops = {
	.hw_params = sunxi_daudio_hw_params,
	.set_sysclk = sunxi_daudio_set_sysclk,
	.set_clkdiv = sunxi_daudio_set_clkdiv,
	.set_fmt = sunxi_daudio_set_fmt,
	.startup = sunxi_daudio_dai_startup,
	.trigger = sunxi_daudio_trigger,
	.prepare = sunxi_daudio_prepare,
	.shutdown = sunxi_daudio_shutdown,
};

static struct snd_soc_dai_driver sunxi_daudio_dai = {
	.probe = sunxi_daudio_probe,
	.suspend = sunxi_daudio_suspend,
	.resume = sunxi_daudio_resume,
	.remove = sunxi_daudio_remove,
	.playback = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S20_3LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S20_3LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_daudio_dai_ops,
};

static const struct snd_soc_component_driver sunxi_daudio_component = {
	.name = DRV_NAME,
};

static struct sunxi_daudio_platform_data sunxi_daudio = {
	.daudio_type = SUNXI_DAUDIO_EXTERNAL_TYPE,
	.external_type = 1,
};

static struct sunxi_daudio_platform_data sunxi_tdmhdmi = {
	.daudio_type = SUNXI_DAUDIO_TDMHDMI_TYPE,
	.external_type = 0,
	.audio_format = 1,
	.signal_inversion = 1,
	.daudio_master = 4,
	.pcm_lrck_period = 32,
	.pcm_lrckr_period = 1,
	.slot_width_select = 32,
	.tx_data_mode = 0,
	.rx_data_mode = 0,
	.tdm_config = 1,
	.mclk_div = 0,
};

static const struct of_device_id sunxi_daudio_of_match[] = {
	{
		.compatible = "allwinner,sunxi-daudio",
		.data = &sunxi_daudio,
	},
	{
		.compatible = "allwinner,sunxi-tdmhdmi",
		.data = &sunxi_tdmhdmi,
	},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_daudio_of_match);

static const struct regmap_config sunxi_daudio_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DAUDIO_DEBUG,
	.cache_type = REGCACHE_NONE,
};

static int sunxi_daudio_dev_probe(struct platform_device *pdev)
{
	struct resource res, *memregion;
	const struct of_device_id *match;
	void __iomem *sunxi_daudio_membase;
	struct sunxi_daudio_info *sunxi_daudio;
	struct device_node *np = pdev->dev.of_node;
	unsigned int temp_val;
	int ret;

	match = of_match_device(sunxi_daudio_of_match, &pdev->dev);
	if (match) {
		sunxi_daudio = devm_kzalloc(&pdev->dev,
					sizeof(struct sunxi_daudio_info),
					GFP_KERNEL);
		if (!sunxi_daudio) {
			dev_err(&pdev->dev, "alloc sunxi_daudio failed\n");
			ret = -ENOMEM;
			goto err_node_put;
		}
		dev_set_drvdata(&pdev->dev, sunxi_daudio);
		sunxi_daudio->dev = &pdev->dev;

		sunxi_daudio->pdata = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_daudio_platform_data),
				GFP_KERNEL);
		if (!sunxi_daudio->pdata) {
			dev_err(&pdev->dev, "alloc sunxi daudio platform data failed\n");
			ret = -ENOMEM;
			goto err_devm_kfree;
		}

		memcpy(sunxi_daudio->pdata, match->data,
			sizeof(struct sunxi_daudio_platform_data));
	} else {
		dev_err(&pdev->dev, "node match failed\n");
		return -EINVAL;
	}

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	}

	memregion = devm_request_mem_region(&pdev->dev, res.start,
					resource_size(&res), DRV_NAME);
	if (!memregion) {
		dev_err(&pdev->dev, "Memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	sunxi_daudio_membase = ioremap(res.start, resource_size(&res));
	if (!sunxi_daudio_membase) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	sunxi_daudio->regmap = devm_regmap_init_mmio(&pdev->dev,
					sunxi_daudio_membase,
					&sunxi_daudio_regmap_config);
	if (IS_ERR(sunxi_daudio->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		ret = PTR_ERR(sunxi_daudio->regmap);
		goto err_iounmap;
	}

	sunxi_daudio->pllclk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(sunxi_daudio->pllclk)) {
		dev_err(&pdev->dev, "pllclk get failed\n");
		ret = PTR_ERR(sunxi_daudio->pllclk);
		goto err_iounmap;
	}

	sunxi_daudio->moduleclk = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(sunxi_daudio->moduleclk)) {
		dev_err(&pdev->dev, "moduleclk get failed\n");
		ret = PTR_ERR(sunxi_daudio->moduleclk);
		goto err_pllclk_put;
	}

	if (clk_set_parent(sunxi_daudio->moduleclk, sunxi_daudio->pllclk)) {
		dev_err(&pdev->dev, "set parent of moduleclk to pllclk failed\n");
		ret = -EBUSY;
		goto err_moduleclk_put;
	}
	clk_prepare_enable(sunxi_daudio->pllclk);
	clk_prepare_enable(sunxi_daudio->moduleclk);

	if (sunxi_daudio->pdata->external_type) {
		sunxi_daudio->pinctrl = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinctrl)) {
			dev_err(&pdev->dev, "pinctrl get failed\n");
			ret = -EINVAL;
			goto err_moduleclk_put;
		}

		sunxi_daudio->pinstate = pinctrl_lookup_state(sunxi_daudio->pinctrl, PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinstate)) {
			dev_err(&pdev->dev, "pinctrl default state get failed\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}

		sunxi_daudio->pinstate_sleep = pinctrl_lookup_state(sunxi_daudio->pinctrl, PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinstate_sleep)) {
			dev_err(&pdev->dev, "pinctrl sleep state get failed\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}
	}

	switch (sunxi_daudio->pdata->daudio_type) {
	case	SUNXI_DAUDIO_EXTERNAL_TYPE:
		ret = of_property_read_u32(np, "tdm_num", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "tdm configuration missing or invalid\n");
			/*
			 * warnning just continue,
			 * making tdm_num as default setting
			 */
			sunxi_daudio->pdata->tdm_num = 0;
		} else {
			if (temp_val > 2)
				sunxi_daudio->pdata->tdm_num = 0;
			else
				sunxi_daudio->pdata->tdm_num = temp_val;
		}

		sunxi_daudio->playback_dma_param.dma_addr =
					res.start + SUNXI_DAUDIO_TXFIFO;
		if (sunxi_daudio->pdata->tdm_num)
			sunxi_daudio->playback_dma_param.dma_drq_type_num =
						DRQDST_DAUDIO_1_TX;
		else
			sunxi_daudio->playback_dma_param.dma_drq_type_num =
						DRQDST_DAUDIO_0_TX;
		sunxi_daudio->playback_dma_param.src_maxburst = 4;
		sunxi_daudio->playback_dma_param.dst_maxburst = 4;

		sunxi_daudio->capture_dma_param.dma_addr =
					res.start + SUNXI_DAUDIO_RXFIFO;
		if (sunxi_daudio->pdata->tdm_num)
			sunxi_daudio->capture_dma_param.dma_drq_type_num =
						DRQSRC_DAUDIO_1_RX;
		else
			sunxi_daudio->capture_dma_param.dma_drq_type_num =
						DRQSRC_DAUDIO_0_RX;
		sunxi_daudio->capture_dma_param.src_maxburst = 4;
		sunxi_daudio->capture_dma_param.dst_maxburst = 4;

		ret = of_property_read_u32(np, "daudio_master", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "daudio_master configuration missing or invalid\n");
			/*
			 * default setting SND_SOC_DAIFMT_CBS_CFS mode
			 * codec clk & FRM slave
			 */
			sunxi_daudio->pdata->daudio_master = 4;
		} else {
			sunxi_daudio->pdata->daudio_master = temp_val;
		}

		ret = of_property_read_u32(np, "pcm_lrck_period", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "pcm_lrck_period configuration missing or invalid\n");
			sunxi_daudio->pdata->pcm_lrck_period = 0;
		} else {
			sunxi_daudio->pdata->pcm_lrck_period = temp_val;
		}

		ret = of_property_read_u32(np, "slot_width_select", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "slot_width_select configuration missing or invalid\n");
			sunxi_daudio->pdata->slot_width_select = 0;
		} else {
			sunxi_daudio->pdata->slot_width_select = temp_val;
		}

		ret = of_property_read_u32(np, "tx_data_mode", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "tx_data_mode configuration missing or invalid\n");
			sunxi_daudio->pdata->tx_data_mode = 0;
		} else {
			sunxi_daudio->pdata->tx_data_mode = temp_val;
		}

		ret = of_property_read_u32(np, "rx_data_mode", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "rx_data_mode configuration missing or invalid\n");
			sunxi_daudio->pdata->rx_data_mode = 0;
		} else {
			sunxi_daudio->pdata->rx_data_mode = temp_val;
		}

		ret = of_property_read_u32(np, "audio_format", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "audio_format configuration missing or invalid\n");
			sunxi_daudio->pdata->audio_format = 1;
		} else {
			sunxi_daudio->pdata->audio_format = temp_val;
		}

		ret = of_property_read_u32(np, "signal_inversion", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "signal_inversion configuration missing or invalid\n");
			sunxi_daudio->pdata->signal_inversion = 1;
		} else {
			sunxi_daudio->pdata->signal_inversion = temp_val;
		}

		ret = of_property_read_u32(np, "tdm_config", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "tdm_config configuration missing or invalid\n");
			sunxi_daudio->pdata->tdm_config = 1;
		} else {
			sunxi_daudio->pdata->tdm_config = temp_val;
		}

		ret = of_property_read_u32(np, "mclk_div", &temp_val);
		if (ret < 0)
			sunxi_daudio->pdata->mclk_div = 0;
		else
			sunxi_daudio->pdata->mclk_div = temp_val;

		break;
	case	SUNXI_DAUDIO_TDMHDMI_TYPE:
#ifndef	CONFIG_ARCH_SUN8IW10
		sunxi_daudio->playback_dma_param.dma_addr =
				res.start + SUNXI_DAUDIO_TXFIFO;
		sunxi_daudio->playback_dma_param.dma_drq_type_num =
					DRQDST_DAUDIO_2_TX;
		sunxi_daudio->playback_dma_param.src_maxburst = 8;
		sunxi_daudio->playback_dma_param.dst_maxburst = 8;
		sunxi_daudio->hdmi_en = 1;
#endif
		break;
	default:
		dev_err(&pdev->dev, "missing digital audio type\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	}

	ret = snd_soc_register_component(&pdev->dev, &sunxi_daudio_component,
					&sunxi_daudio_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "component register failed\n");
		ret = -ENOMEM;
		goto err_pinctrl_put;
	}

	switch (sunxi_daudio->pdata->daudio_type) {
	case	SUNXI_DAUDIO_EXTERNAL_TYPE:
		ret = asoc_dma_platform_register(&pdev->dev, 0);
		if (ret) {
			dev_err(&pdev->dev, "register ASoC platform failed\n");
			ret = -ENOMEM;
			goto err_unregister_component;
		}
		break;
	case	SUNXI_DAUDIO_TDMHDMI_TYPE:
		ret = asoc_dma_platform_register(&pdev->dev,
					SND_DMAENGINE_PCM_FLAG_NO_RESIDUE);
		if (ret) {
			dev_err(&pdev->dev, "register ASoC platform failed\n");
			ret = -ENOMEM;
			goto err_unregister_component;
		}
		break;
	default:
		dev_err(&pdev->dev, "missing digital audio type\n");
		ret = -EINVAL;
		goto err_unregister_component;
	}

	sunxi_daudio_global_enable(sunxi_daudio, 1);

	return 0;

err_unregister_component:
	snd_soc_unregister_component(&pdev->dev);
err_pinctrl_put:
	devm_pinctrl_put(sunxi_daudio->pinctrl);
err_moduleclk_put:
	clk_put(sunxi_daudio->moduleclk);
err_pllclk_put:
	clk_put(sunxi_daudio->pllclk);
err_iounmap:
	iounmap(sunxi_daudio_membase);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_daudio);
err_node_put:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_daudio_dev_remove(struct platform_device *pdev)
{
	struct sunxi_daudio_info *sunxi_daudio = dev_get_drvdata(&pdev->dev);

	snd_soc_unregister_component(&pdev->dev);
	clk_put(sunxi_daudio->moduleclk);
	clk_put(sunxi_daudio->pllclk);
	devm_kfree(&pdev->dev, sunxi_daudio);
	return 0;
}

static struct platform_driver sunxi_daudio_driver = {
	.probe = sunxi_daudio_dev_probe,
	.remove = __exit_p(sunxi_daudio_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_daudio_of_match,
	},
};

module_platform_driver(sunxi_daudio_driver);

MODULE_AUTHOR("wolfgang huang <huangjinhui@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI DAI AUDIO ASoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-daudio");

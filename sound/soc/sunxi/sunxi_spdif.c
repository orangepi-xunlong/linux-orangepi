/*
 * sound\soc\sunxi\sunxi-spdif.c
 * (C) Copyright 2014-2016
 * allwinnertech Technology Co., Ltd. <www.allwinnertech.com>
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
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include "sunxi_spdif.h"
#include "sunxi_dma.h"

#define	DRV_NAME	"sunxi-spdif"

struct sunxi_spdif_info {
	struct device *dev;
	struct regmap *regmap;
	struct mutex mutex;
	struct clk *pllclk;
	struct clk *moduleclk;
	struct snd_soc_dai_driver dai;
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
	struct pinctrl *pinctrl;
	struct pinctrl_state  *pinstate;
	struct pinctrl_state  *pinstate_sleep;
	unsigned int rate;
	unsigned int active;
	bool configured;
};

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

/* Origin freq convert */
static const struct sample_rate sample_rate_orig[] = {
	{44100, 0xF},
	{48000, 0xD},
	{24000, 0x9},
	{32000, 0xC},
	{96000, 0x5},
	{192000, 0x1},
	{22050, 0xB},
	{88200, 0x7},
	{178400, 0x3},
};

static const struct sample_rate sample_rate_freq[] = {
	{44100, 0x0},
	{48000, 0x2},
	{24000, 0x6},
	{32000, 0x3},
	{96000, 0xA},
	{192000, 0xE},
	{22050, 0x4},
	{88200, 0x8},
	{176400, 0xC},
};


static int sunxi_spdif_set_audio_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct snd_soc_dai *dai = card->rtd->cpu_dai;
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	unsigned int reg_val;

	regmap_read(sunxi_spdif->regmap, SUNXI_SPDIF_TXCH_STA0, &reg_val);

	switch(ucontrol->value.integer.value[0]) {
	case	0:
	case	1:
		reg_val = 0;
		break;
	case	2:
		reg_val = 1;
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG,
			(1<<TXCFG_DATA_TYPE), (reg_val<<TXCFG_DATA_TYPE));
	regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCH_STA0,
			(1<<TXCHSTA0_AUDIO), (reg_val<<TXCHSTA0_AUDIO));
	regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_RXCH_STA0,
			(1<<RXCHSTA0_AUDIO), (reg_val<<RXCHSTA0_AUDIO));

	return 0;
}

static int sunxi_spdif_get_audio_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct snd_soc_dai *dai = card->rtd->cpu_dai;
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	unsigned int reg_val;

	regmap_read(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG, &reg_val);
	reg_val = (reg_val & (1<<TXCFG_DATA_TYPE)) ? 1 : 0;
	ucontrol->value.integer.value[0] = reg_val + 1;
	return 0;
}

static const char *spdif_format_function[] = {"null", "pcm", "DTS"};
static const struct soc_enum spdif_format_enum[] = {
        SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spdif_format_function), spdif_format_function),
};


static int sunxi_spdif_get_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct snd_soc_dai *dai = card->rtd->cpu_dai;
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	unsigned int reg_val;

	regmap_read(sunxi_spdif->regmap, SUNXI_SPDIF_FIFO_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (1<<FIFO_CTL_HUBEN)) ? 2 : 1);
	return 0;
}

static int sunxi_spdif_set_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct snd_soc_dai *dai = card->rtd->cpu_dai;
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
	case	1:
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_FIFO_CTL,
				(1<<FIFO_CTL_HUBEN), (0<<FIFO_CTL_HUBEN));
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG,
				(1<<TXCFG_TXEN), (0<<TXCFG_TXEN));
		break;
	case	2:
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_FIFO_CTL,
				(1<<FIFO_CTL_HUBEN), (1<<FIFO_CTL_HUBEN));
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG,
				(1<<TXCFG_TXEN), (1<<TXCFG_TXEN));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


/* sunxi spdif hub mdoe select */
static const char *spdif_hub_function[] = {"null" , "hub_disable", "hub_enable"};

static const struct soc_enum spdif_hub_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spdif_hub_function),
			spdif_hub_function),
};

/* dts pcm Audio Mode Select */
static const struct snd_kcontrol_new sunxi_spdif_controls[] = {
	SOC_ENUM_EXT("spdif audio format Function", spdif_format_enum[0], sunxi_spdif_get_audio_mode, sunxi_spdif_set_audio_mode),

	SOC_ENUM_EXT("sunxi spdif hub mode" , spdif_hub_mode_enum[0], sunxi_spdif_get_hub_mode, sunxi_spdif_set_hub_mode),
};

static void sunxi_spdif_txctrl_enable(struct sunxi_spdif_info *sunxi_spdif, int enable)
{
	if(enable) {
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG, (1<<TXCFG_TXEN), (1<<TXCFG_TXEN));
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_INT, (1<<INT_TXDRQEN), (1<<INT_TXDRQEN));
	} else {
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG, (1<<TXCFG_TXEN), (0<<TXCFG_TXEN));
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_INT, (1<<INT_TXDRQEN), (0<<INT_TXDRQEN));
	}
}

static void sunxi_spdif_rxctrl_enable(struct sunxi_spdif_info *sunxi_spdif, int enable)
{
	if(enable) {
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_INT, (1<<INT_RXDRQEN), (1<<INT_RXDRQEN));
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_RXCFG, (1<<RXCFG_RXEN), (1<<RXCFG_RXEN));
	} else {
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_RXCFG, (1<<RXCFG_RXEN), (0<<RXCFG_RXEN));
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_INT, (1<<INT_RXDRQEN), (0<<INT_RXDRQEN));
	}
}

static void sunxi_spdif_init(struct sunxi_spdif_info *sunxi_spdif)
{
	/* FIFO CTL register default setting */
	regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_FIFO_CTL,
				(CTL_TXTL_MASK<<FIFO_CTL_TXTL), (16<<FIFO_CTL_TXTL));
	regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_FIFO_CTL,
				(CTL_RXTL_MASK<<FIFO_CTL_RXTL), (15<<FIFO_CTL_RXTL));

	regmap_write(sunxi_spdif->regmap, SUNXI_SPDIF_TXCH_STA0, 2<<TXCHSTA0_CHNUM);
	regmap_write(sunxi_spdif->regmap, SUNXI_SPDIF_RXCH_STA0, 2<<RXCHSTA0_CHNUM);
}

static int sunxi_spdif_dai_hw_params(struct snd_pcm_substream *substream, 
			struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	unsigned int reg_temp;
	unsigned int i;
	unsigned int origin_freq_bit = 0, sample_freq_bit = 0;

	/* two substream should be warking on same samplerate */
	mutex_lock(&sunxi_spdif->mutex);
	if(sunxi_spdif->active > 1) {
		if(params_rate(params) != sunxi_spdif->rate) {
			mutex_unlock(&sunxi_spdif->mutex);
			return -EINVAL;
		}
	}
	mutex_unlock(&sunxi_spdif->mutex);

	switch(params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		reg_temp = 0;
		break;
	case	SNDRV_PCM_FORMAT_S20_3LE:
		reg_temp = 1;
		break;
	case	SNDRV_PCM_FORMAT_S24_LE:
		reg_temp = 2;
		break;
	default:
		pr_debug("[sunxi-spdif]Invaild format set\n");
		return -EINVAL;
	}

	for(i=0; i<ARRAY_SIZE(sample_rate_orig); i++) {
		if(params_rate(params) == sample_rate_orig[i].samplerate) {
			origin_freq_bit = sample_rate_orig[i].rate_bit;
			break;
		}
	}

	for(i=0; i<ARRAY_SIZE(sample_rate_freq); i++) {
		if(params_rate(params) == sample_rate_freq[i].samplerate) {
			sample_freq_bit = sample_rate_freq[i].rate_bit;
			sunxi_spdif->rate = sample_rate_freq[i].samplerate;
			break;
		}
	}

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG,
			(3<<TXCFG_SAMPLE_BIT), (reg_temp<<TXCFG_SAMPLE_BIT));

		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_FIFO_CTL,
					(1<<FIFO_CTL_TXIM), (1<<FIFO_CTL_TXIM));

		if(params_channels(params) == 1) {
			regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG,
					(1<<TXCFG_SINGLE_MOD), (1<<TXCFG_SINGLE_MOD));
		} else {
			regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG,
					(1<<TXCFG_SINGLE_MOD), (0<<TXCFG_SINGLE_MOD));
		}

		/* samplerate convertion */
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCH_STA0,
			(0xF<<TXCHSTA0_SAMFREQ), (sample_freq_bit<<TXCHSTA0_SAMFREQ));
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCH_STA1,
			(0xF<<TXCHSTA1_ORISAMFREQ), (origin_freq_bit<<TXCHSTA1_ORISAMFREQ));
		switch(reg_temp) {
		case	0:
			regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCH_STA1,
				(0xF<<TXCHSTA1_MAXWORDLEN), (2<<TXCHSTA1_MAXWORDLEN));
			break;
		case	1:
			regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCH_STA1,
				(0xF<<TXCHSTA1_MAXWORDLEN), (0xC<<TXCHSTA1_MAXWORDLEN));
			break;
		case	2:
			regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCH_STA1,
				(0xF<<TXCHSTA1_MAXWORDLEN), (0xB<<TXCHSTA1_MAXWORDLEN));
			break;
		default:
			pr_debug("[sunxi-spdif]unexpection error\n");
			return -EINVAL;
		}
	} else {
		/* FIXME, not sync as spec says, just test 16bit & 24bit, using 3 working ok */
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_FIFO_CTL,
					(3<<FIFO_CTL_RXOM), (3<<FIFO_CTL_RXOM));

		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_RXCH_STA0,
			(0xF<<RXCHSTA0_SAMFREQ), (sample_freq_bit<<RXCHSTA0_SAMFREQ));
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_RXCH_STA1,
			(0xF<<RXCHSTA1_ORISAMFREQ), (origin_freq_bit<<RXCHSTA1_ORISAMFREQ));

		switch(reg_temp) {
		case	0:
			regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_RXCH_STA1,
				(0xF<<RXCHSTA1_MAXWORDLEN), (2<<RXCHSTA1_MAXWORDLEN));
			break;
		case	1:
			regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_RXCH_STA1,
				(0xF<<RXCHSTA1_MAXWORDLEN), (0xC<<RXCHSTA1_MAXWORDLEN));
			break;
		case	2:
			regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_RXCH_STA1,
				(0xF<<RXCHSTA1_MAXWORDLEN), (0xB<<RXCHSTA1_MAXWORDLEN));
			break;
		default:
			pr_debug("[sunxi-spdif]unexpection error\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int sunxi_spdif_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,
						unsigned int freq, int dir)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	pr_debug("Enter %s\n", __func__);
	mutex_lock(&sunxi_spdif->mutex);
	if(sunxi_spdif->active == 0) {
		pr_debug("active: %u\n", sunxi_spdif->active);
		if(clk_set_rate(sunxi_spdif->pllclk, freq)) {
			dev_err(sunxi_spdif->dev, "pllclk set rate to %uHz failed\n", freq);
			mutex_unlock(&sunxi_spdif->mutex);
			return -EBUSY;
		}
	}
	sunxi_spdif->active++;
	mutex_unlock(&sunxi_spdif->mutex);
	pr_debug("End %s\n", __func__);
	return 0;
}

static int sunxi_spdif_dai_set_clkdiv(struct snd_soc_dai *dai, int clk_id, int clk_div)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);

	pr_debug("Enter %s\n", __func__);

	mutex_lock(&sunxi_spdif->mutex);
	if(sunxi_spdif->configured == false) {
		switch(clk_id) {
		case	0:
			regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_TXCFG,
				(0x1F<<TXCFG_CLK_DIV_RATIO), ((clk_div-1)<<TXCFG_CLK_DIV_RATIO));
			break;
		case	1:
			break;
		default:
			break;
		}
	}
	sunxi_spdif->configured = true;
	mutex_unlock(&sunxi_spdif->mutex);

	pr_debug("End %s\n", __func__);

	return 0;
}

static int sunxi_spdif_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_dai_set_dma_data(dai, substream, &sunxi_spdif->playback_dma_param);
	}
	else {
		snd_soc_dai_set_dma_data(dai, substream, &sunxi_spdif->capture_dma_param);
	}

	return 0;
}

static void sunxi_spdif_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);

	mutex_lock(&sunxi_spdif->mutex);
	if(sunxi_spdif->active)
	{
		sunxi_spdif->active--;
		if(sunxi_spdif->active == 0)
			sunxi_spdif->configured = false;
	}
	mutex_unlock(&sunxi_spdif->mutex);
}

static int sunxi_spdif_trigger(struct snd_pcm_substream *substream,
					int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	switch(cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_spdif_txctrl_enable(sunxi_spdif, 1);
		} else {
			sunxi_spdif_rxctrl_enable(sunxi_spdif, 1);
		}
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_spdif_txctrl_enable(sunxi_spdif, 0);
		} else {
			sunxi_spdif_rxctrl_enable(sunxi_spdif, 0);
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_ARCH_SUN8IW10
static bool spdif_loop_en = false;
module_param_named(spdif_loop_en, spdif_loop_en, bool, S_IRUGO | S_IWUSR);
#endif

/* Flush FIFO & Interrupt */
static int sunxi_spdif_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	unsigned int reg_val;

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
#ifdef	CONFIG_ARCH_SUN8IW10
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_CTL,
					(1<<CTL_LOOP_EN), (spdif_loop_en<<CTL_LOOP_EN));
#endif
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_FIFO_CTL,
					(1<<FIFO_CTL_FTX), (1<<FIFO_CTL_FTX));
		regmap_write(sunxi_spdif->regmap, SUNXI_SPDIF_TXCNT, 0);
	} else {
		regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_FIFO_CTL,
					(1<<FIFO_CTL_FRX), (1<<FIFO_CTL_FRX));
		regmap_write(sunxi_spdif->regmap, SUNXI_SPDIF_RXCNT, 0);
	}

	/* clear all interrupt status */
	regmap_read(sunxi_spdif->regmap, SUNXI_SPDIF_INT_STA, &reg_val);
	regmap_write(sunxi_spdif->regmap, SUNXI_SPDIF_INT_STA, reg_val);

	return 0;
}

static int sunxi_spdif_probe(struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	int ret;

	mutex_init(&sunxi_spdif->mutex);

	ret = snd_soc_add_card_controls(dai->card, sunxi_spdif_controls, ARRAY_SIZE(sunxi_spdif_controls));
	if(ret)
		dev_warn(sunxi_spdif->dev, "Failed to register audio mode control, will continue without it.\n");

	sunxi_spdif_init(sunxi_spdif);
	regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_CTL, (1<<CTL_GEN_EN), (1<<CTL_GEN_EN));

	return 0;
}

static int sunxi_spdif_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_spdif_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	int	ret;

	pr_debug("[SPDIF]Enter %s\n", __func__);

	regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_CTL,
				(1<<CTL_GEN_EN), (0<<CTL_GEN_EN));

	if (sunxi_spdif->pinstate_sleep) {
		ret = pinctrl_select_state(sunxi_spdif->pinctrl, sunxi_spdif->pinstate_sleep);
		if(ret) {
			dev_err(sunxi_spdif->dev, "pinstate sleep select failed\n");
			return ret;
		}
	}

	if (sunxi_spdif->pinctrl != NULL)
		devm_pinctrl_put(sunxi_spdif->pinctrl);

	pr_debug("[sunxi-spdif]sunxi_spdif->clk_enable: %d\n",sunxi_spdif->active);

	sunxi_spdif->pinctrl = NULL;
	sunxi_spdif->pinstate = NULL;
	sunxi_spdif->pinstate_sleep = NULL;
	if(sunxi_spdif->moduleclk != NULL)
		clk_disable_unprepare(sunxi_spdif->moduleclk);
	if(sunxi_spdif->pllclk != NULL)
		clk_disable_unprepare(sunxi_spdif->pllclk);

	pr_debug("[SPDIF]End %s\n", __func__);

	return 0;
}

static int sunxi_spdif_resume(struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	int	ret;

	pr_debug("[sunxi-spdif]Enter %s\n", __func__);

	if(sunxi_spdif->pllclk != NULL)
		ret = clk_prepare_enable(sunxi_spdif->pllclk);
	if(sunxi_spdif->moduleclk != NULL)
		clk_prepare_enable(sunxi_spdif->moduleclk);

	if(sunxi_spdif->pinctrl == NULL) {
		sunxi_spdif->pinctrl = devm_pinctrl_get(sunxi_spdif->dev);
		if(IS_ERR_OR_NULL(sunxi_spdif->pinctrl)) {
			dev_err(sunxi_spdif->dev, "Can't get sunxi spdif pinctrl\n");
			return -EBUSY;
		}
	}

	if (!sunxi_spdif->pinstate) {
		sunxi_spdif->pinstate = pinctrl_lookup_state(sunxi_spdif->pinctrl, PINCTRL_STATE_DEFAULT);
		if(IS_ERR_OR_NULL(sunxi_spdif->pinstate)) {
			dev_err(sunxi_spdif->dev, "Can't get sunxi spdif pinctrl default state\n");
			return -EBUSY;
		}
	}

	if (!sunxi_spdif->pinstate_sleep) {
		sunxi_spdif->pinstate_sleep = pinctrl_lookup_state(sunxi_spdif->pinctrl, PINCTRL_STATE_SLEEP);
		if(IS_ERR_OR_NULL(sunxi_spdif->pinstate_sleep)) {
			dev_err(sunxi_spdif->dev, "Can't get sunxi spdif pinctrl sleep state\n");
			return -EINVAL;
		}
	}

	ret = pinctrl_select_state(sunxi_spdif->pinctrl, sunxi_spdif->pinstate);
	if(ret) {
		dev_err(sunxi_spdif->dev, "select pin default state failed\n");
		return ret;
	}

	sunxi_spdif_init(sunxi_spdif);
	regmap_update_bits(sunxi_spdif->regmap, SUNXI_SPDIF_CTL,
				(1<<CTL_GEN_EN), (1<<CTL_GEN_EN));

	pr_debug("[sunxi-spdif]End %s\n", __func__);
	return 0;
}

#define SUNXI_SPDIF_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sunxi_spdif_dai_ops = {
	.hw_params 	= sunxi_spdif_dai_hw_params,
	.set_clkdiv 	= sunxi_spdif_dai_set_clkdiv,
	.set_sysclk 	= sunxi_spdif_dai_set_sysclk,
	.startup	= sunxi_spdif_dai_startup,
	.shutdown	= sunxi_spdif_dai_shutdown,
	.trigger 	= sunxi_spdif_trigger,
	.prepare	= sunxi_spdif_prepare,
};

static struct snd_soc_dai_driver sunxi_spdif_dai = {
	.probe 		= sunxi_spdif_probe,
	.suspend 	= sunxi_spdif_suspend,
	.resume 	= sunxi_spdif_resume,
	.remove		= sunxi_spdif_remove,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_SPDIF_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE|SNDRV_PCM_FMTBIT_S20_3LE| SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_SPDIF_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE|SNDRV_PCM_FMTBIT_S20_3LE| SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops = &sunxi_spdif_dai_ops,
};

static const struct snd_soc_component_driver sunxi_spdif_component = {
	.name		= DRV_NAME,
};

static const struct regmap_config sunxi_spdif_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_SPDIF_RXCH_STA1,
	.cache_type = REGCACHE_NONE,
};

static int  sunxi_spdif_dev_probe(struct platform_device *pdev)
{
	struct resource res, *memregion;
	struct device_node *node = pdev->dev.of_node;
	void __iomem *sunxi_spdif_membase;
	struct sunxi_spdif_info *sunxi_spdif;
	int	ret;

	sunxi_spdif = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_spdif_info), GFP_KERNEL);
	if(!sunxi_spdif) {
		dev_err(&pdev->dev, "Can't allocate sunxi_spdif memory\n");
		ret = -ENOMEM;
		goto err_node_put;
	}
	dev_set_drvdata(&pdev->dev, sunxi_spdif);
	sunxi_spdif->dev = &pdev->dev;
	sunxi_spdif->dai = sunxi_spdif_dai;
	sunxi_spdif->dai.name = dev_name(&pdev->dev);

	ret = of_address_to_resource(node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "Can't parse device node resource\n");
		return -ENODEV;
	}

	memregion = devm_request_mem_region(&pdev->dev, res.start,
					    resource_size(&res), DRV_NAME);
	if (memregion == NULL) {
		dev_err(&pdev->dev, "Memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	sunxi_spdif_membase = ioremap(res.start, resource_size(&res));
	if(sunxi_spdif_membase == NULL) {
		dev_err(&pdev->dev, "Can't remap sunxi spdif registers\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	}

	sunxi_spdif->regmap = devm_regmap_init_mmio(&pdev->dev, sunxi_spdif_membase, &sunxi_spdif_regmap_config);
	if(IS_ERR(sunxi_spdif->regmap)) {
		dev_err(&pdev->dev, "regmap sunxi spdif membase failed\n");
		ret = PTR_ERR(sunxi_spdif->regmap);
		goto err_iounmap;
	}

	sunxi_spdif->pllclk = of_clk_get(node, 0);
	sunxi_spdif->moduleclk = of_clk_get(node, 1);
	if (IS_ERR(sunxi_spdif->pllclk) || IS_ERR(sunxi_spdif->moduleclk)){
		dev_err(&pdev->dev, "Can't get spdif clocks\n");
		if (IS_ERR(sunxi_spdif->pllclk)) {
			ret = PTR_ERR(sunxi_spdif->pllclk);
			goto err_iounmap;
		}
		else {
			ret = PTR_ERR(sunxi_spdif->moduleclk);
			goto err_pllclk_put;
		}
	} else {
		if (clk_set_parent(sunxi_spdif->moduleclk, sunxi_spdif->pllclk)) {
			dev_err(&pdev->dev, "set parent of moduleclk to pllclk failed! line = %d\n",__LINE__);
		}
		clk_prepare_enable(sunxi_spdif->pllclk);
		clk_prepare_enable(sunxi_spdif->moduleclk);
	}

	sunxi_spdif->playback_dma_param.dma_addr = res.start + SUNXI_SPDIF_TXFIFO;
	sunxi_spdif->playback_dma_param.dma_drq_type_num = DRQDST_SPDIFTX;
	sunxi_spdif->playback_dma_param.dst_maxburst = 8;
	sunxi_spdif->playback_dma_param.src_maxburst = 8;

	sunxi_spdif->capture_dma_param.dma_addr = res.start + SUNXI_SPDIF_RXFIFO;
	sunxi_spdif->capture_dma_param.dma_drq_type_num = DRQSRC_SPDIFRX;
	sunxi_spdif->capture_dma_param.src_maxburst = 8;
	sunxi_spdif->capture_dma_param.dst_maxburst = 8;

	sunxi_spdif->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(sunxi_spdif->pinctrl)) {
		dev_err(&pdev->dev, "request pinctrl handle for audio failed\n");
		ret = -EINVAL;
		goto err_moduleclk_put;
	}

	sunxi_spdif->pinstate = pinctrl_lookup_state(sunxi_spdif->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(sunxi_spdif->pinstate)) {
		dev_err(&pdev->dev, "lookup pin default state failed\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	sunxi_spdif->pinstate_sleep = pinctrl_lookup_state(sunxi_spdif->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(sunxi_spdif->pinstate_sleep)) {
		dev_err (&pdev->dev, "lookup pin sleep state failed\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	ret = snd_soc_register_component(&pdev->dev, &sunxi_spdif_component, &sunxi_spdif->dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -ENOMEM;
		goto err_pinctrl_put;
	}

	ret = asoc_dma_platform_register(&pdev->dev,0);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		ret = -ENOMEM;
		goto err_unregister_component;
	}

	return 0;

err_unregister_component:
	snd_soc_unregister_component(&pdev->dev);
err_pinctrl_put:
	devm_pinctrl_put(sunxi_spdif->pinctrl);
err_moduleclk_put:
	clk_put(sunxi_spdif->moduleclk);
err_pllclk_put:
	clk_put(sunxi_spdif->pllclk);
err_iounmap:
	iounmap(sunxi_spdif_membase);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_spdif);
err_node_put:
	of_node_put(node);
	return ret;
}

static int __exit sunxi_spdif_dev_remove(struct platform_device *pdev)
{
	struct sunxi_spdif_info *sunxi_spdif = dev_get_drvdata(&pdev->dev);

	snd_soc_unregister_component(&pdev->dev);
	clk_put(sunxi_spdif->moduleclk);
	clk_put(sunxi_spdif->pllclk);
	devm_kfree(&pdev->dev, sunxi_spdif);
	return 0;
}

static const struct of_device_id sunxi_spdif_of_match[] = {
	{ .compatible = "allwinner,sunxi-spdif", },
	{},
};

static struct platform_driver sunxi_spdif_driver = {
	.probe = sunxi_spdif_dev_probe,
	.remove = __exit_p(sunxi_spdif_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_spdif_of_match,
	},
};

module_platform_driver(sunxi_spdif_driver);

MODULE_AUTHOR("wolfgang huang <huangjinhui@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI SPDIF ASoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-spdif");

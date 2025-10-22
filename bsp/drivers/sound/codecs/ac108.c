// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * ac108.c -- AC108 ALSA SoC Audio driver
 *
 * Copyright (c) 2022 Allwinnertech Ltd.
 */

#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm_params.h>

#include "ac108.h"

struct ac108_real_to_reg {
	unsigned int real;
	unsigned int reg;
};

struct ac108_pll_div {
	unsigned int freq_in;
	unsigned int freq_out;
	unsigned int m1:5;
	unsigned int m2:1;
	unsigned int n:10;
	unsigned int k1:5;
	unsigned int k2:1;
};

static const struct reg_default ac108_reg_defaults[] = {
	{ 0x00, 0x4a },

	{ 0x01, 0x00 },
	{ 0x03, 0x00 },
	{ 0x06, 0x00 },
	{ 0x07, 0x00 },
	{ 0x09, 0x00 },

	{ 0x10, 0x48 },
	{ 0x11, 0x00 },
	{ 0x12, 0x03 },
	{ 0x13, 0x0d },
	{ 0x14, 0x00 },
	{ 0x16, 0x0f },
	{ 0x17, 0xd0 },
	{ 0x18, 0x00 },

	{ 0x20, 0x00 },
	{ 0x21, 0x00 },
	{ 0x22, 0x00 },
	{ 0x25, 0x30 },

	{ 0x30, 0xc0 },
	{ 0x31, 0x00 },
	{ 0x32, 0x00 },
	{ 0x33, 0x00 },
	{ 0x34, 0x00 },
	{ 0x35, 0x55 },
	{ 0x36, 0x60 },

	{ 0x38, 0x00 },
	{ 0x39, 0x00 },
	{ 0x3A, 0x00 },
	{ 0x3C, 0x00 },
	{ 0x3D, 0x00 },
	{ 0x3E, 0x00 },
	{ 0x3F, 0x00 },

	{ 0x40, 0x00 },
	{ 0x41, 0x00 },
	{ 0x42, 0x00 },
	{ 0x44, 0x00 },
	{ 0x45, 0x00 },
	{ 0x46, 0x00 },
	{ 0x47, 0x00 },

	{ 0x60, 0x00 },
	{ 0x61, 0x00 },
	{ 0x62, 0x00 },
	{ 0x63, 0xe4 },
	{ 0x65, 0x00 },

	{ 0x66, 0x0f },
	{ 0x67, 0x00 },
	{ 0x68, 0xff },
	{ 0x69, 0xaa },
	{ 0x6A, 0x45 },

	{ 0x70, 0xa0 },
	{ 0x71, 0xa0 },
	{ 0x72, 0xa0 },
	{ 0x73, 0xa0 },

	{ 0x76, 0x01 },
	{ 0x77, 0x02 },
	{ 0x78, 0x04 },
	{ 0x79, 0x08 },

	{ 0x7F, 0x00 },

	{ 0x80, 0x11 },
	{ 0x81, 0x11 },

	{ 0x90, 0x00 },
	{ 0x91, 0x00 },
	{ 0x92, 0x00 },
	{ 0x93, 0x00 },

	{ 0x96, 0x00 },
	{ 0x97, 0x00 },
	{ 0x98, 0x00 },
	{ 0x99, 0x00 },
	{ 0x9A, 0x00 },
	{ 0x9B, 0x00 },
	{ 0x9C, 0x00 },
	{ 0x9D, 0x00 },
	{ 0x9E, 0x00 },
	{ 0x9F, 0x00 },

	{ 0xA0, 0x00 },
	{ 0xA1, 0x00 },
	{ 0xA2, 0x03 },
	{ 0xA3, 0x00 },
	{ 0xA4, 0x00 },
	{ 0xA5, 0x00 },
	{ 0xA6, 0x00 },

	{ 0xA7, 0x00 },
	{ 0xA8, 0x00 },
	{ 0xA9, 0x00 },
	{ 0xAA, 0x00 },
	{ 0xAB, 0x00 },
	{ 0xAC, 0x00 },
	{ 0xAD, 0x00 },

	{ 0xAE, 0x00 },
	{ 0xAF, 0x00 },
	{ 0xB0, 0x00 },
	{ 0xB1, 0x00 },
	{ 0xB2, 0x00 },
	{ 0xB3, 0x00 },
	{ 0xB4, 0x00 },

	{ 0xB5, 0x00 },
	{ 0xB6, 0x00 },
	{ 0xB7, 0x00 },
	{ 0xB8, 0x00 },
	{ 0xB9, 0x00 },
	{ 0xBA, 0x00 },
	{ 0xBB, 0x00 },

	{ 0xC0, 0x77 },
	{ 0xC1, 0x77 },
	{ 0xC2, 0x00 },
	{ 0xC3, 0x55 },
	{ 0xC4, 0x00 },
	{ 0xC5, 0x00 },
	{ 0xC6, 0x00 },
	{ 0xC7, 0x00 },
};

/* PLLCLK: FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] */
static const struct ac108_pll_div ac108_pll_div[] = {
	{400000,   24576000, 0,  0, 983,  7,  1},	/* 24.575M */
	{512000,   24576000, 0,  0, 960,  9,  1},	/* 24576000/48 */
	{768000,   24576000, 0,  0, 640,  9,  1},	/* 24576000/32 */
	{800000,   24576000, 0,  0, 768,  24, 0},
	{1024000,  24576000, 0,  0, 480,  9,  1},	/* 24576000/24 */
	{1600000,  24576000, 0,  0, 384,  24, 0},
	{2048000,  24576000, 0,  0, 240,  9,  1},	/* 24576000/12 */
	{3072000,  24576000, 0,  0, 160,  9,  1},	/* 24576000/8 */
	{4096000,  24576000, 0,  0, 120,  9,  1},	/* 24576000/6 */
	{6000000,  24576000, 4,  0, 512,  24, 0},
	{12000000, 24576000, 9,  0, 512,  24, 0},
	{13000000, 24576000, 12, 0, 639,  12, 1},	/* 24.577M */
	{15360000, 24576000, 9,  0, 320,  9,  1},
	{16000000, 24576000, 9,  0, 384,  24, 0},
	{19200000, 24576000, 11, 0, 384,  24, 0},
	{19680000, 24576000, 15, 1, 999,  24, 0},	/* 24.575M */
	{24000000, 24576000, 9,  0, 256,  24, 0},

	{400000,   22579200, 0,  0, 1016, 8,  1},	/* 22.5778M */
	{512000,   22579200, 0,  0, 882,  9,  1},
	{768000,   22579200, 0,  0, 588,  9,  1},
	{800000,   22579200, 0,  0, 508,  8,  1},	/* 22.5778M */
	{1024000,  22579200, 0,  0, 441,  9,  1},
	{1600000,  22579200, 0,  0, 254,  8,  1},	/* 22.5778M */
	{2048000,  22579200, 1,  0, 441,  9,  1},
	{3072000,  22579200, 2,  0, 441,  9,  1},
	{4096000,  22579200, 3,  0, 441,  9,  1},
	{6000000,  22579200, 5,  0, 429,  18, 0},	/* 22.5789M */
	{12000000, 22579200, 11, 0, 429,  18, 0},	/* 22.5789M */
	{13000000, 22579200, 12, 0, 429,  18, 0},	/* 22.5789M */
	{15360000, 22579200, 14, 0, 441,  9,  1},
	{16000000, 22579200, 24, 0, 882,  24, 0},
	{19200000, 22579200, 4,  0, 147,  24, 0},
	{19680000, 22579200, 13, 1, 771,  23, 0},	/* 22.5793M */
	{24000000, 22579200, 24, 0, 588,  24, 0},

	{12288000, 24576000, 9,  0, 400,  9,  1},	/* 24576000/2 */
	{11289600, 22579200, 9,  0, 400,  9,  1},	/* 22579200/2 */

	{24576000 / 1,   24576000, 9,  0, 200, 9, 1},	/* 24576000 */
	{24576000 / 4,   24576000, 4,  0, 400, 9, 1},	/* 6144000 */
	{24576000 / 16,  24576000, 0,  0, 320, 9, 1},	/* 1536000 */
	{24576000 / 64,  24576000, 0,  0, 640, 4, 1},	/* 384000 */
	{24576000 / 96,  24576000, 0,  0, 960, 4, 1},	/* 256000 */
	{24576000 / 128, 24576000, 0,  0, 512, 1, 1},	/* 192000 */
	{24576000 / 176, 24576000, 0,  0, 880, 4, 0},	/* 140000 */
	{24576000 / 192, 24576000, 0,  0, 960, 4, 0},	/* 128000 */

	{22579200 / 1,   22579200, 9,  0, 200, 9, 1},	/* 22579200 */
	{22579200 / 4,   22579200, 4,  0, 400, 9, 1},	/* 5644800 */
	{22579200 / 16,  22579200, 0,  0, 320, 9, 1},	/* 1411200 */
	{22579200 / 64,  22579200, 0,  0, 640, 4, 1},	/* 352800 */
	{22579200 / 96,  22579200, 0,  0, 960, 4, 1},	/* 235200 */
	{22579200 / 128, 22579200, 0,  0, 512, 1, 1},	/* 176400 */
	{22579200 / 176, 22579200, 0,  0, 880, 4, 0},	/* 128290 */
	{22579200 / 192, 22579200, 0,  0, 960, 4, 0},	/* 117600 */

	{22579200 / 6,   22579200, 2,  0, 360, 9, 1},	/* 3763200 */
	{22579200 / 8,   22579200, 0,  0, 160, 9, 1},	/* 2822400 */
	{22579200 / 12,  22579200, 0,  0, 240, 9, 1},	/* 1881600 */
	{22579200 / 24,  22579200, 0,  0, 480, 9, 1},	/* 940800 */
	{22579200 / 32,  22579200, 0,  0, 640, 9, 1},	/* 705600 */
	{22579200 / 48,  22579200, 0,  0, 960, 9, 1},	/* 470400 */
};

static const struct ac108_real_to_reg ac108_sample_bit[] = {
	{8,  1},
	{12, 2},
	{16, 3},
	{20, 4},
	{24, 5},
	{28, 6},
	{32, 7},
};

static const struct ac108_real_to_reg ac108_sample_rate[] = {
	{8000, 0},
	{11025, 1},
	{12000, 2},
	{16000, 3},
	{22050, 4},
	{24000, 5},
	{32000, 6},
	{44100, 7},
	{48000, 8},
	{96000, 89},
};

static const struct ac108_real_to_reg ac108_slot_width[] = {
	{8,  1},
	{12, 2},
	{16, 3},
	{20, 4},
	{24, 5},
	{28, 6},
	{32, 7},
};

static const struct ac108_real_to_reg ac108_bclk_div[] = {
	{1, 1},
	{2, 2},
	{4, 3},
	{6, 4},
	{8, 5},
	{12, 6},
	{16, 7},
	{24, 8},
	{32, 9},
	{48, 10},
	{64, 11},
	{96, 12},
	{128, 13},
	{176, 14},
	{192, 15},
};

DEFINE_SPINLOCK(route_status_lock);

struct ac108_priv {
	struct regmap *regmap;
	unsigned int sysclk_freq;
	unsigned int fmt;
	int slots;
	int slot_width;

	bool dmic1l_en;
	bool dmic1r_en;
	bool dmic2l_en;
	bool dmic2r_en;

	struct ac108_data pdata;
};

static int ac108_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;

/* TODO: use for AMIC only, but DMIC will call this function */
	/* ALDO enable */
	regmap_update_bits(regmap, PWR_CTRL6, 0x1 << LDO33ANA_ENABLE, 0x1 << LDO33ANA_ENABLE);

	/* VREF faststart Enable,
	 * Enable VREF @ 3.4V (AVDD5V) or 3.1V (AVDD3.3V) (needed for Analog LDO and MICBIAS)
	 * VREFP faststart Enable,
	 * Enable VREFP (needed by all audio input channels)
	 */
	regmap_write(regmap, PWR_CTRL7, 0x9b);
	regmap_write(regmap, PWR_CTRL9, 0x81);

	/* DSM low power mode enable, Control bias current for DSM integrator opamps */
	regmap_update_bits(regmap, ANA_ADC3_CTRL7, 0x7 << DSM_OTA_IB_SEL, 0x3 << DSM_OTA_IB_SEL);
	regmap_update_bits(regmap, ANA_ADC3_CTRL7, 0x1 << DSM_LPMODE, 0x1 << DSM_LPMODE);

	/* delay 50ms to let VREF/VRP faststart powerup stable,
	 * then disable faststart VREF & VREFP fast start disable.
	 */
	msleep(50);
	regmap_update_bits(regmap, PWR_CTRL7,
			   0x1 << VREF_FASTSTART_ENABLE, 0x0 << VREF_FASTSTART_ENABLE);
	regmap_update_bits(regmap, PWR_CTRL9,
			   0x1 << VREFP_FASTSTART_ENABLE, 0x0 << VREFP_FASTSTART_ENABLE);
/* TODO: end */

	/* module enable */
	regmap_update_bits(regmap, MOD_CLK_EN,
			   0x1 << MIC_OFFSET_CALIBRATION, 0x1 << MIC_OFFSET_CALIBRATION);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << ADC_DIGITAL, 0x1 << ADC_DIGITAL);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << ADC_ANALOG, 0x1 << ADC_ANALOG);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << I2S, 0x1 << I2S);
	/* MIC Offset Calibration & ADC & I2S de-asserted */
	regmap_update_bits(regmap, MOD_RST_CTRL,
			   0x1 << MIC_OFFSET_CALIBRATION, 0x1 << MIC_OFFSET_CALIBRATION);
	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << ADC_DIGITAL, 0x1 << ADC_DIGITAL);
	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << ADC_ANALOG, 0x1 << ADC_ANALOG);
	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << I2S, 0x1 << I2S);

	return 0;
}

static void ac108_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;

	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << I2S, 0x0 << I2S);
	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << ADC_ANALOG, 0x0 << ADC_ANALOG);
	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << ADC_DIGITAL, 0x0 << ADC_DIGITAL);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << I2S, 0x0 << I2S);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << ADC_ANALOG, 0x0 << ADC_ANALOG);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << ADC_DIGITAL, 0x0 << ADC_DIGITAL);
}

static int ac108_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct ac108_data *pdata = &ac108->pdata;
	struct regmap *regmap = ac108->regmap;
	int i;
	unsigned int sample_bit;
	unsigned int sample_rate;
	unsigned int channels;
	unsigned int bclk_ratio;
	unsigned int sample_bit_reg = 0;
	unsigned int sample_rate_reg = 0;
	unsigned int channels_en_reg = 0;
	unsigned int bclk_ratio_reg = 0;

	/* set sample bit */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		sample_bit = 8;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		sample_bit = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		sample_bit = 20;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		sample_bit = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		sample_bit = 32;
		break;
	default:
		dev_err(dai->dev, "ac108 unsupport the sample bit\n");
		return -EINVAL;
	}
	for (i = 0; i < ARRAY_SIZE(ac108_sample_bit); i++) {
		if (ac108_sample_bit[i].real == sample_bit) {
			sample_bit_reg = ac108_sample_bit[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac108_sample_bit)) {
		dev_err(dai->dev, "ac108 unsupport the sample bit config: %u\n", sample_bit);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_FMT_CTRL2,
			   0x7 << SAMPLE_RESOLUTION, sample_bit_reg << SAMPLE_RESOLUTION);

	/* set sample rate */
	sample_rate = params_rate(params);
	for (i = 0; i < ARRAY_SIZE(ac108_sample_rate); i++) {
		if (ac108_sample_rate[i].real == sample_rate) {
			sample_rate_reg = ac108_sample_rate[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac108_sample_rate)) {
		dev_err(dai->dev, "ac108 unsupport the sample rate config: %u\n", sample_rate);
		return -EINVAL;
	}
	regmap_update_bits(regmap, ADC_SPRC, 0xf << ADC_FS_I2S1, sample_rate_reg << ADC_FS_I2S1);

	/* set channels */
	channels = params_channels(params);
	if (channels > 16) {
		dev_err(dai->dev, "ac108 unsupport the channels config: %u\n", channels);
		return -EINVAL;
	}
	for (i = 0; i < channels; i++)
		channels_en_reg |= (1 << i);
	regmap_update_bits(regmap, I2S_TX1_CTRL1, 0xf << TX1_CHSEL, (channels - 1) << TX1_CHSEL);
	regmap_write(regmap, I2S_TX1_CTRL2, (channels_en_reg & 0xff));
	regmap_write(regmap, I2S_TX1_CTRL3, (channels_en_reg >> 8));
	regmap_update_bits(regmap, I2S_TX2_CTRL1, 0xf << TX2_CHSEL, (channels - 1) << TX2_CHSEL);
	regmap_write(regmap, I2S_TX2_CTRL2, (channels_en_reg & 0xff));
	regmap_write(regmap, I2S_TX2_CTRL3, (channels_en_reg >> 8));

	/* set bclk div: ratio = sysclk / sample_rate / slots / slot_width */
	bclk_ratio = ac108->sysclk_freq / sample_rate / ac108->slots / ac108->slot_width;
	for (i = 0; i < ARRAY_SIZE(ac108_bclk_div); i++) {
		if (ac108_bclk_div[i].real == bclk_ratio) {
			bclk_ratio_reg = ac108_bclk_div[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac108_bclk_div)) {
		dev_err(dai->dev, "ac108 unsupport bclk_div: %d\n", bclk_ratio);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_BCLK_CTRL, 0xf << BCLKDIV, bclk_ratio_reg << BCLKDIV);

	/* PLLCLK enable */
	if (pdata->sysclk_src == SYSCLK_SRC_PLL)
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_EN, 0x1 << PLLCLK_EN);
	/* SYSCLK enable */
	regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_EN, 0x1 << SYSCLK_EN);
	/* ac108 ADC digital enable, I2S TX & Globle enable */
	regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << DG_EN, 0x1 << DG_EN);
	regmap_update_bits(regmap, I2S_CTRL, 0x1 << SDO1_EN, 0x1 << SDO1_EN);
	regmap_update_bits(regmap, I2S_CTRL, 0x1 << SDO2_EN, 0x1 << SDO2_EN);
	regmap_update_bits(regmap, I2S_CTRL, 0x1 << TXEN | 0x1 << GEN, 0x1 << TXEN | 0x1 << GEN);

	return 0;
}

static int ac108_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;

	regmap_update_bits(regmap, I2S_CTRL, 0x1 << TXEN | 0x1 << GEN, 0x0 << TXEN | 0x0 << GEN);
	regmap_update_bits(regmap, I2S_CTRL, 0x1 << SDO1_EN, 0x0 << SDO1_EN);
	regmap_update_bits(regmap, I2S_CTRL, 0x1 << SDO2_EN, 0x0 << SDO2_EN);
	regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << DG_EN, 0x0 << DG_EN);
	regmap_update_bits(regmap, PLL_CTRL1, 0x1 << PLL_COM_EN, 0x0 << PLL_COM_EN);
	regmap_update_bits(regmap, PLL_CTRL1, 0x1 << PLL_EN, 0x0 << PLL_EN);
	regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_EN, 0x0 << SYSCLK_EN);
	regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_EN, 0x0 << PLLCLK_EN);

	return 0;
}

static int ac108_set_dai_pll(struct snd_soc_dai *dai, int pll_id, int source,
			     unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct ac108_data *pdata = &ac108->pdata;
	struct regmap *regmap = ac108->regmap;
	unsigned int i  = 0;
	unsigned int m1 = 0;
	unsigned int m2 = 0;
	unsigned int n  = 0;
	unsigned int k1 = 0;
	unsigned int k2 = 0;

	if (freq_in < 128000 || freq_in > 24576000) {
		dev_err(dai->dev, "ac108 pllclk source input only support [128K,24M], now %u\n",
			freq_in);
		return -EINVAL;
	}

	if (pdata->sysclk_src != SYSCLK_SRC_PLL) {
		dev_dbg(dai->dev, "ac108 sysclk source don't pll, don't need config pll\n");
		return 0;
	}

	switch (pdata->pllclk_src) {
	case PLLCLK_SRC_MCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << PLLCLK_SRC, 0x0 << PLLCLK_SRC);
		break;
	case PLLCLK_SRC_BCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << PLLCLK_SRC, 0x1 << PLLCLK_SRC);
		break;
	case PLLCLK_SRC_GPIO2:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << PLLCLK_SRC, 0x2 << PLLCLK_SRC);
		break;
	case PLLCLK_SRC_GPIO3:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << PLLCLK_SRC, 0x3 << PLLCLK_SRC);
		break;
	default:
		dev_err(dai->dev, "ac108 pllclk source config error: %d\n", pdata->pllclk_src);
		return -EINVAL;
	}

	/* PLLCLK: FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] */
	for (i = 0; i < ARRAY_SIZE(ac108_pll_div); i++) {
		if (ac108_pll_div[i].freq_in == freq_in && ac108_pll_div[i].freq_out == freq_out) {
			m1 = ac108_pll_div[i].m1;
			m2 = ac108_pll_div[i].m2;
			n  = ac108_pll_div[i].n;
			k1 = ac108_pll_div[i].k1;
			k2 = ac108_pll_div[i].k2;
			dev_dbg(dai->dev, "ac108 PLL match freq_in:%u, freq_out:%u\n",
				freq_in, freq_out);
			break;
		}
	}
	if (i == ARRAY_SIZE(ac108_pll_div)) {
		dev_err(dai->dev, "ac108 PLL don't match freq_in and freq_out table\n");
		return -EINVAL;
	}

	/* Config PLL DIV param M1/M2/N/K1/K2 */
	regmap_update_bits(regmap, PLL_CTRL2, 0x1f << PLL_PREDIV1, m1 << PLL_PREDIV1);
	regmap_update_bits(regmap, PLL_CTRL2, 0x1 << PLL_PREDIV2, m2 << PLL_PREDIV2);
	regmap_update_bits(regmap, PLL_CTRL3, 0x3 << PLL_LOOPDIV_MSB, (n >> 8) << PLL_LOOPDIV_MSB);
	regmap_update_bits(regmap, PLL_CTRL4, 0xff << PLL_LOOPDIV_LSB, n << PLL_LOOPDIV_LSB);
	regmap_update_bits(regmap, PLL_CTRL5, 0x1f << PLL_POSTDIV1, k1 << PLL_POSTDIV1);
	regmap_update_bits(regmap, PLL_CTRL5, 0x1 << PLL_POSTDIV2, k2 << PLL_POSTDIV2);

	/* Config PLL module current */
	regmap_update_bits(regmap, PLL_CTRL1, 0x7 << PLL_IBIAS, 0x4 << PLL_IBIAS);
	regmap_update_bits(regmap, PLL_CTRL1, 0x1 << PLL_COM_EN, 0x1 << PLL_COM_EN);
	regmap_update_bits(regmap, PLL_CTRL1, 0x1 << PLL_EN, 0x1 << PLL_EN);
	regmap_update_bits(regmap, PLL_CTRL6, 0x1f << PLL_CP, 0xf << PLL_CP);

	/* PLLCLK lock */
	regmap_update_bits(regmap, PLL_LOCK_CTRL, 0x1 << PLL_LOCK_EN, 0x1 << PLL_LOCK_EN);

	/* GPIO4 output Clock 24MHz from DPLL for pll test */
	regmap_update_bits(regmap, GPIO_CFG2, 0xf << GPIO4_SELECT, 0x9 << GPIO4_SELECT);

	return 0;
}

static int ac108_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
				unsigned int freq, int dir)
{
	struct snd_soc_component *component = dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct ac108_data *pdata = &ac108->pdata;
	struct regmap *regmap = ac108->regmap;

	/* sysclk must be 256*fs (fs=48KHz or 44.1KHz),
	 * sysclk provided by externally clk or internal pll.
	 */
	switch (pdata->sysclk_src) {
	case SYSCLK_SRC_MCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_SRC, 0x0 << SYSCLK_SRC);
		break;
	case SYSCLK_SRC_PLL:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_SRC, 0x1 << SYSCLK_SRC);
		break;
	default:
		dev_err(dai->dev, "ac108 sysclk source config error: %d\n", pdata->sysclk_src);
		return -EINVAL;
	}

	ac108->sysclk_freq = freq;

	return 0;
}

static int ac108_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct ac108_data *pdata = &ac108->pdata;
	struct regmap *regmap = ac108->regmap;
	unsigned int i2s_mode, tx_offset, sign_ext;
	unsigned int brck_polarity, lrck_polarity;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << BCLK_IOEN, 0x1 << BCLK_IOEN);
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << LRCK_IOEN, 0x1 << LRCK_IOEN);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << BCLK_IOEN, 0x0 << BCLK_IOEN);
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << LRCK_IOEN, 0x0 << LRCK_IOEN);
		break;
	default:
		dev_err(dai->dev, "only support CBM_CFM or CBS_CFS\n");
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		i2s_mode = 1;
		tx_offset = 1;
		sign_ext = 3;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		i2s_mode = 2;
		tx_offset = 0;
		sign_ext = 3;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		i2s_mode = 1;
		tx_offset = 0;
		sign_ext = 3;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		i2s_mode = 0;
		tx_offset = 1;
		sign_ext = 3;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		i2s_mode = 0;
		tx_offset = 0;
		sign_ext = 3;
		break;
	default:
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x3 << MODE_SEL, i2s_mode << MODE_SEL);
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << TX1_OFFSET, tx_offset << TX1_OFFSET);
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << TX2_OFFSET, tx_offset << TX2_OFFSET);
	regmap_update_bits(regmap, I2S_FMT_CTRL3, 0x3 << SEXT, sign_ext << SEXT);
	regmap_update_bits(regmap, I2S_FMT_CTRL3, 0x1 << LRCK_WIDTH,
			   (pdata->frame_sync_width - 1) << LRCK_WIDTH);

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		brck_polarity = 0;
		lrck_polarity = 0;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		brck_polarity = 0;
		lrck_polarity = 1;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		brck_polarity = 1;
		lrck_polarity = 0;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		brck_polarity = 1;
		lrck_polarity = 1;
		break;
	default:
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_BCLK_CTRL,
			   0x1 << BCLK_POLARITY, brck_polarity << BCLK_POLARITY);
	regmap_update_bits(regmap, I2S_LRCK_CTRL1,
			   0x1 << LRCK_POLARITY, lrck_polarity << LRCK_POLARITY);

	ac108->fmt = fmt;

	return 0;
}

static int ac108_set_dai_tdm_slot(struct snd_soc_dai *dai,
				  unsigned int tx_mask, unsigned int rx_mask,
				  int slots, int slot_width)
{
	struct snd_soc_component *component = dai->component;
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;
	int i;
	unsigned int slot_width_reg = 0;
	unsigned int lrck_width_reg = 0;

	for (i = 0; i < ARRAY_SIZE(ac108_slot_width); i++) {
		if (ac108_slot_width[i].real == slot_width) {
			slot_width_reg = ac108_slot_width[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac108_slot_width)) {
		dev_err(dai->dev, "ac108 unsupport slot_width: %d\n", slot_width);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_FMT_CTRL2,
			   0x7 << SLOT_WIDTH_SEL, slot_width_reg << SLOT_WIDTH_SEL);

	switch (ac108->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		lrck_width_reg = (slots / 2) * slot_width - 1;
		break;
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		lrck_width_reg = slots * slot_width - 1;
		break;
	default:
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_LRCK_CTRL1,
			   0x3 << LRCK_PERIODH, (lrck_width_reg >> 8) << LRCK_PERIODH);
	regmap_update_bits(regmap, I2S_LRCK_CTRL2,
			   0xff << LRCK_PERIODL, lrck_width_reg << LRCK_PERIODH);

	ac108->slots = slots;
	ac108->slot_width = slot_width;

	return 0;
}

static const struct snd_soc_dai_ops ac108_dai_ops = {
	.startup	= ac108_startup,
	.shutdown	= ac108_shutdown,
	.hw_params	= ac108_hw_params,
	.hw_free	= ac108_hw_free,
	/* should: set_pll -> set_sysclk */
	.set_pll	= ac108_set_dai_pll,
	.set_sysclk	= ac108_set_dai_sysclk,
	/* should: set_fmt -> set_tdm_slot */
	.set_fmt	= ac108_set_dai_fmt,
	.set_tdm_slot	= ac108_set_dai_tdm_slot,
};

static struct snd_soc_dai_driver ac108_dai = {
	.name = "ac108-codec",
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 4,
		.rates		= SNDRV_PCM_RATE_8000_96000,
		.formats	= SNDRV_PCM_FMTBIT_S8
				| SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
	.ops = &ac108_dai_ops,
};

static int ac108_probe(struct snd_soc_component *component)
{
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct ac108_data *pdata = &ac108->pdata;
	struct regmap *regmap = ac108->regmap;
	int i;

	/* adc digita volume set */
	regmap_write(regmap, ADC1_DVOL_CTRL, pdata->ch1_dig_vol);
	regmap_write(regmap, ADC2_DVOL_CTRL, pdata->ch2_dig_vol);
	regmap_write(regmap, ADC3_DVOL_CTRL, pdata->ch3_dig_vol);
	regmap_write(regmap, ADC4_DVOL_CTRL, pdata->ch4_dig_vol);

	/* adc pga gain set */
	regmap_update_bits(regmap, ANA_PGA1_CTRL, 0x1f << ADC1_ANALOG_PGA,
			   pdata->ch1_pga_gain << ADC1_ANALOG_PGA);
	regmap_update_bits(regmap, ANA_PGA2_CTRL, 0x1f << ADC2_ANALOG_PGA,
			   pdata->ch2_pga_gain << ADC2_ANALOG_PGA);
	regmap_update_bits(regmap, ANA_PGA3_CTRL, 0x1f << ADC3_ANALOG_PGA,
			   pdata->ch3_pga_gain << ADC3_ANALOG_PGA);
	regmap_update_bits(regmap, ANA_PGA4_CTRL, 0x1f << ADC4_ANALOG_PGA,
			   pdata->ch4_pga_gain << ADC4_ANALOG_PGA);

	/* I2S: SDO drive data and SDI sample data at the different BCLK edge */
	regmap_update_bits(regmap, I2S_BCLK_CTRL, 0x1 << EDGE_TRANSFER, 0x0 << EDGE_TRANSFER);

	/* I2S:
	 * disable encoding mode
	 * normal mode for the last half cycle of BCLK in the slot
	 * Turn to hi-z state (TDM) when not transferring slot
	 */
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << ENCD_SEL, 0x0 << ENCD_SEL);
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << TX_SLOT_HIZ, 0x1 << TX_SLOT_HIZ);
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << TX_STATE, 0x0 << TX_STATE);

	/* I2S: TX MSB first, SDOUT normal, Linear PCM Data Mode */
	regmap_update_bits(regmap, I2S_FMT_CTRL3, 0x1 << TX_MLS, pdata->pcm_bit_first << TX_MLS);
	regmap_update_bits(regmap, I2S_FMT_CTRL3, 0x1 << OUT1_MUTE, 0x0 << OUT1_MUTE);
	regmap_update_bits(regmap, I2S_FMT_CTRL3, 0x1 << OUT2_MUTE, 0x0 << OUT2_MUTE);
	regmap_update_bits(regmap, I2S_FMT_CTRL3, 0x3 << TX_PDM, 0x0 << TX_PDM);

	/* I2S: adc channel map to i2s channel */
	for (i = 0; i < 4; i++)
		regmap_update_bits(regmap, I2S_TX1_CHMP_CTRL1, 0x3 << i,
				   (pdata->tx_pin0_chmap0 >> (i * 4)) & 0x03);
	for (i = 0; i < 4; i++)
		regmap_update_bits(regmap, I2S_TX1_CHMP_CTRL2, 0x3 << i,
				   (pdata->tx_pin0_chmap0 >> ((i + 4) * 4)) & 0x03);
	for (i = 0; i < 4; i++)
		regmap_update_bits(regmap, I2S_TX1_CHMP_CTRL3, 0x3 << i,
				   (pdata->tx_pin0_chmap1 >> (i * 4)) & 0x03);
	for (i = 0; i < 4; i++)
		regmap_update_bits(regmap, I2S_TX1_CHMP_CTRL4, 0x3 << i,
				   (pdata->tx_pin0_chmap1 >> ((i + 4) * 4)) & 0x03);

	for (i = 0; i < 4; i++)
		regmap_update_bits(regmap, I2S_TX2_CHMP_CTRL1, 0x3 << i,
				   (pdata->tx_pin1_chmap0 >> (i * 4)) & 0x03);
	for (i = 0; i < 4; i++)
		regmap_update_bits(regmap, I2S_TX2_CHMP_CTRL2, 0x3 << i,
				   (pdata->tx_pin1_chmap0 >> ((i + 4) * 4)) & 0x03);
	for (i = 0; i < 4; i++)
		regmap_update_bits(regmap, I2S_TX2_CHMP_CTRL3, 0x3 << i,
				   (pdata->tx_pin1_chmap1 >> (i * 4)) & 0x03);
	for (i = 0; i < 4; i++)
		regmap_update_bits(regmap, I2S_TX2_CHMP_CTRL4, 0x3 << i,
				   (pdata->tx_pin1_chmap1 >> ((i + 4) * 4)) & 0x03);

	/* HPF enable */
	regmap_update_bits(regmap, HPF_EN, 0x1 << DIG_ADC1_HPF_EN, 0x1 << DIG_ADC1_HPF_EN);
	regmap_update_bits(regmap, HPF_EN, 0x1 << DIG_ADC2_HPF_EN, 0x1 << DIG_ADC2_HPF_EN);
	regmap_update_bits(regmap, HPF_EN, 0x1 << DIG_ADC3_HPF_EN, 0x1 << DIG_ADC3_HPF_EN);
	regmap_update_bits(regmap, HPF_EN, 0x1 << DIG_ADC4_HPF_EN, 0x1 << DIG_ADC4_HPF_EN);

	/* ADC PTN sel: 0->normal, 1->0x5A5A5A, 2->0x123456, 3->0x000000 */
	regmap_update_bits(regmap, ADC_DIG_DEBUG, 0x7 << ADC_PTN_SEL, 0x0 << ADC_PTN_SEL);

	return 0;
}

static void ac108_remove(struct snd_soc_component *component)
{
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;

	/* reset chip */
	regmap_write(regmap, CHIP_AUDIO_RST, 0x12);
}

static int ac108_suspend(struct snd_soc_component *component)
{
	(void)component;

	return 0;
}

static int ac108_resume(struct snd_soc_component *component)
{
	ac108_probe(component);

	return 0;
}

static int ac108_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, ANA_ADC4_CTRL7,
				   0x1 << ADC1_CLK_GATING, 0x1 << ADC1_CLK_GATING);
		/* AAF & ADC enable, ADC PGA enable, ADC MICBIAS enable and UnMute */
		regmap_write(regmap, ANA_ADC1_CTRL1, 0x7);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_write(regmap, ANA_ADC1_CTRL1, 0x0);
		regmap_update_bits(regmap, ANA_ADC4_CTRL7,
				   0x1 << ADC1_CLK_GATING, 0x0 << ADC1_CLK_GATING);
		break;
	default:
		break;
	}

	return 0;
}

static int ac108_mic2_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, ANA_ADC4_CTRL7,
				   0x1 << ADC2_CLK_GATING, 0x1 << ADC2_CLK_GATING);
		/* AAF & ADC enable, ADC PGA enable, ADC MICBIAS enable and UnMute */
		regmap_write(regmap, ANA_ADC2_CTRL1, 0x7);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_write(regmap, ANA_ADC2_CTRL1, 0x0);
		regmap_update_bits(regmap, ANA_ADC4_CTRL7,
				   0x1 << ADC2_CLK_GATING, 0x0 << ADC2_CLK_GATING);
		break;
	default:
		break;
	}

	return 0;
}

static int ac108_mic3_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, ANA_ADC4_CTRL7,
				   0x1 << ADC3_CLK_GATING, 0x1 << ADC3_CLK_GATING);
		/* AAF & ADC enable, ADC PGA enable, ADC MICBIAS enable and UnMute */
		regmap_write(regmap, ANA_ADC3_CTRL1, 0x7);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_write(regmap, ANA_ADC3_CTRL1, 0x0);
		regmap_update_bits(regmap, ANA_ADC4_CTRL7,
				   0x1 << ADC3_CLK_GATING, 0x0 << ADC3_CLK_GATING);
		break;
	default:
		break;
	}

	return 0;
}

static int ac108_mic4_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, ANA_ADC4_CTRL7,
				   0x1 << ADC4_CLK_GATING, 0x1 << ADC4_CLK_GATING);
		/* AAF & ADC enable, ADC PGA enable, ADC MICBIAS enable and UnMute */
		regmap_write(regmap, ANA_ADC4_CTRL1, 0x7);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_write(regmap, ANA_ADC4_CTRL1, 0x0);
		regmap_update_bits(regmap, ANA_ADC4_CTRL7,
				   0x1 << ADC4_CLK_GATING, 0x0 << ADC4_CLK_GATING);
		break;
	default:
		break;
	}

	return 0;
}

static int ac108_dmic_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac108_priv *ac108 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac108->regmap;
	struct soc_enum *e = (struct soc_enum *)k->private_value;
	unsigned int shift = e->shift_l;
	unsigned long flags;

	spin_lock_irqsave(&route_status_lock, flags);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		switch (shift) {
		case DMIC1L_SHIFT:
			ac108->dmic1l_en = true;
			break;
		case DMIC1R_SHIFT:
			ac108->dmic1r_en = true;
			break;
		case DMIC2L_SHIFT:
			ac108->dmic2l_en = true;
			break;
		case DMIC2R_SHIFT:
			ac108->dmic2r_en = true;
			break;
		default:
			break;
		}

		/* set GPIO2 as DMIC1_CLK */
		regmap_update_bits(regmap, GPIO_CFG1, 0xff << GPIO2_SELECT, 0xe << GPIO2_SELECT);
		if (ac108->dmic1l_en || ac108->dmic1r_en) {
			/* set GPIO1 as DMIC1_DAT, enable DMIC1 */
			regmap_update_bits(regmap, GPIO_CFG1,
					   0xff << GPIO1_SELECT, 0xe << GPIO1_SELECT);
			regmap_update_bits(regmap, DMIC_EN, 0x1 << DMIC1_EN, 0x1 << DMIC1_EN);
		}
		if (ac108->dmic2l_en || ac108->dmic2r_en) {
			/* set GPIO3 as DMIC2_DAT, enable DMIC2 */
			regmap_update_bits(regmap, GPIO_CFG2,
					   0xff << GPIO3_SELECT, 0xe << GPIO3_SELECT);
			regmap_update_bits(regmap, DMIC_EN, 0x1 << DMIC2_EN, 0x1 << DMIC2_EN);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		switch (shift) {
		case DMIC1L_SHIFT:
			ac108->dmic1l_en = false;
			break;
		case DMIC1R_SHIFT:
			ac108->dmic1r_en = false;
			break;
		case DMIC2L_SHIFT:
			ac108->dmic2l_en = false;
			break;
		case DMIC2R_SHIFT:
			ac108->dmic2r_en = false;
			break;
		default:
			break;
		}

		if (!(ac108->dmic1l_en || ac108->dmic1r_en)) {
			regmap_update_bits(regmap, DMIC_EN, 0x1 << DMIC1_EN, 0x0 << DMIC1_EN);
			regmap_update_bits(regmap, GPIO_CFG1,
					   0xff << GPIO1_SELECT, 0x7 << GPIO1_SELECT);
		}
		if (!(ac108->dmic2l_en || ac108->dmic2r_en)) {
			regmap_update_bits(regmap, DMIC_EN, 0x1 << DMIC2_EN, 0x0 << DMIC2_EN);
			regmap_update_bits(regmap, GPIO_CFG2,
					   0xff << GPIO3_SELECT, 0x7 << GPIO3_SELECT);
		}
		if (!(ac108->dmic2l_en || ac108->dmic2r_en || ac108->dmic2l_en || ac108->dmic2r_en))
			regmap_update_bits(regmap, GPIO_CFG1,
					   0xff << GPIO2_SELECT, 0x7 << GPIO2_SELECT);
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&route_status_lock, flags);

	return 0;
}

static const DECLARE_TLV_DB_SCALE(adc_digital_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(adc_pga_gain_tlv, 0, 100, 0);

static const struct snd_kcontrol_new ac108_snd_controls[] = {
	SOC_SINGLE_TLV("ADC1 Digital Volume", ADC1_DVOL_CTRL, DIG_ADCL1_VOL,
		       0xff, 0, adc_digital_vol_tlv),
	SOC_SINGLE_TLV("ADC2 Digital Volume", ADC2_DVOL_CTRL, DIG_ADCL2_VOL,
		       0xff, 0, adc_digital_vol_tlv),
	SOC_SINGLE_TLV("ADC3 Digital Volume", ADC3_DVOL_CTRL, DIG_ADCL3_VOL,
		       0xff, 0, adc_digital_vol_tlv),
	SOC_SINGLE_TLV("ADC4 Digital Volume", ADC4_DVOL_CTRL, DIG_ADCL4_VOL,
		       0xff, 0, adc_digital_vol_tlv),
	SOC_SINGLE_TLV("ADC1 PGA Gain", ANA_PGA1_CTRL, ADC1_ANALOG_PGA,
		       0x1f, 0, adc_pga_gain_tlv),
	SOC_SINGLE_TLV("ADC2 PGA Gain", ANA_PGA2_CTRL, ADC2_ANALOG_PGA,
		       0x1f, 0, adc_pga_gain_tlv),
	SOC_SINGLE_TLV("ADC3 PGA Gain", ANA_PGA3_CTRL, ADC3_ANALOG_PGA,
		       0x1f, 0, adc_pga_gain_tlv),
	SOC_SINGLE_TLV("ADC4 PGA Gain", ANA_PGA4_CTRL, ADC4_ANALOG_PGA,
		       0x1f, 0, adc_pga_gain_tlv),
};

static const char * const adc_dat_src_mux_text[] = {
	"AMIC", "DMIC",
};

static const char * const adc_pga_src_mux_text[] = {
	"AMIC1", "AMIC2", "AMIC3", "AMIC4",
};

static const struct soc_enum adc12_dat_src_mux_enum =
	SOC_ENUM_SINGLE(DMIC_EN, DMIC1_EN, 2, adc_dat_src_mux_text);

static const struct snd_kcontrol_new adc12_dat_src_mux =
	SOC_DAPM_ENUM("ADC12 DAT MUX", adc12_dat_src_mux_enum);

static const struct soc_enum adc34_dat_src_mux_enum =
	SOC_ENUM_SINGLE(DMIC_EN, DMIC2_EN, 2, adc_dat_src_mux_text);

static const struct snd_kcontrol_new adc34_dat_src_mux =
	SOC_DAPM_ENUM("ADC34 DAT MUX", adc34_dat_src_mux_enum);

static const struct soc_enum adc1_pga_src_mux_enum =
	SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC1_SRS, 4, adc_pga_src_mux_text);

static const struct snd_kcontrol_new adc1_pga_src_mux =
	SOC_DAPM_ENUM("ADC1 PGA MUX", adc1_pga_src_mux_enum);

static const struct soc_enum adc2_pga_src_mux_enum =
	SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC2_SRS, 4, adc_pga_src_mux_text);

static const struct snd_kcontrol_new adc2_pga_src_mux =
	SOC_DAPM_ENUM("ADC2 PGA MUX", adc2_pga_src_mux_enum);

static const struct soc_enum adc3_pga_src_mux_enum =
	SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC3_SRS, 4, adc_pga_src_mux_text);

static const struct snd_kcontrol_new adc3_pga_src_mux =
	SOC_DAPM_ENUM("ADC3 PGA MUX", adc3_pga_src_mux_enum);

static const struct soc_enum adc4_pga_src_mux_enum =
	SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC4_SRS, 4, adc_pga_src_mux_text);

static const struct snd_kcontrol_new adc4_pga_src_mux =
	SOC_DAPM_ENUM("ADC4 PGA MUX", adc4_pga_src_mux_enum);

static const struct snd_kcontrol_new adc1_digital_src_mixer[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", ADC1_DMIX_SRC, ADC1_ADC1_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC2 Switch", ADC1_DMIX_SRC, ADC1_ADC2_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC3 Switch", ADC1_DMIX_SRC, ADC1_ADC3_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC4 Switch", ADC1_DMIX_SRC, ADC1_ADC4_DMXL_SRC, 1, 0),
};

static const struct snd_kcontrol_new adc2_digital_src_mixer[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", ADC2_DMIX_SRC, ADC2_ADC1_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC2 Switch", ADC2_DMIX_SRC, ADC2_ADC2_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC3 Switch", ADC2_DMIX_SRC, ADC2_ADC3_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC4 Switch", ADC2_DMIX_SRC, ADC2_ADC4_DMXL_SRC, 1, 0),
};

static const struct snd_kcontrol_new adc3_digital_src_mixer[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", ADC3_DMIX_SRC, ADC3_ADC1_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC2 Switch", ADC3_DMIX_SRC, ADC3_ADC2_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC3 Switch", ADC3_DMIX_SRC, ADC3_ADC3_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC4 Switch", ADC3_DMIX_SRC, ADC3_ADC4_DMXL_SRC, 1, 0),
};

static const struct snd_kcontrol_new adc4_digital_src_mixer[] = {
	SOC_DAPM_SINGLE("ADC1 Switch", ADC4_DMIX_SRC, ADC4_ADC1_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC2 Switch", ADC4_DMIX_SRC, ADC4_ADC2_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC3 Switch", ADC4_DMIX_SRC, ADC4_ADC3_DMXL_SRC, 1, 0),
	SOC_DAPM_SINGLE("ADC4 Switch", ADC4_DMIX_SRC, ADC4_ADC4_DMXL_SRC, 1, 0),
};

static const struct snd_soc_dapm_widget ac108_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MIC1P"),
	SND_SOC_DAPM_INPUT("MIC1N"),
	SND_SOC_DAPM_INPUT("MIC2P"),
	SND_SOC_DAPM_INPUT("MIC2N"),
	SND_SOC_DAPM_INPUT("MIC3P"),
	SND_SOC_DAPM_INPUT("MIC3N"),
	SND_SOC_DAPM_INPUT("MIC4P"),
	SND_SOC_DAPM_INPUT("MIC4N"),
	SND_SOC_DAPM_INPUT("DMIC1L"),
	SND_SOC_DAPM_INPUT("DMIC1R"),
	SND_SOC_DAPM_INPUT("DMIC2L"),
	SND_SOC_DAPM_INPUT("DMIC2R"),

	SND_SOC_DAPM_PGA_E("MIC1 PGA", SND_SOC_NOPM, 0, 0, NULL, 0,
			   ac108_mic1_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("MIC2 PGA", SND_SOC_NOPM, 0, 0, NULL, 0,
			   ac108_mic2_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("MIC3 PGA", SND_SOC_NOPM, 0, 0, NULL, 0,
			   ac108_mic3_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("MIC4 PGA", SND_SOC_NOPM, 0, 0, NULL, 0,
			   ac108_mic4_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA_E("DMIC1L PGA", SND_SOC_NOPM, DMIC1L_SHIFT, 0, NULL, 0,
			   ac108_dmic_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("DMIC1R PGA", SND_SOC_NOPM, DMIC1R_SHIFT, 0, NULL, 0,
			   ac108_dmic_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("DMIC2L PGA", SND_SOC_NOPM, DMIC2L_SHIFT, 0, NULL, 0,
			   ac108_dmic_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("DMIC2R PGA", SND_SOC_NOPM, DMIC2R_SHIFT, 0, NULL, 0,
			   ac108_dmic_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("ADC12 DAT MUX", SND_SOC_NOPM, 0, 0, &adc12_dat_src_mux),
	SND_SOC_DAPM_MUX("ADC34 DAT MUX", SND_SOC_NOPM, 0, 0, &adc34_dat_src_mux),

	SND_SOC_DAPM_MUX("ADC1 PGA MUX", SND_SOC_NOPM, 0, 0, &adc1_pga_src_mux),
	SND_SOC_DAPM_MUX("ADC2 PGA MUX", SND_SOC_NOPM, 0, 0, &adc2_pga_src_mux),
	SND_SOC_DAPM_MUX("ADC3 PGA MUX", SND_SOC_NOPM, 0, 0, &adc3_pga_src_mux),
	SND_SOC_DAPM_MUX("ADC4 PGA MUX", SND_SOC_NOPM, 0, 0, &adc4_pga_src_mux),

	SND_SOC_DAPM_MIXER("ADC1 MIXER", ADC_DIG_EN, ENAD1, 0,
			   adc1_digital_src_mixer, ARRAY_SIZE(adc1_digital_src_mixer)),
	SND_SOC_DAPM_MIXER("ADC2 MIXER", ADC_DIG_EN, ENAD2, 0,
			   adc2_digital_src_mixer, ARRAY_SIZE(adc2_digital_src_mixer)),
	SND_SOC_DAPM_MIXER("ADC3 MIXER", ADC_DIG_EN, ENAD3, 0,
			   adc3_digital_src_mixer, ARRAY_SIZE(adc3_digital_src_mixer)),
	SND_SOC_DAPM_MIXER("ADC4 MIXER", ADC_DIG_EN, ENAD4, 0,
			   adc4_digital_src_mixer, ARRAY_SIZE(adc4_digital_src_mixer)),

	SND_SOC_DAPM_AIF_OUT("ADC1", "Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("ADC2", "Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("ADC3", "Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("ADC4", "Capture", 0, SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_dapm_route ac108_dapm_routes[] = {
	{"MIC1 PGA", NULL, "MIC1P"},
	{"MIC1 PGA", NULL, "MIC1N"},
	{"MIC2 PGA", NULL, "MIC2P"},
	{"MIC2 PGA", NULL, "MIC2N"},
	{"MIC3 PGA", NULL, "MIC3P"},
	{"MIC3 PGA", NULL, "MIC3N"},
	{"MIC4 PGA", NULL, "MIC4P"},
	{"MIC4 PGA", NULL, "MIC4N"},
	{"DMIC1L PGA", NULL, "DMIC1L"},
	{"DMIC1R PGA", NULL, "DMIC1R"},
	{"DMIC2L PGA", NULL, "DMIC2L"},
	{"DMIC2R PGA", NULL, "DMIC2R"},

	{"ADC1 PGA MUX", "AMIC1", "MIC1 PGA"},
	{"ADC1 PGA MUX", "AMIC2", "MIC2 PGA"},
	{"ADC1 PGA MUX", "AMIC2", "MIC3 PGA"},
	{"ADC1 PGA MUX", "AMIC4", "MIC4 PGA"},

	{"ADC2 PGA MUX", "AMIC1", "MIC1 PGA"},
	{"ADC2 PGA MUX", "AMIC2", "MIC2 PGA"},
	{"ADC2 PGA MUX", "AMIC2", "MIC3 PGA"},
	{"ADC2 PGA MUX", "AMIC4", "MIC4 PGA"},

	{"ADC3 PGA MUX", "AMIC1", "MIC1 PGA"},
	{"ADC3 PGA MUX", "AMIC2", "MIC2 PGA"},
	{"ADC3 PGA MUX", "AMIC2", "MIC3 PGA"},
	{"ADC3 PGA MUX", "AMIC4", "MIC4 PGA"},

	{"ADC4 PGA MUX", "AMIC1", "MIC1 PGA"},
	{"ADC4 PGA MUX", "AMIC2", "MIC2 PGA"},
	{"ADC4 PGA MUX", "AMIC2", "MIC3 PGA"},
	{"ADC4 PGA MUX", "AMIC4", "MIC4 PGA"},

	{"ADC12 DAT MUX", "AMIC", "ADC1 PGA MUX"},
	{"ADC12 DAT MUX", "AMIC", "ADC2 PGA MUX"},
	{"ADC12 DAT MUX", "DMIC", "DMIC1L PGA"},
	{"ADC12 DAT MUX", "DMIC", "DMIC1R PGA"},

	{"ADC34 DAT MUX", "AMIC", "ADC3 PGA MUX"},
	{"ADC34 DAT MUX", "AMIC", "ADC4 PGA MUX"},
	{"ADC34 DAT MUX", "DMIC", "DMIC2L PGA"},
	{"ADC34 DAT MUX", "DMIC", "DMIC2R PGA"},

	{"ADC1 MIXER", "ADC1 Switch", "ADC12 DAT MUX"},
	{"ADC1 MIXER", "ADC2 Switch", "ADC12 DAT MUX"},
	{"ADC1 MIXER", "ADC3 Switch", "ADC34 DAT MUX"},
	{"ADC1 MIXER", "ADC4 Switch", "ADC34 DAT MUX"},

	{"ADC2 MIXER", "ADC1 Switch", "ADC12 DAT MUX"},
	{"ADC2 MIXER", "ADC2 Switch", "ADC12 DAT MUX"},
	{"ADC2 MIXER", "ADC3 Switch", "ADC34 DAT MUX"},
	{"ADC2 MIXER", "ADC4 Switch", "ADC34 DAT MUX"},

	{"ADC3 MIXER", "ADC1 Switch", "ADC12 DAT MUX"},
	{"ADC3 MIXER", "ADC2 Switch", "ADC12 DAT MUX"},
	{"ADC3 MIXER", "ADC3 Switch", "ADC34 DAT MUX"},
	{"ADC3 MIXER", "ADC4 Switch", "ADC34 DAT MUX"},

	{"ADC4 MIXER", "ADC1 Switch", "ADC12 DAT MUX"},
	{"ADC4 MIXER", "ADC2 Switch", "ADC12 DAT MUX"},
	{"ADC4 MIXER", "ADC3 Switch", "ADC34 DAT MUX"},
	{"ADC4 MIXER", "ADC4 Switch", "ADC34 DAT MUX"},

	{"ADC1", NULL, "ADC1 MIXER"},
	{"ADC2", NULL, "ADC2 MIXER"},
	{"ADC3", NULL, "ADC3 MIXER"},
	{"ADC4", NULL, "ADC4 MIXER"},
};

static const struct snd_soc_component_driver soc_component_dev_ac108 = {
	.probe			= ac108_probe,
	.remove			= ac108_remove,
	.suspend		= ac108_suspend,
	.resume			= ac108_resume,
	.controls		= ac108_snd_controls,
	.num_controls		= ARRAY_SIZE(ac108_snd_controls),
	.dapm_widgets		= ac108_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ac108_dapm_widgets),
	.dapm_routes		= ac108_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(ac108_dapm_routes),
};

static const struct regmap_config ac108_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = AC108_MAX_REG,

	.reg_defaults = ac108_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(ac108_reg_defaults),
	.cache_type = REGCACHE_NONE,
};

static void ac108_set_params_from_of(struct i2c_client *i2c, struct ac108_data *pdata)
{
	const struct device_node *np = i2c->dev.of_node;
	int i, ret;
	const char *str;
	struct of_keyval_tavle {
		char *name;
		unsigned int val;
	};

	struct of_keyval_tavle of_pllclk_src_table[] = {
		{ "MCLK",	PLLCLK_SRC_MCLK },
		{ "BCLK",	PLLCLK_SRC_BCLK },
		{ "GPIO2",	PLLCLK_SRC_GPIO2 },
		{ "GPIO3",	PLLCLK_SRC_GPIO3 },
	};
	struct of_keyval_tavle of_sysclk_src_table[] = {
		{ "MCLK",	SYSCLK_SRC_MCLK },
		{ "PLL",	SYSCLK_SRC_PLL },
	};
	struct of_keyval_tavle of_pcm_bit_first_table[] = {
		{ "MSB",	0 },
		{ "LSB",	1 },
	};

	ret = of_property_read_string(np, "pllclk-src", &str);
	if (ret == 0) {
		for (i = 0; i < ARRAY_SIZE(of_pllclk_src_table); i++) {
			if (strcmp(str, of_pllclk_src_table[i].name) == 0) {
				pdata->pllclk_src = of_pllclk_src_table[i].val;
				break;
			}
		}
	} else {
		pdata->pllclk_src = PLLCLK_SRC_MCLK;
	}

	ret = of_property_read_string(np, "sysclk-src", &str);
	if (ret == 0) {
		for (i = 0; i < ARRAY_SIZE(of_sysclk_src_table); i++) {
			if (strcmp(str, of_sysclk_src_table[i].name) == 0) {
				pdata->sysclk_src = of_sysclk_src_table[i].val;
				break;
			}
		}
	} else {
		pdata->sysclk_src = SYSCLK_SRC_MCLK;
	}

	ret = of_property_read_u32(np, "tx-pin0-chmap0", &pdata->tx_pin0_chmap0);
	if (ret < 0)
		pdata->tx_pin0_chmap0 = 0x32103210;
	ret = of_property_read_u32(np, "tx-pin0-chmap1", &pdata->tx_pin0_chmap1);
	if (ret < 0)
		pdata->tx_pin0_chmap1 = 0x32103210;
	ret = of_property_read_u32(np, "tx-pin1-chmap0", &pdata->tx_pin1_chmap0);
	if (ret < 0)
		pdata->tx_pin1_chmap0 = 0x32103210;
	ret = of_property_read_u32(np, "tx-pin1-chmap1", &pdata->tx_pin1_chmap1);
	if (ret < 0)
		pdata->tx_pin1_chmap1 = 0x32103210;

	ret = of_property_read_u32(np, "ch1-dig-vol", &pdata->ch1_dig_vol);
	if (ret < 0)
		pdata->ch1_dig_vol = 160;
	ret = of_property_read_u32(np, "ch2-dig-vol", &pdata->ch2_dig_vol);
	if (ret < 0)
		pdata->ch2_dig_vol = 160;
	ret = of_property_read_u32(np, "ch3-dig-vol", &pdata->ch3_dig_vol);
	if (ret < 0)
		pdata->ch3_dig_vol = 160;
	ret = of_property_read_u32(np, "ch4-dig-vol", &pdata->ch4_dig_vol);
	if (ret < 0)
		pdata->ch4_dig_vol = 160;

	ret = of_property_read_u32(np, "ch1-pga-gain", &pdata->ch1_pga_gain);
	if (ret < 0)
		pdata->ch1_pga_gain = 31;
	ret = of_property_read_u32(np, "ch2-pga-gain", &pdata->ch2_pga_gain);
	if (ret < 0)
		pdata->ch2_pga_gain = 31;
	ret = of_property_read_u32(np, "ch3-pga-gain", &pdata->ch3_pga_gain);
	if (ret < 0)
		pdata->ch3_pga_gain = 31;
	ret = of_property_read_u32(np, "ch4-pga-gain", &pdata->ch4_pga_gain);
	if (ret < 0)
		pdata->ch4_pga_gain = 31;

	ret = of_property_read_string(np, "pcm-bit-first", &str);
	if (ret == 0) {
		for (i = 0; i < ARRAY_SIZE(of_pcm_bit_first_table); i++) {
			if (strcmp(str, of_pcm_bit_first_table[i].name) == 0) {
				pdata->pcm_bit_first = of_pcm_bit_first_table[i].val;
				break;
			}
		}
	} else {
		pdata->pcm_bit_first = 0;
	}

	ret = of_property_read_u32(np, "frame-sync-width", &pdata->frame_sync_width);
	if (ret < 0 || pdata->frame_sync_width > 2)
		pdata->frame_sync_width = 1;
}

static int ac108_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct ac108_data *pdata = dev_get_platdata(&i2c->dev);
	struct ac108_priv *ac108;
	int ret;

	ac108 = devm_kzalloc(&i2c->dev, sizeof(*ac108), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ac108)) {
		dev_err(&i2c->dev, "Unable to allocate ac108 private data\n");
		return -ENOMEM;
	}

	ac108->regmap = devm_regmap_init_i2c(i2c, &ac108_regmap);
	if (IS_ERR(ac108->regmap))
		return PTR_ERR(ac108->regmap);

	if (pdata)
		memcpy(&ac108->pdata, pdata, sizeof(*ac108));
	else if (i2c->dev.of_node)
		ac108_set_params_from_of(i2c, &ac108->pdata);
	else
		dev_err(&i2c->dev, "Unable to allocate ac108 private data\n");

	i2c_set_clientdata(i2c, ac108);

	ret = devm_snd_soc_register_component(&i2c->dev,
					      &soc_component_dev_ac108,
					      &ac108_dai, 1);
	if (ret < 0)
		dev_err(&i2c->dev, "register ac108 codec failed: %d\n", ret);

	return ret;
}

static const struct i2c_device_id ac108_i2c_id[] = {
	{ "ac108", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ac108_i2c_id);

static const struct of_device_id ac108_of_match[] = {
	{ .compatible = "allwinner,sunxi-ac108", },
	{ }
};
MODULE_DEVICE_TABLE(of, ac108_of_match);

static struct i2c_driver ac108_i2c_driver = {
	.driver = {
		.name = "sunxi-ac108",
		.of_match_table = ac108_of_match,
	},
	.probe = ac108_i2c_probe,
	.id_table = ac108_i2c_id,
};

module_i2c_driver(ac108_i2c_driver);

MODULE_DESCRIPTION("ASoC AC108 driver");
MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");

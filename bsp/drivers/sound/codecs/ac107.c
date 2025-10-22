// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * ac107.c -- AC107 ALSA SoC Audio driver
 *
 * Copyright (c) 2022 Allwinnertech Ltd.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm_params.h>

#include "ac107.h"
#include "snd_sunxi_common.h"

static unsigned int ac107_ecdb_ch_nums;

struct ac107_real_to_reg {
	unsigned int real;
	unsigned int reg;
};

struct ac107_pll_div {
	unsigned int freq_in;
	unsigned int freq_out;
	unsigned int m1:5;
	unsigned int m2:1;
	unsigned int n:10;
	unsigned int k1:5;
	unsigned int k2:1;
};

static const struct reg_default ac107_reg_defaults[] = {
	{  0x0, 0x4b },
	{  0x1, 0x00 },
	{  0x2, 0x11 },

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

	{ 0x30, 0x00 },
	{ 0x31, 0x00 },
	{ 0x32, 0x00 },
	{ 0x33, 0x00 },
	{ 0x34, 0x00 },
	{ 0x35, 0x55 },
	{ 0x36, 0x60 },

	{ 0x38, 0x00 },
	{ 0x39, 0x00 },
	{ 0x3a, 0x00 },

	{ 0x3c, 0x00 },
	{ 0x3d, 0x00 },

	{ 0x50, 0x00 },
	{ 0x51, 0x03 },
	{ 0x52, 0x00 },

	{ 0x54, 0x00 },
	{ 0x55, 0x00 },

	{ 0x59, 0x00 },
	{ 0x60, 0x00 },
	{ 0x61, 0x00 },
	{ 0x62, 0x00 },

	{ 0x66, 0x03 },

	{ 0x70, 0xa0 },
	{ 0x71, 0xa0 },

	{ 0x76, 0x01 },
	{ 0x77, 0x02 },

	{ 0x7f, 0x00 },
	{ 0x80, 0x11 },
	{ 0x81, 0x11 },
	{ 0x82, 0x55 },

	{ 0xa0, 0x00 },
	{ 0xa1, 0x00 },
	{ 0xa2, 0x00 },
	{ 0xa3, 0x00 },
	{ 0xa4, 0x00 },
	{ 0xa5, 0x00 },
	{ 0xa6, 0x00 },
	{ 0xa7, 0x00 },
	{ 0xa8, 0x00 },
	{ 0xa9, 0x00 },
	{ 0xaa, 0x00 },
};

/* PLLCLK: FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] */
static const struct ac107_pll_div ac107_pll_div[] = {
	{256000,    12288000,  0,  0,  96,   1,  0},
	{384000,    12288000,  0,  1,  128,  0,  1},
	{512000,    12288000,  1,  0,  960,  19, 0},
	{768000,    12288000,  1,  0,  96,   2,  0},
	{1024000,   12288000,  1,  0,  96,   1,  1},
	{1152000,   12288000,  1,  0,  128,  2,  1},
	{1536000,   12288000,  9,  0,  960,  5,  1},
	{1728000,   12288000,  1,  1,  512,  8,  1},
	{2048000,   12288000,  2,  0,  756,  20, 1},
	{2304000,   12288000,  0,  0,  32,   2,  1},
	{3072000,   12288000,  0,  0,  160,  19, 1},
	{3456000,   12288000,  1,  1,  256,  8,  1},
	{4096000,   12288000,  1,  0,  12,   0,  1},
	{4608000,   12288000,  1,  0,  32,   2,  1},
	{6144000,   12288000,  1,  0,  8,    0,  1},
	{6912000,   12288000,  1,  0,  128,  17, 1},
	{8192000,   12288000,  0,  0,  3,    0,  1},
	{9216000,   12288000,  1,  0,  16,   2,  1},
	{12288000,  12288000,  0,  0,  22,   10, 1},
	{13824000,  12288000,  1,  0,  32,   8,  1},
	{16384000,  12288000,  1,  1,  12,   1,  1},
	{18432000,  12288000,  0,  0,  8,    5,  1},
	{24576000,  12288000,  0,  1,  6,    5,  0},
	{27648000,  12288000,  1,  1,  32,   8,  1},
	{36864000,  12288000,  1,  1,  8,    2,  1},
	{49152000,  12288000,  1,  1,  12,   5,  1},

	{352800,    11289600,  0,  0,  32,   0,  0},
	{529200,    11289600,  1,  1,  512,  2,  1},
	{705600,    11289600,  1,  1,  512,  3,  1},
	{1058400,   11289600,  0,  0,  32,   2,  0},
	{1411200,   11289600,  1,  1,  512,  7,  1},
	{1587600,   11289600,  1,  1,  512,  8,  1},
	{2116800,   11289600,  0,  0,  16,   2,  0},
	{2822400,   11289600,  0,  0,  4,    0,  0},
	{3175200,   11289600,  1,  1,  256,  8,  1},
	{4233600,   11289600,  0,  0,  8,    2,  0},
	{5644800,   11289600,  0,  0,  2,    0,  0},
	{6350400,   11289600,  1,  1,  128,  8,  1},
	{8467200,   11289600,  0,  0,  4,    2,  0},
	{11289600,  11289600,  0,  0,  1,    0,  0},
	{12700800,  11289600,  1,  1,  64,   8,  1},
	{16934400,  11289600,  0,  0,  2,    2,  0},
	{22579200,  11289600,  1,  1,  8,    1,  1},
	{25401600,  11289600,  1,  1,  32,   8,  1},
	{33868800,  11289600,  1,  1,  8,    2,  1},
	{45158400,  11289600,  1,  1,  6,    2,  1},
};

static const struct ac107_real_to_reg ac107_sample_bit[] = {
	{8,  1},
	{12, 2},
	{16, 3},
	{20, 4},
	{24, 5},
	{28, 6},
	{32, 7},
};

static const struct ac107_real_to_reg ac107_sample_rate[] = {
	{8000, 0},
	{11025, 1},
	{12000, 2},
	{16000, 3},
	{22050, 4},
	{24000, 5},
	{32000, 6},
	{44100, 7},
	{48000, 8},
	{64000, 9},
	{88200, 10},
	{96000, 11},
};

static const struct ac107_real_to_reg ac107_slot_width[] = {
	{8,  1},
	{12, 2},
	{16, 3},
	{20, 4},
	{24, 5},
	{28, 6},
	{32, 7},
};

static const struct ac107_real_to_reg ac107_bclk_div[] = {
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

struct ac107_priv {
	struct regmap *regmap;
	unsigned int pll_freq_in;
	unsigned int pll_freq_out;
	unsigned int sysclk_freq;
	unsigned int fmt;
	int slots;
	int slot_width;

	struct ac107_data pdata;
	struct snd_sunxi_rglt *rglt;
};

static int ac107_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac107->regmap;

	/* VREF enable */
	regmap_update_bits(regmap, PWR_CTRL1, 0x1 << VREF_ENABLE, 0x1 << VREF_ENABLE);
	/* module enable */
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << ADC_DIGITAL, 0x1 << ADC_DIGITAL);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << ADC_ANALOG, 0x1 << ADC_ANALOG);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << I2S, 0x1 << I2S);
	/* ADC & I2S de-asserted */
	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << ADC_DIGITAL, 0x1 << ADC_DIGITAL);
	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << I2S, 0x1 << I2S);

	return 0;
}

static void ac107_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac107->regmap;

	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << I2S, 0x0 << I2S);
	regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << ADC_DIGITAL, 0x0 << ADC_DIGITAL);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << I2S, 0x0 << I2S);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << ADC_ANALOG, 0x0 << ADC_ANALOG);
	regmap_update_bits(regmap, MOD_CLK_EN, 0x1 << ADC_DIGITAL, 0x0 << ADC_DIGITAL);
	regmap_update_bits(regmap, PWR_CTRL1, 0x1 << VREF_ENABLE, 0x0 << VREF_ENABLE);
	regmap_update_bits(regmap, I2S_TX_CTRL1, 0xf << TX_CHSEL, 0 << TX_CHSEL);
	regmap_write(regmap, I2S_TX_CTRL2, 0);
	regmap_write(regmap, I2S_TX_CTRL3, 0);
}

static int ac107_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct ac107_data *pdata = &ac107->pdata;
	struct regmap *regmap = ac107->regmap;
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
		dev_err(dai->dev, "ac107 unsupport the sample bit\n");
		return -EINVAL;
	}
	for (i = 0; i < ARRAY_SIZE(ac107_sample_bit); i++) {
		if (ac107_sample_bit[i].real == sample_bit) {
			sample_bit_reg = ac107_sample_bit[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac107_sample_bit)) {
		dev_err(dai->dev, "ac107 unsupport the sample bit config: %u\n", sample_bit);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_FMT_CTRL2,
			   0x7 << SAMPLE_RESOLUTION, sample_bit_reg << SAMPLE_RESOLUTION);

	/* set sample rate */
	if (pdata->ecdn_mode)
		sample_rate = params_rate(params) / (ac107_ecdb_ch_nums / 2);
	else
		sample_rate = params_rate(params);
	for (i = 0; i < ARRAY_SIZE(ac107_sample_rate); i++) {
		if (ac107_sample_rate[i].real == sample_rate) {
			sample_rate_reg = ac107_sample_rate[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac107_sample_rate)) {
		dev_err(dai->dev, "ac107 unsupport the sample rate config: %u\n", sample_rate);
		return -EINVAL;
	}
	regmap_update_bits(regmap, ADC_SPRC, 0xf << ADC_FS_I2S, sample_rate_reg << ADC_FS_I2S);

	/* set channels */
	channels = params_channels(params);
	if (channels > 16) {
		dev_err(dai->dev, "ac107 unsupport the channels config: %u\n", channels);
		return -EINVAL;
	}
	if (pdata->ecdn_mode)
		channels = channels * (ac107_ecdb_ch_nums / 2);
	if (channels % 2)
		channels_en_reg = 0x1 << (pdata->codec_id * 2);
	else
		channels_en_reg = 0x3 << (pdata->codec_id * 2);

	regmap_update_bits(regmap, I2S_TX_CTRL1, 0xf << TX_CHSEL, (channels - 1) << TX_CHSEL);
	regmap_write(regmap, I2S_TX_CTRL2, channels_en_reg & 0xff);
	regmap_write(regmap, I2S_TX_CTRL3, channels_en_reg >> 8);

	/* set bclk div: ratio = sysclk / sample_rate / slots / slot_width */
	bclk_ratio = ac107->sysclk_freq / sample_rate / ac107->slots / ac107->slot_width;
	for (i = 0; i < ARRAY_SIZE(ac107_bclk_div); i++) {
		if (ac107_bclk_div[i].real == bclk_ratio) {
			bclk_ratio_reg = ac107_bclk_div[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac107_bclk_div)) {
		dev_err(dai->dev, "ac107 unsupport bclk_div: %d\n", bclk_ratio);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_BCLK_CTRL, 0xf << BCLKDIV, bclk_ratio_reg << BCLKDIV);

	/* PLLCLK enable */
	if (pdata->sysclk_src == SYSCLK_SRC_PLL)
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_EN, 0x1 << PLLCLK_EN);
	/* SYSCLK enable */
	regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_EN, 0x1 << SYSCLK_EN);
	/* ac107 ADC digital enable, I2S TX & Globle enable */
	regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << DG_EN, 0x1 << DG_EN);
	regmap_update_bits(regmap, I2S_CTRL, 0x1 << SDO_EN, 0x1 << SDO_EN);
	regmap_update_bits(regmap, I2S_CTRL, 0x1 << TXEN | 0x1 << GEN, 0x1 << TXEN | 0x1 << GEN);

	return 0;
}

static int ac107_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac107->regmap;

	regmap_update_bits(regmap, I2S_CTRL, 0x1 << TXEN | 0x1 << GEN, 0x0 << TXEN | 0x0 << GEN);
	regmap_update_bits(regmap, I2S_CTRL, 0x1 << SDO_EN, 0x0 << SDO_EN);
	regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << DG_EN, 0x0 << DG_EN);
	regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_EN, 0x0 << SYSCLK_EN);
	regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_EN, 0x0 << PLLCLK_EN);

	return 0;
}

static int ac107_set_dai_pll(struct snd_soc_dai *dai, int pll_id, int source,
			     unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct ac107_data *pdata = &ac107->pdata;
	struct regmap *regmap = ac107->regmap;
	unsigned int i  = 0;
	unsigned int m1 = 0;
	unsigned int m2 = 0;
	unsigned int n  = 0;
	unsigned int k1 = 0;
	unsigned int k2 = 0;

	if (pdata->sysclk_src != SYSCLK_SRC_PLL) {
		dev_dbg(dai->dev, "ac107 sysclk source don't pll, don't need config pll\n");
		return 0;
	}

	if (freq_in < 128000 || freq_in > 24576000) {
		dev_err(dai->dev, "ac107 pllclk source input only support [128K,24M], now %u\n",
			freq_in);
		return -EINVAL;
	}

	switch (pdata->pllclk_src) {
	case PLLCLK_SRC_MCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << PLLCLK_SRC, 0x0 << PLLCLK_SRC);
		break;
	case PLLCLK_SRC_BCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << PLLCLK_SRC, 0x1 << PLLCLK_SRC);
		break;
	case PLLCLK_SRC_PDMCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << PLLCLK_SRC, 0x2 << PLLCLK_SRC);
		break;
	default:
		dev_err(dai->dev, "ac107 pllclk source config error: %d\n", pdata->pllclk_src);
		return -EINVAL;
	}

	ac107->pll_freq_in = freq_in;
	ac107->pll_freq_out = freq_out;

	/* PLLCLK: FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] */
	for (i = 0; i < ARRAY_SIZE(ac107_pll_div); i++) {
		if (ac107_pll_div[i].freq_in == freq_in && ac107_pll_div[i].freq_out == freq_out) {
			m1 = ac107_pll_div[i].m1;
			m2 = ac107_pll_div[i].m2;
			n  = ac107_pll_div[i].n;
			k1 = ac107_pll_div[i].k1;
			k2 = ac107_pll_div[i].k2;
			dev_dbg(dai->dev, "ac107 PLL match freq_in:%u, freq_out:%u\n",
				freq_in, freq_out);
			break;
		}
	}
	if (i == ARRAY_SIZE(ac107_pll_div)) {
		dev_err(dai->dev, "ac107 PLL don't match freq_in and freq_out table\n");
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

	return 0;
}

static int ac107_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
				unsigned int freq, int dir)
{
	struct snd_soc_component *component = dai->component;
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct ac107_data *pdata = &ac107->pdata;
	struct regmap *regmap = ac107->regmap;

	/* sysclk must be 256*fs (fs=48KHz or 44.1KHz),
	 * sysclk provided by externally clk or internal pll.
	 */
	switch (pdata->sysclk_src) {
	case SYSCLK_SRC_MCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << SYSCLK_SRC, 0x0 << SYSCLK_SRC);
		break;
	case SYSCLK_SRC_BCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << SYSCLK_SRC, 0x1 << SYSCLK_SRC);
		break;
	case SYSCLK_SRC_PLL:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << SYSCLK_SRC, 0x2 << SYSCLK_SRC);
		break;
	default:
		dev_err(dai->dev, "ac107 sysclk source config error: %d\n", pdata->sysclk_src);
		return -EINVAL;
	}

	if (pdata->sysclk_src == SYSCLK_SRC_PLL) {
		ac107->sysclk_freq = ac107->pll_freq_out;
	} else {
		ac107->sysclk_freq = freq;
	}

	return 0;
}

static int ac107_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct ac107_data *pdata = &ac107->pdata;
	struct regmap *regmap = ac107->regmap;
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
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << TX_OFFSET, tx_offset << TX_OFFSET);
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

	ac107->fmt = fmt;

	return 0;
}

static int ac107_set_dai_tdm_slot(struct snd_soc_dai *dai,
				  unsigned int tx_mask, unsigned int rx_mask,
				  int slots, int slot_width)
{
	struct snd_soc_component *component = dai->component;
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac107->regmap;
	int i;
	unsigned int slot_width_reg = 0;
	unsigned int lrck_width_reg = 0;

	for (i = 0; i < ARRAY_SIZE(ac107_slot_width); i++) {
		if (ac107_slot_width[i].real == slot_width) {
			slot_width_reg = ac107_slot_width[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac107_slot_width)) {
		dev_err(dai->dev, "ac107 unsupport slot_width: %d\n", slot_width);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_FMT_CTRL2,
			   0x7 << SLOT_WIDTH_SEL, slot_width_reg << SLOT_WIDTH_SEL);

	switch (ac107->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
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

	ac107->slots = slots;
	ac107->slot_width = slot_width;

	return 0;
}

static const struct snd_soc_dai_ops ac107_dai_ops = {
	.startup	= ac107_startup,
	.shutdown	= ac107_shutdown,
	.hw_params	= ac107_hw_params,
	.hw_free	= ac107_hw_free,
	/* should: set_pll -> set_sysclk */
	.set_pll	= ac107_set_dai_pll,
	.set_sysclk	= ac107_set_dai_sysclk,
	/* should: set_fmt -> set_tdm_slot */
	.set_fmt	= ac107_set_dai_fmt,
	.set_tdm_slot	= ac107_set_dai_tdm_slot,
};

#define CREATE_DAI_DRIVER(n) \
	static struct snd_soc_dai_driver ac107_dai##n = { \
		.name = "ac107-codec" #n, \
		.capture = { \
			.stream_name	= "Capture", \
			.channels_min	= 1, \
			.channels_max	= 2, \
			.rates		= SNDRV_PCM_RATE_8000_96000 \
					| SNDRV_PCM_RATE_KNOT, \
			.formats	= SNDRV_PCM_FMTBIT_S8 \
					| SNDRV_PCM_FMTBIT_S16_LE \
					| SNDRV_PCM_FMTBIT_S20_3LE \
					| SNDRV_PCM_FMTBIT_S24_LE \
					| SNDRV_PCM_FMTBIT_S32_LE, \
			}, \
		.ops = &ac107_dai_ops, \
	}

CREATE_DAI_DRIVER(0);
CREATE_DAI_DRIVER(1);
CREATE_DAI_DRIVER(2);
CREATE_DAI_DRIVER(3);
CREATE_DAI_DRIVER(4);
CREATE_DAI_DRIVER(5);
CREATE_DAI_DRIVER(6);
CREATE_DAI_DRIVER(7);

static struct snd_soc_dai_driver *ac107_dai[] = {
	&ac107_dai0,
	&ac107_dai1,
	&ac107_dai2,
	&ac107_dai3,
	&ac107_dai4,
	&ac107_dai5,
	&ac107_dai6,
	&ac107_dai7,
};

static int ac107_probe(struct snd_soc_component *component)
{
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct ac107_data *pdata = &ac107->pdata;
	struct regmap *regmap = ac107->regmap;
	int i;

	/* adc digita volume set */
	regmap_write(regmap, ADC1_DVOL_CTRL, pdata->ch1_dig_vol);
	regmap_write(regmap, ADC2_DVOL_CTRL, pdata->ch2_dig_vol);

	/* adc pga gain set */
	regmap_update_bits(regmap, ANA_ADC1_CTRL3, 0x1f << RX1_PGA_GAIN_CTRL,
			   pdata->ch1_pga_gain << RX1_PGA_GAIN_CTRL);
	regmap_update_bits(regmap, ANA_ADC2_CTRL3, 0x1f << RX2_PGA_GAIN_CTRL,
			   pdata->ch2_pga_gain << RX2_PGA_GAIN_CTRL);

	/* I2S: SDO drive data and SDI sample data at the different BCLK edge */
	regmap_update_bits(regmap, I2S_BCLK_CTRL, 0x1 << EDGE_TRANSFER, 0x0 << EDGE_TRANSFER);

	/* I2S:
	 * disable encoding mode
	 * normal mode for the last half cycle of BCLK in the slot
	 * Turn to hi-z state (TDM) when not transferring slot
	 */
	if (pdata->ecdn_mode) {
		regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << ENCD_SEL, 0x1 << ENCD_SEL);
		regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << ENCD_FMT, 0x0 << ENCD_FMT);
	} else {
		regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << ENCD_SEL, 0x0 << ENCD_SEL);
	}
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << TX_SLOT_HIZ, 0x0 << TX_SLOT_HIZ);
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << TX_STATE, 0x1 << TX_STATE);

	/* I2S: TX MSB first, SDOUT normal, PCM frame type, Linear PCM Data Mode */
	regmap_update_bits(regmap, I2S_FMT_CTRL3,
			   0x1 << SDOUT_MUTE | 0x3 << TX_PDM,
			   0x0 << SDOUT_MUTE | 0x0 << TX_PDM);
	regmap_update_bits(regmap, I2S_FMT_CTRL3, 0x1 << TX_MLS, pdata->pcm_bit_first << TX_MLS);

	/* I2S: adc channel map to i2s channel */
	for (i = 0; i < 8; i++)
		regmap_update_bits(regmap, I2S_TX_CHMP_CTRL1, 0x1 << i,
				   ((pdata->tx_pin_chmap0 >> (i * 4)) & 0x1) << i);
	for (i = 0; i < 8; i++)
		regmap_update_bits(regmap, I2S_TX_CHMP_CTRL2, 0x1 << i,
				   ((pdata->tx_pin_chmap1 >> (i * 4)) & 0x1) << i);

	/* disable PDM (I2S use default) */
	regmap_update_bits(regmap, PDM_CTRL, 0x1 << PDM_EN, 0x0 << PDM_EN);

	/* HPF enable */
	regmap_update_bits(regmap, HPF_EN, 0x1 << DIG_ADC1_HPF_EN, 0x1 << DIG_ADC1_HPF_EN);
	regmap_update_bits(regmap, HPF_EN, 0x1 << DIG_ADC2_HPF_EN, 0x1 << DIG_ADC2_HPF_EN);

	/* ADC PTN sel: 0->normal, 1->0x5A5A5A, 2->0x123456, 3->0x000000 */
	regmap_update_bits(regmap, ADC_DIG_DEBUG, 0x7 << ADC_PTN_SEL, 0x0 << ADC_PTN_SEL);
	/* VREF Fast Start-up Enable */
	regmap_update_bits(regmap, PWR_CTRL1, 0x1 << VREF_FSU_DISABLE, 0x0 << VREF_FSU_DISABLE);
	/* SDOUT driver level:0x0->level0, 0x1->level1, 0x2->level2, 0x3->level3 */
	regmap_update_bits(regmap, I2S_PADDRV_CTRL, 0x3 << SDOUT_DRV,
			   pdata->dout_drive_level << SDOUT_DRV);

	return 0;
}

static void ac107_remove(struct snd_soc_component *component)
{
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac107->regmap;

	/* reset chip */
	regmap_write(regmap, CHIP_AUDIO_RST, 0x12);
}

static int ac107_suspend(struct snd_soc_component *component)
{
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);

	snd_sunxi_regulator_disable(ac107->rglt);

	return 0;
}

static int ac107_resume(struct snd_soc_component *component)
{
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	int ret;

	ret = snd_sunxi_regulator_enable(ac107->rglt);
	if (ret)
		return ret;

	ac107_probe(component);

	return 0;
}

static int ac107_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac107->regmap;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, PWR_CTRL2, 0x1 << MICBIAS1_EN, 0x1 << MICBIAS1_EN);
		regmap_update_bits(regmap, ANA_ADC1_CTRL5, 1 << RX1_GLOBAL_EN, 1 << RX1_GLOBAL_EN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ANA_ADC1_CTRL5, 1 << RX1_GLOBAL_EN, 0 << RX1_GLOBAL_EN);
		regmap_update_bits(regmap, PWR_CTRL2, 0x1 << MICBIAS1_EN, 0x0 << MICBIAS1_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac107_mic2_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac107_priv *ac107 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac107->regmap;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, PWR_CTRL2, 0x1 << MICBIAS2_EN, 0x1 << MICBIAS2_EN);
		regmap_update_bits(regmap, ANA_ADC2_CTRL5, 1 << RX2_GLOBAL_EN, 1 << RX2_GLOBAL_EN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ANA_ADC2_CTRL5, 1 << RX2_GLOBAL_EN, 0 << RX2_GLOBAL_EN);
		regmap_update_bits(regmap, PWR_CTRL2, 0x1 << MICBIAS2_EN, 0x0 << MICBIAS2_EN);
		break;
	default:
		break;
	}

	return 0;
}

static const DECLARE_TLV_DB_SCALE(digital_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(adc_pga_gain_tlv, 0, 100, 0);

#define CREATE_SND_KCONTROLS(n, m) \
	static const struct snd_kcontrol_new ac107_snd_controls##n[] = { \
		SOC_SINGLE_TLV("ADC" #n " Digital Volume", ADC1_DVOL_CTRL, DIG_ADCL1_VOL, \
			       0xff, 0, digital_vol_tlv), \
		SOC_SINGLE_TLV("ADC" #m " Digital Volume", ADC2_DVOL_CTRL, DIG_ADCL2_VOL, \
			       0xff, 0, digital_vol_tlv), \
		SOC_SINGLE_TLV("ADC" #n " PGA Gain", ANA_ADC1_CTRL3, RX1_PGA_GAIN_CTRL, \
			       0x1f, 0, adc_pga_gain_tlv), \
		SOC_SINGLE_TLV("ADC" #m " PGA Gain", ANA_ADC2_CTRL3, RX2_PGA_GAIN_CTRL, \
			       0x1f, 0, adc_pga_gain_tlv), \
	}; \
	static const struct snd_kcontrol_new adc##n##_digital_src_mixer[] = { \
		SOC_DAPM_SINGLE("ADC" #n " Switch", ADC1_DMIX_SRC, ADC1_ADC1_DMXL_SRC, 1, 0), \
		SOC_DAPM_SINGLE("ADC" #m " Switch", ADC1_DMIX_SRC, ADC1_ADC2_DMXL_SRC, 1, 0), \
	}; \
	static const struct snd_kcontrol_new adc##m##_digital_src_mixer[] = { \
		SOC_DAPM_SINGLE("ADC" #n " Switch", ADC2_DMIX_SRC, ADC2_ADC1_DMXL_SRC, 1, 0), \
		SOC_DAPM_SINGLE("ADC" #m " Switch", ADC2_DMIX_SRC, ADC2_ADC2_DMXL_SRC, 1, 0), \
	}; \
	static const struct snd_soc_dapm_widget ac107_dapm_widgets##n[] = { \
		SND_SOC_DAPM_INPUT("MIC" #n "P"), \
		SND_SOC_DAPM_INPUT("MIC" #n "N"), \
		SND_SOC_DAPM_INPUT("MIC" #m "P"), \
		SND_SOC_DAPM_INPUT("MIC" #m "N"), \
		SND_SOC_DAPM_PGA_E("MIC" #n " PGA", SND_SOC_NOPM, 0, 0, NULL, 0, \
				   ac107_mic1_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD), \
		SND_SOC_DAPM_PGA_E("MIC" #m " PGA", SND_SOC_NOPM, 0, 0, NULL, 0, \
				   ac107_mic2_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD), \
		SND_SOC_DAPM_PGA("ADC" #n " PGA", ADC_DIG_EN, ENAD1, 0, NULL, 0), \
		SND_SOC_DAPM_PGA("ADC" #m " PGA", ADC_DIG_EN, ENAD2, 0, NULL, 0), \
		SND_SOC_DAPM_MIXER("ADC" #n " MIXER", SND_SOC_NOPM, 0, 0, \
				   adc##n##_digital_src_mixer, ARRAY_SIZE(adc##n##_digital_src_mixer)), \
		SND_SOC_DAPM_MIXER("ADC" #m " MIXER", SND_SOC_NOPM, 0, 0, \
				   adc##m##_digital_src_mixer, ARRAY_SIZE(adc##m##_digital_src_mixer)), \
		SND_SOC_DAPM_AIF_OUT("ADC" #n, "Capture", 0, SND_SOC_NOPM, 0, 0), \
		SND_SOC_DAPM_AIF_OUT("ADC" #m, "Capture", 0, SND_SOC_NOPM, 0, 0), \
	}; \
	static const struct snd_soc_dapm_route ac107_dapm_routes##n[] = { \
		{"MIC" #n " PGA", NULL, "MIC" #n "P"}, \
		{"MIC" #n " PGA", NULL, "MIC" #n "N"}, \
		{"MIC" #m " PGA", NULL, "MIC" #m "P"}, \
		{"MIC" #m " PGA", NULL, "MIC" #m "N"}, \
		{"ADC" #n " PGA", NULL, "MIC" #n " PGA"}, \
		{"ADC" #m " PGA", NULL, "MIC" #m " PGA"}, \
		{"ADC" #n " MIXER", "ADC" #n " Switch", "ADC" #n " PGA"}, \
		{"ADC" #n " MIXER", "ADC" #m " Switch", "ADC" #m " PGA"}, \
		{"ADC" #m " MIXER", "ADC" #n " Switch", "ADC" #n " PGA"}, \
		{"ADC" #m " MIXER", "ADC" #m " Switch", "ADC" #m " PGA"}, \
		{"ADC" #n, NULL, "ADC" #n " MIXER"}, \
		{"ADC" #m, NULL, "ADC" #m " MIXER"}, \
	}; \
	static const struct snd_soc_component_driver soc_component_dev_ac107_##n = { \
		.probe			= ac107_probe, \
		.remove			= ac107_remove, \
		.suspend		= ac107_suspend, \
		.resume			= ac107_resume, \
		.controls		= ac107_snd_controls##n, \
		.num_controls		= ARRAY_SIZE(ac107_snd_controls##n), \
		.dapm_widgets		= ac107_dapm_widgets##n, \
		.num_dapm_widgets	= ARRAY_SIZE(ac107_dapm_widgets##n), \
		.dapm_routes		= ac107_dapm_routes##n, \
		.num_dapm_routes	= ARRAY_SIZE(ac107_dapm_routes##n), \
	}

CREATE_SND_KCONTROLS(1, 2);
CREATE_SND_KCONTROLS(3, 4);
CREATE_SND_KCONTROLS(5, 6);
CREATE_SND_KCONTROLS(7, 8);
CREATE_SND_KCONTROLS(9, 10);
CREATE_SND_KCONTROLS(11, 12);
CREATE_SND_KCONTROLS(13, 14);
CREATE_SND_KCONTROLS(15, 16);

static const struct snd_soc_component_driver *soc_component_dev_ac107[] = {
	&soc_component_dev_ac107_1,
	&soc_component_dev_ac107_3,
	&soc_component_dev_ac107_5,
	&soc_component_dev_ac107_7,
	&soc_component_dev_ac107_9,
	&soc_component_dev_ac107_11,
	&soc_component_dev_ac107_13,
	&soc_component_dev_ac107_15,
};

static const struct regmap_config ac107_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = AC107_MAX_REG,

	.reg_defaults = ac107_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(ac107_reg_defaults),
	.cache_type = REGCACHE_NONE,
};

static void ac107_set_params_from_of(struct i2c_client *i2c, struct ac107_data *pdata)
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
		{ "PDMCLK",	PLLCLK_SRC_PDMCLK },
	};
	struct of_keyval_tavle of_sysclk_src_table[] = {
		{ "MCLK",	SYSCLK_SRC_MCLK },
		{ "BCLK",	SYSCLK_SRC_BCLK },
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

	ret = of_property_read_u32(np, "tx-pin-chmap0", &pdata->tx_pin_chmap0);
	if (ret < 0)
		pdata->tx_pin_chmap0 = 0x10101010;
	ret = of_property_read_u32(np, "tx-pin-chmap1", &pdata->tx_pin_chmap1);
	if (ret < 0)
		pdata->tx_pin_chmap1 = 0x10101010;

	ret = of_property_read_u32(np, "ch1-dig-vol", &pdata->ch1_dig_vol);
	if (ret < 0)
		pdata->ch1_dig_vol = 160;

	ret = of_property_read_u32(np, "ch2-dig-vol", &pdata->ch2_dig_vol);
	if (ret < 0)
		pdata->ch2_dig_vol = 160;

	ret = of_property_read_u32(np, "ch1-pga-gain", &pdata->ch1_pga_gain);
	if (ret < 0)
		pdata->ch1_pga_gain = 31;

	ret = of_property_read_u32(np, "ch2-pga-gain", &pdata->ch2_pga_gain);
	if (ret < 0)
		pdata->ch2_pga_gain = 31;

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

	ret = of_property_read_u32(np, "codec-id", &pdata->codec_id);
	if (ret < 0 || pdata->codec_id > 7)
		pdata->codec_id = 0;
	if (of_property_read_bool(np, "encoding-mode")) {
		pdata->ecdn_mode = 1;
	} else {
		pdata->ecdn_mode = 0;
	}

	ret = of_property_read_u32(np, "dout-drive-level", &pdata->dout_drive_level);
	if (ret < 0 || pdata->dout_drive_level > 0x3)
		pdata->dout_drive_level = 0x1;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int ac107_i2c_probe(struct i2c_client *i2c)
#else
static int ac107_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
#endif
{
	struct ac107_data *pdata = dev_get_platdata(&i2c->dev);
	struct ac107_priv *ac107;
	int ret;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	(void)id;
#endif

	ac107 = devm_kzalloc(&i2c->dev, sizeof(*ac107), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ac107)) {
		dev_err(&i2c->dev, "Unable to allocate ac107 private data\n");
		return -ENOMEM;
	}

	ac107->regmap = devm_regmap_init_i2c(i2c, &ac107_regmap);
	if (IS_ERR(ac107->regmap))
		return PTR_ERR(ac107->regmap);

	if (pdata)
		memcpy(&ac107->pdata, pdata, sizeof(*pdata));
	else if (i2c->dev.of_node)
		ac107_set_params_from_of(i2c, &ac107->pdata);
	else
		dev_err(&i2c->dev, "Unable to allocate ac107 private data\n");

	ac107->rglt = snd_sunxi_regulator_init(&i2c->dev);

	i2c_set_clientdata(i2c, ac107);

	ret = devm_snd_soc_register_component(&i2c->dev,
					      soc_component_dev_ac107[ac107->pdata.codec_id],
					      ac107_dai[ac107->pdata.codec_id], 1);
	if (ret < 0)
		dev_err(&i2c->dev, "register ac107 codec failed: %d\n", ret);
	else
		ac107_ecdb_ch_nums += 2;

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static void ac107_i2c_remove(struct i2c_client *i2c)
#else
static int ac107_i2c_remove(struct i2c_client *i2c)
#endif
{
	struct device *dev = &i2c->dev;
	struct device_node *np = i2c->dev.of_node;
	struct ac107_priv *ac107 = dev_get_drvdata(dev);

	snd_sunxi_regulator_exit(ac107->rglt);

	devm_kfree(dev, ac107);
	of_node_put(np);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	return;
#else
	return 0;
#endif
}

static const struct of_device_id ac107_of_match[] = {
	{ .compatible = "allwinner,sunxi-ac107", },
	{ }
};
MODULE_DEVICE_TABLE(of, ac107_of_match);

static struct i2c_driver ac107_i2c_driver = {
	.driver = {
		.name = "sunxi-ac107",
		.of_match_table = ac107_of_match,
	},
	.probe = ac107_i2c_probe,
	.remove = ac107_i2c_remove,
};

module_i2c_driver(ac107_i2c_driver);

MODULE_DESCRIPTION("ASoC AC107 driver");
MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.2");

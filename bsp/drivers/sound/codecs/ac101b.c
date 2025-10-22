// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * ac101b.c -- ac101b ALSA SoC Audio driver
 *
 * Copyright (c) 2022 Allwinnertech Ltd.
 */
#define SUNXI_MODNAME		"sound-ac101b"

#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/extcon.h>
#include <linux/power_supply.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm_params.h>
#include "snd_sunxi_log.h"
#include "snd_sunxi_jack.h"
#include "snd_sunxi_common.h"
#include "ac101b.h"

#define ADC1_OUTPUT	0
#define ADC2_OUTPUT	1
#define ADC3_OUTPUT	2

static atomic_t clk_cnt = ATOMIC_INIT(0);

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(POWER_REG1),
	REG_LABEL(POWER_REG2),
	REG_LABEL(POWER_REG3),
	REG_LABEL(POWER_REG4),
	REG_LABEL(POWER_REG5),
	REG_LABEL(POWER_REG6),
	REG_LABEL(MBIAS_REG),


	REG_LABEL(DAC_REG1),
	REG_LABEL(DAC_REG2),
	REG_LABEL(DAC_REG3),
	REG_LABEL(DAC_REG4),

	REG_LABEL(HP_REG1),
	REG_LABEL(HP_REG2),
	REG_LABEL(HP_REG3),
	REG_LABEL(HP_REG4),
	REG_LABEL(HP_REG5),

	REG_LABEL(SYSCLK_CTRL),

	REG_LABEL(ADC1_REG1),
	REG_LABEL(ADC1_REG2),
	REG_LABEL(ADC1_REG3),
	REG_LABEL(ADC1_REG4),
	REG_LABEL(ADC2_REG1),
	REG_LABEL(ADC2_REG2),
	REG_LABEL(ADC2_REG3),
	REG_LABEL(ADC2_REG4),
	REG_LABEL(ADC3_REG1),
	REG_LABEL(ADC3_REG2),
	REG_LABEL(ADC3_REG3),
	REG_LABEL(ADC3_REG4),

	REG_LABEL(I2S_BCLK_CTRL),
	REG_LABEL(I2S_LRCK_CTRL2),
	REG_LABEL(I2S_FMT_CTRL2),

	REG_LABEL(I2S_RX_CTRL1),
	REG_LABEL(ADDA_FS_CTRL),

	REG_LABEL(HMIC_DET_DBC),
	REG_LABEL(HMIC_DET_TH1),
	REG_LABEL(HMIC_DET_TH2),
	REG_LABEL(HP_DET_CTRL),
	REG_LABEL(HP_DET_DBC),
	REG_LABEL(HP_DET_IRQ),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

struct ac101b_real_to_reg {
	unsigned int real;
	unsigned int reg;
};

struct ac101b_pll_div {
	unsigned int freq_in;
	unsigned int freq_out;
	unsigned int m1;
	unsigned int m2;
	unsigned int n;
	unsigned int k1;
	unsigned int k2;
};

/* PLLCLK: FOUT =(FIN * N) / [(M1+1) * (M2+1) * (K1+1) * (K2+1)] */
static const struct ac101b_pll_div ac101b_pll_div[] = {
	{400000,   24576000, 0,  0, 983,  7,  1},   /* 24.575M */
	{512000,   24576000, 0,  0, 960,  9,  1},
	{768000,   24576000, 0,  0, 640,  9,  1},
	{800000,   24576000, 0,  0, 768,  24, 0},
	{1024000,  24576000, 0,  0, 480,  9,  1},
	{1600000,  24576000, 0,  0, 384,  24, 0},
	{2048000,  24576000, 0,  0, 240,  9,  1},
	{3072000,  24576000, 0,  0, 160,  9,  1},
	{4096000,  24576000, 3,  0, 480,  9,  1},
	{6000000,  24576000, 4,  0, 512,  24, 0},
	{12000000, 24576000, 9,  0, 512,  24, 0},
	{13000000, 24576000, 12, 0, 639,  12, 1},   /* 24.577M */
	{15360000, 24576000, 9,  0, 320,  9,  1},
	{16000000, 24576000, 9,  0, 384,  24, 0},
	{19200000, 24576000, 11, 0, 384,  24, 0},
	{19680000, 24576000, 15, 1, 999,  24, 0},   /* 24.575M */
	{24000000, 24576000, 9,  0, 256,  24, 0},

	{400000,   22579200, 0,  0, 1016, 17, 1},   /* 22.5778M */
	{512000,   22579200, 0,  0, 882,  19, 1},
	{768000,   22579200, 0,  0, 588,  19, 1},
	{800000,   22579200, 0,  0, 508,  17, 1},   /* 22.5778M */
	{1024000,  22579200, 0,  0, 441,  19, 1},
	{1600000,  22579200, 0,  0, 254,  17, 1},   /* 22.5778M */
	{2048000,  22579200, 1,  0, 441,  19, 1},
	{3072000,  22579200, 0,  0, 147,  19, 1},
	{4096000,  22579200, 3,  0, 441,  19, 1},
	{6000000,  22579200, 1,  0, 143,  18, 1},   /* 22.5789M */
	{12000000, 22579200, 3,  0, 143,  18, 1},   /* 22.5789M */
	{13000000, 22579200, 12, 0, 429,  18, 1},   /* 22.5789M */
	{15360000, 22579200, 14, 0, 441,  19, 1},
	{16000000, 22579200, 24, 0, 882,  24, 1},
	{19200000, 22579200, 4,  0, 147,  24, 1},
	{19680000, 22579200, 13, 1, 771,  23, 1},   /* 22.5793M */
	{24000000, 22579200, 24, 0, 588,  24, 1},
};

static const struct ac101b_real_to_reg ac101b_bclk_div[] = {
	{1,   1},
	{2,   2},
	{4,   3},
	{6,   4},
	{8,   5},
	{12,  6},
	{16,  7},
	{24,  8},
	{32,  9},
	{48,  10},
	{64,  11},
	{96,  12},
	{128, 13},
	{176, 14},
	{192, 15},
};

static const struct ac101b_real_to_reg ac101b_sample_rate_div[] = {
	{1,  0},
	{2,  1},
	{3,  2},
	{4,  3},
	{6,  4},
	{8,  5},
	{12, 6},
	{16, 7},
	{24, 8},
};

static const struct ac101b_real_to_reg ac101b_sample_bit[] = {
	{16, 3},
	{20, 4},
	{24, 5},
	{28, 6},
	{32, 7},
};


static const struct ac101b_real_to_reg ac101b_slot_width[] = {
	{16, 3},
	{20, 4},
	{24, 5},
	{28, 6},
	{32, 7},
};

struct ac101b_priv {
	struct regmap *regmap;
	struct device *dev;
	unsigned int sysclk_freq;
	unsigned int fmt;
	int slots;
	int slot_width;

	struct ac101b_data pdata;
	struct snd_sunxi_rglt *rglt;
};

/* jack work  */
static int sunxi_jack_adv_init(void *data);
static void sunxi_jack_adv_exit(void *data);
static int sunxi_jack_adv_suspend(void *data);
static int sunxi_jack_adv_resume(void *data);
static int sunxi_jack_adv_irq_request(void *data, jack_irq_work jack_interrupt);
static void sunxi_jack_adv_irq_free(void *data);
static void sunxi_jack_adv_irq_clean(void *data, int irq);
static void sunxi_jack_adv_irq_enable(void *data);
static void sunxi_jack_adv_irq_disable(void *data);
static void sunxi_jack_adv_det_irq_work(void *data, enum snd_jack_types *jack_type);
static void sunxi_jack_adv_det_scan_work(void *data, enum snd_jack_types *jack_type);

struct sunxi_jack_adv sunxi_jack_adv = {
	.jack_init	= sunxi_jack_adv_init,
	.jack_exit	= sunxi_jack_adv_exit,
	.jack_suspend	= sunxi_jack_adv_suspend,
	.jack_resume	= sunxi_jack_adv_resume,

	.jack_irq_requeset	= sunxi_jack_adv_irq_request,
	.jack_irq_free		= sunxi_jack_adv_irq_free,
	.jack_irq_clean		= sunxi_jack_adv_irq_clean,
	.jack_irq_enable	= sunxi_jack_adv_irq_enable,
	.jack_irq_disable	= sunxi_jack_adv_irq_disable,
	.jack_det_irq_work	= sunxi_jack_adv_det_irq_work,
	.jack_det_scan_work	= sunxi_jack_adv_det_scan_work,
};

static int sunxi_jack_adv_init(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;

	regmap_update_bits(regmap, HMIC_DET_TH1, 0x1f << HMIC_TH1,
			   jack_adv_priv->det_threshold << HMIC_TH2);
	regmap_update_bits(regmap, HMIC_DET_DBC, 0xf << HMIC_N,
			   jack_adv_priv->det_debounce << HMIC_N);

	regmap_update_bits(regmap, HMIC_DET_TH2, 0x1f << HMIC_TH2,
			   jack_adv_priv->key_threshold << HMIC_TH2);
	regmap_update_bits(regmap, HMIC_DET_DBC, 0xf << HMIC_M,
			   jack_adv_priv->key_debounce << HMIC_M);

	/* hp_det */
	regmap_update_bits(regmap, HP_DET_CTRL, 0x1 << HP_DET_EN, 0x1 << HP_DET_EN);
	regmap_update_bits(regmap, HBIAS_REG, 0x1 << HBIAS_MODE, 0x1 << HBIAS_MODE);

	regmap_update_bits(regmap, HP_DET_IRQ, 0x1 << HP_PLUGIN_IRQ,
			   0x1 << HP_PLUGIN_IRQ);

	if (of_property_read_bool(jack_adv_priv->dev->of_node, "extcon")) {
		jack_adv_priv->typec = true;
	}

	jack_adv_priv->irq_sta = JACK_IRQ_NULL;

	SND_LOG_DEBUG("\n");

	return 0;
}

static void sunxi_jack_adv_exit(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;

	SND_LOG_DEBUG("\n");

	/* disable all irq */
	regmap_write(regmap, HP_DET_IRQ, 0x0);
}

static int sunxi_jack_adv_suspend(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;

	/* close hbias */
	regmap_update_bits(regmap, HBIAS_REG,
				0x1 << HBIAS_EN, 0x0 << HBIAS_EN);
	regmap_update_bits(regmap, HBIAS_REG,
				0x1 << HBIASADC_EN, 0x0 << HBIASADC_EN);

	regmap_update_bits(regmap, HP_DET_CTRL, 0x1 << HP_DET_EN, 0x0 << HP_DET_EN);

	regmap_update_bits(regmap, HP_DET_IRQ, 0x1 << HP_PLUGIN_IRQ,
			   0x0 << HP_PLUGIN_IRQ);
	return 0;
}

static int sunxi_jack_adv_resume(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;

	regmap_update_bits(regmap, HBIAS_REG,
				0x1 << HBIAS_EN, 0x1 << HBIAS_EN);
	regmap_update_bits(regmap, HBIAS_REG,
				0x1 << HBIASADC_EN, 0x1 << HBIASADC_EN);

	regmap_update_bits(regmap, HP_DET_CTRL, 0x1 << HP_DET_EN, 0x1 << HP_DET_EN);

	regmap_update_bits(regmap, HP_DET_IRQ, 0x1 << HP_PLUGIN_IRQ,
			   0x1 << HP_PLUGIN_IRQ);

	return 0;
}

static int sunxi_jack_adv_irq_request(void *data, jack_irq_work jack_interrupt)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	int ret;

	SND_LOG_DEBUG("\n");

	/* irq */
	ret = gpio_request_one(jack_adv_priv->irq_gpio, GPIOF_IN, "Headphone detection");
	if (ret) {
		SND_LOG_ERR("jack-detgpio (%d) request failed, err:%d\n", jack_adv_priv->irq_gpio, ret);
		return ret;
	}

	jack_adv_priv->desc = gpio_to_desc(jack_adv_priv->irq_gpio);
	ret = request_irq(gpiod_to_irq(jack_adv_priv->desc),
				      (void *)jack_interrupt,
				      IRQF_TRIGGER_RISING,
				      "Headphone detection",
				      jack_adv_priv);
	if (ret) {
		SND_LOG_ERR("jack-detgpio (%d) request irq failed\n", jack_adv_priv->irq_gpio);
		return ret;
	}

	return ret;
}

static void sunxi_jack_adv_irq_free(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;

	SND_LOG_DEBUG("\n");

	gpio_free(jack_adv_priv->irq_gpio);
	gpiod_unexport(jack_adv_priv->desc);
	free_irq(gpiod_to_irq(jack_adv_priv->desc), jack_adv_priv);
	gpiod_put(jack_adv_priv->desc);
}

static void sunxi_jack_adv_irq_enable(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;

	SND_LOG_DEBUG("\n");

	enable_irq(gpiod_to_irq(jack_adv_priv->desc));
}

static void sunxi_jack_adv_irq_disable(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;

	SND_LOG_DEBUG("\n");

	disable_irq(gpiod_to_irq(jack_adv_priv->desc));
}

static void sunxi_jack_adv_irq_clean(void *data, int irq)
{
	return;
}

static void sunxi_adv_headset_heasphone_det(struct sunxi_jack_adv_priv *jack_adv_priv,
					    enum snd_jack_types *jack_type)
{
	struct regmap *regmap = jack_adv_priv->regmap;
	unsigned int reg_val;
	unsigned int headset_basedata;
	unsigned int i;
	int count = 50;
	int interval_ms = 10;

	for (i = 0; i < count; i++) {
		regmap_read(regmap, HMIC_DET_DATA, &reg_val);
		headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;
		if (headset_basedata > 0 && headset_basedata < jack_adv_priv->key_threshold) {
			SND_LOG_INFO("\033[31m headset_basedata:%d key_threshold:%d\033[0m\n",
				     headset_basedata, jack_adv_priv->key_threshold);
			goto headset;
		}
		msleep(interval_ms);
	}
	SND_LOG_INFO("\033[31m headset_basedata:%d key_threshold:%d\033[0m\n",
		     headset_basedata, jack_adv_priv->key_threshold);

	/* for special jack */
	if (headset_basedata == 0) {
		goto headset;
	}

	*jack_type = SND_JACK_HEADPHONE;

	regmap_update_bits(regmap, HP_DET_IRQ,
			   0x1 << HP_PLUGOUT_IRQ, 0x1 << HP_PLUGOUT_IRQ);
	regmap_update_bits(regmap, HP_DET_IRQ,
			   0x1 << HP_PLUGIN_IRQ, 0x0 << HP_PLUGIN_IRQ);
	regmap_update_bits(regmap, HP_DET_IRQ,
			   0x1 << HMIC_KEYUP_IRQ, 0x0 << HMIC_KEYUP_IRQ);
	regmap_update_bits(regmap, HP_DET_IRQ,
			   0x1 << HMIC_KEYDOWN_IRQ, 0x0 << HMIC_KEYDOWN_IRQ);
	regmap_update_bits(regmap, HP_DET_IRQ,
			   0x1 << HMIC_DATA_IRQ, 0x0 << HMIC_DATA_IRQ);

	/* close hbias */
	regmap_update_bits(regmap, HBIAS_REG,
				0x1 << HBIAS_EN, 0x0 << HBIAS_EN);
	regmap_update_bits(regmap, HBIAS_REG,
				0x1 << HBIASADC_EN, 0x0 << HBIASADC_EN);

	return;

headset:
	*jack_type = SND_JACK_HEADSET;

	regmap_update_bits(regmap, HMIC_DET_TH2, 0x1f << HMIC_TH2,
				jack_adv_priv->key_threshold << HMIC_TH2);

	regmap_update_bits(regmap, HP_DET_IRQ,
				0x1 << HMIC_KEYDOWN_IRQ, 0x1 << HMIC_KEYDOWN_IRQ);
	regmap_update_bits(regmap, HP_DET_IRQ,
				0x1 << HP_PLUGOUT_IRQ, 0x1 << HP_PLUGOUT_IRQ);
	regmap_update_bits(regmap, HP_DET_IRQ,
				0x1 << HMIC_PLUGOUT_IRQ, 0x0 << HMIC_PLUGOUT_IRQ);
	regmap_update_bits(regmap, HP_DET_IRQ,
			   0x1 << HP_PLUGIN_IRQ, 0x0 << HP_PLUGIN_IRQ);
	return;

}

static void sunxi_jack_adv_det_irq_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;
	unsigned int reg_val, irqen_val, hp_det_val, hmic_data, reg_val_tmp;

	regmap_read(regmap, HP_DET_IRQ, &irqen_val);
	regmap_read(regmap, HP_DET_STA, &reg_val);
	regmap_read(regmap, HP_DET_CTRL, &hp_det_val);
	regmap_read(regmap, HMIC_DET_DATA, &hmic_data);

	if (hp_det_val & (1 << HP_DET)) {
		reg_val |= (0x1 << HP_PLUGOUT_PENDING);
		reg_val |= (0x1 << HMIC_PLUGOUT_PENDING);
		regmap_write(regmap, HP_DET_STA, reg_val);
		jack_adv_priv->irq_sta = JACK_IRQ_OUT;
	} else if ((irqen_val & (0x1 << HP_PLUGIN_IRQ))
		   && (reg_val & (0x1 << HP_PLUGIN_PENDING))) {
		reg_val |= (0x1 << HP_PLUGIN_PENDING);
		regmap_write(regmap, HP_DET_STA, reg_val);

		regmap_update_bits(regmap, HP_DET_IRQ,
				   0x1 << HP_PLUGIN_IRQ,
				   0x0 << HP_PLUGIN_IRQ);
		regmap_update_bits(regmap, HP_DET_IRQ,
				   0x1 << HMIC_KEYDOWN_IRQ,
				   0x1 << HMIC_KEYDOWN_IRQ);
		jack_adv_priv->irq_sta = JACK_IRQ_IN;
	} else if ((irqen_val & (0x1 << HMIC_KEYDOWN_IRQ))
		   && (reg_val & (0x1 << HMIC_KEYDOWN_PENDING))) {
		reg_val |= (0x1 << HMIC_KEYDOWN_PENDING);
		regmap_write(regmap, HP_DET_STA, reg_val);
		jack_adv_priv->irq_sta = JACK_IRQ_KEYDOWN;
	}

	switch (jack_adv_priv->irq_sta) {
	case JACK_IRQ_OUT:
		SND_LOG_INFO("jack out\n");
		regmap_update_bits(regmap, HBIAS_REG,
				   0x1 << HBIAS_EN, 0x0 << HBIAS_EN);
		regmap_update_bits(regmap, HBIAS_REG,
				   0x1 << HBIASADC_EN, 0x0 << HBIASADC_EN);

		regmap_update_bits(regmap, HP_DET_IRQ,
				   0x1 << HP_PLUGIN_IRQ, 0x1 << HP_PLUGIN_IRQ);

		*jack_type = 0;
	break;

	case JACK_IRQ_IN:
		SND_LOG_INFO("jack in\n");

		regmap_update_bits(regmap, HBIAS_REG,
				   0x1 << HBIAS_EN, 0x1 << HBIAS_EN);
		regmap_update_bits(regmap, HBIAS_REG,
				   0x1 << HBIASADC_EN, 0x1 << HBIASADC_EN);

		msleep(100);
		sunxi_adv_headset_heasphone_det(jack_adv_priv, jack_type);
	break;

	case JACK_IRQ_KEYDOWN:
		SND_LOG_INFO("jack button\n");

		regmap_read(regmap, HMIC_DET_DATA, &reg_val);
		reg_val_tmp = (reg_val >> HMIC_DATA) & 0x1f;
		SND_LOG_INFO("\033[31m reg_val: 0x%x, HMIC_DATA:%u, vol+:[%d, %d], vol-:[%d %d], hook:[%d %d]\033[0m \n",
			     reg_val,
			     reg_val_tmp,
			     jack_adv_priv->key_det_vol[1][0], jack_adv_priv->key_det_vol[1][1],
			     jack_adv_priv->key_det_vol[2][0], jack_adv_priv->key_det_vol[2][1],
			     jack_adv_priv->key_det_vol[0][0], jack_adv_priv->key_det_vol[0][1]);

		/* SND_JACK_BTN_0 - key-hook
		 * SND_JACK_BTN_1 - key-up
		 * SND_JACK_BTN_2 - key-down
		 * SND_JACK_BTN_3 - key-voice
		 */
		if (reg_val >= jack_adv_priv->key_det_vol[0][0] &&
		    reg_val <= jack_adv_priv->key_det_vol[0][1]) {
			*jack_type |= SND_JACK_BTN_0;
		} else if (reg_val >= jack_adv_priv->key_det_vol[1][0] &&
			   reg_val <= jack_adv_priv->key_det_vol[1][1]) {
			*jack_type |= SND_JACK_BTN_1;
		} else if (reg_val >= jack_adv_priv->key_det_vol[2][0] &&
			   reg_val <= jack_adv_priv->key_det_vol[2][1]) {
			*jack_type |= SND_JACK_BTN_2;
		} else {
			SND_LOG_DEBUG("unsupport jack button\n");
		}
	break;

	default:
		SND_LOG_DEBUG("irq status is invaild\n");
	break;
	}

	jack_adv_priv->jack_type = *jack_type;

	return;
}

static void sunxi_jack_adv_det_scan_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;
	unsigned int hp_det_val;

	regmap_read(regmap, HP_DET_CTRL, &hp_det_val);
	if (!(hp_det_val & (1 << HP_DET))) {
		sunxi_adv_headset_heasphone_det(jack_adv_priv, jack_type);
	} else {
		*jack_type = 0;
		jack_adv_priv->irq_sta = JACK_IRQ_OUT;
		regmap_write(regmap, HP_DET_STA, 0x7f);
		regmap_update_bits(regmap, HP_DET_IRQ,
				   0x1 << HP_PLUGIN_IRQ, 0x1 << HP_PLUGIN_IRQ);

	}

	jack_adv_priv->jack_type = *jack_type;

	return;

}

static struct sunxi_jack_port sunxi_jack_port = {
	.jack_adv = &sunxi_jack_adv,
};

static int ac101b_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG("\n");

	return 0;
}

static void ac101b_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;

	SND_LOG_DEBUG("\n");

	/* fix pop when playback stop, need close pa firstly */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DAC_EN, 0x0 << DAC_EN);
		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DACL_DIG_EN, 0x0 << DACL_DIG_EN);
		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DACR_DIG_EN, 0x0 << DACR_DIG_EN);

		regmap_update_bits(regmap, DAC_REG1, 0x1 << DACL_EN, 0x0 << DACL_EN);
		regmap_update_bits(regmap, DAC_REG1, 0x1 << DACR_EN, 0x0 << DACR_EN);
		regmap_update_bits(regmap, DAC_REG3, 0x1 << SPKL_EN, 0x0 << SPKL_EN);
		regmap_update_bits(regmap, DAC_REG3, 0x1 << SPKR_EN, 0x0 << SPKR_EN);
	}
}

static int ac101b_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct snd_soc_card *card = component->card;
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;
	int i;
	unsigned int sample_bit;
	unsigned int sample_rate;
	unsigned int channels;
	unsigned int bclk_ratio;
	unsigned int osr;
	unsigned int sample_bit_reg = 0;
	unsigned int bclk_ratio_reg = 0;
	unsigned int channels_en_reg = 0;
	unsigned int adda_fs_div = 0;
	unsigned int adda_fs_div_reg = 0;

	SND_LOG_DEBUG("\n");

	/* set sample bit */
	switch (params_format(params)) {
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
		dev_err(dai->dev, "ac101b unsupport the sample bit\n");
		return -EINVAL;
	}
	for (i = 0; i < ARRAY_SIZE(ac101b_sample_bit); i++) {
		if (ac101b_sample_bit[i].real == sample_bit) {
			sample_bit_reg = ac101b_sample_bit[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101b_sample_bit)) {
		dev_err(dai->dev, "ac101b unsupport the sample bit config: %u\n", sample_bit);
		return -EINVAL;
	}

	regmap_update_bits(regmap, I2S_FMT_CTRL2, 0x3 << SAMPLE_RES, sample_bit_reg << SAMPLE_RES);

	/* set fs_div and osr */
	sample_rate = params_rate(params);
	if (sample_rate > 96000) {
		osr = 32;
	} else if (sample_rate == 96000) {
		osr = 64;
	} else {
		osr = 128;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, DAC_DIG_CTRL, 0x3 << DAC_OSR, osr << DAC_OSR);
	} else {
		regmap_update_bits(regmap, ADC_DDT_CTRL, 0x2 << ADC_OSR, osr << ADC_OSR);
	}

	adda_fs_div = ac101b->sysclk_freq / sample_rate / osr;

	for (i = 0; i < ARRAY_SIZE(ac101b_sample_rate_div); i++) {
		if (ac101b_sample_rate_div[i].real == adda_fs_div) {
			adda_fs_div_reg = ac101b_sample_rate_div[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101b_sample_rate_div)) {
		dev_err(dai->dev, "ac101b unsupport the adda fs divconfig: %u\n", adda_fs_div);
		return -EINVAL;
	}
	regmap_update_bits(regmap, ADDA_FS_CTRL, 0xF << ADDA_FS_DIV, adda_fs_div_reg << ADDA_FS_DIV);

	/* set channel */
	channels = params_channels(params);
	if (channels > 16) {
		dev_err(dai->dev, "ac101b unsupport the channels config: %u\n", channels);
		return -EINVAL;
	}

	/* slot */
	for (i = 0; i < channels; i++)
		channels_en_reg |= (1 << i);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, I2S_RX_CTRL1, 0xf << RX_CHSEL, (channels - 1) << RX_CHSEL);
		/* rx_chan_low, ch0~ch8*/
		regmap_write(regmap, I2S_RX_CTRL2, channels_en_reg & 0xff);
		/* rx_chan_high ch9~ch16*/
		regmap_write(regmap, I2S_RX_CTRL3, channels_en_reg >> 8);

	} else {
		regmap_update_bits(regmap, I2S_TX_CTRL1, 0xf << TX_CHSEL, (channels - 1) << TX_CHSEL);
		/* tx_chan_low, ch0~ch8*/
		regmap_write(regmap, I2S_TX_CTRL2, channels_en_reg & 0xff);
		/* tx_chan_high ch9~ch16*/
		regmap_write(regmap, I2S_TX_CTRL3, channels_en_reg >> 8);
	}

	/* set bclk div: ratio = sysclk / sample_rate / slots / slot_width */
	bclk_ratio = ac101b->sysclk_freq / sample_rate / ac101b->slots / ac101b->slot_width;
	for (i = 0; i < ARRAY_SIZE(ac101b_bclk_div); i++) {
		if (ac101b_bclk_div[i].real == bclk_ratio) {
			bclk_ratio_reg = ac101b_bclk_div[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101b_bclk_div)) {
		dev_err(dai->dev, "ac101b unsupport bclk_div: %d\n", bclk_ratio);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_BCLK_CTRL, 0xf << BCLK_DIV, bclk_ratio_reg << BCLK_DIV);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << RX_EN, 0x1 << RX_EN);
	} else {
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << SDO_EN, 0x1 << SDO_EN);
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << TX_EN, 0x1 << TX_EN);
	}

	if (!atomic_read(&clk_cnt)) {
		/* PLLCLK enable */
		if (pdata->sysclk_src == SYSCLK_SRC_PLL)
			regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_EN, 0x1 << PLLCLK_EN);

		/* SYSCLK enable */
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_EN, 0x1 << SYSCLK_EN);

		regmap_update_bits(regmap, I2S_CTRL, 0x1 << I2S_GLOBE_EN, 0x1 << I2S_GLOBE_EN);
	}

	atomic_add(1, &clk_cnt);

	return 0;
}

static int ac101b_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;

	SND_LOG_DEBUG("\n");

	atomic_sub(1, &clk_cnt);

	if (!atomic_read(&clk_cnt)) {
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_EN, 0x0 << SYSCLK_EN);
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_EN, 0x0 << PLLCLK_EN);
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << I2S_GLOBE_EN, 0x0 << I2S_GLOBE_EN);
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << RX_EN, 0x0 << RX_EN);
	} else {
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << SDO_EN, 0x0 << SDO_EN);
		regmap_update_bits(regmap, I2S_CTRL, 0x1 << TX_EN, 0x0 << TX_EN);
	}

	return 0;
}

static int ac101b_set_dai_pll(struct snd_soc_dai *dai, int pll_id, int source,
			      unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;
	unsigned int i  = 0;
	unsigned int m1 = 0;
	unsigned int m2 = 0;
	unsigned int n  = 0;
	unsigned int k1 = 0;
	unsigned int k2 = 0;

	SND_LOG_DEBUG("\n");

	if (freq_in < 400000 || freq_in > 24576000) {
		dev_err(dai->dev, "ac101b pllclk source input only support [400K,24M], now %u\n",
			freq_in);
		return -EINVAL;
	}

	if (pdata->sysclk_src != SYSCLK_SRC_PLL) {
		dev_dbg(dai->dev, "ac101b sysclk source don't pll, don't need config pll\n");
		return 0;
	}

	switch (pdata->pllclk_src) {
	case PLLCLK_SRC_MCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_SRC, 0x0 << PLLCLK_SRC);
		break;
	case PLLCLK_SRC_BCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_SRC, 0x1 << PLLCLK_SRC);
		break;
	default:
		dev_err(dai->dev, "ac101b pllclk source config error: %d\n", pdata->pllclk_src);
		return -EINVAL;
	}

	/* PLLCLK: FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] */
	for (i = 0; i < ARRAY_SIZE(ac101b_pll_div); i++) {
		if (ac101b_pll_div[i].freq_in == freq_in && ac101b_pll_div[i].freq_out == freq_out) {
			m1 = ac101b_pll_div[i].m1;
			m2 = ac101b_pll_div[i].m2;
			n  = ac101b_pll_div[i].n;
			k1 = ac101b_pll_div[i].k1;
			k2 = ac101b_pll_div[i].k2;
			dev_dbg(dai->dev, "ac101b PLL match freq_in:%u, freq_out:%u\n",
				freq_in, freq_out);
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101b_pll_div)) {
		dev_err(dai->dev, "ac101b PLL don't match freq_in and freq_out table\n");
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
	regmap_update_bits(regmap, PLL_CTRL8, 0x1 << PLL_LOCK_EN, 0x1 << PLL_LOCK_EN);

	return 0;
}

static int ac101b_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
				unsigned int freq, int dir)
{
	struct snd_soc_component *component = dai->component;
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;

	SND_LOG_DEBUG("\n");

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
		dev_err(dai->dev, "ac101b sysclk source config error: %d\n", pdata->sysclk_src);
		return -EINVAL;
	}

	ac101b->sysclk_freq = freq;

	return 0;
}

static int ac101b_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	struct ac101b_data *pdata = &ac101b->pdata;
	unsigned int i2s_mode, tx_offset, sign_ext;
	unsigned int brck_polarity, lrck_polarity;

	SND_LOG_DEBUG("\n");

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
	case SND_SOC_DAIFMT_LEFT_J:
		i2s_mode = 1;
		tx_offset = 0;
		sign_ext = 3;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		i2s_mode = 2;
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
	regmap_update_bits(regmap, I2S_FMT_CTRL1, 0x1 << OFFSET, i2s_mode << OFFSET);
	regmap_update_bits(regmap, I2S_FMT_CTRL3, 0x3 << SEXT, sign_ext << SEXT);
	regmap_update_bits(regmap, I2S_LRCK_CTRL1, 0x1 << LRCK_WIDTH,
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
			   0x1 << BCLK_POLARITY,
			   brck_polarity << BCLK_POLARITY);
	regmap_update_bits(regmap, I2S_LRCK_CTRL1,
			   0x1 << LRCK_POLARITY,
			   lrck_polarity << LRCK_POLARITY);

	ac101b->fmt = fmt;

	return 0;
}

static int ac101b_set_dai_tdm_slot(struct snd_soc_dai *dai,
				  unsigned int tx_mask, unsigned int rx_mask,
				  int slots, int slot_width)
{
	struct snd_soc_component *component = dai->component;
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	int i;
	unsigned int slot_width_reg = 0;
	unsigned int lrck_width_reg = 0;

	SND_LOG_DEBUG("\n");

	for (i = 0; i < ARRAY_SIZE(ac101b_slot_width); i++) {
		if (ac101b_slot_width[i].real == slot_width) {
			slot_width_reg = ac101b_slot_width[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101b_slot_width)) {
		dev_err(dai->dev, "ac101b unsupport slot_width: %d\n", slot_width);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_FMT_CTRL2,
			   0x7 << SLOT_WIDTH,
			   slot_width_reg << SLOT_WIDTH);

	switch (ac101b->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
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

	ac101b->slots = slots;
	ac101b->slot_width = slot_width;

	return 0;
}

static const struct snd_soc_dai_ops ac101b_dai_ops = {
	.startup	= ac101b_startup,
	.shutdown	= ac101b_shutdown,
	.hw_params	= ac101b_hw_params,
	.hw_free	= ac101b_hw_free,
	/* should: set_pll -> set_sysclk */
	.set_pll	= ac101b_set_dai_pll,
	.set_sysclk	= ac101b_set_dai_sysclk,
	/* should: set_fmt -> set_tdm_slot */
	.set_fmt	= ac101b_set_dai_fmt,
	.set_tdm_slot	= ac101b_set_dai_tdm_slot,
};

static struct snd_soc_dai_driver ac101b_dai = {
	.name = "ac101b-codec",
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S8
				| SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 4,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S8
				| SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
	.ops = &ac101b_dai_ops,
};


static int sunxi_get_plugin_dtime_mode(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HP_DET_DBC, &reg_val);

	ucontrol->value.integer.value[0] = (reg_val >> PLUG_IN_DBC) & 0x7;

	return 0;
}

static int sunxi_put_plugin_dtime_mode(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HP_DET_DBC, 0x7 << PLUG_IN_DBC, reg_val << PLUG_IN_DBC);

	return 0;
}

static int sunxi_get_plugout_dtime_mode(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HP_DET_DBC, &reg_val);

	ucontrol->value.integer.value[0] = (reg_val >> PLUG_OUT_DBC) & 0x7;

	return 0;
}

static int sunxi_put_plugout_dtime_mode(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HP_DET_DBC, 0x7 << PLUG_OUT_DBC, reg_val << PLUG_OUT_DBC);

	return 0;
}

static int sunxi_get_key_det_thr_mode(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_DET_TH2, &reg_val);

	ucontrol->value.integer.value[0] = (reg_val >> HMIC_TH2) & 0x1f;

	return 0;
}

static int sunxi_put_key_det_thr_mode(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_DET_TH2, 0x1f << HMIC_TH2, reg_val << HMIC_TH2);

	return 0;
}

static int sunxi_get_plug_det_thr_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_DET_TH1, &reg_val);

	ucontrol->value.integer.value[0] =  (reg_val >> HMIC_TH1) & 0x1f;

	return 0;
}

static int sunxi_put_plug_det_thr_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_DET_TH1, 0x1f << HMIC_TH1, reg_val << HMIC_TH1);

	return 0;
}

static int sunxi_get_sample_sel_mode(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_DET_CTRL, &reg_val);

	ucontrol->value.integer.value[0] =  (reg_val >> HMIC_SAMPLE_SEL) & 0x3;

	return 0;
}

static int sunxi_put_sample_sel_mode(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_DET_CTRL, 0x3 << HMIC_SAMPLE_SEL, reg_val << HMIC_SAMPLE_SEL);

	return 0;
}

static int sunxi_get_smooth_filter_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_DET_CTRL, &reg_val);

	ucontrol->value.integer.value[0] =  (reg_val >> HMIC_SF) & 0x3;

	return 0;
}

static int sunxi_put_smooth_filter_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_DET_CTRL, 0x3 << HMIC_SF, reg_val << HMIC_SF);

	return 0;
}

static const struct snd_kcontrol_new ac101b_jack_controls[] = {
	/* hmic sample sel*/
	SOC_SINGLE_EXT("hmic sample sel", HMIC_DET_CTRL, HMIC_SAMPLE_SEL, 3, 0,
		       sunxi_get_sample_sel_mode,
		       sunxi_put_sample_sel_mode),

	SOC_SINGLE_EXT("hmic smooth filter", HMIC_DET_CTRL, HMIC_SF, 3, 0,
		       sunxi_get_smooth_filter_mode,
		       sunxi_put_smooth_filter_mode),

	SOC_SINGLE_EXT("plug in debounce time", HP_DET_DBC, PLUG_IN_DBC, 6, 0,
		       sunxi_get_plugin_dtime_mode,
		       sunxi_put_plugin_dtime_mode),
	SOC_SINGLE_EXT("plug out debounce time", HP_DET_DBC, PLUG_OUT_DBC, 6, 0,
		       sunxi_get_plugout_dtime_mode,
		       sunxi_put_plugout_dtime_mode),

	SOC_SINGLE_EXT("plug det threshold", HMIC_DET_TH1, HMIC_TH1, 31, 0,
		       sunxi_get_plug_det_thr_mode,
		       sunxi_put_plug_det_thr_mode),

	SOC_SINGLE_EXT("key det threshold", HMIC_DET_TH2, HMIC_TH2, 31, 0,
		       sunxi_get_key_det_thr_mode,
		       sunxi_put_key_det_thr_mode),
};

static int ac101b_probe(struct snd_soc_component *component)
{
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;
	int ret;

	SND_LOG_DEBUG("\n");

	regmap_write(regmap, CHIP_SOFT_RST, 0x34);

	mutex_init(&pdata->ac101b_sta.dac_chp_mutex);
	mutex_init(&pdata->ac101b_sta.spk_chp_mutex);
	mutex_init(&pdata->ac101b_sta.mic_mutex);

	/* adc dig vol */
	regmap_update_bits(regmap, ADC1_DVOL_CTRL, 0xff << DIG_ADC1_VOL,
			   pdata->adc1_vol << DIG_ADC1_VOL);
	regmap_update_bits(regmap, ADC2_DVOL_CTRL, 0xff << DIG_ADC2_VOL,
			   pdata->adc2_vol << DIG_ADC2_VOL);
	regmap_update_bits(regmap, ADC3_DVOL_CTRL, 0xff << DIG_ADC3_VOL,
			   pdata->adc3_vol << DIG_ADC3_VOL);

	/* dac dig vol*/
	regmap_update_bits(regmap, DAC_DVC_L, 0xff << DACL_DIG_VOL,
			   pdata->dacl_vol << DACL_DIG_VOL);
	regmap_update_bits(regmap, DAC_DVC_R, 0xff << DACR_DIG_VOL,
			   pdata->dacr_vol << DACR_DIG_VOL);

	/* adc gain*/
	regmap_update_bits(regmap, ADC1_REG3, 0x1f << ADC1_PGA_GAIN_CTRL,
			   pdata->mic1_gain << ADC1_PGA_GAIN_CTRL);
	regmap_update_bits(regmap, ADC2_REG3, 0x1f << ADC2_PGA_GAIN_CTRL,
			   pdata->mic2_gain << ADC2_PGA_GAIN_CTRL);
	regmap_update_bits(regmap, ADC3_REG3, 0x1f << ADC3_PGA_GAIN_CTRL,
			   pdata->mic3_gain << ADC3_PGA_GAIN_CTRL);

	/* hpout gain */
	regmap_update_bits(regmap, HP_REG5, 0x7 << HP_GAIN,
			   pdata->hpout_gain << HP_GAIN);

	/* power */
	regmap_update_bits(regmap, POWER_REG2,
			   0x1 << OSC_EN | 0x7 << OSC_CLK_TRIM,
			   0x1 << OSC_EN | 0x4 << OSC_CLK_TRIM);
	regmap_update_bits(regmap, POWER_REG3,
			   0x1 << DLDO_LQ | 0x1f << DLDO_VSEL,
			   0x1 << DLDO_LQ | 0x1c << DLDO_VSEL);
	regmap_update_bits(regmap, POWER_REG4,
			   0x1 << ALDO_EN | 0x7 << ALDO_VSEL | 0x1 << ADDA_BIASEN,
			   0x1 << ALDO_EN | 0x1 << ALDO_VSEL | 0x1 << ADDA_BIASEN);
	regmap_update_bits(regmap, POWER_REG5,
			   0x1 << VRP_EN | 0x7 << VRP_VOLT_SEL,
			   0x1 << VRP_EN | 0x1 << VRP_VOLT_SEL);
	regmap_update_bits(regmap, POWER_REG6, 0x3 << IOPDACS, 0x3 << IOPDACS);

	regmap_update_bits(regmap, POWER_REG1, 0x1 << BG_BUFEN, 0x1 << BG_BUFEN);

	do {
		regmap_read(regmap, POWER_REG4, &ret);
	} while (!(ret & (0x1 << AVCC_POR)));
	regmap_update_bits(regmap, POWER_REG1,
			   0x1 << VRA1_SEPPEDUP, 0x1 << VRA1_SEPPEDUP);

	/* disable hpf */
	regmap_update_bits(regmap, HPF_EN, 0x1 << DIG_ADC1_HPF_EN, 0x0 << DIG_ADC1_HPF_EN);
	regmap_update_bits(regmap, HPF_EN, 0x1 << DIG_ADC2_HPF_EN, 0x0 << DIG_ADC2_HPF_EN);
	regmap_update_bits(regmap, HPF_EN, 0x1 << DIG_ADC3_HPF_EN, 0x0 << DIG_ADC3_HPF_EN);

	/* component kcontrols -> pa */
	ret = snd_sunxi_pa_pin_probe(jack_adv_priv->pa_cfg,
				     jack_adv_priv->pa_pin_max,
				     component);
	if (ret)
		SND_LOG_ERR("snd_sunxi_pa_pin_probe failed\n");

	ret = snd_soc_add_component_controls(component, ac101b_jack_controls,
					     ARRAY_SIZE(ac101b_jack_controls));
	if (ret)
		SND_LOG_ERR("add ac101b_jack_controls failed\n");

	/* jack */
	pdata->jack_adv_priv.regmap = regmap;
	pdata->jack_adv_priv.dev = ac101b->dev;
	sunxi_jack_adv.data = (void *)(&pdata->jack_adv_priv);
	sunxi_jack_adv.dev = ac101b->dev;
	snd_sunxi_jack_init(&sunxi_jack_port);

	return 0;
}

static void ac101b_remove(struct snd_soc_component *component)
{
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;

	mutex_destroy(&pdata->ac101b_sta.dac_chp_mutex);
	mutex_destroy(&pdata->ac101b_sta.spk_chp_mutex);
	mutex_destroy(&pdata->ac101b_sta.mic_mutex);

	snd_sunxi_jack_exit(&sunxi_jack_port);
}

static int ac101b_suspend(struct snd_soc_component *component)
{
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	SND_LOG_DEBUG("\n");

	snd_sunxi_save_reg(regmap, &sunxi_reg_group);
	snd_sunxi_pa_pin_disable(jack_adv_priv->pa_cfg,
				 jack_adv_priv->pa_pin_max);

	snd_sunxi_regulator_disable(ac101b->rglt);

	return 0;
}

static int ac101b_resume(struct snd_soc_component *component)
{
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;
	int ret;

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_regulator_enable(ac101b->rglt);
	if (ret)
		return ret;

	snd_sunxi_pa_pin_disable(jack_adv_priv->pa_cfg,
				 jack_adv_priv->pa_pin_max);

	/* software reset */
	regmap_write(regmap, CHIP_SOFT_RST, 0x34);

	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);

	return 0;
}

static int ac101b_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, ADC1_REG1, 0x1 << ADC1_EN, 0x1 << ADC1_EN);

		msleep(10);

		mutex_lock(&pdata->ac101b_sta.mic_mutex);
		pdata->ac101b_sta.mic1 = true;
		if (!pdata->ac101b_sta.mic2 && !pdata->ac101b_sta.mic3)
			regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC_EN, 0x1 << ADC_EN);
		mutex_unlock(&pdata->ac101b_sta.mic_mutex);
		regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC1_DIG_EN, 0x1 << ADC1_DIG_EN);

		regmap_update_bits(regmap, I2S_TX_CHMP_CTRL1, 0x3 << TX_CH1_MAP, ADC1_OUTPUT << TX_CH1_MAP);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ADC1_REG1, 0x1 << ADC1_EN, 0x0 << ADC1_EN);

		mutex_lock(&pdata->ac101b_sta.mic_mutex);
		pdata->ac101b_sta.mic1 = false;
		if (!pdata->ac101b_sta.mic2 && !pdata->ac101b_sta.mic3) {
			regmap_write(regmap, I2S_TX_CHMP_CTRL1, 0x0);
			regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC_EN, 0x0 << ADC_EN);
		}
		mutex_unlock(&pdata->ac101b_sta.mic_mutex);
		regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC1_DIG_EN, 0x0 << ADC1_DIG_EN);

		break;
	default:
		break;
	}

	return 0;
}

static int ac101b_mic24_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dac(pga) enable */
		regmap_update_bits(regmap, ADC2_REG1, 0x1 << ADC2_EN, 0x1 << ADC2_EN);

		msleep(10);

		mutex_lock(&pdata->ac101b_sta.mic_mutex);
		pdata->ac101b_sta.mic2 = true;
		if (!pdata->ac101b_sta.mic1 && !pdata->ac101b_sta.mic3)
			regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC_EN, 0x1 << ADC_EN);
		mutex_unlock(&pdata->ac101b_sta.mic_mutex);
		regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC2_DIG_EN, 0x1 << ADC2_DIG_EN);

		if (pdata->ac101b_sta.mic1)
			regmap_update_bits(regmap, I2S_TX_CHMP_CTRL1, 0x3 << TX_CH2_MAP, ADC2_OUTPUT << TX_CH2_MAP);
		else
			regmap_update_bits(regmap, I2S_TX_CHMP_CTRL1, 0x3 << TX_CH1_MAP, ADC2_OUTPUT << TX_CH1_MAP);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ADC2_REG1, 0x1 << ADC2_EN, 0x0 << ADC2_EN);

		mutex_lock(&pdata->ac101b_sta.mic_mutex);
		pdata->ac101b_sta.mic2 = false;
		if (!pdata->ac101b_sta.mic1 && !pdata->ac101b_sta.mic3) {
			regmap_write(regmap, I2S_TX_CHMP_CTRL1, 0x0);
			regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC_EN, 0x0 << ADC_EN);
		}
		mutex_unlock(&pdata->ac101b_sta.mic_mutex);
		regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC2_DIG_EN, 0x0 << ADC2_DIG_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101b_mic3_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dac(pga) enable */
		regmap_update_bits(regmap, ADC3_REG1, 0x1 << ADC3_EN, 0x1 << ADC3_EN);

		msleep(10);

		mutex_lock(&pdata->ac101b_sta.mic_mutex);
		pdata->ac101b_sta.mic3 = true;
		if (!pdata->ac101b_sta.mic1 && !pdata->ac101b_sta.mic2)
			regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC_EN, 0x1 << ADC_EN);
		mutex_unlock(&pdata->ac101b_sta.mic_mutex);
		regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC3_DIG_EN, 0x1 << ADC3_DIG_EN);

		if (pdata->ac101b_sta.mic1 && pdata->ac101b_sta.mic2)
			regmap_update_bits(regmap, I2S_TX_CHMP_CTRL1, 0x3 << TX_CH3_MAP, ADC3_OUTPUT << TX_CH3_MAP);
		else if (pdata->ac101b_sta.mic1 || pdata->ac101b_sta.mic2)
			regmap_update_bits(regmap, I2S_TX_CHMP_CTRL1, 0x3 << TX_CH2_MAP, ADC3_OUTPUT << TX_CH2_MAP);
		else
			regmap_update_bits(regmap, I2S_TX_CHMP_CTRL1, 0x3 << TX_CH1_MAP, ADC3_OUTPUT << TX_CH1_MAP);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ADC3_REG1, 0x1 << ADC3_EN, 0x0 << ADC3_EN);

		mutex_lock(&pdata->ac101b_sta.mic_mutex);
		pdata->ac101b_sta.mic3 = false;
		if (!pdata->ac101b_sta.mic1 && !pdata->ac101b_sta.mic2) {
			regmap_write(regmap, I2S_TX_CHMP_CTRL1, 0x0);
			regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC_EN, 0x0 << ADC_EN);
		}
		mutex_unlock(&pdata->ac101b_sta.mic_mutex);
		regmap_update_bits(regmap, ADC_DIG_EN, 0x1 << ADC3_DIG_EN, 0x0 << ADC3_DIG_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101b_lineoutl_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, DAC_REG1, 0x1 << DACL_EN, 0x1 << DACL_EN);
		regmap_update_bits(regmap, DAC_REG3, 0x1 << SPKL_EN, 0x1 << SPKL_EN);

		mutex_lock(&pdata->ac101b_sta.dac_chp_mutex);
		pdata->ac101b_sta.dacl = true;
		if (pdata->ac101b_sta.dacr == false)
			regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DAC_EN, 0x1 << DAC_EN);
		mutex_unlock(&pdata->ac101b_sta.dac_chp_mutex);

		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DACL_DIG_EN, 0x1 << DACL_DIG_EN);

		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* DAC digital and analog part need close after pa closed */
		pdata->ac101b_sta.dacl = false;
		break;
	default:
		break;
	}

	return 0;
}

static int ac101b_lineoutr_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct regmap *regmap = ac101b->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, DAC_REG1, 0x1 << DACR_EN, 0x1 << DACR_EN);
		regmap_update_bits(regmap, DAC_REG3, 0x1 << SPKR_EN, 0x1 << SPKR_EN);

		mutex_lock(&pdata->ac101b_sta.dac_chp_mutex);
		pdata->ac101b_sta.dacr = true;
		if (pdata->ac101b_sta.dacl == false)
			regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DAC_EN, 0x1 << DAC_EN);
		mutex_unlock(&pdata->ac101b_sta.dac_chp_mutex);
		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DACR_DIG_EN, 0x1 << DACR_DIG_EN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		pdata->ac101b_sta.dacr = false;
		break;
	default:
		break;
	}

	return 0;
}

static int ac101b_hpout_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, POWER_REG5, 0x1 << HPLDO_EN, 0x1 << HPLDO_EN);

		regmap_update_bits(regmap, DAC_REG1, 0x1 << DACL_EN, 0x1 << DACL_EN);
		regmap_update_bits(regmap, DAC_REG1, 0x1 << DACR_EN, 0x1 << DACR_EN);

		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DAC_EN, 0x1 << DAC_EN);
		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DACL_DIG_EN, 0x1 << DACL_DIG_EN);
		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DACR_DIG_EN, 0x1 << DACR_DIG_EN);

		regmap_update_bits(regmap, HP_REG1, 0x1 << CP_EN, 0x1 << CP_EN);

		regmap_update_bits(regmap, HP_REG2,
					   0x1 << HP_CHOPPER_EN,
					   0x1 << HP_CHOPPER_EN);
		regmap_update_bits(regmap, HP_REG2,
					   0x1 << HP_CHOPPER_NOL_EN,
					   0x1 << HP_CHOPPER_NOL_EN);
		regmap_update_bits(regmap, HP_REG2,
					   0x1 << HP_CHOPPER_CKSET,
					   0x3 << HP_CHOPPER_CKSET);

		regmap_update_bits(regmap, HP_REG1, 0x1 << HPPA_EN, 0x1 << HPPA_EN);
		regmap_update_bits(regmap, HP_REG1, 0x1 << HPOUT_EN, 0x1 << HPOUT_EN);

		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, HP_REG1, 0x1 << HPOUT_EN, 0x0 << HPOUT_EN);
		regmap_update_bits(regmap, HP_REG1, 0x1 << HPPA_EN, 0x0 << HPPA_EN);
		regmap_update_bits(regmap, HP_REG1, 0x1 << CP_EN, 0x0 << CP_EN);

		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DAC_EN, 0x0 << DAC_EN);
		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DACL_DIG_EN, 0x0 << DACL_DIG_EN);
		regmap_update_bits(regmap, DAC_DIG_EN, 0x1 << DACR_DIG_EN, 0x0 << DACR_DIG_EN);

		regmap_update_bits(regmap, DAC_REG1, 0x1 << DACL_EN, 0x0 << DACL_EN);
		regmap_update_bits(regmap, DAC_REG1, 0x1 << DACR_EN, 0x0 << DACR_EN);

		regmap_update_bits(regmap, POWER_REG5, 0x1 << HPLDO_EN, 0x0 << HPLDO_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101b_spk_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct ac101b_data *pdata = &ac101b->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	SND_LOG_DEBUG("event:%d\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_sunxi_pa_pin_enable(jack_adv_priv->pa_cfg,
					jack_adv_priv->pa_pin_max);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		snd_sunxi_pa_pin_disable(jack_adv_priv->pa_cfg,
					 jack_adv_priv->pa_pin_max);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101b_get_adc1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, ADC1_MUX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_ADC1_MUX) & 0x7;

	return 0;
}

static int ac101b_set_adc1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, ADC1_MUX_CTRL,
				   0x7 << DIG_ADC1_MUX,
				   0x0 << DIG_ADC1_MUX);
		break;
	case 1:
		regmap_update_bits(regmap, ADC1_MUX_CTRL,
				   0x7 << DIG_ADC1_MUX,
				   0x1 << DIG_ADC1_MUX);
		break;
	case 2:
		regmap_update_bits(regmap, ADC1_MUX_CTRL,
				   0x7 << DIG_ADC1_MUX,
				   0x2 << DIG_ADC1_MUX);
		break;
	case 3:
		regmap_update_bits(regmap, ADC1_MUX_CTRL,
				   0x7 << DIG_ADC1_MUX,
				   0x3 << DIG_ADC1_MUX);
		break;
	case 4:
		regmap_update_bits(regmap, ADC1_MUX_CTRL,
				   0x7 << DIG_ADC1_MUX,
				   0x4 << DIG_ADC1_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101b_get_adc2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, ADC2_MUX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_ADC2_MUX) & 0x7;

	return 0;
}

static int ac101b_set_adc2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, ADC2_MUX_CTRL,
				   0x7 << DIG_ADC2_MUX,
				   0x0 << DIG_ADC2_MUX);
		break;
	case 1:
		regmap_update_bits(regmap, ADC2_MUX_CTRL,
				   0x7 << DIG_ADC2_MUX,
				   0x1 << DIG_ADC2_MUX);
		break;
	case 2:
		regmap_update_bits(regmap, ADC2_MUX_CTRL,
				   0x7 << DIG_ADC2_MUX,
				   0x2 << DIG_ADC2_MUX);
		break;
	case 3:
		regmap_update_bits(regmap, ADC2_MUX_CTRL,
				   0x7 << DIG_ADC2_MUX,
				   0x3 << DIG_ADC2_MUX);
		break;
	case 4:
		regmap_update_bits(regmap, ADC2_MUX_CTRL,
				   0x7 << DIG_ADC2_MUX,
				   0x4 << DIG_ADC2_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101b_get_adc3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, ADC3_MUX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_ADC2_MUX) & 0x7;

	return 0;
}

static int ac101b_set_adc3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, ADC3_MUX_CTRL,
				   0x7 << DIG_ADC3_MUX,
				   0x0 << DIG_ADC3_MUX);
		break;
	case 1:
		regmap_update_bits(regmap, ADC3_MUX_CTRL,
				   0x7 << DIG_ADC3_MUX,
				   0x1 << DIG_ADC3_MUX);
		break;
	case 2:
		regmap_update_bits(regmap, ADC3_MUX_CTRL,
				   0x7 << DIG_ADC3_MUX,
				   0x2 << DIG_ADC3_MUX);
		break;
	case 3:
		regmap_update_bits(regmap, ADC3_MUX_CTRL,
				   0x7 << DIG_ADC3_MUX,
				   0x3 << DIG_ADC3_MUX);
		break;
	case 4:
		regmap_update_bits(regmap, ADC3_MUX_CTRL,
				   0x7 << DIG_ADC3_MUX,
				   0x4 << DIG_ADC3_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

//adc input
static int ac101b_get_adc2_input_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, ADC2_REG1, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> ADC2_MIC_MUX) & 0x3;

	return 0;
}

static int ac101b_set_adc2_input_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, ADC2_REG1,
				   0x3 << ADC2_MIC_MUX,
				   0x0 << ADC2_MIC_MUX);
		break;
	case 1:
		regmap_update_bits(regmap, ADC2_REG1,
				   0x3 << ADC2_MIC_MUX,
				   0x1 << ADC2_MIC_MUX);
		break;
	case 2:
		regmap_update_bits(regmap, ADC2_REG1,
				   0x3 << ADC2_MIC_MUX,
				   0x2 << ADC2_MIC_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101b_get_adc3_input_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, ADC3_REG1, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val & (0x1 << ADC3_MIC_MUX));

	return 0;
}

static int ac101b_set_adc3_input_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, ADC3_REG1,
				   0x1 << ADC3_MIC_MUX,
				   0x0 << ADC3_MIC_MUX);
		break;
	case 1:
		regmap_update_bits(regmap, ADC3_REG1,
				   0x1 << ADC3_MIC_MUX,
				   0x1 << ADC3_MIC_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

//rxm src
static int ac101b_get_rxm1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, I2S_RX_MIX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> RX_MIX1) & 0x3;

	return 0;
}

static int ac101b_set_rxm1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];
	switch (val) {
	case 0:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX1,
				   0x0 << RX_MIX1);
		break;
	case 1:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX1,
				   0x1 << RX_MIX1);
		break;
	case 2:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX1,
				   0x2 << RX_MIX1);
		break;
	case 3:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX1,
				   0x3 << RX_MIX1);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101b_get_rxm2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, I2S_RX_MIX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> RX_MIX2) & 0x3;

	return 0;
}

static int ac101b_set_rxm2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX2,
				   0x0 << RX_MIX2);
		break;
	case 1:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX2,
				   0x1 << RX_MIX2);
		break;
	case 2:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX2,
				   0x2 << RX_MIX2);
		break;
	case 3:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX2,
				   0x3 << RX_MIX2);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101b_get_rxm3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, I2S_RX_MIX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> RX_MIX3) & 0x3;

	return 0;
}

static int ac101b_set_rxm3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX3,
				   0x0 << RX_MIX3);
		break;
	case 1:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX3,
				   0x1 << RX_MIX3);
		break;
	case 2:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX3,
				   0x2 << RX_MIX3);
		break;
	case 3:
		regmap_update_bits(regmap, I2S_RX_MIX_CTRL,
				   0x3 << RX_MIX3,
				   0x3 << RX_MIX3);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

//txm src
static int ac101b_get_txm1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, I2S_TX_MIX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> TX_MIX1) & 0x3;

	return 0;
}

static int ac101b_set_txm1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX1,
				   0x0 << TX_MIX1);
		break;
	case 1:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX1,
				   0x1 << TX_MIX1);
		break;
	case 2:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX1,
				   0x2 << TX_MIX1);
		break;
	case 3:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX1,
				   0x3 << TX_MIX1);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101b_get_txm2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, I2S_TX_MIX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> TX_MIX2) & 0x3;

	return 0;
}

static int ac101b_set_txm2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX2,
				   0x0 << TX_MIX2);
		break;
	case 1:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX2,
				   0x1 << TX_MIX2);
		break;
	case 2:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX2,
				   0x2 << TX_MIX2);
		break;
	case 3:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX2,
				   0x3 << TX_MIX2);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101b_get_txm3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, I2S_TX_MIX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> TX_MIX3) & 0x3;

	return 0;
}

static int ac101b_set_txm3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX3,
				   0x0 << TX_MIX3);
		break;
	case 1:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX3,
				   0x1 << TX_MIX3);
		break;
	case 2:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX3,
				   0x2 << TX_MIX3);
		break;
	case 3:
		regmap_update_bits(regmap, I2S_TX_MIX_CTRL,
				   0x3 << TX_MIX3,
				   0x3 << TX_MIX3);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

//dac src
static int ac101b_get_dacl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, DAC_MUX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_DAC1_MUX) & 0x3;

	return 0;
}

static int ac101b_set_dacl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, DAC_MUX_CTRL,
				   0x3 << DIG_DAC1_MUX,
				   0x0 << DIG_DAC1_MUX);
		break;
	case 1:
		regmap_update_bits(regmap, DAC_MUX_CTRL,
				   0x3 << DIG_DAC1_MUX,
				   0x1 << DIG_DAC1_MUX);
		break;
	case 2:
		regmap_update_bits(regmap, DAC_MUX_CTRL,
				   0x3 << DIG_DAC1_MUX,
				   0x2 << DIG_DAC1_MUX);
		break;
	case 3:
		regmap_update_bits(regmap, DAC_MUX_CTRL,
				   0x3 << DIG_DAC1_MUX,
				   0x3 << DIG_DAC1_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101b_get_dacr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int reg_val;

	regmap_read(regmap, DAC_MUX_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_DAC2_MUX) & 0x3;

	return 0;
}

static int ac101b_set_dacr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101b_priv *ac101b = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101b->regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		regmap_update_bits(regmap, DAC_MUX_CTRL,
				   0x3 << DIG_DAC2_MUX,
				   0x0 << DIG_DAC2_MUX);
		break;
	case 1:
		regmap_update_bits(regmap, DAC_MUX_CTRL,
				   0x3 << DIG_DAC2_MUX,
				   0x1 << DIG_DAC2_MUX);
		break;
	case 2:
		regmap_update_bits(regmap, DAC_MUX_CTRL,
				   0x3 << DIG_DAC2_MUX,
				   0x2 << DIG_DAC2_MUX);
		break;
	case 3:
		regmap_update_bits(regmap, DAC_MUX_CTRL,
				   0x3 << DIG_DAC2_MUX,
				   0x3 << DIG_DAC2_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}


//name, min, step, mute
static const DECLARE_TLV_DB_SCALE(adc_dig_vol_tlv, -6400, 50, 1);
static const DECLARE_TLV_DB_SCALE(dac_dig_vol_tlv, -6400, 50, 1);
static const DECLARE_TLV_DB_SCALE(dac_gain_tlv, -4200, 600, 0);
static const DECLARE_TLV_DB_SCALE(adc_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(hp_gain_tlv, -4200, 600, 0);

static const char *sunxi_switch_text[] = {"Off", "On"};

/* adc dig*/
static const char * const adc1_data_src_mux_text[] = {
	"DEBUG_DAT", "MIC1", "DACL_DAT", "DACR_DAT", "RXM1"
};
static const char * const adc2_data_src_mux_text[] = {
	"DEBUG_DAT", "ADC2_PAG_MUX", "DACL_DAT", "DACR_DAT", "RXM1"
};
static const char * const adc3_data_src_mux_text[] = {
	"DEBUG_DAT", "ADC3_PAG_MUX", "DACL_DAT", "DACR_DAT", "RXM1"
};

/* I2S_TX_MIX_CTRL */
static const char * const txm1_data_src_mux_text[] = {
	"ADC1_DATA_MUX", "PLAY1_DAT", "ADC1_PLAY1_DAT", "ADC1_PLAY1_DAT_AVG"
};

static const char * const txm2_data_src_mux_text[] = {
	"ADC2_DATA_MUX", "PLAY2_DAT", "ADC2_PLAY2_DAT", "ADC2_PLAY2_DAT_AVG"
};

static const char * const txm3_data_src_mux_text[] = {
	"ADC3_DATA_MUX", "RXM1", "ADC3_DAT_RXM1", "ADC3_DAT_RXM1_AVG"
};

static const char * const adc2_input_src_mux_text[] = {
	"MIC2", "MIC4", "LINEOUTL"
};

static const char * const adc3_input_src_mux_text[] = {
	"MIC3", "LINEOUTR"
};

static const char * const rxm1_data_src_mux_text[] = {
	"RXL", "RXR", "RXR_RXL", "RXL_RXR_AVG"
};

static const char * const rxm2_data_src_mux_text[] = {
	"RXM1", "ADC1_DAT", "RXM1_ADC1", "RXM1_ADC1_AVG"
};

static const char * const rxm3_data_src_mux_text[] = {
	"RXR", "ADC2_DAT", "RXR_ADC2", "RXR_ADC2_AVG"
};

static const char * const dac1_data_src_mux_text[] = {
	"RXM2", "-6dB_Sine", "-60dB_Sine", "Zero"
};

static const char * const dac2_data_src_mux_text[] = {
	"RXM3", "-6dB_Sine", "-60dB_Sine", "Zero"
};

/* mic4: 0data*/
static SOC_ENUM_SINGLE_DECL(sunxi_mic12_swap_enum, DMIC_EN_CTRL, MIC1_SWP_MIC2, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_mic34_swap_enum, DMIC_EN_CTRL, MIC3_SWP_MIC4, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dac_swap_enum, DAC_DIG_CTRL, DAC_SWP, sunxi_switch_text);

//adc src
static const struct soc_enum adc1_src_mux_enum =
	SOC_ENUM_SINGLE(ADC1_MUX_CTRL, DIG_ADC1_MUX,
			ARRAY_SIZE(adc1_data_src_mux_text),
			adc1_data_src_mux_text);
static const struct soc_enum adc2_src_mux_enum =
	SOC_ENUM_SINGLE(ADC2_MUX_CTRL, DIG_ADC2_MUX,
			ARRAY_SIZE(adc2_data_src_mux_text),
			adc2_data_src_mux_text);
static const struct soc_enum adc3_src_mux_enum =
	SOC_ENUM_SINGLE(ADC3_MUX_CTRL, DIG_ADC3_MUX,
			ARRAY_SIZE(adc3_data_src_mux_text),
			adc3_data_src_mux_text);

//adc input src
static const struct soc_enum adc2_input_mux_enum =
	SOC_ENUM_SINGLE(ADC2_REG1, ADC2_MIC_MUX,
			ARRAY_SIZE(adc2_input_src_mux_text),
			adc2_input_src_mux_text);
static const struct soc_enum adc3_input_mux_enum =
	SOC_ENUM_SINGLE(ADC3_REG1, ADC3_MIC_MUX,
			ARRAY_SIZE(adc3_input_src_mux_text),
			adc3_input_src_mux_text);

//rxm src
static const struct soc_enum rxm1_src_mux_enum =
	SOC_ENUM_SINGLE(I2S_RX_MIX_CTRL, RX_MIX1,
			ARRAY_SIZE(rxm1_data_src_mux_text),
			rxm1_data_src_mux_text);
static const struct soc_enum rxm2_src_mux_enum =
	SOC_ENUM_SINGLE(I2S_RX_MIX_CTRL, RX_MIX2,
			ARRAY_SIZE(rxm2_data_src_mux_text),
			rxm2_data_src_mux_text);
static const struct soc_enum rxm3_src_mux_enum =
	SOC_ENUM_SINGLE(I2S_RX_MIX_CTRL, RX_MIX3,
			ARRAY_SIZE(rxm3_data_src_mux_text),
			rxm3_data_src_mux_text);

//txm src
static const struct soc_enum txm1_src_mux_enum =
	SOC_ENUM_SINGLE(I2S_TX_MIX_CTRL, TX_MIX1,
			ARRAY_SIZE(txm1_data_src_mux_text),
			txm1_data_src_mux_text);
static const struct soc_enum txm2_src_mux_enum =
	SOC_ENUM_SINGLE(I2S_TX_MIX_CTRL, TX_MIX2,
			ARRAY_SIZE(txm2_data_src_mux_text),
			txm2_data_src_mux_text);
static const struct soc_enum txm3_src_mux_enum =
	SOC_ENUM_SINGLE(I2S_TX_MIX_CTRL, TX_MIX3,
			ARRAY_SIZE(txm3_data_src_mux_text),
			txm3_data_src_mux_text);

//dac src
static const struct soc_enum dac1_src_mux_enum =
	SOC_ENUM_SINGLE(DAC_MUX_CTRL, DIG_DAC1_MUX,
			ARRAY_SIZE(dac1_data_src_mux_text),
			dac1_data_src_mux_text);
static const struct soc_enum dac2_src_mux_enum =
	SOC_ENUM_SINGLE(DAC_MUX_CTRL, DIG_DAC2_MUX,
			ARRAY_SIZE(dac2_data_src_mux_text),
			dac2_data_src_mux_text);

static const struct snd_kcontrol_new adc1_src_mux =
	SOC_DAPM_ENUM_EXT("ADC1 DATA MUX", adc1_src_mux_enum,
			  ac101b_get_adc1_src,
			  ac101b_set_adc1_src);
static const struct snd_kcontrol_new adc2_src_mux =
	SOC_DAPM_ENUM_EXT("ADC2 DATA MUX", adc2_src_mux_enum,
			  ac101b_get_adc2_src,
			  ac101b_set_adc2_src);
static const struct snd_kcontrol_new adc3_src_mux =
	SOC_DAPM_ENUM_EXT("ADC3 DATA MUX", adc3_src_mux_enum,
			  ac101b_get_adc3_src,
			  ac101b_set_adc3_src);

static const struct snd_kcontrol_new adc2_input_src_mux =
	SOC_DAPM_ENUM_EXT("ADC2 PAG MUX", adc2_input_mux_enum,
			  ac101b_get_adc2_input_src,
			  ac101b_set_adc2_input_src);
static const struct snd_kcontrol_new adc3_input_src_mux =
	SOC_DAPM_ENUM_EXT("ADC3 PAG MUX", adc3_input_mux_enum,
			  ac101b_get_adc3_input_src,
			  ac101b_set_adc3_input_src);

static const struct snd_kcontrol_new rxm1_src_mux =
	SOC_DAPM_ENUM_EXT("RXM1", rxm1_src_mux_enum,
			  ac101b_get_rxm1_src,
			  ac101b_set_rxm1_src);
static const struct snd_kcontrol_new rxm2_src_mux =
	SOC_DAPM_ENUM_EXT("RXM2", rxm2_src_mux_enum,
			  ac101b_get_rxm2_src,
			  ac101b_set_rxm2_src);
static const struct snd_kcontrol_new rxm3_src_mux =
	SOC_DAPM_ENUM_EXT("RXM3", rxm3_src_mux_enum,
			  ac101b_get_rxm3_src,
			  ac101b_set_rxm3_src);

static const struct snd_kcontrol_new txm1_src_mux =
	SOC_DAPM_ENUM_EXT("ADC1 MUX", txm1_src_mux_enum,
			  ac101b_get_txm1_src,
			  ac101b_set_txm1_src);
static const struct snd_kcontrol_new txm2_src_mux =
	SOC_DAPM_ENUM_EXT("ADC2 MUX", txm2_src_mux_enum,
			  ac101b_get_txm2_src,
			  ac101b_set_txm2_src);
static const struct snd_kcontrol_new txm3_src_mux =
	SOC_DAPM_ENUM_EXT("ADC3 MUX", txm3_src_mux_enum,
			  ac101b_get_txm3_src,
			  ac101b_set_txm3_src);

static const struct snd_kcontrol_new dacl_src_mux =
	SOC_DAPM_ENUM_EXT("DACL MUX", dac1_src_mux_enum,
			  ac101b_get_dacl_src,
			  ac101b_set_dacl_src);
static const struct snd_kcontrol_new dacr_src_mux =
	SOC_DAPM_ENUM_EXT("DACR MUX", dac2_src_mux_enum,
			  ac101b_get_dacr_src,
			  ac101b_set_dacr_src);

static const struct snd_kcontrol_new ac101b_snd_controls[] = {
	// name, reg, shift, max, invert, tlv_array
	/* adc dig vol*/
	SOC_SINGLE_TLV("ADC1 Volume", ADC1_DVOL_CTRL, DIG_ADC1_VOL, 0xff, 0, adc_dig_vol_tlv),
	SOC_SINGLE_TLV("ADC2 Volume", ADC2_DVOL_CTRL, DIG_ADC2_VOL, 0xff, 0, adc_dig_vol_tlv),
	SOC_SINGLE_TLV("ADC3 Volume", ADC3_DVOL_CTRL, DIG_ADC3_VOL, 0xff, 0, adc_dig_vol_tlv),
	/* dac dig vol */
	SOC_SINGLE_TLV("DACL Volume", DAC_DVC_L, DACL_DIG_VOL, 0xff, 0, dac_dig_vol_tlv),
	SOC_SINGLE_TLV("DACR Volume", DAC_DVC_R, DACR_DIG_VOL, 0xff, 0, dac_dig_vol_tlv),

	SOC_SINGLE_TLV("DAC Gain", DAC_DHP_GAIN_CTRL, DHP_OUTPUT_GAIN, 0x7, 1, dac_gain_tlv),

	/* adc gain */
	SOC_SINGLE_TLV("ADC1 Gain", ADC1_REG3, ADC1_PGA_GAIN_CTRL, 0x1f, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("ADC2 Gain", ADC2_REG3, ADC2_PGA_GAIN_CTRL, 0x1f, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("ADC3 Gain", ADC3_REG3, ADC3_PGA_GAIN_CTRL, 0x1f, 0, adc_gain_tlv),

	/*headphone gain*/
	SOC_SINGLE_TLV("HPOUT Gain", HP_REG5, HP_GAIN, 0x7, 1, hp_gain_tlv),

	/* mic swap */
	SOC_ENUM("ADC1 ADC2 Swap", sunxi_mic12_swap_enum),
	SOC_ENUM("ADC3 ADC4 Swap", sunxi_mic34_swap_enum),
	SOC_ENUM("DACL DACR Swap", sunxi_dac_swap_enum),
};

static const struct snd_soc_dapm_widget ac101b_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MIC1P_PIN"),
	SND_SOC_DAPM_INPUT("MIC1N_PIN"),
	SND_SOC_DAPM_INPUT("MIC2P_PIN"),
	SND_SOC_DAPM_INPUT("MIC2N_PIN"),
	SND_SOC_DAPM_INPUT("MIC3P_PIN"),
	SND_SOC_DAPM_INPUT("MIC3N_PIN"),
	SND_SOC_DAPM_INPUT("MIC4P_PIN"),
	SND_SOC_DAPM_INPUT("MIC4N_PIN"),

	// SND_SOC_DAPM_INPUT("DMIC"),
	SND_SOC_DAPM_INPUT("DEBUG_DAT"),

	SND_SOC_DAPM_OUTPUT("LINEOUTLP_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTLN_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTRP_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTRN_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUT_PIN"),

	SND_SOC_DAPM_AIF_IN("RXL", "AIF Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("RXR", "AIF Playback", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_AIF_OUT("TX1", "AIF Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("TX2", "AIF Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("TX3", "AIF Capture", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_ADC("ADC1", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC2", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC3", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_DAC("DACL", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DACR", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_MICBIAS("MICBIAS", MBIAS_REG, MBIAS_EN, 0),
	SND_SOC_DAPM_MICBIAS("MICBIAS CHOP", MBIAS_REG, MBIAS_CHOPPER_EN, 0),

	SND_SOC_DAPM_MUX("ADC2 PAG MUX", SND_SOC_NOPM, 0, 0, &adc2_input_src_mux),
	SND_SOC_DAPM_MUX("ADC3 PAG MUX", SND_SOC_NOPM, 0, 0, &adc3_input_src_mux),

	SND_SOC_DAPM_MUX("ADC1 DATA MUX", SND_SOC_NOPM, 0, 0, &adc1_src_mux),
	SND_SOC_DAPM_MUX("ADC2 DATA MUX", SND_SOC_NOPM, 0, 0, &adc2_src_mux),
	SND_SOC_DAPM_MUX("ADC3 DATA MUX", SND_SOC_NOPM, 0, 0, &adc3_src_mux),

	// //i2s
	SND_SOC_DAPM_MUX("ADC1 MUX", SND_SOC_NOPM, 0, 0, &txm1_src_mux),
	SND_SOC_DAPM_MUX("ADC2 MUX", SND_SOC_NOPM, 0, 0, &txm2_src_mux),
	SND_SOC_DAPM_MUX("ADC3 MUX", SND_SOC_NOPM, 0, 0, &txm3_src_mux),

	SND_SOC_DAPM_MUX("RX1 MUX", SND_SOC_NOPM, 0, 0, &rxm1_src_mux),
	SND_SOC_DAPM_MUX("RX2 MUX", SND_SOC_NOPM, 0, 0, &rxm2_src_mux),
	SND_SOC_DAPM_MUX("RX3 MUX", SND_SOC_NOPM, 0, 0, &rxm3_src_mux),

	SND_SOC_DAPM_MUX("DACL MUX", SND_SOC_NOPM, 0, 0, &dacl_src_mux),
	SND_SOC_DAPM_MUX("DACR MUX", SND_SOC_NOPM, 0, 0, &dacr_src_mux),

	SND_SOC_DAPM_SPK("LINEOUTL", ac101b_lineoutl_event),
	SND_SOC_DAPM_SPK("LINEOUTR", ac101b_lineoutr_event),

	/* for pa */
	SND_SOC_DAPM_SPK("SPK", ac101b_spk_event),

	SND_SOC_DAPM_HP("HPOUT", ac101b_hpout_event),

	SND_SOC_DAPM_MIC("MIC1", ac101b_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", ac101b_mic24_event),
	SND_SOC_DAPM_MIC("MIC3", ac101b_mic3_event),
	SND_SOC_DAPM_MIC("MIC4", ac101b_mic24_event),
};

static const struct snd_soc_dapm_route ac101b_dapm_routes[] = {
	{"MICBIAS", NULL, "MIC1P_PIN"},
	{"MICBIAS", NULL, "MIC1N_PIN"},
	{"MICBIAS", NULL, "MIC2P_PIN"},
	{"MICBIAS", NULL, "MIC2N_PIN"},
	{"MICBIAS", NULL, "MIC3P_PIN"},
	{"MICBIAS", NULL, "MIC3N_PIN"},
	{"MICBIAS", NULL, "MIC4P_PIN"},
	{"MICBIAS", NULL, "MIC4N_PIN"},

	{"MICBIAS CHOP", NULL, "MICBIAS"},

	{"ADC2 PAG MUX", "MIC2", "MICBIAS CHOP"},
	{"ADC2 PAG MUX", "MIC4", "MICBIAS CHOP"},
	{"ADC2 PAG MUX", "LINEOUTL", "LINEOUTLP_PIN"},
	{"ADC2 PAG MUX", "LINEOUTL", "LINEOUTLN_PIN"},

	{"ADC3 PAG MUX", "MIC3", "MICBIAS CHOP"},
	{"ADC3 PAG MUX", "LINEOUTR", "LINEOUTRP_PIN"},
	{"ADC3 PAG MUX", "LINEOUTR", "LINEOUTRN_PIN"},

	{"ADC1 DATA MUX", "DEBUG_DAT", "DEBUG_DAT"},
	{"ADC1 DATA MUX", "MIC1", "MICBIAS CHOP"},
	{"ADC1 DATA MUX", "DACL_DAT", "DACL MUX"},
	{"ADC1 DATA MUX", "DACR_DAT", "DACR MUX"},
	{"ADC1 DATA MUX", "RXM1", "RX1 MUX"},

	{"ADC2 DATA MUX", "DEBUG_DAT", "DEBUG_DAT"},
	{"ADC2 DATA MUX", "ADC2_PAG_MUX", "ADC2 PAG MUX"},
	{"ADC2 DATA MUX", "DACL_DAT", "DACL MUX"},
	{"ADC2 DATA MUX", "DACR_DAT", "DACR MUX"},
	{"ADC2 DATA MUX", "RXM1", "RX1 MUX"},

	{"ADC3 DATA MUX", "DEBUG_DAT", "DEBUG_DAT"},
	{"ADC3 DATA MUX", "ADC3_PAG_MUX", "ADC3 PAG MUX"},
	{"ADC3 DATA MUX", "DACL_DAT", "DACL MUX"},
	{"ADC3 DATA MUX", "DACR_DAT", "DACR MUX"},
	{"ADC3 DATA MUX", "RXM1", "RX1 MUX"},

	{"ADC1 MUX", "ADC1_DATA_MUX", "ADC1 DATA MUX"},
	{"ADC1 MUX", "PLAY1_DAT", "DACL"},
	{"ADC1 MUX", "ADC1_PLAY1_DAT", "ADC1 DATA MUX"},
	{"ADC1 MUX", "ADC1_PLAY1_DAT", "DACL"},
	{"ADC1 MUX", "ADC1_PLAY1_DAT_AVG", "ADC1 DATA MUX"},
	{"ADC1 MUX", "ADC1_PLAY1_DAT_AVG", "DACL"},

	{"ADC2 MUX", "ADC2_DATA_MUX", "ADC2 DATA MUX"},
	{"ADC2 MUX", "PLAY2_DAT", "DACR"},
	{"ADC2 MUX", "ADC2_PLAY2_DAT", "ADC2 DATA MUX"},
	{"ADC2 MUX", "ADC2_PLAY2_DAT", "DACR"},
	{"ADC2 MUX", "ADC2_PLAY2_DAT_AVG", "ADC2 DATA MUX"},
	{"ADC2 MUX", "ADC2_PLAY2_DAT_AVG", "DACR"},

	{"ADC3 MUX", "ADC3_DATA_MUX", "ADC3 DATA MUX"},
	{"ADC3 MUX", "RXM1", "RX1 MUX"},
	{"ADC3 MUX", "ADC3_DAT_RXM1", "ADC3 DATA MUX"},
	{"ADC3 MUX", "ADC3_DAT_RXM1", "RX1 MUX"},
	{"ADC3 MUX", "ADC3_DAT_RXM1_AVG", "ADC3 DATA MUX"},
	{"ADC3 MUX", "ADC3_DAT_RXM1_AVG", "RX1 MUX"},

	{"ADC1", NULL, "ADC1 MUX"},
	{"ADC2", NULL, "ADC2 MUX"},
	{"ADC3", NULL, "ADC3 MUX"},

	{"TX1", NULL, "ADC1"},
	{"TX2", NULL, "ADC2"},
	{"TX3", NULL, "ADC3"},

	/* dac route */
	{"RX1 MUX", "RXL", "RXL"},
	{"RX1 MUX", "RXR", "RXR"},
	{"RX1 MUX", "RXR_RXL", "RXL"},
	{"RX1 MUX", "RXR_RXL", "RXR"},
	{"RX1 MUX", "RXL_RXR_AVG", "RXL"},
	{"RX1 MUX", "RXL_RXR_AVG", "RXR"},

	{"RX2 MUX", "RXM1", "RX1 MUX"},
	{"RX2 MUX", "ADC1_DAT", "ADC1"},
	{"RX2 MUX", "RXM1_ADC1", "RX1 MUX"},
	{"RX2 MUX", "RXM1_ADC1", "ADC1"},
	{"RX2 MUX", "RXM1_ADC1_AVG", "RX1 MUX"},
	{"RX2 MUX", "RXM1_ADC1_AVG", "ADC1"},

	{"RX3 MUX", "RXR", "RXR"},
	{"RX3 MUX", "ADC2_DAT", "ADC2"},
	{"RX3 MUX", "RXR_ADC2", "RXR"},
	{"RX3 MUX", "RXR_ADC2", "ADC2"},
	{"RX3 MUX", "RXR_ADC2_AVG", "RXR"},
	{"RX3 MUX", "RXR_ADC2_AVG", "ADC2"},

	{"DACL MUX", "RXM2", "RX2 MUX"},
	{"DACL MUX", "-6dB_Sine", "DEBUG_DAT"},
	{"DACL MUX", "-60dB_Sine", "DEBUG_DAT"},
	{"DACL MUX", "Zero", "DEBUG_DAT"},

	{"DACR MUX", "RXM3", "RX3 MUX"},
	{"DACR MUX", "-6dB_Sine", "DEBUG_DAT"},
	{"DACR MUX", "-60dB_Sine", "DEBUG_DAT"},
	{"DACR MUX", "Zero", "DEBUG_DAT"},

	{"DACL", NULL, "DACL MUX"},
	{"DACR", NULL, "DACR MUX"},

	{"LINEOUTLP_PIN", NULL, "DACL"},
	{"LINEOUTLN_PIN", NULL, "DACL"},
	{"LINEOUTRP_PIN", NULL, "DACR"},
	{"LINEOUTRN_PIN", NULL, "DACR"},
	{"HPOUT_PIN", NULL, "DACL"},
	{"HPOUT_PIN", NULL, "DACR"},
};


static const struct snd_soc_component_driver soc_component_dev_ac101b = {
	.probe			= ac101b_probe,
	.remove			= ac101b_remove,
	.suspend		= ac101b_suspend,
	.resume			= ac101b_resume,
	.controls		= ac101b_snd_controls,
	.num_controls		= ARRAY_SIZE(ac101b_snd_controls),
	.dapm_widgets		= ac101b_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ac101b_dapm_widgets),
	.dapm_routes		= ac101b_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(ac101b_dapm_routes),
};

static const struct regmap_config ac101b_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = AC101B_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static int ac101b_set_params_from_of(struct i2c_client *i2c, struct ac101b_data *pdata)
{
	const struct device_node *np = i2c->dev.of_node;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;
	int i, ret;
	unsigned int temp_val;
	const char *str;
	struct of_keyval_tavle {
		char *name;
		unsigned int val;
	};

	struct of_keyval_tavle of_pllclk_src_table[] = {
		{ "MCLK",	PLLCLK_SRC_MCLK },
		{ "BCLK",	PLLCLK_SRC_BCLK },
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

	SND_LOG_DEBUG("\n");

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

	ret = of_property_read_u32(np, "adc1_vol", &pdata->adc1_vol);
	if (ret < 0)
		pdata->adc1_vol = 129;
	ret = of_property_read_u32(np, "adc2_vol", &pdata->adc2_vol);
	if (ret < 0)
		pdata->adc2_vol = 129;
	ret = of_property_read_u32(np, "adc3_vol", &pdata->adc3_vol);
	if (ret < 0)
		pdata->adc3_vol = 129;

	ret = of_property_read_u32(np, "dacl_vol", &pdata->dacl_vol);
	if (ret < 0)
		pdata->dacl_vol = 129;
	ret = of_property_read_u32(np, "dacr_vol", &pdata->dacr_vol);
	if (ret < 0)
		pdata->dacr_vol = 129;

	ret = of_property_read_u32(np, "mic1_gain", &pdata->mic1_gain);
	if (ret < 0)
		pdata->mic1_gain = 31;
	ret = of_property_read_u32(np, "mic2_gain", &pdata->mic2_gain);
	if (ret < 0)
		pdata->mic2_gain = 31;
	ret = of_property_read_u32(np, "mic3_gain", &pdata->mic3_gain);
	if (ret < 0)
		pdata->mic3_gain = 31;

	ret = of_property_read_u32(np, "hpout_gain", &temp_val);
	if (ret < 0) {
		pdata->hpout_gain = 0;
	} else {
		pdata->hpout_gain = 7 - temp_val;
	}

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

	jack_adv_priv->irq_gpio = of_get_named_gpio(np, "irq-gpio", 0);
	if (jack_adv_priv->irq_gpio == -EPROBE_DEFER) {
		SND_LOG_ERR("get hp-det-gpio failed\n");
	}

	if (!gpio_is_valid(jack_adv_priv->irq_gpio)) {
		SND_LOG_ERR("jack-detgpio (%d) is invalid\n", jack_adv_priv->irq_gpio);
	}

	ret = of_property_read_u32(np, "jack-det-threshold", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-det-threshold miss, default 1\n");
		jack_adv_priv->det_threshold = 1;
	} else {
		jack_adv_priv->det_threshold = temp_val;
	}

	ret = of_property_read_u32(np, "jack-det-debouce-time", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-det-debouce-time miss, default 15\n");
		jack_adv_priv->det_debounce = 15;
	} else {
		jack_adv_priv->det_debounce = temp_val;
	}

	ret = of_property_read_u32(np, "jack-key-det-threshold", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-threshold miss, default 10\n");
		jack_adv_priv->key_threshold = 10;
	} else {
		jack_adv_priv->key_threshold = temp_val;
	}

	ret = of_property_read_u32(np, "jack-key-det-debouce-time", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-debouce-time miss, default 2\n");
		jack_adv_priv->key_debounce = 2;
	} else {
		jack_adv_priv->key_debounce = temp_val;
	}

	ret = of_property_read_u32_index(np, "jack-key-det-voltage-hook", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-hook get failed\n");
		jack_adv_priv->key_det_vol[0][0] = 23;
	} else {
		jack_adv_priv->key_det_vol[0][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-hook", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-hook get failed\n");
		jack_adv_priv->key_det_vol[0][1] = 24;
	} else {
		jack_adv_priv->key_det_vol[0][1] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-up", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-up get failed\n");
		jack_adv_priv->key_det_vol[1][0] = 19;
	} else {
		jack_adv_priv->key_det_vol[1][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-up", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-up get failed\n");
		jack_adv_priv->key_det_vol[1][1] = 20;
	} else {
		jack_adv_priv->key_det_vol[1][1] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-down", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-down get failed\n");
		jack_adv_priv->key_det_vol[2][0] = 21;
	} else {
		jack_adv_priv->key_det_vol[2][0] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-down", 1, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-down get failed\n");
		jack_adv_priv->key_det_vol[2][1] = 22;
	} else {
		jack_adv_priv->key_det_vol[2][1] = temp_val;
	}
	ret = of_property_read_u32_index(np, "jack-key-det-voltage-voice", 0, &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-voltage-voice get failed\n");
		jack_adv_priv->key_det_vol[3][0] = 1;
	} else {
		jack_adv_priv->key_det_vol[3][0] = temp_val;
	}

	SND_LOG_DEBUG("irq_gpio        -> %u\n", jack_adv_priv->irq_gpio);

	SND_LOG_DEBUG("jack-det-threshold    -> %u\n",
		      jack_adv_priv->det_threshold);
	SND_LOG_DEBUG("jack-key-det-threshold    -> %u\n",
		      jack_adv_priv->key_threshold);
	SND_LOG_DEBUG("jack-det-debouce-time    -> %u\n",
		      jack_adv_priv->det_debounce);
	SND_LOG_DEBUG("jack-key-det-debouce-time -> %u\n",
		      jack_adv_priv->key_debounce);

	SND_LOG_DEBUG("jack-key-det-voltage-hook   -> %u-%u\n",
		      jack_adv_priv->key_det_vol[0][0],
		      jack_adv_priv->key_det_vol[0][1]);
	SND_LOG_DEBUG("jack-key-det-voltage-up     -> %u-%u\n",
		      jack_adv_priv->key_det_vol[1][0],
		      jack_adv_priv->key_det_vol[1][1]);
	SND_LOG_DEBUG("jack-key-det-voltage-down   -> %u-%u\n",
		      jack_adv_priv->key_det_vol[2][0],
		      jack_adv_priv->key_det_vol[2][1]);

	return 0;

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int ac101b_i2c_probe(struct i2c_client *i2c)
#else
static int ac101b_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
#endif
{
	struct ac101b_data *pdata = dev_get_platdata(&i2c->dev);
	struct platform_device *pdev = container_of(&i2c->dev, struct platform_device, dev);
	struct ac101b_priv *ac101b;
	int ret;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	(void)id;
#endif

	SND_LOG_DEBUG("\n");

	ac101b = devm_kzalloc(&i2c->dev, sizeof(*ac101b), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ac101b)) {
		dev_err(&i2c->dev, "Unable to allocate ac101b private data\n");
		return -ENOMEM;
	}

	ac101b->dev = &i2c->dev;

	ac101b->regmap = devm_regmap_init_i2c(i2c, &ac101b_regmap);
	if (IS_ERR_OR_NULL(ac101b->regmap))
		return PTR_ERR(ac101b->regmap);

	if (pdata)
		memcpy(&ac101b->pdata, pdata, sizeof(struct ac101b_data));
	else if (i2c->dev.of_node) {
		ret =  ac101b_set_params_from_of(i2c, &ac101b->pdata);
		if (ret) {
			dev_err(&i2c->dev, "ac101b_set_params_from_of failed\n");
			return -1;
		}
	} else
		dev_err(&i2c->dev, "Unable to allocate ac101b private data\n");

	ac101b->rglt = snd_sunxi_regulator_init(&i2c->dev);

	ac101b->pdata.jack_adv_priv.pa_cfg = snd_sunxi_pa_pin_init(pdev,
					    &ac101b->pdata.jack_adv_priv.pa_pin_max);

	i2c_set_clientdata(i2c, ac101b);

	ret = devm_snd_soc_register_component(&i2c->dev,
					      &soc_component_dev_ac101b,
					      &ac101b_dai, 1);
	if (ret < 0)
		dev_err(&i2c->dev, "register ac101b codec failed: %d\n", ret);

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static void ac101b_i2c_remove(struct i2c_client *i2c)
#else
static int ac101b_i2c_remove(struct i2c_client *i2c)
#endif
{
	struct device *dev = &i2c->dev;
	struct device_node *np = i2c->dev.of_node;
	struct ac101b_priv *ac101b = dev_get_drvdata(dev);

	snd_sunxi_regulator_exit(ac101b->rglt);

	devm_kfree(dev, ac101b);
	of_node_put(np);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	return;
#else
	return 0;
#endif
}

static const struct of_device_id ac101b_of_match[] = {
	{ .compatible = "allwinner,sunxi-ac101b", },
	{ }
};
MODULE_DEVICE_TABLE(of, ac101b_of_match);

static struct i2c_driver ac101b_i2c_driver = {
	.driver = {
		.name = "sunxi-ac101b",
		.of_match_table = ac101b_of_match,
	},
	.probe = ac101b_i2c_probe,
	.remove = ac101b_i2c_remove,
};

module_i2c_driver(ac101b_i2c_driver);

MODULE_DESCRIPTION("ASoC ac101b driver");
MODULE_AUTHOR("lijingpsw@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

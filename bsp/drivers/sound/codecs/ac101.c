// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * ac101.c -- AC101 ALSA SoC Audio driver
 *
 * Copyright (c) 2022 Allwinnertech Ltd.
 */

#define SUNXI_MODNAME		"sound-ac101"
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

#include "ac101.h"

#define HEADPHONE_HEADSET_TH 10

static atomic_t clk_cnt = ATOMIC_INIT(0);
static atomic_t i2s_mod_cnt = ATOMIC_INIT(0);

/*
 * adc analog need time to stable. for multi adc, if adding delay for each adc analog respectively,
 * which will take much time. so adc_a_cnt is used to count the used adc and add delay when last
 * adc analog open.
 */
static atomic_t adc_a_cnt = ATOMIC_INIT(0);

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(ADC_VOL_CTRL),
	REG_LABEL(DAC_VOL_CTRL),
	REG_LABEL(ADC_APC_CTRL),
	REG_LABEL(ADC_SRCBST_CTRL),

	REG_LABEL(HPOUT_CTRL),
	REG_LABEL(SPKOUT_CTRL),
	REG_LABEL(I2S_SDIN_CTRL),
	REG_LABEL(DAC_MXR_SRC),
	REG_LABEL(HPOUT_CTRL),

	REG_LABEL(SPKOUT_CTRL),
	REG_LABEL(I2S_SDOUT_CTRL),
	REG_LABEL(I2S_DIG_MXR_SRC),
	REG_LABEL(I2S_CLK_CTRL),
	REG_LABEL(TDM_EN),

	REG_LABEL(OMIXER_SR),
	REG_LABEL(ADC_SRC),
	REG_LABEL(HPOUT_CTRL),
	REG_LABEL(SPKOUT_CTRL),

	REG_LABEL(HMIC_CTRL2),
	REG_LABEL(HMIC_CTRL1),
	REG_LABEL(ADC_APC_CTRL),
	REG_LABEL(HMIC_CTRL1),

};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

struct ac101_real_to_reg {
	unsigned int real;
	unsigned int reg;
};

struct ac101_pll_div {
	unsigned int freq_in;
	unsigned int freq_out;
	unsigned int m;
	unsigned int n_i;
	unsigned int n_f;
};

static const struct ac101_pll_div ac101_pll_div[] = {
	{128000,   22579200, 1,  529, 1},
	{192000,   22579200, 1,  352, 4},
	{256000,   22579200, 1,  264, 3},
	{384000,   22579200, 1,  176, 2},
	{6000000,  22579200, 38, 429, 0},
	{13000000, 22579200, 19, 99,  0},
	{19200000, 22579200, 25, 88,  1},

	{128000,   24576000, 1,  576, 0},
	{192000,   24576000, 1,  384, 0},
	{256000,   24576000, 1,  288, 0},
	{384000,   24576000, 1,  192, 0},
	{2048000,  24576000, 1,  36,  0},
	{6000000,  24576000, 25, 307, 1},
	{13000000, 24576000, 42, 238, 1},
	{19200000, 24576000, 25, 88,  1},
};

/* I2S_SR_CTRL 0x06 */
static const struct ac101_real_to_reg ac101_sample_rate[] = {
	{8000,   0},
	{11025,  1},
	{12000,  2},
	{16000,  3},
	{22050,  4},
	{24000,  5},
	{32000,  6},
	{44100,  7},
	{48000,  8},
	{96000,  9},
	{192000, 10},
};

/* I2S1LCK_CTRL 0x10 iI2S_WORD_SIZ*/
static const struct ac101_real_to_reg ac101_sample_bit[] = {
	{8,  0},
	{16, 1},
	{20, 2},
	{24, 3},
};

/* I2S1_SDOUT_CTRL 0x11 I2S1_SLOT_SIZE */
static const struct ac101_real_to_reg ac101_slot_width[] = {
	{8,  0},
	{16, 1},
	{32, 2},
};

/* I2S1CLK_CTRL 0x10 */
static const struct ac101_real_to_reg ac101_bclk_div[] = {
	{1, 0},
	{2, 1},
	{4, 2},
	{6, 3},
	{8, 4},
	{12, 5},
	{16, 6},
	{24, 7},
	{32, 8},
	{48, 9},
	{64, 10},
	{96, 11},
	{128, 12},
	{192, 13},
};

static const struct ac101_real_to_reg ac101_lrck_div[] = {
	{16, 0},
	{32, 1},
	{64, 2},
	{128, 3},
	{256, 4},
};

struct ac101_priv {
	struct regmap *regmap;
	struct device *dev;
	unsigned int sysclk_freq;
	unsigned int fmt;
	int slots;
	int slot_width;

	struct ac101_data pdata;
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
static void sunxi_jack_adv_det_pre_scan_work(void *data, enum snd_jack_types *jack_type);
static void sunxi_jack_adv_det_scan_work(void *data, enum snd_jack_types *jack_type);

/* for jack slow plug in */
static void sunxi_jack_sdbp_scan_work(void *data, enum snd_jack_types *jack_type);

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
	.jack_det_pre_scan_work	= sunxi_jack_adv_det_pre_scan_work,
	.jack_det_scan_work	= sunxi_jack_adv_det_scan_work,

	.jack_sdbp.jack_sdbp_irq_init	= NULL,
	.jack_sdbp.jack_sdbp_irq_exit	= NULL,
	.jack_sdbp.jack_sdbp_irq_clean	= NULL,
	.jack_sdbp.jack_sdbp_irq_work	= NULL,

	.jack_sdbp.jack_sdbp_scan_init	= NULL,
	.jack_sdbp.jack_sdbp_scan_exit	= NULL,
	.jack_sdbp.jack_sdbp_scan_work	= sunxi_jack_sdbp_scan_work,

	.jack_sdbp.is_working = false,
};

static int sunxi_jack_adv_init(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;

	regmap_update_bits(regmap, HMIC_CTRL2, 0xffff << 0, 0x0 << 0);

	regmap_update_bits(regmap, HMIC_CTRL2, 0x1f << HMIC_TH1,
			   jack_adv_priv->det_threshold << HMIC_TH1);

	regmap_update_bits(regmap, HMIC_CTRL2, 0x1f << HMIC_TH2,
			   jack_adv_priv->key_threshold << HMIC_TH2);

	regmap_update_bits(regmap, HMIC_CTRL1, 0xf << HMIC_N,
			   jack_adv_priv->det_debounce << HMIC_N);
	regmap_update_bits(regmap, HMIC_CTRL1, 0xf << HMIC_M,
			   jack_adv_priv->key_debounce << HMIC_M);

	if (of_property_read_bool(jack_adv_priv->dev->of_node, "extcon")) {
		jack_adv_priv->typec = true;
	}

	SND_LOG_DEBUG("\n");

	jack_adv_priv->irq_sta = JACK_IRQ_NULL;

	return 0;
}

static void sunxi_jack_adv_exit(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;

	SND_LOG_DEBUG("\n");

	regmap_update_bits(regmap, ADC_APC_CTRL,
			   0x1 << HBIAS_EN, 0x0 << HBIAS_EN);
	regmap_update_bits(regmap, ADC_APC_CTRL,
			   0x1 << HBIAS_ADC_EN, 0x0 << HBIAS_ADC_EN);

	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << PULLOUT_IRQ_EN, 0x0 << PULLOUT_IRQ_EN);
	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << PULLIN_IRQ_EN, 0x0 << PULLIN_IRQ_EN);
	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << KEYUP_IRQ_EN, 0x0 << KEYUP_IRQ_EN);
	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << KEYDOWN_IRQ_EN, 0x0 << KEYDOWN_IRQ_EN);
}

static int sunxi_jack_adv_suspend(void *data)
{
	return 0;
}

static int sunxi_jack_adv_resume(void *data)
{
	return 0;
}

static int sunxi_jack_adv_irq_request(void *data, jack_irq_work jack_interrupt)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	int ret;

	SND_LOG_DEBUG("\n");

	/* irq_gpio irq */
	ret = gpio_request_one(jack_adv_priv->irq_gpio, GPIOF_IN, "Headphone detection");
	if (ret) {
		SND_LOG_ERR("jack-detgpio (%d) request failed, err:%d\n", jack_adv_priv->irq_gpio, ret);
		return ret;
	}

	jack_adv_priv->irq_desc = gpio_to_desc(jack_adv_priv->irq_gpio);
	ret = request_irq(gpiod_to_irq(jack_adv_priv->irq_desc),
				      (void *)jack_interrupt,
				      IRQF_TRIGGER_FALLING,
				      "Headphone detection",
				      jack_adv_priv);
	if (ret) {
		SND_LOG_ERR("jack-detgpio (%d) request irq failed\n", jack_adv_priv->irq_gpio);
		return ret;
	}

	/* det_gpio irq for 3.5mm */
	if (jack_adv_priv->det_gpio) {
		ret = gpio_request_one(jack_adv_priv->det_gpio, GPIOF_IN, "Headphone plug");
		if (ret) {
			SND_LOG_ERR("jack-detgpio (%d) request failed, err:%d\n", jack_adv_priv->det_gpio, ret);
			return ret;
		}

		jack_adv_priv->det_desc = gpio_to_desc(jack_adv_priv->det_gpio);
		ret = request_irq(gpiod_to_irq(jack_adv_priv->det_desc),
					(void *)jack_interrupt,
					IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
					"Headphone plug",
					jack_adv_priv);
		if (ret) {
			SND_LOG_ERR("plug-gpio (%d) request irq failed\n", jack_adv_priv->det_gpio);
			return ret;
		}
	}

	return ret;
}

static void sunxi_jack_adv_irq_free(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;

	SND_LOG_DEBUG("\n");

	/* irq_gpio */
	gpio_free(jack_adv_priv->irq_gpio);
	gpiod_unexport(jack_adv_priv->irq_desc);
	free_irq(gpiod_to_irq(jack_adv_priv->irq_desc), jack_adv_priv);
	gpiod_put(jack_adv_priv->irq_desc);

	/* det_gpio */
	if (jack_adv_priv->det_gpio) {
		gpio_free(jack_adv_priv->det_gpio);
		gpiod_unexport(jack_adv_priv->det_desc);
		free_irq(gpiod_to_irq(jack_adv_priv->det_desc), jack_adv_priv);
		gpiod_put(jack_adv_priv->det_desc);
	}
}

static void sunxi_jack_adv_irq_clean(void *data, int irq)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;

	jack_adv_priv->irq = irq;

	return;
}

static void sunxi_jack_adv_irq_enable(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;

	SND_LOG_DEBUG("\n");

	enable_irq(gpiod_to_irq(jack_adv_priv->irq_desc));

	if (jack_adv_priv->det_gpio)
		enable_irq(gpiod_to_irq(jack_adv_priv->det_desc));
}

static void sunxi_jack_adv_irq_disable(void *data)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;

	SND_LOG_DEBUG("\n");

	disable_irq(gpiod_to_irq(jack_adv_priv->irq_desc));

	if (jack_adv_priv->det_gpio)
		disable_irq(gpiod_to_irq(jack_adv_priv->det_desc));
}


static void sunxi_adv_headset_heasphone_det(struct sunxi_jack_adv_priv *jack_adv_priv,
					    enum snd_jack_types *jack_type)
{
	struct regmap *regmap = jack_adv_priv->regmap;
	unsigned int reg_val;
	unsigned int hdata_old, hdata_new;
	unsigned int i;
	unsigned int gpio_level;
	int count = 5;
	int interval_ms = 100;

	SND_LOG_DEBUG("\n");

	regmap_read(regmap, HMIC_STS, &reg_val);
	hdata_old = (reg_val >> HMIC_DATA) & 0x1f;

	for (i = 0; i < count; i++) {
		msleep(interval_ms);
		regmap_read(regmap, HMIC_STS, &reg_val);
		hdata_new = (reg_val >> HMIC_DATA) & 0x1f;

		if (hdata_new == hdata_old && 2 < hdata_new
		    && hdata_new < HEADPHONE_HEADSET_TH) {
			SND_LOG_INFO("\033[31m h_data:%d \033[0m\n", hdata_new);
			goto headset;
		}

		hdata_old = hdata_new;
	}

	SND_LOG_INFO("\033[31m h_data:%d\033[0m\n", hdata_new);

	/* fake headset */
	gpio_level = gpio_get_value(jack_adv_priv->det_gpio);
	if ((hdata_new == 0) && (gpio_level != jack_adv_priv->det_gpio_level)) {
		goto headset;
	}

	*jack_type = SND_JACK_HEADPHONE;

	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << KEYUP_IRQ_EN, 0x0 << KEYUP_IRQ_EN);
	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << KEYDOWN_IRQ_EN, 0x0 << KEYDOWN_IRQ_EN);
	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << DATA_IRQ_EN, 0x0 << DATA_IRQ_EN);

	return;

headset:
	*jack_type = SND_JACK_HEADSET;

	regmap_update_bits(regmap, HMIC_CTRL2, 0x1f << HMIC_TH2,
				jack_adv_priv->key_threshold << HMIC_TH2);

	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << KEYDOWN_IRQ_EN, 0x1 << KEYDOWN_IRQ_EN);
	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << DATA_IRQ_EN, 0x0 << DATA_IRQ_EN);
	regmap_update_bits(regmap, HMIC_CTRL1,
			   0x1 << KEYUP_IRQ_EN, 0x0 << KEYUP_IRQ_EN);
	return;
}

static void sunxi_jack_adv_det_irq_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;
	int codec_irq = gpiod_to_irq(jack_adv_priv->irq_desc);
	int det_irq = gpiod_to_irq(jack_adv_priv->det_desc);
	unsigned int reg_val, irqen_val, reg_val_tmp;
	unsigned int gpio_level;

	if (jack_adv_priv->det_gpio) {
		gpio_level = gpio_get_value(jack_adv_priv->det_gpio);
		if (gpio_level == jack_adv_priv->det_gpio_level) {
			regmap_read(regmap, HMIC_STS, &reg_val_tmp);
			reg_val_tmp |= 0x1 << KEYDOWN_PEND;
			reg_val_tmp |= 0x1 << KEYUP_PEND;
			reg_val_tmp |= 0x1 << PLUGIN_PEND;
			reg_val_tmp |= 0x1 << PLUGOUT_PEND;
			reg_val_tmp |= 0x1 << DATA_PEND;
			regmap_write(regmap, HMIC_STS, reg_val_tmp);
			jack_adv_priv->irq_sta = JACK_IRQ_OUT;

			goto jack_irq_sts;
		}
	}

	if (jack_adv_priv->irq == codec_irq) {
		regmap_read(regmap, HMIC_CTRL1, &irqen_val);
		regmap_read(regmap, HMIC_STS, &reg_val);
		if ((reg_val & (1 << KEYDOWN_PEND)) && (irqen_val & (1 << KEYDOWN_IRQ_EN))) {
			regmap_update_bits(regmap, HMIC_CTRL1,
					0x1 << KEYDOWN_IRQ_EN, 0x0 << KEYDOWN_IRQ_EN);
			regmap_read(regmap, HMIC_STS, &reg_val_tmp);
			reg_val_tmp |= 0x1 << KEYDOWN_PEND;
			reg_val_tmp |= 0x1 << PLUGIN_PEND;
			reg_val_tmp |= 0x1 << KEYUP_PEND;
			reg_val_tmp |= 0x1 << PLUGOUT_PEND;
			reg_val_tmp |= 0x1 << DATA_PEND;
			regmap_write(regmap, HMIC_STS, reg_val_tmp);
			jack_adv_priv->irq_sta = JACK_IRQ_KEYDOWN;
		} else {
			return;
		}
	} else if (jack_adv_priv->irq == det_irq) {
		gpio_level = gpio_get_value(jack_adv_priv->det_gpio);
		if (gpio_level == jack_adv_priv->det_gpio_level) {
			regmap_read(regmap, HMIC_STS, &reg_val_tmp);
			reg_val_tmp |= 0x1 << KEYDOWN_PEND;
			reg_val_tmp |= 0x1 << KEYUP_PEND;
			reg_val_tmp |= 0x1 << PLUGIN_PEND;
			reg_val_tmp |= 0x1 << PLUGOUT_PEND;
			reg_val_tmp |= 0x1 << DATA_PEND;
			regmap_write(regmap, HMIC_STS, reg_val_tmp);

			jack_adv_priv->irq_sta = JACK_IRQ_OUT;
		} else {
			regmap_update_bits(regmap, ADC_APC_CTRL,
					0x1 << HBIASMOD, 0x1 << HBIASMOD);
			regmap_update_bits(regmap, ADC_APC_CTRL,
					0x1 << HBIAS_EN, 0x1 << HBIAS_EN);
			regmap_update_bits(regmap, ADC_APC_CTRL,
					0x1 << HBIAS_ADC_EN, 0x1 << HBIAS_ADC_EN);

			regmap_read(regmap, HMIC_STS, &reg_val_tmp);
			reg_val_tmp |= 0x1 << PLUGIN_PEND;
			reg_val_tmp |= 0x1 << PLUGOUT_PEND;
			reg_val_tmp |= 0x1 << KEYUP_PEND;
			reg_val_tmp |= 0x1 << KEYDOWN_PEND;
			reg_val_tmp |= 0x1 << DATA_PEND;
			regmap_write(regmap, HMIC_STS, reg_val_tmp);

			jack_adv_priv->irq_sta = JACK_IRQ_IN;
		}
	} else {
		SND_LOG_ERR("err irq\n");
		return;
	}

jack_irq_sts:
	switch (jack_adv_priv->irq_sta) {
	case JACK_IRQ_OUT:
		SND_LOG_INFO("jack out\n");
		regmap_update_bits(regmap, HMIC_CTRL1,
				   0x1 << KEYUP_IRQ_EN, 0x0 << KEYUP_IRQ_EN);
		regmap_update_bits(regmap, HMIC_CTRL1,
				   0x1 << KEYDOWN_IRQ_EN, 0x0 << KEYDOWN_IRQ_EN);

		regmap_update_bits(regmap, ADC_APC_CTRL,
				0x1 << HBIASMOD, 0x0 << HBIASMOD);
		regmap_update_bits(regmap, ADC_APC_CTRL,
				0x1 << HBIAS_EN, 0x0 << HBIAS_EN);
		regmap_update_bits(regmap, ADC_APC_CTRL,
				0x1 << HBIAS_ADC_EN, 0x0 << HBIAS_ADC_EN);

		*jack_type = 0;
	break;

	case JACK_IRQ_IN:
		SND_LOG_INFO("jack in\n");

		msleep(500);
		sunxi_adv_headset_heasphone_det(jack_adv_priv, jack_type);
	break;
	case JACK_IRQ_KEYDOWN:
		regmap_read(regmap, HMIC_STS, &reg_val);
		reg_val = (reg_val >> HMIC_DATA) & 0x1f;
		if (reg_val < jack_adv_priv->key_threshold) {
			regmap_update_bits(regmap, HMIC_CTRL1,
					   0x1 << KEYDOWN_IRQ_EN, 0x1 << KEYDOWN_IRQ_EN);
			return;
		}

		SND_LOG_INFO("jack button\n");

		regmap_read(regmap, HMIC_STS, &reg_val);
		reg_val = (reg_val >> HMIC_DATA) & 0x1f;
		SND_LOG_INFO("\033[31m HMIC_DATA:%u, vol+:[%d, %d], vol-:[%d %d], hook:[%d %d]\033[0m \n",
			     reg_val,
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

		regmap_update_bits(regmap, HMIC_CTRL1,
				   0x1 << KEYDOWN_IRQ_EN, 0x1 << KEYDOWN_IRQ_EN);
	break;
	default:
		SND_LOG_DEBUG("irq status is invaild\n");
	break;
	}

	jack_adv_priv->jack_type = *jack_type;

	return;
}

static void sunxi_jack_adv_det_pre_scan_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_adv *jack_adv = &sunxi_jack_adv;
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;
	unsigned int gpio_level;

	SND_LOG_DEBUG("\n");

	if (jack_adv_priv->typec) {
		if (!jack_adv->jack_plug_sta) {
			*jack_type = 0;
			jack_adv_priv->jack_type = *jack_type;

			regmap_update_bits(regmap, HMIC_CTRL1,
					0x1 << KEYUP_IRQ_EN, 0x0 << KEYUP_IRQ_EN);
			regmap_update_bits(regmap, HMIC_CTRL1,
					0x1 << KEYDOWN_IRQ_EN, 0x0 << KEYDOWN_IRQ_EN);
			regmap_update_bits(regmap, HMIC_CTRL1,
					0x1 << DATA_IRQ_EN, 0x0 << DATA_IRQ_EN);

			regmap_update_bits(regmap, ADC_APC_CTRL,
						0x1 << HBIASMOD, 0x0 << HBIASMOD);
			regmap_update_bits(regmap, ADC_APC_CTRL,
						0x1 << HBIAS_EN, 0x0 << HBIAS_EN);
			regmap_update_bits(regmap, ADC_APC_CTRL,
						0x1 << HBIAS_ADC_EN, 0x0 << HBIAS_ADC_EN);

			return;
		}
	}

	if (jack_adv_priv->det_gpio) {
		gpio_level = gpio_get_value(jack_adv_priv->det_gpio);
		if (gpio_level == jack_adv_priv->det_gpio_level) {
			*jack_type = 0;
			jack_adv_priv->jack_type = *jack_type;
		}
	}

	*jack_type = SND_JACK_HEADPHONE;
}

static void sunxi_jack_adv_det_scan_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_adv *jack_adv = &sunxi_jack_adv;
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;
	unsigned int reg_val;
	unsigned int headset_basedata_i, headset_basedata_n;
	unsigned int gpio_level;

	SND_LOG_INFO("\n");

	if (jack_adv_priv->typec) {
		if (!jack_adv->jack_plug_sta) {
			*jack_type = 0;
			goto out;
		}

		regmap_update_bits(regmap, ADC_APC_CTRL,
				0x1 << HBIASMOD, 0x1 << HBIASMOD);
		regmap_update_bits(regmap, ADC_APC_CTRL,
				0x1 << HBIAS_EN, 0x1 << HBIAS_EN);
		regmap_update_bits(regmap, ADC_APC_CTRL,
				0x1 << HBIAS_ADC_EN, 0x1 << HBIAS_ADC_EN);

		sunxi_jack_typec_mode_set(&jack_adv->jack_typec_cfg, SND_JACK_MODE_MICI);

		msleep(500);
		regmap_read(regmap, HMIC_STS, &reg_val);
		headset_basedata_i = (reg_val >> HMIC_DATA) & 0x1f;
		if (headset_basedata_i > 0
			&& headset_basedata_i < HEADPHONE_HEADSET_TH) {
			*jack_type = SND_JACK_HEADSET;
			SND_LOG_INFO("[%s %d] headset_basedata_i:%d\n",
				     __func__, __LINE__, headset_basedata_i);
			goto headset;
		}

		SND_LOG_INFO("[%s %d] headset_basedata_i:%d\n", __func__, __LINE__, headset_basedata_i);

		sunxi_jack_typec_mode_set(&jack_adv->jack_typec_cfg, SND_JACK_MODE_MICN);
		msleep(500);
		regmap_read(regmap, HMIC_STS, &reg_val);
		headset_basedata_n = (reg_val >> HMIC_DATA) & 0x1f;
		if (headset_basedata_n > 0
			&& headset_basedata_n < HEADPHONE_HEADSET_TH) {
			*jack_type = SND_JACK_HEADSET;
			SND_LOG_INFO("[%s %d] headset_basedata_n:%d\n",
				     __func__, __LINE__, headset_basedata_n);
			goto headset;
		}

		SND_LOG_INFO("[%s %d] headset_basedata_n:%d\n",
			     __func__, __LINE__, headset_basedata_n);

		/* abnormal jack */
		if (!headset_basedata_i && headset_basedata_n) {
			sunxi_jack_typec_mode_set(&jack_adv->jack_typec_cfg, SND_JACK_MODE_MICI);
			*jack_type = SND_JACK_HEADSET;
			goto headset;
		}

		if (headset_basedata_i && !headset_basedata_n) {
			sunxi_jack_typec_mode_set(&jack_adv->jack_typec_cfg, SND_JACK_MODE_MICN);
			*jack_type = SND_JACK_HEADSET;
			goto headset;
		}

		if (!headset_basedata_i && !headset_basedata_n) {
			SND_LOG_ERR("switch may not work\n");
			*jack_type = SND_JACK_HEADSET;
			goto headset;
		}


		*jack_type = SND_JACK_HEADPHONE;
		jack_adv_priv->jack_type = *jack_type;

		return;
	}

	if (jack_adv_priv->det_gpio) {
		gpio_level = gpio_get_value(jack_adv_priv->det_gpio);
		if (gpio_level == jack_adv_priv->det_gpio_level) {
			*jack_type = 0;
			jack_adv_priv->jack_type = *jack_type;
		} else {
			regmap_update_bits(regmap, ADC_APC_CTRL,
					   0x1 << HBIASMOD, 0x1 << HBIASMOD);
			regmap_update_bits(regmap, ADC_APC_CTRL,
					   0x1 << HBIAS_EN, 0x1 << HBIAS_EN);
			regmap_update_bits(regmap, ADC_APC_CTRL,
					   0x1 << HBIAS_ADC_EN, 0x1 << HBIAS_ADC_EN);

			msleep(500);
			sunxi_adv_headset_heasphone_det(jack_adv_priv, jack_type);
		}
		return;
	}

headset:
	if (*jack_type == SND_JACK_HEADSET) {
		msleep(500);
		regmap_update_bits(regmap, HMIC_CTRL2, 0x1f << HMIC_TH2,
				   jack_adv_priv->key_threshold << HMIC_TH2);

		regmap_read(regmap, HMIC_STS, &reg_val);
		reg_val |= 0x1 << PLUGIN_PEND;
		reg_val |= 0x1 << PLUGOUT_PEND;
		reg_val |= 0x1 << KEYUP_PEND;
		reg_val |= 0x1 << KEYDOWN_PEND;
		reg_val |= 0x1 << DATA_PEND;
		regmap_write(regmap, HMIC_STS, reg_val);

		regmap_update_bits(regmap, HMIC_CTRL1,
				0x1 << KEYDOWN_IRQ_EN, 0x1 << KEYDOWN_IRQ_EN);
		regmap_update_bits(regmap, HMIC_CTRL1,
				0x1 << DATA_IRQ_EN, 0x0 << DATA_IRQ_EN);
		regmap_update_bits(regmap, HMIC_CTRL1,
				0x1 << KEYUP_IRQ_EN, 0x0 << KEYUP_IRQ_EN);
	}

out:
	if (*jack_type == 0) {
		regmap_update_bits(regmap, HMIC_CTRL1,
				   0x1 << KEYUP_IRQ_EN, 0x0 << KEYUP_IRQ_EN);
		regmap_update_bits(regmap, HMIC_CTRL1,
				   0x1 << KEYDOWN_IRQ_EN, 0x0 << KEYDOWN_IRQ_EN);
		regmap_update_bits(regmap, HMIC_CTRL1,
				   0x1 << DATA_IRQ_EN, 0x0 << DATA_IRQ_EN);

		regmap_update_bits(regmap, ADC_APC_CTRL,
					0x1 << HBIASMOD, 0x0 << HBIASMOD);
		regmap_update_bits(regmap, ADC_APC_CTRL,
					0x1 << HBIAS_EN, 0x0 << HBIAS_EN);
		regmap_update_bits(regmap, ADC_APC_CTRL,
					0x1 << HBIAS_ADC_EN, 0x0 << HBIAS_ADC_EN);
	}

	jack_adv_priv->jack_type = *jack_type;

	return;
}

static void sunxi_jack_sdbp_scan_work(void *data, enum snd_jack_types *jack_type)
{
	struct sunxi_jack_adv_priv *jack_adv_priv = data;
	struct regmap *regmap = jack_adv_priv->regmap;

	SND_LOG_DEBUG("jack_codec_priv->jack_type:%d\n", jack_adv_priv->jack_type);

	if (jack_adv_priv->jack_type != SND_JACK_HEADPHONE)
		return;

	regmap_update_bits(regmap, ADC_APC_CTRL,
			   0x1 << HBIASMOD, 0x1 << HBIASMOD);
	regmap_update_bits(regmap, ADC_APC_CTRL,
			   0x1 << HBIAS_EN, 0x1 << HBIAS_EN);
	regmap_update_bits(regmap, ADC_APC_CTRL,
			   0x1 << HBIAS_ADC_EN, 0x1 << HBIAS_ADC_EN);

	sunxi_adv_headset_heasphone_det(jack_adv_priv, jack_type);

	if (*jack_type != SND_JACK_HEADSET) {
		regmap_update_bits(regmap, ADC_APC_CTRL,
				   0x1 << HBIASMOD, 0x0 << HBIASMOD);
		regmap_update_bits(regmap, ADC_APC_CTRL,
				   0x1 << HBIAS_EN, 0x0 << HBIAS_EN);
		regmap_update_bits(regmap, ADC_APC_CTRL,
				   0x1 << HBIAS_ADC_EN, 0x0 << HBIAS_ADC_EN);
	}
}

struct sunxi_jack_port sunxi_jack_port = {
	.jack_adv = &sunxi_jack_adv,
};

static int ac101_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	struct ac101_data *pdata = &ac101->pdata;
	unsigned int reg_val;

	SND_LOG_DEBUG("\n");

	if (!atomic_read(&pdata->working)) {
		/* save reg */
		snd_sunxi_save_reg(regmap, &sunxi_reg_group);

		/* software rst */
		regmap_write(regmap, CHIP_SOFT_RST, 0x123);

		do {
			regmap_read(regmap, CHIP_SOFT_RST, &reg_val);
			SND_LOG_INFO("wait ac101 reset successfully, need 0x101/0x%x\n", reg_val);
		} while (reg_val != 0x101);

		/* recover reg */
		snd_sunxi_echo_reg(regmap, &sunxi_reg_group);
	}

	atomic_add(1, &pdata->working);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, MOD_CLK_ENA, 0x1 << DAC_DIG_MOD, 0x1 << DAC_DIG_MOD);
		regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << DAC_DIG_MOD_RST, 0x1 << DAC_DIG_MOD_RST);

	} else {
		regmap_update_bits(regmap, MOD_CLK_ENA, 0x1 << ADC_DIG_MOD, 0x1 << ADC_DIG_MOD);
		regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << ADC_DIG_MOD_RST, 0x1 << ADC_DIG_MOD_RST);
	}

	if (!atomic_read(&i2s_mod_cnt)) {
		/* module enable */
		regmap_update_bits(regmap, MOD_CLK_ENA, 0x1 << I2S_MOD, 0x1 << I2S_MOD);
		/* moduel rst */
		regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << I2S_MOD_RST, 0x1 << I2S_MOD_RST);
	}

	atomic_add(1, &i2s_mod_cnt);

	return 0;
}

static void ac101_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	struct ac101_data *pdata = &ac101->pdata;

	SND_LOG_DEBUG("\n");

	atomic_sub(1, &i2s_mod_cnt);
	atomic_sub(1, &pdata->working);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << DAC_DIG_MOD_RST, 0x0 << DAC_DIG_MOD_RST);
		regmap_update_bits(regmap, MOD_CLK_ENA, 0x1 << DAC_DIG_MOD, 0x0 << DAC_DIG_MOD);
	} else {
		regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << ADC_DIG_MOD_RST, 0x0 << ADC_DIG_MOD_RST);
		regmap_update_bits(regmap, MOD_CLK_ENA, 0x1 << ADC_DIG_MOD, 0x0 << ADC_DIG_MOD);
	}

	if (!atomic_read(&i2s_mod_cnt)) {
		regmap_update_bits(regmap, MOD_RST_CTRL, 0x1 << I2S_MOD_RST, 0x0 << I2S_MOD_RST);
		regmap_update_bits(regmap, MOD_CLK_ENA, 0x1 << I2S_MOD, 0x0 << I2S_MOD);
	}
}

static int ac101_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct regmap *regmap = ac101->regmap;
	int i;
	unsigned int sample_bit;
	unsigned int sample_rate;
	unsigned int channels;
	unsigned int bclk_ratio;
	unsigned int lrck_ratio;
	unsigned int sample_bit_reg = 0;
	unsigned int sample_rate_reg = 0;
	unsigned int bclk_ratio_reg = 0;
	unsigned int lrck_ratio_reg = 0;

	SND_LOG_DEBUG("\n");

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
	default:
		dev_err(dai->dev, "ac101 unsupport the sample bit\n");
		return -EINVAL;
	}
	for (i = 0; i < ARRAY_SIZE(ac101_sample_bit); i++) {
		if (ac101_sample_bit[i].real == sample_bit) {
			sample_bit_reg = ac101_sample_bit[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101_sample_bit)) {
		dev_err(dai->dev, "ac101 unsupport the sample bit config: %u\n", sample_bit);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_CLK_CTRL, 0x3 << WORD_SIZE, sample_bit_reg << WORD_SIZE);

	/* set sample rate */
	sample_rate = params_rate(params);
	for (i = 0; i < ARRAY_SIZE(ac101_sample_rate); i++) {
		if (ac101_sample_rate[i].real == sample_rate) {
			sample_rate_reg = ac101_sample_rate[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101_sample_rate)) {
		dev_err(dai->dev, "ac101 unsupport the sample rate config: %u\n", sample_rate);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_SR_CTRL, 0xf << ADDA_FS, sample_rate_reg << ADDA_FS);

	/* set channel */
	/* if enable tdm, here channels is 4, no matter what ch param given */
	channels = params_channels(params);
	if (channels > 4) {
		dev_err(dai->dev, "ac101 unsupport the channels config: %u\n", channels);
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (channels == 1) {
			/* rx slot0 L */
			regmap_update_bits(regmap, I2S_SDIN_CTRL,
					   0x1 << I2S_DAC_SLOT0L_EN,
					   0x1 << I2S_DAC_SLOT0L_EN);
		} else {
			regmap_update_bits(regmap, I2S_SDIN_CTRL,
					   0x1 << I2S_DAC_SLOT0L_EN,
					   0x1 << I2S_DAC_SLOT0L_EN);
			regmap_update_bits(regmap, I2S_SDIN_CTRL,
					   0x1 << I2S_DAC_SLOT0R_EN,
					   0x1 << I2S_DAC_SLOT0R_EN);
		}
	} else {
		if (channels == 1) {
			regmap_update_bits(regmap, I2S_SDOUT_CTRL,
					   0x1 << I2S_ADC_SLOT0L_EN,
					   0x1 << I2S_ADC_SLOT0L_EN);

		} else {
			regmap_update_bits(regmap, I2S_SDOUT_CTRL,
					   0x1 << I2S_ADC_SLOT0L_EN,
					   0x1 << I2S_ADC_SLOT0L_EN);
			regmap_update_bits(regmap, I2S_SDOUT_CTRL,
					   0x1 << I2S_ADC_SLOT0R_EN,
					   0x1 << I2S_ADC_SLOT0R_EN);
		}
	}

	/* BLCK DIV: ratio = sysclk / sample_rate / slots / slot_width*/
	bclk_ratio = ac101->sysclk_freq / sample_rate / ac101->slots / ac101->slot_width;
	for (i = 0; i < ARRAY_SIZE(ac101_bclk_div); i++) {
		if (ac101_bclk_div[i].real == bclk_ratio) {
			bclk_ratio_reg = ac101_bclk_div[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101_bclk_div)) {
		dev_err(dai->dev, "ac101 unsupport bclk_div: %d\n", bclk_ratio);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_CLK_CTRL, 0xf << BCLK_DIV, bclk_ratio_reg << BCLK_DIV);

	/* lrck: ratio = bclk / lrck  = sysclk / bclk_ratio / sample_rate */
	lrck_ratio = ac101->sysclk_freq / bclk_ratio / sample_rate;
	for (i = 0; i < ARRAY_SIZE(ac101_lrck_div); i++) {
		if (ac101_lrck_div[i].real == lrck_ratio) {
			lrck_ratio_reg = ac101_lrck_div[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101_lrck_div)) {
		dev_err(dai->dev, "ac101 unsupport lrlk_div: %d\n", lrck_ratio);
		return -EINVAL;
	}
	regmap_update_bits(regmap, I2S_CLK_CTRL, 0x7 << LRCK_DIV, lrck_ratio_reg << LRCK_DIV);

	if (!atomic_read(&clk_cnt)) {
		/* PLL Enable */
		if (pdata->sysclk_src == SYSCLK_SRC_PLL)
			regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_EN, 0x1 << PLLCLK_EN);

		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << I2SCLK_EN, 0x1 << I2SCLK_EN);
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_EN, 0x1 << SYSCLK_EN);
	}

	atomic_add(1, &clk_cnt);

	return 0;
}

static int ac101_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("\n");

	atomic_sub(1, &clk_cnt);

	if (!atomic_read(&clk_cnt)) {
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << SYSCLK_EN, 0x0 << SYSCLK_EN);
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << I2SCLK_EN, 0x0 << I2SCLK_EN);
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_EN, 0x0 << PLLCLK_EN);
	}

	return 0;
}

static int ac101_set_dai_pll(struct snd_soc_dai *dai, int pll_id, int source,
			     unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct regmap *regmap = ac101->regmap;
	unsigned int i  = 0;
	unsigned int m = 0;
	unsigned int n_i = 0;
	unsigned int n_f = 0;

	SND_LOG_DEBUG("freq_in:%d\n", freq_in);

	if (freq_in < 128000 || freq_in > 24576000) {
		dev_err(dai->dev, "ac101 pllclk source input only support [128K,24M], now %u\n",
			freq_in);
		return -EINVAL;
	}

	if (pdata->sysclk_src != SYSCLK_SRC_PLL) {
		dev_err(dai->dev, "ac101 sysclk source don't pll, don't need config pll\n");
		return 0;
	}

	switch (pdata->pllclk_src) {
	case PLLCLK_SRC_MCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << PLLCLK_SRC, 0x0 << PLLCLK_SRC);
		break;
	case PLLCLK_SRC_BCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << PLLCLK_SRC, 0x2 << PLLCLK_SRC);
		break;
	default:
		dev_err(dai->dev, "ac101 pllclk source config error: %d\n", pdata->pllclk_src);
		return -EINVAL;

	}

	/* PLLCLK: FOUT = (FIN * N) / (M * (2K + 1)); (N= N_i + 0.2*N_f)*/
	for (i = 0; i < ARRAY_SIZE(ac101_pll_div); i++) {
		if (ac101_pll_div[i].freq_in == freq_in && ac101_pll_div[i].freq_out == freq_out) {
			m = ac101_pll_div[i].m;
			n_i = ac101_pll_div[i].n_i;
			n_f = ac101_pll_div[i].n_f;
			dev_dbg(dai->dev, "ac101 PLL match freq_in:%u, freq_out:%u\n",
				freq_in, freq_out);
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101_pll_div)) {
		dev_err(dai->dev, "ac101 PLL don't match freq_in and freq_out table\n");
		return -EINVAL;
	}

	/* config pll M; N_I; N_F*/
	regmap_update_bits(regmap, PLL_CTRL1, 0x3f << PLL_DIV_M, m << PLL_DIV_M);
	regmap_update_bits(regmap, PLL_CTRL2, 0x3ff << PLL_DIV_M, n_i << PLL_DIV_M);
	regmap_update_bits(regmap, PLL_CTRL2, 0x7 << PLL_DIV_M, n_f << PLL_DIV_M);

	/* PLL Enable */
	regmap_update_bits(regmap, PLL_CTRL2, 0x1 << PLL_EN, 0x1 << PLL_EN);

	/* PLLCLK Enable*/
	regmap_update_bits(regmap, SYSCLK_CTRL, 0x1 << PLLCLK_EN, 0x1 << PLLCLK_EN);

	return 0;
}

static int ac101_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
				unsigned int freq, int dir)
{
	struct snd_soc_component *component = dai->component;
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("\n");

	/* sysclk = 512 * fs */

	switch (pdata->sysclk_src) {
	case SYSCLK_SRC_MCLK:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << I2SCLK_SRC, 0x0 << I2SCLK_SRC);
		break;
	case SYSCLK_SRC_PLL:
		regmap_update_bits(regmap, SYSCLK_CTRL, 0x3 << I2SCLK_SRC, 0x11 << I2SCLK_SRC);
		break;
	default:
		dev_err(dai->dev, "ac101 sysclk source config error: %d\n", pdata->sysclk_src);
		return -EINVAL;
	}

	ac101->sysclk_freq = freq;

	return 0;
}

static int ac101_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int i2s_mode;
	unsigned int brck_polarity, lrck_polarity;

	SND_LOG_DEBUG("\n");

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		/* 0: master */
		regmap_update_bits(regmap, I2S_CLK_CTRL, 0x1 << I2S_MSTR, 0x0 << I2S_MSTR);

		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		regmap_update_bits(regmap, I2S_CLK_CTRL, 0x1 << I2S_MSTR, 0x1 << I2S_MSTR);

		break;
	default:
		dev_err(dai->dev, "only support CBM_CFM or CBS_CFS\n");
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		i2s_mode = 0;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		i2s_mode = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		i2s_mode = 2;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		i2s_mode = 3;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		i2s_mode = 3;
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(regmap, I2S_CLK_CTRL, 0x3 << DATA_FMT, i2s_mode << DATA_FMT);

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

	regmap_update_bits(regmap, I2S_CLK_CTRL, 0x1 << I2S_BCLK_INV, brck_polarity << I2S_BCLK_INV);
	regmap_update_bits(regmap, I2S_CLK_CTRL, 0x1 << I2S_LRCK_INV, lrck_polarity << I2S_LRCK_INV);

	ac101->fmt = fmt;

	return 0;
}

static int ac101_set_dai_tdm_slot(struct snd_soc_dai *dai,
				  unsigned int tx_mask, unsigned int rx_mask,
				  int slots, int slot_width)
{
	struct snd_soc_component *component = dai->component;
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	int i;
	unsigned int slot_width_reg = 0;

	SND_LOG_DEBUG("\n");

	for (i = 0; i < ARRAY_SIZE(ac101_slot_width); i++) {
		if (ac101_slot_width[i].real == slot_width) {
			slot_width_reg = ac101_slot_width[i].reg;
			break;
		}
	}
	if (i == ARRAY_SIZE(ac101_slot_width)) {
		dev_err(dai->dev, "ac101 unsupport slot_width: %d\n", slot_width);
		return -EINVAL;
	}

	regmap_update_bits(regmap, I2S_SDOUT_CTRL, 0x3 << SLOT_SIZE, slot_width_reg << SLOT_SIZE);

	ac101->slots = slots;
	ac101->slot_width = slot_width;

	return 0;
}

static const struct snd_soc_dai_ops ac101_dai_ops = {
	.startup	= ac101_startup,
	.shutdown	= ac101_shutdown,
	.hw_params	= ac101_hw_params,
	.hw_free	= ac101_hw_free,
	/* should: set_pll -> set_sysclk */
	.set_pll	= ac101_set_dai_pll,
	.set_sysclk	= ac101_set_dai_sysclk,
	/* should: set_fmt -> set_tdm_slot */
	.set_fmt	= ac101_set_dai_fmt,
	.set_tdm_slot	= ac101_set_dai_tdm_slot,
};


static struct snd_soc_dai_driver ac101_dai = {
	.name = "ac101-codec",
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 4,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S8
				| SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE,
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
				| SNDRV_PCM_FMTBIT_S24_LE,
		},
	.ops = &ac101_dai_ops,
};

static int sunxi_get_plug_det_thr_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_CTRL2, &reg_val);

	ucontrol->value.integer.value[0] =  (reg_val >> HMIC_TH1) & 0x1f;

	return 0;
}

static int sunxi_put_plug_det_thr_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_CTRL2, 0x1f << HMIC_TH1, reg_val << HMIC_TH1);

	return 0;
}

static int sunxi_get_plug_det_debounce_time_mode(struct snd_kcontrol *kcontrol,
						 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_CTRL1, &reg_val);

	ucontrol->value.integer.value[0] =  (reg_val >> HMIC_N) & 0xf;

	return 0;
}

static int sunxi_put_plug_det_debounce_time_mode(struct snd_kcontrol *kcontrol,
						 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_CTRL1, 0xf << HMIC_N, reg_val << HMIC_N);

	return 0;
}

static int sunxi_get_key_det_thr_mode(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_CTRL2, &reg_val);

	ucontrol->value.integer.value[0] =  (reg_val >> HMIC_TH2) & 0x1f;

	return 0;
}

static int sunxi_put_key_det_thr_mode(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_CTRL2, 0x1f << HMIC_TH2, reg_val << HMIC_TH2);

	return 0;
}

static int sunxi_get_key_det_debounce_time_mode(struct snd_kcontrol *kcontrol,
						 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_CTRL1, &reg_val);

	ucontrol->value.integer.value[0] =  (reg_val >> HMIC_M) & 0xf;

	return 0;
}

static int sunxi_put_key_det_debounce_time_mode(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_CTRL1, 0xf << HMIC_M, reg_val << HMIC_M);

	return 0;
}

/* hook */
static int sunxi_get_key_det_hook_min_mode(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	ucontrol->value.integer.value[0] = jack_adv_priv->key_det_vol[0][0];

	return 0;
}

static int sunxi_put_key_det_hook_min_mode(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	jack_adv_priv->key_det_vol[0][0] = ucontrol->value.integer.value[0];

	return 0;
}

static int sunxi_get_key_det_hook_max_mode(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	ucontrol->value.integer.value[0] = jack_adv_priv->key_det_vol[0][1];

	return 0;
}

static int sunxi_put_key_det_hook_max_mode(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	jack_adv_priv->key_det_vol[0][1] = ucontrol->value.integer.value[0];

	return 0;
}

/* vol+ */
static int sunxi_get_key_det_vol_add_min_mode(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	ucontrol->value.integer.value[0] = jack_adv_priv->key_det_vol[1][0];

	return 0;
}

static int sunxi_put_key_det_vol_add_min_mode(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	jack_adv_priv->key_det_vol[1][0] = ucontrol->value.integer.value[0];

	return 0;
}

static int sunxi_get_key_det_vol_add_max_mode(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	ucontrol->value.integer.value[0] = jack_adv_priv->key_det_vol[1][1];

	return 0;
}

static int sunxi_put_key_det_vol_add_max_mode(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	jack_adv_priv->key_det_vol[1][1] = ucontrol->value.integer.value[0];

	return 0;
}

/* vol- */
static int sunxi_get_key_det_vol_sub_min_mode(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	ucontrol->value.integer.value[0] = jack_adv_priv->key_det_vol[2][0];

	return 0;
}

static int sunxi_put_key_det_vol_sub_min_mode(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	jack_adv_priv->key_det_vol[2][0] = ucontrol->value.integer.value[0];

	return 0;
}

static int sunxi_get_key_det_vol_sub_max_mode(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	ucontrol->value.integer.value[0] = jack_adv_priv->key_det_vol[2][1];

	return 0;
}

static int sunxi_put_key_det_vol_sub_max_mode(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	jack_adv_priv->key_det_vol[2][1] = ucontrol->value.integer.value[0];

	return 0;
}

/* sample sel */
static int sunxi_get_sample_sel_mode(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_CTRL2, &reg_val);

	ucontrol->value.integer.value[0] =  (reg_val >> SAMPLE_SEL) & 0x3;

	return 0;
}

static int sunxi_put_sample_sel_mode(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_CTRL2, 0x3 << SAMPLE_SEL, reg_val << SAMPLE_SEL);

	return 0;
}

/* smooth filter */
static int sunxi_get_smooth_filter_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HMIC_CTRL2, &reg_val);

	ucontrol->value.integer.value[0] =  (reg_val >> HMIC_SF) & 0x3;

	return 0;
}

static int sunxi_put_smooth_filter_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	reg_val = ucontrol->value.integer.value[0];

	regmap_update_bits(regmap, HMIC_CTRL2, 0x3 << HMIC_SF, reg_val << HMIC_SF);

	return 0;
}

static const struct snd_kcontrol_new ac101_jack_controls[] = {
	/* hmic sample sel*/
	SOC_SINGLE_EXT("hmic sample sel", HMIC_CTRL2, SAMPLE_SEL, 3, 0,
		       sunxi_get_sample_sel_mode,
		       sunxi_put_sample_sel_mode),
	/* hmic smooth filter setting */
	SOC_SINGLE_EXT("hmic smooth filter", HMIC_CTRL2, HMIC_SF, 3, 0,
		       sunxi_get_smooth_filter_mode,
		       sunxi_put_smooth_filter_mode),
	/* plug in/out */
	SOC_SINGLE_EXT("plug det threshold", HMIC_CTRL2, HMIC_TH1, 31, 0,
		       sunxi_get_plug_det_thr_mode,
		       sunxi_put_plug_det_thr_mode),
	SOC_SINGLE_EXT("plug det debouce time", HMIC_CTRL1, HMIC_N, 15, 0,
		       sunxi_get_plug_det_debounce_time_mode,
		       sunxi_put_plug_det_debounce_time_mode),
	/* headphone, headset */
	SOC_SINGLE_EXT("key det threshold", HMIC_CTRL2, HMIC_TH2, 31, 0,
		       sunxi_get_key_det_thr_mode,
		       sunxi_put_key_det_thr_mode),
	SOC_SINGLE_EXT("key det debouce time", HMIC_CTRL1, HMIC_M, 15, 0,
		       sunxi_get_key_det_debounce_time_mode,
		       sunxi_put_key_det_debounce_time_mode),
	/* vol+ */
	SOC_SINGLE_EXT("key det vol+ min", SND_SOC_NOPM, 0, 31, 0,
		       sunxi_get_key_det_vol_add_min_mode,
		       sunxi_put_key_det_vol_add_min_mode),
	SOC_SINGLE_EXT("key det vol+ max", SND_SOC_NOPM, 0, 31, 0,
		       sunxi_get_key_det_vol_add_max_mode,
		       sunxi_put_key_det_vol_add_max_mode),
	/* vol- */
	SOC_SINGLE_EXT("key det vol- min", SND_SOC_NOPM, 0, 31, 0,
		       sunxi_get_key_det_vol_sub_min_mode,
		       sunxi_put_key_det_vol_sub_min_mode),
	SOC_SINGLE_EXT("key det vol- max", SND_SOC_NOPM, 0, 31, 0,
		       sunxi_get_key_det_vol_sub_max_mode,
		       sunxi_put_key_det_vol_sub_max_mode),
	/* hook */
	SOC_SINGLE_EXT("key det hook min", SND_SOC_NOPM, 0, 31, 0,
		       sunxi_get_key_det_hook_min_mode,
		       sunxi_put_key_det_hook_min_mode),
	SOC_SINGLE_EXT("key det hook max", SND_SOC_NOPM, 0, 31, 0,
		       sunxi_get_key_det_hook_max_mode,
		       sunxi_put_key_det_hook_max_mode),
};

static int ac101_probe(struct snd_soc_component *component)
{
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct regmap *regmap = ac101->regmap;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;
	unsigned int reg_val;
	int ret = -1;
	unsigned int i;
	unsigned int try_num = 5;

	SND_LOG_DEBUG("\n");

	/* wait ac101 stable */
	msleep(50);

	for (i = 0; (i < try_num) && (ret < 0); i++) {
		ret = regmap_read(regmap, CHIP_SOFT_RST, &reg_val);
	}
	if (ret) {
		SND_LOG_ERR("try read ac101 5 times but failed, ac101 probe failed\n");
		return -1;
	}

	/* software reset to wait ac101 stable,
	 * write 0x123 to reset ac101,
	 * reset successfully when reg val is 0x101.
	 */
	regmap_write(regmap, CHIP_SOFT_RST, 0x123);

	do {
		regmap_read(regmap, CHIP_SOFT_RST, &reg_val);
		SND_LOG_INFO("wait ac101 reset successfully, need 0x101/0x%x\n", reg_val);
	} while (reg_val != 0x101);

	pdata->working = (atomic_t)ATOMIC_INIT(0);

	regmap_update_bits(regmap, ADC_VOL_CTRL, 0xff << ADCL_VOL, pdata->adcl_vol << ADCL_VOL);
	regmap_update_bits(regmap, ADC_VOL_CTRL, 0xff << ADCR_VOL, pdata->adcr_vol << ADCR_VOL);

	regmap_update_bits(regmap, DAC_VOL_CTRL, 0xff << DACL_VOL, pdata->dacl_vol << DACL_VOL);
	regmap_update_bits(regmap, DAC_VOL_CTRL, 0xff << DACR_VOL, pdata->dacr_vol << DACR_VOL);

	regmap_update_bits(regmap, ADC_APC_CTRL, 0x7 << ADCLG, pdata->adcl_gain << ADCLG);
	regmap_update_bits(regmap, ADC_APC_CTRL, 0x7 << ADCRG, pdata->adcr_gain << ADCRG);

	regmap_update_bits(regmap, ADC_SRCBST_CTRL, 0x7 << MIC1BOOST, pdata->mic1_gain << MIC1BOOST);
	regmap_update_bits(regmap, ADC_SRCBST_CTRL, 0x7 << MIC2BOOST, pdata->mic2_gain << MIC2BOOST);

	regmap_update_bits(regmap, HPOUT_CTRL, 0x3f << HP_VOL, pdata->hpout_vol << HP_VOL);
	regmap_update_bits(regmap, SPKOUT_CTRL, 0x3f << SPK_VOL, pdata->spk_vol << SPK_VOL);

	/* DAC_MXR_SRC, DACL: slot0l, DACR: slot0r */
	regmap_update_bits(regmap, DAC_MXR_SRC,
			   0x1 << DAC_LMXR_I2S_DA0L | 0x1 << DAC_RMXR_I2S_DA0R,
			   0x1 << DAC_LMXR_I2S_DA0L | 0x1 << DAC_RMXR_I2S_DA0R);

	/* HPOUT SRC: outputl/r_mixer*/
	regmap_update_bits(regmap, HPOUT_CTRL,
			   0x1 << RHPSRC | 0x1 << LHPSRC,
			   0x1 << RHPSRC | 0x1 << LHPSRC);

	/* SPKOUT SRC: */
	regmap_update_bits(regmap, SPKOUT_CTRL,
			   0x1 << RSPKSRC | 0x1 << LSPKSRC,
			   0x0 << RSPKSRC | 0x0 << LSPKSRC);

	/* I2S_DIG_MXR_SRC: slot0l src: adcl, slot0r src:adcr */
	regmap_update_bits(regmap, I2S_DIG_MXR_SRC,
			   0x1 << I2SDA0_LMXR_ADCL | 0x1 << I2SDA0_RMXR_ADCR,
			   0x1 << I2SDA0_LMXR_ADCL | 0x1 << I2SDA0_RMXR_ADCR);

	regmap_update_bits(regmap, I2S_CLK_CTRL, 0x1 << TDM_EN, 0x1 << TDM_EN);

	regmap_update_bits(regmap, SPKOUT_CTRL, 0x7 << CLK_ADJUST, 0x7 << CLK_ADJUST);

	ret = snd_soc_add_component_controls(component, ac101_jack_controls,
					     ARRAY_SIZE(ac101_jack_controls));
	if (ret)
		SND_LOG_ERR("add ac101_jack_controls failed\n");

	/* component kcontrols -> pa */
	ret = snd_sunxi_pa_pin_probe(jack_adv_priv->pa_cfg,
				     jack_adv_priv->pa_pin_max,
				     component);
	if (ret)
		SND_LOG_ERR("register pa kcontrols failed\n");

	pdata->jack_adv_priv.regmap = regmap;
	pdata->jack_adv_priv.dev = ac101->dev;
	sunxi_jack_adv.data = (void *)(&pdata->jack_adv_priv);
	sunxi_jack_adv.dev = ac101->dev;
	snd_sunxi_jack_init(&sunxi_jack_port);

	return 0;
}

static void ac101_remove(struct snd_soc_component *component)
{
	SND_LOG_DEBUG("\n");

	snd_sunxi_jack_exit(&sunxi_jack_port);

}

static int ac101_suspend(struct snd_soc_component *component)
{
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct regmap *regmap = ac101->regmap;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;

	SND_LOG_DEBUG("\n");

	snd_sunxi_save_reg(regmap, &sunxi_reg_group);
	snd_sunxi_pa_pin_disable(jack_adv_priv->pa_cfg,
				 jack_adv_priv->pa_pin_max);

	snd_sunxi_regulator_disable(ac101->rglt);

	return 0;
}

static int ac101_resume(struct snd_soc_component *component)
{
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
	struct regmap *regmap = ac101->regmap;
	struct sunxi_jack_adv_priv *jack_adv_priv = &pdata->jack_adv_priv;
	unsigned int reg_val;
	int ret;

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_regulator_enable(ac101->rglt);
	if (ret)
		return ret;

	snd_sunxi_pa_pin_disable(jack_adv_priv->pa_cfg,
				 jack_adv_priv->pa_pin_max);

	msleep(50);
	/* software reset */
	regmap_write(regmap, CHIP_SOFT_RST, 0x123);

	do {
		regmap_read(regmap, CHIP_SOFT_RST, &reg_val);
	} while (reg_val != 0x101);

	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);

	return 0;
}

static int ac101_get_hpl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HPOUT_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << LHPSRC)) ? 1 : 0);

	return 0;
}

static int ac101_set_hpl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << LHPSRC, 0x0 << LHPSRC);
		break;
	case 1:
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << LHPSRC, 0x1 << LHPSRC);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101_get_hpr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, HPOUT_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << RHPSRC)) ? 1 : 0);

	return 0;
}

static int ac101_set_hpr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << RHPSRC, 0x0 << RHPSRC);
		break;
	case 1:
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << RHPSRC, 0x1 << RHPSRC);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

//spk src
static int ac101_get_spkl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, SPKOUT_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << LSPKSRC)) ? 1 : 0);

	return 0;
}

static int ac101_set_spkl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SPKOUT_CTRL, 0x1 << LSPKSRC, 0x0 << LSPKSRC);
		break;
	case 1:
		regmap_update_bits(regmap, SPKOUT_CTRL, 0x1 << LSPKSRC, 0x1 << LSPKSRC);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac101_get_spkr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	unsigned int reg_val;

	regmap_read(regmap, SPKOUT_CTRL, &reg_val);
	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << RSPKSRC)) ? 1 : 0);

	return 0;
}

static int ac101_set_spkr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SPKOUT_CTRL, 0x1 << RSPKSRC, 0x0 << RSPKSRC);
		break;
	case 1:
		regmap_update_bits(regmap, SPKOUT_CTRL, 0x1 << RSPKSRC, 0x1 << RSPKSRC);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static const DECLARE_TLV_DB_SCALE(adc_digital_vol_tlv, -11925, 75, 1);
static const DECLARE_TLV_DB_SCALE(dac_digital_vol_tlv, -11925, 75, 1);
static const DECLARE_TLV_DB_SCALE(adc_mixer_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(dac_mixer_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mixer_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(linein_vol_tlv, -1200, 300, 0);

static const DECLARE_TLV_DB_RANGE(mic_vol_tlv,
	0, 0, TLV_DB_SCALE_ITEM(0, 0, 0),
	1, 7, TLV_DB_SCALE_ITEM(3000, 300, 0),
);

static const DECLARE_TLV_DB_SCALE(hpout_vol_tlv, -6200, 100, 1);
static const DECLARE_TLV_DB_SCALE(spk_vol_tlv, -4350, 150, 1);

static const struct snd_kcontrol_new ac101_snd_controls[] = {
	SOC_SINGLE_TLV("ADCL Volume", ADC_VOL_CTRL, ADCL_VOL, 0xff, 0, adc_digital_vol_tlv),
	SOC_SINGLE_TLV("ADCR Volume", ADC_VOL_CTRL, ADCR_VOL, 0xff, 0, adc_digital_vol_tlv),

	SOC_SINGLE_TLV("DACL Volume", DAC_VOL_CTRL, DACL_VOL, 0xff, 0, dac_digital_vol_tlv),
	SOC_SINGLE_TLV("DACR Volume", DAC_VOL_CTRL, DACR_VOL, 0xff, 0, dac_digital_vol_tlv),

	SOC_SINGLE_TLV("HPOUT Volume", HPOUT_CTRL, HP_VOL, 0x3f, 0, hpout_vol_tlv),
	SOC_SINGLE_TLV("SPK Volume", SPKOUT_CTRL, SPK_VOL, 0x1f, 0, spk_vol_tlv),

	SOC_SINGLE_TLV("ADCL Gain", ADC_APC_CTRL, ADCLG, 0x7, 0, adc_vol_tlv),
	SOC_SINGLE_TLV("ADCR Gain", ADC_APC_CTRL, ADCRG, 0x7, 0, adc_vol_tlv),

	SOC_SINGLE_TLV("MIC1 Gain", ADC_SRCBST_CTRL, MIC1BOOST, 0x7, 0, mic_vol_tlv),
	SOC_SINGLE_TLV("MIC2 Gain", ADC_SRCBST_CTRL, MIC2BOOST, 0x7, 0, mic_vol_tlv),

	SOC_SINGLE_TLV("LINEIN Gain", ADC_SRCBST_CTRL, LINEIN_PREG, 0x7, 0, linein_vol_tlv),
};

static const struct snd_kcontrol_new outputl_src_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", OMIXER_SR, LMXR_MIC1, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", OMIXER_SR, LMXR_MIC2, 1, 0),
	SOC_DAPM_SINGLE("LINEIN Switch", OMIXER_SR, LMXR_LINEIN, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", OMIXER_SR, LMXR_LINEINL, 1, 0),
	SOC_DAPM_SINGLE("DACL Switch", OMIXER_SR, LMXR_DACL, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", OMIXER_SR, LMXR_DACR, 1, 0),
};

static const struct snd_kcontrol_new outputr_src_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", OMIXER_SR, RMXR_MIC1, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", OMIXER_SR, RMXR_MIC2, 1, 0),
	SOC_DAPM_SINGLE("LINEIN Switch", OMIXER_SR, RMXR_LINEIN, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", OMIXER_SR, RMXR_LINEINR, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", OMIXER_SR, RMXR_DACR, 1, 0),
	SOC_DAPM_SINGLE("DACL Switch", OMIXER_SR, RMXR_DACL, 1, 0),
};

static const struct snd_kcontrol_new inputl_src_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", ADC_SRC, MIC1_TO_LADC, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", ADC_SRC, MIC2_TO_LADC, 1, 0),
	SOC_DAPM_SINGLE("LINEIN Switch", ADC_SRC, LINEIN_TO_LADC, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", ADC_SRC, LINEINL_TO_LADC, 1, 0),
	SOC_DAPM_SINGLE("OMIXL Switch", ADC_SRC, OUTPUTL_MIX_TO_LADC, 1, 0),
	SOC_DAPM_SINGLE("OMIXR Switch", ADC_SRC, OUTPUTR_MIX_TO_LADC, 1, 0),
};

static const struct snd_kcontrol_new inputr_src_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", ADC_SRC, MIC1_TO_RADC, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", ADC_SRC, MIC2_TO_RADC, 1, 0),
	SOC_DAPM_SINGLE("LINEIN Switch", ADC_SRC, LINEIN_TO_RADC, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", ADC_SRC, LINEINR_TO_RADC, 1, 0),
	SOC_DAPM_SINGLE("OMIXR Switch", ADC_SRC, OUTPUTR_MIX_TO_RADC, 1, 0),
	SOC_DAPM_SINGLE("OMIXL Switch", ADC_SRC, OUTPUTL_MIX_TO_RADC, 1, 0),
};

static const char *hpl_src_text[] = {"DACL", "OMIXL"};
static const char *hpr_src_text[] = {"DACR", "OMIXR"};

static const char *spkl_src_text[] = {"OMIXL", "OMIXL_R"};
static const char *spkr_src_text[] = {"OMIXR", "OMIXL_R"};

static const struct soc_enum hpl_src_mux_enum =
	SOC_ENUM_SINGLE(HPOUT_CTRL, LHPSRC, 2, hpl_src_text);
static const struct soc_enum hpr_src_mux_enum =
	SOC_ENUM_SINGLE(HPOUT_CTRL, RHPSRC, 2, hpr_src_text);
static const struct soc_enum spkl_src_mux_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL, LSPKSRC, 2, spkl_src_text);
static const struct soc_enum spkr_src_mux_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL, RSPKSRC, 2, spkr_src_text);

static const struct snd_kcontrol_new hpl_src_mux =
	SOC_DAPM_ENUM_EXT("HPL Source Mux", hpl_src_mux_enum, ac101_get_hpl_src, ac101_set_hpl_src);

static const struct snd_kcontrol_new hpr_src_mux =
	SOC_DAPM_ENUM_EXT("HPR Source Mux", hpr_src_mux_enum, ac101_get_hpr_src, ac101_set_hpr_src);

static const struct snd_kcontrol_new spkl_src_mux =
	SOC_DAPM_ENUM_EXT("LINEOUTL Source Mux", spkl_src_mux_enum, ac101_get_spkl_src, ac101_set_spkl_src);

static const struct snd_kcontrol_new spkr_src_mux =
	SOC_DAPM_ENUM_EXT("LINEOUTR Source Mux", spkr_src_mux_enum, ac101_get_spkr_src, ac101_set_spkr_src);

static int ac101_spk_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct ac101_data *pdata = &ac101->pdata;
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

static int ac101_lineoutl_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("event:%d\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* spkl en */
		regmap_update_bits(regmap, SPKOUT_CTRL, 0x1 << LSPK_EN, 0x1 << LSPK_EN);
		msleep(100);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* spkl en */
		regmap_update_bits(regmap, SPKOUT_CTRL, 0x1 << LSPK_EN, 0x0 << LSPK_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101_lineoutr_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("event:%d\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* spkr en */
		regmap_update_bits(regmap, SPKOUT_CTRL, 0x1 << RSPK_EN, 0x1 << RSPK_EN);
		msleep(100);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* spkr en */
		regmap_update_bits(regmap, SPKOUT_CTRL, 0x1 << RSPK_EN, 0x0 << RSPK_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101_hpoutl_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* hp dc offset, 0xf before hp pa enable */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0xf << HP_DCRM_EN, 0xf << HP_DCRM_EN);
		/* hp pa */
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << HPPA_EN, 0x1 << HPPA_EN);
		msleep(10);
		/* hpl enable */
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << MUTE_L, 0x1 << MUTE_L);
		/* wait for stable */
		msleep(4);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* hpl disable */
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << MUTE_L, 0x0 << MUTE_L);
		/* hp dc offset, 0x0 before hp pa disable */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0xf << HP_DCRM_EN, 0x0 << HP_DCRM_EN);
		/* hp pa */
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << HPPA_EN, 0x0 << HPPA_EN);

		break;
	default:
		break;
	}

	return 0;
}

static int ac101_hpoutr_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* hp dc offset, 0xf before hp pa enable */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0xf << HP_DCRM_EN, 0xf << HP_DCRM_EN);
		/* hp pa */
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << HPPA_EN, 0x1 << HPPA_EN);
		msleep(10);
		/* hpl enable */
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << MUTE_R, 0x1 << MUTE_R);
		/* wait for stable */
		msleep(4);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* hpl disable */
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << MUTE_R, 0x0 << MUTE_R);
		/* hp da offset, 0x0 before hp pa disable */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0xf << HP_DCRM_EN, 0x0 << HP_DCRM_EN);
		/* hp pa */
		regmap_update_bits(regmap, HPOUT_CTRL, 0x1 << HPPA_EN, 0x0 << HPPA_EN);

		break;
	default:
		break;
	}

	return 0;
}

static int ac101_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, ADC_SRCBST_CTRL, 0x1 << MIC1AMPEN, 0x1 << MIC1AMPEN);

		atomic_sub(1, &adc_a_cnt);
		if ((!atomic_read(&adc_a_cnt)))
			msleep(250);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ADC_SRCBST_CTRL, 0x1 << MIC1AMPEN, 0x0 << MIC1AMPEN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101_mic2_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, ADC_SRCBST_CTRL, 0x1 << MIC2PIN_EN, 0x1 << MIC2PIN_EN);
		regmap_update_bits(regmap, ADC_SRCBST_CTRL, 0x1 << MIC2AMPEN, 0x1 << MIC2AMPEN);
		atomic_sub(1, &adc_a_cnt);
		if ((!atomic_read(&adc_a_cnt)))
			msleep(250);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ADC_SRCBST_CTRL, 0x1 << MIC2AMPEN, 0x0 << MIC2AMPEN);
		regmap_update_bits(regmap, ADC_SRCBST_CTRL, 0x1 << MIC2PIN_EN, 0x0 << MIC2PIN_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101_dmic_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;
	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, ADC_DIG_CTRL, 0x1 << DIG_MIC_EN, 0x1 << DIG_MIC_EN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ADC_DIG_CTRL, 0x1 << DIG_MIC_EN, 0x1 << DIG_MIC_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101_linein_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (atomic_read(&adc_a_cnt)) {
			atomic_sub(1, &adc_a_cnt);
			if ((!atomic_read(&adc_a_cnt)))
				msleep(250);
		}
		break;
	case SND_SOC_DAPM_PRE_PMD:
		break;
	default:
		break;
	}

	return 0;
}

static int ac101_playbackl_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("event:%d\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* adc Analog left */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0x1 << DACALEN, 0x1 << DACALEN);
		/* left Analog output Mixer */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0x1 << LMIXEN, 0x1 << LMIXEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* adc Analog left */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0x1 << DACALEN, 0x0 << DACALEN);
		/* left Analog output Mixer */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0x1 << LMIXEN, 0x0 << LMIXEN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101_playbackr_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("event:%d\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* adc Analog right */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0x1 << DACAREN, 0x1 << DACAREN);
		/* right Analog output Mixer */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0x1 << RMIXEN, 0x1 << RMIXEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* adc Analog right */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0x1 << DACAREN, 0x0 << DACAREN);
		/* right Analog output Mixer */
		regmap_update_bits(regmap, OMIXER_DACA_CTRL, 0x1 << RMIXEN, 0x0 << RMIXEN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101_capturel_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		atomic_add(1, &adc_a_cnt);
		regmap_update_bits(regmap, ADC_APC_CTRL, 0x1 << ADCLEN, 0x1 << ADCLEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ADC_APC_CTRL, 0x1 << ADCLEN, 0x0 << ADCLEN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac101_capturer_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac101_priv *ac101 = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = ac101->regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		atomic_add(1, &adc_a_cnt);
		regmap_update_bits(regmap, ADC_APC_CTRL, 0x1 << ADCREN, 0x1 << ADCREN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, ADC_APC_CTRL, 0x1 << ADCREN, 0x0 << ADCREN);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget ac101_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MIC1P_PIN"),
	SND_SOC_DAPM_INPUT("MIC1N_PIN"),
	SND_SOC_DAPM_INPUT("MIC2P_PIN"),
	SND_SOC_DAPM_INPUT("MIC2N_PIN"),

	SND_SOC_DAPM_INPUT("LINEINL_PIN"),
	SND_SOC_DAPM_INPUT("LINEINR_PIN"),


	SND_SOC_DAPM_OUTPUT("LINEOUTLP_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTLN_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTRP_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTRN_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTL_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTR_PIN"),

	SND_SOC_DAPM_AIF_OUT_E("ADCL", "Capture", 0, ADC_DIG_CTRL, ADC_DIG_EN, 0,
			       ac101_capturel_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADCR", "Capture", 0, ADC_DIG_CTRL, ADC_DIG_EN, 0,
			       ac101_capturer_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, DAC_DIG_CTRL, ENDA, 0,
			      ac101_playbackl_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, DAC_DIG_CTRL, ENDA, 0,
			      ac101_playbackr_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER("InputL Mixer", SND_SOC_NOPM, 0, 0,
			   inputl_src_mixer, ARRAY_SIZE(inputl_src_mixer)),
	SND_SOC_DAPM_MIXER("InputR Mixer", SND_SOC_NOPM, 0, 0,
			   inputr_src_mixer, ARRAY_SIZE(inputr_src_mixer)),

	SND_SOC_DAPM_MIXER("OutputL Mixer", OMIXER_DACA_CTRL, LMIXEN, 0,
			   outputl_src_mixer, ARRAY_SIZE(outputl_src_mixer)),
	SND_SOC_DAPM_MIXER("OutputR Mixer", OMIXER_DACA_CTRL, RMIXEN, 0,
			   outputr_src_mixer, ARRAY_SIZE(outputr_src_mixer)),

	SND_SOC_DAPM_MUX("HPOUTL Source Mux", SND_SOC_NOPM, 0, 0, &hpl_src_mux),
	SND_SOC_DAPM_MUX("HPOUTR Source Mux", SND_SOC_NOPM, 0, 0, &hpr_src_mux),
	SND_SOC_DAPM_MUX("LINEOUTL Source Mux", SND_SOC_NOPM, 0, 0, &spkl_src_mux),
	SND_SOC_DAPM_MUX("LINEOUTR Source Mux", SND_SOC_NOPM, 0, 0, &spkr_src_mux),

	SND_SOC_DAPM_MICBIAS("MICBIAS", ADC_APC_CTRL, MBIASEN, 0),
	SND_SOC_DAPM_MICBIAS("MICBIAS CHOP", ADC_APC_CTRL, CHOPPER_EN, 0),

	SND_SOC_DAPM_LINE("LINEOUTL", ac101_lineoutl_event),
	SND_SOC_DAPM_LINE("LINEOUTR", ac101_lineoutr_event),
	SND_SOC_DAPM_SPK("SPK", ac101_spk_event),
	SND_SOC_DAPM_HP("HPOUTL", ac101_hpoutl_event),
	SND_SOC_DAPM_HP("HPOUTR", ac101_hpoutr_event),
	SND_SOC_DAPM_MIC("MIC1", ac101_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", ac101_mic2_event),
	SND_SOC_DAPM_MIC("DMIC", ac101_dmic_event),
	SND_SOC_DAPM_LINE("LINEINL", ac101_linein_event),
	SND_SOC_DAPM_LINE("LINEINR", ac101_linein_event),
};

static const struct snd_soc_dapm_route ac101_dapm_routes[] = {
	{"MICBIAS", NULL, "MIC1P_PIN"},
	{"MICBIAS", NULL, "MIC1N_PIN"},
	{"MICBIAS", NULL, "MIC2P_PIN"},
	{"MICBIAS", NULL, "MIC2N_PIN"},

	{"MICBIAS CHOP", NULL, "MICBIAS"},

	{"InputL Mixer", "MIC1 Switch", "MICBIAS CHOP"},
	{"InputL Mixer", "MIC2 Switch", "MICBIAS CHOP"},
	{"InputL Mixer", "LINEIN Switch", "LINEINL_PIN"},
	{"InputL Mixer", "LINEIN Switch", "LINEINR_PIN"},
	{"InputL Mixer", "LINEINL Switch", "LINEINL_PIN"},
	{"InputL Mixer", "OMIXL Switch", "OutputL Mixer"},
	{"InputL Mixer", "OMIXR Switch", "OutputR Mixer"},

	{"InputR Mixer", "MIC1 Switch", "MICBIAS CHOP"},
	{"InputR Mixer", "MIC2 Switch", "MICBIAS CHOP"},
	{"InputR Mixer", "LINEIN Switch", "LINEINL_PIN"},
	{"InputR Mixer", "LINEIN Switch", "LINEINR_PIN"},
	{"InputR Mixer", "LINEINR Switch", "LINEINR_PIN"},
	{"InputR Mixer", "OMIXR Switch", "OutputL Mixer"},
	{"InputR Mixer", "OMIXL Switch", "OutputR Mixer"},

	{"ADCL", NULL, "InputL Mixer"},
	{"ADCR", NULL, "InputR Mixer"},

	{"OutputL Mixer", "MIC1 Switch", "MIC1"},
	{"OutputL Mixer", "MIC2 Switch", "MIC2"},
	{"OutputL Mixer", "LINEIN Switch", "LINEINL_PIN"},
	{"OutputL Mixer", "LINEIN Switch", "LINEINR_PIN"},
	{"OutputL Mixer", "LINEINL Switch", "LINEINL_PIN"},
	{"OutputL Mixer", "DACL Switch", "DACL"},
	{"OutputL Mixer", "DACR Switch", "DACR"},

	{"OutputR Mixer", "MIC1 Switch", "MIC1"},
	{"OutputR Mixer", "MIC2 Switch", "MIC2"},
	{"OutputR Mixer", "LINEIN Switch", "LINEINL_PIN"},
	{"OutputR Mixer", "LINEIN Switch", "LINEINR_PIN"},
	{"OutputR Mixer", "LINEINR Switch", "LINEINR_PIN"},
	{"OutputR Mixer", "DACL Switch", "DACL"},
	{"OutputR Mixer", "DACR Switch", "DACR"},

	{"HPOUTL Source Mux", "OMIXL", "OutputL Mixer"},
	{"HPOUTL Source Mux", "DACL", "DACL"},
	{"HPOUTR Source Mux", "OMIXR", "OutputR Mixer"},
	{"HPOUTR Source Mux", "DACR", "DACR"},

	{"LINEOUTL Source Mux", NULL, "OutputL Mixer"},
	{"LINEOUTL Source Mux", NULL, "OutputR Mixer"},
	{"LINEOUTR Source Mux", NULL, "OutputL Mixer"},
	{"LINEOUTR Source Mux", NULL, "OutputR Mixer"},

	{"HPOUTL_PIN", NULL, "HPOUTL Source Mux"},
	{"HPOUTR_PIN", NULL, "HPOUTR Source Mux"},

	{"LINEOUTLP_PIN", NULL, "LINEOUTL Source Mux"},
	{"LINEOUTLN_PIN", NULL, "LINEOUTL Source Mux"},
	{"LINEOUTRP_PIN", NULL, "LINEOUTR Source Mux"},
	{"LINEOUTRN_PIN", NULL, "LINEOUTR Source Mux"},
};

static const struct snd_soc_component_driver soc_component_dev_ac101 = {
	.probe			= ac101_probe,
	.remove			= ac101_remove,
	.suspend		= ac101_suspend,
	.resume			= ac101_resume,
	.controls		= ac101_snd_controls,
	.num_controls		= ARRAY_SIZE(ac101_snd_controls),
	.dapm_widgets		= ac101_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ac101_dapm_widgets),
	.dapm_routes		= ac101_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(ac101_dapm_routes),
};

static const struct regmap_config ac101_regmap = {
	.reg_bits = 8,
	.val_bits = 16,
	.max_register = AC101_MAX_REG,

	.cache_type = REGCACHE_NONE,
};

static int ac101_set_params_from_of(struct i2c_client *i2c, struct ac101_data *pdata)
{
	struct device_node *np = i2c->dev.of_node;
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

	ret = of_property_read_u32(np, "adcl_vol", &pdata->adcl_vol);
	if (ret < 0)
		pdata->adcl_vol = 160;
	ret = of_property_read_u32(np, "adcr_vol", &pdata->adcr_vol);
	if (ret < 0)
		pdata->adcr_vol = 160;

	ret = of_property_read_u32(np, "dacl_vol", &pdata->dacl_vol);
	if (ret < 0)
		pdata->dacl_vol = 160;
	ret = of_property_read_u32(np, "dacr_vol", &pdata->dacr_vol);
	if (ret < 0)
		pdata->dacr_vol = 160;

	ret = of_property_read_u32(np, "adcl_gain", &pdata->adcl_gain);
	if (ret < 0)
		pdata->adcl_gain = 7;
	ret = of_property_read_u32(np, "adcr_gain", &pdata->adcr_gain);
	if (ret < 0)
		pdata->adcr_gain = 7;

	ret = of_property_read_u32(np, "mic1_gain", &pdata->mic1_gain);
	if (ret < 0)
		pdata->mic1_gain = 7;
	ret = of_property_read_u32(np, "mic2_gain", &pdata->mic2_gain);
	if (ret < 0)
		pdata->mic2_gain = 7;

	ret = of_property_read_u32(np, "hpout_vol", &pdata->hpout_vol);
	if (ret < 0)
		pdata->hpout_vol = 63;

	ret = of_property_read_u32(np, "spk_vol", &pdata->spk_vol);
	if (ret < 0)
		pdata->spk_vol = 31;

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

	/* irq gpio */
	jack_adv_priv->irq_gpio = of_get_named_gpio(np, "irq-gpio", 0);
	if (jack_adv_priv->irq_gpio == -EPROBE_DEFER) {
		SND_LOG_ERR("get irq-gpio failed\n");
	}

	if (!gpio_is_valid(jack_adv_priv->irq_gpio)) {
		SND_LOG_ERR("irq-gpio (%d) is invalid\n", jack_adv_priv->irq_gpio);
	}

	/* jack det gpio */
	jack_adv_priv->det_gpio = of_get_named_gpio(np, "jack-det-gpio", 0);
	if (jack_adv_priv->det_gpio == -EPROBE_DEFER) {
		SND_LOG_ERR("get jack-det-gpio failed\n");
	}

	if (!gpio_is_valid(jack_adv_priv->det_gpio)) {
		jack_adv_priv->det_gpio = 0;
		SND_LOG_ERR("jack-det-gpio (%d) is invalid\n", jack_adv_priv->det_gpio);
	}

	ret = of_property_read_u32(np, "jack-det-gpio-level", &temp_val);
	if (ret < 0) {
		jack_adv_priv->det_gpio_level = 1;
		SND_LOG_INFO("jack-det-gpio-level miss, default 1\n");
	} else {
		jack_adv_priv->det_gpio_level = temp_val;
	}

	ret = of_property_read_u32(np, "jack-det-threshold", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-det-threshold miss, default 1\n");
		jack_adv_priv->det_threshold = 1;
	} else {
		jack_adv_priv->det_threshold = temp_val;
	}

	ret = of_property_read_u32(np, "jack-key-det-threshold", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-threshold miss, default 4\n");
		jack_adv_priv->key_threshold = 10;
	} else {
		jack_adv_priv->key_threshold = temp_val;
	}

	ret = of_property_read_u32(np, "jack-det-debouce-time", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-det-debouce-time miss, default 0\n");
		jack_adv_priv->det_debounce = 0;
	} else {
		jack_adv_priv->det_debounce = temp_val;
	}

	ret = of_property_read_u32(np, "jack-key-det-debouce-time", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-key-det-debouce-time miss, default 0\n");
		jack_adv_priv->key_debounce = 0;
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

	SND_LOG_ERR("irq_gpio        -> %u\n", jack_adv_priv->irq_gpio);
	SND_LOG_ERR("det_gpio        -> %u\n", jack_adv_priv->det_gpio);
	SND_LOG_DEBUG("jack-det-threshold        -> %u\n",
		      jack_adv_priv->det_threshold);
	SND_LOG_DEBUG("jack-key-det-threshold    -> %u\n",
		      jack_adv_priv->key_threshold);
	SND_LOG_DEBUG("jack-det-debouce-time -> %u\n",
		      jack_adv_priv->det_debounce);
	SND_LOG_DEBUG("jack-det-key-debouce-time -> %u\n",
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
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int ac101_i2c_probe(struct i2c_client *i2c)
#else
static int ac101_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
#endif
{
	struct ac101_data *pdata = dev_get_platdata(&i2c->dev);
	struct platform_device *pdev = container_of(&i2c->dev, struct platform_device, dev);
	struct ac101_priv *ac101;
	int ret;

	SND_LOG_INFO("\n");

	ac101 = devm_kzalloc(&i2c->dev, sizeof(*ac101), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ac101)) {
		dev_err(&i2c->dev, "Unable to allocate ac101 private data\n");
		return -ENOMEM;
	}

	ac101->dev = &i2c->dev;

	ac101->regmap = devm_regmap_init_i2c(i2c, &ac101_regmap);
	if (IS_ERR(ac101->regmap))
		return PTR_ERR(ac101->regmap);

	if (pdata)
		memcpy(&ac101->pdata, pdata, sizeof(struct ac101_data));
	else if (i2c->dev.of_node) {
		ret = ac101_set_params_from_of(i2c, &ac101->pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "ac101_set_params_from_of failed\n");
			return -1;
		}
	} else
		dev_err(&i2c->dev, "Unable to allocate ac101 private data\n");

	ac101->rglt = snd_sunxi_regulator_init(&i2c->dev);

	ac101->pdata.jack_adv_priv.pa_cfg = snd_sunxi_pa_pin_init(pdev, &ac101->pdata.jack_adv_priv.pa_pin_max);

	i2c_set_clientdata(i2c, ac101);

	ret = devm_snd_soc_register_component(&i2c->dev,
					      &soc_component_dev_ac101,
					      &ac101_dai, 1);
	if (ret < 0)
		dev_err(&i2c->dev, "register ac101 codec failed: %d\n", ret);

	return ret;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static void ac101_i2c_remove(struct i2c_client *i2c)
#else
static int ac101_i2c_remove(struct i2c_client *i2c)
#endif
{
	struct device *dev = &i2c->dev;
	struct device_node *np = i2c->dev.of_node;
	struct ac101_priv *ac101 = dev_get_drvdata(dev);

	snd_sunxi_regulator_exit(ac101->rglt);

	devm_kfree(dev, ac101);
	of_node_put(np);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	return;
#else
	return 0;
#endif
}

static const struct of_device_id ac101_of_match[] = {
	{ .compatible = "allwinner,sunxi-ac101", },
	{ }
};
MODULE_DEVICE_TABLE(of, ac101_of_match);

static struct i2c_driver ac101_i2c_driver = {
	.driver = {
		.name = "sunxi-ac101",
		.of_match_table = ac101_of_match,
	},
	.probe = ac101_i2c_probe,
	.remove = ac101_i2c_remove,
};

module_i2c_driver(ac101_i2c_driver);

MODULE_DESCRIPTION("ASoC AC101 driver");
MODULE_AUTHOR("lijingpsw@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.5");

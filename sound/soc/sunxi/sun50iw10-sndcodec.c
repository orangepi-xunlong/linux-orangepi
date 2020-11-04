/*
 * sound\soc\sunxi\sun50iw10-sndcodec.c
 * (C) Copyright 2014-2018
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 * liushaohua <liushaohua@allwinnertech.com>
 * yumingfengng <yumingfeng@allwinnertech.com>
 * luguofang <luguofang@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/input.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <linux/of.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/delay.h>

#include "sun50iw10-codec.h"
#include "sunxi_rw_func.h"

#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
static int mdata_threshold = 0x10;
module_param(mdata_threshold, int, 0644);
MODULE_PARM_DESC(mdata_threshold,
		"SUNXI hmic data threshold");

typedef enum {
	RESUME_IRQ  = 0x0,
	SYSINIT_IRQ = 0x1,
	OTHER_IRQ   = 0x2,
} _jack_irq_times;

enum HPDETECTWAY {
	HP_DETECT_LOW = 0x0,
	HP_DETECT_HIGH = 0x1,
};

enum dectect_jack {
	PLUG_OUT = 0x0,
	PLUG_IN  = 0x1,
};

static bool is_irq;
static int switch_state;
#endif

struct sunxi_card_priv {
	struct snd_soc_card *card;
#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
	struct snd_soc_codec *codec;
	struct delayed_work hs_init_work;
	struct delayed_work hs_detect_work;
	struct delayed_work hs_button_work;
	struct delayed_work hs_checkplug_work;
	struct mutex jack_mlock;
	struct snd_soc_jack jack;
	struct timeval tv_headset_plugin;	/*4*/
	_jack_irq_times jack_irq_times;
	u32 detect_state;
	u32 jackirq;				/*switch irq*/
	u32 HEADSET_DATA;			/*threshod for switch insert*/
	u32 headset_basedata;
	u32 switch_status;
	u32 key_volup;
	u32 key_voldown;
	u32 key_hook;
	u32 key_voiceassist;
	u32 hp_detect_case;
#endif
};

#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
/*
 * Identify the jack type as Headset/Headphone/None
 */
static int sunxi_check_jack_type(struct snd_soc_jack *jack)
{
	u32 reg_val = 0;
	u32 jack_type = 0, tempdata = 0;
	struct sunxi_card_priv *priv = container_of(jack, struct sunxi_card_priv, jack);

	reg_val = snd_soc_read(priv->codec, SUNXI_HMIC_STS);
	tempdata = (reg_val >> HMIC_DATA) & 0x1f;

	priv->headset_basedata = tempdata;
	if (tempdata >= priv->HEADSET_DATA) {
		pr_debug("[SND_JACK_HEADSET], priv->HEADSET_DATA:0x%x\n",
		       priv->HEADSET_DATA);
		/*
		 * headset:4
		 */
		jack_type = SND_JACK_HEADSET;
	} else {
		/*
		 * headphone:3
		 * disable hbias and adc
		 */
		pr_debug("[SND_JACK_HEADPHONE] priv->HEADSET_DATA:0x%x\n",
			priv->HEADSET_DATA);
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				(0x1 << HMICBIASEN), (0x0 << HMICBIASEN));
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				(0x1 << MICADCEN), (0x0 << MICADCEN));
		jack_type = SND_JACK_HEADPHONE;
	}

	return jack_type;
}

/* Checks hs insertion by mdet */
static void sunxi_check_hs_plug(struct work_struct *work)
{
	struct sunxi_card_priv *priv =
		container_of(work, struct sunxi_card_priv, hs_checkplug_work.work);
//	struct sunxi_codec *sunxi_codec =
//				snd_soc_codec_get_drvdata(priv->codec);

	mutex_lock(&priv->jack_mlock);
	if (priv->detect_state != PLUG_IN) {
		/* Enable MDET */
		snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x1 << MIC_DET_IRQ_EN));
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				(0x1 << MICADCEN),
				(0x1 << MICADCEN));
		/* Enable PA */
		snd_soc_update_bits(priv->codec, SUNXI_HP_REG,
				(0x1 << HPPA_EN),
				(0x1 << HPPA_EN));

//		if (sunxi_codec->spk_gpio.cfg)
//			gpio_set_value(sunxi_codec->spk_gpio.gpio, 1);
	}
	mutex_unlock(&priv->jack_mlock);
}

/* Checks jack insertion and identifies the jack type.*/
static void sunxi_check_hs_detect_status(struct work_struct *work)
{
	struct sunxi_card_priv *priv =
		container_of(work, struct sunxi_card_priv, hs_detect_work.work);
	int jack_type = 0, reg_val = 0;

	mutex_lock(&priv->jack_mlock);
	if (priv->detect_state == PLUG_IN) {
		/* Enable hbias and adc*/
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				(0x1 << MICADCEN), (0x1 << MICADCEN));
		msleep(100);
		jack_type = sunxi_check_jack_type(&priv->jack);
		if (jack_type != priv->switch_status) {
			priv->switch_status = jack_type;
			snd_jack_report(priv->jack.jack, jack_type);
			pr_err("plugin --> switch:%d\n", jack_type);
			switch_state = jack_type;
		}

		/*
		 * if SND_JACK_HEADSET,enable mic detect irq
		 */
		if (jack_type == SND_JACK_HEADSET) {
			/*
			 * headset:clear headset insert pending.
			 */
			reg_val = snd_soc_read(priv->codec, SUNXI_HMIC_STS);
			priv->headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;
			if (priv->headset_basedata > 3)
				priv->headset_basedata -= 3;

			do_gettimeofday(&priv->tv_headset_plugin);
			usleep_range(1000, 2000);
			snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
				(0x1f << MDATA_THRESHOLD),
				(priv->headset_basedata << MDATA_THRESHOLD));
			snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x1 << MIC_DET_IRQ_EN));
		} else if (jack_type == SND_JACK_HEADPHONE) {
			/*
			 * if is HEADPHONE 3,close mic detect irq
			 */
			snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x0 << MIC_DET_IRQ_EN));
			priv->tv_headset_plugin.tv_sec = 0;
		}
	} else {
		priv->switch_status = 0;
		/*clear headset pulgout pending.*/
		snd_jack_report(priv->jack.jack, priv->switch_status);
		switch_state = priv->switch_status;
		pr_err("plugout --> switch:%d\n", priv->switch_status);
		priv->tv_headset_plugin.tv_sec = 0;
		/*enable hbias and adc*/
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				    (0x1 << HMICBIASEN), (0x1 << HMICBIASEN));
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				    (0x1 << MICADCEN), (0x1 << MICADCEN));
		snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
				(0x1f << MDATA_THRESHOLD),
				(mdata_threshold << MDATA_THRESHOLD));
		snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
				    (0x1 << MIC_DET_IRQ_EN),
				    (0x1 << MIC_DET_IRQ_EN));
	}
	mutex_unlock(&priv->jack_mlock);
}

static void sunxi_hs_init_work(struct work_struct *work)
{
	struct sunxi_card_priv *priv =
	    container_of(work, struct sunxi_card_priv, hs_init_work.work);

	mutex_lock(&priv->jack_mlock);
	if (is_irq == true) {
		is_irq = false;
	} else {
		if (priv->hp_detect_case == HP_DETECT_LOW ||
			(priv->jack_irq_times == RESUME_IRQ)) {
			/*
			 * It should be report after resume.
			 * If the headset plugout after suspend, the system
			 * can not know the state, so we should reset here
			 * when resume.
			 */
			pr_debug("[codec-machine] resume-->report switch.\n");
			priv->switch_status = 0;
			snd_jack_report(priv->jack.jack, priv->switch_status);
			switch_state = 0;
		}
	}
	priv->jack_irq_times = OTHER_IRQ;
	mutex_unlock(&priv->jack_mlock);
}

/* Check for hook release */
static void sunxi_check_hs_button_status(struct work_struct *work)
{
	struct sunxi_card_priv *priv =
		container_of(work, struct sunxi_card_priv, hs_button_work.work);
	u32 i = 0;

	mutex_lock(&priv->jack_mlock);
	for (i = 0; i < 1; i++) {
		if (priv->key_hook == 0) {
			pr_info("Hook (2)!!\n");
			priv->switch_status &= ~SND_JACK_BTN_0;
			snd_jack_report(priv->jack.jack, priv->switch_status);
			break;
		}
		/* may msleep 8 */
		msleep(20);
	}

	mutex_unlock(&priv->jack_mlock);
}

static irqreturn_t jack_interrupt(int irq, void *dev_id)
{
	struct sunxi_card_priv *priv = dev_id;
	struct timeval tv;
	u32 tempdata = 0, regval = 0;
	int jack_state = 0;
//	struct sunxi_codec *sunxi_codec =
//				snd_soc_codec_get_drvdata(priv->codec);

	if (priv->jack_irq_times == RESUME_IRQ ||
	    priv->jack_irq_times == SYSINIT_IRQ) {
		is_irq = true;
		priv->jack_irq_times = OTHER_IRQ;
	}

	pr_debug("[%s] SUNXI_HMIC_STS:0x%x\n", __func__,
		snd_soc_read(priv->codec, SUNXI_HMIC_STS));

	jack_state = snd_soc_read(priv->codec, SUNXI_HMIC_STS);

	if (priv->detect_state != PLUG_IN) {
		if (jack_state & (1 << MIC_DET_ST)) {
			regval = snd_soc_read(priv->codec, SUNXI_HMIC_CTRL);
			regval &= ~(0x1 << MIC_DET_IRQ_EN);
			snd_soc_write(priv->codec, SUNXI_HMIC_CTRL, regval);

			regval = snd_soc_read(priv->codec, SUNXI_MICBIAS_REG);
			regval &= ~(0x1 << MICADCEN);
			snd_soc_write(priv->codec, SUNXI_MICBIAS_REG, regval);

			/* clear mic detect status */
			regval = snd_soc_read(priv->codec, SUNXI_HMIC_STS);
			regval &= ~(0x1 << JACK_DET_IIN_ST);
			regval &= ~(0x1 << JACK_DET_OUT_ST);
			regval |= 0x1 << MIC_DET_ST;
			snd_soc_write(priv->codec, SUNXI_HMIC_STS, regval);

			/* close PA */
			regval = snd_soc_read(priv->codec, SUNXI_HP_REG);
			regval &= ~(0x1 << HPPA_EN);
			snd_soc_write(priv->codec, SUNXI_HP_REG, regval);

//			if (sunxi_codec->spk_gpio.cfg)
//				gpio_set_value(sunxi_codec->spk_gpio.gpio, 0);

			/* prevent mic detect false trigger */
			schedule_delayed_work(&priv->hs_checkplug_work,
				msecs_to_jiffies(700));
		}
	}

	/*headphone insert*/
	if (jack_state & (1 << JACK_DET_IIN_ST)) {
		regval = snd_soc_read(priv->codec, SUNXI_HMIC_STS);
		regval |= 0x1 << JACK_DET_IIN_ST;
		regval &= ~(0x1 << JACK_DET_OUT_ST);
		snd_soc_write(priv->codec, SUNXI_HMIC_STS, regval);
		priv->detect_state = PLUG_IN;
		schedule_delayed_work(&priv->hs_detect_work,
				msecs_to_jiffies(10));
	}

	/*headphone plugout*/
	if (jack_state & (1 << JACK_DET_OUT_ST)) {
		regval = snd_soc_read(priv->codec, SUNXI_HMIC_STS);
		regval &= ~(0x1 << JACK_DET_IIN_ST);
		regval |= 0x1 << JACK_DET_OUT_ST;
		snd_soc_write(priv->codec, SUNXI_HMIC_STS, regval);
		priv->detect_state = PLUG_OUT;
		schedule_delayed_work(&priv->hs_detect_work,
				msecs_to_jiffies(10));
	}

	/*key*/
	if ((priv->detect_state == PLUG_IN) &&
		(jack_state & (1 << MIC_DET_ST))) {
		regval = snd_soc_read(priv->codec, SUNXI_HMIC_STS);
		regval &= ~(0x1 << JACK_DET_IIN_ST);
		regval &= ~(0x1 << JACK_DET_OUT_ST);
		regval |= 0x1 << MIC_DET_ST;
		snd_soc_write(priv->codec, SUNXI_HMIC_STS, regval);

		do_gettimeofday(&tv);
		if (abs(tv.tv_sec - priv->tv_headset_plugin.tv_sec) > 2) {
			tempdata = snd_soc_read(priv->codec, SUNXI_HMIC_STS);
			tempdata = (tempdata & 0x1f00) >> 8;
			pr_err("[%s]: KEY tempdata: %d\n", __func__, tempdata);
			pr_debug("headset key debug tempdata : 0x%x.\n",
				 tempdata);

			if (tempdata == 2) {
				priv->key_hook = 0;
				priv->key_voldown = 0;
				priv->key_voiceassist = 0;
				priv->key_volup++;
				if (priv->key_volup == 1) {
					pr_debug("Volume ++\n");
					priv->key_volup = 0;
					priv->switch_status |= SND_JACK_BTN_1;
					snd_jack_report(priv->jack.jack,
							priv->switch_status);
					priv->switch_status &= ~SND_JACK_BTN_1;
					snd_jack_report(priv->jack.jack,
							priv->switch_status);
				}
			} else if ((tempdata == 5) || tempdata == 4) {
				priv->key_volup = 0;
				priv->key_hook = 0;
				priv->key_voiceassist = 0;
				priv->key_voldown++;
				if (priv->key_voldown == 1) {
					pr_debug("Volume --\n");
					priv->key_voldown = 0;
					priv->switch_status |= SND_JACK_BTN_2;
					snd_jack_report(priv->jack.jack,
							priv->switch_status);
					priv->switch_status &= ~SND_JACK_BTN_2;
					snd_jack_report(priv->jack.jack,
							priv->switch_status);
				}
			/* add KET_VOICECOMMAND for voice assistant */
			} else if (tempdata == 1) {
				priv->key_volup = 0;
				priv->key_hook = 0;
				priv->key_voldown = 0;
				priv->key_voiceassist++;
				if (priv->key_voiceassist == 1) {
					pr_debug("Voice Assistant Open\n");
					priv->key_voiceassist = 0;
					priv->switch_status |= SND_JACK_BTN_3;
					snd_jack_report(priv->jack.jack,
							priv->switch_status);
					priv->switch_status &= ~SND_JACK_BTN_3;
					snd_jack_report(priv->jack.jack,
							priv->switch_status);
				}
			} else if (tempdata == 0x0) {
				priv->key_volup = 0;
				priv->key_voldown = 0;
				priv->key_voiceassist = 0;
				priv->key_hook++;
				if (priv->key_hook >= 1) {
					priv->key_hook = 0;
					if ((priv->switch_status &
					     SND_JACK_BTN_0) == 0) {
						priv->switch_status |=
						    SND_JACK_BTN_0;
						snd_jack_report(
						    priv->jack.jack,
						    priv->switch_status);
						pr_debug("Hook (1)!!\n");
					}
					schedule_delayed_work(
					    &priv->hs_button_work,
					    msecs_to_jiffies(180));
				}
			} else {
				/*This could be key release,fix me ! */
				pr_err("tempdata:0x%x,Key data err:\n",
					 tempdata);
				priv->key_volup = 0;
				priv->key_voldown = 0;
				priv->key_hook = 0;
				priv->key_voiceassist = 0;
			}
		} else {
		}
	}

	return IRQ_HANDLED;
}
#endif

static const struct snd_kcontrol_new sunxi_card_controls[] = {
	SOC_DAPM_PIN_SWITCH("External Speaker"),
//	SOC_DAPM_PIN_SWITCH("Speaker"),
	SOC_DAPM_PIN_SWITCH("Headphone"),
};

static const struct snd_soc_dapm_widget sunxi_card_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Main Mic", NULL),
	SND_SOC_DAPM_MIC("HeadphoneMic", NULL),
};

static const struct snd_soc_dapm_route sunxi_card_routes[] = {
	{"MainMic Bias", NULL, "Main Mic"},
	{"External Speaker", NULL, "LINEOUT"},
};

/*
 * Card initialization
 */
static int sunxi_card_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->component.dapm;

#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	int ret;
	priv->codec = rtd->codec;

	ret = snd_soc_card_jack_new(rtd->card, "sunxi Audio Jack",
			       SND_JACK_HEADSET | SND_JACK_HEADPHONE |
				   SND_JACK_BTN_0 | SND_JACK_BTN_1 |
				   SND_JACK_BTN_2 | SND_JACK_BTN_3,
			       &priv->jack, NULL, 0);
	if (ret) {
		pr_err("jack creation failed\n");
		return ret;
	}

	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);
	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_3, KEY_VOICECOMMAND);
#endif

	snd_soc_dapm_disable_pin(dapm, "HPOUTR");
	snd_soc_dapm_disable_pin(dapm, "HPOUTL");

	snd_soc_dapm_disable_pin(&codec->component.dapm, "External Speaker");
	snd_soc_dapm_enable_pin(&codec->component.dapm, "LINEOUT");

	snd_soc_dapm_disable_pin(&rtd->card->dapm, "Headphone");
//	snd_soc_dapm_disable_pin(&rtd->card->dapm, "Speaker");

	snd_soc_dapm_sync(dapm);

	pr_warn("[%s] card init finished.\n", __func__);
	return 0;
}

static int sunxi_card_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_card *card = rtd->card;
	unsigned int freq;
	int ret;

	switch (params_rate(params)) {
	case	8000:
	case	12000:
	case	16000:
	case	24000:
	case	32000:
	case	48000:
	case	96000:
	case	192000:
		freq = 24576000;
		break;
	case	11025:
	case	22050:
	case	44100:
		freq = 22579200;
		break;
	default:
		dev_err(card->dev, "invalid rate setting\n");
		return -EINVAL;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, 0, freq, 0);
	if (ret < 0) {
		dev_err(card->dev, "set codec dai sysclk faided.\n");
		return ret;
	}

	return 0;
}

static struct snd_soc_ops sunxi_card_ops = {
	.hw_params = sunxi_card_hw_params,
};

static struct snd_soc_dai_link sunxi_card_dai_link[] = {
	{
		.name		= "sun50iw10-codec",
		.stream_name	= "SUNXI-CODEC",
		.codec_name	= "codec",
		.codec_dai_name = "sun50iw10codec",
		.cpu_dai_name	= "codec",
		.platform_name	= "codec",
		.init		= sunxi_card_init,
		.ops		= &sunxi_card_ops,
	},
};

#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
static void sunxi_hs_reg_init(struct sunxi_card_priv *priv)
{
	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL, (0xffff << 0),
			    (0x0 << 0));
	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
			(0x1f << MDATA_THRESHOLD),
			(0x17 << MDATA_THRESHOLD));

	snd_soc_update_bits(priv->codec, SUNXI_HMIC_STS, (0xffff << 0),
			    (0x6000 << 0));
	snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
			(0xff << SELDETADCBF), (0x40 << SELDETADCBF));

	if (priv->hp_detect_case == HP_DETECT_LOW) {
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				    (0x1 << AUTOPLEN), (0x1 << AUTOPLEN));
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				    (0x1 << DETMODE), (0x0 << DETMODE));
	} else {
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				    (0x1 << AUTOPLEN), (0x0 << AUTOPLEN));
		snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				    (0x1 << DETMODE), (0x1 << DETMODE));
	}
	snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG, (0x1 << JACKDETEN),
			    (0x1 << JACKDETEN));
	/*enable jack in /out irq*/
	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_IN_IRQ_EN), (0x1 << JACK_IN_IRQ_EN));
	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_OUT_IRQ_EN), (0x1 << JACK_OUT_IRQ_EN));

	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
			(0xf << HMIC_N), (HP_DEBOUCE_TIME << HMIC_N));

	snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
			    (0x1 << HMICBIASEN), (0x1 << HMICBIASEN));
	snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
			    (0x1 << MICADCEN), (0x1 << MICADCEN));
	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x1 << MIC_DET_IRQ_EN));

	schedule_delayed_work(&priv->hs_init_work, msecs_to_jiffies(10));
}

static void snd_sunxi_unregister_jack(struct sunxi_card_priv *priv)
{
	/*
	 * Set process button events to false so that the button
	 * delayed work will not be scheduled.
	 */
	cancel_delayed_work_sync(&priv->hs_detect_work);
	cancel_delayed_work_sync(&priv->hs_button_work);
	cancel_delayed_work_sync(&priv->hs_init_work);
}
#endif

static int sunxi_card_suspend(struct snd_soc_card *card)
{
#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(card);

	disable_irq(priv->jackirq);

	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
			    (0x1 << MIC_DET_IRQ_EN), (0x0 << MIC_DET_IRQ_EN));
	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_IN_IRQ_EN), (0x0 << JACK_IN_IRQ_EN));
	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_OUT_IRQ_EN), (0x0 << JACK_OUT_IRQ_EN));
	snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG, (0x1 << JACKDETEN),
			    (0x0 << JACKDETEN));
#endif
	pr_debug("[codec-machine]  suspend.\n");

	return 0;
}

static int sunxi_card_resume(struct snd_soc_card *card)
{
#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(card);

	enable_irq(priv->jackirq);
	priv->jack_irq_times = RESUME_IRQ;
	priv->detect_state = PLUG_OUT;/*todo..?*/
	sunxi_hs_reg_init(priv);
#endif
	pr_debug("[codec-machine]  resume.\n");

	return 0;
}

static struct snd_soc_card snd_soc_sunxi_card = {
	.name			= "sun50iw10-codec",
	.owner			= THIS_MODULE,
	.dai_link		= sunxi_card_dai_link,
	.num_links		= ARRAY_SIZE(sunxi_card_dai_link),
	.controls		= sunxi_card_controls,
	.num_controls		= ARRAY_SIZE(sunxi_card_controls),
	.dapm_widgets		= sunxi_card_dapm_widgets,
	.num_dapm_widgets 	= ARRAY_SIZE(sunxi_card_dapm_widgets),
	.dapm_routes 		= sunxi_card_routes,
	.num_dapm_routes 	= ARRAY_SIZE(sunxi_card_routes),
	.suspend_post		= sunxi_card_suspend,
	.resume_post		= sunxi_card_resume,

};

static int sunxi_card_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	u32 temp_val;
	struct sunxi_card_priv *priv = NULL;
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &snd_soc_sunxi_card;

	if (!np) {
		dev_err(&pdev->dev, "can not get dt node for this device.\n");
		return -EINVAL;
	}

	/* register the soc card */
	card->dev = &pdev->dev;

	priv = devm_kzalloc(&pdev->dev,
		sizeof(struct sunxi_card_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "devm_kzalloc failed %d\n", ret);
		return -ENOMEM;
	}
	priv->card = card;

	snd_soc_card_set_drvdata(card, priv);
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
		goto err_devm_kfree;
	}

#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
	ret = of_property_read_u32(np, "hp_detect_case", &temp_val);
	if (ret < 0)
		pr_err("[audio] hp_detect_case  missing or invalid.\n");
	else
		priv->hp_detect_case = temp_val;

	priv->jackirq = platform_get_irq(pdev, 0);
	if (priv->jackirq < 0) {
		pr_err("[audio] irq failed\n");
		ret = -ENODEV;
	}

	priv->jack_irq_times = SYSINIT_IRQ;

	/*
	 * initial the parameters for judge switch state
	 */
	priv->HEADSET_DATA = 0x10;
	priv->detect_state = PLUG_OUT;
	INIT_DELAYED_WORK(&priv->hs_detect_work, sunxi_check_hs_detect_status);
	INIT_DELAYED_WORK(&priv->hs_button_work, sunxi_check_hs_button_status);
	INIT_DELAYED_WORK(&priv->hs_init_work, sunxi_hs_init_work);
	INIT_DELAYED_WORK(&priv->hs_checkplug_work, sunxi_check_hs_plug);
	mutex_init(&priv->jack_mlock);

	ret = request_irq(priv->jackirq, jack_interrupt,
		   0, "audio jack irq", priv);

	sunxi_hs_reg_init(priv);
	pr_debug("[%s] 0x310:0x%X,0x314:0x%X,0x318:0x%X,0x1C:0x%X,0x1D:0x%X\n",
			__func__, snd_soc_read(priv->codec, 0x310),
			snd_soc_read(priv->codec, 0x314),
			snd_soc_read(priv->codec, 0x318),
			snd_soc_read(priv->codec, 0x1C),
			snd_soc_read(priv->codec, 0x1D));
#endif

	dev_warn(&pdev->dev, "[%s] register card finished.\n", __func__);

	return 0;

err_devm_kfree:
	devm_kfree(&pdev->dev, priv);
	return ret;
}

static int __exit sunxi_card_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(card);

#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_IN_IRQ_EN), (0x0 << JACK_IN_IRQ_EN));
	snd_soc_update_bits(priv->codec, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_OUT_IRQ_EN), (0x0 << JACK_OUT_IRQ_EN));
	snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG, (0x1 << JACKDETEN),
			    (0x0 << JACKDETEN));
	snd_soc_update_bits(priv->codec, SUNXI_MICBIAS_REG,
				(0x1 << HMICBIASEN), (0x0 << HMICBIASEN));
	snd_sunxi_unregister_jack(priv);
#endif

	snd_soc_unregister_card(card);
	devm_kfree(&pdev->dev, priv);

	dev_warn(&pdev->dev, "[%s] unregister card finished.\n", __func__);

	return 0;
}

static const struct of_device_id sunxi_card_of_match[] = {
	{ .compatible = "allwinner,sunxi-codec-machine", },
	{},
};

static struct platform_driver sunxi_machine_driver = {
	.driver = {
		.name = "sunxi-codec-machine",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = sunxi_card_of_match,
	},
	.probe = sunxi_card_dev_probe,
	.remove = __exit_p(sunxi_card_dev_remove),
};

module_platform_driver(sunxi_machine_driver);
#ifdef CONFIG_SND_SUNXI_SOC_SUN50IW10_JACK
module_param_named(switch_state, switch_state, int, S_IRUGO | S_IWUSR);
#endif

MODULE_AUTHOR("luguofang <luguofang@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI Codec Machine ASoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-codec-machine");

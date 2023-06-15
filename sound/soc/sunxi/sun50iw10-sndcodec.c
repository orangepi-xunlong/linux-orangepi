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
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/extcon.h>

#include "sunxi_sound_log.h"
#include "sun50iw10-codec.h"

/* #define CONFIG_EXTCON_TYPEC_JACK */

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

#ifdef CONFIG_EXTCON_TYPEC_JACK
struct typec_jack_cfg {
	u32 pin;
	bool used;
	bool level;
};
#endif

struct sunxi_card_priv {
	struct snd_soc_card *card;
	struct snd_soc_component *component;
	struct delayed_work hs_init_work;
	struct delayed_work hs_detect_work;
	struct delayed_work hs_button_work;
	struct delayed_work hs_checkplug_work;
	struct mutex jack_mlock;
	struct snd_soc_jack jack;
#ifdef CONFIG_EXTCON_TYPEC_JACK
	u32 typec_analog_jack_enable;
	struct extcon_dev *extdev;
	struct notifier_block hp_nb;
	struct delayed_work typec_jack_detect_work;
	struct typec_jack_cfg usb_sel;
	struct typec_jack_cfg usb_noe;
	struct typec_jack_cfg mic_sel;
#endif
	struct timespec64 tv_headset_plugin;	/*4*/
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
};

/*
 * Identify the jack type as Headset/Headphone/None
 */
static int sunxi_check_jack_type(struct snd_soc_jack *jack)
{
	u32 reg_val = 0;
	u32 jack_type = 0, tempdata = 0;
	struct sunxi_card_priv *priv = container_of(jack, struct sunxi_card_priv, jack);

	reg_val = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
	tempdata = (reg_val >> HMIC_DATA) & 0x1f;

	priv->headset_basedata = tempdata;
	SOUND_LOG_DEBUG("headset_basedata-> 0x%x\n", priv->headset_basedata);
	if (tempdata >= priv->HEADSET_DATA) {
		/*
		 * headset:4
		 */
		jack_type = SND_JACK_HEADSET;
	} else {
		/*
		 * headphone:3
		 * disable hbias and adc
		 */
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				(0x1 << HMICBIASEN), (0x0 << HMICBIASEN));
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
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
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(priv->component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);

	mutex_lock(&priv->jack_mlock);
	if (priv->detect_state != PLUG_IN) {
		/* Enable MDET */
		snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x1 << MIC_DET_IRQ_EN));
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				(0x1 << MICADCEN),
				(0x1 << MICADCEN));
		/* Enable PA */
		snd_soc_component_update_bits(priv->component, SUNXI_HEADPHONE_REG,
				(0x1 << HPPA_EN),
				(0x1 << HPPA_EN));

		if (spk_cfg->used)
			gpio_set_value(spk_cfg->spk_gpio, spk_cfg->pa_level);
	} else {
		/*
		 * Enable HPPA_EN
		 * FIXME:When the Audio HAL is not at the do_output_standby,
		 * apk not play the music at the same time, we can insert
		 * headset now and click to play music immediately in the apk,
		 * the Audio HAL will write data to the card and not update
		 * the stream routing. Because we also set mute when
		 * the mdet come into force, so that the dapm will not update
		 * and it makes the mute.
		 */
		snd_soc_component_update_bits(priv->component, SUNXI_HEADPHONE_REG,
				(0x1 << HPPA_EN),
				(0x1 << HPPA_EN));
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
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				(0x1 << MICADCEN), (0x1 << MICADCEN));
		msleep(100);
		jack_type = sunxi_check_jack_type(&priv->jack);
		if (jack_type != priv->switch_status) {
			priv->switch_status = jack_type;
			snd_jack_report(priv->jack.jack, jack_type);
			SOUND_LOG_INFO("plugin --> switch:%d\n", jack_type);
			switch_state = jack_type;
		}

		/*
		 * if SND_JACK_HEADSET,enable mic detect irq
		 */
		if (jack_type == SND_JACK_HEADSET) {
			/*
			 * headset:clear headset insert pending.
			 */
			reg_val = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
			priv->headset_basedata = (reg_val >> HMIC_DATA) & 0x1f;
			if (priv->headset_basedata > 3)
				priv->headset_basedata -= 3;

			usleep_range(1000, 2000);
			snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
				(0x1f << MDATA_THRESHOLD),
				(priv->headset_basedata << MDATA_THRESHOLD));
			snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x1 << MIC_DET_IRQ_EN));
		} else if (jack_type == SND_JACK_HEADPHONE) {
			/*
			 * if is HEADPHONE 3,close mic detect irq
			 */
			snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
				(0x1 << MIC_DET_IRQ_EN),
				(0x0 << MIC_DET_IRQ_EN));
		}
	} else {
		priv->switch_status = 0;
		/*clear headset pulgout pending.*/
		snd_jack_report(priv->jack.jack, priv->switch_status);
		switch_state = priv->switch_status;
		SOUND_LOG_INFO("plugout --> switch:%d\n", priv->switch_status);
		/*enable hbias and adc*/
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				    (0x1 << HMICBIASEN), (0x1 << HMICBIASEN));
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				    (0x1 << MICADCEN), (0x1 << MICADCEN));
		snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
				(0x1f << MDATA_THRESHOLD),
				(mdata_threshold << MDATA_THRESHOLD));
		snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
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
			SOUND_LOG_DEBUG("resume-->report switch\n");
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
			SOUND_LOG_INFO("Hook (2)!!\n");
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
	struct timespec64 tv;
	u32 tempdata = 0, regval = 0;
	int jack_state = 0;
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(priv->component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);

	if (priv->jack_irq_times == RESUME_IRQ ||
	    priv->jack_irq_times == SYSINIT_IRQ) {
		is_irq = true;
		priv->jack_irq_times = OTHER_IRQ;
	}

	SOUND_LOG_DEBUG("SUNXI_HMIC_STS:0x%x\n", snd_soc_component_read32(priv->component, SUNXI_HMIC_STS));

	jack_state = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);

	if (priv->detect_state != PLUG_IN) {
		/* when headphone half-insertion, MIC_DET IRQ will be trigger. */
		if (jack_state & (1 << MIC_DET_ST)) {
			regval = snd_soc_component_read32(priv->component, SUNXI_HMIC_CTRL);
			regval &= ~(0x1 << MIC_DET_IRQ_EN);
			snd_soc_component_write(priv->component, SUNXI_HMIC_CTRL, regval);

			regval = snd_soc_component_read32(priv->component, SUNXI_MICBIAS_REG);
			regval &= ~(0x1 << MICADCEN);
			snd_soc_component_write(priv->component, SUNXI_MICBIAS_REG, regval);

			/* clear mic detect status */
			regval = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
			regval &= ~(0x1 << JACK_DET_IIN_ST);
			regval &= ~(0x1 << JACK_DET_OUT_ST);
			regval |= 0x1 << MIC_DET_ST;
			snd_soc_component_write(priv->component, SUNXI_HMIC_STS, regval);

			/* close PA */
			regval = snd_soc_component_read32(priv->component, SUNXI_HEADPHONE_REG);
			regval &= ~(0x1 << HPPA_EN);
			snd_soc_component_write(priv->component, SUNXI_HEADPHONE_REG, regval);

			if (spk_cfg->used)
				gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));
			/* prevent mic detect false trigger */
			schedule_delayed_work(&priv->hs_checkplug_work,
				msecs_to_jiffies(700));
		}
	}

	/*headphone insert*/
	if (jack_state & (1 << JACK_DET_IIN_ST)) {
		regval = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
		regval |= 0x1 << JACK_DET_IIN_ST;
		regval &= ~(0x1 << JACK_DET_OUT_ST);
		snd_soc_component_write(priv->component, SUNXI_HMIC_STS, regval);
		priv->detect_state = PLUG_IN;
		/* hs_checkplug_work will set spk gpio. some time, it will set
		 * spk gpio output befor headphone insert, but tinymix
		 * spk_switch conctrl is OFF now. So, we need to close spk gpio.*/
		if (spk_cfg->used)
			gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));
		ktime_get_real_ts64(&priv->tv_headset_plugin);
		schedule_delayed_work(&priv->hs_detect_work,
				msecs_to_jiffies(10));
	}

	/*headphone plugout*/
	if (jack_state & (1 << JACK_DET_OUT_ST)) {
		regval = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
		regval &= ~(0x1 << JACK_DET_IIN_ST);
		regval |= 0x1 << JACK_DET_OUT_ST;
		snd_soc_component_write(priv->component, SUNXI_HMIC_STS, regval);
		priv->detect_state = PLUG_OUT;
		schedule_delayed_work(&priv->hs_detect_work,
				msecs_to_jiffies(10));
	}

	/*key*/
	if ((priv->detect_state == PLUG_IN) &&
		(jack_state & (1 << MIC_DET_ST))) {
		regval = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
		regval &= ~(0x1 << JACK_DET_IIN_ST);
		regval &= ~(0x1 << JACK_DET_OUT_ST);
		regval |= 0x1 << MIC_DET_ST;
		snd_soc_component_write(priv->component, SUNXI_HMIC_STS, regval);

		ktime_get_real_ts64(&tv);
		if (abs(tv.tv_sec - priv->tv_headset_plugin.tv_sec) > 1) {
			tempdata = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
			tempdata = (tempdata & 0x1f00) >> 8;
			SOUND_LOG_DEBUG("KEY tempdata: %d\n", tempdata);

			if (tempdata == 2) {
				priv->key_hook = 0;
				priv->key_voldown = 0;
				priv->key_voiceassist = 0;
				priv->key_volup++;
				if (priv->key_volup == 1) {
					SOUND_LOG_INFO("Volume ++\n");
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
					SOUND_LOG_INFO("Volume --\n");
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
					SOUND_LOG_INFO("Voice Assistant Open\n");
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
						SOUND_LOG_INFO("Hook (1)!!\n");
					}
					schedule_delayed_work(
					    &priv->hs_button_work,
					    msecs_to_jiffies(180));
				}
			} else {
				/*This could be key release,fix me ! */
				SOUND_LOG_DEBUG("tempdata:0x%x,Key data err:\n", tempdata);
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

#ifdef CONFIG_EXTCON_TYPEC_JACK
static void wait_and_detect_mic_gnd_state(struct sunxi_card_priv *priv)
{
	int i, reg_val;
	int interval_ms = 10;
	int total_ms = 180;
	int count = 5;
	int threshold = 0x8;

	if (!priv->mic_sel.used)
		return;

	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
		    (0x1 << HMICBIASEN), (0x1 << HMICBIASEN));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
		    (0x1 << MICADCEN), (0x1 << MICADCEN));

	gpio_set_value(priv->mic_sel.pin, 0);
	for (i = 0; i < count; i++) {
		reg_val = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
		reg_val = (reg_val >> HMIC_DATA) & 0x1f;
		if (reg_val >= threshold) {
			int ms = total_ms - interval_ms * i;
			msleep(ms);
			return;
		}
		msleep(interval_ms);
	}

	gpio_set_value(priv->mic_sel.pin, 1);
	for (i = 0; i < count; i++) {
		reg_val = snd_soc_component_read32(priv->component, SUNXI_HMIC_STS);
		reg_val = (reg_val >> HMIC_DATA) & 0x1f;
		if (reg_val >= threshold) {
			int ms = total_ms - interval_ms * i;
			msleep(ms);
			return;
		}
		msleep(interval_ms);
	}

	return;
}

static void sunxi_typec_jack_detect_status(struct work_struct *work)
{
	struct sunxi_card_priv *priv =
		container_of(work, struct sunxi_card_priv, typec_jack_detect_work.work);

	mutex_lock(&priv->jack_mlock);
	if (priv->detect_state == PLUG_OUT) {
		/* disable typec jack */
		if (priv->usb_sel.used)
			gpio_set_value(priv->usb_sel.pin, !priv->usb_sel.level);
	} else {
		wait_and_detect_mic_gnd_state(priv);

		/* enable typec jack */
		if (priv->usb_sel.used)
			gpio_set_value(priv->usb_sel.pin, priv->usb_sel.level);
	}

	schedule_delayed_work(&priv->hs_detect_work, msecs_to_jiffies(10));

	mutex_unlock(&priv->jack_mlock);
}

static int hp_plugin_notifier(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct sunxi_card_priv *priv = container_of(nb, struct sunxi_card_priv, hp_nb);

	SOUND_LOG_DEBUG("event -> %lu\n", event);
	if (event)
		priv->detect_state = PLUG_IN;
	else
		priv->detect_state = PLUG_OUT;

	schedule_delayed_work(&priv->typec_jack_detect_work, msecs_to_jiffies(10));

	return NOTIFY_DONE;
}
#endif

static const struct snd_kcontrol_new sunxi_card_controls[] = {
	SOC_DAPM_PIN_SWITCH("Headphone"),
	SOC_DAPM_PIN_SWITCH("HpSpeaker"),
	SOC_DAPM_PIN_SWITCH("LINEOUT"),
};

static const struct snd_soc_dapm_widget sunxi_card_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("HeadphoneMic", NULL),
	SND_SOC_DAPM_MIC("Main Mic", NULL),
};

static const struct snd_soc_dapm_route sunxi_card_routes[] = {
	{"MainMic Bias", NULL, "Main Mic"},
	{"MIC1", NULL, "MainMic Bias"},
	{"MIC2", NULL, "HeadphoneMic"},
};

static void sunxi_hs_reg_init(struct sunxi_card_priv *priv)
{

	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL, (0xffff << 0),
			    (0x0 << 0));

	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			(0x1f << MDATA_THRESHOLD),
			(0x17 << MDATA_THRESHOLD));

	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_STS, (0xffff << 0),
			    (0x6000 << 0));

	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
			(0xff << SELDETADCBF), (0x40 << SELDETADCBF));

	if (priv->hp_detect_case == HP_DETECT_LOW) {
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				    (0x1 << AUTOPLEN), (0x1 << AUTOPLEN));
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				    (0x1 << DETMODE), (0x0 << DETMODE));
	} else {
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				    (0x1 << AUTOPLEN), (0x0 << AUTOPLEN));
		snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				    (0x1 << DETMODE), (0x1 << DETMODE));
	}
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG, (0x1 << JACKDETEN),
			    (0x1 << JACKDETEN));
	/*enable jack in /out irq*/
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_IN_IRQ_EN), (0x1 << JACK_IN_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_OUT_IRQ_EN), (0x1 << JACK_OUT_IRQ_EN));

	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			(0xf << HMIC_N), (HP_DEBOUCE_TIME << HMIC_N));

	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
			    (0x1 << HMICBIASEN), (0x1 << HMICBIASEN));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
			    (0x1 << MICADCEN), (0x1 << MICADCEN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
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
	free_irq(priv->jackirq, priv);
}

/*
 * Card initialization
 */
static int sunxi_card_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = rtd->codec_dai->component;
	struct snd_soc_dapm_context *dapm = &component->dapm;

	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(rtd->card);
	int ret;

	priv->component = rtd->codec_dai->component;

	ret = snd_soc_card_jack_new(rtd->card, "sunxi Audio Jack",
			       SND_JACK_HEADSET | SND_JACK_HEADPHONE |
				   SND_JACK_BTN_0 | SND_JACK_BTN_1 |
				   SND_JACK_BTN_2 | SND_JACK_BTN_3,
			       &priv->jack, NULL, 0);
	if (ret) {
		SOUND_LOG_ERR("jack creation failed\n");
		return ret;
	}

	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);
	snd_jack_set_key(priv->jack.jack, SND_JACK_BTN_3, KEY_VOICECOMMAND);

	snd_soc_dapm_disable_pin(dapm, "HPOUTR");
	snd_soc_dapm_disable_pin(dapm, "HPOUTL");

	snd_soc_dapm_disable_pin(dapm, "LINEOUT");
	snd_soc_dapm_disable_pin(dapm, "HpSpeaker");
	snd_soc_dapm_disable_pin(dapm, "Headphone");

	snd_soc_dapm_sync(dapm);

	return 0;
}

static int sunxi_card_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned int freq;
	int ret;
	int stream_flag;

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
		SOUND_LOG_ERR("invalid rate setting\n");
		return -EINVAL;
	}

	/* the substream type: 0->playback, 1->capture */
	stream_flag = substream->stream;
	SOUND_LOG_DEBUG("stream_flag: %d\n", stream_flag);

	/* To surpport playback and capture func in different freq point */
	if (freq == 22579200) {
		if (stream_flag == 0) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 0, freq, 0);
			if (ret < 0) {
				SOUND_LOG_ERR("set codec dai sysclk faided, freq:%d\n", freq);
				return ret;
			}
		}
	}

	if (freq == 22579200) {
		if (stream_flag == 1) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 1, freq, 0);
			if (ret < 0) {
				SOUND_LOG_ERR("set codec dai sysclk faided, freq:%d\n", freq);
				return ret;
			}
		}
	}

	if (freq == 24576000) {
		if (stream_flag == 0) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 2, freq, 0);
			if (ret < 0) {
				SOUND_LOG_ERR("set codec dai sysclk faided, freq:%d\n", freq);
				return ret;
			}
		}
	}

	if (freq == 24576000) {
			if (stream_flag == 1) {
			ret = snd_soc_dai_set_sysclk(codec_dai, 3, freq, 0);
			if (ret < 0) {
				SOUND_LOG_ERR("set codec dai sysclk faided, freq:%d\n", freq);
				return ret;
			}
		}
	}

	return 0;
}

static struct snd_soc_ops sunxi_card_ops = {
	.hw_params = sunxi_card_hw_params,
};

SND_SOC_DAILINK_DEFS(sun50iw10p1_dai_link,
	DAILINK_COMP_ARRAY(COMP_CPU("sunxi-dummy-cpudai")),
	DAILINK_COMP_ARRAY(COMP_CODEC("sunxi-internal-codec", "sun50iw10codec")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("sunxi-dummy-cpudai")));

static struct snd_soc_dai_link sunxi_card_dai_link[] = {
	{
		.name		= "audiocodec",
		.stream_name	= "SUNXI-CODEC",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBM_CFM,
		.init		= sunxi_card_init,
		.ops		= &sunxi_card_ops,
		SND_SOC_DAILINK_REG(sun50iw10p1_dai_link),
	},
};

static int sunxi_card_suspend(struct snd_soc_card *card)
{
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(card);

	disable_irq(priv->jackirq);

	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << MIC_DET_IRQ_EN), (0x0 << MIC_DET_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_IN_IRQ_EN), (0x0 << JACK_IN_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_OUT_IRQ_EN), (0x0 << JACK_OUT_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG, (0x1 << JACKDETEN),
			    (0x0 << JACKDETEN));
	SOUND_LOG_DEBUG("suspend\n");

	return 0;
}

static int sunxi_card_resume(struct snd_soc_card *card)
{
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(card);

	enable_irq(priv->jackirq);
	priv->jack_irq_times = RESUME_IRQ;
	priv->detect_state = PLUG_OUT;/*todo..?*/
	sunxi_hs_reg_init(priv);
	SOUND_LOG_DEBUG("resume\n");

	return 0;
}

static struct snd_soc_card snd_soc_sunxi_card = {
	.name			= "audiocodec",
	.owner			= THIS_MODULE,
	.dai_link		= sunxi_card_dai_link,
	.num_links		= ARRAY_SIZE(sunxi_card_dai_link),
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
	struct snd_soc_dapm_context *dapm = &card->dapm;

	SOUND_LOG_INFO("register card begin\n");

	if (!np) {
		SOUND_LOG_ERR("can not get dt node for this device\n");
		return -EINVAL;
	}

	/* dai link */
	sunxi_card_dai_link[0].cpus->dai_name = NULL;
	sunxi_card_dai_link[0].cpus->of_node = of_parse_phandle(np,
					"sunxi,cpudai-controller", 0);
	if (!sunxi_card_dai_link[0].cpus->of_node) {
		SOUND_LOG_ERR("Property 'sunxi,cpudai-controller' missing or invalid\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	} else {
		sunxi_card_dai_link[0].platforms->name = NULL;
		sunxi_card_dai_link[0].platforms->of_node =
				sunxi_card_dai_link[0].cpus->of_node;
	}
	sunxi_card_dai_link[0].codecs->name = NULL;
	sunxi_card_dai_link[0].codecs->of_node = of_parse_phandle(np,
						"sunxi,audio-codec", 0);
	if (!sunxi_card_dai_link[0].codecs->of_node) {
		SOUND_LOG_ERR("Property 'sunxi,audio-codec' missing or invalid\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	}

	/* register the soc card */
	card->dev = &pdev->dev;

	priv = devm_kzalloc(&pdev->dev,
		sizeof(struct sunxi_card_priv), GFP_KERNEL);
	if (!priv) {
		SOUND_LOG_ERR("devm_kzalloc failed %d\n", ret);
		return -ENOMEM;
	}
	priv->card = card;

	snd_soc_card_set_drvdata(card, priv);

	ret = snd_soc_register_card(card);
	if (ret) {
		SOUND_LOG_ERR("snd_soc_register_card failed %d\n", ret);
		goto err_devm_kfree;
	}

	ret = snd_soc_add_card_controls(card, sunxi_card_controls,
					ARRAY_SIZE(sunxi_card_controls));
	if (ret)
		SOUND_LOG_ERR("failed to register codec controls!\n");

	snd_soc_dapm_new_controls(dapm, sunxi_card_dapm_widgets,
				ARRAY_SIZE(sunxi_card_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, sunxi_card_routes,
				ARRAY_SIZE(sunxi_card_routes));

	ret = of_property_read_u32(np, "hp_detect_case", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("hp_detect_case  missing or invalid.\n");
	} else {
		priv->hp_detect_case = temp_val;
		SOUND_LOG_INFO("hp_detect_case: %d\n", priv->hp_detect_case);
	}

	priv->jackirq = platform_get_irq(pdev, 0);
	if (priv->jackirq < 0) {
		SOUND_LOG_ERR("irq failed\n");
		ret = -ENODEV;
	}

	priv->jack_irq_times = SYSINIT_IRQ;

	/*
	 * initial the parameters for judge switch state
	 */
	ret = of_property_read_u32(np, "jack_threshold", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("headset_threshold get failed, use default vol -> 0x10\n");
		priv->HEADSET_DATA = 0x10;
	} else {
		priv->HEADSET_DATA = temp_val;
	}
	priv->detect_state = PLUG_OUT;
	INIT_DELAYED_WORK(&priv->hs_detect_work, sunxi_check_hs_detect_status);
	INIT_DELAYED_WORK(&priv->hs_button_work, sunxi_check_hs_button_status);
	INIT_DELAYED_WORK(&priv->hs_init_work, sunxi_hs_init_work);
	INIT_DELAYED_WORK(&priv->hs_checkplug_work, sunxi_check_hs_plug);
	mutex_init(&priv->jack_mlock);

	ret = request_irq(priv->jackirq, jack_interrupt, 0, "audio jack irq", priv);

#ifdef CONFIG_EXTCON_TYPEC_JACK
	ret = of_property_read_u32(np, "typec_analog_jack_enable", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("typec_analog_jack_enable get failed, default disable\n");
		priv->typec_analog_jack_enable = 0;
	} else {
		priv->typec_analog_jack_enable = temp_val;
	}

	if (priv->typec_analog_jack_enable) {

	ret = of_property_read_u32(np, "usb_sel_level", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("usb_sel_level get failed, use default vol -> H\n");
		priv->usb_sel.level = 0;
	} else {
		priv->usb_sel.level = temp_val;
	}
	ret = of_property_read_u32(np, "usb_noe_level", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("usb_noe_level get failed, use default vol -> L\n");
		priv->usb_noe.level = 0;
	} else {
		priv->usb_noe.level = temp_val;
	}

	ret = of_get_named_gpio(np, "usb_sel_gpio", 0);
	priv->usb_sel.used = 0;
	if (ret >= 0) {
		priv->usb_sel.pin = ret;
		if (!gpio_is_valid(priv->usb_sel.pin)) {
			SOUND_LOG_ERR("usb_sel_gpio is invalid\n");
		} else {
			ret = devm_gpio_request(&pdev->dev,
					priv->usb_sel.pin, "USB_SEL");
			if (ret) {
				SOUND_LOG_ERR("failed to request usb_sel_gpio\n");
			} else {
				priv->usb_sel.used = 1;
				gpio_direction_output(priv->usb_sel.pin, 1);
			}
		}
	}
	ret = of_get_named_gpio(np, "usb_noe_gpio", 0);
	priv->usb_noe.used = 0;
	if (ret >= 0) {
		priv->usb_noe.pin = ret;
		if (!gpio_is_valid(priv->usb_noe.pin)) {
			SOUND_LOG_ERR("usb_noe_gpio is invalid\n");
		} else {
			ret = devm_gpio_request(&pdev->dev,
					priv->usb_noe.pin, "USB_NOE");
			if (ret) {
				SOUND_LOG_ERR("failed to request usb_noe_gpio\n");
			} else {
				priv->usb_noe.used = 1;
				gpio_direction_output(priv->usb_noe.pin, 1);
				/* default connect */
				gpio_set_value(priv->usb_noe.pin, priv->usb_noe.level);
			}
		}
	}
	ret = of_get_named_gpio(np, "mic_sel_gpio", 0);
	priv->mic_sel.used = 0;
	if (ret >= 0) {
		priv->mic_sel.pin = ret;
		if (!gpio_is_valid(priv->mic_sel.pin)) {
			SOUND_LOG_ERR("mic_sel_gpio is invalid\n");
		} else {
			ret = devm_gpio_request(&pdev->dev,
					priv->mic_sel.pin, "MIC_SEL");
			if (ret) {
				SOUND_LOG_ERR("failed to request mic_sel_gpio\n");
			} else {
				priv->mic_sel.used = 1;
				gpio_direction_output(priv->mic_sel.pin, 1);
			}
		}
	}

	if (of_property_read_bool(np, "extcon")) {
		priv->extdev = extcon_get_edev_by_phandle(&pdev->dev, 0);
		if (IS_ERR(priv->extdev)) {
			ret = -ENODEV;
			goto err_devm_kfree;
		}
		priv->hp_nb.notifier_call = hp_plugin_notifier;
		ret = extcon_register_notifier(priv->extdev,
				EXTCON_JACK_HEADPHONE, &priv->hp_nb);
		if (ret < 0) {
			SOUND_LOG_ERR("register hp notifier failed\n");
			goto err_devm_kfree;
		}
		INIT_DELAYED_WORK(&priv->typec_jack_detect_work,
				  sunxi_typec_jack_detect_status);
		priv->detect_state = PLUG_OUT;
		ret = extcon_get_state(priv->extdev, EXTCON_JACK_HEADPHONE);
		if (ret > 0) {
			priv->detect_state = PLUG_IN;
		}

		schedule_delayed_work(&priv->typec_jack_detect_work,
				      msecs_to_jiffies(10));
	}
	}
#endif

	sunxi_hs_reg_init(priv);
	SOUND_LOG_DEBUG("0x310:0x%X,0x314:0x%X,0x318:0x%X,0x1C:0x%X,0x1D:0x%X\n",
			snd_soc_component_read32(priv->component, 0x310),
			snd_soc_component_read32(priv->component, 0x314),
			snd_soc_component_read32(priv->component, 0x318),
			snd_soc_component_read32(priv->component, 0x1C),
			snd_soc_component_read32(priv->component, 0x1D));

	SOUND_LOG_INFO("register card finished\n");

	return 0;

err_devm_kfree:
	devm_kfree(&pdev->dev, priv);
	return ret;
}

static int __exit sunxi_card_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct sunxi_card_priv *priv = snd_soc_card_get_drvdata(card);

#ifdef CONFIG_EXTCON_TYPEC_JACK
	if (priv->typec_analog_jack_enable) {

	extcon_unregister_notifier(priv->extdev, EXTCON_JACK_HEADPHONE, &priv->hp_nb);
	/* disable typec jack */
	if (priv->usb_sel.used)
		gpio_set_value(priv->usb_sel.pin, !priv->usb_sel.level);

	}
#endif

	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_IN_IRQ_EN), (0x0 << JACK_IN_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_HMIC_CTRL,
			    (0x1 << JACK_OUT_IRQ_EN), (0x0 << JACK_OUT_IRQ_EN));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG, (0x1 << JACKDETEN),
			    (0x0 << JACKDETEN));
	snd_soc_component_update_bits(priv->component, SUNXI_MICBIAS_REG,
				(0x1 << HMICBIASEN), (0x0 << HMICBIASEN));
	snd_sunxi_unregister_jack(priv);

	snd_soc_unregister_card(card);
	devm_kfree(&pdev->dev, priv);

	SOUND_LOG_INFO("unregister card finished\n");

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

static int __init sunxi_machine_driver_init(void)
{
	return platform_driver_register(&sunxi_machine_driver);
}
module_init(sunxi_machine_driver_init);

static void __exit sunxi_machine_driver_exit(void)
{
	platform_driver_unregister(&sunxi_machine_driver);
}
module_exit(sunxi_machine_driver_exit);

module_param_named(switch_state, switch_state, int, S_IRUGO | S_IWUSR);

MODULE_AUTHOR("luguofang <luguofang@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI Codec Machine ASoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-codec-machine");

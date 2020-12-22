/*
 * sound\soc\sunxi\audiocodec\sunxi_sndcodec.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
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
#include <mach/sys_config.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>

#ifdef CONFIG_ARCH_SUN8IW9
#include "sun8iw9_sndcodec.h"
#ifdef AUDIOCODEC_GPIO
static script_item_u item;
#endif

#undef HS_DBG
#if (1)
    #define HS_DBG(format,args...)  printk("[SWITCH] "format,##args)
#else
    #define HS_DBG(...)
#endif

#define SUN8IW9_INTR_DEBOUNCE               200
#define SUN8IW9_HS_INSERT_DET_DELAY         500
#define SUN8IW9_HS_REMOVE_DET_DELAY         500
#define SUN8IW9_BUTTON_DET_DELAY            100
#define SUN8IW9_HS_DET_POLL_INTRVL          100
#define SUN8IW9_BUTTON_EN_DELAY             1500
#define SUN8IW9_HS_DET_RETRY_COUNT          6

struct sun8iw9_mc_private {
	struct snd_soc_jack jack;
	struct delayed_work hs_insert_work;
	struct delayed_work hs_remove_work;
	struct delayed_work hs_button_work;
	struct mutex jack_mlock;
	/* To enable button press interrupts after a delay after HS detection.
	 * This is to avoid spurious button press events during slow HS insertion
	 */
	struct delayed_work hs_button_en_work;
	int intr_debounce;
	int hs_insert_det_delay;
	int hs_remove_det_delay;
	int button_det_delay;
	int button_en_delay;
	int hs_det_poll_intrvl;
	int hs_det_retry;
	bool process_button_events;
};

static int sun8iw9_hs_detection(void);

static struct snd_soc_jack_gpio hs_gpio = {
	.gpio			= GPIOL(7),
	.name			= "hp-gpio",
	.report			= SND_JACK_HEADSET |
				  SND_JACK_HEADPHONE |
				  SND_JACK_BTN_0,
	.invert			= 1,
	.debounce_time		= SUN8IW9_INTR_DEBOUNCE,
	.jack_status_check	= sun8iw9_hs_detection,
};

static inline void sun8iw9_set_codec_en(struct snd_soc_codec *codec, int jack_type)
{
	switch (jack_type) {
	case SND_JACK_HEADSET:
		snd_soc_update_bits(codec, JACK_MIC_CTRL, (0x1<<HMICBIASEN), (0x1<<HMICBIASEN));
		snd_soc_update_bits(codec, JACK_MIC_CTRL, (0x1<<MICADCEN), (0x1<<MICADCEN));
		snd_soc_update_bits(codec, SUNXI_HMIC_CTRL2, (0x1f<<MDATA_THRESHOLD), (0x1b<<MDATA_THRESHOLD));
		break;
	case SND_JACK_HEADPHONE:
	case 0:
		snd_soc_update_bits(codec, JACK_MIC_CTRL, (0x1<<HMICBIASEN), (0x0<<HMICBIASEN));
		snd_soc_update_bits(codec, JACK_MIC_CTRL, (0x1<<MICADCEN), (0x0<<MICADCEN));
	    break;
	default:
		break;
	}
}

/**
 * sun8iw9_headset_detect - Detect headset.
 * @codec: SoC audio codec device.
 * @jack_insert: Jack insert or not.
 *
 * Detect whether is headset or not when jack inserted.
 *
 * Returns detect status.
 */
static int sun8iw9_headset_detect(struct snd_soc_codec *codec, int jack_insert)
{
	int reg_val = 0;
	int jack_type = 0;

	if (jack_insert) {
			reg_val = snd_soc_read(codec, SUNXI_HMIC_STS);
			reg_val = (reg_val>>8)&0x1f;
			if (reg_val > 0x1b) {
				jack_type = SND_JACK_HEADSET;
			} else {
				jack_type = SND_JACK_HEADPHONE;
			}
	} else {
		jack_type = 0;
	}

	pr_debug("jack_type = %d\n", jack_type);
	return jack_type;
}

/* Identify the jack type as Headset/Headphone/None */
static int sun8iw9_check_jack_type(struct snd_soc_jack *jack, struct snd_soc_codec *codec)
{
	int reg_val;
	int status = 0, jack_type = 0;
	struct sun8iw9_mc_private *ctx = container_of(jack, struct sun8iw9_mc_private, jack);

	reg_val = snd_soc_read(codec, SUNXI_HMIC_STS);
	status = (reg_val>>JACK_DET_IIN_ST)&0x1;
	/* jd status low indicates some accessory has been connected */
	if (!status) {
		HS_DBG("Jack insert intr\n");
		/* Do not process button events until accessory is detected as headset*/
		ctx->process_button_events = false;
		sun8iw9_set_codec_en(codec, SND_JACK_HEADSET);
		jack_type = sun8iw9_headset_detect(codec, true);
		if (jack_type == SND_JACK_HEADSET) {
			ctx->process_button_events = true;
			/* If headset is detected, enable button interrupts after a delay */
			schedule_delayed_work(&ctx->hs_button_en_work,
					msecs_to_jiffies(ctx->button_en_delay));
		}
		if (jack_type != SND_JACK_HEADSET) {
			sun8iw9_set_codec_en(codec, SND_JACK_HEADPHONE);
		}
	} else {
		jack_type = 0;
	}
	HS_DBG("Jack type detected:%d\n", jack_type);

	return jack_type;
}

/* Work function invoked by the Jack Infrastructure.
 * Other delayed works for jack detection/removal/button
 * press are scheduled from this function
 */
static int sun8iw9_hs_detection(void)
{
	int ret;
	int reg_val = 0;
	int status = 0, jack_type = 0;
	struct snd_soc_jack_gpio *gpio = &hs_gpio;
	struct snd_soc_jack *jack = gpio->jack;
	struct snd_soc_codec *codec = jack->codec;
	struct sun8iw9_mc_private *ctx = container_of(jack, struct sun8iw9_mc_private, jack);

	mutex_lock(&ctx->jack_mlock);
	/* Initialize jack status with previous status.
	 * The delayed work will confirm  the event and send updated status later
	 */
	jack_type = jack->status;
	HS_DBG("Enter:%s,%d,jack_type:%d\n", __func__, __LINE__, jack_type);

	if (!jack->status) {
		ctx->hs_det_retry = SUN8IW9_HS_DET_RETRY_COUNT;
		ret = schedule_delayed_work(&ctx->hs_insert_work,
				msecs_to_jiffies(ctx->hs_insert_det_delay));
		if (!ret)
			HS_DBG("sun8iw9_check_hs_insert_status already queued\n");
		else
			HS_DBG("%s:Check hs insertion  after %d msec\n",
					__func__, ctx->hs_insert_det_delay);
	} else {
		/* First check for accessory removal; If not removed,
		 * check for button events
		 */
		reg_val = snd_soc_read(codec, SUNXI_HMIC_STS);
		status = (reg_val>>JACK_DET_OIRQ)&0x1;
		/* jd status high indicates accessory has been disconnected.
		 * However, confirm the removal in the delayed work
		 */
		if (status) {
			/* Do not process button events while we make sure
			 * accessory is disconnected
			 */
			ctx->process_button_events = false;
			ret = schedule_delayed_work(&ctx->hs_remove_work,
					msecs_to_jiffies(ctx->hs_remove_det_delay));
			if (!ret)
				HS_DBG("sun8iw9_check_hs_remove_status already queued\n");
			else
				HS_DBG("%s:Check hs removal after %d msec\n",
						__func__, ctx->hs_remove_det_delay);
		} else {
			/*need TODO... Must be button event. Confirm the event in delayed work*/
			if (((jack->status & SND_JACK_HEADSET) == SND_JACK_HEADSET) &&
					ctx->process_button_events) {
				ret = schedule_delayed_work(&ctx->hs_button_work,
						msecs_to_jiffies(ctx->button_det_delay));
				if (!ret) {
					HS_DBG("sun8iw9_check_hs_button_status already queued\n");
				} else {
					HS_DBG("%s:check BP/BR after %d msec\n",
							__func__, ctx->button_det_delay);
				}
			}
		}
	}

	mutex_unlock(&ctx->jack_mlock);
	HS_DBG("Exit:%s\n", __func__);
	return jack_type;
}

/* Checks jack insertion and identifies the jack type.
 * Retries the detection if necessary
 */
static void sun8iw9_check_hs_insert_status(struct work_struct *work)
{
	struct snd_soc_jack_gpio *gpio = &hs_gpio;
	struct snd_soc_jack *jack = gpio->jack;
	struct snd_soc_codec *codec = jack->codec;
	struct sun8iw9_mc_private *ctx = container_of(work, struct sun8iw9_mc_private, hs_insert_work.work);
	int jack_type = 0;

	mutex_lock(&ctx->jack_mlock);
	HS_DBG("Enter:%s\n", __func__);

	jack_type = sun8iw9_check_jack_type(jack, codec);

	/* Report jack immediately only if jack is headset.
	 * If headphone or no jack was detected, dont report it until the last HS det try.
	 * This is to avoid reporting any temporary jack removal or accessory change
	 * (eg, HP to HS) during the detection tries.
	 * This provides additional debounce that will help in the case of slow insertion.
	 * This also avoids the pause in audio due to accessory change from HP to HS
	 */
	if (ctx->hs_det_retry <= 0) {
		/* end of retries; report the status */
		snd_soc_jack_report(jack, jack_type, gpio->report);
		snd_soc_update_bits(codec, SUNXI_HMIC_STS, (0x1<<JACK_DET_IIN_ST), (0x1<<JACK_DET_IIN_ST));
	} else {
		/* Schedule another detection try if headphone or no jack is detected.
		 * During slow insertion of headset, first a headphone may be detected.
		 * Hence retry until headset is detected
		 */
		if (jack_type == SND_JACK_HEADSET) {
			ctx->hs_det_retry = 0;
			/* HS detected, no more retries needed */
			snd_soc_jack_report(jack, jack_type, gpio->report);
			snd_soc_update_bits(codec, SUNXI_HMIC_STS, (0x1<<JACK_DET_IIN_ST), (0x1<<JACK_DET_IIN_ST));
		} else {
			ctx->hs_det_retry--;
			schedule_delayed_work(&ctx->hs_insert_work,
					msecs_to_jiffies(ctx->hs_det_poll_intrvl));
			HS_DBG("%s:re-try hs detection after %d msec\n",
					__func__, ctx->hs_det_poll_intrvl);
		}
	}

	HS_DBG("Exit:%s", __func__);
	mutex_unlock(&ctx->jack_mlock);
}
/* Checks jack removal. */
static void sun8iw9_check_hs_remove_status(struct work_struct *work)
{
	int reg_val = 0;
	int status = 0, jack_type = 0;
	struct snd_soc_jack_gpio *gpio = &hs_gpio;
	struct snd_soc_jack *jack = gpio->jack;
	struct snd_soc_codec *codec = jack->codec;
	struct sun8iw9_mc_private *ctx = container_of(work, struct sun8iw9_mc_private, hs_remove_work.work);

	/* Cancel any pending insertion detection.
	 * There could be pending insertion detection in the
	 * case of very slow insertion or insertion and immediate removal.
	 */
	cancel_delayed_work_sync(&ctx->hs_insert_work);

	mutex_lock(&ctx->jack_mlock);
	HS_DBG("Enter:%s\n", __func__);
	/* Initialize jack_type with previous status.
	 * If the event was an invalid one, we return the previous state
	 */
	jack_type = jack->status;

	if (jack->status) {
		/* jack is in connected state; look for removal event */
		reg_val = snd_soc_read(codec, SUNXI_HMIC_STS);
		status = (reg_val>>JACK_DET_OIRQ)&0x1;
		if (status) {
			/* jd status high implies accessory disconnected */
			HS_DBG("Jack remove event\n");
			ctx->process_button_events = false;
			cancel_delayed_work_sync(&ctx->hs_button_en_work);
			status = sun8iw9_headset_detect(codec, false);
			jack_type = 0;
			sun8iw9_set_codec_en(codec, 0);
		} else if (((jack->status & SND_JACK_HEADSET) == SND_JACK_HEADSET)
				&& !ctx->process_button_events) {
			/* Jack is still connected. We may come here if there was a spurious
			 * jack removal event. No state change is done until removal is confirmed
			 * by the check_jd_status above.i.e. jack status remains Headset or headphone.
			 * But as soon as the interrupt thread(sun8iw9_hs_detection) detected a jack
			 * removal, button processing gets disabled. Hence re-enable button processing
			 * in the case of headset.
			 */
			HS_DBG(" spurious Jack remove event for headset; re-enable button events\n");
			ctx->process_button_events = true;
		}
	}
	snd_soc_jack_report(jack, jack_type, gpio->report);
	HS_DBG("Exit:%s\n", __func__);
	mutex_unlock(&ctx->jack_mlock);
}

/* Check for button press/release */
static void sun8iw9_check_hs_button_status(struct work_struct *work)
{
	struct snd_soc_jack_gpio *gpio = &hs_gpio;
	struct snd_soc_jack *jack = gpio->jack;
	struct snd_soc_codec *codec = jack->codec;
	struct sun8iw9_mc_private *ctx = container_of(work, struct sun8iw9_mc_private, hs_button_work.work);
	int status = 0;
	int jack_type = 0;
	int reg_val = 0;
	int ret;

	mutex_lock(&ctx->jack_mlock);
	HS_DBG("Enter:%s\n", __func__);
	/* Initialize jack_type with previous status.
	 * If the event was an invalid one, we return the preious state
	 */
	jack_type = jack->status;

	if (((jack->status & SND_JACK_HEADSET) == SND_JACK_HEADSET)
			&& ctx->process_button_events) {
		reg_val = snd_soc_read(codec, SUNXI_HMIC_STS);
		status = (reg_val>>JACK_DET_IIN_ST)&0x1;
		if (!status) {
			/* confirm jack is connected */
			reg_val = snd_soc_read(codec, SUNXI_HMIC_STS);
			status = (reg_val>>MIC_DET_ST)&0x1;
			if (jack->status & SND_JACK_BTN_0) {
				/* if button was previosly in pressed state*/
				if (!status) {
					HS_DBG("BR event received\n");
					jack_type = SND_JACK_HEADSET;
				}
			} else {
				/* If button was previously in released state */
				if (status) {
					HS_DBG("BP event received\n");
					jack_type = SND_JACK_HEADSET | SND_JACK_BTN_0;
				}
			}
		}
		/* There could be button interrupts during jack removal.
		 * There can be situations where a button interrupt is generated
		 * first but no jack removal interrupt is generated.
		 * This can happen on platforms where jack detection is aligned to
		 * Headset Left pin instead of the ground  pin and codec multiplexes
		 * (ORs) the jack and button interrupts. So schedule a jack removal detection work
		 */
		ret = schedule_delayed_work(&ctx->hs_remove_work,
				msecs_to_jiffies(ctx->hs_remove_det_delay));
		if (!ret)
			HS_DBG("sun8iw9_check_hs_remove_status already queued\n");
		else
			HS_DBG("%s:Check hs removal after %d msec\n",
					__func__, ctx->hs_remove_det_delay);

	}
	snd_soc_jack_report(jack, jack_type, gpio->report);
	HS_DBG("Exit:%s\n", __func__);
	mutex_unlock(&ctx->jack_mlock);
}

/* Delayed work for enabling the overcurrent detection circuit
 * and interrupt for generating button events */
static void sun8iw9_enable_hs_button_events(struct work_struct *work)
{
	/* TODO move the enable button event here */
}

static int sunxi_sndpcm_hw_params(struct snd_pcm_substream *substream,
                                       struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq_in = 22579200;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned long sample_rate = params_rate(params);

	switch (sample_rate) {
		case 8000:
		case 16000:
		case 32000:
		case 64000:
		case 128000:
		case 12000:
		case 24000:
		case 48000:
		case 96000:
		case 192000:
			freq_in = 24576000;
			break;
	}
	/* set the codec FLL */
	ret = snd_soc_dai_set_pll(codec_dai, AIF1_CLK, 0, freq_in, freq_in);
	if (ret < 0) {
		pr_err("err:%s,line:%d\n", __func__, __LINE__);
	}
	/*set system clock source freq */
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, freq_in, 0);
	if (ret < 0) {
		return ret;
	}
	/*set system clock source freq_in and set the mode as tdm or pcm*/
	ret = snd_soc_dai_set_sysclk(codec_dai, AIF1_CLK, freq_in, 0);
	if (ret < 0) {
		return ret;
	}
	/*set system fmt:api2s:master aif1:slave*/
	ret = snd_soc_dai_set_fmt(cpu_dai, 0);
	if (ret < 0) {
		pr_err("%s, line:%d\n", __func__, __LINE__);
		return ret;
	}

	/*
	* codec: slave. AP: master
	*/
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		pr_err("%s, line:%d\n", __func__, __LINE__);
		return ret;
	}

	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		pr_err("%s, line:%d\n", __func__, __LINE__);
		return ret;
	}

	return 0;
}

static int ac_speaker_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	switch (event) {
		case SND_SOC_DAPM_POST_PMU:
			HS_DBG("[speaker open ]%s,line:%d\n",__func__,__LINE__);
#ifdef AUDIOCODEC_GPIO
			gpio_set_value(item.gpio.gpio, 1);
#endif
			break;
		case SND_SOC_DAPM_PRE_PMD :
			HS_DBG("[speaker close ]%s,line:%d\n",__func__,__LINE__);
#ifdef AUDIOCODEC_GPIO
			gpio_set_value(item.gpio.gpio, 0);
#endif
		default:
			break;
	}
	return 0;
}

static const struct snd_kcontrol_new ac_pin_controls[] = {
	SOC_DAPM_PIN_SWITCH("External Speaker"),
	SOC_DAPM_PIN_SWITCH("Headphone"),
};

static const struct snd_soc_dapm_widget sun8iw9_ac_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("External Speaker", ac_speaker_event),
	SND_SOC_DAPM_MIC("External MainMic", NULL),
	SND_SOC_DAPM_MIC("HeadphoneMic", NULL),
};

static const struct snd_soc_dapm_route audio_map[] = {
	{"External Speaker", NULL, "SPKL"},
	{"External Speaker", NULL, "SPKR"},

	{"MainMic Bias", NULL, "External MainMic"},
	{"MIC1P", NULL, "MainMic Bias"},
	{"MIC1N", NULL, "MainMic Bias"},

	{"MIC2", NULL, "HeadphoneMic"},
};
/*
 * Card initialization
 */
static int sunxi_daudio_init(struct snd_soc_pcm_runtime *runtime)
{
	int ret;
	struct snd_soc_codec *codec = runtime->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
//	struct snd_soc_card *card = runtime->card;
	struct sun8iw9_mc_private *ctx = snd_soc_card_get_drvdata(runtime->card);

	snd_soc_update_bits(codec, SUNXI_HMIC_CTRL1, (0x1<<JACK_IN_IRQ_EN), (0x1<<JACK_IN_IRQ_EN));
	snd_soc_update_bits(codec, SUNXI_HMIC_CTRL1, (0x1<<JACK_OUT_IRQ_EN), (0x1<<JACK_OUT_IRQ_EN));
	snd_soc_update_bits(codec, SUNXI_HMIC_CTRL1, (0x1<<MIC_DET_IRQ_EN), (0x1<<MIC_DET_IRQ_EN));

	snd_soc_update_bits(codec, JACK_MIC_CTRL, (0x1<<JACKDETEN), (0x1<<JACKDETEN));
	snd_soc_update_bits(codec, JACK_MIC_CTRL, (0x1<<AUTOPLEN), (0x1<<AUTOPLEN));

	ret = snd_soc_jack_new(codec, "Intel MID Audio Jack",
			       SND_JACK_HEADSET | SND_JACK_HEADPHONE | SND_JACK_BTN_0,
			       &ctx->jack);
	if (ret) {
		pr_err("jack creation failed\n");
		return ret;
	}

//	snd_soc_jack_add_pins(&hp_jack, ARRAY_SIZE(hp_jack_pins),
//		hp_jack_pins);

	snd_jack_set_key(ctx->jack.jack, SND_JACK_BTN_0, KEY_MEDIA);

	ret = snd_soc_jack_add_gpios(&ctx->jack, 1, &hs_gpio);
	if (ret) {
		pr_err("adding jack GPIO failed,ret:%x\n", ret);
	//	return ret;
	}

	snd_soc_dapm_disable_pin(&codec->dapm,	"HPOUTR");
	snd_soc_dapm_disable_pin(&codec->dapm,	"HPOUTL");
	snd_soc_dapm_disable_pin(&codec->dapm,	"EAROUTP");
	snd_soc_dapm_disable_pin(&codec->dapm,	"EAROUTN");
	snd_soc_dapm_disable_pin(&codec->dapm,	"SPKL");
	snd_soc_dapm_disable_pin(&codec->dapm,	"SPKR");
	snd_soc_dapm_sync(dapm);

	return 0;
}

static struct snd_soc_ops sunxi_sndpcm_ops = {
       .hw_params              = sunxi_sndpcm_hw_params,
};
#endif

static int goni_voice_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;

	if (params_rate(params) != 8000)
		return -EINVAL;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_LEFT_J |
			SND_SOC_DAIFMT_IB_IF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops goni_voice_ops = {
	.hw_params = goni_voice_hw_params,
};

static struct snd_soc_dai_link sunxi_sndpcm_dai_link[] = {
	{
	.name 			= "audiocodec",
	.stream_name 	= "SUNXI-CODEC",
	.cpu_dai_name 	= "sunxi-codec",
	.codec_dai_name = "codec-aif1",
	.platform_name 	= "sunxi-pcm-codec-audio",
	.codec_name 	= "sunxi-pcm-codec",
	.init 			= sunxi_daudio_init,
#ifdef CONFIG_ARCH_SUN8IW9
    .ops = &sunxi_sndpcm_ops,
#endif
	},{
	.name = "sun8iw9 Voice",
	.stream_name = "Voice",
	.cpu_dai_name = "sunxi-bbcodec",
	.codec_dai_name = "codec-aif2",
	.codec_name = "sunxi-pcm-codec",
	.ops = &goni_voice_ops,
	},
};

static struct snd_soc_card snd_soc_sunxi_sndpcm = {
	.name 		= "audiocodec",
	.owner 		= THIS_MODULE,
	.dai_link 	= sunxi_sndpcm_dai_link,
	.num_links 	= ARRAY_SIZE(sunxi_sndpcm_dai_link),
	.dapm_widgets = sun8iw9_ac_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(sun8iw9_ac_dapm_widgets),
	.dapm_routes = audio_map,
	.num_dapm_routes = ARRAY_SIZE(audio_map),
	.controls = ac_pin_controls,
	.num_controls = ARRAY_SIZE(ac_pin_controls),
};

static int sun8iw9_machine_probe(struct platform_device *pdev)
{
	int ret_val = 0;
	struct sun8iw9_mc_private *drv;
//	int codec_gpio;

	HS_DBG("Entry %s\n", __func__);
#ifdef AUDIOCODEC_GPIO
	script_item_value_type_e  type;
	/*get the default pa val(close)*/
	type = script_get_item("audio0", "audio_pa_ctrl", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("script_get_item return type err\n");
		return -EFAULT;
	}
HS_DBG("%s,line:%d,item.gpio.gpio:%s\n", __func__, __LINE__, item.gpio.gpio);
	/*request pa gpio*/
	ret = gpio_request(item.gpio.gpio, NULL);
	if (0 != ret) {
		pr_err("request gpio failed!\n");
	}
	/*
	* config gpio info of audio_pa_ctrl, the default pa config is close(check pa sys_config1.fex)
	*/
	gpio_direction_output(item.gpio.gpio, 1);
	gpio_set_value(item.gpio.gpio, 0);
#endif

	drv = devm_kzalloc(&pdev->dev, sizeof(*drv), GFP_ATOMIC);
	if (!drv) {
		pr_err("allocation failed\n");
		return -ENOMEM;
	}

	drv->intr_debounce = SUN8IW9_INTR_DEBOUNCE;
	drv->hs_insert_det_delay = SUN8IW9_HS_INSERT_DET_DELAY;
	drv->hs_remove_det_delay = SUN8IW9_HS_REMOVE_DET_DELAY;
	drv->button_det_delay = SUN8IW9_BUTTON_DET_DELAY;
	drv->hs_det_poll_intrvl = SUN8IW9_HS_DET_POLL_INTRVL;
	drv->hs_det_retry = SUN8IW9_HS_DET_RETRY_COUNT;
	drv->button_en_delay = SUN8IW9_BUTTON_EN_DELAY;
	drv->process_button_events = false;

	INIT_DELAYED_WORK(&drv->hs_insert_work, sun8iw9_check_hs_insert_status);
	INIT_DELAYED_WORK(&drv->hs_remove_work, sun8iw9_check_hs_remove_status);
	INIT_DELAYED_WORK(&drv->hs_button_work, sun8iw9_check_hs_button_status);
	INIT_DELAYED_WORK(&drv->hs_button_en_work, sun8iw9_enable_hs_button_events);

	mutex_init(&drv->jack_mlock);

	/* register the soc card */
	snd_soc_sunxi_sndpcm.dev = &pdev->dev;
	snd_soc_card_set_drvdata(&snd_soc_sunxi_sndpcm, drv);
	ret_val = snd_soc_register_card(&snd_soc_sunxi_sndpcm);
	if (ret_val) {
		pr_err("snd_soc_register_card failed %d\n", ret_val);
		return ret_val;
	}
	platform_set_drvdata(pdev, &snd_soc_sunxi_sndpcm);
	pr_info("%s successful\n", __func__);
	return ret_val;
}

static void snd_sun8iw9_unregister_jack(struct sun8iw9_mc_private *ctx)
{
	/* Set process button events to false so that the button
	   delayed work will not be scheduled.*/
	ctx->process_button_events = false;
	cancel_delayed_work_sync(&ctx->hs_insert_work);
	cancel_delayed_work_sync(&ctx->hs_button_en_work);
	cancel_delayed_work_sync(&ctx->hs_button_work);
	cancel_delayed_work_sync(&ctx->hs_remove_work);
	snd_soc_jack_free_gpios(&ctx->jack, 1, &hs_gpio);
}

static void sun8iw9_machine_shutdown(struct platform_device *pdev)
{
	struct snd_soc_card *soc_card = platform_get_drvdata(pdev);
	struct sun8iw9_mc_private *drv = snd_soc_card_get_drvdata(soc_card);

	HS_DBG("In %s\n", __func__);
	snd_sun8iw9_unregister_jack(drv);
}

/*data relating*/
static struct platform_device sun8iw9_machine_device = {
	.name = "sun8iw9_machine",
	.id = -1,
};

/*method relating*/
static struct platform_driver sun8iw9_machine_driver = {
	.driver = {
		.name = "sun8iw9_machine",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
	},
	.probe = sun8iw9_machine_probe,
	.shutdown = sun8iw9_machine_shutdown,
};

static int __init sun8iw9_machine_init(void)
{
	int err = 0;

	if((err = platform_device_register(&sun8iw9_machine_device)) < 0)
		return err;

	if ((err = platform_driver_register(&sun8iw9_machine_driver)) < 0)
		return err;

	return 0;
}
module_init(sun8iw9_machine_init);

static void __exit sun8iw9_machine_exit(void)
{
	platform_driver_unregister(&sun8iw9_machine_driver);
}
module_exit(sun8iw9_machine_exit);

MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUNXI_sndpcm ALSA SoC audio driver");
MODULE_LICENSE("GPL");

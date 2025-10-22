// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-jack"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/input.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "snd_sunxi_jack.h"

#define DETWORK_DTIME	10

static struct sunxi_jack sunxi_jack;

static irqreturn_t jack_interrupt(int irq, void *dev_id)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp = &jack_codec->jack_sdbp;

	SND_LOG_DEBUG("\n");

	if (jack_sdbp->jack_sdbp_method == SDBP_IRQ && jack_sdbp->is_working) {
		if (jack_sdbp->jack_sdbp_irq_clean)
			jack_sdbp->jack_sdbp_irq_clean(jack_codec->data);
		schedule_work(&jack_sdbp->sdbp_irq_work);
	} else {
		if (jack_codec->jack_irq_clean)
			jack_codec->jack_irq_clean(jack_codec->data);
		schedule_work(&sunxi_jack.det_irq_work);
	}

	return IRQ_HANDLED;
}

static void sunxi_jack_sdbp_start(void)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp = &jack_codec->jack_sdbp;

	SND_LOG_DEBUG("\n");

	if (jack_sdbp->jack_sdbp_method == SDBP_IRQ) {
		if (jack_sdbp->jack_sdbp_irq_init)
			jack_sdbp->jack_sdbp_irq_init(jack_codec->data);
	}

	if (jack_sdbp->jack_sdbp_method == SDBP_SCAN) {
		if (jack_sdbp->jack_sdbp_scan_init)
			jack_sdbp->jack_sdbp_scan_init(jack_codec->data);

		jack_sdbp->sdbp_scan_hrt.kt = ktime_set(0, jack_sdbp->jack_sdbp_scan_single_time
							* 1000000);
		hrtimer_start(&jack_sdbp->sdbp_scan_hrt.timer,
			      jack_sdbp->sdbp_scan_hrt.kt, HRTIMER_MODE_REL);
	}

	jack_sdbp->is_working = true;
}

static void sunxi_jack_sdbp_stop(void)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp = &jack_codec->jack_sdbp;

	SND_LOG_DEBUG("\n");

	if (jack_sdbp->jack_sdbp_method == SDBP_IRQ) {
		if (jack_sdbp->jack_sdbp_irq_exit)
			jack_sdbp->jack_sdbp_irq_exit(jack_codec->data);
	} else if (jack_sdbp->jack_sdbp_method == SDBP_SCAN) {
		if (jack_sdbp->jack_sdbp_scan_exit)
			jack_sdbp->jack_sdbp_scan_exit(jack_codec->data);
	}

	jack_sdbp->is_working = false;
}

static enum hrtimer_restart sunxi_jack_sdbp_scan_handler(struct hrtimer *timer)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp = &jack_codec->jack_sdbp;

	SND_LOG_DEBUG("\n");

	if (!jack_sdbp->is_working) {
		sunxi_jack_sdbp_stop();
		return HRTIMER_NORESTART;
	}

	schedule_work(&jack_sdbp->sdbp_scan_work);
	hrtimer_forward_now(&jack_sdbp->sdbp_scan_hrt.timer, jack_sdbp->sdbp_scan_hrt.kt);

	return HRTIMER_RESTART;
}

static void sunxi_jack_sdbp_scan_work(struct work_struct *work)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp = &jack_codec->jack_sdbp;
	unsigned long flags;

	SND_LOG_DEBUG("\n");

	if (sunxi_jack.type == SND_JACK_HEADPHONE && jack_sdbp->jack_sdbp_scan_work) {
		jack_sdbp->jack_sdbp_scan_work(jack_codec->data, &sunxi_jack.type);
		if (sunxi_jack.type == SND_JACK_HEADSET) {
			SND_LOG_INFO("[sound] jack update -> HEADSET\n");
			snd_jack_report(sunxi_jack.jack.jack, 0);
			snd_jack_report(sunxi_jack.jack.jack, SND_JACK_HEADSET);

			spin_lock_irqsave(&jack_sdbp->sdbp_lock, flags);
			sunxi_jack_sdbp_stop();
			spin_unlock_irqrestore(&jack_sdbp->sdbp_lock, flags);

			sunxi_jack.type_old = sunxi_jack.type;
		}
		/* "report-OUT" is performed in sunxi_jack_det_irq_work() */
	}
}

static void sunxi_jack_sdbp_irq_work(struct work_struct *work)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp = &jack_codec->jack_sdbp;
	unsigned long flags;

	SND_LOG_DEBUG("\n");

	if (jack_sdbp->jack_sdbp_irq_work)
		jack_sdbp->jack_sdbp_irq_work(jack_codec->data, &sunxi_jack.type);

	if (sunxi_jack.type == SND_JACK_HEADSET && sunxi_jack.type_old == SND_JACK_HEADPHONE) {
		spin_lock_irqsave(&jack_sdbp->sdbp_lock, flags);
		sunxi_jack_sdbp_stop();
		spin_unlock_irqrestore(&jack_sdbp->sdbp_lock, flags);

		/* The status switch between headphone and headset needs to report jack_out first */
		snd_jack_report(sunxi_jack.jack.jack, 0);
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);

		SND_LOG_INFO("[sound] jack report -> HEADSET\n");
	} else if (sunxi_jack.type == 0) {
		spin_lock_irqsave(&jack_sdbp->sdbp_lock, flags);
		sunxi_jack_sdbp_stop();
		spin_unlock_irqrestore(&jack_sdbp->sdbp_lock, flags);

		snd_jack_report(sunxi_jack.jack.jack, 0);
		SND_LOG_INFO("[sound] jack report -> OUT\n");
	} else {
		SND_LOG_INFO("[sound] jack type -> other:%d\n", sunxi_jack.type);
	}

	sunxi_jack.type_old = sunxi_jack.type;
}

static void sunxi_jack_det_irq_work(struct work_struct *work)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp = &jack_codec->jack_sdbp;
	bool report_out = false;
	unsigned long flags;

	SND_LOG_DEBUG("\n");

	mutex_lock(&sunxi_jack.det_mutex);

	if (jack_codec->jack_det_irq_work)
		jack_codec->jack_det_irq_work(jack_codec->data, &sunxi_jack.type);

	switch (sunxi_jack.system_sta) {
	case JACK_SYS_STA_INIT:
		sunxi_jack.system_sta = JACK_SYS_STA_NORMAL;
	break;
	case JACK_SYS_STA_RESUME:
		/* The status switch between headphone and headset needs to report jack_out first */
		if ((sunxi_jack.type == SND_JACK_HEADPHONE &&
		    sunxi_jack.type_old == SND_JACK_HEADSET) ||
		    (sunxi_jack.type == SND_JACK_HEADSET &&
		    sunxi_jack.type_old == SND_JACK_HEADPHONE)) {
			sunxi_jack.system_sta = JACK_SYS_STA_NORMAL;
			report_out = true;
		} else if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG("jack report -> unchange\n");
			mutex_unlock(&sunxi_jack.det_mutex);
			return;
		}
	break;
	case JACK_SYS_STA_NORMAL:
		if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG("jack report -> unchange\n");
			mutex_unlock(&sunxi_jack.det_mutex);
			return;
		}
	break;
	default:
		SND_LOG_DEBUG("jack setup status is invaild\n");
	break;
	}

	if (jack_codec->jack_status_sync)
		jack_codec->jack_status_sync(jack_codec->data, sunxi_jack.type);

	snd_sunxi_jack_state_upto_modparam(sunxi_jack.type);
	mutex_unlock(&sunxi_jack.det_mutex);

	if (report_out)
		snd_jack_report(sunxi_jack.jack.jack, 0);
	snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
	if (sunxi_jack.type == 0) {
		SND_LOG_INFO("[sound] jack report -> OUT\n");
	} else if (sunxi_jack.type == SND_JACK_HEADSET) {
		SND_LOG_INFO("[sound] jack report -> HEADSET\n");
	} else if (sunxi_jack.type == SND_JACK_HEADPHONE) {
		SND_LOG_INFO("[sound] jack report -> HEADPHONE\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_0)) {
		sunxi_jack.type &= ~SND_JACK_BTN_0;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		SND_LOG_INFO("[sound] jack report -> Hook\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_1)) {
		sunxi_jack.type &= ~SND_JACK_BTN_1;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		SND_LOG_INFO("[sound] jack report -> Volume ++\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_2)) {
		sunxi_jack.type &= ~SND_JACK_BTN_2;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		SND_LOG_INFO("[sound] jack report -> Volume --\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_3)) {
		sunxi_jack.type &= ~SND_JACK_BTN_3;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		SND_LOG_INFO("[sound] jack report -> Voice Assistant\n");
	} else {
		SND_LOG_INFO("[sound] jack report -> others 0x%x\n", sunxi_jack.type);
	}

	sunxi_jack.type_old = sunxi_jack.type;

	spin_lock_irqsave(&jack_sdbp->sdbp_lock, flags);
	if (sunxi_jack.type == SND_JACK_HEADPHONE && jack_sdbp->jack_sdbp_method != SDBP_NONE)
		sunxi_jack_sdbp_start();
	/* when jack_sdbp_method == SDBP_SCAN will execute it */
	if (sunxi_jack.type == 0 && jack_sdbp->is_working)
		sunxi_jack_sdbp_stop();
	spin_unlock_irqrestore(&jack_sdbp->sdbp_lock, flags);
}

static void sunxi_jack_det_scan_work(struct work_struct *work)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;
	bool report_out = false;

	SND_LOG_DEBUG("\n");

	mutex_lock(&sunxi_jack.det_mutex);
	if (jack_codec->jack_det_scan_work)
		jack_codec->jack_det_scan_work(jack_codec->data, &sunxi_jack.type);

	switch (sunxi_jack.system_sta) {
	case JACK_SYS_STA_INIT:
		sunxi_jack.system_sta = JACK_SYS_STA_NORMAL;
	break;
	case JACK_SYS_STA_RESUME:
		if ((sunxi_jack.type == SND_JACK_HEADPHONE &&
		    sunxi_jack.type_old == SND_JACK_HEADSET) ||
		    (sunxi_jack.type == SND_JACK_HEADSET &&
		    sunxi_jack.type_old == SND_JACK_HEADPHONE)) {
			sunxi_jack.system_sta = JACK_SYS_STA_NORMAL;
			report_out = true;
		} else if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG("jack report -> unchange\n");
			mutex_unlock(&sunxi_jack.det_mutex);
			return;
		}
	break;
	case JACK_SYS_STA_NORMAL:
		if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG("jack report -> unchange\n");
			mutex_unlock(&sunxi_jack.det_mutex);
			return;
		}
	break;
	default:
		SND_LOG_DEBUG("jack setup status is invaild\n");
	break;
	}

	if (jack_codec->jack_status_sync)
		jack_codec->jack_status_sync(jack_codec->data, sunxi_jack.type);

	snd_sunxi_jack_state_upto_modparam(sunxi_jack.type);
	mutex_unlock(&sunxi_jack.det_mutex);

	if (report_out)
		snd_jack_report(sunxi_jack.jack.jack, 0);
	snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
	if (sunxi_jack.type == 0) {
		SND_LOG_INFO("[sound] jack report -> OUT\n");
	} else if (sunxi_jack.type == SND_JACK_HEADSET) {
		SND_LOG_INFO("[sound] jack report -> HEADSET\n");
	} else if (sunxi_jack.type == SND_JACK_HEADPHONE) {
		SND_LOG_INFO("[sound] jack report -> HEADPHONE\n");
	} else {
		SND_LOG_INFO("[sound] jack report -> others 0x%x\n", sunxi_jack.type);
	}

	sunxi_jack.type_old = sunxi_jack.type;
}

static int sunxi_jack_suspend(struct snd_soc_card *card)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;

	SND_LOG_DEBUG("\n");

	disable_irq(sunxi_jack.jack_irq);

	if (jack_codec) {
		if (jack_codec->jack_suspend) {
			SND_LOG_INFO("%s, %d\n", __func__, __LINE__);
			jack_codec->jack_suspend(jack_codec->data);
		}
		if (jack_codec->jack_exit)
			jack_codec->jack_exit(jack_codec->data);
	}

	return 0;
}

static int sunxi_jack_resume(struct snd_soc_card *card)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;

	SND_LOG_DEBUG("\n");

	sunxi_jack.system_sta = JACK_SYS_STA_RESUME;

	enable_irq(sunxi_jack.jack_irq);

	if (jack_codec) {
		if (jack_codec->jack_init)
			jack_codec->jack_init(jack_codec->data);

		if (jack_codec->jack_resume)
			jack_codec->jack_resume(jack_codec->data);

		schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));
	}

	return 0;
}

static void sunxi_jack_dts_params_init(struct platform_device *pdev)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp = &jack_codec->jack_sdbp;
	struct device_node *np = pdev->dev.of_node;
	unsigned int temp_val;
	int ret = 0;

	SND_LOG_DEBUG("\n");

	ret = of_property_read_u32(np, "jack-sdbp-method", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-sdbp-method default SDBP_NONE\n");
		jack_sdbp->jack_sdbp_method = SDBP_NONE;
	} else {
		if (temp_val >= SDBP_METHOD_CNT) {
			SND_LOG_DEBUG("jack-sdbp-method invalid\n");
			jack_sdbp->jack_sdbp_method = SDBP_NONE;
		}
		jack_sdbp->jack_sdbp_method = temp_val;
	}

	if (jack_sdbp->jack_sdbp_method == SDBP_SCAN) {
		ret = of_property_read_u32(np, "jack-sdbp-scan-single-time", &temp_val);
		if (ret < 0) {
			SND_LOG_DEBUG("jack-sdbp-scan-single-time get failed\n");
			jack_sdbp->jack_sdbp_method = SDBP_NONE;
		} else {
			if (temp_val < 1000) {
				SND_LOG_DEBUG("jack-sdbp-scan-single-time invalid\n");
				jack_sdbp->jack_sdbp_method = SDBP_NONE;
			}
			jack_sdbp->jack_sdbp_scan_single_time = temp_val;
		}
	}

	if (jack_sdbp->jack_sdbp_method != SDBP_NONE) {
		SND_LOG_DEBUG("jack-sdbp-method            -> %u\n",
			      jack_sdbp->jack_sdbp_method);
		if (jack_sdbp->jack_sdbp_method == SDBP_SCAN)
			SND_LOG_DEBUG("jack-sdbp-scan-single-time  -> %u\n",
				      jack_sdbp->jack_sdbp_scan_single_time);
	}
}

/*******************************************************************************
 * for codec
 ******************************************************************************/
int snd_sunxi_jack_codec_init(void *jack_data)
{
	int ret;
	struct sunxi_jack_codec *jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp;

	SND_LOG_DEBUG("\n");

	if (!jack_data) {
		SND_LOG_ERR("jack_data is invaild\n");
		return -1;
	}
	jack_codec = jack_data;
	sunxi_jack.jack_codec = jack_codec;
	jack_sdbp = &jack_codec->jack_sdbp;

	sunxi_jack_dts_params_init(jack_codec->pdev);

	if (jack_codec->jack_init) {
		ret = jack_codec->jack_init(jack_codec->data);
		if (ret < 0) {
			SND_LOG_ERR("jack_init failed\n");
			return -1;
		}
	}

	mutex_init(&sunxi_jack.det_mutex);
	INIT_WORK(&sunxi_jack.det_irq_work, sunxi_jack_det_irq_work);
	INIT_DELAYED_WORK(&sunxi_jack.det_sacn_work, sunxi_jack_det_scan_work);

	if (jack_sdbp->jack_sdbp_method == SDBP_SCAN) {
		spin_lock_init(&jack_sdbp->sdbp_lock);
		INIT_WORK(&jack_sdbp->sdbp_scan_work, sunxi_jack_sdbp_scan_work);
		hrtimer_init(&jack_sdbp->sdbp_scan_hrt.timer, CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL);
		jack_sdbp->sdbp_scan_hrt.timer.function = sunxi_jack_sdbp_scan_handler;
	} else if (jack_sdbp->jack_sdbp_method == SDBP_IRQ) {
		spin_lock_init(&jack_sdbp->sdbp_lock);
		INIT_WORK(&jack_sdbp->sdbp_irq_work, sunxi_jack_sdbp_irq_work);
	}

	sunxi_jack.jack_irq = platform_get_irq(jack_codec->pdev, 0);
	if (sunxi_jack.jack_irq < 0) {
		SND_LOG_ERR("platform_get_irq failed\n");
		return -ENODEV;
	}

	ret = request_irq(sunxi_jack.jack_irq, jack_interrupt, IRQF_TRIGGER_NONE, "jack irq", NULL);
	if (ret < 0) {
		SND_LOG_ERR("request_irq failed\n");
		return -1;
	}

	/* system startup detection */
	sunxi_jack.system_sta = JACK_SYS_STA_INIT;
	/* recheck after 2s to prevent false triggering of interrupts */
	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(2000));

	return 0;
}

void snd_sunxi_jack_codec_exit(void *jack_data)
{
	struct sunxi_jack_codec *jack_codec;
	struct sunxi_jack_sdbp *jack_sdbp;

	SND_LOG_DEBUG("\n");

	if (!jack_data) {
		SND_LOG_ERR("jack_data is invaild\n");
		return;
	}
	jack_codec = jack_data;
	jack_sdbp = &jack_codec->jack_sdbp;

	free_irq(sunxi_jack.jack_irq, NULL);
	if (jack_codec->jack_exit)
		jack_codec->jack_exit(jack_codec->data);

	cancel_work_sync(&sunxi_jack.det_irq_work);
	cancel_delayed_work_sync(&sunxi_jack.det_sacn_work);

	if (jack_sdbp->jack_sdbp_method == SDBP_SCAN) {
		cancel_work_sync(&jack_sdbp->sdbp_scan_work);
		hrtimer_cancel(&jack_sdbp->sdbp_scan_hrt.timer);
	} else if (jack_sdbp->jack_sdbp_method == SDBP_IRQ) {
		cancel_work_sync(&jack_sdbp->sdbp_irq_work);
	}
	mutex_destroy(&sunxi_jack.det_mutex);

	return;
}

/*******************************************************************************
 * for machcine
 ******************************************************************************/
int snd_sunxi_jack_codec_register(struct snd_soc_card *card)
{
	int ret;

	SND_LOG_DEBUG("\n");

	if (!card) {
		SND_LOG_ERR("snd_soc_card is invaild\n");
		return -1;
	}
	sunxi_jack.card = card;

	sunxi_jack.type = 0;
	sunxi_jack.type_old = 0;
	sunxi_jack.system_sta = JACK_SYS_STA_INIT;
	ret = snd_sunxi_card_jack_new(sunxi_jack.card, "Headphones",
				      SND_JACK_HEADSET
				      | SND_JACK_HEADPHONE
				      | SND_JACK_BTN_0
				      | SND_JACK_BTN_1
				      | SND_JACK_BTN_2
				      | SND_JACK_BTN_3,
				      &sunxi_jack.jack);
	if (ret) {
		SND_LOG_ERR("snd_soc_card_jack_new failed\n");
		return -1;
	}

	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);
	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_3, KEY_VOICECOMMAND);

	card->suspend_pre = sunxi_jack_suspend;
	card->resume_post = sunxi_jack_resume;

	return 0;
}

void snd_sunxi_jack_codec_unregister(struct snd_soc_card *card)
{
	SND_LOG_DEBUG("\n");

	/* sunxi_jack.card = NULL; */

	return;
}

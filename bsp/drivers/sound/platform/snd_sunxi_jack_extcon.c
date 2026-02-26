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
#include <linux/extcon.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "snd_sunxi_jack.h"
#include "snd_sunxi_adapter.h"

#define DETWORK_DTIME	10

static struct sunxi_jack sunxi_jack;

/* for mic det */
static irqreturn_t jack_interrupt(int irq, void *dev_id)
{
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;

	SND_LOG_DEBUG("\n");

	jack_extcon->jack_irq_clean(jack_extcon->data);

	schedule_work(&sunxi_jack.det_irq_work);

	return IRQ_HANDLED;
}

/* for hp det */
static int sunxi_jack_plugin_notifier(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct sunxi_jack_extcon *jack_extcon = container_of(nb, struct sunxi_jack_extcon, hp_nb);

	SND_LOG_DEBUG("event -> %lu\n", event);

	if (event)
		jack_extcon->jack_plug_sta = JACK_PLUG_STA_IN;
	else
		jack_extcon->jack_plug_sta = JACK_PLUG_STA_OUT;

	if (jack_extcon->jack_irq_clean) {
		jack_extcon->jack_irq_clean(jack_extcon->data);
	} else {
		SND_LOG_DEBUG("jack_irq_clean func is unused\n");
	}

	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));

	return NOTIFY_DONE;
}

static void sunxi_jack_det_irq_work(struct work_struct *work)
{
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_extcon->jack_typec_cfg;
	bool report_out = false;

	SND_LOG_DEBUG("\n");

	if (jack_extcon->jack_plug_sta == JACK_PLUG_STA_IN) {
		sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_HP);
		SND_LOG_DEBUG("typec mode set to audio\n");
		goto jack_plug_in;
	} else {
		sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_USB);
		SND_LOG_DEBUG("typec mode set to usb\n");
		goto jack_plug_out;
	}

jack_plug_in:
	mutex_lock(&sunxi_jack.det_mutex);
	if (jack_extcon->jack_det_irq_work) {
		jack_extcon->jack_det_irq_work(jack_extcon->data, &sunxi_jack.type);
	} else {
		SND_LOG_ERR("jack_det_irq_work func is invaild\n");
	}

	mutex_unlock(&sunxi_jack.det_mutex);
	goto jack_report;

jack_plug_out:
	sunxi_jack.type = 0;

jack_report:
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
			return;
		}
	break;
	case JACK_SYS_STA_NORMAL:
		if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG("jack report -> unchange\n");
			return;
		}
	break;
	default:
		SND_LOG_DEBUG("jack setup status is invaild\n");
	break;
	}

	if (jack_extcon->jack_status_sync)
		jack_extcon->jack_status_sync(jack_extcon->data, sunxi_jack.type);

	snd_sunxi_jack_state_upto_modparam(sunxi_jack.type);
	if (report_out)
		snd_jack_report(sunxi_jack.jack.jack, 0);
	snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
	if (sunxi_jack.type == 0) {
		printk("[sound] jack report -> OUT\n");
	} else if (sunxi_jack.type == SND_JACK_HEADSET) {
		printk("[sound] jack report -> HEADSET\n");
	} else if (sunxi_jack.type == SND_JACK_HEADPHONE) {
		printk("[sound] jack report -> HEADPHONE\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_0)) {
		sunxi_jack.type &= ~SND_JACK_BTN_0;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		printk("[sound] jack report -> Hook\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_1)) {
		sunxi_jack.type &= ~SND_JACK_BTN_1;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		printk("[sound] jack report -> Volume ++\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_2)) {
		sunxi_jack.type &= ~SND_JACK_BTN_2;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		printk("[sound] jack report -> Volume --\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_3)) {
		sunxi_jack.type &= ~SND_JACK_BTN_3;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		printk("[sound] jack report -> Voice Assistant\n");
	} else {
		printk("[sound] jack report -> others 0x%x\n", sunxi_jack.type);
	}

	sunxi_jack.type_old = sunxi_jack.type;
}

static void sunxi_jack_det_scan_work(struct work_struct *work)
{
	int ret;
	bool report_out = false;
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_extcon->jack_typec_cfg;

	SND_LOG_DEBUG("\n");

	ret = extcon_get_state(jack_extcon->extdev, EXTCON_JACK_HEADPHONE);
	SND_LOG_DEBUG("jack extcon state %d\n", ret);
	if (ret) {
		jack_extcon->jack_plug_sta = JACK_PLUG_STA_IN;
		sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_HP);
		SND_LOG_DEBUG("typec mode set to audio\n");
		goto jack_plug_in;
	} else {
		jack_extcon->jack_plug_sta = JACK_PLUG_STA_OUT;
		sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_USB);
		SND_LOG_DEBUG("typec mode set to usb\n");
		goto jack_plug_out;
	}

jack_plug_in:
	mutex_lock(&sunxi_jack.det_mutex);
	if (jack_extcon->jack_det_scan_work) {
		jack_extcon->jack_det_scan_work(jack_extcon->data, &sunxi_jack.type);
	} else {
		SND_LOG_ERR("jack_det_scan_work func is invaild\n");
	}

	mutex_unlock(&sunxi_jack.det_mutex);
	goto jack_report;

jack_plug_out:
	sunxi_jack.type = 0;

jack_report:
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
			return;
		}
	break;
	case JACK_SYS_STA_NORMAL:
		if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG("jack report -> unchange\n");
			return;
		}
	break;
	default:
		SND_LOG_DEBUG("jack setup status is invaild\n");
	break;
	}

	if (jack_extcon->jack_status_sync)
		jack_extcon->jack_status_sync(jack_extcon->data, sunxi_jack.type);

	snd_sunxi_jack_state_upto_modparam(sunxi_jack.type);
	if (report_out)
		snd_jack_report(sunxi_jack.jack.jack, 0);
	snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
	if (sunxi_jack.type == 0) {
		printk("[sound] jack report -> OUT\n");
	} else if (sunxi_jack.type == SND_JACK_HEADSET) {
		printk("[sound] jack report -> HEADSET\n");
	} else if (sunxi_jack.type == SND_JACK_HEADPHONE) {
		printk("[sound] jack report -> HEADPHONE\n");
	} else {
		printk("[sound] jack report -> others 0x%x\n", sunxi_jack.type);
	}

	sunxi_jack.type_old = sunxi_jack.type;
}

static int sunxi_jack_suspend(struct snd_soc_card *card)
{
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;

	SND_LOG_DEBUG("\n");

	disable_irq(sunxi_jack.jack_irq);

	if (jack_extcon) {
		if (jack_extcon->jack_suspend) {
			jack_extcon->jack_suspend(jack_extcon->data);
		} else {
			SND_LOG_ERR("jack_suspend func is invaild\n");
		}
	}

	return 0;
}

static int sunxi_jack_resume(struct snd_soc_card *card)
{
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;

	SND_LOG_DEBUG("\n");

	sunxi_jack.system_sta = JACK_SYS_STA_RESUME;

	enable_irq(sunxi_jack.jack_irq);

	if (jack_extcon) {
		if (jack_extcon->jack_init)
			jack_extcon->jack_init(jack_extcon->data);
		else
			SND_LOG_ERR("jack_init func is invaild\n");

		if (jack_extcon->jack_resume)
			jack_extcon->jack_resume(jack_extcon->data);
		else
			SND_LOG_ERR("jack_resume func is invaild\n");

		schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));
	}

	return 0;
}

static int sunxi_jack_typec_init(struct sunxi_jack_extcon *jack_extcon)
{
	int ret, i;
	unsigned int temp_val;
	char str[32] = {0};
	struct platform_device *pdev = jack_extcon->pdev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_extcon->jack_typec_cfg;
	uint32_t *map_value;

	struct {
		char *name;
		unsigned int id;
	} of_mode_table[] = {
		{ "jack-mode-off",	SND_JACK_MODE_OFF },
		{ "jack-mode-usb",	SND_JACK_MODE_USB },
		{ "jack-mode-hp",	SND_JACK_MODE_HP },
		{ "jack-mode-micn",	SND_JACK_MODE_MICN },
		{ "jack-mode-mici",	SND_JACK_MODE_MICI },
	};

	SND_LOG_DEBUG("\n");

	ret = of_property_read_u32(np, "jack-swpin-max", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("jack-swpin-max get failed,stop init\n");
		return 0;
	} else {
		jack_typec_cfg->sw_pin_max = temp_val;
	}

	jack_typec_cfg->jack_pins = devm_kcalloc(&pdev->dev, jack_typec_cfg->sw_pin_max,
						 sizeof(struct sunxi_jack_pins), GFP_KERNEL);
	if (!jack_typec_cfg->jack_pins) {
		SND_LOG_ERR("can't get pin_config memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < jack_typec_cfg->sw_pin_max; ++i) {
		snprintf(str, sizeof(str), "jack-swpin-%d", i);
		ret = of_get_named_gpio(np, str, 0);
		if (ret < 0) {
			SND_LOG_ERR("%s get failed\n", str);
			jack_typec_cfg->jack_pins[i].used = false;
			continue;
		}
		temp_val = ret;
		if (!gpio_is_valid(temp_val)) {
			SND_LOG_ERR("%s (%u) is invalid\n", str, temp_val);
			jack_typec_cfg->jack_pins[i].used = false;
			continue;
		}
		ret = devm_gpio_request(&pdev->dev, temp_val, str);
		if (ret) {
			SND_LOG_ERR("%s (%u) request failed\n", str, temp_val);
			jack_typec_cfg->jack_pins[i].used = false;
			continue;
		}
		jack_typec_cfg->jack_pins[i].used = true;
		jack_typec_cfg->jack_pins[i].pin = temp_val;
		/* pin default set to output */
		gpio_direction_output(jack_typec_cfg->jack_pins[i].pin, 1);
	}

	jack_typec_cfg->modes_map = devm_kcalloc(&pdev->dev, SND_JACK_MODE_CNT,
						 sizeof(struct sunxi_jack_modes_map),
						 GFP_KERNEL);
	if (!jack_typec_cfg->modes_map) {
		SND_LOG_ERR("can't get pin_mode_cfg memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(of_mode_table); ++i) {
		map_value = devm_kcalloc(&pdev->dev, jack_typec_cfg->sw_pin_max, sizeof(uint32_t),
					 GFP_KERNEL);
		if (!map_value) {
			SND_LOG_ERR("can't get map value memory\n");
			return -ENOMEM;
		}

		ret = of_property_read_u32_array(np, of_mode_table[i].name,
						 map_value, jack_typec_cfg->sw_pin_max);
		if (ret) {
			jack_typec_cfg->modes_map[i].type = SND_JACK_MODE_NULL;
			SND_LOG_DEBUG("mode:%s get failed,will not set mode map\n",
				      of_mode_table[i].name);
		} else {
			jack_typec_cfg->modes_map[i].type = of_mode_table[i].id;
			jack_typec_cfg->modes_map[i].map_value = map_value;
		}
		map_value = NULL;
	}

	sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_USB);
	SND_LOG_DEBUG("typec mode set to usb\n");

	return 0;
}

static void sunxi_jack_typec_exit(struct sunxi_jack_extcon *jack_extcon)
{
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_extcon->jack_typec_cfg;

	SND_LOG_DEBUG("\n");

	sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_USB);
	SND_LOG_DEBUG("typec mode set to usb\n");
}

/*******************************************************************************
 * for codec or platform
 ******************************************************************************/
int snd_sunxi_jack_extcon_init(void *jack_data)
{
	int ret;
	struct device_node *np;
	struct platform_device *pdev;
	struct sunxi_jack_extcon *jack_extcon;

	SND_LOG_DEBUG("\n");

	if (!jack_data) {
		SND_LOG_ERR("jack_data is invaild\n");
		return -1;
	}
	jack_extcon = jack_data;
	sunxi_jack.jack_extcon = jack_extcon;

	if (jack_extcon->jack_init) {
		ret = jack_extcon->jack_init(jack_extcon->data);
		if (ret < 0) {
			SND_LOG_ERR("jack_init failed\n");
			return -1;
		}
	} else {
		SND_LOG_ERR("jack_init func is invaild\n");
	}

	mutex_init(&sunxi_jack.det_mutex);
	INIT_WORK(&sunxi_jack.det_irq_work, sunxi_jack_det_irq_work);
	INIT_DELAYED_WORK(&sunxi_jack.det_sacn_work, sunxi_jack_det_scan_work);

	/* for hp det only */
	pdev = jack_extcon->pdev;
	np = pdev->dev.of_node;
	if (of_property_read_bool(np, "extcon")) {
		jack_extcon->extdev = extcon_get_edev_by_phandle(&pdev->dev, 0);
		if (IS_ERR(jack_extcon->extdev)) {
			SND_LOG_ERR("get extcon dev failed\n");
			return -1;
		}
	} else {
		SND_LOG_ERR("get extcon failed\n");
		return -1;
	}
	jack_extcon->hp_nb.notifier_call = sunxi_jack_plugin_notifier;
	ret = extcon_register_notifier(jack_extcon->extdev, EXTCON_JACK_HEADPHONE, &jack_extcon->hp_nb);
	if (ret < 0) {
		SND_LOG_ERR("register jack notifier failed\n");
		return -1;
	}

	ret = sunxi_jack_typec_init(jack_extcon);
	if (ret < 0) {
		SND_LOG_ERR("typec jack init failed\n");
		return -1;
	}

	/* for mic det only */
	sunxi_jack.jack_irq = platform_get_irq(pdev, 0);
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
	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));

	return 0;
}

void snd_sunxi_jack_extcon_exit(void *jack_data)
{
	struct sunxi_jack_extcon *jack_extcon;

	SND_LOG_DEBUG("\n");

	if (!jack_data) {
		SND_LOG_ERR("jack_data is invaild\n");
		return;
	}
	jack_extcon = jack_data;

	free_irq(sunxi_jack.jack_irq, NULL);
	extcon_unregister_notifier(jack_extcon->extdev, EXTCON_JACK_HEADPHONE, &jack_extcon->hp_nb);

	if (jack_extcon->jack_exit) {
		jack_extcon->jack_exit(jack_extcon->data);
	} else {
		SND_LOG_ERR("jack_exit func is invaild\n");
	}

	cancel_work_sync(&sunxi_jack.det_irq_work);
	cancel_delayed_work_sync(&sunxi_jack.det_sacn_work);
	mutex_destroy(&sunxi_jack.det_mutex);

	sunxi_jack_typec_exit(jack_extcon);

	return;
}

/*******************************************************************************
 * for machcine
 ******************************************************************************/
int snd_sunxi_jack_extcon_register(struct snd_soc_card *card)
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

void snd_sunxi_jack_extcon_unregister(struct snd_soc_card *card)
{
	SND_LOG_DEBUG("\n");

	/* sunxi_jack.card = NULL; */

	return;
}

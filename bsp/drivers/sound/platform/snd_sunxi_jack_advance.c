// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2024 - 2029 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2024, Dby <dby@allwinnertech.com>
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
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/extcon.h>
#include <linux/power_supply.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "snd_sunxi_jack.h"

#define DETWORK_DTIME	10

static struct sunxi_jack sunxi_jack;

static irqreturn_t jack_interrupt(int irq, void *dev_id)
{
	struct sunxi_jack_adv *jack_adv = sunxi_jack.jack_adv;
	(void)dev_id;

	SND_LOG_INFO("irq: %d\n", irq);
	if (jack_adv->jack_irq_clean)
		jack_adv->jack_irq_clean(jack_adv->data, irq);
	schedule_work(&sunxi_jack.det_irq_work);

	return IRQ_HANDLED;
}

static void sunxi_jack_det_work(struct sunxi_jack_adv *jack_adv)
{
	bool report_out = false;

	SND_LOG_DEBUG("\n");

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

	if (jack_adv->jack_status_sync)
		jack_adv->jack_status_sync(jack_adv->data, sunxi_jack.type);

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

static void sunxi_jack_det_irq_work(struct work_struct *work)
{
	struct sunxi_jack_adv *jack_adv = sunxi_jack.jack_adv;

	SND_LOG_DEBUG("\n");
	if (jack_adv->jack_det_irq_work)
		jack_adv->jack_det_irq_work(jack_adv->data, &sunxi_jack.type);
	sunxi_jack_det_work(jack_adv);
}

static void sunxi_jack_det_scan_work(struct work_struct *work)
{
	struct sunxi_jack_adv *jack_adv = sunxi_jack.jack_adv;
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_adv->jack_typec_cfg;
	union power_supply_propval temp;

	SND_LOG_DEBUG("\n");
	if (jack_adv->typec && jack_adv->jack_plug_sta != JACK_PLUG_STA_IN) {
		if (!IS_ERR_OR_NULL(jack_adv->pmu_psy))
			power_supply_get_property(jack_adv->pmu_psy,
						  POWER_SUPPLY_PROP_SCOPE, &temp);
		if (temp.intval & (0x1 << 3)) {
			jack_adv->jack_plug_sta = JACK_PLUG_STA_IN;
			sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_HP);
			SND_LOG_INFO("typec mode set to hp\n");
		}
	}

	if (jack_adv->jack_det_scan_work)
		jack_adv->jack_det_scan_work(jack_adv->data, &sunxi_jack.type);
	sunxi_jack_det_work(jack_adv);
}

static int sunxi_jack_suspend(struct snd_soc_card *card)
{
	struct sunxi_jack_adv *jack_adv = sunxi_jack.jack_adv;

	SND_LOG_DEBUG("\n");

	if (!jack_adv) {
		SND_LOG_ERR("jack_adv is invaild\n");
		return 0;
	}
	if (jack_adv->jack_irq_disable)
		jack_adv->jack_irq_disable(jack_adv->data);
	if (jack_adv->jack_suspend)
		jack_adv->jack_suspend(jack_adv->data);

	return 0;
}

static int sunxi_jack_resume(struct snd_soc_card *card)
{
	struct sunxi_jack_adv *jack_adv = sunxi_jack.jack_adv;

	SND_LOG_DEBUG("\n");

	sunxi_jack.system_sta = JACK_SYS_STA_RESUME;

	if (!jack_adv) {
		SND_LOG_ERR("jack_adv is invaild\n");
		return 0;
	}

	if (jack_adv->jack_resume)
		jack_adv->jack_resume(jack_adv->data);
	if (jack_adv->jack_irq_enable)
		jack_adv->jack_irq_enable(jack_adv->data);

	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));

	return 0;
}

static int sunxi_jack_plugin_notifier(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct sunxi_jack_adv *jack_adv = container_of(nb, struct sunxi_jack_adv, hp_nb);
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_adv->jack_typec_cfg;

	SND_LOG_INFO("event -> %lu\n", event);

	if (event) {
		jack_adv->jack_plug_sta = JACK_PLUG_STA_IN;
		sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_HP);
		SND_LOG_INFO("typec mode set to hp\n");
	} else {
		jack_adv->jack_plug_sta = JACK_PLUG_STA_OUT;
		sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_USB);
		SND_LOG_INFO("typec mode set to usb\n");
	}

	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));

	return NOTIFY_DONE;
}

static int snd_sunxi_jack_adv_typec_init(struct sunxi_jack_adv *jack_adv)
{
	int ret, i;
	unsigned int temp_val;
	char str[32] = {0};
	struct device_node *np = jack_adv->dev->of_node;
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_adv->jack_typec_cfg;
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

	jack_typec_cfg->jack_pins = devm_kcalloc(jack_adv->dev, jack_typec_cfg->sw_pin_max,
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
		ret = devm_gpio_request(jack_adv->dev, temp_val, str);
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

	jack_typec_cfg->modes_map = devm_kcalloc(jack_adv->dev, SND_JACK_MODE_CNT,
						 sizeof(struct sunxi_jack_modes_map),
						 GFP_KERNEL);
	if (!jack_typec_cfg->modes_map) {
		SND_LOG_ERR("can't get pin_mode_cfg memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(of_mode_table); ++i) {
		map_value = devm_kcalloc(jack_adv->dev, jack_typec_cfg->sw_pin_max, sizeof(uint32_t),
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

static void snd_sunxi_jack_adv_typec_exit(struct sunxi_jack_adv *jack_adv)
{
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_adv->jack_typec_cfg;

	SND_LOG_DEBUG("\n");

	sunxi_jack_typec_mode_set(jack_typec_cfg, SND_JACK_MODE_USB);
	SND_LOG_DEBUG("typec mode set to usb\n");

	return;
}

/*******************************************************************************
 * for codec of platform
 ******************************************************************************/
int snd_sunxi_jack_adv_init(void *jack_data)
{
	struct sunxi_jack_adv *jack_adv;
	struct device_node *np;
	int ret;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(jack_data)) {
		SND_LOG_ERR("jack_data is invaild\n");
		return -1;
	}
	jack_adv = jack_data;
	sunxi_jack.jack_adv = jack_adv;

	if (jack_adv->jack_init) {
		ret = jack_adv->jack_init(jack_adv->data);
		if (ret < 0) {
			SND_LOG_ERR("jack_init failed\n");
			return -1;
		}
	}

	INIT_WORK(&sunxi_jack.det_irq_work, sunxi_jack_det_irq_work);
	INIT_DELAYED_WORK(&sunxi_jack.det_sacn_work, sunxi_jack_det_scan_work);

	np = jack_adv->dev->of_node;
	if (of_property_read_bool(np, "extcon")) {
		jack_adv->typec = true;

		jack_adv->extdev = extcon_get_edev_by_phandle(jack_adv->dev, 0);
		if (IS_ERR(jack_adv->extdev)) {
			SND_LOG_ERR("get adv extdev failed\n");
			return -1;
		}

		jack_adv->hp_nb.notifier_call = sunxi_jack_plugin_notifier;
		ret = extcon_register_notifier(jack_adv->extdev,
					EXTCON_JACK_HEADPHONE,
					&jack_adv->hp_nb);
		if (ret < 0) {
			SND_LOG_ERR("register jack notifier failed\n");
			return -1;
		}

		jack_adv->pmu_psy = devm_power_supply_get_by_phandle(jack_adv->dev, "extcon");

		ret = snd_sunxi_jack_adv_typec_init(jack_adv);
		if (ret < 0) {
			SND_LOG_ERR("typec jack init failed\n");
			return -1;
		}
	}

	if (jack_adv->jack_irq_requeset) {
		ret = jack_adv->jack_irq_requeset(jack_adv->data, jack_interrupt);
		if (ret < 0) {
			SND_LOG_ERR("jack_irq_requeset failed\n");
			return -1;
		}
	}

	sunxi_jack.system_sta = JACK_SYS_STA_INIT;
	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));

	return 0;
}

void snd_sunxi_jack_adv_exit(void *jack_data)
{
	struct sunxi_jack_adv *jack_adv;
	struct device_node *np;

	SND_LOG_DEBUG("\n");

	if (!jack_data) {
		SND_LOG_ERR("jack_data is invaild\n");
		return;
	}
	jack_adv = jack_data;

	if (jack_adv->jack_irq_free)
		jack_adv->jack_irq_free(jack_adv->data);

	cancel_work_sync(&sunxi_jack.det_irq_work);
	cancel_delayed_work_sync(&sunxi_jack.det_sacn_work);

	np = jack_adv->dev->of_node;
	if (of_property_read_bool(np, "extcon")) {
		extcon_unregister_notifier(jack_adv->extdev,
					   EXTCON_JACK_HEADPHONE,
					   &jack_adv->hp_nb);

		snd_sunxi_jack_adv_typec_exit(jack_adv);
	}

	if (jack_adv->jack_exit)
		jack_adv->jack_exit(jack_adv->data);
}

/*******************************************************************************
 * for machcine
 ******************************************************************************/
int snd_sunxi_jack_adv_register(struct snd_soc_card *card)
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
		return ret;
	}

	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);
	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_3, KEY_VOICECOMMAND);

	card->suspend_pre = sunxi_jack_suspend;
	card->resume_post = sunxi_jack_resume;

	return 0;
}

void snd_sunxi_jack_adv_unregister(struct snd_soc_card *card)
{
	SND_LOG_DEBUG("\n");

	return;
}

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
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "snd_sunxi_jack.h"

static struct sunxi_jack sunxi_jack;

static irqreturn_t jack_interrupt(int irq, void *data)
{
	(void)data;

	SND_LOG_DEBUG("\n");

	schedule_work(&sunxi_jack.det_irq_work);

	return IRQ_HANDLED;
}

static void sunxi_jack_det_work(struct sunxi_jack_gpio *jack_gpio)
{
	int enable;
	bool report_out = false;

	SND_LOG_DEBUG("\n");

	enable = gpiod_get_value_cansleep(jack_gpio->desc);
	if (jack_gpio->det_level)
		enable = !enable;
	if (enable)
		sunxi_jack.type = SND_JACK_HEADPHONE;
	else
		sunxi_jack.type = 0;

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

	if (jack_gpio->jack_status_sync)
		jack_gpio->jack_status_sync(jack_gpio->data, sunxi_jack.type);

	snd_sunxi_jack_state_upto_modparam(sunxi_jack.type);
	if (report_out)
		snd_jack_report(sunxi_jack.jack.jack, 0);
	snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);

	if (sunxi_jack.type == 0) {
		printk("[sound] jack report -> OUT\n");
	} else if (sunxi_jack.type == SND_JACK_HEADPHONE) {
		printk("[sound] jack report -> HEADPHONE\n");
	} else {
		printk("[sound] jack report -> others 0x%x\n", sunxi_jack.type);
	}

	sunxi_jack.type_old = sunxi_jack.type;
}

static void sunxi_jack_det_irq_work(struct work_struct *work)
{
	struct sunxi_jack_gpio *jack_gpio = sunxi_jack.jack_gpio;

	SND_LOG_DEBUG("\n");

	msleep(jack_gpio->debounce_time);
	sunxi_jack_det_work(jack_gpio);
}

static void sunxi_jack_det_scan_work(struct work_struct *work)
{
	struct sunxi_jack_gpio *jack_gpio = sunxi_jack.jack_gpio;

	SND_LOG_DEBUG("\n");

	sunxi_jack_det_work(jack_gpio);
}

static int sunxi_jack_suspend(struct snd_soc_card *card)
{
	struct sunxi_jack_gpio *jack_gpio = sunxi_jack.jack_gpio;

	SND_LOG_DEBUG("\n");

	if (jack_gpio) {
		disable_irq(gpiod_to_irq(jack_gpio->desc));
	}

	return 0;
}

static int sunxi_jack_resume(struct snd_soc_card *card)
{
	struct sunxi_jack_gpio *jack_gpio = sunxi_jack.jack_gpio;

	SND_LOG_DEBUG("\n");

	sunxi_jack.system_sta = JACK_SYS_STA_RESUME;

	if (jack_gpio) {
		enable_irq(gpiod_to_irq(jack_gpio->desc));

		schedule_delayed_work(&sunxi_jack.det_sacn_work,
				msecs_to_jiffies(jack_gpio->debounce_time));
	}

	return 0;
}

/*******************************************************************************
 * for codec of platform
 ******************************************************************************/
int snd_sunxi_jack_gpio_init(void *jack_data)
{
	struct device_node *of_node;
	struct sunxi_jack_gpio *jack_gpio;
	char *det_gpio_name = "hp-det-gpio";
	char *gpio_name	= "Headphone detection";
	unsigned int temp_val;
	int ret;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(jack_data)) {
		SND_LOG_ERR("jack_data is invaild\n");
		return -1;
	}
	jack_gpio = jack_data;
	sunxi_jack.jack_gpio = jack_gpio;

	if (IS_ERR_OR_NULL(jack_gpio->pdev)) {
		SND_LOG_ERR("jack_data pdev is invaild\n");
		return -1;
	}
	of_node = jack_gpio->pdev->dev.of_node;
	jack_gpio->det_gpio = of_get_named_gpio(of_node, det_gpio_name, 0);
	if (jack_gpio->det_gpio == -EPROBE_DEFER) {
		SND_LOG_ERR("get jack-detgpio failed\n");
		return -EPROBE_DEFER;
	}

	ret = of_property_read_u32(of_node, "jack-det-level", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("jack-det-level miss, default 0\n");
		jack_gpio->det_level = 0;
	} else {
		jack_gpio->det_level = temp_val;
	}

	if (!gpio_is_valid(jack_gpio->det_gpio)) {
		SND_LOG_ERR("jack-detgpio (%d) is invalid\n", jack_gpio->det_gpio);
		return -1;
	}

	jack_gpio->debounce_time = 200;

	ret = gpio_request_one(jack_gpio->det_gpio, GPIOF_IN, gpio_name);
	if (ret) {
		SND_LOG_ERR("jack-detgpio (%d) request failed\n", jack_gpio->det_gpio);
		return ret;
	}

	INIT_WORK(&sunxi_jack.det_irq_work, sunxi_jack_det_irq_work);
	INIT_DELAYED_WORK(&sunxi_jack.det_sacn_work, sunxi_jack_det_scan_work);

	jack_gpio->desc = gpio_to_desc(jack_gpio->det_gpio);
	ret = request_any_context_irq(gpiod_to_irq(jack_gpio->desc),
				      jack_interrupt,
				      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				      gpio_name,
				      jack_gpio);
	if (ret) {
		SND_LOG_ERR("jack-detgpio (%d) request irq failed\n", jack_gpio->det_gpio);
		return ret;
	}

	schedule_delayed_work(&sunxi_jack.det_sacn_work,
			      msecs_to_jiffies(jack_gpio->debounce_time));

	SND_LOG_DEBUG("det_gpio      %d\n", jack_gpio->det_gpio);
	SND_LOG_DEBUG("det_level     %d\n", jack_gpio->det_level);
	SND_LOG_DEBUG("debounce_time %u\n", jack_gpio->debounce_time);

	return 0;
}

void snd_sunxi_jack_gpio_exit(void *jack_data)
{
	struct sunxi_jack_gpio *jack_gpio;

	SND_LOG_DEBUG("\n");

	if (!jack_data) {
		SND_LOG_ERR("jack_data is invaild\n");
		return;
	}
	jack_gpio = jack_data;

	gpio_free(jack_gpio->det_gpio);

	gpiod_unexport(jack_gpio->desc);
	free_irq(gpiod_to_irq(jack_gpio->desc), jack_gpio);
	gpiod_put(jack_gpio->desc);
	cancel_work_sync(&sunxi_jack.det_irq_work);
	cancel_delayed_work_sync(&sunxi_jack.det_sacn_work);
}

/*******************************************************************************
 * for machcine
 ******************************************************************************/
int snd_sunxi_jack_gpio_register(struct snd_soc_card *card)
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
				      SND_JACK_HEADPHONE,
				      &sunxi_jack.jack);
	if (ret) {
		SND_LOG_ERR("snd_soc_card_jack_new failed\n");
		return ret;
	}

	card->suspend_pre = sunxi_jack_suspend;
	card->resume_post = sunxi_jack_resume;

	return 0;
}

void snd_sunxi_jack_gpio_unregister(struct snd_soc_card *card)
{
	SND_LOG_DEBUG("\n");

	return;
}

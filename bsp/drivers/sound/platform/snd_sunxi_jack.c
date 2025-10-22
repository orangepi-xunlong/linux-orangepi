// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, <lijingpsw@allwinnertech.com>
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

#include "snd_sunxi_jack.h"

static enum JACK_DET_METHOD g_jack_support;

int snd_sunxi_jack_init(struct sunxi_jack_port *sunxi_jack_port)
{
	int ret;

	SND_LOG_DEBUG("jack_support:%d\n", g_jack_support);

	if (!sunxi_jack_port) {
		SND_LOG_ERR("sunxi_jack_port is invaild\n");
		return -1;
	}

	if (!g_jack_support) {
		SND_LOG_WARN("unsupport jack!\n");
		return 0;
	}

	if (g_jack_support < JACK_DET_NONE || g_jack_support >= JACK_DET_CNT) {
		SND_LOG_ERR("error jack_support:%d!\n", g_jack_support);
		return -1;
	}

	if (g_jack_support == JACK_DET_CODEC) {
		ret = snd_sunxi_jack_codec_init((void *)sunxi_jack_port->jack_codec);
		if (ret) {
			SND_LOG_ERR("snd_sunxi_jack_codec_init failed\n");
			return ret;
		}
	} else if (g_jack_support == JACK_DET_EXTCON) {
		ret = snd_sunxi_jack_extcon_init((void *)sunxi_jack_port->jack_extcon);
		if (ret) {
			SND_LOG_ERR("snd_sunxi_jack_extcon_init failed\n");
			return ret;
		}
	} else if (g_jack_support == JACK_DET_GPIO) {
		ret = snd_sunxi_jack_gpio_init((void *)sunxi_jack_port->jack_gpio);
		if (ret) {
			SND_LOG_ERR("snd_sunxi_jack_gpio_init failed\n");
			return ret;
		}
	} else if (g_jack_support == JACK_DET_ADV) {
		ret = snd_sunxi_jack_adv_init((void *)sunxi_jack_port->jack_adv);
		if (ret) {
			SND_LOG_ERR("snd_sunxi_jack_adv_init failed\n");
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(snd_sunxi_jack_init);

int snd_sunxi_jack_exit(struct sunxi_jack_port *sunxi_jack_port)
{
	SND_LOG_DEBUG("\n");

	if (!sunxi_jack_port) {
		SND_LOG_ERR("sunxi_jack_port is invaild\n");
		return -1;
	}

	if (!g_jack_support) {
		SND_LOG_WARN("unsupport jack!\n");
		return 0;
	}

	if (g_jack_support < JACK_DET_NONE || g_jack_support >= JACK_DET_CNT) {
		SND_LOG_ERR("error jack_support:%d!\n", g_jack_support);
		return -1;
	}

	if (g_jack_support == JACK_DET_CODEC) {
		snd_sunxi_jack_codec_exit((void *)sunxi_jack_port->jack_codec);
	} else if (g_jack_support == JACK_DET_EXTCON) {
		snd_sunxi_jack_extcon_exit((void *)sunxi_jack_port->jack_extcon);
	} else if (g_jack_support == JACK_DET_GPIO) {
		snd_sunxi_jack_gpio_exit((void *)sunxi_jack_port->jack_gpio);
	} else if (g_jack_support == JACK_DET_ADV) {
		snd_sunxi_jack_adv_exit((void *)sunxi_jack_port->jack_adv);
	}

	return 0;
}
EXPORT_SYMBOL(snd_sunxi_jack_exit);

int snd_sunxi_jack_register(struct snd_soc_card *card, enum JACK_DET_METHOD jack_support)
{
	int ret;

	SND_LOG_DEBUG("jack_support:%d\n", jack_support);

	if (!card) {
		SND_LOG_ERR("snd_soc_card is invaild\n");
		return -1;
	}

	if (jack_support < JACK_DET_NONE || jack_support >= JACK_DET_CNT) {
		SND_LOG_ERR("error jack_support:%d\n", jack_support);
		return -1;
	}

	if (jack_support == JACK_DET_NONE) {
		SND_LOG_WARN("unsupport jack!\n");
		return 0;
	}

	g_jack_support = jack_support;
	if (g_jack_support == JACK_DET_CODEC) {
		ret = snd_sunxi_jack_codec_register(card);
		if (ret < 0) {
			SND_LOG_ERR("snd_sunxi_jack_codec_register failed\n");
			return ret;
		}
	} else if (g_jack_support == JACK_DET_EXTCON) {
		ret = snd_sunxi_jack_extcon_register(card);
		if (ret < 0) {
			SND_LOG_ERR("snd_sunxi_jack_extcon_register failed\n");
			return ret;
		}
	} else if (g_jack_support == JACK_DET_GPIO) {
		ret = snd_sunxi_jack_gpio_register(card);
		if (ret < 0) {
			SND_LOG_ERR("snd_sunxi_jack_gpio_register failed\n");
			return ret;
		}
	} else if (g_jack_support == JACK_DET_ADV) {
		ret = snd_sunxi_jack_adv_register(card);
		if (ret) {
			SND_LOG_ERR("snd_sunxi_jack_adv_register failed\n");
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(snd_sunxi_jack_register);

void snd_sunxi_jack_unregister(struct snd_soc_card *card, enum JACK_DET_METHOD jack_support)
{
	SND_LOG_DEBUG("\n");

	if (!card) {
		SND_LOG_ERR("snd_soc_card is invaild\n");
		return;
	}

	if (jack_support < JACK_DET_NONE || jack_support >= JACK_DET_CNT) {
		SND_LOG_ERR("error jack_support:%d\n", jack_support);
		return;
	}

	if (jack_support == JACK_DET_NONE) {
		SND_LOG_WARN("unsupport jack!\n");
		return;
	}

	return;
}
EXPORT_SYMBOL(snd_sunxi_jack_unregister);

/* jack mode selection interface probe */
void sunxi_jack_typec_mode_set(struct sunxi_jack_typec_cfg *jack_typec_cfg,
			       enum sunxi_jack_modes mode)
{
	int i;
	struct sunxi_jack_pins *jack_pins = jack_typec_cfg->jack_pins;
	struct sunxi_jack_modes_map *modes_map = jack_typec_cfg->modes_map;

	if (!modes_map || !jack_pins) {
		SND_LOG_ERR("modes map or jack pins is NULL\n");
		return;
	}

	if (mode >= SND_JACK_MODE_CNT || modes_map[mode].type == SND_JACK_MODE_NULL) {
		SND_LOG_WARN("missing mode value,mode:%d\n", mode);
		return;
	}

	for (i = 0; i < jack_typec_cfg->sw_pin_max; ++i) {
		if (!jack_pins[i].used || modes_map[mode].map_value[i] == 0xf)
			continue;
		gpio_set_value(jack_pins[i].pin, modes_map[mode].map_value[i]);
	}
}
EXPORT_SYMBOL(sunxi_jack_typec_mode_set);

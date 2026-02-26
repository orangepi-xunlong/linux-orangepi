// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-owa-raw"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <sound/soc.h>

#include "snd_sunxi_pcm.h"
#include "snd_sunxi_owa.h"
#include "snd_sunxi_owa_rx61937.h"

static const char *data_fmt[] = {"PCM", "RAW"};
static SOC_ENUM_SINGLE_EXT_DECL(rx_data_fmt_enum, data_fmt);

static int sunxi_owa_get_rx_data_fmt(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = owa->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_OWA_EXP_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> RX_MODE_MAN) & 0x1;

	return 0;
}

static int sunxi_owa_set_rx_data_fmt(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = owa->mem.regmap;
	unsigned int reg_val;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		reg_val = 0;
		break;
	case 1:
		reg_val = 1;
		break;
	default:
		return -EINVAL;
	}

	/* Owa Rx use the non-auto mode */
	regmap_update_bits(regmap, SUNXI_OWA_EXP_CTL, 1 << RX_MODE, 0 << RX_MODE);
	regmap_update_bits(regmap, SUNXI_OWA_EXP_CTL, 1 << RX_MODE_MAN, reg_val << RX_MODE_MAN);

	return 0;
}

static const struct snd_kcontrol_new sunxi_owa_rx_raw_controls[] = {
	SOC_ENUM_EXT("audio rx data format", rx_data_fmt_enum,
		     sunxi_owa_get_rx_data_fmt, sunxi_owa_set_rx_data_fmt),
};

int sunxi_add_rx_raw_controls(struct snd_soc_component *component)
{
	int ret;

	SND_LOG_DEBUG("\n");

	if (!component) {
		SND_LOG_ERR("component invalid\n");
		return -1;
	}

	ret = snd_soc_add_component_controls(component, sunxi_owa_rx_raw_controls,
					     ARRAY_SIZE(sunxi_owa_rx_raw_controls));
	if (ret) {
		SND_LOG_ERR("add rx_raw kcontrols failed\n");
		return -1;
	}

	return 0;
}

MODULE_AUTHOR("huhaoxin@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of owa_rx61937");

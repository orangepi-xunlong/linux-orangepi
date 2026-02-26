// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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

#define SUNXI_MODNAME		"sound-common"
#include "snd_sunxi_log.h"
#include <linux/of.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <sound/soc.h>

#include "snd_sunxi_common.h"
#include "snd_sunxi_adapter.h"
#include "snd_sunxi_common_utils.h"

#include "snd_sunxi_pa.h"

/******* pa config utils *******/
int snd_sunxi_pa_user_trig_init(struct snd_sunxi_pacfg *pa_cfg)
{
#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
	(void)pa_cfg;
	SND_LOG_INFO("pa user mode -> init\n");
#else
	(void)pa_cfg;
	SND_LOG_INFO("pa user mode -> init\n");
#endif

	return 0;	/* if success */
}

void snd_sunxi_pa_user_trig_exit(struct snd_sunxi_pacfg *pa_cfg)
{
#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
	(void)pa_cfg;
	SND_LOG_INFO("pa user mode -> exit\n");
#else
	(void)pa_cfg;
	SND_LOG_INFO("pa user mode -> exit\n");
#endif
}

int snd_sunxi_pa_user_trig_probe(struct snd_sunxi_pacfg *pa_cfg, u32 pa_pin_max,
				 struct snd_soc_component *component)
{
#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
	int ret;
	(void)pa_cfg;
	(void)pa_pin_max;

	ret = aw87xxx_add_codec_controls((void *)component);
	if (ret < 0) {
		SND_LOG_ERR("add_codec_controls failed, err %d\n", ret);
		return ret;
	};
#else
	(void)pa_cfg;
	(void)pa_pin_max;
	(void)component;
	SND_LOG_INFO("pa user mode -> probe\n");
#endif

	return 0;	/* if success */
}

int snd_sunxi_pa_user_trig_enable(struct snd_sunxi_pacfg *pa_cfg)
{
#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
	int ret;
	u32 index = pa_cfg->index;

	ret = aw87xxx_set_profile(index, "Music");
	if (ret < 0) {
		SND_LOG_ERR("set Music profile failed\n");
		return ret;
	}
#else
	(void)pa_cfg;
	SND_LOG_INFO("pa user mode -> enable\n");
#endif

	return 0;	/* if success */
}

void snd_sunxi_pa_user_trig_disable(struct snd_sunxi_pacfg *pa_cfg)
{
#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
	int ret;
	u32 index = pa_cfg->index;

	ret = aw87xxx_set_profile(index, "Off");
	if (ret < 0) {
		SND_LOG_ERR("pa set Off profile failed\n");
		return;
	}
#else
	(void)pa_cfg;
	SND_LOG_INFO("pa user mode -> disable\n");
#endif
}

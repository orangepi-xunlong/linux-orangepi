// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2021, <lijingpsw@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "snd_sunxi_adapter.h"
#include "snd_sunxi_log.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
void sunxi_adpt_set_dai_ops(struct snd_soc_dai_driver *dai_drv, struct snd_soc_dai_ops *ops,
			    struct sunxi_adapt_dai_ops_priv *priv)
{
	ops->probe = priv->probe;
	ops->remove = priv->remove;
	dai_drv->ops = ops;
}
void sunxi_adpt_rt_delay(struct snd_pcm_runtime *runtime, struct dma_tx_state state)
{
	runtime->delay = bytes_to_frames(runtime, (state.in_flight_bytes >> 1));
}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
void sunxi_adpt_set_dai_ops(struct snd_soc_dai_driver *dai_drv, struct snd_soc_dai_ops *ops,
			    struct sunxi_adapt_dai_ops_priv *priv)
{
	dai_drv->probe = priv->probe;
	dai_drv->remove = priv->remove;
	dai_drv->ops = ops;
}
void sunxi_adpt_rt_delay(struct snd_pcm_runtime *runtime, struct dma_tx_state state)
{
	runtime->delay = bytes_to_frames(runtime, (state.in_flight_bytes >> 1));
}
#else
void sunxi_adpt_set_dai_ops(struct snd_soc_dai_driver *dai_drv, struct snd_soc_dai_ops *ops,
			    struct sunxi_adapt_dai_ops_priv *priv)
{
	dai_drv->probe = priv->probe;
	dai_drv->remove = priv->remove;
	dai_drv->ops = ops;
}
void sunxi_adpt_rt_delay(struct snd_pcm_runtime *runtime, struct dma_tx_state state)
{
	return;
}
#endif
EXPORT_SYMBOL_GPL(sunxi_adpt_set_dai_ops);
EXPORT_SYMBOL_GPL(sunxi_adpt_rt_delay);

/*
 * ALSA defect(from linux-5.10 to linux-6.6)
 * unalter active count of components without DAIs will
 * cause hardware resource to be close by mistake.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE <= KERNEL_VERSION(6, 6, 0)
void sunxi_adpt_runtime_action(struct snd_soc_component *component, int action)
{
	component->active += action;
}
#else
void sunxi_adpt_runtime_action(struct snd_soc_component *component, int action)
{
	return;
}
#endif
EXPORT_SYMBOL_GPL(sunxi_adpt_runtime_action);

int snd_sunxi_card_jack_new(struct snd_soc_card *card, const char *id, int type,
			    struct snd_soc_jack *jack)
{
	int ret;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_PULSEAUDIO)
	struct snd_soc_jack_pin sunxi_jack_pin_switchs[] = {
		{
			.pin = "HP",
			.mask = SND_JACK_HEADPHONE,
		},
		{
			.pin = "HS MIC",
			.mask = SND_JACK_MICROPHONE,
		},
	};
#endif

	mutex_init(&jack->mutex);
	jack->card = card;
	INIT_LIST_HEAD(&jack->pins);
	INIT_LIST_HEAD(&jack->jack_zones);
	BLOCKING_INIT_NOTIFIER_HEAD(&jack->notifier);

	ret = snd_jack_new(card->snd_card, id, type, &jack->jack, false, false);
	switch (ret) {
	case -EPROBE_DEFER:
	case -ENOTSUPP:
	case 0:
		break;
	default:
		SND_LOGDEV_ERR(card->dev, "ASoC: error on %s: %d\n", card->name, ret);
	}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_PULSEAUDIO)
	ret = snd_soc_jack_add_pins(jack, ARRAY_SIZE(sunxi_jack_pin_switchs), sunxi_jack_pin_switchs);
	if (ret) {
		SND_LOG_ERR("snd_soc_jack_add_pins failed\n");
		return ret;
	}
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(snd_sunxi_card_jack_new);

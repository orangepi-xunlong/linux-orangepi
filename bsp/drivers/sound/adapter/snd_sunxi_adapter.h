/* SPDX-License-Identifier: GPL-2.0-or-later */
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

#include <linux/version.h>
#include <linux/of_gpio.h>
#include <linux/dmaengine.h>
#include <linux/device.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>

#ifndef __SND_SUNXI_ADAPTER_H
#define __SND_SUNXI_ADAPTER_H

struct sunxi_adapt_dai_ops_priv {
	int (*probe)(struct snd_soc_dai *dai);
	int (*remove)(struct snd_soc_dai *dai);
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
#define SUNXI_ATTR_SHOW_CONVERT(func_ptr)	\
	(ssize_t (*)(const struct class *class, const struct class_attribute *attr,	\
		     char *buf))(func_ptr)

#define SUNXI_ATTR_STORE_CONVERT(func_ptr)	\
	(ssize_t (*)(const struct class *class, const struct class_attribute *attr,	\
		     const char *buf, size_t count))(func_ptr)

#define sunxi_adpt_rtd_codec_dai(rtd, i, dai)	for_each_rtd_codec_dais(rtd, i, dai)
#define sunxi_adpt_rtd_cpu_dai(rtd)		asoc_rtd_to_cpu(rtd, 0)
#define sunxi_adpt_of_get_dai_name(of_node, dai_name) snd_soc_of_get_dai_name(of_node, dai_name, 0)
#define sunxi_adpt_class_create(owner, name)	class_create(name)
static inline void sunxi_adpt_vm_flags_set(struct vm_area_struct *vma, vm_flags_t flags)
{
	vm_flags_set(vma, flags);
}
static inline void *sunxi_adpt_dai_dma_data_get(const struct snd_soc_dai *dai, int stream)
{
	return dai->stream[stream].dma_data;
}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#define SUNXI_ATTR_SHOW_CONVERT(func_ptr)	func_ptr
#define SUNXI_ATTR_STORE_CONVERT(func_ptr)	func_ptr
#define sunxi_adpt_rtd_codec_dai(rtd, i, dai)	for_each_rtd_codec_dais(rtd, i, dai)
#define sunxi_adpt_rtd_cpu_dai(rtd)		asoc_rtd_to_cpu(rtd, 0)
#define sunxi_adpt_of_get_dai_name(of_node, dai_name)	snd_soc_of_get_dai_name(of_node, dai_name)
#define sunxi_adpt_class_create(owner, name)	class_create(owner, name)
static inline void sunxi_adpt_vm_flags_set(struct vm_area_struct *vma, vm_flags_t flags)
{
	vma->vm_flags |= flags;
}
static inline void *sunxi_adpt_dai_dma_data_get(const struct snd_soc_dai *dai, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return dai->playback_dma_data;
	else if (stream == SNDRV_PCM_STREAM_CAPTURE)
		return dai->capture_dma_data;
	return NULL;
}
#else
#define SUNXI_ATTR_SHOW_CONVERT(func_ptr)	func_ptr
#define SUNXI_ATTR_STORE_CONVERT(func_ptr)	func_ptr
#define sunxi_adpt_rtd_codec_dai(rtd, i, dai)	for_each_rtd_codec_dai(rtd, i, dai)
#define sunxi_adpt_rtd_cpu_dai(rtd)		rtd->cpu_dai
#define sunxi_adpt_of_get_dai_name(of_node, dai_name)	snd_soc_of_get_dai_name(of_node, dai_name)
#define sunxi_adpt_class_create(owner, name)	class_create(owner, name)
static inline void sunxi_adpt_vm_flags_set(struct vm_area_struct *vma, vm_flags_t flags)
{
	vma->vm_flags |= flags;
}
static inline void *sunxi_adpt_dai_dma_data_get(const struct snd_soc_dai *dai, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return dai->playback_dma_data;
	else if (stream == SNDRV_PCM_STREAM_CAPTURE)
		return dai->capture_dma_data;
	return NULL;
}
#endif

void sunxi_adpt_runtime_action(struct snd_soc_component *component, int action);
int sunxi_adpt_register_component(struct device *dev);
void sunxi_adpt_unregister_component(struct device *dev);

void sunxi_adpt_set_dai_ops(struct snd_soc_dai_driver *dai_drv, struct snd_soc_dai_ops *ops,
			    struct sunxi_adapt_dai_ops_priv *priv);
int snd_sunxi_card_jack_new(struct snd_soc_card *card, const char *id, int type,
			    struct snd_soc_jack *jack);
void sunxi_adpt_rt_delay(struct snd_pcm_runtime *runtime, struct dma_tx_state state);

#endif /* __SND_SUNXI_ADAPTER_H */

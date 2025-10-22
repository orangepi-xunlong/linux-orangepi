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

#include <linux/dma-mapping.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"

#define SUNXI_DMAENGINE_PCM_DRV_NAME	"sunxi_dmaengine_pcm"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int sunxi_pcm_adpt_probe(struct snd_soc_component *component)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_probe(component);
}

static void sunxi_pcm_adpt_remove(struct snd_soc_component *component)
{
	SND_LOG_DEBUG("\n");
	sunxi_pcm_remove(component);
}

static int sunxi_pcm_adpt_construct(struct snd_soc_component *component,
				    struct snd_soc_pcm_runtime *rtd)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_construct(component, rtd);
}

static void sunxi_pcm_adpt_destruct(struct snd_soc_component *component,
				    struct snd_pcm *pcm)
{
	int stream;

	SND_LOG_DEBUG("\n");

	for (stream = 0; stream < SNDRV_PCM_STREAM_LAST; stream++) {
		sunxi_pcm_destruct(component, pcm, stream);
	}
}

static int sunxi_pcm_adpt_open(struct snd_soc_component *component,
			       struct snd_pcm_substream *substream)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_open(component, substream);
}

static int sunxi_pcm_adpt_close(struct snd_soc_component *component,
				struct snd_pcm_substream *substream)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_close(component, substream);
}

static int sunxi_pcm_adpt_ioctl(struct snd_soc_component *component,
				struct snd_pcm_substream *substream,
				unsigned int cmd, void *arg)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_ioctl(component, substream, cmd, arg);
}

static int sunxi_pcm_adpt_hw_params(struct snd_soc_component *component,
				    struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_hw_params(component, substream, params);
}

static int sunxi_pcm_adpt_hw_free(struct snd_soc_component *component,
				  struct snd_pcm_substream *substream)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_hw_free(component, substream);
}

static int sunxi_pcm_adpt_prepare(struct snd_soc_component *component,
				      struct snd_pcm_substream *substream)
{
	return sunxi_pcm_prepare(component, substream);
}

static int sunxi_pcm_adpt_trigger(struct snd_soc_component *component,
				  struct snd_pcm_substream *substream,
				  int cmd)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_trigger(component, substream, cmd);
}

static snd_pcm_uframes_t sunxi_pcm_adpt_pointer(struct snd_soc_component *component,
						struct snd_pcm_substream *substream)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_pointer(component, substream);
}

static int sunxi_pcm_adpt_mmap(struct snd_soc_component *component,
			       struct snd_pcm_substream *substream,
			       struct vm_area_struct *vma)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_mmap(component, substream, vma);
}

static int sunxi_pcm_adpt_copy(struct snd_soc_component *component,
			       struct snd_pcm_substream *substream,
			       int channel, unsigned long hwoff,
			       struct iov_iter *iter, unsigned long bytes)
{
	void __user *buf = iter->ubuf;
	return sunxi_pcm_copy(component, substream, channel, hwoff, buf, bytes);
}

struct snd_soc_component_driver sunxi_soc_platform = {
	.name		= SUNXI_DMAENGINE_PCM_DRV_NAME,
	.probe		= sunxi_pcm_adpt_probe,
	.remove		= sunxi_pcm_adpt_remove,
	.pcm_construct	= sunxi_pcm_adpt_construct,
	.pcm_destruct	= sunxi_pcm_adpt_destruct,
	.open		= sunxi_pcm_adpt_open,
	.close		= sunxi_pcm_adpt_close,
	.ioctl		= sunxi_pcm_adpt_ioctl,
	.hw_params	= sunxi_pcm_adpt_hw_params,
	.hw_free	= sunxi_pcm_adpt_hw_free,
	.prepare	= sunxi_pcm_adpt_prepare,
	.trigger	= sunxi_pcm_adpt_trigger,
	.pointer	= sunxi_pcm_adpt_pointer,
	.mmap		= sunxi_pcm_adpt_mmap,
	.copy		= sunxi_pcm_adpt_copy,
};

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static int sunxi_pcm_adpt_probe(struct snd_soc_component *component)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_probe(component);
}

static void sunxi_pcm_adpt_remove(struct snd_soc_component *component)
{
	SND_LOG_DEBUG("\n");
	sunxi_pcm_remove(component);
}

static int sunxi_pcm_adpt_construct(struct snd_soc_component *component,
				    struct snd_soc_pcm_runtime *rtd)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_construct(component, rtd);
}

static void sunxi_pcm_adpt_destruct(struct snd_soc_component *component,
				    struct snd_pcm *pcm)
{
	int stream;

	SND_LOG_DEBUG("\n");

	for (stream = 0; stream < SNDRV_PCM_STREAM_LAST; stream++) {
		sunxi_pcm_destruct(component, pcm, stream);
	}
}

static int sunxi_pcm_adpt_open(struct snd_soc_component *component,
			       struct snd_pcm_substream *substream)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_open(component, substream);
}

static int sunxi_pcm_adpt_close(struct snd_soc_component *component,
				struct snd_pcm_substream *substream)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_close(component, substream);
}

static int sunxi_pcm_adpt_ioctl(struct snd_soc_component *component,
				struct snd_pcm_substream *substream,
				unsigned int cmd, void *arg)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_ioctl(component, substream, cmd, arg);
}

static int sunxi_pcm_adpt_hw_params(struct snd_soc_component *component,
				    struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_hw_params(component, substream, params);
}

static int sunxi_pcm_adpt_hw_free(struct snd_soc_component *component,
				  struct snd_pcm_substream *substream)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_hw_free(component, substream);
}

static int sunxi_pcm_adpt_prepare(struct snd_soc_component *component,
				      struct snd_pcm_substream *substream)
{
	return sunxi_pcm_prepare(component, substream);
}

static int sunxi_pcm_adpt_trigger(struct snd_soc_component *component,
				  struct snd_pcm_substream *substream,
				  int cmd)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_trigger(component, substream, cmd);
}

static snd_pcm_uframes_t sunxi_pcm_adpt_pointer(struct snd_soc_component *component,
						struct snd_pcm_substream *substream)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_pointer(component, substream);
}

static int sunxi_pcm_adpt_mmap(struct snd_soc_component *component,
			       struct snd_pcm_substream *substream,
			       struct vm_area_struct *vma)
{
	SND_LOG_DEBUG("\n");
	return sunxi_pcm_mmap(component, substream, vma);
}

static int sunxi_pcm_adpt_copy(struct snd_soc_component *component,
			       struct snd_pcm_substream *substream,
			       int channel, unsigned long hwoff,
			       void __user *buf, unsigned long bytes)
{
	return sunxi_pcm_copy(component, substream, channel, hwoff, buf, bytes);
}

struct snd_soc_component_driver sunxi_soc_platform = {
	.name		= SUNXI_DMAENGINE_PCM_DRV_NAME,
	.probe		= sunxi_pcm_adpt_probe,
	.remove		= sunxi_pcm_adpt_remove,
	.pcm_construct	= sunxi_pcm_adpt_construct,
	.pcm_destruct	= sunxi_pcm_adpt_destruct,
	.open		= sunxi_pcm_adpt_open,
	.close		= sunxi_pcm_adpt_close,
	.ioctl		= sunxi_pcm_adpt_ioctl,
	.hw_params	= sunxi_pcm_adpt_hw_params,
	.hw_free	= sunxi_pcm_adpt_hw_free,
	.prepare	= sunxi_pcm_adpt_prepare,
	.trigger	= sunxi_pcm_adpt_trigger,
	.pointer	= sunxi_pcm_adpt_pointer,
	.copy_user	= sunxi_pcm_adpt_copy,
	.mmap		= sunxi_pcm_adpt_mmap,
};
#else
static int sunxi_pcm_adpt_probe(struct snd_soc_component *component)
{
	SND_LOG_DEBUG("\n");

	return sunxi_pcm_probe(component);
}

static void sunxi_pcm_adpt_remove(struct snd_soc_component *component)
{
	SND_LOG_DEBUG("\n");

	sunxi_pcm_remove(component);
}

static int sunxi_pcm_adpt_construct(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = NULL;

	SND_LOG_DEBUG("\n");

	component = snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);

	return sunxi_pcm_construct(component, rtd);
}

static void sunxi_pcm_adpt_destruct(struct snd_pcm *pcm)
{
	struct snd_soc_component *component = NULL;
	struct snd_pcm_substream *substream = NULL;
	struct snd_soc_pcm_runtime *rtd = NULL;
	int stream;

	SND_LOG_DEBUG("\n");

	for (stream = 0; stream < SNDRV_PCM_STREAM_LAST; ++stream) {
		substream = pcm->streams[stream].substream;
		rtd = substream->private_data;
		component = snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);

		sunxi_pcm_destruct(component, pcm, stream);
	}
}

static int sunxi_pcm_adpt_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	SND_LOG_DEBUG("\n");

	component = snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_open(component, substream);

}

static int sunxi_pcm_adpt_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	SND_LOG_DEBUG("\n");

	component =  snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_close(component, substream);
}

static int sunxi_pcm_adpt_ioctl(struct snd_pcm_substream *substream, unsigned int cmd, void *arg)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	SND_LOG_DEBUG("\n");

	component =  snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_ioctl(component, substream, cmd, arg);
}

static int sunxi_pcm_adpt_hw_params(struct snd_pcm_substream *substream,
				       struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	SND_LOG_DEBUG("\n");

	component =  snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_hw_params(component, substream, params);
}

static int sunxi_pcm_adpt_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	SND_LOG_DEBUG("\n");

	component =  snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_hw_free(component, substream);
}

static int sunxi_pcm_adpt_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	component =  snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_prepare(component, substream);
}

static int sunxi_pcm_adpt_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	SND_LOG_DEBUG("\n");

	component =  snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_trigger(component, substream, cmd);

}

static snd_pcm_uframes_t sunxi_pcm_adpt_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	SND_LOG_DEBUG("\n");

	component =  snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_pointer(component, substream);
}

static int sunxi_pcm_adpt_mmap(struct snd_pcm_substream *substream,
				  struct vm_area_struct *vma)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	SND_LOG_DEBUG("\n");

	component =  snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_mmap(component, substream, vma);
}

static int sunxi_pcm_adpt_copy(struct snd_pcm_substream *substream, int channel,
			       unsigned long hwoff, void __user *buf,
			       unsigned long bytes)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = NULL;

	component =  snd_soc_rtdcom_lookup(rtd, SUNXI_DMAENGINE_PCM_DRV_NAME);
	return sunxi_pcm_copy(component, substream, channel, hwoff, buf, bytes);
}

static struct snd_pcm_ops sunxi_pcm_ops = {
	.open		= sunxi_pcm_adpt_open,
	.close		= sunxi_pcm_adpt_close,
	.ioctl		= sunxi_pcm_adpt_ioctl,
	.hw_params	= sunxi_pcm_adpt_hw_params,
	.hw_free	= sunxi_pcm_adpt_hw_free,
	.prepare	= sunxi_pcm_adpt_prepare,
	.trigger	= sunxi_pcm_adpt_trigger,
	.pointer	= sunxi_pcm_adpt_pointer,
	.copy_user	= sunxi_pcm_adpt_copy,
	.mmap		= sunxi_pcm_adpt_mmap,
};

struct snd_soc_component_driver sunxi_soc_platform = {
	.name		= SUNXI_DMAENGINE_PCM_DRV_NAME,
	.ops		= &sunxi_pcm_ops,
	.probe		= sunxi_pcm_adpt_probe,
	.remove		= sunxi_pcm_adpt_remove,
	.pcm_new	= sunxi_pcm_adpt_construct,
	.pcm_free	= sunxi_pcm_adpt_destruct,
};
#endif

int sunxi_adpt_register_component(struct device *dev)
{
	SND_LOG_DEBUG("\n");
	return snd_soc_register_component(dev, &sunxi_soc_platform, NULL, 0);
}

void sunxi_adpt_unregister_component(struct device *dev)
{
	SND_LOG_DEBUG("\n");
	snd_soc_unregister_component(dev);
}

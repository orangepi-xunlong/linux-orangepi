/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2021, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <sound/soc.h>
#include <sound/pcm.h>

#include "snd_sunxi_common.h"

#ifndef __SND_SUNXI_PCM_H
#define __SND_SUNXI_PCM_H

#define SUNXI_AUDIO_CMA_BLOCK_BYTES	1024
#define SUNXI_AUDIO_CMA_MAX_KBYTES	1024
#define SUNXI_AUDIO_CMA_MIN_KBYTES	64
#define SUNXI_AUDIO_CMA_MAX_BYTES	(SUNXI_AUDIO_CMA_BLOCK_BYTES * SUNXI_AUDIO_CMA_MAX_KBYTES)
#define SUNXI_AUDIO_CMA_MIN_BYTES	(SUNXI_AUDIO_CMA_BLOCK_BYTES * SUNXI_AUDIO_CMA_MIN_KBYTES)

#define SUNXI_AUDIO_FIFO_SIZE		128

struct sunxi_dma_params {
	char *name;
	dma_addr_t dma_addr;
	u8 src_maxburst;
	u8 dst_maxburst;
	u8 dma_drq_type_num;

	/* max buffer set (value must be (2^n)Kbyte) */
	size_t cma_kbytes;
	size_t fifo_size;

	/* dma_buf_mode = 0: dynamically allocate DMA buffer;
	 * dma_buf_mode = 1: allocate DMA buffer when start up.
	 */
	unsigned int dma_buf_mode;

	/* for hdmi audio */
	enum HDMI_FORMAT hdmi_fmt;

	/* runtime->buffer_size should *2 when pcm data is raw data */
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t period_size;

	/* when buffer_size and period_size *2 is true */
	bool change_size_flag;

	/* DMA area */
	unsigned char *raw_dma_area;
	dma_addr_t raw_dma_addr;
	dma_addr_t pcm_dma_addr;
};

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_PCM)
extern int snd_sunxi_dma_platform_register(struct device *dev);
extern void snd_sunxi_dma_platform_unregister(struct device *dev);

int sunxi_pcm_probe(struct snd_soc_component *component);
void sunxi_pcm_remove(struct snd_soc_component *component);
int sunxi_pcm_construct(struct snd_soc_component *component,
			       struct snd_soc_pcm_runtime *rtd);
void sunxi_pcm_destruct(struct snd_soc_component *component,
			       struct snd_pcm *pcm,
			       int stream);
int sunxi_pcm_open(struct snd_soc_component *component,
			  struct snd_pcm_substream *substream);
int sunxi_pcm_close(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream);
int sunxi_pcm_ioctl(struct snd_soc_component *component,
			   struct snd_pcm_substream *substream,
			   unsigned int cmd, void *arg);
int sunxi_pcm_hw_params(struct snd_soc_component *component,
			       struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params);
int sunxi_pcm_hw_free(struct snd_soc_component *component,
			     struct snd_pcm_substream *substream);
int sunxi_pcm_trigger(struct snd_soc_component *component,
			     struct snd_pcm_substream *substream,
			     int cmd);
snd_pcm_uframes_t sunxi_pcm_pointer(struct snd_soc_component *component,
				    struct snd_pcm_substream *substream);
int sunxi_pcm_mmap(struct snd_soc_component *component,
		   struct snd_pcm_substream *substream,
		   struct vm_area_struct *vma);


int sunxi_pcm_copy(struct snd_soc_component *component,
		   struct snd_pcm_substream *substream, int channel,
		   unsigned long hwoff, void __user *buf, unsigned long bytes);
int sunxi_pcm_prepare(struct snd_soc_component *component,
		      struct snd_pcm_substream *substream);

#else
static inline int snd_sunxi_dma_platform_register(struct device *dev)
{
	pr_err("[sound %4d][PCM %s] PCM API is disabled\n", __LINE__, __func__);
	return 0;
}

static inline void snd_sunxi_dma_platform_unregister(struct device *dev)
{
	pr_err("[sound %4d][PCM %s] PCM API is disabled\n", __LINE__, __func__);
}

static inline int sunxi_pcm_probe(struct snd_soc_component *component)
{
	return 0;
}
static inline void sunxi_pcm_remove(struct snd_soc_component *component) { }
static inline int sunxi_pcm_construct(struct snd_soc_component *component,
				      struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}
static inline void sunxi_pcm_destruct(struct snd_soc_component *component,
				      struct snd_pcm *pcm, int stream) { }
static inline int sunxi_pcm_open(struct snd_soc_component *component,
				 struct snd_pcm_substream *substream)
{
	return 0;
}
static inline int sunxi_pcm_close(struct snd_soc_component *component,
				  struct snd_pcm_substream *substream)
{
	return 0;
}
static inline int sunxi_pcm_ioctl(struct snd_soc_component *component,
				  struct snd_pcm_substream *substream,
				  unsigned int cmd, void *arg)
{
	return 0;
}
static inline int sunxi_pcm_hw_params(struct snd_soc_component *component,
				      struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params)
{
	return 0;
}
static inline int sunxi_pcm_hw_free(struct snd_soc_component *component,
				    struct snd_pcm_substream *substream)
{
	return 0;
}
static inline int sunxi_pcm_trigger(struct snd_soc_component *component,
				    struct snd_pcm_substream *substream,
				    int cmd)
{
	return 0;
}
static inline snd_pcm_uframes_t sunxi_pcm_pointer(struct snd_soc_component *component,
						  struct snd_pcm_substream *substream)
{
	return 0;
}
static inline int sunxi_pcm_mmap(struct snd_soc_component *component,
				 struct snd_pcm_substream *substream,
				 struct vm_area_struct *vma)
{
	return 0;
}

static inline int sunxi_pcm_copy_user(struct snd_soc_component *component,
				      struct snd_pcm_substream *substream, int channel,
				      unsigned long hwoff, void __user *buf, unsigned long bytes)
{
	return 0;
}

static inline int sunxi_pcm_prepare(struct snd_soc_component *component,
				    struct snd_pcm_substream *substream)
{
	return 0;
}
#endif

#endif /* __SND_SUNXI_PCM_H */

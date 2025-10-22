/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sound\soc\sunxi\snd_sunxi_loopback.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

/* only applies to linux-5.4 */
#include <linux/module.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/dmaengine_pcm.h>

#include "../platform/snd_sunxi_log.h"
#include "snd_sunxi_pcm_loopback.h"

#define HLOG		"CPUDAI"
#define DRV_NAME	"sunxi-snd-plat-loopback"

#define BUFFER_SIZE (4 * 1024)	/* bytes */

DEFINE_SPINLOCK(buffer_lock);

struct sunxi_hrtime {
	struct hrtimer timer;
	ktime_t kt;

	u16 *dmabuf_addr_16b;
	u32 *dmabuf_addr_32b;

	struct snd_pcm_substream *substream;
	struct work_struct hrtime_work;
};

struct sunxi_loopback {
	const char *module_version;
	struct platform_device *pdev;

	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;

	struct sunxi_hrtime play_hrt;
	struct sunxi_hrtime cap_hrt;

	unsigned int w_ptr;
	unsigned int r_ptr;
	unsigned int buffer_sz;
	void *buffer;
};

struct sunxi_loopback *g_loopback_ptr;
bool play_flag;
bool cap_flag;

/* from pcm_dmaengine.c*/
struct dmaengine_pcm_runtime_data {
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;

	unsigned int pos;
};

static inline struct dmaengine_pcm_runtime_data *substream_to_prtd(
	const struct snd_pcm_substream *substream)
{
	return substream->runtime->private_data;
}
/* end */

static void snd_sunxi_loopback_play_hrtime_work(struct work_struct *work)
{
	struct snd_pcm_substream *substream = g_loopback_ptr->play_hrt.substream;
	struct dmaengine_pcm_runtime_data *prtd;
	unsigned long flags;

	spin_lock_irqsave(&buffer_lock, flags);
	if (play_flag == 0 || !substream || !substream->runtime) {
		SND_LOG_DEBUG("flag invalid\n");
		spin_unlock_irqrestore(&buffer_lock, flags);
		return;
	}

	prtd = substream_to_prtd(substream);
	spin_unlock_irqrestore(&buffer_lock, flags);

	prtd->pos += snd_pcm_lib_period_bytes(substream);
	if (prtd->pos >= snd_pcm_lib_buffer_bytes(substream))
		prtd->pos = 0;

	snd_pcm_period_elapsed(substream);
}

static void snd_sunxi_loopback_cap_hrtime_work(struct work_struct *work)
{
	struct snd_pcm_substream *substream = g_loopback_ptr->cap_hrt.substream;
	struct dmaengine_pcm_runtime_data *prtd;
	unsigned long flags;

	spin_lock_irqsave(&buffer_lock, flags);
	if (cap_flag == 0 || !substream || !substream->runtime) {
		SND_LOG_DEBUG("flag invalid\n");
		spin_unlock_irqrestore(&buffer_lock, flags);
		return;
	}

	prtd = substream_to_prtd(substream);
	spin_unlock_irqrestore(&buffer_lock, flags);

	prtd->pos += snd_pcm_lib_period_bytes(substream);
	if (prtd->pos >= snd_pcm_lib_buffer_bytes(substream))
		prtd->pos = 0;

	snd_pcm_period_elapsed(substream);
}

/* loopback func */
static enum hrtimer_restart play_hrtimer_handler(struct hrtimer *timer)
{
	u16 val_16;
	u32 val_32;
	u16 *addr_16;
	u32 *addr_32;
	unsigned int bits, channels, period_sz, periods;
	unsigned int i, j;
	static unsigned int k;
	unsigned long flags;

	spin_lock_irqsave(&buffer_lock, flags);

	if (play_flag == 0 || g_loopback_ptr->buffer == NULL) {
		SND_LOG_DEBUG("loopback-play stop\n");
		spin_unlock_irqrestore(&buffer_lock, flags);
		return HRTIMER_NORESTART;
	}

	if (!g_loopback_ptr->play_hrt.substream) {
		SND_LOG_DEBUG("loopback-play stop\n");
		SND_LOG_DEBUG("substream invalid\n");
		spin_unlock_irqrestore(&buffer_lock, flags);
		return HRTIMER_NORESTART;
	}

	if (!g_loopback_ptr->play_hrt.substream->runtime) {
		SND_LOG_DEBUG("loopback-play stop\n");
		SND_LOG_DEBUG("runtime invalid\n");
		spin_unlock_irqrestore(&buffer_lock, flags);
		return HRTIMER_NORESTART;
	}

	bits = g_loopback_ptr->play_hrt.substream->runtime->sample_bits;
	channels = g_loopback_ptr->play_hrt.substream->runtime->channels;
	period_sz = g_loopback_ptr->play_hrt.substream->runtime->period_size;
	periods = g_loopback_ptr->play_hrt.substream->runtime->periods;

	if (bits == 16) {
		addr_16 = g_loopback_ptr->play_hrt.dmabuf_addr_16b;
		for (i = 0; i < period_sz; i++) {
			for (j = 0; j < channels; j++) {
				val_16 = *(addr_16 + j + i * channels + k * period_sz * channels);
				*((u16 *)g_loopback_ptr->buffer + g_loopback_ptr->w_ptr) = val_16;
				g_loopback_ptr->w_ptr += 1;

				if (g_loopback_ptr->w_ptr == g_loopback_ptr->buffer_sz)
					g_loopback_ptr->w_ptr = 0;
			}
		}
	} else if (bits == 32) {
		addr_32 = g_loopback_ptr->play_hrt.dmabuf_addr_32b;
		for (i = 0; i < period_sz; i++) {
			for (j = 0; j < channels; j++) {
				val_32 = *(addr_32 + j + i * channels + k * period_sz * channels);
				*((u32 *)g_loopback_ptr->buffer + g_loopback_ptr->w_ptr) = val_32;
				g_loopback_ptr->w_ptr += 1;

				if (g_loopback_ptr->w_ptr == g_loopback_ptr->buffer_sz)
					g_loopback_ptr->w_ptr = 0;
			}
		}
	}

	spin_unlock_irqrestore(&buffer_lock, flags);
	schedule_work(&g_loopback_ptr->play_hrt.hrtime_work);

	if (++k >= periods)
		k = 0;

	g_loopback_ptr->play_hrt.substream->dma_pos = k * period_sz;

	/* timer overflow */
	hrtimer_forward_now(&g_loopback_ptr->play_hrt.timer, g_loopback_ptr->play_hrt.kt);

	return HRTIMER_RESTART;
}

static enum hrtimer_restart cap_hrtimer_handler(struct hrtimer *timer)
{
	u16 val_16 = 0;
	u32 val_32;
	u16 *addr_16;
	u32 *addr_32;
	unsigned int bits, channels, period_sz, periods;
	unsigned int i, j;
	static unsigned int k;
	unsigned long flags;

	spin_lock_irqsave(&buffer_lock, flags);

	if (cap_flag == 0 || g_loopback_ptr->buffer == NULL) {
		SND_LOG_DEBUG("loopback-cap stop");
		spin_unlock_irqrestore(&buffer_lock, flags);
		return HRTIMER_NORESTART;
	}

	if (!g_loopback_ptr->cap_hrt.substream) {
		SND_LOG_DEBUG("loopback-cap stop\n");
		SND_LOG_DEBUG("substream invalid\n");
		spin_unlock_irqrestore(&buffer_lock, flags);
		return HRTIMER_NORESTART;
	}

	if (!g_loopback_ptr->cap_hrt.substream->runtime) {
		SND_LOG_DEBUG("loopback-cap stop\n");
		SND_LOG_DEBUG("runtime invalid\n");
		spin_unlock_irqrestore(&buffer_lock, flags);
		return HRTIMER_NORESTART;
	}

	bits = g_loopback_ptr->cap_hrt.substream->runtime->sample_bits;
	channels = g_loopback_ptr->cap_hrt.substream->runtime->channels;
	period_sz = g_loopback_ptr->cap_hrt.substream->runtime->period_size;
	periods = g_loopback_ptr->cap_hrt.substream->runtime->periods;

	if (bits == 16) {
		addr_16 = g_loopback_ptr->cap_hrt.dmabuf_addr_16b;
		for (i = 0; i < period_sz; i++) {
			for (j = 0; j < channels; j++) {
				val_16 = *((u16 *)g_loopback_ptr->buffer + g_loopback_ptr->r_ptr);
				*(addr_16 + j + i * channels + k * period_sz * channels) = val_16;
				g_loopback_ptr->r_ptr += 1;

				if (g_loopback_ptr->r_ptr == g_loopback_ptr->buffer_sz)
					g_loopback_ptr->r_ptr = 0;
			}
		}
	} else if (bits == 32) {
		addr_32 = g_loopback_ptr->cap_hrt.dmabuf_addr_32b;
		for (i = 0; i < period_sz; i++) {
			for (j = 0; j < channels; j++) {
				val_32 = *((u32 *)g_loopback_ptr->buffer + g_loopback_ptr->r_ptr);
				*(addr_32 + j + i * channels + k * period_sz * channels) = val_32;
				g_loopback_ptr->r_ptr += 1;

				if (g_loopback_ptr->r_ptr == g_loopback_ptr->buffer_sz)
					g_loopback_ptr->r_ptr = 0;
			}
		}
	}

	spin_unlock_irqrestore(&buffer_lock, flags);
	schedule_work(&g_loopback_ptr->cap_hrt.hrtime_work);

	if (++k >= periods)
		k = 0;

	g_loopback_ptr->cap_hrt.substream->dma_pos = k * period_sz;

	/* timer overflow */
	hrtimer_forward_now(&g_loopback_ptr->cap_hrt.timer, g_loopback_ptr->cap_hrt.kt);

	return HRTIMER_RESTART;
}

static int sunxi_loopback_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_loopback *sunxi_loopback = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("stream -> %d\n", substream->stream);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream, &sunxi_loopback->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream, &sunxi_loopback->capture_dma_param);

	return 0;
}

static int sunxi_loopback_dai_trigger(struct snd_pcm_substream *substream,
				      int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_loopback *sunxi_loopback = snd_soc_dai_get_drvdata(dai);
	unsigned long flags;
	unsigned long time;

	SND_LOG_DEBUG("stream:%d, cmd -> %d\n", substream->stream, cmd);

	spin_lock_irqsave(&buffer_lock, flags);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/* create buffer for loopback */
		if (sunxi_loopback->buffer == NULL) {
			sunxi_loopback->buffer = vmalloc(BUFFER_SIZE);
			if (!sunxi_loopback->buffer) {
				SND_LOG_ERR("vmalloc failed\n");
				spin_unlock_irqrestore(&buffer_lock, flags);
				return -1;
			}
			sunxi_loopback->buffer_sz = BUFFER_SIZE / (substream->runtime->sample_bits >> 3);
		}
		memset(sunxi_loopback->buffer, 0, BUFFER_SIZE);

		time = substream->runtime->period_size * 1000000 / substream->runtime->rate;
		time *= 1000;
		SND_LOG_DEBUG("hrtime:%luns\n", time);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			play_flag = 1;
			sunxi_loopback->play_hrt.substream = substream;
			/* get dma_buffer address */
			if (substream->runtime->sample_bits == 16) {
				sunxi_loopback->play_hrt.dmabuf_addr_16b = (u16 *)(substream->runtime->dma_area);
			} else if (substream->runtime->sample_bits == 32) {
				sunxi_loopback->play_hrt.dmabuf_addr_32b = (u32 *)(substream->runtime->dma_area);
			} else {
				SND_LOG_ERR("This sampling rate is not supported yet\n");
				spin_unlock_irqrestore(&buffer_lock, flags);
				return -EINVAL;
			}
			/* set interrupt trigger time and start */
			sunxi_loopback->play_hrt.kt = ktime_set(0, time);
			hrtimer_start(&sunxi_loopback->play_hrt.timer, sunxi_loopback->play_hrt.kt, HRTIMER_MODE_REL);
		} else {
			cap_flag = 1;
			sunxi_loopback->cap_hrt.substream = substream;
			if (substream->runtime->sample_bits == 16) {
				sunxi_loopback->cap_hrt.dmabuf_addr_16b = (u16 *)(substream->runtime->dma_area);
			} else if (substream->runtime->sample_bits == 32) {
				sunxi_loopback->cap_hrt.dmabuf_addr_32b = (u32 *)(substream->runtime->dma_area);
			} else {
				SND_LOG_ERR("This sampling rate is not supported yet\n");
				spin_unlock_irqrestore(&buffer_lock, flags);
				return -EINVAL;
			}
			sunxi_loopback->cap_hrt.kt = ktime_set(0, time);
			hrtimer_start(&sunxi_loopback->cap_hrt.timer, sunxi_loopback->cap_hrt.kt, HRTIMER_MODE_REL);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			play_flag = 0;
		} else {
			cap_flag = 0;
		}
		/* free buffer for loopback */
		if (sunxi_loopback->buffer && !play_flag && !cap_flag) {
			vfree(sunxi_loopback->buffer);
			sunxi_loopback->buffer = NULL;
		}
		break;
	default:
		SND_LOG_ERR("unsupport cmd\n");
		spin_unlock_irqrestore(&buffer_lock, flags);
		return -EINVAL;
	}

	SND_LOG_DEBUG("play_flag:%d\n", play_flag);
	SND_LOG_DEBUG("cap_flag:%d\n", cap_flag);
	SND_LOG_DEBUG("buffer_addr:%p\n", sunxi_loopback->buffer);
	SND_LOG_DEBUG("w_ptr:%u\n", sunxi_loopback->w_ptr);

	spin_unlock_irqrestore(&buffer_lock, flags);

	return 0;
}

static const struct snd_soc_dai_ops sunxi_loopback_dai_ops = {
	.startup	= sunxi_loopback_dai_startup,
	.trigger	= sunxi_loopback_dai_trigger,
};

static int sunxi_loopback_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_loopback *sunxi_loopback = snd_soc_dai_get_drvdata(dai);

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai,
				  &sunxi_loopback->playback_dma_param,
				  &sunxi_loopback->capture_dma_param);

	return 0;
}

static struct snd_soc_dai_driver sunxi_loopback_dai = {
	.name = DRV_NAME,
	.probe = sunxi_loopback_dai_probe,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_loopback_dai_ops,
};

static int sunxi_loopback_component_probe(struct snd_soc_component *component)
{
	struct sunxi_loopback *sunxi_loopback = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	/* init hrtime */
	hrtimer_init(&sunxi_loopback->play_hrt.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	sunxi_loopback->play_hrt.timer.function = play_hrtimer_handler;

	hrtimer_init(&sunxi_loopback->cap_hrt.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	sunxi_loopback->cap_hrt.timer.function = cap_hrtimer_handler;

	INIT_WORK(&sunxi_loopback->play_hrt.hrtime_work, snd_sunxi_loopback_play_hrtime_work);
	INIT_WORK(&sunxi_loopback->cap_hrt.hrtime_work, snd_sunxi_loopback_cap_hrtime_work);

	return 0;
}

static void sunxi_loopback_component_remove(struct snd_soc_component *component)
{
	struct sunxi_loopback *sunxi_loopback = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	hrtimer_cancel(&sunxi_loopback->play_hrt.timer);
	hrtimer_cancel(&sunxi_loopback->cap_hrt.timer);

	cancel_work_sync(&sunxi_loopback->play_hrt.hrtime_work);
	cancel_work_sync(&sunxi_loopback->cap_hrt.hrtime_work);
}

static struct snd_soc_component_driver sunxi_loopback_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_loopback_component_probe,
	.remove		= sunxi_loopback_component_remove,
};

static int sunxi_loopback_parse_dma_param(struct device_node *np, struct sunxi_loopback *sunxi_loopback)
{
	int ret;
	unsigned int temp_val;

	/* set dma max buffer */
	ret = of_property_read_u32(np, "playback-cma", &temp_val);
	if (ret < 0) {
		sunxi_loopback->playback_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("playback-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		sunxi_loopback->playback_dma_param.cma_kbytes = temp_val;
	}

	ret = of_property_read_u32(np, "capture-cma", &temp_val);
	if (ret != 0) {
		sunxi_loopback->capture_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("capture-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		sunxi_loopback->capture_dma_param.cma_kbytes = temp_val;
	}

	/* set fifo size */
	ret = of_property_read_u32(np, "tx-fifo-size", &temp_val);
	if (ret != 0) {
		sunxi_loopback->playback_dma_param.fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("tx-fifo-size miss, using default value\n");
	} else {
		sunxi_loopback->playback_dma_param.fifo_size = temp_val;
	}

	ret = of_property_read_u32(np, "rx-fifo-size", &temp_val);
	if (ret != 0) {
		sunxi_loopback->capture_dma_param.fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("rx-fifo-size miss,using default value\n");
	} else {
		sunxi_loopback->capture_dma_param.fifo_size = temp_val;
	}

	SND_LOG_DEBUG("playback-cma : %zu\n", sunxi_loopback->playback_dma_param.cma_kbytes);
	SND_LOG_DEBUG("capture-cma  : %zu\n", sunxi_loopback->capture_dma_param.cma_kbytes);
	SND_LOG_DEBUG("tx-fifo-size : %zu\n", sunxi_loopback->playback_dma_param.fifo_size);
	SND_LOG_DEBUG("rx-fifo-size : %zu\n", sunxi_loopback->capture_dma_param.fifo_size);

	return 0;
}

static int sunxi_loopback_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_loopback *sunxi_loopback;

	sunxi_loopback = devm_kzalloc(&pdev->dev, sizeof(*sunxi_loopback), GFP_KERNEL);
	SND_LOG_ERR("\n");
	if (!sunxi_loopback) {
		ret = -ENOMEM;
		SND_LOG_ERR("devm_kzalloc failed\n");
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(&pdev->dev, sunxi_loopback);
	SND_LOG_ERR("%s %d\n", __func__, __LINE__);
	g_loopback_ptr = sunxi_loopback;
	sunxi_loopback->pdev = pdev;

	ret = sunxi_loopback_parse_dma_param(np, sunxi_loopback);
	SND_LOG_ERR("%s %d\n", __func__, __LINE__);
	if (ret < 0) {
		SND_LOG_ERR("parse dma param failed\n");
		goto err_sunxi_loopback_parse_dma_param;
	}

	ret = snd_soc_register_component(&pdev->dev, &sunxi_loopback_dev, &sunxi_loopback_dai, 1);
	SND_LOG_ERR("%s %d\n", __func__, __LINE__);
	if (ret) {
		SND_LOG_ERR("component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_sunxi_dma_platform_loopback_register(&pdev->dev);
	SND_LOG_ERR("%s %d\n", __func__, __LINE__);
	if (ret) {
		SND_LOG_ERR("register loopback ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_dma_platform_loopback_register;
	}

	SND_LOG_ERR("%s %d\n", __func__, __LINE__);
	SND_LOG_DEBUG("register loopback platform success\n");

	return 0;

err_snd_sunxi_dma_platform_loopback_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
err_sunxi_loopback_parse_dma_param:
	devm_kfree(&pdev->dev, sunxi_loopback);
err_devm_kzalloc:
	of_node_put(np);
	return ret;
}

static int sunxi_loopback_dev_remove(struct platform_device *pdev)
{
	snd_sunxi_dma_platform_loopback_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	SND_LOG_DEBUG("unregister loopback platform success\n");

	return 0;
}

static const struct of_device_id sunxi_loopback_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_loopback_of_match);

static struct platform_driver sunxi_loopback_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_loopback_of_match,
	},
	.probe	= sunxi_loopback_dev_probe,
	.remove	= sunxi_loopback_dev_remove,
};

int __init sunxi_loopback_dev_init(void)
{
	int ret;

	SND_LOG_ERR("\n");
	ret = platform_driver_register(&sunxi_loopback_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_loopback_dev_exit(void)
{
	platform_driver_unregister(&sunxi_loopback_driver);
}

late_initcall(sunxi_loopback_dev_init);
module_exit(sunxi_loopback_dev_exit);

MODULE_AUTHOR("huhaoxin@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("sunxi soundcard platform of loopback");

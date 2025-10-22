// SPDX-License-Identifier: GPL-2.0-or-later
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

#define SUNXI_MODNAME		"sound-pcm"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>

#include "snd_sunxi_adapter.h"
#include "snd_sunxi_pcm.h"

#define SUNXI_DMAENGINE_PCM_DRV_NAME	"sunxi_dmaengine_pcm"

static u64 sunxi_pcm_mask = DMA_BIT_MASK(48);

static struct snd_pcm_hardware sunxi_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED
				| SNDRV_PCM_INFO_BLOCK_TRANSFER
				| SNDRV_PCM_INFO_MMAP
				| SNDRV_PCM_INFO_MMAP_VALID
				| SNDRV_PCM_INFO_PAUSE
				| SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S8
				| SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 8,
	/* value must be (2^n)Kbyte */
	.buffer_bytes_max	= SUNXI_AUDIO_CMA_MAX_BYTES,
	.period_bytes_min	= 256,
	.period_bytes_max	= SUNXI_AUDIO_CMA_MAX_BYTES / 2,
	.periods_min		= 1,
	.periods_max		= 8,
	.fifo_size		= 128,
};

static const char * const dmaengine_pcm_dma_channel_names[] = {
	[SNDRV_PCM_STREAM_PLAYBACK] = "tx",
	[SNDRV_PCM_STREAM_CAPTURE] = "rx",
};

struct dmaengine_pcm_runtime_data {
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;

	unsigned int pos;
};

/* data format transfer */
static unsigned int channel_status[192];
struct headbpcuv {
	unsigned char other:3;
	unsigned char V:1;
	unsigned char U:1;
	unsigned char C:1;
	unsigned char P:1;
	unsigned char B:1;
};
union head61937 {
	struct headbpcuv head0;
	unsigned char head1;
} head;
union word {
	struct {
		unsigned int bit0:1;
		unsigned int bit1:1;
		unsigned int bit2:1;
		unsigned int bit3:1;
		unsigned int bit4:1;
		unsigned int bit5:1;
		unsigned int bit6:1;
		unsigned int bit7:1;
		unsigned int bit8:1;
		unsigned int bit9:1;
		unsigned int bit10:1;
		unsigned int bit11:1;
		unsigned int bit12:1;
		unsigned int bit13:1;
		unsigned int bit14:1;
		unsigned int bit15:1;
		unsigned int rsvd:16;
	} bits;
	unsigned int wval;
} wordformat;

/* sunxi_transfer_format_61937_to_60958
 * ISO61937 to ISO60958, for HDMIAUDIO
 */
static int snd_sunxi_transfer_format_61937_to_60958(int *out, short *temp,
						    int samples, int rate,
						    enum HDMI_FORMAT data_fmt)
{
	int ret = 0;
	int i;
	static int numtotal;
	union word w1;

	samples >>= 1;
	head.head0.other = 0;
	head.head0.B = 1;
	head.head0.P = 0;
	head.head0.C = 0;
	head.head0.U = 0;
	head.head0.V = 1;

	for (i = 0; i < 192; i++)
		channel_status[i] = 0;

	channel_status[1] = 1;
	/* sample rates */
	if (rate == 32000) {
		channel_status[24] = 1;
		channel_status[25] = 1;
		channel_status[26] = 0;
		channel_status[27] = 0;
	} else if (rate == 44100) {
		channel_status[24] = 0;
		channel_status[25] = 0;
		channel_status[26] = 0;
		channel_status[27] = 0;
	} else if (rate == 48000) {
		channel_status[24] = 0;
		channel_status[25] = 1;
		channel_status[26] = 0;
		channel_status[27] = 0;
	} else if (rate == (32000*4)) {
		channel_status[24] = 1;
		channel_status[25] = 0;
		channel_status[26] = 0;
		channel_status[27] = 0;
	} else if (rate == (44100*4)) {
		channel_status[24] = 0;
		channel_status[25] = 0;
		channel_status[26] = 1;
		channel_status[27] = 1;
	} else if (rate == (48000*4)) {
		channel_status[24] = 0;
		channel_status[25] = 1;
		channel_status[26] = 1;
		channel_status[27] = 1;
		if (data_fmt == HDMI_FMT_DTS_HD || data_fmt == HDMI_FMT_MAT) {
			channel_status[24] = 1;
			channel_status[25] = 0;
			channel_status[26] = 0;
			channel_status[27] = 1;
		}
	} else {
		channel_status[24] = 0;
		channel_status[25] = 1;
		channel_status[26] = 0;
		channel_status[27] = 0;
	}

	for (i = 0; i < samples; i++, numtotal++) {
		if ((numtotal % 384 == 0) || (numtotal % 384 == 1))
			head.head0.B = 1;
		else
			head.head0.B = 0;

		head.head0.C = channel_status[(numtotal % 384)/2];

		if (numtotal % 384 == 0)
			numtotal = 0;

		w1.wval = (*temp) & (0xffff);

		head.head0.P = w1.bits.bit15 ^ w1.bits.bit14 ^ w1.bits.bit13 ^
			       w1.bits.bit12 ^ w1.bits.bit11 ^ w1.bits.bit10 ^
			       w1.bits.bit9 ^ w1.bits.bit8 ^ w1.bits.bit7 ^
			       w1.bits.bit6 ^ w1.bits.bit5 ^ w1.bits.bit4 ^
			       w1.bits.bit3 ^ w1.bits.bit2 ^ w1.bits.bit1 ^
			       w1.bits.bit0;

		ret = (int)(head.head1) << 24;
		/* 11 may can be replace by 8 or 12 */
		ret |= (int)((w1.wval)&(0xffff)) << 11;
		*out = ret;
		out++;
		temp++;
	}

	return 0;
}

static snd_pcm_uframes_t snd_dmaengine_pcm_pointer_raw(struct snd_pcm_substream *substream)
{
	struct dmaengine_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dma_tx_state state;
	enum dma_status status;
	unsigned int buf_size;
	unsigned int pos = 0;

	status = dmaengine_tx_status(prtd->dma_chan, prtd->cookie, &state);
	if (status == DMA_IN_PROGRESS || status == DMA_PAUSED) {
		buf_size = snd_pcm_lib_buffer_bytes(substream);
		if (state.residue > 0 && state.residue <= (buf_size << 1))
			pos = buf_size - (state.residue >> 1);

		sunxi_adpt_rt_delay(runtime, state);
	}

	return bytes_to_frames(runtime, pos);
}

static int sunxi_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream,
					    size_t buffer_bytes_max)
{
	struct snd_dma_buffer *buf = NULL;
	struct snd_pcm_str *streams = NULL;
	struct snd_pcm_substream *substream = NULL;

	SND_LOG_DEBUG("\n");

	streams = &pcm->streams[stream];
	if (IS_ERR_OR_NULL(streams)) {
		SND_LOG_ERR("stream=%d streams is null!\n", stream);
		return -EFAULT;
	}
	substream = pcm->streams[stream].substream;
	if (IS_ERR_OR_NULL(substream)) {
		SND_LOG_ERR("stream=%d substreams is null!\n", stream);
		return -EFAULT;
	}

	buf = &substream->dma_buffer;
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	if (buffer_bytes_max > SUNXI_AUDIO_CMA_MAX_BYTES) {
		buffer_bytes_max = SUNXI_AUDIO_CMA_MAX_BYTES;
		SND_LOG_WARN("buffer_bytes_max too max, set %zu\n", buffer_bytes_max);
	}
	if (buffer_bytes_max < SUNXI_AUDIO_CMA_MIN_BYTES) {
		buffer_bytes_max = SUNXI_AUDIO_CMA_MIN_BYTES;
		SND_LOG_WARN("buffer_bytes_max too min, set %zu\n", buffer_bytes_max);
	}

	buf->area = dma_alloc_coherent(pcm->card->dev, buffer_bytes_max,
				       &buf->addr, GFP_KERNEL);
	if (!buf->area) {
		SND_LOG_ERR("dmaengine alloc coherent failed.\n");
		return -ENOMEM;
	}
	buf->bytes = buffer_bytes_max;

	return 0;
}

static void sunxi_pcm_free_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_dma_buffer *buf;
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(substream)) {
		SND_LOG_WARN("stream=%d streams is null!\n", stream);
		return;
	}

	buf = &substream->dma_buffer;
	if (!buf->area) {
		SND_LOG_WARN("stream=%d buf->area is null!\n", stream);
		return;
	}

	dma_free_coherent(pcm->card->dev, buf->bytes, buf->area, buf->addr);
	buf->area = NULL;
}

int sunxi_pcm_probe(struct snd_soc_component *component)
{
	(void)component;

	SND_LOG_DEBUG("\n");

	return 0;
}

void sunxi_pcm_remove(struct snd_soc_component *component)
{
	(void)component;

	SND_LOG_DEBUG("\n");
}

int sunxi_pcm_construct(struct snd_soc_component *component, struct snd_soc_pcm_runtime *rtd)
{
	int ret;
	struct snd_pcm *pcm = rtd->pcm;
	struct snd_card *card = rtd->card->snd_card;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	size_t cma_bytes_play = SUNXI_AUDIO_CMA_BLOCK_BYTES;
	size_t cma_bytes_cap  = SUNXI_AUDIO_CMA_BLOCK_BYTES;

	struct sunxi_dma_params *dma_params_play = NULL;
	struct sunxi_dma_params *dma_params_cap = NULL;

	SND_LOG_DEBUG("\n");

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &sunxi_pcm_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	dma_params_play = sunxi_adpt_dai_dma_data_get(sunxi_adpt_rtd_cpu_dai(rtd),
						      SNDRV_PCM_STREAM_PLAYBACK);

	dma_params_cap = sunxi_adpt_dai_dma_data_get(sunxi_adpt_rtd_cpu_dai(rtd),
						     SNDRV_PCM_STREAM_CAPTURE);

	if (dai_link->playback_only) {
		if (!IS_ERR_OR_NULL(dma_params_play)) {
			cma_bytes_play *= dma_params_play->cma_kbytes;
			if (dma_params_play->dma_buf_mode == 1) {
				ret = sunxi_pcm_preallocate_dma_buffer(pcm,
							SNDRV_PCM_STREAM_PLAYBACK, cma_bytes_play);
				if (ret) {
					SND_LOG_ERR("pcm new playback failed, err=%d\n", ret);
					return ret;
				}
			}
		} else {
			SND_LOG_ERR("playback DMA params is NULL!\n");
			return -ENODEV;
		}
	} else if (dai_link->capture_only) {
		if (!IS_ERR_OR_NULL(dma_params_cap)) {
			cma_bytes_cap *= dma_params_cap->cma_kbytes;
			if (dma_params_cap->dma_buf_mode == 1) {
				ret = sunxi_pcm_preallocate_dma_buffer(pcm,
							SNDRV_PCM_STREAM_CAPTURE, cma_bytes_cap);
				if (ret) {
					SND_LOG_ERR("pcm new capture failed, err=%d\n", ret);
					return ret;
				}
			}
		} else {
			SND_LOG_ERR("capture DMA params is NULL!\n");
			return -ENODEV;
		}
	} else {
		if (!IS_ERR_OR_NULL(dma_params_play) && !IS_ERR_OR_NULL(dma_params_cap)) {
			cma_bytes_play *= dma_params_play->cma_kbytes;
			cma_bytes_cap *= dma_params_cap->cma_kbytes;

			ret = sunxi_pcm_preallocate_dma_buffer(pcm,
						SNDRV_PCM_STREAM_PLAYBACK, cma_bytes_play);
			if (ret) {
				SND_LOG_ERR("pcm new playback failed, err=%d\n", ret);
				goto err_pcm_prealloc_playback_buffer;
			}
			ret = sunxi_pcm_preallocate_dma_buffer(pcm,
						SNDRV_PCM_STREAM_CAPTURE, cma_bytes_cap);
			if (ret) {
				SND_LOG_ERR("pcm new capture failed, err=%d\n", ret);
				goto err_pcm_prealloc_capture_buffer;
			}
		} else {
			SND_LOG_ERR("playback or capture DMA params is NULL!\n");
			return -ENODEV;
		}
	}

	return 0;

err_pcm_prealloc_capture_buffer:
	sunxi_pcm_free_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
err_pcm_prealloc_playback_buffer:
	return ret;
}

void sunxi_pcm_destruct(struct snd_soc_component *component, struct snd_pcm *pcm, int stream)
{
	SND_LOG_DEBUG("\n");

	for (stream = 0; stream < SNDRV_PCM_STREAM_LAST; stream++) {
		sunxi_pcm_free_dma_buffer(pcm, stream);
	}
}

int sunxi_pcm_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm *pcm = rtd->pcm;
	struct snd_card *card = rtd->card->snd_card;
	struct device *dev = component->dev;
	struct sunxi_dma_params *dma_params = NULL;
	struct dma_chan *chan;
	size_t cma_bytes  = SUNXI_AUDIO_CMA_BLOCK_BYTES;

	SND_LOG_DEBUG("\n");

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &sunxi_pcm_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd), substream);
	if (!IS_ERR_OR_NULL(dma_params)) {
		cma_bytes *= dma_params->cma_kbytes;
		if (dma_params->dma_buf_mode == 0) {
			ret = sunxi_pcm_preallocate_dma_buffer(pcm, substream->stream, cma_bytes);
			if (ret) {
				SND_LOG_ERR("%d pcm new failed, err=%d\n", substream->stream, ret);
				return ret;
			}
		}
	} else {
		SND_LOG_ERR("DMA params is NULL!\n");
		return -ENODEV;
	}

	/* Set HW params now that initialization is complete */
	sunxi_pcm_hardware.buffer_bytes_max = dma_params->cma_kbytes * SUNXI_AUDIO_CMA_BLOCK_BYTES;
	sunxi_pcm_hardware.period_bytes_max = sunxi_pcm_hardware.buffer_bytes_max / 2;
	sunxi_pcm_hardware.fifo_size	    = dma_params->fifo_size;
	snd_soc_set_runtime_hwparams(substream, &sunxi_pcm_hardware);
	ret = snd_pcm_hw_constraint_integer(substream->runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		SND_LOG_ERR("constraint_integer failed, err %d\n", ret);
		goto err_pcm_prealloc_buffer;
	}

	chan = dma_request_chan(dev, dmaengine_pcm_dma_channel_names[substream->stream]);
	if (IS_ERR(chan)) {
		SND_LOG_ERR("DMA channels request %s failed, err -> %d.\n",
			    dmaengine_pcm_dma_channel_names[substream->stream],
			    IS_ERR(chan));
		goto err_pcm_prealloc_buffer;
	}

	ret = snd_dmaengine_pcm_open(substream, chan);
	if (ret < 0) {
		SND_LOG_ERR("dmaengine pcm open failed with err %d\n", ret);
		goto err_pcm_prealloc_buffer;
	}

	/* In soc_pcm_open, set pin default state for each component, but in soc_pcm_close
	 * set pin sleep state for the component which active is 0. Thost caused
	 * playback selient or capture nodata if playback and capture at the same time.
	 */
	sunxi_adpt_runtime_action(component, 1);

	return 0;

err_pcm_prealloc_buffer:
	sunxi_pcm_free_dma_buffer(pcm, substream->stream);
	return -1;
}

int sunxi_pcm_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm *pcm = rtd->pcm;
	struct sunxi_dma_params *dma_params = NULL;

	SND_LOG_DEBUG("\n");

	dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd), substream);

	snd_dmaengine_pcm_close_release_chan(substream);

	if (!IS_ERR_OR_NULL(dma_params)) {
		if (dma_params->dma_buf_mode == 0)
			sunxi_pcm_free_dma_buffer(pcm, substream->stream);
	} else {
		SND_LOG_ERR("DMA params is NULL!\n");
	}

	sunxi_adpt_runtime_action(component, -1);

	return 0;
}

int sunxi_pcm_ioctl(struct snd_soc_component *component,
		    struct snd_pcm_substream *substream,
		    unsigned int cmd, void *arg)
{
	SND_LOG_DEBUG("cmd -> %u\n", cmd);

	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

int sunxi_pcm_hw_params(struct snd_soc_component *component,
			struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	struct sunxi_dma_params *dma_params;
	struct dma_slave_config slave_config;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->dev;
	struct dma_chan *chan;
	int ret;

	SND_LOG_DEBUG("\n");

	chan = snd_dmaengine_pcm_get_chan(substream);
	if (chan == NULL) {
		SND_LOG_ERR("dma pcm get chan failed! chan is NULL\n");
		return -EINVAL;
	}

	dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd), substream);

	ret = snd_hwparams_to_dma_slave_config(substream, params, &slave_config);
	if (ret) {
		SND_LOG_ERR("hw params config failed, err %d\n", ret);
		return ret;
	}

	slave_config.dst_maxburst = dma_params->dst_maxburst;
	slave_config.src_maxburst = dma_params->src_maxburst;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		slave_config.dst_addr =	dma_params->dma_addr;
		slave_config.src_addr_width = slave_config.dst_addr_width;
	} else {
		slave_config.src_addr =	dma_params->dma_addr;
		slave_config.dst_addr_width = slave_config.src_addr_width;
	}

	if (dma_params->hdmi_fmt > HDMI_FMT_PCM) {
		slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

		if (!dev->dma_mask)
			dev->dma_mask = &sunxi_pcm_mask;
		if (!dev->coherent_dma_mask)
			dev->coherent_dma_mask = 0xffffffff;

		dma_params->raw_dma_area = dma_alloc_coherent(dev,
							      (params_buffer_bytes(params) * 2),
							      &(dma_params->raw_dma_addr),
							      GFP_KERNEL);
		if (dma_params->raw_dma_area == NULL) {
			SND_LOG_ERR("pcm rawdata mode get mem failed\n");
			return -ENOMEM;
		}
		dma_params->pcm_dma_addr = substream->dma_buffer.addr;
		substream->dma_buffer.addr = (dma_addr_t)(dma_params->raw_dma_addr);
	}

	ret = dmaengine_slave_config(chan, &slave_config);
	if (ret < 0) {
		SND_LOG_ERR("dma slave config failed, err %d\n", ret);
		return ret;
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	return 0;
}

int sunxi_pcm_hw_free(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->dev;
	struct sunxi_dma_params *dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd),
								       substream);

	SND_LOG_DEBUG("\n");
	if (snd_pcm_lib_buffer_bytes(substream) && dma_params->hdmi_fmt > HDMI_FMT_PCM) {
		dma_free_coherent(dev, (snd_pcm_lib_buffer_bytes(substream) * 2),
				  dma_params->raw_dma_area, dma_params->raw_dma_addr);
		substream->dma_buffer.addr = dma_params->pcm_dma_addr;
		dma_params->raw_dma_area = NULL;
	}

	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

int sunxi_pcm_prepare(struct snd_soc_component *component,
		      struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd),
								       substream);

	if (dma_params->hdmi_fmt > HDMI_FMT_PCM) {
		if (dma_params->change_size_flag) {
			runtime->buffer_size = dma_params->buffer_size;
			runtime->period_size = dma_params->period_size;
		} else {
			dma_params->change_size_flag = true;
			runtime->buffer_size *= 2;
			runtime->period_size *= 2;
			dma_params->buffer_size = runtime->buffer_size;
			dma_params->period_size = runtime->period_size;
		}
	} else {
		if (dma_params->change_size_flag) {
			dma_params->change_size_flag = false;
			runtime->buffer_size = dma_params->buffer_size / 2;
			runtime->period_size = dma_params->period_size / 2;
		}
	}

	return 0;
}

int sunxi_pcm_trigger(struct snd_soc_component *component,
		      struct snd_pcm_substream *substream,
		      int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd),
								       substream);

	SND_LOG_DEBUG("cmd -> %d\n", cmd);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_START);
			if (dma_params->hdmi_fmt > HDMI_FMT_PCM) {
				if (dma_params->change_size_flag) {
					dma_params->change_size_flag = false;
					runtime->buffer_size = dma_params->buffer_size / 2;
					runtime->period_size = dma_params->period_size / 2;
				}
			}
		break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_STOP);
			if (dma_params->hdmi_fmt > HDMI_FMT_PCM) {
				if (dma_params->change_size_flag) {
					dma_params->change_size_flag = false;
					runtime->buffer_size = dma_params->buffer_size / 2;
					runtime->period_size = dma_params->period_size / 2;
				}
			}
		break;
		default:
			SND_LOG_ERR("unsupport trigger -> %d\n", cmd);
			return -1;
		break;
		}
	} else {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_START);
		break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_STOP);
		break;
		default:
			SND_LOG_ERR("unsupport trigger -> %d\n", cmd);
			return -1;
		break;
		}
	}

	return 0;
}

snd_pcm_uframes_t sunxi_pcm_pointer(struct snd_soc_component *component,
				    struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd),
								       substream);

	if (dma_params->hdmi_fmt > HDMI_FMT_PCM)
		return snd_dmaengine_pcm_pointer_raw(substream);
	else
		return snd_dmaengine_pcm_pointer(substream);
}

int sunxi_pcm_mmap(struct snd_soc_component *component,
		   struct snd_pcm_substream *substream,
		   struct vm_area_struct *vma)
{
	SND_LOG_DEBUG("\n");

	if (substream->runtime == NULL) {
		SND_LOG_ERR("substream->runtime is null\n");
		return -EFAULT;
	}

	return dma_mmap_wc(substream->pcm->card->dev, vma,
			   substream->runtime->dma_area,
			   substream->runtime->dma_addr,
			   substream->runtime->dma_bytes);
}

int sunxi_pcm_copy(struct snd_soc_component *component,
		   struct snd_pcm_substream *substream, int channel,
		   unsigned long hwoff, void __user *buf, unsigned long bytes)
{
	int ret = 0;
	char *hwbuf;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd),
								       substream);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		hwbuf = runtime->dma_area + hwoff;
		if (copy_from_user(hwbuf, buf, bytes))
			return -EFAULT;

		if (dma_params->hdmi_fmt > HDMI_FMT_PCM) {
			char *hdmihw_area = dma_params->raw_dma_area + 2 * hwoff;
			snd_sunxi_transfer_format_61937_to_60958(
				(int *)hdmihw_area, (short *)hwbuf,
				bytes, runtime->rate, dma_params->hdmi_fmt);
		}

	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		hwbuf = runtime->dma_area + hwoff;
		if (copy_to_user(buf, hwbuf, bytes))
			return -EFAULT;
	}

	return ret;
}

int snd_sunxi_dma_platform_register(struct device *dev)
{
	SND_LOG_DEBUG("\n");

	return sunxi_adpt_register_component(dev);
}
EXPORT_SYMBOL_GPL(snd_sunxi_dma_platform_register);

void snd_sunxi_dma_platform_unregister(struct device *dev)
{
	SND_LOG_DEBUG("\n");
	sunxi_adpt_unregister_component(dev);
}
EXPORT_SYMBOL_GPL(snd_sunxi_dma_platform_unregister);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi ASoC DMA driver");

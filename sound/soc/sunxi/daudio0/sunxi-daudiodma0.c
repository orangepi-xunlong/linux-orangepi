/*
 * sound\soc\sunxi\daudio\sunxi-daudiodma.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#undef 	AR200_AUDIO
//#define AR200_AUDIO
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dma/sunxi-dma.h>
#include <linux/arisc/arisc.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>

#include <asm/dma.h>
#include <mach/hardware.h>
#include "sunxi-daudiodma0.h"

#ifdef AUDIO_KARAOKE
atomic_t cap_num;
dma_addr_t daudiocap_dma_addr = 0;
unsigned char *daudiocap_dma_area = NULL;	/* DMA area */
unsigned char *daudiocap_area = NULL;
struct timeval tv_start, tv_cur;
#endif

#ifdef AR200_AUDIO
static volatile unsigned int capture_remain_byte 	= 0;
static volatile unsigned int play_remain_byte 		= 0;

struct tdm_runtime_data {
	audio_cb_t 			play_done_cb;
	audio_cb_t 			capture_done_cb;
	arisc_audio_mem_t	audio_mem;
	unsigned int 		pos;
	int 				mode;
};
#endif

static const struct snd_pcm_hardware sunxi_pcm_play_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
				      SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				      SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 1024*1024,    /* value must be (2^n)Kbyte size */
	.period_bytes_min	= 256,
	.period_bytes_max	= 1024*64,
	.periods_min		= 1,
	.periods_max		= 8,
	.fifo_size		= 128,
};

static const struct snd_pcm_hardware sunxi_pcm_capture_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
				      SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				      SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 1024*1024,    /* value must be (2^n)Kbyte size */
	.period_bytes_min	= 256,
	.period_bytes_max	= 1024*64,
	.periods_min		= 1,
	.periods_max		= 8,
	.fifo_size		= 128,
};

#ifdef AR200_AUDIO
static int sunxi_play_perdone(void *parg)
{
	struct snd_pcm_substream *substream = parg;
	struct tdm_runtime_data *prtd = substream->runtime->private_data;
	unsigned long flags;

	snd_pcm_stream_lock_irqsave(substream, flags);
	if (!substream ||!substream->runtime) {
		snd_pcm_stream_unlock_irqrestore(substream, flags);
		return 0;
	}
	prtd->pos += snd_pcm_lib_period_bytes(substream);
	if (prtd->pos >= snd_pcm_lib_buffer_bytes(substream)) {
		prtd->pos = 0;
	}
	snd_pcm_stream_unlock_irqrestore(substream, flags);

	snd_pcm_period_elapsed(substream);
	return 0;
}

static int sunxi_capture_perdone(void *parg)
{
	struct snd_pcm_substream *substream = parg;
	struct tdm_runtime_data *prtd = substream->runtime->private_data;

	prtd->pos += snd_pcm_lib_period_bytes(substream);
	if (prtd->pos >= snd_pcm_lib_buffer_bytes(substream)) {
		prtd->pos = 0;
	}
	snd_pcm_period_elapsed(substream);
	return 0;
}
#endif

static int sunxi_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct sunxi_dma_params *dmap;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
#ifdef AR200_AUDIO
	struct tdm_runtime_data *prtd = substream->runtime->private_data;
#else
	struct device *dev = rtd->platform->dev;
	struct dma_chan *chan = snd_dmaengine_pcm_get_chan(substream);
	struct dma_slave_config slave_config;
	int ret;
#endif
	dmap = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

#ifndef AR200_AUDIO
	ret = snd_hwparams_to_dma_slave_config(substream, params, &slave_config);
	if (ret) {
		dev_err(dev, "hw params config failed with err %d\n", ret);
		return ret;
	}
#endif

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
#ifdef AR200_AUDIO
		prtd->mode 						= AUDIO_PLAY;
		prtd->audio_mem.mode			= prtd->mode;
		prtd->audio_mem.sram_base_addr 	= AUDIO_SRAM_BASE_PALY;
		prtd->audio_mem.buffer_size		= params_buffer_bytes(params);
		prtd->audio_mem.period_size		= params_buffer_bytes(params);
		arisc_buffer_period_paras(prtd->audio_mem);
		/*
		* set callback
		*/
		memset(&prtd->play_done_cb, 0, sizeof(prtd->play_done_cb));
		prtd->play_done_cb.handler = sunxi_play_perdone;
		prtd->play_done_cb.arg = substream;
		arisc_audio_cb_register(prtd->mode, prtd->play_done_cb.handler, prtd->play_done_cb.arg);
		substream->dma_buffer.addr = AUDIO_SRAM_BASE_PALY;
		substream->dma_buffer.area = (unsigned char *)0xf8117000;
#else
		if (SNDRV_PCM_FORMAT_S16_LE == params_format(params)) {
			slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
			slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		} else {
			slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		}
		slave_config.dst_addr = dmap->dma_addr;
		#if defined CONFIG_ARCH_SUN8IW6
		slave_config.dst_maxburst = 8;
		slave_config.src_maxburst = 8;
		#else
		slave_config.dst_maxburst = 4;
		slave_config.src_maxburst = 4;
		#endif
		#if defined CONFIG_ARCH_SUN9IW1
		slave_config.slave_id = sunxi_slave_id(DRQDST_R_DAUDIO_1_TX, DRQSRC_SDRAM);
		#elif defined CONFIG_ARCH_SUN8IW8 || defined CONFIG_ARCH_SUN8IW7
		slave_config.slave_id = sunxi_slave_id(DRQDST_DAUDIO_0_TX, DRQSRC_SDRAM);
		#else
		slave_config.slave_id = sunxi_slave_id(DRQDST_TDM_TX, DRQSRC_SDRAM);
		#endif
#endif
	} else {
#ifdef AR200_AUDIO
		prtd->mode 						= AUDIO_CAPTURE;
		/*use dram buffer for capture*/
		prtd->audio_mem.sram_base_addr 	= AUDIO_SRAM_BASE_CAPTURE;
		prtd->audio_mem.buffer_size		= params_buffer_bytes(params);
		prtd->audio_mem.period_size		= params_buffer_bytes(params);
		substream->dma_buffer.addr = AUDIO_SRAM_BASE_CAPTURE;
		substream->dma_buffer.area = (unsigned char *)0xf811f000;
		arisc_buffer_period_paras(prtd->audio_mem);
		/*
		* set callback
		*/
		memset(&prtd->capture_done_cb, 0, sizeof(prtd->capture_done_cb));
		prtd->capture_done_cb.handler = sunxi_capture_perdone;
		prtd->capture_done_cb.arg = substream;
		arisc_audio_cb_register(prtd->mode, prtd->capture_done_cb.handler, prtd->capture_done_cb.arg);
#else
		slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		slave_config.src_addr = dmap->dma_addr;
		#if defined CONFIG_ARCH_SUN8IW6
		slave_config.dst_maxburst = 8;
		slave_config.src_maxburst = 8;
		#else
		slave_config.dst_maxburst = 4;
		slave_config.src_maxburst = 4;
		#endif
		#if defined CONFIG_ARCH_SUN9IW1
		slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_R_DAUDIO_1_RX);
		#elif defined CONFIG_ARCH_SUN8IW8 || defined CONFIG_ARCH_SUN8IW7
			#ifdef CONFIG_SND_SOC_RT3261
				slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM, 0x01c22810);
			#else
				slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_DAUDIO_0_RX);
			#endif
		#else
		slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQDST_TDMRX);
		#endif
#endif
	}

#ifndef AR200_AUDIO
	ret = dmaengine_slave_config(chan, &slave_config);
	if (ret < 0) {
		dev_err(dev, "dma slave config failed with err %d\n", ret);
		return ret;
	}
#endif
#ifdef CONFIG_ARCH_SUN9IW1
#ifndef AR200_AUDIO
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		substream->dma_buffer.addr = 0x14000;
		substream->dma_buffer.area = (unsigned char *)0xf0014000;
		memset((void *)0xf0014000, 0, 0x4000);
	}
#endif
#endif
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	return 0;
}

static int sunxi_pcm_hw_free(struct snd_pcm_substream *substream)
{
	snd_pcm_set_runtime_buffer(substream, NULL);
	
	return 0;
}
#ifdef AR200_AUDIO
static snd_pcm_uframes_t sunxi_pcm_pointer(struct snd_pcm_substream *substream)
{
	unsigned long play_res = 0, capture_res = 0;
	struct tdm_runtime_data *prtd = NULL;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		prtd = substream->runtime->private_data;

		arisc_get_position(prtd->mode, (unsigned int*)&play_remain_byte);
		/*
		*	use half buffer circle mode to transfer audio buffer.
		*	the pointer pos should be:
		*	play_res = play_src_addr - play_start_addr + play_period_size - play_remain_byte;
		*	while use half buffer circle mode(transfer one buffer in circle mode,
		*	while the dma channel have transfer half buffer, half buffer done irq happened.) transfer audio buffer.
		*	play_src_addr == play_start_addr;
		*	so play_res = play_src_addr - play_start_addr + play_period_size - play_remain_byte;
		*				= play_period_size - play_remain_byte;
		*/
		play_res = prtd->audio_mem.buffer_size - play_remain_byte;

		return bytes_to_frames(substream->runtime, play_res);
	} else {
		prtd = substream->runtime->private_data;
		return bytes_to_frames(substream->runtime, prtd->pos);
		arisc_get_position(prtd->mode, (unsigned int*)&capture_remain_byte);
		capture_res = prtd->audio_mem.period_size - capture_remain_byte;
		if (capture_res >= snd_pcm_lib_buffer_bytes(substream)) {
			if (capture_res == snd_pcm_lib_buffer_bytes(substream))
			capture_res = 0;
		}
		return bytes_to_frames(substream->runtime, capture_res);
	}
}
#endif

static int sunxi_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
#ifdef AR200_AUDIO
	struct tdm_runtime_data *prtd = NULL;
	prtd = substream->runtime->private_data;
#endif
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
#ifdef AR200_AUDIO
			arisc_audio_start(prtd->mode);
#else
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_START);
#endif
		return 0;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
#ifdef AR200_AUDIO
			arisc_audio_stop(prtd->mode);
#else
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_STOP);
#endif
		return 0;
		}
	} else {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
#ifdef AUDIO_KARAOKE
			memset(daudiocap_dma_area, 0, 0x4000);
			atomic_inc(&cap_num);
			do_gettimeofday(&tv_start);
#endif
#ifdef AR200_AUDIO
			arisc_audio_start(prtd->mode);
#else
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_START);
#endif
		return 0;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
#ifdef AUDIO_KARAOKE
		do_gettimeofday(&tv_start);
		atomic_dec(&cap_num);
#endif
#ifdef AR200_AUDIO
			arisc_audio_stop(prtd->mode);
#else
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_STOP);
#endif
		return 0;
		}
	}
	return 0;
}

static int sunxi_pcm_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
#ifdef AR200_AUDIO
	struct tdm_runtime_data *prtd = NULL;
#else
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
#endif

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* Set HW params now that initialization is complete */
		snd_soc_set_runtime_hwparams(substream, &sunxi_pcm_play_hardware);
		ret = snd_pcm_hw_constraint_integer(substream->runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
		if (ret < 0)
			return ret;
#ifdef AR200_AUDIO
		prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
		if (!prtd)
			return -ENOMEM;
		substream->runtime->private_data = prtd;
#else
	#ifdef CONFIG_ARCH_SUN9IW1
	ret = snd_dmaengine_pcm_open(substream, sunxi_rdma_filter_fn, (void *)SUNXI_RDMA_DRV);
	#else
	ret = snd_dmaengine_pcm_open(substream, NULL, NULL);
	#endif
	if (ret) {
		dev_err(dev, "dmaengine pcm open failed with err %d\n", ret);
	}
#endif
	} else {
		/* Set HW params now that initialization is complete */
		snd_soc_set_runtime_hwparams(substream, &sunxi_pcm_capture_hardware);
		ret = snd_pcm_hw_constraint_integer(substream->runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
		if (ret < 0)
			return ret;
#ifdef AR200_AUDIO
		prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
		if (!prtd)
			return -ENOMEM;
		substream->runtime->private_data = prtd;
#else
		#ifdef CONFIG_ARCH_SUN9IW1
			ret = snd_dmaengine_pcm_open(substream, sunxi_rdma_filter_fn, SUNXI_RDMA_DRV);
		#else
			ret = snd_dmaengine_pcm_open(substream, NULL, NULL);
		#endif
		if (ret) {
			dev_err(dev, "dmaengine pcm open failed with err %d\n", ret);
		}
#endif
	}
	return 0;
}

static int sunxi_pcm_close(struct snd_pcm_substream *substream)
{
#ifdef AR200_AUDIO
	struct tdm_runtime_data *prtd = NULL;
	prtd = substream->runtime->private_data;
#endif
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
#ifdef AR200_AUDIO
		arisc_audio_cb_unregister(prtd->mode, prtd->play_done_cb.handler);
		kfree(prtd);
#else
		snd_dmaengine_pcm_close(substream);
#endif
	} else {
#ifdef AR200_AUDIO
		arisc_audio_cb_unregister(prtd->mode, prtd->play_done_cb.handler);
		kfree(prtd);
#else
		snd_dmaengine_pcm_close(substream);
#endif
	}
	return 0;
}

static int sunxi_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = NULL;
	if (substream->runtime!=NULL) {
		runtime = substream->runtime;

		return dma_mmap_writecombine(substream->pcm->card->dev, vma,
					     runtime->dma_area,
					     runtime->dma_addr,
					     runtime->dma_bytes);
	} else {
		return -1;
	}
}

#ifdef AUDIO_KARAOKE
void daudio_restore_capbuf(int *daudiocap_area, short* hwbuf, int samples)
{
	memcpy(daudiocap_area, hwbuf, samples);
}

static int sunxi_pcm_copy(struct snd_pcm_substream *substream, int a,
	 snd_pcm_uframes_t hwoff, void __user *buf, snd_pcm_uframes_t frames)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		char *hwbuf = runtime->dma_area + frames_to_bytes(runtime, hwoff);
		if (copy_from_user(hwbuf, buf, frames_to_bytes(runtime, frames))) {
			return -EFAULT;
		}
		do_gettimeofday(&tv_cur);
		/*mixer capture buffer to the play output buffer*/
		if ((atomic_read(&cap_num) == 1) && ((tv_cur.tv_sec - tv_start.tv_sec) > 4)) {
			if (frames_to_bytes(runtime, frames)>snd_pcm_lib_period_bytes(substream)) {
				audio_mixer_buffer(hwbuf, daudiocap_dma_area, hwbuf, snd_pcm_lib_period_bytes(substream));
				hwbuf = hwbuf+snd_pcm_lib_period_bytes(substream);
				audio_mixer_buffer(hwbuf, daudiocap_area, hwbuf, snd_pcm_lib_period_bytes(substream));
			} else {
				audio_mixer_buffer(hwbuf, daudiocap_area, hwbuf, frames_to_bytes(runtime, frames));
			}
		}
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		char *hwbuf = runtime->dma_area + frames_to_bytes(runtime, hwoff);
		daudiocap_area = daudiocap_dma_area + frames_to_bytes(runtime, hwoff);
		/*restore dma capture buffer to tmp buffer area for mixer output to hdmi/spdif/codec*/
		daudio_restore_capbuf((int*)daudiocap_area, (short*)hwbuf, frames_to_bytes(runtime, frames));
		if (copy_to_user(buf, hwbuf, frames_to_bytes(runtime, frames))) {
			return -EFAULT;
		}
	}

	return ret;
}
#endif

static struct snd_pcm_ops sunxi_pcm_ops = {
	.open			= sunxi_pcm_open,
	.close			= sunxi_pcm_close,
	.ioctl			= snd_pcm_lib_ioctl,
	.hw_params		= sunxi_pcm_hw_params,
	.hw_free		= sunxi_pcm_hw_free,
	.trigger		= sunxi_pcm_trigger,
#ifdef AR200_AUDIO
	.pointer		= sunxi_pcm_pointer,
#else
#ifdef AUDIO_KARAOKE
	.pointer        = snd_dmaengine_pcm_pointer_no_residue,
	.copy			= sunxi_pcm_copy,
#else
	.pointer        = snd_dmaengine_pcm_pointer,
#endif
#endif
	.mmap			= sunxi_pcm_mmap,
};

static int sunxi_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = 0;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		size = sunxi_pcm_play_hardware.buffer_bytes_max;
	} else {
		size = sunxi_pcm_capture_hardware.buffer_bytes_max;
	}
	
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;
	buf->bytes = size;
	return 0;
}

static void sunxi_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;
	
	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
#ifdef AUDIO_KARAOKE
	dma_free_coherent(NULL, 0x4000, daudiocap_dma_area, daudiocap_dma_addr);
	daudiocap_dma_area = NULL;
	daudiocap_area = NULL;
#endif
}

static u64 sunxi_pcm_mask = DMA_BIT_MASK(32);

static int sunxi_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &sunxi_pcm_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = sunxi_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = sunxi_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
#ifdef AUDIO_KARAOKE
	atomic_set(&cap_num, 0);
	daudiocap_dma_area = dma_alloc_coherent(NULL, 0x4000, &daudiocap_dma_addr, GFP_KERNEL);
#endif
 	out:
		return ret;
}

static struct snd_soc_platform_driver sunxi_soc_platform = {
	.ops		= &sunxi_pcm_ops,
	.pcm_new	= sunxi_pcm_new,
	.pcm_free	= sunxi_pcm_free_dma_buffers,
};

static int __init sunxi_daudio_pcm_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &sunxi_soc_platform);
}

static int __exit sunxi_daudio_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device sunxi_daudio_pcm_device = {
	.name = "sunxi-daudio-pcm-audio",
};

/*method relating*/
static struct platform_driver sunxi_daudio_pcm_driver = {
	.probe = sunxi_daudio_pcm_probe,
	.remove = __exit_p(sunxi_daudio_pcm_remove),
	.driver = {
		.name = "sunxi-daudio-pcm-audio",
		.owner = THIS_MODULE,
	},
};

static int __init sunxi_soc_platform_daudio_init(void)
{
	int err = 0;	
	if((err = platform_device_register(&sunxi_daudio_pcm_device)) < 0)
		return err;

	if ((err = platform_driver_register(&sunxi_daudio_pcm_driver)) < 0)
		return err;
	return 0;	
}
module_init(sunxi_soc_platform_daudio_init);

static void __exit sunxi_soc_platform_daudio_exit(void)
{
	return platform_driver_unregister(&sunxi_daudio_pcm_driver);
}
module_exit(sunxi_soc_platform_daudio_exit);

MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUNXI I2S DMA module");
MODULE_LICENSE("GPL");

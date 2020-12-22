/*
 * sound\soc\sunxi\hdmiaudio\sunxi-hdmipcm.c
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <asm/dma.h>
#include <mach/hardware.h>
#include <sound/dmaengine_pcm.h>
#include <linux/dma/sunxi-dma.h>
#if defined CONFIG_ARCH_SUN9I || defined CONFIG_ARCH_SUN8IW6
#include "sunxi-hdmipcm.h"
#endif
#ifdef CONFIG_ARCH_SUN8IW7
#include "sunxi-hdmitdm.h"
#endif
atomic_t card_open_num;

static int raw_flag = 1;
static dma_addr_t hdmiraw_dma_addr = 0;
static dma_addr_t hdmipcm_dma_addr = 0;
static unsigned char *hdmiraw_dma_area;	/* DMA area */
static unsigned int channel_status[192];

#ifdef AUDIO_KARAOKE
extern atomic_t cap_num;
extern unsigned char *daudiocap_area;
extern dma_addr_t daudiocap_dma_addr;
extern unsigned char *daudiocap_dma_area;	/* DMA area */
extern struct timeval tv_start, tv_cur;
#endif

typedef struct headbpcuv{
	unsigned other:3;
    unsigned V:1;
    unsigned U:1;
    unsigned C:1;
    unsigned P:1;
    unsigned B:1;
} headbpcuv;

union head61937
{
 headbpcuv head0;
 unsigned char head1;
}head;

typedef union word
{
	struct
	{
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
	}bits;
	unsigned int wval;
}word_format;

static const struct snd_pcm_hardware sunxi_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
				      SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				      SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE|SNDRV_PCM_FMTBIT_S32_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min		= 32000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 8,
	.buffer_bytes_max	= 1024*1024,    /* value must be (2^n)Kbyte size */
	.period_bytes_min	= 156,
	.period_bytes_max	= 1024*1024,
	.periods_min		= 1,
	.periods_max		= 8,
	.fifo_size			= 128,
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

int hdmi_transfer_format_61937_to_60958(int *out,short* temp, int samples)
{
	int ret =0;
	int i;
	static int numtotal = 0;
	union word w1;

	samples>>=1;
	head.head0.other = 0;
	head.head0.B = 1;
	head.head0.P = 0;
	head.head0.C = 0;
	head.head0.U = 0;
	head.head0.V = 1;

	for (i=0 ; i<192; i++)
	{
		channel_status[i] = 0;
	}
	channel_status[1] = 1;
	//sample rates
	channel_status[24] = 0;
	channel_status[25] = 1;
	channel_status[26] = 0;
	channel_status[27] = 0;

	for (i = 0 ;i<samples;i++,numtotal++) {
		if( (numtotal%384 == 0) || (numtotal%384 == 1) )
		{
			head.head0.B = 1;
		}
		else
		{
			head.head0.B = 0;
		}
		head.head0.C = channel_status[(numtotal%384)/2];

		if(numtotal%384 == 0)
		{
			numtotal = 0;
		}

		w1.wval = (*temp)&(0xffff);

		head.head0.P = w1.bits.bit15 ^ w1.bits.bit14 ^ w1.bits.bit13 ^ w1.bits.bit12
		              ^w1.bits.bit11 ^ w1.bits.bit10 ^ w1.bits.bit9 ^ w1.bits.bit8
		              ^w1.bits.bit7 ^ w1.bits.bit6 ^ w1.bits.bit5 ^ w1.bits.bit4
		              ^w1.bits.bit3 ^ w1.bits.bit2 ^ w1.bits.bit1 ^ w1.bits.bit0;

		ret = (int)(head.head1)<<24;
		ret |= (int)((w1.wval)&(0xffff))<<11;//8 or 12
		*out = ret;
		out++;
		temp++;
	}
	return 0;
}

static int sunxi_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct dma_chan *chan = snd_dmaengine_pcm_get_chan(substream);
	struct sunxi_dma_params *dmap;
	struct dma_slave_config slave_config;
	int ret;
#ifdef CONFIG_SND_SUNXI_SOC_SUPPORT_AUDIO_RAW
	raw_flag = params_raw(params);
#else
	raw_flag = hdmi_format;
#endif
	dmap = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

	ret = snd_hwparams_to_dma_slave_config(substream, params,
						&slave_config);
	if (ret) {
		dev_err(dev, "hw params config failed with err %d\n", ret);
		return ret;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		slave_config.dst_addr = dmap->dma_addr;
#ifdef CONFIG_ARCH_SUN8IW6
		slave_config.dst_maxburst = 8;
		slave_config.src_maxburst = 8;
#else
		slave_config.dst_maxburst = 4;
		slave_config.src_maxburst = 4;
#endif
#ifdef CONFIG_ARCH_SUN8IW1
		slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.slave_id = sunxi_slave_id(DRQDST_HDMI_AUDIO, DRQSRC_SDRAM);
#else
		/*
		* raw_flag == 1 pcm mode;
		*/
		if (SNDRV_PCM_FORMAT_S16_LE == params_format(params)) {
				slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
				slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		} else {
			slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		}
		/*raw_flag>1. rawdata*/
		if (raw_flag > 1) {
			slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			 strcpy(substream->pcm->card->id, "sndhdmiraw");
			 hdmiraw_dma_area = dma_alloc_coherent(NULL, (2*params_buffer_bytes(params)), &hdmiraw_dma_addr, GFP_KERNEL);
			 hdmipcm_dma_addr = substream->dma_buffer.addr;
			 substream->dma_buffer.addr = hdmiraw_dma_addr;
		} else {
			strcpy(substream->pcm->card->id, "sndhdmi");
		}
		#ifdef CONFIG_ARCH_SUN9I
		slave_config.slave_id = sunxi_slave_id(DRQDST_DAUDIO_1_TX, DRQSRC_SDRAM);
		#endif
		#if defined (CONFIG_ARCH_SUN8IW7) || defined (CONFIG_ARCH_SUN8IW6)
		slave_config.slave_id = sunxi_slave_id(DRQDST_DAUDIO_2_TX, DRQSRC_SDRAM);
		#endif	
#endif
	} else {
		slave_config.src_addr = dmap->dma_addr;
		#ifdef CONFIG_ARCH_SUN8IW6
		slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.src_maxburst = 8;
		slave_config.dst_maxburst = 8;
		#else
		slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		slave_config.src_maxburst = 4;
		slave_config.dst_maxburst = 4;
		#endif
#ifdef CONFIG_ARCH_SUN8IW1
		slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_HDMI_AUDIO);
#endif
#ifdef CONFIG_ARCH_SUN8IW7
		slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_DAUDIO_2_RX);
#endif
	}

	ret = dmaengine_slave_config(chan, &slave_config);
	if (ret < 0) {
		dev_err(dev, "dma slave config failed with err %d\n", ret);
		return ret;
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	return 0;
}

static int sunxi_pcm_hw_free(struct snd_pcm_substream *substream)
{
	if (snd_pcm_lib_buffer_bytes(substream)&& (raw_flag > 1)) {
		dma_free_coherent(NULL, (2*snd_pcm_lib_buffer_bytes(substream)),
					      hdmiraw_dma_area, hdmiraw_dma_addr);
		substream->dma_buffer.addr = hdmipcm_dma_addr;
	}

	snd_pcm_set_runtime_buffer(substream, NULL);
  
	return 0;
}

static int sunxi_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_START);
		return 0;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_STOP);
		return 0;
		}
	} else {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_START);
		return 0;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_STOP);
		return 0;
		}
	}
	return 0;
}

static int sunxi_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	int ret = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ret = atomic_read(&card_open_num);
		if (ret > 0) {
			return -EINVAL;
		}
		/* Set HW params now that initialization is complete */
		snd_soc_set_runtime_hwparams(substream, &sunxi_pcm_hardware);

		ret = snd_dmaengine_pcm_open(substream, NULL, NULL);
		if (ret) {
			dev_err(dev, "dmaengine pcm open failed with err %d\n", ret);
			return ret;
		}
		atomic_inc(&card_open_num);
	} else {
		/* Set HW params now that initialization is complete */
		snd_soc_set_runtime_hwparams(substream, &sunxi_pcm_capture_hardware);
		ret = snd_pcm_hw_constraint_integer(substream->runtime,
					    SNDRV_PCM_HW_PARAM_PERIODS);
		if (ret < 0)
			return ret;

		ret = snd_dmaengine_pcm_open(substream, NULL, NULL);
		if (ret) {
			dev_err(dev, "dmaengine pcm open failed with err %d\n", ret);
		}
	}
	return 0;
}

static int sunxi_pcm_close(struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_dmaengine_pcm_close(substream);

		atomic_dec(&card_open_num);
	} else {
		snd_dmaengine_pcm_close(substream);
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
		if (raw_flag > 1) {
			char* hdmihw_area = hdmiraw_dma_area + 2*frames_to_bytes(runtime, hwoff);
			hdmi_transfer_format_61937_to_60958((int*)hdmihw_area, (short*)hwbuf, frames_to_bytes(runtime, frames));
		}
#ifdef AUDIO_KARAOKE
		do_gettimeofday(&tv_cur);
		/*mixer capture buffer to the play output buffer*/
		if ((atomic_read(&cap_num) == 1) && ((tv_cur.tv_sec - tv_start.tv_sec) > 4)&&(raw_flag <= 1)) {
			if (frames_to_bytes(runtime, frames)>snd_pcm_lib_period_bytes(substream)) {
				audio_mixer_buffer(hwbuf, daudiocap_dma_area, hwbuf, snd_pcm_lib_period_bytes(substream));
				hwbuf = hwbuf+snd_pcm_lib_period_bytes(substream);
				audio_mixer_buffer(hwbuf, daudiocap_area, hwbuf, snd_pcm_lib_period_bytes(substream));
			} else {
				audio_mixer_buffer(hwbuf, daudiocap_area, hwbuf, frames_to_bytes(runtime, frames));
			}
		}
#endif
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		char *hwbuf = runtime->dma_area + frames_to_bytes(runtime, hwoff);
		if (copy_to_user(buf, hwbuf, frames_to_bytes(runtime, frames))) {
			return -EFAULT;
		}
	}

	return ret;
}

static struct snd_pcm_ops sunxi_pcm_ops = {
	.open			= sunxi_pcm_open,
	.close			= sunxi_pcm_close,
	.ioctl			= snd_pcm_lib_ioctl,
	.hw_params		= sunxi_pcm_hw_params,
	.hw_free		= sunxi_pcm_hw_free,
	.trigger		= sunxi_pcm_trigger,

//	#if defined CONFIG_SND_SUNXI_SOC_SUPPORT_AUDIO_RAW || defined AUDIO_KARAOKE
	.pointer		= snd_dmaengine_pcm_pointer_no_residue,
//	#else
//	.pointer		= snd_dmaengine_pcm_pointer,
//	#endif
	.mmap			= sunxi_pcm_mmap,
	.copy			= sunxi_pcm_copy,
};

static int sunxi_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf 			= &substream->dma_buffer;
	size_t size = 0;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		size = sunxi_pcm_hardware.buffer_bytes_max;
	} else {
		size = sunxi_pcm_capture_hardware.buffer_bytes_max;
	}
	buf->dev.type 		= SNDRV_DMA_TYPE_DEV;
	buf->dev.dev 		= pcm->card->dev;
	buf->private_data 	= NULL;
	buf->area 			= dma_alloc_writecombine(pcm->card->dev, size, &buf->addr, GFP_KERNEL);
	if (!buf->area) {
		return -ENOMEM;
	}
	buf->bytes = size;
	return 0;
}

static void sunxi_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	int stream 							= 0;
	struct snd_dma_buffer *buf 			= NULL;
	struct snd_pcm_substream *substream = NULL;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area) {
			continue;
		}

		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
}

static u64 sunxi_pcm_mask = DMA_BIT_MASK(32);

static int sunxi_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;
	struct snd_pcm *pcm 	= rtd->pcm;
	struct snd_card *card 	= rtd->card->snd_card;
atomic_set(&card_open_num, 0);
	if (!card->dev->dma_mask) {
		card->dev->dma_mask = &sunxi_pcm_mask;
	}
	if (!card->dev->coherent_dma_mask) {
		card->dev->coherent_dma_mask = 0xffffffff;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = sunxi_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret) {
			goto out;
		}
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = sunxi_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret) {
			goto out;
		}
	}
 out:
	return ret;
}

static struct snd_soc_platform_driver sunxi_soc_platform_hdmiaudio = {
		.ops        = &sunxi_pcm_ops,
		.pcm_new	= sunxi_pcm_new,
		.pcm_free	= sunxi_pcm_free_dma_buffers,
};

static int __init sunxi_hdmiaudio_pcm_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &sunxi_soc_platform_hdmiaudio);
}

static int __exit sunxi_hdmiaudio_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device sunxi_hdmiaudio_pcm_device = {
	.name = "sunxi-hdmiaudio-pcm-audio",
};

static struct platform_driver sunxi_hdmiaudio_pcm_driver = {
	.probe 	= sunxi_hdmiaudio_pcm_probe,
	.remove = __exit_p(sunxi_hdmiaudio_pcm_remove),
	.driver = {
		.name 	= "sunxi-hdmiaudio-pcm-audio",
		.owner 	= THIS_MODULE,
	},
};


static int __init sunxi_soc_platform_hdmiaudio_init(void)
{
	int err = 0;

	if ((err = platform_device_register(&sunxi_hdmiaudio_pcm_device)) < 0) {
		return err;
	}

	if ((err = platform_driver_register(&sunxi_hdmiaudio_pcm_driver)) < 0) {
		return err;
	}

	return 0;
}
module_init(sunxi_soc_platform_hdmiaudio_init);

static void __exit sunxi_soc_platform_hdmiaudio_exit(void)
{
	return platform_driver_unregister(&sunxi_hdmiaudio_pcm_driver);
}
module_exit(sunxi_soc_platform_hdmiaudio_exit);

MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUNXI HDMIAUDIO DMA module");
MODULE_LICENSE("GPL");

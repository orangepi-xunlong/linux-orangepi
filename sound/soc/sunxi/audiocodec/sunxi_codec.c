/*
 * sound\soc\sunxi\audiocodec\sunxi-codec.c
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <mach/hardware.h>
#include "sunxi_codecdma.h"

#ifdef CONFIG_ARCH_SUN8IW5
#include "sun8iw5_sndcodec.h"
#endif
#ifdef CONFIG_ARCH_SUN8IW9
#include "sun8iw9_sndcodec.h"
#endif

#define SUNXI_PCM_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)


#if defined CONFIG_ARCH_SUN8IW5 || defined CONFIG_ARCH_SUN8IW9
static u32 sample_resolution =16;
static struct sunxi_dma_params sunxi_pcm_pcm_stereo_out = {
	.name		= "audio_play",
	.dma_addr	= CODEC_BASSADDRESS + SUNXI_DA_TXFIFO,//send data address
};

static struct sunxi_dma_params sunxi_pcm_pcm_stereo_in = {
	.name   	= "audio_capture",
	.dma_addr	= CODEC_BASSADDRESS + SUNXI_DA_RXFIFO,//accept data address
};

static void sunxi_snd_txctrl(struct snd_pcm_substream *substream, int on)
{
	/*clear TX counter*/
	codec_wr_control(SUNXI_DA_TXCNT, 0xffffffff, TX_CNT, 0);
	/*flush TX FIFO*/
	codec_wr_control(SUNXI_DA_FCTL, 0x1, FTX, 1);
	if (on) {
		/* enable DMA DRQ mode for play */
		codec_wr_control(SUNXI_DA_INT, 0x1, TX_DRQ, 1);
	} else {
		/* DISBALE dma DRQ mode */
		codec_wr_control(SUNXI_DA_INT, 0x1, TX_DRQ, 0);
	}
}

static void sunxi_snd_rxctrl(struct snd_pcm_substream *substream, int on)
{
	/*clear RX counter*/
	codec_wr_control(SUNXI_DA_RXCNT, 0xffffffff, RX_CNT, 0);
	/*flush RX FIFO*/
	codec_wr_control(SUNXI_DA_FCTL, 0x1, FRX, 1);
	if (on) {
		/* enable DMA DRQ mode for record */
		codec_wr_control(SUNXI_DA_INT, 0x1, RX_DRQ, 1);
	} else {
		/* DISBALE dma DRQ mode */
		codec_wr_control(SUNXI_DA_INT, 0x1, RX_DRQ, 0);
	}
}

static int sunxi_i2s_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
			case SNDRV_PCM_TRIGGER_START:
			case SNDRV_PCM_TRIGGER_RESUME:
			case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
				/*enable i2s tx*/
				sunxi_snd_txctrl(substream, 1);
				return 0;
			case SNDRV_PCM_TRIGGER_SUSPEND:
			case SNDRV_PCM_TRIGGER_STOP:
			case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
				sunxi_snd_txctrl(substream, 0);
				return 0;
			default:
				return -EINVAL;
			}
	} else {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			sunxi_snd_rxctrl(substream, 1);
			return 0;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			sunxi_snd_rxctrl(substream, 0);
			return 0;
		default:
			pr_err("error:%s,%d\n", __func__, __LINE__);
			return -EINVAL;
		}
	}
	return 0;
}

static int sunxi_i2s_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	int rs_value  = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_data;

	switch (params_format(params))
	{
		case SNDRV_PCM_FORMAT_S16_LE:
			sample_resolution = 16;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			sample_resolution = 24;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			sample_resolution = 24;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			sample_resolution = 24;
			break;
		default:
			return -EINVAL;
	}

		/* sample rate */
	switch(sample_resolution)
	{
		case 16: rs_value = 0;
			break;
		case 20: rs_value = 1;
			break;
		case 24: rs_value = 2;
			break;
		default:
			return -EINVAL;
		//case 32: rs_value = 3;
		//	break;
	}
	codec_wr_control(SUNXI_DA_FAT0, 0x3, SR, rs_value);
#ifdef CONFIG_ARCH_SUN8IW5
	/*calculate word select bit*/
	switch (sample_resolution)
	{
		case 16: rs_value = 0x1;
			break;
		case 8: rs_value = 0x0;
			break;
		case 20: rs_value = 0x2;
			break;
		case 24: rs_value = 0x3;
			break;
		default:
			break;
	}
	codec_wr_control(SUNXI_AIF1CLK_CTRL, 0x3, AIF1_WORD_SIZ, rs_value);
#endif

	if(sample_resolution == 24)
		codec_wr_control(SUNXI_DA_FCTL, 0xf, RXOM, 0x1);
	else
		codec_wr_control(SUNXI_DA_FCTL, 0xf, RXOM, 0x5);

	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sunxi_pcm_pcm_stereo_out;
	else
		dma_data = &sunxi_pcm_pcm_stereo_in;


	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);

	return 0;
}
extern int sunxi_i2s_set_rate(int );

static int sunxi_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
				  int clk_id, unsigned int freq, int dir)
{
	/*config i2s clk*/
#ifdef CONFIG_ARCH_SUN8IW5
	sunxi_i2s_set_rate(freq);

	/*config aif1 clk from pll*/
	codec_wr_control(SUNXI_SYSCLK_CTL, 0x3, AIF1CLK_SRC, 0x3);

	/*config sys clk from aif1clk*/
	codec_wr_control(SUNXI_SYSCLK_CTL, 0x1, SYSCLK_SRC, 0x0);
#endif
	return 0;
}

static int sunxi_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int samplerate )
{
	u32 mclk_div = 0;
	u32 bclk_div = 0;
	int wss_value = 0;
	//int rs_value  = 0;
	u32 over_sample_rate = 0;
	u32 word_select_size = 32;
//	u32 sample_resolution =16;
#ifdef CONFIG_ARCH_SUN8IW5
	u32 bclk_lrck_div = 64;
	//int aif1_word_size = 16;
#endif
	/*mclk div calculate*/
	switch(samplerate)
	{
		case 8000:
		{
			over_sample_rate = 128;
			mclk_div = 24;
			break;
		}
		case 16000:
		{
			over_sample_rate = 128;
			mclk_div = 12;
			break;
		}
		case 32000:
		{
			over_sample_rate = 128;
			mclk_div = 6;
			break;
		}
		case 64000:
		{
			over_sample_rate = 384;
			mclk_div = 1;
			break;
		}
		case 11025:
		case 12000:
		{
			over_sample_rate = 128;
			mclk_div = 16;
			break;
		}
		case 22050:
		case 24000:
		{
			over_sample_rate = 128;
			mclk_div = 8;
			break;
		 }
		 case 44100:
		 case 48000:
		 {
			over_sample_rate = 128;
			mclk_div = 4;
			break;
		 }
		 case 88200:
		 case 96000:
		{
			 over_sample_rate = 128;
			 mclk_div = 2;
			 break;
		 }
		 case 176400:
		 case 192000:
		 {
			 over_sample_rate = 128;
			 mclk_div = 1;
			 break;
		}

	 }

	 /*bclk div caculate*/
	 bclk_div = over_sample_rate/(2*word_select_size);
	 /*calculate MCLK Divide Ratio*/
	switch(mclk_div)
	{
		case 1: mclk_div = 0;
				 break;
		case 2: mclk_div = 1;
				 break;
		case 4: mclk_div = 2;
				 break;
		case 6: mclk_div = 3;
				 break;
		case 8: mclk_div = 4;
				 break;
		case 12: mclk_div = 5;
				 break;
		case 16: mclk_div = 6;
				 break;
		case 24: mclk_div = 7;
				 break;
		case 32: mclk_div = 8;
				 break;
		case 48: mclk_div = 9;
				 break;
		case 64: mclk_div = 0xA;
				 break;
	}
	mclk_div &= 0xf;

	/*calculate BCLK Divide Ratio*/
	switch(bclk_div)
	 {
		case 2: bclk_div = 0;
				 break;
		case 4: bclk_div = 1;
				 break;
		case 6: bclk_div = 2;
				 break;
		case 8: bclk_div = 3;
				 break;
		case 12: bclk_div = 4;
				 break;
		case 16: bclk_div = 5;
				 break;
		case 32: bclk_div = 6;
				 break;
		case 64: bclk_div = 7;
				break;
	}
	 bclk_div &= 0x7;

	 /*confige mclk and bclk dividor register*/
	codec_wr_control(SUNXI_DA_CLKD, 0x7, BCLKDIV, bclk_div);
	codec_wr_control(SUNXI_DA_CLKD, 0xf, MCLKDIV, mclk_div);
	codec_wr_control(SUNXI_DA_CLKD, 0x1, 7, 1);

	/* word select size */
	switch(word_select_size)
	{
		case 16: wss_value = 0;
			break;
		case 20: wss_value = 1;
			break;
		case 24: wss_value = 2;
			break;
		case 32: wss_value = 3;
			break;
	}
	codec_wr_control(SUNXI_DA_FAT0, 0x3, WSS, wss_value);

#ifdef CONFIG_ARCH_SUN8IW5
/*********aif1 part************ */
	/*calculate bclk_lrck_div Ratio*/
	switch(bclk_lrck_div)
	{
		case 16: bclk_lrck_div = 0;
			break;
		case 32: bclk_lrck_div = 1;
			break;
		case 64: bclk_lrck_div = 2;
			break;
		case 128: bclk_lrck_div = 3;
			break;
		case 256: bclk_lrck_div = 4;
			break;
		default:
			break;
	}
	codec_wr_control(SUNXI_AIF1CLK_CTRL, 0x7, AIF1_LRCK_DIV, bclk_lrck_div);

#endif
	return 0;
}

static int sunxi_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
			       unsigned int fmt)
{
	/*i2s part*/
	/*SDO ON*/
//	codec_wr_control(SUNXI_DA_CTL, 0x1, SDO_EN, 1);
	/*master mode*/
	codec_wr_control(SUNXI_DA_CTL, 0x1, MS, 0);
	/*i2s mode*/
	codec_wr_control(SUNXI_DA_CTL, 0x1, PCM, 0);

	/* DAI signal inversions */
	codec_wr_control(SUNXI_DA_FAT0, 0x1, LRCP, 0);
	codec_wr_control(SUNXI_DA_FAT0, 0x1, BCP, 0);

	/*data format*/
	codec_wr_control(SUNXI_DA_FAT0, 0x3, FMT, 0);/*standard i2s fmt*/
	/*RX FIFO trigger level*/
	codec_wr_control(SUNXI_DA_FCTL, 0x7f, TXTL, 0x40);
	/*TX FIFO empty trigger level*/
	codec_wr_control(SUNXI_DA_FCTL, 0x1f, RXTL, 0x1f);

	//codec_wr_control(SUNXI_DA_FCTL, 0xf, RXOM, 0x5);
#ifdef CONFIG_ARCH_SUN8IW5
/**aif1 part**/
	/*aif1 slave*/
	codec_wr_control(SUNXI_AIF1CLK_CTRL, 0x1, AIF1_MSTR_MOD, 1);
	/*aif1 i2s mode*/
	codec_wr_control(SUNXI_AIF1CLK_CTRL, 0x3, AIF1_DATA_FMT, 0);

	/* DAI signal inversions */
	codec_wr_control(SUNXI_AIF1CLK_CTRL, 0x1, AIF1_BCLK_INV, 0);
	codec_wr_control(SUNXI_AIF1CLK_CTRL, 0x1, AIF1_LRCK_INV, 0);
#endif
	return 0;
}

static int sunxi_i2s_preapre(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	u32 reg_val = 0;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg_val = SUNXI_TXCHSEL_CHNUM(substream->runtime->channels);
		/*confige i2s ap tx channel */
		codec_wr_control(SUNXI_DA_TXCHSEL, 0x7, TX_CHSEL, reg_val);
		if(substream->runtime->channels == 1) {
			reg_val = 0x00;
		} else {
			reg_val = 0x10;
		}
		/*confige i2s ap tx channel mapping*/
		codec_wr_control(SUNXI_DA_TXCHMAP, 0x7, TX_CH0_MAP, reg_val);
		/*SDO ON*/
		codec_wr_control(SUNXI_DA_CTL, 0x1, SDO_EN, 1);
		/* I2S0 TX ENABLE */
		codec_wr_control(SUNXI_DA_CTL, 0x1, TXEN, 1);
	} else {
		reg_val = SUNXI_RXCHSEL_CHNUM(substream->runtime->channels);
		/*confige i2s ap rx channel */
		codec_wr_control(SUNXI_DA_RXCHSEL, 0x7, RX_CHSEL, reg_val);
		if(substream->runtime->channels == 1) {
			reg_val = 0x00;
		} else {
			reg_val = 0x10;
		}
		/*confige i2s ap rx channel mapping*/
		codec_wr_control(SUNXI_DA_RXCHMAP, 0x7, RX_CH0_MAP, reg_val);
		/* I2S0 RX ENABLE */
		codec_wr_control(SUNXI_DA_CTL, 0x1, RXEN, 1);
	}

	return 0;
}

static struct snd_soc_dai_ops sunxi_i2s_dai_ops = {
	.trigger 	= sunxi_i2s_trigger,
	.hw_params 	= sunxi_i2s_hw_params,
	.set_fmt 	= sunxi_i2s_set_fmt,
	.set_clkdiv = sunxi_i2s_set_clkdiv,
	.set_sysclk = sunxi_i2s_set_sysclk, 
	.prepare = sunxi_i2s_preapre,
};
#endif

static struct snd_soc_dai_driver sunxi_pcm_dai = {
	.playback 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_PCM_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_PCM_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	},

	#if defined CONFIG_ARCH_SUN8IW5 || defined CONFIG_ARCH_SUN8IW9
	.ops 		= &sunxi_i2s_dai_ops,
	#endif
};

static int __init sunxi_pcm_dev_probe(struct platform_device *pdev)
{
	int err = -1;

#ifdef CONFIG_ARCH_SUN8IW9
	/*global enable*/
	codec_wr_control(SUNXI_DA_CTL, 0x1, GEN, 0x1);
//	codec_wr_control(SUNXI_DA_CTL, 0x1, LOOP, 0x1);  //for loopback test
#endif
	err = snd_soc_register_dai(&pdev->dev, &sunxi_pcm_dai);	
	if (err) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
		return err;
	}

	return 0;
}

static int __exit sunxi_pcm_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dai(&pdev->dev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

/*data relating*/
static struct platform_device sunxi_pcm_device = {
	.name = "sunxi-codec",
	.id = -1,
};

/*method relating*/
static struct platform_driver sunxi_pcm_driver = {
	.probe = sunxi_pcm_dev_probe,
	.remove = __exit_p(sunxi_pcm_dev_remove),
	.driver = {
		.name = "sunxi-codec",
		.owner = THIS_MODULE,
	},
};

static int __init sunxi_pcm_init(void)
{
	int err = 0;

	if((err = platform_device_register(&sunxi_pcm_device)) < 0)
		return err;

	if ((err = platform_driver_register(&sunxi_pcm_driver)) < 0)
		return err;	

	return 0;
}
module_init(sunxi_pcm_init);

static void __exit sunxi_pcm_exit(void)
{
	platform_driver_unregister(&sunxi_pcm_driver);
}
module_exit(sunxi_pcm_exit);

/* Module information */
MODULE_AUTHOR("REUUIMLLA");
MODULE_DESCRIPTION("sunxi PCM SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pcm");


/*
 * sound\soc\sunxi\hdmiaudio\sunxi-hdmiaudio.c
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
#include <mach/hardware.h>
#include <asm/dma.h>

#include "sunxi-hdmipcm.h"
#if defined CONFIG_ARCH_SUN9I  || CONFIG_ARCH_SUN8IW6
struct sunxi_i2s1_info sunxi_i2s1;

static int regsave[8];
static int i2s1_select 			= 1;
static int over_sample_rate 	= 128;
static int sample_resolution 	= 16;
static int word_select_size 	= 32;

/*
*	sun9iw1: i2s1's pll source is PLL3
*/
static struct clk *i2s1_pllclk 		= NULL;
static struct clk *i2s1_moduleclk	= NULL;
#endif

static struct sunxi_dma_params sunxi_hdmiaudio_pcm_stereo_out = {
	.name 			= "hdmiaudio_play",
	#ifdef CONFIG_ARCH_SUN8IW1
	.dma_addr 		= SUNXI_HDMIBASE + SUNXI_HDMIAUDIO_TX,
	#endif
	#if defined CONFIG_ARCH_SUN9I || CONFIG_ARCH_SUN8IW6
	.dma_addr 		= SUNXI_I2S1BASE + SUNXI_I2S1TXFIFO,
	#endif
};
#if defined CONFIG_ARCH_SUN9I || CONFIG_ARCH_SUN8IW6
void sunxi_snd_txctrl_i2s1(struct snd_pcm_substream *substream, int on,int hub_on)
{
	u32 reg_val;

	reg_val = readl(sunxi_i2s1.regs + SUNXI_TXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUNXI_TXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sunxi_i2s1.regs + SUNXI_TXCHSEL);

	reg_val = readl(sunxi_i2s1.regs + SUNXI_TXCHMAP);
	reg_val = 0;
	if(substream->runtime->channels == 1) {
		reg_val = 0x76543200;
	} else if (substream->runtime->channels == 8) {
		reg_val = 0x54762310;
	} else {
		reg_val = 0x76543210;
	}
	writel(reg_val, sunxi_i2s1.regs + SUNXI_TXCHMAP);

	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val &= ~SUNXI_I2S1CTL_SDO3EN;
	reg_val &= ~SUNXI_I2S1CTL_SDO2EN;
       reg_val &= ~SUNXI_I2S1CTL_SDO1EN;
	reg_val &= ~SUNXI_I2S1CTL_SDO0EN;
	switch(substream->runtime->channels) {
		case 1:
		case 2:
			reg_val |= SUNXI_I2S1CTL_SDO0EN;
			break;
		case 3:
		case 4:
			reg_val |= SUNXI_I2S1CTL_SDO0EN | SUNXI_I2S1CTL_SDO1EN;
			break;
		case 5:
		case 6:
			reg_val |= SUNXI_I2S1CTL_SDO0EN | SUNXI_I2S1CTL_SDO1EN | SUNXI_I2S1CTL_SDO2EN;
			break;
		case 7:
		case 8:
			reg_val |= SUNXI_I2S1CTL_SDO0EN | SUNXI_I2S1CTL_SDO1EN | SUNXI_I2S1CTL_SDO2EN | SUNXI_I2S1CTL_SDO3EN;
			break;
		default:
			reg_val |= SUNXI_I2S1CTL_SDO0EN;
			break;
	}
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

	if (on) {
		/* I2S1 TX ENABLE */
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
		reg_val |= SUNXI_I2S1CTL_TXEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

		/* enable DMA DRQ mode for play */
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1INT);
		reg_val |= SUNXI_I2S1INT_TXDRQEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1INT);
	} else {
		/* I2S1 TX DISABLE */
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
		reg_val &= ~SUNXI_I2S1CTL_TXEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

		/* DISBALE dma DRQ mode */
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1INT);
		reg_val &= ~SUNXI_I2S1INT_TXDRQEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1INT);
	}

	if (hub_on) {
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	    	reg_val |= SUNXI_I2S1FCTL_HUBEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	} else {
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	    	reg_val &= ~SUNXI_I2S1FCTL_HUBEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	}
}
 EXPORT_SYMBOL(sunxi_snd_txctrl_i2s1);

 int sunxi_hdmiaudio_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;

	/*SDO ON*/
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val |= (SUNXI_I2S1CTL_SDO0EN | SUNXI_I2S1CTL_SDO1EN | SUNXI_I2S1CTL_SDO2EN | SUNXI_I2S1CTL_SDO3EN);

	if (!i2s1_select) {
		reg_val |= SUNXI_I2S1CTL_PCM;
	}
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

	/* master or slave selection */
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master, ap is slave*/
			reg_val |= SUNXI_I2S1CTL_MS;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave,ap is master*/
			reg_val &= ~SUNXI_I2S1CTL_MS;
			break;
		default:
			pr_err("unknwon master/slave format\n");
			return -EINVAL;
	}
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

	/* pcm or i2s1 mode selection */
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val1 = readl(sunxi_i2s1.regs + SUNXI_I2S1FAT0);
	reg_val1 &= ~SUNXI_I2S1FAT0_FMT_RVD;
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK){
		case SND_SOC_DAIFMT_I2S:        /* I2S1 mode */
			reg_val &= ~SUNXI_I2S1CTL_PCM;
			reg_val1 |= SUNXI_I2S1FAT0_FMT_I2S1;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val &= ~SUNXI_I2S1CTL_PCM;
			reg_val1 |= SUNXI_I2S1FAT0_FMT_RGT;
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val &= ~SUNXI_I2S1CTL_PCM;
			reg_val1 |= SUNXI_I2S1FAT0_FMT_LFT;
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
			reg_val |= SUNXI_I2S1CTL_PCM;
			reg_val1 &= ~SUNXI_I2S1FAT0_LRCP;
			break;
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			reg_val |= SUNXI_I2S1CTL_PCM;
			reg_val1 |= SUNXI_I2S1FAT0_LRCP;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);
	writel(reg_val1, sunxi_i2s1.regs + SUNXI_I2S1FAT0);

	/* DAI signal inversions */
	reg_val1 = readl(sunxi_i2s1.regs + SUNXI_I2S1FAT0);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
			reg_val1 &= ~SUNXI_I2S1FAT0_LRCP;
			reg_val1 &= ~SUNXI_I2S1FAT0_BCP;
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val1 |= SUNXI_I2S1FAT0_LRCP;
			reg_val1 &= ~SUNXI_I2S1FAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val1 &= ~SUNXI_I2S1FAT0_LRCP;
			reg_val1 |= SUNXI_I2S1FAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
			reg_val1 |= SUNXI_I2S1FAT0_LRCP;
			reg_val1 |= SUNXI_I2S1FAT0_BCP;
			break;
	}
	writel(reg_val1, sunxi_i2s1.regs + SUNXI_I2S1FAT0);

	/* set FIFO control register */
	reg_val = 1 & 0x3;
	reg_val |= (1 & 0x1)<<2;
	reg_val |= SUNXI_I2S1FCTL_TXTL(0x40);				/*TX FIFO empty trigger level*/
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	return 0;
}
 EXPORT_SYMBOL(sunxi_hdmiaudio_set_fmt);
#endif

int sunxi_hdmiaudio_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd 	= NULL;
	struct sunxi_dma_params *dma_data 	= NULL;
	u32 reg_val = 0;
#ifdef CONFIG_SND_SUNXI_SOC_SUPPORT_AUDIO_RAW
	int raw_flag = params_raw(params);
#else
	int raw_flag = hdmi_format;
#endif
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
	if (raw_flag > 1) {
		sample_resolution = 24;
	}
	if (!substream) {
		pr_err("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}

	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1FAT0);
	sunxi_i2s1.samp_res = sample_resolution;
	reg_val &= ~SUNXI_I2S1FAT0_SR_RVD;
	if(sunxi_i2s1.samp_res == 16)
		reg_val |= SUNXI_I2S1FAT0_SR_16BIT;
       else if(sunxi_i2s1.samp_res == 20)
		reg_val |= SUNXI_I2S1FAT0_SR_20BIT;
	else
		reg_val |= SUNXI_I2S1FAT0_SR_24BIT;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FAT0);

	rtd = substream->private_data;
	if (sample_resolution == 24) {
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1FCTL);
		reg_val &= ~(0x1<<2);
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data = &sunxi_hdmiaudio_pcm_stereo_out;
	} else {
		pr_err("error:hdmiaudio can't support capture:%s,line:%d\n", __func__, __LINE__);	
	}

	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	
	return 0;
}
 EXPORT_SYMBOL(sunxi_hdmiaudio_hw_params);
#if defined CONFIG_ARCH_SUN9I || CONFIG_ARCH_SUN8IW6
static int sunxi_i2s1_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			sunxi_snd_txctrl_i2s1(substream, 1,0);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			sunxi_snd_txctrl_i2s1(substream, 0,0);
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int sunxi_i2s1_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id,
                                 unsigned int freq, int i2s1_pcm_select)
{
	if (clk_set_rate(i2s1_pllclk, freq)) {
		pr_err("try to set the i2s1_pllclk failed!\n");
	}
	return 0;
}
int sunxi_hdmii2s_set_rate(int freq)
{
	if (clk_set_rate(i2s1_pllclk, freq)) {
		pr_err("try to set the i2s1_pllclk failed!\n");
	}
	return 0;
}
EXPORT_SYMBOL(sunxi_hdmii2s_set_rate);
int sunxi_i2s1_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int sample_rate)
{
	u32 reg_val;
	u32 mclk;
	u32 mclk_div = 0;
	u32 bclk_div = 0;

	mclk = over_sample_rate;
	if (i2s1_select) {
		/*mclk div calculate*/
		switch(sample_rate)
		{
			case 8000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 24;
								break;
					case 192:	mclk_div = 16;
								break;
					case 256:	mclk_div = 12;
								break;
					case 384:	mclk_div = 8;
								break;
					case 512:	mclk_div = 6;
								break;
					case 768:	mclk_div = 4;
								break;
				}
				break;
			}

			case 16000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 12;
								break;
					case 192:	mclk_div = 8;
								break;
					case 256:	mclk_div = 6;
								break;
					case 384:	mclk_div = 4;
								break;
					case 768:	mclk_div = 2;
								break;
				}
				break;
			}

			case 32000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 6;
								break;
					case 192:	mclk_div = 4;
								break;
					case 384:	mclk_div = 2;
								break;
					case 768:	mclk_div = 1;
								break;
				}
				break;
			}

			case 64000:
			{
				switch(mclk)
				{
					case 192:	mclk_div = 2;
								break;
					case 384:	mclk_div = 1;
								break;
				}
				break;
			}

			case 128000:
			{
				switch(mclk)
				{
					case 192:	mclk_div = 1;
								break;
				}
				break;
			}

			case 11025:
			case 12000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 16;
								break;
					case 256:	mclk_div = 8;
								break;
					case 512:	mclk_div = 4;
								break;
				}
				break;
			}

			case 22050:
			case 24000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 8;
								break;
					case 256:	mclk_div = 4;
								break;
					case 512:	mclk_div = 2;
								break;
				}
				break;
			}

			case 44100:
			case 48000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 4;
								break;
					case 256:	mclk_div = 2;
								break;
					case 512:	mclk_div = 1;
								break;
				}
				break;
			}

			case 88200:
			case 96000:
			{
				switch(mclk)
				{
					case 128:	mclk_div = 2;
								break;
					case 256:	mclk_div = 1;
								break;
				}
				break;
			}

			case 176400:
			case 192000:
			{
				mclk_div = 1;
				break;
			}

		}

		/*bclk div caculate*/
		bclk_div = mclk/(2*word_select_size);
	} else {
		mclk_div = 2;
		bclk_div = 6;
	}
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

	/*set mclk and bclk dividor register*/
	reg_val = mclk_div;
	reg_val |= (bclk_div<<4);
	reg_val |= (0x1<<7);
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CLKD);
	/* word select size */
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1FAT0);
	sunxi_i2s1.ws_size = word_select_size;
	reg_val &= ~SUNXI_I2S1FAT0_WSS_32BCLK;
	if(sunxi_i2s1.ws_size == 16)
		reg_val |= SUNXI_I2S1FAT0_WSS_16BCLK;
       else if(sunxi_i2s1.ws_size == 20)
		reg_val |= SUNXI_I2S1FAT0_WSS_20BCLK;
	else if(sunxi_i2s1.ws_size == 24)
		reg_val |= SUNXI_I2S1FAT0_WSS_24BCLK;
	else
		reg_val |= SUNXI_I2S1FAT0_WSS_32BCLK;

	sunxi_i2s1.samp_res = sample_resolution;
	reg_val &= ~SUNXI_I2S1FAT0_SR_RVD;
	if(sunxi_i2s1.samp_res == 16)
		reg_val |= SUNXI_I2S1FAT0_SR_16BIT;
       else if(sunxi_i2s1.samp_res == 20)
		reg_val |= SUNXI_I2S1FAT0_SR_20BIT;
	else
		reg_val |= SUNXI_I2S1FAT0_SR_24BIT;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FAT0);

	return 0;
}
 EXPORT_SYMBOL(sunxi_i2s1_set_clkdiv);
#endif

static int sunxi_hdmiaudio_dai_probe(struct snd_soc_dai *dai)
{			
	return 0;
}
static int sunxi_hdmiaudio_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}
#if defined CONFIG_ARCH_SUN9I || CONFIG_ARCH_SUN8IW6
static void i2s1regsave(void)
{
	regsave[0] = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	regsave[1] = readl(sunxi_i2s1.regs + SUNXI_I2S1FAT0);
	regsave[2] = readl(sunxi_i2s1.regs + SUNXI_I2S1FAT1);
	regsave[3] = readl(sunxi_i2s1.regs + SUNXI_I2S1FCTL) | (0x3<<24);
	regsave[4] = readl(sunxi_i2s1.regs + SUNXI_I2S1INT);
	regsave[5] = readl(sunxi_i2s1.regs + SUNXI_I2S1CLKD);
	regsave[6] = readl(sunxi_i2s1.regs + SUNXI_TXCHSEL);
	regsave[7] = readl(sunxi_i2s1.regs + SUNXI_TXCHMAP);
}

static void i2s1regrestore(void)
{
	writel(regsave[0], sunxi_i2s1.regs + SUNXI_I2S1CTL);
	writel(regsave[1], sunxi_i2s1.regs + SUNXI_I2S1FAT0);
	writel(regsave[2], sunxi_i2s1.regs + SUNXI_I2S1FAT1);
	writel(regsave[3], sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	writel(regsave[4], sunxi_i2s1.regs + SUNXI_I2S1INT);
	writel(regsave[5], sunxi_i2s1.regs + SUNXI_I2S1CLKD);
	writel(regsave[6], sunxi_i2s1.regs + SUNXI_TXCHSEL);
	writel(regsave[7], sunxi_i2s1.regs + SUNXI_TXCHMAP);
}

static int sunxi_hdmiaudio_suspend(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	pr_debug("[I2S1]Entered %s\n", __func__);

	/*Global disable Digital Audio Interface*/
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val &= ~SUNXI_I2S1CTL_GEN;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

	i2s1regsave();
	if ((NULL == i2s1_moduleclk) ||(IS_ERR(i2s1_moduleclk))) {
		pr_err("i2s1_moduleclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release the module clock*/
		clk_disable(i2s1_moduleclk);
	}
	return 0;
}

static int sunxi_hdmiaudio_resume(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	pr_debug("[I2S1]Entered %s\n", __func__);

	/*enable the module clock*/
	if (clk_enable(i2s1_moduleclk)) {
		pr_err("try to enable i2s1_moduleclk output failed!\n");
	}

	i2s1regrestore();

	/*Global Enable Digital Audio Interface*/
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val &= ~SUNXI_I2S1CTL_GEN;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

	return 0;
}
#endif

#define SUNXI_I2S_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sunxi_hdmiaudio_dai_ops = {
	.hw_params 		= sunxi_hdmiaudio_hw_params,
#if defined CONFIG_ARCH_SUN9I || CONFIG_ARCH_SUN8IW6
	.trigger 		= sunxi_i2s1_trigger,
	.set_fmt 		= sunxi_hdmiaudio_set_fmt,
	.set_clkdiv 	= sunxi_i2s1_set_clkdiv,
    .set_sysclk 	= sunxi_i2s1_set_sysclk,
#endif
};
static struct snd_soc_dai_driver sunxi_hdmiaudio_dai = {
	.probe 		= sunxi_hdmiaudio_dai_probe,
#if defined CONFIG_ARCH_SUN9I || CONFIG_ARCH_SUN8IW6
	.suspend 	= sunxi_hdmiaudio_suspend,
	.resume 	= sunxi_hdmiaudio_resume,
#endif
	.remove 	= sunxi_hdmiaudio_dai_remove,
	.playback 	= {
			.channels_min 	= 1,
			.channels_max 	= 8,
			.rates 			= SUNXI_I2S_RATES,
			.formats 		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE|SNDRV_PCM_FMTBIT_S32_LE,},
	.ops 		= &sunxi_hdmiaudio_dai_ops,
};		

static int __init sunxi_hdmiaudio_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	u32 reg_val;
#ifdef CONFIG_ARCH_SUN9I
	sunxi_i2s1.regs = ioremap(SUNXI_I2S1BASE, 0x100);
	if (sunxi_i2s1.regs == NULL) {
		return -ENXIO;
	}
	i2s1_pllclk = clk_get(NULL, "pll3");
	if ((!i2s1_pllclk)||(IS_ERR(i2s1_pllclk))) {
		pr_err("try to get i2s1_pllclk failed\n");
	}
	if (clk_prepare_enable(i2s1_pllclk)) {
		pr_err("enable i2s1_pllclk failed; \n");
	}
	/*i2s1 module clk*/
	i2s1_moduleclk = clk_get(NULL, "i2s1");
	if ((!i2s1_moduleclk)||(IS_ERR(i2s1_moduleclk))) {
		pr_err("try to get i2s1_moduleclk failed\n");
	}
	if (clk_set_parent(i2s1_moduleclk, i2s1_pllclk)) {
		pr_err("try to set parent of i2s1_moduleclk to i2s1_pll2ck failed! line = %d\n",__LINE__);
	}
	if (clk_set_rate(i2s1_moduleclk, 24576000)) {
		pr_err("set i2s1_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}
	if (clk_prepare_enable(i2s1_moduleclk)) {
		pr_err("open i2s1_moduleclk failed! line = %d\n", __LINE__);
	}

	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val &= ~SUNXI_I2S1CTL_GEN;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

#endif
#ifdef CONFIG_ARCH_SUN8IW6
	sunxi_i2s1.regs = ioremap(SUNXI_I2S1BASE, 0x100);
	if (sunxi_i2s1.regs == NULL) {
		return -ENXIO;
	}
	i2s1_pllclk = clk_get(NULL, "pll2");

	if ((!i2s1_pllclk)||(IS_ERR(i2s1_pllclk))) {
		pr_err("try to get i2s1_pllclk failed\n");
	}
	if (clk_prepare_enable(i2s1_pllclk)) {
		pr_err("enable i2s1_pllclk failed; \n");
	}
	/*i2s1 module clk*/
	i2s1_moduleclk = clk_get(NULL, "i2s2");
	if ((!i2s1_moduleclk)||(IS_ERR(i2s1_moduleclk))) {
		pr_err("try to get i2s1_moduleclk failed\n");
	}
	if (clk_set_parent(i2s1_moduleclk, i2s1_pllclk)) {
		pr_err("try to set parent of i2s1_moduleclk to i2s1_pll2ck failed! line = %d\n",__LINE__);
	}
	if (clk_set_rate(i2s1_moduleclk, 24576000)) {
		pr_err("set i2s1_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}
	if (clk_prepare_enable(i2s1_moduleclk)) {
		pr_err("open i2s1_moduleclk failed! line = %d\n", __LINE__);
	}

	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val &= ~SUNXI_I2S1CTL_GEN;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);
#endif

	if (!pdev) {
		pr_err("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}
	ret = snd_soc_register_dai(&pdev->dev, &sunxi_hdmiaudio_dai);

	return 0;
}

static int __exit sunxi_hdmiaudio_dev_remove(struct platform_device *pdev)
{
	if (!pdev) {
		pr_err("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}
#ifdef CONFIG_ARCH_SUN9I
	if ((NULL == i2s1_moduleclk) ||(IS_ERR(i2s1_moduleclk))) {
		pr_err("i2s1_moduleclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release the module clock*/
		clk_disable(i2s1_moduleclk);
	}
	if ((NULL == i2s1_pllclk) ||(IS_ERR(i2s1_pllclk))) {
		pr_err("i2s1_pllclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release pll3clk*/
		clk_put(i2s1_pllclk);
	}
#endif
	snd_soc_unregister_dai(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_device sunxi_hdmiaudio_device = {
	.name = "sunxi-hdmiaudio",
};

static struct platform_driver sunxi_hdmiaudio_driver = {
	.probe 	= sunxi_hdmiaudio_dev_probe,
	.remove = __exit_p(sunxi_hdmiaudio_dev_remove),
	.driver = {
		.name 	= "sunxi-hdmiaudio",
		.owner 	= THIS_MODULE,
	},	
};

static int __init sunxi_hdmiaudio_init(void)
{
	int err = 0;

	if ((err = platform_device_register(&sunxi_hdmiaudio_device))<0) {
		return err;
	}

	if ((err = platform_driver_register(&sunxi_hdmiaudio_driver)) < 0) {
		return err;
	}

	return 0;
}
module_init(sunxi_hdmiaudio_init);

static void __exit sunxi_hdmiaudio_exit(void)
{	
	platform_driver_unregister(&sunxi_hdmiaudio_driver);
}
module_exit(sunxi_hdmiaudio_exit);

/* Module information */
MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("sunxi hdmiaudio SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: sunxi-hdmiaudio");

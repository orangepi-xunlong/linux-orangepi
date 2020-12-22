/*
 * sound\soc\sunxi\daudio\sunxi-daudio.c
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
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <asm/dma.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/gpio.h>
#include "sunxi-daudiodma1.h"
#include "sunxi-daudio1.h"

struct sunxi_daudio_info1 sunxi_daudio1;

static int regsave[11];
static int daudio_used 			= 0;
static int daudio_select 		= 0;
static int mclk_fs 				= 0;
static int sample_resolution 	= 0;
static int pcm_lrck_period 		= 0;
static int pcm_lrckr_period 	= 0;
static int msb_lsb_first 		= 0;
static int sign_extend 			= 0;
static int slot_index 			= 0;
static int frame_width 			= 0;
static int tx_data_mode 		= 0;
static int rx_data_mode 		= 0;
static int slot_width_select 	= 0;
static bool  daudio1_loop_en 	= false;
#ifdef CONFIG_ARCH_SUN8IW7
static struct clk *daudio_pll			= NULL;
#endif
static struct clk *daudio_moduleclk	= NULL;

static struct sunxi_dma_params sunxi_daudio_pcm_stereo_out = {
	.name		= "daudio_play",
	.dma_addr	= SUNXI_DAUDIO1BASE + SUNXI_DAUDIOTXFIFO,/*send data address	*/
};

static struct sunxi_dma_params sunxi_daudio_pcm_stereo_in = {
	.name   	= "daudio_capture",
	.dma_addr	=SUNXI_DAUDIO1BASE + SUNXI_DAUDIORXFIFO,/*accept data address	*/
};

static void sunxi_snd_txctrl_daudio(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;
	/*flush TX FIFO*/
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
	reg_val |= SUNXI_DAUDIOFCTL_FTX;
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
	/*clear TX counter*/
	writel(0, sunxi_daudio1.regs + SUNXI_DAUDIOTXCNT);

	if (on) {
		/* enable DMA DRQ mode for play */
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOINT);
		reg_val |= SUNXI_DAUDIOINT_TXDRQEN;
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOINT);
	} else {
		/* DISBALE dma DRQ mode */
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOINT);
		reg_val &= ~SUNXI_DAUDIOINT_TXDRQEN;
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOINT);
	}
}

static void sunxi_snd_rxctrl_daudio(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;
	/*flush RX FIFO*/
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
	reg_val |= SUNXI_DAUDIOFCTL_FRX;	
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
	/*clear RX counter*/
	writel(0, sunxi_daudio1.regs + SUNXI_DAUDIORXCNT);

	if (on) {
		/* enable DMA DRQ mode for record */
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOINT);
		reg_val |= SUNXI_DAUDIOINT_RXDRQEN;
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOINT);
	} else {
		/* DISBALE dma DRQ mode */
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOINT);
		reg_val &= ~SUNXI_DAUDIOINT_RXDRQEN;
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOINT);
	}
}

static int sunxi_daudio_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;

	/* master or slave selection */
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master, ap is slave*/
			reg_val &= ~SUNXI_DAUDIOCTL_LRCKOUT;
			reg_val &= ~SUNXI_DAUDIOCTL_BCLKOUT;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave,ap is master*/
			reg_val |= SUNXI_DAUDIOCTL_LRCKOUT;
			reg_val |= SUNXI_DAUDIOCTL_BCLKOUT;
			break;
		default:
			pr_err("unknwon master/slave format\n");
			return -EINVAL;
	}
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	/* pcm or daudio mode selection */
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	reg_val1 = readl(sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHSEL);
	reg_val &= ~SUNXI_DAUDIOCTL_MODESEL;
	reg_val1 &= ~SUNXI_DAUDIOTXn_OFFSET;
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:        /* i2s mode */
			reg_val  |= (1<<4);
			reg_val1 |= (1<<12);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val  |= (2<<4);
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val  |= (1<<4);
			reg_val1  |= (0<<12);
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
			reg_val  |= (0<<4);
			reg_val1  |= (1<<12);
			break;
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			reg_val  |= (0<<4);
			reg_val1  |= (0<<12);
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	writel(reg_val1, sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHSEL);
	/* DAI signal inversions */
	reg_val1 = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
			reg_val1 &= ~SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			reg_val1 &= ~SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val1 |= SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			reg_val1 &= ~SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val1 &= ~SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			reg_val1 |= SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
			reg_val1 |= SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
			reg_val1 |= SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
			break;
	}
	writel(reg_val1, sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);
	return 0;
}

static int sunxi_daudio_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_data;

	u32 reg_val = 0;

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
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);
	sunxi_daudio1.samp_res = sample_resolution;
	reg_val &= ~SUNXI_DAUDIOFAT0_SR;
	if(sample_resolution == 16)
		reg_val |= (3<<4);
	else if(sample_resolution == 20)
		reg_val |= (4<<4);
	else
		reg_val |= (5<<4);
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);

	if (sample_resolution == 24) {
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
		reg_val &= ~(0x1<<2);
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
	} else {
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
		reg_val |= SUNXI_DAUDIOFCTL_TXIM;
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
	}

	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sunxi_daudio_pcm_stereo_out;
	else
		dma_data = &sunxi_daudio_pcm_stereo_in;

	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	return 0;
}

static int sunxi_daudio_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	u32 reg_val;
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sunxi_snd_rxctrl_daudio(substream, 1);
			} else {
				sunxi_snd_txctrl_daudio(substream, 1);
			}
			/*Global Enable Digital Audio Interface*/
			reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
			reg_val |= SUNXI_DAUDIOCTL_GEN;
			if (daudio1_loop_en) {
				reg_val |= SUNXI_DAUDIOCTL_LOOP; /*for test*/
			}
			writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sunxi_snd_rxctrl_daudio(substream, 0);
			} else {
			  	sunxi_snd_txctrl_daudio(substream, 0);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int sunxi_daudio_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id, 
                                 unsigned int freq, int daudio_pcm_select)
{
#ifdef CONFIG_ARCH_SUN8IW7
		if (clk_set_rate(daudio_pll, freq)) {
			pr_err("try to set the daudio_pll rate failed!\n");
		}
#endif
	daudio_select = daudio_pcm_select;

	return 0;
}

static int sunxi_daudio_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int sample_rate)
{
	u32 reg_val = 0;
	u32 mclk_div = 0;
	u32 bclk_div = 0;

	/*enable mclk*/
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCLKD);
	reg_val &= ~(0x1<<8);
	reg_val |= (0x1<<8);

	/*i2s mode*/
	if (daudio_select) {
		switch (sample_rate) {
			case 192000:
			case 96000:
			case 48000:
			case 32000:
			case 24000:
			case 12000:
			case 16000:
			case 8000:
				bclk_div = ((24576000/sample_rate)/(2*pcm_lrck_period));
				mclk_div = 1;
			break;
			default:
				bclk_div = ((22579200/sample_rate)/(2*pcm_lrck_period));
				mclk_div = 1;
			break;
		}
	} else {/*pcm mode*/
		bclk_div = ((24576000/sample_rate)/(pcm_lrck_period));
		mclk_div = 1;//((24576000/sample_rate)/mclk_fs);
	}

	switch(mclk_div)
	{
		case 1: mclk_div = 1;
				break;
		case 2: mclk_div = 2;
				break;
		case 4: mclk_div = 3;
				break;
		case 6: mclk_div = 4;
				break;
		case 8: mclk_div = 5;
				break;
		case 12: mclk_div = 6;
				 break;
		case 16: mclk_div = 7;
				 break;
		case 24: mclk_div = 8;
				 break;
		case 32: mclk_div = 9;
				 break;
		case 48: mclk_div = 10;
				 break;
		case 64: mclk_div = 11;
				 break;
		case 96: mclk_div = 12;
				 break;
		case 128: mclk_div = 13;
				 break;
		case 176: mclk_div = 14;
				 break;
		case 192: mclk_div = 15;
				 break;
	}

	reg_val &= ~(0xf<<0);
	reg_val |= mclk_div<<0;
	switch(bclk_div)
	{
		case 1: bclk_div = 1;
			break;
		case 2: bclk_div = 2;
			break;
		case 4: bclk_div = 3;
			break;
		case 6: bclk_div = 4;
			break;
		case 8: bclk_div = 5;
			break;
		case 12: bclk_div = 6;
			break;
		case 16: bclk_div = 7;
			break;
		case 24: bclk_div = 8;
			break;
		case 32: bclk_div = 9;
			break;
		case 48: bclk_div = 10;
			break;
		case 64: bclk_div = 11;
			break;
		case 96: bclk_div = 12;
			break;
		case 128: bclk_div = 13;
			break;
		case 176: bclk_div = 14;
			break;
		case 192:bclk_div = 15;
	}
	reg_val &= ~(0xf<<4);
	reg_val |= bclk_div<<4;
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCLKD);
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);
	reg_val &= ~(0x3ff<<20);
	reg_val &= ~(0x3ff<<8);
	reg_val |= (pcm_lrck_period-1)<<8;
	reg_val |= (pcm_lrckr_period-1)<<20;
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);
	/* slot size select */
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);
	sunxi_daudio1.slot_width = slot_width_select;
	reg_val &= ~SUNXI_DAUDIOFAT0_SW;
	if(sunxi_daudio1.slot_width == 16)
		reg_val |= (3<<0);
	else if(sunxi_daudio1.slot_width == 20) 
		reg_val |= (4<<0);
	else if(sunxi_daudio1.slot_width == 24)
		reg_val |= (5<<0);
	else if(sunxi_daudio1.slot_width == 32)
		reg_val |= (7<<0);
	else
		reg_val |= (1<<0);

	sunxi_daudio1.samp_res = sample_resolution;
	reg_val &= ~SUNXI_DAUDIOFAT0_SR;
	if(sunxi_daudio1.samp_res == 16)
		reg_val |= (3<<4);
	else if(sunxi_daudio1.samp_res == 20)
		reg_val |= (4<<4);
	else
		reg_val |= (5<<4);
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFAT1);

	sunxi_daudio1.frametype = frame_width;
	if(sunxi_daudio1.frametype)
		reg_val |= SUNXI_DAUDIOFAT0_LRCK_WIDTH;	

	sunxi_daudio1.pcm_start_slot = slot_index;
	reg_val |=(sunxi_daudio1.pcm_start_slot & 0x3)<<6;

	sunxi_daudio1.pcm_lsb_first = msb_lsb_first;
	reg_val |= sunxi_daudio1.pcm_lsb_first<<7;
	reg_val |= sunxi_daudio1.pcm_lsb_first<<6;

	sunxi_daudio1.signext = sign_extend;
	reg_val &= ~(0x3<<4);
	reg_val |= sunxi_daudio1.signext<<4;

	/*linear or u/a-law*/
	reg_val &= ~(0xf<<0);
	reg_val |= (tx_data_mode)<<2;
	reg_val |= (rx_data_mode)<<0;
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOFAT1);

	return 0;
}
static int sunxi_daudio_perpare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	/* play or record */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg_val = readl(sunxi_daudio1.regs + SUNXI_TXCHCFG);
		if (substream->runtime->channels == 1) {
			reg_val &= ~(0x7<<0);
			reg_val |= (0x0)<<0;
		} else {
			reg_val &= ~(0x7<<0);
			reg_val |= (0x1)<<0;
		}
		writel(reg_val, sunxi_daudio1.regs + SUNXI_TXCHCFG);
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHSEL);
		reg_val |= (0x1<<12);
		reg_val &= ~(0xff<<4);
		reg_val &= ~(0x7<<0);
		if (substream->runtime->channels == 1) {
			reg_val |= (0x3<<4);
			reg_val	|= (0x1<<0);
		} else {
			reg_val |= (0x3<<4);
			reg_val	|= (0x1<<0);
		}
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHSEL);
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHMAP);
		reg_val = 0;
		if(substream->runtime->channels == 1) {
			reg_val = 0x0;
		} else {
			reg_val = 0x10;
		}
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHMAP);

		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
		reg_val &= ~SUNXI_DAUDIOCTL_SDO3EN;
		reg_val &= ~SUNXI_DAUDIOCTL_SDO2EN;
		reg_val &= ~SUNXI_DAUDIOCTL_SDO1EN;
		reg_val &= ~SUNXI_DAUDIOCTL_SDO0EN;
		switch (substream->runtime->channels) {
			case 1:
			case 2:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN;
				break;
			case 3:
			case 4:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN | SUNXI_DAUDIOCTL_SDO1EN;
				break;
			case 5:
			case 6:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN | SUNXI_DAUDIOCTL_SDO1EN | SUNXI_DAUDIOCTL_SDO2EN;
				break;
			case 7:
			case 8:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN | SUNXI_DAUDIOCTL_SDO1EN | SUNXI_DAUDIOCTL_SDO2EN | SUNXI_DAUDIOCTL_SDO3EN;
				break;
			default:
				reg_val |= SUNXI_DAUDIOCTL_SDO0EN;
				break;
		}
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
		/*clear TX counter*/
		writel(0, sunxi_daudio1.regs + SUNXI_DAUDIOTXCNT);

		/* DAUDIO TX ENABLE */
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
		reg_val |= SUNXI_DAUDIOCTL_TXEN;
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	} else {
		reg_val = readl(sunxi_daudio1.regs + SUNXI_TXCHCFG);
		if (substream->runtime->channels == 1) {
			reg_val &= ~(0x7<<4);
			reg_val |= (0x0)<<4;
		} else {
			reg_val &= ~(0x7<<4);
			reg_val |= (0x1)<<4;
		}
		writel(reg_val, sunxi_daudio1.regs + SUNXI_TXCHCFG);

		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIORXCHSEL);
		reg_val |= (0x1<<12);
		if(substream->runtime->channels == 1) {
			reg_val	&= ~(0x1<<0);
		} else {
			reg_val	|= (0x1<<0);
		}
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIORXCHSEL);

		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIORXCHMAP);
		reg_val = 0;
		if (substream->runtime->channels == 1) {
			reg_val = 0x00;
		} else {
			reg_val = 0x10;
		}
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIORXCHMAP);
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
		reg_val |= SUNXI_DAUDIOFCTL_RXOM;
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);

		/*clear RX counter*/
		writel(0, sunxi_daudio1.regs + SUNXI_DAUDIORXCNT);

		/* DAUDIO RX ENABLE */
		reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
		reg_val |= SUNXI_DAUDIOCTL_RXEN;
		writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	}
	return 0;
}
static int sunxi_daudio_dai_probe(struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_daudio_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static void daudioregsave(void)
{
	regsave[0] = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	regsave[1] = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);
	regsave[2] = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFAT1);
	regsave[3] = readl(sunxi_daudio1.regs + SUNXI_DAUDIOFCTL) | (0x3<<24);
	regsave[4] = readl(sunxi_daudio1.regs + SUNXI_DAUDIOINT);
	regsave[5] = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCLKD);
	regsave[6] = readl(sunxi_daudio1.regs + SUNXI_TXCHCFG);
	regsave[7] = readl(sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHSEL);
	regsave[8] = readl(sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHMAP);
	regsave[9] = readl(sunxi_daudio1.regs + SUNXI_DAUDIORXCHSEL);
	regsave[10] = readl(sunxi_daudio1.regs + SUNXI_DAUDIORXCHMAP);
}

static void daudioregrestore(void)
{
	writel(regsave[0], sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	writel(regsave[1], sunxi_daudio1.regs + SUNXI_DAUDIOFAT0);
	writel(regsave[2], sunxi_daudio1.regs + SUNXI_DAUDIOFAT1);
	writel(regsave[3], sunxi_daudio1.regs + SUNXI_DAUDIOFCTL);
	writel(regsave[4], sunxi_daudio1.regs + SUNXI_DAUDIOINT);
	writel(regsave[5], sunxi_daudio1.regs + SUNXI_DAUDIOCLKD);

	writel(regsave[6], sunxi_daudio1.regs + SUNXI_TXCHCFG);
	writel(regsave[7], sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHSEL);
	writel(regsave[8], sunxi_daudio1.regs + SUNXI_DAUDIOTX0CHMAP);
	writel(regsave[9], sunxi_daudio1.regs + SUNXI_DAUDIORXCHSEL);
	writel(regsave[10], sunxi_daudio1.regs + SUNXI_DAUDIORXCHMAP);
}

static int sunxi_daudio_suspend(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	pr_debug("[DAUDIO]Entered %s\n", __func__);

	/*Global disable Digital Audio Interface*/
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	reg_val &= ~SUNXI_DAUDIOCTL_GEN;
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCTL);

	daudioregsave();

	if ((NULL == daudio_moduleclk) ||(IS_ERR(daudio_moduleclk))) {
		pr_err("daudio_moduleclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release the module clock*/
		clk_disable(daudio_moduleclk);
	}

	return 0;
}

static int sunxi_daudio_resume(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	pr_debug("[DAUDIO]Entered %s\n", __func__);

	/*enable the module clock*/
	if (clk_prepare_enable(daudio_moduleclk)) {
		pr_err("open daudio_moduleclk failed! line = %d\n", __LINE__);
	}
	daudioregrestore();

	/*Global Enable Digital Audio Interface*/
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	reg_val |= SUNXI_DAUDIOCTL_GEN;
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	
	return 0;
}

#define SUNXI_DAUDIO_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sunxi_daudio_dai_ops = {
	.trigger 	= sunxi_daudio_trigger,
	.hw_params 	= sunxi_daudio_hw_params,
	.set_fmt 	= sunxi_daudio_set_fmt,
	.set_clkdiv = sunxi_daudio_set_clkdiv,
	.set_sysclk = sunxi_daudio_set_sysclk,
	.prepare  =	sunxi_daudio_perpare,
};

static struct snd_soc_dai_driver sunxi_daudio_dai = {	
	.probe 		= sunxi_daudio_dai_probe,
	.suspend 	= sunxi_daudio_suspend,
	.resume 	= sunxi_daudio_resume,
	.remove 	= sunxi_daudio_dai_remove,
	.playback 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops 		= &sunxi_daudio_dai_ops,	
};

static struct pinctrl *daudio_pinctrl;
static int __init sunxi_daudio_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	int reg_val = 0;
	script_item_u val;
	script_item_value_type_e  type;

	sunxi_daudio1.regs = ioremap(SUNXI_DAUDIO1BASE, 0x100);
	if (sunxi_daudio1.regs == NULL) {
		return -ENXIO;
	}
	/*for A80 evb pin config*/
	daudio_pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR_OR_NULL(daudio_pinctrl)) {
		dev_warn(&pdev->dev,
			"pins are not configured from the driver\n");
	}

#ifdef CONFIG_ARCH_SUN8IW7
	daudio_pll = clk_get(NULL, "pll_audio");
	if ((!daudio_pll)||(IS_ERR(daudio_pll))) {
		pr_err("try to get daudio_pll failed\n");
	}
	if (clk_prepare_enable(daudio_pll)) {
		pr_err("enable daudio_pll2clk failed; \n");
	}
	/*daudio module clk*/
	daudio_moduleclk = clk_get(NULL, "i2s1");
	if ((!daudio_moduleclk)||(IS_ERR(daudio_moduleclk))) {
		pr_err("try to get daudio_moduleclk failed\n");
	}
	if (clk_set_parent(daudio_moduleclk, daudio_pll)) {
		pr_err("try to set parent of daudio_moduleclk to daudio_pll2ck failed! line = %d\n",__LINE__);
	}

	if (clk_set_rate(daudio_moduleclk, 24576000)) {
		pr_err("set daudio_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}

	if (clk_prepare_enable(daudio_moduleclk)) {
		pr_err("open daudio_moduleclk failed! line = %d\n", __LINE__);
	}
#endif
	reg_val = readl(sunxi_daudio1.regs + SUNXI_DAUDIOCTL);
	reg_val |= SUNXI_DAUDIOCTL_GEN;
	writel(reg_val, sunxi_daudio1.regs + SUNXI_DAUDIOCTL);

	type = script_get_item(TDM_NAME, "mclk_fs", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] mclk_fs type err!\n");
    }
	mclk_fs = val.val;

	type = script_get_item(TDM_NAME, "sample_resolution", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] sample_resolution type err!\n");
    }
	sample_resolution = val.val;

	type = script_get_item(TDM_NAME, "slot_width_select", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] slot_width_select type err!\n");
    }
	slot_width_select = val.val;

	type = script_get_item(TDM_NAME, "pcm_lrck_period", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] pcm_lrck_period type err!\n");
    }
	pcm_lrck_period = val.val;

	type = script_get_item(TDM_NAME, "pcm_lrckr_period", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] pcm_lrckr_period type err!\n");
    }
	pcm_lrckr_period = val.val;

	type = script_get_item(TDM_NAME, "msb_lsb_first", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] msb_lsb_first type err!\n");
    }
	msb_lsb_first = val.val;
	type = script_get_item(TDM_NAME, "sign_extend", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] sign_extend type err!\n");
    }
	sign_extend = val.val;
	type = script_get_item(TDM_NAME, "slot_index", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] slot_index type err!\n");
    }
	slot_index = val.val;
	type = script_get_item(TDM_NAME, "frame_width", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] frame_width type err!\n");
    }
	frame_width = val.val;
	type = script_get_item(TDM_NAME, "tx_data_mode", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] tx_data_mode type err!\n");
    }
	tx_data_mode = val.val;

	type = script_get_item(TDM_NAME, "rx_data_mode", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] rx_data_mode type err!\n");
    }
	rx_data_mode = val.val;

	ret = snd_soc_register_dai(&pdev->dev, &sunxi_daudio_dai);	
	if (ret) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
	}

	return 0;
}

static int __exit sunxi_daudio_dev_remove(struct platform_device *pdev)
{
	if (daudio_used) {
		daudio_used = 0;

		if ((NULL == daudio_moduleclk) ||(IS_ERR(daudio_moduleclk))) {
			pr_err("daudio_moduleclk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release the module clock*/
			clk_disable(daudio_moduleclk);
		}

		if ((NULL == daudio_pll) ||(IS_ERR(daudio_pll))) {
			pr_err("daudio_pll handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*reease pll clk*/
			clk_put(daudio_pll);
		}

		devm_pinctrl_put(daudio_pinctrl);
		snd_soc_unregister_dai(&pdev->dev);
		platform_set_drvdata(pdev, NULL);
	}
	return 0;
}

/*data relating*/
static struct platform_device sunxi_daudio_device = {
	#ifdef CONFIG_ARCH_SUN8IW7
	.name 	= "pcm1",
	#endif
	.id 	= PLATFORM_DEVID_NONE,
};

/*method relating*/
static struct platform_driver sunxi_daudio_driver = {
	.probe = sunxi_daudio_dev_probe,
	.remove = __exit_p(sunxi_daudio_dev_remove),
	.driver = {
		#ifdef CONFIG_ARCH_SUN8IW7
		.name 	= "pcm1",
		#endif
		.owner = THIS_MODULE,
	},
};

static int __init sunxi_daudio_init(void)
{	
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item(TDM_NAME, "daudio_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] type err!\n");
    }

	daudio_used = val.val;

	type = script_get_item(TDM_NAME, "daudio_select", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[DAUDIO] daudio_select:%s,line:%d type err!\n", __func__, __LINE__);
    }
	daudio_select = val.val;

 	if (daudio_used) {
		if((err = platform_device_register(&sunxi_daudio_device)) < 0)
			return err;
	
		if ((err = platform_driver_register(&sunxi_daudio_driver)) < 0)
			return err;	
	} else {
        pr_err("[DAUDIO]sunxi-daudio cannot find any using configuration for controllers, return directly!\n");
        return 0;
    }

	return 0;
}
module_init(sunxi_daudio_init);
module_param_named(daudio1_loop_en, daudio1_loop_en, bool, S_IRUGO | S_IWUSR);

static void __exit sunxi_daudio_exit(void)
{
	platform_driver_unregister(&sunxi_daudio_driver);
}
module_exit(sunxi_daudio_exit);

/* Module information */
MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("sunxi DAUDIO SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-daudio");


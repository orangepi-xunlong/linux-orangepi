/*
 * sound\soc\sunxi\i2s1\sunxi-i2s1.c
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
#include <linux/pinctrl/pinconf-sunxi.h>
#include "sunxi-i2s1dma.h"
#include "sunxi-i2s1.h"

struct sunxi_i2s1_info sunxi_i2s1;
static unsigned int pin_count = 0;
static script_item_u  *pin_i2s1_list;
static struct pinctrl *i2s1_pinctrl;

static int regsave[8];
static int i2s1_used 			= 0;
static int i2s1_select 			= 0;
static int over_sample_rate 	= 0;
static int sample_resolution 	= 0;
static int word_select_size 	= 0;
static int pcm_sync_period 		= 0;
static int msb_lsb_first 		= 0;
static int sign_extend 			= 0;
static int slot_index 			= 0;
static int slot_width 			= 0;
static int frame_width 			= 0;
static int tx_data_mode 		= 0;
static int rx_data_mode 		= 0;

//static struct clk *i2s1_apbclk               = NULL;
static struct clk *i2s1_pll2clk 		= NULL;
static struct clk *i2s1_pllx8 		= NULL;
static struct clk *i2s1_moduleclk	= NULL;

static struct sunxi_dma_params sunxi_i2s1_pcm_stereo_out = {
       .name           = "i2s1_play",
	.dma_addr	= SUNXI_I2S1BASE + SUNXI_I2S1TXFIFO,/*send data address	*/
};
static struct sunxi_dma_params sunxi_i2s1_pcm_stereo_in = {
	.name   	= "i2s1_capture",
	.dma_addr	=SUNXI_I2S1BASE + SUNXI_I2S1RXFIFO,/*accept data address	*/
};

static void sunxi_snd_txctrl_i2s1(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;

	reg_val = readl(sunxi_i2s1.regs + SUNXI_TXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUNXI_TXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sunxi_i2s1.regs + SUNXI_TXCHSEL);

	reg_val = readl(sunxi_i2s1.regs + SUNXI_TXCHMAP);
	reg_val = 0;
	if(substream->runtime->channels == 1) {
		reg_val = 0x3200;
	} else {
		reg_val = 0x3210;
	}
	writel(reg_val, sunxi_i2s1.regs + SUNXI_TXCHMAP);

	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val |= SUNXI_I2S1CTL_SDO0EN;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

	/*flush TX FIFO*/
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1FCTL);
       reg_val |= SUNXI_I2S1FCTL_FTX;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FCTL);

	/*clear TX counter*/
	writel(0, sunxi_i2s1.regs + SUNXI_I2S1TXCNT);

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
}

static void sunxi_snd_rxctrl_i2s1(struct snd_pcm_substream *substream, int on)
{
       u32 reg_val;

	reg_val = readl(sunxi_i2s1.regs + SUNXI_RXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUNXI_RXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sunxi_i2s1.regs + SUNXI_RXCHSEL);

	reg_val = readl(sunxi_i2s1.regs + SUNXI_RXCHMAP);
	reg_val = 0;
	if(substream->runtime->channels == 1) {
		reg_val = 0x00003200;
	} else {
		reg_val = 0x00003210;
	}
	writel(reg_val, sunxi_i2s1.regs + SUNXI_RXCHMAP);

	/*flush RX FIFO*/
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1FCTL);
       reg_val |= SUNXI_I2S1FCTL_FRX;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FCTL);

	/*clear RX counter*/
	writel(0, sunxi_i2s1.regs + SUNXI_I2S1RXCNT);

	if (on) {
		/* I2S1 RX ENABLE */
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
		reg_val |= SUNXI_I2S1CTL_RXEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

		/* enable DMA DRQ mode for record */
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1INT);
		reg_val |= SUNXI_I2S1INT_RXDRQEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1INT);
	} else {
		/* I2S1 RX DISABLE */
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
		reg_val &= ~SUNXI_I2S1CTL_RXEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

		/* DISBALE dma DRQ mode */
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1INT);
		reg_val &= ~SUNXI_I2S1INT_RXDRQEN;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1INT);
	}
}

static int sunxi_i2s1_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;

	/*SDO ON*/
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val |= (SUNXI_I2S1CTL_SDO0EN);
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
	reg_val |= SUNXI_I2S1FCTL_RXTL(0x1f);				/*RX FIFO trigger level*/
	reg_val |= SUNXI_I2S1FCTL_TXTL(0x40);				/*TX FIFO empty trigger level*/
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	return 0;
}

static int sunxi_i2s1_hw_params(struct snd_pcm_substream *substream,
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

	if (sample_resolution == 24) {
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1FCTL);
		reg_val &= ~(0x1<<2);
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	} else {
		reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1FCTL);
		reg_val |= SUNXI_I2S1FCTL_TXIM_MOD1;
		writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FCTL);
	}

	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sunxi_i2s1_pcm_stereo_out;
	else
		dma_data = &sunxi_i2s1_pcm_stereo_in;

	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	return 0;
}

static int sunxi_i2s1_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	u32 reg_val;
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sunxi_snd_rxctrl_i2s1(substream, 1);
			} else {
				sunxi_snd_txctrl_i2s1(substream, 1);
			}
			/*Global Enable Digital Audio Interface*/
			reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
			reg_val |= SUNXI_I2S1CTL_GEN;
			writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sunxi_snd_rxctrl_i2s1(substream, 0);
			} else {
			  	sunxi_snd_txctrl_i2s1(substream, 0);
			}
			/*Global disable Digital Audio Interface*/
//                     reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
//                     reg_val &= ~SUNXI_I2S1CTL_GEN;
//                     writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);
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
	if (clk_set_rate(i2s1_pll2clk, freq)) {
		pr_err("try to set the i2s1_pll2clk failed!\n");
	}

	i2s1_select = i2s1_pcm_select;

	return 0;
}

static int sunxi_i2s1_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int sample_rate)
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
		mclk_div = 4;
		bclk_div = 12;
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

	/* PCM REGISTER setup */
	sunxi_i2s1.pcm_txtype = tx_data_mode;
	sunxi_i2s1.pcm_rxtype = rx_data_mode;
	reg_val = sunxi_i2s1.pcm_txtype&0x3;
	reg_val |= sunxi_i2s1.pcm_rxtype<<2;

	sunxi_i2s1.pcm_sync_type = frame_width;
	if(sunxi_i2s1.pcm_sync_type)
               reg_val |= SUNXI_I2S1FAT1_SSYNC;

	sunxi_i2s1.pcm_sw = slot_width;
	if(sunxi_i2s1.pcm_sw == 16)
		reg_val |= SUNXI_I2S1FAT1_SW;

	sunxi_i2s1.pcm_start_slot = slot_index;
	reg_val |=(sunxi_i2s1.pcm_start_slot & 0x3)<<6;

	sunxi_i2s1.pcm_lsb_first = msb_lsb_first;
       reg_val |= sunxi_i2s1.pcm_lsb_first<<9;

	sunxi_i2s1.pcm_sync_period = pcm_sync_period;
	if(sunxi_i2s1.pcm_sync_period == 256)
		reg_val |= 0x4<<12;
	else if (sunxi_i2s1.pcm_sync_period == 128)
		reg_val |= 0x3<<12;
	else if (sunxi_i2s1.pcm_sync_period == 64)
		reg_val |= 0x2<<12;
	else if (sunxi_i2s1.pcm_sync_period == 32)
		reg_val |= 0x1<<12;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1FAT1);

I2S1_DBG("%s, line:%d, slot_index:%d\n", __func__, __LINE__, slot_index);
I2S1_DBG("%s, line:%d, slot_width:%d\n", __func__, __LINE__, slot_width);
I2S1_DBG("%s, line:%d, i2s1_select:%d\n", __func__, __LINE__, i2s1_select);
I2S1_DBG("%s, line:%d, frame_width:%d\n", __func__, __LINE__, frame_width);
I2S1_DBG("%s, line:%d, sign_extend:%d\n", __func__, __LINE__, sign_extend);
I2S1_DBG("%s, line:%d, tx_data_mode:%d\n", __func__, __LINE__, tx_data_mode);
I2S1_DBG("%s, line:%d, rx_data_mode:%d\n", __func__, __LINE__, rx_data_mode);
I2S1_DBG("%s, line:%d, msb_lsb_first:%d\n", __func__, __LINE__, msb_lsb_first);
I2S1_DBG("%s, line:%d, pcm_sync_period:%d\n", __func__, __LINE__, pcm_sync_period);
I2S1_DBG("%s, line:%d, word_select_size:%d\n", __func__, __LINE__, word_select_size);
I2S1_DBG("%s, line:%d, over_sample_rate:%d\n", __func__, __LINE__, over_sample_rate);
I2S1_DBG("%s, line:%d, sample_resolution:%d\n", __func__, __LINE__, sample_resolution);

	return 0;
}

static int sunxi_i2s1_dai_probe(struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_i2s1_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

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

static int sunxi_i2s1_suspend(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	u32 pin_index = 0;
	u32 config;
	struct gpio_config *pin_i2s1_cfg;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
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
//     if ((NULL == i2s1_apbclk) ||(IS_ERR(i2s1_apbclk))) {
//             pr_err("i2s1_apbclk handle is invalid, just return\n");
//             return -EFAULT;
//     } else {
//             clk_disable(i2s1_apbclk);
//     }
		devm_pinctrl_put(i2s1_pinctrl);
		/* request pin individually */
		for (pin_index = 0; pin_index < pin_count; pin_index++) {
			pin_i2s1_cfg = &(pin_i2s1_list[pin_index].gpio);
			/* valid pin of sunxi-pinctrl, config pin attributes individually.*/
			sunxi_gpio_to_name(pin_i2s1_cfg->gpio, pin_name);
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
	return 0;
}

static int sunxi_i2s1_resume(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	pr_debug("[I2S1]Entered %s\n", __func__);

	/*release the module clock*/
//     if (clk_enable(i2s1_apbclk)) {
//             pr_err("try to enable i2s1_apbclk output failed!\n");
//     }

	/*release the module clock*/
	if (clk_enable(i2s1_moduleclk)) {
		pr_err("try to enable i2s1_moduleclk output failed!\n");
	}

	i2s1regrestore();

	/*Global Enable Digital Audio Interface*/
	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val |= SUNXI_I2S1CTL_GEN;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);
	i2s1_pinctrl = devm_pinctrl_get_select_default(cpu_dai->dev);
	if (IS_ERR_OR_NULL(i2s1_pinctrl)) {
		dev_warn(cpu_dai->dev,
			"i2s1 pins are not configured from the driver\n");
	}

	return 0;
}

#define SUNXI_I2S1_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sunxi_i2s1_dai_ops = {
	.trigger 	= sunxi_i2s1_trigger,
	.hw_params 	= sunxi_i2s1_hw_params,
	.set_fmt 	= sunxi_i2s1_set_fmt,
	.set_clkdiv = sunxi_i2s1_set_clkdiv,
       .set_sysclk = sunxi_i2s1_set_sysclk,
};

static struct snd_soc_dai_driver sunxi_i2s1_dai = {
	.probe 		= sunxi_i2s1_dai_probe,
	.suspend 	= sunxi_i2s1_suspend,
	.resume 	= sunxi_i2s1_resume,
	.remove 	= sunxi_i2s1_dai_remove,
	.playback 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_I2S1_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_I2S1_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
       .ops            = &sunxi_i2s1_dai_ops,
};
static struct pinctrl *i2s1_pinctrl;

static int __init sunxi_i2s1_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	int reg_val = 0;
	script_item_u val;
	script_item_value_type_e  type;

	sunxi_i2s1.regs = ioremap(SUNXI_I2S1BASE, 0x100);
	if (sunxi_i2s1.regs == NULL) {
		return -ENXIO;
	}

	i2s1_pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR_OR_NULL(i2s1_pinctrl)) {
		dev_warn(&pdev->dev,
			"pins are not configured from the driver\n");
	}
	pin_count = script_get_pio_list ("i2s1", &pin_i2s1_list);
	if (pin_count == 0) {
		/* "daudio0" have no pin configuration */
		pr_err("i2s1 have no pin configuration\n");
	}
#if defined(CONFIG_ARCH_SUN8IW5) \
	|| defined(CONFIG_ARCH_SUN8IW6)
		i2s1_pllx8 = clk_get(NULL, "pll_audiox8");
#else
		i2s1_pllx8 = clk_get(NULL, "pll2x8");
#endif

	if ((!i2s1_pllx8)||(IS_ERR(i2s1_pllx8))) {
		pr_err("try to get i2s1_pllx8 failed\n");
	}
	if (clk_prepare_enable(i2s1_pllx8)) {
		pr_err("enable i2s1_pll2clk failed; \n");
	}

	/*i2s1 pll2clk*/
	i2s1_pll2clk = clk_get(NULL, "pll2");
	if ((!i2s1_pll2clk)||(IS_ERR(i2s1_pll2clk))) {
		pr_err("try to get i2s1_pll2clk failed\n");
	}
	if (clk_prepare_enable(i2s1_pll2clk)) {
		pr_err("enable i2s1_pll2clk failed; \n");
	}

	/*i2s1 module clk*/
	i2s1_moduleclk = clk_get(NULL, "i2s1");
	if ((!i2s1_moduleclk)||(IS_ERR(i2s1_moduleclk))) {
		pr_err("try to get i2s1_moduleclk failed\n");
	}

	if (clk_set_parent(i2s1_moduleclk, i2s1_pll2clk)) {
		pr_err("try to set parent of i2s1_moduleclk to i2s1_pll2ck failed! line = %d\n",__LINE__);
	}

	if (clk_set_rate(i2s1_moduleclk, 24576000/8)) {
		pr_err("set i2s1_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}

	if (clk_prepare_enable(i2s1_moduleclk)) {
		pr_err("open i2s1_moduleclk failed! line = %d\n", __LINE__);
	}

	reg_val = readl(sunxi_i2s1.regs + SUNXI_I2S1CTL);
	reg_val |= SUNXI_I2S1CTL_GEN;
	writel(reg_val, sunxi_i2s1.regs + SUNXI_I2S1CTL);

	type = script_get_item("i2s1", "over_sample_rate", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] over_sample_rate type err!\n");
    }
	over_sample_rate = val.val;

	type = script_get_item("i2s1", "sample_resolution", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] sample_resolution type err!\n");
    }
	sample_resolution = val.val;

	type = script_get_item("i2s1", "word_select_size", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] word_select_size type err!\n");
    }
	word_select_size = val.val;

	type = script_get_item("i2s1", "pcm_sync_period", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] pcm_sync_period type err!\n");
    }
	pcm_sync_period = val.val;

	type = script_get_item("i2s1", "msb_lsb_first", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] msb_lsb_first type err!\n");
    }
	msb_lsb_first = val.val;
	type = script_get_item("i2s1", "sign_extend", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] sign_extend type err!\n");
    }
	sign_extend = val.val;
	type = script_get_item("i2s1", "slot_index", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] slot_index type err!\n");
    }
	slot_index = val.val;
	type = script_get_item("i2s1", "slot_width", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] slot_width type err!\n");
    }
	slot_width = val.val;
	type = script_get_item("i2s1", "frame_width", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] frame_width type err!\n");
    }
	frame_width = val.val;
	type = script_get_item("i2s1", "tx_data_mode", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] tx_data_mode type err!\n");
    }
	tx_data_mode = val.val;

	type = script_get_item("i2s1", "rx_data_mode", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] rx_data_mode type err!\n");
    }
	rx_data_mode = val.val;


       ret = snd_soc_register_dai(&pdev->dev, &sunxi_i2s1_dai);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
	}

	return 0;
}

static int __exit sunxi_i2s1_dev_remove(struct platform_device *pdev)
{
	if (i2s1_used) {
		i2s1_used = 0;

		if ((NULL == i2s1_moduleclk) ||(IS_ERR(i2s1_moduleclk))) {
			pr_err("i2s1_moduleclk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release the module clock*/
			clk_disable(i2s1_moduleclk);
		}
		if ((NULL == i2s1_pllx8) ||(IS_ERR(i2s1_pllx8))) {
			pr_err("i2s1_pllx8 handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*reease pllx8clk*/
			clk_put(i2s1_pllx8);
		}
		if ((NULL == i2s1_pll2clk) ||(IS_ERR(i2s1_pll2clk))) {
			pr_err("i2s1_pll2clk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release pll2clk*/
			clk_put(i2s1_pll2clk);
		}

		devm_pinctrl_put(i2s1_pinctrl);
		snd_soc_unregister_dai(&pdev->dev);
		platform_set_drvdata(pdev, NULL);
	}
	return 0;
}

/*data relating*/
static struct platform_device sunxi_i2s1_device = {
	.name 	= "i2s1",
	.id 	= PLATFORM_DEVID_NONE,
};

/*method relating*/
static struct platform_driver sunxi_i2s1_driver = {
	.probe = sunxi_i2s1_dev_probe,
	.remove = __exit_p(sunxi_i2s1_dev_remove),
	.driver = {
		.name = "i2s1",
		.owner = THIS_MODULE,
	},
};

static int __init sunxi_i2s1_init(void)
{
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("i2s1", "i2s1_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] type err!\n");
    }

	i2s1_used = val.val;

	type = script_get_item("i2s1", "i2s1_select", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[I2S1] i2s1_select type err!\n");
    }
	i2s1_select = val.val;

 	if (i2s1_used) {
		if((err = platform_device_register(&sunxi_i2s1_device)) < 0)
			return err;

		if ((err = platform_driver_register(&sunxi_i2s1_driver)) < 0)
                               return err;
	} else {
        pr_err("[I2S1]sunxi-i2s1 cannot find any using configuration for controllers, return directly!\n");
        return 0;
    }
	return 0;
}
module_init(sunxi_i2s1_init);

static void __exit sunxi_i2s1_exit(void)
{
	platform_driver_unregister(&sunxi_i2s1_driver);
}
module_exit(sunxi_i2s1_exit);

/* Module information */
MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("sunxi I2S1 SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-i2s1");


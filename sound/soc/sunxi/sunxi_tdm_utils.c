/*
 * sound\soc\sunxi\sunxi_tdm_utils.c
 * (C) Copyright 2014-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 * Liu shaohua <liushaohua@allwinnertech.com>
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
#include <linux/gpio.h>
#include "sunxi_dma.h"
#include "sunxi_tdm_utils.h"
static bool  daudio0_loop_en 		= false;

static const struct _i2s_bclk_div_s i2s_bclk_div_s[] = {
	{16, 22579200, 44100, 1411200, 16, 7},
	{16, 22579200, 22050, 705600, 32, 9},
	{16, 22579200, 11025, 352800, 64, 11},
	{16, 22579200, 88200, 2822400, 8, 5},
	{16, 22579200, 176400, 5644800, 4, 3},

	{32, 22579200, 44100, 2822400, 8, 5},
	{32, 22579200, 22050, 1411200, 16, 7},
	{32, 22579200, 11025, 705600, 32, 9},
	{32, 22579200, 88200, 5644800, 4, 3},
	{32, 22579200, 176400, 11289600, 2, 2},

	{64, 22579200, 44100, 5644800, 4, 3},
	{64, 22579200, 22050, 2822400, 8, 5},
	{64, 22579200, 11025, 1411200, 16, 7},
	{64, 22579200, 88200, 11289600, 2, 2},
	{64, 22579200, 176400, 22579200, 1, 1},

	{128, 22579200, 44100, 11289600, 2, 2},
	{128, 22579200, 22050, 5644800, 4, 3},
	{128, 22579200, 11025, 2822400, 8, 5},
	{128, 22579200, 88200, 22579200, 1, 1},

	{256, 22579200, 44100, 22579200, 1, 1},
	{256, 22579200, 22050, 11289600, 2, 2},
	{256, 22579200, 11025, 5644800, 4, 3},

	{512, 22579200, 22050, 22579200, 1, 1},
	{512, 22579200, 11025, 11289600, 2, 2},

	{1024, 22579200, 11025, 22579200, 1, 1},

	{16, 24576000, 192000, 6144000, 4, 3},
	{16, 24576000, 96000, 3072000, 8, 5},
	{16, 24576000, 48000, 1536000, 16, 7},
	{16, 24576000, 32000, 1024000, 24, 8},
	{16, 24576000, 24000, 768000, 32, 9},
	{16, 24576000, 16000, 512000, 48, 10},
	{16, 24576000, 12000, 384000, 64, 11},
	{16, 24576000, 8000, 256000, 96, 12},

	{32, 24576000, 192000, 12288000, 2, 2},
	{32, 24576000, 96000, 6144000, 4, 3},
	{32, 24576000, 48000, 3072000, 8, 5},
	{32, 24576000, 32000, 2048000, 12, 6},
	{32, 24576000, 24000, 1536000, 16, 7},
	{32, 24576000, 16000, 1024000, 24, 8},
	{32, 24576000, 12000, 768000, 32, 9},
	{32, 24576000, 8000, 512000, 48, 10},

	{64, 24576000, 192000, 24576000, 1, 1},
	{64, 24576000, 96000, 12288000, 2, 2},
	{64, 24576000, 48000, 6144000, 4, 3},
	{64, 24576000, 32000, 4096000, 6, 4},
	{64, 24576000, 24000, 3072000, 8, 5},
	{64, 24576000, 16000, 2048000, 12, 6},
	{64, 24576000, 12000, 1536000, 16, 7},
	{64, 24576000, 8000, 1024000, 24, 8},

	{128, 24576000, 96000, 24576000, 1, 1},
	{128, 24576000, 48000, 12288000, 2, 2},
	{128, 24576000, 24000, 6144000, 4, 3},
	{128, 24576000, 16000, 4096000, 6, 4},
	{128, 24576000, 12000, 3072000, 8, 5},
	{128, 24576000, 8000, 2048000, 12, 6},

	{256, 24576000, 48000, 24576000, 1, 1},
	{256, 24576000, 24000, 12288000, 2, 2},
	{256, 24576000, 12000, 6144000, 4, 3},
	{256, 24576000, 8000, 4096000, 6, 4},

	{512, 24576000, 24000, 24576000, 1, 1},
	{512, 24576000, 12000, 12288000, 2, 2},

	{1024, 24576000, 12000, 24576000, 1, 1},
};

static const struct _pcm_bclk_div_s pcm_bclk_div_s[] = {
	{16, 22579200, 176400, 2822400, 8, 5},
	{16, 22579200, 88200, 1411200, 16, 7},
	{16, 22579200, 44100, 705600, 32, 9},
	{16, 22579200, 22050, 352800, 64, 11},
	{16, 22579200, 11025, 176400, 128, 13},

	{32, 22579200, 176400, 5644800, 4, 3},
	{32, 22579200, 88200, 2822400, 8, 5},
	{32, 22579200, 44100, 1411200, 16, 7},
	{32, 22579200, 22050, 705600, 32, 9},
	{32, 22579200, 11025, 352800, 64, 11},

	{64, 22579200, 176400, 11289600, 2, 2},
	{64, 22579200, 88200, 5644800, 4, 3},
	{64, 22579200, 44100, 2822400, 8, 5},
	{64, 22579200, 22050, 1411200, 16, 7},
	{64, 22579200, 11025, 705600, 32, 9},

	{128, 22579200, 176400, 22579200, 1, 1},
	{128, 22579200, 88200, 11289600, 2, 2},
	{128, 22579200, 44100, 5644800, 4, 3},
	{128, 22579200, 22050, 2822400, 8, 5},
	{128, 22579200, 11025, 1411200, 16, 7},

	{256, 22579200, 88200, 22579200, 1, 1},
	{256, 22579200, 44100, 11289600, 2, 2},
	{256, 22579200, 22050, 5644800, 4, 3},
	{256, 22579200, 11025, 2822400, 8, 5},

	{512, 22579200, 44100, 22579200, 1, 1},
	{512, 22579200, 22050, 11289600, 2, 2},
	{512, 22579200, 11025, 5644800, 4, 3},

	{1024, 22579200, 22050, 22579200, 1, 1},
	{1024, 22579200, 11025, 11289600, 2, 2},

	{16, 24576000, 192000, 3072000, 8, 5},
	{16, 24576000, 96000, 1536000, 16, 7},
	{16, 24576000, 48000, 768000, 32, 9},
	{16, 24576000, 32000, 512000, 48, 10},
	{16, 24576000, 24000, 384000, 64, 11},
	{16, 24576000, 16000, 256000, 96, 12},
	{16, 24576000, 12000, 192000, 128, 13},
	{16, 24576000, 8000, 128000, 192, 15},

	{32, 24576000, 192000, 6144000, 4, 3},
	{32, 24576000, 96000, 3072000, 8, 5},
	{32, 24576000, 48000, 1536000, 16, 7},
	{32, 24576000, 32000, 1024000, 24, 8},
	{32, 24576000, 24000, 768000, 32, 9},
	{32, 24576000, 16000, 512000, 48, 10},
	{32, 24576000, 12000, 384000, 64, 11},
	{32, 24576000, 8000, 256000, 96, 12},

	{64, 24576000, 192000, 12288000, 2, 2},
	{64, 24576000, 96000, 6144000, 4, 3},
	{64, 24576000, 48000, 3072000, 8, 5},
	{64, 24576000, 32000, 2048000, 12, 6},
	{64, 24576000, 24000, 1536000, 16, 7},
	{64, 24576000, 16000, 1024000, 24, 8},
	{64, 24576000, 12000, 768000, 32, 9},
	{64, 24576000, 8000, 512000, 48, 10},

	{128, 24576000, 192000, 24576000, 1, 1},
	{128, 24576000, 96000, 12288000, 2, 2},
	{128, 24576000, 48000, 6144000, 4, 3},
	{128, 24576000, 32000, 4096000, 6, 4},
	{128, 24576000, 24000, 3072000, 8, 5},
	{128, 24576000, 16000, 2048000, 12, 6},
	{128, 24576000, 12000, 1536000, 16, 7},
	{128, 24576000, 8000, 1024000, 24, 8},

	{256, 24576000, 96000, 24576000, 1, 1},
	{256, 24576000, 48000, 12288000, 2, 2},
	{256, 24576000, 24000, 6144000, 4, 3},
	{256, 24576000, 12000, 3072000, 8, 5},
	{256, 24576000, 8000, 2048000, 12, 6},

	{512, 24576000, 48000, 24576000, 1, 1},
	{512, 24576000, 24000, 12288000, 2, 2},
	{512, 24576000, 12000, 6144000, 4, 3},

	{1024, 24576000, 24000, 24576000, 1, 1},
	{1024, 24576000, 12000, 12288000, 2, 2},
};
int txctrl_tdm(int on,int hub_en,struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	/*flush TX FIFO*/
	reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
	reg_val |= SUNXI_DAUDIOFCTL_FTX;
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	/*clear TX counter*/
	writel(0, tdm->regs + SUNXI_DAUDIOTXCNT);

	if (on) {
		/* DAUDIO TX ENABLE */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
		reg_val |= SUNXI_DAUDIOCTL_TXEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
		/* enable DMA DRQ mode for play */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOINT);
		reg_val |= SUNXI_DAUDIOINT_TXDRQEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOINT);
	} else {
                if(tdm->others == 0){
		        /*DISABLE TXEN*/
		        reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
		        reg_val &= ~SUNXI_DAUDIOCTL_TXEN;
		        writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
                }
                /* DISBALE dma DRQ mode */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOINT);
		reg_val &= ~SUNXI_DAUDIOINT_TXDRQEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOINT);
	}
	if (hub_en) {
		reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
		reg_val |= SUNXI_DAUDIOFCTL_HUBEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	} else {
		reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
		reg_val &= ~SUNXI_DAUDIOFCTL_HUBEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	}

	return 0;
}
EXPORT_SYMBOL(txctrl_tdm);

int  rxctrl_tdm(int on,struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;

	if (on) {
		/* DAUDIO RX ENABLE */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
		reg_val |= SUNXI_DAUDIOCTL_RXEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
		/* enable DMA DRQ mode for record */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOINT);
		reg_val |= SUNXI_DAUDIOINT_RXDRQEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOINT);
	} else {
		/*DISABLE DAUDIO RX */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
		reg_val &= ~SUNXI_DAUDIOCTL_RXEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
		/* DISBALE dma DRQ mode */
		reg_val = readl(tdm->regs + SUNXI_DAUDIOINT);
		reg_val &= ~SUNXI_DAUDIOINT_RXDRQEN;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOINT);

		/*flush RX FIFO*/
		reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
		reg_val |= SUNXI_DAUDIOFCTL_FRX;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
		/*clear RX counter*/
		writel(0, tdm->regs + SUNXI_DAUDIORXCNT);
		#ifdef CONFIG_ARCH_SUN50I
		/*
		*	while flush RX FIFO, must read RXFIFO DATA three times.
		*	or it wouldn't flush RX FIFO clean; and it will let record data channel reverse!
		*/
		{
			int i = 0;
			for (i = 0; i < 9;i++) {
				reg_val = readl(tdm->regs + SUNXI_DAUDIORXFIFO);
			}
		}
		#endif
	}

	return 0;
}
EXPORT_SYMBOL(rxctrl_tdm);

int tdm_set_fmt(unsigned int fmt,struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;
	u32 reg_val2 = 0;
	struct sunxi_tdm_info *tdm = NULL;

	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	/* master or slave selection */
	reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
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
	writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
	/* pcm or tdm mode selection */
	reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
	reg_val1 = readl(tdm->regs + SUNXI_DAUDIOTX0CHSEL);
	reg_val2 = readl(tdm->regs + SUNXI_DAUDIORXCHSEL);
	reg_val &= ~SUNXI_DAUDIOCTL_MODESEL(3);
	reg_val1 &= ~SUNXI_DAUDIOTXn_OFFSET(3);
	reg_val2 &= ~(SUNXI_DAUDIORXCHSEL_RXOFFSET(3));
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:        /* i2s mode */
			reg_val  |=  SUNXI_DAUDIOCTL_MODESEL(1);
			reg_val1 |= SUNXI_DAUDIOTXn_OFFSET(1);
			reg_val2 |= SUNXI_DAUDIORXCHSEL_RXOFFSET(1);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val  |=  SUNXI_DAUDIOCTL_MODESEL(2);
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val  |=  SUNXI_DAUDIOCTL_MODESEL(1);
			reg_val1 |= SUNXI_DAUDIOTXn_OFFSET(0);
			reg_val2 |= SUNXI_DAUDIORXCHSEL_RXOFFSET(0);
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
			reg_val  |=  SUNXI_DAUDIOCTL_MODESEL(0);
			reg_val1 |= SUNXI_DAUDIOTXn_OFFSET(1);
			reg_val2 |= SUNXI_DAUDIORXCHSEL_RXOFFSET(1);
			break;
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			reg_val  |=  SUNXI_DAUDIOCTL_MODESEL(0);
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
	writel(reg_val1, tdm->regs + SUNXI_DAUDIOTX0CHSEL);
	writel(reg_val2, tdm->regs + SUNXI_DAUDIORXCHSEL);
	/* DAI signal inversions */
	reg_val1 = readl(tdm->regs + SUNXI_DAUDIOFAT0);
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:        /* i2s mode */
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			switch(fmt & SND_SOC_DAIFMT_INV_MASK) {
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
				default:
					pr_warn("not support format!:%d\n", __LINE__);
					break;
			}
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			switch(fmt & SND_SOC_DAIFMT_INV_MASK) {
				case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
					reg_val1 |= SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
					reg_val1 &= ~SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
					break;
				case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
					reg_val1 &= ~SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
					reg_val1 &= ~SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
					break;
				case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
					reg_val1 |= SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
					reg_val1 |= SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
					break;
				case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
					reg_val1 &= ~SUNXI_DAUDIOFAT0_LRCK_POLAYITY;
					reg_val1 |= SUNXI_DAUDIOFAT0_BCLK_POLAYITY;
					break;
				default:
					pr_warn("not support format!:%d\n", __LINE__);
					break;
			}
			break;
		default:
			pr_warn("not support format!:%d\n", __LINE__);
			break;
	}
	writel(reg_val1, tdm->regs + SUNXI_DAUDIOFAT0);

	return 0;
}
EXPORT_SYMBOL(tdm_set_fmt);

int tdm_hw_params(struct snd_pcm_hw_params *params,struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val = 0;
	//u32 sample_resolution = 0;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;

	switch (params_format(params))
	{
	case SNDRV_PCM_FORMAT_S16_LE:
		tdm->samp_res = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		tdm->samp_res = 24;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		tdm->samp_res = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		tdm->samp_res = 24;
		break;
	default:
		return -EINVAL;
	}
	reg_val = readl(tdm->regs + SUNXI_DAUDIOFAT0);
	reg_val &= ~SUNXI_DAUDIOFAT0_SR(7);
	if(tdm->samp_res == 16)
		reg_val |= SUNXI_DAUDIOFAT0_SR(3);
	else if(tdm->samp_res == 20)
		reg_val |= SUNXI_DAUDIOFAT0_SR(4);
	else
		reg_val |= SUNXI_DAUDIOFAT0_SR(5);
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFAT0);

	reg_val = readl(tdm->regs + SUNXI_DAUDIOFCTL);
	reg_val &= ~SUNXI_DAUDIOFCTL_TXIM;
	reg_val &= ~SUNXI_DAUDIOFCTL_RXOM(3);
	if (tdm->samp_res == 24) {
		reg_val &= ~SUNXI_DAUDIOFCTL_TXIM;
		reg_val &= ~SUNXI_DAUDIOFCTL_RXOM(3);
	} else {
		reg_val |= SUNXI_DAUDIOFCTL_TXIM;
		reg_val |= SUNXI_DAUDIOFCTL_RXOM(1);
	}
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFCTL);
	return 0;
}
EXPORT_SYMBOL(tdm_hw_params);

int tdm_trigger(struct snd_pcm_substream *substream,int cmd, struct sunxi_tdm_info *sunxi_tdm)
{
	s32 ret = 0;
	u32 reg_val = 0;
	struct sunxi_tdm_info *tdm = NULL;

	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;

	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				rxctrl_tdm(1,tdm);
			} else {
				txctrl_tdm(1,0,tdm);
			}
		if (daudio0_loop_en) {
			reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
			reg_val |= SUNXI_DAUDIOCTL_LOOP; /*for test*/
			writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
		}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				rxctrl_tdm(0,tdm);
			} else {
			  	txctrl_tdm(0,0,tdm);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}
EXPORT_SYMBOL(tdm_trigger);
module_param_named(daudio0_loop_en, daudio0_loop_en, bool, S_IRUGO | S_IWUSR);

int tdm_set_sysclk(unsigned int freq,struct sunxi_tdm_info *sunxi_tdm)
{
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	if (clk_set_rate(tdm->tdm_pllclk, freq)) {
		pr_err("try to set the tdm_pll2clk failed!\n");
	}
	return 0;
}
EXPORT_SYMBOL(tdm_set_sysclk);

int tdm_set_clkdiv(int sample_rate,struct sunxi_tdm_info *sunxi_tdm)
{
	int i;
	u32 reg_val = 0;
	u32 mclk_div = 1;
	u32 bclk_div_regval = 1;
	struct sunxi_tdm_info *tdm = NULL;

	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;
	/*enable mclk*/
	reg_val = readl(tdm->regs + SUNXI_DAUDIOCLKD);
	reg_val &= ~(SUNXI_DAUDIOCLKD_MCLKOEN);
	reg_val |= SUNXI_DAUDIOCLKD_MCLKOEN;
	reg_val &= ~SUNXI_DAUDIOCLKD_MCLKDIV(0xf);
	reg_val &= ~SUNXI_DAUDIOCLKD_BCLKDIV(0xf);
	reg_val |= SUNXI_DAUDIOCLKD_MCLKDIV(mclk_div)<<0;
	/*i2s mode*/
	if (tdm->tdm_config) {
		for (i = 0; i < ARRAY_SIZE(i2s_bclk_div_s); i++) {
			if ((i2s_bclk_div_s[i].i2s_lrck_period == tdm->pcm_lrck_period)\
					&&(i2s_bclk_div_s[i].sample_rate == sample_rate)) {
				bclk_div_regval = i2s_bclk_div_s[i].bclk_div_regval;
				break;
			}
		}
	} else {
		/*pcm mode*/
		for (i = 0; i < ARRAY_SIZE(pcm_bclk_div_s); i++) {
			if ((pcm_bclk_div_s[i].pcm_lrck_period == tdm->pcm_lrck_period)\
					&&(pcm_bclk_div_s[i].sample_rate == sample_rate)) {
				bclk_div_regval = pcm_bclk_div_s[i].bclk_div_regval;
				break;
			}
		}
	}
	reg_val |= SUNXI_DAUDIOCLKD_BCLKDIV(bclk_div_regval);
	writel(reg_val, tdm->regs + SUNXI_DAUDIOCLKD);

	reg_val = readl(tdm->regs + SUNXI_DAUDIOFAT0);
	reg_val &= ~SUNXI_DAUDIOFAT0_LRCKR_PERIOD(0x3ff);
	reg_val &= ~SUNXI_DAUDIOFAT0_LRCK_PERIOD(0x3ff);
	reg_val |= SUNXI_DAUDIOFAT0_LRCK_PERIOD(tdm->pcm_lrck_period-1);
	reg_val |= SUNXI_DAUDIOFAT0_LRCKR_PERIOD(tdm->pcm_lrckr_period-1);
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFAT0);

	reg_val = readl(tdm->regs + SUNXI_DAUDIOFAT0);
	reg_val &= ~SUNXI_DAUDIOFAT0_SW(7);
	if(tdm->slot_width_select == 16)
		reg_val |= SUNXI_DAUDIOFAT0_SW(3);
	else if(tdm->slot_width_select == 20)
		reg_val |= SUNXI_DAUDIOFAT0_SW(4);
	else if(tdm->slot_width_select == 24)
		reg_val |= SUNXI_DAUDIOFAT0_SW(5);
	else if(tdm->slot_width_select == 28)
		reg_val |= SUNXI_DAUDIOFAT0_SW(6);
	else
		reg_val |= SUNXI_DAUDIOFAT0_SW(7);

	/*pcm mode
	*	(Only apply in PCM mode) LRCK width
	*	0: LRCK = 1 BCLK width(short frame)
	*	1: LRCK = 2 BCLK width(long frame)
	*/
	if(tdm->frametype)
		reg_val |= SUNXI_DAUDIOFAT0_LRCK_WIDTH;
	else
		reg_val &= ~SUNXI_DAUDIOFAT0_LRCK_WIDTH;
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFAT0);

	reg_val = readl(tdm->regs + SUNXI_DAUDIOFAT1);
	if (tdm->pcm_lsb_first) {
		reg_val |= SUNXI_DAUDIOFAT1_TX_MLS;
		reg_val |= SUNXI_DAUDIOFAT1_RX_MLS;
	} else {
		reg_val &= ~SUNXI_DAUDIOFAT1_TX_MLS;
		reg_val &= ~SUNXI_DAUDIOFAT1_RX_MLS;
	}
	/*linear or u/a-law*/
	reg_val &= ~SUNXI_DAUDIOFAT1_RX_PDM(3);
	reg_val &= ~SUNXI_DAUDIOFAT1_TX_PDM(3);
	reg_val |= SUNXI_DAUDIOFAT1_TX_PDM(tdm->tx_data_mode);
	reg_val |= SUNXI_DAUDIOFAT1_RX_PDM(tdm->rx_data_mode);
	writel(reg_val, tdm->regs + SUNXI_DAUDIOFAT1);

	return 0;
}
EXPORT_SYMBOL(tdm_set_clkdiv);

int tdm_perpare(struct snd_pcm_substream *substream,
					struct sunxi_tdm_info *sunxi_tdm)
{
	u32 reg_val;
	struct sunxi_tdm_info *tdm = NULL;
	if (sunxi_tdm != NULL)
		tdm = sunxi_tdm;
	else
		return -1;

	/*i2s mode*/
	if (tdm->tdm_config) {
		if ((tdm->slot_width_select * substream->runtime->channels) > 2*tdm->pcm_lrck_period) {
			pr_warning("not support slot_width,pcm_lrck_period. slot_width*channel should <= 2*pcm_lrck_period!\n");
		}
	} else {
		/*pcm mode*/
		if ((tdm->slot_width_select * substream->runtime->channels) > tdm->pcm_lrck_period) {
			pr_warning("not support slot_width,pcm_lrck_period. slot_width*channel should <= pcm_lrck_period!\n");
		}
	}
	/* play or record */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg_val = readl(tdm->regs + SUNXI_CHCFG);
		reg_val &= ~SUNXI_TXCHCFG_TX_SLOT_NUM(CH_MAX-1);
		reg_val |= SUNXI_TXCHCFG_TX_SLOT_NUM(substream->runtime->channels-1);
		writel(reg_val, tdm->regs + SUNXI_CHCFG);

		reg_val = readl(tdm->regs + SUNXI_DAUDIOTX0CHSEL);
		reg_val &= ~SUNXI_DAUDIOTXn_CHEN(CHEN_MASK);
		reg_val &= ~SUNXI_DAUDIOTXn_CHSEL(CHSEL_MASK);
		reg_val |= SUNXI_DAUDIOTXn_CHEN((CHEN_MASK>>(CH_MAX-substream->runtime->channels)));
		reg_val |= SUNXI_DAUDIOTXn_CHSEL(substream->runtime->channels-1);
		writel(reg_val, tdm->regs + SUNXI_DAUDIOTX0CHSEL);

#ifdef CONFIG_ARCH_SUN8IW10
		reg_val = SUNXI_TXCHANMAP0_DEFAULT;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOTX0CHMAP0);
		reg_val = SUNXI_TXCHANMAP1_DEFAULT;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOTX0CHMAP1);
#else
		reg_val = SUNXI_TXCHANMAP_DEFAULT;
		writel(reg_val, tdm->regs + SUNXI_DAUDIOTX0CHMAP);
#endif
		/*clear TX counter*/
		writel(0, tdm->regs + SUNXI_DAUDIOTXCNT);
	} else {
		reg_val = readl(tdm->regs + SUNXI_CHCFG);
		reg_val &= ~SUNXI_TXCHCFG_RX_SLOT_NUM(CH_MAX-1);
		reg_val |= SUNXI_TXCHCFG_RX_SLOT_NUM(substream->runtime->channels-1);
		writel(reg_val, tdm->regs + SUNXI_CHCFG);

		reg_val = readl(tdm->regs + SUNXI_DAUDIORXCHSEL);
		reg_val &= ~SUNXI_DAUDIORXCHSEL_RXCHSET(CHSEL_MASK);
		reg_val |= SUNXI_DAUDIORXCHSEL_RXCHSET(substream->runtime->channels-1);
		writel(reg_val, tdm->regs + SUNXI_DAUDIORXCHSEL);

#ifdef CONFIG_ARCH_SUN8IW10
		reg_val = SUNXI_RXCHANMAP0_DEFAULT;
		writel(reg_val, tdm->regs + SUNXI_DAUDIORXCHMAP0);
		reg_val = SUNXI_RXCHANMAP1_DEFAULT;
		writel(reg_val, tdm->regs + SUNXI_DAUDIORXCHMAP1);
#else
		reg_val = SUNXI_RXCHANMAP_DEFAULT;
		writel(reg_val, tdm->regs + SUNXI_DAUDIORXCHMAP);
#endif
		/*clear RX counter*/
		writel(0, tdm->regs + SUNXI_DAUDIORXCNT);
	}
	return 0;
}
EXPORT_SYMBOL(tdm_perpare);

int tdm_global_enable(struct sunxi_tdm_info *sunxi_tdm,bool on)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;
	struct sunxi_tdm_info *tdm = NULL;

	if (sunxi_tdm != NULL) {
		tdm = sunxi_tdm;
	} else {
		return -1;
	}

	reg_val = readl(tdm->regs + SUNXI_DAUDIOCTL);
	reg_val1 = readl(tdm->regs + SUNXI_DAUDIOCLKD);
	if (!on) {
		reg_val &= ~SUNXI_DAUDIOCTL_GEN;
		reg_val &= ~SUNXI_DAUDIOCTL_SDO0EN;
		reg_val1 &= ~SUNXI_DAUDIOCLKD_MCLKOEN;
	} else {
		reg_val |= SUNXI_DAUDIOCTL_GEN;
		reg_val |= SUNXI_DAUDIOCTL_SDO0EN;
		reg_val1 |= SUNXI_DAUDIOCLKD_MCLKOEN;
		reg_val1 |= SUNXI_DAUDIOCLKD_MCLKDIV(1);
	}
	writel(reg_val, tdm->regs + SUNXI_DAUDIOCTL);
	writel(reg_val1, tdm->regs + SUNXI_DAUDIOCLKD);

	return 0;
}
EXPORT_SYMBOL(tdm_global_enable);
MODULE_LICENSE("GPL");


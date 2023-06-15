/*
 * sound\soc\sunxi\snd_sunxi_spdif.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_spdif.h"

#define HLOG		"SPDIF"
#define DRV_NAME	"sunxi-snd-plat-spdif"

/* for reg debug */
#define REG_LABEL(constant)	{#constant, constant, 0}
#define REG_LABEL_END		{NULL, 0, 0}

struct reg_label {
	const char *name;
	const unsigned int address;
	unsigned int value;
};
static struct reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_SPDIF_CTL),
	REG_LABEL(SUNXI_SPDIF_TXCFG),
	REG_LABEL(SUNXI_SPDIF_RXCFG),
	REG_LABEL(SUNXI_SPDIF_INT_STA),
	/* REG_LABEL(SUNXI_SPDIF_RXFIFO), */
	REG_LABEL(SUNXI_SPDIF_FIFO_CTL),
	REG_LABEL(SUNXI_SPDIF_FIFO_STA),
	REG_LABEL(SUNXI_SPDIF_INT),
	/* REG_LABEL(SUNXI_SPDIF_TXFIFO), */
	REG_LABEL(SUNXI_SPDIF_TXCNT),
	REG_LABEL(SUNXI_SPDIF_RXCNT),
	REG_LABEL(SUNXI_SPDIF_TXCH_STA0),
	REG_LABEL(SUNXI_SPDIF_TXCH_STA1),
	REG_LABEL(SUNXI_SPDIF_RXCH_STA0),
	REG_LABEL(SUNXI_SPDIF_RXCH_STA1),
	REG_LABEL_END,
};

static struct regmap_config sunxi_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_SPDIF_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

/* origin freq convert */
static const struct sample_rate sample_rate_orig[] = {
	{22050,  0xB},
	{24000,  0x9},
	{32000,  0xC},
	{44100,  0xF},
	{48000,  0xD},
	{88200,  0x7},
	{96000,  0x5},
	{176400, 0x3},
	{192000, 0x1},
};

static const struct sample_rate sample_rate_freq[] = {
	{22050,  0x4},
	{24000,  0x6},
	{32000,  0x3},
	{44100,  0x0},
	{48000,  0x2},
	{88200,  0x8},
	{96000,  0xA},
	{176400, 0xC},
	{192000, 0xE},
};

static int snd_sunxi_save_reg(struct regmap *regmap, struct reg_label *sunxi_reg_labels);
static int snd_sunxi_echo_reg(struct regmap *regmap, struct reg_label *sunxi_reg_labels);
static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_spdif_clk_info *clk_info);
static void snd_sunxi_clk_exit(struct platform_device *pdev, struct sunxi_spdif_clk_info *clk_info);
static int snd_sunxi_clk_enable(struct platform_device *pdev,
				struct sunxi_spdif_clk_info *clk_info);
static void snd_sunxi_clk_disable(struct platform_device *pdev,
				  struct sunxi_spdif_clk_info *clk_info);

static int sunxi_spdif_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_dai_set_dma_data(dai, substream, &spdif_info->playback_dma_param);
	} else {
		snd_soc_dai_set_dma_data(dai, substream, &spdif_info->capture_dma_param);
	}

	return 0;
}

static void sunxi_spdif_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG(HLOG, "\n");
}

static int sunxi_spdif_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
				   unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_clk_info *clk_info = &spdif_info->clk_info;

	SND_LOG_DEBUG(HLOG, "stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out / clk_info->pll_fs);

	if (clk_set_rate(clk_info->clk_parent, freq_in)) {
		SND_LOG_ERR(HLOG, "clk_parent set rate failed\n");
		return -EINVAL;
	}

	if (clk_set_rate(clk_info->clk_spdif, freq_out / clk_info->pll_fs)) {
		SND_LOG_ERR(HLOG, "clk spdif set rate failed\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_spdif_dai_set_clkdiv(struct snd_soc_dai *dai, int clk_id, int clk_div)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_clk_info *clk_info = &spdif_info->clk_info;
	struct regmap *regmap = spdif_info->mem_info.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	clk_div = clk_div / clk_info->pll_fs;
	clk_div = clk_div >> 7;	/* fs = spdif_clk/[(div+1)*64*2] */
	regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG,
			   0x1F << TXCFG_CLK_DIV_RATIO, (clk_div - 1) << TXCFG_CLK_DIV_RATIO);

	return 0;
}

static int sunxi_spdif_dai_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = spdif_info->mem_info.regmap;
	unsigned int tx_input_mode = 0;
	unsigned int rx_output_mode = 0;
	unsigned int origin_freq_bit = 0, sample_freq_bit = 0;
	unsigned int reg_temp;
	unsigned int i;

	SND_LOG_DEBUG(HLOG, "\n");

	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		reg_temp = 0;
		tx_input_mode = 1;
		rx_output_mode = 3;
		break;
	case	SNDRV_PCM_FORMAT_S20_3LE:
		reg_temp = 1;
		tx_input_mode = 0;
		rx_output_mode = 0;
		break;
		/* only for the compatible of tinyalsa */
	case	SNDRV_PCM_FORMAT_S24_LE:
	case	SNDRV_PCM_FORMAT_S32_LE:
		reg_temp = 2;
		tx_input_mode = 0;
		rx_output_mode = 0;
		break;
	default:
		SND_LOG_ERR(HLOG, "params_format[%d] error!\n", params_format(params));
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(sample_rate_orig); i++) {
		if (params_rate(params) == sample_rate_orig[i].samplerate) {
			origin_freq_bit = sample_rate_orig[i].rate_bit;
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(sample_rate_freq); i++) {
		if (params_rate(params) == sample_rate_freq[i].samplerate) {
			sample_freq_bit = sample_rate_freq[i].rate_bit;
			break;
		}
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG,
				   3 << TXCFG_SAMPLE_BIT, reg_temp << TXCFG_SAMPLE_BIT);

		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   1 << FIFO_CTL_TXIM, tx_input_mode << FIFO_CTL_TXIM);

		if (params_channels(params) == 1) {
			regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG,
					   1 << TXCFG_SINGLE_MOD, 1 << TXCFG_SINGLE_MOD);
		} else {
			regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG,
					   1 << TXCFG_SINGLE_MOD, 0 << TXCFG_SINGLE_MOD);
		}

		/* samplerate conversion */
		regmap_update_bits(regmap, SUNXI_SPDIF_TXCH_STA0,
				   0xF << TXCHSTA0_SAMFREQ,
				   sample_freq_bit << TXCHSTA0_SAMFREQ);
		regmap_update_bits(regmap, SUNXI_SPDIF_TXCH_STA1,
				   0xF << TXCHSTA1_ORISAMFREQ,
				   origin_freq_bit << TXCHSTA1_ORISAMFREQ);
		switch (reg_temp) {
		case	0:
			regmap_update_bits(regmap, SUNXI_SPDIF_TXCH_STA1,
					   0xF << TXCHSTA1_MAXWORDLEN, 2 << TXCHSTA1_MAXWORDLEN);
			break;
		case	1:
			regmap_update_bits(regmap, SUNXI_SPDIF_TXCH_STA1,
					   0xF << TXCHSTA1_MAXWORDLEN, 0xC << TXCHSTA1_MAXWORDLEN);
			break;
		case	2:
			regmap_update_bits(regmap, SUNXI_SPDIF_TXCH_STA1,
					   0xF << TXCHSTA1_MAXWORDLEN, 0xB << TXCHSTA1_MAXWORDLEN);
			break;
		default:
			SND_LOG_ERR(HLOG, "unexpection error\n");
			return -EINVAL;
		}
	} else {
		/*
		 * FIXME, not sync as spec says, just test 16bit & 24bit,
		 * using 3 working ok
		 */
		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   0x3 << FIFO_CTL_RXOM,
				   rx_output_mode << FIFO_CTL_RXOM);
		regmap_update_bits(regmap, SUNXI_SPDIF_RXCH_STA0,
				   0xF<<RXCHSTA0_SAMFREQ,
				   sample_freq_bit << RXCHSTA0_SAMFREQ);
		regmap_update_bits(regmap, SUNXI_SPDIF_RXCH_STA1,
				   0xF<<RXCHSTA1_ORISAMFREQ,
				   origin_freq_bit << RXCHSTA1_ORISAMFREQ);

		switch (reg_temp) {
		case	0:
			regmap_update_bits(regmap, SUNXI_SPDIF_RXCH_STA1,
					   0xF << RXCHSTA1_MAXWORDLEN, 2 << RXCHSTA1_MAXWORDLEN);
			break;
		case	1:
			regmap_update_bits(regmap, SUNXI_SPDIF_RXCH_STA1,
					   0xF << RXCHSTA1_MAXWORDLEN, 0xC << RXCHSTA1_MAXWORDLEN);
			break;
		case	2:
			regmap_update_bits(regmap, SUNXI_SPDIF_RXCH_STA1,
					   0xF << RXCHSTA1_MAXWORDLEN, 0xB << RXCHSTA1_MAXWORDLEN);
			break;
		default:
			SND_LOG_ERR(HLOG, "unexpection error\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int sunxi_spdif_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = spdif_info->mem_info.regmap;
	unsigned int reg_val;

	SND_LOG_DEBUG(HLOG, "\n");

	/*as you need to clean up TX or RX FIFO , need to turn off GEN bit*/
	regmap_update_bits(regmap, SUNXI_SPDIF_CTL, 1 << CTL_GEN_EN, 0 << CTL_GEN_EN);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
		regmap_write(regmap, SUNXI_SPDIF_TXCNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   1 << FIFO_CTL_FRX, 1 << FIFO_CTL_FRX);
		regmap_write(regmap, SUNXI_SPDIF_RXCNT, 0);
	}

	/* clear all interrupt status */
	regmap_read(regmap, SUNXI_SPDIF_INT_STA, &reg_val);
	regmap_write(regmap, SUNXI_SPDIF_INT_STA, reg_val);

	/* need reset */
	regmap_update_bits(regmap, SUNXI_SPDIF_CTL,
			   1 << CTL_RESET | 1 << CTL_GEN_EN, 1 << CTL_RESET | 1 << CTL_GEN_EN);

	return 0;
}

static void sunxi_spdif_txctrl_enable(struct sunxi_spdif_info *spdif_info, int enable)
{
	struct regmap *regmap = spdif_info->mem_info.regmap;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG, 1 << TXCFG_TXEN, 1 << TXCFG_TXEN);
		regmap_update_bits(regmap, SUNXI_SPDIF_INT, 1 << INT_TXDRQEN, 1 << INT_TXDRQEN);
	} else {
		regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG, 1 << TXCFG_TXEN, 0 << TXCFG_TXEN);
		regmap_update_bits(regmap, SUNXI_SPDIF_INT, 1 << INT_TXDRQEN, 0 << INT_TXDRQEN);
	}
}

static void sunxi_spdif_rxctrl_enable(struct sunxi_spdif_info *spdif_info, int enable)
{
	struct regmap *regmap = spdif_info->mem_info.regmap;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_SPDIF_RXCFG,
				   1 << RXCFG_CHSR_CP, 1 << RXCFG_CHSR_CP);
		regmap_update_bits(regmap, SUNXI_SPDIF_INT, 1 << INT_RXDRQEN, 1 << INT_RXDRQEN);
		regmap_update_bits(regmap, SUNXI_SPDIF_RXCFG, 1 << RXCFG_RXEN, 1 << RXCFG_RXEN);
	} else {
		regmap_update_bits(regmap, SUNXI_SPDIF_RXCFG, 1 << RXCFG_RXEN, 0 << RXCFG_RXEN);
		regmap_update_bits(regmap, SUNXI_SPDIF_INT, 1 << INT_RXDRQEN, 0 << INT_RXDRQEN);
	}
}

static int sunxi_spdif_dai_trigger(struct snd_pcm_substream *substream,
				   int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_dts_info *dts_info = &spdif_info->dts_info;

	SND_LOG_DEBUG(HLOG, "cmd -> %d\n", cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_spdif_txctrl_enable(spdif_info, 1);
		} else {
			/* rxsync en -> capture route -> drq en -> rxsync start */
			sunxi_spdif_rxctrl_enable(spdif_info, 1);
			if (dts_info->rx_sync_en && dts_info->rx_sync_ctl)
				sunxi_rx_sync_control(dts_info->rx_sync_domain,
						      dts_info->rx_sync_id, true);
		}
	break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_spdif_txctrl_enable(spdif_info, 0);
		} else {
			sunxi_spdif_rxctrl_enable(spdif_info, 0);
			if (dts_info->rx_sync_en && dts_info->rx_sync_ctl)
				sunxi_rx_sync_control(dts_info->rx_sync_domain,
						      dts_info->rx_sync_id, false);
		}
	break;
	default:
		SND_LOG_ERR(HLOG, "unsupport cmd\n");
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_spdif_dai_ops = {
	/* call by machine */
	.set_pll	= sunxi_spdif_dai_set_pll,	// set pllclk
	.set_clkdiv	= sunxi_spdif_dai_set_clkdiv,	// set clk div
	/* call by asoc */
	.startup	= sunxi_spdif_dai_startup,
	.hw_params	= sunxi_spdif_dai_hw_params,	// set hardware params
	.prepare	= sunxi_spdif_dai_prepare,	// clean irq and fifo
	.trigger	= sunxi_spdif_dai_trigger,	// set drq
	.shutdown	= sunxi_spdif_dai_shutdown,
};

static void sunxi_spdif_init(struct sunxi_spdif_info *spdif_info)
{
	struct regmap *regmap = spdif_info->mem_info.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	/* FIFO CTL register default setting */
	regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
			   CTL_TXTL_MASK << FIFO_CTL_TXTL, CTL_TXTL_DEFAULT << FIFO_CTL_TXTL);
	regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
			   CTL_RXTL_MASK << FIFO_CTL_RXTL, CTL_RXTL_DEFAULT << FIFO_CTL_RXTL);

	/* send tx channel status info */
	regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG,
			   1 << TXCFG_CHAN_STA_EN, 1 << TXCFG_CHAN_STA_EN);

	regmap_write(regmap, SUNXI_SPDIF_TXCH_STA0, 0x2 << TXCHSTA0_CHNUM);
	regmap_write(regmap, SUNXI_SPDIF_RXCH_STA0, 0x2 << RXCHSTA0_CHNUM);

	regmap_update_bits(regmap, SUNXI_SPDIF_CTL, 1 << CTL_GEN_EN, 1 << CTL_GEN_EN);
}

static int sunxi_spdif_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai,
				  &spdif_info->playback_dma_param, &spdif_info->capture_dma_param);

	sunxi_spdif_init(spdif_info);

	return 0;
}

static int sunxi_spdif_dai_remove(struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG(HLOG, "\n");

	return 0;
}

static struct snd_soc_dai_driver sunxi_spdif_dai = {
	.probe		= sunxi_spdif_dai_probe,
	.remove		= sunxi_spdif_dai_remove,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_spdif_dai_ops,
};

/*******************************************************************************
 * *** sound card & component function source ***
 * @0 sound card probe
 * @1 component function kcontrol register
 ******************************************************************************/
static int sunxi_get_tx_hub_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *spdif_info = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = spdif_info->mem_info.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_SPDIF_FIFO_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << FIFO_CTL_HUBEN)) ? 1 : 0);

	return 0;
}

static int sunxi_set_tx_hub_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *spdif_info = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = spdif_info->mem_info.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG,
				   0x1 << TXCFG_TXEN, 0x0 << TXCFG_TXEN);
		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   0x1 << FIFO_CTL_HUBEN, 0x0 << FIFO_CTL_HUBEN);
		break;
	case	1:
		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   0x1 << FIFO_CTL_HUBEN, 0x1 << FIFO_CTL_HUBEN);
		regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG,
				   0x1 << TXCFG_TXEN, 0x1 << TXCFG_TXEN);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void sunxi_rx_sync_enable(void *data, bool enable)
{
	struct regmap *regmap = data;

	if (enable)
		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   0x1 << SPDIF_RX_SYNC_EN_START, 0x1 << SPDIF_RX_SYNC_EN_START);
	else
		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   0x1 << SPDIF_RX_SYNC_EN_START, 0x0 << SPDIF_RX_SYNC_EN_START);

}

static int sunxi_get_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *spdif_info = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_dts_info *dts_info = &spdif_info->dts_info;

	ucontrol->value.integer.value[0] = dts_info->rx_sync_ctl;

	return 0;
}

static int sunxi_set_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *spdif_info = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_dts_info *dts_info = &spdif_info->dts_info;
	struct regmap *regmap = spdif_info->mem_info.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		dts_info->rx_sync_ctl = 0;
		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   0x1 << SPDIF_RX_SYNC_EN, 0x0 << SPDIF_RX_SYNC_EN);
		sunxi_rx_sync_shutdown(dts_info->rx_sync_domain, dts_info->rx_sync_id);
		break;
	case	1:
		sunxi_rx_sync_startup(dts_info->rx_sync_domain, dts_info->rx_sync_id,
				      (void *)regmap, sunxi_rx_sync_enable);
		regmap_update_bits(regmap, SUNXI_SPDIF_FIFO_CTL,
				   0x1 << SPDIF_RX_SYNC_EN, 0x1 << SPDIF_RX_SYNC_EN);
		dts_info->rx_sync_ctl = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *sunxi_switch_text[] = {"Off", "On"};
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};

static int sunxi_spdif_component_probe(struct snd_soc_component *component)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_dts_info *dts_info = &spdif_info->dts_info;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	/* component kcontrols -> tx_hub */
	if (dts_info->tx_hub_en) {
		ret = snd_soc_add_component_controls(component, sunxi_tx_hub_controls,
						     ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add tx_hub kcontrols failed\n");
	}

	/* component kcontrols -> rx_sync */
	if (dts_info->rx_sync_en) {
		ret = snd_soc_add_component_controls(component, sunxi_rx_sync_controls,
						     ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add rx_sync kcontrols failed\n");
	}

	return 0;
}

static int sunxi_spdif_component_suspend(struct snd_soc_component *component)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_pinctl_info *pin_info = &spdif_info->pin_info;
	struct sunxi_spdif_clk_info *clk_info = &spdif_info->clk_info;
	struct regmap *regmap = spdif_info->mem_info.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_save_reg(regmap, sunxi_reg_labels);

	pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate_sleep);
	regmap_update_bits(regmap, SUNXI_SPDIF_CTL, 1 << CTL_GEN_EN, 0 << CTL_GEN_EN);
	snd_sunxi_clk_disable(spdif_info->pdev, clk_info);

	return 0;
}

static int sunxi_spdif_component_resume(struct snd_soc_component *component)
{
	struct sunxi_spdif_info *spdif_info = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_pinctl_info *pin_info = &spdif_info->pin_info;
	struct sunxi_spdif_clk_info *clk_info = &spdif_info->clk_info;
	struct regmap *regmap = spdif_info->mem_info.regmap;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = snd_sunxi_clk_enable(spdif_info->pdev, clk_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		return ret;
	}

	regmap_update_bits(regmap, SUNXI_SPDIF_CTL, 1 << CTL_GEN_EN, 1 << CTL_GEN_EN);
	pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate);

	sunxi_spdif_init(spdif_info);
	snd_sunxi_echo_reg(regmap, sunxi_reg_labels);

	return 0;
}

static int sunxi_spdif_set_data_fmt(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *spdif_info = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = spdif_info->mem_info.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_SPDIF_TXCH_STA0, &reg_val);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		reg_val = 0;
		break;
	case 1:
		reg_val = 1;
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_SPDIF_TXCFG,
			   1 << TXCFG_DATA_TYPE, reg_val << TXCFG_DATA_TYPE);
	regmap_update_bits(regmap, SUNXI_SPDIF_TXCH_STA0,
			   1 << TXCHSTA0_AUDIO, reg_val << TXCHSTA0_AUDIO);
	regmap_update_bits(regmap, SUNXI_SPDIF_RXCH_STA0,
			   1 << RXCHSTA0_AUDIO, reg_val << RXCHSTA0_AUDIO);

	return 0;
}

static int sunxi_spdif_get_data_fmt(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *spdif_info = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = spdif_info->mem_info.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_SPDIF_TXCFG, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> TXCFG_DATA_TYPE) & 0x1;

	return 0;
}

static const char *data_fmt[] = {"PCM", "RAW"};
static SOC_ENUM_SINGLE_EXT_DECL(data_fmt_enum, data_fmt);

static const struct snd_kcontrol_new sunxi_spdif_controls[] = {
	SOC_ENUM_EXT("audio data format", data_fmt_enum,
		     sunxi_spdif_get_data_fmt, sunxi_spdif_set_data_fmt),
	SOC_SINGLE("loopback debug", SUNXI_SPDIF_CTL, CTL_LOOP_EN, 1, 0),
};

static struct snd_soc_component_driver sunxi_spdif_component = {
	.name		= DRV_NAME,
	.probe		= sunxi_spdif_component_probe,
	.suspend	= sunxi_spdif_component_suspend,
	.resume		= sunxi_spdif_component_resume,
	.controls	= sunxi_spdif_controls,
	.num_controls	= ARRAY_SIZE(sunxi_spdif_controls),
};

/*******************************************************************************
 * *** kernel source ***
 * @0 reg debug
 * @1 regmap
 * @2 clk
 * @3 regulator
 * @4 dts params
 ******************************************************************************/
static int snd_sunxi_save_reg(struct regmap *regmap, struct reg_label *sunxi_reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (sunxi_reg_labels[i].name != NULL) {
		regmap_read(regmap, sunxi_reg_labels[i].address, &(sunxi_reg_labels[i].value));
		i++;
	}

	return i;
}

static int snd_sunxi_echo_reg(struct regmap *regmap, struct reg_label *sunxi_reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (sunxi_reg_labels[i].name != NULL) {
		regmap_write(regmap, sunxi_reg_labels[i].address, sunxi_reg_labels[i].value);
		i++;
	}

	return i;
}

static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_spdif_mem_info *mem_info)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = of_address_to_resource(np, 0, &mem_info->res);
	if (ret) {
		SND_LOG_ERR(HLOG, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem_info->memregion = devm_request_mem_region(&pdev->dev, mem_info->res.start,
						      resource_size(&mem_info->res), DRV_NAME);
	if (IS_ERR_OR_NULL(mem_info->memregion)) {
		SND_LOG_ERR(HLOG, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem_info->membase = devm_ioremap(&pdev->dev, mem_info->memregion->start,
					 resource_size(mem_info->memregion));
	if (IS_ERR_OR_NULL(mem_info->membase)) {
		SND_LOG_ERR(HLOG, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem_info->regmap = devm_regmap_init_mmio(&pdev->dev, mem_info->membase,
						 &sunxi_regmap_config);
	if (IS_ERR_OR_NULL(mem_info->regmap)) {
		SND_LOG_ERR(HLOG, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem_info->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_spdif_mem_info *mem_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	devm_iounmap(&pdev->dev, mem_info->membase);
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
}

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_spdif_clk_info *clk_info)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get rst clk */
	clk_info->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk_info->clk_rst)) {
		SND_LOG_ERR(HLOG, "clk rst get failed\n");
		ret =  PTR_ERR(clk_info->clk_rst);
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk_info->clk_bus = of_clk_get_by_name(np, "clk_bus_spdif");
	if (IS_ERR_OR_NULL(clk_info->clk_bus)) {
		SND_LOG_ERR(HLOG, "clk bus get failed\n");
		ret = PTR_ERR(clk_info->clk_bus);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	ret = of_property_read_u32(np, "pll-fs", &temp_val);
	if (ret < 0) {
		clk_info->pll_fs = 1;
	} else {
		clk_info->pll_fs = temp_val;
	}
	switch (clk_info->pll_fs) {
	case 1:
		clk_info->clk_pll_audio = of_clk_get_by_name(np, "clk_pll_audio");
		if (IS_ERR_OR_NULL(clk_info->clk_pll_audio)) {
			SND_LOG_ERR(HLOG, "clk pll get failed\n");
			ret = PTR_ERR(clk_info->clk_pll_audio);
			goto err_get_clk_parent;
		}
		clk_info->clk_parent = clk_info->clk_pll_audio;
		break;
	case 4:
		clk_info->clk_pll_audio_4x = of_clk_get_by_name(np, "clk_pll_audio_4x");
		if (IS_ERR_OR_NULL(clk_info->clk_pll_audio_4x)) {
			SND_LOG_ERR(HLOG, "clk pll get failed\n");
			ret = PTR_ERR(clk_info->clk_pll_audio_4x);
			goto err_get_clk_parent;
		}
		clk_info->clk_parent = clk_info->clk_pll_audio_4x;
		break;
	default:
		SND_LOG_ERR(HLOG, "unsupport pll fs\n");
		goto err_get_clk_parent;
	}

	/* get spdif clk */
	clk_info->clk_spdif = of_clk_get_by_name(np, "clk_spdif");
	if (IS_ERR_OR_NULL(clk_info->clk_spdif)) {
		SND_LOG_ERR(HLOG, "clk spdif get failed\n");
		ret = PTR_ERR(clk_info->clk_spdif);
		goto err_get_clk_spdif;
	}

	/* set clk spdif parent of pllaudio */
	if (clk_set_parent(clk_info->clk_spdif, clk_info->clk_parent)) {
		SND_LOG_ERR(HLOG, "set parent clk spdif failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	ret = snd_sunxi_clk_enable(pdev, clk_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		ret = -EINVAL;
		goto err_clk_enable;
	}

	return 0;

err_clk_enable:
err_set_parent:
	clk_put(clk_info->clk_spdif);
err_get_clk_spdif:
	if (IS_ERR_OR_NULL(clk_info->clk_parent))
		clk_put(clk_info->clk_parent);
err_get_clk_parent:
	clk_put(clk_info->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	return ret;
}

static void snd_sunxi_clk_exit(struct platform_device *pdev, struct sunxi_spdif_clk_info *clk_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_clk_disable(pdev, clk_info);
	clk_put(clk_info->clk_spdif);
	clk_put(clk_info->clk_parent);
	clk_put(clk_info->clk_bus);
}

static int snd_sunxi_clk_enable(struct platform_device *pdev,
				struct sunxi_spdif_clk_info *clk_info)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	if (reset_control_deassert(clk_info->clk_rst)) {
		SND_LOG_ERR(HLOG, "clk_rst deassert failed\n");
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk_info->clk_bus)) {
		SND_LOG_ERR(HLOG, "clk_bus enable failed\n");
		goto err_enable_clk_bus;
	}

	if (clk_prepare_enable(clk_info->clk_parent)) {
		SND_LOG_ERR(HLOG, "clk_parent enable failed\n");
		goto err_enable_clk_parent;
	}

	if (clk_prepare_enable(clk_info->clk_spdif)) {
		SND_LOG_ERR(HLOG, "clk_spdif enable failed\n");
		goto err_enable_clk_spdif;
	}

	return 0;

err_enable_clk_spdif:
	clk_disable_unprepare(clk_info->clk_parent);
err_enable_clk_parent:
	clk_disable_unprepare(clk_info->clk_bus);
err_enable_clk_bus:
	reset_control_assert(clk_info->clk_rst);
err_deassert_rst:
	return ret;
}

static void snd_sunxi_clk_disable(struct platform_device *pdev,
				  struct sunxi_spdif_clk_info *clk_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	clk_disable_unprepare(clk_info->clk_spdif);
	clk_disable_unprepare(clk_info->clk_parent);
	clk_disable_unprepare(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev,
				      struct sunxi_spdif_dts_info *dts_info)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get dma params */
	ret = of_property_read_u32(np, "playback_cma", &temp_val);
	if (ret < 0) {
		dts_info->playback_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN(HLOG, "playback_cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts_info->playback_cma = temp_val;
	}
	ret = of_property_read_u32(np, "capture_cma", &temp_val);
	if (ret != 0) {
		dts_info->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN(HLOG, "capture_cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts_info->capture_cma = temp_val;
	}
	ret = of_property_read_u32(np, "tx_fifo_size", &temp_val);
	if (ret != 0) {
		dts_info->playback_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN(HLOG, "tx_fifo_size miss, using default value\n");
	} else {
		dts_info->playback_fifo_size = temp_val;
	}
	ret = of_property_read_u32(np, "rx_fifo_size", &temp_val);
	if (ret != 0) {
		dts_info->capture_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN(HLOG, "rx_fifo_size miss,using default value\n");
	} else {
		dts_info->capture_fifo_size = temp_val;
	}

	SND_LOG_INFO(HLOG, "playback_cma : %zu\n", dts_info->playback_cma);
	SND_LOG_INFO(HLOG, "capture_cma  : %zu\n", dts_info->capture_cma);
	SND_LOG_INFO(HLOG, "tx_fifo_size : %zu\n", dts_info->playback_fifo_size);
	SND_LOG_INFO(HLOG, "rx_fifo_size : %zu\n", dts_info->capture_fifo_size);

	/* tx_hub */
	dts_info->tx_hub_en = of_property_read_bool(np, "tx_hub_en");

	/* components func -> rx_sync */
	dts_info->rx_sync_en = of_property_read_bool(np, "rx_sync_en");
	if (dts_info->rx_sync_en) {
		dts_info->rx_sync_ctl = false;
		dts_info->rx_sync_domain = RX_SYNC_SYS_DOMAIN;
		dts_info->rx_sync_id = sunxi_rx_sync_probe(dts_info->rx_sync_domain);
		if (dts_info->rx_sync_id < 0) {
			SND_LOG_ERR(HLOG, "sunxi_rx_sync_probe failed\n");
		} else {
			SND_LOG_INFO(HLOG, "sunxi_rx_sync_probe successful. domain=%d, id=%d\n",
				     dts_info->rx_sync_domain, dts_info->rx_sync_id);
		}
	}
}

static int snd_sunxi_pin_init(struct platform_device *pdev,
			      struct sunxi_spdif_pinctl_info *pin_info)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	if (of_property_read_bool(np, "pinctrl_used")) {
		pin_info->pinctrl_used = 1;
	} else {
		pin_info->pinctrl_used = 0;
		SND_LOG_INFO(HLOG, "unused pinctrl\n");
		return 0;
	}

	pin_info->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin_info->pinctrl)) {
		SND_LOG_ERR(HLOG, "pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin_info->pinstate = pinctrl_lookup_state(pin_info->pinctrl,
						  PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin_info->pinstate)) {
		SND_LOG_ERR(HLOG, "pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin_info->pinstate_sleep = pinctrl_lookup_state(pin_info->pinctrl,
							PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin_info->pinstate_sleep)) {
		SND_LOG_ERR(HLOG, "pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "spdif set pinctrl default state fail\n");
		ret = -EBUSY;
		goto err_pinctrl_select_default;
	}

	return 0;

err_pinctrl_select_default:
err_loopup_pin_sleep:
err_loopup_pinstate:
	devm_pinctrl_put(pin_info->pinctrl);
	return ret;
}

static void snd_sunxi_pin_exit(struct platform_device *pdev,
			       struct sunxi_spdif_pinctl_info *pin_info)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (pin_info->pinctrl_used)
		devm_pinctrl_put(pin_info->pinctrl);
}

static void snd_sunxi_dma_params_init(struct sunxi_spdif_info *spdif_info)
{
	struct resource *res = &spdif_info->mem_info.res;
	struct sunxi_spdif_dts_info *dts_info = &spdif_info->dts_info;

	SND_LOG_DEBUG(HLOG, "\n");

	spdif_info->playback_dma_param.src_maxburst = 8;
	spdif_info->playback_dma_param.dst_maxburst = 8;
	spdif_info->playback_dma_param.dma_addr = res->start + SUNXI_SPDIF_TXFIFO;
	spdif_info->playback_dma_param.cma_kbytes = dts_info->playback_cma;
	spdif_info->playback_dma_param.fifo_size = dts_info->playback_fifo_size;

	spdif_info->capture_dma_param.src_maxburst = 8;
	spdif_info->capture_dma_param.dst_maxburst = 8;
	spdif_info->capture_dma_param.dma_addr = res->start + SUNXI_SPDIF_RXFIFO;
	spdif_info->capture_dma_param.cma_kbytes = dts_info->capture_cma;
	spdif_info->capture_dma_param.fifo_size = dts_info->capture_fifo_size;
};

static int sunxi_spdif_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_spdif_info *spdif_info;
	struct sunxi_spdif_mem_info *mem_info;
	struct sunxi_spdif_clk_info *clk_info;
	struct sunxi_spdif_pinctl_info *pin_info;
	struct sunxi_spdif_dts_info *dts_info;

	SND_LOG_DEBUG(HLOG, "\n");

	/* sunxi spdif info */
	spdif_info = devm_kzalloc(dev, sizeof(struct sunxi_spdif_info), GFP_KERNEL);
	if (!spdif_info) {
		SND_LOG_ERR(HLOG, "can't allocate sunxi spdif memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, spdif_info);
	mem_info = &spdif_info->mem_info;
	clk_info = &spdif_info->clk_info;
	pin_info = &spdif_info->pin_info;
	dts_info = &spdif_info->dts_info;

	ret = snd_sunxi_mem_init(pdev, mem_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "remap init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_mem_init;
	}

	ret = snd_sunxi_clk_init(pdev, clk_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_clk_init;
	}

	snd_sunxi_dts_params_init(pdev, dts_info);

	ret = snd_sunxi_pin_init(pdev, pin_info);
	if (ret) {
		SND_LOG_ERR(HLOG, "pinctrl init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_pin_init;
	}

	snd_sunxi_dma_params_init(spdif_info);

	ret = snd_soc_register_component(&pdev->dev,
					 &sunxi_spdif_component,
					 &sunxi_spdif_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_soc_sunxi_dma_platform_register(&pdev->dev, 0);
	if (ret) {
		SND_LOG_ERR(HLOG, "register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_sunxi_dma_platform_register;
	}

	SND_LOG_DEBUG(HLOG, "register spdif platform success\n");

	return 0;

err_snd_soc_sunxi_dma_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
err_snd_sunxi_pin_init:
	snd_sunxi_clk_exit(pdev, clk_info);
err_snd_sunxi_clk_init:
	snd_sunxi_mem_exit(pdev, mem_info);
err_snd_sunxi_mem_init:
	devm_kfree(dev, spdif_info);
err_devm_kzalloc:
	of_node_put(np);
	return ret;
}

static int sunxi_spdif_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_spdif_info *spdif_info = dev_get_drvdata(&pdev->dev);
	struct sunxi_spdif_mem_info *mem_info = &spdif_info->mem_info;
	struct sunxi_spdif_clk_info *clk_info = &spdif_info->clk_info;
	struct sunxi_spdif_pinctl_info *pin_info = &spdif_info->pin_info;
	struct sunxi_spdif_dts_info *dts_info = &spdif_info->dts_info;

	/* remove components */
	if (dts_info->rx_sync_en) {
		sunxi_rx_sync_remove(dts_info->rx_sync_domain);
	}

	snd_soc_sunxi_dma_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	snd_sunxi_clk_exit(pdev, clk_info);
	snd_sunxi_mem_exit(pdev, mem_info);
	snd_sunxi_pin_exit(pdev, pin_info);

	devm_kfree(dev, spdif_info);
	of_node_put(np);

	SND_LOG_DEBUG(HLOG, "unregister spdif platform success\n");

	return 0;
}

static const struct of_device_id sunxi_spdif_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_spdif_of_match);

static struct platform_driver sunxi_spdif_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_spdif_of_match,
	},
	.probe	= sunxi_spdif_dev_probe,
	.remove	= sunxi_spdif_dev_remove,
};

int __init sunxi_spdif_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_spdif_driver);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_spdif_dev_exit(void)
{
	platform_driver_unregister(&sunxi_spdif_driver);
}

late_initcall(sunxi_spdif_dev_init);
module_exit(sunxi_spdif_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of spdif");

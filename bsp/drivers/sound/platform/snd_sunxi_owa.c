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

#define SUNXI_MODNAME		"sound-owa"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/dmaengine_pcm.h>

#include "snd_sunxi_owa.h"
#include "snd_sunxi_adapter.h"

#define DRV_NAME	"sunxi-snd-plat-owa"

/* for reg debug */
static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_OWA_CTL),
	REG_LABEL(SUNXI_OWA_TXCFG),
	REG_LABEL(SUNXI_OWA_RXCFG),
	REG_LABEL(SUNXI_OWA_INT_STA),
	/* REG_LABEL(SUNXI_OWA_RXFIFO), */
	REG_LABEL(SUNXI_OWA_FIFO_CTL),
	REG_LABEL(SUNXI_OWA_FIFO_STA),
	REG_LABEL(SUNXI_OWA_INT),
	/* REG_LABEL(SUNXI_OWA_TXFIFO), */
	REG_LABEL(SUNXI_OWA_TXCNT),
	REG_LABEL(SUNXI_OWA_RXCNT),
	REG_LABEL(SUNXI_OWA_TXCH_STA0),
	REG_LABEL(SUNXI_OWA_TXCH_STA1),
	REG_LABEL(SUNXI_OWA_RXCH_STA0),
	REG_LABEL(SUNXI_OWA_RXCH_STA1),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

static struct regmap_config sunxi_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_OWA_REG_MAX,
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

static int sunxi_owa_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_owa *owa = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream, &owa->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream, &owa->capture_dma_param);

	return 0;
}

static void sunxi_owa_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	(void)substream;
	(void)dai;
}

static void sunxi_owa_lock_confirm_work(struct work_struct *work)
{
	unsigned int owa_flag_state;
	unsigned int owa_irq_state;
	struct sunxi_owa_irq *owa_irq = container_of(work,
					     struct sunxi_owa_irq, lock_confirm_work.work);
	struct sunxi_owa *owa = container_of(owa_irq,
					     struct sunxi_owa, owa_irq);
	struct regmap *regmap = owa->mem.regmap;
	const struct sunxi_owa_quirks *quirks = owa->quirks;
	struct snd_pcm_substream *substream = owa_irq->substream;

	SND_LOG_DEBUG("\n");

	/* unplug and plug in again will run this case during recording */
	regmap_read(regmap, SUNXI_OWA_RXCFG, &owa_flag_state);
	if (owa_flag_state & (0x1 << RXCFG_LOCK_FLAG)) {
		SND_LOG_DEBUG("enter lock process\n");

		/* add for false trigger interrupt */
		owa_irq->flag_sta = RX_LOCK;
		if (owa_irq->flag_sta == owa_irq->flag_sta_old) {
			SND_LOG_DEBUG("lock unchanged\n");

			/* clear int status */
			regmap_read(regmap, SUNXI_OWA_INT_STA, &owa_irq_state);
			regmap_write(regmap, SUNXI_OWA_INT_STA, owa_irq_state);

			/* reopen unlock & parity int */
			regmap_update_bits(regmap, SUNXI_OWA_INT,
					   0x1 << INT_RXUNLOCKEN, 0x1 << INT_RXUNLOCKEN);
			regmap_update_bits(regmap, SUNXI_OWA_INT,
					   0x1 << INT_RXPAREN, 0x1 << INT_RXPAREN);
			return;
		}

		/* close RXEN */
		regmap_update_bits(regmap, SUNXI_OWA_RXCFG, 1 << RXCFG_RXEN, 0 << RXCFG_RXEN);

		/* flush RXFIFO */
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   1 << quirks->fifo_ctl_frx, 1 << quirks->fifo_ctl_frx);
		regmap_write(regmap, SUNXI_OWA_RXCNT, 0);

		/* enable RXDRQEN & RXEN */
		regmap_update_bits(regmap, SUNXI_OWA_RXCFG, 1 << RXCFG_CHSR_CP, 1 << RXCFG_CHSR_CP);
		regmap_update_bits(regmap, SUNXI_OWA_INT, 1 << INT_RXDRQEN, 1 << INT_RXDRQEN);
		regmap_update_bits(regmap, SUNXI_OWA_RXCFG, 1 << RXCFG_RXEN, 1 << RXCFG_RXEN);

		/* reopen dma */
		snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_START);

		/* clear int status */
		regmap_read(regmap, SUNXI_OWA_INT_STA, &owa_irq_state);
		regmap_write(regmap, SUNXI_OWA_INT_STA, owa_irq_state);

		/* reopen unlock & parity int */
		regmap_update_bits(regmap, SUNXI_OWA_INT,
				   0x1 << INT_RXUNLOCKEN, 0x1 << INT_RXUNLOCKEN);
		regmap_update_bits(regmap, SUNXI_OWA_INT,
				   0x1 << INT_RXPAREN, 0x1 << INT_RXPAREN);
	} else {
		SND_LOG_DEBUG("enter unlock or parity process\n");

		/* add for false trigger interrupt */
		owa_irq->flag_sta = RX_UNLOCK;
		if (owa_irq->flag_sta == owa_irq->flag_sta_old) {
			SND_LOG_DEBUG("unlock unchanged\n");

			/* clear int status */
			regmap_read(regmap, SUNXI_OWA_INT_STA, &owa_irq_state);
			regmap_write(regmap, SUNXI_OWA_INT_STA, owa_irq_state);

			/* reopen lock int */
			regmap_update_bits(regmap, SUNXI_OWA_INT,
					   0x1 << INT_RXLOCKEN, 0x1 << INT_RXLOCKEN);
			return;
		}

		/* close RXDRQEN & stop dma transfer */
		regmap_update_bits(regmap, SUNXI_OWA_INT, 1 << INT_RXDRQEN, 0 << INT_RXDRQEN);

		/* pause dma */
		snd_dmaengine_pcm_trigger(substream, SNDRV_PCM_TRIGGER_STOP);

		/* clean up dma buffer */
		memset(substream->dma_buffer.area, 0, substream->dma_buffer.bytes);

		/* clear int status */
		regmap_read(regmap, SUNXI_OWA_INT_STA, &owa_irq_state);
		regmap_write(regmap, SUNXI_OWA_INT_STA, owa_irq_state);

		/* reopen lock int */
		regmap_update_bits(regmap, SUNXI_OWA_INT,
				   0x1 << INT_RXLOCKEN, 0x1 << INT_RXLOCKEN);
	}

	owa_irq->flag_sta_old = owa_irq->flag_sta;
}

static irqreturn_t owa_interrupt(int irq, void *dev_id)
{
	struct sunxi_owa *owa = (struct sunxi_owa *)dev_id;
	struct sunxi_owa_irq *owa_irq = &owa->owa_irq;
	struct regmap *regmap = owa->mem.regmap;

	SND_LOG_DEBUG("\n");

	/* diabled owa lock&unlock&parity irq */
	regmap_update_bits(regmap, SUNXI_OWA_INT,
			   0x1 << INT_RXLOCKEN, 0x0 << INT_RXLOCKEN);
	regmap_update_bits(regmap, SUNXI_OWA_INT,
			   0x1 << INT_RXUNLOCKEN, 0x0 << INT_RXUNLOCKEN);
	regmap_update_bits(regmap, SUNXI_OWA_INT,
			   0x1 << INT_RXPAREN, 0x0 << INT_RXPAREN);

	if (owa_irq->running)
		schedule_delayed_work(&owa_irq->lock_confirm_work, msecs_to_jiffies(50));

	return IRQ_HANDLED;
}

static int sunxi_owa_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
				   unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_owa *owa = snd_soc_dai_get_drvdata(dai);
	struct sunxi_owa_dts *dts = &owa->dts;

	freq_out = freq_out / dts->pll_fs;

	SND_LOG_DEBUG("stream -> %s, freq_in ->%u, freq_out ->%u, pll_fs ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out, dts->pll_fs);

	if (snd_owa_clk_rate(owa->clk, freq_in, freq_out)) {
		SND_LOG_ERR("clk set rate failed\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_owa_dai_set_clkdiv(struct snd_soc_dai *dai, int clk_id, int clk_div)
{
	struct sunxi_owa *owa = snd_soc_dai_get_drvdata(dai);
	struct sunxi_owa_dts *dts = &owa->dts;
	struct regmap *regmap = owa->mem.regmap;

	SND_LOG_DEBUG("\n");

	clk_div = clk_div / dts->pll_fs;
	clk_div = clk_div >> 7;	/* fs = owa_clk/[(div+1)*64*2] */
	regmap_update_bits(regmap, SUNXI_OWA_TXCFG,
			   0x1F << TXCFG_CLK_DIV_RATIO, (clk_div - 1) << TXCFG_CLK_DIV_RATIO);

	return 0;
}

static int sunxi_owa_dai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct sunxi_owa *owa = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = owa->mem.regmap;
	unsigned int tx_input_mode = 0;
	unsigned int rx_output_mode = 0;
	unsigned int origin_freq_bit = 0, sample_freq_bit = 0;
	unsigned int reg_temp;
	unsigned int i;

	SND_LOG_DEBUG("\n");

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		reg_temp = 0;
		tx_input_mode = 1;
		rx_output_mode = 3;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		reg_temp = 1;
		tx_input_mode = 0;
		rx_output_mode = 0;
		break;
		/* only for the compatible of tinyalsa */
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		reg_temp = 2;
		tx_input_mode = 0;
		rx_output_mode = 0;
		break;
	default:
		SND_LOG_ERR_STD(E_OWA_SWARG_HW_PARAMS,
				"params_format[%d] error!\n", params_format(params));
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
		regmap_update_bits(regmap, SUNXI_OWA_TXCFG,
				   3 << TXCFG_SAMPLE_BIT, reg_temp << TXCFG_SAMPLE_BIT);

		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   1 << FIFO_CTL_TXIM, tx_input_mode << FIFO_CTL_TXIM);

		if (params_channels(params) == 1) {
			regmap_update_bits(regmap, SUNXI_OWA_TXCFG,
					   1 << TXCFG_SINGLE_MOD, 1 << TXCFG_SINGLE_MOD);
		} else {
			regmap_update_bits(regmap, SUNXI_OWA_TXCFG,
					   1 << TXCFG_SINGLE_MOD, 0 << TXCFG_SINGLE_MOD);
		}

		/* samplerate conversion */
		regmap_update_bits(regmap, SUNXI_OWA_TXCH_STA0,
				   0xF << TXCHSTA0_SAMFREQ,
				   sample_freq_bit << TXCHSTA0_SAMFREQ);
		regmap_update_bits(regmap, SUNXI_OWA_TXCH_STA1,
				   0xF << TXCHSTA1_ORISAMFREQ,
				   origin_freq_bit << TXCHSTA1_ORISAMFREQ);
		switch (reg_temp) {
		case 0:
			regmap_update_bits(regmap, SUNXI_OWA_TXCH_STA1,
					   0xF << TXCHSTA1_MAXWORDLEN, 2 << TXCHSTA1_MAXWORDLEN);
			break;
		case 1:
			regmap_update_bits(regmap, SUNXI_OWA_TXCH_STA1,
					   0xF << TXCHSTA1_MAXWORDLEN, 0xC << TXCHSTA1_MAXWORDLEN);
			break;
		case 2:
			regmap_update_bits(regmap, SUNXI_OWA_TXCH_STA1,
					   0xF << TXCHSTA1_MAXWORDLEN, 0xB << TXCHSTA1_MAXWORDLEN);
			break;
		default:
			SND_LOG_ERR_STD(E_OWA_SWARG_HW_PARAMS, "unexpection error\n");
			return -EINVAL;
		}
	} else {
		/*
		 * FIXME, not sync as spec says, just test 16bit & 24bit,
		 * using 3 working ok
		 */
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   0x3 << FIFO_CTL_RXOM,
				   rx_output_mode << FIFO_CTL_RXOM);
		regmap_update_bits(regmap, SUNXI_OWA_RXCH_STA0,
				   0xF<<RXCHSTA0_SAMFREQ,
				   sample_freq_bit << RXCHSTA0_SAMFREQ);
		regmap_update_bits(regmap, SUNXI_OWA_RXCH_STA1,
				   0xF<<RXCHSTA1_ORISAMFREQ,
				   origin_freq_bit << RXCHSTA1_ORISAMFREQ);

		switch (reg_temp) {
		case 0:
			regmap_update_bits(regmap, SUNXI_OWA_RXCH_STA1,
					   0xF << RXCHSTA1_MAXWORDLEN, 2 << RXCHSTA1_MAXWORDLEN);
			break;
		case 1:
			regmap_update_bits(regmap, SUNXI_OWA_RXCH_STA1,
					   0xF << RXCHSTA1_MAXWORDLEN, 0xC << RXCHSTA1_MAXWORDLEN);
			break;
		case 2:
			regmap_update_bits(regmap, SUNXI_OWA_RXCH_STA1,
					   0xF << RXCHSTA1_MAXWORDLEN, 0xB << RXCHSTA1_MAXWORDLEN);
			break;
		default:
			SND_LOG_ERR_STD(E_OWA_SWARG_HW_PARAMS, "unexpection error\n");
			return -EINVAL;
		}
	}

	/* enable clk after set clk rate */
	if (snd_owa_clk_enable(owa->clk)) {
		SND_LOG_ERR("clk enable failed\n");
		return -EINVAL;
	} else {
		owa->clk_sta = SND_SUNXI_CLK_OPEN;
	}

	return 0;
}

static int sunxi_owa_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_owa *owa = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	/* prevent the closed clks from being closed again */
	if (owa->clk_sta == SND_SUNXI_CLK_OPEN) {
		snd_owa_clk_disable(owa->clk);
		owa->clk_sta = SND_SUNXI_CLK_CLOSE;
	}

	return 0;
}

static int sunxi_owa_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_owa *owa = snd_soc_dai_get_drvdata(dai);
	const struct sunxi_owa_quirks *quirks = owa->quirks;
	struct regmap *regmap = owa->mem.regmap;
	unsigned int reg_val;

	SND_LOG_DEBUG("\n");

	/* as you need to clean up TX or RX FIFO , need to turn off GEN bit */
	regmap_update_bits(regmap, SUNXI_OWA_CTL, 1 << CTL_GEN_EN, 0 << CTL_GEN_EN);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   1 << quirks->fifo_ctl_ftx, 1 << quirks->fifo_ctl_ftx);
		regmap_write(regmap, SUNXI_OWA_TXCNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   1 << quirks->fifo_ctl_frx, 1 << quirks->fifo_ctl_frx);
		regmap_write(regmap, SUNXI_OWA_RXCNT, 0);
	}

	/* clear all interrupt status */
	regmap_read(regmap, SUNXI_OWA_INT_STA, &reg_val);
	regmap_write(regmap, SUNXI_OWA_INT_STA, reg_val);

	/* need reset */
	regmap_update_bits(regmap, SUNXI_OWA_CTL,
			   1 << CTL_RESET | 1 << CTL_GEN_EN, 1 << CTL_RESET | 1 << CTL_GEN_EN);

	return 0;
}

static void sunxi_owa_txctrl_enable(struct sunxi_owa *owa, int enable)
{
	struct regmap *regmap = owa->mem.regmap;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_OWA_TXCFG, 1 << TXCFG_TXEN, 1 << TXCFG_TXEN);
		regmap_update_bits(regmap, SUNXI_OWA_INT, 1 << INT_TXDRQEN, 1 << INT_TXDRQEN);
	} else {
		regmap_update_bits(regmap, SUNXI_OWA_TXCFG, 1 << TXCFG_TXEN, 0 << TXCFG_TXEN);
		regmap_update_bits(regmap, SUNXI_OWA_INT, 1 << INT_TXDRQEN, 0 << INT_TXDRQEN);
	}
}

static void sunxi_owa_rxctrl_enable(struct sunxi_owa *owa, int enable)
{
	struct regmap *regmap = owa->mem.regmap;
	struct sunxi_owa_irq *owa_irq = &owa->owa_irq;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_OWA_RXCFG,
				   1 << RXCFG_CHSR_CP, 1 << RXCFG_CHSR_CP);
		regmap_update_bits(regmap, SUNXI_OWA_INT, 1 << INT_RXDRQEN, 1 << INT_RXDRQEN);
		regmap_update_bits(regmap, SUNXI_OWA_RXCFG, 1 << RXCFG_RXEN, 1 << RXCFG_RXEN);
		if (owa_irq->id) {
			owa_irq->flag_sta_old = RX_LOCK_DEFAULT;
			owa_irq->flag_sta = RX_LOCK_DEFAULT;

			regmap_update_bits(regmap, SUNXI_OWA_INT,
					   0x1 << INT_RXLOCKEN, 0x0 << INT_RXLOCKEN);
			regmap_update_bits(regmap, SUNXI_OWA_INT,
					   0x1 << INT_RXUNLOCKEN, 0x1 << INT_RXUNLOCKEN);
			regmap_update_bits(regmap, SUNXI_OWA_INT,
					   0x1 << INT_RXPAREN, 0x1 << INT_RXPAREN);
		}
	} else {
		regmap_update_bits(regmap, SUNXI_OWA_RXCFG, 1 << RXCFG_RXEN, 0 << RXCFG_RXEN);
		regmap_update_bits(regmap, SUNXI_OWA_INT, 1 << INT_RXDRQEN, 0 << INT_RXDRQEN);
		if (owa_irq->id) {
			owa_irq->flag_sta_old = RX_LOCK_DEFAULT;
			owa_irq->flag_sta = RX_LOCK_DEFAULT;

			regmap_update_bits(regmap, SUNXI_OWA_INT,
					   0x1 << INT_RXLOCKEN, 0x0 << INT_RXLOCKEN);
			regmap_update_bits(regmap, SUNXI_OWA_INT,
					   0x1 << INT_RXUNLOCKEN, 0x0 << INT_RXUNLOCKEN);
			regmap_update_bits(regmap, SUNXI_OWA_INT,
					   0x1 << INT_RXPAREN, 0x0 << INT_RXPAREN);
		}
	}
}

static int sunxi_owa_dai_trigger(struct snd_pcm_substream *substream,
				 int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_owa *owa = snd_soc_dai_get_drvdata(dai);
	struct sunxi_owa_irq *owa_irq = &owa->owa_irq;
	struct sunxi_owa_dts *dts = &owa->dts;

	SND_LOG_DEBUG("cmd -> %d\n", cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_owa_txctrl_enable(owa, 1);
		} else {
			owa_irq->running = true;
			owa_irq->substream = substream;
			/* rxsync en -> capture route -> drq en -> rxsync start */
			sunxi_owa_rxctrl_enable(owa, 1);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
	break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_owa_txctrl_enable(owa, 0);
		} else {
			owa_irq->running = false;
			sunxi_owa_rxctrl_enable(owa, 0);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
	break;
	default:
		SND_LOG_ERR_STD(E_OWA_SWARG_TRIGGER, "unsupport cmd\n");
		return -EINVAL;
	}

	return 0;
}

static struct snd_soc_dai_ops sunxi_owa_dai_ops = {
	/* call by machine */
	.set_pll	= sunxi_owa_dai_set_pll,	/* set pllclk */
	.set_clkdiv	= sunxi_owa_dai_set_clkdiv,	/* set clk div */
	/* call by asoc */
	.startup	= sunxi_owa_dai_startup,
	.hw_params	= sunxi_owa_dai_hw_params,	/* set hardware params */
	.prepare	= sunxi_owa_dai_prepare,	/* clean irq and fifo */
	.trigger	= sunxi_owa_dai_trigger,	/* set drq */
	.hw_free	= sunxi_owa_dai_hw_free,	/* release hw resourse */
	.shutdown	= sunxi_owa_dai_shutdown,
};

static void sunxi_owa_init(struct sunxi_owa *owa)
{
	const struct sunxi_owa_quirks *quirks = owa->quirks;
	struct regmap *regmap = owa->mem.regmap;

	SND_LOG_DEBUG("\n");

	/* FIFO CTL register default setting */
	regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
			   quirks->ctl_txtl_mask << quirks->fifo_ctl_txtl,
			   quirks->ctl_txtl_default << quirks->fifo_ctl_txtl);
	regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
			   quirks->ctl_rxtl_mask << quirks->fifo_ctl_rxtl,
			   quirks->ctl_rxtl_default << quirks->fifo_ctl_rxtl);

	/* send tx channel status info */
	regmap_update_bits(regmap, SUNXI_OWA_TXCFG,
			   1 << TXCFG_CHAN_STA_EN, 1 << TXCFG_CHAN_STA_EN);

	regmap_write(regmap, SUNXI_OWA_TXCH_STA0, 0x2 << TXCHSTA0_CHNUM);
	regmap_write(regmap, SUNXI_OWA_RXCH_STA0, 0x2 << RXCHSTA0_CHNUM);

	regmap_update_bits(regmap, SUNXI_OWA_CTL, 1 << CTL_GEN_EN, 1 << CTL_GEN_EN);
}

static int sunxi_owa_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_owa *owa = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, &owa->playback_dma_param, &owa->capture_dma_param);

	sunxi_owa_init(owa);

	return 0;
}

static int sunxi_owa_dai_remove(struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG("\n");

	return 0;
}

static struct snd_soc_dai_driver sunxi_owa_dai = {
	.name = DRV_NAME,
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
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = owa->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_OWA_FIFO_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << FIFO_CTL_HUBEN)) ? 1 : 0);

	return 0;
}

static int sunxi_set_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = owa->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_OWA_TXCFG,
				   0x1 << TXCFG_TXEN, 0x0 << TXCFG_TXEN);
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   0x1 << FIFO_CTL_HUBEN, 0x0 << FIFO_CTL_HUBEN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   0x1 << FIFO_CTL_HUBEN, 0x1 << FIFO_CTL_HUBEN);
		regmap_update_bits(regmap, SUNXI_OWA_TXCFG,
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
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   0x1 << OWA_RX_SYNC_EN_START, 0x1 << OWA_RX_SYNC_EN_START);
	else
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   0x1 << OWA_RX_SYNC_EN_START, 0x0 << OWA_RX_SYNC_EN_START);

}

static int sunxi_get_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct sunxi_owa_dts *dts = &owa->dts;

	ucontrol->value.integer.value[0] = dts->rx_sync_ctl;

	return 0;
}

static int sunxi_set_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct sunxi_owa_dts *dts = &owa->dts;
	struct regmap *regmap = owa->mem.regmap;

	if (dts->rx_sync_ctl == ucontrol->value.integer.value[0])
		return 0;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dts->rx_sync_ctl = 0;
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   0x1 << OWA_RX_SYNC_EN, 0x0 << OWA_RX_SYNC_EN);
		sunxi_rx_sync_shutdown(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_OWA_FIFO_CTL,
				   0x1 << OWA_RX_SYNC_EN, 0x1 << OWA_RX_SYNC_EN);
		dts->rx_sync_ctl = 1;
		sunxi_rx_sync_startup(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_owa_set_tx_data_fmt(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = owa->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_OWA_TXCH_STA0, &reg_val);

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

	regmap_update_bits(regmap, SUNXI_OWA_TXCFG,
			   1 << TXCFG_DATA_TYPE, reg_val << TXCFG_DATA_TYPE);
	regmap_update_bits(regmap, SUNXI_OWA_TXCH_STA0,
			   1 << TXCHSTA0_AUDIO, reg_val << TXCHSTA0_AUDIO);
	regmap_update_bits(regmap, SUNXI_OWA_RXCH_STA0,
			   1 << RXCHSTA0_AUDIO, reg_val << RXCHSTA0_AUDIO);

	return 0;
}

static int sunxi_owa_get_tx_data_fmt(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = owa->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_OWA_TXCFG, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> TXCFG_DATA_TYPE) & 0x1;

	return 0;
}

static int sunxi_get_rx_debug_info(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = owa->mem.regmap;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int shift = e->shift_l;
	unsigned int reg_val_0, reg_val_1;
	unsigned int word_len, rate;

	regmap_read(regmap, SUNXI_OWA_RXCH_STA0, &reg_val_0);
	regmap_read(regmap, SUNXI_OWA_RXCH_STA1, &reg_val_1);

	switch (shift) {
	case RX_CHANNEL_SHIFT:
		ucontrol->value.integer.value[0] = reg_val_0 & (0xF << RXCHSTA0_CHNUM);
		break;
	case RX_WORD_LEN_SHIFT:
		word_len = reg_val_1 & (0x7 << RXCHSTA1_SAMWORDLEN);
		if (reg_val_1 & (0x1 << RXCHSTA1_MAXWORDLEN)) {
			switch (word_len) {
			case 1:
				ucontrol->value.integer.value[0] = 5;
				break;
			case 2:
				ucontrol->value.integer.value[0] = 7;
				break;
			case 4:
				ucontrol->value.integer.value[0] = 8;
				break;
			case 5:
				ucontrol->value.integer.value[0] = 9;
				break;
			case 6:
				ucontrol->value.integer.value[0] = 6;
				break;
			default:
				ucontrol->value.integer.value[0] = 0;
			}
		} else {
			switch (word_len) {
			case 1:
				ucontrol->value.integer.value[0] = 1;
				break;
			case 2:
				ucontrol->value.integer.value[0] = 3;
				break;
			case 4:
				ucontrol->value.integer.value[0] = 4;
				break;
			case 5:
				ucontrol->value.integer.value[0] = 5;
				break;
			case 6:
				ucontrol->value.integer.value[0] = 2;
				break;
			default:
				ucontrol->value.integer.value[0] = 0;
			}
		}
		break;
	case RX_RATE_SHIFT:
		rate = reg_val_0 & (0xF << RXCHSTA0_SAMFREQ);
		switch (rate) {
		case 0:
			ucontrol->value.integer.value[0] = 4;
			break;
		case 2:
			ucontrol->value.integer.value[0] = 5;
			break;
		case 3:
			ucontrol->value.integer.value[0] = 3;
			break;
		case 4:
			ucontrol->value.integer.value[0] = 1;
			break;
		case 6:
			ucontrol->value.integer.value[0] = 2;
			break;
		case 9:
			ucontrol->value.integer.value[0] = 9;
			break;
		case 10:
			ucontrol->value.integer.value[0] = 6;
			break;
		case 12:
			ucontrol->value.integer.value[0] = 7;
			break;
		case 14:
			ucontrol->value.integer.value[0] = 8;
			break;
		default:
			ucontrol->value.integer.value[0] = 0;
		}
		break;
	case RX_ORIG_RATE_SHIFT:
		rate = reg_val_1 & (0xF << RXCHSTA1_ORISAMFREQ);
		switch (rate) {
		case 1:
			ucontrol->value.integer.value[0] = 13;
			break;
		case 2:
			ucontrol->value.integer.value[0] = 3;
			break;
		case 3:
			ucontrol->value.integer.value[0] = 12;
			break;
		case 5:
			ucontrol->value.integer.value[0] = 11;
			break;
		case 6:
			ucontrol->value.integer.value[0] = 1;
			break;
		case 7:
			ucontrol->value.integer.value[0] = 10;
			break;
		case 8:
			ucontrol->value.integer.value[0] = 4;
			break;
		case 9:
			ucontrol->value.integer.value[0] = 6;
			break;
		case 10:
			ucontrol->value.integer.value[0] = 2;
			break;
		case 11:
			ucontrol->value.integer.value[0] = 5;
			break;
		case 12:
			ucontrol->value.integer.value[0] = 7;
			break;
		case 13:
			ucontrol->value.integer.value[0] = 9;
			break;
		case 15:
			ucontrol->value.integer.value[0] = 8;
			break;
		default:
			ucontrol->value.integer.value[0] = 0;
		}
		break;
	case RX_DATA_TYPE_SHIFT:
		ucontrol->value.integer.value[0] = (reg_val_0 & (0x1 << RXCHSTA0_AUDIO)) ? 1 : 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_set_null_value(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	(void)kcontrol;
	(void)ucontrol;
	return 0;
}

static const char *data_fmt[] = {"PCM", "RAW"};
static const char *sunxi_switch_text[] = {"Off", "On"};
static const char *sunxi_num_text[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11",
				       "12", "13", "14", "15"};
static const char *sunxi_word_len_text[] = {"Null", "16bits", "17bits", "18bits", "19bits",
					    "20bits", "21bits", "22bits", "23bits", "24bits"};
static const char *sunxi_rate_text[] = {"Null", "22.05kHz", "24kHz", "32kHz", "44.1kHz", "48kHz",
					"96kHz", "176.4kHz", "192kHz", "768kHz"};
static const char *sunxi_origin_rate_text[] = {"Null", "8kHz", "11.025kHz", "12kHz", "16kHz",
					       "22.05kHz", "24kHz", "32kHz", "44.1kHz", "48kHz",
					       "88.2kHz", "96kHz", "176.4kHz", "192kHz"};
static const char *sunxi_data_type_text[] = {"Linear_PCM", "Nonlinear_PCM"};
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(tx_data_fmt_enum, data_fmt);
static SOC_ENUM_SINGLE_DECL(sunxi_rx_ch_cnt_enum, SND_SOC_NOPM, RX_CHANNEL_SHIFT, sunxi_num_text);
static SOC_ENUM_SINGLE_DECL(sunxi_word_len_enum, SND_SOC_NOPM, RX_WORD_LEN_SHIFT,
			    sunxi_word_len_text);
static SOC_ENUM_SINGLE_DECL(sunxi_rate_enum, SND_SOC_NOPM, RX_RATE_SHIFT, sunxi_rate_text);
static SOC_ENUM_SINGLE_DECL(sunxi_origin_rate_enum, SND_SOC_NOPM, RX_ORIG_RATE_SHIFT,
			    sunxi_origin_rate_text);
static SOC_ENUM_SINGLE_DECL(sunxi_rx_data_type_enum, SND_SOC_NOPM, RX_DATA_TYPE_SHIFT,
			    sunxi_data_type_text);

static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};

static const struct snd_kcontrol_new sunxi_loopback_controls[] = {
	SOC_SINGLE("loopback debug", SUNXI_OWA_CTL, CTL_LOOP_EN, 1, 0),
};

static const struct snd_kcontrol_new sunxi_owa_tx_raw_controls[] = {
	SOC_ENUM_EXT("audio tx data format", tx_data_fmt_enum,
		     sunxi_owa_get_tx_data_fmt, sunxi_owa_set_tx_data_fmt),
};

static const struct snd_kcontrol_new sunxi_debug_controls[] = {
	SOC_ENUM_EXT("rx channel cnt", sunxi_rx_ch_cnt_enum,
		     sunxi_get_rx_debug_info, sunxi_set_null_value),
	SOC_ENUM_EXT("rx word length", sunxi_word_len_enum,
		     sunxi_get_rx_debug_info, sunxi_set_null_value),
	SOC_ENUM_EXT("rx rate", sunxi_rate_enum,
		     sunxi_get_rx_debug_info, sunxi_set_null_value),
	SOC_ENUM_EXT("rx origin rate", sunxi_origin_rate_enum,
		     sunxi_get_rx_debug_info, sunxi_set_null_value),
	SOC_ENUM_EXT("rx data type", sunxi_rx_data_type_enum,
		     sunxi_get_rx_debug_info, sunxi_set_null_value),
};

static int sunxi_owa_component_probe(struct snd_soc_component *component)
{
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	const struct sunxi_owa_quirks *quirks = owa->quirks;
	struct regmap *regmap = owa->mem.regmap;
	struct sunxi_owa_dts *dts = &owa->dts;
	int ret;

	SND_LOG_DEBUG("\n");

	/* component kcontrols -> tx_hub */
	if (dts->tx_hub_en) {
		ret = snd_soc_add_component_controls(component, sunxi_tx_hub_controls,
						     ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR_STD(E_OWA_SWSYS_COMP_PROBE, "add tx_hub kcontrols failed\n");
	}

	/* component kcontrols -> rx_sync */
	if (quirks->rx_sync_en && dts->rx_sync_en) {
		ret = snd_soc_add_component_controls(component, sunxi_rx_sync_controls,
						     ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR_STD(E_OWA_SWSYS_COMP_PROBE, "add rx_sync kcontrols failed\n");

		dts->rx_sync_ctl = false;
		dts->rx_sync_domain = RX_SYNC_SYS_DOMAIN;
		dts->rx_sync_id = sunxi_rx_sync_probe(dts->rx_sync_domain);
		if (dts->rx_sync_id < 0) {
			SND_LOG_ERR("sunxi_rx_sync_probe failed\n");
		} else {
			SND_LOG_DEBUG("sunxi_rx_sync_probe successful. domain=%d, id=%d\n",
				      dts->rx_sync_domain, dts->rx_sync_id);
			ret = sunxi_rx_sync_register_cb(dts->rx_sync_domain, dts->rx_sync_id,
							(void *)regmap, sunxi_rx_sync_enable);
			if (ret)
				SND_LOG_ERR("callback register failed\n");
		}
	}

	/* component kcontrols -> loopback */
	if (quirks->loop_en) {
		ret = snd_soc_add_component_controls(component, sunxi_loopback_controls,
						     ARRAY_SIZE(sunxi_loopback_controls));
		if (ret)
			SND_LOG_ERR_STD(E_OWA_SWSYS_COMP_PROBE, "add loopback kcontrols failed\n");
	}

	/* component kcontrols -> tx_raw */
	ret = snd_soc_add_component_controls(component, sunxi_owa_tx_raw_controls,
					     ARRAY_SIZE(sunxi_owa_tx_raw_controls));
	if (ret)
		SND_LOG_ERR_STD(E_OWA_SWSYS_COMP_PROBE, "add tx_raw kcontrols failed\n");

	/* component kcontrols -> rx_raw */
	ret = sunxi_add_rx_raw_controls(component);
	if (ret)
		SND_LOG_ERR("add rx_raw kcontrols failed\n");

	/* component kcontrols -> debug */
	ret = snd_soc_add_component_controls(component, sunxi_debug_controls,
					     ARRAY_SIZE(sunxi_debug_controls));
	if (ret)
		SND_LOG_ERR_STD(E_OWA_SWSYS_COMP_PROBE, "add debug kcontrols failed\n");

	return 0;
}

static void sunxi_owa_component_remove(struct snd_soc_component *component)
{
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	const struct sunxi_owa_quirks *quirks = owa->quirks;
	struct sunxi_owa_dts *dts = &owa->dts;

	SND_LOG_DEBUG("\n");

	if (quirks->rx_sync_en && dts->rx_sync_en)
		sunxi_rx_sync_unregister_cb(dts->rx_sync_domain, dts->rx_sync_id);
}

static int sunxi_owa_component_suspend(struct snd_soc_component *component)
{
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct sunxi_owa_pinctl *pin = &owa->pin;
	struct regmap *regmap = owa->mem.regmap;

	SND_LOG_DEBUG("\n");

	snd_sunxi_save_reg(regmap, &sunxi_reg_group);

	if (pin->pinctrl_used)
		pinctrl_select_state(pin->pinctrl, pin->pinstate_sleep);
	regmap_update_bits(regmap, SUNXI_OWA_CTL, 1 << CTL_GEN_EN, 0 << CTL_GEN_EN);
	snd_owa_clk_bus_disable(owa->clk);
	snd_sunxi_regulator_disable(owa->rglt);

	return 0;
}

static int sunxi_owa_component_resume(struct snd_soc_component *component)
{
	struct sunxi_owa *owa = snd_soc_component_get_drvdata(component);
	struct sunxi_owa_pinctl *pin = &owa->pin;
	struct regmap *regmap = owa->mem.regmap;
	int ret;

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_regulator_enable(owa->rglt);
	if (ret) {
		SND_LOG_ERR("regulator enable failed\n");
		return ret;
	}
	ret = snd_owa_clk_bus_enable(owa->clk);
	if (ret) {
		SND_LOG_ERR("clk_bus and clk_rst enable failed\n");
		return ret;
	}

	regmap_update_bits(regmap, SUNXI_OWA_CTL, 1 << CTL_GEN_EN, 1 << CTL_GEN_EN);
	if (pin->pinctrl_used)
		pinctrl_select_state(pin->pinctrl, pin->pinstate);

	sunxi_owa_init(owa);
	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);

	return 0;
}

static struct snd_soc_component_driver sunxi_owa_component = {
	.name		= DRV_NAME,
	.probe		= sunxi_owa_component_probe,
	.remove		= sunxi_owa_component_remove,
	.suspend	= sunxi_owa_component_suspend,
	.resume		= sunxi_owa_component_resume,
};

/*******************************************************************************
 * *** kernel source ***
 * @0 regmap
 * @1 clk
 * @2 regulator
 * @3 dts params
 ******************************************************************************/
static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_owa_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	ret = of_address_to_resource(np, 0, &mem->res);
	if (ret) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_MEM_INIT, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res), DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_MEM_INIT, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev, mem->memregion->start,
				    resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_MEM_INIT, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &sunxi_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_MEM_INIT, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start,
				resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_owa_mem *mem)
{
	SND_LOG_DEBUG("\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start,
				resource_size(mem->memregion));
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_owa_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* get dma params */
	ret = of_property_read_u32(np, "playback-cma", &temp_val);
	if (ret < 0) {
		dts->playback_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("playback-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts->playback_cma = temp_val;
	}
	ret = of_property_read_u32(np, "capture-cma", &temp_val);
	if (ret != 0) {
		dts->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("capture-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts->capture_cma = temp_val;
	}
	ret = of_property_read_u32(np, "tx-fifo-size", &temp_val);
	if (ret != 0) {
		dts->playback_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("tx-fifo-size miss, using default value\n");
	} else {
		dts->playback_fifo_size = temp_val;
	}
	ret = of_property_read_u32(np, "rx-fifo-size", &temp_val);
	if (ret != 0) {
		dts->capture_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("rx-fifo-size miss,using default value\n");
	} else {
		dts->capture_fifo_size = temp_val;
	}

	SND_LOG_DEBUG("playback-cma : %zu\n", dts->playback_cma);
	SND_LOG_DEBUG("capture-cma  : %zu\n", dts->capture_cma);
	SND_LOG_DEBUG("tx-fifo-size : %zu\n", dts->playback_fifo_size);
	SND_LOG_DEBUG("rx-fifo-size : %zu\n", dts->capture_fifo_size);

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

	/* components func -> rx_sync */
	dts->rx_sync_en = of_property_read_bool(np, "rx-sync-en");

	/* clk fs */
	ret = of_property_read_u32(np, "pll-fs", &temp_val);
	if (ret < 0) {
		dts->pll_fs = 1;
	} else {
		dts->pll_fs = temp_val;
	}
}

static int snd_sunxi_pin_init(struct platform_device *pdev, struct sunxi_owa_pinctl *pin)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	if (of_property_read_bool(np, "pinctrl-used")) {
		pin->pinctrl_used = 1;
	} else {
		pin->pinctrl_used = 0;
		SND_LOG_DEBUG("unused pinctrl\n");
		return 0;
	}

	pin->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin->pinctrl)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_PIN_INIT, "pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin->pinstate = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin->pinstate)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_PIN_INIT, "pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin->pinstate_sleep = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin->pinstate_sleep)) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_PIN_INIT, "pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin->pinctrl, pin->pinstate);
	if (ret < 0) {
		SND_LOG_ERR_STD(E_OWA_SWDEP_PIN_INIT, "owa set pinctrl default state fail\n");
		ret = -EBUSY;
		goto err_pinctrl_select_default;
	}

	return 0;

err_pinctrl_select_default:
err_loopup_pin_sleep:
err_loopup_pinstate:
	devm_pinctrl_put(pin->pinctrl);
	return ret;
}

static void snd_sunxi_pin_exit(struct platform_device *pdev, struct sunxi_owa_pinctl *pin)
{
	SND_LOG_DEBUG("\n");

	if (pin->pinctrl_used)
		devm_pinctrl_put(pin->pinctrl);
}

static int snd_sunxi_owa_irq_init(struct platform_device *pdev, struct sunxi_owa *owa)
{
	int ret = 0;
	struct sunxi_owa_irq *owa_irq = &owa->owa_irq;

	SND_LOG_DEBUG("\n");

	INIT_DELAYED_WORK(&owa_irq->lock_confirm_work, sunxi_owa_lock_confirm_work);

	owa_irq->id = platform_get_irq(pdev, 0);
	if (owa_irq->id < 0) {
		SND_LOG_WARN("owa platform_get_irq failed\n");
		owa_irq->id = 0;
		return -ENODEV;
	}

	ret = request_irq(owa_irq->id, owa_interrupt, IRQF_TRIGGER_HIGH, "owa irq", owa);
	if (ret < 0) {
		SND_LOG_WARN("owa request_irq failed\n");
		owa_irq->id = 0;
		return -1;
	}

	return ret;
}

static void snd_sunxi_owa_irq_exit(struct sunxi_owa_irq *owa_irq)
{
	struct sunxi_owa *owa = container_of(owa_irq, struct sunxi_owa, owa_irq);
	SND_LOG_DEBUG("\n");

	if (owa_irq->id) {
		free_irq(owa_irq->id, owa);
		cancel_delayed_work(&owa_irq->lock_confirm_work);
	}
}

static void snd_sunxi_dma_params_init(struct sunxi_owa *owa)
{
	struct resource *res = &owa->mem.res;
	struct sunxi_owa_dts *dts = &owa->dts;

	SND_LOG_DEBUG("\n");

	owa->playback_dma_param.src_maxburst = 8;
	owa->playback_dma_param.dst_maxburst = 8;
	owa->playback_dma_param.dma_addr = res->start + SUNXI_OWA_TXFIFO;
	owa->playback_dma_param.cma_kbytes = dts->playback_cma;
	owa->playback_dma_param.fifo_size = dts->playback_fifo_size;

	owa->capture_dma_param.src_maxburst = 8;
	owa->capture_dma_param.dst_maxburst = 8;
	owa->capture_dma_param.dma_addr = res->start + SUNXI_OWA_RXFIFO;
	owa->capture_dma_param.cma_kbytes = dts->capture_cma;
	owa->capture_dma_param.fifo_size = dts->capture_fifo_size;
};

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
/* sysfs debug */
static void snd_sunxi_dump_version(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_owa *owa = (struct sunxi_owa *)priv;

	if (!owa) {
		SND_LOG_ERR_STD(E_OWA_SWARG_DUMP_VER, "priv to owa failed\n");
		return;
	}
	if (owa->pdev)
		if (owa->pdev->dev.driver)
			if (owa->pdev->dev.driver->owner)
				goto module_version;
	return;

module_version:
	owa->module_version = owa->pdev->dev.driver->owner->version;
	count_tmp += sprintf(buf + count_tmp, "%s\n", owa->module_version);

	*count = count_tmp;
}

static void snd_sunxi_dump_help(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;

	count_tmp += sprintf(buf + count_tmp, "1. reg read : echo {num} > dump && cat dump\n");
	count_tmp += sprintf(buf + count_tmp, "num: 0(all)\n");
	count_tmp += sprintf(buf + count_tmp, "2. reg write: echo {reg} {value} > dump\n");
	count_tmp += sprintf(buf + count_tmp, "eg. echo 0x00 0xaa > dump\n");

	*count = count_tmp;
}

static int snd_sunxi_dump_show(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_owa *owa = (struct sunxi_owa *)priv;
	unsigned int i = 0;
	unsigned int output_reg_val;
	struct regmap *regmap;

	if (!owa) {
		SND_LOG_ERR_STD(E_OWA_SWARG_DUMP_SHOW, "priv to owa failed\n");
		return -1;
	}
	if (!owa->show_reg_all)
		return 0;
	else
		owa->show_reg_all = false;

	regmap = owa->mem.regmap;
	for (i = 0; i < ARRAY_SIZE(sunxi_reg_labels); ++i) {
		regmap_read(regmap, sunxi_reg_labels[i].address, &output_reg_val);
		count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
				     sunxi_reg_labels[i].address, output_reg_val);
	}

	*count = count_tmp;

	return 0;
}

static int snd_sunxi_dump_store(void *priv, const char *buf, size_t count)
{
	struct sunxi_owa *owa = (struct sunxi_owa *)priv;
	int scanf_cnt;
	unsigned int input_reg_offset, input_reg_val, output_reg_val;
	struct regmap *regmap;

	if (count <= 1)	/* null or only "\n" */
		return 0;
	if (!owa) {
		SND_LOG_ERR_STD(E_OWA_SWARG_DUMP_STORE, "priv to owa failed\n");
		return -1;
	}
	regmap = owa->mem.regmap;

	if (!strcmp(buf, "0\n")) {
		owa->show_reg_all = true;
		return 0;
	}

	scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	if (scanf_cnt != 2) {
		pr_err("wrong format: %s\n", buf);
		return -1;
	}
	if (input_reg_offset > SUNXI_OWA_REG_MAX) {
		pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_OWA_REG_MAX);
		return -1;
	}
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
	regmap_write(regmap, input_reg_offset, input_reg_val);
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);

	return 0;
}
#endif

static int sunxi_owa_dev_probe(struct platform_device *pdev)
{
	struct sunxi_adapt_dai_ops_priv priv;
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_owa *owa;
	struct sunxi_owa_mem *mem;
	struct sunxi_owa_pinctl *pin;
	struct sunxi_owa_dts *dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump;
#endif
	const struct sunxi_owa_quirks *quirks;

	SND_LOG_DEBUG("\n");

	/* sunxi owa info */
	owa = devm_kzalloc(dev, sizeof(*owa), GFP_KERNEL);
	if (!owa) {
		SND_LOG_ERR_STD(E_OWA_SWSYS_PLAT_PROBE, "can't allocate sunxi owa memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, owa);
	mem = &owa->mem;
	pin = &owa->pin;
	dts = &owa->dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	dump = &owa->dump;
#endif
	owa->pdev = pdev;

	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR("remap init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_mem_init;
	}

	owa->clk = snd_owa_clk_init(pdev);
	if (!owa->clk) {
		SND_LOG_ERR("clk init failed\n");
		ret = -EINVAL;
		goto err_snd_owa_clk_init;
	}

	ret = snd_owa_clk_bus_enable(owa->clk);
	if (ret) {
		SND_LOG_ERR("clk_bus and clk_rst enable failed\n");
		ret = -EINVAL;
		goto err_clk_bus_enable;
	}

	owa->rglt = snd_sunxi_regulator_init(dev);
	if (!owa->rglt) {
		SND_LOG_ERR("rglt init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_rglt_init;
	}

	snd_sunxi_dts_params_init(pdev, dts);

	ret = snd_sunxi_pin_init(pdev, pin);
	if (ret) {
		SND_LOG_ERR("pinctrl init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_pin_init;
	}

	snd_sunxi_dma_params_init(owa);

	quirks = of_device_get_match_data(&pdev->dev);
	if (quirks == NULL) {
		SND_LOG_ERR_STD(E_OWA_SWSYS_PLAT_PROBE, "quirks get failed\n");
		return -ENODEV;
	}
	owa->quirks = quirks;

	priv.probe = sunxi_owa_dai_probe;
	priv.remove = sunxi_owa_dai_remove;
	sunxi_adpt_set_dai_ops(&sunxi_owa_dai, &sunxi_owa_dai_ops, &priv);

	ret = snd_soc_register_component(&pdev->dev,
					 &sunxi_owa_component,
					 &sunxi_owa_dai, 1);
	if (ret) {
		SND_LOG_ERR_STD(E_OWA_SWSYS_PLAT_PROBE, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_sunxi_dma_platform_register(&pdev->dev);
	if (ret) {
		SND_LOG_ERR("register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_platform_register;
	}

	ret = snd_sunxi_owa_irq_init(pdev, owa);
	if (ret)
		SND_LOG_WARN("irq init failed\n");

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snprintf(owa->module_name, 32, "%s", "OWA");
	dump->name = owa->module_name;
	dump->priv = owa;
	dump->dump_version = snd_sunxi_dump_version;
	dump->dump_help = snd_sunxi_dump_help;
	dump->dump_show = snd_sunxi_dump_show;
	dump->dump_store = snd_sunxi_dump_store;
	ret = snd_sunxi_dump_register(dump);
	if (ret)
		SND_LOG_WARN("snd_sunxi_dump_register failed\n");
#endif

	SND_LOG_DEBUG("register owa platform success\n");

	return 0;

err_snd_sunxi_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
err_snd_sunxi_pin_init:
	snd_sunxi_regulator_exit(owa->rglt);
err_snd_sunxi_rglt_init:
err_clk_bus_enable:
	snd_owa_clk_exit(owa->clk);
err_snd_owa_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_snd_sunxi_mem_init:
	devm_kfree(dev, owa);
err_devm_kzalloc:
	of_node_put(np);
	return ret;
}

static int sunxi_owa_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_owa *owa = dev_get_drvdata(&pdev->dev);
	struct sunxi_owa_mem *mem = &owa->mem;
	struct sunxi_owa_pinctl *pin = &owa->pin;
	struct sunxi_owa_dts *dts = &owa->dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump = &owa->dump;
#endif
	struct sunxi_owa_irq *owa_irq = &owa->owa_irq;

	/* remove components */
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snd_sunxi_dump_unregister(dump);
#endif
	if (dts->rx_sync_en) {
		sunxi_rx_sync_remove(dts->rx_sync_domain);
	}

	snd_sunxi_dma_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	snd_sunxi_regulator_exit(owa->rglt);
	snd_owa_clk_bus_disable(owa->clk);
	snd_owa_clk_exit(owa->clk);
	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_pin_exit(pdev, pin);
	snd_sunxi_owa_irq_exit(owa_irq);

	devm_kfree(dev, owa);
	of_node_put(np);

	SND_LOG_DEBUG("unregister owa platform success\n");

	return 0;
}

static const struct sunxi_owa_quirks sunxi_owa_quirks = {
	.fifo_ctl_rxtl = FIFO_CTL_RXTL,
	.fifo_ctl_txtl = FIFO_CTL_TXTL,
	.fifo_ctl_frx = FIFO_CTL_FRX,
	.fifo_ctl_ftx = FIFO_CTL_FTX,
	.ctl_txtl_mask = CTL_TXTL_MASK,
	.ctl_rxtl_mask = CTL_RXTL_MASK,
	.ctl_txtl_default = CTL_TXTL_DEFAULT,
	.ctl_rxtl_default = CTL_RXTL_DEFAULT,
	.loop_en = true,
	.rx_sync_en = true,
};

static const struct sunxi_owa_quirks sun8iw11_owa_quirks = {
	.fifo_ctl_rxtl = SUN8IW11_FIFO_CTL_RXTL,
	.fifo_ctl_txtl = SUN8IW11_FIFO_CTL_TXTL,
	.fifo_ctl_frx = SUN8IW11_FIFO_CTL_FRX,
	.fifo_ctl_ftx = SUN8IW11_FIFO_CTL_FTX,
	.ctl_txtl_mask = SUN8IW11_CTL_TXTL_MASK,
	.ctl_rxtl_mask = SUN8IW11_CTL_RXTL_MASK,
	.ctl_txtl_default = SUN8IW11_CTL_TXTL_DEFAULT,
	.ctl_rxtl_default = SUN8IW11_CTL_RXTL_DEFAULT,
	.loop_en = false,
	.rx_sync_en = false,
};

static const struct of_device_id sunxi_owa_of_match[] = {
	{
		.compatible = "allwinner," DRV_NAME,
		.data = &sunxi_owa_quirks,
	},
	{
		.compatible = "allwinner,sun8iw11-owa",
		.data = &sun8iw11_owa_quirks,
	},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_owa_of_match);

static struct platform_driver sunxi_owa_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_owa_of_match,
	},
	.probe	= sunxi_owa_dev_probe,
	.remove	= sunxi_owa_dev_remove,
};

int __init sunxi_owa_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_owa_driver);
	if (ret != 0) {
		SND_LOG_ERR_STD(E_OWA_SWSYS_MOD_INIT, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_owa_dev_exit(void)
{
	platform_driver_unregister(&sunxi_owa_driver);
}

late_initcall(sunxi_owa_dev_init);
module_exit(sunxi_owa_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.11");
MODULE_DESCRIPTION("sunxi soundcard platform of owa");

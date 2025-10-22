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

#define SUNXI_MODNAME		"sound-i2s"
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
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_i2s.h"
#include "snd_sunxi_adapter.h"


#define DRV_NAME	"sunxi-snd-plat-i2s"

/* for reg debug */
static unsigned int audio_reg_addrs[] = {
	SUNXI_I2S_CTL,
	SUNXI_I2S_FMT0,
	SUNXI_I2S_FMT1,
	SUNXI_I2S_INTSTA,
	/* SUNXI_I2S_RXFIFO */
	SUNXI_I2S_FIFOCTL,
	SUNXI_I2S_FIFOSTA,
	SUNXI_I2S_INTCTL,
	/* SUNXI_I2S_TXFIFO */
	SUNXI_I2S_CLKDIV,
	SUNXI_I2S_TXCNT,
	SUNXI_I2S_RXCNT,

	SUNXI_I2S_CHCFG,
	SUNXI_I2S_TX0CHSEL,
	SUNXI_I2S_TX1CHSEL,
	SUNXI_I2S_TX2CHSEL,
	SUNXI_I2S_TX3CHSEL,
	SUNXI_I2S_TX0CHMAP0,
	SUNXI_I2S_TX0CHMAP1,
	SUNXI_I2S_TX1CHMAP0,
	SUNXI_I2S_TX1CHMAP1,
	SUNXI_I2S_TX2CHMAP0,
	SUNXI_I2S_TX2CHMAP1,
	SUNXI_I2S_TX3CHMAP0,
	SUNXI_I2S_TX3CHMAP1,
	SUNXI_I2S_RXCHSEL,
	SUNXI_I2S_RXCHMAP0,
	SUNXI_I2S_RXCHMAP1,
	SUNXI_I2S_RXCHMAP2,
	SUNXI_I2S_RXCHMAP3,

	SUNXI_I2S_DEBUG,
	SUNXI_I2S_REV,
};

static unsigned int sun8iw11_reg_addrs[] = {
	SUNXI_I2S_CTL,
	SUNXI_I2S_FMT0,
	SUNXI_I2S_FMT1,
	SUNXI_I2S_INTSTA,
	/* SUNXI_I2S_RXFIFO */
	SUNXI_I2S_FIFOCTL,
	SUNXI_I2S_FIFOSTA,
	SUNXI_I2S_INTCTL,
	/* SUNXI_I2S_TXFIFO */
	SUNXI_I2S_CLKDIV,
	SUNXI_I2S_TXCNT,
	SUNXI_I2S_RXCNT,

	SUNXI_I2S_8SLOT_CHCFG,
	SUNXI_I2S_8SLOT_TX0CHSEL,
	SUNXI_I2S_8SLOT_TX1CHSEL,
	SUNXI_I2S_8SLOT_TX2CHSEL,
	SUNXI_I2S_8SLOT_TX3CHSEL,
	SUNXI_I2S_8SLOT_TX0CHMAP,
	SUNXI_I2S_8SLOT_TX1CHMAP,
	SUNXI_I2S_8SLOT_TX2CHMAP,
	SUNXI_I2S_8SLOT_TX3CHMAP,
	SUNXI_I2S_8SLOT_RXCHSEL,
	SUNXI_I2S_8SLOT_RXCHMAP,
};

static unsigned int sun8iw17_reg_addrs[] = {
	SUNXI_I2S_CTL,
	SUNXI_I2S_FMT0,
	SUNXI_I2S_FMT1,
	SUNXI_I2S_INTSTA,
	/* SUNXI_I2S_RXFIFO */
	SUNXI_I2S_FIFOCTL,
	SUNXI_I2S_FIFOSTA,
	SUNXI_I2S_INTCTL,
	/* SUNXI_I2S_TXFIFO */
	SUNXI_I2S_CLKDIV,
	SUNXI_I2S_TXCNT,
	SUNXI_I2S_RXCNT,

	SUNXI_I2S_CHCFG,
	SUNXI_I2S_TX0CHSEL,
	SUNXI_I2S_TX0CHMAP0,
	SUNXI_I2S_TX0CHMAP1,
	SUNXI_I2S_RXCHSEL,
	SUNXI_I2S_RXCHMAP0,
	SUNXI_I2S_RXCHMAP1,
};

static struct regmap_config g_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_I2S_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static int sunxi_get_i2s_dai_fmt(struct sunxi_i2s_dai_fmt *i2s_dai_fmt,
				 enum SUNXI_I2S_DAI_FMT_SEL dai_fmt_sel, unsigned int *val);
static int sunxi_set_i2s_dai_fmt(struct sunxi_i2s_dai_fmt *i2s_dai_fmt,
				 enum SUNXI_I2S_DAI_FMT_SEL dai_fmt_sel, unsigned int val);

static void sunxi_rx_sync_enable(void *data, bool enable);

static void sunxi_i2s_get_dai_ucfmt(struct snd_notifier_block *snd_nb)
{
	struct sunxi_i2s *i2s;
	struct snd_sunxi_dai_ucfmt *fmt_data;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt;

	if (!snd_nb) {
		SND_LOG_ERR_STD(E_I2S_SWARG_UCFMT_CB, "snd_nb is null\n");
		return;
	}

	i2s = (struct sunxi_i2s *)snd_nb->cb_data;
	fmt_data = (struct snd_sunxi_dai_ucfmt *)snd_nb->tx_data;
	i2s_dai_fmt = &i2s->i2s_dai_fmt;

	i2s_dai_fmt->data_late = fmt_data->data_late;
	i2s_dai_fmt->tx_lsb_first = fmt_data->tx_lsb_first;
	i2s_dai_fmt->rx_lsb_first = fmt_data->rx_lsb_first;
}

static void sunxi_i2s_get_hdmi_fmt(struct snd_notifier_block *snd_nb)
{
	struct sunxi_i2s *i2s;
	enum HDMI_FORMAT *hdmi_fmt;

	if (!snd_nb) {
		SND_LOG_ERR_STD(E_I2S_SWARG_UCFMT_CB, "snd_nb is null\n");
		return;
	}

	i2s = (struct sunxi_i2s *)snd_nb->cb_data;
	hdmi_fmt = (enum HDMI_FORMAT *)snd_nb->tx_data;

	if (i2s->dts.dai_type == SUNXI_DAI_HDMI_TYPE) {
		i2s->playback_dma_param.hdmi_fmt = *hdmi_fmt;
		SND_LOG_DEBUG("hdmi fmt -> %d\n", i2s->playback_dma_param.hdmi_fmt);
	}
}

static int sunxi_i2s_set_ch_en(struct sunxi_i2s *i2s, int stream, unsigned int channels)
{
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	unsigned int slot_en_num;
	uint32_t channels_en_slot[16] = {
		0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
		0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
	};
	int ret;

	if (IS_ERR_OR_NULL(i2s) || channels < 1 || channels > 16)
		return -EINVAL;

	ret = sunxi_get_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_SLOT_NUM, &slot_en_num);
	if (ret < 0)
		return -EINVAL;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_I2S_CHCFG, 0xF << TX_SLOT_NUM,
				   (channels - 1) << TX_SLOT_NUM);

		if (dts->tx_pin[0]) {
			regmap_update_bits(regmap, SUNXI_I2S_TX0CHSEL, 0xF << TX_CHSEL,
					   (slot_en_num - 1) << TX_CHSEL);
			regmap_update_bits(regmap, SUNXI_I2S_TX0CHSEL, 0xFFFF << TX_CHEN,
					   channels_en_slot[slot_en_num - 1] << TX_CHEN);
		}
		if (dts->tx_pin[1]) {
			regmap_update_bits(regmap, SUNXI_I2S_TX1CHSEL, 0xF << TX_CHSEL,
					   (slot_en_num - 1) << TX_CHSEL);
			regmap_update_bits(regmap, SUNXI_I2S_TX1CHSEL, 0xFFFF << TX_CHEN,
					   channels_en_slot[slot_en_num - 1] << TX_CHEN);
		}
		if (dts->tx_pin[2]) {
			regmap_update_bits(regmap, SUNXI_I2S_TX2CHSEL, 0xF << TX_CHSEL,
					   (slot_en_num - 1) << TX_CHSEL);
			regmap_update_bits(regmap, SUNXI_I2S_TX2CHSEL, 0xFFFF << TX_CHEN,
					   channels_en_slot[slot_en_num - 1] << TX_CHEN);
		}
		if (dts->tx_pin[3]) {
			regmap_update_bits(regmap, SUNXI_I2S_TX3CHSEL, 0xF << TX_CHSEL,
					   (slot_en_num - 1) << TX_CHSEL);
			regmap_update_bits(regmap, SUNXI_I2S_TX3CHSEL, 0xFFFF << TX_CHEN,
					   channels_en_slot[slot_en_num - 1] << TX_CHEN);
		}
	} else {
		regmap_update_bits(regmap, SUNXI_I2S_CHCFG, 0xF << RX_SLOT_NUM,
				   (channels - 1) << RX_SLOT_NUM);
		regmap_update_bits(regmap, SUNXI_I2S_RXCHSEL, 0xF << RX_CHSEL,
				   (channels - 1) << RX_CHSEL);
	}

	return 0;
}

static int sun8iw11_i2s_set_ch_en(struct sunxi_i2s *i2s, int stream, unsigned int channels)
{
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	unsigned int slot_en_num;
	uint32_t channels_en_slot[8] = {
		0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
	};
	int ret;

	if (IS_ERR_OR_NULL(i2s) || channels < 1 || channels > 8)
		return -EINVAL;

	ret = sunxi_get_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_SLOT_NUM, &slot_en_num);
	if (ret < 0)
		return -EINVAL;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_I2S_CHCFG, 0x7 << TX_SLOT_NUM,
				   (channels - 1) << TX_SLOT_NUM);

		if (dts->tx_pin[0]) {
			regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX0CHSEL,
					   0x7 << TX_CHSEL_8SLOT,
					   (slot_en_num - 1) << TX_CHSEL_8SLOT);
			regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX0CHSEL,
					   0xFF << TX_CHEN_8SLOT,
					   channels_en_slot[slot_en_num - 1] << TX_CHEN_8SLOT);
		}
		if (dts->tx_pin[1]) {
			regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX1CHSEL,
					   0x7 << TX_CHSEL_8SLOT,
					   (slot_en_num - 1) << TX_CHSEL_8SLOT);
			regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX1CHSEL,
					   0xFF << TX_CHEN_8SLOT,
					   channels_en_slot[slot_en_num - 1] << TX_CHEN_8SLOT);
		}
		if (dts->tx_pin[2]) {
			regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX2CHSEL,
					   0x7 << TX_CHSEL_8SLOT,
					   (slot_en_num - 1) << TX_CHSEL_8SLOT);
			regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX2CHSEL,
					   0xFF << TX_CHEN_8SLOT,
					   channels_en_slot[slot_en_num - 1] << TX_CHEN_8SLOT);
		}
		if (dts->tx_pin[3]) {
			regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX3CHSEL,
					   0x7 << TX_CHSEL_8SLOT,
					   (slot_en_num - 1) << TX_CHSEL_8SLOT);
			regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX3CHSEL,
					   0xFF << TX_CHEN_8SLOT,
					   channels_en_slot[slot_en_num - 1] << TX_CHEN_8SLOT);
		}
	} else {
		regmap_update_bits(regmap, SUNXI_I2S_8SLOT_CHCFG, 0x7 << RX_SLOT_NUM_8SLOT,
				   (channels - 1) << RX_SLOT_NUM_8SLOT);
		regmap_update_bits(regmap, SUNXI_I2S_8SLOT_RXCHSEL, 0x7 << RX_CHSEL_8SLOT,
				   (channels - 1) << RX_CHSEL_8SLOT);
	}

	return 0;
}

static int sun8iw17_i2s_set_ch_en(struct sunxi_i2s *i2s, int stream, unsigned int channels)
{
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	const struct sunxi_i2s_quirks *quirks = i2s->quirks;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	unsigned int slot_en_num;
	unsigned int i;
	uint32_t channels_en_slot[16] = {
		0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
		0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
	};
	int ret;

	if (IS_ERR_OR_NULL(i2s) || channels < 1 || channels > 16)
		return -EINVAL;

	ret = sunxi_get_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_SLOT_NUM, &slot_en_num);
	if (ret < 0)
		return -EINVAL;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (dts->tx_pin[1] || dts->tx_pin[2] || dts->tx_pin[3])
			SND_LOG_WARN("only support DOUT0\n");

		regmap_update_bits(regmap, SUNXI_I2S_CHCFG, 0xF << TX_SLOT_NUM,
				   (channels - 1) << TX_SLOT_NUM);

		if (dts->tx_pin[0]) {
			regmap_update_bits(regmap, SUNXI_I2S_TX0CHSEL, 0xF << TX_CHSEL,
					   (slot_en_num - 1) << TX_CHSEL);
			regmap_update_bits(regmap, SUNXI_I2S_TX0CHSEL, 0xFFFF << TX_CHEN,
					   channels_en_slot[slot_en_num - 1] << TX_CHEN);
		}
	} else {
		for (i = 0; i < quirks->slot_num_max; ++i) {
			if (dts->rxfifo_pinmap[i] != 0)
				SND_LOG_WARN("only support DIN0\n");
		}

		regmap_update_bits(regmap, SUNXI_I2S_CHCFG, 0xF << RX_SLOT_NUM,
				   (channels - 1) << RX_SLOT_NUM);
		regmap_update_bits(regmap, SUNXI_I2S_RXCHSEL, 0xF << RX_CHSEL,
				   (channels - 1) << RX_CHSEL);
	}

	return 0;
}

static int sunxi_i2s_set_daifmt_fmt(struct sunxi_i2s *i2s, unsigned int format)
{
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	struct regmap *regmap = i2s->mem.regmap;
	unsigned int mode;

	if (IS_ERR_OR_NULL(i2s))
		return -EINVAL;

	switch (format) {
	case SND_SOC_DAIFMT_I2S:
		mode = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		mode = 2;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		mode = 1;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		mode = 0;
		/* L data MSB after FRM LRC (short frame) */
		regmap_update_bits(regmap, SUNXI_I2S_FMT0, 1 << LRCK_WIDTH, 0 << LRCK_WIDTH);
		break;
	case SND_SOC_DAIFMT_DSP_B:
		mode = 0;
		/* L data MSB during FRM LRC (long frame) */
		regmap_update_bits(regmap, SUNXI_I2S_FMT0, 1 << LRCK_WIDTH, 1 << LRCK_WIDTH);
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_SET, "format setting failed\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_I2S_CTL, 3 << MODE_SEL, mode << MODE_SEL);

	regmap_update_bits(regmap, SUNXI_I2S_TX0CHSEL,
			   3 << TX_OFFSET, i2s_dai_fmt->data_late << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_I2S_TX1CHSEL,
			   3 << TX_OFFSET, i2s_dai_fmt->data_late << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_I2S_TX2CHSEL,
			   3 << TX_OFFSET, i2s_dai_fmt->data_late << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_I2S_TX3CHSEL,
			   3 << TX_OFFSET, i2s_dai_fmt->data_late << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_I2S_RXCHSEL,
			   3 << RX_OFFSET, i2s_dai_fmt->data_late << RX_OFFSET);

	if (!i2s_dai_fmt->rx_lsb_first && !i2s_dai_fmt->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 0 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 0 << TX_MLS);
	} else if (i2s_dai_fmt->rx_lsb_first && !i2s_dai_fmt->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 1 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 0 << TX_MLS);
	} else if (!i2s_dai_fmt->rx_lsb_first && i2s_dai_fmt->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 0 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 1 << TX_MLS);
	} else {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 1 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 1 << TX_MLS);
	}

	return 0;
}

static int sun8iw11_i2s_set_daifmt_fmt(struct sunxi_i2s *i2s, unsigned int format)
{
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	struct regmap *regmap = i2s->mem.regmap;
	unsigned int mode;

	if (IS_ERR_OR_NULL(i2s))
		return -EINVAL;

	switch (format) {
	case SND_SOC_DAIFMT_I2S:
		mode = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		mode = 2;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		mode = 1;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		mode = 0;
		/* L data MSB after FRM LRC (short frame) */
		regmap_update_bits(regmap, SUNXI_I2S_FMT0, 1 << LRCK_WIDTH, 0 << LRCK_WIDTH);
		break;
	case SND_SOC_DAIFMT_DSP_B:
		mode = 0;
		/* L data MSB during FRM LRC (long frame) */
		regmap_update_bits(regmap, SUNXI_I2S_FMT0, 1 << LRCK_WIDTH, 1 << LRCK_WIDTH);
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_SET, "format setting failed\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_I2S_CTL, 3 << MODE_SEL, mode << MODE_SEL);

	regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX0CHSEL,
			   3 << 12, i2s_dai_fmt->data_late << 12);
	regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX1CHSEL,
			   3 << 12, i2s_dai_fmt->data_late << 12);
	regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX2CHSEL,
			   3 << 12, i2s_dai_fmt->data_late << 12);
	regmap_update_bits(regmap, SUNXI_I2S_8SLOT_TX3CHSEL,
			   3 << 12, i2s_dai_fmt->data_late << 12);
	regmap_update_bits(regmap, SUNXI_I2S_8SLOT_RXCHSEL,
			   3 << 12, i2s_dai_fmt->data_late << 12);

	if (!i2s_dai_fmt->rx_lsb_first && !i2s_dai_fmt->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 0 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 0 << TX_MLS);
	} else if (i2s_dai_fmt->rx_lsb_first && !i2s_dai_fmt->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 1 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 0 << TX_MLS);
	} else if (!i2s_dai_fmt->rx_lsb_first && i2s_dai_fmt->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 0 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 1 << TX_MLS);
	} else {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 1 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 1 << TX_MLS);
	}

	return 0;
}

static int sun8iw17_i2s_set_daifmt_fmt(struct sunxi_i2s *i2s, unsigned int format)
{
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	struct regmap *regmap = i2s->mem.regmap;
	unsigned int mode;

	if (IS_ERR_OR_NULL(i2s))
		return -EINVAL;

	switch (format) {
	case SND_SOC_DAIFMT_I2S:
		mode = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		mode = 2;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		mode = 1;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		mode = 0;
		/* L data MSB after FRM LRC (short frame) */
		regmap_update_bits(regmap, SUNXI_I2S_FMT0, 1 << LRCK_WIDTH, 0 << LRCK_WIDTH);
		break;
	case SND_SOC_DAIFMT_DSP_B:
		mode = 0;
		/* L data MSB during FRM LRC (long frame) */
		regmap_update_bits(regmap, SUNXI_I2S_FMT0, 1 << LRCK_WIDTH, 1 << LRCK_WIDTH);
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_SET, "format setting failed\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_I2S_CTL, 3 << MODE_SEL, mode << MODE_SEL);

	regmap_update_bits(regmap, SUNXI_I2S_TX0CHSEL,
			   3 << TX_OFFSET, i2s_dai_fmt->data_late << TX_OFFSET);
	regmap_update_bits(regmap, SUNXI_I2S_RXCHSEL,
			   3 << RX_OFFSET, i2s_dai_fmt->data_late << RX_OFFSET);

	if (!i2s_dai_fmt->rx_lsb_first && !i2s_dai_fmt->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 0 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 0 << TX_MLS);
	} else if (i2s_dai_fmt->rx_lsb_first && !i2s_dai_fmt->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 1 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 0 << TX_MLS);
	} else if (!i2s_dai_fmt->rx_lsb_first && i2s_dai_fmt->tx_lsb_first) {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 0 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 1 << TX_MLS);
	} else {
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << RX_MLS, 1 << RX_MLS);
		regmap_update_bits(regmap, SUNXI_I2S_FMT1, 1 << TX_MLS, 1 << TX_MLS);
	}

	return 0;
}

static int sunxi_i2s_set_ch_map(struct sunxi_i2s *i2s, unsigned int channels)
{
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	unsigned int reg_val_tx[8] = {0};
	unsigned int reg_val_rx[4] = {0};
	unsigned int i, j;

	(void)channels;

	if (IS_ERR_OR_NULL(i2s))
		return -EINVAL;

	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 8; ++j) {
			reg_val_tx[i << 1] |= (dts->tx_pin_chmap[i][j] << (j << 2));
			reg_val_tx[(i << 1) + 1] |= (dts->tx_pin_chmap[i][j + 8] << (j << 2));
		}
	}

	regmap_write(regmap, SUNXI_I2S_TX0CHMAP0, reg_val_tx[1]);
	regmap_write(regmap, SUNXI_I2S_TX0CHMAP1, reg_val_tx[0]);
	regmap_write(regmap, SUNXI_I2S_TX1CHMAP0, reg_val_tx[3]);
	regmap_write(regmap, SUNXI_I2S_TX1CHMAP1, reg_val_tx[2]);
	regmap_write(regmap, SUNXI_I2S_TX2CHMAP0, reg_val_tx[5]);
	regmap_write(regmap, SUNXI_I2S_TX2CHMAP1, reg_val_tx[4]);
	regmap_write(regmap, SUNXI_I2S_TX3CHMAP0, reg_val_tx[7]);
	regmap_write(regmap, SUNXI_I2S_TX3CHMAP1, reg_val_tx[6]);

	j = 0;
	for (i = 0 ; i < 16; ++i)
		reg_val_rx[j < (i / 4) ? (++j) : j] |=
			(dts->rxfifo_chmap[i] | (dts->rxfifo_pinmap[i] << 4)) << (8 * (i % 4));

	regmap_write(regmap, SUNXI_I2S_RXCHMAP0, reg_val_rx[3]);
	regmap_write(regmap, SUNXI_I2S_RXCHMAP1, reg_val_rx[2]);
	regmap_write(regmap, SUNXI_I2S_RXCHMAP2, reg_val_rx[1]);
	regmap_write(regmap, SUNXI_I2S_RXCHMAP3, reg_val_rx[0]);

	return 0;
}

static int sun8iw11_i2s_set_ch_map(struct sunxi_i2s *i2s, unsigned int channels)
{
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	unsigned int reg_val_tx[4] = {0};
	unsigned int reg_val_rx = 0;
	unsigned int i, j;

	if (IS_ERR_OR_NULL(i2s))
		return -EINVAL;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 8; j++) {
			reg_val_tx[i] |= (dts->tx_pin_chmap[i][j] << (j << 2));
		}
	}

	regmap_write(regmap, SUNXI_I2S_8SLOT_TX0CHMAP, reg_val_tx[0]);
	regmap_write(regmap, SUNXI_I2S_8SLOT_TX1CHMAP, reg_val_tx[1]);
	regmap_write(regmap, SUNXI_I2S_8SLOT_TX2CHMAP, reg_val_tx[2]);
	regmap_write(regmap, SUNXI_I2S_8SLOT_TX3CHMAP, reg_val_tx[3]);

	for (i = 0; i < 8; i++)
		reg_val_rx |= (dts->rxfifo_chmap[i] << (j << 2));

	regmap_write(regmap, SUNXI_I2S_8SLOT_RXCHMAP, reg_val_rx);

	return 0;
}

static int sun8iw17_i2s_set_ch_map(struct sunxi_i2s *i2s, unsigned int channels)
{
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	unsigned int reg_val_tx[2] = {0};
	unsigned int reg_val_rx[2] = {0};
	unsigned int i;

	(void)channels;

	if (IS_ERR_OR_NULL(i2s))
		return -EINVAL;

	for (i = 0; i < 8; ++i) {
		reg_val_tx[0] |= (dts->tx_pin_chmap[0][i] << (i << 2));
		reg_val_tx[1] |= (dts->tx_pin_chmap[0][i + 8] << (i << 2));
	}

	regmap_write(regmap, SUNXI_I2S_TX0CHMAP0, reg_val_tx[1]);
	regmap_write(regmap, SUNXI_I2S_TX0CHMAP1, reg_val_tx[0]);

	for (i = 0; i < 8; ++i) {
		reg_val_rx[0] |= (dts->rxfifo_chmap[i] << (i << 2));
		reg_val_rx[1] |= (dts->rxfifo_chmap[i + 8] << (i << 2));
	}

	regmap_write(regmap, SUNXI_I2S_RXCHMAP0, reg_val_rx[1]);
	regmap_write(regmap, SUNXI_I2S_RXCHMAP1, reg_val_rx[0]);

	return 0;
}

static int sunxi_get_i2s_dai_fmt(struct sunxi_i2s_dai_fmt *i2s_dai_fmt,
				 enum SUNXI_I2S_DAI_FMT_SEL dai_fmt_sel,
				 unsigned int *val)
{
	switch (dai_fmt_sel) {
	case SUNXI_I2S_DAI_PLL:
		*val = i2s_dai_fmt->pllclk_freq;
		break;
	case SUNXI_I2S_DAI_MCLK:
		*val = i2s_dai_fmt->moduleclk_freq;
		break;
	case SUNXI_I2S_DAI_FMT:
		*val = i2s_dai_fmt->fmt & SND_SOC_DAIFMT_FORMAT_MASK;
		break;
	case SUNXI_I2S_DAI_MASTER:
		*val = i2s_dai_fmt->fmt & SND_SOC_DAIFMT_MASTER_MASK;
		break;
	case SUNXI_I2S_DAI_INVERT:
		*val = i2s_dai_fmt->fmt & SND_SOC_DAIFMT_INV_MASK;
		break;
	case SUNXI_I2S_DAI_SLOT_NUM:
		*val = i2s_dai_fmt->slots;
		break;
	case SUNXI_I2S_DAI_SLOT_WIDTH:
		*val = i2s_dai_fmt->slot_width;
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_GET, "unsupport dai fmt sel %d\n", dai_fmt_sel);
		return -EINVAL;
	}

	return 0;
}

static int sunxi_set_i2s_dai_fmt(struct sunxi_i2s_dai_fmt *i2s_dai_fmt,
				 enum SUNXI_I2S_DAI_FMT_SEL dai_fmt_sel,
				 unsigned int val)
{
	switch (dai_fmt_sel) {
	case SUNXI_I2S_DAI_PLL:
		i2s_dai_fmt->pllclk_freq = val;
		break;
	case SUNXI_I2S_DAI_MCLK:
		i2s_dai_fmt->moduleclk_freq = val;
		break;
	case SUNXI_I2S_DAI_FMT:
		i2s_dai_fmt->fmt &= ~SND_SOC_DAIFMT_FORMAT_MASK;
		i2s_dai_fmt->fmt |= SND_SOC_DAIFMT_FORMAT_MASK & val;
		break;
	case SUNXI_I2S_DAI_MASTER:
		i2s_dai_fmt->fmt &= ~SND_SOC_DAIFMT_MASTER_MASK;
		i2s_dai_fmt->fmt |= SND_SOC_DAIFMT_MASTER_MASK & val;
		break;
	case SUNXI_I2S_DAI_INVERT:
		i2s_dai_fmt->fmt &= ~SND_SOC_DAIFMT_INV_MASK;
		i2s_dai_fmt->fmt |= SND_SOC_DAIFMT_INV_MASK & val;
		break;
	case SUNXI_I2S_DAI_SLOT_NUM:
		i2s_dai_fmt->slots = val;
		break;
	case SUNXI_I2S_DAI_SLOT_WIDTH:
		i2s_dai_fmt->slot_width = val;
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_SET, "unsupport dai fmt sel %d\n", dai_fmt_sel);
		return -EINVAL;
	}

	return 0;
}

static void sunxi_sdout_enable(struct regmap *regmap, bool *tx_pin)
{
	/* tx_pin[x] -- x < 4 */
	regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << (SDO0_EN + 0),
			   tx_pin[0] << (SDO0_EN + 0));
	regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << (SDO0_EN + 1),
			   tx_pin[1] << (SDO0_EN + 1));
	regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << (SDO0_EN + 2),
			   tx_pin[2] << (SDO0_EN + 2));
	regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << (SDO0_EN + 3),
			   tx_pin[3] << (SDO0_EN + 3));
}

static void sunxi_sdout_disable(struct regmap *regmap)
{
	regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << (SDO0_EN + 0), 0 << (SDO0_EN + 0));
	regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << (SDO0_EN + 1), 0 << (SDO0_EN + 0));
	regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << (SDO0_EN + 2), 0 << (SDO0_EN + 0));
	regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << (SDO0_EN + 3), 0 << (SDO0_EN + 0));
}

static int sunxi_i2s_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
				 unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	int ret;

	SND_LOG_DEBUG("\n");

	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_PLL, freq_in);
	if (ret < 0)
		return -EINVAL;
	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_MCLK, freq_out);
	if (ret < 0)
		return -EINVAL;

	return 0;
}

static int sunxi_i2s_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	unsigned int pllclk_freq, mclk_ratio, mclk_ratio_map;
	int ret;

	SND_LOG_DEBUG("\n");

	if (freq == 0) {
		regmap_update_bits(regmap, SUNXI_I2S_CLKDIV, 1 << MCLKOUT_EN, 0 << MCLKOUT_EN);
		return 0;
	}

	ret = sunxi_get_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_PLL, &pllclk_freq);
	if (ret < 0)
		return -EINVAL;

	if (pllclk_freq == 0) {
		SND_LOG_ERR_STD(E_I2S_SWARG_CLK_SET, "pllclk freq is invalid\n");
		return -ENOMEM;
	}
	mclk_ratio = pllclk_freq / freq;

	switch (mclk_ratio) {
	case 1:
		mclk_ratio_map = 1;
		break;
	case 2:
		mclk_ratio_map = 2;
		break;
	case 4:
		mclk_ratio_map = 3;
		break;
	case 6:
		mclk_ratio_map = 4;
		break;
	case 8:
		mclk_ratio_map = 5;
		break;
	case 12:
		mclk_ratio_map = 6;
		break;
	case 16:
		mclk_ratio_map = 7;
		break;
	case 24:
		mclk_ratio_map = 8;
		break;
	case 32:
		mclk_ratio_map = 9;
		break;
	case 48:
		mclk_ratio_map = 10;
		break;
	case 64:
		mclk_ratio_map = 11;
		break;
	case 96:
		mclk_ratio_map = 12;
		break;
	case 128:
		mclk_ratio_map = 13;
		break;
	case 176:
		mclk_ratio_map = 14;
		break;
	case 192:
		mclk_ratio_map = 15;
		break;
	default:
		regmap_update_bits(regmap, SUNXI_I2S_CLKDIV, 1 << MCLKOUT_EN, 0 << MCLKOUT_EN);
		SND_LOG_ERR_STD(E_I2S_SWARG_CLK_SET, "mclk freq div unsupport\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_I2S_CLKDIV,
			   0xf << MCLK_DIV, mclk_ratio_map << MCLK_DIV);
	regmap_update_bits(regmap, SUNXI_I2S_CLKDIV, 1 << MCLKOUT_EN, 1 << MCLKOUT_EN);

	return 0;
}

static int sunxi_i2s_dai_set_bclk_ratio(struct snd_soc_dai *dai, unsigned int ratio)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = i2s->mem.regmap;
	unsigned int bclk_ratio;

	SND_LOG_DEBUG("\n");

	/* ratio -> cpudai pllclk / pcm rate */
	switch (ratio) {
	case 1:
		bclk_ratio = 1;
		break;
	case 2:
		bclk_ratio = 2;
		break;
	case 4:
		bclk_ratio = 3;
		break;
	case 6:
		bclk_ratio = 4;
		break;
	case 8:
		bclk_ratio = 5;
		break;
	case 12:
		bclk_ratio = 6;
		break;
	case 16:
		bclk_ratio = 7;
		break;
	case 24:
		bclk_ratio = 8;
		break;
	case 32:
		bclk_ratio = 9;
		break;
	case 48:
		bclk_ratio = 10;
		break;
	case 64:
		bclk_ratio = 11;
		break;
	case 96:
		bclk_ratio = 12;
		break;
	case 128:
		bclk_ratio = 13;
		break;
	case 176:
		bclk_ratio = 14;
		break;
	case 192:
		bclk_ratio = 15;
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_CLK_SET, "bclk freq div unsupport\n");
		return -EINVAL;
	}

	regmap_update_bits(regmap, SUNXI_I2S_CLKDIV, 0xf << BCLK_DIV, bclk_ratio << BCLK_DIV);

	return 0;
}

static int sunxi_i2s_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	const struct sunxi_i2s_quirks *quirks = i2s->quirks;
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	unsigned int lrck_polarity, bclk_polarity;
	/* dai mode of i2s format
	 * I2S/RIGHT_J/LEFT_J	-> 0
	 * DSP_A/DSP_B		-> 1
	 */
	unsigned int dai_mode;
	int ret;

	SND_LOG_DEBUG("dai fmt -> 0x%x\n", fmt);

	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_FMT, fmt);
	if (ret < 0)
		return -EINVAL;
	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_MASTER, fmt);
	if (ret < 0)
		return -EINVAL;
	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_INVERT, fmt);
	if (ret < 0)
		return -EINVAL;

	/* get dai mode */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		dai_mode = 0;
		break;
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		dai_mode = 1;
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_SET, "dai_mode setting failed\n");
		return -EINVAL;
	}

	/* set TDM format */
	ret = quirks->set_daifmt_format(i2s, fmt & SND_SOC_DAIFMT_FORMAT_MASK);
	if (ret < 0) {
		SND_LOG_ERR("set daifmt format failed\n");
		return -EINVAL;
	}

	/* set lrck & bclk polarity */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		lrck_polarity = 0;
		bclk_polarity = 0;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		lrck_polarity = 1;
		bclk_polarity = 0;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		lrck_polarity = 0;
		bclk_polarity = 1;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		lrck_polarity = 1;
		bclk_polarity = 1;
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_SET, "invert clk setting failed\n");
		return -EINVAL;
	}

	/* lrck polarity of i2s format
	 * LRCK_POLARITY	-> 0
	 * Left channel when LRCK is low(I2S/RIGHT_J/LEFT_J);
	 * PCM LRCK asserted at the negative edge(DSP_A/DSP_B);
	 * LRCK_POLARITY	-> 1
	 * Left channel when LRCK is high(I2S/RIGHT_J/LEFT_J);
	 * PCM LRCK asserted at the positive edge(DSP_A/DSP_B);
	 */
	lrck_polarity ^= dai_mode;

	regmap_update_bits(regmap, SUNXI_I2S_FMT0,
			   1 << LRCK_POLARITY,
			   lrck_polarity << LRCK_POLARITY);
	regmap_update_bits(regmap, SUNXI_I2S_FMT0,
			   1 << BCLK_POLARITY,
			   bclk_polarity << BCLK_POLARITY);

	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	/* bclk & lrck dir input */
	case SND_SOC_DAIFMT_CBM_CFM:
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << BCLK_OUT, 0 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << LRCK_OUT, 0 << LRCK_OUT);
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << BCLK_OUT, 1 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << LRCK_OUT, 0 << LRCK_OUT);
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << BCLK_OUT, 0 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << LRCK_OUT, 1 << LRCK_OUT);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << BCLK_OUT, 1 << BCLK_OUT);
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << LRCK_OUT, 1 << LRCK_OUT);
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_SET, "unknown master/slave format\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_i2s_dai_set_tdm_slot(struct snd_soc_dai *dai,
				      unsigned int tx_mask, unsigned int rx_mask,
				      int slots, int slot_width)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	unsigned int slot_width_map, lrck_width_map;
	unsigned int dai_fmt_get;
	int ret;

	SND_LOG_DEBUG("\n");

	switch (slot_width) {
	case 8:
		slot_width_map = 1;
		break;
	case 12:
		slot_width_map = 2;
		break;
	case 16:
		slot_width_map = 3;
		break;
	case 20:
		slot_width_map = 4;
		break;
	case 24:
		slot_width_map = 5;
		break;
	case 28:
		slot_width_map = 6;
		break;
	case 32:
		slot_width_map = 7;
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_SET, "unknown slot width\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_I2S_FMT0,
			   7 << SLOT_WIDTH, slot_width_map << SLOT_WIDTH);

	/* bclk num of per channel
	 * I2S/RIGHT_J/LEFT_J	-> lrck long total is lrck_width_map * 2
	 * DSP_A/DSP_B		-> lrck long total is lrck_width_map * 1
	 */
	ret = sunxi_get_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_FMT, &dai_fmt_get);
	if (ret < 0)
		return -EINVAL;
	switch (dai_fmt_get) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		lrck_width_map = (slots / 2) * slot_width - 1;
		break;
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		lrck_width_map = slots * slot_width - 1;
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_DAIFMT_SET, "unsupoort format\n");
		return -EINVAL;
	}
	regmap_update_bits(regmap, SUNXI_I2S_FMT0,
			   0x3ff << LRCK_PERIOD, lrck_width_map << LRCK_PERIOD);

	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_SLOT_NUM, slots);
	if (ret < 0)
		return -EINVAL;
	ret = sunxi_set_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_SLOT_WIDTH, slot_width);
	if (ret < 0)
		return -EINVAL;

	return 0;
}

static int sunxi_i2s_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream, &i2s->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream, &i2s->capture_dma_param);

	return 0;
}

static void sunxi_i2s_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	(void)substream;
	(void)dai;
}

static int sunxi_i2s_clk_set_rate_en(struct sunxi_i2s *i2s)
{
	struct sunxi_i2s_dai_fmt *i2s_dai_fmt = &i2s->i2s_dai_fmt;
	struct regmap *regmap = i2s->mem.regmap;
	unsigned int freq_in, freq_out;
	int ret;

	ret = sunxi_get_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_PLL, &freq_in);
	if (ret < 0)
		return -EINVAL;

	ret = sunxi_get_i2s_dai_fmt(i2s_dai_fmt, SUNXI_I2S_DAI_MCLK, &freq_out);
	if (ret < 0)
		return -EINVAL;

	if (snd_i2s_clk_rate(i2s->clk, freq_in, freq_out)) {
		SND_LOG_ERR("clk set rate failed\n");
		return -EINVAL;
	}

	ret = snd_i2s_clk_enable(i2s->clk);
	if (ret < 0)
		return -1;

	regmap_update_bits(regmap, SUNXI_I2S_CTL,
			   1 << GLOBAL_EN, 1 << GLOBAL_EN);

	return 0;
}


static int sunxi_i2s_clk_open(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params,
			      struct snd_soc_dai *dai)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct sunxi_i2s_clk_sta *i2s_clk_sta = &i2s->i2s_clk_sta;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	struct regmap *regmap = i2s->mem.regmap;
	unsigned int sample_rate;

	sample_rate = params_rate(params);

	/* open clk */
	if (!i2s_clk_sta->p_work && !i2s_clk_sta->c_work && !i2s_clk_sta->old_rate) {
		if (sunxi_i2s_clk_set_rate_en(i2s))
			return -1;
	}

	/* if clk_keep, need disable and then open clk for sample rate change */
	if (!i2s_clk_sta->p_work && !i2s_clk_sta->c_work &&
	    sample_rate != i2s_clk_sta->old_rate && dts->clk_keep) {
		regmap_update_bits(regmap, SUNXI_I2S_CTL,
				   1 << GLOBAL_EN, 0 << GLOBAL_EN);

		snd_i2s_clk_disable(i2s->clk);

		if (sunxi_i2s_clk_set_rate_en(i2s))
			return -1;
	}

	/* need different sample rate when i2s is woring, this cased i2s woring error */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK &&
	    !i2s_clk_sta->p_work && i2s_clk_sta->c_work &&
	    sample_rate != i2s_clk_sta->old_rate) {
		SND_LOG_ERR("i2s is capture in %dKHz, but new rate is %dKHz,"
			    "capture has wrong!!!\n",
			    i2s_clk_sta->old_rate, sample_rate);
		return -1;
	}
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE &&
	    !i2s_clk_sta->c_work && i2s_clk_sta->p_work &&
	    sample_rate != i2s_clk_sta->old_rate) {
		SND_LOG_ERR("i2s is playing in %dKHz, but new rate is %dKHz,"
			    "playing has wrong!!!\n",
			    i2s_clk_sta->old_rate, sample_rate);
		return -1;
	}

	i2s_clk_sta->old_rate = sample_rate;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		i2s_clk_sta->p_work = 1;
	} else {
		i2s_clk_sta->c_work = 1;
	}

	return 0;
}

static int sunxi_i2s_dai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	const struct sunxi_i2s_quirks *quirks = i2s->quirks;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	struct regmap *regmap = i2s->mem.regmap;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_params = snd_soc_dai_get_dma_data(sunxi_adpt_rtd_cpu_dai(rtd),
								       substream);
	int ret;

	SND_LOG_DEBUG("\n");

	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (i2s->dts.dai_type == SUNXI_DAI_HDMI_TYPE &&
			    dma_params->hdmi_fmt > HDMI_FMT_PCM) {
				regmap_update_bits(regmap, SUNXI_I2S_FMT0,
						   0x7 << I2S_SAMPLE_RESOLUTION,
						   0x5 << I2S_SAMPLE_RESOLUTION);
				regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL,
						   0x1 << TXIM, 0x0 << TXIM);
			} else {
				regmap_update_bits(regmap, SUNXI_I2S_FMT0,
						   0x7 << I2S_SAMPLE_RESOLUTION,
						   0x3 << I2S_SAMPLE_RESOLUTION);
				regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL,
						   0x1 << TXIM, 0x1 << TXIM);
			}
		} else {
			regmap_update_bits(regmap, SUNXI_I2S_FMT0,
					   0x7 << I2S_SAMPLE_RESOLUTION,
					   0x3 << I2S_SAMPLE_RESOLUTION);
			regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL, 0x3 << RXOM, 0x1 << RXOM);
		}
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
		regmap_update_bits(regmap, SUNXI_I2S_FMT0,
				   0x7 << I2S_SAMPLE_RESOLUTION,
				   0x5 << I2S_SAMPLE_RESOLUTION);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL, 0x1 << TXIM, 0x1 << TXIM);
		} else {
			regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL, 0x3 << RXOM, 0x1 << RXOM);
		}
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(regmap, SUNXI_I2S_FMT0,
				   0x7 << I2S_SAMPLE_RESOLUTION,
				   0x7 << I2S_SAMPLE_RESOLUTION);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL, 0x1 << TXIM, 0x1 << TXIM);
		} else {
			regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL, 0x3 << RXOM, 0x1 << RXOM);
		}
		break;
	default:
		SND_LOG_ERR_STD(E_I2S_SWARG_HW_PARAMS, "unrecognized format\n");
		return -EINVAL;
	}

	/* set channels map */
	ret = quirks->set_channels_map(i2s, params_channels(params));

	/* set channels */
	ret = quirks->set_channel_enable(i2s, substream->stream, params_channels(params));
	if (ret < 0) {
		SND_LOG_ERR("set channel enable failed\n");
		return -EINVAL;
	}

	mutex_lock(&i2s->i2s_clk_sta.clk_mutex);
	ret = sunxi_i2s_clk_open(substream, params, dai);
	mutex_unlock(&i2s->i2s_clk_sta.clk_mutex);
	if (ret < 0)
		return -1;

	if (dts->clk_en_post_delay)
		msleep(dts->clk_en_post_delay);

	return 0;
}

static int sunxi_i2s_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct sunxi_i2s_dts *dts = &i2s->dts;
	struct regmap *regmap = i2s->mem.regmap;
	struct sunxi_i2s_clk_sta *i2s_clk_sta = &i2s->i2s_clk_sta;

	SND_LOG_DEBUG("\n");

	mutex_lock(&i2s->i2s_clk_sta.clk_mutex);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (!i2s_clk_sta->p_work)
			goto exit;

		if ((!dts->clk_keep) && (!i2s_clk_sta->c_work)) {
			regmap_update_bits(regmap, SUNXI_I2S_CTL,
					   1 << GLOBAL_EN, 0 << GLOBAL_EN);

			snd_i2s_clk_disable(i2s->clk);

			i2s_clk_sta->old_rate = 0;
		}

		i2s_clk_sta->p_work = 0;
	} else {
		if (!i2s_clk_sta->c_work)
			goto exit;

		if ((!dts->clk_keep) && !(i2s_clk_sta->p_work)) {
			regmap_update_bits(regmap, SUNXI_I2S_CTL,
					   1 << GLOBAL_EN, 0 << GLOBAL_EN);

			snd_i2s_clk_disable(i2s->clk);

			i2s_clk_sta->old_rate = 0;
		}

		i2s_clk_sta->c_work = 0;
	}

exit:
	mutex_unlock(&i2s->i2s_clk_sta.clk_mutex);
	return 0;
}

static int sunxi_i2s_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = i2s->mem.regmap;
	unsigned int i;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0 ; i < 10 ; i++) {
			regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL,
					   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
			mdelay(1);
		}
		regmap_write(regmap, SUNXI_I2S_TXCNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL,
				   1 << FIFO_CTL_FRX, 1 << FIFO_CTL_FRX);
		regmap_write(regmap, SUNXI_I2S_RXCNT, 0);
	}

	return 0;
}

static void sunxi_i2s_dai_tx_route(struct sunxi_i2s *i2s, bool enable)
{
	struct sunxi_i2s_dts *dts = &i2s->dts;
	struct regmap *regmap = i2s->mem.regmap;
	unsigned int reg_val;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_I2S_INTCTL, 1 << TXDRQEN, 1 << TXDRQEN);
		sunxi_sdout_enable(regmap, dts->tx_pin);
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << CTL_TXEN, 1 << CTL_TXEN);
	} else {
		regmap_update_bits(regmap, SUNXI_I2S_INTCTL, 1 << TXDRQEN, 0 << TXDRQEN);

		/* add this to avoid the i2s pop */
		while (1) {
			regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL,
					   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
			regmap_write(regmap, SUNXI_I2S_TXCNT, 0);
			regmap_read(regmap, SUNXI_I2S_FIFOSTA, &reg_val);
			reg_val = ((reg_val & 0xFF0000) >> 16);
			if (reg_val == 0x80)
				break;
		}
		udelay(250);

		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << CTL_TXEN, 0 << CTL_TXEN);
		sunxi_sdout_disable(regmap);
	}
}

static void sunxi_i2s_dai_rx_route(struct sunxi_i2s *i2s, bool enable)
{
	struct regmap *regmap = i2s->mem.regmap;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << CTL_RXEN, 1 << CTL_RXEN);
		regmap_update_bits(regmap, SUNXI_I2S_INTCTL, 1 << RXDRQEN, 1 << RXDRQEN);
	} else {
		regmap_update_bits(regmap, SUNXI_I2S_INTCTL, 1 << RXDRQEN, 0 << RXDRQEN);
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << CTL_RXEN, 0 << CTL_RXEN);
	}
}

static int sunxi_i2s_dai_trigger(struct snd_pcm_substream *substream,
				 int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	const struct sunxi_i2s_quirks *quirks = i2s->quirks;
	struct sunxi_i2s_dts *dts = &i2s->dts;

	SND_LOG_DEBUG("\n");

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (i2s->tx_trigger_bypass)
				return 0;
			sunxi_i2s_dai_tx_route(i2s, true);
		} else {
			sunxi_i2s_dai_rx_route(i2s, true);
			if (dts->rx_sync_en && dts->rx_sync_ctl && quirks->rx_sync_en)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (i2s->tx_trigger_bypass)
				return 0;
			sunxi_i2s_dai_tx_route(i2s, true);
			if (i2s->audio_sta.spk)
				snd_sunxi_pa_pin_enable(i2s->pa_cfg, i2s->pa_pin_max);
		} else {
			sunxi_i2s_dai_rx_route(i2s, true);
			if (dts->rx_sync_en && dts->rx_sync_ctl && quirks->rx_sync_en)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (i2s->tx_trigger_bypass)
				return 0;
			sunxi_i2s_dai_tx_route(i2s, false);
		} else {
			sunxi_i2s_dai_rx_route(i2s, false);
			if (dts->rx_sync_en && dts->rx_sync_ctl && quirks->rx_sync_en)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (i2s->tx_trigger_bypass)
				return 0;
			if (i2s->audio_sta.spk)
				snd_sunxi_pa_pin_disable_irp(i2s->pa_cfg, i2s->pa_pin_max,
							     NULL, NULL);
			sunxi_i2s_dai_tx_route(i2s, false);
		} else {
			sunxi_i2s_dai_rx_route(i2s, false);
			if (dts->rx_sync_en && dts->rx_sync_ctl && quirks->rx_sync_en)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct snd_soc_dai_ops sunxi_i2s_dai_ops = {
	/* call by machine */
	.set_pll	= sunxi_i2s_dai_set_pll,	/* set pllclk */
	.set_sysclk	= sunxi_i2s_dai_set_sysclk,	/* set mclk */
	.set_bclk_ratio	= sunxi_i2s_dai_set_bclk_ratio,	/* set bclk freq */
	.set_fmt	= sunxi_i2s_dai_set_fmt,	/* set tdm fmt */
	.set_tdm_slot	= sunxi_i2s_dai_set_tdm_slot,	/* set slot num and width */
	/* call by asoc */
	.startup	= sunxi_i2s_dai_startup,
	.hw_params	= sunxi_i2s_dai_hw_params,
	.prepare	= sunxi_i2s_dai_prepare,
	.trigger	= sunxi_i2s_dai_trigger,
	.hw_free	= sunxi_i2s_dai_hw_free,
	.shutdown	= sunxi_i2s_dai_shutdown,
};

static int sunxi_i2s_init(struct sunxi_i2s *i2s)
{
	const struct sunxi_i2s_quirks *quirks = i2s->quirks;
	struct regmap *regmap = i2s->mem.regmap;

	regmap_update_bits(regmap, SUNXI_I2S_FMT1, 3 << SEXT, 0 << SEXT);
	regmap_update_bits(regmap, SUNXI_I2S_FMT1, 3 << TX_PDM, 0 << TX_PDM);
	regmap_update_bits(regmap, SUNXI_I2S_FMT1, 3 << RX_PDM, 0 << RX_PDM);

	if (quirks->rx_sync_en)
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << RX_SYNC_EN, 0 << RX_SYNC_EN);

	return 0;
}

static int sunxi_i2s_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_i2s *i2s = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG("\n");

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai,
				  &i2s->playback_dma_param,
				  &i2s->capture_dma_param);

	sunxi_i2s_init(i2s);
	return 0;
}

static int sunxi_i2s_dai_remove(struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG("\n");

	return 0;
}

static struct snd_soc_dai_driver sunxi_i2s_dai = {
	.name = DRV_NAME,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
};

/* clk */
static int sunxi_get_i2s_delay_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct sunxi_i2s_dts *dts = &i2s->dts;

	ucontrol->value.integer.value[0] = dts->clk_en_post_delay;

	return 0;
}

static int sunxi_put_i2s_delay_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct sunxi_i2s_dts *dts = &i2s->dts;

	dts->clk_en_post_delay = ucontrol->value.integer.value[0];

	return 0;
}

static int sunxi_get_i2s_clk_keep_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct sunxi_i2s_dts *dts = &i2s->dts;

	ucontrol->value.integer.value[0] = dts->clk_keep;

	return 0;
}

static int sunxi_put_i2s_clk_keep_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct sunxi_i2s_clk_sta *i2s_clk_sta = &i2s->i2s_clk_sta;
	struct sunxi_i2s_dts *dts = &i2s->dts;

	mutex_lock(&i2s_clk_sta->clk_mutex);
	if (!i2s_clk_sta->p_work && !i2s_clk_sta->c_work) {
		dts->clk_keep = ucontrol->value.integer.value[0];
	} else {
		SND_LOG_ERR("i2s is working, cannot change thie value");
	}
	mutex_unlock(&i2s_clk_sta->clk_mutex);

	return 0;
}

/*******************************************************************************
 * *** sound card & component function source ***
 * @0 sound card probe
 * @1 component function kcontrol register
 ******************************************************************************/
static void sunxi_rx_sync_enable(void *data, bool enable)
{
	struct regmap *regmap = data;

	SND_LOG_DEBUG("%s\n", enable ? "on" : "off");

	if (enable) {
		regmap_update_bits(regmap, SUNXI_I2S_CTL,
				   0x1 << RX_SYNC_EN_START, 0x1 << RX_SYNC_EN_START);
	} else {
		regmap_update_bits(regmap, SUNXI_I2S_CTL,
				   0x1 << RX_SYNC_EN_START, 0x0 << RX_SYNC_EN_START);
	}

	return;
}

static int sunxi_get_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = i2s->mem.regmap;

	unsigned int reg_val;

	regmap_read(regmap, SUNXI_I2S_FIFOCTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_set_tx_hub_mode(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct sunxi_i2s_dts *dts = &i2s->dts;
	struct regmap *regmap = i2s->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << CTL_TXEN, 0 << CTL_TXEN);
		sunxi_sdout_disable(regmap);
		regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL, 1 << HUB_EN, 0 << HUB_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL, 1 << HUB_EN, 1 << HUB_EN);
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << CTL_TXEN, 1 << CTL_TXEN);
		sunxi_sdout_enable(regmap, dts->tx_pin);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_get_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct sunxi_i2s_dts *dts = &i2s->dts;

	ucontrol->value.integer.value[0] = dts->rx_sync_ctl;

	return 0;
}

static int sunxi_set_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct sunxi_i2s_dts *dts = &i2s->dts;
	struct regmap *regmap = i2s->mem.regmap;

	if (dts->rx_sync_ctl == ucontrol->value.integer.value[0])
		return 0;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dts->rx_sync_ctl = 0;
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << RX_SYNC_EN, 0 << RX_SYNC_EN);
		sunxi_rx_sync_shutdown(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_I2S_CTL, 1 << RX_SYNC_EN, 1 << RX_SYNC_EN);
		dts->rx_sync_ctl = 1;
		sunxi_rx_sync_startup(dts->rx_sync_domain, dts->rx_sync_id);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_get_tx_trigger_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = i2s->tx_trigger_bypass;

	return 0;
}

static int sunxi_set_tx_trigger_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);

	i2s->tx_trigger_bypass = ucontrol->value.integer.value[0];

	return 0;
}

static const char *sunxi_switch_text[] = {"Off", "On"};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_trigger_mode_enum, sunxi_switch_text);

static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};
static const struct snd_kcontrol_new sunxi_tx_tirgger_controls[] = {
	SOC_ENUM_EXT("tx trigger bypass", sunxi_tx_trigger_mode_enum,
		     sunxi_get_tx_trigger_mode, sunxi_set_tx_trigger_mode),
};
static const struct snd_kcontrol_new sunxi_i2s_controls[] = {
	SOC_SINGLE("loopback debug", SUNXI_I2S_CTL, LOOP_EN, 1, 0),
};

static const struct snd_kcontrol_new sunxi_i2s_delay_controls[] = {
	SOC_SINGLE_EXT("clk en post delay", SND_SOC_NOPM, 0, 5000, 0,
		       sunxi_get_i2s_delay_mode,
		       sunxi_put_i2s_delay_mode),
};

static const struct snd_kcontrol_new sunxi_i2s_clk_keep_controls[] = {
	SOC_SINGLE_BOOL_EXT("clk keep", 0,
			    sunxi_get_i2s_clk_keep_mode,
			    sunxi_put_i2s_clk_keep_mode),
};

static int sunxi_spk_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		mutex_lock(&i2s->audio_sta.apf_mutex);
		i2s->audio_sta.spk = true;
		mutex_unlock(&i2s->audio_sta.apf_mutex);
		snd_sunxi_pa_pin_enable(i2s->pa_cfg, i2s->pa_pin_max);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		snd_sunxi_pa_pin_disable(i2s->pa_cfg, i2s->pa_pin_max);
		mutex_lock(&i2s->audio_sta.apf_mutex);
		i2s->audio_sta.spk = false;
		mutex_unlock(&i2s->audio_sta.apf_mutex);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget sunxi_i2s_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN("DAC", "Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_OUTPUT("I2S_PIN"),
	SND_SOC_DAPM_SPK("SPK", sunxi_spk_event),
};

static const struct snd_soc_dapm_route sunxi_i2s_dapm_routes[] = {
	{"I2S_PIN", NULL, "DAC"},
};

static int sunxi_i2s_component_probe(struct snd_soc_component *component)
{
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct snd_soc_dapm_context *dapm = &component->dapm;
	const struct sunxi_i2s_quirks *quirks = i2s->quirks;
	struct audio_reg_label *reg_label = i2s->reg_group.label;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	struct regmap *regmap = i2s->mem.regmap;
	unsigned int i;
	int ret;

	SND_LOG_DEBUG("\n");

	mutex_init(&i2s->audio_sta.apf_mutex);
	mutex_init(&i2s->i2s_clk_sta.clk_mutex);

	for (i = 0; i < i2s->reg_group.size; ++i)
		reg_label[i].address = quirks->audio_reg_addrs[i];

	/* component kcontrols -> tx_hub */
	if (dts->tx_hub_en) {
		ret = snd_soc_add_component_controls(component, sunxi_tx_hub_controls,
						     ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR_STD(E_I2S_SWSYS_COMP_PROBE, "add tx_hub kcontrols failed\n");
	}

	/* component kcontrols -> rx_sync */
	if (quirks->rx_sync_en && dts->rx_sync_en) {
		ret = snd_soc_add_component_controls(component, sunxi_rx_sync_controls,
						     ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR_STD(E_I2S_SWSYS_COMP_PROBE, "add rx_sync kcontrols failed\n");

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

	/* component kcontrols -> tx_trigger_bypass */
	ret = snd_soc_add_component_controls(component, sunxi_tx_tirgger_controls,
					     ARRAY_SIZE(sunxi_tx_tirgger_controls));
	if (ret)
		SND_LOG_ERR_STD(E_I2S_SWSYS_COMP_PROBE, "add tx_trigger kcontrols failed\n");

	/* dapm-widget */
	ret = snd_soc_dapm_new_controls(dapm, sunxi_i2s_dapm_widgets,
					ARRAY_SIZE(sunxi_i2s_dapm_widgets));
	if (ret)
		SND_LOG_ERR("register i2s dapm_widgets failed\n");

	/* dapm-routes */
	ret = snd_soc_dapm_add_routes(dapm, sunxi_i2s_dapm_routes,
				      ARRAY_SIZE(sunxi_i2s_dapm_routes));
	if (ret)
		SND_LOG_ERR("register i2s dapm_routes failed\n");

	ret = snd_sunxi_extparam_register_cb(component->card->name, EXTPARAM_ID_DAI_UCFMT,
					     sunxi_i2s_get_dai_ucfmt, (void *)i2s);
	if (ret) {
		SND_LOG_ERR("dai ucfmt callback register failed\n");
		return -EINVAL;
	}
	ret = snd_sunxi_extparam_register_cb(component->card->name, EXTPARAM_ID_HDMI_FMT,
					     sunxi_i2s_get_hdmi_fmt, (void *)i2s);
	if (ret) {
		SND_LOG_ERR("hdmi fmt callback register failed\n");
		return -EINVAL;
	}

	ret = snd_soc_add_component_controls(component, sunxi_i2s_delay_controls,
					     ARRAY_SIZE(sunxi_i2s_delay_controls));
	if (ret)
		SND_LOG_ERR_STD(E_I2S_SWSYS_COMP_PROBE, "add clk_en_post_delay kcontrols failed\n");

	ret = snd_soc_add_component_controls(component, sunxi_i2s_clk_keep_controls,
					     ARRAY_SIZE(sunxi_i2s_clk_keep_controls));
	if (ret)
		SND_LOG_ERR_STD(E_I2S_SWSYS_COMP_PROBE, "add clk_keep kcontrols failed\n");

	return 0;
}

static void sunxi_i2s_component_remove(struct snd_soc_component *component)
{
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	const struct sunxi_i2s_quirks *quirks = i2s->quirks;
	struct sunxi_i2s_dts *dts = &i2s->dts;
	SND_LOG_DEBUG("\n");

	mutex_destroy(&i2s->audio_sta.apf_mutex);
	mutex_destroy(&i2s->i2s_clk_sta.clk_mutex);

	if (quirks->rx_sync_en && dts->rx_sync_en)
		sunxi_rx_sync_unregister_cb(dts->rx_sync_domain, dts->rx_sync_id);

	snd_sunxi_extparam_unregister_cb(component->card->name, EXTPARAM_ID_DAI_UCFMT,
					 sunxi_i2s_get_dai_ucfmt);
	snd_sunxi_extparam_unregister_cb(component->card->name, EXTPARAM_ID_HDMI_FMT,
					 sunxi_i2s_get_hdmi_fmt);
}

static int sunxi_i2s_component_suspend(struct snd_soc_component *component)
{
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = i2s->mem.regmap;

	SND_LOG_DEBUG("\n");

	/* save reg value */
	snd_sunxi_save_reg(regmap, &i2s->reg_group);

	/* disable clk & regulator & pa */
	snd_sunxi_pa_pin_disable(i2s->pa_cfg, i2s->pa_pin_max);
	snd_sunxi_regulator_disable(i2s->rglt);
	snd_i2s_clk_bus_disable(i2s->clk);

	return 0;
}

static int sunxi_i2s_component_resume(struct snd_soc_component *component)
{
	struct sunxi_i2s *i2s = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = i2s->mem.regmap;
	int ret;
	int i;

	SND_LOG_DEBUG("\n");

	ret = snd_i2s_clk_bus_enable(i2s->clk);
	if (ret) {
		SND_LOG_ERR("clk_bus and clk_rst enable failed\n");
		return ret;
	}
	ret = snd_sunxi_regulator_enable(i2s->rglt);
	if (ret) {
		SND_LOG_ERR("regulator enable failed\n");
		return ret;
	}

	snd_sunxi_pa_pin_disable(i2s->pa_cfg, i2s->pa_pin_max);

	/* for i2s init */
	sunxi_i2s_init(i2s);

	/* resume reg value */
	snd_sunxi_echo_reg(regmap, &i2s->reg_group);

	/* for clear TX fifo */
	for (i = 0 ; i < 10 ; i++) {
		regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL,
				   1 << FIFO_CTL_FTX, 1 << FIFO_CTL_FTX);
		mdelay(1);
	}
	regmap_write(regmap, SUNXI_I2S_TXCNT, 0);

	/* for clear RX fifo */
	regmap_update_bits(regmap, SUNXI_I2S_FIFOCTL, 1 << FIFO_CTL_FRX, 1 << FIFO_CTL_FRX);
	regmap_write(regmap, SUNXI_I2S_RXCNT, 0);

	return 0;
}

static struct snd_soc_component_driver sunxi_i2s_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_i2s_component_probe,
	.remove		= sunxi_i2s_component_remove,
	.suspend	= sunxi_i2s_component_suspend,
	.resume		= sunxi_i2s_component_resume,
	.controls	= sunxi_i2s_controls,
	.num_controls	= ARRAY_SIZE(sunxi_i2s_controls),
};

/*******************************************************************************
 * *** kernel source ***
 * @1 regmap
 * @2 clk
 * @3 regulator
 * @4 dts params
 * @5 dma params
 * @6 pinctrl
 * @7 reg debug
 ******************************************************************************/
static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_i2s_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	ret = of_address_to_resource(np, 0, &mem->res);
	if (ret) {
		SND_LOG_ERR_STD(E_I2S_SWDEP_MEM_INIT, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res),
						 DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR_STD(E_I2S_SWDEP_MEM_INIT, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev, mem->memregion->start,
				    resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR_STD(E_I2S_SWDEP_MEM_INIT, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &g_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR_STD(E_I2S_SWDEP_MEM_INIT, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_i2s_mem *mem)
{
	SND_LOG_DEBUG("\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_i2s *i2s)
{
	struct sunxi_i2s_dts *dts = &i2s->dts;
	const struct sunxi_i2s_quirks *quirks = i2s->quirks;
	unsigned int i, j, k;
	int ret;
	unsigned int tmp_val0, tmp_val1;
	unsigned int tx_pin_cnt;
	char tx_pin_chmap_str[32] = "";
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* get dma params */
	ret = of_property_read_u32(np, "playback-cma", &tmp_val0);
	if (ret < 0) {
		dts->playback_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("playback-cma missing, using default value\n");
	} else {
		if (tmp_val0		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			tmp_val0	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (tmp_val0	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			tmp_val0	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts->playback_cma = tmp_val0;
	}
	ret = of_property_read_u32(np, "capture-cma", &tmp_val0);
	if (ret != 0) {
		dts->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN("capture-cma missing, using default value\n");
	} else {
		if (tmp_val0		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			tmp_val0	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (tmp_val0	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			tmp_val0	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts->capture_cma = tmp_val0;
	}
	ret = of_property_read_u32(np, "tx-fifo-size", &tmp_val0);
	if (ret != 0) {
		dts->playback_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("tx-fifo-size miss, using default value\n");
	} else {
		dts->playback_fifo_size = tmp_val0;
	}
	ret = of_property_read_u32(np, "rx-fifo-size", &tmp_val0);
	if (ret != 0) {
		dts->capture_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN("rx-fifo-size miss,using default value\n");
	} else {
		dts->capture_fifo_size = tmp_val0;
	}

	ret = of_property_read_u32(np, "tdm-num", &tmp_val0);
	if (ret < 0) {
		SND_LOG_WARN("tdm-num config missing\n");
		dts->tdm_num = 0;
	} else {
		dts->tdm_num = tmp_val0;
	}

	tx_pin_cnt = of_property_count_elems_of_size(np, "tx-pin", 4);
	for (i = 0; i < tx_pin_cnt; ++i) {
		ret = of_property_read_u32_index(np, "tx-pin", i, &tmp_val0);
		if (tmp_val0 > 3) {
			SND_LOG_WARN("tx-pin[%u] config invalid\n", i);
			continue;
		}
		if (ret < 0) {
			dts->tx_pin[tmp_val0] = false;
			SND_LOG_WARN("tx-pin[%u] config missing\n", i);
		} else {
			dts->tx_pin[tmp_val0] = true;
		}

		/* tmp_val0 length shounld be 1 char */
		snprintf(tx_pin_chmap_str, 32, "tx-pin%u-chmap", tmp_val0);
		for (j = 0; j < quirks->slot_num_max; ++j) {
			ret = of_property_read_u32_index(np, tx_pin_chmap_str, j, &tmp_val1);
			if (j == 0 && ret < 0) {
				SND_LOG_DEBUG("tx-pin%u-chmap config miss, default set:"
					      "0 1 2 ... %u\n", i, quirks->slot_num_max - 1);
				for (k = 0; k < quirks->slot_num_max; ++k)
					dts->tx_pin_chmap[tmp_val0][k] = k;
				break;
			} else if (ret < 0) {
				break;
			} else if (tmp_val1 >= quirks->slot_num_max) {
				SND_LOG_WARN("tx-pin%u-chmap[%u] config invalid: %u,"
					     "should less than %u, default set 0\n",
					     i, j, tmp_val1, quirks->slot_num_max);
				dts->tx_pin_chmap[tmp_val0][j] = 0;
			} else {
				dts->tx_pin_chmap[tmp_val0][j] = tmp_val1;
			}
		}
	}

	for (i = 0; i < quirks->slot_num_max; ++i) {
		ret = of_property_read_u32_index(np, "rxfifo-pinmap", i, &tmp_val1);
		if (i == 0 && ret < 0) {
			SND_LOG_DEBUG("rxfifo-pinmap config miss, rxfifo-pinmap default set: 0\n");
			for (j = 0; j < quirks->slot_num_max; j++)
				dts->rxfifo_pinmap[j] = 0;
			break;
		} else if (ret < 0) {
			break;
		} else if (tmp_val1 >= quirks->slot_num_max) {
			SND_LOG_WARN("rxfifo-pinmap[%u] config invalid: %u,"
				     "should less than %u, default set 0\n",
				     i, tmp_val1, quirks->slot_num_max);
			dts->rxfifo_pinmap[i] = 0;
		} else {
			dts->rxfifo_pinmap[i] = tmp_val1;
		}
	}

	for (i = 0; i < quirks->slot_num_max; ++i) {
		ret = of_property_read_u32_index(np, "rxfifo-chmap", i, &tmp_val1);
		if (i == 0 && ret < 0) {
			SND_LOG_DEBUG("rxfifo-chmap config miss, rxfifo-chmap default set:"
				     "0 1 2 ... %u\n", quirks->slot_num_max - 1);
			for (j = 0; j < quirks->slot_num_max; j++)
				dts->rxfifo_chmap[j] = j;
			break;
		} else if (ret < 0) {
			break;
		} else if (tmp_val1 >= quirks->slot_num_max) {
			SND_LOG_WARN("rxfifo_chmap[%u] config invalid: %u,"
				     "should less than %u, default set 0\n",
				     i, tmp_val1, quirks->slot_num_max);
			dts->rxfifo_chmap[i] = 0;
		} else {
			dts->rxfifo_chmap[i] = tmp_val1;
		}
	}

	SND_LOG_DEBUG("playback-cma : %zu\n", dts->playback_cma);
	SND_LOG_DEBUG("capture-cma  : %zu\n", dts->capture_cma);
	SND_LOG_DEBUG("tx-fifo-size : %zu\n", dts->playback_fifo_size);
	SND_LOG_DEBUG("rx-fifo-size : %zu\n", dts->capture_fifo_size);

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

	/* components func -> rx_sync */
	dts->rx_sync_en = of_property_read_bool(np, "rx-sync-en");

	/* dai-type */
	ret = snd_sunxi_hdmi_get_dai_type(np, &dts->dai_type);
	if (ret)
		dts->dai_type = SUNXI_DAI_I2S_TYPE;

	ret = of_property_read_u32(np, "clk-en-post-delay", &tmp_val0);
	if (ret < 0) {
		SND_LOG_WARN("clk-en-post-delay missing\n");
		dts->clk_en_post_delay = 0;
	} else {
		dts->clk_en_post_delay = tmp_val0;
	}

	ret = of_property_read_u32(np, "clk-keep", &tmp_val0);
	if (ret < 0) {
		SND_LOG_WARN("clk-keep missing\n");
		dts->clk_keep = 0;
	} else {
		dts->clk_keep = tmp_val0;
	}
}

static int snd_sunxi_pin_init(struct platform_device *pdev, struct sunxi_i2s_pinctl *pin)
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
		SND_LOG_ERR_STD(E_I2S_SWDEP_PIN_INIT, "pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin->pinstate = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin->pinstate)) {
		SND_LOG_ERR_STD(E_I2S_SWDEP_PIN_INIT, "pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin->pinstate_sleep = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin->pinstate_sleep)) {
		SND_LOG_ERR_STD(E_I2S_SWDEP_PIN_INIT, "pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin->pinctrl, pin->pinstate);
	if (ret < 0) {
		SND_LOG_ERR_STD(E_I2S_SWDEP_PIN_INIT, "i2s set pinctrl default state fail\n");
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

static void snd_sunxi_dma_params_init(struct sunxi_i2s *i2s)
{
	struct resource *res = &i2s->mem.res;
	struct sunxi_i2s_dts *dts = &i2s->dts;

	SND_LOG_DEBUG("\n");

	i2s->playback_dma_param.src_maxburst = 8;
	i2s->playback_dma_param.dst_maxburst = 8;
	i2s->playback_dma_param.dma_addr = res->start + SUNXI_I2S_TXFIFO;
	i2s->playback_dma_param.cma_kbytes = dts->playback_cma;
	i2s->playback_dma_param.fifo_size = dts->playback_fifo_size;
	i2s->playback_dma_param.hdmi_fmt = HDMI_FMT_PCM;

	i2s->capture_dma_param.src_maxburst = 8;
	i2s->capture_dma_param.dst_maxburst = 8;
	i2s->capture_dma_param.dma_addr = res->start + SUNXI_I2S_RXFIFO;
	i2s->capture_dma_param.cma_kbytes = dts->capture_cma;
	i2s->capture_dma_param.fifo_size = dts->capture_fifo_size;
};

static void snd_sunxi_pin_exit(struct platform_device *pdev, struct sunxi_i2s_pinctl *pin)
{
	SND_LOG_DEBUG("\n");

	if (pin->pinctrl_used)
		devm_pinctrl_put(pin->pinctrl);
}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
/* sysfs debug */
static void snd_sunxi_dump_version(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_i2s *i2s = (struct sunxi_i2s *)priv;

	if (!i2s) {
		SND_LOG_ERR_STD(E_I2S_SWARG_DUMP_VER, "priv to i2s failed\n");
		return;
	}
	if (i2s->pdev)
		if (i2s->pdev->dev.driver)
			if (i2s->pdev->dev.driver->owner)
				goto module_version;
	return;

module_version:
	i2s->module_version = i2s->pdev->dev.driver->owner->version;
	count_tmp += sprintf(buf + count_tmp, "%s\n", i2s->module_version);

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
	struct sunxi_i2s *i2s = (struct sunxi_i2s *)priv;
	struct audio_reg_group *reg_group = &i2s->reg_group;
	unsigned int i = 0;
	unsigned int output_reg_val;
	struct regmap *regmap;

	if (!i2s) {
		SND_LOG_ERR_STD(E_I2S_SWARG_DUMP_SHOW, "priv to i2s failed\n");
		return -1;
	}
	if (!i2s->show_reg_all)
		return 0;
	else
		i2s->show_reg_all = false;

	regmap = i2s->mem.regmap;
	for (i = 0; i < reg_group->size; ++i) {
		regmap_read(regmap, reg_group->label[i].address, &output_reg_val);
		count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
				     reg_group->label[i].address, output_reg_val);
	}

	*count = count_tmp;

	return 0;
}

static int snd_sunxi_dump_store(void *priv, const char *buf, size_t count)
{
	struct sunxi_i2s *i2s = (struct sunxi_i2s *)priv;
	int scanf_cnt;
	unsigned int input_reg_offset, input_reg_val, output_reg_val;
	struct regmap *regmap;

	if (count <= 1)	/* null or only "\n" */
		return 0;
	if (!i2s) {
		SND_LOG_ERR_STD(E_I2S_SWARG_DUMP_STORE, "priv to i2s failed\n");
		return -1;
	}
	regmap = i2s->mem.regmap;

	if (!strcmp(buf, "0\n")) {
		i2s->show_reg_all = true;
		return 0;
	}

	scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	if (scanf_cnt != 2) {
		pr_err("wrong format: %s\n", buf);
		return -1;
	}
	if (input_reg_offset > SUNXI_I2S_MAX_REG) {
		pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_I2S_MAX_REG);
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

static int sunxi_i2s_dev_probe(struct platform_device *pdev)
{
	struct sunxi_adapt_dai_ops_priv priv;
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_i2s *i2s;
	struct audio_reg_label *reg_label;
	struct sunxi_i2s_mem *mem;
	struct sunxi_i2s_pinctl *pin;
	struct sunxi_i2s_dts *dts;
	struct snd_sunxi_dump *dump;
	const struct sunxi_i2s_quirks *quirks;

	SND_LOG_DEBUG("\n");

	/* sunxi i2s */
	i2s = devm_kzalloc(dev, sizeof(*i2s), GFP_KERNEL);
	if (IS_ERR_OR_NULL(i2s)) {
		SND_LOG_ERR_STD(E_I2S_SWSYS_PLAT_PROBE, "alloc sunxi_i2s failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, i2s);

	mem = &i2s->mem;
	pin = &i2s->pin;
	dts = &i2s->dts;
	dump = &i2s->dump;
	i2s->pdev = pdev;

	quirks = of_device_get_match_data(&pdev->dev);
	if (quirks == NULL) {
		SND_LOG_ERR_STD(E_I2S_SWSYS_PLAT_PROBE, "quirks get failed\n");
		ret = -ENODEV;
		goto err_devm_kzalloc;
	}
	i2s->quirks = quirks;

	/* pa_pin init */
	i2s->pa_cfg = snd_sunxi_pa_pin_init(pdev, &i2s->pa_pin_max);

	reg_label = devm_kzalloc(dev, sizeof(*reg_label) * quirks->audio_reg_size,
				 GFP_KERNEL);
	if (IS_ERR_OR_NULL(reg_label)) {
		SND_LOG_ERR("alloc reg_label failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	i2s->reg_group.label = reg_label;
	i2s->reg_group.size = quirks->audio_reg_size;

	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR("remap init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_mem_init;
	}

	i2s->clk = snd_i2s_clk_init(pdev);
	if (!i2s->clk) {
		SND_LOG_ERR("clk init failed\n");
		ret = -EINVAL;
		goto err_snd_i2s_clk_init;
	}
	ret = snd_i2s_clk_bus_enable(i2s->clk);
	if (ret) {
		SND_LOG_ERR("clk_bus and clk_rst enable failed\n");
		ret = -EINVAL;
		goto err_clk_bus_enable;
	}

	i2s->rglt = snd_sunxi_regulator_init(dev);
	if (!i2s->rglt) {
		SND_LOG_ERR("rglt init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_rglt_init;
	}

	snd_sunxi_dts_params_init(pdev, i2s);
	snd_sunxi_dma_params_init(i2s);

	ret = snd_sunxi_pin_init(pdev, pin);
	if (ret) {
		SND_LOG_ERR("pinctrl init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_pin_init;
	}

	priv.probe = sunxi_i2s_dai_probe;
	priv.remove = sunxi_i2s_dai_remove;
	sunxi_adpt_set_dai_ops(&sunxi_i2s_dai, &sunxi_i2s_dai_ops, &priv);

	ret = snd_soc_register_component(&pdev->dev, &sunxi_i2s_dev, &sunxi_i2s_dai, 1);
	if (ret) {
		SND_LOG_ERR_STD(E_I2S_SWSYS_PLAT_PROBE, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_sunxi_dma_platform_register(&pdev->dev);
	if (ret) {
		SND_LOG_ERR("register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_sunxi_platform_register;
	}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snprintf(i2s->module_name, 32, "%s%u", "I2S", dts->tdm_num);
	dump->name = i2s->module_name;
	dump->priv = i2s;
	dump->dump_version = snd_sunxi_dump_version;
	dump->dump_help = snd_sunxi_dump_help;
	dump->dump_show = snd_sunxi_dump_show;
	dump->dump_store = snd_sunxi_dump_store;
	ret = snd_sunxi_dump_register(dump);
	if (ret)
		SND_LOG_WARN("snd_sunxi_dump_register failed\n");
#endif

	SND_LOG_DEBUG("register i2s platform success\n");

	return 0;

err_snd_sunxi_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
	snd_sunxi_pin_exit(pdev, pin);
err_snd_sunxi_pin_init:
	snd_sunxi_regulator_exit(i2s->rglt);
err_snd_sunxi_rglt_init:
err_clk_bus_enable:
	snd_i2s_clk_exit(i2s->clk);
err_snd_i2s_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_snd_sunxi_mem_init:
	devm_kfree(dev, i2s->reg_group.label);
	devm_kfree(dev, i2s);
err_devm_kzalloc:
	of_node_put(np);

	return ret;
}

static int sunxi_i2s_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_i2s *i2s = dev_get_drvdata(dev);
	struct sunxi_i2s_mem *mem = &i2s->mem;
	struct sunxi_i2s_pinctl *pin = &i2s->pin;
	struct sunxi_i2s_dts *dts = &i2s->dts;

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump = &i2s->dump;
#endif

	SND_LOG_DEBUG("\n");

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	/* remove components */
	snd_sunxi_dump_unregister(dump);
#endif
	if (dts->rx_sync_en)
		sunxi_rx_sync_remove(dts->rx_sync_domain);


	snd_sunxi_dma_platform_unregister(&pdev->dev);

	snd_soc_unregister_component(&pdev->dev);

	snd_sunxi_pin_exit(pdev, pin);
	snd_i2s_clk_bus_disable(i2s->clk);
	snd_i2s_clk_exit(i2s->clk);
	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_regulator_exit(i2s->rglt);
	snd_sunxi_pa_pin_exit(i2s->pa_cfg, i2s->pa_pin_max);

	devm_kfree(dev, i2s->reg_group.label);
	devm_kfree(dev, i2s);
	of_node_put(np);

	SND_LOG_DEBUG("unregister i2s platform success\n");

	return 0;
}

static const struct sunxi_i2s_quirks sunxi_i2s_quirks = {
	.slot_num_max		= 16,
	.audio_reg_addrs	= audio_reg_addrs,
	.audio_reg_size		= ARRAY_SIZE(audio_reg_addrs),
	.reg_max		= SUNXI_I2S_MAX_REG,
	.rx_sync_en = true,
	.set_channel_enable	= sunxi_i2s_set_ch_en,
	.set_daifmt_format	= sunxi_i2s_set_daifmt_fmt,
	.set_channels_map	= sunxi_i2s_set_ch_map,
};

static const struct sunxi_i2s_quirks sun8iw11_i2s_quirks = {
	.slot_num_max		= 8,
	.audio_reg_addrs	= sun8iw11_reg_addrs,
	.audio_reg_size		= ARRAY_SIZE(sun8iw11_reg_addrs),
	.reg_max		= SUN8IW11_I2S_MAX_REG,
	.rx_sync_en = false,
	.set_channel_enable	= sun8iw11_i2s_set_ch_en,
	.set_daifmt_format	= sun8iw11_i2s_set_daifmt_fmt,
	.set_channels_map	= sun8iw11_i2s_set_ch_map,
};

static const struct sunxi_i2s_quirks sun8iw17_i2s_quirks = {
	.slot_num_max		= 16,
	.audio_reg_addrs	= sun8iw17_reg_addrs,
	.audio_reg_size		= ARRAY_SIZE(sun8iw17_reg_addrs),
	.reg_max		= SUN8IW17_I2S_MAX_REG,
	.rx_sync_en = false,
	.set_channel_enable	= sun8iw17_i2s_set_ch_en,
	.set_daifmt_format	= sun8iw17_i2s_set_daifmt_fmt,
	.set_channels_map	= sun8iw17_i2s_set_ch_map,
};

static const struct of_device_id sunxi_i2s_of_match[] = {
	{
		.compatible = "allwinner," DRV_NAME,
		.data = &sunxi_i2s_quirks,
	},
	{
		.compatible = "allwinner,sun8iw11-i2s",
		.data = &sun8iw11_i2s_quirks,
	},
	{
		.compatible = "allwinner,sun8iw17-i2s",
		.data = &sun8iw17_i2s_quirks,
	},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_i2s_of_match);

static struct platform_driver sunxi_i2s_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_i2s_of_match,
	},
	.probe	= sunxi_i2s_dev_probe,
	.remove	= sunxi_i2s_dev_remove,
};

int __init sunxi_i2s_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_i2s_driver);
	if (ret != 0) {
		SND_LOG_ERR_STD(E_I2S_SWSYS_MOD_INIT, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_i2s_dev_exit(void)
{
	platform_driver_unregister(&sunxi_i2s_driver);
}

late_initcall(sunxi_i2s_dev_init);
module_exit(sunxi_i2s_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.15");
MODULE_DESCRIPTION("sunxi soundcard platform of i2s");

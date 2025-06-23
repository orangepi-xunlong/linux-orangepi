// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.

#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

/* Special Function Registers */
/* Control Register */
#define I2S_CTRL				0x00
#define I2S_CTRL_LR_PACK			BIT(31)
#define I2S_CTRL_FIFO_AFULL_MASK		BIT(30)
#define I2S_CTRL_FIFO_FULL_MASK			BIT(29)
#define I2S_CTRL_FIFO_AEMPTY_MASK		BIT(28)
#define I2S_CTRL_FIFO_EMPTY_MASK		BIT(27)
#define I2S_CTRL_I2S_MASK			BIT(26)
#define I2S_CTRL_INTREQ_MASK			BIT(25)
#define I2S_CTRL_I2S_STB			BIT(24)
#define I2S_CTRL_HOST_DATA_ALIGN		BIT(23)
#define I2S_CTRL_DATA_ORDER			BIT(22)
#define I2S_CTRL_DATA_ALIGN			BIT(21)
#define I2S_CTRL_DATA_WS_DEL			GENMASK(20, 16)
#define I2S_CTRL_WS_POLAR			BIT(15)
#define I2S_CTRL_SCK_POLAR			BIT(14)
#define I2S_CTRL_AUDIO_MODE			BIT(13)
#define I2S_CTRL_MONO_MODE			BIT(12)
#define I2S_CTRL_WS_MODE			GENMASK(11, 8)
#define I2S_CTRL_CHN_WIDTH			GENMASK(7, 5)
#define I2S_CTRL_CHN_WIDTH_8			0
#define I2S_CTRL_CHN_WIDTH_12			1
#define I2S_CTRL_CHN_WIDTH_16			2
#define I2S_CTRL_CHN_WIDTH_18			3
#define I2S_CTRL_CHN_WIDTH_20			4
#define I2S_CTRL_CHN_WIDTH_24			5
#define I2S_CTRL_CHN_WIDTH_28			6
#define I2S_CTRL_CHN_WIDTH_32			7
#define I2S_CTRL_FIFO_RST			BIT(4)
#define I2S_CTRL_SFR_RST			BIT(3)
#define I2S_CTRL_MS_CFG				BIT(2)
#define I2S_CTRL_DIR_CFG			BIT(1)
#define I2S_CTRL_I2S_EN				BIT(0)

/* Full-Duplex Mode Control Register */
#define I2S_CTRL_FDX				0x04
#define I2S_CTRL_FDX_RFIFO_AFULL_MASK		BIT(30)
#define I2S_CTRL_FDX_RFIFO_FULL_MASK		BIT(29)
#define I2S_CTRL_FDX_RFIFO_AEMPTY_MASK		BIT(28)
#define I2S_CTRL_FDX_RFIFO_EMPTY_MASK		BIT(27)
#define I2S_CTRL_FDX_RI2S_MASK			BIT(26)
#define I2S_CTRL_FDX_RFIFO_RST			BIT(4)
#define I2S_CTRL_FDX_I2S_FRX_EN			BIT(2)
#define I2S_CTRL_FDX_I2S_FTX_EN			BIT(1)
#define I2S_CTRL_FDX_FULL_DUPLEX		BIT(0)

/* Sample Resolution Register */
#define I2S_SRES				0x08
#define I2S_SRES_RESOLUTION			GENMASK(4, 0)

/* Full-Duplex Mode Receive Sample Resolution Register */
#define I2S_SRES_FDR				0x0c
#define I2S_SRES_FDR_RRESOLUTION		GENMASK(4, 0)

/* Transceiver Sample Rate Register */
#define I2S_SRATE				0x10
#define I2S_SRATE_SAMPLE_RATE			GENMASK(19, 0)

/* Status Flags Register */
#define I2S_STAT				0x14
#define I2S_STAT_RFIFO_AFULL			BIT(19)
#define I2S_STAT_RFIFO_FULL			BIT(18)
#define I2S_STAT_RFIFO_AEMPTY			BIT(17)
#define I2S_STAT_RFIFO_EMPTY			BIT(16)
#define I2S_STAT_FIFO_AFULL			BIT(5)
#define I2S_STAT_FIFO_FULL			BIT(4)
#define I2S_STAT_FIFO_AEMPTY			BIT(3)
#define I2S_STAT_FIFO_EMPTY			BIT(2)
#define I2S_STAT_RDATA_OVERR			BIT(1)
#define I2S_STAT_TDATA_UNDERR			BIT(0)

/* FIFO Level Register (read only) */
#define I2S_FIFO_LEVEL				0x18

/* FIFO Almost Empty Level Register */
#define I2S_FIFO_AEMPTY				0x1c

/* FIFO Almost Full Level Register */
#define I2S_FIFO_AFULL				0x20

/* Full-Duplex Mode Receiver FIFO Level Register (read only) */
#define I2S_FIFO_LEVEL_FDR			0x24

/* Full-Duplex Mode Receiver FIFO Almost Empty Level Register */
#define I2S_FIFO_AEMPTY_FDR			0x28

/* Full-Duplex Mode Receiver FIFO Almost Full Level Register */
#define I2S_FIFO_AFULL_FDR			0x2c

/* Time Division Multiplexing Control Register */
#define I2S_TDM_CTRL				0x30
#define I2S_TDM_CTRL_CHN_EN			GENMASK(31, 16)
#define I2S_TDM_CTRL_CHN_NO			GENMASK(4, 1)
#define I2S_TDM_CTRL_TDM_EN			BIT(0)

/* Time Division Multiplexing Full-Duplex Mode Channels Direction Register */
#define I2S_TDM_FD_DIR				0x34
#define I2S_TDM_FD_DIR_CHN_RXEN			GENMASK(31, 16)
#define I2S_TDM_FD_DIR_CHN_TXEN			GENMASK(15, 0)

/* Transmit And Receive FIFOs Address */
#define I2S_FIFO_ADDRESS			0x40

#define DRV_NAME	"cdns-i2s-sc"

#define SKY1_AUDSS_CRU_INFO_I2S_LOOP		0x5c
#define SKY1_AUDSS_CRU_INFO_I2S_LOOP_MAST	GENMASK(1, 0)

#define SKY1_AUDSS_CRU_INFO_MCLK		0x70
#define SKY1_AUDSS_CRU_INFO_MCLK_DIV_OFF(x)	(10 + (3 * (x)))
#define SKY1_AUDSS_CRU_INFO_MCLK_DIV_MASK(x)	GENMASK((12 + (3 * (x))), (10 + (3 * (x))))

enum {
	AUDIO_CLK0,
	AUDIO_CLK2,
	AUDIO_CLK_NUM,
};

static const char *cdns_i2s_sc_clk_pll_names[AUDIO_CLK_NUM] = {
	[AUDIO_CLK0] = "audio_clk0",
	[AUDIO_CLK2] = "audio_clk2",
};

struct cdns_i2s_sc_devtype_data {
	u32 fifo_depth;
	u32 rx_fifo_aempty_threshold;
	u32 rx_fifo_afull_threshold;
	u32 tx_fifo_aempty_threshold;
	u32 tx_fifo_afull_threshold;
};

struct cdns_i2s_sc_tdm_config {
	unsigned int tx_mask;
	unsigned int rx_mask;
	int slots;
	int slot_width;
};

struct cdns_i2s_sc_stats {
	u32 tdata_underr;
	u32 rdata_overr;
};

struct cdns_i2s_sc_priv {
	struct device *dev;
	struct regmap *regmap;
	struct regmap *cru_regmap;
	struct reset_control *i2s_rst;

	struct clk *clk_hst;
	struct clk *clk_i2s;
	struct clk *clk_mclk;
	struct clk *clks[AUDIO_CLK_NUM];

	struct snd_dmaengine_dai_dma_data capture_dma_data;
	struct snd_dmaengine_dai_dma_data playback_dma_data;

	const struct cdns_i2s_sc_devtype_data *devtype_data;
	struct cdns_i2s_sc_tdm_config tdm_config;
	struct cdns_i2s_sc_stats stats;

	/* i2s id index */
	int idx;

	bool is_master_mode;
	bool is_tdm_mode;

	/*
	 * NOTE:
	 * I2S_CTRL_FDX[I2S_FTX_EN] = 1 && I2S_CTRL[I2S_EN] = 1 to enable transmitter
	 * I2S_CTRL_FDX[I2S_FRX_EN] = 1 && I2S_CTRL[I2S_EN] = 1 to enable receiver
	 * We can dynamically switch of the transmitter and receiver enable separately,
	 * but need always enable I2S transceiver, so I2S-SC controller hopes to start
	 * the transmitter and receiver together, also to stop them when they are both
	 * try to stop.
	 */
	bool rx_start;
	bool tx_start;

	u8 mclk_idx;
};

/* I2S SC index */
enum {
	I2S_SC0 = 0,
	I2S_SC1,
	I2S_SC2
};

static const char * const i2s_sc_loopback_src_text[] = {
	"No Loopback",
	"Loopback from i2s0",
	"Loopback from i2s1",
	"Loopback from i2s3(mc2a)"};

static const struct soc_enum lp_src_enum =
    SOC_ENUM_SINGLE(SND_SOC_NOPM,
		    0,
		    ARRAY_SIZE(i2s_sc_loopback_src_text),
		    i2s_sc_loopback_src_text);

static int lp_src_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *dai = snd_kcontrol_chip(kcontrol);
	struct cdns_i2s_sc_priv *i2s_sc_priv = snd_soc_dai_get_drvdata(dai);
	int val;

	if (!i2s_sc_priv->cru_regmap)
		return 0;

	if (pm_runtime_status_suspended(i2s_sc_priv->dev))
		return 0;

	regmap_read(i2s_sc_priv->cru_regmap, SKY1_AUDSS_CRU_INFO_I2S_LOOP, &val);
	val &= SKY1_AUDSS_CRU_INFO_I2S_LOOP_MAST;

	ucontrol->value.enumerated.item[0] = val;

	return 0;
}

static int lp_src_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dai *dai = snd_kcontrol_chip(kcontrol);
	struct cdns_i2s_sc_priv *i2s_sc_priv = snd_soc_dai_get_drvdata(dai);
	int lp_src_index = ucontrol->value.enumerated.item[0];

	if (!i2s_sc_priv->cru_regmap)
		return 0;

	if (pm_runtime_status_suspended(i2s_sc_priv->dev)) {
		dev_info(i2s_sc_priv->dev,
			 "usage: playback from i2s0/1/3, set this mixer, then capture by i2s2");
		return 0;
	}

	regmap_update_bits(i2s_sc_priv->cru_regmap, SKY1_AUDSS_CRU_INFO_I2S_LOOP,
			   SKY1_AUDSS_CRU_INFO_I2S_LOOP_MAST,
			   FIELD_PREP(SKY1_AUDSS_CRU_INFO_I2S_LOOP_MAST, lp_src_index));

	return 0;
}

static const struct snd_kcontrol_new i2s_sc_lb_controls[] = {
	SOC_ENUM_EXT("Loopback Src", lp_src_enum, lp_src_get, lp_src_set),
};

static int cdns_i2s_sc_clks_enable(struct cdns_i2s_sc_priv *i2s_sc_priv)
{
	int ret;

	ret = clk_prepare_enable(i2s_sc_priv->clk_hst);
	if (ret)
		return ret;

	ret = clk_prepare_enable(i2s_sc_priv->clk_i2s);
	if (ret)
		clk_disable_unprepare(i2s_sc_priv->clk_hst);

	ret = clk_prepare_enable(i2s_sc_priv->clk_mclk);
	if (ret) {
		clk_disable_unprepare(i2s_sc_priv->clk_hst);
		clk_disable_unprepare(i2s_sc_priv->clk_i2s);
	}

	return ret;
}

static void cdns_i2s_sc_clks_disable(struct cdns_i2s_sc_priv *i2s_sc_priv)
{
	clk_disable_unprepare(i2s_sc_priv->clk_hst);
	clk_disable_unprepare(i2s_sc_priv->clk_i2s);
	clk_disable_unprepare(i2s_sc_priv->clk_mclk);
}

static void cdns_i2s_sc_rxtx_common_config(struct cdns_i2s_sc_priv *i2s_sc_priv, bool on)
{
	if (on) {
		/* Full-duplex mode enable */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX,
				   I2S_CTRL_FDX_FULL_DUPLEX, I2S_CTRL_FDX_FULL_DUPLEX);

		if (i2s_sc_priv->is_tdm_mode) {
			/* TDM mode enable */
			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_CTRL,
					   I2S_TDM_CTRL_TDM_EN, I2S_TDM_CTRL_TDM_EN);

			/* Number of supported audio channels in TDM mode */
			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_CTRL, I2S_TDM_CTRL_CHN_NO,
					   FIELD_PREP(I2S_TDM_CTRL_CHN_NO, i2s_sc_priv->tdm_config.slots - 1));

			/* TDM mode channels enable */
			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_CTRL, I2S_TDM_CTRL_CHN_EN,
					   FIELD_PREP(I2S_TDM_CTRL_CHN_EN,
						      i2s_sc_priv->tdm_config.rx_mask |
						      i2s_sc_priv->tdm_config.tx_mask));
		}

		/* Transceiver clock enable */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL,
				   I2S_CTRL_I2S_STB, 0);

		/* All interrupt requests unmask */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL,
				   I2S_CTRL_INTREQ_MASK, I2S_CTRL_INTREQ_MASK);
	} else {
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL,
				   I2S_CTRL_INTREQ_MASK, 0);

		if (i2s_sc_priv->is_tdm_mode) {
			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_CTRL,
					   I2S_TDM_CTRL_TDM_EN, 0);

			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_CTRL,
					   I2S_TDM_CTRL_CHN_NO, 0);

			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_CTRL,
					   I2S_TDM_CTRL_CHN_EN, 0);
		}

		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX,
				   I2S_CTRL_FDX_FULL_DUPLEX, 0);
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL,
				   I2S_CTRL_I2S_EN, 0);
	}
}

static void cdns_i2s_sc_tx_config(struct cdns_i2s_sc_priv *i2s_sc_priv, bool on)
{
	u32 irq_mask = 0;

	irq_mask |= I2S_CTRL_I2S_MASK;

	if (on) {
		cdns_i2s_sc_rxtx_common_config(i2s_sc_priv, on);

		/* Transmitter data underrun interrupt unmask */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL, irq_mask, irq_mask);

		if (i2s_sc_priv->is_tdm_mode)
			/* TDM mode channels transmit enale */
			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_FD_DIR, I2S_TDM_FD_DIR_CHN_TXEN,
					   FIELD_PREP(I2S_TDM_FD_DIR_CHN_TXEN, i2s_sc_priv->tdm_config.tx_mask));

		/* Full-duplex mode transmitter enable */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX,
				   I2S_CTRL_FDX_I2S_FTX_EN, I2S_CTRL_FDX_I2S_FTX_EN);

		/* Transceiver enable */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL,
				   I2S_CTRL_I2S_EN, I2S_CTRL_I2S_EN);

		i2s_sc_priv->tx_start = true;
	} else {
		i2s_sc_priv->tx_start = false;

		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX,
				   I2S_CTRL_FDX_I2S_FTX_EN, 0);

		if (i2s_sc_priv->is_tdm_mode)
			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_FD_DIR,
					   I2S_TDM_FD_DIR_CHN_TXEN, 0);

		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL, irq_mask, 0);

		if (!i2s_sc_priv->rx_start)
			cdns_i2s_sc_rxtx_common_config(i2s_sc_priv, on);
	}
}

static void cdns_i2s_sc_rx_config(struct cdns_i2s_sc_priv *i2s_sc_priv, bool on)
{
	u32 irq_mask = 0;

	irq_mask |= I2S_CTRL_FDX_RI2S_MASK;

	if (on) {
		cdns_i2s_sc_rxtx_common_config(i2s_sc_priv, on);

		/* Receiver data overrun interrupt unmask */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX, irq_mask, irq_mask);

		if (i2s_sc_priv->is_tdm_mode)
			/* TDM mode channels receive enale */
			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_FD_DIR, I2S_TDM_FD_DIR_CHN_RXEN,
					   FIELD_PREP(I2S_TDM_FD_DIR_CHN_RXEN, i2s_sc_priv->tdm_config.rx_mask));

		/* Full-duplex mode receiver enable */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX,
				   I2S_CTRL_FDX_I2S_FRX_EN, I2S_CTRL_FDX_I2S_FRX_EN);

		/* Transceiver enable */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL,
				   I2S_CTRL_I2S_EN, I2S_CTRL_I2S_EN);

		i2s_sc_priv->rx_start = true;
	} else {
		i2s_sc_priv->rx_start = false;

		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX,
				   I2S_CTRL_FDX_I2S_FRX_EN, 0);

		if (i2s_sc_priv->is_tdm_mode)
			regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_FD_DIR,
					   I2S_TDM_FD_DIR_CHN_RXEN, 0);

		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX, irq_mask, 0);

		if (!i2s_sc_priv->tx_start)
			cdns_i2s_sc_rxtx_common_config(i2s_sc_priv, on);
	}
}

static irqreturn_t cdns_i2s_sc_isr(int irq, void *devid)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = (struct cdns_i2s_sc_priv *)devid;
	struct device *dev = i2s_sc_priv->dev;
	u32 stat, stat_mask = 0;

	regmap_read(i2s_sc_priv->regmap, I2S_STAT, &stat);
	if (!stat)
		return IRQ_NONE;

	/* Clear the status */
	stat_mask |= I2S_STAT_TDATA_UNDERR | I2S_STAT_RDATA_OVERR;
	regmap_update_bits(i2s_sc_priv->regmap, I2S_STAT, stat_mask, 0);

	/* Transmitter status */
	if (stat & I2S_STAT_TDATA_UNDERR) {
		i2s_sc_priv->stats.tdata_underr++;
		dev_dbg(dev, "isr: tx data underrun\n");
	}

	/* Receiver status */
	if (stat & I2S_STAT_RDATA_OVERR) {
		i2s_sc_priv->stats.rdata_overr++;
		dev_dbg(dev, "isr: rx data overrun\n");
	}

	dev_dbg(dev, "%s: tdata_underr:%u, rdata_overr:%u\n",
		__func__, i2s_sc_priv->stats.tdata_underr, i2s_sc_priv->stats.rdata_overr);

	return IRQ_HANDLED;
}

static int cdns_i2s_sc_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id,
				  unsigned int freq, int dir)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = i2s_sc_priv->dev;
	u32 val;

	dev_dbg(dev, "mclk_idx = %d, mclk_div = %d, mclk freq = %d\n",
		i2s_sc_priv->mclk_idx, clk_id, freq);

	regmap_read(i2s_sc_priv->cru_regmap, SKY1_AUDSS_CRU_INFO_MCLK, &val);
	val &= ~SKY1_AUDSS_CRU_INFO_MCLK_DIV_MASK(i2s_sc_priv->mclk_idx);
	val |= (clk_id << SKY1_AUDSS_CRU_INFO_MCLK_DIV_OFF(i2s_sc_priv->mclk_idx));
	regmap_write(i2s_sc_priv->cru_regmap, SKY1_AUDSS_CRU_INFO_MCLK, val);

	return 0;
}

static int cdns_i2s_sc_set_tdm_slot(struct snd_soc_dai *cpu_dai, unsigned int tx_mask,
				    unsigned int rx_mask, int slots, int slot_width)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = i2s_sc_priv->dev;

	dev_dbg(dev, "tx_mask = 0x%x, rx_maske = 0x%x, slots = 0x%x, slot_width = 0x%x\n",
		     tx_mask, rx_mask, slots, slot_width);
	if (slots > 16 || slot_width > 32) {
		dev_err(i2s_sc_priv->dev,
			"TDM mode supports up to 16 slots and maximum slot width is 32 bit\n");
		return -EINVAL;
	}

	i2s_sc_priv->tdm_config.slots = slots;
	i2s_sc_priv->tdm_config.slot_width = slot_width;
	i2s_sc_priv->tdm_config.rx_mask = rx_mask;
	i2s_sc_priv->tdm_config.tx_mask = tx_mask;

	i2s_sc_priv->is_tdm_mode = true;

	return 0;
}

static int cdns_i2s_sc_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = i2s_sc_priv->dev;
	u32 ctrl = 0, ctrl_mask = 0;

	dev_dbg(dev, "format = 0x%x\n", fmt);

	/* DAI hardware audio formats */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		ctrl |= FIELD_PREP(I2S_CTRL_WS_MODE, 1) |
			FIELD_PREP(I2S_CTRL_DATA_WS_DEL, 1);
		ctrl &= ~(I2S_CTRL_DATA_ALIGN | I2S_CTRL_DATA_ORDER);
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		ctrl |= FIELD_PREP(I2S_CTRL_WS_MODE, 1) |
			FIELD_PREP(I2S_CTRL_DATA_WS_DEL, 0) |
			I2S_CTRL_DATA_ALIGN;
		ctrl &= ~I2S_CTRL_DATA_ORDER;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		ctrl |= FIELD_PREP(I2S_CTRL_WS_MODE, 1) |
			FIELD_PREP(I2S_CTRL_DATA_WS_DEL, 0);
		ctrl &= ~(I2S_CTRL_DATA_ALIGN | I2S_CTRL_DATA_ORDER);
		break;
	case SND_SOC_DAIFMT_DSP_A:
		ctrl |= FIELD_PREP(I2S_CTRL_WS_MODE, 0) |
			FIELD_PREP(I2S_CTRL_DATA_WS_DEL, 1);
		ctrl &= ~(I2S_CTRL_DATA_ALIGN | I2S_CTRL_DATA_ORDER);
		break;
	case SND_SOC_DAIFMT_DSP_B:
		ctrl |= FIELD_PREP(I2S_CTRL_WS_MODE, 0) |
			FIELD_PREP(I2S_CTRL_DATA_WS_DEL, 0);
		ctrl &= ~(I2S_CTRL_DATA_ALIGN | I2S_CTRL_DATA_ORDER);
		break;
	default:
		return -EINVAL;
	}

	/* DAI hardware signal polarity */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		/* both normal clocks */
		ctrl |= I2S_CTRL_SCK_POLAR;
		ctrl &= ~I2S_CTRL_WS_POLAR;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		/* invert frame clock */
		ctrl |= I2S_CTRL_SCK_POLAR;
		ctrl |= I2S_CTRL_WS_POLAR;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		/* invet bit clock */
		ctrl &= ~I2S_CTRL_SCK_POLAR;
		ctrl &= ~I2S_CTRL_WS_POLAR;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		/* invert both clocks */
		ctrl &= ~I2S_CTRL_WS_POLAR;
		ctrl |= I2S_CTRL_WS_POLAR;
		break;
	default:
		return -EINVAL;
	}

	/* DAI hardware clock masters */
	switch (fmt & SND_SOC_DAIFMT_CLOCK_PROVIDER_MASK) {
	case SND_SOC_DAIFMT_BC_FC:
		ctrl &= ~I2S_CTRL_MS_CFG;
		i2s_sc_priv->is_master_mode = false;
		break;
	case SND_SOC_DAIFMT_BP_FP:
		ctrl |= I2S_CTRL_MS_CFG;
		i2s_sc_priv->is_master_mode = true;
		break;
	default:
		return -EINVAL;
	}

	ctrl_mask |= I2S_CTRL_WS_MODE | I2S_CTRL_DATA_WS_DEL | I2S_CTRL_DATA_ALIGN |
		     I2S_CTRL_DATA_ORDER | I2S_CTRL_SCK_POLAR | I2S_CTRL_WS_POLAR |
		     I2S_CTRL_MS_CFG;
	regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL, ctrl_mask, ctrl);

	return 0;
}

static int cdns_i2s_sc_startup(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *cpu_dai)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = snd_soc_dai_get_drvdata(cpu_dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/*
		 * Transmitter FIFO reset
		 * When LOW, FIFO pointer is reset to zero. Threshold levels for
		 * FIFO are unchanged. This bit is automatically set to HIGH after
		 * one clock cycle.
		 * Dessert then assert this bit here, since I2S_CTRL register is not
		 * volatible, would not read from hardware any longer. If not, it would
		 * clear tx fifo every time when write this register.
		 */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL, I2S_CTRL_FIFO_RST, 0);
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL,
				   I2S_CTRL_FIFO_RST, I2S_CTRL_FIFO_RST);

		/* Transmitter FIFO threshold set */
		regmap_write(i2s_sc_priv->regmap, I2S_FIFO_AEMPTY,
			     i2s_sc_priv->devtype_data->tx_fifo_aempty_threshold);
		regmap_write(i2s_sc_priv->regmap, I2S_FIFO_AFULL,
			     i2s_sc_priv->devtype_data->tx_fifo_afull_threshold);
	} else {
		/*
		 * Receiver FIFO reset
		 * When '0', RFIFO pointer is reset to zero. Threshold levels for RFIFO
		 * are unchanged. The bit is automatically set to '1' after one clock cycle.
		 * Dessert then assert this bit here, since I2S_CTRL_FDX register is not
		 * volatible, would not read from hardware any longer. If not, it would
		 * clear rx fifo every time when write this register.
		 */
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX, I2S_CTRL_FDX_RFIFO_RST, 0);
		regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX,
				   I2S_CTRL_FDX_RFIFO_RST, I2S_CTRL_FDX_RFIFO_RST);

		/* Receiver FIFO threshold set */
		regmap_write(i2s_sc_priv->regmap, I2S_FIFO_AEMPTY_FDR,
			     i2s_sc_priv->devtype_data->rx_fifo_aempty_threshold);
		regmap_write(i2s_sc_priv->regmap, I2S_FIFO_AFULL_FDR,
			     i2s_sc_priv->devtype_data->rx_fifo_afull_threshold);
	}

	return 0;
}

static int cdns_i2s_sc_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *cpu_dai)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = i2s_sc_priv->dev;
	u32 slots, slot_width, resolution, val;
	u32 i2s_clk_rate, rate, sample_rate;
	struct clk *clk_parent;
	int ret;

	/* Set sample slots, slot width and resolution */
	slots = params_channels(params);
	resolution = params_width(params);
	dev_dbg(dev, "slots = %d, resolution = %d\n", slots, resolution);

	if (i2s_sc_priv->is_tdm_mode) {
		/* TDM mode */
		if (slots != i2s_sc_priv->tdm_config.slots) {
			dev_err(i2s_sc_priv->dev,
				"Slot number does not match with specified tdm slots number\n");
			return -EINVAL;
		}

		if (resolution != i2s_sc_priv->tdm_config.slot_width) {
			dev_err(i2s_sc_priv->dev,
				"Sample width does not match with specified tdm slot width\n");
			return -EINVAL;
		}

		slot_width = i2s_sc_priv->tdm_config.slot_width;
	} else {
		/* I2S mode */
		if (slots != 2) {
			dev_err(i2s_sc_priv->dev,
				"Only support stereo audio in I2S mode\n");
			return -EINVAL;
		}

		slot_width = 32;
	}

	dev_dbg(dev, "slot_width = %d\n", slot_width);

	if (slot_width == 8) {
		val = I2S_CTRL_CHN_WIDTH_8;
	} else if (slot_width == 12) {
		val = I2S_CTRL_CHN_WIDTH_12;
	} else if (slot_width == 16) {
		val = I2S_CTRL_CHN_WIDTH_16;
	} else if (slot_width == 18) {
		val = I2S_CTRL_CHN_WIDTH_18;
	} else if (slot_width == 20) {
		val = I2S_CTRL_CHN_WIDTH_20;
	} else if (slot_width == 24) {
		val = I2S_CTRL_CHN_WIDTH_24;
	} else if (slot_width == 28) {
		val = I2S_CTRL_CHN_WIDTH_28;
	} else if (slot_width == 32) {
		val = I2S_CTRL_CHN_WIDTH_32;
	} else {
		dev_err(i2s_sc_priv->dev, "Slot width is invalid value\n");
		return -EINVAL;
	}

	regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL, I2S_CTRL_CHN_WIDTH,
			   FIELD_PREP(I2S_CTRL_CHN_WIDTH, val));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		regmap_write(i2s_sc_priv->regmap, I2S_SRES,
			     FIELD_PREP(I2S_SRES_RESOLUTION, (resolution - 1)));
	else
		regmap_write(i2s_sc_priv->regmap, I2S_SRES_FDR,
			     FIELD_PREP(I2S_SRES_FDR_RRESOLUTION, (resolution - 1)));

	rate = params_rate(params);
	dev_dbg(dev, "rate = %d\n", rate);

	/* switch clk mux to select the appropriate clk parent */
	if (rate % 8000 == 0) {
		/* Sampling rate is a multiple of 8KHz, select "audio_clk0" */
		clk_parent = i2s_sc_priv->clks[AUDIO_CLK0];

		ret = clk_set_parent(i2s_sc_priv->clk_i2s, clk_parent);
		ret |= clk_set_parent(i2s_sc_priv->clk_mclk, clk_parent);
	} else if (rate % 11025 == 0){
		/* Sampling rate is a multiple of 11.025KHz, select "audio_clk2" */
		clk_parent = i2s_sc_priv->clks[AUDIO_CLK2];

		ret = clk_set_parent(i2s_sc_priv->clk_i2s, clk_parent);
		ret |= clk_set_parent(i2s_sc_priv->clk_mclk, clk_parent);
	} else {
		dev_err(i2s_sc_priv->dev, "Invalid sample rate\n");
		return -EINVAL;
	}
	if (ret) {
		dev_err(i2s_sc_priv->dev, "Failed to set i2s clock rate\n");
		return ret;
	}

	i2s_clk_rate = clk_get_rate(i2s_sc_priv->clk_i2s);
	dev_dbg(dev, "i2s clk rate = %d\n", i2s_clk_rate);

	if (i2s_clk_rate < rate * slot_width * slots * 6) {
		dev_err(i2s_sc_priv->dev,
			"clk freq %d is too low, must greater then %d * %d * %d * 6 = %d\n",
			i2s_clk_rate, rate, slot_width, slots, rate * slot_width * slots * 6);
		return -EINVAL;
	}

	/* Set sample rate */
	if (i2s_sc_priv->is_master_mode) {
		sample_rate = DIV_ROUND_CLOSEST(i2s_clk_rate, (rate * slots * slot_width));
		regmap_write(i2s_sc_priv->regmap, I2S_SRATE,
			     FIELD_PREP(I2S_SRATE_SAMPLE_RATE, sample_rate));
	}

	return 0;
}

static int cdns_i2s_sc_trigger(struct snd_pcm_substream *substream,
			       int cmd, struct snd_soc_dai *cpu_dai)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = snd_soc_dai_get_drvdata(cpu_dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			cdns_i2s_sc_tx_config(i2s_sc_priv, true);
		else
			cdns_i2s_sc_rx_config(i2s_sc_priv, true);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			cdns_i2s_sc_tx_config(i2s_sc_priv, false);
		else
			cdns_i2s_sc_rx_config(i2s_sc_priv, false);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops cdns_i2s_sc_dai_ops = {
	.set_sysclk = cdns_i2s_sc_set_sysclk,
	.set_tdm_slot = cdns_i2s_sc_set_tdm_slot,
	.set_fmt = cdns_i2s_sc_set_fmt,

	.startup = cdns_i2s_sc_startup,
	.hw_params = cdns_i2s_sc_hw_params,
	.trigger = cdns_i2s_sc_trigger,
};

static int cdns_i2s_sc_dai_probe(struct snd_soc_dai *cpu_dai)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = snd_soc_dai_get_drvdata(cpu_dai);
	u32 ctrl = 0;
	int ret = 0;

	/* i2s_sc2 for loopback */
	if (I2S_SC2 == i2s_sc_priv->idx) {
		ret = snd_soc_add_dai_controls(cpu_dai, i2s_sc_lb_controls,
					ARRAY_SIZE(i2s_sc_lb_controls));
		if (ret < 0)
			dev_warn(i2s_sc_priv->dev, "failed add dai controls\n");
	}

	/*
	 * Transceiver disable
	 * Transceiver clock disable
	 * All interrupts masked
	 */
	ctrl &= ~(I2S_CTRL_I2S_EN | I2S_CTRL_INTREQ_MASK);
	ctrl |= I2S_CTRL_I2S_STB;
	regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL,
			   I2S_CTRL_I2S_EN | I2S_CTRL_INTREQ_MASK |
			   I2S_CTRL_LR_PACK | I2S_CTRL_I2S_STB,
			   ctrl);

	/*
	 * Full-duplex mode disable
	 * Full-duplex mode transmitter disable
	 * Full-duplex mode receiver disable
	 */
	regmap_update_bits(i2s_sc_priv->regmap, I2S_CTRL_FDX,
			   I2S_CTRL_FDX_FULL_DUPLEX | I2S_CTRL_FDX_I2S_FTX_EN |
			   I2S_CTRL_FDX_I2S_FRX_EN,
			   0);

	/* TDM mode disable, default works in standard stereo I2S mode */
	regmap_update_bits(i2s_sc_priv->regmap, I2S_TDM_CTRL, I2S_TDM_CTRL_TDM_EN, 0);

	snd_soc_dai_init_dma_data(cpu_dai, &i2s_sc_priv->playback_dma_data,
				  &i2s_sc_priv->capture_dma_data);

	snd_soc_dai_set_drvdata(cpu_dai, i2s_sc_priv);

	return 0;
}

static struct snd_soc_dai_driver cdns_i2s_sc_dai = {
	.probe = cdns_i2s_sc_dai_probe,
	.playback = {
		.stream_name = "I2S-SC-Playback",
		.channels_min = 1,
		.channels_max = 16,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE |
			    SNDRV_PCM_FMTBIT_S24_LE |
			    SNDRV_PCM_FMTBIT_S32_LE),
	},
	.capture = {
		.stream_name = "I2S-SC-Capture",
		.channels_min = 1,
		.channels_max = 16,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE |
			    SNDRV_PCM_FMTBIT_S24_LE |
			    SNDRV_PCM_FMTBIT_S32_LE),
	},
	.ops = &cdns_i2s_sc_dai_ops,
	.symmetric_rate = 1,
	.symmetric_channels = 1,
};

static const struct snd_soc_component_driver cdns_i2s_sc_component = {
	.name = DRV_NAME,
	.legacy_dai_naming      = 1,
};

static const struct reg_default cdns_i2s_sc_reg_defaults[] = {
	{I2S_CTRL, 0x000001b8},
	{I2S_CTRL_FDX, 0x00000010},
	{I2S_SRES, 0x00000000},
	{I2S_SRES_FDR, 0x00000000},
	{I2S_SRATE, 0x00000000},
	{I2S_STAT, 0x0003000c},
	{I2S_FIFO_LEVEL, 0x00000000},
	{I2S_FIFO_AEMPTY, 0x00000000},
	{I2S_FIFO_AFULL, 0x0000000f},
	{I2S_FIFO_LEVEL_FDR, 0x00000000},
	{I2S_FIFO_AEMPTY_FDR, 0x00000000},
	{I2S_FIFO_AFULL_FDR, 0x0000000f},
	{I2S_TDM_CTRL, 0xffff0000},
	{I2S_TDM_FD_DIR, 0x0000ffff},
};

static bool cdns_i2s_sc_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case I2S_CTRL:
	case I2S_CTRL_FDX:
	case I2S_SRES:
	case I2S_SRES_FDR:
	case I2S_SRATE:
	case I2S_STAT:
	case I2S_FIFO_LEVEL:
	case I2S_FIFO_AEMPTY:
	case I2S_FIFO_AFULL:
	case I2S_FIFO_LEVEL_FDR:
	case I2S_FIFO_AEMPTY_FDR:
	case I2S_FIFO_AFULL_FDR:
	case I2S_TDM_CTRL:
	case I2S_TDM_FD_DIR:
		return true;
	default:
		return false;
	}
};

static bool cdns_i2s_sc_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case I2S_CTRL:
	case I2S_CTRL_FDX:
	case I2S_SRES:
	case I2S_SRES_FDR:
	case I2S_SRATE:
	case I2S_STAT:
	case I2S_FIFO_AEMPTY:
	case I2S_FIFO_AFULL:
	case I2S_FIFO_AEMPTY_FDR:
	case I2S_FIFO_AFULL_FDR:
	case I2S_TDM_CTRL:
	case I2S_TDM_FD_DIR:
		return true;
	default:
		return false;
	}
};

static bool cdns_i2s_sc_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case I2S_STAT:
	case I2S_FIFO_LEVEL:
	case I2S_FIFO_LEVEL_FDR:
		return true;
	default:
		return false;
	}
};

static struct regmap_config cdns_i2s_sc_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,

	.max_register = I2S_TDM_FD_DIR,
	.reg_defaults = cdns_i2s_sc_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(cdns_i2s_sc_reg_defaults),
	.readable_reg = cdns_i2s_sc_readable_reg,
	.writeable_reg = cdns_i2s_sc_writeable_reg,
	.volatile_reg = cdns_i2s_sc_volatile_reg,
	.cache_type = REGCACHE_FLAT,
};

static struct snd_dmaengine_pcm_config *devm_snd_pcm_alloc_config(
			struct device *dev)
{
	struct snd_dmaengine_pcm_config *config = NULL;
	int cnt, ret;

	if (!dev)
		return NULL;

	config = devm_kzalloc(dev, sizeof(*config), GFP_KERNEL);
	if (!config)
		return NULL;

	cnt = device_property_string_array_count(dev, "dma-names");
	if (cnt < 0)
		goto ERROR;

	if (cnt > SNDRV_PCM_STREAM_LAST + 1) /* enum start from 0 */
		cnt = SNDRV_PCM_STREAM_LAST + 1;

	ret = device_property_read_string_array(dev, "dma-names",
						config->chan_names, cnt);
	if (ret < 0)
		goto ERROR;

	config->prepare_slave_config = snd_dmaengine_pcm_prepare_slave_config;

	return config;

ERROR:
	if (config)
		kfree(config);
	dev_err(dev, "snd pcm alloc config fail\n");

	return NULL;
}

static void cdns_i2s_sc_rst(struct cdns_i2s_sc_priv *i2s_sc_priv)
{
	u32 idx = i2s_sc_priv->idx;

	dev_dbg(i2s_sc_priv->dev, "i2s index = %d\n", idx);

	/* reset */
	reset_control_assert(i2s_sc_priv->i2s_rst);

	usleep_range(1, 2);

	/* release reset */
	reset_control_deassert(i2s_sc_priv->i2s_rst);
}

static int cdns_i2s_sc_probe(struct platform_device *pdev)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv;
	struct resource *res;
	void __iomem *base;
	int i, irq, ret;
	char *irq_name;

	i2s_sc_priv = devm_kzalloc(&pdev->dev, sizeof(*i2s_sc_priv), GFP_KERNEL);
	if (!i2s_sc_priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, i2s_sc_priv);

	base = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	i2s_sc_priv->dev = &pdev->dev;
	i2s_sc_priv->devtype_data = device_get_match_data(&pdev->dev);
	i2s_sc_priv->idx = of_alias_get_id(i2s_sc_priv->dev->of_node, "i2s");
	if (i2s_sc_priv->idx < 0)
		device_property_read_u32(&pdev->dev, "id", &i2s_sc_priv->idx);

	i2s_sc_priv->regmap = devm_regmap_init_mmio(&pdev->dev, base,
						    &cdns_i2s_sc_regmap_config);
	if (IS_ERR(i2s_sc_priv->regmap)) {
		dev_err(&pdev->dev, "Failed to initialize managed register map\n");
		return PTR_ERR(i2s_sc_priv->regmap);
	}

	i2s_sc_priv->clk_hst = devm_clk_get(&pdev->dev, "hst");
	if (IS_ERR(i2s_sc_priv->clk_hst)) {
		dev_err(&pdev->dev, "Failed to get clk_hst clock\n");
		return PTR_ERR(i2s_sc_priv->clk_hst);
	}

	i2s_sc_priv->clk_i2s = devm_clk_get(&pdev->dev, "i2s");
	if (IS_ERR(i2s_sc_priv->clk_i2s)) {
		dev_err(&pdev->dev, "Failed to get clk_i2s clock\n");
		return PTR_ERR(i2s_sc_priv->clk_i2s);
	}

	i2s_sc_priv->clk_mclk = devm_clk_get_optional(&pdev->dev, "mclk");
	if (IS_ERR(i2s_sc_priv->clk_mclk)) {
		dev_err(&pdev->dev, "Failed to get clk_mclk clock\n");
		return PTR_ERR(i2s_sc_priv->clk_mclk);
	} else if (i2s_sc_priv->clk_mclk) {
		ret = device_property_read_u8(&pdev->dev, "cdns,mclk-idx", &i2s_sc_priv->mclk_idx);
		if (ret) {
			dev_err(&pdev->dev, "Failed to get mclk-idx: %d", ret);
			return ret;
		}
	}

	for (i = 0; i < AUDIO_CLK_NUM; i++) {
		i2s_sc_priv->clks[i] = devm_clk_get(&pdev->dev,
						    cdns_i2s_sc_clk_pll_names[i]);
		if (IS_ERR(i2s_sc_priv->clks[i])) {
			dev_err(&pdev->dev, "Failed to get clock %s\n",
				cdns_i2s_sc_clk_pll_names[i]);
			return PTR_ERR(i2s_sc_priv->clks[i]);
		}
	}

	i2s_sc_priv->i2s_rst = devm_reset_control_get(&pdev->dev, "i2s");
	if (IS_ERR(i2s_sc_priv->i2s_rst))
		return PTR_ERR(i2s_sc_priv->i2s_rst);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	irq_name = devm_kasprintf(&pdev->dev, GFP_KERNEL,
				  "%s%d",
				  dev_name(&pdev->dev), i2s_sc_priv->idx);
	if (!irq_name)
			return -ENOMEM;

	ret = devm_request_irq(&pdev->dev, irq, cdns_i2s_sc_isr, IRQF_SHARED,
			       irq_name, i2s_sc_priv);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq: %d", ret);
		return ret;
	}

	i2s_sc_priv->playback_dma_data.addr = res->start + I2S_FIFO_ADDRESS;
	/* Buswidth will be set by framework at runtime */
	i2s_sc_priv->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
	i2s_sc_priv->playback_dma_data.maxburst = 4;

	i2s_sc_priv->capture_dma_data.addr = res->start + I2S_FIFO_ADDRESS;
	/* Buswidth will be set by framework at runtime */
	i2s_sc_priv->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
	i2s_sc_priv->capture_dma_data.maxburst = 4;

	ret = devm_snd_soc_register_component(&pdev->dev, &cdns_i2s_sc_component,
					      &cdns_i2s_sc_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register soc componnet:%d\n", ret);
		return ret;
	}

	if (ACPI_COMPANION(&pdev->dev))
		/*
		 * For now dmaengine_pcm_request_chan_of() was called in
		 * function devm_snd_dmaengine_pcm_register(), which didn't
		 * work in acpi intialize if you want request the dma channel.
		 * So that snd dma config should be specified, and dma channel
		 * will be requested in snd pcm creation process.
		 */
		ret = devm_snd_dmaengine_pcm_register(&pdev->dev,
				devm_snd_pcm_alloc_config(&pdev->dev), 0);
	else
		ret = devm_snd_dmaengine_pcm_register(&pdev->dev, NULL, 0);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register dmaengine component:%d\n", ret);
		return ret;
	}

	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	ret = cdns_i2s_sc_clks_enable(i2s_sc_priv);
	if (ret) {
		dev_err(&pdev->dev, "Failed to enable clocks:%d\n", ret);
		return ret;
	}

	i2s_sc_priv->cru_regmap =
		device_syscon_regmap_lookup_by_property(&pdev->dev,
					"cdns,cru-ctrl");
	if (PTR_ERR(i2s_sc_priv->cru_regmap) == -ENODEV) {
		i2s_sc_priv->cru_regmap = NULL;
	} else if (IS_ERR(i2s_sc_priv->cru_regmap)) {
		ret = PTR_ERR(i2s_sc_priv->cru_regmap);
		goto regmap_failed;
	}

	cdns_i2s_sc_rst(i2s_sc_priv);

	/*
	 * Let pm_runtime_put() disable the clocks.
	 * If CONFIG_PM is not enabled, the clock will stay powered.
	 */
	pm_runtime_put(&pdev->dev);

	dev_info(&pdev->dev, "I2S-SC driver probed\n");

	return 0;

regmap_failed:
	pm_runtime_put(&pdev->dev);

	return ret;
}

static int cdns_i2s_sc_remove(struct platform_device *pdev)
{
	if (!pm_runtime_status_suspended(&pdev->dev))
		pm_runtime_force_suspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static int __maybe_unused cdns_i2s_sc_runtime_suspend(struct device *dev)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = dev_get_drvdata(dev);

	regcache_cache_only(i2s_sc_priv->regmap, true);
	cdns_i2s_sc_clks_disable(i2s_sc_priv);

	return 0;
}

static int __maybe_unused cdns_i2s_sc_runtime_resume(struct device *dev)
{
	struct cdns_i2s_sc_priv *i2s_sc_priv = dev_get_drvdata(dev);
	int ret;

	ret = cdns_i2s_sc_clks_enable(i2s_sc_priv);
	if (ret) {
		dev_err(dev, "Failed to enable clocks:%d\n", ret);
		return ret;
	}

	cdns_i2s_sc_rst(i2s_sc_priv);

	regcache_cache_only(i2s_sc_priv->regmap, false);
	regcache_mark_dirty(i2s_sc_priv->regmap);

	ret = regcache_sync(i2s_sc_priv->regmap);
	if (ret)
		cdns_i2s_sc_clks_disable(i2s_sc_priv);

	return ret;
}

static const struct dev_pm_ops cdns_i2s_sc_pm_ops = {
	SET_RUNTIME_PM_OPS(cdns_i2s_sc_runtime_suspend,
			   cdns_i2s_sc_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
};

static const struct cdns_i2s_sc_devtype_data sky1_devtype_data = {
	/* When DEPTH = 4 then each FIFO has 16 words and each FIFO memory size is 64 bytes */
	.fifo_depth = 4,
	.rx_fifo_aempty_threshold = 4,
	.rx_fifo_afull_threshold = 12,
	.tx_fifo_aempty_threshold = 4,
	.tx_fifo_afull_threshold = 12,
};

static const struct of_device_id cdns_i2s_sc_of_match[] = {
	{ .compatible = "cdns,sky1-i2s-sc", .data = &sky1_devtype_data},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, cdns_i2s_sc_of_match);

static const struct acpi_device_id cdns_i2s_sc_acpi_match[] = {
	{ "CIXH6010", .driver_data = (kernel_ulong_t)&sky1_devtype_data },
	{ },
};
MODULE_DEVICE_TABLE(acpi, cdns_i2s_sc_acpi_match);

static struct platform_driver cdns_i2s_sc_driver = {
	.probe = cdns_i2s_sc_probe,
	.remove = cdns_i2s_sc_remove,
	.driver = {
		.name = DRV_NAME,
		.pm = &cdns_i2s_sc_pm_ops,
		.of_match_table = cdns_i2s_sc_of_match,
		.acpi_match_table = ACPI_PTR(cdns_i2s_sc_acpi_match),
	},
};
module_platform_driver(cdns_i2s_sc_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Joakim Zhang <joakim.zhang@cixtech.com>");
MODULE_DESCRIPTION("Cadence I2S-SC Controller Driver");

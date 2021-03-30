/*
 * sound\soc\sunxi\sunxi_daudio.h
 * (C) Copyright 2014-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
 * wolfgang huang <huangjinhui@allwinertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef	__SUNXI_DAUDIO_H_
#define	__SUNXI_DAUDIO_H_

/* DAUDIO register definition */
#define	SUNXI_DAUDIO_CTL	0x00
#define	SUNXI_DAUDIO_FMT0	0x04
#define	SUNXI_DAUDIO_FMT1	0x08
#define	SUNXI_DAUDIO_INTSTA	0x0C
#define	SUNXI_DAUDIO_RXFIFO	0x10
#define	SUNXI_DAUDIO_FIFOCTL	0x14
#define	SUNXI_DAUDIO_FIFOSTA	0x18
#define	SUNXI_DAUDIO_INTCTL	0x1C
#define	SUNXI_DAUDIO_TXFIFO	0x20
#define	SUNXI_DAUDIO_CLKDIV	0x24
#define	SUNXI_DAUDIO_TXCNT	0x28
#define	SUNXI_DAUDIO_RXCNT	0x2C
#define	SUNXI_DAUDIO_CHCFG	0x30
#define	SUNXI_DAUDIO_TX0CHSEL	0x34
#define	SUNXI_DAUDIO_TX1CHSEL	0x38
#define	SUNXI_DAUDIO_TX2CHSEL	0x3C
#define	SUNXI_DAUDIO_TX3CHSEL	0x40

#if	defined(CONFIG_ARCH_SUN8IW10)
#define	SUNXI_DAUDIO_TX0CHMAP0	0x44
#define	SUNXI_DAUDIO_TX0CHMAP1	0x48
#define	SUNXI_DAUDIO_TX1CHMAP0	0x4C
#define	SUNXI_DAUDIO_TX1CHMAP1	0x50
#define	SUNXI_DAUDIO_TX2CHMAP0	0x54
#define	SUNXI_DAUDIO_TX2CHMAP1	0x58
#define	SUNXI_DAUDIO_TX3CHMAP0	0x5C
#define	SUNXI_DAUDIO_TX3CHMAP1	0x60
#define	SUNXI_DAUDIO_RXCHSEL	0x64
#define	SUNXI_DAUDIO_RXCHMAP0	0x68
#define	SUNXI_DAUDIO_RXCHMAP1	0x6C
#define	SUNXI_DAUDIO_DEBUG		0x70
#else
#define	SUNXI_DAUDIO_TX0CHMAP0	0x44
#define	SUNXI_DAUDIO_TX1CHMAP0	0x48
#define	SUNXI_DAUDIO_TX2CHMAP0	0x4C
#define	SUNXI_DAUDIO_TX3CHMAP0	0x50
#define	SUNXI_DAUDIO_RXCHSEL	0x54
#define	SUNXI_DAUDIO_RXCHMAP	0x58

#define	SUNXI_DAUDIO_DEBUG	0x5C
#endif

/* SUNXI_DAUDIO_CTL:0x00 */
#define	BCLK_OUT		18
#define	LRCK_OUT		17
#define	LRCKR_CTL		16
#define	SDO3_EN			11
#define	SDO2_EN			10
#define	SDO1_EN			9
#define	SDO0_EN			8
#define	MUTE_CTL		6
#define	MODE_SEL		4
#define	LOOP_EN			3
#define	CTL_TXEN		2
#define	CTL_RXEN		1
#define	GLOBAL_EN		0

/* SUNXI_DAUDIO_FMT0:0x04 */
#define	SDI_SYNC_SEL		31
#define	LRCK_WIDTH		30
#define	LRCKR_PERIOD		20
#define	LRCK_POLARITY		19
#define	LRCK_PERIOD		8
#define	BRCK_POLARITY		7
#define	SAMPLE_RESOLUTION	4
#define	EDGE_TRANSFER		3
#define	SLOT_WIDTH		0

/* SUNXI_DAUDIO_FMT1:0x08 */
#define	RX_MLS			7
#define	TX_MLS			6
#define	SEXT			4
#define	RX_PDM			2
#define	TX_PDM			0

/* SUNXI_DAUDIO_INTSTA:0x0C */
#define	TXU_INT			6
#define	TXO_INT			5
#define	TXE_INT			4
#define	RXU_INT			2
#define RXO_INT			1
#define	RXA_INT			0

/* SUNXI_DAUDIO_FIFOCTL:0x14 */
#define	HUB_EN			31
#define	FIFO_CTL_FTX		25
#define	FIFO_CTL_FRX		24
#define	TXTL			12
#define	RXTL			4
#define	TXIM			2
#define	RXOM			0

/* SUNXI_DAUDIO_FIFOSTA:0x18 */
#define	FIFO_TXE		28
#define	FIFO_TX_CNT		16
#define	FIFO_RXA		8
#define	FIFO_RX_CNT		0

/* SUNXI_DAUDIO_INTCTL:0x1C */
#define	TXDRQEN			7
#define	TXUI_EN			6
#define	TXOI_EN			5
#define	TXEI_EN			4
#define	RXDRQEN			3
#define	RXUIEN			2
#define	RXOIEN			1
#define	RXAIEN			0

/* SUNXI_DAUDIO_CLKDIV:0x24 */
#define	MCLKOUT_EN		8
#define	BCLK_DIV		4
#define	MCLK_DIV		0

/* SUNXI_DAUDIO_CHCFG:0x30 */
#define	TX_SLOT_HIZ		9
#define	TX_STATE		8
#define	RX_SLOT_NUM		4
#define	TX_SLOT_NUM		0

/* SUNXI_DAUDIO_TXnCHSEL:0X34+n*0x04 */
#if	defined(CONFIG_ARCH_SUN8IW10)
#define	TX_OFFSET		20
#define	TX_CHSEL		16
#define	TX_CHEN			0
#else
#define	TX_OFFSET		12
#define	TX_CHEN			4
#define	TX_CHSEL		0
#endif

/* SUNXI_DAUDIO_RXCHSEL */
#if	defined(CONFIG_ARCH_SUN8IW10)
#define	RX_OFFSET		20
#define	RX_CHSEL		16
#else
#define	RX_OFFSET		12
#define	RX_CHSEL		0
#endif

/* sun8iw10 CHMAP default setting */
#define	SUNXI_DEFAULT_CHMAP0	0x76543210
#define	SUNXI_DEFAULT_CHMAP1	0xFEDCBA98

/* RXCHMAP default setting */
#define	SUNXI_DEFAULT_CHMAP	0x76543210

/* Shift & Mask define */

/* SUNXI_DAUDIO_CTL:0x00 */
#define	SUNXI_DAUDIO_MODE_CTL_MASK		3
#define	SUNXI_DAUDIO_MODE_CTL_PCM		0
#define	SUNXI_DAUDIO_MODE_CTL_I2S		1
#define	SUNXI_DAUDIO_MODE_CTL_LEFT		1
#define	SUNXI_DAUDIO_MODE_CTL_RIGHT		2
#define	SUNXI_DAUDIO_MODE_CTL_REVD		3
/* combine LRCK_CLK & BCLK setting */
#define	SUNXI_DAUDIO_LRCK_OUT_MASK		3
#define	SUNXI_DAUDIO_LRCK_OUT_DISABLE	0
#define	SUNXI_DAUDIO_LRCK_OUT_ENABLE	3

/* SUNXI_DAUDIO_FMT0 */
#define	SUNXI_DAUDIO_LRCK_PERIOD_MASK	0x3FF
#define	SUNXI_DAUDIO_SLOT_WIDTH_MASK	7
/* Left Blank */
#define	SUNXI_DAUDIO_SR_MASK			7
#define	SUNXI_DAUDIO_SR_16BIT			3
#define	SUNXI_DAUDIO_SR_24BIT			5
#define	SUNXI_DAUDIO_SR_32BIT			7

#define	SUNXI_DAUDIO_LRCK_POLARITY_NOR	0
#define	SUNXI_DAUDIO_LRCK_POLARITY_INV	1
#define	SUNXI_DAUDIO_BCLK_POLARITY_NOR	0
#define	SUNXI_DAUDIO_BCLK_POLARITY_INV	1

/* SUNXI_DAUDIO_FMT1 */
#define	SUNXI_DAUDIO_FMT1_DEF			0x30

/* SUNXI_DAUDIO_FIFOCTL */
#define	SUNXI_DAUDIO_TXIM_MASK			1
#define	SUNXI_DAUDIO_TXIM_VALID_MSB		0
#define	SUNXI_DAUDIO_TXIM_VALID_LSB		1
/* Left Blank */
#define	SUNXI_DAUDIO_RXOM_MASK			3
/* Expanding 0 at LSB of RX_FIFO */
#define	SUNXI_DAUDIO_RXOM_EXP0			0
/* Expanding sample bit at MSB of RX_FIFO */
#define	SUNXI_DAUDIO_RXOM_EXPH			1
/* Fill RX_FIFO low word be 0 */
#define	SUNXI_DAUDIO_RXOM_TUNL			2
/* Fill RX_FIFO high word be higher sample bit */
#define	SUNXI_DAUDIO_RXOM_TUNH			3

/* SUNXI_DAUDIO_CLKDIV */
#define	SUNXI_DAUDIO_BCLK_DIV_MASK		0xF
#define	SUNXI_DAUDIO_BCLK_DIV_1			1
#define	SUNXI_DAUDIO_BCLK_DIV_2			2
#define	SUNXI_DAUDIO_BCLK_DIV_3			3
#define	SUNXI_DAUDIO_BCLK_DIV_4			4
#define	SUNXI_DAUDIO_BCLK_DIV_5			5
#define	SUNXI_DAUDIO_BCLK_DIV_6			6
#define	SUNXI_DAUDIO_BCLK_DIV_7			7
#define	SUNXI_DAUDIO_BCLK_DIV_8			8
#define	SUNXI_DAUDIO_BCLK_DIV_9			9
#define	SUNXI_DAUDIO_BCLK_DIV_10		10
#define	SUNXI_DAUDIO_BCLK_DIV_11		11
#define	SUNXI_DAUDIO_BCLK_DIV_12		12
#define	SUNXI_DAUDIO_BCLK_DIV_13		13
#define	SUNXI_DAUDIO_BCLK_DIV_14		14
#define	SUNXI_DAUDIO_BCLK_DIV_15		15
/* Left Blank */
#define	SUNXI_DAUDIO_MCLK_DIV_MASK		0xF
#define	SUNXI_DAUDIO_MCLK_DIV_1			1
#define	SUNXI_DAUDIO_MCLK_DIV_2			2
#define	SUNXI_DAUDIO_MCLK_DIV_3			3
#define	SUNXI_DAUDIO_MCLK_DIV_4			4
#define	SUNXI_DAUDIO_MCLK_DIV_5			5
#define	SUNXI_DAUDIO_MCLK_DIV_6			6
#define	SUNXI_DAUDIO_MCLK_DIV_7			7
#define	SUNXI_DAUDIO_MCLK_DIV_8			8
#define	SUNXI_DAUDIO_MCLK_DIV_9			9
#define	SUNXI_DAUDIO_MCLK_DIV_10		10
#define	SUNXI_DAUDIO_MCLK_DIV_11		11
#define	SUNXI_DAUDIO_MCLK_DIV_12		12
#define	SUNXI_DAUDIO_MCLK_DIV_13		13
#define	SUNXI_DAUDIO_MCLK_DIV_14		14
#define	SUNXI_DAUDIO_MCLK_DIV_15		15

/* SUNXI_DAUDIO_CHCFG */
#define	SUNXI_DAUDIO_TX_SLOT_MASK		7
#define	SUNXI_DAUDIO_RX_SLOT_MASK		7

/* SUNXI_DAUDIO_TX0CHSEL: */
#define	SUNXI_DAUDIO_TX_OFFSET_MASK		3
#define	SUNXI_DAUDIO_TX_OFFSET_0		0
#define	SUNXI_DAUDIO_TX_OFFSET_1		1
/* Left Blank */
#define	SUNXI_DAUDIO_TX_CHEN_MASK		0xFF
#define	SUNXI_DAUDIO_TX_CHSEL_MASK		7

/* SUNXI_DAUDIO_RXCHSEL */
#ifndef	CONFIG_ARCH_SUN8IW10
#define SUNXI_DAUDIO_RX_OFFSET_MASK		1
#else
#define SUNXI_DAUDIO_RX_OFFSET_MASK		3
#endif
#define	SUNXI_DAUDIO_RX_CHSEL_MASK		7

#define	SND_SOC_DAIFMT_SIG_SHIFT		8
#define	SND_SOC_DAIFMT_MASTER_SHIFT		12

#ifndef	CONFIG_SND_SUNXI_SOC_HDMIAUDIO
static inline int sndhdmi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	return 0;
}

static inline int sndhdmi_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	return 0;
}

static inline void sndhdmi_shutdown(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	return;
}

#else	/* !CONFIG_SND_SUNXI_SOC_HDMIAUDIO */
extern int sndhdmi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai);
extern void sndhdmi_shutdown(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai);
extern int sndhdmi_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai);
#endif	/* CONFIG_SND_SUNXI_SOC_HDMIAUDIO */

#endif	/* __SUNXI_DAUDIO_H_ */

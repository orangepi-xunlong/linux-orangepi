/* sound\soc\sunxi\snd_sunxi_daudio.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_DAUDIO_H
#define __SND_SUNXI_DAUDIO_H

/* DAUDIO register definition */
#define	SUNXI_DAUDIO_CTL		0x00
#define	SUNXI_DAUDIO_FMT0		0x04
#define	SUNXI_DAUDIO_FMT1		0x08
#define	SUNXI_DAUDIO_INTSTA		0x0C
#define	SUNXI_DAUDIO_RXFIFO		0x10
#define	SUNXI_DAUDIO_FIFOCTL		0x14
#define	SUNXI_DAUDIO_FIFOSTA		0x18
#define	SUNXI_DAUDIO_INTCTL		0x1C
#define	SUNXI_DAUDIO_TXFIFO		0x20
#define	SUNXI_DAUDIO_CLKDIV		0x24
#define	SUNXI_DAUDIO_TXCNT		0x28
#define	SUNXI_DAUDIO_RXCNT		0x2C
#define	SUNXI_DAUDIO_CHCFG		0x30
#define	SUNXI_DAUDIO_TX0CHSEL		0x34
#define	SUNXI_DAUDIO_TX1CHSEL		0x38
#define	SUNXI_DAUDIO_TX2CHSEL		0x3C
#define	SUNXI_DAUDIO_TX3CHSEL		0x40
#define	SUNXI_DAUDIO_TX0CHMAP0		0x44
#define	SUNXI_DAUDIO_TX0CHMAP1		0x48
#define	SUNXI_DAUDIO_TX1CHMAP0		0x4C
#define	SUNXI_DAUDIO_TX1CHMAP1		0x50
#define	SUNXI_DAUDIO_TX2CHMAP0		0x54
#define	SUNXI_DAUDIO_TX2CHMAP1		0x58
#define	SUNXI_DAUDIO_TX3CHMAP0		0x5C
#define	SUNXI_DAUDIO_TX3CHMAP1		0x60
#define	SUNXI_DAUDIO_RXCHSEL		0x64
#define	SUNXI_DAUDIO_RXCHMAP0		0x68
#define	SUNXI_DAUDIO_RXCHMAP1		0x6C
#define	SUNXI_DAUDIO_RXCHMAP2		0x70
#define	SUNXI_DAUDIO_RXCHMAP3		0x74

#define	SUNXI_DAUDIO_DEBUG		0x78
#define	SUNXI_DAUDIO_REV		0x7C

#define SUNXI_DAUDIO_MAX_REG		SUNXI_DAUDIO_REV

/* SUNXI_DAUDIO_CTL:0x00 */
#define RX_SYNC_EN_START		21
#define RX_SYNC_EN			20
#define	BCLK_OUT			18
#define	LRCK_OUT			17
#define	LRCKR_CTL			16
#define	SDO3_EN				11
#define	SDO2_EN				10
#define	SDO1_EN				9
#define	SDO0_EN				8
#define	MUTE_CTL			6
#define	MODE_SEL			4
#define	LOOP_EN				3
#define	CTL_TXEN			2
#define	CTL_RXEN			1
#define	GLOBAL_EN			0

/* SUNXI_DAUDIO_FMT0:0x04 */
#define	SDI_SYNC_SEL			31
#define	LRCK_WIDTH			30
#define	LRCKR_PERIOD			20
#define	LRCK_POLARITY			19
#define	LRCK_PERIOD			8
#define	BRCK_POLARITY			7
#define	DAUDIO_SAMPLE_RESOLUTION	4
#define	EDGE_TRANSFER			3
#define	SLOT_WIDTH			0

/* SUNXI_DAUDIO_FMT1:0x08 */
#define	RX_MLS				7
#define	TX_MLS				6
#define	SEXT				4
#define	RX_PDM				2
#define	TX_PDM				0

/* SUNXI_DAUDIO_INTSTA:0x0C */
#define	TXU_INT				6
#define	TXO_INT				5
#define	TXE_INT				4
#define	RXU_INT				2
#define RXO_INT				1
#define	RXA_INT				0

/* SUNXI_DAUDIO_FIFOCTL:0x14 */
#define	HUB_EN				31
#define	FIFO_CTL_FTX			25
#define	FIFO_CTL_FRX			24
#define	TXTL				12
#define	RXTL				4
#define	TXIM				2
#define	RXOM				0

/* SUNXI_DAUDIO_FIFOSTA:0x18 */
#define	FIFO_TXE			28
#define	FIFO_TX_CNT			16
#define	FIFO_RXA			8
#define	FIFO_RX_CNT			0

/* SUNXI_DAUDIO_INTCTL:0x1C */
#define	TXDRQEN				7
#define	TXUI_EN				6
#define	TXOI_EN				5
#define	TXEI_EN				4
#define	RXDRQEN				3
#define	RXUIEN				2
#define	RXOIEN				1
#define	RXAIEN				0

/* SUNXI_DAUDIO_CLKDIV:0x24 */
#define	MCLKOUT_EN			8
#define	BCLK_DIV			4
#define	MCLK_DIV			0

/* SUNXI_DAUDIO_CHCFG:0x30 */
#define	TX_SLOT_HIZ			9
#define	TX_STATE			8
#define	RX_SLOT_NUM			4
#define	TX_SLOT_NUM			0

/* SUNXI_DAUDIO_TXnCHSEL:0X34+n*0x04 */
#define	TX_OFFSET			20
#define	TX_CHSEL			16
#define	TX_CHEN				0

/* SUNXI_DAUDIO_RXCHSEL */
#define	RX_OFFSET			20
#define	RX_CHSEL			16

/* CHMAP default setting */
#define	SUNXI_DEFAULT_TXCHMAP0		0xFEDCBA98
#define	SUNXI_DEFAULT_TXCHMAP1		0x76543210

/* RXCHMAP default setting */
#define	SUNXI_DEFAULT_RXCHMAP0		0x0F0E0D0C
#define	SUNXI_DEFAULT_RXCHMAP1		0x0B0A0908
#define	SUNXI_DEFAULT_RXCHMAP2		0x07060504
#define	SUNXI_DEFAULT_RXCHMAP3		0x03020100

/* Shift & Mask define */
/* Left Blank */
#define	SUNXI_DAUDIO_SR_MASK			7
#define	SUNXI_DAUDIO_SR_16BIT			3
#define	SUNXI_DAUDIO_SR_24BIT			5
#define	SUNXI_DAUDIO_SR_32BIT			7
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

/* SUNXI_DAUDIO_CHCFG */
#define	SUNXI_DAUDIO_TX_SLOT_MASK		0XF
#define	SUNXI_DAUDIO_RX_SLOT_MASK		0XF

/* Left Blank */
#define	SUNXI_DAUDIO_TX_CHEN_MASK		0xFFFF
#define	SUNXI_DAUDIO_TX_CHSEL_MASK		0xF

/* SUNXI_DAUDIO_RXCHSEL */
#define SUNXI_DAUDIO_RX_OFFSET_MASK		3
#define	SUNXI_DAUDIO_RX_CHSEL_MASK		0XF

/* To clear FIFO */
#define SUNXI_DAUDIO_FTX_TIMES			10

struct sunxi_daudio_mem_info {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_daudio_clk_info {
	struct clk *clk_pll_audio;
	struct clk *clk_i2s;

	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

struct sunxi_daudio_pinctl_info {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;

	bool pinctrl_used;
};

struct sunxi_daudio_dts_info {
	/* value must be (2^n)Kbyte */
	size_t playback_cma;
	size_t playback_fifo_size;
	size_t capture_cma;
	size_t capture_fifo_size;

	unsigned int dai_type;
	unsigned int tdm_num;
	unsigned int tx_pin;
	unsigned int rx_pin;

	/* tx_hub */
	bool tx_hub_en;

	/* components func -> rx_sync */
	bool rx_sync_en;	/* read from dts */
	bool rx_sync_ctl;
	int rx_sync_id;
	rx_sync_domain_t rx_sync_domain;
};

struct sunxi_daudio_regulator_info {
	struct regulator *daudio_regulator;
	const char *regulator_name;
};

struct sunxi_daudio_info {
	struct platform_device *pdev;

	struct sunxi_daudio_mem_info mem_info;
	struct sunxi_daudio_clk_info clk_info;
	struct sunxi_daudio_pinctl_info pin_info;
	struct sunxi_daudio_dts_info dts_info;
	struct sunxi_daudio_regulator_info rglt_info;

	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;

	/* for Hardware param setting */
	unsigned int fmt;
	unsigned int pllclk_freq;
	unsigned int moduleclk_freq;
	unsigned int mclk_freq;
	unsigned int lrck_freq;
	unsigned int bclk_freq;
};

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
static inline int snd_sunxi_sysfs_create_group(struct platform_device *pdev,
					       struct attribute_group *attr)
{
	return sysfs_create_group(&pdev->dev.kobj, attr);
}

static inline void snd_sunxi_sysfs_remove_group(struct platform_device *pdev,
						struct attribute_group *attr)
{
	sysfs_remove_group(&pdev->dev.kobj, attr);
}
#else
static inline int snd_sunxi_sysfs_create_group(struct platform_device *pdev,
					       struct attribute_group *attr)
{
	return 0;
}

static inline void snd_sunxi_sysfs_remove_group(struct platform_device *pdev,
						struct attribute_group *attr)
{
	return;
}
#endif

#endif /* __SND_SUNXI_DAUDIO_H */

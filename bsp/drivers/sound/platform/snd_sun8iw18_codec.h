/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, lijingpsw <lijingpsw@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _SUN8IW18_CODEC_H
#define _SUN8IW18_CODEC_H

#define SUNXI_DAC_DPC		0x00
#define SUNXI_DAC_FIFO_CTL	0x10
#define SUNXI_DAC_FIFO_STA	0x14
#define SUNXI_DAC_TXDATA	0X20
#define SUNXI_DAC_CNT		0x24
#define SUNXI_DAC_DG		0x28

#define SUNXI_ADC_FIFO_CTL	0x30
#define SUNXI_ADC_FIFO_STA	0x38
#define SUNXI_ADC_RXDATA	0x40
#define SUNXI_ADC_CNT		0x44
#define SUNXI_ADC_DG		0x4C

/* DAP */
#define SUNXI_DAC_DAP_CTL 	0xf0
#define SUNXI_ADC_DAP_CTL 	0xf8

#define SUNXI_AC_VERSION	(0x2c0)
#define AC_DAC_DRC_CTL		(0x108)
/* DAC_DRC Control Register */

#define DAC_DRC_CTL_CONTROL_DRC_EN	(4)
#define DAC_DRC_CTL_SIGNAL_FUN_SEL	(3)
#define DAC_DRC_CTL_DEL_FUN_EN		(2)
#define DAC_DRC_CTL_DRC_LT_EN		(1)
#define DAC_DRC_CTL_DRC_ET_EN		(0)

/* Analog register base - Digital register base */
/* SUNXI_PR_CFG is to tear the acreg and dcreg, it is of no real meaning */
#define SUNXI_PR_CFG		0x300
#define SUNXI_HP_CTL		(SUNXI_PR_CFG + 0x00)
#define SUNXI_MIX_DAC_CTL	(SUNXI_PR_CFG + 0x03)
#define SUNXI_LINEOUT_CTL0	(SUNXI_PR_CFG + 0x05)
#define SUNXI_LINEOUT_CTL1	(SUNXI_PR_CFG + 0x06)
#define SUNXI_MIC1_CTL		(SUNXI_PR_CFG + 0x07)
#define SUNXI_MIC2_MIC3_CTL	(SUNXI_PR_CFG + 0x08)
#define SUNXI_LADCMIX_SRC	(SUNXI_PR_CFG + 0x09)
#define SUNXI_RADCMIX_SRC	(SUNXI_PR_CFG + 0x0A)
#define SUNXI_XADCMIX_SRC	(SUNXI_PR_CFG + 0x0B)

#define SUNXI_ADC_CTL		(SUNXI_PR_CFG + 0x0D)
#define SUNXI_MBIAS_CTL		(SUNXI_PR_CFG + 0x0E)
#define SUNXI_APT_REG		(SUNXI_PR_CFG + 0x0F)
#define SUNXI_OP_BIAS_CTL0	(SUNXI_PR_CFG + 0x10)
#define SUNXI_OP_BIAS_CTL1	(SUNXI_PR_CFG + 0x11)
#define SUNXI_ZC_VOL_CTL	(SUNXI_PR_CFG + 0x12)
#define SUNXI_BIAS_CAL_CTRL	(SUNXI_PR_CFG + 0x15)

#define SUNXI_AUDIO_MAX_REG	SUNXI_PR_CFG
#define SUNXI_AUDIO_MAX_REG_PR	SUNXI_BIAS_CAL_CTRL

/* SUNXI_DAC_DPC:0x00 */
#define EN_DAC			31
#define MODQU			25
#define DWA_EN			24
#define HPF_EN			18
#define DVOL			12
#define DAC_HUB_EN		0

/* SUNXI_DAC_FIFO_CTL:0x10 */
#define DAC_FS			29
#define FIR_VER			28
#define SEND_LASAT		26
#define FIFO_MODE		24
#define DAC_DRQ_CLR_CNT		21
#define TX_TRIG_LEVEL		8
#define DAC_MONO_EN		6
#define TX_SAMPLE_BITS		5
#define DAC_DRQ_EN		4
#define DAC_IRQ_EN		3
#define FIFO_UNDERRUN_IRQ_EN	2
#define FIFO_OVERRUN_IRQ_EN	1
#define FIFO_FLUSH		0

/* SUNXI_DAC_FIFO_STA:0x14 */
#define TX_EMPTY		23
#define DAC_TXE_CNT		8
#define DAC_TXE_INT		3
#define DAC_TXU_INT		2
#define DAC_TXO_INT		1

/* SUNXI_DAC_DG:0x28 */
#define DAC_MODU_SEL		11
#define DAC_PATTERN_SEL		9
#define CODEC_CLK_SELECT	8
#define DA_SWP			6
#define ADDA_LOOP_MODE		0

/* SUNXI_ADC_FIFO_CTL:0x30 */
#define ADC_FS			29
#define EN_AD			28
#define ADCFDT			26
#define ADCDFEN			25
#define RX_FIFO_MODE		24
#define RX_SAMPLE_BITS		16
#define ADCX_CHAN_SEL		14
#define ADCR_CHAN_SEL		13
#define ADCL_CHAN_SEL		12
#define RX_FIFO_TRG_LEVEL	4
#define ADC_DRQ_EN		3
#define ADC_IRQ_EN		2
#define ADC_OVERRUN_IRQ_EN	1
#define ADC_FIFO_FLUSH		0

/* SUNXI_ADC_FIFO_STA:0x38 */
#define ADC_RXA			23
#define ADC_RXA_CNT		8
#define ADC_RXA_INT		3
#define ADC_RXO_INT		1

/* SUNXI_ADC_DG:0x4C */
#define AD_SWP			24

/* SUNXI_DAC_DAP_CTL:0xf0 */
#define DDAP_EN			31
#define DDAP_DRC_EN		29
#define DDAP_HPF_EN		28

/* SUNXI_ADC_DAP_CTL:0xf8 */
#define ADC_DAP0_EN		31
#define ADC_DRC0_EN		29
#define ADC_HPF0_EN		28
#define ADC_DAP1_EN		27
#define ADC_DRC1_EN		25
#define ADC_HPF1_EN		24

/* SUNXI_PR_CFG:0x07010280 */
#define AC_PR_RST		28
#define AC_PR_RW		24
#define AC_PR_ADDR		16
#define ADDA_PR_WDAT		8
#define ADDA_PR_RDAT		0

/* Analog domain */

/* SUNXI_HP_CTL:0x00 */
#define PA_CLK_GATE		7

/* SUNXI_MIX_DAC_CTL:0x03 */
#define DACALEN			6

/* SUNXI_LINEOUT_CTL0:0x05 */
#define LINEOUTL_EN		7
#define LINEOUTR_EN		6
#define LINEOUTL_SRC		5
#define LINEOUTR_SRC		4

/* SUNXI_LINEOUT_CTL1:0x06 */
#define LINEOUT_VOL		0

/* SUNXI_MIC1_CTL:0x07 */
#define MIC1AMPEN		3
#define MIC1BOOST		0

/* SUNXI_MIC2_MIC3_CTL:0x08 */
#define MIC3AMPEN		7
#define MIC3BOOST		4
#define MIC2AMPEN		3
#define MIC2BOOST		0

/* SUNXI_LADCMIX_CTL:0x09 */
#define LADC_MIC3_BST		4
#define LADC_MIC2_BST		3
#define LADC_MIC1_BST		2
#define LADC_DACL		1

/* SUNXI_RADCMIX_CTL:0x0A */
#define RADC_MIC3_BST		4
#define RADC_MIC2_BST		3
#define RADC_MIC1_BST		2
#define RADC_DACL		0

/* SUNXI_XADCMIX_CTL:0x0B */
#define XADC_MIC3_BST		4
#define XADC_MIC2_BST		3
#define XADC_MIC1_BST		2
#define XADC_DACL		1

/* SUNXI_ADC_CTL:0x0D */
#define ADCREN			7
#define ADCLEN			6
#define ADCXEN			4
#define DITHER_SEL		3
#define ADCG			0

/* SUNXI_MBIAS_CTL:0x0E */
#define MMICBIASEN		7
#define MBIASSEL		5

/* SUNXI_APT_REG:0x0F */
#define MMIC_BIAS_CHOP_EN	7
#define MMIC_BIAS_CHOP_CLK_SEL	5
#define DITHER			4
#define DITHER_CLK_SEL		2
#define BIHE_CTRL		0

/* SUNXI_OP_BIAS_CTL0:0x10 */
#define OPDRV_OPEAR_CUR		6
#define OPADC1_BIAS_CUR		4
#define OPADC2_BIAS_CUR		2
#define OPAAF_BIAS_CUR		0

/* SUNXI_OP_BIAS_CTL1:0x11 */
#define OPMIC_BIAS_CUR		6
#define OPVR_BIAS_CUR		4
#define OPDAC_BIAS_CUR		2
#define OPMIX_BIAS_CUR		0

/* SUNXI_ZC_VOL_CTL:0x12 */
#define ZC_EN			7
#define ZC_TIMEOUT_SEL		6
#define USB_BIAS_CUR		0

/* SUNXI_BIAS_CAL_CTRL:0x15 */
#define CUR_TEST_SEL		6


#define LABEL(constant)		{#constant, constant, 0}
#define LABEL_END		{NULL, 0, -1}

struct sunxi_codec_dap {
	unsigned int dap_enable;
	struct mutex mutex;
};

struct sunxi_codec_dts {
	u32 mic1_gain;
	u32 mic2_gain;
	u32 mic3_gain;

	u32 adc_gain;

	u32 dac_dig_vol;
	u32 lineout_vol;

	u32 adc_delay_time;

	bool lineout_single;
	bool tx_hub_en;		/* tx_hub */
};

struct sunxi_audio_status {
	struct mutex mic_mutex;
	bool mic1;
	bool mic2;
	bool mic3;
};

struct sunxi_codec_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_codec_clk {
	struct reset_control *clk_rst;
	struct clk *clk_bus_audio;
	struct clk *clk_pll_audio;
	struct clk *clk_audio;
};

struct sunxi_codec_rglt {
	u32 avcc_vol;
	bool avcc_external;
	struct regulator *avcc;
};

struct sunxi_codec {
	const char *module_version;
	struct platform_device *pdev;

	struct sunxi_codec_mem mem;
	struct sunxi_codec_clk clk;
	struct sunxi_codec_rglt rglt;
	struct sunxi_codec_dts dts;

	enum SND_SUNXI_CLK_STATUS clk_sta;

	struct sunxi_codec_dap dac_dap;
	struct sunxi_codec_dap adc_dap;

	struct sunxi_audio_status audio_sts;
	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;

	/* debug */
	char module_name[32];
	struct snd_sunxi_dump dump;
	bool show_reg_all;
};

#endif /* __SUN8IW18_CODEC_H */

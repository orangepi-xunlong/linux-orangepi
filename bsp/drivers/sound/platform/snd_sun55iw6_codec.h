/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, zhouxijing <zhouxijing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUN55IW6_CODEC_H
#define __SND_SUN55IW6_CODEC_H

/* REG-Digital */
#define SUNXI_DAC_DPC		0x00
#define SUNXI_DAC_VOL_CTL	0x04
#define SUNXI_DAC_FIFO_CTL	0x10
#define SUNXI_DAC_FIFO_STA	0x14
#define SUNXI_DAC_TXDATA	0X20
#define SUNXI_DAC_CNT		0x24
#define SUNXI_DAC_DEBUG		0x28

#define SUNXI_VAR1SPEEDUP_DOWN_CTL	0x54

#define SUNXI_ANA_DEBUG_REG2	0x5c

#define SUNXI_DAC_DAP_CTL	0xF0
#define SUNXI_DAC_DRC_CTL	0x108

#define SUNXI_VERSION		0x2C0

/* REG-Analog */
#define SUNXI_DAC_AN_REG	0x310

#define SUNXI_RAMP_REG		0x31C
#define SUNXI_BIAS_AN_CTL	0x320
#define SUNXI_AUDIO_MAX_REG	SUNXI_BIAS_AN_CTL

/* BITS */
/* SUNXI_DAC_DPC:0x00 */
#define DAC_DIG_EN		31
#define MODQU			25
#define DWA_EN			24
#define HPF_EN			18
#define DVOL			12
#define DITHER_SGM		8
#define DITHER_SFT		4
#define DITHER_EN		1
#define HUB_EN			0
/* SUNXI_DAC_VOL_CTL:0x04 */
#define DAC_VOL_SEL		16
#define DAC_VOL_L		8
/* SUNXI_DAC_FIFO_CTL:0x10 */
#define DAC_FS			29
#define FIR_VER			28
#define SEND_LASAT		26
#define DAC_FIFO_MODE		24
#define DAC_DRQ_CLR_CNT		21
#define TX_TRIG_LEVEL		8
#define DAC_MONO_EN		6
#define TX_SAMPLE_BITS		5
#define DAC_DRQ_EN		4
#define DAC_IRQ_EN		3
#define DAC_FIFO_UNDERRUN_IRQ_EN	2
#define DAC_FIFO_OVERRUN_IRQ_EN		1
#define DAC_FIFO_FLUSH		0
/* SUNXI_DAC_FIFO_STA:0x14 */
#define	DAC_TX_EMPTY		23
#define	DAC_TXE_CNT		8
#define	DAC_TXE_INT		3
#define	DAC_TXU_INT		2
#define	DAC_TXO_INT		1
/* SUNXI_DAC_DEBUG:0x28 */
#define	DAC_MODU_SEL		11
#define	DAC_PATTERN_SEL		9
#define	CODEC_CLK_SEL		8
#define	DA_SWP			6
/* SUNXI_VAR1SPEEDUP_DOWN_CTL:0x54 */
#define VRA1SPEEDUP_DOWN_STATE		4
#define VRA1SPEEDUP_DOWN_CTL		1
#define VRA1SPEEDUP_DOWN_RST_CTL	0
/* SUNXI_DAC_DAP_CTL:0xF0 */
#define DDAP_EN			31
#define DDAP_DRC_EN		29
#define DDAP_HPF_EN		28
/* SUNXI_DAC_AN_REG:0x310 */
#define CURRENT_TEST_SEL		31
#define VRA1SPEEDUP_DIS_MANUAL  	22
#define IOPVRS				20
#define ILINEOUTAMPS			18
#define IOPDACS				16
#define DACL_EN				15
#define LINEOUTL_EN			13
#define LMUTE				12
#define OPVR_OI_CTRL			7
#define LINEOUT_DIFFEN			6
#define LINEOUT_GAIN			0
/* SUNXI_RAMP_REG:0x31C */
#define RMC_EN				1
#define RD_EN				0
/* SUNXI_BIAS_AN_CTL:0x320 */
#define BIASDATA			0

#define DACDRC_SHIFT		1
#define DACHPF_SHIFT		2

struct sunxi_codec_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_codec_clk {
	/* parent */
	struct clk *clk_pll_audio0_4x;
	struct clk *clk_pll_audio1_4x;
	/* module */
	struct clk *clk_audio_dac;
	/* bus & reset */
	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

struct sunxi_codec_rglt {
	/* supply power to the bias */
	u32 avcc_vol;
	bool avcc_external;
	struct regulator *avcc;
};

struct sunxi_codec_dts {
	/* tx_hub */
	bool tx_hub_en;

	/* volume & gain */
	u32 dac_vol;
	u32 dacl_vol;
	u32 dacr_vol;
	u32 lineout_gain;
};

struct sunxi_audio_status {
	struct mutex apf_mutex; /* audio playback function mutex lock */
	bool spk;
};

struct sunxi_codec {
	const char *module_version;
	struct platform_device *pdev;

	struct sunxi_codec_mem mem;
	struct sunxi_codec_clk clk;
	struct sunxi_codec_dts dts;
	struct snd_sunxi_rglt *rglt;
	struct sunxi_audio_status audio_sta;
	enum SND_SUNXI_CLK_STATUS clk_sta;

	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;

	/* debug */
	char module_name[32];
	struct snd_sunxi_dump dump;
	bool show_reg_all;
};

#endif /* __SND_SUN55IW6_CODEC_H */

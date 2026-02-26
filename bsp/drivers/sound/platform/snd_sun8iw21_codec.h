/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2025 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, xudongpdc <xudongpdc@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */


#ifndef __SND_SUN8IW21_CODEC_H
#define __SND_SUN8IW21_CODEC_H

/* REG-Digital */
#define SUNXI_DAC_DPC			0x00
#define SUNXI_DAC_VOL_CTRL		0x04
#define SUNXI_DAC_FIFOC			0x10
#define SUNXI_DAC_FIFOS			0x14

#define SUNXI_DAC_TXDATA		0X20
#define SUNXI_DAC_CNT			0x24
#define SUNXI_DAC_DG			0x28

#define	SUNXI_ADC_FIFOC			0x30
#define	SUNXI_ADC_VOL_CTRL		0x34
#define SUNXI_ADC_FIFOS			0x38
#define SUNXI_ADC_RXDATA		0x40
#define SUNXI_ADC_CNT			0x44
#define SUNXI_ADC_DG			0x4C
#define SUNXI_ADC_DIG_CTRL		0x50
#define SUNXI_VRA1SPEEDUP_DOWN_CTRL	0x54

#define SUNXI_DAC_DAP_CTL	0xF0
#define SUNXI_ADC_DAP_CTL	0xF8

#define SUNXI_DAC_DRC_HHPFC	0x100
#define SUNXI_DAC_DRC_LHPFC	0x104
#define SUNXI_DAC_DRC_CTRL	0x108
#define SUNXI_DAC_DRC_LPFHAT	0x10C
#define SUNXI_DAC_DRC_LPFLAT	0x110
#define SUNXI_DAC_DRC_RPFHAT	0x114
#define SUNXI_DAC_DRC_RPFLAT	0x118
#define SUNXI_DAC_DRC_LPFHRT	0x11C
#define SUNXI_DAC_DRC_LPFLRT	0x120
#define SUNXI_DAC_DRC_RPFHRT	0x124
#define SUNXI_DAC_DRC_RPFLRT	0x128
#define SUNXI_DAC_DRC_LRMSHAT	0x12C
#define SUNXI_DAC_DRC_LRMSLAT	0x130
#define SUNXI_DAC_DRC_RRMSHAT	0x134
#define SUNXI_DAC_DRC_RRMSLAT	0x138
#define SUNXI_DAC_DRC_HCT	0x13C
#define SUNXI_DAC_DRC_LCT	0x140
#define SUNXI_DAC_DRC_HKC	0x144
#define SUNXI_DAC_DRC_LKC	0x148
#define SUNXI_DAC_DRC_HOPC	0x14C
#define SUNXI_DAC_DRC_LOPC	0x150
#define SUNXI_DAC_DRC_HLT	0x154
#define SUNXI_DAC_DRC_LLT	0x158
#define SUNXI_DAC_DRC_HKI	0x15C
#define SUNXI_DAC_DRC_LKI	0x160
#define SUNXI_DAC_DRC_HOPL	0x164
#define SUNXI_DAC_DRC_LOPL	0x168
#define SUNXI_DAC_DRC_HET	0x16C
#define SUNXI_DAC_DRC_LET	0x170
#define SUNXI_DAC_DRC_HKE	0x174
#define SUNXI_DAC_DRC_LKE	0x178
#define SUNXI_DAC_DRC_HOPE	0x17C
#define SUNXI_DAC_DRC_LOPE	0x180
#define SUNXI_DAC_DRC_HKN	0x184
#define SUNXI_DAC_DRC_LKN	0x188
#define SUNXI_DAC_DRC_SFHAT	0x18C
#define SUNXI_DAC_DRC_SFLAT	0x190
#define SUNXI_DAC_DRC_SFHRT	0x194
#define SUNXI_DAC_DRC_SFLRT	0x198
#define SUNXI_DAC_DRC_MXGHS	0x19C
#define SUNXI_DAC_DRC_MXGLS	0x1A0
#define SUNXI_DAC_DRC_MNGHS	0x1A4
#define SUNXI_DAC_DRC_MNGLS	0x1A8
#define SUNXI_DAC_DRC_EPSHC	0x1AC
#define SUNXI_DAC_DRC_EPSLC	0x1B0
#define SUNXI_DAC_DRC_OPT	0x1B4
#define SUNXI_DAC_DRC_HPFHGAIN	0x1B8
#define SUNXI_DAC_DRC_HPFLGAIN	0x1BC

#define SUNXI_ADC_DRC_HHPFC	0x200
#define SUNXI_ADC_DRC_LHPFC	0x204
#define SUNXI_ADC_DRC_CTRL	0x208
#define SUNXI_ADC_DRC_LPFHAT	0x20C
#define SUNXI_ADC_DRC_LPFLAT	0x210
#define SUNXI_ADC_DRC_RPFHAT	0x214
#define SUNXI_ADC_DRC_RPFLAT	0x218
#define SUNXI_ADC_DRC_LPFHRT	0x21C
#define SUNXI_ADC_DRC_LPFLRT	0x220
#define SUNXI_ADC_DRC_RPFHRT	0x224
#define SUNXI_ADC_DRC_RPFLRT	0x228
#define SUNXI_ADC_DRC_LRMSHAT	0x22C
#define SUNXI_ADC_DRC_LRMSLAT	0x230
#define SUNXI_ADC_DRC_HCT	0x23C
#define SUNXI_ADC_DRC_LCT	0x240
#define SUNXI_ADC_DRC_HKC	0x244
#define SUNXI_ADC_DRC_LKC	0x248
#define SUNXI_ADC_DRC_HOPC	0x24C
#define SUNXI_ADC_DRC_LOPC	0x250
#define SUNXI_ADC_DRC_HLT	0x254
#define SUNXI_ADC_DRC_LLT	0x258
#define SUNXI_ADC_DRC_HKI	0x25C
#define SUNXI_ADC_DRC_LKI	0x260
#define SUNXI_ADC_DRC_HOPL	0x264
#define SUNXI_ADC_DRC_LOPL	0x268
#define SUNXI_ADC_DRC_HET	0x26C
#define SUNXI_ADC_DRC_LET	0x270
#define SUNXI_ADC_DRC_HKE	0x274
#define SUNXI_ADC_DRC_LKE	0x278
#define SUNXI_ADC_DRC_HOPE	0x27C
#define SUNXI_ADC_DRC_LOPE	0x280
#define SUNXI_ADC_DRC_HKN	0x284
#define SUNXI_ADC_DRC_LKN	0x288
#define SUNXI_ADC_DRC_SFHAT	0x28C
#define SUNXI_ADC_DRC_SFLAT	0x290
#define SUNXI_ADC_DRC_SFHRT	0x294
#define SUNXI_ADC_DRC_SFLRT	0x298
#define SUNXI_ADC_DRC_MXGHS	0x29C
#define SUNXI_ADC_DRC_MXGLS	0x2A0
#define SUNXI_ADC_DRC_MNGHS	0x2A4
#define SUNXI_ADC_DRC_MNGLS	0x2A8
#define SUNXI_ADC_DRC_EPSHC	0x2AC
#define SUNXI_ADC_DRC_EPSLC	0x2B0
#define SUNXI_ADC_DRC_OPT	0x2B4
#define SUNXI_ADC_DRC_HPFHGAIN	0x2B8
#define SUNXI_ADC_DRC_HPFLGAIN	0x2BC

#define SUNXI_AC_VERSION		0x2C0

/* Analog register */
#define SUNXI_ADC1_REG			0x300
#define SUNXI_ADC2_REG			0x304

#define SUNXI_DAC_REG			0x310
#define SUNXI_MICBIAS_REG		0x318
#define SUNXI_RAMP_REG			0x31C	/* only set bit[1] at init */
#define SUNXI_BIAS_REG			0x320

#define SUNXI_POWER_REG			0x348
#define SUNXI_ADC_CUR_REG		0x34C
#define SUNXI_CODEC_REG_MAX		SUNXI_ADC_CUR_REG

/* SUNXI_DAC_DPC:0x00 */
#define EN_DAC				31
#define MODQU				25
#define DWA_EN				24
#define HPF_EN				18
#define DVOL				12
#define DAC_HUB_EN			0

/* SUNXI_DAC_VOL_CTRL:0x04 */
#define DAC_VOL_SEL			16
#define DAC_VOL_L			8
#define DAC_VOL_R			0

/* SUNXI_DAC_FIFOC:0x10 */
#define DAC_FS				29
#define FIR_VER				28
#define SEND_LASAT			26
#define FIFO_MODE			24
#define DAC_DRQ_CLR_CNT			21
#define TX_TRIG_LEVEL			8
#define DAC_MONO_EN			6
#define TX_SAMPLE_BITS			5
#define DAC_DRQ_EN			4
#define DAC_IRQ_EN			3
#define FIFO_UNDERRUN_IRQ_EN		2
#define FIFO_OVERRUN_IRQ_EN		1
#define FIFO_FLUSH			0

/* SUNXI_DAC_FIFOS:0x14 */
#define	TX_EMPTY			23
#define	DAC_TXE_CNT			8
#define	DAC_TXE_INT			3
#define	DAC_TXU_INT			2
#define	DAC_TXO_INT			1

/* SUNXI_DAC_DG:0x28 */
#define	DAC_MODU_SEL			11
#define	DAC_PATTERN_SEL			9
#define	DAC_CODEC_CLK_SEL		8
#define	DAC_SWP				6
#define	ADDA_LOOP_MODE			0

/* SUNXI_ADC_FIFOC:0x30 */
#define ADC_FS				29
#define EN_AD				28
#define ADCFDT				26
#define ADCDFEN				25
#define RX_FIFO_MODE			24
#define RX_SYNC_EN_START		21
#define RX_SYNC_EN			20
#define RX_SAMPLE_BITS			16
#define RX_FIFO_TRG_LEVEL		4
#define ADC_DRQ_EN			3
#define ADC_IRQ_EN			2
#define ADC_OVERRUN_IRQ_EN		1
#define ADC_FIFO_FLUSH			0

/* SUNXI_ADC_VOL_CTRL:0x34 */
#define ADC2_VOL			8
#define ADC1_VOL			0

/* SUNXI_ADC_FIFOS:0x38 */
#define	RXA				23
#define	ADC_RXA_CNT			8
#define	ADC_RXA_INT			3
#define	ADC_RXO_INT			1

/* SUNXI_ADC_DG:0x4C */
#define	AD_SWP1				24

/* SUNXI_ADC_DIG_CTRL:0x50 */
#define	ADC1_2_VOL_EN			16
#define	ADC2_CHANNEL_EN			1
#define	ADC1_CHANNEL_EN			0
#define	ADC_CHANNEL_EN			0

/* SUNXI_VRA1SPEEDUP_DOWN_CTRL:0x54 */
#define	VRA1SPEEDUP_DOWN_STATE		4
#define	VRA1SPEEDUP_DOWN_CTRL		1
#define	VRA1SPEEDUP_DOWN_RST_CTRL	0

/* SUNXI_DAC_DAP_CTL:0xf0 */
#define	DDAP_EN			31
#define	DDAP_DRC_EN		29
#define	DDAP_HPF_EN		28

/* SUNXI_ADC_DAP_CTL:0xf8 */
#define	ADC_DAP0_EN		31
#define	ADC_DRC0_EN		29
#define	ADC_HPF0_EN		28
#define	ADC_DAP1_EN		27
#define	ADC_DRC1_EN		25
#define	ADC_HPF1_EN		24

/* SUNXI_DAC_DRC_HHPFC: 0x100*/
#define DAC_HHPF_CONF		0

/* SUNXI_DAC_DRC_LHPFC: 0x104*/
#define DAC_LHPF_CONF		0

/* SUNXI_DAC_DRC_CTRL: 0x108*/
#define DAC_DRC_DELAY_OUT_STATE	15
#define DAC_DRC_SIGNAL_DELAY	8
#define DAC_DRC_DELAY_BUF_EN	7
#define DAC_DRC_GAIN_MAX_EN	6
#define DAC_DRC_GAIN_MIN_EN	5
#define DAC_DRC_NOISE_DET_EN	4
#define DAC_DRC_SIGNAL_SEL	3
#define DAC_DRC_DELAY_EN	2
#define DAC_DRC_LT_EN		1
#define DAC_DRC_ET_EN		0

/* SUNXI_ADC_DRC_HHPFC: 0x200*/
#define ADC_HHPF_CONF		0

/* SUNXI_ADC_DRC_LHPFC: 0x204*/
#define ADC_LHPF_CONF		0

/* SUNXI_ADC_DRC_CTRL: 0x208*/
#define ADC_DRC_DELAY_OUT_STATE	15
#define ADC_DRC_SIGNAL_DELAY	8
#define ADC_DRC_DELAY_BUF_EN	7
#define ADC_DRC_GAIN_MAX_EN	6
#define ADC_DRC_GAIN_MIN_EN	5
#define ADC_DRC_NOISE_DET_EN	4
#define ADC_DRC_SIGNAL_SEL	3
#define ADC_DRC_DELAY_EN	2
#define ADC_DRC_LT_EN		1
#define ADC_DRC_ER_EN		0

/* SUNXI_DAC_DRC_HHPFC: 0x100*/
#define DAC_HHPF_CONF			0

/* SUNXI_DAC_DRC_LHPFC: 0x104*/
#define DAC_LHPF_CONF			0

/* SUNXI_ADC_DRC_HHPFC: 0x200*/
#define ADC_HHPF_CONF			0

/* SUNXI_ADC_DRC_LHPFC: 0x204*/
#define ADC_LHPF_CONF			0

/* SUNXI_ADC1_REG : 0x300 */
#define ADC1_EN				31
#define MIC1_PGA_EN			30
#define ADC1_DITHER_CTRL		29
#define MIC1_SIN_EN			28
#define FMINLEN				27
#define FMINLG				26
#define ADC1_DSM_DITHER_LVL		24
#define LINEINLEN			23
#define LINEINLG			22
#define ADC1_IOPBUFFER			20
#define ADC1_PGA_CTRL_RCM		18
#define ADC1_PGA_IN_VCM_CTRL		16
#define ADC1_2_CURRENT_SEL		14
#define ADC1_SINGLE_NOISE_CTL		13
#define ADC1_PGA_GAIN_CTRL		8
#define ADC1_IOPAAF			6
#define ADC1_IOPSDM1			4
#define ADC1_IOPSDM2			2
#define ADC1_IOPMIC			0

/* SUNXI_ADC2_REG : 0x304 */
#define ADC2_EN				31
#define MIC2_PGA_EN			30
#define ADC2_DITHER_CTRL		29
#define MIC2_SIN_EN			28
#define FMINREN				27
#define FMINRG				26
#define ADC2_DSM_DITHER_LVL		24
#define LINEINREN			23
#define LINEINRG			22
#define ADC2_IOPBUFFER			20
#define ADC2_PGA_CTRL_RCM		18
#define ADC2_PGA_IN_VCM_CTRL		16
#define ADC2_SINGLE_NOISE_CTL		13
#define ADC2_PGA_GAIN_CTRL		8
#define ADC2_IOPAAF			6
#define ADC2_IOPSDM1			4
#define ADC2_IOPSDM2			2
#define ADC2_IOPMIC			0

/* SUNXI_DAC_REG : 0x310 */
#define P_CURRENT_TEST_SELECT		24
#define N_CURRENT_TEST_SELECT		22
#define	VRA2_IOPVRS			20
#define	ILINEOUTAMPS			18
#define IOPDACS				16
#define DACLEN				15
#define LINEOUTLEN			13
#define DACLMUTE			12
#define VRA2_OPVR_OI_CTRL		7
#define LINEOUTLDIFFEN			6
#define LINEOUT_VOL			0

/* SUNXI_MICBIAS_REG : 0x318 */
#define MMICBIASEN			7
#define	MBIASSEL			5
#define	MMICBIAS_CHOP_EN		4
#define MMICBIAS_CHOP_CLK_SEL		2

/* SUNXI_RAMP_REG : 0x31C */
#define RMC_EN				1

/* SUNXI_BIAS_REG : 0x320 */
#define AC_BIASDATA			0

/* SUNXI_POWER_REG      :0x348 */
#define ALDO_EN				31
#define VAR1SPEEDUP_DOWN_FURTHER_CTRL	29
#define BG_BUFFER_DISABLE		15
#define ALDO_OUTPUT_VOLTAGE		12
#define BG_ROUGH_TRIM			8
#define BG_FINE_TRIM			0

/* SUNXI_ADC_CUR_REG    :0x34C */
#define ADC2_IOPMIC2			12
#define ADC2_OP_MIC1_CUR		10
#define ADC2_OP_MIC2_CUR		8
#define ADC1_IOPMIC2			4
#define ADC1_OP_MIC1_CUR		2
#define ADC1_OP_MIC2_CUR		0

#define DACDRC_SHIFT		1
#define DACHPF_SHIFT		2
#define ADCDRC0_SHIFT		3
#define ADCHPF0_SHIFT		4
#define ADCDRC1_SHIFT		5
#define ADCHPF1_SHIFT		6

struct sunxi_codec_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_codec_clk {
	/* parent */
	struct clk *clk_pll_audio_1x;
	/* module */
	struct clk *clk_audio_dac;
	struct clk *clk_audio_adc;
	/* bus & reset */
	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

struct sunxi_codec_rglt {
	/* supply power to the audio module */
	u32 vdd_vol;
	bool vdd_external;
	struct regulator *vdd;

	/* supply power to the bias */
	u32 avcc_vol;
	bool avcc_external;
	struct regulator *avcc;

};

struct sunxi_codec_dts {
	/* tx_hub */
	bool tx_hub_en;

	/* rx_sync */
	bool rx_sync_en;
	bool rx_sync_ctl;
	int rx_sync_id;
	rx_sync_domain_t rx_sync_domain;

	/* volume & gain */
	u32 dac_vol;
	u32 dacl_vol;
	u32 adc1_vol;
	u32 adc2_vol;
	u32 lineout_gain;
	u32 adc1_gain;
	u32 adc2_gain;
	u32 adc_dtime;
};

struct sunxi_audio_status {
	struct mutex apf_mutex; /* audio playback function mutex lock */
	bool spk;

	struct mutex acf_mutex; /* audio capture function mutex lock */
	bool mic1;
	bool mic2;
	bool linel;
	bool liner;
};


struct sunxi_codec {
	const char *module_version;
	struct platform_device *pdev;

	struct sunxi_codec_mem mem;
	struct sunxi_codec_clk clk;
	struct sunxi_codec_rglt rglt;
	struct sunxi_codec_dts dts;

	struct sunxi_audio_status audio_sta;
	enum SND_SUNXI_CLK_STATUS clk_sta;

	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;

	/* debug */
	char module_name[32];
	struct snd_sunxi_dump dump;
	bool show_reg_all;

};

#endif /* __SND_SUN8IW21_CODEC_H */

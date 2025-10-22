/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUN65IW1_CODEC_H
#define __SND_SUN65IW1_CODEC_H

/* REG-Digital */
#define SUNXI_DAC_DPC		0x00
#define SUNXI_DAC_VOL_CTL	0x04
#define SUNXI_DAC_FIFO_CTL	0x10
#define SUNXI_DAC_FIFO_STA	0x14
#define SUNXI_DAC_TXDATA	0X20
#define SUNXI_DAC_CNT		0x24
#define SUNXI_DAC_DEBUG		0x28

#define	SUNXI_ADC_FIFO_CTL	0x30
#define SUNXI_ADC_VOL_CTL1	0x34
#define SUNXI_ADC_FIFO_STA	0x38
#define SUNXI_ADC_RXDATA	0x40
#define SUNXI_ADC_CNT		0x44
#define SUNXI_ADC_DEBUG		0x4C
#define SUNXI_ADC_DIG_CTL	0x50

#define SUNXI_VAR1SPEEDUP_DOWN_CTL	0x54

#define SUNXI_AN_DEBUG1		0x58
#define SUNXI_AN_DEBUG2		0x5C

#define SUNXI_DAC_DAP_CTL	0xF0
#define SUNXI_ADC_DAP_CTL	0xF8
#define SUNXI_DAC_DRC_CTL	0x108
#define SUNXI_ADC_DRC_CTL	0x208

#define SUNXI_VERSION		0x2C0

/* REG-Analog */
#define SUNXI_ADC1_AN_CTL	0x300
#define SUNXI_ADC2_AN_CTL	0x304

#define SUNXI_DAC_AN_CTL	0x310
#define SUNXI_DAC2_AN_CTL	0x314

#define SUNXI_MICBIAS_AN_CTL	0x318
#define SUNXI_RAMP_CTL		0x31C
#define SUNXI_BIAS_AN_CTL	0x320
#define SUNXI_HP_AN_CTL		0x324
#define SUNXI_HMIC_CTL		0x328
#define SUNXI_HMIC_STA		0x32C
#define SUNXI_POWER_AN_CTL	0x348
#define SUNXI_AUDIO_MAX_REG	SUNXI_POWER_AN_CTL

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
#define DAC_VOL_R		0
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
#define	AD2DA_LOOP_MODE		12
#define	DAC_MODU_SEL		11
#define	DAC_PATTERN_SEL		9
#define	CODEC_CLK_SEL		8
#define	DAC_SWP			6
#define	DA2AD_LOOP_MODE		0
/* SUNXI_ADC_FIFO_CTL:0x30 */
#define ADC_FS			29
#define ADC_DIG_EN		28
#define ADCFDT			26
#define ADCDFEN			25
#define RX_FIFO_MODE		24
#define RX_SYNC_EN_START	21
#define RX_SYNC_EN		20
#define RX_SAMPLE_BITS		16
#define RX_FIFO_TRG_LEVEL	4
#define ADC_DRQ_EN		3
#define ADC_IRQ_EN		2
#define ADC_OVERRUN_IRQ_EN	1
#define ADC_FIFO_FLUSH		0
/* SUNXI_ADC_VOL_CTL1:0x34 */
#define ADC2_VOL		8
#define ADC1_VOL		0
/* SUNXI_ADC_FIFO_STA:0x38 */
#define	ADC_RXA			23
#define	ADC_RXA_CNT		8
#define	ADC_RXA_INT		3
#define	ADC_RXO_INT		1
/* SUNXI_ADC_DEBUG:0x4C */
#define	ADC_SWP		24
/* SUNXI_ADC_DIG_CTL:0x50 */
#define ADC1_2_VOL_EN		16
#define ADC_CHANNEL_EN		0
/* SUNXI_VAR1SPEEDUP_DOWN_CTL:0x54 */
#define VRA1SPEEDUP_DOWN_STATE		4
#define VRA1SPEEDUP_DOWN_CTL		1
#define VRA1SPEEDUP_DOWN_RST_CTL	0
/* SUNXI_AN_DEBUG1:0x58 */
#define DAC_MUTE_EN		18
/* SUNXI_AN_DEBUG2:0x5C */
#define DACR_DATA		23
#define DACR_DBG_DATA		16
#define DACR_DATA_CTL		15
#define DACL_DATA		8
#define DACL_DBG_DATA		1
#define DACL_DATA_CTL		0
/* SUNXI_DAC_DAP_CTL:0xF0 */
#define DDAP_EN			31
#define DDAP_DRC_EN		29
#define DDAP_HPF_EN		28
/* SUNXI_ADC_DAP_CTL:0xF8 */
#define ADAP_EN			31
#define ADAP_DRC_EN		29
#define ADAP_HPF_EN		28
/* SUNXI_ADC1_AN_CTL:0x300 */
#define ADC1_EN			31
#define MIC1_PGA_EN		30
#define ADC1_DITHER_CTL		29
#define DSM_DITHER_LVL		24
#define ADC1_OUTPUT_CURRENT	20
#define ADC1_PGA_CTL_RCM	18
#define ADC1_PGA_IN_VCM_CTL	16
#define IOPADC			14
#define ADC1_PGA_GAIN_CTL	8
#define ADC1_IOPAAF		6
#define ADC1_IOPSDM1		4
#define ADC1_IOPSDM2		2
#define ADC1_IOPMIC		0
/* SUNXI_ADC2_AN_CTL:0x304 */
#define ADC2_EN			31
#define MIC2_PGA_EN		30
#define ADC2_DITHER_CTL		29
#define DSM_DITHER_LVL		24
#define ADC2_OUTPUT_CURRENT	20
#define ADC2_PGA_CTL_RCM	18
#define ADC2_PGA_IN_VCM_CTL	16
#define ADC2_PGA_GAIN_CTL	8
#define ADC2_IOPAAF		6
#define ADC2_IOPSDM1		4
#define ADC2_IOPSDM2		2
#define ADC2_IOPMIC1		0
/* SUNXI_DAC_AN_CTL:0x310 */
#define CURRENT_TEST_SEL	31
#define HEADPHONE_GAIN		28
#define IOPHPDRV		26
#define CPLDO_VOLTAGE		24
#define OPDRV_CUR		22
#define IOPVRS			20
#define ILINEOUTAMPS		18
#define IOPDACS			16
#define DACL_EN			15
#define DACR_EN			14
#define LINEOUTL_EN		13
#define LMUTE			12
#define LINEOUTR_EN		11
#define RMUTE			10
#define CPLDO_BIAS		8
#define CPLDO_EN		7
#define LINEOUT_GAIN		0
/* SUNXI_DAC2_AN_CTL:0x314 */
#define CKDAC_DELAY_SET		16
#define DACLR_CHOPPER_EN	15
#define DACLR_CHOPPER_NOL_EN	14
#define DACLR_CHOPPER_CKSET	12
#define DACLR_CHOPPER_DELAY_SET			10
#define DACLR_CHOPPER_NOL_DELAY_SET		8
#define LINEOUTLR_CHOPPER_EN			7
#define LINEOUTLR_CHOPPER_NOL_ENABLE		6
#define LINEOUTLR_CHOPPER_CKSET			4
#define LINEOUTLR_CHOPPER_DELAY_SET		2
#define LINEOUTLR_CHOPPER_NOL_DELAY_SET		0
/* SUNXI_MICBIAS_AN_CTL:0x318 */
#define SEL_DET_ADC_FS		28
#define SEL_DET_ADC_DB		26
#define SEL_DET_ADC_BF		24
#define JACK_DET_EN		23
#define SEL_DET_ADC_DELAY	21
#define MIC_DET_ADC_EN		20
#define POPFREE			19
#define DET_MODE		18
#define AUTO_PULL_LOW_EN	17
#define MIC_DET_PULL		16
#define HMIC_BIAS_EN		15
#define HMIC_BIAS_SEL		13
#define HMIC_BIAS_CHOPPER_EN		12
#define HMIC_BIAS_CHOPPER_CLK_SEL	10
#define MMIC_BIAS_EN			7
#define MMIC_BIAS_VOL_SEL		5
#define MMIC_BIAS_CHOPPER_EN		4
#define MMIC_BIAS_CHOPPER_CLK_SEL	2
/* SUNXI_RAMP_CTL:0x31C */
#define RAMP_RISE_INT_EN	31
#define RAMP_RISE_INT		30
#define RAMP_FALL_INT_EN	29
#define RAMP_FALL_INT		28
#define RAMP_SOFT_RESET		24
#define RAMP_CLK_DIV_M		16
#define HP_PULL_OUT_EN		15
#define RAMP_HOLD_STEP		12
#define GAP_STEP		8
#define RAMP_STEP		4
#define RAMP_DOWN_EN		3
#define RAMP_UP_EN		2
#define RAMP_CTL_EN		1
#define RAMP_DIG_EN		0
/* SUNXI_BIAS_AN_CTL:0x320 */
#define BIASDATA		0
/* SUNXI_HP_AN_CTL:0x324 */
#define HPR_CALI_VERIFY		24
#define HPL_CALI_VERIFY		16
#define HPPA_EN			15
#define CP_EN			14
#define HP_EN			13
#define HP_INPUT_EN		11
#define HP_OUTPUT_EN		10
#define HPPA_DELAY		8
#define HP_CHOPPER_EN		7
#define CP_CLK_SEL		6
#define HP_CALI_MODE_SEL	5
#define HP_CALI_VERIFY_EN	4
#define HP_CALI_FIRST		3
#define HP_CALI_CLK_SEL		0
/* SUNXI_HMIC_CTL:0x328 */
#define HMIC_FS_SEL		21
#define MDATA_THRESHOLD		16
#define HMIC_SMOOTH_FILTER_SET	14
#define HMIC_M			10
#define HMIC_N			6
#define MDATA_THRESHOLD_DB	3
#define JACK_OUT_IRQ_EN		2
#define JACK_IN_IRQ_EN		1
#define MIC_DET_IRQ_EN		0
/* SUNXI_HMIC_STA:0x32C */
#define MDATA_DISCARD		13
#define HMIC_DATA		8
#define JACK_OUT_IRQ_STA	4
#define JACK_IN_IRQ_STA		3
#define JACK_IO_STA		1
#define MIC_DET_IRQ_STA		0
/* SUNXI_POWER_AN_CTL:0x348 */
#define ALDO_EN				31
#define VAR1SPEEDUP_DOWN_FURTHER_CTL	29
#define VRP_LDO_CHOPPER_EN		27
#define VRP_LDO_CHOPPER_CLK_SEL		25
#define VRP_LDO_EN		24
#define AVCCPOR_MONITOR		16
#define BG_BUFFER_DISABLE	15
#define ALDO_OUTPUT_VOL		12
#define BG_ROUGH_VOL_TRIM	8
#define BG_FINE_TRIM		0

#define DACDRC_SHIFT		1
#define DACHPF_SHIFT		2
#define ADCDRC_SHIFT		3
#define ADCHPF_SHIFT		4

struct sunxi_codec_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_codec_clk {
	/* parent */
	struct clk *clk_pll_audio0;
	struct clk *clk_pll_audio1_5x;
	/* module */
	struct clk *clk_adda_dac;
	struct clk *clk_adda_adc;
	/* bus & reset */
	struct clk *clk_bus;
	struct reset_control *clk_rst;
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
	u32 dacr_vol;
	u32 adc1_vol;
	u32 adc2_vol;
	u32 lineout_gain;
	u32 hpout_gain;
	u32 adc1_gain;
	u32 adc2_gain;
};

struct sunxi_audio_status {
	struct mutex apf_mutex; /* audio playback function mutex lock */
	bool spk;

	struct mutex acf_mutex; /* audio capture function mutex lock */
	bool mic1;
	bool mic2;
};

enum JACKDETECTWAY {
	JACK_DETECT_LOW = 0,
	JACK_DETECT_HIGH,
};

enum SUNXI_JACK_IRQ_TIME {
	JACK_IRQ_NORMAL	= 0,
	JACK_IRQ_SCAN,
};

enum SUNXI_JACK_IRQ_STA {
	JACK_IRQ_NULL	= -1,
	JACK_IRQ_OUT	= 0,
	JACK_IRQ_IN,
	JACK_IRQ_MIC,
	JACK_IRQ_SDBP,
};

/* jack -> codec */
struct sunxi_jack_codec_priv {
	struct regmap *regmap;

	bool det_level;
	unsigned int det_threshold;
	unsigned int det_debounce;

	/* key_det_vol[0][0] - key_hook_vol_min,  key_det_vol[0][1] - key_hook_vol_max
	 * key_det_vol[1][0] - key_up_vol_min,    key_det_vol[1][1] - key_up_vol_max
	 * key_det_vol[2][0] - key_down_vol_min,  key_det_vol[2][1] - key_down_vol_max
	 * key_det_vol[3][0] - key_voice_vol_min, key_det_vol[3][1] - key_voice_vol_max
	 */
	unsigned int key_det_vol[4][2];

	enum SUNXI_JACK_IRQ_STA irq_sta;
	enum SUNXI_JACK_IRQ_TIME irq_time;

	enum snd_jack_types jack_type;

	unsigned int hp_det_basedata;

	struct timespec64 tv_mic;
	struct timespec64 tv_headset_plugin;
};

/* jack -> extcon */
struct sunxi_jack_extcon_priv {
	struct regmap *regmap;

	bool det_level;
	unsigned int det_threshold;
	unsigned int det_debounce;

	/* key_det_vol[0][0] - key_hook_vol_min,  key_det_vol[0][1] - key_hook_vol_max
	 * key_det_vol[1][0] - key_up_vol_min,    key_det_vol[1][1] - key_up_vol_max
	 * key_det_vol[2][0] - key_down_vol_min,  key_det_vol[2][1] - key_down_vol_max
	 * key_det_vol[3][0] - key_voice_vol_min, key_det_vol[3][1] - key_voice_vol_max
	 */
	unsigned int key_det_vol[4][2];

	enum SUNXI_JACK_IRQ_STA irq_sta;
	struct timespec64 tv_headset_plugin;
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

	struct sunxi_jack_codec_priv jack_codec_priv;
	struct sunxi_jack_extcon_priv jack_extcon_priv;

	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;

	/* debug */
	char module_name[32];
	struct snd_sunxi_dump dump;
	bool show_reg_all;
};

#endif /* __SND_SUN65IW1_CODEC_H */

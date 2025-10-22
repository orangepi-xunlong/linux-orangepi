/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2024, Dby <dby@allwinnertech.com>
 * 		       lijingpsw <lijingpsw@allwinnertech.com>
 * 		       wuhao <wuhao@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _SUN8IW20_CODEC_H
#define _SUN8IW20_CODEC_H

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
#define SUNXI_ADC_DEBUG			0x4C
#define SUNXI_ADC_DIG_CTRL		0x50
#define SUNXI_VRA1SPEEDUP_DOWN_CTRL	0x54

#define SUNXI_DAC_DAP_CTL		0xF0
#define SUNXI_ADC_DAP_CTL		0xF8
#define SUNXI_DAC_DRC_CTRL		0x108
#define SUNXI_ADC_DRC_CTRL		0x208

#define SUNXI_AC_VERSION		0x2C0

/* Analog register */
#define SUNXI_ADC1_REG			0x300
#define SUNXI_ADC2_REG			0x304
#define SUNXI_ADC3_REG			0x308
#define SUNXI_DAC_REG			0x310
#define SUNXI_MICBIAS_AN_CTL		0x318
#define SUNXI_RAMP_REG			0x31C
#define SUNXI_BIAS_REG			0x320
#define SUNXI_HMIC_CTRL			0x328
#define SUNXI_HMIC_STS			0x32C
#define SUNXI_HP2_REG			0x340
#define SUNXI_POWER_REG			0x348
#define SUNXI_ADC_CUR_REG		0x34C
#define SUNXI_AUDIO_MAX_REG		SUNXI_ADC_CUR_REG

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
#define TX_EMPTY			23
#define DAC_TXE_CNT			8
#define DAC_TXE_INT			3
#define DAC_TXU_INT			2
#define DAC_TXO_INT			1

/* SUNXI_DAC_DG:0x28 */
#define DAC_MODU_SEL			11
#define DAC_PATTERN_SEL			9
#define DAC_CODEC_CLK_SEL		8
#define DAC_SWP				6
#define ADDA_LOOP_MODE			0

/* SUNXI_ADC_FIFOC:0x30 */
#define ADC_FS				29
#define EN_AD				28
#define ADCFDT				26
#define ADCDFEN				25
#define RX_FIFO_MODE			24
#define RX_SYNC_EN_STA			21
#define RX_SYNC_EN			20
#define RX_SAMPLE_BITS			16
#define RX_FIFO_TRG_LEVEL		4
#define ADC_DRQ_EN			3
#define ADC_IRQ_EN			2
#define ADC_OVERRUN_IRQ_EN		1
#define ADC_FIFO_FLUSH			0

/* SUNXI_ADC_VOL_CTRL:0x34 */
#define ADC3_VOL			16
#define ADC2_VOL			8
#define ADC1_VOL			0

/* SUNXI_ADC_FIFOS:0x38 */
#define RXA				23
#define ADC_RXA_CNT			8
#define ADC_RXA_INT			3
#define ADC_RXO_INT			1

/* SUNXI_ADC_DEBUG:0x4C */
#define ADC_SWP1			25
#define ADC_SWP2			24

/* SUNXI_ADC_DIG_CTRLRL:0x50 */
#define ADC3_VOL_EN			17
#define ADC1_2_VOL_EN			16
#define ADC3_CHANNEL_EN			2
#define ADC2_CHANNEL_EN			1
#define ADC1_CHANNEL_EN			0
#define ADC_CHANNEL_EN			0

/* SUNXI_VRA1SPEEDUP_DOWN_CTRL:0x54 */
#define VRA1SPEEDUP_DOWN_STATE		4
#define VRA1SPEEDUP_DOWN_CTRL		1
#define VRA1SPEEDUP_DOWN_RST_CTRL	0

/* SUNXI_DAC_DAP_CTL:0xF0 */
#define DDAP_EN				31
#define DDAP_DRC_EN			29
#define DDAP_HPF_EN			28
/* SUNXI_ADC_DAP_CTL:0xF8 */
#define ADAP0_EN			31
#define ADAP0_DRC_EN			29
#define ADAP0_HPF_EN			28
#define ADAP1_EN			27
#define ADAP1_DRC_EN			25
#define ADAP1_HPF_EN			24

/* SUNXI_DAC_DRC_HHPFC : 0x100*/
#define DAC_HHPF_CONF			0

/* SUNXI_DAC_DRC_LHPFC : 0x104*/
#define DAC_LHPF_CONF			0

/* SUNXI_DAC_DRC_CTRL : 0x108*/
#define DAC_DRC_DELAY_OUT_STATE		15
#define DAC_DRC_SIGNAL_DELAY		8
#define DAC_DRC_DELAY_BUF_EN		7
#define DAC_DRC_GAIN_MAX_EN		6
#define DAC_DRC_GAIN_MIN_EN		5
#define DAC_DRC_NOISE_DET_EN		4
#define DAC_DRC_SIGNAL_SEL		3
#define DAC_DRC_DELAY_EN		2
#define DAC_DRC_LT_EN			1
#define DAC_DRC_ET_EN			0

/* SUNXI_ADC_DRC_HHPFC : 0x200*/
#define ADC_HHPF_CONF			0

/* SUNXI_ADC_DRC_LHPFC : 0x204*/
#define ADC_LHPF_CONF			0

/* SUNXI_ADC_DRC_CTRL : 0x208*/
#define ADC_DRC_DELAY_OUT_STATE		15
#define ADC_DRC_SIGNAL_DELAY		8
#define ADC_DRC_DELAY_BUF_EN		7
#define ADC_DRC_GAIN_MAX_EN		6
#define ADC_DRC_GAIN_MIN_EN		5
#define ADC_DRC_NOISE_DET_EN		4
#define ADC_DRC_SIGNAL_SEL		3
#define ADC_DRC_DELAY_EN		2
#define ADC_DRC_LT_EN			1
#define ADC_DRC_ET_EN			0

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
#define IOPADC				14
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
#define ADC2_PGA_GAIN_CTRL		8
#define ADC2_IOPAAF			6
#define ADC2_IOPSDM1			4
#define ADC2_IOPSDM2			2
#define ADC2_IOPMIC			0

/* SUNXI_ADC3_REG : 0x308 */
#define ADC3_EN				31
#define MIC3_PGA_EN			30
#define ADC3_DITHER_CTRL		29
#define MIC3_SIN_EN			28
#define ADC3_DSM_DITHER_LVL		24
#define ADC3_IOPBUFFER			20
#define ADC3_PGA_CTRL_RCM		18
#define ADC3_PGA_IN_VCM_CTRL		16
#define ADC3_PGA_GAIN_CTRL		8
#define ADC3_IOPAAF			6
#define ADC3_IOPSDM1			4
#define ADC3_IOPSDM2			2
#define ADC3_IOPMIC			0

/* SUNXI_DAC_REG : 0x310 */
#define CURRENT_TEST_SELECT		23
#define	VRA2_IOPVRS			20
#define	ILINEOUTAMPS			18
#define IOPDACS				16
#define DACLEN				15
#define DACREN				14
#define LINEOUTLEN			13
#define DACLMUTE			12
#define LINEOUTREN			11
#define DACRMUTE			10
#define LINEOUTLDIFFEN			6
#define LINEOUTRDIFFEN			5
#define LINEOUT_VOL			0

/* SUNXI_MICBIAS_AN_CTL : 0x318 */
#define SELDETADCFS			28
#define SELDETADCDB			26
#define SELDETADCBF			24
#define JACK_DET_EN			23
#define SELDETADCDY			21
#define MIC_DET_ADC_EN			20
#define POPFREE				19
#define DET_MODE			18
#define AUTO_PULL_LOW_EN		17
#define MICDETPL			16
#define HMIC_BIAS_EN			15
#define HBIASSEL			13
#define	HMICBIAS_CHOP_EN		12
#define HMICBIAS_CHOP_CLK_SEL		10
#define MMICBIASEN			7
#define	MBIASSEL			5
#define	MMICBIAS_CHOP_EN		4
#define MMICBIAS_CHOP_CLK_SEL		2

/* SUNXI_RAMP_REG : 0x31C */
#define RAMP_RISE_INT_EN		31
#define RAMP_RISE_INT			30
#define RAMP_FALL_INT_EN		29
#define RAMP_FALL_INT			28
#define RAMP_SRST			24
#define RAMP_CLK_DIV_M			16
#define HP_PULL_OUT_EN			15
#define RAMP_HOLD_STEP			12
#define GAP_STEP			8
#define RAMP_STEP			4
#define RMD_EN				3
#define RMU_EN				2
#define RMC_EN				1
#define RD_EN				0

/* SUNXI_BIAS_REG : 0x320 */
#define AC_BIASDATA			0

/* SUNXI_HMIC_CTRL : 0x328 */
#define HMIC_SAMPLE_SEL			21
#define MDATA_THRESHOLD			16
#define HMIC_SF				14
#define HMIC_M				10
#define HMIC_N				6
#define MDATA_THRESHOLD_DB		3
#define JACK_OUT_IRQ_EN			2
#define JACK_IN_IRQ_EN			1
#define MIC_DET_IRQ_EN			0

/* SUNXI_HMIC_STS : 0x32C */
#define MDATA_DISCARD			13
#define	HMIC_DATA			8
#define JACK_OUT_IRQ_STA		4
#define JACK_IN_IRQ_STA 		3
#define MIC_DET_IRQ_STA			0

/* SUNXI_HP2_REG	:0x340 */
#define HPFB_BUF_EN			31
#define HEADPHONE_GAIN			28
#define HPFB_RES			26
#define OPDRV_CUR			24
#define IOPHP				22
#define HP_DRVEN			21
#define HP_DRVOUTEN			20
#define RSWITCH				19
#define RAMPEN				18
#define HPFB_IN_EN			17
#define RAMP_FINAL_CTRL			16
#define RAMP_OUT_EN			15
#define RAMP_FINAL_STATE_RES		13
#define FB_BUF_OUTPUT_CURRENT		8

/* SUNXI_POWER_REG      :0x348 */
#define ALDO_EN				31
#define HPLDO_EN			30
#define VAR1SPEEDUP_DOWN_FURTHER_CTRL	29
#define AVCCPOR				16
#define ALDO_OUTPUT_VOLTAGE		12
#define HPLDO_OUTPUT_VOLTAGE		8
#define BG_TRIM				0

/* SUNXI_ADC_CUR_REG    :0x34C */
#define ADC3_IOPMIC2			20
#define ADC3_OP_MIC1_CUR		18
#define ADC3_OP_MIC2_CUR		16
#define ADC2_IOPMIC2			12
#define ADC2_OP_MIC1_CUR		10
#define ADC2_OP_MIC2_CUR		8
#define ADC1_IOPMIC2			4
#define ADC1_OP_MIC1_CUR		2
#define ADC1_OP_MIC2_CUR		0

/* gain select */
#define MIC1_GAIN_SHIFT			1
#define MIC2_GAIN_SHIFT			2
#define MIC3_GAIN_SHIFT			3

/* for DRC/HPF Switch */
#define DACDRC_SHIFT			1
#define DACHPF_SHIFT			2
#define ADCDRC0_SHIFT			3
#define ADCHPF0_SHIFT			4
#define ADCDRC1_SHIFT			5
#define ADCHPF1_SHIFT			6

/* for input mux Switch */
#define INPUT1_SHIFT		0
#define INPUT2_SHIFT		1
#define INPUT3_SHIFT		2

/* for mic single/diff switch */
#define MIC1SIN_SHIFT		0
#define MIC2SIN_SHIFT		1
#define MIC3SIN_SHIFT		2

struct sunxi_codec_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_codec_clk {
	/* parent */
	struct clk *clk_pll_audio0;
	struct clk *clk_pll_audio1_div5;
	/* module */
	struct clk *clk_audio_dac;
	struct clk *clk_audio_adc;
	/* bus & reset */
	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

struct sunxi_audio_status {
	struct mutex apf_mutex; /* audio playback function mutex lock */
	bool spk;

	struct mutex acf_mutex; /* audio capture function mutex lock */
	bool mic1;
	bool mic2;
	bool mic3;
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
	u32 lineout_vol;
	u32 adc1_vol;
	u32 adc2_vol;
	u32 adc3_vol;
	u32 adc1_gain;
	u32 adc2_gain;
	u32 adc3_gain;
	u32 hpout_gain;
	u32 fminl_gain;
	u32 fminr_gain;
	u32 lineinl_gain;
	u32 lineinr_gain;
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

struct sunxi_codec {
	const char *module_version;
	struct platform_device *pdev;

	struct sunxi_codec_mem mem;
	struct sunxi_codec_clk clk;
	struct snd_sunxi_rglt *rglt;
	struct sunxi_codec_dts dts;
	struct sunxi_audio_status audio_sta;
	enum SND_SUNXI_CLK_STATUS clk_sta;

	struct sunxi_jack_codec_priv jack_codec_priv;

	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;

	/* debug */
	char module_name[32];
	struct snd_sunxi_dump dump;
	bool show_reg_all;
};

#endif /* __SND_SUN8IW20_CODEC_H */

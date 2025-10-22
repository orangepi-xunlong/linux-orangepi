/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUN50IW10_CODEC_H
#define __SND_SUN50IW10_CODEC_H

#define SUNXI_DAC_DPC		0x00
#define SUNXI_DAC_VOL_CTRL	0x04
#define SUNXI_DAC_FIFOC		0x10
#define SUNXI_DAC_FIFOS		0x14

#define SUNXI_DAC_TXDATA	0X20
#define SUNXI_DAC_CNT		0x24
#define SUNXI_DAC_DG		0x28

#define	SUNXI_ADC_FIFOC		0x30
#define	SUNXI_ADC_VOL_CTRL	0x34
#define SUNXI_ADC_FIFOS		0x38
#define SUNXI_ADC_RXDATA	0x40
#define SUNXI_ADC_CNT		0x44
#define SUNXI_ADC_DG		0x4C

#define SUNXI_DAC_DAP_CTL	0xF0
#define SUNXI_ADC_DAP_CTL	0xF8

#define SUNXI_AC_VERSION	0x2C0

#define SUNXI_ADCL_REG		0x300
#define SUNXI_ADCR_REG		0x304
#define SUNXI_DAC_REG		0x310
#define SUNXI_MICBIAS_REG	0x318
#define SUNXI_BIAS_REG		0x320
#define SUNXI_HEADPHONE_REG	0x324
#define SUNXI_HMIC_CTRL		0x328
#define SUNXI_HMIC_STS		0x32c

#define SUNXI_AUDIO_MAX_REG	SUNXI_HMIC_STS

/* SUNXI_DAC_DPC:0x00 */
#define EN_DAC			31
#define MODQU			25
#define DWA_EN			24
#define HPF_EN			18
#define DVOL			12
#define DAC_HUB_EN		0

/* SUNXI_DAC_VOL_CTRL:0x04 */
#define DAC_VOL_SEL		16
#define DAC_VOL_L		8
#define DAC_VOL_R		0

/* SUNXI_DAC_FIFOC:0x10 */
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
#define DAC_FIFO_FLUSH		0

/* SUNXI_DAC_FIFOS:0x14 */
#define	TX_EMPTY		23
#define	DAC_TXE_CNT		8
#define	DAC_TXE_INT		3
#define	DAC_TXU_INT		2
#define	DAC_TXO_INT		1

/* SUNXI_DAC_DG:0x28 */
#define	DAC_MODU_SEL		11
#define	DAC_PATTERN_SEL		9
#define	DAC_CODEC_CLK_SEL	8
#define	DA_SWP			6
#define	ADDA_LOOP_MODE		0

/* SUNXI_ADC_FIFOC:0x30 */
#define ADC_FS			29
#define EN_AD			28
#define ADCFDT			26
#define ADCDFEN			25
#define RX_FIFO_MODE		24
#define RX_SYNC_EN_START	21
#define RX_SYNC_EN		20
#define ADC_VOL_SEL		17
#define RX_SAMPLE_BITS		16
#define ADCR_CHAN_EN		13
#define ADCL_CHAN_EN		12
#define RX_FIFO_TRG_LEVEL	4
#define ADC_DRQ_EN		3
#define ADC_IRQ_EN		2
#define ADC_OVERRUN_IRQ_EN	1
#define ADC_FIFO_FLUSH		0

/* SUNXI_ADC_VOL_CTRL:0x34 */
#define ADC_VOL_L		8
#define ADC_VOL_R		0

/* SUNXI_ADC_FIFOS:0x38 */
#define	RXA			23
#define	ADC_RXA_CNT		8
#define	ADC_RXA_INT		3
#define	ADC_RXO_INT		1

/* SUNXI_ADC_DG:0x4C */
#define	AD_SWP			24

/* SUNXI_DAC_DAP_CTL:0xf0 */
#define	DAC_DAP_EN		31
#define	DDAP_DRC_EN		29
#define	DDAP_HPF_EN		28

/* SUNXI_ADC_DAP_CTL:0xf8 */
#define	ADC_DAP_EN		31
#define	ADAP_DRC_EN		29
#define	ADAP_HPF_EN		28

/* SUNXI_ADCL_REG : 0x300 */
#define ADCL_EN			31
#define MIC1AMPEN		30
#define ADCL_DITHER		29
#define ADCL_PGA_CTRL_RCM	18
#define ADCL_PGA_IN_VCM_CTRL	16
#define ADCL_PGA_GAIN_CTRL	8
#define ADCL_IOPAAFL		6
#define ADCL_IOPSDML1		4
#define ADCL_IOPSDML2		2
#define ADCL_IOPMICL		0

/* SUNXI_ADCR_REG : 0x304 */
#define ADCR_EN			31
#define MIC2AMPEN		30
#define ADCR_DITHER		29
#define ADCR_PGA_CTRL_RCM	18
#define ADCR_PGA_IN_VCM_CTRL	16
#define ADCR_PGA_GAIN_CTRL	8
#define ADCR_IOPAAFL		6
#define ADCR_IOPSDML1		4
#define ADCR_IOPSDML2		2
#define ADCR_IOPMICL		0

/* SUNXI_DAC_REG : 0x310 */
#define CURRENT_TEST_SELECT	31
#define HEADPHONE_GAIN		28
#define CPLDO_EN		27
#define CPLDO_VOLTAGE		24
#define OPDRV_CUR		22
#define	VRA2_IOPVRS		20
#define	ILINEOUTAMPS		18
#define IOPDACS			16
#define DACLEN			15
#define DACREN			14
#define LINEOUTLEN		13
#define DACLMUTE		12
#define LINEOUTLDIFFEN		6
#define LINEOUT_VOL		0

/* SUNXI_MICBIAS_REG : 0x318 */
#define SELDETADCFS		28
#define SELDETADCDB		26
#define SELDETADCBF		24
#define JACKDETEN		23
#define SELDETADCDY		21
#define MICADCEN		20
#define POPFREE			19
#define DETMODE			18
#define AUTOPLEN		17
#define MICDETPL		16
#define HMICBIASEN		15
#define HBIASSEL		13
#define	HMICBIAS_CHOP_EN	12
#define HMICBIAS_CHOP_CLK_SEL	10
#define MMICBIASEN		7
#define	MBIASSEL		5
#define	MMICBIAS_CHOP_EN	4
#define MMICBIAS_CHOP_CLK_SEL	2

/* SUNXI_BIAS_REG : 0x320 */
#define AC_BIASDATA		0

/* SUNXI_HEADPHONE_REG : 0x324 */
#define HPRCALIVERIFY		24
#define HPLCALIVERIFY		16
#define HPPA_EN			15
#define HPINPUTEN		11
#define HPOUTPUTEN		10
#define HPPA_DEL		8
#define CP_CLKS			6
#define HPCALIMODE		5
#define HPCALIVERIFY		4
#define HPCALIFIRST		3
#define HPCALICKS		0

/* SUNXI_HMIC_CTRL : 0x328 */
#define HMIC_SAMPLE_SEL		21
#define MDATA_THRESHOLD		16
#define HMIC_SF			14
#define HMIC_M			10
#define HMIC_N			6
#define MDATA_THRESHOLD_DB	3
#define JACK_OUT_IRQ_EN		2
#define JACK_IN_IRQ_EN		1
#define MIC_DET_IRQ_EN		0

/* SUNXI_HMIC_STS : 0x32c */
#define MDATA_DISCARD		13
#define	HMIC_DATA		8
#define JACK_DET_OUT_ST		4
#define JACK_DET_OIRQ		4
#define JACK_DET_IIN_ST 	3
#define JACK_DET_IIRQ		3
#define MIC_DET_ST		0

/* for DRC/HPF Switch */
#define DACDRC_SHIFT		0
#define DACHPF_SHIFT		1
#define ADCDRC_SHIFT		2
#define ADCHPF_SHIFT		3

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
	struct clk *clk_pll_com;
	struct clk *clk_pll_com_audio;

	struct clk *clk_audio_dac;
	struct clk *clk_audio_adc;
};

struct sunxi_codec_rglt {
	u32 avcc_vol;
	u32 dvcc_vol;
	bool avcc_external;
	bool dvcc_external;
	struct regulator *avcc;
	struct regulator *dvcc;
};

struct sunxi_codec_dts {
	u32 adc_dig_vol_l;
	u32 adc_dig_vol_r;
	u32 mic1_vol;
	u32 mic2_vol;

	u32 dac_dig_vol;
	u32 dac_dig_vol_l;
	u32 dac_dig_vol_r;
	u32 lineout_vol;
	u32 hp_vol;

	u32 adc_dtime;

	bool lineout_single;	/* true: single mode; false: differ mode */

	bool tx_hub_en;		/* tx_hub */
	bool rx_sync_en;	/* rx_sync read from dts */
	bool rx_sync_ctl;
	int rx_sync_id;
	rx_sync_domain_t rx_sync_domain;
};

struct sunxi_audio_status {
	struct mutex apf_mutex;
	bool lineout;
	bool hpout;
	bool spk;

	struct mutex acf_mutex;
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
};

/* jack -> codec */
enum JACK_INSERT_STA {
	JACK_INSERT_OUT = 0,
	JACK_INSERT_IN,
	JACK_INSERT_ING,
	JACK_INSERT_EXIT,
};

struct sunxi_jack_codec_priv {
	struct regmap *regmap;

	bool det_level;
	unsigned int det_threshold;
	unsigned int det_debouce_time;

	/* key_det_vol[0][0] - key_hook_vol_min,  key_det_vol[0][1] - key_hook_vol_max
	 * key_det_vol[1][0] - key_up_vol_min,    key_det_vol[1][1] - key_up_vol_max
	 * key_det_vol[2][0] - key_down_vol_min,  key_det_vol[2][1] - key_down_vol_max
	 * key_det_vol[3][0] - key_voice_vol_min, key_det_vol[3][1] - key_voice_vol_max
	 */
	unsigned int key_det_vol[4][2];

	enum SUNXI_JACK_IRQ_STA irq_sta;
	enum SUNXI_JACK_IRQ_TIME irq_time;

	enum snd_jack_types jack_type;

	struct delayed_work plug_process_check_work;
	struct delayed_work plug_process_check_again_work;

	unsigned int hp_det_basedata;
	enum JACK_INSERT_STA insert_sta;
};

/* jack -> extcon */
struct sunxi_jack_extcon_priv {
	struct regmap *regmap;

	bool det_level;
	unsigned int det_threshold;
	unsigned int det_debouce_time;

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
	struct sunxi_codec_rglt rglt;
	struct sunxi_codec_dts dts;

	struct sunxi_jack_codec_priv jack_codec_priv;
	struct sunxi_jack_extcon_priv jack_extcon_priv;

	struct sunxi_audio_status audio_sta;
	enum SND_SUNXI_CLK_STATUS clk_sta;

	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;

	/* debug */
	char module_name[32];
	struct snd_sunxi_dump dump;
	bool show_reg_all;
};

#endif /* __SND_SUN50IW10_CODEC_H */

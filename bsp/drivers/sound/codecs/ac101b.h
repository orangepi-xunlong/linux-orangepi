/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * ac101b.h -- AC101B ALSA SoC Audio driver
 *
 * Copyright (c) 2022 Allwinnertech Ltd.
 */

#ifndef __AC101B_H
#define __AC101B_H

/*** AC101B Codec Register Define ***/
/* reset */
#define CHIP_SOFT_RST		0x00

/* POWER */
#define POWER_REG1		0x01
#define POWER_REG2		0x02
#define POWER_REG3		0x03
#define POWER_REG4		0x04
#define POWER_REG5		0x05
#define POWER_REG6		0x06
#define MBIAS_REG		0x07
#define HBIAS_REG		0x08

/* DAC Analog */
#define DAC_REG1		0x09
#define DAC_REG2		0x0a
#define DAC_REG3		0x0b
#define DAC_REG4		0x0c

/* PLLCLK */
#define PLL_CTRL1		0x10
#define PLL_CTRL2		0x11
#define PLL_CTRL3		0x12
#define PLL_CTRL4		0x13
#define PLL_CTRL5		0x14
#define PLL_CTRL6		0x16
#define PLL_CTRL7		0x17
#define PLL_CTRL8		0x18

/* HP Analog */
#define HP_REG1			0x19
#define HP_REG2			0x1a
#define HP_REG3			0x1b
#define HP_REG4			0x1c
#define HP_REG5			0x1d

/* SYSCLK */
#define SYSCLK_CTRL		0x20

/* ADC Analog */
#define ADC1_REG1		0x24
#define ADC1_REG2		0x25
#define ADC1_REG3		0x26
#define ADC1_REG4		0x27
#define ADC2_REG1		0x28
#define ADC2_REG2		0x29
#define ADC2_REG3		0x2a
#define ADC2_REG4		0x2b
#define ADC3_REG1		0x2c
#define ADC3_REG2		0x2d
#define ADC3_REG3		0x2e
#define ADC3_REG4		0x2f

/* I2S */
#define I2S_CTRL		0x30
#define I2S_BCLK_CTRL		0x31
#define I2S_LRCK_CTRL1		0x32
#define I2S_LRCK_CTRL2		0x33
#define I2S_FMT_CTRL1		0x34
#define I2S_FMT_CTRL2		0x35
#define I2S_FMT_CTRL3		0x36
#define I2S_TX_CTRL1		0x38
#define I2S_TX_CTRL2		0x39
#define I2S_TX_CTRL3		0x3a
#define I2S_TX_MIX_CTRL		0x3b
#define I2S_TX_CHMP_CTRL1	0x3c
#define I2S_TX_CHMP_CTRL2	0x3d
#define I2S_TX_CHMP_CTRL3	0x3e
#define I2S_TX_CHMP_CTRL4	0x3f
#define I2S_RX_CTRL1		0x40
#define I2S_RX_CTRL2		0x41
#define I2S_RX_CTRL3		0x42
#define I2S_RX_MIX_CTRL		0x43
#define I2S_RX_CHMP_CTRL	0x44

/* ADDA sample rate */
#define ADDA_FS_CTRL		0x60

/* ADC Digital */
#define ADC_DIG_EN		0x61
#define DMIC_EN_CTRL		0x62
#define ADC_DDT_CTRL		0x63
#define HPF_EN			0x66
#define HPF1_COEF_REGH		0x67
#define HPF1_COEF_REGL		0x68
#define HPF2_COEF_REGH		0x69
#define HPF2_COEF_REGL		0x6a
#define HPF3_COEF_REGH		0x6b
#define HPF3_COEF_REGL		0x6c
#define ADC1_MUX_CTRL		0x6d
#define ADC2_MUX_CTRL		0x6e
#define ADC3_MUX_CTRL		0x6f
#define ADC1_DVOL_CTRL		0x70
#define ADC2_DVOL_CTRL		0x71
#define ADC3_DVOL_CTRL		0x72
#define HMIC_DET_CTRL		0x73
#define HMIC_DET_DBC		0x74
#define HMIC_DET_TH1		0x75
#define HMIC_DET_TH2		0x76
#define HMIC_DET_DATA		0x77
#define HP_DET_CTRL		0x78
#define HP_DET_DBC		0x79
#define HP_DET_IRQ		0x7a
#define HP_DET_STA		0x7b
#define ADC_DIG_DEBUG		0x7f

/* DAC Digital */
#define DAC_DIG_EN		0x81
#define DAC_DIG_CTRL		0x82
#define DAC_DHP_GAIN_CTRL	0x83
#define DAC_LOUT_CTRL		0x84
#define DAC_DVC_L		0x85
#define DAC_DVC_R		0x86
#define EQ_CTRL			0x87
#define EQ_COEF_CTRL		0x88
#define DAC_MUX_CTRL		0x89
#define HP_AVR_CTRL		0x8a
#define HP_AVR_THH		0x8b
#define HP_AVR_THM		0x8c
#define HP_AVR_THL		0x8d
#define HP_AVR_DBC		0x8e
#define HP_LOUT_CTRL		0x8f

#define AC101B_MAX_REG		0xff


/* POWER_REG1 0x01 */
#define VRA1_SEPPEDUP		7
#define BG_BUFEN		6
#define BG_VOLT_SEL		4
#define BG_VOLT_TRIM		0

/* POWER_REG2 0x02 */
#define CUR_SINK_TEST		7
#define BG_CUR_TRIM		4
#define OSC_EN			3
#define OSC_CLK_TRIM		0

/* POWER_REG3 0x03 */
#define VRP_BYPASS		7
#define DLDO_LQ			6
#define VRP_NOCAP_EN		5
#define DLDO_VSEL		0

/* POWER_REG4 0x04 */
#define ALDO_EN			7
#define ALDO_VSEL		4
#define ALDO_BYPASS		3
#define AVCC_POR		1
#define ADDA_BIASEN		0

/* POWER_REG5 0x05 */
#define HPLDO_EN		7
#define HPLDO_VOLT_SEL		4
#define VRP_EN			3
#define VRP_VOLT_SEL		0

/* POWER_REG6 0x06 */
#define IOPDRVS			6
#define IOPVRS			4
#define IOPDACS			2
#define ISPKS			0

/* MBIAS_REG 0x07 */
#define MBIAS_EN		7
#define MBIAS_VOLT_SEL		5
#define MBIAS_CHOPPER_EN	4
#define MBIAS_CHOPPER_CLK_SEL	2

/* HBIAS_REG 0x08 */
#define HBIAS_VOLT_SEL		4
#define HBIAS_MODE		2
#define HBIAS_EN		1
#define HBIASADC_EN		0

/* DAC_REG1 0x09 */
#define DACL_EN			7
#define DACR_EN			6
#define CKADC_DELAY_SET		4

/* DAC_REG2 0x0a */
#define DAC_CHOPPER_EN		7
#define DAC_CHOPPER_NOL_EN	6
#define DAC_CHOPPER_CKSET	4
#define DAC_CHOPPER_DELAY	2
#define DAC_CHOPPER_NOL_DELAY	0

/* DAC_REG3 0x0b */
#define SPKL_EN			7
#define SPKR_EN			6
#define SPK_SELPAG_EN		5
#define SPK_CH_OUT_EDGB		0

/* DAC_REG4 0x0c */
#define SPK_CHOPPER_EN			7
#define SPK_CHOPPER_NOL_EN		6
#define SPK_CHOPPER_CKSET		4
#define SPK_CHOPPER_DELAY_SET		2
#define SPK_CHOPPER_NOL_DELAY_SET	0

/* PLL_CTRL1 0x10 */
#define PLL_IBIAS		4
#define PLL_NDET		3
#define PLL_LOCK_STS		2
#define PLL_COM_EN		1
#define PLL_EN			0

/* PLL_CTRL2 0x11 */
#define PLL_PREDIV2		5
#define PLL_PREDIV1		0

/* PLL_CTRL3 0x12 */
#define PLL_LOOPDIV_MSB		0

/* PLL_CTRL4 0x13 */
#define PLL_LOOPDIV_LSB		0

/* PLL_CTRL5 0x14 */
#define PLL_POSTDIV2		5
#define PLL_POSTDIV1		0

/* PLL_CTRL6 0x16 */
#define PLL_LDO			6
#define PLL_CP			0

/* PLL_CTRL7 0x17 */
#define PLL_CAP			6
#define PLL_RES			4

/* PLL_CTRL8 0x18 */
#define HOLD_TIME		4
#define LOCK_LEVEL1		2
#define LOCK_LEVEL2		1
#define PLL_LOCK_EN		0

/* HP_REG1 0x19 */
#define HPPA_EN			7
#define CP_EN			6
#define HPOUT_EN		5
#define CP_CLKS			0

/* HP_REG2 0x1a */
#define HP_CHOPPER_EN			7
#define HP_CHOPPER_NOL_EN		6
#define HP_CHOPPER_CKSET		4
#define HP_CHOPPER_DELAY_SET		2
#define HP_CHOPPER_NOL_DELAY_SET	0

/* HP_REG3 0x1b */
/* HP_REG4 0x1c */
/* HP_REG5 0x1d */
#define OPDRV_CUR		4
#define HP_GAIN			0

/* SYSCLK_CTRL 0x20 */
#define PLLCLK_EN		7
#define PLLCLK_SRC		4
#define SYSCLK_SRC		2
#define MCLK_DIV		1
#define SYSCLK_EN		0

/* ADC1_REG1 0x24 */
#define ADC1_EN			7
#define ADC1_DSM_DIS		6
#define ADC1_DSM_OTA_CTRL	4
#define ADC1_DSM_EN_DITHER	3
#define ADC1_DSM_DITHER_LVL	0

/* ADC1_REG2 0x25 */
#define ADC_PAG_CHOPPER_EDGE	7
#define ADC1_DSM_OTA_IB_SEL	4
#define ADC1_DSM_OP_BIAS_CTRL	2
#define ADC1_PGA_OP_BIAS_CTRL	0

/* ADC1_REG3 0x26 */
#define ADC1_PGA_OTA_IB_SEL	5
#define ADC1_PGA_GAIN_CTRL	0

/* ADC1_REG4 0x27 */
#define ADC1_PGA_CHOPPER_EN		7
#define ADC1_PGA_CHOPPER_NOL_EN		6
#define ADC1_PGA_CHOPPER_CKSET		4
#define ADC1_PGA_CHOPPER_DELAY_SET	2
#define ADC1_PGA_CHOPPER_NOL_DELAY_SET	0

/* ADC2_REG1 0x28 */
#define ADC2_EN			7
#define ADC2_DSM_DIS		6
#define ADC2_DSM_OTA_CTRL	4
#define ADC2_DSM_DEMOFF		3
#define ADC2_AVOID_CROSS_EN	2
#define ADC2_MIC_MUX		0

/* ADC2_REG2 0x29 */
#define ADC2_DSM_SEL_OUT_EDGE		7
#define ADC2_DSM_OTA_IB_SEL		4
#define ADC2_DSM_OP_BIAS_CTRL		2
#define ADC2_PGA_OP_BIAS_CTRL		0

/* ADC2_REG3 0x2a */
#define ADC2_PGA_OTA_IB_SEL		5
#define ADC2_PGA_GAIN_CTRL		0

/* ADC2_REG4 0x2b */
#define ADC2_PGA_CHOOPER_EN		7
#define ADC2_PGA_CHOPPER_NOL_EN		6
#define ADC2_PGA_CHOPPER_CKSET		4
#define ADC2_PGA_CHOPPER_DELAY_SET	2
#define ADC2_PGA_CHOPPER_NOL_DELAY_SET	0

/* ADC3_REG1 0x2c */
#define ADC3_EN				7
#define ADC3_DSM_DIS			6
#define ADC3_DSM_OTA_CTRL		4
#define ADC3_DSM_DEMOFF			3
#define ADC1_DSM_DEMOFF			2
#define ADC3_MIC_MUX			1
#define ADC1_DSM_SEL_OUT_EDGE		0

/* ADC3_REG2 0x2d */
#define ADC3_DSM_SEL_OUT_EDGE		7
#define ADC3_DSM_OTA_IB_SEL		4
#define ADC3_DSM_OP_BIAS_CTRL		2
#define ADC3_PGA_OP_BIAS_CTRL		0

/* ADC3_REG3 0x2e */
#define ADC3_PGA_OTA_IB_SEL		5
#define ADC3_PGA_GAIN_CTRL		0

/* ADC3_REG4 0x2f */
#define ADC3_PGA_CHOPPER_EN		7
#define ADC3_PGA_CHOPPER_NOL_EN		6
#define ADC3_PGA_CHOPPER_CKSET		4
#define ADC3_PGA_CHOPPER_DELAY_SET	2
#define ADC3_PGA_CHOPPER_NOL_DELAY_SET	0

/* I2S_CTRL 0x30 */
#define BCLK_IOEN		7
#define LRCK_IOEN		6
#define MCLK_IOEN		5
#define SDO_EN			4
#define OUT_MUTE		3
#define TX_EN			2
#define RX_EN			1
#define I2S_GLOBE_EN		0

/* I2S_BCLK_CTRL 0x31 */
#define EDGE_TRANSFER		5
#define BCLK_POLARITY		4
#define BCLK_DIV		0

/* I2S_LRCK_CTRL1 0x32 */
#define LRCK_WIDTH		5
#define LRCK_POLARITY		4
#define LRCK_PERIODH		0

/* I2S_LRCK_CTRL2 0x33 */
#define LRCK_PERIODL		0

/* I2S_FMT_CTRL1 0x34 */
#define ENCD_FMT		7
#define ENCD_SEL		6
#define MODE_SEL		4
#define OFFSET			2
#define TX_SLOT_HIZ		1
#define TX_STATE		0

/* I2S_FMT_CTRL2 0x35 */
#define SLOT_WIDTH		4
#define SAMPLE_RES		0

/* I2S_FMT_CTRL3 0x36 */
#define RX_MLS			7
#define TX_MLS			6
#define SEXT			4
#define RX_PDM			2
#define TX_PDM			0

/* I2S_TX_CTRL1 0x38 */
#define TX_CHSEL		0

/* I2S_TX_CTRL2 0x39 */
#define	TX_CHEN_LOW		0

/* I2S_TX_CTRL3 0x3a */
#define TX_CHEN_HIGH		0

/* I2S_TX_MIX_CTRL 0x3b */
#define TX_MIX3			4
#define TX_MIX2			2
#define TX_MIX1			0

/* I2S_TX_CHMP_CTRL1 0x3c */
#define TX_CH4_MAP		6
#define TX_CH3_MAP		4
#define TX_CH2_MAP		2
#define TX_CH1_MAP		0

/* I2S_TX_CHMP_CTRL2 0x3d */
#define TX_CH8_MAP		6
#define TX_CH7_MAP		4
#define TX_CH6_MAP		2
#define TX_CH5_MAP		0

/* I2S_TX_CHMP_CTRL3 0x3e */
#define TX_CH12_MAP		6
#define TX_CH11_MAP		4
#define TX_CH10_MAP		2
#define TX_CH9_MAP		0

/* I2S_TX_CHMP_CTRL4 0x3f */
#define TX_CH16_MAP		6
#define TX_CH15_MAP		4
#define TX_CH14_MAP		2
#define TX_CH13_MAP		0

/* I2S_RX_CTRL1 0x40 */
#define RX_CHSEL		0

/* I2S_RX_CTRL2 0x41 */
#define RX_CHEN_LOW		0

/* I2S_RX_CTRL3 0x42 */
#define RX_CHEN_HIGH		0

/* I2S_RX_MIX_CTRL 0x43 */
#define RX_MIX3			4
#define RX_MIX2			2
#define RX_MIX1			0

/* I2S_RX_CHMP_CTRL 0x44 */
#define RXR_MAP			4
#define RXL_MAP			0

/* ADDA_FS_CTRL 0x60 */
#define ADDA_FS_DIV		0

/* ADC_DIG_EN 0x61 */
#define REQ_WIDTH		4
#define ADC_EN			3
#define ADC3_DIG_EN		2
#define ADC2_DIG_EN		1
#define ADC1_DIG_EN		0

/* DMIC_EN_CTRL 0x62 */
#define MIC3_SWP_MIC4		5
#define MIC1_SWP_MIC2		4
#define DMIC_VOL_FIX		3
#define ADC2_SRC_SEL		2
#define ADC1_SRC_SEL		1
#define DMIC_EN			0

/* ADC_DDT_CTRL 0x63 */
#define ADC_DVC_ZCD_EN		7
#define ADC_OSR			5
#define ADC_DLY_MODE		3
#define ADC_DTS			0

/* HPF_EN 0x66 */
#define DIG_ADC3_HPF_EN		2
#define DIG_ADC2_HPF_EN		1
#define DIG_ADC1_HPF_EN		0

/* HPF1_COEF_REGH 0x67 */
/* HPF1_COEF_REGL 0x68 */
/* HPF2_COEF_REGH 0x69 */
/* HPF2_COEF_REGL 0x6a */
/* HPF3_COEF_REGH 0x6b */
/* HPF3_COEF_REGL 0x6c */
/* ADC1_MUX_CTRL 0x6d */
#define DIG_ADC1_MUX		0

/* ADC2_MUX_CTRL 0x6e */
#define DIG_ADC2_MUX		0

/* ADC3_MUX_CTRL 0x6f */
#define DIG_ADC3_MUX		0

/* ADC1_DVOL_CTRL 0x70 */
#define DIG_ADC1_VOL		0

/* ADC2_DVOL_CTRL 0x71 */
#define DIG_ADC2_VOL		0

/* ADC3_DVOL_CTRL 0x72 */
#define DIG_ADC3_VOL		0

/* HMIC_DET_CTRL 0x73 */
#define KEYUP_CLEAR		5
#define HMIC_DATA_IRQ_MODE	4
#define HMIC_SF			2
#define HMIC_SAMPLE_SEL		0

/* HMIC_DET_DBC 0x74 */
#define HMIC_M			4
#define	HMIC_N			0

/* HMIC_DET_TH1 0x75 */
#define HMIC_TH1_HYSTERESIS	5
#define HMIC_TH1		0

/* HMIC_DET_TH2 0x76 */
#define HMIC_TH2_HYSTERESIS	5
#define HMIC_TH2		0

/* HMIC_DET_DATA 0x77 */
#define HMIC_DATA		0

/* HP_DET_CTRL 0x78 */
#define HP_DET			4
#define HMIC_DET_AUTO_DIS	3
#define HMIC_DET_AUTO_EN	2
#define DET_POLY		1
#define HP_DET_EN		0

/* HP_DET_DBC 0x79 */
#define PLUG_OUT_DBC		4
#define PLUG_IN_DBC		0

/* HP_DET_IRQ 0x7a */
#define HP_PLUGOUT_IRQ		6
#define HP_PLUGIN_IRQ		5
#define HMIC_PLUGOUT_IRQ	4
#define HMIC_PLUGIN_IRQ		3
#define HMIC_KEYUP_IRQ		2
#define HMIC_KEYDOWN_IRQ	1
#define HMIC_DATA_IRQ		0

/* HP_DET_STA 0x7b */
#define HP_PLUGOUT_PENDING	6
#define HP_PLUGIN_PENDING	5
#define HMIC_PLUGOUT_PENDING	4
#define HMIC_PLUGIN_PENDING	3
#define HMIC_KEYUP_PENDING	2
#define HMIC_KEYDOWN_PENDING	1
#define HMIC_DATA_PENDING	0

/* ADC_DIG_DEBUG 0x7f */
#define DSM_DITHER_CTRL		4
#define I2S_LPB_DEBUG		3
#define ADC_PTN_SEL		0

/* DAC_DIG_EN 0x81 */
#define DAC_EN			2
#define DACL_DIG_EN		1
#define DACR_DIG_EN		0

/* DAC_DIG_CTRL 0x82 */
#define DVC_ZCD_EN		7
#define DITHER_SGM		4
#define DAC_SWP			2
#define DAC_OSR			0

/* DAC_DHP_GAIN_CTRL 0x83 */
#define DAC_MUTE_DET_T		4
#define DHP_OUTPUT_GAIN		0

/* DAC_LOUT_CTRL 0x84 */
#define ATT_STEP		4
#define LOUT_AUTO_ATT		2
#define LOUT_AUTO_MUTE		1
#define DHP_GAIN_ZC_DEN		0

/* DAC_DVC_L 0x85 */
#define DACL_DIG_VOL		0

/* DAC_DVC_R 0x86 */
#define DACR_DIG_VOL		0

/* EQ_CTRL 0x87 */
#define EQL_EN_CTRL		4
#define EQR_EN_CTRL		0

/* EQ_COEF_CTRL 0x88 */
#define EQ_CH_SEL		7
#define EQ_BAND_CTRL		0

/* DAC_MUX_CTRL 0x89 */
#define DIG_DAC2_MUX		4
#define DIG_DAC1_MUX		0

/* HP_AVR_CTRL 0x8a */
#define HP_AVR_DEN		7
#define HPLDO_HVOLT		4
#define HPLDO_LVOLT		0

/* HP_AVR_THH 0x8b */
/* HP_AVR_THM 0x8c */
/* HP_AVR_THL 0x8d */
/* HP_AVR_DBC 0x8e */
#define	HV_DBC			0

/* HP_LOUT_CTRL 0x8f */
#define HP_ATT_STIP		4
#define HP_LOUT_AUTO_ATT	2
#define HP_GAIN_ZC_DEN		0

/*** Config Value ***/
enum ac101b_pllclk_src {
	PLLCLK_SRC_MCLK,
	PLLCLK_SRC_BCLK,
};

enum ac101b_sysclk_src {
	SYSCLK_SRC_MCLK,
	SYSCLK_SRC_BCLK,
	SYSCLK_SRC_PLL,
};

struct ac101b_status {
	struct mutex dac_chp_mutex;
	bool dacl;
	bool dacr;

	struct mutex spk_chp_mutex;
	bool spkl;
	bool spkr;

	bool adc1_en;
	bool adc2_en;
	bool adc3_en;

	struct mutex mic_mutex;
	bool mic1;
	bool mic2;
	bool mic3;
};

enum SUNXI_JACK_IRQ_STA {
	JACK_IRQ_NULL	= -1,
	JACK_IRQ_OUT	= 0,
	JACK_IRQ_IN,
	JACK_IRQ_KEYDOWN,
};

/* jack */
struct sunxi_jack_adv_priv {
	struct regmap *regmap;
	struct device *dev;

	bool typec;

	unsigned int det_threshold;
	unsigned int key_threshold;
	unsigned int det_debounce;
	unsigned int key_debounce;

	/* key_det_vol[0][0] - key_hook_vol_min,  key_det_vol[0][1] - key_hook_vol_max
	 * key_det_vol[1][0] - key_up_vol_min,    key_det_vol[1][1] - key_up_vol_max
	 * key_det_vol[2][0] - key_down_vol_min,  key_det_vol[2][1] - key_down_vol_max
	 * key_det_vol[3][0] - key_voice_vol_min, key_det_vol[3][1] - key_voice_vol_max
	 */
	unsigned int key_det_vol[4][2];

	enum SUNXI_JACK_IRQ_STA irq_sta;

	// gpio get irq
	int irq_gpio;
	struct gpio_desc *desc;

	enum snd_jack_types jack_type;

	/* pa config */
	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;
};

struct ac101b_data {

	unsigned int adc1_vol;
	unsigned int adc2_vol;
	unsigned int adc3_vol;

	unsigned int dacl_vol;
	unsigned int dacr_vol;

	unsigned int mic1_gain;
	unsigned int mic2_gain;
	unsigned int mic3_gain;

	unsigned int hpout_gain;

	unsigned int codec_id;
	unsigned int ecdn_mode;

	enum ac101b_pllclk_src pllclk_src;
	enum ac101b_sysclk_src sysclk_src;

	struct ac101b_status ac101b_sta;

	unsigned int pcm_bit_first:1;
	unsigned int frame_sync_width;

	struct sunxi_jack_adv_priv jack_adv_priv;
};

#endif

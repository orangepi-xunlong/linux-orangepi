/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * ac101.h -- AC101 ALSA SoC Audio driver
 *
 * Copyright (c) 2022 Allwinnertech Ltd.
 */

#ifndef __AC101_H
#define __AC101_H

/*** AC101 Codec Register Define ***/
#define CHIP_SOFT_RST		0x00

/* PLL Control*/
#define PLL_CTRL1		0x01
#define PLL_CTRL2		0x02

/* System Clock Control */
#define SYSCLK_CTRL		0x03
#define MOD_CLK_ENA		0x04
#define MOD_RST_CTRL		0x05

/* I2S Common Control */
#define I2S_SR_CTRL		0x06
#define I2S_CLK_CTRL		0x10
#define I2S_SDOUT_CTRL		0x11
#define I2S_SDIN_CTRL		0x12
#define I2S_DIG_MXR_SRC		0x13
#define I2S_VOL_CTRL1		0x14
#define I2S_VOL_CTRL2		0x15
#define I2S_VOL_CTRL3		0x16
#define I2S_VOL_CTRL4		0x17
#define I2S_MXR_GAIN		0x18

/* ADC Digital Control */
#define ADC_DIG_CTRL		0x40
#define ADC_VOL_CTRL		0x41

/* HMIC Control */
#define HMIC_CTRL1		0x44
#define HMIC_CTRL2		0x45
#define HMIC_STS		0x46

/* DAC Digital Control */
#define DAC_DIG_CTRL		0x48
#define DAC_VOL_CTRL		0x49
#define DAC_MXR_SRC		0x4c
#define DAC_MXR_GAIN		0x4d

/* ADC Analog Control */
#define ADC_APC_CTRL		0x50
#define ADC_SRC			0x51
#define ADC_SRCBST_CTRL		0x52

/* DAC Analog Control */
#define OMIXER_DACA_CTRL	0x53
#define OMIXER_SR		0x54
#define OMIXER_BST1_CTRL	0x55

#define HPOUT_CTRL		0x56
#define SPKOUT_CTRL		0x58
//ADC DAP Control
#define ADC_DAPL_CTRL		0x82
#define ADC_DAPR_CTRL		0x83

//DAC DRC/HPF Control
#define DAC_DAP_CTRL		0xa0
#define DAC_HPF_COEF_H		0xa1
#define DAC_HPF_COEF_L		0xa2
#define DACL_DRC_ENERGY_H	0xa3
#define DACL_DRC_ENERGY_L	0xa4
#define DACR_DRC_ENERGY_H	0xa5
#define DACR_DRC_ENERGY_L	0xa6
#define DAC_DRC_DECAY_H		0xa7
#define DAC_DRC_DECAY_L		0xa8
#define DAC_DRC_ATTACK_H	0xa9
#define DAC_DRC_ATTACK_L	0xaa
#define DAC_DRC_THRESHOLD_H	0xab
#define DAC_DRC_THRESHOLD_L	0xac
#define DAC_DRC_K_PARAM_H	0xad
#define DAC_DRC_K_PARAM_L	0xae
#define DAC_DRC_OFFSET_H	0xaf
#define DAC_DRC_OFFSET_L	0xb0
#define DAC_DRC_OPTIMUM		0xb1
#define AGC_ENA			0xb4
#define DRC_ENA			0xb5

#define AC101_MAX_REG		DRC_ENA

/* PLL_CTRL1 0x1*/
#define PLL_DIV_M		8
#define CLOSE_LOOP		7
#define INT			0

/* PLL_CTRL2 0x2*/
#define PLL_EN			15
#define PLL_LOCK_STATUS		14
#define PLL_PREDIV_NI		4
#define PLL_POSTDIV_NF		0

/* SYSCLK_CTRL 0x3 */
#define PLLCLK_EN		15
#define PLLCLK_SRC		12
#define I2SCLK_EN		11
#define I2SCLK_SRC		8
#define SYSCLK_EN		3

/* MOD_CLK_ENA 0x4 */
#define I2S_MOD			15
#define HPF_AGC_MOD		7
#define HPF_DRC_MOD		6
#define ADC_DIG_MOD		3
#define DAC_DIG_MOD		2

/* MOD_RST_CTRL 0x5 */
#define I2S_MOD_RST		15
#define HPF_AGC_MOD_RST		7
#define HPF_DRC_MOD_RST		6
#define ADC_DIG_MOD_RST		3
#define DAC_DIG_MOD_RST		2

/* I2S_SR_CTRL 0x6*/
#define ADDA_FS			12

/* I2S_CLK_CTRL 0x10 */
#define I2S_MSTR		15
#define I2S_BCLK_INV		14
#define I2S_LRCK_INV		13
#define BCLK_DIV		9
#define	LRCK_DIV		6
#define WORD_SIZE		4
#define DATA_FMT		2
#define MONO_PCM		1
#define TDM_EN			0

/* I2S_SDOUT_CTRL 0x11 */
#define I2S_ADC_SLOT0L_EN	15
#define I2S_ADC_SLOT0R_EN	14
#define I2S_ADC_SLOT1L_EN	13
#define I2S_ADC_SLOT1R_EN	12
#define I2S_ADCL0_SRC		10
#define I2S_ADCR0_SRC		8
#define I2S_ADCL1_SRC		6
#define I2S_ADCR1_SRC		4
#define ADCP_EN			3
#define ADCP_SEL		2
#define SLOT_SIZE		0

/* I2S_SDIN_CTRL 0x12 */
#define I2S_DAC_SLOT0L_EN	15
#define I2S_DAC_SLOT0R_EN	14
#define I2S_DAC_SLOT1L_EN	13
#define I2S_DAC_SLOT1R_EN	12
#define I2S_DA0L_SRC		10
#define I2S_DA0R_SRC		8
#define I2S_DA1L_SRC		6
#define I2S_DA1R_SRC		4
#define DACP_EN			3
#define DACP_SEL		2
#define I2S_LOOP		0

/* I2S_DIG_MXR_SRC 0x13 */
#define I2SDA0_LMXR_DA0L	15
#define I2SDA0_LMXR_ADCL	13
#define I2SDA0_RMXR_DA0R	11
#define I2SDA0_RMXR_ADCR	9
#define I2SDA1_LMXR_ADCL	6
#define I2SDA1_RMXR_ADCR	2

/* I2S_VOL_CTRL1 0x14 */
#define ADCL0_VOL		8
#define ADCR0_VOL		0

/* I2S_VOL_CTRL2 0x15 */
#define ADCL1_VOL		8
#define ADCR1_VOL		0

/* I2S_VOL_CTRL3 0x16 */
#define DACL0_VOL		8
#define DACR0_VOL		0

/* I2S_VOL_CTRL4 0x17 */
#define DACL1_VOL		8
#define DACR1_VOL		0

/* I2S_MXR_GAIN 0x18 */
#define ADCL0_MXR_GAIN		12
#define ADCR0_MXR_GAIN		8
#define ADCL1_MXR_GAIN		6
#define ADCR1_MXR_GAIN		2

/* ADC_DIG_CTRL 0x40 */
#define ADC_DIG_EN		15
#define DIG_MIC_EN		14
#define ADFIR32			13
#define ADOUT_DTS		2
#define ADOUT_DLY		1

/* ADC_VOL_CTRL 0x41*/
#define ADCL_VOL		8
#define ADCR_VOL		0

/* HMIC_CTRL1 0x44 */
#define HMIC_M			12
#define HMIC_N			8
#define DATA_IRQ_MOD		7
#define TH1_HYSTER		5
#define PULLOUT_IRQ_EN		4
#define PULLIN_IRQ_EN		3
#define KEYUP_IRQ_EN		2
#define KEYDOWN_IRQ_EN		1
#define DATA_IRQ_EN		0

/* HMIC_CTRL2 0x45 */
#define SAMPLE_SEL		14
#define TH2_HYSTER		13
#define HMIC_TH2		8
#define HMIC_SF			6
#define KEYUP_CLEAR		5
#define HMIC_TH1		0

/* HMIC_STS 0x46 */
#define HMIC_DATA		8
#define PLUGOUT_PEND		4
#define PLUGIN_PEND		3
#define KEYUP_PEND		2
#define KEYDOWN_PEND		1
#define DATA_PEND		0

/* DAC_DIG_CTRL 0x48 */
#define ENDA			15
#define ENHPF			14
#define DAFIR32			13
#define MODQU			8

/* DAC_VOL_CTRL 0x49 */
#define DACL_VOL		8
#define DACR_VOL		0

/* DAC_MXR_SRC 0x4c */
#define DAC_LMXR_I2S_DA0L	15
#define DAC_LMXR_I2S_DA1L	14
#define DAC_LMXR_ADCL		12
#define DAC_RMXR_I2S_DA0R	11
#define DAC_RMXR_I2S_DA1R	10
#define DAC_RMXR_ADCR		8

/* DAC_MXR_GAIN 0x4d */
#define DACL_MXR_GAIN		12
#define DACR_MXR_GAIN		8

/* ADC_APC_CTRL 0x50 */
#define ADCREN			15
#define ADCRG			12
#define ADCLEN			11
#define ADCLG			8
#define MBIASEN			7
#define CHOPPER_EN		6
#define CHOPPER_CKS		4
#define HBIASMOD		2
#define HBIAS_EN		1
#define HBIAS_ADC_EN		0

/* ADC_SRC 0x51 */
/* RADC_MIXMUTE */
#define MIC1_TO_RADC		13
#define MIC2_TO_RADC		12
#define LINEIN_TO_RADC		11
#define LINEINR_TO_RADC		10
#define OUTPUTR_MIX_TO_RADC	8
#define OUTPUTL_MIX_TO_RADC	7
/* LADC_MIXMUTE */
#define MIC1_TO_LADC		6
#define MIC2_TO_LADC		5
#define LINEIN_TO_LADC		4
#define LINEINL_TO_LADC		3
#define OUTPUTL_MIX_TO_LADC	1
#define OUTPUTR_MIX_TO_LADC	0

/* ADC_SRCBST_CTRL 0x52 */
#define MIC1AMPEN		15
#define MIC1BOOST		12
#define MIC2AMPEN		11
#define MIC2BOOST		8
#define MIC2PIN_EN		7
#define LINEIN_PREG		4

/* OMIXER_DACA_CTRL 0x53 */
#define DACAREN			15
#define DACALEN			14
#define RMIXEN			13
#define LMIXEN			12
#define HP_DCRM_EN		8

/* OMIXER_SR 0x54 */
#define RMXR_MIC1		13
#define RMXR_MIC2		12
#define RMXR_LINEIN		11
#define RMXR_LINEINR		10
#define RMXR_DACR		8
#define RMXR_DACL		7
#define LMXR_MIC1		6
#define LMXR_MIC2		5
#define LMXR_LINEIN		4
#define LMXR_LINEINL		3
#define LMXR_DACL		1
#define LMXR_DACR		0

/* OMIXER_BST1_CTRL 0x55 */
#define HBIASSEL		14
#define MBIASSEL		12
#define MIC1G			6
#define MIC2G			3
#define LINEING			0

/* HPOUT_CTRL 0x56 */
#define RHPSRC			15
#define LHPSRC			14
#define MUTE_R			13
#define MUTE_L			12
#define HPPA_EN			11
#define HP_VOL			4
#define HPPA_DELAY		2
#define HPPA_ST			0

/* SPKOUT_CTRL 0x58 */
#define CLK_ADJUST		13
#define RSPKSRC			12
#define RSPKINVEN		11
#define RSPK_EN			9
#define LSPKSRC			8
#define LSPKINVEN		7
#define LSPK_EN			5
#define SPK_VOL			0

/* ADC_DAPL_CTRL */
#define L_HPF_EN		13

/* ADC_DAPR_CTRL */
#define R_HPF_EN		13

/* AGC_ENA */
#define ADCL_AGC_EN		7
#define ADCR_AGC_EN		6

/*** Config Value ***/
enum ac101_pllclk_src {
	PLLCLK_SRC_MCLK,
	PLLCLK_SRC_BCLK,
};

enum ac101_sysclk_src {
	SYSCLK_SRC_MCLK,
	SYSCLK_SRC_PLL,
};

enum SUNXI_JACK_IRQ_TIME {
	JACK_IRQ_NORMAL	= 0,
	JACK_IRQ_SCAN,
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
	enum SUNXI_JACK_IRQ_TIME irq_time;

	// gpio get irq
	int det_gpio;
	struct gpio_desc *det_desc;

	int plug_gpio;
	struct gpio_desc *plug_desc;

	enum snd_jack_types jack_type;

	int irq;

	/* pa config */
	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;
};

struct ac101_data {
	unsigned int adcl_vol;
	unsigned int adcr_vol;

	unsigned int dacl_vol;
	unsigned int dacr_vol;

	unsigned int adcl_gain;
	unsigned int adcr_gain;

	unsigned int mic1_gain;
	unsigned int mic2_gain;

	unsigned int hpout_vol;
	unsigned int spk_vol;

	unsigned int codec_id;
	unsigned int ecdn_mode;

	enum ac101_pllclk_src pllclk_src;
	enum ac101_sysclk_src sysclk_src;

	unsigned int pcm_bit_first:1;
	unsigned int frame_sync_width;

	atomic_t working;

	struct sunxi_jack_adv_priv jack_adv_priv;
};


#endif

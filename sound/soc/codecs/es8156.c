/*
 * es8156.c -- es8156 ALSA SoC audio driver
 * Copyright Everest Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include "es8156.h"

#define INVALID_GPIO -1
#define GPIO_LOW  0
#define GPIO_HIGH 1
#define es8156_DEF_VOL			0xBF
#define ES8156_VOL_MAX			0xBF
/*
* If your system doesn't have MCLK, define this to 0 or the
* driver will crash.
*/
#define MCLK 0

static struct snd_soc_component *es8156_codec;

static const struct reg_default es8156_reg_defaults[] = {
	{0x00, 0x1c}, {0x01, 0x20}, {0x02, 0x00}, {0x03, 0x01},
	{0x04, 0x00}, {0x05, 0x04}, {0x06, 0x11}, {0x07, 0x00},
	{0x08, 0x06}, {0x09, 0x00}, {0x0a, 0x50}, {0x0b, 0x50},
	{0x0c, 0x00}, {0x0d, 0x10}, {0x10, 0x40}, {0x10, 0x40},
	{0x11, 0x00}, {0x12, 0x04}, {0x13, 0x11}, {0x14, 0xbf},
	{0x15, 0x00}, {0x16, 0x00}, {0x17, 0xf7}, {0x18, 0x00},
	{0x19, 0x20}, {0x1a, 0x00}, {0x20, 0x16}, {0x21, 0x7f},
	{0x22, 0x00}, {0x23, 0x86}, {0x24, 0x00}, {0x25, 0x07},
	{0xfc, 0x00}, {0xfd, 0x81}, {0xfe, 0x55}, {0xff, 0x10},
};

/* codec private data */
struct es8156_priv {
	struct regmap *regmap;
	unsigned int dmic_amic;
	unsigned int sysclk;
	struct snd_pcm_hw_constraint_list *sysclk_constraints;
	struct clk *mclk;
	struct regulator *avdd;
	struct regulator *dvdd;
	struct regulator *pvdd;
	int debounce_time;
	int hp_det_invert;
	struct delayed_work work;

	int spk_ctl_gpio;
	int hp_det_gpio;
	bool muted;
	bool hp_inserted;
	bool spk_active_level;

	int pwr_count;
	u32 mclk_sclk_ratio;

	u32 suspend_reg_00;
	u32 suspend_reg_01;
	u32 suspend_reg_02;
	u32 suspend_reg_03;
	u32 suspend_reg_04;
	u32 suspend_reg_05;
	u32 suspend_reg_06;
	u32 suspend_reg_07;
	u32 suspend_reg_08;
	u32 suspend_reg_09;
	u32 suspend_reg_0A;
	u32 suspend_reg_0B;
	u32 suspend_reg_0C;
	u32 suspend_reg_0D;
	u32 suspend_reg_10;
	u32 suspend_reg_11;
	u32 suspend_reg_12;
	u32 suspend_reg_13;
	u32 suspend_reg_14;
	u32 suspend_reg_15;
	u32 suspend_reg_16;
	u32 suspend_reg_17;
	u32 suspend_reg_18;
	u32 suspend_reg_19;
	u32 suspend_reg_1A;
	u32 suspend_reg_20;
	u32 suspend_reg_21;
	u32 suspend_reg_22;
	u32 suspend_reg_23;
	u32 suspend_reg_24;
	u32 suspend_reg_25;
};

/*
 * es8156_reset
 */
static int es8156_reset(struct snd_soc_component *codec)
{
	snd_soc_component_write(codec, ES8156_RESET_REG00, 0x1c);
	usleep_range(5000, 5500);
	return snd_soc_component_write(codec, ES8156_RESET_REG00, 0x03);
}

static void es8156_enable_spk(struct es8156_priv *es8156, bool enable)
{
	bool level;

	level = enable ? es8156->spk_active_level : !es8156->spk_active_level;
	gpio_set_value(es8156->spk_ctl_gpio, level);
}


static const char *es8156_DAC_SRC[] = { "Left to Left, Right to Right",
"Right to both Left and Right","Left to both Left & Right", "Left to Right, Right to Left" };

static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -9600, 50, 1);
static const DECLARE_TLV_DB_SCALE(alc_gain_tlv,-2800,400,1);
static SOC_ENUM_SINGLE_DECL(es8165_dac_enum, ES8156_MISC_CONTROL3_REG18, 4, es8156_DAC_SRC);

static const struct snd_kcontrol_new es8156_DAC_MUX =SOC_DAPM_ENUM("Route", es8165_dac_enum);

static const struct snd_kcontrol_new es8156_snd_controls[] = {
	SOC_SINGLE("Timer 1",ES8156_TIME_CONTROL1_REG0A,0,63,0),
	SOC_SINGLE("Timer 2",ES8156_TIME_CONTROL2_REG0B,0,63,0),
	SOC_SINGLE("DAC Automute Gate",ES8156_AUTOMUTE_SET_REG12,4,15,0),
	SOC_SINGLE_TLV("ALC Gain",ES8156_ALC_CONFIG1_REG15,1,7,1,alc_gain_tlv),
	SOC_SINGLE("ALC Ramp Rate",ES8156_ALC_CONFIG2_REG16,4,15,0),
	SOC_SINGLE("ALC Window Size",ES8156_ALC_CONFIG2_REG16,0,15,0),
	SOC_DOUBLE("ALC Maximum Minimum Volume",ES8156_ALC_CONFIG3_REG17,
	4,0,15,0),
	/* DAC Digital controls */
	SOC_SINGLE_TLV("DAC Playback Volume", ES8156_VOLUME_CONTROL_REG14,
			  0, ES8156_VOL_MAX, 0, dac_vol_tlv),
	SOC_SINGLE("HP Switch",ES8156_ANALOG_SYS3_REG22,3,1,0),


};


static const struct snd_soc_dapm_widget es8156_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_OUT("AIFSDOUT", "I2S Capture", 0,
			     ES8156_P2S_CONTROL_REG0D, 2, 0),

	SND_SOC_DAPM_AIF_IN("SDIN", "I2S Playback", 0,
			    SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_MUX("Channel Select Mux", SND_SOC_NOPM, 0, 0,
			 &es8156_DAC_MUX),

	SND_SOC_DAPM_DAC("DACL", "Left Playback", ES8156_DAC_MUTE_REG13, 1, 1),
	SND_SOC_DAPM_DAC("DACR", "Right Playback", ES8156_DAC_MUTE_REG13, 2, 1),


	SND_SOC_DAPM_PGA("SDOUT TRISTATE", ES8156_P2S_CONTROL_REG0D,
			 0, 1, NULL, 0),

	SND_SOC_DAPM_OUTPUT("SDOUT"),
	SND_SOC_DAPM_OUTPUT("LOUT"),
	SND_SOC_DAPM_OUTPUT("ROUT"),
};

/*************** parameter define ***************/
#define STATEconfirm       0x0C
#define NORMAL_I2S         0x00
#define NORMAL_LJ          0x01
#define NORMAL_DSPA        0x03
#define NORMAL_DSPB        0x07
#define Format_Len24       0x00
#define Format_Len20       0x01
#define Format_Len18       0x02
#define Format_Len16       0x03
#define Format_Len32       0x04

#define VDDA_3V3           0x00
#define VDDA_1V8           0x01
#define MCLK_PIN           0x00
#define SCLK_PIN           0x01

/**************************************************/
#define MSMode_MasterSelOn  0                 // SlaveMode:0, MasterMode:1
static unsigned int Ratio = 64;               // Ratio = MCLK/LRCK on board
#define Format              NORMAL_I2S
#define Format_Len          Format_Len16      // data format
#define SCLK_DIV            8                 // SCLK_DIV = MCLK/SCLK
#define SCLK_INV            0
static unsigned int         MCLK_SOURCE;      // select MCLK source, MCLK_PIN or SCLK_PIN
#define EQ7bandOn           0
#define VDDA_VOLTAGE        VDDA_3V3
#define DAC_Volume          191               // DAC digital gain
#define DACHPModeOn         0                 // disable:0, enable:1

/**************************************************/

static int es8156_init_regs(struct snd_soc_component *codec)
{
	//struct es8156_priv *priv = snd_soc_component_get_drvdata(codec);
   snd_soc_component_write(codec,0x02,(MCLK_SOURCE<<7) + (SCLK_INV<<4) +  (EQ7bandOn<<3) + 0x04 + MSMode_MasterSelOn);
   snd_soc_component_write(codec,0x19,0x20);

   if (DACHPModeOn == 0) {  // output from PA
		snd_soc_component_write(codec,0x20,0x2A);
		snd_soc_component_write(codec,0x21,0x3C);
		snd_soc_component_write(codec,0x22,0x02);
		snd_soc_component_write(codec,0x24,0x07);
		snd_soc_component_write(codec,0x23,0x40 + (0x30*VDDA_VOLTAGE));
    } else if (DACHPModeOn == 1) {  // output from headphone
		snd_soc_component_write(codec,0x20,0x16);
		snd_soc_component_write(codec,0x21,0x3F);
		snd_soc_component_write(codec,0x22,0x0A);
		snd_soc_component_write(codec,0x24,0x01);
		snd_soc_component_write(codec,0x23,0xCA + (0x30*VDDA_VOLTAGE));
    }
   snd_soc_component_write(codec,0x0A,0x01);
   snd_soc_component_write(codec,0x0B,0x01);
   //snd_soc_component_write(codec,0x11,NORMAL_I2S + (Format_Len<<4));

   if (Ratio == 1536) {// Ratio=MCLK/LRCK=1536; 12M288/8K; 24M576/16K
		snd_soc_component_write(codec,0x01,0x26 - (0x03*EQ7bandOn)); // 1536 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x09,0x00); // 1536 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x03,0x06); // LRCK H
		snd_soc_component_write(codec,0x04,0x00); // LRCK=MCLK/1536
		snd_soc_component_write(codec,0x05,SCLK_DIV); // BCLK=MCLK/4
	} else if (Ratio == 1024) {// Ratio=MCLK/LRCK=1024; 12M288/12K; 24M576/24K
		snd_soc_component_write(codec,0x01,0x24 - (0x02*EQ7bandOn)); // 256 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x09,0x00); // 256 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x03,0x04); // LRCK H
		snd_soc_component_write(codec,0x04,0x00); // LRCK=MCLK/256
		snd_soc_component_write(codec,0x05,SCLK_DIV); // BCLK=MCLK/4
	} else if (Ratio == 768) {// Ratio=MCLK/LRCK=768; 12M288/16K; 24M576/32K
		snd_soc_component_write(codec,0x01,0x23 + (0x40*EQ7bandOn)); // 768 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x09,0x00); // 768 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x03,0x03); // LRCK H
		snd_soc_component_write(codec,0x04,0x00); // LRCK=MCLK/768
		snd_soc_component_write(codec,0x05,SCLK_DIV); // BCLK=MCLK/4
	} else if (Ratio == 512) {// Ratio=MCLK/LRCK=512; 12M288/24K; 24M576/48K
		snd_soc_component_write(codec,0x01,0xC0 + 0x22 - (0x01*EQ7bandOn)); // 512 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x09,0x00); // 512 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x03,0x02); // LRCK H
		snd_soc_component_write(codec,0x04,0x00); // LRCK=MCLK/512
		snd_soc_component_write(codec,0x05,SCLK_DIV); //BCLK=MCLK/4
	} else if (Ratio == 400) {// Ratio=MCLK/LRCK=400; 19M2/48K
		// DVDD must be 3.3V
		snd_soc_component_write(codec,0x01,0x21 + (0x40*EQ7bandOn)); // 384 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x09,0x00); // 400 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x10,0x64); // 400 OSR
		snd_soc_component_write(codec,0x03,0x01); // LRCK H
		snd_soc_component_write(codec,0x04,0x90); // LRCK=MCLK/400
		snd_soc_component_write(codec,0x05,SCLK_DIV); // BCLK=MCLK/4
	} else if (Ratio == 384) {// Ratio=MCLK/LRCK=384; 12M288/32K; 6M144/16K
		snd_soc_component_write(codec,0x01,0x63 + (0x40*EQ7bandOn)); // 384 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x09,0x00); // 384 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x03,0x01); // LRCK H
		snd_soc_component_write(codec,0x04,0x80); // LRCK=MCLK/384
		snd_soc_component_write(codec,0x05,SCLK_DIV); // BCLK=MCLK/4
	} else if (Ratio == 256) {// Ratio=MCLK/LRCK=256; 12M288/48K
		snd_soc_component_write(codec,0x01,0x21 + (0x40*EQ7bandOn)); // 256 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x09,0x00); // 256 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x03,0x01); // LRCK H
		snd_soc_component_write(codec,0x04,0x00); // LRCK=MCLK/256
		snd_soc_component_write(codec,0x05,SCLK_DIV); // BCLK=MCLK/4
	} else if(Ratio == 128) {// Ratio=MCLK/LRCK=128; 6M144/48K
		snd_soc_component_write(codec,0x01,0x61 + (0x40*EQ7bandOn)); // 128 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x09,0x00); // 128 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x03,0x00); // LRCK H
		snd_soc_component_write(codec,0x04,0x80); // LRCK=MCLK/128
		snd_soc_component_write(codec,0x05,SCLK_DIV); // BCLK=MCLK/4
	} else if(Ratio == 64) {// Ratio=MCLK/LRCK=64; 3M072/48K
		snd_soc_component_write(codec,0x01,0xE1); // 64 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x09,0x02); // 64 Ratio(MCLK/LRCK)
		snd_soc_component_write(codec,0x03,0x00); // LRCK H
		snd_soc_component_write(codec,0x04,0x40); // LRCK=MCLK/64
		snd_soc_component_write(codec,0x05,SCLK_DIV); // BCLK=MCLK/2
	}

   snd_soc_component_write(codec,0x0D,0x14);
   snd_soc_component_write(codec,0x18,0x00);
   snd_soc_component_write(codec,0x08,0x3F);
   snd_soc_component_write(codec,0x00,0x02);
   snd_soc_component_write(codec,0x00,0x03);
   snd_soc_component_write(codec,0x25,0x20);

   return 0;
}

static int es8156_init_sequence(struct snd_soc_component *codec)
{
	es8156_init_regs(codec);
	snd_soc_component_write(codec, ES8156_VOLUME_CONTROL_REG14, DAC_Volume);

	return 0;
}

static const struct snd_soc_dapm_route es8156_dapm_routes[] = {
	{"SDOUT TRISTATE",NULL,"SDIN"},
	{"SDOUT",NULL,"SDOUT TRISTATE"},

	{"Channel Select Mux","Left to Left, Right to Right","SDIN"},
	{"Channel Select Mux","Right to both Left and Right","SDIN"},
	{"Channel Select Mux","Left to both Left & Right","SDIN"},
	{"Channel Select Mux","Left to Right, Right to Left","SDIN"},

	{"DACL",NULL,"Channel Select Mux"},
	{"DACR",NULL,"Channel Select Mux"},



	{ "LOUT", NULL, "DACL" },
	{ "ROUT", NULL, "DACR" },
};

static int es8156_set_dai_fmt(struct snd_soc_dai *codec_dai,
			      unsigned int fmt)
{
	struct snd_soc_component *codec = codec_dai->component;
	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM://es8156 master
		snd_soc_component_update_bits(codec, ES8156_SCLK_MODE_REG02, 0x01,0x01);
		break;
	case SND_SOC_DAIFMT_CBS_CFS://es8156 slave
		snd_soc_component_update_bits(codec, ES8156_SCLK_MODE_REG02, 0x01,0x00);
		break;
	default:
		return -EINVAL;
	}
	/* interface format */

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		snd_soc_component_update_bits(codec, ES8156_DAC_SDP_REG11, 0x07,0x00);
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		return -EINVAL;
	case SND_SOC_DAIFMT_LEFT_J:
		snd_soc_component_update_bits(codec, ES8156_DAC_SDP_REG11, 0x07,0x01);
		break;
	case SND_SOC_DAIFMT_DSP_A:
		snd_soc_component_update_bits(codec, ES8156_DAC_SDP_REG11, 0x07,0x03);
		break;
	case SND_SOC_DAIFMT_DSP_B:
		snd_soc_component_update_bits(codec, ES8156_DAC_SDP_REG11, 0x07,0x07);
		break;
	default:
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		snd_soc_component_update_bits(codec, ES8156_SCLK_MODE_REG02, 0x01,0x00);
		break;
	case SND_SOC_DAIFMT_IB_IF:
		snd_soc_component_update_bits(codec, ES8156_SCLK_MODE_REG02, 0x01,0x01);
		break;
	case SND_SOC_DAIFMT_IB_NF:
		snd_soc_component_update_bits(codec, ES8156_SCLK_MODE_REG02, 0x01,0x01);
		break;
	case SND_SOC_DAIFMT_NB_IF:
		snd_soc_component_update_bits(codec, ES8156_SCLK_MODE_REG02, 0x01,0x00);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static int es8156_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_component *codec = dai->component;
	struct snd_pcm_runtime *runtime = substream->runtime;

	snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
				     4096, UINT_MAX);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		snd_soc_component_update_bits(codec, ES8156_DAC_SDP_REG11, 0x70,0x30);
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		snd_soc_component_update_bits(codec, ES8156_DAC_SDP_REG11, 0x70,0x10);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		snd_soc_component_update_bits(codec, ES8156_DAC_SDP_REG11, 0x70,0x00);
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		snd_soc_component_update_bits(codec, ES8156_DAC_SDP_REG11, 0x70,0x40);
		break;
	}
	return 0;
}

static int es8156_set_bias_level(struct snd_soc_component *codec,
				 enum snd_soc_bias_level level)
{
	//int ret, i;
	//struct es8156_priv *priv = snd_soc_component_get_drvdata(codec);

	switch (level)
	{
		case SND_SOC_BIAS_ON:
			break;

		case SND_SOC_BIAS_PREPARE:
			break;

		case SND_SOC_BIAS_STANDBY:
		break;

	case SND_SOC_BIAS_OFF:
		break;
	}
	return 0;
}

#define es8156_RATES SNDRV_PCM_RATE_8000_96000

#define es8156_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FORMAT_S32_LE)

static struct snd_soc_dai_ops es8156_ops = {
    .startup = NULL,
	.hw_params = es8156_pcm_hw_params,
	.set_fmt = es8156_set_dai_fmt,
	.set_sysclk = NULL,
	.shutdown = NULL,
};

static struct snd_soc_dai_driver es8156_dai = {
	.name = "ES8156 HiFi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = es8156_RATES,
		.formats = es8156_FORMATS,
	},
	.ops = &es8156_ops,
};

#ifdef CONFIG_PM_SLEEP
static int es8156_suspend(struct snd_soc_component *codec)
{
	struct es8156_priv *priv = snd_soc_component_get_drvdata(codec);

	pr_debug("Entered: %s\n", __func__);

	es8156_set_bias_level(codec, SND_SOC_BIAS_OFF);
	priv->suspend_reg_00 = snd_soc_component_read(codec, ES8156_RESET_REG00);
	priv->suspend_reg_01 = snd_soc_component_read(codec, ES8156_MAINCLOCK_CTL_REG01);
	priv->suspend_reg_02 = snd_soc_component_read(codec, ES8156_SCLK_MODE_REG02);
	priv->suspend_reg_03 = snd_soc_component_read(codec, ES8156_LRCLK_DIV_H_REG03);
	priv->suspend_reg_04 = snd_soc_component_read(codec, ES8156_LRCLK_DIV_L_REG04);
	priv->suspend_reg_05 = snd_soc_component_read(codec, ES8156_SCLK_DIV_REG05);
	priv->suspend_reg_06 = snd_soc_component_read(codec, ES8156_NFS_CONFIG_REG06);
	priv->suspend_reg_07 = snd_soc_component_read(codec, ES8156_MISC_CONTROL1_REG07);
	priv->suspend_reg_08 = snd_soc_component_read(codec, ES8156_CLOCK_ON_OFF_REG08);
	priv->suspend_reg_09 = snd_soc_component_read(codec, ES8156_MISC_CONTROL2_REG09);
	priv->suspend_reg_0A = snd_soc_component_read(codec, ES8156_TIME_CONTROL1_REG0A);
	priv->suspend_reg_0B = snd_soc_component_read(codec, ES8156_TIME_CONTROL2_REG0B);
	priv->suspend_reg_0C = snd_soc_component_read(codec, ES8156_CHIP_STATUS_REG0C);
	priv->suspend_reg_0D = snd_soc_component_read(codec, ES8156_P2S_CONTROL_REG0D);
	priv->suspend_reg_10 = snd_soc_component_read(codec, ES8156_DAC_OSR_COUNTER_REG10);
	priv->suspend_reg_11 = snd_soc_component_read(codec, ES8156_DAC_SDP_REG11);
	priv->suspend_reg_12 = snd_soc_component_read(codec, ES8156_AUTOMUTE_SET_REG12);
	priv->suspend_reg_13 = snd_soc_component_read(codec, ES8156_DAC_MUTE_REG13);
	priv->suspend_reg_14 = snd_soc_component_read(codec, ES8156_VOLUME_CONTROL_REG14);
	priv->suspend_reg_15 = snd_soc_component_read(codec, ES8156_ALC_CONFIG1_REG15);
	priv->suspend_reg_16 = snd_soc_component_read(codec, ES8156_ALC_CONFIG2_REG16);
	priv->suspend_reg_17 = snd_soc_component_read(codec, ES8156_ALC_CONFIG3_REG17);
	priv->suspend_reg_18 = snd_soc_component_read(codec, ES8156_MISC_CONTROL3_REG18);
	priv->suspend_reg_19 = snd_soc_component_read(codec, ES8156_EQ_CONTROL1_REG19);
	priv->suspend_reg_1A = snd_soc_component_read(codec, ES8156_EQ_CONTROL2_REG1A);
	priv->suspend_reg_20 = snd_soc_component_read(codec, ES8156_ANALOG_SYS1_REG20);
	priv->suspend_reg_21 = snd_soc_component_read(codec, ES8156_ANALOG_SYS2_REG21);
	priv->suspend_reg_22 = snd_soc_component_read(codec, ES8156_ANALOG_SYS3_REG22);
	priv->suspend_reg_23 = snd_soc_component_read(codec, ES8156_ANALOG_SYS4_REG23);
	priv->suspend_reg_24 = snd_soc_component_read(codec, ES8156_ANALOG_LP_REG24);
	priv->suspend_reg_25 = snd_soc_component_read(codec, ES8156_ANALOG_SYS5_REG25);

	return 0;
}

static int es8156_resume(struct snd_soc_component *codec)
{
	struct es8156_priv *priv = snd_soc_component_get_drvdata(codec);

	pr_debug("Entered: %s\n", __func__);
	snd_soc_component_write(codec, ES8156_RESET_REG00, priv->suspend_reg_00);
	snd_soc_component_write(codec, ES8156_MAINCLOCK_CTL_REG01, priv->suspend_reg_01);
	snd_soc_component_write(codec, ES8156_SCLK_MODE_REG02, priv->suspend_reg_02);
	snd_soc_component_write(codec, ES8156_LRCLK_DIV_H_REG03, priv->suspend_reg_03);
	snd_soc_component_write(codec, ES8156_LRCLK_DIV_L_REG04, priv->suspend_reg_04);
	snd_soc_component_write(codec, ES8156_SCLK_DIV_REG05, priv->suspend_reg_05);
	snd_soc_component_write(codec, ES8156_NFS_CONFIG_REG06, priv->suspend_reg_06);
	snd_soc_component_write(codec, ES8156_MISC_CONTROL1_REG07, priv->suspend_reg_07);
	snd_soc_component_write(codec, ES8156_CLOCK_ON_OFF_REG08, priv->suspend_reg_08);
	snd_soc_component_write(codec, ES8156_MISC_CONTROL2_REG09, priv->suspend_reg_09);
	snd_soc_component_write(codec, ES8156_TIME_CONTROL1_REG0A, priv->suspend_reg_0A);
	snd_soc_component_write(codec, ES8156_TIME_CONTROL2_REG0B, priv->suspend_reg_0B);
	snd_soc_component_write(codec, ES8156_CHIP_STATUS_REG0C, priv->suspend_reg_0C);
	snd_soc_component_write(codec, ES8156_P2S_CONTROL_REG0D, priv->suspend_reg_0D);
	snd_soc_component_write(codec, ES8156_DAC_OSR_COUNTER_REG10, priv->suspend_reg_10);
	snd_soc_component_write(codec, ES8156_DAC_SDP_REG11, priv->suspend_reg_11);
	snd_soc_component_write(codec, ES8156_AUTOMUTE_SET_REG12, priv->suspend_reg_12);
	snd_soc_component_write(codec, ES8156_DAC_MUTE_REG13, priv->suspend_reg_13);
	snd_soc_component_write(codec, ES8156_VOLUME_CONTROL_REG14, priv->suspend_reg_14);
	snd_soc_component_write(codec, ES8156_ALC_CONFIG1_REG15, priv->suspend_reg_15);
	snd_soc_component_write(codec, ES8156_ALC_CONFIG2_REG16, priv->suspend_reg_16);
	snd_soc_component_write(codec, ES8156_ALC_CONFIG3_REG17, priv->suspend_reg_17);
	snd_soc_component_write(codec, ES8156_MISC_CONTROL3_REG18, priv->suspend_reg_18);
	snd_soc_component_write(codec, ES8156_EQ_CONTROL1_REG19, priv->suspend_reg_19);
	snd_soc_component_write(codec, ES8156_EQ_CONTROL2_REG1A, priv->suspend_reg_1A);
	snd_soc_component_write(codec, ES8156_ANALOG_SYS1_REG20, priv->suspend_reg_20);
	snd_soc_component_write(codec, ES8156_ANALOG_SYS2_REG21, priv->suspend_reg_21);
	snd_soc_component_write(codec, ES8156_ANALOG_SYS3_REG22, priv->suspend_reg_22);
	snd_soc_component_write(codec, ES8156_ANALOG_SYS4_REG23, priv->suspend_reg_23);
	snd_soc_component_write(codec, ES8156_ANALOG_LP_REG24, priv->suspend_reg_24);
	snd_soc_component_write(codec, ES8156_ANALOG_SYS5_REG25, priv->suspend_reg_25);

	return 0;
}
#endif

#ifdef HP_DET_FUNTION
static irqreturn_t es8156_irq_handler(int irq, void *data)
{
	struct es8156_priv *es8156 = data;

	queue_delayed_work(system_power_efficient_wq, &es8156->work,
			   msecs_to_jiffies(es8156->debounce_time));

	return IRQ_HANDLED;
}
#endif

/*
 * Call from rk_headset_irq_hook_adc.c
 *
 * Enable micbias for HOOK detection and disable external Amplifier
 * when jack insertion.
 */
int es8156_headset_detect(int jack_insert)
{
	struct es8156_priv *es8156;

	if (!es8156_codec)
		return -1;

	es8156 = snd_soc_component_get_drvdata(es8156_codec);

	es8156->hp_inserted = jack_insert;

	/*enable micbias and disable PA*/
	if (jack_insert) {
		es8156_enable_spk(es8156, false);
	}

	return 0;
}
EXPORT_SYMBOL(es8156_headset_detect);

#ifdef HP_DET_FUNTION
static void hp_work(struct work_struct *work)
{
	struct es8156_priv *es8156;
	int enable;

	es8156 = container_of(work, struct es8156_priv, work.work);
	enable = gpio_get_value(es8156->hp_det_gpio);
	if (es8156->hp_det_invert)
		enable = !enable;

	es8156->hp_inserted = enable ? true : false;
	if (!es8156->muted) {
		if (es8156->hp_inserted)
			es8156_enable_spk(es8156, false);
		else
			es8156_enable_spk(es8156, true);
	}
}
#endif

static int es8156_probe(struct snd_soc_component *codec)
{
	struct es8156_priv *es8156 = snd_soc_component_get_drvdata(codec);
	int ret = 0;

	es8156_codec = codec;

	/* power up the controller */
	if (es8156->avdd)
		ret |= regulator_enable(es8156->avdd);
	if (es8156->dvdd)
		ret |= regulator_enable(es8156->dvdd);
	if (es8156->pvdd)
		ret |= regulator_enable(es8156->pvdd);
	if (ret) {
		pr_err("Failed to enable VDD regulator: %d\n", ret);
		return ret;
	}

#if MCLK
	es8156->mclk = devm_clk_get(codec->dev, "mclk");
	if (PTR_ERR(es8156->mclk) == -EPROBE_DEFER)
		return -EPROBE_DEFER;
	ret = clk_prepare_enable(es8156->mclk);
#endif
	es8156_reset(codec);
	es8156_init_sequence(codec);
	return ret;
}

static void es8156_remove(struct snd_soc_component *codec)
{
	struct es8156_priv *es8156 = snd_soc_component_get_drvdata(codec);

	es8156_set_bias_level(codec, SND_SOC_BIAS_OFF);
	/* power down the controller */
	if (es8156->pvdd)
		regulator_disable(es8156->pvdd);
	if (es8156->dvdd)
		regulator_disable(es8156->dvdd);
	if (es8156->avdd)
		regulator_disable(es8156->avdd);
}

const struct regmap_config es8156_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= 0xff,
	.cache_type	= REGCACHE_RBTREE,
	.reg_defaults = es8156_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(es8156_reg_defaults),
};

static struct snd_soc_component_driver soc_codec_dev_es8156 = {
	.name = "es8156",
	.probe =	es8156_probe,
	.remove =	es8156_remove,
#ifdef CONFIG_PM_SLEEP
	.suspend =	es8156_suspend,
	.resume =	es8156_resume,
#endif
	.set_bias_level = es8156_set_bias_level,
	.controls = es8156_snd_controls,
	.num_controls = ARRAY_SIZE(es8156_snd_controls),
	.dapm_widgets = es8156_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(es8156_dapm_widgets),
	.dapm_routes = es8156_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(es8156_dapm_routes),
};

static ssize_t es8156_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        return 0;
}

static ssize_t es8156_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;

	for (i = 0; i < 0x26; i++) {
		printk("reg 0x%x = 0x%x\n", i, snd_soc_component_read(es8156_codec, i));
	}

	return 0;
}

static DEVICE_ATTR(registers, 0644, es8156_show, es8156_store);

static struct attribute *es8156_debug_attrs[] = {
	&dev_attr_registers.attr,
	NULL,
};

static struct attribute_group es8516_debug_attr_group = {
	.name = "es8156_debug",
	.attrs = es8156_debug_attrs,
};

/*
*es8156 7bit i2c address:CE pin:0 0x08 / CE pin:1 0x09
*/
static int es8156_i2c_probe(struct i2c_client *i2c)
{
	struct es8156_priv *es8156;
	int ret = -1;
	struct device_node *np = i2c->dev.of_node;
#ifdef HP_DET_FUNTION
	int hp_irq;
	enum of_gpio_flags flags;
#endif
	es8156 = devm_kzalloc(&i2c->dev, sizeof(*es8156), GFP_KERNEL);
	if (!es8156)
		return -ENOMEM;

	es8156->debounce_time = 200;
	es8156->hp_det_invert = 0;
	es8156->pwr_count = 0;
	es8156->hp_inserted = false;
	es8156->muted = true;

	es8156->regmap = devm_regmap_init_i2c(i2c, &es8156_regmap_config);
	if (IS_ERR(es8156->regmap)) {
		ret = PTR_ERR(es8156->regmap);
		dev_err(&i2c->dev, "Failed to init regmap: %d\n", ret);
		return ret;
	}

	es8156->avdd = devm_regulator_get(&i2c->dev, "AVDD");
	if (IS_ERR(es8156->avdd)) {
		ret = PTR_ERR(es8156->avdd);
		dev_warn(&i2c->dev, "Failed to get AVDD regulator: %d\n", ret);
		es8156->avdd = NULL;
	}
	es8156->dvdd = devm_regulator_get(&i2c->dev, "DVDD");
	if (IS_ERR(es8156->dvdd)) {
		ret = PTR_ERR(es8156->dvdd);
		dev_warn(&i2c->dev, "Failed to get DVDD regulator: %d\n", ret);
		es8156->dvdd = NULL;
	}
	es8156->pvdd = devm_regulator_get(&i2c->dev, "PVDD");
	if (IS_ERR(es8156->pvdd)) {
		ret = PTR_ERR(es8156->pvdd);
		dev_warn(&i2c->dev, "Failed to get PVDD regulator: %d\n", ret);
		es8156->pvdd = NULL;
	}

	if (of_property_read_u32(np, "mclk-sclk-ratio", &es8156->mclk_sclk_ratio) != 0) {
		es8156->mclk_sclk_ratio = 1;
	}

	Ratio *= es8156->mclk_sclk_ratio;

   	if (es8156->mclk_sclk_ratio == 1) {
		MCLK_SOURCE = SCLK_PIN;
	} else {
		MCLK_SOURCE = MCLK_PIN;
	}

	i2c_set_clientdata(i2c, es8156);
#ifdef HP_DET_FUNTION
	es8156->spk_ctl_gpio = of_get_named_gpio_flags(np,
						       "spk-con-gpio",
						       0,
						       &flags);
	if (es8156->spk_ctl_gpio < 0) {
		dev_info(&i2c->dev, "Can not read property spk_ctl_gpio\n");
		es8156->spk_ctl_gpio = INVALID_GPIO;
	} else {
		es8156->spk_active_level = !(flags & OF_GPIO_ACTIVE_LOW);
		ret = devm_gpio_request_one(&i2c->dev, es8156->spk_ctl_gpio,
					    GPIOF_DIR_OUT, NULL);
		if (ret) {
			dev_err(&i2c->dev, "Failed to request spk_ctl_gpio\n");
			return ret;
		}
		es8156_enable_spk(es8156, false);
	}

	es8156->hp_det_gpio = of_get_named_gpio_flags(np,
						      "hp-det-gpio",
						      0,
						      &flags);
	if (es8156->hp_det_gpio < 0) {
		dev_info(&i2c->dev, "Can not read property hp_det_gpio\n");
		es8156->hp_det_gpio = INVALID_GPIO;
	} else {
		INIT_DELAYED_WORK(&es8156->work, hp_work);
		es8156->hp_det_invert = !!(flags & OF_GPIO_ACTIVE_LOW);
		ret = devm_gpio_request_one(&i2c->dev, es8156->hp_det_gpio,
					    GPIOF_IN, "hp det");
		if (ret < 0)
			return ret;
		hp_irq = gpio_to_irq(es8156->hp_det_gpio);
		ret = devm_request_threaded_irq(&i2c->dev, hp_irq, NULL,
						es8156_irq_handler,
						IRQF_TRIGGER_FALLING |
						IRQF_TRIGGER_RISING |
						IRQF_ONESHOT,
						"es8156_interrupt", es8156);
		if (ret < 0) {
			dev_err(&i2c->dev, "request_irq failed: %d\n", ret);
			return ret;
		}

		schedule_delayed_work(&es8156->work,
				      msecs_to_jiffies(es8156->debounce_time));
	}
#endif
	ret = snd_soc_register_component(&i2c->dev,
				     &soc_codec_dev_es8156,
				     &es8156_dai, 1);

	ret = sysfs_create_group(&i2c->dev.kobj, &es8516_debug_attr_group);
	if (ret) {
			pr_err("failed to create attr group\n");
	}

	return ret;
}

static  void es8156_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_component(&client->dev);
	return ;
}

static void es8156_i2c_shutdown(struct i2c_client *client)
{
	struct es8156_priv *es8156 = i2c_get_clientdata(client);

	if (es8156_codec != NULL) {
		es8156_enable_spk(es8156, false);
		msleep(20);
		es8156_set_bias_level(es8156_codec, SND_SOC_BIAS_OFF);
	}
}

static const struct i2c_device_id es8156_i2c_id[] = {
	{"es8156", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, es8156_i2c_id);


static const struct of_device_id es8156_of_match[] = {
	{ .compatible = "everest,es8156", },
	{ }
};
MODULE_DEVICE_TABLE(of, es8156_of_match);

static struct i2c_driver es8156_i2c_driver = {
	.driver = {
		.name		= "es8156",
		.of_match_table = es8156_of_match,
	},
	.probe    = es8156_i2c_probe,
	.remove   = es8156_i2c_remove,
	.shutdown = es8156_i2c_shutdown,
	.id_table = es8156_i2c_id,
};
module_i2c_driver(es8156_i2c_driver);

MODULE_DESCRIPTION("ASoC es8156 driver");
MODULE_LICENSE("GPL");

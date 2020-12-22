/*
 * sound\soc\codec\ac100.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/io.h>
#include <mach/sys_config.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/switch.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/arisc/arisc.h>
#include <linux/power/scenelock.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>
#include <mach/gpio.h>
#include "ac100.h"

#ifndef CONFIG_SUNXI_SWITCH_GPIO
static volatile int reset_flag = 0;
static int hook_flag1 = 0;
static int hook_flag2 = 0;
static int KEY_VOLUME_FLAG = 0;
/*1=headphone in slot, else 0*/
static int headphone_state = 0;
static volatile int irq_flag = 0;
static script_item_u item_eint;
static struct workqueue_struct *switch_detect_queue;
static struct workqueue_struct *codec_irq_queue;
/* key define */
#define KEY_HEADSETHOOK         226
#define HEADSET_CHECKCOUNT  (10)
#define HEADSET_CHECKCOUNT_SUM  (2)
#endif
struct spk_gpio spkgpio;
static int speaker_double_used 	= 0;
static int double_speaker_val 	= 0;
static int single_speaker_val 	= 0;
static int headset_val 		= 0;
static int earpiece_val 	= 0;
static int mainmic_val 		= 0;
static int headsetmic_val 	= 0;
static int dmic_used 		= 0;
static int adc_digital_val 	= 0;
static int agc_used 		= 0;
static int drc_used 		= 0;
static int aif2_lrck_div 	= 0;
static int aif2_bclk_div 	= 0;

#define ac100_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define ac100_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static script_item_u item;
enum headphone_mode_u {
	HEADPHONE_IDLE,
	FOUR_HEADPHONE_PLUGIN,
	THREE_HEADPHONE_PLUGIN,
};
/*supply voltage*/
static const char *ac100_supplies[] = {
	"vcc-avcc",
	"vcc-io1",
	"vcc-io2",
	"vcc-ldoin",
	"vcc-cpvdd",
};

/*struct for ac100*/
struct ac100_priv {
	struct ac100 *ac100;
	struct snd_soc_codec *codec;

	struct mutex dac_mutex;
	struct mutex adc_mutex;
	struct mutex mute_mutex;
	u8 dac_enable;
	u8 adc_enable;
	struct mutex aifclk_mutex;
	u8 aif1_clken;
	u8 aif2_clken;
	u8 aif3_clken;

	u8 aif2_mute;
	u8 aif1_mute;

	/*voltage supply*/
	int num_supplies;
	struct regulator_bulk_data *supplies;

	/*headset*/
	int virq; /*headset irq*/
	struct switch_dev sdev;
	int state;
	int check_count;
	int check_count_sum;
	int reset_flag;

	enum headphone_mode_u mode;
	struct work_struct work;
	struct work_struct clear_codec_irq;
	struct work_struct codec_resume;
	struct work_struct state_work;

	struct semaphore sem;

	struct timer_list timer;

	struct input_dev *key;
};

static void get_configuration(void)
{
	script_item_value_type_e  type;
	script_item_u val;
	type = script_get_item("ac100_audio0", "speaker_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] speaker_double_used type err!\n");
	}
	speaker_double_used = val.val;

	type = script_get_item("ac100_audio0", "double_speaker_val", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] double_speaker_val type err!\n");
	}
	double_speaker_val = val.val;

	type = script_get_item("ac100_audio0", "single_speaker_val", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] single_speaker_val type err!\n");
	}
	single_speaker_val = val.val;

	type = script_get_item("ac100_audio0", "headset_val", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] headset_val type err!\n");
	}
	headset_val = val.val;

	type = script_get_item("ac100_audio0", "earpiece_val", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] headset_val type err!\n");
	}
	earpiece_val = val.val;

	type = script_get_item("ac100_audio0", "mainmic_val", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] headset_val type err!\n");
	}
	mainmic_val = val.val;

	type = script_get_item("ac100_audio0", "headsetmic_val", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] headset_val type err!\n");
	}
	headsetmic_val = val.val;

	type = script_get_item("ac100_audio0", "dmic_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] dmic_used type err!\n");
	}
	dmic_used = val.val;
	if (dmic_used) {
		type = script_get_item("ac100_audio0", "adc_digital_val", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[CODEC] adc_digital_val type err!\n");
		}

		adc_digital_val = val.val;
	}
	type = script_get_item("ac100_audio0", "agc_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] agc_used type err!\n");
	} else {
		agc_used = val.val;
	}
	type = script_get_item("ac100_audio0", "drc_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] drc_used type err!\n");
	} else {
		drc_used = val.val;
	}

}
static void agc_config(struct snd_soc_codec *codec)
{
	int reg_val;

	reg_val = snd_soc_read(codec, 0xb4);
	reg_val |= (0x3<<6);
	snd_soc_write(codec, 0xb4, reg_val);

	reg_val = snd_soc_read(codec, 0x84);
	reg_val &= ~(0x3f<<8);
	reg_val |= (0x31<<8);
	snd_soc_write(codec, 0x84, reg_val);

	reg_val = snd_soc_read(codec, 0x84);
	reg_val &= ~(0xff<<0);
	reg_val |= (0x28<<0);
	snd_soc_write(codec, 0x84, reg_val);

	reg_val = snd_soc_read(codec, 0x85);
	reg_val &= ~(0x3f<<8);
	reg_val |= (0x31<<8);
	snd_soc_write(codec, 0x85, reg_val);

	reg_val = snd_soc_read(codec, 0x85);
	reg_val &= ~(0xff<<0);
	reg_val |= (0x28<<0);
	snd_soc_write(codec, 0x85, reg_val);

	reg_val = snd_soc_read(codec, 0x8a);
	reg_val &= ~(0x7fff<<0);
	reg_val |= (0x24<<0);
	snd_soc_write(codec, 0x8a, reg_val);

	reg_val = snd_soc_read(codec, 0x8b);
	reg_val &= ~(0x7fff<<0);
	reg_val |= (0x2<<0);
	snd_soc_write(codec, 0x8b, reg_val);

	reg_val = snd_soc_read(codec, 0x8c);
	reg_val &= ~(0x7fff<<0);
	reg_val |= (0x24<<0);
	snd_soc_write(codec, 0x8c, reg_val);

	reg_val = snd_soc_read(codec, 0x8d);
	reg_val &= ~(0x7fff<<0);
	reg_val |= (0x2<<0);
	snd_soc_write(codec, 0x8d, reg_val);

	reg_val = snd_soc_read(codec, 0x8e);
	reg_val &= ~(0x1f<<8);
	reg_val |= (0xf<<8);
	reg_val &= ~(0x1f<<0);
	reg_val |= (0xf<<0);
	snd_soc_write(codec, 0x8e, reg_val);

	reg_val = snd_soc_read(codec, 0x93);
	reg_val &= ~(0x7ff<<0);
	reg_val |= (0xfc<<0);
	snd_soc_write(codec, 0x93, reg_val);
	snd_soc_write(codec, 0x94, 0xabb3);
}

static void drc_config(struct snd_soc_codec *codec)
{
	int reg_val;
	reg_val = snd_soc_read(codec, 0xa3);
	reg_val &= ~(0x7ff<<0);
	reg_val |= 1<<0;
	snd_soc_write(codec, 0xa3, reg_val);
	snd_soc_write(codec, 0xa4, 0x2baf);

	reg_val = snd_soc_read(codec, 0xa5);
	reg_val &= ~(0x7ff<<0);
	reg_val |= 1<<0;
	snd_soc_write(codec, 0xa5, reg_val);
	snd_soc_write(codec, 0xa6, 0x2baf);

	reg_val = snd_soc_read(codec, 0xa7);
	reg_val &= ~(0x7ff<<0);
	snd_soc_write(codec, 0xa7, reg_val);
	snd_soc_write(codec, 0xa8, 0x44a);

	reg_val = snd_soc_read(codec, 0xa9);
	reg_val &= ~(0x7ff<<0);
	snd_soc_write(codec, 0xa9, reg_val);
	snd_soc_write(codec, 0xaa, 0x1e06);

	reg_val = snd_soc_read(codec, 0xab);
	reg_val &= ~(0x7ff<<0);
	reg_val |= (0x352<<0);
	snd_soc_write(codec, 0xab, reg_val);
	snd_soc_write(codec, 0xac, 0x6910);

	reg_val = snd_soc_read(codec, 0xad);
	reg_val &= ~(0x7ff<<0);
	reg_val |= (0x77a<<0);
	snd_soc_write(codec, 0xad, reg_val);
	snd_soc_write(codec, 0xae, 0xaaaa);

	reg_val = snd_soc_read(codec, 0xaf);
	reg_val &= ~(0x7ff<<0);
	reg_val |= (0x2de<<0);
	snd_soc_write(codec, 0xaf, reg_val);
	snd_soc_write(codec, 0xb0, 0xc982);
	snd_soc_write(codec, 0x16, 0x9f9f);
}
static void agc_enable(struct snd_soc_codec *codec,bool on)
{
	int reg_val;
	if (on) {
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val |= (0x1<<7);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val |= (0x1<<7);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		reg_val = snd_soc_read(codec, 0x82);
		reg_val &= ~(0xf<<0);
		reg_val |= (0x6<<0);

		reg_val &= ~(0x7<<12);
		reg_val |= (0x7<<12);
		snd_soc_write(codec, 0x82, reg_val);

		reg_val = snd_soc_read(codec, 0x83);
		reg_val &= ~(0xf<<0);
		reg_val |= (0x6<<0);

		reg_val &= ~(0x7<<12);
		reg_val |= (0x7<<12);
		snd_soc_write(codec, 0x83, reg_val);
	} else {
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val &= ~(0x1<<7);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val &= ~(0x1<<7);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		reg_val = snd_soc_read(codec, 0x82);
		reg_val &= ~(0xf<<0);
		reg_val &= ~(0x7<<12);
		snd_soc_write(codec, 0x82, reg_val);

		reg_val = snd_soc_read(codec, 0x83);
		reg_val &= ~(0xf<<0);
		reg_val &= ~(0x7<<12);
		snd_soc_write(codec, 0x83, reg_val);
	}
}
static void drc_enable(struct snd_soc_codec *codec,bool on)
{
	int reg_val;
	if (on) {
		snd_soc_write(codec, 0xb5, 0x80);
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val |= (0x1<<6);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val |= (0x1<<6);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		reg_val = snd_soc_read(codec, 0xa0);
		reg_val |= (0x7<<0);
		snd_soc_write(codec, 0xa0, reg_val);
	} else {
		snd_soc_write(codec, 0xb5, 0x0);
		reg_val = snd_soc_read(codec, MOD_CLK_ENA);
		reg_val &= ~(0x1<<6);
		snd_soc_write(codec, MOD_CLK_ENA, reg_val);
		reg_val = snd_soc_read(codec, MOD_RST_CTRL);
		reg_val &= ~(0x1<<6);
		snd_soc_write(codec, MOD_RST_CTRL, reg_val);

		reg_val = snd_soc_read(codec, 0xa0);
		reg_val &= ~(0x7<<0);
		snd_soc_write(codec, 0xa0, reg_val);
	}
}
static void set_configuration(struct snd_soc_codec *codec)
{
	if (speaker_double_used) {
		snd_soc_update_bits(codec, SPKOUT_CTRL, (0x1f<<SPK_VOL), (double_speaker_val<<SPK_VOL));
	} else {
		snd_soc_update_bits(codec, SPKOUT_CTRL, (0x1f<<SPK_VOL), (single_speaker_val<<SPK_VOL));
	}
	snd_soc_update_bits(codec, HPOUT_CTRL, (0x3f<<HP_VOL), (headset_val<<HP_VOL));
	snd_soc_update_bits(codec, ESPKOUT_CTRL, (0x1f<<ESP_VOL), (earpiece_val<<ESP_VOL));
	snd_soc_update_bits(codec, ADC_SRCBST_CTRL, (0x7<<ADC_MIC1G), (mainmic_val<<ADC_MIC1G));
	snd_soc_update_bits(codec, ADC_SRCBST_CTRL, (0x7<<ADC_MIC2G), (headsetmic_val<<ADC_MIC2G));
	if (dmic_used) {
		snd_soc_write(codec, ADC_VOL_CTRL, adc_digital_val);
	}
	if (agc_used) {
		agc_config(codec);
	}
	if (drc_used) {
		drc_config(codec);
	}
	/*headphone calibration clock frequency select*/
	snd_soc_update_bits(codec, SPKOUT_CTRL, (0x7<<HPCALICKS), (0x7<<HPCALICKS));

	#ifdef CONFIG_SND_SUNXI_SOC_DAUDIO0_KARAOKE_MACHINE
	snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<ADCLEN), (0x1<<ADCLEN));
	snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<ADCREN), (0x1<<ADCREN));
	snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENAD), (0x1<<ENAD));
	snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF2CLK_ENA), (0x1<<AIF2CLK_ENA));
	snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF1CLK_ENA), (0x1<<AIF1CLK_ENA));
	snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_ADC_DIG), (0x1<<MOD_CLK_ADC_DIG));
	snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_ADC_DIG), (0x1<<MOD_RESET_ADC_DIG));
	#endif

}
static int late_enable_dac(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&ac100->dac_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		AC100_DBG("%s,line:%d\n",__func__,__LINE__);
		if (ac100->dac_enable == 0){
			/*enable dac module clk*/
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_DAC_DIG), (0x1<<MOD_CLK_DAC_DIG));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_DAC_DIG), (0x1<<MOD_RESET_DAC_DIG));
			snd_soc_update_bits(codec, DAC_DIG_CTRL, (0x1<<ENDA), (0x1<<ENDA));
		}
		ac100->dac_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac100->dac_enable > 0){
			ac100->dac_enable--;
			if (ac100->dac_enable == 0){
				snd_soc_update_bits(codec, DAC_DIG_CTRL, (0x1<<ENDA), (0x0<<ENDA));
				/*disable dac module clk*/
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_DAC_DIG), (0x0<<MOD_CLK_DAC_DIG));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_DAC_DIG), (0x0<<MOD_RESET_DAC_DIG));
			}
		}
		break;
	}
	mutex_unlock(&ac100->dac_mutex);
	return 0;
}

static int late_enable_adc(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&ac100->adc_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (ac100->adc_enable == 0){
			/*enable adc module clk*/
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_ADC_DIG), (0x1<<MOD_CLK_ADC_DIG));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_ADC_DIG), (0x1<<MOD_RESET_ADC_DIG));
			snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENAD), (0x1<<ENAD));
		}
		ac100->adc_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac100->adc_enable > 0){
			ac100->adc_enable--;
			if (ac100->adc_enable == 0){
				#ifndef CONFIG_SND_SUNXI_SOC_DAUDIO0_KARAOKE_MACHINE
				snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENAD), (0x0<<ENAD));
				/*disable adc module clk*/
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_ADC_DIG), (0x0<<MOD_CLK_ADC_DIG));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_ADC_DIG), (0x0<<MOD_RESET_ADC_DIG));
				#endif
			}
		}
		break;
	}
	mutex_unlock(&ac100->adc_mutex);
	return 0;
}
static int ac100_speaker_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	struct snd_soc_codec *codec = w->codec;
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		AC100_DBG("[speaker open ]%s,line:%d\n",__func__,__LINE__);
		if (drc_used) {
			drc_enable(codec,1);
		}
		msleep(30);
		if (spkgpio.used)
			gpio_set_value(spkgpio.gpio, 1);
		break;
	case SND_SOC_DAPM_PRE_PMD :
		AC100_DBG("[speaker close ]%s,line:%d\n",__func__,__LINE__);
		if (spkgpio.used)
			gpio_set_value(spkgpio.gpio, 0);
		if (drc_used) {
			drc_enable(codec,0);
		}
	default:
		break;

	}
	return 0;
}
static int ac100_earpiece_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	struct snd_soc_codec *codec = w->codec;
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		AC100_DBG("[earpiece open ]%s,line:%d\n",__func__,__LINE__);
		snd_soc_update_bits(codec, ESPKOUT_CTRL, (0x1<<ESPPA_EN), (0x1<<ESPPA_EN));
		break;
	case SND_SOC_DAPM_PRE_PMD :
		AC100_DBG("[earpiece close ]%s,line:%d\n",__func__,__LINE__);
		snd_soc_update_bits(codec, ESPKOUT_CTRL, (0x1<<ESPPA_EN), (0x0<<ESPPA_EN));
	default:
		break;

	}
	return 0;
}
static int ac100_headphone_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k,	int event)
{
	struct snd_soc_codec *codec = w->codec;
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/*open*/
		AC100_DBG("post:open:%s,line:%d\n", __func__, __LINE__);
		snd_soc_update_bits(codec, OMIXER_DACA_CTRL, (0xf<<HPOUTPUTENABLE), (0xf<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, HPOUT_CTRL, (0x1<<HPPA_EN), (0x1<<HPPA_EN));
		msleep(10);
		snd_soc_update_bits(codec, HPOUT_CTRL, (0x3<<LHPPA_MUTE), (0x3<<LHPPA_MUTE));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/*close*/
		AC100_DBG("pre:close:%s,line:%d\n", __func__, __LINE__);
		snd_soc_update_bits(codec, HPOUT_CTRL, (0x1<<HPPA_EN), (0x0<<HPPA_EN));
		snd_soc_update_bits(codec, OMIXER_DACA_CTRL, (0xf<<HPOUTPUTENABLE), (0x0<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, HPOUT_CTRL, (0x3<<LHPPA_MUTE), (0x0<<LHPPA_MUTE));
		break;
	}
	return 0;
}

int ac100_aif1clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&ac100->aifclk_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (ac100->aif1_clken == 0) {
			/*enable AIF1CLK*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF1CLK_ENA), (0x1<<AIF1CLK_ENA));
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF1), (0x1<<MOD_CLK_AIF1));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF1), (0x1<<MOD_RESET_AIF1));

			/*enable systemclk*/
			if (ac100->aif2_clken == 0 && ac100->aif3_clken == 0)
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
		}
		ac100->aif1_clken++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac100->aif1_clken > 0){
			ac100->aif1_clken--;
			if (ac100->aif1_clken == 0){
				/*disable AIF1CLK*/
				#ifndef CONFIG_SND_SUNXI_SOC_DAUDIO0_KARAOKE_MACHINE
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF1CLK_ENA), (0x0<<AIF1CLK_ENA));
				#endif
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF1), (0x0<<MOD_CLK_AIF1));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF1), (0x0<<MOD_RESET_AIF1));
				/*DISABLE systemclk*/
				if (ac100->aif2_clken == 0 && ac100->aif3_clken == 0)
					snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
			}
		}
		break;
	}
	mutex_unlock(&ac100->aifclk_mutex);
	return 0;
}

int ac100_aif2clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&ac100->aifclk_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (ac100->aif2_clken == 0){
			/*enable AIF2CLK*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF2CLK_ENA), (0x1<<AIF2CLK_ENA));
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF2), (0x1<<MOD_CLK_AIF2));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF2), (0x1<<MOD_RESET_AIF2));
			/*enable systemclk*/
			if (ac100->aif1_clken == 0 && ac100->aif3_clken == 0)
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
		}
		ac100->aif2_clken++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac100->aif2_clken > 0){
			ac100->aif2_clken--;
			if (ac100->aif2_clken == 0){
				/*disable AIF2CLK*/
				#ifndef CONFIG_SND_SUNXI_SOC_DAUDIO0_KARAOKE_MACHINE
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF2CLK_ENA), (0x0<<AIF2CLK_ENA));
				#endif
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF2), (0x0<<MOD_CLK_AIF2));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF2), (0x0<<MOD_RESET_AIF2));
				/*DISABLE systemclk*/
				if (ac100->aif1_clken == 0 && ac100->aif3_clken == 0)
					snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
			}
		}
		break;
	}
	mutex_unlock(&ac100->aifclk_mutex);
	return 0;
}

int ac100_aif3clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&ac100->aifclk_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (ac100->aif2_clken == 0){
			/*enable AIF2CLK*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF2CLK_ENA), (0x1<<AIF2CLK_ENA));
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF2), (0x1<<MOD_CLK_AIF2));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF2), (0x1<<MOD_RESET_AIF2));
			/*enable systemclk*/
			if (ac100->aif1_clken == 0 && ac100->aif3_clken == 0)
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
		}
		ac100->aif2_clken++;
		if (ac100->aif3_clken == 0){
			/*enable AIF3CLK*/
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF3), (0x1<<MOD_CLK_AIF3));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF3), (0x1<<MOD_RESET_AIF3));
		}
		ac100->aif3_clken++;

		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac100->aif2_clken > 0){
			ac100->aif2_clken--;
			if (ac100->aif2_clken == 0){
				/*disable AIF2CLK*/
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<AIF2CLK_ENA), (0x0<<AIF2CLK_ENA));
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF2), (0x0<<MOD_CLK_AIF2));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF2), (0x0<<MOD_RESET_AIF2));
				/*DISABLE systemclk*/
				if (ac100->aif1_clken == 0 && ac100->aif3_clken == 0)
					snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
			}
		}
		if (ac100->aif3_clken > 0){
			ac100->aif3_clken--;
			if (ac100->aif3_clken == 0){
			/*enable AIF3CLK*/
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_AIF3), (0x0<<MOD_CLK_AIF3));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_AIF3), (0x0<<MOD_RESET_AIF3));
			}
		}

		break;
	}
	mutex_unlock(&ac100->aifclk_mutex);
	return 0;
}

static int aif2inl_vir_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_update_bits(codec, AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x1<<AIF2_DAC_SRC));
		break;
	case SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x0<<AIF2_DAC_SRC));
		break;
	}
	return 0;
}
static int aif2inr_vir_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_update_bits(codec, AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x2<<AIF2_DAC_SRC));
		break;
	case SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x0<<AIF2_DAC_SRC));
		break;
	}
	return 0;
}
static int dmic_mux_ev(struct snd_soc_dapm_widget *w,
		      struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	switch (event){
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENDM), (0x1<<ENDM));
		break;
	case SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENDM), (0x0<<ENDM));
		break;
	}
	mutex_lock(&ac100->adc_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (ac100->adc_enable == 0){
			/*enable adc module clk*/
			snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_ADC_DIG), (0x1<<MOD_CLK_ADC_DIG));
			snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_ADC_DIG), (0x1<<MOD_RESET_ADC_DIG));
			snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENAD), (0x1<<ENAD));
		}
		ac100->adc_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (ac100->adc_enable > 0){
			ac100->adc_enable--;
			if (ac100->adc_enable == 0){
				snd_soc_update_bits(codec, ADC_DIG_CTRL, (0x1<<ENAD), (0x0<<ENAD));
				/*disable adc module clk*/
				snd_soc_update_bits(codec, MOD_CLK_ENA, (0x1<<MOD_CLK_ADC_DIG), (0x0<<MOD_CLK_ADC_DIG));
				snd_soc_update_bits(codec, MOD_RST_CTRL, (0x1<<MOD_RESET_ADC_DIG), (0x0<<MOD_RESET_ADC_DIG));
			}
		}
		break;
	}
	mutex_unlock(&ac100->adc_mutex);
	return 0;
}

static const DECLARE_TLV_DB_SCALE(headphone_vol_tlv, -6300, 100, 0);
static const DECLARE_TLV_DB_SCALE(lineout_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(speaker_vol_tlv, -4800, 150, 0);
static const DECLARE_TLV_DB_SCALE(earpiece_vol_tlv, -4350, 150, 0);

static const DECLARE_TLV_DB_SCALE(aif1_ad_slot0_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot1_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_da_slot0_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_da_slot1_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot0_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot1_mix_vol_tlv, -600, 600, 0);

static const DECLARE_TLV_DB_SCALE(aif2_ad_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif2_da_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif2_ad_mix_vol_tlv, -600, 600, 0);

static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);

static const DECLARE_TLV_DB_SCALE(dig_vol_tlv, -7308, 116, 0);
static const DECLARE_TLV_DB_SCALE(dac_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(adc_input_vol_tlv, -450, 150, 0);

/*mic1/mic2: 0db when 000, and from 30db to 48db when 001 to 111*/
static const DECLARE_TLV_DB_SCALE(mic1_boost_vol_tlv, 0, 200, 0);
static const DECLARE_TLV_DB_SCALE(mic2_boost_vol_tlv, 0, 200, 0);

static const DECLARE_TLV_DB_SCALE(linein_amp_vol_tlv, -1200, 300, 0);
static const DECLARE_TLV_DB_SCALE(axui_amp_vol_tlv, -1200, 300, 0);

static const DECLARE_TLV_DB_SCALE(axin_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic1_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic2_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(linein_to_l_r_mix_vol_tlv, -450, 150, 0);

static const struct snd_kcontrol_new ac100_controls[] = {
	/*AIF1*/
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 0 volume", AIF1_VOL_CTRL1, AIF1_AD0L_VOL, AIF1_AD0R_VOL, 0xff, 0, aif1_ad_slot0_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 1 volume", AIF1_VOL_CTRL2, AIF1_AD1L_VOL, AIF1_AD1R_VOL, 0xff, 0, aif1_ad_slot1_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 DAC timeslot 0 volume", AIF1_VOL_CTRL3, AIF1_DA0L_VOL, AIF1_DA0R_VOL, 0xff, 0, aif1_da_slot0_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 DAC timeslot 1 volume", AIF1_VOL_CTRL4, AIF1_DA1L_VOL, AIF1_DA1R_VOL, 0xff, 0, aif1_da_slot1_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 0 mixer gain", AIF1_MXR_GAIN, AIF1_AD0L_MXR_GAIN, AIF1_AD0R_MXR_GAIN, 0xf, 0, aif1_ad_slot0_mix_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 1 mixer gain", AIF1_MXR_GAIN, AIF1_AD1L_MXR_GAIN, AIF1_AD1R_MXR_GAIN, 0x3, 0, aif1_ad_slot1_mix_vol_tlv),

	/*AIF2*/
	SOC_DOUBLE_TLV("AIF2 ADC volume", AIF2_VOL_CTRL1, AIF2_ADCL_VOL,AIF2_ADCR_VOL, 0xff, 0, aif2_ad_vol_tlv),
	SOC_DOUBLE_TLV("AIF2 DAC volume", AIF2_VOL_CTRL2, AIF2_DACL_VOL,AIF2_DACR_VOL, 0xff, 0, aif2_da_vol_tlv),
	SOC_DOUBLE_TLV("AIF2 ADC mixer gain", AIF2_MXR_GAIN, AIF2_ADCL_MXR_GAIN,AIF2_ADCR_MXR_GAIN, 0xf, 0, aif2_ad_mix_vol_tlv),

	/*ADC*/
	SOC_DOUBLE_TLV("ADC volume", ADC_VOL_CTRL, ADC_VOL_L, ADC_VOL_R, 0xff, 0, adc_vol_tlv),
	/*DAC*/
	SOC_DOUBLE_TLV("DAC volume", DAC_VOL_CTRL, DAC_VOL_L, DAC_VOL_R, 0xff, 0, dac_vol_tlv),
	SOC_DOUBLE_TLV("DAC mixer gain", DAC_MXR_GAIN, DACL_MXR_GAIN, DACR_MXR_GAIN, 0xf, 0, dac_mix_vol_tlv),

	SOC_SINGLE_TLV("digital volume", DAC_DBG_CTRL, DVC, 0x3f, 0, dig_vol_tlv),

	/*ADC*/
	SOC_DOUBLE_TLV("ADC input gain", ADC_APC_CTRL, ADCLG, ADCRG, 0x7, 0, adc_input_vol_tlv),

	SOC_SINGLE_TLV("MIC1 boost amplifier gain", ADC_SRCBST_CTRL, ADC_MIC1G, 0x7, 0, mic1_boost_vol_tlv),
	SOC_SINGLE_TLV("MIC2 boost amplifier gain", ADC_SRCBST_CTRL, ADC_MIC2G, 0x7, 0, mic2_boost_vol_tlv),
	SOC_SINGLE_TLV("LINEINL-LINEINR pre-amplifier gain", ADC_SRCBST_CTRL, LINEIN_PREG, 0x7, 0, linein_amp_vol_tlv),
	SOC_SINGLE_TLV("AUXI pre-amplifier gain", ADC_SRCBST_CTRL, AUXI_PREG, 0x7, 0, axui_amp_vol_tlv),

	SOC_SINGLE_TLV("AXin to L_R output mixer gain", OMIXER_BST1_CTRL, AXG, 0x7, 0, axin_to_l_r_mix_vol_tlv),
	SOC_SINGLE_TLV("MIC1 BST stage to L_R outp mixer gain", OMIXER_BST1_CTRL, OMIXER_MIC1G, 0x7, 0, mic1_to_l_r_mix_vol_tlv),
	SOC_SINGLE_TLV("MIC2 BST stage to L_R outp mixer gain", OMIXER_BST1_CTRL, OMIXER_MIC2G, 0x7, 0, mic2_to_l_r_mix_vol_tlv),
	SOC_SINGLE_TLV("LINEINL/R to L_R output mixer gain", OMIXER_BST1_CTRL, LINEING, 0x7, 0, linein_to_l_r_mix_vol_tlv),

	SOC_SINGLE_TLV("earpiece volume", ESPKOUT_CTRL, ESP_VOL, 0x1f, 0, earpiece_vol_tlv),
	SOC_SINGLE_TLV("speaker volume", SPKOUT_CTRL, SPK_VOL, 0x1f, 0, speaker_vol_tlv),
	SOC_SINGLE_TLV("line out volume", LOUT_CTRL, LINEOUTG, 0x7, 0, lineout_vol_tlv),
	SOC_SINGLE_TLV("headphone volume", HPOUT_CTRL, HP_VOL, 0x3f, 0, headphone_vol_tlv),
#ifdef CONFIG_SND_SUNXI_SOC_DAUDIO0_KARAOKE_MACHINE
	SOC_SINGLE("AIF1 ADCR Switch Duplicate", AIF1_MXR_SRC, AIF1_AD0R_ADCR_MXR, 0x1, 0),
	SOC_SINGLE("DACR ADCR Switch Duplicate", DAC_MXR_SRC,  	DACR_MXR_ADCR, 1, 0),
#endif

};
/*AIF1 AD0 OUT */
static const char *aif1out0l_text[] = {
	"AIF1_AD0L", "AIF1_AD0R","SUM_AIF1AD0L_AIF1AD0R", "AVE_AIF1AD0L_AIF1AD0R"
};
static const char *aif1out0r_text[] = {
	"AIF1_AD0R", "AIF1_AD0L","SUM_AIF1AD0L_AIF1AD0R", "AVE_AIF1AD0L_AIF1AD0R"
};

static const struct soc_enum aif1out0l_enum =
	SOC_ENUM_SINGLE(AIF1_ADCDAT_CTRL, 10, 4, aif1out0l_text);

static const struct snd_kcontrol_new aif1out0l_mux =
	SOC_DAPM_ENUM("AIF1OUT0L Mux", aif1out0l_enum);

static const struct soc_enum aif1out0r_enum =
	SOC_ENUM_SINGLE(AIF1_ADCDAT_CTRL, 8, 4, aif1out0r_text);

static const struct snd_kcontrol_new aif1out0r_mux =
	SOC_DAPM_ENUM("AIF1OUT0R Mux", aif1out0r_enum);

/*AIF1 AD1 OUT */
static const char *aif1out1l_text[] = {
	"AIF1_AD1L", "AIF1_AD1R","SUM_AIF1ADC1L_AIF1ADC1R", "AVE_AIF1ADC1L_AIF1ADC1R"
};
static const char *aif1out1r_text[] = {
	"AIF1_AD1R", "AIF1_AD1L","SUM_AIF1ADC1L_AIF1ADC1R", "AVE_AIF1ADC1L_AIF1ADC1R"
};

static const struct soc_enum aif1out1l_enum =
	SOC_ENUM_SINGLE(AIF1_ADCDAT_CTRL, 6, 4, aif1out1l_text);

static const struct snd_kcontrol_new aif1out1l_mux =
	SOC_DAPM_ENUM("AIF1OUT1L Mux", aif1out1l_enum);

static const struct soc_enum aif1out1r_enum =
	SOC_ENUM_SINGLE(AIF1_ADCDAT_CTRL, 4, 4, aif1out1r_text);

static const struct snd_kcontrol_new aif1out1r_mux =
	SOC_DAPM_ENUM("AIF1OUT1R Mux", aif1out1r_enum);

/*AIF1 DA0 IN*/
static const char *aif1in0l_text[] = {
	"AIF1_DA0L", "AIF1_DA0R", "SUM_AIF1DA0L_AIF1DA0R", "AVE_AIF1DA0L_AIF1DA0R"
};
static const char *aif1in0r_text[] = {
	"AIF1_DA0R", "AIF1_DA0L", "SUM_AIF1DA0L_AIF1DA0R", "AVE_AIF1DA0L_AIF1DA0R"
};

static const struct soc_enum aif1in0l_enum =
	SOC_ENUM_SINGLE(AIF1_DACDAT_CTRL, 10, 4, aif1in0l_text);

static const struct snd_kcontrol_new aif1in0l_mux =
	SOC_DAPM_ENUM("AIF1IN0L Mux", aif1in0l_enum);

static const struct soc_enum aif1in0r_enum =
	SOC_ENUM_SINGLE(AIF1_DACDAT_CTRL, 8, 4, aif1in0r_text);

static const struct snd_kcontrol_new aif1in0r_mux =
	SOC_DAPM_ENUM("AIF1IN0R Mux", aif1in0r_enum);

/*AIF1 DA1 IN*/
static const char *aif1in1l_text[] = {
	"AIF1_DA1L", "AIF1_DA1R","SUM_AIF1DA1L_AIF1DA1R", "AVE_AIF1DA1L_AIF1DA1R"
};
static const char *aif1in1r_text[] = {
	"AIF1_DA1R", "AIF1_DA1L","SUM_AIF1DA1L_AIF1DA1R", "AVE_AIF1DA1L_AIF1DA1R"
};

static const struct soc_enum aif1in1l_enum =
	SOC_ENUM_SINGLE(AIF1_DACDAT_CTRL, 6, 4, aif1in1l_text);

static const struct snd_kcontrol_new aif1in1l_mux =
	SOC_DAPM_ENUM("AIF1IN1L Mux", aif1in1l_enum);

static const struct soc_enum aif1in1r_enum =
	SOC_ENUM_SINGLE(AIF1_DACDAT_CTRL, 4, 4, aif1in1r_text);

static const struct snd_kcontrol_new aif1in1r_mux =
	SOC_DAPM_ENUM("AIF1IN1R Mux", aif1in1r_enum);

/*0x13register*/
/*AIF1 ADC0 MIXER SOURCE*/
static const struct snd_kcontrol_new aif1_ad0l_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF1 DA0L Switch", 	AIF1_MXR_SRC,  	AIF1_AD0L_AIF1_DA0L_MXR, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACL Switch", 	AIF1_MXR_SRC, 	AIF1_AD0L_AIF2_DACL_MXR, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 		AIF1_MXR_SRC, 	AIF1_AD0L_ADCL_MXR, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACR Switch", 	AIF1_MXR_SRC, 	AIF1_AD0L_AIF2_DACR_MXR, 1, 0),
};
static const struct snd_kcontrol_new aif1_ad0r_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF1 DA0R Switch", 	AIF1_MXR_SRC,  	AIF1_AD0R_AIF1_DA0R_MXR, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACR Switch", 	AIF1_MXR_SRC, 	AIF1_AD0R_AIF2_DACR_MXR, 1, 0),
	#ifdef CONFIG_SND_SUNXI_SOC_DAUDIO0_KARAOKE_MACHINE
	SOC_DAPM_SINGLE("ADCR Switch", 	ESPKOUT_CTRL, 	0, 1, 0),
	#else
	SOC_DAPM_SINGLE("ADCR Switch", 		AIF1_MXR_SRC, 	AIF1_AD0R_ADCR_MXR, 1, 0),
	#endif
	SOC_DAPM_SINGLE("AIF2 DACL Switch", 	AIF1_MXR_SRC, 	AIF1_AD0R_AIF2_DACL_MXR, 1, 0),
};

/*AIF1 ADC1 MIXER SOURCE*/
static const struct snd_kcontrol_new aif1_ad1l_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF2 DACL Switch", 	AIF1_MXR_SRC,  	AIF1_AD1L_AIF2_DACL_MXR, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 	AIF1_MXR_SRC, 	AIF1_AD1L_ADCL_MXR, 1, 0),
};
static const struct snd_kcontrol_new aif1_ad1r_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF2 DACR Switch", 	AIF1_MXR_SRC,  	AIF1_AD1R_AIF2_DACR_MXR, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", 	AIF1_MXR_SRC, 	AIF1_AD1R_ADCR_MXR, 1, 0),
};

/*4C register*/
static const struct snd_kcontrol_new dacl_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("ADCL Switch", 			DAC_MXR_SRC,  	DACL_MXR_ADCL, 1, 0),
	SOC_DAPM_SINGLE("AIF2DACL Switch", 		DAC_MXR_SRC, 	DACL_MXR_AIF2_DACL, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA1L Switch", 		DAC_MXR_SRC, 	DACL_MXR_AIF1_DA1L, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA0L Switch", 		DAC_MXR_SRC, 	DACL_MXR_AIF1_DA0L, 1, 0),
};
static const struct snd_kcontrol_new dacr_mxr_src_controls[] = {
	#ifdef CONFIG_SND_SUNXI_SOC_DAUDIO0_KARAOKE_MACHINE
	SOC_DAPM_SINGLE("ADCR Switch", 			ESPKOUT_CTRL,  	1, 1, 0),
	#else
	SOC_DAPM_SINGLE("ADCR Switch", 			DAC_MXR_SRC,  	DACR_MXR_ADCR, 1, 0),
	#endif
	SOC_DAPM_SINGLE("AIF2DACR Switch", 		DAC_MXR_SRC, 	DACR_MXR_AIF2_DACR, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA1R Switch", 		DAC_MXR_SRC, 	DACR_MXR_AIF1_DA1R, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA0R Switch", 		DAC_MXR_SRC, 	DACR_MXR_AIF1_DA0R, 1, 0),
};

/*output mixer source select*/
/*defined left output mixer*/
static const struct snd_kcontrol_new ac100_loutmix_controls[] = {
	SOC_DAPM_SINGLE("DACR Switch", OMIXER_SR, LMIXMUTEDACR, 1, 0),
	SOC_DAPM_SINGLE("DACL Switch", OMIXER_SR, LMIXMUTEDACL, 1, 0),
	SOC_DAPM_SINGLE("AUXINL Switch", OMIXER_SR, LMIXMUTEAUXINL, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", OMIXER_SR, LMIXMUTELINEINL, 1, 0),
	SOC_DAPM_SINGLE("LINEINL-LINEINR Switch", OMIXER_SR, LMIXMUTELINEINLR, 1, 0),
	SOC_DAPM_SINGLE("MIC2Booststage Switch", OMIXER_SR, LMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC1Booststage Switch", OMIXER_SR, LMIXMUTEMIC1BOOST, 1, 0),
};

/*defined right output mixer*/
static const struct snd_kcontrol_new ac100_routmix_controls[] = {
	SOC_DAPM_SINGLE("DACL Switch", OMIXER_SR, RMIXMUTEDACL, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", OMIXER_SR, RMIXMUTEDACR, 1, 0),
	SOC_DAPM_SINGLE("AUXINR Switch", OMIXER_SR, RMIXMUTEAUXINR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", OMIXER_SR, RMIXMUTELINEINR, 1, 0),
	SOC_DAPM_SINGLE("LINEINL-LINEINR Switch", OMIXER_SR, RMIXMUTELINEINLR, 1, 0),
	SOC_DAPM_SINGLE("MIC2Booststage Switch", OMIXER_SR, RMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC1Booststage Switch", OMIXER_SR, RMIXMUTEMIC1BOOST, 1, 0),
};

/*hp source select*/
/*headphone input source*/
static const char *ac100_hp_r_func_sel[] = {
	"DACR HPR Switch", "Right Analog Mixer HPR Switch"};
static const struct soc_enum ac100_hp_r_func_enum =
	SOC_ENUM_SINGLE(HPOUT_CTRL, RHPS, 2, ac100_hp_r_func_sel);

static const struct snd_kcontrol_new ac100_hp_r_func_controls =
	SOC_DAPM_ENUM("HP_R Mux", ac100_hp_r_func_enum);

static const char *ac100_hp_l_func_sel[] = {
	"DACL HPL Switch", "Left Analog Mixer HPL Switch"};
static const struct soc_enum ac100_hp_l_func_enum =
	SOC_ENUM_SINGLE(HPOUT_CTRL, LHPS, 2, ac100_hp_l_func_sel);

static const struct snd_kcontrol_new ac100_hp_l_func_controls =
	SOC_DAPM_ENUM("HP_L Mux", ac100_hp_l_func_enum);

/*spk source select*/
static const char *ac100_rspks_func_sel[] = {
	"MIXER Switch", "MIXR MIXL Switch"};
static const struct soc_enum ac100_rspks_func_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL, RSPKS, 2, ac100_rspks_func_sel);

static const struct snd_kcontrol_new ac100_rspks_func_controls =
	SOC_DAPM_ENUM("SPK_R Mux", ac100_rspks_func_enum);

static const char *ac100_lspks_l_func_sel[] = {
	"MIXEL Switch", "MIXL MIXR  Switch"};
static const struct soc_enum ac100_lspks_func_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL, LSPKS, 2, ac100_lspks_l_func_sel);

static const struct snd_kcontrol_new ac100_lspks_func_controls =
	SOC_DAPM_ENUM("SPK_L Mux", ac100_lspks_func_enum);

/*earpiece source select*/
static const char *ac100_earpiece_func_sel[] = {
	"DACR", "DACL", "Right Analog Mixer", "Left Analog Mixer"};
static const struct soc_enum ac100_earpiece_func_enum =
	SOC_ENUM_SINGLE(ESPKOUT_CTRL, ESPSR, 4, ac100_earpiece_func_sel);

static const struct snd_kcontrol_new ac100_earpiece_func_controls =
	SOC_DAPM_ENUM("EAR Mux", ac100_earpiece_func_enum);

/*AIF2 out */
static const char *aif2outl_text[] = {
	"AIF2_ADCL", "AIF2_ADCR","SUM_AIF2_ADCL_AIF2_ADCR", "AVE_AIF2_ADCL_AIF2_ADCR"
};
static const char *aif2outr_text[] = {
	"AIF2_ADCR", "AIF2_ADCL","SUM_AIF2_ADCL_AIF2_ADCR", "AVE_AIF2_ADCL_AIF2_ADCR"
};

static const struct soc_enum aif2outl_enum =
	SOC_ENUM_SINGLE(AIF2_ADCDAT_CTRL, 10, 4, aif2outl_text);

static const struct snd_kcontrol_new aif2outl_mux =
	SOC_DAPM_ENUM("AIF2OUTL Mux", aif2outl_enum);

static const struct soc_enum aif2outr_enum =
	SOC_ENUM_SINGLE(AIF2_ADCDAT_CTRL, 8, 4, aif2outr_text);

static const struct snd_kcontrol_new aif2outr_mux =
	SOC_DAPM_ENUM("AIF2OUTR Mux", aif2outr_enum);

/*AIF2 IN*/
static const char *aif2inl_text[] = {
	"AIF2_DACL", "AIF2_DACR","SUM_AIF2DACL_AIF2DACR", "AVE_AIF2DACL_AIF2DACR"
};
static const char *aif2inr_text[] = {
	"AIF2_DACR", "AIF2_DACL","SUM_AIF2DACL_AIF2DACR", "AVE_AIF2DACL_AIF2DACR"
};

static const struct soc_enum aif2inl_enum =
	SOC_ENUM_SINGLE(AIF2_DACDAT_CTRL, 10, 4, aif2inl_text);

static const struct snd_kcontrol_new aif2inl_mux =
	SOC_DAPM_ENUM("AIF2INL Mux", aif2inl_enum);

static const struct soc_enum aif2inr_enum =
	SOC_ENUM_SINGLE(AIF2_DACDAT_CTRL, 8, 4, aif2inr_text);

static const struct snd_kcontrol_new aif2inr_mux =
	SOC_DAPM_ENUM("AIF2INR Mux", aif2inr_enum);

/*23 REGIDTER*/
/*AIF2 source select*/
static const struct snd_kcontrol_new aif2_adcl_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("AIF1 DA0L Switch", 			AIF2_MXR_SRC,  	AIF2_ADCL_AIF1DA0L_MXR, 1, 0),
	SOC_DAPM_SINGLE("AIF1 DA1L Switch", 		AIF2_MXR_SRC, 	AIF2_ADCL_AIF1DA1L_MXR, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACR Switch", 		AIF2_MXR_SRC, 	AIF2_ADCL_AIF2DACR_MXR, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 		AIF2_MXR_SRC, 	AIF2_ADCL_ADCL_MXR, 1, 0),
};
static const struct snd_kcontrol_new aif2_adcr_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("AIF1 DA0R Switch", 			AIF2_MXR_SRC,  	AIF2_ADCR_AIF1DA0R_MXR, 1, 0),
	SOC_DAPM_SINGLE("AIF1 DA1R Switch", 		AIF2_MXR_SRC, 	AIF2_ADCR_AIF1DA1R_MXR, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACL Switch", 		AIF2_MXR_SRC, 	AIF2_ADCR_AIF2DACL_MXR, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", 		AIF2_MXR_SRC, 	AIF2_ADCR_ADCR_MXR, 1, 0),
};

/*aif3 out 33 REGISTER*/
static const char *aif3out_text[] = {
	"AIF2 ADC left channel", "AIF2 ADC right channel"
};

static const unsigned int aif3out_values[] = {1,2};

static const struct soc_enum aif3out_enum =
		SOC_VALUE_ENUM_SINGLE(AIF3_SGP_CTRL, 10, 3,
		ARRAY_SIZE(aif3out_text),aif3out_text, aif3out_values);

static const struct snd_kcontrol_new aif3out_mux =
	SOC_DAPM_VALUE_ENUM("AIF3OUT Mux", aif3out_enum);

/*aif2 DAC INPUT SOURCE SELECT 33 REGISTER*/
static const char *aif2dacin_text[] = {
	"Left_s right_s AIF2","Left_s AIF3 Right_s AIF2", "Left_s AIF2 Right_s AIF3"
};

static const struct soc_enum aif2dacin_enum =
	SOC_ENUM_SINGLE(AIF3_SGP_CTRL, 8, 3, aif2dacin_text);

static const struct snd_kcontrol_new aif2dacin_mux =
	SOC_DAPM_ENUM("AIF2 DAC SRC Mux", aif2dacin_enum);

/*ADC SOURCE SELECT*/
/*defined left input adc mixer*/
static const struct snd_kcontrol_new ac100_ladcmix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", ADC_SRC, LADCMIXMUTEMIC1BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", ADC_SRC, LADCMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("LININL-R Switch", ADC_SRC, LADCMIXMUTELINEINLR, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", ADC_SRC, LADCMIXMUTELINEINL, 1, 0),
	SOC_DAPM_SINGLE("AUXINL Switch", ADC_SRC, LADCMIXMUTEAUXINL, 1, 0),
	SOC_DAPM_SINGLE("Lout_Mixer_Switch", ADC_SRC, LADCMIXMUTELOUTPUT, 1, 0),
	SOC_DAPM_SINGLE("Rout_Mixer_Switch", ADC_SRC, LADCMIXMUTEROUTPUT, 1, 0),
};

/*defined right input adc mixer*/
static const struct snd_kcontrol_new ac100_radcmix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", ADC_SRC, RADCMIXMUTEMIC1BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", ADC_SRC, RADCMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("LINEINL-R Switch", ADC_SRC, RADCMIXMUTELINEINLR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", ADC_SRC, RADCMIXMUTELINEINR, 1, 0),
	SOC_DAPM_SINGLE("AUXINR Switch", ADC_SRC, RADCMIXMUTEAUXINR, 1, 0),
	SOC_DAPM_SINGLE("Rout_Mixer_Switch", ADC_SRC, RADCMIXMUTEROUTPUT, 1, 0),
	SOC_DAPM_SINGLE("Lout_Mixer_Switch", ADC_SRC, RADCMIXMUTELOUTPUT, 1, 0),
};

/*mic2 source select*/
static const char *mic2src_text[] = {
	"MIC2","MIC3"};

static const struct soc_enum mic2src_enum =
	SOC_ENUM_SINGLE(ADC_SRCBST_CTRL, 7, 2, mic2src_text);

static const struct snd_kcontrol_new mic2src_mux =
	SOC_DAPM_ENUM("MIC2 SRC", mic2src_enum);
/*59 register*/
/*defined lineout mixer*/
static const struct snd_kcontrol_new lineout_mix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", LOUT_CTRL, LINEOUTS0, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", LOUT_CTRL, LINEOUTS1, 1, 0),
	SOC_DAPM_SINGLE("Rout_Mixer_Switch", LOUT_CTRL, LINEOUTS2, 1, 0),
	SOC_DAPM_SINGLE("Lout_Mixer_Switch", LOUT_CTRL, LINEOUTS3, 1, 0),
};
/*DMIC*/
static const char *adc_mux_text[] = {
	"ADC",
	"DMIC",
};
static const struct soc_enum adc_enum =
	SOC_ENUM_SINGLE(0, 0, 2, adc_mux_text);
static const struct snd_kcontrol_new adcl_mux =
	SOC_DAPM_ENUM_VIRT("ADCL Mux", adc_enum);
static const struct snd_kcontrol_new adcr_mux =
	SOC_DAPM_ENUM_VIRT("ADCR Mux", adc_enum);

static const struct snd_kcontrol_new aif2inl_aif2switch =
	SOC_DAPM_SINGLE("aif2inl aif2Switch", ADC_SRCBST_CTRL, 0, 1, 0);
static const struct snd_kcontrol_new aif2inr_aif2switch =
	SOC_DAPM_SINGLE("aif2inr aif2Switch", ADC_SRCBST_CTRL, 1, 1, 0);

static const struct snd_kcontrol_new aif2inl_aif3switch =
	SOC_DAPM_SINGLE("aif2inl aif3Switch", OMIXER_BST1_CTRL, 9, 1, 1);
static const struct snd_kcontrol_new aif2inr_aif3switch =
	SOC_DAPM_SINGLE("aif2inr aif3Switch", OMIXER_BST1_CTRL, 10, 1, 1);
/*built widget*/
static const struct snd_soc_dapm_widget ac100_dapm_widgets[] = {
	/*aif2 switch*/
	SND_SOC_DAPM_SWITCH("AIF2INL Mux switch", SND_SOC_NOPM, 0, 1,
			    &aif2inl_aif2switch),
	SND_SOC_DAPM_SWITCH("AIF2INR Mux switch", SND_SOC_NOPM, 0, 1,
			    &aif2inr_aif2switch),

	SND_SOC_DAPM_SWITCH("AIF2INL Mux VIR switch", SND_SOC_NOPM, 0, 1,
			    &aif2inl_aif3switch),
	SND_SOC_DAPM_SWITCH("AIF2INR Mux VIR switch", SND_SOC_NOPM, 0, 1,
			    &aif2inr_aif3switch),

	SND_SOC_DAPM_MUX("AIF1OUT0L Mux", AIF1_ADCDAT_CTRL, 15, 0, &aif1out0l_mux),
	SND_SOC_DAPM_MUX("AIF1OUT0R Mux", AIF1_ADCDAT_CTRL, 14, 0, &aif1out0r_mux),

	SND_SOC_DAPM_MUX("AIF1OUT1L Mux", AIF1_ADCDAT_CTRL, 13, 0, &aif1out1l_mux),
	SND_SOC_DAPM_MUX("AIF1OUT1R Mux", AIF1_ADCDAT_CTRL, 12, 0, &aif1out1r_mux),

	SND_SOC_DAPM_MUX("AIF1IN0L Mux", AIF1_DACDAT_CTRL, 15, 0, &aif1in0l_mux),
	SND_SOC_DAPM_MUX("AIF1IN0R Mux", AIF1_DACDAT_CTRL, 14, 0, &aif1in0r_mux),

	SND_SOC_DAPM_MUX("AIF1IN1L Mux", AIF1_DACDAT_CTRL, 13, 0, &aif1in1l_mux),
	SND_SOC_DAPM_MUX("AIF1IN1R Mux", AIF1_DACDAT_CTRL, 12, 0, &aif1in1r_mux),

	SND_SOC_DAPM_MIXER("AIF1 AD0L Mixer", SND_SOC_NOPM, 0, 0, aif1_ad0l_mxr_src_ctl, ARRAY_SIZE(aif1_ad0l_mxr_src_ctl)),
	SND_SOC_DAPM_MIXER("AIF1 AD0R Mixer", SND_SOC_NOPM, 0, 0, aif1_ad0r_mxr_src_ctl, ARRAY_SIZE(aif1_ad0r_mxr_src_ctl)),

	SND_SOC_DAPM_MIXER("AIF1 AD1L Mixer", SND_SOC_NOPM, 0, 0, aif1_ad1l_mxr_src_ctl, ARRAY_SIZE(aif1_ad1l_mxr_src_ctl)),
	SND_SOC_DAPM_MIXER("AIF1 AD1R Mixer", SND_SOC_NOPM, 0, 0, aif1_ad1r_mxr_src_ctl, ARRAY_SIZE(aif1_ad1r_mxr_src_ctl)),

	SND_SOC_DAPM_MIXER_E("DACL Mixer", OMIXER_DACA_CTRL, DACALEN, 0, dacl_mxr_src_controls, ARRAY_SIZE(dacl_mxr_src_controls),
		     	late_enable_dac, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("DACR Mixer", OMIXER_DACA_CTRL, DACAREN, 0, dacr_mxr_src_controls, ARRAY_SIZE(dacr_mxr_src_controls),
		     	late_enable_dac, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*dac digital enble*/
	SND_SOC_DAPM_DAC("DAC En", NULL, DAC_DIG_CTRL, ENDA, 0),

	/*ADC digital enble*/
	SND_SOC_DAPM_ADC("ADC En", NULL, ADC_DIG_CTRL, ENAD, 0),

	SND_SOC_DAPM_MIXER("Left Output Mixer", OMIXER_DACA_CTRL, LMIXEN, 0,
			ac100_loutmix_controls, ARRAY_SIZE(ac100_loutmix_controls)),
	SND_SOC_DAPM_MIXER("Right Output Mixer", OMIXER_DACA_CTRL, RMIXEN, 0,
			ac100_routmix_controls, ARRAY_SIZE(ac100_routmix_controls)),

	SND_SOC_DAPM_MUX("HP_R Mux", SND_SOC_NOPM, 0, 0,	&ac100_hp_r_func_controls),
	SND_SOC_DAPM_MUX("HP_L Mux", SND_SOC_NOPM, 0, 0,	&ac100_hp_l_func_controls),

	SND_SOC_DAPM_MUX("SPK_R Mux", SPKOUT_CTRL, RSPK_EN, 0,	&ac100_rspks_func_controls),
	SND_SOC_DAPM_MUX("SPK_L Mux", SPKOUT_CTRL, LSPK_EN, 0,	&ac100_lspks_func_controls),

	SND_SOC_DAPM_PGA("SPK_LR Adder", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MUX("EAR Mux", ESPKOUT_CTRL, ESPPA_MUTE, 0,	&ac100_earpiece_func_controls),

	/*output widget*/
	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),
	SND_SOC_DAPM_OUTPUT("EAROUTP"),
	SND_SOC_DAPM_OUTPUT("EAROUTN"),
	SND_SOC_DAPM_OUTPUT("SPK1P"),
	SND_SOC_DAPM_OUTPUT("SPK2P"),
	SND_SOC_DAPM_OUTPUT("SPK1N"),
	SND_SOC_DAPM_OUTPUT("SPK2N"),

	SND_SOC_DAPM_OUTPUT("LINEOUTP"),
	SND_SOC_DAPM_OUTPUT("LINEOUTN"),

	SND_SOC_DAPM_MUX("AIF2OUTL Mux", AIF2_ADCDAT_CTRL, 15, 0, &aif2outl_mux),
	SND_SOC_DAPM_MUX("AIF2OUTR Mux", AIF2_ADCDAT_CTRL, 14, 0, &aif2outr_mux),

	SND_SOC_DAPM_MUX("AIF2INL Mux", AIF2_DACDAT_CTRL, 15, 0, &aif2inl_mux),
	SND_SOC_DAPM_MUX("AIF2INR Mux", AIF2_DACDAT_CTRL, 14, 0, &aif2inr_mux),

	SND_SOC_DAPM_PGA("AIF2INL_VIR", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("AIF2INR_VIR", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("AIF2 ADL Mixer", SND_SOC_NOPM, 0, 0, aif2_adcl_mxr_src_controls, ARRAY_SIZE(aif2_adcl_mxr_src_controls)),
	SND_SOC_DAPM_MIXER("AIF2 ADR Mixer", SND_SOC_NOPM, 0, 0, aif2_adcr_mxr_src_controls, ARRAY_SIZE(aif2_adcr_mxr_src_controls)),

	SND_SOC_DAPM_MUX("AIF3OUT Mux", SND_SOC_NOPM, 0, 0, &aif3out_mux),

	SND_SOC_DAPM_MUX("AIF2 DAC SRC Mux", SND_SOC_NOPM, 0, 0, &aif2dacin_mux),
	/*virtual widget*/
	SND_SOC_DAPM_PGA_E("AIF2INL Mux VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
			aif2inl_vir_event, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("AIF2INR Mux VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
			aif2inr_vir_event, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	#ifndef CONFIG_SND_SUNXI_SOC_DAUDIO0_KARAOKE_MACHINE
	SND_SOC_DAPM_MIXER_E("LEFT ADC input Mixer", ADC_APC_CTRL, ADCLEN, 0,
		ac100_ladcmix_controls, ARRAY_SIZE(ac100_ladcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("RIGHT ADC input Mixer", ADC_APC_CTRL, ADCREN, 0,
		ac100_radcmix_controls, ARRAY_SIZE(ac100_radcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	#else
	SND_SOC_DAPM_MIXER_E("LEFT ADC input Mixer", SND_SOC_NOPM, 0, 0,
		ac100_ladcmix_controls, ARRAY_SIZE(ac100_ladcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("RIGHT ADC input Mixer", SND_SOC_NOPM, 0, 0,
		ac100_radcmix_controls, ARRAY_SIZE(ac100_radcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	#endif
	/*mic reference*/
	SND_SOC_DAPM_PGA("MIC1 PGA", ADC_SRCBST_CTRL, MIC1AMPEN, 0, NULL, 0),
	SND_SOC_DAPM_PGA("MIC2 PGA", ADC_SRCBST_CTRL, MIC2AMPEN, 0, NULL, 0),

	SND_SOC_DAPM_PGA("LINEIN PGA", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MUX("MIC2 SRC", SND_SOC_NOPM, 0, 0, &mic2src_mux),

	SND_SOC_DAPM_MIXER("Line Out Mixer", LOUT_CTRL, LINEOUTEN, 0,
		lineout_mix_controls, ARRAY_SIZE(lineout_mix_controls)),

	/*INPUT widget*/
	SND_SOC_DAPM_INPUT("MIC1P"),
	SND_SOC_DAPM_INPUT("MIC1N"),

	SND_SOC_DAPM_MICBIAS("MainMic Bias", ADC_APC_CTRL, MBIASEN, 0),
	SND_SOC_DAPM_MICBIAS("HMic Bias", SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_INPUT("MIC2"),
	SND_SOC_DAPM_INPUT("MIC3"),

	SND_SOC_DAPM_INPUT("LINEINP"),
	SND_SOC_DAPM_INPUT("LINEINN"),

	SND_SOC_DAPM_INPUT("AXIR"),
	SND_SOC_DAPM_INPUT("AXIL"),
	SND_SOC_DAPM_INPUT("D_MIC"),
	/*aif1 interface*/
	SND_SOC_DAPM_AIF_IN_E("AIF1DACL", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0,ac100_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("AIF1DACR", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0,ac100_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("AIF1ADCL", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0,ac100_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("AIF1ADCR", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0,ac100_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*aif2 interface*/
	SND_SOC_DAPM_AIF_IN_E("AIF2DACL", "AIF2 Playback", 0, SND_SOC_NOPM, 0, 0,ac100_aif2clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_IN_E("AIF2DACR", "AIF2 Playback", 0, SND_SOC_NOPM, 0, 0,ac100_aif2clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("AIF2ADCL", "AIF2 Capture", 0, SND_SOC_NOPM, 0, 0,ac100_aif2clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("AIF2ADCR", "AIF2 Capture", 0, SND_SOC_NOPM, 0, 0,ac100_aif2clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*aif3 interface*/
	SND_SOC_DAPM_AIF_OUT_E("AIF3OUT", "AIF3 Capture", 0, SND_SOC_NOPM, 0, 0,ac100_aif3clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_IN_E("AIF3IN", "AIF3 Playback", 0, SND_SOC_NOPM, 0, 0,ac100_aif3clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	/*headphone*/
	SND_SOC_DAPM_HP("Headphone", ac100_headphone_event),
	/*speaker*/
	SND_SOC_DAPM_SPK("External Speaker", ac100_speaker_event),
	/*earpiece*/
	SND_SOC_DAPM_SPK("Earpiece", ac100_earpiece_event),

	/*DMIC*/
	SND_SOC_DAPM_VIRT_MUX("ADCL Mux", SND_SOC_NOPM, 0, 0, &adcl_mux),
	SND_SOC_DAPM_VIRT_MUX("ADCR Mux", SND_SOC_NOPM, 0, 0, &adcr_mux),

	SND_SOC_DAPM_PGA_E("DMICL VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
				dmic_mux_ev, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA_E("DMICR VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
				dmic_mux_ev, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
};

static const struct snd_soc_dapm_route ac100_dapm_routes[] = {
	{"AIF1ADCL", NULL, "AIF1OUT0L Mux"},
	{"AIF1ADCR", NULL, "AIF1OUT0R Mux"},

	{"AIF1ADCL", NULL, "AIF1OUT1L Mux"},
	{"AIF1ADCR", NULL, "AIF1OUT1R Mux"},

	/* aif1out0 mux 11---13*/
	{"AIF1OUT0L Mux", "AIF1_AD0L", "AIF1 AD0L Mixer"},
	{"AIF1OUT0L Mux", "AIF1_AD0R", "AIF1 AD0R Mixer"},

	{"AIF1OUT0R Mux", "AIF1_AD0R", "AIF1 AD0R Mixer"},
	{"AIF1OUT0R Mux", "AIF1_AD0L", "AIF1 AD0L Mixer"},

	/*AIF1OUT1 mux 11--13 */
	{"AIF1OUT1L Mux", "AIF1_AD1L", "AIF1 AD1L Mixer"},
	{"AIF1OUT1L Mux", "AIF1_AD1R", "AIF1 AD1R Mixer"},

	{"AIF1OUT1R Mux", "AIF1_AD1R", "AIF1 AD1R Mixer"},
	{"AIF1OUT1R Mux", "AIF1_AD1L", "AIF1 AD1L Mixer"},

	/*AIF1 AD0L Mixer*/
	{"AIF1 AD0L Mixer", "AIF1 DA0L Switch",		"AIF1IN0L Mux"},
	{"AIF1 AD0L Mixer", "AIF2 DACL Switch",		"AIF2INL_VIR"},
	{"AIF1 AD0L Mixer", "ADCL Switch",		"ADCL Mux"},
	{"AIF1 AD0L Mixer", "AIF2 DACR Switch",		"AIF2INR_VIR"},

	/*AIF1 AD0R Mixer*/
	{"AIF1 AD0R Mixer", "AIF1 DA0R Switch",		"AIF1IN0R Mux"},
	{"AIF1 AD0R Mixer", "AIF2 DACR Switch",		"AIF2INR_VIR"},
	{"AIF1 AD0R Mixer", "ADCR Switch",		"ADCR Mux"},
	{"AIF1 AD0R Mixer", "AIF2 DACL Switch",		"AIF2INL_VIR"},

	/*AIF1 AD1L Mixer*/
	{"AIF1 AD1L Mixer", "AIF2 DACL Switch",		"AIF2INL_VIR"},
	{"AIF1 AD1L Mixer", "ADCL Switch",		"ADCL Mux"},
	/*AIF1 AD1R Mixer*/
	{"AIF1 AD1R Mixer", "AIF2 DACR Switch",		"AIF2INR_VIR"},
	{"AIF1 AD1R Mixer", "ADCR Switch",		"ADCR Mux"},
	/*AIF1 DA0 IN 12h*/
	{"AIF1IN0L Mux", "AIF1_DA0L",		"AIF1DACL"},
	{"AIF1IN0L Mux", "AIF1_DA0R",		"AIF1DACR"},

	{"AIF1IN0R Mux", "AIF1_DA0R",		"AIF1DACR"},
	{"AIF1IN0R Mux", "AIF1_DA0L",		"AIF1DACL"},

	/*AIF1 DA1 IN 12h*/
	{"AIF1IN1L Mux", "AIF1_DA1L",		"AIF1DACL"},
	{"AIF1IN1L Mux", "AIF1_DA1R",		"AIF1DACR"},

	{"AIF1IN1R Mux", "AIF1_DA1R",		"AIF1DACR"},
	{"AIF1IN1R Mux", "AIF1_DA1L",		"AIF1DACL"},

	/*aif2 virtual*/
	{"AIF2INL Mux switch", "aif2inl aif2Switch",		"AIF2INL Mux"},
	{"AIF2INR Mux switch", "aif2inr aif2Switch",		"AIF2INR Mux"},

	{"AIF2INL_VIR", NULL,		"AIF2INL Mux switch"},
	{"AIF2INR_VIR", NULL,		"AIF2INR Mux switch"},

	{"AIF2INL_VIR", NULL,		"AIF2INL Mux VIR"},
	{"AIF2INR_VIR", NULL,		"AIF2INR Mux VIR"},

	/*4c*/
	{"DACL Mixer", "AIF1DA0L Switch",		"AIF1IN0L Mux"},
	{"DACL Mixer", "AIF1DA1L Switch",		"AIF1IN1L Mux"},

	{"DACL Mixer", "ADCL Switch",		"ADCL Mux"},
	{"DACL Mixer", "AIF2DACL Switch",		"AIF2INL_VIR"},
	{"DACR Mixer", "AIF1DA0R Switch",		"AIF1IN0R Mux"},
	{"DACR Mixer", "AIF1DA1R Switch",		"AIF1IN1R Mux"},

	{"DACR Mixer", "ADCR Switch",		"ADCR Mux"},
	{"DACR Mixer", "AIF2DACR Switch",		"AIF2INR_VIR"},

	{"Right Output Mixer", "DACR Switch",		"DACR Mixer"},
	{"Right Output Mixer", "DACL Switch",		"DACL Mixer"},

	{"Right Output Mixer", "AUXINR Switch",		"AXIR"},
	{"Right Output Mixer", "LINEINR Switch",		"LINEINN"},
	{"Right Output Mixer", "LINEINL-LINEINR Switch",		"LINEIN PGA"},
	{"Right Output Mixer", "MIC2Booststage Switch",		"MIC2 PGA"},
	{"Right Output Mixer", "MIC1Booststage Switch",		"MIC1 PGA"},

	{"Left Output Mixer", "DACL Switch",		"DACL Mixer"},
	{"Left Output Mixer", "DACR Switch",		"DACR Mixer"},

	{"Left Output Mixer", "AUXINL Switch",		"AXIL"},
	{"Left Output Mixer", "LINEINL Switch",		"LINEINP"},
	{"Left Output Mixer", "LINEINL-LINEINR Switch",		"LINEIN PGA"},
	{"Left Output Mixer", "MIC2Booststage Switch",		"MIC2 PGA"},
	{"Left Output Mixer", "MIC1Booststage Switch",		"MIC1 PGA"},

	/*hp mux*/
	{"HP_R Mux", "DACR HPR Switch",		"DACR Mixer"},
	{"HP_R Mux", "Right Analog Mixer HPR Switch",		"Right Output Mixer"},

	{"HP_L Mux", "DACL HPL Switch",		"DACL Mixer"},
	{"HP_L Mux", "Left Analog Mixer HPL Switch",		"Left Output Mixer"},

	/*hp endpoint*/
	{"HPOUTR", NULL,				"HP_R Mux"},
	{"HPOUTL", NULL,				"HP_L Mux"},

	{"Headphone", NULL,				"HPOUTR"},
	{"Headphone", NULL,				"HPOUTL"},

	/*External Speaker*/
	{"External Speaker", NULL, "SPK1P"},
	{"External Speaker", NULL, "SPK1N"},

	{"External Speaker", NULL, "SPK2P"},
	{"External Speaker", NULL, "SPK2N"},

	/*spk mux*/
	{"SPK_LR Adder", NULL,				"Right Output Mixer"},
	{"SPK_LR Adder", NULL,				"Left Output Mixer"},

	{"SPK_L Mux", "MIXL MIXR  Switch",				"SPK_LR Adder"},
	{"SPK_L Mux", "MIXEL Switch",				"Left Output Mixer"},

	{"SPK_R Mux", "MIXR MIXL Switch",				"SPK_LR Adder"},
	{"SPK_R Mux", "MIXER Switch",				"Right Output Mixer"},

	{"SPK1P", NULL,				"SPK_R Mux"},
	{"SPK1N", NULL,				"SPK_R Mux"},

	{"SPK2P", NULL,				"SPK_L Mux"},
	{"SPK2N", NULL,				"SPK_L Mux"},

	/*earpiece mux*/
	{"EAR Mux", "DACR",				"DACR Mixer"},
	{"EAR Mux", "DACL",				"DACL Mixer"},
	{"EAR Mux", "Right Analog Mixer",				"Right Output Mixer"},
	{"EAR Mux", "Left Analog Mixer",				"Left Output Mixer"},
	{"EAROUTP", NULL,		"EAR Mux"},
	{"EAROUTN", NULL,		"EAR Mux"},
	{"Earpiece", NULL,				"EAROUTP"},
	{"Earpiece", NULL,				"EAROUTN"},

	/*LADC SOURCE mixer*/
	{"LEFT ADC input Mixer", "MIC1 boost Switch",				"MIC1 PGA"},
	{"LEFT ADC input Mixer", "MIC2 boost Switch",				"MIC2 PGA"},
	{"LEFT ADC input Mixer", "LININL-R Switch",				"LINEIN PGA"},
	{"LEFT ADC input Mixer", "LINEINL Switch",				"LINEINN"},
	{"LEFT ADC input Mixer", "AUXINL Switch",				"AXIL"},
	{"LEFT ADC input Mixer", "Lout_Mixer_Switch",				"Left Output Mixer"},
	{"LEFT ADC input Mixer", "Rout_Mixer_Switch",				"Right Output Mixer"},

	/*RADC SOURCE mixer*/
	{"RIGHT ADC input Mixer", "MIC1 boost Switch",				"MIC1 PGA"},
	{"RIGHT ADC input Mixer", "MIC2 boost Switch",				"MIC2 PGA"},
	{"RIGHT ADC input Mixer", "LINEINL-R Switch",				"LINEIN PGA"},
	{"RIGHT ADC input Mixer", "LINEINR Switch",				"LINEINP"},
	{"RIGHT ADC input Mixer", "AUXINR Switch",				"AXIR"},
	{"RIGHT ADC input Mixer", "Rout_Mixer_Switch",				"Right Output Mixer"},
	{"RIGHT ADC input Mixer", "Lout_Mixer_Switch",				"Left Output Mixer"},

	{"MIC1 PGA", NULL,				"MIC1P"},
	{"MIC1 PGA", NULL,				"MIC1N"},

	{"MIC2 PGA", NULL,				"MIC2 SRC"},

	{"MIC2 SRC", "MIC2",				"MIC2"},
	{"MIC2 SRC", "MIC3",				"MIC3"},

	{"LINEIN PGA", NULL,				"LINEINP"},
	{"LINEIN PGA", NULL,				"LINEINN"},

	{"LINEOUTP", NULL,				"Line Out Mixer"},
	{"LINEOUTN", NULL,				"Line Out Mixer"},

	/*lineout*/
	{"Line Out Mixer", "MIC1 boost Switch",				"MIC1 PGA"},
	{"Line Out Mixer", "MIC2 boost Switch",				"MIC2 PGA"},
	{"Line Out Mixer", "Rout_Mixer_Switch",				"Right Output Mixer"},
	{"Line Out Mixer", "Lout_Mixer_Switch",				"Left Output Mixer"},

	/*AIF2 out */
	{"AIF2ADCL", NULL,				"AIF2OUTL Mux"},
	{"AIF2ADCR", NULL,				"AIF2OUTR Mux"},

	{"AIF2OUTL Mux", "AIF2_ADCL",				"AIF2 ADL Mixer"},
	{"AIF2OUTL Mux", "AIF2_ADCR",				"AIF2 ADR Mixer"},

	{"AIF2OUTR Mux", "AIF2_ADCR",				"AIF2 ADR Mixer"},
	{"AIF2OUTR Mux", "AIF2_ADCL",				"AIF2 ADL Mixer"},

	/*23*/
	{"AIF2 ADL Mixer", "AIF1 DA0L Switch",				"AIF1IN0L Mux"},
	{"AIF2 ADL Mixer", "AIF1 DA1L Switch",				"AIF1IN1L Mux"},
	{"AIF2 ADL Mixer", "AIF2 DACR Switch",				"AIF2INR_VIR"},
	{"AIF2 ADL Mixer", "ADCL Switch",				"ADCL Mux"},
	{"AIF2 ADR Mixer", "AIF1 DA0R Switch",				"AIF1IN0R Mux"},
	{"AIF2 ADR Mixer", "AIF1 DA1R Switch",				"AIF1IN1R Mux"},
	{"AIF2 ADR Mixer", "AIF2 DACL Switch",				"AIF2INL_VIR"},
	{"AIF2 ADR Mixer", "ADCR Switch",				"ADCR Mux"},

	/*aif2*/
	{"AIF2INL Mux", "AIF2_DACL",				"AIF2DACL"},
	{"AIF2INL Mux", "AIF2_DACR",				"AIF2DACR"},

	{"AIF2INR Mux", "AIF2_DACR",				"AIF2DACR"},
	{"AIF2INR Mux", "AIF2_DACL",				"AIF2DACL"},

	/*aif3*/
	{"AIF2INL Mux VIR switch", "aif2inl aif3Switch",				"AIF3IN"},
	{"AIF2INR Mux VIR switch", "aif2inr aif3Switch",				"AIF3IN"},

	{"AIF2INL Mux VIR", NULL,				"AIF2INL Mux VIR switch"},
	{"AIF2INR Mux VIR", NULL,				"AIF2INR Mux VIR switch"},

	{"AIF3OUT", NULL,				"AIF3OUT Mux"},
	{"AIF3OUT Mux", "AIF2 ADC left channel",				"AIF2 ADL Mixer"},
	{"AIF3OUT Mux", "AIF2 ADC right channel",				"AIF2 ADR Mixer"},

	/*ADC--ADCMUX*/
	{"ADCR Mux", "ADC", "RIGHT ADC input Mixer"},
	{"ADCL Mux", "ADC", "LEFT ADC input Mixer"},

	/*DMIC*/
	{"ADCR Mux", "DMIC", "DMICR VIR"},
	{"ADCL Mux", "DMIC", "DMICL VIR"},

	{"DMICL VIR", NULL, "D_MIC"},
	{"DMICR VIR", NULL, "D_MIC"},
};

/* PLL divisors */
struct pll_div {
	unsigned int pll_in;
	unsigned int pll_out;
	int m;
	int n_i;
	int n_f;
};

struct aif1_fs {
	unsigned int samplerate;
	int aif1_bclk_bit;
	int aif1_srbit;
};

struct aif1_bclk {
	int aif1_bclk_div;
	int aif1_bclk_bit;
};

struct aif1_lrck {
	int aif1_lrck_div;
	int aif1_lrck_bit;
};

struct aif1_word_size {
	int aif1_wsize_val;
	int aif1_wsize_bit;
};

/*
*	Note : pll code from original tdm/i2s driver.
* 	freq_out = freq_in * N/(m*(2k+1)) , k=1,N=N_i+N_f,N_f=factor*0.2;
*/
static const struct pll_div codec_pll_div[] = {
	{128000, 22579200, 1, 529, 1},
	{192000, 22579200, 1, 352, 4},
	{256000, 22579200, 1, 264, 3},
	{384000, 22579200, 1, 176, 2},/*((176+2*0.2)*6000000)/(38*(2*1+1))*/
	{6000000, 22579200, 38, 429, 0},/*((429+0*0.2)*6000000)/(38*(2*1+1))*/
	{13000000, 22579200, 19, 99, 0},
	{19200000, 22579200, 25, 88, 1},
	{24000000, 22579200, 38, 107, 1},
	{128000, 24576000, 1, 576, 0},
	{192000, 24576000, 1, 384, 0},
	{256000, 24576000, 1, 288, 0},
	{384000, 24576000, 1, 192, 0},
	{2048000, 24576000, 1, 36, 0},
	{6000000, 24576000, 25, 307, 1},
	{13000000, 24576000, 42, 238, 1},
	{19200000, 24576000, 25, 88, 1},
	{24000000, 24576000, 25, 76, 4},
};

/*for all of the fs freq. lrck_div is 64*/
static const struct aif1_fs codec_aif1_fs[] = {
	{44100, 4, 7},
	{48000, 4, 8},
	{8000, 9, 0},
	{11025, 8, 1},
	{12000, 8, 2},
	{16000, 7, 3},
	{22050, 6, 4},
	{24000, 6, 5},
	{32000, 5, 6},
	{96000, 2, 9},
	{192000, 1, 10},
};

static const struct aif1_bclk codec_aif1_bclk[] = {
	{1, 0},
	{2, 1},
	{4, 2},
	{6, 3},
	{8, 4},
	{12, 5},
	{16, 6},
	{24, 7},
	{32, 8},
	{48, 9},
	{64, 10},
	{96, 11},
	{128, 12},
	{192, 13},
};

static const struct aif1_lrck codec_aif1_lrck[] = {
	{16, 0},
	{32, 1},
	{64, 2},
	{128, 3},
	{256, 4},
};

static const struct aif1_word_size codec_aif1_wsize[] = {
	{8, 0},
	{16, 1},
	{20, 2},
	{24, 3},
};

static int ac100_aif_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&ac100->mute_mutex);
	if(mute){
		if (0 == ac100->aif2_mute)
			snd_soc_write(codec, DAC_VOL_CTRL, 0);
	}else{
		snd_soc_write(codec, DAC_VOL_CTRL, 0xa0a0);
	}
	mutex_unlock(&ac100->mute_mutex);
	return 0;
}
static int ac100_aif2_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&ac100->mute_mutex);
	if (mute == 0) {
		snd_soc_write(codec, DAC_VOL_CTRL, 0xa0a0);
		ac100->aif2_mute = 1;
	}
	else
		ac100->aif2_mute = 0;
	mutex_unlock(&ac100->mute_mutex);
	return 0;
}
static void ac100_aif_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	int reg_val;
	AC100_DBG("%s,line:%d\n", __func__, __LINE__);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if(agc_used){
			agc_enable(codec, 0);
		}
		reg_val = (snd_soc_read(codec, AIF_SR_CTRL) >> 12);
		reg_val &= 0xf;
		if (codec_dai->playback_active && dmic_used && reg_val == 0x4) {
			snd_soc_update_bits(codec, AIF_SR_CTRL, (0xf<<AIF1_FS), (0x7<<AIF1_FS));
		}

	}

}

static int ac100_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int i = 0;
	int AIF_CLK_CTRL = 0;
	int aif1_word_size = 16;
	/*
	* 22.5792M/8 = 2.8224M;
	* 2.8224M/64 = 44.1k;
	*
	* 24.576M/8 = 3.072M;
	* 3.072M/64 = 48k;
	*/
	int aif1_bclk_div = 8;
	int aif1_lrck_div = 64;

	struct snd_soc_codec *codec = codec_dai->codec;
	switch (codec_dai->id) {
	case 1:
		AIF_CLK_CTRL = AIF1_CLK_CTRL;
		aif1_lrck_div = 64;
		break;
	case 2:
		AIF_CLK_CTRL = AIF2_CLK_CTRL;
		aif1_lrck_div = 64;
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_bclk); i++) {
		if (codec_aif1_bclk[i].aif1_bclk_div == aif1_bclk_div) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0xf<<AIF1_BCLK_DIV), ((codec_aif1_bclk[i].aif1_bclk_bit)<<AIF1_BCLK_DIV));
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_lrck); i++) {
		if (codec_aif1_lrck[i].aif1_lrck_div == aif1_lrck_div) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x7<<AIF1_LRCK_DIV), ((codec_aif1_lrck[i].aif1_lrck_bit)<<AIF1_LRCK_DIV));
			break;
		}
	}
	/*for all of the fs freq. lrck_div is 64*/
	for (i = 0; i < ARRAY_SIZE(codec_aif1_fs); i++) {
		if (codec_aif1_fs[i].samplerate ==  params_rate(params)) {
			if (codec_dai->capture_active && dmic_used && codec_aif1_fs[i].samplerate == 44100) {
				snd_soc_update_bits(codec, AIF_SR_CTRL, (0xf<<AIF1_FS), (0x4<<AIF1_FS));
			} else
				snd_soc_update_bits(codec, AIF_SR_CTRL, (0xf<<AIF1_FS), ((codec_aif1_fs[i].aif1_srbit)<<AIF1_FS));
			snd_soc_update_bits(codec, AIF_SR_CTRL, (0xf<<AIF2_FS), ((codec_aif1_fs[i].aif1_srbit)<<AIF2_FS));
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0xf<<AIF1_BCLK_DIV), ((codec_aif1_fs[i].aif1_bclk_bit)<<AIF1_BCLK_DIV));
			break;
		}
	}
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S24_LE:
		case SNDRV_PCM_FORMAT_S32_LE:
			aif1_word_size = 24;
		break;
		case SNDRV_PCM_FORMAT_S16_LE:
		default:
			aif1_word_size = 16;
		break;
	}
	for (i = 0; i < ARRAY_SIZE(codec_aif1_wsize); i++) {
		if (codec_aif1_wsize[i].aif1_wsize_val == aif1_word_size) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x3<<AIF1_WORK_SIZ), ((codec_aif1_wsize[i].aif1_wsize_bit)<<AIF1_WORK_SIZ));
			break;
		}
	}

	return 0;
}

static int ac100_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	switch (clk_id) {
		case AIF1_CLK:
			AC100_DBG("%s,line:%d,snd_soc_read(codec, SYSCLK_CTRL):%x\n", __func__, __LINE__, snd_soc_read(codec, SYSCLK_CTRL));
			/*system clk from aif1*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_SRC), (0x0<<SYSCLK_SRC));
			break;
		case AIF2_CLK:
			/*system clk from aif2*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<SYSCLK_SRC), (0x1<<SYSCLK_SRC));
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static int ac100_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	int reg_val;
	int AIF_CLK_CTRL = 0;
	struct snd_soc_codec *codec = codec_dai->codec;

	switch (codec_dai->id) {
	case 1:
		AC100_DBG("%s,line:%d\n", __func__, __LINE__);
		AIF_CLK_CTRL = AIF1_CLK_CTRL;
		break;
	case 2:
		AC100_DBG("%s,line:%d\n", __func__, __LINE__);
		AIF_CLK_CTRL = AIF2_CLK_CTRL;
		break;
	default:
		return -EINVAL;
	}
	AC100_DBG("%s,line:%d\n", __func__, __LINE__);

	/*
	* 	master or slave selection
	*	0 = Master mode
	*	1 = Slave mode
	*/
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	reg_val &=~(0x1<<AIF1_MSTR_MOD);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master, ap is slave*/
			reg_val |= (0x0<<AIF1_MSTR_MOD);
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave,ap is master*/
			reg_val |= (0x1<<AIF1_MSTR_MOD);
			break;
		default:
			pr_err("unknwon master/slave format\n");
			return -EINVAL;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	/* i2s mode selection */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	reg_val&=~(3<<AIF1_DATA_FMT);
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK){
		case SND_SOC_DAIFMT_I2S:        /* I2S1 mode */
			reg_val |= (0x0<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val |= (0x2<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val |= (0x1<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L reg_val msb after FRM LRC */
			reg_val |= (0x3<<AIF1_DATA_FMT);
			break;
		default:
			pr_err("%s, line:%d\n", __func__, __LINE__);
			return -EINVAL;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	/* DAI signal inversions */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + nor frame */
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + inv frm */
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	return 0;
}

static int ac100_set_fll(struct snd_soc_dai *codec_dai, int pll_id, int source,
									unsigned int freq_in, unsigned int freq_out)
{
	int i = 0;
	int m 	= 0;
	int n_i = 0;
	int n_f = 0;

	struct snd_soc_codec *codec = codec_dai->codec;
	AC100_DBG("%s, line:%d, pll_id:%d\n", __func__, __LINE__, pll_id);
	if (!freq_out)
		return 0;
	if ((freq_in < 128000) || (freq_in > 24576000)) {
		return -EINVAL;
	} else if ((freq_in == 24576000) || (freq_in == 22579200)) {
		switch (pll_id) {
			case AC100_MCLK1:
				/*select aif1 clk source from mclk1*/
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<AIF1CLK_SRC), (0x0<<AIF1CLK_SRC));
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<AIF2CLK_SRC), (0x0<<AIF2CLK_SRC));
				break;
			case AC100_MCLK2:
				/*select aif1 clk source from mclk2*/
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<AIF1CLK_SRC), (0x1<<AIF1CLK_SRC));
				snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<AIF2CLK_SRC), (0x1<<AIF2CLK_SRC));
				break;
			default:
				return -EINVAL;

		}
		return 0;
	}
	switch (pll_id) {
		case AC100_MCLK1:
			/*pll source from MCLK1*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<PLLCLK_SRC), (0x0<<PLLCLK_SRC));
			break;
		case AC100_MCLK2:
			/*pll source from MCLK2*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<PLLCLK_SRC), (0x1<<PLLCLK_SRC));
			break;
		case AC100_BCLK1:
			/*pll source from BCLK1*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<PLLCLK_SRC), (0x2<<PLLCLK_SRC));
			break;
		case AC100_BCLK2:
			/*pll source from BCLK2*/
			snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<PLLCLK_SRC), (0x3<<PLLCLK_SRC));
			break;
		default:
			return -EINVAL;
	}
	/* freq_out = freq_in * n/(m*(2k+1)) , k=1,N=N_i+N_f */
	for (i = 0; i < ARRAY_SIZE(codec_pll_div); i++) {
		if ((codec_pll_div[i].pll_in == freq_in) && (codec_pll_div[i].pll_out == freq_out)) {
			m 	= codec_pll_div[i].m;
			n_i = codec_pll_div[i].n_i;
			n_f	= codec_pll_div[i].n_f;
			break;
		}
	}
	/*config pll m*/
	snd_soc_update_bits(codec, PLL_CTRL1, (0x3f<<PLL_POSTDIV_M), (m<<PLL_POSTDIV_M));
	/*config pll n*/
	snd_soc_update_bits(codec, PLL_CTRL2, (0x3ff<<PLL_PREDIV_NI), (n_i<<PLL_PREDIV_NI));
	snd_soc_update_bits(codec, PLL_CTRL2, (0x7<<PLL_POSTDIV_NF), (n_f<<PLL_POSTDIV_NF));
	snd_soc_update_bits(codec, PLL_CTRL2, (0x1<<PLL_EN), (1<<PLL_EN));
	/*enable pll_enable*/
	snd_soc_update_bits(codec, SYSCLK_CTRL, (0x1<<PLLCLK_ENA), (1<<PLLCLK_ENA));
	snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<AIF1CLK_SRC), (0x3<<AIF1CLK_SRC));
	snd_soc_update_bits(codec, SYSCLK_CTRL, (0x3<<AIF2CLK_SRC), (0x3<<AIF2CLK_SRC));

	return 0;
}

static int ac100_audio_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	AC100_DBG("%s,line:%d\n", __func__, __LINE__);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if(agc_used){
			agc_enable(codec, 1);
		}
	}
	return 0;
}
static int ac100_aif2_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int i = 0;
	int AIF_CLK_CTRL = 0;
	int aif1_word_size = 16;
	int aif1_bclk_div = aif2_bclk_div;/*aif2_bclk_div=8, 24.576M/8=3.072M*/
	int aif1_lrck_div = aif2_lrck_div;/*aif2_lrck_div=64, 3.072M/64=48k*/
	struct snd_soc_codec *codec = codec_dai->codec;

	switch (codec_dai->id) {
		case 1:
			AIF_CLK_CTRL = AIF1_CLK_CTRL;
			break;
		case 2:
			AIF_CLK_CTRL = AIF2_CLK_CTRL;
			break;
		default:
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_bclk); i++) {
		if (codec_aif1_bclk[i].aif1_bclk_div == aif1_bclk_div) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0xf<<AIF1_BCLK_DIV), ((codec_aif1_bclk[i].aif1_bclk_bit)<<AIF1_BCLK_DIV));
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_lrck); i++) {
		if (codec_aif1_lrck[i].aif1_lrck_div == aif1_lrck_div) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x7<<AIF1_LRCK_DIV), ((codec_aif1_lrck[i].aif1_lrck_bit)<<AIF1_LRCK_DIV));
			break;
		}
	}
	for (i = 0; i < ARRAY_SIZE(codec_aif1_fs); i++) {
		if (codec_aif1_fs[i].samplerate ==  params_rate(params)) {
			snd_soc_update_bits(codec, AIF_SR_CTRL, (0xf<<AIF1_FS), ((codec_aif1_fs[i].aif1_srbit)<<AIF1_FS));
			snd_soc_update_bits(codec, AIF_SR_CTRL, (0xf<<AIF2_FS), ((codec_aif1_fs[i].aif1_srbit)<<AIF2_FS));
			break;
		}
	}
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S24_LE:
		case SNDRV_PCM_FORMAT_S32_LE:
			aif1_word_size = 24;
		break;
		case SNDRV_PCM_FORMAT_S16_LE:
		default:
			aif1_word_size = 16;
		break;
	}
	for (i = 0; i < ARRAY_SIZE(codec_aif1_wsize); i++) {
		if (codec_aif1_wsize[i].aif1_wsize_val == aif1_word_size) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x3<<AIF1_WORK_SIZ), ((codec_aif1_wsize[i].aif1_wsize_bit)<<AIF1_WORK_SIZ));
			break;
		}
	}
	if (params_channels(params) == 1) {
	
		snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x1<<1), (0x1<<1));
	} else
		snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x1<<1), (0x1<<0));
	return 0;
}

static int ac100_aif3_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	int reg_val;
	struct snd_soc_codec *codec = codec_dai->codec;
	AC100_DBG("%s,line:%d\n", __func__, __LINE__);
	/* DAI signal inversions */
	reg_val = snd_soc_read(codec, AIF3_CLK_CTRL);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + nor frame */
			reg_val &= ~(0x1<<AIF3_LRCK_INV);
			reg_val &= ~(0x1<<AIF3_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val |= (0x1<<AIF3_LRCK_INV);
			reg_val &= ~(0x1<<AIF3_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val &= ~(0x1<<AIF3_LRCK_INV);
			reg_val |= (0x1<<AIF3_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + inv frm */
			reg_val |= (0x1<<AIF3_LRCK_INV);
			reg_val |= (0x1<<AIF3_BCLK_INV);
			break;
	}
	snd_soc_write(codec, AIF3_CLK_CTRL, reg_val);

	return 0;
}

static int ac100_aif3_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int aif3_word_size = 0;
	int aif3_size =0;
	struct snd_soc_codec *codec = codec_dai->codec;
	/*config aif3clk from aif2clk*/
	snd_soc_update_bits(codec, AIF3_CLK_CTRL, (0x3<<AIF3_CLOC_SRC), (0x1<<AIF3_CLOC_SRC));
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S24_LE:
		//case SNDRV_PCM_FORMAT_S32_LE:
			aif3_word_size = 24;
			aif3_size = 3;
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
		default:
			aif3_word_size = 16;
			aif3_size = 1;
			break;
	}
	snd_soc_update_bits(codec, AIF3_CLK_CTRL, (0x3<<AIF3_WORD_SIZ), aif3_size<<AIF3_WORD_SIZ);
	return 0;
}
#ifndef CONFIG_SUNXI_SWITCH_GPIO
/*
**switch_hw_config:config the 53 codec register
*/
static void switch_hw_config(struct snd_soc_codec *codec)
{
	/*HMIC/MMIC BIAS voltage level select:2.5v*/
	snd_soc_update_bits(codec, OMIXER_BST1_CTRL, (0xf<<BIASVOLTAGE), (0xf<<BIASVOLTAGE));
	/*debounce when Key down or keyup*/
	snd_soc_update_bits(codec, HMIC_CTRL1, (0xf<<HMIC_M), (0x0<<HMIC_M));
	/*debounce when earphone plugin or pullout*/
	snd_soc_update_bits(codec, HMIC_CTRL1, (0xf<<HMIC_N), (0x0<<HMIC_N));
	/*Down Sample Setting Select/11:Downby 8,16Hz*/
	snd_soc_update_bits(codec, HMIC_CTRL2, (0x3<<HMIC_SAMPLE_SELECT), (0x0<<HMIC_SAMPLE_SELECT));
	/*Hmic_th2 for detecting Keydown or Keyup.*/
	snd_soc_update_bits(codec, HMIC_CTRL2, (0x1f<<HMIC_TH2), (0x8<<HMIC_TH2));
	/*Hmic_th1[4:0],detecting eraphone plugin or pullout*/
	snd_soc_update_bits(codec, HMIC_CTRL2, (0x1f<<HMIC_TH1), (0x1<<HMIC_TH1));
	/*Headset microphone BIAS Enable*/
	snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<HBIASEN), (0x1<<HBIASEN));
	/*Headset microphone BIAS Current sensor & ADC Enable*/
	snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<HBIASADCEN), (0x1<<HBIASADCEN));
	/*Earphone Plugin/out Irq Enable*/
	snd_soc_update_bits(codec, HMIC_CTRL1, (0x1<<HMIC_PULLOUT_IRQ), (0x1<<HMIC_PULLOUT_IRQ));
	snd_soc_update_bits(codec, HMIC_CTRL1, (0x1<<HMIC_PLUGIN_IRQ), (0x1<<HMIC_PLUGIN_IRQ));

	/*Hmic KeyUp/key down Irq Enable*/
	snd_soc_update_bits(codec, HMIC_CTRL1, (0x1<<HMIC_KEYDOWN_IRQ), (0x1<<HMIC_KEYDOWN_IRQ));
	snd_soc_update_bits(codec, HMIC_CTRL1, (0x1<<HMIC_KEYUP_IRQ), (0x1<<HMIC_KEYUP_IRQ));

	/*headphone calibration clock frequency select*/
	snd_soc_update_bits(codec, SPKOUT_CTRL, (0x7<<HPCALICKS), (0x7<<HPCALICKS));
}
#endif
static int ac100_set_bias_level(struct snd_soc_codec *codec,
				      enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
		AC100_DBG("%s,line:%d, SND_SOC_BIAS_ON\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_PREPARE:
		AC100_DBG("%s,line:%d, SND_SOC_BIAS_PREPARE\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_STANDBY:
		#ifndef CONFIG_SUNXI_SWITCH_GPIO
		switch_hw_config(codec);
		#endif
		AC100_DBG("%s,line:%d, SND_SOC_BIAS_STANDBY\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_OFF:
		#ifndef CONFIG_SUNXI_SWITCH_GPIO
		snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<HBIASEN), (0<<HBIASEN));
		snd_soc_update_bits(codec, ADC_APC_CTRL, (0x1<<HBIASADCEN), (0<<HBIASADCEN));
		#endif
		snd_soc_update_bits(codec, OMIXER_DACA_CTRL, (0xf<<HPOUTPUTENABLE), (0<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, ADDA_TUNE3, (0x1<<OSCEN), (0<<OSCEN));
		AC100_DBG("%s,line:%d, SND_SOC_BIAS_OFF\n", __func__, __LINE__);
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}
static const struct snd_soc_dai_ops ac100_aif1_dai_ops = {
	.set_sysclk	= ac100_set_dai_sysclk,
	.set_fmt	= ac100_set_dai_fmt,
	.hw_params	= ac100_hw_params,
	.shutdown	= ac100_aif_shutdown,
	.digital_mute	= ac100_aif_mute,
	.set_pll	= ac100_set_fll,
	.startup = ac100_audio_startup,
};

static const struct snd_soc_dai_ops ac100_aif2_dai_ops = {
	.set_sysclk	= ac100_set_dai_sysclk,
	.set_fmt	= ac100_set_dai_fmt,
	.hw_params	= ac100_aif2_hw_params,
	.shutdown	= ac100_aif_shutdown,
	.set_pll	= ac100_set_fll,
	.digital_mute	= ac100_aif2_mute,
};

static const struct snd_soc_dai_ops ac100_aif3_dai_ops = {
	.hw_params	= ac100_aif3_hw_params,
	.set_fmt	= ac100_aif3_set_dai_fmt,
};

static struct snd_soc_dai_driver ac100_dai[] = {
	{
		.name = "ac100-aif1",
		.id = 1,
		.playback = {
			.stream_name = "AIF1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ac100_RATES,
			.formats = ac100_FORMATS,
		},
		.capture = {
			.stream_name = "AIF1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ac100_RATES,
			.formats = ac100_FORMATS,
		 },
		.ops = &ac100_aif1_dai_ops,
	},
	{
		.name = "ac100-aif2",
		.id = 2,
		.playback = {
			.stream_name = "AIF2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ac100_RATES,
			.formats = ac100_FORMATS,
		},
		.capture = {
			.stream_name = "AIF2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ac100_RATES,
			.formats = ac100_FORMATS,
		},
		.ops = &ac100_aif2_dai_ops,
	},
	{
		.name = "ac100-aif3",
		.id = 3,
		.playback = {
			.stream_name = "AIF3 Playback",
			.channels_min = 1,
			.channels_max = 1,
			.rates = ac100_RATES,
			.formats = ac100_FORMATS,
		},
		.capture = {
			.stream_name = "AIF3 Capture",
			.channels_min = 1,
			.channels_max = 1,
			.rates = ac100_RATES,
			.formats = ac100_FORMATS,
		 },
		.ops = &ac100_aif3_dai_ops,
	}
};
#ifndef CONFIG_SUNXI_SWITCH_GPIO
static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct ac100_priv	*ac100 =
		container_of(sdev, struct ac100_priv, sdev);
	return sprintf(buf, "%d\n", ac100->state);
}

static ssize_t print_headset_name(struct switch_dev *sdev, char *buf)
{
	struct ac100_priv	*ac100 =
		container_of(sdev, struct ac100_priv, sdev);
	return sprintf(buf, "%s\n", ac100->sdev.name);
}

/*
**switch_status_update: update the switch state.
*/
static void switch_status_update(struct ac100_priv *para)
{
	struct ac100_priv *ac100 = para;
	AC100_DBG("%s,line:%d,ac100->state:%d\n",__func__, __LINE__, ac100->state);
	down(&ac100->sem);
	switch_set_state(&ac100->sdev, ac100->state);
	up(&ac100->sem);
}

/*
**clear_codec_irq_work: clear audiocodec pending and Record the interrupt.
*/
static void clear_codec_irq_work(struct work_struct *work)
{
	int reg_val = 0;
	struct ac100_priv *ac100 = container_of(work, struct ac100_priv, clear_codec_irq);
	struct snd_soc_codec *codec = ac100->codec;
	irq_flag = irq_flag+1;
	reg_val = snd_soc_read(codec, HMIC_STS);
	if ((0x1<<4)&reg_val) {
		reset_flag++;
		pr_debug("reset_flag:%d\n",reset_flag);
	}
	reg_val |= (0x1f<<0);
	snd_soc_write(codec, HMIC_STS, reg_val);
	reg_val = snd_soc_read(codec, HMIC_STS);
	if((reg_val&0x1f) != 0){
		reg_val |= (0x1f<<0);
		snd_soc_write(codec, HMIC_STS, reg_val);
	}

	if (cancel_work_sync(&ac100->work) != 0) {
			irq_flag--;
	}

	if (0 == queue_work(switch_detect_queue, &ac100->work)) {
		irq_flag--;
		pr_err("[clear_codec_irq_work]add work struct failed!\n");
	}
}

/*
**earphone_switch_work: judge the status of the headphone
*/
static void earphone_switch_work(struct work_struct *work)
{
	int reg_val = 0;
	int tmp  = 0;
	unsigned int temp_value[11];
	struct ac100_priv *ac100 = container_of(work, struct ac100_priv, work);
	struct snd_soc_codec *codec = ac100->codec;
	irq_flag--;
	ac100->check_count = 0;
	ac100->check_count_sum = 0;
	/*read HMIC_DATA */
	tmp = snd_soc_read(codec, HMIC_STS);
	reg_val = tmp;
	tmp = (tmp>>HMIC_DATA);
	tmp &= 0x1f;

	if ((tmp>=0xb) && (ac100->mode== FOUR_HEADPHONE_PLUGIN)) {
		tmp = snd_soc_read(codec, HMIC_STS);
		tmp = (tmp>>HMIC_DATA);
		tmp &= 0x1f;
		if(tmp>=0x19){
			msleep(150);
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(((tmp<0xb && tmp>=0x1) || tmp>=0x19)&&(reset_flag == 0)){
				input_report_key(ac100->key, KEY_HEADSETHOOK, 1);
				input_sync(ac100->key);
				pr_debug("%s,line:%d,KEY_HEADSETHOOK1\n",__func__,__LINE__);
				if(hook_flag1 != hook_flag2){
					hook_flag1 = hook_flag2 = 0;
				}
				hook_flag1++;
			}
			if(reset_flag)
				reset_flag--;
		}else if(tmp<0x19 && tmp>=0x17){
			msleep(80);
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(tmp<0x19 && tmp>=0x17 &&(reset_flag == 0)) {
				KEY_VOLUME_FLAG = 1;
				input_report_key(ac100->key, KEY_VOLUMEUP, 1);
				input_sync(ac100->key);
				input_report_key(ac100->key, KEY_VOLUMEUP, 0);
				input_sync(ac100->key);
				pr_debug("%s,line:%d,tmp:%d,KEY_VOLUMEUP\n",__func__,__LINE__,tmp);
			}
			if(reset_flag)
				reset_flag--;
		}else if(tmp<0x17 && tmp>=0x13){
			msleep(80);
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(tmp<0x17 && tmp>=0x13  && (reset_flag == 0)){
				KEY_VOLUME_FLAG = 1;
				input_report_key(ac100->key, KEY_VOLUMEDOWN, 1);
				input_sync(ac100->key);
				input_report_key(ac100->key, KEY_VOLUMEDOWN, 0);
				input_sync(ac100->key);
				pr_debug("%s,line:%d,KEY_VOLUMEDOWN\n",__func__,__LINE__);
			}
			if(reset_flag)
				reset_flag--;
		}
	} else if ((tmp<0xb && tmp>=0x2) && (ac100->mode== FOUR_HEADPHONE_PLUGIN)) {
		/*read HMIC_DATA */
		tmp = snd_soc_read(codec, HMIC_STS);
		tmp = (tmp>>HMIC_DATA);
		tmp &= 0x1f;
		if (tmp<0xb && tmp>=0x2) {
			if(KEY_VOLUME_FLAG) {
				KEY_VOLUME_FLAG = 0;
			}
			if(hook_flag1 == (++hook_flag2)) {
				hook_flag1 = hook_flag2 = 0;
				input_report_key(ac100->key, KEY_HEADSETHOOK, 0);
				input_sync(ac100->key);
				pr_debug("%s,line:%d,KEY_HEADSETHOOK0\n",__func__,__LINE__);
			}
		}
	} else {
		while (irq_flag == 0) {
			msleep(20);
			/*read HMIC_DATA */
			tmp = snd_soc_read(codec, HMIC_STS);
			tmp = (tmp>>HMIC_DATA);
			tmp &= 0x1f;
			if(ac100->check_count_sum <= HEADSET_CHECKCOUNT_SUM){
				if (ac100->check_count <= HEADSET_CHECKCOUNT){
					temp_value[ac100->check_count] = tmp;
					ac100->check_count++;
					if(ac100->check_count >= 2){
						if( !(temp_value[ac100->check_count - 1] == temp_value[(ac100->check_count) - 2])){
							ac100->check_count = 0;
							ac100->check_count_sum = 0;
						}
					}
				}else{
					ac100->check_count_sum++;
				}
			}else{
				if (temp_value[ac100->check_count -2] >= 0xb) {

					ac100->state		= 2;
					ac100->mode = THREE_HEADPHONE_PLUGIN;
					switch_status_update(ac100);
					ac100->check_count = 0;
					ac100->check_count_sum = 0;
					reset_flag = 0;
					break;
				} else if(temp_value[ac100->check_count - 2]>=0x1 && temp_value[ac100->check_count -2]<0xb) {
					ac100->mode = FOUR_HEADPHONE_PLUGIN;
					ac100->state		= 1;
					switch_status_update(ac100);
					ac100->check_count = 0;
					ac100->check_count_sum = 0;
					reset_flag = 0;
					break;
				} else {
					ac100->mode = HEADPHONE_IDLE;
					ac100->state = 0;
					switch_status_update(ac100);
					ac100->check_count = 0;
					ac100->check_count_sum = 0;
					reset_flag = 0;
					break;
				}
			}
		}
	}
}

/*
**audio_hmic_irq:  the interrupt handlers
*/
static irqreturn_t audio_hmic_irq(int irq, void *para)
{
	struct ac100_priv *ac100 = (struct ac100_priv *)para;
	if (ac100 == NULL) {
		return -EINVAL;
	}
	if(codec_irq_queue == NULL)
		pr_err("------------codec_irq_queue is null!!----------");
	if(&ac100->clear_codec_irq == NULL)
		pr_err("------------ac100->clear_codec_irq is null!!----------");

	if(0 == queue_work(codec_irq_queue, &ac100->clear_codec_irq)){
		pr_err("[audio_hmic_irq]add work struct failed!\n");
	}
	return 0;
}
#endif
static void codec_resume_work(struct work_struct *work)
{
	struct ac100_priv *ac100 = container_of(work, struct ac100_priv, codec_resume);
	struct snd_soc_codec *codec = ac100->codec;
	int i ,ret =0;
#ifndef CONFIG_SUNXI_SWITCH_GPIO
	ac100->virq = gpio_to_irq(item_eint.gpio.gpio);
	if (IS_ERR_VALUE(ac100->virq)) {
		pr_warn("[AC100] map gpio to virq failed, errno = %d\n",ac100->virq);
		//return -EINVAL;
	}

	pr_debug("[AC100] gpio [%d] map to virq [%d] ok\n", item_eint.gpio.gpio, ac100->virq);
	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(codec->dev, ac100->virq, audio_hmic_irq, IRQF_TRIGGER_FALLING, "SWTICH_EINT", ac100);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("[AC100] request virq %d failed, errno = %d\n", ac100->virq, ret);
        	//return -EINVAL;
	}
	gpio_set_debounce(item_eint.gpio.gpio, 1);
#endif
	for (i = 0; i < ARRAY_SIZE(ac100_supplies); i++){
		ret = regulator_enable(ac100->supplies[i].consumer);

		if (0 != ret) {
		pr_err("[AC100] %s: some error happen, fail to enable regulator!\n", __func__);
		}
	}
	msleep(50);
	set_configuration(codec);
	if (agc_used) {
		agc_config(codec);
	}
	if (drc_used) {
		drc_config(codec);
	}
	/*enable this bit to prevent leakage from ldoin*/
	snd_soc_update_bits(codec, ADDA_TUNE3, (0x1<<OSCEN), (0x1<<OSCEN));
	if (spkgpio.used) {
		gpio_direction_output(spkgpio.gpio, 1);
		gpio_set_value(spkgpio.gpio, 0);
	}
	#ifndef CONFIG_SUNXI_SWITCH_GPIO
	msleep(200);
	ret = snd_soc_read(codec, HMIC_STS);
	ret = (ret>>HMIC_DATA);
	ret &= 0x1f;
	if (ret < 1) {
		ac100->state		= 0;
		switch_status_update(ac100);
	}
	#endif
}

/***************************************************************************/
static ssize_t ac100_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	static int val = 0, flag = 0;
	u8 reg,num,i=0;
	u16 value_w,value_r[128];
	struct ac100_priv *ac100 = dev_get_drvdata(dev);
	val = simple_strtol(buf, NULL, 16);
	flag = (val >> 24) & 0xF;
	if(flag) {//write
		reg = (val >> 16) & 0xFF;
		value_w =  val & 0xFFFF;
		snd_soc_write(ac100->codec, reg, value_w);
		printk("write 0x%x to reg:0x%x\n",value_w,reg);
	} else {
		reg =(val>>8)& 0xFF;
		num=val&0xff;
		printk("\n");
		printk("read:start add:0x%x,count:0x%x\n",reg,num);
		do{
			value_r[i] = snd_soc_read(ac100->codec, reg);
			printk("0x%x: 0x%04x ",reg,value_r[i]);
			reg+=1;
			i++;
			if(i == num)
				printk("\n");
			if(i%4==0)
				printk("\n");
		}while(i<num);
	}
	return count;
}
static ssize_t ac100_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	printk("echo flag|reg|val > ac100\n");
	printk("eg read star addres=0x06,count 0x10:echo 0610 >ac100\n");
	printk("eg write value:0x13fe to address:0x06 :echo 10613fe > ac100\n");
    return 0;
}
static DEVICE_ATTR(ac100, 0644, ac100_debug_show, ac100_debug_store);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_ac100.attr,
	NULL,
};

static struct attribute_group audio_debug_attr_group = {
	.name   = "ac100_debug",
	.attrs  = audio_debug_attrs,
};
/************************************************************/
static int ac100_codec_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	int i = 0;
#ifndef CONFIG_SUNXI_SWITCH_GPIO
	int req_status = 0;
#endif
	script_item_value_type_e  type;
	script_item_u val;
	struct ac100_priv *ac100;
	//struct device *dev = codec->dev;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	ac100 = dev_get_drvdata(codec->dev);
	if (ac100 == NULL) {
		return -ENOMEM;
	}
	ac100->codec = codec;
	snd_soc_codec_set_drvdata(codec, ac100);
	/*ac100 switch driver*/
#ifndef CONFIG_SUNXI_SWITCH_GPIO
	ac100->sdev.state 		= 0;
	ac100->state				= -1;
	ac100->check_count		= 0;
	ac100->check_count_sum 	= 0;
	ac100->sdev.name 			= "h2w";
	ac100->sdev.print_name 	= print_headset_name;
	ac100->sdev.print_state 	= switch_gpio_print_state;

	ret = switch_dev_register(&ac100->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}

	/*use for judge the state of switch*/
	INIT_WORK(&ac100->work, earphone_switch_work);
	INIT_WORK(&ac100->clear_codec_irq,clear_codec_irq_work);

	/********************create input device************************/
	ac100->key = input_allocate_device();
	if (!ac100->key) {
	     pr_err("[AC100] input_allocate_device: not enough memory for input device\n");
	     ret = -ENOMEM;
	     goto err_input_allocate_device;
	}

	ac100->key->name          = "headset";
	ac100->key->phys          = "headset/input0";
	ac100->key->id.bustype    = BUS_HOST;
	ac100->key->id.vendor     = 0x0001;
	ac100->key->id.product    = 0xffff;
	ac100->key->id.version    = 0x0100;

	ac100->key->evbit[0] = BIT_MASK(EV_KEY);

	set_bit(KEY_HEADSETHOOK, ac100->key->keybit);
	set_bit(KEY_VOLUMEUP, ac100->key->keybit);
	set_bit(KEY_VOLUMEDOWN, ac100->key->keybit);

	ret = input_register_device(ac100->key);
	if (ret) {
	    pr_err("[AC100] input_register_device: input_register_device failed\n");
	    goto err_input_register_device;
	}
	headphone_state = 0;
	ac100->mode = HEADPHONE_IDLE;
	sema_init(&ac100->sem, 1);
	codec_irq_queue = create_singlethread_workqueue("codec_irq");
	switch_detect_queue = create_singlethread_workqueue("codec_resume");
	if (switch_detect_queue == NULL || codec_irq_queue == NULL) {
		pr_err("[AC100] try to create workqueue for codec failed!\n");
		ret = -ENOMEM;
		goto err_switch_work_queue;
	}

	type = script_get_item("ac100_audio0", "audio_int_ctrl", &item_eint);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("[AC100] script_get_item return type err\n");
	}
#endif
	INIT_WORK(&ac100->codec_resume, codec_resume_work);
	ac100->dac_enable = 0;
	ac100->adc_enable = 0;
	ac100->aif1_clken = 0;
	ac100->aif2_clken = 0;
	ac100->aif3_clken = 0;
	mutex_init(&ac100->dac_mutex);
	mutex_init(&ac100->adc_mutex);
	mutex_init(&ac100->aifclk_mutex);
	mutex_init(&ac100->mute_mutex);

	/*get the default pa val(close)*/
	type = script_get_item("ac100_audio0", "audio_pa_ctrl", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("ac100_audio0 audio_pa_ctrl script_get_item return type err\n");
		spkgpio.used = false;
	} else {
		spkgpio.used = true;
		spkgpio.gpio = item.gpio.gpio;
		/*request pa gpio*/
		ret = gpio_request(spkgpio.gpio, NULL);
		if (0 != ret) {
			pr_err("request gpio failed!\n");
		}
		/*
		* config gpio info of audio_pa_ctrl, the default pa config is close(check pa sys_config1.fex)
		*/
		gpio_direction_output(spkgpio.gpio, 1);
		gpio_set_value(spkgpio.gpio, 0);
	}
	type = script_get_item("ac100_audio0", "aif2_lrck_div", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] aif2_lrck_div type err, use default aif2 lrck div 256!\n");
		aif2_lrck_div = 256;
	} else
		aif2_lrck_div = val.val;

	type = script_get_item("ac100_audio0", "aif2_bclk_div", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[CODEC] aif2_lrck_div type err, use default aif2 bclk div 12!\n");
		aif2_bclk_div = 12;
	} else
		aif2_bclk_div = val.val;

#ifndef CONFIG_SUNXI_SWITCH_GPIO
	/*
	* map the virq of gpio
	* headphone gpio irq pin is ***
	* item_eint.gpio.gpio = ****;
	*/
#ifdef CONFIG_ARCH_SUN8IW6
	ac100->virq = gpio_to_irq(item_eint.gpio.gpio);
	if (IS_ERR_VALUE(ac100->virq)) {
		pr_warn("[AC100] map gpio to virq failed, errno = %d\n",ac100->virq);
		return -EINVAL;
	}

	pr_debug("[AC100] gpio [%d] map to virq [%d] ok\n", item_eint.gpio.gpio, ac100->virq);
	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(codec->dev, ac100->virq, audio_hmic_irq, IRQF_TRIGGER_FALLING, "SWTICH_EINT", ac100);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("[AC100] request virq %d failed, errno = %d\n", ac100->virq, ret);
	        return -EINVAL;
	}

	/*
	* item_eint.gpio.gpio = GPIO*(*);
	* select HOSC 24Mhz(PIO Interrupt Clock Select)
	*/
	req_status = gpio_request(item_eint.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_warn("[AC100]request gpio[%d] failed!\n", item_eint.gpio.gpio);
		return -EINVAL;
	}
	gpio_set_debounce(item_eint.gpio.gpio, 1);

#endif
#endif
	ac100->num_supplies = ARRAY_SIZE(ac100_supplies);
	ac100->supplies = devm_kzalloc(ac100->codec->dev,
						sizeof(struct regulator_bulk_data) *
						ac100->num_supplies, GFP_KERNEL);
	if (!ac100->supplies) {
		pr_err("[AC100] Failed to get mem.\n");
		return -ENOMEM;
	}
	for (i = 0; i < ARRAY_SIZE(ac100_supplies); i++)
		ac100->supplies[i].supply = ac100_supplies[i];

	ret = regulator_bulk_get(NULL, ac100->num_supplies,
					 ac100->supplies);
	if (ret != 0) {
		pr_err("[AC100] Failed to get supplies: %d\n", ret);
	}

	for (i = 0; i < ARRAY_SIZE(ac100_supplies); i++){
		ret = regulator_enable(ac100->supplies[i].consumer);

		if (0 != ret) {
		pr_err("[AC100] %s: some error happen, fail to enable regulator!\n", __func__);
		}
	}
	get_configuration();
	set_configuration(ac100->codec);

	/*enable this bit to prevent leakage from ldoin*/
	snd_soc_update_bits(codec, ADDA_TUNE3, (0x1<<OSCEN), (0x1<<OSCEN));
	snd_soc_write(codec, DAC_VOL_CTRL, 0);
	ret = snd_soc_add_codec_controls(codec, ac100_controls,
					ARRAY_SIZE(ac100_controls));
	if (ret) {
		pr_err("[AC100] Failed to register audio mode control, "
				"will continue without it.\n");
	}

	snd_soc_dapm_new_controls(dapm, ac100_dapm_widgets, ARRAY_SIZE(ac100_dapm_widgets));
 	snd_soc_dapm_add_routes(dapm, ac100_dapm_routes, ARRAY_SIZE(ac100_dapm_routes));
	return 0;
#ifndef CONFIG_SUNXI_SWITCH_GPIO
err_switch_work_queue:
	devm_free_irq(codec->dev,ac100->virq,NULL);

err_input_register_device:
	if(ac100->key){
		input_free_device(ac100->key);
	}

err_input_allocate_device:
	switch_dev_unregister(&ac100->sdev);

err_switch_dev_register:
	kfree(ac100);
#endif
	return ret;
}

/* power down chip */
static int ac100_codec_remove(struct snd_soc_codec *codec)
{
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	struct device *dev = codec->dev;
	int i = 0;
	int ret = 0;

	devm_free_irq(dev,ac100->virq,NULL);
	if (ac100->key) {
		input_unregister_device(ac100->key);
		input_free_device(ac100->key);
   	}
 	switch_dev_unregister(&ac100->sdev);
	for (i = 0; i < ARRAY_SIZE(ac100_supplies); i++){
		ret = regulator_disable(ac100->supplies[i].consumer);

		if (0 != ret) {
		pr_err("[AC100] %s: some error happen, fail to disable regulator!\n", __func__);
		}
		regulator_put(ac100->supplies[i].consumer);
	}

	kfree(ac100);
	return 0;
}

static int ac100_codec_suspend(struct snd_soc_codec *codec)
{
	int i ,ret =0;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	unsigned long      config;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	AC100_DBG("[codec]:suspend\n");
	/* check if called in talking standby */
	if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
		pr_err("In talking standby, audio codec do not suspend!!\n");
		return 0;
	}
	ac100_set_bias_level(codec, SND_SOC_BIAS_OFF);
	for (i = 0; i < ARRAY_SIZE(ac100_supplies); i++){
		ret = regulator_disable(ac100->supplies[i].consumer);

		if (0 != ret) {
		pr_err("[AC100] %s: some error happen, fail to disable regulator!\n", __func__);
		}
	}
	#ifndef CONFIG_SUNXI_SWITCH_GPIO
	devm_free_irq(codec->dev,ac100->virq,ac100);
	sunxi_gpio_to_name(item_eint.gpio.gpio, pin_name);
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
	pin_config_set(SUNXI_PINCTRL, pin_name, config);
	#endif
	if (spkgpio.used) {
		sunxi_gpio_to_name(spkgpio.gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
	}
	return 0;
}

static int ac100_codec_resume(struct snd_soc_codec *codec)
{
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	AC100_DBG("[codec]:resume");
	#ifndef CONFIG_SUNXI_SWITCH_GPIO
	ac100->mode = HEADPHONE_IDLE;
	headphone_state = 0;
	ac100->state	= -1;
	#endif
	/*process for normal standby*/
	#if 0
	if (NORMAL_STANDBY == standby_type) {
	/*process for super standby*/
	} else if(SUPER_STANDBY == standby_type) {
		schedule_work(&ac100->codec_resume);
	}
	#endif
	//schedule_work(&ac100->codec_resume);
	ac100_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	schedule_work(&ac100->codec_resume);
	return 0;
}

static unsigned int sndvir_audio_read(struct snd_soc_codec *codec,
					  unsigned int reg)
{
	unsigned int data;
    struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
    struct ac100 *ac100_dev = ac100->ac100;

	/* Device I/O API */
	data = ac100_reg_read(ac100_dev, reg);

	return data;
}

static int sndvir_audio_write(struct snd_soc_codec *codec,
				  unsigned int reg, unsigned int value)
{
	int ret = 0;
	struct ac100_priv *ac100 = snd_soc_codec_get_drvdata(codec);
	struct ac100 *ac100_dev = ac100->ac100;
	ret = ac100_reg_write(ac100_dev, reg, value);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_sndvir_audio = {
	.probe 		=	ac100_codec_probe,
	.remove 	=   ac100_codec_remove,
	.suspend 	= 	ac100_codec_suspend,
	.resume 	=	ac100_codec_resume,
	.set_bias_level = ac100_set_bias_level,
	.read 		= 	sndvir_audio_read,
	.write 		= 	sndvir_audio_write,
	.ignore_pmdown_time = 1,
};

static int __devinit ac100_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct ac100_priv *ac100;
#ifdef CONFIG_ARCH_SUN8IW7
	int reg_val;

	/*PLL_PERIPH1*/
	writel(0x89010000,0xf1c00090);
	writel(0xa707000f,0xf1f01444);
	writel(0xa707000f,0xf1f01444);

	reg_val = readl(0xf1c20804);
	reg_val &=~(0x7<<8);
	reg_val |= (0x1<<8);
	writel(reg_val, 0xf1c20804);
	reg_val = readl(0xf1c20810);
	reg_val |= (0x1<<10);
	writel(reg_val, 0xf1c20810);
#endif
	pr_debug("%s,line:%d\n", __func__, __LINE__);

	ac100 = devm_kzalloc(&pdev->dev, sizeof(struct ac100_priv), GFP_KERNEL);
	if (ac100 == NULL) {
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, ac100);

	ac100->ac100 = dev_get_drvdata(pdev->dev.parent);

	ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sndvir_audio, ac100_dai, ARRAY_SIZE(ac100_dai));
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register ac100: %d\n", ret);
	}
	ret = sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
	if (ret) {
		pr_err("failed to create attr group\n");
	}
	return 0;
}
static void ac100_shutdown(struct platform_device *pdev)
{
	int reg_val;
	struct ac100_priv *ac100 = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = ac100->codec;
	/*set headphone volume to 0*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~(0x3f<<HP_VOL);
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	/*disable pa*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~(0x1<<HPPA_EN);
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	/*hardware xzh support*/
	reg_val = snd_soc_read(codec, OMIXER_DACA_CTRL);
	reg_val &= ~(0xf<<HPOUTPUTENABLE);
	snd_soc_write(codec, OMIXER_DACA_CTRL, reg_val);

	/*unmute l/r headphone pa*/
	reg_val = snd_soc_read(codec, HPOUT_CTRL);
	reg_val &= ~((0x1<<RHPPA_MUTE)|(0x1<<LHPPA_MUTE));
	snd_soc_write(codec, HPOUT_CTRL, reg_val);

	/*disable pa_ctrl*/
	if (spkgpio.used)
		gpio_set_value(spkgpio.gpio, 0);

}
static int __devexit ac100_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &audio_debug_attr_group);
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}
static struct platform_driver ac100_codec_driver = {
	.driver = {
		.name = "ac100-codec",
		.owner = THIS_MODULE,
	},
	.probe = ac100_probe,
	.remove = __devexit_p(ac100_remove),
	.shutdown = ac100_shutdown,
};
module_platform_driver(ac100_codec_driver);

MODULE_DESCRIPTION("ASoC AC100 driver");
MODULE_AUTHOR("huangxin");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ac100-codec");

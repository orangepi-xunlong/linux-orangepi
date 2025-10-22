// SPDX-License-Identifier: GPL-2.0-or-later
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

#define SUNXI_MODNAME		"sound-codec"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "snd_sunxi_pcm.h"
#include "snd_sunxi_common.h"
#include "snd_sun8iw11_codec.h"

#define	DRV_NAME	"sunxi-snd-codec"

static struct audio_reg_label sunxi_reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_FIFO_CTR),
	REG_LABEL(SUNXI_DAC_FIFO_STA),
	REG_LABEL(SUNXI_ADC_FIFO_CTR),
	REG_LABEL(SUNXI_ADC_FIFO_STA),
	/* REG_LABEL(SUNXI_ADC_RXDATA), */
	/* REG_LABEL(SUNXI_DAC_TXDATA), */
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_ADC_CNT),
	REG_LABEL(SUNXI_DAC_DG),
	REG_LABEL(SUNXI_ADC_DG),
	REG_LABEL(SUNXI_HMIC_CTRL),
	REG_LABEL(SUNXI_HMIC_DATA),

	REG_LABEL(SUNXI_HP_VOLC),
	REG_LABEL(SUNXI_LOMIX_SRC),
	REG_LABEL(SUNXI_ROMIX_SRC),
	REG_LABEL(SUNXI_DAC_PA_SRC),
	REG_LABEL(SUNXI_LINEIN_GCTR),
	REG_LABEL(SUNXI_FM_GCTR),
	REG_LABEL(SUNXI_MICIN_GCTR),
	REG_LABEL(SUNXI_PAEN_HP_CTR),
	REG_LABEL(SUNXI_PHONEOUT_CTR),
	REG_LABEL(SUNXI_MIC2G_LINEEN_CTR),
	REG_LABEL(SUNXI_MIC1G_MICBIAS_CTR),
	REG_LABEL(SUNXI_LADCMIX_SRC),
	REG_LABEL(SUNXI_RADCMIX_SRC),

	REG_LABEL(SUNXI_PA_POP_CTR),
	REG_LABEL(SUNXI_ADC_AP_EN),
	REG_LABEL(SUNXI_ADDA_APT0),
	REG_LABEL(SUNXI_ADDA_APT1),
	REG_LABEL(SUNXI_ADDA_APT2),
	REG_LABEL(SUNXI_CHOP_CAL_CTR),
	REG_LABEL(SUNXI_BIAS_DA16_CAL_CTR),
	REG_LABEL(SUNXI_DA16_CALI_DATA),
	REG_LABEL(SUNXI_BIAS_CALI_DATA),
	REG_LABEL(SUNXI_BIAS_CALI_SET),
};
static struct audio_reg_group sunxi_reg_group = REG_GROUP(sunxi_reg_labels);

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

static const struct sample_rate sunxi_sample_rate_conv[] = {
	{44100, 0},
	{48000, 0},
	{8000, 5},
	{32000, 1},
	{22050, 2},
	{24000, 2},
	{16000, 3},
	{11025, 4},
	{12000, 4},
	{192000, 6},
	{96000, 7},
};

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_bus_disable(struct sunxi_codec_clk *clk);

static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_exit(struct sunxi_codec_rglt *rglt);
static int snd_sunxi_rglt_enable(struct sunxi_codec_rglt *rglt);
static void snd_sunxi_rglt_disable(struct sunxi_codec_rglt *rglt);

static const DECLARE_TLV_DB_SCALE(digital_tlv, -7308, 116, 0);
static const DECLARE_TLV_DB_SCALE(headphone_tlv, -6300, 100, 1);
static const DECLARE_TLV_DB_SCALE(linein_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(fm_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic_gain_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(phoneout_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(adc_gain_tlv, -450, 150, 0);
static const unsigned int sunxi_mic_boost_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 0, TLV_DB_SCALE_ITEM(0, 0, 0),
	1, 7, TLV_DB_SCALE_ITEM(2400, 300, 0),
};

static unsigned int sunxi_regmap_read_prcm(struct regmap *regmap, unsigned int reg)
{
	unsigned int addr;
	unsigned int reg_val;
	unsigned int write_val = 0x10000000;

	SND_LOG_DEBUG("\n");

	addr = reg - SUNXI_PR_CFG;
	write_val |= (addr<<16);

	regmap_write(regmap, SUNXI_PR_CFG, write_val);
	regmap_read(regmap, SUNXI_PR_CFG, &reg_val);

	reg_val &= 0xff;

	return reg_val;
}

static void sunxi_regmap_write_prcm(struct regmap *regmap, unsigned int reg, unsigned int val)
{
	unsigned int addr;
	unsigned int write_val = 0x11000000;

	SND_LOG_DEBUG("\n");

	addr = reg - SUNXI_PR_CFG;
	val &= 0xff;

	write_val |= (addr<<16);
	write_val |= (val<<8);
	regmap_write(regmap, SUNXI_PR_CFG, write_val);

	write_val &= ~(0x1<<24);
	regmap_write(regmap, SUNXI_PR_CFG, write_val);
}

static void sunxi_regmap_update_prcm_bits(struct regmap *regmap, unsigned int reg,
					  unsigned int mask, unsigned int val)
{
	unsigned int old, new;

	SND_LOG_DEBUG("\n");

	old = sunxi_regmap_read_prcm(regmap, reg);
	new = (old & ~mask) | val;

	sunxi_regmap_write_prcm(regmap, reg, new);
}

static int sunxi_codec_get_hub_mode(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DPC, &reg_val);
	ucontrol->value.integer.value[0] = ((reg_val & (1<<HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_hub_mode(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, (0x1<<HUB_EN), (0x0<<HUB_EN));
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, (0x1<<HUB_EN), (0x1<<HUB_EN));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_get_hpl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	reg_val = sunxi_regmap_read_prcm(codec->mem.regmap, SUNXI_PAEN_HP_CTR);
	if (reg_val & (1<<RTLNMUTE)) {
		ucontrol->value.enumerated.item[0] = 2;
		return 0;
	} else {
		reg_val = sunxi_regmap_read_prcm(codec->mem.regmap, SUNXI_DAC_PA_SRC);
		ucontrol->value.enumerated.item[0] = ((reg_val & (0x1<<LHPIS)) ? 1 : 0);
	}

	return 0;
}

static int sunxi_set_hpl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.enumerated.item[0]) {
	case 0:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<RTLNMUTE, 0x0<<RTLNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<LHPPAMUTE | 0x1<<LHPIS,
					      0x1<<LHPPAMUTE | 0x0<<LHPIS);
		break;
	case 1:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<RTLNMUTE, 0x0<<RTLNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<LHPPAMUTE | 0x1<<LHPIS,
					      0x1<<LHPPAMUTE | 0x1<<LHPIS);
		break;
	case 2:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<RTLNMUTE, 0x1<<RTLNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<LHPPAMUTE, 0x0<<LHPPAMUTE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_get_hpr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	reg_val = sunxi_regmap_read_prcm(codec->mem.regmap, SUNXI_PAEN_HP_CTR);
	if (reg_val & (1<<LTRNMUTE)) {
		ucontrol->value.enumerated.item[0] = 2;
		return 0;
	} else {
		reg_val = sunxi_regmap_read_prcm(codec->mem.regmap, SUNXI_DAC_PA_SRC);
		ucontrol->value.enumerated.item[0] = (reg_val & (1<<RHPIS)) ? 1 : 0;
	}

	return 0;
}

static int sunxi_set_hpr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.enumerated.item[0]) {
	case 0:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<LTRNMUTE, 0x0<<LTRNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE | 0x1<<RHPIS,
					      0x1<<RHPPAMUTE | 0x0<<RHPIS);
		break;
	case 1:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<LTRNMUTE, 0x0<<LTRNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE | 0x1<<RHPIS,
					      0x1<<RHPPAMUTE | 0x1<<RHPIS);
		break;
	case 2:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
					      0x1<<LTRNMUTE, 0x1<<LTRNMUTE);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE, 0x0<<RHPPAMUTE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_get_dap_status(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int shift = e->shift_l;
	unsigned int reg_val;

	switch (shift) {
	case DACDRC_SHIFT:
		regmap_read(regmap, SUNXI_DAC_DAP_CTR, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << DAC_DAP_EN) && (reg_val & 0x1 << DDAP_DRC_EN) ? 1 : 0;
		break;
	case DACHPF_SHIFT:
		regmap_read(regmap, SUNXI_DAC_DAP_CTR, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << DAC_DAP_EN) && (reg_val & 0x1 << DDAP_HPF_EN) ? 1 : 0;
		break;
	case ADCDRC_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTR, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADC_DAP_EN) && (reg_val & 0x1 << ADAP_DRC_EN) ? 1 : 0;
		break;
	case ADCHPF_SHIFT:
		regmap_read(regmap, SUNXI_ADC_DAP_CTR, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << ADC_DAP_EN) && (reg_val & 0x1 << ADAP_HPF_EN) ? 1 : 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_set_dap_status(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int shift = e->shift_l;

	mutex_lock(&dts->dap_sta.dap_mutex);
	switch (shift) {
	case DACDRC_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTR,
					   0x1 << DAC_DAP_EN | 0x1 << DDAP_DRC_EN,
					   0x1 << DAC_DAP_EN | 0x1 << DDAP_DRC_EN);
			dts->dap_sta.dac_drc_en = 1;
		} else {
			if (!dts->dap_sta.dac_hpf_en)
				regmap_update_bits(regmap, SUNXI_DAC_DAP_CTR,
						   0x1 << DAC_DAP_EN, 0x0 << DAC_DAP_EN);
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTR,
					   0x1 << DDAP_DRC_EN, 0x0 << DDAP_DRC_EN);
			dts->dap_sta.dac_drc_en = 0;
		}
		break;
	case DACHPF_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTR,
					   0x1 << DAC_DAP_EN | 0x1 << DDAP_HPF_EN,
					   0x1 << DAC_DAP_EN | 0x1 << DDAP_HPF_EN);
			dts->dap_sta.dac_hpf_en = 1;
		} else {
			if (!dts->dap_sta.dac_drc_en)
				regmap_update_bits(regmap, SUNXI_DAC_DAP_CTR,
						   0x1 << DAC_DAP_EN, 0x0 << DAC_DAP_EN);
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTR,
					   0x1 << DDAP_HPF_EN, 0x0 << DDAP_HPF_EN);
			dts->dap_sta.dac_hpf_en = 0;
		}
		break;
	case ADCDRC_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTR,
					   0x1 << ADC_DAP_EN | 0x1 << ADAP_DRC_EN,
					   0x1 << ADC_DAP_EN | 0x1 << ADAP_DRC_EN);
			dts->dap_sta.adc_drc_en = 1;
		} else {
			if (!dts->dap_sta.adc_hpf_en)
				regmap_update_bits(regmap, SUNXI_ADC_DAP_CTR,
						   0x1 << ADC_DAP_EN, 0x0 << ADC_DAP_EN);
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTR,
					   0x1 << ADAP_DRC_EN, 0x0 << ADAP_DRC_EN);
			dts->dap_sta.adc_drc_en = 0;
		}
		break;
	case ADCHPF_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTR,
					   0x1 << ADC_DAP_EN | 0x1 << ADAP_HPF_EN,
					   0x1 << ADC_DAP_EN | 0x1 << ADAP_HPF_EN);
			dts->dap_sta.adc_hpf_en = 1;
		} else {
			if (!dts->dap_sta.adc_drc_en)
				regmap_update_bits(regmap, SUNXI_ADC_DAP_CTR,
						   0x1 << ADC_DAP_EN, 0x0 << ADC_DAP_EN);
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTR,
					   0x1 << ADAP_HPF_EN, 0x0 << ADAP_HPF_EN);
			dts->dap_sta.adc_hpf_en = 0;
		}
		break;
	default:
		return -EINVAL;
	}
	mutex_unlock(&dts->dap_sta.dap_mutex);

	return 0;
}

/* Define Text */
static const char *sunxi_switch_text[] = {"Off", "On"};
static const char *sunxi_hpl_mux_text[] = {"DACL", "LOMIX", "HPR"};
static const char *sunxi_hpr_mux_text[] = {"DACR", "ROMIX", "HPL"};

/* Define Enum */
static SOC_ENUM_SINGLE_DECL(sunxi_dacdrc_sta_enum, SND_SOC_NOPM, DACDRC_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dachpf_sta_enum, SND_SOC_NOPM, DACHPF_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adcdrc_sta_enum, SND_SOC_NOPM, ADCDRC_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adchpf_sta_enum, SND_SOC_NOPM, ADCHPF_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_codec_hub_mode_enum, sunxi_switch_text);

static int sunxi_playback_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1<<EN_DAC, 0x1<<EN_DAC);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1<<EN_DAC, 0x0<<EN_DAC);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_capture_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(0x1<<EN_AD), (0x1<<EN_AD));
		break;
	case	SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(0x1<<EN_AD), (0x0<<EN_AD));
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC1G_MICBIAS_CTR,
					      0x1<<MIC1_AMPEN | 0x1<<MMICBIASEN,
					      0x1<<MIC1_AMPEN | 0x1<<MMICBIASEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC1G_MICBIAS_CTR,
					      0x1<<MIC1_AMPEN | 0x1<<MMICBIASEN,
					      0x0<<MIC1_AMPEN | 0x0<<MMICBIASEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic2_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC1G_MICBIAS_CTR,
					      0x1<<MMICBIASEN, 0x1<<MMICBIASEN);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC2G_LINEEN_CTR,
					      0x1<<MIC2AMPEN, 0x1<<MIC2AMPEN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC1G_MICBIAS_CTR,
					      0x1<<MMICBIASEN, 0x0<<MMICBIASEN);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC2G_LINEEN_CTR,
					      0x1<<MIC2AMPEN, 0x0<<MIC2AMPEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_hpout_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* Enable HPPALR */
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE | 0x1<<LHPPAMUTE,
					      0x1<<RHPPAMUTE | 0x1<<LHPPAMUTE);
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_PAEN_HP_CTR, 1<<HPPAEN, 1<<HPPAEN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		sunxi_regmap_update_prcm_bits(regmap, SUNXI_PAEN_HP_CTR, 1<<HPPAEN, 0<<HPPAEN);
		sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_PA_SRC,
					      0x1<<RHPPAMUTE | 0x1<<LHPPAMUTE,
					      0x0<<RHPPAMUTE | 0x0<<LHPPAMUTE);
		break;
	default:
		break;
	}
	return 0;
}

static int sunxi_spk_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_sunxi_pa_pin_enable(codec->pa_cfg, codec->pa_pin_max);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	/* DRC/HPF Switch */
	SOC_ENUM_EXT("DAC DRC Switch", sunxi_dacdrc_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("DAC HPF Switch", sunxi_dachpf_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC DRC Switch", sunxi_adcdrc_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),
	SOC_ENUM_EXT("ADC HPF Switch", sunxi_adchpf_sta_enum,
		     sunxi_codec_get_dap_status, sunxi_codec_set_dap_status),

	/* Hub Mode */
	SOC_ENUM_EXT("tx hub mode", sunxi_codec_hub_mode_enum,
		     sunxi_codec_get_hub_mode, sunxi_codec_set_hub_mode),

	/* DAC Digital Vol */
	SOC_SINGLE_TLV("DAC Volume", SUNXI_DAC_DPC, DVOL, 0x3F, 1, digital_tlv),

	/* ADC Analog Gain */
	SOC_SINGLE_TLV("ADC Gain", SUNXI_ADC_AP_EN, ADCG, 0x7, 0, adc_gain_tlv),

	/* MIC Gain */
	SOC_SINGLE_TLV("MIC1 Gain", SUNXI_MIC1G_MICBIAS_CTR, MIC1_BOOST, 0x7, 0,
		       sunxi_mic_boost_tlv),
	SOC_SINGLE_TLV("MIC2 Gain", SUNXI_MIC2G_LINEEN_CTR, MIC2BOOST, 0x7, 0,
		       sunxi_mic_boost_tlv),

	/* Input Stage to output Mixer Gain */
	SOC_SINGLE_TLV("MIC1 to OMIX Gain", SUNXI_MICIN_GCTR, MIC1_GAIN, 0x7, 0, mic_gain_tlv),
	SOC_SINGLE_TLV("MIC2 to OMIX Gain", SUNXI_MICIN_GCTR, MIC2_GAIN, 0x7, 0, mic_gain_tlv),

	SOC_SINGLE_TLV("FMIN to OMIX Gain", SUNXI_FM_GCTR, FMG, 0x7, 0, fm_tlv),
	SOC_SINGLE_TLV("LINEIN to OMIX Gain", SUNXI_FM_GCTR, LINEING, 0x7, 0, linein_tlv),

	/* LINEIN/FMIN to Output Mixer Gain */
	SOC_SINGLE_TLV("LINEINL to ROMIX Gain", SUNXI_LINEIN_GCTR, LINEINLG, 0x7, 0, linein_tlv),
	SOC_SINGLE_TLV("LINEINR to LOMIX Gain", SUNXI_LINEIN_GCTR, LINEINRG, 0x7, 0, linein_tlv),

	/* PHONEOUT Gain */
	SOC_SINGLE_TLV("PHONEOUT Gain", SUNXI_PHONEOUT_CTR, PHONEOUTG, 0x7, 0, phoneout_tlv),

	/* HPOUT Gain */
	SOC_SINGLE_TLV("HPOUT Gain", SUNXI_HP_VOLC, HPVOL, 0x3F, 0, headphone_tlv),
};

/* Define Mux Controls */
static const struct soc_enum sunxi_hpl_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, 3, sunxi_hpl_mux_text);
static const struct soc_enum sunxi_hpr_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, 3, sunxi_hpr_mux_text);

static const struct snd_kcontrol_new sunxi_hpl_mux =
	SOC_DAPM_ENUM_EXT("HPL Source", sunxi_hpl_enum, sunxi_get_hpl_src, sunxi_set_hpl_src);
static const struct snd_kcontrol_new sunxi_hpr_mux =
	SOC_DAPM_ENUM_EXT("HPR Source", sunxi_hpr_enum, sunxi_get_hpr_src, sunxi_set_hpr_src);

/* Define Mixer Controls */
static const struct snd_kcontrol_new sunxi_left_output_mixer[] = {
	SOC_DAPM_SINGLE("DACL Switch", SUNXI_LOMIX_SRC, LMIX_LDAC, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", SUNXI_LOMIX_SRC, LMIX_RDAC, 1, 0),
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_LOMIX_SRC, LMIX_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_LOMIX_SRC, LMIX_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("FMINL Switch", SUNXI_LOMIX_SRC, LMIX_FML, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", SUNXI_LOMIX_SRC, LMIX_LINEINL, 1, 0),
	SOC_DAPM_SINGLE("LINEINLR Switch", SUNXI_LOMIX_SRC, LMIX_LINEINLR, 1, 0),
};

static const struct snd_kcontrol_new sunxi_right_output_mixer[] = {
	SOC_DAPM_SINGLE("DACL Switch", SUNXI_ROMIX_SRC, RMIX_LDAC, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", SUNXI_ROMIX_SRC, RMIX_RDAC, 1, 0),
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_ROMIX_SRC, RMIX_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_ROMIX_SRC, RMIX_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("FMINR Switch", SUNXI_ROMIX_SRC, RMIX_FMR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", SUNXI_ROMIX_SRC, RMIX_LINEINR, 1, 0),
	SOC_DAPM_SINGLE("LINEINLR Switch", SUNXI_ROMIX_SRC, RMIX_LINEINLR, 1, 0),
};

static const struct snd_kcontrol_new sunxi_left_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_LADCMIX_SRC, LADC_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_LADCMIX_SRC, LADC_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("FMINL Switch", SUNXI_LADCMIX_SRC, LADC_FML, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", SUNXI_LADCMIX_SRC, LADC_LINEINL, 1, 0),
	SOC_DAPM_SINGLE("LINEINLR Switch", SUNXI_LADCMIX_SRC, LADC_LINEINLR, 1, 0),
	SOC_DAPM_SINGLE("LOMIX Switch", SUNXI_LADCMIX_SRC, LADC_LOUT_MIX, 1, 0),
	SOC_DAPM_SINGLE("ROMIX Switch", SUNXI_LADCMIX_SRC, LADC_ROUT_MIX, 1, 0),
};

static const struct snd_kcontrol_new sunxi_right_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_RADCMIX_SRC, RADC_MIC1_BST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_RADCMIX_SRC, RADC_MIC2_BST, 1, 0),
	SOC_DAPM_SINGLE("FMINR Switch", SUNXI_RADCMIX_SRC, RADC_FMR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", SUNXI_RADCMIX_SRC, RADC_LINEINR, 1, 0),
	SOC_DAPM_SINGLE("LINEINLR Switch", SUNXI_RADCMIX_SRC, RADC_LINEINLR, 1, 0),
	SOC_DAPM_SINGLE("LOMIX Switch", SUNXI_RADCMIX_SRC, RADC_LOUT_MIX, 1, 0),
	SOC_DAPM_SINGLE("ROMIX Switch", SUNXI_RADCMIX_SRC, RADC_ROUT_MIX, 1, 0),
};

static const struct snd_kcontrol_new sunxi_phoneout_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_PHONEOUT_CTR, PHONEOUTS3, 1, 0),
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_PHONEOUT_CTR, PHONEOUTS2, 1, 0),
	SOC_DAPM_SINGLE("LOMIX Switch", SUNXI_PHONEOUT_CTR, PHONEOUTS0, 1, 0),
	SOC_DAPM_SINGLE("ROMIX Switch", SUNXI_PHONEOUT_CTR, PHONEOUTS1, 1, 0),
};

static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SUNXI_DAC_PA_SRC, DACALEN, 0,
				sunxi_playback_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, SUNXI_DAC_PA_SRC, DACAREN, 0,
				sunxi_playback_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADCL", "Capture", 0, SUNXI_ADC_AP_EN, ADCLEN, 0,
				sunxi_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADCR", "Capture", 0, SUNXI_ADC_AP_EN, ADCREN, 0,
				sunxi_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER("Left Output Mixer", SUNXI_DAC_PA_SRC, LMIXEN, 0,
			   sunxi_left_output_mixer, ARRAY_SIZE(sunxi_left_output_mixer)),
	SND_SOC_DAPM_MIXER("Right Output Mixer", SUNXI_DAC_PA_SRC, RMIXEN, 0,
			   sunxi_right_output_mixer, ARRAY_SIZE(sunxi_right_output_mixer)),
	SND_SOC_DAPM_MIXER("Left Input Mixer", SND_SOC_NOPM, 0, 0,
			   sunxi_left_input_mixer, ARRAY_SIZE(sunxi_left_input_mixer)),
	SND_SOC_DAPM_MIXER("Right Input Mixer", SND_SOC_NOPM, 0, 0,
			   sunxi_right_input_mixer, ARRAY_SIZE(sunxi_right_input_mixer)),
	SND_SOC_DAPM_MIXER("PHONEOUT Mixer", SUNXI_PHONEOUT_CTR, PHONEOUTEN, 0,
			   sunxi_phoneout_mixer, ARRAY_SIZE(sunxi_phoneout_mixer)),

	SND_SOC_DAPM_MUX("HPL Source", SND_SOC_NOPM, 0, 0, &sunxi_hpl_mux),
	SND_SOC_DAPM_MUX("HPR Source", SND_SOC_NOPM, 0, 0, &sunxi_hpr_mux),

	SND_SOC_DAPM_INPUT("MIC1_PIN"),
	SND_SOC_DAPM_INPUT("MIC2_PIN"),
	SND_SOC_DAPM_INPUT("FMINL_PIN"),
	SND_SOC_DAPM_INPUT("FMINR_PIN"),
	SND_SOC_DAPM_INPUT("LINEINL_PIN"),
	SND_SOC_DAPM_INPUT("LINEINR_PIN"),

	SND_SOC_DAPM_OUTPUT("HPOUTL_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUTR_PIN"),
	SND_SOC_DAPM_OUTPUT("PHONEOUTP_PIN"),
	SND_SOC_DAPM_OUTPUT("PHONEOUTN_PIN"),

	SND_SOC_DAPM_MIC("MIC1", sunxi_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", sunxi_mic2_event),
	SND_SOC_DAPM_LINE("FMIN", NULL),
	SND_SOC_DAPM_LINE("LINEIN", NULL),
	SND_SOC_DAPM_HP("HPOUT", sunxi_hpout_event),
	SND_SOC_DAPM_HP("PHONEOUT", NULL),
	SND_SOC_DAPM_SPK("SPK", sunxi_spk_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* input path */

	/*
	 * {"MIC1_PIN", NULL, "MIC1"},
	 * {"MIC2_PIN", NULL, "MIC2"},
	 * {"LINEINL_PIN", NULL, "LINEIN"},
	 * {"LINEINR_PIN", NULL, "LINEIN"},
	 * {"FMINL_PIN", NULL, "FMIN"},
	 * {"FMINR_PIN", NULL, "FMIN"},
	 */

	{"Left Input Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"Left Input Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"Left Input Mixer", "FMINL Switch", "FMINL_PIN"},
	{"Left Input Mixer", "LINEINL Switch", "LINEINL_PIN"},
	{"Left Input Mixer", "LINEINLR Switch", "LINEINL_PIN"},
	{"Left Input Mixer", "LINEINLR Switch", "LINEINR_PIN"},
	{"Left Input Mixer", "LOMIX Switch", "Left Output Mixer"},
	{"Left Input Mixer", "ROMIX Switch", "Right Output Mixer"},

	{"Right Input Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"Right Input Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"Right Input Mixer", "FMINR Switch", "FMINR_PIN"},
	{"Right Input Mixer", "LINEINR Switch", "LINEINR_PIN"},
	{"Right Input Mixer", "LINEINLR Switch", "LINEINL_PIN"},
	{"Right Input Mixer", "LINEINLR Switch", "LINEINR_PIN"},
	{"Right Input Mixer", "LOMIX Switch", "Left Output Mixer"},
	{"Right Input Mixer", "ROMIX Switch", "Right Output Mixer"},

	{"ADCL", NULL, "Left Input Mixer"},
	{"ADCR", NULL, "Right Input Mixer"},

	/* output path */
	{"Left Output Mixer", "DACL Switch", "DACL"},
	{"Left Output Mixer", "DACR Switch", "DACR"},
	{"Left Output Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"Left Output Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"Left Output Mixer", "FMINL Switch", "FMINL_PIN"},
	{"Left Output Mixer", "LINEINL Switch", "LINEINL_PIN"},
	{"Left Output Mixer", "LINEINLR Switch", "LINEINL_PIN"},
	{"Left Output Mixer", "LINEINLR Switch", "LINEINR_PIN"},

	{"Right Output Mixer", "DACL Switch", "DACL"},
	{"Right Output Mixer", "DACR Switch", "DACR"},
	{"Right Output Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"Right Output Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"Right Output Mixer", "FMINR Switch", "FMINR_PIN"},
	{"Right Output Mixer", "LINEINR Switch", "LINEINR_PIN"},
	{"Right Output Mixer", "LINEINLR Switch", "LINEINL_PIN"},
	{"Right Output Mixer", "LINEINLR Switch", "LINEINR_PIN"},

	{"HPL Source", "DACL", "DACL"},
	{"HPL Source", "LOMIX", "Left Output Mixer"},
	{"HPL Source", "HPR", "HPR Source"},

	{"HPR Source", "DACR", "DACR"},
	{"HPR Source", "ROMIX", "Right Output Mixer"},
	{"HPR Source", "HPL", "HPL Source"},

	{"PHONEOUT Mixer", "MIC1 Switch", "MIC1_PIN"},
	{"PHONEOUT Mixer", "MIC2 Switch", "MIC2_PIN"},
	{"PHONEOUT Mixer", "LOMIX Switch", "Left Output Mixer"},
	{"PHONEOUT Mixer", "ROMIX Switch", "Right Output Mixer"},

	{"HPOUTL_PIN", NULL, "HPL Source"},
	{"HPOUTR_PIN", NULL, "HPR Source"},

	{"PHONEOUTP_PIN", NULL, "PHONEOUT Mixer"},
	{"PHONEOUTN_PIN", NULL, "PHONEOUT Mixer"},

	/*
	 * {"HPOUT", NULL, "HPOUTL_PIN"},
	 * {"HPOUT", NULL, "HPOUTR_PIN"},
	 * {"PHONEOUT", NULL, "PHONEOUTP_PIN"},
	 * {"PHONEOUT", NULL, "PHONEOUTN_PIN"},
	 * {"SPK", NULL, "HPOUTL_PIN"},
	 * {"SPK", NULL, "HPOUTR_PIN"},
	 * {"SPK", NULL, "PHONEOUTP_PIN"},
	 * {"SPK", NULL, "PHONEOUTN_PIN"},
	 */
};

static void sunxi_codec_init(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	/* Enable ADCFDT to Overcome Niose At the Beginning */
	regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR, 7 << ADCDFEN, 7 << ADCDFEN);
	/* Enable Hardware ZeroCross Overcomm Volume Update Clicks Noise */
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_ADDA_APT2,
				      1 << ZERO_CROSS_EN, 1 << ZERO_CROSS_EN);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_ADDA_APT2,
				      1 << PA_SLOPE_SELECT, 1 << PA_SLOPE_SELECT);
	/* Setting the Anti-pop Time(rise wave time) to be 393ms */
	sunxi_regmap_write_prcm(codec->mem.regmap, SUNXI_PA_POP_CTR, 0x2);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
				      3 << PA_ANTI_POP_CTRL, 2 << PA_ANTI_POP_CTRL);

	/* set default drc/hpf status */
	regmap_update_bits(regmap, SUNXI_DAC_DAP_CTR,
			   0x1 << DDAP_DRC_EN, codec->dts.dap_sta.dac_drc_en << DDAP_DRC_EN);
	regmap_update_bits(regmap, SUNXI_DAC_DAP_CTR,
			   0x1 << DDAP_HPF_EN, codec->dts.dap_sta.dac_hpf_en << DDAP_HPF_EN);
	regmap_update_bits(regmap, SUNXI_ADC_DAP_CTR,
			   0x1 << ADAP_DRC_EN, codec->dts.dap_sta.adc_drc_en << ADAP_DRC_EN);
	regmap_update_bits(regmap, SUNXI_ADC_DAP_CTR,
			   0x1 << ADAP_HPF_EN, codec->dts.dap_sta.adc_hpf_en << ADAP_HPF_EN);
	if (codec->dts.dap_sta.dac_drc_en || codec->dts.dap_sta.dac_hpf_en)
		regmap_update_bits(regmap, SUNXI_DAC_DAP_CTR, 0x1 << DAC_DAP_EN, 0x1 << DAC_DAP_EN);
	if (codec->dts.dap_sta.adc_drc_en || codec->dts.dap_sta.adc_hpf_en)
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTR, 0x1 << ADC_DAP_EN, 0x1 << ADC_DAP_EN);

	/* Set default vol/gain control value */
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_DAC_DPC,
				      0x3F << DVOL, codec->dts.dac_vol << DVOL);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_ADC_AP_EN,
				      0x7 << ADCG, codec->dts.adc_gain << ADCG);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC1G_MICBIAS_CTR,
				      0x7 << MIC1_BOOST, codec->dts.mic1_gain << MIC1_BOOST);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MIC2G_LINEEN_CTR,
				      0x7 << MIC2BOOST, codec->dts.mic2_gain << MIC2BOOST);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MICIN_GCTR,
				      0x7 << MIC1_GAIN, codec->dts.mic1_to_omix_gain << MIC1_GAIN);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_MICIN_GCTR,
				      0x7 << MIC2_GAIN, codec->dts.mic2_to_omix_gain << MIC2_GAIN);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_FM_GCTR,
				      0x7 << FMG, codec->dts.fmin_to_omix_gain << FMG);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_FM_GCTR,
				      0x7 << LINEING, codec->dts.linein_to_omix_gain << LINEING);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_LINEIN_GCTR, 0x7 << LINEINLG,
				      codec->dts.lineinl_to_romix_gain << LINEINLG);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_LINEIN_GCTR, 0x7 << LINEINRG,
				      codec->dts.lineinr_to_lomix_gain << LINEINRG);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PHONEOUT_CTR,
				      0x7 << PHONEOUTG, codec->dts.phoneout_gain << PHONEOUTG);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_HP_VOLC,
				      0x3F << HPVOL, codec->dts.hpout_gain << HPVOL);

	/* Set HPCOM */
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
				      0x3 << HPCOM_FC, 0x3 << HPCOM_FC);
	sunxi_regmap_update_prcm_bits(codec->mem.regmap, SUNXI_PAEN_HP_CTR,
				      0x1 << HPCOM_PT, 0x1 << HPCOM_PT);
}

static int sunxi_codec_dai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	int i = 0;

	SND_LOG_DEBUG("\n");

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(3<<FIFO_MODE), (3<<FIFO_MODE));
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
				(1<<TX_SAMPLE_BITS), (0<<TX_SAMPLE_BITS));
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<RX_FIFO_MODE), (1<<RX_FIFO_MODE));
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<RX_SAMPLE_BITS), (0<<RX_SAMPLE_BITS));
		}
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(3<<FIFO_MODE), (0<<FIFO_MODE));
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
				(1<<TX_SAMPLE_BITS), (1<<TX_SAMPLE_BITS));
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<RX_FIFO_MODE), (0<<RX_FIFO_MODE));
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<RX_SAMPLE_BITS), (1<<RX_SAMPLE_BITS));
		}
		break;
	default:
		break;
	}

	for (i = 0; i < ARRAY_SIZE(sunxi_sample_rate_conv); i++) {
		if (sunxi_sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(0x7<<DAC_FS),
					(sunxi_sample_rate_conv[i].rate_bit<<DAC_FS));
			} else {
				if (sunxi_sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(0x7<<ADC_FS),
					(sunxi_sample_rate_conv[i].rate_bit<<ADC_FS));
			}
		}
	}

	if (params_channels(params) == 1) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(1<<DAC_MONO_EN), 1<<DAC_MONO_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(1<<ADC_MONO_EN), (1<<ADC_MONO_EN));
		}
	} else {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(1<<DAC_MONO_EN), (0<<DAC_MONO_EN));
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(1<<ADC_MONO_EN), (0<<ADC_MONO_EN));
		}
	}

	/* enable clk after set clk rate */
	if (snd_sunxi_clk_enable(&codec->clk)) {
		SND_LOG_ERR("clk enable failed\n");
		return -EINVAL;
	} else {
		codec->clk_sta = SND_SUNXI_CLK_OPEN;
	}

	return 0;
}

static int sunxi_codec_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_clk *clk = &codec->clk;

	SND_LOG_DEBUG("\n");

	/* prevent the closed clks from being closed again */
	if (codec->clk_sta == SND_SUNXI_CLK_OPEN) {
		snd_sunxi_clk_disable(clk);
		codec->clk_sta = SND_SUNXI_CLK_CLOSE;
	}

	return 0;
}

static int sunxi_codec_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
					    unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_clk *clk = &codec->clk;

	SND_LOG_DEBUG("stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out);

	if (pll_id == SNDRV_PCM_STREAM_PLAYBACK)
		goto playback;
	else
		goto capture;

playback:
	if (clk_set_rate(clk->clk_pll_audio, freq_in)) {
		SND_LOG_ERR("clk pllaudio set rate failed\n");
		return -EINVAL;
	}

	return 0;

capture:
	if (clk_set_rate(clk->clk_pll_audio, freq_in)) {
		SND_LOG_ERR("clk pllaudio set rate failed\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_dai_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
				(1<<FIFO_FLUSH), (1<<FIFO_FLUSH));
		regmap_write(regmap, SUNXI_DAC_FIFO_STA,
				(1<<TXE_INT|1<<TXU_INT|1<<TXO_INT));
		regmap_write(regmap, SUNXI_DAC_CNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
				(1<<FIFO_FLUSH), (1<<FIFO_FLUSH));
		regmap_write(regmap, SUNXI_ADC_FIFO_STA,
				(1<<RXA_INT|1<<RXO_INT));
		regmap_write(regmap, SUNXI_ADC_CNT, 0);
	}
	return 0;
}

static int sunxi_codec_dai_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(1<<DAC_DRQ_EN), (1<<DAC_DRQ_EN));
		else
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(1<<ADC_DRQ_EN), (1<<ADC_DRQ_EN));
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTR,
					(1<<DAC_DRQ_EN), (0<<DAC_DRQ_EN));
		else
			regmap_update_bits(regmap, SUNXI_ADC_FIFO_CTR,
					(1<<ADC_DRQ_EN), (0<<ADC_DRQ_EN));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.set_pll	= sunxi_codec_dai_set_pll,
	.hw_params	= sunxi_codec_dai_hw_params,
	.prepare	= sunxi_codec_dai_prepare,
	.trigger	= sunxi_codec_dai_trigger,
	.hw_free	= sunxi_codec_dai_hw_free,
};

static struct snd_soc_dai_driver sunxi_codec_dai = {
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates	= SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &sunxi_codec_dai_ops,
};

static int sunxi_codec_component_probe(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	int ret;

	SND_LOG_DEBUG("\n");

	/* component kcontrols -> pa */
	ret = snd_sunxi_pa_pin_probe(codec->pa_cfg, codec->pa_pin_max, component);
	if (ret)
		SND_LOG_ERR("register pa kcontrols failed\n");

	sunxi_codec_init(component);
	return 0;
}

static void sunxi_codec_component_remove(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	snd_sunxi_pa_pin_remove(codec->pa_cfg, codec->pa_pin_max);
}

static int sunxi_codec_component_suspend(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG("\n");

	pr_debug("Enter %s\n", __func__);

	snd_sunxi_save_reg(regmap, &sunxi_reg_group);
	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
	snd_sunxi_rglt_disable(&codec->rglt);
	snd_sunxi_clk_bus_disable(&codec->clk);

	pr_debug("End %s\n", __func__);

	return 0;
}

static int sunxi_codec_component_resume(struct snd_soc_component *component)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	int ret;

	pr_debug("Enter %s\n", __func__);

	snd_sunxi_rglt_enable(&codec->rglt);
	ret = snd_sunxi_clk_bus_enable(&codec->clk);
	if (ret) {
		SND_LOG_ERR("clk enable failed\n");
		return ret;
	}

	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);

	sunxi_codec_init(component);
	snd_sunxi_echo_reg(regmap, &sunxi_reg_group);
	pr_debug("End %s\n", __func__);

	return 0;
}

static unsigned int sunxi_codec_component_read(struct snd_soc_component *component,
					       unsigned int reg)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	SND_LOG_DEBUG("\n");

	if (reg >= SUNXI_PR_CFG)
		return sunxi_regmap_read_prcm(codec->mem.regmap, reg);
	else
		regmap_read(codec->mem.regmap, reg, &reg_val);

	return reg_val;
}

static int sunxi_codec_component_write(struct snd_soc_component *component,
				       unsigned int reg, unsigned int val)
{
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	if (reg >= SUNXI_PR_CFG)
		sunxi_regmap_write_prcm(codec->mem.regmap, reg, val);
	else
		regmap_write(codec->mem.regmap, reg, val);

	return 0;
}

static struct snd_soc_component_driver sunxi_codec_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_codec_component_probe,
	.remove		= sunxi_codec_component_remove,
	.suspend	= sunxi_codec_component_suspend,
	.resume		= sunxi_codec_component_resume,
	.read		= sunxi_codec_component_read,
	.write		= sunxi_codec_component_write,
	.controls		= sunxi_codec_controls,
	.num_controls		= ARRAY_SIZE(sunxi_codec_controls),
	.dapm_widgets		= sunxi_codec_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(sunxi_codec_dapm_widgets),
	.dapm_routes		= sunxi_codec_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(sunxi_codec_dapm_routes),
};

/*******************************************************************************
 * *** kernel source ***
 * @1 regmap
 * @2 clk
 * @3 rglt
 * @4 pa pin
 * @5 dts params
 * @6 sysfs
 ******************************************************************************/
static struct regmap_config sunxi_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_AUDIO_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_codec_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	ret = of_address_to_resource(np, 0, &mem->res);
	if (ret) {
		SND_LOG_ERR("parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res),
						 DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR("memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev, mem->memregion->start,
				    resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR("ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &sunxi_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR("regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_codec_mem *mem)
{
	SND_LOG_DEBUG("\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_codec_clk *clk)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* get rst clk */
	clk->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk->clk_rst)) {
		SND_LOG_ERR("clk rst get failed\n");
		ret =  PTR_ERR(clk->clk_rst);
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus_audio = of_clk_get_by_name(np, "clk_bus_audio");
	if (IS_ERR_OR_NULL(clk->clk_bus_audio)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus_audio);
		goto err_get_clk_bus;
	}

	clk->clk_pll_audio = of_clk_get_by_name(np, "clk_pll_audio");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio)) {
		SND_LOG_ERR("clk pll get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio);
		goto err_get_clk_pll;
	}

	clk->clk_module_audio = of_clk_get_by_name(np, "clk_ac_digital_gate");
	if (IS_ERR_OR_NULL(clk->clk_module_audio)) {
		SND_LOG_ERR("clk module get failed\n");
		ret = PTR_ERR(clk->clk_module_audio);
		goto err_get_clk_module;
	}

	if (clk_set_parent(clk->clk_module_audio, clk->clk_pll_audio)) {
		SND_LOG_ERR("set parent of clk_pll_audio to clk_module_audio failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	return 0;

err_set_parent:
	clk_put(clk->clk_module_audio);
err_get_clk_module:
	clk_put(clk->clk_pll_audio);
err_get_clk_pll:
	clk_put(clk->clk_bus_audio);
err_get_clk_bus:
err_get_clk_rst:
	return ret;
}

static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_module_audio);
	clk_put(clk->clk_pll_audio);
	clk_put(clk->clk_bus_audio);
}

static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (clk_prepare_enable(clk->clk_pll_audio)) {
		SND_LOG_ERR("clk pll enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_pll;
	}

	if (clk_prepare_enable(clk->clk_module_audio)) {
		SND_LOG_ERR("clk module enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_module;
	}

	return 0;

err_enable_clk_module:
	clk_disable_unprepare(clk->clk_pll_audio);
err_enable_clk_pll:
	return ret;
}

static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR("clk rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus_audio)) {
		SND_LOG_ERR("clk bus enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_bus;
	}

	return 0;

err_enable_clk_bus:
	reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return ret;
}

static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_module_audio);
	clk_disable_unprepare(clk->clk_pll_audio);
}

static void snd_sunxi_clk_bus_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_bus_audio);
	reset_control_assert(clk->clk_rst);
}

static int snd_sunxi_rglt_init(struct platform_device *pdev, struct sunxi_codec_rglt *rglt)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	rglt->avcc_external = of_property_read_bool(np, "avcc-external");
	ret = of_property_read_u32(np, "avcc-vol", &temp_val);
	if (ret < 0)
		rglt->avcc_vol = 1800000;	/* default avcc voltage: 1.8v */
	else
		rglt->avcc_vol = temp_val;

	rglt->dvcc_external = of_property_read_bool(np, "dvcc-external");
	ret = of_property_read_u32(np, "dvcc-vol", &temp_val);
	if (ret < 0)
		rglt->dvcc_vol = 1800000;	/* default dvcc voltage: 1.8v */
	else
		rglt->dvcc_vol = temp_val;

	if (rglt->avcc_external) {
		SND_LOG_DEBUG("use external avcc\n");
		rglt->avcc = regulator_get(&pdev->dev, "avcc");
		if (IS_ERR_OR_NULL(rglt->avcc)) {
			SND_LOG_DEBUG("unused external pmu\n");
		} else {
			ret = regulator_set_voltage(rglt->avcc, rglt->avcc_vol, rglt->avcc_vol);
			if (ret < 0) {
				SND_LOG_ERR("set avcc voltage failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
			ret = regulator_enable(rglt->avcc);
			if (ret < 0) {
				SND_LOG_ERR("enable avcc failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
		}
	} else {
		SND_LOG_ERR("unsupport internal avcc\n");
		ret = -EFAULT;
		goto err_rglt;
	}

	if (rglt->dvcc_external) {
		SND_LOG_DEBUG("use external dvcc\n");
		rglt->dvcc = regulator_get(&pdev->dev, "dvcc");
		if (IS_ERR_OR_NULL(rglt->dvcc)) {
			SND_LOG_DEBUG("unused external pmu\n");
		} else {
			ret = regulator_set_voltage(rglt->dvcc, rglt->dvcc_vol, rglt->dvcc_vol);
			if (ret < 0) {
				SND_LOG_ERR("set dvcc voltage failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
			ret = regulator_enable(rglt->dvcc);
			if (ret < 0) {
				SND_LOG_ERR("enable dvcc failed\n");
				ret = -EFAULT;
				goto err_rglt;
			}
		}
	} else {
		SND_LOG_ERR("unsupport internal cpvdd for headphone charge pump\n");
		ret = -EFAULT;
		goto err_rglt;
	}

	return 0;

err_rglt:
	snd_sunxi_rglt_exit(rglt);
	return ret;
}

static void snd_sunxi_rglt_exit(struct sunxi_codec_rglt *rglt)
{
	SND_LOG_DEBUG("\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			regulator_disable(rglt->avcc);
			regulator_put(rglt->avcc);
		}

	if (rglt->dvcc)
		if (!IS_ERR_OR_NULL(rglt->dvcc)) {
			regulator_disable(rglt->dvcc);
			regulator_put(rglt->dvcc);
		}
}

static int snd_sunxi_rglt_enable(struct sunxi_codec_rglt *rglt)
{
	int ret;

	SND_LOG_DEBUG("\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			ret = regulator_enable(rglt->avcc);
			if (ret) {
				SND_LOG_ERR("enable avcc failed\n");
				return -1;
			}
		}

	if (rglt->dvcc)
		if (!IS_ERR_OR_NULL(rglt->dvcc)) {
			ret = regulator_enable(rglt->dvcc);
			if (ret) {
				SND_LOG_ERR("enable dvcc failed\n");
				return -1;
			}
		}

	return 0;
}

static void snd_sunxi_rglt_disable(struct sunxi_codec_rglt *rglt)
{
	SND_LOG_DEBUG("\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc))
			regulator_disable(rglt->avcc);

	if (rglt->dvcc)
		if (!IS_ERR_OR_NULL(rglt->dvcc))
			regulator_disable(rglt->dvcc);
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_codec_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

	/* drc/hpf en */
	dts->dap_sta.dac_drc_en = of_property_read_bool(np, "dac-drc-en");
	dts->dap_sta.dac_hpf_en = of_property_read_bool(np, "dac-hpf-en");
	dts->dap_sta.adc_drc_en = of_property_read_bool(np, "adc-drc-en");
	dts->dap_sta.adc_hpf_en = of_property_read_bool(np, "adc-hpf-en");


	ret = of_property_read_u32(np, "dac-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("dac volume get failed\n");
		dts->dac_vol = 63;
	} else {
		dts->dac_vol = temp_val;
	}

	ret = of_property_read_u32(np, "adc-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("adc gain get failed\n");
		dts->adc_gain = 3;
	} else {
		dts->adc_gain = temp_val;
	}

	ret = of_property_read_u32(np, "mic1-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("mic1 gain get failed\n");
		dts->mic1_gain = 4;
	} else {
		dts->mic1_gain = temp_val;
	}

	ret = of_property_read_u32(np, "mic2-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("mic2 gain get failed\n");
		dts->mic2_gain = 4;
	} else {
		dts->mic2_gain = temp_val;
	}

	ret = of_property_read_u32(np, "mic1-to-omix-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("mic1 to omix gain get failed\n");
		dts->mic1_to_omix_gain = 3;
	} else {
		dts->mic1_to_omix_gain = temp_val;
	}

	ret = of_property_read_u32(np, "mic2-to-omix-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("mic2 to omix gain get failed\n");
		dts->mic2_to_omix_gain = 3;
	} else {
		dts->mic2_to_omix_gain = temp_val;
	}

	ret = of_property_read_u32(np, "fmin-to-omix-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("fmin to omix gain get failed\n");
		dts->fmin_to_omix_gain = 3;
	} else {
		dts->fmin_to_omix_gain = temp_val;
	}

	ret = of_property_read_u32(np, "linein-to-omix-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("linein to omix gain get failed\n");
		dts->linein_to_omix_gain = 3;
	} else {
		dts->linein_to_omix_gain = temp_val;
	}

	ret = of_property_read_u32(np, "lineinl-to-romix-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("lineinl to romix gain get failed\n");
		dts->lineinl_to_romix_gain = 3;
	} else {
		dts->lineinl_to_romix_gain = temp_val;
	}

	ret = of_property_read_u32(np, "lineinr-to-lomix-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("lineinr to lomix gain get failed\n");
		dts->lineinr_to_lomix_gain = 3;
	} else {
		dts->lineinr_to_lomix_gain = temp_val;
	}

	ret = of_property_read_u32(np, "phoneout-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("phoneout gain get failed\n");
		dts->phoneout_gain = 3;
	} else {
		dts->phoneout_gain = temp_val;
	}

	ret = of_property_read_u32(np, "hpout-gain", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("hpout gain get failed\n");
		dts->hpout_gain = 63;
	} else {
		dts->hpout_gain = temp_val;
	}

	SND_LOG_DEBUG("dac_vol:%u, adc_gain:%u, mic1_gain:%u, mic2_gain:%u\n",
		      dts->dac_vol, dts->adc_gain, dts->mic1_gain, dts->mic2_gain);
	SND_LOG_DEBUG("mic1_to_omix_gain:%u, mic2_to_omix_gain:%u, fmin_to_omix_gain:%u\n",
		      dts->mic1_to_omix_gain, dts->mic2_to_omix_gain, dts->fmin_to_omix_gain);
	SND_LOG_DEBUG("linein_to_omix_gain:%u, lineinl_to_romix_gain:%u, "
		      "lineinr_to_lomix_gain:%u\n", dts->linein_to_omix_gain,
		      dts->lineinl_to_romix_gain, dts->lineinr_to_lomix_gain);
	SND_LOG_DEBUG("phoneout_gain:%u, hpout_gain:%u\n",
		      dts->phoneout_gain, dts->hpout_gain);
}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
/* sysfs debug */
static void snd_sunxi_dump_version(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_codec *codec = (struct sunxi_codec *)priv;

	if (!codec) {
		SND_LOG_ERR("priv to codec failed\n");
		return;
	}
	if (codec->pdev)
		if (codec->pdev->dev.driver)
			if (codec->pdev->dev.driver->owner)
				goto module_version;
	return;

module_version:
	codec->module_version = codec->pdev->dev.driver->owner->version;
	count_tmp += sprintf(buf + count_tmp, "%s\n", codec->module_version);

	*count = count_tmp;
}

static void snd_sunxi_dump_help(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;

	count_tmp += sprintf(buf + count_tmp, "1. reg read : echo {num} > dump && cat dump\n");
	count_tmp += sprintf(buf + count_tmp, "num: 0(all)\n");
	count_tmp += sprintf(buf + count_tmp, "2. reg write: echo {reg} {value} > dump\n");
	count_tmp += sprintf(buf + count_tmp, "eg. echo 0x00 0xaa > dump\n");

	*count = count_tmp;
}

static int snd_sunxi_dump_show(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_codec *codec = (struct sunxi_codec *)priv;
	int i = 0;
	unsigned int output_reg_val;
	struct regmap *regmap;

	if (!codec) {
		SND_LOG_ERR("priv to codec failed\n");
		return -1;
	}
	if (!codec->show_reg_all)
		return 0;
	else
		codec->show_reg_all = false;

	regmap = codec->mem.regmap;
	/* digital reg */
	while (sunxi_reg_labels[i].address < SUNXI_AUDIO_MAX_REG) {
		regmap_read(regmap, sunxi_reg_labels[i].address, &output_reg_val);
		count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
				     sunxi_reg_labels[i].address, output_reg_val);
		i++;
	}

	/* analog reg */
	while (sunxi_reg_labels[i].address <= SUNXI_AUDIO_MAX_REG_PR) {
		output_reg_val = sunxi_regmap_read_prcm(regmap, sunxi_reg_labels[i].address);
		count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
				     sunxi_reg_labels[i].address, output_reg_val);
		i++;
	}

	*count = count_tmp;

	return 0;
}

static int snd_sunxi_dump_store(void *priv, const char *buf, size_t count)
{
	struct sunxi_codec *codec = (struct sunxi_codec *)priv;
	int scanf_cnt;
	unsigned int input_reg_offset, input_reg_val, output_reg_val;
	struct regmap *regmap;

	if (count <= 1)	/* null or only "\n" */
		return 0;
	if (!codec) {
		SND_LOG_ERR("priv to codec failed\n");
		return -1;
	}
	regmap = codec->mem.regmap;

	if (!strcmp(buf, "0\n")) {
		codec->show_reg_all = true;
		return 0;
	}

	scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	if (scanf_cnt != 2) {
		pr_err("wrong format: %s\n", buf);
		return -1;
	}
	if (input_reg_offset > SUNXI_AUDIO_MAX_REG_PR) {
		pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_AUDIO_MAX_REG_PR);
		return -1;
	}

	if (input_reg_offset < SUNXI_AUDIO_MAX_REG) {
		/* digital reg */
		regmap_read(regmap, input_reg_offset, &output_reg_val);
		pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
		regmap_write(regmap, input_reg_offset, input_reg_val);
		regmap_read(regmap, input_reg_offset, &output_reg_val);
		pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);
	} else {
		/* analog reg */
		output_reg_val = sunxi_regmap_read_prcm(regmap, input_reg_offset);
		pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
		sunxi_regmap_write_prcm(regmap, input_reg_offset, input_reg_val);
		output_reg_val = sunxi_regmap_read_prcm(regmap, input_reg_offset);
		pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);
	}

	return 0;
}
#endif

/* Platform Driver */
static int  sunxi_codec_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec;
	struct sunxi_codec_mem *mem;
	struct sunxi_codec_clk *clk;
	struct sunxi_codec_rglt *rglt;
	struct sunxi_codec_dts *dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump;
#endif

	SND_LOG_DEBUG("\n");

	/* sunxi codec info */
	codec = devm_kzalloc(dev, sizeof(*codec), GFP_KERNEL);
	if (!codec) {
		SND_LOG_ERR("can't allocate sunxi codec memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, codec);
	mem = &codec->mem;
	clk = &codec->clk;
	rglt = &codec->rglt;
	dts = &codec->dts;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	dump = &codec->dump;
#endif
	codec->pdev = pdev;

	/* memio init */
	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR("mem init failed\n");
		ret = -ENOMEM;
		goto err_mem_init;
	}

	/* clk init */
	ret = snd_sunxi_clk_init(pdev, clk);
	if (ret) {
		SND_LOG_ERR("clk init failed\n");
		ret = -ENOMEM;
		goto err_clk_init;
	}

	/* clk_bus and clk_rst enable */
	ret = snd_sunxi_clk_bus_enable(clk);
	if (ret) {
		SND_LOG_ERR("clk_bus and clk_rst enable failed\n");
		ret = -EINVAL;
		goto err_clk_bus_enable;
	}

	/* rglt init */
	ret = snd_sunxi_rglt_init(pdev, rglt);
	if (ret) {
		SND_LOG_ERR("rglt init failed\n");
		ret = -ENOMEM;
		goto err_rglt_init;
	}

	/* dts_params init */
	snd_sunxi_dts_params_init(pdev, dts);

	/* pa_pin init */
	codec->pa_cfg = snd_sunxi_pa_pin_init(pdev, &codec->pa_pin_max);

	/* alsa component register */
	ret = snd_soc_register_component(dev, &sunxi_codec_dev, &sunxi_codec_dai, 1);
	if (ret) {
		SND_LOG_ERR("internal-codec component register failed\n");
		ret = -ENOMEM;
		goto err_register_component;
	}

	/* mutex for drc/hpf status */
	mutex_init(&dts->dap_sta.dap_mutex);

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snprintf(codec->module_name, 32, "%s", "AudioCodec");
	dump->name = codec->module_name;
	dump->priv = codec;
	dump->dump_version = snd_sunxi_dump_version;
	dump->dump_help = snd_sunxi_dump_help;
	dump->dump_show = snd_sunxi_dump_show;
	dump->dump_store = snd_sunxi_dump_store;
	ret = snd_sunxi_dump_register(dump);
	if (ret)
		SND_LOG_WARN("snd_sunxi_dump_register failed\n");
#endif

	return 0;

err_register_component:
	snd_sunxi_rglt_exit(rglt);
err_rglt_init:
err_clk_bus_enable:
	snd_sunxi_clk_exit(clk);
err_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_mem_init:
	devm_kfree(dev, codec);
err_devm_kzalloc:
	of_node_put(np);

	return ret;
}

static int sunxi_codec_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_codec *codec = dev_get_drvdata(dev);
	struct sunxi_codec_dts *dts = &codec->dts;
	struct sunxi_codec_mem *mem = &codec->mem;
	struct sunxi_codec_clk *clk = &codec->clk;
	struct sunxi_codec_rglt *rglt = &codec->rglt;

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump = &codec->dump;
#endif

	SND_LOG_DEBUG("\n");

	mutex_destroy(&dts->dap_sta.dap_mutex);
	/* remove components */
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snd_sunxi_dump_unregister(dump);
#endif

	snd_soc_unregister_component(dev);

	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_clk_bus_disable(clk);
	snd_sunxi_clk_exit(clk);
	snd_sunxi_rglt_exit(rglt);
	snd_sunxi_pa_pin_exit(codec->pa_cfg, codec->pa_pin_max);

	devm_kfree(dev, codec);
	of_node_put(pdev->dev.of_node);

	SND_LOG_DEBUG("unregister internal-codec codec success\n");

	return 0;
}

static const struct of_device_id sunxi_codec_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME,},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_codec_of_match);

static struct platform_driver sunxi_codec_driver = {
	.driver = {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = sunxi_codec_of_match,
	},
	.probe	= sunxi_codec_dev_probe,
	.remove = sunxi_codec_dev_remove,
};

int __init sunxi_codec_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_codec_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_codec_dev_exit(void)
{
	platform_driver_unregister(&sunxi_codec_driver);
}

late_initcall(sunxi_codec_dev_init);
module_exit(sunxi_codec_dev_exit);

MODULE_AUTHOR("huhaoxin@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.3");
MODULE_DESCRIPTION("sunxi soundcard codec of internal-codec");

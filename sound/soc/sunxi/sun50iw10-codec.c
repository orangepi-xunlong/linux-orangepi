/*
 * sound\soc\sunxi\sun50iw10-codec.c
 * (C) Copyright 2014-2019
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 * luguofang <luguofang@allwinnertech.com>
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
#include <linux/init.h>
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
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/reset.h>
#include <asm/dma.h>
#include <sound/pcm.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/core.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "sunxi_sound_log.h"
#include "sun50iw10-codec.h"

/* digital audio process function */
enum sunxi_hw_dap {
	DAP_HP_EN = 0x1,
	DAP_SPK_EN = 0x2,
	/* DAP_HP_EN | DAP_SPK_EN */
	DAP_HPSPK_EN = 0x3,
};

static const struct sample_rate sample_rate_conv[] = {
	{8000,   5},
	{11025,  4},
	{12000,  4},
	{16000,  3},
	{22050,  2},
	{24000,  2},
	{32000,  1},
	{44100,  0},
	{48000,  0},
	{96000,  7},
	{192000, 6},
};

static const DECLARE_TLV_DB_SCALE(digital_tlv, -7424, 116, 0);
static const DECLARE_TLV_DB_SCALE(headphone_gain_tlv, -4200, 600, 0);
static const DECLARE_TLV_DB_SCALE(mic1gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(mic2gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);
static const unsigned int lineout_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 0, 1),
	2, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

static struct reg_label reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_VOL_CTRL),
	REG_LABEL(SUNXI_DAC_FIFOC),
	REG_LABEL(SUNXI_DAC_FIFOS),
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_DAC_DG),
	REG_LABEL(SUNXI_ADC_FIFOC),
	REG_LABEL(SUNXI_ADC_VOL_CTRL),
	REG_LABEL(SUNXI_ADC_FIFOS),
	REG_LABEL(SUNXI_ADC_CNT),
	REG_LABEL(SUNXI_ADC_DG),
#ifdef SUNXI_CODEC_DAP_ENABLE
	REG_LABEL(SUNXI_DAC_DAP_CTL),
	REG_LABEL(SUNXI_ADC_DAP_CTL),
#endif
	REG_LABEL(SUNXI_ADCL_REG),
	REG_LABEL(SUNXI_ADCR_REG),
	REG_LABEL(SUNXI_DAC_REG),
	REG_LABEL(SUNXI_MICBIAS_REG),
	REG_LABEL(SUNXI_BIAS_REG),
	REG_LABEL(SUNXI_HEADPHONE_REG),
	REG_LABEL(SUNXI_HMIC_CTRL),
	REG_LABEL(SUNXI_HMIC_STS),
	REG_LABEL_END,
};

#ifdef SUNXI_CODEC_DAP_ENABLE
static void adcdrc_config(struct snd_soc_component *component)
{
	/* 0x0208: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_CTRL, 0x00BB);

	/* left peak filter attack time */
	/* 0x020C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFHAT, 0x000B);
	/* 0x0210: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFLAT, 0x77F0);

	/* right peak filter attack time */
	/* 0x0214: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFHAT, 0x000B);
	/* 0x0218: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFLAT, 0x77F0);

	/* Left peak filter release time */
	/* 0x021C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFHRT, 0x00FF);
	/* 0x0220: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LPFLRT, 0xE1F8);

	/* Right peak filter release time */
	/* 0x0224: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFHRT, 0x00FF);
	/* 0x0228: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_RPFLRT, 0xE1F8);

	/* Left RMS filter attack time */
	/* 0x022C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LRMSHAT, 0x0001);
	/* 0x0230: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LRMSLAT, 0x2BB0);

	/* Right RMS filter attack time */
	/* 0x0234: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_RRMSHAT, 0x0001);
	/* 0x0238: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_RRMSLAT, 0x2BB0);

	/* CT */
	/* 0x023C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HCT, 0x0B92);
	/* 0x0240: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LCT, 0x461C);

	/* Kc */
	/* 0x0244: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HKC, 0x0087);
	/* 0x0248: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LKC, 0x3ECB);

	/* OPC */
	/* 0x024C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HOPC, 0xFBAE);
	/* 0x0250: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LOPC, 0x765C);

	/* LT */
	/* 0x0254: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HLT, 0x05B3);
	/* 0x0258: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LLT, 0xE068);

	/* Ki */
	/* 0x025C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HKI, 0x0018);
	/* 0x0260: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LKI, 0xDAB8);

	/* OPL */
	/* 0x0264: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HOPL, 0xFEC8);
	/* 0x0268: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LOPL, 0x2E83);

	/* ET */
	/* 0x026C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HET, 0x0DF3);
	/* 0x0270: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LET, 0xBCAE);

	/* Ke */
	/* 0x0274: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HKE, 0x0345);
	/* 0x0278: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LKE, 0x5554);

	/* OPE */
	/* 0x027C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HOPE, 0xF815);
	/* 0x0280: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LOPE, 0x2E50);

	/* Kn */
	/* 0x0284: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HKN, 0x0182);
	/* 0x0288: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_LKN, 0xFA08);

	/* smooth filter attack time */
	/* 0x028C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_SFHAT, 0x0001);
	/* 0x0290: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_SFLAT, 0x7665);

	/* gain smooth filter release time */
	/* 0x0294: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_SFHRT, 0x0000);
	/* 0x0298: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_SFLRT, 0x0F04);

	/* MXG */
	/* 0x029C: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_MXGHS, 0x0352);
	/* 0x02A0: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_MXGLS, 0x69E0);

	/* MNG */
	/* 0x02A4: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_MNGHS, 0xF95B);
	/* 0x02A8: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_MNGLS, 0x2C3F);

	/* EPS */
	/* 0x02AC: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_EPSHC, 0x0002);
	/* 0x02B0: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_EPSLC, 0x5600);

	/* 0x02B4: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_OPT, 0x0000);
	/* 0x02B8: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HPFHGAIN, 0x0100);
	/* 0x02BC: */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HPFLGAIN, 0x0000);
}

static void adcdrc_enable(struct snd_soc_component *component, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_dap *adc_dap = &sunxi_codec->adc_dap;

	if (on) {
		if (adc_dap->drc_enable++ == 0) {
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DRC0_EN), (0x1 << ADC_DRC0_EN));
			if (sunxi_codec->adc_dap_enable++ == 0) {
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x1 << ADC_DAP0_EN));
			}
		}
	} else {
		if (--adc_dap->drc_enable <= 0) {
			adc_dap->drc_enable = 0;
			if (--sunxi_codec->adc_dap_enable <= 0) {
				sunxi_codec->adc_dap_enable = 0;
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x0 << ADC_DAP0_EN));
			}
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DRC0_EN), (0x0 << ADC_DRC0_EN));
		}
	}
}

static void adchpf_config(struct snd_soc_component *component)
{
	/* HPF */
	snd_soc_component_write(component, SUNXI_ADC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_ADC_DRC_LHPFC, 0xFFFAC1 & 0xFFFF);
}

static void adchpf_enable(struct snd_soc_component *component, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_dap *adc_dap = &sunxi_codec->adc_dap;

	if (on) {
		if (adc_dap->hpf_enable++ == 0) {
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_HPF0_EN), (0x1 << ADC_HPF0_EN));
			if (sunxi_codec->adc_dap_enable++ == 0) {
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x1 << ADC_DAP0_EN));
			}
		}
	} else {
		if (--adc_dap->hpf_enable <= 0) {
			adc_dap->hpf_enable = 0;
			if (--sunxi_codec->adc_dap_enable <= 0) {
				sunxi_codec->adc_dap_enable = 0;
				snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x0 << ADC_DAP0_EN));
			}
			snd_soc_component_update_bits(component, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_HPF0_EN), (0x0 << ADC_HPF0_EN));
		}
	}
}

static void dacdrc_config(struct snd_soc_component *component)
{
	/* 0x0108: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_CTRL, 0x00BB);

	/* left peak filter attack time */
	/* 0x010C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LPFHAT, 0x000B);
	/* 0x0110: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LPFLAT, 0x77F0);

	/* right peak filter attack time */
	/* 0x0114: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_RPFHAT, 0x000B);
	/* 0x0118: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_RPFLAT, 0x77F0);

	/* Left peak filter release time */
	/* 0x011C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LPFHRT, 0x00FF);
	/* 0x0120: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LPFLRT, 0xE1F8);

	/* Right peak filter release time */
	/* 0x0124: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_RPFHRT, 0x00FF);
	/* 0x0128: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_RPFLRT, 0xE1F8);

	/* Left RMS filter attack time */
	/* 0x012C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LRMSHAT, 0x0001);
	/* 0x0130: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LRMSLAT, 0x2BB0);

	/* Right RMS filter attack time */
	/* 0x0134: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_RRMSHAT, 0x0001);
	/* 0x0138: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_RRMSLAT, 0x2BB0);

	/* CT */
	/* 0x013C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HCT, 0x0B92);
	/* 0x0140: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LCT, 0x461C);

	/* Kc */
	/* 0x0144: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HKC, 0x0087);
	/* 0x0148: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LKC, 0x3ECB);

	/* OPC */
	/* 0x014C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HOPC, 0xFBAE);
	/* 0x0150: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LOPC, 0x765C);

	/* LT */
	/* 0x0154: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HLT, 0x05B3);
	/* 0x0158: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LLT, 0xE068);

	/* Ki */
	/* 0x015C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HKI, 0x0018);
	/* 0x0160: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LKI, 0xDAB8);

	/* OPL */
	/* 0x0164: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HOPL, 0xFEC8);
	/* 0x0168: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LOPL, 0x2E83);

	/* ET */
	/* 0x016C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HET, 0x0DF3);
	/* 0x0170: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LET, 0xBCAE);

	/* Ke */
	/* 0x0174: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HKE, 0x0345);
	/* 0x0178: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LKE, 0x5554);

	/* OPE */
	/* 0x017C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HOPE, 0xF815);
	/* 0x0180: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LOPE, 0x2E50);

	/* Kn */
	/* 0x0184: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HKN, 0x0182);
	/* 0x0188: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_LKN, 0xFA08);

	/* smooth filter attack time */
	/* 0x018C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_SFHAT, 0x0001);
	/* 0x0190: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_SFLAT, 0x7665);

	/* gain smooth filter release time */
	/* 0x0194: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_SFHRT, 0x0000);
	/* 0x0198: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_SFLRT, 0x0F04);

	/* MXG */
	/* 0x019C: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_MXGHS, 0x0352);
	/* 0x01A0: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_MXGLS, 0x69E0);

	/* MNG */
	/* 0x01A4: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_MNGHS, 0xF95B);
	/* 0x01A8: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_MNGLS, 0x2C3F);

	/* EPS */
	/* 0x01AC: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_EPSHC, 0x0002);
	/* 0x01B0: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_EPSLC, 0x5600);

	/* 0x01B4: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_OPT, 0x0000);
	/* 0x01B8: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HPFHGAIN, 0x0100);
	/* 0x01BC: */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HPFLGAIN, 0x0000);
}

static void dacdrc_enable(struct snd_soc_component *component, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_dap *dac_dap = &sunxi_codec->dac_dap;

	if (on) {
		if (dac_dap->drc_enable++ == 0) {
			snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_DRC_EN),
				(0x1 << DDAP_DRC_EN));

			if (sunxi_codec->dac_dap_enable++ == 0)
				snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x1 << DDAP_EN));
		}
	} else {
		if (--dac_dap->drc_enable <= 0) {
			dac_dap->drc_enable = 0;
			if (--sunxi_codec->dac_dap_enable <= 0) {
				sunxi_codec->dac_dap_enable = 0;
				snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x0 << DDAP_EN));
			}

			snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_DRC_EN),
				(0x0 << DDAP_DRC_EN));
		}
	}
}

static void dachpf_config(struct snd_soc_component *component)
{
	/* HPF */
	snd_soc_component_write(component, SUNXI_DAC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	snd_soc_component_write(component, SUNXI_DAC_DRC_LHPFC, 0xFFFAC1 & 0xFFFF);
}

static void dachpf_enable(struct snd_soc_component *component, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_dap *dac_dap = &sunxi_codec->dac_dap;

	if (on) {
		if (dac_dap->hpf_enable++ == 0) {
			snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_HPF_EN), (0x1 << DDAP_HPF_EN));

			if (sunxi_codec->dac_dap_enable++ == 0)
				snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x1 << DDAP_EN));
		}
	} else {
		if (--dac_dap->hpf_enable <= 0) {
			dac_dap->hpf_enable = 0;
			if (--sunxi_codec->dac_dap_enable <= 0) {
				sunxi_codec->dac_dap_enable = 0;
				snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x0 << DDAP_EN));
			}

			snd_soc_component_update_bits(component, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_HPF_EN),
				(0x0 << DDAP_HPF_EN));
		}
	}
}
#endif

#ifdef SUNXI_CODEC_HUB_ENABLE
static int sunxi_codec_get_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	unsigned int reg_val;

	reg_val = snd_soc_component_read32(component, SUNXI_DAC_DPC);

	ucontrol->value.integer.value[0] =
				((reg_val & (0x1 << DAC_HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
				(0x1 << DAC_HUB_EN), (0x0 << DAC_HUB_EN));
		break;
	case	1:
		snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
				(0x1 << DAC_HUB_EN), (0x1 << DAC_HUB_EN));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/* sunxi codec hub mdoe select */
static const char * const sunxi_codec_hub_function[] = {
				"hub_disable", "hub_enable"};

static const struct soc_enum sunxi_codec_hub_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sunxi_codec_hub_function),
			sunxi_codec_hub_function),
};
#endif

#ifdef SUNXI_CODEC_ADCSWAP_ENABLE
static int sunxi_codec_get_adcswap_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	unsigned int reg_val;

	reg_val = snd_soc_component_read32(component, SUNXI_ADC_DG);

	ucontrol->value.integer.value[0] =
				((reg_val & (0x1 << AD_SWP)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_adcswap_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		snd_soc_component_update_bits(component, SUNXI_ADC_DG,
				(0x1 << AD_SWP), (0x0 << AD_SWP));
		break;
	case	1:
		snd_soc_component_update_bits(component, SUNXI_ADC_DG,
				(0x1 << AD_SWP), (0x1 << AD_SWP));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* sunxi codec adc swap func */
static const char * const sunxi_codec_adcswap_function[] = {
				"Off", "On"};

static const struct soc_enum sunxi_codec_adcswap_func_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sunxi_codec_adcswap_function),
			sunxi_codec_adcswap_function),
};
#endif

static int sunxi_codec_hpspeaker_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(component, 1);
		else if (hw_cfg->dacdrc_cfg & DAP_HP_EN)
			dacdrc_enable(component, 0);

		if (hw_cfg->dachpf_cfg & DAP_SPK_EN)
			dachpf_enable(component, 1);
		else if (hw_cfg->dachpf_cfg & DAP_HP_EN)
			dachpf_enable(component, 0);
#endif
		if (spk_cfg->used) {
			gpio_direction_output(spk_cfg->spk_gpio, 1);
			gpio_set_value(spk_cfg->spk_gpio, spk_cfg->pa_level);
			/* time delay to wait spk pa work fine */
			msleep(spk_cfg->pa_msleep);
		}
		break;
	case	SND_SOC_DAPM_PRE_PMD:
		if (spk_cfg->used)
			gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(component, 0);
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dachpf_enable(component, 0);
#endif
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_headphone_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k,	int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_HP_EN)
			dacdrc_enable(component, 1);
		if (hw_cfg->dachpf_cfg & DAP_HP_EN)
			dachpf_enable(component, 1);
#endif
		/*open*/
		snd_soc_component_update_bits(component, SUNXI_HEADPHONE_REG,
				(0x1 << HPINPUTEN), (0x1 << HPINPUTEN));
		snd_soc_component_update_bits(component, SUNXI_HEADPHONE_REG,
				(0x1 << HPOUTPUTEN), (0x1 << HPOUTPUTEN));
		snd_soc_component_update_bits(component, SUNXI_HEADPHONE_REG,
				(0x1 << HPPA_EN), (0x1 << HPPA_EN));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/*close*/
		snd_soc_component_update_bits(component, SUNXI_HEADPHONE_REG,
				(0x1 << HPPA_EN), (0x0 << HPPA_EN));
		snd_soc_component_update_bits(component, SUNXI_HEADPHONE_REG,
				(0x1 << HPOUTPUTEN), (0x0 << HPOUTPUTEN));
		snd_soc_component_update_bits(component, SUNXI_HEADPHONE_REG,
				(0x1 << HPINPUTEN), (0x0 << HPINPUTEN));

#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_HP_EN)
			dacdrc_enable(component, 0);
		if (hw_cfg->dachpf_cfg & DAP_HP_EN)
			dachpf_enable(component, 0);
#endif
		break;
	}

	return 0;
}

static int sunxi_codec_lineout_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(component, 1);
		if (hw_cfg->dachpf_cfg & DAP_SPK_EN)
			dachpf_enable(component, 1);
#endif
		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << LINEOUTLEN), (0x1 << LINEOUTLEN));

		if (spk_cfg->used) {
			gpio_direction_output(spk_cfg->spk_gpio, 1);
			gpio_set_value(spk_cfg->spk_gpio, spk_cfg->pa_level);
			/* time delay to wait spk pa work fine */
			msleep(spk_cfg->pa_msleep);
		}
		break;
	case	SND_SOC_DAPM_PRE_PMD:
		if (spk_cfg->used)
			gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));

		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << LINEOUTLEN), (0x0 << LINEOUTLEN));

#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(component, 0);
		if (hw_cfg->dachpf_cfg & DAP_SPK_EN)
			dachpf_enable(component, 0);
#endif
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_playback_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);

	switch (event) {
	case	SND_SOC_DAPM_PRE_PMU:
		/* DACL to left channel LINEOUT Mute control 0:mute 1: not mute */
		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << DACLMUTE),
				(0x1 << DACLMUTE));
		snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
				(0x1<<EN_DAC), (0x1<<EN_DAC));
		msleep(30);
		break;
	case	SND_SOC_DAPM_POST_PMD:
		snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
				(0x1<<EN_DAC), (0x0<<EN_DAC));
		/* DACL to left channel LINEOUT Mute control 0:mute 1: not mute */
		snd_soc_component_update_bits(component, SUNXI_DAC_REG,
				(0x1 << DACLMUTE),
				(0x0 << DACLMUTE));
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_capture_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		/* delay 80ms to avoid the capture pop at the beginning */
		mdelay(80);
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x1 << EN_AD));
		break;
	case	SND_SOC_DAPM_POST_PMD:
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x0 << EN_AD));
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_adc_mixer_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
#ifdef SUNXI_CODEC_DAP_ENABLE
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);
	unsigned int adcctrl_val = 0;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (hw_cfg->adcdrc_cfg & DAP_HP_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADCL_REG);
			if ((adcctrl_val >> MIC1AMPEN) & 0x1)
				adcdrc_enable(component, 1);
		} else if (hw_cfg->adcdrc_cfg & DAP_SPK_EN) {
			adcctrl_val = snd_soc_component_read32(component, SUNXI_ADCR_REG);
			if ((adcctrl_val >> MIC2AMPEN) & 0x1)
				adcdrc_enable(component, 1);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		if ((hw_cfg->adcdrc_cfg & DAP_SPK_EN) ||
			(hw_cfg->adcdrc_cfg & DAP_HP_EN))
			adcdrc_enable(component, 0);
		break;
	default:
		break;
	}
#endif

	return 0;
}

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
#ifdef SUNXI_CODEC_HUB_ENABLE
	SOC_ENUM_EXT("codec hub mode", sunxi_codec_hub_mode_enum[0],
				sunxi_codec_get_hub_mode,
				sunxi_codec_set_hub_mode),
#endif
#ifdef SUNXI_CODEC_ADCSWAP_ENABLE
	SOC_ENUM_EXT("ADC Swap", sunxi_codec_adcswap_func_enum[0],
				sunxi_codec_get_adcswap_mode,
				sunxi_codec_set_adcswap_mode),
#endif
	/* Digital Volume */
	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DPC,
					DVOL, 0x3F, 1, digital_tlv),
	/* MIC1 Gain */
	SOC_SINGLE_TLV("MIC1 gain volume", SUNXI_ADCL_REG,
					ADCL_PGA_GAIN_CTRL, 0x1F, 0, mic1gain_tlv),
	/* MIC2 Gain */
	SOC_SINGLE_TLV("MIC2 gain volume", SUNXI_ADCR_REG,
					ADCR_PGA_GAIN_CTRL, 0x1F, 0, mic2gain_tlv),
	/* DAC Volume */
	SOC_SINGLE_TLV("LINEOUT volume", SUNXI_DAC_REG, LINEOUT_VOL,
			0x1F, 0, lineout_tlv),
	/* DAC Volume / HEADPHONE VOL */
	SOC_DOUBLE_TLV("DAC volume", SUNXI_DAC_VOL_CTRL, DAC_VOL_L, DAC_VOL_R,
		       0xFF, 0, dac_vol_tlv),
	/* ADC Volume */
	SOC_DOUBLE_TLV("ADC volume", SUNXI_ADC_VOL_CTRL, ADC_VOL_L, ADC_VOL_R,
		       0xFF, 0, adc_vol_tlv),
	/* Headphone Gain */
	SOC_SINGLE_TLV("Headphone Volume", SUNXI_DAC_REG, HEADPHONE_GAIN,
			0x7, 0, headphone_gain_tlv),
};

/* lineout controls */
static const char * const left_lineout_text[] = {
	"DAC_SINGLE", "DAC_DIFFER",
};

static const struct soc_enum left_lineout_enum =
	SOC_ENUM_SINGLE(SUNXI_DAC_REG, LINEOUTLDIFFEN,
			ARRAY_SIZE(left_lineout_text), left_lineout_text);

static const struct snd_kcontrol_new left_lineout_mux =
	SOC_DAPM_ENUM("LINEOUT Output Select", left_lineout_enum);

/* mic controls */
static const struct snd_kcontrol_new mic1_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Boost Switch", SUNXI_ADCL_REG, MIC1AMPEN, 1, 0),
};

static const struct snd_kcontrol_new mic2_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC2 Boost Switch", SUNXI_ADCR_REG, MIC2AMPEN, 1, 0),
};

/*audio dapm widget */
static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SUNXI_DAC_REG,
				DACLEN, 0,
				sunxi_codec_playback_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, SUNXI_DAC_REG,
				DACREN, 0,
				sunxi_codec_playback_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("ADCL", "Capture", 0, SUNXI_ADCL_REG,
				ADCL_EN, 0,
				sunxi_codec_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("ADCR", "Capture", 0, SUNXI_ADCR_REG,
				ADCR_EN, 0,
				sunxi_codec_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("LINEOUT Output Select", SND_SOC_NOPM,
			0, 0, &left_lineout_mux),

	SND_SOC_DAPM_MIXER_E("ADCL Input", SND_SOC_NOPM, 0, 0,
			mic1_input_mixer, ARRAY_SIZE(mic1_input_mixer),
			sunxi_codec_adc_mixer_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER_E("ADCR Input", SND_SOC_NOPM, 0, 0,
			mic2_input_mixer, ARRAY_SIZE(mic2_input_mixer),
			sunxi_codec_adc_mixer_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MICBIAS("MainMic Bias", SUNXI_MICBIAS_REG,
				MMICBIASEN, 0),

	SND_SOC_DAPM_INPUT("MIC1"),
	SND_SOC_DAPM_INPUT("MIC2"),

	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),

	SND_SOC_DAPM_HP("Headphone", sunxi_codec_headphone_event),
	SND_SOC_DAPM_LINE("LINEOUT", sunxi_codec_lineout_event),
	SND_SOC_DAPM_SPK("HpSpeaker", sunxi_codec_hpspeaker_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* Mic input route */
	{"ADCL Input", "MIC1 Boost Switch", "MIC1"},
	{"ADCR Input", "MIC2 Boost Switch", "MIC2"},

	{"ADCL", NULL, "ADCL Input"},
	{"ADCR", NULL, "ADCR Input"},

	/* LINEOUT Output route */
	{"LINEOUT Output Select", "DAC_SINGLE", "DACL"},
	{"LINEOUT Output Select", "DAC_DIFFER", "DACL"},

	{"LINEOUT", NULL, "LINEOUT Output Select"},

	/* Headphone output route */
	{"HPOUTL", NULL, "DACL"},
	{"HPOUTR", NULL, "DACR"},

	{"Headphone", NULL, "HPOUTL"},
	{"Headphone", NULL, "HPOUTR"},

	{"HpSpeaker", NULL, "HPOUTR"},
	{"HpSpeaker", NULL, "HPOUTL"},
};

static void sunxi_codec_init(struct snd_soc_component *component)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	/* DAC_VOL_SEL default disabled */
	snd_soc_component_update_bits(component, SUNXI_DAC_VOL_CTRL,
			(0x1 << DAC_VOL_SEL), (0x1 << DAC_VOL_SEL));

	/* ADC_VOL_SEL default disabled */
	snd_soc_component_update_bits(component, SUNXI_ADC_VOL_CTRL,
			(0x1 << ADC_VOL_SEL), (0x1 << ADC_VOL_SEL));

	/* Enable ADCFDT to overcome niose at the beginning */
	snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
			(0x7 << ADCDFEN), (0x7 << ADCDFEN));

	/* Digital VOL defeult setting */
	snd_soc_component_update_bits(component, SUNXI_DAC_DPC,
			0x3F << DVOL,
			sunxi_codec->digital_vol << DVOL);

	/* LINEOUT VOL defeult setting */
	snd_soc_component_update_bits(component, SUNXI_DAC_REG,
			0x1F << LINEOUT_VOL,
			sunxi_codec->lineout_vol << LINEOUT_VOL);

	/* Headphone Gain defeult setting */
	snd_soc_component_update_bits(component, SUNXI_DAC_REG,
			0x7 << HEADPHONE_GAIN,
			sunxi_codec->headphonegain << HEADPHONE_GAIN);

	/* ADCL MIC1 gain defeult setting */
	snd_soc_component_update_bits(component, SUNXI_ADCL_REG,
			0x1F << ADCL_PGA_GAIN_CTRL,
			sunxi_codec->mic1gain << ADCL_PGA_GAIN_CTRL);

	/* ADCR MIC2 gain defeult setting */
	snd_soc_component_update_bits(component, SUNXI_ADCR_REG,
			0x1F << ADCR_PGA_GAIN_CTRL,
			sunxi_codec->mic2gain << ADCR_PGA_GAIN_CTRL);

	/* ADCL/R IOP params default setting */
	snd_soc_component_update_bits(component, SUNXI_ADCL_REG,
			0xFF << ADCL_IOPMICL, 0x55 << ADCL_IOPMICL);
	snd_soc_component_update_bits(component, SUNXI_ADCR_REG,
			0xFF << ADCR_IOPMICL, 0x55 << ADCR_IOPMICL);

	/* For improve performance of THD+N about HP */
	snd_soc_component_update_bits(component, SUNXI_HEADPHONE_REG,
			(0x3 << CP_CLKS), (0x2 << CP_CLKS));

	/* To fix some pop noise */
	snd_soc_component_update_bits(component, SUNXI_HEADPHONE_REG,
			(0x1 << HPCALIFIRST), (0x1 << HPCALIFIRST));

	snd_soc_component_update_bits(component, SUNXI_HEADPHONE_REG,
			(0x3 << HPPA_DEL), (0x3 << HPPA_DEL));

	snd_soc_component_update_bits(component, SUNXI_DAC_REG,
			(0x3 << CPLDO_VOLTAGE), (0x1 << CPLDO_VOLTAGE));

#ifdef SUNXI_CODEC_DAP_ENABLE
	if (sunxi_codec->hw_config.adcdrc_cfg)
		adcdrc_config(component);
	if (sunxi_codec->hw_config.adchpf_cfg)
		adchpf_config(component);

	if (sunxi_codec->hw_config.dacdrc_cfg)
		dacdrc_config(component);
	if (sunxi_codec->hw_config.dachpf_cfg)
		dachpf_config(component);
#endif
}

static int sunxi_codec_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
#ifdef SUNXI_CODEC_DAP_ENABLE
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_codec->hw_config.adchpf_cfg)
			adchpf_enable(component, 1);
	}
#endif

	return 0;
}

static int sunxi_codec_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	int i = 0;

	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(3 << FIFO_MODE), (3 << FIFO_MODE));
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(1 << TX_SAMPLE_BITS), (0 << TX_SAMPLE_BITS));
		} else {
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << RX_FIFO_MODE), (1 << RX_FIFO_MODE));
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << RX_SAMPLE_BITS), (0 << RX_SAMPLE_BITS));
		}
		break;
	case	SNDRV_PCM_FORMAT_S24_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(3 << FIFO_MODE), (0 << FIFO_MODE));
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(1 << TX_SAMPLE_BITS), (1 << TX_SAMPLE_BITS));
		} else {
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << RX_FIFO_MODE), (0 << RX_FIFO_MODE));
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << RX_SAMPLE_BITS), (1 << RX_SAMPLE_BITS));
		}
		break;
	default:
		break;
	}

	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
					(0x7 << DAC_FS),
					(sample_rate_conv[i].rate_bit << DAC_FS));
			} else {
				if (sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
					(0x7 << ADC_FS),
					(sample_rate_conv[i].rate_bit<<ADC_FS));
			}
		}
	}

	/* reset the adchpf func setting for different sampling */
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_codec->hw_config.adchpf_cfg) {
			if (params_rate(params) == 16000) {
				snd_soc_component_write(component, SUNXI_ADC_DRC_HHPFC,
						(0x00F623A5 >> 16) & 0xFFFF);

				snd_soc_component_write(component, SUNXI_ADC_DRC_LHPFC,
							0x00F623A5 & 0xFFFF);

			} else if (params_rate(params) == 44100) {
				snd_soc_component_write(component, SUNXI_ADC_DRC_HHPFC,
						(0x00FC60DB >> 16) & 0xFFFF);

				snd_soc_component_write(component, SUNXI_ADC_DRC_LHPFC,
							0x00FC60DB & 0xFFFF);
			} else {
				snd_soc_component_write(component, SUNXI_ADC_DRC_HHPFC,
						(0x00FCABB3 >> 16) & 0xFFFF);

				snd_soc_component_write(component, SUNXI_ADC_DRC_LHPFC,
							0x00FCABB3 & 0xFFFF);
			}
		}
	}

	switch (params_channels(params)) {
	case 1:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(0x1 << DAC_MONO_EN), 0x1 << DAC_MONO_EN);
		} else {
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x3 << ADC_CHAN_EN), (0x1 << ADC_CHAN_EN));
		}
		break;
	case 2:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
				(0x1 << DAC_MONO_EN), (0x0 << DAC_MONO_EN));
		} else {
			snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(0x3 << ADC_CHAN_EN), (0x3 << ADC_CHAN_EN));
		}
		break;
	default:
		SOUND_LOG_ERR("only support mono or stereo mode.\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_set_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	switch (clk_id) {
	case	0:
		/* For setting the clk source to 90.3168M to surpport playback */
		if (clk_set_parent(sunxi_codec->dacclk, sunxi_codec->pllcomdiv5)) {
			SOUND_LOG_ERR("set parent of dacclk to pllcomdiv5 failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->dacclk, freq)) {
			SOUND_LOG_ERR("codec set dac clk rate failed\n");
			return -EINVAL;
		}
		break;
	case	1:
		/* For setting the clk source to 90.3168M to surpport capture */
		if (clk_set_parent(sunxi_codec->adcclk, sunxi_codec->pllcomdiv5)) {
			SOUND_LOG_ERR("set parent of adcclk to pllcomdiv5 failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->adcclk, freq)) {
			SOUND_LOG_ERR("codec set adc clk rate failed\n");
			return -EINVAL;
		}
		break;
	case	2:
		/* For setting the clk source to 98.304M to surpport playback */
		if (clk_set_parent(sunxi_codec->dacclk, sunxi_codec->pllclk)) {
			SOUND_LOG_ERR("set parent of dacclk to pllclk failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->dacclk, freq)) {
			SOUND_LOG_ERR("codec set dac clk rate failed\n");
			return -EINVAL;
		}
		break;
	case	3:
		/* For setting the clk source to 98.304M to surpport capture */
		if (clk_set_parent(sunxi_codec->adcclk, sunxi_codec->pllclk)) {
			SOUND_LOG_ERR("set parent of adcclk to pllclk failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->adcclk, freq)) {
			SOUND_LOG_ERR("codec set adc clk rate failed\n");
			return -EINVAL;
		}
		break;
	default:
		SOUND_LOG_ERR("Bad clk params input!\n");
		return -EINVAL;
	}

	return 0;
}

static void sunxi_codec_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
#ifdef SUNXI_CODEC_DAP_ENABLE
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_codec->hw_config.adchpf_cfg)
			adchpf_enable(component, 0);
	}
#endif
}

static int sunxi_codec_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_component_update_bits(component, SUNXI_DAC_FIFOC,
			(1 << FIFO_FLUSH), (1 << FIFO_FLUSH));
		snd_soc_component_write(component, SUNXI_DAC_FIFOS,
			(1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT));
		snd_soc_component_write(component, SUNXI_DAC_CNT, 0);
	} else {
		snd_soc_component_update_bits(component, SUNXI_ADC_FIFOC,
				(1 << ADC_FIFO_FLUSH), (1 << ADC_FIFO_FLUSH));
		snd_soc_component_write(component, SUNXI_ADC_FIFOS,
				(1 << ADC_RXA_INT | 1 << ADC_RXO_INT));
		snd_soc_component_write(component, SUNXI_ADC_CNT, 0);
	}

	return 0;
}

static int sunxi_codec_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(dai->component);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_codec->regmap, SUNXI_DAC_FIFOC,
				(1 << DAC_DRQ_EN), (1 << DAC_DRQ_EN));
		else
			regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFOC,
				(1 << ADC_DRQ_EN), (1 << ADC_DRQ_EN));
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_codec->regmap, SUNXI_DAC_FIFOC,
				(1 << DAC_DRQ_EN), (0 << DAC_DRQ_EN));
		else
			regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFOC,
				(1 << ADC_DRQ_EN), (0 << ADC_DRQ_EN));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.startup	= sunxi_codec_startup,
	.hw_params	= sunxi_codec_hw_params,
	.shutdown	= sunxi_codec_shutdown,
	.set_sysclk	= sunxi_codec_set_sysclk,
	.trigger	= sunxi_codec_trigger,
	.prepare	= sunxi_codec_prepare,
};

static struct snd_soc_dai_driver sunxi_codec_dai[] = {
	{
		.name	= "sun50iw10codec",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates	= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &sunxi_codec_dai_ops,
	},
};

static int sunxi_codec_probe(struct snd_soc_component *component)
{
	int ret;
	struct snd_soc_dapm_context *dapm = &component->dapm;

	ret = snd_soc_add_component_controls(component, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));
	if (ret)
		SOUND_LOG_ERR("failed to register codec controls!\n");

	snd_soc_dapm_new_controls(dapm, sunxi_codec_dapm_widgets,
				ARRAY_SIZE(sunxi_codec_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, sunxi_codec_dapm_routes,
				ARRAY_SIZE(sunxi_codec_dapm_routes));

	sunxi_codec_init(component);

	return 0;
}

static void sunxi_codec_remove(struct snd_soc_component *component)
{

}

static int save_audio_reg(struct sunxi_codec_info *sunxi_codec)
{
	int i = 0;

	while (reg_labels[i].name != NULL) {
		regmap_read(sunxi_codec->regmap, reg_labels[i].address,
			&reg_labels[i].value);
		i++;
	}

	return i;
}

static int echo_audio_reg(struct sunxi_codec_info *sunxi_codec)
{
	int i = 0;

	while (reg_labels[i].name != NULL) {
		regmap_write(sunxi_codec->regmap, reg_labels[i].address,
					reg_labels[i].value);
		i++;
	}

	return i;
}

static int sunxi_codec_suspend(struct snd_soc_component *component)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);

	SOUND_LOG_DEBUG("Enter\n");
	save_audio_reg(sunxi_codec);

	if (spk_cfg->used)
		gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));

	if (sunxi_codec->vol_supply.avcc)
		regulator_disable(sunxi_codec->vol_supply.avcc);

	if (sunxi_codec->vol_supply.cpvin)
		regulator_disable(sunxi_codec->vol_supply.cpvin);

	clk_disable_unprepare(sunxi_codec->dacclk);
	clk_disable_unprepare(sunxi_codec->adcclk);
	clk_disable_unprepare(sunxi_codec->pllcomdiv5);
	clk_disable_unprepare(sunxi_codec->pllcom);
	clk_disable_unprepare(sunxi_codec->pllclk);
	clk_disable_unprepare(sunxi_codec->codec_clk_bus);
	reset_control_assert(sunxi_codec->codec_clk_rst);

	SOUND_LOG_DEBUG("End\n");

	return 0;
}

static int sunxi_codec_resume(struct snd_soc_component *component)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);
	unsigned int ret;

	SOUND_LOG_DEBUG("Enter\n");

	if (sunxi_codec->vol_supply.avcc) {
		ret = regulator_enable(sunxi_codec->vol_supply.avcc);
		if (ret)
			SOUND_LOG_ERR("resume avcc enable failed!\n");
	}

	if (sunxi_codec->vol_supply.cpvin) {
		ret = regulator_enable(sunxi_codec->vol_supply.cpvin);
		if (ret)
			SOUND_LOG_ERR("resume cpvin enable failed!\n");
	}

	if (clk_set_rate(sunxi_codec->pllcom, 451584000)) {
		SOUND_LOG_ERR("resume codec source set pllcom rate failed\n");
		return -EBUSY;

	}

	if (clk_set_rate(sunxi_codec->pllclk, 98304000)) {
		SOUND_LOG_ERR("resume codec source set pllclk rate failed\n");
		return -EBUSY;
	}

	if (reset_control_deassert(sunxi_codec->codec_clk_rst)) {
		SOUND_LOG_ERR("resume deassert the codec reset failed\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->codec_clk_bus)) {
		SOUND_LOG_ERR("enable codec bus clk failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->pllclk)) {
		SOUND_LOG_ERR("enable pllclk failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->pllcom)) {
		SOUND_LOG_ERR("enable pllcom failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->pllcomdiv5)) {
		SOUND_LOG_ERR("enable pllcomdiv5 failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->dacclk)) {
		SOUND_LOG_ERR("enable dacclk failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->adcclk)) {
		SOUND_LOG_ERR("enable  adcclk failed, resume exit\n");
		clk_disable_unprepare(sunxi_codec->adcclk);
		return -EBUSY;
	}

	if (spk_cfg->used) {
		gpio_direction_output(spk_cfg->spk_gpio, 1);
		gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));
	}

	sunxi_codec_init(component);
	echo_audio_reg(sunxi_codec);

	SOUND_LOG_DEBUG("End\n");

	return 0;
}

static unsigned int sunxi_codec_read(struct snd_soc_component *component,
					unsigned int reg)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	regmap_read(sunxi_codec->regmap, reg, &reg_val);

	return reg_val;
}

static int sunxi_codec_write(struct snd_soc_component *component,
				unsigned int reg, unsigned int val)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_component_get_drvdata(component);

	regmap_write(sunxi_codec->regmap, reg, val);

	return 0;
};

static struct snd_soc_component_driver soc_codec_dev_sunxi = {
	.probe = sunxi_codec_probe,
	.remove = sunxi_codec_remove,
	.suspend = sunxi_codec_suspend,
	.resume = sunxi_codec_resume,
	.read = sunxi_codec_read,
	.write = sunxi_codec_write,
};

/* audiocodec reg dump about */
static ssize_t show_audio_reg(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(dev);
	int count = 0, i = 0;
	unsigned int reg_val;
	unsigned int size = ARRAY_SIZE(reg_labels);

	count += sprintf(buf, "dump audiocodec reg:\n");

	while ((i < size) && (reg_labels[i].name != NULL)) {
		regmap_read(sunxi_codec->regmap,
				reg_labels[i].address, &reg_val);
		count += sprintf(buf + count, "%-20s [0x%03x]: 0x%-10x save_val:0x%x\n",
			reg_labels[i].name, (reg_labels[i].address),
			reg_val, reg_labels[i].value);
		i++;
	}

	return count;
}

/* ex:
 * param 1: 0 read;1 write
 * param 2: reg value;
 * param 3: write value;
	read:
		echo 0,0x00> audio_reg
	write:
		echo 1,0x00,0xa > audio_reg
*/
static ssize_t store_audio_reg(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int input_reg_val = 0;
	int input_reg_offset = 0;
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(dev);

	ret = sscanf(buf, "%d,0x%x,0x%x", &rw_flag,
			&input_reg_offset, &input_reg_val);
	SOUND_LOG_INFO("ret:%d, reg_offset:%d, reg_val:0x%x\n",
			ret, input_reg_offset, input_reg_val);

	if (!(rw_flag == 1 || rw_flag == 0)) {
		SOUND_LOG_ERR("not rw_flag\n");
		ret = count;
		goto out;
	}

	if (input_reg_offset > SUNXI_BIAS_REG) {
		SOUND_LOG_ERR("the reg offset[0x%03x] > SUNXI_BIAS_REG[0x%03x]\n",
			      input_reg_offset, SUNXI_BIAS_REG);
		ret = count;
		goto out;
	}

	if (rw_flag) {
		regmap_write(sunxi_codec->regmap,
				input_reg_offset, input_reg_val);
	} else {
		regmap_read(sunxi_codec->regmap,
				input_reg_offset, &input_reg_val);
		SOUND_LOG_INFO("\n\n Reg[0x%x] : 0x%08x\n\n",
				input_reg_offset, input_reg_val);
	}
	ret = count;
out:
	return ret;
}

static DEVICE_ATTR(audio_reg, 0644, show_audio_reg, store_audio_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group audio_debug_attr_group = {
	.name   = "audio_reg_debug",
	.attrs  = audio_debug_attrs,
};

/* regmap configuration */
static const struct regmap_config sunxi_codec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_HMIC_STS,
	.cache_type = REGCACHE_NONE,
};
static const struct snd_pcm_hardware snd_rockchip_hardware = {
	.info			= SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_PAUSE |
				  SNDRV_PCM_INFO_RESUME |
				  SNDRV_PCM_INFO_INTERLEAVED,
	.period_bytes_min	= 32,
	.period_bytes_max	= 8192,
	.periods_min		= 1,
	.periods_max		= 52,
	.buffer_bytes_max	= 64 * 1024,
	.fifo_size		= 32,
};

static int sunxi_codec_regulator_init(struct platform_device *pdev,
				struct sunxi_codec_info *sunxi_codec)
{
	int ret = -EFAULT;

	sunxi_codec->vol_supply.avcc = regulator_get(&pdev->dev, "avcc");
	if (IS_ERR(sunxi_codec->vol_supply.avcc)) {
		SOUND_LOG_ERR("get audio avcc failed\n");
		goto err_regulator;
	} else {
		ret = regulator_set_voltage(sunxi_codec->vol_supply.avcc,
							1800000, 1800000);
		if (ret) {
			SOUND_LOG_ERR("audiocodec avcc set vol failed\n");
			goto err_regulator_avcc;
		}

		ret = regulator_enable(sunxi_codec->vol_supply.avcc);
		if (ret) {
			SOUND_LOG_ERR("avcc enable failed!\n");
			goto err_regulator_avcc;
		}
	}

	sunxi_codec->vol_supply.cpvin = regulator_get(&pdev->dev, "cpvin");
	if (IS_ERR(sunxi_codec->vol_supply.cpvin)) {
		SOUND_LOG_ERR("get cpvin failed\n");
		goto err_regulator_avcc;
	} else {
		ret = regulator_set_voltage(sunxi_codec->vol_supply.cpvin,
							1800000, 1800000);
		if (ret) {
			SOUND_LOG_ERR("cpvin set vol failed\n");
			goto err_regulator_cpvin;
		}

		ret = regulator_enable(sunxi_codec->vol_supply.cpvin);
		if (ret) {
			SOUND_LOG_ERR("cpvin enable failed!\n");
			goto err_regulator_cpvin;
		}
	}
	return 0;

err_regulator_cpvin:
	regulator_put(sunxi_codec->vol_supply.cpvin);
err_regulator_avcc:
	regulator_put(sunxi_codec->vol_supply.avcc);
err_regulator:
	return ret;
}

static int sunxi_codec_clk_init(struct device_node *np,
				struct platform_device *pdev,
				struct sunxi_codec_info *sunxi_codec)
{
	int ret = -EBUSY;
	/* get the parent clk and the module clk */
	sunxi_codec->pllclk = of_clk_get_by_name(np, "pll_audio");
	sunxi_codec->dacclk = of_clk_get_by_name(np, "codec_dac");
	sunxi_codec->adcclk = of_clk_get_by_name(np, "codec_adc");
	sunxi_codec->pllcom = of_clk_get_by_name(np, "pll_com");
	sunxi_codec->pllcomdiv5 = of_clk_get_by_name(np, "pll_com_audio");
	sunxi_codec->codec_clk_bus = of_clk_get_by_name(np, "codec_bus");
	sunxi_codec->codec_clk_rst = devm_reset_control_get(&pdev->dev, NULL);

	if (reset_control_deassert(sunxi_codec->codec_clk_rst)) {
		SOUND_LOG_ERR("deassert the codec reset failed\n");
		goto err_devm_kfree;
	}

	if (clk_set_parent(sunxi_codec->pllcomdiv5, sunxi_codec->pllcom)) {
		SOUND_LOG_ERR("set parent of pllcomdiv5 to pllcom failed\n");
		goto err_devm_kfree;
	}

	if (clk_set_parent(sunxi_codec->dacclk, sunxi_codec->pllcomdiv5)) {
		SOUND_LOG_ERR("set parent of dacclk to pllcomdiv5 failed\n");
		goto err_devm_kfree;
	}

	if (clk_set_parent(sunxi_codec->adcclk, sunxi_codec->pllcomdiv5)) {
		SOUND_LOG_ERR("set parent of adcclk to pllcomdiv5 failed\n");
		goto err_devm_kfree;
	}

	if (clk_set_rate(sunxi_codec->pllcom, 451584000)) {
		SOUND_LOG_ERR("codec source set pllcom rate failed\n");
		goto err_devm_kfree;
	}

	if (clk_set_rate(sunxi_codec->pllclk, 98304000)) {
		SOUND_LOG_ERR("codec source set pllclk rate failed\n");
		goto err_devm_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->codec_clk_bus)) {
		SOUND_LOG_ERR("codec clk bus enable failed\n");
	}

	if (clk_prepare_enable(sunxi_codec->pllclk)) {
		SOUND_LOG_ERR("pllclk enable failed\n");
		goto err_bus_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->pllcom)) {
		SOUND_LOG_ERR("pllcom enable failed\n");
		goto err_pllclk_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->pllcomdiv5)) {
		SOUND_LOG_ERR("pllcomdiv5 enable failed\n");
		goto err_pllcom_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->dacclk)) {
		SOUND_LOG_ERR("dacclk enable failed\n");
		goto err_pllcomdiv5_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->adcclk)) {
		SOUND_LOG_ERR("moduleclk enable failed\n");
		goto err_dacclk_kfree;
	}
	return 0;

err_dacclk_kfree:
	clk_disable_unprepare(sunxi_codec->dacclk);
err_pllcomdiv5_kfree:
	clk_disable_unprepare(sunxi_codec->pllcomdiv5);
err_pllcom_kfree:
	clk_disable_unprepare(sunxi_codec->pllcom);
err_pllclk_kfree:
	clk_disable_unprepare(sunxi_codec->pllclk);
err_bus_kfree:
	clk_disable_unprepare(sunxi_codec->codec_clk_bus);
err_devm_kfree:
	return ret;
}

static int sunxi_codec_parse_params(struct device_node *np,
				    struct platform_device *pdev,
				    struct sunxi_codec_info *sunxi_codec)
{
	int ret = 0;
	unsigned int temp_val;
	/* get the special property form the board.dts */
	ret = of_property_read_u32(np, "digital_vol", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("digital volume get failed, use default vol\n");
		sunxi_codec->digital_vol = 0;
	} else {
		sunxi_codec->digital_vol = temp_val;
	}

	/* lineout volume */
	ret = of_property_read_u32(np, "lineout_vol", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("lineout volume get failed, use default vol\n");
		sunxi_codec->lineout_vol = 0;
	} else {
		sunxi_codec->lineout_vol = temp_val;
	}

	/* headphone volume */
	ret = of_property_read_u32(np, "headphonegain", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("headphonegain volume get failed, use default vol\n");
		sunxi_codec->headphonegain = 0;
	} else {
		sunxi_codec->headphonegain = temp_val;
	}

	/* mic gain for capturing */
	ret = of_property_read_u32(np, "mic1gain", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("mic1gain get failed, use default vol\n");
		sunxi_codec->mic1gain = 32;
	} else {
		sunxi_codec->mic1gain = temp_val;
	}
	ret = of_property_read_u32(np, "mic2gain", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("mic2gain get failed, use default vol\n");
		sunxi_codec->mic2gain = 32;
	} else {
		sunxi_codec->mic2gain = temp_val;
	}

	/* Pa's delay time(ms) to work fine */
	ret = of_property_read_u32(np, "pa_msleep_time", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("pa_msleep get failed, use default vol\n");
		sunxi_codec->spk_config.pa_msleep = 160;
	} else {
		sunxi_codec->spk_config.pa_msleep = temp_val;
	}

	/* PA/SPK enable property */
	ret = of_property_read_u32(np, "pa_level", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("pa_level get failed, use default vol\n");
		sunxi_codec->spk_config.pa_level = 1;
	} else {
		sunxi_codec->spk_config.pa_level = temp_val;
	}

	SOUND_LOG_DEBUG("digital_vol:%d, lineout_vol:%d, "
			"mic1gain:%d, mic2gain:%d, "
			"pa_msleep:%d, pa_level:%d\n",
			sunxi_codec->digital_vol,
			sunxi_codec->lineout_vol,
			sunxi_codec->mic1gain,
			sunxi_codec->mic2gain,
			sunxi_codec->spk_config.pa_msleep,
			sunxi_codec->spk_config.pa_level);

#ifdef SUNXI_CODEC_DAP_ENABLE
	/* ADC/DAC DRC/HPF func enable property */
	ret = of_property_read_u32(np, "adcdrc_cfg", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("adcdrc_cfg configs missing or invalid.\n");
	} else {
		sunxi_codec->hw_config.adcdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "adchpf_cfg", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("adchpf_cfg configs missing or invalid.\n");
	} else {
		sunxi_codec->hw_config.adchpf_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "dacdrc_cfg", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("dacdrc_cfg configs missing or invalid.\n");
	} else {
		sunxi_codec->hw_config.dacdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "dachpf_cfg", &temp_val);
	if (ret < 0) {
		SOUND_LOG_WARN("dachpf_cfg configs missing or invalid.\n");
	} else {
		sunxi_codec->hw_config.dachpf_cfg = temp_val;
	}

	SOUND_LOG_DEBUG("adcdrc_cfg:%d, adchpf_cfg:%d, "
			"dacdrc_cfg:%d, dachpf:%d\n",
			sunxi_codec->hw_config.adcdrc_cfg,
			sunxi_codec->hw_config.adchpf_cfg,
			sunxi_codec->hw_config.dacdrc_cfg,
			sunxi_codec->hw_config.dachpf_cfg);
#endif
	/* get the gpio number to control external speaker */
	ret = of_get_named_gpio(np, "gpio-spk", 0);
	if (ret >= 0) {
		sunxi_codec->spk_config.used = 1;
		sunxi_codec->spk_config.spk_gpio = ret;
		if (!gpio_is_valid(sunxi_codec->spk_config.spk_gpio)) {
			SOUND_LOG_ERR("gpio-spk is invalid\n");
			return -EINVAL;
		} else {
			ret = devm_gpio_request(&pdev->dev,
				sunxi_codec->spk_config.spk_gpio, "SPK");
			if (ret) {
				SOUND_LOG_ERR("failed to request gpio-spk\n");
				return -EBUSY;
			}
		}
	} else {
		sunxi_codec->spk_config.used = 0;
		SOUND_LOG_ERR("gpio-spk no exist\n");
	}

	return ret;
}

static int sunxi_internal_codec_probe(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec;
	struct resource res;
	struct resource *memregion = NULL;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	if (IS_ERR_OR_NULL(np)) {
		SOUND_LOG_ERR("pdev->dev.of_node is err.\n");
		ret = -EFAULT;
		goto err_node_put;
	}

	sunxi_codec = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_codec_info), GFP_KERNEL);
	if (!sunxi_codec) {
		SOUND_LOG_ERR("Can't allocate sunxi codec memory\n");
		ret = -ENOMEM;
		goto err_node_put;
	}
	dev_set_drvdata(&pdev->dev, sunxi_codec);
	sunxi_codec->dev = &pdev->dev;

	if (sunxi_codec_regulator_init(pdev, sunxi_codec) != 0) {
		SOUND_LOG_ERR("Failed to init sunxi audio regulator\n");
		ret = -EFAULT;
		goto err_devm_kfree;
	}

	if (sunxi_codec_clk_init(np, pdev, sunxi_codec) != 0) {
		SOUND_LOG_ERR("Failed to init sunxi audio clk\n");
		ret = -EFAULT;
		goto err_devm_kfree;
	}

	ret = sunxi_codec_parse_params(np, pdev, sunxi_codec);
	if (ret != 0) {
		SOUND_LOG_WARN("Failed to parse sunxi audio params\n");
		if (ret == -EPROBE_DEFER) {
			SOUND_LOG_WARN("probe get some params failed and retry probe!\n");
			goto err_devm_kfree;
		}
	}

	/* codec reg_base */
	/* get the true codec addr form np0 to avoid the build warning */
	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		SOUND_LOG_ERR("Failed to get sunxi resource\n");
		return -EINVAL;
		goto err_moduleclk_disable;
	}

	memregion = devm_request_mem_region(&pdev->dev, res.start,
				resource_size(&res), "sunxi-internal-codec");
	if (!memregion) {
		SOUND_LOG_ERR("sunxi memory region already claimed\n");
		ret = -EBUSY;
		goto err_moduleclk_disable;
	}

	sunxi_codec->digital_base = devm_ioremap(&pdev->dev, res.start,
						 resource_size(&res));
	if (!sunxi_codec->digital_base) {
		SOUND_LOG_ERR("sunxi digital_base ioremap failed\n");
		ret = -EBUSY;
		goto err_moduleclk_disable;
	}

	sunxi_codec->regmap = devm_regmap_init_mmio(&pdev->dev,
				sunxi_codec->digital_base,
				&sunxi_codec_regmap_config);
	if (IS_ERR_OR_NULL(sunxi_codec->regmap)) {
		SOUND_LOG_ERR("regmap init failed\n");
		ret = PTR_ERR(sunxi_codec->regmap);
		goto err_moduleclk_disable;
	}

	/* CODEC DAI cfg and register */
	ret = devm_snd_soc_register_component(&pdev->dev,
					      &soc_codec_dev_sunxi,
					      sunxi_codec_dai,
					      ARRAY_SIZE(sunxi_codec_dai));
	if (ret < 0) {
		SOUND_LOG_ERR("register codec failed\n");
		goto err_moduleclk_disable;
	}

	ret  = sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
	if (ret) {
		SOUND_LOG_ERR("failed to create attr group\n");
		goto err_moduleclk_disable;
	}

	SOUND_LOG_INFO("audiocodec probe finished.\n");

	return 0;


err_moduleclk_disable:
	clk_disable_unprepare(sunxi_codec->dacclk);
	clk_disable_unprepare(sunxi_codec->adcclk);
	clk_disable_unprepare(sunxi_codec->pllclk);
	clk_disable_unprepare(sunxi_codec->pllcomdiv5);
	clk_disable_unprepare(sunxi_codec->pllcom);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_codec);
err_node_put:
	of_node_put(np);
	return ret;
}

static int  __exit sunxi_internal_codec_remove(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(&pdev->dev);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);

	if (spk_cfg->used) {
		devm_gpio_free(&pdev->dev,
					sunxi_codec->spk_config.spk_gpio);
	}

	if (sunxi_codec->vol_supply.avcc) {
		regulator_disable(sunxi_codec->vol_supply.avcc);
		regulator_put(sunxi_codec->vol_supply.avcc);
	}

	if (sunxi_codec->vol_supply.cpvin) {
		regulator_disable(sunxi_codec->vol_supply.cpvin);
		regulator_put(sunxi_codec->vol_supply.cpvin);
	}

	sysfs_remove_group(&pdev->dev.kobj, &audio_debug_attr_group);
	snd_soc_unregister_component(&pdev->dev);

	clk_disable_unprepare(sunxi_codec->dacclk);
	clk_put(sunxi_codec->dacclk);
	clk_disable_unprepare(sunxi_codec->adcclk);
	clk_put(sunxi_codec->adcclk);
	clk_disable_unprepare(sunxi_codec->pllcomdiv5);
	clk_put(sunxi_codec->pllcomdiv5);
	clk_disable_unprepare(sunxi_codec->pllcom);
	clk_put(sunxi_codec->pllcom);
	clk_disable_unprepare(sunxi_codec->pllclk);
	clk_put(sunxi_codec->pllclk);
	clk_disable_unprepare(sunxi_codec->codec_clk_bus);
	clk_put(sunxi_codec->codec_clk_bus);
	reset_control_assert(sunxi_codec->codec_clk_rst);

	devm_iounmap(&pdev->dev, sunxi_codec->digital_base);
	devm_kfree(&pdev->dev, sunxi_codec);
	platform_set_drvdata(pdev, NULL);

	SOUND_LOG_INFO("audiocodec remove finished.\n");

	return 0;
}

static const struct of_device_id sunxi_internal_codec_of_match[] = {
	{ .compatible = "allwinner,sunxi-internal-codec", },
	{},
};

static struct platform_driver sunxi_internal_codec_driver = {
	.driver = {
		.name = "sunxi-internal-codec",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_internal_codec_of_match,
	},
	.probe = sunxi_internal_codec_probe,
	.remove = __exit_p(sunxi_internal_codec_remove),
};
module_platform_driver(sunxi_internal_codec_driver);

MODULE_DESCRIPTION("SUNXI Codec ASoC driver");
MODULE_AUTHOR("luguofang <luguofang@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-internal-codec");

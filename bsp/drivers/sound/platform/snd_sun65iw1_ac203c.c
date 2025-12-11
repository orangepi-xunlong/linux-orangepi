// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * ac203c.c -- ac203c ALSA SoC Audio driver
 *
 * Copyright (c) 2022 Allwinnertech Ltd.
 */
#define SUNXI_MODNAME		"sound-ac203c"

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sunxi-sid.h>
#include "snd_sunxi_log.h"
#include "snd_sunxi_jack.h"
#include "snd_sunxi_common.h"
#include "snd_sun65iw1_ac203c.h"

#define ADC1_OUTPUT	0
#define ADC2_OUTPUT	1
#define ADC3_OUTPUT	2

#define DRV_NAME	"sunxi-ac203c"

static struct audio_reg_label sunxi_dig_reg_labels[] = {
	REG_LABEL(SUNXI_MOD_VER),
	REG_LABEL(SUNXI_PLL_LOCK_CTL),
	REG_LABEL(SUNXI_SYS_CLK_CTL),
	REG_LABEL(SUNXI_I2S_CTL),
	REG_LABEL(SUNXI_ADDA1_DAC_DATA),
	REG_LABEL(SUNXI_I2S_BCLK_CTL),
	REG_LABEL(SUNXI_I2S_LRCK_CTL),
	REG_LABEL(SUNXI_I2S_FMT_CTL),
	REG_LABEL(SUNXI_I2S_TX_CTL),
	REG_LABEL(SUNXI_ADC_MIX_CTL),
	REG_LABEL(SUNXI_I2S_TX_CHMP_CTL),
	REG_LABEL(SUNXI_I2S_RX_CTL),
	REG_LABEL(SUNXI_DAC_MIX_CTL),
	REG_LABEL(SUNXI_I2S_RX_CHMP_CTL),
	REG_LABEL(SUNXI_DIG_BYPASS_CTL),
	REG_LABEL(SUNXI_ADDA_FS_CTL),
	REG_LABEL(SUNXI_ADC_DIG_EN),
	REG_LABEL(SUNXI_ADC_DDT_CTL),
	REG_LABEL(SUNXI_HPF_EN),
	REG_LABEL(SUNXI_HPF1_COEF_REG),
	REG_LABEL(SUNXI_HPF2_COEF_REG),
	REG_LABEL(SUNXI_HPF3_COEF_REG),
	REG_LABEL(SUNXI_ADC_MUX_CTL),
	REG_LABEL(SUNXI_ADC_DVOL_CTL),
	REG_LABEL(SUNXI_ADC_DIG_DEBUG),
	REG_LABEL(SUNXI_DAC_DIG_EN),
	REG_LABEL(SUNXI_DAC_DIG_CTL),
	REG_LABEL(SUNXI_DAC_DHP_GAIN_CTL),
	REG_LABEL(SUNXI_DAC_LOUT_CTL),
	REG_LABEL(SUNXI_DAC_DVC),
	REG_LABEL(SUNXI_EQ_CTL),
	REG_LABEL(SUNXI_EQ_COFE_CTL),
	REG_LABEL(SUNXI_DAC_MUX_CTL),
	REG_LABEL(SUNXI_HP_AVR_CTL),
	REG_LABEL(SUNXI_HP_AVR_TH),
	REG_LABEL(SUNXI_HP_AVR_DBC),
	REG_LABEL(SUNXI_EQ_B0),
	REG_LABEL(SUNXI_EQ_B1),
	REG_LABEL(SUNXI_EQ_B2),
	REG_LABEL(SUNXI_EQ_A1),
	REG_LABEL(SUNXI_EQ_A2),
	REG_LABEL(SUNXI_DRC_ENA),
	REG_LABEL(SUNXI_DRC_HPFC),
	REG_LABEL(SUNXI_DRC_CTL),
	REG_LABEL(SUNXI_DRC_DLY_CTL),
	REG_LABEL(SUNXI_DRC_OPT),
	REG_LABEL(SUNXI_DRC_LPFAT),
	REG_LABEL(SUNXI_DRC_RPFAT),
	REG_LABEL(SUNXI_DRC_LPFRT),
	REG_LABEL(SUNXI_DRC_RPFRT),
	REG_LABEL(SUNXI_DRC_LRMSAT),
	REG_LABEL(SUNXI_DRC_RRMSAT),
	REG_LABEL(SUNXI_DRC_CT),
	REG_LABEL(SUNXI_DRC_KC),
	REG_LABEL(SUNXI_DRC_OPC),
	REG_LABEL(SUNXI_DRC_LT),
	REG_LABEL(SUNXI_DRC_KL),
	REG_LABEL(SUNXI_DRC_OPL),
	REG_LABEL(SUNXI_DRC_ET),
	REG_LABEL(SUNXI_DRC_KE),
	REG_LABEL(SUNXI_DRC_OPE),
	REG_LABEL(SUNXI_DRC_KN),
	REG_LABEL(SUNXI_DRC_SFAT),
	REG_LABEL(SUNXI_DRC_SFRT),
	REG_LABEL(SUNXI_DRC_MXGS),
	REG_LABEL(SUNXI_DRC_MNGS),
	REG_LABEL(SUNXI_DRC_EPSC),
	REG_LABEL(SUNXI_ADDA1_INT_CTL),
	REG_LABEL(SUNXI_ADDA1_INT_STAT),
	REG_LABEL(SUNXI_ADDA1_FIFO_CTL),
	REG_LABEL(SUNXI_ADDA1_FIFO_STAT),
	REG_LABEL(SUNXI_ADDA1_DAC_CNT),
	REG_LABEL(SUNXI_ADDA1_ADC_DATA),
	REG_LABEL(SUNXI_ADDA1_ADC_CNT),
	REG_LABEL(SUNXI_ADDA1_SYNC_CTL),
};
static struct audio_reg_group sunxi_dig_reg_group = REG_GROUP(sunxi_dig_reg_labels);

static struct audio_reg_label sunxi_ana_reg_labels[] = {
	REG_LABEL(SUNXI_CHIP_SOFT_RST),
	REG_LABEL(SUNXI_POWER_REG1),
	REG_LABEL(SUNXI_POWER_REG2),
	REG_LABEL(SUNXI_POWER_REG3),
	REG_LABEL(SUNXI_POWER_REG4),
	REG_LABEL(SUNXI_POWER_REG5),
	REG_LABEL(SUNXI_POWER_REG6),
	REG_LABEL(SUNXI_MBIAS_REG),
	REG_LABEL(SUNXI_HBIAS_REG),
	REG_LABEL(SUNXI_DAC_REG1),
	REG_LABEL(SUNXI_DAC_REG2),
	REG_LABEL(SUNXI_DAC_REG3),
	REG_LABEL(SUNXI_DAC_REG4),
	REG_LABEL(SUNXI_HP_REG1),
	REG_LABEL(SUNXI_HP_REG2),
	REG_LABEL(SUNXI_HP_REG3),
	REG_LABEL(SUNXI_HP_REG4),
	REG_LABEL(SUNXI_HP_REG5),
	REG_LABEL(SUNXI_SYSCLK_CTL),
	REG_LABEL(SUNXI_ADC1_REG1),
	REG_LABEL(SUNXI_ADC1_REG2),
	REG_LABEL(SUNXI_ADC1_REG3),
	REG_LABEL(SUNXI_ADC1_REG4),
	REG_LABEL(SUNXI_ADC2_REG1),
	REG_LABEL(SUNXI_ADC2_REG2),
	REG_LABEL(SUNXI_ADC2_REG3),
	REG_LABEL(SUNXI_ADC2_REG4),
	REG_LABEL(SUNXI_ADC3_REG1),
	REG_LABEL(SUNXI_ADC3_REG2),
	REG_LABEL(SUNXI_ADC3_REG3),
	REG_LABEL(SUNXI_ADC3_REG4),
	REG_LABEL(SUNXI_I2S_CTL_2),
	REG_LABEL(SUNXI_I2S_BCLK_CLT1),
	REG_LABEL(SUNXI_I2S_BCLK_CLT2),
	REG_LABEL(SUNXI_I2S_LRCK_CLT1),
	REG_LABEL(SUNXI_I2S_LRCK_CLT2),
	REG_LABEL(SUNXI_I2S_LRCK_CLT3),
	REG_LABEL(SUNXI_I2S_LRCK_CLT4),
	REG_LABEL(SUNXI_I2S_FMT_CLT1),
	REG_LABEL(SUNXI_I2S0_FMT_CLT2),
	REG_LABEL(SUNXI_I2S1_FMT_CLT2),
	REG_LABEL(SUNXI_I2S_FMT_CLT3),
	REG_LABEL(SUNXI_I2S_FMT_CLT4),
	REG_LABEL(SUNXI_I2S_TX_CLT1),
	REG_LABEL(SUNXI_I2S_TX_CLT2),
	REG_LABEL(SUNXI_I2S_TX_CLT3),
	REG_LABEL(SUNXI_I2S_TX_MIX_CTL),
	REG_LABEL(SUNXI_I2S_TX_CHMP_CTL1),
	REG_LABEL(SUNXI_I2S_TX_CHMP_CTL2),
	REG_LABEL(SUNXI_I2S_TX_CHMP_CTL3),
	REG_LABEL(SUNXI_I2S_TX_CHMP_CTL4),
	REG_LABEL(SUNXI_I2S_RX_CTL1),
	REG_LABEL(SUNXI_I2S_RX_CTL2),
	REG_LABEL(SUNXI_I2S_RX_CTL3),
	REG_LABEL(SUNXI_I2S_RX_MIX_CTL),
	REG_LABEL(SUNXI_I2S_RX_CHMP_CTL_2),
	REG_LABEL(SUNXI_DEBUG_CTL),
	REG_LABEL(SUNXI_I2S_PADDRV_CTL),
	REG_LABEL(SUNXI_DEBUG_PADDRV_CTL),
	REG_LABEL(SUNXI_DIG_BYPASS_CTL_2),
	REG_LABEL(SUNXI_EFUSE_WR_CTL),
	REG_LABEL(SUNXI_ADDA_FS_CTL_2),
	REG_LABEL(SUNXI_ADC_DIG_EN_2),
	REG_LABEL(SUNXI_ADC_DDT_CTL_2),
	REG_LABEL(SUNXI_HMIC_DET_CTL),
	REG_LABEL(SUNXI_HMIC_DET_DBC),
	REG_LABEL(SUNXI_HMIC_DET_TH1),
	REG_LABEL(SUNXI_HMIC_DET_TH2),
	REG_LABEL(SUNXI_HMIC_DET_DATA),
	REG_LABEL(SUNXI_HP_DET_CTL),
	REG_LABEL(SUNXI_HP_DET_DBC),
	REG_LABEL(SUNXI_HP_DET_IRQ),
	REG_LABEL(SUNXI_HP_DET_STA),
	REG_LABEL(SUNXI_ADC_DIG_DEBUG_2),
	REG_LABEL(SUNXI_DAC_DIG_EN_2),
	REG_LABEL(SUNXI_DAC_DIG_CTL_2),
	REG_LABEL(SUNXI_HP_AVR_CTL_2),
	REG_LABEL(SUNXI_HP_AVR_THH),
	REG_LABEL(SUNXI_HP_AVR_THM),
	REG_LABEL(SUNXI_HP_AVR_THL),
	REG_LABEL(SUNXI_HP_AVR_DBC_2),
	REG_LABEL(SUNXI_INT_ADDR_CONF_REG),
};
static struct audio_reg_group sunxi_ana_reg_group = REG_GROUP(sunxi_ana_reg_labels);

static atomic_t adc_a_cnt = ATOMIC_INIT(3);

struct ac203c_priv {
	struct device *dev;

	struct sunxi_codec_mem mem;
	struct sunxi_codec_clk clk;
	struct sunxi_codec_pinctl pin;
	struct ac203c_data pdata;
	struct snd_sunxi_rglt *rglt;
	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;
	enum SND_SUNXI_CLK_STATUS clk_sta;
};

/* clk */
static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_bus_disable(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_rate(struct sunxi_codec_clk *clk, int stream,
			      unsigned int freq_in, unsigned int freq_out);

static void sunxi_regmap_update_bits(struct regmap *regmap, uint32_t reg,
				     uint32_t mask, uint32_t val)
{
	if (reg >= 0x800)
		regmap_update_bits(regmap, reg - 0x800, mask, val);
	else
		regmap_update_bits(regmap, reg, mask, val);
}

static void sunxi_regmap_write(struct regmap *regmap, uint32_t reg, uint32_t val)
{
	if (reg >= 0x800)
		regmap_write(regmap, reg - 0x800, val);
	else
		regmap_write(regmap, reg, val);
}

static void sunxi_regmap_read(struct regmap *regmap, uint32_t reg, uint32_t *val)
{
	if (reg >= 0x800)
		regmap_read(regmap, reg - 0x800, val);
	else
		regmap_read(regmap, reg, val);
}

static int sunxi_save_reg(struct regmap *regmap, struct audio_reg_group *reg_group)
{
	unsigned int i;

	SND_LOG_DEBUG("\n");

	if (reg_group->label[0].address >= 0x800) {
		for (i = 0 ; i < reg_group->size; ++i)
			sunxi_regmap_read(regmap, reg_group->label[i].address - 0x800,
				    &(reg_group->label[i].value));
	} else {
		for (i = 0 ; i < reg_group->size; ++i)
			sunxi_regmap_read(regmap, reg_group->label[i].address,
				    &(reg_group->label[i].value));
	}

	return i;
}

static int sunxi_echo_reg(struct regmap *regmap, struct audio_reg_group *reg_group)
{
	unsigned int i;

	SND_LOG_DEBUG("\n");

	if (reg_group->label[0].address >= 0x800) {
		for (i = 0 ; i < reg_group->size; ++i)
			regmap_write(regmap, reg_group->label[i].address - 0x800,
				     reg_group->label[i].value);
	} else {
		for (i = 0 ; i < reg_group->size; ++i)
			regmap_write(regmap, reg_group->label[i].address,
				     reg_group->label[i].value);
	}

	return i;
}

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_codec_clk *clk)
{
	int ret = 0;

	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* get rst clk */
	clk->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk->clk_rst)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk rst get failed\n");
		ret =  PTR_ERR(clk->clk_rst);
		goto err_get_clk_rst;
	}

	/* get bus clk */
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_adda");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk bus get failed\n");
		ret = PTR_ERR(clk->clk_bus);
		goto err_get_clk_bus;
	}

	/* get parent clk */
	clk->clk_pll_audio0 = of_clk_get_by_name(np, "clk_pll_audio0");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio0)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_pll_audio0 get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio0);
		goto err_get_clk_pll_audio0;
	}

	clk->clk_pll_audio1_5x = of_clk_get_by_name(np, "clk_pll_audio1_5x");
	if (IS_ERR_OR_NULL(clk->clk_pll_audio1_5x)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_pll_audio1_5x get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio1_5x);
		goto err_get_clk_pll_audio1_5x;
	}

	/* get module clk */
	clk->clk_adda_dac = of_clk_get_by_name(np, "clk_adda_dac");
	if (IS_ERR_OR_NULL(clk->clk_adda_dac)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_INIT, "clk_adda_dac get failed\n");
		ret = PTR_ERR(clk->clk_adda_dac);
		goto err_get_clk_adda_dac;
	}

	return 0;

err_get_clk_adda_dac:
	clk_put(clk->clk_pll_audio1_5x);
err_get_clk_pll_audio1_5x:
	clk_put(clk->clk_pll_audio0);
err_get_clk_pll_audio0:
	clk_put(clk->clk_bus);
err_get_clk_bus:
err_get_clk_rst:
	return ret;
}

static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_put(clk->clk_adda_dac);
	clk_put(clk->clk_pll_audio1_5x);
	clk_put(clk->clk_pll_audio0);
	clk_put(clk->clk_bus);
}

static int snd_sunxi_clk_bus_enable(struct sunxi_codec_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	/* to avoid register modification before module load */
	reset_control_assert(clk->clk_rst);
	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_rst deassert failed\n");
		ret = -EINVAL;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_bus enable failed\n");
		ret = -EINVAL;
		goto err_enable_clk_bus;
	}

	return 0;

err_enable_clk_bus:
	reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return ret;
}

static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	if (clk_prepare_enable(clk->clk_pll_play)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_pll_play enable failed\n");
		return -EINVAL;
	}

	if (clk_prepare_enable(clk->clk_adda_dac)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_EN, "clk_adda_dac enable failed\n");
		clk_disable_unprepare(clk->clk_pll_play);
		return -EINVAL;
	}

	return 0;
}

static void snd_sunxi_clk_bus_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_bus);
	reset_control_assert(clk->clk_rst);
}

static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_adda_dac);
	clk_disable_unprepare(clk->clk_pll_play);
}

static int snd_sunxi_clk_rate(struct sunxi_codec_clk *clk, int stream,
			      unsigned int freq_in, unsigned int freq_out)
{
	SND_LOG_DEBUG("\n");

	if (freq_in % 24576000 == 0) {
		if (clk_set_parent(clk->clk_adda_dac, clk->clk_pll_audio1_5x)) {
			SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
					"set dac parent clk failed\n");
			return -EINVAL;
		}
		clk->clk_pll_play = clk->clk_pll_audio1_5x;
	} else {
		if (clk_set_parent(clk->clk_adda_dac, clk->clk_pll_audio0)) {
			SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
					"set dac parent clk failed\n");
			return -EINVAL;
		}
		clk->clk_pll_play = clk->clk_pll_audio0;
	}
	if (clk_set_rate(clk->clk_adda_dac, freq_out * 4)) {
		SND_LOG_ERR_STD(E_AUDIOCODEC_SWDEP_CLK_SET,
				"set clk_adda_dac rate failed, rate: %u\n", freq_out);
		return -EINVAL;
	}

	return 0;
}

static int ac203c_trigger(struct snd_pcm_substream *substream,
			  int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;

	SND_LOG_DEBUG("\n");

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADDA1_INT_CTL,
						 0x1 << TX_DRQ_EN, 0x1 << TX_DRQ_EN);
		} else {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADDA1_INT_CTL,
						 0x1 << RX_DRQ_EN, 0x1 << RX_DRQ_EN);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADDA1_INT_CTL,
						 0x1 << TX_DRQ_EN, 0x0 << TX_DRQ_EN);
		} else {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADDA1_INT_CTL,
						 0x1 << RX_DRQ_EN, 0x0 << RX_DRQ_EN);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ac203c_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int sample_rate;
	unsigned int osr;
	unsigned int rate_div;
	unsigned int fs_div;
	unsigned int dac_div;

	SND_LOG_DEBUG("\n");

	/* sample bit - 24bits(Not configurable) */

	/* set fs_div and osr */
	sample_rate = params_rate(params);
	if (sample_rate == 16000 || sample_rate == 32000) {
		sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_BCLK_CTL,
					 0xF << BCLKDIV, 0x1 << BCLKDIV);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_LRCK_CTL,
					 0x3FF << LRCK_PERIOD, 0xb << LRCK_PERIOD);

		sunxi_regmap_write(ana_regmap, SUNXI_I2S_LRCK_CLT3, 0xb);
		sunxi_regmap_write(ana_regmap, SUNXI_I2S_LRCK_CLT4, 0xb);
		sunxi_regmap_write(ana_regmap, SUNXI_I2S0_FMT_CLT2, 0x22);
		sunxi_regmap_write(ana_regmap, SUNXI_I2S1_FMT_CLT2, 0x22);
	} else {
		sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_BCLK_CTL,
					 0xF << BCLKDIV, 0x1 << BCLKDIV);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_LRCK_CTL,
					 0x3FF << LRCK_PERIOD, 0x7 << LRCK_PERIOD);

		sunxi_regmap_write(ana_regmap, SUNXI_I2S_LRCK_CLT3, 0x7);
		sunxi_regmap_write(ana_regmap, SUNXI_I2S_LRCK_CLT4, 0x7);
		sunxi_regmap_write(ana_regmap, SUNXI_I2S0_FMT_CLT2, 0x11);
		sunxi_regmap_write(ana_regmap, SUNXI_I2S1_FMT_CLT2, 0x11);
	}

	if (sample_rate < 96000) {
		osr = 128;
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DDT_CTL,
					 0x3 << ADC_OSR, 0x0 << ADC_OSR);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_CTL,
					 0x3 << DAC_SWP, 0x0 << DAC_SWP);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_CTL_2,
					 0x3 << DAC_OSR, 0x0 << DAC_OSR);
	} else if (sample_rate > 96000) {
		osr = 32;
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DDT_CTL,
					 0x3 << ADC_OSR, 0x2 << ADC_OSR);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_CTL,
					 0x3 << DAC_SWP, 0x2 << DAC_SWP);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_CTL_2,
					 0x3 << DAC_OSR, 0x2 << DAC_OSR);
	} else {
		osr = 64;
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DDT_CTL,
					 0x3 << ADC_OSR, 0x1 << ADC_OSR);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_CTL,
					 0x3 << DAC_SWP, 0x1 << DAC_SWP);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_CTL_2,
					 0x3 << DAC_OSR, 0x1 << DAC_OSR);
	}

	if (sample_rate == 48000 || sample_rate == 44100) {
		dac_div = 0;
	} else if (sample_rate == 32000) {
		dac_div = 1;
	} else if (sample_rate == 24000 || sample_rate == 22050) {
		dac_div = 2;
	} else if (sample_rate == 16000) {
		dac_div = 3;
	} else if (sample_rate == 12000 || sample_rate == 11025) {
		dac_div = 4;
	} else if (sample_rate == 8000) {
		dac_div = 5;
	} else if (sample_rate == 192000) {
		dac_div = 6;
	} else if (sample_rate == 96000) {
		dac_div = 7;
	} else {
		SND_LOG_ERR("sample_rate invalid:%u\n", sample_rate);
		return -EINVAL;
	}

	sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN,
				 0x7 << DAC_FS, dac_div << DAC_FS);

	if (24576000 % sample_rate == 0)
		rate_div = 24576000 / sample_rate / osr;
	else
		rate_div = 22579200 / sample_rate / osr;

	switch (rate_div) {
	case 1:
		fs_div = 0;
	break;
	case 2:
		fs_div = 1;
	break;
	case 3:
		fs_div = 2;
	break;
	case 4:
		fs_div = 3;
	break;
	case 6:
		fs_div = 4;
	break;
	case 8:
		fs_div = 5;
	break;
	case 12:
		fs_div = 6;
	break;
	case 16:
		fs_div = 7;
	break;
	case 24:
		fs_div = 8;
	break;
	default:
		fs_div = 3;
		SND_LOG_ERR("rate_div invalid:%u, default 3\n", rate_div);
	}

	sunxi_regmap_update_bits(dig_regmap, SUNXI_ADDA_FS_CTL,
				 0xF << ADDA_FS_DIV, fs_div << ADDA_FS_DIV);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADDA_FS_CTL_2,
					0xF << DAC_FS_DIV, fs_div << DAC_FS_DIV);
	else
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADDA_FS_CTL_2,
					0xF << ADC_FS_DIV, fs_div << ADC_FS_DIV);

	/* enable clk after set clk rate */
	if (snd_sunxi_clk_enable(&ac203c->clk)) {
		dev_err(dai->dev, "clk enable failed\n");
		return -EINVAL;
	} else {
		ac203c->clk_sta = SND_SUNXI_CLK_OPEN;
	}

	mutex_lock(&pdata->ac203c_sta.audio_mutex);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (pdata->ac203c_sta.cap == true &&
		    pdata->ac203c_sta.cap_rate != params_rate(params)) {
			SND_LOG_ERR("the rates are inconsistent,cap rate:%u\n",
				    pdata->ac203c_sta.cap_rate);
			return -EINVAL;
		}
		pdata->ac203c_sta.play = true;
		pdata->ac203c_sta.play_rate = params_rate(params);
	} else {
		if (pdata->ac203c_sta.play == true &&
		    pdata->ac203c_sta.play_rate != params_rate(params)) {
			SND_LOG_ERR("the rates are inconsistent,play rate:%u\n",
				    pdata->ac203c_sta.play_rate);
			return -EINVAL;
		}
		pdata->ac203c_sta.cap = true;
		pdata->ac203c_sta.cap_rate = params_rate(params);
	}
	mutex_unlock(&pdata->ac203c_sta.audio_mutex);

	return 0;
}

static int ac203c_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;
	struct sunxi_codec_clk *clk = &ac203c->clk;

	SND_LOG_DEBUG("\n");

	mutex_lock(&pdata->ac203c_sta.audio_mutex);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		pdata->ac203c_sta.play = false;
	else
		pdata->ac203c_sta.cap = false;
	mutex_unlock(&pdata->ac203c_sta.audio_mutex);

	if (ac203c->clk_sta == SND_SUNXI_CLK_OPEN) {
		snd_sunxi_clk_disable(clk);
		ac203c->clk_sta = SND_SUNXI_CLK_CLOSE;
	}

	return 0;
}

static int ac203c_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;

	SND_LOG_DEBUG("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADDA1_FIFO_CTL, 0x1 << FTX, 0x1 << FTX);
		sunxi_regmap_write(dig_regmap, SUNXI_ADDA1_INT_STAT,
				   0x1 << TX_OI | 0x1 << TX_UI | 0x1 << TX_EI);
		sunxi_regmap_write(dig_regmap, SUNXI_ADDA1_DAC_CNT, 0x0);
	} else {
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADDA1_FIFO_CTL, 0x1 << FRX, 0x1 << FRX);
		sunxi_regmap_write(dig_regmap, SUNXI_ADDA1_INT_STAT,
				   0x1 << RX_OI | 0x1 << RX_UI | 0x1 << RX_AI);
		sunxi_regmap_write(dig_regmap, SUNXI_ADDA1_ADC_CNT, 0x0);
	}

	return 0;
}

static int ac203c_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
			  unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct sunxi_codec_clk *clk = &ac203c->clk;

	SND_LOG_DEBUG("stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out);

	return snd_sunxi_clk_rate(clk, pll_id, freq_in, freq_out);
}

static const struct snd_soc_dai_ops ac203c_dai_ops = {
	.set_pll	= ac203c_set_pll,
	.prepare	= ac203c_prepare,
	.trigger	= ac203c_trigger,
	.hw_params	= ac203c_hw_params,
	.hw_free	= ac203c_hw_free,
};

static struct snd_soc_dai_driver ac203c_dai = {
	.name = "ac203c-codec",
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S8
				| SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 4,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S8
				| SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S24_3LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
	.ops = &ac203c_dai_ops,
};

static int ac203c_probe(struct snd_soc_component *component)
{
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	int ret;

	SND_LOG_DEBUG("\n");

	mutex_init(&pdata->ac203c_sta.audio_mutex);

	/* component kcontrols -> pa */
	ret = snd_sunxi_pa_pin_probe(ac203c->pa_cfg, ac203c->pa_pin_max, component);
	if (ret)
		SND_LOG_ERR("register pa kcontrols failed\n");

	/* adda init */
	sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_LOUT_CTL,
				 0x1 << DHP_GAIN_ZC_DEN, 0x1 << DHP_GAIN_ZC_DEN);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_LOUT_CTL,
				 0x1 << LOUT_AUTO_MUTE, 0x1 << LOUT_AUTO_MUTE);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_LOUT_CTL,
				 0x1 << LOUT_AUTO_ATT, 0x1 << LOUT_AUTO_ATT);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG5,
				 0x3 << OPDRV_CUR, 0x3 << OPDRV_CUR);
	/* hp init */
	sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG1, 0x3 << CP_CLKS, 0x3 << CP_CLKS);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG1,
				 0x1 << USB_HP_SEL_CTL, 0x1 << USB_HP_SEL_CTL);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG1,
				 0x3 << HPOUT_SHORT_CTL, 0x3 << HPOUT_SHORT_CTL);

	/* adc dig vol */
	sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DVOL_CTL,
				 0xff << DIG_ADC1_VOL, pdata->adc1_vol << DIG_ADC1_VOL);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DVOL_CTL,
				 0xff << DIG_ADC2_VOL, pdata->adc2_vol << DIG_ADC2_VOL);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DVOL_CTL,
				 0xff << DIG_ADC3_VOL, pdata->adc3_vol << DIG_ADC3_VOL);

	/* dac dig vol*/
	sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DVC,
				 0xff << DAC_DVC_L, pdata->dacl_vol << DAC_DVC_L);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DVC,
				 0xff << DAC_DVC_R, pdata->dacr_vol << DAC_DVC_R);

	/* adc gain*/
	sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC1_REG3,
				 0xf << ADC1_PGA_GAIN_CTL, pdata->mic1_gain << ADC1_PGA_GAIN_CTL);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC2_REG3,
				 0xf << ADC2_PGA_GAIN_CTL, pdata->mic2_gain << ADC2_PGA_GAIN_CTL);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC3_REG3,
				 0xf << ADC3_PGA_GAIN_CTL, pdata->mic3_gain << ADC3_PGA_GAIN_CTL);

	/* dac gain */
	sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DHP_GAIN_CTL,
				 0x7 << DHP_OUT_GAIN, pdata->dac_gain << DHP_OUT_GAIN);

	/* power */
	sunxi_regmap_update_bits(ana_regmap, SUNXI_POWER_REG1,
				 0x1 << BG_BUFEN, 0x1 << BG_BUFEN);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_POWER_REG2,
				 0x1 << OSC_EN, 0x1 << OSC_EN);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_POWER_REG4,
				 0x1 << ALDO_EN | 0x1 << ADDA_BIAS_EN | 0x1 << ALDO_BYPASS,
				 0x1 << ALDO_EN | 0x1 << ADDA_BIAS_EN | 0x0 << ALDO_BYPASS);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_POWER_REG6,
				 0x3 << IOPDRVS, 0x3 << IOPDRVS);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_POWER_REG1,
				 0x1 << VRA1_SPEEDUP_DISABLE, 0x1 << VRA1_SPEEDUP_DISABLE);

	/* i2s */
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_CTL,
				 0x1 << BCLK_TX_IOEN, 0x1 << BCLK_TX_IOEN);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_CTL,
				 0x1 << LRCK_TX_IOEN, 0x1 << LRCK_TX_IOEN);

	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_BCLK_CTL,
				 0x1 << BCLKDIV, 0x1 << BCLKDIV);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_LRCK_CTL,
				 0x3FF << LRCK_PERIOD, 0x7 << LRCK_PERIOD);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_FMT_CTL,
				 0x7 << SW, 0x1 << SW);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_FMT_CTL,
				 0x7 << SR, 0x1 << SR);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_FMT_CTL,
				 0x3 << MODE_SEL, 0x1 << MODE_SEL);

	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_TX_CTL,
				 0xFFFF << TX_CHEN_LOW, 0x3 << TX_CHEN_LOW);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_TX_CTL,
				 0xF << TX_CHSEL, 0x1 << TX_CHSEL);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_CHMP_CTL1,
				 0x3 << TX_CH2_MAP_2, 0x1 << TX_CH2_MAP_2);

	sunxi_regmap_write(dig_regmap, SUNXI_I2S_TX_CHMP_CTL, 0xFFFFFFFF);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_TX_CHMP_CTL,
				 0x3 << TX_CH1_MAP, 0x0 << TX_CH1_MAP);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_TX_CHMP_CTL,
				 0x3 << TX_CH2_MAP, 0x1 << TX_CH2_MAP);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_RX_CTL,
				 0xF << RX_CHSEL, 0x1 << RX_CHSEL);

	sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_CTL_2,
				 0x1 << I2S1_BCLK_IOEN, 0x1 << I2S1_BCLK_IOEN);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_BCLK_CLT1, 0x11);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_LRCK_CLT1, 0x40);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_LRCK_CLT3, 0x07);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_LRCK_CLT4, 0x07);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_FMT_CLT1, 0x11);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S0_FMT_CLT2, 0x11);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S1_FMT_CLT2, 0x11);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_FMT_CLT3, 0x30);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_FMT_CLT4, 0xDD);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_RX_CTL1, 0x01);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_RX_CTL2, 0x03);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_TX_CLT1, 0x01);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_TX_CLT2, 0x03);
	sunxi_regmap_write(ana_regmap, SUNXI_I2S_PADDRV_CTL, 0x15);
	sunxi_regmap_write(ana_regmap, SUNXI_DEBUG_PADDRV_CTL, 0x54);

	/* en */
	sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_CTL_2, 0x1 << TX_EN, 0x1 << TX_EN);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_CTL_2, 0x1 << RX_EN, 0x1 << RX_EN);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_CTL_2, 0x1 << SDO_EN_2, 0x1 << SDO_EN_2);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_CTL_2, 0x1 << I2S_GEN, 0x1 << I2S_GEN);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_CTL, 0x1 << TXEN, 0x1 << TXEN);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_CTL, 0x1 << RXEN, 0x1 << RXEN);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_CTL, 0x1 << SDO_EN, 0x1 << SDO_EN);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_I2S_CTL, 0x1 << I2SGEN, 0x1 << I2SGEN);
	sunxi_regmap_update_bits(dig_regmap, SUNXI_SYS_CLK_CTL,
				 0x1 << SYSCLK_EN, 0x1 << SYSCLK_EN);
	sunxi_regmap_update_bits(ana_regmap, SUNXI_SYSCLK_CTL,
				 0x1 << SYSCLK_EN_2, 0x1 << SYSCLK_EN_2);

	/* disable */
	sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC_DDT_CTL_2,
				 0xF << ADC_DVC_ZCD_EN, 0x0 << ADC_DVC_ZCD_EN);

	return 0;
}

static void ac203c_remove(struct snd_soc_component *component)
{
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;

	mutex_destroy(&pdata->ac203c_sta.audio_mutex);

	snd_sunxi_pa_pin_remove(ac203c->pa_cfg, ac203c->pa_pin_max);
}

static int ac203c_suspend(struct snd_soc_component *component)
{
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	SND_LOG_DEBUG("\n");

	sunxi_save_reg(dig_regmap, &sunxi_dig_reg_group);
	sunxi_save_reg(ana_regmap, &sunxi_ana_reg_group);
	snd_sunxi_regulator_disable(ac203c->rglt);

	snd_sunxi_clk_bus_disable(&ac203c->clk);

	return 0;
}

static int ac203c_resume(struct snd_soc_component *component)
{
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	int ret;

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_regulator_enable(ac203c->rglt);
	if (ret)
		return ret;

	ret = snd_sunxi_clk_bus_enable(&ac203c->clk);
	if (ret)
		return ret;

	sunxi_echo_reg(dig_regmap, &sunxi_dig_reg_group);
	sunxi_echo_reg(ana_regmap, &sunxi_ana_reg_group);

	return 0;
}

static unsigned int sunxi_codec_component_read(struct snd_soc_component *component,
					       unsigned int reg)
{
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	unsigned int reg_val;

	SND_LOG_DEBUG("\n");

	if (reg >= SUNXI_VIR_OFFSET)
		sunxi_regmap_read(ac203c->mem.ana_regmap, reg - SUNXI_VIR_OFFSET, &reg_val);
	else
		sunxi_regmap_read(ac203c->mem.dig_regmap, reg, &reg_val);

	return reg_val;
}

static int sunxi_codec_component_write(struct snd_soc_component *component,
				       unsigned int reg, unsigned int val)
{
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("\n");

	if (reg >= SUNXI_VIR_OFFSET)
		sunxi_regmap_write(ac203c->mem.ana_regmap, reg - SUNXI_VIR_OFFSET, val);
	else
		sunxi_regmap_write(ac203c->mem.dig_regmap, reg, val);

	return 0;
}

static int sunxi_dacl_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_REG1, 0x1 << DACL_EN, 0x1 << DACL_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN, 0x1 << ENDAL, 0x1 << ENDAL);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
					 0x1 << EN_DACL, 0x1 << EN_DACL);
		break;
	case SND_SOC_DAPM_POST_PMD:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_REG1, 0x1 << DACL_EN, 0x0 << DACL_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN, 0x1 << ENDAL, 0x0 << ENDAL);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
					 0x1 << EN_DACL, 0x0 << EN_DACL);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_dacr_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_REG1, 0x1 << DACR_EN, 0x1 << DACR_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN, 0x1 << ENDAR, 0x1 << ENDAR);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
					 0x1 << EN_DACR, 0x1 << EN_DACR);
		break;
	case SND_SOC_DAPM_POST_PMD:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_REG1, 0x1 << DACR_EN, 0x0 << DACR_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN, 0x1 << ENDAR, 0x0 << ENDAR);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
					 0x1 << EN_DACR, 0x0 << EN_DACR);
		break;
	default:
		break;
	}

	return 0;
}

static int ac203c_capture_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		atomic_sub(1, &adc_a_cnt);
		if ((!atomic_read(&adc_a_cnt))) {
			msleep(200);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		atomic_add(1, &adc_a_cnt);
		break;
	default:
		break;
	}

	return 0;
}

static int ac203c_mic1_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC1_REG1,
					 0x1 << ADC1_EN, 0x1 << ADC1_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN, 0x1 << ENAD1, 0x1 << ENAD1);

		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.adc1 = true;
		if (!pdata->ac203c_sta.adc2 && !pdata->ac203c_sta.adc3) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN,
						 0x1 << ADC_EN, 0x1 << ADC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC_DIG_EN_2,
						 0x1 << ADC_EN_2, 0x1 << ADC_EN_2);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);

		break;
	case SND_SOC_DAPM_POST_PMD:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC1_REG1,
					 0x1 << ADC1_EN, 0x0 << ADC1_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN, 0x1 << ENAD1, 0x0 << ENAD1);

		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.adc1 = false;
		if (!pdata->ac203c_sta.adc2 && !pdata->ac203c_sta.adc3) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN,
						 0x1 << ADC_EN, 0x0 << ADC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC_DIG_EN_2,
						 0x1 << ADC_EN_2, 0x0 << ADC_EN_2);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);

		break;
	default:
		break;
	}

	return 0;
}

static int ac203c_mic24_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dac(pga) enable */
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC2_REG1,
					 0x1 << ADC2_EN, 0x1 << ADC2_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN, 0x1 << ENAD2, 0x1 << ENAD2);

		msleep(10);

		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.adc2 = true;
		if (!pdata->ac203c_sta.adc1 && !pdata->ac203c_sta.adc3) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN,
						 0x1 << ADC_EN, 0x1 << ADC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC_DIG_EN_2,
						 0x1 << ADC_EN_2, 0x1 << ADC_EN_2);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);

		break;
	case SND_SOC_DAPM_POST_PMD:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC2_REG1,
					 0x1 << ADC2_EN, 0x0 << ADC2_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN, 0x1 << ENAD2, 0x0 << ENAD2);

		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.adc2 = false;
		if (!pdata->ac203c_sta.adc1 && !pdata->ac203c_sta.adc3) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN,
						 0x1 << ADC_EN, 0x0 << ADC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC_DIG_EN_2,
						 0x1 << ADC_EN_2, 0x0 << ADC_EN_2);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);

		break;
	default:
		break;
	}

	return 0;
}

static int ac203c_mic3_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dac(pga) enable */
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC3_REG1,
					 0x1 << ADC3_EN, 0x1 << ADC3_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN, 0x1 << ENAD3, 0x1 << ENAD3);

		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.adc3 = true;
		if (!pdata->ac203c_sta.adc1 && !pdata->ac203c_sta.adc2) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN,
						 0x1 << ADC_EN, 0x1 << ADC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC_DIG_EN_2,
						 0x1 << ADC_EN_2, 0x1 << ADC_EN_2);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);

		break;
	case SND_SOC_DAPM_POST_PMD:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC3_REG1,
					 0x1 << ADC3_EN, 0x0 << ADC3_EN);
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN, 0x1 << ENAD3, 0x0 << ENAD3);

		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.adc3 = false;
		if (!pdata->ac203c_sta.adc1 && !pdata->ac203c_sta.adc2) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_DIG_EN,
						 0x1 << ADC_EN, 0x0 << ADC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC_DIG_EN_2,
						 0x1 << ADC_EN_2, 0x0 << ADC_EN_2);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);

		break;
	default:
		break;
	}

	return 0;
}

static int ac203c_lineoutl_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_REG3, 0x1 << SPKL_EN, 0x1 << SPKL_EN);

		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.lineoutl = true;
		if (pdata->ac203c_sta.hpout == false && pdata->ac203c_sta.lineoutr == false) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN,
						 0x1 << DAC_EN, 0x1 << DAC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
						 0x1 << EN_DAC, 0x1 << EN_DAC);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);

		break;
	case SND_SOC_DAPM_PRE_PMD:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_REG3, 0x1 << SPKL_EN, 0x0 << SPKL_EN);

		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.lineoutl = false;
		if (pdata->ac203c_sta.hpout == false && pdata->ac203c_sta.lineoutr == false) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN,
						 0x1 << DAC_EN, 0x0 << DAC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
						 0x1 << EN_DAC, 0x0 << EN_DAC);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);
		break;
	default:
		break;
	}

	return 0;
}

static int ac203c_lineoutr_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_REG3, 0x1 << SPKR_EN, 0x1 << SPKR_EN);
		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.lineoutr = true;
		if (pdata->ac203c_sta.hpout == false && pdata->ac203c_sta.lineoutl == false) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN,
						 0x1 << DAC_EN, 0x1 << DAC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
						 0x1 << EN_DAC, 0x1 << EN_DAC);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_REG3, 0x1 << SPKR_EN, 0x0 << SPKR_EN);
		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.lineoutr = false;
		if (pdata->ac203c_sta.hpout == false && pdata->ac203c_sta.lineoutl == false) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN,
						 0x1 << DAC_EN, 0x0 << DAC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
						 0x1 << EN_DAC, 0x0 << EN_DAC);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);
		break;
	default:
		break;
	}

	return 0;
}

static int ac203c_hpout_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct ac203c_data *pdata = &ac203c->pdata;
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;

	SND_LOG_DEBUG("\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.hpout = true;
		if (pdata->ac203c_sta.lineoutl == false || pdata->ac203c_sta.lineoutr == false) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN,
						 0x1 << DAC_EN, 0x1 << DAC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
						 0x1 << EN_DAC, 0x1 << EN_DAC);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);

		sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG1,
					 0x1 << HPOUT_EN, 0x1 << HPOUT_EN);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG1, 0x1 << CP_EN, 0x1 << CP_EN);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG1,
					 0x1 << HPDRV_EN, 0x1 << HPDRV_EN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		mutex_lock(&pdata->ac203c_sta.audio_mutex);
		pdata->ac203c_sta.hpout = false;
		if (pdata->ac203c_sta.lineoutl == false && pdata->ac203c_sta.lineoutr == false) {
			sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_DIG_EN,
						 0x1 << DAC_EN, 0x1 << DAC_EN);
			sunxi_regmap_update_bits(ana_regmap, SUNXI_DAC_DIG_EN_2,
						 0x1 << EN_DAC, 0x1 << EN_DAC);
		}
		mutex_unlock(&pdata->ac203c_sta.audio_mutex);

		sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG1,
					 0x1 << HPOUT_EN, 0x0 << HPOUT_EN);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG1, 0x1 << CP_EN, 0x0 << CP_EN);
		sunxi_regmap_update_bits(ana_regmap, SUNXI_HP_REG1,
					 0x1 << HPDRV_EN, 0x0 << HPDRV_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int ac203c_spk_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);

	SND_LOG_DEBUG("event:%d\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_sunxi_pa_pin_enable(ac203c->pa_cfg,
					ac203c->pa_pin_max);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		snd_sunxi_pa_pin_disable(ac203c->pa_cfg,
					 ac203c->pa_pin_max);
		break;
	default:
		break;
	}

	return 0;
}

static int ac203c_get_adc1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(dig_regmap, SUNXI_ADC_MUX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_ADC1_MUX) & 0x7;

	return 0;
}

static int ac203c_set_adc1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC1_MUX,
					 0x0 << DIG_ADC1_MUX);
		break;
	case 1:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC1_MUX,
					 0x1 << DIG_ADC1_MUX);
		break;
	case 2:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC1_MUX,
					 0x2 << DIG_ADC1_MUX);
		break;
	case 3:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC1_MUX,
					 0x3 << DIG_ADC1_MUX);
		break;
	case 4:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC1_MUX,
					 0x4 << DIG_ADC1_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac203c_get_adc2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(dig_regmap, SUNXI_ADC_MUX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_ADC2_MUX) & 0x7;

	return 0;
}

static int ac203c_set_adc2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC2_MUX,
					 0x0 << DIG_ADC2_MUX);
		break;
	case 1:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC2_MUX,
					 0x1 << DIG_ADC2_MUX);
		break;
	case 2:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC2_MUX,
					 0x2 << DIG_ADC2_MUX);
		break;
	case 3:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC2_MUX,
					 0x3 << DIG_ADC2_MUX);
		break;
	case 4:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC2_MUX,
					 0x4 << DIG_ADC2_MUX);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ac203c_get_adc3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(dig_regmap, SUNXI_ADC_MUX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_ADC2_MUX) & 0x7;

	return 0;
}

static int ac203c_set_adc3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC3_MUX,
					 0x0 << DIG_ADC3_MUX);
		break;
	case 1:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC3_MUX,
					 0x1 << DIG_ADC3_MUX);
		break;
	case 2:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC3_MUX,
					 0x2 << DIG_ADC3_MUX);
		break;
	case 3:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC3_MUX,
					 0x3 << DIG_ADC3_MUX);
		break;
	case 4:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_ADC_MUX_CTL,
					 0x7 << DIG_ADC3_MUX,
					 0x4 << DIG_ADC3_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

//adc input
static int ac203c_get_adc2_input_src(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(ana_regmap, SUNXI_ADC2_REG1, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> ADC2_MIC_MIX_MUX2) & 0x3;

	return 0;
}

static int ac203c_set_adc2_input_src(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC2_REG1,
					 0x3 << ADC2_MIC_MIX_MUX2,
					 0x0 << ADC2_MIC_MIX_MUX2);
		break;
	case 1:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC2_REG1,
					 0x3 << ADC2_MIC_MIX_MUX2,
					 0x1 << ADC2_MIC_MIX_MUX2);
		break;
	case 2:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC2_REG1,
					 0x3 << ADC2_MIC_MIX_MUX2,
					 0x2 << ADC2_MIC_MIX_MUX2);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac203c_get_adc3_input_src(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(ana_regmap, SUNXI_ADC3_REG1, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val & (0x1 << ADC3_MIC_MIX));

	return 0;
}

static int ac203c_set_adc3_input_src(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC3_REG1,
					 0x1 << ADC3_MIC_MIX,
					 0x0 << ADC3_MIC_MIX);
		break;
	case 1:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_ADC3_REG1,
					 0x1 << ADC3_MIC_MIX,
					 0x1 << ADC3_MIC_MIX);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

//rxm src
static int ac203c_get_rxm1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(ana_regmap, SUNXI_I2S_RX_MIX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> RX_MIX1) & 0x3;

	return 0;
}

static int ac203c_set_rxm1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];
	switch (val) {
	case 0:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX1,
					 0x0 << RX_MIX1);
		break;
	case 1:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX1,
					 0x1 << RX_MIX1);
		break;
	case 2:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX1,
					 0x2 << RX_MIX1);
		break;
	case 3:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX1,
					 0x3 << RX_MIX1);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac203c_get_rxm2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(ana_regmap, SUNXI_I2S_RX_MIX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> RX_MIX2) & 0x3;

	return 0;
}

static int ac203c_set_rxm2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX2,
					 0x0 << RX_MIX2);
		break;
	case 1:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX2,
					 0x1 << RX_MIX2);
		break;
	case 2:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX2,
					 0x2 << RX_MIX2);
		break;
	case 3:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX2,
					 0x3 << RX_MIX2);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac203c_get_rxm3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(ana_regmap, SUNXI_I2S_RX_MIX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> RX_MIX3) & 0x3;

	return 0;
}

static int ac203c_set_rxm3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX3,
					 0x0 << RX_MIX3);
		break;
	case 1:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX3,
					 0x1 << RX_MIX3);
		break;
	case 2:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX3,
					 0x2 << RX_MIX3);
		break;
	case 3:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_RX_MIX_CTL,
					 0x3 << RX_MIX3,
					 0x3 << RX_MIX3);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

//txm src
static int ac203c_get_txm1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(ana_regmap, SUNXI_I2S_TX_MIX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> TX_MIX1) & 0x3;

	return 0;
}

static int ac203c_set_txm1_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX1,
					 0x0 << TX_MIX1);
		break;
	case 1:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX1,
					 0x1 << TX_MIX1);
		break;
	case 2:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX1,
					 0x2 << TX_MIX1);
		break;
	case 3:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX1,
					 0x3 << TX_MIX1);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac203c_get_txm2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(ana_regmap, SUNXI_I2S_TX_MIX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> TX_MIX2) & 0x3;

	return 0;
}

static int ac203c_set_txm2_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX2,
					 0x0 << TX_MIX2);
		break;
	case 1:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX2,
					 0x1 << TX_MIX2);
		break;
	case 2:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX2,
					 0x2 << TX_MIX2);
		break;
	case 3:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX2,
					 0x3 << TX_MIX2);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac203c_get_txm3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(ana_regmap, SUNXI_I2S_TX_MIX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> TX_MIX3) & 0x3;

	return 0;
}

static int ac203c_set_txm3_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *ana_regmap = ac203c->mem.ana_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX3,
					 0x0 << TX_MIX3);
		break;
	case 1:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX3,
					 0x1 << TX_MIX3);
		break;
	case 2:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX3,
					 0x2 << TX_MIX3);
		break;
	case 3:
		sunxi_regmap_update_bits(ana_regmap, SUNXI_I2S_TX_MIX_CTL,
					 0x3 << TX_MIX3,
					 0x3 << TX_MIX3);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

//dac src
static int ac203c_get_dacl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(dig_regmap, SUNXI_DAC_MUX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_DAC1_MUX) & 0x3;

	return 0;
}

static int ac203c_set_dacl_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_MUX_CTL,
					 0x3 << DIG_DAC1_MUX,
					 0x0 << DIG_DAC1_MUX);
		break;
	case 1:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_MUX_CTL,
					 0x3 << DIG_DAC1_MUX,
					 0x1 << DIG_DAC1_MUX);
		break;
	case 2:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_MUX_CTL,
					 0x3 << DIG_DAC1_MUX,
					 0x2 << DIG_DAC1_MUX);
		break;
	case 3:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_MUX_CTL,
					 0x3 << DIG_DAC1_MUX,
					 0x3 << DIG_DAC1_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int ac203c_get_dacr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int reg_val;

	sunxi_regmap_read(dig_regmap, SUNXI_DAC_MUX_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> DIG_DAC2_MUX) & 0x3;

	return 0;
}

static int ac203c_set_dacr_src(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	unsigned int val;

	val = ucontrol->value.integer.value[0];

	switch (val) {
	case 0:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_MUX_CTL,
					 0x3 << DIG_DAC2_MUX,
					 0x0 << DIG_DAC2_MUX);
		break;
	case 1:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_MUX_CTL,
					 0x3 << DIG_DAC2_MUX,
					 0x1 << DIG_DAC2_MUX);
		break;
	case 2:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_MUX_CTL,
					 0x3 << DIG_DAC2_MUX,
					 0x2 << DIG_DAC2_MUX);
		break;
	case 3:
		sunxi_regmap_update_bits(dig_regmap, SUNXI_DAC_MUX_CTL,
					 0x3 << DIG_DAC2_MUX,
					 0x3 << DIG_DAC2_MUX);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

//dap
static int ac203c_get_dap_status(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int shift = e->shift_l;
	unsigned int reg_val;

	switch (shift) {
	case ADCHPF1_SHIFT:
		regmap_read(dig_regmap, SUNXI_HPF_EN, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << DIG_ADC1_HPF_EN) ? 1 : 0;
		break;
	case ADCHPF2_SHIFT:
		regmap_read(dig_regmap, SUNXI_HPF_EN, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << DIG_ADC2_HPF_EN) ? 1 : 0;
		break;
	case ADCHPF3_SHIFT:
		regmap_read(dig_regmap, SUNXI_HPF_EN, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << DIG_ADC3_HPF_EN) ? 1 : 0;
		break;
	case DACDRC_SHIFT:
		regmap_read(dig_regmap, SUNXI_DRC_ENA, &reg_val);
		ucontrol->value.integer.value[0] =
			(reg_val & 0x1 << DRC_CAL_ENA) && (reg_val & 0x1 << DRC_HPF_ENA) ? 1 : 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int ac203c_set_dap_status(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct ac203c_priv *ac203c = snd_soc_component_get_drvdata(component);
	struct regmap *dig_regmap = ac203c->mem.dig_regmap;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int shift = e->shift_l;

	switch (shift) {
	case ADCHPF1_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(dig_regmap, SUNXI_HPF_EN,
					   0x1 << DIG_ADC1_HPF_EN, 0x1 << DIG_ADC1_HPF_EN);
		} else {
			regmap_update_bits(dig_regmap, SUNXI_HPF_EN,
					   0x1 << DIG_ADC1_HPF_EN, 0x0 << DIG_ADC1_HPF_EN);
		}
		break;
	case ADCHPF2_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(dig_regmap, SUNXI_HPF_EN,
					   0x1 << DIG_ADC2_HPF_EN, 0x1 << DIG_ADC2_HPF_EN);
		} else {
			regmap_update_bits(dig_regmap, SUNXI_HPF_EN,
					   0x1 << DIG_ADC2_HPF_EN, 0x0 << DIG_ADC2_HPF_EN);
		}
		break;
	case ADCHPF3_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(dig_regmap, SUNXI_HPF_EN,
					   0x1 << DIG_ADC3_HPF_EN, 0x1 << DIG_ADC3_HPF_EN);
		} else {
			regmap_update_bits(dig_regmap, SUNXI_HPF_EN,
					   0x1 << DIG_ADC3_HPF_EN, 0x0 << DIG_ADC3_HPF_EN);
		}
		break;
	case DACDRC_SHIFT:
		if (ucontrol->value.integer.value[0]) {
			regmap_update_bits(dig_regmap, SUNXI_DRC_ENA,
					   0x1 << DRC_CAL_ENA | 0x1 << DRC_HPF_ENA,
					   0x1 << DRC_CAL_ENA | 0x1 << DRC_HPF_ENA);
		} else {
			regmap_update_bits(dig_regmap, SUNXI_DRC_ENA,
					   0x1 << DRC_CAL_ENA | 0x1 << DRC_HPF_ENA,
					   0x0 << DRC_CAL_ENA | 0x0 << DRC_HPF_ENA);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

//name, min, step, mute
static const DECLARE_TLV_DB_SCALE(adc_dig_vol_tlv, -6400, 50, 1);
static const DECLARE_TLV_DB_SCALE(dac_dig_vol_tlv, -6400, 50, 1);
static const DECLARE_TLV_DB_SCALE(dac_gain_tlv, -4200, 600, 0);
static const DECLARE_TLV_DB_SCALE(adc_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(hp_gain_tlv, -4200, 600, 0);

static const char *sunxi_switch_text[] = {"Off", "On"};

/* adc dig */
static const char * const adc1_data_src_mux_text[] = {
	"Debug_Data", "MIC1", "DACL_Data", "DACR_Data", "RXM1"
};
static const char * const adc2_data_src_mux_text[] = {
	"Debug_Data", "ADC2_Input", "DACL_Data", "DACR_Data", "RXM1"
};
static const char * const adc3_data_src_mux_text[] = {
	"Debug_Data", "ADC3_Input", "DACL_Data", "DACR_Data", "RXM1"
};

/* I2S_TX_MIX_CTRL */
static const char * const txm1_data_src_mux_text[] = {
	"ADC1_Data", "DACL_Data", "ADC1_DACL_Data", "ADC1_DACL_Data_AVG"
};

static const char * const txm2_data_src_mux_text[] = {
	"ADC2_Data", "DACR_Data", "ADC2_DACR_Data", "ADC2_DACR_Data_AVG"
};

static const char * const txm3_data_src_mux_text[] = {
	"ADC3_Data", "RXM1", "ADC3_RXM1_Data", "ADC3_RXM1_Data_AVG"
};

static const char * const adc2_input_src_mux_text[] = {
	"MIC2", "MIC4", "LINEOUTL"
};

static const char * const adc3_input_src_mux_text[] = {
	"MIC3", "LINEOUTR"
};

static const char * const rxm1_data_src_mux_text[] = {
	"AIF_RXL", "AIF_RXR", "AIF_RXL_RXR", "AIF_RXL_RXR_AVG"
};

static const char * const rxm2_data_src_mux_text[] = {
	"RXM1", "ADC1_Data", "RXM1_ADC1", "RXM1_ADC1_AVG"
};

static const char * const rxm3_data_src_mux_text[] = {
	"AIF_RXR", "ADC2_Data", "AIFRXR_ADC2", "AIFRXR_ADC2_AVG"
};

static const char * const dac1_data_src_mux_text[] = {
	"RXM2", "-6dB_Sine", "-60dB_Sine", "Zero"
};

static const char * const dac2_data_src_mux_text[] = {
	"RXM3", "-6dB_Sine", "-60dB_Sine", "Zero"
};

static SOC_ENUM_SINGLE_DECL(sunxi_dac_swap_enum, SUNXI_DAC_DIG_CTL, DAC_SWP, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adchpf1_sta_enum, SND_SOC_NOPM, ADCHPF1_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adchpf2_sta_enum, SND_SOC_NOPM, ADCHPF2_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_adchpf3_sta_enum, SND_SOC_NOPM, ADCHPF3_SHIFT, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_dacdrc_sta_enum, SND_SOC_NOPM, DACDRC_SHIFT, sunxi_switch_text);

//adc src
static const struct soc_enum adc1_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_ADC_MUX_CTL, DIG_ADC1_MUX,
			ARRAY_SIZE(adc1_data_src_mux_text),
			adc1_data_src_mux_text);
static const struct soc_enum adc2_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_ADC_MUX_CTL, DIG_ADC2_MUX,
			ARRAY_SIZE(adc2_data_src_mux_text),
			adc2_data_src_mux_text);
static const struct soc_enum adc3_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_ADC_MUX_CTL, DIG_ADC3_MUX,
			ARRAY_SIZE(adc3_data_src_mux_text),
			adc3_data_src_mux_text);

//adc input src
static const struct soc_enum adc2_input_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_ADC2_REG1, ADC2_MIC_MIX_MUX2,
			ARRAY_SIZE(adc2_input_src_mux_text),
			adc2_input_src_mux_text);
static const struct soc_enum adc3_input_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_ADC3_REG1, ADC3_MIC_MIX,
			ARRAY_SIZE(adc3_input_src_mux_text),
			adc3_input_src_mux_text);

//rxm src
static const struct soc_enum rxm1_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_I2S_RX_MIX_CTL, RX_MIX1,
			ARRAY_SIZE(rxm1_data_src_mux_text),
			rxm1_data_src_mux_text);
static const struct soc_enum rxm2_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_I2S_RX_MIX_CTL, RX_MIX2,
			ARRAY_SIZE(rxm2_data_src_mux_text),
			rxm2_data_src_mux_text);
static const struct soc_enum rxm3_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_I2S_RX_MIX_CTL, RX_MIX3,
			ARRAY_SIZE(rxm3_data_src_mux_text),
			rxm3_data_src_mux_text);

//txm src
static const struct soc_enum txm1_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_I2S_TX_MIX_CTL, TX_MIX1,
			ARRAY_SIZE(txm1_data_src_mux_text),
			txm1_data_src_mux_text);
static const struct soc_enum txm2_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_I2S_TX_MIX_CTL, TX_MIX2,
			ARRAY_SIZE(txm2_data_src_mux_text),
			txm2_data_src_mux_text);
static const struct soc_enum txm3_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_I2S_TX_MIX_CTL, TX_MIX3,
			ARRAY_SIZE(txm3_data_src_mux_text),
			txm3_data_src_mux_text);

//dac src
static const struct soc_enum dac1_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_DAC_MUX_CTL, DIG_DAC1_MUX,
			ARRAY_SIZE(dac1_data_src_mux_text),
			dac1_data_src_mux_text);
static const struct soc_enum dac2_src_mux_enum =
	SOC_ENUM_SINGLE(SUNXI_DAC_MUX_CTL, DIG_DAC2_MUX,
			ARRAY_SIZE(dac2_data_src_mux_text),
			dac2_data_src_mux_text);

static const struct snd_kcontrol_new adc1_src_mux =
	SOC_DAPM_ENUM_EXT("ADC1 Data Select Mux", adc1_src_mux_enum,
			  ac203c_get_adc1_src,
			  ac203c_set_adc1_src);
static const struct snd_kcontrol_new adc2_src_mux =
	SOC_DAPM_ENUM_EXT("ADC2 Data Select Mux", adc2_src_mux_enum,
			  ac203c_get_adc2_src,
			  ac203c_set_adc2_src);
static const struct snd_kcontrol_new adc3_src_mux =
	SOC_DAPM_ENUM_EXT("ADC3 Data Select Mux", adc3_src_mux_enum,
			  ac203c_get_adc3_src,
			  ac203c_set_adc3_src);

static const struct snd_kcontrol_new adc2_input_src_mux =
	SOC_DAPM_ENUM_EXT("ADC2 Input Src Mux", adc2_input_mux_enum,
			  ac203c_get_adc2_input_src,
			  ac203c_set_adc2_input_src);
static const struct snd_kcontrol_new adc3_input_src_mux =
	SOC_DAPM_ENUM_EXT("ADC3 Input Src Mux", adc3_input_mux_enum,
			  ac203c_get_adc3_input_src,
			  ac203c_set_adc3_input_src);

static const struct snd_kcontrol_new rxm1_src_mux =
	SOC_DAPM_ENUM_EXT("RX1 Mux", rxm1_src_mux_enum,
			  ac203c_get_rxm1_src,
			  ac203c_set_rxm1_src);
static const struct snd_kcontrol_new rxm2_src_mux =
	SOC_DAPM_ENUM_EXT("RX2 Mux", rxm2_src_mux_enum,
			  ac203c_get_rxm2_src,
			  ac203c_set_rxm2_src);
static const struct snd_kcontrol_new rxm3_src_mux =
	SOC_DAPM_ENUM_EXT("RX3 Mux", rxm3_src_mux_enum,
			  ac203c_get_rxm3_src,
			  ac203c_set_rxm3_src);

static const struct snd_kcontrol_new txm1_src_mux =
	SOC_DAPM_ENUM_EXT("AIF TX1 Mux", txm1_src_mux_enum,
			  ac203c_get_txm1_src,
			  ac203c_set_txm1_src);
static const struct snd_kcontrol_new txm2_src_mux =
	SOC_DAPM_ENUM_EXT("AIF TX2 Mux", txm2_src_mux_enum,
			  ac203c_get_txm2_src,
			  ac203c_set_txm2_src);
static const struct snd_kcontrol_new txm3_src_mux =
	SOC_DAPM_ENUM_EXT("AIF TX3 Mux", txm3_src_mux_enum,
			  ac203c_get_txm3_src,
			  ac203c_set_txm3_src);

static const struct snd_kcontrol_new dacl_src_mux =
	SOC_DAPM_ENUM_EXT("DACL Data Select Mux", dac1_src_mux_enum,
			  ac203c_get_dacl_src,
			  ac203c_set_dacl_src);
static const struct snd_kcontrol_new dacr_src_mux =
	SOC_DAPM_ENUM_EXT("DACR Data Select Mux", dac2_src_mux_enum,
			  ac203c_get_dacr_src,
			  ac203c_set_dacr_src);

static const struct snd_kcontrol_new ac203c_snd_controls[] = {
	/* adc dig vol*/
	SOC_SINGLE_TLV("ADC1 Volume", SUNXI_ADC_DVOL_CTL, DIG_ADC1_VOL, 0xff, 0, adc_dig_vol_tlv),
	SOC_SINGLE_TLV("ADC2 Volume", SUNXI_ADC_DVOL_CTL, DIG_ADC2_VOL, 0xff, 0, adc_dig_vol_tlv),
	SOC_SINGLE_TLV("ADC3 Volume", SUNXI_ADC_DVOL_CTL, DIG_ADC3_VOL, 0xff, 0, adc_dig_vol_tlv),
	/* dac dig vol */
	SOC_SINGLE_TLV("DACL Volume", SUNXI_DAC_DVC, DAC_DVC_L, 0xff, 0, dac_dig_vol_tlv),
	SOC_SINGLE_TLV("DACR Volume", SUNXI_DAC_DVC, DAC_DVC_R, 0xff, 0, dac_dig_vol_tlv),
	/* dac gain */
	SOC_SINGLE_TLV("DAC Gain", SUNXI_DAC_DHP_GAIN_CTL, DHP_OUT_GAIN, 0x7, 1, dac_gain_tlv),
	/* adc gain */
	SOC_SINGLE_TLV("ADC1 Gain", SUNXI_ADC1_REG3, ADC1_PGA_GAIN_CTL, 0xf, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("ADC2 Gain", SUNXI_ADC2_REG3, ADC2_PGA_GAIN_CTL, 0xf, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("ADC3 Gain", SUNXI_ADC3_REG3, ADC3_PGA_GAIN_CTL, 0xf, 0, adc_gain_tlv),

	/* swap */
	SOC_ENUM("DACL DACR Swap", sunxi_dac_swap_enum),

	/* DAP Func */
	SOC_ENUM_EXT("ADC HPF1 Mode", sunxi_adchpf1_sta_enum, ac203c_get_dap_status,
		     ac203c_set_dap_status),
	SOC_ENUM_EXT("ADC HPF2 Mode", sunxi_adchpf2_sta_enum, ac203c_get_dap_status,
		     ac203c_set_dap_status),
	SOC_ENUM_EXT("ADC HPF3 Mode", sunxi_adchpf3_sta_enum, ac203c_get_dap_status,
		     ac203c_set_dap_status),
	SOC_ENUM_EXT("DAC DRC Mode", sunxi_dacdrc_sta_enum, ac203c_get_dap_status,
		     ac203c_set_dap_status),
};

static const struct snd_soc_dapm_widget ac203c_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MIC1P_PIN"),
	SND_SOC_DAPM_INPUT("MIC1N_PIN"),
	SND_SOC_DAPM_INPUT("MIC2P_PIN"),
	SND_SOC_DAPM_INPUT("MIC2N_PIN"),
	SND_SOC_DAPM_INPUT("MIC3P_PIN"),
	SND_SOC_DAPM_INPUT("MIC3N_PIN"),
	SND_SOC_DAPM_INPUT("MIC4P_PIN"),
	SND_SOC_DAPM_INPUT("MIC4N_PIN"),

	// SND_SOC_DAPM_INPUT("DMIC"),
	SND_SOC_DAPM_INPUT("Debug_Data"),

	SND_SOC_DAPM_OUTPUT("LINEOUTLP_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTLN_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTRP_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTRN_PIN"),
	SND_SOC_DAPM_OUTPUT("HPOUT_PIN"),

	SND_SOC_DAPM_AIF_IN_E("AIF RXL", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0,
			      sunxi_dacl_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("AIF RXR", "AIF2 Playback", 0, SND_SOC_NOPM, 0, 0,
			      sunxi_dacr_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT("AIF TX1", "AIF Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF TX2", "AIF Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF TX3", "AIF Capture", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_ADC_E("ADC1", NULL, SND_SOC_NOPM, 0, 0,
			   ac203c_capture_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_ADC_E("ADC2", NULL, SND_SOC_NOPM, 0, 0,
			   ac203c_capture_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_ADC_E("ADC3", NULL, SND_SOC_NOPM, 0, 0,
			   ac203c_capture_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_DAC("DACL", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DACR", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_MICBIAS("MICBIAS", SUNXI_MBIAS_REG, MBIAS_EN, 0),
	SND_SOC_DAPM_MICBIAS("MICBIAS CHOP", SUNXI_MBIAS_REG, MBIAS_CHOPPER_EN, 0),

	SND_SOC_DAPM_MUX("ADC2 Input Src Mux", SND_SOC_NOPM, 0, 0, &adc2_input_src_mux),
	SND_SOC_DAPM_MUX("ADC3 Input Src Mux", SND_SOC_NOPM, 0, 0, &adc3_input_src_mux),

	SND_SOC_DAPM_MUX("ADC1 Data Select Mux", SND_SOC_NOPM, 0, 0, &adc1_src_mux),
	SND_SOC_DAPM_MUX("ADC2 Data Select Mux", SND_SOC_NOPM, 0, 0, &adc2_src_mux),
	SND_SOC_DAPM_MUX("ADC3 Data Select Mux", SND_SOC_NOPM, 0, 0, &adc3_src_mux),

	//i2s
	SND_SOC_DAPM_MUX("AIF TX1 Mux", SND_SOC_NOPM, 0, 0, &txm1_src_mux),
	SND_SOC_DAPM_MUX("AIF TX2 Mux", SND_SOC_NOPM, 0, 0, &txm2_src_mux),
	SND_SOC_DAPM_MUX("AIF TX3 Mux", SND_SOC_NOPM, 0, 0, &txm3_src_mux),

	SND_SOC_DAPM_MUX("RX1 Mux", SND_SOC_NOPM, 0, 0, &rxm1_src_mux),
	SND_SOC_DAPM_MUX("RX2 Mux", SND_SOC_NOPM, 0, 0, &rxm2_src_mux),
	SND_SOC_DAPM_MUX("RX3 Mux", SND_SOC_NOPM, 0, 0, &rxm3_src_mux),

	SND_SOC_DAPM_MUX("DACL Data Select Mux", SND_SOC_NOPM, 0, 0, &dacl_src_mux),
	SND_SOC_DAPM_MUX("DACR Data Select Mux", SND_SOC_NOPM, 0, 0, &dacr_src_mux),

	SND_SOC_DAPM_SPK("LINEOUTL", ac203c_lineoutl_event),
	SND_SOC_DAPM_SPK("LINEOUTR", ac203c_lineoutr_event),

	/* for pa */
	SND_SOC_DAPM_SPK("SPK", ac203c_spk_event),

	SND_SOC_DAPM_HP("HPOUT", ac203c_hpout_event),

	SND_SOC_DAPM_MIC("MIC1", ac203c_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", ac203c_mic24_event),
	SND_SOC_DAPM_MIC("MIC3", ac203c_mic3_event),
	SND_SOC_DAPM_MIC("MIC4", ac203c_mic24_event),
};

static const struct snd_soc_dapm_route ac203c_dapm_routes[] = {
	{"MICBIAS", NULL, "MIC1P_PIN"},
	{"MICBIAS", NULL, "MIC1N_PIN"},
	{"MICBIAS", NULL, "MIC2P_PIN"},
	{"MICBIAS", NULL, "MIC2N_PIN"},
	{"MICBIAS", NULL, "MIC3P_PIN"},
	{"MICBIAS", NULL, "MIC3N_PIN"},
	{"MICBIAS", NULL, "MIC4P_PIN"},
	{"MICBIAS", NULL, "MIC4N_PIN"},

	{"MICBIAS CHOP", NULL, "MICBIAS"},

	{"ADC2 Input Src Mux", "MIC2", "MICBIAS CHOP"},
	{"ADC2 Input Src Mux", "MIC4", "MICBIAS CHOP"},
	{"ADC2 Input Src Mux", "LINEOUTL", "LINEOUTLP_PIN"},
	{"ADC2 Input Src Mux", "LINEOUTL", "LINEOUTLN_PIN"},

	{"ADC3 Input Src Mux", "MIC3", "MICBIAS CHOP"},
	{"ADC3 Input Src Mux", "LINEOUTR", "LINEOUTRP_PIN"},
	{"ADC3 Input Src Mux", "LINEOUTR", "LINEOUTRN_PIN"},

	{"ADC1 Data Select Mux", "Debug_Data", "Debug_Data"},
	{"ADC1 Data Select Mux", "MIC1", "MICBIAS CHOP"},
	{"ADC1 Data Select Mux", "DACL_Data", "DACL Data Select Mux"},
	{"ADC1 Data Select Mux", "DACR_Data", "DACR Data Select Mux"},
	{"ADC1 Data Select Mux", "RXM1", "RX1 Mux"},

	{"ADC2 Data Select Mux", "Debug_Data", "Debug_Data"},
	{"ADC2 Data Select Mux", "ADC2_Input", "ADC2 Input Src Mux"},
	{"ADC2 Data Select Mux", "DACL_Data", "DACL Data Select Mux"},
	{"ADC2 Data Select Mux", "DACR_Data", "DACR Data Select Mux"},
	{"ADC2 Data Select Mux", "RXM1", "RX1 Mux"},

	{"ADC3 Data Select Mux", "Debug_Data", "Debug_Data"},
	{"ADC3 Data Select Mux", "ADC3_Input", "ADC3 Input Src Mux"},
	{"ADC3 Data Select Mux", "DACL_Data", "DACL Data Select Mux"},
	{"ADC3 Data Select Mux", "DACR_Data", "DACR Data Select Mux"},
	{"ADC3 Data Select Mux", "RXM1", "RX1 Mux"},

	{"ADC1", NULL, "ADC1 Data Select Mux"},
	{"ADC2", NULL, "ADC2 Data Select Mux"},
	{"ADC3", NULL, "ADC3 Data Select Mux"},

	{"AIF TX1 Mux", "ADC1_Data", "ADC1"},
	{"AIF TX1 Mux", "DACL_Data", "DACL"},
	{"AIF TX1 Mux", "ADC1_DACL_Data", "ADC1"},
	{"AIF TX1 Mux", "ADC1_DACL_Data", "DACL"},
	{"AIF TX1 Mux", "ADC1_DACL_Data_AVG", "ADC1"},
	{"AIF TX1 Mux", "ADC1_DACL_Data_AVG", "DACL"},

	{"AIF TX2 Mux", "ADC2_Data", "ADC2"},
	{"AIF TX2 Mux", "DACR_Data", "DACR"},
	{"AIF TX2 Mux", "ADC2_DACR_Data", "ADC2"},
	{"AIF TX2 Mux", "ADC2_DACR_Data", "DACR"},
	{"AIF TX2 Mux", "ADC2_DACR_Data_AVG", "ADC2"},
	{"AIF TX2 Mux", "ADC2_DACR_Data_AVG", "DACR"},

	{"AIF TX3 Mux", "ADC3_Data", "ADC3"},
	{"AIF TX3 Mux", "RXM1", "RX1 Mux"},
	{"AIF TX3 Mux", "ADC3_RXM1_Data", "ADC3"},
	{"AIF TX3 Mux", "ADC3_RXM1_Data", "RX1 Mux"},
	{"AIF TX3 Mux", "ADC3_RXM1_Data_AVG", "ADC3"},
	{"AIF TX3 Mux", "ADC3_RXM1_Data_AVG", "RX1 Mux"},

	{"AIF TX1", NULL, "AIF TX1 Mux"},
	{"AIF TX2", NULL, "AIF TX2 Mux"},
	{"AIF TX3", NULL, "AIF TX3 Mux"},

	/* dac route */
	{"RX1 Mux", "AIF_RXL", "AIF RXL"},
	{"RX1 Mux", "AIF_RXR", "AIF RXR"},
	{"RX1 Mux", "AIF_RXL_RXR", "AIF RXL"},
	{"RX1 Mux", "AIF_RXL_RXR", "AIF RXR"},
	{"RX1 Mux", "AIF_RXL_RXR_AVG", "AIF RXL"},
	{"RX1 Mux", "AIF_RXL_RXR_AVG", "AIF RXR"},

	{"RX2 Mux", "RXM1", "RX1 Mux"},
	{"RX2 Mux", "ADC1_Data", "ADC1"},
	{"RX2 Mux", "RXM1_ADC1", "RX1 Mux"},
	{"RX2 Mux", "RXM1_ADC1", "ADC1"},
	{"RX2 Mux", "RXM1_ADC1_AVG", "RX1 Mux"},
	{"RX2 Mux", "RXM1_ADC1_AVG", "ADC1"},

	{"RX3 Mux", "AIF_RXR", "AIF RXR"},
	{"RX3 Mux", "ADC2_Data", "ADC2"},
	{"RX3 Mux", "AIFRXR_ADC2", "AIF RXR"},
	{"RX3 Mux", "AIFRXR_ADC2", "ADC2"},
	{"RX3 Mux", "AIFRXR_ADC2_AVG", "AIF RXR"},
	{"RX3 Mux", "AIFRXR_ADC2_AVG", "ADC2"},

	{"DACL Data Select Mux", "RXM2", "RX2 Mux"},
	{"DACL Data Select Mux", "-6dB_Sine", "Debug_Data"},
	{"DACL Data Select Mux", "-60dB_Sine", "Debug_Data"},
	{"DACL Data Select Mux", "Zero", "Debug_Data"},

	{"DACR Data Select Mux", "RXM3", "RX3 Mux"},
	{"DACR Data Select Mux", "-6dB_Sine", "Debug_Data"},
	{"DACR Data Select Mux", "-60dB_Sine", "Debug_Data"},
	{"DACR Data Select Mux", "Zero", "Debug_Data"},

	{"DACL", NULL, "DACL Data Select Mux"},
	{"DACR", NULL, "DACR Data Select Mux"},

	{"LINEOUTLP_PIN", NULL, "DACL"},
	{"LINEOUTLN_PIN", NULL, "DACL"},
	{"LINEOUTRP_PIN", NULL, "DACR"},
	{"LINEOUTRN_PIN", NULL, "DACR"},
	{"HPOUT_PIN", NULL, "DACL"},
	{"HPOUT_PIN", NULL, "DACR"},
};

static const struct snd_soc_component_driver soc_component_dev_ac203c = {
	.probe			= ac203c_probe,
	.remove			= ac203c_remove,
	.suspend		= ac203c_suspend,
	.resume			= ac203c_resume,
	.read			= sunxi_codec_component_read,
	.write			= sunxi_codec_component_write,
	.controls		= ac203c_snd_controls,
	.num_controls		= ARRAY_SIZE(ac203c_snd_controls),
	.dapm_widgets		= ac203c_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ac203c_dapm_widgets),
	.dapm_routes		= ac203c_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(ac203c_dapm_routes),
};

static const struct regmap_config ac203c_dig_regmap = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_AUDIO_DIG_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static const struct regmap_config ac203c_ana_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = SUNXI_AUDIO_ANA_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static int snd_sunxi_mem_init(struct i2c_client *i2c, struct platform_device *pdev,
			      struct sunxi_codec_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	uint32_t temp_val;

	SND_LOG_DEBUG("\n");

	mem->ana_regmap = devm_regmap_init_i2c(i2c, &ac203c_ana_regmap);
	if (IS_ERR_OR_NULL(mem->ana_regmap)) {
		SND_LOG_ERR("ana_regmap init failed\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "bus-reg-start", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("bus-reg-start get failed\n");
		return -EINVAL;
	} else {
		mem->res.start = temp_val;
	}

	ret = of_property_read_u32(np, "bus-reg-offset", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("bus-reg-offset get failed\n");
		return -EINVAL;
	} else {
		mem->res.end = mem->res.start + temp_val;
	}

	mem->res.flags = IORESOURCE_MEM;

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res), DRV_NAME);
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

	mem->dig_regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &ac203c_dig_regmap);
	if (IS_ERR_OR_NULL(mem->dig_regmap)) {
		SND_LOG_ERR("dig_regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
err_devm_request_region:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_codec_mem *mem)
{
	SND_LOG_DEBUG("\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static int ac203c_set_params_from_of(struct i2c_client *i2c, struct ac203c_data *pdata)
{
	const struct device_node *np = i2c->dev.of_node;
	int ret;
	unsigned int temp_val;

	SND_LOG_DEBUG("\n");

	ret = of_property_read_u32(np, "adc1_vol", &pdata->adc1_vol);
	if (ret < 0)
		pdata->adc1_vol = 129;
	ret = of_property_read_u32(np, "adc2_vol", &pdata->adc2_vol);
	if (ret < 0)
		pdata->adc2_vol = 129;
	ret = of_property_read_u32(np, "adc3_vol", &pdata->adc3_vol);
	if (ret < 0)
		pdata->adc3_vol = 129;

	ret = of_property_read_u32(np, "dacl_vol", &pdata->dacl_vol);
	if (ret < 0)
		pdata->dacl_vol = 129;
	ret = of_property_read_u32(np, "dacr_vol", &pdata->dacr_vol);
	if (ret < 0)
		pdata->dacr_vol = 129;

	ret = of_property_read_u32(np, "mic1_gain", &pdata->mic1_gain);
	if (ret < 0)
		pdata->mic1_gain = 15;
	ret = of_property_read_u32(np, "mic2_gain", &pdata->mic2_gain);
	if (ret < 0)
		pdata->mic2_gain = 15;
	ret = of_property_read_u32(np, "mic3_gain", &pdata->mic3_gain);
	if (ret < 0)
		pdata->mic3_gain = 15;

	ret = of_property_read_u32(np, "dac_gain", &temp_val);
	if (ret < 0) {
		pdata->dac_gain = 0;
	} else {
		pdata->dac_gain = 7 - temp_val;
	}

	return 0;
}

static int snd_sunxi_pin_init(struct platform_device *pdev, struct sunxi_codec_pinctl *pin)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	if (of_property_read_bool(np, "pinctrl-used")) {
		pin->pinctrl_used = 1;
	} else {
		pin->pinctrl_used = 0;
		SND_LOG_DEBUG("unused pinctrl\n");
		return 0;
	}

	pin->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin->pinctrl)) {
		SND_LOG_ERR("pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin->pinstate = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin->pinstate)) {
		SND_LOG_ERR("pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin->pinstate_sleep = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin->pinstate_sleep)) {
		SND_LOG_ERR("pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin->pinctrl, pin->pinstate);
	if (ret < 0) {
		SND_LOG_ERR("codec1 set pinctrl default state fail\n");
		ret = -EBUSY;
		goto err_pinctrl_select_default;
	}

	return 0;

err_pinctrl_select_default:
err_loopup_pin_sleep:
err_loopup_pinstate:
	devm_pinctrl_put(pin->pinctrl);
	return ret;
}

static void snd_sunxi_pin_exit(struct platform_device *pdev, struct sunxi_codec_pinctl *pin)
{
	SND_LOG_DEBUG("\n");

	if (pin->pinctrl_used)
		devm_pinctrl_put(pin->pinctrl);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int ac203c_i2c_probe(struct i2c_client *i2c)
#else
static int ac203c_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
#endif
{
	struct ac203c_data *pdata = dev_get_platdata(&i2c->dev);
	struct platform_device *pdev = container_of(&i2c->dev, struct platform_device, dev);
	struct device_node *np = pdev->dev.of_node;
	struct ac203c_priv *ac203c;
	int ret;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	(void)id;
#endif

	SND_LOG_DEBUG("\n");

	ac203c = devm_kzalloc(&i2c->dev, sizeof(*ac203c), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ac203c)) {
		SND_LOG_ERR("Unable to allocate ac203c private data\n");
		return -ENOMEM;
	}

	ac203c->dev = &i2c->dev;

	ret = snd_sunxi_mem_init(i2c, pdev, &ac203c->mem);
	if (ret) {
		SND_LOG_ERR("mem init failed\n");
		ret = -ENOMEM;
		goto err_mem_init;
	}

	/* clk init */
	ret = snd_sunxi_clk_init(pdev, &ac203c->clk);
	if (ret) {
		SND_LOG_ERR("clk init failed\n");
		ret = -ENOMEM;
		goto err_clk_init;
	}

	/* pin init */
	ret = snd_sunxi_pin_init(pdev, &ac203c->pin);
	if (ret) {
		SND_LOG_ERR("pinctrl init failed\n");
		ret = -EINVAL;
		goto err_pin_init;
	}

	/* pa_pin init */
	ac203c->pa_cfg = snd_sunxi_pa_pin_init(pdev, &ac203c->pa_pin_max);

	/* clk_bus and clk_rst enable */
	ret = snd_sunxi_clk_bus_enable(&ac203c->clk);
	if (ret) {
		SND_LOG_ERR("clk_bus and clk_rst enable failed\n");
		ret = -EINVAL;
		goto err_clk_bus_enable;
	}

	if (pdata) {
		memcpy(&ac203c->pdata, pdata, sizeof(struct ac203c_data));
	} else if (i2c->dev.of_node) {
		ret = ac203c_set_params_from_of(i2c, &ac203c->pdata);
		if (ret) {
			SND_LOG_ERR("ac203c_set_params_from_of failed\n");
			ret = -ENOMEM;
			goto err_params_from_of;
		}
	} else {
		SND_LOG_ERR("Unable to allocate ac203c private data\n");
		ret = -ENOMEM;
		goto err_params_from_of;
	}

	ac203c->rglt = snd_sunxi_regulator_init(&i2c->dev);
	if (!ac203c->rglt) {
		SND_LOG_ERR("rglt init failed\n");
		ret = -EINVAL;
		goto err_regulator_init;
	}

	i2c_set_clientdata(i2c, ac203c);

	ret = devm_snd_soc_register_component(&i2c->dev,
					      &soc_component_dev_ac203c,
					      &ac203c_dai, 1);
	if (ret < 0) {
		SND_LOG_ERR("register ac203c codec failed: %d\n", ret);
		goto err_register_component;
	}

	return ret;

err_register_component:
	snd_sunxi_regulator_exit(ac203c->rglt);
err_regulator_init:
err_params_from_of:
err_clk_bus_enable:
	snd_sunxi_pin_exit(pdev, &ac203c->pin);
err_pin_init:
	snd_sunxi_clk_exit(&ac203c->clk);
err_clk_init:
	snd_sunxi_mem_exit(pdev, &ac203c->mem);
err_mem_init:
	devm_kfree(&i2c->dev, ac203c);
	of_node_put(np);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static void ac203c_i2c_remove(struct i2c_client *i2c)
#else
static int ac203c_i2c_remove(struct i2c_client *i2c)
#endif
{
	struct device *dev = &i2c->dev;
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct device_node *np = i2c->dev.of_node;
	struct ac203c_priv *ac203c = dev_get_drvdata(dev);

	snd_sunxi_mem_exit(pdev, &ac203c->mem);
	snd_sunxi_pin_exit(pdev, &ac203c->pin);
	snd_sunxi_clk_bus_disable(&ac203c->clk);
	snd_sunxi_clk_exit(&ac203c->clk);
	snd_sunxi_regulator_exit(ac203c->rglt);
	snd_sunxi_pa_pin_exit(ac203c->pa_cfg, ac203c->pa_pin_max);

	devm_kfree(dev, ac203c);
	of_node_put(np);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	return;
#else
	return 0;
#endif
}

static const struct of_device_id ac203c_of_match[] = {
	{ .compatible = "allwinner,sunxi-ac203c"},
	{ }
};
MODULE_DEVICE_TABLE(of, ac203c_of_match);

static struct i2c_driver ac203c_i2c_driver = {
	.driver = {
		.name = DRV_NAME,
		.of_match_table = ac203c_of_match,
	},
	.probe = ac203c_i2c_probe,
	.remove = ac203c_i2c_remove,
};

module_i2c_driver(ac203c_i2c_driver);

MODULE_DESCRIPTION("ASoC ac203c driver");
MODULE_AUTHOR("huhaoxin@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

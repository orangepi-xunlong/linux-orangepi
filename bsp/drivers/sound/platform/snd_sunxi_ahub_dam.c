// SPDX-License-Identifier: GPL-2.0-or-later
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

#define SUNXI_MODNAME		"sound-ahub-dam"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <sound/soc.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>

#include "snd_sunxi_pcm.h"
#include "snd_sunxi_common.h"
#include "snd_sunxi_ahub_dam.h"

#define DRV_NAME	"sunxi-snd-plat-ahub_dam"

/* ahub_reg */
static struct audio_reg_label ahub_reg_public[] = {
	REG_LABEL(SUNXI_AHUB_CTL),
	REG_LABEL(SUNXI_AHUB_VER),
	REG_LABEL(SUNXI_AHUB_RST),
	REG_LABEL(SUNXI_AHUB_GAT),
};
/* APBIF */
static struct audio_reg_label ahub_reg_apbif0[] = {
	REG_LABEL(SUNXI_AHUB_APBIF_TX_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_STA(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_STA(0)),
	/* SUNXI_AHUB_APBIF_TXFIFO(0), */
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CNT(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_STA(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_STA(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CONT(0)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO(0)), */
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CNT(0)),
};
static struct audio_reg_label ahub_reg_apbif1[] = {
	REG_LABEL(SUNXI_AHUB_APBIF_TX_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_STA(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_STA(1)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO(1)), */
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CNT(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_STA(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_STA(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CONT(1)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO(1)), */
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CNT(1)),
};
static struct audio_reg_label ahub_reg_apbif2[] = {
	REG_LABEL(SUNXI_AHUB_APBIF_TX_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_STA(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_STA(2)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO(2)), */
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CNT(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_STA(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_STA(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CONT(2)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO(2)), */
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CNT(2)),
};
/* I2S */
static struct audio_reg_label ahub_reg_i2s0[] = {
	REG_LABEL(SUNXI_AHUB_I2S_CTL(0)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT0(0)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT1(0)),
	REG_LABEL(SUNXI_AHUB_I2S_CLKD(0)),
	REG_LABEL(SUNXI_AHUB_I2S_RXCONT(0)),
	REG_LABEL(SUNXI_AHUB_I2S_CHCFG(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_CTL(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_STA(0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(0, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(0, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(0, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(0, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(0, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(0, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(0, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(0, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(0, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(0, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(0, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(0, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_SLOT(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP0(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP1(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP2(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP3(0)),
};
static struct audio_reg_label ahub_reg_i2s1[] = {
	REG_LABEL(SUNXI_AHUB_I2S_CTL(1)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT0(1)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT1(1)),
	REG_LABEL(SUNXI_AHUB_I2S_CLKD(1)),
	REG_LABEL(SUNXI_AHUB_I2S_RXCONT(1)),
	REG_LABEL(SUNXI_AHUB_I2S_CHCFG(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_CTL(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_STA(1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(1, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(1, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(1, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(1, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(1, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(1, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(1, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(1, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(1, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(1, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(1, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(1, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_SLOT(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP0(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP1(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP2(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP3(1)),
};
static struct audio_reg_label ahub_reg_i2s2[] = {
	REG_LABEL(SUNXI_AHUB_I2S_CTL(2)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT0(2)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT1(2)),
	REG_LABEL(SUNXI_AHUB_I2S_CLKD(2)),
	REG_LABEL(SUNXI_AHUB_I2S_RXCONT(2)),
	REG_LABEL(SUNXI_AHUB_I2S_CHCFG(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_CTL(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_STA(2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(2, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(2, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(2, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(2, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(2, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(2, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(2, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(2, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(2, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(2, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(2, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(2, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_SLOT(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP0(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP1(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP2(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP3(2)),
};
static struct audio_reg_label ahub_reg_i2s3[] = {
	REG_LABEL(SUNXI_AHUB_I2S_CTL(3)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT0(3)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT1(3)),
	REG_LABEL(SUNXI_AHUB_I2S_CLKD(3)),
	REG_LABEL(SUNXI_AHUB_I2S_RXCONT(3)),
	REG_LABEL(SUNXI_AHUB_I2S_CHCFG(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_CTL(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_STA(3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(3, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(3, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(3, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(3, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(3, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(3, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(3, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(3, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(3, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(3, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(3, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(3, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_SLOT(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP0(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP1(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP2(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP3(3)),
};
/* DAM */
static struct audio_reg_label ahub_reg_dam0[] = {
	REG_LABEL(SUNXI_AHUB_DAM_CTL(0)),
	REG_LABEL(SUNXI_AHUB_DAM_RX0_SRC(0)),
	REG_LABEL(SUNXI_AHUB_DAM_RX1_SRC(0)),
	REG_LABEL(SUNXI_AHUB_DAM_RX2_SRC(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL0(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL1(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL2(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL3(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL4(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL5(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL6(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL7(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL0(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL1(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL2(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL3(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL4(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL5(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL6(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL7(0)),
};
static struct audio_reg_label ahub_reg_dam1[] = {
	REG_LABEL(SUNXI_AHUB_DAM_CTL(1)),
	REG_LABEL(SUNXI_AHUB_DAM_RX0_SRC(1)),
	REG_LABEL(SUNXI_AHUB_DAM_RX1_SRC(1)),
	REG_LABEL(SUNXI_AHUB_DAM_RX2_SRC(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL0(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL1(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL2(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL3(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL4(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL5(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL6(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL7(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL0(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL1(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL2(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL3(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL4(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL5(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL6(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL7(1)),
};

static struct audio_reg_group sunxi_reg_groups[] = {
	REG_GROUP(ahub_reg_public),
	REG_GROUP(ahub_reg_apbif0),
	REG_GROUP(ahub_reg_apbif1),
	REG_GROUP(ahub_reg_apbif2),
	REG_GROUP(ahub_reg_i2s0),
	REG_GROUP(ahub_reg_i2s1),
	REG_GROUP(ahub_reg_i2s2),
	REG_GROUP(ahub_reg_i2s3),
	REG_GROUP(ahub_reg_dam0),
	REG_GROUP(ahub_reg_dam1),
};

static struct resource sunxi_res;
static struct regmap_config sunxi_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_AHUB_MAX_REG,
	.cache_type = REGCACHE_NONE,
};
struct sunxi_ahub_mem sunxi_mem = {
	.res = &sunxi_res,
};
static struct sunxi_ahub_clk sunxi_clk;

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
static struct sunxi_ahub_dump sunxi_dump;
#endif

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_ahub_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_ahub_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_ahub_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_ahub_clk *clk);

static struct snd_soc_dai_driver sunxi_ahub_dam_dai = {
};

static int sunxi_ahub_dam_probe(struct snd_soc_component *component)
{
	SND_LOG_DEBUG("\n");

	return 0;
}

static int sunxi_ahub_dam_suspend(struct snd_soc_component *component)
{
	struct sunxi_ahub_clk *clk = &sunxi_clk;
	struct regmap *regmap = sunxi_mem.regmap;
	unsigned int i;

	SND_LOG_DEBUG("\n");

	for (i = 0; i < ARRAY_SIZE(sunxi_reg_groups); ++i)
		snd_sunxi_save_reg(regmap, &sunxi_reg_groups[i]);

	snd_sunxi_clk_disable(clk);

	return 0;
}

static int sunxi_ahub_dam_resume(struct snd_soc_component *component)
{
	struct sunxi_ahub_clk *clk = &sunxi_clk;
	struct regmap *regmap = sunxi_mem.regmap;
	unsigned int i;
	int ret;

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_clk_enable(clk);
	if (ret) {
		SND_LOG_ERR("clk enable failed\n");
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(sunxi_reg_groups); ++i)
		snd_sunxi_echo_reg(regmap, &sunxi_reg_groups[i]);

	return 0;
}

static const unsigned int ahub_mux_reg[] = {
	SUNXI_AHUB_APBIF_RXFIFO_CONT(0),
	SUNXI_AHUB_APBIF_RXFIFO_CONT(1),
	SUNXI_AHUB_APBIF_RXFIFO_CONT(2),
	SUNXI_AHUB_I2S_RXCONT(0),
	SUNXI_AHUB_I2S_RXCONT(1),
	SUNXI_AHUB_I2S_RXCONT(2),
	SUNXI_AHUB_I2S_RXCONT(3),
	SUNXI_AHUB_DAM_RX0_SRC(0),
	SUNXI_AHUB_DAM_RX1_SRC(0),
	SUNXI_AHUB_DAM_RX2_SRC(0),
	SUNXI_AHUB_DAM_RX0_SRC(1),
	SUNXI_AHUB_DAM_RX1_SRC(1),
	SUNXI_AHUB_DAM_RX2_SRC(1),
};
static const unsigned int ahub_mux_values[] = {
	0,
	1 << I2S_RX_APBIF_TXDIF0,
	1 << I2S_RX_APBIF_TXDIF1,
	1 << I2S_RX_APBIF_TXDIF2,
	1 << I2S_RX_I2S0_TXDIF,
	1 << I2S_RX_I2S1_TXDIF,
	1 << I2S_RX_I2S2_TXDIF,
	1 << I2S_RX_I2S3_TXDIF,
	1 << I2S_RX_DAM0_TXDIF,
	1 << I2S_RX_DAM1_TXDIF,
};

static const char *ahub_mux_text[] = {
	"NONE",
	"APBIF_TXDIF0",
	"APBIF_TXDIF1",
	"APBIF_TXDIF2",
	"I2S0_TXDIF",
	"I2S1_TXDIF",
	"I2S2_TXDIF",
	"I2S3_TXDIF",
	"DAM0_TXDIF",
	"DAM1_TXDIF",
};
static SOC_ENUM_SINGLE_EXT_DECL(ahub_mux, ahub_mux_text);

static int sunxi_ahub_mux_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int i;
	unsigned int reg_val;
	unsigned int src_reg;
	struct regmap *regmap = sunxi_mem.regmap;

	if (kcontrol->id.numid > ARRAY_SIZE(ahub_mux_reg))
		return -EINVAL;

	src_reg = ahub_mux_reg[kcontrol->id.numid - 1];
	regmap_read(regmap, src_reg, &reg_val);
	reg_val &= 0xEE888000;

	for (i = 1; i < ARRAY_SIZE(ahub_mux_values); i++) {
		if (reg_val & ahub_mux_values[i]) {
			ucontrol->value.integer.value[0] = i;
			return 0;
		}
	}
	ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int sunxi_ahub_mux_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	unsigned int src_reg, src_regbit;
	struct regmap *regmap = sunxi_mem.regmap;

	if (kcontrol->id.numid > ARRAY_SIZE(ahub_mux_reg))
		return -EINVAL;
	if (ucontrol->value.integer.value[0] > ARRAY_SIZE(ahub_mux_values))
		return -EINVAL;

	src_reg = ahub_mux_reg[kcontrol->id.numid - 1];
	src_regbit = ahub_mux_values[ucontrol->value.integer.value[0]];
	regmap_update_bits(regmap, src_reg, 0xEE888000, src_regbit);

	return 0;
}

static const struct snd_kcontrol_new sunxi_ahub_dam_controls[] = {
	SOC_ENUM_EXT("APBIF0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("APBIF1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("APBIF2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S3 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM0C0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM0C1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM0C2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM1C0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM1C1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM1C2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
};

static struct snd_soc_component_driver sunxi_ahub_dam_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_ahub_dam_probe,
	.suspend	= sunxi_ahub_dam_suspend,
	.resume		= sunxi_ahub_dam_resume,
	.controls	= sunxi_ahub_dam_controls,
	.num_controls	= ARRAY_SIZE(sunxi_ahub_dam_controls),
};

struct sunxi_dapm_widget {
	unsigned int id;
	void *priv;				/* widget specific data */
	int (*event)(void *, bool);
};

static void sunxi_ahub_dam_mux_get(unsigned int mux_seq, unsigned int *mux_src)
{
	int i;
	unsigned int reg_val;
	struct regmap *regmap = sunxi_mem.regmap;

	regmap_read(regmap, ahub_mux_reg[mux_seq], &reg_val);
	reg_val &= 0xEE888000;
	for (i = 0; i < ARRAY_SIZE(ahub_mux_values); i++)
		if (reg_val & ahub_mux_values[i])
			break;

	*mux_src = i;
}

void sunxi_ahub_dam_ctrl(bool enable,
			 unsigned int apb_num, unsigned int tdm_num, unsigned int channels)
{
	static unsigned char dam_use;
	unsigned char dam0_mask = 0x07;
	unsigned char dam1_mask = 0x70;
	unsigned int mux_src;
	struct regmap *regmap = sunxi_mem.regmap;

	SND_LOG_DEBUG("\n");

	if (enable)
		goto enable;
	else
		goto disable;

enable:
	sunxi_ahub_dam_mux_get(7, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam0_mask & BIT(0));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX0EN, 1 << DAM_CTL_RX0EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX0_NUM,
				   (channels - 1) << DAM_CTL_RX0_NUM);
	}
	sunxi_ahub_dam_mux_get(8, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam0_mask & BIT(1));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX1EN, 1 << DAM_CTL_RX1EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX1_NUM,
				   (channels - 1) << DAM_CTL_RX1_NUM);
	}
	sunxi_ahub_dam_mux_get(9, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam0_mask & BIT(2));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX2EN, 1 << DAM_CTL_RX2EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX2_NUM,
				   (channels - 1) << DAM_CTL_RX2_NUM);
	}

	sunxi_ahub_dam_mux_get(10, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam1_mask & BIT(4 + 0));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX0EN, 1 << DAM_CTL_RX0EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX0_NUM,
				   (channels - 1) << DAM_CTL_RX0_NUM);
	}
	sunxi_ahub_dam_mux_get(11, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam1_mask & BIT(4 + 1));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX1EN, 1 << DAM_CTL_RX1EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX1_NUM,
				   (channels - 1) << DAM_CTL_RX1_NUM);
	}
	sunxi_ahub_dam_mux_get(12, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam1_mask & BIT(4 + 2));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX2EN, 1 << DAM_CTL_RX2EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX2_NUM,
				   (channels - 1) << DAM_CTL_RX2_NUM);
	}

	if (dam_use & dam0_mask) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST, 1 << DAM0_RST, 1 << DAM0_RST);
		regmap_update_bits(regmap, SUNXI_AHUB_GAT, 1 << DAM0_GAT, 1 << DAM0_GAT);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_TX_NUM,
				   (channels - 1) << DAM_CTL_TX_NUM);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_TXEN, 1 << DAM_CTL_TXEN);
	}
	if (dam_use & dam1_mask) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST, 1 << DAM1_RST, 1 << DAM1_RST);
		regmap_update_bits(regmap, SUNXI_AHUB_GAT, 1 << DAM1_GAT, 1 << DAM1_GAT);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_TX_NUM,
				   (channels - 1) << DAM_CTL_TX_NUM);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_TXEN, 1 << DAM_CTL_TXEN);
	}

	return;

disable:
	sunxi_ahub_dam_mux_get(7, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam0_mask & (~BIT(0)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX0EN, 0 << DAM_CTL_RX0EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX0_NUM, 0x0 << DAM_CTL_RX0_NUM);
	}
	sunxi_ahub_dam_mux_get(8, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam0_mask & (~BIT(1)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX1EN, 0 << DAM_CTL_RX1EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX1_NUM, 0x0 << DAM_CTL_RX1_NUM);
	}
	sunxi_ahub_dam_mux_get(9, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam0_mask & (~BIT(2)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX2EN, 0 << DAM_CTL_RX2EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX2_NUM, 0x0 << DAM_CTL_RX2_NUM);
	}

	sunxi_ahub_dam_mux_get(10, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam1_mask & (~BIT(4 + 0)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX0EN, 0 << DAM_CTL_RX0EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX0_NUM, 0x0 << DAM_CTL_RX0_NUM);
	}
	sunxi_ahub_dam_mux_get(11, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam1_mask & (~BIT(4 + 1)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX1EN, 0 << DAM_CTL_RX1EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX1_NUM, 0x0 << DAM_CTL_RX1_NUM);
	}
	sunxi_ahub_dam_mux_get(12, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam1_mask & (~BIT(4 + 2)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX2EN, 0 << DAM_CTL_RX2EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX2_NUM, 0x0 << DAM_CTL_RX2_NUM);
	}

	if (!(dam_use & dam0_mask)) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST, 1 << DAM0_RST, 0 << DAM0_RST);
		regmap_update_bits(regmap, SUNXI_AHUB_GAT, 1 << DAM0_GAT, 0 << DAM0_GAT);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_TX_NUM, 0x0 << DAM_CTL_TX_NUM);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_TXEN, 0 << DAM_CTL_TXEN);
	}
	if (!(dam_use & dam1_mask)) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST, 1 << DAM1_RST, 0 << DAM1_RST);
		regmap_update_bits(regmap, SUNXI_AHUB_GAT, 1 << DAM1_GAT, 0 << DAM1_GAT);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_TX_NUM, 0x0 << DAM_CTL_TX_NUM);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_TXEN, 0 << DAM_CTL_TXEN);
	}
	return;
}
EXPORT_SYMBOL_GPL(sunxi_ahub_dam_ctrl);

/*******************************************************************************
 * *** kernel source ***
 * @1 regmap
 * @2 clk
 * @3 reg debug
 ******************************************************************************/
int snd_sunxi_ahub_mem_get(struct sunxi_ahub_mem *mem)
{
	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(sunxi_mem.regmap)) {
		SND_LOG_ERR("regmap is invalid\n");
		return -EINVAL;
	}
	if (IS_ERR_OR_NULL(sunxi_mem.res)) {
		SND_LOG_ERR("res is invalid\n");
		return -EINVAL;
	}

	mem->regmap = sunxi_mem.regmap;
	mem->res = sunxi_mem.res;

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_ahub_mem_get);

int snd_sunxi_ahub_clk_get(struct sunxi_ahub_clk *clk)
{
	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(sunxi_clk.clk_pll)) {
		SND_LOG_ERR("clk_pll is invalid\n");
		return -EINVAL;
	}
	if (IS_ERR_OR_NULL(sunxi_clk.clk_pllx4)) {
		SND_LOG_ERR("clk_pllx4 is invalid\n");
		return -EINVAL;
	}
	if (IS_ERR_OR_NULL(sunxi_clk.clk_module)) {
		SND_LOG_ERR("clk_module is invalid\n");
		return -EINVAL;
	}

	clk->clk_pll = sunxi_clk.clk_pll;
	clk->clk_pllx4 = sunxi_clk.clk_pllx4;
	clk->clk_module = sunxi_clk.clk_module;

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_ahub_clk_get);

static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_ahub_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	ret = of_address_to_resource(np, 0, mem->res);
	if (ret) {
		SND_LOG_ERR("parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res->start,
						 resource_size(mem->res),
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
};

static void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_ahub_mem *mem)
{
	SND_LOG_DEBUG("\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_ahub_clk *clk)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	/* get rst clk */
	clk->clk_rst = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(clk->clk_rst)) {
		SND_LOG_ERR("clk rst get failed\n");
		ret = -EBUSY;
		goto err_get_rst_clk;
	}

	/* get bus clk */
	clk->clk_bus = of_clk_get_by_name(np, "clk_bus_audio_hub");
	if (IS_ERR_OR_NULL(clk->clk_bus)) {
		SND_LOG_ERR("clk bus get failed\n");
		ret = -EBUSY;
		goto err_get_bus_clk;
	}

	/* get parent clk */
	clk->clk_pll = of_clk_get_by_name(np, "clk_pll_audio");
	if (IS_ERR_OR_NULL(clk->clk_pll)) {
		SND_LOG_ERR("clk pll get failed\n");
		ret = -EBUSY;
		goto err_get_pll_clk;
	}
	clk->clk_pllx4 = of_clk_get_by_name(np, "clk_pll_audio_4x");
	if (IS_ERR_OR_NULL(clk->clk_pllx4)) {
		SND_LOG_ERR("clk pllx4 get failed\n");
		ret = -EBUSY;
		goto err_get_pllx4_clk;
	}

	/* get module clk */
	clk->clk_module = of_clk_get_by_name(np, "clk_audio_hub");
	if (IS_ERR_OR_NULL(clk->clk_module)) {
		SND_LOG_ERR("clk module get failed\n");
		ret = -EBUSY;
		goto err_get_module_clk;
	}

	/* set ahub clk parent */
	if (clk_set_parent(clk->clk_module, clk->clk_pllx4)) {
		SND_LOG_ERR("set parent of clk_module to pllx4 failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	/* enable clk of ahub */
	ret = snd_sunxi_clk_enable(clk);
	if (ret) {
		SND_LOG_ERR("clk enable failed\n");
		ret = -EINVAL;
		goto err_clk_enable;
	}

	return 0;

err_clk_enable:
err_set_parent:
	clk_put(clk->clk_module);
err_get_module_clk:
	clk_put(clk->clk_pllx4);
err_get_pllx4_clk:
	clk_put(clk->clk_pll);
err_get_pll_clk:
	clk_put(clk->clk_bus);
err_get_bus_clk:
err_get_rst_clk:
	return ret;
}

static void snd_sunxi_clk_exit(struct sunxi_ahub_clk *clk)
{
	SND_LOG_DEBUG("\n");

	snd_sunxi_clk_disable(clk);
	clk_put(clk->clk_module);
	clk_put(clk->clk_pll);
	clk_put(clk->clk_pllx4);
	clk_put(clk->clk_bus);
}

static int snd_sunxi_clk_enable(struct sunxi_ahub_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG("\n");

	if (reset_control_deassert(clk->clk_rst)) {
		SND_LOG_ERR("deassert reset clk failed\n");
		ret = -EBUSY;
		goto err_deassert_rst;
	}

	if (clk_prepare_enable(clk->clk_bus)) {
		SND_LOG_ERR("ahub clk bus enable failed\n");
		ret = -EBUSY;
		goto err_enable_clk_bus;
	}

	if (clk_prepare_enable(clk->clk_pll)) {
		SND_LOG_ERR("clk_pll enable failed\n");
		ret = -EBUSY;
		goto err_enable_pll_clk;
	}
	if (clk_prepare_enable(clk->clk_pllx4)) {
		SND_LOG_ERR("clk_pllx4 enable failed\n");
		ret = -EBUSY;
		goto err_enable_pllx4_clk;
	}
	if (clk_prepare_enable(clk->clk_module)) {
		SND_LOG_ERR("clk_module enable failed\n");
		ret = -EBUSY;
		goto err_enable_module_clk;
	}

	return 0;

err_enable_module_clk:
	clk_disable_unprepare(clk->clk_pllx4);
err_enable_pllx4_clk:
	clk_disable_unprepare(clk->clk_pll);
err_enable_pll_clk:
	clk_disable_unprepare(clk->clk_bus);
err_enable_clk_bus:
	reset_control_assert(clk->clk_rst);
err_deassert_rst:
	return ret;
}

static void snd_sunxi_clk_disable(struct sunxi_ahub_clk *clk)
{
	SND_LOG_DEBUG("\n");

	clk_disable_unprepare(clk->clk_module);
	clk_disable_unprepare(clk->clk_pllx4);
	clk_disable_unprepare(clk->clk_pll);
	clk_disable_unprepare(clk->clk_bus);
	reset_control_assert(clk->clk_rst);
}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
/* sysfs debug */
static void snd_sunxi_dump_version(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_ahub_dump *ahub_dump = (struct sunxi_ahub_dump *)priv;

	if (!ahub_dump) {
		SND_LOG_ERR("priv to ahub_dump failed\n");
		return;
	}
	if (ahub_dump->pdev)
		if (ahub_dump->pdev->dev.driver)
			if (ahub_dump->pdev->dev.driver->owner)
				goto module_version;
	return;

module_version:
	ahub_dump->module_version = ahub_dump->pdev->dev.driver->owner->version;
	count_tmp += sprintf(buf + count_tmp, "%s\n", ahub_dump->module_version);

	*count = count_tmp;
}

static void snd_sunxi_dump_help(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;

	count_tmp += sprintf(buf + count_tmp, "1. reg read : echo {num} > dump && cat dump\n");
	count_tmp += sprintf(buf + count_tmp, "num:\n");
	count_tmp += sprintf(buf + count_tmp, "0(all)    1(public)\n");
	count_tmp += sprintf(buf + count_tmp, "2(APBIF0) 3(APBIF1) 4(APBIF2)\n");
	count_tmp += sprintf(buf + count_tmp, "5(I2S0)   6(I2S1)   7(I2S2)   8(I2S3)\n");
	count_tmp += sprintf(buf + count_tmp, "9(DAM0)   10(DAM1)\n");
	count_tmp += sprintf(buf + count_tmp, "2. reg write: echo {reg} {value} > dump\n");
	count_tmp += sprintf(buf + count_tmp, "eg. echo 0x00 0xaa > dump\n");

	*count = count_tmp;
}

static int snd_sunxi_dump_show(void *priv, char *buf, size_t *count)
{
	size_t count_tmp = 0;
	struct sunxi_ahub_dump *ahub_dump = (struct sunxi_ahub_dump *)priv;
	unsigned int i = 0, j = 0;
	int reg_domain;
	unsigned int output_reg_val;
	struct regmap *regmap;

	if (!ahub_dump) {
		SND_LOG_ERR("priv to ahub_dump failed\n");
		return -1;
	}
	if (ahub_dump->show_reg_num < 0 || ahub_dump->show_reg_num > 10) {
		return 0;
	} else {
		reg_domain = ahub_dump->show_reg_num;
		ahub_dump->show_reg_num = -1;
	}

	regmap = ahub_dump->regmap;

	/* show specify domain reg */
	if (reg_domain != 0) {
		for (i = 0; i < sunxi_reg_groups[reg_domain].size; ++i) {
			regmap_read(regmap, sunxi_reg_groups[reg_domain].label[i].address,
				    &output_reg_val);
			count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
					     sunxi_reg_groups[reg_domain].label[i].address,
					     output_reg_val);
		}
		goto end;
	}

	/* show all domain reg */
	for (i = 0; i < ARRAY_SIZE(sunxi_reg_groups); ++i) {
		for (j = 0; j < sunxi_reg_groups[i].size; ++j) {
			regmap_read(regmap, sunxi_reg_groups[i].label[j].address, &output_reg_val);
			count_tmp += sprintf(buf + count_tmp, "[0x%03x]: 0x%8x\n",
				     sunxi_reg_groups[i].label[j].address, output_reg_val);
		}
		count_tmp += sprintf(buf + count_tmp, "\n");
	}

end:
	*count = count_tmp;
	return 0;
}

static int snd_sunxi_dump_store(void *priv, const char *buf, size_t count)
{
	struct sunxi_ahub_dump *ahub_dump = (struct sunxi_ahub_dump *)priv;
	int ret, scanf_cnt;
	unsigned int input_reg_offset, input_reg_val, output_reg_val;
	struct regmap *regmap;

	if (count <= 1)	/* null or only "\n" */
		return 0;
	if (!ahub_dump) {
		SND_LOG_ERR("priv to ahub_dump failed\n");
		return -1;
	}
	regmap = ahub_dump->regmap;

	/* 0xnn 0xnn, at least 4 char. */
	if (count < 4) {
		ret = kstrtoint(buf, 10, &ahub_dump->show_reg_num);
		if (ret) {
			pr_err("wrong format: %s\n", buf);
			return -1;
		}
		if (ahub_dump->show_reg_num < 0 || ahub_dump->show_reg_num > 10)
			ahub_dump->show_reg_num = -1; /* unshow */
		return 0;
	}

	scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	if (scanf_cnt != 2) {
		pr_err("wrong format: %s\n", buf);
		return -1;
	}
	if (input_reg_offset > SUNXI_AHUB_MAX_REG) {
		pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_AHUB_MAX_REG);
		return -1;
	}
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
	regmap_write(regmap, input_reg_offset, input_reg_val);
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);

	return 0;
}
#endif

static int sunxi_ahub_dam_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_ahub_mem *mem = &sunxi_mem;
	struct sunxi_ahub_clk *clk = &sunxi_clk;
#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump = &sunxi_dump.dump;
#endif

	SND_LOG_DEBUG("\n");

	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR("remap init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_mem_init;
	}

	ret = snd_sunxi_clk_init(pdev, clk);
	if (ret) {
		SND_LOG_ERR("clk init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_clk_init;
	}

	ret = snd_soc_register_component(&pdev->dev, &sunxi_ahub_dam_dev, &sunxi_ahub_dam_dai, 1);
	if (ret) {
		SND_LOG_ERR("component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	sunxi_dump.pdev = pdev;
	sunxi_dump.show_reg_num = -1;	/* default: unshow */
	sunxi_dump.regmap = mem->regmap;
	snprintf(sunxi_dump.module_name, 32, "%s", "AHUB");
	dump->name = sunxi_dump.module_name;
	dump->priv = &sunxi_dump;
	dump->dump_version = snd_sunxi_dump_version;
	dump->dump_help = snd_sunxi_dump_help;
	dump->dump_show = snd_sunxi_dump_show;
	dump->dump_store = snd_sunxi_dump_store;
	ret = snd_sunxi_dump_register(dump);
	if (ret)
		SND_LOG_WARN("snd_sunxi_dump_register failed\n");
#endif

	SND_LOG_DEBUG("register ahub_dam platform success\n");

	return 0;

err_snd_soc_register_component:
	snd_sunxi_clk_exit(clk);
err_snd_sunxi_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_snd_sunxi_mem_init:
	of_node_put(np);
	return ret;
}

static int sunxi_ahub_dam_dev_remove(struct platform_device *pdev)
{
	struct sunxi_ahub_mem *mem = &sunxi_mem;
	struct sunxi_ahub_clk *clk = &sunxi_clk;

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	struct snd_sunxi_dump *dump = &sunxi_dump.dump;
#endif

	SND_LOG_DEBUG("\n");

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	snd_sunxi_dump_unregister(dump);
#endif
	snd_soc_unregister_component(&pdev->dev);

	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_clk_exit(clk);

	SND_LOG_DEBUG("unregister ahub_dam platform success\n");

	return 0;
}

static const struct of_device_id sunxi_ahub_dam_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_ahub_dam_of_match);

static struct platform_driver sunxi_ahub_dam_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_ahub_dam_of_match,
	},
	.probe	= sunxi_ahub_dam_dev_probe,
	.remove	= sunxi_ahub_dam_dev_remove,
};

int __init sunxi_ahub_dam_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_ahub_dam_driver);
	if (ret != 0) {
		SND_LOG_ERR("platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_ahub_dam_dev_exit(void)
{
	platform_driver_unregister(&sunxi_ahub_dam_driver);
}

late_initcall(sunxi_ahub_dam_dev_init);
module_exit(sunxi_ahub_dam_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.2");
MODULE_DESCRIPTION("sunxi soundcard platform of ahub_dam");

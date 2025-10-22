/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2021, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_MACH_UTILS_H
#define __SND_SUNXI_MACH_UTILS_H

#include "snd_sunxi_jack.h"

#define simple_priv_to_card(priv)	(&(priv)->snd_card)
#define simple_priv_to_props(priv, i)	((priv)->dai_props + (i))
#define simple_priv_to_dev(priv)	(simple_priv_to_card(priv)->dev)
#define simple_priv_to_link(priv, i)	(simple_priv_to_card(priv)->dai_link + (i))

#define asoc_simple_parse_cpu(node, dai_link,				\
			      list_name, cells_name, is_single_link)	\
	asoc_simple_parse_dai(node, dai_link->cpus,			\
			      list_name, cells_name, is_single_link)

#define asoc_simple_parse_codec(node, dai_link,				\
				list_name, cells_name)			\
	asoc_simple_parse_dai(node, dai_link->codecs,			\
			      list_name, cells_name, NULL)

#define asoc_simple_parse_platform(node, dai_link,			\
				   list_name, cells_name)		\
	asoc_simple_parse_dai(node, dai_link->platforms,		\
			      list_name, cells_name, NULL)

struct asoc_simple_dai {
	const char *name;
	unsigned int sysclk;
	int clk_direction;
	int slots;
	int slot_width;
	unsigned int tx_slot_mask;
	unsigned int rx_slot_mask;
	struct clk *clk;
};

struct asoc_simple_data {
	u32 convert_rate;
	u32 convert_channels;
};

struct asoc_simple_ucfmt {
	unsigned int fmt;

	u32 data_late;
	bool tx_lsb_first;
	bool rx_lsb_first;
};

struct asoc_simple_jack {
	struct snd_soc_jack jack;
	struct snd_soc_jack_pin pin;
	struct snd_soc_jack_gpio gpio;
};

/* note:
 * soundcard-mach,cpu
 *
 * "mclk-fs"
 * if not defined it or equal to 0, disable mclk.
 *
 * "mclk-fp" (if defined "mclk-fs")
 * 1. if not defined "mclk-fp", mclk_freq = mclk-fs * sample_rate;
 * 2. if defined "mclk-fp", it should carry 2 value, like: mclk-fp = <val1 val2>;
 *    it means: mclk_freq(44.1k fp) = mclk-fs * val1;
 *              mclk_freq(48k fp)   = mclk-fs * val2.
 */

/* note:
 * soundcard-mach,codec
 *
 * "pllin-mode"
 * 1. if not defined or 0, pllin = 24.576M * pllin-fs or pllin = 22.5792M * pllin-fs;
 * 2. if defined 1, pllin = pll-fp * pllin-fs;
 * 3. if defined 2, pllin = sample_rate * slots * slot_width = bclk.
 *
 * "pllout-mode"
 * 1. if not defined or 0, pllout = 24.576M * pll-fs or 22.5792M * pll-fs;
 * 2. if defined 1, pllout = pll-fp * pllout-fs;
 * 3. if defined 2, pllout = sample_rate * slots * slot_width = bclk.
 *
 * "pll-fp" (if defined "pllin-mode" or "pllout-mode").
 * "pllin-fs" Only works when "pllin-mode" is equal to 1.
 * "pllout-fs" Only works when "pllout-mode" is equal to 1.
 *
 * eg.
 * "fp * fs mode"
 * pllin-mode = 0, pllin-fs = 1, pllout-mode = 0, pllout-fs = 1;
 * "codec-fp * fs mode"
 * pllin-mode = 1, pllin-fs = 1, pllout-mode = 1, pllout-fs = 1;
 * "bclk mode"
 * pllin-mode = 2, pllin-fs = 1, pllout-mode = 2, pllout-fs = 1.
 */

struct asoc_simple_priv {
	struct snd_soc_card snd_card;
	struct simple_dai_props {
		struct asoc_simple_dai *cpu_dai;
		struct asoc_simple_dai *codec_dai;
		struct snd_soc_dai_link_component cpus;   /* single cpu */
		struct snd_soc_dai_link_component *codecs; /* multi codec */
		struct snd_soc_dai_link_component platforms;
		struct asoc_simple_data adata;
		struct snd_sunxi_dai_ucfmt dai_ucfmt;
		struct snd_soc_codec_conf *codec_conf;
		unsigned int mclk_fp[2];
		unsigned int mclk_fs;
		unsigned int cpu_pll_fs;
		unsigned int codec_pll_fp[2];
		unsigned int codec_pllin_mode;
		unsigned int codec_pllin_fs;
		unsigned int codec_pllout_mode;
		unsigned int codec_pllout_fs;
	} *dai_props;
	struct asoc_simple_jack hp_jack;
	struct asoc_simple_jack mic_jack;
	struct snd_soc_dai_link *dai_link;
	struct asoc_simple_dai *dais;
	struct snd_soc_codec_conf *codec_conf;
	struct gpio_desc *pa_gpio;

	/* misc */
	enum JACK_DET_METHOD jack_support;
	long wait_time;

	/* debug */
	char module_name[32];
	const char *module_version;
	struct snd_sunxi_dump dump;
	bool show_daifmt;
};

int asoc_simple_clean_reference(struct snd_soc_card *card);
int asoc_simple_init_priv(struct asoc_simple_priv *priv);

int asoc_simple_is_i2sdai(struct asoc_simple_dai *dais);

int asoc_simple_parse_widgets(struct snd_soc_card *card, char *prefix);
int asoc_simple_parse_routing(struct snd_soc_card *card, char *prefix);
int asoc_simple_parse_pin_switches(struct snd_soc_card *card, char *prefix);
int asoc_simple_parse_misc(struct device_node *node, char *prefix, struct asoc_simple_priv *priv);

int asoc_simple_parse_daistream(struct device_node *node,
				char *prefix,
				struct snd_soc_dai_link *dai_link);
int asoc_simple_parse_daifmt(struct device_node *node,
			     struct device_node *codec,
			     char *prefix,
			     unsigned int *retfmt);
int asoc_simple_parse_ucfmt(struct device_node *node, char *prefix,
				struct asoc_simple_priv *priv);
int asoc_simple_parse_tdm_slot(struct device_node *node,
			       char *prefix,
			       struct asoc_simple_dai *dais);
int asoc_simple_parse_dlc_name(struct device *dev, struct device_node *node,
			       char *prefix, struct snd_soc_dai_link *dai_link);
int asoc_simple_parse_tdm_clk(struct device_node *cpu,
			      struct device_node *codec,
			      char *prefix,
			      struct simple_dai_props *dai_props);

int asoc_simple_parse_card_name(struct snd_soc_card *card, char *prefix);
int asoc_simple_parse_dai(struct device_node *node,
			  struct snd_soc_dai_link_component *dlc,
			  const char *list_name,
			  const char *cells_name,
			  int *is_single_link);

int asoc_simple_set_dailink_name(struct device *dev,
				 struct snd_soc_dai_link *dai_link,
				 const char *fmt, ...);
void asoc_simple_canonicalize_platform(struct snd_soc_dai_link *dai_link);
void asoc_simple_canonicalize_cpu(struct snd_soc_dai_link *dai_link, int is_single_links);

#endif /* __SND_SUNXI_MACH_UTILS_H */

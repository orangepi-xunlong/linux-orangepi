/* sound\soc\sunxi\snd_sunxi_mach_utils.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_MACH_UTILS_H
#define __SND_SUNXI_MACH_UTILS_H

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

struct asoc_simple_jack {
	struct snd_soc_jack jack;
	struct snd_soc_jack_pin pin;
	struct snd_soc_jack_gpio gpio;
};

struct asoc_simple_priv {
	struct snd_soc_card snd_card;
	struct simple_dai_props {
		struct asoc_simple_dai *cpu_dai;
		struct asoc_simple_dai *codec_dai;
		struct snd_soc_dai_link_component cpus;   /* single cpu */
		struct snd_soc_dai_link_component codecs; /* single codec */
		struct snd_soc_dai_link_component platforms;
		struct asoc_simple_data adata;
		struct snd_soc_codec_conf *codec_conf;
		bool mclk_fp;
		unsigned int mclk_fs;
		unsigned int cpu_pll_fs;
		unsigned int codec_pll_fs;
	} *dai_props;
	struct asoc_simple_jack hp_jack;
	struct asoc_simple_jack mic_jack;
	struct snd_soc_dai_link *dai_link;
	struct asoc_simple_dai *dais;
	struct snd_soc_codec_conf *codec_conf;
	struct gpio_desc *pa_gpio;
};

int asoc_simple_clean_reference(struct snd_soc_card *card);
int asoc_simple_init_priv(struct asoc_simple_priv *priv);

int asoc_simple_parse_widgets(struct snd_soc_card *card, char *prefix);
int asoc_simple_parse_routing(struct snd_soc_card *card, char *prefix);
int asoc_simple_parse_pin_switches(struct snd_soc_card *card, char *prefix);

int asoc_simple_parse_daistream(struct device_node *node,
				char *prefix,
				struct snd_soc_dai_link *dai_link);
int asoc_simple_parse_daifmt(struct device_node *node,
			     struct device_node *codec,
			     char *prefix,
			     unsigned int *retfmt);
int asoc_simple_parse_tdm_slot(struct device_node *node,
			       char *prefix,
			       struct asoc_simple_dai *dais);
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
void asoc_simple_canonicalize_cpu(struct snd_soc_dai_link *dai_link,
				  int is_single_links);

#endif /* __SND_SUNXI_MACH_UTILS_H */

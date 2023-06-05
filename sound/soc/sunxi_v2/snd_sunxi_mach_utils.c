/*
 * sound\soc\sunxi\snd_sunxi_mach_utils.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * based on ${LINUX}/sound/soc/generic/simple-card.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <sound/soc.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_mach_utils.h"

#define HLOG		"mach_utils"

int asoc_simple_clean_reference(struct snd_soc_card *card)
{
	struct snd_soc_dai_link *dai_link;
	int i;

	for_each_card_prelinks(card, i, dai_link) {
		of_node_put(dai_link->cpus->of_node);
		of_node_put(dai_link->codecs->of_node);
	}
	return 0;
}

int asoc_simple_init_priv(struct asoc_simple_priv *priv)
{
	struct snd_soc_card *card = simple_priv_to_card(priv);
	struct device *dev = simple_priv_to_dev(priv);
	struct snd_soc_dai_link *dai_link;
	struct simple_dai_props *dai_props;
	struct asoc_simple_dai *dais;
	struct snd_soc_codec_conf *cconf = NULL;

	dai_props = devm_kcalloc(dev, 1, sizeof(*dai_props), GFP_KERNEL);
	dai_link  = devm_kcalloc(dev, 1, sizeof(*dai_link),  GFP_KERNEL);
	dais      = devm_kcalloc(dev, 1, sizeof(*dais),      GFP_KERNEL);
	if (!dai_props || !dai_link || !dais)
		return -ENOMEM;

	/*
	if (li->conf) {
		cconf = devm_kcalloc(dev, li->conf, sizeof(*cconf), GFP_KERNEL);
		if (!cconf)
			return -ENOMEM;
	}
	*/

	/*
	 * Use snd_soc_dai_link_component instead of legacy style
	 * It is codec only. but cpu/platform will be supported in the future.
	 * see
	 *	soc-core.c :: snd_soc_init_multicodec()
	 *
	 * "platform" might be removed
	 * see
	 *	simple-card-utils.c :: asoc_simple_canonicalize_platform()
	 */
	dai_link->cpus		= &dai_props->cpus;
	dai_link->num_cpus	= 1;
	dai_link->codecs	= &dai_props->codecs;
	dai_link->num_codecs	= 1;
	dai_link->platforms	= &dai_props->platforms;
	dai_link->num_platforms	= 1;

	priv->dai_props		= dai_props;
	priv->dai_link		= dai_link;
	priv->dais		= dais;
	priv->codec_conf	= cconf;

	card->dai_link		= priv->dai_link;
	card->num_links		= 1;
	card->codec_conf	= cconf;
	card->num_configs	= 0;

	return 0;
}

int asoc_simple_parse_widgets(struct snd_soc_card *card, char *prefix)
{
	struct device_node *node = card->dev->of_node;
	char prop[128];

	if (!prefix)
		prefix = "";

	snprintf(prop, sizeof(prop), "%s%s", prefix, "widgets");

	if (of_property_read_bool(node, prop))
		return snd_soc_of_parse_audio_simple_widgets(card, prop);

	/* no widgets is not error */
	return 0;
}

int asoc_simple_parse_routing(struct snd_soc_card *card, char *prefix)
{
	struct device_node *node = card->dev->of_node;
	char prop[128];

	if (!prefix)
		prefix = "";

	snprintf(prop, sizeof(prop), "%s%s", prefix, "routing");

	if (!of_property_read_bool(node, prop))
		return 0;

	return snd_soc_of_parse_audio_routing(card, prop);
}

int asoc_simple_parse_pin_switches(struct snd_soc_card *card, char *prefix)
{
	const unsigned int nb_controls_max = 16;
	const char **strings, *control_name;
	struct snd_kcontrol_new *controls;
	struct device *dev = card->dev;
	unsigned int i, nb_controls;
	char prop[128];
	int ret;

	if (!prefix)
		prefix = "";

	snprintf(prop, sizeof(prop), "%s%s", prefix, "pin-switches");

	if (!of_property_read_bool(dev->of_node, prop))
		return 0;

	strings = devm_kcalloc(dev, nb_controls_max,
			       sizeof(*strings), GFP_KERNEL);
	if (!strings)
		return -ENOMEM;

	ret = of_property_read_string_array(dev->of_node, prop,
					    strings, nb_controls_max);
	if (ret < 0)
		return ret;

	nb_controls = (unsigned int)ret;

	controls = devm_kcalloc(dev, nb_controls,
				sizeof(*controls), GFP_KERNEL);
	if (!controls)
		return -ENOMEM;

	for (i = 0; i < nb_controls; i++) {
		control_name = devm_kasprintf(dev, GFP_KERNEL,
					      "%s Switch", strings[i]);
		if (!control_name)
			return -ENOMEM;

		controls[i].iface = SNDRV_CTL_ELEM_IFACE_MIXER;
		controls[i].name = control_name;
		controls[i].info = snd_soc_dapm_info_pin_switch;
		controls[i].get = snd_soc_dapm_get_pin_switch;
		controls[i].put = snd_soc_dapm_put_pin_switch;
		controls[i].private_value = (unsigned long)strings[i];
	}

	card->controls = controls;
	card->num_controls = nb_controls;

	return 0;
}

int asoc_simple_parse_daifmt(struct device_node *node,
			     struct device_node *codec,
			     char *prefix,
			     unsigned int *retfmt)
{
	struct device_node *bitclkmaster = NULL;
	struct device_node *framemaster = NULL;
	unsigned int daifmt;

	daifmt = snd_soc_daifmt_parse_format(node, prefix);

	snd_soc_daifmt_parse_clock_provider_as_phandle(node, prefix, &bitclkmaster, &framemaster);
	if (!bitclkmaster && !framemaster) {
		/*
		 * No dai-link level and master setting was not found from
		 * sound node level, revert back to legacy DT parsing and
		 * take the settings from codec node.
		 */
		SND_LOG_DEBUG(HLOG, "Revert to legacy daifmt parsing\n");

		daifmt |= snd_soc_daifmt_parse_clock_provider_as_flag(codec, NULL);
	} else {
		daifmt |= snd_soc_daifmt_clock_provider_from_bitmap(
	                ((codec == bitclkmaster) << 4) | (codec == framemaster));
	}

	of_node_put(bitclkmaster);
	of_node_put(framemaster);

	*retfmt = daifmt;

	return 0;
}

int asoc_simple_parse_daistream(struct device_node *node, char *prefix,
				struct snd_soc_dai_link *dai_link)
{
	char prop[128];

	if (!prefix)
		prefix = "";

	/* check "[prefix]playback-only" */
	snprintf(prop, sizeof(prop), "%splayback-only", prefix);
	if (of_property_read_bool(node, prop))
		dai_link->playback_only = 1;

	/* check "[prefix]capture-only" */
	snprintf(prop, sizeof(prop), "%scapture-only", prefix);
	if (of_property_read_bool(node, prop))
		dai_link->capture_only = 1;

	return 0;
}

int asoc_simple_parse_tdm_slot(struct device_node *node, char *prefix,
			       struct asoc_simple_dai *dais)
{
	int ret;
	char prop[128];
	unsigned int val;

	if (!prefix)
		prefix = "";

	snprintf(prop, sizeof(prop), "%sslot-num", prefix);
	ret = of_property_read_u32(node, prop, &val);
	if (!ret)
		dais->slots = val;

	snprintf(prop, sizeof(prop), "%sslot-width", prefix);
	ret = of_property_read_u32(node, prop, &val);
	if (!ret)
		dais->slot_width = val;

	return 0;
}

int asoc_simple_parse_tdm_clk(struct device_node *cpu,
			      struct device_node *codec,
			      char *prefix,
			      struct simple_dai_props *dai_props)
{
	int ret;
	char prop[128];
	unsigned int val;

	if (!prefix)
		prefix = "";

	snprintf(prop, sizeof(prop), "%spll-fs", prefix);
	ret = of_property_read_u32(cpu, prop, &val);
	if (ret)
		dai_props->cpu_pll_fs = 1;	/* default sysclk 24.576 or 22.5792MHz * 1 */
	else
		dai_props->cpu_pll_fs = val;

	ret = of_property_read_u32(codec, prop, &val);
	if (ret)
		dai_props->codec_pll_fs = 1;	/* default sysclk 24.576 or 22.5792MHz * 1 */
	else
		dai_props->codec_pll_fs = val;

	snprintf(prop, sizeof(prop), "%smclk-fp", prefix);
	dai_props->mclk_fp = of_property_read_bool(cpu, prop);

	snprintf(prop, sizeof(prop), "%smclk-fs", prefix);
	ret = of_property_read_u32(cpu, prop, &val);
	if (ret)
		dai_props->mclk_fs = 0;		/* default mclk 0Hz(un output) */
	else
		dai_props->mclk_fs = val;

	return 0;
}

int asoc_simple_parse_card_name(struct snd_soc_card *card,
				char *prefix)
{
	int ret;

	if (!prefix)
		prefix = "";

	/* Parse the card name from DT */
	ret = snd_soc_of_parse_card_name(card, "label");
	if (ret < 0 || !card->name) {
		char prop[128];

		snprintf(prop, sizeof(prop), "%sname", prefix);
		ret = snd_soc_of_parse_card_name(card, prop);
		if (ret < 0)
			return ret;
	}

	if (!card->name && card->dai_link)
		card->name = card->dai_link->name;

	return 0;
}

int asoc_simple_parse_dai(struct device_node *node,
			  struct snd_soc_dai_link_component *dlc,
			  const char *list_name, const char *cells_name,
			  int *is_single_link)
{
	struct of_phandle_args args;
	int ret;

	if (!node)
		return 0;

	/*
	 * Get node via "sound-dai = <&phandle port>"
	 * it will be used as xxx_of_node on soc_bind_dai_link()
	 */
	ret = of_parse_phandle_with_args(node, list_name, cells_name, 0, &args);
	if (ret)
		return ret;

	/*
	 * FIXME
	 *
	 * Here, dlc->dai_name is pointer to CPU/Codec DAI name.
	 * If user unbinded CPU or Codec driver, but not for Sound Card,
	 * dlc->dai_name is keeping unbinded CPU or Codec
	 * driver's pointer.
	 *
	 * If user re-bind CPU or Codec driver again, ALSA SoC will try
	 * to rebind Card via snd_soc_try_rebind_card(), but because of
	 * above reason, it might can't bind Sound Card.
	 * Because Sound Card is pointing to released dai_name pointer.
	 *
	 * To avoid this rebind Card issue,
	 * 1) It needs to alloc memory to keep dai_name eventhough
	 *    CPU or Codec driver was unbinded, or
	 * 2) user need to rebind Sound Card everytime
	 *    if he unbinded CPU or Codec.
	 */
	ret = snd_soc_of_get_dai_name(node, &dlc->dai_name);
	if (ret < 0)
		return ret;

	dlc->of_node = args.np;

	if (is_single_link)
		*is_single_link = !args.args_count;

	return 0;
}

int asoc_simple_set_dailink_name(struct device *dev,
				 struct snd_soc_dai_link *dai_link,
				 const char *fmt, ...)
{
	va_list ap;
	char *name = NULL;
	int ret = -ENOMEM;

	va_start(ap, fmt);
	name = devm_kvasprintf(dev, GFP_KERNEL, fmt, ap);
	va_end(ap);

	if (name) {
		ret = 0;

		dai_link->name		= name;
		dai_link->stream_name	= name;
	}

	return ret;
}

void asoc_simple_canonicalize_platform(struct snd_soc_dai_link *dai_link)
{
	/* Assumes platform == cpu */
	if (!dai_link->platforms->of_node)
		dai_link->platforms->of_node = dai_link->cpus->of_node;

	/*
	 * DPCM BE can be no platform.
	 * Alloced memory will be waste, but not leak.
	 */
	if (!dai_link->platforms->of_node)
		dai_link->num_platforms = 0;
}

void asoc_simple_canonicalize_cpu(struct snd_soc_dai_link *dai_link,
				  int is_single_links)
{
	/*
	 * In soc_bind_dai_link() will check cpu name after
	 * of_node matching if dai_link has cpu_dai_name.
	 * but, it will never match if name was created by
	 * fmt_single_name() remove cpu_dai_name if cpu_args
	 * was 0. See:
	 *	fmt_single_name()
	 *	fmt_multiple_name()
	 */
	if (is_single_links)
		dai_link->cpus->dai_name = NULL;
}

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard machine utils");

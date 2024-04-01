// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2020 BayLibre, SAS.
// Author: Jerome Brunet <jbrunet@baylibre.com>

#include <linux/bitfield.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>

#include <dt-bindings/sound/meson-aiu.h>
#include "aiu.h"
#include "meson-codec-glue.h"

#define AIU_HDMI_CLK_DATA_CTRL_CLK_SEL			GENMASK(1, 0)
#define AIU_HDMI_CLK_DATA_CTRL_CLK_SEL_DISABLE		0x0
#define AIU_HDMI_CLK_DATA_CTRL_CLK_SEL_PCM		0x1
#define AIU_HDMI_CLK_DATA_CTRL_CLK_SEL_AIU		0x2
#define AIU_HDMI_CLK_DATA_CTRL_DATA_SEL			GENMASK(5, 4)
#define AIU_HDMI_CLK_DATA_CTRL_DATA_SEL_OUTPUT_ZERO	0x0
#define AIU_HDMI_CLK_DATA_CTRL_DATA_SEL_PCM_DATA	0x1
#define AIU_HDMI_CLK_DATA_CTRL_DATA_SEL_I2S_DATA	0x2

#define AIU_CLK_CTRL_MORE_AMCLK				BIT(6)

#define AIU_HDMI_CTRL_MUX_DISABLED			0
#define AIU_HDMI_CTRL_MUX_PCM				1
#define AIU_HDMI_CTRL_MUX_I2S				2

static const char * const aiu_codec_hdmi_ctrl_mux_texts[] = {
	[AIU_HDMI_CTRL_MUX_DISABLED] =  "DISABLED",
	[AIU_HDMI_CTRL_MUX_PCM] = "PCM",
	[AIU_HDMI_CTRL_MUX_I2S] = "I2S",
};

static int aiu_codec_ctrl_mux_get_enum(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_dapm_kcontrol_component(kcontrol);
	unsigned int ctrl, more, mux = AIU_HDMI_CTRL_MUX_DISABLED;

	ctrl = snd_soc_component_read(component, AIU_HDMI_CLK_DATA_CTRL);
	if (FIELD_GET(AIU_HDMI_CLK_DATA_CTRL_CLK_SEL, ctrl) !=
	    AIU_HDMI_CLK_DATA_CTRL_CLK_SEL_AIU) {
		goto out;
	}

	more = snd_soc_component_read(component, AIU_CLK_CTRL_MORE);
	if (FIELD_GET(AIU_HDMI_CLK_DATA_CTRL_DATA_SEL, ctrl) ==
	    AIU_HDMI_CLK_DATA_CTRL_DATA_SEL_I2S_DATA &&
	    !!(more & AIU_CLK_CTRL_MORE_AMCLK)) {
		mux = AIU_HDMI_CTRL_MUX_I2S;
		goto out;
	}

	if (FIELD_GET(AIU_HDMI_CLK_DATA_CTRL_DATA_SEL, ctrl) ==
	    AIU_HDMI_CLK_DATA_CTRL_DATA_SEL_OUTPUT_ZERO &&
	    !(more & AIU_CLK_CTRL_MORE_AMCLK)) {
		mux = AIU_HDMI_CTRL_MUX_PCM;
		goto out;
	}

out:
	ucontrol->value.enumerated.item[0] = mux;
	return 0;
}

static int aiu_codec_ctrl_mux_put_enum(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
		snd_soc_dapm_kcontrol_component(kcontrol);
	struct snd_soc_dapm_context *dapm =
		snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int mux, ctrl, more;

	mux = snd_soc_enum_item_to_val(e, ucontrol->value.enumerated.item[0]);

	if (mux == AIU_HDMI_CTRL_MUX_I2S) {
		ctrl = FIELD_PREP(AIU_HDMI_CLK_DATA_CTRL_DATA_SEL,
				  AIU_HDMI_CLK_DATA_CTRL_DATA_SEL_I2S_DATA);
		more = AIU_CLK_CTRL_MORE_AMCLK;
	} else {
		ctrl = FIELD_PREP(AIU_HDMI_CLK_DATA_CTRL_DATA_SEL,
				  AIU_HDMI_CLK_DATA_CTRL_DATA_SEL_OUTPUT_ZERO);
		more = 0;
	}

	if (mux == AIU_HDMI_CTRL_MUX_DISABLED) {
		ctrl |= FIELD_PREP(AIU_HDMI_CLK_DATA_CTRL_CLK_SEL,
				   AIU_HDMI_CLK_DATA_CTRL_CLK_SEL_DISABLE);
	} else {
		ctrl |= FIELD_PREP(AIU_HDMI_CLK_DATA_CTRL_CLK_SEL,
				   AIU_HDMI_CLK_DATA_CTRL_CLK_SEL_AIU);
	}

	/* Force disconnect of the mux while updating */
	snd_soc_dapm_mux_update_power(dapm, kcontrol, 0, NULL, NULL);

	snd_soc_component_update_bits(component, AIU_HDMI_CLK_DATA_CTRL,
				      AIU_HDMI_CLK_DATA_CTRL_CLK_SEL |
				      AIU_HDMI_CLK_DATA_CTRL_DATA_SEL,
				      ctrl);

	snd_soc_component_update_bits(component, AIU_CLK_CTRL_MORE,
				      AIU_CLK_CTRL_MORE_AMCLK,
				      more);

	snd_soc_dapm_mux_update_power(dapm, kcontrol, mux, e, NULL);

	return 1;
}

static SOC_ENUM_SINGLE_VIRT_DECL(aiu_hdmi_ctrl_mux_enum,
				 aiu_codec_hdmi_ctrl_mux_texts);

static const struct snd_kcontrol_new aiu_hdmi_ctrl_mux =
	SOC_DAPM_ENUM_EXT("HDMI Source", aiu_hdmi_ctrl_mux_enum,
			  aiu_codec_ctrl_mux_get_enum,
			  aiu_codec_ctrl_mux_put_enum);

static const struct snd_soc_dapm_widget aiu_hdmi_ctrl_widgets[] = {
	SND_SOC_DAPM_MUX("HDMI CTRL SRC", SND_SOC_NOPM, 0, 0,
			 &aiu_hdmi_ctrl_mux),
};

static const struct snd_soc_dai_ops aiu_codec_ctrl_input_ops = {
	.hw_params	= meson_codec_glue_input_hw_params,
	.set_fmt	= meson_codec_glue_input_set_fmt,
};

static const struct snd_soc_dai_ops aiu_codec_ctrl_output_ops = {
	.startup	= meson_codec_glue_output_startup,
};

#define AIU_CODEC_CTRL_FORMATS					\
	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |	\
	 SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S24_LE |	\
	 SNDRV_PCM_FMTBIT_S32_LE)

#define AIU_CODEC_CTRL_STREAM(xname, xsuffix)			\
{								\
	.stream_name	= xname " " xsuffix,			\
	.channels_min	= 1,					\
	.channels_max	= 8,					\
	.rate_min       = 5512,					\
	.rate_max	= 192000,				\
	.formats	= AIU_CODEC_CTRL_FORMATS,		\
}

#define AIU_CODEC_CTRL_INPUT(xname) {				\
	.name = "CODEC CTRL " xname,				\
	.playback = AIU_CODEC_CTRL_STREAM(xname, "Playback"),	\
	.ops = &aiu_codec_ctrl_input_ops,			\
	.probe = meson_codec_glue_input_dai_probe,		\
	.remove = meson_codec_glue_input_dai_remove,		\
}

#define AIU_CODEC_CTRL_OUTPUT(xname) {				\
	.name = "CODEC CTRL " xname,				\
	.capture = AIU_CODEC_CTRL_STREAM(xname, "Capture"),	\
	.ops = &aiu_codec_ctrl_output_ops,			\
}

static struct snd_soc_dai_driver aiu_hdmi_ctrl_dai_drv[] = {
	[CTRL_I2S] = AIU_CODEC_CTRL_INPUT("HDMI I2S IN"),
	[CTRL_PCM] = AIU_CODEC_CTRL_INPUT("HDMI PCM IN"),
	[CTRL_OUT] = AIU_CODEC_CTRL_OUTPUT("HDMI OUT"),
};

static const struct snd_soc_dapm_route aiu_hdmi_ctrl_routes[] = {
	{ "HDMI CTRL SRC", "I2S", "HDMI I2S IN Playback" },
	{ "HDMI CTRL SRC", "PCM", "HDMI PCM IN Playback" },
	{ "HDMI OUT Capture", NULL, "HDMI CTRL SRC" },
};

static int aiu_hdmi_of_xlate_dai_name(struct snd_soc_component *component,
				      const struct of_phandle_args *args,
				      const char **dai_name)
{
	return aiu_of_xlate_dai_name(component, args, dai_name, AIU_HDMI);
}

static const struct snd_soc_component_driver aiu_hdmi_ctrl_component = {
	.name			= "AIU HDMI Codec Control",
	.dapm_widgets		= aiu_hdmi_ctrl_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(aiu_hdmi_ctrl_widgets),
	.dapm_routes		= aiu_hdmi_ctrl_routes,
	.num_dapm_routes	= ARRAY_SIZE(aiu_hdmi_ctrl_routes),
	.of_xlate_dai_name	= aiu_hdmi_of_xlate_dai_name,
	.endianness		= 1,
#ifdef CONFIG_DEBUG_FS
	.debugfs_prefix		= "hdmi",
#endif
};

int aiu_hdmi_ctrl_register_component(struct device *dev)
{
	return snd_soc_register_component(dev, &aiu_hdmi_ctrl_component,
					  aiu_hdmi_ctrl_dai_drv,
					  ARRAY_SIZE(aiu_hdmi_ctrl_dai_drv));
}


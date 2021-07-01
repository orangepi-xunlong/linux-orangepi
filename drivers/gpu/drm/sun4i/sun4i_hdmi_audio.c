// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Olimex Ltd.
 *   Author: Stefan Mavrodiev <stefan@olimex.com>
 */
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/module.h>
#include <linux/of_dma.h>
#include <linux/regmap.h>

#include <drm/drm_print.h>

#include <sound/dmaengine_pcm.h>
#include <sound/pcm_drm_eld.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "sun4i_hdmi.h"

static int sun4i_hdmi_audio_ctl_eld_info(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = MAX_ELD_BYTES;
	return 0;
}

static int sun4i_hdmi_audio_ctl_eld_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_card *card = snd_soc_component_get_drvdata(component);
	struct sun4i_hdmi *hdmi = snd_soc_card_get_drvdata(card);

	memcpy(ucontrol->value.bytes.data,
	       hdmi->connector.eld,
	       MAX_ELD_BYTES);

	return 0;
}

static const struct snd_kcontrol_new sun4i_hdmi_audio_controls[] = {
	{
		.access = SNDRV_CTL_ELEM_ACCESS_READ |
			  SNDRV_CTL_ELEM_ACCESS_VOLATILE,
		.iface = SNDRV_CTL_ELEM_IFACE_PCM,
		.name = "ELD",
		.info = sun4i_hdmi_audio_ctl_eld_info,
		.get = sun4i_hdmi_audio_ctl_eld_get,
	},
};

static const struct snd_soc_dapm_widget sun4i_hdmi_audio_widgets[] = {
	SND_SOC_DAPM_OUTPUT("TX"),
};

static const struct snd_soc_dapm_route sun4i_hdmi_audio_routes[] = {
	{ "TX", NULL, "Playback" },
};

static const struct snd_soc_component_driver sun4i_hdmi_audio_component = {
	.controls               = sun4i_hdmi_audio_controls,
	.num_controls           = ARRAY_SIZE(sun4i_hdmi_audio_controls),
	.dapm_widgets		= sun4i_hdmi_audio_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(sun4i_hdmi_audio_widgets),
	.dapm_routes		= sun4i_hdmi_audio_routes,
	.num_dapm_routes	= ARRAY_SIZE(sun4i_hdmi_audio_routes),
};

static int sun4i_hdmi_audio_startup(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct sun4i_hdmi *hdmi = snd_soc_card_get_drvdata(card);
	u32 reg;
	int ret;

	regmap_write(hdmi->regmap, SUN4I_HDMI_AUDIO_CTRL_REG, 0);
	regmap_write(hdmi->regmap,
		     SUN4I_HDMI_AUDIO_CTRL_REG,
		     SUN4I_HDMI_AUDIO_CTRL_RESET);
	ret = regmap_read_poll_timeout(hdmi->regmap,
				       SUN4I_HDMI_AUDIO_CTRL_REG,
				       reg, !reg, 100, 50000);
	if (ret < 0) {
		DRM_ERROR("Failed to reset HDMI Audio\n");
		return ret;
	}

	regmap_write(hdmi->regmap,
		     SUN4I_HDMI_AUDIO_CTRL_REG,
		     SUN4I_HDMI_AUDIO_CTRL_ENABLE);

	return snd_pcm_hw_constraint_eld(substream->runtime,
					hdmi->connector.eld);
}

static void sun4i_hdmi_audio_shutdown(struct snd_pcm_substream *substream,
				      struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct sun4i_hdmi *hdmi = snd_soc_card_get_drvdata(card);

	regmap_write(hdmi->regmap, SUN4I_HDMI_AUDIO_CTRL_REG, 0);
}

static int sun4i_hdmi_setup_audio_infoframes(struct sun4i_hdmi *hdmi)
{
	union hdmi_infoframe frame;
	u8 buffer[14];
	int i, ret;

	ret = hdmi_audio_infoframe_init(&frame.audio);
	if (ret < 0) {
		DRM_ERROR("Failed to init HDMI audio infoframe\n");
		return ret;
	}

	frame.audio.coding_type = HDMI_AUDIO_CODING_TYPE_STREAM;
	frame.audio.sample_frequency = HDMI_AUDIO_SAMPLE_FREQUENCY_STREAM;
	frame.audio.sample_size = HDMI_AUDIO_SAMPLE_SIZE_STREAM;
	frame.audio.channels = hdmi->audio.channels;

	ret = hdmi_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (ret < 0) {
		DRM_ERROR("Failed to pack HDMI audio infoframe\n");
		return ret;
	}

	for (i = 0; i < sizeof(buffer); i++)
		writeb(buffer[i],
		       hdmi->base + SUN4I_HDMI_AUDIO_INFOFRAME_REG(i));

	return 0;
}

static void sun4i_hdmi_audio_set_cts_n(struct sun4i_hdmi *hdmi,
				       struct snd_pcm_hw_params *params)
{
	struct drm_encoder *encoder = &hdmi->encoder;
	struct drm_crtc *crtc = encoder->crtc;
	const struct drm_display_mode *mode = &crtc->state->adjusted_mode;
	u32 rate = params_rate(params);
	u32 n, cts;
	u64 tmp;

	/**
	 * Calculate Cycle Time Stamp (CTS) and Numerator (N):
	 *
	 * N = 128 * Samplerate / 1000
	 * CTS = (Ftdms * N) / (128 * Samplerate)
	 */

	n = 128 * rate / 1000;
	tmp = (u64)(mode->clock * 1000) * n;
	do_div(tmp, 128 * rate);
	cts = tmp;

	regmap_write(hdmi->regmap,
		     SUN4I_HDMI_AUDIO_CTS_REG,
		     SUN4I_HDMI_AUDIO_CTS(cts));

	regmap_write(hdmi->regmap,
		     SUN4I_HDMI_AUDIO_N_REG,
		     SUN4I_HDMI_AUDIO_N(n));
}

static int sun4i_hdmi_audio_set_hw_rate(struct sun4i_hdmi *hdmi,
					struct snd_pcm_hw_params *params)
{
	u32 rate = params_rate(params);
	u32 val;

	switch (rate) {
	case 44100:
		val = 0x0;
		break;
	case 48000:
		val = 0x2;
		break;
	case 32000:
		val = 0x3;
		break;
	case 88200:
		val = 0x8;
		break;
	case 96000:
		val = 0x9;
		break;
	case 176400:
		val = 0xc;
		break;
	case 192000:
		val = 0xe;
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(hdmi->regmap,
			   SUN4I_HDMI_AUDIO_STAT0_REG,
			   SUN4I_HDMI_AUDIO_STAT0_FREQ_MASK,
			   SUN4I_HDMI_AUDIO_STAT0_FREQ(val));

	return 0;
}

static int sun4i_hdmi_audio_set_hw_channels(struct sun4i_hdmi *hdmi,
					    struct snd_pcm_hw_params *params)
{
	u32 channels = params_channels(params);

	if (channels > 8)
		return -EINVAL;

	hdmi->audio.channels = channels;

	regmap_update_bits(hdmi->regmap,
			   SUN4I_HDMI_AUDIO_FMT_REG,
			   SUN4I_HDMI_AUDIO_FMT_LAYOUT,
			   (channels > 2) ? SUN4I_HDMI_AUDIO_FMT_LAYOUT : 0);

	regmap_update_bits(hdmi->regmap,
			   SUN4I_HDMI_AUDIO_FMT_REG,
			   SUN4I_HDMI_AUDIO_FMT_CH_CFG_MASK,
			   SUN4I_HDMI_AUDIO_FMT_CH_CFG(channels));

	regmap_write(hdmi->regmap, SUN4I_HDMI_AUDIO_PCM_REG, 0x76543210);

	/**
	 * If only one channel is required, send the same sample
	 * to the sink device as a left and right channel.
	 */
	if (channels == 1)
		regmap_update_bits(hdmi->regmap,
				   SUN4I_HDMI_AUDIO_PCM_REG,
				   SUN4I_HDMI_AUDIO_PCM_CH_MAP_MASK(1),
				   SUN4I_HDMI_AUDIO_PCM_CH_MAP(1, 1));

	return 0;
}

static int sun4i_hdmi_audio_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params,
				      struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct sun4i_hdmi *hdmi = snd_soc_card_get_drvdata(card);
	int ret;

	ret = sun4i_hdmi_audio_set_hw_rate(hdmi, params);
	if (ret < 0)
		return ret;

	ret = sun4i_hdmi_audio_set_hw_channels(hdmi, params);
	if (ret < 0)
		return ret;

	sun4i_hdmi_audio_set_cts_n(hdmi, params);

	return 0;
}

static int sun4i_hdmi_audio_trigger(struct snd_pcm_substream *substream,
				    int cmd,
				    struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct sun4i_hdmi *hdmi = snd_soc_card_get_drvdata(card);
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		ret = sun4i_hdmi_setup_audio_infoframes(hdmi);
		break;
	default:
		break;
	}

	return ret;
}

static const struct snd_soc_dai_ops sun4i_hdmi_audio_dai_ops = {
	.startup = sun4i_hdmi_audio_startup,
	.shutdown = sun4i_hdmi_audio_shutdown,
	.hw_params = sun4i_hdmi_audio_hw_params,
	.trigger = sun4i_hdmi_audio_trigger,
};

static int sun4i_hdmi_audio_dai_probe(struct snd_soc_dai *dai)
{
	struct snd_dmaengine_dai_dma_data *dma_data;

	dma_data = devm_kzalloc(dai->dev, sizeof(*dma_data), GFP_KERNEL);
	if (!dma_data)
		return -ENOMEM;

	dma_data->addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dma_data->maxburst = 8;

	snd_soc_dai_init_dma_data(dai, dma_data, NULL);

	return 0;
}

static struct snd_soc_dai_driver sun4i_hdmi_audio_dai = {
	.name = "HDMI",
	.ops = &sun4i_hdmi_audio_dai_ops,
	.probe = sun4i_hdmi_audio_dai_probe,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 8,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE,
		.rates		= SNDRV_PCM_RATE_8000_192000,
	},
};

static const struct snd_pcm_hardware sun4i_hdmi_audio_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED |
				  SNDRV_PCM_INFO_BLOCK_TRANSFER |
				  SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_PAUSE |
				  SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates                  = SNDRV_PCM_RATE_8000_192000,
	.rate_min               = 8000,
	.rate_max               = 192000,
	.channels_min           = 1,
	.channels_max           = 8,
	.buffer_bytes_max	= 128 * 1024,
	.period_bytes_min	= 4 * 1024,
	.period_bytes_max	= 32 * 1024,
	.periods_min		= 2,
	.periods_max		= 8,
	.fifo_size		= 128,
};

static const struct snd_dmaengine_pcm_config sun4i_hdmi_audio_pcm_config = {
	.chan_names[SNDRV_PCM_STREAM_PLAYBACK] = "audio-tx",
	.pcm_hardware = &sun4i_hdmi_audio_pcm_hardware,
	.prealloc_buffer_size = 128 * 1024,
	.prepare_slave_config = snd_dmaengine_pcm_prepare_slave_config,
};

struct snd_soc_card sun4i_hdmi_audio_card = {
	.name = "sun4i-hdmi",
};

int sun4i_hdmi_audio_create(struct sun4i_hdmi *hdmi)
{
	struct snd_soc_card *card = &sun4i_hdmi_audio_card;
	struct snd_soc_dai_link_component *comp;
	struct snd_soc_dai_link *link;
	int ret;

	ret = snd_dmaengine_pcm_register(hdmi->dev,
					 &sun4i_hdmi_audio_pcm_config, 0);
	if (ret < 0) {
		DRM_ERROR("Could not register PCM\n");
		return ret;
	}

	ret = snd_soc_register_component(hdmi->dev,
					 &sun4i_hdmi_audio_component,
					 &sun4i_hdmi_audio_dai, 1);
	if (ret < 0) {
		DRM_ERROR("Could not register DAI\n");
		goto unregister_pcm;
	}

	link = devm_kzalloc(hdmi->dev, sizeof(*link), GFP_KERNEL);
	if (!link) {
		ret = -ENOMEM;
		goto unregister_component;
	}

	comp = devm_kzalloc(hdmi->dev, sizeof(*comp) * 3, GFP_KERNEL);
	if (!comp) {
		ret = -ENOMEM;
		goto unregister_component;
	}

	link->cpus = &comp[0];
	link->codecs = &comp[1];
	link->platforms = &comp[2];

	link->num_cpus = 1;
	link->num_codecs = 1;
	link->num_platforms = 1;

	link->playback_only = 1;

	link->name = "SUN4I-HDMI";
	link->stream_name = "SUN4I-HDMI PCM";

	link->codecs->name = dev_name(hdmi->dev);
	link->codecs->dai_name	= sun4i_hdmi_audio_dai.name;

	link->cpus->dai_name = dev_name(hdmi->dev);

	link->platforms->name = dev_name(hdmi->dev);

	link->dai_fmt = SND_SOC_DAIFMT_I2S;

	card->dai_link = link;
	card->num_links = 1;
	card->dev = hdmi->dev;

	hdmi->audio.card = card;

	/**
	 * snd_soc_register_card() will overwrite the driver_data pointer.
	 * So before registering the card, store the original pointer in
	 * card->drvdata.
	 */
	snd_soc_card_set_drvdata(card, hdmi);
	ret = snd_soc_register_card(card);
	if (ret)
		goto unregister_component;

	return 0;

unregister_component:
	snd_soc_unregister_component(hdmi->dev);
unregister_pcm:
	snd_dmaengine_pcm_unregister(hdmi->dev);
	return ret;
}

void sun4i_hdmi_audio_destroy(struct sun4i_hdmi *hdmi)
{
	struct snd_soc_card *card = hdmi->audio.card;
	void *data;

	/**
	 * Before removing the card, restore the previously stored driver_data.
	 * This will ensure proper removal of the sun4i-hdmi module, since it
	 * uses dev_get_drvdata() in the unbind function.
	 */
	data = snd_soc_card_get_drvdata(card);

	snd_soc_unregister_card(card);
	snd_soc_unregister_component(hdmi->dev);
	snd_dmaengine_pcm_unregister(hdmi->dev);

	dev_set_drvdata(hdmi->dev, data);
}

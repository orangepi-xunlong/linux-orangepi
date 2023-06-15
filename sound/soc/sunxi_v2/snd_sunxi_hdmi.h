/*
 * sound\soc\sunxi\snd_sunxi_hdmi.h
 * (C) Copyright 2021-2025
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_HDMI_H
#define __SND_SUNXI_HDMI_H

#define	SUNXI_DAI_I2S_TYPE	0
#define	SUNXI_DAI_HDMI_TYPE	1

enum HDMI_FORMAT {
	HDMI_FMT_NULL = 0,
	HDMI_FMT_PCM = 1,
	HDMI_FMT_AC3,
	HDMI_FMT_MPEG1,
	HDMI_FMT_MP3,
	HDMI_FMT_MPEG2,
	HDMI_FMT_AAC,
	HDMI_FMT_DTS,
	HDMI_FMT_ATRAC,
	HDMI_FMT_ONE_BIT_AUDIO,
	HDMI_FMT_DOLBY_DIGITAL_PLUS,
	HDMI_FMT_DTS_HD,
	HDMI_FMT_MAT,
	HDMI_FMT_DST,
	HDMI_FMT_WMAPRO,
};

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_HDMIAUDIO)
extern int snd_sunxi_transfer_format_61937_to_60958(int *out, short *temp,
						    int samples, int rate,
						    enum HDMI_FORMAT data_fmt);
extern enum HDMI_FORMAT snd_sunxi_hdmi_get_fmt(void);
extern int snd_sunxi_hdmi_add_controls(struct snd_soc_component *component);
extern int snd_sunxi_hdmi_get_dai_type(struct device_node *np,
				       unsigned int *dai_type);
extern int snd_sunxi_hdmi_init(void);
extern int snd_sunxi_hdmi_hw_params(struct snd_pcm_hw_params *params,
				    enum HDMI_FORMAT hdmi_fmt);
extern int snd_sunxi_hdmi_prepare(void);
extern void snd_sunxi_hdmi_shutdown(void);
#else
static inline int snd_sunxi_transfer_format_61937_to_60958(int *out, short *temp,
							   int samples, int rate,
							   enum HDMI_FORMAT data_fmt)
{
	pr_err("HDMIAUDIO API is disabled\n");

	return 0;
}

static inline enum HDMI_FORMAT snd_sunxi_hdmi_get_fmt(void)
{
	return HDMI_FMT_PCM;
}

static inline int snd_sunxi_hdmi_add_controls(struct snd_soc_component *component)
{
	return 0;
}

static inline int snd_sunxi_hdmi_get_dai_type(struct device_node *np,
					      unsigned int *dai_type)
{
	*dai_type = SUNXI_DAI_I2S_TYPE;

	return 0;
}

static inline int snd_sunxi_hdmi_init(void)
{
	return 0;
}

static inline int snd_sunxi_hdmi_hw_params(struct snd_pcm_hw_params *params,
					   enum HDMI_FORMAT hdmi_fmt)
{
	return 0;
}

static inline int snd_sunxi_hdmi_prepare(void)
{
	return 0;
}

static inline void snd_sunxi_hdmi_shutdown(void)
{
	return;
}
#endif

#endif /* __SND_SUNXI_HDMI_H */


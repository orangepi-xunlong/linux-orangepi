/*
 * sound\soc\sunxi\snd_sunxi_hdmi.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include <linux/module.h>
#include <video/drv_hdmi.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_hdmi.h"

#define HLOG		"AHDMI"

/* for HDMI func api */
struct sunxi_hdmi_priv {
	hdmi_audio_t hdmi_para;
	bool update_param;
};
static enum HDMI_FORMAT g_hdmi_fmt;
static __audio_hdmi_func g_hdmi_func;
static struct sunxi_hdmi_priv g_hdmi_priv;

/* data format transfer */
static unsigned int channel_status[192];
struct headbpcuv {
	unsigned char other:3;
	unsigned char V:1;
	unsigned char U:1;
	unsigned char C:1;
	unsigned char P:1;
	unsigned char B:1;
};
union head61937 {
	struct headbpcuv head0;
	unsigned char head1;
} head;
union word {
	struct {
		unsigned int bit0:1;
		unsigned int bit1:1;
		unsigned int bit2:1;
		unsigned int bit3:1;
		unsigned int bit4:1;
		unsigned int bit5:1;
		unsigned int bit6:1;
		unsigned int bit7:1;
		unsigned int bit8:1;
		unsigned int bit9:1;
		unsigned int bit10:1;
		unsigned int bit11:1;
		unsigned int bit12:1;
		unsigned int bit13:1;
		unsigned int bit14:1;
		unsigned int bit15:1;
		unsigned int rsvd:16;
	} bits;
	unsigned int wval;
} wordformat;

/* sunxi_transfer_format_61937_to_60958
 * ISO61937 to ISO60958, for HDMIAUDIO & SPDIF
 */
int snd_sunxi_transfer_format_61937_to_60958(int *out, short *temp,
					     int samples, int rate,
					     enum HDMI_FORMAT data_fmt)
{
	int ret = 0;
	int i;
	static int numtotal;
	union word w1;

	samples >>= 1;
	head.head0.other = 0;
	head.head0.B = 1;
	head.head0.P = 0;
	head.head0.C = 0;
	head.head0.U = 0;
	head.head0.V = 1;

	for (i = 0; i < 192; i++)
		channel_status[i] = 0;

	channel_status[1] = 1;
	/* sample rates */
	if (rate == 32000) {
		channel_status[24] = 1;
		channel_status[25] = 1;
		channel_status[26] = 0;
		channel_status[27] = 0;
	} else if (rate == 44100) {
		channel_status[24] = 0;
		channel_status[25] = 0;
		channel_status[26] = 0;
		channel_status[27] = 0;
	} else if (rate == 48000) {
		channel_status[24] = 0;
		channel_status[25] = 1;
		channel_status[26] = 0;
		channel_status[27] = 0;
	} else if (rate == (32000*4)) {
		channel_status[24] = 1;
		channel_status[25] = 0;
		channel_status[26] = 0;
		channel_status[27] = 0;
	} else if (rate == (44100*4)) {
		channel_status[24] = 0;
		channel_status[25] = 0;
		channel_status[26] = 1;
		channel_status[27] = 1;
	} else if (rate == (48000*4)) {
		channel_status[24] = 0;
		channel_status[25] = 1;
		channel_status[26] = 1;
		channel_status[27] = 1;
		if (data_fmt == HDMI_FMT_DTS_HD || data_fmt == HDMI_FMT_MAT) {
			channel_status[24] = 1;
			channel_status[25] = 0;
			channel_status[26] = 0;
			channel_status[27] = 1;
		}
	} else {
		channel_status[24] = 0;
		channel_status[25] = 1;
		channel_status[26] = 0;
		channel_status[27] = 0;
	}

	for (i = 0; i < samples; i++, numtotal++) {
		if ((numtotal % 384 == 0) || (numtotal % 384 == 1))
			head.head0.B = 1;
		else
			head.head0.B = 0;

		head.head0.C = channel_status[(numtotal % 384)/2];

		if (numtotal % 384 == 0)
			numtotal = 0;

		w1.wval = (*temp) & (0xffff);

		head.head0.P = w1.bits.bit15 ^ w1.bits.bit14 ^ w1.bits.bit13 ^
			       w1.bits.bit12 ^ w1.bits.bit11 ^ w1.bits.bit10 ^
			       w1.bits.bit9 ^ w1.bits.bit8 ^ w1.bits.bit7 ^
			       w1.bits.bit6 ^ w1.bits.bit5 ^ w1.bits.bit4 ^
			       w1.bits.bit3 ^ w1.bits.bit2 ^ w1.bits.bit1 ^
			       w1.bits.bit0;

		ret = (int)(head.head1) << 24;
		/* 11 may can be replace by 8 or 12 */
		ret |= (int)((w1.wval)&(0xffff)) << 11;
		*out = ret;
		out++;
		temp++;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_transfer_format_61937_to_60958);

enum HDMI_FORMAT snd_sunxi_hdmi_get_fmt(void)
{
	return g_hdmi_fmt;
}
EXPORT_SYMBOL_GPL(snd_sunxi_hdmi_get_fmt);

static int sunxi_data_fmt_get_data_fmt(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = g_hdmi_fmt;

	return 0;
}

static int sunxi_data_fmt_set_data_fmt(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	g_hdmi_fmt = ucontrol->value.integer.value[0];

	return 0;
}

static const char *data_fmt[] = {
	"NULL", "PCM", "AC3", "MPEG1", "MP3", "MPEG2", "AAC", "DTS", "ATRAC",
	"ONE_BIT_AUDIO", "DOLBY_DIGITAL_PLUS", "DTS_HD", "MAT", "DST", "WMAPRO"
};
static SOC_ENUM_SINGLE_EXT_DECL(data_fmt_enum, data_fmt);
static const struct snd_kcontrol_new data_fmt_controls[] = {
	SOC_ENUM_EXT("audio data format", data_fmt_enum,
		     sunxi_data_fmt_get_data_fmt,
		     sunxi_data_fmt_set_data_fmt),
};

int snd_sunxi_hdmi_add_controls(struct snd_soc_component *component)
{
	int ret;

	if (!component) {
		SND_LOG_ERR(HLOG, "component is err\n");
		return -1;
	}

	g_hdmi_fmt = HDMI_FMT_PCM;

	ret = snd_soc_add_component_controls(component,
					     data_fmt_controls,
					     ARRAY_SIZE(data_fmt_controls));
	if (ret)
		SND_LOG_ERR(HLOG, "add kcontrols failed\n");

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_hdmi_add_controls);

int snd_sunxi_hdmi_get_dai_type(struct device_node *np, unsigned int *dai_type)
{
	int ret;
	const char *str;

	SND_LOG_DEBUG(HLOG, "\n");

	if (!np) {
		SND_LOG_ERR(HLOG, "np is err\n");
		return -1;
	}

	ret = of_property_read_string(np, "dai_type", &str);
	if (ret < 0) {
		*dai_type = SUNXI_DAI_I2S_TYPE;
	} else {
		if (strcmp(str, "hdmi") == 0) {
			*dai_type = SUNXI_DAI_HDMI_TYPE;
		} else {
			*dai_type = SUNXI_DAI_I2S_TYPE;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_hdmi_get_dai_type);

int snd_sunxi_hdmi_init(void)
{
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = snd_hdmi_get_func(&g_hdmi_func);
	if (ret) {
		SND_LOG_ERR(HLOG, "get hdmi audio func failed\n");
		return -1;
	}

	if (!g_hdmi_func.hdmi_audio_enable) {
		SND_LOG_ERR(HLOG, "hdmi_audio_enable func is null\n");
		return -1;
	}
	if (!g_hdmi_func.hdmi_set_audio_para) {
		SND_LOG_ERR(HLOG, "hdmi_set_audio_para func is null\n");
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_hdmi_init);

int snd_sunxi_hdmi_hw_params(struct snd_pcm_hw_params *params,
			     enum HDMI_FORMAT hdmi_fmt)
{
	static hdmi_audio_t hdmi_para_tmp;
	static hdmi_audio_t *hdmi_para = &g_hdmi_priv.hdmi_para;

	SND_LOG_DEBUG(HLOG, "\n");

	hdmi_para_tmp.sample_rate = params_rate(params);
	hdmi_para_tmp.channel_num = params_channels(params);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		hdmi_para_tmp.sample_bit = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		hdmi_para_tmp.sample_bit = 24;
		break;
	default:
		return -EINVAL;
	}
	if (hdmi_fmt > HDMI_FMT_PCM) {
		hdmi_para_tmp.sample_bit = 24;
	}

	if (hdmi_para_tmp.channel_num == 8)
		hdmi_para_tmp.ca = 0x13;
	else if (hdmi_para_tmp.channel_num == 6)
		hdmi_para_tmp.ca = 0x0b;
	else if ((hdmi_para_tmp.channel_num >= 3))
		hdmi_para_tmp.ca = 0x1f;
	else
		hdmi_para_tmp.ca = 0x0;

	hdmi_para_tmp.data_raw = hdmi_fmt;

	if (hdmi_para_tmp.sample_rate	!= hdmi_para->sample_rate ||
	    hdmi_para_tmp.channel_num	!= hdmi_para->channel_num ||
	    hdmi_para_tmp.sample_bit	!= hdmi_para->sample_bit ||
	    hdmi_para_tmp.ca		!= hdmi_para->ca ||
	    hdmi_para_tmp.data_raw	!= hdmi_para->data_raw) {
		g_hdmi_priv.update_param = 1;
		hdmi_para->sample_rate = hdmi_para_tmp.sample_rate;
		hdmi_para->channel_num = hdmi_para_tmp.channel_num;
		hdmi_para->sample_bit = hdmi_para_tmp.sample_bit;
		hdmi_para->ca = hdmi_para_tmp.ca;
		hdmi_para->data_raw = hdmi_para_tmp.data_raw;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_hdmi_hw_params);

int snd_sunxi_hdmi_prepare(void)
{
	static hdmi_audio_t *hdmi_para = &g_hdmi_priv.hdmi_para;

	SND_LOG_DEBUG(HLOG, "\n");

	if (!g_hdmi_priv.update_param)
		return 0;

	SND_LOG_DEBUG(HLOG, "hdmi audio update info\n");
	SND_LOG_DEBUG(HLOG, "data raw : %d\n", hdmi_para->data_raw);
	SND_LOG_DEBUG(HLOG, "bit      : %d\n", hdmi_para->sample_bit);
	SND_LOG_DEBUG(HLOG, "channel  : %d\n", hdmi_para->channel_num);
	SND_LOG_DEBUG(HLOG, "rate     : %d\n", hdmi_para->sample_rate);

	g_hdmi_func.hdmi_set_audio_para(hdmi_para);
	if (g_hdmi_func.hdmi_audio_enable(1, 1)) {
		SND_LOG_ERR(HLOG, "hdmi_audio_enable failed\n");
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_hdmi_prepare);

void snd_sunxi_hdmi_shutdown(void)
{
	SND_LOG_DEBUG(HLOG, "\n");

	g_hdmi_priv.update_param = 0;

	if (g_hdmi_func.hdmi_audio_enable)
		g_hdmi_func.hdmi_audio_enable(0, 1);
}
EXPORT_SYMBOL_GPL(snd_sunxi_hdmi_shutdown);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of hdmiaudio");

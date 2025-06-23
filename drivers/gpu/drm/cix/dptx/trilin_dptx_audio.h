/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Cix Technology Group Co., Ltd. */

#ifndef __TRILIN_DPTX_AUDIO_H__
#define __TRILIN_DPTX_AUDIO_H__

#include <sound/jack.h>
#include <sound/hdmi-codec.h>

//------------------------------------------------------------------------------
//  for audio channel status
//------------------------------------------------------------------------------
/* bit 20~21 clock accuracy */
#define DPTX_CS_CLOCK_ACCURACY_LEVELI      (0b10)
#define DPTX_CS_CLOCK_ACCURACY_LEVELII     (0b00)
#define DPTX_CS_CLOCK_ACCURACY_LEVELIII    (0b01)
/* bit 24~27 Sampling frequency */
#define DPTX_CS_SAMPLING_FREQ_32000Hz      (0b1100 << 4)
#define DPTX_CS_SAMPLING_FREQ_44100Hz      (0b0000 << 4);
#define DPTX_CS_SAMPLING_FREQ_48000Hz      (0b0100 << 4);
/* bit 32~35 Word length */
#define DPTX_CS_SAMPLE_WORD_LENGTH_16BITS  (0b0100 << 4)
#define DPTX_CS_SAMPLE_WORD_LENGTH_18BITS  (0b0010 << 4)
#define DPTX_CS_SAMPLE_WORD_LENGTH_20BITS  (0b1100 << 4)
#define DPTX_CS_SAMPLE_WORD_LENGTH_24BITS  (0b1101 << 4)
/* bit 36~39 Original sampling frequency */
#define DPTX_CS_SAMPLING_ORIG_FREQ_32000HZ (0b0011)
#define DPTX_CS_SAMPLING_ORIG_FREQ_44100HZ (0b1111)
#define DPTX_CS_SAMPLING_ORIG_FREQ_48000HZ (0b1011)
#define DPTX_CS_SOURCE_NUMBER_SHIFT        (4)
#define DPTX_CS_CATEGORY_CODE              (0)
#define DPTX_CS_COPYRIGHT                  (0)
#define DPTX_CS_TIMESTAMP_INTERVAL         (0x1f)
#define DPTX_CS_AUDIO_CHANNEL_MAP_DEFAULT  (0x87654321)

struct dptx_audio {
	struct platform_device *pdev;
	hdmi_codec_plugged_cb plugged_cb;
	struct device *codec_dev;

	struct hdmi_codec_params params;
	bool running;
};

extern const struct hdmi_codec_ops dptx_audio_codec_ops;
void dptx_audio_handle_plugged_change(struct dptx_audio *dp_audio,
				      bool plugged);

void dptx_audio_reconfig_and_enable(void *data);

#endif

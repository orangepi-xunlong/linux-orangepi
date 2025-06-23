/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Cix Technology Group Co., Ltd. */

#ifndef __HDA_PCM_H__
#define __HDA_PCM_H__
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/soc-dai.h>
#include <sound/pcm_params.h>

#define HDA_RATES   (SNDRV_PCM_RATE_8000_192000)
#define HDA_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
				| SNDRV_PCM_FMTBIT_S24_LE \
				| SNDRV_PCM_FMTBIT_S24_3LE \
				| SNDRV_PCM_FMTBIT_S32_LE)

struct hda_ipbloq;

int hda_pcm_open(struct snd_soc_component *component,
				struct snd_pcm_substream *ss);
int hda_pcm_close(struct snd_soc_component *component,
				struct snd_pcm_substream *ss);
int hda_pcm_hw_params(struct snd_soc_component *component,
				struct snd_pcm_substream *ss,
				struct snd_pcm_hw_params *params);
int hda_pcm_hw_free(struct snd_soc_component *component,
					struct snd_pcm_substream *ss);
int hda_pcm_prepare(struct snd_soc_component *component,
					struct snd_pcm_substream *ss);
int hda_pcm_trigger(struct snd_soc_component *component,
					struct snd_pcm_substream *ss, int cmd);
snd_pcm_uframes_t hda_pcm_pointer(struct snd_soc_component *component,
								struct snd_pcm_substream *ss);
int hda_pcm_new(struct snd_soc_component *component,
			     struct snd_soc_pcm_runtime *rtd);

void hda_pcm_free(struct snd_soc_component *component,
		  struct snd_pcm *pcm);
#endif

// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.
#include "hda_pcm.h"
#include "hdac.h"

#define MAX_PREALLOC_SIZE       (32 * 1024 * 1024)

static const struct snd_pcm_hardware hda_pcm_hw = {
	.info =			(SNDRV_PCM_INFO_MMAP |
				 SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_BLOCK_TRANSFER |
				 SNDRV_PCM_INFO_MMAP_VALID |
				 /* No full-resume yet implemented */
				 /* SNDRV_PCM_INFO_RESUME |*/
				 SNDRV_PCM_INFO_PAUSE |
				 SNDRV_PCM_INFO_SYNC_START |
				 SNDRV_PCM_INFO_HAS_WALL_CLOCK | /* legacy */
				 SNDRV_PCM_INFO_HAS_LINK_ATIME |
				 SNDRV_PCM_INFO_NO_PERIOD_WAKEUP),
	.formats =	HDA_FORMATS,
	.rates =	SNDRV_PCM_RATE_48000,
	.rate_min =	44100,
	.rate_max =	48000,
	.channels_min =		2,
	.channels_max =		2,
	.buffer_bytes_max =	AZX_MAX_BUF_SIZE,
	.period_bytes_min =	128,
	.period_bytes_max =	AZX_MAX_BUF_SIZE / 2,
	.periods_min =		2,
	.periods_max =		AZX_MAX_FRAG,
	.fifo_size =		0,
};

/* assign a stream for the PCM */
static inline struct hdac_stream *
hdac_assign_stream(struct hdac *hdac, struct snd_pcm_substream *ss)
{
	struct hdac_stream *hstr;

	hstr = snd_hdac_stream_assign(hdac_bus(hdac), ss);
	if (!hstr)
		return NULL;

	return hstr;
}

/* release the assigned stream */
static inline void hdac_release_stream(struct hdac_stream *hstr)
{
	snd_hdac_stream_release(hstr);
}

/* get the assigned stream */
static inline struct hdac_stream *
get_hdac_stream(struct snd_pcm_substream *ss)
{
	return ss->runtime->private_data;
}

/* get the hdac data */
static inline struct hdac *hdac_data(struct snd_pcm_substream *ss)
{
	return snd_pcm_substream_chip(ss);
}

int hda_pcm_new(struct snd_soc_component *component,
				 struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct hda_ipbloq *p_ipb_hda = snd_soc_dai_get_drvdata(cpu_dai);
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	struct hdac *hdac;
	struct hdac_bus *bus;
	size_t size;
	int ret, i;

	hdac = &p_ipb_hda->hdac;
	bus = hdac_bus(hdac);

	p_ipb_hda->hdac.card = card;

	ret = dma_coerce_mask_and_coherent(hdac->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	/* buffer pre-allocation */
	size = CONFIG_SND_HDA_PREALLOC_SIZE * 1024;
	if (size > MAX_PREALLOC_SIZE)
		size = MAX_PREALLOC_SIZE;

	for (i = 0; i <= SNDRV_PCM_STREAM_LAST; i++) {
		struct snd_pcm_substream *ss = pcm->streams[i].substream;

		ss->private_data = hdac;
	}

	snd_pcm_set_managed_buffer_all(pcm,
		SNDRV_DMA_TYPE_DEV_SG,
		hdac->dev,
		size, MAX_PREALLOC_SIZE);

	return 0;
}
EXPORT_SYMBOL_GPL(hda_pcm_new);

void hda_pcm_free(struct snd_soc_component *component,
		  struct snd_pcm *pcm)
{
	int i;

	for (i = 0; i <= SNDRV_PCM_STREAM_LAST; i++) {
		struct snd_pcm_substream *ss = pcm->streams[i].substream;
		struct hdac *hdac = ss->private_data;

		if (hdac && hdac->card)
			hdac->card = NULL;
	}

}
EXPORT_SYMBOL_GPL(hda_pcm_free);

int hda_pcm_open(struct snd_soc_component *component,
				 struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct hda_ipbloq *p_ipb_hda = snd_soc_dai_get_drvdata(cpu_dai);
	struct hdac *hdac =  &p_ipb_hda->hdac;
	struct hdac_stream *hstr;
	int err;
	int buff_step;

	mutex_lock(&hdac->open_mutex);
	hstr = hdac_assign_stream(hdac, ss);
	if (hstr == NULL) {
		err = -EBUSY;
		goto unlock;
	}

	runtime->private_data = hstr;

	snd_soc_set_runtime_hwparams(ss, &hda_pcm_hw);
	snd_pcm_limit_hw_rates(runtime);
	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	/* avoid wrap-around with wall-clock */
	snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_BUFFER_TIME,
					 20,
					 178000000);

	if (hdac->align_buffer_size)
		/* constrain buffer sizes to be multiple of 128
		 * bytes. This is more efficient in terms of memory
		 * access but isn't required by the HDA spec and
		 * prevents users from specifying exact period/buffer
		 * sizes. For example for 44.1kHz, a period size set
		 * to 20ms will be rounded to 19.59ms.
		 */
		buff_step = 128;
	else
		/* Don't enforce steps on buffer sizes, still need to
		 * be multiple of 4 bytes (HDA spec). Tested on Intel
		 * HDA controllers, may not work on all devices where
		 * option needs to be disabled
		 */
		buff_step = 4;

	snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
				   buff_step);
	snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES,
				   buff_step);

	/* disable LINK_ATIME timestamps for capture streams
	 * until we figure out how to handle digital inputs
	 */
	if (ss->stream == SNDRV_PCM_STREAM_CAPTURE) {
		runtime->hw.info &= ~SNDRV_PCM_INFO_HAS_WALL_CLOCK; /* legacy */
		runtime->hw.info &= ~SNDRV_PCM_INFO_HAS_LINK_ATIME;
	}

	snd_pcm_set_sync(ss);
	mutex_unlock(&hdac->open_mutex);

	return 0;

 unlock:
	mutex_unlock(&hdac->open_mutex);
	return err;
}
EXPORT_SYMBOL_GPL(hda_pcm_open);

int hda_pcm_close(struct snd_soc_component *component,
			   struct snd_pcm_substream *ss)
{
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct hda_ipbloq *p_ipb_hda = snd_soc_dai_get_drvdata(cpu_dai);
	struct hdac *hdac =  &p_ipb_hda->hdac;
	struct hdac_stream *hstr = get_hdac_stream(ss);

	mutex_lock(&hdac->open_mutex);
	hdac_release_stream(hstr);
	mutex_unlock(&hdac->open_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(hda_pcm_close);

int hda_pcm_hw_params(struct snd_soc_component *component,
			   struct snd_pcm_substream *ss,
			   struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct hda_ipbloq *p_ipb_hda = snd_soc_dai_get_drvdata(cpu_dai);
	struct snd_dma_buffer *dmab = snd_pcm_get_dma_buf(ss);
	struct hdac_stream *hstr = get_hdac_stream(ss);
	int ret = 0;

	if (dmab)
		hdac_pcm_stream_fixedup_remap((unsigned int *)(&dmab->addr),
					      p_ipb_hda->remap_offset);

	hstr->bufsize = 0;
	hstr->period_bytes = 0;
	hstr->format_val = 0;

	return ret;
}
EXPORT_SYMBOL_GPL(hda_pcm_hw_params);

int hda_pcm_hw_free(struct snd_soc_component *component,
					struct snd_pcm_substream *ss)
{
	struct hdac_stream *hstr = get_hdac_stream(ss);

	/* reset BDL address */
	snd_hdac_stream_cleanup(hstr);

	hstr->prepared = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(hda_pcm_hw_free);

int hda_pcm_prepare(struct snd_soc_component *component,
				struct snd_pcm_substream *ss)
{
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct hda_ipbloq *p_ipb_hda = snd_soc_dai_get_drvdata(cpu_dai);
	struct hdac *hdac =  &p_ipb_hda->hdac;
	struct hdac_stream *hstr = get_hdac_stream(ss);
	struct snd_pcm_runtime *runtime = ss->runtime;
	unsigned int format_val, stream_tag, maxbps;
	int err;

	snd_hdac_stream_reset(hstr);

	switch (runtime->format) {
	case SNDRV_PCM_FORMAT_S16_LE:
		maxbps = 16;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		maxbps = 24;
		break;
	default:
		maxbps = 32;
		break;
	}

	format_val = snd_hdac_calc_stream_format(runtime->rate,
						runtime->channels,
						runtime->format,
						maxbps,
						0);
	if (!format_val) {
		dev_err(hdac->card->dev,
			"invalid format_val, rate=%d, ch=%d, format=%d\n",
			runtime->rate, runtime->channels, runtime->format);
		err = -EINVAL;
		goto error;
	}

	err = snd_hdac_stream_set_params(hstr, format_val);
	if (err < 0)
		goto error;

	snd_hdac_stream_setup(hstr);
	stream_tag = hstr->stream_tag;
	hstr->prepared = 1;

error:
	return err;
}
EXPORT_SYMBOL_GPL(hda_pcm_prepare);

int hda_pcm_trigger(struct snd_soc_component *component,
			 struct snd_pcm_substream *ss, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct hda_ipbloq *p_ipb_hda = snd_soc_dai_get_drvdata(cpu_dai);
	struct hdac *hdac =  &p_ipb_hda->hdac;
	struct hdac_bus *bus = hdac_bus(hdac);
	struct hdac_stream *hstr = get_hdac_stream(ss);
	struct snd_pcm_substream *s;
	bool start;
	int sbits = 0;
	int sync_reg;

	sync_reg = AZX_REG_SSYNC;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_RESUME:
		start = true;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
		start = false;
		break;
	default:
		return -EINVAL;
	}

#if 0 //tmp not sync
	snd_pcm_group_for_each_entry(s, ss) {
		if (s->pcm->card != ss->pcm->card)
			continue;
		sbits |= 1 << hstr->index;
		snd_pcm_trigger_done(s, ss);
	}
#endif

	spin_lock(&bus->reg_lock);

	/* first, set SYNC bits of corresponding streams */
	snd_hdac_stream_sync_trigger(hstr, true, sbits, sync_reg);

	snd_pcm_group_for_each_entry(s, ss) {
		if (s->pcm->card != ss->pcm->card)
			continue;
		if (start)
			snd_hdac_stream_start(hstr, true);
		else
			snd_hdac_stream_stop(hstr);
	}
	spin_unlock(&bus->reg_lock);

	snd_hdac_stream_sync(hstr, start, sbits);

	spin_lock(&bus->reg_lock);
	/* reset SYNC bits */
	snd_hdac_stream_sync_trigger(hstr, false, sbits, sync_reg);
	if (start)
		snd_hdac_stream_timecounter_init(hstr, sbits);
	spin_unlock(&bus->reg_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(hda_pcm_trigger);

unsigned int azx_get_pos_posbuf(struct hdac *hdac, struct hdac_stream *hstr)
{
	return snd_hdac_stream_get_pos_posbuf(hstr);
}

static unsigned int azx_get_position(struct hdac *hdac,
									struct hdac_stream *hstr)
{
	struct snd_pcm_substream *ss = hstr->substream;
	unsigned int pos;
	int stream = ss->stream;
	int delay = 0;

	if (hdac->get_position[stream])
		pos = hdac->get_position[stream](hdac, hstr);
	else /* use the position buffer as default */
		pos = azx_get_pos_posbuf(hdac, hstr);

	pos = snd_hdac_stream_readl(hstr, SD_LPIB);

	if (pos >= hstr->bufsize)
		pos = 0;

	if (ss->runtime) {

	snd_hdac_chip_updatel(hdac_bus(hdac), INTCTL,
				  AZX_INT_CTRL_EN | AZX_INT_GLOBAL_EN,
				  AZX_INT_CTRL_EN | AZX_INT_GLOBAL_EN);



#if 0//tmp
		if (hdac->get_delay[stream])
			delay += hdac->get_delay[stream](hdac, hstr, pos);
#endif

		ss->runtime->delay = delay;
	}

	return pos;
}

snd_pcm_uframes_t hda_pcm_pointer(struct snd_soc_component *component,
					   struct snd_pcm_substream *ss)
{
	struct snd_pcm_runtime *runtime = ss->runtime;
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct hda_ipbloq *p_ipb_hda = snd_soc_dai_get_drvdata(cpu_dai);
	struct hdac *hdac =  &p_ipb_hda->hdac;
	struct hdac_stream *hstr = get_hdac_stream(ss);

	return bytes_to_frames(runtime,
				   azx_get_position(hdac, hstr));
}
EXPORT_SYMBOL_GPL(hda_pcm_pointer);

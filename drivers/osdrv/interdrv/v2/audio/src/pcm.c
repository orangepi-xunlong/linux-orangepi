
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/dirent.h>
#include <linux/syscalls.h>
#include <linux/utime.h>
#include <linux/file.h>

 #include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/syscalls.h>

#include <linux/fd.h>
#include <linux/tty.h>
#include <linux/suspend.h>
#include <linux/root_dev.h>
#include <linux/security.h>
#include <linux/delay.h>
#include <linux/genhd.h>
#include <linux/mount.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/initrd.h>
#include <linux/async.h>
#include <linux/fs_struct.h>
#include <linux/slab.h>
#include <linux/ramfs.h>
#include <linux/shmem_fs.h>

#include "asoundlib.h"

#define REMOVE_FROM_PCM_SAMPLE 0
#define PARAM_MAX SNDRV_PCM_HW_PARAM_LAST_INTERVAL

/* Logs information into a string; follows snprintf() in that
 * offset may be greater than size, and though no characters are copied
 * into string, characters are still counted into offset
 */

#define STRLOG(string, offset, size, ...) \
	do { int temp, clipoffset = offset > size ? size : offset; \
		temp = snprintf(string + clipoffset, size - clipoffset, __VA_ARGS__); \
		if (temp > 0) offset += temp; } while (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* refer to SNDRV_PCM_ACCESS_##index in sound/asound.h. */
static const char * const access_lookup[] = {
	"MMAP_INTERLEAVED",
	"MMAP_NONINTERLEAVED",
	"MMAP_COMPLEX",
	"RW_INTERLEAVED",
	"RW_NONINTERLEAVED",
};

/* refer to SNDRV_PCM_FORMAT_##index in sound/asound.h. */
static const char * const format_lookup[] = {
	/*[0] =*/ "S8",
	"U8",
	"S16_LE",
	"S16_BE",
	"U16_LE",
	"U16_BE",
	"S24_LE",
	"S24_BE",
	"U24_LE",
	"U24_BE",
	"S32_LE",
	"S32_BE",
	"U32_LE",
	"U32_BE",
	"FLOAT_LE",
	"FLOAT_BE",
	"FLOAT64_LE",
	"FLOAT64_BE",
	"IEC958_SUBFRAME_LE",
	"IEC958_SUBFRAME_BE",
	"MU_LAW",
	"A_LAW",
	"IMA_ADPCM",
	"MPEG",
	/*[24] =*/ "GSM",
	/* gap */
	[31] = "SPECIAL",
	"S24_3LE",
	"S24_3BE",
	"U24_3LE",
	"U24_3BE",
	"S20_3LE",
	"S20_3BE",
	"U20_3LE",
	"U20_3BE",
	"S18_3LE",
	"S18_3BE",
	"U18_3LE",
	/*[43] =*/ "U18_3BE",
#if REMOVE_FROM_PCM_SAMPLE
	/* recent additions, may not be present on local asound.h */
	"G723_24",
	"G723_24_1B",
	"G723_40",
	"G723_40_1B",
	"DSD_U8",
	"DSD_U16_LE",
#endif
};

/* refer to SNDRV_PCM_SUBFORMAT_##index in sound/asound.h. */
static const char * const subformat_lookup[] = {
	"STD",
};

static inline int param_is_mask(int p)
{
	return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) && (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int param_is_interval(int p)
{
	return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
		(p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static inline struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p, int n)
{
	return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
	return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static void _param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned int bit)
{
	if (bit >= SNDRV_MASK_MAX)
		return;
	if (param_is_mask(n)) {
		struct snd_mask *m = param_to_mask(p, n);

		m->bits[0] = 0;
		m->bits[1] = 0;
		m->bits[bit >> 5] |= (1 << (bit & 31));
	}
}

static void _param_set_min(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
	if (param_is_interval(n)) {
		struct snd_interval *i = param_to_interval(p, n);

		i->min = val;
	}
}


static void _param_set_int(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
	if (param_is_interval(n)) {
		struct snd_interval *i = param_to_interval(p, n);

		i->min = val;
		i->max = val;
		i->integer = 1;
	}
}

static unsigned int _param_get_int(struct snd_pcm_hw_params *p, int n)
{
	if (param_is_interval(n)) {
		struct snd_interval *i = param_to_interval(p, n);

		if (i->integer)
			return i->max;
	}
	return 0;
}

static void _param_init(struct snd_pcm_hw_params *p)
{
	int n;

	memset(p, 0, sizeof(*p));
	for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
		n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
		struct snd_mask *m = param_to_mask(p, n);

		m->bits[0] = ~0;
		m->bits[1] = ~0;
	}
	for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
		n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {

		struct snd_interval *i = param_to_interval(p, n);

		i->min = 0;
		i->max = ~0;
	}
	p->rmask = ~0U;
	p->cmask = 0;
	p->info = ~0U;
}
#if REMOVE_FROM_PCM_SAMPLE
static int oops(struct pcm *pcm, int e, const char *fmt, ...)
{

	va_list ap;
	int sz;

	va_start(ap, fmt);
	vsnprintf(pcm->error, PCM_ERROR_MAX, fmt, ap);
	va_end(ap);
	sz = strlen(pcm->error);

	if (e)
		snprintf(pcm->error + sz, PCM_ERROR_MAX - sz,
			 ": %s", strerror(e));

	return -1;
}
#endif

static unsigned int pcm_format_to_alsa(enum pcm_format format)
{
	switch (format) {
	case PCM_FORMAT_S32_LE:
		return SNDRV_PCM_FORMAT_S32_LE;
	case PCM_FORMAT_S8:
		return SNDRV_PCM_FORMAT_S8;
	case PCM_FORMAT_S24_3LE:
		return SNDRV_PCM_FORMAT_S24_3LE;
	case PCM_FORMAT_S24_LE:
		return SNDRV_PCM_FORMAT_S24_LE;
	default:
	case PCM_FORMAT_S16_LE:
		return SNDRV_PCM_FORMAT_S16_LE;
	};
}

unsigned int pcm_format_to_bits(enum pcm_format format)
{
	switch (format) {
	case PCM_FORMAT_S32_LE:
	case PCM_FORMAT_S24_LE:
		return 32;
	case PCM_FORMAT_S24_3LE:
		return 24;
	default:
	case PCM_FORMAT_S16_LE:
		return 16;
	};
}

unsigned int pcm_bytes_to_frames(struct pcm *pcm, unsigned int bytes)
{
	return bytes / (pcm->config.channels * (pcm_format_to_bits(pcm->config.format) >> 3));
}

unsigned int pcm_frames_to_bytes(struct pcm *pcm, unsigned int frames)
{
	return frames * pcm->config.channels * (pcm_format_to_bits(pcm->config.format) >> 3);
}


int pcm_is_ready(struct pcm *pcm)
{
	return pcm->fp != 0;
}

int pcm_prepare(struct pcm *pcm)
{
	if (pcm->prepared)
		return 0;

	if (vfs_ioctl(pcm->fp, SNDRV_PCM_IOCTL_PREPARE, 0) < 0) {
		pr_err("cannot prepare channel\n");
		return -1;
	}

	pcm->prepared = 1;
	return 0;
}

int pcm_start(struct pcm *pcm)
{
	int prepare_error = pcm_prepare(pcm);

	if (prepare_error)
		return prepare_error;

	if (pcm->flags & PCM_MMAP)
		return -1;

	if (vfs_ioctl(pcm->fp, SNDRV_PCM_IOCTL_START, 0) < 0) {
		pr_err("cannot start channel\n");
		return -1;
	}

	pcm->running = 1;
	return 0;
}

int pcm_stop(struct pcm *pcm)
{
	if (vfs_ioctl(pcm->fp, SNDRV_PCM_IOCTL_DROP, 0) < 0) {
		pr_err("cannot stop channel\n");
		return -1;
	}

	pcm->prepared = 0;
	pcm->running = 0;
	return 0;
}


int pcm_write(struct pcm *pcm, const void *data, unsigned int count)
{
	struct snd_xferi x;

	if (pcm->flags & PCM_IN)
		return -EINVAL;

	x.buf = (void *)data;
	x.frames = count / (pcm->config.channels * pcm_format_to_bits(pcm->config.format) / 8);

	for (;;) {
		if (!pcm->running) {
			int prepare_error = pcm_prepare(pcm);

		if (prepare_error)
			return prepare_error;
		if (vfs_ioctl(pcm->fp, SNDRV_PCM_IOCTL_WRITEI_FRAMES, (unsigned long)&x))
			return -2;
		pcm->running = 1;
		return 0;
	}
	if (vfs_ioctl(pcm->fp, SNDRV_PCM_IOCTL_WRITEI_FRAMES, (unsigned long)&x)) {
		pcm->prepared = 0;
		pcm->running = 0;
		#if REMOVE_FROM_PCM_SAMPLE
		if (errno == EPIPE) {
			/* we failed to make our window -- try to restart if we are
			 * allowed to do so.  Otherwise, simply allow the EPIPE error to
			 * propagate up to the app level
			 */
			pcm->underruns++;
			if (pcm->flags & PCM_NORESTART)
				return -EPIPE;
			continue;
		}
		#endif
		return -10;
	}
	return 0;
	}
}
EXPORT_SYMBOL(pcm_write);


int pcm_read(struct pcm *pcm, void *data, unsigned int count)
{
	struct snd_xferi x;

	if (!(pcm->flags & PCM_IN))
		return -EINVAL;

	x.buf = data;
	x.frames = count / (pcm->config.channels *
			pcm_format_to_bits(pcm->config.format) / 8);

	for (;;) {
		if (!pcm->running) {
			if (pcm_start(pcm) < 0) {
				pr_err("%s:%d\n", __func__, __LINE__);
				return -3;
			}
		}

		if (vfs_ioctl(pcm->fp, SNDRV_PCM_IOCTL_READI_FRAMES, (unsigned long)&x)) {
			pcm->prepared = 0;
			pcm->running = 0;


			//if (errno == EPIPE) {
				/* we failed to make our window -- try to restart */
				pcm->underruns++;
				usleep_range(1000 * 10, 1000 * 20);
				if (pcm->underruns > 1000) {
					pr_err("%s:%d underrun over 1000\n", __func__, __LINE__);
					pcm->underruns = 0;
					return -4;
				}
				continue;
			//}

			//return -4;
		}
		pcm->underruns = 0;
		return 0;
	}
}
EXPORT_SYMBOL(pcm_read);

static struct pcm bad_pcm = {
	.fd = 0,
	.fp = NULL,
};


int pcm_close(struct pcm *pcm)
{
	if (pcm == &bad_pcm)
		return 0;

	if (pcm->fp != 0)
		filp_close(pcm->fp, 0);

	pcm->prepared = 0;
	pcm->running = 0;
	pcm->buffer_size = 0;
	pcm->fd = -1;
	pcm->fp = NULL;
	kfree(pcm);
	return 0;
}
EXPORT_SYMBOL(pcm_close);
char fn[128];
struct pcm *pcm_open(unsigned int card, unsigned int device,
			unsigned int flags, struct pcm_config *config)
{
	struct pcm *pcm;
	struct snd_pcm_info info;
	struct snd_pcm_hw_params params;
	struct snd_pcm_sw_params sparams;

	//int rc;
	//char *fn = kzalloc(sizeof(char) * 256, GFP_KERNEL);
	if (!config) {
		return &bad_pcm; /* TODO: could support default config here */
	}
	pcm = kzalloc(sizeof(struct pcm), GFP_KERNEL);
	if (!pcm)
		return &bad_pcm; /* TODO: could support default config here */

	pcm->config = *config;

	snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", card, device,
		flags & PCM_IN ? 'c' : 'p');
	//snprintf(fn, sizeof(char) * 256, "/dev/snd/pcmC%uD%u%c", card, device,
	//	flags & PCM_IN ? 'c' : 'p');

	pcm->flags = flags;
	pcm->fp = filp_open(fn, O_RDWR/*|O_NONBLOCK*/, 0644);
	if (!pcm->fp) {
		pr_err("cannot open device '%s'", fn);
		//kfree(fn);
		return pcm;
	} else
		pr_err("open device '%s'", fn);
	//pr_err("cannot get info\n");
#if REMOVE_FROM_PCM_SAMPLE
	if (fcntl(pcm->fd, F_SETFL, fcntl(pcm->fd, F_GETFL) & ~O_NONBLOCK) < 0) {
		oops(pcm, errno, "failed to reset blocking mode '%s'", fn);
		goto fail_close;
	}
#endif
	//kfree(fn);

	if (vfs_ioctl(pcm->fp, SNDRV_PCM_IOCTL_INFO, (unsigned long)&info)) {
		pr_err("cannot get info[%s][%d]\n", __func__, __LINE__);
		goto fail_close;
	}
	pcm->subdevice = info.subdevice;
	_param_init(&params);
	_param_set_mask(&params, SNDRV_PCM_HW_PARAM_FORMAT,
				pcm_format_to_alsa(config->format));
	_param_set_mask(&params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
				SNDRV_PCM_SUBFORMAT_STD);
	_param_set_min(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, config->period_size);
	_param_set_int(&params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
				pcm_format_to_bits(config->format));
	_param_set_int(&params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
				pcm_format_to_bits(config->format) * config->channels);
	_param_set_int(&params, SNDRV_PCM_HW_PARAM_CHANNELS,
				config->channels);
	_param_set_int(&params, SNDRV_PCM_HW_PARAM_PERIODS, config->period_count);
	_param_set_int(&params, SNDRV_PCM_HW_PARAM_RATE, config->rate);


	_param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
				SNDRV_PCM_ACCESS_RW_INTERLEAVED);

	if (vfs_ioctl(pcm->fp, SNDRV_PCM_IOCTL_HW_PARAMS, (unsigned long)&params)) {
		pr_err("cannot set hw params");
		goto fail_close;
	}

	/* get our refined hw_params */
	config->period_size = _param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
	config->period_count = _param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIODS);
	pcm->buffer_size = config->period_count * config->period_size;

	memset(&sparams, 0, sizeof(sparams));
	sparams.tstamp_mode = SNDRV_PCM_TSTAMP_ENABLE;
	sparams.period_step = 1;

	if (!config->start_threshold) {
		if (pcm->flags & PCM_IN)
			pcm->config.start_threshold = sparams.start_threshold = 1;
		else {
			pcm->config.start_threshold = sparams.start_threshold =
			config->period_count * config->period_size / 2;
		}
	} else
		sparams.start_threshold = config->start_threshold;

	/* pick a high stop threshold - todo: does this need further tuning */
	if (!config->stop_threshold) {
		if (pcm->flags & PCM_IN) {
			pcm->config.stop_threshold =
				sparams.stop_threshold =
				config->period_count * config->period_size * 10;
		} else {
			pcm->config.stop_threshold =
				sparams.stop_threshold =
				config->period_count * config->period_size;
		}
	} else
		sparams.stop_threshold = config->stop_threshold;

	if (!pcm->config.avail_min) {
		if (pcm->flags & PCM_MMAP)
			pcm->config.avail_min = sparams.avail_min = pcm->config.period_size;
		else
			pcm->config.avail_min = sparams.avail_min = 1;
	} else
		sparams.avail_min = config->avail_min;

	sparams.xfer_align = config->period_size / 2; /* needed for old kernels */
	sparams.silence_threshold = config->silence_threshold;
	sparams.silence_size = config->silence_size;
	pcm->boundary = sparams.boundary = pcm->buffer_size;

	while (pcm->boundary * 2 <= INT_MAX - pcm->buffer_size)
		pcm->boundary *= 2;

	if (vfs_ioctl(pcm->fp, SNDRV_PCM_IOCTL_SW_PARAMS, (unsigned long)&sparams)) {
		pr_err("cannot set sw params");
		goto fail;
	}

	pcm->underruns = 0;
	return pcm;

fail:
fail_close:
	pr_err("[error][%s][%d]\n", __func__, __LINE__);
	if (pcm->fp)
		filp_close(pcm->fp, 0);
	pcm->fd = -1;
	pcm->fp = NULL;
	return pcm;
}
EXPORT_SYMBOL(pcm_open);


// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 KY
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <linux/genalloc.h>

#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/ktime.h>
#include <linux/rtc.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/processor.h>
#include <asm/page.h>
#include <linux/wait.h>
#include <linux/debugfs.h>
#include <linux/namei.h>

#define KY_DBG_BUFF_SIZE (128*1024)
struct ky_dbg_buf {
	char *pStart;
	char *pEnd;
	char *pRdPtr;
	char *pWrPtr;
	int uBufSize;
	int uDataSize;
	int type;
	int mode;
	int malloc_flag;
	wait_queue_head_t data;
	struct mutex lock;
	struct task_struct *daemon;
	struct snd_pcm_runtime *runtime;
};
struct dentry *g_debug_file;
static char dump_file_dir[64] = "/tmp";
static unsigned int dump_id = 0;

#define KY_CODEC_TYPE 0
#define KY_HDMI_TYPE  1

#define KY_PLAY_MODE  0
#define KY_CAPT_MODE  1

#define OUTPUT_BUFFER_SIZE 256
#endif

#define DRV_NAME "ky-snd-dma"

#define I2S_PERIOD_SIZE          1024
#define I2S_PERIOD_COUNT         4
#define I2S0_REG_BASE            0xD4026000
#define I2S1_REG_BASE            0xD4026800
#define DATAR                    0x10    /* SSP Data Register */

#define DMA_I2S0 0
#define DMA_I2S1 1
#define DMA_HDMI 2

#define HDMI_REFORMAT_ENABLE
#define I2S_HDMI_REG_BASE        0xC0883900
#define HDMI_TXDATA              0x80
#define HDMI_PERIOD_SIZE         480

#define SAMPLE_PRESENT_FLAG_OFFSET      31
#define AUDIO_FRAME_START_BIT_OFFSET    30
#define PARITY_BIT_OFFSET               27
#define CHANNEL_STATUS_OFFSET           26
#define VALID_OFFSET                    24

#define CS_CTRL1 ((1 << SAMPLE_PRESENT_FLAG_OFFSET) | (1 << AUDIO_FRAME_START_BIT_OFFSET))
#define CS_CTRL2 ((1 << SAMPLE_PRESENT_FLAG_OFFSET) | (0 << AUDIO_FRAME_START_BIT_OFFSET))

#define CS_SAMPLING_FREQUENCY           25
#define CS_MAX_SAMPLE_WORD              32

#define P2(n) n, n^1, n^1, n
#define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
#define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)

struct ky_snd_dmadata {
	struct dma_chan *dma_chan;
	unsigned int dma_id;
	dma_cookie_t cookie;
	spinlock_t dma_lock;
	void *private_data;

	int stream;
	/*DOMAIN:config from userspace*/
	struct snd_pcm_substream *substream;
	unsigned long pos;
	bool playback_data;

#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
	int play_flag;
	int capt_flag;
	int debug_flag;
	struct ky_dbg_buf play_buf;
	struct ky_dbg_buf capt_buf;
#endif
};

struct ky_snd_soc_device {
	struct ky_snd_dmadata dmadata[2];
	unsigned long pos;
	bool playback_en;
	bool capture_en;
	spinlock_t lock;
};

struct hdmi_priv {
	dma_addr_t phy_addr;
	void __iomem	*buf_base;
};

/* HDMI initalization data */
struct hdmi_codec_priv {
	uint32_t srate;
	uint32_t channels;
	uint8_t iec_offset;
};

static struct hdmi_codec_priv hdmi_ptr = {0};
static const bool ParityTable256[256] =
{
	P6(0), P6(1), P6(1), P6(0)
};
static struct hdmi_priv priv;
static dma_addr_t hdmiraw_dma_addr;
static dma_addr_t hdmipcm_dma_addr;
static unsigned char *hdmiraw_dma_area;	/* DMA area */
#ifdef HDMI_REFORMAT_ENABLE
static unsigned char *hdmiraw_dma_area_tmp;
#endif
#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
#ifdef CONFIG_ADD_WAV_HEADER
typedef struct {
	char riffType[4];
	char wavType[4];
	char formatType[4];
	char dataType[4];
	unsigned short audioFormat;
	unsigned short numChannels;
	unsigned short blockAlign;
	unsigned short bitsPerSample;
	unsigned int formatSize;
	unsigned int sampleRate;
	unsigned int bytesPerSecond;
	unsigned int riffSize;
	unsigned int dataSize;
} head_data_t;
#endif

#if defined CONFIG_KY_PLAY_DEBUG || defined CONFIG_KY_CAPT_DEBUG
static struct file *try_to_create_pcm_file(char *type, int dma_id)
{
	char fname[128];
	struct timespec64 ts;
	struct rtc_time tm;
	struct file *filep = NULL;

	ktime_get_real_ts64(&ts);
	if (ts.tv_nsec)
		ts.tv_sec++;
	ts.tv_sec+=8*60*60;
	rtc_time64_to_tm(ts.tv_sec, &tm);

	//create file
	memset(fname, 0, sizeof(fname));
	strcat(fname, dump_file_dir);
	if(dma_id < DMA_HDMI) {
		sprintf(fname+strlen(fname), "/dump-i2s%d-%s-%d-%02d-%02d-%02d-%02d-%02d.pcm", dma_id, type,
			tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	} else {
		sprintf(fname+strlen(fname), "/dump-hdmi-%s-%d-%02d-%02d-%02d-%02d-%02d.pcm", type,
			tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
	filep = filp_open(fname, O_RDWR|O_CREAT, 0666);
	if (IS_ERR(filep)) {
			pr_err("filp_open %s error\n", fname);
			return NULL;
	}
	return filep;
}

#ifdef CONFIG_ADD_WAV_HEADER
static int pcm_add_wave_header(struct file *fp, unsigned int channels,
					unsigned int bits, unsigned int sample_rate,
					unsigned int len)
{
	loff_t wav_pos = 0;
	ssize_t ret;
	head_data_t wav_header;
	if (NULL == fp) {
		pr_err("Input file ptr is null:%s\n", __func__);
		return -1;
	}
	memcpy(wav_header.riffType, "RIFF", strlen("RIFF"));
	wav_header.riffSize = len - 8;
	memcpy(wav_header.wavType, "WAVE", strlen("WAVE"));
	memcpy(wav_header.formatType, "fmt ", strlen("fmt "));
	wav_header.formatSize = 16;
	wav_header.audioFormat = 1;
	wav_header.numChannels = channels;
	wav_header.sampleRate = sample_rate;
	wav_header.blockAlign = channels * bits / 8;
	wav_header.bitsPerSample = bits;
	wav_header.bytesPerSecond = wav_header.sampleRate * wav_header.blockAlign;
	memcpy(wav_header.dataType, "data", strlen("data"));
	wav_header.dataSize = len - 44;
	ret = kernel_write(fp, &wav_header, 44, &wav_pos);
	if (ret > 0) {
		printk("add wav header size:%ld", ret);
	} else {
		pr_err("kernel_write error:%s,line: %d\n", __func__, __LINE__);
		return -1;
	}
	return 0;
}
#endif

static int ky_debug_data_daemon(void *data)
{
	loff_t pos = 0;
	int ret;
	char *wr_ptr;
	struct file *dump_filep = NULL;
	struct ky_dbg_buf *dbgPara = (struct ky_dbg_buf *)data;
	struct ky_snd_dmadata *dmadata = dbgPara->runtime->private_data;

	dump_filep = try_to_create_pcm_file(dbgPara->mode == 0 ? "play":"capt", dmadata->dma_id);
	if (IS_ERR(dump_filep)) {
		pr_err("ERR: %s try to create pcm file failed!\n", __func__);
		goto thread_out;
	}

	/* init the buffer parameter */
	mutex_lock(&dbgPara->lock);
	dbgPara->uBufSize = KY_DBG_BUFF_SIZE;
	dbgPara->pRdPtr = dbgPara->pStart;
	dbgPara->pWrPtr = dbgPara->pStart;
	printk("dbgPara->pStart :%p", dbgPara->pStart);
	dbgPara ->uDataSize = 0;
	dbgPara->pEnd = dbgPara->pStart + KY_DBG_BUFF_SIZE;
	mutex_unlock(&dbgPara->lock);

	#ifdef CONFIG_ADD_WAV_HEADER
	pos = 44;	//wave header offset
	#endif

	for(;;){
		if (kthread_should_stop()) break;
		while(dbgPara->uDataSize) {
			wr_ptr = dbgPara->pWrPtr;
			if (dbgPara->pRdPtr > wr_ptr) {
				/* write the tail */
				kernel_write(dump_filep, dbgPara->pRdPtr, dbgPara->pEnd - dbgPara->pRdPtr, &pos);
				mutex_lock(&dbgPara->lock);
				dbgPara->uDataSize -= dbgPara->pEnd - dbgPara->pRdPtr;
				if (dbgPara->uDataSize<0)
					dbgPara->uDataSize = 0;
				dbgPara->pRdPtr = dbgPara->pStart;
				mutex_unlock(&dbgPara->lock);
			} else {
				/* write the head */
				kernel_write(dump_filep, dbgPara->pRdPtr, wr_ptr - dbgPara->pRdPtr, &pos);
				mutex_lock(&dbgPara->lock);
				dbgPara->uDataSize -= wr_ptr - dbgPara->pRdPtr;
				if (dbgPara->uDataSize<0)
					dbgPara->uDataSize = 0;
				dbgPara->pRdPtr = (wr_ptr>=dbgPara->pEnd)? dbgPara->pStart : wr_ptr;
				mutex_unlock(&dbgPara->lock);
			}
		}

		ret = wait_event_interruptible(dbgPara->data, (dbgPara->uDataSize > 0) || kthread_should_stop());
		if (ret < 0) break;
	}

thread_out:
	pr_info("thread_out\n");
	pr_info("dump file size:%llu", pos);

#ifdef CONFIG_ADD_WAV_HEADER
	pcm_add_wave_header(dump_filep, dbgPara->runtime->channels,
				dbgPara->runtime->sample_bits,
				dbgPara->runtime->rate, pos);
#endif

	if (dump_filep) {
		filp_close(dump_filep, NULL);
	}
	if (dbgPara->pStart) {
		vfree(dbgPara->pStart);
		dbgPara->pStart = 0;
	}
	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
}

static void ky_debug_data(char *hwbuf, struct ky_dbg_buf *debug_buf,
				int *debug_flag, int *mode_flag,
				struct snd_pcm_runtime *runtime,
				unsigned long bytes, int type, int mode,
				int malloc_flag)
{
	char thread_name[24];
	struct ky_snd_dmadata *dmadata = runtime->private_data;

	if (*debug_flag && !(*mode_flag)) {
		/* start task */
		memset(debug_buf, 0, sizeof(*debug_buf));
		debug_buf->type = type;
		debug_buf->mode = mode;
		debug_buf->runtime = runtime;
		debug_buf->malloc_flag = malloc_flag;
		init_waitqueue_head(&debug_buf->data);
		mutex_init(&debug_buf->lock);
		if ((type == KY_CODEC_TYPE) && (mode == KY_PLAY_MODE)) {
			snprintf(thread_name, sizeof(thread_name), "i2s%d-play-dbg", dmadata->dma_id);
		} else if ((type == KY_CODEC_TYPE) && (mode == KY_CAPT_MODE)) {
			snprintf(thread_name, sizeof(thread_name), "i2s%d-capt-dbg", dmadata->dma_id);
		} else if ((type == KY_HDMI_TYPE) && (mode == KY_PLAY_MODE)) {
			snprintf(thread_name, sizeof(thread_name), "hdmi-play-dbg");
		}
		if (thread_name[0] != '\0') {
			debug_buf->daemon = kthread_run(ky_debug_data_daemon, debug_buf, thread_name);
		}
		*mode_flag = 1;
	} else if (!(*debug_flag) && *mode_flag) {
		/* stop task */
		kthread_stop(debug_buf->daemon);
		memset(debug_buf, 0, sizeof(*debug_buf));
		*mode_flag = 0;
	}
	if (!debug_buf->malloc_flag) {
		debug_buf->pStart = vmalloc(KY_DBG_BUFF_SIZE);
		if (!debug_buf->pStart) {
			pr_err("ERR: %s try to vmalloc %d bytes failed!\n", __func__, KY_DBG_BUFF_SIZE);
		} else {
			pr_info("try to vmalloc buffer success\n");
		}
		debug_buf->malloc_flag = 1;
	}
	if (*debug_flag && *mode_flag && debug_buf->pStart && debug_buf->uBufSize) {
		size_t size = bytes;
		if (size > (debug_buf->uBufSize - debug_buf->uDataSize)) {
			size = debug_buf->uBufSize - debug_buf->uDataSize;
		}

		if (size <= (debug_buf->pEnd - debug_buf->pWrPtr)) {
			memcpy(debug_buf->pWrPtr, hwbuf, size);
			mutex_lock(&debug_buf->lock);
			debug_buf->pWrPtr += size;
			debug_buf->uDataSize += size;
			if (debug_buf->pWrPtr == debug_buf->pEnd)
				debug_buf->pWrPtr = debug_buf->pStart;
			mutex_unlock(&debug_buf->lock);
		} else {
			memcpy(debug_buf->pWrPtr, hwbuf, debug_buf->pEnd - debug_buf->pWrPtr);
			memcpy(debug_buf->pStart, hwbuf+(debug_buf->pEnd - debug_buf->pWrPtr), size-(debug_buf->pEnd - debug_buf->pWrPtr));
			mutex_lock(&debug_buf->lock);
			debug_buf->pWrPtr = debug_buf->pStart + size - (debug_buf->pEnd - debug_buf->pWrPtr);
			debug_buf->uDataSize += size;
			mutex_unlock(&debug_buf->lock);
		}
		/* wakeup the daemon */
		wake_up(&debug_buf->data);
	}
}
#endif
#endif
static int ky_snd_dma_init(struct device *paraent, struct ky_snd_soc_device *dev,
			     struct ky_snd_dmadata *dmadata);

static const struct snd_pcm_hardware ky_snd_pcm_hardware = {
	.info		  = SNDRV_PCM_INFO_INTERLEAVED |
			    SNDRV_PCM_INFO_BATCH,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.rates            = SNDRV_PCM_RATE_48000,
	.rate_min         = SNDRV_PCM_RATE_48000,
	.rate_max         = SNDRV_PCM_RATE_48000,
	.channels_min     = 2,
	.channels_max     = 2,
	.buffer_bytes_max = I2S_PERIOD_SIZE * I2S_PERIOD_COUNT * 4,
	.period_bytes_min = I2S_PERIOD_SIZE * 4,
	.period_bytes_max = I2S_PERIOD_SIZE * 4,
	.periods_min	  = I2S_PERIOD_COUNT,
	.periods_max	  = I2S_PERIOD_COUNT,
};

static const struct snd_pcm_hardware ky_snd_pcm_hardware_hdmi = {
	.info		  = SNDRV_PCM_INFO_INTERLEAVED |
			    SNDRV_PCM_INFO_BATCH,
	.formats	  = SNDRV_PCM_FMTBIT_S16_LE,
	.rates		  = SNDRV_PCM_RATE_48000,
	.rate_min	  = SNDRV_PCM_RATE_48000,
	.rate_max	  = SNDRV_PCM_RATE_48000,
	.channels_min	  = 2,
	.channels_max	  = 2,
	.buffer_bytes_max = HDMI_PERIOD_SIZE * 4 * 4,
	.period_bytes_min = HDMI_PERIOD_SIZE * 4,
	.period_bytes_max = HDMI_PERIOD_SIZE * 4,
	.periods_min	  = 4,
	.periods_max	  = 4,
};
static int ky_dma_slave_config(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct dma_slave_config *slave_config,
		int dma_id)
{
	int ret = snd_hwparams_to_dma_slave_config(substream, params, slave_config);
	if (ret)
		return ret;

	slave_config->dst_maxburst = 32;
	slave_config->src_maxburst = 32;

	slave_config->src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	slave_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	/* for tmda , don't need to config dma controller addr, set to 0*/
	if (dma_id == DMA_I2S0) {
		pr_debug("i2s0_datar\n");
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			slave_config->dst_addr = I2S0_REG_BASE + DATAR;
			slave_config->src_addr = 0;
		}
		else {
			slave_config->src_addr = I2S0_REG_BASE + DATAR;
			slave_config->dst_addr = 0;
		}
	} else if (dma_id == DMA_I2S1) {
		pr_debug("i2s1_datar\n");
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			slave_config->dst_addr = I2S1_REG_BASE + DATAR;
			slave_config->src_addr = 0;
		}
		else {
			slave_config->src_addr = I2S1_REG_BASE + DATAR;
			slave_config->dst_addr = 0;
		}
	} else if (dma_id == DMA_HDMI) {
		pr_debug("i2s_hdmi_datar\n");
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			slave_config->dst_addr = I2S_HDMI_REG_BASE + HDMI_TXDATA;
			slave_config->src_addr = 0;
			#ifdef HDMI_REFORMAT_ENABLE
			slave_config->src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			slave_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			#else
			slave_config->src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
			slave_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
			#endif
		}
		else {
			slave_config->src_addr = I2S_HDMI_REG_BASE + 0x00;
			slave_config->dst_addr = 0;
		}
	}else {
		pr_err("unsupport dma platform\n");
		return -1;
	}

	pr_debug("leave %s\n", __FUNCTION__);

	return 0;
}

static bool ky_get_stream_is_enable(struct ky_snd_soc_device *dev, int stream)
{
	bool ret;
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = dev->playback_en;
	else
		ret = dev->capture_en;
	return ret;
}

static void ky_update_stream_status(struct ky_snd_soc_device *dev, int stream, bool enable)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		dev->playback_en = enable;
	else
		dev->capture_en = enable;
	return;
}

static int ky_snd_pcm_hw_params(struct snd_soc_component *component, struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *params)
{
	int ret;
	struct dma_slave_config slave_config;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ky_snd_dmadata *dmadata = runtime->private_data;

	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	struct snd_pcm *pcm = rtd->pcm;
	struct snd_pcm_substream *substream_tx;

	struct ky_snd_soc_device *dev = snd_soc_component_get_drvdata(component);
	struct ky_snd_dmadata *txdma = &dev->dmadata[0];
	struct dma_slave_config slave_config_tx;

	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	pr_debug("enter %s!! allocbytes=%d, dmadata=0x%lx\n",
		__FUNCTION__, params_buffer_bytes(params), (unsigned long)dmadata);

	if (dmadata->stream == SNDRV_PCM_STREAM_PLAYBACK
			&& ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_CAPTURE)) {
		ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
		if (ret < 0)
			goto unlock;
	}
	memset(&slave_config, 0, sizeof(slave_config));
	memset(&slave_config_tx, 0, sizeof(slave_config_tx));

	if (dmadata->dma_id != DMA_I2S0 && dmadata->dma_id != DMA_I2S1) {
		pr_err("unsupport dma platform\n");
		ret = -EINVAL;
		goto unlock;
	}

	if (dmadata->stream == SNDRV_PCM_STREAM_CAPTURE
			&& !ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_PLAYBACK)) {
		substream_tx = pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;
		dmadata = txdma;
		ky_dma_slave_config(substream_tx, params, &slave_config_tx, dmadata->dma_id);
		slave_config_tx.direction = DMA_MEM_TO_DEV;
		slave_config_tx.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config_tx.dst_addr = I2S0_REG_BASE + DATAR;
		slave_config_tx.src_addr = 0;
		ret = dmaengine_slave_config(dmadata->dma_chan, &slave_config_tx);
		if (ret)
			goto unlock;
		dmadata->substream = substream_tx;
		dmadata->pos = 0;
	}

	dmadata = runtime->private_data;
	ky_dma_slave_config(substream, params, &slave_config, dmadata->dma_id);

	ret = dmaengine_slave_config(dmadata->dma_chan, &slave_config);
	if (ret)
		goto unlock;

	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (ret < 0)
		goto unlock;

	dmadata->substream = substream;
	dmadata->pos = 0;

	pr_debug("leave %s!!\n", __FUNCTION__);
unlock:
	spin_unlock_irqrestore(&dev->lock, flags);
	if (ret < 0)
		return ret;
	return 0;
}

static int ky_snd_pcm_hw_free(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	pr_debug("enter %s!!\n", __FUNCTION__);
	return snd_pcm_lib_free_pages(substream);
}

static int ky_snd_pcm_hdmi_hw_params(struct snd_soc_component *component, struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *params)
{
	//config hdmi and callback
	int ret;
	struct dma_slave_config slave_config;

	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ky_snd_dmadata *dmadata = runtime->private_data;

	pr_debug("enter %s!! allocbytes=%d, dmadata=0x%lx\n",
		__FUNCTION__, params_buffer_bytes(params), (unsigned long)dmadata);

	memset(&slave_config, 0, sizeof(slave_config));
	if (dmadata->dma_id != DMA_HDMI) {
		pr_err("unsupport adma platform\n");
		return -1;
	}
	ky_dma_slave_config(substream, params, &slave_config, dmadata->dma_id);

	ret = dmaengine_slave_config(dmadata->dma_chan, &slave_config);
	if (ret)
		return ret;

	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (ret < 0)
		return ret;

	hdmiraw_dma_area = (void *)priv.buf_base;
	hdmiraw_dma_addr = (dma_addr_t)priv.phy_addr;

	if (hdmiraw_dma_area == NULL) {
		pr_err("hdmi:raw:get mem failed...\n");
		return -ENOMEM;
	}
#ifdef HDMI_REFORMAT_ENABLE
	hdmiraw_dma_area_tmp = kzalloc((params_buffer_bytes(params) * 2), GFP_KERNEL);
	if (hdmiraw_dma_area_tmp == NULL) {
		pr_err("hdmi:raw:get mem tmp failed...\n");
		return -ENOMEM;
	}
#endif

	hdmipcm_dma_addr = substream->dma_buffer.addr;
	substream->dma_buffer.addr = (dma_addr_t)hdmiraw_dma_addr;

	dmadata->substream = substream;
	dmadata->pos = 0;
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	pr_debug("leave %s!!\n", __FUNCTION__);
	return 0;
}

static int ky_snd_pcm_hdmi_hw_free(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	pr_debug("enter %s!!\n", __FUNCTION__);

	substream->dma_buffer.addr = hdmipcm_dma_addr;
	hdmiraw_dma_area = NULL;
#ifdef HDMI_REFORMAT_ENABLE
	kfree(hdmiraw_dma_area_tmp);
#endif
	return snd_pcm_lib_free_pages(substream);
}

static void ky_snd_hdmi_dma_complete(void *arg)
{
	struct ky_snd_dmadata *dmadata = arg;
	struct snd_pcm_substream *substream = dmadata->substream;
	unsigned int pos = dmadata->pos;
	struct ky_snd_soc_device *dev = (struct ky_snd_soc_device *)dmadata->private_data;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);

	pos += snd_pcm_lib_period_bytes(substream);
	pos %= snd_pcm_lib_buffer_bytes(substream);
	dmadata->pos = pos;

	spin_unlock_irqrestore(&dev->lock, flags);
	if (snd_pcm_running(substream)) {
		snd_pcm_period_elapsed(substream);
	}
	return;
}

static void ky_snd_dma_complete(void *arg)
{
	struct ky_snd_dmadata *dmadata = arg;
	struct snd_pcm_substream *substream = dmadata->substream;
	struct ky_snd_soc_device *dev = (struct ky_snd_soc_device *)dmadata->private_data;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);

	if (dmadata->stream == SNDRV_PCM_STREAM_PLAYBACK
		&& ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_CAPTURE)
		&& !ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_PLAYBACK)) {
			spin_unlock_irqrestore(&dev->lock, flags);
			return;
	}

	spin_unlock_irqrestore(&dev->lock, flags);
	if (snd_pcm_running(substream)) {
		snd_pcm_period_elapsed(substream);
	}
	return;
}

static int ky_snd_dma_submit(struct ky_snd_dmadata *dmadata)
{
	struct dma_async_tx_descriptor *desc;
	struct snd_pcm_substream *substream = dmadata->substream;
	struct dma_chan *chan = dmadata->dma_chan;
	unsigned long flags = DMA_CTRL_ACK;

	pr_debug("enter %s!!\n", __FUNCTION__);

	if (substream->runtime && !substream->runtime->no_period_wakeup)
		flags |= DMA_PREP_INTERRUPT;

#ifdef HDMI_REFORMAT_ENABLE
	if (dmadata->dma_id == DMA_HDMI){
		desc = dmaengine_prep_dma_cyclic(chan,
			substream->runtime->dma_addr,
			snd_pcm_lib_buffer_bytes(substream) * 2,
			snd_pcm_lib_period_bytes(substream) * 2,
			snd_pcm_substream_to_dma_direction(substream),
			flags);
	} else
#endif
	{
		if (dmadata->stream == SNDRV_PCM_STREAM_PLAYBACK){
			desc = dmaengine_prep_dma_cyclic(chan,
				substream->dma_buffer.addr,
				I2S_PERIOD_SIZE * I2S_PERIOD_COUNT * 4,
				I2S_PERIOD_SIZE * 4,
				snd_pcm_substream_to_dma_direction(substream),
				flags);
		}
		else {
			desc = dmaengine_prep_dma_cyclic(chan,
			substream->runtime->dma_addr,
			snd_pcm_lib_buffer_bytes(substream),
			snd_pcm_lib_period_bytes(substream),
			snd_pcm_substream_to_dma_direction(substream),
			flags);
		}
	}

	if (!desc)
		return -ENOMEM;

#ifdef HDMI_REFORMAT_ENABLE
	if (dmadata->dma_id == DMA_HDMI){
		desc->callback = ky_snd_hdmi_dma_complete;
	} else
#endif
	{
		desc->callback = ky_snd_dma_complete;
	}

	desc->callback_param = dmadata;
	dmadata->cookie = dmaengine_submit(desc);

	return 0;
}

static int ky_snd_pcm_trigger(struct snd_soc_component *component, struct snd_pcm_substream *substream, int cmd)
{
	struct ky_snd_dmadata *dmadata = substream->runtime->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ky_snd_soc_device *dev = snd_soc_component_get_drvdata(component);

	int ret = 0;
	struct ky_snd_dmadata *txdma = &dev->dmadata[0];
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);

	pr_debug("pcm_trigger: cmd=%d,dma_id=%d,dir=%d,p=%d,c=%d\n", cmd, dmadata->dma_id, substream->stream,
		dev->playback_en, dev->capture_en);
	if (dmadata->stream == SNDRV_PCM_STREAM_CAPTURE
		&& !ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_PLAYBACK)) {
		dmadata = txdma;
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
			memset(dmadata->substream->dma_buffer.area, 0, I2S_PERIOD_SIZE * I2S_PERIOD_COUNT * 4);
			ret = ky_snd_dma_submit(dmadata);
			if (ret < 0)
				goto unlock;
			dma_async_issue_pending(dmadata->dma_chan);
			dmadata->pos = 0;

			break;
		case SNDRV_PCM_TRIGGER_STOP:
			dmaengine_terminate_async(dmadata->dma_chan);
			dmadata->pos = 0;
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			dmaengine_pause(dmadata->dma_chan);
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
			if (runtime->info & SNDRV_PCM_INFO_PAUSE)
				dmaengine_pause(dmadata->dma_chan);
			else
				dmaengine_terminate_async(dmadata->dma_chan);
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		case SNDRV_PCM_TRIGGER_RESUME:
			dmaengine_resume(dmadata->dma_chan);
			break;
		default:
			ret = -EINVAL;
			goto unlock;
		}
		dmadata = substream->runtime->private_data;
	}

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (dmadata->stream == SNDRV_PCM_STREAM_PLAYBACK
			&& ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_CAPTURE)) {
				dmadata->playback_data = 0;
				dmadata->pos = 0;

		} else {
			ret = ky_snd_dma_submit(dmadata);
			if (ret < 0)
				goto unlock;
			dma_async_issue_pending(dmadata->dma_chan);
			dmadata->playback_data = 0;
			dmadata->pos = 0;
		}
		ky_update_stream_status(dev, dmadata->stream, true);

		break;
	case SNDRV_PCM_TRIGGER_STOP:
		if (dmadata->stream == SNDRV_PCM_STREAM_PLAYBACK
			&& ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_CAPTURE)) {
				dmadata->playback_data = 0;
				dmadata->pos = 0;

		} else {
			dmaengine_terminate_async(dmadata->dma_chan);
			dmadata->playback_data = 0;
			dmadata->pos = 0;
		}
		ky_update_stream_status(dev, dmadata->stream, false);

		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		dmaengine_pause(dmadata->dma_chan);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (runtime->info & SNDRV_PCM_INFO_PAUSE)
			dmaengine_pause(dmadata->dma_chan);
		else {
			dmaengine_terminate_async(dmadata->dma_chan);
			dmadata->playback_data = 0;
			dmadata->pos = 0;
			ky_update_stream_status(dev, dmadata->stream, false);
		}
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_RESUME:
		dmaengine_resume(dmadata->dma_chan);
		break;
	default:
		ret = -EINVAL;
	}
unlock:
	spin_unlock_irqrestore(&dev->lock, flags);
	return ret;
}

snd_pcm_uframes_t
ky_snd_pcm_pointer(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct ky_snd_dmadata *dmadata = substream->runtime->private_data;
	struct ky_snd_soc_device *dev = snd_soc_component_get_drvdata(component);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dma_tx_state state;
	enum dma_status status;
	unsigned int buf_size;
	unsigned int preriod_size;
	unsigned int pos = 0;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	status = dmaengine_tx_status(dmadata->dma_chan, dmadata->cookie, &state);
	if (status == DMA_IN_PROGRESS || status == DMA_PAUSED) {
		buf_size = I2S_PERIOD_SIZE * I2S_PERIOD_COUNT * 4;
		preriod_size = I2S_PERIOD_SIZE * 4;
		if (state.residue > 0 && state.residue <= buf_size) {
			pos = ((buf_size - state.residue) / preriod_size) * preriod_size;
		}
		runtime->delay = bytes_to_frames(runtime, state.in_flight_bytes);
	}
	if (dmadata->stream == SNDRV_PCM_STREAM_PLAYBACK
		&& ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_CAPTURE)
		&& ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_PLAYBACK)) {
		if (dmadata->playback_data == 0 && pos != 0)
			pos = 0;
		else
			dmadata->playback_data = 1;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return bytes_to_frames(runtime, pos);
}

static snd_pcm_uframes_t
ky_snd_pcm_hdmi_pointer(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct ky_snd_dmadata *dmadata = substream->runtime->private_data;
	return bytes_to_frames(substream->runtime, dmadata->pos);
}

static int ky_snd_pcm_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct ky_snd_soc_device *dev;
	struct ky_snd_dmadata *dmadata;
	struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
	unsigned long flags;

	pr_debug("%s enter, rtd->dev=%s,dir=%d\n", __FUNCTION__, dev_name(rtd->dev),substream->stream);

	if (!component) {
		pr_err("%s!! coundn't find component %s\n", __FUNCTION__, DRV_NAME);
		ret = -1;
		goto out;
	}

	dev = snd_soc_component_get_drvdata(component);
	if (!dev) {
		pr_err("%s!! get dev error\n", __FUNCTION__);
		ret = -1;
		goto out;
	}
	spin_lock_irqsave(&dev->lock, flags);

	dmadata = &dev->dmadata[substream->stream];

	#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
	dmadata->debug_flag = 1;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		#ifdef CONFIG_KY_PLAY_DEBUG
		dmadata->play_flag = 0;
		#endif
	} else {
		#ifdef CONFIG_KY_CAPT_DEBUG
		dmadata->capt_flag = 0;
		#endif
	}
	#endif

	if (dmadata->dma_id == DMA_HDMI) {
		ret = snd_soc_set_runtime_hwparams(substream, &ky_snd_pcm_hardware_hdmi);
	} else {
		ret = snd_soc_set_runtime_hwparams(substream, &ky_snd_pcm_hardware);
	}

	if (ret) {
		goto unlock;
	}

	ret = snd_pcm_hw_constraint_integer(substream->runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		pr_err("%s!! got serror\n", __FUNCTION__);
		goto unlock;
	}
	substream->runtime->private_data = dmadata;

	if (dmadata->dma_id == DMA_HDMI) {
        hdmi_ptr.iec_offset = 0;
        hdmi_ptr.srate = 48000;
	}
unlock:
	spin_unlock_irqrestore(&dev->lock, flags);
out:
	pr_debug("%s exit dma_id=%d\n", __FUNCTION__, dmadata->dma_id);

	return ret;
}

static int ky_snd_pcm_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct ky_snd_dmadata *dmadata = substream->runtime->private_data;
	struct dma_chan *chan = dmadata->dma_chan;
	struct ky_snd_soc_device *dev = snd_soc_component_get_drvdata(component);
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	pr_debug("%s debug, dir=%d, dma_id=%d\n", __FUNCTION__,substream->stream, dmadata->dma_id);
	if (dmadata->stream == SNDRV_PCM_STREAM_PLAYBACK
		&& ky_get_stream_is_enable(dev, SNDRV_PCM_STREAM_CAPTURE)) {
		goto unlock;
	}
	dmaengine_terminate_all(chan);
	if (dmadata->dma_id == DMA_HDMI) {
		hdmi_ptr.iec_offset = 0;
	}
unlock:
	spin_unlock_irqrestore(&dev->lock, flags);

	#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
	dmadata->debug_flag = 0;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		#ifdef CONFIG_KY_PLAY_DEBUG
		if (dmadata->play_flag) {
			/* stop task */
			kthread_stop(dmadata->play_buf.daemon);
			vfree(dmadata->play_buf.pStart);
			memset(&dmadata->play_buf, 0, sizeof(dmadata->play_buf));
			dmadata->play_flag = 0;
		}
		#endif
	} else {
		#ifdef CONFIG_KY_CAPT_DEBUG
		if (dmadata->capt_flag) {
			kthread_stop(dmadata->capt_buf.daemon);
			memset(&dmadata->capt_buf, 0, sizeof(dmadata->capt_buf));
			dmadata->capt_flag = 0;
		}
		#endif
	}
	#endif

	return 0;
}

static int ky_snd_pcm_lib_ioctl(struct snd_soc_component *component, struct snd_pcm_substream *substream,
				unsigned int cmd, void *arg)
{
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

static const char * const ky_pcm_dma_channel_names[] = {
	[SNDRV_PCM_STREAM_PLAYBACK] = "tx",
	[SNDRV_PCM_STREAM_CAPTURE] = "rx",
};

static int ky_snd_pcm_new(struct snd_soc_component *component, struct snd_soc_pcm_runtime *rtd)
{
	int i;
	int ret;
	int chan_num;

	struct ky_snd_soc_device *dev;
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;

	pr_debug("%s enter, dev=%s\n", __FUNCTION__, dev_name(rtd->dev));

	if (!component) {
		pr_err("%s: coundn't find component %s\n", __FUNCTION__, DRV_NAME);
		return -1;
	}

	dev = snd_soc_component_get_drvdata(component);
	if (!dev) {
		pr_err("%s: get dev error\n", __FUNCTION__);
		return -1;
	}
	if (dev->dmadata->dma_id == DMA_HDMI) {
		chan_num = 1;
		pr_debug("%s playback_only, dev=%s\n", __FUNCTION__, dev_name(rtd->dev));
	} else {
		chan_num = 2;
	}
	dev->dmadata[0].stream = SNDRV_PCM_STREAM_PLAYBACK;
	dev->dmadata[1].stream = SNDRV_PCM_STREAM_CAPTURE;

	for (i = 0; i < chan_num; i++) {
		ret = ky_snd_dma_init(component->dev, dev, &dev->dmadata[i]);
		if (ret)
			goto exit;
	}

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
		card->dev, 64 * 1024 * 32, 4 * 1024 * 1024);

	return 0;

exit:
	for (i = 0; i < chan_num; i++) {
		if (dev->dmadata[i].dma_chan)
			dma_release_channel(dev->dmadata[i].dma_chan);
		dev->dmadata[i].dma_chan = NULL;
	}

	return ret;
}

static int ky_snd_dma_init(struct device *paraent, struct ky_snd_soc_device *dev,
					struct ky_snd_dmadata *dmadata)
{
	dma_cap_mask_t mask;
	spin_lock_init(&dmadata->dma_lock);

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_cap_set(DMA_CYCLIC, mask);

	dmadata->dma_chan = dma_request_slave_channel(paraent, ky_pcm_dma_channel_names[dmadata->stream]);
	if (!dmadata->dma_chan) {
		pr_err("DMA channel for %s is not available\n",
			dmadata->stream == SNDRV_PCM_STREAM_PLAYBACK ?
			"playback" : "capture");
		return -EBUSY;
	}

	return 0;
}

static int ky_snd_pcm_probe(struct snd_soc_component *component)
{
	struct ky_snd_soc_device *ky_snd_device = snd_soc_component_get_drvdata(component);

	ky_snd_device->dmadata[0].private_data = ky_snd_device;
	ky_snd_device->dmadata[1].private_data = ky_snd_device;

	return 0;
}

static void ky_snd_pcm_remove(struct snd_soc_component *component)
{
	int i;
	int chan_num;
	struct ky_snd_soc_device *dev = snd_soc_component_get_drvdata(component);

	pr_debug("%s enter\n", __FUNCTION__);

	if (dev->dmadata->dma_id == DMA_HDMI) {
		chan_num = 1;
	} else {
		chan_num = 2;
	}
	for (i = 0; i < chan_num; i++) {
		struct ky_snd_dmadata *dmadata = &dev->dmadata[i];
		struct dma_chan *chan = dmadata->dma_chan;

		if (chan) {
			dma_release_channel(chan);
		}
		dev->dmadata[i].dma_chan = NULL;
	}
}

static uint32_t parity_even(uint32_t sample)
{
	bool parity = 0;
	sample ^= sample >> 16;
	sample ^= sample >> 8;
	parity = ParityTable256[sample & 0xff];
	if (parity)
		return 1;
	else
		return 0;
}

static int32_t cal_cs_status_48kHz(int32_t offset)
{
	if ((offset == CS_SAMPLING_FREQUENCY) || (offset == CS_MAX_SAMPLE_WORD))
	{
		return 1;
	} else {
		return 0;
	}
}

static void hdmi_reformat(void *dst, void *src, int len)
{
	uint32_t *dst32 = (uint32_t *)dst;
	uint16_t *src16 = (uint16_t *)src;
	struct hdmi_codec_priv *dw = &hdmi_ptr;
	uint16_t frm_cnt = len;
	uint32_t ctrl;
	uint32_t sample,parity;
	dw->channels = 2;
	while (frm_cnt--) {
		for (int i = 0; i < dw->channels; i++) {
			//bit 0-23
			sample = ((uint32_t)(*src16++) << 8);
			//bit 24
			sample = sample | (1 << VALID_OFFSET);
			//bit 26
			sample = sample | (cal_cs_status_48kHz(dw->iec_offset) << CHANNEL_STATUS_OFFSET);
			//bit 27
			parity = parity_even(sample);
			sample = sample | (parity << PARITY_BIT_OFFSET);

			//bit 30 31
			if (dw->iec_offset == 0) {
				ctrl = CS_CTRL1;
			}  else {
				ctrl = CS_CTRL2;
			}
			sample = sample | ctrl;

			*dst32++ = sample;
		}

		dw->iec_offset++;
		if (dw->iec_offset >= 192){
			dw->iec_offset = 0;
		}
	}
}

#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
static int ky_snd_pcm_copy(struct snd_soc_component *component,
							struct snd_pcm_substream *substream,
							int channel, unsigned long hwoff,
							struct iov_iter *iter, unsigned long bytes)
{
	int ret = 0;
	char *hwbuf;
	struct snd_pcm_runtime *runtime = substream->runtime;
#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
	struct ky_snd_dmadata *dmadata = runtime->private_data;
#endif

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		hwbuf = runtime->dma_area + hwoff;
		if (copy_from_iter(hwbuf, bytes, iter) != bytes)
						return -EFAULT;
#ifdef CONFIG_KY_PLAY_DEBUG
		if (dump_id & (1 << (2 * dmadata->dma_id))) {
			ky_debug_data(hwbuf, &dmadata->play_buf, &dmadata->debug_flag, &dmadata->play_flag, runtime,
				bytes, KY_CODEC_TYPE, KY_PLAY_MODE, 0);
		}
#endif

	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		hwbuf = runtime->dma_area + hwoff;
		if (copy_to_iter(hwbuf, bytes, iter) != bytes)
						return -EFAULT;
#ifdef CONFIG_KY_CAPT_DEBUG
		if ((dump_id & (1 << (2 * dmadata->dma_id + 1)))) {
			ky_debug_data(hwbuf, &dmadata->capt_buf, &dmadata->debug_flag, &dmadata->capt_flag, runtime,
				bytes, KY_CODEC_TYPE, KY_CAPT_MODE,0);
		}
#endif

	}

	return ret;
}
#endif

static int ky_snd_pcm_hdmi_copy(struct snd_soc_component *component,
							struct snd_pcm_substream *substream,
							int channel, unsigned long hwoff,
							struct iov_iter *iter, unsigned long bytes)
{
	int ret = 0;
	char *hwbuf;
	char *hdmihw_area;
	struct snd_pcm_runtime *runtime = substream->runtime;
#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
	struct ky_snd_dmadata *dmadata = runtime->private_data;
#endif
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
#ifdef HDMI_REFORMAT_ENABLE
		hwbuf = runtime->dma_area + hwoff;
		if (copy_from_iter(hwbuf, bytes, iter) != bytes)
						return -EFAULT;
		hdmihw_area = hdmiraw_dma_area_tmp + 2 * hwoff;
		hdmi_reformat((int *)hdmihw_area, (short *)hwbuf, bytes_to_frames(substream->runtime, bytes));
		memcpy((void *)(hdmiraw_dma_area + 2 * hwoff), (void *)hdmihw_area, bytes * 2);
#else
		hwbuf = hdmiraw_dma_area + hwoff;
		if (hwbuf == NULL)
			pr_err("%s addr null !!!!!!!!!!!!\n", __func__);
		if (copy_from_iter(hwbuf, bytes, iter) != bytes)
						return -EFAULT;
#endif
#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
#ifdef CONFIG_KY_PLAY_DEBUG
		if (dump_id & (1 << (2 * dmadata->dma_id))) {
			ky_debug_data(hwbuf, &dmadata->play_buf, &dmadata->debug_flag, &dmadata->play_flag, runtime,
				bytes, KY_HDMI_TYPE,KY_PLAY_MODE, 0);
		}
#endif
#endif

	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		hwbuf = runtime->dma_area + hwoff;
		if (copy_to_iter(hwbuf, bytes, iter) != bytes)
						return -EFAULT;
	}

	return ret;
}


static const struct snd_soc_component_driver ky_snd_dma_component = {
	.name          = DRV_NAME,
	.probe		   = ky_snd_pcm_probe,
	.remove		   = ky_snd_pcm_remove,
	.open		   = ky_snd_pcm_open,
	.close		   = ky_snd_pcm_close,
	.ioctl		   = ky_snd_pcm_lib_ioctl,
	.hw_params	   = ky_snd_pcm_hw_params,
	.hw_free	   = ky_snd_pcm_hw_free,
	.trigger	   = ky_snd_pcm_trigger,
	.pointer	   = ky_snd_pcm_pointer,
	.pcm_construct = ky_snd_pcm_new,
#ifdef CONFIG_KY_AUDIO_DATA_DEBUG
	.copy		   = ky_snd_pcm_copy,
#endif
};

static const struct snd_soc_component_driver ky_snd_dma_component_hdmi = {
	.name          = DRV_NAME,
	.probe		   = ky_snd_pcm_probe,
	.remove		   = ky_snd_pcm_remove,
	.open		   = ky_snd_pcm_open,
	.close		   = ky_snd_pcm_close,
	.ioctl		   = ky_snd_pcm_lib_ioctl,
	.hw_params	   = ky_snd_pcm_hdmi_hw_params,
	.hw_free	   = ky_snd_pcm_hdmi_hw_free,
	.trigger	   = ky_snd_pcm_trigger,
	.pointer	   = ky_snd_pcm_hdmi_pointer,
	.pcm_construct = ky_snd_pcm_new,
	.copy		   = ky_snd_pcm_hdmi_copy,
};

static int ky_snd_dma_pdev_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	struct ky_snd_soc_device *device;

	device = devm_kzalloc(&pdev->dev, sizeof(*device), GFP_KERNEL);
	if (!device) {
		pr_err("%s: alloc memory failed\n", __FUNCTION__);
		return -ENOMEM;
	}

	pr_debug("%s enter: dev name %s\n", __func__, dev_name(&pdev->dev));

	if (of_device_is_compatible(np, "ky,ky-snd-dma-hdmi")){
		device->dmadata->dma_id = DMA_HDMI;
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		pr_debug("%s, start=0x%lx, end=0x%lx\n", __FUNCTION__, (unsigned long)res->start, (unsigned long)res->end);
		priv.phy_addr = res->start;
		priv.buf_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(priv.buf_base)) {
			pr_err("%s audio buf alloc failed\n", __FUNCTION__);
			return PTR_ERR(priv.buf_base);
		}
		ret = snd_soc_register_component(&pdev->dev, &ky_snd_dma_component_hdmi, NULL, 0);
	} else {
		if (of_device_is_compatible(np, "ky,ky-snd-dma0")){
			device->dmadata->dma_id = DMA_I2S0;
		} else if (of_device_is_compatible(np, "ky,ky-snd-dma1")) {
			device->dmadata->dma_id = DMA_I2S1;
		}
		ret = snd_soc_register_component(&pdev->dev, &ky_snd_dma_component, NULL, 0);
	}
	spin_lock_init(&device->lock);
	if (ret != 0) {
		dev_err(&pdev->dev, "failed to register DAI\n");
		return ret;
	}
	dev_set_drvdata(&pdev->dev, device);
	return 0;
}

static int ky_snd_dma_pdev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ky_snd_dma_ids[] = {
	{ .compatible = "ky,ky-snd-dma0", },
	{ .compatible = "ky,ky-snd-dma1", },
	{ .compatible = "ky,ky-snd-dma-hdmi", },
	{},
};
#endif

static struct platform_driver ky_snd_dma_pdrv = {
	.driver = {
		.name = "ky-snd-dma",
		.of_match_table = of_match_ptr(ky_snd_dma_ids),
	},
	.probe = ky_snd_dma_pdev_probe,
	.remove = ky_snd_dma_pdev_remove,
};

static void __exit ky_snd_pcm_exit(void)
{
	platform_driver_unregister(&ky_snd_dma_pdrv);
}
module_exit(ky_snd_pcm_exit);

static int ky_snd_pcm_init(void)
{
	return platform_driver_register(&ky_snd_dma_pdrv);
}
late_initcall(ky_snd_pcm_init);

#ifdef CONFIG_KY_AUDIO_DATA_DEBUG

static int check_path_exists(const char *path)
{
	struct path kp;
	if (kern_path(path, LOOKUP_PARENT, &kp) == 0) {
		return 1;
	} else {
		return 0;
	}
}

static void print_dump_config(char *buffer, int size)
{
	int i;
	unsigned int mask = 0x1;
	const char *dma_chans[] = {"I2S0", "I2S1", "HDMI"};
	int remaining_size;
	int playback_enabled, capture_enabled, line_length;
	snprintf(buffer, size,
				"Dump Configuration:\n"
				"Dmahan  Playback  Capture\n"
				"------- --------  -------\n");
	remaining_size = size - strlen(buffer);
	if (remaining_size <= 0) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(dma_chans); i++) {
		playback_enabled = (dump_id & (mask << (2 * i))) ? 1 : 0;
		capture_enabled = (dump_id & (mask << (2 * i + 1))) ? 1 : 0;

		line_length = snprintf(NULL, 0, "%6s %5d %10d\n", dma_chans[i], playback_enabled, capture_enabled);
		if (remaining_size < line_length + 1) {
			break;
		}
		snprintf(buffer + strlen(buffer), remaining_size, "%6s %5d %10d\n",
			dma_chans[i], playback_enabled, capture_enabled);
		remaining_size -= line_length;
	}
}

static int config_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

static ssize_t config_read(struct file *file, char __user *buffer, size_t count, loff_t *pos)
{
	char *output = NULL;
	int len, retval;
	size_t alloc_size = OUTPUT_BUFFER_SIZE;

	output = vmalloc(alloc_size);
	if (!output) {
		return -ENOMEM;
	}

	memset(output, 0, alloc_size);

	print_dump_config(output, alloc_size);
	len = strlen(output);

	if (*pos >= len) {
		retval = 0;
		goto out_free;
	}

	if (count > len - *pos)
		count = len - *pos;

	if (copy_to_user(buffer, output + *pos, count)) {
		retval = -EFAULT;
		goto out_free;
	}

	*pos += count;
	retval = count;

out_free:
	vfree(output);
	return retval;
}

static ssize_t config_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char input[32];
	char *p_input = input;
	char *param[4];
	char *token;
	char *endptr;
	int param_count = 0;
	unsigned int dma_chan;
	const char *usage_info=
		"Usage: [dma_chan:%%d] [mode:c/p] [enable:1/0] [/path/to/file]\n"
		"dma_chan: 0: I2S0, 1: I2S1, 2: HDMI\n"
		"mode:     c: capture, p: playback\n"
		"enable:   1: enable, 0: disable\n"
		"example: echo 0 p 1 /tmp/dump > /sys/kernel/debug/asoc/dump\n"
		"echo help/h > /sys/kernel/debug/asoc/dump\n";

	if (count > sizeof(input) - 1)
		count = sizeof(input) - 1;

	if (copy_from_user(input, buffer, count))
		return -EFAULT;

	input[count-1] = '\0';
	if (strcmp(input, "help") == 0 || strcmp(input, "h") == 0) {
		pr_info("%s", usage_info);
		return count;
	}

	while ((token = strsep(&p_input, " ")) != NULL) {
		if (param_count >= 4) {
			pr_err("Too many parameters.\n");
			return -EINVAL;
		}
		param[param_count++] = token;
	}

	if (param_count > 4 || param_count < 3) {
		pr_info("%s", usage_info);
		return -EINVAL;
	}

	dma_chan = (unsigned int)simple_strtol(param[0], &endptr, 10);
	if (endptr == param[0] || *endptr != '\0') {
		pr_err("Invalid dma_chan, please input a number\n");
		return -EINVAL;
	}
	if (dma_chan < DMA_I2S0 || dma_chan > DMA_HDMI) {
		pr_err("Invalid dma_chan, please input a number between %d and %d\n", DMA_I2S0, DMA_HDMI);
		return -EINVAL;
	}

	if (strcmp(param[1], "p") != 0 && strcmp(param[1], "c") != 0) {
		pr_err("Invalid mode: please input 'c' for capture or 'p' for playback\n");
		return -EINVAL;
	} else if (strcmp(param[2], "1") != 0 && strcmp(param[2], "0") != 0) {
		pr_err("Invalid status: please input '1' to enable or '0' to disable\n");
		return -EINVAL;
	}

	if (strcmp(param[1], "p") == 0 && strcmp(param[2], "0") == 0)
		dump_id &= ~(1 << (2 * dma_chan));
	else if (strcmp(param[1], "p") == 0 && strcmp(param[2], "1") == 0)
		dump_id |= (1 << (2 * dma_chan));
	else if (strcmp(param[1], "c") == 0 && strcmp(param[2], "0") == 0)
		dump_id &= ~(1 << (2 * dma_chan + 1));
	else if (strcmp(param[1], "c") == 0 && strcmp(param[2], "1") == 0)
		dump_id |= (1 << (2 * dma_chan + 1));
	else
		return -EINVAL;

	if (param_count == 4) {
		if (check_path_exists(param[3]) == 0) {
			pr_err("Path does not exist\n");
			return -EINVAL;
		} else {
			strncpy(dump_file_dir, param[3], sizeof(dump_file_dir) - 1);
		}
	}
	return count;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read  = config_read,
	.write = config_write,
	.open = config_open,
};

static int __init dump_debugfs_init(void)
{
	g_debug_file = debugfs_create_file("dump", 0666, snd_soc_debugfs_root, NULL, &fops);
	if (!g_debug_file) {
		pr_err("Failed to create debugfs file\n");
		return -ENOMEM;
	}
	pr_info("Dump debugfs initialized\n");
	return 0;
}

static void __exit dump_debugfs_exit(void)
{
	debugfs_remove(g_debug_file);
	pr_info("Debugfs removed\n");
}
module_init(dump_debugfs_init);
module_exit(dump_debugfs_exit);
#endif

MODULE_DESCRIPTION("KY ASoC PCM Platform Driver");
MODULE_LICENSE("GPL");

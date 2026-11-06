#include <audio_interface.h>
#include <audio_ioctl_cmd.h>
#include <audio_type.h>
#include <linux/spinlock.h>
#include <linux/dma-buf.h>
// add rots cmd
#include "rtos_cmdqu.h"
#include "sys.h"
// add for file save
#include <linux/fs.h>
#if (KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE)
#include <asm/segment.h>
#endif
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
// add for kthread
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/delay.h>
// add for kfifo
#include <linux/log2.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
// add for pcm record
#include <sound/pcm.h>
#include "asoundlib.h"
#include "ion.h"

#define AUDIO_ION_SIZE_VALID(X)  (((X) > CVAUDIO_AUDIO_ION_SIZE_REQUIRE) ? (-1) : (1))

// for unit test
#if (KERNEL_VERSION(5, 1, 0) <= LINUX_VERSION_CODE)
#define AUDIO_SAVE_FILE 1
#else
#define AUDIO_SAVE_FILE 1
#endif
#define GET_VARIABLE_NAME(Variable) (#Variable)
#ifdef AUDIO_SAVE_FILE
/*******************************************************
 *  specific defines for file operation
 ******************************************************/
// for LINUX_VERSION_CODE > 5.1.0
struct file *file_open2(const char *path, int flags, int rights)
{
	struct file *filp = NULL;
	int err = 0;

	filp = filp_open(path, flags, rights);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}
	return filp;
}

// for LINUX_VERSION_CODE < 5.1.0
#if (KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE)
struct file *file_open(const char *path, int flags, int rights)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}

	return filp;
}
#endif

#ifdef CONFIG_SDK_VER_32BIT
static int writeFile(struct file *fp, char *buf, int len)
{
	int wlen = 0, sum = 0;
#if (KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE)
	mm_segment_t oldfs;

	oldfs = get_fs();
#if (KERNEL_VERSION(5, 1, 0) <= LINUX_VERSION_CODE)
	set_fs(KERNEL_DS);
#else
	set_fs(get_ds());
#endif
#endif
	if (!fp->f_op || !fp->f_op->write)
		return -EPERM;

	while (sum < len) {
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
		wlen = (int)kernel_write(fp, buf + sum, len - sum, &fp->f_pos);
#else
		wlen = fp->f_op->write(fp, buf + sum, len - sum, &fp->f_pos);
#endif
		if (wlen > 0)
			sum += wlen;
		else if (0 != wlen) {
			return wlen;
			// return wlen
		} else
			break;
	}
#if (KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE)
	set_fs(oldfs);
#endif
	return sum;
}
#endif

#endif

/*******************************************************
 *  MACRO defines
 ******************************************************/

/*******************************************************
 *  Global variables
 ******************************************************/
static atomic_t	audio_dev_open_cnt;
u32 audio_log_lv = AUDIO_ERR;
st_audio_status gst_audio_status;
static wait_queue_head_t tWaitQueueDev[AUDIO_DRV_MAX_ALL_NUM];
ST_AIN_MEM_CTX stAinMemCtx[_MAX_AI_DEVICE_ID_NUM];
ST_AOUT_MEM_CTX stAoutMemCtx[_MAX_AO_DEVICE_ID_NUM];
ST_AENC_MEM_CTX stAencMemCtx;
ST_ADEC_MEM_CTX stAdecMemCtx;
ST_AUD_USAGE_STATUS stShareBufIndex[MAX_SHAREBUFFER_COUNT];
static DEFINE_SEMAPHORE(audio_core_sem);
// for rtos and rtos unit test
ST_SSP_RTOS_INIT *pSspRtosUnitTesCfg;
uint64_t SspRtosUnitTesCfgPhy;
static struct spinlock lock;
#ifdef AUDIO_SAVE_FILE
struct file *faudio_ut;
int valid_write_file;
int valid_isr_from_rtos;
#endif
// for rtos and rtos command
//replace old parameter gpstSspRtosData
ST_AUDIO_MAILBOX *gpstaudioMailBox;
uint64_t gstaudioMailBoxPhy;
ST_SSP_MIC_VQE_ADDR gstSspVqeAddr;
ST_SSP_RTOS_INDICATOR_ADDR gstSspIndicatorAddr;
ST_SSP_MIC_BUF_TABLE_ADDR gstSspBufTblAddr;
static int bEventExitPending;
static struct pcm_config mInputConfig = {
	.channels = 2,
	.rate = 16000,
	.period_size = 320,
	.period_count = 4,
	.format = PCM_FORMAT_S16_LE,
	.start_threshold = 0,
	.stop_threshold = 2147483647,
};
//#endif
/*******************************************************
 * Internal static APIs for audio_interface_dev.c
 ******************************************************/
int _audio_kzalloc_mempool(struct audio_dev *adev)
{
	int ret = 0;

	pr_err("_audio_kzalloc_mempool do the memory allocate in kernel\n");
	adev->shared_mem = kzalloc(AUDIO_SHARE_MEM_SIZE, GFP_ATOMIC);
	pr_err("shm[0x"PRT_PT"]\n", (TYPE_PT)adev->shared_mem);

	if (!adev->shared_mem) {
		pr_err("audio kzalloc size[%d] failure\n", AUDIO_SHARE_MEM_SIZE);
		return -ENOMEM;
	}

	return ret;
}

//static void _audio_mempool_reset(void)
//{
//	pr_err("[%s][%d] nothing\n", __func__, __LINE__);
//}

//static int _audio_mempool_setup(struct audio_dev *adev)
//{
//	int ret = 0;
//
//	_audio_mempool_reset();
//	ret = _audio_kzalloc_mempool(adev);
//
//	return ret;
//}

static int _audio_reset_all_mem(void)
{
	int ret = 0;
	int dev = 0;
	int chn = 0;
	// for ain memory context
	for (dev = 0; dev < _MAX_AI_DEVICE_ID_NUM; dev++) {
		stAinMemCtx[dev].tinyalsaReadBuf_addr = 0;
		stAinMemCtx[dev].tinyalsaReadBuf_size = 0;
		for (chn = 0; chn < _AUD_MAX_CHANNEL_NUM; chn++) {
			stAinMemCtx[dev].chn_mem_ctx[chn].vqebuff_addr = 0;
			stAinMemCtx[dev].chn_mem_ctx[chn].cycbufread_addr = 0;
			stAinMemCtx[dev].chn_mem_ctx[chn].vqebuf_size = 0;
			stAinMemCtx[dev].chn_mem_ctx[chn].cycbufread_size = 0;
			stAinMemCtx[dev].chn_mem_ctx[chn].resample_handler_addr = 0;
			stAinMemCtx[dev].chn_mem_ctx[chn].resample_handler_size = 0;
		}
	}
	// for aout memory context
	for (dev = 0; dev < _MAX_AO_DEVICE_ID_NUM; dev++) {
		for (chn = 0; chn < _AUD_MAX_CHANNEL_NUM; chn++) {
			stAoutMemCtx[dev].chn_mem_ctx[chn].sharebuf_first = 0;
			stAoutMemCtx[dev].chn_mem_ctx[chn].sharebuf_index = 0;
		}
	}
	for (chn = 0; chn < _AUD_MAX_CHANNEL_NUM; chn++) {
		// for aenc memory context
		stAencMemCtx.chn_mem_ctx[chn].EncInBuff_addr = 0;
		stAencMemCtx.chn_mem_ctx[chn].EncBuff_addr = 0;
		stAencMemCtx.chn_mem_ctx[chn].EncBuff_aec_addr = 0;
		stAencMemCtx.chn_mem_ctx[chn].EncInBuff_size = 0;
		stAencMemCtx.chn_mem_ctx[chn].EncBuff_size = 0;
		stAencMemCtx.chn_mem_ctx[chn].EncBuff_aec_size = 0;
		stAencMemCtx.chn_mem_ctx[chn].sharebuf_first = 0;
		stAencMemCtx.chn_mem_ctx[chn].sharebuf_index = 0;
		// for adec memory context
		stAdecMemCtx.chn_mem_ctx[chn].sharebuf_index = 0;
		stAdecMemCtx.chn_mem_ctx[chn].sharebuf_first = 0;
		stAdecMemCtx.chn_mem_ctx[chn].DecBuff_addr = 0;
		stAdecMemCtx.chn_mem_ctx[chn].DecBuff_size = 0;
		stAdecMemCtx.chn_mem_ctx[chn].DecReadBuff_addr = 0;
		stAdecMemCtx.chn_mem_ctx[chn].DecReadBuff_size = 0;
	}

	// reset stSspCtx
	if (gpstaudioMailBox == NULL) {
		int total_size = 0;

		base_ion_alloc(&gstaudioMailBoxPhy,
				(void *)&gpstaudioMailBox,
				"gstaudioMailBox_ION",
				CVAUDIO_AUDIO_ION_SIZE_REQUIRE, 1);
		total_size += sizeof(ST_AUDIO_MAILBOX);
		total_size = AUDIO_ALIGN(total_size, AUDIO_BYTES_ALIGNMENT);
		if (AUDIO_ION_SIZE_VALID(total_size) < 0) {
			audio_pr(AUDIO_ERR, "Not enough size...in mem alloc\n");
			goto return_failure;
		}

		gstSspVqeAddr.AinVqeCfgPhy = gstaudioMailBoxPhy + total_size;
		gstSspVqeAddr.pAinVqeCfg = (AI_TALKVQE_CONFIG_S_RTOS *)((size_t)gpstaudioMailBox + total_size);
		total_size += sizeof(AI_TALKVQE_CONFIG_S_RTOS);
		total_size = AUDIO_ALIGN(total_size, AUDIO_BYTES_ALIGNMENT);
		if (AUDIO_ION_SIZE_VALID(total_size) < 0) {
			audio_pr(AUDIO_ERR, "Not enough size...in mem alloc\n");
			goto return_failure;
		}

		gstSspIndicatorAddr.indicatorPhy = gstaudioMailBoxPhy + total_size;
		gstSspIndicatorAddr.pindicator = (ST_SSP_RTOS_INDICATOR *)((size_t)gpstaudioMailBox + total_size);
		total_size += sizeof(ST_SSP_RTOS_INDICATOR);
		total_size = AUDIO_ALIGN(total_size, AUDIO_BYTES_ALIGNMENT);
		if (AUDIO_ION_SIZE_VALID(total_size) < 0) {
			audio_pr(AUDIO_ERR, "Not enough size...in mem alloc\n");
			goto return_failure;
		}
		gstSspBufTblAddr.buffertblPhy = gstaudioMailBoxPhy + total_size;
		gstSspBufTblAddr.pbuffertbl = (ST_SSP_BUFTBL *)((size_t)gpstaudioMailBox + total_size);
		total_size += sizeof(ST_SSP_BUFTBL) * AUDIO_SSP_CHUNK_NUMBERS;
		total_size = AUDIO_ALIGN(total_size, AUDIO_BYTES_ALIGNMENT);
		if (AUDIO_ION_SIZE_VALID(total_size) < 0) {
			audio_pr(AUDIO_ERR, "Not enough size...in mem alloc\n");
			goto return_failure;
		}

		audio_pr(AUDIO_INFO,
			"PhYaudio_ mailbox[0x%llx] vqeCfg[0x%llx] indicator[0x%llx] bufferTbl[0x%llx]\n",
			gstaudioMailBoxPhy,
			gstSspVqeAddr.AinVqeCfgPhy,
			gstSspIndicatorAddr.indicatorPhy,
			gstSspBufTblAddr.buffertblPhy);
		audio_pr(AUDIO_INFO,
			"Vir_mb[0x"PRT_PT"] vCfg[0x"PRT_PT"] indic[0x"PRT_PT"] bTbl[0x"PRT_PT"]\n",
			(TYPE_PT)gpstaudioMailBox,
			(TYPE_PT)gstSspVqeAddr.pAinVqeCfg,
			(TYPE_PT)gstSspIndicatorAddr.pindicator,
			(TYPE_PT)gstSspBufTblAddr.pbuffertbl);
		//allocate the mic_in/ref_in/output address for table
		//mic_in and ref_in memory should be side by side as
		//LR|LR|LR = (MIC_IN)|(REF_IN) (MIC_IN)|(REF_IN)
		ret = base_ion_alloc(&gstSspBufTblAddr.mic_in_phy,
			(void *)&gstSspBufTblAddr.pmic_in_vir,
			"gstSspBufTblAddr_mic_in",
			AUDIO_MAX_BUFFER_SIZE * 2, 1);
		if (ret) {
			audio_pr(AUDIO_ERR, "Cannot require mic in ion memory\n");
			goto return_failure;
		}
		gstSspBufTblAddr.pref_in_vir = gstSspBufTblAddr.pmic_in_vir + (AUDIO_MAX_BUFFER_SIZE);
		audio_pr(AUDIO_INFO, "pmic_in_vir[0x"PRT_PT"]\n",
			(TYPE_PT)gstSspBufTblAddr.pmic_in_vir);
		audio_pr(AUDIO_INFO, "pref_in_vir[0x"PRT_PT"]\n",
			(TYPE_PT)gstSspBufTblAddr.pref_in_vir);
		gstSspBufTblAddr.ref_in_phy = gstSspBufTblAddr.mic_in_phy + (AUDIO_MAX_BUFFER_SIZE);
		//	(void *)&gstSspBufTblAddr.pref_in_vir,
		//	"gstSspBufTblAddr_ref_in",
		//	AUDIO_MAX_BUFFER_SIZE);
		ret = base_ion_alloc(&gstSspBufTblAddr.output_phy,
			(void *)&gstSspBufTblAddr.poutput_vir,
			"gstSspBufTblAddr_output",
			AUDIO_MAX_BUFFER_SIZE, 1);
		if (ret) {
			audio_pr(AUDIO_ERR, "Cannot require output ion memory\n");
			goto return_failure;
		}
	}
	memset(gpstaudioMailBox, 0, CVAUDIO_AUDIO_ION_SIZE_REQUIRE);
	// init the spinlock
	spin_lock_init(&lock);
	return ret;
return_failure:
	audio_pr(AUDIO_ERR, "audio_core [%s] error!!!\n", __func__);
	return -1;
}


static int _get_avail_sharebuf_index(void)
{
	int index = 1;

	for (index = 1; index < MAX_SHAREBUFFER_COUNT; index++) {
		if (stShareBufIndex[index].in_used == 0) {
			stShareBufIndex[index].in_used = 1;
			break;
		}
	}
	return index;
}

//static int _release_avail_sharebuf_index(int index)
//{
//	if (index < 0 || index > MAX_SHAREBUFFER_COUNT) {
//		audio_pr(AUDIO_ERR, "share index out of bound!!\n");
//		return -1;
//	}
//
//	if (stShareBufIndex[index].in_used == 1)
//		stShareBufIndex[index].in_used = 0;
//	else {
//		audio_pr(AUDIO_ERR, "sharebuf index already release!!\n");
//		return -1;
//	}
//
//	return 0;
//}

static int _audio_release_all_mem(void)
{
	// release all instance memory
	int ret = 0;
	int dev = 0;
	int chn = 0;
	int index = 0;

	index = index;
	for (dev = 0; dev < _MAX_AI_DEVICE_ID_NUM; dev++) {
		SAFE_FREE_BUF_PHY(&stAinMemCtx[dev].tinyalsaReadBuf_addr);
		for (chn = 0; chn < _AUD_MAX_CHANNEL_NUM; chn++) {
			SAFE_FREE_BUF_PHY(&stAinMemCtx[dev].chn_mem_ctx[chn].vqebuff_addr);
			SAFE_FREE_BUF_PHY(&stAinMemCtx[dev].chn_mem_ctx[chn].cycbufread_addr);
			SAFE_FREE_BUF_PHY(&stAinMemCtx[dev].chn_mem_ctx[chn].resample_handler_addr);
		}
		memset(&stAinMemCtx[dev], 0, sizeof(ST_AIN_MEM_CTX));
	}

	for (dev = 0; dev < _MAX_AO_DEVICE_ID_NUM; dev++) {
		for (chn = 0; chn < _AUD_MAX_CHANNEL_NUM; chn++) {
			// TODO: check safe free buffer for audio out
		}
		memset(&stAoutMemCtx[dev], 0, sizeof(ST_AOUT_MEM_CTX));
	}

	for (chn = 0; chn < _AUD_MAX_CHANNEL_NUM; chn++) {
		//safe free buf phy address reset to 0
		SAFE_FREE_BUF_PHY(&stAencMemCtx.chn_mem_ctx[chn].EncInBuff_addr);
		SAFE_FREE_BUF_PHY(&stAencMemCtx.chn_mem_ctx[chn].EncBuff_addr);
		SAFE_FREE_BUF_PHY(&stAencMemCtx.chn_mem_ctx[chn].EncBuff_aec_addr);
		//safe free buf phy address reset to 0 for adec
		SAFE_FREE_BUF_PHY(&stAdecMemCtx.chn_mem_ctx[chn].DecBuff_addr);
		SAFE_FREE_BUF_PHY(&stAdecMemCtx.chn_mem_ctx[chn].DecReadBuff_addr);
	}
	memset(&stAencMemCtx, 0, sizeof(ST_AENC_MEM_CTX));
	memset(&stAdecMemCtx, 0, sizeof(ST_ADEC_MEM_CTX));

	//for (index = 0; index < MAX_SHAREBUFFER_COUNT; index++)
	//	_release_avail_sharebuf_index(index);

	pr_err("release all memory in audio_core.ko xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>\n");
	//release RTOS SSP related memory
	//release mailbox for rtos communication
	if (!gstSspBufTblAddr.pmic_in_vir)
		base_ion_free(gstSspBufTblAddr.mic_in_phy);
	if (!gstSspBufTblAddr.poutput_vir)
		base_ion_free(gstSspBufTblAddr.output_phy);
	if (!gpstaudioMailBox)
		base_ion_free(gstaudioMailBoxPhy);
	return ret;
}

static int _audio_init_param(struct audio_dev *adev)
{
	// init the variable once when insmod audio_core.ko
	int ret = 0;

	atomic_set(&audio_dev_open_cnt, 0);
	// gst_audio_status.counter = 0;
	return ret;
}

/*******************************************************
 * Export APIs for audio device
 ******************************************************/
int audio_create_instance(struct platform_device *pdev)
{
	int ret = 0;

	struct audio_dev *adev = platform_get_drvdata(pdev);

	if (!adev) {
		pr_err("invalid data[%s][%d]\n", __func__, __LINE__);
		return -EINVAL;
	}
	//do not init the mempool in probe stage
	// user using ioctl to get the corresponding struct address
	//ret = _audio_mempool_setup(adev);
	//if (ret) {
	//	pr_err("Failed to setup audio memory[%s][%d]\n", __func__, __LINE__);
	//	goto err;
	//}

	ret = _audio_init_param(adev);
	if (ret) {
		pr_err("Failed to setup audio init parameters[%s][%d]\n", __func__, __LINE__);
		goto err;
	}

err:
	return ret;
}

int audio_destroy_instance(struct platform_device *pdev)
{
	int ret = 0;

	return ret;
}
int audio_device_open(struct inode *inode, struct file *filp)
{
	unsigned int minor = iminor(inode);
	struct audio_dev *adev;

	pr_err("[%s][%d] open device minor no. %u\n", __func__, __LINE__, minor);
	// attach adev to private data
	adev = container_of(inode->i_cdev, struct audio_dev, cdev_ceo);
	filp->private_data = adev;
	if (!atomic_read(&audio_dev_open_cnt)) {
		// memory pool reset
		// status reset
		memset(&gst_audio_status, 0x00, sizeof(st_audio_status));
		_audio_reset_all_mem();
		pr_err("[%s][%d]--------------------------->\n", __func__, __LINE__);
	}
	atomic_inc(&audio_dev_open_cnt);
	// pr_err("open count:[%ld]\n", audio_dev_open_cnt);
	gst_audio_status.counter += 1;

	init_waitqueue_head(&tWaitQueueDev[minor]);
	return 0;
}
static int _audio_flush_ssp_indicator_and_tbl(void)
{
//make sure the indicator and tbl into physical address
	unsigned long flags;

	if (!gstSspIndicatorAddr.pindicator || !gstSspBufTblAddr.pbuffertbl)
		return 0;

	spin_lock_irqsave(&lock, flags);
	base_ion_cache_flush(gstSspIndicatorAddr.indicatorPhy,
			gstSspIndicatorAddr.pindicator,
			sizeof(ST_SSP_RTOS_INDICATOR));
	base_ion_cache_flush(gstSspBufTblAddr.buffertblPhy,
			gstSspBufTblAddr.pbuffertbl,
			sizeof(ST_SSP_BUFTBL)*AUDIO_SSP_CHUNK_NUMBERS);
	spin_unlock_irqrestore(&lock, flags);
	return 1;
}

static int _audio_flush_ssp_input_data(void)
{
//make sure the input mic_in/ref_in data into physical address
	unsigned long flags;

	if (!gstSspBufTblAddr.pmic_in_vir)
		return 0;

	spin_lock_irqsave(&lock, flags);
	base_ion_cache_flush(gstSspBufTblAddr.mic_in_phy,
			gstSspBufTblAddr.pmic_in_vir,
			AUDIO_MAX_BUFFER_SIZE * 2);
	spin_unlock_irqrestore(&lock, flags);
	return 1;
}

static int _audio_invalidate_ssp_indicator_and_tbl(void)
{
//make sure the kernel virtual addr update the indicator and tble
	unsigned long flags;

	if (!gstSspIndicatorAddr.pindicator || !gstSspBufTblAddr.pbuffertbl)
		return 0;

	spin_lock_irqsave(&lock, flags);
	base_ion_cache_invalidate(gstSspIndicatorAddr.indicatorPhy,
		gstSspIndicatorAddr.pindicator,
		sizeof(ST_SSP_RTOS_INDICATOR));
	base_ion_cache_invalidate(gstSspBufTblAddr.buffertblPhy,
		gstSspBufTblAddr.pbuffertbl,
		sizeof(ST_SSP_BUFTBL)*AUDIO_SSP_CHUNK_NUMBERS);
	spin_unlock_irqrestore(&lock, flags);
	return 1;
}

static int _audio_invalidate_ssp_output_data(void)
{
//make sure the kernel virtual add update the output data
	unsigned long flags;

	if (!gstSspBufTblAddr.poutput_vir)
		return 0;

	spin_lock_irqsave(&lock, flags);
	base_ion_cache_invalidate(gstSspBufTblAddr.output_phy,
				gstSspBufTblAddr.poutput_vir,
				AUDIO_MAX_BUFFER_SIZE);
	spin_unlock_irqrestore(&lock, flags);
	return 1;
}

static void _audio_dump_ssp_indicator_and_tbl(const char *_input_name, int line)
{
	unsigned long flags;
	int index = 0;
	ST_SSP_RTOS_INDICATOR *_pindicator = gstSspIndicatorAddr.pindicator;
	ST_SSP_BUFTBL	*_pbuffertbl = &gstSspBufTblAddr.pbuffertbl[0];

	if (!gstSspIndicatorAddr.pindicator || !gstSspBufTblAddr.pbuffertbl)
		return;

	spin_lock_irqsave(&lock, flags);
	audio_pr(AUDIO_INFO, "DDDDDDDDDDDDDDDfrom[%s][%d]DDDDDDDDDDDDDDDDDDDDDDDD-START\n",
		_input_name, line);
	audio_pr(AUDIO_INFO, "Wpt[%d] Rpt[%d] Ppt[%d]chn[%d] on_off[%d]\n",
		_pindicator->Wpt_index,
		_pindicator->Rpt_index,
		_pindicator->Ppt_index,
		_pindicator->channel_nums,
		_pindicator->ssp_on);
	for (index = 0; index < AUDIO_SSP_CHUNK_NUMBERS; index++) {
		audio_pr(AUDIO_INFO, "tbl index[%d] occpuied_flag[%d]\n",
			index, _pbuffertbl[index].bBufOccupy);
	}
	audio_pr(AUDIO_INFO, "DDDDDDDDDDDDDDDfrom[%s][%d]DDDDDDDDDDDDDDDDDDDDDDDD-END\n",
		_input_name, line);
	spin_lock_irqsave(&lock, flags);
}

static int pcm_event_handler(void *data)
{

	int ret = 0;
	struct pcm *mPcmHandle = NULL;
	int period_bytes = mInputConfig.period_size * mInputConfig.channels * 2;
	short *pBuffer = NULL;
	unsigned int required_chunks = 0;
	unsigned int each_get_chunks = mInputConfig.period_size / AUDIO_SSP_SINGLE_CHUNK_SAMPLES;
	unsigned char cur_write_cnt = 0;
	unsigned char cur_pt = 0;
	ST_SSP_RTOS_INDICATOR *_pindicator = gstSspIndicatorAddr.pindicator;
	ST_SSP_BUFTBL	*pbuffertbl;
	char *cur_mic_in_addr;
	char *cur_ref_in_addr;
	unsigned long flags;

	audio_pr(AUDIO_INFO, "mInputConfig.period_size[%d]chn[%d]rate[%d]\n",
		mInputConfig.period_size,
		mInputConfig.channels,
		mInputConfig.rate);

	mPcmHandle = pcm_open(0, 0, PCM_IN, &mInputConfig);
	if (!_pindicator || !gstSspBufTblAddr.pbuffertbl) {
		audio_pr(AUDIO_ERR, "NULL pt fatal error\n");
		goto PCM_EVENT_LEAVE;
	}
	if (mInputConfig.period_size)
		pBuffer = kzalloc(period_bytes, GFP_KERNEL);
	if (!pBuffer) {
		audio_pr(AUDIO_ERR, "Buffer null in %s thread\n", __func__);
		goto PCM_EVENT_LEAVE;
	}
	if (!mPcmHandle) {
		audio_pr(AUDIO_ERR, "pcm open error in %s thread\n", __func__);
		goto PCM_EVENT_LEAVE;
	}
	pbuffertbl = &gstSspBufTblAddr.pbuffertbl[0];
	required_chunks = (_pindicator->chunks_number < each_get_chunks) ? each_get_chunks : _pindicator->chunks_number;
	audio_pr(AUDIO_INFO, "required_chunks[%d]\n", required_chunks);

	while (!bEventExitPending) {
		//check enough empty chunks for input data
		cur_write_cnt = 0;
		cur_pt = _pindicator->Wpt_index;
		while (cur_write_cnt < required_chunks) {
			if (pbuffertbl[cur_pt].bBufOccupy != AUDIO_BUF_TBL_UNOCCUPIED) {
				audio_pr(AUDIO_ERR,
					"Not enough empty space for input pcm data Wpt need_cnt[%d]xxx\n",
					required_chunks);
				_audio_dump_ssp_indicator_and_tbl(__func__, __LINE__);
				msleep(200);
				break;

			} else {
				cur_write_cnt += 1;
				cur_pt = (cur_pt + 1) % AUDIO_SSP_CHUNK_NUMBERS;
			}
		}
		if (cur_write_cnt < required_chunks) {
			mdelay(10);
			audio_pr(AUDIO_ERR,
				"Not enough empty space for input cur_write_cnt[%d] need_cnt[%d]xxx\n",
				cur_write_cnt, required_chunks);
			if (!_audio_invalidate_ssp_indicator_and_tbl()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}
		} else if (cur_write_cnt == required_chunks) {
			unsigned int remained_chunks = required_chunks;
			unsigned int each_get_samples = mInputConfig.period_size;
			short *pmic;
			short *pref;

			audio_pr(AUDIO_INFO, "ToGo Wpt_index[%d]req_chunks[%d]each_get_chunks[%d]\n",
					_pindicator->Wpt_index, required_chunks, each_get_chunks);
			cur_pt = _pindicator->Wpt_index;
			cur_mic_in_addr = gstSspBufTblAddr.pmic_in_vir + cur_pt * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
			cur_ref_in_addr = gstSspBufTblAddr.pref_in_vir + cur_pt * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
			while ((remained_chunks) && (remained_chunks >= each_get_chunks)) {
				ret = pcm_read(mPcmHandle, (char *)pBuffer, period_bytes);
				if (ret < 0) {
					audio_pr(AUDIO_ERR,
						"pcm_read error ..force break pcm_event!!\n");
					goto PCM_EVENT_LEAVE;
				}
				if (cur_pt + each_get_chunks > AUDIO_SSP_CHUNK_NUMBERS) {
					unsigned char first_chunk = AUDIO_SSP_CHUNK_NUMBERS - cur_pt;
					unsigned char second_chunk = each_get_chunks - first_chunk;
					audio_pr(AUDIO_INFO, "Two steps Copy\n");

					if (mInputConfig.channels == 1) {
						char *tmp;

						memcpy(cur_mic_in_addr, (char *)pBuffer,
							AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES * first_chunk);
						cur_mic_in_addr = gstSspBufTblAddr.pmic_in_vir;
						cur_ref_in_addr = gstSspBufTblAddr.pref_in_vir;
						tmp = (char *)pBuffer +
							AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES * first_chunk;
						memcpy(cur_mic_in_addr, tmp,
							second_chunk * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
					} else if (mInputConfig.channels == 2) {
						int i = 0;
						int first_size = first_chunk * AUDIO_SSP_SINGLE_CHUNK_SAMPLES;

						pmic = (short *)cur_mic_in_addr;
						pref = (short *)cur_ref_in_addr;
						i = 0;

					while (i < first_chunk * AUDIO_SSP_SINGLE_CHUNK_SAMPLES) {
						pmic[i] = pBuffer[2*i];
						pref[i] = pBuffer[2*i + 1];
						i += 1;
					}

						pmic = (short *)gstSspBufTblAddr.pmic_in_vir;
						pref = (short *)gstSspBufTblAddr.pref_in_vir;
						i = 0;

					while (i < second_chunk * AUDIO_SSP_SINGLE_CHUNK_SAMPLES) {
						pmic[i] = pBuffer[2*i + first_size];
						pref[i] = pBuffer[2*i + 1 + first_size];
						i += 1;
					}
					} else {
						audio_pr(AUDIO_ERR, "chr error force leave pcm_event\n");
						goto PCM_EVENT_LEAVE;
					}
					cur_pt = (cur_pt + each_get_chunks) % AUDIO_SSP_CHUNK_NUMBERS;
					cur_mic_in_addr = gstSspBufTblAddr.pmic_in_vir + cur_pt *
								AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
					cur_ref_in_addr = gstSspBufTblAddr.pref_in_vir + cur_pt *
								AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;

				} else {
					audio_pr(AUDIO_INFO, "One steps Copy\n");
					if (mInputConfig.channels == 1) {
						memcpy(cur_mic_in_addr, (char *)pBuffer, period_bytes);
					} else if (mInputConfig.channels == 2) {
						unsigned int i = 0;

						pmic = (short *)cur_mic_in_addr;
						pref = (short *)cur_ref_in_addr;
					for (i = 0; i < each_get_samples; i++) {
						pmic[i] = pBuffer[2*i];
						pref[i] = pBuffer[2*i + 1];
					}
					} else {
						audio_pr(AUDIO_ERR, "chr error force leave pcm_event\n");
						goto PCM_EVENT_LEAVE;
					}
					cur_pt = (cur_pt + each_get_chunks) % AUDIO_SSP_CHUNK_NUMBERS;
					audio_pr(AUDIO_INFO, "After One step cur_pt[%d]\n", cur_pt);
					cur_mic_in_addr = gstSspBufTblAddr.pmic_in_vir + cur_pt *
								AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
					cur_ref_in_addr = gstSspBufTblAddr.pref_in_vir + cur_pt *
								AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
				}
				remained_chunks -= each_get_chunks;
			}
			//update indicator to occupy and update wpt
			spin_lock_irqsave(&lock, flags);
			cur_pt = _pindicator->Wpt_index;
			cur_write_cnt = 0;
			while (cur_write_cnt < (required_chunks - remained_chunks)) {
				pbuffertbl[cur_pt].bBufOccupy = AUDIO_BUF_TBL_INPUT;
				cur_pt = (cur_pt + 1) % AUDIO_SSP_CHUNK_NUMBERS;
				cur_write_cnt += 1;
			}
			_pindicator->Wpt_index = cur_pt;
			spin_unlock_irqrestore(&lock, flags);
			//flush input data first,then turn the flag from empty to occupied
			if (!_audio_flush_ssp_input_data()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}
			if (!_audio_flush_ssp_indicator_and_tbl()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}


		} else {
			audio_pr(AUDIO_ERR, "FATAL_ERROR in input pcm thread\n");
			goto PCM_EVENT_LEAVE;
		}

		//enough empty chunks, start pcm_read
		//ret = pcm_read(mPcmHandle, (char *)pBuffer, period_bytes);
		//if (ret < 0) {
		//	usleep_range(1000 * 10, 1000 * 20);
		//	continue;
		//}
	}

PCM_EVENT_LEAVE:
	audio_pr(AUDIO_ERR, "thread leave[%s][%d]\n", __func__, __LINE__);
	if (mPcmHandle)
		pcm_close(mPcmHandle);
	kfree(pBuffer);
	return 0;
}

#ifndef PCM_NATIVE_EXPORT_SSP
static int kssp_input_task(void *arg)
{
	int *d = (int *)arg;
	char *cur_mic_in_addr;
	unsigned char test_bytes = 0x01;
	unsigned long flags;

	d = d;
	while (1) {

		//TODO: WAIT INPUT_FIFO HAVE enough data and cobuffer is not occupy
		ST_SSP_RTOS_INDICATOR *_pindicator = gstSspIndicatorAddr.pindicator;
		ST_SSP_BUFTBL	*pbuffertbl;
		unsigned char cur_pt, cur_write_cnt, valid_input_cnt, target_input_cnt;
		unsigned int input_size = 640;//bytes, for simulate

		if (!_pindicator) {
			audio_pr(AUDIO_ERR, "NULL pt fatal error\n");
			break;
		}

		if (!gstSspBufTblAddr.pbuffertbl) {
			audio_pr(AUDIO_ERR, "NULL pt fatal error\n");
			break;
		}
		pbuffertbl = &gstSspBufTblAddr.pbuffertbl[0];
		cur_pt = _pindicator->Wpt_index;
		cur_write_cnt = 0;
		valid_input_cnt = 0;

		target_input_cnt = input_size / AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
		if ((input_size % AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES) != 0)
			audio_pr(AUDIO_ERR, "residual bytes while record in...\n");
		while (cur_write_cnt < target_input_cnt) {
			if (pbuffertbl[cur_pt].bBufOccupy != AUDIO_BUF_TBL_UNOCCUPIED) {
				audio_pr(AUDIO_ERR,
					"Not enough empty space for input pcm data Wpt need_cnt[%d]xxx\n",
					target_input_cnt);
				_audio_dump_ssp_indicator_and_tbl(__func__, __LINE__);
				msleep(200);
				break;
			}
			cur_write_cnt += 1;
			cur_pt = ((cur_pt + 1) % AUDIO_SSP_CHUNK_NUMBERS);
		}
		if (cur_write_cnt < target_input_cnt) {
			//not enough empty space for input pcm data
			mdelay(10);
			if (!_audio_invalidate_ssp_indicator_and_tbl()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}
		} else if (cur_write_cnt == target_input_cnt) {
			cur_pt = _pindicator->Wpt_index;
			cur_mic_in_addr = gstSspBufTblAddr.pmic_in_vir + cur_pt * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;

			if (cur_pt + target_input_cnt > AUDIO_SSP_CHUNK_NUMBERS) {
				//buffer will loop back, need two memcpy
				unsigned char first_chunk = AUDIO_SSP_CHUNK_NUMBERS - cur_pt;
				unsigned char second_chunk = target_input_cnt - first_chunk;

				memcpy(cur_mic_in_addr, &test_bytes,
					first_chunk * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
				memcpy(gstSspBufTblAddr.pmic_in_vir, &test_bytes,
					second_chunk * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
			} else {
				//buffer will not loop back, need only once copy
				memcpy(cur_mic_in_addr, &test_bytes,
					target_input_cnt * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
			}
			//update indicator to occupy and update wpt
			spin_lock_irqsave(&lock, flags);
			cur_pt = _pindicator->Wpt_index;
			cur_write_cnt = 0;
			while (cur_write_cnt < target_input_cnt) {
				pbuffertbl[cur_pt].bBufOccupy = AUDIO_BUF_TBL_INPUT;
				cur_pt = (cur_pt + 1) % AUDIO_SSP_CHUNK_NUMBERS;
				cur_write_cnt += 1;
			}
			_pindicator->Wpt_index = cur_pt;
			spin_unlock_irqrestore(&lock, flags);
			//flush input data first,then turn the flag from empty to occupied
			if (!_audio_flush_ssp_input_data()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}
			if (!_audio_flush_ssp_indicator_and_tbl()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}
			test_bytes += 1;

		} else {
			audio_pr(AUDIO_ERR, "FATAL_ERROR in input pcm thread\n");
			break;
		}

		//udelay(1500);
		//udelay(1500);->stock the ioctl
		//msleep(5); -> will force sleep atleast 20ms, idealy 5ms
		usleep_range(10 * 1000, 20 * 1000);
		if (kthread_should_stop()) {
			audio_pr(AUDIO_INFO, "[%s]thread called stop!!\n", __func__);
			break;
		}
	}

	return 0;
}
#else
static int kssp_input_task(void *arg)
{
	int *d = (int *)arg;
	int msleep_time = 50;
	unsigned int required_size_bytes = 640;//bytes, for simulate
	unsigned int required_chunks = 0;
	unsigned int chn_num = 0;
	unsigned char cur_pt, cur_write_cnt, valid_input_cnt, target_input_cnt;
	char *cur_mic_in_addr;
	char *cur_ref_in_addr;
	unsigned long flags;
#define AUDIO_BUFFER_CHUNK_SIZE (AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES * 2) //prepare for 2 channel data bytes
	char *_buffer = kzalloc(AUDIO_BUFFER_CHUNK_SIZE, GFP_KERNEL);

	d = d;
	audio_pr(AUDIO_INFO, "PCM_NATIVE_EXPORT_SSP, get pcm read from linux/pcm_native.c\n");


	while (1) {
		ST_SSP_RTOS_INDICATOR *_pindicator = gstSspIndicatorAddr.pindicator;
		ST_SSP_BUFTBL	*pbuffertbl;


		if (!snd_pcm_ssp_rdy()) {
			//pcm open not success yet
			msleep_time = 100;
			goto KSSP_INPUT_TASK_WAIT;
		} else {
			msleep_time = 0;
		}

		if (!_pindicator) {
			audio_pr(AUDIO_ERR, "NULL pt fatal error\n");
			break;
		}

		if (!gstSspBufTblAddr.pbuffertbl) {
			audio_pr(AUDIO_ERR, "NULL pt fatal error\n");
			break;
		}
		pbuffertbl = &gstSspBufTblAddr.pbuffertbl[0];
		cur_pt = _pindicator->Wpt_index;
		cur_write_cnt = 0;
		valid_input_cnt = 0;
		required_chunks = _pindicator->chunks_number;
		required_size_bytes = required_chunks * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;

		if (_pindicator->ssp_with_aec) {
			chn_num = 2;
			required_size_bytes = required_size_bytes * 2;
		} else
			chn_num = 1;

		target_input_cnt = required_size_bytes / AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
		if ((required_size_bytes % AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES) != 0)
			audio_pr(AUDIO_ERR, "residual bytes while record in...\n");
		while (cur_write_cnt < target_input_cnt) {
			if (pbuffertbl[cur_pt].bBufOccupy != AUDIO_BUF_TBL_UNOCCUPIED) {
				audio_pr(AUDIO_ERR,
					"Not enough empty space for input pcm data Wpt need_cnt[%d]xxx\n",
					target_input_cnt);
				_audio_dump_ssp_indicator_and_tbl(__func__, __LINE__);
				msleep(200);
				break;
			}
			cur_write_cnt += 1;
			cur_pt = ((cur_pt + 1) % AUDIO_SSP_CHUNK_NUMBERS);
		}
		if (cur_write_cnt < target_input_cnt) {
			//not enough empty space for input pcm data
			mdelay(10);
			if (!_audio_invalidate_ssp_indicator_and_tbl()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}
		} else if (cur_write_cnt == target_input_cnt) {
			short *tmp;
			short *pmic;
			short *pref;
			int i = 0;
			int ret = 0;

			cur_pt = _pindicator->Wpt_index;
			cur_mic_in_addr = gstSspBufTblAddr.pmic_in_vir + cur_pt * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
			cur_ref_in_addr = gstSspBufTblAddr.pref_in_vir + cur_pt * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;


			tmp = (short *)_buffer;
			if (cur_pt + target_input_cnt > AUDIO_SSP_CHUNK_NUMBERS) {
				//buffer will loop back, need two memcpy
				unsigned char first_chunk = AUDIO_SSP_CHUNK_NUMBERS - cur_pt;
				unsigned char second_chunk = target_input_cnt - first_chunk;

				while (first_chunk) {
					ret = ssp_pcm_read(_buffer, AUDIO_SSP_SINGLE_CHUNK_SAMPLES);

					if (ret < 0) {
						audio_pr(AUDIO_ERR, "ssp_pcm_read < 0\n");
						goto KSSP_INPUT_TASK_ERROR;
					}
					if (chn_num == 1) {
						memcpy(cur_mic_in_addr, _buffer, AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
					} else if (chn_num == 2) {
						pmic = (short *)cur_mic_in_addr;
						pref = (short *)cur_ref_in_addr;

					for (i = 0; i < AUDIO_SSP_SINGLE_CHUNK_SAMPLES; i++) {
						pmic[i] = tmp[2*i];
						pref[i] = tmp[2*i + 1];
					}

					} else {
						audio_pr(AUDIO_ERR, "chn_err  force leave input_task !!\n");
						goto KSSP_INPUT_TASK_ERROR;
					}
					cur_mic_in_addr = cur_mic_in_addr + AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
					cur_ref_in_addr = cur_ref_in_addr + AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
					first_chunk -= 1;

				}
				cur_mic_in_addr = gstSspBufTblAddr.pmic_in_vir;
				cur_ref_in_addr = gstSspBufTblAddr.pref_in_vir;
				while (second_chunk) {
					if (!snd_pcm_ssp_rdy()) {
						//pcm open not success yet
						msleep_time = 100;
						goto KSSP_INPUT_TASK_WAIT;
					} else
						msleep_time = 0;

					ret = ssp_pcm_read(_buffer, AUDIO_SSP_SINGLE_CHUNK_SAMPLES);
					if (ret < 0) {
						audio_pr(AUDIO_ERR, "ssp_pcm_read < 0\n");
						goto KSSP_INPUT_TASK_ERROR;
					}
					if (chn_num == 1) {
						memcpy(cur_mic_in_addr, _buffer, AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
					} else if (chn_num == 2) {
						pmic = (short *)cur_mic_in_addr;
						pref = (short *)cur_ref_in_addr;
					for (i = 0; i < AUDIO_SSP_SINGLE_CHUNK_SAMPLES; i++) {
						pmic[i] = tmp[2*i];
						pref[i] = tmp[2*i + 1];
					}

					} else {
						audio_pr(AUDIO_ERR, "chn_err  force leave input_task !!\n");
						goto KSSP_INPUT_TASK_ERROR;
					}
					cur_mic_in_addr = cur_mic_in_addr + AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
					cur_ref_in_addr = cur_ref_in_addr + AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
					second_chunk -= 1;

				}
			} else {
				//buffer will not loop back, need only once copy
				unsigned char total_chunk = target_input_cnt;

				while (total_chunk) {
					ret = ssp_pcm_read(_buffer, AUDIO_SSP_SINGLE_CHUNK_SAMPLES);
					if (ret < 0) {
						audio_pr(AUDIO_ERR, "ssp_pcm_read < 0\n");
						goto KSSP_INPUT_TASK_ERROR;
					}

					if (chn_num == 1) {
						memcpy(cur_mic_in_addr, _buffer, AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
					} else if (chn_num == 2) {
						pmic = (short *)cur_mic_in_addr;
						pref = (short *)cur_ref_in_addr;

					for (i = 0; i < AUDIO_SSP_SINGLE_CHUNK_SAMPLES; i++) {
						pmic[i] = tmp[2*i];
						pref[i] = tmp[2*i + 1];
					}

					} else {
						audio_pr(AUDIO_ERR, "chn_err  force leave input_task !!\n");
						goto KSSP_INPUT_TASK_ERROR;
					}
					cur_mic_in_addr = cur_mic_in_addr + AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
					cur_ref_in_addr = cur_ref_in_addr + AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
					total_chunk -= 1;
				}
			}
			//update indicator to occupy and update wpt
			spin_lock_irqsave(&lock, flags);
			cur_pt = _pindicator->Wpt_index;
			cur_write_cnt = 0;
			while (cur_write_cnt < target_input_cnt) {
				pbuffertbl[cur_pt].bBufOccupy = AUDIO_BUF_TBL_INPUT;
				cur_pt = (cur_pt + 1) % AUDIO_SSP_CHUNK_NUMBERS;
				cur_write_cnt += 1;
			}
			_pindicator->Wpt_index = cur_pt;
			spin_unlock_irqrestore(&lock, flags);
			//flush input data first,then turn the flag from empty to occupied
			if (!_audio_flush_ssp_input_data()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}
			if (!_audio_flush_ssp_indicator_and_tbl()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}

		} else {
			audio_pr(AUDIO_ERR, "FATAL_ERROR in input pcm thread\n");
			break;
		}
KSSP_INPUT_TASK_WAIT:
		//udelay(1500);->stock the ioctl
		if (msleep_time)
			msleep(msleep_time);
		else
			usleep_range(10 * 1000, 20 * 1000);
		if (kthread_should_stop()) {
			kfree(_buffer);
			audio_pr(AUDIO_INFO, "[%s]thread called stop!!\n", __func__);
			break;
		}
	}
#if 0//small test for pcm read
	int ret = 0;
	int cnt = 10;

	while (1) {
		if (!snd_pcm_ssp_rdy()) {
			//pcm open not success yet
			msleep_time = 100;
			goto KSSP_INPUT_TASK_WAIT;
		} else
			msleep_time = 0;

		ret = ssp_pcm_read(_buffer, AUDIO_SSP_SINGLE_CHUNK_SAMPLES);
		if (ret < 0) {
			audio_pr(AUDIO_ERR, "ssp_pcm_read < 0\n");
			if (cnt == 0)
				goto KSSP_INPUT_TASK_ERROR;
			else {
				cnt -= 1;
				continue;
			}
		} else {
			audio_pr(AUDIO_INFO, "ssp_pcm_read success-->[ret:%d]\n",
				ret);
		}
KSSP_INPUT_TASK_WAIT:
		if (msleep_time)
			msleep(msleep_time);
		else
			usleep_range(10 * 1000, 20 * 1000);
	}

#endif

	kfree(_buffer);
	return 0;
KSSP_INPUT_TASK_ERROR:
	audio_pr(AUDIO_ERR, "input_task bind pcm error!!!\n");
	kfree(_buffer);
	return -1;
}

#endif

static int kssp_output_task(void *arg)
{
	int *d = (int *)arg;
	//unsigned long flags;

	d = d;
	while (1) {

		if (valid_isr_from_rtos)  {
			ST_SSP_RTOS_INDICATOR *_pindicator = gstSspIndicatorAddr.pindicator;
			unsigned char valid_target_cnt;
			unsigned char cur_pt, cur_cnt;
			ST_SSP_BUFTBL	*pbuffertbl;

			if (!_pindicator) {
				audio_pr(AUDIO_ERR, "NULL pt fatal error\n");
				break;
			}

			if (!gstSspBufTblAddr.pbuffertbl) {
				audio_pr(AUDIO_ERR, "NULL pt fatal error\n");
				break;
			}
			pbuffertbl = &gstSspBufTblAddr.pbuffertbl[0];
			valid_target_cnt = _pindicator->chunks_number;
			cur_pt = _pindicator->Ppt_index;
			cur_cnt = 0;
			while (cur_cnt < valid_target_cnt) {
				if (pbuffertbl[cur_pt].bBufOccupy != AUDIO_BUF_TBL_AFTER_SSP) {
					audio_pr(AUDIO_ERR, "ISR get when index[%d] not process SSP\n", cur_pt);
					break;
				}
				cur_cnt += 1;
				if (cur_pt == 0)
					cur_pt = (AUDIO_SSP_CHUNK_NUMBERS - 1);//index_max = number_max - 1
				else
					cur_pt -= 1;
			}

			#if 0
			spin_lock_irqsave(&lock, flags);
			base_ion_cache_flush(gstSspIndicatorAddr.indicatorPhy,
				gstSspIndicatorAddr.pindicator,
				sizeof(ST_SSP_RTOS_INDICATOR));
			base_ion_cache_flush(gstSspBufTblAddr.buffertblPhy,
				gstSspBufTblAddr.pbuffertbl,
				sizeof(ST_SSP_BUFTBL)*AUDIO_SSP_CHUNK_NUMBERS);
			spin_unlock_irqrestore(&lock, flags);
			#endif
			valid_isr_from_rtos = 0;

		}
		//udelay(1500);->stock the ioctl
		//msleep(5); -> will force sleep atleast 20ms, idealy 5ms
		usleep_range(10 * 1000, 20 * 1000);
		if (kthread_should_stop()) {
			audio_pr(AUDIO_INFO, "[%s] thread called stop!\n", __func__);
			break;
		}
	}
	return 0;
}


static int kunit_test(void *arg);
static struct task_struct *brook_tsk;
static int data;
static int kunit_test(void *arg)
{
	int *d = (int *)arg;
	unsigned long flags;
	unsigned char index = 0;
	unsigned long long offset = 0;

	d = d;
	offset = offset;

	while (1) {
		if (valid_write_file) {
			ST_SSP_BUFTBL *pbuffertbl = (ST_SSP_BUFTBL *)pSspRtosUnitTesCfg->buffertbl;
			char *pbuffer = (char *)pSspRtosUnitTesCfg->poutput_vir;

			audio_pr(AUDIO_INFO, "[%s]valid cnt correct!!\n", __func__);
			valid_write_file = 0;
			if (!pSspRtosUnitTesCfg->poutput_vir) {
				audio_pr(AUDIO_ERR, "[%s]error in[%d]\n", __func__, __LINE__);
				goto AUDIO_IRQ_ERR_AND_LEAVE;
			} else {
				audio_pr(AUDIO_DBG, "[%s]output addr[0x"PRT_PT"]\n", __func__,
					 (TYPE_PT)pSspRtosUnitTesCfg->poutput_vir);
			}

#ifdef AUDIO_SAVE_FILE
#if (KERNEL_VERSION(5, 1, 0) <= LINUX_VERSION_CODE)
			kernel_write(faudio_ut, pbuffer, AUDIO_MAX_BUFFER_SIZE, &offset);
#else
			writeFile(faudio_ut, pbuffer, AUDIO_MAX_BUFFER_SIZE);
#endif
#endif
			spin_lock_irqsave(&lock, flags);
			for (index = 0; index < AUDIO_SSP_CHUNK_NUMBERS; index++) {
				pbuffertbl[index].bBufOccupy = 1;
			}
			base_ion_cache_flush(SspRtosUnitTesCfgPhy,
					pSspRtosUnitTesCfg,
					sizeof(ST_SSP_RTOS_INIT));
			spin_unlock_irqrestore(&lock, flags);
			valid_write_file = 0;
		}
		udelay(1000);
		if (kthread_should_stop()) {
			audio_pr(AUDIO_INFO, "kunit_test thread called stop!\n");
			break;
		}
	}
	if (!faudio_ut)
		filp_close(faudio_ut, NULL);

	audio_pr(AUDIO_INFO, "[%s]leaving\n", __func__);
AUDIO_IRQ_ERR_AND_LEAVE:
	audio_pr(AUDIO_INFO, "[%s]leaving\n", __func__);
	return 0;
}

static struct task_struct *ssp_input_tsk;
static struct task_struct *ssp_output_tsk;
static struct task_struct *pcm_thread;//this should replace ssp_input_tsk
static int input_data;//for dummy
static int output_data;//for dummy

// setup the callback handler for free rtos, proc mode
static void audio_rtos_irq_handler(void *ptr, void *dev_id)
{
	audio_pr(AUDIO_INFO,
		 "[kernel_mode]rtos_irq_handler indicator[0x%llx] buf[0x%llx]\n",
		 gstSspIndicatorAddr.indicatorPhy,
		 gstSspBufTblAddr.buffertblPhy);
	if (!_audio_invalidate_ssp_output_data()) {
		audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
			__func__, __LINE__);
	}

	if (!_audio_invalidate_ssp_indicator_and_tbl()) {
		audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
			__func__, __LINE__);
	}

	valid_isr_from_rtos = 1;
}
// setup the callback handler for free_rtos, unit test mode
static void audio_rtos_irq_unit_test_handler(void *ptr, void *dev_id)
{
#ifdef AUDIO_SAVE_FILE
	unsigned char valid_cnt = 0;
	unsigned char index = 0;
	// unsigned long flags = 0;
	ST_SSP_RTOS_SPK_DATA_RET *pSpkSspRtosData_Ret = (ST_SSP_RTOS_SPK_DATA_RET *)pSspRtosUnitTesCfg->CbVirAddr;
	ST_SSP_BUFTBL *pbuffertbl = (ST_SSP_BUFTBL *)pSspRtosUnitTesCfg->buffertbl;
	// flags = flags;

	audio_pr(AUDIO_INFO,
		 "[kernel_mode]base_ion_cache_invalidate[0x%llx]size[%d]\n",
		 SspRtosUnitTesCfgPhy,
		 (uint32_t)sizeof(ST_SSP_RTOS_INIT));

	base_ion_cache_invalidate(SspRtosUnitTesCfgPhy,
				pSspRtosUnitTesCfg,
				sizeof(ST_SSP_RTOS_INIT));
	if (faudio_ut == NULL) {
		audio_pr(AUDIO_ERR, "[irq]]faudio_ut is NULL....do nothing !!!\n");
		goto AUDIO_IRQ_ERR_AND_LEAVE;
	}

	if (pSspRtosUnitTesCfg == NULL) {
		audio_pr(AUDIO_ERR, "global handler has something wrong!! force leave\n");
		goto AUDIO_IRQ_ERR_AND_LEAVE;
	}

	if (strcmp((const char *)dev_id, "UNIT_TEST") == 0)
		audio_pr(AUDIO_INFO, "[irq]catch phrase UNIT_TEST\n");

	if (pSpkSspRtosData_Ret->cb_command == AUDIO_RTOS_CMD_SSP_UNIT_TEST) {
		audio_pr(AUDIO_INFO, "[irq]pSpkSspRtosData_Ret->status[%d]\n", pSpkSspRtosData_Ret->status);
	}

	audio_pr(AUDIO_DBG, "pbuffertbl vir address[0x"PRT_PT"]\n",
		 (TYPE_PT)pSspRtosUnitTesCfg->buffertbl);
	if (pbuffertbl == NULL || pSspRtosUnitTesCfg->buffertbl[0].mic_in_addr == 0) {
		audio_pr(AUDIO_ERR, "ssp buffer has something wrong in irq..force leave\n");
		goto AUDIO_IRQ_ERR_AND_LEAVE;
	}
	for (index = 0; index < AUDIO_SSP_CHUNK_NUMBERS; index++) {
		if (pbuffertbl[index].bBufOccupy == 0)
			valid_cnt += 1;
		else
			audio_pr(AUDIO_INFO, "valid cnt index xx[%d]\n", index);
	}
	if (valid_cnt == AUDIO_SSP_CHUNK_NUMBERS) {
		audio_pr(AUDIO_DBG, "enter[%s][%d]valid\n", __func__, __LINE__);
		valid_write_file = 1;
	} else {
		audio_pr(AUDIO_ERR, "[%s]valid cnt incorrect force leave..not saving file\n", __func__);
		goto AUDIO_IRQ_ERR_AND_LEAVE;
	}
#else
	audio_pr(AUDIO_INFO, "[%s]Not save file for unit test mode\n");
#endif

	return;
AUDIO_IRQ_ERR_AND_LEAVE:
	audio_pr(AUDIO_ERR, "[%s]handler error or valid cnt mismatch[[0x%llx]\n",
		__func__, SspRtosUnitTesCfgPhy);
	kthread_stop(brook_tsk);
	return;

}

long audio_device_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	int ret = 0;
	cmdqu_t rtos_cmdq;
	ST_SSP_MSG stSspMsg;
	ST_SSP_DATA_MSG stSspDataMsg;
	unsigned long flags;

	switch (cmd) {
	case AUDIO_INIT_CHECK:
	{
		s32 s32Counter = gst_audio_status.counter;

		//pr_err("AUDIO_INIT_CHECK\n");
		pr_err("AUDIO_INIT_CHECK counter : [%d]\n", gst_audio_status.counter);
		ret = copy_to_user((s32 *)arg, &s32Counter,
				   sizeof(s32));
	}
	break;
	case AUDIO_GET_MMAP_SIZE:
	{
		s32 s32Tmp = (s32)AUDIO_SHARE_MEM_SIZE;

		ret = copy_to_user((s32 *)arg, &s32Tmp,
				   sizeof(s32));
	}
	break;
	case AUDIO_AENC_SHAREBUFFER_INIT:
	{
		ST_AENC_MSG stAencMsg;
		ST_AENC_MEM_CTX *pAencMemCtx;
		ST_AENC_CHN_MEM_CTX *pAencChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_AENC_SHAREBUFFER_INIT\n");

		pAencMemCtx = &stAencMemCtx;
		pAencChnMemCtx = &pAencMemCtx->chn_mem_ctx[stAencMsg.AenChn];

		if (pAencChnMemCtx->sharebuf_index == 0) {
			pAencChnMemCtx->sharebuf_index = _get_avail_sharebuf_index();
			pAencChnMemCtx->sharebuf_first = 1;
		} else {
			audio_pr(AUDIO_INFO, "AUDIO_AENC_SHAREBUFFER_INIT chn[%d] index[%d]\n",
				 stAencMsg.AenChn,
				 pAencChnMemCtx->sharebuf_index);
			pAencChnMemCtx->sharebuf_first = 0;
		}
		stAencMsg.sharebuf_index = pAencChnMemCtx->sharebuf_index;
		stAencMsg.bsharebuf_first = pAencChnMemCtx->sharebuf_first;
		if (copy_to_user((void __user *)arg, &stAencMsg, sizeof(ST_AENC_MSG)) != 0)
		{
			audio_pr(AUDIO_ERR, "AUDIO_AENC_SHAREBUFFER_INIT cmd to user failure\n");
			break;
		}
	}
	break;
	case AUDIO_ADEC_SHAREBUFFER_INIT:
	{
		ST_ADEC_MSG stAdecMsg;
		ST_ADEC_MEM_CTX *pAdecMemCtx;
		ST_ADEC_CHN_MEM_CTX *pAdecChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_ADEC_SHAREBUFFER_INIT\n");
		if (copy_from_user(&stAdecMsg, (void __user *)arg, sizeof(ST_ADEC_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_ADEC_SHAREBUFFER_INIT cmd failure\n");
			break;
		}

		pAdecMemCtx = &stAdecMemCtx;
		pAdecChnMemCtx = &stAdecMemCtx.chn_mem_ctx[stAdecMsg.AdecChn];

		if (pAdecChnMemCtx->sharebuf_index == 0) {
			pAdecChnMemCtx->sharebuf_index = _get_avail_sharebuf_index();
			pAdecChnMemCtx->sharebuf_first = 1;
		} else {
			audio_pr(AUDIO_INFO, "AUDIO_ADEC_SHAREBUFFER_INIT chn[%d] index[%d]\n",
				 stAdecMsg.AdecChn,
				 pAdecChnMemCtx->sharebuf_index);
			pAdecChnMemCtx->sharebuf_first = 0;
		}
		stAdecMsg.bsharebuf_first = pAdecChnMemCtx->sharebuf_first;
		stAdecMsg.sharebuf_index = pAdecChnMemCtx->sharebuf_index;
		if (copy_to_user((void __user *)arg, &stAdecMsg, sizeof(ST_ADEC_MSG)) != 0)
		{
			audio_pr(AUDIO_ERR, "AUDIO_ADEC_SHAREBUFFER_INIT cmd to user failure\n");
			break;
		}
	}
	break;
	case AUDIO_AOUT_SHAREBUFFER_INIT:
	{
		ST_AOUT_MSG stAoutMsg;
		ST_AOUT_MEM_CTX *pAoutMemCtx;
		ST_AOUT_CHN_MEM_CTX *pAoutChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_AOUT_SHAREBUFFER_INIT\n");
		if (copy_from_user(&stAoutMsg, (void __user *)arg, sizeof(ST_AOUT_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AOUT_SHAREBUFFER_INIT cmd failure\n");
			break;
		}

		pAoutMemCtx = &stAoutMemCtx[stAoutMsg.AoDev];
		pAoutChnMemCtx = &pAoutMemCtx->chn_mem_ctx[stAoutMsg.AoChn];

		if (pAoutChnMemCtx->sharebuf_index == 0) {
			pAoutChnMemCtx->sharebuf_index = _get_avail_sharebuf_index();
			pAoutChnMemCtx->sharebuf_first = 1;
		} else {
			audio_pr(AUDIO_INFO, "AUDIO_AOUT_SHREBUFFER_INIT chn[%d] index[%d]\n",
				 stAoutMsg.AoChn, pAoutChnMemCtx->sharebuf_index);
			pAoutChnMemCtx->sharebuf_first = 0;
		}
		stAoutMsg.bsharebuf_first = pAoutChnMemCtx->sharebuf_first;
		stAoutMsg.sharebuf_index = pAoutChnMemCtx->sharebuf_index;
		if (copy_to_user((void __user *)arg, &stAoutMsg, sizeof(ST_AOUT_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AOUT_SHAREBUFFER_INIT cmd to user failure\n");
			break;
		}
	}
	break;
	case AUDIO_ADEC_CHN_BUFFER_ACQUIRE:
	{
		ST_ADEC_MSG stAdecMsg;
		ST_ADEC_MEM_CTX *pAdecMemCtx;
		ST_ADEC_CHN_MEM_CTX *pAdecChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_ADEC_CHN_BUFFER_ACQUIRE\n");
		if (copy_from_user(&stAdecMsg, (void __user *)arg, sizeof(ST_ADEC_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_ADEC_CHN_BUFFER_ACQUIRE cmd failure\n");
			break;
			ret = -1;
		}

		if (stAdecMsg.DecBuff_size == 0 || stAdecMsg.DecReadBuff_size == 0) {
			audio_pr(AUDIO_ERR, "AUDIO_ADEC_CHN_BUFFER_ACQUIRE require size 0\n");
			ret = -1;
			break;
		}

		if (AUDIO_CHECK_CHN_VALID(stAdecMsg.AdecChn) < 0) {
			audio_pr(AUDIO_ERR, "channel id invalid[%d]\n", stAdecMsg.AdecChn);
			break;
		}
		pAdecMemCtx = &stAdecMemCtx;
		pAdecChnMemCtx = &stAdecMemCtx.chn_mem_ctx[stAdecMsg.AdecChn];
		if (pAdecChnMemCtx->DecBuff_size == 0 || pAdecChnMemCtx->DecReadBuff_size == 0) {
			//first allocate
			char *_DecBuff = NULL;
			char *_DecReadBuff = NULL;

			_DecBuff = kzalloc(stAdecMsg.DecBuff_size, GFP_ATOMIC);
			_DecReadBuff = kzalloc(stAdecMsg.DecReadBuff_size, GFP_ATOMIC);
			if (_DecBuff == NULL || _DecReadBuff == NULL) {
				audio_pr(AUDIO_ERR, "Cannot kzalloc buffer for adec channel\n");
				ret = -1;
				break;
			}
			pAdecChnMemCtx->DecBuff_addr = virt_to_phys(_DecBuff);
			pAdecChnMemCtx->DecReadBuff_addr = virt_to_phys(_DecReadBuff);
			pAdecChnMemCtx->DecBuff_size = stAdecMsg.DecBuff_size;
			pAdecChnMemCtx->DecReadBuff_size = stAdecMsg.DecReadBuff_size;
		} else {
			audio_pr(AUDIO_INFO, "alread exist adec channel buffer for chn[%d]\n",
				stAdecMsg.AdecChn);
		}
		stAdecMsg.DecBuff = pAdecChnMemCtx->DecBuff_addr;
		stAdecMsg.DecReadBuff = pAdecChnMemCtx->DecReadBuff_addr;
		if (copy_to_user((void __user *)arg, &stAdecMsg, sizeof(ST_ADEC_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_ADEC_CHN-BUFFER ACQUIRE cmd to user failure\n");
			break;
		}

	}
	break;
	case AUDIO_AENC_CHN_BUFFER_ACQUIRE:
	{
		ST_AENC_MSG stAencMsg;
		ST_AENC_MEM_CTX *pAencMemCtx;
		ST_AENC_CHN_MEM_CTX *pAencChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_AENC_CHN_BUFFER_ACQUIRE\n");
		if (copy_from_user(&stAencMsg, (void __user *)arg, sizeof(ST_AENC_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AENC_CHN_BUFFER_ACQUIRE cmd failure\n");
			ret = -1;
			break;
		}

		if (stAencMsg.EncBuff_aec_size == 0 || stAencMsg.EncBuff_size == 0 ||
		    stAencMsg.EncInBuff_size == 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AENC_CHN_BUFFER_ACQUIRE require 0 size[%d][%d][%d]\n",
				 stAencMsg.EncBuff_aec_size, stAencMsg.EncBuff_size, stAencMsg.EncInBuff_size);
			ret = -1;
			break;
		}

		if (AUDIO_CHECK_CHN_VALID(stAencMsg.AenChn) < 0) {
			audio_pr(AUDIO_ERR, "channel id invalid[%d]\n", stAencMsg.AenChn);
			break;
		}
		pAencMemCtx = &stAencMemCtx;
		pAencChnMemCtx = &pAencMemCtx->chn_mem_ctx[stAencMsg.AenChn];
		if (pAencChnMemCtx->EncBuff_aec_size == 0 || pAencChnMemCtx->EncBuff_size == 0 ||
		    pAencChnMemCtx->EncInBuff_size == 0) {
			char *_EncBuff_aec = NULL;
			char *_EncBuff = NULL;
			char *_EncInBuff = NULL;
			char *_EncVqeBuff = NULL;

			_EncBuff_aec = kzalloc(stAencMsg.EncBuff_aec_size, GFP_ATOMIC);
			_EncBuff = kzalloc(stAencMsg.EncBuff_size, GFP_ATOMIC);
			_EncInBuff = kzalloc(stAencMsg.EncInBuff_size, GFP_ATOMIC);
			_EncVqeBuff = kzalloc(stAencMsg.EncVqeBuff_size, GFP_ATOMIC);
			if (_EncBuff_aec == NULL || _EncBuff == NULL || _EncInBuff == NULL) {
				audio_pr(AUDIO_ERR, "Cannot kzalloc buffer for aenc channel\n");
				ret = -1;
				break;
			}

			pAencChnMemCtx->EncInBuff_addr = virt_to_phys(_EncInBuff);
			pAencChnMemCtx->EncBuff_addr = virt_to_phys(_EncBuff);
			pAencChnMemCtx->EncBuff_aec_addr = virt_to_phys(_EncBuff_aec);
			pAencChnMemCtx->EncVqeBuff_addr = virt_to_phys(_EncVqeBuff);
			pAencChnMemCtx->EncBuff_aec_size = stAencMsg.EncBuff_aec_size;
			pAencChnMemCtx->EncBuff_size = stAencMsg.EncBuff_size;
			pAencChnMemCtx->EncInBuff_size = stAencMsg.EncInBuff_size;
			pAencChnMemCtx->EncVqeBuff_size = stAencMsg.EncVqeBuff_size;
		} else {
			audio_pr(AUDIO_INFO, "already exist aenc channel buffer for chn[%d]\n",
				 stAencMsg.AenChn);
		}

		stAencMsg.EncBuff = pAencChnMemCtx->EncBuff_addr;
		stAencMsg.EncBuff_aec = pAencChnMemCtx->EncBuff_aec_addr;
		stAencMsg.EncInBuff = pAencChnMemCtx->EncInBuff_addr;
		stAencMsg.EncVqeBuff = pAencChnMemCtx->EncVqeBuff_addr;
		if (copy_to_user((void __user *)arg, &stAencMsg, sizeof(ST_AENC_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AENC_CHN_BUFFER_ACQUIRE cmd to user failure\n");
			break;
		}

	}
	break;
	case AUDIO_AIN_SHAREBUFFER_INIT:
	{
		ST_AIN_MSG stAinMsg;
		ST_AIN_MEM_CTX *pAinMemCtx;
		ST_AIN_CHN_MEM_CTX *pAinChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_AIN_SHAREBUFFER_INIT\n");
		if (copy_from_user(&stAinMsg, (void __user *)arg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_SHAREBUFFER_INIT cmd failure\n");
			break;
		}

		pAinMemCtx = &stAinMemCtx[stAinMsg.AiDev];
		pAinChnMemCtx = &pAinMemCtx->chn_mem_ctx[stAinMsg.AiChnId];

		if (pAinChnMemCtx->sharebuf_index == 0) {
			// first one trigger
			pAinChnMemCtx->sharebuf_index = _get_avail_sharebuf_index();
			pAinChnMemCtx->sharebuf_first = 1;
		} else {
			// not the first trigger for this buffer in specific chn
			audio_pr(AUDIO_INFO, "AUDIO_AIN_SHAREBUFFER_INIT chn[%d] index[%d]\n",
				 stAinMsg.AiChnId,
				 pAinChnMemCtx->sharebuf_index);
			pAinChnMemCtx->sharebuf_first = 0;
		}
		stAinMsg.bsharebuf_first = pAinChnMemCtx->sharebuf_first;
		stAinMsg.sharebuf_index = pAinChnMemCtx->sharebuf_index;
		if (copy_to_user((void __user *)arg, &stAinMsg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_SHAREBUFFER_INIT cmd to user failure\n");
			break;
		}
	}
	break;
	case AUDIO_AIN_RESAMPLE_HANDLER_ATTACH:
	{
		ST_AIN_MSG stAinMsg;
		ST_AIN_MEM_CTX *pAinMemCtx;
		ST_AIN_CHN_MEM_CTX *pAinChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_AIN_RESAMPLE_HANDLER_ATTACH\n");
		if (copy_from_user(&stAinMsg, (void __user *)arg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_RESAMPLE_HANDLER_ATTACH cmd failure\n");
			ret = -1;
			break;
		}

		pAinMemCtx = &stAinMemCtx[stAinMsg.AiDev];
		pAinChnMemCtx = &pAinMemCtx->chn_mem_ctx[stAinMsg.AiChnId];
		if (pAinChnMemCtx->resample_handler_size == 0 || pAinChnMemCtx->resample_handler_addr == 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_RESAMPLE_HANDLER_ATTACH not exist!!!\n");
			stAinMsg.resample_handler_addr = stAinMsg.resample_handler_size = 0;
		}

		stAinMsg.resample_handler_addr = pAinChnMemCtx->resample_handler_addr;
		stAinMsg.resample_handler_size = pAinChnMemCtx->resample_handler_size;
		if (copy_to_user((void __user *)arg, &stAinMsg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_RESAMPLE_HANDLER_ATTACH cmd to user failure\n");
			break;
		}
	}
	break;
	case AUDIO_AOUT_RESAMPLE_HANDLER_ACQUIRE:
	{
		ST_AOUT_MSG stAoutMsg;
		ST_AOUT_MEM_CTX *pAoutMemCtx;
		ST_AOUT_CHN_MEM_CTX *pAoutChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_AOUT_RESAMPLE_HANDLER_ACQUIRE\n");
		if (copy_from_user(&stAoutMsg, (void __user *)arg, sizeof(ST_AOUT_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AOUT_RESAMPLE_HANDLER_ACQUIRE cmd failure\n");
			ret = -1;
			break;
		}

		if (stAoutMsg.resample_handler_size == 0) {
			audio_pr(AUDIO_ERR, "require 0 size for aout resample handler\n");
			ret = -1;
			break;
		}
		pAoutChnMemCtx = &pAoutMemCtx->chn_mem_ctx[stAoutMsg.AoChn];
		if (pAoutChnMemCtx->resample_handler_size == 0 || pAoutChnMemCtx->resample_handler_addr == 0) {
			char *pBuf = NULL;

			pBuf = kzalloc(stAoutMsg.resample_handler_size, GFP_ATOMIC);
			memcpy(pBuf, &stAoutMsg.ResInfo, sizeof(RESAMPLE_INFO));
			pAoutChnMemCtx->resample_handler_addr = virt_to_phys(pBuf);
			pAoutChnMemCtx->resample_handler_size = stAoutMsg.resample_handler_size;
		}
		stAoutMsg.resample_handler_addr = pAoutChnMemCtx->resample_handler_addr;
		if (copy_to_user((void __user *)arg, &stAoutMsg, sizeof(ST_AOUT_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_RESAMPLE_HANDLER_ACQUIRE cmd to user failure\n");
			break;
		}
	}
	break;
	case AUDIO_AIN_RESAMPLE_HANDLER_ACQUIRE:
	{
		ST_AIN_MSG stAinMsg;
		ST_AIN_MEM_CTX *pAinMemCtx;
		ST_AIN_CHN_MEM_CTX *pAinChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_AIN_RESAMPLE_HANDLER_ACQUIRE\n");
		if (copy_from_user(&stAinMsg, (void __user *)arg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_CHN_BUFFER_ACQUIRE cmd failure\n");
			ret = -1;
			break;
		}

		if (stAinMsg.resample_handler_size == 0) {
			audio_pr(AUDIO_ERR, "require 0 size for ain resample handler\n");
			ret = -1;
			break;
		}
		pAinMemCtx = &stAinMemCtx[stAinMsg.AiDev];
		pAinChnMemCtx = &pAinMemCtx->chn_mem_ctx[stAinMsg.AiChnId];
		if (pAinChnMemCtx->resample_handler_size == 0 || pAinChnMemCtx->resample_handler_addr == 0) {
			char *pBuf = NULL;

			pBuf = kzalloc(stAinMsg.resample_handler_size, GFP_ATOMIC);
			memcpy(pBuf, &stAinMsg.ResInfo, sizeof(RESAMPLE_INFO));
			pAinChnMemCtx->resample_handler_addr = virt_to_phys(pBuf);
			pAinChnMemCtx->resample_handler_size = stAinMsg.resample_handler_size;
		}
		stAinMsg.resample_handler_addr = pAinChnMemCtx->resample_handler_addr;
		if (copy_to_user((void __user *)arg, &stAinMsg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_RESAMPLE_HANDLER_ACQUIRE cmd to user failure\n");
			break;
		}
	}
	break;
	case AUDIO_AIN_CHN_BUFFER_ACQUIRE:
	{
		ST_AIN_MSG stAinMsg;
		ST_AIN_MEM_CTX *pAinMemCtx;
		ST_AIN_CHN_MEM_CTX *pAinChnMemCtx;

		audio_pr(AUDIO_INFO, "AUDIO_AIN_CHN_BUFFER_ACQUIRE\n");
		if (copy_from_user(&stAinMsg, (void __user *)arg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_CHN_BUFFER_ACQUIRE cmd failure\n");
			ret = -1;
			break;
		}
		if (stAinMsg.vqebuf_size == 0 || stAinMsg.cycbufread_size == 0) {
			audio_pr(AUDIO_ERR, "require 0 size, abnormal!!\n");
			ret = -1;
			break;
		}
		pAinMemCtx = &stAinMemCtx[stAinMsg.AiDev];
		pAinChnMemCtx = &pAinMemCtx->chn_mem_ctx[stAinMsg.AiChnId];
		if (pAinChnMemCtx->cycbufread_size == 0 || pAinChnMemCtx->vqebuf_size == 0) {
			char *pcycread_buf = NULL;
			char *pvqe_buf = NULL;

			pcycread_buf = kzalloc(stAinMsg.cycbufread_size, GFP_ATOMIC);
			pvqe_buf = kzalloc(stAinMsg.vqebuf_size, GFP_ATOMIC);
			if (pcycread_buf == NULL || pvqe_buf == NULL) {
				audio_pr(AUDIO_ERR, "Cannot kzalloc cycle read / vqe buffer\n");
				ret = -1;
				break;
			}
			pAinChnMemCtx->vqebuf_size = pAinChnMemCtx->vqebuf_size;
			pAinChnMemCtx->cycbufread_size = stAinMsg.cycbufread_size;
			pAinChnMemCtx->cycbufread_addr = virt_to_phys(pcycread_buf);
			pAinChnMemCtx->vqebuff_addr = virt_to_phys(pvqe_buf);
		} else {
			if ((pAinChnMemCtx->cycbufread_size > 0 &&
			     stAinMsg.cycbufread_size > pAinChnMemCtx->cycbufread_size) ||
			    (pAinChnMemCtx->vqebuf_size > 0 &&
			     stAinMsg.vqebuf_size > pAinChnMemCtx->vqebuf_size)) {
				char *pcycread_buf = NULL;
				char *pvqe_buf = NULL;

				pcycread_buf = kzalloc(stAinMsg.cycbufread_size, GFP_ATOMIC);
				pvqe_buf = kzalloc(stAinMsg.vqebuf_size, GFP_ATOMIC);
				if (pcycread_buf == NULL || pvqe_buf == NULL) {
					audio_pr(AUDIO_ERR, "Cannot kzalloc cycle read / vqe buffer\n");
					ret = -1;
					break;
				}
				kfree(phys_to_virt(pAinChnMemCtx->cycbufread_addr));
				kfree(phys_to_virt(pAinChnMemCtx->vqebuff_addr));
				pAinChnMemCtx->cycbufread_addr = virt_to_phys(pcycread_buf);
				pAinChnMemCtx->vqebuff_addr = virt_to_phys(pvqe_buf);
				pAinChnMemCtx->cycbufread_size = stAinMsg.cycbufread_size;
				pAinChnMemCtx->vqebuf_size = stAinMsg.vqebuf_size;
			} else {
				audio_pr(AUDIO_INFO, "already exist ain channel buffer for dev[%d] chn[%d]\n",
					 stAinMsg.AiDev, stAinMsg.AiChnId);
			}
		}
		stAinMsg.cycbufread_size = pAinChnMemCtx->cycbufread_size;
		stAinMsg.vqebuf_size = pAinChnMemCtx->vqebuf_size;
		stAinMsg.vqebuf_addr = pAinChnMemCtx->vqebuff_addr;
		stAinMsg.cycbufread_addr = pAinChnMemCtx->cycbufread_addr;
		if (copy_to_user((void __user *)arg, &stAinMsg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_CHN_BUFFER_ACQUIRE cmd to user failure\n");
			break;
		}
	}
	break;
	case AUDIO_AIN_ALSA_READ_BUFFER_ACQUIRE:
	{
		ST_AIN_MSG stAinMsg;
		ST_AIN_MEM_CTX *pAinMemCtx;
		int required_size = 0;
		char *pBuf = NULL;

		//pr_err("AUDIO_AIN_ALSA_READ_BUFFER_ACQUIRE\n");
		audio_pr(AUDIO_INFO, "AUDIO_AIN_ALSA_READ_BUFFER_ACQUIRE\n");
		if (copy_from_user(&stAinMsg, (void __user *)arg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_CYCLEBUFFER_INIT cmd failure\n");
			break;
		}

		if (stAinMsg.tinyalsaReadBuf_size == 0) {
			audio_pr(AUDIO_ERR, "require size = 0\n");
			break;

		} else
			required_size = stAinMsg.tinyalsaReadBuf_size;

		pAinMemCtx = &stAinMemCtx[stAinMsg.AiDev];

		if (pAinMemCtx->tinyalsaReadBuf_size == 0 || pAinMemCtx->tinyalsaReadBuf_addr == 0) {
			pr_err("[%s][%d]\n", __func__, __LINE__);
			pBuf = kzalloc(required_size, GFP_ATOMIC);
			if (pBuf == NULL) {
				audio_pr(AUDIO_ERR, "Cannot kzalloc pcm read buffer\n");
				ret = -1;
				break;
			}
			pAinMemCtx->tinyalsaReadBuf_size = required_size;
			pAinMemCtx->tinyalsaReadBuf_addr = virt_to_phys(pBuf);
		} else {
			if (pAinMemCtx->tinyalsaReadBuf_size > 0 &&
			    pAinMemCtx->tinyalsaReadBuf_size < required_size &&
			    pAinMemCtx->tinyalsaReadBuf_addr != 0) {
				kfree(phys_to_virt(pAinMemCtx->tinyalsaReadBuf_addr));
				pBuf = kzalloc(required_size, GFP_ATOMIC);
				if (pBuf == NULL) {
					audio_pr(AUDIO_ERR, "Cannot kzalloc pcm read buffer\n");
					ret = -1;
					break;
				}
				pAinMemCtx->tinyalsaReadBuf_size = required_size;
				pAinMemCtx->tinyalsaReadBuf_addr = virt_to_phys(pBuf);
			} else {
				audio_pr(AUDIO_INFO, "already exist pcm read buffer for dev[%d] size[%d]\n",
					 stAinMsg.AiDev, pAinMemCtx->tinyalsaReadBuf_size);
			}
		}
		stAinMsg.tinyalsaReadBuf_size = required_size;
		stAinMsg.tinyalsaReadBuf = pAinMemCtx->tinyalsaReadBuf_addr;
		if (copy_to_user((void __user *)arg, &stAinMsg, sizeof(ST_AIN_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_AIN_ALSA_READ_BUFFER_ACQUIRE cmd to user failure\n");
			ret = -1;
			break;
		}
	}
	break;
	case AUDIO_IOCTL_SSP_UPDATE_PCM_RECORD_CFG:
	{
		ST_SSP_PCM_MSG stSspPcmMsg;

		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_UPDATE_PCM_RECORD_CFG\n");
		if (copy_from_user(&stSspPcmMsg, (void __user *)arg, sizeof(ST_SSP_PCM_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_IOCTL_SSP_UPDATE_PCM_RECORD_CFG cmd failure\n");
			break;
		}
		audio_pr(AUDIO_INFO, "stSspPcmMsg.channels[%d] rate[%d] period_size[%d]\n",
				stSspPcmMsg.channels, stSspPcmMsg.rate, stSspPcmMsg.period_size);

		mInputConfig.channels = stSspPcmMsg.channels;
		mInputConfig.period_size = stSspPcmMsg.period_size;
		mInputConfig.rate = stSspPcmMsg.rate;
		if (copy_to_user((void __user *)arg, &stSspPcmMsg, sizeof(ST_SSP_PCM_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_IOCTL_SSP_UPDATE_PCM_RECORD_CFG to user failure\n");
			break;
		}
	}
	break;
	case AUDIO_IOCTL_SSP_INIT:
	{
		long ret = 0;
		int index = 0;
		AI_TALKVQE_CONFIG_S *pAinVqeCfg;
		AI_TALKVQE_CONFIG_S_RTOS *pAinVqeCfgRtos;

		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_INIT\n");
		//step 1: check the audio_ ion allocate for rtos or not
		if ((gstSspVqeAddr.AinVqeCfgPhy == 0) ||
		    (gstSspIndicatorAddr.indicatorPhy == 0) ||
		    (gstSspBufTblAddr.buffertblPhy == 0)) {
			audio_pr(AUDIO_ERR, "Allocate AUDIO_IOCTL_SSP_INIT not yet having address !!\n");
			break;

		} else
			audio_pr(AUDIO_INFO, "[Warning] SSP_INIT phy init...[0x%llx][]0x%llx][0x%llx]!!\n",
					gstSspVqeAddr.AinVqeCfgPhy,
					gstSspIndicatorAddr.indicatorPhy,
					gstSspBufTblAddr.buffertblPhy);
		if (!gpstaudioMailBox) {
			audio_pr(AUDIO_ERR, "Mailbox memory not ready ...force leave!!!\n");
			break;
		}

		//step 2: get the VQE information from userspace and dump out to check the correctness
		if (copy_from_user(&stSspMsg, (void __user *)arg, sizeof(ST_SSP_MSG)) != 0) {
			audio_pr(AUDIO_ERR, "AUDIO_IOCTL_SSP_INIT cmd failure\n");
			break;
		}

		//step 3: fill in the correspondent phy memory, and reset the buffer tbl
		gpstaudioMailBox->AinVqeCfgPhy = gstSspVqeAddr.AinVqeCfgPhy;
		gpstaudioMailBox->buffertblPhy = gstSspBufTblAddr.buffertblPhy;
		gpstaudioMailBox->indicatorPhy = gstSspIndicatorAddr.indicatorPhy;
		gpstaudioMailBox->u64RevMask = AUDIO_RTOS_MAGIC_WORD_KERNEL_BIND_MODE;
		for (index = 0; index < AUDIO_SSP_CHUNK_NUMBERS; index++) {
			if (stSspMsg.channel_cnt == 1) {
				//pcm data only have left channel(mic_in). Certainly do not have ref data
				gstSspBufTblAddr.pbuffertbl[index].mic_in_addr =
					gstSspBufTblAddr.mic_in_phy +
					(index * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
				gstSspBufTblAddr.pbuffertbl[index].ref_in_addr =
					gstSspBufTblAddr.pbuffertbl[0].mic_in_addr +
					AUDIO_MAX_BUFFER_SIZE + (index * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
			} else {
				//mic_in, ref_in ->align the input of pcm left/right/left/right/left/right data
				gstSspBufTblAddr.pbuffertbl[index].mic_in_addr =
					gstSspBufTblAddr.mic_in_phy +
					(index * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
				gstSspBufTblAddr.pbuffertbl[index].ref_in_addr =
					gstSspBufTblAddr.pbuffertbl[0].mic_in_addr +
					AUDIO_MAX_BUFFER_SIZE + (index * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
			}

			gstSspBufTblAddr.pbuffertbl[index].output_addr =
				gstSspBufTblAddr.output_phy +
				index * (AUDIO_MAX_BUFFER_SIZE / AUDIO_SSP_CHUNK_NUMBERS);
			gstSspBufTblAddr.pbuffertbl[index].bBufOccupy = AUDIO_BUF_TBL_UNOCCUPIED;
		}
		//memcpy(gstSspVqeAddr.pAinVqeCfg, &stSspMsg.stVqeConfig, sizeof(AI_TALKVQE_CONFIG_S));
		//from user space
		pAinVqeCfg = (AI_TALKVQE_CONFIG_S *)&stSspMsg.stVqeConfig;
		//to rtos space parameters alignment
		pAinVqeCfgRtos = (AI_TALKVQE_CONFIG_S_RTOS *)gstSspVqeAddr.pAinVqeCfg;

		pAinVqeCfgRtos->u32OpenMask = pAinVqeCfg->u32OpenMask;
		pAinVqeCfgRtos->s32WorkSampleRate = pAinVqeCfg->s32WorkSampleRate;
		pAinVqeCfgRtos->stAecCfg.para_aec_filter_len = pAinVqeCfg->stAecCfg.para_aec_filter_len;
		pAinVqeCfgRtos->stAecCfg.para_aes_std_thrd = pAinVqeCfg->stAecCfg.para_aes_std_thrd;
		pAinVqeCfgRtos->stAecCfg.para_aes_supp_coeff = pAinVqeCfg->stAecCfg.para_aes_supp_coeff;
		pAinVqeCfgRtos->stAnrCfg.para_nr_snr_coeff = pAinVqeCfg->stAnrCfg.para_nr_snr_coeff;
		pAinVqeCfgRtos->stAnrCfg.para_nr_init_sile_time = pAinVqeCfg->stAnrCfg.para_nr_init_sile_time;
		pAinVqeCfgRtos->stAgcCfg.para_agc_max_gain = pAinVqeCfg->stAgcCfg.para_agc_max_gain;

		pAinVqeCfgRtos->stAgcCfg.para_agc_target_high = pAinVqeCfg->stAgcCfg.para_agc_target_high;
		pAinVqeCfgRtos->stAgcCfg.para_agc_target_low = pAinVqeCfg->stAgcCfg.para_agc_target_low;
		pAinVqeCfgRtos->stAgcCfg.para_agc_vad_ena = pAinVqeCfg->stAgcCfg.para_agc_vad_ena;
		pAinVqeCfgRtos->stAecDelayCfg.para_aec_init_filter_len =
			pAinVqeCfg->stAecDelayCfg.para_aec_init_filter_len;
		pAinVqeCfgRtos->stAecDelayCfg.para_dg_target = pAinVqeCfg->stAecDelayCfg.para_dg_target;
		pAinVqeCfgRtos->stAecDelayCfg.para_delay_sample = pAinVqeCfg->stAecDelayCfg.para_delay_sample;
		pAinVqeCfgRtos->para_notch_freq = pAinVqeCfg->para_notch_freq;

#ifdef VQE_DEBUG
	//debug for parameter correctness
		audio_pr(AUDIO_INFO,
			"Phy[0X%llx]vir add_gstSspVqeAddr.pAinVqeCfg[0x"PRT_PT"]\n",
			gstSspVqeAddr.AinVqeCfgPhy,
			(TYPE_PT)gstSspVqeAddr.pAinVqeCfg);

		audio_pr(AUDIO_ERR, "AUDIO_IOCTL_SSP_INIT  init kernel cmd\n");
		audio_pr(AUDIO_INFO, "u32OpenMask[0x%x]\n", pAinVqeCfg->u32OpenMask);
		audio_pr(AUDIO_INFO, "s32WorkSampleRate[%d]\n",
			 pAinVqeCfg->s32WorkSampleRate);
		audio_pr(AUDIO_INFO, "para_aec_filter_len[%d]\n",
			 pAinVqeCfg->stAecCfg.para_aec_filter_len);
		audio_pr(AUDIO_INFO, "para_aes_std_thrd[%d]\n",
			 pAinVqeCfg->stAecCfg.para_aes_std_thrd);
		audio_pr(AUDIO_INFO, "para_aes_supp_coeff[%d]\n",
			 pAinVqeCfg->stAecCfg.para_aes_supp_coeff);
		audio_pr(AUDIO_INFO, "para_nr_snr_coeff[%d]\n",
			 pAinVqeCfg->stAnrCfg.para_nr_snr_coeff);
		audio_pr(AUDIO_INFO, "para_nr_init_sile_time[%d]\n",
			 pAinVqeCfg->stAnrCfg.para_nr_init_sile_time);
		audio_pr(AUDIO_INFO, "para_agc_max_gain[%d]\n",
			 pAinVqeCfg->stAgcCfg.para_agc_max_gain);
		audio_pr(AUDIO_INFO, "para_agc_target_high[%d]\n",
			 pAinVqeCfg->stAgcCfg.para_agc_target_high);
		audio_pr(AUDIO_INFO, "para_agc_target_low[%d]\n",
			 pAinVqeCfg->stAgcCfg.para_agc_target_low);
		audio_pr(AUDIO_INFO, "para_agc_vad_ena[%d]\n",
			 pAinVqeCfg->stAgcCfg.para_agc_vad_ena);
		audio_pr(AUDIO_INFO, "para_aec_init_filter_len[%d]\n",
			 pAinVqeCfg->stAecDelayCfg.para_aec_init_filter_len);
		audio_pr(AUDIO_INFO, "para_dg_target[%d]\n",
			 pAinVqeCfg->stAecDelayCfg.para_dg_target);
		audio_pr(AUDIO_INFO, "para_delay_sample[%d]\n",
			 pAinVqeCfg->stAecDelayCfg.para_delay_sample);
		audio_pr(AUDIO_INFO, "para_notch_freq[%d]\n",
			 pAinVqeCfg->para_notch_freq);
		// dump out the msg
		audio_pr(AUDIO_INFO, "[ToRt]ssp buffer index and address-------------[start]\n");
		audio_pr(AUDIO_INFO, "[ToRt]u32OpenMask[0x%x]\n", pAinVqeCfgRtos->u32OpenMask);
		audio_pr(AUDIO_INFO, "[ToRt]s32WorkSampleRate[%d]\n",
			 pAinVqeCfgRtos->s32WorkSampleRate);
		audio_pr(AUDIO_INFO, "[ToRt]para_aec_filter_len[%d]\n",
			 pAinVqeCfgRtos->stAecCfg.para_aec_filter_len);
		audio_pr(AUDIO_INFO, "[ToRt]para_aes_std_thrd[%d]\n",
			 pAinVqeCfgRtos->stAecCfg.para_aes_std_thrd);
		audio_pr(AUDIO_INFO, "[ToRt]para_aes_supp_coeff[%d]\n",
			 pAinVqeCfgRtos->stAecCfg.para_aes_supp_coeff);
		audio_pr(AUDIO_INFO, "[ToRt]para_nr_snr_coeff[%d]\n",
			 pAinVqeCfgRtos->stAnrCfg.para_nr_snr_coeff);
		audio_pr(AUDIO_INFO, "[ToRt]para_nr_init_sile_time[%d]\n",
			 pAinVqeCfgRtos->stAnrCfg.para_nr_init_sile_time);
		audio_pr(AUDIO_INFO, "[ToRt]para_agc_max_gain[%d]\n",
			 pAinVqeCfgRtos->stAgcCfg.para_agc_max_gain);
		audio_pr(AUDIO_INFO, "[ToRt]para_agc_target_high[%d]\n",
			 pAinVqeCfgRtos->stAgcCfg.para_agc_target_high);
		audio_pr(AUDIO_INFO, "[ToRt]para_agc_target_low[%d]\n",
			 pAinVqeCfgRtos->stAgcCfg.para_agc_target_low);
		audio_pr(AUDIO_INFO, "[ToRt]para_agc_vad_ena[%d]\n",
			 pAinVqeCfgRtos->stAgcCfg.para_agc_vad_ena);
		audio_pr(AUDIO_INFO, "[ToRt]para_aec_init_filter_len[%d]\n",
			 pAinVqeCfgRtos->stAecDelayCfg.para_aec_init_filter_len);
		audio_pr(AUDIO_INFO, "[ToRt]para_dg_target[%d]\n",
			 pAinVqeCfgRtos->stAecDelayCfg.para_dg_target);
		audio_pr(AUDIO_INFO, "[ToRt]para_delay_sample[%d]\n",
			 pAinVqeCfgRtos->stAecDelayCfg.para_delay_sample);
		audio_pr(AUDIO_INFO, "[ToRt]para_notch_freq[%d]\n",
			 pAinVqeCfgRtos->para_notch_freq);
		audio_pr(AUDIO_INFO, "[ToRt]ssp buffer index and address--------------[end]\n");
		audio_pr(AUDIO_ERR, "Do the deinit before the ssp init command\n");

#endif
		//step 4: update the buffer indicator, reset all the indicators
		memset(gstSspIndicatorAddr.pindicator, 0, sizeof(ST_SSP_RTOS_INDICATOR));
		gstSspIndicatorAddr.pindicator->ssp_on = (stSspMsg.stVqeConfig.u32OpenMask == 0) ? 0 : 1;
		if ((pAinVqeCfg->u32OpenMask & LP_AEC_ENABLE) || (pAinVqeCfg->u32OpenMask & NLP_AES_ENABLE)) {
			gstSspIndicatorAddr.pindicator->ssp_with_aec = 1;
			gstSspIndicatorAddr.pindicator->channel_nums = 2;
		}
		if (stSspMsg.bytes_per_period > 0) {
			gstSspIndicatorAddr.pindicator->chunks_number =
				stSspMsg.bytes_per_period/AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
			if (gstSspIndicatorAddr.pindicator->chunks_number > AUDIO_SSP_CHUNK_NUMBERS)
				gstSspIndicatorAddr.pindicator->chunks_number = AUDIO_RTOS_TRIGGER_THRESHOLD;

		} else
			gstSspIndicatorAddr.pindicator->chunks_number = AUDIO_RTOS_TRIGGER_THRESHOLD;


		//ssp_input_tsk = kthread_create(kssp_input_task, &input_data, "ssp_input_tsk");
		//if (IS_ERR(ssp_input_tsk))
		//{
		//	// check error
		//	audio_pr(AUDIO_ERR, "[%s][%d]create thread failure\n", __func__, __LINE__);
		//}
		pcm_thread = kthread_run(pcm_event_handler, NULL, "pcm-handler%d", 0);
		ssp_output_tsk = kthread_create(kssp_output_task, &output_data, "ssp_output_tsk");
		if (IS_ERR(ssp_output_tsk)) {
			// check error
			audio_pr(AUDIO_ERR, "[%s][%d]create thread failure\n", __func__, __LINE__);
		}

		bEventExitPending = 0;
		wake_up_process(ssp_output_tsk);

		//wake_up_process(ssp_input_tsk);
		//step 7: create a handler for RTOS->audio_core.ko handler

		ret = request_rtos_irq(IP_AUDIO,
					audio_rtos_irq_handler,
					"RTOS_CMDQU_IRQ_AUDIO",
					"AUDIO_IOCTL_SSP_ISR");

		//step 6: send the rtos command to RTOS AUDIO task
		memset(&rtos_cmdq, 0, sizeof(struct cmdqu_t));
		rtos_cmdq.ip_id = IP_AUDIO;
		rtos_cmdq.cmd_id = AUDIO_RTOS_CMD_SSP_INIT;
		//do not send block

		// packing the free rtos init config
		audio_pr(AUDIO_INFO,
			"PhYaudio_ mailbox[0x%llx] vqeCfg[0x%llx] indicator[0x%llx] bufferTbl[0x%llx]\n",
			gstaudioMailBoxPhy,
			gstSspVqeAddr.AinVqeCfgPhy,
			gstSspIndicatorAddr.indicatorPhy,
			gstSspBufTblAddr.buffertblPhy);
		audio_pr(AUDIO_INFO,
			"audio_ mb[0x"PRT_PT"] vCfg[0x"PRT_PT"] indic[0x"PRT_PT"] Tbl[0x"PRT_PT"]\n",
			(TYPE_PT)gpstaudioMailBox,
			(TYPE_PT)gstSspVqeAddr.pAinVqeCfg,
			(TYPE_PT)gstSspIndicatorAddr.pindicator,
			(TYPE_PT)gstSspBufTblAddr.pbuffertbl);
		audio_pr(AUDIO_INFO,
			"[xxx]RTOS SSP trigger ISR every cnt[%d]\n",
			gstSspIndicatorAddr.pindicator->chunks_number);
		base_ion_cache_flush(gstaudioMailBoxPhy,
				gpstaudioMailBox,
				sizeof(ST_AUDIO_MAILBOX));
		_audio_flush_ssp_indicator_and_tbl();
		base_ion_cache_flush(gstSspVqeAddr.AinVqeCfgPhy,
				gstSspVqeAddr.pAinVqeCfg,
				sizeof(AI_TALKVQE_CONFIG_S_RTOS));

		if ((gstaudioMailBoxPhy != 0) && (gstSspIndicatorAddr.pindicator->ssp_on)) {
			rtos_cmdq.param_ptr = gstaudioMailBoxPhy; // need to send physical address
			ret = rtos_cmdqu_send(&rtos_cmdq);
			if (ret != 0)
				audio_pr(AUDIO_ERR, "AUDIO: rtos command: failure\n");
			else {
				rtos_cmdq.cmd_id = AUDIO_RTOS_CMD_SSP_PROCESS;
				ret = rtos_cmdqu_send(&rtos_cmdq);
				if (ret != 0)
					audio_pr(AUDIO_ERR, "AUDIO: rtos command: failure\n");
			}
		} else
			audio_pr(AUDIO_ERR, "Did not send  cmd:AUDIO_RTOS_CMD_SSP_INIT\n");

	}
	break;
	case AUDIO_IOCTL_SSP_DEBUG:
	{
		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_DEBUG\n");
		ssp_input_tsk = kthread_create(kssp_input_task, &input_data, "ssp_input_tsk");
		if (IS_ERR(ssp_input_tsk)) {
			// check error
			audio_pr(AUDIO_ERR, "[%s][%d]create thread failure\n", __func__, __LINE__);
		}
		wake_up_process(ssp_input_tsk);
		//pcm_thread = kthread_run(pcm_event_handler, NULL, "pcm-handler%d", 0);

	}
	break;
	case AUDIO_IOCTL_SSP_DEINIT:
	{
		cmdqu_t rtos_cmdq;

		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_DEINIT\n");
		if (ssp_input_tsk)
			kthread_stop(ssp_input_tsk);
		if (ssp_output_tsk)
			kthread_stop(ssp_output_tsk);
		bEventExitPending = 1;
		if (pcm_thread)
			kthread_stop(pcm_thread);


		memset(&rtos_cmdq, 0, sizeof(struct cmdqu_t));
		rtos_cmdq.ip_id = IP_AUDIO;
		rtos_cmdq.cmd_id = AUDIO_RTOS_CMD_SSP_DEINIT;
		ret = rtos_cmdqu_send(&rtos_cmdq);
		if (ret != 0) {
			audio_pr(AUDIO_ERR, "AUDIO: rtos command: failure\n");
			break;
		}
	}
	break;
	case AUDIO_IOCTL_SSP_PROC:
	{
		ST_SSP_RTOS_INDICATOR *_pindicator = gstSspIndicatorAddr.pindicator;
		ST_SSP_BUFTBL	*_pbuffertbl = gstSspBufTblAddr.pbuffertbl;
		unsigned char cur_pt;
		unsigned char cur_cnt;
		int chunks_need;
		char *cur_output_addr;
		char *cur_mic_in_addr;

		//audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_PROC\n");
		//step 1: check the address of virtual buffer tbl valid / incicator valid
		if (!_pindicator || !gstSspBufTblAddr.poutput_vir || !_pbuffertbl) {
			audio_pr(AUDIO_ERR, "not exist yet...[0x"PRT_PT"][0x"PRT_PT"][0x"PRT_PT"]\n",
				(TYPE_PT)_pindicator,
				(TYPE_PT)gstSspBufTblAddr.poutput_vir,
				(TYPE_PT)_pbuffertbl);
			break;
		}
		//step 2: get the data from co-buffer(ST_SSP_MIC_BUF_TABLE_ADDR)
		//step 3: copy to user
		if (copy_from_user(&stSspDataMsg, (void __user *)arg, sizeof(ST_SSP_DATA_MSG)) != 0)
			audio_pr(AUDIO_ERR, "AUDIO_IOCTL_SSP_PROC cmd failure\n");

		cur_pt = _pindicator->Rpt_index;
		cur_cnt = 0;
		chunks_need = stSspDataMsg.required_size_bytes / AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;

		if (!stSspDataMsg.data_addr) {
			audio_pr(AUDIO_ERR, "NULL pt detect in data_addr ["PRT_PT"]\n",
				(TYPE_PT)stSspDataMsg.data_addr);
			break;
		}
		if (chunks_need < 0 || chunks_need > AUDIO_SSP_CHUNK_NUMBERS) {
			audio_pr(AUDIO_ERR,
				"Error size[%d]at least require one single frame(160 sampels = 320 bytes)[%d]\n",
				stSspDataMsg.required_size_bytes,
				chunks_need);
			stSspDataMsg.data_valid = -1;
		}

		if ((stSspDataMsg.required_size_bytes % AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES) != 0) {
			audio_pr(AUDIO_ERR,
				"Error size should align to single frame(160 samples = 320 bytes)[%d]\n",
				stSspDataMsg.required_size_bytes);
			stSspDataMsg.data_valid = -1;
			break;
		}
		//check sufficient valid data or not ...
		while (cur_cnt < chunks_need) {
			if (_pindicator->ssp_on) {
				if (_pbuffertbl[cur_pt].bBufOccupy != AUDIO_BUF_TBL_AFTER_SSP) {
					audio_pr(AUDIO_ERR,
						"SSP not enough ssp out data cur_cnt[%d] cur_pt[%d]Rpt_cnt_need[%d]\n",
						cur_cnt, cur_pt,
						chunks_need);
					_audio_dump_ssp_indicator_and_tbl(__func__, __LINE__);
					break;
				}

			} else {
				if (_pbuffertbl[cur_pt].bBufOccupy != AUDIO_BUF_TBL_INPUT) {
					audio_pr(AUDIO_ERR,
						"not enough pcm input data cur_cnt[%d] cur_pt[%d]Rpt_needcnt[%d]\n",
						cur_cnt,
						cur_pt,
						chunks_need);
					_audio_dump_ssp_indicator_and_tbl(__func__, __LINE__);
					break;
				}
			}
			cur_pt = (cur_pt + 1) % AUDIO_SSP_CHUNK_NUMBERS;
			cur_cnt += 1;
		}
		if (cur_cnt != chunks_need) {
			//return invalid, not enough data
			audio_pr(AUDIO_ERR, "not enough data cur_cnt[%d] chunks_need[%d]Rpt[%d]\n",
						cur_cnt, chunks_need, _pindicator->Rpt_index);
			stSspDataMsg.data_valid = -1;
			stSspDataMsg.data_addr = NULL;
			ret = copy_to_user((ST_SSP_DATA_MSG *)arg, &stSspDataMsg, sizeof(ST_SSP_DATA_MSG));
			if (ret != 0) {
				audio_pr(AUDIO_ERR, "copy to user error in line[%s][%d]\n",
					__func__, __LINE__);
				break;
			}


		} else {

			//lock while doing the data moving/copying in co-buffer
			cur_pt = _pindicator->Rpt_index;
			cur_cnt = 0;
			spin_lock_irqsave(&lock, flags);
			//calculate the cur_pt virtual position

			cur_output_addr = gstSspBufTblAddr.poutput_vir + cur_pt * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
			cur_mic_in_addr = gstSspBufTblAddr.pmic_in_vir + cur_pt * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES;
			if (cur_pt + chunks_need > AUDIO_SSP_CHUNK_NUMBERS) {
				//buffer need loop back
				unsigned char first_chunk = AUDIO_SSP_CHUNK_NUMBERS - cur_pt;
				unsigned char second_chunk = chunks_need - first_chunk;

			if (_pindicator->ssp_on) {
				memcpy(stSspDataMsg.data_addr,
					cur_output_addr,
					first_chunk * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
				memcpy(stSspDataMsg.data_addr + first_chunk * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES,
					gstSspBufTblAddr.poutput_vir,
					second_chunk * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
			} else {
				//return the original data
				memcpy(stSspDataMsg.data_addr,
					cur_mic_in_addr, first_chunk * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
				memcpy(stSspDataMsg.data_addr + first_chunk * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES,
					gstSspBufTblAddr.pmic_in_vir,
					second_chunk * AUDIO_SSP_SINGLE_CHUNK_SIZE_BYTES);
			}
				stSspDataMsg.data_valid = 1;
			} else {
				//buffer not loop back ...excute with single memcpy
				if (_pindicator->ssp_on) {
					memcpy(stSspDataMsg.data_addr, cur_output_addr,
						stSspDataMsg.required_size_bytes);
				} else {
					//return the original data
					memcpy(stSspDataMsg.data_addr, cur_mic_in_addr,
						stSspDataMsg.required_size_bytes);
				}
				stSspDataMsg.data_valid = 1;
			}
			spin_unlock_irqrestore(&lock, flags);
			audio_pr(AUDIO_INFO, "valid[%d]\n", stSspDataMsg.data_valid);
			if (copy_to_user((void __user *)arg, &stSspDataMsg, sizeof(ST_SSP_DATA_MSG)) != 0) {
				audio_pr(AUDIO_ERR, "copy to user error in line[%s][%d]\n", __func__, __LINE__);
				break;
			}
			//update indicator(userspace Rpt_index) & co-buffer +
			spin_lock_irqsave(&lock, flags);
			cur_pt = _pindicator->Rpt_index;
			cur_cnt = 0;
			while (cur_cnt < chunks_need) {
				_pbuffertbl[cur_pt].bBufOccupy = AUDIO_BUF_TBL_UNOCCUPIED;
				cur_pt = (cur_pt + 1) % AUDIO_SSP_CHUNK_NUMBERS;
				cur_cnt += 1;
			}
			_pindicator->Rpt_index = cur_pt;
			spin_unlock_irqrestore(&lock, flags);

			if (!_audio_flush_ssp_indicator_and_tbl()) {
				audio_pr(AUDIO_ERR, "Error in [%s][%d]\n",
					__func__, __LINE__);
			}

		}
	}
	break;
	case AUDIO_IOCTL_SSP_SPK_INIT:
	{
		// TODO:
		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_SPK_INIT\n");
	}
	break;
	case AUDIO_IOCTL_SSP_SPK_DEINIT:
	{
		// TODO:
		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_SPK_DEINIT\n");
	}
	break;
	case AUDIO_IOCTL_SSP_SPK_PROC:
	{
		// TODO:
		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_SPK_PROC\n");
	}
	break;
	case AUDIO_IOCTL_SSP_UNIT_TEST_STOP:
	{
		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_UNIT_TEST_STOP\n");
		// step 1: kill the thread
		if (brook_tsk)
			kthread_stop(brook_tsk);
		else
			audio_pr(AUDIO_ERR, "brook_tsk audio unit test thread not exist!!\n");
		valid_write_file = 0;
		// deallocate the memory for buffer and global variable
		if (!pSspRtosUnitTesCfg)
		{
			audio_pr(AUDIO_INFO, "pSspRtosUnitTesCfg not exist!!!\n");
		} else {
			if (!pSspRtosUnitTesCfg->buffertbl[0].mic_in_addr) {
				base_ion_free(pSspRtosUnitTesCfg->buffertbl[0].mic_in_addr);
				pSspRtosUnitTesCfg->buffertbl[0].mic_in_addr = 0;
			}
			if (!pSspRtosUnitTesCfg->buffertbl[0].ref_in_addr) {
				base_ion_free(pSspRtosUnitTesCfg->buffertbl[0].ref_in_addr);
				pSspRtosUnitTesCfg->buffertbl[0].ref_in_addr = 0;
			}
			if (!pSspRtosUnitTesCfg->buffertbl[0].output_addr) {
				base_ion_free(pSspRtosUnitTesCfg->buffertbl[0].output_addr);
				pSspRtosUnitTesCfg->buffertbl[0].output_addr = 0;
			}
			if (!pSspRtosUnitTesCfg->CbPhyAddr) {
				base_ion_free(pSspRtosUnitTesCfg->CbPhyAddr);
				pSspRtosUnitTesCfg->CbPhyAddr = 0;
			}
			if (!SspRtosUnitTesCfgPhy) {
				base_ion_free(SspRtosUnitTesCfgPhy);
				SspRtosUnitTesCfgPhy = 0;
			}
			audio_pr(AUDIO_INFO, "Warning !!!..free rtos irq for Audio!!\n");
			free_rtos_irq(IP_AUDIO);
		}
		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_UNIT_TEST_STOP..DONE\n");
	}
	break;
	case AUDIO_IOCTL_SSP_UNIT_TEST:
	{
		unsigned int index = 0;

		audio_pr(AUDIO_INFO, "AUDIO_IOCTL_SSP_UNIT_TEST\n");
		audio_pr(AUDIO_INFO, "Do not need the userspace info\n");
		audio_pr(AUDIO_INFO, "Only need to create the buffer and trigger rtos unit test\n");
		index = index;
		if (pSspRtosUnitTesCfg == NULL)
		{
			uint64_t mic_in_phy, ref_in_phy, output_phy, CbAddrPhy;
			// step 1 allocate global handle
			// need require none-cache memory ->
			// free rtos have cache, free rtos read need invalid cache (first time always none-cache)

			// pSspRtosUnitTesCfg =  kzalloc(sizeof(ST_SSP_RTOS_INIT), GFP_ATOMIC);
			base_ion_alloc(&SspRtosUnitTesCfgPhy,
				      (void *)&pSspRtosUnitTesCfg,
				      "pSspRtosUnitTesCfg",
				      sizeof(ST_SSP_RTOS_INIT),
				      1);

			audio_pr(AUDIO_DBG, "[0x%x][0x"PRT_PT"]\n",
				 (unsigned int)SspRtosUnitTesCfgPhy,
				 (TYPE_PT)pSspRtosUnitTesCfg);
			memset(pSspRtosUnitTesCfg, 0, sizeof(ST_SSP_RTOS_INIT));
			// step 2 create the mic_in/ref_in/out buffer chunks for audio_core.ko and free_rtos
			// co-buffer in the physical address
			audio_pr(AUDIO_DBG, "enter[%s][%d]\n", __func__, __LINE__);

			base_ion_alloc(&mic_in_phy,
				      (void *)&pSspRtosUnitTesCfg->pmic_in_vir,
				      "SspRtosUnitTes_mic_in",
				      AUDIO_MAX_BUFFER_SIZE,
				      1);
			base_ion_alloc(&ref_in_phy,
				      (void *)&pSspRtosUnitTesCfg->pref_in_vir,
				      "SspRtosUnitTes_ref_in",
				      AUDIO_MAX_BUFFER_SIZE,
				      1);

			base_ion_alloc(&output_phy,
				      (void *)&pSspRtosUnitTesCfg->poutput_vir,
				      "SspRtosUnitTes_output_in",
				      AUDIO_MAX_BUFFER_SIZE,
				      1);

			audio_pr(AUDIO_DBG,
				 "[%s][%d]mic[0x"PRT_PT"][0x%lx]ref[0x"PRT_PT"][0x%lx]o[0x"PRT_PT"][0x%lx]\n",
				 __func__,
				 __LINE__,
				 (TYPE_PT)pSspRtosUnitTesCfg->pmic_in_vir,
				 (unsigned long)mic_in_phy,
				 (TYPE_PT)pSspRtosUnitTesCfg->pref_in_vir,
				 (unsigned long)ref_in_phy,
				 (TYPE_PT)pSspRtosUnitTesCfg->poutput_vir,
				 (unsigned long)output_phy);

			memset(&pSspRtosUnitTesCfg->buffertbl,
			       0,
			       sizeof(ST_SSP_BUFTBL) * AUDIO_SSP_CHUNK_NUMBERS);
			// step 3 fill in the physical address in buffertbl, this table will be the
			// co-buffer of audio_core.ko and free_rtos
			for (index = 0; index < AUDIO_SSP_CHUNK_NUMBERS; index++) {
				pSspRtosUnitTesCfg->buffertbl[index].mic_in_addr =
				    mic_in_phy + index * (AUDIO_MAX_BUFFER_SIZE / AUDIO_SSP_CHUNK_NUMBERS);
				pSspRtosUnitTesCfg->buffertbl[index].ref_in_addr =
				    ref_in_phy + index * (AUDIO_MAX_BUFFER_SIZE / AUDIO_SSP_CHUNK_NUMBERS);
				pSspRtosUnitTesCfg->buffertbl[index].output_addr =
				    output_phy + index * (AUDIO_MAX_BUFFER_SIZE / AUDIO_SSP_CHUNK_NUMBERS);
			}
			// step 4: Do not need to set the talk attribute of vqe
			pSspRtosUnitTesCfg->s32RevMask = AUDIO_RTOS_MAGIC_WORD_UNIT_TEST_MODE;
			audio_pr(AUDIO_INFO,
				 "[kernel space]pSspRtosUnitTesCfg->s32RevMask[0x%x]\n",
				 pSspRtosUnitTesCfg->s32RevMask);
			audio_pr(AUDIO_INFO, "pSspRtosUnitTesCfg->tbl[0].mic_in_addr = [0x"PRT_PT"]\n",
				 (TYPE_PT)pSspRtosUnitTesCfg->buffertbl[0].mic_in_addr);
			// step 5: Force toggle 1 for buffer occupy
			// setup for unit test only ...
			audio_pr(AUDIO_DBG,
				 "--->pbuffertbl vir address[0x"PRT_PT"]\n",
				 (TYPE_PT)pSspRtosUnitTesCfg->buffertbl);
			for (index = 0; index < AUDIO_SSP_CHUNK_NUMBERS; index++)
				pSspRtosUnitTesCfg->buffertbl[index].bBufOccupy = 1;

			for (index = 0; index < AUDIO_SSP_CHUNK_NUMBERS; index++) {
				audio_pr(AUDIO_DBG, "TesCfg->buffertbl[%d].bBufOccupy[%d]micin[0x%llx]\n",
					 index,
					 pSspRtosUnitTesCfg->buffertbl[index].bBufOccupy,
					 pSspRtosUnitTesCfg->buffertbl[index].mic_in_addr);
			}

			// step 6: register the callback handler for the unit test audio
			base_ion_alloc(&CbAddrPhy,
				      (void *)&pSspRtosUnitTesCfg->CbVirAddr,
				      "SspRtosUnitTes_Cb",
				      sizeof(ST_SSP_RTOS_SPK_DATA_RET),
				      1);
			pSspRtosUnitTesCfg->CbPhyAddr = CbAddrPhy;
			ret = request_rtos_irq(IP_AUDIO,
					       audio_rtos_irq_unit_test_handler,
					       "RTOS_CMDQU_IRQ_AUDIO",
					       "UNIT_TEST");
			audio_pr(AUDIO_INFO, "enter[%s][%d]CbPhyAddr[0x%lx][0x%lx]\n",
				 __func__, __LINE__,
				 (unsigned long)pSspRtosUnitTesCfg->CbPhyAddr,
				 (unsigned long)CbAddrPhy);
// step 7: open a file to save the irq response data
#ifdef AUDIO_SAVE_FILE
#if (KERNEL_VERSION(5, 1, 0) <= LINUX_VERSION_CODE)
			faudio_ut = file_open2("/tmp/audio_rtos_ut.pcm", O_CREAT | O_WRONLY, 0666);
#else
			faudio_ut = file_open("/tmp/audio_rtos_ut.pcm", O_CREAT | O_WRONLY, 0666);
#endif
			if (!faudio_ut) {
				audio_pr(AUDIO_ERR,
					 "Cannot open file force return!!\n");
				ret = -1;
				break;
			}
			valid_write_file = 0;
			brook_tsk = kthread_create(kunit_test, &data, "brook");
			if (IS_ERR(brook_tsk)) {
				// check error
				audio_pr(AUDIO_ERR, "[%s][%d]create thread failure\n", __func__, __LINE__);
			}
			wake_up_process(brook_tsk);
#endif
			// step 8:send the rtos command with unit test cmd
			memset(&rtos_cmdq, 0, sizeof(struct cmdqu_t));
			base_ion_cache_flush(SspRtosUnitTesCfgPhy,
					pSspRtosUnitTesCfg,
					sizeof(ST_SSP_RTOS_INIT));
			rtos_cmdq.ip_id = IP_AUDIO;
			rtos_cmdq.cmd_id = AUDIO_RTOS_CMD_SSP_UNIT_TEST;
			rtos_cmdq.param_ptr = SspRtosUnitTesCfgPhy;
			audio_pr(AUDIO_INFO, "[kernel_space]rtos_cmdq.param_ptr[%x]\n", rtos_cmdq.param_ptr);
			ret = rtos_cmdqu_send(&rtos_cmdq);
			audio_pr(AUDIO_INFO, "enter[%s][%d]\n", __func__, __LINE__);
			if (ret != 0) {
				audio_pr(AUDIO_ERR, "AUDIO_IOCTL_SSP_UNIT_TEST ko send failure\n");
				ret = -1;
				break;
			}
			audio_pr(AUDIO_INFO, "enter[%s][%d]\n", __func__, __LINE__);
		} else {
			audio_pr(AUDIO_ERR, "not entering the command itself xxx\n");
		}
	}
	break;
	default:
		ret = -1;
		pr_err("[%s]Unrecognized commands[%d][0x%x]\n", __func__, (int)cmd, (int)cmd);
		break;
	}
	return ret;
}

int audio_device_release(struct inode *inode, struct file *filp)
{
	unsigned int minor = iminor(inode);
	pr_err("close device minor no. %u\n", minor);

	atomic_dec(&audio_dev_open_cnt);
	if (!atomic_read(&audio_dev_open_cnt)) {
		// release mmap
		pr_err("[%s]..no more process exist\n", __func__);
		_audio_release_all_mem();
		// release global variable
	}
	gst_audio_status.counter -= 1;
	return 0;
}

unsigned int audio_device_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	unsigned int minor = iminor(file_inode(filp));
	int ret = 0;

	ret = down_interruptible(&audio_core_sem);
	if (ret != 0) {
		up(&audio_core_sem);

		return mask;
	}

	poll_wait(filp, &tWaitQueueDev[minor], wait);

	mask |= POLLIN | POLLRDNORM;

	up(&audio_core_sem);

	return mask;
}

int audio_device_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct audio_dev *adev;
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos;

	adev = filp->private_data;
	if (adev == NULL) {
		pr_err("audio_core error NULL[%s][%d]\n", __func__, __LINE__);
		return -EINVAL;
	}
	if (adev->shared_mem == NULL) {
		pr_err("audio_core error NULL[%s][%d]\n", __func__, __LINE__);
		return -EINVAL;
	}
	pos = adev->shared_mem;
	pr_err("shm[0x"PRT_PT"]\n", (TYPE_PT)adev->shared_mem);

	if ((vm_size + offset) > AUDIO_SHARE_MEM_SIZE) {
		pr_err("[Error]%s shared memory size not enough\n", __func__);
		return -EINVAL;
	}
	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot)) {
			pr_err("[audio][error][%s][%d]\n", __func__, __LINE__);
			return -EAGAIN;
		}
		// pr_err("audio mmap vir ok(%p) phys(%#llx)\n", pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

int audio_device_cb(void *dev, enum enum_modules_id caller, u32 cmd, void *arg)
{
	return 0;
}

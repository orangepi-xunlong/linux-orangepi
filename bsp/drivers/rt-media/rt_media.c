/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define LOG_TAG "rt_media"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/rmap.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

#include "rt_media.h"
#include <uapi/rt-media/uapi_rt_media.h>

#include "component/rt_common.h"
#include "component/rt_component.h"
#include "component/rt_venc_component.h"
#include "component/rt_vi_component.h"
#include <linux/aw_rpmsg.h>
#include <uapi/linux/sched/types.h>
#include <linux/dma-heap.h>
#include <linux/dma-buf.h>
#ifndef RT_MEDIA_DEV_MAJOR
#define RT_MEDIA_DEV_MAJOR (160)
#endif
#ifndef RT_MEDIA_DEV_MINOR
#define RT_MEDIA_DEV_MINOR (0)
#endif

#define MOTION_TOTAL_REGION_NUM_DEFAULT (50)
#define REGION_D3D_TOTAL_REGION_NUM_DEFAULT (120)

int g_rt_media_dev_major = RT_MEDIA_DEV_MAJOR;
int g_rt_media_dev_minor = RT_MEDIA_DEV_MINOR;
/*S_IRUGO represent that g_rt_media_dev_major can be read,but canot be write*/
module_param(g_rt_media_dev_major, int, 0444);
module_param(g_rt_media_dev_minor, int, 0444);

typedef struct stream_buffer_manager {
	struct list_head empty_stream_list;
	struct list_head valid_stream_list;
	struct list_head caller_hold_stream_list;
	struct mutex mutex;
	int empty_num;

	wait_queue_head_t wait_bitstream;
	unsigned int wait_bitstream_condition;
	unsigned int need_wait_up_flag;
} stream_buffer_manager;

typedef enum rt_media_state {
	RT_MEDIA_STATE_IDLE     = 0,
	RT_MEDIA_STATE_CONFIGED = 1,
	RT_MEDIA_STATE_EXCUTING = 2,
	RT_MEDIA_STATE_PAUSE    = 3,
} rt_media_state;

typedef struct media_private_info {
	int channel;
} media_private_info;

typedef struct video_recoder {
	int activate_vi_flag;
	int activate_venc_flag;
	rt_component_type *venc_comp;
	rt_component_type *vi_comp;
	stream_buffer_manager stream_buf_manager;
	int bcreate_comp_flag;
	rt_media_state state;
	rt_media_config_s config;
	config_yuv_buf_info yuv_buf_info;
	rt_pixelformat_type pixelformat;
	unsigned char *enc_yuv_phy_addr;
	int stream_data_cb_count;
	int max_fps;
	int enable_high_fps;
	int bReset_high_fps_finish;
	int bsetup_recorder_finish;
	int bvin_is_ready;//rt_media thread setup not ready fact.
	RTVencMotionSearchParam motion_search_param;
	RTVencMotionSearchRegion *motion_region;
	int motion_region_len; //unit:bytes
	RTVencRegionD3DParam region_d3d_param;
	RTVencRegionD3DRegion *region_d3d_region;
	int region_d3d_region_len; //unit:bytes
} video_recoder;

struct rt_media_dev {
	struct cdev cdev; /* char device struct				   */
	struct device *dev; /* ptr to class device struct		   */
	struct device *platform_dev; /* ptr to class device struct		   */
	struct class *class; /* class for auto create device node  */

	struct semaphore sem; /* mutual exclusion semaphore		   */

	wait_queue_head_t wq; /* wait queue for poll ops			   */

	struct mutex lock_vdec;

	video_recoder recoder[VIDEO_INPUT_CHANNEL_NUM]; //1: major, 2: minor, 3: yuv

	struct task_struct *setup_thread;

	struct task_struct *reset_high_fps_thread;

	int in_high_fps_status;
	video_recoder *need_start_recoder_1;
	video_recoder *need_start_recoder_2;
	int bcsi_err;
	int bcsi_status_set[VIDEO_INPUT_CHANNEL_NUM];
};

struct vi_part_cfg {
	int wdr;
	int width;
	int height;
	int venc_format; //1:H264 2:H265
	int flicker_mode; ////0:disable,1:50HZ,2:60HZ, default 50HZ
	int fps;
};
struct csi_status_pair {
	int bset;
	int status;
};
static struct rt_media_dev *rt_media_devp;

static int bvin_is_ready_rt_not_probe[VIDEO_INPUT_CHANNEL_NUM];//Deal with the fact that rt_media not registered
static struct csi_status_pair g_csi_status_pair[VIDEO_INPUT_CHANNEL_NUM];

static video_stream_s *ioctl_get_stream_data(video_recoder *recoder);
static int ioctl_return_stream_data(video_recoder *recoder,
				    video_stream_s *video_stream);
int ioctl_start(video_recoder *recoder);

struct device *rt_get_rt_media_dev(void)
{
	return rt_media_devp->dev;
}

error_type venc_event_handler(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_event_type eEvent,
	PARAM_IN unsigned int nData1,
	PARAM_IN unsigned int nData2,
	PARAM_IN void *pEventData)
{
	/* todo; */
	RT_LOGI("not support venc_event_handler");
	return -1;
}

error_type rt_venc_empty_in_buffer_done(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	video_recoder *recoder      = (video_recoder *)pAppData;
	video_frame_s *pvideo_frame = (video_frame_s *)pBuffer->private;

	if (recoder->enc_yuv_phy_addr != NULL)
		RT_LOGE("the enc_yuv_phy_addr[%px] is not null", recoder->enc_yuv_phy_addr);

	recoder->enc_yuv_phy_addr = pvideo_frame->phy_addr[0];
	RT_LOGD("set recoder->enc_yuv_phy_addr: %px", recoder->enc_yuv_phy_addr);
	return 0;
}

/* empty_list --> valid_list*/
error_type rt_venc_fill_out_buffer_done(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	video_stream_node *stream_node	= NULL;
	video_recoder *recoder		      = (video_recoder *)pAppData;
	video_stream_s *video_stream	  = (video_stream_s *)pBuffer->private;
	stream_buffer_manager *stream_buf_mgr = &recoder->stream_buf_manager;

	recoder->stream_data_cb_count++;
	if (recoder->stream_data_cb_count == 1) {
		RT_LOGW("channel %d first stream data, pts = %lld, time = %lld, width = %d", recoder->config.channelId,
			video_stream->pts, get_cur_time(), recoder->config.width);
	}

	mutex_lock(&stream_buf_mgr->mutex);

	if (list_empty(&stream_buf_mgr->empty_stream_list)) {
		RT_LOGE("rt_venc_fill_out_buffer_done error: empty list is null");
		mutex_unlock(&stream_buf_mgr->mutex);
		return ERROR_TYPE_ERROR;
	}
	stream_node = list_first_entry(&stream_buf_mgr->empty_stream_list, video_stream_node, mList);
	memcpy(&stream_node->video_stream, video_stream, sizeof(video_stream_s));

	stream_buf_mgr->empty_num--;
	list_move_tail(&stream_node->mList, &stream_buf_mgr->valid_stream_list);

	if (stream_buf_mgr->need_wait_up_flag == 1) {
		stream_buf_mgr->need_wait_up_flag	= 0;
		stream_buf_mgr->wait_bitstream_condition = 1;
		wake_up(&stream_buf_mgr->wait_bitstream);
	}
	mutex_unlock(&stream_buf_mgr->mutex);

	/* todo; */
	return 0;
}

comp_callback_type venc_callback = {
	.EventHandler	 = venc_event_handler,
	.empty_in_buffer_done = rt_venc_empty_in_buffer_done,
	.fill_out_buffer_done = rt_venc_fill_out_buffer_done,
};

static int thread_reset_high_fps(void *param)
{
	int ret		       = 0;
	int channel_id	 = 0;
	video_recoder *recoder = &rt_media_devp->recoder[channel_id];

	RT_LOGW("start");
	/* set config: reset high fps */
	ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_SET_RESET_HIGH_FPS, NULL);
	if (ret < 0) {
		RT_LOGE("IOCTL_SET_POWER_LINE_FREQ failed, ret = %d", ret);
		return -EFAULT;
	}

	ioctl_start(recoder);

	recoder->bReset_high_fps_finish = 1;

	RT_LOGW("rt_media_devp->need_start_recoder = %px, %px",
		rt_media_devp->need_start_recoder_1, rt_media_devp->need_start_recoder_2);

	if (rt_media_devp->need_start_recoder_1) {
		ioctl_start(rt_media_devp->need_start_recoder_1);
		rt_media_devp->need_start_recoder_1 = NULL;
	}

	if (rt_media_devp->need_start_recoder_2) {
		ioctl_start(rt_media_devp->need_start_recoder_2);
		rt_media_devp->need_start_recoder_2 = NULL;
	}

	rt_media_devp->in_high_fps_status = 0;
	RT_LOGW("finish");
	return 0;
}

error_type vi_event_handler(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_event_type eEvent,
	PARAM_IN unsigned int nData1,
	PARAM_IN unsigned int nData2,
	PARAM_IN void *pEventData)
{
	/*video_recoder *recoder = (video_recoder *)pAppData;*/
	RT_LOGI("vi_event_handler, eEvent = %d", eEvent);

	if (eEvent == COMP_EVENT_CMD_RESET_ISP_HIGH_FPS) {
		struct sched_param param = {.sched_priority = 1 };
		rt_media_devp->reset_high_fps_thread = kthread_create(thread_reset_high_fps,
								      rt_media_devp, "reset-high-fps");
		sched_setscheduler(rt_media_devp->reset_high_fps_thread, SCHED_FIFO, &param);
		wake_up_process(rt_media_devp->reset_high_fps_thread);
	}

	return 0;
}

error_type rt_vi_empty_in_buffer_done(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	/* todo; */
	RT_LOGE("not support rt_vi_empty_in_buffer_done");
	return -1;
}

error_type rt_vi_fill_out_buffer_done(
	PARAM_IN comp_handle component,
	PARAM_IN void *pAppData,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	/* todo; */
	RT_LOGE("not support rt_vi_fill_out_buffer_done");
	return -1;
}

comp_callback_type vi_callback = {
	.EventHandler	 = vi_event_handler,
	.empty_in_buffer_done = rt_vi_empty_in_buffer_done,
	.fill_out_buffer_done = rt_vi_fill_out_buffer_done,
};

static int get_component_handle(video_recoder *recoder)
{
	int ret = 0;

	RT_LOGD("recoder->activate_vi_flag: %d", recoder->activate_vi_flag);
	if (recoder->activate_vi_flag == 1) {
		ret = comp_get_handle((comp_handle *)&recoder->vi_comp,
				      "video.vipp",
				      recoder,
					  &recoder->config,
				      &vi_callback);
		if (ret != ERROR_TYPE_OK) {
			RT_LOGE("get vi comp handle error: %d", ret);
			return -1;
		}
	}
	RT_LOGD("recoder->activate_venc_flag: %d", recoder->activate_venc_flag);
	if (recoder->activate_venc_flag == 1) {
		ret = comp_get_handle((comp_handle *)&recoder->venc_comp,
				      "video.encoder",
				      recoder,
					  &recoder->config,
				      &venc_callback);
		if (ret != ERROR_TYPE_OK) {
			RT_LOGE("get venc comp handle error: %d", ret);
			return -1;
		}
	}
	recoder->bcreate_comp_flag = 1;

	return ret;
}

int fps_change_id_1;
int fps_change_id_2;

int get_fps_change_channel_id_1(void)
{
	return fps_change_id_1;
}

void rt_media_csi_fps_change_first(int change_fps)
{
	int ret = 0;
	int channel_id = get_fps_change_channel_id_1();
	video_recoder *recoder = &rt_media_devp->recoder[channel_id];

	if (change_fps <= 0 || change_fps > recoder->max_fps) {
		RT_LOGW("IOCTL_SET_FPS: invalid change_fps[%d], setup to max_fps[%d]", change_fps, recoder->max_fps);
		change_fps = recoder->max_fps;
	}

	RT_LOGI("IOCTL_SET_FPS: change_fps = %d, max_fps = %d, vi_comp = %px, venc_comp = %px",
		change_fps, recoder->max_fps, recoder->vi_comp, recoder->venc_comp);

	if (!recoder->vi_comp)
		return;
	ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_FPS, (void *)&change_fps);
	if (ret < 0) {
		RT_LOGE("IOCTL_SET_FPS failed, ret = %d, change_fps = %d", ret, change_fps);
		return ;
	}

	if (!recoder->venc_comp)
		return;
	ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS, (void *)&change_fps);
	if (ret < 0) {
		RT_LOGE("IOCTL_SET_FPS failed, ret = %d, change_fps = %d", ret, change_fps);
		return ;
	}
}
/* to do
static void set_fps_change_channel_id_2(int id)
{
	fps_change_id_2 = id;
}*/

int get_fps_change_channel_id_2(void)
{
	return fps_change_id_2;
}
void rt_media_csi_fps_change_second(int change_fps)
{
	int ret = 0;
	int channel_id = get_fps_change_channel_id_2();
	video_recoder *recoder	 = &rt_media_devp->recoder[channel_id];

	if (change_fps <= 0 || change_fps > recoder->max_fps) {
		RT_LOGW("IOCTL_SET_FPS: invalid change_fps[%d], setup to max_fps[%d]", change_fps, recoder->max_fps);
		change_fps = recoder->max_fps;
	}

	RT_LOGI("IOCTL_SET_FPS: change_fps = %d, max_fps = %d, vi_comp = %px, venc_comp = %px",
		change_fps, recoder->max_fps, recoder->vi_comp, recoder->venc_comp);

	ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_FPS, (void *)&change_fps);
	if (ret < 0) {
		RT_LOGE("IOCTL_SET_FPS failed, ret = %d, change_fps = %d", ret, change_fps);
		return ;
	}

	ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS, (void *)&change_fps);
	if (ret < 0) {
		RT_LOGE("IOCTL_SET_FPS failed, ret = %d, change_fps = %d", ret, change_fps);
		return ;
	}
}

static void fill_venc_config(venc_comp_base_config *pvenc_config, video_recoder *recoder)
{
	rt_media_config_s *media_config = &recoder->config;

	/* setup venc config */
	if (media_config->encodeType == 0)
		pvenc_config->codec_type = RT_VENC_CODEC_H264;
	else if (media_config->encodeType == 1)
		pvenc_config->codec_type = RT_VENC_CODEC_JPEG;
	else if (media_config->encodeType == 2)
		pvenc_config->codec_type = RT_VENC_CODEC_H265;
	else {
		RT_LOGE("codec_type[%d] not support, use h264", media_config->encodeType);
		pvenc_config->codec_type = RT_VENC_CODEC_H264;
	}

	pvenc_config->qp_range		    = media_config->qp_range;
	pvenc_config->profile		    = media_config->profile;
	pvenc_config->level		    = media_config->level;
	pvenc_config->src_width		    = media_config->width;
	if (media_config->en_16_align_fill_data) {
		pvenc_config->src_height = RT_ALIGN(media_config->height, 16);
	} else {
		pvenc_config->src_height = media_config->height;
	}
	pvenc_config->dst_width		    = media_config->dst_width;
	pvenc_config->dst_height	    = media_config->dst_height;
	pvenc_config->frame_rate	    = media_config->fps;
	pvenc_config->bit_rate		    = media_config->bitrate * 1024;
	pvenc_config->vbv_thresh_size   = media_config->vbv_thresh_size;
	pvenc_config->vbv_buf_size      = media_config->vbv_buf_size;
	pvenc_config->pixelformat	   = recoder->pixelformat;
	pvenc_config->max_keyframe_interval = media_config->gop;
	pvenc_config->enable_overlay	= media_config->enable_overlay;
	pvenc_config->channel_id = media_config->channelId;
	pvenc_config->quality = media_config->jpg_quality;
	pvenc_config->jpg_mode = media_config->jpg_mode;
	pvenc_config->bit_rate_range.bitRateMax = media_config->bit_rate_range.bitRateMax * 1024;
	pvenc_config->bit_rate_range.bitRateMin = media_config->bit_rate_range.bitRateMin * 1024;
	pvenc_config->bOnlineChannel = media_config->bonline_channel;
	pvenc_config->share_buf_num = media_config->share_buf_num;
	pvenc_config->en_encpp_sharp = media_config->enable_sharp;
	pvenc_config->is_ipc_case = media_config->is_ipc;
	pvenc_config->breduce_refrecmem = media_config->breduce_refrecmem;
	if (media_config->enable_crop) {
		pvenc_config->enable_crop = media_config->enable_crop;
		pvenc_config->s_crop_rect.nLeft = media_config->s_crop_rect.nLeft;
		pvenc_config->s_crop_rect.nTop = media_config->s_crop_rect.nTop;
		pvenc_config->s_crop_rect.nWidth = media_config->s_crop_rect.nWidth;
		pvenc_config->s_crop_rect.nHeight = media_config->s_crop_rect.nHeight;
	}
	memcpy(&pvenc_config->venc_video_signal, &media_config->venc_video_signal, sizeof(RTVencVideoSignal));

	if (media_config->vbr == 0)
		pvenc_config->rc_mode = VENC_COMP_RC_MODE_H264CBR;
	else if (media_config->vbr == 1) {
		pvenc_config->rc_mode		     = VENC_COMP_RC_MODE_H264VBR;
		pvenc_config->vbr_param.max_bit_rate = pvenc_config->bit_rate;
		pvenc_config->vbr_param.moving_th    = 20;
		pvenc_config->vbr_param.quality      = 10;
	} else {
		RT_LOGE("media_config->vbr[%d] not support, use cbr", media_config->vbr);
		pvenc_config->rc_mode = VENC_COMP_RC_MODE_H264CBR;
	}
}

static void flush_stream_data(video_recoder *recoder)
{
	video_stream_s *video_stream = NULL;

	video_stream = ioctl_get_stream_data(recoder);

	while (video_stream) {
		ioctl_return_stream_data(recoder, video_stream);

		video_stream = ioctl_get_stream_data(recoder);
	}
}

int ioctl_init(video_recoder *recoder)
{
	int i				      = 0;
	int ret				      = 0;
	stream_buffer_manager *stream_buf_mgr = &recoder->stream_buf_manager;

	ret = get_component_handle(recoder);

	/* init stream buf manager*/
	mutex_init(&stream_buf_mgr->mutex);
	stream_buf_mgr->empty_num = VENC_OUT_BUFFER_LIST_NODE_NUM;
	INIT_LIST_HEAD(&stream_buf_mgr->empty_stream_list);
	INIT_LIST_HEAD(&stream_buf_mgr->valid_stream_list);
	INIT_LIST_HEAD(&stream_buf_mgr->caller_hold_stream_list);
	for (i = 0; i < VENC_OUT_BUFFER_LIST_NODE_NUM; i++) {
		video_stream_node *pNode = kmalloc(sizeof(video_stream_node), GFP_KERNEL);

		if (pNode == NULL) {
			RT_LOGE("fatal error! kmalloc fail!");
			return ERROR_TYPE_NOMEM;
		}
		memset(pNode, 0, sizeof(video_stream_node));
		list_add_tail(&pNode->mList, &stream_buf_mgr->empty_stream_list);
	}
	init_waitqueue_head(&stream_buf_mgr->wait_bitstream);

	return 0;
}

int ioctl_config(video_recoder *recoder, rt_media_config_s *config)
{
	int connect_flag = 0;
	venc_comp_base_config venc_config;
	vi_comp_base_config vi_config;
	rt_media_config_s *media_config = &recoder->config;
	RT_LOGD("Ryan kernel rt-media config recoder->config.output_mode: %d", recoder->config.output_mode);

	if (bvin_is_ready_rt_not_probe[config->channelId]) {
		int i = 0;
		for (; i < VIDEO_INPUT_CHANNEL_NUM; i++) {
			if (g_csi_status_pair[i].bset) {
				rt_media_devp->bcsi_status_set[i] = g_csi_status_pair[i].status;
			}
		}
	}
    /*LOG_LEVEL_VERBOSE = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_INFO = 4,
    LOG_LEVEL_WARNING = 5,
    LOG_LEVEL_ERROR = 6,*/
	//cdc_log_set_level(CONFIG_RT_MEDIA_CDC_LOG_LEVEL);//not encode

	if (config->vin_buf_num < CONFIG_YUV_BUF_NUM)
		config->vin_buf_num = CONFIG_YUV_BUF_NUM;

	memset(&venc_config, 0, sizeof(venc_comp_base_config));
	memset(&vi_config, 0, sizeof(vi_comp_base_config));
	venc_config.channel_id = recoder->config.channelId;
	if (media_config == NULL) {
		RT_LOGE("ioctl_config: param is null");
		return -1;
	}

	if (recoder->state != RT_MEDIA_STATE_IDLE) {
		RT_LOGW("channel %d the state[%d] is not idle", config->channelId, recoder->state);
		return 0;
	}
	if (config->dst_width == 0 || config->dst_height == 0) {
		config->dst_width = config->width;
		config->dst_height = config->height;
	}
	memcpy(media_config, config, sizeof(rt_media_config_s));

	/* set the init-config-fps as max_fps */
	recoder->max_fps = media_config->fps;

	if (media_config->channelId < 0 || media_config->channelId > (VIDEO_INPUT_CHANNEL_NUM - 1)) {
		RT_LOGE("channel[%d] is error", media_config->channelId);
		return -1;
	}
	RT_LOGD("recoder->activate_vi_flag: %d recoder->config.output_mode: %d", recoder->activate_vi_flag, recoder->config.output_mode);
	if (recoder->config.output_mode == OUTPUT_MODE_STREAM) {
		recoder->activate_venc_flag = 1;
		recoder->activate_vi_flag   = 1;
	} else if (recoder->config.output_mode == OUTPUT_MODE_YUV) {
		recoder->activate_venc_flag = 0;
		recoder->activate_vi_flag = 1;
	} else if (recoder->config.output_mode == OUTPUT_MODE_ONLY_ENCDODER) {
		recoder->activate_venc_flag = 1;
		recoder->activate_vi_flag   = 0;
	}

	RT_LOGD("recoder->activate_vi_flag: %d", recoder->activate_vi_flag);
	if (recoder->state == RT_MEDIA_STATE_IDLE)
		ioctl_init(recoder);

#if defined CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL && defined CONFIG_VIDEO_RT_MEDIA
	RT_LOGW("CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL = %d, configSize = %ld",
	CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL, sizeof(rt_media_config_s));
#else
	RT_LOGW("CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL id not enable, configSize = %ld", sizeof(rt_media_config_s));
#endif

	RT_LOGW("out_mode = %d, activate_venc = %d, vi = %d, venc_comp = %px, vi = %px, max_fps = %d",
		recoder->config.output_mode,
		recoder->activate_venc_flag, recoder->activate_vi_flag,
		recoder->venc_comp, recoder->vi_comp, recoder->max_fps);

	if (recoder->config.pixelformat == RT_PIXEL_NUM) {
		if (recoder->config.output_mode == OUTPUT_MODE_STREAM)
			recoder->pixelformat = RT_PIXEL_LBC_25X;
		else
			recoder->pixelformat = RT_PIXEL_YVU420SP; //* nv21
	} else
		recoder->pixelformat = recoder->config.pixelformat;

	fill_venc_config(&venc_config, recoder);

	/* setup vipp config */
	vi_config.width		 = media_config->width;
	vi_config.height	 = media_config->height;
	vi_config.frame_rate     = media_config->fps;
	vi_config.channel_id     = media_config->channelId;
	vi_config.output_mode    = media_config->output_mode;
	vi_config.pixelformat    = recoder->pixelformat;
	vi_config.enable_wdr     = media_config->enable_wdr;
	vi_config.drop_frame_num = media_config->drop_frame_num;
	vi_config.bonline_channel = media_config->bonline_channel;
	vi_config.share_buf_num = media_config->share_buf_num;
	vi_config.en_16_align_fill_data = media_config->en_16_align_fill_data;
	vi_config.vin_buf_num = media_config->vin_buf_num;

	memcpy(&vi_config.venc_video_signal, &media_config->venc_video_signal, sizeof(RTVencVideoSignal));

	RT_LOGI("enable_wdr = %d, %d", media_config->enable_wdr, vi_config.enable_wdr);
	if (recoder->vi_comp)
		comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Base, &vi_config);

	if (recoder->venc_comp)
		comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Base, &venc_config);

	if (recoder->vi_comp && recoder->venc_comp) {
		connect_flag = 1;
		comp_setup_tunnel(recoder->venc_comp, COMP_INPUT_PORT, recoder->vi_comp, connect_flag);
		comp_setup_tunnel(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->venc_comp, connect_flag);
	}
	if (recoder->vi_comp) {
		if (comp_init(recoder->vi_comp) != 0) {
			RT_LOGE("comp_init error");
			return -1;
		}
	}
	if (media_config->width == 0 || media_config->height == 0) {
		vi_comp_base_config tmp_vi_config;

		RT_LOGW("err, width x height: %d %d", media_config->width, media_config->height);
		comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_GET_BASE_CONFIG, &tmp_vi_config);
		media_config->width = tmp_vi_config.width;
		media_config->height = tmp_vi_config.height;
		if (media_config->dst_width == 0 || media_config->dst_height == 0) {
			media_config->dst_width = media_config->width;
			media_config->dst_height = media_config->height;
		}
		venc_config.src_width = media_config->width;
		venc_config.src_height = media_config->height;
		venc_config.dst_width = media_config->dst_width;
		venc_config.dst_height = media_config->dst_height;
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Base, &venc_config);
		if (recoder->vi_comp)
			comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Base, &tmp_vi_config);
	}
	RT_LOGI("begin init venc comp");
	//* init venc comp
	if (recoder->venc_comp) {
		comp_init(recoder->venc_comp);

//		RT_LOGI("media_config->enable_bin_image = %d, th = %d",
//			media_config->enable_bin_image, media_config->bin_image_moving_th);
//		if (media_config->enable_bin_image == 1) {
//			rt_venc_bin_image_param bin_param;
//
//			bin_param.enable    = 1;
//			bin_param.moving_th = media_config->bin_image_moving_th;
//			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_ENABLE_BIN_IMAGE, &bin_param);
//		}
//		if (media_config->enable_mv_info == 1)
//			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_ENABLE_MV_INFO, &media_config->enable_mv_info);
	}
	recoder->state = RT_MEDIA_STATE_CONFIGED;
	RT_LOGI("ioctl_config finish");
	return 0;
}

int ioctl_start(video_recoder *recoder)
{
	RT_LOGI("ioctl_start begin");
	if (recoder->state != RT_MEDIA_STATE_CONFIGED && recoder->state != RT_MEDIA_STATE_PAUSE) {
		RT_LOGW("channel %d ioctl_start: state[%d] is not right", recoder->config.channelId, recoder->state);
		return -1;
	}

	/* int64_t time_end = get_cur_time(); */


	/* start vi comp */
	/*int64_t time_start = get_cur_time(); */
	if (recoder->vi_comp)
		comp_start(recoder->vi_comp);
	/* start venc */
	if (recoder->venc_comp)
		comp_start(recoder->venc_comp);
	/*
	int64_t time_end_1 = get_cur_time();

	RT_LOGD("time[vi start] = %lld, time[venc start] = %lld",
			(time_end - time_start), (time_end_1 - time_end));
	*/

	RT_LOGI("ioctl_start finish");
	recoder->state = RT_MEDIA_STATE_EXCUTING;
	return 0;
}

int ioctl_pause(video_recoder *recoder)
{
	if (recoder->state != RT_MEDIA_STATE_EXCUTING) {
		RT_LOGE("ioctl_start: state[%d] is not right", recoder->state);
		return -1;
	}

	RT_LOGD("comp_pause vi start");
	/* todo : pause vi comp */
	if (recoder->vi_comp)
		comp_pause(recoder->vi_comp);
	RT_LOGD("comp_pause vi end");

	/* pause venc */
	if (recoder->venc_comp)
		comp_pause(recoder->venc_comp);

	RT_LOGD("comp_pause venc end");

	recoder->state = RT_MEDIA_STATE_PAUSE;
	return 0;
}

int ioctl_destroy(video_recoder *recoder)
{
	/* todo */
	video_stream_node *stream_node = NULL;
	int free_stream_cout	   = 0;

	if (recoder->state == RT_MEDIA_STATE_IDLE) {
		RT_LOGW("state is RT_MEDIA_STATE_IDLE");
	}

	/*free the comp*/
	if (recoder->venc_comp) {
		comp_stop(recoder->venc_comp);
		comp_free_handle(recoder->venc_comp);
		recoder->venc_comp = NULL;
	}
	if (recoder->vi_comp) {
		comp_stop(recoder->vi_comp);
		comp_free_handle(recoder->vi_comp);
		recoder->vi_comp = NULL;
	}

	while ((!list_empty(&recoder->stream_buf_manager.empty_stream_list))) {
		stream_node = list_first_entry(&recoder->stream_buf_manager.empty_stream_list, video_stream_node, mList);
		if (stream_node) {
			free_stream_cout++;
			list_del(&stream_node->mList);
			kfree(stream_node);
		}
	}

	while ((!list_empty(&recoder->stream_buf_manager.valid_stream_list))) {
		stream_node = list_first_entry(&recoder->stream_buf_manager.valid_stream_list, video_stream_node, mList);
		if (stream_node) {
			free_stream_cout++;
			list_del(&stream_node->mList);
			kfree(stream_node);
		}
	}

	while ((!list_empty(&recoder->stream_buf_manager.caller_hold_stream_list))) {
		stream_node = list_first_entry(&recoder->stream_buf_manager.caller_hold_stream_list, video_stream_node, mList);
		if (stream_node) {
			free_stream_cout++;
			list_del(&stream_node->mList);
			kfree(stream_node);
		}
	}

	RT_LOGD("free_stream_cout = %d", free_stream_cout);

	if (free_stream_cout != VENC_OUT_BUFFER_LIST_NODE_NUM)
		RT_LOGE("free num of stream node is not match: %d, %d", free_stream_cout, VENC_OUT_BUFFER_LIST_NODE_NUM);

	recoder->state			= RT_MEDIA_STATE_IDLE;
	recoder->enable_high_fps	= 0;
	recoder->bReset_high_fps_finish = 0;

	if (recoder->motion_region) {
		kfree(recoder->motion_region);
		recoder->motion_region = NULL;
		recoder->motion_region_len = 0;
	}

	if (recoder->region_d3d_region) {
		kfree(recoder->region_d3d_region);
		recoder->region_d3d_region = NULL;
		recoder->region_d3d_region_len = 0;
	}

	return -1;
}

static int ioctl_get_vbv_buf_info(video_recoder *recoder,
				  KERNEL_VBV_BUFFER_INFO *vbv_buf_info)
{
	error_type error = ERROR_TYPE_OK;
	KERNEL_VBV_BUFFER_INFO venc_vbv_info;

	memset(&venc_vbv_info, 0, sizeof(KERNEL_VBV_BUFFER_INFO));

	error = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_VBV_BUF_INFO, &venc_vbv_info);
	if (error != ERROR_TYPE_OK) {
		RT_LOGI("get vbv buf info failed");
		return -1;
	}

	vbv_buf_info->share_fd = venc_vbv_info.share_fd;
	vbv_buf_info->size     = venc_vbv_info.size;
	return 0;
}

static int ioctl_get_stream_header(video_recoder *recoder, venc_comp_header_data *header_data)
{
	error_type error = ERROR_TYPE_OK;

	RT_LOGD("call comp_get_config start");
	error = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_STREAM_HEADER, header_data);
	RT_LOGD("call comp_get_config finish, error = %d", error);

	if (error != ERROR_TYPE_OK) {
		RT_LOGW("get vbv buf info failed");
		return -1;
	}

	RT_LOGD("ioctl_get_header_data: size = %d, data = %px\n",
		header_data->nLength, header_data->pBuffer);

	return 0;
}

/* valid_list --> caller_hold_list */
static video_stream_s *ioctl_get_stream_data(video_recoder *recoder)
{
	video_stream_node *stream_node	= NULL;
	stream_buffer_manager *stream_buf_mgr = &recoder->stream_buf_manager;

	mutex_lock(&stream_buf_mgr->mutex);

	if (list_empty(&stream_buf_mgr->valid_stream_list)) {
		RT_LOGD("stream buf valid list is null");
		mutex_unlock(&stream_buf_mgr->mutex);
		return NULL;
	}
	stream_node = list_first_entry(&stream_buf_mgr->valid_stream_list, video_stream_node, mList);

	list_move_tail(&stream_node->mList, &stream_buf_mgr->caller_hold_stream_list);

	mutex_unlock(&stream_buf_mgr->mutex);

	return &stream_node->video_stream;
}

/* caller_hold_list -->  empty_list*/
static int ioctl_return_stream_data(video_recoder *recoder,
				    video_stream_s *video_stream)
{
	video_stream_node *stream_node	= NULL;
	stream_buffer_manager *stream_buf_mgr = &recoder->stream_buf_manager;
	comp_buffer_header_type buffer_header;
	video_stream_s mvideo_stream;

	memset(&mvideo_stream, 0, sizeof(video_stream_s));
	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));

	mutex_lock(&stream_buf_mgr->mutex);

	if (list_empty(&stream_buf_mgr->caller_hold_stream_list)) {
		RT_LOGW("ioctl_return_stream_data: error, caller_hold_list is null");
		mutex_unlock(&stream_buf_mgr->mutex);
		return -1;
	}

	stream_node = list_first_entry(&stream_buf_mgr->caller_hold_stream_list, video_stream_node, mList);
	memmove(&stream_node->video_stream, video_stream, sizeof(video_stream_s));
	memcpy(&mvideo_stream, &stream_node->video_stream, sizeof(video_stream_s));

	list_move_tail(&stream_node->mList, &stream_buf_mgr->empty_stream_list);
	stream_buf_mgr->empty_num++;

	mutex_unlock(&stream_buf_mgr->mutex);

	if (mvideo_stream.id != video_stream->id) {
		RT_LOGW("ioctl_return_stream_data: id not match: %d, %d",
			mvideo_stream.id, video_stream->id);
	}

	buffer_header.private = &mvideo_stream;

	comp_fill_this_out_buffer(recoder->venc_comp, &buffer_header);

	return 0;
}

static int copy_jpeg_data(void *user_buf_info, video_stream_s *video_stream)
{
	int result = 0;
	catch_jpeg_buf_info src_buf_info;

	memset(&src_buf_info, 0, sizeof(catch_jpeg_buf_info));

	if (copy_from_user(&src_buf_info, (void __user *)user_buf_info, sizeof(struct catch_jpeg_buf_info))) {
		RT_LOGE("IOCTL_CATCH_JPEG_START copy_from_user fail\n");
		return -EFAULT;
	}

	if (video_stream->size0 > src_buf_info.buf_size) {
		RT_LOGE("buf_size overflow: %d > %d", video_stream->size0, src_buf_info.buf_size);
		result = -1;
		goto copy_jpeg_data_exit;
	}

	if (copy_to_user((void *)src_buf_info.buf, video_stream->data0, video_stream->size0)) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto copy_jpeg_data_exit;
	}

	if (copy_to_user((void *)user_buf_info, &video_stream->size0, sizeof(unsigned int))) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto copy_jpeg_data_exit;
	}

copy_jpeg_data_exit:

	return result;
}

static int ioctl_output_yuv_catch_jpeg_start(video_recoder *recoder,
					     catch_jpeg_config *jpeg_config)
{
	int ret		    = 0;
	int connect_flag    = 0;
	int catch_jpeg_flag = 0;
	venc_comp_base_config venc_config;
	rt_media_config_s *media_config = &recoder->config;

	memset(&venc_config, 0, sizeof(venc_comp_base_config));
	ret = comp_get_handle((comp_handle *)&recoder->venc_comp,
			"video.encoder",
			recoder,
			&recoder->config,
			&venc_callback);
	if (ret != ERROR_TYPE_OK) {
		RT_LOGE("get venc comp handle error: %d", ret);
		return -1;
	}

	venc_config.quality		  = jpeg_config->qp;
	venc_config.codec_type		  = RT_VENC_CODEC_JPEG;
	venc_config.src_width		  = media_config->width;
	venc_config.src_height		  = media_config->height;
	venc_config.dst_width		  = jpeg_config->width;
	venc_config.dst_height		  = jpeg_config->height;
	venc_config.frame_rate		  = media_config->fps;
	venc_config.bit_rate		  = media_config->bitrate * 1024;
	venc_config.pixelformat		  = recoder->pixelformat;
	venc_config.max_keyframe_interval = media_config->gop;
	venc_config.rotate_angle = jpeg_config->rotate_angle;
	memcpy(&venc_config.venc_video_signal, &media_config->venc_video_signal, sizeof(RTVencVideoSignal));
	venc_config.rc_mode = VENC_COMP_RC_MODE_H264CBR;
	venc_config.en_encpp_sharp = media_config->enable_sharp;

	RT_LOGI("src_w&h = %d, %d; dst_w&h = %d, %d, pixelformat = %d", venc_config.src_width, venc_config.src_height,
		venc_config.dst_width, venc_config.dst_height, venc_config.pixelformat);

	comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Base, &venc_config);

	connect_flag = 1;

	comp_setup_tunnel(recoder->venc_comp, COMP_INPUT_PORT, recoder->vi_comp, connect_flag);

	comp_setup_tunnel(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->venc_comp, connect_flag);

	comp_init(recoder->venc_comp);

	comp_start(recoder->venc_comp);

	catch_jpeg_flag = 1;
	comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_CATCH_JPEG, (void *)&catch_jpeg_flag);

	return 0;
}

static int ioctl_output_yuv_catch_jpeg_stop(video_recoder *recoder)
{
	int connect_flag    = 0;
	int catch_jpeg_flag = 0;

	comp_pause(recoder->vi_comp);

	/* disconnect the tunnel */
	comp_setup_tunnel(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->venc_comp, connect_flag);

	comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_CATCH_JPEG, (void *)&catch_jpeg_flag);

	comp_start(recoder->vi_comp);

	/* destroy venc comp*/
	comp_pause(recoder->venc_comp);

	comp_stop(recoder->venc_comp);

	comp_free_handle(recoder->venc_comp);

	recoder->venc_comp = NULL;
	RT_LOGI(" recoder->venc_comp = %px", recoder->venc_comp);
	return 0;
}

static int ioctl_output_yuv_catch_jpeg_getData(video_recoder *recoder,
					       void *user_buf_info)
{
	video_stream_s *video_stream = NULL;
	int loop_count		     = 0;
	int loop_max_count	   = 200; /* mean 2 s*/

catch_jpeg_get_stream_data:

	video_stream = ioctl_get_stream_data(recoder);
	if (video_stream == NULL) {
		if (loop_count >= loop_max_count) {
			RT_LOGE("get stream data timeout");
			return -EFAULT;
		}

		RT_LOGD("catch_jpeg_getData: video stream is null\n");

		mutex_lock(&recoder->stream_buf_manager.mutex);
		recoder->stream_buf_manager.wait_bitstream_condition = 0;
		recoder->stream_buf_manager.need_wait_up_flag	= 1;
		mutex_unlock(&recoder->stream_buf_manager.mutex);

		wait_event_timeout(recoder->stream_buf_manager.wait_bitstream,
				   recoder->stream_buf_manager.wait_bitstream_condition, 1);

		loop_count++;
		goto catch_jpeg_get_stream_data;
	}

	RT_LOGI("video_stream->offset0 = %d, size0 = %d, data0 = %px",
		video_stream->offset0, video_stream->size0, video_stream->data0);

	copy_jpeg_data(user_buf_info, video_stream);

	ioctl_return_stream_data(recoder, video_stream);

	return 0;
}

static int ioctl_reset_encoder_type(video_recoder *recoder, int encoder_type)
{
	venc_comp_base_config venc_config;
	int connect_flag	   = 0;
	int ret			   = 0;
	comp_state_type comp_state = 0;

	memset(&venc_config, 0, sizeof(venc_comp_base_config));

	if (recoder->config.encodeType == encoder_type) {
		RT_LOGE("encoder type is the same, no need to reset: %d, %d",
			recoder->config.encodeType, encoder_type);
		return -1;
	}

	/* pause venc and vi comp */
	comp_get_state(recoder->venc_comp, &comp_state);
	RT_LOGI("state of venc_comp: %d", comp_state);

	if (comp_state != COMP_STATE_PAUSE)
		comp_pause(recoder->venc_comp);

	comp_get_state(recoder->vi_comp, &comp_state);
	RT_LOGI("state of vi_comp: %d", comp_state);

	if (comp_state != COMP_STATE_PAUSE)
		comp_pause(recoder->vi_comp);

	flush_stream_data(recoder);

	/* flush buffer of venc comp */
	comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_Flush_buffer, NULL);

	/* disconnect the tunnel */
	connect_flag = 0;
	comp_setup_tunnel(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->venc_comp, connect_flag);

	/* destroy venc comp*/
	comp_stop(recoder->venc_comp);

	comp_free_handle(recoder->venc_comp);

	recoder->venc_comp = NULL;

	/* create and init venc comp */
	ret = comp_get_handle((comp_handle *)&recoder->venc_comp,
			"video.encoder",
			recoder,
			&recoder->config,
			&venc_callback);
	if (ret != ERROR_TYPE_OK) {
		RT_LOGE("get venc comp handle error: %d", ret);
		return -1;
	}

	recoder->config.encodeType = encoder_type;

	fill_venc_config(&venc_config, recoder);

	comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Base, &venc_config);

	connect_flag = 1;

	comp_setup_tunnel(recoder->venc_comp, COMP_INPUT_PORT, recoder->vi_comp, connect_flag);

	comp_setup_tunnel(recoder->vi_comp, COMP_OUTPUT_PORT, recoder->venc_comp, connect_flag);

	comp_init(recoder->venc_comp);

	/* restart venc and vi comp*/
	//comp_start(recoder->venc_comp);

	//comp_start(recoder->vi_comp);

	return 0;
}
static int fops_mmap(struct file *filp, struct vm_area_struct *vma)
{
	//* todo

	return 0;
}

static int fops_open(struct inode *inode, struct file *filp)
{
	media_private_info *media_info;

	media_info = kmalloc(sizeof(media_private_info), GFP_KERNEL);
	if (!media_info)
		return -ENOMEM;

	memset(media_info, 0, sizeof(media_private_info));
	media_info->channel = -1;

	filp->private_data = media_info;

	nonseekable_open(inode, filp);
	return 0;
}

static int fops_release(struct inode *inode, struct file *filp)
{
	/* todo */
	media_private_info *media_info = filp->private_data;

	kfree(media_info);

	return 0;
}

static long fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/* todo */
	int ret			       = 0;
	int i = 0;
	media_private_info *media_info = filp->private_data;
	video_recoder *recoder	 = &rt_media_devp->recoder[media_info->channel];

	switch (cmd) {
	case IOCTL_CONFIG: {
		rt_media_config_s config;

		memset(&config, 0, sizeof(rt_media_config_s));

		if (copy_from_user(&config, (void __user *)arg, sizeof(struct rt_media_config_s))) {
			RT_LOGE("IOCTL_CONFIG copy_from_user fail\n");
			return -EFAULT;
		}

		if (rt_media_devp->bcsi_err) {
			RT_LOGE("csi is err\n");
			return -1;
		}

		if (config.channelId < 0 || config.channelId > (VIDEO_INPUT_CHANNEL_NUM - 1)) {
			RT_LOGE("IOCTL_CONFIG: channel[%d] is error", config.channelId);
			return -1;
		}

		RT_LOGD("IOCTL_CONFIG: media_info->channel = %d, config->channelId = %d",
			media_info->channel, config.channelId);

		if (media_info->channel == -1)
			media_info->channel = config.channelId;

		if (media_info->channel != config.channelId) {
			RT_LOGE("IOCTL_CONFIG: channel id not match = %d, %d",
				media_info->channel, config.channelId);
			return -1;
		}

		recoder = &rt_media_devp->recoder[media_info->channel];
		if (ioctl_config(recoder, &config) != 0) {
			RT_LOGE("config err");
			return -1;
		}
		RT_LOGD("IOCTL_CONFIG end");
		break;
	}
	case IOCTL_START: {
		RT_LOGD("IOCTL_START start %d", recoder->config.width);
		RT_LOGS("rt_media_devp->in_high_fps_status %d", rt_media_devp->in_high_fps_status);
		if (recoder->enable_high_fps == 1 && recoder->bReset_high_fps_finish == 0) {
			RT_LOGW("bReset_high_fps_finish is not 1, not start, state = %d", recoder->state);
			return EFAULT;
		}

		if (recoder->config.channelId == 1 && rt_media_devp->in_high_fps_status == 1) {
			rt_media_devp->need_start_recoder_1 = recoder;
			RT_LOGW("the high fps is not finish , not start the second recoder channel");
			return 0;
		}

		if (recoder->config.channelId == 2 && rt_media_devp->in_high_fps_status == 1) {
			rt_media_devp->need_start_recoder_2 = recoder;
			RT_LOGW("the high fps is not finish , not start the third recoder channel");
			return 0;
		}

	#if defined CONFIG_VIDEO_RT_MEDIA
		if (rt_media_devp->bcsi_status_set[media_info->channel]) {
			ioctl_start(recoder);
		} else {
			RT_LOGW("csi not ready.");
			return -1;
		}
	#else
		ioctl_start(recoder);
	#endif

		RT_LOGD("IOCTL_START end");
		break;
	}
	case IOCTL_PAUSE: {
		RT_LOGD("IOCTL_PAUSE start");
		ioctl_pause(recoder);
		RT_LOGD("IOCTL_PAUSE end");
		break;
	}
	case IOCTL_DESTROY: {
		RT_LOGD("IOCTL_DESTROY start");
		ioctl_destroy(recoder);
		RT_LOGD("IOCTL_DESTROY end");
		break;
	}
	case IOCTL_GET_VBV_BUFFER_INFO: {
		KERNEL_VBV_BUFFER_INFO vbv_buf_info;

		memset(&vbv_buf_info, 0, sizeof(vbv_buf_info));

		if (recoder->venc_comp == NULL) {
			RT_LOGE("IOCTL_GET_VBV_BUFFER_INFO: recoder->venc_comp is null\n");
			return -EFAULT;
		}

		if (ioctl_get_vbv_buf_info(recoder, &vbv_buf_info) != 0) {
			RT_LOGI("ioctl_get_vbv_buf_info failed");
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &vbv_buf_info, sizeof(KERNEL_VBV_BUFFER_INFO))) {
			RT_LOGE("IOCTL_GET_VBV_BUFFER_INFO copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_STREAM_HEADER_SIZE: {
		venc_comp_header_data header_data;

		memset(&header_data, 0, sizeof(venc_comp_header_data));

		if (recoder->venc_comp == NULL) {
			RT_LOGE("IOCTL_GET_STREAM_HEADER_SIZE: recoder->venc_comp is null\n");
			return -EFAULT;
		}

		if (ioctl_get_stream_header(recoder, &header_data) != 0) {
			RT_LOGW("ioctl_get_stream_header failed");
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &header_data.nLength, sizeof(int))) {
			RT_LOGE("IOCTL_GET_VBV_BUFFER_INFO copy_to_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("end IOCTL_GET_STREAM_HEADER_SIZE: header_data.nLength %d\n", header_data.nLength);
		break;
	}
	case IOCTL_GET_STREAM_HEADER_DATA: {
		venc_comp_header_data header_data;
		memset(&header_data, 0, sizeof(venc_comp_header_data));

		if (recoder->venc_comp == NULL) {
			RT_LOGE("IOCTL_GET_STREAM_HEADER_DATA: recoder->venc_comp is null\n");
			return -EFAULT;
		}

		if (ioctl_get_stream_header(recoder, &header_data) != 0) {
			RT_LOGW("ioctl_get_stream_header failed");
			return -EFAULT;
		}
		if (copy_to_user((void *)arg, header_data.pBuffer, header_data.nLength)) {
			RT_LOGE("IOCTL_GET_VBV_BUFFER_INFO copy_to_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("IOCTL_GET_STREAM_HEADER_DATA: %x %x %x %x %x %x",
			header_data.pBuffer[0],
			header_data.pBuffer[1],
			header_data.pBuffer[2],
			header_data.pBuffer[3],
			header_data.pBuffer[4],
			header_data.pBuffer[5]);
		break;
	}
	case IOCTL_GET_STREAM_DATA: {
		video_stream_s *video_stream = NULL;

		if (recoder->venc_comp == NULL) {
			RT_LOGE("IOCTL_GET_STREAM_DATA: recoder->venc_comp is null\n");
			return -EFAULT;
		}
		video_stream = ioctl_get_stream_data(recoder);
		if (video_stream == NULL) {
			RT_LOGD("IOCTL_GET_STREAM_DATA: video stream is null\n");

			mutex_lock(&recoder->stream_buf_manager.mutex);
			recoder->stream_buf_manager.wait_bitstream_condition = 0;
			recoder->stream_buf_manager.need_wait_up_flag	= 1;
			mutex_unlock(&recoder->stream_buf_manager.mutex);

			wait_event_timeout(recoder->stream_buf_manager.wait_bitstream,
					   recoder->stream_buf_manager.wait_bitstream_condition, 1);
			return -EFAULT;
		}

		RT_LOGD("video_stream->offset0 = %d, size0 = %d",
			video_stream->offset0, video_stream->size0);

		RT_LOGI("get stream, pts = %lld", video_stream->pts);
		if (copy_to_user((void *)arg, video_stream, sizeof(video_stream_s))) {
			RT_LOGE("IOCTL_GET_STREAM_DATA copy_to_user fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_RETURN_STREAM_DATA: {
		video_stream_s cur_stream;

		memset(&cur_stream, 0, sizeof(video_stream_s));

		if (recoder->venc_comp == NULL) {
			RT_LOGE("IOCTL_RETURN_STREAM_DATA: recoder->venc_comp is null\n");
			return -EFAULT;
		}

		if (copy_from_user(&cur_stream, (void __user *)arg, sizeof(struct video_stream_s))) {
			RT_LOGE("IOCTL_CONFIG copy_from_user fail\n");
			return -EFAULT;
		}

		ioctl_return_stream_data(recoder, &cur_stream);

		break;
	}
	case IOCTL_CATCH_JPEG_START: {
		catch_jpeg_config jpeg_config;
		memset(&jpeg_config, 0, sizeof(catch_jpeg_config));

		if (copy_from_user(&jpeg_config, (void __user *)arg, sizeof(struct catch_jpeg_config))) {
			RT_LOGE("IOCTL_CATCH_JPEG_START copy_from_user fail\n");
			return -EFAULT;
		}

		if (recoder->config.output_mode == OUTPUT_MODE_STREAM) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START, &jpeg_config);
			if (ret != 0) {
				RT_LOGE("COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START failed");
				return -EFAULT;
			}
		} else {
			ioctl_output_yuv_catch_jpeg_start(recoder, &jpeg_config);
		}

		break;
	}
	case IOCTL_CATCH_JPEG_STOP: {
		if (recoder->config.output_mode == OUTPUT_MODE_STREAM) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_CATCH_JPEG_STOP, 0);
			if (ret != 0) {
				RT_LOGE("COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START failed");
				return -EFAULT;
			}
		} else
			ioctl_output_yuv_catch_jpeg_stop(recoder);

		break;
	}
	case IOCTL_CATCH_JPEG_GET_DATA: {
		if (recoder->config.output_mode == OUTPUT_MODE_STREAM) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_CATCH_JPEG_GET_DATA, (void *)arg);
			if (ret != 0) {
				RT_LOGE("COMP_INDEX_VENC_CONFIG_CATCH_JPEG_GET_DATA failed");
				return -EFAULT;
			}
		} else
			return ioctl_output_yuv_catch_jpeg_getData(recoder, (void *)arg);

		break;
	}
	case IOCTL_SET_OSD: {
		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_OSD, (void *)arg);
		if (ret != 0) {
			RT_LOGE("IOCTL_SET_OSD failed");
			return -EFAULT;
		}
		break;
	}
//	case IOCTL_GET_BIN_IMAGE_DATA: {
//		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA, (void *)arg);
//		if (ret <= 0) {
//			RT_LOGE("IOCTL_GET_BIN_IMAGE_DATA failed");
//			return -EFAULT;
//		}
//		RT_LOGI("IOCTL_GET_BIN_IMAGE_DATA, ret = %d", ret);
//		return ret;
//	}
//	case IOCTL_GET_MV_INFO_DATA: {
//		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_MV_INFO_DATA, (void *)arg);
//		if (ret <= 0) {
//			RT_LOGE("IOCTL_GET_MV_INFO_DATA failed");
//			return -EFAULT;
//		}
//		RT_LOGI("IOCTL_GET_MV_INFO_DATA, ret = %d", ret);
//		return ret;
//	}
	case IOCTL_CONFIG_YUV_BUF_INFO: {
		if (copy_from_user(&recoder->yuv_buf_info, (void __user *)arg, sizeof(struct config_yuv_buf_info))) {
			RT_LOGE("IOCTL_CONFIG_YUV_BUF_INFO copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_SET_YUV_BUF_INFO, (void *)&recoder->yuv_buf_info);
		if (ret < 0) {
			RT_LOGE("IOCTL_CONFIG_YUV_BUF_INFO failed, ret = %d", ret);
			return -EFAULT;
		}

		break;
	}
	case IOCTL_REQUEST_YUV_FRAME: {
		rt_yuv_info s_yuv_info;

		memset(&s_yuv_info, 0, sizeof(rt_yuv_info));
		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_REQUEST_YUV_FRAME, (void *)&s_yuv_info);
		if (ret < 0) {
			RT_LOGE("IOCTL_REQUEST_YUV_FRAME failed, ret = %d", ret);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &s_yuv_info, sizeof(s_yuv_info))) {
			RT_LOGE("IOCTL_REQUEST_YUV_FRAME copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_RETURN_YUV_FRAME: {
		unsigned char *phy_addr = NULL;

		if (copy_from_user(&phy_addr, (void __user *)arg, sizeof(unsigned char *))) {
			RT_LOGE("IOCTL_RETURN_YUV_FRAME copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_RETURN_YUV_FRAME, (void *)phy_addr);
		if (ret < 0) {
			RT_LOGE("IOCTL_RETURN_YUV_FRAME failed, ret = %d", ret);
			return -EFAULT;
		}

		break;
	}
	case IOCTL_GET_ISP_LV: {
		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_LV, NULL);
		if (ret < 0) {
			RT_LOGE("IOCTL_GET_ISP_LV failed, ret = %d", ret);
			return -EFAULT;
		}
		return ret;
	}
	case IOCTL_GET_ISP_HIST: {
		unsigned int hist[256];

		memset(&hist, 0, 256 * sizeof(unsigned int));

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_HIST, (void *)&hist[0]);
		if (ret < 0) {
			RT_LOGE("IOCTL_GET_ISP_HIST failed, ret = %d", ret);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &hist[0], 256 * sizeof(unsigned int))) {
			RT_LOGE("IOCTL_GET_ISP_HIST copy_to_user fail\n");
			return -EFAULT;
		}
		return ret;
	}
	case IOCTL_GET_ISP_EXP_GAIN: {
		RTIspExpGain exp_gain;

		memset(&exp_gain, 0, sizeof(RTIspExpGain));

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_EXP_GAIN, (void *)&exp_gain);
		if (ret < 0) {
			RT_LOGE("IOCTL_GET_ISP_EXP_GAIN failed, ret = %d", ret);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &exp_gain, sizeof(RTIspExpGain))) {
			RT_LOGE("IOCTL_GET_ISP_EXP_GAIN copy_to_user fail\n");
			return -EFAULT;
		}
		return ret;
	}
	case IOCTL_SET_ISP_ATTR_CFG: {
		RTIspCtrlAttr ctrlattr;

		memset(&ctrlattr, 0, sizeof(RTIspCtrlAttr));

		if (copy_from_user(&ctrlattr, (void __user *)arg, sizeof(RTIspCtrlAttr))) {
			RT_LOGE("IOCTL_SET_ISP_ATTR_CFG copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("IOCTL_SET_ISP_ATTR_CFG:ctrl id:%d\n", ctrlattr.isp_attr_cfg.cfg_id);

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_ISP_ARRT_CFG, (void *)&ctrlattr);
		if (ret < 0) {
			RT_LOGE("COMP_INDEX_VI_CONFIG_Dynamic_SET_ISP_ARRT_CFG failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_ISP_ATTR_CFG: {
		RTIspCtrlAttr ctrlattr;

		memset(&ctrlattr, 0, sizeof(RTIspCtrlAttr));

		if (copy_from_user(&ctrlattr, (void __user *)arg, sizeof(RTIspCtrlAttr))) {
			RT_LOGE("IOCTL_GET_ISP_ATTR_CFG copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("IOCTL_GET_ISP_ATTR_CFG:ctrl id:%d\n", ctrlattr.isp_attr_cfg.cfg_id);

		ret = comp_get_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_GET_ISP_ARRT_CFG, (void *)&ctrlattr);
		if (ret < 0) {
			RT_LOGE("COMP_INDEX_VI_CONFIG_Dynamic_GET_ISP_ARRT_CFG failed, ret = %d", ret);
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &ctrlattr, sizeof(RTIspCtrlAttr))) {
			RT_LOGE("IOCTL_GET_ISP_ATTR_CFG copy_to_user fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_SET_ISP_ORL: {
		RTIspOrl isp_orl;

		memset(&isp_orl, 0, sizeof(RTIspOrl));

		if (copy_from_user(&isp_orl, (void __user *)arg, sizeof(RTIspOrl))) {
			RT_LOGE("IOCTL_SET_ISP_ATTR_CFG copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_ORL, (void *)&isp_orl);
		if (ret < 0) {
			RT_LOGE("COMP_INDEX_VI_CONFIG_Dynamic_SET_ORL failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_ISP_AE_METERING_MODE: {
		int ae_metering_mode = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_AE_METERING_MODE, (void *)&ae_metering_mode);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_POWER_LINE_FREQ failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_IR_PARAM: {
		RTIrParam mIrParam;
		int is_night_case = 0;

		memset(&mIrParam, 0, sizeof(RTIrParam));

		if (copy_from_user(&mIrParam, (void __user *)arg, sizeof(RTIrParam))) {
			RT_LOGE("IOCTL_SET_IR_COLOR2GREY copy_from_user fail\n");
			return -EFAULT;
		}

		if (mIrParam.grey == 1)
			is_night_case = 1;

		RT_LOGW("is_night_case = %d, venc_comp = %px", is_night_case, recoder->venc_comp);

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_IS_NIGHT_CASE, (void *)&is_night_case);
			if (ret < 0)
				RT_LOGW("VENC_CONFIG_Dynamic_SET_IS_NIGHT_CASE failed, ret = %d", ret);
		}

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_IR_PARAM, (void *)&mIrParam);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_IR_COLOR2GREY failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_HORIZONTAL_FLIP: {
		int bhflip = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_H_FLIP, (void *)&bhflip);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_HORIZONTAL_FLIP failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_VERTICAL_FLIP: {
		int bvflip = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_V_FLIP, (void *)&bvflip);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_VERTICAL_FLIP failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_FORCE_KEYFRAME: {
		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_ForceKeyFrame, NULL);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_FORCE_KEYFRAME failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_RESET_ENCODER_TYPE: {
		int encoder_type = (int)arg;

		ret = ioctl_reset_encoder_type(recoder, encoder_type);
		if (ret != 0) {
			RT_LOGE("reset encoder type, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_REQUEST_ENC_YUV_FRAME: {
		unsigned char *phy_addr = NULL;

		if (recoder->enc_yuv_phy_addr == NULL) {
			//RT_LOGI(" enc_yuv_phy_addr is null");
			return -1;
		}

		phy_addr		  = recoder->enc_yuv_phy_addr;
		recoder->enc_yuv_phy_addr = NULL;
		RT_LOGD("set recoder->enc_yuv_phy_addr = NULL");
		if (copy_to_user((void *)arg, &phy_addr, sizeof(unsigned char *))) {
			RT_LOGE("IOCTL_REQUEST_ENC_YUV_FRAME copy_to_user fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_SUBMIT_ENC_YUV_FRAME: {
		unsigned char *phy_addr = NULL;
		comp_buffer_header_type mbuffer_header;
		video_frame_s mvideo_frame;
		int align_width  = RT_ALIGN(recoder->config.width, 16);
		int align_height = RT_ALIGN(recoder->config.height, 16);
		int y_size       = align_width * align_height;

		memset(&mbuffer_header, 0, sizeof(comp_buffer_header_type));
		memset(&mvideo_frame, 0, sizeof(video_frame_s));

		if (copy_from_user(&phy_addr, (void __user *)arg, sizeof(unsigned char *))) {
			RT_LOGE("IOCTL_RETURN_YUV_FRAME copy_from_user fail\n");
			return -EFAULT;
		}
		mvideo_frame.phy_addr[0] = phy_addr;
		mvideo_frame.phy_addr[1] = phy_addr + y_size;
		mbuffer_header.private   = &mvideo_frame;
		comp_empty_this_in_buffer(recoder->venc_comp, &mbuffer_header);

		break;
	}
	case IOCTL_SET_POWER_LINE_FREQ: {
		int power_line_freq = (int)arg;

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_POWER_LINE_FREQ, (void *)&power_line_freq);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_POWER_LINE_FREQ failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_QP_RANGE: {
		video_qp_range qp_range;

		if (copy_from_user(&qp_range, (void __user *)arg, sizeof(video_qp_range))) {
			RT_LOGE("IOCTL_SET_QP_RANGE copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_QP_RANGE, (void *)&qp_range);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_QP_RANGE failed, ret = %d", ret);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_BITRATE: {
		int bitrate = ((int)arg) * 1024;

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_BITRATE, (void *)&bitrate);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_BITRATE failed, ret = %d, bitrate = %d", ret, bitrate);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_FPS: {
		int fps = ((int)arg);

		if (fps <= 0 || fps > recoder->max_fps) {
			RT_LOGW("IOCTL_SET_FPS: invalid fps[%d], setup to max_fps[%d]", fps, recoder->max_fps);
			fps = recoder->max_fps;
		}

		RT_LOGI("IOCTL_SET_FPS: fps = %d, max_fps = %d, vi_comp = %px, venc_comp = %px",
			fps, recoder->max_fps, recoder->vi_comp, recoder->venc_comp);

		ret = comp_set_config(recoder->vi_comp, COMP_INDEX_VI_CONFIG_Dynamic_SET_FPS, (void *)&fps);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_FPS failed, ret = %d, fps = %d", ret, fps);
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS, (void *)&fps);
		if (ret < 0) {
			RT_LOGE("IOCTL_SET_FPS failed, ret = %d, fps = %d", ret, fps);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_VBR_PARAM: {
		RTVencVbrParam rt_vbr_param;

		memset(&rt_vbr_param, 0, sizeof(RTVencVbrParam));

		if (copy_from_user(&rt_vbr_param, (void __user *)arg, sizeof(RTVencVbrParam))) {
			RT_LOGE("IOCTL_SET_VBR_PARAM copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_VBR_PARAM, (void *)&rt_vbr_param);
		if (ret != 0) {
			RT_LOGE("VENC SET_VBR_PARAM fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_GET_SUM_MB_INFO: {
		RTVencMBSumInfo rt_mb_sum;

		memset(&rt_mb_sum, 0, sizeof(RTVencMBSumInfo));

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_GET_SUM_MB_INFO, (void *)&rt_mb_sum);
		if (ret != 0) {
			RT_LOGE("VENC GET_SUM_MB_INFO fail\n");
			return -EFAULT;
		}

		if (copy_to_user((void *)arg, &rt_mb_sum, sizeof(RTVencMBSumInfo))) {
			RT_LOGE("IOCTL_REQUEST_ENC_YUV_FRAME copy_to_user fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_SET_IPC_CASE: {
		int is_ipc_case = 0;

		if (copy_from_user(&is_ipc_case, (void __user *)arg, sizeof(int))) {
			RT_LOGE("IOCTL_SET_IPC_CASE copy_from_user fail\n");
			return -EFAULT;
		}
		RT_LOGI("is_ipc_case %d", is_ipc_case);
		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_IPC_CASE, (void *)&is_ipc_case);
		if (ret != 0) {
			RT_LOGE("VENC IOCTL_SET_IPC_CASE fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_SET_ENC_MOTION_SEARCH_PARAM: {
		RTVencMotionSearchParam *pmotion_search_param = &recoder->motion_search_param;
		int total_region_num;

		if (copy_from_user(pmotion_search_param, (void __user *)arg, sizeof(RTVencMotionSearchParam))) {
			RT_LOGE("IOCTL_SET_ENC_MOTION_SEARCH_PARAM copy_from_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("motion_search_param hor_region_num %d, ver_region_num %d", recoder->motion_search_param.hor_region_num, recoder->motion_search_param.ver_region_num);

		if (0 == recoder->motion_search_param.en_motion_search) {
			if (recoder->motion_region) {
				kfree(recoder->motion_region);
				recoder->motion_region = NULL;
				recoder->motion_region_len = 0;
			}
		} else {
			if (0 == recoder->motion_search_param.dis_default_para) {
				total_region_num = MOTION_TOTAL_REGION_NUM_DEFAULT;
			} else {
				if ((0 == recoder->motion_search_param.hor_region_num) || (0 == recoder->motion_search_param.ver_region_num)) {
					RT_LOGE("IOCTL_SET_ENC_MOTION_SEARCH_PARAM invalid params, %d %d\n", recoder->motion_search_param.hor_region_num,
						recoder->motion_search_param.ver_region_num);
					return -EFAULT;
				} else {
					total_region_num = recoder->motion_search_param.hor_region_num * recoder->motion_search_param.ver_region_num;
				}
			}

			recoder->motion_region_len = total_region_num * sizeof(RTVencMotionSearchRegion);
			RT_LOGI("total_region_num %d, motion_region_len %d", total_region_num, recoder->motion_region_len);

			// free motion_region when disable motion_search or ioctl_destroy
			recoder->motion_region = kmalloc(recoder->motion_region_len, GFP_KERNEL);
			if (recoder->motion_region == NULL) {
				RT_LOGE("IOCTL_SET_ENC_MOTION_SEARCH_PARAM: kmalloc fail\n");
				return -EFAULT;
			}
			memset(recoder->motion_region, 0, recoder->motion_region_len);
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_MOTION_SEARCH_PARAM, (void *)pmotion_search_param);
		if (ret != 0) {
			RT_LOGE("VENC COMP_INDEX_VENC_CONFIG_SET_MOTION_SEARCH_PARAM fail\n");
			if (recoder->motion_region) {
				kfree(recoder->motion_region);
				recoder->motion_region = NULL;
				recoder->motion_region_len = 0;
			}
			return -EFAULT;
		}

		break;
	}
	case IOCTL_GET_ENC_MOTION_SEARCH_RESULT: {
		RTVencMotionSearchResult motion_result;
		RTVencMotionSearchResult motion_result_from_user;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		if (recoder->motion_region == NULL || 0 == recoder->motion_region_len) {
			RT_LOGE("IOCTL_GET_ENC_MOTION_SEARCH_RESULT: invalid motion_region\n");
			return -EFAULT;
		}

		memset(&motion_result_from_user, 0, sizeof(RTVencMotionSearchResult));
		if (copy_from_user(&motion_result_from_user, (void __user *)arg, sizeof(RTVencMotionSearchResult))) {
			RT_LOGE("motion_result_from_user copy_from_user fail\n");
			return -EFAULT;
		}

		memset(&motion_result, 0, sizeof(RTVencMotionSearchResult));
		motion_result.region = recoder->motion_region;

		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT, (void *)&motion_result);
		if (ret != 0) {
			RT_LOGE("VENC COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT fail\n");
			return -EFAULT;
		}

		if (copy_to_user((void *)motion_result_from_user.region, motion_result.region, recoder->motion_region_len)) {
			RT_LOGE("region copy_to_user fail\n");
			return -EFAULT;
		}
		motion_result.region = motion_result_from_user.region;
		if (copy_to_user((void *)arg, &motion_result, sizeof(RTVencMotionSearchResult))) {
			RT_LOGE("motion_result copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_VENC_SUPER_FRAME_PARAM: {
		RTVencSuperFrameConfig super_frame_config;

		memset(&super_frame_config, 0, sizeof(RTVencSuperFrameConfig));

		if (copy_from_user(&super_frame_config, (void __user *)arg, sizeof(RTVencSuperFrameConfig))) {
			RT_LOGE("IOCTL_SET_VENC_SUPER_FRAME_PARAM copy_from_user fail\n");
			return -EFAULT;
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_SET_SUPER_FRAME_PARAM, (void *)&super_frame_config);
		if (ret != 0) {
			RT_LOGE("VENC SET_SUPER_FRAME_PARAM fail\n");
			return -EFAULT;
		}

		break;
	}
	case IOCTL_SET_SHARP: {
		int bSharp = (int)arg;
		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_SHARP, (void *)&bSharp);
		if (ret != 0) {
			RT_LOGE("VENC COMP_INDEX_VENC_CONFIG_SET_SHARP fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_GET_CSI_STATUS: {
		//not buildin mode, such ko mode, this is invalid
		int i = 0;
		for (; i < VIDEO_INPUT_CHANNEL_NUM; i++) {
		#if defined CONFIG_VIDEO_RT_MEDIA//build in mode
			if (g_csi_status_pair[i].bset) {
				rt_media_devp->bcsi_status_set[i] = g_csi_status_pair[i].status;
			}
		#else//not for real value, only compatibility
			rt_media_devp->bcsi_status_set[i] = 1;
		#endif
		}
		if (copy_to_user((void *)arg, rt_media_devp->bcsi_status_set, sizeof(int) * VIDEO_INPUT_CHANNEL_NUM)) {
			RT_LOGE("IOCTL_GET_CSI_STATUS copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_ROI: {
		RTVencROIConfig sRoiConfig[MAX_ROI_AREA];

		memset(sRoiConfig, 0, sizeof(RTVencROIConfig) * MAX_ROI_AREA);

		if (copy_from_user(sRoiConfig, (void __user *)arg, sizeof(RTVencROIConfig) * MAX_ROI_AREA)) {
			RT_LOGE("IOCTL_SET_ROI copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp) {
			for (i = 0; i < MAX_ROI_AREA; i++) {
				comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_ROI, &sRoiConfig[i]);
			}
		}
		break;
	}
	case IOCTL_SET_GDC: {
		RTsGdcParam sGdcConfig;

		memset(&sGdcConfig, 0, sizeof(RTsGdcParam));

		if (copy_from_user(&sGdcConfig, (void __user *)arg, sizeof(RTsGdcParam))) {
			RT_LOGE("IOCTL_SET_GDC copy_from_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("eMountMode %d peak_weights_strength %d peak_m %d %d", sGdcConfig.eMountMode, sGdcConfig.peak_weights_strength, sGdcConfig.peak_m, sGdcConfig.th_strong_edge);
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_GDC, &sGdcConfig);
		break;
	}
	case IOCTL_SET_ROTATE: {
		unsigned int rotate_angle = (unsigned int)arg;

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_ROTATE, &rotate_angle);
		break;
	}
	case IOCTL_SET_REC_REF_LBC_MODE: {
		RTeVeLbcMode rec_lbc_mode = (RTeVeLbcMode)arg;
		RT_LOGD("rec_lbc_mode %d\n", rec_lbc_mode);

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_REC_REF_LBC_MODE, &rec_lbc_mode);
		break;
	}
	case IOCTL_SET_WEAK_TEXT_TH: {
		RTVenWeakTextTh WeakTextTh;

		memset(&WeakTextTh, 0, sizeof(RTVenWeakTextTh));

		if (copy_from_user(&WeakTextTh, (void __user *)arg, sizeof(RTVenWeakTextTh))) {
			RT_LOGE("IOCTL_SET_WEAK_TEXT_TH copy_from_user fail\n");
			return -EFAULT;
		}

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		if (recoder->venc_comp && WeakTextTh.en_weak_text_th) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_WEAK_TEXT_TH, (void *)&WeakTextTh.weak_text_th);
			if (ret != 0) {
				RT_LOGE("COMP_INDEX_VENC_CONFIG_SET_VE2ISP_D2D_LIMIT fail\n");
				return -EFAULT;
			}
		}

		break;
	}
	case IOCTL_SET_REGION_D3D_PARAM: {
		RTVencRegionD3DParam *pregion_d3d_param = &recoder->region_d3d_param;
		int total_region_num;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		if (copy_from_user(pregion_d3d_param, (void __user *)arg, sizeof(RTVencRegionD3DParam))) {
			RT_LOGE("IOCTL_SET_REGION_D3D_PARAM copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("RegionD3D Region:%dx%d, Expand:%dx%d, Coef:%d,{%d,%d,%d},{%d,%d,%d,%d}\n",
			pregion_d3d_param->hor_region_num,
			pregion_d3d_param->ver_region_num,
			pregion_d3d_param->hor_expand_num,
			pregion_d3d_param->ver_expand_num,
			pregion_d3d_param->chroma_offset,
			pregion_d3d_param->static_coef[0],
			pregion_d3d_param->static_coef[1],
			pregion_d3d_param->static_coef[2],
			pregion_d3d_param->motion_coef[0],
			pregion_d3d_param->motion_coef[1],
			pregion_d3d_param->motion_coef[2],
			pregion_d3d_param->motion_coef[3]);

		if (recoder->region_d3d_param.en_region_d3d == 0) {
			if (recoder->region_d3d_region) {
				kfree(recoder->region_d3d_region);
				recoder->region_d3d_region = NULL;
				recoder->region_d3d_region_len = 0;
			}
		} else {
			if (recoder->region_d3d_param.dis_default_para == 0) {
				total_region_num = REGION_D3D_TOTAL_REGION_NUM_DEFAULT;
			} else {
				if ((recoder->region_d3d_param.hor_region_num == 0) || (recoder->region_d3d_param.ver_region_num == 0)) {
					RT_LOGE("IOCTL_SET_REGION_D3D_PARAM invalid params, %d %d\n", recoder->region_d3d_param.hor_region_num,
						recoder->region_d3d_param.ver_region_num);
					return -EFAULT;
				} else {
					total_region_num = recoder->region_d3d_param.hor_region_num * recoder->region_d3d_param.ver_region_num;
				}
			}

			recoder->region_d3d_region_len = total_region_num * sizeof(RTVencRegionD3DRegion);
			RT_LOGI("total_region_num %d, region_d3d_region_len %d", total_region_num, recoder->region_d3d_region_len);

			// free region_d3d_region when disable region_d3d or ioctl_destroy
			recoder->region_d3d_region = kmalloc(recoder->region_d3d_region_len, GFP_KERNEL);
			if (recoder->region_d3d_region == NULL) {
				RT_LOGE("IOCTL_SET_REGION_D3D_PARAM: kmalloc fail\n");
				return -EFAULT;
			}
			memset(recoder->region_d3d_region, 0, recoder->region_d3d_region_len);
		}

		ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_REGION_D3D_PARAM, (void *)pregion_d3d_param);
		if (ret != 0) {
			RT_LOGE("VENC COMP_INDEX_VENC_CONFIG_SET_REGION_D3D_PARAM fail\n");
			if (recoder->region_d3d_region) {
				kfree(recoder->region_d3d_region);
				recoder->region_d3d_region = NULL;
				recoder->region_d3d_region_len = 0;
			}
			return -EFAULT;
		}

		break;
	}
	case IOCTL_GET_REGION_D3D_RESULT: {
		RTVencRegionD3DResult region_d3d_result;
		RTVencRegionD3DResult region_d3d_result_from_user;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		if (recoder->region_d3d_region == NULL || 0 == recoder->region_d3d_region_len) {
			RT_LOGE("IOCTL_GET_REGION_D3D_RESULT: invalid region_d3d_region\n");
			return -EFAULT;
		}

		memset(&region_d3d_result_from_user, 0, sizeof(RTVencRegionD3DResult));
		if (copy_from_user(&region_d3d_result_from_user, (void __user *)arg, sizeof(RTVencRegionD3DResult))) {
			RT_LOGE("region_d3d_result_from_user copy_from_user fail\n");
			return -EFAULT;
		}

		memset(&region_d3d_result, 0, sizeof(RTVencRegionD3DResult));
		region_d3d_result.region = recoder->region_d3d_region;

		ret = comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_Dynamic_GET_REGION_D3D_RESULT, (void *)&region_d3d_result);
		if (ret != 0) {
			RT_LOGE("VENC COMP_INDEX_VENC_CONFIG_Dynamic_GET_REGION_D3D_RESULT fail\n");
			return -EFAULT;
		}

		if (copy_to_user((void *)region_d3d_result_from_user.region, region_d3d_result.region, recoder->region_d3d_region_len)) {
			RT_LOGE("region copy_to_user fail\n");
			return -EFAULT;
		}
		region_d3d_result.region = region_d3d_result_from_user.region;
		if (copy_to_user((void *)arg, &region_d3d_result, sizeof(RTVencRegionD3DResult))) {
			RT_LOGE("region_d3d_result copy_to_user fail\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SET_CHROMA_QP_OFFSET: {
		int nChromaQPOffset = (int)arg;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}
		RT_LOGD("nChromaQPOffset %d\n", nChromaQPOffset);

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_CHROMA_QP_OFFSET, &nChromaQPOffset);
		break;
	}
	case IOCTL_SET_H264_CONSTRAINT_FLAG: {
		RTVencH264ConstraintFlag ConstraintFlag;

		if (recoder->config.encodeType != 0) {
			RT_LOGW("only support h.264, return\n");
			return -1;
		}

		memset(&ConstraintFlag, 0, sizeof(RTVencH264ConstraintFlag));

		if (copy_from_user(&ConstraintFlag, (void __user *)arg, sizeof(RTVencH264ConstraintFlag))) {
			RT_LOGE("IOCTL_SET_H264_CONSTRAINT_FLAG copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("constraint:%d %d %d  %d %d %d", ConstraintFlag.constraint_0, ConstraintFlag.constraint_1,
			ConstraintFlag.constraint_2, ConstraintFlag.constraint_3, ConstraintFlag.constraint_4, ConstraintFlag.constraint_5);

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_H264_CONSTRAINT_FLAG, (void *)&ConstraintFlag);
			if (ret != 0) {
				RT_LOGE("COMP_INDEX_VENC_CONFIG_SET_H264_CONSTRAINT_FLAG fail\n");
				return -EFAULT;
			}
		}

		break;
	}
	case IOCTL_SET_VE2ISP_D2D_LIMIT: {
		RTVencVe2IspD2DLimit D2DLimit;

		if (recoder->config.encodeType == 1) {
			RT_LOGW("not support jpg, return\n");
			return -1;
		}

		memset(&D2DLimit, 0, sizeof(RTVencVe2IspD2DLimit));

		if (copy_from_user(&D2DLimit, (void __user *)arg, sizeof(RTVencVe2IspD2DLimit))) {
			RT_LOGE("IOCTL_SET_VE2ISP_D2D_LIMIT copy_from_user fail\n");
			return -EFAULT;
		}

		RT_LOGD("en_d2d_limit:%d, d2d_level:%d %d %d %d %d %d", D2DLimit.en_d2d_limit,
			D2DLimit.d2d_level[0], D2DLimit.d2d_level[1], D2DLimit.d2d_level[2],
			D2DLimit.d2d_level[3], D2DLimit.d2d_level[4], D2DLimit.d2d_level[5]);

		if (recoder->venc_comp) {
			ret = comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_VE2ISP_D2D_LIMIT, (void *)&D2DLimit);
			if (ret != 0) {
				RT_LOGE("COMP_INDEX_VENC_CONFIG_SET_VE2ISP_D2D_LIMIT fail\n");
				return -EFAULT;
			}
		}

		break;
	}
	case IOCTL_SET_GRAY: {
		unsigned int enable_gray = (unsigned int)arg;

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_GRAY, &enable_gray);
		break;
	}
	case IOCTL_SET_WB_YUV: {
		RTsWbYuvParam sWbyuvConfig;

		memset(&sWbyuvConfig, 0, sizeof(RTsWbYuvParam));

		if (copy_from_user(&sWbyuvConfig, (void __user *)arg, sizeof(RTsWbYuvParam))) {
			RT_LOGE("IOCTL_SET_WB_YUV copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_WBYUV, &sWbyuvConfig);
		RT_LOGD("bEnableWbYuv %d ", sWbyuvConfig.bEnableWbYuv);
		break;
	}
	case IOCTL_GET_WB_YUV: {
		RTVencThumbInfo s_wbyuv_info;

		memset(&s_wbyuv_info, 0, sizeof(RTVencThumbInfo));

		if (copy_from_user(&s_wbyuv_info, (void __user *)arg, sizeof(RTVencThumbInfo))) {
			RT_LOGE("IOCTL_GET_WB_YUV copy_from_user fail\n");
			return -EFAULT;
		}
		RT_LOGD("nThumbSize %d %px", s_wbyuv_info.nThumbSize, s_wbyuv_info.pThumbBuf);
		if (recoder->venc_comp)
			comp_get_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_GET_WBYUV, &s_wbyuv_info);
		break;
	}
	case IOCTL_SET_2DNR: {
		RTs2DfilterParam s_2dnr_param;

		memset(&s_2dnr_param, 0, sizeof(RTs2DfilterParam));

		if (copy_from_user(&s_2dnr_param, (void __user *)arg, sizeof(RTs2DfilterParam))) {
			RT_LOGE("IOCTL_SET_2DNR copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_2DNR, &s_2dnr_param);
		break;
	}
	case IOCTL_SET_3DNR: {
		RTs3DfilterParam s_3dnr_param;

		memset(&s_3dnr_param, 0, sizeof(RTs3DfilterParam));

		if (copy_from_user(&s_3dnr_param, (void __user *)arg, sizeof(RTs3DfilterParam))) {
			RT_LOGE("IOCTL_SET_3DNR copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_3DNR, &s_3dnr_param);
		break;
	}
	case IOCTL_SET_CYCLIC_INTRA_REFRESH: {
		RTVencCyclicIntraRefresh s_intra_refresh;

		memset(&s_intra_refresh, 0, sizeof(RTVencCyclicIntraRefresh));

		if (copy_from_user(&s_intra_refresh, (void __user *)arg, sizeof(RTVencCyclicIntraRefresh))) {
			RT_LOGE("IOCTL_SET_CYCLIC_INTRA_REFRESH copy_from_user fail\n");
			return -EFAULT;
		}
		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_CYCLE_INTRA_REFRESH, &s_intra_refresh);
		break;
	}
	case IOCTL_SET_P_FRAME_INTRA: {
		unsigned int enable_p_intra = (unsigned int)arg;

		if (recoder->venc_comp)
			comp_set_config(recoder->venc_comp, COMP_INDEX_VENC_CONFIG_SET_P_FRAME_INTRA, &enable_p_intra);
		break;
	}
	default: {
		RT_LOGE("not support cmd: %d", cmd);
		break;
	}
	}

	return 0;
}

static const struct file_operations rt_media_fops = {
	.owner		= THIS_MODULE,
	.mmap		= fops_mmap,
	.open		= fops_open,
	.release	= fops_release,
	.llseek		= no_llseek,
	.unlocked_ioctl = fops_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = fops_ioctl,
#endif
};

#if defined(CONFIG_OF)
static struct of_device_id rt_media_match[] = {
	{
		.compatible = "allwinner,rt-media",
	},
	{}
};
MODULE_DEVICE_TABLE(of, rt_media_match);
#endif

#ifdef CONFIG_PM
static int rt_media_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* todo */
	RT_LOGD("[cedar] standby suspend\n");

	return 0;
}

static int rt_media_resume(struct platform_device *pdev)
{
	/* todo */
	RT_LOGD("[cedar] standby resume\n");
	return 0;
}
#endif
/*
static int rt_is_sensor_0(int vipp_num)
{
	if (vipp_num == CSI_SENSOR_0_VIPP_0
		|| vipp_num == CSI_SENSOR_0_VIPP_1
		|| vipp_num == CSI_SENSOR_0_VIPP_2
	)
		return 1;

	return 0;
}

static int rt_is_sensor_1(int vipp_num)
{
	if (vipp_num == CSI_SENSOR_1_VIPP_0
		|| vipp_num == CSI_SENSOR_1_VIPP_1
		|| vipp_num == CSI_SENSOR_1_VIPP_2
	)
		return 1;

	return 0;
}
*/

int rt_vin_is_ready(int channel_id)
{
	int ret = 0;
	RT_LOGD("rt_vin_is_ready vinc%d", channel_id);
	if (rt_media_devp) {
		video_recoder *recoder = &rt_media_devp->recoder[channel_id];
		rt_media_devp->bcsi_status_set[channel_id] = 1;
		if (recoder->bsetup_recorder_finish) {
			ioctl_start(recoder);
		} else {
			RT_LOGW("%d rt_media thread not ready", channel_id);
		}
		recoder->bvin_is_ready = 1;
	} else {
		RT_LOGW("%d rt_media not probe yet", channel_id);
		bvin_is_ready_rt_not_probe[channel_id] = 1;
		g_csi_status_pair[channel_id].bset = 1;
		g_csi_status_pair[channel_id].status = 1;
	}

	return ret;
}
EXPORT_SYMBOL(rt_vin_is_ready);

#if defined CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL && defined CONFIG_VIDEO_RT_MEDIA

static void set_fps_change_channel_id_1(int id)
{
	fps_change_id_1 = id;
}

static int wdr_get_from_partition(struct vi_part_cfg *vi_part_cfg, int sensor_num)
{
#if defined CONFIG_ISP_SERVER_MELIS
	int ret     = 0;
	void *vaddr = NULL;
	int check_sign = -1;
	SENSOR_ISP_CONFIG_S *sensor_isp_cfg;

	if (sensor_num == 1) {
		check_sign = SENSOR_1_SIGN;
		vaddr = vin_map_kernel(VIN_SENSOR1_RESERVE_ADDR, VIN_RESERVE_SIZE);
	} else {
		check_sign = SENSOR_0_SIGN;
		vaddr = vin_map_kernel(VIN_SENSOR0_RESERVE_ADDR, VIN_RESERVE_SIZE);
	}

	if (vaddr == NULL) {
		RT_LOGE("%s:map 0x%x paddr err!!!", __func__, VIN_SENSOR0_RESERVE_ADDR);
		ret = -EFAULT;
		goto ekzalloc;
	}

	sensor_isp_cfg = (SENSOR_ISP_CONFIG_S *)vaddr;

	/* check id */
	if (sensor_isp_cfg->sign != check_sign) {
		RT_LOGW("%s:sign is 0x%x but not 0x%x\n", __func__, sensor_isp_cfg->sign, check_sign);
		ret = -EINVAL;
		goto unmap;
	}

	vi_part_cfg->wdr = sensor_isp_cfg->wdr_mode > 1 ? 0 : sensor_isp_cfg->wdr_mode;
	vi_part_cfg->width = sensor_isp_cfg->width;
	vi_part_cfg->height = sensor_isp_cfg->height;
	vi_part_cfg->venc_format = sensor_isp_cfg->venc_format > 2 ? 1 : (sensor_isp_cfg->venc_format == 0 ? 1 : sensor_isp_cfg->venc_format);
	vi_part_cfg->flicker_mode = sensor_isp_cfg->flicker_mode > 2 ? 1 : sensor_isp_cfg->flicker_mode;

	vi_part_cfg->fps = sensor_isp_cfg->fps <= 0 ? 15 : sensor_isp_cfg->fps;

	RT_LOGS("wdr/width/height/vformat/flicker get from partiton is %d/%d/%d/%d %d %d %d\n",
		sensor_isp_cfg->wdr_mode, sensor_isp_cfg->width,
		sensor_isp_cfg->height, sensor_isp_cfg->venc_format,
		vi_part_cfg->flicker_mode, vi_part_cfg->fps, sensor_isp_cfg->fps);

//sensor_isp_cfg->sign = 0xFFFFFFFF;

unmap:
	vin_unmap_kernel(vaddr);
ekzalloc:
	return ret;
#else
	return 0;
#endif
}


void rt_vin_sensor_fps_change_callback(int channel_id)
{
	set_fps_change_channel_id_1(channel_id);
	vin_sensor_fps_change_callback(channel_id, rt_media_csi_fps_change_first);
}

static int rt_which_sensor(int vipp_num)
{
	switch (vipp_num) {
	case CSI_SENSOR_0_VIPP_0:
	case CSI_SENSOR_0_VIPP_1:
	case CSI_SENSOR_0_VIPP_2:
		return 0;
	case CSI_SENSOR_1_VIPP_0:
	case CSI_SENSOR_1_VIPP_1:
	case CSI_SENSOR_1_VIPP_2:
		return 1;
	default:
		return 0;//default sensor 0
	}

	return 0;//default sensor 0
}

static int setup_recoder_config(int channel_id)
{
	int ret			= 0;
	video_recoder *recoder  = &rt_media_devp->recoder[channel_id];
	rt_media_config_s config;
	struct vi_part_cfg vi_part_cfg;

	memset(&config, 0, sizeof(rt_media_config_s));
	memset(&vi_part_cfg, 0, sizeof(struct vi_part_cfg));
	config.channelId	 = channel_id;
	config.encodeType	= MAIN_CHANNEL_ENCODE_TYPE;
	config.width  = MAIN_CHANNEL_WIDTH;
	config.height = MAIN_CHANNEL_HEIGHT;
	config.fps		 = MAIN_CHANNEL_FPS;
	config.bitrate		 = MAIN_CHANNEL_BIT_RATE;
	config.gop		 = MAIN_CHANNEL_GOP;
	config.vbr		 = MAIN_CHANNEL_VBR;
	config.qp_range.i_min_qp = MAIN_CHANNEL_I_MIN_QP;
	config.qp_range.i_max_qp = MAIN_CHANNEL_I_MAX_QP;
	config.qp_range.p_min_qp = MAIN_CHANNEL_P_MIN_QP;
	config.qp_range.p_max_qp = MAIN_CHANNEL_P_MAX_QP;
	ret = wdr_get_from_partition(&vi_part_cfg, rt_which_sensor(channel_id));
	if (!ret) {
		RT_LOGW("get info fram part: w&h = %d, %d, venc_format = %d, wdr = %d, flicker = %d",
			vi_part_cfg.width, vi_part_cfg.height, vi_part_cfg.venc_format,
			vi_part_cfg.wdr, vi_part_cfg.flicker_mode);

		if (vi_part_cfg.width && vi_part_cfg.height) {
			config.width  = vi_part_cfg.width;
			config.height = vi_part_cfg.height;
		}

		if (vi_part_cfg.venc_format == 2)
			config.encodeType = 2;
		else if (vi_part_cfg.venc_format == 1)
			config.encodeType = 0;

		//if (vi_part_cfg.flicker_mode == 1)
		//	config.fps = 16;
		config.fps = vi_part_cfg.fps;
		config.enable_wdr = vi_part_cfg.wdr;
	}
	config.profile	= AW_Video_H264ProfileMain;
	config.level	  = AW_Video_H264Level51;
	config.drop_frame_num = 0;
	config.output_mode    = MAIN_CHANNEL_OUT_MODE;
	config.pixelformat = MAIN_CHANNEL_PXL_FMT;

	config.venc_video_signal.video_format = RT_DEFAULT;
	config.venc_video_signal.full_range_flag = 1;
	config.venc_video_signal.src_colour_primaries = RT_VENC_BT709;
	config.venc_video_signal.dst_colour_primaries = RT_VENC_BT709;
	config.breduce_refrecmem = MAIN_CHANNEL_REDUCE_REFREC_MEM;
	config.enable_sharp = 1;//default enable sharp.
//!defined CONFIG_RT_MEDIA_DUAL_SENSOR
	// RT_LOGW("sigle sensor enter online mode");
	// config.bonline_channel = 1;
	// config.share_buf_num = 2;
	ret = ioctl_config(recoder, &config);
	if (ret != 0) {
		RT_LOGE("config err ret %d", ret);
		ioctl_destroy(recoder);
		return -1;
	}
	//rt_vin_sensor_fps_change_callback(config.channelId);

	return ret;
}

int rt_media_vin_is_ready(void)
{
	video_recoder *recoder_single  = &rt_media_devp->recoder[CSI_SENSOR_0_VIPP_0];

	if (recoder_single->bvin_is_ready || rt_media_devp->bcsi_status_set[CSI_SENSOR_0_VIPP_0])
		return 1;
	else
		return 0;
}

static int setup_recoder(void)
{
	int single_channel_id	= CSI_SENSOR_0_VIPP_0;
	video_recoder *recoder_single  = &rt_media_devp->recoder[single_channel_id];
#if defined CONFIG_RT_MEDIA_DUAL_SENSOR && RT_SECOND_SENSOR_KERNEL_START
	int dual_channel_id		= CSI_SENSOR_1_VIPP_0;
	video_recoder *recoder_dual  = &rt_media_devp->recoder[dual_channel_id];
#endif

	RT_LOGW("* start setup_recoder");

	setup_recoder_config(single_channel_id);

	if (recoder_single->bvin_is_ready || bvin_is_ready_rt_not_probe[single_channel_id]) {
		rt_media_devp->bcsi_status_set[single_channel_id] = 1;
		ioctl_start(recoder_single);
	} else {
		recoder_single->bsetup_recorder_finish = 1;
	}

#if defined CONFIG_RT_MEDIA_DUAL_SENSOR && RT_SECOND_SENSOR_KERNEL_START
	setup_recoder_config(dual_channel_id);
	if (recoder_dual->bvin_is_ready || bvin_is_ready_rt_not_probe[dual_channel_id]) {
		rt_media_devp->bcsi_status_set[dual_channel_id] = 1;
		ioctl_start(recoder_dual);
	} else {
		recoder_dual->bsetup_recorder_finish = 1;
	}
#endif

	RT_LOGW("finish");
	return 0;
}

static int thread_setup(void *param)
{
	setup_recoder();
	RT_LOGW("finish");
	return 0;
}
#endif

#if defined CONFIG_VIN_INIT_MELIS
static int rt_media_csi_err(void *dev, void *data, int len)
{
	RT_LOGE("melis rpmsg notify csi is err");
	rt_media_devp->bcsi_err = 1;
	return 0;
}
#endif

static int rt_media_probe(struct platform_device *pdev)
{
	int i   = 0;
	int ret = 0;
	int devno;
#if defined CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL && defined CONFIG_VIDEO_RT_MEDIA
	struct sched_param param = {.sched_priority = 1 };
#endif

#if defined(CONFIG_OF)
	struct device_node *node;
#endif
	dev_t dev;

	dev = 0;

	RT_LOGW("rt_media install start!!!\n");

#if defined(CONFIG_OF)
	node = pdev->dev.of_node;
#endif
	/*register or alloc the device number.*/
	if (g_rt_media_dev_major) {
		dev = MKDEV(g_rt_media_dev_major, g_rt_media_dev_minor);
		ret = register_chrdev_region(dev, 1, "rt-media");
	} else {
		ret		     = alloc_chrdev_region(&dev, g_rt_media_dev_minor, 1, "rt-media");
		g_rt_media_dev_major = MAJOR(dev);
		g_rt_media_dev_minor = MINOR(dev);
	}

	if (ret < 0) {
		RT_LOGW("rt_media_dev: can't get major %d\n", g_rt_media_dev_major);
		return ret;
	}

	rt_media_devp = kmalloc(sizeof(struct rt_media_dev), GFP_KERNEL);
	if (rt_media_devp == NULL) {
		RT_LOGW("malloc mem for cedar device err\n");
		return -ENOMEM;
	}
	memset(rt_media_devp, 0, sizeof(struct rt_media_dev));
	rt_media_devp->platform_dev = &pdev->dev;

	for (i = 0; i < VIDEO_INPUT_CHANNEL_NUM; i++) {
		rt_media_devp->recoder[i].state = RT_MEDIA_STATE_IDLE;
#if !defined CONFIG_VIN_INIT_MELIS
//if e907 not enable, default vin is ready.
		rt_media_devp->bcsi_status_set[i] = 1;
		rt_media_devp->recoder[i].bvin_is_ready = 1;
#endif
	}

#if defined CONFIG_VIN_INIT_MELIS
	rpmsg_notify_add("e907_rproc@0", "rt-media", rt_media_csi_err, rt_media_devp);
#endif

	/* Create char device */
	devno = MKDEV(g_rt_media_dev_major, g_rt_media_dev_minor);
	cdev_init(&rt_media_devp->cdev, &rt_media_fops);
	rt_media_devp->cdev.owner = THIS_MODULE;
	/* rt_media_devp->cdev.ops = &rt_media_fops; */
	ret = cdev_add(&rt_media_devp->cdev, devno, 1);
	if (ret)
		RT_LOGW("Err:%d add cedardev", ret);
	rt_media_devp->class = class_create(THIS_MODULE, "rt-media");
	rt_media_devp->dev   = device_create(rt_media_devp->class, NULL, devno, NULL, "rt-media");

	RT_LOGW("rt_media install end!!!\n");
	RT_LOGD("rt_media_devp %d vi%px ven%px", rt_media_devp->recoder[0].activate_vi_flag, rt_media_devp->recoder[0].vi_comp, rt_media_devp->recoder[0].venc_comp);
#if defined CONFIG_RT_MEDIA_SETUP_RECORDER_IN_KERNEL && defined CONFIG_VIDEO_RT_MEDIA
	if (!rt_media_devp->bcsi_err) {
		rt_media_devp->setup_thread = kthread_create(thread_setup, rt_media_devp, "rt_media thread");
		sched_setscheduler(rt_media_devp->setup_thread, SCHED_FIFO, &param);
		wake_up_process(rt_media_devp->setup_thread);
	}
#endif

	return 0;
}
static int rt_media_remove(struct platform_device *pdev)
{
	dev_t dev;

	dev = MKDEV(g_rt_media_dev_major, g_rt_media_dev_minor);

	/* Destroy char device */
	if (rt_media_devp) {
		if (rt_media_devp->setup_thread)
			kthread_stop(rt_media_devp->setup_thread);

		if (rt_media_devp->reset_high_fps_thread)
			kthread_stop(rt_media_devp->reset_high_fps_thread);

		cdev_del(&rt_media_devp->cdev);
		device_destroy(rt_media_devp->class, dev);
		class_destroy(rt_media_devp->class);
	}

	unregister_chrdev_region(dev, 1);

	kfree(rt_media_devp);
	return 0;
}

static struct platform_driver rt_media_driver = {
	.probe  = rt_media_probe,
	.remove = rt_media_remove,
#if defined(CONFIG_PM)
	.suspend = rt_media_suspend,
	.resume  = rt_media_resume,
#endif
	.driver = {
		.name  = "rt-media",
		.owner = THIS_MODULE,

#if defined(CONFIG_OF)
		.of_match_table = rt_media_match,
#endif
	},
};

static int __init rt_media_module_init(void)
{
	/*need not to gegister device here,because the device is registered by device tree */
	/*platform_device_register(&sunxi_device_cedar);*/
	int ret = 0;

	RT_LOGD("rt_media version 0.1\n");
	ret =  platform_driver_register(&rt_media_driver);
	RT_LOGD("rt_media ret %d\n", ret);
	return ret;
}

static void __exit rt_media_module_exit(void)
{
	platform_driver_unregister(&rt_media_driver);
}

fs_initcall(rt_media_module_init);
//late_initcall(rt_media_module_init);

module_exit(rt_media_module_exit);

MODULE_AUTHOR("Soft-wxw");
MODULE_DESCRIPTION("User rt-media interface");
MODULE_LICENSE("GPL");
MODULE_VERSION("rt-v1.0");
MODULE_ALIAS("platform:rt-media");

//* todo

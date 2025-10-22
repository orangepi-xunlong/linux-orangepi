/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/module.h>
//#include <linux/g2d_driver.h>
#include <linux/fs.h>
#include "vin_video_api.h"
#define LOG_TAG "rt_venc_comp"
#include "../ven_adapter/vencoder.h"
#include "rt_common.h"
#include "rt_venc_component.h"
#include "rt_message.h"
#include <uapi/rt-media/uapi_rt_media.h>

#define TEST_ENCODER_BY_SELF (0)
#define DEBUG_VBV_CACHE_TIME           (4)  // unit:seconds, exp:1,2,3,4...
#define ENABLE_SAVE_NATIVE_OVERLAY_DATA (0)

DEFINE_MUTEX(venc_mutex);
/*
extern int g2d_blit_h_new(g2d_blt_h *para);
extern int g2d_open(struct inode *inode, struct file *file);
extern int g2d_release(struct inode *inode, struct file *file);
extern void g2d_ioctl_mutex_lock(void);
extern void g2d_ioctl_mutex_unlock(void);
*/
typedef struct catch_jpeg_cxt {
	int enable;
	int encoder_finish_flag;
	VideoEncoder *vencoder;
	int width;
	int height;
	int qp;
	wait_queue_head_t wait_enc_finish;
	unsigned int wait_enc_finish_condition;
} catch_jpeg_cxt;

typedef struct venc_comp_ctx {
	int vencoder_init_flag;
	VideoEncoder *vencoder;
	comp_state_type state;
	struct task_struct *venc_thread;
	message_queue_t msg_queue;
	comp_callback_type *callback;
	void *callback_data;
	rt_component_type *self_comp;
	wait_queue_head_t wait_reply[WAIT_REPLY_NUM];
	unsigned int wait_reply_condition[WAIT_REPLY_NUM];
	venc_inbuf_manager in_buf_manager;
	venc_outbuf_manager out_buf_manager;
	comp_tunnel_info in_port_tunnel_info;
	comp_tunnel_info out_port_tunnel_info;
	venc_comp_base_config base_config;
	venc_comp_normal_config normal_config;
	int actual_en_encpp_sharp; ///< indicate if actually enable encpp sharp on venc.
	unsigned char *vbvStartAddr;
	catch_jpeg_cxt jpeg_cxt;
	int is_first_frame;
	VencCbType vencCallBack;
} venc_comp_ctx;

#define GLOBAL_OSD_MAX_NUM (3)

typedef struct native_osd_info {
	VencOverlayInfoS sOverlayInfo;

	unsigned int ori_frame_w;
	unsigned int ori_frame_h;
	struct mem_interface *memops;

	/* the g2d_in_buf will not release at all, so it can setup to vencoder immediately when start */
	unsigned char *g2d_in_ion_vir_addr[GLOBAL_OSD_MAX_NUM];
	unsigned int g2d_in_buf_size[GLOBAL_OSD_MAX_NUM];
} native_osd_info;

//static native_osd_info *global_osd_info;

static int thread_process_venc(void *param);
int adjust_native_overlay_info(venc_comp_ctx *venc_comp);

static int config_VencOutputBuffer_by_videostream(venc_comp_ctx *venc_comp,
						  VencOutputBuffer *output_buffer,
						  video_stream_s *video_stream)
{
	if (venc_comp->vbvStartAddr == NULL) {
		RT_LOGE("venc_comp->vbvStartAddr is null");
		return -1;
	}

	output_buffer->nID    = video_stream->id;
	output_buffer->nPts   = video_stream->pts;
	output_buffer->nFlag  = video_stream->flag;
	output_buffer->nSize0 = video_stream->size0;
	output_buffer->nSize1 = video_stream->size1;
	output_buffer->nSize2 = video_stream->size2;
	output_buffer->pData0 = venc_comp->vbvStartAddr + video_stream->offset0;
	output_buffer->pData1 = venc_comp->vbvStartAddr + video_stream->offset1;
	output_buffer->pData2 = venc_comp->vbvStartAddr + video_stream->offset2;

	output_buffer->pData0 = video_stream->data0;
	output_buffer->pData1 = video_stream->data1;
	output_buffer->pData2 = video_stream->data2;
	RT_LOGD("request stream: pData0 =%px, %px, id = %d",
		video_stream->data0, output_buffer->pData0, output_buffer->nID);

	return 0;
}

static int config_videostream_by_VencOutputBuffer(venc_comp_ctx *venc_comp,
						  VencOutputBuffer *output_buffer,
						  video_stream_s *video_stream)
{
	if (venc_comp->vbvStartAddr == NULL) {
		RT_LOGE("venc_comp->vbvStartAddr is null");
		return -1;
	}

	video_stream->id      = output_buffer->nID;
	video_stream->pts     = output_buffer->nPts;
	video_stream->flag    = output_buffer->nFlag;
	video_stream->size0   = output_buffer->nSize0;
	video_stream->size1   = output_buffer->nSize1;
	video_stream->size2   = output_buffer->nSize2;
	video_stream->offset0 = output_buffer->pData0 - venc_comp->vbvStartAddr;

	if (output_buffer->pData1)
		video_stream->offset1 = output_buffer->pData1 - venc_comp->vbvStartAddr;
	if (output_buffer->pData2)
		video_stream->offset2 = output_buffer->pData2 - venc_comp->vbvStartAddr;

	video_stream->data0 = output_buffer->pData0;
	video_stream->data1 = output_buffer->pData1;
	video_stream->data2 = output_buffer->pData2;

	RT_LOGD("reqeust stream: pData0 =%px, %px, id = %d",
		video_stream->data0, output_buffer->pData0,
		output_buffer->nID);

	if (video_stream->flag & VENC_BUFFERFLAG_KEYFRAME)
		video_stream->keyframe_flag = 1;
	else
		video_stream->keyframe_flag = 0;

	return 0;
}

static int venc_comp_event_handler(VideoEncoder *pEncoder, void *pAppData, VencEventType eEvent,
		unsigned int nData1, unsigned int nData2, void *pEventData)
{
	venc_comp_ctx *venc_comp = (venc_comp_ctx *)pAppData;

	if (VencEvent_UpdateIspToVeParam == eEvent) {
		int vipp_id = venc_comp->base_config.channel_id;
		int en_encpp = 0;
		int final_en_encpp_sharp = 0;
		sEncppSharpParam mSharpParam;
		sEncppSharpParamDynamic dynamic_sharp;
		sEncppSharpParamStatic static_sharp;

		vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_EN, &en_encpp);
		RT_LOGI("vencoder:%px, codec_type:%d, en_encpp %d %d", venc_comp->vencoder, venc_comp->base_config.codec_type, en_encpp, venc_comp->base_config.en_encpp_sharp);
		final_en_encpp_sharp = en_encpp && venc_comp->base_config.en_encpp_sharp;
		if (venc_comp->actual_en_encpp_sharp != final_en_encpp_sharp) {
			RT_LOGW("Be careful! vencoder:%px, codec_type:%d, en_encpp change:%d->%d(%d,%d)",
				venc_comp->vencoder, venc_comp->base_config.codec_type,
				venc_comp->actual_en_encpp_sharp, final_en_encpp_sharp, en_encpp, venc_comp->base_config.en_encpp_sharp);
			venc_comp->actual_en_encpp_sharp = final_en_encpp_sharp;
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableEncppSharp, (void *)&venc_comp->actual_en_encpp_sharp);
		}
		if (venc_comp->actual_en_encpp_sharp) {
			vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_DYNAMIC_CFG, &dynamic_sharp);
			vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_STATIC_CFG, &static_sharp);

			RT_LOGI("hfr_hf_wht_clp %d max_clp_ratio %d", dynamic_sharp.hfr_hf_wht_clp, dynamic_sharp.max_clp_ratio);
			RT_LOGI("ls_dir_ratio %d ss_dir_ratio %d", static_sharp.ls_dir_ratio, static_sharp.ss_dir_ratio);
			mSharpParam.pDynamicParam = &dynamic_sharp;
			mSharpParam.pStaticParam = &static_sharp;
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamSharpConfig, &mSharpParam);
		}
	} else if (VencEvent_UpdateVeToIspParam == eEvent) {
		VencVe2IspParam *s_ve2isp_param = (VencVe2IspParam *)pEventData;
		RTIspCfgAttrData rt_isp_cfg;

		RT_LOGI("d2d_level %d %d", s_ve2isp_param->d2d_level, s_ve2isp_param->d3d_level);
		memset(&rt_isp_cfg, 0, sizeof(RTIspCfgAttrData));
		//set 2d param
		rt_isp_cfg.cfg_id = RT_ISP_CTRL_DN_STR;
		rt_isp_cfg.denoise_level = s_ve2isp_param->d2d_level;
		vin_set_isp_attr_cfg_special(venc_comp->base_config.channel_id, &rt_isp_cfg);

		//set 3d param
		rt_isp_cfg.cfg_id = RT_ISP_CTRL_3DN_STR;
		rt_isp_cfg.tdf_level = s_ve2isp_param->d3d_level;
		vin_set_isp_attr_cfg_special(venc_comp->base_config.channel_id, &rt_isp_cfg);
	}
	return 0;
}
static int setup_vbv_buffer_size(venc_comp_ctx *venc_comp)
{
	unsigned int vbv_size = venc_comp->base_config.vbv_buf_size;
	int min_size;
	unsigned int thresh_size = venc_comp->base_config.vbv_thresh_size;
	unsigned int bit_rate = venc_comp->base_config.bit_rate;

	min_size    = venc_comp->base_config.dst_width * venc_comp->base_config.dst_height * 3 / 2;
	RT_LOGD("vbv_size %d %d", vbv_size, thresh_size);
	if (!vbv_size || !thresh_size) {
		if (bit_rate) {
			thresh_size = bit_rate / 8 / venc_comp->base_config.frame_rate * 15;
		} else {
			thresh_size = venc_comp->base_config.dst_width * venc_comp->base_config.dst_height;
		}
		if (thresh_size > 7 * 1024 * 1024) {
			RT_LOGW("Be careful! threshSize[%d]bytes too large, reduce to 7MB", thresh_size);
			thresh_size = 7 * 1024 * 1024;
		}

		if (bit_rate > 0) {
			vbv_size    = bit_rate / 8 * DEBUG_VBV_CACHE_TIME + thresh_size;
		} else {
			RT_LOGW("the bitrate error: %d", bit_rate);
			vbv_size = min_size;
		}
	}
	vbv_size = RT_ALIGN(vbv_size, 1024);

	if (vbv_size <= thresh_size) {
		RT_LOGE("fatal error! vbv_size[%d] <= thresh_size[%d]", vbv_size, thresh_size);
	}

	if (vbv_size > 12 * 1024 * 1024) {
		RT_LOGE("Be careful! vbv_size[%d] too large, exceed 24M byte", vbv_size);
		vbv_size = 12 * 1024 * 1024;
	}

	RT_LOGD("bit rate is %d bytes, set encode vbv size %d, frame length threshold %d, minSize = %d",
		bit_rate, vbv_size, thresh_size, min_size);

	if (venc_comp->base_config.dst_width >= 3840) {
		vbv_size    = 2 * 1024 * 1024;
		thresh_size = 1 * 1024 * 1024;
		RT_LOGW("the size is too large[%d x %d], so we reset vbv size and thresh_size to: %d, %d",
			venc_comp->base_config.dst_width, venc_comp->base_config.dst_height,
			vbv_size, thresh_size);
	}

	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetVbvSize, &vbv_size);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetFrameLenThreshold, &thresh_size);
	UNLOCK_MUTEX(&venc_mutex);

	return 0;
}

static int vencoder_create(venc_comp_ctx *venc_comp)
{
	VENC_CODEC_TYPE type = VENC_CODEC_H264;

	/* map codec type */
	switch (venc_comp->base_config.codec_type) {
	case RT_VENC_CODEC_H264: {
		type = VENC_CODEC_H264;
		break;
	}
	case RT_VENC_CODEC_H265: {
		type = VENC_CODEC_H265;
		break;
	}
	case RT_VENC_CODEC_JPEG: {
		type = VENC_CODEC_JPEG;
		break;
	}
	default: {
		RT_LOGE("nor support codec type: %d", venc_comp->base_config.codec_type);
		return ERROR_TYPE_ILLEGAL_PARAM;
		break;
	}
	}
	LOCK_MUTEX(&venc_mutex);
	venc_comp->vencoder = VencCreate(type);
	UNLOCK_MUTEX(&venc_mutex);
	return -1;
}

static int vencoder_init(venc_comp_ctx *venc_comp)
{
	VencBaseConfig base_config;
	int en_encpp = 0;
	int vipp_id = venc_comp->base_config.channel_id;

	memset(&base_config, 0, sizeof(VencBaseConfig));

	base_config.bOnlineMode		   = 1; //venc_comp->base_config.bOnlineMode;
	base_config.bOnlineChannel	 = venc_comp->base_config.bOnlineChannel;
	base_config.nOnlineShareBufNum = venc_comp->base_config.share_buf_num;
	base_config.bEncH264Nalu	   = 0;
	base_config.nInputWidth		   = venc_comp->base_config.src_width;
	base_config.nInputHeight	   = venc_comp->base_config.src_height;
	base_config.nDstWidth		   = venc_comp->base_config.dst_width;
	base_config.nDstHeight		   = venc_comp->base_config.dst_height;
	base_config.nStride		   = RT_ALIGN(base_config.nInputWidth, 16);
	base_config.eInputFormat	   = venc_comp->base_config.pixelformat;
	base_config.bOnlyWbFlag		   = 0;
	base_config.memops		   = NULL;
	base_config.veOpsS		   = NULL;
	base_config.bLbcLossyComEnFlag2x   = 0;
	base_config.bLbcLossyComEnFlag2_5x = 0;
	base_config.bIsVbvNoCache = 1;
	base_config.channel_id = venc_comp->base_config.channel_id;

	if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_25X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2_5x = 1;
	} else if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2x = 1;
	}

	RT_LOGW("channel_id %d bOnlineChannel %d input format = %d, 2_5x_flag = %d", venc_comp->base_config.channel_id, base_config.bOnlineChannel, base_config.eInputFormat,
		base_config.bLbcLossyComEnFlag2_5x);

	vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_EN, &en_encpp);
	RT_LOGW("vencoder:%px, codec_type:%d, en_encpp %d %d, ipc_case:%d", venc_comp->vencoder, venc_comp->base_config.codec_type, en_encpp, venc_comp->base_config.en_encpp_sharp, venc_comp->base_config.is_ipc_case);
	venc_comp->actual_en_encpp_sharp = en_encpp && venc_comp->base_config.en_encpp_sharp;
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableEncppSharp, (void *)&venc_comp->actual_en_encpp_sharp);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamProductCase, (void *)&venc_comp->base_config.is_ipc_case);
	/*int64_t time_start = get_cur_time();*/
	LOCK_MUTEX(&venc_mutex);
	VencInit(venc_comp->vencoder, &base_config);
	UNLOCK_MUTEX(&venc_mutex);

	if (venc_comp->base_config.s_wbyuv_param.bEnableWbYuv) {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableWbYuv, &venc_comp->base_config.s_wbyuv_param);
		UNLOCK_MUTEX(&venc_mutex);
	}

	/*
	int64_t time_end = get_cur_time();
	RT_LOGD("time of VideoEncInit: %lld",(time_end - time_start));
	*/

	RT_LOGD("VencInit finish");

#if TEST_ENCODER_BY_SELF
	VencAllocateBufferParam bufferParam;
	memset(&bufferParam, 0, sizeof(VencAllocateBufferParam));
	bufferParam.nSizeY     = base_config.nInputWidth * base_config.nInputHeight;
	bufferParam.nSizeC     = base_config.nInputWidth * base_config.nInputHeight / 2;
	bufferParam.nBufferNum = 1;
	AllocInputBuffer(venc_comp->vencoder, &bufferParam);
#endif

	return 0;
}

static VENC_H264PROFILETYPE match_h264_profile(int src_profile)
{
	VENC_H264PROFILETYPE h264_profile = VENC_H264ProfileHigh;

	switch (src_profile) {
	case AW_Video_H264ProfileBaseline:
	case AW_Video_H264ProfileMain:
	case AW_Video_H264ProfileHigh:
		h264_profile = src_profile;
		break;
	default: {
		RT_LOGW("can not match the h264 profile: %d, use defaut: %d", src_profile, h264_profile);
		break;
	}
	}

	return h264_profile;
}

static VENC_H264LEVELTYPE match_h264_level(int src_level)
{
	VENC_H264LEVELTYPE h264_level = VENC_H264Level51;

	switch (src_level) {
	case AW_Video_H264Level1:
	case AW_Video_H264Level11:
	case AW_Video_H264Level12:
	case AW_Video_H264Level13:
	case AW_Video_H264Level2:
	case AW_Video_H264Level21:
	case AW_Video_H264Level22:
	case AW_Video_H264Level3:
	case AW_Video_H264Level31:
	case AW_Video_H264Level32:
	case AW_Video_H264Level4:
	case AW_Video_H264Level41:
	case AW_Video_H264Level42:
	case AW_Video_H264Level5:
	case AW_Video_H264Level51:
		h264_level = src_level;
		break;
	default: {
		RT_LOGW("can not match the h264 level: %d, defaut: %d", src_level, h264_level);
		break;
	}
	}

	return h264_level;
}

static VENC_H265PROFILETYPE match_h265_profile(int src_profile)
{
	VENC_H265PROFILETYPE h265_profile = VENC_H265ProfileMain;

	switch (src_profile) {
	case AW_Video_H265ProfileMain:
	case AW_Video_H265ProfileMain10:
	case AW_Video_H265ProfileMainStill:
		h265_profile = src_profile;
		break;
	default: {
		RT_LOGW("can not match the h265 profile: %d, defaut: %d", src_profile, h265_profile);
		break;
	}
	}

	return h265_profile;
}

static VENC_H265LEVELTYPE match_h265_level(int src_level)
{
	VENC_H265LEVELTYPE h265_level = VENC_H265Level51;

	switch (src_level) {
	case AW_Video_H265Level1:
	case AW_Video_H265Level2:
	case AW_Video_H265Level21:
	case AW_Video_H265Level3:
	case AW_Video_H265Level31:
	case AW_Video_H265Level41:
	case AW_Video_H265Level5:
	case AW_Video_H265Level51:
	case AW_Video_H265Level52:
	case AW_Video_H265Level6:
	case AW_Video_H265Level61:
	case AW_Video_H265Level62:
		h265_level = src_level;
		break;
	default: {
		RT_LOGW("can not match the h265 level: %d, defaut: %d", src_level, h265_level);
		break;
	}
	}

	return h265_level;
}

static int set_param_h264(venc_comp_ctx *venc_comp)
{
	VencH264VideoTiming video_time;
	VencH264Param param_h264;

	memset(&param_h264, 0, sizeof(VencH264Param));

	param_h264.sProfileLevel.nProfile  = match_h264_profile(venc_comp->base_config.profile);
	param_h264.sProfileLevel.nLevel    = match_h264_level(venc_comp->base_config.level);
	param_h264.bEntropyCodingCABAC     = 1;
	param_h264.sQPRange.nMinqp	 = venc_comp->base_config.qp_range.i_min_qp;
	param_h264.sQPRange.nMaxqp	 = venc_comp->base_config.qp_range.i_max_qp;
	param_h264.sQPRange.nMinPqp	= venc_comp->base_config.qp_range.p_min_qp;
	param_h264.sQPRange.nMaxPqp	= venc_comp->base_config.qp_range.p_max_qp;
	param_h264.sQPRange.nQpInit	= venc_comp->base_config.qp_range.i_init_qp;
	param_h264.nFramerate		   = venc_comp->base_config.frame_rate;
	param_h264.nSrcFramerate	   = venc_comp->base_config.frame_rate;
	param_h264.nBitrate		   = venc_comp->base_config.bit_rate;
	param_h264.nMaxKeyInterval	 = venc_comp->base_config.max_keyframe_interval;
	param_h264.nCodingMode		   = VENC_FRAME_CODING;
	param_h264.sGopParam.bUseGopCtrlEn = 1;
	param_h264.sGopParam.eGopMode      = AW_NORMALP;
	param_h264.breduce_refrecmem = venc_comp->base_config.breduce_refrecmem;

	if (venc_comp->base_config.rc_mode == VENC_COMP_RC_MODE_H264CBR)
		param_h264.sRcParam.eRcMode = AW_CBR;
	else if (venc_comp->base_config.rc_mode == VENC_COMP_RC_MODE_H264VBR) {
		param_h264.sRcParam.eRcMode		  = AW_VBR;
		param_h264.sRcParam.sVbrParam.uMaxBitRate = venc_comp->base_config.vbr_param.max_bit_rate;
		param_h264.sRcParam.sVbrParam.nMovingTh   = venc_comp->base_config.vbr_param.moving_th;
		param_h264.sRcParam.sVbrParam.nQuality    = venc_comp->base_config.vbr_param.quality;
	} else {
		RT_LOGW("not support the rc_mode: %d", venc_comp->base_config.rc_mode);
	}

	RT_LOGI("h264 param: profile = %d, level = %d, min_qp = %d, max_qp = %d",
		param_h264.sProfileLevel.nProfile,
		param_h264.sProfileLevel.nLevel,
		param_h264.sQPRange.nMinqp,
		param_h264.sQPRange.nMaxqp);

	memset(&video_time, 0, sizeof(VencH264VideoTiming));

	video_time.num_units_in_tick     = 1;
	video_time.time_scale		 = 32;
	video_time.fixed_frame_rate_flag = 1;

	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264Param, &param_h264);
	//VideoEncSetParameter(venc_comp->vencoder, VENC_IndexParamH264VideoTiming, &video_time);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264VideoSignal, &venc_comp->base_config.venc_video_signal);
	UNLOCK_MUTEX(&venc_mutex);

	return 0;
}

static int set_param_h265(venc_comp_ctx *venc_comp)
{
	VencH265Param h265Param;

	memset(&h265Param, 0, sizeof(VencH265Param));

	h265Param.sGopParam.bUseGopCtrlEn = 1;
	h265Param.sGopParam.eGopMode      = AW_NORMALP;
	h265Param.nBitrate		  = venc_comp->base_config.bit_rate;
	h265Param.nFramerate		  = venc_comp->base_config.frame_rate;
	h265Param.nQPInit		  = 35;
	h265Param.idr_period		  = venc_comp->base_config.max_keyframe_interval;
	h265Param.nGopSize		  = h265Param.idr_period;
	h265Param.nIntraPeriod		  = h265Param.idr_period;
	h265Param.sProfileLevel.nProfile  = match_h265_profile(venc_comp->base_config.profile);
	h265Param.sProfileLevel.nLevel    = match_h265_level(venc_comp->base_config.profile);

	h265Param.sQPRange.nMinqp  = venc_comp->base_config.qp_range.i_min_qp;
	h265Param.sQPRange.nMaxqp  = venc_comp->base_config.qp_range.i_max_qp;
	h265Param.sQPRange.nMinPqp = venc_comp->base_config.qp_range.p_min_qp;
	h265Param.sQPRange.nMaxPqp = venc_comp->base_config.qp_range.p_max_qp;
	h265Param.sQPRange.nQpInit = venc_comp->base_config.qp_range.i_init_qp;
	h265Param.breduce_refrecmem = venc_comp->base_config.breduce_refrecmem;

	if (venc_comp->base_config.rc_mode == VENC_COMP_RC_MODE_H264CBR)
		h265Param.sRcParam.eRcMode = AW_CBR;
	else if (venc_comp->base_config.rc_mode == VENC_COMP_RC_MODE_H264VBR) {
		h265Param.sRcParam.eRcMode		 = AW_VBR;
		h265Param.sRcParam.sVbrParam.uMaxBitRate = venc_comp->base_config.vbr_param.max_bit_rate;
		h265Param.sRcParam.sVbrParam.nMovingTh   = venc_comp->base_config.vbr_param.moving_th;
		h265Param.sRcParam.sVbrParam.nQuality    = venc_comp->base_config.vbr_param.quality;
	} else {
		RT_LOGW("not support the rc_mode: %d", venc_comp->base_config.rc_mode);
	}

	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamH265Param, &h265Param);

	VencSetParameter(venc_comp->vencoder, VENC_IndexParamVUIVideoSignal, &venc_comp->base_config.venc_video_signal);
	UNLOCK_MUTEX(&venc_mutex);
	return 0;
}

static int set_param_jpeg(venc_comp_ctx *venc_comp)
{
	VencJpegVideoSignal sVideoSignal;
	int rotate_angle = venc_comp->base_config.rotate_angle;
	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamJpegEncMode, &venc_comp->base_config.jpg_mode);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamJpegQuality,
			 &venc_comp->base_config.quality);
	if (venc_comp->base_config.jpg_mode) {//mjpg
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamBitrate, &venc_comp->base_config.bit_rate);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamFramerate, &venc_comp->base_config.frame_rate);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetBitRateRange, &venc_comp->base_config.bit_rate_range);
	}

	memset(&sVideoSignal, 0, sizeof(VencJpegVideoSignal));
	sVideoSignal.src_colour_primaries = venc_comp->base_config.venc_video_signal.src_colour_primaries;
	sVideoSignal.dst_colour_primaries = venc_comp->base_config.venc_video_signal.dst_colour_primaries;
	RT_LOGD("src_colour_primaries %d %d", sVideoSignal.src_colour_primaries, sVideoSignal.dst_colour_primaries);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamJpegVideoSignal, &sVideoSignal);
	if (rotate_angle == 90 || rotate_angle == 180 || rotate_angle == 270) {
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamRotation, &rotate_angle);
	}
	RT_LOGI("jpg param %d %d %d %d %d %d", venc_comp->base_config.jpg_mode, venc_comp->base_config.quality, venc_comp->base_config.frame_rate,
	venc_comp->base_config.bit_rate, venc_comp->base_config.bit_rate_range.bitRateMin,
	venc_comp->base_config.bit_rate_range.bitRateMax);
	UNLOCK_MUTEX(&venc_mutex);
	return 0;
}

static void check_qp_value(video_qp_range *qp_range)
{
	int min_qp = qp_range->i_min_qp;
	int max_qp = qp_range->i_max_qp;

	if (!(min_qp >= 1 && min_qp <= 51)) {
		RT_LOGW("i_min_qp should in range:[1,51]! but usr_SetVal: %d! change it to default: 1", min_qp);
		min_qp = 1;
	}
	if (!(max_qp >= min_qp && max_qp >= 1 && max_qp <= 51)) {
		RT_LOGW("i_max_qp should in range:[min_qp,51]! but usr_SetVal: %d! change it to default: 51", max_qp);
		max_qp = 51;
	}

	qp_range->i_min_qp = min_qp;
	qp_range->i_max_qp = max_qp;

	min_qp = qp_range->p_min_qp;
	max_qp = qp_range->p_max_qp;
	if (!(min_qp >= 1 && min_qp <= 51)) {
		RT_LOGW("p_min_qp should in range:[1,51]! but usr_SetVal: %d! change it to default: 1", min_qp);
		min_qp = 1;
	}
	if (!(max_qp >= min_qp && max_qp >= 1 && max_qp <= 51)) {
		RT_LOGW("p_max_qp should in range:[min_qp,51]! but usr_SetVal: %d! change it to default: 51", max_qp);
		max_qp = 51;
	}

	qp_range->p_min_qp = min_qp;
	qp_range->p_max_qp = max_qp;

	if (qp_range->i_init_qp <= 0 || qp_range->i_init_qp > 51)
		qp_range->i_init_qp = 35;
}

static void init_2d_3d_param(s2DfilterParam *p2DfilterParam, s3DfilterParam *p3DfilterParam)
{
    //init 2dfilter(2dnr) is open
    p2DfilterParam->enable_2d_filter = 1;
    p2DfilterParam->filter_strength_y = 127;
    p2DfilterParam->filter_strength_uv = 127;
    p2DfilterParam->filter_th_y = 11;
    p2DfilterParam->filter_th_uv = 7;

    //init 3dfilter(3dnr) is open
    p3DfilterParam->enable_3d_filter = 1;
    p3DfilterParam->adjust_pix_level_enable = 0;
    p3DfilterParam->smooth_filter_enable = 1;
    p3DfilterParam->max_pix_diff_th = 6;
    p3DfilterParam->max_mv_th = 8;
    p3DfilterParam->max_mad_th = 14;
    p3DfilterParam->min_coef = 13;
    p3DfilterParam->max_coef = 16;
	return ;
}

static int vencoder_set_param(venc_comp_ctx *venc_comp)
{
	s3DfilterParam m3DfilterParam;
	s2DfilterParam m2DfilterParam;
#if defined(CONFIG_DEBUG_FS)
    VeProcSet sVeProcInfo;
	int channel_id = 0;
#endif
	// file g2d_file;
	check_qp_value(&venc_comp->base_config.qp_range);

	RT_LOGI("i_qp = %d~%d, p_qp = %d~%d",
		venc_comp->base_config.qp_range.i_min_qp, venc_comp->base_config.qp_range.i_max_qp,
		venc_comp->base_config.qp_range.p_min_qp, venc_comp->base_config.qp_range.p_max_qp);

	if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H264)
		set_param_h264(venc_comp);
	else if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H265)
		set_param_h265(venc_comp);
	else if (venc_comp->base_config.codec_type == RT_VENC_CODEC_JPEG)
		set_param_jpeg(venc_comp);

#if defined(CONFIG_DEBUG_FS)
	memset(&sVeProcInfo, 0, sizeof(VeProcSet));
    sVeProcInfo.bProcEnable = 1;
    sVeProcInfo.nProcFreq = 10;
	channel_id = venc_comp->base_config.channel_id;
	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamChannelNum, &channel_id);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamProcSet, &sVeProcInfo);
	UNLOCK_MUTEX(&venc_mutex);
#endif

	//init 2d/3d param
	init_2d_3d_param(&m2DfilterParam, &m3DfilterParam);
	if (venc_comp->base_config.codec_type != RT_VENC_CODEC_JPEG) {
		VencSetParameter(venc_comp->vencoder, VENC_IndexParam3DFilterNew, &m3DfilterParam);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParam2DFilter, &m2DfilterParam);
	}

	setup_vbv_buffer_size(venc_comp);

	if (venc_comp->vencoder_init_flag == 0) {
		vencoder_init(venc_comp);
		venc_comp->vencoder_init_flag = 1;
	}

#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
	RT_LOGW("enable_overlay = %d", venc_comp->base_config.enable_overlay);
	if (venc_comp->base_config.enable_overlay == 1) {
		if (adjust_native_overlay_info(venc_comp) == 0) {
			LOCK_MUTEX(&venc_mutex);
			g2d_open(0, &g2d_file);
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetOverlay, &global_osd_info->sOverlayInfo);
			g2d_release(0, &g2d_file);
			UNLOCK_MUTEX(&venc_mutex);
		}
	}
#endif

	return 0;
}

static int commond_process(struct venc_comp_ctx *venc_comp, message_t *msg)
{
	int cmd_error		      = 0;
	int cmd			      = msg->command;
	wait_queue_head_t *wait_reply = (wait_queue_head_t *)msg->wait_queue;
	unsigned int *wait_condition  = (unsigned int *)msg->wait_condition;

	RT_LOGI("cmd process: cmd = %d, state = %d, wait_reply = %px, wait_condition = %px",
		cmd, venc_comp->state, wait_reply, wait_condition);

	if (cmd == COMP_COMMAND_INIT) {
		if (venc_comp->state != COMP_STATE_IDLE) {
			cmd_error = 1;
		} else {
			vencoder_create(venc_comp);
			venc_comp->state = COMP_STATE_INITIALIZED;
		}
	} else if (cmd == COMP_COMMAND_START) {
		if (venc_comp->state != COMP_STATE_INITIALIZED && venc_comp->state != COMP_STATE_PAUSE) {
			cmd_error = 1;
		} else {
			venc_comp->state = COMP_STATE_EXECUTING;
		}
	} else if (cmd == COMP_COMMAND_PAUSE) {
		venc_comp->state = COMP_STATE_PAUSE;
	} else if (cmd == COMP_COMMAND_STOP) {
		venc_comp->state = COMP_STATE_IDLE;
	}

	if (cmd_error == 1) {
		venc_comp->callback->EventHandler(
			venc_comp->self_comp,
			venc_comp->callback_data,
			COMP_EVENT_CMD_ERROR,
			COMP_COMMAND_INIT,
			0,
			NULL);
	} else {
		venc_comp->callback->EventHandler(
			venc_comp->self_comp,
			venc_comp->callback_data,
			COMP_EVENT_CMD_COMPLETE,
			cmd,
			0,
			NULL);
	}

	if (wait_reply) {
		RT_LOGD("wait up: cmd = %d, state = %d", cmd, venc_comp->state);
		*wait_condition = 1;
		wake_up(wait_reply);
	}

	return 0;
}

/* the input buffer had useded, return it*/
/* valid_list --> empty_list */
static int venc_empty_in_buffer_done(void *venc_comp,
				     void *frame_info)
{
	struct venc_comp_ctx *pvenc_comp = (struct venc_comp_ctx *)venc_comp;
	video_frame_s *pframe_info       = (video_frame_s *)frame_info;

	comp_buffer_header_type buffer_header;
	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));
	buffer_header.private = pframe_info;

	if (pvenc_comp->in_port_tunnel_info.valid_flag == 0) {
		pvenc_comp->callback->empty_in_buffer_done(
			pvenc_comp->self_comp,
			pvenc_comp->callback_data,
			&buffer_header);
	} else {
		comp_fill_this_out_buffer(pvenc_comp->in_port_tunnel_info.tunnel_comp, &buffer_header);
	}

	RT_LOGI("venc_comp %px channel id %d release input FrameId[%d]", venc_comp, pvenc_comp->base_config.channel_id, pframe_info->id);
	return ERROR_TYPE_OK;
}

/* empty_list --> valid_list */
static int venc_fill_out_buffer_done(struct venc_comp_ctx *venc_comp,
				     VencOutputBuffer *out_buffer)
{
	video_stream_node *stream_node = NULL;
	error_type error	       = ERROR_TYPE_OK;
	comp_buffer_header_type buffer_header;
	video_stream_s mvideo_stream;

	memset(&mvideo_stream, 0, sizeof(video_stream_s));
	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));

	mutex_lock(&venc_comp->out_buf_manager.mutex);
	stream_node = list_first_entry(&venc_comp->out_buf_manager.empty_stream_list, video_stream_node, mList);

	config_videostream_by_VencOutputBuffer(venc_comp, out_buffer, &stream_node->video_stream);
	memcpy(&mvideo_stream, &stream_node->video_stream, sizeof(video_stream_s));

	venc_comp->out_buf_manager.empty_num--;
	list_move_tail(&stream_node->mList, &venc_comp->out_buf_manager.valid_stream_list);
	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	buffer_header.private = &mvideo_stream;
	if (venc_comp->out_port_tunnel_info.valid_flag == 0) {
		venc_comp->callback->fill_out_buffer_done(
			venc_comp->self_comp,
			venc_comp->callback_data,
			&buffer_header);
	} else {
		comp_empty_this_in_buffer(venc_comp->out_port_tunnel_info.tunnel_comp, &buffer_header);
	}

	RT_LOGD(" id = %d", mvideo_stream.id);
	return error;
}


static int catch_jpeg_encoder(venc_comp_ctx *venc_comp, VencInputBuffer *in_buf)
{
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;
	int result		 = 0;

	result = VencQueueInputBuf(jpeg_cxt->vencoder, in_buf);
	if (result != 0) {
		RT_LOGE("fatal error! VencQueueInputBuf fail[%d]", result);
	}

	LOCK_MUTEX(&venc_mutex);
	result = encodeOneFrame(jpeg_cxt->vencoder);
	UNLOCK_MUTEX(&venc_mutex);

	RT_LOGI("encoder result = %d", result);

	jpeg_cxt->wait_enc_finish_condition = 1;
	wake_up(&jpeg_cxt->wait_enc_finish);

	jpeg_cxt->encoder_finish_flag = 1;

	return 0;
}

static int catch_jpeg_start(venc_comp_ctx *venc_comp, catch_jpeg_config *jpeg_config)
{
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;
	VENC_CODEC_TYPE type     = VENC_CODEC_JPEG;
	VencBaseConfig base_config;
	unsigned int vbv_size = jpeg_config->width * jpeg_config->height / 2;
	unsigned int quality  = jpeg_config->qp;

	jpeg_cxt->width  = jpeg_config->width;
	jpeg_cxt->height = jpeg_config->height;
	jpeg_cxt->qp     = jpeg_config->qp;

	LOCK_MUTEX(&venc_mutex);
	jpeg_cxt->vencoder = VencCreate(type);
	UNLOCK_MUTEX(&venc_mutex);

	if (!jpeg_cxt->vencoder) {
		RT_LOGE("create vencoder failed");
		return -1;
	}

	VencSetParameter(jpeg_cxt->vencoder, VENC_IndexParamSetVbvSize, &vbv_size);
	VencSetParameter(jpeg_cxt->vencoder, VENC_IndexParamJpegQuality, &quality);

	memset(&base_config, 0, sizeof(VencBaseConfig));

	base_config.bEncH264Nalu	   = 0;
	base_config.nInputWidth		   = venc_comp->base_config.src_width;
	base_config.nInputHeight	   = venc_comp->base_config.src_height;
	base_config.nDstWidth		   = jpeg_cxt->width;
	base_config.nDstHeight		   = jpeg_cxt->height;
	base_config.nStride		   = RT_ALIGN(base_config.nInputWidth, 16);
	base_config.eInputFormat	   = VENC_PIXEL_YUV420SP;
	base_config.bOnlyWbFlag		   = 0;
	base_config.memops		   = NULL;
	base_config.veOpsS		   = NULL;
	base_config.bLbcLossyComEnFlag2x   = 0;
	base_config.bLbcLossyComEnFlag2_5x = 0;

	if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_25X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2_5x = 1;
	} else if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2x = 1;
	}

	LOCK_MUTEX(&venc_mutex);
	VencInit(jpeg_cxt->vencoder, &base_config);
	UNLOCK_MUTEX(&venc_mutex);

	/* timeout is 2000 ms: HZ is 100, HZ == 1s, so 1 jiffies is 10 ms */
	jpeg_cxt->wait_enc_finish_condition = 0;
	jpeg_cxt->enable		    = 1;
	wait_event_timeout(jpeg_cxt->wait_enc_finish, jpeg_cxt->wait_enc_finish_condition, 200);

	if (jpeg_cxt->wait_enc_finish_condition == 0) {
		RT_LOGE("wait for enc finish timeout");
		return -1;
	}

	return 0;
}

static int catch_jpeg_stop(venc_comp_ctx *venc_comp)
{
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;

	if (jpeg_cxt->vencoder) {
		LOCK_MUTEX(&venc_mutex);
		VencDestroy(jpeg_cxt->vencoder);
		jpeg_cxt->vencoder = NULL;
		UNLOCK_MUTEX(&venc_mutex);
	} else {
		RT_LOGE("the jpeg_cxt->vencoder is null");
		return -1;
	}

	jpeg_cxt->enable = 0;

	return 0;
}

static int catch_jpeg_get_data(venc_comp_ctx *venc_comp, void *user_buf_info)
{
	int result = 0;
	VencOutputBuffer out_buffer;
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;
	catch_jpeg_buf_info src_buf_info;

	if (jpeg_cxt->enable == 0 || !jpeg_cxt->vencoder) {
		RT_LOGE("error: enable = %d, vencoder = %px", jpeg_cxt->enable, jpeg_cxt->vencoder);
		return -1;
	}

	memset(&src_buf_info, 0, sizeof(catch_jpeg_buf_info));
	memset(&out_buffer, 0, sizeof(VencOutputBuffer));

	if (copy_from_user(&src_buf_info, (void __user *)user_buf_info, sizeof(struct catch_jpeg_buf_info))) {
		RT_LOGE("IOCTL_CATCH_JPEG_START copy_from_user fail\n");
		return -EFAULT;
	}

	result = VencDequeueOutputBuf(jpeg_cxt->vencoder, &out_buffer);
	if (result != 0) {
		RT_LOGE("have no bitstream");
		return -1;
	}

	RT_LOGI("data_size = %d, max_size = %d",
		out_buffer.nSize0, src_buf_info.buf_size);

	if (out_buffer.nSize0 > src_buf_info.buf_size) {
		RT_LOGE("buf_size overflow: %d > %d", out_buffer.nSize0, src_buf_info.buf_size);
		result = -1;
		goto catch_jpeg_get_data_exit;
	}

	if (copy_to_user((void *)src_buf_info.buf, out_buffer.pData0, out_buffer.nSize0)) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto catch_jpeg_get_data_exit;
	}

	if (copy_to_user((void *)user_buf_info, &out_buffer.nSize0, sizeof(unsigned int))) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto catch_jpeg_get_data_exit;
	}

catch_jpeg_get_data_exit:

	VencQueueOutputBuf(jpeg_cxt->vencoder, &out_buffer);

	return result;
}

typedef struct osd_convert_dst_info {
	unsigned int dst_ext_buf_size;
	unsigned int dst_w_ext;
	unsigned int dst_h_ext;
	unsigned int dst_crop_x;
	unsigned int dst_crop_y;
	unsigned int dst_crop_w;
	unsigned int dst_crop_h;
	unsigned int dst_start_x;
	unsigned int dst_start_y;
} osd_convert_dst_info;

typedef struct osd_convert_src_info {
	unsigned int start_x;
	unsigned int start_y;
	unsigned int widht;
	unsigned int height;
} osd_convert_src_info;

#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
int convert_overlay_info_native(venc_comp_ctx *venc_comp, VencOverlayInfoS *pvenc_osd, VideoInputOSD *user_osd_info)
{
	int i			     = 0;
	VencOverlayHeaderS *dst_item = NULL;
	OverlayItemInfo *src_item    = NULL;
	int align_end_x		     = 0;
	int align_end_y		     = 0;
	struct mem_param param;
	unsigned int in_phy_addr = 0;
	osd_convert_dst_info osd_dst_info;
	osd_convert_src_info osd_src_info;

	memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
	memset(&osd_src_info, 0, sizeof(osd_convert_src_info));

	if (global_osd_info == NULL) {
		RT_LOGW("the global_osd_info is null");
		return -1;
	}

	if (global_osd_info->memops == NULL) {
		global_osd_info->memops = mem_create(MEM_TYPE_ION, param);
		if (global_osd_info->memops == NULL) {
			RT_LOGE("mem_create failed\n");
			return -1;
		}
	}

	if (global_osd_info->ori_frame_w == 0 || global_osd_info->ori_frame_h == 0) {
		global_osd_info->ori_frame_w = venc_comp->base_config.dst_width;
		global_osd_info->ori_frame_h = venc_comp->base_config.dst_height;
	}

	pvenc_osd->blk_num	  = user_osd_info->osd_num;
	pvenc_osd->argb_type	= user_osd_info->argb_type;
	pvenc_osd->is_user_buf_flag = 0;

	for (i = 0; i < pvenc_osd->blk_num; i++) {
		if (i >= GLOBAL_OSD_MAX_NUM) {
			RT_LOGW("osd num overlay: blk_num = %d, GLOBAL_OSD_MAX_NUM = %d",
				pvenc_osd->blk_num, GLOBAL_OSD_MAX_NUM);
			break;
		}
		dst_item = &pvenc_osd->overlayHeaderList[i];
		src_item = &user_osd_info->item_info[i];
		/* copy osd data to native */
		if (global_osd_info->g2d_in_ion_vir_addr[i] == NULL || global_osd_info->g2d_in_buf_size[i] < src_item->data_size) {
			if (global_osd_info->g2d_in_ion_vir_addr[i])
				//cdc_mem_pfree(global_osd_info->memops, global_osd_info->g2d_in_ion_vir_addr[i]);

			//global_osd_info->g2d_in_ion_vir_addr[i] = cdc_mem_palloc(global_osd_info->memops, src_item->data_size);
			global_osd_info->g2d_in_buf_size[i]     = src_item->data_size;
		}
		if (copy_from_user(global_osd_info->g2d_in_ion_vir_addr[i], (void __user *)src_item->data_buf, src_item->data_size)) {
			RT_LOGE("g2d_scale copy_from_user fail\n");
			return -1;
		}

		//cdc_mem_flush_cache(global_osd_info->memops, global_osd_info->g2d_in_ion_vir_addr[i], src_item->data_size);

		//in_phy_addr = cdc_mem_get_phy(global_osd_info->memops, global_osd_info->g2d_in_ion_vir_addr[i]);

		memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
		memset(&osd_src_info, 0, sizeof(osd_convert_src_info));

		osd_src_info.start_x = src_item->start_x;
		osd_src_info.start_y = src_item->start_y;
		osd_src_info.widht   = src_item->widht;
		osd_src_info.height  = src_item->height;

		convert_osd_pos_info(venc_comp, &osd_dst_info, &osd_src_info);

		align_end_x = osd_dst_info.dst_start_x + osd_dst_info.dst_w_ext;
		align_end_y = osd_dst_info.dst_start_y + osd_dst_info.dst_h_ext;

		dst_item->start_mb_x = osd_dst_info.dst_start_x / 16;
		dst_item->start_mb_y = osd_dst_info.dst_start_y / 16;
		dst_item->end_mb_x   = align_end_x / 16 - 1;
		dst_item->end_mb_y   = align_end_y / 16 - 1;

		dst_item->overlay_blk_addr    = src_item->data_buf;
		dst_item->bitmap_size	 = osd_dst_info.dst_ext_buf_size;
		dst_item->overlay_type	= NORMAL_OVERLAY;
		dst_item->extra_alpha_flag    = 0;
		dst_item->bforce_reverse_flag = 0;
		RT_LOGI("osd item[%d]: s_x&y = %d, %d; e_x&y = %d, %d, size = %d, buf = %px",
			i, dst_item->start_mb_x, dst_item->start_mb_y, dst_item->end_mb_x,
			dst_item->end_mb_y, dst_item->bitmap_size, dst_item->overlay_blk_addr);
	}

	return 0;
}

static int adjust_native_overlay_info(venc_comp_ctx *venc_comp)
{
	int i			     = 0;
	VencOverlayHeaderS *dst_item = NULL;
	int align_end_x		     = 0;
	int align_end_y		     = 0;
	osd_convert_dst_info osd_dst_info;
	osd_convert_src_info osd_src_info;

	memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
	memset(&osd_src_info, 0, sizeof(osd_convert_src_info));

	if (global_osd_info == NULL) {
		RT_LOGW("the global_osd_info is null");
		return -1;
	}

	for (i = 0; i < global_osd_info->sOverlayInfo.blk_num; i++) {
		dst_item = &global_osd_info->sOverlayInfo.overlayHeaderList[i];

		memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
		memset(&osd_src_info, 0, sizeof(osd_convert_src_info));
		convert_osd_pos_info(venc_comp, &osd_dst_info, &osd_src_info);

		align_end_x = osd_dst_info.dst_start_x + osd_dst_info.dst_w_ext;
		align_end_y = osd_dst_info.dst_start_y + osd_dst_info.dst_h_ext;

		dst_item->start_mb_x = osd_dst_info.dst_start_x / 16;
		dst_item->start_mb_y = osd_dst_info.dst_start_y / 16;
		dst_item->end_mb_x   = align_end_x / 16 - 1;
		dst_item->end_mb_y   = align_end_y / 16 - 1;

		dst_item->bitmap_size = osd_dst_info.dst_ext_buf_size;
		RT_LOGI("osd item[%d]: s_x&y = %d, %d; e_x&y = %d, %d, size = %d, buf = %px",
			i, dst_item->start_mb_x, dst_item->start_mb_y, dst_item->end_mb_x,
			dst_item->end_mb_y, dst_item->bitmap_size, dst_item->overlay_blk_addr);
	}
	return 0;
}

#else
static int convert_overlay_info(VencOverlayInfoS *pvenc_osd, VideoInputOSD *user_osd_info)
{
	int i			     = 0;
	unsigned char *argb_addr = NULL;

	VencOverlayHeaderS *dst_item = NULL;
	OverlayItemInfo *src_item    = NULL;
	int align_start_x	    = 0;
	int align_start_y	    = 0;
	int align_end_x		     = 0;
	int align_end_y		     = 0;

	pvenc_osd->blk_num	  = user_osd_info->osd_num;
	pvenc_osd->argb_type	= user_osd_info->argb_type;
	pvenc_osd->is_user_buf_flag = 1;
	pvenc_osd->invert_mode = user_osd_info->invert_mode;
	pvenc_osd->invert_threshold = user_osd_info->invert_threshold;

	for (i = 0; i < pvenc_osd->blk_num; i++) {
		dst_item = &pvenc_osd->overlayHeaderList[i];
		src_item = &user_osd_info->item_info[i];
		if (src_item->data_buf && src_item->data_size && src_item->osd_type != RT_COVER_OSD) {
			argb_addr = vmalloc(src_item->data_size);
			 if (argb_addr)
				if (copy_from_user(argb_addr, (void __user *)src_item->data_buf, src_item->data_size)) {
					loge("copy_from_user fail\n");
					return -EFAULT;
				}
		} else {
			argb_addr = NULL;
		}
		align_start_x = RT_ALIGN(src_item->start_x, 16);
		align_start_y = RT_ALIGN(src_item->start_y, 16);
		align_end_x   = align_start_x + src_item->widht;
		align_end_y   = align_start_y + src_item->height;

		dst_item->overlay_blk_addr = argb_addr;
		dst_item->bitmap_size      = src_item->data_size;
		dst_item->start_mb_x       = align_start_x / 16;
		dst_item->start_mb_y       = align_start_y / 16;
		dst_item->end_mb_x	 = align_end_x / 16 - 1;
		dst_item->end_mb_y	 = align_end_y / 16 - 1;

		dst_item->overlay_type	= src_item->osd_type;
		if (dst_item->overlay_type == COVER_OVERLAY)
			memcpy(&dst_item->cover_yuv, &src_item->cover_yuv, sizeof(VencOverlayCoverYuvS));

		dst_item->extra_alpha_flag    = 0;
		dst_item->bforce_reverse_flag = 0;
		RT_LOGI("osd item[%d]: s_x = %d, s_y = %d, e_x = %d, e_y = %d, size = %d, buf = %px",
			i, dst_item->start_mb_x, dst_item->start_mb_y, dst_item->end_mb_x,
			dst_item->end_mb_y, dst_item->bitmap_size, dst_item->overlay_blk_addr);
	}
	return 0;
}
#endif
static int flush_buffer(venc_comp_ctx *venc_comp)
{
	video_frame_node *frame_node = NULL;
	video_frame_s cur_video_frame;

	memset(&cur_video_frame, 0, sizeof(video_frame_s));

	mutex_lock(&venc_comp->in_buf_manager.mutex);
	while (!list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
		frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
		memcpy(&cur_video_frame, &frame_node->video_frame, sizeof(video_frame_s));
		list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
		venc_comp->in_buf_manager.empty_num++;

		RT_LOGW("flush in frame buf, frame_node = %px", frame_node);

		mutex_unlock(&venc_comp->in_buf_manager.mutex);

		venc_empty_in_buffer_done(venc_comp, &cur_video_frame);

		mutex_lock(&venc_comp->in_buf_manager.mutex);
	}
	mutex_unlock(&venc_comp->in_buf_manager.mutex);

	return 0;
}
static error_type config_dynamic_param(
	venc_comp_ctx *venc_comp,
	comp_index_type index,
	void *param_data)
{
	error_type error = ERROR_TYPE_OK;
	int ret		 = 0;

	if (venc_comp->vencoder == NULL) {
		RT_LOGW("vencoder is not be created when config dynamic param");
		return ERROR_TYPE_ERROR;
	}

	switch (index) {
	case COMP_INDEX_VENC_CONFIG_Dynamic_ForceKeyFrame: {
		RT_LOGI("*****ForceKeyFrame***** ");
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamForceKeyFrame, param_data);
		UNLOCK_MUTEX(&venc_mutex);

		break;
	}
	case COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START: {
		catch_jpeg_config *jpeg_config = (catch_jpeg_config *)param_data;
		if (catch_jpeg_start(venc_comp, jpeg_config) != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_CATCH_JPEG_STOP: {
		if (catch_jpeg_stop(venc_comp) != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_CATCH_JPEG_GET_DATA: {
		if (catch_jpeg_get_data(venc_comp, param_data) != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_OSD: {
#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
		VencOverlayInfoS *pOverlayInfo = &global_osd_info->sOverlayInfo;
		VideoInputOSD inputOsd;
		struct file g2d_file;

		memset(pOverlayInfo, 0, sizeof(VencOverlayInfoS));
		memset(&inputOsd, 0, sizeof(VideoInputOSD));

		if (copy_from_user(&inputOsd, (void __user *)param_data, sizeof(VideoInputOSD))) {
			RT_LOGE("set osd copy_from_user fail\n");
			return -EFAULT;
		}

		convert_overlay_info_native(venc_comp, pOverlayInfo, &inputOsd);

		LOCK_MUTEX(&venc_mutex);
		g2d_open(0, &g2d_file);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetOverlay, pOverlayInfo);
		g2d_release(0, &g2d_file);
		UNLOCK_MUTEX(&venc_mutex);
#else
		VencOverlayInfoS *pOverlayInfo;
		VideoInputOSD *pinputOsd;
		int i = 0;

		pOverlayInfo = kmalloc(sizeof(VencOverlayInfoS), GFP_KERNEL);
		pinputOsd = kmalloc(sizeof(VideoInputOSD), GFP_KERNEL);
		memset(pOverlayInfo, 0, sizeof(VencOverlayInfoS));
		memset(pinputOsd, 0, sizeof(VideoInputOSD));

		if (copy_from_user(pinputOsd, (void __user *)param_data, sizeof(VideoInputOSD))) {
			RT_LOGE("set osd copy_from_user fail\n");
			return -EFAULT;
		}

		convert_overlay_info(pOverlayInfo, pinputOsd);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetOverlay, pOverlayInfo);
		UNLOCK_MUTEX(&venc_mutex);
		for (i = 0; i < pOverlayInfo->blk_num; i++) {
			if (pOverlayInfo->overlayHeaderList[i].overlay_blk_addr) {
				vfree(pOverlayInfo->overlayHeaderList[i].overlay_blk_addr);
				pOverlayInfo->overlayHeaderList[i].overlay_blk_addr = NULL;
			}
		}
		kfree(pOverlayInfo);
#endif
		RT_LOGI("ENABLE_SAVE_NATIVE_OVERLAY_DATA = %d", ENABLE_SAVE_NATIVE_OVERLAY_DATA);
		break;
	}
//	case COMP_INDEX_VENC_CONFIG_ENABLE_BIN_IMAGE: {
//		rt_venc_bin_image_param *bin_param = (rt_venc_bin_image_param *)param_data;
//		VencBinImageParam venc_bin_param;
//
//		venc_bin_param.enable = bin_param->enable;
//		venc_bin_param.moving_th = bin_param->moving_th;
//
//		RT_LOGI("COMP_INDEX_VENC_CONFIG_ENABLE_BIN_IMAGE: enable = %d, th = %d",
//				venc_bin_param.enable, venc_bin_param.moving_th);
//		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableGetBinImage, &venc_bin_param);
//		break;
//	}
//	case COMP_INDEX_VENC_CONFIG_ENABLE_MV_INFO: {
//		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableMvInfo, param_data);
//		break;
//	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_Flush_buffer: {
		flush_buffer(venc_comp);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_QP_RANGE: {
		VencQPRange mVencQpRange;
		video_qp_range *pQp_range = (video_qp_range *)param_data;

		RT_LOGW("SET_QP_RANGE: i_qp = %d ~ %d, p_qp = %d ~ %d, bEnMbQpLimit = %d",
			pQp_range->i_min_qp, pQp_range->i_max_qp,
			pQp_range->p_min_qp, pQp_range->p_max_qp, pQp_range->enable_mb_qp_limit);

		mVencQpRange.nMinqp       = pQp_range->i_min_qp;
		mVencQpRange.nMaxqp       = pQp_range->i_max_qp;
		mVencQpRange.nMinPqp      = pQp_range->p_min_qp;
		mVencQpRange.nMaxPqp      = pQp_range->p_max_qp;
		mVencQpRange.nQpInit      = pQp_range->i_init_qp;
		mVencQpRange.bEnMbQpLimit = pQp_range->enable_mb_qp_limit;

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264QPRange, &mVencQpRange);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_BITRATE: {
		int bitrate = *((int *)param_data);

		RT_LOGI("SET_BITRATE: bitrate = %d", bitrate);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamBitrate, &bitrate);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS: {
		int fps = *((int *)param_data);

		RT_LOGI("SET_FPS: fps = %d", fps);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamFramerate, &fps);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_VBR_PARAM: {
		RTVencVbrParam *rt_vbr_param = (RTVencVbrParam *)param_data;
		VencVbrParam mVbrParam;

		memset(&mVbrParam, 0, sizeof(VencVbrParam));

		mVbrParam.uMaxBitRate   = rt_vbr_param->uMaxBitRate * 1024;
		mVbrParam.nMovingTh     = rt_vbr_param->nMovingTh;
		mVbrParam.nQuality      = rt_vbr_param->nQuality;
		mVbrParam.nIFrmBitsCoef = rt_vbr_param->nIFrmBitsCoef;
		mVbrParam.nPFrmBitsCoef = rt_vbr_param->nPFrmBitsCoef;

		RT_LOGI("SET_VBR_PARAM: uMaxBitRate = %d, nMovingTh = %d, nQuality = %d, coef = %d, %d",
			mVbrParam.uMaxBitRate, mVbrParam.nMovingTh,
			mVbrParam.nQuality, mVbrParam.nIFrmBitsCoef, mVbrParam.nPFrmBitsCoef);

		LOCK_MUTEX(&venc_mutex);
		ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetVbrParam, &mVbrParam);
		if (ret != 0)
			error = ERROR_TYPE_ERROR;
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_GET_SUM_MB_INFO: {
		//RTVencMBSumInfo *prt_mb_sum = (RTVencMBSumInfo *)param_data;
		VencMBSumInfo venc_mb_sum;

		memset(&venc_mb_sum, 0, sizeof(VencMBSumInfo));

		ret = VencGetParameter(venc_comp->vencoder, VENC_IndexParamMBSumInfoOutput, &venc_mb_sum);
		if (ret != 0)
			error = ERROR_TYPE_ERROR;
/* //todo
		prt_mb_sum->sum_mad = venc_mb_sum.sum_mad;
		prt_mb_sum->sum_qp  = venc_mb_sum.sum_qp;
		prt_mb_sum->sum_sse = venc_mb_sum.sum_sse;
		prt_mb_sum->avg_sse = venc_mb_sum.avg_sse;
		RT_LOGI("GET_SUM_MB_INFO: sum_mad = %d, sum_qp = %d, sum_sse = %lld, avg_sse = %d",
			prt_mb_sum->sum_mad, prt_mb_sum->sum_qp, prt_mb_sum->sum_sse, prt_mb_sum->avg_sse);
*/
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_IS_NIGHT_CASE: {
		int is_night_case = *(int *)param_data;

		RT_LOGW("is_night_case = %d", is_night_case);

		if (venc_comp->vencoder) {
			LOCK_MUTEX(&venc_mutex);
			ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamIsNightCaseFlag, &is_night_case);
			if (ret != 0)
				error = ERROR_TYPE_ERROR;
			UNLOCK_MUTEX(&venc_mutex);
		}

		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_SUPER_FRAME_PARAM: {
		RTVencSuperFrameConfig *pconfig = (RTVencSuperFrameConfig *)param_data;

		if (venc_comp->vencoder) {
			LOCK_MUTEX(&venc_mutex);
			ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamSuperFrameConfig, pconfig);
			if (ret != 0)
				error = ERROR_TYPE_ERROR;
			UNLOCK_MUTEX(&venc_mutex);
		}

		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_SHARP: {
//		RTsEncppSharp mSharp;
//		RTsEncppSharpParamDynamic dynamic_sharp;
//		RTsEncppSharpParamStatic static_sharp;
//
//		if (copy_from_user(&mSharp, (void __user *)param_data, sizeof(RTsEncppSharp))) {
//			RT_LOGE("COMP_INDEX_VENC_CONFIG_SET_SHARP copy_from_user fail\n");
//			return -EFAULT;
//		}
//
//		if (copy_from_user(&dynamic_sharp, (void __user *)mSharp.sEncppSharpParam.pDynamicParam, sizeof(sEncppSharpParamDynamic))) {
//			RT_LOGE("pDynamicParam copy_from_user fail\n");
//			return -EFAULT;
//		}
//		if (copy_from_user(&static_sharp, (void __user *)mSharp.sEncppSharpParam.pStaticParam, sizeof(sEncppSharpParamStatic))) {
//			RT_LOGE("pStaticParam copy_from_user fail\n");
//			return -EFAULT;
//		}
//
//		mSharp.sEncppSharpParam.pDynamicParam = &dynamic_sharp;
//		mSharp.sEncppSharpParam.pStaticParam = &static_sharp;
//
//		RT_LOGD("hfr_hf_wht_clp %d max_clp_ratio %d", mSharp.sEncppSharpParam.pDynamicParam->hfr_hf_wht_clp, mSharp.sEncppSharpParam.pDynamicParam->max_clp_ratio);
//		RT_LOGD("ls_dir_ratio %d ss_dir_ratio %d", mSharp.sEncppSharpParam.pStaticParam->ls_dir_ratio, mSharp.sEncppSharpParam.pStaticParam->ss_dir_ratio);
//
//		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSharpConfig, &mSharp.sEncppSharpParam);

		int bSharp = *(int *)param_data;
		if (venc_comp->base_config.en_encpp_sharp != bSharp) {
			int vipp_id = venc_comp->base_config.channel_id;
			RT_LOGW("vipp:%d,venc_type:%d: en_encpp_sharp base config change:%d->%d", vipp_id, venc_comp->base_config.codec_type, venc_comp->base_config.en_encpp_sharp, bSharp);
			venc_comp->base_config.en_encpp_sharp = bSharp;
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableEncppSharp, (void *)&venc_comp->base_config.en_encpp_sharp);
		}
		break;
	}
	default: {
		RT_LOGE("nor support config index");
		return ERROR_TYPE_ILLEGAL_PARAM;
		break;
	}
	}

	return error;
}

static int post_msg_and_wait(venc_comp_ctx *venc_comp, int cmd_id)
{
	message_t cmd_msg;
	memset(&cmd_msg, 0, sizeof(message_t));
	cmd_msg.command	= cmd_id;
	cmd_msg.wait_queue     = (char *)&venc_comp->wait_reply[cmd_id];
	cmd_msg.wait_condition = (char *)&venc_comp->wait_reply_condition[cmd_id];

	venc_comp->wait_reply_condition[cmd_id] = 0;

	put_message(&venc_comp->msg_queue, &cmd_msg);

	RT_LOGD("wait for : cmd[%d], start", cmd_id);
	wait_event(venc_comp->wait_reply[cmd_id],
		   venc_comp->wait_reply_condition[cmd_id]);
	RT_LOGD("wait for : cmd[%d], finish", cmd_id);

	return 0;
}

error_type venc_comp_init(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_init: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	post_msg_and_wait(venc_comp, COMP_COMMAND_INIT);
	return error;
}

error_type venc_comp_start(
	PARAM_IN comp_handle component)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_start: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	post_msg_and_wait(venc_comp, COMP_COMMAND_PAUSE);

	/* set param for vencoder here as:
	 *	vencoder_set_param will palloc vbv-buf, and we must make sure the pid
	 *	of 'palloc vbv-buf' code is the same as ioctl-calling, to valid ion share
	 *	between user and kenrl.
	 */
	vencoder_set_param(venc_comp);

	venc_comp->vencCallBack.EventHandler = venc_comp_event_handler;
	VencSetCallbacks(venc_comp->vencoder, &venc_comp->vencCallBack, venc_comp, NULL);
	VencStart(venc_comp->vencoder);
	post_msg_and_wait(venc_comp, COMP_COMMAND_START);
	return error;
}

error_type venc_comp_pause(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_start: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	RT_LOGD("Ryan VencPause");
	VencPause(venc_comp->vencoder);

	post_msg_and_wait(venc_comp, COMP_COMMAND_PAUSE);

	return error;
}

error_type venc_comp_stop(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_stop: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	RT_LOGD("Ryan VencDestroy");
	//VencDestroy(venc_comp->vencoder);

	post_msg_and_wait(venc_comp, COMP_COMMAND_STOP);
	return error;
}

error_type venc_comp_destroy(
	PARAM_IN comp_handle component)
{
	int free_frame_cout		= 0;
	int free_stream_cout		= 0;
	video_stream_node *stream_node  = NULL;
	video_frame_node *frame_node    = NULL;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_destroy: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	/*should make sure the thread do nothing importent */
	if (venc_comp->venc_thread)
		kthread_stop(venc_comp->venc_thread);

	message_destroy(&venc_comp->msg_queue);

	if (venc_comp->vencoder) {
		LOCK_MUTEX(&venc_mutex);
		VencDestroy(venc_comp->vencoder);
		UNLOCK_MUTEX(&venc_mutex);
	}

	while ((!list_empty(&venc_comp->in_buf_manager.empty_frame_list))) {
		frame_node = list_first_entry(&venc_comp->in_buf_manager.empty_frame_list, video_frame_node, mList);
		if (frame_node) {
			list_del(&frame_node->mList);
			free_frame_cout++;
			kfree(frame_node);
		}
	}

	while ((!list_empty(&venc_comp->in_buf_manager.valid_frame_list))) {
		frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
		if (frame_node) {
			list_del(&frame_node->mList);
			free_frame_cout++;
			kfree(frame_node);
		}
	}

	if (free_frame_cout != VENC_IN_BUFFER_LIST_NODE_NUM)
		RT_LOGE("free num of frame node is not match: %d, %d", free_frame_cout, VENC_IN_BUFFER_LIST_NODE_NUM);

	RT_LOGD("free_frame_cout = %d", free_frame_cout);

	while ((!list_empty(&venc_comp->out_buf_manager.empty_stream_list))) {
		stream_node = list_first_entry(&venc_comp->out_buf_manager.empty_stream_list, video_stream_node, mList);
		if (stream_node) {
			list_del(&stream_node->mList);
			free_stream_cout++;
			kfree(stream_node);
		}
	}

	while ((!list_empty(&venc_comp->out_buf_manager.valid_stream_list))) {
		stream_node = list_first_entry(&venc_comp->out_buf_manager.valid_stream_list, video_stream_node, mList);
		if (stream_node) {
			list_del(&stream_node->mList);
			free_stream_cout++;
			kfree(stream_node);
		}
	}

	if (free_stream_cout != VENC_OUT_BUFFER_LIST_NODE_NUM)
		RT_LOGE("free num of stream node is not match: %d, %d", free_stream_cout, VENC_OUT_BUFFER_LIST_NODE_NUM);

	RT_LOGD("free_stream_cout = %d", free_stream_cout);

	kfree(venc_comp);

	return error;
}

error_type venc_comp_get_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_INOUT void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_get_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	RT_LOGD("venc_comp_get_config : %d, start", index);
	switch (index) {
	case COMP_INDEX_VENC_CONFIG_GET_WBYUV: {
		RTVencThumbInfo *p_wb_info = (RTVencThumbInfo *)param_data;
		VencThumbInfo s_venc_thumb_info;
		memset(&s_venc_thumb_info, 0, sizeof(VencThumbInfo));

		s_venc_thumb_info.nThumbSize = p_wb_info->nThumbSize;
		s_venc_thumb_info.pThumbBuf = p_wb_info->pThumbBuf;

		LOCK_MUTEX(&venc_mutex);
		VencGetParameter(venc_comp->vencoder, VENC_IndexParamGetThumbYUV, &s_venc_thumb_info);
		UNLOCK_MUTEX(&venc_mutex);
		RT_LOGD("nThumbSize %d %px", p_wb_info->nThumbSize, p_wb_info->pThumbBuf);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_GET_VBV_BUF_INFO: {
		KERNEL_VBV_BUFFER_INFO *param_vbv = (KERNEL_VBV_BUFFER_INFO *)param_data;
		VbvInfo vbv_info;
		int share_fd = 0;

		memset(&vbv_info, 0, sizeof(VbvInfo));

		if (venc_comp->vencoder_init_flag == 0) {
			RT_LOGD("get vbv info: vencoder not init");
			return ERROR_TYPE_ERROR;
		}

		VencGetParameter(venc_comp->vencoder, VENC_IndexParamVbvInfo, &vbv_info);
		param_vbv->size = vbv_info.vbv_size;

		VencGetParameter(venc_comp->vencoder, VENC_IndexParamGetVbvShareFd, &share_fd);
		param_vbv->share_fd = share_fd;

		RT_LOGW("get vbv info: share_fd = %d, size = %d", param_vbv->share_fd, param_vbv->size);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_GET_STREAM_HEADER: {
		venc_comp_header_data *header_data = (venc_comp_header_data *)param_data;
		VencHeaderData sps_pps_data;
		memset(&sps_pps_data, 0, sizeof(VencHeaderData));

		if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H264) {
			VencGetParameter(venc_comp->vencoder, VENC_IndexParamH264SPSPPS, &sps_pps_data);
		} else if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H265) {
			VencGetParameter(venc_comp->vencoder, VENC_IndexParamH265Header, &sps_pps_data);
		} else {
			RT_LOGW("not support COMP_INDEX_VENC_CONFIG_GET_VBV_BUF_INFO yet");
			return ERROR_TYPE_ERROR;
		}
		RT_LOGD("1 get stream header = %d, data = %x %x %x %x %x %x %x %x", sps_pps_data.nLength,
			sps_pps_data.pBuffer[0],
			sps_pps_data.pBuffer[1],
			sps_pps_data.pBuffer[2],
			sps_pps_data.pBuffer[3],
			sps_pps_data.pBuffer[4],
			sps_pps_data.pBuffer[5],
			sps_pps_data.pBuffer[6],
			sps_pps_data.pBuffer[7]);
		RT_LOGD("2 get stream header size = %d, data = %x %x %x %x %x %x %x %x", sps_pps_data.nLength,
			sps_pps_data.pBuffer[8],
			sps_pps_data.pBuffer[9],
			sps_pps_data.pBuffer[10],
			sps_pps_data.pBuffer[11],
			sps_pps_data.pBuffer[12],
			sps_pps_data.pBuffer[13],
			sps_pps_data.pBuffer[14],
			sps_pps_data.pBuffer[15]);
		RT_LOGD("3 get stream header size = %d, data = %x %x %x %x %x %x %x", sps_pps_data.nLength,
			sps_pps_data.pBuffer[16],
			sps_pps_data.pBuffer[17],
			sps_pps_data.pBuffer[18],
			sps_pps_data.pBuffer[19],
			sps_pps_data.pBuffer[20],
			sps_pps_data.pBuffer[21],
			sps_pps_data.pBuffer[22]);

		header_data->pBuffer = sps_pps_data.pBuffer;
		header_data->nLength = sps_pps_data.nLength;

		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT: {
		RTVencMotionSearchResult *motion_result = (RTVencMotionSearchResult *)param_data;

		RT_LOGI("COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT");

		return VencGetParameter(venc_comp->vencoder, VENC_IndexParamMotionSearchResult, motion_result);
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_GET_REGION_D3D_RESULT: {
		RTVencRegionD3DResult *pRegionD3DResult = (RTVencRegionD3DResult *)param_data;
		error = VencGetParameter(venc_comp->vencoder, VENC_IndexParamRegionD3DResult, pRegionD3DResult);
		break;
	}
	default: {
		RT_LOGW("get config: not support the index = 0x%x", index);
		break;
	}
	}
	RT_LOGD("venc_comp_get_config : %d, finish", index);
	return error;
}

error_type venc_comp_set_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_IN void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_set_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	switch (index) {
	case COMP_INDEX_VENC_CONFIG_Base: {
		venc_comp_base_config *base_config = (venc_comp_base_config *)param_data;
		memcpy(&venc_comp->base_config, base_config, sizeof(venc_comp_base_config));
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Normal: {
		venc_comp_normal_config *normal_config = (venc_comp_normal_config *)param_data;
		memcpy(&venc_comp->normal_config, normal_config, sizeof(venc_comp_normal_config));
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_MOTION_SEARCH_PARAM: {
		if (venc_comp->base_config.codec_type == RT_VENC_CODEC_H264
			|| venc_comp->base_config.codec_type == RT_VENC_CODEC_H265) {
			LOCK_MUTEX(&venc_mutex);
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamMotionSearchParam, (RTVencMotionSearchParam *)param_data);
			UNLOCK_MUTEX(&venc_mutex);
		}
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_IPC_CASE: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamProductCase, (int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_ROI: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamROIConfig, (RTVencROIConfig *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_GDC: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamGdcConfig, (RTsGdcParam *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_ROTATE: {
		if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_25X || venc_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
			RT_LOGW("lbc format not support rotate!");
			break;
		}
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamRotation, (unsigned int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_REC_REF_LBC_MODE: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetRecRefLbcMode, (RTeVeLbcMode *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_WEAK_TEXT_TH: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamWeakTextTh, (float *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_REGION_D3D_PARAM: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamRegionD3DParam, (RTVencRegionD3DParam *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_CHROMA_QP_OFFSET: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamChromaQPOffset, (int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_H264_CONSTRAINT_FLAG: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264ConstraintFlag, (RTVencH264ConstraintFlag *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_VE2ISP_D2D_LIMIT: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamVe2IspD2DLimit, (RTVencVe2IspD2DLimit *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_GRAY: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamChmoraGray, (unsigned int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_WBYUV: {
		RTsWbYuvParam *p_wbyuv_param = (RTsWbYuvParam *)param_data;
		if (p_wbyuv_param->bEnableWbYuv) {//en wbyuv will set int venc_init.
			memcpy(&venc_comp->base_config.s_wbyuv_param, p_wbyuv_param, sizeof(RTsWbYuvParam));
		} else {
			LOCK_MUTEX(&venc_mutex);
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableWbYuv, (RTsWbYuvParam *)param_data);
			UNLOCK_MUTEX(&venc_mutex);
		}
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_2DNR: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParam2DFilter, (s2DfilterParam *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_3DNR: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParam3DFilterNew, (s3DfilterParam *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_CYCLE_INTRA_REFRESH: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264CyclicIntraRefresh, (VencCyclicIntraRefresh *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_P_FRAME_INTRA: {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamPFrameIntraEn, (unsigned int *)param_data);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	default: {
		error = config_dynamic_param(venc_comp, index, param_data);
		break;
	}
	}

	return error;
}

error_type venc_comp_get_state(
	PARAM_IN comp_handle component,
	PARAM_OUT comp_state_type *pState)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_get_state: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	*pState = venc_comp->state;
	return error;
}

/* empty_list --> valid_list */
error_type venc_comp_empty_this_in_buffer(
	PARAM_IN comp_handle component,
	PARAM_IN comp_buffer_header_type *buffer_header)
{
	video_frame_node *pNode		= NULL;
	video_frame_s *src_frame	= NULL;
	video_frame_node *first_node    = NULL;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;
	message_t msg;
	static int cnt_1 = 1;

	memset(&msg, 0, sizeof(message_t));

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_empty_this_in_buffer: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	if (venc_comp->state != COMP_STATE_EXECUTING) {
		RT_LOGW("channel %d submit inbuf when venc state[0x%x] isn not executing",
			venc_comp->base_config.channel_id, venc_comp->state);
	}

	src_frame = (video_frame_s *)buffer_header->private;

	if (src_frame == NULL) {
		RT_LOGE("submit in buf: frame is null");
		error = ERROR_TYPE_ILLEGAL_PARAM;
		goto submit_exit;
	}

	RT_LOGI("empty buf: pts = %lld", src_frame->pts);

	mutex_lock(&venc_comp->in_buf_manager.mutex);

	if (list_empty(&venc_comp->in_buf_manager.empty_frame_list)) {
		RT_LOGW("Low probability! venc idle frame is empty!");
		if (venc_comp->in_buf_manager.empty_num != 0) {
			RT_LOGE("fatal error! empty_num must be zero!");
		}

		pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			mutex_unlock(&venc_comp->in_buf_manager.mutex);
			goto submit_exit;
		}
		memset(pNode, 0, sizeof(video_frame_node));
		list_add_tail(&pNode->mList, &venc_comp->in_buf_manager.empty_frame_list);
		venc_comp->in_buf_manager.empty_num++;
	}

	first_node = list_first_entry(&venc_comp->in_buf_manager.empty_frame_list, video_frame_node, mList);

	memcpy(&first_node->video_frame, src_frame, sizeof(video_frame_s));
	venc_comp->in_buf_manager.empty_num--;

	list_move_tail(&first_node->mList, &venc_comp->in_buf_manager.valid_frame_list);
#if 1
	if (cnt_1) {
		cnt_1 = 0;
		RT_LOGW("channel %d first time add buffer_node to valid_frame_list, time: %lld",
			venc_comp->base_config.channel_id, get_cur_time());
	}
#endif
	if (venc_comp->in_buf_manager.no_frame_flag) {
		venc_comp->in_buf_manager.no_frame_flag = 0;
		msg.command				      = COMP_COMMAND_VENC_INPUT_FRAME_VALID;
		put_message(&venc_comp->msg_queue, &msg);
	}

	mutex_unlock(&venc_comp->in_buf_manager.mutex);
submit_exit:

	return error;
}

/* valid_list --> empty_list */
error_type venc_comp_fill_this_out_buffer(
	PARAM_IN comp_handle component,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;
	video_stream_s *src_stream      = NULL;
	video_stream_node *stream_node  = NULL;
	VencOutputBuffer output_buffer;

	memset(&output_buffer, 0, sizeof(VencOutputBuffer));

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_fill_this_out_buffer: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	src_stream = (video_stream_s *)pBuffer->private;

	RT_LOGD(" id = %d", src_stream->id);

	/* get the ready frame */
	mutex_lock(&venc_comp->out_buf_manager.mutex);

	if (list_empty(&venc_comp->out_buf_manager.valid_stream_list)) {
		RT_LOGE("the valid stream list is empty when return out buffer");
		mutex_unlock(&venc_comp->out_buf_manager.mutex);
		return ERROR_TYPE_ERROR;
	}
	stream_node = list_first_entry(&venc_comp->out_buf_manager.valid_stream_list, video_stream_node, mList);

	if (src_stream->id != stream_node->video_stream.id) {
		RT_LOGE("fill this out buf: buf is not match, id = %d ,%d",
			src_stream->id, stream_node->video_stream.id);
		mutex_unlock(&venc_comp->out_buf_manager.mutex);
		return -1;
	}

	memcpy(&stream_node->video_stream, src_stream, sizeof(video_stream_s));

	/* free bitstream frame to vencoder */
	config_VencOutputBuffer_by_videostream(venc_comp, &output_buffer, &stream_node->video_stream);

	LOCK_MUTEX(&venc_mutex);
	VencQueueOutputBuf(venc_comp->vencoder, &output_buffer);
	UNLOCK_MUTEX(&venc_mutex);

	list_move_tail(&stream_node->mList, &venc_comp->out_buf_manager.empty_stream_list);
	venc_comp->out_buf_manager.empty_num++;
	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	return error;
}

error_type venc_comp_set_callbacks(
	PARAM_IN comp_handle component,
	PARAM_IN comp_callback_type *callback,
	PARAM_IN void *callback_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	venc_comp->callback      = callback;
	venc_comp->callback_data = callback_data;
	return error;
}

error_type venc_comp_setup_tunnel(
	PARAM_IN comp_handle component,
	PARAM_IN comp_port_type port_type,
	PARAM_IN comp_handle tunnel_comp,
	PARAM_IN int connect_flag)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL || tunnel_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks, param error: %px, %px, %px",
			rt_component, venc_comp, tunnel_comp);
		error = ERROR_TYPE_ILLEGAL_PARAM;
		goto setup_tunnel_exit;
	}

	if (port_type == COMP_INPUT_PORT) {
		if (connect_flag == 1) {
			if (venc_comp->in_port_tunnel_info.valid_flag == 1) {
				RT_LOGE("in port tunnel had setuped !!");
				error = ERROR_TYPE_ERROR;
				goto setup_tunnel_exit;
			} else {
				venc_comp->in_port_tunnel_info.valid_flag  = 1;
				venc_comp->in_port_tunnel_info.tunnel_comp = tunnel_comp;
			}
		} else {
			if (venc_comp->in_port_tunnel_info.valid_flag == 1 && venc_comp->in_port_tunnel_info.tunnel_comp == tunnel_comp) {
				venc_comp->in_port_tunnel_info.valid_flag  = 0;
				venc_comp->in_port_tunnel_info.tunnel_comp = NULL;
			} else {
				RT_LOGE("disconnect in tunnel failed:  valid_flag = %d, tunnel_comp = %px, %px",
					venc_comp->in_port_tunnel_info.valid_flag,
					venc_comp->in_port_tunnel_info.tunnel_comp, tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
	} else if (port_type == COMP_OUTPUT_PORT) {
		if (connect_flag == 1) {
			if (venc_comp->out_port_tunnel_info.valid_flag == 1) {
				RT_LOGE("out port tunnel had setuped !!");
				error = ERROR_TYPE_ERROR;
				goto setup_tunnel_exit;
			} else {
				venc_comp->out_port_tunnel_info.valid_flag  = 1;
				venc_comp->out_port_tunnel_info.tunnel_comp = tunnel_comp;
			}
		} else {
			if (venc_comp->out_port_tunnel_info.valid_flag == 1 && venc_comp->out_port_tunnel_info.tunnel_comp == tunnel_comp) {
				venc_comp->out_port_tunnel_info.valid_flag  = 0;
				venc_comp->out_port_tunnel_info.tunnel_comp = NULL;
			} else {
				RT_LOGE("disconnect out tunnel failed:	valid_flag = %d, tunnel_comp = %px, %px",
					venc_comp->out_port_tunnel_info.valid_flag,
					venc_comp->out_port_tunnel_info.tunnel_comp, tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
	}

setup_tunnel_exit:

	return error;
}

error_type venc_comp_component_init(PARAM_IN comp_handle component, const rt_media_config_s *pmedia_config)
{
	int i				= 0;
	int ret				= 0;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = NULL;

	venc_comp = kmalloc(sizeof(struct venc_comp_ctx), GFP_KERNEL);
	if (venc_comp == NULL) {
		RT_LOGE("kmalloc for venc_comp failed");
		error = ERROR_TYPE_NOMEM;
		goto EXIT;
	}
	memset(venc_comp, 0, sizeof(struct venc_comp_ctx));

#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
	if (global_osd_info == NULL) {
		global_osd_info = kmalloc(sizeof(struct native_osd_info), GFP_KERNEL);
		if (global_osd_info == NULL) {
			RT_LOGW("kmalloc for global_osd_info failed, size = %d", sizeof(struct native_osd_info));
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		} else
			memset(global_osd_info, 0, sizeof(struct native_osd_info));

		/* force set ori size to 2560x1440*/
		global_osd_info->ori_frame_w = 2560;
		global_osd_info->ori_frame_h = 1440;
		RT_LOGI("ori_frame_w = %d, h = %d", global_osd_info->ori_frame_w, global_osd_info->ori_frame_h);
	}
#endif

	ret = message_create(&venc_comp->msg_queue);
	if (ret < 0) {
		RT_LOGE("message create failed");
		error = ERROR_TYPE_ERROR;
		goto EXIT;
	}

	rt_component->component_private    = venc_comp;
	rt_component->init		   = venc_comp_init;
	rt_component->start		   = venc_comp_start;
	rt_component->pause		   = venc_comp_pause;
	rt_component->stop		   = venc_comp_stop;
	rt_component->destroy		   = venc_comp_destroy;
	rt_component->get_config	   = venc_comp_get_config;
	rt_component->set_config	   = venc_comp_set_config;
	rt_component->get_state		   = venc_comp_get_state;
	rt_component->empty_this_in_buffer = venc_comp_empty_this_in_buffer;
	rt_component->fill_this_out_buffer = venc_comp_fill_this_out_buffer;
	rt_component->set_callbacks	= venc_comp_set_callbacks;
	rt_component->setup_tunnel	 = venc_comp_setup_tunnel;

	venc_comp->base_config.channel_id = pmedia_config->channelId;
	venc_comp->self_comp = rt_component;
	venc_comp->state     = COMP_STATE_IDLE;

	for (i = 0; i < WAIT_REPLY_NUM; i++) {
		init_waitqueue_head(&venc_comp->wait_reply[i]);
	}

	init_waitqueue_head(&venc_comp->jpeg_cxt.wait_enc_finish);

	/* init in buf manager*/
	mutex_init(&venc_comp->in_buf_manager.mutex);
	venc_comp->in_buf_manager.empty_num = VENC_IN_BUFFER_LIST_NODE_NUM;
	INIT_LIST_HEAD(&venc_comp->in_buf_manager.empty_frame_list);
	INIT_LIST_HEAD(&venc_comp->in_buf_manager.valid_frame_list);
	for (i = 0; i < VENC_IN_BUFFER_LIST_NODE_NUM; i++) {
		video_frame_node *pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		}
		memset(pNode, 0, sizeof(video_frame_node));
		list_add_tail(&pNode->mList, &venc_comp->in_buf_manager.empty_frame_list);
	}

	/* init out buf manager*/
	mutex_init(&venc_comp->out_buf_manager.mutex);
	venc_comp->out_buf_manager.empty_num = VENC_OUT_BUFFER_LIST_NODE_NUM;
	INIT_LIST_HEAD(&venc_comp->out_buf_manager.empty_stream_list);
	INIT_LIST_HEAD(&venc_comp->out_buf_manager.valid_stream_list);
	for (i = 0; i < VENC_OUT_BUFFER_LIST_NODE_NUM; i++) {
		video_stream_node *pNode = kmalloc(sizeof(video_stream_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		}
		memset(pNode, 0, sizeof(video_stream_node));
		list_add_tail(&pNode->mList, &venc_comp->out_buf_manager.empty_stream_list);
	}
	RT_LOGS("channel_id %d venc_comp %px create thread_process_venc", venc_comp->base_config.channel_id, venc_comp);
	venc_comp->venc_thread = kthread_create(thread_process_venc,
						venc_comp, "venc comp thread");
	wake_up_process(venc_comp->venc_thread);

EXIT:

	return error;
}

#if TEST_ENCODER_BY_SELF
static void loop_encode_one_frame(struct venc_comp_ctx *venc_comp)
{
	VencInputBuffer in_buf;
	video_frame_s cur_video_frame;
	video_frame_node *frame_node = NULL;
	int result		     = 0;

	if (venc_comp->is_first_frame == 0) {
		mutex_lock(&venc_comp->in_buf_manager.mutex);
		if (list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
			RT_LOGD("valid frame list is null, wait for it  start");
			venc_comp->in_buf_manager.no_frame_flag = 1;
			mutex_unlock(&venc_comp->in_buf_manager.mutex);
			TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 10);
			RT_LOGD("valid frame list is null, wait for it finish");
			return;
		}
		frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
		memcpy(&cur_video_frame, &frame_node->video_frame, sizeof(video_frame_s));
		mutex_unlock(&venc_comp->in_buf_manager.mutex);

		GetOneAllocInputBuffer(venc_comp->vencoder, &in_buf);
		memcpy(in_buf.pAddrVirY, cur_video_frame.vir_addr[0], cur_video_frame.buf_size);
		FlushCacheAllocInputBuffer(venc_comp->vencoder, &in_buf);

		VencQueueInputBuf(venc_comp->vencoder, &in_buf);
		result = encodeOneFrame(venc_comp->vencoder);

		RT_LOGW("first encode, ret = %d, buf_size = %d, vir_addr = %px, %px",
			result, cur_video_frame.buf_size,
			in_buf.pAddrVirY, cur_video_frame.vir_addr[0]);

		mutex_lock(&venc_comp->in_buf_manager.mutex);
		list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
		venc_comp->in_buf_manager.empty_num++;
		mutex_unlock(&venc_comp->in_buf_manager.mutex);

		venc_empty_in_buffer_done(venc_comp, &cur_video_frame);
		venc_comp->is_first_frame = 1;
	} else {
		GetOneAllocInputBuffer(venc_comp->vencoder, &in_buf);
		//int y_size = venc_comp->base_config.src_width*venc_comp->base_config.src_height;
		//memset(in_buf.pAddrVirY, 0x80, y_size);
		//memset(in_buf.pAddrVirC, 0x10, y_size/2);

		//FlushCacheAllocInputBuffer(venc_comp->vencoder, &in_buf);
		VencQueueInputBuf(venc_comp->vencoder, &in_buf);
		result = encodeOneFrame(venc_comp->vencoder);

		if (result != 0)
			msleep(20);
		RT_LOGD("**test copy encoder, result = %d", result);
	}
}
#endif


static int venc_comp_stream_buffer_to_valid_list(struct venc_comp_ctx *venc_comp, VencOutputBuffer *p_out_buffer)
{
	int result = 0;

	mutex_lock(&venc_comp->out_buf_manager.mutex);
	if (list_empty(&venc_comp->out_buf_manager.empty_stream_list)) {
		if (list_empty(&venc_comp->out_buf_manager.empty_stream_list))
			RT_LOGI("venc_comp %px channel_id %d empty list is null", venc_comp, venc_comp->base_config.channel_id);
		mutex_unlock(&venc_comp->out_buf_manager.mutex);
		return -1;
	}
	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	LOCK_MUTEX(&venc_mutex);
	result = VencDequeueOutputBuf(venc_comp->vencoder, p_out_buffer);
	UNLOCK_MUTEX(&venc_mutex);
	if (result != 0) {
		RT_LOGD(" have no bitstream, result = %d", result);
		return -1;
	}
	RT_LOGI(" get bitstream , result = %d, pts = %lld", result, p_out_buffer->nPts);

	if (venc_comp->vbvStartAddr == NULL) {
		VbvInfo vbv_info;

		memset(&vbv_info, 0, sizeof(VbvInfo));
		VencGetParameter(venc_comp->vencoder, VENC_IndexParamVbvInfo, &vbv_info);
		venc_comp->vbvStartAddr = vbv_info.start_addr;
	}

	venc_fill_out_buffer_done(venc_comp, p_out_buffer);
	return 0;
}

static int thread_process_venc(void *param)
{
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)param;
	message_t cmd_msg;
	int result		     = 0;
	video_frame_node *frame_node = NULL;
	VencOutputBuffer out_buffer;
	VencInputBuffer in_buf;
	//int64_t time_start  = 0;
	//int64_t time_finish = 0;
	video_frame_s cur_video_frame;
	static int cnt_01, cnt_1, cnt_11;

	memset(&cur_video_frame, 0, sizeof(video_frame_s));
	memset(&out_buffer, 0, sizeof(VencOutputBuffer));

	RT_LOGS("channel_id %d thread_process_venc start !", venc_comp->base_config.channel_id);
	while (1) {
	process_message:
		if (kthread_should_stop())
			break;

		if (get_message(&venc_comp->msg_queue, &cmd_msg) == 0) {
			commond_process(venc_comp, &cmd_msg);
			/* continue process message */
			continue;
		}

		if (venc_comp->state == COMP_STATE_EXECUTING) {
/* get the ready frame */
#if TEST_ENCODER_BY_SELF
			loop_encode_one_frame(venc_comp);
#else

		#if DEBUG_SHOW_ENCODE_TIME
			static int cnt_0 = 1;
			if (cnt_0 && venc_comp->base_config.channel_id == 0) {
			    cnt_0 = 0;
			    RT_LOGW("channel %d first time in execution state, time: %lld", venc_comp->base_config.channel_id, get_cur_time());
			}
			cnt_01 = 1;
			if (cnt_01 && venc_comp->base_config.channel_id == 1) {
			    cnt_01 = 0;
			    RT_LOGW("channel %d first time in execution state, time: %lld", venc_comp->base_config.channel_id, get_cur_time());
			}
		#endif

			if (venc_comp->base_config.codec_type == RT_VENC_CODEC_JPEG
				&& VencGetValidOutputBufNum(venc_comp->vencoder)) {
				result = venc_comp_stream_buffer_to_valid_list(venc_comp, &out_buffer);
				RT_LOGD("ValidOutputBufNum %d result %d", VencGetValidOutputBufNum(venc_comp->vencoder), result);
			}
			/* get frame from valid_list */
			if (venc_comp->base_config.bOnlineChannel == 0) {
				mutex_lock(&venc_comp->in_buf_manager.mutex);
				if (list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
					static int count;
					if (count % 100 == 0)
						RT_LOGD("valid frame list is null, wait for it  start");
					venc_comp->in_buf_manager.no_frame_flag = 1;
					mutex_unlock(&venc_comp->in_buf_manager.mutex);
					TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 10);
					if (count % 100 == 0) {
						RT_LOGD("valid frame list is null, wait for it finish");
					}
					count++;
					goto process_message;
				}
			#if DEBUG_SHOW_ENCODE_TIME
				cnt_1 = 1;
				if (cnt_1 && venc_comp->base_config.channel_id == 0) {
					cnt_1 = 0;
					RT_LOGW("channel %d first time found valid frame buffer, time: %lld",
						venc_comp->base_config.channel_id, get_cur_time());
				}
				cnt_11 = 1;
				if (cnt_11 && venc_comp->base_config.channel_id == 1) {
					cnt_11 = 0;
					RT_LOGW("channel %d first time found valid frame buffer, time: %lld",
						venc_comp->base_config.channel_id, get_cur_time());
				}
			#endif
				frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
				memcpy(&cur_video_frame, &frame_node->video_frame, sizeof(video_frame_s));

				list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
				venc_comp->in_buf_manager.empty_num++;
				mutex_unlock(&venc_comp->in_buf_manager.mutex);

				/*process frame start*/
				memset(&in_buf, 0, sizeof(VencInputBuffer));
				in_buf.nID       = cur_video_frame.id;
				in_buf.nPts      = cur_video_frame.pts;
				in_buf.nFlag     = 0;
				in_buf.pAddrPhyY = cur_video_frame.phy_addr[0];
				in_buf.pAddrPhyC = cur_video_frame.phy_addr[1];
				in_buf.pAddrVirY = (unsigned char *)cur_video_frame.vir_addr[0];
				in_buf.pAddrVirC = (unsigned char *)cur_video_frame.vir_addr[1];
				if (venc_comp->base_config.enable_crop) {
					in_buf.bEnableCorp = venc_comp->base_config.enable_crop;
					in_buf.sCropInfo.nLeft = venc_comp->base_config.s_crop_rect.nLeft;
					in_buf.sCropInfo.nTop = venc_comp->base_config.s_crop_rect.nTop;
					in_buf.sCropInfo.nWidth = venc_comp->base_config.s_crop_rect.nWidth;
					in_buf.sCropInfo.nHeight = venc_comp->base_config.s_crop_rect.nHeight;
				}
				RT_LOGI("phyY = %px, phyC = %px, %px, %px", in_buf.pAddrPhyY, in_buf.pAddrPhyC,
					in_buf.pAddrVirY, in_buf.pAddrVirC);
				if (venc_comp->jpeg_cxt.enable == 1 && venc_comp->jpeg_cxt.encoder_finish_flag == 0) {
					catch_jpeg_encoder(venc_comp, &in_buf);
				}

				venc_comp->vencCallBack.empty_in_buffer_done = venc_empty_in_buffer_done;
				VencSetCallbacks(venc_comp->vencoder, &venc_comp->vencCallBack, venc_comp, &cur_video_frame);

				result = VencQueueInputBuf(venc_comp->vencoder, &in_buf);
				if (result != 0) {
					RT_LOGE("fatal error! VencQueueInputBuf fail[%d]", result);
				}
/* process frame end */
#endif
			}
			/* get bitstream */
			while (1) {
				result = -1;
				if (VencGetValidOutputBufNum(venc_comp->vencoder))
					result = venc_comp_stream_buffer_to_valid_list(venc_comp, &out_buffer);
				if (result != 0) {
					break;
				}
			}

		} else {
			static int count;
			count++;
			if (count % 50 == 0) {
				RT_LOGD("TMessage_WaitQueueNotEmpty  !");
			}
			TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 10);
		}
	}

	RT_LOGD("thread_process_venc finish !");
	return 0;
}

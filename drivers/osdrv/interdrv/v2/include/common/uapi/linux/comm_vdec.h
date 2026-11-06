/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: comm_vdec.h
 * Description:
 */

#ifndef __COMM_VDEC_H__
#define __COMM_VDEC_H__
#include <linux/defines.h>
#include <linux/comm_video.h>
#include <linux/common.h>
#include <linux/comm_vb.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define DRV_VDEC_STR_LEN	255
#define DRV_VDEC_MASK_ERR	0x1
#define DRV_VDEC_MASK_WARN	0x2
#define DRV_VDEC_MASK_INFO	0x4
#define DRV_VDEC_MASK_FLOW	0x8
#define DRV_VDEC_MASK_DBG	0x10
#define DRV_VDEC_MASK_MEM	0x80
#define DRV_VDEC_MASK_BS	0x100
#define DRV_VDEC_MASK_SRC	0x200
#define DRV_VDEC_MASK_API	0x400
#define DRV_VDEC_MASK_DISP	0x800
#define DRV_VDEC_MASK_PERF	0x1000
#define DRV_VDEC_MASK_CFG	0x2000
#define DRV_VDEC_MASK_TRACE	0x4000
#define DRV_VDEC_MASK_DUMP_YUV	0x10000
#define DRV_VDEC_MASK_DUMP_BS	0x20000
#define DRV_VDEC_MASK_CURR	(DRV_VDEC_MASK_ERR)


#define IO_BLOCK 1
#define IO_NOBLOCK 0

typedef enum _video_mode_e {
	VIDEO_MODE_STREAM = 0, /* send by stream */
	VIDEO_MODE_FRAME, /* send by frame  */
	VIDEO_MODE_COMPAT, /* One frame supports multiple packets sending. */
	/* The current frame is considered to end when bEndOfFrame is equal to HI_TRUE */
	VIDEO_MODE_BUTT
} video_mode_e;

enum VDEC_BIND_MODE_E {
	VDEC_BIND_DISABLE  = 0,
	VDEC_BIND_VPSS_VO,
	VDEC_BIND_VPSS_VENC,
};

typedef struct _vdec_attr_video_s {
	unsigned int u32RefFrameNum; /* RW, Range: [0, 16]; reference frame num. */
	unsigned char bTemporalMvpEnable; /* RW; */
	/* specifies whether temporal motion vector predictors can be used for inter prediction */
	unsigned int u32TmvBufSize; /* RW; tmv buffer size(Byte) */
} vdec_attr_video_s;

typedef struct _buffer_info_s {
	unsigned int  size;
	unsigned long phys_addr;
	unsigned long virt_addr;
} buffer_info_s;

typedef struct _vdec_buffer_info_s {
	buffer_info_s* bitstream_buffer;
	buffer_info_s* frame_buffer;
	buffer_info_s* Ytable_buffer;
	buffer_info_s* Ctable_buffer;
	unsigned int numOfDecFbc;
	unsigned int numOfDecwtl;
} vdec_buffer_info_s;

typedef struct _vdec_chn_attr_s {
	payload_type_e enType; /* RW; video type to be decoded   */
	video_mode_e enMode; /* RW; send by stream or by frame */
	unsigned int u32PicWidth; /* RW; max pic width */
	unsigned int u32PicHeight; /* RW; max pic height */
	unsigned int u32StreamBufSize; /* RW; stream buffer size(Byte) */
	unsigned int u32FrameBufSize; /* RW; frame buffer size(Byte) */
	unsigned int u32FrameBufCnt;
	compress_mode_e enCompressMode; /* RW; compress mode */
	unsigned char u8CommandQueueDepth; /* RW; command queue depth [0,4]*/
	unsigned char u8ReorderEnable;
	union {
		vdec_attr_video_s
		stVdecVideoAttr; /* structure with video ( h264/h265) */
	};
	vdec_buffer_info_s stBufferInfo;
} vdec_chn_attr_s;

typedef struct _vdec_stream_s {
	unsigned int u32Len; /* W; stream len */
	uint64_t u64PTS; /* W; present time stamp */
	uint64_t u64DTS; /* W; decoded time stamp */
	unsigned char bEndOfFrame; /* W; is the end of a frame */
	unsigned char bEndOfStream; /* W; is the end of all stream */
	unsigned char bDisplay; /* W; is the current frame displayed. only valid by VIDEO_MODE_FRAME */
	unsigned char *pu8Addr; /* W; stream address */
} vdec_stream_s;

typedef struct _vdec_userdata_s {
	uint64_t u64PhyAddr; /* R; userdata data phy address */
	unsigned int u32Len; /* R; userdata data len */
	unsigned char bValid; /* R; is valid? */
	unsigned char *pu8Addr; /* R; userdata data vir address */
} vdec_userdata_s;

typedef struct _vdec_decode_error_s {
	int s32FormatErr; /* R; format error. eg: do not support filed */
	int s32PicSizeErrSet; /* R; picture width or height is larger than channel width or height */
	int s32StreamUnsprt; /* R; unsupport the stream specification */
	int s32PackErr; /* R; stream package error */
	int s32PrtclNumErrSet; /* R; protocol num is not enough. eg: slice, pps, sps */
	int s32RefErrSet; /* R; reference num is not enough */
	int s32PicBufSizeErrSet; /* R; the buffer size of picture is not enough */
	int s32StreamSizeOver; /* R; the stream size is too big and force discard stream */
	int s32VdecStreamNotRelease; /* R; the stream not released for too long time */
} vdec_decode_error_s;

typedef struct {
	unsigned int left;   /**< A horizontal pixel offset of top-left corner of rectangle from (0, 0) */
	unsigned int top;    /**< A vertical pixel offset of top-left corner of rectangle from (0, 0) */
	unsigned int right;  /**< A horizontal pixel offset of bottom-right corner of rectangle from (0, 0) */
	unsigned int bottom; /**< A vertical pixel offset of bottom-right corner of rectangle from (0, 0) */
} vdec_rect_s;

typedef struct _seq_initial_info_s {
	int s32PicWidth; /*R; Horizontal picture size in pixel*/
	int s32PicHeight; /*R; Vertical picture size in pixel*/
	int s32FRateNumerator; /*R; The numerator part of frame rate fraction*/
	int s32FRateDenominator; /*R; The denominator part of frame rate fraction*/
	vdec_rect_s  stPicCropRect; /*R; Picture cropping rectangle information*/
	int s32MinFrameBufferCount; /*R; This is the minimum number of frame buffers required for decoding.*/
	int s32FrameBufDelay; /*R; the maximum display frame buffer delay for buffering decoded picture reorder.*/
	int s32Profile;/*R; H.265/H.264 : profile_idc*/
	int s32Level;/*R;H.265/H.264 : level_idc*/
	int s32Interlace; /*R; progressive or interlace frame */
	int s32MaxNumRefFrmFlag;/*R; one of the SPS syntax elements in H.264.*/
	int s32MaxNumRefFrm;
	/* When avcIsExtSAR is 0, this indicates aspect_ratio_idc[7:0]. When avcIsExtSAR is 1, this indicates sar_width[31:16] and sar_height[15:0].
		If aspect_ratio_info_present_flag = 0, the register returns -1 (0xffffffff).*/
	int s32AspectRateInfo;/**/
	int s32BitRate; /*R; The bitrate value written in bitstream syntax. If there is no bitRate, this reports -1. */
	int s32LumaBitdepth; /*R; bit-depth of luma sample */
	int s32ChromaBitdepth; /*R; bit-depth of chroma sample */
	unsigned char  u8CoreIdx; /*R, 0:ve_core, 1:vd_core0, 2:vd_core1*/
}seq_initial_info_s;

typedef struct _vdec_chn_status_s {
	payload_type_e enType; /* R; video type to be decoded */
	int u32LeftStreamBytes; /* R; left stream bytes waiting for decode */
	int u32LeftStreamFrames; /* R; left frames waiting for decode,only valid for VIDEO_MODE_FRAME */
	int u32LeftPics; /* R; pics waiting for output */
	int u32EmptyStreamBufSzie;
	unsigned char bStartRecvStream; /* R; had started recv stream? */
	unsigned int u32RecvStreamFrames; /* R; how many frames of stream has been received. valid when send by frame. */
	unsigned int u32DecodeStreamFrames; /* R; how many frames of stream has been decoded. valid when send by frame. */
	vdec_decode_error_s stVdecDecErr; /* R; information about decode error */
	unsigned int u32Width; /* R; the width of the currently decoded stream */
	unsigned int u32Height; /* R; the height of the currently decoded stream */
	unsigned char u8FreeSrcBuffer;
	unsigned char u8BusySrcBuffer;
	unsigned char u8Status;
	seq_initial_info_s stSeqinitalInfo;
} vdec_chn_status_s;

typedef enum _video_dec_mode_e {
	VIDEO_DEC_MODE_IPB = 0,
	VIDEO_DEC_MODE_IP,
	VIDEO_DEC_MODE_I,
	VIDEO_DEC_MODE_BUTT
} video_dec_mode_e;

typedef enum _video_output_order_e {
	VIDEO_OUTPUT_ORDER_DISP = 0,
	VIDEO_OUTPUT_ORDER_DEC,
	VIDEO_OUTPUT_ORDER_BUTT
} video_output_order_e;

typedef struct _vdec_param_video_s {
	int s32ErrThreshold; /* RW, Range: [0, 100]; */
	/* threshold for stream error process, 0: discard with any error, 100 : keep data with any error */
	video_dec_mode_e enDecMode; /* RW; */
	/* decode mode , 0: deocde IPB frames, 1: only decode I frame & P frame , 2: only decode I frame */
	video_output_order_e enOutputOrder; /* RW; */
	/* frames output order ,0: the same with display order , 1: the same width decoder order */
	compress_mode_e enCompressMode; /* RW; compress mode */
	video_format_e enVideoFormat; /* RW; video format */
} vdec_param_video_s;

typedef struct _vdec_param_picture_s {
	unsigned int u32Alpha; /* RW, Range: [0, 255]; value 0 is transparent. */
	/* [0 ,127]   is deemed to transparent when enPixelFormat is ARGB1555 or
	 * ABGR1555 [128 ,256] is deemed to non-transparent when enPixelFormat is
	 * ARGB1555 or ABGR1555
	 */
	unsigned int u32HDownSampling;
	unsigned int u32VDownSampling;
	int s32ROIEnable;
	int s32ROIOffsetX;
	int s32ROIOffsetY;
	int s32ROIOffset;
	int s32ROIWidth;
	int s32ROIHeight;
	int s32RotAngle;
	int s32MirDir;
    int s32MaxFrameWidth;
    int s32MaxFrameHeight;
    int s32MinFrameWidth;
    int s32MinFrameHeight;
} vdec_param_picture_s;

typedef struct _vdec_chn_param_s {
	payload_type_e enType; /* RW; video type to be decoded   */
	pixel_format_e enPixelFormat; /* RW; out put pixel format */
	unsigned int u32DisplayFrameNum; /* RW, Range: [0, 16]; display frame num */
	union {
		vdec_param_video_s
		stVdecVideoParam; /* structure with video ( h265/h264) */
		vdec_param_picture_s
		stVdecPictureParam; /* structure with picture (jpeg/mjpeg ) */
	};
} vdec_chn_param_s;

typedef struct _h264_prtcl_param_s {
	int s32MaxSliceNum; /* RW; max slice num support */
	int s32MaxSpsNum; /* RW; max sps num support */
	int s32MaxPpsNum; /* RW; max pps num support */
} h264_prtcl_param_s;

typedef struct _h265_prtcl_param_s {
	int s32MaxSliceSegmentNum; /* RW; max slice segmnet num support */
	int s32MaxVpsNum; /* RW; max vps num support */
	int s32MaxSpsNum; /* RW; max sps num support */
	int s32MaxPpsNum; /* RW; max pps num support */
} h265_prtcl_param_s;

typedef struct _vdec_prtcl_param_s {
	payload_type_e
	enType; /* RW; video type to be decoded, only h264 and h265 supported */
	union {
		h264_prtcl_param_s
		stH264PrtclParam; /* protocol param structure for h264 */
		h265_prtcl_param_s
		stH265PrtclParam; /* protocol param structure for h265 */
	};
} vdec_prtcl_param_s;

typedef struct _vdec_chn_pool_s {
	vb_pool hPicVbPool; /* RW;  vb pool id for pic buffer */
	vb_pool hTmvVbPool; /* RW;  vb pool id for tmv buffer */
} vdec_chn_pool_s;

typedef enum _vdec_evnt_e {
	VDEC_EVNT_STREAM_ERR = 1,
	VDEC_EVNT_UNSUPPORT,
	VDEC_EVNT_OVER_REFTHR,
	VDEC_EVNT_REF_NUM_OVER,
	VDEC_EVNT_SLICE_NUM_OVER,
	VDEC_EVNT_SPS_NUM_OVER,
	VDEC_EVNT_PPS_NUM_OVER,
	VDEC_EVNT_PICBUF_SIZE_ERR,
	VDEC_EVNT_SIZE_OVER,
	VDEC_EVNT_IMG_SIZE_CHANGE,
	VDEC_EVNT_VPS_NUM_OVER,
	VDEC_EVNT_BUTT
} vdec_evnt_e;

typedef enum _vdec_capacity_strategy_e {
	VDEC_CAPACITY_STRATEGY_BY_MOD = 0,
	VDEC_CAPACITY_STRATEGY_BY_CHN = 1,
	VDEC_CAPACITY_STRATEGY_BUTT
} vdec_capacity_strategy_e;

typedef struct _vdec_video_mod_param_s {
	unsigned int u32MaxPicWidth;
	unsigned int u32MaxPicHeight;
	unsigned int u32MaxSliceNum;
	unsigned int u32VdhMsgNum;
	unsigned int u32VdhBinSize;
	unsigned int u32VdhExtMemLevel;
} vdec_video_mod_param_s;

typedef struct _vdec_picture_mod_param_s {
	unsigned int u32MaxPicWidth;
	unsigned int u32MaxPicHeight;
	unsigned char bSupportProgressive;
	unsigned char bDynamicAllocate;
	vdec_capacity_strategy_e enCapStrategy;
} vdec_picture_mod_param_s;

typedef struct _vdec_mod_param_s {
	vb_source_e enVdecVBSource; /* RW, Range: [1, 3];  frame buffer mode  */
	unsigned int u32MiniBufMode; /* RW, Range: [0, 1];  stream buffer mode */
	unsigned int u32ParallelMode; /* RW, Range: [0, 1];  VDH working mode   */
	vdec_video_mod_param_s stVideoModParam;
	vdec_picture_mod_param_s stPictureModParam;
} vdec_mod_param_s;

typedef struct _vdec_user_data_attr_s {
	unsigned char bEnable;
	unsigned int u32MaxUserDataLen;
} vdec_user_data_attr_s;

// TODO: refinememt for hardcode
#define ENUM_ERR_VDEC_INVALID_CHNID \
	((int)(0xC0000000L | ((ID_VDEC) << 16) | ((4) << 13) | (2)))
#define ENUM_ERR_VDEC_ERR_INIT \
	((int)(0xC0000000L | ((ID_VDEC) << 16) | ((4) << 13) | (64)))

typedef enum {
	DRV_ERR_VDEC_INVALID_CHNID = ENUM_ERR_VDEC_INVALID_CHNID,
	DRV_ERR_VDEC_ILLEGAL_PARAM,
	DRV_ERR_VDEC_EXIST,
	DRV_ERR_VDEC_UNEXIST,
	DRV_ERR_VDEC_NULL_PTR,
	DRV_ERR_VDEC_NOT_CONFIG,
	DRV_ERR_VDEC_NOT_SUPPORT,
	DRV_ERR_VDEC_NOT_PERM,
	DRV_ERR_VDEC_INVALID_PIPEID,
	DRV_ERR_VDEC_INVALID_GRPID,
	DRV_ERR_VDEC_NOMEM,
	DRV_ERR_VDEC_NOBUF,
	DRV_ERR_VDEC_BUF_EMPTY,
	DRV_ERR_VDEC_BUF_FULL,
	DRV_ERR_VDEC_SYS_NOTREADY,
	DRV_ERR_VDEC_BADADDR,
	DRV_ERR_VDEC_BUSY,
	DRV_ERR_VDEC_SIZE_NOT_ENOUGH,
	DRV_ERR_VDEC_INVALID_VB,
    ///========//
	DRV_ERR_VDEC_ERR_INIT = ENUM_ERR_VDEC_ERR_INIT,
	DRV_ERR_VDEC_ERR_INVALID_RET,
	DRV_ERR_VDEC_ERR_SEQ_OPER,
	DRV_ERR_VDEC_ERR_VDEC_MUTEX,
	DRV_ERR_VDEC_ERR_SEND_FAILED,
	DRV_ERR_VDEC_ERR_GET_FAILED,
	DRV_ERR_VDEC_BUTT
} vdec_recode_e_errtype;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef  __COMM_VDEC_H__ */

/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/** @file
 * Copyright (C) 2022 Allwinner, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _UAPI_RT_MEDIA_H_
#define _UAPI_RT_MEDIA_H_

//#include <stdio.h>
//#include <sys/types.h>

#define SENSOR_0_SIGN 0xAA66AA66
#define SENSOR_1_SIGN 0xBB66BB66
#define SENSOR_2_SIGN 0xCC66CC66

#if 1//config 0
#define CSI_SENSOR_0_VIPP_0 0
#define CSI_SENSOR_0_VIPP_1 4
#define CSI_SENSOR_0_VIPP_2 8
#define CSI_SENSOR_0_VIPP_3 12

#define CSI_SENSOR_1_VIPP_0 1
#define CSI_SENSOR_1_VIPP_1 5
#define CSI_SENSOR_1_VIPP_2 9
#define CSI_SENSOR_1_VIPP_3 13

#define CSI_SENSOR_2_VIPP_0 2
#define CSI_SENSOR_2_VIPP_1 6
#define CSI_SENSOR_2_VIPP_2 10
#define CSI_SENSOR_2_VIPP_3 14

#else//config 1
#define CSI_SENSOR_0_VIPP_0 0
#define CSI_SENSOR_0_VIPP_1 4
#define CSI_SENSOR_0_VIPP_2 8
#define CSI_SENSOR_0_VIPP_3 12

#define CSI_SENSOR_1_VIPP_0 2
#define CSI_SENSOR_1_VIPP_1 6
#define CSI_SENSOR_1_VIPP_2 10
#define CSI_SENSOR_1_VIPP_3 14

#define CSI_SENSOR_2_VIPP_0 1
#define CSI_SENSOR_2_VIPP_1 5
#define CSI_SENSOR_2_VIPP_2 9
#define CSI_SENSOR_2_VIPP_3 13
#endif

#define LBC_2X_COM_RATIO_EVEN 600
#define LBC_2X_COM_RATIO_ODD 450

#define LBC_2_5X_COM_RATIO_EVEN 440
#define LBC_2_5X_COM_RATIO_ODD 380

#define LBC_BIT_DEPTH 8

#define LBC_EXT_SIZE (2 * 1024)

#define NEI_TEST_CODE 0

#define RT_AE_FACE_MAX_NUM 8
#define RT_AE_FACE_WIN_WEIGHT_LENGTH 16
#define RT_AE_FACE_POS_WEIGHT_LENGTH 64

enum RT_MEDIA_IOCTL_CMD {
	IOCTL_RT_UNKOWN = 0x100,

	IOCTL_CONFIG,  //+1
	IOCTL_START,   //+2
	IOCTL_PAUSE,   //+3
	IOCTL_DESTROY, //+4

	IOCTL_GET_VBV_BUFFER_INFO, //+5

	IOCTL_GET_STREAM_HEADER_SIZE,	 //+6
	IOCTL_GET_STREAM_HEADER_DATA,	 //+7

	IOCTL_GET_STREAM_DATA,	   //+8
	IOCTL_RETURN_STREAM_DATA,  //+9

	IOCTL_CATCH_JPEG_START,
	IOCTL_CATCH_JPEG_GET_DATA,
	IOCTL_CATCH_JPEG_STOP,

	IOCTL_SET_OSD,

	//IOCTL_GET_BIN_IMAGE_DATA,
	//IOCTL_GET_MV_INFO_DATA,

	IOCTL_CONFIG_YUV_BUF_INFO,
	IOCTL_REQUEST_YUV_FRAME,
	IOCTL_RETURN_YUV_FRAME,

	IOCTL_GET_ISP_LV,
	IOCTL_GET_ISP_HIST,
	IOCTL_GET_ISP_EXP_GAIN, //+20
	IOCTL_GET_ISP_AE_METERING_MODE,
	IOCTL_SET_IR_PARAM,
	IOCTL_SET_VERTICAL_FLIP,
	IOCTL_SET_HORIZONTAL_FLIP,
	IOCTL_SET_ISP_ATTR_CFG,
	IOCTL_GET_ISP_ATTR_CFG,
	IOCTL_SET_ISP_ORL,

	IOCTL_SET_ISP_AE_MODE,

	IOCTL_SET_FORCE_KEYFRAME,
	IOCTL_RESET_ENCODER_TYPE,

	IOCTL_REQUEST_ENC_YUV_FRAME, //+30
	IOCTL_SUBMIT_ENC_YUV_FRAME,

	IOCTL_SET_POWER_LINE_FREQ,
	IOCTL_SET_QP_RANGE,
	IOCTL_SET_BITRATE,
	IOCTL_SET_VBR_PARAM,
	IOCTL_GET_SUM_MB_INFO,

	IOCTL_SET_ENC_MOTION_SEARCH_PARAM,
	IOCTL_GET_ENC_MOTION_SEARCH_RESULT,
	IOCTL_SET_VENC_SUPER_FRAME_PARAM,
	IOCTL_SET_FPS, //+40
	IOCTL_SET_SHARP,
	IOCTL_GET_CSI_STATUS,
	IOCTL_SET_ROI,
	IOCTL_SET_GDC,
	IOCTL_SET_ROTATE,
	IOCTL_SET_REC_REF_LBC_MODE,
	IOCTL_SET_WEAK_TEXT_TH,
	IOCTL_SET_REGION_D3D_PARAM,
	IOCTL_GET_REGION_D3D_RESULT, //+50
	IOCTL_SET_CHROMA_QP_OFFSET,
	IOCTL_SET_H264_CONSTRAINT_FLAG,
	IOCTL_SET_VE2ISP_D2D_LIMIT,
	IOCTL_SET_EN_SMALL_SEARCH_RANGE,
	IOCTL_SET_FORCE_CONF_WIN,
	IOCTL_SET_ROT_VE2ISP,
	IOCTL_SET_INSERT_DATA,
	IOCTL_GET_INSERT_DATA_BUF_STATUS,
	IOCTL_SET_GRAY,
	IOCTL_SET_WB_YUV, //+60
	IOCTL_GET_WB_YUV,
	IOCTL_SET_2DNR,
	IOCTL_SET_3DNR,
	IOCTL_SET_CYCLIC_INTRA_REFRESH,
	IOCTL_SET_P_FRAME_INTRA,
	IOCTL_RESET_IN_OUT_BUFFER,
	IOCTL_DROP_FRAME, ///< notify rt_media to drop <n> video frames
	IOCTL_SET_CAMERA_MOVE_STATUS,
	IOCTL_SET_CROP,
	IOCTL_SET_VENC_TARTGET_BITS_CLIP_PARAM,
	IOCTL_SET_JPEG_QUALITY,
	IOCTL_SET_FIX_QP,
	IOCTL_SET_MB_RC_MOVE_STATUS,
	IOCTL_SET_H264_VIDEO_TIMING,
	IOCTL_SET_H265_VIDEO_TIMING,
	IOCTL_GET_TDM_BUF,
	IOCTL_RETURN_TDM_BUF,
	IOCTL_GET_TDM_DATA,
	IOCTL_SET_BRIGHTNESS,
	IOCTL_SET_CONTRAST,
	IOCTL_SET_SATURATION,
	IOCTL_SET_HUE,
	IOCTL_SET_SHARPNESS,
	IOCTL_SET_SENSOR_EXP,
	IOCTL_SET_SENSOR_GAIN,
	IOCTL_GET_SENSOR_NAME,
	IOCTL_GET_SENSOR_RESOLUTION,
	IOCTL_GET_TUNNING_CTL_DATA,
	IOCTL_SET_ENC_AND_DEC_CASE,
	IOCTL_GET_SENSOR_RESERVE_ADDR,
	IOCTL_GET_ISP_REG,
	IOCTL_SET_TDM_DROP_FRAME,
	IOCTL_SET_INPUT_BIT_WIDTH_START,
	IOCTL_SET_INPUT_BIT_WIDTH_STOP,
};

typedef struct KERNEL_VBV_BUFFER_INFO{
	int share_fd;
	int size;
} KERNEL_VBV_BUFFER_INFO;

typedef enum {
	RT_VENC_CODEC_H264 = 0,
	RT_VENC_CODEC_JPEG = 1,
	RT_VENC_CODEC_H265 = 2,
} RT_VENC_CODEC_TYPE;

// dynamic config
typedef struct video_stream_s{
	int id;
	uint64_t pts;
	unsigned int flag;
	unsigned int size0;
	unsigned int size1;
	unsigned int size2;

	unsigned int offset0;
	unsigned int offset1;
	unsigned int offset2;

	int keyframe_flag;
	struct timeval tv;

	unsigned char *data0;
	unsigned char *data1;
	unsigned char *data2;
} video_stream_s;
typedef video_stream_s STREAM_DATA_INFO;

/* overlay related */
#define MAX_OVERLAY_ITEM_SIZE  (64)

typedef enum OVERLAY_ARGB_TYPE {
	OVERLAY_ARGB_MIN   = -1,
	OVERLAY_ARGB8888   = 0,
	OVERLAY_ARGB4444   = 1,
	OVERLAY_ARGB1555   = 2,
	OVERLAY_ARGB_MAX   = 3,
} OVERLAY_ARGB_TYPE; ///< same to VENC_OVERLAY_ARGB_TYPE

typedef enum RT_VENC_OVERLAY_TYPE {
	RT_NORMAL_OVERLAY		   = 0,    //normal overlay
	RT_COVER_OSD		   = 1,    //use the setting yuv to cover region
	RT_LUMA_REVERSE_OVERLAY    = 2,    //normal overlay and luma reverse
} RT_VENC_OVERLAY_TYPE; ///< same to VENC_OVERLAY_TYPE

typedef struct VencCoverYuvS {
	 unsigned char		 use_cover_yuv_flag; //1:use the cover yuv; 0:transform the argb data to yuv ,then cover
	 unsigned char		 cover_y; //the value of cover y
	 unsigned char		 cover_u; //the value of cover u
	 unsigned char		 cover_v; //the value of cover v
} VencCoverYuvS; ///< same to VencOverlayCoverYuvS

typedef struct OverlayItemInfo_t {
	unsigned short		start_x;
	unsigned short		start_y;
	unsigned int		widht;	// must 16_align: 'width%16 == 0'
	unsigned int		height; // must 16_align: 'height%16 == 0'
	VencCoverYuvS		cover_yuv;		   //when use RT_COVER_OSD should set the cover yuv
	RT_VENC_OVERLAY_TYPE	   osd_type;	   //reference definition of VENC_OVERLAY_TYPE
	unsigned char       *data_buf;	//the vir addr of overlay block
	unsigned int		data_size;	//the size of bitmap
} OverlayItemInfo;

typedef struct VideoInputOSD_t {
	unsigned char		   osd_num; //num of overlay region
	OVERLAY_ARGB_TYPE	   argb_type;//reference definition of VENC_OVERLAY_ARGB_TYPE
	OverlayItemInfo 	   item_info[MAX_OVERLAY_ITEM_SIZE];
	//* for v5v200 and newer ic
    unsigned int                invert_mode;
    unsigned int                invert_threshold;
    //* end
} VideoInputOSD;

typedef struct catch_jpeg_config {
	int 		   channel_id;
	int 		   width;
	int 		   height;
	int 		   qp;
	int 		   rotate_angle;
	VideoInputOSD  overlay_info;
} catch_jpeg_config;

typedef struct catch_jpeg_buf_info {
	unsigned int   buf_size; // set the max_size, return valid_size
	unsigned char *buf;
} catch_jpeg_buf_info;

//typedef struct VideoGetBinImageBufInfo {
//	unsigned int   max_size;
//	unsigned char  *buf;
//} VideoGetBinImageBufInfo;

//typedef struct VideoGetMvInfoBufInfo {
//	unsigned int   max_size;
//	unsigned char  *buf;
//} VideoGetMvInfoBufInfo;

#if defined(CONFIG_FRAMEDONE_TWO_BUFFER)
#define CONFIG_YUV_BUF_NUM (2)
#else
#define CONFIG_YUV_BUF_NUM (3)
#endif

typedef struct config_yuv_buf_info {
	unsigned char *phy_addr[CONFIG_YUV_BUF_NUM];
	int buf_size;
	int buf_num;
} config_yuv_buf_info;

typedef struct rt_yuv_info {
	int bset;
	int fd;
	unsigned char *phy_addr;
	int width;
	int height;
} rt_yuv_info;


#define VIDEO_INPUT_CHANNEL_NUM (16)

typedef enum {
	AW_Video_H264ProfileBaseline  = 66, 		/**< Baseline profile */
	AW_Video_H264ProfileMain	  = 77, 		/**< Main profile */
	AW_Video_H264ProfileHigh	  = 100,		/**< High profile */
} AW_Video_H264PROFILETYPE; ///< same to #VENC_H264PROFILETYPE

/**
 * H264 level types
 */
typedef enum {
	AW_Video_H264Level1   = 10, 	/**< Level 1 */
	AW_Video_H264Level11  = 11, 	/**< Level 1.1 */
	AW_Video_H264Level12  = 12, 	/**< Level 1.2 */
	AW_Video_H264Level13  = 13, 	/**< Level 1.3 */
	AW_Video_H264Level2   = 20, 	/**< Level 2 */
	AW_Video_H264Level21  = 21, 	/**< Level 2.1 */
	AW_Video_H264Level22  = 22, 	/**< Level 2.2 */
	AW_Video_H264Level3   = 30, 	/**< Level 3 */
	AW_Video_H264Level31  = 31, 	/**< Level 3.1 */
	AW_Video_H264Level32  = 32, 	/**< Level 3.2 */
	AW_Video_H264Level4   = 40, 	/**< Level 4 */
	AW_Video_H264Level41  = 41, 	/**< Level 4.1 */
	AW_Video_H264Level42  = 42, 	/**< Level 4.2 */
	AW_Video_H264Level5   = 50, 	/**< Level 5 */
	AW_Video_H264Level51  = 51, 	/**< Level 5.1 */
	AW_Video_H264Level52  = 52,     /**< Level 5.2 */
	AW_Video_H264LevelDefault = 0
} AW_Video_H264LEVELTYPE; ///< same to #VENC_H264LEVELTYPE

typedef enum {
	AW_Video_H265ProfileMain		= 1,
	AW_Video_H265ProfileMain10		= 2,
	AW_Video_H265ProfileMainStill	= 3
} AW_VideoH265PROFILETYPE; ///< same to #VENC_H265PROFILETYPE

typedef enum {
	AW_Video_H265Level1   = 30, 	/**< Level 1 */
	AW_Video_H265Level2   = 60, 	/**< Level 2 */
	AW_Video_H265Level21  = 63, 	/**< Level 2.1 */
	AW_Video_H265Level3   = 90, 	/**< Level 3 */
	AW_Video_H265Level31  = 93, 	/**< Level 3.1 */
	AW_Video_H265Level41  = 123,	/**< Level 4.1 */
	AW_Video_H265Level5   = 150,	/**< Level 5 */
	AW_Video_H265Level51  = 153,	/**< Level 5.1 */
	AW_Video_H265Level52  = 156,	/**< Level 5.2 */
	AW_Video_H265Level6   = 180,	/**< Level 6 */
	AW_Video_H265Level61  = 183,	/**< Level 6.1 */
	AW_Video_H265Level62  = 186, 	/**< Level 6.2 */
	AW_Video_H265LevelDefault = 0
} AW_Video_H265LEVELTYPE; ///< same to #VENC_H265LEVELTYPE

typedef struct video_qp_range {
	int i_min_qp;//[1 ~ 51]
	int i_max_qp;//[1 ~ 51]
	int p_min_qp;//[1 ~ 51]
	int p_max_qp;//[1 ~ 51]
	int i_init_qp;
	int enable_mb_qp_limit;
} video_qp_range;

typedef struct RTVencFixQP {
	int bEnable;
	int nIQp;
	int nPQp;
} RTVencFixQP;

typedef enum VIDEO_OUTPUT_MODE{
	OUTPUT_MODE_STREAM = 0,
	OUTPUT_MODE_YUV    = 1,
	OUTPUT_MODE_ENCODE_FILE_YUV	= 2,
	OUTPUT_MODE_ENCODE_OUTSIDE_YUV = 3,
} VIDEO_OUTPUT_MODE;

typedef enum rt_pixelformat_type {
	RT_PIXEL_YUV420SP, //nv12
	RT_PIXEL_YVU420SP, //nv21
	RT_PIXEL_YUV420P,
	RT_PIXEL_YVU420P,
	RT_PIXEL_YUV422SP,
	RT_PIXEL_YVU422SP,
	RT_PIXEL_YUV422P,
	RT_PIXEL_YVU422P,
	RT_PIXEL_YUYV422,
	RT_PIXEL_UYVY422,
	RT_PIXEL_YVYU422,
	RT_PIXEL_VYUY422,
	RT_PIXEL_LBC_25X,
	RT_PIXEL_LBC_2X,
	RT_PIXEL_NUM
} rt_pixelformat_type;
typedef rt_pixelformat_type RT_PIXELFORMAT_TYPE;

typedef enum rt_outputformat_type {
    RT_OUTPUT_SAME_AS_INPUT,
    RT_OUTPUT_420,
    RT_OUTPUT_444,
    RT_OUTPUT_422,
    RT_OUPUT_NUM
} rt_outputformat_type;
typedef rt_outputformat_type RT_OUTPUTFORMAT_TYPE;

typedef struct {
	int en_motion_search;
	int dis_default_para;
	int hor_region_num;
	int ver_region_num;
	int large_mv_th;
	float large_mv_ratio_th;	// include intra and large mv
	float non_zero_mv_ratio_th; // include intra, large mv and samll mv
	float large_sad_ratio_th;
} RTVencMotionSearchParam; ///< same to VencMotionSearchParam
typedef struct RTVencBitRateRange {
	int bitRateMax; //unit: kbps
	int bitRateMin; //unit: kbps
} RTVencBitRateRange;
typedef enum RT_VENC_VIDEO_FORMAT {
	RT_COMPONENT = 0,
	RT_PAL		 = 1,
	RT_NTSC 	 = 2,
	RT_SECAM	 = 3,
	RT_MAC		 = 4,
	RT_DEFAULT	 = 5,
} RT_VENC_VIDEO_FORMAT; /**< same to VENC_VIDEO_FORMAT [vencoder.h] */
typedef enum RT_VENC_COLOR_SPACE {
	RT_RESERVED0	   = 0,
	RT_VENC_BT709	   = 1, // include BT709-5, BT1361, IEC61966-2-1 IEC61966-2-4
	RT_RESERVED1	   = 2,
	RT_RESERVED2	   = 3,
	RT_VENC_BT470	   = 4, // include BT470-6_SystemM,
	RT_VENC_BT601	   = 5, // include BT470-6_SystemB, BT601-6_625, BT1358_625, BT1700_625
	RT_VENC_SMPTE_170M = 6, // include SMPTE_170M, BT601-6_525, BT1358_525, BT1700_NTSC
	RT_VENC_SMPTE_240M = 7, // include SMPTE_240M
	RT_VENC_YCC    = 8, // include GENERIC_FILM
	RT_VENC_BT2020	   = 9, // include BT2020
	RT_VENC_ST_428	   = 10, // include SMPTE_428-1
} RT_VENC_COLOR_SPACE; ///< same to VENC_COLOR_SPACE [vencoder.h]
typedef struct RTVencH264VideoSignal {
	RT_VENC_VIDEO_FORMAT video_format;

	unsigned char full_range_flag;

	RT_VENC_COLOR_SPACE src_colour_primaries;
	RT_VENC_COLOR_SPACE dst_colour_primaries;
} RTVencVideoSignal; ///< same to VencJpegVideoSignal [vencoder.h]
typedef struct RTVencRect {
	int nLeft;
	int nTop;
	int nWidth;
	int nHeight;
} RTVencRect; ///< same to VencRect
typedef struct RTVencROIConfig {
	int 					bEnable;
	int 					index;
	int 					nQPoffset;
	unsigned char			roi_abs_flag;
	RTVencRect				  sRect;
} RTVencROIConfig; ///< same to VencROIConfig

typedef enum {
    RTVENC_ISP_SCALER_0       = 0, //not scaler
    RTVENC_ISP_SCALER_EIGHTH  = 1, //scaler 1/8 write back
    RTVENC_ISP_SCALER_HALF    = 2, //scaler 1/2 write back
    RTVENC_ISP_SCALER_QUARTER = 3, //scaler 1/4 write back
} RTeVencEncppScalerRatio;
typedef struct {
    unsigned int bEnableWbYuv;
    unsigned int nWbBufferNum;
    unsigned int bEnableCrop;
    RTVencRect     sWbYuvcrop;
    RTeVencEncppScalerRatio scalerRatio;
} RTsWbYuvParam;

typedef struct {
    unsigned int        nThumbSize;
    unsigned char       *pThumbBuf;
    unsigned int        bWriteToFile;
    unsigned int        nEncodeCnt;
} RTVencThumbInfo;

typedef struct {
    int            enable_crop;
    RTVencRect     s_crop_rect;
} RTCropInfo;

typedef enum {
    RTVENC_PRODUCT_STATIC_IPC = 0,
    RTVENC_PRODUCT_MOVING_IPC = 1,
    RTVENC_PRODUCT_DOORBELL = 2,
    RTVENC_PRODUCT_CDR = 3,
    RTVENC_PRODUCT_SDV = 4,
    RTVENC_PRODUCT_PROJECTION = 5,
    RTVENC_PRODUCT_UAV = 6,       // Unmanned Aerial Vehicle
    RTVENC_PRODUCT_NUM,
} RTeVencProductMode; ///< same to eVencProductMode

#define MAX_ROI_AREA 8

typedef struct {
	unsigned int			uMaxBitRate; //* kbps
	unsigned int			nMovingTh;		//range[1,31], 1:all frames are moving,
											//			  31:have no moving frame, default: 20
	int 					nQuality;		//range[1,50], 1:worst quality, 10: common quality, 50:best quality
	int 					nIFrmBitsCoef;	 //range[1, 20], 1:worst quality, 20:best quality
	int 					nPFrmBitsCoef;	 //range[1, 50], 1:worst quality, 50:best quality
} RTVencVbrParam;

typedef struct rt_media_config_s {
	int channelId;				// channel ID. equivalent to vipp number, scope[0,15].
	int encodeType; 			//0)h264 1)mjpeg 2)h265
	int width;	// resolution width
	int height;// resolution height
	int dst_width; ///< operate venc. encode output size.
	int dst_height;
	int fps;					///< operate isp. fps:1~30
	int dst_fps;					///< operate isp. fps:1~30
	int bitrate;				///< operate venc. h264 bitrate (kbps)
	int gop;					///< operate venc. h264
	int vbr;					///< operate venc. VBR=1, CBR=0
	RTVencVbrParam vbr_param;
	int profile; //AW_Video_H264PROFILETYPE or AW_VideoH265PROFILETYPE
	int level; //AW_Video_H264LEVELTYPE or AW_Video_H265LEVELTYPE
	int drop_frame_num;
	video_qp_range qp_range;
	VIDEO_OUTPUT_MODE output_mode;

	//int enable_bin_image;	// 0: disable, 1: enable
	//int bin_image_moving_th;//range[1,31], 1:all frames are moving,
							//			  31:have no moving frame, default: 20

	//int enable_mv_info; 	// 0: disable, 1: enable

	//* about isp
	int enable_wdr; ///< config isp: set wdr, [0,1]

	RT_PIXELFORMAT_TYPE pixelformat;//* just set it to: RT_PIXELFORMAT_UNKNOWN
	RT_OUTPUTFORMAT_TYPE outputformat;
	//RTVencMotionSearchParam motion_search_param;// should setup the param if use motion search by encoder
	int enable_overlay;
	int jpg_quality;
	int bonline_channel;
	int share_buf_num;
	int jpg_mode;//0: jpg 1: mjpg
	RTVencBitRateRange bit_rate_range;
	RTVencVideoSignal venc_video_signal;
	RTeVencProductMode product_mode;
	//RTVencROIConfig   sRoiConfig[MAX_ROI_AREA];

	int demo_start;
	int enable_sharp; ///< operate venc. only config venc to enable sharp.
	int en_16_align_fill_data;
	int vin_buf_num;
	int breduce_refrecmem;//Can reduce memory usage and optimize memory efficiency. default set enable
	RTCropInfo	   s_crop_info;
	RTsWbYuvParam  s_wbyuv_param;
	int vbv_buf_size;			//all bitstream buf size.
	int vbv_thresh_size;		//one bitstream max size.

	int enable_ve_isp_linkage;
	int skip_sharp_param_frame_num;
	int ve_encpp_sharp_atten_coef_per;      //decrease encpp sharpness percent [0, 100]. 100 means no decrease

	int enable_aiisp;
	unsigned int tdm_rxbuf_cnt;
} rt_media_config_s;
typedef rt_media_config_s VideoInputConfig;

typedef struct {
	int pix_x_bgn;
	int pix_x_end;
	int pix_y_bgn;
	int pix_y_end;
	int total_num;
	int intra_num;
	int large_mv_num;
	int small_mv_num;
	int zero_mv_num;
	int large_sad_num;
	int is_motion;
} RTVencMotionSearchRegion; ///< same to VencMotionSearchRegion

typedef struct {
	int total_region_num;
	int motion_region_num;
	RTVencMotionSearchRegion *region;
} RTVencMotionSearchResult; ///< same to VencMotionSearchResult

//#define ISP_REG_TBL_LENGTH			33
//typedef struct RTsEncppSharpParamStatic {
//	unsigned char ss_shp_ratio;
//	unsigned char ls_shp_ratio;
//	unsigned short ss_dir_ratio;
//	unsigned short ls_dir_ratio;
//	unsigned short ss_crc_stren;
//	unsigned char ss_crc_min;
//	unsigned short wht_sat_ratio;
//	unsigned short blk_sat_ratio;
//	unsigned char wht_slp_bt;
//	unsigned char blk_slp_bt;
//
//	unsigned short sharp_ss_value[ISP_REG_TBL_LENGTH];
//	unsigned short sharp_ls_value[ISP_REG_TBL_LENGTH];
//	unsigned short sharp_hsv[46];
//} RTsEncppSharpParamStatic;
//
//typedef struct RTsEncppSharpParamDynamic {
//	unsigned short ss_ns_lw;
//	unsigned short ss_ns_hi;
//	unsigned short ls_ns_lw;
//	unsigned short ls_ns_hi;
//	unsigned char ss_lw_cor;
//	unsigned char ss_hi_cor;
//	unsigned char ls_lw_cor;
//	unsigned char ls_hi_cor;
//	unsigned short ss_blk_stren;
//	unsigned short ss_wht_stren;
//	unsigned short ls_blk_stren;
//	unsigned short ls_wht_stren;
//	unsigned char ss_avg_smth;
//	unsigned char ss_dir_smth;
//	unsigned char dir_smth[4];
//	unsigned char hfr_smth_ratio;
//	unsigned short hfr_hf_wht_stren;
//	unsigned short hfr_hf_blk_stren;
//	unsigned short hfr_mf_wht_stren;
//	unsigned short hfr_mf_blk_stren;
//	unsigned char hfr_hf_cor_ratio;
//	unsigned char hfr_mf_cor_ratio;
//	unsigned short hfr_hf_mix_ratio;
//	unsigned short hfr_mf_mix_ratio;
//	unsigned char hfr_hf_mix_min_ratio;
//	unsigned char hfr_mf_mix_min_ratio;
//	unsigned short hfr_hf_wht_clp;
//	unsigned short hfr_hf_blk_clp;
//	unsigned short hfr_mf_wht_clp;
//	unsigned short hfr_mf_blk_clp;
//	unsigned short wht_clp_para;
//	unsigned short blk_clp_para;
//	unsigned char wht_clp_slp;
//	unsigned char blk_clp_slp;
//	unsigned char max_clp_ratio;
//
//	unsigned short sharp_edge_lum[ISP_REG_TBL_LENGTH];
//} RTsEncppSharpParamDynamic;
//
//typedef struct RTsEncppSharpParam {
//	RTsEncppSharpParamDynamic *pDynamicParam;
//	RTsEncppSharpParamStatic  *pStaticParam;
//} RTsEncppSharpParam;
//
//typedef struct RTsEncppSharp {
//	int bsharp;
//	RTsEncppSharpParam sEncppSharpParam;
//} RTsEncppSharp;

typedef struct {
	unsigned int sum_mad;
	unsigned int sum_qp;
	unsigned long long sum_sse;
	unsigned int avg_sse;
} RTVencMBSumInfo;

typedef enum RTVENC_SUPERFRAME_MODE {
	RT_VENC_SUPERFRAME_NONE,
	RT_VENC_SUPERFRAME_DISCARD,
	RT_VENC_SUPERFRAME_REENCODE,
} RTVENC_SUPERFRAME_MODE; ///< same to VENC_SUPERFRAME_MODE

typedef struct RTVencSuperFrameConfig {
	RTVENC_SUPERFRAME_MODE	  eSuperFrameMode;
	unsigned int			nMaxIFrameBits;
	unsigned int			nMaxPFrameBits;
	int 					nMaxRencodeTimes;
	float					nMaxP2IFrameBitsRatio;
} RTVencSuperFrameConfig; ///< same to VencSuperFrameConfig




typedef enum RT_POWER_LINE_FREQUENCY{
	RT_FREQUENCY_DISABLED  = 0,
	RT_FREQUENCY_50HZ	   = 1,
	RT_FREQUENCY_60HZ	   = 2,
	RT_FREQUENCY_AUTO	   = 3,
} RT_POWER_LINE_FREQUENCY;

typedef enum RT_AE_METERING_MODE{
	RT_AE_METERING_MODE_AVERAGE = 0,
	RT_AE_METERING_MODE_CENTER = 1,
	RT_AE_METERING_MODE_SPOT = 2,
	RT_AE_METERING_MODE_MATRIX = 3,
} RT_AE_METERING_MODE;

typedef enum {
	/*isp_ctrl*/
	RT_ISP_CTRL_MODULE_EN = 0,
	RT_ISP_CTRL_DIGITAL_GAIN,
	RT_ISP_CTRL_PLTMWDR_STR,
	RT_ISP_CTRL_DN_STR,
	RT_ISP_CTRL_3DN_STR,
	RT_ISP_CTRL_HIGH_LIGHT,
	RT_ISP_CTRL_BACK_LIGHT,
	RT_ISP_CTRL_WB_MGAIN,
	RT_ISP_CTRL_AGAIN_DGAIN,
	RT_ISP_CTRL_COLOR_EFFECT,
	RT_ISP_CTRL_AE_ROI,
	RT_ISP_CTRL_AF_METERING,
	RT_ISP_CTRL_COLOR_TEMP,
	RT_ISP_CTRL_EV_IDX,
	RT_ISP_CTRL_MAX_EV_IDX,
	RT_ISP_CTRL_PLTM_HARDWARE_STR,
	RT_ISP_CTRL_ISO_LUM_IDX,
	RT_ISP_CTRL_COLOR_SPACE,
	RT_ISP_CTRL_VENC2ISP_PARAM,
	RT_ISP_CTRL_NPU_NR_PARAM,
	RT_ISP_CTRL_TOTAL_GAIN,
	RT_ISP_CTRL_AE_EV_LV,
	RT_ISP_CTRL_AE_EV_LV_ADJ,
	RT_ISP_CTRL_AE_WEIGHT_LUM,
	RT_ISP_CTRL_AE_LOCK,
	RT_ISP_CTRL_AE_FACE_CFG,
	RT_ISP_CTRL_MIPI_SWITCH,
	RT_ISP_CTRL_AE_TABLE,
	RT_ISP_CTRL_AE_STATS,
	RT_ISP_CTRL_IR_STATUS,
	RT_ISP_CTRL_IR_AWB_GAIN,
	RT_ISP_CTRL_READ_BIN_PARAM,
	RT_ISP_CTRL_AE_ROI_TARGET,
	RT_ISP_CTRL_AI_ISP,
} RT_ISP_CTRL_CFG_IDS; ///< ref to hw_isp_ctrl_cfg_ids

typedef enum {
	/*tunning_ctrl*/
	RT_ISP_CTRL_GET_SENSOR_CFG = 0,
	RT_ISP_CTRL_RPBUF_INIT,
	RT_ISP_CTRL_RPBUF_RELEASE,
	RT_ISP_CTRL_GET_ISP_PARAM,
	RT_ISP_CTRL_SET_ISP_PARAM,
	RT_ISP_CTRL_GET_LOG,
	RT_ISP_CTRL_GET_3A_STAT,
} RT_TUNNING_CTRL_IDS; //ref to hw_tunning_ctrl_ids

typedef struct {
	int grey; // 0 color 1 gray
	int ir_on;
	int ir_flash_on;
} RTIrParam;

typedef struct {
	int exp_val;
	int exp_mid_val;
	int exp_short_val;
	int gain_val;
	int gain_mid_val;
	int gain_short_val;
	int r_gain;
	int b_gain;
} RTIspExpGain;

typedef struct {
	unsigned int min_exp;//us
	unsigned int max_exp;
	unsigned int min_gain;
	unsigned int max_gain;
	unsigned int min_iris;
	unsigned int max_iris;
} RTIspAeTable; ///< ref to struct ae_table

typedef struct {
	RTIspAeTable ae_tbl[10];
	int length;
	int ev_step;
	int shutter_shift;
} RTIspAeTableInfo; ///< ref to struct ae_table_info

typedef struct {
	int awb_rgain_ir;
	int awb_bgain_ir;
} RTIspIrAwbGain; ///< same to struct isp_ir_awb_gain

typedef struct {
	int x1;
	int y1;
	int x2;
	int y2;
} RTIspH3aCoorWin; ///< same to struct isp_h3a_coor_win

typedef struct {
	unsigned short enable;
	unsigned short force_ae_target;
	RTIspH3aCoorWin coor;
} RTIspAeRoiAttr; ///< same to tstruct isp_ae_roi_attr

typedef struct {
	unsigned char is_overflow;
	unsigned short moving_level_table[484];
} RTEncMovingLevelInfo; ///< same to struct enc_MovingLevelInfo

typedef struct {
	RTIspH3aCoorWin face_ae_coor[RT_AE_FACE_MAX_NUM];
	unsigned char enable;
	unsigned char vaild_face_cnt;
	short face_ae_tolerance;
	short face_ae_speed;
	short face_ae_target;
	short face_ae_delay_cnt;
	unsigned short face_up_percent;
	unsigned short face_down_percent;
	unsigned short ae_face_block_num_thrd;
	unsigned short ae_face_block_weight;
	unsigned short ae_over_face_max_exp_control;
	unsigned short ae_face_win_weight[RT_AE_FACE_WIN_WEIGHT_LENGTH];
	int ae_face_pos_weight[RT_AE_FACE_POS_WEIGHT_LENGTH];
} RTFaceAeCfg;

typedef enum {
	RT_MIPI_SWITCH_A = 0,
	RT_MIPI_SWITCH_B = 1,
	RT_MIPI_SWITCH_MAX,
} RTMipiSwitchChoiceType;

typedef enum {
	RT_MIPI_GET_SWITCH = 0,
	RT_MIPI_SET_SWITCH = 1,
	RT_MIPI_CTRL_MAX,
} RTMipiSwitchCtrlType;

typedef struct {
	RTMipiSwitchCtrlType switch_ctrl;
	unsigned int mipi_switch_status;
	unsigned int comp_ratio;
	unsigned int exp_comp;
	unsigned int gain_comp;
	unsigned int drop_frame_num;
	unsigned long long time_stamp;
} RTSensorMipiSwitchEntity;

typedef struct {
	unsigned char enable_3d_filter;
	unsigned char adjust_pix_level_enable; // adjustment of coef pix level enable
	unsigned char smooth_filter_enable; //* 3x3 smooth filter enable
	unsigned char max_pix_diff_th; //* range[0~31]: maximum threshold of pixel difference
	unsigned char max_mad_th; //* range[0~63]: maximum threshold of mad
	unsigned char max_mv_th; //* range[0~63]: maximum threshold of motion vector
	unsigned char min_coef; //* range[0~16]: minimum weight of 3d filter
	unsigned char max_coef; //* range[0~16]: maximum weight of 3d filter,
} RTs3DfilterParam;

typedef struct {
	unsigned char enable_2d_filter;
	unsigned char filter_strength_uv; //* range[0~255], 0 means close 2d filter, advice: 32
	unsigned char filter_strength_y; //* range[0~255], 0 means close 2d filter, advice: 32
	unsigned char filter_th_uv; //* range[0~15], advice: 2
	unsigned char filter_th_y; //* range[0~15], advice: 2
} RTs2DfilterParam;

typedef struct RTVencH264VideoTiming {
	unsigned long num_units_in_tick;
	unsigned long time_scale;
	unsigned int fixed_frame_rate_flag;

} RTVencH264VideoTiming;

typedef struct RTVencH265TimingS {
	//0:stream without timing info; 1:stream with timing info
	unsigned char timing_info_present_flag;
	unsigned int num_units_in_tick; //time_scale/frameRate
	unsigned int time_scale; //1second is  average divided by time_scale
	unsigned int num_ticks_poc_diff_one; //num ticks of diff frame
} RTVencH265TimingS;

typedef struct {
	int bEnable;
	int nBlockNumber;
} RTVencCyclicIntraRefresh;

typedef struct {
	int d2d_level; //[1,1024], 256 means 1X
	int d3d_level; //[1,1024], 256 means 1X
	RTEncMovingLevelInfo mMovingLevelInfo;
} RTEncVencVe2IspParam; ///< same to struct enc_VencVe2IspParam

typedef struct {
	unsigned short cfg_id; ///< ref to RT_ISP_CTRL_CFG_IDS
	unsigned short update_flag;
	int ev_digital_gain;
	int pltmwdr_level;
	int denoise_level;
	int tdf_level;
	int ir_status;
	int ae_ev_idx;
	int ae_max_ev_idx;
	int ae_lum_idx;
	int ae_ev_lv;
	int ae_ev_lv_adj;
	int ae_lock;
	int awb_color_temp;
	int ae_weight_lum;
	RTIspIrAwbGain awb_ir_gain;
	RTIspAeTableInfo *ae_table;
	char path[100];
	RTIspAeRoiAttr ae_roi_area;
	unsigned char ae_stat_avg[432];
	RTEncVencVe2IspParam VencVe2IspParam;
	unsigned char ai_isp_en;
	unsigned char ai_isp_update;
	RTFaceAeCfg ae_face_info;
	RTSensorMipiSwitchEntity mipi_switch_info;
} RTIspCfgAttrData; ///< same to struct isp_cfg_attr_data

typedef struct {
	RTIspCfgAttrData isp_attr_cfg;
} RTIspCtrlAttr;

typedef struct {
	unsigned char isp_id;
	unsigned short sensor_width;
	unsigned short sensor_height;
	unsigned short act_fps;
	unsigned char wdr;
} RTTunningSensorCfg; //< same to struct tunning_sensor_cfg

typedef struct {
	unsigned short cfg_id;
	char path[100];
	RTTunningSensorCfg sensor_cfg;
} RTTunningCtlData; // < same to tunning_ctl_data

typedef struct {
	unsigned int width_max;
	unsigned int height_max;
	unsigned int width_min;
	unsigned int height_min;
} RTSensorResolution;

#define MAX_ISP_ORL_NUM (16)
struct orl_win {
	unsigned int left;
	unsigned int top;
	unsigned int width;
	unsigned int height;
	unsigned int rgb_orl;
};

typedef struct {
	unsigned char on;
	unsigned char orl_cnt;
	unsigned int orl_width;
	struct orl_win orl_win[MAX_ISP_ORL_NUM];
} RTIspOrl;

typedef enum RTeGdcPerspFunc {
	RT_Gdc_Persp_Only,
	RT_Gdc_Persp_LDC
} RTeGdcPerspFunc; ///< same to eGdcPerspFunc

typedef enum RTeGdcLensDistModel {
	RT_Gdc_DistModel_WideAngle,
	RT_Gdc_DistModel_FishEye
} RTeGdcLensDistModel; ///< same to eGdcLensDistModel

typedef enum RTeGdcMountType {
	RT_Gdc_Mount_Top,
	RT_Gdc_Mount_Wall,
	RT_Gdc_Mount_Bottom
} RTeGdcMountType; ///< same to eGdcMountType

typedef enum RTeGdcWarpType {
	RT_Gdc_Warp_LDC,
	RT_Gdc_Warp_Pano180,
	RT_Gdc_Warp_Pano360,
	RT_Gdc_Warp_Normal,
	RT_Gdc_Warp_Fish2Wide,
	RT_Gdc_Warp_Perspective,
	RT_Gdc_Warp_BirdsEye,
	RT_Gdc_Warp_User
} RTeGdcWarpType; ///< same to eGdcWarpType

typedef struct {
	unsigned char bGDC_en;
	RTeGdcWarpType eWarpMode;
	RTeGdcMountType eMountMode;
	unsigned char bMirror;
	unsigned int calib_widht;
	unsigned int calib_height;
	float fx;
	float fy;
	float cx;
	float cy;
	float fx_scale;
	float fy_scale;
	float cx_scale;
	float cy_scale;
	RTeGdcLensDistModel eLensDistModel;
	float distCoef_wide_ra[3];
	float distCoef_wide_ta[2];
	float distCoef_fish_k[4];
	int centerOffsetX;
	int centerOffsetY;
	int rotateAngle;
	int radialDistortCoef;
	int trapezoidDistortCoef;
	int fanDistortCoef;
	int pan;
	int tilt;
	int zoomH;
	int zoomV;
	int scale;
	int innerRadius;
	float roll;
	float pitch;
	float yaw;
	RTeGdcPerspFunc perspFunc;
	float perspectiveProjMat[9];
	int birdsImg_width;
	int birdsImg_height;
	float mountHeight;
	float roiDist_ahead;
	float roiDist_left;
	float roiDist_right;
	float roiDist_bottom;

	int peaking_en;
	int peaking_clamp;
	int peak_m;
	int th_strong_edge;
	int peak_weights_strength;
} RTsGdcParam; ///< same to sGdcParam

typedef enum RTeVeLbcMode {
	RT_LBC_MODE_DISABLE  = 0,
	RT_LBC_MODE_1_5X     = 1,
	RT_LBC_MODE_2_0X     = 2,
	RT_LBC_MODE_2_5X     = 3,
	RT_LBC_MODE_NO_LOSSY = 4,
} RTeVeLbcMode; ///< same to eVeLbcMode

typedef struct {
	int en_weak_text_th;
	float weak_text_th;
} RTVenWeakTextTh;

typedef struct {
	float weak_text_th;
} RTDynamicProcSettings;

typedef struct {
	int pix_x_bgn;
	int pix_x_end;
	int pix_y_bgn;
	int pix_y_end;
	int total_num;
	int zero_mv_num;
	int is_possible_static;
	int is_really_static;
	float zero_mv_rate;
} RTVencRegionD3DRegion; ///< same to VencRegionD3DRegion

typedef struct sRTVencRegionD3DResult RTVencRegionD3DResult; ///< same to VencRegionD3DResult
struct sRTVencRegionD3DResult{
    int total_region_num;
    int static_region_num;
    RTVencRegionD3DRegion *region;
    RTVencRegionD3DResult **prev;
    RTVencRegionD3DResult **next;
};

typedef struct {
	int en_region_d3d;
	int dis_default_para;
	int result_num;
	int hor_region_num;
	int ver_region_num;
	int hor_expand_num;
	int ver_expand_num;
    int lv_weak_th[9];
	float zero_mv_rate_th[3];
	unsigned char chroma_offset;
	unsigned char static_coef[3];		   //* fallow by zero_mv_rate
	unsigned char motion_coef[4];		   //* fallow by MoveStatus
} RTVencRegionD3DParam; ///< same to VencRegionD3DParam

typedef struct {
	unsigned char                 reserve_zero : 2;
	unsigned char                 constraint_5 : 1;
	unsigned char                 constraint_4 : 1;
	unsigned char                 constraint_3 : 1;
	unsigned char                 constraint_2 : 1;
	unsigned char                 constraint_1 : 1;
	unsigned char                 constraint_0 : 1;
} RTVencH264ConstraintFlag; ///< same to VencH264ConstraintFlag

typedef struct {
	int en_d2d_limit;
	int d2d_level[6];
} RTVencVe2IspD2DLimit; ///< same to VencVe2IspD2DLimit

typedef struct {
    unsigned int                  en_force_conf;
    unsigned int                  left_offset;
    unsigned int                  right_offset;
    unsigned int                  top_offset;
    unsigned int                  bottom_offset;
} RTVencForceConfWin; ///< same to VencForceConfWin

typedef struct {
    int dis_default_d2d;
    int d2d_min_lv_th;
    int d2d_max_lv_th;
    int d2d_min_lv_level;
    int d2d_max_lv_level;
    int dis_default_d3d;
    int d3d_min_lv_th;
    int d3d_max_lv_th;
    int d3d_min_lv_level;
    int d3d_max_lv_level;
} RTVencRotVe2Isp; ///< same to VencRotVe2Isp

typedef struct {
    unsigned char *pBuffer;
    unsigned int  nBufLen;
    unsigned int  nDataLen;
    unsigned int  nType;
} RTVencSeiData; ///< same to VencSeiData

typedef struct {
    unsigned int nSeiNum;
    RTVencSeiData  *pSeiData;
} RTVencSeiParam; ///< same to VencSeiParam

#define RT_MAX_INSERT_FRAME_LEN (196569)

typedef enum {
    RT_BUF_IDLE,
    RT_BUF_STANDBY,
    RT_BUF_OCCUPY,
} RT_VENC_BUF_STATUS; ///< same to VENC_BUF_STATUS

typedef struct {
    unsigned char *pBuffer;
    unsigned int  nBufLen;
    unsigned int  nDataLen;
    unsigned int  nFrameRate;
    RT_VENC_BUF_STATUS eStatus;
} RTVencInsertData; ///< same to VencInsertData

// notify callback event to AWVideoInput_cxt, using netlink.
typedef enum {
    RTCB_EVENT_VENC_DROP_FRAME = 0,
} RTCbEventType;

typedef struct {
    RTCbEventType eEventType;
    int nData1;
    int nData2;
} RTCbEvent;

/**
  generic netlink rt_media family definition.
*/
enum {
	RT_MEDIA_CMD_UNSPEC = 0,	/* Reserved */
	RT_MEDIA_CMD_SEND,		/* user->kernel, send some infos such as channelId */
	RT_MEDIA_CMD_CB_EVENT,	/* kernel->user, send rt_media callback event to AWVideoInput */
	__RT_MEDIA_CMD_MAX,
};

#define RT_MEDIA_CMD_MAX (__RT_MEDIA_CMD_MAX - 1)

/**
  rt_media family netlink attribute definition.
*/
enum {
	RT_MEDIA_CMD_ATTR_UNSPEC = 0,
	RT_MEDIA_CMD_ATTR_CHNID, //record channel id, type = int.
	RT_MEDIA_CMD_ATTR_RTCBEVENT, //type = RTCbEvent
	__RT_MEDIA_CMD_ATTR_MAX,
};

#define RT_MEDIA_CMD_ATTR_MAX (__RT_MEDIA_CMD_ATTR_MAX - 1)

typedef enum {
    RT_VENC_CAMERA_ADAPTIVE_STATIC = 0,
    RT_VENC_CAMERA_FORCE_STATIC = 1,
    RT_VENC_CAMERA_FORCE_MOVING = 2,
    RT_VENC_CAMERA_ADAPTIVE_MOVING_AND_STATIC = 3,
    RT_VENC_CAMERA_STATUS_NUM
} RT_VENC_CAMERA_MOVE_STATUS;

typedef struct {
	int dis_default_para;
	int mode;
	int en_gop_clip;
	float gop_bit_ratio_th[3];
	float coef_th[5][2];
} RT_VENC_TARGET_BITS_CLIP_PARAM; /* < same to VencTargetBitsClipParam > */

typedef struct {
	int flag;			//0: print; 1: save to file; others: reserve for future use
	char *path;			//path string
	unsigned int len;	//path string length
} VIN_ISP_REG_GET_CFG;

typedef enum {
	RT_BX_TO_CLOSE = 0,
	RT_B10_TO_B8 = 1,
	RT_B8_TO_B10 = 2,
} RT_VIN_SET_BIT_WIDTH;

/**
  NETLINK_GENERIC related info
  RT_MEDIA_GENL_NAME: rt_media generic netlink family name, string_length + 1 <= GENL_NAMSIZ.
  RT_MEDIA_GENL_VERSION: rt_media generic netlink family version
*/
#define RT_MEDIA_GENL_NAME	"genl-rt_media"
#define RT_MEDIA_GENL_VERSION	0x1

#endif

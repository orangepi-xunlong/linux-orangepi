/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File name: include/comm_video.h
 * Description:
 *   Common video definitions.
 */

#ifndef __COMM_VIDEO_H__
#define __COMM_VIDEO_H__

#include <linux/types.h>
#include <linux/common.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define SRC_LENS_COEF_SEG 2
#define DST_LENS_COEF_SEG 3
#define SRC_LENS_COEF_NUM 4
#define DST_LENS_COEF_NUM 4

#define ISP_BAYER_CHN (4)

#define IS_FMT_RGB(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) || \
	 (fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR))

#define IS_FMT_YUV(fmt) \
	((fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) || \
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) || \
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) || \
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) || \
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) || \
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY) || \
	 (fmt == PIXEL_FORMAT_YUV_444))

#define IS_FMT_YUV420(fmt) \
	((fmt == PIXEL_FORMAT_YUV_PLANAR_420) || \
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21))

#define IS_FMT_YUV422(fmt) \
	((fmt == PIXEL_FORMAT_YUV_PLANAR_422) || \
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) || \
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) || \
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY))

#define IS_FRAME_OFFSET_INVALID(f) \
	((f).offset_left < 0 || (f).offset_right < 0 || \
	 (f).offset_top < 0 || (f).offset_bottom < 0 || \
	 ((u32)((f).offset_left + (f).offset_right) > (f).width) || \
	 ((u32)((f).offset_top + (f).offset_bottom) > (f).height))

typedef enum _operation_mode_e {
	OPERATION_MODE_AUTO = 0,
	OPERATION_MODE_MANUAL = 1,
	OPERATION_MODE_BUTT
} operation_mode_e;

/*Angle of rotation*/
typedef enum _rotation_e {
	ROTATION_0 = 0,
	ROTATION_90,
	ROTATION_180,
	ROTATION_270,
	ROTATION_XY_FLIP,
	ROTATION_MAX
} rotation_e;

/* Mirror direction*/
typedef enum _mirror_type_e {
    MIRDIR_TYPE_NONE,   /**< No mirroring */
    MIRDIR_TYPE_VER,    /**< Vertical mirroring */
    MIRDIR_TYPE_HOR,    /**< Horizontal mirroring */
    MIRDIR_TYPE_HOR_VER, /**< Horizontal and vertical mirroring */
	MIRDIR_TYPE_BUTT
} mirror_type_e;

typedef enum _vb_source_e {
	VB_SOURCE_COMMON = 0,
	VB_SOURCE_MODULE = 1,
	VB_SOURCE_PRIVATE = 2,
	VB_SOURCE_USER = 3,
	VB_SOURCE_BUTT
} vb_source_e;

typedef struct _border_s {
	unsigned int top_width;
	unsigned int bottom_width;
	unsigned int left_width;
	unsigned int right_width;
	unsigned int color;
} border_s;

typedef struct _point_s {
	int x;
	int y;
} point_s;

typedef struct _size_s {
	unsigned int width;
	unsigned int height;
} size_s;

typedef struct _rect_s {
	int x;
	int y;
	unsigned int width;
	unsigned int height;
} rect_s;

typedef struct _video_region_info_s {
	unsigned int region_num; /* W; count of the region */
	rect_s *region; /* W; region attribute */
} video_region_info_s;

typedef struct _crop_info_s {
	unsigned char enable;
	rect_s rect;
} crop_info_s;

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef struct _frame_rate_ctrl_s {
	int src_frame_rate; /* RW; source frame rate */
	int dst_frame_rate; /* RW; dest frame rate */
} frame_rate_ctrl_s;
// -------- If you want to change these interfaces, please contact the isp team. --------

/*
 * ASPECT_RATIO_NONE: full screen
 * ASPECT_RATIO_AUTO: Keep ratio, automatically get the region of video.
 * ASPECT_RATIO_MANUAL: Manully set the region of video.
 */
typedef enum _aspect_ratio_e {
	ASPECT_RATIO_NONE = 0,
	ASPECT_RATIO_AUTO,
	ASPECT_RATIO_MANUAL,
	ASPECT_RATIO_MAX
} aspect_ratio_e;

/*
 * enMode: aspect ratio mode: none/auto/manual
 * enableBgColor: fill bgcolor
 * u32BgColor: background color, RGB 888
 * stVideoRect: valid in ASPECT_RATIO_MANUAL mode
 */
typedef struct _aspect_ratio_s {
	aspect_ratio_e mode;
	unsigned char enable_bgcolor;
	unsigned int bgcolor;
	rect_s video_rect;
} aspect_ratio_s;

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _pixel_format_e {
	PIXEL_FORMAT_RGB_888 = 0,
	PIXEL_FORMAT_BGR_888,
	PIXEL_FORMAT_RGB_888_PLANAR,
	PIXEL_FORMAT_BGR_888_PLANAR,

	PIXEL_FORMAT_ARGB_1555, // 4,
	PIXEL_FORMAT_ARGB_4444,
	PIXEL_FORMAT_ARGB_8888,

	PIXEL_FORMAT_RGB_BAYER_8BPP, // 7,
	PIXEL_FORMAT_RGB_BAYER_10BPP,
	PIXEL_FORMAT_RGB_BAYER_12BPP,
	PIXEL_FORMAT_RGB_BAYER_14BPP,
	PIXEL_FORMAT_RGB_BAYER_16BPP,

	PIXEL_FORMAT_YUV_PLANAR_422, // 12,
	PIXEL_FORMAT_YUV_PLANAR_420,
	PIXEL_FORMAT_YUV_PLANAR_444,
	PIXEL_FORMAT_YUV_400,

	PIXEL_FORMAT_HSV_888, // 16,
	PIXEL_FORMAT_HSV_888_PLANAR,

	PIXEL_FORMAT_NV12, // 18,
	PIXEL_FORMAT_NV21,
	PIXEL_FORMAT_NV16,
	PIXEL_FORMAT_NV61,
	PIXEL_FORMAT_YUYV, //22
	PIXEL_FORMAT_UYVY,
	PIXEL_FORMAT_YVYU,
	PIXEL_FORMAT_VYUY,
	PIXEL_FORMAT_YUV_444,

	PIXEL_FORMAT_FP32_C1 = 32, // 32
	PIXEL_FORMAT_FP32_C3_PLANAR,
	PIXEL_FORMAT_INT32_C1,
	PIXEL_FORMAT_INT32_C3_PLANAR,
	PIXEL_FORMAT_UINT32_C1,
	PIXEL_FORMAT_UINT32_C3_PLANAR,
	PIXEL_FORMAT_FP16_C1,
	PIXEL_FORMAT_FP16_C3_PLANAR,
	PIXEL_FORMAT_BF16_C1, // 40
	PIXEL_FORMAT_BF16_C3_PLANAR,
	PIXEL_FORMAT_INT16_C1,
	PIXEL_FORMAT_INT16_C3_PLANAR,
	PIXEL_FORMAT_UINT16_C1,
	PIXEL_FORMAT_UINT16_C3_PLANAR,
	PIXEL_FORMAT_INT8_C1,
	PIXEL_FORMAT_INT8_C3_PLANAR,
	PIXEL_FORMAT_UINT8_C1,
	PIXEL_FORMAT_UINT8_C3_PLANAR,

	PIXEL_FORMAT_8BIT_MODE = 50, //50

	PIXEL_FORMAT_MAX
} pixel_format_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

/*
 * VIDEO_FORMAT_LINEAR: nature video line.
 */
// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _video_format_e {
	VIDEO_FORMAT_LINEAR = 0,
	VIDEO_FORMAT_MAX
} video_format_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

/*
 * COMPRESS_MODE_NONE: no compress.
 * COMPRESS_MODE_TILE: compress unit is a tile.
 * COMPRESS_MODE_LINE: compress unit is the whole line.
 * COMPRESS_MODE_FRAME: compress unit is the whole frame.
 */
// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _compress_mode_e {
	COMPRESS_MODE_NONE = 0,
	COMPRESS_MODE_TILE,
	COMPRESS_MODE_LINE,
	COMPRESS_MODE_FRAME,
	COMPRESS_MODE_BUTT
} compress_mode_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _bayer_format_e {
	BAYER_FORMAT_BG = 0,
	BAYER_FORMAT_GB,
	BAYER_FORMAT_GR,
	BAYER_FORMAT_RG,
	/*for rgb-ir sensor*/
	BAYER_FORMAT_GRGBI = 8,
	BAYER_FORMAT_RGBGI,
	BAYER_FORMAT_GBGRI,
	BAYER_FORMAT_BGRGI,
	BAYER_FORMAT_IGRGB,
	BAYER_FORMAT_IRGBG,
	BAYER_FORMAT_IBGRG,
	BAYER_FORMAT_IGBGR,
	BAYER_FORMAT_MAX
} bayer_format_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

typedef enum _video_display_mode_e {
	VIDEO_DISPLAY_MODE_PREVIEW = 0x0,
	VIDEO_DISPLAY_MODE_PLAYBACK = 0x1,

	VIDEO_DISPLAY_MODE_MAX
} video_display_mode_e;

/*
 * u32ISO:  ISP internal ISO : Again*Dgain*ISPgain
 * u32ExposureTime:  Exposure time (reciprocal of shutter speed),unit is us
 * u32FNumber: The actual F-number (F-stop) of lens when the image was taken
 * u32SensorID: which sensor is used
 * u32HmaxTimes: Sensor HmaxTimes,unit is ns
 * u32VcNum: when dump wdr frame, which is long or short  exposure frame.
 */
// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef struct _isp_frame_info_s {
	unsigned int iso;
	unsigned int exposure_time;
	unsigned int isp_dgain;
	unsigned int again;
	unsigned int dgain;
	unsigned int ratio[3];
	unsigned int isp_nrstrength;
	unsigned int f_number;
	unsigned int sensor_id;
	unsigned int sensor_mode;
	unsigned int hmax_times;
	unsigned int vc_num;
} isp_frame_info_s;
// -------- If you want to change these interfaces, please contact the isp team. --------

typedef struct _isp_hdr_info_s {
	unsigned int color_temp;
	unsigned short ccm[9];
	unsigned char saturation;
} isp_hdr_info_s;

typedef struct _isp_attach_info_s {
	isp_hdr_info_s isp_hdr;
	unsigned int iso;
	unsigned char *sns_wdr_mode;
} isp_attach_info_s;

typedef enum _frame_flag_e {
	FRAME_FLAG_SNAP_FLASH = 0x1 << 0,
	FRAME_FLAG_SNAP_CUR = 0x1 << 1,
	FRAME_FLAG_SNAP_REF = 0x1 << 2,
	FRAME_FLAG_SNAP_END = 0x1 << 31,
	FRAME_FLAG_MAX
} frame_flag_e;

/* RGGB=4 */
#define ISP_WB_GAIN_NUM 4
/* 3*3=9 matrix */
#define ISP_CAP_CCM_NUM 9
typedef struct _isp_config_info_s {
	unsigned int iso;
	unsigned int isp_dgain;
	unsigned int exposure_time;
	unsigned int white_balance_gain[ISP_WB_GAIN_NUM];
	unsigned int color_temperature;
	unsigned short cap_ccm[ISP_CAP_CCM_NUM];
} isp_config_info_s;

/*
 * pJpegDCFVirAddr: JPEG_DCF_S, used in JPEG DCF
 * pIspInfoVirAddr: ISP_FRAME_INFO_S, used in ISP debug, when get raw and send raw
 * pLowDelayVirAddr: used in low delay
 */
typedef struct _video_supplement_s {
	unsigned long long jpeg_dcf_phy_addr;
	unsigned long long isp_info_phy_addr;
	unsigned long long low_delay_phy_addr;
	unsigned long long frame_dng_phy_addr;

	void * ATTRIBUTE jpeg_dcf_vir_addr;
	void * ATTRIBUTE isp_info_vir_addr;
	void * ATTRIBUTE low_delay_vir_addr;
	void * ATTRIBUTE frame_dng_vir_addr;
} video_supplement_s;

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _color_gamut_e {
	COLOR_GAMUT_BT601 = 0,
	COLOR_GAMUT_BT709,
	COLOR_GAMUT_BT2020,
	COLOR_GAMUT_USER,
	COLOR_GAMUT_MAX
} color_gamut_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

typedef struct _isp_colorgammut_info_s {
	color_gamut_e encolorgamut;
} isp_colorgammut_info_s;

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _dynamic_range_e {
	DYNAMIC_RANGE_SDR8 = 0,
	DYNAMIC_RANGE_SDR10,
	DYNAMIC_RANGE_HDR10,
	DYNAMIC_RANGE_HLG,
	DYNAMIC_RANGE_SLF,
	DYNAMIC_RANGE_XDR,
	DYNAMIC_RANGE_MAX
} dynamic_range_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _data_bitwidth_e {
	DATA_BITWIDTH_8 = 0,
	DATA_BITWIDTH_10,
	DATA_BITWIDTH_12,
	DATA_BITWIDTH_14,
	DATA_BITWIDTH_16,
	DATA_BITWIDTH_MAX
} data_bitwidth_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

/**
 * @brief Define video frame
 *
 * offset_top: top offset of show area
 * offset_bottom: bottom offset of show area
 * offset_left: left offset of show area
 * offset_right: right offset of show area
 * u32FrameFlag: FRAME_FLAG_E, can be OR operation.
 */
// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef struct _video_frame_s {
	unsigned int width;
	unsigned int height;
	pixel_format_e pixel_format;
	bayer_format_e bayer_format;
	video_format_e video_format;
	compress_mode_e compress_mode;
	dynamic_range_e dynamic_range;
	color_gamut_e color_gamut;
	unsigned int stride[3];

	unsigned long long phyaddr[3];
	unsigned char *viraddr[3];
#ifdef __arm__
	unsigned int vir_addr_padding[3];
#endif
	unsigned int length[3];

	unsigned long long ext_phy_addr;
	unsigned char *ext_virt_addr;
#ifdef __arm__
	unsigned int ext_virt_addr_padding;
#endif
	unsigned int ext_length;

	short offset_top;
	short offset_bottom;
	short offset_left;
	short offset_right;

	unsigned int time_ref;
	unsigned long long pts;
	unsigned long long dts;

	void *private_data;
#ifdef __arm__
	unsigned int private_data_padding;
#endif
	unsigned int frame_flag;
	int frame_idx;
	unsigned char srcend;
	unsigned int align;
	unsigned char interl_aced_frame;
	unsigned char pic_type;
	unsigned char endian;
	unsigned int seqenceno;
} video_frame_s;

// -------- If you want to change these interfaces, please contact the isp team. --------

/**
 * @brief Define the information of video frame.
 *
 * video_frame: Video frame info.
 * u32PoolId: VB pool ID.
 */
// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef struct _video_frame_info_s {
	video_frame_s video_frame;
	unsigned int pool_id;
} video_frame_info_s;
// -------- If you want to change these interfaces, please contact the isp team. --------

/*
 * u32VBSize: size of VB needed.
 * u32MainStride: stride of planar0.
 * u32CStride: stride of planar1/2 if there is.
 * u32MainSize: size of all planars.
 * u32MainYSize: size of planar0.
 * main_c_size: size of planar1/2 if there is.
 * u16AddrAlign: address alignment needed between planar.
 */
typedef struct _vb_cal_config_s {
	unsigned int vb_size;

	unsigned int main_stride;
	unsigned int c_stride;
	unsigned int main_size;
	unsigned int main_y_size;
	unsigned int main_c_size;
	unsigned short addr_align;
	unsigned char  plane_num;
} vb_cal_config_s;

/*
 * pixel_format: Bitmap's pixel format
 * width: Bitmap's width
 * height: Bitmap's height
 * pData: Address of Bitmap's data
 */
typedef struct _bitmap_s {
	pixel_format_e pixel_format;
	unsigned int width;
	unsigned int height;

	void * ATTRIBUTE data;
} bitmap_s;

/*
 *
 * s32CenterXOffset: RW; Range: [-511, 511], horizontal offset of the image distortion center relative to image center
 * s32CenterYOffset: RW; Range: [-511, 511], vertical offset of the image distortion center relative to image center
 * s32DistortionRatio: RW; Range: [-300, 500], LDC Distortion ratio.
 *		    When spread on,s32DistortionRatio range should be [0, 500]
 */
// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++

typedef struct _homograph_region_s {
	char region_name[128];
	float matrix[9];
	unsigned char meshgrid_x;
	unsigned char meshgrid_y;
	unsigned char meshgrid_w;
	unsigned char meshgrid_h;
} homograph_region_s;

typedef struct _grid_info_attr_s {
	unsigned char enable;
	char grid_file_name[128];
	char grid_bind_name[128];
	size_s grid_in;
	size_s grid_out;
	unsigned char is_blending;
	unsigned char eis_enable; /* enable eis */
	unsigned char homorgn_num;
#if 0
	homograph_region_s homo_regions[gdc_max_region_num];
#endif
} grid_info_attr_s;

typedef struct _ldc_attr_s {
	unsigned char aspect; /* rw;whether aspect ration  is keep */
	int x_ratio; /* rw; range: [0, 100], field angle ration of  horizontal,valid when baspect=0.*/
	int y_ratio; /* rw; range: [0, 100], field angle ration of  vertical,valid when baspect=0.*/
	int xy_ratio; /* rw; range: [0, 100], field angle ration of  all,valid when baspect=1.*/
	int center_x_offset;
	int center_y_offset;
	int distortion_ratio;
	grid_info_attr_s grid_info_attr;
	unsigned char enable_hw_ldc;
	rotation_e rotation;
} ldc_attr_s;
// -------- If you want to change these interfaces, please contact the isp team. --------

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _wdr_mode_e {
	WDR_MODE_NONE = 0,
	WDR_MODE_BUILT_IN,
	WDR_MODE_QUDRA,

	WDR_MODE_2TO1_LINE,
	WDR_MODE_2TO1_FRAME,
	WDR_MODE_2TO1_FRAME_FULL_RATE,

	WDR_MODE_3TO1_LINE,
	WDR_MODE_3TO1_FRAME,
	WDR_MODE_3TO1_FRAME_FULL_RATE,

	WDR_MODE_4TO1_LINE,
	WDR_MODE_4TO1_FRAME,
	WDR_MODE_4TO1_FRAME_FULL_RATE,

	WDR_MODE_MAX,
} wdr_mode_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _proc_amp_e {
	PROC_AMP_BRIGHTNESS = 0,
	PROC_AMP_CONTRAST,
	PROC_AMP_SATURATION,
	PROC_AMP_HUE,
	PROC_AMP_MAX,
} proc_amp_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

typedef struct _proc_amp_ctrl_s {
	int minimum;
	int maximum;
	int step;
	int default_value;
} proc_amp_ctrl_s;

typedef struct _vcodec_perf_fps_s {
	unsigned int in_fps;
	unsigned int out_fps;
	unsigned long long hw_time;
	unsigned long long max_hw_time;
} vcodec_perf_fps_s;

typedef enum {
	SEQ_INIT_NON,
	SEQ_INIT_START,
	SEQ_CHANGE,
	SEQ_DECODE_START,
	SEQ_DECODE_FINISH,
}seq_status;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* _COMM_VIDEO_H_ */

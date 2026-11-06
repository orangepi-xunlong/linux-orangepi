/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_comm_vpss.h
 * Description:
 *   The common data type defination for VPSS module.
 */

#ifndef __COMM_VPSS_H__
#define __COMM_VPSS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <linux/common.h>
#include <linux/comm_video.h>
#include <linux/comm_vpss.h>

#define VPSS_INVALID_FRMRATE -1
#define VPSS_CHN0             0
#define VPSS_CHN1             1
#define VPSS_CHN2             2
#define VPSS_CHN3             3
#define VPSS_INVALID_CHN     -1
#define VPSS_INVALID_GRP     -1

/*
 * VPSS_CROP_RATIO_COOR: Ratio coordinate.
 * VPSS_CROP_ABS_COOR: Absolute coordinate.
 */
typedef enum _vpss_crop_coordinate_e {
	VPSS_CROP_RATIO_COOR = 0,
	VPSS_CROP_ABS_COOR,
} vpss_crop_coordinate_e;

typedef enum _vpss_rounding_e {
	VPSS_ROUNDING_TO_EVEN = 0,
	VPSS_ROUNDING_AWAY_FROM_ZERO,
	VPSS_ROUNDING_TRUNCATE,
	VPSS_ROUNDING_MAX,
} vpss_rounding_e;

/*
 * enable: Whether Normalize is enabled.
 * factor: scaling factors for 3 planes, [1.0f/8192, 1].
 * mean: minus means for 3 planes, [0, 255].
 */
typedef struct _vpss_normalize_s {
	unsigned char enable;
	float factor[3];
	float mean[3];
	vpss_rounding_e rounding;
} vpss_normalize_s;

/*
 * w: Range: Width of source image.
 * h: Range: Height of source image.
 * pixel_format: Pixel format of target image.
 * frame_rate: Frame rate control info.
 */
typedef struct _vpss_grp_attr_s {
	unsigned int w;
	unsigned int h;
	pixel_format_e pixel_format;
	frame_rate_ctrl_s frame_rate;
} vpss_grp_attr_s;

/*
 * width: Width of target image.
 * height: Height of target image.
 * video_format: Video format of target image.
 * pixel_format: Pixel format of target image.
 * frame_rate: Frame rate control info.
 * mirror: Mirror enable.
 * flip: Flip enable.
 * depth: User get list depth.
 * aspect_ratio: Aspect Ratio info.
 */
typedef struct _vpss_chn_attr_s {
	unsigned int width;
	unsigned int height;
	video_format_e video_format;
	pixel_format_e pixel_format;
	frame_rate_ctrl_s frame_rate;
	unsigned char mirror;
	unsigned char flip;
	unsigned int depth;
	aspect_ratio_s aspect_ratio;
	vpss_normalize_s normalize;
} vpss_chn_attr_s;

/*
 * enable: RW; CROP enable.
 * crop_coordinate: RW; Coordinate mode of the crop start point.
 * crop_rect: CROP rectangle.
 */
typedef struct _vpss_crop_info_s {
	unsigned char enable;
	vpss_crop_coordinate_e crop_coordinate;
	rect_s crop_rect;
} vpss_crop_info_s;

/*
 * enable: Whether LDC is enbale.
 * attr: LDC Attribute.
 */
typedef struct _vpss_ldc_attr_s {
	unsigned char enable;
	ldc_attr_s attr;
} vpss_ldc_attr_s;

typedef enum _vpss_buf_source_e {
	VPSS_COMMON_VB = 0,
	VPSS_USER_VB,
	VPSS_USER_ION,
} vpss_buf_source_e;

typedef struct _vpss_param_mod_s {
	vpss_buf_source_e vpss_buf_source;
} vpss_mod_param_s;

typedef enum _vpss_scale_coef_e {
	VPSS_SCALE_COEF_BICUBIC = 0,
	VPSS_SCALE_COEF_BILINEAR,
	VPSS_SCALE_COEF_NEAREST,
	VPSS_SCALE_COEF_BICUBIC_OPENCV,
	VPSS_SCALE_COEF_MAX,
} vpss_scale_coef_e;

typedef struct _vpss_rect_s {
	unsigned char enable;
	unsigned short thick;   /* width of line */
	unsigned int bg_color; /* rgb888, b:bit0 - bit7, g:bit8 - bit15, r:bit16 - bit23 */
	rect_s rect;      /*outside rectangle*/
} vpss_rect_s;

typedef struct _vpss_draw_rect_s {
	vpss_rect_s rects[VPSS_RECT_NUM];
} vpss_draw_rect_s;

typedef struct _vpss_convert_s {
	unsigned char enable;
	unsigned int a_factor[3];
	unsigned int b_factor[3];
} vpss_convert_s;

typedef struct _vpss_chn_buf_wrap_s {
	unsigned char enable;
	unsigned int buf_line;	// 64, 128
	unsigned int wrap_buffer_size;	// depth for now
} vpss_chn_buf_wrap_s;

typedef struct _vpss_stitch_chn_attr_s {
	video_frame_info_s video_frame;
	rect_s dst_rect;
} vpss_stitch_chn_attr_s;

typedef struct _vpss_stitch_output_attr_s {
	unsigned int color; // backgroud color rgb888, b:bit0 - bit7, g:bit8 - bit15, r:bit16 - bit23
	pixel_format_e pixel_format;
	unsigned int width;
	unsigned int height;
} vpss_stitch_output_attr_s;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /* __CVI_COMM_VPSS_H__ */

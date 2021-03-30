/*
 ******************************************************************************
 *
 * camera.h
 *
 * Hawkview ISP - camera.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http:
 *
 * Version		Author         Date		    Description
 *
 *   3.0		Yang Feng   	2015/12/18	VIDEO INPUT
 *
 *****************************************************************************
 */

#ifndef __CAMERA__H__
#define __CAMERA__H__

#include <media/v4l2-subdev.h>
#include <linux/videodev2.h>
#include "../../vin-video/vin_core.h"
#include "../../utility/vin_supply.h"
#include "../../vin-cci/cci_helper.h"
#include "camera_cfg.h"
#include "../../platform/platform_cfg.h"
/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define ABS_SENSOR(x)                 ((x) > 0 ? (x) : -(x))

#define HXGA_WIDTH    4000
#define HXGA_HEIGHT   3000
#define QUXGA_WIDTH   3264
#define QUXGA_HEIGHT  2448
#define QSXGA_WIDTH   2592
#define QSXGA_HEIGHT  1936
#define QXGA_WIDTH    2048
#define QXGA_HEIGHT   1536
#define HD1080_WIDTH  1920
#define HD1080_HEIGHT 1080
#define UXGA_WIDTH    1600
#define UXGA_HEIGHT   1200
#define SXGA_WIDTH    1280
#define SXGA_HEIGHT   960
#define HD720_WIDTH   1280
#define HD720_HEIGHT  720
#define XGA_WIDTH     1024
#define XGA_HEIGHT    768
#define SVGA_WIDTH    800
#define SVGA_HEIGHT   600
#define VGA_WIDTH     640
#define VGA_HEIGHT    480
#define QVGA_WIDTH    320
#define QVGA_HEIGHT   240
#define CIF_WIDTH     352
#define CIF_HEIGHT    288
#define QCIF_WIDTH    176
#define QCIF_HEIGHT   144

#define CSI_GPIO_HIGH     1
#define CSI_GPIO_LOW     0
#define CCI_BITS_8           8
#define CCI_BITS_16         16
#define SENSOR_MAGIC_NUMBER 0x156977

struct sensor_info {
	struct v4l2_subdev sd;
	struct media_pad sensor_pads[SENSOR_PAD_NUM];
	struct mutex lock;
	struct sensor_format_struct *fmt;	/* Current format */
	unsigned int width;
	unsigned int height;
	unsigned int capture_mode;	/*V4L2_MODE_VIDEO/V4L2_MODE_IMAGE*/
	unsigned int af_first_flag;
	unsigned int preview_first_flag;
	unsigned int auto_focus;	/*0:not in contin_focus 1: contin_focus*/
	unsigned int focus_status;	/*0:idle 1:busy*/
	unsigned int low_speed;		/*0:high speed 1:low speed*/
	int brightness;
	int contrast;
	int saturation;
	int hue;
	unsigned int hflip;
	unsigned int vflip;
	unsigned int gain;
	unsigned int autogain;
	unsigned int exp;
	int exp_bias;
	enum v4l2_exposure_auto_type autoexp;
	unsigned int autowb;
	enum v4l2_auto_n_preset_white_balance wb;
	enum v4l2_colorfx clrfx;
	enum v4l2_flash_led_mode flash_mode;
	enum v4l2_power_line_frequency band_filter;
	struct v4l2_fract tpf;
	struct sensor_win_size *current_wins;
	unsigned int magic_num;
};

#define SENSOR_ENUM_MBUS_CODE \
static int sensor_enum_mbus_code(struct v4l2_subdev *sd, \
				 struct v4l2_subdev_fh *fh, \
				 struct v4l2_subdev_mbus_code_enum *code) \
{ \
	if (code->index >= N_FMTS) \
		return -EINVAL; \
	code->code = sensor_formats[code->index].mbus_code; \
	return 0; \
}

#define SENSOR_ENUM_FRAME_SIZE \
static int sensor_enum_frame_size(struct v4l2_subdev *sd, \
				  struct v4l2_subdev_fh *fh, \
				  struct v4l2_subdev_frame_size_enum *fse) \
{ \
	if (fse->index >= N_WIN_SIZES) \
		return -EINVAL; \
	fse->min_width = sensor_win_sizes[fse->index].width; \
	fse->max_width = fse->min_width; \
	fse->max_height = sensor_win_sizes[fse->index].height; \
	fse->min_height = fse->max_height; \
	return 0; \
}

#define SENSOR_FIND_MBUS_CODE \
static struct sensor_format_struct *sensor_find_mbus_code(struct v4l2_mbus_framefmt *fmt) \
{ \
	int i; \
	for (i = 0; i < N_FMTS; ++i) { \
		if (sensor_formats[i].mbus_code == fmt->code) \
			break; \
	} \
	if (i >= N_FMTS) \
		return sensor_formats; \
	return sensor_formats + i; \
}

#define SENSOR_FIND_FRAME_SIZE \
static struct sensor_win_size *sensor_find_frame_size(struct v4l2_mbus_framefmt *fmt) \
{ \
	struct sensor_win_size *ws; \
	struct sensor_win_size *best_ws; \
	int best_dist = INT_MAX; \
	int i; \
	ws = sensor_win_sizes; \
	best_ws = NULL; \
	for (i = 0; i < N_WIN_SIZES; ++i) { \
		int dist = abs(ws->width - fmt->width) + \
		    abs(ws->height - fmt->height); \
		if (dist < best_dist) { \
			best_dist = dist; \
			best_ws = ws; \
		} \
		++ws; \
	} \
	return best_ws; \
}

#define SENSOR_TRY_FORMAT \
static void sensor_try_format(struct sensor_info *info, \
			      struct v4l2_subdev_fh *fh, \
			      struct v4l2_subdev_format *fmt, \
			      struct sensor_win_size **ws, \
			      struct sensor_format_struct **sf) \
{ \
	u32 code = V4L2_MBUS_FMT_YUYV8_2X8; \
	if (fmt->pad == SENSOR_PAD_SOURCE) { \
		*ws = sensor_find_frame_size(&fmt->format); \
		*sf = sensor_find_mbus_code(&fmt->format); \
		code = (*sf)->mbus_code; \
	} \
	sensor_fill_mbus_fmt(&fmt->format, *ws, code); \
}

#define SENSOR_GET_FMT \
static int sensor_get_fmt(struct v4l2_subdev *sd, \
			  struct v4l2_subdev_fh *fh, \
			  struct v4l2_subdev_format *fmt) \
{ \
	struct sensor_info *info = to_state(sd); \
	const struct sensor_win_size *ws; \
	u32 code; \
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) { \
		fmt->format = *v4l2_subdev_get_try_format(fh, fmt->pad); \
		return 0; \
	} \
	mutex_lock(&info->lock); \
	switch (fmt->pad) { \
	case SENSOR_PAD_SOURCE: \
		code = info->fmt->mbus_code; \
		ws = info->current_wins; \
		break; \
	default: \
		mutex_unlock(&info->lock); \
		return -EINVAL; \
	} \
	sensor_fill_mbus_fmt(&fmt->format, ws, code); \
	mutex_unlock(&info->lock); \
	return 0; \
}

#define SENSOR_SET_FMT \
static int sensor_set_fmt(struct v4l2_subdev *sd, \
			  struct v4l2_subdev_fh *fh, \
			  struct v4l2_subdev_format *fmt) \
{ \
	struct sensor_win_size *ws = NULL; \
	struct sensor_format_struct *sf = NULL; \
	struct sensor_info *info = to_state(sd); \
	struct v4l2_mbus_framefmt *mf; \
	int ret = 0; \
	mutex_lock(&info->lock); \
	sensor_print("%s %d*%d 0x%x 0x%x\n", __func__, fmt->format.width, \
		  fmt->format.height, fmt->format.code, fmt->format.field); \
	sensor_try_format(info, fh, fmt, &ws, &sf); \
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) { \
		if (NULL == fh) { \
			sensor_err("fh is NULL when V4L2_SUBDEV_FORMAT_TRY\n"); \
			mutex_unlock(&info->lock); \
			return -EINVAL; \
		} \
		mf = v4l2_subdev_get_try_format(fh, fmt->pad); \
		*mf = fmt->format; \
	} else { \
		switch (fmt->pad) { \
		case SENSOR_PAD_SOURCE: \
			info->current_wins = ws; \
			info->fmt = sf; \
			break; \
		default: \
			ret = -EBUSY; \
		} \
	} \
	mutex_unlock(&info->lock); \
	return ret;\
}

#endif /*__CAMERA__H__*/

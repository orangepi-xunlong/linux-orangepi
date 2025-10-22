/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
  * sensor_helper.c: helper function for sensors.
  *
  * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
  *
  * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation.
  */

#include <linux/module.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <media/v4l2-dev.h>

#include "sensor_helper.h"
#include "camera.h"

struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}
EXPORT_SYMBOL_GPL(to_state);

static struct sensor_format_struct *sensor_find_mbus_code(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *fmt)
{
	struct sensor_info *info = to_state(sd);
	int i;

	for (i = 0; i < info->fmt_num; ++i) {
		if (info->fmt_pt[i].mbus_code == fmt->code)
			break;
	}
	if (i >= info->fmt_num)
		return info->fmt_pt;
	return &info->fmt_pt[i];
}
#if 0
static struct sensor_win_size *sensor_find_frame_size(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *ws = info->win_pt;
	struct sensor_win_size *best_ws = &ws[0];
	int best_dist = INT_MAX;
	int fps_flag = 0;
	int i;

	/*judge if sensor have wdr command win*/
	if (info->isp_wdr_mode == ISP_COMANDING_MODE) {
		for (i = 0; i < info->win_size_num; ++i) {
			if (ws->wdr_mode == ISP_COMANDING_MODE) {
				best_ws = ws;
				break;
			}
			++ws;
		}
		if (i == info->win_size_num)
			info->isp_wdr_mode = 0;
	}

	/*judge if sensor have wdr dol win*/
	ws = info->win_pt;
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		for (i = 0; i < info->win_size_num; ++i) {
			if (ws->wdr_mode == ISP_DOL_WDR_MODE) {
				best_ws = ws;
				break;
			}
			++ws;
		}
		if (i == info->win_size_num)
			info->isp_wdr_mode = 0;
	}

	/*judge if sensor have the right fps win*/
	ws = info->win_pt;
	if (info->isp_wdr_mode == ISP_COMANDING_MODE) {
		for (i = 0; i < info->win_size_num; ++i) {
			if ((ws->fps_fixed == info->tpf.denominator) &&
			   (ws->wdr_mode == ISP_COMANDING_MODE)) {
				best_ws = ws;
				fps_flag = 1;
				break;
			}
			++ws;
		}
	} else if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		for (i = 0; i < info->win_size_num; ++i) {
			if ((ws->fps_fixed == info->tpf.denominator) &&
			   (ws->wdr_mode == ISP_DOL_WDR_MODE)) {
				best_ws = ws;
				fps_flag = 1;
				break;
			}
			++ws;
		}
	} else {
		for (i = 0; i < info->win_size_num; ++i) {
			if ((ws->fps_fixed == info->tpf.denominator) &&
			   (ws->wdr_mode == ISP_NORMAL_MODE)) {
				best_ws = ws;
				fps_flag = 1;
				break;
			}
			++ws;
		}
	}

	/*judge if sensor have the right resoulution win*/
	ws = info->win_pt;
	if (info->isp_wdr_mode == ISP_COMANDING_MODE) {
		if (fps_flag) {
			for (i = 0; i < info->win_size_num; ++i) {
				int dist = abs(ws->width - fmt->width) +
				    abs(ws->height - fmt->height);

				if ((dist < best_dist) &&
				    (ws->width >= fmt->width) &&
				    (ws->height >= fmt->height) &&
				    (ws->wdr_mode == ISP_COMANDING_MODE) &&
				    (ws->fps_fixed == info->tpf.denominator)) {
					best_dist = dist;
					best_ws = ws;
				}
				++ws;
			}
		} else {
			for (i = 0; i < info->win_size_num; ++i) {
				int dist = abs(ws->width - fmt->width) +
				    abs(ws->height - fmt->height);

				if ((dist < best_dist) &&
				    (ws->width >= fmt->width) &&
				    (ws->height >= fmt->height) &&
				    (ws->wdr_mode == ISP_COMANDING_MODE)) {
					best_dist = dist;
					best_ws = ws;
				}
				++ws;
			}
		}
	} else if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		if (fps_flag) {
			for (i = 0; i < info->win_size_num; ++i) {
				int dist = abs(ws->width - fmt->width) +
				    abs(ws->height - fmt->height);

				if ((dist < best_dist) &&
				    (ws->width >= fmt->width) &&
				    (ws->height >= fmt->height) &&
				    (ws->wdr_mode == ISP_DOL_WDR_MODE) &&
				    (ws->fps_fixed == info->tpf.denominator)) {
					best_dist = dist;
					best_ws = ws;
				}
				++ws;
			}
		} else {
			for (i = 0; i < info->win_size_num; ++i) {
				int dist = abs(ws->width - fmt->width) +
				    abs(ws->height - fmt->height);

				if ((dist < best_dist) &&
				    (ws->width >= fmt->width) &&
				    (ws->height >= fmt->height) &&
				    (ws->wdr_mode == ISP_DOL_WDR_MODE)) {
					best_dist = dist;
					best_ws = ws;
				}
				++ws;
			}
		}
	} else {
		if (fps_flag) {
			for (i = 0; i < info->win_size_num; ++i) {
				int dist = abs(ws->width - fmt->width) +
				    abs(ws->height - fmt->height);

				if ((dist < best_dist) &&
				    (ws->width >= fmt->width) &&
				    (ws->height >= fmt->height) &&
				    (ws->wdr_mode == ISP_NORMAL_MODE) &&
				    (ws->fps_fixed == info->tpf.denominator)) {
					best_dist = dist;
					best_ws = ws;
				}
				++ws;
			}
		} else {
			for (i = 0; i < info->win_size_num; ++i) {
				int dist = abs(ws->width - fmt->width) +
				    abs(ws->height - fmt->height);

				if ((dist < best_dist) &&
				    (ws->width >= fmt->width) &&
				    (ws->height >= fmt->height) &&
				    (ws->wdr_mode == ISP_NORMAL_MODE)) {
					best_dist = dist;
					best_ws = ws;
				}
				++ws;
			}
		}
	}

	info->isp_wdr_mode = best_ws->wdr_mode;

	return best_ws;
}
#endif

static void sensor_fill_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *mf,
				const struct sensor_win_size *ws, u32 code)
{
//	struct sensor_info *info = to_state(sd);
//	struct mbus_framefmt_res *res = (void *)mf->reserved;

	mf->width = ws->width;
	mf->height = ws->height;
	mf->code = code;
/*	mf->field = info->sensor_field;
	res->res_mipi_bps = ws->mipi_bps;
	res->res_combo_mode = info->combo_mode | ws->if_mode;
	res->res_wdr_mode = ws->wdr_mode;
	res->res_lp_mode = ws->lp_mode;
	res->res_time_hs = info->time_hs;
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE && info->wdr_time_hs)
		res->res_time_hs = info->wdr_time_hs;
*/
}

static void sensor_try_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_format *fmt,
				struct sensor_win_size **ws,
				struct sensor_format_struct **sf)
{
	struct sensor_info *info = to_state(sd);
	u32 code = MEDIA_BUS_FMT_YUYV8_2X8;

	if ((sd->entity.stream_count > 0) &&
	    info->current_wins != NULL) {
		code = info->fmt->mbus_code;
		*ws = info->current_wins;
		*sf = info->fmt;
//		info->isp_wdr_mode = info->current_wins->wdr_mode;
	} else {
		*ws = info->current_wins;
		*sf = sensor_find_mbus_code(sd, &fmt->format);
		code = (*sf)->mbus_code;
	}
	sensor_fill_mbus_fmt(sd, &fmt->format, *ws, code);
}

int sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *ws = NULL;
	struct sensor_format_struct *sf = NULL;
//	struct v4l2_mbus_framefmt *mf;

	mutex_lock(&info->lock);
	vfe_print("%s %s %d*%d 0x%x 0x%x\n", sd->name, __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	sensor_try_format(sd, cfg, fmt, &ws, &sf);
	info->current_wins = ws;
	info->fmt = sf;
	mutex_unlock(&info->lock);

	return 0;
#if 0
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		if (cfg == NULL) {
			pr_err("%s cfg is NULL!\n", sd->name);
			mutex_unlock(&info->lock);
			return -EINVAL;
		}
		mf = &cfg->try_fmt;
		*mf = fmt->format;
	} else {
		switch (fmt->pad) {
		case SENSOR_PAD_SOURCE:
			info->current_wins = ws;
			info->fmt = sf;
			break;
		default:
			ret = -EBUSY;
		}
	}
	mutex_unlock(&info->lock);
	return ret;
#endif
}
EXPORT_SYMBOL_GPL(sensor_set_fmt);

int sensor_get_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
//	const struct sensor_win_size *ws;
//	u32 code;

	mutex_lock(&info->lock);
	fmt->format.code = info->fmt->mbus_code;
	mutex_unlock(&info->lock);

	return fmt->format.code;

#if 0
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		if (cfg == NULL) {
			pr_err("%s cfg is NULL!\n", sd->name);
			return -EINVAL;
		}
		fmt->format = cfg->try_fmt;
		return 0;
	}
	mutex_lock(&info->lock);
	switch (fmt->pad) {
	case SENSOR_PAD_SOURCE:
		code = info->fmt->mbus_code;
		ws = info->current_wins;
		break;
	default:
		mutex_unlock(&info->lock);
		return -EINVAL;
	}
	sensor_fill_mbus_fmt(sd, &fmt->format, ws, code);
	mutex_unlock(&info->lock);
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(sensor_get_fmt);

int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;

	cp->timeperframe.numerator = info->tpf.numerator;
	cp->timeperframe.denominator = info->tpf.denominator;

	return 0;
}
EXPORT_SYMBOL_GPL(sensor_g_parm);

int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (info->tpf.numerator == 0)
		return -EINVAL;

	info->capture_mode = cp->capturemode;

	return 0;
}
EXPORT_SYMBOL_GPL(sensor_s_parm);

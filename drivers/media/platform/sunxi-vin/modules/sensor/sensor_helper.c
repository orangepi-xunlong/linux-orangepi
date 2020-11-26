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

#include "camera.h"
#include "sensor_helper.h"

struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}
EXPORT_SYMBOL_GPL(to_state);

void sensor_cfg_req(struct v4l2_subdev *sd, struct sensor_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	if (info == NULL) {
		pr_err("%s is not initialized\n", sd->name);
		return;
	}
	if (info->current_wins == NULL) {
		pr_err("%s format is not initialized\n", sd->name);
		return;
	}

	cfg->width = info->current_wins->width_input;
	cfg->height = info->current_wins->height_input;
	cfg->hoffset = info->current_wins->hoffset;
	cfg->voffset = info->current_wins->voffset;
	cfg->hts = info->current_wins->hts;
	cfg->vts = info->current_wins->vts;
	cfg->pclk = info->current_wins->pclk;
	cfg->fps_fixed = info->current_wins->fps_fixed;
	cfg->bin_factor = info->current_wins->bin_factor;
	cfg->intg_min = info->current_wins->intg_min;
	cfg->intg_max = info->current_wins->intg_max;
	cfg->gain_min = info->current_wins->gain_min;
	cfg->gain_max = info->current_wins->gain_max;
	cfg->mbus_code = info->fmt->mbus_code;
	cfg->wdr_mode = info->current_wins->wdr_mode;
}
EXPORT_SYMBOL_GPL(sensor_cfg_req);

void sensor_isp_input(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
	struct sensor_info *info = to_state(sd);

	if (info == NULL) {
		pr_err("%s is not initialized\n", sd->name);
		return;
	}
	if (info->current_wins == NULL) {
		pr_err("%s format is not initialized\n", sd->name);
		return;
	}

	info->current_wins->width_input = mf->width;
	info->current_wins->height_input = mf->height;
}
EXPORT_SYMBOL_GPL(sensor_isp_input);

unsigned int sensor_get_exp(struct v4l2_subdev *sd)
{
	struct sensor_info *info = to_state(sd);
	unsigned int exp_us = 0;
	if (info == NULL) {
		pr_err("%s is not initialized\n", sd->name);
		return -1;
	}
	if (info->exp == 0) {
		return -1;
	}
	exp_us = info->exp/16*info->current_wins->hts/(info->current_wins->pclk/1000000);
	return exp_us;
}
EXPORT_SYMBOL_GPL(sensor_get_exp);

int sensor_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_mbus_code_enum *code)
{
	struct sensor_info *info = to_state(sd);

	if (code->index >= info->fmt_num)
		return -EINVAL;
	code->code = info->fmt_pt[code->index].mbus_code;
	return 0;
}
EXPORT_SYMBOL_GPL(sensor_enum_mbus_code);

int sensor_enum_frame_size(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_frame_size_enum *fse)
{
	struct sensor_info *info = to_state(sd);

	if (fse->index >= info->win_size_num)
		return -EINVAL;
	fse->min_width = info->win_pt[fse->index].width;
	fse->max_width = fse->min_width;
	fse->max_height = info->win_pt[fse->index].height;
	fse->min_height = fse->max_height;
	return 0;
}
EXPORT_SYMBOL_GPL(sensor_enum_frame_size);

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

static void sensor_fill_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *mf,
				const struct sensor_win_size *ws, u32 code)
{
	struct sensor_info *info = to_state(sd);
	struct mbus_framefmt_res *res = (void *)mf->reserved;

	mf->width = ws->width;
	mf->height = ws->height;
	mf->code = code;
	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field = info->sensor_field;
	res->res_mipi_bps = ws->mipi_bps;
	res->res_combo_mode = info->combo_mode | ws->if_mode;
	res->res_wdr_mode = ws->wdr_mode;
}

static void sensor_try_format(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_format *fmt,
				struct sensor_win_size **ws,
				struct sensor_format_struct **sf)
{
	struct sensor_info *info = to_state(sd);
	u32 code = MEDIA_BUS_FMT_YUYV8_2X8;

	if (fmt->pad == SENSOR_PAD_SOURCE) {
		if ((sd->entity.stream_count > 0 || info->use_current_win) &&
		    info->current_wins != NULL) {
			code = info->fmt->mbus_code;
			*ws = info->current_wins;
			*sf = info->fmt;
		} else {
			*ws = sensor_find_frame_size(sd, &fmt->format);
			*sf = sensor_find_mbus_code(sd, &fmt->format);
			code = (*sf)->mbus_code;
		}
	}
	sensor_fill_mbus_fmt(sd, &fmt->format, *ws, code);
}

int sensor_get_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
	const struct sensor_win_size *ws;
	u32 code;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		if (cfg == NULL) {
			pr_err("%s cfg is NULL!\n", sd->name);
			mutex_unlock(&info->lock);
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
}
EXPORT_SYMBOL_GPL(sensor_get_fmt);

int sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *ws = NULL;
	struct sensor_format_struct *sf = NULL;
	struct v4l2_mbus_framefmt *mf;
	int ret = 0;

	mutex_lock(&info->lock);
	vin_log(VIN_LOG_FMT, "%s %s %d*%d 0x%x 0x%x\n", sd->name, __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);
	sensor_try_format(sd, cfg, fmt, &ws, &sf);
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
}
EXPORT_SYMBOL_GPL(sensor_set_fmt);

int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;
	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;
	if (info->current_wins && info->current_wins->fps_fixed)
		cp->timeperframe.denominator = info->current_wins->fps_fixed;
	else
		cp->timeperframe.denominator = info->tpf.denominator;
	cp->timeperframe.numerator = 1;
	return 0;
}
EXPORT_SYMBOL_GPL(sensor_g_parm);

int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;
	if (info->tpf.numerator == 0)
		return -EINVAL;
	info->capture_mode = cp->capturemode;
	info->tpf = cp->timeperframe;
	info->use_current_win = cp->reserved[0];
	info->isp_wdr_mode = cp->reserved[1];
	return 0;
}
EXPORT_SYMBOL_GPL(sensor_s_parm);

int actuator_init(struct v4l2_subdev *sd, struct actuator_para *range)
{
	struct modules_config *modules = sd_to_modules(sd);
	struct v4l2_subdev *act_sd = NULL;
	struct actuator_para_t vcm_para;

	if (modules == NULL)
		return -EINVAL;

	act_sd = modules->modules.act[modules->sensors.valid_idx].sd;
	if (act_sd == NULL)
		return 0;

	if (act_sd->entity.use_count > 0)
		return 0;

	vcm_para.active_min = range->code_min;
	vcm_para.active_max = range->code_max;
	return v4l2_subdev_call(act_sd, core, ioctl, ACT_INIT, &vcm_para);
}
EXPORT_SYMBOL_GPL(actuator_init);

int actuator_set_code(struct v4l2_subdev *sd, struct actuator_ctrl *pos)
{
	struct modules_config *modules = sd_to_modules(sd);
	struct v4l2_subdev *act_sd = NULL;
	struct actuator_ctrl_word_t vcm_ctrl;

	if (modules == NULL)
		return -EINVAL;

	act_sd = modules->modules.act[modules->sensors.valid_idx].sd;
	if (act_sd == NULL)
		return 0;

	vcm_ctrl.code = pos->code;
	vcm_ctrl.sr = 0x0;
	return v4l2_subdev_call(act_sd, core, ioctl, ACT_SET_CODE, &vcm_ctrl);
}
EXPORT_SYMBOL_GPL(actuator_set_code);


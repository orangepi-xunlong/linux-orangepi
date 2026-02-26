/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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
#include "../utility/vin_log.h"
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/aw_rpmsg.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include "../platform/platform_cfg.h"
#include "sunxi_scaler.h"
#include "../vin-video/vin_core.h"

#define SCALER_MODULE_NAME "vin_scaler"

struct scaler_dev *glb_vipp[VIN_MAX_SCALER];

#define MIN_IN_WIDTH			192
#define MIN_IN_HEIGHT			128
#define MAX_IN_WIDTH			4224
#define MAX_IN_HEIGHT			4224

#define MIN_OUT_WIDTH			16
#define MIN_OUT_HEIGHT			10
#define MAX_OUT_WIDTH			4224
#define MAX_OUT_HEIGHT			4224

#define MIN_RATIO			256
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW15P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW16P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW17P1)
#define MAX_RATIO			2048
#else
#define MAX_RATIO			4096
#endif

static void __scaler_try_crop(const struct v4l2_mbus_framefmt *sink,
			    const struct v4l2_mbus_framefmt *source,
			    struct v4l2_rect *crop)
{
	/* Crop can not go beyond of the input rectangle */
	crop->left = clamp_t(u32, crop->left, 0, sink->width - source->width);
	crop->width = clamp_t(u32, crop->width, source->width, sink->width - crop->left);
	crop->top = clamp_t(u32, crop->top, 0, sink->height - source->height);
	crop->height = clamp_t(u32, crop->height, source->height, sink->height - crop->top);
}

static int __scaler_w_shift(int x_ratio, int y_ratio)
{
	int m, n;
	int sum_weight = 0;
	int weight_shift;
	int xr = (x_ratio >> 8) + 1;
	int yr = (y_ratio >> 8) + 1;

#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1) || IS_ENABLED(CONFIG_ARCH_SUN50IW10) || defined VIPP_200
	int weight_shift_bak = 0;

	weight_shift = -9;
	for (weight_shift = 17; weight_shift > 0; weight_shift--) {
		sum_weight = 0;
		for (m = 0; m <= xr; m++) {
			for (n = 0; n <= yr; n++) {
				sum_weight += (y_ratio - abs((n << 8) - (yr << 7)))*
					(x_ratio - abs((m << 8) - (xr << 7))) >> (weight_shift + 8);
			}
		}
		if (sum_weight > 0 && sum_weight < 256)
			weight_shift_bak = weight_shift;
		if (sum_weight > 255 && sum_weight < 486)
			break;
	}
	if (weight_shift == 0)
		weight_shift = weight_shift_bak;
#else
	weight_shift = -8;
	for (m = 0; m <= xr; m++) {
		for (n = 0; n <= yr; n++) {
			sum_weight += (y_ratio - abs((n << 8) - (yr << 7)))
				* (x_ratio - abs((m << 8) - (xr << 7)));
		}
	}
	sum_weight >>= 8;
	while (sum_weight != 0) {
		weight_shift++;
		sum_weight >>= 1;
	}
#endif
	return weight_shift;
}

static void __scaler_calc_ratios(struct scaler_dev *scaler,
			       struct v4l2_rect *input,
			       struct v4l2_mbus_framefmt *output,
			       struct scaler_para *para)
{
	unsigned int width;
	unsigned int height;
	unsigned int r_min;

	output->width = clamp_t(u32, output->width, MIN_OUT_WIDTH, input->width);
	output->height = clamp_t(u32, output->height, MIN_OUT_HEIGHT, input->height);

	para->xratio = 256 * input->width / output->width;
	para->yratio = 256 * input->height / output->height;
	para->xratio = clamp_t(u32, para->xratio, MIN_RATIO, MAX_RATIO);
	para->yratio = clamp_t(u32, para->yratio, MIN_RATIO, MAX_RATIO);

	r_min = min(para->xratio, para->yratio);
#ifdef CROP_AFTER_SCALER
	width = ALIGN(256 * input->width / r_min, 4);
	height = ALIGN(256 * input->height / r_min, 2);
	para->xratio = 256 * input->width / width;
	para->yratio = 256 * input->height / height;
	para->xratio = clamp_t(u32, para->xratio, MIN_RATIO, MAX_RATIO);
	para->yratio = clamp_t(u32, para->yratio, MIN_RATIO, MAX_RATIO);
	para->width = width;
	para->height = height;
	vin_log(VIN_LOG_SCALER, "para: xr = %d, yr = %d, w = %d, h = %d\n",
		  para->xratio, para->yratio, para->width, para->height);

	output->width = width;
	output->height = height;
#else
	if (scaler->large_image == 3)
		width = ALIGN(min(output->width * r_min / 256, input->width), 4);
	else
		width = ALIGN(min(output->width * r_min / 256, input->width), 2);
	height = ALIGN(min(output->height * r_min / 256, input->height), 2);
	para->xratio = 256 * width / output->width;
	para->yratio = 256 * height / output->height;
	para->xratio = clamp_t(u32, para->xratio, MIN_RATIO, MAX_RATIO);
	para->yratio = clamp_t(u32, para->yratio, MIN_RATIO, MAX_RATIO);
	para->width = output->width;
	para->height = output->height;
	vin_log(VIN_LOG_SCALER, "para: xr = %d, yr = %d, w = %d, h = %d\n",
		  para->xratio, para->yratio, para->width, para->height);
	/* Center the new crop rectangle.
	 * crop is before scaler
	 */
	input->left += (input->width - width) / 2;
	input->top += (input->height - height) / 2;
	input->left = ALIGN(input->left, 2);
	input->top = ALIGN(input->top, 1);
	input->width = width;
	input->height = height;
#endif
	vin_log(VIN_LOG_SCALER, "crop: left = %d, top = %d, w = %d, h = %d\n",
		input->left, input->top, input->width, input->height);
}

static void __scaler_try_format(struct scaler_dev *scaler,
			      struct v4l2_subdev_pad_config *cfg, unsigned int pad,
			      struct v4l2_mbus_framefmt *fmt,
			      enum v4l2_subdev_format_whence which)
{
	struct scaler_para ratio;

	switch (pad) {
	case SCALER_PAD_SINK:
		if (!scaler->large_image) {
			fmt->width =
			    clamp_t(u32, fmt->width, MIN_IN_WIDTH, MAX_IN_WIDTH);
			fmt->height =
			    clamp_t(u32, fmt->height, MIN_IN_HEIGHT, MAX_IN_HEIGHT);
		}
		break;
	case SCALER_PAD_SOURCE:
		fmt->code = scaler->formats[SCALER_PAD_SINK].code;
		__scaler_calc_ratios(scaler, &scaler->crop.request, fmt, &ratio);
		break;
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static int sunxi_scaler_subdev_get_fmt(struct v4l2_subdev *sd,
				       struct v4l2_subdev_state *state,
				       struct v4l2_subdev_format *fmt)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);

	fmt->format = scaler->formats[fmt->pad];

	return 0;
}

static int sunxi_scaler_subdev_set_fmt(struct v4l2_subdev *sd,
				       struct v4l2_subdev_state *state,
				       struct v4l2_subdev_format *fmt)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format = &scaler->formats[fmt->pad];

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__, fmt->format.width,
		  fmt->format.height, fmt->format.code, fmt->format.field);
	__scaler_try_format(scaler, state->pads, fmt->pad, &fmt->format, fmt->which);
	*format = fmt->format;

	if (fmt->pad == SCALER_PAD_SINK) {
		/* reset crop rectangle */
		scaler->crop.request.left = 0;
		scaler->crop.request.top = 0;
		scaler->crop.request.width = fmt->format.width;
		scaler->crop.request.height = fmt->format.height;

		/* Propagate the format from sink to source */
		format = &scaler->formats[SCALER_PAD_SOURCE];
		*format = fmt->format;
		__scaler_try_format(scaler, state->pads, SCALER_PAD_SOURCE, format,
				  fmt->which);
	}

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
		scaler->crop.active = scaler->crop.request;
		__scaler_calc_ratios(scaler, &scaler->crop.active, format,
				   &scaler->para);
	}

	/* return the format for other subdev */
	fmt->format = *format;

	return 0;
}

static int sunxi_scaler_subdev_get_selection(struct v4l2_subdev *sd,
					     struct v4l2_subdev_state *state,
					     struct v4l2_subdev_selection *sel)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_source;
	struct v4l2_mbus_framefmt *format_sink;

	vin_log(VIN_LOG_SCALER, "%s\n", __func__);

	if (sel->pad != SCALER_PAD_SINK)
		return -EINVAL;

	format_sink = &scaler->formats[SCALER_PAD_SINK];
	format_source = &scaler->formats[SCALER_PAD_SOURCE];

	switch (sel->target) {
	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = INT_MAX;
		sel->r.height = INT_MAX;
		__scaler_try_crop(format_sink, format_source, &sel->r);
		break;
	case V4L2_SEL_TGT_CROP:
		sel->r = scaler->crop.request;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_scaler_subdev_set_selection(struct v4l2_subdev *sd,
					     struct v4l2_subdev_state *state,
					     struct v4l2_subdev_selection *sel)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_sink, *format_source;
	struct scaler_para para;
	struct vipp_scaler_config scaler_cfg;
	struct vipp_crop crop;

	if (scaler->noneed_register)
		return 0;
	if (sel->target != V4L2_SEL_TGT_CROP || sel->pad != SCALER_PAD_SINK)
		return -EINVAL;

	format_sink = &scaler->formats[SCALER_PAD_SINK];
	format_source = &scaler->formats[SCALER_PAD_SOURCE];

	if (sel->r.width == 0 || sel->r.height == 0) {
		sel->r.left = clamp_t(u32, sel->r.left, 0, format_sink->width - format_source->width);
		if (sel->r.left > ((format_sink->width - format_source->width) / 2))
			sel->r.width = format_sink->width - sel->r.left;
		else
			sel->r.width = format_sink->width - 2 * sel->r.left;

		sel->r.top = clamp_t(u32, sel->r.top, 0, format_sink->height - format_source->height);
		if (sel->r.top > ((format_sink->height - format_source->height) / 2))
			sel->r.height = format_sink->height - sel->r.top;
		else
			sel->r.height = format_sink->height - 2 * sel->r.top;
	}

	vin_log(VIN_LOG_FMT, "%s: L = %d, T = %d, W = %d, H = %d\n", __func__,
		  sel->r.left, sel->r.top, sel->r.width, sel->r.height);

	vin_log(VIN_LOG_FMT, "%s: input = %dx%d, output = %dx%d\n", __func__,
		  format_sink->width, format_sink->height,
		  format_source->width, format_source->height);

	__scaler_try_crop(format_sink, format_source, &sel->r);

	if (sel->reserved[0] != VIPP_ONLY_SHRINK) { /* vipp crop */
		__scaler_calc_ratios(scaler, &sel->r, format_source, &para);

		if (sel->which == V4L2_SUBDEV_FORMAT_TRY)
			return 0;

		scaler->para = para;
		scaler->crop.active = sel->r;
		scaler->crop.request = sel->r;

#if !defined VIPP_200
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
		if (scaler->stream_count)
#else
		if (sd->entity.stream_count)
#endif
			vipp_set_para_ready(scaler->id, NOT_READY);
#endif
		/* we need update register when streaming */
		crop.hor = scaler->crop.active.left;
		crop.ver = scaler->crop.active.top;
		crop.width = scaler->crop.active.width;
		crop.height = scaler->crop.active.height;

		if (scaler->large_image == 3) {
			if (format_sink->width / 2 < scaler->crop.active.left) {
				vin_err("vipp crop left is greater than the half of width\n");
				return -1;
			}
			crop.width = scaler->crop.active.width/2;
			if (scaler->id % 4 == 0)
				crop.hor = format_sink->width/2 - crop.width;
			else
				crop.hor = vin_set_large_overlayer(format_sink->width);
		}
		vipp_set_crop(scaler->id, &crop);

		if (scaler->id < MAX_OSD_NUM)
			scaler_cfg.sc_out_fmt = YUV422;
		else
			scaler_cfg.sc_out_fmt = YUV420;
		scaler_cfg.sc_x_ratio = scaler->para.xratio;
		scaler_cfg.sc_y_ratio = scaler->para.yratio;
		scaler_cfg.sc_w_shift = __scaler_w_shift(scaler->para.xratio, scaler->para.yratio);
		vipp_scaler_cfg(scaler->id, &scaler_cfg);

#if !defined VIPP_200
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
		if (scaler->stream_count)
#else
		if (sd->entity.stream_count)
#endif
			vipp_set_para_ready(scaler->id, HAS_READY);
#endif
		vin_log(VIN_LOG_SCALER, "active crop: l = %d, t = %d, w = %d, h = %d\n",
			scaler->crop.active.left, scaler->crop.active.top,
			scaler->crop.active.width, scaler->crop.active.height);
	} else { /* vipp shrink */
		if ((sel->r.width != format_source->width) || (sel->r.height != format_source->height)) {
			vin_err("vipp shrink size is not equal to output size");
			return -EINVAL;
		}

		if (sel->r.width % 16 != 0)
			vin_warn("please make sure width is a multiples of 16");
		scaler->crop.active.left = sel->r.left;
		scaler->crop.active.top = sel->r.top;
		scaler->crop.active.width = format_sink->width;
		scaler->crop.active.height = format_sink->height;
		scaler->para.xratio = clamp_t(u32, scaler->crop.active.width * 256 / sel->r.width, MIN_RATIO, MAX_RATIO);
		scaler->para.yratio = clamp_t(u32, scaler->crop.active.height * 256 / sel->r.height, MIN_RATIO, MAX_RATIO);

		vin_log(VIN_LOG_SCALER, "active shrink: l = %d, t = %d, w = %d, h = %d, xratio = %d, yratio = %d\n",
			scaler->crop.active.left, scaler->crop.active.top,
			scaler->crop.active.width, scaler->crop.active.height,
			scaler->para.xratio, scaler->para.yratio);
	}

#if defined VIPP_200
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
		if (scaler->stream_count) {
#else
		if (sd->entity.stream_count) {
#endif
		vipp_clear_status(scaler->id,
			CHN0_REG_LOAD_PD << (vipp_virtual_find_ch[scaler->id] * VIPP_CHN_INT_AMONG_OFFSET));
		vipp_irq_enable(scaler->id,
			CHN0_REG_LOAD_EN << (vipp_virtual_find_ch[scaler->id] * VIPP_CHN_INT_AMONG_OFFSET));
	}
#endif

	return 0;
}

#else /* before linux-5.15 */
static int sunxi_scaler_subdev_get_fmt(struct v4l2_subdev *sd,
				       struct v4l2_subdev_pad_config *cfg,
				       struct v4l2_subdev_format *fmt)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);

	fmt->format = scaler->formats[fmt->pad];

	return 0;
}

static int sunxi_scaler_subdev_set_fmt(struct v4l2_subdev *sd,
				       struct v4l2_subdev_pad_config *cfg,
				       struct v4l2_subdev_format *fmt)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format = &scaler->formats[fmt->pad];

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__, fmt->format.width,
		  fmt->format.height, fmt->format.code, fmt->format.field);
	__scaler_try_format(scaler, cfg, fmt->pad, &fmt->format, fmt->which);
	*format = fmt->format;

	if (fmt->pad == SCALER_PAD_SINK) {
		/* reset crop rectangle */
		scaler->crop.request.left = 0;
		scaler->crop.request.top = 0;
		scaler->crop.request.width = fmt->format.width;
		scaler->crop.request.height = fmt->format.height;

		/* Propagate the format from sink to source */
		format = &scaler->formats[SCALER_PAD_SOURCE];
		*format = fmt->format;
		__scaler_try_format(scaler, cfg, SCALER_PAD_SOURCE, format,
				  fmt->which);
	}

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
		scaler->crop.active = scaler->crop.request;
		__scaler_calc_ratios(scaler, &scaler->crop.active, format,
				   &scaler->para);
	}

	/* return the format for other subdev */
	fmt->format = *format;

	return 0;
}

static int sunxi_scaler_subdev_get_selection(struct v4l2_subdev *sd,
					     struct v4l2_subdev_pad_config *cfg,
					     struct v4l2_subdev_selection *sel)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_source;
	struct v4l2_mbus_framefmt *format_sink;

	vin_log(VIN_LOG_SCALER, "%s\n", __func__);

	if (sel->pad != SCALER_PAD_SINK)
		return -EINVAL;

	format_sink = &scaler->formats[SCALER_PAD_SINK];
	format_source = &scaler->formats[SCALER_PAD_SOURCE];

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = INT_MAX;
		sel->r.height = INT_MAX;
		__scaler_try_crop(format_sink, format_source, &sel->r);
		break;
	case V4L2_SEL_TGT_CROP:
		sel->r = scaler->crop.request;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_scaler_subdev_set_selection(struct v4l2_subdev *sd,
					     struct v4l2_subdev_pad_config *cfg,
					     struct v4l2_subdev_selection *sel)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format_sink, *format_source;
	struct scaler_para para;
	struct vipp_scaler_config scaler_cfg;
	struct vipp_crop crop;

	if (sel->target != V4L2_SEL_TGT_CROP || sel->pad != SCALER_PAD_SINK)
		return -EINVAL;

	format_sink = &scaler->formats[SCALER_PAD_SINK];
	format_source = &scaler->formats[SCALER_PAD_SOURCE];

	vin_log(VIN_LOG_FMT, "%s: L = %d, T = %d, W = %d, H = %d\n", __func__,
		  sel->r.left, sel->r.top, sel->r.width, sel->r.height);

	vin_log(VIN_LOG_FMT, "%s: input = %dx%d, output = %dx%d\n", __func__,
		  format_sink->width, format_sink->height,
		  format_source->width, format_source->height);

	__scaler_try_crop(format_sink, format_source, &sel->r);

	if (sel->reserved[0] != VIPP_ONLY_SHRINK) { /* vipp crop */
	__scaler_calc_ratios(scaler, &sel->r, format_source, &para);

	if (sel->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	scaler->para = para;
	scaler->crop.active = sel->r;
	scaler->crop.request = sel->r;

		if (sd->entity.stream_count)
			vipp_set_para_ready(scaler->id, NOT_READY);
	/* we need update register when streaming */
	crop.hor = scaler->crop.active.left;
	crop.ver = scaler->crop.active.top;
	crop.width = scaler->crop.active.width;
	crop.height = scaler->crop.active.height;
	vipp_set_crop(scaler->id, &crop);

	if (scaler->id < MAX_OSD_NUM)
		scaler_cfg.sc_out_fmt = YUV422;
	else
		scaler_cfg.sc_out_fmt = YUV420;
	scaler_cfg.sc_x_ratio = scaler->para.xratio;
	scaler_cfg.sc_y_ratio = scaler->para.yratio;
	scaler_cfg.sc_w_shift = __scaler_w_shift(scaler->para.xratio, scaler->para.yratio);
	vipp_scaler_cfg(scaler->id, &scaler_cfg);

		if (sd->entity.stream_count)
			vipp_set_para_ready(scaler->id, HAS_READY);
	vin_log(VIN_LOG_SCALER, "active crop: l = %d, t = %d, w = %d, h = %d\n",
		scaler->crop.active.left, scaler->crop.active.top,
		scaler->crop.active.width, scaler->crop.active.height);
	} else { /* vipp shrink */
		if ((sel->r.width != format_source->width) || (sel->r.height != format_source->height)) {
			vin_err("vipp shrink size is not equal to output size");
			return -EINVAL;
		}

		scaler->crop.active.left = sel->r.left;
		scaler->crop.active.top = sel->r.top;
		scaler->crop.active.width = format_sink->width;
		scaler->crop.active.height = format_sink->height;
		scaler->para.xratio = scaler->crop.active.width * 256 / sel->r.width;
		scaler->para.yratio = scaler->crop.active.height * 256 / sel->r.height;

		vin_log(VIN_LOG_SCALER, "active shrink: l = %d, t = %d, w = %d, h = %d, xratio = %d, yratio = %d\n",
			scaler->crop.active.left, scaler->crop.active.top,
			scaler->crop.active.width, scaler->crop.active.height,
			scaler->para.xratio, scaler->para.yratio);
	}
	return 0;
}
#endif

int sunxi_scaler_subdev_init(struct v4l2_subdev *sd, u32 val)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);

	if (scaler->noneed_register)
		return 0;

	vin_log(VIN_LOG_SCALER, "%s, val = %d.\n", __func__, val);
	if (val) {
		if (!scaler->delay_para_ready) {
			memset(scaler->vipp_reg.vir_addr, 0, scaler->vipp_reg.size);
#if !defined VIPP_200
			vipp_set_reg_load_addr(scaler->id, (unsigned long)scaler->vipp_reg.dma_addr);
#else
			scaler->load_select = true;
			vipp_set_reg_load_addr(scaler->id, (unsigned long)scaler->load_para[0].dma_addr);
#endif
			vipp_set_osd_para_load_addr(scaler->id, (unsigned long)scaler->osd_para.dma_addr);
			vipp_set_osd_stat_load_addr(scaler->id, (unsigned long)scaler->osd_stat.dma_addr);
			vipp_set_osd_cv_update(scaler->id, NOT_UPDATED);
			vipp_set_osd_ov_update(scaler->id, NOT_UPDATED);
			vipp_set_para_ready(scaler->id, NOT_READY);
		}
	}
	return 0;
}

#if defined VIPP_200
static void vin_streamoff_logic_isp(struct scaler_dev *logic_scaler, unsigned char virtual_id)
{
#if defined SUPPORT_ISP_TDM && defined TDM_V200
	struct vin_md *vind = dev_get_drvdata(logic_scaler->subdev.v4l2_dev->dev);
	struct vin_core *vinc = vind->vinc[virtual_id];
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct tdm_rx_dev *tdm_rx = NULL;
	struct tdm_dev *tdm = NULL;

	if (vinc->tdm_rx_sel != 0xff) {
		tdm_rx = container_of(cap->pipe.sd[VIN_IND_TDM_RX], struct tdm_rx_dev, subdev);
		tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);

		if (tdm->stream_cnt == 0) {
			bsp_isp_top_capture_stop(tdm->id);
			bsp_isp_enable(tdm->id, 0);
			/* bsp_isp_sram_boot_mode_ctrl(tdm->id, SRAM_BOOT_MODE); */

			csic_tdm_set_speed_dn(tdm->id, 0);
			csic_tdm_tx_cap_disable(tdm->id);
			csic_tdm_tx_disable(tdm->id);
			csic_tdm_disable(tdm->id);
			csic_tdm_top_disable(tdm->id);
			sunxi_tdm_buffer_free(tdm->id);
		}
	}
#endif
}
#endif

static int sunxi_scaler_logic_s_stream(unsigned char virtual_id, int on)
{
#if defined VIPP_200
	unsigned char logic_id = vipp_virtual_find_logic[virtual_id];
	struct scaler_dev *logic_scaler = glb_vipp[logic_id];
	int i;

	if (logic_scaler->work_mode == VIPP_ONLINE && virtual_id != logic_id) {
		vin_err("vipp%d work on online mode, vipp%d cannot to work!!\n", logic_id, virtual_id);
		return -1;
	}

	if (on && (logic_scaler->logic_top_stream_count)++ > 0)
		return 0;
	else if (!on && (logic_scaler->logic_top_stream_count == 0 || --(logic_scaler->logic_top_stream_count) > 0))
		return 0;

	if (on) {
		vipp_cap_disable(logic_scaler->id);
		for (i = 0; i < VIPP_VIRT_NUM; i++)
			vipp_chn_cap_disable(logic_scaler->id);

		vipp_work_mode(logic_scaler->id, logic_scaler->work_mode);
		vipp_top_clk_en(logic_scaler->id, on);
		vipp_clear_status(logic_scaler->id, VIPP_STATUS_ALL);
#if VIN_FALSE
		vipp_irq_enable(logic_scaler->id, ID_LOST_EN | AHB_MBUS_W_EN |
				CHN0_REG_LOAD_EN | CHN0_FRAME_LOST_EN | CHN0_HBLANK_SHORT_EN | CHN0_PARA_NOT_READY_EN |
				CHN1_REG_LOAD_EN | CHN1_FRAME_LOST_EN | CHN1_HBLANK_SHORT_EN | CHN1_PARA_NOT_READY_EN |
				CHN2_REG_LOAD_EN | CHN2_FRAME_LOST_EN | CHN2_HBLANK_SHORT_EN | CHN2_PARA_NOT_READY_EN |
				CHN3_REG_LOAD_EN | CHN3_FRAME_LOST_EN | CHN3_HBLANK_SHORT_EN | CHN3_PARA_NOT_READY_EN);
#else
		vipp_irq_enable(logic_scaler->id, ID_LOST_EN | AHB_MBUS_W_EN |
				CHN0_FRAME_LOST_EN | CHN0_HBLANK_SHORT_EN | CHN0_PARA_NOT_READY_EN | CHN0_HSHORT_INT_EN | CHN0_VSHORT_INT_EN |
				CHN1_FRAME_LOST_EN | CHN1_HBLANK_SHORT_EN | CHN1_PARA_NOT_READY_EN | CHN1_HSHORT_INT_EN | CHN1_VSHORT_INT_EN |
				CHN2_FRAME_LOST_EN | CHN2_HBLANK_SHORT_EN | CHN2_PARA_NOT_READY_EN | CHN2_HSHORT_INT_EN | CHN2_VSHORT_INT_EN |
				CHN3_FRAME_LOST_EN | CHN3_HBLANK_SHORT_EN | CHN3_PARA_NOT_READY_EN | CHN3_HSHORT_INT_EN | CHN3_VSHORT_INT_EN);
#endif
		vipp_cap_enable(logic_scaler->id);
	} else {
		if (logic_scaler->work_mode == VIPP_ONLINE) {
			vipp_cap_disable(logic_scaler->id);
			vipp_top_clk_en(logic_scaler->id, on);
		}
		vipp_cap_disable(logic_scaler->id);
		vipp_clear_status(logic_scaler->id, VIPP_STATUS_ALL);
		vipp_irq_disable(logic_scaler->id, VIPP_EN_ALL);

		if (logic_scaler->work_mode == VIPP_OFFLINE) {
			vin_streamoff_logic_isp(logic_scaler, virtual_id);
		}
	}
#endif
	return 0;
}

static int sunxi_scaler_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &scaler->formats[SCALER_PAD_SOURCE];
	struct mbus_framefmt_res *res = (void *)scaler->formats[SCALER_PAD_SOURCE].reserved;
	struct vipp_scaler_config scaler_cfg;
	struct vipp_scaler_size scaler_size;
	struct vipp_crop crop;
	enum vipp_format out_fmt;
	enum vipp_format sc_fmt;

#if defined VIPP_200
	if (scaler->noneed_register) {
		if (enable && glb_vipp[vipp_virtual_find_logic[scaler->id]]->work_mode == VIPP_ONLINE && vipp_virtual_find_logic[scaler->id] != scaler->id) {
			vin_err("vipp%d work on online mode, vipp%d cannot to work!!\n", vipp_virtual_find_logic[scaler->id], scaler->id);
		}
		return -1;
	}
#endif

	switch (res->res_pix_fmt) {
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
	case V4L2_PIX_FMT_SBGGR14:
	case V4L2_PIX_FMT_SGBRG14:
	case V4L2_PIX_FMT_SGRBG14:
	case V4L2_PIX_FMT_SRGGB14:
	case V4L2_PIX_FMT_SBGGR16:
	case V4L2_PIX_FMT_SGBRG16:
	case V4L2_PIX_FMT_SGRBG16:
	case V4L2_PIX_FMT_SRGGB16:
#if IS_ENABLED(CONFIG_SENSOR_RGB)
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB565:
#endif
		vin_log(VIN_LOG_FMT, "%s output fmt is raw, return directly\n", __func__);
		return 0;
	default:
		break;
	}
	if (mf->field == V4L2_FIELD_INTERLACED || mf->field == V4L2_FIELD_TOP ||
	    mf->field == V4L2_FIELD_BOTTOM) {
		vin_log(VIN_LOG_SCALER, "Scaler not support field mode, return directly!\n");
		return 0;
	}

	if (enable) {
#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
		if (scaler->delay_para_ready) {
			memset(scaler->vipp_reg.vir_addr, 0, scaler->vipp_reg.size);
#if !defined VIPP_200
			vipp_set_reg_load_addr(scaler->id, (unsigned long)scaler->vipp_reg.dma_addr);
#else
			scaler->load_select = true;
			vipp_set_reg_load_addr(scaler->id, (unsigned long)scaler->load_para[0].dma_addr);
#endif
			vipp_set_osd_para_load_addr(scaler->id, (unsigned long)scaler->osd_para.dma_addr);
			vipp_set_osd_stat_load_addr(scaler->id, (unsigned long)scaler->osd_stat.dma_addr);
			vipp_set_osd_cv_update(scaler->id, NOT_UPDATED);
			vipp_set_osd_ov_update(scaler->id, NOT_UPDATED);
			vipp_set_para_ready(scaler->id, NOT_READY);
			scaler->delay_para_ready = 0;
		}
#endif
		if (sunxi_scaler_logic_s_stream(scaler->id, enable))
			return -1;

		crop.hor = scaler->crop.active.left;
		crop.ver = scaler->crop.active.top;
		crop.width = scaler->crop.active.width;
		crop.height = scaler->crop.active.height;
		scaler_size.sc_width = scaler->para.width;
		scaler_size.sc_height = scaler->para.height;
		if (scaler->large_image == 3) {
			vin_log(VIN_LOG_SCALER, "vipp%d before change crop: left = %d, top = %d, w = %d, h = %d\n",
				scaler->id, crop.hor, crop.ver, crop.width, crop.height);
			vin_log(VIN_LOG_SCALER, "vipp%d before change scaler: xr = %d, yr = %d, w = %d, h = %d\n",
			scaler->id, scaler->para.xratio, scaler->para.yratio,
			scaler_size.sc_width, scaler_size.sc_height);
			if (scaler->crop.active.width / 2 <= scaler->crop.active.left) {
				vin_err("vipp crop left is greater than the half of width\n");
				return -1;
			}
			if (scaler->id % 4 == 0)
				crop.hor = scaler->crop.active.left;
			else
				crop.hor = vin_set_large_overlayer(scaler->crop.active.width);

			crop.width = scaler->crop.active.width/2;
			scaler_size.sc_width = scaler->para.width/2;
			scaler->para.width /= 2;
			vin_log(VIN_LOG_SCALER, "vipp%d after change crop: left = %d, top = %d, w = %d, h = %d\n",
				scaler->id, crop.hor, crop.ver, crop.width, crop.height);
			vin_log(VIN_LOG_SCALER, "vipp%d after change scaler: xr = %d, yr = %d, w = %d, h = %d\n",
			scaler->id, scaler->para.xratio, scaler->para.yratio,
			scaler_size.sc_width, scaler_size.sc_height);
		}
		vipp_set_crop(scaler->id, &crop);
		vipp_scaler_output_size(scaler->id, &scaler_size);

		switch (res->res_pix_fmt) {
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YUV420M:
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YVU420M:
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV21M:
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_FBC:
		case V4L2_PIX_FMT_LBC_2_0X:
		case V4L2_PIX_FMT_LBC_2_5X:
		case V4L2_PIX_FMT_LBC_1_5X:
		case V4L2_PIX_FMT_LBC_1_0X:
			if (scaler->id < MAX_OSD_NUM) {
				sc_fmt = YUV422;
				out_fmt = YUV420;
				vipp_chroma_ds_en(scaler->id, 1);
			} else {
				sc_fmt = YUV420;
				out_fmt = YUV420;
			}
			break;
		case V4L2_PIX_FMT_YUV422P:
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV61:
		case V4L2_PIX_FMT_NV61M:
		case V4L2_PIX_FMT_NV16M:
			sc_fmt = YUV422;
			out_fmt = YUV422;
			break;
#if defined VIPP_200 && IS_ENABLED(CONFIG_VIPP_YUV2RGB)
		case V4L2_PIX_FMT_RGB24:
		case V4L2_PIX_FMT_BGR24:
			sc_fmt = YUV422;
			out_fmt = YUV2RGB888;
			break;
		case V4L2_PIX_FMT_RGB565:
			sc_fmt = YUV422;
			out_fmt = YUV2RGB565;
			break;
#endif
		default:
			sc_fmt = YUV420;
			out_fmt = YUV420;
			break;
		}
		scaler_cfg.sc_out_fmt = sc_fmt;
		scaler_cfg.sc_x_ratio = scaler->para.xratio;
		scaler_cfg.sc_y_ratio = scaler->para.yratio;
		scaler_cfg.sc_w_shift = __scaler_w_shift(scaler->para.xratio, scaler->para.yratio);
		vipp_scaler_cfg(scaler->id, &scaler_cfg);
		vipp_output_fmt_cfg(scaler->id, out_fmt);
#if defined VIPP_200 && IS_ENABLED(CONFIG_VIPP_YUV2RGB)
		if (res->res_pix_fmt == V4L2_PIX_FMT_RGB24 || \
			res->res_pix_fmt == V4L2_PIX_FMT_BGR24 || \
			res->res_pix_fmt == V4L2_PIX_FMT_RGB565) {
#if IS_ENABLED(CONFIG_ARCH_SUN60IW1)
			if (vipp_virtual_find_logic[scaler->id] == 4 || \
				vipp_virtual_find_logic[scaler->id] == 16) {
				vipp_yuv2rgb_en(scaler->id, 1);
				vipp_yuv2rgb_coef_cfg(scaler->id);
			} else
				vin_log(VIN_LOG_FMT, "only vipp1/7 support yuv2rgb!\n");
#else
			vipp_yuv2rgb_en(scaler->id, 1);
			vipp_yuv2rgb_coef_cfg(scaler->id);
#endif
		}
#endif
		vipp_scaler_en(scaler->id, 1);
		vipp_osd_en(scaler->id, 1);
		vipp_set_osd_ov_update(scaler->id, HAS_UPDATED);
		vipp_set_osd_cv_update(scaler->id, HAS_UPDATED);
#if !defined VIPP_200
		vipp_set_para_ready(scaler->id, HAS_READY);
		vipp_top_clk_en(scaler->id, enable);
		vipp_enable(scaler->id);
#else
		memcpy(scaler->load_para[0].vir_addr, scaler->vipp_reg.vir_addr, VIPP_REG_SIZE);
		vipp_set_para_ready(scaler->id, HAS_READY);
		vipp_chn_cap_enable(scaler->id);
#endif
	} else {
#if !defined VIPP_200
		vipp_disable(scaler->id);
		vipp_top_clk_en(scaler->id, enable);
		vipp_set_para_ready(scaler->id, NOT_READY);
#else
		vipp_chn_cap_disable(scaler->id);
#if VIN_FALSE
		/* set chn_cap_en(bit0) to 0, but read it is 1 until frame is processed */
		vipp_chn_cap_disable(scaler->id);
		/* set para_ready(ready) to 0, read chn_cap_en(bit0) is 1, and than set it lead to hn_cap_en(bit0) is 1*/
		vipp_set_para_ready(scaler->id, NOT_READY);
#else
		vipp_chn_cap_disable_para_notready(scaler->id);
#endif
#endif
		vipp_chroma_ds_en(scaler->id, 0);
		vipp_osd_en(scaler->id, 0);
		vipp_scaler_en(scaler->id, 0);
#if defined VIPP_200 && IS_ENABLED(CONFIG_VIPP_YUV2RGB)
		if (res->res_pix_fmt == V4L2_PIX_FMT_RGB24 || \
			res->res_pix_fmt == V4L2_PIX_FMT_BGR24 || \
			res->res_pix_fmt == V4L2_PIX_FMT_RGB565)
#if IS_ENABLED(CONFIG_ARCH_SUN60IW1)
			if (vipp_virtual_find_logic[scaler->id] == 4 || \
				vipp_virtual_find_logic[scaler->id] == 16)
#endif
				vipp_yuv2rgb_en(scaler->id, 0);
#endif
		vipp_set_osd_ov_update(scaler->id, NOT_UPDATED);
		vipp_set_osd_cv_update(scaler->id, NOT_UPDATED);
		sunxi_scaler_logic_s_stream(scaler->id, enable);
	}
	vin_log(VIN_LOG_FMT, "vipp%d %s, %d*%d hoff: %d voff: %d xr: %d yr: %d\n",
		scaler->id, enable ? "stream on" : "stream off",
		scaler->para.width, scaler->para.height,
		scaler->crop.active.left, scaler->crop.active.top,
		scaler->para.xratio, scaler->para.yratio);

	return 0;
}

int sunxi_scaler_subdev_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct v4l2_captureparm *cp = &param->parm.capture;
	struct scaler_dev *scaler = v4l2_get_subdevdata(sd);

	scaler->large_image = cp->reserved[2];
	return 0;
}

static const struct v4l2_subdev_core_ops sunxi_scaler_subdev_core_ops = {
	.init = sunxi_scaler_subdev_init,
};
static const struct v4l2_subdev_video_ops sunxi_scaler_subdev_video_ops = {
	.s_stream = sunxi_scaler_subdev_s_stream,
};
/* subdev pad operations */
static const struct v4l2_subdev_pad_ops sunxi_scaler_subdev_pad_ops = {
	.get_fmt = sunxi_scaler_subdev_get_fmt,
	.set_fmt = sunxi_scaler_subdev_set_fmt,
	.get_selection = sunxi_scaler_subdev_get_selection,
	.set_selection = sunxi_scaler_subdev_set_selection,
};

static struct v4l2_subdev_ops sunxi_scaler_subdev_ops = {
	.core = &sunxi_scaler_subdev_core_ops,
	.video = &sunxi_scaler_subdev_video_ops,
	.pad = &sunxi_scaler_subdev_pad_ops,
};

int __scaler_init_subdev(struct scaler_dev *scaler)
{
	int ret;
	struct v4l2_subdev *sd = &scaler->subdev;

	mutex_init(&scaler->subdev_lock);

	v4l2_subdev_init(sd, &sunxi_scaler_subdev_ops);
	sd->grp_id = VIN_GRP_ID_SCALER;
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_scaler.%u", scaler->id);
	v4l2_set_subdevdata(sd, scaler);

	/* sd->entity->ops = &isp_media_ops; */
	scaler->scaler_pads[SCALER_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	scaler->scaler_pads[SCALER_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_PROC_VIDEO_SCALER;

	ret = media_entity_pads_init(&sd->entity, SCALER_PAD_NUM,
			scaler->scaler_pads);
	if (ret < 0)
		return ret;
	return 0;
}

static int scaler_resource_alloc(struct scaler_dev *scaler)
{
	int ret = 0;
#if !defined VIPP_200
	scaler->vipp_reg.size = VIPP_REG_SIZE + OSD_PARA_SIZE + OSD_STAT_SIZE;

	ret = os_mem_alloc(&scaler->pdev->dev, &scaler->vipp_reg);
	if (ret < 0) {
		vin_err("scaler regs load addr requset failed!\n");
		return -ENOMEM;
	}

	scaler->osd_para.dma_addr = scaler->vipp_reg.dma_addr + VIPP_REG_SIZE;
	scaler->osd_para.vir_addr = scaler->vipp_reg.vir_addr + VIPP_REG_SIZE;
	scaler->osd_stat.dma_addr = scaler->osd_para.dma_addr + OSD_PARA_SIZE;
	scaler->osd_stat.vir_addr = scaler->osd_para.vir_addr + OSD_PARA_SIZE;
#else
	scaler->vipp_reg.size = 3 * VIPP_REG_SIZE;

	ret = os_mem_alloc(&scaler->pdev->dev, &scaler->vipp_reg);
	if (ret < 0) {
		vin_err("scaler regs load addr requset failed!\n");
		return -ENOMEM;
	}

	scaler->load_para[0].dma_addr = scaler->vipp_reg.dma_addr + VIPP_REG_SIZE;
	scaler->load_para[0].vir_addr = scaler->vipp_reg.vir_addr + VIPP_REG_SIZE;
	scaler->load_para[1].dma_addr = scaler->vipp_reg.dma_addr + VIPP_REG_SIZE * 2;
	scaler->load_para[1].vir_addr = scaler->vipp_reg.vir_addr + VIPP_REG_SIZE * 2;
	memset(scaler->vipp_reg.vir_addr, 0, scaler->vipp_reg.size);
#endif
	return 0;
}

static void scaler_resource_free(struct scaler_dev *scaler)
{
	os_mem_free(&scaler->pdev->dev, &scaler->vipp_reg);
}

#if defined VIPP_200
static irqreturn_t scaler_isr(int irq, void *priv)
{
	unsigned long flags;
	struct scaler_dev *scaler = (struct scaler_dev *)priv;
	struct vipp_status vipp_status;

	memset(&vipp_status, 0, sizeof(struct vipp_status));
	vipp_get_status(scaler->id, &vipp_status);

	spin_lock_irqsave(&scaler->slock, flags);

	if (vipp_status.id_lost_pd) {
		vipp_clear_status(scaler->id, ID_LOST_PD);
		vin_err("scaler%d channel ID nember is lost!!!\n", scaler->id);
	}

	if (vipp_status.ahb_mbus_w_pd) {
		vipp_clear_status(scaler->id, AHB_MBUS_W_PD);
		vin_err("scaler%d AHB and MBUS write conflict!!!\n", scaler->id);
	}

	if (vipp_status.chn0_frame_lost_pd) {
		vipp_clear_status(scaler->id, CHN0_FRAME_LOST_PD);
		vin_err("scaler%d frame lost!!!\n", scaler->id + 0);
	}
	if (vipp_status.chn1_frame_lost_pd) {
		vipp_clear_status(scaler->id, CHN1_FRAME_LOST_PD);
		vin_err("scaler%d frame lost!!!\n", scaler->id + 1);
	}
	if (vipp_status.chn2_frame_lost_pd) {
		vipp_clear_status(scaler->id, CHN2_FRAME_LOST_PD);
		vin_err("scaler%d frame lost!!!\n", scaler->id + 2);
	}
	if (vipp_status.chn3_frame_lost_pd) {
		vipp_clear_status(scaler->id, CHN3_FRAME_LOST_PD);
		vin_err("scaler%d frame lost!!!\n", scaler->id + 3);
	}

	if (vipp_status.chn0_hblank_short_pd) {
		vipp_clear_status(scaler->id, CHN0_HBLANK_SHORT_PD);
		vin_err("scaler%d Hblank short!!!\n", scaler->id + 0);
	}
	if (vipp_status.chn1_hblank_short_pd) {
		vipp_clear_status(scaler->id, CHN1_HBLANK_SHORT_PD);
		vin_err("scaler%d Hblank short!!!\n", scaler->id + 1);
	}
	if (vipp_status.chn2_hblank_short_pd) {
		vipp_clear_status(scaler->id, CHN2_HBLANK_SHORT_PD);
		vin_err("scaler%d Hblank short!!!\n", scaler->id + 2);
	}
	if (vipp_status.chn3_hblank_short_pd) {
		vipp_clear_status(scaler->id, CHN3_HBLANK_SHORT_PD);
		vin_err("scaler%d Hblank short!!!\n", scaler->id + 3);
	}

	if (vipp_status.chn0_para_not_ready_pd) {
		vipp_clear_status(scaler->id, CHN0_PARA_NOT_READY_PD);
		vin_err("scaler%d param not ready!!!\n", scaler->id + 0);
	}
	if (vipp_status.chn1_para_not_ready_pd) {
		vipp_clear_status(scaler->id, CHN1_PARA_NOT_READY_PD);
		vin_err("scaler%d param not ready!!!\n", scaler->id + 1);
	}
	if (vipp_status.chn2_para_not_ready_pd) {
		vipp_clear_status(scaler->id, CHN2_PARA_NOT_READY_PD);
		vin_err("scaler%d param not ready!!!\n", scaler->id + 2);
	}
	if (vipp_status.chn3_para_not_ready_pd) {
		vipp_clear_status(scaler->id, CHN3_PARA_NOT_READY_PD);
		vin_err("scaler%d param not ready!!!\n", scaler->id + 3);
	}

	if (vipp_status.chn0_hshort_pd) {
		vipp_clear_status(scaler->id, CHN0_HSHORT_INT_PD);
		vin_err("scaler%d horizontal short!!!\n", scaler->id + 0);
	}
	if (vipp_status.chn1_hshort_pd) {
		vipp_clear_status(scaler->id, CHN1_HSHORT_INT_PD);
		vin_err("scaler%d horizontal short!!!\n", scaler->id + 1);
	}
	if (vipp_status.chn2_hshort_pd) {
		vipp_clear_status(scaler->id, CHN2_HSHORT_INT_PD);
		vin_err("scaler%d horizontal short!!!\n", scaler->id + 2);
	}
	if (vipp_status.chn3_hshort_pd) {
		vipp_clear_status(scaler->id, CHN3_HSHORT_INT_PD);
		vin_err("scaler%d horizontal short!!!\n", scaler->id + 3);
	}

	if (vipp_status.chn0_vshort_pd) {
		vipp_clear_status(scaler->id, CHN0_VSHORT_INT_PD);
		vin_err("scaler%d vertical short!!!\n", scaler->id + 0);
	}
	if (vipp_status.chn1_vshort_pd) {
		vipp_clear_status(scaler->id, CHN1_VSHORT_INT_PD);
		vin_err("scaler%d vertical short!!!\n", scaler->id + 1);
	}
	if (vipp_status.chn2_vshort_pd) {
		vipp_clear_status(scaler->id, CHN2_VSHORT_INT_PD);
		vin_err("scaler%d vertical short!!!\n", scaler->id + 2);
	}
	if (vipp_status.chn3_vshort_pd) {
		vipp_clear_status(scaler->id, CHN3_VSHORT_INT_PD);
		vin_err("scaler%d vertical short!!!\n", scaler->id + 3);
	}

	if (vipp_status.chn0_reg_load_pd) {
		vipp_clear_status(scaler->id, CHN0_REG_LOAD_PD);
		if (!vipp_get_irq_en(scaler->id, CHN0_REG_LOAD_EN)) {
			spin_unlock_irqrestore(&scaler->slock, flags);
			return IRQ_HANDLED;
		}
		vin_log(VIN_LOG_SCALER, "vipp%d change to load reg\n", scaler->id);
		if (scaler->load_select) {
			memcpy(scaler->load_para[1].vir_addr, scaler->vipp_reg.vir_addr, VIPP_REG_SIZE);
			vipp_set_reg_load_addr(scaler->id, (unsigned long)scaler->load_para[1].dma_addr);
			scaler->load_select = false;
		} else {
			memcpy(scaler->load_para[0].vir_addr, scaler->vipp_reg.vir_addr, VIPP_REG_SIZE);
			vipp_set_reg_load_addr(scaler->id, (unsigned long)scaler->load_para[0].dma_addr);
			scaler->load_select = true;
		}
		vipp_irq_disable(scaler->id, CHN0_REG_LOAD_EN);
	}
	if (vipp_status.chn1_reg_load_pd) {
		vipp_clear_status(scaler->id, CHN1_REG_LOAD_PD);
		if (!vipp_get_irq_en(scaler->id, CHN1_REG_LOAD_EN)) {
			spin_unlock_irqrestore(&scaler->slock, flags);
			return IRQ_HANDLED;
		}
		vin_log(VIN_LOG_SCALER, "vipp%d change to load reg\n", scaler->id + 1);
		if (glb_vipp[scaler->id + 1]->load_select) {
			memcpy(glb_vipp[scaler->id + 1]->load_para[1].vir_addr, glb_vipp[scaler->id + 1]->vipp_reg.vir_addr, VIPP_REG_SIZE);
			vipp_set_reg_load_addr(glb_vipp[scaler->id + 1]->id, (unsigned long)glb_vipp[scaler->id + 1]->load_para[1].dma_addr);
			scaler->load_select = false;
		} else {
			memcpy(glb_vipp[scaler->id + 1]->load_para[0].vir_addr, glb_vipp[scaler->id + 1]->vipp_reg.vir_addr, VIPP_REG_SIZE);
			vipp_set_reg_load_addr(glb_vipp[scaler->id + 1]->id, (unsigned long)glb_vipp[scaler->id + 1]->load_para[0].dma_addr);
			scaler->load_select = true;
		}
		vipp_irq_disable(scaler->id, CHN1_REG_LOAD_EN);
	}
	if (vipp_status.chn2_reg_load_pd) {
		vipp_clear_status(scaler->id, CHN2_REG_LOAD_PD);
		if (!vipp_get_irq_en(scaler->id, CHN2_REG_LOAD_EN)) {
			spin_unlock_irqrestore(&scaler->slock, flags);
			return IRQ_HANDLED;
		}
		vin_log(VIN_LOG_SCALER, "vipp%d change to load reg\n", scaler->id + 2);
		if (glb_vipp[scaler->id + 2]->load_select) {
			memcpy(glb_vipp[scaler->id + 2]->load_para[1].vir_addr, glb_vipp[scaler->id + 2]->vipp_reg.vir_addr, VIPP_REG_SIZE);
			vipp_set_reg_load_addr(glb_vipp[scaler->id + 2]->id, (unsigned long)glb_vipp[scaler->id + 2]->load_para[1].dma_addr);
			scaler->load_select = false;
		} else {
			memcpy(glb_vipp[scaler->id + 2]->load_para[0].vir_addr, glb_vipp[scaler->id + 2]->vipp_reg.vir_addr, VIPP_REG_SIZE);
			vipp_set_reg_load_addr(glb_vipp[scaler->id + 2]->id, (unsigned long)glb_vipp[scaler->id + 2]->load_para[0].dma_addr);
			scaler->load_select = true;
		}
		vipp_irq_disable(scaler->id, CHN2_REG_LOAD_EN);
	}
	if (vipp_status.chn3_reg_load_pd) {
		vipp_clear_status(scaler->id, CHN3_REG_LOAD_PD);
		if (!vipp_get_irq_en(scaler->id, CHN3_REG_LOAD_EN)) {
			spin_unlock_irqrestore(&scaler->slock, flags);
			return IRQ_HANDLED;
		}
		vin_log(VIN_LOG_SCALER, "vipp%d change to load reg\n", scaler->id + 3);
		if (glb_vipp[scaler->id + 3]->load_select) {
			memcpy(glb_vipp[scaler->id + 3]->load_para[1].vir_addr, glb_vipp[scaler->id + 3]->vipp_reg.vir_addr, VIPP_REG_SIZE);
			vipp_set_reg_load_addr(glb_vipp[scaler->id + 3]->id, (unsigned long)glb_vipp[scaler->id + 3]->load_para[1].dma_addr);
			scaler->load_select = false;
		} else {
			memcpy(glb_vipp[scaler->id + 3]->load_para[0].vir_addr, glb_vipp[scaler->id + 3]->vipp_reg.vir_addr, VIPP_REG_SIZE);
			vipp_set_reg_load_addr(glb_vipp[scaler->id + 3]->id, (unsigned long)glb_vipp[scaler->id + 3]->load_para[0].dma_addr);
			scaler->load_select = true;
		}
		vipp_irq_disable(scaler->id, CHN3_REG_LOAD_EN);
	}

	spin_unlock_irqrestore(&scaler->slock, flags);

	return IRQ_HANDLED;
}
#endif

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
static int scaler_irq_enable(void *dev, void *data, int len)
{
	struct scaler_dev *scaler = dev;
	int ret;

	if (!data || !strncmp(data, "return", len)) {
		ret = request_irq(scaler->irq, scaler_isr, IRQF_SHARED, scaler->pdev->name, scaler);
		if (ret) {
			vin_err("scaler%d request irq failed\n", scaler->id);
			return -EINVAL;
		}

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
			vin_iommu_en(CSI_IOMMU_MASTER, true);
#endif
	} else if (!strncmp(data, "get", len)) {
		if (scaler->irq)
			free_irq(scaler->irq, scaler);

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		vin_iommu_en(CSI_IOMMU_MASTER, false);
#endif
	}

	return 0;
}
#endif

static int scaler_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct scaler_dev *scaler = NULL;
	__maybe_unused struct scaler_dev *logic_scaler = NULL;
	__maybe_unused char name[16];
	int ret = 0;

	if (np == NULL) {
		vin_err("Scaler failed to get of node\n");
		return -ENODEV;
	}

	scaler = kzalloc(sizeof(*scaler), GFP_KERNEL);
	if (!scaler) {
		ret = -ENOMEM;
		vin_err("sunxi scaler kzalloc failed!\n");
		goto ekzalloc;
	}
	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("Scaler failed to get device id\n");
		ret = -EINVAL;
		goto freedev;
	}

	scaler->id = pdev->id;
	scaler->pdev = pdev;

#if defined VIPP_200
	of_property_read_u32(np, "work_mode", &scaler->work_mode);
	if (scaler->work_mode < 0) {
		vin_err("Scaler%d failed to get work_mode\n", scaler->id);
		ret = -EINVAL;
		goto freedev;

	} else {
		if (scaler->id == vipp_virtual_find_logic[scaler->id]) {
			scaler->work_mode = clamp_t(unsigned int, scaler->work_mode, VIPP_ONLINE, VIPP_OFFLINE);
		} else if (scaler->work_mode == 0xff) {
			logic_scaler = glb_vipp[vipp_virtual_find_logic[scaler->id]];
			if (logic_scaler->work_mode == VIPP_ONLINE) { /*logic vipp work in online*/
				vin_log(VIN_LOG_VIDEO, "scaler%d work in online mode, scaler%d cannot to work!\n", logic_scaler->id, scaler->id);
				__scaler_init_subdev(scaler);
				platform_set_drvdata(pdev, scaler);
				glb_vipp[scaler->id] = scaler;
				scaler->noneed_register = 1;
				vin_log(VIN_LOG_SCALER, "scaler%d probe end\n", scaler->id);
				return 0;
			}
		}
	}
#endif
	scaler->base = of_iomap(np, 0);
	if (!scaler->base) {
		scaler->is_empty = 1;
		scaler->base = kzalloc(0x400, GFP_KERNEL);
		if (!scaler->base) {
			ret = -EIO;
			goto freedev;
		}
	}

#if defined VIPP_200
	/*get irq resource */
	scaler->irq = irq_of_parse_and_map(np, 0);
	if (scaler->irq <= 0) {
		scaler->is_irq_empty = 1;
		if (scaler->id == vipp_virtual_find_logic[scaler->id])
			vin_err("failed to get vipp%d IRQ resource\n", scaler->id);
		else {
#if !defined CONFIG_VIN_INIT_MELIS
			scaler->delay_para_ready = 0;
#else
			scaler->delay_para_ready = 1;
#endif
		}
	} else {
#if !defined CONFIG_VIN_INIT_MELIS
		ret = request_irq(scaler->irq, scaler_isr, IRQF_SHARED, scaler->pdev->name, scaler);
		if (ret) {
			vin_err("scaler%d request irq failed\n", scaler->id);
			ret = -EINVAL;
			goto unmap;
		}
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		vin_iommu_en(CSI_IOMMU_MASTER, true);
#endif
		scaler->delay_para_ready = 0;
#else
		of_property_read_u32(np, "delay_init", &scaler->delay_init);
		if (scaler->delay_init == 0) {
			ret = request_irq(scaler->irq, scaler_isr, IRQF_SHARED, scaler->pdev->name, scaler);
			if (ret) {
				vin_err("scaler%d request irq failed\n", scaler->id);
				ret = -EINVAL;
				goto unmap;
			}
		} else {
			sprintf(name, "scaler%d", scaler->id);
			rpmsg_notify_add("7130000.e906_rproc", name, scaler_irq_enable, scaler);
		}
		scaler->delay_para_ready = 1;
#endif
	}
#endif
	__scaler_init_subdev(scaler);

	spin_lock_init(&scaler->slock);

	if (scaler_resource_alloc(scaler) < 0) {
		ret = -ENOMEM;
		goto unmap;
	}

	vipp_set_base_addr(scaler->id, (unsigned long)scaler->base);
	vipp_map_reg_load_addr(scaler->id, (unsigned long)scaler->vipp_reg.vir_addr);
	vipp_map_osd_para_load_addr(scaler->id, (unsigned long)scaler->osd_para.vir_addr);

	platform_set_drvdata(pdev, scaler);

	glb_vipp[scaler->id] = scaler;

	vin_log(VIN_LOG_SCALER, "scaler%d probe end\n", scaler->id);
	return 0;
unmap:
	if (!scaler->is_empty)
		iounmap(scaler->base);
	else
		kfree(scaler->base);
freedev:
	kfree(scaler);
ekzalloc:
	vin_err("scaler probe err!\n");
	return ret;
}

static int scaler_remove(struct platform_device *pdev)
{
	struct scaler_dev *scaler = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &scaler->subdev;
	__maybe_unused char name[16];

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	if (scaler->delay_init) {
		sprintf(name, "scaler%d", scaler->id);
		rpmsg_notify_del("7130000.e906_rproc", name);
	}
#endif
	if (scaler->noneed_register == 1) {
		platform_set_drvdata(pdev, NULL);
		media_entity_cleanup(&sd->entity);
		kfree(scaler);
		return 0;
	}
	scaler_resource_free(scaler);
	platform_set_drvdata(pdev, NULL);
	media_entity_cleanup(&sd->entity);
	v4l2_set_subdevdata(sd, NULL);
	if (scaler->base) {
		if (!scaler->is_empty)
			iounmap(scaler->base);
		else
			kfree(scaler->base);
#if defined VIPP_200
		if (!scaler->is_irq_empty)
			free_irq(scaler->irq, scaler);
#endif
	}
	kfree(scaler);
	return 0;
}

static const struct of_device_id sunxi_scaler_match[] = {
	{.compatible = "allwinner,sunxi-scaler",},
	{},
};

static struct platform_driver scaler_platform_driver = {
	.probe    = scaler_probe,
	.remove   = scaler_remove,
	.driver = {
		.name   = SCALER_MODULE_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_scaler_match,
	},
};

struct v4l2_subdev *sunxi_scaler_get_subdev(int id)
{
	if (id < VIN_MAX_SCALER && glb_vipp[id])
		return &glb_vipp[id]->subdev;
	else
		return NULL;
}
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1)
int sunxi_vipp_get_osd_stat(int id, unsigned int *stat)
{
	struct v4l2_subdev *sd = sunxi_scaler_get_subdev(id);
	struct scaler_dev *vipp = v4l2_get_subdevdata(sd);
	unsigned long long *stat_buf = vipp->osd_stat.vir_addr;
	int i;

	if (stat_buf == NULL || stat == NULL)
		return -EINVAL;

	for (i = 0; i < MAX_OVERLAY_NUM; i++)
		stat[i] = stat_buf[i];

	return 0;
}
#endif
int sunxi_scaler_platform_register(void)
{
	return platform_driver_register(&scaler_platform_driver);
}

void sunxi_scaler_platform_unregister(void)
{
	platform_driver_unregister(&scaler_platform_driver);
	vin_log(VIN_LOG_SCALER, "scaler_exit end\n");
}

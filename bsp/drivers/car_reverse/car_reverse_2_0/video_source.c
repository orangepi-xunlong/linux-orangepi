/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse video source module
 *
 * Copyright (C) 2015-2023 AllwinnerTech, Inc.
 *
 * Authors:  Huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "car_reverse.h"
#include "video_source.h"
#include "buffer_pool.h"

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#define VS_MAX 256

struct car_reverse_video_source *g_vsource[VS_MAX];
struct car_reverse_video_source *video_source;
struct buffer_node *t_node;
int video_source_cnt;

extern void car_reverse_display_update(int id);

struct buffer_node *video_source_dequeue_buffer(int video_id)
{
	int retval = 0;
	struct video_source_buffer *vsbuf;
	struct buffer_node *node = NULL;

	if (!video_source || !video_source->dequeue_buffer) {
		CAR_REVERSE_ERR("Invalid video source function: dequeue_buffer!\n");
		return ERR_PTR(-EINVAL);
	}

	retval = video_source->dequeue_buffer(video_id, &vsbuf);

	if (retval < 0) {
		VIDEO_SOURCE_DBG("dequeue_buffer failed, buffer maybe not ready!\n");
		return ERR_PTR(-EINVAL);
	}

	if (IS_ERR_OR_NULL(vsbuf)) {
		CAR_REVERSE_ERR("video source buffer not found!\n");
		return ERR_PTR(-ENOMEM);
	}

	node = container_of(vsbuf, struct buffer_node, vsbuf);
	VIDEO_SOURCE_DBG("video_id: %d, node->dmabuf_fd = %d\n", \
			 video_id, node->dmabuf_fd);

	return node;

}

int video_source_queue_buffer(struct buffer_node *node, int video_id)
{
	struct video_source_buffer *vsbuf;

	if (!video_source || !video_source->queue_buffer) {
		CAR_REVERSE_ERR("Invalid video source function: queue_buffer!\n");
		return -EINVAL;
	}

	VIDEO_SOURCE_DBG("video_id: %d, node->dmabuf_fd = %d\n", \
			 video_id, node->dmabuf_fd);

	vsbuf = &node->vsbuf;

	vsbuf->dmabuf_fd = node->dmabuf_fd;

	return video_source->queue_buffer(video_id, vsbuf);
}

int video_source_info(int video_id)
{
	if (!video_source) {
		CAR_REVERSE_ERR("Invalid video source function: video_open!\n");
		return -EINVAL;
	}

	if (video_source->video_info)
		return video_source->video_info();
	else
		return 0;
}

int video_source_open(int video_id)
{
	if (!video_source || !video_source->video_open) {
		CAR_REVERSE_ERR("Invalid video source function: video_open!\n");
		return -EINVAL;
	}

	VIDEO_SOURCE_DBG("video_id: %d\n", video_id);
	return video_source->video_open(video_id);
}

int video_source_close(int video_id)
{
	if (!video_source || !video_source->video_close) {
		CAR_REVERSE_ERR("Invalid video source function: video_close!\n");
		return -EINVAL;
	}

	VIDEO_SOURCE_DBG("video_id: %d\n", video_id);
	return video_source->video_close(video_id);
}

int video_source_set_format(int video_id, struct v4l2_format *fmt)
{
	if (!video_source || !video_source->video_set_format) {
		CAR_REVERSE_ERR("Invalid video source function: video_set_format!\n");
		return -EINVAL;
	}

	VIDEO_SOURCE_DBG("video_id: %d\n", video_id);
	return video_source->video_set_format(video_id, fmt);
}

int video_source_get_format(int video_id, struct v4l2_format *fmt)
{
	if (!video_source || !video_source->video_get_format) {
		CAR_REVERSE_ERR("Invalid video source function: video_get_format!\n");
		return -EINVAL;
	}

	VIDEO_SOURCE_DBG("video_id: %d\n", video_id);
	return video_source->video_get_format(video_id, fmt);
}

int video_source_streamon(int video_id)
{
	if (!video_source || !video_source->video_streamon) {
		CAR_REVERSE_ERR("Invalid video source function: video_streamon!\n");
		return -EINVAL;
	}

	VIDEO_SOURCE_DBG("video_id: %d\n", video_id);
	return video_source->video_streamon(video_id);
}

int video_source_streamoff(int video_id)
{
	if (!video_source || !video_source->video_streamoff) {
		CAR_REVERSE_ERR("Invalid video source function: video_streamoff!\n");
		return -EINVAL;
	}

	VIDEO_SOURCE_DBG("video_id: %d\n", video_id);
	return video_source->video_streamoff(video_id);
}

int video_source_select(int source_type)
{
	char source_name[10];
	int i;

	if (source_type == 1)
		sprintf(source_name, "tvd");
	else if (source_type == 2)
		sprintf(source_name, "virtual");
	else
		sprintf(source_name, "vin");

	for (i = 0; i < video_source_cnt; i++) {
		if (!strcmp(source_name, g_vsource[i]->type)) {
			CAR_REVERSE_INFO("video source match:%s!\n", g_vsource[i]->type);
			video_source = g_vsource[i];
			return 0;
		}
	}

	CAR_REVERSE_ERR("video source: %s is not registered yet!\n", source_name);
	return -EPROBE_DEFER;
}

void video_source_release(void)
{
	video_source = NULL;
}


int car_reverse_video_source_register(struct car_reverse_video_source *video_source_register)
{
	if (video_source_cnt < VS_MAX) {
		video_source_register->car_reverse_callback = car_reverse_display_update;
		g_vsource[video_source_cnt] = video_source_register;
		video_source_cnt++;
	} else
		CAR_REVERSE_ERR("video source overflow!\n");

	return 0;
}
EXPORT_SYMBOL(car_reverse_video_source_register);

int car_reverse_video_source_unregister(struct car_reverse_video_source *video_source_register)
{
	int i;

	for (i = 0; i < video_source_cnt; i++) {
		if (!strcmp(video_source_register->type, g_vsource[i]->type)) {
			CAR_REVERSE_INFO("video source unregister:%s!\n", g_vsource[i]->type);
			g_vsource[i] = NULL;
			return 0;
		}
	}

	return 0;
}
EXPORT_SYMBOL(car_reverse_video_source_unregister);

#if IS_ENABLED(CONFIG_CAR_REVERSE_SUPPORT_VIN)
extern int vin_open_special(int id);
extern int vin_close_special(int id);
extern int vin_dqbuffer_special(int id, struct vin_buffer **vsbuf);
extern int vin_qbuffer_special(int id, struct vin_buffer *vsbuf);
extern int vin_g_fmt_special(int id, struct v4l2_format *f);
extern int vin_g_fmt_special_ext(int id, struct v4l2_format *f);
extern int vin_s_fmt_special(int id, struct v4l2_format *f);
extern int vin_s_input_special(int id, int i);
extern int vin_streamoff_special(int video_id, enum v4l2_buf_type i);
extern int vin_streamon_special(int video_id, enum v4l2_buf_type i);
extern int vin_force_reset_buffer(int video_id);
extern void vin_register_buffer_done_callback(int id, void *func);
extern int vin_tvin_special(int id, struct tvin_init_info *info);

static int vin_video_open(int id)
{
	int ret = -1;

	ret = vin_open_special(id);
	if (ret)
		return ret;

	vin_register_buffer_done_callback(id, car_reverse_display_update);

	return 0;
}

static int vin_video_qbuffer(int id, struct video_source_buffer *vsbuf)
{
	struct vin_buffer *vin_buf = &vsbuf->vin_buf;

	vin_buf->dmabuf_fd = vsbuf->dmabuf_fd;

	return vin_qbuffer_special(id, vin_buf);
}

static int vin_video_dqbuffer(int id, struct video_source_buffer **vsbuf)
{
	struct vin_buffer *vin_buf = NULL;
	int ret  = 0;

	ret = vin_dqbuffer_special(id, &vin_buf);
	if (ret < 0) {
		VIDEO_SOURCE_DBG("vin dequeue buffer fail!\n");
		return ret;
	}

	*vsbuf = container_of(vin_buf, struct video_source_buffer, vin_buf);
	if (IS_ERR_OR_NULL(*vsbuf)) {
		CAR_REVERSE_ERR("video source buffer not found!\n");
		return -ENOMEM;
	}

	return 0;
}

static int vin_video_get_fmt(int id, struct v4l2_format *f)
{
	vin_s_input_special(id, 0);

	return vin_g_fmt_special_ext(id, f);
}

static int vin_video_streamon(int id)
{
	return vin_streamon_special(id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
}

static int vin_video_streamoff(int id)
{
	return vin_streamoff_special(id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
}

static int video_tvin_setting(int tvd_id, enum reverse_tvin_type cvbs_used)
{
	struct tvin_init_info info;
	int ch_id;

	if (tvd_id > TVIN_VIDEO_MAX)
		ch_id = tvd_id / TVIN_VIDEO_STRIP + tvd_id % TVIN_VIDEO_STRIP;
	else
		ch_id = tvd_id / TVIN_VIDEO_STRIP;
	info.ch_id = ((ch_id >= TVIN_SEPARATE)?(ch_id - TVIN_SEPARATE):(ch_id));

	switch (cvbs_used) {
	case R_CVBS_NTSC:
		info.work_mode = Tvd_Input_Type_4CH_CVBS;
		info.ch_3d_filter = 0;
		info.input_fmt[info.ch_id] = CVBS_NTSC;
		break;
	case R_CVBS_PAL:
		info.work_mode = Tvd_Input_Type_4CH_CVBS;
		info.ch_3d_filter = 0;
		info.input_fmt[info.ch_id] = CVBS_PAL;
		break;
	case R_CVBS_H1440_NTSC:
		info.work_mode = Tvd_Input_Type_4CH_CVBS;
		info.ch_3d_filter = 0;
		info.input_fmt[info.ch_id] = CVBS_H1440_NTSC;
		break;
	case R_CVBS_H1440_PAL:
		info.work_mode = Tvd_Input_Type_4CH_CVBS;
		info.ch_3d_filter = 0;
		info.input_fmt[info.ch_id] = CVBS_H1440_PAL;
		break;
	case R_AHD720P25:
		info.work_mode = 0;
		info.input_fmt[info.ch_id] = AHD720P25;
		break;
	case R_AHD1080P25:
		info.work_mode = 0;
		info.input_fmt[info.ch_id] = AHD1080P25;
		break;
	default:
		info.work_mode = 0;
		info.input_fmt[info.ch_id] = AHD1080P25;
		break;
	}

	if (vin_tvin_special(tvd_id, &info) != 0) {
		CAR_REVERSE_ERR("vin_tvin_special error!\n");
		return -1;
	}

	return 0;
}

struct car_reverse_video_source video_source_vin = {
	.type = "vin",
	.video_open = vin_video_open,
	.video_close = vin_close_special,
	.video_streamon = vin_video_streamon,
	.video_streamoff = vin_video_streamoff,
	.queue_buffer = vin_video_qbuffer,
	.dequeue_buffer = vin_video_dqbuffer,
	.video_set_format = vin_s_fmt_special,
	.video_get_format = vin_video_get_fmt,
	.force_reset_buffer = vin_force_reset_buffer,
};
#endif


#if IS_ENABLED(CONFIG_CAR_REVERSE_SUPPORT_TVD)
extern int tvd_open_special(int id);
extern int tvd_close_special(int id);
extern int dqbuffer_special(int id, struct tvd_buffer **vsbuf);
extern int qbuffer_special(int id, struct tvd_buffer *vsbuf);
extern int vidioc_s_fmt_vid_cap_special(int id, struct v4l2_format *f);
extern int vidioc_g_fmt_vid_cap_special(int id, struct v4l2_format *f);
extern int vidioc_streamoff_special(int video_id);
extern int vidioc_streamon_special(int video_id);
extern int tvd_force_reset_buffer(int video_id);
extern void tvd_register_buffer_done_callback(int id, void *func);
extern int tvd_info_special(void);

static int tvd_video_open(int id)
{
	int ret = -1;

	ret = tvd_open_special(id);
	if (ret)
		return ret;

	tvd_register_buffer_done_callback(id, car_reverse_display_update);

	return 0;
}

static int tvd_video_qbuffer(int id, struct video_source_buffer *vsbuf)
{
	struct tvd_buffer *tvd_buf = &vsbuf->tvd_buf;

	tvd_buf->dmabuf_fd = vsbuf->dmabuf_fd;

	return qbuffer_special(id, tvd_buf);
}

static int tvd_video_dqbuffer(int id, struct video_source_buffer **vsbuf)
{
	struct tvd_buffer *tvd_buf = NULL;
	int ret  = 0;

	ret = dqbuffer_special(id, &tvd_buf);
	if (ret < 0) {
		VIDEO_SOURCE_DBG("vin dequeue buffer fail!\n");
		return ret;
	}

	*vsbuf = container_of(tvd_buf, struct video_source_buffer, tvd_buf);
	if (IS_ERR_OR_NULL(*vsbuf)) {
		CAR_REVERSE_ERR("video source buffer not found!\n");
		return -ENOMEM;
	}

	return 0;
}

struct car_reverse_video_source video_source_tvd = {
	.type = "tvd",
	.video_open = tvd_video_open,
	.video_close = tvd_close_special,
	.video_info = tvd_info_special,
	.video_streamon = vidioc_streamon_special,
	.video_streamoff = vidioc_streamoff_special,
	.queue_buffer = tvd_video_qbuffer,
	.dequeue_buffer = tvd_video_dqbuffer,
	.video_set_format = vidioc_s_fmt_vid_cap_special,
	.video_get_format = vidioc_g_fmt_vid_cap_special,
	.force_reset_buffer = tvd_force_reset_buffer,
};
#endif


void car_reverse_video_source_init(void)
{
#if IS_ENABLED(CONFIG_CAR_REVERSE_SUPPORT_VIN)
	car_reverse_video_source_register(&video_source_vin);
#endif

#if IS_ENABLED(CONFIG_CAR_REVERSE_SUPPORT_TVD)
	car_reverse_video_source_register(&video_source_tvd);
#endif
}

/* get tvin fmt ----- only for camera hybrid input*/
void __get_video_inputfmt(struct preview_params *params, enum reverse_tvin_type *fmt)
{
	if (params->src_height >= 720) {
		if (params->src_width == 1920 && params->src_height == 1080) {
			*fmt = R_AHD1080P25;
		} else {
			*fmt = R_AHD720P25;
		}
	} else {
		if (params->src_height == 576) {
			if (params->src_width == 1440) {
				*fmt = R_CVBS_H1440_PAL;
			} else {
				*fmt = R_CVBS_PAL;
			}
		} else {
			if (params->src_width == 1440) {
				*fmt = R_CVBS_H1440_NTSC;
			} else {
				*fmt = R_CVBS_NTSC;
			}
		}
	}

	VIDEO_SOURCE_DBG("tvin fmt get:%d, %dx%d\n", *fmt, params->src_width, params->src_height);
}

static int video_source_format_setting(struct preview_params *params, int video_id)
{
	struct v4l2_format format;
	struct v4l2_format format_prew;
	enum reverse_tvin_type input_fmt; // for camera hybrid input

	memset(&format, 0, sizeof(struct v4l2_format));
	memset(&format_prew, 0, sizeof(struct v4l2_format));

	video_source_get_format(video_id, &format);

	/* src size will be adjust by video source when src_size_adaptive enable */
	if (params->src_size_adaptive) {
		params->src_width  = format.fmt.pix.width;
		params->src_height = format.fmt.pix.height;
	}

#if IS_ENABLED(CONFIG_CAR_REVERSE_SUPPORT_VIN)
	/* only for camera hybrid input */
	__get_video_inputfmt(params, &input_fmt);
	video_tvin_setting(video_id, input_fmt);
#endif

	format_prew.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format_prew.fmt.pix.pixelformat = params->format;
	format_prew.fmt.pix.width  = params->src_width;
	format_prew.fmt.pix.height = params->src_height;

	if (video_source_set_format(video_id, &format_prew) != 0) {
		CAR_REVERSE_ERR("video source set format error!\n");
		return -1;
	}

	VIDEO_SOURCE_DBG("format_got:%d, %dx%d\n", format.fmt.pix.pixelformat, \
		format.fmt.pix.width, format.fmt.pix.height);

	return 0;
}

int video_source_connect(struct preview_params *params, int video_id)
{
	int ret = 0;

	ret = video_source_open(video_id);
	if (ret <  0) {
		CAR_REVERSE_ERR("video_source_open fail!\n");
		return ret;
	}

	ret = video_source_format_setting(params, video_id);
	if (ret < 0) {
		CAR_REVERSE_ERR("video_source_format_setting fail!\n");
		video_source_close(video_id);
		return ret;
	}

	return 0;
}

int video_source_disconnect(int video_id)
{
	return video_source_close(video_id);
}

int video_source_force_reset_buffer(int video_id)
{
	return video_source->force_reset_buffer(video_id);
}

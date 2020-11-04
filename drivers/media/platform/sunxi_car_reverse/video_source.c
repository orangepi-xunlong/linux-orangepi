/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sunxi-gpio.h>
#include <linux/gpio.h>

#include "sunxi_tvd.h"
#include "car_reverse.h"
#ifdef CONFIG_VIDEO_SUNXI_TVD
static struct tvd_fmt _default_fmt;
#endif
#ifndef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
int vin_open_special(int tvd_fd)
{
	return 0;
}
int vin_close_special(int tvd_fd)
{
	return 0;
}
int vin_s_input_special(int id, int i)
{
	return 0;
}
int vin_s_fmt_special(int tvd_fd, struct v4l2_format *f)
{
	return 0;
}
int vin_g_fmt_special(int tvd_fd, struct v4l2_format *f)
{
	return 0;
}
int vin_dqbuffer_special(int tvd_fd, struct vin_buffer **buf)
{
	return 0;
}
int vin_qbuffer_special(int tvd_fd, struct vin_buffer *buf)
{
	return 0;
}
int vin_streamon_special(int tvd_fd, enum v4l2_buf_type i)
{
	return 0;
}
int vin_streamoff_special(int tvd_fd, enum v4l2_buf_type i)
{
	return 0;
}
void vin_register_buffer_done_callback(int tvd_fd, void *func)
{
	return;
}
#endif

#ifdef VIDEO_SUNXI_TVD_SPECIAL
struct buffer_node *video_source_dequeue_buffer(int tvd_fd)
{
	int retval = 0;
	struct tvd_buffer *buffer;
	struct buffer_node *node = NULL;

	retval = dqbuffer_special(tvd_fd, &buffer);
	if (!retval && buffer)
		node = container_of(buffer, struct buffer_node, handler);

	return node;
}
#endif
struct buffer_node *video_source_dequeue_buffer_vin(int tvd_fd)
{
	int retval = 0;
	struct vin_buffer *buffer;
	struct buffer_node *node = NULL;


	retval = vin_dqbuffer_special(tvd_fd, &buffer);
	if (!retval && buffer) {
		node = container_of(buffer, struct buffer_node, handler_vin);
	}

	return node;
}

#ifdef VIDEO_SUNXI_TVD_SPECIAL

void video_source_queue_buffer(struct buffer_node *node, int tvd_fd)
{
	struct tvd_buffer *buffer;

	if (node) {
		buffer = &node->handler;
		buffer->paddr = node->phy_address;
#ifdef _VIRTUAL_DEBUG_
		buffer->vaddr = node->vir_address;
#endif
		buffer->fmt = &_default_fmt;

		qbuffer_special(tvd_fd, buffer);
	} else {
		logerror(KERN_ERR "%s: not enought buffer!\n", __func__);
	}
}
#endif
void video_source_queue_buffer_vin(struct buffer_node *node, int tvd_fd)
{
	struct vin_buffer *buffer;

	if (node) {
		buffer = &node->handler_vin;
		buffer->paddr = node->phy_address;
		vin_qbuffer_special(tvd_fd, buffer);

	} else {
		logerror(KERN_ERR "%s: not enought buffer!\n", __func__);
	}
}


extern void car_reverse_display_update(int tvd_fd);
void buffer_done_callback(int tvd_fd)
{
	car_reverse_display_update(tvd_fd);
}

#ifdef VIDEO_SUNXI_TVD_SPECIAL
int video_source_streamon(int tvd_fd)
{
#ifdef _VIRTUAL_DEBUG_
	virtual_tvd_register_buffer_done_callback(buffer_done_callback);
#else
	tvd_register_buffer_done_callback(buffer_done_callback);
#endif

	vidioc_streamon_special(tvd_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	return 0;
}
#endif
int video_source_streamon_vin(int tvd_fd)
{
#ifdef _VIRTUAL_DEBUG_
	virtual_tvd_register_buffer_done_callback(buffer_done_callback);
#else
	vin_register_buffer_done_callback(tvd_fd, buffer_done_callback);
#endif
	vin_streamon_special(tvd_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	return 0;
}

#ifdef VIDEO_SUNXI_TVD_SPECIAL
int video_source_streamoff(int tvd_fd)
{
	vidioc_streamoff_special(tvd_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	return 0;
}
#endif
int video_source_streamoff_vin(int tvd_fd)
{
	vin_streamoff_special(tvd_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	return 0;
}


const char *interfaces_desc[] = {
	[CVBS_INTERFACE]   = "cvbs",
	[YPBPRI_INTERFACE] = "YPbPr-i",
	[YPBPRP_INTERFACE] = "YpbPr-p",
};

const char *systems_desc[] = {
	[NTSC] = "NTSC",
	[PAL]  = "PAL",
};
#ifdef VIDEO_SUNXI_TVD_SPECIAL
const char *formats_desc[] = {
	[TVD_PL_YUV420] = "YUV420 PL",
	[TVD_MB_YUV420] = "YUV420 MB",
	[TVD_PL_YUV422] = "YUV422 PL",
};
#endif

static int video_source_format_setting(struct preview_params *params, int tvd_id)
{
	int locked = 0;
	struct v4l2_format format;
	struct v4l2_format format_prew;
	memset(&format, 0, sizeof(format));
	memset(&format_prew, 0, sizeof(format_prew));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (params->input_src) {
			vin_s_input_special(tvd_id, 0);
			vin_g_fmt_special(tvd_id, &format);
			locked = 1;
			params->locked = 1;
			format.fmt.pix.width  = 1280;
			format.fmt.pix.height = 720;
		} else {
			#ifdef VIDEO_SUNXI_TVD_SPECIAL
			if (vidioc_g_fmt_vid_cap_special(tvd_id, &format) == 0) {
				locked = 1;
				params->locked = 1;
				format_prew.fmt.pix.width  = format.fmt.pix.width;
				format_prew.fmt.pix.height = format.fmt.pix.height;
			}
			#endif
		}

	params->locked = locked;
	params->src_width  = format.fmt.pix.width;
	params->src_height = format.fmt.pix.height;
	if (params->input_src == 0) {
		params->system = format.fmt.raw_data[105];
		params->interface = format.fmt.raw_data[104];
		if	(!locked) {
			params->src_width  = 720;
			params->src_height = 576;
			params->system = 1;
			params->locked = 1;
		}
	}
	format_prew.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format_prew.fmt.pix.pixelformat = params->format;
	format_prew.fmt.pix.width  = format.fmt.pix.width;
	format_prew.fmt.pix.height = format.fmt.pix.height;
#ifndef _VIRTUAL_DEBUG_
	if (params->input_src) {
		if (vin_s_fmt_special(tvd_id,  &format_prew) != 0) {
			logerror("video format config error!\n");
			return -1;
		}
	} else {
		#ifdef VIDEO_SUNXI_TVD_SPECIAL
		if (vidioc_s_fmt_vid_cap_special(tvd_id, &format_prew) != 0) {
			logerror("video format config error!\n");
			return -1;
		}
		#endif
	}
	logerror("return  format - %d, %dx%d\n", format.fmt.pix.pixelformat,
		format.fmt.pix.width, format.fmt.pix.height);
	logerror("request format - %d, %dx%d\n", params->format,
		params->src_width, params->src_height);
#endif
	return 0;
}

int video_source_connect(struct preview_params *params, int tvd_id)
{
	int retval = -1;
	if (params->input_src)
		retval = vin_open_special(tvd_id);
	else{
		#ifdef VIDEO_SUNXI_TVD_SPECIAL
		retval = tvd_open_special(tvd_id);
		#endif
		}
	if (retval != 0)
		return -1;

	if (video_source_format_setting(params, tvd_id) < 0) {
		logerror("video_source_format_setting fail!!!!!!!!!!\n");
		return -1;
	}

	if (params->locked != 1) {
		logerror("unlock!!!!!!!!!!\n");
		if (params->input_src)
			vin_close_special(tvd_id);
		else{
			#ifdef VIDEO_SUNXI_TVD_SPECIAL
			tvd_close_special(tvd_id);
			#endif
			}
		return -1;
	}
	#ifdef VIDEO_SUNXI_TVD_SPECIAL
	memset(&_default_fmt, 0, sizeof(_default_fmt));
	_default_fmt.output_fmt = params->format;
	#endif
	return 0;
}

int video_source_disconnect(struct preview_params *params, int tvd_id)
{
	if (params->input_src)
		vin_close_special(tvd_id);
	else{
		#ifdef VIDEO_SUNXI_TVD_SPECIAL
		tvd_close_special(tvd_id);
		#endif
		}
	return 0;
}


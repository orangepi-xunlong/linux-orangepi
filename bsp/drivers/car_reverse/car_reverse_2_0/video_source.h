/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2013-2022 AllwinnerTech, Inc.
 *
 * Authors:  Huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __VIDEO_SOURCE_H__
#define __VIDEO_SOURCE_H__

#include "car_reverse.h"
#include "buffer_pool.h"

enum reverse_tvin_type {
	R_CVBS_PAL = 0,
	R_CVBS_NTSC,
	R_CVBS_H1440_PAL,
	R_CVBS_H1440_NTSC,
	R_AHD720P25,
	R_AHD720P30,
	R_AHD1080P25,
	R_AHD1080P30,
};
struct car_reverse_video_source {
	char *type;
	int (*video_open)(int video_id);
	int (*video_close)(int video_id);
	int (*video_info)(void);
	int (*video_streamon)(int video_id);
	int (*video_streamoff)(int video_id);
	int (*queue_buffer)(int video_id, struct video_source_buffer *buf);
	int (*dequeue_buffer)(int video_id, struct video_source_buffer **buf);
	int (*force_reset_buffer)(int video_id);
	int (*video_set_format)(int video_id, struct v4l2_format *fmt);
	int (*video_get_format)(int video_id, struct v4l2_format *fmt);
	void (*car_reverse_callback)(int video_id);
};

int car_reverse_video_source_register(struct car_reverse_video_source *ops);
int car_reverse_video_source_unregister(struct car_reverse_video_source *ops);
int video_source_connect(struct preview_params *params, int video_id);
int video_source_disconnect(int video_id);
int video_source_streamon(int video_id);
int video_source_streamoff(int video_id);
int video_source_streamon_vin(int video_id);
int video_source_streamoff_vin(int video_id);
int video_source_force_reset_buffer(int video_id);
int video_source_select(int source_type);
int video_source_queue_buffer(struct buffer_node *node, int video_id);
struct buffer_node *video_source_dequeue_buffer(int video_id);
void car_reverse_callback_register(void *func);
void video_source_release(void);
void car_reverse_video_source_init(void);

#endif

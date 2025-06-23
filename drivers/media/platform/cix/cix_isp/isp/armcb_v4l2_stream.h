/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __ARMCB_V4L2_STREAM_H__
#define __ARMCB_V4L2_STREAM_H__

//#include <linux/videodev2.h>
#include <armcb_isp.h>
#include <media/videobuf2-v4l2.h>

/* Sensor data types */
#define MAX_SENSOR_PRESET_SIZE 10
#define MAX_SENSOR_FPS_SIZE 10

typedef struct _armcb_v4l2_sensor_preset {
	uint32_t width;
	uint32_t height;
	uint32_t fps[MAX_SENSOR_FPS_SIZE];
	uint32_t idx[MAX_SENSOR_FPS_SIZE];
	uint8_t fps_num;
	uint8_t fps_cur;
	uint8_t exposures[MAX_SENSOR_FPS_SIZE];
} armcb_v4l2_sensor_preset;

typedef struct _armcb_v4l2_sensor_info {
	/* resolution preset */
	armcb_v4l2_sensor_preset preset[MAX_SENSOR_PRESET_SIZE];
	uint8_t preset_num;
	uint8_t preset_cur;
} armcb_v4l2_sensor_info;

/* buffer for one video frame */
typedef struct _armcb_v4l2_buffer {
	struct vb2_v4l2_buffer vvb;
	struct list_head list;
} armcb_v4l2_buffer_t;

/**
 * struct armcb_v4l2_stream_common
 */
typedef struct _armcb_v4l2_frame_sizes {
	/* resolution table for FR stream */
	struct v4l2_frmsize_discrete frmsize[MAX_SENSOR_PRESET_SIZE];
	/* for now this is same since FR path
	 * doesn't have downscaler block
	 */
	uint8_t frmsize_num;
} armcb_v4l2_frame_sizes;

typedef struct _armcb_v4l2_stream_common {
	armcb_v4l2_sensor_info sensor_info;
	armcb_v4l2_frame_sizes snapshot_sizes;
} armcb_v4l2_stream_common;

/**
 * struct armcb_v4l2_stream_t - All internal data for one instance of ISP
 */
typedef struct _armcb_v4l2_stream_t {
	/* Control fields */
	uint32_t ctx_id;
	int stream_id;
	armcb_v4l2_stream_type_t stream_type;
	int stream_started;
	uint32_t last_frame_id;

	/* Input stream */
	armcb_v4l2_stream_common *stream_common;

	/* Stream format */
	struct v4l2_format cur_v4l2_fmt;
	u32 outport;

	/* Video buffer field*/
	struct list_head stream_buffer_list;
	struct list_head stream_buffer_list_busy;
	spinlock_t slock;

	/* Temporal fields for memcpy */

	atomic_t running; // since metadata has no thread for syncing

	int fw_frame_seq_count;
	u32 reserved_buf_addr;
} armcb_v4l2_stream_t;

int armcb_v4l2_stream_init(armcb_v4l2_stream_t **ppstream, int stream_id,
			   int ctx_num);
void armcb_v4l2_stream_deinit(armcb_v4l2_stream_t *pstream);
int armcb_v4l2_stream_on(armcb_v4l2_stream_t *pstream);
void armcb_v4l2_stream_off(armcb_v4l2_stream_t *pstream);
int armcb_v4l2_stream_get_format(armcb_v4l2_stream_t *pstream,
				 struct v4l2_format *f);
int armcb_v4l2_stream_set_format(armcb_v4l2_stream_t *pstream,
				 struct v4l2_format *f);

#endif

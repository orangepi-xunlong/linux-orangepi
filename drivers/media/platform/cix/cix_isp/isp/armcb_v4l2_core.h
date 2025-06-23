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
#ifndef __ARMCB_V4L2_CORE_H__
#define __ARMCB_V4L2_CORE_H__

#include "armcb_isp.h"
#include "armcb_v4l2_stream.h"
#include <linux/fb.h>
#include <linux/workqueue.h>
#include <media/cec.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-v4l2.h>

#define CORE_NEVENTS (32)
#define ARMCB_MAX_DEVS (16)

#define CIX_CAMERA_MODULE_INDEX "cix,camera-module-index"

typedef struct armcb_v4l2_dev {
	struct platform_device *pvdev;

	/* device */
	uint32_t ctx_id;
	struct v4l2_device v4l2_dev;
	struct video_device vid_cap_dev;

	spinlock_t slock;
	struct mutex mutex;
	spinlock_t v4l2_event_slock;
	struct mutex v4l2_event_mutex;
	struct mutex ordered_sd_mutex;
	struct list_head ordered_sd_list;

	/* capabilities */
	u32 vid_cap_caps;

	/* streams */
	armcb_v4l2_stream_t *pstreams[V4L2_STREAM_TYPE_MAX];
	int32_t stream_id_index[V4L2_STREAM_TYPE_MAX];
	atomic_t stream_on_cnt;

	/* open counter for stream id */
	atomic_t opened;
	unsigned int stream_mask;

	/* Error injection (not used now)*/
	bool queue_setup_error;
	bool buf_prepare_error;
	bool start_streaming_error;
	bool dqbuf_error;
	bool seq_wrap;
	bool has_vid_cap;

	struct v4l2_async_notifier notifier;
	struct media_entity_enum crashed;
	struct media_device media_dev;

	/* v4l2_subdev async register */
	struct v4l2_async_notifier dts_notifier;
#ifdef CONFIG_MEDIA_CONTROLLER
	struct media_device mdev;
#endif
} armcb_v4l2_dev_t;

struct armcb_v4l_dev_info {
	u32 video_num;
	armcb_v4l2_dev_t *armcb_devs[ARMCB_MAX_DEVS];
};

static inline int armcb_outport_bits_to_idx(u32 bits)
{
	unsigned long bits_ul = (unsigned long)bits;

	return find_first_bit(&bits_ul, 32);
}

armcb_v4l2_dev_t *armcb_v4l2_core_get_dev(uint32_t ctx_id);
uint32_t armcb_v4l2_core_find_1st_opened_dev(void);
armcb_v4l2_dev_t *armcb_register_instance(struct platform_device *pdev,
					  struct device *devnode, u32 cam_id);

void armcb_isp_put_frame(uint32_t ctx_id, int stream_type,
			 isp_output_port_t port);
int armcb_v4l2_find_stream(armcb_v4l2_stream_t **ppstream, uint32_t ctx_id,
			   int stream_type);
int armcb_v4l2_find_ctx_stream_by_outport(uint32_t outport, uint32_t *p_ctx_id,
					  uint32_t *p_stream_id);
int armcb_v4l2_find_stream_by_outport_ctx(uint32_t outport, uint32_t ctx_id,
					  uint32_t *p_stream_id);
void armcb_cam_instance_destroy(void);
#endif

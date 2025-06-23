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
#ifndef __ARMCB_V4L2_CONFIG_H__
#define __ARMCB_V4L2_CONFIG_H__

#include "armcb_isp_driver.h"
#include <linux/interrupt.h>
#include <media/v4l2-device.h>

struct armcb_irq_event{
	struct isp_irq_info info;
	struct list_head fh_node;
};

struct armcb_irq_msg_fh {
	struct armcb_irq_event ev_array[32];
	int in_use;
	int first;
	int elems;
	spinlock_t fh_lock;
	struct list_head avaliable;
};

typedef struct armcb_v4l2_config_dev {
	struct platform_device *pvdev;

	/* device */
	struct v4l2_device v4l2_dev;
	struct video_device vid_cap_dev;


	spinlock_t slock;
	struct mutex mutex;
	spinlock_t v4l2_event_slock;
	struct mutex v4l2_event_mutex;
	/* capabilities */
	u32 vid_cap_caps;

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
	/* video capture */
	struct v4l2_async_notifier notifier;
	struct media_entity_enum crashed;
	struct media_device media_dev;

	/* buffer_done_work */
	struct tasklet_struct buffer_done_task;
	struct armcb_irq_msg_fh isr_fh;
} armcb_v4l2_config_dev_t;

static inline unsigned int irqev_pos(const struct armcb_irq_msg_fh *fh,
				 unsigned int idx)
{
	idx += fh->first;
	return idx >= fh->elems ? idx - fh->elems : idx;
}

int armcb_v4l2_config_update_stream_vin_addr(armcb_v4l2_stream_t *pstream);
int armcb_v4l2_config_update_stream_hw_addr(armcb_v4l2_stream_t *pstream);

#ifdef ARMCB_CAM_KO
void *armcb_get_v4l2_cfg_driver_instance(void);
void armcb_v4l2_cfg_driver_destroy(void);
#endif

#endif

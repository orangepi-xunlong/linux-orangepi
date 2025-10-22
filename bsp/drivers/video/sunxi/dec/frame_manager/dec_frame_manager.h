/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dec_frame_manager.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: yajianz <yajianz@allwinnertech.com>
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

#ifndef _VIDEO_FRAME_MGR_H_
#define _VIDEO_FRAME_MGR_H_

#include <linux/kref.h>
#include <linux/spinlock.h>

#include "dec_frame_manager_private.h"

#define FRAME_QUEUE_DEPTH 4
#define REPEAT_COUNT_BEFOR_STOP 4

#define THRESHOLD_OF_HW_FIELD_REPEAT_BEGIN 2

#define FRAME_PTR_FIFO_SIZE (128 * sizeof(void *))

struct afbd_reg_t;

struct dec_frame_manager;
typedef void (*video_frame_release_cb)(void *);
typedef void (*video_frame_sync_cb)(struct afbd_reg_t *, void *, int, unsigned int, unsigned int);
typedef void (*video_frame_repeat_ctrl)(struct afbd_reg_t *, unsigned int);

typedef struct video_frame {
	struct kref refcount;
	uint32_t frame_number;
	struct list_head link;

	// private fields for userland
	void *private;
	video_frame_release_cb release_pfn;

} video_frame_t;

typedef struct frame_queue_trace_info {
	/*
	 * When vsync interrupt comes, if there is no new video frame,
	 * this value will increase by 1.
	 */
	int repeat_count;

	/*
	 * This frame_count will reset to zero on stream stop !
	 *
	 * We need this field to identify the first frame of a interlace stream,
	 * if the interlace stream pause right afther the first frame,
	 * We need to do a soft frame repeat before setup up the hardware repeat mode.
	 */
	int frame_count;

} frame_queue_trace_info_t;

struct interlace_data {
	uint32_t enable;
	uint32_t current_field;
	uint32_t field_mask;
	struct video_frame *frame;

	/*
	 * use this counter to trigger hardware repeat mode
	 * for interlace stream.
	 */
	uint32_t frame_repeat_count;
};

typedef struct frame_queue {
	spinlock_t frame_lock;
	uint32_t dirty;
	uint32_t vinfo_dirty;
	struct interlace_data interlace;
	video_frame_t *frames[FRAME_QUEUE_DEPTH];

	int notify_mips;
	int notify_stop;
	video_frame_t *cache_frame;

	// video frames should be release on next vsync interrupt
	DECLARE_KFIFO_PTR(pending_release_fifo, typeof(struct video_frame *));
	// When ADDR_OUT_MUX_ID set to 2 (requires by svp),
	// AFBD is still reading from the queue->frames[3] although it had been move out from the frame queue!
	// so we defer all the frame moving out from the frame queue before add it to the pending_release_fifo.
	video_frame_t *defer_release_frame;

	// private fields for userland
	struct afbd_reg_t *base;
	video_frame_setting_t data;
	video_frame_sync_cb hardware_sync_pfn;

	int hardware_repeat_enable;
	video_frame_repeat_ctrl repeat_ctrl_pfn;

	struct frame_queue_trace_info trace_info;
} frame_queue_t;

typedef enum stream_type {
	STREAM_PROGRESSIVE,
	STREAM_INTERLACED,
} stream_type_t;

struct dec_frame_manager *dec_frame_manager_init(void);
int dec_frame_manager_exit(struct dec_frame_manager *fmgr);
int dec_frame_manager_enqueue_frame(struct dec_frame_manager *fmgr, void *private, video_frame_release_cb cb);
int dec_frame_manager_stop(struct dec_frame_manager *fmgr);
int dec_frame_manager_set_stream_type(struct dec_frame_manager *fmgr, stream_type_t type);
int dec_frame_manager_enqueue_interlace_frame(struct dec_frame_manager *fmgr, void *top, void *bottom, video_frame_release_cb cb);
int dec_frame_manager_register_sync_cb(struct dec_frame_manager *fmgr, struct afbd_reg_t *base, video_frame_sync_cb cb);
int dec_frame_manager_register_repeat_ctrl_pfn(struct dec_frame_manager *fmgr, struct afbd_reg_t *base, video_frame_repeat_ctrl pfn);
int dec_frame_manager_dump(struct dec_frame_manager *fmgr, char *strbuf);

void dec_vsync_process(struct dec_frame_manager *fmgr);

void set_repeat_count_for_signal_changed(int repeat_count);
int get_repeat_count_for_signal_changed(void);

struct overscan_info;
int dec_frame_manager_set_overscan(struct dec_frame_manager *fmgr, struct overscan_info *info);
struct overscan_info *dec_frame_manager_get_overscan(struct dec_frame_manager *fmgr);

#endif

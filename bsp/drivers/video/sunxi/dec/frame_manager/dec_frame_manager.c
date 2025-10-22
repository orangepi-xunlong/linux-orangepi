/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dec_frame_manager.c
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

#define pr_fmt(fmt) "decoder-display: " fmt

#include <asm-generic/errno-base.h>
#include <linux/atomic.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/module.h>

#include "../dec_trace.h"
#include "../dec_video_info.h"
#include "dec_frame_manager.h"

static int repeat_count_for_signal_chaned;

void set_repeat_count_for_signal_changed(int repeat_count)
{
	repeat_count_for_signal_chaned = repeat_count;
}

int get_repeat_count_for_signal_changed(void)
{
	return repeat_count_for_signal_chaned;
}

enum dec_error_code {
	/* everything is ok, no any bad */
	DECD_ERROR_NONE,

	/* frame queue is dirty and not sync to hardware yet  */
	DECD_FRAME_QUEUE_DIRTY,

	DECD_NO_FRAME,
};

typedef enum stream_state {
	STREAM_STOP,
	STREAM_PLAYING,
	STREAM_PAUSE,
	STREAM_EXITING,
	/* notify mips that we need to stop play */
	STREAM_SIGNAL_CHANGED,
	STREAM_CLEANUP,
} stream_state_t;

typedef enum request_cmd {
	CMD_INVALID,
	CMD_STOP,
	CMD_PLAY,
	CMD_PAUSE,
} request_cmd_t;

typedef enum field_mode {
	TOP_FIELD,
	BOTTOM_FIELD,
} field_mode_t;

struct frame_recycle_work {
	spinlock_t lock;
	DECLARE_KFIFO_PTR(release_frame_fifo, typeof(struct video_frame *));

	struct work_struct work;
};

struct interlace_frames {
	uint32_t enable;
	uint32_t frame_count;

	/*
	 * frames[0] -- P
	 * frames[1] -- I
	 */
	struct video_frame *frames[2];
};

typedef struct debug_trace_info {
	int repeat_flag;
	int frame_count;
} debug_trace_info_t;

struct dec_frame_manager {
	uint32_t frame_seqnum;
	atomic_t current_state;
	atomic_t request;

	stream_type_t type;

	/*
	 * video frame ready to be present
	 */
	spinlock_t lock;
	struct list_head ready_frame_list;
	struct overscan_info *overscan;

	struct debug_trace_info traceinfo;

	/*
	 * for interlace mode debug
	 */
	struct interlace_frames interlace;
	struct interlace_data interlace_stream;

	struct frame_queue *queue;

	uint64_t irqcount;

	struct tasklet_struct refresh_queue_tasklet;

	// handle video_frame release on workqueue
	struct frame_recycle_work recycle_work;

	wait_queue_head_t wait_queue;
};

extern int dec_is_interlace_frame(const void *data);

static inline void video_frame_get(struct video_frame *frame);
static inline void video_frame_put(struct video_frame *frame);

/*
 * Hardware repeat control of interlace video stream.
 */
static const int hardware_field_repeat_used = 1;

static inline void dec_trace_frame_count(int count)
{
	DEC_TRACE_INT_F("frame-count", count);
}

static inline void dec_trace_frame_repeat(int flag)
{
	DEC_TRACE_INT_F("frame-repeat", flag);
}

static int dec_frame_ready(struct dec_frame_manager *fmgr)
{
	unsigned long flags;
	int ready = 0;

	spin_lock_irqsave(&fmgr->lock, flags);
	ready = !list_empty(&fmgr->ready_frame_list);
	spin_unlock_irqrestore(&fmgr->lock, flags);

	return ready;
}

/*
 * Pop a video frame from ready_frame_list
 */
static video_frame_t *dec_frame_pop(struct dec_frame_manager *fmgr)
{
	unsigned long flags;
	uint32_t field_idx = 0;
	video_frame_t *frame = NULL;

	spin_lock_irqsave(&fmgr->lock, flags);

	// just for top/bottom interlace frame debug
	if (fmgr->type == STREAM_INTERLACED && fmgr->interlace.enable) {
		field_idx = fmgr->interlace.frame_count & 0x01;
		frame = fmgr->interlace.frames[field_idx];
		video_frame_get(frame);
		fmgr->interlace.frame_count++;
		goto __out;
	}

	if (!list_empty(&fmgr->ready_frame_list)) {
		frame = list_first_entry(&fmgr->ready_frame_list, video_frame_t, link);
		list_del_init(&frame->link);
		dec_trace_frame_count(--fmgr->traceinfo.frame_count);
	}

__out:
	spin_unlock_irqrestore(&fmgr->lock, flags);
	return frame;
}

/*
 * Push a video frame to ready_frame_list,
 * Which will be present on screen after some vsync period.
 */
static void dec_frame_push(struct dec_frame_manager *fmgr, video_frame_t *frame)
{
	unsigned long flags;

	spin_lock_irqsave(&fmgr->lock, flags);
	list_add_tail(&frame->link, &fmgr->ready_frame_list);
	dec_trace_frame_count(++fmgr->traceinfo.frame_count);
	spin_unlock_irqrestore(&fmgr->lock, flags);
}

/*
 * When enqueue failed,
 * add the frame to the begin of ready_frame_list
 */
static void dec_frame_push_back(struct dec_frame_manager *fmgr, video_frame_t *frame)
{
	unsigned long flags;

	spin_lock_irqsave(&fmgr->lock, flags);

	if (fmgr->type == STREAM_INTERLACED) {
		video_frame_put(frame);
		goto __out;
	}

	list_add(&frame->link, &fmgr->ready_frame_list);
	dec_trace_frame_count(++fmgr->traceinfo.frame_count);

__out:
	spin_unlock_irqrestore(&fmgr->lock, flags);
}

static struct video_frame *dec_create_video_frame(uint32_t frame_number)
{
	struct video_frame *frame;

	frame = kzalloc(sizeof(*frame), GFP_KERNEL);
	if (!frame)
		return ERR_PTR(-ENOMEM);

	frame->frame_number = frame_number;
	kref_init(&frame->refcount);

	DEC_TRACE_FRAME(__func__, frame->frame_number);
	return frame;
}

static void video_frame_release(struct kref *kref)
{
	struct video_frame *frame = container_of(kref, struct video_frame, refcount);
	DEC_TRACE_FRAME(__func__, frame->frame_number);

	if (frame->release_pfn)
		frame->release_pfn(frame->private);

	kfree(frame);
}

static inline void video_frame_get(struct video_frame *frame)
{
	kref_get(&frame->refcount);
}

static inline void video_frame_put(struct video_frame *frame)
{
	kref_put(&frame->refcount, video_frame_release);
}


static void dec_frame_release_process(struct work_struct *work)
{
	video_frame_t *frame = NULL;
	struct frame_recycle_work *recycle_work;

	recycle_work = container_of(work, struct frame_recycle_work, work);

	while (kfifo_out_spinlocked(&recycle_work->release_frame_fifo,
				&frame, 1, &recycle_work->lock) == 1) {
		if (frame != NULL) {
			video_frame_put(frame);
			frame = NULL;
		}
	}
}

static inline void dec_frame_defer_release(struct frame_queue *queue, video_frame_t *legacy)
{
	video_frame_t *defer = queue->defer_release_frame;

	if (likely(defer != NULL))
		kfifo_in(&queue->pending_release_fifo, &defer, 1);

	queue->defer_release_frame = legacy;
}

static inline void dec_frame_defer_release_finish(struct frame_queue *queue)
{
	video_frame_t *defer = queue->defer_release_frame;
	if (likely(defer != NULL))
		kfifo_in(&queue->pending_release_fifo, &defer, 1);

	queue->defer_release_frame = NULL;
}

// should be call within queue->frame_lock protect;
static void dec_frame_recycle(struct frame_queue *queue, struct frame_recycle_work *recycle_work)
{
	video_frame_t *frame = NULL;

	while (kfifo_out(&queue->pending_release_fifo, &frame, 1) == 1) {
		if (frame != NULL) {
			kfifo_in_spinlocked(&recycle_work->release_frame_fifo,
					&frame, 1,
					&recycle_work->lock);
			frame = NULL;
		}
	}

	schedule_work(&recycle_work->work);
}

/*
 * frame_queue
 */

static struct frame_queue *dec_init_frame_queue(void)
{
	int error;
	struct frame_queue *queue;

	queue = kzalloc(sizeof(*queue), GFP_KERNEL);
	if (!queue)
		return ERR_PTR(-ENOMEM);

	spin_lock_init(&queue->frame_lock);
	queue->dirty = 0;
	queue->vinfo_dirty = 0;

	queue->defer_release_frame = NULL;
	INIT_KFIFO(queue->pending_release_fifo);
	error = kfifo_alloc(&queue->pending_release_fifo, FRAME_PTR_FIFO_SIZE, GFP_KERNEL);
	if (error) {
		pr_err("frame_queue: kfifo_alloc failed\n");
		kfree(queue);
		return ERR_PTR(-ENOMEM);
	}

	return queue;
}

static void dec_terminate_frame_queue(struct frame_queue *queue)
{
	kfifo_free(&queue->pending_release_fifo);
	kfree(queue);
}

// init frame_queue for video stream
static int dec_start_frame_queue(struct dec_frame_manager *fmgr)
{
	unsigned long flags;
	int i;
	struct frame_queue *queue;
	video_frame_t *frame = NULL;
	unsigned int interlace = 0;
	int ret = DECD_ERROR_NONE;

	queue = fmgr->queue;

	spin_lock_irqsave(&queue->frame_lock, flags);

	if (queue->dirty) {
		spin_unlock_irqrestore(&queue->frame_lock, flags);
		return DECD_FRAME_QUEUE_DIRTY;
	}

	frame = dec_frame_pop(fmgr);

	if (frame) {
		for (i = 0; i < FRAME_QUEUE_DEPTH; i++) {
			video_frame_get(frame);
			queue->frames[i] = frame;
		}

		interlace = dec_is_interlace_frame(frame->private);

		if (interlace) {
			dec_frame_manager_set_stream_type(fmgr, STREAM_INTERLACED);
			queue->interlace.enable = 1;
			queue->interlace.current_field = TOP_FIELD;
			queue->interlace.field_mask = 0x00000000;
			queue->interlace.frame = frame;
		} else if (fmgr->interlace.enable) {
			// TODO: just for test
			dec_frame_manager_set_stream_type(fmgr, STREAM_INTERLACED);
			queue->interlace.enable = 0;
			queue->interlace.current_field = TOP_FIELD;
			queue->interlace.field_mask = 0x00000000;
			video_frame_put(frame);
		} else {
			memset(&queue->interlace, 0, sizeof(queue->interlace));
			dec_frame_manager_set_stream_type(fmgr, STREAM_PROGRESSIVE);
			video_frame_put(frame);
		}

		queue->notify_mips = 1;
		queue->dirty = 1;
		queue->trace_info.repeat_count = 0;
		queue->trace_info.frame_count  = 1;
		atomic_set(&fmgr->current_state, STREAM_PLAYING);
	} else {
		spin_unlock_irqrestore(&queue->frame_lock, flags);
		return DECD_NO_FRAME;
	}

	spin_unlock_irqrestore(&queue->frame_lock, flags);
	return ret;
}

/*
 * prepare video frames to be update on vs interrupt
 */
static int dec_frame_queue_enqueue(struct frame_queue *queue, struct video_frame *frame)
{
	video_frame_t *legacy = NULL;

	if (!frame || !queue)
		return -EINVAL;

	if (queue->dirty) {
		pr_err("enqueue frame on dirty!");
		return DECD_FRAME_QUEUE_DIRTY;
	}

	legacy = queue->frames[3];
	dec_frame_defer_release(queue, legacy);

	queue->frames[3] = queue->frames[2];
	queue->frames[2] = queue->frames[1];
	queue->frames[1] = queue->frames[0];
	queue->frames[0] = frame;

	video_frame_get(frame);

	queue->dirty = 1;
	queue->trace_info.repeat_count = 0;

	return DECD_ERROR_NONE;
}

static inline int dec_frame_queue_hardware_repeat_ctrl(struct frame_queue *queue, int enable)
{
	if (queue->hardware_repeat_enable != enable) {
		queue->hardware_repeat_enable = enable;
		queue->data.hardware_repeat_enable = enable;

		// we had exit from pause, reset ready_to_exit_pause
		if (queue->hardware_repeat_enable)
			queue->data.ready_to_exit_pause = 0;

		DEC_TRACE_FRAME("request hardware repeat", enable);
		return 1;
	}
	return 0;
}

static inline void dec_frame_queue_hardware_repeat_ctrl_exit_prepare(struct frame_queue *queue)
{
	queue->data.hardware_repeat_enable = 0;
	queue->data.ready_to_exit_pause = 1;
}

void dec_frame_queue_make_interlace_pause_sequence(struct frame_queue *queue)
{
	video_frame_t *legacy = NULL;

	/*
	 * buffer sequence when interlace stream pause:
	 * [addr0] (N+2) B
	 * [addr1] (N+2) B
	 * [addr2] (N+2) T
	 * [addr3] (N+1) B
	 */

	// release frames[3]
	legacy = queue->frames[2];
	dec_frame_defer_release(queue, legacy);

	/* frames[1] is copy to frames[2], increase the refcount */
	if (queue->frames[1])
		video_frame_get(queue->frames[1]);
	queue->frames[2] = queue->frames[1];

	/* addr sequence: BBTB */
	queue->interlace.field_mask = (BOTTOM_FIELD << 3) | (TOP_FIELD << 2) | (BOTTOM_FIELD << 1) | (BOTTOM_FIELD << 0);
}

static void dec_frame_queue_repeat_interlace(struct frame_queue *queue)
{
	video_frame_t *legacy = NULL;
	uint32_t field;

	// TODO: just for vs debug interlace stream pause
	if (hardware_field_repeat_used) {
		field = queue->interlace.current_field;
		if (field == TOP_FIELD) {

			// release frames[3]
			legacy = queue->frames[3];
			dec_frame_defer_release(queue, legacy);

			/* frames[0] is copy from interlace.frame, increase the refcount */
			if (queue->frames[0])
				video_frame_get(queue->frames[0]);

			queue->frames[3] = queue->frames[2];
			queue->frames[2] = queue->frames[1];
			queue->frames[1] = queue->frames[0];

			queue->interlace.current_field = BOTTOM_FIELD;
			queue->interlace.field_mask = (queue->interlace.field_mask << 1) | BOTTOM_FIELD;
			queue->frames[0] = queue->interlace.frame;

			queue->dirty = 1;

		} else {
			/*
			 * We believe that this function will only be entered when the playback needs to be stopped,
			 * So we will stay in the BOTTOM_FIELD state until the frame_manager stop!
			 */
			queue->interlace.current_field = BOTTOM_FIELD;
			if (dec_frame_queue_hardware_repeat_ctrl(queue, 1)) {
				//dec_frame_queue_make_interlace_pause_sequence(queue);
				queue->dirty = 1;
			}
		}
		return;
	}

	// release frames[3]
	legacy = queue->frames[3];
	dec_frame_defer_release(queue, legacy);

	queue->frames[3] = queue->frames[2];
	queue->frames[2] = queue->frames[1];
	queue->frames[1] = queue->frames[0];

	// repeat the current frame
	field = queue->interlace.current_field;
	switch (field) {
	case TOP_FIELD:
		queue->interlace.current_field = BOTTOM_FIELD;
		queue->interlace.field_mask = (queue->interlace.field_mask << 1) | BOTTOM_FIELD;
		queue->frames[0] = queue->interlace.frame;
		break;
	case BOTTOM_FIELD:
		queue->interlace.current_field = TOP_FIELD;
		queue->interlace.field_mask = (queue->interlace.field_mask << 1) | TOP_FIELD;
		queue->frames[0] = queue->interlace.frame;
		break;
	default:
		break;
	}

	/* frames[0] is copy from interlace.frame, increase the refcount */
	if (queue->frames[0])
		video_frame_get(queue->frames[0]);

	queue->dirty = 1;
	queue->trace_info.repeat_count++;
}

static void dec_frame_queue_repeat(struct frame_queue *queue)
{
	video_frame_t *legacy = NULL;

	// Need special processing for interlace video
	if (queue->interlace.enable) {
		queue->trace_info.repeat_count++;
		dec_frame_queue_repeat_interlace(queue);
		return;
	}

	legacy = queue->frames[3];
	dec_frame_defer_release(queue, legacy);

	queue->frames[3] = queue->frames[2];
	queue->frames[2] = queue->frames[1];
	queue->frames[1] = queue->frames[0];

	/* frames[0] has been repeat to frames[1], increase the refcount */
	if (queue->frames[0])
		video_frame_get(queue->frames[0]);

	queue->dirty = 1;
	queue->trace_info.repeat_count++;
}

static void dec_frame_queue_cleanup_prepare(struct frame_queue *queue)
{
	int i;
	video_frame_t *legacy = NULL;

	// Increase the last frame refcount,
	// prevent the AFBD still reading from the last frame, when the frame manager cleanup.
	if (queue->interlace.frame) {
		queue->cache_frame = queue->interlace.frame;
		queue->interlace.frame = NULL;
	} else if (queue->frames[0]) {
		video_frame_get(queue->frames[0]);
		queue->cache_frame = queue->frames[0];
	}

	for (i = 0; i < FRAME_QUEUE_DEPTH; i++) {
		legacy = queue->frames[i];
		dec_frame_defer_release(queue, legacy);
		queue->frames[i] = NULL;
	}

	/*
	 * exit from playing, disable hardware repeat.
	 */
	queue->data.field_mode = 0;
	queue->data.hardware_repeat_enable = 0;
	queue->data.frame = 0;

	queue->dirty = 1;
	queue->notify_stop = 0;
	queue->trace_info.repeat_count = 0;
	queue->trace_info.frame_count  = 0;
}

static void dec_frame_queue_signal_changed_prepare(struct frame_queue *queue)
{
	queue->dirty = 1;
	queue->notify_stop = 1;
	queue->trace_info.repeat_count = 0;
}

static inline int video_frame_has_same_frame_item(video_frame_t *a, video_frame_t *b)
{
	if ((!a) || (!b))
		return 0;
	if ((!a->private) || (!b->private))
		return 0;
	return (a->private == b->private);
}

static void dec_frame_queue_update_overscan(struct frame_queue *queue, struct overscan_info *info)
{
	int already_update = 0;
	size_t i = 0, j = 0;

	if (!info)
		return;

	DEC_TRACE_FUNC();
	for (i = 0; i < FRAME_QUEUE_DEPTH; i++) {
		already_update = 0;
		for (j = 0; j < i; j++) {
			if (video_frame_has_same_frame_item(queue->frames[i], queue->frames[j])) {
				already_update = 1;
				break;
			}
		}

		if (!already_update && queue->frames[i]) {
			video_info_buffer_update_overscan(queue->frames[i]->private, info);
		}
	}

	kfree(info);
	queue->vinfo_dirty = 1;
	DEC_TRACE_END("");
}

static void dec_frame_queue_playing(struct dec_frame_manager *fmgr)
{
	int error;
	unsigned long flags;
	video_frame_t *frame = NULL;
	struct frame_queue *queue = fmgr->queue;
	uint32_t field;
	int had_ready_frame = 0;

	spin_lock_irqsave(&queue->frame_lock, flags);

	if (queue->dirty) {
		// the frame queue is not sync to hardware yet
		spin_unlock_irqrestore(&queue->frame_lock, flags);
		return;
	}

	if (queue->interlace.enable) {
		field = queue->interlace.current_field;
		switch (field) {
		case TOP_FIELD:
			queue->interlace.current_field = BOTTOM_FIELD;
			if (queue->hardware_repeat_enable) {
				/*
				 * Do nothing and skip out when hardware field repeat mode enabled.
				 */
				queue->data.field_mode = BOTTOM_FIELD;

				// had ready frame ,prepare pause exit ahead !!!
				if (dec_frame_ready(fmgr))
					dec_frame_queue_hardware_repeat_ctrl_exit_prepare(queue);

				break;
			}
			queue->interlace.field_mask = (queue->interlace.field_mask << 1) | BOTTOM_FIELD;

			dec_frame_queue_enqueue(queue, queue->interlace.frame);
			break;
		case BOTTOM_FIELD:
			queue->interlace.current_field = TOP_FIELD;

			had_ready_frame = dec_frame_ready(fmgr);

			if (unlikely(had_ready_frame == 0)) {
				/*
				 * If the interlace stream pause right afther the first frame,
				 * We need to do a soft frame repeat before setup up the hardware repeat mode.
				 */
				if (queue->trace_info.frame_count <= 4) {
					queue->interlace.field_mask = (queue->interlace.field_mask << 1) | TOP_FIELD;
					queue->interlace.frame_repeat_count = 0;
					queue->trace_info.frame_count += 1;
					fmgr->traceinfo.repeat_flag = 0;
					dec_frame_queue_enqueue(queue, queue->interlace.frame);
					DEC_TRACE_FRAME("soft repeat", 1);
					break;
				}
			}

			if (had_ready_frame &&
					((!queue->hardware_repeat_enable) || (queue->hardware_repeat_enable && queue->data.ready_to_exit_pause))) {

				if (queue->hardware_repeat_enable)
					dec_frame_queue_hardware_repeat_ctrl(queue, 0);

				// replace with the next frame if any frame is available
				frame = dec_frame_pop(fmgr);
				queue->interlace.field_mask = (queue->interlace.field_mask << 1) | TOP_FIELD;
				queue->interlace.frame_repeat_count = 0;
				video_frame_put(queue->interlace.frame);
				queue->interlace.frame = frame;
				queue->trace_info.frame_count += 1;
				fmgr->traceinfo.repeat_flag = 0;
			} else {
				queue->interlace.frame_repeat_count++;

				// TODO: just for vs debug interlace stream pause
				if (hardware_field_repeat_used) {
					if (dec_frame_queue_hardware_repeat_ctrl(queue, 1)) {
						queue->repeat_ctrl_pfn(queue->base, 1);
						//dec_frame_queue_make_interlace_pause_sequence(queue);
						queue->dirty = 0;
					}
					queue->data.field_mode = TOP_FIELD;

					// async update video info for overscan change!!!
					dec_frame_queue_update_overscan(queue, dec_frame_manager_get_overscan(fmgr));

					break;
				}
				// repeat the current frame
				queue->trace_info.repeat_count++;
				fmgr->traceinfo.repeat_flag = 1;
			}

			dec_frame_queue_enqueue(queue, queue->interlace.frame);
			break;
		default:
			pr_err("frame queue current field invalid!\n");
			break;
		}

		goto __out;
	}

	if (fmgr->interlace.enable) {
		field = queue->interlace.current_field;
		switch (field) {
		case TOP_FIELD:
			queue->interlace.current_field = BOTTOM_FIELD;
			queue->interlace.field_mask = (queue->interlace.field_mask << 1) | BOTTOM_FIELD;
			dec_frame_queue_enqueue(queue, fmgr->interlace.frames[1]);
			break;
		case BOTTOM_FIELD:
			queue->interlace.current_field = TOP_FIELD;
			queue->interlace.field_mask = (queue->interlace.field_mask << 1) | TOP_FIELD;
			dec_frame_queue_enqueue(queue, fmgr->interlace.frames[0]);
			break;
		default:
			pr_err("frame queue current field invalid!\n");
			break;
		}

		goto __out;
	}

	frame = dec_frame_pop(fmgr);
	if (frame) {
		error = dec_frame_queue_enqueue(queue, frame);
		if (error == DECD_ERROR_NONE) {
			/*
			 * This frame has been enqueue to frame_queue success,
			 * And frame_queue has increased the refcount, so we can release the ownership here.
			 */
			video_frame_put(frame);
		} else {
			/*
			 * enqueue failed, return to the ready_frame_list
			 */
			dec_frame_push_back(fmgr, frame);
		}
		fmgr->traceinfo.repeat_flag = 0;
	} else {
		// not any incoming frame yet, just repeat the first one.
		dec_frame_queue_repeat(queue);
		// async update video info for overscan change!!!
		dec_frame_queue_update_overscan(queue, dec_frame_manager_get_overscan(fmgr));
		queue->vinfo_dirty = 0;
		fmgr->traceinfo.repeat_flag = 1;
	}

__out:
	spin_unlock_irqrestore(&queue->frame_lock, flags);

	dec_trace_frame_repeat(fmgr->traceinfo.repeat_flag);
}

static void dec_frame_queue_exiting(struct dec_frame_manager *fmgr)
{
	unsigned long flags;
	stream_state_t next;
	struct frame_queue *queue = fmgr->queue;

	spin_lock_irqsave(&queue->frame_lock, flags);

	if (queue->dirty) {
		// the frame queue is not sync to hardware yet
		spin_unlock_irqrestore(&queue->frame_lock, flags);
		return;
	}

	if (queue->trace_info.repeat_count < REPEAT_COUNT_BEFOR_STOP) {
		dec_frame_queue_repeat(queue);
	} else {
		next = atomic_read(&fmgr->request) == CMD_STOP ? STREAM_SIGNAL_CHANGED : STREAM_PAUSE;
		atomic_set(&fmgr->current_state, next);

		if (next == STREAM_SIGNAL_CHANGED) {
			queue->trace_info.repeat_count = 0;
			dec_frame_queue_signal_changed_prepare(queue);
		}
	}

	spin_unlock_irqrestore(&queue->frame_lock, flags);
}

static void dec_frame_queue_signal_changed(struct dec_frame_manager *fmgr)
{
	unsigned long flags;
	struct frame_queue *queue = fmgr->queue;

	spin_lock_irqsave(&queue->frame_lock, flags);
	if (queue->dirty) {
		// the frame queue is not sync to hardware yet
		spin_unlock_irqrestore(&queue->frame_lock, flags);
		return;
	}

	if (queue->trace_info.repeat_count < repeat_count_for_signal_chaned)
		dec_frame_queue_repeat(queue);
	else {
		queue->trace_info.repeat_count = 0;
		atomic_set(&fmgr->current_state, STREAM_CLEANUP);
		dec_frame_queue_cleanup_prepare(queue);
	}

	spin_unlock_irqrestore(&queue->frame_lock, flags);
}

static void dec_frame_queue_cleanup(struct dec_frame_manager *fmgr)
{
	unsigned long flags;
	video_frame_t *legacy = NULL;
	struct frame_queue *queue = fmgr->queue;

	spin_lock_irqsave(&queue->frame_lock, flags);
	if (queue->dirty) {
		// the frame queue is not sync to hardware yet
		spin_unlock_irqrestore(&queue->frame_lock, flags);
		return;
	}

	// repeat empty frame before release the last cache frame
	if (queue->trace_info.repeat_count < REPEAT_COUNT_BEFOR_STOP)
		dec_frame_queue_repeat(queue);
	else {
		if (queue->cache_frame) {
			legacy = queue->cache_frame;
			dec_frame_defer_release(queue, legacy);
			queue->cache_frame = NULL;
			dec_frame_defer_release_finish(queue);
			dec_frame_recycle(queue, &fmgr->recycle_work);
		}

		// should not happen, because we had move the interlace.frame to cache_frame
		// on cleanup_prepare()
		if (queue->interlace.frame) {
			legacy = queue->interlace.frame;
			dec_frame_defer_release(queue, legacy);
			queue->interlace.frame = NULL;
			dec_frame_defer_release_finish(queue);
			dec_frame_recycle(queue, &fmgr->recycle_work);
		}

		atomic_set(&fmgr->current_state, STREAM_STOP);
		dec_frame_queue_hardware_repeat_ctrl(queue, 0);
	}

	spin_unlock_irqrestore(&queue->frame_lock, flags);
}

extern void dec_reg_int_to_display_atomic(struct afbd_reg_t *p_reg);

static void dec_frame_queue_sync(struct frame_queue *queue)
{
	int index;
	unsigned int field;

	queue->dirty = 0;

	for (index = 0; index < FRAME_QUEUE_DEPTH; index++) {
		field = (queue->interlace.field_mask >> index) & 0x01;
		if (queue->frames[index]) {
			queue->data.frame = queue->frames[index]->private;
			queue->hardware_sync_pfn(queue->base, &queue->data, queue->notify_stop, index, field);
			DEC_TRACE_HWREG("SYNC", index, queue->frames[index]->frame_number);
		} else {
			queue->data.frame = NULL;
			queue->hardware_sync_pfn(queue->base, &queue->data, 1, index, field);
			DEC_TRACE_HWREG("SYNC", index, 0);
		}
	}

	if (queue->notify_mips) {
		queue->notify_mips = 0;
		dec_reg_int_to_display_atomic(queue->base);
	}
}

extern void dec_sync_interlace_cfg_to_hardware(struct afbd_reg_t *base, void *private);
static void dec_frame_queue_toggle_for_interlace(struct frame_queue *queue)
{
	if (queue->frames[0]) {
		queue->data.frame = queue->frames[0]->private;
		queue->repeat_ctrl_pfn(queue->base, queue->data.hardware_repeat_enable);
		DEC_TRACE_FRAME("repeat-mode", queue->data.hardware_repeat_enable);
		dec_sync_interlace_cfg_to_hardware(queue->base, &queue->data);
	}
}

extern void dec_sync_videoinfo_to_hardware(struct afbd_reg_t *base, void *private, unsigned int id, unsigned int field);
static void dec_frame_queue_sync_videoinfo(struct frame_queue *queue)
{
	int index;
	unsigned int field;

	for (index = 0; index < FRAME_QUEUE_DEPTH; index++) {
		field = (queue->interlace.field_mask >> index) & 0x01;
		if (queue->frames[index]) {
			queue->data.frame = queue->frames[index]->private;
			dec_sync_videoinfo_to_hardware(queue->base,  queue->frames[index]->private, index, field);
			DEC_TRACE_HWREG("SYNC-vinfo", index, queue->frames[index]->frame_number);
		} else {
			DEC_TRACE_HWREG("SYNC-vinfo", index, 0);
		}
	}
}

/*
 * call from **interrupt context** for handling frame flip.
 */
void dec_vsync_process(struct dec_frame_manager *fmgr)
{
	unsigned long flags;
	stream_state_t current_state;
	struct frame_queue *queue = fmgr->queue;

	DEC_TRACE_FUNC();

	spin_lock_irqsave(&queue->frame_lock, flags);

	if (queue->dirty) {
		// sync frame queue to hardware register
		dec_frame_queue_sync(queue);
		dec_frame_recycle(queue, &fmgr->recycle_work);
	} else if (queue->hardware_repeat_enable) {
		if (queue->vinfo_dirty) {
			dec_frame_queue_sync_videoinfo(queue);
			queue->vinfo_dirty = 0;
		}
		dec_frame_queue_toggle_for_interlace(queue);
	}

	spin_unlock_irqrestore(&queue->frame_lock, flags);

	current_state = atomic_read(&fmgr->current_state);

	DEC_TRACE_FRAME("current state", current_state);
	if (current_state != STREAM_PAUSE && current_state != STREAM_STOP) {
		// refresh frame queue on tasklet
		tasklet_schedule(&fmgr->refresh_queue_tasklet);
	} else {
		if (current_state == STREAM_STOP && waitqueue_active(&fmgr->wait_queue)) {
			wake_up(&fmgr->wait_queue);
			DEC_TRACE_FRAME("wait_up", 0);
		}
	}

	fmgr->irqcount += 1;

	DEC_TRACE_END("");
}

static void dec_frame_queue_refresh(unsigned long data)
{
	stream_state_t current_state;
	struct dec_frame_manager *fmgr = (struct dec_frame_manager *)data;

	current_state = atomic_read(&fmgr->current_state);

	switch (current_state) {
	case STREAM_PLAYING:
		dec_frame_queue_playing(fmgr);
		break;
	case STREAM_EXITING:
		dec_frame_queue_exiting(fmgr);
		break;
	case STREAM_SIGNAL_CHANGED:
		dec_frame_queue_signal_changed(fmgr);
		break;
	case STREAM_CLEANUP:
		dec_frame_queue_cleanup(fmgr);
		break;
	default:
		break;
	}
}

struct dec_frame_manager *dec_frame_manager_init(void)
{
	int error;
	struct dec_frame_manager *fmgr;

	fmgr = kzalloc(sizeof(*fmgr), GFP_KERNEL);
	if (!fmgr)
		return ERR_PTR(-ENOMEM);

	fmgr->queue = dec_init_frame_queue();

	if (IS_ERR_OR_NULL(fmgr->queue)) {
		pr_err("dec_init_frame_queue return error\n");
		kfree(fmgr);
		return ERR_PTR(-ENOMEM);
	}

	spin_lock_init(&fmgr->lock);

	fmgr->type = STREAM_PROGRESSIVE;
	fmgr->irqcount = 0;
	fmgr->frame_seqnum = 0;
	atomic_set(&fmgr->request, CMD_INVALID);
	atomic_set(&fmgr->current_state, STREAM_STOP);

	INIT_LIST_HEAD(&fmgr->ready_frame_list);

	fmgr->overscan = NULL;
	fmgr->traceinfo.frame_count = 0;
	fmgr->traceinfo.repeat_flag = 0;

	tasklet_init(&fmgr->refresh_queue_tasklet,
			dec_frame_queue_refresh, (unsigned long)fmgr);

	// recycle_work init
	spin_lock_init(&fmgr->recycle_work.lock);
	INIT_WORK(&fmgr->recycle_work.work, dec_frame_release_process);

	INIT_KFIFO(fmgr->recycle_work.release_frame_fifo);
	error = kfifo_alloc(&fmgr->recycle_work.release_frame_fifo,
			    FRAME_PTR_FIFO_SIZE, GFP_KERNEL);
	if (error) {
		pr_err("frame_queue: kfifo_alloc failed\n");
		dec_terminate_frame_queue(fmgr->queue);
		kfree(fmgr);
		return ERR_PTR(-ENOMEM);
	}

	init_waitqueue_head(&fmgr->wait_queue);

	return fmgr;
}

static void dec_frame_manager_free(struct dec_frame_manager *fmgr)
{
	unsigned long flags;
	struct list_head tmp;
	video_frame_t *frame, *next;
	video_frame_t *interlace_frames[2];

	spin_lock_irqsave(&fmgr->lock, flags);
	list_replace_init(&fmgr->ready_frame_list, &tmp);

	interlace_frames[0] = fmgr->interlace.frames[0];
	interlace_frames[1] = fmgr->interlace.frames[1];
	fmgr->interlace.frames[0] = 0;
	fmgr->interlace.frames[1] = 0;
	fmgr->type = STREAM_PROGRESSIVE;

	fmgr->traceinfo.repeat_flag = 0;
	fmgr->traceinfo.frame_count = 0;

	spin_unlock_irqrestore(&fmgr->lock, flags);

	list_for_each_entry_safe(frame, next, &tmp, link) {
		list_del(&frame->link);
		video_frame_put(frame);
	}

	if (interlace_frames[0])
		video_frame_put(interlace_frames[0]);

	if (interlace_frames[1])
		video_frame_put(interlace_frames[1]);
}

int dec_frame_manager_exit(struct dec_frame_manager *fmgr)
{

	tasklet_kill(&fmgr->refresh_queue_tasklet);

	flush_work(&fmgr->recycle_work.work);
	kfifo_free(&fmgr->recycle_work.release_frame_fifo);

	if (fmgr->queue)
		dec_terminate_frame_queue(fmgr->queue);

	dec_frame_manager_free(fmgr);
	kfree(fmgr);

	return 0;
}

static inline void dec_frame_queue_resume(struct dec_frame_manager *fmgr)
{
	stream_state_t current_state = STREAM_STOP;

	current_state = atomic_read(&fmgr->current_state);
	switch (current_state) {
	case STREAM_STOP:
		dec_start_frame_queue(fmgr);
		break;
	case STREAM_PAUSE:
		atomic_set(&fmgr->current_state, STREAM_PLAYING);
		break;
	default:
		break;
	}
}

int dec_frame_manager_enqueue_frame(
		struct dec_frame_manager *fmgr, void *private, video_frame_release_cb cb)
{
	video_frame_t *frame = dec_create_video_frame(fmgr->frame_seqnum++);

	if (IS_ERR_OR_NULL(frame)) {
		return -ENOMEM;
	}

	frame->private = private;
	frame->release_pfn = cb;
	dec_frame_push(fmgr, frame);

	dec_frame_queue_resume(fmgr);

	return 0;
}

int dec_frame_manager_set_overscan(struct dec_frame_manager *fmgr, struct overscan_info *info)
{
	unsigned long flags;
	void *legacy = NULL;

	if (IS_ERR_OR_NULL(info)) {
		return -EINVAL;
	}

	spin_lock_irqsave(&fmgr->lock, flags);

	if (fmgr->overscan)
		legacy = fmgr->overscan;
	fmgr->overscan = info;

	spin_unlock_irqrestore(&fmgr->lock, flags);

	if (legacy)
		kfree(legacy);

	return 0;
}

struct overscan_info *dec_frame_manager_get_overscan(struct dec_frame_manager *fmgr)
{
	unsigned long flags;
	struct overscan_info *info = NULL;

	spin_lock_irqsave(&fmgr->lock, flags);
	info = fmgr->overscan;
	fmgr->overscan = NULL;
	spin_unlock_irqrestore(&fmgr->lock, flags);

	return info;
}

int dec_frame_manager_enqueue_interlace_frame(
		struct dec_frame_manager *fmgr, void *top, void *bottom, video_frame_release_cb cb)
{
	unsigned long flags;
	int i;
	video_frame_t *frames[2];
	video_frame_t *legacy;
	struct frame_recycle_work *recycle_work = &fmgr->recycle_work;

	if (fmgr->type != STREAM_INTERLACED) {
		return -EINVAL;
	}

	frames[0] = dec_create_video_frame(fmgr->frame_seqnum++);
	frames[1] = dec_create_video_frame(fmgr->frame_seqnum++);

	if (IS_ERR_OR_NULL(frames[0]) || IS_ERR_OR_NULL(frames[1])) {
		return -ENOMEM;
	}

	frames[0]->private = top;
	frames[0]->release_pfn = cb;

	frames[1]->private = bottom;
	frames[1]->release_pfn = cb;

	spin_lock_irqsave(&fmgr->lock, flags);
	fmgr->interlace.enable = 1;
	fmgr->interlace.frame_count = 0;

	/* remove the legacy interlace frames */
	for (i = 0; i < 2; i++) {
		legacy = fmgr->interlace.frames[i];
		if (legacy) {
			kfifo_in_spinlocked(&recycle_work->release_frame_fifo,
					&legacy, 1,
					&recycle_work->lock);
			legacy = NULL;
			fmgr->interlace.frames[i] = NULL;
		}
	}
	schedule_work(&recycle_work->work);

	fmgr->interlace.frames[0] = frames[0];
	fmgr->interlace.frames[1] = frames[1];

	spin_unlock_irqrestore(&fmgr->lock, flags);

	dec_frame_queue_resume(fmgr);

	return 0;
}

static int dec_video_strame_is_stopped(struct dec_frame_manager *fmgr)
{
	return atomic_read(&fmgr->current_state) == STREAM_STOP;
}

int dec_frame_manager_stop(struct dec_frame_manager *fmgr)
{
	int ret;
	stream_state_t current_state = atomic_read(&fmgr->current_state);

	if (current_state == STREAM_STOP)
		return 0;

	if (current_state == STREAM_PLAYING || current_state == STREAM_PAUSE) {
		atomic_set(&fmgr->request, CMD_STOP);
		atomic_set(&fmgr->current_state, STREAM_EXITING);
	}

	ret = wait_event_timeout(fmgr->wait_queue,
			dec_video_strame_is_stopped(fmgr),
			msecs_to_jiffies(10000));
	DEC_TRACE_FRAME("wait_stop", ret);

	if (ret > 0)
		dec_frame_manager_free(fmgr);

	return 0;
}

int dec_frame_manager_set_stream_type(struct dec_frame_manager *fmgr, stream_type_t type)
{
	unsigned long flags;
	spin_lock_irqsave(&fmgr->lock, flags);
	fmgr->type = type;
	spin_unlock_irqrestore(&fmgr->lock, flags);
	return 0;
}

int dec_frame_manager_register_sync_cb(struct dec_frame_manager *fmgr,
		struct afbd_reg_t *base, video_frame_sync_cb cb)
{
	unsigned long flags;
	struct frame_queue *queue = fmgr->queue;

	spin_lock_irqsave(&queue->frame_lock, flags);
	queue->base = base;
	queue->hardware_sync_pfn = cb;
	spin_unlock_irqrestore(&queue->frame_lock, flags);

	return 0;
}

int dec_frame_manager_register_repeat_ctrl_pfn(struct dec_frame_manager *fmgr,
		struct afbd_reg_t *base, video_frame_repeat_ctrl pfn)
{
	unsigned long flags;
	struct frame_queue *queue = fmgr->queue;

	spin_lock_irqsave(&queue->frame_lock, flags);
	queue->repeat_ctrl_pfn = pfn;
	spin_unlock_irqrestore(&queue->frame_lock, flags);

	return 0;
}

int dec_frame_manager_dump(struct dec_frame_manager *fmgr, char *strbuf)
{
	unsigned long flags;
	video_frame_t *frame, *tmp;
	struct frame_queue *queue;

	int index;
	int printed = 0;

	printed += sprintf(strbuf + printed, "video frame manager info:\n");
	printed += sprintf(strbuf + printed, "           type: %d\n", fmgr->type);
	printed += sprintf(strbuf + printed, "  current state: %d\n", atomic_read(&fmgr->current_state));
	printed += sprintf(strbuf + printed, "        request: %d\n", atomic_read(&fmgr->request));
	printed += sprintf(strbuf + printed, "   frame seqnum: %d\n", fmgr->frame_seqnum);
	printed += sprintf(strbuf + printed, "       irqcount: %lld\n", fmgr->irqcount);

	printed += sprintf(strbuf + printed, "\nready frame list:\n");
	spin_lock_irqsave(&fmgr->lock, flags);
	list_for_each_entry_safe (frame, tmp, &fmgr->ready_frame_list, link) {
		printed += sprintf(strbuf + printed, "  %pK seqnum: 0x%08x\n", frame, frame->frame_number);
	}

	printed += sprintf(strbuf + printed, "\ninterlace frames info:\n");
	for (index = 0; index < 2; index++) {
		frame = fmgr->interlace.frames[index];
		if (frame)
			printed += sprintf(strbuf + printed, "  [%d] %pK seqnum: 0x%08x\n", index, frame, frame->frame_number);
	}

	spin_unlock_irqrestore(&fmgr->lock, flags);


	printed += sprintf(strbuf + printed, "\nframe queue info:\n");

	queue = fmgr->queue;
	spin_lock_irqsave(&queue->frame_lock, flags);
	printed += sprintf(strbuf + printed, "  hardwa repeat: %d\n", queue->hardware_repeat_enable);
	printed += sprintf(strbuf + printed, "      dirty: %d\n", queue->dirty);
	printed += sprintf(strbuf + printed, "     repeat: %d\n", queue->trace_info.repeat_count);
	printed += sprintf(strbuf + printed, "  fifo size: %d\n", kfifo_len(&queue->pending_release_fifo));

	for (index = 0; index < FRAME_QUEUE_DEPTH; index++) {
		frame = queue->frames[index];
		if (frame)
			printed += sprintf(strbuf + printed, "  [%d] %pK seqnum: 0x%08x\n", index, frame, frame->frame_number);
	}
	spin_unlock_irqrestore(&queue->frame_lock, flags);

	return printed;
}

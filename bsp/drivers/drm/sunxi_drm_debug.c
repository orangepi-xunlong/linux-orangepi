/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
#include <drm/drm_gem_cma_helper.h>
#else
#include <drm/drm_gem_dma_helper.h>
#endif

#include <drm/drm_fourcc.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <drm/drm_framebuffer.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <sunxi-iommu.h>
#include "sunxi_drm_debug.h"

typedef struct trace_buf_info {
	u32 seq;
	u64 update_ts;
	u32 plane;
	u32 cur_line;
	u32 format;
	unsigned int width;
	unsigned int height;
	unsigned int pitches[3];
	unsigned int offsets[3];
	dma_addr_t dma_addr;

	// drm drm_framebuffer unmap record
	struct drm_framebuffer *fb;
	int name;
	u64 unmap_ts;
	u32 mapped;
} trace_buf_t;

#define DE_MAX_COUNT 2u
#define FRAME_TRACE_LEN 64

typedef struct frame_trace_data {
	u32 crtc;
	u32 framecount;
	int wpos;
	int exception_pos;
	trace_buf_t data[FRAME_TRACE_LEN];
	spinlock_t lock;
} frame_trace_t;

typedef struct sunxidrm_debug_data {
	bool init;
	/* set after iommu exception happend */
	atomic_t exception;
	u64 exception_ts;

	frame_trace_t *frames[DE_MAX_COUNT];
	struct work_struct work;
} sunxidrm_debug_data_t;

static sunxidrm_debug_data_t dbgdat;

static void  __maybe_unused sunxidrm_debug_record_exception(void)
{
	unsigned long flags;
	int id = 0;

	if (atomic_read(&dbgdat.exception)) {
		pr_err("exception had already record !\n");
		return;
	}
	atomic_inc(&dbgdat.exception);
	pr_err("exception record !!!\n");

	dbgdat.exception_ts = ktime_get();

	for (id = 0; id < DE_MAX_COUNT; id++) {
		frame_trace_t *frame = dbgdat.frames[id];
		spin_lock_irqsave(&frame->lock, flags);
		frame->exception_pos = frame->wpos;
		spin_unlock_irqrestore(&frame->lock, flags);
	}

	// schedule dprintk task
	schedule_work(&dbgdat.work);
}

static void sunxidrm_debug_work_func(struct work_struct *work)
{
	int i, id = 0;
	unsigned long flags;

	pr_err("--- SUNXI DRM DEBUG ---\n");

	for (id = 0; id < DE_MAX_COUNT; id++) {
		frame_trace_t *frame = dbgdat.frames[id];
		spin_lock_irqsave(&frame->lock, flags);
		if (frame->wpos == 0) {
			/* skip the unuse DE */
			spin_unlock_irqrestore(&frame->lock, flags);
			continue;
		}
		pr_err("DE-%d exception pos: %d\n", id, frame->exception_pos);
		for (i = 0; i < FRAME_TRACE_LEN; i++) {
			trace_buf_t *tbuf = &frame->data[i];
			pr_err("[%02d] %d ts %lld plane %d line %d format 0x%08x %dx%d %d-%d-%d %d-%d-%d %pad "
			       "map:%d "
			       "unmap-ts:%lld\n",
			       i, tbuf->seq, tbuf->update_ts, tbuf->plane, tbuf->cur_line, tbuf->format, tbuf->width,
			       tbuf->height, tbuf->pitches[0], tbuf->pitches[1], tbuf->pitches[2], tbuf->offsets[0],
			       tbuf->offsets[1], tbuf->offsets[2], &tbuf->dma_addr, tbuf->mapped, tbuf->unmap_ts);
		}
		spin_unlock_irqrestore(&frame->lock, flags);
	}
}

int sunxidrm_debug_show(struct seq_file *sfile, void *offset)
{
	int i, id = 0;
	unsigned long flags;

	if (!dbgdat.init)
		return 0;

	seq_printf(sfile, "SUNXI-DRM exception: %d timestamp %lld\n", atomic_read(&dbgdat.exception),
		   dbgdat.exception_ts);

	for (id = 0; id < DE_MAX_COUNT; id++) {
		frame_trace_t *frame = dbgdat.frames[id];
		spin_lock_irqsave(&frame->lock, flags);
		if (frame->wpos == 0) {
			/* skip the unuse DE */
			spin_unlock_irqrestore(&frame->lock, flags);
			continue;
		}
		seq_printf(sfile, "DE-%d exception pos: %d\n", id, frame->exception_pos);
		for (i = 0; i < FRAME_TRACE_LEN; i++) {
			trace_buf_t *tbuf = &frame->data[i];
			seq_printf(sfile,
				   "[%02d] %d ts %lld plane %d line %d format 0x%08x %dx%d %d-%d-%d %d-%d-%d %pad "
				   "map:%d "
				   "unmap-ts:%lld\n",
				   i, tbuf->seq, tbuf->update_ts, tbuf->plane, tbuf->cur_line, tbuf->format,
				   tbuf->width, tbuf->height, tbuf->pitches[0], tbuf->pitches[1], tbuf->pitches[2],
				   tbuf->offsets[0], tbuf->offsets[1], tbuf->offsets[2], &tbuf->dma_addr, tbuf->mapped,
				   tbuf->unmap_ts);
		}
		seq_putc(sfile, '\n');

		spin_unlock_irqrestore(&frame->lock, flags);
	}
	return 0;
}

void sunxidrm_debug_init(struct platform_device *pdev)
{
	int id = 0;

	for (id = 0; id < DE_MAX_COUNT; id++) {
		dbgdat.frames[id] = kmalloc(sizeof(*dbgdat.frames[0]), GFP_KERNEL | __GFP_ZERO);
		dbgdat.frames[id]->wpos = 0;
		spin_lock_init(&dbgdat.frames[id]->lock);
	}
	atomic_set(&dbgdat.exception, 0);
	INIT_WORK(&dbgdat.work, sunxidrm_debug_work_func);

#if IS_ENABLED(CONFIG_AW_IOMMU)
{
	u32 master = 0;
	// get iommu master id from dts
	if (of_property_read_u32_index(pdev->dev.of_node, "iommus", 1, &master)) {
		dev_err(&pdev->dev, "of_property_read_u32_index iommus failed\n");
	} else {
		dev_info(&pdev->dev, "register iommu fault callback for sunxi-drm, master=%d\n", master);
		sunxi_iommu_register_fault_cb(sunxidrm_debug_record_exception, master);
	}
}
#endif

	dbgdat.init = 1;
}

void sunxidrm_debug_term(void)
{
	int id = 0;

	dbgdat.init = 0;
	for (id = 0; id < DE_MAX_COUNT; id++) {
		kfree(dbgdat.frames[id]);
	}
}

void sunxidrm_debug_trace_begin(u32 crtc)
{
	unsigned long flags;

	if (unlikely(crtc >= DE_MAX_COUNT))
		return;

	spin_lock_irqsave(&dbgdat.frames[crtc]->lock, flags);
	dbgdat.frames[crtc]->framecount += 1;
	spin_unlock_irqrestore(&dbgdat.frames[crtc]->lock, flags);
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
static inline dma_addr_t get_gem_device_addr(struct drm_gem_object *obj)
{
	struct drm_gem_cma_object *cma_obj;
	cma_obj = to_drm_gem_cma_obj(obj);
	return cma_obj->paddr;
}
#else
static inline dma_addr_t get_gem_device_addr(struct drm_gem_object *obj)
{
	struct drm_gem_dma_object *dma_obj;
	dma_obj = to_drm_gem_dma_obj(obj);
	return dma_obj->dma_addr;
}
#endif

void sunxidrm_debug_trace_frame(u32 crtc, u32 plane, struct drm_framebuffer *fb)
{
	unsigned long flags;
	frame_trace_t *frame = NULL;
	int index = 0;

	if (unlikely(atomic_read(&dbgdat.exception) || (!dbgdat.init)))
		return;
	if (unlikely(crtc >= DE_MAX_COUNT))
		return;

	frame = dbgdat.frames[crtc];

	spin_lock_irqsave(&frame->lock, flags);
	index = frame->wpos % FRAME_TRACE_LEN;
	memset(&frame->data[index], 0, sizeof(trace_buf_t));
	frame->data[index].seq = frame->framecount;
	frame->data[index].plane = plane;
	frame->data[index].update_ts = ktime_get();
	frame->data[index].fb = fb;
	frame->data[index].mapped = 1;
	if (fb && fb->format) {
		frame->data[index].format = fb->format->format;
		frame->data[index].width = fb->width;
		frame->data[index].height = fb->height;
		frame->data[index].pitches[0] = fb->pitches[0];
		frame->data[index].pitches[1] = fb->pitches[1];
		frame->data[index].pitches[2] = fb->pitches[2];
		frame->data[index].offsets[0] = fb->offsets[0];
		frame->data[index].offsets[1] = fb->offsets[1];
		frame->data[index].offsets[2] = fb->offsets[2];
	}
	if (fb->obj[0]) {
		frame->data[index].dma_addr = get_gem_device_addr(fb->obj[0]);
		frame->data[index].name = fb->obj[0]->name;
	}

	frame->wpos++;
	spin_unlock_irqrestore(&frame->lock, flags);
}

void sunxidrm_debug_trace_framebuffer_unmap(struct drm_framebuffer *fb)
{
	int i, id = 0;
	unsigned long flags;

	if (unlikely(atomic_read(&dbgdat.exception) || (!dbgdat.init)))
		return;

	if (fb) {
		if (fb->obj[0]) {
			dma_addr_t dma_addr = get_gem_device_addr(fb->obj[0]);
			for (id = 0; id < DE_MAX_COUNT; id++) {
				frame_trace_t *frame = dbgdat.frames[id];
				spin_lock_irqsave(&frame->lock, flags);
				if (frame->wpos == 0) {
					/* skip the unuse DE */
					spin_unlock_irqrestore(&frame->lock, flags);
					continue;
				}
				for (i = 0; i < FRAME_TRACE_LEN; i++) {
					if (frame->data[i].fb == fb && frame->data[i].name == fb->obj[0]->name &&
					    frame->data[i].dma_addr == dma_addr) {
						frame->data[i].mapped = 0;
						frame->data[i].unmap_ts = ktime_get();
					}
				}
				spin_unlock_irqrestore(&frame->lock, flags);
			}
		}
	}
}


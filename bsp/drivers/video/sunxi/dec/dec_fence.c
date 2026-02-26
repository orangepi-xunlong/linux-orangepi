/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dec_fence.c
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

#include <linux/atomic.h>
#include <linux/dma-fence.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/sync_file.h>

struct dec_fence_context {
	char name[32];
	unsigned context;
	uint32_t seqno;
	spinlock_t spinlock;
};

struct dec_fence_context *
dec_fence_context_alloc(const char *name)
{
	struct dec_fence_context *fctx;

	fctx = kzalloc(sizeof(*fctx), GFP_KERNEL);
	if (!fctx)
		return ERR_PTR(-ENOMEM);

	strncpy(fctx->name, name, sizeof(fctx->name));
	fctx->context = dma_fence_context_alloc(1);
	spin_lock_init(&fctx->spinlock);

	return fctx;
}

void dec_fence_context_free(struct dec_fence_context *fctx)
{
	kfree(fctx);
}

struct dec_fence {
	struct dma_fence base;
	atomic_t signaled;
	struct dec_fence_context *fctx;
};

enum dec_fence_state {
	FENCE_NO_SIGNALED = 0,
	FENCE_SIGNALED,
};

static inline struct dec_fence *to_dec_fence(struct dma_fence *fence)
{
	return container_of(fence, struct dec_fence, base);
}

static const char *dec_fence_get_driver_name(struct dma_fence *fence)
{
	return "dec";
}

static const char *dec_fence_get_timeline_name(struct dma_fence *fence)
{
	struct dec_fence *f = to_dec_fence(fence);
	return f->fctx->name;
}

static bool dec_fence_signaled(struct dma_fence *fence)
{
	struct dec_fence *f = to_dec_fence(fence);
	return atomic_read(&f->signaled) == FENCE_SIGNALED;
}

static const struct dma_fence_ops dec_fence_ops = {
	.get_driver_name   = dec_fence_get_driver_name,
	.get_timeline_name = dec_fence_get_timeline_name,
	.signaled		   = dec_fence_signaled,
};

struct dma_fence *
dec_fence_alloc(struct dec_fence_context *fctx)
{
	struct dec_fence *fence;

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence) {
		pr_err("%s: oom!?\n", __func__);
		return NULL;
	}

	atomic_set(&fence->signaled, FENCE_NO_SIGNALED);
	fence->fctx = fctx;

	dma_fence_init(&fence->base, &dec_fence_ops, &fctx->spinlock,
			fctx->context, ++fctx->seqno);

	return &fence->base;
}

void dec_fence_free(struct dma_fence *fence)
{
	dma_fence_put(fence);
}

void dec_fence_signal(struct dma_fence *fence)
{
	unsigned long flags;
	struct dec_fence *f = to_dec_fence(fence);
	struct dec_fence_context *fctx = f->fctx;

	spin_lock_irqsave(&fctx->spinlock, flags);
	dma_fence_signal_locked(fence);
	atomic_set(&f->signaled, FENCE_SIGNALED);
	spin_unlock_irqrestore(&fctx->spinlock, flags);
}

int dec_fence_fd_create(struct dma_fence *fence)
{
	int ret;
	int fd = -1;
	struct sync_file *sync_file = NULL;

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		ret = fd;
		goto err_out;
	}

	sync_file = sync_file_create(fence);
	if (!sync_file) {
		ret = -ENOMEM;
		goto err_out;
	}

	fd_install(fd, sync_file->file);
	return fd;

err_out:
	if (fd >= 0)
		put_unused_fd(fd);
	return ret;
}


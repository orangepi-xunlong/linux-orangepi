/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse buffer manager module
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
#include "buffer_pool.h"

#include <linux/version.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/file.h>
#include <linux/fdtable.h>

struct buffer_node *buffer_node_alloc(struct device *dev, int sizes, int alloc_path)
{
	struct buffer_node *node;
	struct dma_buf *dmabuf;
	struct dma_heap *dmaheap;
	unsigned int align_sizes;

	node = kzalloc(sizeof(struct buffer_node), GFP_KERNEL);
	if (!node) {
		CAR_REVERSE_ERR("alloc failed\n");
		return ERR_PTR(-EINVAL);
	}

	align_sizes = PAGE_ALIGN(sizes);

	if (alloc_path) {
#if IS_ENABLED(CONFIG_AW_IOMMU) && IS_ENABLED(CONFIG_VIN_IOMMU) && IS_ENABLED(CONFIG_VIDEO_SUNXI_VIN_SPECIAL)
		dmaheap = dma_heap_find("system-uncached");
#else
		dmaheap = dma_heap_find("reserved");
#endif
	} else
		dmaheap = dma_heap_find("reserved");

	if (IS_ERR_OR_NULL(dmaheap)) {
		CAR_REVERSE_ERR("failed, size=%u dmaheap=0x%p\n", align_sizes, dmaheap);
		kfree(node);
		return ERR_PTR(-ENOENT);
	}

	dmabuf = dma_heap_buffer_alloc(dmaheap, align_sizes, O_RDWR, 0);

	if (IS_ERR_OR_NULL(dmabuf)) {
		CAR_REVERSE_ERR("failed, size=%u dmabuf=0x%p\n", align_sizes, dmabuf);
		kfree(node);
		return ERR_PTR(-EINVAL);
	}

	node->dmabuf = dmabuf;
	node->dmabuf_fd = dma_buf_fd(dmabuf, O_CLOEXEC);
	node->dev = dev;
	node->size = align_sizes;

	BUFFER_POOL_DBG("node->dmabuf_fd: %d\n", node->dmabuf_fd);

	return node;
}

void free_buffer_node(struct buffer_node *node)
{
	BUFFER_POOL_DBG("node->dmabuf_fd: %d\n", node->dmabuf_fd);

	close_fd(node->dmabuf_fd);
	kfree(node);
}

struct buffer_node *buffer_pool_dequeue_buffer(struct buffer_pool *bp)
{
	struct buffer_node *node = NULL;

	spin_lock(&bp->lock);
	if (!list_empty(&bp->head)) {
		node = list_entry(bp->head.next, struct buffer_node, list);
		list_del(&node->list);
		BUFFER_POOL_DBG("node->dmabuf_fd: %d\n", node->dmabuf_fd);
	}
	spin_unlock(&bp->lock);
	return node;
}

void buffer_pool_queue_buffer(struct buffer_pool *bp, struct buffer_node *node)
{
	BUFFER_POOL_DBG("node->dmabuf_fd: %d\n", node->dmabuf_fd);
	spin_lock(&bp->lock);
	list_add_tail(&node->list, &bp->head);
	spin_unlock(&bp->lock);
}

struct buffer_pool *
alloc_buffer_pool(struct device *dev, int depth, int buf_size, int alloc_path)
{
	int i;
	struct buffer_pool *bp;
	struct buffer_node *node;

	bp = kzalloc(sizeof(struct buffer_pool), GFP_KERNEL);
	if (!bp) {
		CAR_REVERSE_ERR("buffer pool alloc failed\n");
		goto _out;
	}

	bp->depth = depth;
	bp->pool = kzalloc(sizeof(struct buffer_node *) * depth, GFP_KERNEL);
	if (!bp->pool) {
		CAR_REVERSE_ERR("buffer node alloc failed\n");
		kfree(bp);
		bp = NULL;
		goto _out;
	}
	spin_lock_init(&bp->lock);
	BUFFER_POOL_DBG("\n");

	/* alloc memory for buffer node */
	INIT_LIST_HEAD(&bp->head);
	for (i = 0; i < depth; i++) {
		node = buffer_node_alloc(dev, buf_size, alloc_path);
		if (node) {
			list_add(&node->list, &bp->head);
			bp->pool[i] = node;
		}
	}

_out:
	return bp;
}

void free_buffer_pool(struct buffer_pool *bp)
{
	struct buffer_node *node;

	spin_lock(&bp->lock);
	BUFFER_POOL_DBG("\n");
	while (!list_empty(&bp->head)) {
		node = list_entry(bp->head.next, struct buffer_node, list);
		list_del(&node->list);
		free_buffer_node(node);
	}

	spin_unlock(&bp->lock);

	kfree(bp->pool);
	kfree(bp);
}

void rest_buffer_pool(struct buffer_pool *bp)
{
	int i;
	struct buffer_node *node;

	INIT_LIST_HEAD(&bp->head);
	for (i = 0; i < bp->depth; i++) {
		node = bp->pool[i];
		if (node)
			list_add(&node->list, &bp->head);
	}
}

void dump_buffer_pool(struct buffer_pool *bp)
{
	int i = 0;
	struct buffer_node *node;

	spin_lock(&bp->lock);
	list_for_each_entry(node, &bp->head, list) {
		CAR_REVERSE_PRINT("buffer[%d]->dmabuf_fd: %d\n", i++, node->dmabuf_fd);
	}
	spin_unlock(&bp->lock);
}

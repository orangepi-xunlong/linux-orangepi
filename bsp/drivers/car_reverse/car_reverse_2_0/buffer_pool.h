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

#ifndef __BUFFER_POOL_H__
#define __BUFFER_POOL_H__

#include "../../vin/vin-video/vin_video.h"
#include "../../tvd/tvd.h"

struct video_source_buffer {
	struct vin_buffer vin_buf;
	struct tvd_buffer tvd_buf;
	struct list_head list;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	struct buffer_node *node;
	struct device *dev;
	int dmabuf_fd;
	dma_addr_t dma_address;
	void *vir_address;
	unsigned int size;
	enum vb2_buffer_state state;
};

struct buffer_node {
	struct list_head list;
	unsigned int size;
	dma_addr_t dma_address;
	void *vir_address;
	struct dma_buf *dmabuf;
	int dmabuf_fd;
	struct video_source_buffer vsbuf;
	struct device *dev;
};

struct buffer_pool {
	spinlock_t lock;
	struct list_head head;
	int depth;
	struct buffer_node **pool;
};

struct buffer_node *buffer_pool_dequeue_buffer(struct buffer_pool *bp);
void buffer_pool_queue_buffer(struct buffer_pool *bp, struct buffer_node *node);
struct buffer_pool *alloc_buffer_pool(struct device *dev, int depth, int buf_size, int alloc_path);
void free_buffer_pool(struct buffer_pool *bp);
struct buffer_node *buffer_node_alloc(struct device *dev, int size, int alloc_path);
void free_buffer_node(struct buffer_node *node);
void rest_buffer_pool(struct buffer_pool *bp);
void dump_buffer_pool(struct buffer_pool *bp);

#endif

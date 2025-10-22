/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs g2d driver.
 *
 * Copyright (C) 2020 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>

#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>

#define CREATE_TRACE_POINTS
#include "g2d_trace.h"

struct dma_buf_attach_info {
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
};

typedef struct cache_entry {
	struct dma_buf *buf;
	struct dma_buf_attach_info info;
	int map_count;
	struct list_head link; // link into cache_list
} cache_entry_t;

typedef struct dma_buf_cache {
	struct list_head cache_list;
	int size;
	int max_capacity;
	spinlock_t lock;

	struct device *device;

	// debug info
	int cache_hit;
	int cache_miss;

	atomic_t total_cached_entry;
	atomic_t total_remove_entry;
} dam_buf_cache_t;

struct buffer_release_context {
	struct work_struct work;
	struct list_head free_buffer_list;
	spinlock_t lock;
};

struct delayed_work lrucache_refresh_work;

static struct dma_buf_cache *cachepool;
static struct workqueue_struct *buffer_release_wq;
static struct buffer_release_context buffer_release_info;

static int __attach_buf(struct dma_buf *buf, struct dma_buf_attach_info *info);
static int __detach_buf(struct dma_buf *buf, struct dma_buf_attach_info *info);
static void move_to_pending_release_list(struct list_head *head);
static int dma_buf_is_free(struct dma_buf *buf);

struct dma_buf_cache *create_dma_buf_cache(struct device *dev, int capacity)
{
	struct dma_buf_cache *cache =
		(struct dma_buf_cache *)kmalloc(sizeof(struct dma_buf_cache), GFP_KERNEL | __GFP_ZERO);
	if (cache == NULL) {
		pr_err("kmalloc for dma_buf_cache failed\n");
		return NULL;
	}

	cache->max_capacity = capacity;
	cache->size = 0;
	cache->cache_hit = 0;
	cache->cache_miss = 0;
	cache->device = dev;
	atomic_set(&cache->total_cached_entry, 0);
	atomic_set(&cache->total_remove_entry, 0);
	INIT_LIST_HEAD(&cache->cache_list);
	spin_lock_init(&cache->lock);

	return cache;
}

void destroy_dma_buf_cache(struct dma_buf_cache *cache)
{
	// TODO
}

static int dma_buf_cache_size(struct dma_buf_cache *cache)
{
	int size = 0;
	spin_lock_irq(&cache->lock);
	size = cache->size;
	spin_unlock_irq(&cache->lock);
	return size;
}

static inline void cache_entry_get(struct cache_entry *entry)
{
	++entry->map_count;
}

static inline void cache_entry_put(struct cache_entry *entry)
{
	--entry->map_count;
}

static struct cache_entry *dma_buf_cache_get_locked(struct dma_buf_cache *cache, struct dma_buf *buf)
{
	struct cache_entry *retval = NULL;
	struct cache_entry *entry, *next;

	list_for_each_entry_safe(entry, next, &cache->cache_list, link) {
		if (entry->buf == buf) {
			list_del(&entry->link);
			list_add(&entry->link, &cache->cache_list);
			retval = entry;
			break;
		}
	}
	return retval;
}

static void dma_buf_cache_remove_locked(struct dma_buf_cache *cache, struct cache_entry *entry)
{
	list_del(&entry->link);
	--cache->size;
}

static void dma_buf_cache_trim_to_size_locked(struct dma_buf_cache *cache)
{
	struct cache_entry *entry, *next;
	struct list_head pending_free_list;
	int exceeds = 0;

	INIT_LIST_HEAD(&pending_free_list);

	// remove all idle entry first
	list_for_each_entry_safe(entry, next, &cache->cache_list, link) {
		if (entry->map_count == 0 && dma_buf_is_free(entry->buf)) {
			dma_buf_cache_remove_locked(cache, entry);
			list_add(&entry->link, &pending_free_list);
		}
	}

	if (cache->size > cache->max_capacity) {
		exceeds = cache->size - cache->max_capacity;

		// iterate backwards, so that we will delete the oldlest item
		// of the cache list.
		list_for_each_entry_safe_reverse(entry, next, &cache->cache_list, link) {
			if (entry->map_count == 0) {
				dma_buf_cache_remove_locked(cache, entry);

				// move this entry to pending list
				list_add(&entry->link, &pending_free_list);

				if (--exceeds == 0)
					break;
			}
		}
	}

	// the idle entry will be release by workqueue
	move_to_pending_release_list(&pending_free_list);
}

static void dma_buf_cache_put_locked(struct dma_buf_cache *cache, struct cache_entry *entry)
{
	list_add(&entry->link, &cache->cache_list);
	++cache->size;

	dma_buf_cache_trim_to_size_locked(cache);
}

int g2d_buf_cached_map(int fd, dma_addr_t *addr)
{
	int hit = 0;
	struct cache_entry *entry;
	struct dma_buf *buf = dma_buf_get(fd);

	if (IS_ERR(buf)) {
		pr_err("dma_buf_get failed, fd=%d\n", fd);
		return -EINVAL;
	}

	G2D_TRACE_BEGIN("dma_buf_map");

	spin_lock_irq(&cachepool->lock);
	entry = dma_buf_cache_get_locked(cachepool, buf);
	if (entry) {
		cache_entry_get(entry);
		*addr = entry->info.dma_addr;
		cachepool->cache_hit++;
		hit = 1;
	}
	spin_unlock_irq(&cachepool->lock);

	if (!hit) {
		struct cache_entry *entry =
			(struct cache_entry *)kmalloc(sizeof(struct cache_entry), GFP_KERNEL | __GFP_ZERO);
		if (entry == NULL) {
			pr_err("kmalloc for cache_entry failed\n");
			dma_buf_put(buf);
			G2D_TRACE_END("");
			return -ENOMEM;
		}
		if (__attach_buf(buf, &entry->info) != 0) {
			dma_buf_put(buf);
			G2D_TRACE_END("");
			return -EINVAL;
		}

		*addr = entry->info.dma_addr;
		entry->buf = buf;
		entry->map_count = 0;

		spin_lock_irq(&cachepool->lock);
		cache_entry_get(entry);
		dma_buf_cache_put_locked(cachepool, entry);
		cachepool->cache_miss++;
		spin_unlock_irq(&cachepool->lock);
	} else {
		dma_buf_put(buf);
		dma_buf_begin_cpu_access(entry->buf, DMA_TO_DEVICE);
	}

	G2D_TRACE_END("");
	return 0;
}

int g2d_buf_cached_unmap(int fd, dma_addr_t addr)
{
	struct cache_entry *entry, *next;
	struct dma_buf *buf = dma_buf_get(fd);

	if (IS_ERR(buf)) {
		pr_err("dma_buf_get failed, fd=%d\n", fd);
		return -EINVAL;
	}

	G2D_TRACE_BEGIN("dma_buf_unmap");

	spin_lock_irq(&cachepool->lock);
	list_for_each_entry_safe(entry, next, &cachepool->cache_list, link) {
		if (entry->buf == buf && entry->info.dma_addr == addr) {
			cache_entry_put(entry);
			break;
		}
	}
	dma_buf_cache_trim_to_size_locked(cachepool);
	spin_unlock_irq(&cachepool->lock);

	dma_buf_put(buf);

	G2D_TRACE_END("");
	return 0;
}

static int __attach_buf(struct dma_buf *buf, struct dma_buf_attach_info *info)
{
	info->attachment = dma_buf_attach(buf, cachepool->device);
	if (IS_ERR(info->attachment)) {
		pr_err("dma_buf_attach failed\n");
		return -1;
	}
	info->sgt = dma_buf_map_attachment(info->attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(info->sgt)) {
		pr_err("dma_buf_map_attachment failed\n");
		dma_buf_detach(buf, info->attachment);
		return -1;
	}

	info->dma_addr = sg_dma_address(info->sgt->sgl);
	atomic_inc(&cachepool->total_cached_entry);
	return 0;
}

static int __detach_buf(struct dma_buf *buf, struct dma_buf_attach_info *info)
{
	dma_buf_unmap_attachment(info->attachment, info->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(buf, info->attachment);
	atomic_inc(&cachepool->total_remove_entry);
	return 0;
}

static void move_to_pending_release_list(struct list_head *head)
{
	if (!list_empty(head)) {
		spin_lock_irq(&buffer_release_info.lock);
		list_splice(head, &buffer_release_info.free_buffer_list);
		spin_unlock_irq(&buffer_release_info.lock);
	}

	queue_work(buffer_release_wq, &buffer_release_info.work);
}

static void release_dma_buf_process(struct work_struct *work)
{
	struct cache_entry *entry, *next;
	struct list_head idle_buffers;
	struct buffer_release_context *ctx;

	ctx = container_of(work, struct buffer_release_context, work);

	// move all pending release buffer to idle_buffers list
	spin_lock_irq(&ctx->lock);
	list_replace_init(&ctx->free_buffer_list, &idle_buffers);
	spin_unlock_irq(&ctx->lock);

	list_for_each_entry_safe(entry, next, &idle_buffers, link) {
		__detach_buf(entry->buf, &entry->info);
		dma_buf_put(entry->buf);
		list_del(&entry->link);
		kfree(entry);
	}

	if (dma_buf_cache_size(cachepool)) {
		mod_delayed_work(buffer_release_wq, &lrucache_refresh_work, msecs_to_jiffies(64));
	}
}

static int dma_buf_is_free(struct dma_buf *buf)
{
	// we has a refcounting to this dma_buf on __attach_buf,
	// when the refcounts is equal to 1, it means not any other
	// has refer to this dma_buf.
	return (file_count(buf->file) == 1);
}

static void lrucache_refresh_process(struct work_struct *work)
{
	struct cache_entry *entry, *next;
	struct list_head free_buffers;

	INIT_LIST_HEAD(&free_buffers);

	// move all pending release buffer to idle_buffers list
	spin_lock_irq(&cachepool->lock);

	list_for_each_entry_safe(entry, next, &cachepool->cache_list, link) {
		if (entry->map_count == 0 && dma_buf_is_free(entry->buf)) {
			dma_buf_cache_remove_locked(cachepool, entry);
			list_add(&entry->link, &free_buffers);
		}
	}

	spin_unlock_irq(&cachepool->lock);

	move_to_pending_release_list(&free_buffers);
}

int g2d_buf_cache_init(struct device *dev, int cache_size)
{
	pr_info("lrucahce system init\n");
	cachepool = create_dma_buf_cache(dev, cache_size);

	buffer_release_wq = create_singlethread_workqueue("g2d-lrucache");
	if (!buffer_release_wq) {
		pr_err("workqueue creation failed\n");
		destroy_dma_buf_cache(cachepool);
		cachepool = NULL;
		return -1;
	}

	INIT_LIST_HEAD(&buffer_release_info.free_buffer_list);
	INIT_WORK(&buffer_release_info.work, release_dma_buf_process);
	spin_lock_init(&buffer_release_info.lock);

	INIT_DELAYED_WORK(&lrucache_refresh_work, lrucache_refresh_process);

	return 0;
}

int g2d_buf_cache_exit(void)
{
	if (buffer_release_wq) {
		flush_workqueue(buffer_release_wq);
		destroy_workqueue(buffer_release_wq);
		buffer_release_wq = NULL;
	}

	return 0;
}

size_t g2d_buf_cache_debug_show(char *buf)
{
	int printed = 0;
	printed += sprintf(buf, "LruCache: ");

	spin_lock_irq(&cachepool->lock);

	printed += sprintf(buf + printed, "%d entry, cache hit: %d cache miss: %d\n", cachepool->size,
			   cachepool->cache_hit, cachepool->cache_miss);

	struct cache_entry *entry, *next;
	list_for_each_entry_safe(entry, next, &cachepool->cache_list, link) {
		printed += sprintf(buf + printed, "  %p[ref:%3ld] map count %d\n", entry->buf,
				   file_count(entry->buf->file), entry->map_count);
	}

	spin_unlock_irq(&cachepool->lock);

	printed += sprintf(buf + printed, "Total cached: %d\n", atomic_read(&cachepool->total_cached_entry));
	printed += sprintf(buf + printed, "Total remove: %d\n", atomic_read(&cachepool->total_remove_entry));

	return printed;
}

MODULE_AUTHOR("yajianz@allwinnertech.com");
MODULE_LICENSE("GPL");

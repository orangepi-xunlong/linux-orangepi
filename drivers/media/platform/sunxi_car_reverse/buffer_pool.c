/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/version.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/ion_sunxi.h>
#include <linux/spinlock.h>

#include "car_reverse.h"

#define SUNXI_MEM
#ifdef SUNXI_MEM
#include <linux/ion.h>
#include <linux/ion_sunxi.h>

struct ion_private {
	char *client_name;
	struct ion_client *client;
	struct list_head handle_list;
};

struct ion_memory {
	struct buffer_node node;

	struct list_head list;
	int alloc_size;
	struct ion_handle *handle;
	void *vir_address;
	unsigned long phy_address;
};

static struct ion_private *__ion_private;
static struct ion_private *get_ion_client(void)
{
	const char *client_name = "buffer-pool";

	if (__ion_private)
		return __ion_private;

	__ion_private = kzalloc(sizeof(struct ion_private), GFP_KERNEL);
	if (!__ion_private) {
		logerror("%s: alloc failed\n", __func__);
		return NULL;
	}

	INIT_LIST_HEAD(&__ion_private->handle_list);
	__ion_private->client_name = (char *)client_name;
	__ion_private->client = sunxi_ion_client_create(__ion_private->client_name);
	if (IS_ERR_OR_NULL(__ion_private->client)) {
		logerror("%s: create ion client failed\n", __func__);

		kfree(__ion_private);
		__ion_private = NULL;
		return NULL;
	}

	return __ion_private;
}

static struct buffer_node *__buffer_node_alloc(struct device *dev, int size)
{
	struct ion_memory *mem = NULL;
	int alloc_size = PAGE_ALIGN(size);
	struct ion_private *ion = get_ion_client();

	if (!ion || !size)
		return NULL;

	mem = kzalloc(sizeof(struct ion_memory), GFP_KERNEL);
	if (!mem) {
		logerror("%s: alloc failed\n", __func__);
		return NULL;
	}

	mem->handle = ion_alloc(ion->client, alloc_size, PAGE_SIZE,
						ION_HEAP_CARVEOUT_MASK|ION_HEAP_TYPE_DMA_MASK, 0);
	if (IS_ERR_OR_NULL(mem->handle)) {
		logerror("%s: ion alloc failed\n", __func__);
		goto ion_alloc_err;
	}

	mem->vir_address = ion_map_kernel(ion->client, mem->handle);
	if (IS_ERR_OR_NULL(mem->vir_address)) {
		logerror("%s: ion_map_kernel failed\n", __func__);
		goto ion_map_err;
	}

	if (ion_phys(ion->client, mem->handle, (ion_phys_addr_t *)&mem->phy_address, &alloc_size)) {
		logerror("%s: ion_phys failed\n", __func__);
		goto ion_phys_err;
	}

	mem->alloc_size = alloc_size;
	list_add(&mem->list, &ion->handle_list);

	mem->node.vir_address = mem->vir_address;
	mem->node.phy_address = (void *)mem->phy_address;
	mem->node.size = alloc_size;

	return &mem->node;

ion_phys_err:
	ion_unmap_kernel(ion->client, mem->handle);
ion_map_err:
	ion_free(ion->client, mem->handle);
ion_alloc_err:
	kfree(mem);
	return NULL;
}

static void try_release_ion_client(void)
{
	if (list_empty(&__ion_private->handle_list)) {
		ion_client_destroy(__ion_private->client);
		kfree(__ion_private);
		__ion_private = NULL;
	}
}

static void __buffer_node_free(struct device *dev, struct buffer_node *node)
{
	struct ion_memory *mem = container_of(node, struct ion_memory, node);
	struct ion_private *ion = get_ion_client();

	list_del(&mem->list);
	ion_unmap_kernel(ion->client, mem->handle);
	ion_free(ion->client, mem->handle);
	kfree(mem);

	try_release_ion_client();
}

#else

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 1, 0)
#include <linux/ion_sunxi.h>
static struct buffer_node *__buffer_node_alloc(int size)
{
	struct buffer_node *node = NULL;
	unsigned int phyaddr;

	node = kzalloc(sizeof(struct buffer_node), GFP_KERNEL);
	if (!node) {
		logerror("%s: alloc failed\n", __func__);
		return NULL;
	}

	node->size = PAGE_ALIGN(size);
	node->vir_address = sunxi_buf_alloc(node->size, &phyaddr);
	node->phy_address = (void *)phyaddr;
	if (!node->vir_address) {
		logerror("%s: sunxi_buf_alloc failed!\n", __func__);
		kfree(node);
		return NULL;
	}
	return node;
}

static void __buffer_node_free(struct buffer_node *node)
{
	if (node && node->vir_address) {
		sunxi_buf_free(node->vir_address,
			(unsigned int)node->phy_address, node->size);

		kfree(node);
	}
}

#else

static struct buffer_node *__buffer_node_alloc(struct device *dev, int size)
{
	struct buffer_node *node = NULL;
	unsigned long phyaddr;

	node = kzalloc(sizeof(struct buffer_node), GFP_KERNEL);
	if (!node) {
		logerror("%s: alloc failed\n", __func__);
		return NULL;
	}

	node->size = PAGE_ALIGN(size);
	node->vir_address = dma_alloc_coherent(dev, node->size,
				(dma_addr_t *)&phyaddr, GFP_KERNEL);
	node->phy_address = (void *)phyaddr;
	if (!node->vir_address) {
		logerror("%s: sunxi_buf_alloc failed!\n", __func__);
		kfree(node);
		return NULL;
	}
	return node;
}

static void __buffer_node_free(struct device *dev, struct buffer_node *node)
{
	if (node && node->vir_address && node->phy_address) {
		dma_free_coherent(dev, node->size,
			node->vir_address, (dma_addr_t)node->phy_address);
		kfree(node);
	}
}

#endif
#endif

static struct buffer_node *buffer_pool_dequeue(struct buffer_pool *bp)
{
	struct buffer_node *retval = NULL;

	spin_lock(&bp->lock);
	if (!list_empty(&bp->head)) {
		retval = list_entry(bp->head.next, struct buffer_node, list);
		list_del(&retval->list);
	}
	spin_unlock(&bp->lock);

	return retval;
}

static void
buffer_pool_queue(struct buffer_pool *bp, struct buffer_node *node)
{
	spin_lock(&bp->lock);
	list_add_tail(&node->list, &bp->head);
	spin_unlock(&bp->lock);
}


struct buffer_pool *
alloc_buffer_pool(struct device *dev, int depth, int buf_size)
{
	int i;
	struct buffer_pool *bp;
	struct buffer_node *node;

	bp = kzalloc(sizeof(struct buffer_pool), GFP_KERNEL);
	if (!bp) {
		logerror("%s: alloc failed\n", __func__);
		goto _out;
	}

	bp->depth = depth;
	bp->pool = kzalloc(sizeof(struct buffer_pool *)*depth, GFP_KERNEL);
	if (!bp->pool) {
		logerror("%s: alloc failed\n", __func__);
		kfree(bp);
		bp = NULL;
		goto _out;
	}

	spin_lock_init(&bp->lock);

	bp->queue_buffer   = buffer_pool_queue;
	bp->dequeue_buffer = buffer_pool_dequeue;

	/* alloc memory for buffer node */
	INIT_LIST_HEAD(&bp->head);
	for (i = 0; i < depth; i++) {
		node = __buffer_node_alloc(dev, buf_size);
		if (node) {
			list_add(&node->list, &bp->head);
			bp->pool[i] = node;

			logdebug("%s: [%d] vaddr %p, paddr %p\n", __func__, i,
				node->vir_address, node->phy_address);
		}
	}

_out:
	return bp;
}

void free_buffer_pool(struct device *dev, struct buffer_pool *bp)
{
	struct buffer_node *node;

	spin_lock(&bp->lock);
	while (!list_empty(&bp->head)) {
		node = list_entry(bp->head.next, struct buffer_node, list);
		list_del(&node->list);
		logdebug("%s: free %p\n", __func__, node->phy_address);
		__buffer_node_free(dev, node);
	}
	spin_unlock(&bp->lock);

	kfree(bp);
}

void rest_buffer_pool(struct device *dev, struct buffer_pool *bp)
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

void dump_buffer_pool(struct device *dev, struct buffer_pool *bp)
{
	int i = 0;
	struct buffer_node *node;

	spin_lock(&bp->lock);
	list_for_each_entry(node, &bp->head, list) {
		logdebug("%s: [%d] %p\n", __func__, i++, node->phy_address);
	}
	spin_unlock(&bp->lock);
}

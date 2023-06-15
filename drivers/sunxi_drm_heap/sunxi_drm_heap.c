// SPDX-License-Identifier: GPL-2.0
/*
 * DMABUF System heap exporter
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (C) 2019 Linaro Ltd.
 */

#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/sched/signal.h>
#include <asm/page.h>

#include <linux/sunxi-smc.h>
#include <linux/genalloc.h>

/*
 * google gonna use these source in GKI 2.0 (suppose
 * to lunch in 2021), so these source may encounter
 * namespace conflict in less than one year, just
 * inclue them with all function static, so no conflict
 * with other sources
 * 							--ouyangkun 2020.09.12
 */
#include "third-party/heap.c"
#include "third-party/heap-helpers.c"

struct dma_heap *sunxi_drm_heap;
struct {
	unsigned long drm_base;
	size_t drm_size;
	unsigned long tee_base;
} sunxi_drm_info;

static struct gen_pool *drm_pool;

static void sunxi_drm_heap_free(struct heap_helper_buffer *buffer)
{
//	printk("release buffer at 0x%llx with size 0x%lx\n",
//	       (unsigned long long)buffer->vaddr, buffer->size);
	gen_pool_free(drm_pool, (unsigned long)buffer->vaddr, buffer->size);
	kfree(buffer->pages);
	kfree(buffer);
}

static int sunxi_drm_heap_allocate(struct dma_heap *heap, unsigned long len,
				   unsigned long fd_flags,
				   unsigned long heap_flags)
{
	struct heap_helper_buffer *helper_buffer;
	struct dma_buf *dmabuf;
	int ret = -ENOMEM;
	pgoff_t pg;
	unsigned long va;
	struct page *allocated_page;

	helper_buffer = kzalloc(sizeof(*helper_buffer), GFP_KERNEL);
	if (!helper_buffer)
		return -ENOMEM;

	init_heap_helper_buffer(helper_buffer, sunxi_drm_heap_free);
	helper_buffer->heap = heap;
	helper_buffer->size = len;

	helper_buffer->pagecount = len / PAGE_SIZE;

	va = gen_pool_alloc(drm_pool, helper_buffer->pagecount * PAGE_SIZE);
	if (!va) {
		ret = -ENOMEM;
		goto err0;
	}
	helper_buffer->vaddr = (void *)va;
//	printk("allocate buffer at 0x%llx (phy 0x%llx) with size 0x%lx\n",
//	       (unsigned long long)helper_buffer->vaddr,
//	       gen_pool_virt_to_phys(drm_pool, va), helper_buffer->size);

	helper_buffer->pages =
		kmalloc_array(helper_buffer->pagecount,
			      sizeof(*helper_buffer->pages), GFP_KERNEL);
	if (!helper_buffer->pages) {
		ret = -ENOMEM;
		goto err0;
	}

	allocated_page = phys_to_page(gen_pool_virt_to_phys(drm_pool, va));
	for (pg = 0; pg < helper_buffer->pagecount; pg++) {
		helper_buffer->pages[pg] = &allocated_page[pg];
	}

	/* create the dmabuf */
	dmabuf = heap_helper_export_dmabuf(helper_buffer, fd_flags);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);
		goto err1;
	}

	helper_buffer->dmabuf = dmabuf;

	ret = dma_buf_fd(dmabuf, fd_flags);
	if (ret < 0) {
		dma_buf_put(dmabuf);
		/* just return, as put will call release and that will free */
		return ret;
	}

	return ret;

err1:
	while (pg > 0)
		__free_page(helper_buffer->pages[--pg]);
	kfree(helper_buffer->pages);
err0:
	kfree(helper_buffer);

	return ret;
}

int sunxi_drm_heap_phys(struct dma_heap *heap, int dma_buf_fd,
			unsigned int *tee_addr, unsigned int *phy_addr, unsigned int *len)
{
	struct dma_buf *dma_buf;
	struct heap_helper_buffer *helper;
	dma_buf = dma_buf_get(dma_buf_fd);
	helper = (struct heap_helper_buffer *)dma_buf->priv;
	*phy_addr = page_to_phys(helper->pages[0]);
	*tee_addr = *phy_addr - sunxi_drm_info.drm_base +
		sunxi_drm_info.tee_base;
	*len = helper->size;
	dma_buf_put(dma_buf);
	return 0;
}

static const struct dma_heap_ops sunxi_drm_heap_ops = {
	.allocate = sunxi_drm_heap_allocate,
	.phys = sunxi_drm_heap_phys,
};

static int sunxi_drm_heap_create(void)
{
	struct dma_heap_export_info exp_info;
	int ret = 0;
	ret = optee_probe_drm_configure(&sunxi_drm_info.drm_base,
					&sunxi_drm_info.drm_size,
					&sunxi_drm_info.tee_base);
	if (ret)
		return ret;
	drm_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!drm_pool)
		return -ENOMEM;
	gen_pool_set_algo(drm_pool, gen_pool_best_fit, NULL);
	ret = gen_pool_add_virt(drm_pool, sunxi_drm_info.tee_base,
				sunxi_drm_info.drm_base,
				sunxi_drm_info.drm_size, -1);
	if (ret)
		goto exit;
	ret = dma_heap_init();
	if (ret)
		goto exit;

	exp_info.name = "sunxi_drm_heap";
	exp_info.ops = &sunxi_drm_heap_ops;
	exp_info.priv = NULL;

	sunxi_drm_heap = dma_heap_add(&exp_info);
	if (IS_ERR(sunxi_drm_heap)) {
		ret = PTR_ERR(sunxi_drm_heap);
		goto exit;
	}

	return 0;

exit:
	gen_pool_destroy(drm_pool);
	return ret;
}
module_init(sunxi_drm_heap_create);
MODULE_LICENSE("GPL v2");

/*
 * @File        dma_lma_heap.c
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/genalloc.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/dma-map-ops.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include "dma_lma_heap.h"

struct dma_lma_heap {
	struct dma_heap *heap;
	struct gen_pool *pool;
	phys_addr_t base;
	bool allow_cpu_map;
	phys_addr_t offset;

	bool uncached;
};

struct lma_dma_heap_attachment {
	struct device *dev;
	struct sg_table *table;
	struct list_head list;
	bool mapped;

	bool uncached;
};

struct lma_heap_buffer {
	struct dma_lma_heap *heap;
	struct list_head attachments;
	struct mutex lock;
	unsigned long len;
	struct sg_table *table;
	int vmap_cnt;
	void *vaddr;

	bool uncached;
};

static struct sg_table *dup_sg_table(struct sg_table *table)
{
	struct scatterlist *sg, *new_sg;
	struct sg_table *new_table;
	int ret, i;

	new_table = kzalloc(sizeof(*new_table), GFP_KERNEL);
	if (!new_table)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(new_table, table->orig_nents, GFP_KERNEL);
	if (ret) {
		kfree(new_table);
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_table->sgl;
	for_each_sgtable_sg(table, sg, i) {
		sg_set_page(new_sg, sg_page(sg), sg->length, sg->offset);
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static int lma_heap_attach(struct dma_buf *dmabuf,
		struct dma_buf_attachment *attachment)
{
	struct lma_heap_buffer *buffer =
		(struct lma_heap_buffer *)dmabuf->priv;
	struct lma_dma_heap_attachment *attach;
	struct sg_table *table;

	attach = kzalloc(sizeof(*attach), GFP_KERNEL);
	if (!attach)
		return -ENOMEM;

	table = dup_sg_table(buffer->table);
	if (IS_ERR(table)) {
		kfree(attach);
		return -ENOMEM;
	}

	attach->table = table;
	attach->dev = attachment->dev;
	INIT_LIST_HEAD(&attach->list);
	attach->mapped = false;
	attach->uncached = buffer->uncached;
	attachment->priv = attach;

	mutex_lock(&buffer->lock);
	list_add(&attach->list, &buffer->attachments);
	mutex_unlock(&buffer->lock);

	return 0;
}

static void lma_heap_detach(struct dma_buf *dmabuf,
		struct dma_buf_attachment *attachment)
{
	struct lma_dma_heap_attachment *attach =
		(struct lma_dma_heap_attachment *)attachment->priv;
	struct lma_heap_buffer *buffer =
		(struct lma_heap_buffer *)dmabuf->priv;

	mutex_lock(&buffer->lock);
	list_del(&attach->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(attach->table);
	kfree(attach->table);
	kfree(attach);
}

static struct sg_table *
lma_heap_map_dma_buf(struct dma_buf_attachment *attachment,
		enum dma_data_direction direction)
{
	struct lma_dma_heap_attachment *attach =
		(struct lma_dma_heap_attachment *)attachment->priv;
	struct sg_table *table = attach->table;
	int attr = attachment->dma_map_attrs;
	int ret;

	if (attach->uncached)
		attr |= DMA_ATTR_SKIP_CPU_SYNC;

	ret = dma_map_sgtable(attachment->dev, table, direction, attr);
	if (ret)
		return ERR_PTR(ret);

	attach->mapped = true;

	return table;
}

static void lma_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
		struct sg_table *table,
		enum dma_data_direction direction)
{
	struct lma_dma_heap_attachment *attach =
		(struct lma_dma_heap_attachment *)attachment->priv;
	int attr = attachment->dma_map_attrs;

	if (attach->uncached)
		attr |= DMA_ATTR_SKIP_CPU_SYNC;
	attach->mapped = false;
	dma_unmap_sgtable(attachment->dev, table, direction, attr);
}

static int lma_heap_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
		 enum dma_data_direction direction)
{
	struct lma_heap_buffer *buffer =
		(struct lma_heap_buffer *)dmabuf->priv;
	struct lma_dma_heap_attachment *attach;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		invalidate_kernel_vmap_range(buffer->vaddr, buffer->len);

	if (!buffer->uncached) {
		list_for_each_entry(attach, &buffer->attachments, list) {
			if (!attach->mapped)
				continue;
			dma_sync_sgtable_for_cpu(attach->dev, attach->table, direction);
		}
	}

	mutex_unlock(&buffer->lock);

	return 0;
}

static int lma_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
		enum dma_data_direction direction)
{
	struct lma_heap_buffer *buffer =
		(struct lma_heap_buffer *)dmabuf->priv;
	struct lma_dma_heap_attachment *attach;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		flush_kernel_vmap_range(buffer->vaddr, buffer->len);

	if (!buffer->uncached) {
		list_for_each_entry(attach, &buffer->attachments, list) {
			if (!attach->mapped)
				continue;
			dma_sync_sgtable_for_device(attach->dev, attach->table, direction);
		}
	}

	mutex_unlock(&buffer->lock);

	return 0;
}

static int lma_heap_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct lma_heap_buffer *buffer =
		(struct lma_heap_buffer *)dmabuf->priv;
	struct sg_table *table = buffer->table;
	struct dma_lma_heap *lma_heap;
	struct page *page;
	phys_addr_t paddr;
	int err;

	page = sg_page(table->sgl);
	paddr = PFN_PHYS(page_to_pfn(page));
	lma_heap = buffer->heap;

	mutex_lock(&buffer->lock);

	if (buffer->uncached)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	if (!lma_heap->allow_cpu_map) {
		pr_err("Trying to map_user fake secure ION handle\n");
		return -EPERM;
	}

	/* add the offset to get the real host cpu physical address */
	paddr += lma_heap->offset;

	err = remap_pfn_range(vma, vma->vm_start, PFN_DOWN(paddr) + vma->vm_pgoff,
			vma->vm_end - vma->vm_start,
			pgprot_writecombine(vma->vm_page_prot));
	if (err)
		pr_err("%s: Failed to map buffer to userspace\n", __func__);

	mutex_unlock(&buffer->lock);

	return err;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void *lma_heap_vmap(struct dma_buf *dmabuf)
#else
static int lma_heap_vmap(struct dma_buf *dmabuf, struct dma_buf_map *map)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */
{
	struct lma_heap_buffer *buffer =
		(struct lma_heap_buffer *)dmabuf->priv;
	struct dma_lma_heap *lma_heap;
	__maybe_unused int ret = 0;
	struct sg_table *table;
	struct page *page;
	phys_addr_t paddr;
	void *vaddr;

	table = buffer->table;
	page = sg_page(table->sgl);
	paddr = PFN_PHYS(page_to_pfn(page));
	lma_heap = buffer->heap;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;
		vaddr = buffer->vaddr;
		goto unlock;
	}

	/* add the offset to get the real host cpu physical address */
	paddr += lma_heap->offset;

	if (!lma_heap->allow_cpu_map) {
		pr_err("Trying to map_kernel fake secure ION handle\n");
		vaddr = ERR_PTR(-EPERM);
		goto unlock;
	}

	vaddr = ioremap_wc(paddr, buffer->len);
	if (IS_ERR(vaddr)) {
		pr_err("%s: Failed to map buffer to kernel space\n", __func__);
		ret = -ENOMEM;
		goto unlock;
	}

	buffer->vaddr = vaddr;
	buffer->vmap_cnt++;

unlock:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))
	if (ret == 0)
		dma_buf_map_set_vaddr_iomem(map, vaddr);
	mutex_unlock(&buffer->lock);
	return ret;
#else
	mutex_unlock(&buffer->lock);
	return vaddr;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0) */
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))
static void lma_heap_vunmap(struct dma_buf *dmabuf, struct dma_buf_map *map)
#else
static void lma_heap_vunmap(struct dma_buf *dmabuf, void *vaddr)
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0) */
{
	struct lma_heap_buffer *buffer =
		(struct lma_heap_buffer *)dmabuf->priv;

	mutex_lock(&buffer->lock);

	buffer->vmap_cnt--;

	if (!buffer->vmap_cnt) {
		iounmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))
        dma_buf_map_clear(map);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0) */

	mutex_unlock(&buffer->lock);
}

static void lma_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct lma_heap_buffer *buffer =
		(struct lma_heap_buffer *)dmabuf->priv;
	struct dma_lma_heap *lma_heap;
	struct sg_table *table;
	struct page *page;
	phys_addr_t paddr;

	table = buffer->table;
	page = sg_page(table->sgl);
	paddr = PFN_PHYS(page_to_pfn(page));
	lma_heap = buffer->heap;

	/* Do not zero the LMA heap from the CPU. This is very slow with
	 * the current TCF (w/ no DMA engine). We will use the TLA to clear
	 * the memory with Rogue in another place.
	 *
	 * We also skip the CPU cache maintenance for the heap space, as we
	 * statically know that the TCF PCI memory bar has UC/WC set by the
	 * MTRR/PAT subsystem.
	 */

	gen_pool_free(lma_heap->pool, paddr, buffer->len);
	sg_free_table(table);
	kfree(table);
}

static const struct dma_buf_ops lma_heap_buf_ops = {
	.attach = lma_heap_attach,
	.detach = lma_heap_detach,
	.map_dma_buf = lma_heap_map_dma_buf,
	.unmap_dma_buf = lma_heap_unmap_dma_buf,
	.begin_cpu_access = lma_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = lma_heap_dma_buf_end_cpu_access,
	.mmap = lma_heap_mmap,
	.vmap = lma_heap_vmap,
	.vunmap = lma_heap_vunmap,
	.release = lma_heap_dma_buf_release,
};

static struct dma_buf *lma_heap_allocate(struct dma_heap *heap,
		unsigned long len,
		unsigned long fd_flags,
		unsigned long heap_flags)
{
	struct dma_lma_heap *lma_heap = dma_heap_get_drvdata(heap);
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	struct lma_heap_buffer *buffer;
	size_t size = PAGE_ALIGN(len);
	struct dma_buf *dmabuf;
	struct sg_table *table;
	phys_addr_t paddr;
	int ret = -ENOMEM;

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer->attachments);
	mutex_init(&buffer->lock);

	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		goto err_free_buffer;

	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto err_free_table;

	paddr = gen_pool_alloc(lma_heap->pool, size);
	if (!paddr)
		goto err_free_sg_table;

	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(paddr)), size, 0);

	buffer->table = table;
	buffer->len = size;
	buffer->heap = lma_heap;
	buffer->uncached = lma_heap->uncached;

	/* create the dmabuf */
	exp_info.exp_name = dma_heap_get_name(heap);
	exp_info.ops = &lma_heap_buf_ops;
	exp_info.size = buffer->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);
		goto err_free_sg_table;
	}

	return dmabuf;

err_free_sg_table:
	sg_free_table(table);
err_free_table:
	kfree(table);
err_free_buffer:
	kfree(buffer);

	return ERR_PTR(ret);
}

static const struct dma_heap_ops lma_heap_ops = {
	.allocate = lma_heap_allocate,
};

struct dma_heap *dma_lma_heap_create(struct tc_dma_heap_info *heap_data)
{
	struct dma_heap_export_info exp_info;
	size_t size = heap_data->size;
	struct dma_lma_heap *lma_heap;
	struct page *page;

	page = pfn_to_page(PFN_DOWN(heap_data->base));

	/* Do not zero the LMA heap from the CPU. This is very slow with
	 * the current TCF (w/ no DMA engine). We will use the TLA to clear
	 * the memory with Rogue in another place.
	 *
	 * We also skip the CPU cache maintenance for the heap space, as we
	 * statically know that the TCF PCI memory bar has UC/WC set by the
	 * MTRR/PAT subsystem.
	 */

	lma_heap = kzalloc(sizeof(*lma_heap), GFP_KERNEL);
	if (!lma_heap)
		return ERR_PTR(-ENOMEM);

	lma_heap->pool = gen_pool_create(12, -1);
	if (!lma_heap->pool) {
		kfree(lma_heap);
		return ERR_PTR(-ENOMEM);
	}

	lma_heap->base = heap_data->base;
	/* Manage the heap in device local physical address space. This is so
	 * the GPU/PDP gets the local view of the memory. Host access will be
	 * adjusted by adding the offset again.
	 */
	lma_heap->offset = (phys_addr_t)heap_data->priv;

	gen_pool_add(lma_heap->pool,
		     lma_heap->base - lma_heap->offset, size, -1);

	lma_heap->allow_cpu_map = heap_data->allow_cpu_map;
	lma_heap->uncached = heap_data->uncached;

	exp_info.name = heap_data->name;
	exp_info.ops = &lma_heap_ops;
	exp_info.priv = lma_heap;

	lma_heap->heap = dma_heap_add(&exp_info);
	if (IS_ERR(lma_heap->heap)) {
		kfree(lma_heap);
		return ERR_PTR(-ENOMEM);
	}

	return lma_heap->heap;
}

void dma_lma_heap_destroy(struct dma_heap *heap)
{
	struct dma_lma_heap *lma_heap = dma_heap_get_drvdata(heap);

	gen_pool_destroy(lma_heap->pool);
	kfree(lma_heap);
}

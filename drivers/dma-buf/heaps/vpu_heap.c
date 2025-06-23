// SPDX-License-Identifier: GPL-2.0
/*
 * DMABUF vpu heap exporter
 *
 * Copyright (C) 2023-2024 Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of_reserved_mem.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/highmem.h>
#include <linux/genalloc.h>

struct vpu_heap {
	struct dma_heap *heap;
	struct gen_pool *pool;
};

struct vpu_buffer {
	struct vpu_heap *heap;
	struct list_head attachments;
	struct mutex lock;
	struct dma_buf *dmabuf;
	size_t size;
	struct sg_table table;
	int vmap_cnt;
	void *vaddr;
};

struct vpu_heaps_attachment {
	struct device *dev;
	struct sg_table *table;
	struct list_head list;
	bool mapped;
};

static struct sg_table *dup_sg_table(struct sg_table *table)
{
	struct sg_table *new_table;
	int ret, i;
	struct scatterlist *sg, *new_sg;

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
		sg_dma_address(new_sg) = sg_dma_address(sg);
		sg_dma_len(new_sg) = sg->length;
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static int vpu_heap_attach(struct dma_buf *dmabuf,
			   struct dma_buf_attachment *attachment)
{
	struct vpu_heaps_attachment *a;
	struct vpu_buffer *buffer = dmabuf->priv;
	struct sg_table *table;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	table = dup_sg_table(&buffer->table);
	if (IS_ERR(table)) {
		kfree(a);
		return -ENOMEM;
	}

	a->table = table;
	a->dev = attachment->dev;
	INIT_LIST_HEAD(&a->list);
	a->mapped = false;

	attachment->priv = a;

	mutex_lock(&buffer->lock);
	list_add(&a->list, &buffer->attachments);
	mutex_unlock(&buffer->lock);

	return 0;
}

static void vpu_heap_detach(struct dma_buf *dmabuf,
			    struct dma_buf_attachment *attachment)
{
	struct vpu_buffer *buffer = dmabuf->priv;
	struct vpu_heaps_attachment *a = attachment->priv;

	mutex_lock(&buffer->lock);
	list_del(&a->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(a->table);
	kfree(a);
}

static
struct sg_table *vpu_heap_map_dma_buf(struct dma_buf_attachment *attachment,
				      enum dma_data_direction direction)
{
	struct vpu_heaps_attachment *a = attachment->priv;
	struct sg_table *table = a->table;
	int ret;

	ret = dma_map_sgtable(attachment->dev, table, direction, 0);
	if (ret)
		return ERR_PTR(-ENOMEM);
	a->mapped = true;
	return a->table;
}

static void vpu_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
				   struct sg_table *table,
				   enum dma_data_direction direction)
{
	struct vpu_heaps_attachment *a = attachment->priv;
	a->mapped = false;
	dma_unmap_sgtable(attachment->dev, table, direction, 0);
}

static int vpu_heap_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
					     enum dma_data_direction direction)
{
	struct vpu_buffer *buffer = dmabuf->priv;
	struct vpu_heaps_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		invalidate_kernel_vmap_range(buffer->vaddr, buffer->size);

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sgtable_for_cpu(a->dev, a->table, direction);
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static int vpu_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
					   enum dma_data_direction direction)
{
	struct vpu_buffer *buffer = dmabuf->priv;
	struct vpu_heaps_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		flush_kernel_vmap_range(buffer->vaddr, buffer->size);

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sgtable_for_device(a->dev, a->table, direction);
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static int vpu_heap_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct vpu_buffer *buffer = dmabuf->priv;
	struct sg_table *table = &buffer->table;
	unsigned long addr = vma->vm_start;
	struct sg_page_iter piter;
	int ret;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	for_each_sgtable_page(table, &piter, vma->vm_pgoff) {
		struct page *page = sg_page_iter_page(&piter);

		ret = remap_pfn_range(vma, addr, page_to_pfn(page), PAGE_SIZE,
				      vma->vm_page_prot);
		if (ret)
			return ret;
		addr += PAGE_SIZE;
	}

	return 0;
}

static void *vpu_heap_do_vmap(struct vpu_buffer *buffer)
{
	struct sg_table *table = &buffer->table;
	int npages = PAGE_ALIGN(buffer->size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct sg_page_iter piter;
	void *vaddr;

	if (!pages)
		return ERR_PTR(-ENOMEM);

	for_each_sgtable_page(table, &piter, 0) {
		WARN_ON(tmp - pages >= npages);
		*tmp++ = sg_page_iter_page(&piter);
	}

	vaddr = vmap(pages, npages, VM_MAP, PAGE_KERNEL);
	vfree(pages);

	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	return vaddr;
}

static int vpu_heap_vmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct vpu_buffer *buffer = dmabuf->priv;
	void *vaddr;
	int ret = 0;

	mutex_lock(&buffer->lock);
	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;
		iosys_map_set_vaddr(map, buffer->vaddr);
		goto out;
	}

	vaddr = vpu_heap_do_vmap(buffer);
	if (IS_ERR(vaddr)) {
		ret = PTR_ERR(vaddr);
		goto out;
	}

	buffer->vaddr = vaddr;
	buffer->vmap_cnt++;
	iosys_map_set_vaddr(map, buffer->vaddr);
out:
	mutex_unlock(&buffer->lock);

	return ret;
}

static void vpu_heap_vunmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct vpu_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	if (!--buffer->vmap_cnt) {
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}
	mutex_unlock(&buffer->lock);
	iosys_map_clear(map);
}

static void vpu_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct vpu_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct scatterlist *sg;
	int i;

	table = &buffer->table;
	for_each_sgtable_sg(table, sg, i) {
		struct page *page = sg_page(sg);

		__free_pages(page, compound_order(page));
	}
	for_each_sg(table->sgl, sg, table->nents, i)
		gen_pool_free(buffer->heap->pool, sg_dma_address(sg), sg_dma_len(sg));
	sg_free_table(table);
	kfree(buffer);
}

static const struct dma_buf_ops vpu_heap_dma_ops = {
	.map_dma_buf = vpu_heap_map_dma_buf,
	.unmap_dma_buf = vpu_heap_unmap_dma_buf,
	.begin_cpu_access = vpu_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = vpu_heap_dma_buf_end_cpu_access,
	.mmap = vpu_heap_mmap,
	.vmap = vpu_heap_vmap,
	.vunmap = vpu_heap_vunmap,
	.release = vpu_heap_dma_buf_release,
	.attach = vpu_heap_attach,
	.detach = vpu_heap_detach,
};

static struct dma_buf *vpu_heap_export_dmabuf(struct vpu_buffer *buffer,
					  int fd_flags, const char *name)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	exp_info.exp_name = name;
	exp_info.ops = &vpu_heap_dma_ops;
	exp_info.size = buffer->size;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;

	return dma_buf_export(&exp_info);
}

static struct dma_buf *vpu_heap_allocate(struct dma_heap *heap,
			     unsigned long len,
			     unsigned long fd_flags,
			     unsigned long heap_flags)
{
	struct vpu_heap *vpu_heap = dma_heap_get_drvdata(heap);
	struct vpu_buffer *vpu_buffer;
	struct dma_buf *dmabuf;
	struct sg_table *table;
	unsigned long size = roundup(len, PAGE_SIZE);
	unsigned long phy_addr = 0;

	vpu_buffer = kzalloc(sizeof(*vpu_buffer), GFP_KERNEL);
	if (!vpu_buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&vpu_buffer->attachments);
	mutex_init(&vpu_buffer->lock);

	vpu_buffer->heap = vpu_heap;
	vpu_buffer->size = len;
	phy_addr = gen_pool_alloc(vpu_heap->pool, size);
	if (!phy_addr)
		goto err;

	table = &vpu_buffer->table;
	if (sg_alloc_table(table, 1, GFP_KERNEL))
		goto err;

	sg_set_page(table->sgl,	phys_to_page(phy_addr),	size, 0);
	sg_dma_address(table->sgl) = phy_addr;
	sg_dma_len(table->sgl) = size;

	/* create the dmabuf */
	dmabuf = vpu_heap_export_dmabuf(vpu_buffer, fd_flags,
								dma_heap_get_name(heap));
	if (IS_ERR(dmabuf))
		goto err;

	vpu_buffer->dmabuf = dmabuf;

	return dmabuf;

err:
	if (phy_addr)
		gen_pool_free(vpu_heap->pool, phy_addr, size);
	kfree(vpu_buffer);

	return dmabuf;
}

static const struct dma_heap_ops vpu_heap_ops = {
	.allocate = vpu_heap_allocate,
};

static int vpu_heap_create_single(const char *name)
{
	struct dma_heap_export_info exp_info;
	struct vpu_heap *vpu_heap;
	struct reserved_mem *rmem;
	struct device_node np;
	struct gen_pool *pool = NULL;

	np.full_name = name;
	np.name = name;
	rmem = of_reserved_mem_lookup(&np);
	if (!rmem) {
		pr_err("of_reserved_mem_lookup() returned NULL\n");
		return -EINVAL;
	}

	if (rmem->base == 0 || rmem->size == 0) {
		pr_err("%s base or size is not correct\n", name);
		return -EINVAL;
	}

	vpu_heap = kzalloc(sizeof(*vpu_heap), GFP_KERNEL);
	if (!vpu_heap)
		return -ENOMEM;

	pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!pool) {
		pr_err("Failed to create gen pool\n");
		kfree(vpu_heap);
		return -ENOMEM;
	}

	if (gen_pool_add(pool, rmem->base, rmem->size, -1) < 0) {
		pr_err("Failed to add reserved memory into pool\n");
		gen_pool_destroy(pool);
		kfree(vpu_heap);
		return -ENOMEM;
	}

	vpu_heap->pool = pool;

	exp_info.name = name;
	exp_info.ops = &vpu_heap_ops;
	exp_info.priv = vpu_heap;
	vpu_heap->heap = dma_heap_add(&exp_info);
	if (IS_ERR(vpu_heap->heap)) {
		int ret = PTR_ERR(vpu_heap->heap);

		gen_pool_destroy(pool);
		kfree(vpu_heap);
		return ret;
	}

	return 0;
}

static int vpu_heap_create(void)
{
	int ret = 0;

	ret = vpu_heap_create_single("vpu_private");
	if (ret)
		return ret;

	ret = vpu_heap_create_single("vpu_protected");
	if (ret)
		return ret;

	ret = vpu_heap_create_single("media_protected");
	if (ret)
		return ret;

	return 0;
}
module_init(vpu_heap_create);
MODULE_LICENSE("GPL v2");

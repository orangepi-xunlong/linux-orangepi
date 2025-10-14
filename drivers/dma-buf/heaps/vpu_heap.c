// SPDX-License-Identifier: GPL-2.0
/*
 * DMABUF vpu heap exporter
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of_reserved_mem.h>
#include <linux/slab.h>
#include <linux/mutex.h>

struct vpu_heap {
	struct dma_heap *heap;
	phys_addr_t base;
	phys_addr_t size;
	unsigned long npages;
	struct mutex page_map_lock;
	unsigned long *page_map;
};

struct vpu_buffer {
	struct dma_heap *heap;
	struct dma_buf *dmabuf;
	phys_addr_t *pages;
	size_t size;
	pgoff_t pagecount;
	struct sg_table table;
};

struct vpu_heaps_attachment {
	struct sg_table table;
};

static int vpu_heap_attach(struct dma_buf *dmabuf,
			   struct dma_buf_attachment *attachment)
{
	struct vpu_heaps_attachment *a;
	struct vpu_buffer *buffer = dmabuf->priv;
	struct scatterlist *s;
	int i;
	int ret;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	ret = sg_alloc_table(&a->table,
			buffer->pagecount, GFP_KERNEL);
	if (ret)
		return ret;

	for_each_sgtable_sg(&a->table, s, i) {
		sg_dma_address(s) = buffer->pages[i];
		sg_dma_len(s) = PAGE_SIZE;
		s->length = PAGE_SIZE;
	}

	attachment->priv = a;

	return 0;
}

static void vpu_heap_detach(struct dma_buf *dmabuf,
			    struct dma_buf_attachment *attachment)
{
	struct vpu_heaps_attachment *a = attachment->priv;
	sg_free_table(&a->table);
	kfree(a);
}

static
struct sg_table *vpu_heap_map_dma_buf(struct dma_buf_attachment *attachment,
				      enum dma_data_direction direction)
{
	struct vpu_heaps_attachment *a = attachment->priv;
	return &a->table;
}

static void vpu_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
				   struct sg_table *table,
				   enum dma_data_direction direction)
{
}

static phys_addr_t vpu_heap_alloc_page(struct vpu_heap *vpu_heap)
{
	unsigned long id;
	phys_addr_t pa;
	mutex_lock(&vpu_heap->page_map_lock);
	id = find_first_zero_bit(vpu_heap->page_map, vpu_heap->npages);
	if (id >= vpu_heap->npages) {
		mutex_unlock(&vpu_heap->page_map_lock);
		return 0;
	}
	set_bit(id, vpu_heap->page_map);
	mutex_unlock(&vpu_heap->page_map_lock);
	pa = vpu_heap->base + (id << PAGE_SHIFT);
	return pa;
}

static int vpu_heap_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct vpu_buffer *buffer = dmabuf->priv;
	int ret;

	/* Make sure its contiguous memory before calling vm_iomap_memory() */
	int i;
	for (i = 1; i < buffer->pagecount; i++) {
		if (buffer->pages[i] - buffer->pages[i-1] != PAGE_SIZE)
			return -EINVAL;
	}

	ret = vm_iomap_memory(vma, buffer->pages[0], buffer->size);
	if (ret < 0)
		return ret;

	return 0;
}

static void vpu_heap_free_page(struct vpu_heap *vpu_heap,
				 phys_addr_t page)
{
	phys_addr_t pa = page;
	unsigned long id = (pa - vpu_heap->base) >> PAGE_SHIFT;
	mutex_lock(&vpu_heap->page_map_lock);
	clear_bit(id, vpu_heap->page_map);
	mutex_unlock(&vpu_heap->page_map_lock);
}

static void vpu_heap_free(struct vpu_buffer *buffer)
{
	struct vpu_heap *vpu_heap = dma_heap_get_drvdata(buffer->heap);
	pgoff_t pg;

	for (pg = 0; pg < buffer->pagecount; pg++)
		vpu_heap_free_page(vpu_heap, buffer->pages[pg]);
	kfree(buffer->pages);
	kfree(buffer);
}

static void vpu_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct vpu_buffer *buffer = dmabuf->priv;

	vpu_heap_free(buffer);
}

static const struct dma_buf_ops vpu_heap_dma_ops = {
	.map_dma_buf = vpu_heap_map_dma_buf,
	.unmap_dma_buf = vpu_heap_unmap_dma_buf,
	.mmap = vpu_heap_mmap,
	.release = vpu_heap_dma_buf_release,
	.attach = vpu_heap_attach,
	.detach = vpu_heap_detach,
};

static struct dma_buf *vpu_heap_export_dmabuf(struct vpu_buffer *buffer,
					  int fd_flags)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

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
	int ret = -ENOMEM;
	pgoff_t pg;

	vpu_buffer = kzalloc(sizeof(*vpu_buffer), GFP_KERNEL);
	if (!vpu_buffer)
		return ERR_PTR(-ENOMEM);

	vpu_buffer->heap = heap;
	vpu_buffer->size = len;
	vpu_buffer->pagecount = len / PAGE_SIZE;
	vpu_buffer->pages = kmalloc_array(vpu_buffer->pagecount,
					     sizeof(*vpu_buffer->pages),
					     GFP_KERNEL);
	if (!vpu_buffer->pages) {
		dmabuf = ERR_PTR(-ENOMEM);
		goto err0;
	}

	for (pg = 0; pg < vpu_buffer->pagecount; pg++) {
		/*
		 * Avoid trying to allocate memory if the process
		 * has been killed by by SIGKILL
		 */
		if (fatal_signal_pending(current))
			goto err1;

		vpu_buffer->pages[pg] = vpu_heap_alloc_page(vpu_heap);
		if (!vpu_buffer->pages[pg])
			goto err1;
	}

	/* create the dmabuf */
	dmabuf = vpu_heap_export_dmabuf(vpu_buffer, fd_flags);
	if (IS_ERR(dmabuf))
		goto err1;

	vpu_buffer->dmabuf = dmabuf;

	ret = dma_buf_fd(dmabuf, fd_flags);
	if (ret < 0) {
		dma_buf_put(dmabuf);
		/* just return, as put will call release and that will free */
		return ERR_PTR(ret);
	}

	return dmabuf;

err1:
	while (pg > 0)
		vpu_heap_free_page(vpu_heap, vpu_buffer->pages[--pg]);
	kfree(vpu_buffer->pages);
err0:
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
	unsigned int npages;

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

	npages = DIV_ROUND_UP(rmem->size, PAGE_SIZE);
	vpu_heap->page_map = kcalloc(BITS_TO_LONGS(npages),
					     sizeof(*vpu_heap->page_map),
					     GFP_KERNEL);
	if (!vpu_heap->page_map) {
		kfree(vpu_heap);
		return -ENOMEM;
	}

	vpu_heap->base = rmem->base;
	vpu_heap->size = rmem->size;
	vpu_heap->npages = npages;
	mutex_init(&vpu_heap->page_map_lock);

	exp_info.name = name;
	exp_info.ops = &vpu_heap_ops;
	exp_info.priv = vpu_heap;
	vpu_heap->heap = dma_heap_add(&exp_info);
	if (IS_ERR(vpu_heap->heap)) {
		int ret = PTR_ERR(vpu_heap->heap);

		kfree(vpu_heap->page_map);
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

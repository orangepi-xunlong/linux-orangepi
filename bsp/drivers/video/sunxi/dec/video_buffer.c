/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2007-2021 Allwinnertech Co., Ltd.
 *
 * Author: Yajiaz <yajianz@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <asm-generic/errno-base.h>
#define pr_fmt(fmt) "video-buffer: " fmt

#include <linux/dma-buf.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/highmem.h>
#include <linux/debugfs.h>
#include <sunxi-iommu.h>
#include <linux/of_reserved_mem.h>
#include <linux/version.h>

#include "dma_buf.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#define DEC_VIDEO_FRAMEBUFFER_FROM_DMA_HEAP (1)
#endif

struct reserved_buffer {
	struct sg_table *sg_table;
	struct page *pages;
	size_t size;
	struct list_head link;
};

struct local_buf_attachment {
	struct device *dev;
	struct sg_table *table;
	bool mapped;
};

struct mapped_buffer {
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t address;

	// link to video_buffer_data->dmabuf_head
	struct list_head link;
};

struct vbibuffer {
	dma_addr_t offset; /* physical address */
	dma_addr_t iommu_address;
	size_t size;
};

struct video_buffer_data {
	struct device *device;
	struct list_head dmabuf_head;

	struct vbibuffer *vbi;
};

struct video_buffer_debug_info {
	struct dentry *dbgfs;
	size_t size;
	uint32_t physical_address;
	dma_addr_t iommu_address;
};

struct video_framebuffer {
	int token;
	size_t size;
	struct dec_dmabuf_t *buffer;
	struct list_head link;
};

struct video_framebuffer_data {
	struct mutex lock;
	struct device *device;
	struct list_head framebuffer_head;
	uint32_t allocated_count;
	int unique_id;
};

static struct video_framebuffer_data allocated_framebuffer;
static struct video_buffer_data video_buffer;
static struct video_buffer_debug_info dbginfo;

static void vb_debug_print(struct seq_file *s);
static void video_framebuffer_init(struct device *pdev);

static int video_buffer_debugfs_show(struct seq_file *s, void *unused)
{
	struct video_framebuffer *buf, *next;
	seq_puts(s, "tvdisp video buffer info:\n");
	seq_printf(s, "              size: 0x%08zx\n", dbginfo.size);
	seq_printf(s, "  physical address: 0x%08x\n", dbginfo.physical_address);
	seq_printf(s, "     iommu address: 0x%08x\n", (uint32_t)dbginfo.iommu_address);

	mutex_lock(&allocated_framebuffer.lock);
	seq_printf(s, "allocated framebuffer: %d\n", allocated_framebuffer.allocated_count);
	list_for_each_entry_safe (
		buf, next, &allocated_framebuffer.framebuffer_head, link) {
		seq_printf(s, "  dma address: %pad size: %zu\n",
			   &buf->buffer->dma_addr, buf->size);
	}
	mutex_unlock(&allocated_framebuffer.lock);

	vb_debug_print(s);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(video_buffer_debugfs);

static int video_buffer_debugfs_init(void)
{
	dbginfo.dbgfs = debugfs_create_dir("video-buffer", NULL);
	debugfs_create_file_unsafe("info", 0444, dbginfo.dbgfs, NULL,
			&video_buffer_debugfs_fops);
	return 0;
}

void video_buffer_init(struct device *pdev)
{
	if (video_buffer.device != NULL) {
		pr_warn("video buffer had already init!\n");
		return;
	}

	video_buffer.device = pdev;
	video_buffer.vbi = NULL;
	INIT_LIST_HEAD(&video_buffer.dmabuf_head);

	video_framebuffer_init(pdev);

	video_buffer_debugfs_init();
}

static struct sg_table *dup_sg_table(struct sg_table *table)
{
	struct sg_table *new_table;
	int ret, i;
	struct scatterlist *sg, *new_sg;

	new_table = kzalloc(sizeof(*new_table), GFP_KERNEL);
	if (!new_table)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(new_table, table->nents, GFP_KERNEL);
	if (ret) {
		kfree(new_table);
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_table->sgl;
	for_each_sg(table->sgl, sg, table->nents, i) {
		memcpy(new_sg, sg, sizeof(*sg));
		new_sg->dma_address = 0;
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static void free_duped_table(struct sg_table *table)
{
	sg_free_table(table);
	kfree(table);
}

static struct reserved_buffer *create_reserved_buffer(uint32_t phy, unsigned long len)
{
	struct reserved_buffer *buffer;
	struct sg_table *table;
	struct page *pages = phys_to_page(phy);
	unsigned long size = PAGE_ALIGN(len);
	unsigned long nr_pages = size >> PAGE_SHIFT;
	int ret;

	if (!pages) {
		pr_err("invalid pages! can not create reserved buffer.");
		return NULL;
	}

	if (PageHighMem(pages)) {
		unsigned long nr_clear_pages = nr_pages;
		struct page *page = pages;

		while (nr_clear_pages > 0) {
			void *vaddr = kmap_atomic(page);

			memset(vaddr, 0, PAGE_SIZE);
			kunmap_atomic(vaddr);
			page++;
			nr_clear_pages--;
		}
	} else {
		memset(page_address(pages), 0, size);
	}

	buffer = kmalloc(sizeof(struct reserved_buffer), GFP_KERNEL | __GFP_ZERO);
	if (buffer == NULL) {
		pr_err("kmalloc for reserved_buffer failed\n");
		return NULL;
	}


	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		goto err;

	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto free_table;

	sg_set_page(table->sgl, pages, size, 0);

	buffer->pages = pages;
	buffer->sg_table = table;
	buffer->size = size;

	INIT_LIST_HEAD(&buffer->link);

	return buffer;

free_table:
	kfree(table);

err:
	kfree(buffer);
	return NULL;
}

static int video_buffer_dma_buf_attach(struct dma_buf *dmabuf,
		struct dma_buf_attachment *attachment)
{
	struct local_buf_attachment *a;
	struct sg_table *table;
	struct reserved_buffer *buffer = dmabuf->priv;

	pr_debug("%s\n", __func__);

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	table = dup_sg_table(buffer->sg_table);
	if (IS_ERR(table)) {
		kfree(a);
		return -ENOMEM;
	}

	a->table = table;
	a->dev = attachment->dev;
	a->mapped = false;

	attachment->priv = a;

	return 0;
}

static void video_buffer_dma_buf_detatch(struct dma_buf *dmabuf,
		struct dma_buf_attachment *attachment)
{
	struct local_buf_attachment *a = attachment->priv;
	free_duped_table(a->table);
	kfree(a);

	pr_debug("%s\n", __func__);
}

static struct sg_table *video_buffer_map_dma_buf(struct dma_buf_attachment *attachment,
					enum dma_data_direction direction)
{
	struct local_buf_attachment *a;
	struct sg_table *table;
	unsigned long attrs = attachment->dma_map_attrs;

	pr_debug("%s\n", __func__);

	a = attachment->priv;
	table = a->table;

	attrs |= DMA_ATTR_SKIP_CPU_SYNC;

	if (!dma_map_sg_attrs(attachment->dev, table->sgl, table->nents,
			      direction, attrs))
		return ERR_PTR(-ENOMEM);

	a->mapped = true;

	return table;
}

static void video_buffer_unmap_dma_buf(struct dma_buf_attachment *attachment,
			      struct sg_table *table,
			      enum dma_data_direction direction)
{
	struct local_buf_attachment *a = attachment->priv;
	unsigned long attrs = attachment->dma_map_attrs;

	pr_debug("%s\n", __func__);

	a->mapped = false;

	attrs |= DMA_ATTR_SKIP_CPU_SYNC;

	dma_unmap_sg_attrs(attachment->dev, table->sgl, table->nents,
			direction, attrs);
}

static void video_buffer_dma_buf_release(struct dma_buf *dmabuf)
{
	struct reserved_buffer *buffer = dmabuf->priv;

	sg_free_table(buffer->sg_table);
	kfree(buffer);

	pr_debug("%s\n", __func__);
}

static const struct dma_buf_ops dma_buf_ops = {
	.attach = video_buffer_dma_buf_attach,
	.detach = video_buffer_dma_buf_detatch,
	.map_dma_buf = video_buffer_map_dma_buf,
	.unmap_dma_buf = video_buffer_unmap_dma_buf,
	.release = video_buffer_dma_buf_release,
};

static struct dma_buf *video_buffer_create_dmabuf(uint32_t phy, size_t len)
{
	struct reserved_buffer *buffer;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	struct dma_buf *dmabuf;

	pr_debug("%s: phy %08x len %zu\n", __func__, phy, len);

	buffer = create_reserved_buffer(phy, len);
	if (IS_ERR(buffer))
		return ERR_CAST(buffer);

	exp_info.ops = &dma_buf_ops;
	exp_info.size = buffer->size;
	exp_info.flags = O_RDWR;
	exp_info.priv = buffer;

	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf))
		pr_info("dma_buf_export error!\n");

	return dmabuf;
}

int video_buffer_map(uint32_t phy, size_t len, dma_addr_t *device_addr)
{
	struct mapped_buffer *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int ret = -1;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	INIT_LIST_HEAD(&buf->link);

	buf->dmabuf = video_buffer_create_dmabuf(phy, len);

	if (IS_ERR(buf->dmabuf)) {
		pr_err("create video dmabuf failed!\n");
		ret = -EINVAL;
		goto err;
	}

	attachment = dma_buf_attach(buf->dmabuf, video_buffer.device);
	if (IS_ERR_OR_NULL(attachment)) {
		pr_err("dma_buf_attach failed\n");
		ret = -ENOMEM;
		goto err_attach;
	}

	sgt = dma_buf_map_attachment(attachment, DMA_TO_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		pr_err("dma_buf_map_attachment failed\n");
		ret = -ENOMEM;
		goto err_buf_detach;
	}

	buf->address = sg_dma_address(sgt->sgl);
	buf->attachment = attachment;
	buf->sgt = sgt;

	list_add_tail(&buf->link, &video_buffer.dmabuf_head);
	*device_addr = buf->address;

	pr_debug("video_buffer_map: phy %08x --> device addr %pad\n",
			phy, device_addr);

	dbginfo.size = len;
	dbginfo.physical_address = phy;
	dbginfo.iommu_address = buf->address;

	return 0;

err_buf_detach:
	dma_buf_detach(buf->dmabuf, attachment);
err_attach:
	dma_buf_put(buf->dmabuf);
err:
	kfree(buf);
	return ret;
}

void video_buffer_unmap(dma_addr_t device_addr)
{
	struct mapped_buffer *buf, *next;
	struct mapped_buffer *target = NULL;

	list_for_each_entry_safe(buf, next, &video_buffer.dmabuf_head, link) {
		if (buf->address == device_addr) {
			list_del(&buf->link);
			target = buf;
			break;
		}
	}

	if (target) {
		dma_buf_unmap_attachment(target->attachment, target->sgt, DMA_TO_DEVICE);
		dma_buf_detach(target->dmabuf, target->attachment);
		dma_buf_put(target->dmabuf);
		kfree(target);
	}
}


static int vbibuffer_reserved_mem_get(
		struct device *dev, dma_addr_t *addr, size_t *size)
{
	struct device_node *mem_node;
	struct reserved_mem *rmem;

	mem_node = of_parse_phandle(dev->of_node, "memory-region", 1);
	if (!mem_node) {
		dev_err(dev, "No memory-region found\n");
		return -ENODEV;
	}

	rmem = of_reserved_mem_lookup(mem_node);
	if (!rmem) {
		dev_err(dev, "of_reserved_mem_lookup() returned NULL\n");
		return -ENODEV;
	}
	*addr = rmem->base;
	*size = rmem->size;
	dev_info(dev, "vbi-buffer: %pad size %zu\n", addr, *size);
	return 0;
}

int dec_vbi_buffer_query(uint32_t *offset, uint64_t *ioaddr, uint32_t *size)
{
	int ret = 0;
	struct vbibuffer *vbi;

	if (video_buffer.vbi == NULL) {
		vbi = kmalloc(sizeof(*vbi), GFP_KERNEL | __GFP_ZERO);
		if (IS_ERR_OR_NULL(vbi)) {
			pr_err("vbi allocate failed: out of memory\n");
			return -ENOMEM;
		}
		ret = vbibuffer_reserved_mem_get(video_buffer.device,
						 &vbi->offset, &vbi->size);
		if (ret != 0) {
			pr_err("vbibuffer reserved mem lookup failed !\n");
			kfree(vbi);
			return -EINVAL;
		}

		ret = video_buffer_map(vbi->offset, vbi->size, &vbi->iommu_address);
		if (ret != 0) {
			kfree(vbi);
			return -EINVAL;
		}
		video_buffer.vbi = vbi;
	}

	*offset = video_buffer.vbi->offset;
	*ioaddr = video_buffer.vbi->iommu_address;
	*size = video_buffer.vbi->size;

	return 0;
}

// allocate system buffer as svp framebuffer

#ifdef DEC_VIDEO_FRAMEBUFFER_FROM_DMA_HEAP
#include <linux/dma-heap.h>

#if IS_ENABLED(CONFIG_AW_IOMMU)
#define ALLOC_HEAP_NAME "system"
#else
#define ALLOC_HEAP_NAME "reserved"
#endif

#else
#include <linux/ion.h>
#include <uapi/linux/ion.h>
#endif

static void vb_debug_init(void);
static void vb_debug_trace_dma_buf_map(int token, uint32_t addr, uint32_t size);
static void vb_debug_trace_dma_buf_unmap(int token, uint32_t addr);

static void video_framebuffer_init(struct device *pdev)
{
	mutex_init(&allocated_framebuffer.lock);
	INIT_LIST_HEAD(&allocated_framebuffer.framebuffer_head);
	allocated_framebuffer.device = pdev;
	allocated_framebuffer.allocated_count = 0;
	allocated_framebuffer.unique_id = 0;

	vb_debug_init();
}

int video_framebuffer_allocate(size_t len, dma_addr_t *device_addr)
{
	struct dma_buf *dmabuf;
	struct video_framebuffer *buffer;

#ifdef DEC_VIDEO_FRAMEBUFFER_FROM_DMA_HEAP
	struct dma_heap *heap = dma_heap_find(ALLOC_HEAP_NAME);
	if (IS_ERR_OR_NULL(heap)) {
		pr_err("video_framebuffer_allocate failed: can't find dma-heap '%s'\n", ALLOC_HEAP_NAME);
		return -ENODEV;
	}
	dmabuf = dma_heap_buffer_alloc(heap, len, O_CLOEXEC, 0);
	dma_heap_put(heap);
#else
	unsigned int flags = 0;
	unsigned int heap_id_mask = 1 << ION_HEAP_TYPE_SYSTEM;
	dmabuf = ion_alloc(len, heap_id_mask, flags);
#endif

	if (IS_ERR_OR_NULL(dmabuf)) {
		pr_err("video_framebuffer_allocate failed: size = %zu\n", len);
		return -ENOMEM;
	}

	buffer = kmalloc(sizeof(*buffer), GFP_KERNEL | __GFP_ZERO);
	if (IS_ERR_OR_NULL(buffer)) {
		pr_err("video_framebuffer_allocate failed: out of memory\n");
		dma_buf_put(dmabuf);
		return -ENOMEM;
	}

	buffer->buffer = dec_dma_map_dmabuf(dmabuf, allocated_framebuffer.device);
	if (IS_ERR_OR_NULL(buffer->buffer)) {
		pr_err("video_framebuffer_allocate failed: dec_dma_map error!\n");
		kfree(buffer);
		dma_buf_put(dmabuf);
		return -ENOMEM;
	}

	buffer->size = len;

	mutex_lock(&allocated_framebuffer.lock);
	list_add_tail(&buffer->link, &allocated_framebuffer.framebuffer_head);
	allocated_framebuffer.allocated_count += 1;
	buffer->token = allocated_framebuffer.unique_id++;
	mutex_unlock(&allocated_framebuffer.lock);

	*device_addr = buffer->buffer->dma_addr;

	vb_debug_trace_dma_buf_map(buffer->token, buffer->buffer->dma_addr, buffer->size);

	return 0;
}

int video_framebuffer_free(dma_addr_t device_addr)
{
	struct video_framebuffer *buf, *next;
	struct video_framebuffer *target = NULL;

	mutex_lock(&allocated_framebuffer.lock);
	list_for_each_entry_safe (
		buf, next, &allocated_framebuffer.framebuffer_head, link) {
		if (buf->buffer->dma_addr == device_addr) {
			list_del(&buf->link);
			allocated_framebuffer.allocated_count -= 1;
			target = buf;
			break;
		}
	}
	mutex_unlock(&allocated_framebuffer.lock);

	if (target) {
		vb_debug_trace_dma_buf_unmap(buf->token, buf->buffer->dma_addr);
		dec_dma_unmap(buf->buffer);
		kfree(buf);
	}

	return 0;
}

typedef struct video_buf_info {
	int token;
	uint32_t iommu_addr;
	uint32_t length;
	int map;
} video_buf_info_t;

#define VIDEO_INFO_TRACE_LEN 64
typedef struct video_buf_trace_data {
	spinlock_t lock;
	int wpos;
	video_buf_info_t data[VIDEO_INFO_TRACE_LEN];

	int exception;
	int error_pos;
	int error_map;
	struct work_struct work;
} video_buf_trace_t;

static video_buf_trace_t vb_dbgdat;

static void vb_debug_print(struct seq_file *s)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&vb_dbgdat.lock, flags);

	seq_printf(s, "video buffer map info: wpos=%d\n", vb_dbgdat.wpos % VIDEO_INFO_TRACE_LEN);
	seq_printf(s, "video buffer map info: error wpos=%d map=%d\n", vb_dbgdat.error_pos, vb_dbgdat.error_map);
	for (i = 0; i < VIDEO_INFO_TRACE_LEN; i++) {
		seq_printf(s, "[%02d] token=%08x %08x size %08x map %d\n",
			 i, vb_dbgdat.data[i].token, vb_dbgdat.data[i].iommu_addr, vb_dbgdat.data[i].length,
			 vb_dbgdat.data[i].map);
	}

	spin_unlock_irqrestore(&vb_dbgdat.lock, flags);
}

#ifdef DEC_IOMMU_EXCEPTION_DEBUG
static void vb_debug_record_exception(void)
{
	int i;
	unsigned long flags;

	if (vb_dbgdat.exception) {
		pr_err("video buf exception had already record !\n");
		return;
	}

	spin_lock_irqsave(&vb_dbgdat.lock, flags);
	i = (vb_dbgdat.wpos == 0) ? (VIDEO_INFO_TRACE_LEN - 1) : (vb_dbgdat.wpos - 1);
	i = i % VIDEO_INFO_TRACE_LEN;
	vb_dbgdat.error_pos = i;
	vb_dbgdat.error_map = vb_dbgdat.data[i].map;
	vb_dbgdat.exception = 1;
	spin_unlock_irqrestore(&vb_dbgdat.lock, flags);

	schedule_work(&vb_dbgdat.work);
}
#endif

static void vb_debug_work_func(struct work_struct *work)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&vb_dbgdat.lock, flags);

	pr_err("video buffer map info: wpos=%d\n", vb_dbgdat.wpos % VIDEO_INFO_TRACE_LEN);
	pr_err("video buffer map info: error wpos=%d map=%d\n", vb_dbgdat.error_pos, vb_dbgdat.error_map);
	for (i = 0; i < VIDEO_INFO_TRACE_LEN; i++) {
		pr_err("[%02d] token=%08x %08x size %08x map %d\n",
				i, vb_dbgdat.data[i].token, vb_dbgdat.data[i].iommu_addr, vb_dbgdat.data[i].length,
				vb_dbgdat.data[i].map);
	}

	spin_unlock_irqrestore(&vb_dbgdat.lock, flags);
}

static void vb_debug_init(void)
{
	memset(&vb_dbgdat, 0, sizeof(vb_dbgdat));

	INIT_WORK(&vb_dbgdat.work, vb_debug_work_func);
	spin_lock_init(&vb_dbgdat.lock);

#ifdef DEC_IOMMU_EXCEPTION_DEBUG
	sunxi_iommu_register_fault_cb(vb_debug_record_exception, 3);
	sunxi_iommu_register_fault_cb(vb_debug_record_exception, 4);
#endif
}

static void vb_debug_trace_dma_buf_map(int token, uint32_t addr, uint32_t size)
{
	int index = 0;
	unsigned long flags;

	if (vb_dbgdat.exception)
		return;

	spin_lock_irqsave(&vb_dbgdat.lock, flags);
	index = vb_dbgdat.wpos % VIDEO_INFO_TRACE_LEN;
	vb_dbgdat.data[index].iommu_addr = addr;
	vb_dbgdat.data[index].length = size;
	vb_dbgdat.data[index].token = token;
	vb_dbgdat.data[index].map = 1;

	vb_dbgdat.wpos++;
	spin_unlock_irqrestore(&vb_dbgdat.lock, flags);
}

static void vb_debug_trace_dma_buf_unmap(int token, uint32_t addr)
{
	int i;
	unsigned long flags;
	video_buf_info_t *record;

	if (vb_dbgdat.exception)
		return;

	spin_lock_irqsave(&vb_dbgdat.lock, flags);
	for (i = 0; i < VIDEO_INFO_TRACE_LEN; i++) {
		record = &vb_dbgdat.data[i];
		if (record->token == token && record->iommu_addr == addr) {
			record->map = 0;
			break;
		}
	}
	spin_unlock_irqrestore(&vb_dbgdat.lock, flags);
}


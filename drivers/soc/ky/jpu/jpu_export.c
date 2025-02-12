// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/genalloc.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/iommu.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/dma-buf.h>
#include <linux/highmem.h>
#include <linux/types.h>
#include <linux/types.h>
#include "jpu_export.h"
struct jpu_alloc_dma_buf {
	__s32 fd;		/* fd */
	__u32 flags;		/* flags to map with */
	__u64 size;		/* size */
};
struct jpu_data {
	bool is_continue;
	int npages;
	unsigned int size;
	struct page *pages[];
};

#define jpu_IOCTL_ALLOC_DMA_BUF \
	_IOWR('V', 10, struct jpu_alloc_dma_buf)

static struct device *pdev;
static int jpu_exporter_attach(struct dma_buf *dmabuf, struct dma_buf_attachment *attachment)
{
	return 0;
}

static void jpu_exporter_detach(struct dma_buf *dmabuf, struct dma_buf_attachment *attachment)
{
}

static struct sg_table *jpu_exporter_map_dma_buf(struct dma_buf_attachment *attachment,
						 enum dma_data_direction dir)
{
	struct jpu_data *data;
	struct sg_table *table;
	struct scatterlist *sg;
	int i;

	data = attachment->dmabuf->priv;
	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return ERR_PTR(-ENOMEM);

	if (sg_alloc_table(table, data->npages, GFP_KERNEL)) {
		kfree(table);
		return ERR_PTR(-ENOMEM);
	}
	sg = table->sgl;
	for (i = 0; i < data->npages; i++) {
		sg_set_page(sg, data->pages[i], data->size, 0);
		sg = sg_next(sg);
	}

	if (!dma_map_sg(pdev, table->sgl, table->nents, dir)) {
		sg_free_table(table);
		kfree(table);
		return ERR_PTR(-ENOMEM);
	}

	return table;
}

static void jpu_exporter_unmap_dma_buf(struct dma_buf_attachment *attachment,
				       struct sg_table *table, enum dma_data_direction dir)
{
	dma_unmap_sg(pdev, table->sgl, table->nents, dir);
	sg_free_table(table);
	kfree(table);
}

static void jpu_exporter_release(struct dma_buf *dma_buf)
{
	struct jpu_data *data = dma_buf->priv;
	int i;

	pr_info("dmabuf release data:%px\n", data);

	for (i = 0; i < data->npages; i++)
		put_page(data->pages[i]);

	kfree(data);
}

static int jpu_exporter_mmap(struct dma_buf *dma_buf, struct vm_area_struct *vma)
{
	struct jpu_data *data = dma_buf->priv;
	unsigned long vm_start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	int i = 0, ret;
	//pr_info("dma mmap vm start:0x%llx,size:0x%llx\n", vm_start, size);
	if (data->is_continue) {
		ret = remap_pfn_range(vma, vm_start, page_to_pfn(data->pages[i]),
				      size, vma->vm_page_prot);
	} else {
		for (i = 0; i < data->npages; i++) {
			remap_pfn_range(vma, vm_start, page_to_pfn(data->pages[i]),
					PAGE_SIZE, vma->vm_page_prot);
			vm_start += PAGE_SIZE;
		}
	}
	return 0;
}

static int jpu_exporter_begin_cpu_access(struct dma_buf *dmabuf, enum dma_data_direction dir)
{
	struct dma_buf_attachment *attachment;
	struct sg_table *table;

	if (list_empty(&dmabuf->attachments))
		return 0;

	attachment = list_first_entry(&dmabuf->attachments, struct dma_buf_attachment, node);
	table = attachment->priv;
	dma_sync_sg_for_cpu(NULL, table->sgl, table->nents, dir);

	return 0;
}

static int jpu_exporter_end_cpu_access(struct dma_buf *dmabuf, enum dma_data_direction dir)
{
	struct dma_buf_attachment *attachment;
	struct sg_table *table;

	if (list_empty(&dmabuf->attachments))
		return 0;

	attachment = list_first_entry(&dmabuf->attachments, struct dma_buf_attachment, node);
	table = attachment->priv;
	dma_sync_sg_for_device(NULL, table->sgl, table->nents, dir);

	return 0;
}

static const struct dma_buf_ops jpu_dmabuf_ops = {
	.attach = jpu_exporter_attach,
	.detach = jpu_exporter_detach,
	.map_dma_buf = jpu_exporter_map_dma_buf,
	.unmap_dma_buf = jpu_exporter_unmap_dma_buf,
	.release = jpu_exporter_release,
	.mmap = jpu_exporter_mmap,
	.begin_cpu_access = jpu_exporter_begin_cpu_access,
	.end_cpu_access = jpu_exporter_end_cpu_access,
};

//#define ALLOC_PAGES
#ifndef ALLOC_PAGES
static struct dma_buf *jpu_exp_alloc(struct jpu_alloc_dma_buf *alloc_data)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	struct dma_buf *dmabuf;
	struct jpu_data *data;

	int i, npages, size;

	size = alloc_data->size;
	npages = PAGE_ALIGN(size) / PAGE_SIZE;
	if (!npages)
		return ERR_PTR(-EINVAL);

	data = kmalloc(sizeof(*data) + npages * sizeof(struct page *), GFP_KERNEL);
	if (!data)
		return ERR_PTR(-ENOMEM);

	data->is_continue = 0;
	for (i = 0; i < npages; i++) {
		data->pages[i] = alloc_page(GFP_KERNEL);
		if (!data->pages[i])
			goto err;
	}
	data->npages = npages;
	data->size = PAGE_SIZE;
	pr_info("dmabuf alloc data:%px, npages:%d, size:0x%x\n", data, npages, size);

	exp_info.ops = &jpu_dmabuf_ops;
	exp_info.size = npages * PAGE_SIZE;
	exp_info.flags = O_CLOEXEC | O_RDWR;
	exp_info.priv = data;

	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf))
		goto err;

	return dmabuf;

err:
	while (i--)
		put_page(data->pages[i]);
	kfree(data);
	return ERR_PTR(-ENOMEM);
}
#else
static struct dma_buf *jpu_exp_alloc(struct jpu_alloc_dma_buf *alloc_data)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	struct dma_buf *dmabuf;
	struct jpu_data *data;
	int npages, size, order;

	size = alloc_data->size;
	npages = PAGE_ALIGN(size) / PAGE_SIZE;
	order = get_order(size);
	if (!npages)
		return ERR_PTR(-EINVAL);

	npages = 1 << order;
	data = kmalloc(sizeof(*data) + sizeof(struct page *), GFP_KERNEL);
	if (!data)
		return ERR_PTR(-ENOMEM);
	data->is_continue = 1;
	data->pages[0] = alloc_pages(GFP_KERNEL, order);
	data->npages = 1;
	data->size = npages * PAGE_SIZE;
	pr_info("dmabuf alloc data:%px, real num:%d, order:%d, size:0x%x\n", data,
		npages, order, size);

	exp_info.ops = &jpu_dmabuf_ops;
	exp_info.size = npages * PAGE_SIZE;
	exp_info.flags = O_CLOEXEC | O_RDWR;
	exp_info.priv = data;

	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf))
		goto err;

	return dmabuf;

err:
	put_page(data->pages[0]);
	kfree(data);
	return ERR_PTR(-ENOMEM);
}
#endif

static long jpu_exp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct dma_buf *dmabuf = NULL;
	unsigned int fd;
	struct jpu_alloc_dma_buf alloc_data;
	switch (cmd) {
	case jpu_IOCTL_ALLOC_DMA_BUF:
		if (copy_from_user(&alloc_data, (void __user *)arg, sizeof(alloc_data)))
			return -EFAULT;

		dmabuf = jpu_exp_alloc(&alloc_data);
		if (!dmabuf) {
			pr_err("error: exporter alloc page failed\n");
			return -ENOMEM;
		}
		fd = dma_buf_fd(dmabuf, O_CLOEXEC);
		pr_info("dmabuf fd:%d\n", fd);
		alloc_data.fd = fd;
		if (copy_to_user((void __user *)arg, &alloc_data, sizeof(alloc_data)))
			return -EFAULT;
		break;
	default:
		break;
	}
	return 0;
}

static struct file_operations jpu_exp_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = jpu_exp_ioctl,
};

static struct miscdevice jpu_exp = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "jpu_exp",
	.fops = &jpu_exp_fops,
};

static u64 jpu_exp_dmamask = 0xffffffffffUL;

int jpu_exp_init(void)
{
	int ret;
	ret = misc_register(&jpu_exp);
	pdev = jpu_exp.this_device;
	pdev->dma_mask = &jpu_exp_dmamask;
	pdev->coherent_dma_mask = 0xffffffffffull;
	return ret;
}

void jpu_exp_exit(void)
{
	misc_deregister(&jpu_exp);
}

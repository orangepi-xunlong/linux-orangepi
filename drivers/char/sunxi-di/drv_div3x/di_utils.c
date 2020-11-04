/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "di_utils.h"
#include "di_debug.h"

#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <drm/drm_fourcc.h>

#define TAG "[DI_UTILS]"

static struct device *dma_dev;

void di_utils_set_dma_dev(struct device *dev)
{
	dma_dev = dev;
}


static struct info_mem di_mem[MAX_DI_MEM_INDEX];
void *di_malloc(__u32 bytes_num, uintptr_t *phy_addr)
{
	void *address = NULL;

#if defined(CONFIG_ION_SUNXI)
	u32 actual_bytes;

	if (bytes_num != 0) {
		actual_bytes = PAGE_ALIGN(bytes_num);

		address = dma_alloc_coherent(dma_dev, actual_bytes,
					     (dma_addr_t *) phy_addr,
					     GFP_KERNEL);
		if (address) {
			DI_INFO("dma_alloc_coherent ok, address=0x%p, size=0x%x\n",
			    (void *)(*(unsigned long *)phy_addr), bytes_num);
			return address;
		}

		DI_ERR("dma_alloc_coherent fail, size=0x%x\n", bytes_num);
		return NULL;
	}
	DI_ERR("%s size is zero\n", __func__);
#else
	unsigned map_size = 0;
	struct page *page;

	if (bytes_num != 0) {
		map_size = PAGE_ALIGN(bytes_num);
		page = alloc_pages(GFP_KERNEL, get_order(map_size));
		if (page != NULL) {
			address = page_address(page);
			if (address == NULL) {
				free_pages((unsigned long)(page),
					   get_order(map_size));
				DI_ERR("page_address fail!\n");
				return NULL;
			}
			*phy_addr = virt_to_phys(address);
			return address;
		}
		DI_ERR("alloc_pages fail!\n");
		return NULL;
	}
	DI_ERR("%s size is zero\n", __func__);
#endif

	return NULL;
}

void di_free(void *virt_addr, void *phy_addr, unsigned int size)
{
#if defined(CONFIG_ION_SUNXI)
	u32 actual_bytes;

	actual_bytes = PAGE_ALIGN(size);
	if (phy_addr && virt_addr)
		dma_free_coherent(dma_dev, actual_bytes, virt_addr,
				  (dma_addr_t) phy_addr);
#else
	unsigned map_size = PAGE_ALIGN(size);
	unsigned page_size = map_size;

	if (virt_addr == NULL)
		return;

	free_pages((unsigned long)virt_addr, get_order(page_size));
#endif
}

__s32 di_get_free_mem_index(void)
{
	__u32 i = 0;

	for (i = 0; i < MAX_DI_MEM_INDEX; i++) {
		if (di_mem[i].b_used == 0)
			return i;
	}
	return -1;
}

int di_mem_request(__u32 size, u64 *phyaddr)
{
	__s32 sel;
	unsigned long ret = 0;
	uintptr_t phy_addr;

	sel = di_get_free_mem_index();
	if (sel < 0) {
		DI_ERR("di_get_free_mem_index fail!\n");
		return -EINVAL;
	}

	ret = (unsigned long)di_malloc(size, &phy_addr);
	if (ret != 0) {
		di_mem[sel].virt_addr = (void *)ret;
		memset(di_mem[sel].virt_addr, 0, size);
		di_mem[sel].phy_addr = phy_addr;
		di_mem[sel].mem_len = size;
		di_mem[sel].b_used = 1;
		*phyaddr = phy_addr;

		DI_INFO("map_di_memory[%d]: pa=%08lx va=%p size:%x\n", sel,
		     di_mem[sel].phy_addr, di_mem[sel].virt_addr, size);
		return sel;
	}
	DI_ERR("fail to alloc reserved memory!\n");
	return -ENOMEM;
}

int di_mem_release(__u32 sel)
{
	if (di_mem[sel].b_used == 0) {
		DI_ERR("mem not used in di_mem_release,%d\n", sel);
		return -EINVAL;
	}

	di_free((void *)di_mem[sel].virt_addr, (void *)di_mem[sel].phy_addr,
		 di_mem[sel].mem_len);
	memset(&di_mem[sel], 0, sizeof(struct info_mem));

	return 0;
}

static struct di_dma_item *di_dma_item_create(
	struct dma_buf *dmabuf,
	struct dma_buf_attachment *attach,
	struct sg_table *sgt,
	enum dma_data_direction dir)
{
	struct di_dma_item *item;
	struct sg_table *sgt_bak;
	struct scatterlist *sgl, *sgl_bak;
	s32 sg_count = 0;
	int i;

	/*DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);*/

	item = kzalloc(sizeof(*item), GFP_KERNEL);
	if (item == NULL) {
		DI_ERR(TAG"alloc mem for dma_item fail\n");
		return NULL;
	}

	sgt_bak = kzalloc(sizeof(*sgt_bak), GFP_KERNEL);
	if (sgt_bak == NULL) {
		DI_ERR(TAG"alloc mem for sgt_bak fail\n");
		goto err_item_kfree;
	}
	if (sg_alloc_table(sgt_bak, sgt->nents, GFP_KERNEL)) {
		DI_ERR(TAG"alloc sgt fail\n");
		goto err_sgt_kfree;
	}

	sgl_bak = sgt_bak->sgl;
	for_each_sg(sgt->sgl, sgl, sgt->nents, i)  {
		sg_set_page(sgl_bak, sg_page(sgl), sgl->length, sgl->offset);
		sgl_bak = sg_next(sgl_bak);
	}
	sg_count = dma_map_sg_attrs(dma_dev,
		sgt_bak->sgl, sgt_bak->nents, dir, DMA_ATTR_SKIP_CPU_SYNC);
	if (sg_count != 1) {
		DI_ERR(TAG"dma_map_sg_attrs failed:%d\n", sg_count);
		goto err_free_sgt;
	}

	item->buf = dmabuf;
	item->attach = attach;
	item->sgt_org = sgt;
	item->sgt_bak = sgt_bak;
	item->dir = dir;
	item->dma_addr = sg_dma_address(sgt_bak->sgl);

	return item;

err_free_sgt:
	sg_free_table(sgt_bak);

err_sgt_kfree:
	kfree(sgt_bak);

err_item_kfree:
	kfree(item);

	return NULL;
}

struct di_dma_item *di_dma_buf_self_map(
	int fd, enum dma_data_direction dir)
{
	struct di_dma_item *dma_item = NULL;

	struct dma_buf *dmabuf = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct sg_table *sgt = NULL;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		DI_ERR(TAG"dma buf get fail. fd=%d\n", fd);
		return NULL;
	}

	attach = dma_buf_attach(dmabuf, dma_dev);
	if (IS_ERR(attach)) {
		DI_ERR(TAG"dma buf attach fail\n");
		goto out_buf_put;
	}

	sgt = dma_buf_map_attachment(attach, dir);
	if (IS_ERR_OR_NULL(sgt)) {
		DI_ERR(TAG"dma_buf_map_attachment fail\n");
		goto out_buf_detach;
	}

	dma_item = di_dma_item_create(dmabuf, attach, sgt, dir);
	if (dma_item != NULL)
		return dma_item;

/* out_buf_unmap: */
	dma_buf_unmap_attachment(attach, sgt, dir);

out_buf_detach:
	dma_buf_detach(dmabuf, attach);

out_buf_put:
	dma_buf_put(dmabuf);

	return dma_item;
}

void di_dma_buf_self_unmap(struct di_dma_item *item)
{
	/*DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);*/

	dma_unmap_sg_attrs(dma_dev, item->sgt_bak->sgl,
		item->sgt_bak->nents, item->dir, DMA_ATTR_SKIP_CPU_SYNC);
	dma_buf_unmap_attachment(item->attach,
		item->sgt_bak, item->dir);
	sg_free_table(item->sgt_bak);
	kfree(item->sgt_bak);
	dma_buf_detach(item->buf, item->attach);
	dma_buf_put(item->buf);

	kfree(item);
}

struct di_mapped_buf *di_dma_buf_alloc_map(u32 size)
{
	struct di_mapped_buf *mbuf =
		kzalloc(sizeof(*mbuf), GFP_KERNEL);

	if (mbuf == NULL) {
		DI_ERR(TAG"kzalloc for mapped buf fail, size=%d\n",
			(u32)sizeof(*mbuf));
		return NULL;
	}

	mbuf->size_request = size;
	mbuf->size_alloced = PAGE_ALIGN(size);
	mbuf->vir_addr = dma_alloc_coherent(dma_dev,
		mbuf->size_alloced, &mbuf->dma_addr, GFP_KERNEL);
	if (mbuf->vir_addr == NULL) {
		DI_ERR(TAG"dma_alloc_coherent fail, size=0x%x->0x%x\n",
			mbuf->size_request, mbuf->size_alloced);
		kfree(mbuf);
		return NULL;
	}

	DI_DEBUG(TAG"%s: addr[%p,%p], size=0x%x\n", __func__,
		mbuf->vir_addr, (void *)mbuf->dma_addr, mbuf->size_alloced);

	return mbuf;
}

void di_dma_buf_unmap_free(struct di_mapped_buf *mbuf)
{
	DI_DEBUG(TAG"%s: addr[%p,%p], size=0x%x\n", __func__,
		mbuf->vir_addr, (void *)mbuf->dma_addr, mbuf->size_alloced);
	dma_free_coherent(dma_dev, mbuf->size_alloced,
		mbuf->vir_addr, mbuf->dma_addr);
	kfree(mbuf);
}

const char *di_format_to_string(u32 format)
{
	switch (format) {
	case DRM_FORMAT_YUV420:
		return "YU12";
	case DRM_FORMAT_YVU420:
		return "YV12";
	case DRM_FORMAT_YUV422:
		return "YU16";
	case DRM_FORMAT_YVU422:
		return "YV16";
	case DRM_FORMAT_NV12:
		return "NV12";
	case DRM_FORMAT_NV21:
		return "NV21";
	case DRM_FORMAT_NV16:
		return "NV16";
	case DRM_FORMAT_NV61:
		return "NV61";
	default:
		DI_INFO(TAG"%s format(0x%x)\n", __func__, format);
		return "unknown format";
	}
}

u32 di_format_get_planar_num(u32 format)
{
	switch (format) {
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YUV422:
	case DRM_FORMAT_YVU422:
		return 3;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
		return 2;
	default:
		DI_INFO(TAG"%s:%s\n", __func__,
			di_format_to_string(format));
		return 1;
	}
}

u32 di_format_get_uvseq(u32 format)
{
	switch (format) {
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV61:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YVU422:
		/* TODO: add more format to support */
		return DI_UVSEQ_VU;
	default:
		return DI_UVSEQ_UV;
	}
}

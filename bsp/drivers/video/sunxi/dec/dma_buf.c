/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "dma_buf.h"
#include "dec_trace.h"
#include "linux/err.h"

static int dec_dma_map_core(struct dma_buf *dmabuf, struct dec_dmabuf_t *item)
{
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int ret = -1;

	attachment = dma_buf_attach(dmabuf, item->p_device);
	if (IS_ERR_OR_NULL(attachment)) {
		dev_err(item->p_device, "dma_buf_attach failed\n");
		goto exit;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_TO_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		dev_err(item->p_device, "dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	item->dmabuf = dmabuf;
	item->sgt = sgt;
	item->attachment = attachment;
	item->dma_addr = sg_dma_address(sgt->sgl);
	ret = 0;

	DEC_TRACE_ADDRESS("+map", item->dma_addr);
	goto exit;

	/* unmap attachment sgt, not sgt_bak, cause it's not alloc yet! */
	dma_buf_unmap_attachment(attachment, sgt, DMA_TO_DEVICE);
err_buf_detach:
	dma_buf_detach(dmabuf, attachment);
exit:
	return ret;
}

static void dec_dma_unmap_core(struct dec_dmabuf_t *item)
{
	DEC_TRACE_ADDRESS("-umap", item->dma_addr);

	dma_buf_unmap_attachment(item->attachment, item->sgt, DMA_TO_DEVICE);
	dma_buf_detach(item->dmabuf, item->attachment);
	dma_buf_put(item->dmabuf);
}

struct dec_dmabuf_t *dec_dma_map(int fd, struct device *p_device)
{
	struct dec_dmabuf_t *item = NULL;
	struct dma_buf *dmabuf = NULL;

	if (!p_device)
		goto exit;

	if (fd < 0) {
		dev_err(p_device, "dma_buf_id(%d) is invalid\n", fd);
		goto exit;
	}

	DEC_TRACE_BEGIN("dma_map");
	item = kmalloc(sizeof(struct dec_dmabuf_t),
				  GFP_KERNEL | __GFP_ZERO);

	if (!item) {
		dev_err(p_device, "malloc memory of size %d fail!\n",
			   (unsigned int)sizeof(struct dec_dmabuf_t));
		DEC_TRACE_END("");
		goto exit;
	}

	item->p_device = p_device;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		dev_err(item->p_device, "dma_buf_get failed, fd=%d %ld\n", fd, (long)dmabuf);
		kfree(item);
		goto exit;
	}

	if (dec_dma_map_core(dmabuf, item) != 0) {
		dma_buf_put(dmabuf);
		kfree(item);
		item = NULL;
	}
	DEC_TRACE_END("");

exit:
	return item;
}

struct dec_dmabuf_t *dec_dma_map_dmabuf(struct dma_buf *dmabuf, struct device *p_device)
{
	struct dec_dmabuf_t *item = NULL;

	if (!p_device)
		goto exit;

	if (IS_ERR_OR_NULL(dmabuf)) {
		dev_err(p_device, "dma_buf(%p) is invalid\n", dmabuf);
		goto exit;
	}

	DEC_TRACE_BEGIN("dma_map");
	item = kmalloc(sizeof(struct dec_dmabuf_t),
				  GFP_KERNEL | __GFP_ZERO);

	if (!item) {
		dev_err(p_device, "malloc memory of size %d fail!\n",
			   (unsigned int)sizeof(struct dec_dmabuf_t));
		DEC_TRACE_END("");
		goto exit;
	}

	item->p_device = p_device;
	if (dec_dma_map_core(dmabuf, item) != 0) {
		kfree(item);
		item = NULL;
	}
	DEC_TRACE_END("");

exit:
	return item;
}

void dec_dma_unmap(struct dec_dmabuf_t *item)
{
	DEC_TRACE_BEGIN("dma_unmap");
	dec_dma_unmap_core(item);
	kfree(item);
	DEC_TRACE_END("");
}

unsigned int dec_dma_buf_size(struct dec_dmabuf_t *item)
{
	struct scatterlist *sgl;
	struct sg_table *sgt = item->sgt;
	unsigned int size = 0;
	unsigned int i = 0;

	for_each_sg(sgt->sgl, sgl, sgt->nents, i) {
		size += sg_dma_len(sgl);
	}

	return size;
}


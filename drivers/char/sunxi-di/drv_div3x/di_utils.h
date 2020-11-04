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

#ifndef _DI_UTILS_H_
#define _DI_UTILS_H_

#include <linux/types.h>
#include <linux/dma-buf.h>

#define MAX_DI_MEM_INDEX       100

struct info_mem {
	unsigned long phy_addr;
	void *virt_addr;
	__u32 b_used;
	__u32 mem_len;
};

struct di_dma_item {
	struct dma_buf *buf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt_org;
	struct sg_table *sgt_bak;
	enum dma_data_direction dir;
	dma_addr_t dma_addr;
};

struct di_mapped_buf {
	u32 size_request;
	u32 size_alloced;
	void *vir_addr;
	dma_addr_t dma_addr;
};

void di_utils_set_dma_dev(struct device *dev);

int di_mem_release(__u32 sel);
int di_mem_request(__u32 size, u64 *phyaddr);

struct di_dma_item *di_dma_buf_self_map(
	int fd, enum dma_data_direction dir);
void di_dma_buf_self_unmap(struct di_dma_item *item);

struct di_mapped_buf *di_dma_buf_alloc_map(u32 size);
void di_dma_buf_unmap_free(struct di_mapped_buf *mbuf);


enum {
	DI_UVSEQ_UV = 0, /* uv-combined: LSB U0V0U1V1 MSB */
	DI_UVSEQ_VU = 1, /* uv-combined: LSB V0U0V1U1 MSB */
};

const char *di_format_to_string(u32 format);
u32 di_format_get_planar_num(u32 format);
u32 di_format_get_uvseq(u32 format);

#endif /* #ifndef _DI_UTILS_H_ */

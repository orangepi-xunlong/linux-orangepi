/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dma_buf.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
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
#ifndef _DMA_BUF_H
#define _DMA_BUF_H

#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/stddef.h>
#include <linux/kfifo.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#ifdef __cplusplus
extern "C" {
#endif


struct dec_dmabuf_t {
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	struct device *p_device;
};

struct dec_dmabuf_t *dec_dma_map(int fd, struct device *p_device);
struct dec_dmabuf_t *dec_dma_map_dmabuf(struct dma_buf *dmabuf, struct device *p_device);
void dec_dma_unmap(struct dec_dmabuf_t *item);
unsigned int dec_dma_buf_size(struct dec_dmabuf_t *item);

#ifdef __cplusplus
}
#endif

#endif /*End of file*/

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

#ifndef __VIN__OS__H__
#define __VIN__OS__H__

#include <linux/device.h>
#include <linux/clk.h>
/* #include <sunxi-clk.h> */
#include <linux/interrupt.h>
#include "../platform/platform_cfg.h"
#include <linux/dma-mapping.h>

#define ION_HEAP_SYSTEM_MASK		(1 << 0)
#define ION_HEAP_TYPE_DMA_MASK		(1 << 1)

#define IS_FLAG(x, y) (((x)&(y)) == y)

struct vin_mm {
	size_t size;
	void *phy_addr;
	void *vir_addr;
	void *dma_addr;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	/* struct ion_heap *heap; */
	struct dma_heap *dmaHeap;
};

extern unsigned int vin_set_large_overlayer(unsigned int width);
extern int os_gpio_set(struct gpio_config *gpio_list);
extern int os_gpio_write(u32 gpio, __u32 out_value, int force_value_flag);
extern int os_mem_alloc(struct device *dev, struct vin_mm *mem_man);
extern void os_mem_free(struct device *dev, struct vin_mm *mem_man);
extern void vin_iommu_en(unsigned int mester_id, bool en);

#endif	/* __VIN__OS__H__ */

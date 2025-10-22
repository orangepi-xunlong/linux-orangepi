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
#include "vin_log.h"
#include <linux/module.h>
#include "vin_os.h"
#include <linux/dma-heap.h>
#include <linux/dma-buf.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#include <linux/iosys-map.h>
#endif

unsigned int vin_log_mask = 0xffff - VIN_LOG_ISP - VIN_LOG_STAT - VIN_LOG_VIDEO - VIN_LOG_TDM;
EXPORT_SYMBOL_GPL(vin_log_mask);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
MODULE_IMPORT_NS(DMA_BUF);
#endif

unsigned int vin_set_large_overlayer(unsigned int width)
{
	unsigned int max_overlayer = 0,  min_overlayer = 0;
	unsigned int overlayer = 0;
	unsigned align = 16;
	unsigned int max_width = 3264;

	min_overlayer = ALIGN(width / 61, align) * 3;
	if (min_overlayer / 2 + width / 2 > max_width) {
		vin_err("min overlayer/2 + width/2 is more than max_width, %d + %d > %d\n", min_overlayer / 2, width / 2, max_width);
		return overlayer;
	}

	max_overlayer = ALIGN(width / 57, align) * 7;
	overlayer = max_overlayer;
	while (overlayer / 2 + width / 2 > max_width)
		overlayer -= align / 2;
	vin_log(VIN_LOG_LARGE, "max_overlayer/2 is %d, min_overlayer/2 is %d, overlayer/2 is %d\n", max_overlayer / 2, min_overlayer / 2, overlayer / 2);

	if (overlayer == 0)
		overlayer = 256;

	return overlayer / 2;
}
EXPORT_SYMBOL_GPL(vin_set_large_overlayer);

void mem_dump_regs(void *addr, unsigned long size)
{
	int cnt = 0;

	do {
		if (cnt % 4 == 0)
			printk(KERN_CONT "0x%016x:", cnt * 4);
		printk(KERN_CONT " 0x%08x ", *((unsigned int *)(addr + cnt * 4)));
		cnt++;
		if (cnt % 4 == 0 && cnt != 0)
			printk(KERN_CONT "\n");
	} while (size > cnt * 4);
}
EXPORT_SYMBOL_GPL(mem_dump_regs);

void ahb_dump_regs(volatile void __iomem *addr, unsigned long size)
{
	unsigned int val;
	int cnt = 0;

	do {
		if (cnt % 4 == 0)
			printk(KERN_CONT "0x%016x:", cnt * 4);
		val = readl(addr + cnt * 4);
		printk(KERN_CONT " 0x%08x ", val);
		cnt++;
		if (cnt % 4 == 0 && cnt != 0)
			printk(KERN_CONT "\n");
	} while (size > cnt * 4);
}
EXPORT_SYMBOL_GPL(ahb_dump_regs);

int os_gpio_write(u32 gpio, __u32 out_value, int force_value_flag)
{
#ifndef FPGA_VER
	if (gpio == GPIO_INDEX_INVALID)
		return 0;

	if (force_value_flag == 1) {
		gpio_direction_output(gpio, out_value);
		__gpio_set_value(gpio, out_value);
	} else {
		if (out_value == 0) {
			gpio_direction_output(gpio, out_value);
			__gpio_set_value(gpio, out_value);
		} else {
			gpio_direction_input(gpio);
		}
	}
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(os_gpio_write);

int vin_get_ion_phys(struct device *dev, struct vin_mm *mem_man)
{
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int ret = -1;

	if (IS_ERR(mem_man->buf)) {
		pr_err("dma_buf is null\n");
		return ret;
	}

	attachment = dma_buf_attach(mem_man->buf, get_device(dev));
	if (IS_ERR(attachment)) {
		pr_err("dma_buf_attach failed\n");
		goto err_buf_attach;
	}

	sgt = dma_buf_map_attachment(attachment, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		pr_err("dma_buf_map_attachment failed\n");
		goto err_buf_map_attachment;
	}

	mem_man->phy_addr = (void *)sg_dma_address(sgt->sgl);
	mem_man->sgt = sgt;
	mem_man->attachment = attachment;
	ret = 0;
	goto exit;

err_buf_map_attachment:
	dma_buf_unmap_attachment(attachment, sgt, DMA_FROM_DEVICE);
err_buf_attach:
	dma_buf_detach(mem_man->buf, attachment);
exit:
	return ret;

}

void vin_free_ion_phys(struct device *dev, struct vin_mm *mem_man)
{
	dma_buf_unmap_attachment(mem_man->attachment, mem_man->sgt, DMA_FROM_DEVICE);
	dma_buf_detach(mem_man->buf, mem_man->attachment);
}

int os_mem_alloc(struct device *dev, struct vin_mm *mem_man)
{
	int ret = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	__maybe_unused struct iosys_map map;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	__maybe_unused struct dma_buf_map map;
#endif

	if (mem_man == NULL)
		return -1;

#ifdef SUNXI_MEM
#if IS_ENABLED(CONFIG_AW_IOMMU) && IS_ENABLED(CONFIG_VIN_IOMMU)
	/* DMA BUFFER HEAP (after linux 5.10) */
	mem_man->dmaHeap = dma_heap_find("system-uncached");
	if (!mem_man->dmaHeap) {
		vin_err("dma_heap_find failed\n");
		goto err_alloc;
	}
	mem_man->buf = dma_heap_buffer_alloc(mem_man->dmaHeap, mem_man->size, O_RDWR, 0);
	if (IS_ERR(mem_man->buf)) {
		vin_err("dma_heap_buffer_alloc failed\n");
		goto err_alloc;
	}
#else
	/* CMA or CARVEOUT */
	mem_man->dmaHeap = dma_heap_find("reserved");
	if (!mem_man->dmaHeap) {
		vin_err("dma_heap_find failed\n");
		goto err_alloc;
	}
	mem_man->buf = dma_heap_buffer_alloc(mem_man->dmaHeap, mem_man->size, O_RDWR, 0);
	if (IS_ERR(mem_man->buf)) {
		vin_err("dma_heap_buffer alloc failed\n");
		goto err_alloc;
	}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = dma_buf_vmap(mem_man->buf, &map);
	if (ret) {
		vin_err("dma_buf_vmap failed!!");
		goto err_map_kernel;
	}
	mem_man->vir_addr = map.vaddr;
#else /* before linux-5.15 */
	mem_man->vir_addr = dma_buf_vmap(mem_man->buf);
	if (IS_ERR_OR_NULL(mem_man->vir_addr)) {
		vin_err("ion_map_kernel failed!!");
		goto err_map_kernel;
	}
#endif

	/* IOMMU or CMA or CARVEOUT */
	ret = vin_get_ion_phys(dev, mem_man);
	if (ret) {
		vin_err("ion_phys failed!!");
		goto err_phys;
	}
	mem_man->dma_addr = mem_man->phy_addr;

	return ret;

err_phys:

	dma_buf_vunmap(mem_man->buf, mem_man->vir_addr);

err_map_kernel:

	dma_heap_buffer_free(mem_man->buf);

err_alloc:
	return ret;
#else
	mem_man->vir_addr = dma_alloc_coherent(dev, (size_t) mem_man->size,
					(dma_addr_t *)&mem_man->phy_addr,
					GFP_KERNEL);
	if (!mem_man->vir_addr) {
		vin_err("dma_alloc_coherent memory alloc failed\n");
		return -ENOMEM;
	}
	mem_man->dma_addr = mem_man->phy_addr;
	ret = 0;
	return ret;
#endif
}
EXPORT_SYMBOL_GPL(os_mem_alloc);


void os_mem_free(struct device *dev, struct vin_mm *mem_man)
{
#ifdef SUNXI_MEM
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(mem_man->vir_addr);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	struct dma_buf_map map = DMA_BUF_MAP_INIT_VADDR(mem_man->vir_addr);
#endif
#endif
	if (mem_man == NULL)
		return;

#ifdef SUNXI_MEM
	vin_free_ion_phys(dev, mem_man);
	/* ion_heap_unmap_kernel(mem_man->heap, mem_man->buf->priv); */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	if (mem_man->vir_addr)
		dma_buf_vunmap(mem_man->buf, &map);
#else
	if (mem_man->vir_addr)
		dma_buf_vunmap(mem_man->buf, mem_man->vir_addr);
#endif
	/* ion_free(mem_man->buf->priv); */
	dma_heap_buffer_free(mem_man->buf);
#else
	if (mem_man->vir_addr)
		dma_free_coherent(dev, mem_man->size, mem_man->vir_addr,
				  (dma_addr_t) mem_man->phy_addr);
#endif
	mem_man->phy_addr = NULL;
	mem_man->dma_addr = NULL;
	mem_man->vir_addr = NULL;
}
EXPORT_SYMBOL_GPL(os_mem_free);

extern void sunxi_enable_device_iommu(unsigned int master_id, bool flag);
void vin_iommu_en(unsigned int mester_id, bool en)
{
#if IS_ENABLED(CONFIG_AW_IOMMU) && IS_ENABLED(CONFIG_VIN_IOMMU)
	sunxi_enable_device_iommu(mester_id, en);
#endif
}
EXPORT_SYMBOL_GPL(vin_iommu_en);

MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video front end OSAL for sunxi");

/*
 * linux-5.4/drivers/media/platform/sunxi-vin/utility/vin_os.c
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
#include <linux/module.h>
#include <linux/ion.h>

#include "vin_os.h"

unsigned int vin_log_mask;
EXPORT_SYMBOL_GPL(vin_log_mask);

int os_gpio_write(u32 gpio, __u32 out_value, int force_value_flag)
{
#ifndef FPGA_VER

	if (force_value_flag == 1) {
		gpio_direction_output(gpio, 0);
		__gpio_set_value(gpio, out_value);
	} else {
		if (out_value == 0) {
			gpio_direction_output(gpio, 0);
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
		goto err_buf_put;
	}

	sgt = dma_buf_map_attachment(attachment, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		pr_err("dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	mem_man->phy_addr = (void *)sg_dma_address(sgt->sgl);
	mem_man->sgt = sgt;
	mem_man->attachment = attachment;
	ret = 0;
	goto exit;

err_buf_detach:
	dma_buf_detach(mem_man->buf, attachment);
err_buf_put:
	dma_buf_put(mem_man->buf);
exit:
	return ret;

}

void vin_free_ion_phys(struct device *dev, struct vin_mm *mem_man)
{
	dma_buf_unmap_attachment(mem_man->attachment, mem_man->sgt, DMA_FROM_DEVICE);
	dma_buf_detach(mem_man->buf, mem_man->attachment);
	dma_buf_put(mem_man->buf);

}
int os_mem_alloc(struct device *dev, struct vin_mm *mem_man)
{
	int ret = -1;
	__maybe_unused struct ion_buffer *ion_buf;

	if (mem_man == NULL)
		return -1;
	ion_buf = (struct ion_buffer *)mem_man->buf->priv;

#ifdef SUNXI_MEM
#if IS_ENABLED(CONFIG_SUNXI_IOMMU) && IS_ENABLED(CONFIG_VIN_IOMMU)
	/* IOMMU */
	mem_man->buf = ion_alloc(mem_man->size, ION_HEAP_SYSTEM_MASK, 0);
	if (IS_ERR_OR_NULL(mem_man->buf)) {
		vin_err("iommu alloc failed\n");
		goto err_alloc;
	}

#else
	/* CMA or CARVEOUT */
	mem_man->buf = ion_alloc(mem_man->size, ION_HEAP_TYPE_DMA_MASK, 0);
	if (IS_ERR_OR_NULL(mem_man->buf)) {
		vin_err("cma alloc failed\n");
		goto err_alloc;
	}

#endif

	mem_man->vir_addr = ion_heap_map_kernel(NULL, mem_man->buf->priv);
	if (IS_ERR_OR_NULL(mem_man->vir_addr)) {
		vin_err("ion_map_kernel failed!!");
		goto err_map_kernel;
	}

	/*IOMMU or CMA or CARVEOUT */
	ret = vin_get_ion_phys(dev, mem_man);
	if (ret) {
		vin_err("ion_phys failed!!");
		goto err_phys;
	}
	mem_man->dma_addr = mem_man->phy_addr;
	return ret;

err_phys:
	ion_heap_unmap_kernel(mem_man->heap, mem_man->buf->priv);
err_map_kernel:
	ion_free(mem_man->buf->priv);
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
	 __maybe_unused struct ion_buffer *ion_buf;

	 if (mem_man == NULL)
		 return;
	 ion_buf = (struct ion_buffer *)mem_man->buf->priv;

#ifdef SUNXI_MEM
	vin_free_ion_phys(dev, mem_man);
	ion_heap_unmap_kernel(mem_man->heap, mem_man->buf->priv);
	ion_free(mem_man->buf->priv);
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

MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video front end OSAL for sunxi");

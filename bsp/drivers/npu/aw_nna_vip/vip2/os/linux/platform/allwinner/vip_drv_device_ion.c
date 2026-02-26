/*
 *
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

#include "vip_drv_device_ion.h"



int aw_vip_mem_alloc(struct device *dev, vip_ion_mm_t *vip_mem, const char *name)
{
	int ret = -1;
	printk("enter aw vip mem alloc size %d %s\n", vip_mem->size, name);
	vip_mem->vir_addr = dma_alloc_attrs(dev, vip_mem->size, &vip_mem->dma_addr,
					GFP_KERNEL, DMA_ATTR_SKIP_CPU_SYNC);
	if (!vip_mem->vir_addr) {
		printk("dma_alloc_attrs fail!!\n");
	return -1;
    }

	vip_mem->phy_addr = vip_mem->dma_addr;

	printk("aw_vip_mem_alloc vir 0x%px, phy 0x%px\n", (unsigned long)vip_mem->vir_addr, (unsigned long)vip_mem->phy_addr);
	vip_mem->dma_buffer = NULL;

	return 0;
}




void aw_vip_mem_free(struct device *dev, vip_ion_mm_t *vip_mem)
{
	printk("aw_vip_mem_free vir 0x%px, phy 0x%px\n", (unsigned long)vip_mem->vir_addr, (unsigned long)vip_mem->phy_addr);
	dma_free_attrs(dev, vip_mem->size, vip_mem->vir_addr, vip_mem->dma_addr, DMA_ATTR_SKIP_CPU_SYNC);

	vip_mem->phy_addr = 0;
	vip_mem->dma_addr = 0;
	vip_mem->vir_addr = NULL;
}

/*void aw_vip_mem_flush(struct device *dev, vip_ion_mm_t *vip_mem)
{
		dma_sync_sg_for_device(get_device(dev), vip_mem->handle->buffer->sg_table->sgl, vip_mem->handle->buffer->sg_table->nents,
						DMA_BIDIRECTIONAL);
}

int aw_vip_mmap(vip_ion_mm_t *vip_mem, struct vm_area_struct *vma, unsigned long pgoff)
{
	vip_mem->dma_buffer = ion_share_dma_buf(vip_mem->client,
				  vip_mem->handle);
	return dma_buf_mmap(vip_mem->dma_buffer, vma, pgoff);
}*/

int aw_vip_munmap(vip_ion_mm_t *vip_mem)
{

	return 0;
}

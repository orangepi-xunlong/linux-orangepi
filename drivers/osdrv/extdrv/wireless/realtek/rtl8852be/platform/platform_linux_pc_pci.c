/******************************************************************************
 *
 * Copyright(c) 2019 -  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#include "drv_types.h"
#ifdef CONFIG_RTW_DEDICATED_CMA_POOL
#include <linux/platform_device.h>
extern struct platform_device *g_pldev;
#endif

#ifdef CONFIG_PLATFORM_AML_S905
extern struct device * g_pcie_reserved_mem_dev;
#endif


void pci_cache_wback(struct pci_dev *hwdev,
			dma_addr_t *bus_addr, size_t size, int direction)
{
	if (NULL != hwdev && NULL != bus_addr) {
#ifdef CONFIG_PLATFORM_AML_S905_V1
		if (g_pcie_reserved_mem_dev)
			hwdev->dev.dma_mask = NULL;
#endif
	  	dma_sync_single_for_device(&hwdev->dev, *bus_addr, size,
					direction);
	} else
		RTW_ERR("pcie hwdev handle or bus addr is NULL!\n");
}
void pci_cache_inv(struct pci_dev *hwdev,
			dma_addr_t *bus_addr, size_t size, int direction)
{
	if (NULL != hwdev && NULL != bus_addr) {
#ifdef CONFIG_PLATFORM_AML_S905_V1
		if (g_pcie_reserved_mem_dev)
			hwdev->dev.dma_mask = NULL;
#endif
		dma_sync_single_for_cpu(&hwdev->dev, *bus_addr, size, direction);
	} else
		RTW_ERR("pcie hwdev handle or bus addr is NULL!\n");
}
void pci_get_bus_addr(struct pci_dev *hwdev,
			void *vir_addr, dma_addr_t *bus_addr,
			size_t size, int direction)
{
	if (NULL != hwdev) {
#ifdef CONFIG_PLATFORM_AML_S905_V1
		if (g_pcie_reserved_mem_dev)
			hwdev->dev.dma_mask = NULL;
#endif
		*bus_addr = dma_map_single(&hwdev->dev, vir_addr, size, direction);
	} else {
		RTW_ERR("pcie hwdev handle is NULL!\n");
		*bus_addr = (dma_addr_t)virt_to_phys(vir_addr);
		/*RTW_ERR("Get bus_addr: %x by virt_to_phys()\n", bus_addr);*/
	}
}
void pci_unmap_bus_addr(struct pci_dev *hwdev,
			dma_addr_t *bus_addr, size_t size, int direction)
{
	if (NULL != hwdev && NULL != bus_addr) {
#ifdef CONFIG_PLATFORM_AML_S905_V1
		if (g_pcie_reserved_mem_dev)
			hwdev->dev.dma_mask = NULL;
#endif
		dma_unmap_single(&hwdev->dev, *bus_addr, size, direction);
	} else
		RTW_ERR("pcie hwdev handle or bus addr is NULL!\n");
}
void *pci_alloc_cache_mem(struct pci_dev *pdev,
			dma_addr_t *bus_addr, size_t size, int direction)
{
	void *vir_addr = NULL;

	vir_addr = rtw_zmalloc(size);

	if (!vir_addr)
		bus_addr = NULL;
	else
		pci_get_bus_addr(pdev, vir_addr, bus_addr, size, direction);

	return vir_addr;
}

void *pci_alloc_noncache_mem(struct pci_dev *pdev,
			dma_addr_t *bus_addr, size_t size)
{
	void *vir_addr = NULL;
	struct device *dev = NULL;

#ifdef CONFIG_RTW_DEDICATED_CMA_POOL
	if (NULL != g_pldev){
		dev = &g_pldev->dev;
#else
	if (NULL != pdev) {
		dev = &pdev->dev;
#ifdef CONFIG_PLATFORM_AML_S905
		if (g_pcie_reserved_mem_dev)\
			dev = g_pcie_reserved_mem_dev;
#endif
#endif
		vir_addr = dma_alloc_coherent(dev,
				size, bus_addr,
				(in_atomic() ? GFP_ATOMIC : GFP_KERNEL));
	}
	if (!vir_addr)
		bus_addr = NULL;
	else
		bus_addr = (dma_addr_t *)((((SIZE_PTR)bus_addr + 3) / 4) * 4);

	return vir_addr;
}
void pci_free_cache_mem(struct pci_dev *pdev,
		void *vir_addr, dma_addr_t *bus_addr,
		size_t size, int direction)
{
	pci_unmap_bus_addr(pdev, bus_addr, size, direction);
	rtw_mfree(vir_addr, size);

	vir_addr = NULL;
}

void pci_free_noncache_mem(struct pci_dev *pdev,
		void *vir_addr, dma_addr_t *bus_addr, size_t size)
{
	struct device *dev = NULL;

#ifdef CONFIG_RTW_DEDICATED_CMA_POOL
	if (NULL != g_pldev){
		dev = &g_pldev->dev;
#else
	if (NULL != pdev){
		dev = &pdev->dev;
#ifdef CONFIG_PLATFORM_AML_S905
	if (g_pcie_reserved_mem_dev)
		dev = g_pcie_reserved_mem_dev;
#endif
#endif
		dma_free_coherent(dev, size, vir_addr, *bus_addr);
	}
	vir_addr = NULL;
}

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

extern int ky_wlan_set_power(int on);

void pci_cache_wback(struct pci_dev *hwdev,
			dma_addr_t *bus_addr, size_t size, int direction)
{
	if (NULL != hwdev && NULL != bus_addr && *bus_addr != DMA_MAPPING_ERROR) {
	  	dma_sync_single_for_device(&hwdev->dev, *bus_addr, size,
					direction);
	} else
		RTW_ERR("pcie hwdev handle or bus addr is NULL!\n");
}
void pci_cache_inv(struct pci_dev *hwdev,
			dma_addr_t *bus_addr, size_t size, int direction)
{
	if (NULL != hwdev && NULL != bus_addr && *bus_addr != DMA_MAPPING_ERROR) {
		dma_sync_single_for_cpu(&hwdev->dev, *bus_addr, size, direction);
	} else
		RTW_ERR("pcie hwdev handle or bus addr is NULL!\n");
}
void pci_get_bus_addr(struct pci_dev *hwdev,
			void *vir_addr, dma_addr_t *bus_addr,
			size_t size, int direction)
{
	if (NULL != hwdev) {
		*bus_addr = dma_map_single(&hwdev->dev, vir_addr, size, direction);
		if (dma_mapping_error(&hwdev->dev, *bus_addr)) {
			RTW_ERR("dma_map_single error, bus addr is invalid!\n");
			*bus_addr = DMA_MAPPING_ERROR;
		}
	} else {
		RTW_ERR("pcie hwdev handle is NULL!\n");
		*bus_addr = (dma_addr_t)virt_to_phys(vir_addr);
		/*RTW_ERR("Get bus_addr: %x by virt_to_phys()\n", bus_addr);*/
	}
}
void pci_unmap_bus_addr(struct pci_dev *hwdev,
			dma_addr_t *bus_addr, size_t size, int direction)
{
	if (NULL != hwdev && NULL != bus_addr && *bus_addr != DMA_MAPPING_ERROR) {
		dma_unmap_single(&hwdev->dev, *bus_addr, size, direction);
	} else
		RTW_ERR("pcie hwdev handle or bus addr is NULL!\n");
}
void *pci_alloc_cache_mem(struct pci_dev *pdev,
			dma_addr_t *bus_addr, size_t size, int direction)
{
	void *vir_addr = NULL;

	vir_addr = rtw_zmalloc(size);

	if (!vir_addr && bus_addr)
		*bus_addr = DMA_MAPPING_ERROR;
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
#endif
		dma_free_coherent(dev, size, vir_addr, *bus_addr);
	}
	vir_addr = NULL;
}

extern int get_wifi_chip_type(void);
extern int rockchip_wifi_power(int on);
extern int rockchip_wifi_set_carddetect(int val);

void platform_wifi_get_oob_irq(int *oob_irq)
{

}

void platform_wifi_mac_addr(u8 *mac_addr)
{

}

int platform_wifi_power_on(void)
{
	int ret = 0;

	RTW_PRINT("\n");
	RTW_PRINT("=======================================================\n");
	RTW_PRINT("==== Launching Wi-Fi driver! (Powered by Ky) ====\n");
	RTW_PRINT("=======================================================\n");
	RTW_PRINT("Realtek %s WiFi driver (Powered by Ky,Ver %s) init.\n", DRV_NAME, DRIVERVERSION);
	ky_wlan_set_power(1);

	return ret;
}

void platform_wifi_power_off(void)
{
	RTW_PRINT("\n");
	RTW_PRINT("=======================================================\n");
	RTW_PRINT("==== Dislaunching Wi-Fi driver! (Powered by Ky) ====\n");
	RTW_PRINT("=======================================================\n");
	RTW_PRINT("Realtek %s WiFi driver (Powered by Ky,Ver %s) init.\n", DRV_NAME, DRIVERVERSION);

	ky_wlan_set_power(0);
}
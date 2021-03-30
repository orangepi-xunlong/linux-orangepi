
/*
 ******************************************************************************
 *
 * vfe_os.c
 *
 * Hawkview ISP - vfe_os.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/12/02	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include <linux/module.h>
#include "vin_os.h"

unsigned int vin_log_mask;

EXPORT_SYMBOL_GPL(vin_log_mask);

int os_gpio_request(struct gpio_config *pin_cfg, __u32 group_count_max)
{
#ifdef VIN_GPIO
	int ret = 0;
	char pin_name[32];

	if (pin_cfg == NULL)
		return -1;

	if (pin_cfg->gpio == GPIO_INDEX_INVALID)
		return -1;

	ret = gpio_request(pin_cfg->gpio, NULL);
	if (0 != ret) {
		sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
		vin_warn("%s request failed, gpio%d, ret = %d\n", pin_name,
			 pin_cfg->gpio, ret);
		return -1;
	}
#endif
	return 0;
}

EXPORT_SYMBOL_GPL(os_gpio_request);
int os_gpio_set(struct gpio_config *pin_cfg, __u32 group_count_max)
{
#ifdef VIN_GPIO
	char pin_name[32];
	__u32 config;

	if (pin_cfg == NULL)
		return -1;
	if (pin_cfg->gpio == GPIO_INDEX_INVALID)
		return -1;

	if (!IS_AXP_PIN(pin_cfg->gpio)) {
		/* valid pin of sunxi-pinctrl,
		 * config pin attributes individually.
		 */
		sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
		config =
		    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (pin_cfg->pull != GPIO_PULL_DEFAULT) {
			config =
			    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD,
					      pin_cfg->pull);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
			config =
			    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV,
					      pin_cfg->drv_level);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg->data != GPIO_DATA_DEFAULT) {
			config =
			    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT,
					      pin_cfg->data);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
	} else {
		/* valid pin of axp-pinctrl,
		 * config pin attributes individually.
		 */
		sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
		config =
		    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
		pin_config_set(AXP_PINCTRL, pin_name, config);
		if (pin_cfg->data != GPIO_DATA_DEFAULT) {
			config =
			    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT,
					      pin_cfg->data);
			pin_config_set(AXP_PINCTRL, pin_name, config);
		}
	}
#endif
	return 0;
}

EXPORT_SYMBOL_GPL(os_gpio_set);

int os_gpio_release(u32 p_handler, __s32 if_release_to_default_status)
{
#ifdef VIN_GPIO
	if (p_handler != GPIO_INDEX_INVALID) {
		gpio_free(p_handler);
	} else {
		vin_log(VIN_LOG_POWER, "os_gpio_release, hdl is INVALID\n");
	}
#endif
	return 0;
}

EXPORT_SYMBOL_GPL(os_gpio_release);

int os_gpio_write(u32 p_handler, __u32 value_to_gpio, const char *gpio_name,
		  int force_value_flag)
{
#ifdef VIN_GPIO
	if (1 == force_value_flag) {
		if (p_handler != GPIO_INDEX_INVALID)
			__gpio_set_value(p_handler, value_to_gpio);
		else
			vin_log(VIN_LOG_POWER, "os_gpio_write, hdl is INVALID\n");
	} else {
		if (p_handler != GPIO_INDEX_INVALID) {
			if (value_to_gpio == 0) {
				os_gpio_set_status(p_handler, 1, gpio_name);
				__gpio_set_value(p_handler, value_to_gpio);
			} else {
				os_gpio_set_status(p_handler, 0, gpio_name);
			}
		} else {
			vin_log(VIN_LOG_POWER, "os_gpio_write, hdl is INVALID\n");
		}
	}
#endif
	return 0;
}

EXPORT_SYMBOL_GPL(os_gpio_write);

int os_gpio_set_status(u32 p_handler, __u32 if_set_to_output_status,
		       const char *gpio_name)
{
	int ret = 0;
#ifdef VIN_GPIO
	if (p_handler != GPIO_INDEX_INVALID) {
		if (if_set_to_output_status) {
			ret = gpio_direction_output(p_handler, 0);
			if (ret != 0) {
				vin_warn("gpio_direction_output fail!\n");
			}
		} else {
			ret = gpio_direction_input(p_handler);
			if (ret != 0) {
				vin_warn("gpio_direction_input fail!\n");
			}
		}
	} else {
		vin_warn("os_gpio_set_status, hdl is INVALID\n");
		ret = -1;
	}
#endif
	return ret;
}

EXPORT_SYMBOL_GPL(os_gpio_set_status);

int os_mem_alloc(struct vin_mm *mem_man)
{
#ifdef SUNXI_MEM
	int ret = -1;
	char *ion_name = "ion_vfe";
	mem_man->client = sunxi_ion_client_create(ion_name);
	if (IS_ERR_OR_NULL(mem_man->client)) {
		vin_err("sunxi_ion_client_create failed!!");
		goto err_client;
	}
	mem_man->handle = ion_alloc(mem_man->client, mem_man->size, PAGE_SIZE,
				    /*ION_HEAP_CARVEOUT_MASK| */
				    ION_HEAP_TYPE_DMA_MASK, 0);
	if (IS_ERR_OR_NULL(mem_man->handle)) {
		vin_err("ion_alloc failed!!");
		goto err_alloc;
	}
	mem_man->vir_addr = ion_map_kernel(mem_man->client, mem_man->handle);
	if (IS_ERR_OR_NULL(mem_man->vir_addr)) {
		vin_err("ion_map_kernel failed!!");
		goto err_map_kernel;
	}
	ret =
	    ion_phys(mem_man->client, mem_man->handle,
		     (ion_phys_addr_t *)&mem_man->phy_addr, &mem_man->size);
	if (ret) {
		vin_err("ion_phys failed!!");
		goto err_phys;
	}
	mem_man->dma_addr =
	    mem_man->phy_addr + HW_DMA_OFFSET - CPU_DRAM_PADDR_ORG;
	return 0;
err_phys:
	ion_unmap_kernel(mem_man->client, mem_man->handle);
err_map_kernel:
	ion_free(mem_man->client, mem_man->handle);
err_alloc:
	ion_client_destroy(mem_man->client);
err_client:
	return -1;
#else
	mem_man->vir_addr = dma_alloc_coherent(NULL, (size_t) mem_man->size,
					(dma_addr_t *)&mem_man->phy_addr,
					GFP_KERNEL);
	if (!mem_man->vir_addr) {
		vin_err("dma_alloc_coherent memory alloc size %d failed\n",
			mem_man->size);
		return -ENOMEM;
	}
	mem_man->dma_addr =
	    mem_man->phy_addr + HW_DMA_OFFSET - CPU_DRAM_PADDR_ORG;
	return 0;
#endif
}

EXPORT_SYMBOL_GPL(os_mem_alloc);

void os_mem_free(struct vin_mm *mem_man)
{
#ifdef SUNXI_MEM
	if (IS_ERR_OR_NULL(mem_man->client) || IS_ERR_OR_NULL(mem_man->handle)
	    || IS_ERR_OR_NULL(mem_man->vir_addr))
		return;
	ion_unmap_kernel(mem_man->client, mem_man->handle);
	ion_free(mem_man->client, mem_man->handle);
	ion_client_destroy(mem_man->client);
#else
	if (mem_man->vir_addr)
		dma_free_coherent(NULL, mem_man->size, mem_man->vir_addr,
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

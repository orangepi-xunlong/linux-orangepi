/*
 * linux-4.9/drivers/media/platform/sunxi-vin/utility/vin_os.c
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

/*
 ******************************************************************************
 *
 * vfe_os.c
 *
 * Hawkview ISP - vfe_os.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version         Author         Date       Description
 *
 *   3.0          Yang Feng    2015/12/02  ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#include <linux/module.h>
#include "vin_os.h"

unsigned int vin_log_mask;
EXPORT_SYMBOL_GPL(vin_log_mask);

int os_gpio_set(struct gpio_config *gc)
{
#ifndef FPGA_VER
	char pin_name[32];
	__u32 config;

	if (gc == NULL)
		return -1;
	if (gc->gpio == GPIO_INDEX_INVALID)
		return -1;

	if (!IS_AXP_PIN(gc->gpio)) {
		/* valid pin of sunxi-pinctrl,
		 * config pin attributes individually.
		 */
		sunxi_gpio_to_name(gc->gpio, pin_name);
		config =
		    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gc->mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (gc->pull != GPIO_PULL_DEFAULT) {
			config =
			    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, gc->pull);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (gc->drv_level != GPIO_DRVLVL_DEFAULT) {
			config =
			    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV,
					      gc->drv_level);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
		if (gc->data != GPIO_DATA_DEFAULT) {
			config =
			    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, gc->data);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		}
	} else {
		/* valid pin of axp-pinctrl,
		 * config pin attributes individually.
		 */
		sunxi_gpio_to_name(gc->gpio, pin_name);
		config =
		    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gc->mul_sel);
		pin_config_set(AXP_PINCTRL, pin_name, config);
		if (gc->data != GPIO_DATA_DEFAULT) {
			config =
			    SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, gc->data);
			pin_config_set(AXP_PINCTRL, pin_name, config);
		}
	}
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(os_gpio_set);

int os_gpio_write(u32 gpio, __u32 out_value, int force_value_flag)
{
#ifndef FPGA_VER
	if (gpio == GPIO_INDEX_INVALID)
		return 0;

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

int os_mem_alloc(struct device *dev, struct vin_mm *mem_man)
{
	mem_man->vir_addr = dma_alloc_coherent(dev, (size_t) mem_man->size,
					(dma_addr_t *)&mem_man->phy_addr,
					GFP_KERNEL);
	if (!mem_man->vir_addr) {
		vin_err("dma_alloc_coherent memory alloc failed\n");
		return -ENOMEM;
	}
	mem_man->dma_addr = mem_man->phy_addr;
	return 0;
}
EXPORT_SYMBOL_GPL(os_mem_alloc);

void os_mem_free(struct device *dev, struct vin_mm *mem_man)
{
	if (mem_man->vir_addr)
		dma_free_coherent(dev, mem_man->size, mem_man->vir_addr,
				  (dma_addr_t) mem_man->phy_addr);
	mem_man->phy_addr = NULL;
	mem_man->dma_addr = NULL;
	mem_man->vir_addr = NULL;
}
EXPORT_SYMBOL_GPL(os_mem_free);

MODULE_AUTHOR("raymonxiu");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video front end OSAL for sunxi");

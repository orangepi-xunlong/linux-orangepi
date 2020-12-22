/*
 * Platform interfaces for XRadio drivers
 *
 * Implemented by platform vendor(such as AllwinnerTech).
 *
 * Copyright (c) 2013, XRadio
 * Author: XRadio
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/delay.h>

#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/ioport.h>

#include <linux/regulator/consumer.h>
#include <asm/mach-types.h>
#include <mach/sys_config.h>

#include "xradio.h"
#include "platform.h"
#include "sbus.h"

static int wlan_bus_id;
static int wlan_irq_gpio;
extern int rf_module_power(int onoff);
extern void wifi_pm_power(int on);

/*********************Interfaces called by xradio core. *********************/
static int xradio_get_syscfg(void)
{
	script_item_u val;
	script_item_value_type_e type;

	/* Get SDIO/USB config. */
#if defined(CONFIG_XRADIO_SDIO)
	type = script_get_item("wifi_para", "wifi_sdc_id", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(KERN_ERR "failed to fetch sdio card's sdcid\n");
		return -1;
	}
#elif defined(CONFIG_XRADIO_USB)
	type = script_get_item("wifi_para", "wifi_usbc_id", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(KERN_ERR "failed to fetch usb's id\n");
		return -1;
	}
#endif
	wlan_bus_id = val.val;

	type = script_get_item("wifi_para", "wl_host_wake", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk(KERN_ERR "failed to fetch xradio_wl_host_wake\n");
		return -1;
	}
	wlan_irq_gpio = val.gpio.gpio;

	return 0;
}

int  xradio_plat_init(void)
{
	int ret = 0;

	ret = xradio_get_syscfg();
	if (ret)
		return ret;

#if defined(PMU_POWER_WLAN_RETAIN)
	rf_module_power(1);
#endif
	return ret;
}

void xradio_plat_deinit(void)
{
#if defined(PMU_POWER_WLAN_RETAIN)
	rf_module_power(0);
#endif
}

int xradio_wlan_power(int on)
{
	printk(KERN_ERR "xradio wlan power %s\n", on?"on":"off");
	if (on) {
		wifi_pm_power(1);
		mdelay(50);
	} else {
		wifi_pm_power(0);
	}
	return 0;
}

int xradio_sdio_detect(int enable)
{
	int insert = enable;
	MCI_RESCAN_CARD(wlan_bus_id, insert);
	xradio_dbg(XRADIO_DBG_ALWY, "%s SDIO card %d\n",
	           enable?"Detect":"Remove", wlan_bus_id);
	mdelay(10);
	return 0;
}

#ifdef CONFIG_XRADIO_USE_GPIO_IRQ
static u32 gpio_irq_handle = 0;
static irqreturn_t xradio_gpio_irq_handler(int irq, void *sbus_priv)
{
	struct sbus_priv *self = (struct sbus_priv *)sbus_priv;
	unsigned long flags;

	SYS_BUG(!self);
	spin_lock_irqsave(&self->lock, flags);
	if (self->irq_handler)
		self->irq_handler(self->irq_priv);
	spin_unlock_irqrestore(&self->lock, flags);
	return IRQ_HANDLED;
}

int xradio_request_gpio_irq(struct device *dev, void *sbus_priv)
{
	int ret = -1;
	if(!gpio_irq_handle) {
		gpio_request(wlan_irq_gpio, "xradio_irq");
		gpio_direction_input(wlan_irq_gpio);
		gpio_irq_handle = gpio_to_irq(wlan_irq_gpio);
		ret = devm_request_irq(dev, gpio_irq_handle,
		                      (irq_handler_t)xradio_gpio_irq_handler,
		                       IRQF_TRIGGER_RISING, "xradio_irq", sbus_priv);
		if (IS_ERR_VALUE(ret)) {
			gpio_irq_handle = 0;
		}
	} else {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: error, irq exist already!\n", __func__);
	}

	if (gpio_irq_handle) {
		xradio_dbg(XRADIO_DBG_NIY, "%s: request_irq sucess! irq=0x%08x\n",
		           __func__, gpio_irq_handle);
		ret = 0;
	} else {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: request_irq err: %d\n", __func__, ret);
		ret = -1;
	}
	return ret;
}

void xradio_free_gpio_irq(struct device *dev, void *sbus_priv)
{
	struct sbus_priv *self = (struct sbus_priv *)sbus_priv;

	if(gpio_irq_handle) {
		//for linux3.4
		devm_free_irq(dev, gpio_irq_handle, self);
		gpio_free(wlan_irq_gpio);
		gpio_irq_handle = 0;
	}
}
#endif /* CONFIG_XRADIO_USE_GPIO_IRQ */

/******************************************************************************************/
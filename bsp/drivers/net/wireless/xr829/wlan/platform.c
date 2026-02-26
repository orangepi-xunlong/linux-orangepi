/*
 * platform interfaces for XRadio drivers
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
//#include <asm/mach-types.h>
//#include <mach/sys_config.h>
#include "xradio.h"
#include "platform.h"
#include "sbus.h"
#include <linux/gpio.h>
#include <linux/types.h>
//#include <linux/power/scenelock.h>
//#include <linux/power/aw_pm.h>
#include <linux/pm_wakeirq.h>

MODULE_AUTHOR("XRadioTech");
MODULE_DESCRIPTION("XRadioTech WLAN driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("xradio_wlan");

extern void sunxi_wlan_set_power(bool on);
extern int sunxi_wlan_get_bus_index(void);
extern int sunxi_wlan_get_oob_irq(int *, int *);
extern void sunxi_bluetooth_set_power(bool on_off);

static int wlan_bus_id;
static u32 gpio_irq_handle;
static int irq_flags, wakeup_enable;

int xradio_get_syscfg(void)
{
	int wlan_bus_index = 0;
	wlan_bus_index = sunxi_wlan_get_bus_index();
	if (wlan_bus_index < 0)
		return wlan_bus_index;
	else
		wlan_bus_id = wlan_bus_index;
	gpio_irq_handle = sunxi_wlan_get_oob_irq(&irq_flags, &wakeup_enable);
	return wlan_bus_index;
}
/*********************Interfaces called by xradio core. *********************/
int  xradio_plat_init(void)
{
  return 0;
}

void xradio_plat_deinit(void)
{
;
}

int xradio_wlan_power(int on)
{
	int ret = 0;
	if (on) {
	    ret = xradio_get_syscfg();
		if (ret < 0)
			return ret;
	}
	sunxi_wlan_set_power(on);
	mdelay(100);
	return ret;
}

void xradio_bt_power(int on_off)
{
	sunxi_bluetooth_set_power(on_off);
	mdelay(100);
}

void xradio_sdio_detect(int enable)
{
	MCI_RESCAN_CARD(wlan_bus_id);
	xradio_dbg(XRADIO_DBG_ALWY, "%s SDIO card %d\n",
				enable?"Detect":"Remove", wlan_bus_id);
	mdelay(10);
}

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

	ret = devm_request_irq(dev, gpio_irq_handle,
					(irq_handler_t)xradio_gpio_irq_handler,
					irq_flags, "xradio_irq", sbus_priv);
	if (ret < 0) {
			gpio_irq_handle = 0;
			xradio_dbg(XRADIO_DBG_ERROR, "%s: request_irq FAIL!ret=%d\n",
					__func__, ret);
	}

	if (wakeup_enable) {
		ret = device_init_wakeup(dev, true);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "device init wakeup failed!\n");
			return ret;
		}

		ret = dev_pm_set_wake_irq(dev, gpio_irq_handle);
		if (ret < 0) {
			xradio_dbg(XRADIO_DBG_ERROR, "can't enable wakeup src!\n");
			return ret;
		}
	}

	return ret;
}

void xradio_free_gpio_irq(struct device *dev, void *sbus_priv)
{
	struct sbus_priv *self = (struct sbus_priv *)sbus_priv;
	if (wakeup_enable) {
		device_init_wakeup(dev, false);
		dev_pm_clear_wake_irq(dev);
	}
	devm_free_irq(dev, gpio_irq_handle, self);
	gpio_irq_handle = 0;
}

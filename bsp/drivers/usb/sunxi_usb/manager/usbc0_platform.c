/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2011-4-14, create this file
 *
 * usb controller0 device info.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/power_supply.h>

#include <asm/io.h>
#include <asm/unaligned.h>

#include  "../include/sunxi_usb_config.h"
#include  "usb_hw_scan.h"
#include  "usb_msg_center.h"
#include  "usbc_platform.h"

int usb_hw_scan_debug;

static int set_cc_status(int type)
{
	struct power_supply *pmu_psy;
	union power_supply_propval temp;

	if (g_usb_cfg.port.detect_type == USB_DETECT_TYPE_VBUS_PMU) {
		pmu_psy = g_usb_cfg.port.pmu_psy;
		if (pmu_psy) {
			temp.intval = type;
			power_supply_set_property(pmu_psy, POWER_SUPPLY_PROP_SCOPE, &temp);
		}
	}

	return 0;
}

static ssize_t otg_enable(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	mutex_lock(&g_usb_cfg.lock);
	/* restart usb scan */
	atomic_set(&rolesw_suspend_flag, 0);
	atomic_set(&thread_suspend_flag, 0);
	atomic_set(&notify_suspend_flag, 0);
	set_cc_status(POWER_SUPPLY_SCOPE_UNKNOWN);
	if (g_usb_cfg.port.detect_mode == USB_DETECT_MODE_NOTIFY) {
		/**
		 * rmmod usb device/host driver first,
		 * then insmod usb host/device driver.
		 */
		usb_hw_scan(&g_usb_cfg);
		usb_msg_center(&g_usb_cfg);

		msleep(1000);
		usb_hw_scan(&g_usb_cfg);
		usb_msg_center(&g_usb_cfg);
		/* restore det_flag for auto detect */
		atomic_set(&g_usb_cfg.det_flag, 1);
	}
	mutex_unlock(&g_usb_cfg.lock);

	return sprintf(buf, "%s\n", "otg_enable finished!");
}

static ssize_t device_chose(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	mutex_lock(&g_usb_cfg.lock);
	/* stop usb scan */
	atomic_set(&rolesw_suspend_flag, 1);
	atomic_set(&thread_suspend_flag, 1);
	atomic_set(&notify_suspend_flag, 1);

	hw_rmmod_usb_host();
	hw_rmmod_usb_device();
	usb_msg_center(&g_usb_cfg);

	hw_insmod_usb_device();
	usb_msg_center(&g_usb_cfg);
	set_cc_status(POWER_SUPPLY_SCOPE_DEVICE);
	mutex_unlock(&g_usb_cfg.lock);

	return sprintf(buf, "%s\n", "device_chose finished, otg disabled!");
}

static ssize_t host_chose(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	mutex_lock(&g_usb_cfg.lock);
	/* stop usb scan */
	atomic_set(&rolesw_suspend_flag, 1);
	atomic_set(&thread_suspend_flag, 1);
	atomic_set(&notify_suspend_flag, 1);

	hw_rmmod_usb_host();
	hw_rmmod_usb_device();
	usb_msg_center(&g_usb_cfg);

	hw_insmod_usb_host();
	usb_msg_center(&g_usb_cfg);
	set_cc_status(POWER_SUPPLY_SCOPE_SYSTEM);
	mutex_unlock(&g_usb_cfg.lock);

	return sprintf(buf, "%s\n", "host_chose finished, otg disabled!");
}

static ssize_t null_chose(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	mutex_lock(&g_usb_cfg.lock);
	/* stop usb scan */
	atomic_set(&rolesw_suspend_flag, 1);
	atomic_set(&thread_suspend_flag, 1);
	atomic_set(&notify_suspend_flag, 1);

	hw_rmmod_usb_host();
	hw_rmmod_usb_device();
	usb_msg_center(&g_usb_cfg);
	mutex_unlock(&g_usb_cfg.lock);

	return sprintf(buf, "%s\n", "null_chose finished, otg disabled!");
}

static ssize_t show_otg_role(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int value = 0;
	char buf_role[16];

	value = get_usb_role();

	switch (value) {
	case SW_USB_ROLE_NULL:
		strcpy(buf_role, "null");
		break;

	case SW_USB_ROLE_DEVICE:
		strcpy(buf_role, "usb_device");
		break;

	case SW_USB_ROLE_HOST:
		strcpy(buf_role, "usb_host");
		break;

	default:
		DMSG_INFO("err: unknown otg role(%d)\n", value);
		strcpy(buf_role, "unknown");
	}

	return sprintf(buf, "%s\n", buf_role);
}

static ssize_t set_otg_role(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int value = 0;
	int ret = 0;

	if (strncmp(buf, "usb_host", 8) == 0) {
		value = SW_USB_ROLE_HOST;
	} else if (strncmp(buf, "usb_device", 10) == 0) {
		value = SW_USB_ROLE_DEVICE;
	} else if (strncmp(buf, "null", 4) == 0) {
		value = SW_USB_ROLE_NULL;
	} else {
		ret = kstrtoint(buf, 0, &value);
		if (ret != 0)
			return ret;
	}

	switch (value) {
	case SW_USB_ROLE_NULL:
		null_chose(dev, attr, (char *)buf);
		break;

	case SW_USB_ROLE_DEVICE:
		device_chose(dev, attr, (char *)buf);
		break;

	case SW_USB_ROLE_HOST:
		host_chose(dev, attr, (char *)buf);
		break;

	default:
		DMSG_INFO("err: unknown otg role(%d)\n", value);
		null_chose(dev, attr, (char *)buf);
	}

	return count;
}

static struct device_attribute chose_attrs[] = {
	__ATTR(usb_otg, 0400, otg_enable, NULL),
	__ATTR(usb_null, 0400, null_chose, NULL),
	__ATTR(usb_host, 0400, host_chose, NULL),
	__ATTR(usb_device, 0400, device_chose, NULL),
	__ATTR(otg_role, 0644, show_otg_role, set_otg_role),
};

static ssize_t show_otg_hw_scan_debug(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", usb_hw_scan_debug);
}

static ssize_t otg_hw_scan_debug(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int debug = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &debug);
	if (ret != 0)
		return ret;

	usb_hw_scan_debug = debug;

	return count;
}
static DEVICE_ATTR(hw_scan_debug,
		0644,
		show_otg_hw_scan_debug,
		otg_hw_scan_debug);

__s32 create_node_file(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0;

	mutex_init(&g_usb_cfg.lock);
	device_create_file(&pdev->dev, &dev_attr_hw_scan_debug);

	for (i = 0; i < ARRAY_SIZE(chose_attrs); i++) {
		ret = device_create_file(&pdev->dev, &chose_attrs[i]);
		if (ret)
			DMSG_INFO("create_chose_attrs_file fail\n");
	}

	return 0;
}

__s32 remove_node_file(struct platform_device *pdev)
{
	int i = 0;

	device_remove_file(&pdev->dev, &dev_attr_hw_scan_debug);

	for (i = 0; i < ARRAY_SIZE(chose_attrs); i++)
		device_remove_file(&pdev->dev, &chose_attrs[i]);

	return 0;
}

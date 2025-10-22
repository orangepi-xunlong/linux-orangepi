/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yangnaitian, 2011-5-24, create this file
 * javen, 2011-6-26, add suspend and resume
 * javen, 2011-7-18, move clock and power operations out from driver
 *
 * SoftWinner EHCI Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/regulator/consumer.h>
#include "ehci.h"
#include "sunxi-hci.h"

#define DRIVER_DESC "EHCI SUNXI driver"

#define  SUNXI_EHCI_NAME	"sunxi-ehci"
static const char ehci_name[] = SUNXI_EHCI_NAME;

#if IS_ENABLED(CONFIG_USB_SUNXI_EHCI0)
#define  SUNXI_EHCI0_OF_MATCH	"allwinner,sunxi-ehci0"
#else
#define  SUNXI_EHCI0_OF_MATCH   "null"
#endif

#if IS_ENABLED(CONFIG_USB_SUNXI_EHCI1)
#define  SUNXI_EHCI1_OF_MATCH	"allwinner,sunxi-ehci1"
#else
#define  SUNXI_EHCI1_OF_MATCH   "null"
#endif

#if IS_ENABLED(CONFIG_USB_SUNXI_EHCI2)
#define  SUNXI_EHCI2_OF_MATCH	"allwinner,sunxi-ehci2"
#else
#define  SUNXI_EHCI2_OF_MATCH   "null"
#endif

#if IS_ENABLED(CONFIG_USB_SUNXI_EHCI3)
#define  SUNXI_EHCI3_OF_MATCH	"allwinner,sunxi-ehci3"
#else
#define  SUNXI_EHCI3_OF_MATCH   "null"
#endif

static struct sunxi_hci_hcd *g_sunxi_ehci[4];
static u32 ehci_first_probe[4] = {1, 1, 1, 1};
static u32 ehci_enable[4] = {1, 1, 1, 1};
static atomic_t ehci_in_standby;

#if IS_ENABLED(CONFIG_PM)
static void sunxi_ehci_resume_work(struct work_struct *work);
#endif

int sunxi_usb_disable_ehci(__u32 usbc_no);
int sunxi_usb_enable_ehci(__u32 usbc_no);

static void  ehci_set_interrupt_enable(const struct ehci_hcd *ehci,
		void __iomem *regs, u32 enable)
{
	ehci_writel(ehci, enable & 0x3f, (regs + EHCI_OPR_USBINTR));
}

static void ehci_disable_periodic_schedule(const struct ehci_hcd *ehci,
		void __iomem *regs)
{
	u32 reg_val = 0;

	reg_val =  ehci_readl(ehci, (regs + EHCI_OPR_USBCMD));
	reg_val	&= ~(0x1<<4);
	ehci_writel(ehci, reg_val, (regs + EHCI_OPR_USBCMD));
}

static void ehci_disable_async_schedule(const struct ehci_hcd *ehci,
		void __iomem *regs)
{
	unsigned int reg_val = 0;

	reg_val =  ehci_readl(ehci, (regs + EHCI_OPR_USBCMD));
	reg_val &= ~(0x1<<5);
	ehci_writel(ehci, reg_val, (regs + EHCI_OPR_USBCMD));
}

static void ehci_set_config_flag(const struct ehci_hcd *ehci,
		void __iomem *regs)
{
	ehci_writel(ehci, 0x1, (regs + EHCI_OPR_CFGFLAG));
}

static void ehci_test_stop(const struct ehci_hcd *ehci,
		void __iomem *regs)
{
	unsigned int reg_val = 0;

	reg_val =  ehci_readl(ehci, (regs + EHCI_OPR_USBCMD));
	reg_val &= (~0x1);
	ehci_writel(ehci, reg_val, (regs + EHCI_OPR_USBCMD));
}

static void ehci_test_reset(const struct ehci_hcd *ehci,
		void __iomem *regs)
{
	u32 reg_val = 0;

	reg_val =  ehci_readl(ehci, (regs + EHCI_OPR_USBCMD));
	reg_val	|= (0x1<<1);
	ehci_writel(ehci, reg_val, (regs + EHCI_OPR_USBCMD));
}

static unsigned int ehci_test_reset_complete(const struct ehci_hcd *ehci,
		void __iomem *regs)
{

	unsigned int reg_val = 0;

	reg_val = ehci_readl(ehci, (regs + EHCI_OPR_USBCMD));
	reg_val &= (0x1<<1);

	return !reg_val;
}

static void ehci_start(const struct ehci_hcd *ehci, void __iomem *regs)
{
	unsigned int reg_val = 0;

	reg_val =  ehci_readl(ehci, (regs + EHCI_OPR_USBCMD));
	reg_val	|= 0x1;
	ehci_writel(ehci, reg_val, (regs + EHCI_OPR_USBCMD));
}

static unsigned int ehci_is_halt(const struct ehci_hcd *ehci,
		void __iomem *regs)
{
	unsigned int reg_val = 0;

	reg_val = ehci_readl(ehci, (regs + EHCI_OPR_USBSTS))  >> 12;
	reg_val &= 0x1;
	return reg_val;
}

static void ehci_port_control(const struct ehci_hcd *ehci,
		void __iomem *regs, u32 port_no, u32 control)
{
	ehci_writel(ehci, control, (regs + EHCI_OPR_USBCMD + (port_no<<2)));
}

static void  ehci_put_port_suspend(const struct ehci_hcd *ehci,
		void __iomem *regs)
{
	unsigned int reg_val = 0;

	reg_val =  ehci_readl(ehci, (regs + EHCI_OPR_PORTSC));
	reg_val	|= (0x01<<7);
	ehci_writel(ehci, reg_val, (regs + EHCI_OPR_PORTSC));
}

static void ehci_test_mode(const struct ehci_hcd *ehci,
		void __iomem *regs, u32 test_mode)
{
	unsigned int reg_val = 0;

	reg_val =  ehci_readl(ehci, (regs + EHCI_OPR_PORTSC));
	reg_val &= ~(0x0f<<16);
	reg_val |= test_mode;
	ehci_writel(ehci, reg_val, (regs + EHCI_OPR_PORTSC));
}

static void __ehci_ed_test(const struct ehci_hcd *ehci,
		void __iomem *regs, __u32 test_mode)
{
	ehci_set_interrupt_enable(ehci, regs, 0x00);
	ehci_disable_periodic_schedule(ehci, regs);
	ehci_disable_async_schedule(ehci, regs);

	ehci_set_config_flag(ehci, regs);

	ehci_test_stop(ehci, regs);
	ehci_test_reset(ehci, regs);

	/* Wait until EHCI reset complete. */
	while (!ehci_test_reset_complete(ehci, regs))
		;

	if (!ehci_is_halt(ehci, regs))
		DMSG_ERR("%s_%d\n", __func__, __LINE__);

	ehci_start(ehci, regs);
	/* Wait until EHCI to be not halt. */
	while (ehci_is_halt(ehci, regs))
		;

	/* Ehci start, config to test. */
	ehci_set_config_flag(ehci, regs);
	ehci_port_control(ehci, regs, 0, EHCI_PORTSC_POWER);

	ehci_disable_periodic_schedule(ehci, regs);
	ehci_disable_async_schedule(ehci, regs);

	/* Put port suspend. */
	ehci_put_port_suspend(ehci, regs);

	ehci_test_stop(ehci, regs);

	while ((!ehci_is_halt(ehci, regs)))
		;

	/* Test pack. */
	DMSG_INFO("Start Host Test,mode:0x%x!\n", test_mode);
	ehci_test_mode(ehci, regs, test_mode);
	DMSG_INFO("End Host Test,mode:0x%x!\n", test_mode);
}

static ssize_t show_ed_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "USB2.0 host test mode:\n"
				"echo:\ntest_j_state\ntest_k_state\ntest_se0_nak\n"
				"test_pack\ntest_force_enable\ntest_mask\n\n");
}

static ssize_t ehci_ed_test(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	__u32 test_mode = 0;
	struct usb_hcd *hcd = NULL;
	struct ehci_hcd *ehci = NULL;

	if (dev == NULL) {
		DMSG_ERR("ERR: Argment is invalid\n");
		return 0;
	}

	hcd = dev_get_drvdata(dev);
	if (hcd == NULL) {
		DMSG_ERR("ERR: hcd is null\n");
		return 0;
	}

	ehci = hcd_to_ehci(hcd);
	if (ehci == NULL) {
		DMSG_ERR("ERR: ehci is null\n");
		return 0;
	}

	mutex_lock(&dev->mutex);

	DMSG_INFO("ehci_ed_test:%s\n", buf);

	if (!strncmp(buf, "test_not_operating", 18)) {
		test_mode = EHCI_PORTSC_PTC_DIS;
		DMSG_INFO("test_mode:0x%x\n", test_mode);
	} else if (!strncmp(buf, "test_j_state", 12)) {
		test_mode = EHCI_PORTSC_PTC_J;
		DMSG_INFO("test_mode:0x%x\n", test_mode);
	} else if (!strncmp(buf, "test_k_state", 12)) {
		test_mode = EHCI_PORTSC_PTC_K;
		DMSG_INFO("test_mode:0x%x\n", test_mode);
	} else if (!strncmp(buf, "test_se0_nak", 12)) {
		test_mode = EHCI_PORTSC_PTC_SE0NAK;
		DMSG_INFO("test_mode:0x%x\n", test_mode);
	} else if (!strncmp(buf, "test_pack", 9)) {
		test_mode = EHCI_PORTSC_PTC_PACKET;
		DMSG_INFO("test_mode:0x%x\n", test_mode);
	} else if (!strncmp(buf, "test_force_enable", 17)) {
		test_mode = EHCI_PORTSC_PTC_FORCE;
		DMSG_INFO("test_mode:0x%x\n", test_mode);
	} else if (!strncmp(buf, "test_mask", 9)) {
		test_mode = EHCI_PORTSC_PTC_MASK;
		DMSG_INFO("test_mode:0x%x\n", test_mode);
	} else {
		DMSG_ERR("ERR: test_mode Argment is invalid\n");
		mutex_unlock(&dev->mutex);
		return count;
	}

	DMSG_INFO("regs: 0x%p\n", hcd->regs);
	__ehci_ed_test(ehci, hcd->regs, test_mode);

	mutex_unlock(&dev->mutex);

	return count;
}

static DEVICE_ATTR(ed_test, 0644, show_ed_test, ehci_ed_test);

static ssize_t show_phy_threshold(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;

	sunxi_ehci = dev->platform_data;

	return sprintf(buf, "threshold:0x%x\n",
			usb_phyx_tp_read(sunxi_ehci, 0x2a, 2));
}

static ssize_t ehci_phy_threshold(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;
	int val = 0;
	int err;

	err = kstrtoint(buf, 16, &val);
	if (err != 0)
		return -EINVAL;

	if (dev == NULL) {
		DMSG_ERR("ERR: Argment is invalid\n");
		return 0;
	}
	sunxi_ehci = dev->platform_data;

	if ((val >= 0) && (val <= 0x3)) {
		usb_phyx_tp_write(sunxi_ehci, 0x2a, val, 2);
	} else {
		DMSG_WARN("adjust disconnect threshold 0x%x is fail, value:0x0~0x3\n", val);
		return count;
	}

	DMSG_INFO("adjust succeed: threshold val:0x%x, no:%x\n",
			usb_phyx_tp_read(sunxi_ehci, 0x2a, 2),
			sunxi_ehci->usbc_no);

	return count;
}

static DEVICE_ATTR(phy_threshold, 0644, show_phy_threshold, ehci_phy_threshold);

static ssize_t show_phy_range(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;

	sunxi_ehci = dev->platform_data;
#if IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1)
	DMSG_INFO("PHY's rate and range:0x0~0x1fff\n");
	return sprintf(buf, "rate:0x%x\n",
		usb_new_phyx_read(sunxi_ehci));
#elif IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	DMSG_INFO("PHY's rate and range:0x0~0x3ff\n");
	return sprintf(buf, "rate:0x%x\n",
		usb_new_phyx_read(sunxi_ehci));
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	DMSG_INFO("PHY's rate and range:0x0~0xfff\n");
	return sprintf(buf, "rate:0x%x\n",
		usb_new_phyx_read(sunxi_ehci));
#elif IS_ENABLED(CONFIG_ARCH_SUN8IW17) | IS_ENABLED(CONFIG_ARCH_SUN8IW11)
	DMSG_INFO("PHY's rate and range:0x0~0x1f\n");
	return sprintf(buf, "rate:0x%x\n",
		usb_phyx_tp_read(sunxi_ehci, 0x20, 5));
#else
	DMSG_INFO("PHY's rate and range:0x0~0x3ff\n");
	return sprintf(buf, "rate:0x%x\n",
		usb_phyx_read(sunxi_ehci));
#endif
}

static ssize_t ehci_phy_range(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;
	int err;

#if IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) \
	|| IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	int val = 0, range = 0;

#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	range = 0x3ff;
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) \
	|| IS_ENABLED(CONFIG_ARCH_SUN251IW1)
	range = 0xfff;
#else
	range = 0x1fff;
#endif

	sunxi_ehci = dev->platform_data;

	err = kstrtoint(buf, 16, &val);
	if (err != 0)
		return -EINVAL;

	DMSG_INFO("adjust PHY's rate and range:0x0~0x%x\n", range);

	if ((val >= 0x0) && (val <= range)) {
		usb_new_phyx_write(sunxi_ehci, val);
	} else {
		DMSG_WARN("adjust PHY's paraments 0x%x is fail! value:0x0~0x%x\n", val, range);
		return count;
	}

	DMSG_INFO("adjust succeed:,PHY's paraments :0x%x, no:%d\n",
		usb_new_phyx_read(sunxi_ehci), sunxi_ehci->usbc_no);
#elif IS_ENABLED(CONFIG_ARCH_SUN8IW17) || IS_ENABLED(CONFIG_ARCH_SUN8IW11)
	int val = 0;

	sunxi_ehci = dev->platform_data;

	err = kstrtoint(buf, 16, &val);
	if (err != 0)
		return -EINVAL;

	DMSG_INFO("adjust PHY's rate and range:0x0~0x1f\n");
	if ((val >= 0) && (val <= 0x1f)) {
		usb_phyx_tp_write(sunxi_ehci, 0x20, val, 5);
	} else {
		DMSG_WARN("adjust PHY's rate and range 0x%x is fail, value:0x0~0x1f\n", val);
		return count;
	}

	DMSG_INFO("adjust succeed:,rate val:0x%x, no:%d\n",
			usb_phyx_tp_read(sunxi_ehci, 0x20, 5),
			sunxi_ehci->usbc_no);
#else
	int val = 0;

	sunxi_ehci = dev->platform_data;

	err = kstrtoint(buf, 16, &val);
	if (err != 0)
		return -EINVAL;

	DMSG_INFO("adjust PHY's rate and range:0x0~0x3ff\n");
	if ((val >= 0x0) && (val <= 0x3ff)) {
		usb_phyx_write(sunxi_ehci, val);
	} else {
		DMSG_WARN("adjust PHY's paraments 0x%x is fail! value:0x0~0x3ff\n", val);
		return count;
	}

	DMSG_INFO("adjust succeed:,PHY's paraments :0x%x, no:%d\n",
		usb_phyx_read(sunxi_ehci), sunxi_ehci->usbc_no);
#endif

	return count;
}

static DEVICE_ATTR(phy_range, 0644, show_phy_range, ehci_phy_range);

static ssize_t ehci_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;

	if (dev == NULL) {
		DMSG_ERR("ERR: Argment is invalid\n");
		return 0;
	}

	sunxi_ehci = dev->platform_data;
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: sw_ehci is null\n");
		return 0;
	}

	return sprintf(buf, "ehci:%d, probe:%u\n",
			sunxi_ehci->usbc_no, sunxi_ehci->probe);
}

static ssize_t ehci_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;
	int value = 0;
	int err;

	if (dev == NULL) {
		DMSG_ERR("ERR: Argment is invalid\n");
		return 0;
	}

	sunxi_ehci = dev->platform_data;
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: sw_ehci is null\n");
		return 0;
	}

	ehci_first_probe[sunxi_ehci->usbc_no] = 0;

	err = kstrtoint(buf, 10, &value);
	if (err != 0)
		return -EINVAL;
	if (value == 1) {
		ehci_enable[sunxi_ehci->usbc_no] = 0;
		sunxi_ehci->hsic_enable_flag = 1;
		sunxi_usb_enable_ehci(sunxi_ehci->usbc_no);

		if (sunxi_ehci->usbc_no == HCI0_USBC_NO)
			sunxi_set_host_vbus(sunxi_ehci, 1);

		if (sunxi_ehci->hsic_ctrl_flag)
			sunxi_set_host_hisc_rdy(sunxi_ehci, 1);
	} else if (value == 0) {
		if (sunxi_ehci->hsic_ctrl_flag)
			sunxi_set_host_hisc_rdy(sunxi_ehci, 0);

		ehci_enable[sunxi_ehci->usbc_no] = 1;
		sunxi_usb_disable_ehci(sunxi_ehci->usbc_no);
		sunxi_ehci->hsic_enable_flag = 0;
		ehci_enable[sunxi_ehci->usbc_no] = 0;
	} else {
		DMSG_INFO("unknown value (%d)\n", value);
	}

	return count;
}

static DEVICE_ATTR(ehci_enable, 0664, ehci_enable_show, ehci_enable_store);

static void sunxi_hcd_board_set_vbus(
		struct sunxi_hci_hcd *sunxi_ehci, int is_on)
{
	sunxi_ehci->set_power(sunxi_ehci, is_on);
}

static void sunxi_hcd_board_set_passby(struct sunxi_hci_hcd *sunxi_ehci,
						int is_on)
{
	sunxi_ehci->usb_passby(sunxi_ehci, is_on);
}

static int open_ehci_clock(struct sunxi_hci_hcd *sunxi_ehci)
{
	return sunxi_ehci->open_clock(sunxi_ehci, 0);
}

static int close_ehci_clock(struct sunxi_hci_hcd *sunxi_ehci)
{
	return sunxi_ehci->close_clock(sunxi_ehci, 0);
}

static void sunxi_start_ehci(struct sunxi_hci_hcd *sunxi_ehci)
{
	open_ehci_clock(sunxi_ehci);
	sunxi_hcd_board_set_passby(sunxi_ehci, 1);
	sunxi_hcd_board_set_vbus(sunxi_ehci, 1);
}

static void sunxi_stop_ehci(struct sunxi_hci_hcd *sunxi_ehci)
{
	sunxi_hcd_board_set_vbus(sunxi_ehci, 0);
	sunxi_hcd_board_set_passby(sunxi_ehci, 0);
	close_ehci_clock(sunxi_ehci);
}

static int sunxi_ehci_pm_notify(struct notifier_block *nb,
				unsigned long mode, void *_unused)
{
	switch (mode) {
	case PM_HIBERNATION_PREPARE:
	case PM_RESTORE_PREPARE:
	case PM_SUSPEND_PREPARE:
		atomic_set(&ehci_in_standby, 1);
		break;
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		atomic_set(&ehci_in_standby, 0);
		sunxi_hci_standby_completion(SUNXI_USB_EHCI);
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block sunxi_ehci_pm_nb = {
	.notifier_call = sunxi_ehci_pm_notify,
};

static struct hc_driver sunxi_ehci_hc_driver;
extern atomic_t ehci_enable_flag[4];

static int sunxi_insmod_ehci(struct platform_device *pdev)
{
	struct usb_hcd *hcd	= NULL;
	struct ehci_hcd *ehci	= NULL;
	struct sunxi_hci_hcd *sunxi_ehci = NULL;
	struct device *dev = NULL;
	int ret = 0;

	sunxi_ehci = pdev->dev.platform_data;
	if (!sunxi_ehci) {
		DMSG_ERR("ERR: sunxi_ehci is null\n");
		ret = -ENOMEM;
		goto ERR1;
	}

	sunxi_ehci->pdev = pdev;
	g_sunxi_ehci[sunxi_ehci->usbc_no] = sunxi_ehci;

	DMSG_INFO("[%s%d]: probe, pdev->name: %s, sunxi_ehci: 0x%px, 0x:%px, irq_no:%x\n",
		ehci_name, sunxi_ehci->usbc_no, pdev->name,
		sunxi_ehci, sunxi_ehci->usb_vbase, sunxi_ehci->irq_no);

	/* get io resource */
	sunxi_ehci->ehci_base		= sunxi_ehci->usb_vbase;
	sunxi_ehci->ehci_reg_length	= SUNXI_USB_EHCI_LEN;

	/* not init ehci, when driver probe */
	if (sunxi_ehci->usbc_no == HCI0_USBC_NO) {
		if (sunxi_ehci->port_type != USB_PORT_TYPE_HOST) {
			if (ehci_first_probe[sunxi_ehci->usbc_no]) {
				ehci_first_probe[sunxi_ehci->usbc_no] = 0;
				DMSG_INFO("[%s%d]: Not init ehci0\n",
					  ehci_name, sunxi_ehci->usbc_no);
				return 0;
			}
		}
	} else if (sunxi_ehci->usbc_no == HCI1_USBC_NO) {
		if (sunxi_ehci->extcon_supported) {
			if (ehci_first_probe[sunxi_ehci->usbc_no]) {
				ehci_first_probe[sunxi_ehci->usbc_no] = 0;
				DMSG_INFO("[%s%d]: Not init ehci1\n",
					  ehci_name, sunxi_ehci->usbc_no);
				return 0;
			}
		}
	}

	/* creat a usb_hcd for the ehci controller */
	hcd = usb_create_hcd(&sunxi_ehci_hc_driver, &pdev->dev, ehci_name);
	if (!hcd) {
		DMSG_ERR("ERR: usb_create_hcd failed\n");
		ret = -ENOMEM;
		goto ERR2;
	}

	hcd->rsrc_start = sunxi_ehci->usb_base_res->start;
	hcd->rsrc_len	= SUNXI_USB_EHCI_LEN;
	hcd->regs	= sunxi_ehci->ehci_base;
	sunxi_ehci->hcd = hcd;

	dev = &pdev->dev;
	sunxi_ehci->supply = regulator_get(dev, "drvvbus");

	if (IS_ERR(sunxi_ehci->supply)) {
		DMSG_WARN("%s()%d WARN: get supply failed\n", __func__, __LINE__);
		sunxi_ehci->supply = NULL;
	}

	sunxi_ehci->hci_regulator = regulator_get(dev, "hci");
	if (IS_ERR(sunxi_ehci->hci_regulator)) {
		DMSG_WARN("%s()%d WARN: get hci regulator failed\n", __func__, __LINE__);
		sunxi_ehci->hci_regulator = NULL;
	}
	/* echi start to work */
	sunxi_start_ehci(sunxi_ehci);

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs +
			HC_LENGTH(ehci, readl(&ehci->caps->hc_capbase));

	/* cache this readonly data, minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	ret = usb_add_hcd(hcd, sunxi_ehci->irq_no, IRQF_SHARED);
	if (ret != 0) {
		DMSG_ERR("ERR: usb_add_hcd failed\n");
		ret = -ENOMEM;
		goto ERR3;
	}

	device_wakeup_enable(hcd->self.controller);
	platform_set_drvdata(pdev, hcd);

	device_create_file(&pdev->dev, &dev_attr_ed_test);
	device_create_file(&pdev->dev, &dev_attr_phy_threshold);
	device_create_file(&pdev->dev, &dev_attr_phy_range);

#ifndef USB_SYNC_SUSPEND
	device_enable_async_suspend(&pdev->dev);
#endif

#if IS_ENABLED(CONFIG_PM)
	if (!sunxi_ehci->wakeup_suspend)
		INIT_WORK(&sunxi_ehci->resume_work, sunxi_ehci_resume_work);
#endif

	atomic_add(1, &ehci_enable_flag[sunxi_ehci->usbc_no]);

	sunxi_ehci->probe = 1;

	return 0;

ERR3:
	sunxi_stop_ehci(sunxi_ehci);
	usb_put_hcd(hcd);

ERR2:
	sunxi_ehci->hcd = NULL;
	g_sunxi_ehci[sunxi_ehci->usbc_no] = NULL;

ERR1:

	return ret;
}

static int sunxi_rmmod_ehci(struct platform_device *pdev)
{

	struct usb_hcd *hcd = NULL;
	struct sunxi_hci_hcd *sunxi_ehci = NULL;
	unsigned long time_left;

	if (pdev == NULL) {
		DMSG_ERR("ERR: Argment is invalid\n");
		return -1;
	}

	hcd = platform_get_drvdata(pdev);
	if (hcd == NULL) {
		DMSG_ERR("ERR: hcd is null\n");
		return -1;
	}

	sunxi_ehci = pdev->dev.platform_data;
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: sunxi_ehci is null\n");
		return -1;
	}

	if (atomic_read(&ehci_in_standby)) {
		reinit_completion(&sunxi_ehci->standby_complete);
		DMSG_INFO("INFO: sunxi_ehci disable, waiting until standby finish\n");
		time_left = wait_for_completion_timeout(&sunxi_ehci->standby_complete,
						msecs_to_jiffies(STANDBY_TIMEOUT));
		if (time_left)
			DMSG_INFO("INFO: sunxi_ehci disable time_left = %lu\n", time_left);
		else
			DMSG_ERR("ERR: sunxi_ehci waiting standby failed, go on disable\n");
		atomic_notifier_call_chain(&usb_pm_notifier_list, 0, NULL);

	}

	sunxi_ehci->probe = 0;

#ifndef USB_SYNC_SUSPEND
	device_disable_async_suspend(&pdev->dev);
#endif

	DMSG_INFO("[%s%d]: remove, pdev->name: %s, sunxi_ehci: 0x%px\n",
		ehci_name, sunxi_ehci->usbc_no, pdev->name, sunxi_ehci);

	device_remove_file(&pdev->dev, &dev_attr_ed_test);
	device_remove_file(&pdev->dev, &dev_attr_phy_threshold);
	device_remove_file(&pdev->dev, &dev_attr_phy_range);

	device_wakeup_disable(hcd->self.controller);

	usb_remove_hcd(hcd);

#if IS_ENABLED(CONFIG_PM)
	if (!sunxi_ehci->wakeup_suspend)
		if (!IS_ERR_OR_NULL(&sunxi_ehci->resume_work))
			cancel_work_sync(&sunxi_ehci->resume_work);
#endif

	sunxi_stop_ehci(sunxi_ehci);

	usb_put_hcd(hcd);

	if (sunxi_ehci->supply)
		regulator_put(sunxi_ehci->supply);

	if (sunxi_ehci->hci_regulator)
		regulator_put(sunxi_ehci->hci_regulator);

	atomic_sub(1, &ehci_enable_flag[sunxi_ehci->usbc_no]);

	sunxi_ehci->hcd = NULL;

	return 0;
}

static int sunxi_ehci_hcd_probe(struct platform_device *pdev)
{
	int ret = 0;

	struct sunxi_hci_hcd *sunxi_ehci = NULL;
	if (pdev == NULL) {
		DMSG_ERR("ERR: %s, Argment is invalid\n", __func__);
		return -1;
	}

	/* if usb is disabled, can not probe */
	if (usb_disabled()) {
		DMSG_ERR("ERR: usb hcd is disabled\n");
		return -ENODEV;
	}

	ret = init_sunxi_hci(pdev, SUNXI_USB_EHCI);
	if (ret != 0) {
		sunxi_err(&pdev->dev, "init_sunxi_hci is fail\n");
		return -1;
	}

	/* Initialize dma_mask and coherent_dma_mask to 32-bits */
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		sunxi_err(&pdev->dev, "set dma mask and coherent fail\n");
		return -1;
	}

	sunxi_insmod_ehci(pdev);

	sunxi_ehci = pdev->dev.platform_data;
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: %s, sunxi_ehci is null\n", __func__);
		return -1;
	}

	if (sunxi_ehci->usbc_no == HCI0_USBC_NO) {
		ret = register_pm_notifier(&sunxi_ehci_pm_nb);
		if (ret) {
			DMSG_ERR("ERR: %s, can not register suspend notifier\n", __func__);
			return -1;
		}
	}

	init_completion(&sunxi_ehci->standby_complete);

	if (ehci_enable[sunxi_ehci->usbc_no]) {
		device_create_file(&pdev->dev, &dev_attr_ehci_enable);
		ehci_enable[sunxi_ehci->usbc_no] = 0;
	}

	return 0;
}

static int sunxi_ehci_hcd_remove(struct platform_device *pdev)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;
	int ret = 0;

	if (pdev == NULL) {
		DMSG_ERR("ERR: %s, Argment is invalid\n", __func__);
		return -1;
	}

	sunxi_ehci = pdev->dev.platform_data;
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: %s, sunxi_ehci is null\n", __func__);
		return -1;
	}

	if (ehci_enable[sunxi_ehci->usbc_no] == 0) {
		device_remove_file(&pdev->dev, &dev_attr_ehci_enable);
		ehci_enable[sunxi_ehci->usbc_no] = 1;
	}

	if (sunxi_ehci->wakeup_source_flag && sunxi_ehci->wakeup_suspend) {
		dev_pm_clear_wake_irq(&pdev->dev);
	}

	if (sunxi_ehci->usbc_no == HCI0_USBC_NO)
		unregister_pm_notifier(&sunxi_ehci_pm_nb);

	if (sunxi_ehci->probe == 1) {
		ret = sunxi_rmmod_ehci(pdev);
		if (ret == 0)
			exit_sunxi_hci(sunxi_ehci);

		return ret;
	} else
		return 0;

}

static void sunxi_ehci_hcd_shutdown(struct platform_device *pdev)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;

	if (pdev == NULL) {
		DMSG_ERR("ERR: %s, Argment is invalid\n", __func__);
		return;
	}

	sunxi_ehci = pdev->dev.platform_data;
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: %s, is null\n", __func__);
		return;
	}

	if (sunxi_ehci->probe == 0) {
		DMSG_INFO("%s, %s is disable, need not shutdown\n",
			 __func__, sunxi_ehci->hci_name);
		return;
	}

	DMSG_INFO("[%s]: ehci shutdown start\n", sunxi_ehci->hci_name);
	usb_hcd_platform_shutdown(pdev);

	/**
	 * disable usb otg INTUSBE, to solve usb0 device mode
	 * catch audio udev on reboot system is fail.
	 */
	if (sunxi_ehci->usbc_no == 0) {
		if (sunxi_ehci->otg_vbase) {
			writel(0, (sunxi_ehci->otg_vbase
						+ SUNXI_USBC_REG_INTUSBE));
		}
	}
	sunxi_hcd_board_set_vbus(sunxi_ehci, 0);
	DMSG_INFO("[%s]: ehci shutdown end\n", sunxi_ehci->hci_name);
}

#if IS_ENABLED(CONFIG_PM)

static int sunxi_ehci_hcd_suspend(struct device *dev)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;
	struct usb_hcd *hcd = NULL;
	struct ehci_hcd *ehci = NULL;
	int val = 0;

	if (dev == NULL) {
		DMSG_ERR("ERR: %s, Argment is invalid\n", __func__);
		return 0;
	}

	hcd = dev_get_drvdata(dev);
	if (hcd == NULL) {
		DMSG_ERR("ERR: hcd is null\n");
		return 0;
	}

	sunxi_ehci = dev->platform_data;
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: sunxi_ehci is null\n");
		return 0;
	}

	if (sunxi_ehci->no_suspend) {
		DMSG_INFO("[%s]:ehci is being enable, stop system suspend\n",
			sunxi_ehci->hci_name);
		return -1;
	}

	if (sunxi_ehci->probe == 0) {
		DMSG_INFO("[%s]: is disable, can not suspend\n",
			sunxi_ehci->hci_name);
		return 0;
	}

	ehci = hcd_to_ehci(hcd);
	if (ehci == NULL) {
		DMSG_ERR("ERR: ehci is null\n");
		return 0;
	}

	if (sunxi_ehci->wakeup_suspend == USB_STANDBY) {
		DMSG_INFO("[%s] usb suspend\n", sunxi_ehci->hci_name);
		disable_irq(sunxi_ehci->irq_no);
		/* phy reg, offset:0x18 bit0 bit1 bit2, set 1 */
		val = ehci_readl(ehci, &ehci->regs->intr_enable);
		val |= (0x7 << 0);
		ehci_writel(ehci, val, &ehci->regs->intr_enable);

#if IS_ENABLED(SUNXI_USB_STANDBY_LOW_POW_MODE)
		/* phy reg, offset:0x10 bit4 bit5, set 0 */
		val = ehci_readl(ehci, &ehci->regs->command);
		val &= ~(0x30);
		ehci_writel(ehci, val, &ehci->regs->command);
		/* enable standby irq */
#endif
		enter_usb_standby(sunxi_ehci);

	} else if (sunxi_ehci->wakeup_suspend == NORMAL_STANDBY) {
		DMSG_INFO("[%s]: normal suspend\n", sunxi_ehci->hci_name);
		val = ehci_readl(ehci, &ehci->regs->intr_enable);
		val |= (0x7 << 0);
		ehci_writel(ehci, val, &ehci->regs->intr_enable);
	} else {
		DMSG_INFO("[%s]super suspend\n", sunxi_ehci->hci_name);

		ehci_suspend(hcd, device_may_wakeup(dev));
		cancel_work_sync(&sunxi_ehci->resume_work);
		sunxi_stop_ehci(sunxi_ehci);
	}

	return 0;
}

static void sunxi_ehci_resume_work(struct work_struct *work)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;

	sunxi_ehci = container_of(work, struct sunxi_hci_hcd, resume_work);

	sunxi_hcd_board_set_vbus(sunxi_ehci, 1);
}

static int sunxi_ehci_hcd_resume(struct device *dev)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;
	struct usb_hcd *hcd = NULL;
	struct ehci_hcd *ehci = NULL;
	int __maybe_unused val = 0;

	if (dev == NULL) {
		DMSG_ERR("ERR: Argment is invalid\n");
		return 0;
	}

	hcd = dev_get_drvdata(dev);
	if (hcd == NULL) {
		DMSG_ERR("ERR: hcd is null\n");
		return 0;
	}

	sunxi_ehci = dev->platform_data;
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: sunxi_ehci is null\n");
		return 0;
	}

	if (sunxi_ehci->probe == 0) {
		DMSG_INFO("[%s]: is disable, can not resume\n",
			sunxi_ehci->hci_name);
		return 0;
	}

	ehci = hcd_to_ehci(hcd);
	if (ehci == NULL) {
		DMSG_ERR("ERR: ehci is null\n");
		return 0;
	}

	if (sunxi_ehci->wakeup_suspend == USB_STANDBY) {
		DMSG_INFO("[%s]usb resume\n", sunxi_ehci->hci_name);

		exit_usb_standby(sunxi_ehci);
#if IS_ENABLED(SUNXI_USB_STANDBY_LOW_POW_MODE)
		val = ehci_readl(ehci, &ehci->regs->command);
		val |= 0x30;
		ehci_writel(ehci, val, &ehci->regs->command);

#endif
		enable_irq(sunxi_ehci->irq_no);
	} else if (sunxi_ehci->wakeup_suspend == NORMAL_STANDBY) {
		DMSG_INFO("[%s]:normal resume\n", sunxi_ehci->hci_name);
	} else {
		DMSG_INFO("[%s]super resume\n", sunxi_ehci->hci_name);

		open_ehci_clock(sunxi_ehci);
		sunxi_hcd_board_set_passby(sunxi_ehci, 1);
		ehci_resume(hcd, false);

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		if (sunxi_ehci->usbc_no == HCI0_USBC_NO) {
			/* phy reg, offset:0x10 bit3 set 0, enable siddq */
			val = USBC_Readl(sunxi_ehci->usb_common_phy_config
					 + SUNXI_HCI_PHY_CTRL);
			val &= ~(0x1 << SUNXI_HCI_PHY_CTRL_SIDDQ);
			USBC_Writel(val, sunxi_ehci->usb_common_phy_config
				    + SUNXI_HCI_PHY_CTRL);
		}
#endif

		schedule_work(&sunxi_ehci->resume_work);
	}

	return 0;
}

static const struct dev_pm_ops  aw_ehci_pmops = {
	.suspend	= sunxi_ehci_hcd_suspend,
	.resume		= sunxi_ehci_hcd_resume,
};

#define SUNXI_EHCI_PMOPS	(&aw_ehci_pmops)

#else

#define SUNXI_EHCI_PMOPS	NULL

#endif


static const struct of_device_id sunxi_ehci_match[] = {
	{.compatible = SUNXI_EHCI0_OF_MATCH, },
	{.compatible = SUNXI_EHCI1_OF_MATCH, },
	{.compatible = SUNXI_EHCI2_OF_MATCH, },
	{.compatible = SUNXI_EHCI3_OF_MATCH, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_ehci_match);

static struct platform_driver sunxi_ehci_hcd_driver = {
	.probe  = sunxi_ehci_hcd_probe,
	.remove	= sunxi_ehci_hcd_remove,
	.shutdown = sunxi_ehci_hcd_shutdown,
	.driver = {
			.name	= ehci_name,
			.pm	= SUNXI_EHCI_PMOPS,
			.of_match_table = sunxi_ehci_match,
		}
};

int sunxi_usb_disable_ehci(__u32 usbc_no)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;

	sunxi_ehci = g_sunxi_ehci[usbc_no];
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: sunxi_ehci is null\n");
		return -1;
	}

	if (sunxi_ehci->probe == 0) {
		DMSG_ERR("ERR: sunxi_ehci is disable, can not disable again\n");
		return -1;
	}

	sunxi_ehci->probe = 0;

	DMSG_INFO("[%s]: sunxi_usb_disable_ehci\n", sunxi_ehci->hci_name);

	sunxi_rmmod_ehci(sunxi_ehci->pdev);

	return 0;
}
EXPORT_SYMBOL(sunxi_usb_disable_ehci);

int sunxi_usb_enable_ehci(__u32 usbc_no)
{
	struct sunxi_hci_hcd *sunxi_ehci = NULL;
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	int val;
#endif

	sunxi_ehci = g_sunxi_ehci[usbc_no];
	if (sunxi_ehci == NULL) {
		DMSG_ERR("ERR: sunxi_ehci is null\n");
		return -1;
	}

	if (sunxi_ehci->probe == 1) {
		DMSG_ERR("ERR: sunxi_ehci is already enable, can not enable again\n");
		return -1;
	}

	sunxi_ehci->no_suspend = 1;

	DMSG_INFO("[%s]: sunxi_usb_enable_ehci\n", sunxi_ehci->hci_name);

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		if (sunxi_ehci->usbc_no == HCI0_USBC_NO) {
			/* phy reg, offset:0x10 bit3 set 0, enable siddq */
			val = USBC_Readl(sunxi_ehci->usb_common_phy_config
					 + SUNXI_HCI_PHY_CTRL);
			val &= ~(0x1 << SUNXI_HCI_PHY_CTRL_SIDDQ);
			USBC_Writel(val, sunxi_ehci->usb_common_phy_config
				    + SUNXI_HCI_PHY_CTRL);
		}
#endif

	sunxi_insmod_ehci(sunxi_ehci->pdev);

	sunxi_ehci->no_suspend = 0;

	return 0;
}
EXPORT_SYMBOL(sunxi_usb_enable_ehci);

static int __init sunxi_ehci_hcd_init(void)
{
	if (usb_disabled()) {
		DMSG_ERR(KERN_ERR "%s nousb\n", ehci_name);
		return -ENODEV;
	}

	DMSG_INFO("%s: " DRIVER_DESC "\n", ehci_name);
	ehci_init_driver(&sunxi_ehci_hc_driver, NULL);
	return platform_driver_register(&sunxi_ehci_hcd_driver);
}
late_initcall(sunxi_ehci_hcd_init);

static void __exit sunxi_ehci_hcd_cleanup(void)
{
	platform_driver_unregister(&sunxi_ehci_hcd_driver);
}
module_exit(sunxi_ehci_hcd_cleanup);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" SUNXI_EHCI_NAME);
MODULE_AUTHOR("javen");
MODULE_VERSION("1.1.5");

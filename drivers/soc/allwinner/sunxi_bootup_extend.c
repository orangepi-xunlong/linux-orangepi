/*
 * driver/soc/allwinner/sunxi_bootup_extend.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * allwinner sunxi platform bootup extend code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/hwspinlock.h>
#include <linux/reboot.h>
#include <linux/of_address.h>
#include <linux/bootup_extend.h>

static int bootup_extend_enable;

static ssize_t bootup_extend_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	ret = sprintf(buf, "%d\n", bootup_extend_enable);
	return ret;
}

static ssize_t bootup_extend_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long enable;
	enable = simple_strtoul(buf, NULL, 10);
	bootup_extend_enable = (enable > 0) ? 1 : 0;

	return count;
}

static DEVICE_ATTR(bootup_extend_enable, S_IRUGO | S_IWUGO,
	bootup_extend_enable_show,
		bootup_extend_enable_store);

static struct attribute *bootup_extend_attribute[] = {
	&dev_attr_bootup_extend_enable.attr,
	NULL,
};

static const struct attribute_group bootup_extend_attr_group = {
	.attrs = bootup_extend_attribute,
};

extern int sunxi_rtc_set_bootup_extend_mode(int mode);
void sunxi_bootup_extend_fix(unsigned int *cmd)
{
	if (bootup_extend_enable == 1) {
		if (*cmd == LINUX_REBOOT_CMD_POWER_OFF) {
			sunxi_rtc_set_bootup_extend_mode(SUNXI_BOOTUP_EXTEND_MODE_POWEROFF);
			*cmd = LINUX_REBOOT_CMD_RESTART;
			printk(KERN_INFO "will enter boot_start_os\n");
		} else if (*cmd == LINUX_REBOOT_CMD_RESTART ||
			*cmd == LINUX_REBOOT_CMD_RESTART2) {
			printk(KERN_INFO "not enter boot_start_os\n");
			sunxi_rtc_set_bootup_extend_mode(SUNXI_BOOTUP_EXTEND_MODE_RESTART);
		}
	}
}
EXPORT_SYMBOL(sunxi_bootup_extend_fix);

static const struct of_device_id sunxi_bootup_extend_of_match[] = {
	{.compatible = "allwinner,box_start_os"},
	{},
};

MODULE_DEVICE_TABLE(of, sunxi_bootup_extend_of_match);

static int sunxi_bootup_extend_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	if (!of_device_is_available(np)) {
		pr_err("%s: bootup extend is disable\n", __func__);
		goto exit;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &bootup_extend_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "Unable to create sysfs %s\n", __func__);
		goto exit;
	}
	bootup_extend_enable = 1;

	pr_err("%s: bootup extend state %d\n", __func__, bootup_extend_enable);

	pr_err("bootup extend probe ok\n");
	return 0;

exit:
	pr_err("%s exit\n", __func__);
	return ret;
}

static int sunxi_bootup_extend_remove(struct platform_device *pdev)
{
	bootup_extend_enable = 0;
	sysfs_remove_group(&(pdev->dev.kobj), &bootup_extend_attr_group);
	return 0;
}

static struct platform_driver sunxi_bootup_extend_driver = {
	.probe      = sunxi_bootup_extend_probe,
	.remove	    = sunxi_bootup_extend_remove,
	.driver     = {
		.name   = "bootup_extend",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(sunxi_bootup_extend_of_match),
	},
};

module_platform_driver(sunxi_bootup_extend_driver);

MODULE_AUTHOR(" <@> ");
MODULE_DESCRIPTION("sunxi bootup_extend");
MODULE_LICENSE("GPL");

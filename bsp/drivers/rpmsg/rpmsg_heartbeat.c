/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner rpmsg-heartbeat driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

/* #define DEBUG */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/workqueue.h>
#include <linux/remoteproc.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
#include <linux/cdev.h>
#endif

#define HEART_RATE		(16 * HZ)
#define TICK_INTERVAL		100
#define MODULE_NAME "rpmsg-heartbeat"

struct rpmsg_heartbeat_private_data {
	struct platform_device *pdev;
	struct device *dev;
	uint32_t tick;
};

static dev_t devid;
static struct cdev *rpmsg_heartbeat_cdev;
static struct class *rpmsg_heartbeat_class;
static struct device *rpmsg_heartbeat_dev;

static struct rpmsg_heartbeat_private_data *rpmsg_heartbeat;

extern int sunxi_rproc_report_crash(char *name, enum rproc_crash_type type);

struct rpmsg_heartbeat {
	struct rpmsg_device *rpdev;
	uint32_t tick;
	char master[32];
	u8 sleep;
};

struct hearbeat_packet {
	char name[32];
	uint32_t tick;
};

char rproc_name[32];  /* global rproc name to report crash */

static void time_out_handler(struct timer_list *unused)
{
	sunxi_rproc_report_crash(rproc_name, RPROC_WATCHDOG);
}

static DEFINE_TIMER(rpmsg_heartbeat_timer, time_out_handler);

static int rpmsg_heartbeat_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct rpmsg_heartbeat *chip = dev_get_drvdata(&rpdev->dev);
	struct hearbeat_packet *pack = data;

	dev_dbg(&rpdev->dev, "%s heartbeat: %d\n", pack->name, pack->tick);

	if (chip->master[0] == '\0') {
		memcpy(chip->master, pack->name, 32);
		chip->master[31] = '\0';
		memcpy(rproc_name, chip->master, 32);
	}

	if (chip->sleep) {
		dev_err(&rpdev->dev, "%s invalid heartbeat\n", pack->name);
		return 0;
	}

	if (chip->tick != pack->tick)
		chip->tick = pack->tick;

	chip->tick++;
	chip->tick %= TICK_INTERVAL;

	if (rpmsg_heartbeat)
		rpmsg_heartbeat->tick = chip->tick;

	mod_timer(&rpmsg_heartbeat_timer, jiffies + HEART_RATE);

	return 0;
}

static int rpmsg_heartbeat_probe(struct rpmsg_device *rpdev)
{
	struct rpmsg_heartbeat *chip;

	chip = devm_kzalloc(&rpdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->rpdev = rpdev;
	chip->tick = 0;

	dev_set_drvdata(&rpdev->dev, chip);
	/* wo need to announce the new ept to remote */
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	return 0;
}

static void rpmsg_heartbeat_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "%s is removed\n", dev_name(&rpdev->dev));
}

#ifdef CONFIG_PM_SLEEP
static int sunxi_rpmsg_hearbeat_suspend(struct device *dev)
{
	struct rpmsg_heartbeat *chip = NULL;
	chip = dev_get_drvdata(dev);

	chip->sleep = 1;
	del_timer_sync(&rpmsg_heartbeat_timer);

	return 0;
}

static void sunxi_rpmsg_hearbeat_resume(struct device *dev)
{
	struct rpmsg_heartbeat *chip = NULL;
	chip = dev_get_drvdata(dev);

	chip->sleep = 0;
	mod_timer(&rpmsg_heartbeat_timer, jiffies + HEART_RATE);
}

static struct dev_pm_ops sunxi_rpmsg_hearbeat_pm_ops = {
	.prepare = sunxi_rpmsg_hearbeat_suspend,
	.complete = sunxi_rpmsg_hearbeat_resume,
};
#endif

static ssize_t heartbeat_tick_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s heartbeat tick count %d\n", rproc_name, rpmsg_heartbeat->tick);
}

static DEVICE_ATTR(tick, 0664, heartbeat_tick_show, NULL);

static struct attribute *rpmsg_heartbeat_attributes[] = {
	&dev_attr_tick.attr,
	NULL
};

static struct attribute_group rpmsg_heartbeat_attribute_group = {
	.name = "heartbeat_tick",
	.attrs = rpmsg_heartbeat_attributes
};

static int rpmsg_heartbeat_platform_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&rpmsg_heartbeat_dev->kobj, &rpmsg_heartbeat_attribute_group);
	device_destroy(rpmsg_heartbeat_class, devid);

	class_destroy(rpmsg_heartbeat_class);
	cdev_del(rpmsg_heartbeat_cdev);
	dev_dbg(&pdev->dev, "rpmsg heartbeat module exit\n");

	return 0;
}

static int rpmsg_heartbeat_platform_probe(struct platform_device *pdev)
{
	int ret;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "of_node is missing\n");
		ret = -EINVAL;
		goto _err_out;
	}

	if (rpmsg_heartbeat) {
		kfree(rpmsg_heartbeat);
		rpmsg_heartbeat = NULL;
	}
	rpmsg_heartbeat = devm_kzalloc(
		&pdev->dev, sizeof(struct rpmsg_heartbeat_private_data), GFP_KERNEL);
	if (!rpmsg_heartbeat) {
		dev_err(&pdev->dev, "kzalloc for private data failed\n");
		ret = -ENOMEM;
		goto _err_out;
	}

	rpmsg_heartbeat->pdev = pdev;
	rpmsg_heartbeat->dev = &pdev->dev;
	platform_set_drvdata(pdev, rpmsg_heartbeat);

	/* Create and add a character device */
	alloc_chrdev_region(&devid, 0, 1, "rpmsg_heartbeat");/* corely for device number */
	rpmsg_heartbeat_cdev = cdev_alloc();
	cdev_init(rpmsg_heartbeat_cdev, NULL);
	rpmsg_heartbeat_cdev->owner = THIS_MODULE;

	ret = cdev_add(rpmsg_heartbeat_cdev, devid, 1);/* /proc/device/rpmsg_heartbeat */
	if (ret) {
		dev_err(&pdev->dev, "Error: rpmsg_heartbeat cdev_add fail.\n");
		return -1;
	}

	/* Create a path: sys/class/rpmsg_heartbeat */
	rpmsg_heartbeat_class = class_create(THIS_MODULE, "rpmsg_heartbeat");
	if (IS_ERR(rpmsg_heartbeat_class)) {
		dev_err(&pdev->dev, "Error:rpmsg_heartbeat class_create fail\n");
		return -1;
	}

	/* Create a path "sys/class/rpmsg_heartbeat/rpmsg_heartbeat_class" */
	rpmsg_heartbeat_dev = device_create(rpmsg_heartbeat_class, NULL, devid, NULL, "rpmsg_heartbeat");
	ret = sysfs_create_group(&rpmsg_heartbeat_dev->kobj, &rpmsg_heartbeat_attribute_group);

_err_out:
	dev_err(&pdev->dev, "rpmsg heartbeat module exit, ret %d!\n", ret);
	return ret;

}
static const struct of_device_id rpmsg_heartbeat_dt_ids[] = {
	{.compatible = "allwinner,sunxi-rpmsg-heartbeat"}, {},
};

static struct platform_driver rpmsg_heartbeat_driver = {
	.probe = rpmsg_heartbeat_platform_probe,
	.remove = rpmsg_heartbeat_platform_remove,
	.driver = {
		.name = MODULE_NAME,
		.pm = NULL,
		.of_match_table = rpmsg_heartbeat_dt_ids,
	},
};

static int __init rpmsg_heartbeat_module_init(void)
{
	int ret;

	ret = platform_driver_register(&rpmsg_heartbeat_driver);
	if (ret) {
		return ret;
	}

	return 0;
}

static void __exit rpmsg_heartbeat_module_exit(void)
{
	kfree(rpmsg_heartbeat);

	platform_driver_unregister(&rpmsg_heartbeat_driver);
}

module_init(rpmsg_heartbeat_module_init);
module_exit(rpmsg_heartbeat_module_exit);

static struct rpmsg_device_id rpmsg_driver_hearbeat_id_table[] = {
	{ .name	= "sunxi,rpmsg_heartbeat" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_hearbeat_id_table);

static struct rpmsg_driver rpmsg_sample_client = {
	.drv = {
		.name	= KBUILD_MODNAME,
#ifdef CONFIG_PM_SLEEP
		.pm = &sunxi_rpmsg_hearbeat_pm_ops,
#endif
	},
	.id_table	= rpmsg_driver_hearbeat_id_table,
	.probe		= rpmsg_heartbeat_probe,
	.callback	= rpmsg_heartbeat_cb,
	.remove		= rpmsg_heartbeat_remove,
};
module_rpmsg_driver(rpmsg_sample_client);

MODULE_DESCRIPTION("Remote Processor Heartbeat Receive Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lijiajian <lijiajian@allwinnertech.com>");
MODULE_VERSION("1.0.1");

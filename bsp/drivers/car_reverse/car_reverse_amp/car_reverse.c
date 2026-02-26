/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse driver module
 *
 * Copyright (C) 2015-2023 AllwinnerTech, Inc.
 *
 * Authors:  wujiayi <wujiayi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <sunxi-log.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/rpmsg.h>
#include <linux/workqueue.h>
#include <linux/remoteproc.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/reboot.h>
#include "car_reverse.h"

#define MODULE_NAME "car-reverse"
#define CAR_REVERSE_PACKET_MAGIC		0xA5A51234
#define SUSPEND_TIMEOUT					msecs_to_jiffies(1000)

static dev_t devid;
static struct cdev *car_reverse_cdev;
static struct class *car_reverse_class;
static struct device *cardev;

u32 reverse_loglevel_debug;

struct car_reverse_private_data {
	struct platform_device *pdev;
	struct device *dev;
	struct rpmsg_endpoint *ept;
	struct rpmsg_device *rpdev;

	/* arm car reverse */
	int arm_car_reverse_status;
	int reverse_gpio;
	int car_reverse_ctrl;

	struct mutex lock;
	int status;
	int standby;

	struct completion complete_ack;
};

static struct car_reverse_private_data *car_reverse;

static int amp_car_reverse_rpmsg_send(struct car_reverse_amp_packet *pack);

static int car_reverse_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int car_reverse_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t car_reverse_read(struct file *file, char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static ssize_t car_reverse_write(struct file *file, const char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static int car_reverse_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}

static long car_reverse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long arg64;
	struct car_reverse_amp_packet pack;
	void __user *argp = (void __user *)arg;
	int ret = -1;

	switch (cmd) {
	case CMD_CAR_REVERSE_GET_CTRL_TO_AMP:
		//if (car_reverse->arm_car_reverse_status == CAR_REVERSE_START)
		//	return 0;
		pack.type = ARM_TRY_GET_CAR_REVERSE_CRTL;
		pack.arm_car_reverse_status = ARM_TRY_GET_CRTL;
		ret = amp_car_reverse_rpmsg_send(&pack);
		if (ret < 0)
			arg64 = 0;
		else
			arg64 = 1;

		ret = copy_to_user(argp, (void *)&arg64, sizeof(unsigned long));
		if (ret < 0) {
			sunxi_err(NULL, "copy_to_user err\n");
			return -EINVAL;
		}
		break;
	case CMD_CAR_REVERSE_CHECK_STATE:
		arg64 = car_reverse->arm_car_reverse_status;
		ret = copy_to_user(argp, (void *)&arg64, sizeof(unsigned long));
		if (ret < 0) {
			CAR_REVERSE_ERR("copy_to_user err\n");
			return -EINVAL;
		}
		break;
	default:
		CAR_REVERSE_ERR("ERROR cmd:%d!\n", cmd);
		return -EINVAL;
	}
	return 0;
}

//#ifdef CONFIG_COMPAT
//static long car_reverse_compat_ioctl(struct file *filp, unsigned int cmd,
//						unsigned long arg)
//{
//	return car_reverse_ioctl(filp, cmd, (unsigned long)arg);
//}
//#endif

static const struct file_operations car_reverse_fops = {
	.owner		= THIS_MODULE,
	.open		= car_reverse_open,
	.release	= car_reverse_release,
	.write		= car_reverse_write,
	.read		= car_reverse_read,
	.unlocked_ioctl	= car_reverse_ioctl,
//#ifdef CONFIG_COMPAT
//	.compat_ioctl	= car_reverse_compat_ioctl,
//#endif
	.mmap		= car_reverse_mmap,
};

int car_reverse_preview_start(void)
{
	return 0;
}

static int car_reverse_preview_stop(void)
{
	struct car_reverse_amp_packet pack;

	car_reverse->arm_car_reverse_status = CAR_REVERSE_STOP;

	pack.type = ARM_TRY_GET_CAR_REVERSE_CRTL;
	pack.arm_car_reverse_status = car_reverse->arm_car_reverse_status;
	amp_car_reverse_rpmsg_send(&pack);

	return 0;
}

static ssize_t force_start_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	switch (car_reverse->arm_car_reverse_status) {
	case CAR_REVERSE_STOP:
		return sprintf(buf, "arm_car_reverse_status = stop\n");
	case CAR_REVERSE_START:
		return sprintf(buf, "arm_car_reverse_status = start\n");
	case ARM_TRY_GET_CRTL:
		return sprintf(buf, "arm_car_reverse_status = try get car reverse  try\n");
	case ARM_GET_CRTL_OK:
		return sprintf(buf, "arm_car_reverse_status = get car reverse crtl ok \n");
	case ARM_GET_CRTL_FAIL:
		return sprintf(buf, "arm_car_reverse_status = get car reverse crtl fail\n");
	default:
		return sprintf(buf, "arm_car_reverse_status = null\n");
	}
}

static ssize_t force_start_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct car_reverse_amp_packet pack;
	if (!strncmp(buf, "stop", 4)) {
		if (car_reverse->arm_car_reverse_status == CAR_REVERSE_STOP)
			return count;
		car_reverse->arm_car_reverse_status = CAR_REVERSE_STOP;
	} else if (!strncmp(buf, "start", 5)) {
		if (car_reverse->arm_car_reverse_status == CAR_REVERSE_START)
			return count;
		car_reverse->arm_car_reverse_status = CAR_REVERSE_START;
	} else if (!strncmp(buf, "null", 4)) {
		car_reverse->arm_car_reverse_status = CAR_REVERSE_NULL;
	} else {
		CAR_REVERSE_ERR("Invalid param!\n");
		return count;
	}

	pack.type = ARM_TRY_GET_CAR_REVERSE_CRTL;
	pack.arm_car_reverse_status = car_reverse->arm_car_reverse_status;
	amp_car_reverse_rpmsg_send(&pack);

	return count;
}

static DEVICE_ATTR(force_start, 0660, force_start_show, force_start_store);

static struct attribute *car_reverse_attributes[] = {
	&dev_attr_force_start.attr,
	NULL
};

static struct attribute_group car_reverse_attribute_group = {
	.name = "debug",
	.attrs = car_reverse_attributes
};

static void of_get_gpio_by_name(struct platform_device *pdev, const char *name,
				int *ret)
{
	int gpio_index;
	enum of_gpio_flags config;

	gpio_index = of_get_named_gpio_flags(pdev->dev.of_node, name, 0, &config);
	if (!gpio_is_valid(gpio_index)) {
		sunxi_err(&pdev->dev, "failed to get gpio '%s'\n", name);
		*ret = 0;
		return;
	}
	*ret = gpio_index;
}

static void parse_config(struct platform_device *pdev,
			 struct car_reverse_private_data *priv)
{

	of_get_gpio_by_name(pdev, "reverse_int_pin", &priv->reverse_gpio);
}

#if IS_ENABLED(CONFIG_SUNXI_CAR_REVERSE_GPIO_DETECT)
static irqreturn_t reverse_irq_handle(int irqnum, void *data)
{
	int value;

	if (car_reverse->car_reverse_ctrl == ARM_CTRL) {
		value = gpio_get_value(car_reverse->reverse_gpio);
		if (value == 0) {
			car_reverse->arm_car_reverse_status = CAR_REVERSE_STOP;
		} else {
			car_reverse->arm_car_reverse_status = CAR_REVERSE_START;
		}
		CAR_REVERSE_ERR("reverse_irq_handle enter, status = %d\n ", value);
	}
	return IRQ_HANDLED;
}
#endif

static int amp_car_reverse_rpmsg_send(struct car_reverse_amp_packet *pack)
{
	//struct car_reverse_amp_packet packet;
	int ret = 0;
	//
	//mutex_lock(&car_reverse->lock);
	if (!car_reverse->ept)
		ret = -ENODEV;
	//mutex_unlock(&car_reverse->lock);
	if (ret)
		return ret;
	pack->magic = CAR_REVERSE_PACKET_MAGIC;
	ret = rpmsg_send(car_reverse->ept, pack, sizeof(*pack));

	return ret;
}

static int rpmsg_car_reverse_probe(struct rpmsg_device *rpdev)
{
	//mutex_lock(&car_reverse->lock);
	car_reverse->ept = rpdev->ept;
	car_reverse->rpdev = rpdev;
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	//mutex_unlock(&car_reverse->lock);
	return 0;
}

static int rpmsg_car_reverse_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct car_reverse_amp_packet *pack = data;

	if (pack->magic != CAR_REVERSE_PACKET_MAGIC || len != sizeof(*pack)) {
		sunxi_err(&rpdev->dev, "packet invalid magic or size %d %d %x\n",
				len, (int)sizeof(*pack), pack->magic);
		return 0;
	}

	sunxi_info(&rpdev->dev, "receive pack->type %d\n", pack->type);

	if (pack->type != ARM_TRY_GET_CAR_REVERSE_CRTL) {
		return 0;
	}

	switch (pack->arm_car_reverse_status) {
	case ARM_GET_CRTL_OK:
		car_reverse->arm_car_reverse_status = pack->arm_car_reverse_status;
		car_reverse->car_reverse_ctrl = ARM_CTRL;

		sunxi_info(&rpdev->dev, "arm_car_reverse_status %d\n", car_reverse->arm_car_reverse_status);
		break;
	case CAR_REVERSE_STOP:
		car_reverse->arm_car_reverse_status = pack->arm_car_reverse_status;
		car_reverse->car_reverse_ctrl = ARM_CTRL;

		complete(&car_reverse->complete_ack);
		break;
	case CAR_REVERSE_START:
		car_reverse->arm_car_reverse_status = pack->arm_car_reverse_status;
		car_reverse->car_reverse_ctrl = RV_CTRL;

		break;
	default:
		break;
	}
	return 0;
}

static void rpmsg_car_reverse_remove(struct rpmsg_device *rpdev)
{
	sunxi_info(&rpdev->dev, "%s is removed\n", dev_name(&rpdev->dev));
	//mutex_lock(&preview->lock);
	//preview->ept = NULL;
	//mutex_unlock(&preview->lock);
}

static struct rpmsg_device_id rpmsg_driver_car_reverse_id_table[] = {
	{ .name	= "sunxi,rpmsg_car_reverse" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_car_reverse_id_table);

static struct rpmsg_driver rpmsg_car_reverse_client = {
	.drv = {
		.name	= KBUILD_MODNAME,
	},
	.id_table	= rpmsg_driver_car_reverse_id_table,
	.probe		= rpmsg_car_reverse_probe,
	.callback	= rpmsg_car_reverse_cb,
	.remove		= rpmsg_car_reverse_remove,
};

void rpmsg_car_reverse_init(void)
{
	register_rpmsg_driver(&rpmsg_car_reverse_client);
}

void rpmsg_car_reverse_exit(void)
{
	unregister_rpmsg_driver(&rpmsg_car_reverse_client);
}

static int car_reverse_probe(struct platform_device *pdev)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_SUNXI_CAR_REVERSE_GPIO_DETECT)
	long reverse_pin_irqnum;
#endif

	sunxi_info(&pdev->dev, "%s->%d\n", __func__, __LINE__);
	if (!pdev->dev.of_node) {
		sunxi_err(&pdev->dev, "of_node is missing\n");
		ret = -EINVAL;
		goto _err_out;
	}
	sunxi_info(&pdev->dev, "%s->%d\n", __func__, __LINE__);

	if (car_reverse) {
		kfree(car_reverse);
		car_reverse = NULL;
	}
	car_reverse = devm_kzalloc(
		&pdev->dev, sizeof(struct car_reverse_private_data), GFP_KERNEL);
	if (!car_reverse) {
		sunxi_err(&pdev->dev, "kzalloc for private data failed\n");
		ret = -ENOMEM;
		goto _err_out;
	}
	sunxi_info(&pdev->dev, "%s->%d\n", __func__, __LINE__);

	parse_config(pdev, car_reverse);
	car_reverse->pdev = pdev;
	car_reverse->dev = &pdev->dev;
	platform_set_drvdata(pdev, car_reverse);

	mutex_init(&car_reverse->lock);

	/* Create and add a character device */
	alloc_chrdev_region(&devid, 0, 1, "car_reverse");/* corely for device number */
	car_reverse_cdev = cdev_alloc();
	cdev_init(car_reverse_cdev, &car_reverse_fops);
	car_reverse_cdev->owner = THIS_MODULE;

	ret = cdev_add(car_reverse_cdev, devid, 1);/* /proc/device/car_reverse */
	if (ret) {
		CAR_REVERSE_ERR("Error: car_reverse cdev_add fail.\n");
		return -1;
	}

	/* Create a path: sys/class/car_reverse */
	car_reverse_class = class_create(THIS_MODULE, "car_reverse");
	if (IS_ERR(car_reverse_class)) {
		CAR_REVERSE_ERR("Error:car_reverse class_create fail\n");
		return -1;
	}

	/* Create a path "sys/class/car_reverse/car_reverse" */
	cardev = device_create(car_reverse_class, NULL, devid, NULL, "car_reverse");
	ret = sysfs_create_group(&cardev->kobj, &car_reverse_attribute_group);

	car_reverse->arm_car_reverse_status = CAR_REVERSE_NULL;
	car_reverse->car_reverse_ctrl = RV_CTRL;

#if IS_ENABLED(CONFIG_SUNXI_CAR_REVERSE_GPIO_DETECT)
	reverse_pin_irqnum = gpio_to_irq(car_reverse->reverse_gpio);
	if (IS_ERR_VALUE(reverse_pin_irqnum)) {
		sunxi_err(&pdev->dev,
			"map gpio [%d] to virq failed, errno = %ld\n",
			car_reverse->reverse_gpio, reverse_pin_irqnum);
		ret = -EINVAL;
		goto _err_out;
	}
	sunxi_info(&pdev->dev, "map gpio [%d] to virq ok \n", car_reverse->reverse_gpio);
	if (devm_request_irq(car_reverse->dev, reverse_pin_irqnum, reverse_irq_handle,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"car-reverse", pdev)) {
		sunxi_err(&pdev->dev, "request irq %ld failed\n",
			reverse_pin_irqnum);
		ret = -EBUSY;
		goto _err_out;
	}
#endif

	rpmsg_car_reverse_init();

	init_completion(&car_reverse->complete_ack);

	sunxi_info(&pdev->dev, "car reverse module probe ok\n");
	return 0;

_err_out:
	sunxi_err(&pdev->dev, "car reverse module exit, errno %d!\n", ret);
	return ret;
}

static int car_reverse_remove(struct platform_device *pdev)
{
	rpmsg_car_reverse_exit();
	sysfs_remove_group(&cardev->kobj, &car_reverse_attribute_group);
	device_destroy(car_reverse_class, devid);

	reinit_completion(&car_reverse->complete_ack);

	class_destroy(car_reverse_class);
	cdev_del(car_reverse_cdev);
	sunxi_info(&pdev->dev, "car reverse module exit\n");
	return 0;
}

static int car_reverse_pm_suspend(struct device *dev)
{
	return 0;
}

static int car_reverse_pm_resume(struct device *dev)
{
	return 0;
}

static int car_reverse_pm_prepare(struct device *dev)
{
	int ret;

	if (CAR_REVERSE_START == car_reverse->arm_car_reverse_status) {
		car_reverse_preview_stop();

		ret = wait_for_completion_timeout(&car_reverse->complete_ack, SUSPEND_TIMEOUT);
		if (!ret) {
			CAR_REVERSE_ERR("car_reverse timeout return suspend ack\n");
			return -EBUSY;
		}
	}
	return 0;
}

static void car_reverse_pm_complete(struct device *dev)
{

}


static const struct dev_pm_ops car_reverse_pm_ops = {
	.prepare = car_reverse_pm_prepare,
	.suspend = car_reverse_pm_suspend,
	.resume = car_reverse_pm_resume,
	.complete = car_reverse_pm_complete,
};

static const struct of_device_id car_reverse_dt_ids[] = {
	{.compatible = "allwinner,sunxi-car-reverse-amp"}, {},
};

static struct platform_driver car_reverse_driver = {
	.probe = car_reverse_probe,
	.remove = car_reverse_remove,
	.driver = {
		.name = MODULE_NAME,
		.pm = &car_reverse_pm_ops,
		.of_match_table = car_reverse_dt_ids,
	},
};

static int car_reverse_reboot(struct notifier_block *self, unsigned long event, void *data)
{
	struct car_reverse_amp_packet pack;

	pack.type = ARM_TRY_GET_CAR_REVERSE_CRTL;
	pack.arm_car_reverse_status = CAR_REVERSE_STOP;

	amp_car_reverse_rpmsg_send(&pack);

	return NOTIFY_OK;
}

static struct notifier_block car_reverse_reboot_notifier = {
	.notifier_call = car_reverse_reboot,
};

static int __init car_reverse_module_init(void)
{
	int ret;

	ret = platform_driver_register(&car_reverse_driver);
	if (ret) {
		CAR_REVERSE_ERR("platform driver register failed\n");
		return ret;
	}

	ret = register_reboot_notifier(&car_reverse_reboot_notifier);
	if (ret) {
		CAR_REVERSE_ERR("platform driver register reboot notifier fail\n");
		return ret;
	}

	return 0;
}

static void __exit car_reverse_module_exit(void)
{
	kfree(car_reverse);

	platform_driver_unregister(&car_reverse_driver);

	unregister_reboot_notifier(&car_reverse_reboot_notifier);
}

module_init(car_reverse_module_init);
module_exit(car_reverse_module_exit);

MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(ANDROID_GKI_VFS_EXPORT_ONLY);
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
MODULE_AUTHOR("<wujiayi@allwinnertech.com>");
MODULE_DESCRIPTION("Sunxi fast car reverse image preview for amp");
MODULE_VERSION("1.1.0");

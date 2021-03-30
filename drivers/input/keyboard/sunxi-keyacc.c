/*
 * Copyright (c) 2013-2015 liaoyongming@allwinnertech.com
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *
 * ChangeLog
 * status  :       power off    working             suspend                  working
 *                            ___________                              ______________________
 * acc     :      ___________|           |____________________________| need to wake up when rising, by power on
 *                ___________             ____________________________
 * acc_irq :                 |___________|need to suspend when rising |______________________
 *
 * gsensor :                                  |wakeup by gsensor,recording
 *
 * 3G      :                                  |wakeup by 3G, recording
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/sys_config.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/power/scenelock.h>
#include <linux/power/axp_depend.h>
#include <linux/pm.h>

#define ACC_KEY_CODE              KEY_HOME

struct sunxi_acc_device {
	u32 irq_hd;
	struct gpio_config pio;
	struct input_dev *idev;
};

static struct sunxi_acc_device sunxi_acc_dev;
static int acc_irq_status;
static int acc_irq_wakeup_enable;

static ssize_t sunxi_acc_irq_wakeup_enable_show(struct class *class, struct class_attribute *attr, char *buf)
{
	int ret = 0;
	ret = sprintf(buf, "%d\n", acc_irq_wakeup_enable);
	return ret;
}

static ssize_t sunxi_acc_irq_wakeup_enable_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	int enable;
	if (buf == NULL)
		return -1;
	enable = simple_strtol(buf, NULL, 0);
	acc_irq_wakeup_enable = (enable > 0) ? 1 : 0;

	if (acc_irq_wakeup_enable)
		enable_wakeup_src(CPUS_GPIO_SRC, sunxi_acc_dev.pio.gpio);
	else
		disable_wakeup_src(CPUS_GPIO_SRC, sunxi_acc_dev.pio.gpio);

	return count;
}

static ssize_t sunxi_acc_irq_status_show(struct class *dev, struct class_attribute *attr, char *buf)
{
	int ret = 0;
	ret = sprintf(buf, "%d\n", acc_irq_status);
	acc_irq_status = 0;
	return ret;
}

static ssize_t sunxi_acc_io_status_show(struct class *dev, struct class_attribute *attr, char *buf)
{
	int ret = 0;
	ret = sprintf(buf, "%d\n", gpio_get_value(sunxi_acc_dev.pio.gpio));
	return ret;
}

static struct class_attribute acc_status_attrs[] = {
	__ATTR(io_status, S_IRUGO, sunxi_acc_io_status_show, NULL),
	__ATTR(irq_status, S_IRUGO, sunxi_acc_irq_status_show, NULL),
	__ATTR(irq_wakeup_enable, S_IRUGO | S_IWUGO, sunxi_acc_irq_wakeup_enable_show, sunxi_acc_irq_wakeup_enable_store),
	__ATTR_NULL
};

static struct class sunxi_acc_status_attr_group = {
	.name = "gpio_acc",
	.class_attrs = acc_status_attrs,
};

static int sunxi_acc_create_input_device(struct sunxi_acc_device *acc_dev)
{
	acc_dev->idev = input_allocate_device();
	if (!acc_dev->idev) {
		pr_err("[sunxi acc]: not enought memory for input device\n");
		return -ENOMEM;
	}

	acc_dev->idev->name         = "acc_dec";
	acc_dev->idev->phys         = "acc_dec/input0";
	acc_dev->idev->id.bustype   = BUS_HOST;
	acc_dev->idev->id.vendor    = 0x0001;
	acc_dev->idev->id.product   = 0x0001;
	acc_dev->idev->id.version   = 0x0100;

#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	acc_dev->idev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	printk(KERN_DEBUG "REPORT_REPEAT_KEY_BY_INPUT_CORE is defined, support report repeat key value. \n");
#else
	acc_dev->idev->evbit[0] = BIT_MASK(EV_KEY);
#endif
	set_bit(ACC_KEY_CODE, acc_dev->idev->keybit);

	if (input_register_device(acc_dev->idev)) {
		pr_err("[sunxi acc]: input register device failed\n");
		input_free_device(acc_dev->idev);
		return -ENOMEM;
	}

	return 0;
}

static int sunxi_acc_free_input_device(struct sunxi_acc_device *acc_dev)
{
	if (acc_dev->idev) {
		input_unregister_device(acc_dev->idev);
	}

	return 0;
}

static int sunxi_acc_irq_onoff(int onoff)
{
	if (onoff) {
		pr_info("acc irq enable\n");
		if (sunxi_acc_dev.pio.gpio < 1024)
			enable_irq(sunxi_acc_dev.irq_hd);
		else
			axp_gpio_irq_enable(0, sunxi_acc_dev.pio.gpio);
	} else {
		pr_info("acc irq disable\n");
		if (sunxi_acc_dev.pio.gpio < 1024)
			disable_irq_nosync(sunxi_acc_dev.irq_hd);
		else
			axp_gpio_irq_disable(0, sunxi_acc_dev.pio.gpio);
	}

	return 0;
}

static int sunxi_acc_gpio_free(void)
{
	if (sunxi_acc_dev.pio.gpio < 1024)
		free_irq(sunxi_acc_dev.irq_hd, NULL);
	else
		axp_gpio_irq_free(0, sunxi_acc_dev.pio.gpio);

	return 0;
}

static u32 sunxi_acc_irq_func(int irq, void *para)
{
	pr_info("sunxi acc irq happened\n");
	acc_irq_status = 1;
	input_report_key(sunxi_acc_dev.idev, ACC_KEY_CODE, 1);
	input_sync(sunxi_acc_dev.idev);
	mdelay(100);
	input_report_key(sunxi_acc_dev.idev, ACC_KEY_CODE, 0);
	input_sync(sunxi_acc_dev.idev);
	return 0;
}

static int sunxi_acc_gpio_request(struct platform_device *pdev)
{
	int ret = -1;

	struct device_node *np = pdev->dev.of_node;

	sunxi_acc_dev.pio.gpio = of_get_named_gpio_flags(np, "acc_int", 0, (enum of_gpio_flags *)(&(sunxi_acc_dev.pio)));
	if (!gpio_is_valid(sunxi_acc_dev.pio.gpio)) {
		pr_err("[sunxi acc]: gpio key is invalied\n");
		ret = -EPERM;
		goto exit;
	}

	if (sunxi_acc_dev.pio.gpio < 1024) {
		pr_info("request cpu gpio irq\n");
		sunxi_acc_dev.irq_hd = gpio_to_irq(sunxi_acc_dev.pio.gpio);
		ret = request_irq(sunxi_acc_dev.irq_hd, sunxi_acc_irq_func, IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING, "sunxi acc", NULL);
		if (IS_ERR_VALUE(ret)) {
			pr_err("sunxi acc cpux gpio request irq failed\n");
			goto exit;
		}
	} else {
		pr_info("request axp gpio irq\n");
		ret = axp_gpio_irq_request(0, sunxi_acc_dev.pio.gpio, sunxi_acc_irq_func, NULL);
		if (IS_ERR_VALUE(ret)) {
			pr_err("sunxi acc axp gpio request irq failed\n");
			goto exit;
		}
		axp_gpio_irq_set_type(0, sunxi_acc_dev.pio.gpio, AXP_GPIO_IRQF_TRIGGER_RISING);
		axp_gpio_irq_enable(0, sunxi_acc_dev.pio.gpio);
	}

exit:
	printk("%s\n", __func__);
	return ret;
}

int sunxi_acc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = -1;

	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi acc is diable\n", __func__);
		ret = -EPERM;
		goto exit;
	}

	memset(&sunxi_acc_dev, 0, sizeof(struct sunxi_acc_device));

	ret = sunxi_acc_create_input_device(&sunxi_acc_dev);
	if (ret) {
		pr_err("[sunxi acc]: create input device err\n");
		goto exit;
	}

	ret = sunxi_acc_gpio_request(pdev);
	if (ret) {
		pr_err("[sunxi acc]: request irq failed\n");
		goto exit_dev;
	}

	ret = class_register(&sunxi_acc_status_attr_group);
	if (ret) {
		pr_err("create device file failed\n");
		ret = -EINVAL;
		goto exit_irq;
	}
	pr_err("[sunxi acc] %s ok\n", __func__);
	return 0;

exit_irq:
	sunxi_acc_gpio_free();
exit_dev:
	sunxi_acc_free_input_device(&sunxi_acc_dev);
exit:
	printk("%s exit\n", __func__);
	return ret;
}

static int sunxi_acc_remove(struct platform_device *pdev)
{
	sunxi_acc_gpio_free();
	class_unregister(&sunxi_acc_status_attr_group);
	sunxi_acc_free_input_device(&sunxi_acc_dev);
	return 0;
}

static void sunxi_acc_shutdown(struct platform_device *pdev)
{
	return;
}

static int sunxi_acc_suspend(struct device *dev)
{
	if (!acc_irq_wakeup_enable)
		sunxi_acc_irq_onoff(0);
	return 0;
}

static int sunxi_acc_resume(struct device *dev)
{
	if (!acc_irq_wakeup_enable)
		sunxi_acc_irq_onoff(1);
	return 0;
}

static const struct dev_pm_ops sunxi_acc_pm_ops = {
	.suspend = sunxi_acc_suspend,
	.resume = sunxi_acc_resume,
};

static const struct of_device_id sunxi_acc_of_match[] = {
	{.compatible = "allwinner,sunxi-acc-det"},
	{},
};

MODULE_DEVICE_TABLE(of, sunxi_acc_of_match);

static struct platform_driver sunxi_acc_device_driver = {
	.probe      = sunxi_acc_probe,
	.remove	    = sunxi_acc_remove,
	.shutdown   = sunxi_acc_shutdown,
	.driver     = {
		.name   = "sunxi-acc-det",
		.owner  = THIS_MODULE,
		.pm     = &sunxi_acc_pm_ops,
		.of_match_table = of_match_ptr(sunxi_acc_of_match),
	},
};

static int __init sunxi_acc_init(void)
{
	return platform_driver_register(&sunxi_acc_device_driver);
}

static void __exit sunxi_acc_exit(void)
{
	platform_driver_unregister(&sunxi_acc_device_driver);
}

module_init(sunxi_acc_init);
module_exit(sunxi_acc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(" <@> ");
MODULE_DESCRIPTION("Keyboard driver for GPIOs");

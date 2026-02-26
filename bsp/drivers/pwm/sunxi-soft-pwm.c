/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner soft pwm driver.
 *
 * Copyright(c) 2023-2028 Allwinnertech Co., Ltd.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/ioctl.h>

#define PWM_PERIOD_SET		_IOW('A', 1, unsigned long)
#define PWM_DUTY_SET		_IOW('A', 2, unsigned long)
#define PWM_START		_IOW('A', 3, unsigned long)
#define PWM_CLASS_NAME		"sunxi-soft-pwm"
#define PWM_MAJOR		247
#define PWM_PERIOD		40000
#define PWM_DUTY		20000

typedef enum {
	PWM_DISABLE = 0,
	PWM_ENABLE,
} sunxi_pwm_status_t;

struct sunxi_pwm_chip {
	dev_t devno;
	struct cdev *cdev;
	struct device *dev;
	unsigned long period;
	unsigned long duty;
	struct hrtimer mytimer;
	ktime_t kt;
	sunxi_pwm_status_t status;
	const char *desc;
	int gpio;
	int active_low;
};

struct sunxi_soft_pwm_platform_data {
	struct sunxi_pwm_chip  *pwms;
	int npwms;
};

struct sunxi_pwm_chip *pwm1_dev;
struct sunxi_pwm_chip *pwm2_dev;
static struct sunxi_pwm_chip *sunxi_gloabl_pwms_dev;
static struct sunxi_soft_pwm_platform_data *pdata;

static enum hrtimer_restart hrtimer1_handler(struct hrtimer *timer)
{
	if (gpio_get_value(pwm1_dev->gpio) == 1) {
		/* There is no need to pull down when the duty cycle is 100% */
		if (pwm1_dev->duty != 0) {
			gpio_set_value(pwm1_dev->gpio, 0);
			pwm1_dev->kt = ktime_set(0, pwm1_dev->duty);
		}
		/* timer overflow */
		hrtimer_forward_now(&pwm1_dev->mytimer, pwm1_dev->kt);
	} else {
		/* There is no need to pull up when the duty cycle is 0 */
		if (pwm1_dev->duty != pwm1_dev->period) {
			gpio_set_value(pwm1_dev->gpio, 1);
			pwm1_dev->kt = ktime_set(0, pwm1_dev->period-pwm1_dev->duty);
		}
		/* timer overflow */
		hrtimer_forward_now(&pwm1_dev->mytimer, pwm1_dev->kt);
	}

	return HRTIMER_RESTART;
}

static enum hrtimer_restart hrtimer2_handler(struct hrtimer *timer)
{
	if (gpio_get_value(pwm2_dev->gpio) == 1) {
		/* There is no need to pull down when the duty cycle is 100% */
		if (pwm2_dev->duty != 0) {
			gpio_set_value(pwm2_dev->gpio, 0);
			pwm2_dev->kt = ktime_set(0, pwm2_dev->duty);
		}
		/* timer overflow */
		hrtimer_forward_now(&pwm2_dev->mytimer, pwm2_dev->kt);
	} else {
		/* There is no need to pull up when the duty cycle is 0 */
		if (pwm2_dev->duty != pwm2_dev->period) {
			gpio_set_value(pwm2_dev->gpio, 1);
			pwm2_dev->kt = ktime_set(0, pwm2_dev->period-pwm2_dev->duty);
		}
		/* timer overflow */
		hrtimer_forward_now(&pwm2_dev->mytimer, pwm2_dev->kt);
	}

	return HRTIMER_RESTART;
}

static void sunxi_soft_pwm_start(int minor)
{
	pr_debug("sunxi_soft_pwm_start...minor=%d..\n", minor);
	switch (minor) {
	case 0:
		hrtimer_init(&pwm1_dev->mytimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		pwm1_dev->mytimer.function = hrtimer1_handler;
		pwm1_dev->kt = ktime_set(0, pwm1_dev->period - pwm1_dev->duty);
		hrtimer_start(&pwm1_dev->mytimer, pwm1_dev->kt, HRTIMER_MODE_REL);
		break;
	case 1:
		hrtimer_init(&pwm2_dev->mytimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		pwm2_dev->mytimer.function = hrtimer2_handler;
		pwm2_dev->kt = ktime_set(0, pwm2_dev->period - pwm2_dev->duty);
		hrtimer_start(&pwm2_dev->mytimer, pwm2_dev->kt, HRTIMER_MODE_REL);
		break;
	default:
		break;
	}
}

int sunxi_soft_pwm_open(struct inode *inode, struct file *filp)
{
	long minor;
	long major;

	minor = iminor(inode);
	major = imajor(inode);
	filp->private_data = (void *)minor;
	pr_debug("major = %ld minor = %ld\n", major, minor);

	return 0;
}

long sunxi_soft_pwm_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	int ret = 0, minornum = 0;
	struct inode *ginode = NULL;
	struct sunxi_pwm_chip *pwm_dev = NULL;

	ginode = file_inode(filep);
	minornum = iminor(ginode);
	pwm_dev = &sunxi_gloabl_pwms_dev[minornum];
	pr_debug("pwm_drv_ioctl.minornum=%d.period=%ld..duty=%ld..\n", minornum, pwm_dev->period, pwm_dev->duty);

	switch (minornum) {
	case 0:
		pwm1_dev =  &sunxi_gloabl_pwms_dev[minornum];
		break;
	case 1:
		pwm2_dev =  &sunxi_gloabl_pwms_dev[minornum];
		break;
	default:
		break;
	}

	switch (cmd) {
	case PWM_PERIOD_SET:
		if (minornum == 0)
			pwm1_dev->period = arg;
		else if (minornum == 1)
			pwm2_dev->period = arg;
		break;
	case PWM_DUTY_SET:
		if (minornum == 0)
			pwm1_dev->duty = arg;
		else if (minornum == 1)
			pwm2_dev->duty = arg;
		break;
	case PWM_START:
		if (minornum == 0) {
			if (pwm1_dev->status == PWM_DISABLE) {
				/* start timer */
				sunxi_soft_pwm_start(minornum);
				pwm1_dev->status = PWM_ENABLE;
			} else
				pr_debug("debug:pwm1_gpio aready work\n");
			}
		else if (minornum == 1) {
			if (pwm2_dev->status == PWM_DISABLE) {
				/* start timer */
				sunxi_soft_pwm_start(minornum);
				pwm2_dev->status = PWM_ENABLE;
			} else
				pr_debug("debug:pwm2_gpio aready work\n");
			}
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

int sunxi_soft_pwm_close(struct inode *inode, struct file *filp)
{
	return 0;
}

const struct file_operations pwm_fops = {
	.open = sunxi_soft_pwm_open,
	.unlocked_ioctl = sunxi_soft_pwm_ioctl,
	.release = sunxi_soft_pwm_close,
};

static int sunxi_soft_pwm_get_config(struct device *dev)
{
	struct device_node *node, *pp;
	struct sunxi_pwm_chip *pwm;
	int npwms;
	int i = 0;

	node = dev->of_node;
	if (!node)
		return -EINVAL;

	npwms = of_get_child_count(node);
	if (npwms == 0)
		return -EINVAL;

	pdata = devm_kzalloc(dev, sizeof(*pdata) + npwms * (sizeof *pwm), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->pwms = (struct sunxi_pwm_chip *)(pdata + 1);
	pdata->npwms = npwms;

	for_each_child_of_node(node, pp) {
		enum of_gpio_flags flags;
		if (!of_find_property(pp, "gpios", NULL)) {
			pdata->npwms--;
			pr_debug("Found pwm without gpios\n");
			continue;
		}
		pwm = &pdata->pwms[i++];
		pwm->gpio = of_get_gpio_flags(pp, 0, &flags);
		pr_debug("pwm->gpio=%d,flags=%d", pwm->gpio, flags);
		if (pwm->gpio < 0) {
			pr_err("get gpio flags failed\n");
			return pwm->gpio;
		} else {
			pwm->active_low = flags;
		}

		pwm->desc = of_get_property(pp, "label", NULL);
	}

	if (pdata->npwms == 0)
		return -EINVAL;
	return 0;
}

static int sunxi_soft_pwm_probe(struct platform_device *pdev)
{
	int ret, i;
	int alloc_flag = 0;
	int cdev_flag = 0;
	int device_flag = 0;
	int gpio_request_flag = 0;
	unsigned int gpio;
	struct device *dev = &pdev->dev;
	struct sunxi_pwm_chip *gpwm = NULL;
	static struct class *pwm_class;

	ret = sunxi_soft_pwm_get_config(dev);
	if (ret)
		return -EINVAL;

	sunxi_gloabl_pwms_dev = devm_kzalloc(dev, pdata->npwms * sizeof(*sunxi_gloabl_pwms_dev),
		GFP_KERNEL);
	if (!sunxi_gloabl_pwms_dev) {
		pr_err("no memory for sunxi_gloabl_pwms_dev data\n");
		return -ENOMEM;
	}
	memcpy(sunxi_gloabl_pwms_dev, pdata->pwms, pdata->npwms * sizeof(*sunxi_gloabl_pwms_dev));

	for (i = 0; i < pdata->npwms; i++) {
		gpwm = &sunxi_gloabl_pwms_dev[i];
		gpwm->devno = MKDEV(PWM_MAJOR, i);
		ret = alloc_chrdev_region(&gpwm->devno, 0, 1, gpwm->desc);
		if (ret < 0)
			goto err_alloc_region;
		alloc_flag++;
		gpwm->cdev = cdev_alloc();
		gpwm->cdev->owner = THIS_MODULE;
		cdev_init(gpwm->cdev, &pwm_fops);
		ret = cdev_add(gpwm->cdev, gpwm->devno, 1);
		if (ret)
			goto err_cdev_add;
		cdev_flag++;
	}
	pwm_class = class_create(THIS_MODULE, PWM_CLASS_NAME);
	if (IS_ERR(pwm_class)) {
		pr_err("class creat failed\n");
		ret = PTR_ERR(pwm_class);
		goto err_cdev_add;
	}

	for (i = 0; i < pdata->npwms; i++) {
		gpwm = &sunxi_gloabl_pwms_dev[i];
		gpio = gpwm->gpio;
		gpwm->dev = device_create(pwm_class, NULL, gpwm->devno, NULL, "pwm%d", i + 1);
		if (IS_ERR(gpwm->dev)) {
			pr_err("device creat failed\n");
			ret = PTR_ERR(gpwm);
			goto err_device_create;
		}
		device_flag++;

		if (!gpio_is_valid(gpio))
			pr_err("debug:invalid gpio,gpio=0x%x\n", gpio);

		ret = gpio_direction_output(gpio, !((gpwm->active_low == OF_GPIO_ACTIVE_LOW) ? 0 : 1));
		if (ret) {
			pr_err("unable to set direction on gpio %u, err=%d\n", gpio, ret);
			return ret;
		}

		/* default period and duty cycle */
		gpwm->period = PWM_PERIOD;
		gpwm->duty = PWM_DUTY;

		ret = devm_gpio_request(dev, gpio, gpwm->desc);
		if (ret) {
			pr_err("unable to request gpio %u, err=%d\n", gpio, ret);
			goto err_gpio_request;
		}
		gpio_request_flag++;

		gpwm->status = PWM_DISABLE;
	}
	pr_debug("sunxi soft pwm probe success\n");

	return 0;

err_gpio_request:
	for (i = 0; i < gpio_request_flag; i++) {
		gpwm = &sunxi_gloabl_pwms_dev[i];
		devm_gpio_free(dev, gpwm->gpio);
	}
err_device_create:
	class_destroy(pwm_class);
	for (i = 0; i < device_flag; i++) {
		gpwm = &sunxi_gloabl_pwms_dev[i];
		device_destroy(pwm_class, gpwm->devno);
	}
err_cdev_add:
	for (i = 0; i < cdev_flag; i++) {
		gpwm = &sunxi_gloabl_pwms_dev[i];
		cdev_del(gpwm->cdev);
	}
err_alloc_region:
	for (i = 0; i < alloc_flag; i++) {
		gpwm = &sunxi_gloabl_pwms_dev[i];
		unregister_chrdev_region(gpwm->devno, 1);
	}
	return ret;
}

static int sunxi_soft_pwm_remove(struct platform_device *pdev)
{
	int i;
	struct device *dev = &pdev->dev;
	struct sunxi_pwm_chip *gpwm = NULL;
	static struct class *pwm_class;

	for (i = 0; i < pdata->npwms; i++) {
		gpwm = &sunxi_gloabl_pwms_dev[i];
		hrtimer_cancel(&gpwm->mytimer);
		gpio_set_value(gpwm->gpio, 1);
		devm_gpio_free(dev, gpwm->gpio);
	}

	class_destroy(pwm_class);

	for (i = 0; i < pdata->npwms; i++) {
		gpwm = &sunxi_gloabl_pwms_dev[i];
		device_destroy(pwm_class, gpwm->devno);
		cdev_del(gpwm->cdev);
		unregister_chrdev_region(gpwm->devno, 1);
	}

	return 0;
}

static struct of_device_id sunxi_soft_pwm_of_match[] = {
	{.compatible = "sunxi-soft-pwm"},
	{},
}
MODULE_DEVICE_TABLE(of, sunxi_soft_pwm_of_match);

static struct platform_driver sunxi_soft_pwm_driver = {
	.probe = sunxi_soft_pwm_probe,
	.remove = sunxi_soft_pwm_remove,
	.driver = {
		.name = "sunxi-soft-pwm",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sunxi_soft_pwm_of_match),
	}
};

module_platform_driver(sunxi_soft_pwm_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("soft pwm driver");
MODULE_AUTHOR("zhaiyaya <zhaiyaya@allwinnertech.com>");
MODULE_VERSION("1.0.1");

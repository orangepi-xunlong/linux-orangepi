// SPDX-License-Identifier: GPL-2.0
/*
 * simple driver for PWM (Pulse Width Modulator) controller
 *
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/workqueue.h>

static struct gpio_desc *camera_pwr_key, *lid_status_key;
volatile bool camera_pwr_flag = false;
struct kobject *kobj_camera_power;

struct camera_power_queue {
	struct delayed_work power_work;
	struct workqueue_struct *power_workqueue;
};
static struct camera_power_queue *camera_on, *camera_off;

static ssize_t sysfs_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	if (camera_pwr_flag)
		return sprintf(buf, "camera power on\n");
	return sprintf(buf, "camera power off\n");
}

static void camera_power_on_work(struct work_struct *work)
{
	pr_info("camera_power_on_work start\n");
	camera_pwr_flag = true;
	gpiod_set_value_cansleep(camera_pwr_key, 1);
	pr_info("camera_power_on_work end\n");
}

static void camera_power_off_work(struct work_struct *work)
{
	pr_info("camera_power_off_work start\n");
	camera_pwr_flag = false;
	gpiod_set_value_cansleep(camera_pwr_key, 0);
	pr_info("camera_power_off_work end\n");

}

static ssize_t sysfs_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	if (!strcmp(buf, "on\n")) {
		pr_info("camera on!\n");
		gpiod_set_value_cansleep(camera_pwr_key, 1);
		camera_pwr_flag = true;
	} else if (!strcmp(buf, "off\n")) {
		pr_info("camera off!\n");
		gpiod_set_value_cansleep(camera_pwr_key, 0);
		camera_pwr_flag = false;
	} else
		pr_err("please input on/off to control camera power!\n");

	return count;
}

struct kobj_attribute sysfs_camera_power_attr = __ATTR(camera_pwr_flag, 0664, sysfs_show, sysfs_store);


static irqreturn_t e_shutter_interrupt(int irq, void *dev)
{
	int value = gpiod_get_value(lid_status_key);

	if (value)
		queue_delayed_work(camera_on->power_workqueue, &camera_on->power_work, 0);
	else
		queue_delayed_work(camera_off->power_workqueue, &camera_off->power_work, 0);

	return IRQ_HANDLED;
}

static int e_shutter_probe(struct platform_device *pdev)
{
	int ret, irq;

	lid_status_key = devm_gpiod_get_optional(&pdev->dev, "lid", GPIOD_IN);
	if (IS_ERR(lid_status_key))
		return PTR_ERR(lid_status_key);

	camera_pwr_flag = true;
	camera_on = devm_kzalloc(&pdev->dev, sizeof(struct camera_power_queue), GFP_KERNEL);
	camera_off = devm_kzalloc(&pdev->dev, sizeof(struct camera_power_queue), GFP_KERNEL);

	INIT_DELAYED_WORK(&camera_on->power_work, camera_power_on_work);
	INIT_DELAYED_WORK(&camera_off->power_work, camera_power_off_work);

	camera_on->power_workqueue = alloc_workqueue("power_on_workqueue", WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (camera_on->power_workqueue == NULL) {
		dev_err(&pdev->dev, "alloc workqueue failed\n");
		ret = -ENOMEM;
		return ret;
	}

	camera_off->power_workqueue = alloc_workqueue("power_off_workqueue", WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (camera_off->power_workqueue == NULL) {
		dev_err(&pdev->dev, "alloc workqueue failed\n");
		ret = -ENOMEM;
		return ret;
	}

	irq = gpiod_to_irq(lid_status_key);
	if (irq < 0) {
		dev_err(&pdev->dev, "unable to get IRQ(%d)\n", irq);
		return irq;
	}

	ret = devm_request_irq(&pdev->dev,
			       irq,
			       e_shutter_interrupt,
			       IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
			       "e_shutter_ack",
			       NULL);
	if (ret) {
		dev_err(&pdev->dev, "failed to request e_shutter interrupt\n");
		return ret;
	}

	camera_pwr_key = devm_gpiod_get_optional(&pdev->dev, "poweren", GPIOD_OUT_HIGH);
	if (IS_ERR(camera_pwr_key))
		return PTR_ERR(camera_pwr_key);

	kobj_camera_power = kobject_create_and_add("camera_power_status", NULL);

	if (sysfs_create_file(kobj_camera_power, &sysfs_camera_power_attr.attr)) {
		pr_err("create sysfs file error!\n");
		goto error_sysfs;

	}
	pr_info("e_shutter_probe complete!\n");
	return 0;

error_sysfs:
	kobject_put(kobj_camera_power);
	sysfs_remove_file(kernel_kobj, &sysfs_camera_power_attr.attr);
	return -1;
}

static int e_shutter_remove(struct platform_device *pdev)
{
	kobject_put(kobj_camera_power);
	sysfs_remove_file(kernel_kobj, &sysfs_camera_power_attr.attr);
	pr_info("remove e shutter module!\n");
	return 0;
}

static const struct of_device_id e_shutter_dt_ids[] = {
	{ .compatible = "sky1, e_shutter",},
	{ /* sentinel */ }
};

static struct platform_driver e_shutter_driver = {
	.driver = {
		.name = "e_shutter",
		.of_match_table = e_shutter_dt_ids,
	},
	.probe = e_shutter_probe,
	.remove = e_shutter_remove,
};
module_platform_driver(e_shutter_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jerry Zhu <jerry.zhu@cixtech.com>");

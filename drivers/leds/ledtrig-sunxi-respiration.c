#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/leds.h>
#include <asm/uaccess.h>
#include "leds.h"

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);

struct sunxi_trig_data {
	unsigned long on_ms;
	unsigned long off_ms;
	unsigned long color;
};

static struct mutex trig_lock; 

struct sunxi_trig_data trig_data ={
	.on_ms = 0,
	.off_ms =0,
};

static ssize_t RGB_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t RGB_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	return count;
}

static ssize_t on_ms_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%lu\n",trig_data.on_ms);
}

static ssize_t on_ms_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long state = simple_strtoul(buf, NULL, 10);
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	mutex_lock(&trig_lock);
	trig_data.on_ms = state;
	led_blink_set(led_cdev, &trig_data.on_ms, &trig_data.off_ms);
	mutex_unlock(&trig_lock);
	return count;
}

static ssize_t off_ms_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%lu\n",trig_data.off_ms);
}

static ssize_t off_ms_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long state = simple_strtoul(buf, NULL, 10);
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	mutex_lock(&trig_lock);
	trig_data.off_ms = state;
	led_blink_set(led_cdev, &trig_data.on_ms, &trig_data.off_ms);
	mutex_unlock(&trig_lock);
	return count;
}

static ssize_t ctrl_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	if(count < sizeof(struct sunxi_trig_data))
		return -1;
	mutex_lock(&trig_lock);
	memcpy((void*)&trig_data,buf,sizeof(struct sunxi_trig_data));
	led_blink_set(led_cdev, &trig_data.on_ms, &trig_data.off_ms);
	mutex_unlock(&trig_lock);
	return sizeof(struct sunxi_trig_data);
}

static DEVICE_ATTR(RGB, S_IRUGO|S_IWUSR|S_IWGRP,
		RGB_show, RGB_store);

static DEVICE_ATTR(on_ms, S_IRUGO|S_IWUSR|S_IWGRP,
		on_ms_show, on_ms_store);

static DEVICE_ATTR(off_ms, S_IRUGO|S_IWUSR|S_IWGRP,
		off_ms_show, off_ms_store);

static DEVICE_ATTR(ctrl, S_IRUGO|S_IWUSR|S_IWGRP,
		NULL, ctrl_store);

static struct attribute *sunxi_respiration_attributes[] = {
	&dev_attr_RGB.attr,
	&dev_attr_on_ms.attr,
	&dev_attr_off_ms.attr,
	&dev_attr_ctrl.attr,
	NULL
};

static struct attribute_group sunxi_respiration_attribute_group = {
	.attrs = sunxi_respiration_attributes,
};

static void respiration_trig_activate(struct led_classdev *led_cdev)
{
	/*  here */
	sysfs_create_group(&led_cdev->dev->kobj,
						 &sunxi_respiration_attribute_group);
	mutex_lock(&trig_lock);
	led_blink_set(led_cdev, &trig_data.on_ms, &trig_data.off_ms);
	mutex_unlock(&trig_lock);
}

static void respiration_trig_deactivate(struct led_classdev *led_cdev)
{
	/*  here */
	unsigned long data0 = 0;
	unsigned long data1 = 0;
	mutex_lock(&trig_lock);
	led_blink_set(led_cdev, &data0, &data1);
	mutex_unlock(&trig_lock);
	sysfs_remove_group(&led_cdev->dev->kobj,
						 &sunxi_respiration_attribute_group);
}

static struct led_trigger respiration_lamp_trigger = {
	.name		= "sunxi_respiration_trigger",
	.activate	= respiration_trig_activate,
	.deactivate	= respiration_trig_deactivate,
};

static int __init respiration_trig_init(void)
{
	mutex_init(&trig_lock);
	if(led_trigger_register(&respiration_lamp_trigger))
	{
		pr_err("[%s]trigger register fail.\n",__func__);
		return -1;
	}
	return 0;	
}

static void __exit respiration_trig_exit(void)
{
	led_trigger_unregister(&respiration_lamp_trigger);
}

module_init(respiration_trig_init);
module_exit(respiration_trig_exit);

MODULE_AUTHOR("Qin ");
MODULE_DESCRIPTION("Sunxi respiration LED trigger");
MODULE_LICENSE("GPL");


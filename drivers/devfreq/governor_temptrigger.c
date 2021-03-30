/*
 * linux/drivers/devfreq/governor_temptrigger.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *
 * Author: Pan Nan <pannan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sunxi_dramfreq.h>
#include "governor.h"

long temp_data;
unsigned int temp_max = 105;
unsigned int temp_min = 95;

static ssize_t show_temp(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%ld\n", temp_data);
}

static ssize_t show_max(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%u\n", temp_max);
}

static ssize_t store_max(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned int value;
	int ret;

	ret = sscanf(buf, "%u", &value);
	if (ret != 1)
		return -EINVAL;

	if (value <= temp_min)
		return -EINVAL;

	temp_max = value;

	return count;
}

static ssize_t show_min(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%u\n", temp_min);
}

static ssize_t store_min(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned int value;
	int ret;

	ret = sscanf(buf, "%u", &value);
	if (ret != 1)
		return -EINVAL;

	if (value >= temp_max)
		return -EINVAL;

	temp_min = value;

	return count;
}

static DEVICE_ATTR(temp, 0444, show_temp, NULL);
static DEVICE_ATTR(max, 0644, show_max, store_max);
static DEVICE_ATTR(min, 0644, show_min, store_min);
static struct attribute *dev_entries[] = {
	&dev_attr_temp.attr,
	&dev_attr_max.attr,
	&dev_attr_min.attr,
	NULL,
};
static struct attribute_group dev_attr_group = {
	.name   = "temptrigger",
	.attrs  = dev_entries,
};

static int devfreq_temptrigger_func(struct devfreq *df, unsigned long *freq)
{
	struct devfreq_dev_status stat;
	int err = df->profile->get_dev_status(df->dev.parent, &stat);

	if (err)
		return err;

	temp_data = *(long *)stat.private_data;

	if (temp_data > temp_max) {
		pr_debug("temp:%ld > %u\n", temp_data, temp_max);
		*freq = df->max_freq >> 1;
	} else if (temp_data < temp_min) {
		pr_debug("temp:%ld < %u\n", temp_data, temp_min);
		*freq = df->max_freq;
	} else {
		*freq = df->previous_freq;
	}

	return 0;
}

static int temptrigger_init(struct devfreq *devfreq)
{
	int ret = 0;

	if (!devfreq)
		return -EINVAL;

	ret = sysfs_create_group(&devfreq->dev.kobj, &dev_attr_group);
	if (ret)
		return ret;

	if (!dramfreq) {
		pr_err("%s: paras error\n", __func__);
		return -EINVAL;
	}

	dramfreq->governor_state_update(devfreq->governor_name, STATE_INIT);

	return 0;
}

static void temptrigger_exit(struct devfreq *devfreq)
{
	if (!devfreq)
		return;

	sysfs_remove_group(&devfreq->dev.kobj, &dev_attr_group);

	if (!dramfreq) {
		pr_err("%s: paras error\n", __func__);
		return;
	}

	dramfreq->governor_state_update(devfreq->governor_name, STATE_EXIT);
}

static int devfreq_temptrigger_handler(struct devfreq *devfreq,
			unsigned int event, void *data)
{
	int ret = 0;

	switch (event) {
	case DEVFREQ_GOV_START:
		ret = temptrigger_init(devfreq);
		break;
	case DEVFREQ_GOV_STOP:
		temptrigger_exit(devfreq);
		break;
	default:
		break;
	}

	return ret;
}

static struct devfreq_governor devfreq_temptrigger = {
	.name = "temptrigger",
	.get_target_freq = devfreq_temptrigger_func,
	.event_handler = devfreq_temptrigger_handler,
};

static int __init devfreq_temptrigger_init(void)
{
	return devfreq_add_governor(&devfreq_temptrigger);
}
late_initcall(devfreq_temptrigger_init);

static void __exit devfreq_temptrigger_exit(void)
{
	if (devfreq_remove_governor(&devfreq_temptrigger))
		pr_err("%s: failed remove governor\n", __func__);

	return;
}
module_exit(devfreq_temptrigger_exit);
MODULE_LICENSE("GPL");

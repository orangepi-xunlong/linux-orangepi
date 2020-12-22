/**
 * dummy-acc.c
 * Dummy Acceleration Sensor Driver
 *
 * Copyright (C) 2013-2015 Allwinnertech Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <asm/io.h>

#define DUMMY_ACC_DRV_NAME      "Virtual.Acc"

#define INPUT_FUZZ               2
#define INPUT_FLAT               2
#define POLL_INTERVAL_MAX       500
#define POLL_INTERVAL           100     /* msecs */

#define ACC_MAX_DELAY   (1000)
#define ACC_MIN_DELAY      (1)

struct dummy_acc_t {
    struct input_polled_dev *dev;
    struct mutex interval_mutex;
    atomic_t delay;
    atomic_t enable;
};

struct dummy_acc_t dummy_acc;

static ssize_t dummy_acc_delay_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    pr_info("%s: output delay %dms\n", __func__, atomic_read(&dummy_acc.delay));
    return sprintf(buf, "%d\n", atomic_read(&dummy_acc.delay));
}

static ssize_t dummy_acc_delay_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (data > ACC_MAX_DELAY)
        data = ACC_MAX_DELAY;
    atomic_set(&dummy_acc.delay, (unsigned int) data);

    /* modify the poll interval */
	mutex_lock(&dummy_acc.interval_mutex);
	dummy_acc.dev->poll_interval = atomic_read(&dummy_acc.delay);
	mutex_unlock(&dummy_acc.interval_mutex);

    return count;
}

static ssize_t dummy_acc_enable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    pr_info("%s: output enable %d\n", __func__, atomic_read(&dummy_acc.enable));
    return sprintf(buf, "%d\n", atomic_read(&dummy_acc.enable));
}

static ssize_t dummy_acc_enable_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if ((data == 0)||(data==1)) {
        atomic_set(&dummy_acc.enable, (unsigned int) data);
    }

    return count;
}

static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
        dummy_acc_delay_show, dummy_acc_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
        dummy_acc_enable_show, dummy_acc_enable_store);

static struct attribute *dummy_acc_attributes[] = {
    &dev_attr_delay.attr,
    &dev_attr_enable.attr,
    NULL
};

static struct attribute_group dummy_acc_attribute_group = {
    .attrs = dummy_acc_attributes
};

/* report dump value */
static void dummy_acc_poll(struct input_polled_dev *dev)
{
    input_report_abs(dummy_acc.dev->input, ABS_X, 0);
    input_report_abs(dummy_acc.dev->input, ABS_Y, 5);
    input_report_abs(dummy_acc.dev->input, ABS_Z, 0);

    input_sync(dummy_acc.dev->input);
}

static int __init dummy_acc_init(void)
{
    struct input_dev *idev;
    int result = 0;

    /*input poll device register */
    dummy_acc.dev = input_allocate_polled_device();
    if (IS_ERR_OR_NULL(dummy_acc.dev)) {
        pr_err("dummy-acc: alloc poll device failed!\n");
        result = -ENOMEM;
        return result;
    }

    dummy_acc.dev->poll = dummy_acc_poll;
    dummy_acc.dev->poll_interval = POLL_INTERVAL;
    dummy_acc.dev->poll_interval_max = POLL_INTERVAL_MAX;
    idev = dummy_acc.dev->input;
    idev->name = DUMMY_ACC_DRV_NAME;
    idev->id.bustype = BUS_VIRTUAL;
    idev->evbit[0] = BIT_MASK(EV_ABS);

    input_set_abs_params(idev, ABS_X, -512, 512, INPUT_FUZZ, INPUT_FLAT);
    input_set_abs_params(idev, ABS_Y, -512, 512, INPUT_FUZZ, INPUT_FLAT);
    input_set_abs_params(idev, ABS_Z, -512, 512, INPUT_FUZZ, INPUT_FLAT);

    result = input_register_polled_device(dummy_acc.dev);
    if (result) {
        pr_err("dummy-acc: register poll device failed!\n");
        goto failed_register;
    }
    dummy_acc.dev->input->close(dummy_acc.dev->input);

    result = sysfs_create_group(&dummy_acc.dev->input->dev.kobj, &dummy_acc_attribute_group);
    if (result) {
        pr_err("dummy-acc: sysfs_create_group err\n");
        goto sysfs_create_failed;
    }

	mutex_init(&dummy_acc.interval_mutex);
    atomic_set(&dummy_acc.enable, 0);
    atomic_set(&dummy_acc.delay, ACC_MAX_DELAY);

    return 0;

sysfs_create_failed:
    input_unregister_polled_device(dummy_acc.dev);
failed_register:
    input_free_polled_device(dummy_acc.dev);
    return result;
}

static void __exit dummy_acc_exit(void)
{
    sysfs_remove_group(&dummy_acc.dev->input->dev.kobj, &dummy_acc_attribute_group);
    input_unregister_polled_device(dummy_acc.dev);
    input_free_polled_device(dummy_acc.dev);
}

module_init(dummy_acc_init);
module_exit(dummy_acc_exit);

MODULE_DESCRIPTION("Dummy Acceleration Sensor Driver");
MODULE_LICENSE("GPL v2");

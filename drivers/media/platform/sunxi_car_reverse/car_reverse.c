/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/sys_config.h>
#include <linux/platform_device.h>

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
#include <linux/switch.h>
#endif

#include "include.h"
#include "car_reverse.h"

#define MODULE_NAME "car-reverse"

#define SWAP_BUFFER_CNT		(5)

#define THREAD_NEED_STOP	(1 << 0)
#define THREAD_RUN			(1 << 1)
struct car_reverse_private_data {
	struct preview_params config;
	int reverse_gpio;

	struct buffer_pool *buffer_pool;

	struct work_struct status_detect;
	struct workqueue_struct *preview_workqueue;

	struct task_struct *display_update_task;
	struct list_head pending_frame;
	spinlock_t display_lock;

	int needexit;
	int status;
	int debug;

	int thread_mask;
	spinlock_t thread_lock;
};

static int rotate;
module_param(rotate, int, 0644);

static struct car_reverse_private_data *car_reverse;

#define UPDATE_STATE 1
#if defined(UPDATE_STATE) && (defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH))

static ssize_t print_dev_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", sdev->name);
}

static struct switch_dev car_reverse_switch = {
	.name  = "parking-switch",
	.state = 0,
	.print_name  = print_dev_name,
};

static void car_reverse_switch_register(void)
{
	switch_dev_register(&car_reverse_switch);
}

#if 0
static void car_reverse_switch_unregister(void)
{
	switch_dev_unregister(&car_reverse_switch);
}
#endif

static void car_reverse_switch_update(int flag)
{
	switch_set_state(&car_reverse_switch, flag);
}
#else
static void car_reverse_switch_register(void)   {};
static void car_reverse_switch_update(int flag) {};
#endif

static void of_get_value_by_name(struct platform_device *pdev,
				 const char *name, int *retval, unsigned int defval)
{
	if (of_property_read_u32(pdev->dev.of_node, name, retval) != 0) {
		dev_err(&pdev->dev,
			"missing property '%s', default value %d\n",
			name, defval);
		*retval = defval;
	}
}

static void of_get_gpio_by_name(struct platform_device *pdev,
				const char *name, int *retval)
{
	int gpio_index;
	struct gpio_config config;

	gpio_index = of_get_named_gpio_flags(pdev->dev.of_node, name,
					     0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(gpio_index)) {
		dev_err(&pdev->dev, "failed to get gpio '%s'\n", name);
		*retval = 0;
		return;
	}
	*retval = gpio_index;

	dev_info(&pdev->dev,
		 "%s: gpio=%d mul-sel=%d pull=%d drv_level=%d data=%d\n",
		 name, config.gpio, config.mul_sel, config.pull,
		 config.drv_level, config.data);
}

static void parse_config(struct platform_device *pdev,
			 struct car_reverse_private_data *priv)
{
	of_get_value_by_name(pdev,
		"tvd_id", &priv->config.tvd_id, 0);
	of_get_value_by_name(pdev,
		"screen_width", &priv->config.screen_width, 0);
	of_get_value_by_name(pdev,
		"screen_height", &priv->config.screen_height, 0);
	of_get_value_by_name(pdev,
		"rotation", &priv->config.rotation, 1);

	of_get_gpio_by_name(pdev, "reverse_pin", &priv->reverse_gpio);
}

void car_reverse_display_update(void)
{
	struct buffer_node *node;
	struct list_head *pending_frame = &car_reverse->pending_frame;

	spin_lock(&car_reverse->display_lock);
	while (!list_empty(pending_frame)) {
		node = list_entry(pending_frame->next, struct buffer_node, list);
		list_del(&node->list);
		video_source_queue_buffer(node);
	}
	node = video_source_dequeue_buffer();
	if (node)
		list_add(&node->list, pending_frame);
	spin_unlock(&car_reverse->display_lock);

	spin_lock(&car_reverse->thread_lock);
	if (car_reverse->thread_mask & THREAD_NEED_STOP) {
		spin_unlock(&car_reverse->thread_lock);
		return;
	}
	wake_up_process(car_reverse->display_update_task);
	car_reverse->thread_mask |= THREAD_RUN;
	spin_unlock(&car_reverse->thread_lock);
}

static int car_reverse_preview_start(void)
{
	int retval = 0;
	int i;
	struct buffer_node *node;
	struct buffer_pool *bp = car_reverse->buffer_pool;

	retval = video_source_connect(&car_reverse->config);
	if (retval != 0) {
		logerror("can't connect to video source!\n");
		return -1;
	}

	preview_output_start(&car_reverse->config);

	for (i = 0; i < (SWAP_BUFFER_CNT - 1); i++) {
		node = bp->dequeue_buffer(bp);
		video_source_queue_buffer(node);
	}

	video_source_streamon();
	car_reverse->status = CAR_REVERSE_START;
	car_reverse->thread_mask = 0;
	return 0;
}

static int car_reverse_preview_stop(void)
{
	struct buffer_node *node;
	struct buffer_pool *bp = car_reverse->buffer_pool;
	struct list_head *pending_frame = &car_reverse->pending_frame;

	car_reverse->status = CAR_REVERSE_STOP;
	video_source_streamoff();

	spin_lock(&car_reverse->thread_lock);
	car_reverse->thread_mask |= THREAD_NEED_STOP;
	spin_unlock(&car_reverse->thread_lock);

	while (car_reverse->thread_mask & THREAD_RUN)
		msleep(1);

	preview_output_stop();

__buffer_gc:
	node = video_source_dequeue_buffer();
	if (node) {
		bp->queue_buffer(bp, node);
		logdebug("%s: collect %p\n", __func__, node->phy_address);
		goto __buffer_gc;
	}

	spin_lock(&car_reverse->display_lock);
	while (!list_empty(pending_frame)) {
		node = list_entry(pending_frame->next, struct buffer_node, list);
		list_del(&node->list);
		bp->queue_buffer(bp, node);
	}
	spin_unlock(&car_reverse->display_lock);
	rest_buffer_pool(NULL, bp);
	dump_buffer_pool(NULL, bp);

	video_source_disconnect(&car_reverse->config);

	return 0;
}

static int car_reverse_gpio_status(void)
{
#ifdef _REVERSE_DEBUG_
	return (car_reverse->debug == CAR_REVERSE_START ?
					CAR_REVERSE_START : CAR_REVERSE_STOP);
#else
	int value = gpio_get_value(car_reverse->reverse_gpio);
	return (value == 0) ? CAR_REVERSE_START : CAR_REVERSE_STOP;
#endif
}

/*
 *  current status | gpio status | next status
 *  ---------------+-------------+------------
 *        STOP     |    STOP     |    HOLD
 *  ---------------+-------------+------------
 *        STOP     |    START    |    START
 *  ---------------+-------------+------------
 *        START    |    STOP     |    STOP
 *  ---------------+-------------+------------
 *        START    |    START    |    HOLD
 *  ---------------+-------------+------------
 */
const int _transfer_table[3][3] = {
	[0]                 = {0, 0, 0},
	[CAR_REVERSE_START] = {0, CAR_REVERSE_HOLD,  CAR_REVERSE_STOP},
	[CAR_REVERSE_STOP]  = {0, CAR_REVERSE_START, CAR_REVERSE_HOLD},
};

static int car_reverse_get_next_status(void)
{
	int next_status;
	int gpio_status = car_reverse_gpio_status();
	int curr_status = car_reverse->status;

	car_reverse_switch_update(gpio_status == CAR_REVERSE_START ? 1 : 0);
	next_status = _transfer_table[curr_status][gpio_status];

	return next_status;
}

static void status_detect_func(struct work_struct *work)
{
	int retval;
	int status = car_reverse_get_next_status();

	switch (status) {
	case CAR_REVERSE_START:
		if (!car_reverse->needexit) {
			retval = car_reverse_preview_start();
			logdebug("start car reverse, return %d\n", retval);
		}
		break;
	case CAR_REVERSE_STOP:
		retval = car_reverse_preview_stop();
		logdebug("stop car reverse, return %d\n", retval);
		break;
	case CAR_REVERSE_HOLD:
	default:
		break;
	}
	return;
}

static irqreturn_t reverse_irq_handle(int irqnum, void *data)
{
	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	return IRQ_HANDLED;
}

static int display_update_thread(void *data)
{
	struct buffer_node *new_frame = NULL, *old_frame = NULL;
	struct list_head *pending_frame = &car_reverse->pending_frame;
	struct buffer_pool *bp = car_reverse->buffer_pool;

	while (!kthread_should_stop()) {

		spin_lock(&car_reverse->display_lock);
		if (pending_frame->next != pending_frame) {
			new_frame = list_entry(pending_frame->next,
								struct buffer_node, list);
			list_del(&new_frame->list);
			bp->queue_buffer(bp, new_frame);
		}

		old_frame = bp->dequeue_buffer(bp);
		list_add(&old_frame->list, pending_frame);
		spin_unlock(&car_reverse->display_lock);

		preview_update(new_frame);

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			set_current_state(TASK_RUNNING);

		car_reverse->thread_mask &= (~THREAD_RUN);
		schedule();
	}
	logdebug("%s stop\n", __func__);
	return 0;
}

static ssize_t car_reverse_status_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	int count = 0;

	if (car_reverse->status == CAR_REVERSE_STOP)
		count += sprintf(buf, "%s\n", "stop");
	else if (car_reverse->status == CAR_REVERSE_START)
		count += sprintf(buf, "%s\n", "start");
	else
		count += sprintf(buf, "%s\n", "unknow");
	return count;
}

static ssize_t car_reverse_needexit_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1))
		car_reverse->needexit = 1;
	else
		car_reverse->needexit = 0;

	return count;
}

#ifdef _REVERSE_DEBUG_
static ssize_t car_reverse_debug_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count)
{
	if (!strncmp(buf, "stop", 4))
		car_reverse->debug = CAR_REVERSE_STOP;
	else if (!strncmp(buf, "start", 5))
		car_reverse->debug = CAR_REVERSE_START;

	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);

	return count;
}
#endif

static struct class_attribute car_reverse_attrs[] = {
	__ATTR(status, S_IRUGO | S_IWUSR,
			car_reverse_status_show, NULL),
	__ATTR(needexit, S_IRUGO | S_IWUSR,
			NULL, car_reverse_needexit_store),
#ifdef _REVERSE_DEBUG_
	__ATTR(debug, S_IRUGO | S_IWUSR,
			NULL, car_reverse_debug_store),
#endif
	__ATTR_NULL
};

static struct class car_reverse_class = {
	.name = "car_reverse",
	.class_attrs = car_reverse_attrs,
};

static int car_reverse_probe(struct platform_device *pdev)
{
	int retval = 0;
	int reverse_pin_irqnum;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "of_node is missing\n");
		retval = -EINVAL;
		goto _err_out;
	}

	car_reverse = devm_kzalloc(&pdev->dev,
			    sizeof(struct car_reverse_private_data),
			    GFP_KERNEL);
	if (!car_reverse) {
		dev_err(&pdev->dev, "kzalloc for private data failed\n");
		retval = -ENOMEM;
		goto _err_out;
	}
	platform_set_drvdata(pdev, car_reverse);
	parse_config(pdev, car_reverse);
	INIT_LIST_HEAD(&car_reverse->pending_frame);
	spin_lock_init(&car_reverse->display_lock);
	spin_lock_init(&car_reverse->thread_lock);

	car_reverse->needexit = 0;
	car_reverse->status = CAR_REVERSE_STOP;
	car_reverse->config.dev = &pdev->dev;

	reverse_pin_irqnum = gpio_to_irq(car_reverse->reverse_gpio);
	if (IS_ERR_VALUE(reverse_pin_irqnum)) {
		dev_err(&pdev->dev,
			"map gpio [%d] to virq failed, errno = %d\n",
			car_reverse->reverse_gpio, reverse_pin_irqnum);
		retval = -EINVAL;
		goto _err_out;
	}

	car_reverse->preview_workqueue =
		create_singlethread_workqueue("car-reverse-wq");
	if (!car_reverse->preview_workqueue) {
		dev_err(&pdev->dev, "create workqueue failed\n");
		retval = -EINVAL;
		goto _err_out;
	}
	INIT_WORK(&car_reverse->status_detect, status_detect_func);

	/* FIXME: Calculate buffer size by preview info */
	car_reverse->buffer_pool = alloc_buffer_pool(&pdev->dev, SWAP_BUFFER_CNT, 720*574*2);

	if (!car_reverse->buffer_pool) {
		dev_err(&pdev->dev, "alloc buffer memory failed\n");
		retval = -ENOMEM;
		goto _err_out;
	}

	car_reverse->display_update_task =
		kthread_create(display_update_thread, NULL, "sunxi-preview");
	if (!car_reverse->display_update_task) {
		dev_err(&pdev->dev, "failed to create kthread\n");
		retval = -EINVAL;
		goto _err_free_buffer;
	}

	if (request_irq(reverse_pin_irqnum, reverse_irq_handle,
			IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "car-reverse", pdev)) {

		dev_err(&pdev->dev, "request irq %d failed\n",
			reverse_pin_irqnum);
		retval = -EBUSY;
		goto _err_free_buffer;
	}
	class_register(&car_reverse_class);
	car_reverse_switch_register();

#if 1
	car_reverse->debug = CAR_REVERSE_START;
	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
#endif

	dev_info(&pdev->dev, "car reverse module probe ok\n");
	return 0;

_err_free_buffer:
	free_buffer_pool(&pdev->dev, car_reverse->buffer_pool);
_err_out:
	dev_err(&pdev->dev, "car reverse module exit, errno %d!\n", retval);
	return retval;
}

static int car_reverse_remove(struct platform_device *pdev)
{
	struct car_reverse_private_data *priv = car_reverse;

	/* car_reverse_switch_unregister(); */
	class_unregister(&car_reverse_class);
	kthread_stop(priv->display_update_task);
	free_irq(gpio_to_irq(priv->reverse_gpio), pdev);
	free_buffer_pool(&pdev->dev, priv->buffer_pool);

	cancel_work_sync(&priv->status_detect);
	if (priv->preview_workqueue != NULL) {
		flush_workqueue(priv->preview_workqueue);
		destroy_workqueue(priv->preview_workqueue);
		priv->preview_workqueue = NULL;
	}

	dev_info(&pdev->dev, "car reverse module exit\n");
	return 0;
}

static int car_reverse_suspend(struct device *dev)
{
	return 0;
}

static int car_reverse_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops car_reverse_pm_ops = {
	.suspend = car_reverse_suspend,
	.resume = car_reverse_resume,
};

static const struct of_device_id car_reverse_dt_ids[] = {
	{.compatible = "allwinner,sunxi-car-reverse"},
	{},
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

static int __init car_reverse_module_init(void)
{
	int ret;

	ret = platform_driver_register(&car_reverse_driver);
	if (ret) {
		pr_err("platform driver register failed\n");
		return ret;
	}

	return 0;
}

static void __exit car_reverse_module_exit(void)
{
	platform_driver_unregister(&car_reverse_driver);
}

subsys_initcall_sync(car_reverse_module_init);
module_exit(car_reverse_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zeng.Yajian <zengyajian@allwinnertech.com>");
MODULE_DESCRIPTION("Sunxi fast car reverse image preview");

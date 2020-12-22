/*
 * linux/drivers/devfreq/governor_dsm.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *
 * Author: Pan Nan <pannan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#ifdef CONFIG_INPUT
#include <linux/input.h>
#endif
#include <linux/module.h>
#include <linux/kthread.h>
#ifdef CONFIG_EARLYSUSPEND
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#endif
#include <mach/sys_config.h>
#include "dramfreq/sunxi-mdfs.h"
#include "governor.h"

#if (1)
	#define DSM_DBG(format,args...)   printk("[dsm] "format,##args)
#else
	#define DSM_DBG(format,args...)   do{}while(0)
#endif

#define DSM_ERR(format,args...)   printk(KERN_ERR "[dsm] ERR:"format,##args)

extern unsigned int sunxi_ddrfreq_max;
extern unsigned int sunxi_ddrfreq_min;
static int dsm_enable = 1;
static struct task_struct *dsm_task;
static struct devfreq *this_devfreq;

#ifdef CONFIG_INPUT
static atomic_t g_input_lock = ATOMIC_INIT(0);
static struct timer_list input_timer;
static unsigned long input_expired_time;
#endif

#ifdef CONFIG_EARLYSUSPEND
atomic_t ddrfreq_dsm_suspend_lock = ATOMIC_INIT(0);
#endif

enum DDR_SCENE_TYPE
{
	SCENE_DEFAULT = 0,
	SCENE_MAIN,
	SCENE_VIDEO,
	SCENE_MUSIC,
	SCENE_VIDEO_4K,
	SCENE_NONE,
};

static enum DDR_SCENE_TYPE g_scene = SCENE_DEFAULT;
static unsigned int table_length_syscfg = SCENE_NONE;
static unsigned int scene_count_used    = SCENE_NONE;
static int ddr_freq_table_syscfg[SCENE_NONE][2];

#if defined(CONFIG_ARCH_SUN9IW1P1)
static int ddr_freq_table[][2] = {
	{ SCENE_DEFAULT,  672000 },
	{ SCENE_MAIN,     480000 },
	{ SCENE_VIDEO,    240000 },
	{ SCENE_VIDEO_4K, 480000 },
	{ SCENE_MUSIC,    168000 },
};
#elif defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW8P1)
static int ddr_freq_table[][2] = {
	{ SCENE_DEFAULT,  552000 },
	{ SCENE_MAIN,     360000 },
	{ SCENE_VIDEO,    240000 },
	{ SCENE_MUSIC,    168000 },
};
#elif defined(CONFIG_ARCH_SUN8IW6P1)
static int ddr_freq_table[][2] = {
	{ SCENE_DEFAULT,  672000 },
	{ SCENE_MAIN,     336000 },
	{ SCENE_VIDEO,    224000 },
	{ SCENE_MUSIC,    168000 },
};

extern unsigned int mdfs_in_cfs;
#endif

static int __get_freq_by_scene(int (*table)[2], enum DDR_SCENE_TYPE scene)
{
	int i, target_freq = -1;

	for (i = 0; i < scene_count_used; i++) {
		if (table[i][0] == scene) {
			target_freq = table[i][1];
			break;
		}
	}

	return target_freq;
}

static int devfreq_dsm_func(struct devfreq *df, unsigned long *freq)
{
	int target = 0;

#ifdef CONFIG_INPUT
	if (atomic_read(&g_input_lock)) {
		target = __get_freq_by_scene(df->data, SCENE_DEFAULT);
		atomic_set(&g_input_lock, 0);
	} else {
		target = __get_freq_by_scene(df->data, g_scene);
	}
#else
	target = __get_freq_by_scene(df->data, g_scene);
#endif

	if (target == -1) {
		DSM_ERR("can not find target ddr freq\n");
		return -1;
	}

	*freq = target;

	return 0;
}

static int ddr_state_change_task(void *data)
{
	struct devfreq *df = (struct devfreq *)data;

	while (1) {
		if (dsm_enable) {
			mutex_lock(&df->lock);
			update_devfreq(df);
			mutex_unlock(&df->lock);
		}

		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		if (kthread_should_stop())
			break;
		set_current_state(TASK_RUNNING);
	}

	return 0;
}

static ssize_t show_enable(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d\n", dsm_enable);
}

static ssize_t store_enable(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned int value;
	int ret;

	ret = sscanf(buf, "%u", &value);
	if (ret != 1)
		goto out;

	if (value && (!dsm_enable)) {
		dsm_enable = 1;
		wake_up_process(dsm_task);
	} else if ((!value) && dsm_enable) {
		g_scene = SCENE_DEFAULT;
		wake_up_process(dsm_task);
		dsm_enable = 0;
	}

	ret = count;

out:
	return ret;
}

static ssize_t show_scene(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	if (!dsm_enable)
		return sprintf(buf, "unsupported!\n");
	return sprintf(buf, "%d\n", g_scene);
}

static ssize_t store_scene(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned int value;
	int ret;

	if (!dsm_enable)
		return count;

	ret = sscanf(buf, "%u", &value);
	if (ret != 1)
		goto out;

	if (value >= scene_count_used)
		return count;

	g_scene = value;
	DSM_DBG("Scene:%d \"%s\" (pid:%i)\n", g_scene, current->comm, current->pid);

#ifdef CONFIG_EARLYSUSPEND
	if ((g_scene == SCENE_MUSIC) && !atomic_read(&ddrfreq_dsm_suspend_lock)) {
		DSM_DBG("ddrfreq_early_suspend behind\n");
		return count;
	}
#endif

#ifdef CONFIG_INPUT
	if ((g_scene == SCENE_MAIN) && time_before(jiffies, input_expired_time)) {
		DSM_DBG("continuous input event, no freq change\n");
		return count;
	} else {
		wake_up_process(dsm_task);
	}
#else
	wake_up_process(dsm_task);
#endif

	ret = count;

out:
	return ret;
}

static DEVICE_ATTR(enable, 0644, show_enable, store_enable);
static DEVICE_ATTR(scene, 0644, show_scene, store_scene);


static struct attribute *dev_entries[] = {
	&dev_attr_enable.attr,
	&dev_attr_scene.attr,
	NULL,
};

static struct attribute_group dev_attr_group = {
	.name   = "dsm",
	.attrs  = dev_entries,
};

#ifdef CONFIG_EARLYSUSPEND
static void ddrfreq_early_suspend(struct early_suspend *h)
{
	if (!dsm_enable)
		return;

	atomic_set(&ddrfreq_dsm_suspend_lock, 1);

	if (g_scene == SCENE_MUSIC) {
		mutex_lock(&this_devfreq->lock);
		update_devfreq(this_devfreq);
		mutex_unlock(&this_devfreq->lock);
	}
}

static void ddrfreq_late_resume(struct early_suspend *h)
{
	if (!dsm_enable)
		return;

	g_scene = SCENE_DEFAULT;
	atomic_set(&ddrfreq_dsm_suspend_lock, 0);

	mutex_lock(&this_devfreq->lock);
	update_devfreq(this_devfreq);
	mutex_unlock(&this_devfreq->lock);
}

static struct early_suspend ddrfreq_earlysuspend =
{
	.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB + 300,
	.suspend = ddrfreq_early_suspend,
	.resume  = ddrfreq_late_resume,
};
#endif /* CONFIG_EARLYSUSPEND */

#ifdef CONFIG_INPUT
static void do_input_timer(unsigned long data)
{
	if ((g_scene != SCENE_MAIN) && (g_scene != SCENE_MUSIC))
		return;

	wake_up_process(dsm_task);
}

/*
 * trigger ddr frequency to a high speed when input event coming.
 * such as key, ir, touchpannel for ex. , but skip gsensor.
 */
static void ddrfreq_dsm_input_event(struct input_handle *handle,
						unsigned int type,
						unsigned int code, int value)
{
	if (type == EV_SYN && code == SYN_REPORT) {
		atomic_set(&g_input_lock, 1);
		wake_up_process(dsm_task);
		input_expired_time = jiffies + msecs_to_jiffies(2000);
		mod_timer_pinned(&input_timer, input_expired_time);
	}
}

static int ddrfreq_dsm_input_connect(struct input_handler *handler,
						 struct input_dev *dev,
						 const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "ddrfreq_dsm";

	error = input_register_handle(handle);
	if (error)
		goto err;

	error = input_open_device(handle);
	if (error)
		goto err_open;

	return 0;

err_open:
	input_unregister_handle(handle);
err:
	kfree(handle);
	return error;
}

static void ddrfreq_dsm_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id ddrfreq_dsm_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			 INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
				BIT_MASK(ABS_MT_POSITION_X) |
				BIT_MASK(ABS_MT_POSITION_Y) },
	}, /* multi-touch touchscreen */
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
			 INPUT_DEVICE_ID_MATCH_ABSBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
		.absbit = { [BIT_WORD(ABS_X)] =
				BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) },
	}, /* touchpad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			 INPUT_DEVICE_ID_MATCH_BUS   |
			 INPUT_DEVICE_ID_MATCH_VENDOR |
			 INPUT_DEVICE_ID_MATCH_PRODUCT |
			 INPUT_DEVICE_ID_MATCH_VERSION,
		.bustype = BUS_HOST,
		.vendor = 0x0001,
		.product = 0x0001,
		.version = 0x0100,
		.evbit = { BIT_MASK(EV_KEY) },
	}, /* keyboard/ir */
	{ },
};

static struct input_handler ddrfreq_dsm_input_handler = {
	.event          = ddrfreq_dsm_input_event,
	.connect        = ddrfreq_dsm_input_connect,
	.disconnect     = ddrfreq_dsm_input_disconnect,
	.name           = "ddrfreq_dsm",
	.id_table       = ddrfreq_dsm_ids,
};
#endif /* CONFIG_INPUT */

static int __init_scene_freq_table_syscfg(void)
{
	int i, ret = -1;
	char name[16] = {0};
	script_item_u val;
	script_item_value_type_e type;

	type = script_get_item("dram_scene_table", "LV_count", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		goto fail;
	}
	table_length_syscfg = val.val;

	if (table_length_syscfg > (ARRAY_SIZE(ddr_freq_table_syscfg) - 2)) {
		DSM_ERR("ddr_freq_table_syscfg IndexOutOfBoundsException\n");
		goto fail;
	}

	for (i = 1; i <= table_length_syscfg; i++) {
		sprintf(name, "LV%d_scene", i);
		type = script_get_item("dram_scene_table", name, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			return -ENODEV;
		}
		ddr_freq_table_syscfg[i][0] = i >= SCENE_MUSIC ? (i + 1) : val.val;

		sprintf(name, "LV%d_freq", i);
		type = script_get_item("dram_scene_table", name, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			return -ENODEV;
		}
		ddr_freq_table_syscfg[i][1] = val.val / 1000;
	}

	ddr_freq_table_syscfg[0][0] = SCENE_DEFAULT;
	ddr_freq_table_syscfg[0][1] = sunxi_ddrfreq_max;
	ddr_freq_table_syscfg[table_length_syscfg+1][0] = SCENE_MUSIC;
	ddr_freq_table_syscfg[table_length_syscfg+1][1] = sunxi_ddrfreq_min;

	return 0;

fail:
	return ret;
}

static void __scece_freq_table_show(int (*table)[2], int length)
{
	int i;

	pr_debug("---------Dram scene-freq Table--------\n");
	for(i = 0; i < length; i++){
		pr_debug("scene = %4d \tfrequency = %4dKHz\n",
			table[i][0], table[i][1]);
	}
	pr_debug("--------------------------------------\n");
}

static int dsm_init(struct devfreq *devfreq)
{
	int ret = 0;

	if (__init_scene_freq_table_syscfg()) {
		pr_debug("use default config\n");
		ddr_freq_table[0][0] = SCENE_DEFAULT;
		ddr_freq_table[0][1] = sunxi_ddrfreq_max;
		scene_count_used = ARRAY_SIZE(ddr_freq_table);
		ddr_freq_table[scene_count_used-1][0] = SCENE_MUSIC;
		ddr_freq_table[scene_count_used-1][1] = sunxi_ddrfreq_min;
#if defined(CONFIG_ARCH_SUN8IW6P1)
		if (ddr_freq_table[SCENE_MAIN][1] > (sunxi_ddrfreq_max / 2)) {
			ddr_freq_table[SCENE_MAIN][1] = sunxi_ddrfreq_max / 1;
		} else {
			ddr_freq_table[SCENE_MAIN][1] = sunxi_ddrfreq_max / 2;
		}

		if (ddr_freq_table[SCENE_VIDEO][1] > (sunxi_ddrfreq_max / 3)) {
			if (ddr_freq_table[SCENE_VIDEO][1] > (sunxi_ddrfreq_max / 2))
				ddr_freq_table[SCENE_VIDEO][1] = sunxi_ddrfreq_max / 1;
			else
				ddr_freq_table[SCENE_VIDEO][1] = sunxi_ddrfreq_max / 2;
		} else {
			ddr_freq_table[SCENE_VIDEO][1] = sunxi_ddrfreq_max / 3;
		}
#endif
		__scece_freq_table_show(&ddr_freq_table[0], scene_count_used);
		devfreq->data = &ddr_freq_table[0];
	} else {
		pr_debug("use sysconfig\n");
		scene_count_used = table_length_syscfg + 2;
#if defined(CONFIG_ARCH_SUN8IW6P1)
		if (mdfs_in_cfs == 0) {
			if (ddr_freq_table_syscfg[SCENE_MAIN][1] > (sunxi_ddrfreq_max / 2)) {
				ddr_freq_table_syscfg[SCENE_MAIN][1] = sunxi_ddrfreq_max / 1;
			} else {
				ddr_freq_table_syscfg[SCENE_MAIN][1] = sunxi_ddrfreq_max / 2;
			}

			if (ddr_freq_table_syscfg[SCENE_VIDEO][1] > (sunxi_ddrfreq_max / 3)) {
				if (ddr_freq_table_syscfg[SCENE_VIDEO][1] > (sunxi_ddrfreq_max / 2))
					ddr_freq_table_syscfg[SCENE_VIDEO][1] = sunxi_ddrfreq_max / 1;
				else
					ddr_freq_table_syscfg[SCENE_VIDEO][1] = sunxi_ddrfreq_max / 2;
			} else {
					ddr_freq_table_syscfg[SCENE_VIDEO][1] = sunxi_ddrfreq_max / 3;
			}
		}
#endif
		__scece_freq_table_show(&ddr_freq_table_syscfg[0], scene_count_used);
		devfreq->data = &ddr_freq_table_syscfg[0];
	}

	this_devfreq = devfreq;

#ifdef CONFIG_EARLYSUSPEND
	register_early_suspend(&ddrfreq_earlysuspend);
#endif

	ret = sysfs_create_group(&devfreq->dev.kobj, &dev_attr_group);
	if (ret)
		return ret;

	/* create task */
	dsm_task = kthread_create(ddr_state_change_task, this_devfreq, "kdsm");
	if (IS_ERR(dsm_task)) {
		return PTR_ERR(dsm_task);
	}

	get_task_struct(dsm_task);

#ifdef CONFIG_INPUT
	ret = input_register_handler(&ddrfreq_dsm_input_handler);
	if (ret)
		return ret;

	/* init input event timer */
	init_timer_deferrable(&input_timer);
	input_timer.function = do_input_timer;
	input_timer.expires = jiffies + msecs_to_jiffies(2000);
	add_timer_on(&input_timer, 0);
#endif

	wake_up_process(dsm_task);

	return ret;
}

static void dsm_exit(struct devfreq *devfreq)
{
#ifdef CONFIG_INPUT
	input_unregister_handler(&ddrfreq_dsm_input_handler);
#endif

	kthread_stop(dsm_task);
	put_task_struct(dsm_task);
	sysfs_remove_group(&devfreq->dev.kobj, &dev_attr_group);
	this_devfreq = NULL;
#ifdef CONFIG_EARLYSUSPEND
	unregister_early_suspend(&ddrfreq_earlysuspend);
#endif
}

const struct devfreq_governor devfreq_dsm = {
	.name = "dsm",
	.init = dsm_init,
	.exit = dsm_exit,
	.get_target_freq = devfreq_dsm_func,
	.no_central_polling = true,
};

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("'governor_dsm' - ddr scene manager policy");
MODULE_AUTHOR("pannan");

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2023 - 2028 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Critical temperature handler for Allwinner SOC
 *
 * Copyright (c) 2023 Allwinnertech Ltd.
 */
#include <sunxi-log.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/suspend.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/reboot.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <sunxi-sip.h>

#include "sunxi_thermal.h"
#include "sunxi_critical_handler.h"

#define DEFAULT_SUSPEND_HANDLER_THRESHOLD	(0xffffffff)

struct ths_critical_suspend {
	struct work_struct	suspend_work;
	struct mutex		suspend_lock;
	uint32_t 		critical_handling;
	uint32_t 		suspend_with_alarm;
	uint32_t		backup_cnt;
	uint32_t		suspend_retry_times;
	uint32_t		suspend_handler_threshold;
	uint32_t		suspend_with_handler;
	char	 		alarm_path[64];
	char 			alarm_sec[16];
};

static struct ths_critical_suspend *critical_suspend;

static void sunxi_ths_critical_backup(void)
{
	int poweroff_delay_ms = CONFIG_THERMAL_EMERGENCY_POWEROFF_DELAY_MS;

	sunxi_err(NULL, "%s(%d): critical temperature reached, "
		"shutting down by backup handler\n",
		__func__, __LINE__);

	hw_protection_shutdown("Temperature too high", poweroff_delay_ms);

	while (1) {
		msleep(1000);
	}
}

static void sunxi_ths_critical_suspend_work(struct work_struct *work)
{
	struct file *filp;
	loff_t pos = 0;
	char *clear_alarm = "0";
	int clear_len = strlen(clear_alarm);
	int ret;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	mm_segment_t old_fs;

	if (critical_susend->suspend_with_alarm) {
		filp = filp_open(critical_suspend->alarm_path, O_RDWR, 0);
		if (!IS_ERR(filp)) {
			old_fs = get_fs();
			set_fs(KERNEL_DS);
			ret = vfs_write(filp, clear_alarm, clear_len, &pos);
			if (ret < 0)
				sunxi_warn(NULL, "%s(%d): Error clearing rtc alarm\n",
					__func__, __LINE__);

			if (critical_suspend->backup_cnt \
				< critical_suspend->suspend_retry_times) {
				ret = vfs_write(filp, critical_suspend->alarm_sec,
					strlen(critical_suspend->alarm_sec), &pos);
				if (ret < 0)
					sunxi_warn(NULL, "%s(%d): Error writing rtc alarm"
						"suspend without alarm\n",
						__func__, __LINE__);
			}
			set_fs(old_fs);
			filp_close(filp, NULL);
		} else {
			sunxi_warn(NULL, " %s(%d): rtc alarm file_open failed"
				"suspend without alarm\n",
				__func__, __LINE__);
		}
	}
#else
	if (critical_suspend->suspend_with_alarm) {
		filp = filp_open(critical_suspend->alarm_path, O_RDWR, 0);
		if (!IS_ERR(filp)) {
			ret = kernel_write(filp, clear_alarm, clear_len, &pos);
			if (ret < 0)
				sunxi_warn(NULL, "%s(%d): Error writing rtc alarm\n",
					__func__, __LINE__);

			if (critical_suspend->backup_cnt \
				< critical_suspend->suspend_retry_times) {
				ret = kernel_write(filp, critical_suspend->alarm_sec,
					strlen(critical_suspend->alarm_sec), &pos);
				if (ret < 0)
					sunxi_warn(NULL, "%s(%d): Error writing rtc alarm"
						"suspend without alarm\n",
						__func__, __LINE__);
			}
			filp_close(filp, NULL);
		} else {
			sunxi_warn(NULL, "%s(%d): rtc alarm file_open failed"
				"suspend without alarm\n",
				__func__, __LINE__);
		}
	}
#endif

	if (critical_suspend->backup_cnt \
		>= critical_suspend->suspend_retry_times) {
		sunxi_ths_critical_backup();
		/* alarm could trigger poweron */
	}

	ret = pm_suspend(PM_SUSPEND_MEM);
	if (ret) {
		critical_suspend->backup_cnt += 1;
		sunxi_warn(NULL, "%s(%d): critical suspend trigger failed, return: %d\n",
			__func__, __LINE__, ret);
	} else {
		critical_suspend->backup_cnt = 0;
	}

	mutex_lock(&critical_suspend->suspend_lock);
	critical_suspend->critical_handling = 0;
	mutex_unlock(&critical_suspend->suspend_lock);
}

static int sunxi_ths_critical_suspend_init(struct device *dev)
{
	int ret;
	uint32_t alarm_second;
	struct device_node *np = dev->of_node;
	const char *path;

	if (!critical_suspend) {
		critical_suspend = kzalloc(sizeof(*critical_suspend), GFP_KERNEL);
		if (!critical_suspend) {
			sunxi_err(NULL, "%s(%d): critical suspend alloc failed\n",
				__func__, __LINE__);
			return -ENOMEM;
		}
	} else {
		sunxi_err(NULL, "%s(%d): Error critical suspend init\n",
			__func__, __LINE__);
			return -EEXIST;
	}

	if (of_get_property(np, "suspend-with-alarm", NULL)) {
		critical_suspend->suspend_with_alarm = 1;

		ret = of_property_read_string(np, "alarm-path", &path);
		if (ret) {
			sunxi_err(NULL, "%s(%d): get alarm_path failed\n",
				__func__, __LINE__);
			critical_suspend->suspend_with_alarm = 0;
		} else {
			ret = snprintf(critical_suspend->alarm_path, 64,
				"%s", path);
			if (ret < 0) {
				sunxi_err(NULL, "%s(%d): store alarm_path failed\n",
					__func__, __LINE__);
				critical_suspend->suspend_with_alarm = 0;
			}

		}

		ret = of_property_read_u32(np, "alarm-second", &alarm_second);
		if (ret) {
			sunxi_err(NULL, "%s(%d): get alarm_sec failed\n",
				__func__, __LINE__);
			critical_suspend->suspend_with_alarm = 0;
		} else {
			ret = sprintf(critical_suspend->alarm_sec,
				"+%d", alarm_second);
			if (ret < 0) {
				sunxi_err(NULL, "%s(%d): change alarm_sec failed\n",
					__func__, __LINE__);
				critical_suspend->suspend_with_alarm = 0;
			}
		}
	}


	ret = of_property_read_u32(np, "suspend-retry-times", &critical_suspend->suspend_retry_times);
	if (ret) {
		sunxi_warn(NULL, "%s(%d): get suspend retry times failed, set times to 0\n",
			__func__, __LINE__);
		critical_suspend->suspend_retry_times = 0;
	}

	ret = of_property_read_u32(np, "suspend-handler-threshold", &critical_suspend->suspend_handler_threshold);
	if (ret) {
		sunxi_warn(NULL, "%s(%d): get suspend-handler-threshold failed, set default threshold\n",
			__func__, __LINE__);
		critical_suspend->suspend_handler_threshold = DEFAULT_SUSPEND_HANDLER_THRESHOLD;
	}

	critical_suspend->backup_cnt = 0;

	mutex_init(&critical_suspend->suspend_lock);
	INIT_WORK(&critical_suspend->suspend_work, sunxi_ths_critical_suspend_work);

	return 0;
}

static void sunxi_ths_critical_suspend_deinit(void)
{
	mutex_destroy(&critical_suspend->suspend_lock);
	kfree(critical_suspend);
}

#if IS_ENABLED(CONFIG_AW_THERMAL_REWRITE_CRITICAL_OPS)
static void sunxi_ths_critical_suspend(struct thermal_zone_device *tzd)
{
	if (mutex_trylock(&critical_suspend->suspend_lock)) {
		if (!critical_suspend->critical_handling) {
			pr_err(" %s: critical temperature reached, "
				"try to suspend\n", tzd->type);
			critical_suspend->critical_handling = 1;
			schedule_work(&critical_suspend->suspend_work);
		}
		mutex_unlock(&critical_suspend->suspend_lock);
	}
}

static void sunxi_ths_critical_notify(struct thermal_zone_device *tzd)
{
	sunxi_ths_critical_suspend(tzd);
}

void sunxi_ths_critical_rewrite_ops(struct ths_device *tmdev)
{
	int i;
	int count;
	int trips_count;
	enum thermal_trip_type type;
	struct thermal_zone_device *tzd;

	if (!tmdev || !tmdev->chip) {
		pr_err("%s(%d), invalid ths device\n", __func__, __LINE__);
		return;
	}

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		tzd = tmdev->sensor[i].tzd;
#if IS_ENABLED(CONFIG_AW_KERNEL_AOSP)
		trips_count = tzd->trips;
#else
		trips_count = tzd->num_trips;
#endif
		for (count = 0; count < trips_count; count++) {
			tzd->ops->get_trip_type(tzd, count, &type);
			if (type == THERMAL_TRIP_CRITICAL) {
				tzd->ops->critical = sunxi_ths_critical_notify;
			}
		}
	}
}
#endif

void sunxi_ths_critical_suspend_handler(struct ths_device *tmdev)
{
	int i;
	int last_temp;

	if (critical_suspend->suspend_handler_threshold == DEFAULT_SUSPEND_HANDLER_THRESHOLD)
		return;


	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		if (IS_ENABLED(CONFIG_THERMAL_EMULATION) && tmdev->sensor[i].tzd->emul_temperature)
			last_temp = tmdev->sensor[i].tzd->emul_temperature;
		else
			last_temp = tmdev->sensor[i].last_temp;

		if (last_temp >= critical_suspend->suspend_handler_threshold) {
			critical_suspend->suspend_with_handler = 1;
			sunxi_warn(&tmdev->sensor[i].tzd->device,
				"%s: temperature(%d) reaches threshold(%d), request dram to keep running\n",
				tmdev->sensor[i].tzd->type, last_temp,
				critical_suspend->suspend_handler_threshold);
		}
	}

	if (critical_suspend->suspend_with_handler)
		invoke_scp_fn_smc(SET_DRAM_KEEP_RUNNING, 0, 0, 0);
}

void sunxi_ths_critical_resume_handler(struct ths_device *tmdev)
{
	if (critical_suspend->suspend_handler_threshold == DEFAULT_SUSPEND_HANDLER_THRESHOLD)
		return;

	if (critical_suspend->suspend_with_handler) {
		invoke_scp_fn_smc(CLEAR_DRAM_KEEP_RUNNING, 0, 0, 0);
		critical_suspend->suspend_with_handler = 0;
	}
}

int sunxi_ths_critical_handler_init(struct device *dev, struct ths_device *tmdev)
{
	int i;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		tmdev->sensor[i].last_temp = 0;
	}

	return sunxi_ths_critical_suspend_init(dev);
}

void sunxi_ths_critical_handler_deinit(void)
{
	sunxi_ths_critical_suspend_deinit();
}

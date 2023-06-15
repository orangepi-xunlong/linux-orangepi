/* Vibrator driver for sunxi platform
 * ported from msm pmic vibrator driver
 *  by tom cubie <tangliang@reuuimllatech.com>
 *
 * Copyright (C) 2011 ReuuiMlla Technology.
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include "../init-input.h"
#include "../../staging/android/timed_output.h"

static struct work_struct vibrator_work;
static struct hrtimer vibe_timer;
static int vibe_state;
struct motor_config_info motor_info = {
	.input_type = MOTOR_TYPE,
	.motor_gpio.gpio = 0,
};

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_DATA_INFO = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
};
static u32 debug_mask;

#define dprintk(level_mask, fmt, arg...)    do {\
	if (unlikely(debug_mask & level_mask)) \
		printk(KERN_DEBUG fmt, ## arg); \
} while (0)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
static struct mutex lock;

static void set_sunxi_vibrator(int on)
{
	dprintk(DEBUG_DATA_INFO, "sunxi_vibrator on %d\n", on);

	//if (0 != motor_info.motor_gpio.gpio) {
	//	if(on) {
	//		__gpio_set_value(motor_info.motor_gpio.gpio, !motor_info.vibe_off);
	//	} else {
	//		__gpio_set_value(motor_info.motor_gpio.gpio, motor_info.vibe_off);
	//	}
	//} else {
	if (on) {
		if (regulator_is_enabled(motor_info.motor_power_ldo) == 0)
			input_set_power_enable(&(motor_info.input_type), 1);
	} else {
		if (regulator_is_enabled(motor_info.motor_power_ldo))
			input_set_power_enable(&(motor_info.input_type), 0);
	}
	//}
}

static void update_vibrator(struct work_struct *work)
{
	set_sunxi_vibrator(vibe_state);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	mutex_lock(&lock);
	if (vibe_state == 1) {
		dprintk(DEBUG_DATA_INFO, "cancle vibe_timer");
		vibe_state = 0;
		set_sunxi_vibrator(vibe_state);
	}

	hrtimer_cancel(&vibe_timer);
	dprintk(DEBUG_DATA_INFO, "sunxi_vibrator enable %d\n", value);
	dprintk(DEBUG_DATA_INFO, "vibe_state = %d", vibe_state);
	if (value <= 0)
		vibe_state = 0;
	else {
		value = (value > 15000 ? 15000 : value);
		vibe_state = 1;
		hrtimer_start(&vibe_timer,
				ktime_set(value / 1000, (value % 1000) * 1000000),
				HRTIMER_MODE_REL);
	}
	//set_sunxi_vibrator(vibe_state)
	mutex_unlock(&lock);
	schedule_work(&vibrator_work);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	struct timespec time_tmp;
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		time_tmp = ktime_to_timespec(r);
		//return r.tv.sec * 1000 + r.tv.nsec/1000000;
		return time_tmp.tv_sec* 1000 + time_tmp.tv_nsec/1000000;
	} else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	vibe_state = 0;
	schedule_work(&vibrator_work);
	dprintk(DEBUG_DATA_INFO, "sunxi_vibrator timer expired\n");
	return HRTIMER_NORESTART;
}

static struct timed_output_dev sunxi_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

static int __init sunxi_vibrator_init(void)
{
	if (input_sensor_startup(&(motor_info.input_type))) {
		printk("%s: motor_fetch_sysconfig_para err.\n", __func__);
		goto exit;
	}
	if (motor_info.motor_used == 0) {
		printk("*** motor_used set to 0 !\n");
		goto exit;
	}

	//ret = input_init_platform_resource(&(motor_info.input_type));
    //    if(0 != ret) {
	//	printk("%s:motor_ops.init_platform_resource err. \n", __func__);
	//	goto exit;
	//}

	if (input_sensor_init(&(motor_info.input_type))) {
		printk("*** motor init power failed!!!\n");
		goto exit;
	}

	mutex_init(&lock);
	INIT_WORK(&vibrator_work, update_vibrator);

	vibe_state = 0;
	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&sunxi_vibrator);

	dprintk(DEBUG_INIT, "sunxi_vibrator init end\n");

	return 0;
exit:
	return -1;
}

static void __exit sunxi_vibrator_exit(void)
{
	dprintk(DEBUG_INIT, "bye, sunxi_vibrator_exit\n");
	mutex_destroy(&lock);
	timed_output_dev_unregister(&sunxi_vibrator);
	//input_free_platform_resource(&(motor_info.input_type));
}
module_init(sunxi_vibrator_init);
module_exit(sunxi_vibrator_exit);

/* Module information */
MODULE_DESCRIPTION("timed output vibrator device for sunxi");
MODULE_LICENSE("GPL");

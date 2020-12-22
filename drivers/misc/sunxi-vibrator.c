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
#include <linux/init-input.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/regulator/consumer.h>
#include "../drivers/staging/android/timed_output.h"

static struct work_struct vibrator_work;
static struct hrtimer vibe_timer;
static spinlock_t vibe_lock;
static int vibe_state;
static struct regulator *ldo = NULL;
struct motor_config_info motor_info = {
	.input_type = MOTOR_TYPE,
	.motor_gpio.gpio = 0,
};

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_DATA_INFO = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
};
static u32 debug_mask = 0xff;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static void set_sunxi_vibrator(int on)
{
	dprintk(DEBUG_DATA_INFO, "sunxi_vibrator on %d\n", on);

	if (0 != motor_info.motor_gpio.gpio) {
		if(on) {
			__gpio_set_value(motor_info.motor_gpio.gpio, !motor_info.vibe_off);
		} else {
			__gpio_set_value(motor_info.motor_gpio.gpio, motor_info.vibe_off);
		}
	} else {
		if (motor_info.ldo) {
			if(on) {
				regulator_enable(ldo);
			} else {
				if (regulator_is_enabled(ldo))
					regulator_disable(ldo);
			}
		}
	}
}

static void update_vibrator(struct work_struct *work)
{
	set_sunxi_vibrator(vibe_state);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	unsigned long	flags;

	spin_lock_irqsave(&vibe_lock, flags);
	hrtimer_cancel(&vibe_timer);

	dprintk(DEBUG_DATA_INFO, "sunxi_vibrator enable %d\n", value);

	if (value <= 0)
		vibe_state = 0;
	else {
		value = (value > 15000 ? 15000 : value);
		vibe_state = 1;
		hrtimer_start(&vibe_timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&vibe_lock, flags);

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
	int ret = 0;

	if (input_fetch_sysconfig_para(&(motor_info.input_type))) {
		printk("%s: motor_fetch_sysconfig_para err.\n", __func__);
		goto exit;
	}
	if (motor_info.motor_used == 0) {
		printk("*** motor_used set to 0 !\n");
		goto exit;
	}

	ret = input_init_platform_resource(&(motor_info.input_type));
        if(0 != ret) {
		printk("%s:motor_ops.init_platform_resource err. \n", __func__);
		goto exit;
	}

        if (motor_info.ldo) {
		ldo = regulator_get(NULL, motor_info.ldo);
		if (!ldo) {
			pr_err("%s: could not get moto ldo '%s' in probe, maybe config error,"
			"ignore firstly !!!!!!!\n", __func__, motor_info.ldo);
			goto exit;
		}
		regulator_set_voltage(ldo, (int)(motor_info.ldo_voltage)*1000,
                                        (int)(motor_info.ldo_voltage)*1000);
	}

	INIT_WORK(&vibrator_work, update_vibrator);

	spin_lock_init(&vibe_lock);
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
	timed_output_dev_unregister(&sunxi_vibrator);
	input_free_platform_resource(&(motor_info.input_type));	
}
module_init(sunxi_vibrator_init);
module_exit(sunxi_vibrator_exit);

/* Module information */
MODULE_DESCRIPTION("timed output vibrator device for sunxi");
MODULE_LICENSE("GPL");


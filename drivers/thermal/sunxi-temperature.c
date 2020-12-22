/*
 * drivers/thermal/sunxi-temperature.c
 *
 * Copyright (C) 2013-2014 allwinner.
 *	Li Ming<liming@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/init-input.h>
#include <linux/thermal.h>
#include <linux/clk.h>
#include <mach/sunxi-smc.h>
#if defined(CONFIG_ARCH_SUN9IW1P1)
#include <linux/clk/clk-sun9iw1.h>
#elif defined(CONFIG_ARCH_SUN8IW7P1)
#include <linux/clk/clk-sun8iw7.h>
#endif
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif
#include "sunxi-thermal.h"
#include "sunxi-temperature.h"

#ifdef CONFIG_SUNXI_BUDGET_COOLING
	#define SUNXI_THERMAL_COOLING_DEVICE_NAMER    "thermal-budget-0"
#else
	#define SUNXI_THERMAL_COOLING_DEVICE_NAMER    "thermal-cpufreq-0"
#endif
static u32 debug_mask = 0x0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(fmt , ## arg)

struct sunxi_ths_data {
	void __iomem *base_addr;
	int circle_num;
	int temp_data1[10];
	int temp_data2[10];
	int temp_data3[10];
	int temp_data4[10];
	int thermal_data[6];
	atomic_t delay;
	struct delayed_work work;
	struct work_struct  irq_work;

	int irq_used;

	struct input_dev *ths_input_dev;
	atomic_t input_delay;
	atomic_t input_enable;
	struct delayed_work input_work;
	struct mutex input_enable_mutex;

#ifdef CONFIG_PM
struct dev_pm_domain ths_pm_domain;
#endif
};
static struct sunxi_ths_data *thermal_data;
static int temperature[6] = {5, 5, 5, 5, 5, 5};
static DEFINE_SPINLOCK(data_lock);
static struct ths_config_info ths_info = {
	.input_type = THS_TYPE,
};

static void sunxi_ths_reg_clear(void);
static void sunxi_ths_reg_init(void);

#if defined(CONFIG_ARCH_SUN9IW1P1)
static void sunxi_ths_clk_cfg(void);
static struct clk *gpadc_clk;
static struct clk *gpadc_clk_source;
#elif defined(CONFIG_ARCH_SUN8IW7P1)
static struct clk *ths_clk;
static struct clk *ths_clk_source;
#endif

/********************************** input *****************************************/

static ssize_t sunxi_ths_input_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	dprintk(DEBUG_CONTROL_INFO, "%d, %s\n", atomic_read(&thermal_data->input_delay), __FUNCTION__);
	return sprintf(buf, "%d\n", atomic_read(&thermal_data->input_delay));

}

static ssize_t sunxi_ths_input_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (data > THERMAL_DATA_DELAY)
		data = THERMAL_DATA_DELAY;
	atomic_set(&thermal_data->input_delay, (unsigned int) data);

	return count;
}


static ssize_t sunxi_ths_input_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	dprintk(DEBUG_CONTROL_INFO, "%d, %s\n", atomic_read(&thermal_data->input_enable), __FUNCTION__);
	return sprintf(buf, "%d\n", atomic_read(&thermal_data->input_enable));
}

static void sunxi_ths_input_set_enable(struct device *dev, int enable)
{
	int pre_enable = atomic_read(&thermal_data->input_enable);

	mutex_lock(&thermal_data->input_enable_mutex);
	if (enable) {
		if (pre_enable == 0) {
			schedule_delayed_work(&thermal_data->input_work,
				msecs_to_jiffies(atomic_read(&thermal_data->input_delay)));
			atomic_set(&thermal_data->input_enable, 1);
		}

	} else {
		if (pre_enable == 1) {
			cancel_delayed_work_sync(&thermal_data->input_work);
			atomic_set(&thermal_data->input_enable, 0);
		}
	}
	mutex_unlock(&thermal_data->input_enable_mutex);
}

static ssize_t sunxi_ths_input_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if ((data == 0)||(data==1)) {
		sunxi_ths_input_set_enable(dev,data);
	}

	return count;
}

static ssize_t sunxi_ths_show_temp(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t cnt = 0;
	int i = 0;
	printk("%s: enter \n", __func__);

	for (i=0; i<6; i++) {
		cnt += sprintf(buf + cnt,"temperature[%d]:%d\n" , i, ths_read_data(i));
	}

	return cnt;
}

static ssize_t sunxi_ths_set_temp(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned int data1, data2;
	unsigned long irqflags;
	char *endp;

	cancel_delayed_work_sync(&thermal_data->work);

	data1 = simple_strtoul(buf, &endp, 10);
	if (*endp != ' ') {
		printk("%s: %d\n", __func__, __LINE__);
		return -EINVAL;
	}
	buf = endp + 1;

	data2 = simple_strtoul(buf, &endp, 10);

	spin_lock_irqsave(&data_lock, irqflags);
	temperature[data1] = data2;
	spin_unlock_irqrestore(&data_lock, irqflags);

	printk("temperature[%d] = %d\n", data1, data2);

	return count;
}


static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
		sunxi_ths_input_delay_show, sunxi_ths_input_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
		sunxi_ths_input_enable_show, sunxi_ths_input_enable_store);
static DEVICE_ATTR(temperature, S_IRUGO|S_IWUSR|S_IWGRP,
		sunxi_ths_show_temp, sunxi_ths_set_temp);

static struct attribute *sunxi_ths_input_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_temperature.attr,
	NULL
};

static struct attribute_group sunxi_ths_input_attribute_group = {
	.attrs = sunxi_ths_input_attributes
};

static void sunxi_ths_input_work_func(struct work_struct *work)
{
	static int tempetature = 5;
	struct sunxi_ths_data *data = container_of((struct delayed_work *)work,
			struct sunxi_ths_data, input_work);
	unsigned long delay = msecs_to_jiffies(atomic_read(&data->input_delay));

	tempetature = ths_read_data(4);
	input_report_abs(data->ths_input_dev, ABS_MISC, tempetature);
	input_sync(data->ths_input_dev);
	dprintk(DEBUG_DATA_INFO, "%s: temperature %d,\n", __func__, tempetature);

	schedule_delayed_work(&data->input_work, delay);
}


static int sunxi_ths_input_init(struct sunxi_ths_data *data)
{
	int err = 0;

	data->ths_input_dev = input_allocate_device();
	if (IS_ERR_OR_NULL(data->ths_input_dev)) {
		printk(KERN_ERR "temp_dev: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	data->ths_input_dev->name = "sunxi-ths";
	data->ths_input_dev->phys = "sunxiths/input0";
	data->ths_input_dev->id.bustype = BUS_HOST;
	data->ths_input_dev->id.vendor = 0x0001;
	data->ths_input_dev->id.product = 0x0001;
	data->ths_input_dev->id.version = 0x0100;

	input_set_capability(data->ths_input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(data->ths_input_dev, ABS_MISC, -50, 180, 0, 0);

	err = input_register_device(data->ths_input_dev);
	if (0 < err) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(data->ths_input_dev);
		goto fail2;
	}

	INIT_DELAYED_WORK(&data->input_work, sunxi_ths_input_work_func);

	mutex_init(&data->input_enable_mutex);
	atomic_set(&data->input_enable, 0);
	atomic_set(&data->input_delay, THERMAL_DATA_DELAY);

	err = sysfs_create_group(&data->ths_input_dev->dev.kobj,
						 &sunxi_ths_input_attribute_group);
	if (err < 0)
	{
		pr_err("%s: sysfs_create_group err\n", __func__);
		goto fail3;
	}

	return err;
fail3:
	input_unregister_device(data->ths_input_dev);
fail2:
	kfree(data->ths_input_dev);
fail1:
	return err;

}

static void sunxi_ths_input_exit(struct sunxi_ths_data *data)
{
	sysfs_remove_group(&data->ths_input_dev->dev.kobj, &sunxi_ths_input_attribute_group);
	input_unregister_device(data->ths_input_dev);
}

/********************************** input end *****************************************/


static void ths_write_data(int index, struct sunxi_ths_data *data)
{
	unsigned long irqflags;

	spin_lock_irqsave(&data_lock, irqflags);
	temperature[index] = data->thermal_data[index];
	spin_unlock_irqrestore(&data_lock, irqflags);

	return;
}

int ths_read_data(int index)
{
	unsigned long irqflags;
	int data;

	spin_lock_irqsave(&data_lock, irqflags);
	data = temperature[index];
	spin_unlock_irqrestore(&data_lock, irqflags);

	return data;
}
EXPORT_SYMBOL(ths_read_data);

static void calc_temperature(int value, int divisor, int minus, struct sunxi_ths_data *data)
{
	unsigned int i = 0;
	int64_t avg_temp[4] = {0, 0, 0, 0};

	if (data->circle_num % 5 || !data->circle_num)
		return;
	if (value > 0) {
		for (i = 0; i < data->circle_num; i++) {
			avg_temp[0] += data->temp_data1[i];
		}
	}
	if (value > 1) {
		for (i = 0; i < data->circle_num; i++) {
			avg_temp[1] += data->temp_data2[i];
		}
	}
	if (value > 2) {
		for (i = 0; i < data->circle_num; i++) {
			avg_temp[2] += data->temp_data3[i];
		}
	}
	if (value > 3) {
		for (i = 0; i < data->circle_num; i++) {
			avg_temp[3] += data->temp_data4[i];
		}
	}

	for (i = 0; i < value; i++) {
		do_div(avg_temp[i], data->circle_num);
		dprintk(DEBUG_DATA_INFO, "avg_temp_reg=%lld\n", avg_temp[i]);

#if defined(CONFIG_ARCH_SUN9IW1P1) || defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW7P1)
		avg_temp[i] *= 1000;
		if (0 != avg_temp[i])
			do_div(avg_temp[i], divisor);

		avg_temp[i]= minus-avg_temp[i];
#else
		avg_temp[i] *= 100;
		if (0 != avg_temp[i])
			do_div(avg_temp[i], divisor);

		avg_temp[i] -= minus;
#endif
		dprintk(DEBUG_DATA_INFO, "avg_temp=%lld C\n", avg_temp[i]);

		data->thermal_data[i] = avg_temp[i];
	}

	data->circle_num = 0;
	return;
}

#ifdef CONFIG_PM
static int sunxi_ths_suspend(struct device *dev)
{
	dprintk(DEBUG_SUSPEND, "%s: suspend\n", __func__);
	if (NORMAL_STANDBY == standby_type) {
		mutex_lock(&thermal_data->input_enable_mutex);
		if (atomic_read(&thermal_data->input_enable)==1) {
			cancel_delayed_work_sync(&thermal_data->input_work);
		}
		mutex_unlock(&thermal_data->input_enable_mutex);

		if (thermal_data->irq_used)
			disable_irq_nosync(THS_IRQNO);
		cancel_delayed_work_sync(&thermal_data->work);
		sunxi_ths_reg_clear();
	} else if (SUPER_STANDBY == standby_type) {
		mutex_lock(&thermal_data->input_enable_mutex);
		if (atomic_read(&thermal_data->input_enable)==1) {
			cancel_delayed_work_sync(&thermal_data->input_work);
		}
		mutex_unlock(&thermal_data->input_enable_mutex);

		if (thermal_data->irq_used)
			disable_irq_nosync(THS_IRQNO);
		cancel_delayed_work_sync(&thermal_data->work);
		sunxi_ths_reg_clear();
	}
#if defined(CONFIG_ARCH_SUN9IW1P1)
	clk_disable_unprepare(gpadc_clk);
#elif defined(CONFIG_ARCH_SUN8IW7P1)
	clk_disable_unprepare(ths_clk);
#endif
	return 0;
}

static int sunxi_ths_resume(struct device *dev)
{
	dprintk(DEBUG_SUSPEND, "%s: resume\n", __func__);

#if defined(CONFIG_ARCH_SUN9IW1P1)
	clk_prepare_enable(gpadc_clk);
#elif defined(CONFIG_ARCH_SUN8IW7P1)
	clk_prepare_enable(ths_clk);
#endif

	if (NORMAL_STANDBY == standby_type) {
		sunxi_ths_reg_init();
		if (thermal_data->irq_used)
			enable_irq(THS_IRQNO);
		schedule_delayed_work(&thermal_data->work,
				msecs_to_jiffies(atomic_read(&thermal_data->delay)));

		mutex_lock(&thermal_data->input_enable_mutex);
		if (atomic_read(&thermal_data->input_enable)==1) {
			schedule_delayed_work(&thermal_data->input_work,
				msecs_to_jiffies(atomic_read(&thermal_data->input_delay)));
		}
		mutex_unlock(&thermal_data->input_enable_mutex);
	} else if (SUPER_STANDBY == standby_type) {
		sunxi_ths_reg_init();
		if (thermal_data->irq_used)
			enable_irq(THS_IRQNO);
		schedule_delayed_work(&thermal_data->work,
				msecs_to_jiffies(atomic_read(&thermal_data->delay)));

		mutex_lock(&thermal_data->input_enable_mutex);
		if (atomic_read(&thermal_data->input_enable)==1) {
			schedule_delayed_work(&thermal_data->input_work,
				msecs_to_jiffies(atomic_read(&thermal_data->input_delay)));
		}
		mutex_unlock(&thermal_data->input_enable_mutex);
	}
	return 0;
}
#endif

#ifdef CONFIG_ARCH_SUN8IW3P1
static void sunxi_ths_work_func(struct work_struct *work)
{
	struct sunxi_ths_data *data = container_of((struct delayed_work *)work,
			struct sunxi_ths_data, work);
	unsigned long delay = msecs_to_jiffies(atomic_read(&data->delay));

	data->temp_data1[data->circle_num] = readl(data->base_addr + THS_DATA_REG);
	dprintk(DEBUG_DATA_INFO, "THS data = %d\n", data->temp_data1[data->circle_num]);

	if (1000 < data->temp_data1[data->circle_num])
		data->circle_num++;
	calc_temperature(1, 625, 265, data);

	data->thermal_data[4] = data->thermal_data[0];
	ths_write_data(0, data);
	ths_write_data(4, data);

	schedule_delayed_work(&data->work, delay);
}
static void sunxi_ths_reg_clear(void)
{
	writel(0, thermal_data->base_addr + THS_CTRL_REG1);
	writel(0, thermal_data->base_addr + THS_PRO_CTRL_REG);
}

static void sunxi_ths_reg_init(void)
{
	writel(THS_CTRL_REG0_VALUE, thermal_data->base_addr + THS_CTRL_REG0);
	writel(THS_CTRL_REG1_VALUE, thermal_data->base_addr + THS_CTRL_REG1);
	writel(THS_PRO_CTRL_REG_VALUE, thermal_data->base_addr + THS_PRO_CTRL_REG);

	dprintk(DEBUG_INIT, "THS_CTRL_REG0 = 0x%x\n", readl(thermal_data->base_addr + THS_CTRL_REG0));
	dprintk(DEBUG_INIT, "THS_CTRL_REG1 = 0x%x\n", readl(thermal_data->base_addr + THS_CTRL_REG1));
	dprintk(DEBUG_INIT, "THS_PRO_CTRL_REG = 0x%x\n", readl(thermal_data->base_addr + THS_PRO_CTRL_REG));
}

static struct thermal_trip_point_conf sunxi_trip_data;

static struct thermal_cooling_conf sunxi_cooling_data;

static struct thermal_sensor_conf sunxi_sensor_conf = {
	.name			= SUNXI_THERMAL_COOLING_DEVICE_NAMER,
	.read_temperature	= ths_read_data,
	.trip_data		= &sunxi_trip_data,
	.cooling_data		= &sunxi_cooling_data,
};

static struct sunxi_thermal_zone ths_zone = {
	.id			= 0,
	.name			= "sunxi-therm",
	.sunxi_ths_sensor_conf	= &sunxi_sensor_conf,
};

static int __init sunxi_ths_init(void)
{
	int err = 0;

	dprintk(DEBUG_INIT, "%s: enter!\n", __func__);

	if (input_fetch_sysconfig_para(&(ths_info.input_type))) {
		pr_err("%s: err.\n", __func__);
		return -EPERM;
	}

	sunxi_trip_data.trip_count = ths_info.trip1_count;
	sunxi_trip_data.trip_val[0] = ths_info.trip1_0;
	sunxi_trip_data.trip_val[1] = ths_info.trip1_1;
	sunxi_trip_data.trip_val[2] = ths_info.trip1_2;

	sunxi_cooling_data.freq_clip_count = ths_info.trip1_count-1;
	sunxi_cooling_data.freq_data[0].freq_clip_min = ths_info.trip1_0_min;
	sunxi_cooling_data.freq_data[0].freq_clip_max = ths_info.trip1_0_max;
	sunxi_cooling_data.freq_data[0].temp_level = ths_info.trip1_0;
	sunxi_cooling_data.freq_data[1].freq_clip_min = ths_info.trip1_1_min;
	sunxi_cooling_data.freq_data[1].freq_clip_max = ths_info.trip1_1_max;
	sunxi_cooling_data.freq_data[1].temp_level = ths_info.trip1_1;

	thermal_data = kzalloc(sizeof(*thermal_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(thermal_data)) {
		pr_err("thermal_data: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxi_ths_input_init(thermal_data);
	err = sunxi_ths_register_thermal(&ths_zone);
	if(err < 0) {
		pr_err("therma: register thermal core failed\n");
		goto fail2;
	}

	thermal_data->base_addr = (void __iomem *)THERMAL_BASSADDRESS;
	sunxi_ths_reg_init();
	thermal_data->irq_used = 0;

#ifdef CONFIG_PM
	thermal_data->ths_pm_domain.ops.suspend = sunxi_ths_suspend;
	thermal_data->ths_pm_domain.ops.resume = sunxi_ths_resume;
	thermal_data->ths_input_dev->dev.pm_domain = &thermal_data->ths_pm_domain;
#endif
	INIT_DELAYED_WORK(&thermal_data->work, sunxi_ths_work_func);
	atomic_set(&thermal_data->delay, (unsigned int) THERMAL_DATA_DELAY);
	schedule_delayed_work(&thermal_data->work,
				msecs_to_jiffies(atomic_read(&thermal_data->delay)));

	dprintk(DEBUG_INIT, "%s: OK!\n", __func__);

	return 0;
fail2:
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
fail1:
	return err;
}

static void __exit sunxi_ths_exit(void)
{
	cancel_delayed_work_sync(&thermal_data->input_work);
	cancel_delayed_work_sync(&thermal_data->work);
	sunxi_ths_reg_clear();
	sunxi_ths_unregister_thermal(&ths_zone);
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
}

#elif defined(CONFIG_ARCH_SUN8IW5P1)

#define SUN8IW5_SID_CALIBRATION
#ifdef SUN8IW5_SID_CALIBRATION
#define SID_BASE	(void __iomem *)(0xf1C23800)
#define SID_THS		(SID_BASE + 0X10)
static void ths_calibration(void)
{
	u32 reg_val;
	reg_val = readl(SID_THS);
	if(reg_val != 0)
	{
		writel(reg_val,thermal_data->base_addr + TEMP_CATA);
	}
}
#endif

static void sunxi_ths_work_func(struct work_struct *work)
{
	struct sunxi_ths_data *data = container_of((struct delayed_work *)work,
			struct sunxi_ths_data, work);
	unsigned long delay = msecs_to_jiffies(atomic_read(&data->delay));

	data->temp_data1[data->circle_num] = readl(data->base_addr + THS_DATA_REG);
	//dprintk(DEBUG_DATA_INFO, "THS data = %d\n", data->temp_data1[data->circle_num]);

	if (1000 < data->temp_data1[data->circle_num])
		data->circle_num++;
	calc_temperature(1, 618, 269, data);

	data->thermal_data[4] = data->thermal_data[0];
	ths_write_data(0, data);
	ths_write_data(4, data);

	schedule_delayed_work(&data->work, delay);
}
static void sunxi_ths_reg_clear(void)
{
	writel(0, thermal_data->base_addr + THS_CTRL_REG1);
	writel(0, thermal_data->base_addr + THS_PRO_CTRL_REG);
}

static void sunxi_ths_reg_init(void)
{
#ifdef SUN8IW5_SID_CALIBRATION
	ths_calibration();
#endif
	writel(THS_CTRL_REG0_VALUE, thermal_data->base_addr + THS_CTRL_REG0);
	writel(THS_CTRL_REG1_VALUE, thermal_data->base_addr + THS_CTRL_REG1);
	writel(THS_PRO_CTRL_REG_VALUE, thermal_data->base_addr + THS_PRO_CTRL_REG);

	dprintk(DEBUG_INIT, "THS_CTRL_REG0 = 0x%x\n", readl(thermal_data->base_addr + THS_CTRL_REG0));
	dprintk(DEBUG_INIT, "THS_CTRL_REG1 = 0x%x\n", readl(thermal_data->base_addr + THS_CTRL_REG1));
	dprintk(DEBUG_INIT, "THS_PRO_CTRL_REG = 0x%x\n", readl(thermal_data->base_addr + THS_PRO_CTRL_REG));
}

static struct thermal_trip_point_conf sunxi_trip_data;

static struct thermal_cooling_conf sunxi_cooling_data;

static struct thermal_sensor_conf sunxi_sensor_conf = {
	.name			= SUNXI_THERMAL_COOLING_DEVICE_NAMER,
	.read_temperature	= ths_read_data,
	.trip_data		= &sunxi_trip_data,
	.cooling_data		= &sunxi_cooling_data,
};

static struct sunxi_thermal_zone ths_zone = {
	.id			= 0,
	.name			= "sunxi-therm",
	.sunxi_ths_sensor_conf	= &sunxi_sensor_conf,
};

static int __init sunxi_ths_init(void)
{
	int err = 0;

	dprintk(DEBUG_INIT, "%s: enter!\n", __func__);

	if (input_fetch_sysconfig_para(&(ths_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -EPERM;
	}

	sunxi_trip_data.trip_count = ths_info.trip1_count;
	sunxi_trip_data.trip_val[0] = ths_info.trip1_0;
	sunxi_trip_data.trip_val[1] = ths_info.trip1_1;
	sunxi_trip_data.trip_val[2] = ths_info.trip1_2;

	sunxi_cooling_data.freq_clip_count = ths_info.trip1_count-1;
	sunxi_cooling_data.freq_data[0].freq_clip_min = ths_info.trip1_0_min;
	sunxi_cooling_data.freq_data[0].freq_clip_max = ths_info.trip1_0_max;
	sunxi_cooling_data.freq_data[0].temp_level = ths_info.trip1_0;
	sunxi_cooling_data.freq_data[1].freq_clip_min = ths_info.trip1_1_min;
	sunxi_cooling_data.freq_data[1].freq_clip_max = ths_info.trip1_1_max;
	sunxi_cooling_data.freq_data[1].temp_level = ths_info.trip1_1;

	thermal_data = kzalloc(sizeof(*thermal_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(thermal_data)) {
		printk(KERN_ERR "thermal_data: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxi_ths_input_init(thermal_data);

	ths_zone.sunxi_ths_sensor_conf->trend = ths_info.ths_trend;
#ifdef CONFIG_SUNXI_THERMAL_DYNAMIC
	err = sunxi_ths_register_thermal_dynamic(&ths_zone);
#else
	err = sunxi_ths_register_thermal(&ths_zone);
#endif
	if(err < 0) {
		printk(KERN_ERR "therma: register thermal core failed\n");
		goto fail2;
	}

	thermal_data->base_addr = (void __iomem *)THERMAL_BASSADDRESS;
	sunxi_ths_reg_init();
	thermal_data->irq_used = 0;

#ifdef CONFIG_PM
	thermal_data->ths_pm_domain.ops.suspend = sunxi_ths_suspend;
	thermal_data->ths_pm_domain.ops.resume = sunxi_ths_resume;
	thermal_data->ths_input_dev->dev.pm_domain = &thermal_data->ths_pm_domain;
#endif
	INIT_DELAYED_WORK(&thermal_data->work, sunxi_ths_work_func);
	atomic_set(&thermal_data->delay, (unsigned int) THERMAL_DATA_DELAY);
	schedule_delayed_work(&thermal_data->work,
				msecs_to_jiffies(atomic_read(&thermal_data->delay)));

	dprintk(DEBUG_INIT, "%s: OK!\n", __func__);

	return 0;
fail2:
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
fail1:
	return err;
}

static void __exit sunxi_ths_exit(void)
{
	cancel_delayed_work_sync(&thermal_data->input_work);
	cancel_delayed_work_sync(&thermal_data->work);
	sunxi_ths_reg_clear();
#ifdef CONFIG_SUNXI_THERMAL_DYNAMIC
	sunxi_ths_unregister_thermal_dynamic(&ths_zone);
#else
	sunxi_ths_unregister_thermal(&ths_zone);
#endif
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
}

#elif defined(CONFIG_ARCH_SUN8IW6P1)
#define SUN8IW6_SID_CALIBRATION
#ifdef SUN8IW6_SID_CALIBRATION
#define SID_BASE				(void __iomem *)(0xf1c14000)

#define SID_SRAM				(SID_BASE + 0x200)

#define SID_THERMAL_CDATA1				(0xf1c0e000 + 0x34)
#define SID_THERMAL_CDATA2				(0xf1c0e000 + 0x38)

#define SID_THERMAL_CDATA1_SRAM				(SID_SRAM + 0x34)
#define SID_THERMAL_CDATA2_SRAM				(SID_SRAM + 0x38)

#endif


static struct workqueue_struct *thermal_wq;

/****************************** thermal zone 1 *************************************/

static struct	thermal_trip_point_conf sunxi_trip_data_1 = {
	.trip_count		= 3,
	.trip_val[0]		= 80,
	.trip_val[1]		= 100,
	.trip_val[2]		= 110,
};

static struct thermal_cooling_conf sunxi_cooling_data_1 = {
	.freq_data[0] = {
		.freq_clip_min = 408000,
		.freq_clip_max = 2016000,
		.temp_level = 80,
	},
	.freq_data[1] = {
		.freq_clip_min = 60000,
		.freq_clip_max = 408000,
		.temp_level = 100,
	},
	.freq_clip_count = 2,
};

static struct thermal_sensor_conf sunxi_sensor_conf_1 = {
	.name			= SUNXI_THERMAL_COOLING_DEVICE_NAMER,
	.read_temperature	= ths_read_data,
	.trip_data		= &sunxi_trip_data_1,
	.cooling_data		= &sunxi_cooling_data_1,
};

static struct sunxi_thermal_zone ths_zone_1 = {
	.id			= 4,
	.name			= "sunxi-therm-1",
	.sunxi_ths_sensor_conf	= &sunxi_sensor_conf_1,
};

/**************************** thermal zone 1 end ***********************************/

/****************************** thermal zone 2 *************************************/

static struct	thermal_trip_point_conf sunxi_trip_data_2 = {
	.trip_count		= 1,
	.trip_val[0]		= 110,
};

static struct thermal_cooling_conf sunxi_cooling_data_2 = {
	.freq_data[0] = {
		.freq_clip_min = 60000,
		.freq_clip_max = 2016000,
		.temp_level = 110,
	},
	.freq_clip_count = 1,
};

static struct thermal_sensor_conf sunxi_sensor_conf_2 = {
	.name			= SUNXI_THERMAL_COOLING_DEVICE_NAMER,
	.read_temperature	= ths_read_data,
	.trip_data		= &sunxi_trip_data_2,
	.cooling_data		= &sunxi_cooling_data_2,
};

static struct sunxi_thermal_zone ths_zone_2 = {
	.id			= 4,
	.name			= "sunxi-therm-2",
	.sunxi_ths_sensor_conf	= &sunxi_sensor_conf_2,
};

/**************************** thermal zone 2 end ***********************************/

static int temperature_to_reg(int temp_value, int multiplier, int minus)
{
	int64_t temp_reg;

	temp_reg = minus*1000 - multiplier*temp_value;

	do_div(temp_reg, 1000);
	dprintk(DEBUG_INIT, "temp_reg=%lld \n", temp_reg);

	return (int)(temp_reg);
}

static inline unsigned long ths_get_intsta(struct sunxi_ths_data *data)
{
	return (sunxi_smc_readl(data->base_addr + THS_INT_STA_REG));
}

static inline void ths_clr_intsta(struct sunxi_ths_data *data)
{
	sunxi_smc_writel(THS_CLEAR_INT_STA, data->base_addr + THS_INT_STA_REG);
}

static void ths_irq_work_func(struct work_struct *work)
{
	dprintk(DEBUG_DATA_INFO, "%s enter\n", __func__);
	thermal_zone_device_update(ths_zone_2.therm_dev);
	return;
}

static irqreturn_t ths_irq_service(int irqno, void *dev_id)
{
	unsigned long intsta;
	dprintk(DEBUG_DATA_INFO, "THS IRQ Serve\n");

	intsta = ths_get_intsta(thermal_data);

	ths_clr_intsta(thermal_data);

	if (intsta & (THS_INTS_SHT0|THS_INTS_SHT1|THS_INTS_SHT2)){
		queue_work(thermal_wq, &thermal_data->irq_work);
	}

	return IRQ_HANDLED;
}

static void sunxi_ths_work_func(struct work_struct *work)
{
	int i = 0;
	int temp;
	uint64_t avg_temp = 0;
	struct sunxi_ths_data *data = container_of((struct delayed_work *)work,
			struct sunxi_ths_data, work);
	unsigned long delay = msecs_to_jiffies(atomic_read(&data->delay));

	data->temp_data1[data->circle_num] = sunxi_smc_readl(data->base_addr + THS_DATA_REG0);
	dprintk(DEBUG_DATA_INFO, "THS data0 = %d\n", data->temp_data1[data->circle_num]);
	data->temp_data2[data->circle_num] = sunxi_smc_readl(data->base_addr + THS_DATA_REG1);
	dprintk(DEBUG_DATA_INFO, "THS data1 = %d\n", data->temp_data2[data->circle_num]);
	data->temp_data3[data->circle_num] = sunxi_smc_readl(data->base_addr + THS_DATA_REG2);
	dprintk(DEBUG_DATA_INFO, "THS data2 = %d\n", data->temp_data3[data->circle_num]);

	if ((1000 < data->temp_data1[data->circle_num]) && (1000 < data->temp_data2[data->circle_num])
		&& (1000 < data->temp_data3[data->circle_num]))
		data->circle_num++;
	calc_temperature(3, 14186, 192, data);

	if (0 == data->circle_num) {
		temp = data->thermal_data[0];
		for(i=0; i < 3; i++) {
			if (temp < data->thermal_data[i])
				temp = data->thermal_data[i];
			ths_write_data(i, data);
			avg_temp += data->thermal_data[i];
		}
		data->thermal_data[4] = temp;
		ths_write_data(4, data);
		do_div(avg_temp, 3);
		data->thermal_data[5] = (int)(avg_temp);
		ths_write_data(5, data);
	}

	schedule_delayed_work(&data->work, delay);
}

static void sunxi_ths_reg_clear(void)
{
	sunxi_smc_writel(0, thermal_data->base_addr + THS_CTRL2_REG);
}

static void sunxi_ths_reg_init(void)
{
	int reg_value;

	sunxi_smc_writel(THS_CTRL1_VALUE, thermal_data->base_addr + THS_CTRL1_REG);

#ifdef SUN8IW6_SID_CALIBRATION
	reg_value = sunxi_smc_readl(SID_THERMAL_CDATA1_SRAM);
	if (0 != reg_value)
		sunxi_smc_writel(reg_value, thermal_data->base_addr + THS_0_1_CDATA_REG);
	reg_value = sunxi_smc_readl(SID_THERMAL_CDATA2_SRAM);
	if (0 != reg_value)
		sunxi_smc_writel(reg_value, thermal_data->base_addr + THS_2_CDATA_REG);
#endif

	sunxi_smc_writel(THS_CTRL0_VALUE, thermal_data->base_addr + THS_CTRL0_REG);
	sunxi_smc_writel(THS_CTRL2_VALUE, thermal_data->base_addr + THS_CTRL2_REG);
	sunxi_smc_writel(THS_INT_CTRL_VALUE, thermal_data->base_addr + THS_INT_CTRL_REG);
	sunxi_smc_writel(THS_CLEAR_INT_STA, thermal_data->base_addr + THS_INT_STA_REG);
	sunxi_smc_writel(THS_FILT_CTRL_VALUE, thermal_data->base_addr + THS_FILT_CTRL_REG);

	reg_value = temperature_to_reg(ths_zone_2.sunxi_ths_sensor_conf->trip_data->trip_val[0], 14186, 2719);
	reg_value = (reg_value<<16);

	sunxi_smc_writel(reg_value, thermal_data->base_addr + THS_INT_SHUT_TH_REG0);
	sunxi_smc_writel(reg_value, thermal_data->base_addr + THS_INT_SHUT_TH_REG1);
	sunxi_smc_writel(reg_value, thermal_data->base_addr + THS_INT_SHUT_TH_REG2);

	dprintk(DEBUG_INIT, "THS_CTRL_REG = 0x%x\n", sunxi_smc_readl(thermal_data->base_addr + THS_CTRL2_REG));
	dprintk(DEBUG_INIT, "THS_INT_CTRL_REG = 0x%x\n", sunxi_smc_readl(thermal_data->base_addr + THS_INT_CTRL_REG));
	dprintk(DEBUG_INIT, "THS_INT_STA_REG = 0x%x\n", sunxi_smc_readl(thermal_data->base_addr + THS_INT_STA_REG));
	dprintk(DEBUG_INIT, "THS_FILT_CTRL_REG = 0x%x\n", sunxi_smc_readl(thermal_data->base_addr + THS_FILT_CTRL_REG));
}


static int __init sunxi_ths_init(void)
{
	int err = 0;

	dprintk(DEBUG_INIT, "%s: enter!\n", __func__);

	if (input_fetch_sysconfig_para(&(ths_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -EPERM;
	}

	sunxi_trip_data_1.trip_count = ths_info.trip1_count;
	sunxi_trip_data_1.trip_val[0] = ths_info.trip1_0;
	sunxi_trip_data_1.trip_val[1] = ths_info.trip1_1;
	sunxi_trip_data_1.trip_val[2] = ths_info.trip1_2;
	sunxi_trip_data_1.trip_val[3] = ths_info.trip1_3;
	sunxi_trip_data_1.trip_val[4] = ths_info.trip1_4;
	sunxi_trip_data_1.trip_val[5] = ths_info.trip1_5;
	sunxi_trip_data_1.trip_val[6] = ths_info.trip1_6;
	sunxi_trip_data_1.trip_val[7] = ths_info.trip1_7;

	sunxi_cooling_data_1.freq_clip_count = ths_info.trip1_count-1;
	sunxi_cooling_data_1.freq_data[0].freq_clip_min = ths_info.trip1_0_min;
	sunxi_cooling_data_1.freq_data[0].freq_clip_max = ths_info.trip1_0_max;
	sunxi_cooling_data_1.freq_data[0].temp_level = ths_info.trip1_0;
	sunxi_cooling_data_1.freq_data[1].freq_clip_min = ths_info.trip1_1_min;
	sunxi_cooling_data_1.freq_data[1].freq_clip_max = ths_info.trip1_1_max;
	sunxi_cooling_data_1.freq_data[1].temp_level = ths_info.trip1_1;
	sunxi_cooling_data_1.freq_data[2].freq_clip_min = ths_info.trip1_2_min;
	sunxi_cooling_data_1.freq_data[2].freq_clip_max = ths_info.trip1_2_max;
	sunxi_cooling_data_1.freq_data[2].temp_level = ths_info.trip1_2;
	sunxi_cooling_data_1.freq_data[3].freq_clip_min = ths_info.trip1_3_min;
	sunxi_cooling_data_1.freq_data[3].freq_clip_max = ths_info.trip1_3_max;
	sunxi_cooling_data_1.freq_data[3].temp_level = ths_info.trip1_3;
	sunxi_cooling_data_1.freq_data[4].freq_clip_min = ths_info.trip1_4_min;
	sunxi_cooling_data_1.freq_data[4].freq_clip_max = ths_info.trip1_4_max;
	sunxi_cooling_data_1.freq_data[4].temp_level = ths_info.trip1_4;
	sunxi_cooling_data_1.freq_data[5].freq_clip_min = ths_info.trip1_5_min;
	sunxi_cooling_data_1.freq_data[5].freq_clip_max = ths_info.trip1_5_max;
	sunxi_cooling_data_1.freq_data[5].temp_level = ths_info.trip1_5;
	sunxi_cooling_data_1.freq_data[6].freq_clip_min = ths_info.trip1_6_min;
	sunxi_cooling_data_1.freq_data[6].freq_clip_max = ths_info.trip1_6_max;
	sunxi_cooling_data_1.freq_data[6].temp_level = ths_info.trip1_6;

	sunxi_trip_data_2.trip_count = ths_info.trip2_count;
	sunxi_trip_data_2.trip_val[0] = ths_info.trip2_0;
	sunxi_cooling_data_2.freq_clip_count = ths_info.trip2_count-1;
	sunxi_cooling_data_2.freq_data[0].temp_level = ths_info.trip2_0;

	thermal_data = kzalloc(sizeof(*thermal_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(thermal_data)) {
		printk(KERN_ERR "thermal_data: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxi_ths_input_init(thermal_data);
	err = sunxi_ths_register_thermal(&ths_zone_1);
	if(err < 0) {
		printk(KERN_ERR "therma: register thermal core failed\n");
		goto fail2;
	}
	err = sunxi_ths_register_thermal(&ths_zone_2);
	if(err < 0) {
		printk(KERN_ERR "therma: register thermal core failed\n");
		goto fail2;
	}

	thermal_data->base_addr = (void __iomem *)THERMAL_BASSADDRESS;
	sunxi_ths_reg_init();

	INIT_WORK(&thermal_data->irq_work, ths_irq_work_func);
	thermal_wq = create_singlethread_workqueue("thermal_wq");
	if (!thermal_wq) {
		printk(KERN_ALERT "Creat thermal_wq failed.\n");
		return -ENOMEM;
	}
	flush_workqueue(thermal_wq);
	
	thermal_data->irq_used = 1;
	if (request_irq(THS_IRQNO, ths_irq_service, 0, "Thermal",
			&(thermal_data->ths_input_dev))) {
		err = -EBUSY;
		goto fail3;
	}

#ifdef CONFIG_PM
	thermal_data->ths_pm_domain.ops.suspend = sunxi_ths_suspend;
	thermal_data->ths_pm_domain.ops.resume = sunxi_ths_resume;
	thermal_data->ths_input_dev->dev.pm_domain = &thermal_data->ths_pm_domain;
#endif
	INIT_DELAYED_WORK(&thermal_data->work, sunxi_ths_work_func);
	atomic_set(&thermal_data->delay, (unsigned int) THERMAL_DATA_DELAY);
	schedule_delayed_work(&thermal_data->work,
				msecs_to_jiffies(atomic_read(&thermal_data->delay)));

	dprintk(DEBUG_INIT, "%s: OK!\n", __func__);

	return 0;
fail3:
	sunxi_ths_unregister_thermal(&ths_zone_2);
	sunxi_ths_unregister_thermal(&ths_zone_1);
fail2:
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
fail1:
	return err;
}

static void __exit sunxi_ths_exit(void)
{
	cancel_delayed_work_sync(&thermal_data->input_work);
	cancel_delayed_work_sync(&thermal_data->work);
	free_irq(THS_IRQNO, thermal_data->ths_input_dev);
	if (thermal_wq)
		destroy_workqueue(thermal_wq);
	sunxi_ths_reg_clear();
	sunxi_ths_unregister_thermal(&ths_zone_2);
	sunxi_ths_unregister_thermal(&ths_zone_1);
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
}

#elif defined(CONFIG_ARCH_SUN8IW7P1)
#define SUN8IW7_SID_CALIBRATION
#ifdef SUN8IW7_SID_CALIBRATION
#define SID_CAL_DATA				(void __iomem *)(0xf1c14234)
#endif

static struct workqueue_struct *thermal_wq;

/****************************** thermal zone 1 *************************************/

static struct	thermal_trip_point_conf sunxi_trip_data_1 = {
	.trip_count		= 3,
	.trip_val[0]		= 80,
	.trip_val[1]		= 100,
	.trip_val[2]		= 110,
};

static struct thermal_cooling_conf sunxi_cooling_data_1 = {
	.freq_data[0] = {
		.freq_clip_min = 408000,
		.freq_clip_max = 2016000,
		.temp_level = 80,
	},
	.freq_data[1] = {
		.freq_clip_min = 60000,
		.freq_clip_max = 408000,
		.temp_level = 100,
	},
	.freq_clip_count = 2,
};

static struct thermal_sensor_conf sunxi_sensor_conf_1 = {
	.name			= SUNXI_THERMAL_COOLING_DEVICE_NAMER,
	.read_temperature	= ths_read_data,
	.trip_data		= &sunxi_trip_data_1,
	.cooling_data		= &sunxi_cooling_data_1,
};

static struct sunxi_thermal_zone ths_zone_1 = {
	.id			= 4,
	.name			= "sunxi-therm-1",
	.sunxi_ths_sensor_conf	= &sunxi_sensor_conf_1,
};

/**************************** thermal zone 1 end ***********************************/

/****************************** thermal zone 2 *************************************/

static struct	thermal_trip_point_conf sunxi_trip_data_2 = {
	.trip_count		= 1,
	.trip_val[0]		= 110,
};

static struct thermal_cooling_conf sunxi_cooling_data_2 = {
	.freq_data[0] = {
		.freq_clip_min = 60000,
		.freq_clip_max = 2016000,
		.temp_level = 110,
	},
	.freq_clip_count = 1,
};

static struct thermal_sensor_conf sunxi_sensor_conf_2 = {
	.name			= SUNXI_THERMAL_COOLING_DEVICE_NAMER,
	.read_temperature	= ths_read_data,
	.trip_data		= &sunxi_trip_data_2,
	.cooling_data		= &sunxi_cooling_data_2,
};

static struct sunxi_thermal_zone ths_zone_2 = {
	.id			= 4,
	.name			= "sunxi-therm-2",
	.sunxi_ths_sensor_conf	= &sunxi_sensor_conf_2,
};

/**************************** thermal zone 2 end ***********************************/

static int temperature_to_reg(int temp_value, int multiplier, int minus)
{
	int64_t temp_reg;

	temp_reg = minus*1000 - multiplier*temp_value;

	do_div(temp_reg, 1000);
	dprintk(DEBUG_INIT, "temp_reg=%lld \n", temp_reg);

	return (int)(temp_reg);
}

static inline unsigned long ths_get_intsta(struct sunxi_ths_data *data)
{
	return (readl(data->base_addr + THS_INT_STA_REG));
}

static inline void ths_clr_intsta(struct sunxi_ths_data *data)
{
	writel(THS_CLEAR_INT_STA, data->base_addr + THS_INT_STA_REG);
}

static void ths_irq_work_func(struct work_struct *work)
{
	dprintk(DEBUG_DATA_INFO, "%s enter\n", __func__);
	thermal_zone_device_update(ths_zone_2.therm_dev);
	return;
}

static irqreturn_t ths_irq_service(int irqno, void *dev_id)
{
	unsigned long intsta;
	dprintk(DEBUG_DATA_INFO, "THS IRQ Serve\n");

	intsta = ths_get_intsta(thermal_data);

	ths_clr_intsta(thermal_data);

	if (intsta & (THS_INTS_SHT0)){
		queue_work(thermal_wq, &thermal_data->irq_work);
	}
	
	return IRQ_HANDLED;
}

static void sunxi_ths_work_func(struct work_struct *work)
{
	struct sunxi_ths_data *data = container_of((struct delayed_work *)work,
			struct sunxi_ths_data, work);
	unsigned long delay = msecs_to_jiffies(atomic_read(&data->delay));

	data->temp_data1[data->circle_num] = readl(data->base_addr + THS_DATA_REG);
	dprintk(DEBUG_DATA_INFO, "THS data = %d\n", data->temp_data1[data->circle_num]);

	if ((556 < data->temp_data1[data->circle_num]) && (2000 > data->temp_data1[data->circle_num]))
		data->circle_num++;
	calc_temperature(1, 8253, 217, data);

	if (0 == data->circle_num) {
		ths_write_data(0, data);
		data->thermal_data[4] = data->thermal_data[0];
		ths_write_data(4, data);
		data->thermal_data[5] = data->thermal_data[0];
		ths_write_data(5, data);
	}

	schedule_delayed_work(&data->work, delay);
}

static void sunxi_ths_clk_cfg(void)
{

	unsigned long rate = 0; /* 3Mhz */

	ths_clk_source = clk_get(NULL, HOSC_CLK);
	if (!ths_clk_source || IS_ERR(ths_clk_source)) {
		printk(KERN_DEBUG "try to get ths_clk_source clock failed!\n");
		return;
	}

	rate = clk_get_rate(ths_clk_source);
	dprintk(DEBUG_INIT, "%s: get ths_clk_source rate %dHZ\n", __func__, (__u32)rate);

	ths_clk = clk_get(NULL, THS_CLK);
	if (!ths_clk || IS_ERR(ths_clk)) {
		printk(KERN_DEBUG "try to get ths clock failed!\n");
		return;
	}

	if(clk_set_parent(ths_clk, ths_clk_source))
		printk("%s: set ths_clk parent to ir_clk_source failed!\n", __func__);

	if (clk_set_rate(ths_clk, 4000000)) {
		printk(KERN_DEBUG "set ths clock freq to 4M failed!\n");
	}

	if (clk_prepare_enable(ths_clk)) {
			printk(KERN_DEBUG "try to enable ths_clk failed!\n");
	}

	return;
}

static void sunxi_ths_clk_uncfg(void)
{

	if(NULL == ths_clk || IS_ERR(ths_clk)) {
		printk("ths_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(ths_clk);
		clk_put(ths_clk);
		ths_clk = NULL;
	}

	if(NULL == ths_clk_source || IS_ERR(ths_clk_source)) {
		printk("ths_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(ths_clk_source);
		ths_clk_source = NULL;
	}
	return;
}

static void sunxi_ths_reg_clear(void)
{
	writel(0, thermal_data->base_addr + THS_CTRL2_REG);
}

static void sunxi_ths_reg_init(void)
{
	int reg_value;

#ifdef SUN8IW7_SID_CALIBRATION
	reg_value = readl(SID_CAL_DATA);
	reg_value &= 0xfff;
	if (0 != reg_value)
		writel(reg_value, thermal_data->base_addr + THS_CDATA_REG);
#endif
	writel(THS_CTRL0_VALUE, thermal_data->base_addr + THS_CTRL0_REG);
	writel(THS_INT_CTRL_VALUE, thermal_data->base_addr + THS_INT_CTRL_REG);
	writel(THS_CLEAR_INT_STA, thermal_data->base_addr + THS_INT_STA_REG);
	writel(THS_FILT_CTRL_VALUE, thermal_data->base_addr + THS_FILT_CTRL_REG);

	reg_value = temperature_to_reg(ths_zone_2.sunxi_ths_sensor_conf->trip_data->trip_val[0], 8253, 1794);
	reg_value = (reg_value<<16);

	writel(reg_value, thermal_data->base_addr + THS_INT_SHUT_TH_REG);
	writel(THS_CTRL2_VALUE, thermal_data->base_addr + THS_CTRL2_REG);

	dprintk(DEBUG_INIT, "THS_CTRL_REG = 0x%x\n", readl(thermal_data->base_addr + THS_CTRL2_REG));
	dprintk(DEBUG_INIT, "THS_INT_CTRL_REG = 0x%x\n", readl(thermal_data->base_addr + THS_INT_CTRL_REG));
	dprintk(DEBUG_INIT, "THS_INT_STA_REG = 0x%x\n", readl(thermal_data->base_addr + THS_INT_STA_REG));
	dprintk(DEBUG_INIT, "THS_FILT_CTRL_REG = 0x%x\n", readl(thermal_data->base_addr + THS_FILT_CTRL_REG));
}

static int __init sunxi_ths_init(void)
{
	int err = 0;

	dprintk(DEBUG_INIT, "%s: enter!\n", __func__);

	if (input_fetch_sysconfig_para(&(ths_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -EPERM;
	}

	sunxi_trip_data_1.trip_count = ths_info.trip1_count;
	sunxi_trip_data_1.trip_val[0] = ths_info.trip1_0;
	sunxi_trip_data_1.trip_val[1] = ths_info.trip1_1;
	sunxi_trip_data_1.trip_val[2] = ths_info.trip1_2;
	sunxi_trip_data_1.trip_val[3] = ths_info.trip1_3;
	sunxi_trip_data_1.trip_val[4] = ths_info.trip1_4;
	sunxi_trip_data_1.trip_val[5] = ths_info.trip1_5;
	sunxi_trip_data_1.trip_val[6] = ths_info.trip1_6;
	sunxi_trip_data_1.trip_val[7] = ths_info.trip1_7;

	sunxi_cooling_data_1.freq_clip_count = ths_info.trip1_count-1;
	sunxi_cooling_data_1.freq_data[0].freq_clip_min = ths_info.trip1_0_min;
	sunxi_cooling_data_1.freq_data[0].freq_clip_max = ths_info.trip1_0_max;
	sunxi_cooling_data_1.freq_data[0].temp_level = ths_info.trip1_0;
	sunxi_cooling_data_1.freq_data[1].freq_clip_min = ths_info.trip1_1_min;
	sunxi_cooling_data_1.freq_data[1].freq_clip_max = ths_info.trip1_1_max;
	sunxi_cooling_data_1.freq_data[1].temp_level = ths_info.trip1_1;
	sunxi_cooling_data_1.freq_data[2].freq_clip_min = ths_info.trip1_2_min;
	sunxi_cooling_data_1.freq_data[2].freq_clip_max = ths_info.trip1_2_max;
	sunxi_cooling_data_1.freq_data[2].temp_level = ths_info.trip1_2;
	sunxi_cooling_data_1.freq_data[3].freq_clip_min = ths_info.trip1_3_min;
	sunxi_cooling_data_1.freq_data[3].freq_clip_max = ths_info.trip1_3_max;
	sunxi_cooling_data_1.freq_data[3].temp_level = ths_info.trip1_3;
	sunxi_cooling_data_1.freq_data[4].freq_clip_min = ths_info.trip1_4_min;
	sunxi_cooling_data_1.freq_data[4].freq_clip_max = ths_info.trip1_4_max;
	sunxi_cooling_data_1.freq_data[4].temp_level = ths_info.trip1_4;
	sunxi_cooling_data_1.freq_data[5].freq_clip_min = ths_info.trip1_5_min;
	sunxi_cooling_data_1.freq_data[5].freq_clip_max = ths_info.trip1_5_max;
	sunxi_cooling_data_1.freq_data[5].temp_level = ths_info.trip1_5;
	sunxi_cooling_data_1.freq_data[6].freq_clip_min = ths_info.trip1_6_min;
	sunxi_cooling_data_1.freq_data[6].freq_clip_max = ths_info.trip1_6_max;
	sunxi_cooling_data_1.freq_data[6].temp_level = ths_info.trip1_6;

	sunxi_trip_data_2.trip_count = ths_info.trip2_count;
	sunxi_trip_data_2.trip_val[0] = ths_info.trip2_0;
	sunxi_cooling_data_2.freq_clip_count = ths_info.trip2_count-1;
	sunxi_cooling_data_2.freq_data[0].temp_level = ths_info.trip2_0;

	thermal_data = kzalloc(sizeof(*thermal_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(thermal_data)) {
		printk(KERN_ERR "thermal_data: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxi_ths_input_init(thermal_data);
	err = sunxi_ths_register_thermal(&ths_zone_1);
	if(err < 0) {
		printk(KERN_ERR "therma: register thermal core failed\n");
		goto fail2;
	}
	err = sunxi_ths_register_thermal(&ths_zone_2);
	if(err < 0) {
		printk(KERN_ERR "therma: register thermal core failed\n");
		goto fail2;
	}

	sunxi_ths_clk_cfg();

	thermal_data->base_addr = (void __iomem *)THERMAL_BASSADDRESS;
	sunxi_ths_reg_init();

	INIT_WORK(&thermal_data->irq_work, ths_irq_work_func);
	thermal_wq = create_singlethread_workqueue("thermal_wq");
	if (!thermal_wq) {
		printk(KERN_ALERT "Creat thermal_wq failed.\n");
		return -ENOMEM;
	}
	flush_workqueue(thermal_wq);
	
	thermal_data->irq_used = 1;
	if (request_irq(THS_IRQNO, ths_irq_service, 0, "Thermal",
			&(thermal_data->ths_input_dev))) {
		err = -EBUSY;
		goto fail3;
	}

#ifdef CONFIG_PM
	thermal_data->ths_pm_domain.ops.suspend = sunxi_ths_suspend;
	thermal_data->ths_pm_domain.ops.resume = sunxi_ths_resume;
	thermal_data->ths_input_dev->dev.pm_domain = &thermal_data->ths_pm_domain;
#endif
	INIT_DELAYED_WORK(&thermal_data->work, sunxi_ths_work_func);
	atomic_set(&thermal_data->delay, (unsigned int) THERMAL_DATA_DELAY);
	schedule_delayed_work(&thermal_data->work,
				msecs_to_jiffies(atomic_read(&thermal_data->delay)));

	dprintk(DEBUG_INIT, "%s: OK!\n", __func__);

	return 0;
fail3:
	sunxi_ths_unregister_thermal(&ths_zone_2);
	sunxi_ths_unregister_thermal(&ths_zone_1);
fail2:
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
fail1:
	return err;
}

static void __exit sunxi_ths_exit(void)
{
	cancel_delayed_work_sync(&thermal_data->input_work);
	cancel_delayed_work_sync(&thermal_data->work);
	free_irq(THS_IRQNO, thermal_data->ths_input_dev);
	sunxi_ths_reg_clear();
	sunxi_ths_clk_uncfg();
	sunxi_ths_unregister_thermal(&ths_zone_2);
	sunxi_ths_unregister_thermal(&ths_zone_1);
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
}

#elif defined(CONFIG_ARCH_SUN9IW1P1)

#define SUN9I_SID_CALIBRATION
#ifdef SUN9I_SID_CALIBRATION
#define SID_BASE				(void __iomem *)(0xf1c0e000)
#define SID_PRCTL				(SID_BASE + 0x40)
#define SID_PRKEY				(SID_BASE + 0x50)
#define SID_RDKEY				(SID_BASE + 0x60)
#define SJTAG_AT0				(SID_BASE + 0x80)
#define SJTAG_AT1				(SID_BASE + 0x84)
#define SJTAG_S					(SID_BASE + 0x88)

#define SID_SRAM				(SID_BASE + 0x200)
#define SID_OP_LOCK				(0xAC)

#define SID_THERMAL1				(0xf1c0e000 + 0x44)
#define SID_THERMAL2				(0xf1c0e000 + 0x48)

#define SID_THERMAL1_SRAM				(SID_SRAM + 0x44)
#define SID_THERMAL2_SRAM				(SID_SRAM + 0x48)

#endif

/****************************** thermal zone 1 *************************************/
static struct workqueue_struct *thermal_wq;

static struct	thermal_trip_point_conf sunxi_trip_data_1;

static struct thermal_cooling_conf sunxi_cooling_data_1;

static struct thermal_sensor_conf sunxi_sensor_conf_1 = {
	.name			= SUNXI_THERMAL_COOLING_DEVICE_NAMER,
	.read_temperature	= ths_read_data,
	.trip_data		= &sunxi_trip_data_1,
	.cooling_data		= &sunxi_cooling_data_1,
};

static struct sunxi_thermal_zone ths_zone_1 = {
	.id			= 4,
	.name			= "sunxi-therm-1",
	.sunxi_ths_sensor_conf	= &sunxi_sensor_conf_1,
};

/**************************** thermal zone 1 end ***********************************/

/****************************** thermal zone 2 *************************************/

static struct	thermal_trip_point_conf sunxi_trip_data_2;

static struct thermal_cooling_conf sunxi_cooling_data_2 = {
	.freq_data[0] = {
		.freq_clip_min = 60000,
		.freq_clip_max = 2016000,
	},
};

static struct thermal_sensor_conf sunxi_sensor_conf_2 = {
	.name			= SUNXI_THERMAL_COOLING_DEVICE_NAMER,
	.read_temperature	= ths_read_data,
	.trip_data		= &sunxi_trip_data_2,
	.cooling_data		= &sunxi_cooling_data_2,
};

static struct sunxi_thermal_zone ths_zone_2 = {
	.id			= 4,
	.name			= "sunxi-therm-2",
	.sunxi_ths_sensor_conf	= &sunxi_sensor_conf_2,
};

/**************************** thermal zone 2 end ***********************************/

static int temperature_to_reg(int temp_value, int multiplier, int minus)
{
	int64_t temp_reg;

	temp_reg = minus*1000 - multiplier*temp_value;

	do_div(temp_reg, 1000);
	dprintk(DEBUG_INIT, "temp_reg=%lld \n", temp_reg);

	return (int)(temp_reg);
}

static inline unsigned long ths_get_intsta(struct sunxi_ths_data *data)
{
	return (readl(data->base_addr + THS_INT_STA_REG));
}

static inline void ths_clr_intsta(struct sunxi_ths_data *data)
{
	writel(THS_CLEAR_INT_STA, data->base_addr + THS_INT_STA_REG);
}

static void ths_irq_work_func(struct work_struct *work)
{
	dprintk(DEBUG_DATA_INFO, "%s enter\n", __func__);
	thermal_zone_device_update(ths_zone_1.therm_dev);
	return;
}

static irqreturn_t ths_irq_service(int irqno, void *dev_id)
{
	unsigned long intsta;
	unsigned long intreg;
	dprintk(DEBUG_DATA_INFO, "THS IRQ Serve\n");

	intsta = ths_get_intsta(thermal_data);
	intreg = readl(thermal_data->base_addr + THS_INT_CTRL_REG);
	if (intsta & (THS_INTS_ALARM0|THS_INTS_ALARM1|THS_INTS_ALARM2|THS_INTS_ALARM3))
		intreg &=0xff0;
	writel(intreg, thermal_data->base_addr + THS_INT_CTRL_REG);

	ths_clr_intsta(thermal_data);

	queue_work(thermal_wq, &thermal_data->irq_work);

	return IRQ_HANDLED;
}

static void sunxi_ths_work_func(struct work_struct *work)
{
	int i = 0;
	int temp;
	uint64_t avg_temp = 0;
	unsigned long intreg;
	struct sunxi_ths_data *data = container_of((struct delayed_work *)work,
			struct sunxi_ths_data, work);
	unsigned long delay = msecs_to_jiffies(atomic_read(&data->delay));

	data->temp_data1[data->circle_num] = readl(data->base_addr + THS_DATA_REG0);
	//dprintk(DEBUG_DATA_INFO, "THS data0 = %d\n", data->temp_data1[data->circle_num]);
	data->temp_data2[data->circle_num] = readl(data->base_addr + THS_DATA_REG1);
	//dprintk(DEBUG_DATA_INFO, "THS data1 = %d\n", data->temp_data2[data->circle_num]);
	data->temp_data3[data->circle_num] = readl(data->base_addr + THS_DATA_REG2);
	//dprintk(DEBUG_DATA_INFO, "THS data2 = %d\n", data->temp_data3[data->circle_num]);
	data->temp_data4[data->circle_num] = readl(data->base_addr + THS_DATA_REG3);
	//dprintk(DEBUG_DATA_INFO, "THS data3 = %d\n", data->temp_data4[data->circle_num]);

	if ((1000 < data->temp_data1[data->circle_num]) && (1000 < data->temp_data2[data->circle_num])
		&& (1000 < data->temp_data3[data->circle_num])
		&& (1000 < data->temp_data4[data->circle_num]))
		data->circle_num++;
	calc_temperature(4, 14543, 190, data);

	if ((data->thermal_data[0] < ths_zone_1.sunxi_ths_sensor_conf->trip_data->trip_val[0]) ||
	(data->thermal_data[1] < ths_zone_1.sunxi_ths_sensor_conf->trip_data->trip_val[0]) ||
	(data->thermal_data[2] < ths_zone_1.sunxi_ths_sensor_conf->trip_data->trip_val[0]) ||
	(data->thermal_data[3] < ths_zone_1.sunxi_ths_sensor_conf->trip_data->trip_val[0])) {
		intreg = readl(thermal_data->base_addr + THS_INT_CTRL_REG);
		if (!(intreg & 0x00f)) {
			intreg |=0x00f;
			writel(intreg, thermal_data->base_addr + THS_INT_CTRL_REG);
		}
	}

	if (0 == data->circle_num) {
		temp = data->thermal_data[0];
		for(i=0; i < 4; i++) {
			if (temp < data->thermal_data[i])
				temp = data->thermal_data[i];
			ths_write_data(i, data);
			avg_temp += data->thermal_data[i];
		}
		data->thermal_data[4] = temp;
		ths_write_data(4, data);
		do_div(avg_temp, 4);
		data->thermal_data[5] = (int)(avg_temp);
		ths_write_data(5, data);
	}

	schedule_delayed_work(&data->work, delay);
}

static void sunxi_ths_clk_cfg(void)
{

	unsigned long rate = 0; /* 3Mhz */

	gpadc_clk_source = clk_get(NULL, HOSC_CLK);
	if (!gpadc_clk_source || IS_ERR(gpadc_clk_source)) {
		printk(KERN_DEBUG "try to get ir_clk_source clock failed!\n");
		return;
	}

	rate = clk_get_rate(gpadc_clk_source);
	dprintk(DEBUG_INIT, "%s: get ir_clk_source rate %dHZ\n", __func__, (__u32)rate);

	gpadc_clk = clk_get(NULL, GPADC_CLK);
	if (!gpadc_clk || IS_ERR(gpadc_clk)) {
		printk(KERN_DEBUG "try to get ir clock failed!\n");
		return;
	}

	if(clk_set_parent(gpadc_clk, gpadc_clk_source))
		printk("%s: set ir_clk parent to ir_clk_source failed!\n", __func__);

	if (clk_set_rate(gpadc_clk, 4000000)) {
		printk(KERN_DEBUG "set ir clock freq to 3M failed!\n");
	}

	if (clk_prepare_enable(gpadc_clk)) {
			printk(KERN_DEBUG "try to enable ir_clk failed!\n");
	}

	return;
}

static void sunxi_ths_clk_uncfg(void)
{

	if(NULL == gpadc_clk || IS_ERR(gpadc_clk)) {
		printk("gpadc_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(gpadc_clk);
		clk_put(gpadc_clk);
		gpadc_clk = NULL;
	}

	if(NULL == gpadc_clk_source || IS_ERR(gpadc_clk_source)) {
		printk("gpadc_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(gpadc_clk_source);
		gpadc_clk_source = NULL;
	}
	return;
}

static void sunxi_ths_reg_clear(void)
{
	writel(0, thermal_data->base_addr + THS_CTRL_REG);
}

static void sunxi_ths_reg_init(void)
{
	int reg_value;

	writel(THS_CTRL_VALUE, thermal_data->base_addr + THS_CTRL_REG);
	writel(THS_INT_CTRL_VALUE, thermal_data->base_addr + THS_INT_CTRL_REG);
	writel(THS_CLEAR_INT_STA, thermal_data->base_addr + THS_INT_STA_REG);
	writel(THS_FILT_CTRL_VALUE, thermal_data->base_addr + THS_FILT_CTRL_REG);

	reg_value = temperature_to_reg(ths_zone_1.sunxi_ths_sensor_conf->trip_data->trip_val[0], 14882, 2794);

	reg_value = (reg_value<<16);
	reg_value |= 0xfff;

	writel(reg_value, thermal_data->base_addr + THS_INT_ALM_TH_REG0);
	writel(reg_value, thermal_data->base_addr + THS_INT_ALM_TH_REG1);
	writel(reg_value, thermal_data->base_addr + THS_INT_ALM_TH_REG2);
	writel(reg_value, thermal_data->base_addr + THS_INT_ALM_TH_REG3);

	reg_value = temperature_to_reg(ths_zone_2.sunxi_ths_sensor_conf->trip_data->trip_val[0], 14882, 2794);

	reg_value = (reg_value<<16);
	reg_value |= 0xfff;

	writel(reg_value, thermal_data->base_addr + THS_INT_SHUT_TH_REG0);
	writel(reg_value, thermal_data->base_addr + THS_INT_SHUT_TH_REG1);
	writel(reg_value, thermal_data->base_addr + THS_INT_SHUT_TH_REG2);
	writel(reg_value, thermal_data->base_addr + THS_INT_SHUT_TH_REG3);

	dprintk(DEBUG_INIT, "THS_CTRL_REG = 0x%x\n", readl(thermal_data->base_addr + THS_CTRL_REG));
	dprintk(DEBUG_INIT, "THS_INT_CTRL_REG = 0x%x\n", readl(thermal_data->base_addr + THS_INT_CTRL_REG));
	dprintk(DEBUG_INIT, "THS_INT_STA_REG = 0x%x\n", readl(thermal_data->base_addr + THS_INT_STA_REG));
	dprintk(DEBUG_INIT, "THS_FILT_CTRL_REG = 0x%x\n", readl(thermal_data->base_addr + THS_FILT_CTRL_REG));

#ifdef SUN9I_SID_CALIBRATION
	reg_value = readl(SID_THERMAL1_SRAM);
	if (reg_value != 0)
		writel(reg_value, thermal_data->base_addr + THS_0_1_CDAT_REG);
	reg_value = readl(SID_THERMAL2_SRAM);
	if (reg_value != 0)
		writel(reg_value, thermal_data->base_addr + THS_2_3_CDAT_REG);
#endif
	dprintk(DEBUG_INIT, "THS_0_1_CDAT_REG = 0x%x\n", readl(thermal_data->base_addr + THS_0_1_CDAT_REG));
	dprintk(DEBUG_INIT, "THS_2_3_CDAT_REG = 0x%x\n", readl(thermal_data->base_addr + THS_2_3_CDAT_REG));
}

static int __init sunxi_ths_init(void)
{
	int err = 0;

	dprintk(DEBUG_INIT, "%s: enter!\n", __func__);

	if (input_fetch_sysconfig_para(&(ths_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -EPERM;
	}

	sunxi_trip_data_1.trip_count = ths_info.trip1_count;
	sunxi_trip_data_1.trip_val[0] = ths_info.trip1_0;
	sunxi_trip_data_1.trip_val[1] = ths_info.trip1_1;
	sunxi_trip_data_1.trip_val[2] = ths_info.trip1_2;
	sunxi_trip_data_1.trip_val[3] = ths_info.trip1_3;
	sunxi_trip_data_1.trip_val[4] = ths_info.trip1_4;
	sunxi_trip_data_1.trip_val[5] = ths_info.trip1_5;
	sunxi_trip_data_1.trip_val[6] = ths_info.trip1_6;
	sunxi_trip_data_1.trip_val[7] = ths_info.trip1_7;

	sunxi_cooling_data_1.freq_clip_count = ths_info.trip1_count-1;
	sunxi_cooling_data_1.freq_data[0].freq_clip_min = ths_info.trip1_0_min;
	sunxi_cooling_data_1.freq_data[0].freq_clip_max = ths_info.trip1_0_max;
	sunxi_cooling_data_1.freq_data[0].temp_level = ths_info.trip1_0;
	sunxi_cooling_data_1.freq_data[1].freq_clip_min = ths_info.trip1_1_min;
	sunxi_cooling_data_1.freq_data[1].freq_clip_max = ths_info.trip1_1_max;
	sunxi_cooling_data_1.freq_data[1].temp_level = ths_info.trip1_1;
	sunxi_cooling_data_1.freq_data[2].freq_clip_min = ths_info.trip1_2_min;
	sunxi_cooling_data_1.freq_data[2].freq_clip_max = ths_info.trip1_2_max;
	sunxi_cooling_data_1.freq_data[2].temp_level = ths_info.trip1_2;
	sunxi_cooling_data_1.freq_data[3].freq_clip_min = ths_info.trip1_3_min;
	sunxi_cooling_data_1.freq_data[3].freq_clip_max = ths_info.trip1_3_max;
	sunxi_cooling_data_1.freq_data[3].temp_level = ths_info.trip1_3;
	sunxi_cooling_data_1.freq_data[4].freq_clip_min = ths_info.trip1_4_min;
	sunxi_cooling_data_1.freq_data[4].freq_clip_max = ths_info.trip1_4_max;
	sunxi_cooling_data_1.freq_data[4].temp_level = ths_info.trip1_4;
	sunxi_cooling_data_1.freq_data[5].freq_clip_min = ths_info.trip1_5_min;
	sunxi_cooling_data_1.freq_data[5].freq_clip_max = ths_info.trip1_5_max;
	sunxi_cooling_data_1.freq_data[5].temp_level = ths_info.trip1_5;
	sunxi_cooling_data_1.freq_data[6].freq_clip_min = ths_info.trip1_6_min;
	sunxi_cooling_data_1.freq_data[6].freq_clip_max = ths_info.trip1_6_max;
	sunxi_cooling_data_1.freq_data[6].temp_level = ths_info.trip1_6;

	sunxi_trip_data_2.trip_count = ths_info.trip2_count;
	sunxi_trip_data_2.trip_val[0] = ths_info.trip2_0;
	sunxi_cooling_data_2.freq_clip_count = ths_info.trip2_count-1;
	sunxi_cooling_data_2.freq_data[0].temp_level = ths_info.trip2_0;

	thermal_data = kzalloc(sizeof(*thermal_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(thermal_data)) {
		printk(KERN_ERR "thermal_data: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxi_ths_input_init(thermal_data);
	ths_zone_1.sunxi_ths_sensor_conf->trend = ths_info.ths_trend;
#ifdef CONFIG_SUNXI_THERMAL_DYNAMIC
	err = sunxi_ths_register_thermal_dynamic(&ths_zone_1);
#else
	err = sunxi_ths_register_thermal(&ths_zone_1);
#endif
	if(err < 0) {
		printk(KERN_ERR "therma: register thermal core failed\n");
		goto fail2;
	}
	err = sunxi_ths_register_thermal(&ths_zone_2);
	if(err < 0) {
		printk(KERN_ERR "therma: register thermal core failed\n");
		goto fail2;
	}

	sunxi_ths_clk_cfg();

	thermal_data->base_addr = (void __iomem *)THERMAL_BASSADDRESS;
	sunxi_ths_reg_init();

	INIT_WORK(&thermal_data->irq_work, ths_irq_work_func);
	thermal_wq = create_singlethread_workqueue("thermal_wq");
	if (!thermal_wq) {
		printk(KERN_ALERT "Creat thermal_wq failed.\n");
		return -ENOMEM;
	}
	flush_workqueue(thermal_wq);

	thermal_data->irq_used = 1;
	if (request_irq(THS_IRQNO, ths_irq_service, 0, "Thermal",
			thermal_data->ths_input_dev)) {
		err = -EBUSY;
		goto fail3;
	}

#ifdef CONFIG_PM
	thermal_data->ths_pm_domain.ops.suspend = sunxi_ths_suspend;
	thermal_data->ths_pm_domain.ops.resume = sunxi_ths_resume;
	thermal_data->ths_input_dev->dev.pm_domain = &thermal_data->ths_pm_domain;
#endif
	INIT_DELAYED_WORK(&thermal_data->work, sunxi_ths_work_func);
	atomic_set(&thermal_data->delay, (unsigned int) THERMAL_DATA_DELAY);
	schedule_delayed_work(&thermal_data->work,
				msecs_to_jiffies(atomic_read(&thermal_data->delay)));

	dprintk(DEBUG_INIT, "%s: OK!\n", __func__);

	return 0;
fail3:
	sunxi_ths_unregister_thermal(&ths_zone_2);
#ifdef CONFIG_SUNXI_THERMAL_DYNAMIC
	sunxi_ths_unregister_thermal_dynamic(&ths_zone_1);
#else
	sunxi_ths_unregister_thermal(&ths_zone_1);
#endif
fail2:
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
fail1:
	return err;
}

static void __exit sunxi_ths_exit(void)
{
	cancel_delayed_work_sync(&thermal_data->input_work);
	cancel_delayed_work_sync(&thermal_data->work);
	free_irq(THS_IRQNO, thermal_data->ths_input_dev);
	if (thermal_wq)
		destroy_workqueue(thermal_wq);
	sunxi_ths_reg_clear();
	sunxi_ths_clk_uncfg();
	sunxi_ths_unregister_thermal(&ths_zone_2);
#ifdef CONFIG_SUNXI_THERMAL_DYNAMIC
	sunxi_ths_unregister_thermal_dynamic(&ths_zone_1);
#else
	sunxi_ths_unregister_thermal(&ths_zone_1);
#endif
	sunxi_ths_input_exit(thermal_data);
	kfree(thermal_data);
}
#endif

late_initcall(sunxi_ths_init);
module_exit(sunxi_ths_exit);
module_param_named(debug_mask, debug_mask, int, 0644);
MODULE_DESCRIPTION("thermal seneor driver");
MODULE_AUTHOR("Li Ming <liming@allwinnertech.com>");
MODULE_LICENSE("GPL");


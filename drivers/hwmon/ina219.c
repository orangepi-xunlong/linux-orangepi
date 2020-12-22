/*
 * Driver for Texas Instruments INA219, INA226 power monitor chips
 *
 * INA219:
 * Zero Drift Bi-Directional Current/Power Monitor with I2C Interface
 * Datasheet: http://www.ti.com/product/ina219
 *
 * INA220:
 * Bi-Directional Current/Power Monitor with I2C Interface
 * Datasheet: http://www.ti.com/product/ina220
 *
 * INA226:
 * Bi-Directional Current/Power Monitor with I2C Interface
 * Datasheet: http://www.ti.com/product/ina226
 *
 * INA230:
 * Bi-directional Current/Power Monitor with I2C Interface
 * Datasheet: http://www.ti.com/product/ina230
 *
 * Copyright (C) 2012 Lothar Felten <l-felten@ti.com>
 * Thanks to Jan Volkering
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/workqueue.h>

#include "ina219.h"

/* common register definitions */
#define INA2XX_CONFIG			0x00
#define INA2XX_SHUNT_VOLTAGE	0x01 /* readonly */
#define INA2XX_BUS_VOLTAGE		0x02 /* readonly */
#define INA2XX_POWER			0x03 /* readonly */
#define INA2XX_CURRENT			0x04 /* readonly */
#define INA2XX_CALIBRATION		0x05
#ifdef CONFIG_SENSORS_INA219_ACCUMULATE
#define INA2XX_ACCUM_POWER		0x0A /* the accumulate power, not ina219 register number */
#define INA2XX_ACCUM_TIMES  	0x0B /* the times of accumulate power */
#define INA2XX_ACCUM_TSPMS		0x0C /* the sample time of accumulate powerm, unit is ms */
#endif

/* INA226 register definitions */
#define INA226_MASK_ENABLE		0x06
#define INA226_ALERT_LIMIT		0x07
#define INA226_DIE_ID			0xFF

/* register count */
#define INA219_REGISTERS		6
#define INA226_REGISTERS		8

#define INA2XX_MAX_REGISTERS	INA219_REGISTERS

/* settings - depend on use case
 * 0x07FF: PGA=1, 16VFSR, 128 samples average, conversion time is 68.10ms
 * 0x0FFF: PGA=2, shunt voltage is 80mv
 * 0x019F: PGA=1, 16VFSR, 1 samples average, conversion time is 532us
 * 0x399F: PGA=8, 32VFSR, 1 samples average, conversion time is 532us
 */
#define INA219_CONFIG_DEFAULT 0x07FF
#define INA219_A15_CONFIG_DEFAULT 0x0FFF

/* worst case is 68.10 ms (~14.6Hz, ina219) */
#define INA2XX_CONVERSION_RATE		15

enum ina219_ids { ina219, ina219_a15 };

struct ina219_config {
	u16 config_default;
	int calibration_factor;
	int registers;
	int shunt_div;
	int bus_voltage_shift;
	int bus_voltage_lsb;	/* uV */
	int power_lsb;		/* uW */
};

struct ina219_data {
	struct device *hwmon_dev;
	const struct ina219_config *config;

	struct mutex update_lock;
	bool valid;
	unsigned long last_updated;

	int kind;
	u16 regs[INA2XX_MAX_REGISTERS];
#ifdef CONFIG_SENSORS_INA219_ACCUMULATE
	unsigned int ina219_accumpower;
	unsigned int ina219_accmnumber;
	struct timer_list ina219_timer;
	unsigned int sample_time; /* based on ms */
	struct work_struct ina_works;
	struct device *dev;
#endif
};

static const struct ina219_config ina219_config[] = {
	[ina219] = {
		.config_default = INA219_CONFIG_DEFAULT,
		.calibration_factor = 40960000,
		.registers = INA219_REGISTERS,
		.shunt_div = 100,
		.bus_voltage_shift = 3,
		.bus_voltage_lsb = 4000,
		.power_lsb = 20000,
	},
	[ina219_a15] = {
		.config_default = INA219_A15_CONFIG_DEFAULT,
		.calibration_factor = 40960000,
		.registers = INA219_REGISTERS,
		.shunt_div = 100,
		.bus_voltage_shift = 3,
		.bus_voltage_lsb = 4000,
		.power_lsb = 20000,
	},
};

static struct ina219_data *ina219_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ina219_data *data = i2c_get_clientdata(client);
	struct ina219_data *ret = data;

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated +
		       HZ / INA2XX_CONVERSION_RATE) || !data->valid) {

		int i;

		dev_dbg(&client->dev, "Starting ina219 update\n");

		/* Read all registers */
		for (i = 0; i < data->config->registers; i++) {
			int rv = i2c_smbus_read_word_swapped(client, i);
			if (rv < 0) {
				ret = ERR_PTR(rv);
				goto abort;
			}
			data->regs[i] = rv;
		}
		data->last_updated = jiffies;
		data->valid = 1;
	}

abort:
	mutex_unlock(&data->update_lock);

	return ret;
}

static unsigned int ina219_get_value(struct ina219_data *data, u8 reg)
{
	unsigned int val;

	switch (reg) {
	case INA2XX_SHUNT_VOLTAGE:
		/* the unit is millvolt when val divide data->config->shunt_div*/
		/* the unit is uv when val multiply DIV_ROUND_CLOSEST(1000, data->config->shunt_div) */
		val = data->regs[reg] * 10;
		break;
	case INA2XX_BUS_VOLTAGE:
		val = (data->regs[reg] >> data->config->bus_voltage_shift) \
		  * data->config->bus_voltage_lsb;
		val = DIV_ROUND_CLOSEST(val, 1000);
		break;
	case INA2XX_POWER:
		//val = data->regs[reg] * data->config->power_lsb;
		/* use P=U*I to calibrate power, the unit is uW*/
		val = DIV_ROUND_CLOSEST((data->regs[INA2XX_BUS_VOLTAGE] >> data->config->bus_voltage_shift) \
		  * data->config->bus_voltage_lsb, 1000) * data->regs[INA2XX_CURRENT];
		break;
	case INA2XX_CURRENT:
		/* LSB=1mA (selected). Is in mA */
		val = data->regs[reg];
		break;
#ifdef CONFIG_SENSORS_INA219_ACCUMULATE
	case INA2XX_ACCUM_POWER:
		val = data->ina219_accumpower;
		break;
	case INA2XX_ACCUM_TIMES:
		val = data->ina219_accmnumber;
		break;
	case INA2XX_ACCUM_TSPMS:
		val = data->sample_time;
		break;
#endif
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		val = 0;
		break;
	}

	return val;
}

static ssize_t ina219_show_value(struct device *dev,
				 struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct ina219_data *data = ina219_update_device(dev);

	if (IS_ERR(data))
		return PTR_ERR(data);

	return snprintf(buf, PAGE_SIZE, "%u\n",
			ina219_get_value(data, attr->index));
}

#ifdef CONFIG_SENSORS_INA219_ACCUMULATE
static unsigned int ina219_set_value(struct ina219_data *data, u8 reg, unsigned int val)
{
	switch (reg) {
	case INA2XX_ACCUM_TSPMS:
		data->sample_time = val;
		dev_info(data->dev, "ina_ampmw will overflow at %d hours if average power is 2000mw\n",\
				DIV_ROUND_CLOSEST(596 * val, 1000)); /* 596 is 2^32 / 60 / 60 / 2000 */
		break;
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		val = 0;
		break;
	}

	return 0;
}

static ssize_t ina219_store_value(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int nr = to_sensor_dev_attr(attr)->index;
	unsigned long val;
	int err;
	struct i2c_client *client = to_i2c_client(dev);
	struct ina219_data *data = i2c_get_clientdata(client);

	if (IS_ERR(data))
		return PTR_ERR(data);

	err = kstrtoul(buf, 10, &val);
	if (err < 0)
		return err;

	ina219_set_value(data, nr, (unsigned int)val);

	return count;
}

static void __ina219_thread(struct work_struct *work)
{
	struct ina219_data *data = container_of(work, struct ina219_data, ina_works);

	data = ina219_update_device(data->dev);
	data->ina219_accumpower += DIV_ROUND_CLOSEST(ina219_get_value(data, INA2XX_POWER), 1000);
	data->ina219_accmnumber++;
}

static void ina219_timer_handler(unsigned long dev)
{
	struct i2c_client *client = to_i2c_client((struct device *)dev);
	struct ina219_data *data = i2c_get_clientdata(client);;

	(void)schedule_work(&data->ina_works);

	/* set next trig */
	mod_timer(&(data->ina219_timer), jiffies + data->sample_time);
}

static int ina219_timer_init(struct timer_list *itimer, struct device *dev)
{
	init_timer(itimer);
	itimer->expires = jiffies + HZ * 30; /* start this timer after 30S */
	itimer->function = ina219_timer_handler;
	itimer->data = (unsigned long)dev;

	add_timer(itimer);
	dev_info(dev, "add ina219 timer done\n");

	return 0;
}

static void ina219_timer_exit(struct timer_list *itimer, struct i2c_client *client)
{
	del_timer(itimer);
	dev_info(&client->dev, "ina219 timer removed\n");
}
#endif

/* shunt voltage */
static SENSOR_DEVICE_ATTR(ina_shuuv, S_IRUGO, ina219_show_value, NULL,
			  INA2XX_SHUNT_VOLTAGE);

/* bus voltage */
static SENSOR_DEVICE_ATTR(ina_busmv, S_IRUGO, ina219_show_value, NULL,
			  INA2XX_BUS_VOLTAGE);

/* calculated current */
static SENSOR_DEVICE_ATTR(ina_curma, S_IRUGO, ina219_show_value, NULL,
			  INA2XX_CURRENT);

/* real time power */
static SENSOR_DEVICE_ATTR(ina_powuw, S_IRUGO, ina219_show_value, NULL,
			  INA2XX_POWER);

#ifdef CONFIG_SENSORS_INA219_ACCUMULATE
/* accumulate power */
static SENSOR_DEVICE_ATTR(ina_ampmw, S_IRUGO, ina219_show_value, NULL,
			  INA2XX_ACCUM_POWER);

/* times of accumulate power */
static SENSOR_DEVICE_ATTR(ina_amnum, S_IRUGO, ina219_show_value, NULL,
			  INA2XX_ACCUM_TIMES);

/* sample time of accumulate power */
static SENSOR_DEVICE_ATTR(ina_tspms, S_IRUGO | S_IWUSR, ina219_show_value,\
			  ina219_store_value, INA2XX_ACCUM_TSPMS);
#endif

/* pointers to created device attributes */
static struct attribute *ina219_attributes[] = {
	&sensor_dev_attr_ina_shuuv.dev_attr.attr,
	&sensor_dev_attr_ina_busmv.dev_attr.attr,
	&sensor_dev_attr_ina_curma.dev_attr.attr,
	&sensor_dev_attr_ina_powuw.dev_attr.attr,
#ifdef CONFIG_SENSORS_INA219_ACCUMULATE
	&sensor_dev_attr_ina_ampmw.dev_attr.attr,
	&sensor_dev_attr_ina_amnum.dev_attr.attr,
	&sensor_dev_attr_ina_tspms.dev_attr.attr,
#endif
	NULL,
};

static const struct attribute_group ina219_group = {
	.attrs = ina219_attributes,
};

static int ina219_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = client->adapter;
	struct ina219_data *data;
	struct ina219_platform_data *pdata;
	int ret;
	long shunt;

#if (defined CONFIG_ARCH_SUN9IW1P1)
	if(client->addr == 0x45)
		shunt = 5000; /* Cluster1 shunt value 5mOhms */
	else
		shunt = 10000; /* default shunt value 10mOhms */
#else
	shunt = 10000; /* default shunt value 10mOhms */
#endif
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;

	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (client->dev.platform_data) {
		pdata = (struct ina219_platform_data *)client->dev.platform_data;
		shunt = pdata->shunt_uohms;
	}

	if (shunt <= 0)
		return -ENODEV;

	/* set the device type */
	data->kind = id->driver_data;
	data->config = &ina219_config[data->kind];

	/* device configuration */
	ret = i2c_smbus_write_word_swapped(client, INA2XX_CONFIG,
				data->config->config_default);
	if(ret)
		return ret;

	/* set current LSB to 1mA, shunt is in uOhms */
	/* (equation 13 in datasheet) */
	i2c_smbus_write_word_swapped(client, INA2XX_CALIBRATION,
				     data->config->calibration_factor / shunt);

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	ret = sysfs_create_group(&client->dev.kobj, &ina219_group);
	if (ret)
		return ret;

	data->hwmon_dev = hwmon_device_register_attr(&client->dev, client->name);
	if (IS_ERR(data->hwmon_dev)) {
		ret = PTR_ERR(data->hwmon_dev);
		goto out_err_hwmon;
	}

	dev_info(&client->dev, "power monitor %s (Rshunt = %li uOhm, config:%x)\n",
		 id->name, shunt, data->config->config_default);

#ifdef CONFIG_SENSORS_INA219_ACCUMULATE
	data->ina219_accmnumber = 0;
	data->sample_time = 100;	/* the sample time's default value is 100ms */
	ina219_timer_init(&data->ina219_timer, &client->dev);
	INIT_WORK(&data->ina_works, __ina219_thread);
	data->dev = &client->dev;
#endif

	return 0;

out_err_hwmon:
	dev_err(&client->dev, "power monitor registed failed\n");
	sysfs_remove_group(&client->dev.kobj, &ina219_group);

	return ret;
}

static int ina219_remove(struct i2c_client *client)
{
	struct ina219_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_SENSORS_INA219_ACCUMULATE
	ina219_timer_exit(&data->ina219_timer, client);
	destroy_work_on_stack(&data->ina_works);
#endif

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &ina219_group);

	return 0;
}

#if (defined CONFIG_ARCH_SUN8IW6P1)
static const struct i2c_device_id ina219_id[] = {
	{ "ina219_vcc3", ina219 },
	{ "ina219_cpua", ina219 },
	{ "ina219_cpub", ina219 },
	{ "ina219_syuh", ina219 },
	{ "ina219_dram", ina219 },
	{ "ina219_vgpu", ina219 },
	{ }
};
#endif

#if (defined CONFIG_ARCH_SUN9IW1P1)
static const struct i2c_device_id ina219_id[] = {
	{ "ina219_sn3v", ina219 },
	{ "ina219_gpu",  ina219 },
	{ "ina219_cpua", ina219 },
	{ "ina219_sys",  ina219 },
	{ "ina219_dram", ina219 },
	{ "ina219_cpub", ina219_a15 },
	{ "ina219_vpu",  ina219 },
	{ "ina219_audi", ina219 },
	{ }
};
#endif
MODULE_DEVICE_TABLE(i2c, ina219_id);

static struct i2c_driver ina219_driver = {
	.driver = {
		.name	= "ina219",
	},
	.probe		= ina219_probe,
	.remove		= ina219_remove,
	.id_table	= ina219_id,
};

module_i2c_driver(ina219_driver);

MODULE_AUTHOR("Lothar Felten <l-felten@ti.com>");
MODULE_DESCRIPTION("ina219 driver");
MODULE_LICENSE("GPL");

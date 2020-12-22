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
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/init-input.h>
#include <linux/mfd/axp-mfd.h>
#include "sunxi-thermal.h"

#ifdef CONFIG_SUNXI_BUDGET_COOLING
	#define SUNXI_THERMAL_COOLING_DEVICE_NAMER    "thermal-budget-0"
#else
	#define SUNXI_THERMAL_COOLING_DEVICE_NAMER    "thermal-cpufreq-0"
#endif

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0xf;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

static struct ths_config_info bat_info = {
	.input_type = BAT_TYPE,
};

int sunxi_bat_read_cap(int value)
{
	int bat_cap = 0;

	bat_cap = axp_read_bat_cap();
	bat_cap = 100 - bat_cap;
	if (axp_read_ac_chg())
		bat_cap = 0;

	return bat_cap;
}

/****************************** bat zone 1 *************************************/
static struct	thermal_trip_point_conf sunxi_trip_data_1;

static struct thermal_cooling_conf sunxi_cooling_data_1;

static struct thermal_sensor_conf sunxi_bat_conf_1 = {
	.name			= SUNXI_THERMAL_COOLING_DEVICE_NAMER,
	.read_temperature	= sunxi_bat_read_cap,
	.trip_data		= &sunxi_trip_data_1,
	.cooling_data		= &sunxi_cooling_data_1,
};

static struct sunxi_thermal_zone bat_zone_1 = {
	.id			= 0,
	.name			= "sunxi-bat-1",
	.sunxi_ths_sensor_conf	= &sunxi_bat_conf_1,
};

/**************************** bat zone 1 end ***********************************/

static int __init sunxi_bat_init(void)
{
	int err = 0;

	dprintk(DEBUG_INIT, "%s: enter!\n", __func__);

	if (input_fetch_sysconfig_para(&(bat_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -EPERM;
	}

	sunxi_trip_data_1.trip_count = bat_info.trip1_count;
	sunxi_trip_data_1.trip_val[0] = 100-bat_info.trip1_0;
	sunxi_trip_data_1.trip_val[1] = 100-bat_info.trip1_1;
	sunxi_trip_data_1.trip_val[2] = 100-bat_info.trip1_2;
	sunxi_trip_data_1.trip_val[3] = 100-bat_info.trip1_3;
	sunxi_trip_data_1.trip_val[4] = 100-bat_info.trip1_4;
	sunxi_trip_data_1.trip_val[5] = 100-bat_info.trip1_5;
	sunxi_trip_data_1.trip_val[6] = 100-bat_info.trip1_6;
	sunxi_trip_data_1.trip_val[7] = 100-bat_info.trip1_7;
	sunxi_cooling_data_1.freq_clip_count = bat_info.trip1_count-1;
	sunxi_cooling_data_1.freq_data[0].freq_clip_min = bat_info.trip1_0_min;
	sunxi_cooling_data_1.freq_data[0].freq_clip_max = bat_info.trip1_0_max;
	sunxi_cooling_data_1.freq_data[0].temp_level = 100-bat_info.trip1_0;
	sunxi_cooling_data_1.freq_data[1].freq_clip_min = bat_info.trip1_1_min;
	sunxi_cooling_data_1.freq_data[1].freq_clip_max = bat_info.trip1_1_max;
	sunxi_cooling_data_1.freq_data[1].temp_level = 100-bat_info.trip1_1;
	sunxi_cooling_data_1.freq_data[2].freq_clip_min = bat_info.trip1_2_min;
	sunxi_cooling_data_1.freq_data[2].freq_clip_max = bat_info.trip1_2_max;
	sunxi_cooling_data_1.freq_data[2].temp_level = 100-bat_info.trip1_2;
	sunxi_cooling_data_1.freq_data[3].freq_clip_min = bat_info.trip1_3_min;
	sunxi_cooling_data_1.freq_data[3].freq_clip_max = bat_info.trip1_3_max;
	sunxi_cooling_data_1.freq_data[3].temp_level = 100-bat_info.trip1_3;
	sunxi_cooling_data_1.freq_data[4].freq_clip_min = bat_info.trip1_4_min;
	sunxi_cooling_data_1.freq_data[4].freq_clip_max = bat_info.trip1_4_max;
	sunxi_cooling_data_1.freq_data[4].temp_level = 100-bat_info.trip1_4;
	sunxi_cooling_data_1.freq_data[5].freq_clip_min = bat_info.trip1_5_min;
	sunxi_cooling_data_1.freq_data[5].freq_clip_max = bat_info.trip1_5_max;
	sunxi_cooling_data_1.freq_data[5].temp_level = 100-bat_info.trip1_5;
	sunxi_cooling_data_1.freq_data[6].freq_clip_min = bat_info.trip1_6_min;
	sunxi_cooling_data_1.freq_data[6].freq_clip_max = bat_info.trip1_6_max;
	sunxi_cooling_data_1.freq_data[6].temp_level = 100-bat_info.trip1_6;
	bat_zone_1.sunxi_ths_sensor_conf->trend = bat_info.ths_trend;
#ifdef CONFIG_SUNXI_THERMAL_DYNAMIC
	err = sunxi_ths_register_thermal_dynamic(&bat_zone_1);
#else
	err = sunxi_ths_register_thermal(&bat_zone_1);
#endif
	if(err < 0) {
		printk(KERN_ERR "bat: register thermal core failed\n");
		goto fail;
	}

	dprintk(DEBUG_INIT, "%s: OK!\n", __func__);

	return 0;
fail:
	return err;
}

static void __exit sunxi_bat_exit(void)
{
#ifdef CONFIG_SUNXI_THERMAL_DYNAMIC
	sunxi_ths_unregister_thermal_dynamic(&bat_zone_1);
#else
	sunxi_ths_unregister_thermal(&bat_zone_1);
#endif
}

late_initcall(sunxi_bat_init);
module_exit(sunxi_bat_exit);
module_param_named(debug_mask, debug_mask, int, 0644);
MODULE_DESCRIPTION("bat thermal driver");
MODULE_AUTHOR("Ming Li<liming@allwinnertech.com>");
MODULE_LICENSE("GPL");


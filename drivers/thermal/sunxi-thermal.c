/*
 * drivers/thermal/sunxi-thermal.c
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
#include <linux/cpufreq.h>
#include <linux/thermal.h>
#include <asm/io.h>
#include "sunxi-thermal.h"
#ifndef CONFIG_SUNXI_BUDGET_COOLING
static int sunxi_get_frequency_level(unsigned int cpu, unsigned int freq)
{
	int i = 0, ret = -EINVAL;
	struct cpufreq_frequency_table *table = NULL;
#ifdef CONFIG_CPU_FREQ
	table = cpufreq_frequency_get_table(cpu);
#endif
	if (!table)
		return ret;

	while (table[i].frequency != CPUFREQ_TABLE_END) {
		if (table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;
		if (table[i].frequency == freq)
			return i;
		i++;
	}
	return ret;
}
#endif

/* Bind callback functions for thermal zone */
static int sunxi_ths_bind(struct thermal_zone_device *thermal,
			struct thermal_cooling_device *cdev)
{
	int ret = 0, i;
	unsigned long lower, upper;
	unsigned long max_state = 0;
	struct sunxi_thermal_zone *ths_zone = thermal->devdata;
#ifdef CONFIG_SUNXI_BUDGET_COOLING
    int state_mismatch = 0;
    unsigned int lowest,highest;
#endif
	if (!strlen(ths_zone->sunxi_ths_sensor_conf->name))
		return -EINVAL;

	pr_debug("%s : %s", __func__, cdev->type);

	/* No matching cooling device */
	if (strcmp(ths_zone->sunxi_ths_sensor_conf->name, cdev->type) != 0)
		return 0;

	lower = THERMAL_NO_LIMIT;
	upper = THERMAL_NO_LIMIT;
	cdev->ops->get_max_state(cdev, &max_state);
#ifdef CONFIG_SUNXI_BUDGET_COOLING
    lowest = 0;
    highest = 0;
	for (i = 0; i < ths_zone->sunxi_ths_sensor_conf->trip_data->trip_count; i++) {
		if (ths_zone->sunxi_ths_sensor_conf->trip_data->trip_val[i] !=
			ths_zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].temp_level) {
			continue;
		}
		lower = ths_zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_min;
		upper = ths_zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_max;
	        if(lower >max_state || upper> max_state)
	        {
	            state_mismatch =1;
	            if(upper >highest)
	                highest = upper;
	        }
    	}
#endif
	/* Bind the thermal zone to the cpufreq cooling device */
	for (i = 0; i < ths_zone->sunxi_ths_sensor_conf->trip_data->trip_count; i++) {
		if (ths_zone->sunxi_ths_sensor_conf->trip_data->trip_val[i] !=
			ths_zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].temp_level) {
			continue;
		}

#ifndef CONFIG_SUNXI_BUDGET_COOLING

		lower = sunxi_get_frequency_level(0,
			ths_zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_max);

		lower = max_state - lower;

		upper = sunxi_get_frequency_level(0,
			ths_zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_min);

		upper = max_state - upper;
		pr_debug("ths_zone trip = %d, lower = %ld, upper = %ld\n",
			ths_zone->sunxi_ths_sensor_conf->trip_data->trip_val[i],
			lower, upper);
#else
	         lower = ths_zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_min;
	         upper = ths_zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_max;
	        if(state_mismatch)
	        {

	            lower = ((max_state+1)*(lower-lowest))/(highest-lowest+1);
	            upper = ((max_state+1)*(upper-lowest))/(highest-lowest+1);
	        }
	        lower=(lower>max_state)?0:lower;
	        upper=(upper>max_state)?max_state:upper;
			pr_debug("ths_zone trip = %d, adjust lower = %ld, upper = %ld\n",
				ths_zone->sunxi_ths_sensor_conf->trip_data->trip_val[i],
				lower, upper);
#endif
		if (thermal_zone_bind_cooling_device(thermal, i, cdev,
							upper, lower)) {
			 pr_err("error binding cdev inst %d\n", i);
			ret = -EINVAL;
		}
		ths_zone->bind = true;
	}
	return ret;
}

/* Unbind callback functions for thermal zone */
static int sunxi_ths_unbind(struct thermal_zone_device *thermal,
			struct thermal_cooling_device *cdev)
{
	int ret = 0, i;
	struct sunxi_thermal_zone *ths_zone = thermal->devdata;
	
	if (ths_zone->bind == false)
		return 0;

	/* Unbind the thermal zone to the cpufreq cooling device */
	for (i = 0; i < ths_zone->sunxi_ths_sensor_conf->trip_data->trip_count; i++) {
		if (thermal_zone_unbind_cooling_device(thermal, i,
								cdev)) {
			 pr_err("error unbinding cdev inst %d\n", i);
			ret = -EINVAL;
		}
		ths_zone->bind = false;
	}
	return ret;
}

/* Get trip type callback functions for thermal zone */
static int sunxi_ths_get_trip_type(struct thermal_zone_device *thermal, int trip,
				 enum thermal_trip_type *type)
{
	struct sunxi_thermal_zone *ths_zone = thermal->devdata;

	if ((trip < 0) || (trip > ths_zone->sunxi_ths_sensor_conf->trip_data->trip_count-1))
		return -EINVAL;

	if (trip == (ths_zone->sunxi_ths_sensor_conf->trip_data->trip_count-1))
		*type = THERMAL_TRIP_CRITICAL;
	else
		*type = THERMAL_TRIP_ACTIVE;
	return 0;
}

/* Get trip temperature callback functions for thermal zone */
static int sunxi_ths_get_trip_temp(struct thermal_zone_device *thermal, int trip,
				unsigned long *temp)
{
	struct sunxi_thermal_zone *ths_zone = thermal->devdata;

	if ((trip < 0) || (trip > ths_zone->sunxi_ths_sensor_conf->trip_data->trip_count-1))
		return -EINVAL;

	*temp = ths_zone->sunxi_ths_sensor_conf->trip_data->trip_val[trip];

	return 0;
}

/* Get critical temperature callback functions for thermal zone */
static int sunxi_ths_get_crit_temp(struct thermal_zone_device *thermal,
				unsigned long *temp)
{
	int ret;
	struct sunxi_thermal_zone *ths_zone = thermal->devdata;

	ret = sunxi_ths_get_trip_temp(thermal,
			(ths_zone->sunxi_ths_sensor_conf->trip_data->trip_count-1), temp);
	return ret;
}

/* Get mode callback functions for thermal zone */
static int sunxi_ths_get_mode(struct thermal_zone_device *thermal,
			enum thermal_device_mode *mode)
{
	struct sunxi_thermal_zone *ths_zone = thermal->devdata;

	if (ths_zone)
		*mode = ths_zone->mode;
	return 0;
}

/* Set mode callback functions for thermal zone */
static int sunxi_ths_set_mode(struct thermal_zone_device *thermal,
			enum thermal_device_mode mode)
{
	struct sunxi_thermal_zone *ths_zone = thermal->devdata;

	if (!ths_zone->therm_dev) {
		pr_notice("thermal zone not registered\n");
		return 0;
	}

	mutex_lock(&ths_zone->therm_dev->lock);

	if (mode == THERMAL_DEVICE_ENABLED)
		ths_zone->therm_dev->polling_delay = IDLE_INTERVAL;
	else
		ths_zone->therm_dev->polling_delay = 0;

	mutex_unlock(&ths_zone->therm_dev->lock);

	ths_zone->mode = mode;
	thermal_zone_device_update(ths_zone->therm_dev);
	pr_debug("thermal polling set for duration=%d msec\n",
				ths_zone->therm_dev->polling_delay);
	return 0;
}

static void sunxi_ths_update_trend_temp(struct sunxi_thermal_zone *ths_zone,unsigned long temp)
{
    ths_zone->ptrend->temp100[ths_zone->ptrend->index] = temp*100;
    ths_zone->ptrend->index = (ths_zone->ptrend->index+1)%TREND_SAMPLE_MAX;
    ths_zone->ptrend->cnt++;
    ths_zone->ptrend->cnt=(ths_zone->ptrend->cnt >TREND_SAMPLE_MAX)?TREND_SAMPLE_MAX:ths_zone->ptrend->cnt;
}
static int sunxi_ths_get_trend_real(struct sunxi_thermal_zone *ths_zone,unsigned long temp, unsigned long limit, unsigned long pre, unsigned long next)
{
    int i,j;
    int Y=0,X=0,XY=0,XX=0,M=ths_zone->ptrend->cnt;
    for(i=0;i<ths_zone->ptrend->cnt;i++)
    {
        if(ths_zone->ptrend->cnt < TREND_SAMPLE_MAX)
            j=i;
         else
            j = (i+ths_zone->ptrend->index)%TREND_SAMPLE_MAX;
        Y+= ths_zone->ptrend->temp100[j];
        X += (i+1);
        XY += ths_zone->ptrend->temp100[j]*(i+1);
        XX += (i+1)*(i+1);
    }
    /*if sample data too little, return STABLE */
    if(ths_zone->ptrend->cnt < TREND_SAMPLE_MAX)
            return THERMAL_TREND_STABLE;
    /*Avoid div zero */
    if(!M || !((M* XX - X*X)))
            return THERMAL_TREND_STABLE;
    ths_zone->ptrend->a1 = (int)(M*XY - X*Y)/(M* XX - X*X);
    ths_zone->ptrend->a0 = (int)(Y/M - ths_zone->ptrend->a1*X/M);
	if (temp >= limit)
    {
        if(next && temp >=next && ths_zone->ptrend->a1 >0)
            ths_zone->ptrend->trend = THERMAL_TREND_RAISE_FULL;
        else
        {
            if(ths_zone->ptrend->a1 >1)
                ths_zone->ptrend->trend = THERMAL_TREND_RAISING;
            else if(ths_zone->ptrend->a1 >-1)
            {
                if (temp >= limit+2)
                    ths_zone->ptrend->trend = THERMAL_TREND_STABLE;
                else
                    ths_zone->ptrend->trend = THERMAL_TREND_DROPPING;
            }
            else
                ths_zone->ptrend->trend =  THERMAL_TREND_STABLE;
        }
    }
    else
    {
        if(pre && temp < pre && ths_zone->ptrend->a1 <0)
            ths_zone->ptrend->trend = THERMAL_TREND_DROP_FULL;
        else
        {
            if(ths_zone->ptrend->a1 >1)
                ths_zone->ptrend->trend = THERMAL_TREND_RAISING;
            else if(ths_zone->ptrend->a1 >-1)
                ths_zone->ptrend->trend = THERMAL_TREND_DROPPING;
            else
                ths_zone->ptrend->trend =  THERMAL_TREND_STABLE;
        }
    }
    return ths_zone->ptrend->trend ;
}
/* Get temperature callback functions for thermal zone */
static int sunxi_ths_get_temp(struct thermal_zone_device *thermal,
			unsigned long *temp)
{
	struct sunxi_thermal_zone *ths_zone = thermal->devdata;

	*temp = (unsigned long)(ths_zone->sunxi_ths_sensor_conf->read_temperature(ths_zone->id));
    sunxi_ths_update_trend_temp(ths_zone,*temp);
	return 0;
}

/* Get the temperature trend */
static int sunxi_ths_get_trend(struct thermal_zone_device *thermal,
			int trip, enum thermal_trend *trend)
{
	int ret = 0;
	unsigned long trip_temp,trip_next,trip_pre;
    int simple_trend;
	struct sunxi_thermal_zone *ths_zone = thermal->devdata;

	ret = sunxi_ths_get_trip_temp(thermal, trip, &trip_temp);
	if(ret < 0)
        return ret;

	if (thermal->temperature >= trip_temp)
		simple_trend = THERMAL_TREND_RAISING;
	else
		simple_trend = THERMAL_TREND_DROPPING;

	ret = sunxi_ths_get_trip_temp(thermal, trip+1, &trip_next);
	if(ret < 0)
        trip_next = 0;
	ret = sunxi_ths_get_trip_temp(thermal, trip+1, &trip_pre);
	if(ret < 0)
        trip_pre = 0;
    if(!ths_zone->sunxi_ths_sensor_conf->trend)
        *trend = simple_trend;
    else
        *trend = sunxi_ths_get_trend_real(ths_zone,thermal->temperature,trip_temp, trip_pre,trip_next);

	return 0;
}

/* Operation callback functions for thermal zone */
static struct thermal_zone_device_ops const sunxi_ths_dev_ops = {
	.bind = sunxi_ths_bind,
	.unbind = sunxi_ths_unbind,
	.get_temp = sunxi_ths_get_temp,
	.get_trend = sunxi_ths_get_trend,
	.get_mode = sunxi_ths_get_mode,
	.set_mode = sunxi_ths_set_mode,
	.get_trip_type = sunxi_ths_get_trip_type,
	.get_trip_temp = sunxi_ths_get_trip_temp,
	.get_crit_temp = sunxi_ths_get_crit_temp,
};

/* Un-Register with the in-kernel thermal management */
void sunxi_ths_unregister_thermal(struct sunxi_thermal_zone *ths_zone)
{
	if (!ths_zone)
		return;
	if (ths_zone->therm_dev)
		thermal_zone_device_unregister(ths_zone->therm_dev);
        if(ths_zone->ptrend)
       {
            kfree(ths_zone->ptrend);
            ths_zone->ptrend = NULL;
       }
	pr_debug("suxi_ths: Kernel Thermal management unregistered\n");
}
EXPORT_SYMBOL(sunxi_ths_unregister_thermal);
int sunxi_ths_register_thermal(struct sunxi_thermal_zone *ths_zone)
{
	int ret = 0;
    ths_zone->ptrend =  kzalloc(sizeof(struct sunxi_thermal_zone_trend_info),GFP_KERNEL);
	ths_zone->therm_dev = thermal_zone_device_register(ths_zone->name,
		ths_zone->sunxi_ths_sensor_conf->trip_data->trip_count, 0, ths_zone, &sunxi_ths_dev_ops, NULL, 0,
		IDLE_INTERVAL);

	if (IS_ERR(ths_zone->therm_dev)) {
		pr_err("Failed to register thermal zone device\n");
		ret = -EINVAL;
		goto err_unregister;
	}
	ths_zone->mode = THERMAL_DEVICE_ENABLED;

	pr_debug("suxi_ths: Kernel Thermal management registered\n");
	return 0;

err_unregister:
	sunxi_ths_unregister_thermal(ths_zone);
	return ret;
}
EXPORT_SYMBOL(sunxi_ths_register_thermal);

#ifdef CONFIG_SUNXI_THERMAL_DYNAMIC
extern int sunxi_thermal_register_thermalbind(struct sunxi_thermal_zone *ths_zone);
extern int sunxi_thermal_unregister_thermalbind(struct sunxi_thermal_zone *ths_zone);
int sunxi_ths_register_thermal_dynamic(struct sunxi_thermal_zone *ths_zone)
{
	int ret = 0;
    ret = sunxi_ths_register_thermal(ths_zone);
    if(!ret)
       sunxi_thermal_register_thermalbind(ths_zone);
    return ret;
}
void sunxi_ths_unregister_thermal_dynamic(struct sunxi_thermal_zone *ths_zone)
{
    sunxi_thermal_unregister_thermalbind(ths_zone);
    sunxi_ths_unregister_thermal(ths_zone);
}
EXPORT_SYMBOL(sunxi_ths_register_thermal_dynamic);
EXPORT_SYMBOL(sunxi_ths_unregister_thermal_dynamic);
#endif

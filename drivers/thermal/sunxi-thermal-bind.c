/*
 * drivers/thermal/sunxi-thermal-bind.c
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
#include <linux/fs.h> 
#include <asm/io.h>
#include "sunxi-thermal.h"
#include <linux/miscdevice.h> 
#define THERMAL_BIND_NODE_MAX 8
#define MAX_BINDDEV           2

struct thermal_bind_nonde_t
{
    unsigned int temp;
    unsigned int min;
    unsigned int max;    
};
struct thermalbind_t
{
    unsigned int                trip_count;
    unsigned int                trip_trend;
    struct thermal_bind_nonde_t nodes[THERMAL_BIND_NODE_MAX];
    
};
struct thermalbind_device
{
    struct sunxi_thermal_zone*  zone;
    unsigned int                used;
    struct thermalbind_t        binddata;    
    struct miscdevice           miscdev;    
    struct device_attribute     devattr[2];
};  
////////////////////////////////////////////////////////////////////   
static ssize_t thermaltrend_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
    int ret=0,total=0;
    struct miscdevice* mdev = dev_get_drvdata(dev);
    struct thermalbind_device* binddev = container_of(mdev, struct thermalbind_device, miscdev);
    ret = sprintf(buf, "%d\n",binddev->zone->sunxi_ths_sensor_conf->trend);
    buf += (ret >0?ret:0);
    total += (ret >0?ret:0);
    return total;
}
static ssize_t thermaltrend_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
    struct miscdevice* mdev = dev_get_drvdata(dev);
    struct thermalbind_device* binddev = container_of(mdev, struct thermalbind_device, miscdev);

    sscanf(buf,"%u\n",&binddev->binddata.trip_trend);
    sunxi_ths_unregister_thermal(binddev->zone);
    binddev->zone->sunxi_ths_sensor_conf->trend = binddev->binddata.trip_trend;
    sunxi_ths_register_thermal(binddev->zone);
    return count;
}

static ssize_t thermalbind_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
    int i=0,ret=0,total=0;
    struct miscdevice* mdev = dev_get_drvdata(dev);
    struct thermalbind_device* binddev = container_of(mdev, struct thermalbind_device, miscdev);     
    
    ret = sprintf(buf, "count %d",binddev->zone->sunxi_ths_sensor_conf->trip_data->trip_count);
    buf += (ret >0?ret:0);
    total += (ret >0?ret:0);
    for(i=0;i<binddev->zone->sunxi_ths_sensor_conf->trip_data->trip_count;i++)
    {
      if(i < binddev->zone->sunxi_ths_sensor_conf->trip_data->trip_count -1)        
            ret = sprintf(buf, " trip%d %d %d %d",i,
                          binddev->zone->sunxi_ths_sensor_conf->trip_data->trip_val[i],
                          binddev->zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_min, 
                          binddev->zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_max);
      else
            ret = sprintf(buf, " trip%d %d %d %d\n",i,
                          binddev->zone->sunxi_ths_sensor_conf->trip_data->trip_val[i],0,0);  
      buf += (ret >0?ret:0);
      total += (ret >0?ret:0);
    } 
    return total;
}  
static ssize_t thermalbind_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{ 
    int i=0;
    struct miscdevice* mdev = dev_get_drvdata(dev);
    struct thermalbind_device* binddev = container_of(mdev, struct thermalbind_device, miscdev);   
    
    binddev->binddata.trip_count = 0;
    sscanf(buf,"%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
            &binddev->binddata.trip_count,
            &binddev->binddata.nodes[0].temp,
            &binddev->binddata.nodes[0].min,
            &binddev->binddata.nodes[0].max,
            &binddev->binddata.nodes[1].temp,
            &binddev->binddata.nodes[1].min,
            &binddev->binddata.nodes[1].max,
            &binddev->binddata.nodes[2].temp,
            &binddev->binddata.nodes[2].min,
            &binddev->binddata.nodes[2].max,
            &binddev->binddata.nodes[3].temp,
            &binddev->binddata.nodes[3].min,
            &binddev->binddata.nodes[3].max,
            &binddev->binddata.nodes[4].temp,
            &binddev->binddata.nodes[4].min,
            &binddev->binddata.nodes[4].max,
            &binddev->binddata.nodes[5].temp,
            &binddev->binddata.nodes[5].min,
            &binddev->binddata.nodes[5].max,
            &binddev->binddata.nodes[6].temp,
            &binddev->binddata.nodes[6].min,
            &binddev->binddata.nodes[6].max,
            &binddev->binddata.nodes[7].temp,
            &binddev->binddata.nodes[7].min,
            &binddev->binddata.nodes[7].max);
    if(!binddev->binddata.trip_count ||binddev->binddata.trip_count >THERMAL_BIND_NODE_MAX)
        return -1;       
        
    sunxi_ths_unregister_thermal(binddev->zone);
    binddev->zone->sunxi_ths_sensor_conf->trip_data->trip_count = binddev->binddata.trip_count;
    
    for(i=0;i<binddev->binddata.trip_count;i++)
    {
      binddev->zone->sunxi_ths_sensor_conf->trip_data->trip_val[i]= binddev->binddata.nodes[i].temp;
      if(i < binddev->binddata.trip_count -1)
      {
          binddev->zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_min = binddev->binddata.nodes[i].min;
          binddev->zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].freq_clip_max = binddev->binddata.nodes[i].max;
          binddev->zone->sunxi_ths_sensor_conf->cooling_data->freq_data[i].temp_level= binddev->binddata.nodes[i].temp;
      } 
    }
    sunxi_ths_register_thermal(binddev->zone);  
    return count;
}  

static struct thermalbind_device bindlist[]=
{
    {.zone    = NULL,
     .used    = 0,
     .miscdev = {  
                .minor  =   MISC_DYNAMIC_MINOR,  
                .name   =   "thermalbind0",  
            },
     .devattr ={
                {
                    .attr = {
                        .name = "bind",
                        .mode = 0644,
                    },
                    .show = thermalbind_show,
                    .store= thermalbind_store,
                },
                {
                    .attr = {
                        .name = "trend",
                        .mode = 0644,
                    },
                    .show = thermaltrend_show,
                    .store= thermaltrend_store,
                },
            },            
    },
    {.zone    = NULL,
     .used    = 0,
     .miscdev = {  
                .minor  =   MISC_DYNAMIC_MINOR,  
                .name   =   "thermalbind1",  
            },
     .devattr ={
                {
                    .attr = {
                        .name = "bind",
                        .mode = 0644,
                    },
                    .show = thermalbind_show,
                    .store= thermalbind_store,
                },
                {
                    .attr = {
                        .name = "trend",
                        .mode = 0644,
                    },
                    .show = thermaltrend_show,
                    .store= thermaltrend_store,
                },
        },
    },
};
static unsigned int bindmax=sizeof(bindlist)/sizeof(struct thermalbind_device);


int sunxi_thermal_register_thermalbind(struct sunxi_thermal_zone *ths_zone)
{
    int i=0,ret=0;

    while (i < bindmax)
    {
        if(!bindlist[i].used)
            break;
        i++;
    }
    if(i >= bindmax)
        return -1;
    bindlist[i].zone = ths_zone;    
    bindlist[i].used = 1;
    ret = misc_register(&bindlist[i].miscdev); 
	if(ret) {
		pr_err("%s register sunxi debug register driver as misc device error\n", __func__);
		goto exit;
	}
	ret=device_create_file(bindlist[i].miscdev.this_device, &bindlist[i].devattr[0]);
	if(ret)
		pr_err("%s sysfs_create_group  error\n", __func__);
	ret=device_create_file(bindlist[i].miscdev.this_device, &bindlist[i].devattr[1]);
	if(ret)
		pr_err("%s sysfs_create_group  error\n", __func__);
exit:
	return ret;    
}
int sunxi_thermal_unregister_thermalbind(struct sunxi_thermal_zone *ths_zone)
{
    int i=0;

    while (i < bindmax)
    {
        if(bindlist[i].zone == ths_zone)
            break;
        i++;
    }
    if(i >= bindmax)
        return -1;
    misc_deregister(&(bindlist[i].miscdev)); 
    bindlist[i].used = 0;
	device_remove_file(bindlist[i].miscdev.this_device, &bindlist[i].devattr[1]);
	device_remove_file(bindlist[i].miscdev.this_device, &bindlist[i].devattr[0]);
    return 0;
}
EXPORT_SYMBOL(sunxi_thermal_unregister_thermalbind);
EXPORT_SYMBOL(sunxi_thermal_register_thermalbind);

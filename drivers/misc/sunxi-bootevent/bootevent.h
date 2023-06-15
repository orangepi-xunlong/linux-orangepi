/*
 * Copyright(c) 2017-2018 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#ifndef _BOOTEVENT_H_
#define _BOOTEVENT_H_
#ifdef CONFIG_SCHEDSTATS
extern void log_boot(char *str);
#else
#define log_boot(str)
#endif

#include <linux/sched.h>
#ifndef TIME_LOG_START
#define TIME_LOG_START() \
	({ts = sched_clock(); })
#endif

#ifndef TIME_LOG_END
#define TIME_LOG_END() \
	({ts = sched_clock() - ts; })
#endif

#include <linux/platform_device.h>
#include <linux/sched/clock.h>
void bootevent_initcall(initcall_t fn, unsigned long long ts);
void bootevent_probe(unsigned long long ts, struct device *dev,
		    struct device_driver *drv, unsigned long probe);
void bootevent_pdev_register(unsigned long long ts,
			    struct platform_device *pdev);
#endif

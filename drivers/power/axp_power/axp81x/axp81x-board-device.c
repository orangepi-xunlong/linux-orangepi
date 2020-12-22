/*
 * PMU init driver for allwinnertech AXP81X
 *
 * Copyright (C) 2014 ALLWINNERTECH.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/err.h>

static struct platform_device axp81x_platform_device = {
	.name		    = "axp81x_board",
	.id		        = PLATFORM_DEVID_NONE,
};

static int __init axp81x_board_device_init(void)
{
	int ret = 0;

#ifdef	CONFIG_AXP_TWI_USED
#else
	ret = platform_device_register(&axp81x_platform_device);
	if (IS_ERR_VALUE(ret)) {
			printk("register axp81x platform device failed\n");
			return ret;
	}
#endif
        return ret;
}

subsys_initcall(axp81x_board_device_init);

MODULE_DESCRIPTION("ALLWINNERTECH axp board device");
MODULE_AUTHOR("Ming Li");
MODULE_LICENSE("GPL");


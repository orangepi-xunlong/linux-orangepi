/*
 * w1-gpio-board.c - Board driver for GPIO w1 bus master
 *
 * Copyright (C) 2016 FriendlyARM (www.friendlyarm.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/w1-gpio.h>
#include <linux/gpio.h>

#include <mach/platform.h>

static int gpio = -1;
module_param(gpio, int, 0644);


static struct w1_gpio_platform_data w1_gpio_pdata = {
	.pin		= -1,
	.is_open_drain	= 0,
};

static void w1_device_release(struct device *dev) {
	// nothing here yet
}

static struct platform_device matrix_w1_device = {
	.name		= "w1-gpio",
	.id			= -1,
	.dev		= {
		.release	= w1_device_release,
		.platform_data	= &w1_gpio_pdata,
	}
};


static int __init w1_gpio_board_init(void)
{
	w1_gpio_pdata.pin = gpio;
	printk("plat: add device w1-gpio, gpio=%d\n", w1_gpio_pdata.pin);
	return platform_device_register(&matrix_w1_device);
}

static void __exit w1_gpio_board_exit(void)
{
	platform_device_unregister(&matrix_w1_device);
}

module_init(w1_gpio_board_init);
module_exit(w1_gpio_board_exit);

MODULE_DESCRIPTION("Board driver for GPIO w1 bus master");
MODULE_AUTHOR("FriendlyARM");
MODULE_LICENSE("GPL");

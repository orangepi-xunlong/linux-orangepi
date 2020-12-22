/*
 * Copyright (C) 2013 Allwinnertech
 * Heming <lvheming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <mach/gpio.h>
#include <mach/sys_config.h>


struct gpio_led gpio_leds[] = {
{
        .name                   = "default_led",
        .default_trigger        = "none",
        .gpio                   = 0xffff,
        .retain_state_suspended = 1,
        }, {
        .name                   = "default_led",
        .default_trigger        = "none",
        .gpio                   = 0xffff,
        .retain_state_suspended = 1,
        }, {
        .name                   = "default_led",
        .default_trigger        = "none",
        .gpio                   = 0xffff,
        .retain_state_suspended = 1,
        },
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds		= gpio_leds,
	.num_leds	= 0,
};

static struct platform_device sunxi_leds = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_led_info,
	}
};

static int sunxi_leds_fetch_sysconfig_para(void)
{
        int num = 0,i = 0;
        script_item_u   val;
        script_item_value_type_e  type;
        static char* led_name[3] = {"red_led","green_led","blue_led"};
        char led_active_low[25];

        type = script_get_item("leds_para", "leds_used", &val);
        if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
                printk(KERN_ERR "%s script_parser_fetch \"leds_para\" leds_used = %d\n",
                __FUNCTION__, val.val);
                goto script_get_err;
        }

        if(!val.val) {
                printk(KERN_ERR"%s leds is not used in config\n", __FUNCTION__);
                goto script_get_err;
        }
        for(i = 0,num = 0;i < ARRAY_SIZE(gpio_leds);i++){
                type = script_get_item("leds_para", led_name[i], &val);
                if(SCIRPT_ITEM_VALUE_TYPE_PIO != type)
                        printk(KERN_ERR "no %s, ignore it!\n",led_name[i]);
                else {
                        gpio_leds[num].name = led_name[i];
                        gpio_leds[num].gpio = val.gpio.gpio;
                        gpio_leds[num].default_state = val.gpio.data ? LEDS_GPIO_DEFSTATE_ON : LEDS_GPIO_DEFSTATE_OFF;
                        sprintf(led_active_low,"%s_active_low", led_name[i]);
                        type = script_get_item("leds_para", led_active_low, &val);
                        if(SCIRPT_ITEM_VALUE_TYPE_INT != type) {
                                printk(KERN_ERR "no %s, %s failed!\n",led_active_low,__FUNCTION__);
                                goto script_get_err;
                        } else 
                                gpio_leds[num].active_low = val.val ? true : false;
                        num++;
                }
        }
        return num;
script_get_err:
        pr_notice("=========script_get_err============\n");
        return -1;
}

static int __init sunxi_leds_init(void)
{
        int led_num = 0,ret = 0;
        led_num = sunxi_leds_fetch_sysconfig_para();
        if(led_num > 0){
                gpio_led_info.num_leds = led_num;
                ret = platform_device_register(&sunxi_leds);
        }
        return ret;
}

static void __exit sunxi_leds_exit(void)
{
        if(gpio_led_info.num_leds > 0){
                platform_device_unregister(&sunxi_leds);
        }
}

module_init(sunxi_leds_init);
module_exit(sunxi_leds_exit);

MODULE_DESCRIPTION("sunxi leds driver");
MODULE_AUTHOR("Heming");
MODULE_LICENSE("GPL");




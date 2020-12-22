/*
 * SoftWinners 3G module core Linux support
 *
 * Copyright (C) 2012 SoftWinners Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kmemcheck.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/signal.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif

#include <linux/time.h>
#include <linux/timer.h>
#include <linux/input.h>
#include <linux/ioport.h>
#include <linux/io.h>

#include <mach/platform.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>

#include "sw_module.h"

//#define  SW_3G_GPIO_WAKEUP_SYSTEM

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void sw_module_mdelay(u32 time)
{
	mdelay(time);
}
EXPORT_SYMBOL(sw_module_mdelay);

s32 modem_get_config(struct sw_modem *modem)
{
    script_item_value_type_e type = 0;
    script_item_u item_temp;

    /* 3g_used */
    type = script_get_item("3g_para", "3g_used", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        modem->used = item_temp.val;
    }else{
        modem_err("ERR: get 3g_used failed\n");
        modem->used = 0;
    }

    /* 3g_usbc_num */
    type = script_get_item("3g_para", "3g_usbc_num", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        modem->usbc_no = item_temp.val;
    }else{
        modem_err("ERR: get 3g_usbc_num failed\n");
        modem->usbc_no = 0;
    }

    /* 3g_uart_num */
    type = script_get_item("3g_para", "3g_uart_num", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        modem->uart_no = item_temp.val;
    }else{
        modem_err("ERR: get 3g_uart_num failed\n");
        modem->uart_no = 0;
    }

    /* bb_name */
    type = script_get_item("3g_para", "bb_name", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
        strcpy(modem->name, item_temp.str);
        modem_dbg("%s modem support\n", modem->name);
    }else{
        modem_err("ERR: get bb_name failed\n");
        modem->name[0] = 0;
    }

    /* bb_vbat */
    type = script_get_item("3g_para", "bb_vbat", &modem->bb_vbat.pio);
    if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
        modem->bb_vbat.valid = 1;
    }else{
        modem_err("ERR: get bb_vbat failed\n");
        modem->bb_vbat.valid = 0;
    }

    /* bb_pwr_on */
    type = script_get_item("3g_para", "bb_pwr_on", &modem->bb_pwr_on.pio);
    if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
        modem->bb_pwr_on.valid = 1;
    }else{
        modem_err("ERR: get bb_pwr_on failed\n");
        modem->bb_pwr_on.valid  = 0;
    }

    /* bb_rst */
    type = script_get_item("3g_para", "bb_rst", &modem->bb_rst.pio);
    if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
        modem->bb_rst.valid = 1;
    }else{
        modem_err("ERR: get bb_rst failed\n");
        modem->bb_rst.valid  = 0;
    }

    /* bb_rf_dis */
    type = script_get_item("3g_para", "bb_rf_dis", &modem->bb_rf_dis.pio);
    if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
        modem->bb_rf_dis.valid = 1;
    }else{
        modem_err("ERR: get bb_rf_dis failed\n");
        modem->bb_rf_dis.valid  = 0;
    }

    /* bb_wake_ap */
    type = script_get_item("wakeup_src_para", "bb_wake_ap", &modem->bb_wake_ap.pio);
    if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
        modem->bb_wake_ap.valid = 1;
    }else{
        modem_err("ERR: get bb_wake_ap failed\n");
        modem->bb_wake_ap.valid  = 0;
    }

    /* bb_wake */
    type = script_get_item("3g_para", "bb_wake", &modem->bb_wake.pio);
    if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
        modem->bb_wake.valid = 1;
    }else{
        modem_err("ERR: get bb_wake failed\n");
        modem->bb_wake.valid  = 0;
    }

	/* bb_dldo */
    type = script_get_item("3g_para", "bb_dldo", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
        strcpy(modem->dldo_name, item_temp.str);
        modem_dbg("%s modem support\n", modem->dldo_name);
    }else{
        modem_err("ERR: get dldo_name failed\n");
    }

    /* bb_dldo_min_uV */
    type = script_get_item("3g_para", "bb_dldo_min_uV", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        modem->dldo_min_uV = item_temp.val;
    }else{
        modem_err("ERR: get bb_dldo_min_uV failed\n");
        modem->dldo_min_uV = 0;
    }

    /* bb_dldo_max_uV */
    type = script_get_item("3g_para", "bb_dldo_max_uV", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        modem->dldo_max_uV = item_temp.val;
    }else{
        modem_err("ERR: get bb_dldo_max_uV failed\n");
        modem->dldo_max_uV = 0;
    }

    return 0;
}
EXPORT_SYMBOL(modem_get_config);

s32 modem_pin_init(struct sw_modem *modem)
{
    int ret = 0;
	char pin_name[255];
	unsigned long config;

    //---------------------------------
    //  bb_vbat
    //---------------------------------
    if(modem->bb_vbat.valid){
        ret = gpio_request(modem->bb_vbat.pio.gpio.gpio, "bb_vbat");
        if(ret != 0){
            modem_err("gpio_request bb_vbat failed\n");
            modem->bb_vbat.valid = 0;
        }else{
            /* set config, ouput */
			gpio_direction_output(modem->bb_vbat.pio.gpio.gpio, modem->bb_vbat.pio.gpio.data);
            //sw_gpio_setcfg(modem->bb_vbat.pio.gpio.gpio, GPIO_CFG_OUTPUT);
        }
    }

    //---------------------------------
    //  bb_pwr_on
    //---------------------------------
    if(modem->bb_pwr_on.valid){
        ret = gpio_request(modem->bb_pwr_on.pio.gpio.gpio, "bb_pwr_on");
        if(ret != 0){
            modem_err("gpio_request bb_pwr_on failed\n");
            modem->bb_pwr_on.valid = 0;
        }else{
            /* set config, ouput */
			gpio_direction_output(modem->bb_pwr_on.pio.gpio.gpio, modem->bb_pwr_on.pio.gpio.data);
            //sw_gpio_setcfg(modem->bb_pwr_on.pio.gpio.gpio, GPIO_CFG_OUTPUT);
        }
    }

    //---------------------------------
    //  bb_rst
    //---------------------------------
    if(modem->bb_rst.valid){
        ret = gpio_request(modem->bb_rst.pio.gpio.gpio, "bb_rst");
        if(ret != 0){
            modem_err("gpio_request bb_rst failed\n");
            modem->bb_rst.valid = 0;
        }else{
            /* set config, ouput */
			gpio_direction_output(modem->bb_rst.pio.gpio.gpio, modem->bb_rst.pio.gpio.data);
            //sw_gpio_setcfg(modem->bb_rst.pio.gpio.gpio, GPIO_CFG_OUTPUT);
        }
    }

    //---------------------------------
    //  bb_wake
    //---------------------------------
    if(modem->bb_wake.valid){
        ret = gpio_request(modem->bb_wake.pio.gpio.gpio, "bb_wake");
        if(ret != 0){
            modem_err("gpio_request bb_wake failed\n");
            modem->bb_wake.valid = 0;
        }else{
            /* set config, ouput */
			gpio_direction_output(modem->bb_wake.pio.gpio.gpio, modem->bb_wake.pio.gpio.data);
            //sw_gpio_setcfg(modem->bb_wake.pio.gpio.gpio, GPIO_CFG_OUTPUT);
        }
    }

    //---------------------------------
    //  bb_rf_dis
    //---------------------------------
    if(modem->bb_rf_dis.valid){
        ret = gpio_request(modem->bb_rf_dis.pio.gpio.gpio, "bb_rf_dis");
        if(ret != 0){
            modem_err("gpio_request bb_rf_dis failed\n");
            modem->bb_rf_dis.valid = 0;
        }else{
            /* set config, ouput */
			gpio_direction_output(modem->bb_rf_dis.pio.gpio.gpio, modem->bb_rf_dis.pio.gpio.data);
            //sw_gpio_setcfg(modem->bb_rf_dis.pio.gpio.gpio, GPIO_CFG_OUTPUT);
        }
    }

    return 0;
}

EXPORT_SYMBOL(modem_pin_init);

s32 modem_pin_exit(struct sw_modem *modem)
{
    //---------------------------------
    //  bb_vbat
    //---------------------------------
    if(modem->bb_vbat.valid){
        gpio_free(modem->bb_vbat.pio.gpio.gpio);
        modem->bb_vbat.valid = 0;
    }

    //---------------------------------
    //  bb_pwr_on
    //---------------------------------
    if(modem->bb_pwr_on.valid){
        gpio_free(modem->bb_pwr_on.pio.gpio.gpio);
        modem->bb_pwr_on.valid = 0;
    }

    //---------------------------------
    //  bb_rst
    //---------------------------------
    if(modem->bb_rst.valid){
        gpio_free(modem->bb_rst.pio.gpio.gpio);
        modem->bb_rst.valid = 0;
    }

    //---------------------------------
    //  bb_wake
    //---------------------------------
    if(modem->bb_wake.valid){
        gpio_free(modem->bb_wake.pio.gpio.gpio);
        modem->bb_wake.valid = 0;
    }

    //---------------------------------
    //  bb_rf_dis
    //---------------------------------
    if(modem->bb_rf_dis.valid){
        gpio_free(modem->bb_rf_dis.pio.gpio.gpio);
        modem->bb_rf_dis.valid = 0;
    }

    return 0;
}
EXPORT_SYMBOL(modem_pin_exit);

void modem_vbat(struct sw_modem *modem, u32 value)
{
    if(!modem->bb_vbat.valid){
        return;
    }

    __gpio_set_value(modem->bb_vbat.pio.gpio.gpio, value);

    return;
}
EXPORT_SYMBOL(modem_vbat);

/* modem reset delay is 100 */
void modem_reset(struct sw_modem *modem, u32 value)
{
    if(!modem->bb_rst.valid){
        return;
    }

    __gpio_set_value(modem->bb_rst.pio.gpio.gpio, value);

    return;
}
EXPORT_SYMBOL(modem_reset);

void modem_sleep(struct sw_modem *modem, u32 value)
{
    if(!modem->bb_wake.valid){
        return;
    }

    __gpio_set_value(modem->bb_wake.pio.gpio.gpio, value);

    return;
}
EXPORT_SYMBOL(modem_sleep);

void modem_dldo_on_off(struct sw_modem *modem, u32 on)
{
	struct regulator *ldo = NULL;
	int ret = 0;

    if(modem->dldo_name[0] == 0 || modem->dldo_min_uV == 0 || modem->dldo_max_uV == 0){
        modem_err("err: dldo parameter is invalid. dldo_name=%s, dldo_min_uV=%d, dldo_max_uV=%d.\n",
                  modem->dldo_name, modem->dldo_min_uV, modem->dldo_max_uV);
        return;
    }

    /* set ldo */
	ldo = regulator_get(NULL, modem->dldo_name);
	if (!ldo) {
		modem_err("get power regulator failed.\n");
		return;
	}

	if(on){
		regulator_set_voltage(ldo, modem->dldo_min_uV, modem->dldo_max_uV);
		ret = regulator_enable(ldo);
		if(ret < 0){
			modem_err("regulator_enable failed.\n");
			regulator_put(ldo);
			return;
		}
	}else{
		ret = regulator_disable(ldo);
		if(ret < 0){
			modem_err("regulator_disable failed.\n");
			regulator_put(ldo);
			return;
		}
	}

	regulator_put(ldo);

	return;
}
EXPORT_SYMBOL(modem_dldo_on_off);

void modem_power_on_off(struct sw_modem *modem, u32 value)
{
    if(!modem->bb_pwr_on.valid){
        return;
    }

    __gpio_set_value(modem->bb_pwr_on.pio.gpio.gpio, value);

    return;
}
EXPORT_SYMBOL(modem_power_on_off);

void modem_rf_disable(struct sw_modem *modem, u32 value)
{
    if(!modem->bb_rf_dis.valid){
        return;
    }

    __gpio_set_value(modem->bb_rf_dis.pio.gpio.gpio, value);

    return;
}
EXPORT_SYMBOL(modem_rf_disable);

#ifdef  SW_3G_GPIO_WAKEUP_SYSTEM
static int modem_create_input_device(struct sw_modem *modem)
{
    int ret = 0;

    modem->key = input_allocate_device();
    if (!modem->key) {
        modem_err("err: not enough memory for input device\n");
        return -ENOMEM;
    }

    modem->key->name          = "sw_3g_modem";
    modem->key->phys          = "modem/input0";
    modem->key->id.bustype    = BUS_HOST;
    modem->key->id.vendor     = 0xf001;
    modem->key->id.product    = 0xffff;
    modem->key->id.version    = 0x0100;

    modem->key->evbit[0] = BIT_MASK(EV_KEY);
    set_bit(KEY_POWER, modem->key->keybit);

    ret = input_register_device(modem->key);
    if (ret) {
        modem_err("err: input_register_device failed\n");
        input_free_device(modem->key);
        return -ENOMEM;
    }

    return 0;
}

static int modem_free_input_device(struct sw_modem *modem)
{
    if(modem->key){
        input_unregister_device(modem->key);
        input_free_device(modem->key);
    }

    return 0;
}

/* 通知android，唤醒系统 */
static void modem_wakeup_system(struct sw_modem *modem)
{
    modem_dbg("---------%s modem wakeup system----------\n", modem->name);

    input_report_key(modem->key, KEY_POWER, 1);
    input_sync(modem->key);
    msleep(100);
    input_report_key(modem->key, KEY_POWER, 0);
    input_sync(modem->key);

    return ;
}
#endif

static void modem_irq_enable(struct sw_modem *modem)
{
	
	enable_irq(modem->irq_hd);

    return;
}

static void modem_irq_disable(struct sw_modem *modem)
{
	
	disable_irq(modem->irq_hd);

    return;
}

#ifdef  SW_3G_GPIO_WAKEUP_SYSTEM
static void modem_irq_work(struct work_struct *data)
{
	struct sw_modem *modem = container_of(data, struct sw_modem, irq_work);

	modem_wakeup_system(modem);

	return;
}
#endif

static irqreturn_t modem_irq_interrupt(int irq, void *para)
{
	struct sw_modem *modem = (struct sw_modem *)para;
    int result = 0;

    //modem_irq_disable(modem);

#ifdef  SW_3G_GPIO_WAKEUP_SYSTEM
    if(result){
        schedule_work(&modem->irq_work);
    }
#endif

	return IRQ_HANDLED;
}

int modem_irq_init(struct sw_modem *modem, unsigned long trig_type)
{
#ifdef  SW_3G_GPIO_WAKEUP_SYSTEM
    int ret = 0;
	

    ret = modem_create_input_device(modem);
    if(ret != 0){
        modem_err("err: modem_create_input_device failed\n");
        return -1;
    }

	INIT_WORK(&modem->irq_work, modem_irq_work);
#endif
	int ret;
    modem->trig_type = trig_type;
	/*
    modem->irq_hd = sw_gpio_irq_request(modem->bb_wake_ap.pio.gpio.gpio,
                                        modem->trig_type,
                                        modem_irq_interrupt,
                                        modem);
	*/
	modem->irq_hd = gpio_to_irq(modem->bb_wake_ap.pio.gpio.gpio);
	ret = request_irq(modem->irq_hd, modem_irq_interrupt, modem->trig_type, "SoftWinner 3G Module", modem);
	
    	if(IS_ERR_VALUE(ret)){
        modem_err("err: request_irq failed\n");
#ifdef  SW_3G_GPIO_WAKEUP_SYSTEM
        modem_free_input_device(modem);
#endif
        return -1;
    }

    modem_irq_disable(modem);

    return 0;
}
EXPORT_SYMBOL(modem_irq_init);

int modem_irq_exit(struct sw_modem *modem)
{
    if(!modem->bb_wake_ap.valid){
        return -1;
    }

	modem_irq_disable(modem);
	free_irq(modem->irq_hd, modem);

    cancel_work_sync(&modem->irq_work);
#if 0
    modem_free_input_device(modem);
#endif
    return 0;
}
EXPORT_SYMBOL(modem_irq_exit);

void modem_early_suspend(struct sw_modem *modem)
{
    modem_irq_enable(modem);

    return;
}
EXPORT_SYMBOL(modem_early_suspend);

void modem_early_resume(struct sw_modem *modem)
{
    modem_irq_disable(modem);

    return;
}
EXPORT_SYMBOL(modem_early_resume);



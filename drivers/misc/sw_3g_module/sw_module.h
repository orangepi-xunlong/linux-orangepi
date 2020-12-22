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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define  modem_dbg(format,args...)      printk("[sw_module]: "format,##args)
#define  modem_err(format,args...)      printk("[sw_module]: "format,##args)

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#define SW_3G_NAME_LEN	128
#define SW_DRIVER_NAME          "SoftWinner 3G Module"
#define SW_DEVICE_NODE_NAME     "modem"

#define MODULE_GPIO_PULL_DISABLE  0
#define MODULE_GPIO_PULL_UP       1
#define MODULE_GPIO_PULL_DOWN     2

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
struct sw_modem;

struct sw_module_pio{
    u32 valid;                      /* pin is vlaid? */
    script_item_u pio;              /* pin info */
};

struct sw_modem_ops{
	void (*power) (struct sw_modem *modem, u32 on);
	void (*reset) (struct sw_modem *modem);
	void (*sleep) (struct sw_modem *modem, u32 sleep);
	void (*rf_disable) (struct sw_modem *modem, u32 disable);

	int (*start) (struct sw_modem *modem);
	int (*stop) (struct sw_modem *modem);

	void (*early_suspend) (struct sw_modem *modem);
	void (*early_resume) (struct sw_modem *modem);

	int (*suspend) (struct sw_modem *modem);
	int (*resume) (struct sw_modem *modem);
};

struct sw_modem{
    char name[SW_3G_NAME_LEN];
    u8 start;

    struct work_struct irq_work;
    u32 irq_hd;                         /* ÖÐ¶Ï¾ä±ú     */
    unsigned long trig_type;  /* ÖÐ¶Ï´¥·¢·½Ê½ */
    struct input_dev *key;

    u32 used;
    u32 usbc_no;                        /* ¹ÒÔØµÄUSB¿ØÖÆÆ÷±àºÅ */
    u32 uart_no;                        /* ¹ÒÔØµÄuart¿ØÖÆÆ÷ */

    struct sw_module_pio bb_vbat;
    struct sw_module_pio bb_pwr_on;
    struct sw_module_pio bb_rst;
    struct sw_module_pio bb_rf_dis;
    struct sw_module_pio bb_wake;
    struct sw_module_pio bb_wake_ap;

	char dldo_name[SW_3G_NAME_LEN];
	u32 dldo_min_uV;
    u32 dldo_max_uV;

    struct sw_modem_ops *ops;
    void *prv;                          /* private data, eg. struct sw_module_dev */
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void sw_module_mdelay(u32 time);
s32 modem_get_config(struct sw_modem *modem);
s32 modem_pin_init(struct sw_modem *modem);
s32 modem_pin_exit(struct sw_modem *modem);
void modem_vbat(struct sw_modem *modem, u32 on);
void modem_reset(struct sw_modem *modem, u32 time);
void modem_sleep(struct sw_modem *modem, u32 sleep);
void modem_power_on_off(struct sw_modem *modem, u32 up);
void modem_rf_disable(struct sw_modem *modem, u32 disable);
int modem_irq_init(struct sw_modem *modem, unsigned long trig_type);
int modem_irq_exit(struct sw_modem *modem);
void modem_early_suspend(struct sw_modem *modem);
void modem_early_resume(struct sw_modem *modem);
void modem_dldo_on_off(struct sw_modem *modem, u32 on);


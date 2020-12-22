/*
 * arch/arm/mach-sun7i/powernow.c
 * (c) Copyright 2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * James Deng <csjamesdeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <mach/powernow.h>

#define POWERNOW_LEVEL              3
#define MODE_NAME_LEN               16

#if (POWERNOW_LEVEL == 1)
    #define powernow_dbg(format, args...)  do {} while (0)
    #define powernow_inf(format, args...)  do {} while (0)
    #define powernow_err(format, args...)  printk(KERN_ERR "[powernow err] "format, ##args)
#elif (POWERNOW_LEVEL == 2)
    #define powernow_dbg(format, args...)  do {} while (0)
    #define powernow_inf(format, args...)  printk(KERN_INFO"[powernow inf] "format, ##args)
    #define powernow_err(format, args...)  printk(KERN_ERR "[powernow err] "format, ##args)
#elif (POWERNOW_LEVEL == 3)
    #define powernow_dbg(format, args...)  printk(KERN_INFO"[powernow dbg] "format, ##args)
    #define powernow_inf(format, args...)  printk(KERN_INFO"[powernow inf] "format, ##args)
    #define powernow_err(format, args...)  printk(KERN_ERR "[powernow err] "format, ##args)
#endif

BLOCKING_NOTIFIER_HEAD(sw_powernow_notifier_list);

struct sw_powernow_mode {
    int code;
    char *name;
};

static struct sw_powernow_mode powernow_modes[] = {
    { .code = SW_POWERNOW_EXTREMITY,    .name = "extremity"     },
    { .code = SW_POWERNOW_PERFORMANCE,  .name = "performance"   },
// these modes below are implemented by cpufreq_fantasys in a23 platform
#ifndef CONFIG_ARCH_SUN8IW3
    { .code = SW_POWERNOW_NORMAL,       .name = "normal"        },
    { .code = SW_POWERNOW_USEREVENT,    .name = "userevent"     },
    { .code = SW_POWERNOW_USB,          .name = "usb"           },
    { .code = SW_POWERNOW_MAXPOWER,     .name = "maxpower"      },
#endif
};
static int cur_mode = -1;
static int usb_status = SW_POWERNOW_USBSTAT_INACTIVE;

DEFINE_MUTEX(mode_mutex);

/*
 * register function to be called at sw_powernow mode changed.
 */
int register_sw_powernow_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&sw_powernow_notifier_list, nb);
}
EXPORT_SYMBOL(register_sw_powernow_notifier);

int unregister_sw_powernow_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&sw_powernow_notifier_list, nb);
}
EXPORT_SYMBOL(unregister_sw_powernow_notifier);

void sw_powernow_switch_to(int mode)
{
    int target_mode = mode;
    mutex_lock(&mode_mutex);

    if (mode == SW_POWERNOW_USEREVENT || mode == SW_POWERNOW_MAXPOWER) {
        if (cur_mode == SW_POWERNOW_EXTREMITY ||
            usb_status == SW_POWERNOW_USBSTAT_ACTIVE){
            goto out;
        }
    }
    if (usb_status == SW_POWERNOW_USBSTAT_ACTIVE && target_mode != SW_POWERNOW_EXTREMITY){
        target_mode = SW_POWERNOW_USB;
    }

    blocking_notifier_call_chain(&sw_powernow_notifier_list,
            powernow_modes[target_mode].code, powernow_modes[target_mode].name);
    if (mode != SW_POWERNOW_USEREVENT 
        && mode != SW_POWERNOW_MAXPOWER 
        && mode != SW_POWERNOW_USB) {
        cur_mode = mode;
    }

out:
    mutex_unlock(&mode_mutex);
}
EXPORT_SYMBOL(sw_powernow_switch_to);

void sw_powernow_set_usb(int status)
{
    usb_status = status;
    sw_powernow_switch_to(cur_mode);
}
EXPORT_SYMBOL(sw_powernow_set_usb);

static ssize_t mode_show(struct class *class, struct class_attribute *attr,
        char *buf)
{
    int mode = cur_mode;
    if (usb_status == SW_POWERNOW_USBSTAT_ACTIVE){
        mode = SW_POWERNOW_USB;
    }
    if (mode < 0 || mode >= ARRAY_SIZE(powernow_modes)) {
        return sprintf(buf, "<unknown>\n");
    }
    return sprintf(buf, "%s\n", powernow_modes[mode].name);
}

static ssize_t mode_store(struct class *class, struct class_attribute *attr,
        const char *buf, size_t count)
{
    unsigned int ret = -EINVAL;
    int i;
    char mode_name[MODE_NAME_LEN];

    ret = sscanf(buf, "%15s", mode_name);
    if (ret != 1) {
        return -EINVAL;
    }

    ret = -EINVAL;
    for (i = 0; i < ARRAY_SIZE(powernow_modes); i++) {
        if (strcmp(mode_name, powernow_modes[i].name)) {
            continue;
        }

        if (i == SW_POWERNOW_NORMAL){
            i = SW_POWERNOW_PERFORMANCE;
        }
        
        if (cur_mode != i) {
            sw_powernow_switch_to(i);
        }

        ret = count;
        break;
    }

    return ret;
}

static ssize_t usb_stat_show(struct class *class, struct class_attribute *attr,
        char *buf)
{
    return sprintf(buf, "%d\n", usb_status);
}

static ssize_t usb_stat_store(struct class *class, struct class_attribute *attr,
        const char *buf, size_t count)
{
    unsigned int ret = -EINVAL;
    unsigned int input;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1) {
        return -EINVAL;
    }

    sw_powernow_set_usb(input);
    return count;
}


static ssize_t available_modes_show(struct class *class, struct class_attribute *attr,
        char *buf)
{
    int i;
	ssize_t len = 0;

    for (i = 0; i < ARRAY_SIZE(powernow_modes); i++) {
        len += scnprintf(&buf[len], MODE_NAME_LEN, "%s ", powernow_modes[i].name);
    }
    len += sprintf(&buf[len], "\n");

    return len;
}

static struct class_attribute class_attrs[] = {
    __ATTR(mode, 0660, mode_show, mode_store),
    __ATTR(usb_stat, 0660, usb_stat_show, usb_stat_store),
    __ATTR(available_modes, 0660, available_modes_show, NULL),
    __ATTR_NULL,
};

static struct class sw_powernow_class = {
    .name           = "sw_powernow",
    .owner          = THIS_MODULE,
    .class_attrs    = class_attrs,
};

static int __init sw_powernow_init(void)
{
    int ret;

    powernow_inf("module init\n");
    ret = class_register(&sw_powernow_class);
    if (ret < 0) {
        powernow_err("create class sw_powernow failed\n");
    } else {
        powernow_inf("create class sw_powernow done\n");
    }

    return ret;
}

static void __exit sw_powernow_exit(void)
{
    powernow_inf("module exit\n");
}

module_init(sw_powernow_init);
module_exit(sw_powernow_exit);

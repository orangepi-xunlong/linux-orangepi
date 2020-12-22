/*
 * linux/arch/arm/mach-sunxi/sunxi_bootup_extend.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * allwinner sunxi platform bootup extend code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/reboot.h>

#include <asm/mcpm.h>
#include <asm/proc-fns.h>
#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/cp15.h>
#include <asm/smp_plat.h>
#include <asm/hardware/gic.h>
#include <linux/arisc/hwspinlock.h>

#include <mach/platform.h>
#include <mach/sys_config.h>
#include <mach/sunxi-chip.h>
#include <mach/sunxi-smc.h>

static int bootup_extend_enabel = 0;

#define GENERAL_RTC_REG_MAX   (0x03)
#define GENERAL_RTC_REG_MIN   (0x01)

#if (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN9IW1P1)
#define __RTC_REG    (SUNXI_R_PRCM_VBASE + 0x1F0)
#endif

#if (defined CONFIG_ARCH_SUN8IW7P1)
#define __RTC_REG  (SUNXI_RTC_VBASE + 0x100)
#endif

static int __hwspin_lock_timeout(int hwid, unsigned int timeout, \
                                 spinlock_t *lock, unsigned long *flags)
{
#ifdef CONFIG_SUNXI_ARISC
	return arisc_hwspin_lock_timeout(hwid, timeout, lock, flags);
#else
    #error "no arisc driver!"
	return -ESRCH;
#endif
}

static int __hwspin_unlock(int hwid, spinlock_t *lock, unsigned long *flags)
{
#ifdef CONFIG_SUNXI_ARISC
	return arisc_hwspin_unlock(hwid, lock, flags);
#else
    #error "no arisc driver!"
	return -ESRCH;
#endif
}

static DEFINE_SPINLOCK(rtc_hw_lock);

static int __rtc_reg_write(u32 addr, u32 data)
{
    u32 temp = 0;
    unsigned long hwflags;

    if ((addr < GENERAL_RTC_REG_MIN) || (addr > GENERAL_RTC_REG_MAX)) {
        printk(KERN_ERR "%s: rtc address error, address:0x%x\n", __func__, addr);
        return -1;
    }

    if (data > 0xff) {
        printk(KERN_ERR "%s: rtc data error, data:0x%x\n", __func__, data);
        return -1;
    }

    if (!__hwspin_lock_timeout(AW_RTC_REG_HWSPINLOCK, 100, &rtc_hw_lock, &hwflags)) {
#if (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN9IW1P1)
        temp = (addr << 16) | (data << 8);
        sunxi_smc_writel(temp, __RTC_REG);
        temp |= 1 << 31;
        sunxi_smc_writel(temp, __RTC_REG);
        temp &= ~(1<< 31);
        sunxi_smc_writel(temp, __RTC_REG);
#endif

#if (defined CONFIG_ARCH_SUN8IW7P1)
        sunxi_smc_writel(data, (__RTC_REG + 0x4 * addr));
#endif
        __hwspin_unlock(AW_RTC_REG_HWSPINLOCK, &rtc_hw_lock, &hwflags);
        printk(KERN_DEBUG "%s: write rtc reg success, rtc reg 0x%x:0x%x\n", __func__, addr, data);
        return 0;
    }

    printk(KERN_DEBUG "%s: get hwspinlock unsuccess\n", __func__);
    return -1;
}

void sunxi_bootup_extend_fix(unsigned int *cmd)
{
    if (bootup_extend_enabel==1) {
        if (*cmd == LINUX_REBOOT_CMD_POWER_OFF) {
            __rtc_reg_write(3, 0x1);
            *cmd = LINUX_REBOOT_CMD_RESTART;
            printk(KERN_INFO "%s: enter boot standby\n", __func__);
        } else if (*cmd == LINUX_REBOOT_CMD_RESTART ||
                   *cmd == LINUX_REBOOT_CMD_RESTART2 ) {
            __rtc_reg_write(3, 0xf);
        }
    }
}
EXPORT_SYMBOL(sunxi_bootup_extend_fix);

static int __init sunxi_bootup_extend_init(void)
{
    script_item_value_type_e item_type;
    script_item_u item;

    item_type = script_get_item("box_start_os", "used", &item);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != item_type) {
        printk(KERN_INFO "%s: bootup extend disable(default)\n", __func__);
        bootup_extend_enabel = 0;
        return -1;
    }
    bootup_extend_enabel = item.val;
    printk(KERN_INFO "%s: bootup extend state %d\n", __func__, bootup_extend_enabel);

    return 0;
}
late_initcall(sunxi_bootup_extend_init);

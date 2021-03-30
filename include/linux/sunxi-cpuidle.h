/*
 * include/linux/sunxi-cpuidle.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *
 * Author: Pan Nan <pannan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SUNXI_CPUIDLE_H__
#define __SUNXI_CPUIDLE_H__

#define BOOT_CPU_HP_FLAG_REG		(0xb8)
#define CPU_SOFT_ENTRY_REG0		(0xbc)
#define CPUCFG_CPUIDLE_EN_REG		(0x140)
#define CPUCFG_CORE_FLAG_REG		(0x144)
#define CPUCFG_PWR_SWITCH_DELAY_REG	(0x150)

#define GIC_DIST_PBASE			(0x01C81000)

extern void sunxi_idle_cpux_flag_init(void);
extern void sunxi_cpux_entry_judge(void);
extern void sunxi_idle_cpux_flag_set(unsigned int cpu, int hotplug);
extern void sunxi_idle_cpux_flag_valid(unsigned int cpu, int value);

#endif /* __SUNXI_CPUIDLE_H__ */

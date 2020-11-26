/*
 *  arch/arm/mach-sun6i/arisc/arisc_i.h
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
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
 *
 */

#ifndef __ARISC_I_H__
#define __ARISC_I_H__

#include "./include/arisc_includes.h"
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#include <linux/reboot.h>
#include <linux/arisc/arisc.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/arisc/arisc-notifier.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/power/axp_depend.h>
#include <linux/sunxi-sid.h>
#include <linux/arm-smccc.h>

#define DRV_NAME    "sunxi-arisc"
#define DEV_NAME    "sunxi-arisc"

#define DRV_VERSION "3.00"

#define CPUS_ENABLE_POWER_EXP   (1<<31)
#define CPUS_WAKEUP_POWER_STA   (1<<1)
#define CPUS_WAKEUP_POWER_CSM   (1<<0)

/* used for arisc */
struct sst_power_info_para {
	/*
	 * define for sun9iw1p1
	 * power_reg bit0 ~ 7 AXP_main REG10, bit 8~15 AXP_main REG12
	 * power_reg bit16~23 AXP_slav REG10, bit24~31 AXP_slav REG11
	 *
	 * AXP_main REG10: 0-off, 1-on
	 * bit7   bit6   bit5   bit4   bit3   bit2   bit1   bit0
	 * aldo2  aldo1  dcdc5  dcdc4  dcdc3  dcdc2  dcdc1  dc5ldo
	 *
	 * REG12:
	 * bit0~5:0-off, 1-on,
	 * bit6~7: 0-on, 1-off, dc1sw's power come from dcdc1
	 * bit7   bit6   bit5   bit4   bit3   bit2   bit1   bit0
	 * dc1sw  swout  aldo3  dldo2  dldo1  eldo3  eldo2  eldo1
	 *
	 * AXP_slave REG10: 0-off, 1-on. dcdc b&c is not used, ignore them.
	 * bit7   bit6   bit5   bit4   bit3   bit2   bit1   bit0
	 * aldo3  aldo2  aldo1  dcdce  dcdcd  dcdcc  dcdcb  dcdca
	 *
	 * AXP_slave REG11: 0-off, 1-on
	 * bit7   bit6   bit5   bit4   bit3   bit2   bit1   bit0
	 * swout  cldo3  cldo2  cldo1  bldo4  bldo3  bldo2  bldo1
	 */
	/*
	 * define for sun8iw5p1
	 * power_reg0 ~ 7 AXP_main REG10,  8~15 AXP_main REG12
	 * power_reg16~32 null
	 *
	 * AXP_main REG10: 0-off, 1-on
	 * bit7   bit6	 bit5   bit4   bit3   bit2   bit1   bit0
	 * aldo2  aldo1  dcdc5  dcdc4  dcdc3  dcdc2  dcdc1  dc5ldo
	 *
	 * REG12: 0-off, 1-on
	 * bit7   bit6   bit5   bit4   bit3   bit2   bit1   bit0
	 * dc1sw  dldo4  dldo3  dldo2  dldo1  eldo3  eldo2  eldo1
	 *
	 * REG13: bit16 aldo3, 0-off, 1-on
	 * REG90: bit17 gpio0/ldo, 0-off, 1-on
	 * REG92: bit18 gpio1/ldo, 0-off, 1-on
	 */
	/* enable bit */
	/* registers of power state should be */
	/* the max power of system, signed,
	* power mabe negative when charge */
	unsigned int enable;
	unsigned int power_reg;
	signed int system_power;
};

/* used for arisc */
typedef struct sst_dram_info {
	unsigned int dram_crc_enable;
	unsigned int dram_crc_src;
	unsigned int dram_crc_len;
	unsigned int dram_crc_error;
	unsigned int dram_crc_total_count;
	unsigned int dram_crc_error_count;
} sst_dram_info_t;

/* used for arisc */
typedef struct standby_info_para {
	struct sst_power_info_para power_state; /* size 3W=12B */
	sst_dram_info_t dram_state; /*size 6W=24B */
} standby_info_para_t;

struct super_standby_para {
	/* cpus wakeup event types */
	unsigned int event;
	/* cpux resume code src */
	unsigned int resume_code_src;
	/* cpux resume code length */
	unsigned int resume_code_length;
	/* cpux resume entry */
	unsigned int resume_entry;
#if (defined CONFIG_ARCH_SUN8IW12P1) ||\
	(defined CONFIG_ARCH_SUN8IW15)
		/* kernel cpu_resume entry */
	unsigned int cpu_resume_entry;
	#endif
#if (defined CONFIG_ARCH_SUN50IW6P1)
	unsigned int hdmi_cec_phyaddr;
#endif
	/* wakeup after timeout seconds */
	unsigned int timeout;
	unsigned int gpio_enable_bitmap;
	unsigned int cpux_gpiog_bitmap;
	unsigned int pextended_standby;
};


extern unsigned int arisc_debug_dram_crc_en;
extern unsigned int arisc_debug_dram_crc_srcaddr;
extern unsigned int arisc_debug_dram_crc_len;
extern unsigned int arisc_debug_dram_crc_error;
extern unsigned int arisc_debug_dram_crc_total_count;
extern unsigned int arisc_debug_dram_crc_error_count;
extern unsigned int arisc_debug_level;
extern struct standby_info_para arisc_powchk_back;

extern void arisc_power_off(void);
extern char *arisc_binary_start;
extern char *arisc_binary_end;
/* local functions */
extern int arisc_config_dram_paras(void);
extern int arisc_sysconfig_ir_paras(void);
extern int arisc_config_pmu_paras(void);
extern int arisc_suspend_flag_query(void);
#ifdef CONFIG_ARM
int invoke_scp_fn_smc(u32 function_id, u32 arg0, u32 arg1, u32 arg2);
#endif
#ifdef CONFIG_ARM64
int invoke_scp_fn_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2);
#endif

#endif  /* __ARISC_I_H__ */

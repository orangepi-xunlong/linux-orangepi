#ifndef _PM_O_H
#define _PM_O_H

/*
 * Copyright (c) 2011-2015 njubie@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include "pm.h"
#include <linux/module.h>
#include <linux/suspend.h>
#include <asm/suspend.h>
#include <linux/cpufreq.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/device.h>
#include <linux/console.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/cpu_pm.h>
#include <asm/system_misc.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#include <linux/power/aw_pm.h>
#include <asm/cacheflush.h>
#include <linux/arisc/arisc.h>

#include <linux/power/scenelock.h>
#include <linux/kobject.h>
#include <linux/ctype.h>
#include <linux/regulator/consumer.h>
#include <linux/power/axp_depend.h>
#include "../../../../kernel/power/power.h"

/*#include "mem_mapping.h"*/
#define  print_call_info(...)({                                                                         \
		do {                                                                                                                     \
		printk("%s, %s, %d. \n" , __FILE__, __func__, __LINE__);\
		} while (0) })
		;

/*#define CROSS_MAPPING_STANDBY*/

#define AW_PM_DBG   1
/*#undef PM_DBG*/
#if (AW_PM_DBG)
#define PM_DBG(format, args...)   printk("[pm]"format, ##args)
#else
#define PM_DBG(format, args...)   do {} while (0)
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#define PRE_DISABLE_MMU		/*actually, mean ,prepare condition to disable mmu */
#endif

#ifdef ENTER_SUPER_STANDBY
#undef PRE_DISABLE_MMU
#endif

#ifdef ENTER_SUPER_STANDBY_WITH_NOMMU
#define PRE_DISABLE_MMU		/*actually, mean ,prepare condition to disable mmu */
#endif

#ifdef RETURN_FROM_RESUME0_WITH_MMU
#undef PRE_DISABLE_MMU
#endif

#ifdef WATCH_DOG_RESET
#define PRE_DISABLE_MMU		/*actually, mean ,prepare condition to disable mmu */
#endif

/*#define VERIFY_RESTORE_STATUS*/

/* define major number for power manager */
#define AW_PMU_MAJOR    267

#define pm_printk(mask, format, args...)   do	{   \
    if (unlikely(debug_mask & mask)) {		    \
	printk(KERN_INFO format, ##args);   \
    }					    \
} while (0)

extern unsigned long cpu_brom_start;
extern char *standby_bin_start;
extern char *standby_bin_end;
extern char *suspend_bin_start;
extern char *suspend_bin_end;

#ifdef RESUME_FROM_RESUME1
extern char *resume1_bin_start;
extern char *resume1_bin_end;
#endif

/*mem_cpu_asm.S*/
extern int mem_arch_suspend(void);
extern int mem_arch_resume(void);
extern unsigned int disable_prefetch(void);
extern void restore_prefetch(unsigned int);
extern asmlinkage int mem_clear_runtime_context(void);
extern void save_runtime_context(__u32 *addr);
extern void clear_reg_context(void);

#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
extern void cpufreq_user_event_notify(void);
#endif

#ifdef	CONFIG_AW_AXP
extern void axp_powerkey_set(int value);
#endif

extern int standby_mode;
extern const extended_standby_manager_t *extended_standby_manager_id;

extern standby_space_cfg_t standby_space;
extern unsigned long time_to_wakeup;
extern struct aw_pm_info standby_info;
extern void init_wakeup_src(unsigned int event, unsigned int gpio_enable_bitmap, unsigned int cpux_gpiog_bitmap);
extern void exit_wakeup_src(unsigned int event, unsigned int gpio_enable_bitmap, unsigned int cpux_gpiog_bitmap);
extern int suspend_freq;
extern int suspend_delay_ms;
extern __mem_tmr_reg_t saved_tmr_state;
extern __u32 debug_mask;

extern ssize_t parse_status_code_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size);

extern ssize_t debug_mask_show(struct device *dev,
			       struct device_attribute *attr, char *buf);

extern ssize_t debug_mask_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

extern void aw_pm_dev_status(char *name, int mask);
extern void aw_pm_show_dev_status(void);
extern ssize_t parse_status_code_show(struct device *dev,
				      struct device_attribute *attr, char *buf);

/*
 * platform dependant interface.
 */

extern void mem_device_init(void);
extern void mem_device_save(void);
extern void mem_device_restore(void);
extern int aw_standby_enter(unsigned long arg);
extern void query_wakeup_source(struct aw_pm_info *arg);
extern int fetch_and_save_dram_para(dram_para_t *pstandby_dram_para);

extern int axp_mem_save(void);
extern void axp_mem_restore(void);
extern int resume_sys_pwr_state(void);
extern int check_pwr_status(void);
extern int init_sys_pwr_dm(void);
extern int config_pmu_para(void);
extern int config_dynamic_standby(void);
extern int config_sys_pwr(void);

extern void sunxi_pinctrl_state_show(void);

#endif /*_PM_O_H*/

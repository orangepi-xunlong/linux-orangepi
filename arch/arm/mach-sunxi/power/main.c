/*
 * SUNXI suspend
 *
 * Copyright (C) 2014 AllWinnertech Ltd.
 * Author: xiafeng <xiafeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpu_pm.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/time.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/device.h>
#include <linux/console.h>
#include <linux/syscore_ops.h>
#include <linux/clk/sunxi_name.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#include <asm/mach/map.h>
#include <mach/sys_config.h>

#include <asm/smp_plat.h>
#include <asm/delay.h>
#include <asm/cp15.h>
#include <asm/suspend.h>
#include <asm/cacheflush.h>
#include <asm/hardware/gic.h>
#include <mach/cci.h>
#include <asm/mcpm.h>

#include <linux/arisc/arisc.h>
#include <linux/power/aw_pm.h>
#include <linux/power/scenelock.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sunxi-smc.h>

#include "./brom/resumes.h"

#define SUNXI_PM_DBG   1
#ifdef PM_DBG
#undef PM_DBG
#endif
#if SUNXI_PM_DBG
    #define PM_DBG(format,args...)   printk("[pm]"format,##args)
#else
    #define PM_DBG(format,args...)   do{}while(0)
#endif

standby_type_e standby_type = NON_STANDBY;
EXPORT_SYMBOL(standby_type);
standby_level_e standby_level = STANDBY_INITIAL;
EXPORT_SYMBOL(standby_level);

static unsigned long debug_mask = 0;
module_param_named(debug_mask, debug_mask, ulong, S_IRUGO | S_IWUSR);

static unsigned long time_to_wakeup = 0;
module_param_named(time_to_wakeup, time_to_wakeup, ulong, S_IRUGO | S_IWUSR);

static unsigned int wakeup_events = 0;
module_param_named(wakeup_events, wakeup_events, uint, S_IRUGO);

extern char sunxi_bromjump_start;
extern char sunxi_bromjump_end;
static const extended_standby_manager_t *est_manager_id = NULL;

extern unsigned long cpu_brom_addr[2];

static inline int sunxi_mem_get_status(void)
{
	return sunxi_smc_readl(IO_ADDRESS(SUNXI_RTC_PBASE + 0x104));
}

static inline void sunxi_mem_set_status(int val)
{
	sunxi_smc_writel(val, IO_ADDRESS(SUNXI_RTC_PBASE + 0x104));
	asm volatile ("dsb");
	asm volatile ("isb");
}

/**
 * sunxi_pm_valid() - determine if given system sleep state is supported
 *
 * @state: suspend state
 * @return: if the state is valid, return 1, else return 0
 */
static int sunxi_pm_valid(suspend_state_t state)
{
	if (!((state > PM_SUSPEND_ON) && (state < PM_SUSPEND_MAX))) {
		PM_DBG("state:%d invalid!\n", state);
		return 0;
	}
	PM_DBG("state:%d valid\n", state);

	if (PM_SUSPEND_STANDBY == state)
		standby_type = SUPER_STANDBY;
	else if (PM_SUSPEND_MEM == state || PM_SUSPEND_BOOTFAST == state)
		standby_type = NORMAL_STANDBY;

	return 1;
}

/**
 * sunxi_pm_begin() - Initialise a transition to given system sleep state
 *
 * @state: suspend state
 * @return: return 0 for process successed
 * @note: this function will be called before devices suspened
 */
static int sunxi_pm_begin(suspend_state_t state)
{
	static bool backup_console_suspend_enabled = 0;
	static bool backup_initcall_debug = 0;
	static int backup_console_loglevel = 0;
	static int backup_debug_mask = 0;
	static int fail_num_back = 0;
	static int need_write_back = 0;

	if (fail_num_back < suspend_stats.fail) {
		printk(KERN_WARNING "last suspend err!, rtc:%x\n", \
		       sunxi_mem_get_status());
		/* adjust: loglevel, console_suspend, initcall_debug.
		 * set debug_mask, disable console suspend if last suspend fail
		 */
		backup_console_suspend_enabled = console_suspend_enabled;
		console_suspend_enabled = 0;
		backup_initcall_debug = initcall_debug;
		initcall_debug = 1;
		backup_console_loglevel = console_loglevel;
		console_loglevel = 8;
		backup_debug_mask = debug_mask;
		debug_mask |= 0x0f;
		fail_num_back = suspend_stats.fail;
		need_write_back = 1;
	} else if (need_write_back) {
		/* restore console suspend, initcall_debug, debug_mask, if
		 * suspend_stats.success + .fail is 0 means the first time enter
		 * suspend, all the back data is 0.
		 */
		console_suspend_enabled = backup_console_suspend_enabled;
		initcall_debug = backup_initcall_debug;
		console_loglevel = backup_console_loglevel;
		debug_mask = backup_debug_mask;
		need_write_back = 0;
	}

	sunxi_mem_set_status(SUSPEND_BEGIN | 0x01);

	return 0;
}

/**
 * sunxi_pm_prepare() - Prepare for entering the system suspend state
 *
 * @return: return 0 for process successed, and negative code for error
 * @note: called after devices suspended, and before device late suspend
 *             call-back functions
 */
static int sunxi_pm_prepare(void)
{
	sunxi_mem_set_status(SUSPEND_BEGIN | 0x03);

	return 0;
}

/**
 * sunxi_pm_prepare_late() - Finish preparing for entering the system suspend
 *
 * @return: return 0 for process successed, and negative code for error
 * @note: called before disabling nonboot CPUs and after device
 *        drivers' late suspend callbacks have been executed
 */
static int sunxi_pm_prepare_late(void)
{
	sunxi_mem_set_status(SUSPEND_BEGIN | 0x05);

	return 0;
}

static inline int sunxi_suspend_cpu_die(void)
{
	unsigned long actlr;

	gic_cpu_exit(0);
	/* step1: disable cache */
	asm("mrc    p15, 0, %0, c1, c0, 0" : "=r" (actlr) );
	actlr &= ~(1<<2);
	asm("mcr    p15, 0, %0, c1, c0, 0\n" : : "r" (actlr));

	/* step2: clean and ivalidate L1 cache */
	flush_cache_all();
	outer_flush_all();

	/* step3: execute a CLREX instruction */
	asm("clrex" : : : "memory", "cc");

	/* step4: switch cpu from SMP mode to AMP mode,
	 * aim is to disable cache coherency */
	asm("mrc    p15, 0, %0, c1, c0, 1" : "=r" (actlr) );
	actlr &= ~(1<<6);
	asm("mcr    p15, 0, %0, c1, c0, 1\n" : : "r" (actlr));

	/* step5: execute an ISB instruction */
	isb();
	/* step6: execute a DSB instruction  */
	dsb();

	/*
	 * step7: if define trustzone, switch to secure os
	 * and then enter to wfi mode
	 */
#ifdef CONFIG_SUNXI_TRUSTZONE
	call_firmware_op(suspend);
#else
	/* step7: execute a WFI instruction */
	asm("wfi" : : : "memory", "cc");
#endif

    return 0;
}

static struct sram_para st_sram_para;

/**
 * sunxi_suspend_enter() - enter suspend state
 *
 * @val: useless
 * @return: no return if success
 */
static int sunxi_suspend_enter(unsigned long val)
{
	int i;
	super_standby_para_t st_para;

	sunxi_mem_set_status(SUSPEND_ENTER | 0x03);

	standby_level = STANDBY_WITH_POWER_OFF;

	memset((void *)&st_para, 0, sizeof(super_standby_para_t));

	st_para.event = CPUS_MEM_WAKEUP;
	st_para.event |= CPUS_WAKEUP_IR;
#ifdef CONFIG_USB_SUSPEND
	st_para.event |= CPUS_WAKEUP_USBMOUSE;
#endif
	st_para.timeout = time_to_wakeup;
	if (st_para.timeout > 0)
		st_para.event |= CPUS_WAKEUP_TIMEOUT;
	st_para.gpio_enable_bitmap = 0;
	st_para.cpux_gpiog_bitmap = 0;
	st_para.pextended_standby = NULL;
	/* the wakeup src is independent of the scene_lock. the developer only
	 * need to care about: the scene support the wakeup src
	 */
#ifdef CONFIG_SCENELOCK
	est_manager_id = get_extended_standby_manager();
	if (NULL != est_manager_id) {
		st_para.event |= est_manager_id->event;
		st_para.gpio_enable_bitmap = est_manager_id->wakeup_gpio_map;
		st_para.cpux_gpiog_bitmap = est_manager_id->wakeup_gpio_group;
		if (est_manager_id->pextended_standby)
			st_para.pextended_standby = (extended_standby_t *) \
				virt_to_phys(est_manager_id->pextended_standby);
	}
#endif
	/* set cpu0 entry address */
#if (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN9IW1P1)
	mcpm_set_entry_vector(0, 0, cpu_resume);
	st_para.resume_entry = virt_to_phys(&sunxi_bromjump_start);
	st_para.resume_code_src = virt_to_phys(mcpm_entry_point);
	st_para.resume_code_length = sizeof(unsigned long);
	PM_DBG("cpu resume:%x, mcpm enter:%x\n",
	       cpu_resume, virt_to_phys(mcpm_entry_point));
#else
	//cpu_brom_addr[0] = cpu_resume;
	st_para.resume_entry = virt_to_phys(&sunxi_bromjump_start);
	st_sram_para.resume_code_src = virt_to_phys(cpu_resume);
	st_para.resume_code_src = virt_to_phys(&st_sram_para);
	st_para.resume_code_length = sizeof(struct sram_para);
	PM_DBG("cpu resume:%x\n", virt_to_phys(cpu_resume));
#endif
	if (unlikely(debug_mask)) {
		printk(KERN_INFO "standby paras:\n" \
		       "  event:%x\n" \
		       "  resume_code_src:%x\n" \
		       "  resume_entry:%x\n" \
		       "  timeout:%u\n" \
		       "  gpio_enable_bitmap:%x\n" \
		       "  cpux_gpiog_bitmap:%x\n" \
		       "  pextended_standby:%p\n", \
		       (unsigned int)st_para.event,
		       (unsigned int)st_para.resume_code_src,
		       (unsigned int)st_para.resume_entry,
		       (unsigned int)st_para.timeout,
		       (unsigned int)st_para.gpio_enable_bitmap,
		       (unsigned int)st_para.cpux_gpiog_bitmap,
		       st_para.pextended_standby);
#ifdef CONFIG_SCENELOCK
		extended_standby_show_state();
#endif
		printk(KERN_INFO "system environment paras:\n");
		for (i = 0; i < st_para.resume_code_length; i += 4)
			printk(KERN_INFO "[%03d],%08x\n", i, \
			       readl((int *)((u32)&st_sram_para + i)));
	}

#ifdef CONFIG_SUNXI_ARISC
	arisc_standby_super(&st_para, NULL, NULL);
	sunxi_suspend_cpu_die();
#else
	asm("wfe" : : : "memory", "cc");
#endif

	return 0;
}

#ifdef CONFIG_SUNXI_TRUSTZONE
static unsigned long sunxi_back_gic_secure(struct regs_restore *regback)
{
	int i, num = 0;

	/* back GICC_CTLR & GICC_PMR */
	for (i = 0; i < 0x08; i += 4, regback++) {
		regback->addr = SUNXI_GIC_CPU_PBASE + i;
		regback->value = sunxi_smc_readl(SUNXI_GIC_CPU_VBASE + 0x0 + i);
		num++;
	}

	/* back GICD_CTLR */
	regback->addr = SUNXI_GIC_DIST_PBASE;
	regback->value = sunxi_smc_readl(SUNXI_GIC_DIST_VBASE);
	regback++;

	/* back GICD_IGROUPRn(0~5, the max irq number is 156, 156/32=5 ) */
	for (i = 0; i < 0x18; i += 4, regback++) {
		regback->addr = SUNXI_GIC_DIST_PBASE + 0x80 + i;
		regback->value = sunxi_smc_readl(SUNXI_GIC_DIST_VBASE + 0x80 + i);
		num++;
	}

	return num * sizeof(unsigned int);
}
#endif

/**
 * sunxi_pm_enter() - Enter the system sleep state
 *
 * @state: suspend state
 * @return: return 0 is process successed
 * @note: the core function for platform sleep
 */
static int sunxi_pm_enter(suspend_state_t state)
{
	sunxi_mem_set_status(SUSPEND_ENTER | 0x01);
	memset(&st_sram_para, 0, sizeof(struct sram_para));
#ifdef CONFIG_SUNXI_TRUSTZONE
	/* note: switch to secureos and save monitor vector to mem_para_info. */
	st_sram_para.monitor_vector = call_firmware_op(suspend_prepare);
	printk("hsr: monitor_vector %lx\n", st_sram_para.monitor_vector);
	call_firmware_op(get_cp15_status, (void *)virt_to_phys((void *)&( \
	                 st_sram_para.saved_secure_mmu_state)));
	st_sram_para.regs_num = sunxi_back_gic_secure(st_sram_para.regs_back);
#endif

	cpu_pm_enter();
	//cpu_cluster_pm_enter();
	cpu_suspend(0, sunxi_suspend_enter);
	//cpu_cluster_pm_enter();
	cpu_pm_exit();

	return 0;
}

/**
 * sunxi_pm_wake() - platform wakeup
 *
 * @return: called when the system has just left a sleep state,
 *          after the nonboot CPUs have been enabled and before
 *          device drivers' early resume callbacks are executed.
 */
static void sunxi_pm_wake(void)
{
	sunxi_mem_set_status(AFTER_LATE_RESUME);
}

/**
 * sunxi_pm_finish() - Finish wake-up of the platform
 *
 * @return: called prior to calling device drivers' regular suspend callbacks
 */
static void sunxi_pm_finish(void)
{
	sunxi_mem_set_status(RESUME_COMPLETE_FLAG);
}

/**
 * sunxi_pm_end() - Notify the platform that system is in work mode now
 *
 * @note: called after resuming devices, to indicate the
 *        platform that the system has returned to the working state or the
 *        transition to the sleep state has been aborted.
 */
static void sunxi_pm_end(void)
{
	sunxi_mem_set_status(RESUME_COMPLETE_FLAG);
#ifdef CONFIG_SUNXI_ARISC
	arisc_query_wakeup_source(&wakeup_events);
#endif
}

/**
 * sunxi_pm_recover() - Recover platform from a suspend failure
 *
 * @note: called by the PM core if the suspending of devices fails.
 */
static void sunxi_pm_recover(void)
{
	printk(KERN_WARNING "suspend failure!\n");

	sunxi_mem_set_status(SUSPEND_FAIL_FLAG);
}


/* define platform_suspend_ops call-back functions, registered into PM core */
static struct platform_suspend_ops sunxi_pm_ops = {
    .valid = sunxi_pm_valid,
    .begin = sunxi_pm_begin,
    .prepare = sunxi_pm_prepare,
    .prepare_late = sunxi_pm_prepare_late,
    .enter = sunxi_pm_enter,
    .wake = sunxi_pm_wake,
    .finish = sunxi_pm_finish,
    .end = sunxi_pm_end,
    .recover = sunxi_pm_recover,
};

/* for back ahb configuration */
static unsigned long pll_cpu_back;
static unsigned int cpuclk_config;
static unsigned int ahbclk_config;

static int sunxi_pm_syscore_suspend(void)
{
	struct clk *pll_cpu = NULL;

	/* back CPU AHB1/APB1 Configuration Register */
	cpuclk_config = sunxi_smc_readl(IO_ADDRESS(SUNXI_CCM_PBASE + 0x50));
	ahbclk_config = sunxi_smc_readl(IO_ADDRESS(SUNXI_CCM_PBASE + 0x54));

	pll_cpu = clk_get(NULL, PLL_CPU_CLK);
	if(IS_ERR(pll_cpu)){
		printk(KERN_ERR "back pll_cpu failed!\n");
		return -EINVAL;
	}
	pll_cpu_back = clk_get_rate(pll_cpu);
	clk_put(pll_cpu);

	if (unlikely(debug_mask)) {
		printk(KERN_INFO "cpu pll&clk config:%lu %x ahb clk config:%x\n", \
		       pll_cpu_back, cpuclk_config, ahbclk_config);
	}

	return 0;
}

static void sunxi_pm_syscore_resume(void)
{
	unsigned long value;
	struct clk *pll_cpu = NULL;

	sunxi_mem_set_status(CLK_RESUME_START);
	/* restore CPU Configuration Register form low to high frequency */
	value = sunxi_smc_readl(IO_ADDRESS(SUNXI_CCM_PBASE + 0x50));
	/* set CPU_APB_CLK_DIV, bit8~9 */
	value &= ~(0x03 << 8);
	value |= (cpuclk_config & (0x03 << 8));
	sunxi_smc_writel(value, IO_ADDRESS(SUNXI_CCM_PBASE + 0x50));
	udelay(10);
	sunxi_mem_set_status(CLK_RESUME_START | 1);
	/* set AXI_CLK_DIV_RATIO, bit0~1 */
	value &= ~(0x03 << 0);
	value |= (cpuclk_config & (0x03 << 0));
	sunxi_smc_writel(value, IO_ADDRESS(SUNXI_CCM_PBASE + 0x50));
	mdelay(1);
	sunxi_mem_set_status(CLK_RESUME_START | 2);
	/* set APB1_CLK_SRC_SEL, bit16~13 */
	sunxi_smc_writel(cpuclk_config, IO_ADDRESS(SUNXI_CCM_PBASE + 0x50));
	mdelay(1);
	sunxi_mem_set_status(CLK_RESUME_START | 3);

	/* restore AHB1/APB1 Configuration Register form low to high frequency */
	value = sunxi_smc_readl(IO_ADDRESS(SUNXI_CCM_PBASE + 0x54));
	/* set AHB1_PRE_DIV, bit6~7 */
	value &= ~(0x03 << 6);
	value |= (ahbclk_config & (0x03 << 6));
	sunxi_smc_writel(value, IO_ADDRESS(SUNXI_CCM_PBASE + 0x54));
	udelay(10);
	sunxi_mem_set_status(CLK_RESUME_START | 4);
	/* set AHB1_CLK_DIV_RATIO, bit4~5 */
	value &= ~(0x03 << 4);
	value |= (ahbclk_config & (0x03 << 4));
	sunxi_smc_writel(value, IO_ADDRESS(SUNXI_CCM_PBASE + 0x54));
	udelay(10);
	sunxi_mem_set_status(CLK_RESUME_START | 5);
	/* set APB1_CLK_RATIO, bit8~9 */
	value &= ~(0x03 << 8);
	value |= (ahbclk_config & (0x03 << 8));
	sunxi_smc_writel(value, IO_ADDRESS(SUNXI_CCM_PBASE + 0x54));
	udelay(10);
	sunxi_mem_set_status(CLK_RESUME_START | 7);
	/* set APB1_CLK_SRC_SEL, bit12~13 */
	sunxi_smc_writel(ahbclk_config, IO_ADDRESS(SUNXI_CCM_PBASE + 0x54));
	mdelay(2);
	sunxi_mem_set_status(CLK_RESUME_START | 9);

	/* restore CPU PLL form low to high frequency */
	pll_cpu = clk_get(NULL, PLL_CPU_CLK);
	if (IS_ERR(pll_cpu)) {
		printk(KERN_ERR "get pll_cpu failed!\n");
		return;
	}
	if (clk_set_rate(pll_cpu, pll_cpu_back)) {
		printk(KERN_ERR "set pll_cpu failed!\n");
		return;
	}
	if (clk_prepare_enable(pll_cpu)) {
		printk(KERN_ERR "enable pll_cpu failed!\n");
		return;
	}
	clk_put(pll_cpu);
}

static struct syscore_ops sunxi_pm_syscore_ops = {
	.suspend = sunxi_pm_syscore_suspend,
	.resume = sunxi_pm_syscore_resume,
};

/**
 * sunxi_pm_init() - initial pm sub-system
 */
static int __init sunxi_pm_init(void)
{
#ifdef CONFIG_SCENELOCK
	script_item_u *list = NULL;
	int wakeup_src_cnt = 0;
	unsigned int i = 0;
	unsigned int gpio = 0;

	/* config wakeup sources */
	wakeup_src_cnt = script_get_pio_list("wakeup_src_para",&list);
	pr_info("wakeup src cnt is : %d. \n", wakeup_src_cnt);

	if (wakeup_src_cnt) {
		while (wakeup_src_cnt--) {
			gpio = (list + (i++) )->gpio.gpio;
			extended_standby_enable_wakeup_src(CPUS_GPIO_SRC, gpio);
		}
	}
#endif
	register_syscore_ops(&sunxi_pm_syscore_ops);
	suspend_set_ops(&sunxi_pm_ops);
	pr_info("sunxi pm init\n");

	return 0;
}

/**
 * sunxi_pm_exit() - exit pm sub-system
 */
static void __exit sunxi_pm_exit(void)
{
	unregister_syscore_ops(&sunxi_pm_syscore_ops);
	suspend_set_ops(NULL);
	pr_info("sunxi pm exit\n");
}

core_initcall(sunxi_pm_init);
module_exit(sunxi_pm_exit);

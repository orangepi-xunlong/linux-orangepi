/*
 * SUNXI generic CPU idle driver.
 *
 * Copyright (C) 2014 Allinertech Ltd.
 * Author: pannan <pannan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpuidle.h>
#include <linux/cpumask.h>
#include <linux/cpu_pm.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <asm/cpuidle.h>
#include <asm/suspend.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <linux/clockchips.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/cpu.h>
#include <linux/sunxi-cpuidle.h>
#include <linux/syscore_ops.h>

#ifndef CONFIG_SMP
#define SUNXI_CPUCFG_PBASE		(0x01C25C00)
#define SUNXI_SYSCTL_PBASE		(0x01C00000)
static void __iomem *sunxi_cpucfg_base;
static void __iomem *sunxi_sysctl_base;
#else
extern void __iomem *sunxi_cpucfg_base;
extern void __iomem *sunxi_sysctl_base;
#endif

static void __iomem *gic_distbase;
static int cpu0_entry_flag __cacheline_aligned_in_smp;
static int cpu1_entry_flag __cacheline_aligned_in_smp;
static int cpu2_entry_flag __cacheline_aligned_in_smp;
static int cpu3_entry_flag __cacheline_aligned_in_smp;
void *cpux_flag_entry[NR_CPUS];

static inline bool sunxi_idle_pending_sgi(void)
{
	u32 pending_set;

	pending_set = readl_relaxed(gic_distbase + GIC_DIST_PENDING_SET);
	return pending_set & 0xFFFF ? true : false;
}

void sunxi_idle_cpux_flag_set(unsigned int cpu, int hotplug)
{
	if (cpu == 0)
		cpu0_entry_flag = hotplug;
	else if (cpu == 1)
		cpu1_entry_flag = hotplug;
	else if (cpu == 2)
		cpu2_entry_flag = hotplug;
	else if (cpu == 3)
		cpu3_entry_flag = hotplug;
}

void sunxi_idle_cpux_flag_valid(unsigned int cpu, int value)
{
	if (cpu == 0)
		BUG_ON(cpu0_entry_flag != value);
	else if (cpu == 1)
		BUG_ON(cpu1_entry_flag != value);
	else if (cpu == 2)
		BUG_ON(cpu2_entry_flag != value);
	else if (cpu == 3)
		BUG_ON(cpu3_entry_flag != value);
}

static int sunxi_idle_core_power_down(unsigned long val)
{
	unsigned int value;
	unsigned int target_cpu = raw_smp_processor_id();

	if (sunxi_idle_pending_sgi())
		return 1;

	sunxi_idle_cpux_flag_set(target_cpu, 0);

	/* disable gic cpu interface */
	gic_cpu_if_down();

	/* set core flag */
	writel_relaxed(0x1 << target_cpu,
				sunxi_cpucfg_base + CPUCFG_CORE_FLAG_REG);

	/* disable the data cache */
	asm("mrc p15, 0, %0, c1, c0, 0" : "=r" (value) );
	value &= ~(1<<2);
	asm volatile("mcr p15, 0, %0, c1, c0, 0\n" : : "r" (value));

	/* clean and ivalidate L1 cache */
	flush_cache_all();

	/* execute a CLREX instruction */
	asm("clrex" : : : "memory", "cc");

	/*
	 * step4: switch cpu from SMP mode to AMP mode,
	 * aim is to disable cache coherency
	 */
	asm("mrc p15, 0, %0, c1, c0, 1" : "=r" (value) );
	value &= ~(1<<6);
	asm volatile("mcr p15, 0, %0, c1, c0, 1\n" : : "r" (value));

	/* execute an ISB instruction */
	isb();

	/* execute a DSB instruction */
	dsb();

	/* execute a WFI instruction */
	while (1) {
		asm("wfi" : : : "memory", "cc");
	}

	return 1;
}

static int sunxi_idle_enter_c1(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int idx)
{
	int ret;

#ifndef CONFIG_SMP
	writel_relaxed((void *)(virt_to_phys(cpu_resume)),
				sunxi_sysctl_base + CPU_SOFT_ENTRY_REG0);
#endif

	cpu_pm_enter();
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_ENTER, &dev->cpu);
	smp_wmb();

	ret = cpu_suspend(0, sunxi_idle_core_power_down);

	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_EXIT, &dev->cpu);
	cpu_pm_exit();

	return idx;
}

static struct cpuidle_driver sunxi_idle_driver = {
	.name = "sunxi_idle",
	.owner = THIS_MODULE,
	.states[0] = ARM_CPUIDLE_WFI_STATE,
	.states[1] = {
		.enter                  = sunxi_idle_enter_c1,
		.exit_latency           = 10,
		.target_residency       = 10000,
		.flags                  = CPUIDLE_FLAG_TIME_VALID,
		.name                   = "CPD",
		.desc                   = "CORE POWER DOWN",
	},
	.state_count = 2,
};

static void sunxi_idle_iomap_init(void)
{
	gic_distbase = ioremap(GIC_DIST_PBASE, SZ_4K);
#ifndef CONFIG_SMP
	sunxi_cpucfg_base = ioremap(SUNXI_CPUCFG_PBASE, SZ_1K);
	sunxi_sysctl_base = ioremap(SUNXI_SYSCTL_PBASE, SZ_1K);
#endif
	pr_debug("%s: cpucfg_base=0x%p sysctl_base=0x%p\n", __func__,
				sunxi_cpucfg_base, sunxi_sysctl_base);
}

static void sunxi_idle_hw_init(void)
{
	/* write hotplug flag, cpu0 use */
	writel_relaxed(0xFA50392F, sunxi_sysctl_base + BOOT_CPU_HP_FLAG_REG);

	/* set delay0 to 1us */
	writel_relaxed(0x1, sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_DELAY_REG);

	/* enable cpuidle */
	writel_relaxed(0x16AA0000, sunxi_cpucfg_base + CPUCFG_CPUIDLE_EN_REG);
	writel_relaxed(0xAA160001, sunxi_cpucfg_base + CPUCFG_CPUIDLE_EN_REG);
}

static int cpuidle_notify(struct notifier_block *self, unsigned long action,
				void *hcpu)
{
	switch (action) {
	case CPU_UP_PREPARE:
		cpuidle_pause_and_lock();
		break;
	case CPU_ONLINE:
		cpuidle_resume_and_unlock();
		break;
	case CPU_DOWN_PREPARE:
		cpuidle_pause_and_lock();
		break;
	case CPU_POST_DEAD:
		cpuidle_resume_and_unlock();
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block cpuidle_nb = {
	.notifier_call = cpuidle_notify,
};

void sunxi_idle_cpux_flag_init(void)
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		if (cpu == 0) {
			cpu0_entry_flag = 1;
			cpux_flag_entry[cpu] = (void *)(virt_to_phys(
				&cpu0_entry_flag));
		} else if (cpu == 1) {
			cpu1_entry_flag = 1;
			cpux_flag_entry[cpu] = (void *)(virt_to_phys(
				&cpu1_entry_flag));
		} else if (cpu == 2) {
			cpu2_entry_flag = 1;
			cpux_flag_entry[cpu] = (void *)(virt_to_phys(
				&cpu2_entry_flag));
		} else if (cpu == 3) {
			cpu3_entry_flag = 1;
			cpux_flag_entry[cpu] = (void *)(virt_to_phys(
				&cpu3_entry_flag));
		}
	}
}

static void sunxi_idle_pm_resume(void)
{
	sunxi_idle_hw_init();
}

static struct syscore_ops sunxi_idle_pm_syscore_ops = {
	.resume = sunxi_idle_pm_resume,
};

static int __init sunxi_idle_init(void)
{
	int ret;

	if (!of_machine_is_compatible("arm,sun8iw11p1"))
		return -ENODEV;

	sunxi_idle_iomap_init();

	ret = register_cpu_notifier(&cpuidle_nb);
	if (ret)
		goto err_cpu_notifier;

	register_syscore_ops(&sunxi_idle_pm_syscore_ops);

	ret = cpuidle_register(&sunxi_idle_driver, NULL);
	if (!ret)
		sunxi_idle_hw_init();
	else
		goto err_cpuidle_register;

	return 0;

err_cpuidle_register:
	unregister_syscore_ops(&sunxi_idle_pm_syscore_ops);
	unregister_cpu_notifier(&cpuidle_nb);
err_cpu_notifier:
	return ret;
}
device_initcall(sunxi_idle_init);

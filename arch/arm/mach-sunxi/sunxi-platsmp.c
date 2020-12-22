/*
 * Sunxi platform smp source file.
 * It contains platform specific fucntions needed for the linux smp kernel.
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * Sugar (shuge@allwinnertech.com)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/cpu_pm.h>

#include <asm/smp.h>
#include <asm/io.h>
#include <asm/smp_scu.h>
#include <asm/hardware/gic.h>
#include <mach/cpuidle-sunxi.h>
#include <mach/sunxi-chip.h>

#include <mach/platform.h>
#ifdef CONFIG_HOTPLUG
#include "sunxi-hotplug.h"
#endif

#ifdef CONFIG_SMP

#define get_nr_cores()					\
	({						\
		unsigned int __val;			\
		asm("mrc	p15, 1, %0, c9, c0, 2"	\
		    : "=r" (__val)			\
		    :					\
		    : "cc");				\
		((__val>>24) & 0x03) + 1;		\
	})

extern void sunxi_secondary_startup(void);
static DEFINE_SPINLOCK(boot_lock);
#ifdef CONFIG_ARCH_SUN8I
#include <mach/sun8i/platsmp.h>
#elif defined CONFIG_ARCH_SUN9I
#include <mach/sun9i/platsmp.h>
#endif

/* boot entry for each cpu */
extern void *cpus_boot_entry[NR_CPUS];
extern void secondary_startup(void);

#ifdef CONFIG_SUNXI_TRUSTZONE
static void sunxi_secure_set_secondary_entry(void *entry)
{
	if (sunxi_soc_is_secure()) {
		call_firmware_op(set_secondary_entry, entry);
	} else {
		sunxi_set_secondary_entry(entry);
	}
}
#endif

void sunxi_set_cpus_boot_entry(int cpu, void *entry)
{
	if(cpu < NR_CPUS) {
		cpus_boot_entry[cpu] = (void *)(virt_to_phys(entry));
		smp_wmb();
		__cpuc_flush_dcache_area(cpus_boot_entry, NR_CPUS*4);
		outer_clean_range(__pa(&cpus_boot_entry), __pa(&cpus_boot_entry + 1));
	}
}

/*
 * Setup the set of possible CPUs (via set_cpu_possible)
 */
void sunxi_smp_init_cpus(void)
{
	unsigned int i, ncores;

	ncores = get_nr_cores();
	pr_debug("[%s] ncores=%d\n", __func__, ncores);

	#ifdef CONFIG_ARCH_SUN8IW7
	{
		unsigned int chip_ver = sunxi_get_soc_ver();
		/* parse hardware parameter */
		switch(chip_ver) {
			case SUN8IW7P1_REV_A:
			case SUN8IW7P1_REV_B:
				nr_cpu_ids = 4;
				break;
			case SUN8IW7P2_REV_A:
			case SUN8IW7P2_REV_B:
			default:
				nr_cpu_ids = 2;
				break;
		}
		pr_debug("chip version is:0x%x, number of possible cores is:%d\n", chip_ver, nr_cpu_ids);
	}
	#endif

	/*
	 * sanity check, the cr_cpu_ids is configured form CONFIG_NR_CPUS
	 */
	if (ncores > nr_cpu_ids) {
	        pr_warn("SMP: %u cores greater than maximum (%u), clipping\n",
				ncores, nr_cpu_ids);
	        ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; i++) {
	    set_cpu_possible(i, true);
	}

#if defined(CONFIG_ARM_SUNXI_CPUIDLE)
	set_smp_cross_call(sunxi_raise_softirq);
#else
	set_smp_cross_call(gic_raise_softirq);
#endif
}

static void sunxi_smp_prepare_cpus(unsigned int max_cpus)
{
	pr_info("[%s] enter\n", __func__);
#ifdef CONFIG_SUNXI_TRUSTZONE
	sunxi_secure_set_secondary_entry((void *)(virt_to_phys(sunxi_secondary_startup)));
#else
	sunxi_set_secondary_entry((void *)(virt_to_phys(sunxi_secondary_startup)));
#endif
}

/*
 * Perform platform specific initialisation of the specified CPU.
 */
void sunxi_smp_secondary_init(unsigned int cpu)
{
	/*
	 * if any interrupts are already enabled for the primary
	 * core (e.g. timer irq), then they will not have been enabled
	 * for us: do so
	 */
	gic_secondary_init(0);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

/*
 * Boot a secondary CPU, and assign it the specified idle task.
 * This also gives us the initial stack to use for this CPU.
 */
int  sunxi_smp_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	pr_debug("[%s] enter\n", __func__);
	spin_lock(&boot_lock);
	sunxi_set_cpus_boot_entry(cpu, secondary_startup);
	enable_cpu(cpu);

	/*
	 * Now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);
	return 0;
}

#endif
struct smp_operations sunxi_smp_ops __initdata = {
	.smp_init_cpus		= sunxi_smp_init_cpus,
	.smp_prepare_cpus	= sunxi_smp_prepare_cpus,
	.smp_secondary_init	= sunxi_smp_secondary_init,
	.smp_boot_secondary	= sunxi_smp_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= sunxi_cpu_die,
	.cpu_kill		= sunxi_cpu_kill,
	.cpu_disable		= sunxi_cpu_disable,
#endif
};

static void *sunxi_secondary_entry_save;

static void sunxi_cpu_save(void)
{
	//pr_info("[%s] enter\n", __func__);

	/* save the secondary cpu boot entry address */
	sunxi_secondary_entry_save = sunxi_get_secondary_entry();
}

static void sunxi_cpu_restore(void)
{
	//pr_info("[%s] enter\n", __func__);

	/* restore the secondary cpu boot entry address */
	sunxi_set_secondary_entry(sunxi_secondary_entry_save);
}

static int sunxi_cpususpend_notifier(struct notifier_block *self, unsigned long cmd, void *v)
{
	switch (cmd) {
	case CPU_PM_ENTER:
		sunxi_cpu_save();
		break;
	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
		sunxi_cpu_restore();
		break;
	case CPU_CLUSTER_PM_ENTER:
	case CPU_CLUSTER_PM_ENTER_FAILED:
	case CPU_CLUSTER_PM_EXIT:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block sunxi_cpususpend_notifier_block = {
	.notifier_call = sunxi_cpususpend_notifier,
};

static int __init register_cpususpend_notifier(void)
{
	return cpu_pm_register_notifier(&sunxi_cpususpend_notifier_block);
}
core_initcall(register_cpususpend_notifier);


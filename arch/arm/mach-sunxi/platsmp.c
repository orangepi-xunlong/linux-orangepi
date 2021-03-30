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

#include <linux/delay.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <linux/sunxi-sid.h>

#include "platsmp.h"

extern void sunxi_secondary_startup(void);
extern void secondary_startup(void);

static DEFINE_SPINLOCK(boot_lock);
void __iomem *sunxi_cpucfg_base;
void __iomem *sunxi_rtc_base;
void __iomem *sunxi_sysctl_base;
void *cpus_boot_entry[NR_CPUS];

static void sunxi_set_cpus_boot_entry(int cpu, void *entry)
{
	if (cpu < NR_CPUS) {
		cpus_boot_entry[cpu] = (void *)(virt_to_phys(entry));
		smp_wmb();
		__cpuc_flush_dcache_area(cpus_boot_entry, sizeof(cpus_boot_entry));
		outer_clean_range(__pa(&cpus_boot_entry), __pa(&cpus_boot_entry + 1));
	}
}

static void sunxi_smp_iomap_init(void)
{
	sunxi_cpucfg_base = ioremap(SUNXI_CPUCFG_PBASE, SZ_1K);
#if defined(CONFIG_ARCH_SUN8IW10)
	sunxi_rtc_base = ioremap(SUNXI_RTC_PBASE, SZ_1K);
	pr_debug("cpucfg_base=0x%p rtc_base=0x%p\n", sunxi_cpucfg_base, sunxi_rtc_base);
#else
	sunxi_sysctl_base = ioremap(SUNXI_SYSCTL_PBASE, SZ_1K);
	pr_debug("cpucfg_base=0x%p sysctl_base=0x%p\n", sunxi_cpucfg_base, sunxi_sysctl_base);
#endif
}

static void sunxi_smp_init_cpus(void)
{
	unsigned int i, ncores = get_nr_cores();

#if defined(CONFIG_ARCH_SUN8IW11)
	unsigned int chip_ver = sunxi_get_soc_ver();

	switch (chip_ver) {
	case SUN8IW11P2_REV_A:
	case SUN8IW11P3_REV_A:
	case SUN8IW11P4_REV_A:
		ncores = 4;
		break;
	case SUN8IW11P1_REV_A:
	default:
		ncores = 2;
		break;
	}
#endif

	pr_debug("[%s] ncores=%d\n", __func__, ncores);

	 /* Limit possible CPUs to defconfig */
	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %u CPUs physically present. Only %d configured.\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

#ifdef CONFIG_CPU_IDLE_SUNXI
	sunxi_idle_cpux_flag_init();
#endif

	sunxi_smp_iomap_init();
	pr_debug("[%s] done\n", __func__);
}

/*
 * Boot a secondary CPU, and assign it the specified idle task.
 * This also gives us the initial stack to use for this CPU.
 */
int sunxi_smp_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	spin_lock(&boot_lock);
#ifdef CONFIG_CPU_IDLE_SUNXI
	sunxi_set_secondary_entry((void *)
				(virt_to_phys(sunxi_cpux_entry_judge)));
#else
	sunxi_set_secondary_entry((void *)
				(virt_to_phys(sunxi_secondary_startup)));
#endif
	sunxi_set_cpus_boot_entry(cpu, secondary_startup);
	sunxi_enable_cpu(cpu);

	/*
	 * Now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);
	pr_debug("[%s] done\n", __func__);

	return 0;
}

struct smp_operations sunxi_smp_ops __initdata = {
	.smp_init_cpus		= sunxi_smp_init_cpus,
	.smp_boot_secondary	= sunxi_smp_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die			= sunxi_cpu_die,
	.cpu_kill			= sunxi_cpu_kill,
	.cpu_disable		= sunxi_cpu_disable,
#endif
};

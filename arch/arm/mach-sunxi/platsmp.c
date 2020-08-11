/*
 * SMP support for Allwinner SoCs
 *
 * Copyright (C) 2013 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * Based on code
 *  Copyright (C) 2012-2013 Allwinner Ltd.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/smp.h>
#include <linux/sunxi-sid.h>

#include <asm/cacheflush.h>

#include "platsmp.h"

void *cpus_boot_entry[NR_CPUS];

static void __iomem *cpucfg_membase;
static void __iomem *prcm_membase;

static DEFINE_SPINLOCK(cpu_lock);

static void __init sun6i_smp_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "allwinner,sun6i-a31-prcm");
	if (!node) {
		pr_err("Missing A31 PRCM node in the device tree\n");
		return;
	}

	prcm_membase = of_iomap(node, 0);
	if (!prcm_membase) {
		pr_err("Couldn't map A31 PRCM registers\n");
		return;
	}

	node = of_find_compatible_node(NULL, NULL,
				       "allwinner,sun6i-a31-cpuconfig");
	if (!node) {
		pr_err("Missing A31 CPU config node in the device tree\n");
		return;
	}

	cpucfg_membase = of_iomap(node, 0);
	if (!cpucfg_membase)
		pr_err("Couldn't map A31 CPU config registers\n");

}

static int sun6i_smp_boot_secondary(unsigned int cpu,
				    struct task_struct *idle)
{
	u32 reg;
	int i;

	if (!(prcm_membase && cpucfg_membase))
		return -EFAULT;

	spin_lock(&cpu_lock);

	/* Set CPU boot address */
	writel(virt_to_phys(secondary_startup),
	       cpucfg_membase + CPUCFG_PRIVATE0_REG);

	/* Assert the CPU core in reset */
	writel(0, cpucfg_membase + CPUCFG_CPU_RST_CTRL_REG(cpu));

	/* Assert the L1 cache in reset */
	reg = readl(cpucfg_membase + CPUCFG_GEN_CTRL_REG);
	writel(reg & ~BIT(cpu), cpucfg_membase + CPUCFG_GEN_CTRL_REG);

	/* Disable external debug access */
	reg = readl(cpucfg_membase + CPUCFG_DBG_CTL1_REG);
	writel(reg & ~BIT(cpu), cpucfg_membase + CPUCFG_DBG_CTL1_REG);

	/* Power up the CPU */
	for (i = 0; i <= 8; i++)
		writel(0xff >> i, prcm_membase + PRCM_CPU_PWR_CLAMP_REG(cpu));
	mdelay(10);

	/* Clear CPU power-off gating */
	reg = readl(prcm_membase + PRCM_CPU_PWROFF_REG);
	writel(reg & ~BIT(cpu), prcm_membase + PRCM_CPU_PWROFF_REG);
	mdelay(1);

	/* Deassert the CPU core reset */
	writel(3, cpucfg_membase + CPUCFG_CPU_RST_CTRL_REG(cpu));

	/* Enable back the external debug accesses */
	reg = readl(cpucfg_membase + CPUCFG_DBG_CTL1_REG);
	writel(reg | BIT(cpu), cpucfg_membase + CPUCFG_DBG_CTL1_REG);

	spin_unlock(&cpu_lock);

	return 0;
}

static struct smp_operations sun6i_smp_ops __initdata = {
	.smp_prepare_cpus	= sun6i_smp_prepare_cpus,
	.smp_boot_secondary	= sun6i_smp_boot_secondary,
};
CPU_METHOD_OF_DECLARE(sun6i_a31_smp, "allwinner,sun6i-a31", &sun6i_smp_ops);

static void __init sun8i_smp_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "allwinner,sun8i-a23-prcm");
	if (!node) {
		pr_err("Missing A23 PRCM node in the device tree\n");
		return;
	}

	prcm_membase = of_iomap(node, 0);
	if (!prcm_membase) {
		pr_err("Couldn't map A23 PRCM registers\n");
		return;
	}

	node = of_find_compatible_node(NULL, NULL,
				       "allwinner,sun8i-a23-cpuconfig");
	if (!node) {
		pr_err("Missing A23 CPU config node in the device tree\n");
		return;
	}

	cpucfg_membase = of_iomap(node, 0);
	if (!cpucfg_membase)
		pr_err("Couldn't map A23 CPU config registers\n");

}

static int sun8i_smp_boot_secondary(unsigned int cpu,
				    struct task_struct *idle)
{
	u32 reg;

	if (!(prcm_membase && cpucfg_membase))
		return -EFAULT;

	spin_lock(&cpu_lock);

	/* Set CPU boot address */
	writel(virt_to_phys(secondary_startup),
	       cpucfg_membase + CPUCFG_PRIVATE0_REG);

	/* Assert the CPU core in reset */
	writel(0, cpucfg_membase + CPUCFG_CPU_RST_CTRL_REG(cpu));

	/* Assert the L1 cache in reset */
	reg = readl(cpucfg_membase + CPUCFG_GEN_CTRL_REG);
	writel(reg & ~BIT(cpu), cpucfg_membase + CPUCFG_GEN_CTRL_REG);

	/* Clear CPU power-off gating */
	reg = readl(prcm_membase + PRCM_CPU_PWROFF_REG);
	writel(reg & ~BIT(cpu), prcm_membase + PRCM_CPU_PWROFF_REG);
	mdelay(1);

	/* Deassert the CPU core reset */
	writel(3, cpucfg_membase + CPUCFG_CPU_RST_CTRL_REG(cpu));

	spin_unlock(&cpu_lock);

	return 0;
}

struct smp_operations sun8i_smp_ops __initdata = {
	.smp_prepare_cpus	= sun8i_smp_prepare_cpus,
	.smp_boot_secondary	= sun8i_smp_boot_secondary,
};
CPU_METHOD_OF_DECLARE(sun8i_a23_smp, "allwinner,sun8i-a23", &sun8i_smp_ops);

static void sunxi_set_cpus_boot_entry(int cpu, void *entry)
{
	if (cpu < num_possible_cpus()) {
		cpus_boot_entry[cpu] = (void *)(virt_to_phys(entry));
		/* barrier */
		smp_wmb();
		__cpuc_flush_dcache_area(cpus_boot_entry,
						sizeof(cpus_boot_entry));
		outer_clean_range(__pa(&cpus_boot_entry),
					__pa(&cpus_boot_entry + 1));
	}
}

static void __init sunxi_smp_init_cpus(void)
{
	unsigned int i, ncores = get_nr_cores();

#if defined(CONFIG_ARCH_SUN8IW11) || defined(CONFIG_ARCH_SUN8IW7)
	unsigned int chip_ver = sunxi_get_soc_ver();

	switch (chip_ver) {
	case SUN8IW11P2_REV_A:
	case SUN8IW11P3_REV_A:
	case SUN8IW11P4_REV_A:
	case SUN8IW7P1_REV_A:
	case SUN8IW7P1_REV_B:
		ncores = 4;
		break;
	case SUN8IW11P1_REV_A:
	case SUN8IW7P2_REV_A:
	case SUN8IW7P2_REV_B:
	default:
		ncores = 2;
		break;
	}
#endif
	pr_debug("[%s] ncores = %u\n", __func__, ncores);
	if (ncores > nr_cpu_ids) {
		pr_warn(KERN_ERR "SMP: %u CPUs physically present. Only %d configured.\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

#ifdef CONFIG_CPU_IDLE_SUNXI
	sunxi_idle_cpux_flag_init();
#endif
	pr_debug("[%s] done\n", __func__);
}

/*
 * Boot a secondary CPU, and assign it the specified idle task.
 * This also gives us the initial stack to use for this CPU.
 */
static int sunxi_smp_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	if (!(sunxi_cpucfg_base && sunxi_soft_entry_base))
		return -EFAULT;

	spin_lock(&cpu_lock);

#ifdef CONFIG_CPU_IDLE_SUNXI
	writel(virt_to_phys(sunxi_cpux_entry_judge),
		sunxi_soft_entry_base + CPU_SOFT_ENTRY_REG(cpu));
#else
	writel(virt_to_phys(sunxi_secondary_startup),
		sunxi_soft_entry_base + CPU_SOFT_ENTRY_REG(cpu));
#endif

	sunxi_set_cpus_boot_entry(cpu, secondary_startup);

	sunxi_enable_cpu(cpu);

	/*
	 * Now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&cpu_lock);
	pr_debug("[%s] done\n", __func__);

	return 0;
}

struct smp_operations sunxi_smp_ops = {
	.smp_init_cpus		= sunxi_smp_init_cpus,
	.smp_boot_secondary	= sunxi_smp_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= sunxi_cpu_die,
	.cpu_kill		= sunxi_cpu_kill,
	.cpu_disable		= sunxi_cpu_disable,
#endif
};


CPU_METHOD_OF_DECLARE(sun8iw11p1_smp, "allwinner,sun8iw11p1", &sunxi_smp_ops);
CPU_METHOD_OF_DECLARE(sun8iw15p1_smp, "allwinner,sun8iw15p1", &sunxi_smp_ops);
CPU_METHOD_OF_DECLARE(sun8iw12p1_smp, "allwinner,sun8iw12p1", &sunxi_smp_ops);
CPU_METHOD_OF_DECLARE(sun8iw17p1_smp, "allwinner,sun8iw17p1", &sunxi_smp_ops);
CPU_METHOD_OF_DECLARE(sun8iw7p1_smp, "allwinner,sun8iw7p1", &sunxi_smp_ops);

/*
 * linux/arch/arm/mach-sunxi/sun8i-mcpm.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner sun8i platform multi-cluster power management operations.
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

#include <asm/mcpm.h>
#include <asm/proc-fns.h>
#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/cp15.h>
#include <asm/smp_plat.h>
#include <asm/hardware/gic.h>

#include <mach/platform.h>
#include <mach/cci.h>
#include <mach/sun8i/platsmp.h>
#include <linux/arisc/arisc.h>
#include <linux/arisc/arisc-notifier.h>
#include <mach/sys_config.h>
#include <mach/sunxi-chip.h>
#include <mach/cpuidle-sunxi.h>

#ifdef CONFIG_EVB_PLATFORM
#define MCPM_WITH_ARISC_DVFS_SUPPORT	1
#endif

#ifdef MCPM_WITH_ARISC_DVFS_SUPPORT
/* sync with arisc module */
static bool arisc_ready = 0;
#endif

#define is_arisc_ready()   (arisc_ready)
#define set_arisc_ready(x) (arisc_ready = (x))

#if defined(CONFIG_ARCH_SUN8IW6)
static unsigned int soc_version;
#define CORES_PER_CLUSTER	(4)
#elif defined(CONFIG_ARCH_SUN8IW9)
#define CORES_PER_CLUSTER	(3)
#endif

/* sun8i platform support two clusters,
 * cluster0 : cortex-a7,
 * cluster1 : cortex-a7.
 */

#define CLUSTER_0	    0
#define CLUSTER_1	    1
//#define MAX_CLUSTERS	2


#define SUN8I_CPU_IS_WFI_MODE(cluster, cpu) (readl(sun8i_cpuxcfg_base + SUNXI_CLUSTER_CPU_STATUS(cluster)) & (1 << (16 + cpu)))
#define SUN8I_L2CACHE_IS_WFI_MODE(cluster)  (readl(sun8i_cpuxcfg_base + SUNXI_CLUSTER_CPU_STATUS(cluster)) & (1 << 0))

#define SUN8I_C0_CLSUTER_PWRUP_FREQ   (600000)  /* freq base on khz */
#define SUN8I_C1_CLSUTER_PWRUP_FREQ   (600000)  /* freq base on khz */

static unsigned int cluster_pll[MAX_CLUSTERS];
static unsigned int cluster_powerup_freq[MAX_CLUSTERS];

/*
 * We can't use regular spinlocks. In the switcher case, it is possible
 * for an outbound CPU to call power_down() after its inbound counterpart
 * is already live using the same logical CPU number which trips lockdep
 * debugging.
 */
static arch_spinlock_t sun8i_mcpm_lock = __ARCH_SPIN_LOCK_UNLOCKED;

/* sunxi cluster and cpu power-management models */
static void __iomem *sun8i_prcm_base;
static void __iomem *sun8i_cpuxcfg_base;
static void __iomem *sun8i_cpuscfg_base;

/* sunxi cluster and cpu use status,
 * this is use to detect the first-man and last-man.
 */
static int sun8i_cpu_use_count[MAX_NR_CLUSTERS][MAX_CPUS_PER_CLUSTER];
static int sun8i_cluster_use_count[MAX_NR_CLUSTERS];

/* use to sync cpu kill and die sync,
 * the cpu which will to die just process cpu internal operations,
 * such as invalid L1 cache and shutdown SMP bit,
 * the cpu power-down relative operations should be executed by killer cpu.
 * so we must ensure the kill operation should after cpu die operations.
 */
static cpumask_t dead_cpus[MAX_NR_CLUSTERS];

/* add by huangshr
 * to check cpu really power state*/
cpumask_t cpu_power_up_state_mask;
/* end by huangshr*/

/* hrtimer, use to power-down cluster process,
 * power-down cluster use the time-delay solution mainly for keep long time
 * L2-cache coherecy, this can decrease IKS switch and cpu power-on latency.
 */
static struct hrtimer cluster_power_down_timer;

static int sun8i_cpu_power_switch_set(unsigned int cluster, unsigned int cpu, bool enable)
{
	if (enable) {
		if (0x00 == readl(sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			pr_debug("%s: power switch enable already\n", __func__);
			return 0;
		}
		/* de-active cpu power clamp */
		writel(0xFE, sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(20);

		writel(0xF8, sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);

		writel(0xE0, sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);
#ifdef CONFIG_ARCH_SUN8IW6
		writel(0xc0, sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);
#endif
		writel(0x80, sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);

		writel(0x00, sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(20);
		while(0x00 != readl(sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			;
		}
	} else {
		if (0xFF == readl(sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			pr_debug("%s: power switch disable already\n", __func__);
			return 0;
		}
		writel(0xFF, sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(30);
		while(0xFF != readl(sun8i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			;
		}
	}
	return 0;
}


int sun8i_cpu_power_set(unsigned int cluster, unsigned int cpu, bool enable)
{
	unsigned int value;
	if (enable) {
		/*
		 * power-up cpu core process
		 */
		pr_debug("sun8i power-up cluster-%d cpu-%d\n", cluster, cpu);

		cpumask_set_cpu(((cluster)*4 + cpu), &cpu_power_up_state_mask);

		/* assert cpu core reset */
		value  = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value &= (~(1<<cpu));
		writel(value, sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		/* assert cpu power-on reset */
		value  = readl(sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		value &= (~(1<<cpu));
		writel(value, sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		udelay(10);

		/* L1RSTDISABLE hold low */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL0(cluster));
		value &= ~(1<<cpu);
		writel(value, sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL0(cluster));

		/* release power switch */
		sun8i_cpu_power_switch_set(cluster, cpu, 1);

#ifdef CONFIG_ARCH_SUN8IW6
		if (cpu == 0) {
			if (soc_version == SUN8IW6P1_REV_A) {
				/* de-assert cpu power-on reset */
				value = readl(sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
				value |= ((1<<cpu));
				writel(value, sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
				udelay(10);

				/* assert cpu core reset */
				value  = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
				value |= (1<<cpu);
				writel(value, sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
				udelay(10);
				return 0;
			} else {
				/* bit4: C1_cpu0 */
				cpu = 4;
			}
		}
#endif

		/* clear power-off gating */
		value = readl(sun8i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		value &= (~(0x1<<cpu));
		writel(value, sun8i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		udelay(20);

#ifdef CONFIG_ARCH_SUN8IW6
		if (cpu == 4)
			cpu = 0;
#endif

		/* de-assert cpu power-on reset */
		value  = readl(sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		value |= ((1<<cpu));
		writel(value, sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		udelay(10);

		/* de-assert core reset */
		value  = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value |= (1<<cpu);
		writel(value, sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		pr_debug("sun8i power-up cluster-%d cpu-%d already\n", cluster, cpu);
	} else {
		/*
		 * power-down cpu core process
		 */
		pr_debug("sun8i power-down cluster-%d cpu-%d\n", cluster, cpu);
#ifdef CONFIG_ARCH_SUN8IW6
		if (cpu == 0) {
			if (soc_version == SUN8IW6P1_REV_A) {
				/* assert cpu core reset */
				value = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
				value &= (~(1<<cpu));
				writel(value, sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
				udelay(10);

				cpumask_clear_cpu(((cluster)*4 + cpu), &cpu_power_up_state_mask);
				return 0;
			} else {
				/* bit4: C1_cpu0 */
				cpu = 4;
			}
		}
#endif
		/* enable cpu power-off gating */
		value = readl(sun8i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		value |= (1 << cpu);
		writel(value, sun8i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		udelay(20);

#ifdef CONFIG_ARCH_SUN8IW6
		if (cpu == 4)
			cpu = 0;
#endif

		/* active the power output switch */
		sun8i_cpu_power_switch_set(cluster, cpu, 0);

		cpumask_clear_cpu(((cluster)*4 + cpu), &cpu_power_up_state_mask);
		pr_debug("sun8i power-down cpu%d ok.\n", cpu);
	}
	return 0;
}

static int sun8i_cluster_power_set(unsigned int cluster, bool enable)
{
	unsigned int value;
	int          i;
#ifdef MCPM_WITH_ARISC_DVFS_SUPPORT
	/* cluster operation must wait arisc ready */
	if (!is_arisc_ready()) {
		pr_debug("%s: arisc not ready, can't power-up cluster\n", __func__);
		return -EINVAL;
	}
#endif
	if (enable) {
		pr_debug("sun8i power-up cluster-%d\n", cluster);

		/* assert cluster cores resets */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
#if defined(CONFIG_ARCH_SUN8IW6)
		value &= (~(0xF<<0));   /* Core Reset    */
#elif defined(CONFIG_ARCH_SUN8IW9)
		value &= (~(0x7<<0));   /* Core Reset    */
#endif
		writel(value, sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		/* assert cluster cores power-on reset */
		value = readl(sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
#if defined(CONFIG_ARCH_SUN8IW6)
		value &= (~(0xF));
#elif defined(CONFIG_ARCH_SUN8IW9)
		value &= (~(0x7));
#endif
		writel(value, sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		udelay(10);

		/* assert cluster resets */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value &= (~(0x1<<24));  /* SOC DBG Reset */
#if defined(CONFIG_ARCH_SUN8IW6)
		value &= (~(0xF<<20));  /* ETM Reset     */
		value &= (~(0xF<<16));  /* Debug Reset   */
#elif defined(CONFIG_ARCH_SUN8IW9)
		value &= (~(0x7<<20));  /* ETM Reset     */
		value &= (~(0x7<<16));  /* Debug Reset   */
#endif
		value &= (~(0x1<<12));  /* HReset        */
		value &= (~(0x1<<8));   /* L2 Cache Reset*/
		writel(value, sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		/* Set L2RSTDISABLE LOW */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL0(cluster));
		value &= (~(0x1<<4));
		writel(value, sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL0(cluster));

#ifdef MCPM_WITH_ARISC_DVFS_SUPPORT
		/* notify arisc to power-up cluster */
		arisc_dvfs_set_cpufreq(cluster_powerup_freq[cluster], cluster_pll[cluster],
		                       ARISC_MESSAGE_ATTR_SOFTSYN, NULL, NULL);
		mdelay(1);
#endif

#ifdef CONFIG_ARCH_SUN8IW6
		if (soc_version == SUN8IW6P1_REV_A)
			sun8i_cpu_power_switch_set(cluster, 0, 1);
#endif
		/* active ACINACTM */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL1(cluster));
		value |= (1<<0);
		writel(value, sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL1(cluster));

		/* clear cluster power-off gating */
		value = readl(sun8i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
#ifdef CONFIG_ARCH_SUN8IW6
		if (soc_version == SUN8IW6P1_REV_A)
			value &= (~(0x1<<4));
		value &= (~(0x1<<0));
#else
		value &= (~(0x1<<4));
#endif
		writel(value, sun8i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		udelay(20);

		/* de-active ACINACTM */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL1(cluster));
		value &= (~(1<<0));
		writel(value, sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL1(cluster));

		/* de-assert cores reset */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value |= (0x1<<24);  /* SOC DBG Reset */
#if defined(CONFIG_ARCH_SUN8IW6)
		value |= (0xF<<20);  /* ETM Reset     */
		value |= (0xF<<16);  /* Debug Reset   */
#elif defined(CONFIG_ARCH_SUN8IW9)
		value |= (0x7<<20);  /* ETM Reset     */
		value |= (0x7<<16);  /* Debug Reset   */
#endif
		value |= (0x1<<12);  /* HReset        */
		value |= (0x1<<8);   /* L2 Cache Reset*/
		writel(value, sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(20);
		pr_debug("sun8i power-up cluster-%d ok\n", cluster);
	} else {

		pr_debug("sun8i power-down cluster-%d\n", cluster);

		/* active ACINACTM */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL1(cluster));
		value |= (1<<0);
		writel(value, sun8i_cpuxcfg_base + SUNXI_CLUSTER_CTRL1(cluster));

		while (1) {
			if (SUN8I_L2CACHE_IS_WFI_MODE(cluster)) {
				break;
			}
			/* maybe should support timeout to avoid deadloop */
		}

		/* assert cluster cores resets */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
#if defined(CONFIG_ARCH_SUN8IW6)
		value &= (~(0xF<<0));   /* Core Reset    */
#elif defined(CONFIG_ARCH_SUN8IW9)
		value &= (~(0x7<<0));   /* Core Reset    */
#endif
		writel(value, sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		/* assert cluster cores power-on reset */
		value = readl(sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
#if defined(CONFIG_ARCH_SUN8IW6)
		value &= (~(0xF));
#elif defined(CONFIG_ARCH_SUN8IW9)
		value &= (~(0x7));
#endif
		writel(value, sun8i_cpuscfg_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		udelay(10);

		/* assert cluster resets */
		value = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value &= (~(0x1<<24));  /* SOC DBG Reset */
#if defined(CONFIG_ARCH_SUN8IW6)
		value &= (~(0xF<<20));  /* ETM Reset     */
		value &= (~(0xF<<16));  /* Debug Reset   */
#elif defined(CONFIG_ARCH_SUN8IW9)
		value &= (~(0x7<<20));  /* ETM Reset     */
		value &= (~(0x7<<16));  /* Debug Reset   */
#endif
		value &= (~(0x1<<12));  /* HReset        */
		value &= (~(0x1<<8));   /* L2 Cache Reset*/
		writel(value, sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		/* enable cluster and cores power-off gating */
		value = readl(sun8i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		value |= (1<<4);
#if defined(CONFIG_ARCH_SUN8IW6)
		value |= (0xF<<0);
#elif defined(CONFIG_ARCH_SUN8IW9)
		value |= (0x7<<0);
#endif
		writel(value, sun8i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		udelay(20);

		/* disable cluster cores power switch */
		for (i = 0; i < CORES_PER_CLUSTER; i++) {
			sun8i_cpu_power_switch_set(cluster, i, 0);
		}

#ifdef MCPM_WITH_ARISC_DVFS_SUPPORT
		/* notify arisc to power-down cluster,
		 * arisc will disable cluster clock and power-off cpu power domain.
		 */
		arisc_dvfs_set_cpufreq(0, cluster_pll[cluster],
		                       ARISC_MESSAGE_ATTR_SOFTSYN, NULL, NULL);
#endif
		pr_debug("sun8i power-down cluster-%d ok\n", cluster);
	}

	return 0;
}

static int sun8i_cluster_power_status(unsigned int cluster)
{
	int          status = 0;
	unsigned int value;
#ifdef CONFIG_ARCH_SUN8IW6
	unsigned int value_rst;
#endif

	value = readl(sun8i_cpuxcfg_base + SUNXI_CLUSTER_CPU_STATUS(cluster));

	/* cluster WFI status :
	 * all cpu cores enter WFI mode + L2Cache enter WFI status
	 */
#ifdef CONFIG_ARCH_SUN8IW6
	value_rst = readl(sun8i_cpuxcfg_base + SUNXI_CPU_RST_CTRL(cluster));
	if( (!(value_rst & 0x1)) &&  (((value >> 16) & 0xe) == 0xe))
	{
		status = 1;
	}
#elif defined(CONFIG_ARCH_SUN8IW9)
	if (((value >> 16) & 0x7) == 0x7) {
		status = 1;
	}
#endif
	return status;
}

enum hrtimer_restart sun8i_cluster_power_down(struct hrtimer *timer)
{
	ktime_t              period;
	int                  cluster;
	enum hrtimer_restart ret;

	arch_spin_lock(&sun8i_mcpm_lock);
	if (sun8i_cluster_use_count[0] == 0) {
		cluster = 0;
	} else if (sun8i_cluster_use_count[1] == 0) {
		cluster = 1;
	} else {
		ret = HRTIMER_NORESTART;
		goto end;
	}

	if (sun8i_cluster_power_status(cluster)) {
		period = ktime_set(0, 10000000);
		hrtimer_forward_now(timer, period);
		ret = HRTIMER_RESTART;
		goto end;
	} else {
		sun8i_cluster_power_set(cluster, 0);
		ret = HRTIMER_NORESTART;
	}
end:
	arch_spin_unlock(&sun8i_mcpm_lock);
	return ret;
}

static int sun8i_mcpm_power_up(unsigned int cpu, unsigned int cluster)
{
	pr_debug("%s: cluster %u cpu %u\n", __func__, cluster, cpu);
	if (cpu >= MAX_CPUS_PER_CLUSTER || cluster >= MAX_NR_CLUSTERS) {
		return -EINVAL;
	}
	/*
	 * Since this is called with IRQs enabled, and no arch_spin_lock_irq
	 * variant exists, we need to disable IRQs manually here.
	 */
	local_irq_disable();
	arch_spin_lock(&sun8i_mcpm_lock);

	sun8i_cpu_use_count[cluster][cpu]++;
	if (sun8i_cpu_use_count[cluster][cpu] == 1) {
		sun8i_cluster_use_count[cluster]++;
		if (sun8i_cluster_use_count[cluster] == 1) {
			/* first-man should power-up cluster resource */
			pr_debug("%s: power-on cluster-%d", __func__, cluster);
			if (sun8i_cluster_power_set(cluster, 1)) {
				pr_debug("%s: power-on cluster-%d fail\n", __func__, cluster);
				sun8i_cluster_use_count[cluster]--;
				sun8i_cpu_use_count[cluster][cpu]--;
				arch_spin_unlock(&sun8i_mcpm_lock);
				local_irq_enable();
				return -EINVAL;
			}
		}
		/* power-up cpu core */
		sun8i_cpu_power_set(cluster, cpu, 1);
	} else if (sun8i_cpu_use_count[cluster][cpu] != 2) {
		/*
		 * The only possible values are:
		 * 0 = CPU down
		 * 1 = CPU (still) up
		 * 2 = CPU requested to be up before it had a chance
		 *     to actually make itself down.
		 * Any other value is a bug.
		 */
		BUG();
	} else {
		pr_debug("sun8i power-on cluster-%d cpu-%d been skiped, usage count %d\n",
		         cluster, cpu, sun8i_cpu_use_count[cluster][cpu]);
	}
	arch_spin_unlock(&sun8i_mcpm_lock);
	local_irq_enable();

	return 0;
}

static void sun8i_mcpm_power_down(void)
{
	unsigned int mpidr, cpu, cluster;
	bool last_man = false, skip_power_down = false;
	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	/* gic cpu interface exit */
	gic_cpu_exit(0);

	BUG_ON(cpu >= MAX_CPUS_PER_CLUSTER || cluster >= MAX_NR_CLUSTERS);

	__mcpm_cpu_going_down(cpu, cluster);

	arch_spin_lock(&sun8i_mcpm_lock);
	BUG_ON(__mcpm_cluster_state(cluster) != CLUSTER_UP);

	sun8i_cpu_use_count[cluster][cpu]--;
	if (sun8i_cpu_use_count[cluster][cpu] == 0) {
		/* check is the last-man */
		sun8i_cluster_use_count[cluster]--;
		if (sun8i_cluster_use_count[cluster] == 0) {
			last_man = true;
		}
	} else if (sun8i_cpu_use_count[cluster][cpu] == 1) {
		/*
		 * A power_up request went ahead of us.
		 * Even if we do not want to shut this CPU down,
		 * the caller expects a certain state as if the WFI
		 * was aborted.  So let's continue with cache cleaning.
		 */
		skip_power_down = true;
	} else {
		/* all other state is error */
		BUG();
	}

	/* notify sun8i_mcpm_cpu_kill() that hardware shutdown is finished */
	if (!skip_power_down) {
		cpumask_set_cpu(cpu, &dead_cpus[cluster]);
	}

	if (last_man && __mcpm_outbound_enter_critical(cpu, cluster)) {
		arch_spin_unlock(&sun8i_mcpm_lock);

		/*
		 * Flush all cache levels for this cluster.
		 *
		 * Cluster1/Cluster0 can hit in the cache with SCTLR.C=0, so we don't need
		 * a preliminary flush here for those CPUs.  At least, that's
		 * the theory -- without the extra flush, Linux explodes on
		 * RTSM (maybe not needed anymore, to be investigated).
		 */
		flush_cache_all();
		set_cr(get_cr() & ~CR_C);
		flush_cache_all();

		/*
		 * This is a harmless no-op.  On platforms with a real
		 * outer cache this might either be needed or not,
		 * depending on where the outer cache sits.
		 */
		outer_flush_all();

		/* Disable local coherency by clearing the ACTLR "SMP" bit: */
		set_auxcr(get_auxcr() & ~(1 << 6));

		__mcpm_outbound_leave_critical(cluster, CLUSTER_DOWN);

		/* disable cluster cci snoop */
		disable_cci_snoops(cluster);
	} else {
		arch_spin_unlock(&sun8i_mcpm_lock);

		/*
		 * Flush the local CPU cache.
		 *
		 * Cluster1/Cluster0 can hit in the cache with SCTLR.C=0, so we don't need
		 * a preliminary flush here for those CPUs.  At least, that's
		 * the theory -- without the extra flush, Linux explodes on
		 * RTSM (maybe not needed anymore, to be investigated).
		 */
		flush_cache_louis();
		set_cr(get_cr() & ~CR_C);
		flush_cache_louis();

		/* Disable local coherency by clearing the ACTLR "SMP" bit: */
		set_auxcr(get_auxcr() & ~(1 << 6));
	}

	__mcpm_cpu_down(cpu, cluster);

	/* Now we are prepared for power-down, do it: */
	if (!skip_power_down) {
		dsb();
		while (1) {
			wfi();
		}
	}
	/* Not dead at this point?  Let our caller cope. */
}

static void sun8i_mcpm_powered_up(void)
{
	unsigned int mpidr, cpu, cluster;

	/* this code is execute in inbound cpu context */
	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	/* check outbound cluster have valid cpu core */
	if (sun8i_cluster_use_count[!cluster] == 0) {
		/* startup cluster power-down timer */
		hrtimer_start(&cluster_power_down_timer, ktime_set(0, 1000000), HRTIMER_MODE_REL);
	}
}

static void sun8i_smp_init_cpus(void)
{
	unsigned int i, ncores;

	ncores = MAX_NR_CLUSTERS * MAX_CPUS_PER_CLUSTER;

	/* sanity check */
	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %u cores greater than maximum (%u), clipping\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}
	/* setup the set of possible cpus */
	for (i = 0; i < ncores; i++) {
		set_cpu_possible(i, true);
	}
}

static int sun8i_mcpm_cpu_kill(unsigned int cpu)
{
	int i;
	int killer;
	unsigned int cluster_id;
	unsigned int cpu_id;
	unsigned int mpidr;

	mpidr      = cpu_logical_map(cpu);
	cpu_id     = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	killer = get_cpu();
	put_cpu();
	pr_debug("sun8i hotplug: cpu(%d) try to kill cpu(%d)\n", killer, cpu);

	for (i = 0; i < 1000; i++) {
		if (cpumask_test_cpu(cpu_id, &dead_cpus[cluster_id]) &&
        SUN8I_CPU_IS_WFI_MODE(cluster_id, cpu_id)) {
			local_irq_disable();
			arch_spin_lock(&sun8i_mcpm_lock);
			/* power-down cpu core */
			sun8i_cpu_power_set(cluster_id, cpu_id, 0);
#ifndef CONFIG_BL_SWITCHER
			/* if bL swithcer disable, the last-man should power-down cluster */
			if ((sun8i_cluster_use_count[cluster_id] == 0) && (sun8i_cluster_power_status(cluster_id))) {
				sun8i_cluster_power_set(cluster_id, 0);
			}
#endif
			arch_spin_unlock(&sun8i_mcpm_lock);
			local_irq_enable();
			pr_debug("sun8i hotplug: cpu:%d is killed!\n", cpu);
			return 1;
		}
		mdelay(1);
	}

	pr_err("sun8i hotplug: try to kill cpu:%d failed!\n", cpu);

	return 0;
}

int sun8i_mcpm_cpu_disable(unsigned int cpu)
{
	unsigned int cluster_id;
	unsigned int cpu_id;
	unsigned int mpidr;

	mpidr      = cpu_logical_map(cpu);
	cpu_id     = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	cpumask_clear_cpu(cpu_id, &dead_cpus[cluster_id]);
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	return cpu == 0 ? -EPERM : 0;
}

#if (defined CONFIG_ARCH_SUN8IW6)
static void sun8i_mcpm_cpu_suspend(u64 expected_residency)
{
	struct sunxi_cpuidle_para sunxi_idle_para;

	/* call cpus to power off */
	sunxi_idle_para.flags = 0;
	sunxi_idle_para.mpidr = (unsigned long)expected_residency;
	arisc_enter_cpuidle(NULL, NULL, &sunxi_idle_para);

	/* set this cpu die */
	sunxi_idle_cpu_die();
}
#endif

static const struct mcpm_platform_ops sun8i_mcpm_power_ops = {
	.power_up	= sun8i_mcpm_power_up,
	.power_down	= sun8i_mcpm_power_down,
	.powered_up     = sun8i_mcpm_powered_up,
	.smp_init_cpus  = sun8i_smp_init_cpus,
	.cpu_kill       = sun8i_mcpm_cpu_kill,
	.cpu_disable    = sun8i_mcpm_cpu_disable,
#if (defined CONFIG_ARCH_SUN8IW6)
	.suspend        = sun8i_mcpm_cpu_suspend,
#endif
};
#ifdef MCPM_WITH_ARISC_DVFS_SUPPORT
/* sun8i_arisc_notify_call - callback that gets triggered when arisc ready. */
static int sun8i_arisc_notify_call(struct notifier_block *nfb,
                                   unsigned long action, void *parg)
{
	switch (action) {
		case ARISC_INIT_READY: {
		unsigned int cpu;

		/* arisc ready now */
		set_arisc_ready(1);

		/* power-off cluster-1 first */
		sun8i_cluster_power_set(CLUSTER_1, 0);

		/* power-up off-line cpus*/
		for_each_present_cpu(cpu) {
			if (num_online_cpus() >= setup_max_cpus) {
				break;
			}
			if (!cpu_online(cpu)) {
				cpu_up(cpu);
			}
		}
		break;
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block sun8i_arisc_notifier = {
	&sun8i_arisc_notify_call,
	NULL,
	0
};
#endif
static void __init sun8i_mcpm_boot_cpu_init(void)
{
	unsigned int mpidr, cpu, cluster;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	BUG_ON(cpu >= MAX_CPUS_PER_CLUSTER || cluster >= MAX_NR_CLUSTERS);
	sun8i_cpu_use_count[cluster][cpu] = 1;
	sun8i_cluster_use_count[cluster]++;

	cpumask_clear(&cpu_power_up_state_mask);
	cpumask_set_cpu((cluster*4 + cpu),&cpu_power_up_state_mask);
}

int sun8i_mcpm_cpu_map_init(void)
{
	/* default cpu logical map */
	cpu_logical_map(0) = 0x000;
	cpu_logical_map(1) = 0x001;
	cpu_logical_map(2) = 0x002;
	cpu_logical_map(3) = 0x003;
	cpu_logical_map(4) = 0x100;
	cpu_logical_map(5) = 0x101;
	cpu_logical_map(6) = 0x102;
	cpu_logical_map(7) = 0x103;


	return 0;
}
extern void sun8i_power_up_setup(unsigned int affinity_level);
extern int __init cci_init(void);

static int __init sun8i_mcpm_init(void)
{
	int ret;

	/* initialize cci driver first */
	cci_init();

	/* initialize prcm and cpucfg model virtual base address */
	sun8i_prcm_base   = (void __iomem *)(SUNXI_R_PRCM_VBASE);
	sun8i_cpuxcfg_base = (void __iomem *)(SUNXI_CPUXCFG_VBASE);
	sun8i_cpuscfg_base = (void __iomem *)(SUNXI_R_CPUCFG_VBASE);
	sun8i_mcpm_boot_cpu_init();

	/* initialize cluster power-down timer */
	hrtimer_init(&cluster_power_down_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cluster_power_down_timer.function = sun8i_cluster_power_down;
	hrtimer_start(&cluster_power_down_timer, ktime_set(0, 10000000), HRTIMER_MODE_REL);

	ret = mcpm_platform_register(&sun8i_mcpm_power_ops);
	if (!ret) {
		ret = mcpm_sync_init(sun8i_power_up_setup);
	}

	/* set sun8i platform non-boot cpu startup entry. */
	sunxi_set_secondary_entry((void *)(virt_to_phys(mcpm_entry_point)));

	/* initialize cluster0 and cluster1 cluster pll number */
	cluster_pll[CLUSTER_0] = ARISC_DVFS_PLL1;
	cluster_pll[CLUSTER_1] = ARISC_DVFS_PLL2;

	/* initialize ca7 and ca15 cluster power-up freq as deafult */
	cluster_powerup_freq[CLUSTER_0] = SUN8I_C0_CLSUTER_PWRUP_FREQ;
	cluster_powerup_freq[CLUSTER_1] = SUN8I_C1_CLSUTER_PWRUP_FREQ;
#ifdef MCPM_WITH_ARISC_DVFS_SUPPORT
	/* register arisc ready notifier */
	arisc_register_notifier(&sun8i_arisc_notifier);
#endif
#ifdef CONFIG_FPGA_V7_PLATFORM
	/* hard-encode by sunny to support sun8i 2big+2little,
	 * we should use device-tree to config the cluster and cpu topology information.
	 * but sun8i not support device-tree now, so I just hard-encode for temp debug.
	 */
	cpu_logical_map(0) = 0x000;
	cpu_logical_map(1) = 0x001;
	cpu_logical_map(2) = 0x100;
	cpu_logical_map(3) = 0x101;
#endif

#ifdef CONFIG_ARCH_SUN8IW6
	soc_version = sunxi_get_soc_ver();
#endif

	return 0;
}
early_initcall(sun8i_mcpm_init);

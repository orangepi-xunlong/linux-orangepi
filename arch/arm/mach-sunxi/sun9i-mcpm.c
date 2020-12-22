/*
 * linux/arch/arm/mach-sunxi/sun9i-mcpm.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner sun9i platform multi-cluster power management operations.
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
#include <asm/bL_switcher.h>
#include <asm/hardware/gic.h>

#include <mach/platform.h>
#include <mach/cci.h>
#include <mach/sun9i/platsmp.h>
#include <linux/arisc/arisc.h>
#include <linux/arisc/arisc-notifier.h>
#include <mach/sys_config.h>
#include <mach/sunxi-smc.h>
#include <asm/firmware.h>

void sun9i_set_secondary_entry(void *entry)
{
	if (sunxi_soc_is_secure()) {
		call_firmware_op(set_secondary_entry, entry);
	} else {
		sunxi_set_secondary_entry(entry);
	}
}

/* sync with arisc module */
static bool arisc_ready = 0;
#define is_arisc_ready()   (arisc_ready)
#define set_arisc_ready(x) (arisc_ready = (x))

/* sun9i platform support two clusters,
 * cluster0 : cortex-a7,
 * cluster1 : cortex-a15.
 */
#define A7_CLUSTER	0
#define A15_CLUSTER	1
#define MAX_CLUSTERS	2

#define SUN9I_CPU_IS_WFI_MODE(cluster, cpu) (sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CLUSTER_CPU_STATUS(cluster)) & (1 << (16 + cpu)))
#define SUN9I_L2CACHE_IS_WFI_MODE(cluster)  (sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CLUSTER_CPU_STATUS(cluster)) & (1 << 0))

#define SUN9I_A7_CLSUTER_PWRUP_FREQ   (600000)  /* freq base on khz */
#define SUN9I_A15_CLSUTER_PWRUP_FREQ  (600000)  /* freq base on khz */

static unsigned int cluster_pll[MAX_CLUSTERS];
static unsigned int cluster_powerup_freq[MAX_CLUSTERS];

/*
 * We can't use regular spinlocks. In the switcher case, it is possible
 * for an outbound CPU to call power_down() after its inbound counterpart
 * is already live using the same logical CPU number which trips lockdep
 * debugging.
 */
static arch_spinlock_t sun9i_mcpm_lock = __ARCH_SPIN_LOCK_UNLOCKED;

/* sunxi cluster and cpu power-management models */
static void __iomem *sun9i_prcm_base;
static void __iomem *sun9i_cpucfg_base;
static void __iomem *sun9i_sram_b_vbase;
#define SUNXI_CPU0_SUPPORT_HOTPLUG_ADD1 	(0x1000)
#define SUNXI_CPU0_SUPPORT_HOTPLUG_ADD2 	(0x1004)

/* sunxi cluster and cpu use status,
 * this is use to detect the first-man and last-man.
 */
static int sun9i_cpu_use_count[MAX_NR_CLUSTERS][MAX_CPUS_PER_CLUSTER];
static int sun9i_cluster_use_count[MAX_NR_CLUSTERS];

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

static int sun9i_ca7_power_switch_set(unsigned int cluster, unsigned int cpu, bool enable)
{
	if (enable) {
		if (0x00 == sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			pr_debug("%s: power switch enable already\n", __func__);
			return 0;
		}
		/* de-active cpu power clamp */
		sunxi_smc_writel(0xFE, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(20);
		
		sunxi_smc_writel(0xF8, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);
		
		sunxi_smc_writel(0xE0, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);

		sunxi_smc_writel(0x80, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(10);

		sunxi_smc_writel(0x00, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(20);
		while(0x00 != sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			;
		}
	} else {
		if (0xFF == sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			pr_debug("%s: power switch disable already\n", __func__);
			return 0;
		}
		sunxi_smc_writel(0xFF, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		udelay(30);
		while(0xFF != sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			;
		}
	}
	return 0;
}

static int sun9i_ca15_power_switch_set(unsigned int cluster, unsigned int cpu, bool enable)
{
	if (sunxi_get_soc_ver() >= SUN9IW1P1_REV_B) {
		if (enable) {
			if (0x00 == sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
				pr_debug("%s: power switch enable already\n", __func__);
				return 0;
			}
			/* de-active cpu power clamp */
			sunxi_smc_writel(0xFE, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(20);

			sunxi_smc_writel(0xF8, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(10);

			sunxi_smc_writel(0xE0, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(10);

			sunxi_smc_writel(0x80, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(10);

			sunxi_smc_writel(0x00, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(20);
			while(0x00 != sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
				;
			}
		} else {
			if (0xFF == sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
				pr_debug("%s: power switch disable already\n", __func__);
				return 0;
			}
			sunxi_smc_writel(0xFF, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(30);
			while(0xFF != sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
				;
			}
		}
	} else {
		if (enable) {
			if (0xFF == sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
				pr_debug("%s: power switch enable already\n", __func__);
				return 0;
			}
			sunxi_smc_writel(0x01, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(20);
        	
			sunxi_smc_writel(0x07, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(10);
        	
			sunxi_smc_writel(0x1F, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(10);
        	
			sunxi_smc_writel(0x7F, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(10);
        	
			sunxi_smc_writel(0xFF, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(20);
			while(0xFF != sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
				;
			}
		} else {
			if (0x00 == sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
				pr_debug("%s: power switch disable already\n", __func__);
				return 0;
			}
			sunxi_smc_writel(0x00, sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
			udelay(30);
			while(0x00 != sunxi_smc_readl(sun9i_prcm_base + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
				;
			}
		}
	}
	return 0;
}

static int sun9i_cpu_power_switch_set(unsigned int cluster, unsigned int cpu, bool enable)
{
	int ret;
	if (cluster == A15_CLUSTER) {
		ret = sun9i_ca15_power_switch_set(cluster, cpu, enable);
	} else {
		ret = sun9i_ca7_power_switch_set(cluster, cpu, enable);
	}
	return ret;
}


#define SUN9I_IS_BOOT_CPU(cluster, cpu)   ((cluster == 0) && (cpu == 0))
static int sun9i_boot_cpu_hotplug_enable(bool enable)
{
	if(enable){
		sunxi_smc_writel(0xFA50392F,(sun9i_sram_b_vbase+SUNXI_CPU0_SUPPORT_HOTPLUG_ADD1));
		sunxi_smc_writel(0x790DCA3A,(sun9i_sram_b_vbase+SUNXI_CPU0_SUPPORT_HOTPLUG_ADD2));
	} else {
		sunxi_smc_writel(0x00000000,(sun9i_sram_b_vbase+SUNXI_CPU0_SUPPORT_HOTPLUG_ADD1));
		sunxi_smc_writel(0x00000000,(sun9i_sram_b_vbase+SUNXI_CPU0_SUPPORT_HOTPLUG_ADD2));
	}
	return 0;
}

int sun9i_cpu_power_set(unsigned int cluster, unsigned int cpu, bool enable)
{
	unsigned int value;

	if (enable) {
		/*
		 * power-up cpu core process
		 */
		pr_debug("sun9i power-up cluster-%d cpu-%d\n", cluster, cpu);

		cpumask_set_cpu(((cluster)*4 + cpu), &cpu_power_up_state_mask);

		/* if boot cpu, should enable boot cpu hotplug first.*/
		if (SUN9I_IS_BOOT_CPU(cluster, cpu)) {
			sun9i_boot_cpu_hotplug_enable(1);
		}

		/* assert cpu core reset */
		value  = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value &= (~(1<<cpu));
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		/* assert cpu power-on reset */
		value  = sunxi_smc_readl(sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		value &= (~(1<<cpu));
		sunxi_smc_writel(value, sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		udelay(10);

		/* L1RSTDISABLE hold low */
		if (cluster == A7_CLUSTER) {
			/* L1RSTDISABLE control bit just use for A7_CLUSTER,
			 * the A15_CLUSTER default reset by hardware when power-up, 
			 * software can't control it.
			 */
			value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL0(cluster));
			value &= ~(1<<cpu);
			sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL0(cluster));
		}

		/* release power switch */
		sun9i_cpu_power_switch_set(cluster, cpu, 1);

		/* clear power-off gating */
		value = sunxi_smc_readl(sun9i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		value &= (~(0x1<<cpu));
		sunxi_smc_writel(value, sun9i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		udelay(20);

		/* de-assert cpu power-on reset */
		value  = sunxi_smc_readl(sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		value |= ((1<<cpu));
		sunxi_smc_writel(value, sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		udelay(10);

		/* de-assert core reset */
		value  = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value |= (1<<cpu);
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		pr_debug("sun9i power-up cluster-%d cpu-%d already\n", cluster, cpu);
	} else {
		/*
		 * power-down cpu core process
		 */
		pr_debug("sun9i power-down cluster-%d cpu-%d\n", cluster, cpu);

		/* enable cpu power-off gating */
		value = sunxi_smc_readl(sun9i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		value |= (1 << cpu);
		sunxi_smc_writel(value, sun9i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		udelay(20);

		/* active the power output switch */
		sun9i_cpu_power_switch_set(cluster, cpu, 0);

		/* if boot cpu, should disable boot cpu hotplug.*/
		if (SUN9I_IS_BOOT_CPU(cluster, cpu)) {
			sun9i_boot_cpu_hotplug_enable(0);
		}
		cpumask_clear_cpu(((cluster)*4 + cpu), &cpu_power_up_state_mask);
		pr_debug("sun9i power-down cpu%d ok.\n", cpu);
	}
	return 0;
}

int sun9i_cluster_power_set(unsigned int cluster, bool enable)
{
	unsigned int value;
	int          i;
	
	/* cluster operation must wait arisc ready */
	if (!is_arisc_ready()) {
		pr_debug("%s: arisc not ready, can't power-up cluster\n", __func__);
		return -EINVAL;
	}
	if (enable) {
		pr_debug("sun9i power-up cluster-%d\n", cluster);

		/* active ACINACTM */
		value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL1(cluster));
		value |= (1<<0);
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL1(cluster));

		/* assert cluster cores resets */
		value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value &= (~(0xF<<0));   /* Core Reset    */
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		/* assert cluster cores power-on reset */
		value = sunxi_smc_readl(sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		value &= (~(0xF));
		sunxi_smc_writel(value, sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		udelay(10);
		
		/* assert cluster resets */
		value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value &= (~(0x1<<24));  /* SOC DBG Reset */
		value &= (~(0xF<<16));  /* Debug Reset   */
		value &= (~(0x1<<12));  /* HReset        */
		value &= (~(0x1<<8));   /* L2 Cache Reset*/
		if (cluster == A7_CLUSTER) {
			value &= (~(0xF<<20));  /* ETM Reset     */
		} else {
			value &= (~(0xF<<4));   /* Neon Reset   */
		}
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		/* Set L2RSTDISABLE LOW */
		if (cluster == A7_CLUSTER) {
			value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL0(cluster));
			value &= (~(0x1<<4));
			sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL0(cluster));
		} else {
			value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL0(cluster));
			value &= (~(0x1<<0));
			sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL0(cluster));			
		}

		/* notify arisc to power-up cluster */
		arisc_dvfs_set_cpufreq(cluster_powerup_freq[cluster], cluster_pll[cluster], 
		                       ARISC_MESSAGE_ATTR_SOFTSYN, NULL, NULL);
		mdelay(1);
		
		/* clear cluster power-off gating */
		value = sunxi_smc_readl(sun9i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		value &= (~(0x1<<4));
		sunxi_smc_writel(value, sun9i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		udelay(20);

		/* de-active ACINACTM */
		value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL1(cluster));
		value &= (~(1<<0));
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL1(cluster));

		/* de-assert cores reset */
		value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value |= (0x1<<24);  /* SOC DBG Reset */
		value |= (0xF<<16);  /* Debug Reset   */
		value |= (0x1<<12);  /* HReset        */
		value |= (0x1<<8);   /* L2 Cache Reset*/
		if (cluster == A7_CLUSTER) {
			value |= (0xF<<20);  /* ETM Reset     */
		} else {
			value |= (0xF<<4);   /* Neon Reset   */
		}
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
                udelay(20);
		
		/* de-assert cores power-on reset */
		value = sunxi_smc_readl(sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		value |= (0xF);
		sunxi_smc_writel(value, sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		udelay(60);
		
		/* de-assert cores reset */
		value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value |= (0xF<<0);   /* Core Reset    */
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
                udelay(20);
                
		pr_debug("sun9i power-up cluster-%d ok\n", cluster);
		
	} else {

		pr_debug("sun9i power-down cluster-%d\n", cluster);

		/* active ACINACTM */
		value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL1(cluster));
		value |= (1<<0);
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CLUSTER_CTRL1(cluster));
		
		while (1) {
			if (SUN9I_L2CACHE_IS_WFI_MODE(cluster)) {
				break;
			}
			/* maybe should support timeout to avoid deadloop */
		}
		
		/* assert cluster cores resets */
		value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value &= (~(0xF<<0));   /* Core Reset    */
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);
		
		/* assert cluster cores power-on reset */
		value = sunxi_smc_readl(sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		value &= (~(0xF));
		sunxi_smc_writel(value, sun9i_prcm_base + SUNXI_CLUSTER_PWRON_RESET(cluster));
		udelay(10);
		
		/* assert cluster resets */
		value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		value &= (~(0x1<<24));  /* SOC DBG Reset */
		value &= (~(0xF<<16));  /* Debug Reset   */
		value &= (~(0x1<<12));  /* HReset        */
		value &= (~(0x1<<8));   /* L2 Cache Reset*/
		if (cluster == A7_CLUSTER) {
			value &= (~(0xF<<20));  /* ETM Reset     */
		} else {
			value &= (~(0xF<<4));   /* Neon Reset   */
		}
		sunxi_smc_writel(value, sun9i_cpucfg_base + SUNXI_CPU_RST_CTRL(cluster));
		udelay(10);

		/* enable cluster and cores power-off gating */
		value = sunxi_smc_readl(sun9i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		value |= (1<<4);
		value |= (0xF<<0);
		sunxi_smc_writel(value, sun9i_prcm_base + SUNXI_CLUSTER_PWROFF_GATING(cluster));
		udelay(20);
		
		/* disable cluster cores power switch */
		for (i = 0; i < 4; i++) {
			sun9i_cpu_power_switch_set(cluster, i, 0);
		}

		/* notify arisc to power-down cluster,
		 * arisc will disable cluster clock and power-off cpu power domain.
		 */
		arisc_dvfs_set_cpufreq(0, cluster_pll[cluster], 
		                       ARISC_MESSAGE_ATTR_SOFTSYN, NULL, NULL);
		
		
		pr_debug("sun9i power-down cluster-%d ok\n", cluster);
	}

	return 0;
}

static int sun9i_cluster_power_status(unsigned int cluster)
{
	int          status = 0;
	unsigned int value;

	value = sunxi_smc_readl(sun9i_cpucfg_base + SUNXI_CLUSTER_CPU_STATUS(cluster));

	/* cluster WFI status :
	 * all cpu cores enter WFI mode + L2Cache enter WFI status
	 */
	if (((value >> 16) & 0xf) == 0xf) {
		status = 1;
	}

	return status;
}

enum hrtimer_restart sun9i_cluster_power_down(struct hrtimer *timer)
{
	ktime_t              period;
	int                  cluster;
	enum hrtimer_restart ret;

	arch_spin_lock(&sun9i_mcpm_lock);
	if (sun9i_cluster_use_count[0] == 0) {
		cluster = 0;
	} else if (sun9i_cluster_use_count[1] == 0) {
		cluster = 1;
	} else {
		ret = HRTIMER_NORESTART;
		goto end;
	}

	if (sun9i_cluster_power_status(cluster)) {
		period = ktime_set(0, 10000000);
		hrtimer_forward_now(timer, period);
		ret = HRTIMER_RESTART;
		goto end;
	} else {
		sun9i_cluster_power_set(cluster, 0);
		ret = HRTIMER_NORESTART;
	}
end:
	arch_spin_unlock(&sun9i_mcpm_lock);
	return ret;
}

static int sun9i_mcpm_power_up(unsigned int cpu, unsigned int cluster)
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
	arch_spin_lock(&sun9i_mcpm_lock);

	sun9i_cpu_use_count[cluster][cpu]++;
	if (sun9i_cpu_use_count[cluster][cpu] == 1) {
		sun9i_cluster_use_count[cluster]++;
		if (sun9i_cluster_use_count[cluster] == 1) {
			/* first-man should power-up cluster resource */
			pr_debug("%s: power-on cluster-%d", __func__, cluster);
			if (sun9i_cluster_power_set(cluster, 1)) {
				pr_debug("%s: power-on cluster-%d fail\n", __func__, cluster);
				sun9i_cluster_use_count[cluster]--;
				sun9i_cpu_use_count[cluster][cpu]--;
				arch_spin_unlock(&sun9i_mcpm_lock);
				local_irq_enable();
				return -EINVAL;
			}
		}
		/* power-up cpu core */
		sun9i_cpu_power_set(cluster, cpu, 1);
	} else if (sun9i_cpu_use_count[cluster][cpu] != 2) {
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
		pr_debug("sun9i power-on cluster-%d cpu-%d been skiped, usage count %d\n",
		         cluster, cpu, sun9i_cpu_use_count[cluster][cpu]);
	}
	arch_spin_unlock(&sun9i_mcpm_lock);
	local_irq_enable();

	return 0;
}

static void sun9i_mcpm_power_down(void)
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

	arch_spin_lock(&sun9i_mcpm_lock);
	BUG_ON(__mcpm_cluster_state(cluster) != CLUSTER_UP);

	sun9i_cpu_use_count[cluster][cpu]--;
	if (sun9i_cpu_use_count[cluster][cpu] == 0) {
		/* check is the last-man */
		sun9i_cluster_use_count[cluster]--;
		if (sun9i_cluster_use_count[cluster] == 0) {
			last_man = true;
		}
	} else if (sun9i_cpu_use_count[cluster][cpu] == 1) {
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

	/* notify sun9i_mcpm_cpu_kill() that hardware shutdown is finished */
	if (!skip_power_down) {
		cpumask_set_cpu(cpu, &dead_cpus[cluster]);
	}

	if (last_man && __mcpm_outbound_enter_critical(cpu, cluster)) {
		arch_spin_unlock(&sun9i_mcpm_lock);

		/*
		 * Flush all cache levels for this cluster.
		 *
		 * A15/A7 can hit in the cache with SCTLR.C=0, so we don't need
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
		arch_spin_unlock(&sun9i_mcpm_lock);

		/*
		 * Flush the local CPU cache.
		 *
		 * A15/A7 can hit in the cache with SCTLR.C=0, so we don't need
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

static void sun9i_mcpm_powered_up(void)
{
	unsigned int mpidr, cpu, cluster;

	/* this code is execute in inbound cpu context */
	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

#ifdef CONFIG_BL_SWITCHER
	if (is_arisc_ready()) {
		unsigned int ob_cluster,ob_cpu, i;
		if(cluster){
			ob_cluster = 0;
		}else{
			ob_cluster = 1;
		}
		ob_cpu = 3 - cpu;
		/*kill outbound cpu*/
		pr_debug("[%s]: inbound cpu(%d-%d) try to kill outbound cpu(%d-%d)\n",
		        __func__,cluster,cpu,ob_cluster,ob_cpu);
		for (i = 0; i < 1000; i++) {
			if (cpumask_test_cpu(ob_cpu, &dead_cpus[ob_cluster]) &&
					SUN9I_CPU_IS_WFI_MODE(ob_cluster, ob_cpu)) {
				/* power-down cpu core */
				sun9i_cpu_power_set(ob_cluster, ob_cpu, 0);
				pr_debug("[%s]: inbound cpu(%d-%d) kill outbound cpu(%d-%d)success\n"
								,__func__,cluster,cpu,ob_cluster,ob_cpu);
				break;
			}
			mdelay(1);
		}
	}
#endif
	/* check outbound cluster have valid cpu core */
	if (sun9i_cluster_use_count[!cluster] == 0) {
		/* startup cluster power-down timer */
		hrtimer_start(&cluster_power_down_timer, ktime_set(0, 1000000), HRTIMER_MODE_REL);
	}
}

static void sun9i_smp_init_cpus(void)
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

static int sun9i_mcpm_cpu_kill(unsigned int cpu)
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
	pr_debug("sun9i hotplug: cpu(%d) try to kill cpu(%d)\n", killer, cpu);

	for (i = 0; i < 1000; i++) {
		if (cpumask_test_cpu(cpu_id, &dead_cpus[cluster_id]) && 
		    SUN9I_CPU_IS_WFI_MODE(cluster_id, cpu_id)) {
			local_irq_disable();
			arch_spin_lock(&sun9i_mcpm_lock);
			/* power-down cpu core */
			sun9i_cpu_power_set(cluster_id, cpu_id, 0);

#ifndef CONFIG_BL_SWITCHER
			/* if bL swithcer disable, the last-man should power-down cluster */
			if ((sun9i_cluster_use_count[cluster_id] == 0) && (sun9i_cluster_power_status(cluster_id))) {
				sun9i_cluster_power_set(cluster_id, 0);
			}
#endif
			arch_spin_unlock(&sun9i_mcpm_lock);
			local_irq_enable();
			pr_debug("sun9i hotplug: cpu:%d is killed!\n", cpu);
			return 1;
		}
		mdelay(1);
	}
	pr_err("sun9i hotplug: try to kill cpu:%d failed!\n", cpu);

	return 0;
}

int sun9i_mcpm_cpu_disable(unsigned int cpu)
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

static const struct mcpm_platform_ops sun9i_mcpm_power_ops = {
	.power_up	= sun9i_mcpm_power_up,
	.power_down	= sun9i_mcpm_power_down,
	.powered_up     = sun9i_mcpm_powered_up,
	.smp_init_cpus  = sun9i_smp_init_cpus,
	.cpu_kill       = sun9i_mcpm_cpu_kill,
	.cpu_disable    = sun9i_mcpm_cpu_disable,
};

/* sun9i_arisc_notify_call - callback that gets triggered when arisc ready. */
static int sun9i_arisc_notify_call(struct notifier_block *nfb, 
                                   unsigned long action, void *parg)
{
	unsigned int mpidr, cpu, cluster;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	switch (action) {
		case ARISC_INIT_READY: {
		unsigned int cpu;
		
		/* arisc ready now */
		set_arisc_ready(1);
		
		if (cluster == A7_CLUSTER) {
			/* power-off cluster-a15 first */
			sun9i_cluster_power_set(A15_CLUSTER, 0);
		} else if (cluster == A15_CLUSTER) {
			/* power-off cluster-a7 first */
			sun9i_cluster_power_set(A7_CLUSTER, 0);
		}
		
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

static struct notifier_block sun9i_arisc_notifier = { 
	&sun9i_arisc_notify_call, 
	NULL, 
	0 
};

static void __init sun9i_mcpm_boot_cpu_init(void)
{
	unsigned int mpidr, cpu, cluster;

	mpidr = read_cpuid_mpidr();
	cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	pr_debug("%s: cpu %u cluster %u\n", __func__, cpu, cluster);
	BUG_ON(cpu >= MAX_CPUS_PER_CLUSTER || cluster >= MAX_NR_CLUSTERS);
	sun9i_cpu_use_count[cluster][cpu] = 1;
	sun9i_cluster_use_count[cluster]++;

    cpumask_clear(&cpu_power_up_state_mask);
    cpumask_set_cpu((cluster*4 + cpu),&cpu_power_up_state_mask);
}


static int sun9i_mcpm_get_cfg(char *main, char *sub, u32 *val)
{
	script_item_u script_val;
	script_item_value_type_e type;
	type = script_get_item(main, sub, &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: %s-%s config type err", __func__, main, sub);
		return -EINVAL;
	}
	*val = script_val.val;
	pr_debug("%s: mcpm config [%s][%s] : %d\n", __func__, main, sub, *val);
	return 0;
}

int sun9i_mcpm_cpu_map_init(void)
{
	int index = 0;
	u32 cpu_count = 0;
	u32 logical_map = 0;
	char cpu_name[32];

	/* default cpu logical map */
	cpu_logical_map(0) = 0x000;
	cpu_logical_map(1) = 0x001;
	cpu_logical_map(2) = 0x002;
	cpu_logical_map(3) = 0x003;
	cpu_logical_map(4) = 0x100;
	cpu_logical_map(5) = 0x101;
	cpu_logical_map(6) = 0x102;
	cpu_logical_map(7) = 0x103;

	/* readout config mapping information from sys_config */
	if (sun9i_mcpm_get_cfg("cpu_logical_map", "cpu_count", &cpu_count)) {
		pr_warn("%s: invalid config cpu_count\n", __func__);
		return -EINVAL;
	}
	for (index = 0; index < cpu_count; index++) {
		sprintf(cpu_name, "cpu%d", index);
		if (sun9i_mcpm_get_cfg("cpu_logical_map", cpu_name, &logical_map) == 0) {
			cpu_logical_map(index) = logical_map;
		}
	}

	return 0;
}

extern void sun9i_power_up_setup(unsigned int affinity_level);

extern int __init cci_init(void);
static int __init sun9i_mcpm_init(void)
{
	int ret;

	/* initialize cci driver first */
	cci_init();

	/* initialize prcm and cpucfg model virtual base address */
	sun9i_prcm_base   = (void __iomem *)(SUNXI_R_PRCM_VBASE);
	sun9i_cpucfg_base = (void __iomem *)(SUNXI_R_CPUCFG_VBASE);
	sun9i_sram_b_vbase = (void __iomem *)(SUNXI_SRAM_B_VBASE);

	sun9i_mcpm_boot_cpu_init();

	/* initialize cluster power-down timer */
	hrtimer_init(&cluster_power_down_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cluster_power_down_timer.function = sun9i_cluster_power_down;
	hrtimer_start(&cluster_power_down_timer, ktime_set(0, 10000000), HRTIMER_MODE_REL);

	ret = mcpm_platform_register(&sun9i_mcpm_power_ops);
#ifndef CONFIG_SUNXI_TRUSTZONE
	if (!ret) {
		ret = mcpm_sync_init(sun9i_power_up_setup);
	}
#endif
	/* map brom address to 0x0 */
	sunxi_smc_writel(0x16AA0000, (void *)0xF08000E0);

	/* set sun9i platform non-boot cpu startup entry. */
	sun9i_set_secondary_entry((void *)(virt_to_phys(mcpm_entry_point)));

	/* initialize ca7 and ca15 cluster pll number */
	cluster_pll[A7_CLUSTER] = ARISC_DVFS_PLL1;
	cluster_pll[A15_CLUSTER] = ARISC_DVFS_PLL2;

	/* initialize ca7 and ca15 cluster power-up freq as deafult */
	cluster_powerup_freq[A7_CLUSTER] = SUN9I_A7_CLSUTER_PWRUP_FREQ;
	cluster_powerup_freq[A15_CLUSTER] = SUN9I_A15_CLSUTER_PWRUP_FREQ;
	
	/* register arisc ready notifier */
	arisc_register_notifier(&sun9i_arisc_notifier);
	
	return 0;
}
early_initcall(sun9i_mcpm_init);

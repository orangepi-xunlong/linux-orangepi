/*
 * arch/arm/mach-sunxi/include/mach/sun9i/platsmp.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sun9i smp ops header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUN9I_PLAT_SMP_H
#define __SUN9I_PLAT_SMP_H

#include <asm/smp_plat.h>

static inline void enable_cpu(int cpu_nr)
{
	unsigned int cluster;
	unsigned int cpu;
	unsigned int mpidr;
	unsigned int value;

	mpidr   = cpu_logical_map(cpu_nr);
	cpu     = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	/*
	 * power-up cpu core process
	 */
	/* step1: Assert nCOREPORESET LOW and hold L1RSTDISABLE LOW.
	          Ensure DBGPWRDUP is held LOW to prevent any external
	          debug access to the processor.
	*/
	/* assert cpu power-on reset */
	value  = readl(SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWRON_RESET(cluster));
	value &= (~(1<<cpu));
	writel(value, SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWRON_RESET(cluster));

	/* assert cpu core reset */
	value  = readl(SUNXI_R_CPUCFG_VBASE + SUNXI_CPU_RST_CTRL(cluster));
	value &= (~(1<<cpu));
	writel(value, SUNXI_R_CPUCFG_VBASE + SUNXI_CPU_RST_CTRL(cluster));

	///* L1RSTDISABLE hold low */
	//value = readl(SUNXI_R_CPUCFG_VBASE + AW_CPUCFG_GENCTL);
	//value &= ~(1<<cpu);
	//writel(value, SUNXI_R_CPUCFG_VBASE + AW_CPUCFG_GENCTL);

	/* step2: release power clamp */
	writel(0x00, SUNXI_R_PRCM_VBASE + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
	while(0x00 != readl(SUNXI_R_PRCM_VBASE + SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
		;
	}
	mdelay(2);

	/* step3: clear power-off gating */
	value = readl(SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWROFF_GATING(cluster));
	value &= (~(0x1<<cpu));
	writel(value, SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWROFF_GATING(cluster));
	mdelay(1);

	/* step4: de-assert core reset */
	value  = readl(SUNXI_R_CPUCFG_VBASE + SUNXI_CPU_RST_CTRL(cluster));
	value |= (1<<cpu);
	writel(value, SUNXI_R_CPUCFG_VBASE + SUNXI_CPU_RST_CTRL(cluster));

	/* step4: de-assert cpu power-on reset */
	value  = readl(SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWRON_RESET(cluster));
	value |= ((1<<cpu));
	writel(value, SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWRON_RESET(cluster));
}

static inline void disable_cpu(int cpu_nr)
{
	unsigned int cluster;
	unsigned int cpu;
	unsigned int mpidr;
	unsigned int value;

	mpidr   = cpu_logical_map(cpu_nr);
	cpu     = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);

	/* step11: assert cpu power-on reset */
	value  = readl(SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWRON_RESET(cluster));
	value &= (~(1<<cpu));
	writel(value, SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWRON_RESET(cluster));

	/* step10: assert cpu core reset */
	value  = readl(SUNXI_R_CPUCFG_VBASE + SUNXI_CPU_RST_CTRL(cluster));
	value &= (~(1<<cpu));
	writel(value, SUNXI_R_CPUCFG_VBASE + SUNXI_CPU_RST_CTRL(cluster));

	/* enable cluster power-off gating */
	value = readl(SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWROFF_GATING(cluster));
	value |= (1 << cpu);
	writel(value, SUNXI_R_PRCM_VBASE + SUNXI_CLUSTER_PWROFF_GATING(cluster));
	mdelay(1);

	/* step12: active the power output clamp */
	writel(0xFF, SUNXI_R_PRCM_VBASE + SUNXI_CPU_PWR_CLAMP(cluster, cpu));
}

/*
 * set the sencodary cpu boot entry address.
 */
static inline void sunxi_set_secondary_entry(void *entry)
{
	writel((u32)entry, (void *)(SUNXI_R_PRCM_VBASE + PRIVATE_REG0));
}

/*
 * set the boot cpu hotplug flg.
 */
#define CPU0_SUPPORT_HOTPLUG_ADD1   	(0x1000)
#define CPU0_SUPPORT_HOTPLUG_ADD2   	(0x1004)

static inline void sunxi_set_bootcpu_hotplugflg(void)
{
	writel(0xFA50392F, (SUNXI_SRAM_B_VBASE + CPU0_SUPPORT_HOTPLUG_ADD1));
	writel(0x790DCA3A, (SUNXI_SRAM_B_VBASE + CPU0_SUPPORT_HOTPLUG_ADD2));
}

/*
 * get the sencodary cpu boot entry address.
 */
static inline void *sunxi_get_secondary_entry(void)
{
	return (void *)readl(SUNXI_R_PRCM_VBASE + PRIVATE_REG0);
}


#endif /* __SUN9I_PLAT_SMP_H */

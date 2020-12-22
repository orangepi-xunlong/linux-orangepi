/*
 * linux/arch/arm/mach-sunxi/sun9i-cci.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner sun9i soc platform CCI(Cache Coherent Interconnect) driver.
 * this init version clone from samsung exynos platform.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/cache.h>
#include <linux/syscore_ops.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <asm/cacheflush.h>

#include <mach/cci.h>
#include <mach/platform.h>
#include <mach/cci-regs.h>

static void __iomem *cci_base;
static int cci_enabled __read_mostly;

void enable_cci_snoops(unsigned int cluster_id)
{
	void __iomem *control_reg;
	unsigned int value;

	if (!cci_enabled) {
		return;
	}

	//pr_info("sunxi cci: enable cluster[%d] snoop\n", cluster_id);

	if (cluster_id) {
		control_reg = CCI_A15_SL_IFACE(cci_base) + SNOOP_CTLR_REG;
	} else {
		control_reg = CCI_A7_SL_IFACE(cci_base) + SNOOP_CTLR_REG;
	}

	if ((readl(control_reg) & 0x3) == 0x3) {
		/* cci snoop enable already */
		return;
	}

	/* Turn on CCI snoops */
	value = readl(control_reg);
	value |= 0x3;
	writel(value, control_reg);
	dsb();

	/* Wait for the dust to settle down */
	while (readl(cci_base + STATUS_REG) & 0x1);

	return;
}

void disable_cci_snoops(unsigned int cluster_id)
{
	void __iomem *control_reg;
	unsigned int value;

	if (!cci_enabled) {
		return;
	}

	if (cluster_id) {
		control_reg = CCI_A15_SL_IFACE(cci_base) + SNOOP_CTLR_REG;
	} else {
		control_reg = CCI_A7_SL_IFACE(cci_base) + SNOOP_CTLR_REG;
	}

	if (!(readl(control_reg) & 0x3)) {
		return;
	}

	/* Turn off CCI snoops */
	value = readl(control_reg);
	value &= (~0x3);
	writel(value, control_reg);
	dsb();

	/* Wait for the dust to settle down */
	while (readl(cci_base + STATUS_REG) & 0x1);

	return;
}

/*
 * Use our own MPIDR accessors as the generic ones in asm/cputype.h have
 * __attribute_const__ and we don't want the compiler to assume any
 * constness here.
 */

static int read_mpidr(void)
{
	unsigned int id;
	asm volatile ("mrc\tp15, 0, %0, c0, c0, 5" : "=r" (id));
	return id;
}

#if defined(CONFIG_PM)
static int cci_status[2];

static int get_cci_snoop_status(unsigned int cluster_id)
{
	void __iomem *control_reg;

	if (!cci_enabled) {
		return 0;
	}

	if (cluster_id) {
		control_reg = CCI_A15_SL_IFACE(cci_base) + SNOOP_CTLR_REG;
		
	} else {
		control_reg = CCI_A7_SL_IFACE(cci_base) + SNOOP_CTLR_REG;
	}

	if ((readl(control_reg) & 0x3)) {
		return 1;
	}

	return 0;
}

static int cci_suspend(void)
{
	int i;

	if (!cci_enabled) {
		return 0;
	}

	for (i = 0; i < 2; i++) {
		cci_status[i] = get_cci_snoop_status(i);
	}

	return 0;
}

static void cci_resume(void)
{
	unsigned int cluster_id = (read_mpidr() >> 8) & 0xf;

	if((cluster_id !=0) && (cluster_id !=1)){
		pr_info("[%s]cluster id error! cluster id %d\n",__func__,cluster_id);
	}

	if(cci_enabled){
		if (cci_status[cluster_id]) {
			enable_cci_snoops(cluster_id);
		}
	}

}
#else
#define cci_suspend NULL
#define cci_resume  NULL
#endif

static struct syscore_ops cci_syscore_ops = {
	.suspend	= cci_suspend,
	.resume		= cci_resume,
};

int __init cci_init(void)
{
	unsigned int cluster_id = (read_mpidr() >> 8) & 0xf;

#if defined(CONFIG_SUN9I_CCI)
	cci_enabled = 1;
#endif
	if (!cci_enabled) {
		pr_info("sunxi cci is not supported\n");
		goto disabled;
	}
	cci_base = (void __iomem *)(SUNXI_CCI400_VBASE);

	enable_cci_snoops(cluster_id);

disabled:
	register_syscore_ops(&cci_syscore_ops);

	return 0;
}
//early_initcall(cci_init);

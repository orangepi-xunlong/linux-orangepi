/*
 * linux/include/linux/sunxi-cpufreq.h
 *
 * (C) Copyright 2017-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * lsh <liush@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _LINUX_SUNXI_CPUFREQ_H
#define _LINUX_SUNXI_CPUFREQ_H

/* dvfs config */
#define DVFS_VF_TABLE_MAX         (16)
struct cpufreq_dvfs_table {
	u32 freq;
	u32 voltage;
	u32 axi_div;
#ifdef CONFIG_ARM_SUNXI_AVS
	u32 pval;
#endif
};
#endif /* _LINUX_SUNXI_CPUFREQ_H*/

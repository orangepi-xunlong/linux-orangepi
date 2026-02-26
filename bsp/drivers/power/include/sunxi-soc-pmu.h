/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Based on the sunxi-soc-pmu driver and the previous sunxi-soc-pmu driver
 */

#ifndef __LINUX_MFD_SOC_PMU_H
#define __LINUX_MFD_SOC_PMU_H

#include "sunxi-power-mfd.h"

enum {
	V821_ID = 0,
	NR_SOC_PMU_VARIANTS,
};

/* For V821 */
#define V821_POWER_CTRL_BASE				(0x4A000800)

#define V821_WAKE_IO_CTRL					(0x0)
#define V821_LDO_RTC_1V8_CTRL				(0x4)
#define V821_LDO_RTC_0V9_CTRL				(0x8)
#define V821_LDO_2V8_CTRL					(0x10)
#define V821_LDO_1V8_CTRL					(0x14)
#define V821_POWER_CTRL_END					(0x94)

#define V821_POWER_CTRL_AON_BASE			(0x4A011000)

#define V821_RES_LIST_APP_BASE				(0x43045000)
#define V821_RES_REQ_ACT_CTRL				(0x0)

enum {
	V821_LDO1 = 0,		//ldo 2V8
	V821_PMU_REG_ID_MAX,
};

struct soc_pmu_dev {
	struct device 			*dev;
	struct regmap			*regmap;
	long		 		variant;
	const struct regmap_config	*regmap_cfg;
	void (*dts_parse)(struct soc_pmu_dev *);
};

#define devm_regmap_init_sunxi_soc_pmu(dev, regs, config) devm_regmap_init_mmio(dev, regs, config)

#endif /*  __LINUX_MFD_SOC_PMU_H */

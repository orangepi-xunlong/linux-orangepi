/*
 * Copyright (C) 2019 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: Albert Yu <yuxyun@allwinnertech.com>
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_
#include "img_types.h"
#include "pvrsrv_error.h"
#include "pvrsrv_device.h"
#include "servicesext.h"
#include <linux/version.h>
#include <linux/reset.h>

enum scene_ctrl_cmd {
	SCENE_CTRL_NORMAL_MODE,
	SCENE_CTRL_PERFORMANCE_MODE
};

struct sunxi_clks {
	struct clk *pll;
	struct clk *core;
	struct clk *bus;
	struct reset_control *reset;
};

struct sunxi_clks_table {
	long core_clk;
	long true_clk;
	int  gpu_dfs;
	unsigned long volt;
};

struct sunxi_platform {
	bool initialized;
	u32 reg_base;
	u32 reg_size;
	u32 irq_num;
	bool power_on;
	bool power_idle;
	bool dvfs;
	bool soft_mode;
	bool independent_power;
	bool man_ctrl;
	bool scenectrl;
	spinlock_t lock;
	void __iomem *ppu_reg;
	void __iomem *gpu_reg;
	struct device *dev;
	struct regulator	*regula;
	struct sunxi_clks clks;
	long pll_clk_rate;
	int table_num;
	struct sunxi_clks_table *clk_table;
	struct sunxi_clks_table *current_clk;
	PVRSRV_DEVICE_CONFIG   *config;
};

bool sunxi_ic_version_ctrl(struct device *dev);
int sunxi_platform_init(struct device *dev);
void sunxi_platform_term(void);
void sunxiSetFrequencyDVFS(IMG_UINT32 ui32Frequency);
void sunxiSetVoltageDVFS(IMG_UINT32 ui32Volt);
PVRSRV_ERROR sunxiPrePowerState(IMG_HANDLE hSysData,
					 PVRSRV_SYS_POWER_STATE eNewPowerState,
					 PVRSRV_SYS_POWER_STATE eCurrentPowerState,
					 IMG_BOOL bForced);
PVRSRV_ERROR sunxiPostPowerState(IMG_HANDLE hSysData,
					  PVRSRV_SYS_POWER_STATE eNewPowerState,
					  PVRSRV_SYS_POWER_STATE eCurrentPowerState,
					  IMG_BOOL bForced);

#endif /* _PLATFORM_H_ */

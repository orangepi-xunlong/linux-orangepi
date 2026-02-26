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

enum scene_ctrl_cmd {
	SCENE_CTRL_NORMAL_MODE,
	SCENE_CTRL_PERFORMANCE_MODE
};

struct reg {
	unsigned long phys;
	void __iomem *ioaddr;
};

struct sunxi_regs {
	struct reg drm;
};

struct sunxi_data {
	struct sunxi_regs regs;
	struct clk *pll_gpu;
	struct reset_control *reset;
	bool idle_ctrl;
	bool dvfs_ctrl;
	bool independent_power;
	bool sence_ctrl;
	bool power_on;
	bool is_resume_pll_gpu;
	struct mutex sunxi_lock;
	unsigned long max_freq;
	unsigned long max_u_volt;
	unsigned long current_freq;
	unsigned long current_u_volt;
	struct kbasep_pm_metrics sunxi_last;
};

#endif /* _PLATFORM_H_ */

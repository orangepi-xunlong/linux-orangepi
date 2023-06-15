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
#if defined(CONFIG_ARCH_SUN50IW9)
	struct reg poweroff_gating;
#endif
};

struct sunxi_data {
	struct sunxi_regs regs;
#if defined(CONFIG_ARCH_SUN50IW9)
	struct clk *bak_clk;
#endif
	struct clk *bus_clk;
	struct reset_control *reset;
	bool man_ctrl;
	bool idle_ctrl;
	bool dvfs_ctrl;
	bool independent_power;
	bool sence_ctrl;
	struct mutex sunxi_lock;
	bool power_on;
	bool clk_on;
	unsigned long max_freq;
	unsigned long max_u_volt;
#if defined(CONFIG_ARCH_SUN50IW9)
	unsigned long bak_freq;
	unsigned long bak_u_volt;
#endif
	unsigned long current_freq;
	unsigned long current_u_volt;
	struct kbasep_pm_metrics sunxi_last;
};

#endif /* _PLATFORM_H_ */

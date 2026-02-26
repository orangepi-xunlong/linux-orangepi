/*
 * Copyright (C) 2023 Allwinner Technology Limited. All rights reserved.
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
 * Author: zhengwanyu<zhengwanyu@allwinnertech.com>
 */
#ifndef _SUNXI_DVFS_CTRL_H_
#define _SUNXI_DVFS_CTRL_H_

#include <linux/device.h>
#include <linux/clk.h>

struct sunxi_dvfs_init_params {
	unsigned long reg_base;

	struct clk *clk;
	unsigned int clk_rate;

	unsigned int irq_no;
};


#define FREQ_DELAY_MS  2
#define SUNXI_DVFS_CTRL_OFFSET 0x80000

int sunxi_dvfs_ctrl_init(struct device *dev,
	struct sunxi_dvfs_init_params *para);
int sunxi_dvfs_ctrl_get_opp_table(IMG_OPP **opp, u32 *num);
#endif

/**
 * Copyright (C) 2015-2016 Allwinner Technology Limited. All rights reserved.
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

#ifndef _MALI_SUN8I_W11P1_H_
#define _MALI_SUN8I_W11P1_H_

aw_private_data aw_private = {
#ifdef CONFIG_MALI_DT
	.np_gpu        = NULL,
#endif /* CONFIG_MALI_DT */
	.tempctrl      = {
		.temp_ctrl_status = 1,
	},
	.pm            = {
		.regulator      = NULL,
		.regulator_id   = "vdd-gpu",
		.clk[0]         = {
			.clk_name   = "pll",
			.clk_handle = NULL,
		},
		.clk[1]         = {
			.clk_name   = "mali",
			.clk_handle = NULL,
		},
		.dvfs_status   = 0,
		.vf_table[0]   = {
			.vol  = 0,
			.freq = 144,
		},
		.vf_table[1]   = {
			.vol  = 1100,
			.freq = 240,
		},
		.vf_table[2]   = {
			.vol  = 1100,
			.freq = 312,
		},
		.vf_table[3]   = {
			.vol  = 1100,
			.freq = 384,
		},
		.dvfs_status       = 0,
		.begin_level       = 3,
		.max_level         = 3,
		.scene_ctrl_cmd    = 0,
		.scene_ctrl_status = 1,
		.independent_pow   = 0,
		.dvm               = 0,
	},
	.debug           = {
		.enable      = 0,
		.frequency   = 0,
		.voltage     = 0,
		.tempctrl    = 0,
		.scenectrl   = 0,
		.dvfs        = 0,
		.level       = 0,
	}
};

#endif

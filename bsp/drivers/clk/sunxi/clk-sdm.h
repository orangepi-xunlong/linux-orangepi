/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022. All rights reserved.
 */

#ifndef _CCU_SDM_H
#define _CCU_SDM_H
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>

#define DTS_SDM_OFF	0
#define DTS_SDM_ON	1
#define CODE_SDM	2

struct clk_sdm_info {
	const char *clk_name;
	int sdm_enable;
	int sdm_factor;
	int freq_mode;
	int sdm_freq;
};

int sunxi_parse_sdm_info(struct device_node *node);
int sunxi_clk_get_sdm_info(const char *clk_name, struct clk_sdm_info *sdm_info_out);

#endif

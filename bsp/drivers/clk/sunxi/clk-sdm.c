// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2022 Allwinner.
 * Emma <liujuan1@allwinnertech.com>
 */

#include "clk-sdm.h"
#include <dt-bindings/clock/sunxi-clk.h>

struct clk_sdm_info *sdm_info;
static int len;

int sunxi_parse_sdm_info(struct device_node *node)
{
	int i = 0;
	struct device_node *child_node = NULL;
	struct device_node *sdm_node = NULL;

	if (!node)
		return -1;

	sdm_node = of_get_child_by_name(node, "sdm_info");
	if (!sdm_node)
		return -1;

	len = of_get_child_count(sdm_node);
	if (!len)
		return -1;

	sdm_info = kcalloc(len, sizeof(struct clk_sdm_info), GFP_KERNEL);
	if (!sdm_info) {
		return -ENOMEM;
	}

	for_each_child_of_node(sdm_node, child_node) {
		if (of_property_read_u32(child_node, "sdm-enable", &sdm_info[i].sdm_enable))
			continue;
		if (sdm_info[i].sdm_enable != DTS_SDM_OFF &&
		    sdm_info[i].sdm_enable != DTS_SDM_ON) {
			pr_err("clk: %s invaild enable: %d, should be 0 or 1\n",
				child_node->name, sdm_info[i].sdm_enable);
			continue;
		}

		if (of_property_read_u32(child_node, "sdm-factor", &sdm_info[i].sdm_factor))
			continue;
		/* the unit of factor is 1/1000 */
		if (sdm_info[i].sdm_factor > 99) {
			pr_err("clk: %s invaild factor: %d, should be within 99\n",
				child_node->name, sdm_info[i].sdm_factor);
			continue;
		}

		if (of_property_read_u32(child_node, "freq-mode", &sdm_info[i].freq_mode)) {
			sdm_info[i].freq_mode = TR_N;
		} else {
			if (sdm_info[i].freq_mode < 0 || sdm_info[i].freq_mode > 1) {
				pr_err("clk: %s invaild freq_mod: %d, should be within 0~1, use default TR_N\n",
					child_node->name, sdm_info[i].sdm_factor);
				sdm_info[i].freq_mode = TR_N;
			}
		}

		if (of_property_read_u32(child_node, "sdm-freq", &sdm_info[i].sdm_freq)) {
			sdm_info[i].sdm_freq = FREQ_31_5;
		} else {
			if (sdm_info[i].sdm_freq < 0 || sdm_info[i].sdm_freq > 3) {
				pr_err("clk: %s invaild sdm_freq : %d, should be within 0~3, use default FREQ_31_5\n", child_node->name, sdm_info[i].sdm_factor);
				sdm_info[i].sdm_freq = FREQ_31_5;
			}
		}

		sdm_info[i].clk_name = child_node->name;
		i++;
	}
	len = i;
	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_parse_sdm_info);

int sunxi_clk_get_sdm_info(const char *clk_name, struct clk_sdm_info *sdm_info_out)
{
	int i;
	sdm_info_out->clk_name = clk_name;
	if (!sdm_info)
		return -1;

	for (i = 0; i < len; i++) {
		if (!strcmp(clk_name, sdm_info[i].clk_name)) {
			sdm_info_out->sdm_enable = sdm_info[i].sdm_enable;
			sdm_info_out->sdm_factor = sdm_info[i].sdm_factor;
			sdm_info_out->freq_mode  = sdm_info[i].freq_mode;
			sdm_info_out->sdm_freq   = sdm_info[i].sdm_freq;
			return 0;
		};
	}
	return -1;
}
EXPORT_SYMBOL_GPL(sunxi_clk_get_sdm_info);

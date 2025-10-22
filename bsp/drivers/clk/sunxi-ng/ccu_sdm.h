/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2017 Chen-Yu Tsai. All rights reserved.
 */

#ifndef _CCU_SDM_H
#define _CCU_SDM_H

#include <linux/clk-provider.h>
#include <dt-bindings/clock/sunxi-ccu.h>

#include "ccu_common.h"

struct ccu_sdm_setting {
	u64		rate;

	/*
	 * XXX We don't know what the step and bottom register fields
	 * mean. Just copy the whole register value from the vendor
	 * kernel for now.
	 */
	u32		pattern;

	/*
	 * M and N factors here should be the values used in
	 * calculation, not the raw values written to registers
	 */
	u32		m;
	u32		n;
};

struct ccu_sdm_internal {
	struct ccu_sdm_setting	*table;
	u32		table_size;
	/* early SoCs don't have the SDM enable bit in the PLL register */
	u32		enable;
	/* second enable bit in tuning register */
	u32		tuning_enable;
	u16		tuning_reg;
	/* on some platforms, the sdm enable bit in pattern1 register */
	u32		pattern1_enable;
	u16		pattern1_reg;
};

struct clk_sdm_info {
	const char *clk_name;
	int sdm_enable;
	int sdm_factor;
	int freq_mode;
	int sdm_freq;
};

#define _SUNXI_CCU_SDM_COMPLETE(_table, _enable,	\
				   _reg, _reg_enable, _reg_1, _reg_1_enable)		\
	{						\
		.table		= _table,		\
		.table_size = ARRAY_SIZE(_table),	\
		.enable 	= _enable,		\
		.tuning_enable	= _reg_enable,		\
		.tuning_reg = _reg, 		\
		.pattern1_enable	= _reg_1_enable,		\
		.pattern1_reg	= _reg_1,			\
	}

#define _SUNXI_CCU_SDM(_table, _enable,	_reg, _reg_enable)		\
	_SUNXI_CCU_SDM_COMPLETE(_table, _enable, _reg, _reg_enable, 0, 0)

#define _SUNXI_CCU_SDM_PATTERN1(_table, _reg, _reg_enable, _reg_1, _reg_1_enable)		\
	_SUNXI_CCU_SDM_COMPLETE(_table, 0, _reg, _reg_enable, _reg_1, _reg_1_enable)


#define _SUNXI_CCU_SDM_INFO(_enable, _reg)		\
	{						\
		.enable		= _enable,			\
		.tuning_reg	= _reg,		\
	}

#define _SUNXI_CCU_SDM_PATTERN1_INFO(_enable, _reg, _enable_1, _reg_1)		\
	{						\
		.enable		= _enable,			\
		.tuning_reg	= _reg,		\
		.pattern1_enable	= _enable_1,		\
		.pattern1_reg	= _reg_1,			\
	}

#define DTS_SDM_OFF	0
#define DTS_SDM_ON	1

void ccu_common_set_sdm_value(struct ccu_common *common, struct ccu_sdm_internal *sdm, u32 sdmval);
u32 ccu_get_sdmval(unsigned long rate, struct ccu_common *common, u32 m, u32 n);
int sunxi_parse_sdm_info(struct device_node *node);
struct clk_sdm_info *sunxi_clk_get_sdm_info(const char *clk_name);
bool ccu_sdm_helper_is_enabled(struct ccu_common *common,
			       struct ccu_sdm_internal *sdm);
void ccu_sdm_helper_enable(struct ccu_common *common,
			   struct ccu_sdm_internal *sdm,
			   u64 rate);
void ccu_sdm_helper_disable(struct ccu_common *common,
			    struct ccu_sdm_internal *sdm);

bool ccu_sdm_helper_has_rate(struct ccu_common *common,
			     struct ccu_sdm_internal *sdm,
			     u64 rate);

unsigned long ccu_sdm_helper_read_rate(struct ccu_common *common,
				       struct ccu_sdm_internal *sdm,
				       u32 m, u32 n);

int ccu_sdm_helper_get_factors(struct ccu_common *common,
			       struct ccu_sdm_internal *sdm,
			       u64 rate,
			       unsigned long *m, unsigned long *n);

#endif

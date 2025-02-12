// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023, ky Corporation.
 *
 */

#ifndef _CCU_DPLL_H_
#define _CCU_DPLL_H_

#include <linux/spinlock_types.h>
#include <linux/clk-provider.h>
#include "ccu-ky-x1.h"

struct ccu_dpll_rate_tbl {
	unsigned long long rate;
	u32 reg0;
	u32 reg1;
	u32 reg2;
	u32 reg3;
	u32 reg4;
	u32 reg5;
	u32 reg6;
	u32 reg7;
};

struct ccu_dpll_config {
	struct ccu_dpll_rate_tbl * rate_tbl;
	u32 tbl_size;
};

#define DPLL_RATE(_rate, _reg0, _reg1, _reg2, _reg3, _reg4, _reg5, _reg6, _reg7)		\
	{						\
		.rate	=	(_rate),			\
		.reg0	=	(_reg0),			\
		.reg1	=	(_reg1),			\
		.reg2	=	(_reg2),			\
		.reg3	=	(_reg3),			\
		.reg4	=	(_reg4),			\
		.reg5	=	(_reg5),			\
		.reg6	=	(_reg6),			\
		.reg7	=	(_reg7),			\
	}

struct ccu_dpll {
	struct ccu_dpll_config	dpll;
	struct ccu_common	common;
};

#define _KY_CCU_DPLL_CONFIG(_table, _size)	\
	{						\
		.rate_tbl	= (struct ccu_dpll_rate_tbl *)_table,	\
		.tbl_size	= _size,			\
	}

#define KY_CCU_DPLL(_struct, _name, _table, _size,	\
						 _base_type, _reg_ctrl, _reg_sel, _is_pll,	\
						 _flags)				\
	struct ccu_dpll _struct = {					\
		.dpll	= _KY_CCU_DPLL_CONFIG(_table, _size),	\
		.common = { 					\
			.reg_ctrl		= _reg_ctrl,		\
			.reg_sel		= _reg_sel,		\
			.base_type		= _base_type,		\
			.is_pll			= 0,			\
			.hw.init		= CLK_HW_INIT_NO_PARENT(_name,	\
							&ccu_dpll_ops,	\
							_flags),	\
		}							\
	}

static inline struct ccu_dpll *hw_to_ccu_dpll(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	return container_of(common, struct ccu_dpll, common);
}

extern const struct clk_ops ccu_dpll_ops;

#endif

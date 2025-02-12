// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023, ky Corporation.
 *
 */

#ifndef _CCU_PLL_H_
#define _CCU_PLL_H_

#include <linux/spinlock_types.h>
#include <linux/clk-provider.h>
#include "ccu-ky-x1.h"

struct ccu_pll_rate_tbl {
	unsigned long long rate;
	u32 reg5;
	u32 reg6;
	u32 reg7;
	u32 reg8;
	unsigned int div_int;
	unsigned int div_frac;
};

struct ccu_pll_config {
	struct ccu_pll_rate_tbl * rate_tbl;
	u32 tbl_size;
	void __iomem	*lock_base;
	u32		reg_lock;
	u32 lock_enable_bit;
};

#define PLL_RATE(_rate, _reg5, _reg6, _reg7, _reg8, _div_int, _div_frac)		\
	{						\
		.rate	=	(_rate),		\
		.reg5	=	(_reg5),			\
		.reg6	=	(_reg6),			\
		.reg7	=	(_reg7),			\
		.reg8	=	(_reg8),			\
		.div_int	=	(_div_int),			\
		.div_frac	=	(_div_frac),			\
	}

struct ccu_pll {
	struct ccu_pll_config	pll;
	struct ccu_common	common;
};

#define _KY_CCU_PLL_CONFIG(_table, _size, _reg_lock, _lock_enable_bit)	\
	{						\
		.rate_tbl	= (struct ccu_pll_rate_tbl *)_table,			\
		.tbl_size	= _size,			\
		.reg_lock 	= _reg_lock,        \
		.lock_enable_bit	= _lock_enable_bit,			\
	}

#define KY_CCU_PLL(_struct, _name, _table, _size,	\
						 _base_type, _reg_ctrl, _reg_sel, _reg_xtc,\
						 _reg_lock, _lock_enable_bit, _is_pll,	\
						 _flags)				\
	struct ccu_pll _struct = {					\
		.pll	= _KY_CCU_PLL_CONFIG(_table, _size, _reg_lock, _lock_enable_bit),	\
		.common = { 					\
			.reg_ctrl		= _reg_ctrl, 			\
			.reg_sel		= _reg_sel, 			\
			.reg_xtc		= _reg_xtc, 			\
			.base_type		= _base_type,	   \
			.is_pll 		= _is_pll,       \
			.hw.init	= CLK_HW_INIT_NO_PARENT(_name,	\
								  &ccu_pll_ops, \
								  _flags),	\
		}							\
	}


static inline struct ccu_pll *hw_to_ccu_pll(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	return container_of(common, struct ccu_pll, common);
}

extern const struct clk_ops ccu_pll_ops;

#endif

// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023, ky Corporation.
 *
 */

#ifndef _CCU_DDN_H_
#define _CCU_DDN_H_


#include <linux/spinlock_types.h>
#include <linux/clk-provider.h>

#include "ccu-ky-x1.h"

struct ccu_ddn_tbl {
	unsigned int num;
	unsigned int den;
};

struct ccu_ddn_info {
	unsigned int factor;
	unsigned int num_mask;
	unsigned int den_mask;
	unsigned int num_shift;
	unsigned int den_shift;
};

struct ccu_ddn_config {
	struct ccu_ddn_info * info;
	struct ccu_ddn_tbl * tbl;
	u32 tbl_size;
};

#define PLL_DDN_TBL(_num, _den)		\
	{						\
		.num	=	(_num),		\
		.den	=	(_den),			\
	}

struct ccu_ddn {
	u32 gate;
	struct ccu_ddn_config  ddn;
	struct ccu_common	common;
};

#define _KY_CCU_DDN_CONFIG(_info, _table, _size)	\
	{						\
		.info	= (struct ccu_ddn_info *)_info,			\
		.tbl	= (struct ccu_ddn_tbl *)_table,			\
		.tbl_size	= _size,			\
	}

#define KY_CCU_DDN(_struct, _name, _parent, _info, _table, _size,	\
						 _base_type, _reg_ctrl, 	\
						 _flags)				\
	struct ccu_ddn _struct = {					\
		.ddn	= _KY_CCU_DDN_CONFIG(_info, _table, _size),	\
		.common = { 					\
			.reg_ctrl		= _reg_ctrl, 			\
			.base_type		= _base_type,	   \
			.hw.init	= CLK_HW_INIT(_name,	\
								  _parent, \
								  &ccu_ddn_ops, \
								  _flags),	\
		}							\
	}

#define KY_CCU_DDN_GATE(_struct, _name, _parent, _info, _table, _size,	\
							 _base_type, _reg_ddn, __reg_gate, _gate_mask, \
							 _flags)				\
	struct ccu_ddn _struct = {					\
		.gate	= _gate_mask,	 \
		.ddn	= _KY_CCU_DDN_CONFIG(_info, _table, _size),	\
		.common = { 					\
			.reg_ctrl		= _reg_ddn,			\
			.reg_sel		= __reg_gate,			\
			.base_type		= _base_type,	   \
			.hw.init	= CLK_HW_INIT(_name,	\
								  _parent, \
								  &ccu_ddn_ops, \
								  _flags),	\
		}							\
	}


static inline struct ccu_ddn *hw_to_ccu_ddn(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	return container_of(common, struct ccu_ddn, common);
}

extern const struct clk_ops ccu_ddn_ops;


#endif

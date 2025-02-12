// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023, ky Corporation.
 *
 */

#ifndef _CCU_DDR_H_
#define _CCU_DDR_H_

#include <linux/clk-provider.h>
#include "ccu-ky-x1.h"
#include "ccu_mix.h"

struct ccu_ddr {
	struct ccu_mux_config *mux;
	struct ccu_common common;
};

#define MAX_FREQ_LV			7  //level0~7

#define KY_CCU_DDR_FC(_struct, _name, _parents, _base_type, _reg,	_fc,\
						  _shift, _width, _flags) \
	struct ccu_ddr _struct = {					\
		.mux	= CCU_MUX_INIT(_shift, _width, NULL, 0),	 \
		.common = { 					\
			.reg_sel		= _reg, 			\
			.fc			= _fc,				\
			.base_type		= _base_type,			\
			.name			= _name,			\
			.parent_names		= _parents,			\
			.num_parents		= ARRAY_SIZE(_parents),		\
			.hw.init		= CLK_HW_INIT_PARENTS(_name,	\
							  _parents,		\
							  &ccu_ddr_ops, \
							  _flags|CLK_GET_RATE_NOCACHE),		\
		}							\
	}

static inline struct ccu_ddr *hw_to_ccu_ddr(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	return container_of(common, struct ccu_ddr, common);
}

extern const struct clk_ops ccu_ddr_ops;

#endif /* _CCU_DDR_H_ */

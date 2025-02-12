// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023, ky Corporation.
 *
 */

#ifndef _CCU_KY_X1_H_
#define _CCU_KY_X1_H_

#include <linux/compiler.h>
#include <linux/clk-provider.h>

enum ccu_base_type{
	BASE_TYPE_MPMU       = 0,
	BASE_TYPE_APMU       = 1,
	BASE_TYPE_APBC       = 2,
	BASE_TYPE_APBS       = 3,
	BASE_TYPE_CIU        = 4,
	BASE_TYPE_DCIU       = 5,
	BASE_TYPE_DDRC       = 6,
	BASE_TYPE_AUDC       = 7,
	BASE_TYPE_APBC2      = 8,
	BASE_TYPE_RCPU       = 9,
	BASE_TYPE_RCPU2      = 10,
};

enum {
	CLK_DIV_TYPE_1REG_NOFC_V1 = 0,
	CLK_DIV_TYPE_1REG_FC_V2,
	CLK_DIV_TYPE_2REG_NOFC_V3,
	CLK_DIV_TYPE_2REG_FC_V4,
	CLK_DIV_TYPE_1REG_FC_DIV_V5,
	CLK_DIV_TYPE_1REG_FC_MUX_V6,
};

struct ccu_common {
	void __iomem	*base;
	enum ccu_base_type base_type;
	u32 	reg_type;
	u32 	reg_ctrl;
	u32 	reg_sel;
	u32 	reg_xtc;
	u32 	fc;
	bool	is_pll;
	const char		*name;
	const struct clk_ops	*ops;
	const char		* const *parent_names;
	u8	num_parents;
	unsigned long	flags;
	spinlock_t	*lock;
	struct clk_hw	hw;
};

struct ky_x1_clk {
	void __iomem *mpmu_base;
	void __iomem *apmu_base;
	void __iomem *apbc_base;
	void __iomem *apbs_base;
	void __iomem *ciu_base;
	void __iomem *dciu_base;
	void __iomem *ddrc_base;
	void __iomem *audio_ctrl_base;
	void __iomem *apbc2_base;
	void __iomem *rcpu_base;
	void __iomem *rcpu2_base;
};

struct clk_hw_table {
	char	*name;
	u32 clk_hw_id;
};

extern spinlock_t g_cru_lock;

static inline struct ccu_common *hw_to_ccu_common(struct clk_hw *hw)
{
	return container_of(hw, struct ccu_common, hw);
}

int ky_ccu_probe(struct device_node *node, struct ky_x1_clk *clk_info,
		    struct clk_hw_onecell_data *desc);

#endif /* _CCU_KY_X1_H_ */

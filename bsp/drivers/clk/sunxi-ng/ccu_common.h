/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2016 Maxime Ripard. All rights reserved.
 */

#ifndef _CCU_COMMON_H_
#define _CCU_COMMON_H_

#define SUNXI_MODNAME "ccu-ng"
#include <sunxi-common.h>
#include <linux/compiler.h>
#include <linux/clk-provider.h>

#define CCU_FEATURE_FRACTIONAL		BIT(0)
#define CCU_FEATURE_VARIABLE_PREDIV	BIT(1)
#define CCU_FEATURE_FIXED_PREDIV	BIT(2)
#define CCU_FEATURE_FIXED_POSTDIV	BIT(3)
#define CCU_FEATURE_ALL_PREDIV		BIT(4)
#define CCU_FEATURE_LOCK_REG		BIT(5)
#define CCU_FEATURE_MMC_TIMING_SWITCH	BIT(6)
#define CCU_FEATURE_SIGMA_DELTA_MOD	BIT(7)

/* Support key-field reg setting */
#define CCU_FEATURE_KEY_FIELD_MOD	BIT(8)

/* New formula support in MP: clk = parent / M / P */
#define CCU_FEATURE_MP_NO_INDEX_MODE	BIT(9)

/* Support fixed rate in gate-clk */
#define CCU_FEATURE_FIXED_RATE_GATE	BIT(10)

/* Some clks need config the mux reg repeatedly to fix ic bug */
#define CCU_FEATURE_REPEAT_SET_MUX	BIT(11)

/* Write one bit update the cfg to displl, auto clear */
#define CCU_FEATURE_CLEAR_MOD		BIT(12)

/* Some clks need two gate register */
#define CCU_FEATURE_GATE_DOUBLE_REG	BIT(13)

/* Gate unnecessary clk in int process */
#define CCU_FEATURE_INIT_GATE		BIT(14)

/* Calculate the frequency and save it in the list first */
#define CCU_FEATURE_CLAC_CACHED		BIT(15)

/* Gate config is reverse, 0 is enable, 1 is disable */
#define CCU_FEATURE_GATE_IS_REVERSE	BIT(20)
/*
 * bit16->bit19: used to describe ccu type
 * 1: NKMP
 * 2: MN
 * 3: MP
 * @TODO
 * */
#define CCU_FEATURE_TYPE_MASK		(0xf << 16)
#define CCU_FEATURE_TYPE_NKMP		(1 << 16)
#define CCU_FEATURE_TYPE_MN		(2 << 16)
/* MMC timing mode switch bit */
#define CCU_MMC_NEW_TIMING_MODE		BIT(30)

#define SET_BITS(shift, width, reg, val) \
    (((reg) & ~GENMASK(width + shift - 1, shift)) | (val << (shift)))

struct device_node;

#define CCU_FEATURE_GET_TYPE(feature)	\
	(feature & CCU_FEATURE_TYPE_MASK)

/**
 * struct ccu_reg_dump: register dump of clock controller registers.
 * @offset: clock register offset from the controller base address.
 * @value: the value to be register at offset.
 */
struct ccu_reg_dump {
	u32	offset;
	u32	value;
};

struct ccu_common {
	void __iomem	*base;		/* base addr */
	u16		reg;		/* first reg */
	u16		key_reg;	/* first reg corresponding to the key_reg */
	u16		assoc_reg;	/* second reg */
	u16		lock_reg;	/* lock reg */
	u16		ssc_reg;	/* ssc reg */
	u32		prediv;
	u32		key_value;	/* first reg corresponding to the key_value */
	u32		clear;
	u32		assoc_val;	/* second reg gate bit */
	u32		assoc_key_value;	/* second reg corresponding to the key_value */

	struct clk_sdm_info *sdm_info;
	unsigned long	features;
	spinlock_t	*lock;
	struct clk_hw	hw;
	struct list_head	calc_cache;
};

static inline struct ccu_common *hw_to_ccu_common(struct clk_hw *hw)
{
	return container_of(hw, struct ccu_common, hw);
}

struct sunxi_ccu_desc {
	struct ccu_common		**ccu_clks;
	unsigned long			num_ccu_clks;

	struct clk_hw_onecell_data	*hw_clks;

	struct ccu_reset_map		*resets;
	unsigned long			num_resets;
};

void ccu_helper_wait_for_lock(struct ccu_common *common, u32 lock);
void ccu_helper_wait_for_clear(struct ccu_common *common, u32 clear);

struct ccu_pll_nb {
	struct notifier_block	clk_nb;
	struct ccu_common	*common;

	u32	enable;
	u32	lock;
};

#define to_ccu_pll_nb(_nb) container_of(_nb, struct ccu_pll_nb, clk_nb)

int ccu_pll_notifier_register(struct ccu_pll_nb *pll_nb);

int sunxi_ccu_probe(struct device_node *node, void __iomem *reg,
		    const struct sunxi_ccu_desc *desc);

void sunxi_ccu_sleep_init(void __iomem *reg_base,
			  struct ccu_common **rdump,
			  unsigned long nr_rdump,
			  const struct ccu_reg_dump *rsuspend,
			  unsigned long nr_rsuspend);

void set_reg(char __iomem *addr, u32 val, u8 bw, u8 bs);

void set_reg_key(char __iomem *addr,
		 u32 key, u8 kbw, u8 kbs,
		 u32 val, u8 bw, u8 bs);

unsigned int ccu_get_table_div(const struct clk_div_table *table,
			       unsigned int val);

unsigned int ccu_get_table_val(const struct clk_div_table *table,
			       unsigned int div);

#endif /* _CCU_COMMON_H_ */

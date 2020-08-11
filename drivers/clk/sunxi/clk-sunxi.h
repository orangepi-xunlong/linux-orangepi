/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUNXI_H
#define __MACH_SUNXI_CLK_SUNXI_H

#define to_clk_factor(_hw) container_of(_hw, struct sunxi_clk_factors, hw)

#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	(((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	(((reg) & CLRMASK(width, shift)) | (val << (shift)))

#define __SUNXI_ALL_CLK_IGNORE_UNUSED__  1

struct sunxi_reg_ops {
	u32 (*reg_readl)(void __iomem *reg);
	void (*reg_writel)(u32 val, void __iomem *reg);
};

extern spinlock_t clk_lock;
extern void __iomem *sunxi_clk_base;
extern void __iomem *sunxi_clk_cpus_base;

void __init sunxi_clocks_init(struct device_node *node);
struct factor_init_data *sunxi_clk_get_factor_by_name(const char *name);
struct periph_init_data *sunxi_clk_get_periph_by_name(const char *name);


struct periph_init_data *sunxi_clk_get_periph_cpus_by_name(const char *name);

#endif

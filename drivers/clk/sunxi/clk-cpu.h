/*
 * Copyright (C) 2013 Allwinnertech.
 * Author:huanghuafeng <huafenghuang@allwinnertech.com>
 *
 * base on clk/samsung/clk-cpu.c
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */

#ifndef __MACH_SUNXI_CLK_CPU_H
#define __MACH_SUNXI_CLK_CPU_H
#include "clk-sunxi.h"
#include "clk-periph.h"

struct sunxi_cpuclk {
	struct sunxi_clk_periph *periph;
	struct clk				*alt_parent;
	struct clk				*parent;
	struct clk				*clk;
	struct notifier_block          clk_nb;
};

struct clk *sunxi_clk_register_cpu(struct periph_init_data *pd,
	void __iomem  *base, const char *alt_parent, const char *parent);

#endif /*__SUNXI_CLK_CPU_H*/



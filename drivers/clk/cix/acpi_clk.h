// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef __CIX_APCI_CLK_H__
#define __CIX_APCI_CLK_H__

#include <linux/clkdev.h>

enum {
	ACLK_ID = 0,
	ACLK_CON,
	ACLK_DEV,

	ACLK_MAX,
};

struct acpi_clk {
	struct clk_hw *hw;
	struct clk_lookup cl;

	struct list_head list;
};

struct acpi_clk_hw {
	unsigned int clk_id;
	struct clk_hw hw;
	struct device *dev;

	struct list_head list;
};

#define to_acpi_clk_hw(_hw) container_of(_hw, struct acpi_clk_hw, hw)

#define CLK_NAME_LEN 32

int cix_acpi_parse_clkt(struct device *dev,
		struct clk_hw *(get_hw)(struct device *, int));

#endif

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */

#ifndef __ACPI_SCMI_CLOCK_H_
#define __ACPI_SCMI_CLOCK_H_

#define CLOCK_LIST	"ClockList"
#define CLOCK_ID	"ClockId"
#define MAX_STR_SIZE	16
#define MAX_NUM_RATES	16
struct clock_list
{
	const char *name[MAX_NUM_RATES];
	u32 clk_id[MAX_NUM_RATES];
	union {
		struct {
			int num_rates;
			u64 rates[MAX_NUM_RATES];
		} list;
		struct {
			u64 min_rate;
			u64 max_rate;
			u64 step_size;
		} range;
	};
};

#ifdef CONFIG_CIX_ACPI_CLK
int acpi_clk_set_rate(struct device *dev, u64 rate,
		     u32 clk_id);
u64 acpi_clk_recalc_rate(struct device *dev, u32 clk_id);
int acpi_clk_enable(struct device *dev, u32 clk_id);
int acpi_clk_disable(struct device *dev,u32 clk_id);
#else
static int acpi_clk_enable(struct device *dev, u32 clk_id)
{
	return 0;
}

static int acpi_clk_disable(struct device *dev, u32 clk_id)
{
	return 0;
}

static int acpi_clk_set_rate(struct device *dev, u64 rate,
			    u32 clk_id)
{
	return 0;
}

static u64 acpi_clk_recalc_rate(struct device *dev,
			       u32 clk_id);
{
	return 0;
}
#endif
#endif

// SPDX-License-Identifier: GPL-2.0
/*
 *Copyright 2024 Cix Technology Group Co., Ltd.
 */
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/acpi.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include "acpi_clk.h"

static struct clk_hw *get_clk_hw_from_provider(struct device *dev, int clk_id)
{
	struct clk_hw_onecell_data *clk_data;
	struct clk_hw *hw;

	clk_data = (struct clk_hw_onecell_data *)dev_get_drvdata(dev);

	if (!clk_data || clk_id < 0 || clk_id >= clk_data->num)
		return NULL;

	hw = clk_data->hws[clk_id];
	if (!hw)
		return NULL;

	return hw;
}

static acpi_status acpi_bus_scmi_clk_scan(acpi_handle handle, u32 level,
					void *context, void **ret_p)
{
	struct device *dev = context;
	acpi_object_type acpi_type;
	int ret;

	if (ACPI_FAILURE(acpi_get_type(handle, &acpi_type)))
		return AE_OK;

	if (acpi_type != ACPI_TYPE_DEVICE)
		return AE_OK;

	if (!dev)
		return AE_OK;

	ret = cix_acpi_parse_clkt_handle(handle, "CLKT",
					 get_clk_hw_from_provider, dev);

	if (ret && ret != -ENODEV)
		return AE_ERROR;

	return AE_OK;
}

int cix_acpi_clks_parse(struct device *dev)
{
	acpi_status status;

	status = acpi_walk_namespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT,
				ACPI_UINT32_MAX, acpi_bus_scmi_clk_scan,
				NULL, dev, NULL);
	return status;
}
EXPORT_SYMBOL_GPL(cix_acpi_clks_parse);

// SPDX-License-Identifier: GPL-2.0-only
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/of_clk.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

static const struct of_device_id ky_dma_range_dt_match[] = {
	{ .compatible = "ky-dram-bus", },
	{ },
};

static int ky_dma_range_probe(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver ky_dma_range_driver = {
	.probe = ky_dma_range_probe,
	.driver = {
		.name   = "ky-dma-range",
		.of_match_table = ky_dma_range_dt_match,
	},
};

static int __init ky_dma_range_drv_register(void)
{
	return platform_driver_register(&ky_dma_range_driver);
}

core_initcall(ky_dma_range_drv_register);

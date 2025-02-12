// SPDX-License-Identifier: GPL-2.0-only
/*
 * Memory-mapped interface driver for DW SPI Core
 *
 * Copyright (c) 2023, ky.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/acpi.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include "spi-dw-espi.h"

#define DRIVER_NAME "dw_spi_mmio_ext"

struct dw_spi_mmio_ext {
	struct dw_spi  dws;
	struct clk     *clk;
	struct clk     *pclk;
	void           *priv;
	struct reset_control *rstc;
};

static int dw_spi_hssi_ext_init(struct platform_device *pdev,
			    struct dw_spi_mmio_ext *dwsmmio)
{
	dwsmmio->dws.ip = DW_HSSI_ID;
	dwsmmio->dws.caps = DW_SPI_CAP_EXT_SPI;

	dw_spi_dma_setup_generic(&dwsmmio->dws);

	return 0;
}

static int dw_spi_mmio_ext_probe(struct platform_device *pdev)
{
	int (*init_func)(struct platform_device *pdev,
			 struct dw_spi_mmio_ext *dwsmmio);
	struct dw_spi_mmio_ext *dwsmmio;
    struct resource *mem;
	struct dw_spi *dws;
	int ret;
	int num_cs;

	dwsmmio = devm_kzalloc(&pdev->dev, sizeof(struct dw_spi_mmio_ext),
			GFP_KERNEL);
	if (!dwsmmio)
		return -ENOMEM;

	dws = &dwsmmio->dws;

	/* Get basic io resource and map it */
	dws->regs = devm_platform_get_and_ioremap_resource(pdev, 0, &mem);
	if (IS_ERR(dws->regs))
		return PTR_ERR(dws->regs);

	dws->paddr = mem->start;

	dws->irq = platform_get_irq(pdev, 0);
	if (dws->irq < 0)
		return dws->irq; /* -ENXIO */

	dwsmmio->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(dwsmmio->clk))
		return PTR_ERR(dwsmmio->clk);
	ret = clk_prepare_enable(dwsmmio->clk);
	if (ret)
		return ret;

	/* Optional clock needed to access the registers */
	dwsmmio->pclk = devm_clk_get_optional(&pdev->dev, "pclk");
	if (IS_ERR(dwsmmio->pclk)) {
		ret = PTR_ERR(dwsmmio->pclk);
		goto out_clk;
	}
	ret = clk_prepare_enable(dwsmmio->pclk);
	if (ret)
		goto out_clk;

	/* find an optional reset controller */
	dwsmmio->rstc = devm_reset_control_get_optional_exclusive(&pdev->dev, NULL);
	if (IS_ERR(dwsmmio->rstc)) {
		ret = PTR_ERR(dwsmmio->rstc);
		dev_err(&pdev->dev, "failed to get reset.\n");
		goto out_clk;
	}
	reset_control_deassert(dwsmmio->rstc);

	/* set bus number */
	dws->bus_num = pdev->id;

	/* get supported freq in max */
	dws->max_freq = clk_get_rate(dwsmmio->clk);

	/* get reg width of controler */
	device_property_read_u32(&pdev->dev, "reg-io-width", &dws->reg_io_width);

	/* get chip select count of controler */
	num_cs = 4;
	device_property_read_u32(&pdev->dev, "num-cs", &num_cs);
	dws->num_cs = num_cs;
	init_func = device_get_match_data(&pdev->dev);
	if (init_func) {
		ret = init_func(pdev, dwsmmio);
		if (ret)
			goto out;
	}

	pm_runtime_enable(&pdev->dev);

	ret = dw_spi_ext_add_host(&pdev->dev, dws);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, dwsmmio);
	dev_info(&pdev->dev, "dw_spi_ext_probe success.\n");
	return 0;

out:
	pm_runtime_disable(&pdev->dev);
	clk_disable_unprepare(dwsmmio->pclk);
out_clk:
	clk_disable_unprepare(dwsmmio->clk);
	reset_control_assert(dwsmmio->rstc);

	return ret;
}

static int dw_spi_mmio_ext_remove(struct platform_device *pdev)
{
	struct dw_spi_mmio_ext *dwsmmio = platform_get_drvdata(pdev);

	dw_spi_ext_remove_host(&dwsmmio->dws);
    pm_runtime_disable(&pdev->dev);
	clk_disable_unprepare(dwsmmio->pclk);
	clk_disable_unprepare(dwsmmio->clk);
	reset_control_assert(dwsmmio->rstc);

	return 0;
}

static const struct of_device_id dw_spi_mmio_ext_of_match[] = {
	{ .compatible = "snps,dwc-ssi-1.02a", .data = dw_spi_hssi_ext_init},
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, dw_spi_mmio_ext_of_match);

static struct platform_driver dw_spi_mmio_ext_driver = {
	.probe		= dw_spi_mmio_ext_probe,
	.remove		= dw_spi_mmio_ext_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.of_match_table = dw_spi_mmio_ext_of_match,
	},
};
module_platform_driver(dw_spi_mmio_ext_driver);

MODULE_AUTHOR("George hu");
MODULE_DESCRIPTION("Ky qspi driver");
MODULE_LICENSE("GPL v2");

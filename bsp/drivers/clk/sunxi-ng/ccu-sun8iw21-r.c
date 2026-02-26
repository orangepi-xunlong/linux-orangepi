// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_mult.h"
#include "ccu_nk.h"
#include "ccu_nkm.h"
#include "ccu_nkmp.h"
#include "ccu_nm.h"

#include "ccu-sun8iw21-r.h"

/* ccu_des_start */

static SUNXI_CCU_GATE(r_twd_clk, "r-twd",
		"dcxo24M",
		0x012C, BIT(0), 0);

static SUNXI_CCU_GATE(r_ppu_clk, "r-ppu",
		"dcxo24M",
		0x01AC, BIT(0), 0);

static SUNXI_CCU_GATE(r_rtc_clk, "r-rtc",
		"dcxo24M",
		0x020C, BIT(0), 0);

static SUNXI_CCU_GATE(r_cpucfg_clk, "r-cpucfg",
		"dcxo24M",
		0x022C, BIT(0), 0);

/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun8iw21_r_ccu_resets[] = {
	[RST_BUS_R_PPU]		= { 0x01ac, BIT(16) },
	[RST_BUS_R_RTC]		= { 0x020c, BIT(16) },
	[RST_BUS_R_CPUCFG]	= { 0x022c, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun8iw21_r_hw_clks = {
	.hws    = {
		[CLK_R_TWD]			= &r_twd_clk.common.hw,
		[CLK_R_PPU]			= &r_ppu_clk.common.hw,
		[CLK_R_RTC]			= &r_rtc_clk.common.hw,
		[CLK_R_CPUCFG]			= &r_cpucfg_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun8iw21_r_ccu_clks[] = {
	&r_twd_clk.common,
	&r_ppu_clk.common,
	&r_rtc_clk.common,
	&r_cpucfg_clk.common,
};

static const struct sunxi_ccu_desc sun8iw21_r_ccu_desc = {
	.ccu_clks	= sun8iw21_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun8iw21_r_ccu_clks),

	.hw_clks	= &sun8iw21_r_hw_clks,

	.resets		= sun8iw21_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun8iw21_r_ccu_resets),
};

static int sun8iw21_r_ccu_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device *dev = &pdev->dev;
	void __iomem *reg;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Fail to get IORESOURCE_MEM\n");
		return -EINVAL;
	}

	reg = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(reg)) {
		dev_err(dev, "Fail to map IO resource\n");
		return PTR_ERR(reg);
	}

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun8iw21_r_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun8iw21_r_ccu_clks,
			ARRAY_SIZE(sun8iw21_r_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun8iw21_r_ccu_ids[] = {
	{ .compatible = "allwinner,sun8iw21-r-ccu" },
	{ }
};

static struct platform_driver sun8iw21_r_ccu_driver = {
	.probe	= sun8iw21_r_ccu_probe,
	.driver	= {
		.name	= "sun8iw21-r-ccu",
		.of_match_table	= sun8iw21_r_ccu_ids,
	},
};

static int __init sun8iw21_r_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun8iw21_r_ccu_driver);
	if (err)
		pr_err("register ccu sun8iw21_r failed\n");

	return err;
}

core_initcall(sun8iw21_r_ccu_init);

static void __exit sun8iw21_r_ccu_exit(void)
{
	platform_driver_unregister(&sun8iw21_r_ccu_driver);
}
module_exit(sun8iw21_r_ccu_exit);

MODULE_DESCRIPTION("Allwinner sun8iw21_r clk driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.0.1");

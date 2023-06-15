/*
 * sunxi RTC ccu driver
 *
 * Copyright (c) 2020, Martin <wuyan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/rtc.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

/* set the parent of osc32k-out fixed to osc32k-sys, to simplify the clock tree */
#define OSC32KOUT_PARENT_FIX_TO_OSC32KSYS	1

#define LOSC_CTRL_REG			0x00
#define KEY_FIELD_MAGIC			0x16AA0000
#define LOSC_SRC_SEL			BIT(0)  /* 0: from RC16M; 1: from external OSC (ext-osc32k) */
#define RTC_SRC_SEL			BIT(1)  /* 0: osc32k-sys; 1: dcxo24M-div-32k */
#define LOSC_AUTO_SWT_32K_SEL_EN	BIT(14) /* LOSC auto switch 32k clk source sel enable. 1: enable */
#define LOSC_AUTO_SWT_DISABLE		BIT(15)	/* LOSC auto switch function disable. 1: disable */
#define EXT_LOSC_GSM			(0x2 << 2)  /* 0x8 */

/* sun8iw20 does not have this register */
#define INTOSC_CLK_AUTO_CALI_REG	0x0C
#define RC_CLK_SRC_SEL			BIT(0)  /* 0: Normal RC; 1: Calibrated RC */
#define RC_CALI_EN			BIT(1)  /* 0: disable; 1: enable */

#define LOSC_OUT_GATING_REG		0x60  /* Or: 32K_FOUT_CTRL_GATING_REG */
#define BIT_INDEX_LOSC_OUT_GATING	0     /* bit 0 of LOSC_OUT_GATING_REG. 0: disable output; 1: enable output */
#define BIT_INDEX_LOSC_OUT_SRC_SEL	1     /* bit 1~2 of LOSC_OUT_GATING_REG */
#define BIT_WIDTH_LOSC_OUT_SRC_SEL	2     /* 2 bits are used */
#define LOSC_OUT_SRC_SEL_32K_SYS	0x0   /* 0b00: osc32k-sys */
#define LOSC_OUT_SRC_SEL_32K_EXT	0x1   /* 0b01: ext-osc32k */
#define LOSC_OUT_SRC_SEL_32K_DCXO	0x2   /* 0b10: dcxo24M-div-32k */
#define LOSC_OUT_SRC_SEL_MASK		0x3   /* 0b11: bit mask */
#define HOSC_TO_32K_DIVIDER_ENABLE	BIT(16)  /* 0: disable; 1: enable */

#define XO_CTRL_REG			0x160  /* XO Control register */
#define BIT_INDEX_CLK_REQ_ENB		31     /* bit 31 of XO_CTRL_REG. 0: enable; 1: disable */
#define DCXO_EN				BIT(1) /* 0: disable; 1: enable */

/* sun8iw20 does not have this register */
#define CALI_CTRL_REG			0x164
#define WAKEUP_DCXO_EN			BIT(31)  /* 0: 常校准; 1: 关机校准 */

#define CLK_PARENT_CNT_MAX		3  /* the max count of a clock's parent */

struct sunxi_rtc_ccu;  /* Forward declaration */
typedef int (*clk_reg_fn)(struct sunxi_rtc_ccu *priv, int clk_index);

struct clk_info {
	char *clk_name;
	char *parent_names[CLK_PARENT_CNT_MAX];  /* Must be NULL terminated */
	clk_reg_fn reg_fn;  /* The function to register this clock */
	clk_reg_fn unreg_fn;  /* The function to unregister this clock */
};

struct sunxi_rtc_ccu_hw_data {
	bool support_cali;  /* Does the hardware support RC-16M calibration circuit? */
	bool support_rtc_src_sel;  /* Does the RTC_SRC_SEL bit exist? */
	struct clk_info *clk_info_table; /* Must be NULL terminated */
};

/* Driver's private resource */
struct sunxi_rtc_ccu {
	struct sunxi_rtc_ccu_hw_data *hw_data;
	struct device *dev;
	void __iomem *reg_base;
	struct clk **clks;
	int clks_num;
};

static void config_clock_tree(struct sunxi_rtc_ccu *priv)
{
	__maybe_unused struct device *dev = priv->dev;
	struct sunxi_rtc_ccu_hw_data *hw_data = priv->hw_data;
	void __iomem *reg_base = priv->reg_base;
	void __iomem *reg;
	u32 val;

	/* Let's make it easier by simplify the clock tree: make it a fixed tree */

	/* (1) enable DCXO */
	/* by default, DCXO_EN = 1. We don't have to do this... */
	reg = reg_base + XO_CTRL_REG;
	val = readl(reg);
	val |= DCXO_EN;
	writel(val, reg);

	/* (2) enable calibrated RC-16M, and switch to it */
	if (hw_data->support_cali) {
		reg = reg_base + CALI_CTRL_REG;
		val = readl(reg);
		val &= ~WAKEUP_DCXO_EN;
		writel(val, reg);
		reg = reg_base + INTOSC_CLK_AUTO_CALI_REG;
		val = readl(reg);
		val |= RC_CALI_EN;
		val |= RC_CLK_SRC_SEL;
		writel(val, reg);
	}

	/* (3) enable auto switch function */
	/*
	 * In some cases, we boot with auto switch function disabled, and try to
	 * enable the auto switch function by rebooting.
	 * But the rtc default value does not change unless vcc-rtc is loss.
	 * So we should not rely on the default value of reg.
	 */
	reg = reg_base + LOSC_CTRL_REG;
	val = readl(reg);
	val &= ~LOSC_AUTO_SWT_DISABLE;
	val |= LOSC_AUTO_SWT_32K_SEL_EN;
	val |= KEY_FIELD_MAGIC;
	writel(val, reg);

	/* (4) set the parent of osc32k-sys to ext-osc32k */
	reg = reg_base + LOSC_CTRL_REG;
	val = readl(reg);
	val |= LOSC_SRC_SEL;
	val |= KEY_FIELD_MAGIC;
	writel(val, reg);

	/* (5) set the parent of rtc-32k to osc32k-sys */
	if (hw_data->support_rtc_src_sel) {
		/* by default, RTC_SRC_SEL = 0x0. We don't have to do this... */
		reg = reg_base + LOSC_CTRL_REG;
		val = readl(reg);
		val &= ~RTC_SRC_SEL;
		val |= KEY_FIELD_MAGIC;
		writel(val, reg);
	}

#if OSC32KOUT_PARENT_FIX_TO_OSC32KSYS
	/* (6) set the parent of osc32k-out to osc32k-sys */
	/* by default, LOSC_OUT_SRC_SEL = 0x0. We don't have to do this... */
	reg = reg_base + LOSC_OUT_GATING_REG;
	val = readl(reg);
	val &= ~(LOSC_OUT_SRC_SEL_MASK << BIT_INDEX_LOSC_OUT_SRC_SEL);
	val |= (LOSC_OUT_SRC_SEL_32K_SYS << BIT_INDEX_LOSC_OUT_SRC_SEL);
	writel(val, reg);
#else
	/* (6) enable dcxo24M-div-32k */
	reg = reg_base + LOSC_OUT_GATING_REG;
	val = readl(reg);
	val |= HOSC_TO_32K_DIVIDER_ENABLE;
	writel(val, reg);
#endif

	/* Now we have a fixed clock tree, we can treat as many as possible clocks as fixed clock */
}

/* dcxo24M-out: DCXO 24M output to WiFi */
static int register_dcxo24M_out_as_gate(struct sunxi_rtc_ccu *priv, int clk_index)
{
	struct sunxi_rtc_ccu_hw_data *hw_data = priv->hw_data;
	struct device *dev = priv->dev;
	void __iomem *reg_base = priv->reg_base;
	struct clk_info *clk_info = &(hw_data->clk_info_table[clk_index]);
	const char *clkname;
	char *parent_clkname;
	void __iomem *reg;
	int bit_idx;
	u8 flags;

	clkname = clk_info->clk_name;
	parent_clkname = clk_info->parent_names[0];  /* There should be only one parent for gate clock */
	reg = reg_base + XO_CTRL_REG;
	bit_idx = BIT_INDEX_CLK_REQ_ENB;
	flags = CLK_GATE_SET_TO_DISABLE;
	priv->clks[clk_index] = clk_register_gate(NULL, clkname, parent_clkname, 0, reg, bit_idx, flags, NULL);
	if (IS_ERR(priv->clks[clk_index])) {
		dev_err(dev, "Couldn't register clk '%s'\n", clkname);
		return PTR_ERR(priv->clks[clk_index]);
	}

	return 0;
}
static int unregister_dcxo24M_out_as_gate(struct sunxi_rtc_ccu *priv, int clk_index)
{
	clk_unregister_gate(priv->clks[clk_index]);
	return 0;
}

/* iosc: Internal RC 16M */
static int register_iosc_as_fixed_rate(struct sunxi_rtc_ccu *priv, int clk_index)
{
	struct sunxi_rtc_ccu_hw_data *hw_data = priv->hw_data;
	struct device *dev = priv->dev;
	struct clk_info *clk_info = &(hw_data->clk_info_table[clk_index]);
	const char *clkname;
	char *parent_clkname;
	unsigned long fixed_rate;

	clkname = clk_info->clk_name;
	parent_clkname = clk_info->parent_names[0];  /* There should be only one parent for fixed rate clock */
	fixed_rate = 16000000;
	priv->clks[clk_index] = clk_register_fixed_rate(NULL, clkname, parent_clkname, 0, fixed_rate);
	if (IS_ERR(priv->clks[clk_index])) {
		dev_err(dev, "Couldn't register clk '%s'\n", clkname);
		return PTR_ERR(priv->clks[clk_index]);
	}

	return 0;
}
static int unregister_iosc_as_fixed_rate(struct sunxi_rtc_ccu *priv, int clk_index)
{
	clk_unregister_fixed_rate(priv->clks[clk_index]);
	return 0;
}

/* osc32k (osc32k-sys): internal 32k to the system */
static int register_osc32k_as_fixed_rate(struct sunxi_rtc_ccu *priv, int clk_index)
{
	struct sunxi_rtc_ccu_hw_data *hw_data = priv->hw_data;
	struct device *dev = priv->dev;
	struct clk_info *clk_info = &(hw_data->clk_info_table[clk_index]);
	const char *clkname;
	char *parent_clkname;
	unsigned long fixed_rate;

	clkname = clk_info->clk_name;
	parent_clkname = clk_info->parent_names[0];  /* There should be only one parent for fixed rate clock */
	fixed_rate = 32768;
	priv->clks[clk_index] = clk_register_fixed_rate(NULL, clkname, parent_clkname, 0, fixed_rate);
	if (IS_ERR(priv->clks[clk_index])) {
		dev_err(dev, "Couldn't register clk '%s'\n", clkname);
		return PTR_ERR(priv->clks[clk_index]);
	}

	return 0;
}
static int unregister_osc32k_as_fixed_rate(struct sunxi_rtc_ccu *priv, int clk_index)
{
	clk_unregister_fixed_rate(priv->clks[clk_index]);
	return 0;
}

/* osc32k-out: 32k output on pin */
static int register_osc32k_out_as_gate(struct sunxi_rtc_ccu *priv, int clk_index)
{
	struct sunxi_rtc_ccu_hw_data *hw_data = priv->hw_data;
	struct device *dev = priv->dev;
	void __iomem *reg_base = priv->reg_base;
	struct clk_info *clk_info = &(hw_data->clk_info_table[clk_index]);
	const char *clkname;
	char *parent_clkname;
	void __iomem *reg;
	int bit_idx;
	u8 flags;

	clkname = clk_info->clk_name;
	parent_clkname = clk_info->parent_names[0];  /* There should be only one parent for gate clock */
	reg = reg_base + LOSC_OUT_GATING_REG;
	bit_idx = BIT_INDEX_LOSC_OUT_GATING;
	flags = 0;
	priv->clks[clk_index] = clk_register_gate(NULL, clkname, parent_clkname, 0, reg, bit_idx, flags, NULL);
	if (IS_ERR(priv->clks[clk_index])) {
		dev_err(dev, "Couldn't register clk '%s'\n", clkname);
		return PTR_ERR(priv->clks[clk_index]);
	}

	return 0;
}
static int unregister_osc32k_out_as_gate(struct sunxi_rtc_ccu *priv, int clk_index)
{
	clk_unregister_gate(priv->clks[clk_index]);
	return 0;
}

/* rtc-1k = osc32k / 32 */
static int register_rtc_1k_as_fixed_factor(struct sunxi_rtc_ccu *priv, int clk_index)
{
	struct sunxi_rtc_ccu_hw_data *hw_data = priv->hw_data;
	struct device *dev = priv->dev;
	struct clk_info *clk_info = &(hw_data->clk_info_table[clk_index]);
	const char *clkname;
	char *parent_clkname;
	u8 flags;
	unsigned int mult;
	unsigned int div;

	clkname = clk_info->clk_name;
	parent_clkname = clk_info->parent_names[0];  /* There should be only one parent for fixed factor clock */
	flags = 0;
	mult = 1;
	div = 32;
	priv->clks[clk_index] = clk_register_fixed_factor(NULL, clkname, parent_clkname, flags, mult, div);
	if (IS_ERR(priv->clks[clk_index])) {
		dev_err(dev, "Couldn't register clk '%s'\n", clkname);
		return PTR_ERR(priv->clks[clk_index]);
	}

	return 0;
}
static int unregister_rtc_1k_as_fixed_factor(struct sunxi_rtc_ccu *priv, int clk_index)
{
	clk_unregister_fixed_factor(priv->clks[clk_index]);
	return 0;
}

static int sunxi_rtc_clk_provider_register(struct sunxi_rtc_ccu *priv)
{
	struct sunxi_rtc_ccu_hw_data *hw_data = priv->hw_data;
	struct clk_info *clk_info_table = hw_data->clk_info_table;
	struct device *dev = priv->dev;
	struct clk_hw_onecell_data *clk_data;
	int err;
	int i;

	config_clock_tree(priv);

	for (i = 0; clk_info_table[i].clk_name; i++)  /* get the clks_num */
		priv->clks_num++;
	priv->clks = devm_kzalloc(dev, priv->clks_num * sizeof(priv->clks), GFP_KERNEL);
	if (!priv->clks) {
		dev_err(dev, "Fail to alloc memory for priv->clks\n");
		return -ENOMEM;
	}

	for (i = 0; i < priv->clks_num; i++) {
		dev_info(dev, "Registering clk '%s'\n", clk_info_table[i].clk_name);
		err = clk_info_table[i].reg_fn(priv, i);
		if (err) {
			while (i--)
				clk_info_table[i].unreg_fn(priv, i);
			return err;
		}
	}

	clk_data = devm_kzalloc(dev, struct_size(clk_data, hws, priv->clks_num), GFP_KERNEL);
	if (!clk_data) {
		dev_err(dev, "Fail to alloc memory for clk_data\n");
		return -ENOMEM;
	}

	clk_data->num = priv->clks_num;
	for (i = 0; i < clk_data->num; i++)
		clk_data->hws[i] = __clk_get_hw(priv->clks[i]);
	err = devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get, clk_data);
	if (err) {
		dev_err(dev, "Fail to add clk provider\n");
		return err;
	}

	return 0;
}

static void sunxi_rtc_clk_provider_unregister(struct sunxi_rtc_ccu *priv)
{
}

static struct clk_info sun50iw10_clk_info_table[] = {
	/* Keep it in the same order with the DT-BINDINGS in include/dt-bindings/clock/sun50iw10-rtc.h */
	{
		.clk_name = "dcxo24M-out",
		.parent_names = { "dcxo24M", NULL },
		.reg_fn = register_dcxo24M_out_as_gate,
		.unreg_fn = unregister_dcxo24M_out_as_gate,
	},
	{
		.clk_name = "iosc",
		.parent_names = { NULL },
		.reg_fn = register_iosc_as_fixed_rate,
		.unreg_fn = unregister_iosc_as_fixed_rate,
	},
	{
		.clk_name = "osc32k",
		.parent_names = { NULL },
		.reg_fn = register_osc32k_as_fixed_rate,
		.unreg_fn = unregister_osc32k_as_fixed_rate,
	},
#if OSC32KOUT_PARENT_FIX_TO_OSC32KSYS
	{
		.clk_name = "osc32k-out",
		.parent_names = { "osc32k", NULL },
		.reg_fn = register_osc32k_out_as_gate,
		.unreg_fn = unregister_osc32k_out_as_gate,
	},
#else
	/* @TODO */
#endif
	{
		.clk_name = "rtc-1k",
		.parent_names = { "osc32k", NULL },
		.reg_fn = register_rtc_1k_as_fixed_factor,
		.unreg_fn = unregister_rtc_1k_as_fixed_factor,
	},
	{
		/* sentinel */
	},
};

static struct sunxi_rtc_ccu_hw_data sun50iw10_hw_data = {
	.support_cali = true,
	.support_rtc_src_sel = false,
	.clk_info_table = sun50iw10_clk_info_table,
};

static const struct of_device_id sunxi_rtc_ccu_dt_ids[] = {
	{ .compatible = "allwinner,sun50iw10p1-rtc-ccu", .data = &sun50iw10_hw_data },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sunxi_rtc_ccu_dt_ids);

static int sunxi_rtc_ccu_probe(struct platform_device *pdev)
{
	struct sunxi_rtc_ccu *priv;
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct resource *res;
	int err;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	of_id = of_match_device(sunxi_rtc_ccu_dt_ids, dev);
	if (!of_id) {
		dev_err(dev, "of_match_device() failed\n");
		return -EINVAL;
	}
	priv->hw_data = (struct sunxi_rtc_ccu_hw_data *)(of_id->data);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Fail to get IORESOURCE_MEM\n");
		return -EINVAL;
	}

	/*
	 * Don't use devm_ioremap_resource() here! Or else the RTC driver will
	 * not able to get the same resource later in rtc-sunxi.c.
	 */
	//priv->reg_base = devm_ioremap_resource(dev, res);
	priv->reg_base = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(priv->reg_base)) {
		dev_err(dev, "Fail to map IO resource\n");
		return PTR_ERR(priv->reg_base);
	}

	platform_set_drvdata(pdev, priv);
	priv->dev = dev;

	/* Register clk providers */
	err = sunxi_rtc_clk_provider_register(priv);
	if (err) {
		dev_err(dev, "sunxi_rtc_clk_provider_register() failed\n");
		return err;
	}

	dev_info(dev, "sunxi rtc-ccu probed\n");
	return 0;
}

static int sunxi_rtc_ccu_remove(struct platform_device *pdev)
{
	struct sunxi_rtc_ccu *priv = platform_get_drvdata(pdev);

	sunxi_rtc_clk_provider_unregister(priv);

	return 0;
}

static struct platform_driver sunxi_rtc_ccu = {
	.probe    = sunxi_rtc_ccu_probe,
	.remove   = sunxi_rtc_ccu_remove,
	.driver   = {
		.name  = "sunxi-rtc-ccu",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_rtc_ccu_dt_ids,
	},
};

static int __init sunxi_rtc_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sunxi_rtc_ccu);
	if (err)
		pr_err("Fail to register sunxi_rtc_ccu as platform device\n");

	return err;
}
core_initcall(sunxi_rtc_ccu_init);

static void __exit sunxi_rtc_ccu_exit(void)
{
	platform_driver_unregister(&sunxi_rtc_ccu);
}
module_exit(sunxi_rtc_ccu_exit);

MODULE_DESCRIPTION("sunxi RTC CCU driver");
MODULE_AUTHOR("Martin <wuyan@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.1.0");

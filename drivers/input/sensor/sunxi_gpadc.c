/*
 * drivers/input/sensor/sunxi_gpadc.c
 *
 * Copyright (C) 2016 Allwinner.
 * fuzhaoke <fuzhaoke@allwinnertech.com>
 *
 * SUNXI GPADC Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm.h>
#include "sunxi_gpadc.h"

static struct sunxi_gpadc *p_sunxi_gpadc;


/* clk_in: source clock, round_clk: sample rate */
void sunxi_gpadc_sample_rate_set(void __iomem *reg_base, u32 clk_in, u32 round_clk)
{
	u32 div, reg_val;
	if (round_clk > clk_in)
		pr_err("%s, invalid round clk!\n", __func__);
	div = clk_in / round_clk - 1 ;
	reg_val = readl(reg_base + GP_SR_REG);
	reg_val &= ~GP_SR_CON;
	reg_val |= (div << 16);
	writel(reg_val, reg_base + GP_SR_REG);
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_sample_rate_set);

void sunxi_gpadc_ctrl_set(void __iomem *reg_base, u32 ctrl_para)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val |= ctrl_para;

	writel(reg_val, reg_base + GP_CTRL_REG);
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_ctrl_set);

static u32 __gpadc_ch_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val = 0;


	reg_val = readl(reg_base + GP_CTRL_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}

	writel(reg_val, reg_base + GP_CTRL_REG);

	return 0;
}

u32 sunxi_gpadc_ch_select(struct sunxi_gpadc *p_sunxi_gpadc, enum gp_channel_id id)
{
	int ret = 0;
	if (id >= GP_CH_MAX)
		return -EINVAL;

	spin_lock_irq(&p_sunxi_gpadc->gpadc_lock);

	if (!__gpadc_ch_select(p_sunxi_gpadc->reg_base, id))
		p_sunxi_gpadc->ch_open_cnt[id]++;
	else
		ret = -EINVAL;

	spin_unlock_irq(&p_sunxi_gpadc->gpadc_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_ch_select);

static u32 __gpadc_ch_deselect(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_CTRL_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}

	writel(reg_val, reg_base + GP_CTRL_REG);

	return 0;
}

u32 sunxi_gpadc_ch_deselect(struct sunxi_gpadc *p_sunxi_gpadc, enum gp_channel_id id)
{
	if (id >= GP_CH_MAX)
		return -EINVAL;

	spin_lock_irq(&p_sunxi_gpadc->gpadc_lock);

	if (0 == --(p_sunxi_gpadc->ch_open_cnt[id]))
		__gpadc_ch_deselect(p_sunxi_gpadc->reg_base, id);

	spin_unlock_irq(&p_sunxi_gpadc->gpadc_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_ch_deselect);

u32 sunxi_gpadc_ch_cmp(void __iomem *reg_base, enum gp_channel_id id, u32 low_uv, u32 hig_uv)
{
	u32 reg_val = 0, low = 0, hig = 0, unit = 0;

	if ((low_uv > hig_uv) || (hig_uv > VOL_RANGE)) {
		pr_err("%s, invalid compare value!", __func__);
		return -EINVAL;
	}

	/* anolog voltage range 0~2.3v, 12bits sample rate, unit=2.3v/(2^12)=561uv */
	unit = VOL_RANGE / 4096; /* 12bits sample rate */
	low = low_uv / unit;
	hig = hig_uv / unit;
	reg_val = low + (hig << 16);
	switch (id) {
	case GP_CH_0:
		writel(reg_val, reg_base + GP_CH0_CMP_DATA_REG);
		break;
	case GP_CH_1:
		writel(reg_val, reg_base + GP_CH1_CMP_DATA_REG);
		break;
	case GP_CH_2:
		writel(reg_val, reg_base + GP_CH2_CMP_DATA_REG);
		break;
	case GP_CH_3:
		writel(reg_val, reg_base + GP_CH3_CMP_DATA_REG);
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_ch_cmp);

u32 __gpadc_cmp_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_CTRL_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_CMP_EN;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_CMP_EN;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_CMP_EN;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_CMP_EN;
		break;
	default:
		pr_err("%s, invalid value!", __func__);
		return -EINVAL;
	}

	writel(reg_val, reg_base + GP_CTRL_REG);

	return 0;
}

u32 sunxi_gpadc_cmp_select(struct sunxi_gpadc *p_sunxi_gpadc, enum gp_channel_id id)
{
	int ret = 0;

	if (id >= GP_CH_MAX)
		return -EINVAL;

	spin_lock_irq(&p_sunxi_gpadc->gpadc_lock);

	if (!__gpadc_cmp_select(p_sunxi_gpadc->reg_base, id))
		p_sunxi_gpadc->ch_cmp_cnt[id]++;
	else
		ret = -EINVAL;

	spin_unlock_irq(&p_sunxi_gpadc->gpadc_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_cmp_select);

u32 __gpadc_cmp_deselect(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_CTRL_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_CMP_EN;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_CMP_EN;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_CMP_EN;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_CMP_EN;
		break;
	default:
		pr_err("%s, invalid value!", __func__);
		return -EINVAL;
	}

	writel(reg_val, reg_base + GP_CTRL_REG);

	return 0;
}

u32 sunxi_gpadc_cmp_deselect(struct sunxi_gpadc *p_sunxi_gpadc, enum gp_channel_id id)
{
	if (id >= GP_CH_MAX)
		return -EINVAL;

	spin_lock_irq(&p_sunxi_gpadc->gpadc_lock);

	if (0 == --(p_sunxi_gpadc->ch_cmp_cnt[id]))
		__gpadc_cmp_deselect(p_sunxi_gpadc->reg_base, id);

	spin_unlock_irq(&p_sunxi_gpadc->gpadc_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_cmp_deselect);

u32 sunxi_gpadc_mode_select(void __iomem *reg_base, enum gp_select_mode mode)
{
	u32 reg_val = 0;

	if (mode >= GP_NUM_MAX) {
		pr_err("%s invalid mode!\n", __func__);
		return -EINVAL;
	}
	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val &= ~GP_MODE_SELECT;
	reg_val |= (mode << 8);
	writel(reg_val, reg_base + GP_CTRL_REG);

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_mode_select);

/* enable gpadc function, true:enable, false:disable */
static void __gpadc_en(void __iomem *reg_base, bool onoff)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_CTRL_REG);

	if (true == onoff)
		reg_val |= GP_ADC_EN;
	else
		reg_val &= ~GP_ADC_EN;

	writel(reg_val, reg_base + GP_CTRL_REG);
}

void sunxi_gpadc_en(struct sunxi_gpadc *p_sunxi_gpadc, bool onoff)
{
	spin_lock_irq(&p_sunxi_gpadc->gpadc_lock);

	if (true == onoff) {
		__gpadc_en(p_sunxi_gpadc->reg_base, true);
		p_sunxi_gpadc->gpadc_en_cnt++;
	} else {
		if (0 == --(p_sunxi_gpadc->gpadc_en_cnt))
			__gpadc_en(p_sunxi_gpadc->reg_base, false);
	}

	spin_unlock_irq(&p_sunxi_gpadc->gpadc_lock);
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_en);

u32 sunxi_gpadc_int_cfg(void __iomem *reg_base)
{
	return readl(reg_base + GP_INTC_REG);
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_int_cfg);

void sunxi_gpadc_int_set(void __iomem *reg_base, u32 int_para)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_INTC_REG);
	reg_val |= int_para;

	writel(reg_val, reg_base + GP_INTC_REG);
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_int_set);

void sunxi_gpadc_int_clr(void __iomem *reg_base, u32 int_para)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_INTC_REG);
	reg_val &= ~int_para;

	writel(reg_val, reg_base + GP_INTC_REG);
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_int_clr);

u32 sunxi_gpadc_read_ints(void __iomem *reg_base)
{
	return readl(reg_base + GP_INTS_REG);

}
EXPORT_SYMBOL_GPL(sunxi_gpadc_read_ints);

void sunxi_gpadc_clr_ints(void __iomem *reg_base, u32 int_para)
{
	writel(int_para, reg_base + GP_INTS_REG);
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_clr_ints);

u32 sunxi_gpadc_read_data(void __iomem *reg_base, enum gp_channel_id id)
{
	switch (id) {
	case GP_CH_0:
		return readl(reg_base + GP_CH0_DATA_REG) & GP_CH0_DATA_MASK;
	case GP_CH_1:
		return readl(reg_base + GP_CH1_DATA_REG) & GP_CH1_DATA_MASK;
	case GP_CH_2:
		return readl(reg_base + GP_CH2_DATA_REG) & GP_CH2_DATA_MASK;
	case GP_CH_3:
		return readl(reg_base + GP_CH3_DATA_REG) & GP_CH3_DATA_MASK;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_read_data);

/* only low 12bit valid */
void sunxi_gpadc_set_calibration(void __iomem *reg_base, u16 cali_val)
{
	writel(cali_val & 0xfff, reg_base + GP_CB_DATA_REG);
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_set_calibration);

struct sunxi_gpadc *sunxi_gpadc_get_handler(void)
{
	return p_sunxi_gpadc;
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_get_handler);

int sunxi_gpadc_probe(struct platform_device *pdev)

{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi gpadc is disable\n", __func__);
		return -EPERM;
	}

	p_sunxi_gpadc = kzalloc(sizeof(struct sunxi_gpadc), GFP_KERNEL);
	if (IS_ERR_OR_NULL(p_sunxi_gpadc)) {
		pr_err("not enough memory for sunxi_gpadc\n");
		return -ENOMEM;
	}

	p_sunxi_gpadc->reg_base = of_iomap(np, 0);
	if (NULL == p_sunxi_gpadc->reg_base) {
		pr_err("sunxi_gpadc iomap fail\n");
		ret = -EBUSY;
		goto eiomap;
	}

	p_sunxi_gpadc->irq_num = irq_of_parse_and_map(np, 0);
	if (0 == p_sunxi_gpadc->irq_num) {
		pr_err("sunxi_gpadc fail to map irq\n");
		ret = -EBUSY;
		goto eirq;
	}

	p_sunxi_gpadc->mclk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(p_sunxi_gpadc->mclk)) {
		pr_err("sunxi_gpadc has no clk\n");
		ret = -EINVAL;
		goto eclk;
	} else{
		if (clk_prepare_enable(p_sunxi_gpadc->mclk)) {
			pr_err("enable sunxi_gpadc clock failed!\n");
			ret = -EINVAL;
			goto eclk;
		}
	}

	platform_set_drvdata(pdev, p_sunxi_gpadc);
	spin_lock_init(&p_sunxi_gpadc->gpadc_lock);

	return 0;

eclk:
	free_irq(p_sunxi_gpadc->irq_num, p_sunxi_gpadc);
eirq:
	iounmap(p_sunxi_gpadc->reg_base);
eiomap:
	kfree(p_sunxi_gpadc);

	return ret;
}

int sunxi_gpadc_remove(struct platform_device *pdev)

{
	struct sunxi_gpadc *p_sunxi_gpadc = platform_get_drvdata(pdev);

	sunxi_gpadc_en(p_sunxi_gpadc->reg_base, false);
	free_irq(p_sunxi_gpadc->irq_num, p_sunxi_gpadc);
	clk_disable_unprepare(p_sunxi_gpadc->mclk);
	iounmap(p_sunxi_gpadc->reg_base);
	kfree(p_sunxi_gpadc);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_gpadc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_gpadc *p_sunxi_gpadc = platform_get_drvdata(pdev);

	disable_irq_nosync(p_sunxi_gpadc->irq_num);
	sunxi_gpadc_en(p_sunxi_gpadc->reg_base, false);
	clk_disable_unprepare(p_sunxi_gpadc->mclk);

	return 0;
}

static int sunxi_gpadc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_gpadc *p_sunxi_gpadc = platform_get_drvdata(pdev);

	enable_irq(p_sunxi_gpadc->irq_num);
	sunxi_gpadc_en(p_sunxi_gpadc->reg_base, true);
	clk_prepare_enable(p_sunxi_gpadc->mclk);

	return 0;
}

static const struct dev_pm_ops sunxi_gpadc_dev_pm_ops = {
	.suspend = sunxi_gpadc_suspend,
	.resume = sunxi_gpadc_resume,
};

#define SUNXI_GPADC_DEV_PM_OPS (&sunxi_gpadc_dev_pm_ops)
#else
#define SUNXI_GPADC_DEV_PM_OPS NULL
#endif

static struct of_device_id sunxi_gpadc_of_match[] = {
	{ .compatible = "allwinner,sunxi-gpadc"},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_gpadc_of_match);

static struct platform_driver sunxi_gpadc_driver = {
	.probe  = sunxi_gpadc_probe,
	.remove = sunxi_gpadc_remove,
	.driver = {
		.name   = "sunxi-gpadc",
		.owner  = THIS_MODULE,
		.pm = SUNXI_GPADC_DEV_PM_OPS,
		.of_match_table = of_match_ptr(sunxi_gpadc_of_match),
	},
};
module_platform_driver(sunxi_gpadc_driver);

MODULE_AUTHOR("Fuzhaoke");
MODULE_DESCRIPTION("sunxi-gpadc driver");
MODULE_LICENSE("GPL");

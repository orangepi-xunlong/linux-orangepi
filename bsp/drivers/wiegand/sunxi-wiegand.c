/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2021-2028 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
//#define DEBUG
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/timer.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <asm/irq.h>
#include <linux/input.h>
#include "sunxi-wiegand.h"

struct sunxi_wiegand {
	struct clk *pclk;
	struct clk *wclk;
	struct device *dev;
	struct pinctrl *pctrl;
	void __iomem *base;
	int irq;
	struct input_dev *input_dev;
	struct reset_control *reset;
	struct sunxi_wiegand_config config;
};

static u32 sunxi_wiegand_read(struct sunxi_wiegand *chip, u32 reg)
{
	u32 val = readl(chip->base + reg);
	dev_dbg(chip->dev, "%s(): read reg 0x%x = 0x%x\n", __func__, reg, val);

	return val;
}

static void sunxi_wiegand_write(struct sunxi_wiegand *chip, u32 reg, u32 val)
{
	writel(val, chip->base + reg);
	dev_dbg(chip->dev, "%s(): write reg 0x%x = 0x%x\n", __func__, reg, val);
}

static int wiegand_get_data_and_report(struct sunxi_wiegand *chip)
{
	u32 reg_val;
	u32 reg_check;

	reg_val = sunxi_wiegand_read(chip, WIEGAND_ISR);
	if (reg_val & WIEGAND_ISR_TIME_OUT) {
		dev_dbg(chip->dev, "%s: the irq is from timeout!\n", __func__);
		return 0;
	}

	if (reg_val & WIEGAND_ISR_RECEIVE_DATA)
		dev_dbg(chip->dev, "%s: the irq is from receive data!\n", __func__);

	reg_val = sunxi_wiegand_read(chip, WIEGAND_RXD);
	reg_check = sunxi_wiegand_read(chip, WIEGAND_CBSR);

	if (reg_check == WIEGAND_CBSR_CHECK_OK) {
		dev_dbg(chip->dev, "%s: the odd parity and even parity ok!\n", __func__);
		input_event(chip->input_dev, EV_MSC, MSC_SCAN, reg_val);
		input_sync(chip->input_dev);
	} else if (reg_check == WIEGAND_CBSR_EVEN_FAILED) {
		dev_err(chip->dev, "%s: the even parity failed!\n", __func__);
		return -EPERM;
	} else if (reg_check == WIEGAND_CBSR_ODD_FAILED) {
		dev_err(chip->dev, "%s: the odd parity failed!\n", __func__);
		return -EPERM;
	} else if (reg_check == WIEGAND_CBSR_ODD_EVEN_FAILED) {
		dev_err(chip->dev, "%s: the odd parity and even parity failed\n", __func__);
		return -EPERM;
	}

	return 0;
}

static irqreturn_t wiegand_irq_handler(int irqnum, void *dev_id)
{
	int reg_val;
	int err;
	struct sunxi_wiegand *chip = (struct sunxi_wiegand *)dev_id;

	err = wiegand_get_data_and_report(chip);
	if (err)
		dev_err(chip->dev, "%s: the get data and report failed\n", __func__);

	/* Clear the interrupt flag */
	reg_val = sunxi_wiegand_read(chip, WIEGAND_ISR);
	reg_val |= WIEGAND_ISR_RIS | WIEGAND_ISR_TRIS;
	sunxi_wiegand_write(chip, WIEGAND_ISR, reg_val);

	return IRQ_HANDLED;
}

static int sunxi_wiegand_select_gpio_state(struct sunxi_wiegand *chip, char *name)
{
	int err;
	struct pinctrl_state *pctrl_state = NULL;

	pctrl_state = pinctrl_lookup_state(chip->pctrl, name);
	if (IS_ERR(pctrl_state)) {
		dev_err(chip->dev, "sunxi_wiegand_select_gpio_state look_state(%s) failed\n", name);
		return PTR_ERR(pctrl_state);
	}

	err = pinctrl_select_state(chip->pctrl, pctrl_state);
	if (err) {
		dev_err(chip->dev, "sunxi_wiegand_select_gpio_state select_state(%s) failed\n", name);
		return err;
	}

	return 0;
}

static int sunxi_wiegand_gpio_setup(struct sunxi_wiegand *chip)
{
	chip->pctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR(chip->pctrl)) {
		dev_err(chip->dev, "devm_pinctrl_get() failed\n");
		return PTR_ERR(chip->pctrl);
	}

	return sunxi_wiegand_select_gpio_state(chip, PINCTRL_STATE_DEFAULT);
}

static void sunxi_wiegand_gpio_destroy(struct sunxi_wiegand *chip)
{
	int err;

	err = sunxi_wiegand_select_gpio_state(chip, PINCTRL_STATE_SLEEP);
	if (err)
		dev_err(chip->dev, "%s():fail to select gpio state\n", __func__);
}

static int sunxi_wiegand_irq_setup(struct platform_device *pdev, struct sunxi_wiegand *chip)
{
	int err;
	struct device *dev = &pdev->dev;

	chip->irq = platform_get_irq(pdev, 0);
	if (chip->irq < 0)
		return -EINVAL;

	err = devm_request_irq(dev, chip->irq, wiegand_irq_handler, 0, dev_name(dev), chip);
	if (err) {
		dev_err(dev, "Could not request IRQ\n");
		return err;
	}

	return 0;
}

static void sunxi_wiegand_irq_destroy(struct sunxi_wiegand *chip)
{
}

static int sunxi_wiegand_clk_enable(struct sunxi_wiegand *chip)
{
	int err;

	err = clk_prepare_enable(chip->pclk);
	if (err) {
		dev_err(chip->dev, "Cannot enable clock 'pclk'\n");
		goto err0;
	}

	err = clk_prepare_enable(chip->wclk);
	if (err) {
		dev_err(chip->dev, "Cannot enable clock 'wclk'\n");
		goto err1;
	}

	return 0;

err1:
	clk_disable_unprepare(chip->pclk);
err0:
	return err;
}

static void sunxi_wiegand_clk_disable(struct sunxi_wiegand *chip)
{
	clk_disable_unprepare(chip->pclk);
	clk_disable_unprepare(chip->wclk);
}

static int sunxi_wiegand_clk_setup(struct sunxi_wiegand *chip)
{
	int err;

	chip->pclk = devm_clk_get(chip->dev, "pclk");
	if (IS_ERR(chip->pclk)) {
		dev_err(chip->dev, "Failed to get clock 'pclk'\n");
		return PTR_ERR(chip->pclk);
	}

	chip->wclk = devm_clk_get(chip->dev, "wclk");
	if (IS_ERR(chip->wclk)) {
		dev_err(chip->dev, "Failed to get clock 'wclk'\n");
		return PTR_ERR(chip->wclk);
	}

	err = sunxi_wiegand_clk_enable(chip);
	if (err)
		return err;

	return 0;
}

static void sunxi_wiegand_clk_destroy(struct sunxi_wiegand *chip)
{
	sunxi_wiegand_clk_disable(chip);
}
/* initialization register */
static int sunxi_wiegand_hwinit(struct platform_device *pdev, struct sunxi_wiegand *chip)
{
	int reg_val;
	int err;
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct sunxi_wiegand_config *config = &chip->config;

	err = of_property_read_u32(np, "protocol-type", &config->protocol_type);
	if (err) {
		dev_err(dev, "%s: get the protocol-type failed!\n", __func__);
		return err;
	}
	dev_dbg(dev, "%s: the protocol-type is %d (0:26bit, 1:34bit)\n", __func__, config->protocol_type);
	sunxi_wiegand_write(chip, WIEGAND_WMR, config->protocol_type);

	err = of_property_read_u32(np, "high-parity-polar", &config->high_parity_polar);
	if (err) {
		dev_err(dev, "%s: get high-parity-polar failed!\n", __func__);
		return err;
	}
	reg_val = sunxi_wiegand_read(chip, WIEGAND_CBP);
	reg_val &= ~(BIT(1));
	reg_val |= (config->high_parity_polar << 1);
	sunxi_wiegand_write(chip, WIEGAND_CBP, reg_val);
	dev_dbg(dev, "%s: the high-parity-polar is %d (0:even check 1:odd check)\n", __func__, config->high_parity_polar);

	err = of_property_read_u32(np, "low-parity-polar", &config->low_parity_polar);
	if (err) {
		dev_err(dev, "%s: get low-parity-polar failed!\n", __func__);
		return err;
	}
	reg_val = sunxi_wiegand_read(chip, WIEGAND_CBP);
	reg_val &= ~(BIT(0));
	reg_val |= (config->low_parity_polar << 0);
	sunxi_wiegand_write(chip, WIEGAND_CBP, reg_val);
	dev_dbg(dev, "%s: the low-parity-polar is %d (0:even check 1:odd check) \n", __func__, config->low_parity_polar);

	err = of_property_read_u32(np, "data-polar", &config->data_polar);
	if (err) {
		dev_err(dev, "%s: get data-polar failed!\n", __func__);
		return err;
	}
	reg_val = sunxi_wiegand_read(chip, WIEGAND_DBP);
	reg_val &= ~(BIT(0));
	reg_val |= (config->data_polar << 0);
	sunxi_wiegand_write(chip, WIEGAND_DBP, reg_val);
	dev_dbg(dev, "%s: the data-polar is %d (0:normal, 1:inverse)\n", __func__, config->data_polar);

	err = of_property_read_u32(np, "clock-div", &config->clock_div);
	if (err) {
		dev_err(dev, "%s: get clock-div failed!\n", __func__);
		return err;
	}
	dev_dbg(dev, "%s: the clock-div is %d \n", __func__, config->clock_div);
	if (config->clock_div > 48) {
		config->clock_div = 48;
	} else if ((config->clock_div == 0) || (config->clock_div == 1)) {
		config->clock_div = 2;
	}
	sunxi_wiegand_write(chip, WIEGAND_CLK_DIV, config->clock_div);

	err = of_property_read_u32(np, "signal-duration", &config->signal_duration);
	if (err) {
		dev_err(dev, "%s: get signal-duration failed!\n", __func__);
		return err;
	}
	dev_dbg(dev, "%s: the signal-duration is %d \n", __func__, config->signal_duration);
	sunxi_wiegand_write(chip, WIEGAND_TP, config->signal_duration);

	err = of_property_read_u32(np, "signal-period", &config->signal_period);
	if (err) {
		dev_err(dev, "%s: get signal-period failed!\n", __func__);
		return err;
	}
	dev_dbg(dev, "%s: the signal-period is %d \n", __func__, config->signal_period);
	sunxi_wiegand_write(chip, WIEGAND_TW, config->signal_period);

	sunxi_wiegand_write(chip, WIEGAND_ICR, WIEGAND_ICR_VALUE);
	sunxi_wiegand_write(chip, WIEGAND_FUN_EN, WIEGAND_FUN_EN_VALUE);

	return 0;
}

static int sunxi_wiegand_input_setup(struct sunxi_wiegand *chip)
{
	static struct input_dev *input_dev;
	int err;

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(chip->dev, "input_dev: not enough memory for input device\n");
		return -ENOMEM;
	}

	input_dev->name = "sunxi-wiegand0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x001;
	input_dev->id.product = 0x001;
	input_dev->id.version = 0x0100;

	input_dev->evbit[0] = BIT_MASK(EV_MSC);
	/* all input devices support EV_SYN synchronization events */
	set_bit(EV_MSC, input_dev->evbit);
	set_bit(MSC_SCAN, input_dev->mscbit);

	chip->input_dev = input_dev;
	err = input_register_device(chip->input_dev);
	if (err) {
		dev_err(chip->dev, "wiegand input register failed\n");
		goto err0;
	}

	return 0;

err0:
	input_free_device(chip->input_dev);
	return err;
}

static void sunxi_wiegand_input_destroy(struct sunxi_wiegand *chip)
{
	input_unregister_device(chip->input_dev);
	input_free_device(chip->input_dev);
}

static struct miscdevice wiegand_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "wiegand driver",
};

static int sunxi_wiegand_probe(struct platform_device *pdev)
{
	int err;
	struct sunxi_wiegand *chip = NULL;
	struct device *dev = &pdev->dev;
	struct resource *res;

	dev_dbg(dev, "%s(): BEGIN\n", __func__);

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		dev_err(dev, "fail to alloc memery\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Fail to get IORESOURCE_MEM\n");
		return -EINVAL;
	}

	chip->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(chip->base)) {
		dev_err(dev, "Fail to map IO resource\n");
		return PTR_ERR(chip->base);
	}

	err = misc_register(&wiegand_dev);
	if (err) {
		dev_err(dev, "misc_register() failed!\n");
		goto err0;
	}

	err = sunxi_wiegand_input_setup(chip);
	if (err) {
		dev_err(dev, "sunxi_wiegand_input_setup() failed\n");
		goto err1;
	}

	err = sunxi_wiegand_clk_setup(chip);
	if (err) {
		dev_err(dev, "sunxi_wiegand_clk_setup() failed!\n");
		goto err2;
	}

	err = sunxi_wiegand_gpio_setup(chip);
	if (err) {
		dev_err(dev, "sunxi_wiegand_gpio_setup() failed!\n");
		goto err3;
	}

	err = sunxi_wiegand_hwinit(pdev, chip);
	if (err) {
		dev_err(dev, "sunxi_wiegand_hwinit() failed\n");
		goto err4;
	}

	err = sunxi_wiegand_irq_setup(pdev, chip);
	if (err) {
		dev_err(dev, "sunxi_wiegand_irq_setup() failed!\n");
		goto err4;
	}

	platform_set_drvdata(pdev, chip);

	dev_dbg(dev, "%s():success!\n", __func__);

	return 0;
err4:
	sunxi_wiegand_gpio_destroy(chip);
err3:
	sunxi_wiegand_clk_destroy(chip);
err2:
	sunxi_wiegand_input_destroy(chip);
err1:
	misc_deregister(&wiegand_dev);
err0:

	return err;
}

static int sunxi_wiegand_remove(struct platform_device *pdev)
{
	struct sunxi_wiegand *chip = platform_get_drvdata(pdev);

	sunxi_wiegand_irq_destroy(chip);
	sunxi_wiegand_gpio_destroy(chip);
	sunxi_wiegand_clk_destroy(chip);
	sunxi_wiegand_input_destroy(chip);
	misc_deregister(&wiegand_dev);

	return 0;
}

#ifdef CONFIG_PM

static int sunxi_wiegand_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_wiegand *chip = platform_get_drvdata(pdev);

	dev_dbg(dev, "sunxi_wiegand_suspend\n");

	disable_irq(chip->irq);
	sunxi_wiegand_select_gpio_state(chip, PINCTRL_STATE_SLEEP);
	sunxi_wiegand_clk_disable(chip);

	return 0;
}

static int sunxi_wiegand_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_wiegand *chip = platform_get_drvdata(pdev);

	dev_dbg(dev, "sunxi_wiegand_resume\n");

	sunxi_wiegand_clk_enable(chip);
	sunxi_wiegand_select_gpio_state(chip, PINCTRL_STATE_DEFAULT);
	enable_irq(chip->irq);

	return 0;
}

static const struct dev_pm_ops sunxi_wiegand_pm_ops = {
	.suspend	= sunxi_wiegand_suspend,
	.resume		= sunxi_wiegand_resume,
};
#define SUNXI_WIEGAND_PM_OPS (&sunxi_wiegand_pm_ops)
#else
#define SUNXI_WIEGAND_PM_OPS NULL
#endif

static const struct of_device_id sunxi_wiegand_of_match[] = {
	{ .compatible = "allwinner,sunxi-wiegand", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_wiegand_of_match);

static struct platform_driver sunxi_wiegand_driver = {
	.probe = sunxi_wiegand_probe,
	.remove = sunxi_wiegand_remove,
	.driver = {
		.name	= "sunxi-wiegand",
		.owner	= THIS_MODULE,
		.of_match_table = sunxi_wiegand_of_match,
		.pm	= SUNXI_WIEGAND_PM_OPS,
	},
};

module_platform_driver(sunxi_wiegand_driver);
MODULE_AUTHOR("Lu Ruixiang <luruixiang@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi wiegand driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("v1.0.0");

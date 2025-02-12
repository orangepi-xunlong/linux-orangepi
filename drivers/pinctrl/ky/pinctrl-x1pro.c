// SPDX-License-Identifier: GPL-2.0+
//
// Ky x1-pro pinctrl driver
//
// Copyright (c) 2023, ky Corporation.
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include "pinctrl-ky.h"
#include <dt-bindings/pinctrl/x1-pro-pinctrl.h>

static const struct pinctrl_pin_desc x1pro_pins[] = {
	KY_PINCTRL_PIN(GPIOA0),
	KY_PINCTRL_PIN(GPIOA1),
	KY_PINCTRL_PIN(GPIOA2),
	KY_PINCTRL_PIN(GPIOA3),
	KY_PINCTRL_PIN(GPIOA4),
	KY_PINCTRL_PIN(GPIOA5),
	KY_PINCTRL_PIN(GPIOA6),
	KY_PINCTRL_PIN(GPIOA7),
	KY_PINCTRL_PIN(GPIOA8),
	KY_PINCTRL_PIN(GPIOA9),
	KY_PINCTRL_PIN(GPIOA10),
	KY_PINCTRL_PIN(GPIOA11),
	KY_PINCTRL_PIN(GPIOA12),
	KY_PINCTRL_PIN(GPIOA13),
	KY_PINCTRL_PIN(GPIOA14),
	KY_PINCTRL_PIN(GPIOA15),
	KY_PINCTRL_PIN(GPIOA16),
	KY_PINCTRL_PIN(GPIOA17),
	KY_PINCTRL_PIN(GPIOA18),
	KY_PINCTRL_PIN(GPIOA19),
	KY_PINCTRL_PIN(GPIOA20),
	KY_PINCTRL_PIN(GPIOA21),
	KY_PINCTRL_PIN(GPIOA22),
	KY_PINCTRL_PIN(GPIOA23),
	KY_PINCTRL_PIN(GPIOA24),
	KY_PINCTRL_PIN(GPIOA25),
	KY_PINCTRL_PIN(GPIOA26),
	KY_PINCTRL_PIN(GPIOA27),
	KY_PINCTRL_PIN(GPIOA28),
	KY_PINCTRL_PIN(GPIOA29),

	KY_PINCTRL_PIN(GPIOB0),
	KY_PINCTRL_PIN(GPIOB1),
	KY_PINCTRL_PIN(GPIOB2),
	KY_PINCTRL_PIN(GPIOB3),
	KY_PINCTRL_PIN(GPIOB4),
	KY_PINCTRL_PIN(GPIOB5),
	KY_PINCTRL_PIN(GPIOB6),
	KY_PINCTRL_PIN(GPIOB7),
	KY_PINCTRL_PIN(GPIOB8),
	KY_PINCTRL_PIN(GPIOB9),
	KY_PINCTRL_PIN(GPIOB10),
	KY_PINCTRL_PIN(GPIOB11),
	KY_PINCTRL_PIN(GPIOB12),
	KY_PINCTRL_PIN(GPIOB13),
	KY_PINCTRL_PIN(GPIOB14),
	KY_PINCTRL_PIN(GPIOB15),
	KY_PINCTRL_PIN(GPIOB16),
	KY_PINCTRL_PIN(GPIOB17),
	KY_PINCTRL_PIN(GPIOB18),
	KY_PINCTRL_PIN(GPIOB19),
	KY_PINCTRL_PIN(GPIOB20),
	KY_PINCTRL_PIN(GPIOB21),
	KY_PINCTRL_PIN(GPIOB22),
	KY_PINCTRL_PIN(GPIOB23),
	KY_PINCTRL_PIN(GPIOB24),

	KY_PINCTRL_PIN(GPIOC0),
	KY_PINCTRL_PIN(GPIOC1),
	KY_PINCTRL_PIN(GPIOC2),
	KY_PINCTRL_PIN(GPIOC3),
	KY_PINCTRL_PIN(GPIOC4),
	KY_PINCTRL_PIN(GPIOC5),
	KY_PINCTRL_PIN(GPIOC6),
	KY_PINCTRL_PIN(GPIOC7),
	KY_PINCTRL_PIN(GPIOC8),
	KY_PINCTRL_PIN(GPIOC9),
	KY_PINCTRL_PIN(GPIOC10),
	KY_PINCTRL_PIN(GPIOC11),
	KY_PINCTRL_PIN(GPIOC12),
	KY_PINCTRL_PIN(GPIOC13),
	KY_PINCTRL_PIN(GPIOC14),
	KY_PINCTRL_PIN(GPIOC15),
	KY_PINCTRL_PIN(GPIOC16),
	KY_PINCTRL_PIN(GPIOC17),
	KY_PINCTRL_PIN(GPIOC18),
	KY_PINCTRL_PIN(GPIOC19),
	KY_PINCTRL_PIN(GPIOC20),
	KY_PINCTRL_PIN(GPIOC21),
	KY_PINCTRL_PIN(GPIOC22),
	KY_PINCTRL_PIN(GPIOC23),
	KY_PINCTRL_PIN(GPIOC24),
};

static const struct ky_regs x1pro_regs = {
	.cfg = 0x000,
	.reg_len = 0x80,
};

static const struct ky_pin_conf x1pro_pin_conf = {
	.fs_shift = 0,
	.fs_width = 2,
	.od_shift = 4,
	.pe_shift = 8,
	.pull_shift = 9,
	.ds_shift = 12,
	.st_shift = 16,
	.rte_shift = 20,
};

static struct ky_pinctrl_soc_data x1pro_pinctrl_data = {
	.regs = &x1pro_regs,
	.pinconf = &x1pro_pin_conf,
	.pins = x1pro_pins,
	.npins = ARRAY_SIZE(x1pro_pins),
};

static int x1pro_pinctrl_probe(struct platform_device *pdev)
{
	return ky_pinctrl_probe(pdev, &x1pro_pinctrl_data);
}

static const struct of_device_id x1pro_pinctrl_of_match[] = {
	{ .compatible = "ky,x1pro-pinctrl", },
	{ /* sentinel */ }
};

static struct platform_driver x1pro_pinctrl_driver = {
	.driver = {
		.name = "x1pro-pinctrl",
		.suppress_bind_attrs = true,
		.of_match_table = x1pro_pinctrl_of_match,
	},
	.probe = x1pro_pinctrl_probe,
};

static int __init x1pro_pinctrl_init(void)
{
	return platform_driver_register(&x1pro_pinctrl_driver);
}
postcore_initcall(x1pro_pinctrl_init);
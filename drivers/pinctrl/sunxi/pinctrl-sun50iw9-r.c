// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/*
 * Copyright (c) 2020 frank@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun50iw9_r_pins[] = {
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "s_twi0")),		/* SCK */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "s_twi0")),		/* SDA */
};

static const struct sunxi_pinctrl_desc sun50iw9_r_pinctrl_data = {
	.pins = sun50iw9_r_pins,
	.npins = ARRAY_SIZE(sun50iw9_r_pins),
	.pin_base = SUNXI_PIN_BASE('L'),
	.hw_type = SUNXI_PCTL_HW_TYPE_0,
};

static int sun50iw9_r_pinctrl_probe(struct platform_device *pdev)
{
	return sunxi_pinctrl_init(pdev, &sun50iw9_r_pinctrl_data);
}

static struct of_device_id sun50iw9_r_pinctrl_match[] = {
	{ .compatible = "allwinner,sun50iw9-r-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun50iw9_r_pinctrl_match);

static struct platform_driver sun50iw9_r_pinctrl_driver = {
	.probe	= sun50iw9_r_pinctrl_probe,
	.driver	= {
		.name		= "sun50iw9-r-pinctrl",
		.of_match_table	= sun50iw9_r_pinctrl_match,
	},
};

static int __init sun50iw9_r_pio_init(void)
{
	return platform_driver_register(&sun50iw9_r_pinctrl_driver);
}
postcore_initcall(sun50iw9_r_pio_init);

MODULE_DESCRIPTION("Allwinner sun50iw9 R_PIO pinctrl driver");
MODULE_LICENSE("GPL");

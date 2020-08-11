/*
 * LED Driver for X-Powers AXP813 PMIC.
 *
 * Copyright(c) 2017 Ondrej Jirman <megous@megous.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/mfd/axp20x.h>

#define AXP813_CHGLED_CTRL_MASK		BIT(3)
#define AXP813_CHGLED_CTRL_CHARGER	BIT(3)
#define AXP813_CHGLED_CTRL_USER		0

#define AXP813_CHGLED_USER_STATE_MASK		GENMASK(5, 4)
#define AXP813_CHGLED_USER_STATE_OFFSET		4
#define AXP813_CHGLED_USER_STATE_OFF		0
#define AXP813_CHGLED_USER_STATE_BLINK_SLOW	1
#define AXP813_CHGLED_USER_STATE_BLINK_FAST	2
#define AXP813_CHGLED_USER_STATE_ON		3

struct axp20x_led {
	struct led_classdev cdev;
	struct regmap *regmap;
};

static int axp20x_led_set(struct led_classdev *led_cdev,
			   enum led_brightness value)
{
	struct axp20x_led *led =
			container_of(led_cdev, struct axp20x_led, cdev);
	unsigned int val;

	val = value == LED_OFF ? AXP813_CHGLED_USER_STATE_OFF :
		AXP813_CHGLED_USER_STATE_ON;

	return regmap_update_bits(led->regmap, AXP20X_OFF_CTRL,
				  AXP813_CHGLED_USER_STATE_MASK,
				  val << AXP813_CHGLED_USER_STATE_OFFSET);

}

static int axp20x_led_probe(struct platform_device *pdev)
{
	struct axp20x_dev *axp20x;
	struct axp20x_led *led;
	int ret;

	if (!of_device_is_available(pdev->dev.of_node))
		return -ENODEV;

	axp20x = dev_get_drvdata(pdev->dev.parent);
	if (!axp20x)
		return -EINVAL;

	led = devm_kzalloc(&pdev->dev,
			   sizeof(struct axp20x_led),
			   GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	led->regmap = axp20x->regmap;

	led->cdev.name = "chgled";
	led->cdev.brightness_set_blocking = axp20x_led_set;
	led->cdev.brightness = LED_OFF;
	led->cdev.max_brightness = 1;

	ret = led_classdev_register(pdev->dev.parent, &led->cdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register led %s\n",
			led->cdev.name);
		return ret;
	}

	ret = regmap_update_bits(led->regmap, AXP20X_OFF_CTRL,
				 AXP813_CHGLED_CTRL_MASK,
				 AXP813_CHGLED_CTRL_USER);
	if (ret) {
		dev_err(&pdev->dev, "Failed to enable user cnotrol\n");
	}

	ret = axp20x_led_set(&led->cdev, led->cdev.brightness);
	if (ret) {
		dev_err(&pdev->dev, "Failed to init led %s\n",
			led->cdev.name);
	}

	platform_set_drvdata(pdev, led);
	return 0;
}

static int axp20x_led_remove(struct platform_device *pdev)
{
	struct axp20x_led *led = platform_get_drvdata(pdev);

	axp20x_led_set(&led->cdev, LED_OFF);

	regmap_update_bits(led->regmap, AXP20X_OFF_CTRL,
			   AXP813_CHGLED_CTRL_MASK,
			   AXP813_CHGLED_CTRL_CHARGER);

	led_classdev_unregister(&led->cdev);

	return 0;
}

static const struct of_device_id axp20x_leds_of_match[] = {
	{ .compatible = "x-powers,axp813-leds", },
	{ },
};
MODULE_DEVICE_TABLE(of, axp20x_leds_of_match);

static struct platform_driver axp20x_led_driver = {
	.driver		= {
		.name	= "axp20x-leds",
		.of_match_table = axp20x_leds_of_match,
	},
	.probe		= axp20x_led_probe,
	.remove		= axp20x_led_remove,
};

module_platform_driver(axp20x_led_driver);

MODULE_AUTHOR("Ondrej Jirman <megous@megous.com>");
MODULE_DESCRIPTION("LED driver for AXP813 PMIC");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:leds-axp20x");

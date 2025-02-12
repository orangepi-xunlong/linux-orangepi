// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for simulating a mouse on GPIO lines.
 *
 * Copyright (C) 2007 Atmel Corporation
 * Copyright (C) 2017 Linus Walleij <linus.walleij@linaro.org>
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/pm_wakeirq.h>
#include <linux/gpio/consumer.h>
#include <linux/property.h>
#include <linux/pm_wakeirq.h>
#include <linux/of.h>

struct _hall {
	struct device *dev;
	struct input_dev *input;
	struct gpio_desc *gpio;
	int wakeup_irq, normal_irq;
};

static irqreturn_t hall_wakeup_detect(int irq, void *arg)
{
	unsigned char state = 0;
	struct _hall *hall = (struct _hall *)arg;

	state = gpiod_get_value(hall->gpio);

	pm_wakeup_event(hall->dev, 0);

	input_report_switch(hall->input, SW_LID, !state);
 	input_sync(hall->input);

	return IRQ_HANDLED;
}

static int ky_lid_probe(struct platform_device *pdev)
{
	struct _hall *hall;
	struct input_dev *input;
	int error;

	hall = devm_kzalloc(&pdev->dev, sizeof(*hall), GFP_KERNEL);
	if (!hall)
		return -ENOMEM;

	hall->gpio = devm_gpiod_get(&pdev->dev, "lid", GPIOD_IN);
	if (IS_ERR_OR_NULL(hall->gpio)) {
		return PTR_ERR(hall->gpio);
	}

	hall->normal_irq = platform_get_irq(pdev, 0);
	if (hall->normal_irq < 0)
		return -EINVAL;

	hall->wakeup_irq = platform_get_irq(pdev, 1);
	if (hall->wakeup_irq < 0) {
		return -EINVAL;
	}

	error = devm_request_irq(&pdev->dev, hall->normal_irq,
			hall_wakeup_detect,
			IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
			"hall-detect", (void *)hall);
	if (error) {
		pr_err("request hall pinctrl dectect failed\n");
		return -EINVAL;
	}

	input = devm_input_allocate_device(&pdev->dev);
	if (!input)
		return -ENOMEM;

	input->name = pdev->name;
	input->id.bustype = BUS_HOST;
	input_set_capability(input, EV_SW, SW_LID);

	input_set_drvdata(input, hall);

	hall->dev = &pdev->dev;
	hall->input = input;

	error = input_register_device(input);
	if (error) {
		dev_err(&pdev->dev, "could not register input device\n");
		return error;
	}

	dev_pm_set_dedicated_wake_irq_ky(&pdev->dev, hall->wakeup_irq, IRQ_TYPE_EDGE_RISING);
 	device_init_wakeup(&pdev->dev, true);

	return 0;
}

static const struct of_device_id ky_lid_of_match[] = {
	{ .compatible = "ky,x1-lid", },
	{ },
};
MODULE_DEVICE_TABLE(of, ky_lid_of_match);

static struct platform_driver ky_lid_device_driver = {
	.probe		= ky_lid_probe,
	.driver		= {
		.name	= "ky-lid",
		.of_match_table = ky_lid_of_match,
	}
};
module_platform_driver(ky_lid_device_driver);

MODULE_AUTHOR("Hans-Christian Egtvedt <egtvedt@samfundet.no>");
MODULE_DESCRIPTION("ky lid driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ky-lid"); /* work with hotplug and coldplug */

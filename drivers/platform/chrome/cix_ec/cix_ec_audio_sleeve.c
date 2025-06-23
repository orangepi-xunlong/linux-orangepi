// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Cix Power Key driver
 *
 * Copyright (C) 2024 cix, Inc.
 *
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/platform_data/cros_ec_commands.h>
#include <linux/platform_data/cros_ec_proto.h>

#define SLEEVEKEY_PRESS_SHIFT	14
#define SLEEVEKEY_RELEASE_SHIFT	15

struct cix_ec_sleevekey_device {
	struct cros_ec_device *ec;
	struct input_dev *sleeve;
} *ec_sleeve_dev;

static void sleevekey_press_report(void)
{
	struct input_dev *sleeve = ec_sleeve_dev->sleeve;

	input_report_key(sleeve, KEY_MUTE, 1);
	input_sync(sleeve);
}

static void sleevekey_release_report(void)
{
	struct input_dev *sleeve = ec_sleeve_dev->sleeve;

	input_report_key(sleeve, KEY_MUTE, 0);
	input_sync(sleeve);
}

int cix_ec_sleevekey_handler(struct notifier_block *nb, unsigned long event,
		       void *arg)
{
	struct cros_ec_device *ec_dev;
	uint32_t data;

	ec_dev = (struct cros_ec_device *)arg;
	data = ec_dev->irq_info.data;
	if (!(data & (1 << SLEEVEKEY_PRESS_SHIFT) ||
	      data & (1 << SLEEVEKEY_RELEASE_SHIFT)))
		return NOTIFY_DONE;

	if (data & (1 << SLEEVEKEY_PRESS_SHIFT))
		sleevekey_press_report();
	else if (data & (1 << SLEEVEKEY_RELEASE_SHIFT))
		sleevekey_release_report();

	return NOTIFY_OK;
}

static struct notifier_block cix_sleevekey_notifier = {
	.notifier_call = cix_ec_sleevekey_handler,
};

static const struct of_device_id cix_ec_sleevekey_of_match[] = {
	{ .compatible = "cix,cix-ec-sleevekey", },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ec_pwrkey_of_match);

static int cix_sleevekey_probe(struct platform_device *pdev)
{
	struct cros_ec_device *ec = dev_get_drvdata(pdev->dev.parent);
	struct input_dev *sleeve;
	int err;

	ec_sleeve_dev = kmalloc(sizeof(struct cix_ec_sleevekey_device), GFP_KERNEL);
	if (!ec_sleeve_dev) {
		dev_err(&pdev->dev, "Can't allocate sleevekey device\n");
		return -ENOMEM;
	}
	sleeve = devm_input_allocate_device(&pdev->dev);
	if (!sleeve) {
		dev_err(&pdev->dev, "Can't allocate sleeve button\n");
		return -ENOMEM;
	}

	sleeve->name = "cix sleevekey";
	sleeve->phys = "cix_sleevekey/input0";
	sleeve->id.bustype = BUS_HOST;
	input_set_capability(sleeve, EV_KEY, KEY_MUTE);

	err = input_register_device(sleeve);
	if (err) {
		dev_err(&pdev->dev, "Can't register sleeve button: %d\n", err);
		return err;
	}

	platform_set_drvdata(pdev, sleeve);
	device_init_wakeup(&pdev->dev, false);

	ec_sleeve_dev->sleeve = sleeve;
	ec_sleeve_dev->ec = ec;
	err = blocking_notifier_chain_register(&ec->event_notifier,
					      &cix_sleevekey_notifier);
	if (err)
		return err;

	return 0;
}

static struct platform_driver cix_sleevekey_driver = {
	.probe	= cix_sleevekey_probe,
	.driver	= {
		.name = "cix-sleevekey",
		.of_match_table = cix_ec_sleevekey_of_match,
	},
};
module_platform_driver(cix_sleevekey_driver);

MODULE_ALIAS("platform:cix sleevekey");
MODULE_AUTHOR("Jerry Zhu <jerry.zhu@cixtech.com>");
MODULE_DESCRIPTION("Cix Audio Sleeve Key driver");
MODULE_LICENSE("GPL");

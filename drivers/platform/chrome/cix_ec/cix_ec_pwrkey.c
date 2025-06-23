// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Cix Power Key driver
 *
 * Copyright 2024 Cix Technology Group Co., Ltd..
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

#define PWRKEY_PRESS_SHIFT 6
#define PWRKEY_RELEASE_SHIFT 1

struct cix_ec_pwrkey_device {
	struct cros_ec_device *ec;
	struct input_dev *pwr;
	int stat;
} *ec_pwr_dev;

static void pwrkey_press_report(void)
{
	struct input_dev *pwr = ec_pwr_dev->pwr;

	input_report_key(pwr, KEY_POWER, 1);
	input_sync(pwr);
}

static void pwrkey_release_report(void)
{
	struct input_dev *pwr = ec_pwr_dev->pwr;

	input_report_key(pwr, KEY_POWER, 0);
	input_sync(pwr);
}

static int cix_pwrkey_info_ec(struct cros_ec_device *ec)
{
	int rc;
	struct {
		struct cros_ec_command msg;
		struct ec_pwrkey_info params;
	} __packed buf;
	struct ec_pwrkey_info *params = &buf.params;
	struct cros_ec_command *msg = &buf.msg;

	memset(&buf, 0, sizeof(buf));
	msg->version = 0;
	msg->command = EC_CMD_SET_OS_TYPE;
	msg->insize = 0;
	msg->outsize = sizeof(*params);
	params->os_type = 1;
	rc = cros_ec_cmd_xfer_status(ec, msg);
	if (rc < 0)
		return rc;

	return 0;
}

int cix_ec_pwrkey_handler(struct notifier_block *nb, unsigned long event,
		       void *arg)
{
	struct cros_ec_device *ec_dev;
	uint32_t data;

	ec_dev = (struct cros_ec_device *)arg;
	data = ec_dev->irq_info.data;
	if (!(data & (1 << PWRKEY_PRESS_SHIFT) ||
	      data & (1 << PWRKEY_RELEASE_SHIFT)))
		return NOTIFY_DONE;

	if (data & (1 << PWRKEY_PRESS_SHIFT))
		pwrkey_press_report();
	if (data & (1 << PWRKEY_RELEASE_SHIFT))
		pwrkey_release_report();

	return NOTIFY_OK;
}

static struct notifier_block cix_pwrkey_notifier = {
	.notifier_call = cix_ec_pwrkey_handler,
};

static const struct of_device_id cix_ec_pwrkey_of_match[] = {
	{ .compatible = "cix,cix-ec-pwrkey", },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ec_pwrkey_of_match);

static int cix_pwrkey_probe(struct platform_device *pdev)
{
	struct cros_ec_device *ec = dev_get_drvdata(pdev->dev.parent);
	struct input_dev *pwr;
	int err;

	ec_pwr_dev = kmalloc(sizeof(struct cix_ec_pwrkey_device), GFP_KERNEL);
	if (!ec_pwr_dev) {
		dev_err(&pdev->dev, "Can't allocate powerkey device\n");
		return -ENOMEM;
	}
	pwr = devm_input_allocate_device(&pdev->dev);
	if (!pwr) {
		dev_err(&pdev->dev, "Can't allocate power button\n");
		return -ENOMEM;
	}

	pwr->name = "cix pwrkey";
	pwr->phys = "cix_pwrkey/input0";
	pwr->id.bustype = BUS_HOST;
	input_set_capability(pwr, EV_KEY, KEY_POWER);

	err = input_register_device(pwr);
	if (err) {
		dev_err(&pdev->dev, "Can't register power button: %d\n", err);
		return err;
	}

	platform_set_drvdata(pdev, pwr);
	device_init_wakeup(&pdev->dev, true);

	ec_pwr_dev->pwr = pwr;
	ec_pwr_dev->ec = ec;
	err = blocking_notifier_chain_register(&ec->event_notifier,
					      &cix_pwrkey_notifier);
	if (err)
		return err;

	err = cix_pwrkey_info_ec(ec);
	if (err)
		return err;

	return 0;
}

static struct platform_driver cix_pwrkey_driver = {
	.probe	= cix_pwrkey_probe,
	.driver	= {
		.name = "cix-pwrkey",
		.of_match_table = cix_ec_pwrkey_of_match,
	},
};
module_platform_driver(cix_pwrkey_driver);

MODULE_ALIAS("platform:cix pwrkey");
MODULE_AUTHOR("Gary Yang <garyyang@cixtech.com>");
MODULE_DESCRIPTION("Cix Power Key driver");
MODULE_LICENSE("GPL");

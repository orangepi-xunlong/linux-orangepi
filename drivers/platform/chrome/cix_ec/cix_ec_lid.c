// SPDX-License-Identifier: GPL-2.0
/*
 * lid sensor driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/module.h>
#include <linux/platform_data/cros_ec_commands.h>
#include <linux/platform_data/cros_ec_proto.h>
#include <linux/platform_device.h>
#include <linux/iio/iio.h>
#include <linux/input.h>
#include <linux/of.h>

#define LID_SHIFT 8

enum LID_STATE {
	LID_DISABLE = 0,
	LID_ENABLE = 1
};

struct cix_ec_lid_device {
	struct cros_ec_device *ec;
	int enable;
	uint8_t is_close;
	struct input_dev *lid_input;
};

static struct cix_ec_lid_device *ec_lid_dev;

static const struct iio_chan_spec cix_ec_lid_channels[] = {
	{
		.type = IIO_ANGL,
		.info_mask_separate = BIT(IIO_CHAN_INFO_ENABLE)
	}
};

/*
 * For Cix EC, enable lid meaning EC will report the lid state
 * if disabled, EC will force lid in open state and will not report the lid state 
 */
static int cix_ec_lid_enable(int enable)
{
	int rc;
	struct cros_ec_device *ec = ec_lid_dev->ec;
	struct {
		struct cros_ec_command msg;
		struct ec_lid_open params;
	} __packed buf;
	struct ec_lid_open *params = &buf.params;
	struct cros_ec_command *msg = &buf.msg;

	memset(&buf, 0, sizeof(buf));
	msg->version = 0;
	msg->command = EC_CMD_FORCE_LID_OPEN;
	msg->insize = 1;
	msg->outsize = sizeof(*params);
	params->force_open = !enable;

	rc = cros_ec_cmd_xfer_status(ec, msg);
	if (rc < 0)
		return rc;

	ec_lid_dev->enable = enable;
	return 0;
}

static int cix_ec_lid_write_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan, int val,
				int val2, long mask)
{
	int rc;

	switch (mask) {
	case IIO_CHAN_INFO_ENABLE:
		if (val != LID_ENABLE && val != LID_DISABLE)
			return -EINVAL;
		rc = cix_ec_lid_enable(val);
		return rc;
	default:
		return -EINVAL;
	}
	return 0;
}
static int cix_ec_lid_read_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan, int *val,
			       int *val2, long mask)
{
	struct cix_ec_lid_device *ec_lid = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_ENABLE:
		*val = ec_lid->enable;
		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}
}

static const struct iio_info cix_ec_lid_info = {
	.read_raw = cix_ec_lid_read_raw,
	.write_raw = cix_ec_lid_write_raw
};

static int cix_ec_lid_get_stat(struct cros_ec_device *ec)
{
	int rc;
	struct {
		struct cros_ec_command msg;
		struct ec_response_lid_get_stat resp;
	} __packed buf;
	struct ec_response_lid_get_stat *resp = &buf.resp;
	struct cros_ec_command *msg = &buf.msg;

	memset(&buf, 0, sizeof(buf));
	msg->version = 0;
	msg->command = EC_CMD_GET_LID_STAT;
	msg->insize = sizeof(*resp);
	msg->outsize = 0;
	rc = cros_ec_cmd_xfer_status(ec, msg);
	if (rc < 0)
		return rc;

	ec_lid_dev->is_close = resp->data;
	return 0;
}

int cix_ec_lid_handler(struct notifier_block *nb, unsigned long event,
		       void *arg)
{
	struct cros_ec_device *ec_dev;
	uint32_t data;
	struct input_dev *lid_input;
	int rc;

	ec_dev = (struct cros_ec_device *)arg;
	data = ec_dev->irq_info.data;
	if (!(data & (1 << LID_SHIFT)))
		return NOTIFY_DONE;

	rc = cix_ec_lid_get_stat(ec_dev);
	if (rc)
		return -EINVAL;
	lid_input = ec_lid_dev->lid_input;
	input_report_switch(lid_input, SW_LID, ec_lid_dev->is_close);
	input_sync(lid_input);

	return NOTIFY_OK;
}

static struct notifier_block cix_ec_notifier = {
	.notifier_call = cix_ec_lid_handler,
};

static int cix_ec_lid_probe(struct platform_device *pdev)
{
	struct cros_ec_device *ec = dev_get_drvdata(pdev->dev.parent);
	struct device *dev = &pdev->dev;
	struct iio_dev *indio_dev;
	struct input_dev *lid_input;
	int rc;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*ec_lid_dev));
	if (!indio_dev)
		return -ENOMEM;

	ec_lid_dev = iio_priv(indio_dev);
	platform_set_drvdata(pdev, indio_dev);
	ec_lid_dev->ec = ec;

	/* enable lid as default */
	rc = cix_ec_lid_enable(LID_ENABLE);
	if (rc)
		return rc;

	indio_dev->info = &cix_ec_lid_info;
	indio_dev->channels = cix_ec_lid_channels;
	indio_dev->num_channels = ARRAY_SIZE(cix_ec_lid_channels);
	indio_dev->name = "Cix EC Lid";
	indio_dev->modes = INDIO_DIRECT_MODE;

	rc = devm_iio_device_register(dev, indio_dev);
	if (rc)
		return rc;

	lid_input = devm_input_allocate_device(dev);
	if (!lid_input)
		return -ENOMEM;

	lid_input->name = "Lid Switch";
	lid_input->phys = "button/input0";
	lid_input->id.bustype = BUS_HOST;
	lid_input->id.product = 0x0005;

	input_set_capability(lid_input, EV_SW, SW_LID);
	rc = input_register_device(lid_input);
	if (rc)
		return rc;
	ec_lid_dev->lid_input = lid_input;

	rc = blocking_notifier_chain_register(&ec->event_notifier,
					      &cix_ec_notifier);
	if (rc)
		return rc;

	return 0;
}

static int cix_ec_lid_remove(struct platform_device *pdev)
{
	struct cros_ec_device *ec = dev_get_drvdata(pdev->dev.parent);

	blocking_notifier_chain_unregister(&ec->event_notifier,
					   &cix_ec_notifier);
	input_unregister_device(ec_lid_dev->lid_input);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cix_ec_lid_of_match[] = {
	{ .compatible = "cix,cix-ec-lid" },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ec_lid_of_match);
#endif

static struct platform_driver cix_ec_lid_driver = {
	.probe = cix_ec_lid_probe,
	.remove = cix_ec_lid_remove,
	.driver = {
		.name = "cix-ec-lid",
		.of_match_table = of_match_ptr(cix_ec_lid_of_match),
	},
};
module_platform_driver(cix_ec_lid_driver);

MODULE_ALIAS("platform:cix-ec-lid");
MODULE_DESCRIPTION("CIX EC Lid driver");
MODULE_LICENSE("GPL v2");

// SPDX-License-Identifier: GPL-2.0
/*
 * light sensor driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/module.h>
#include <linux/platform_data/cros_ec_commands.h>
#include <linux/platform_data/cros_ec_proto.h>
#include <linux/platform_device.h>
#include <linux/iio/iio.h>
#include <linux/of.h>

struct cix_ec_light_device {
	struct cros_ec_device *ec;
};

static const struct iio_chan_spec cix_ec_light_channels[] = {
	{
		.type = IIO_LIGHT,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW)
	}
};

static int cix_ec_light_read(struct cix_ec_light_device *ec_light)
{
	int rc;
	struct cros_ec_device *ec = ec_light->ec;
	struct {
		struct cros_ec_command msg;
		struct ec_params_light_read_info resp;
	} __packed buf;
	struct ec_params_light_read_info *resp = &buf.resp;
	struct cros_ec_command *msg = &buf.msg;

	memset(&buf, 0, sizeof(buf));

	msg->version = 0;
	msg->command = EC_CMD_LIGHT_READ_RAW;
	msg->insize = sizeof(*resp);
	msg->outsize = 0;
	rc = cros_ec_cmd_xfer_status(ec, msg);
	if (rc < 0)
		return rc;
	return resp->value;
}

static int cix_ec_light_read_raw(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan, int *val,
				 int *val2, long mask)
{
	struct cix_ec_light_device *ec_light = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = cix_ec_light_read(ec_light);
		if (ret < 0)
			return ret;
		else
			*val = ret;
		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}
}

static const struct iio_info cix_ec_light_info = {
	.read_raw = cix_ec_light_read_raw,
};

static int cix_ec_light_probe(struct platform_device *pdev)
{
	struct cros_ec_device *ec = dev_get_drvdata(pdev->dev.parent);
	struct device *dev = &pdev->dev;
	struct cix_ec_light_device *ec_light;
	struct iio_dev *indio_dev;
	int rc;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*ec_light));
	if (!indio_dev)
		return -ENOMEM;

	ec_light = iio_priv(indio_dev);
	platform_set_drvdata(pdev, indio_dev);
	ec_light->ec = ec;

	indio_dev->info = &cix_ec_light_info;
	indio_dev->channels = cix_ec_light_channels;
	indio_dev->num_channels = ARRAY_SIZE(cix_ec_light_channels);
	indio_dev->name = "cix ec light";
	indio_dev->modes = INDIO_DIRECT_MODE;

	rc = iio_device_register(indio_dev);
	if (rc < 0)
		return rc;

	return 0;
}

static int cix_ec_light_remove(struct platform_device *dev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(dev);
	iio_device_unregister(indio_dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cix_ec_light_of_match[] = {
	{ .compatible = "cix,cix-ec-light" },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ec_light_of_match);
#endif

static struct platform_driver cix_ec_light_driver = {
	.probe = cix_ec_light_probe,
	.remove = cix_ec_light_remove,
	.driver = {
		.name = "cix-ec-light",
		.of_match_table = of_match_ptr(cix_ec_light_of_match),
	},
};
module_platform_driver(cix_ec_light_driver);

MODULE_ALIAS("platform:cix-ec-light");
MODULE_DESCRIPTION("CIX EC light driver");
MODULE_LICENSE("GPL v2");

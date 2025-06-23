// SPDX-License-Identifier: GPL-2.0
/*
 * gpio sensor driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/module.h>
#include <linux/platform_data/cros_ec_commands.h>
#include <linux/platform_data/cros_ec_proto.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>

struct cix_ec_gpio_priv {
	struct cros_ec_device *ec;
	struct gpio_chip gc;
};

static int cix_ec_get_direction(struct gpio_chip *gc, unsigned offset)
{
	return 0;
}

static int cix_ec_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	return 0;
}

static int cix_ec_dir_out(struct gpio_chip *gc, unsigned int gpio, int val)
{
	return 0;
}

static int cix_ec_get(struct gpio_chip *gc, unsigned offset)
{
	struct cix_ec_gpio_priv *priv = gpiochip_get_data(gc);
	struct cros_ec_device *ec = priv->ec;
	int rc = 0;
	int val = 0;
	int i = 0;

	struct {
		struct cros_ec_command msg;
		union {
			struct ec_params_gpio_read params;
			struct ec_response_gpio_read resp;
		};
	} __packed buf;
	struct ec_params_gpio_read *params = &buf.params;
	struct ec_response_gpio_read *resp = &buf.resp;
	struct cros_ec_command *msg = &buf.msg;
	if (offset >= 88)
		offset += 112;
	for (i = 0; i < 2; i++) {
		msg->version = 0;
		msg->command = EC_CMD_INT_READ_GPIO;
		msg->insize = sizeof(*resp);
		msg->outsize = sizeof(*params);
		params->gpio_num = offset;
		rc = cros_ec_cmd_xfer_status(ec, msg);
	}
	if (rc < 0)
		printk("cix_ec_get failed\n");
	val = resp->gpio_val;
	printk("cix_ec_get offset = %u val=%d\n", offset, val);
	return val;
}

static void cix_ec_set(struct gpio_chip *gc, unsigned offset, int value)
{
	struct cix_ec_gpio_priv *priv = gpiochip_get_data(gc);
	struct cros_ec_device *ec = priv->ec;
	int rc = 0;

	struct {
		struct cros_ec_command msg;
		struct ec_params_gpio_write params;
	} __packed buf;
	struct ec_params_gpio_write *params = &buf.params;
	struct cros_ec_command *msg = &buf.msg;
	if (offset >= 88)
		offset += 112;
	printk("cix_ec_set offset = %u val=%u\n", offset, value);
	if (ec == NULL) {
		printk("ec is null\n");
	}
	memset(&buf, 0, sizeof(buf));
	msg->version = 0;
	msg->command = EC_CMD_INT_WRITE_GPIO;
	msg->insize = 1;
	msg->outsize = sizeof(*params);
	params->gpio_num = offset;
	params->gpio_val = value;
	rc = cros_ec_cmd_xfer_status(ec, msg);
	if (rc < 0)
		printk("cix_ec_set failed\n");
}

static int cix_ec_gpio_probe(struct platform_device *pdev)
{
	struct cros_ec_device *ec = dev_get_drvdata(pdev->dev.parent);
	struct cix_ec_gpio_priv *priv;
	struct device *dev = &pdev->dev;

	printk("cix_ec_gpio_probe\n");
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->gc.parent = &pdev->dev;
	priv->gc.direction_input = cix_ec_dir_in;
	priv->gc.direction_output = cix_ec_dir_out;
	priv->gc.get_direction = cix_ec_get_direction;
	priv->gc.get = cix_ec_get;
	priv->gc.set = cix_ec_set;
	priv->gc.ngpio = 88+16;
	priv->gc.owner = THIS_MODULE;
	priv->gc.base = -1;

	priv->ec = ec;

	platform_set_drvdata(pdev, priv);

	return devm_gpiochip_add_data(&pdev->dev, &priv->gc, priv);
}

#ifdef CONFIG_OF
static const struct of_device_id cix_ec_gpio_of_match[] = {
	{ .compatible = "cix,cix-ec-gpio" },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ec_gpio_of_match);
#endif

static struct platform_driver cix_ec_gpio_driver = {
	.probe = cix_ec_gpio_probe,
	.remove = NULL,
	.driver = {
		.name = "cix-ec-gpio",
		.of_match_table = of_match_ptr(cix_ec_gpio_of_match),
	},
};
module_platform_driver(cix_ec_gpio_driver);

MODULE_ALIAS("platform:cix-ec-gpio");
MODULE_DESCRIPTION("CIX EC gpio driver");
MODULE_LICENSE("GPL v2");

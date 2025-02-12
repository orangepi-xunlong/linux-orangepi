// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/regmap.h>

struct ocp2138_dev {
	struct device *dev;
	struct regmap *regmap;
	struct i2c_client *client;
};

static const struct regmap_config ocp2138_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xff,
};

static int ocp2138_probe(struct i2c_client *client)
{
	struct ocp2138_dev *ocp2138;
	unsigned int value;

	dev_info(&client->dev, "%s()\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "Failed check I2C functionality");
		return -ENODEV;
	}

	ocp2138 = devm_kzalloc(&client->dev, sizeof(*ocp2138), GFP_KERNEL);
	if (!ocp2138)
		return -ENOMEM;

	i2c_set_clientdata(client, ocp2138);
	ocp2138->dev = &client->dev;
	ocp2138->client = client;

	ocp2138->regmap = devm_regmap_init_i2c(client, &ocp2138_regmap_config);
	if (IS_ERR(ocp2138->regmap)) {
		dev_err(ocp2138->dev, "regmap i2c init failed\n");
		return PTR_ERR(ocp2138->regmap);
	}

	regmap_write(ocp2138->regmap, 0x00, 0x14);
	regmap_write(ocp2138->regmap, 0x01, 0x14);
	regmap_write(ocp2138->regmap, 0xff, 0x80);

	regmap_read(ocp2138->regmap, 0x00, &value);
	dev_info(ocp2138->dev, "%s() 0x00 %d\n", __func__, value);
	regmap_read(ocp2138->regmap, 0x01, &value);
	dev_info(ocp2138->dev, "%s() 0x01 %d\n", __func__, value);
	regmap_read(ocp2138->regmap, 0xff, &value);
	dev_info(ocp2138->dev, "%s() 0xff %d\n", __func__, value);

	return 0;
}

static void ocp2138_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "%s()\n", __func__);
}

static const struct i2c_device_id ocp2138_ids[] = {
	{ "ocp2138", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, ocp2138_ids);

static const struct of_device_id ocp2138_of_match[] = {
	{ .compatible = "ky,lcd_bias_ocp2138", },
	{ },
};
MODULE_DEVICE_TABLE(of, ocp2138_of_match);

static struct i2c_driver ocp2138_i2c_driver = {
	.driver = {
		.name = "ocp2138",
		.of_match_table = ocp2138_of_match,
	},
	.probe = ocp2138_probe,
	.remove = ocp2138_remove,
	.id_table = ocp2138_ids,
};
module_i2c_driver(ocp2138_i2c_driver);

MODULE_DESCRIPTION("OCP2138 I2C Driver");
MODULE_LICENSE("GPL v2");
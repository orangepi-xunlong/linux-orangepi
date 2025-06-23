// SPDX-License-Identifier: GPL-2.0
/*
 * MMC5633 - MEMSIC 3-axis Magnetic Sensor
 *
 *Copyright 2024 Cix Technology Group Co., Ltd. *
 * IIO driver for MMC5633 (7-bit I2C slave address 0x30).
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/acpi.h>
#include <linux/pm.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define MMC5633_DRV_NAME "mmc5633"
#define MMC5633_REGMAP_NAME  "MMC5633_regmap"

#define MMC5633_REG_TEMPERATURE	0x09
#define MMC5633_REG_STATUS	0x18
#define MMC5633_REG_CTRL0	0x1B
#define MMC5633_REG_ID		0x39

#define PRODUCT_ID 0x10
#define MMC5633_CTRL1_BW_MASK	 0x3

struct mmc5633_data {
	struct i2c_client *client;
	struct mutex mutex;
	struct regmap *regmap;
};

static int mmc5633_init(struct mmc5633_data *data)
{
	unsigned int reg_id;
	unsigned temperature;
	unsigned int temp;
	int ret = 0;
	ret = regmap_read(data->regmap, MMC5633_REG_ID, &reg_id);
	if (ret < 0) {
		dev_err(&data->client->dev, "Error reading product id\n");
		return ret;
	}

	dev_info(&data->client->dev, "mmc5633 product id = 0x%x\n", reg_id);

	ret = regmap_update_bits(data->regmap, MMC5633_REG_CTRL0,
				 MMC5633_CTRL1_BW_MASK, MMC5633_CTRL1_BW_MASK);
	if (ret < 0)
		return ret;

	ret = regmap_read(data->regmap, MMC5633_REG_TEMPERATURE, &temp);
	temperature = (unsigned int)(8 * temp/10) - 75;
	if (ret < 0) {
		dev_err(&data->client->dev, "Error reading temperature\n");
		return ret;
	}

	dev_info(&data->client->dev, "temperature = %d\n", temperature);

	return 0;
}

static const struct regmap_config mmc5633_regmap_config = {
	.name = MMC5633_REGMAP_NAME,
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x7F,
};

static int mmc5633_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	struct mmc5633_data *data;
	struct iio_dev *indio_dev;
	struct regmap *regmap;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	regmap = devm_regmap_init_i2c(client, &mmc5633_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&client->dev, "regmap initialization failed\n");
		return PTR_ERR(regmap);
	}
	data = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	data->client = client;
	data->regmap = regmap;
	ret = mmc5633_init(data);
	return ret;
}


static const struct of_device_id mmc5633_of_match[] = {
	{ .compatible = "memsic,mmc5633", },
	{ }
};
MODULE_DEVICE_TABLE(of, mmc5633_of_match);

static const struct i2c_device_id mmc5633_id[] = {
	{"mmc5633", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, mmc5633_id);

static const struct acpi_device_id mmc5633_acpi_match[] = {
	{ "CIXHA011", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, mmc5633_acpi_match);

static struct i2c_driver mmc5633_driver = {
	.driver = {
		.name = MMC5633_DRV_NAME,
		.of_match_table = mmc5633_of_match,
		.acpi_match_table = ACPI_PTR(mmc5633_acpi_match),
	},
	.probe		= mmc5633_probe,
	.id_table	= mmc5633_id,
};

module_i2c_driver(mmc5633_driver);

MODULE_AUTHOR("Hongliang yang <hongliang.yang@cixtech.com>");
MODULE_DESCRIPTION("MEMSIC mmc5633 magnetic sensor driver");
MODULE_LICENSE("GPL v2");

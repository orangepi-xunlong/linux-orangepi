// SPDX-License-Identifier: GPL-2.0-only
/*
 * MFD core driver for X-Powers' AC200 IC
 *
 * The AC200 is a chip which is co-packaged with Allwinner H6 SoC and
 * includes analog audio codec, analog TV encoder, ethernet PHY, eFuse
 * and RTC.
 *
 * Copyright (c) 2019 Jernej Skrabec <jernej.skrabec@siol.net>
 *
 * Based on AC100 driver with following copyrights:
 * Copyright (2016) Chen-Yu Tsai
 */

#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/mfd/core.h>
#include <linux/mfd/ac200.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/regmap.h>

struct ac200_dev {
	struct clk	*clk;
	/*
	 * Lock is needed for serializing concurrent access to
	 * AC200 registers in order not to mess with register page.
	 */
	struct mutex	lock;
	struct regmap	*regmap;
};

/*
 * Register values can't be cached because registers are divided
 * into multiple pages.
 */
static const struct regmap_config ac200_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 16,
};

int ac200_reg_read(struct ac200_dev *ac200, u16 reg, u16 *value)
{
	unsigned int val;
	int ret;

	mutex_lock(&ac200->lock);
	ret = regmap_write(ac200->regmap, AC200_TWI_REG_ADDR_H, reg >> 8);
	if (ret)
		goto read_reg_out;

	ret = regmap_read(ac200->regmap, reg & 0xff, &val);
	*value = val;

read_reg_out:
	mutex_unlock(&ac200->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ac200_reg_read);

int ac200_reg_write(struct ac200_dev *ac200, u16 reg, u16 value)
{
	int ret;

	mutex_lock(&ac200->lock);
	ret = regmap_write(ac200->regmap, AC200_TWI_REG_ADDR_H, reg >> 8);
	if (ret)
		goto write_reg_out;

	ret = regmap_write(ac200->regmap, reg & 0xff, value);

write_reg_out:
	mutex_unlock(&ac200->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ac200_reg_write);

int ac200_reg_mod(struct ac200_dev *ac200, u16 reg, u16 mask, u16 value)
{
	unsigned int val;
	int ret;

	mutex_lock(&ac200->lock);
	ret = regmap_write(ac200->regmap, AC200_TWI_REG_ADDR_H, reg >> 8);
	if (ret)
		goto mod_reg_out;

	ret = regmap_read(ac200->regmap, reg & 0xff, &val);
	if (ret)
		goto mod_reg_out;

	val &= ~mask;
	val |= value;

	ret = regmap_write(ac200->regmap, reg & 0xff, val);

mod_reg_out:
	mutex_unlock(&ac200->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(ac200_reg_mod);

static struct mfd_cell ac200_cells[] = {
	{
		.name		= "ac200-codec",
		.of_compatible	= "x-powers,ac200-codec",
	}, {
		.name		= "ac200-efuse",
		.of_compatible	= "x-powers,ac200-efuse",
	}, {
		.name		= "ac200-ephy",
		.of_compatible	= "x-powers,ac200-ephy",
	}, {
		.name		= "ac200-rtc",
		.of_compatible	= "x-powers,ac200-rtc",
	}, {
		.name		= "ac200-tve",
		.of_compatible	= "x-powers,ac200-tve",
	},
};

static int ac200_i2c_probe(struct i2c_client *i2c,
			   const struct i2c_device_id *id)
{
	struct device *dev = &i2c->dev;
	struct ac200_dev *ac200;
	int ret;

	ac200 = devm_kzalloc(dev, sizeof(*ac200), GFP_KERNEL);
	if (!ac200)
		return -ENOMEM;

	mutex_init(&ac200->lock);
	i2c_set_clientdata(i2c, ac200);

	ac200->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(ac200->clk)) {
		ret = PTR_ERR(ac200->clk);
		dev_err(dev, "Can't obtain the clock: %d\n", ret);
		return ret;
	}

	ac200->regmap = devm_regmap_init_i2c(i2c, &ac200_regmap_config);
	if (IS_ERR(ac200->regmap)) {
		ret = PTR_ERR(ac200->regmap);
		dev_err(dev, "Regmap init failed: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(ac200->clk);
	if (ret) {
		dev_err(dev, "Can't enable the clock: %d\n", ret);
		return ret;
	}

	ret = ac200_reg_write(ac200, AC200_SYS_CONTROL, 0);
	if (ret) {
		dev_err(dev, "Can't put AC200 in reset: %d\n", ret);
		goto err;
	}

	ret = ac200_reg_write(ac200, AC200_SYS_CONTROL, 1);
	if (ret) {
		dev_err(dev, "Can't put AC200 out of reset: %d\n", ret);
		goto err;
	}

	ret = devm_mfd_add_devices(dev, PLATFORM_DEVID_NONE, ac200_cells,
				   ARRAY_SIZE(ac200_cells), NULL, 0, NULL);
	if (ret) {
		dev_err(dev, "Failed to add MFD devices: %d\n", ret);
		goto err;
	}

	return 0;

err:
	clk_disable_unprepare(ac200->clk);
	return ret;
}

static int ac200_i2c_remove(struct i2c_client *i2c)
{
	struct ac200_dev *ac200 = i2c_get_clientdata(i2c);

	ac200_reg_write(ac200, AC200_SYS_CONTROL, 0);

	clk_disable_unprepare(ac200->clk);

	return 0;
}

static const struct i2c_device_id ac200_ids[] = {
	{ "ac200", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, ac200_ids);

static const struct of_device_id ac200_of_match[] = {
	{ .compatible = "x-powers,ac200" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ac200_of_match);

static struct i2c_driver ac200_i2c_driver = {
	.driver = {
		.name	= "ac200",
		.of_match_table	= of_match_ptr(ac200_of_match),
	},
	.probe	= ac200_i2c_probe,
	.remove = ac200_i2c_remove,
	.id_table = ac200_ids,
};
module_i2c_driver(ac200_i2c_driver);

MODULE_DESCRIPTION("MFD core driver for AC200");
MODULE_AUTHOR("Jernej Skrabec <jernej.skrabec@siol.net>");
MODULE_LICENSE("GPL v2");

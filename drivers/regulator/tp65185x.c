// SPDX-License-Identifier: GPL-2.0+
//
// Regulator device driver for tp65185x (eInk panel regulator)
//
// Copyright (C) 2019 Ondřej Jirman <megous@megous.com>

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/gpio/consumer.h>
#include <linux/hwmon.h>
#include <linux/delay.h>

#define REG_TMST_VALUE    0x00
#define REG_ENABLE        0x01
#define REG_VADJ          0x02
#define REG_VCOM1         0x03
#define REG_VCOM2         0x04
#define REG_INT_EN1       0x05
#define REG_INT_EN2       0x06
#define REG_INT1          0x07
#define REG_INT2          0x08
#define REG_UPSEQ0        0x09
#define REG_UPSEQ1        0x0A
#define REG_DWNSEQ0       0x0B
#define REG_DWNSEQ1       0x0C
#define REG_TMST1         0x0D
#define REG_TMST2         0x0E
#define REG_PG            0x0F
#define REG_REVID         0x10

#define REG_TMST1_READ_THERM	0x80
#define REG_TMST1_CONV_END	0x20

struct tp65185x {
	struct device* dev;
	struct gpio_desc* wakeup_gpio;
	struct gpio_desc* vcom_gpio;
	struct gpio_desc* powerup_gpio;
	struct gpio_desc* powergood_gpio;
	struct regmap *regmap;
	bool is_enabled;
	struct mutex wakeup_mutex;
	int wake_refs;
	int vcom;
};

static const struct regmap_config tp65185x_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x10,
	.cache_type = REGCACHE_NONE,
};

struct voltage_rail {
	const char* name;
	int bit;
	int bit_en;
};

static const struct voltage_rail rails[] = {
	{ "VB",    7, -1 },
	{ "VDDH",  6, 3 },
	{ "VN",    5, -1 },
	{ "VPOS",  4, 2 },
	{ "VEE",   3, 1 },
	{ "VNEG",  1, 0 },
	{ "VCOM",  -1, 4 },
	{ "V3P3",  -1, 5 },
};

static int apply_voltage(struct tp65185x *tp, int vcom)
{
	int ret;

	ret = regmap_write(tp->regmap, REG_VCOM1, vcom & 0xff);
	if (ret)
		return ret;

	return regmap_write(tp->regmap, REG_VCOM2, (vcom >> 8) & 1);
}

static int set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV,
		       unsigned *selector)
{
	struct tp65185x* tp = rdev_get_drvdata(rdev);
	unsigned int vcom = min_uV / 10000;

	if (vcom > 511 || min_uV < 0)
		return -EINVAL;

	tp->vcom = vcom;

	// setup VCOM

	if (!tp->is_enabled)
		return 0;

	return apply_voltage(tp, vcom);
}

static int get_voltage(struct regulator_dev *rdev)
{
	struct tp65185x* tp = rdev_get_drvdata(rdev);
	unsigned int lsb, msb;
	int ret;

	if (tp->is_enabled) {
		ret = regmap_read(rdev->regmap, REG_VCOM1, &lsb);
		if (ret)
			return ret;

		ret = regmap_read(rdev->regmap, REG_VCOM2, &msb);
		if (ret)
			return ret;

		return (lsb | ((msb & 1) << 8)) * 10000;
	}

	return tp->vcom * 10000;
}

#ifdef DEBUG
static int show_power_status(struct tp65185x* tp, const char* state)
{
	unsigned int reg;
	int i, ret;

	ret = regmap_read(tp->regmap, REG_PG, &reg);
	if (ret)
		return ret;

	dev_warn(tp->dev, "%s:\n", state);

	dev_warn(tp->dev, "  voltage rail power good:\n");
	for (i = 0; i < ARRAY_SIZE(rails); i++)
		if (rails[i].bit >= 0)
			dev_warn(tp->dev, "    %s %s\n", rails[i].name,
				 reg & BIT(rails[i].bit) ? "good" : "fail");

	ret = regmap_read(tp->regmap, REG_ENABLE, &reg);
	if (ret)
		return ret;

	dev_warn(tp->dev, "  voltage rail enable status:\n");
	for (i = 0; i < ARRAY_SIZE(rails); i++)
		if (rails[i].bit_en >= 0)
			dev_warn(tp->dev, "    %s %s\n", rails[i].name,
				 reg & BIT(rails[i].bit_en)
					? "enabled" : "disabled");

	return 0;
}
#else
static int show_power_status(struct tp65185x* tp, const char* state)
{
	return 0;
}
#endif

static int show_power_bad(struct tp65185x* tp)
{
	unsigned int reg;
	int i, ret;

	ret = regmap_read(tp->regmap, REG_PG, &reg);
	if (ret)
		return ret;

	dev_warn(tp->dev, "Voltage rail failures:\n");
	for (i = 0; i < ARRAY_SIZE(rails); i++)
		if (rails[i].bit >= 0 && !(reg & BIT(rails[i].bit)))
			dev_warn(tp->dev, "  %s failed\n", rails[i].name);

	return 0;
}

static int wait_for_power_good(struct tp65185x *tp)
{
	int ret, loops = 10;

	// wait for power good
	while (loops-- > 0) {
		ret = gpiod_get_value(tp->powergood_gpio);
		if (ret < 0)
			return ret;

		if (ret)
			return 0;

		msleep(5);
	}

	show_power_bad(tp);
	return -ETIMEDOUT;
}

static void wakeup_regulator(struct tp65185x *tp, int wake)
{
	mutex_lock(&tp->wakeup_mutex);

	if (wake) {
		tp->wake_refs++;
		if (tp->wake_refs > 1)
			goto out_unlock;

		gpiod_set_value(tp->wakeup_gpio, 1);
		usleep_range(3000, 4000);
	} else {
		tp->wake_refs--;
		if (tp->wake_refs > 0)
			goto out_unlock;

		gpiod_set_value(tp->wakeup_gpio, 0);
		usleep_range(100, 200);
	}

out_unlock:
	mutex_unlock(&tp->wakeup_mutex);
}

static int enable_supply(struct regulator_dev *rdev)
{
	struct tp65185x* tp = rdev_get_drvdata(rdev);
	int ret;

	if (tp->is_enabled)
		return 0;

	wakeup_regulator(tp, 1);

	ret = apply_voltage(tp, tp->vcom);
	if (ret) {
		dev_err(tp->dev, "vcom restore failed (%d)\n", ret);
		goto err;
	}

	show_power_status(tp, "pre-powerup");

	// enable the VDD on the panel (V3P3) first
	ret = regmap_write(tp->regmap, REG_ENABLE, 0x20);
	if (ret) {
		dev_err(tp->dev, "vdd enable failed (%d)\n", ret);
		goto err;
	}

	usleep_range(2000, 2200);

	show_power_status(tp, "V3P3 en");

	// powerup by default takes about 20ms
	gpiod_set_value(tp->powerup_gpio, 1);
	usleep_range(22000, 24000);

	ret = wait_for_power_good(tp);
	if (ret)
		goto err;

	show_power_status(tp, "powerup done");

	// enable VCOM last
	gpiod_set_value(tp->vcom_gpio, 1);
	usleep_range(4000, 5000);

	show_power_status(tp, "powerup vcom");

        tp->is_enabled = true;
	return 0;

err:
	gpiod_set_value(tp->vcom_gpio, 0);
	usleep_range(2000, 3000);
	gpiod_set_value(tp->powerup_gpio, 0);
	msleep(100);
	wakeup_regulator(tp, 0);
	return ret;
}

static int disable_supply(struct regulator_dev *rdev)
{
	struct tp65185x* tp = rdev_get_drvdata(rdev);

	if (!tp->is_enabled)
		return 0;

	show_power_status(tp, "pre-poweroff");

	gpiod_set_value(tp->vcom_gpio, 0);
	usleep_range(5000, 6000);

	show_power_status(tp, "vcom down");

	gpiod_set_value(tp->powerup_gpio, 0);

	// it may take up to 100ms to power off all high voltage rails
	msleep(100);

	show_power_status(tp, "power down");

	// this will power down the V3P3 switch too
	wakeup_regulator(tp, 0);

	tp->is_enabled = false;

	return 0;
}

static int is_supply_enabled(struct regulator_dev *rdev)
{
	struct tp65185x* tp = rdev_get_drvdata(rdev);
	//int ret;

	//ret = gpiod_get_value(tp->powergood_gpio);
	//if (ret < 0)
		//return ret;

	return tp->is_enabled;
}

static const struct regulator_ops tp65185x_ops = {
	.is_enabled		= is_supply_enabled,
	.enable			= enable_supply,
	.disable		= disable_supply,
	.set_voltage		= set_voltage,
	.get_voltage		= get_voltage,
};

static const struct regulator_desc tp65185x_reg = {
	.name = "tp65185x",
	.id = 0,
	.continuous_voltage_range = 1,
	.ops = &tp65185x_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
};

static int tp65185x_ntc_read_temperature(struct tp65185x* tp, long *val)
{
	int ret;
	unsigned int reg;

	wakeup_regulator(tp, 1);

	ret = regmap_update_bits(tp->regmap, REG_TMST1,
				 REG_TMST1_READ_THERM,
				 REG_TMST1_READ_THERM);
	if (ret)
		goto err_sleep;

	ret = regmap_read_poll_timeout(tp->regmap, REG_TMST1, reg,
				       reg & REG_TMST1_CONV_END,
				       2000, 100000);
	if (ret)
		goto err_sleep;

	ret = regmap_read(tp->regmap, REG_TMST_VALUE, &reg);
	if (ret)
		goto err_sleep;

	*val = (s8)(u8)reg;

err_sleep:
	wakeup_regulator(tp, 0);
	return ret;
}

static int tp65185x_ntc_read(struct device *dev, enum hwmon_sensor_types type,
			     u32 attr, int channel, long *val)
{
	struct tp65185x *tp = dev_get_drvdata(dev);

	if (type == hwmon_temp && attr == hwmon_temp_input) {
		return tp65185x_ntc_read_temperature(tp, val);
	} else if (type == hwmon_temp && attr == hwmon_temp_type) {
		*val = 4;
		return 0;
	}

	return -EINVAL;
}

static umode_t tp65185x_ntc_is_visible(const void *data,
				       enum hwmon_sensor_types type,
				       u32 attr, int channel)
{
	if (type == hwmon_temp) {
		switch (attr) {
		case hwmon_temp_input:
		case hwmon_temp_type:
			return 0444;
		default:
			break;
		}
	}

	return 0;
}

static const struct hwmon_channel_info *tp65185x_ntc_info[] = {
	HWMON_CHANNEL_INFO(chip, HWMON_C_REGISTER_TZ),
	HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT | HWMON_T_TYPE),
	NULL
};

static const struct hwmon_ops tp65185x_ntc_hwmon_ops = {
	.is_visible = tp65185x_ntc_is_visible,
	.read = tp65185x_ntc_read,
};

static const struct hwmon_chip_info tp65185x_ntc_chip_info = {
	.ops = &tp65185x_ntc_hwmon_ops,
	.info = tp65185x_ntc_info,
};

static int tp65185x_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct device *dev = &i2c->dev;
	struct regulator_dev *rdev;
	struct regulator_config config = { };
	unsigned int reg;
	struct tp65185x* tp;
	const char* rev = NULL;
	struct device *hwmon_dev;
	int ret;

	tp = devm_kzalloc(dev, sizeof(*tp), GFP_KERNEL);
	if (!tp)
		return -ENOMEM;

	tp->dev = dev;
	mutex_init(&tp->wakeup_mutex);

	tp->powergood_gpio = devm_gpiod_get(dev, "powergood", GPIOD_IN);
	if (IS_ERR(tp->powergood_gpio)) {
		ret = PTR_ERR(tp->powergood_gpio);
		dev_err(dev, "Can't get wakeup gpio (%d)\n", ret);
		return ret;
	}

	tp->powerup_gpio = devm_gpiod_get(dev, "powerup", GPIOD_OUT_LOW);
	if (IS_ERR(tp->powerup_gpio)) {
		ret = PTR_ERR(tp->powerup_gpio);
		dev_err(dev, "Can't get wakeup gpio (%d)\n", ret);
		return ret;
	}

	tp->vcom_gpio = devm_gpiod_get(dev, "vcom", GPIOD_OUT_LOW);
	if (IS_ERR(tp->vcom_gpio)) {
		ret = PTR_ERR(tp->vcom_gpio);
		dev_err(dev, "Can't get wakeup gpio (%d)\n", ret);
		return ret;
	}

	tp->wakeup_gpio = devm_gpiod_get(dev, "wakeup", GPIOD_OUT_HIGH);
	if (IS_ERR(tp->wakeup_gpio)) {
		ret = PTR_ERR(tp->wakeup_gpio);
		dev_err(dev, "Can't get wakeup gpio (%d)\n", ret);
		return ret;
	}

	// wait for wakeup
	usleep_range(10000, 12000);

	i2c_set_clientdata(i2c, tp);

	tp->regmap = devm_regmap_init_i2c(i2c, &tp65185x_regmap_config);
	if (IS_ERR(tp->regmap)) {
		ret = PTR_ERR(tp->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n", ret);
		return ret;
	}

	ret = regmap_read(tp->regmap, REG_REVID, &reg);
	if (ret) {
		dev_err(dev, "chip id read failed (%d)\n", ret);
		return ret;
	}

	switch (reg) {
		case 0x45: rev = "TPS65185 1p0"; break;
		case 0x55: rev = "TPS65185 1p1"; break;
		case 0x65: rev = "TPS65185 1p2"; break;
		case 0x66: rev = "TPS651851 1p0"; break;
		default:
			dev_err(dev, "reading chip id\n");
			break;

	}

	dev_info(dev, "detected chip id 0x%02x (%s)\n", (int)reg, rev);

	// disable regulators, move to sleep
	gpiod_set_value(tp->wakeup_gpio, 0);

	config.driver_data = tp;
	config.dev = &i2c->dev;
	config.regmap = tp->regmap;
	config.of_node = dev->of_node;
	config.init_data = of_get_regulator_init_data(dev, dev->of_node,
						      &tp65185x_reg);
	if (!config.init_data)
		return -ENOMEM;

	rdev = devm_regulator_register(&i2c->dev, &tp65185x_reg, &config);
	if (IS_ERR(rdev)) {
		ret = PTR_ERR(rdev);
		dev_err(&i2c->dev, "Failed to register egulator (%d)\n", ret);
		return ret;
	}

	hwmon_dev = devm_hwmon_device_register_with_info(&i2c->dev,
							 "tps65185",
							 tp,
							 &tp65185x_ntc_chip_info,
							 NULL);
        if (IS_ERR(hwmon_dev)) {
		ret = PTR_ERR(hwmon_dev);
                dev_err(dev, "unable to register tmst as hwmon device (%d)\n",
			ret);
                return ret;
        }

	return 0;
}

static const struct i2c_device_id tp65185x_i2c_id[] = {
	{ "tp65185x", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, tp65185x_i2c_id);

static struct i2c_driver tp65185x_regulator_driver = {
	.driver = {
		.name = "tp65185x",
	},
	.probe = tp65185x_i2c_probe,
	.id_table = tp65185x_i2c_id,
};

module_i2c_driver(tp65185x_regulator_driver);

MODULE_AUTHOR("Ondřej Jirman <megous@megous.com>");
MODULE_DESCRIPTION("Regulator device driver for tp65185x");
MODULE_LICENSE("GPL");

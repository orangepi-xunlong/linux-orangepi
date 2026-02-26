// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2013 Allwinner.
 * Charles <yanjianbo@allwinnertech.com>
 *
 * sunxi direct gpio driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 *
 * 2022.2.22 shaosidi <shaosidi@allwinnertech.com>
 *	Code optimized and refactored
 */

/* #define DEBUG */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/pinconf-generic.h>

#if IS_ENABLED(CONFIG_PM)
#include <linux/pm.h>
#endif

#define PIN_CFG_NUM		2
#define PIN_PULL_NUM		3
#define PIN_DRV			4

/*
 * @gpio:	gpio global index, must be unique
 * @cfg:	cfg val: 0 - input, 1 - output.
 * @pull:	pull val: 0 - pull up/down disable, 1 - pull up
 * @drv:	drv val: 0 - level 0, 1 - level 1
 * @active:	active_level val: 0 - low, 1 - high, the active level of led light
 */
struct sunxi_dirgpio_pin {
	char *name;
	unsigned int gpio;
	unsigned int cfg;
	unsigned int pull;
	unsigned int drv;
	unsigned int active_level;
	struct device *dev;
};

struct sunxi_dirgpio {
	struct sunxi_dirgpio_pin *pins;
	struct device_node *node;
	struct class *gpio_class;
	struct platform_device *pdev;
	struct device *dev;
	bool gpio_sdcard_reused;
	int gpio_num;
	char **gpio_names;
};

static int sunxi_dirgpio_cfg_set(struct sunxi_dirgpio_pin *pin, int mul_cfg)
{
	int err;

	if (mul_cfg)
		err = gpio_direction_output(pin->gpio, 0);
	else
		err = gpio_direction_input(pin->gpio);

	pin->cfg = mul_cfg;
	return err;
}

static int sunxi_dirgpio_pull_set(struct sunxi_dirgpio_pin *pin, int pull)
{
	unsigned long tmp, mask;
	int ret;

	switch (pull) {
	case 0:
		mask = PIN_CONFIG_BIAS_DISABLE;
		break;
	case 1:
		mask = PIN_CONFIG_BIAS_PULL_UP;
		break;
	case 2:
		mask = PIN_CONFIG_BIAS_PULL_DOWN;
		break;
	default:
		return -EINVAL;
	}

	tmp = PIN_CONF_PACKED(mask, pull);
	ret = pinctrl_gpio_set_config(pin->gpio, tmp);
	pin->pull = pull;

	return ret;
}

static int sunxi_dirgpio_drv_set(struct sunxi_dirgpio_pin *pin, int drv)
{
	unsigned long tmp;
	int ret;

	tmp = PIN_CONF_PACKED(PIN_CONFIG_DRIVE_STRENGTH, drv);
	ret = pinctrl_gpio_set_config(pin->gpio, tmp);
	pin->drv = drv;

	return ret;
}

static void sunxi_dirgpio_data_set(struct sunxi_dirgpio_pin *pin, int data)
{
	__gpio_set_value(pin->gpio, data);
}

static int sunxi_dirgpio_data_get(struct sunxi_dirgpio_pin *pin)
{
	return __gpio_get_value(pin->gpio);
}

static ssize_t sunxi_dirgpio_cfg_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", pin->cfg);
}

static ssize_t sunxi_dirgpio_cfg_store(struct device *dev,
			     struct device_attribute *attr, const char *buf,
			     size_t size)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);
	unsigned int user_data;
	int err;

	if (kstrtouint(buf, 10, &user_data))
		return -EINVAL;

	if (user_data >= PIN_CFG_NUM)
		return -EINVAL;

	err = sunxi_dirgpio_cfg_set(pin, user_data);
	if (err)
		return err;

	return size;
}

static ssize_t sunxi_dirgpio_pull_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", pin->pull);
}

static ssize_t sunxi_dirgpio_pull_store(struct device *dev,
			  struct device_attribute *attr, const char *buf,
			  size_t size)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);
	unsigned int user_data;
	int err;

	if (kstrtouint(buf, 10, &user_data))
		return -EINVAL;

	if (user_data >= PIN_PULL_NUM)
		return -EINVAL;

	err = sunxi_dirgpio_pull_set(pin, user_data);
	if (err)
		return err;

	return size;
}

static ssize_t sunxi_dirgpio_drv_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", pin->drv);
}

static ssize_t sunxi_dirgpio_drv_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t size)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);
	unsigned int user_data;
	int ret;

	if (kstrtouint(buf, 10, &user_data))
		return -EINVAL;

	if (user_data >= PIN_DRV)
		return -EINVAL;

	ret = sunxi_dirgpio_drv_set(pin, user_data);
	if (ret < 0)
		return ret;

	return size;
}

static ssize_t sunxi_dirgpio_data_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);
	unsigned int real_level = sunxi_dirgpio_data_get(pin);

	return sprintf(buf, "%u\n", real_level);
}

static ssize_t sunxi_dirgpio_data_store(struct device *dev,
			  struct device_attribute *attr, const char *buf,
			  size_t size)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);
	unsigned int real_level;

	if (kstrtouint(buf, 10, &real_level))
		return -EINVAL;

	sunxi_dirgpio_data_set(pin, real_level);
	return size;
}

static ssize_t sunxi_dirgpio_light_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);
	unsigned int real_level = sunxi_dirgpio_data_get(pin);

	return sprintf(buf, "%u\n", real_level == pin->active_level);
}

static ssize_t sunxi_dirgpio_light_store(struct device *dev,
			  struct device_attribute *attr, const char *buf,
			  size_t size)
{
	struct sunxi_dirgpio_pin *pin = dev_get_drvdata(dev);
	unsigned int light;

	if (kstrtouint(buf, 10, &light))
		return -EINVAL;

	sunxi_dirgpio_data_set(pin, light ? pin->active_level : !pin->active_level);  /* OR: light == pin->active_level */
	return size;
}

static struct device_attribute gpio_class_attrs[] = {
	__ATTR(cfg,  0664, sunxi_dirgpio_cfg_show,  sunxi_dirgpio_cfg_store),
	__ATTR(pull, 0664, sunxi_dirgpio_pull_show, sunxi_dirgpio_pull_store),
	__ATTR(drv,  0664, sunxi_dirgpio_drv_show,  sunxi_dirgpio_drv_store),
	__ATTR(data, 0664, sunxi_dirgpio_data_show, sunxi_dirgpio_data_store),
	__ATTR(light, 0664, sunxi_dirgpio_light_show, sunxi_dirgpio_light_store),
};

static int sunxi_dirgpio_resource_get(struct sunxi_dirgpio *chip)
{
	int err;

	chip->gpio_sdcard_reused = of_property_read_bool(chip->node, "gpio-sdcard-reused");
	if (!chip->gpio_sdcard_reused)
		dev_dbg(chip->dev, "dts property 'gpio_sdcard_reused' is not assigned\n");

	chip->gpio_num = of_property_count_strings(chip->node, "pin-names");
	if (chip->gpio_num < 0) {
		dev_err(chip->dev, "get gpio_names failed\n");
		return chip->gpio_num;
	}

	chip->gpio_names = devm_kcalloc(chip->dev, chip->gpio_num, sizeof(*chip->gpio_names), GFP_KERNEL);
	if (!chip->gpio_names)
		return -ENOMEM;

	err = of_property_read_string_array(chip->node, "pin-names",
					(const char **)chip->gpio_names, chip->gpio_num);
	if (err < 0)
		return err;

	return 0;
}

static int sunxi_dirgpio_sysfs_create(struct sunxi_dirgpio *chip)
{
	int i, j, err;
	struct device *dev;

	for (i = 0; i < chip->gpio_num; i++) {
		dev = chip->pins[i].dev;
		dev = device_create(chip->gpio_class, chip->dev, i,
						&chip->pins[i], "%s",
						chip->pins[i].name);
		if (IS_ERR(dev)) {
			err = PTR_ERR(dev);
			goto err0;
		}

		for (j = 0; j < ARRAY_SIZE(gpio_class_attrs); j++) {
			err = device_create_file(dev, &gpio_class_attrs[j]);
			if (err) {
				dev_err(chip->dev, "device_create_file() failed\n");
				goto err1;
			}
		}
	}

	return 0;
err1:
	while (j--)
		device_remove_file(dev, &gpio_class_attrs[j]);

	device_destroy(chip->gpio_class, i);
err0:
	while (i--) {
		dev = chip->pins[i].dev;

		for (j = 0; j < ARRAY_SIZE(gpio_class_attrs); j++)
			device_remove_file(dev, &gpio_class_attrs[j]);

		device_destroy(chip->gpio_class, i);
	}

	return err;
}

static void sunxi_dirgpio_sysfs_remove(struct sunxi_dirgpio *chip)
{
	int i, j;
	struct device *dev;

	for (i = 0; i < chip->gpio_num; i++) {
		dev = chip->pins[i].dev;
		for (j = 0; j < ARRAY_SIZE(gpio_class_attrs); j++)
			device_remove_file(dev, &gpio_class_attrs[j]);
		device_destroy(chip->gpio_class, i);
	}
}

static int sunxi_dirgpio_hw_init(struct sunxi_dirgpio *chip)
{
	u32 level;
	int i, err, gpio;
	enum of_gpio_flags flag;

	dev_dbg(chip->dev, "chip->gpio_num is %d\n", chip->gpio_num);
	chip->pins = devm_kcalloc(chip->dev, chip->gpio_num, sizeof(*chip->pins), GFP_KERNEL);
	if (!chip->pins)
		return -ENOMEM;

	for (i = 0; i < chip->gpio_num; i++) {
		gpio = of_get_named_gpio_flags(chip->node, "gpio-pins", i, &flag);
		if (!gpio_is_valid(gpio)) {
			dev_err(chip->dev, "get gpio error!\n");
			return gpio;
		}

		/* if gpio_sdcard_reused == 1, reuse pin port of sdcard, it does not need to request gpio */
		if (!chip->gpio_sdcard_reused) {
			err = devm_gpio_request(chip->dev, gpio, dev_name(chip->dev));
			if (err) {
				dev_err(chip->dev, "gpio_pin_%d(%d) gpio_request failed\n", i + 1, gpio);
				return err;
			}
		}
		dev_dbg(chip->dev, "gpio_pin_%d(%d), init flag is %d, gpio_is_valid\n", i + 1, gpio, flag);

		err = of_property_read_u32_index(chip->node, "init-status", i, &level);
		if (err) {
			dev_err(chip->dev, "dts property 'init-status' is not assigned\n");
			return err;
		}

		/* flag == GPIO_ACTIVE_HIGH == 0, so we reverse the flag to get the real level */
		chip->pins[i].active_level = !flag;

		/* initialize gpio as !level and set it as output */
		err = gpio_direction_output(gpio, !level);
		if (err) {
			dev_err(chip->dev, "failed to set gpio as output\n");
			return err;
		}

		chip->pins[i].name = devm_kasprintf(chip->dev, GFP_KERNEL, "%s", chip->gpio_names[i]);
		if (!chip->pins[i].name)
			return -ENOMEM;

		chip->pins[i].gpio = gpio;
	}

	return 0;
}

static int sunxi_dirgpio_probe(struct platform_device *pdev)
{
	int err;
	struct sunxi_dirgpio *chip;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->pdev = pdev;
	chip->dev = &pdev->dev;
	chip->node = chip->dev->of_node;

	/* get dts resource */
	err = sunxi_dirgpio_resource_get(chip);
	if (err) {
		dev_err(chip->dev, "sunxi_dirgpio_resource_get failed\n");
		goto err0;
	}

	/* create debug dir: /sys/class/gpio_sw */
	chip->gpio_class = class_create(THIS_MODULE, "gpio_sw");
	if (IS_ERR(chip->gpio_class)) {
		dev_err(chip->dev, "sunxi_dirgpio class_create failed\n");
		err = PTR_ERR(chip->gpio_class);
		goto err0;
	}

	err = sunxi_dirgpio_hw_init(chip);
	if (err) {
		dev_err(chip->dev, "sunxi_dirgpio_hw_init failed\n");
		goto err1;
	}

	err = sunxi_dirgpio_sysfs_create(chip);
	if (err) {
		dev_err(chip->dev, "sunxi_dirgpio_sysfs_create failed\n");
		goto err1;
	}

	platform_set_drvdata(pdev, chip);
	dev_info(chip->dev, "sunxi direct gpio probe success\n");
	return 0;

err1:
	class_destroy(chip->gpio_class);
err0:
	return err;
}

static int sunxi_dirgpio_remove(struct platform_device *pdev)
{
	struct sunxi_dirgpio *chip = platform_get_drvdata(pdev);

	sunxi_dirgpio_sysfs_remove(chip);
	class_destroy(chip->gpio_class);

	dev_info(chip->dev, "sunxi direct gpio remove success\n");
	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static int sunxi_dirgpio_suspend(struct device *dev)
{
	return 0;
}

static int sunxi_dirgpio_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops sunxi_dirgpio_pm_ops = {
	.suspend = sunxi_dirgpio_suspend,
	.resume = sunxi_dirgpio_resume,
};

#define SUNXI_DIRGPIO_PM_OPS (&sunxi_dirgpio_pm_ops)
#else
#define SUNXI_DIRGPIO_PM_OPS NULL
#endif

static const struct of_device_id sunxi_dirgpio_of_match[] = {
	{.compatible = "allwinner,sunxi-direct-gpio",},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_dirgpio_of_match);

static struct platform_driver sunxi_dirgpio_driver = {
	.driver = {
		   .name = "sunxi-direct-gpio",
		   .owner = THIS_MODULE,
		   .pm = SUNXI_DIRGPIO_PM_OPS,
		   .of_match_table = of_match_ptr(sunxi_dirgpio_of_match),
	},
	.probe	= sunxi_dirgpio_probe,
	.remove	= sunxi_dirgpio_remove,
};

module_platform_driver(sunxi_dirgpio_driver);

MODULE_AUTHOR("shaosidi <shaosidi@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi direct gpio driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");

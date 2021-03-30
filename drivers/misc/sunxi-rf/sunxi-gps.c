#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/sys_config.h>
#include <linux/etherdevice.h>
#include <linux/pinctrl/pinconf-sunxi.h>

struct sunxi_gps_platdata {
	struct regulator *gps_power;
	struct clk *lpo;
	int gpio_gps_nstandby;
	char *gps_power_name;
	int nstandby_state;
	struct platform_device *pdev;
};
static struct sunxi_gps_platdata *gps_data;
static DEFINE_MUTEX(sunxi_gps_mutex);

static ssize_t nstandby_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gps_data->nstandby_state);
}

static int sunxi_gps_power(struct sunxi_gps_platdata *data, bool on_off)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	int ret = 0;

	if (on_off) {
		ret = regulator_enable(data->gps_power);
		if (ret < 0) {
			dev_err(dev, "regulator gps_power enable failed\n");
			return ret;
		}

		ret = regulator_get_voltage(data->gps_power);
		if (ret < 0) {
			dev_err(dev, "regulator gps_power get voltage failed\n");
			return ret;
		}
		dev_info(dev, "check gps_power voltage: %d\n", ret);
	} else {
		ret = regulator_disable(data->gps_power);
		if (ret < 0) {
			dev_err(dev, "regulator gps_power disable failed\n");
			return ret;
		}
	}
	return 0;
}

static ssize_t nstandby_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	int err;

	dev_info(dev, "set nstandby_state: %s\n", buf);
	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	if (state > 1)
		return -EINVAL;

	mutex_lock(&sunxi_gps_mutex);
	if (state != gps_data->nstandby_state &&
			gpio_is_valid(gps_data->gpio_gps_nstandby)) {
		err = gpio_direction_output(gps_data->gpio_gps_nstandby, state);
		if (err) {
			dev_err(dev, "set power failed\n");
		} else {
			gps_data->nstandby_state = state;
		}
	}
	mutex_unlock(&sunxi_gps_mutex);

	return count;
}

static DEVICE_ATTR(nstandby_state, S_IWUSR | S_IWGRP | S_IRUGO,
		nstandby_state_show, nstandby_state_store);

static struct attribute *misc_attributes[] = {
	&dev_attr_nstandby_state.attr,
	NULL,
};

static struct attribute_group misc_attribute_group = {
	.name  = "rf-ctrl",
	.attrs = misc_attributes,
};

static struct miscdevice sunxi_gps_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "sunxi-gps",
};

static int sunxi_gps_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct sunxi_gps_platdata *data = NULL;
	struct gpio_config config;
	const char *power = NULL;
	char pin_name[32] = {0};
	int clk_gpio = 0;
	unsigned long pin_config = 0;
	int ret = 0;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	data->pdev = pdev;
	gps_data = data;

	if (of_property_read_string(np, "gps_power", &power)) {
		dev_warn(dev, "Missing gps_power.\n");
	} else {
		data->gps_power_name = devm_kzalloc(dev, 64, GFP_KERNEL);
		if (!data->gps_power_name) {
			return -ENOMEM;
		} else {
			strcpy(data->gps_power_name, power);
			dev_info(dev, "gps_power_name (%s)\n",
				data->gps_power_name);
		}
		data->gps_power = regulator_get(dev, data->gps_power_name);
		if (!IS_ERR(data->gps_power)) {
			ret = sunxi_gps_power(data, 1);
			if (ret) {
				dev_err(dev, "gps power on failed\n");
				return ret;
			} else {
				dev_info(dev, "gps power on success\n");
			}
		}
	}

	data->gpio_gps_nstandby = of_get_named_gpio_flags(np,
		"gps_nstandby", 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_gps_nstandby)) {
		dev_err(dev, "get gpio gps_nstandby failed\n");
	} else {
		dev_info(dev, "gps_nstandby gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_gps_nstandby,
				"gps_nstandby");
		if (ret < 0) {
			dev_err(dev, "can't request gps_nstandby gpio %d\n",
				data->gpio_gps_nstandby);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_gps_nstandby, 0);
		if (ret < 0) {
			dev_err(dev, "can't request output direction gps_nstandby gpio %d\n",
				data->gpio_gps_nstandby);
			return ret;
		} else {
			data->nstandby_state = 0;
		}
	}

	clk_gpio = of_get_named_gpio_flags(np, "gps_clk_gpio", 0,
						(enum of_gpio_flags *)&config);
	if (!gpio_is_valid(clk_gpio)) {
		dev_err(dev, "get gpio gps_clk_gpio failed\n");
	} else {
		dev_info(dev, "gps_clk_gpio gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, clk_gpio, "gps_clk_gpio");
		if (ret < 0) {
			dev_err(dev, "can't request gps_clk_gpio gpio %d\n",
				clk_gpio);
			return ret;
		}

		sunxi_gpio_to_name(config.gpio, pin_name);
		pin_config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,
					config.mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, pin_config);
	}
	data->lpo = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(data->lpo)) {
		dev_warn(dev, "clk not config\n");
	} else {
		dev_warn(dev, "enable clk\n");
		ret = clk_prepare_enable(data->lpo);
		if (ret < 0)
			dev_warn(dev, "can't enable clk\n");
	}

	ret = misc_register(&sunxi_gps_dev);
	if (ret) {
		dev_err(dev, "sunxi-gps register driver as misc device error!\n");
		return ret;
	}
	ret = sysfs_create_group(&sunxi_gps_dev.this_device->kobj,
			&misc_attribute_group);
	if (ret) {
		dev_err(dev, "sunxi-gps register sysfs create group failed!\n");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_gps_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (gps_data->nstandby_state &&
			gpio_is_valid(gps_data->gpio_gps_nstandby)) {
		gpio_direction_output(gps_data->gpio_gps_nstandby, 0);
	}
	return 0;
}

static int sunxi_gps_resume(struct platform_device *pdev)
{
	if (gps_data->nstandby_state &&
			gpio_is_valid(gps_data->gpio_gps_nstandby)) {
		gpio_direction_output(gps_data->gpio_gps_nstandby, 1);
	}
	return 0;
}
#endif

static int sunxi_gps_remove(struct platform_device *pdev)
{
	WARN_ON(0 != misc_deregister(&sunxi_gps_dev));
	sysfs_remove_group(&(sunxi_gps_dev.this_device->kobj),
			&misc_attribute_group);

	if (!IS_ERR_OR_NULL(gps_data->lpo)) {
		clk_disable_unprepare(gps_data->lpo);
		clk_put(gps_data->lpo);
	}

	if (!IS_ERR(gps_data->gps_power)) {
		sunxi_gps_power(gps_data, 0);
		regulator_put(gps_data->gps_power);
	}

	return 0;
}

static const struct of_device_id sunxi_gps_ids[] = {
	{ .compatible = "allwinner,sunxi-gps" },
	{ /* Sentinel */ }
};

static struct platform_driver sunxi_gps_driver = {
	.probe	= sunxi_gps_probe,
	.remove	= sunxi_gps_remove,
#ifdef CONFIG_PM
	.suspend = sunxi_gps_suspend,
	.resume = sunxi_gps_resume,
#endif
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "sunxi-gps",
		.of_match_table	= sunxi_gps_ids,
	},
};

module_platform_driver(sunxi_gps_driver);

MODULE_DESCRIPTION("sunxi gps driver");
MODULE_LICENSE(GPL);

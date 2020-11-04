#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/rfkill.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/sunxi-gpio.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>
#include <linux/capability.h>
#include <linux/pm_wakeirq.h>

#include "sunxi-rfkill.h"

static struct sunxi_wlan_platdata *wlan_data;

static int sunxi_wlan_on(struct sunxi_wlan_platdata *data, bool on_off);
static DEFINE_MUTEX(sunxi_wlan_mutex);

void sunxi_wl_chipen_set(int dev, int on_off)
{
	/* Only wifi and bt both close, chip_en goes down,
	 * otherwise, set chip_en up to keep module work.
	 * dev   : device to set power status. 0: wifi, 1: bt
	 * on_off: power status to set. 0: off, 1: on
	 */
	static int power_state;

	if (dev == WL_DEV_WIFI || dev == WL_DEV_BLUETOOTH) {
		power_state &= ~(1 << dev);
		power_state |= ((on_off > 0) << dev);
	}

	if (gpio_is_valid(wlan_data->gpio_chip_en))
		gpio_set_value(wlan_data->gpio_chip_en, (power_state != 0));
}
EXPORT_SYMBOL_GPL(sunxi_wl_chipen_set);

void sunxi_wl_poweren_set(int dev, int on_off)
{
	/* Only wifi and bt both close, power_en goes down,
	 * otherwise, set power_en up to keep module work.
	 * dev   : device to set power status. 0: wifi, 1: bt
	 * on_off: power status to set. 0: off, 1: on
	 */
	static int power_state;

	if (dev == WL_DEV_WIFI || dev == WL_DEV_BLUETOOTH) {
		power_state &= ~(1 << dev);
		power_state |= ((on_off > 0) << dev);
	}

	if (gpio_is_valid(wlan_data->gpio_power_en))
		gpio_set_value(wlan_data->gpio_power_en, (power_state != 0));
}
EXPORT_SYMBOL_GPL(sunxi_wl_poweren_set);

void sunxi_wlan_set_power(bool on_off)
{
	struct platform_device *pdev;
	int ret = 0;

	if (!wlan_data)
		return;

	pdev = wlan_data->pdev;
	mutex_lock(&sunxi_wlan_mutex);
	sunxi_wl_poweren_set(WL_DEV_WIFI, on_off);
	if (on_off != wlan_data->power_state) {
		ret = sunxi_wlan_on(wlan_data, on_off);
		if (ret)
			dev_err(&pdev->dev, "set power failed\n");
	}

	sunxi_wl_chipen_set(WL_DEV_WIFI, on_off);

	mutex_unlock(&sunxi_wlan_mutex);
}
EXPORT_SYMBOL_GPL(sunxi_wlan_set_power);

int sunxi_wlan_get_bus_index(void)
{
	struct platform_device *pdev;

	if (!wlan_data)
		return -EINVAL;

	pdev = wlan_data->pdev;
	dev_info(&pdev->dev, "bus_index: %d\n", wlan_data->bus_index);
	return wlan_data->bus_index;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_get_bus_index);

int sunxi_wlan_get_oob_irq(void)
{
	struct platform_device *pdev;
	int host_oob_irq = 0;

	if (!wlan_data || !gpio_is_valid(wlan_data->gpio_wlan_hostwake))
		return 0;

	pdev = wlan_data->pdev;

	host_oob_irq = gpio_to_irq(wlan_data->gpio_wlan_hostwake);
	if (host_oob_irq < 0)
		dev_err(&pdev->dev, "map gpio [%d] to virq failed, errno = %d\n",
			wlan_data->gpio_wlan_hostwake, host_oob_irq);

	return host_oob_irq;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_get_oob_irq);

int sunxi_wlan_get_oob_irq_flags(void)
{
	int oob_irq_flags;

	if (!wlan_data)
		return 0;

	oob_irq_flags = (IRQF_TRIGGER_HIGH | IRQF_SHARED | IRQF_NO_SUSPEND);

	return oob_irq_flags;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_get_oob_irq_flags);

static int sunxi_wlan_on(struct sunxi_wlan_platdata *data, bool on_off)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	int ret = 0;

	if (!on_off && gpio_is_valid(data->gpio_wlan_regon))
		gpio_set_value(data->gpio_wlan_regon, 0);

	if (data->wlan_power_name) {
		data->wlan_power = regulator_get(dev, data->wlan_power_name);
		if (!IS_ERR(data->wlan_power)) {
			if (on_off) {
				ret = regulator_enable(data->wlan_power);
				if (ret < 0) {
					dev_err(dev, "regulator wlan_power enable failed\n");
					regulator_put(data->wlan_power);
					return ret;
				}

				ret = regulator_get_voltage(data->wlan_power);
				if (ret < 0) {
					dev_err(dev, "regulator wlan_power get voltage failed\n");
					regulator_put(data->wlan_power);
					return ret;
				}
				dev_info(dev, "check wlan wlan_power voltage: %d\n",
						 ret);
			} else {
				ret = regulator_disable(data->wlan_power);
				if (ret < 0) {
					dev_err(dev, "regulator wlan_power disable failed\n");
					regulator_put(data->wlan_power);
					return ret;
				}
			}
			regulator_put(data->wlan_power);
		}
	}

	if (data->io_regulator_name) {
		data->io_regulator = regulator_get(dev,
				data->io_regulator_name);
		if (!IS_ERR(data->io_regulator)) {
			if (on_off) {
				ret = regulator_enable(data->io_regulator);
				if (ret < 0) {
					dev_err(dev, "regulator io_regulator enable failed\n");
					regulator_put(data->io_regulator);
					return ret;
				}

				ret = regulator_get_voltage(data->io_regulator);
				if (ret < 0) {
					dev_err(dev, "regulator io_regulator get voltage failed\n");
					regulator_put(data->io_regulator);
					return ret;
				}
				dev_info(dev, "check wlan io_regulator voltage: %d\n",
						ret);
			} else {
				ret = regulator_disable(data->io_regulator);
				if (ret < 0) {
					dev_err(dev, "regulator io_regulator disable failed\n");
					regulator_put(data->io_regulator);
					return ret;
				}
			}
			regulator_put(data->io_regulator);
		}
	}

	if (on_off && gpio_is_valid(data->gpio_wlan_regon)) {
		mdelay(10);
		gpio_set_value(data->gpio_wlan_regon, 1);
	}
	wlan_data->power_state = on_off;

	return 0;
}

static ssize_t power_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", wlan_data->power_state);
}

static ssize_t power_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	int err;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	if (state > 1)
		return -EINVAL;

	if (state != wlan_data->power_state) {
		sunxi_wlan_set_power(state);
	}

	return count;
}

static DEVICE_ATTR(power_state, S_IRUGO | S_IWUSR,
		power_state_show, power_state_store);

static ssize_t scan_device_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	int err;
	int bus = wlan_data->bus_index;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	dev_info(dev, "start scan device on bus_index: %d\n",
			wlan_data->bus_index);
	if (bus < 0) {
		dev_err(dev, "scan device fail!\n");
		return -1;
	}
	sunxi_mmc_rescan_card(bus);

	return count;
}

static DEVICE_ATTR(scan_device, S_IRUGO | S_IWUSR,
		NULL, scan_device_store);

static struct attribute *misc_attributes[] = {
	&dev_attr_power_state.attr,
	&dev_attr_scan_device.attr,
	NULL,
};

static struct attribute_group misc_attribute_group = {
	.name  = "rf-ctrl",
	.attrs = misc_attributes,
};

static struct miscdevice sunxi_wlan_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "sunxi-wlan",
};

static int sunxi_wlan_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct sunxi_wlan_platdata *data;
	struct gpio_config config;
	u32 val;
	const char *power, *io_regulator;
	int ret = 0;
	char *pctrl_name = PINCTRL_STATE_DEFAULT;
	struct pinctrl_state *pctrl_state = NULL;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	data->pdev = pdev;
	wlan_data = data;

	data->bus_index = -1;
	if (!of_property_read_u32(np, "wlan_busnum", &val)) {
		switch (val) {
		case 0:
		case 1:
		case 2:
		case 3:
			data->bus_index = val;
			break;
		default:
			dev_err(dev, "unsupported wlan_busnum (%u)\n", val);
			return -EINVAL;
		}
	}
	dev_info(dev, "wlan_busnum (%u)\n", val);

	if (of_property_read_string(np, "wlan_power", &power)) {
		dev_warn(dev, "Missing wlan_power.\n");
	} else {
		data->wlan_power_name = devm_kzalloc(dev, 64, GFP_KERNEL);
		if (!data->wlan_power_name)
			return -ENOMEM;
		strcpy(data->wlan_power_name, power);
	}
	dev_info(dev, "wlan_power_name (%s)\n", data->wlan_power_name);

	if (of_property_read_string(np, "wlan_io_regulator", &io_regulator)) {
		dev_warn(dev, "Missing wlan_io_regulator.\n");
	} else {
		data->io_regulator_name = devm_kzalloc(dev, 64, GFP_KERNEL);
		if (!data->io_regulator_name)
			return -ENOMEM;
		strcpy(data->io_regulator_name, io_regulator);
	}
	dev_info(dev, "io_regulator_name (%s)\n", data->io_regulator_name);

	data->gpio_wlan_regon = of_get_named_gpio_flags(np, "wlan_regon",
			0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_wlan_regon)) {
		dev_err(dev, "get gpio wlan_regon failed\n");
	} else {
		dev_info(dev, "wlan_regon gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_wlan_regon,
				"wlan_regon");
		if (ret < 0) {
			dev_err(dev, "can't request wlan_regon gpio %d\n",
				data->gpio_wlan_regon);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_wlan_regon, 0);
		if (ret < 0) {
			dev_err(dev, "can't request output direction wlan_regon gpio %d\n",
				data->gpio_wlan_regon);
			return ret;
		}
	}

	data->gpio_chip_en = of_get_named_gpio_flags(np, "chip_en",
			0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_chip_en)) {
		dev_err(dev, "get gpio chip_en failed\n");
	} else {
		dev_info(dev, "chip_en gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_chip_en, "chip_en");
		if (ret < 0) {
			dev_err(dev, "can't request chip_en gpio %d\n",
				data->gpio_chip_en);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_chip_en, 0);
		if (ret < 0) {
			dev_err(dev, "can't request output direction chip_en gpio %d\n",
				data->gpio_chip_en);
			return ret;
		}
	}

	data->gpio_power_en = of_get_named_gpio_flags(np, "power_en",
			0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_power_en)) {
		dev_err(dev, "get gpio power_en failed\n");
	} else {
		dev_info(dev, "power_en gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_power_en, "power_en");
		if (ret < 0) {
			dev_err(dev, "can't request power_en gpio %d\n",
				data->gpio_power_en);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_power_en, 0);
		if (ret < 0) {
			dev_err(dev, "can't request output direction power_en gpio %d\n",
				data->gpio_power_en);
			return ret;
		}
	}

	data->gpio_wlan_hostwake = of_get_named_gpio_flags(np, "wlan_hostwake",
			0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_wlan_hostwake)) {
		dev_err(dev, "get gpio wlan_hostwake failed\n");
	} else {
		dev_info(dev,
				"wlan_hostwake gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_wlan_hostwake,
				"wlan_hostwake");
		if (ret < 0) {
			dev_err(dev, "can't request wlan_hostwake gpio %d\n",
				data->gpio_wlan_hostwake);
			return ret;
		}

		ret = gpio_direction_input(data->gpio_wlan_hostwake);
		if (ret < 0) {
			dev_err(dev,
				"can't request input direction wlan_hostwake gpio %d\n",
				data->gpio_wlan_hostwake);
			return ret;
		}

		/*
		 * wakeup_source relys on wlan_hostwake, if wlan_hostwake gpio
		 * isn't configured, then whether wakeup_source is configured
		 * or not is unmeaningful.
		 */
		if (!of_property_read_u32(np, "wakeup_source",
			&data->wakeup_enable) && (data->wakeup_enable == 0)) {
			dev_warn(dev, "wakeup source is disabled!\n");
		} else {
			ret = device_init_wakeup(dev, true);
			if (ret < 0) {
				dev_err(dev, "device init wakeup failed!\n");
				return ret;
			}

			ret = dev_pm_set_wake_irq(dev, gpio_to_irq(data->gpio_wlan_hostwake));
			if (ret < 0) {
				dev_err(dev, "can't enable wakeup src for wlan_hostwake %d\n",
					data->gpio_wlan_hostwake);
				return ret;
			}
			data->wakeup_enable = 1;
		}
	}

	data->lpo = devm_clk_get(dev, NULL);
	if (IS_ERR_OR_NULL(data->lpo)) {
		dev_warn(dev, "clk not config\n");
	} else {
		ret = clk_prepare_enable(data->lpo);
		if (ret < 0)
			dev_warn(dev, "can't enable clk\n");
	}

	data->pctrl = devm_pinctrl_get(dev);
	if (IS_ERR(data->pctrl)) {
		dev_warn(dev, "devm_pinctrl_get() failed!\n");
	} else {
		pctrl_state = pinctrl_lookup_state(data->pctrl, pctrl_name);
		if (IS_ERR(pctrl_state)) {
			dev_warn(dev, "pinctrl_lookup_state(%s) failed! return %p \n",
					pctrl_name, pctrl_state);
		} else {
			ret = pinctrl_select_state(data->pctrl, pctrl_state);
			if (ret < 0) {
				dev_warn(dev, "pinctrl_select_state(%s) failed! return %d \n",
						pctrl_name, ret);
			}
		}
	}

	ret = misc_register(&sunxi_wlan_dev);
	if (ret) {
		dev_err(dev, "sunxi-wlan register driver as misc device error!\n");
		return ret;
	}
	ret = sysfs_create_group(&sunxi_wlan_dev.this_device->kobj,
			&misc_attribute_group);
	if (ret) {
		dev_err(dev, "sunxi-wlan register sysfs create group failed!\n");
		return ret;
	}

	data->power_state = 0;

	return 0;
}

static int sunxi_wlan_remove(struct platform_device *pdev)
{
	misc_deregister(&sunxi_wlan_dev);
	sysfs_remove_group(&(sunxi_wlan_dev.this_device->kobj),
			&misc_attribute_group);

	if (!IS_ERR_OR_NULL(wlan_data->lpo))
		clk_disable_unprepare(wlan_data->lpo);

	if (wlan_data->wakeup_enable) {
		dev_info(&pdev->dev, "Deinit wakeup source");
		device_init_wakeup(&pdev->dev, false);
		dev_pm_clear_wake_irq(&pdev->dev);
	}

	return 0;
}

static const struct of_device_id sunxi_wlan_ids[] = {
	{ .compatible = "allwinner,sunxi-wlan" },
	{ /* Sentinel */ }
};

static struct platform_driver sunxi_wlan_driver = {
	.probe		= sunxi_wlan_probe,
	.remove	= sunxi_wlan_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "sunxi-wlan",
		.of_match_table	= sunxi_wlan_ids,
	},
};

module_platform_driver(sunxi_wlan_driver);

MODULE_DESCRIPTION("sunxi wlan driver");
MODULE_LICENSE(GPL);

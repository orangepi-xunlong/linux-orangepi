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
#include <linux/miscdevice.h>
#include <linux/capability.h>
#include <linux/pm_wakeirq.h>
#include "internal.h"
#include "sunxi-rfkill.h"

static struct sunxi_wlan_platdata *wlan_data;
static const struct of_device_id sunxi_wlan_ids[];

static int sunxi_wlan_on(struct sunxi_wlan_platdata *data, bool on_off);
static DEFINE_MUTEX(sunxi_wlan_mutex);

#if IS_ENABLED(CONFIG_MMC_SUNXI)
extern void sunxi_mmc_rescan_card(unsigned ids);
#else
static void sunxi_mmc_rescan_card(unsigned ids)
{
	(void)ids;
}
#endif

void sunxi_wlan_set_power(bool on_off)
{
	struct platform_device *pdev;
	int ret = 0;

	if (!wlan_data)
		return;

	pdev = wlan_data->pdev;
	mutex_lock(&sunxi_wlan_mutex);
	rfkill_poweren_set(WL_DEV_WIFI, on_off);
	if (on_off != wlan_data->power_state) {
		ret = sunxi_wlan_on(wlan_data, on_off);
		if (ret)
			dev_err(&pdev->dev, "set power failed\n");
	}

	rfkill_chipen_set(WL_DEV_WIFI, on_off);

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

int sunxi_wlan_get_oob_irq(int *irq_flags, int *wakup_enable)
{
	struct platform_device *pdev;
	int host_oob_irq = 0;
	int oob_irq_flags = 0;

	if (!wlan_data || !gpio_is_valid(wlan_data->gpio_wlan_hostwake))
		return 0;

	pdev = wlan_data->pdev;

	host_oob_irq = gpio_to_irq(wlan_data->gpio_wlan_hostwake);
	if (host_oob_irq < 0)
		dev_err(&pdev->dev, "map gpio [%d] to virq failed, errno = %d\n",
			wlan_data->gpio_wlan_hostwake, host_oob_irq);

	oob_irq_flags = IRQF_SHARED;
	if (wlan_data->gpio_wlan_hostwake_assert)
		oob_irq_flags |= IRQF_TRIGGER_HIGH;
	else
		oob_irq_flags |= IRQF_TRIGGER_LOW;

	*irq_flags = oob_irq_flags;
	*wakup_enable = wlan_data->wakeup_enable;

	return host_oob_irq;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_get_oob_irq);

static int sunxi_wlan_on(struct sunxi_wlan_platdata *data, bool on_off)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	int ret = 0, i;

	if (on_off) {
		for (i = 0; i < CLK_MAX; i++) {
			if (!IS_ERR_OR_NULL(data->clk[i]))
				clk_prepare_enable(data->clk[i]);
		}

		for (i = 0; i < PWR_MAX; i++) {
			if (!IS_ERR_OR_NULL(data->power[i])) {
				if (data->power_vol[i]) {
					ret = regulator_set_voltage(data->power[i],
							data->power_vol[i], data->power_vol[i]);
					if (ret < 0) {
						dev_err(dev, "wlan power[%d] (%s) set voltage failed\n",
									i, data->power_name[i]);
						return ret;
					}

					ret = regulator_get_voltage(data->power[i]);
					if (ret != data->power_vol[i]) {
						dev_err(dev, "wlan power[%d] (%s) get voltage failed\n",
								i, data->power_name[i]);
						return ret;
					}
				}

				ret = regulator_enable(data->power[i]);
				if (ret < 0) {
					dev_err(dev, "wlan power[%d] (%s) enable failed\n",
								i, data->power_name[i]);
					return ret;
				}
			}
		}

		if (gpio_is_valid(data->gpio_wlan_regon)) {
			mdelay(10);
			gpio_set_value(data->gpio_wlan_regon, data->gpio_wlan_regon_assert);
		}
	} else {
		if (gpio_is_valid(data->gpio_wlan_regon))
			gpio_set_value(data->gpio_wlan_regon, !data->gpio_wlan_regon_assert);

		for (i = 0; i < PWR_MAX; i++) {
			if (!IS_ERR_OR_NULL(data->power[i])) {
				ret = regulator_disable(data->power[i]);
				if (ret < 0) {
					dev_err(dev, "wlan power[%d] (%s) disable failed\n",
								i, data->power_name[i]);
					return ret;
				}
			}
		}

		for (i = 0; i < CLK_MAX; i++) {
			if (!IS_ERR_OR_NULL(data->clk[i]))
				clk_disable_unprepare(data->clk[i]);
		}
	}

	wlan_data->power_state = on_off;
	dev_info(dev, "wlan power %s success\n", on_off ? "on" : "off");

	return 0;
}

static ssize_t power_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (!wlan_data)
		return 0;
	return sprintf(buf, "%d\n", wlan_data->power_state);
}

static ssize_t power_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	int err;

	if (!wlan_data)
		return 0;

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

	if (!wlan_data)
		return 0;

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

int sunxi_wlan_init(struct platform_device *pdev)
{
	struct device_node *np = of_find_matching_node(pdev->dev.of_node, sunxi_wlan_ids);
	struct device *dev = &pdev->dev;
	struct sunxi_wlan_platdata *data;
	enum of_gpio_flags config;
	u32 val;
	int ret = 0;
	int count, i;

	if (!dev)
		return -ENOMEM;

	if (!np)
		return 0;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);

	data->pdev = pdev;

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

	count = of_property_count_strings(np, "wlan_power");
	if (count <= 0) {
		dev_warn(dev, "Missing wlan_power.\n");
	} else {
		if (count > PWR_MAX) {
			dev_warn(dev, "wlan power count large than max(%d > %d).\n",
						count, PWR_MAX);
			count = PWR_MAX;
		}
		ret = of_property_read_string_array(np, "wlan_power",
					(const char **)data->power_name, count);
		if (ret < 0)
			return -ENOMEM;

		ret = of_property_read_u32_array(np, "wlan_power_vol",
					(u32 *)data->power_vol, count);
		if (ret < 0)
			dev_warn(dev, "Missing wlan_power_vol config.\n");

		for (i = 0; i < count; i++) {
			data->power[i] = regulator_get(dev, data->power_name[i]);

			if (IS_ERR_OR_NULL(data->power[i]))
				return -ENOMEM;

			dev_info(dev, "wlan power[%d] (%s) voltage: %dmV\n",
					i, data->power_name[i], data->power_vol[i] / 1000);
		}
	}

	count = of_property_count_strings(np, "clock-names");
	if (count <= 0) {
		count = CLK_MAX;
		for (i = 0; i < count; i++) {
			data->clk[i] = of_clk_get(np, i);
			if (IS_ERR_OR_NULL(data->clk[i]))
				break;
			data->clk_name[i] = devm_kzalloc(dev, 16, GFP_KERNEL);
			sprintf(data->clk_name[i], "clk%d", i);
			dev_info(dev, "wlan clock[%d] (%s)\n", i, data->clk_name[i]);
		}
	} else {
		if (count > CLK_MAX) {
			dev_warn(dev, "wlan clocks count large than max(%d).\n", CLK_MAX);
			count = CLK_MAX;
		}
		ret = of_property_read_string_array(np, "clock-names",
					(const char **)data->clk_name, count);
		if (ret < 0)
			return -ENOMEM;

		for (i = 0; i < count; i++) {
			data->clk[i] = of_clk_get(np, i);
			if (IS_ERR_OR_NULL(data->clk[i]))
				return -ENOMEM;
			dev_info(dev, "wlan clock[%d] (%s)\n", i, data->clk_name[i]);
		}
	}

	data->gpio_wlan_regon = of_get_named_gpio_flags(np, "wlan_regon", 0, &config);
	if (!gpio_is_valid(data->gpio_wlan_regon)) {
		dev_err(dev, "get gpio wlan_regon failed\n");
	} else {
		data->gpio_wlan_regon_assert = (config == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		dev_info(dev, "wlan_regon gpio=%d assert=%d\n", data->gpio_wlan_regon, data->gpio_wlan_regon_assert);

		ret = devm_gpio_request(dev, data->gpio_wlan_regon,
				"wlan_regon");
		if (ret < 0) {
			dev_err(dev, "can't request wlan_regon gpio %d\n",
				data->gpio_wlan_regon);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_wlan_regon, !data->gpio_wlan_regon_assert);
		if (ret < 0) {
			dev_err(dev, "can't request output direction wlan_regon gpio %d\n",
				data->gpio_wlan_regon);
			return ret;
		}
	}

	data->gpio_wlan_hostwake = of_get_named_gpio_flags(np, "wlan_hostwake", 0, &config);
	if (!gpio_is_valid(data->gpio_wlan_hostwake)) {
		dev_err(dev, "get gpio wlan_hostwake failed\n");
	} else {
		data->gpio_wlan_hostwake_assert = (config == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		dev_info(dev, "wlan_hostwake gpio=%d assert=%d\n", data->gpio_wlan_hostwake, data->gpio_wlan_hostwake_assert);

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
		if (!of_property_read_bool(np, "wakeup-source")) {
			dev_info(dev, "wakeup source is disbled!\n");
		} else {
			dev_info(dev, "wakeup source is enabled\n");
			data->wakeup_enable = 1;
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
	wlan_data = data;
	return 0;
}

int sunxi_wlan_deinit(struct platform_device *pdev)
{
	struct sunxi_wlan_platdata *data = wlan_data;
	int i;

	if (!data)
		return 0;

	sysfs_remove_group(&(sunxi_wlan_dev.this_device->kobj),
			&misc_attribute_group);
	misc_deregister(&sunxi_wlan_dev);

	if (data->power_state)
		sunxi_wlan_set_power(0);

	for (i = 0; i < PWR_MAX; i++) {
		if (!IS_ERR_OR_NULL(data->power[i]))
			regulator_put(data->power[i]);
	}

	wlan_data = NULL;

	return 0;
}

static const struct of_device_id sunxi_wlan_ids[] = {
	{ .compatible = "allwinner,sunxi-wlan" },
	{ /* Sentinel */ }
};

MODULE_DESCRIPTION("sunxi wlan driver");
MODULE_LICENSE("GPL");

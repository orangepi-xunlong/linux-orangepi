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
#include <linux/rfkill.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include "internal.h"
#include "sunxi-rfkill.h"

static struct sunxi_bt_platdata *bluetooth_data;
static const struct of_device_id sunxi_bt_ids[];

static int sunxi_bt_on(struct sunxi_bt_platdata *data, bool on_off);
static DEFINE_MUTEX(sunxi_bluetooth_mutex);

void sunxi_bluetooth_set_power(bool on_off)
{
	struct platform_device *pdev;
	int ret = 0;

	if (!bluetooth_data)
		return;

	pdev = bluetooth_data->pdev;
	mutex_lock(&sunxi_bluetooth_mutex);
	rfkill_poweren_set(WL_DEV_BLUETOOTH, on_off);
	if (on_off != bluetooth_data->power_state) {
		ret = sunxi_bt_on(bluetooth_data, on_off);
		if (ret)
			dev_err(&pdev->dev, "set power failed\n");
	}
	rfkill_chipen_set(WL_DEV_BLUETOOTH, on_off);
	mutex_unlock(&sunxi_bluetooth_mutex);
}
EXPORT_SYMBOL_GPL(sunxi_bluetooth_set_power);

static int sunxi_bt_on(struct sunxi_bt_platdata *data, bool on_off)
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
						dev_err(dev, "bt power[%d] (%s) set voltage failed\n",
								i, data->power_name[i]);
						return ret;
					}

					ret = regulator_get_voltage(data->power[i]);
					if (ret != data->power_vol[i]) {
						dev_err(dev, "bt power[%d] (%s) get voltage failed\n",
								i, data->power_name[i]);
						return ret;
					}
				}

				ret = regulator_enable(data->power[i]);
				if (ret < 0) {
					dev_err(dev, "bt power[%d] (%s) enable failed\n",
								i, data->power_name[i]);
					return ret;
				}
			}
		}

		if (gpio_is_valid(data->gpio_bt_rst)) {
			mdelay(10);
			gpio_set_value(data->gpio_bt_rst, !data->gpio_bt_rst_assert);
		}
	} else {
		if (gpio_is_valid(data->gpio_bt_rst))
			gpio_set_value(data->gpio_bt_rst, data->gpio_bt_rst_assert);

		for (i = 0; i < PWR_MAX; i++) {
			if (!IS_ERR_OR_NULL(data->power[i])) {
				ret = regulator_disable(data->power[i]);
				if (ret < 0) {
					dev_err(dev, "bt power[%d] (%s) disable failed\n",
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

	data->power_state = on_off;
	dev_info(dev, "bt power %s success\n", on_off ? "on" : "off");

	return 0;
}

static int sunxi_bt_set_block(void *data, bool blocked)
{
	struct sunxi_bt_platdata *platdata = data;
	struct platform_device *pdev = platdata->pdev;
	int ret;

	if (!bluetooth_data)
		return 0;

	if (blocked != platdata->power_state) {
		dev_warn(&pdev->dev, "block state already is %d\n", blocked);
		return 0;
	}

	dev_info(&pdev->dev, "set block: %d\n", blocked);
	rfkill_poweren_set(WL_DEV_BLUETOOTH, !blocked);
	ret = sunxi_bt_on(platdata, !blocked);
	if (ret) {
		dev_err(&pdev->dev, "set block failed\n");
		return ret;
	}

	rfkill_chipen_set(WL_DEV_BLUETOOTH, !blocked);

	return 0;
}

static const struct rfkill_ops sunxi_bt_rfkill_ops = {
	.set_block = sunxi_bt_set_block,
};

int sunxi_bt_init(struct platform_device *pdev)
{
	struct device_node *np = of_find_matching_node(pdev->dev.of_node, sunxi_bt_ids);
	struct device *dev = &pdev->dev;
	struct sunxi_bt_platdata *data;
	enum of_gpio_flags config;
	int ret = 0;
	int count, i;

	if (!dev)
		return -ENOMEM;

	if (!np)
		return 0;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	data->pdev = pdev;

	count = of_property_count_strings(np, "bt_power");
	if (count <= 0) {
		dev_warn(dev, "Missing bt_power.\n");
	} else {
		if (count > PWR_MAX) {
			dev_warn(dev, "bt power count large than max(%d).\n", PWR_MAX);
			count = PWR_MAX;
		}
		ret = of_property_read_string_array(np, "bt_power",
					(const char **)data->power_name, count);
		if (ret < 0)
			return -ENOMEM;

		ret = of_property_read_u32_array(np, "bt_power_vol",
					(u32 *)data->power_vol, count);
		if (ret < 0)
			dev_warn(dev, "Missing bt_power_vol config.\n");

		for (i = 0; i < count; i++) {
			data->power[i] = regulator_get(dev, data->power_name[i]);
			if (IS_ERR_OR_NULL(data->power[i]))
				return -ENOMEM;

			dev_info(dev, "bt power[%d] (%s) voltage: %dmV\n",
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
			dev_info(dev, "bt clock[%d] (%s)\n", i, data->clk_name[i]);
		}
	} else {
		if (count > CLK_MAX) {
			dev_warn(dev, "bt clocks count large than max(%d > %d).\n",
						count, CLK_MAX);
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
			dev_info(dev, "bt clock[%d] (%s)\n", i, data->clk_name[i]);
		}
	}

	data->gpio_bt_rst = of_get_named_gpio_flags(np, "bt_rst_n", 0, &config);
	if (!gpio_is_valid(data->gpio_bt_rst)) {
		dev_err(dev, "get gpio bt_rst failed\n");
	} else {
		data->gpio_bt_rst_assert = (config == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		dev_info(dev, "bt_rst gpio=%d assert=%d\n", data->gpio_bt_rst, data->gpio_bt_rst_assert);

		ret = devm_gpio_request(dev, data->gpio_bt_rst, "bt_rst");
		if (ret < 0) {
			dev_err(dev, "can't request bt_rst gpio %d\n",
				data->gpio_bt_rst);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_bt_rst, data->gpio_bt_rst_assert);
		if (ret < 0) {
			dev_err(dev, "can't request output direction bt_rst gpio %d\n",
				data->gpio_bt_rst);
			return ret;
		}
	}

	data->rfkill = rfkill_alloc("sunxi-bt", dev, RFKILL_TYPE_BLUETOOTH,
				&sunxi_bt_rfkill_ops, data);
	if (!data->rfkill)
		return -ENOMEM;

	rfkill_set_states(data->rfkill, true, false);

	ret = rfkill_register(data->rfkill);
	if (ret)
		goto fail_rfkill;

	data->power_state = 0;
	bluetooth_data = data;
	return 0;

fail_rfkill:
	if (data->rfkill)
		rfkill_destroy(data->rfkill);

	return ret;
}

int sunxi_bt_deinit(struct platform_device *pdev)
{
	struct sunxi_bt_platdata *data = bluetooth_data;
	struct rfkill *rfk;
	int i;

	if (!data)
		return 0;

	rfk = data->rfkill;
	if (rfk) {
		rfkill_unregister(rfk);
		rfkill_destroy(rfk);
	}

	if (data->power_state)
		sunxi_bluetooth_set_power(0);

	for (i = 0; i < PWR_MAX; i++) {
		if (!IS_ERR_OR_NULL(data->power[i]))
			regulator_put(data->power[i]);
	}

	bluetooth_data = NULL;

	return 0;
}

static const struct of_device_id sunxi_bt_ids[] = {
	{ .compatible = "allwinner,sunxi-bt" },
	{ /* Sentinel */ }
};

MODULE_DESCRIPTION("sunxi bluetooth driver");
MODULE_LICENSE("GPL");

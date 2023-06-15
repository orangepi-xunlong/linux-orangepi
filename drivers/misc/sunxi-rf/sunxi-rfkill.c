#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/capability.h>
#include "internal.h"
#include "sunxi-rfkill.h"

#define MODULE_CUR_VERSION   "v1.0.9"

static struct sunxi_rfkill_platdata *rfkill_data;

void rfkill_chipen_set(int dev, int on_off)
{
	/* Only wifi and bt both close, chip_en goes down,
	 * otherwise, set chip_en up to keep module work.
	 * dev   : device to set power status. 0: wifi, 1: bt
	 * on_off: power status to set. 0: off, 1: on
	 */
	static int power_state;
	bool set_val;

	if (!rfkill_data)
		return;

	if (dev == WL_DEV_WIFI || dev == WL_DEV_BLUETOOTH) {
		power_state &= ~(1 << dev);
		power_state |= ((on_off > 0) << dev);
	}

	if (gpio_is_valid(rfkill_data->gpio_chip_en)) {
		set_val = (power_state != 0) ?
					 rfkill_data->gpio_chip_en_assert :
					!rfkill_data->gpio_chip_en_assert;
		gpio_set_value(rfkill_data->gpio_chip_en, set_val);
	}
}

void rfkill_poweren_set(int dev, int on_off)
{
	/* Only wifi and bt both close, power_en goes down,
	 * otherwise, set power_en up to keep module work.
	 * dev   : device to set power status. 0: wifi, 1: bt
	 * on_off: power status to set. 0: off, 1: on
	 */
	static int power_state;
	bool set_val;

	if (!rfkill_data)
		return;

	if (dev == WL_DEV_WIFI || dev == WL_DEV_BLUETOOTH) {
		power_state &= ~(1 << dev);
		power_state |= ((on_off > 0) << dev);
	}

	if (gpio_is_valid(rfkill_data->gpio_power_en)) {
		set_val = (power_state != 0) ?
					 rfkill_data->gpio_power_en_assert :
					!rfkill_data->gpio_power_en_assert;
		gpio_set_value(rfkill_data->gpio_power_en, set_val);
	}
}

static int rfkill_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct sunxi_rfkill_platdata *data;
	enum of_gpio_flags config;
	char *pctrl_name = PINCTRL_STATE_DEFAULT;
	struct pinctrl_state *pctrl_state = NULL;
	int ret = 0;

	if (!dev)
		return -ENOMEM;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);

	dev_info(dev, "module version: %s\n", MODULE_CUR_VERSION);
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

	data->gpio_chip_en = of_get_named_gpio_flags(np, "chip_en", 0, &config);
	if (!gpio_is_valid(data->gpio_chip_en)) {
		dev_err(dev, "get gpio chip_en failed\n");
	} else {
		data->gpio_chip_en_assert = (config == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		dev_info(dev, "chip_en gpio=%d assert=%d\n", data->gpio_chip_en, data->gpio_chip_en_assert);

		ret = devm_gpio_request(dev, data->gpio_chip_en, "chip_en");
		if (ret < 0) {
			dev_err(dev, "can't request chip_en gpio %d\n",
				data->gpio_chip_en);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_chip_en, !data->gpio_chip_en_assert);
		if (ret < 0) {
			dev_err(dev, "can't request output direction chip_en gpio %d\n",
				data->gpio_chip_en);
			return ret;
		}
	}

	data->gpio_power_en = of_get_named_gpio_flags(np, "power_en", 0, &config);
	if (!gpio_is_valid(data->gpio_power_en)) {
		dev_err(dev, "get gpio power_en failed\n");
	} else {
		data->gpio_power_en_assert = (config == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		dev_info(dev, "power_en gpio=%d assert=%d\n", data->gpio_power_en, data->gpio_power_en_assert);

		ret = devm_gpio_request(dev, data->gpio_power_en, "power_en");
		if (ret < 0) {
			dev_err(dev, "can't request power_en gpio %d\n",
				data->gpio_power_en);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_power_en, !data->gpio_power_en);
		if (ret < 0) {
			dev_err(dev, "can't request output direction power_en gpio %d\n",
				data->gpio_power_en);
			return ret;
		}
	}

	ret = sunxi_wlan_init(pdev);
	if (ret)
		goto err_wlan;

	ret = sunxi_bt_init(pdev);
	if (ret)
		goto err_bt;

	ret = sunxi_modem_init(pdev);
	if (ret)
		goto err_modem;

	rfkill_data = data;
	return 0;

err_modem:
	sunxi_bt_deinit(pdev);
err_bt:
	sunxi_wlan_deinit(pdev);
err_wlan:
	return ret;
}

static int rfkill_remove(struct platform_device *pdev)
{
	sunxi_modem_deinit(pdev);
	sunxi_bt_deinit(pdev);
	sunxi_wlan_deinit(pdev);
	rfkill_data = NULL;
	return 0;
}

static const struct of_device_id rfkill_ids[] = {
	{ .compatible = "allwinner,sunxi-rfkill" },
	{ /* Sentinel */ }
};

static struct platform_driver rfkill_driver = {
	.probe  = rfkill_probe,
	.remove = rfkill_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name  = "sunxi-rfkill",
		.of_match_table = rfkill_ids,
	},
};

module_platform_driver_probe(rfkill_driver, rfkill_probe);

MODULE_DESCRIPTION("sunxi rfkill driver");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>

#define LOG(x...) pr_info("[WLAN_RFKILL]: " x)
#define POWER_ON (1)
#define POWER_OFF (0)

struct rfkill_wlan_data {
	const char		*name;
	struct gpio_desc	*vbat_gpio;
	struct gpio_desc	*poweren_gpio;
	struct gpio_desc	*wakehost_gpio;
};
static struct rfkill_wlan_data *g_rfkill = NULL;

int rfkill_set_wifi_soc_power(int on)
{
	struct rfkill_wlan_data *rfkill = g_rfkill;
	int ret = 0;

	if (!rfkill || !rfkill->vbat_gpio) {
		LOG("rfkill or vbat_gpio is NULL\n");
		return -ENODEV;
	}

	if (on) 
		gpiod_set_value(rfkill->vbat_gpio, 1);
	else
		gpiod_set_value(rfkill->vbat_gpio, 0);

	LOG("%s: %s\n", __func__, on? "on" : "off");
	return ret;
}
EXPORT_SYMBOL(rfkill_set_wifi_soc_power);

int rfkill_set_wifi_power(int on)
{
	struct rfkill_wlan_data *rfkill = g_rfkill;
	int ret = 0;

	if (!rfkill || !rfkill->poweren_gpio) {
		LOG("rfkill or poweren_gpio is NULL\n");
		return -ENODEV;
	}

	if (on)
		gpiod_set_value(rfkill->poweren_gpio, 1);
	else
		gpiod_set_value(rfkill->poweren_gpio, 0);

	LOG("%s: %s\n", __func__, on? "on" : "off");
	return ret;
}
EXPORT_SYMBOL(rfkill_set_wifi_power);

static int rfkill_wlan_probe(struct platform_device *pdev)
{
	struct rfkill_wlan_data *rfkill;
	struct gpio_desc *gpio;

	rfkill = devm_kzalloc(&pdev->dev, sizeof(*rfkill), GFP_KERNEL);
	if (!rfkill)
		return -ENOMEM;

	if (!rfkill->name)
		rfkill->name = dev_name(&pdev->dev);

	gpio = devm_gpiod_get_optional(&pdev->dev, "vbat", GPIOD_OUT_HIGH);
	if (IS_ERR(gpio))
		return PTR_ERR(gpio);

	rfkill->vbat_gpio = gpio;

	gpio = devm_gpiod_get_optional(&pdev->dev, "poweren", GPIOD_OUT_HIGH);
	if (IS_ERR(gpio))
		return PTR_ERR(gpio);

	rfkill->poweren_gpio = gpio;

	gpio = devm_gpiod_get_optional(&pdev->dev, "wakehost", GPIOD_OUT_HIGH);
	if (IS_ERR(gpio))
		return PTR_ERR(gpio);

	rfkill->wakehost_gpio = gpio;

	platform_set_drvdata(pdev, rfkill);
	g_rfkill = rfkill;
	rfkill_set_wifi_soc_power(POWER_ON);
	rfkill_set_wifi_power(POWER_ON);

	LOG("%s: Inited\n", __func__);

	return 0;

}

static int rfkill_wlan_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id wlan_platdata_of_match[] = {
	{ .compatible = "wlan-rfkill" },
	{}
};
MODULE_DEVICE_TABLE(of, wlan_platdata_of_match);

static struct platform_driver rfkill_gpio_driver = {
	.probe = rfkill_wlan_probe,
	.remove = rfkill_wlan_remove,
	.driver = {
		.name = "rfkill_wlan",
		.of_match_table = of_match_ptr(wlan_platdata_of_match),
	},
};

module_platform_driver(rfkill_gpio_driver);

MODULE_DESCRIPTION("cix rfkill for wifi");
MODULE_AUTHOR("lihua.liu@cixtech.com");
MODULE_LICENSE("GPL");
/* SPDX-License-Identifier: GPL-2.0-or-later */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "sunxi_gpio_vbus_power.h"

struct sunxi_gpio_power {
	char				*name;
	struct device			*dev;
	struct power_supply		*gpio_vbus_supply;
	int					usb_det_gpio;
	int 				usb_det_gpio_irq;
	struct delayed_work		gpio_vbus_supply_mon;
};

static enum power_supply_property sunxi_gpio_vbus_props[] = {
	/* real_time */
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_STATUS,
	/* static */
	POWER_SUPPLY_PROP_MODEL_NAME,
};

static int sunxi_gpio_vbus_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct sunxi_gpio_power *gpio_power = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = gpio_get_value(gpio_power->usb_det_gpio);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = gpio_get_value(gpio_power->usb_det_gpio);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		if (gpio_get_value(gpio_power->usb_det_gpio))
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	default:
		break;
	}

	return 0;
}

static irqreturn_t sunxi_gpio_vbus_irq_handler(int irq, void *data)
{
	struct sunxi_gpio_power *gpio_power = data;

	power_supply_changed(gpio_power->gpio_vbus_supply);
	return IRQ_HANDLED;
}

static const struct power_supply_desc sunxi_gpio_vbus_desc = {
	.name = "sunxi-gpio-vbus",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.get_property = sunxi_gpio_vbus_get_property,
	.properties = sunxi_gpio_vbus_props,
	.num_properties = ARRAY_SIZE(sunxi_gpio_vbus_props),
};

static void sunxi_gpio_power_monitor(struct work_struct *work)
{
	struct sunxi_gpio_power *gpio_power =
		container_of(work, typeof(*gpio_power), gpio_vbus_supply_mon.work);
	schedule_delayed_work(&gpio_power->gpio_vbus_supply_mon, msecs_to_jiffies(1 * 1000));
}

static int sunxi_gpio_power_gpio_check(struct sunxi_gpio_power *gpio_power, int gpio_no, const char *name)
{
	int ret = 0;
	if (!gpio_is_valid(gpio_no)) {
		PMIC_WARN("sunxi_gpio power gpio[%d]:%s is no valid\n", gpio_no, name);
		return -EPROBE_DEFER;
	}
	ret = devm_gpio_request(gpio_power->dev, gpio_no, name);
	if (ret != 0) {
		PMIC_WARN("sunxi_gpio_power gpio[%d]:%s request failed\n", gpio_no, name);
		return -EPROBE_DEFER;
	}
	return ret;
}

static int sunxi_gpio_power_gpio_init(struct sunxi_gpio_power *gpio_power)
{
	int ret = 0;

	/* set charge irq pin */
	ret = of_get_named_gpio(gpio_power->dev->of_node, "usb_det_gpio", 0);

	if (ret < 0) {
		PMIC_INFO("no gpio interrupts:%d", ret);
	} else {
		gpio_power->usb_det_gpio = ret;
		ret = sunxi_gpio_power_gpio_check(gpio_power, gpio_power->usb_det_gpio, "sunxi_usb_det_gpio");
		if (ret < 0) {
			return ret;
		}
		gpio_direction_input(gpio_power->usb_det_gpio);
		/* irq config setting */
		gpio_power->usb_det_gpio_irq = gpio_to_irq(gpio_power->usb_det_gpio);
	}

	PMIC_DEBUG("eta6973_gpio_int:%d irq:%d", gpio_power->usb_det_gpio, gpio_power->usb_det_gpio_irq);

	return 0;
}

static int sunxi_gpio_vbus_init_chip(struct sunxi_gpio_power *gpio_power)
{
	int ret = 0;

	ret = sunxi_gpio_power_gpio_init(gpio_power);
	if (ret != 0) {
		PMIC_INFO("gpio init failed\n");
		return ret;
	}

	return 0;
}

static int sunxi_gpio_power_probe(struct platform_device *pdev)
{
	struct power_supply_config psy_cfg = {};
	struct sunxi_gpio_power *gpio_power;
	int ret = 0;

	PMIC_INFO("sunxi_gpio_power_probe device probe\n");
	gpio_power = devm_kzalloc(&pdev->dev, sizeof(*gpio_power), GFP_KERNEL);
	if (!gpio_power) {
		pr_err("sunxi_virtual ac power alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}
	gpio_power->name = "sunxi-gpio-power";
	gpio_power->dev = &pdev->dev;

	ret = sunxi_gpio_vbus_init_chip(gpio_power);
	if (ret < 0) {
		PMIC_DEV_ERR(gpio_power->dev, "sunxi gpio vbus init chip fail!\n");
		ret = -ENODEV;
		goto err;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = gpio_power;

	platform_set_drvdata(pdev, gpio_power);
	gpio_power->gpio_vbus_supply = devm_power_supply_register(gpio_power->dev,
			&sunxi_gpio_vbus_desc, &psy_cfg);
	if (IS_ERR(gpio_power->gpio_vbus_supply)) {
		ret = PTR_ERR(gpio_power->gpio_vbus_supply);
		pr_err("failed to register sunxi gpio vbus power:%d\n", ret);
		return ret;
	}

	INIT_DELAYED_WORK(&gpio_power->gpio_vbus_supply_mon, sunxi_gpio_power_monitor);

	if (gpio_power->usb_det_gpio > 0) {
		PMIC_DEBUG("gpio interrupts:%d", gpio_power->usb_det_gpio);
		ret = devm_request_threaded_irq(&pdev->dev, gpio_power->usb_det_gpio_irq, NULL,
										sunxi_gpio_vbus_irq_handler, IRQF_ONESHOT | IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
										"sunxi-gpio-vbus", gpio_power);
		if (ret < 0) {
				PMIC_DEV_ERR(&pdev->dev, "failed to request sunxi gpio vbus IRQ %d: %d\n", gpio_power->usb_det_gpio_irq, ret);
				goto cancel_work;
		}
		if (!of_property_read_bool(pdev->dev.of_node, "wakeup-source")) {
			PMIC_INFO("wakeup source is disabled!\n");
		} else {
			ret = device_init_wakeup(&pdev->dev, true);
			if (ret < 0) {
					PMIC_DEV_ERR(&pdev->dev, "failed to init wake IRQ %d: %d\n", gpio_power->usb_det_gpio_irq, ret);
					goto cancel_work;
			}
			ret = dev_pm_set_wake_irq(&pdev->dev, gpio_power->usb_det_gpio_irq);
			if (ret < 0) {
					PMIC_DEV_ERR(&pdev->dev, "failed to set wake IRQ %d: %d\n", gpio_power->usb_det_gpio_irq, ret);
					goto cancel_work;
			}
		}
	}

	schedule_delayed_work(&gpio_power->gpio_vbus_supply_mon, msecs_to_jiffies(500));

	platform_set_drvdata(pdev, gpio_power);

	return ret;

cancel_work:
	cancel_delayed_work_sync(&gpio_power->gpio_vbus_supply_mon);

err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
	return 0;
}


static int sunxi_gpio_power_remove(struct platform_device *pdev)
{
	struct sunxi_gpio_power *gpio_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&gpio_power->gpio_vbus_supply_mon);

	PMIC_DEV_DEBUG(&pdev->dev, "==============sunxi gpio power unegister==============\n");
	if (gpio_power->gpio_vbus_supply)
		power_supply_unregister(gpio_power->gpio_vbus_supply);
	PMIC_DEV_DEBUG(&pdev->dev, "sunxi gpio power teardown dev\n");

	return 0;
}

static void sunxi_gpio_power_shutdown(struct platform_device *pdev)
{
	struct sunxi_gpio_power *gpio_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&gpio_power->gpio_vbus_supply_mon);

}

static int sunxi_gpio_power_suspend(struct platform_device *p, pm_message_t state)
{
	struct sunxi_gpio_power *gpio_power = platform_get_drvdata(p);

	cancel_delayed_work_sync(&gpio_power->gpio_vbus_supply_mon);

	return 0;
}

static int sunxi_gpio_power_resume(struct platform_device *p)
{
	struct sunxi_gpio_power *gpio_power = platform_get_drvdata(p);

	schedule_delayed_work(&gpio_power->gpio_vbus_supply_mon, 0);

	return 0;
}

static const struct of_device_id sunxi_gpio_power_match[] = {
	{
		.compatible = "sunxi-gpio-vbus-supply",
	}, { /* sentinel */ }
};

static struct platform_driver sunxi_gpio_power_driver = {
	.driver = {
		.name = "sunxi-gpio-vbus-supply",
		.of_match_table = sunxi_gpio_power_match,
	},
	.probe = sunxi_gpio_power_probe,
	.remove = sunxi_gpio_power_remove,
	.shutdown = sunxi_gpio_power_shutdown,
	.suspend = sunxi_gpio_power_suspend,
	.resume = sunxi_gpio_power_resume,
};

module_platform_driver(sunxi_gpio_power_driver);
MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi-gpio-vbus-supply driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

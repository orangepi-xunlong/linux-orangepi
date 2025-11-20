/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "sunxi_usb_power.h"

static enum power_supply_property sunxi_usb_power_props[] = {
/*------------------------------
 * real time props
 *------------------------------
 */
	/* main */
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,

	/* input ctrl */
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,

	/* other props */
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_SCOPE,

/*------------------------------
 * static props
 *------------------------------
 */
	/* main */
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
};


/*------------------------------
 * check supply online status
 *------------------------------
 */

/**
 * sunxi_usb_power_is_vbus_det_gpio_online - Check VBUS detection GPIO status
 * @usb_power: Pointer to USB power supply data structure
 *
 * Return: true if VBUS detection GPIO is high, false otherwise
 */

static bool sunxi_usb_power_is_vbus_det_gpio_online(struct sunxi_usb_power_supply_data *usb_power)
{
	int ret = 0;

	ret = gpio_get_value(usb_power->sunxi_usb_power_vbus_det.gpio);
	if (ret < 0) {
		dev_err(usb_power->dev, "Failed to get VBUS detect GPIO value: %d\n", ret);
		return false;
	}

	return ret ? true : false;
}

/**
 * sunxi_usb_power_is_vbus_online - Determine VBUS online status
 * @usb_power: Pointer to USB power supply data structure
 *
 * This function checks VBUS status using one of these methods:
 * 1. GPIO detection (when configured)
 * 2. Default USB power supply status (fallback)
 *
 * The detection method is determined by usb_power->vbus_detect_type
 *
 * Return: true if VBUS is detected online, false otherwise
 */

static bool sunxi_usb_power_is_vbus_online(struct sunxi_usb_power_supply_data *usb_power)
{
	union power_supply_propval val;
	struct power_supply *usb_psy = usb_power->usb_power_psy;
	bool online = 0;
	int ret = 0;

	switch (usb_power->vbus_detect_type) {
	case DETEC_BY_GPIO:
		online = sunxi_usb_power_is_vbus_det_gpio_online(usb_power);
		break;
	default:
		ret = power_supply_get_property(usb_psy, POWER_SUPPLY_PROP_ONLINE, &val);
		if (ret < 0)
			val.intval = 0;
		online = val.intval;
		break;
	}

	PMIC_DEBUG("%s online status is :%d\n", __func__, online);

	return online;
}

int sunxi_usb_power_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sunxi_usb_power_supply_data *usb_power = power_supply_get_drvdata(psy);
	struct power_supply *usb_psy = usb_power->usb_power_psy;
	int ret = 0;


	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		fallthrough;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = sunxi_usb_power_is_vbus_online(usb_power);
		break;
	default:
		ret = power_supply_get_property(usb_psy, psp, val);
		break;
	}

	if (ret < 0) {
		val->intval = 0;
		ret = 0;
	}

	return ret;
}

int sunxi_usb_power_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sunxi_usb_power_supply_data *usb_power = power_supply_get_drvdata(psy);
	struct power_supply *usb_psy = usb_power->usb_power_psy;
	int ret = 0;

	ret = power_supply_set_property(usb_psy, psp, val);

	return ret;
}

static int sunxi_usb_power_property_is_writeable(struct power_supply *psy,
				 enum power_supply_property psp)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = 1;
		break;
	default:
		ret = 0;
	}
	return ret;
}

static struct power_supply_desc sunxi_usb_power_desc = {
	.name = "sunxi-usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.get_property = sunxi_usb_power_get_property,
	.set_property = sunxi_usb_power_set_property,
	.properties = sunxi_usb_power_props,
	.num_properties = ARRAY_SIZE(sunxi_usb_power_props),
	.property_is_writeable = sunxi_usb_power_property_is_writeable,
};

/*---------------------------------------------
 * delay work process
 *---------------------------------------------
 */

static void sunxi_usb_power_monitor(struct work_struct *work)
{
	struct sunxi_usb_power_supply_data *usb_power =
		container_of(work, typeof(*usb_power), usb_power_supply_mon.work);

	schedule_delayed_work(&usb_power->usb_power_supply_mon, msecs_to_jiffies(500));
}

/*---------------------------------------------
 * vbus irq handler & vbus det notify & extcon notify
 *---------------------------------------------
 */

static irqreturn_t sunxi_usb_power_vbus_gpio_det_isr(int irq, void *data)
{
	struct sunxi_usb_power_supply_data *usb_power = data;
	struct power_supply *usb_psy = usb_power->usb_power_psy;
	union power_supply_propval val;

	/* type-c setting*/
	if (!sunxi_usb_power_is_vbus_online(usb_power)) {
		val.intval = POWER_SUPPLY_SCOPE_UNKNOWN;
		power_supply_set_property(usb_psy, POWER_SUPPLY_PROP_SCOPE, &val);
	}

	power_supply_changed(usb_power->usb_power_core_psy);

	return IRQ_HANDLED;
}

static int sunxi_usb_power_vbus_online_det_notify(struct notifier_block *nb, unsigned long val, void *v)
{
	struct sunxi_usb_power_supply_data *usb_power = container_of(nb, struct sunxi_usb_power_supply_data, psy_nb);
	struct power_supply *usb_psy = usb_power->usb_power_psy;
	struct power_supply *psy = v;

	if ((val == PSY_EVENT_PROP_CHANGED) && (psy == usb_psy)) {
		power_supply_changed(usb_power->usb_power_core_psy);
	}

	return NOTIFY_OK;
}

/*---------------------------------------------
 * init notify
 *---------------------------------------------
 */

static int sunxi_usb_power_vbus_online_det_notify_init(struct sunxi_usb_power_supply_data *usb_power)
{
	int ret = 0;

	usb_power->psy_nb.notifier_call = sunxi_usb_power_vbus_online_det_notify;
	usb_power->psy_nb.priority = 0;
	ret = power_supply_reg_notifier(&usb_power->psy_nb);
	if (ret < 0) {
		PMIC_ERR("failed to register notifier :%d\n", ret);
		return ret;
	}

	return ret;
}

/*---------------------------------------------
 * init by node paras
 *---------------------------------------------
 */

static int sunxi_usb_power_parse_device_tree(struct sunxi_usb_power_supply_data *usb_power)
{
	struct device_node *np = NULL;
	struct power_supply *psy = NULL;
	int ret = 0;

	np = of_parse_phandle(usb_power->dev->of_node, "det_usb_supply", 0);
	if (np) {
		if (of_device_is_available(np)) {
			PMIC_INFO("usb power device is enabled\n");
			psy = devm_power_supply_get_by_phandle(usb_power->dev, "det_usb_supply");
			if (IS_ERR_OR_NULL(psy)) {
				PMIC_ERR("usb power device is not ready\n");
				return -EPROBE_DEFER;
			}
		} else {
			PMIC_INFO("usb power device is not enabled\n");
			return -ENODEV;
		}
	} else {
		PMIC_INFO("usb power device is not configed\n");
		return -ENODEV;
	}

	usb_power->usb_power_psy = psy;

	return ret;
}

/*---------------------------------------------
 * init detection type & delay work
 *---------------------------------------------
 */

static void sunxi_usb_power_set_detection_type(struct sunxi_usb_power_supply_data *usb_power,
						int detect_type)
{
	char *type_name;

	usb_power->vbus_detect_type = detect_type;
	switch (detect_type) {
	/* input ctrl */
	case DETEC_BY_GPIO:
		type_name = "gpio";
		break;
	default:
		type_name = "normal ways";
		break;
	}
	PMIC_INFO("usb detect by %s\n", type_name);
}

static int sunxi_usb_power_gpio_det_init_common(struct sunxi_usb_power_supply_data *usb_power,
						const char *gpio_name,
						const char *irq_name)
{
	struct sunxi_usb_power_gpio_para gpio_para;
	unsigned long irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
							 IRQF_ONESHOT | IRQF_NO_SUSPEND;
	struct power_supply *psy = usb_power->usb_power_psy;
	int ret;

	memset(&gpio_para, 0, sizeof(gpio_para));

	gpio_para.gpio = of_get_named_gpio(psy->of_node, gpio_name, 0);
	if (gpio_para.gpio < 0) {
		PMIC_INFO("%s not detected\n", gpio_name);
		return 0;
	}

	if (!gpio_is_valid(gpio_para.gpio)) {
		PMIC_ERR("Invalid %s GPIO\n", gpio_name);
		return -EPROBE_DEFER;
	}

	ret = gpio_request(gpio_para.gpio, irq_name);
	if (ret) {
		PMIC_ERR("%s request failed\n", irq_name);
		return -EINVAL;
	}
	gpio_direction_input(gpio_para.gpio);

	gpio_para.irq_num = gpio_to_irq(gpio_para.gpio);
	ret = devm_request_threaded_irq(usb_power->dev, gpio_para.irq_num, NULL,
								  sunxi_usb_power_vbus_gpio_det_isr, irq_flags,
								  irq_name, usb_power);
	if (ret) {
		PMIC_ERR("%s IRQ failed, err %d\n", irq_name, ret);
		return -EINVAL;
	}

	sunxi_usb_power_set_detection_type(usb_power, DETEC_BY_GPIO);
	usb_power->sunxi_usb_power_vbus_det = gpio_para;

	PMIC_DEBUG("%s IRQ success: %d\n", irq_name, ret);
	return 0;
}

static int sunxi_usb_power_vbus_gpio_det_init(struct sunxi_usb_power_supply_data *usb_power)
{
	return sunxi_usb_power_gpio_det_init_common(usb_power,
						"pmu_vbus_det_gpio",
						"pmu_vbus_det_gpio");
}

static int sunxi_usb_power_detect_type_init(struct sunxi_usb_power_supply_data *usb_power)
{
	int ret;

	usb_power->vbus_detect_type = DETEC_UNKNOWN;

	/* Try GPIO detection first */
	ret = sunxi_usb_power_vbus_gpio_det_init(usb_power);
	if (!ret && usb_power->vbus_detect_type == DETEC_BY_GPIO) {
		return 0;
	}

	/* Default to VBUS detection */
	sunxi_usb_power_set_detection_type(usb_power, DETEC_BY_VBUS);

	return 0;
}

static int sunxi_usb_power_init_paras_late(struct sunxi_usb_power_supply_data *usb_power)
{
	int ret = 0;

	INIT_DELAYED_WORK(&usb_power->usb_power_supply_mon, sunxi_usb_power_monitor);

	ret = sunxi_usb_power_detect_type_init(usb_power);
	if (ret < 0) {
		return ret;
	}

	ret = sunxi_usb_power_vbus_online_det_notify_init(usb_power);
	if (ret < 0) {
		return ret;
	}

	schedule_delayed_work(&usb_power->usb_power_supply_mon, 500);

	return ret;
}

static int sunxi_usb_power_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct sunxi_usb_power_supply_data *usb_power;
	struct power_supply_config psy_cfg = {};
	struct device_node *node = pdev->dev.of_node;

	if (!of_device_is_available(node)) {
		PMIC_ERR("sunxi_usb device is not configed\n");
		return -ENODEV;
	}

	usb_power = devm_kzalloc(&pdev->dev, sizeof(*usb_power), GFP_KERNEL);
	if (usb_power == NULL) {
		PMIC_ERR("sunxi_usb alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	usb_power->name = "sunxi_usb";
	usb_power->dev = &pdev->dev;

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = usb_power;

	ret = sunxi_usb_power_parse_device_tree(usb_power);
	if (ret < 0) {
		goto err;
	}

	usb_power->usb_power_core_psy = devm_power_supply_register(usb_power->dev,
			&sunxi_usb_power_desc, &psy_cfg);

	if (IS_ERR(usb_power->usb_power_core_psy)) {
		PMIC_ERR("failed to register sunxi usb power\n");
		ret = PTR_ERR(usb_power->usb_power_core_psy);
		goto err;
	}

	platform_set_drvdata(pdev, usb_power);

	ret = sunxi_usb_power_init_paras_late(usb_power);
	if (ret < 0) {
		goto err;
	}

	return ret;
err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static inline void sunxi_usb_power_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void sunxi_usb_power_virq_node_set(struct sunxi_usb_power_supply_data *usb_power, bool enable)
{
	if (usb_power->vbus_detect_type == DETEC_BY_GPIO) {
		sunxi_usb_power_irq_set(usb_power->sunxi_usb_power_vbus_det.irq_num,
				enable);
	}
}

static void sunxi_usb_power_delayed_work_set(struct sunxi_usb_power_supply_data *usb_power, bool enable)
{
	if (enable) {
		schedule_delayed_work(&usb_power->usb_power_supply_mon, 0);
	} else {
		cancel_delayed_work_sync(&usb_power->usb_power_supply_mon);
	}
}

static int sunxi_usb_power_remove(struct platform_device *pdev)
{
	struct sunxi_usb_power_supply_data *usb_power = platform_get_drvdata(pdev);

	PMIC_DEV_DEBUG(&pdev->dev, "==============sunxi usb power unegister==============\n");
	if (usb_power->usb_power_core_psy) {
		sunxi_usb_power_virq_node_set(usb_power, false);
		sunxi_usb_power_delayed_work_set(usb_power, false);
		power_supply_unregister(usb_power->usb_power_core_psy);
	}
	PMIC_DEV_DEBUG(&pdev->dev, "teardown sunxi usb power dev\n");

	return 0;
}

static void sunxi_usb_power_shutdown(struct platform_device *pdev)
{
	struct sunxi_usb_power_supply_data *usb_power = platform_get_drvdata(pdev);

	sunxi_usb_power_virq_node_set(usb_power, false);
	sunxi_usb_power_delayed_work_set(usb_power, false);
}

static int sunxi_usb_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sunxi_usb_power_supply_data *usb_power = platform_get_drvdata(pdev);

	sunxi_usb_power_virq_node_set(usb_power, false);
	sunxi_usb_power_delayed_work_set(usb_power, false);

	return 0;
}

static int sunxi_usb_power_resume(struct platform_device *pdev)
{
	struct sunxi_usb_power_supply_data *usb_power = platform_get_drvdata(pdev);

	sunxi_usb_power_delayed_work_set(usb_power, true);
	sunxi_usb_power_virq_node_set(usb_power, true);

	return 0;
}

static const struct of_device_id sunxi_usb_power_match[] = {
	{
		.compatible = "x-powers,sunxi-usb-power-supply",
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, sunxi_usb_power_match);

static struct platform_driver sunxi_usb_power_driver = {
	.driver = {
		.name = "sunxi-usb-power-supply",
		.of_match_table = sunxi_usb_power_match,
	},
	.probe = sunxi_usb_power_probe,
	.remove = sunxi_usb_power_remove,
	.shutdown = sunxi_usb_power_shutdown,
	.suspend = sunxi_usb_power_suspend,
	.resume = sunxi_usb_power_resume,
};

module_platform_driver(sunxi_usb_power_driver);

MODULE_VERSION("1.0.1");
MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi usb power driver");
MODULE_LICENSE("GPL");

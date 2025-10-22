/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs pmic driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp515_charger.h"

struct axp515_acin_power {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct power_supply       *acin_supply;
	struct axp_config_info    dts_info;
	struct delayed_work       acin_supply_mon;
	struct delayed_work       acin_chg_state;

	/* acin_detect_type */
	int                       acin_detect_type;

	/* acin_gpio_config */
	struct axp_gpio_para      axp_acin_det;

	/* acin_extcon */
	struct extcon_dev         *acin_edev;
	struct notifier_block	  acin_nb;
	atomic_t                  acin_online;

	/* usb_power_supply */
	bool                      usb_supply_exist;
	struct power_supply       *usb_psy;
};


static enum power_supply_property axp515_acin_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_ONLINE,
};

enum axp515_acin_detect_type {
	DETEC_UNKNOWN = 0,
	DETEC_BY_GPIO,
	DETEC_BY_EXTCON,
};

static void axp515_init_usb_para(struct axp515_acin_power *acin_power)
{
	struct device_node *np = NULL;
	struct axp_config_info *axp_config = &acin_power->dts_info;
	int paras = -1;

	np = of_parse_phandle(acin_power->dev->of_node, "det_usb_supply", 0);
	if (!of_device_is_available(np)) {
		PMIC_ERR("axp515 acin-sypply need usb power, but is not available\n");
		return;
	}

	acin_power->usb_supply_exist = true;

	of_property_read_u32(np, "pmu_usbad_cur", &paras);
	if (paras > 0)
		axp_config->pmu_usbad_cur = paras;
	else
		axp_config->pmu_usbad_cur = 2500;

	of_property_read_u32(np, "pmu_usbpc_cur", &paras);
	if (paras > 0)
		axp_config->pmu_usbpc_cur = paras;
	else
		axp_config->pmu_usbpc_cur = 500;

	return;
}

static int axp515_check_usb(struct axp515_acin_power *acin_power)
{
	if (!acin_power->usb_supply_exist)
		return 0;

	if (acin_power->usb_psy == NULL) {
		acin_power->usb_psy = devm_power_supply_get_by_phandle(acin_power->dev,
									"det_usb_supply");
	}
	if (!(acin_power->usb_psy && (!IS_ERR(acin_power->usb_psy))))
		return 0;

	return 1;
}

static int axp515_acin_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct axp515_acin_power *acin_power = power_supply_get_drvdata(psy);

	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		switch (acin_power->acin_detect_type) {
		case DETEC_BY_GPIO:
			val->intval = gpio_get_value(acin_power->axp_acin_det.gpio);
			break;
		case DETEC_BY_EXTCON:
			val->intval = atomic_read(&acin_power->acin_online);
			break;
		default:
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct power_supply_desc axp515_acin_desc = {
	.name = "axp515-acin",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.get_property = axp515_acin_get_property,
	.properties = axp515_acin_props,
	.num_properties = ARRAY_SIZE(axp515_acin_props),
};

int axp515_irq_handler_acin_in(void *data)
{
	struct axp515_acin_power *acin_power = data;
	struct axp_config_info *axp_config = &acin_power->dts_info;
	union power_supply_propval temp;

	PMIC_INFO("[acin_irq]axp515_acin_in\n");
	power_supply_changed(acin_power->acin_supply);
	if (axp515_check_usb(acin_power)) {
		temp.intval = axp_config->pmu_usbad_cur;
		PMIC_INFO("[acin_irq] ad_current_limit = %d\n", temp.intval);
		power_supply_set_property(acin_power->usb_psy,
						POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &temp);
	}
	return 0;
}

int axp515_irq_handler_acin_out(void *data)
{
	struct axp515_acin_power *acin_power = data;
	struct axp_config_info *axp_config = &acin_power->dts_info;
	union power_supply_propval temp;

	PMIC_INFO("[acout_irq]axp515_acin_out\n");
	power_supply_changed(acin_power->acin_supply);
	if (axp515_check_usb(acin_power)) {
		power_supply_get_property(acin_power->usb_psy, POWER_SUPPLY_PROP_ONLINE, &temp);
		if (temp.intval) {
			power_supply_get_property(acin_power->usb_psy, POWER_SUPPLY_PROP_USB_TYPE, &temp);
			if (temp.intval == POWER_SUPPLY_USB_TYPE_SDP) {
				temp.intval = axp_config->pmu_usbpc_cur;
				PMIC_INFO("[acout_irq] pc_current_limit = %d\n", temp.intval);
				power_supply_set_property(acin_power->usb_psy,
							POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &temp);
			}
		}
	}
	return 0;
}

static irqreturn_t axp_acin_isr(int irq, void *data)
{
	struct axp515_acin_power *acin_power = data;

	cancel_delayed_work_sync(&acin_power->acin_chg_state);
	schedule_delayed_work(&acin_power->acin_chg_state, 1);

	return IRQ_HANDLED;
}

static void axp515_acin_power_set_current_fsm(struct work_struct *work)
{
	struct axp515_acin_power *acin_power =
		container_of(work, typeof(*acin_power), acin_chg_state.work);
	int acin_det_status_value;
	static int acin_det_status_value_old;

	if (acin_power->acin_detect_type == DETEC_BY_GPIO) {
		acin_det_status_value = gpio_get_value(acin_power->axp_acin_det.gpio);
	} else if (acin_power->acin_detect_type == DETEC_BY_EXTCON) {
		acin_det_status_value = atomic_read(&acin_power->acin_online);
	} else {
		return;
	}

	if (acin_det_status_value_old == acin_det_status_value) {
		return;
	} else {
		acin_det_status_value_old = acin_det_status_value;
	}

	PMIC_INFO("[ac_status] ac_in_flag :%d\n", acin_det_status_value);
	if (acin_det_status_value == 1) {
		axp515_irq_handler_acin_in(acin_power);
	} else {
		axp515_irq_handler_acin_out(acin_power);
	}
}

static int axp_acin_gpio_init(struct axp515_acin_power *acin_power)
{
	int ret = 0;
	unsigned long irq_flags = 0;

	if (!gpio_is_valid(acin_power->axp_acin_det.gpio)) {
		PMIC_WARN("get pmu_acin_det_gpio is fail\n");
		return -EPROBE_DEFER;
	}

	ret = gpio_request(
			acin_power->axp_acin_det.gpio,
			"pmu_acin_det_gpio");
	if (ret != 0) {
		PMIC_WARN("pmu_acin_det gpio_request failed\n");
		return -EPROBE_DEFER;
	}
	/* set acin input */
	gpio_direction_input(acin_power->axp_acin_det.gpio);

	/* irq config setting */
	irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
			IRQF_ONESHOT | IRQF_NO_SUSPEND;
	acin_power->axp_acin_det.irq_num = gpio_to_irq(acin_power->axp_acin_det.gpio);

	/* init delay work */
	acin_power->acin_detect_type = DETEC_BY_GPIO;
	ret = devm_request_threaded_irq(acin_power->dev, acin_power->axp_acin_det.irq_num, NULL, axp_acin_isr, irq_flags,
				"pmu_acin_det_gpio", acin_power);

	if (IS_ERR_VALUE((unsigned long)ret)) {
		PMIC_ERR("Requested pmu_acin_det_gpio IRQ failed, err %d\n", ret);
		acin_power->acin_detect_type = DETEC_UNKNOWN;
		return -EINVAL;
	}
	PMIC_DEV_DEBUG(acin_power->dev, "Requested pmu_acin_det_gpio IRQ %d: %d\n",
		acin_power->axp_acin_det.irq_num, ret);

	return 0;
}

static int axp515_acin_extcon_notifier(struct notifier_block *nb,
	unsigned long event, void *ptr)
{
	struct axp515_acin_power *acin_power = container_of(nb, struct axp515_acin_power, acin_nb);

	PMIC_INFO("acin event %lu\n", event);
	if (event) {
		atomic_set(&acin_power->acin_online, 1);
	} else {
		atomic_set(&acin_power->acin_online, 0);
	}
	schedule_delayed_work(&acin_power->acin_chg_state, 1);

	return NOTIFY_DONE;
}

static int axp515_acin_extcon_init(struct axp515_acin_power *acin_power)
{
	int ret = 0;

	acin_power->acin_nb.notifier_call = axp515_acin_extcon_notifier;
	ret = devm_extcon_register_notifier(acin_power->dev, acin_power->acin_edev,
					EXTCON_CHG_USB_DCP, &acin_power->acin_nb);
	if (ret < 0) {
		PMIC_ERR("failed to register notifier :%d\n", ret);
		return ret;
	}

	acin_power->acin_detect_type = DETEC_BY_EXTCON;
	if (extcon_get_state(acin_power->acin_edev, EXTCON_CHG_USB_DCP) == true) {
		atomic_set(&acin_power->acin_online, 1);
		axp515_irq_handler_acin_in(acin_power);
	} else {
		atomic_set(&acin_power->acin_online, 0);
		axp515_irq_handler_acin_out(acin_power);
	}

	return ret;
}

static int axp515_acin_detect_type_init(struct axp515_acin_power *acin_power)
{
	struct extcon_dev *edev;
	int ret = 0;

	acin_power->acin_detect_type = DETEC_UNKNOWN;

	/* detect by gpio */
	ret = of_get_named_gpio(acin_power->dev->of_node, "pmu_acin_det_gpio", 0);
	PMIC_INFO("%s:%d ret: %d\n", __func__, __LINE__, ret);

	if (ret < 0) {
		acin_power->axp_acin_det.gpio = 0;
	} else {
		acin_power->axp_acin_det.gpio = ret;
		ret = axp_acin_gpio_init(acin_power);
		if (ret != 0) {
			PMIC_ERR("axp acin init failed\n");
			return ret;
		}
		PMIC_INFO("axp515 acin detect by gpio\n");
		return 0;
	}

	/* detect by extcon */
	if (of_property_read_bool(acin_power->dev->of_node, "extcon")) {
		edev = extcon_get_edev_by_phandle(acin_power->dev, 0);
		if (IS_ERR_OR_NULL(edev)) {
			PMIC_ERR("couldn't get extcon device\n");
			return -EPROBE_DEFER;
		}
		acin_power->acin_edev = edev;
		ret = axp515_acin_extcon_init(acin_power);
		if (ret != 0) {
			PMIC_ERR("axp extcon acin init failed\n");
			return ret;
		}
		PMIC_INFO("axp515 acin detect by extcon\n");
		return 0;
	}

	return -EINVAL;
}

static void axp515_acin_power_monitor(struct work_struct *work)
{
	struct axp515_acin_power *acin_power =
		container_of(work, typeof(*acin_power), acin_supply_mon.work);

	schedule_delayed_work(&acin_power->acin_supply_mon, msecs_to_jiffies(1000));
}

int axp515_acin_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	return 0;
}

static void axp515_acin_parse_device_tree(struct axp515_acin_power *acin_power)
{
	int ret;
	struct axp_config_info *cfg;

	if (!acin_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	cfg = &acin_power->dts_info;
	ret = axp515_acin_dt_parse(acin_power->dev->of_node, cfg);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}
}

static int axp515_acin_probe(struct platform_device *pdev)
{
	struct axp515_acin_power *acin_power;
	struct device_node *node = pdev->dev.of_node;
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	int ret = 0;

	if (!of_device_is_available(node)) {
		PMIC_ERR("axp515-acin device is not configed\n");
		return -ENODEV;
	}

	if (!axp_dev->irq) {
		PMIC_ERR("can not register axp515-acin without irq\n");
		return -EINVAL;
	}

	acin_power = devm_kzalloc(&pdev->dev, sizeof(*acin_power), GFP_KERNEL);
	if (acin_power == NULL) {
		PMIC_ERR("axp515_acin_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	acin_power->name = "axp515_acin";
	acin_power->dev = &pdev->dev;
	acin_power->regmap = axp_dev->regmap;

	/* parse device tree and set register */
	axp515_acin_parse_device_tree(acin_power);

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = acin_power;

	acin_power->acin_supply = devm_power_supply_register(acin_power->dev,
			&axp515_acin_desc, &psy_cfg);

	if (IS_ERR(acin_power->acin_supply)) {
		PMIC_ERR("axp515 failed to register acin power-sypply\n");
		ret = PTR_ERR(acin_power->acin_supply);
		return ret;
	}

	acin_power->usb_psy = NULL;
	acin_power->usb_supply_exist = false;
	if (of_find_property(acin_power->dev->of_node, "det_usb_supply", NULL)) {
		axp515_init_usb_para(acin_power);
	} else {
		PMIC_ERR("axp515 acin-sypply failed to find usb power\n");
	}

	INIT_DELAYED_WORK(&acin_power->acin_supply_mon, axp515_acin_power_monitor);
	INIT_DELAYED_WORK(&acin_power->acin_chg_state, axp515_acin_power_set_current_fsm);

	ret = axp515_acin_detect_type_init(acin_power);
	if (ret != 0) {
		PMIC_ERR("axp515 acin-sypply failed to init detect type\n");
		return ret;
	}

	schedule_delayed_work(&acin_power->acin_supply_mon, msecs_to_jiffies(1000));
	platform_set_drvdata(pdev, acin_power);
	PMIC_INFO("axp515_acin_probe finish: %s , %d \n", __func__, __LINE__);

	return ret;

err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp515_acin_remove(struct platform_device *pdev)
{
	struct axp515_acin_power *acin_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&acin_power->acin_supply_mon);
	cancel_delayed_work_sync(&acin_power->acin_chg_state);

	PMIC_DEV_DEBUG(&pdev->dev, "==============AXP515 acin unegister==============\n");
	if (acin_power->acin_supply)
		power_supply_unregister(acin_power->acin_supply);
	PMIC_DEV_DEBUG(&pdev->dev, "axp515 teardown acin dev\n");

	return 0;
}

static inline void axp515_acin_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp515_acin_virq_dts_set(struct axp515_acin_power *acin_power, bool enable)
{
	if (acin_power->acin_detect_type == DETEC_BY_GPIO) {
		axp515_acin_irq_set(acin_power->axp_acin_det.irq_num,
				enable);
	}
}

static void axp515_acin_shutdown(struct platform_device *pdev)
{
	struct axp515_acin_power *acin_power = platform_get_drvdata(pdev);

	axp515_acin_virq_dts_set(acin_power, false);

	cancel_delayed_work_sync(&acin_power->acin_supply_mon);
	cancel_delayed_work_sync(&acin_power->acin_chg_state);
}

static int axp515_acin_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp515_acin_power *acin_power = platform_get_drvdata(pdev);

	axp515_acin_virq_dts_set(acin_power, false);

	cancel_delayed_work_sync(&acin_power->acin_supply_mon);
	cancel_delayed_work_sync(&acin_power->acin_chg_state);

	return 0;
}

static int axp515_acin_resume(struct platform_device *pdev)
{
	struct axp515_acin_power *acin_power = platform_get_drvdata(pdev);

	schedule_delayed_work(&acin_power->acin_supply_mon, 0);
	schedule_delayed_work(&acin_power->acin_chg_state, 0);

	axp515_acin_virq_dts_set(acin_power, true);

	return 0;
}

static const struct of_device_id axp515_acin_power_match[] = {
	{
		.compatible = "x-powers,axp515-acin-power-supply",
		.data = (void *)AXP515_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp515_acin_power_match);

static struct platform_driver axp515_acin_power_driver = {
	.driver = {
		.name = "axp515-acin-power-supply",
		.of_match_table = axp515_acin_power_match,
	},
	.probe = axp515_acin_probe,
	.remove = axp515_acin_remove,
	.shutdown = axp515_acin_shutdown,
	.suspend = axp515_acin_suspend,
	.resume = axp515_acin_resume,
};

module_platform_driver(axp515_acin_power_driver);

MODULE_VERSION("1.0.0");
MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("axp515 acin driver");
MODULE_LICENSE("GPL");

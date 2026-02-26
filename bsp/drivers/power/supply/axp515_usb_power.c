/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp515_charger.h"

struct axp515_usb_power {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct power_supply       *usb_supply;
	struct axp_config_info    dts_info;
	struct delayed_work       usb_supply_mon;

	/* power debugfs */
	struct sunxi_power_debug_data	*debug;
};

static enum power_supply_property axp515_usb_props[] = {
	/* real_time */
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_TEMP,
	/* static */
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
};

ATOMIC_NOTIFIER_HEAD(usb_power_notifier_list);
EXPORT_SYMBOL(usb_power_notifier_list);

static int axp515_get_vbus_state(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp515_usb_power *usb_supply = power_supply_get_drvdata(ps);
	struct regmap *regmap = usb_supply->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP515_STATUS0, &data);
	if (ret < 0)
		return ret;

	val->intval = !!(data & AXP515_MASK_VBUS_STAT);
	return 0;
}

static int _axp515_get_iin_limit(struct axp515_usb_power *usb_supply)
{
	struct regmap *regmap = usb_supply->regmap;
	unsigned int data;
	int limit_cur, ret = 0;

	ret = regmap_read(regmap, AXP515_ILIMIT, &data);
	if (ret < 0)
		return ret;
	data &= 0x3F;

	limit_cur = data * 50 + 100;

	return limit_cur;
}

static int axp515_get_iin_limit(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(ps);
	int limit_cur;

	limit_cur = _axp515_get_iin_limit(usb_power);
	if (limit_cur < 0)
		return limit_cur;

	val->intval = limit_cur;

	return 0;
}

static int axp515_get_vindpm(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = usb_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP515_RBFET_SET, &data);
	if (ret < 0)
		return ret;

	data = (data * 80) + 3880;
	val->intval = data;

	return 0;
}

/* read temperature */
static inline int axp_vts_to_temp(u16 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 10625 / 10000 - 2677;
}

static inline int axp515_get_adc_temp(struct regmap *regmap)
{
	unsigned char temp_val[2];
	unsigned short ts_res;
	int die_temp;
	int ret = 0;

	ret = regmap_bulk_read(regmap, AXP515_DIE_TEMP, temp_val, 2);
	if (ret < 0)
		return ret;

	ts_res = ((unsigned short) temp_val[0] << 8) | temp_val[1];
	die_temp = axp_vts_to_temp(ts_res);

	return die_temp;
}

static int axp515_get_ic_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = usb_power->regmap;

	int i = 0, temp, old_temp;

	old_temp = axp515_get_adc_temp(regmap);

	/* read until abs(old_temp - temp) < 10*/
	temp = axp515_get_adc_temp(regmap);

	PMIC_DEBUG("old_temp:%d, temp:%d\n", old_temp, temp);
	while ((abs(old_temp - temp) > 100) && (i < 10)) {
		old_temp = temp;
		temp = axp515_get_adc_temp(regmap);
		i++;
		PMIC_DEBUG("turn[%d]:old_temp:%d, temp:%d\n", i, old_temp, temp);
	}

	val->intval = temp;

	return 0;
}


static int axp515_set_iin_limit(struct regmap *regmap, int mA)
{
	unsigned int data;
	int ret = 0, reg_val;

	data = mA;
	if (data > 3250)
		data = 3250;
	if	(data < 100)
		data = 100;

	regmap_read(regmap, AXP515_DPM_LOOP_SET, &reg_val);
	if ((data < 3000) && (reg_val & BIT(4)))
		regmap_update_bits(regmap, AXP515_DPM_LOOP_SET, BIT(4), 0);
	else if  ((data >= 3000) && (!(reg_val & BIT(4))))
		regmap_update_bits(regmap, AXP515_DPM_LOOP_SET, BIT(4), BIT(4));

	data = ((data - 100) / 50);
	ret = regmap_update_bits(regmap, AXP515_ILIMIT, GENMASK(5, 0),
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp515_set_vindpm(struct regmap *regmap, int mV)
{
	unsigned int data;
	int ret = 0;

	data = mV;

	if (data > 5080)
		data = 5080;
	if	(data < 3880)
		data = 3880;
	data = ((data - 3880) / 80);
	ret = regmap_update_bits(regmap, AXP515_RBFET_SET, GENMASK(3, 0),
				 data);
	if (ret < 0)
		return ret;
	return 0;
}

static int axp515_usb_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = axp515_get_vbus_state(psy, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = axp515_get_vbus_state(psy, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = axp515_get_iin_limit(psy, val);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp515_get_ic_temp(psy, val);
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP515_MANUFACTURER;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		ret = axp515_get_vindpm(psy, val);
		break;
	default:
		break;
	}

	return ret;
}

static int axp515_usb_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(psy);
	struct regmap *regmap = usb_power->regmap;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		SUNXI_POWER_LOG_INFO(usb_power->debug, "set_iin_limit: %d mA", val->intval);
		if (!val->intval)
			break;
		ret = axp515_set_iin_limit(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		SUNXI_POWER_LOG_INFO(usb_power->debug, "set_vindpm: %d mV", val->intval);
		ret = axp515_set_vindpm(regmap, val->intval);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int axp515_usb_power_property_is_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;

}

static const struct power_supply_desc axp515_usb_desc = {
	.name = "axp515-usb",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.get_property = axp515_usb_get_property,
	.properties = axp515_usb_props,
	.set_property = axp515_usb_set_property,
	.num_properties = ARRAY_SIZE(axp515_usb_props),
	.property_is_writeable = axp515_usb_power_property_is_writeable,
};

static irqreturn_t axp515_irq_handler_usb_in(int irq, void *data)
{
	struct axp515_usb_power *usb_power = data;

	mdelay(50);

	SUNXI_POWER_LOG_INFO(usb_power->debug, "Vbus status in");
	atomic_notifier_call_chain(&usb_power_notifier_list, 1, NULL);
	power_supply_changed(usb_power->usb_supply);

	return IRQ_HANDLED;
}

static irqreturn_t axp515_irq_handler_usb_out(int irq, void *data)
{
	struct axp515_usb_power *usb_power = data;

	SUNXI_POWER_LOG_INFO(usb_power->debug, "Vbus status out");
	atomic_notifier_call_chain(&usb_power_notifier_list, 1, NULL);
	power_supply_changed(usb_power->usb_supply);

	return IRQ_HANDLED;
}

enum axp515_usb_virq_index {
	AXP515_VIRQ_USB_IN,
	AXP515_VIRQ_USB_OUT,
	AXP515_USB_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_usb_irq[] = {
	[AXP515_VIRQ_USB_IN] = { "usb in", axp515_irq_handler_usb_in },
	[AXP515_VIRQ_USB_OUT] = { "usb out", axp515_irq_handler_usb_out },
};

static void axp515_usb_power_monitor(struct work_struct *work)
{
	struct axp515_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_supply_mon.work);

	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));
}

static void axp515_usb_power_init(struct axp515_usb_power *usb_power)
{
	struct regmap *regmap = usb_power->regmap;
	struct axp_config_info *dinfo = &usb_power->dts_info;

	/* set vindpm value */
	axp515_set_vindpm(regmap, dinfo->pmu_usbad_vol);

	/* enable die adc */
	regmap_update_bits(regmap, AXP515_ADC_CONTROL, AXP515_ADC_DIETMP_ENABLE, AXP515_ADC_DIETMP_ENABLE);

	/* set bc12 en/disable  */
	if (dinfo->pmu_bc12_en) {
		regmap_update_bits(regmap, AXP515_AUTO_SETS, BIT(6), BIT(6));
	} else {
		regmap_update_bits(regmap, AXP515_AUTO_SETS, BIT(6), 0);
	}
}

static int axp515_usb_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		PMIC_ERR("%s: failed\n", __func__);
		return -1;
	}


	AXP_OF_PROP_READ(pmu_usbpc_vol,                    4600);
	AXP_OF_PROP_READ(pmu_usbpc_cur,                    500);
	AXP_OF_PROP_READ(pmu_usbad_vol,                    4600);
	AXP_OF_PROP_READ(pmu_usbad_cur,                    1500);
	AXP_OF_PROP_READ(pmu_bc12_en,                         0);
	AXP_OF_PROP_READ(pmu_usb_typec_used,                  1);

	axp_config->wakeup_usb_in =
		of_property_read_bool(node, "wakeup_usb_in");
	axp_config->wakeup_usb_out =
		of_property_read_bool(node, "wakeup_usb_out");
	axp_config->wakeup_typec_in =
		of_property_read_bool(node, "wakeup_typec_in");
	axp_config->wakeup_typec_out =
		of_property_read_bool(node, "wakeup_typec_out");

	return 0;
}

static void axp515_usb_parse_device_tree(struct axp515_usb_power *usb_power)
{
	int ret;
	struct axp_config_info *cfg;

	/* set input current limit */
	if (!usb_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	cfg = &usb_power->dts_info;
	ret = axp515_usb_dt_parse(usb_power->dev->of_node, cfg);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}

	/*init axp515 usb by device tree*/
	axp515_usb_power_init(usb_power);
}

static int axp515_usb_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;

	struct axp515_usb_power *usb_power;

	struct sunxi_power_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};

	if (!axp_dev->irq) {
		PMIC_ERR("can not register axp515-usb without irq\n");
		return -EINVAL;
	}

	usb_power = devm_kzalloc(&pdev->dev, sizeof(*usb_power), GFP_KERNEL);
	if (usb_power == NULL) {
		PMIC_ERR("axp515_usb_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	usb_power->name = "axp515_usb";
	usb_power->dev = &pdev->dev;
	usb_power->regmap = axp_dev->regmap;

	/* parse device tree and set register */
	axp515_usb_parse_device_tree(usb_power);

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = usb_power;

	usb_power->usb_supply = devm_power_supply_register(usb_power->dev,
			&axp515_usb_desc, &psy_cfg);

	if (IS_ERR(usb_power->usb_supply)) {
		PMIC_ERR("axp515 failed to register usb power\n");
		ret = PTR_ERR(usb_power->usb_supply);
		return ret;
	}

	INIT_DELAYED_WORK(&usb_power->usb_supply_mon, axp515_usb_power_monitor);

	for (i = 0; i < ARRAY_SIZE(axp_usb_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_usb_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			PMIC_DEV_ERR(&pdev->dev, "can not get irq\n");
			ret = irq;
			goto cancel_work;
		}
		/* we use this variable to suspend irq */
		axp_usb_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_usb_irq[i].isr, 0,
						   axp_usb_irq[i].name, usb_power);
		if (ret < 0) {
			PMIC_DEV_ERR(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_usb_irq[i].name, irq, ret);
			goto cancel_work;
		} else {
			ret = 0;
		}

		PMIC_DEV_DEBUG(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_usb_irq[i].name, irq, ret);
	}

	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));

	platform_set_drvdata(pdev, usb_power);

	usb_power->debug = sunxi_power_debugfs_init(&pdev->dev);
	if (IS_ERR_OR_NULL(usb_power->debug))
		dev_warn(&pdev->dev, "Failed to init debugfs\n");

	SUNXI_POWER_LOG_INFO(usb_power->debug, "USB power driver initialized");

	return ret;

cancel_work:
	cancel_delayed_work_sync(&usb_power->usb_supply_mon);


err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp515_usb_remove(struct platform_device *pdev)
{
	struct axp515_usb_power *usb_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);

	PMIC_DEV_DEBUG(&pdev->dev, "==============AXP515 usb unegister==============\n");
	if (usb_power->usb_supply)
		power_supply_unregister(usb_power->usb_supply);
	sunxi_power_debugfs_exit(usb_power->debug);
	PMIC_DEV_DEBUG(&pdev->dev, "axp515 teardown usb dev\n");

	return 0;
}

static inline void axp515_usb_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp515_usb_virq_dts_set(struct axp515_usb_power *usb_power, bool enable)
{
	struct axp_config_info *dts_info = &usb_power->dts_info;

	if (!dts_info->wakeup_usb_in)
		axp515_usb_irq_set(axp_usb_irq[AXP515_VIRQ_USB_IN].irq,
				enable);

	if (!dts_info->wakeup_usb_out)
		axp515_usb_irq_set(axp_usb_irq[AXP515_VIRQ_USB_OUT].irq,
				enable);
}

static void axp515_usb_shutdown(struct platform_device *pdev)
{
	struct axp515_usb_power *usb_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);
}

static int axp515_usb_suspend(struct platform_device *p, pm_message_t state)
{
	struct axp515_usb_power *usb_power = platform_get_drvdata(p);

	axp515_usb_virq_dts_set(usb_power, false);

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);

	return 0;
}

static int axp515_usb_resume(struct platform_device *p)
{
	struct axp515_usb_power *usb_power = platform_get_drvdata(p);

	schedule_delayed_work(&usb_power->usb_supply_mon, 0);

	axp515_usb_virq_dts_set(usb_power, true);

	return 0;
}

static const struct of_device_id axp515_usb_power_match[] = {
	{
		.compatible = "x-powers,axp515-usb-power-supply",
		.data = (void *)AXP515_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp515_usb_power_match);

static struct platform_driver axp515_usb_power_driver = {
	.driver = {
		.name = "axp515-usb-power-supply",
		.of_match_table = axp515_usb_power_match,
	},
	.probe = axp515_usb_probe,
	.remove = axp515_usb_remove,
	.shutdown = axp515_usb_shutdown,
	.suspend = axp515_usb_suspend,
	.resume = axp515_usb_resume,
};

module_platform_driver(axp515_usb_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp515 usb driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.3");

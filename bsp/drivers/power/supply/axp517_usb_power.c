/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp517_charger.h"

struct axp517_usb_power {
	/* base */
	char				*name;
	struct device			*dev;
	struct regmap			*regmap;
	struct power_supply		*usb_supply;
	struct axp_config_info		dts_info;
	struct delayed_work		usb_supply_mon;

	/* input limit */
	atomic_t			vbus_online_status;
	atomic_t			set_input_vol;

	/* usb notifier */
	struct notifier_block		usb_nb;

	/* power debugfs */
	struct sunxi_power_debug_data	*debug;
};

static enum power_supply_property axp517_usb_props[] = {
	/* real_time */
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_TEMP,
	/* static */
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
};

static int _axp517_get_vbus_state(struct regmap *regmap)
{
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP517_STATUS0, &data);
	if (ret < 0)
		return ret;

	ret = !!(data & AXP517_MASK_VBUS_STAT);

	return ret;
}

static int axp517_get_vbus_state(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp517_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct axp_config_info *dinfo = &usb_power->dts_info;
	struct regmap *regmap = usb_power->regmap;
	int ret = 0;

	if (dinfo->pmu_vbus_det_typec)
		ret = atomic_read(&usb_power->vbus_online_status);
	else
		ret = _axp517_get_vbus_state(regmap);

	if (ret < 0)
		return ret;

	val->intval = ret;
	return 0;
}

static int _axp517_get_iin_limit(struct axp517_usb_power *usb_supply)
{
	struct regmap *regmap = usb_supply->regmap;
	unsigned int data;
	int limit_cur, ret = 0;

	ret = regmap_read(regmap, AXP517_MODULE_EN, &data);
	if (ret < 0)
		return ret;

	if (!(data & BIT(3)))
		return 0;

	ret = regmap_read(regmap, AXP517_IIN_LIM, &data);
	if (ret < 0)
		return ret;

	data &= ~(0x3);

	limit_cur = (data >> 2) * 50 + 100;

	return limit_cur;
}

static int axp517_get_iin_limit(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp517_usb_power *usb_supply = power_supply_get_drvdata(ps);
	int limit_cur;

	limit_cur = _axp517_get_iin_limit(usb_supply);
	if (limit_cur < 0)
		return limit_cur;

	val->intval = limit_cur;

	return 0;
}

static int axp517_get_vindpm(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp517_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = usb_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP517_VINDPM_CFG, &data);
	if (ret < 0)
		return ret;

	data &= ~(0x80);
	if (!data)
		data = 1;

	data = ((data - 1) * 100) + 3600;
	val->intval = data;

	return 0;
}

/* read temperature */
static inline int axp517_tdie_to_temp(u32 reg)
{
	return (721 * 5 - ((int)(reg & 0xFFF))) * 100 / 191 / 5;
}

static inline int axp517_get_tdie_adc_temp(struct regmap *regmap)
{
	unsigned char temp_val[2];
	unsigned int reg_value;
	u32 ts_res;
	int die_temp;
	int ret = 0;

	ret = regmap_read(regmap, AXP517_ADC_CONTROL, &reg_value);
	if (ret < 0)
		return ret;

	if ((reg_value & 0xf) != AXP517_ADC_TDIE) {
		reg_value &= ~(0xf);
		reg_value |= AXP517_ADC_TDIE;
		ret = regmap_write(regmap, AXP517_ADC_CONTROL, reg_value);
		if (ret < 0)
			return ret;
		mdelay(1);
	}

	ret = regmap_bulk_read(regmap, AXP517_ADC_RES, temp_val, 2);
	if (ret < 0)
		return ret;

	temp_val[0] &= GENMASK(5, 0);
	ts_res = (temp_val[0] << 4) | (temp_val[1] & 0x0F);
	die_temp = axp517_tdie_to_temp(ts_res);

	return die_temp;
}

static int axp517_get_ic_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp517_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = usb_power->regmap;

	int i = 0, temp, old_temp;

	old_temp = axp517_get_tdie_adc_temp(regmap);

	/* read until abs(old_temp - temp) < 10*/
	temp = axp517_get_tdie_adc_temp(regmap);

	PMIC_DEBUG("old_temp:%d, temp:%d\n", old_temp, temp);
	while ((abs(old_temp - temp) > 100) && (i < 10)) {
		old_temp = temp;
		temp = axp517_get_tdie_adc_temp(regmap);
		i++;
		PMIC_DEBUG("turn[%d]:old_temp:%d, temp:%d\n", i, old_temp, temp);
	}

	val->intval = temp;

	return 0;
}

static int axp517_set_iin_limit(struct regmap *regmap, int mA)
{
	unsigned int data;
	int ret = 0;

	PMIC_DEBUG("%s: %d limit: %d\n", __func__, __LINE__, mA);

	data = mA;

	if (data < 100) {
		ret = regmap_update_bits(regmap, AXP517_MODULE_EN, BIT(3), 0);

		if (ret < 0)
			return ret;
	} else {
		if (data > 3250)
			data = 3250;

		data = ((data - 100) / 50) << 2;

		ret = regmap_update_bits(regmap, AXP517_IIN_LIM, GENMASK(7, 0),
					 data);

		if (ret < 0)
			return ret;

		ret = regmap_update_bits(regmap, AXP517_MODULE_EN, BIT(3), BIT(3));

		if (ret < 0)
			return ret;
	}

	return 0;
}

static int axp517_set_vindpm(struct regmap *regmap, int mV)
{
	unsigned int data;
	int ret = 0;

	data = mV;

	if (data > 16200)
		data = 16200;
	if	(data < 3600)
		data = 3600;

	data = ((data - 3600) / 100) + 1;
	ret = regmap_update_bits(regmap, AXP517_VINDPM_CFG, GENMASK(6, 0),
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp517_usb_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct axp517_usb_power *usb_power = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = axp517_get_vbus_state(psy, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = axp517_get_vbus_state(psy, val);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = atomic_read(&usb_power->set_input_vol);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = axp517_get_iin_limit(psy, val);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp517_get_ic_temp(psy, val);
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP517_MANUFACTURER;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		ret = axp517_get_vindpm(psy, val);
		break;
	default:
		break;
	}

	return ret;
}

static int axp517_usb_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp517_usb_power *usb_power = power_supply_get_drvdata(psy);
	struct axp_config_info *dinfo = &usb_power->dts_info;

	struct regmap *regmap = usb_power->regmap;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		SUNXI_POWER_LOG_INFO(usb_power->debug, "set input vol: %d mV", val->intval);
		if (!val->intval)
			break;
		atomic_set(&usb_power->set_input_vol, val->intval);
		if (val->intval >= 9000)
			axp517_set_vindpm(regmap, 5500);
		else
			axp517_set_vindpm(regmap, dinfo->pmu_usbad_vol);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		SUNXI_POWER_LOG_INFO(usb_power->debug, "set_iin_limit: %d mA", val->intval);
		ret = axp517_set_iin_limit(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		SUNXI_POWER_LOG_INFO(usb_power->debug, "set_vindpm: %d mV", val->intval);
		ret = axp517_set_vindpm(regmap, val->intval);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int axp517_usb_power_property_is_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_USB_TYPE:
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;

}

static const struct power_supply_desc axp517_usb_desc = {
	.name = "axp517-usb",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.get_property = axp517_usb_get_property,
	.properties = axp517_usb_props,
	.set_property = axp517_usb_set_property,
	.num_properties = ARRAY_SIZE(axp517_usb_props),
	.property_is_writeable = axp517_usb_power_property_is_writeable,
};

static irqreturn_t axp517_irq_handler_vbus_status(int irq, void *data)
{
	struct axp517_usb_power *usb_power = data;

	SUNXI_POWER_LOG_INFO(usb_power->debug, "Vbus status changes");

	power_supply_changed(usb_power->usb_supply);

	return IRQ_HANDLED;
}

enum axp517_usb_virq_index {
	AXP517_VIRQ_USB_IN,
	AXP517_VIRQ_USB_OUT,

	AXP517_USB_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_usb_irq[] = {
	[AXP517_VIRQ_USB_IN] = { "vbus_insert", axp517_irq_handler_vbus_status },
	[AXP517_VIRQ_USB_OUT] = { "vbus_remove", axp517_irq_handler_vbus_status },
};

static void axp517_usb_power_monitor(struct work_struct *work)
{
	struct axp517_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_supply_mon.work);

	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));
}

static int axp517_usb_power_notifier(struct notifier_block *nb, unsigned long event, void *data)
{
	struct axp517_usb_power *usb_power = container_of(nb, struct axp517_usb_power, usb_nb);
	int ret = 0, status;

	PMIC_DEBUG("notifier event %lu\n", event);

	switch (event) {
	case AW_PSY_EVENT_VBUS_ONLINE_CHECK:
		status = *(bool *)data;
		atomic_set(&usb_power->vbus_online_status, status);
		PMIC_INFO("vbus_online_check notify status:%d\n", status);
		mdelay(30);
		power_supply_changed(usb_power->usb_supply);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return NOTIFY_DONE;
}

static int axp517_usb_power_notifier_init(struct axp517_usb_power *usb_power)
{
	int ret = 0;

	usb_power->usb_nb.notifier_call = axp517_usb_power_notifier;
	usb_power->usb_nb.priority = 0;
	ret = sunxi_power_supply_reg_notifier(&usb_power->usb_nb);
	if (ret < 0) {
		PMIC_ERR("failed to register sunxi notifier :%d\n", ret);
		return ret;
	}

	return ret;
}

static void axp517_usb_power_init(struct axp517_usb_power *usb_power)
{
	struct regmap *regmap = usb_power->regmap;
	struct axp_config_info *dinfo = &usb_power->dts_info;

	/* set default value */
	atomic_set(&usb_power->vbus_online_status, 0);

	/* enable die adc */
	regmap_update_bits(regmap, AXP517_ADC_CH_EN0, BIT(4), BIT(4));

	if (dinfo->pmu_vbus_det_typec)
		axp517_usb_power_notifier_init(usb_power);
}

static int axp517_usb_dt_parse(struct device_node *node,
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

	axp_config->wakeup_usb_in =
		of_property_read_bool(node, "wakeup_usb_in");
	axp_config->wakeup_usb_out =
		of_property_read_bool(node, "wakeup_usb_out");
	axp_config->pmu_vbus_det_typec =
		of_property_read_bool(node, "pmu_vbus_det_typec");

	return 0;
}

static void axp517_usb_parse_device_tree(struct axp517_usb_power *usb_power)
{
	int ret;
	struct axp_config_info *cfg;

	/* set input current limit */
	if (!usb_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	cfg = &usb_power->dts_info;
	ret = axp517_usb_dt_parse(usb_power->dev->of_node, cfg);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}

	/*init axp517 usb by device tree*/
	axp517_usb_power_init(usb_power);
}

static int axp517_usb_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;

	struct axp517_usb_power *usb_power;

	struct sunxi_power_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};

	if (!axp_dev->irq) {
		PMIC_ERR("can not register axp517-usb without irq\n");
		return -EINVAL;
	}

	usb_power = devm_kzalloc(&pdev->dev, sizeof(*usb_power), GFP_KERNEL);
	if (usb_power == NULL) {
		PMIC_ERR("axp517_usb_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	usb_power->name = "axp517_usb";
	usb_power->dev = &pdev->dev;
	usb_power->regmap = axp_dev->regmap;

	/* parse device tree and set register */
	axp517_usb_parse_device_tree(usb_power);

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = usb_power;

	usb_power->usb_supply = devm_power_supply_register(usb_power->dev,
			&axp517_usb_desc, &psy_cfg);

	if (IS_ERR(usb_power->usb_supply)) {
		PMIC_ERR("axp517 failed to register usb power\n");
		ret = PTR_ERR(usb_power->usb_supply);
		return ret;
	}

	INIT_DELAYED_WORK(&usb_power->usb_supply_mon, axp517_usb_power_monitor);

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

static int axp517_usb_remove(struct platform_device *pdev)
{
	struct axp517_usb_power *usb_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);

	PMIC_DEV_DEBUG(&pdev->dev, "==============AXP517 usb unegister==============\n");
	if (usb_power->usb_supply)
		power_supply_unregister(usb_power->usb_supply);
	sunxi_power_debugfs_exit(usb_power->debug);
	PMIC_DEV_DEBUG(&pdev->dev, "axp517 teardown usb dev\n");

	return 0;
}

static inline void axp517_usb_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp517_usb_virq_dts_set(struct axp517_usb_power *usb_power, bool enable)
{
	struct axp_config_info *dts_info = &usb_power->dts_info;

	if (!dts_info->wakeup_usb_in)
		axp517_usb_irq_set(axp_usb_irq[AXP517_VIRQ_USB_IN].irq,
				enable);

	if (!dts_info->wakeup_usb_out)
		axp517_usb_irq_set(axp_usb_irq[AXP517_VIRQ_USB_OUT].irq,
				enable);
}

static void axp517_usb_shutdown(struct platform_device *pdev)
{
	struct axp517_usb_power *usb_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);
}

static int axp517_usb_suspend(struct platform_device *p, pm_message_t state)
{
	struct axp517_usb_power *usb_power = platform_get_drvdata(p);

	axp517_usb_virq_dts_set(usb_power, false);

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);

	return 0;
}

static int axp517_usb_resume(struct platform_device *p)
{
	struct axp517_usb_power *usb_power = platform_get_drvdata(p);

	schedule_delayed_work(&usb_power->usb_supply_mon, 0);

	axp517_usb_virq_dts_set(usb_power, true);

	return 0;
}

static const struct of_device_id axp517_usb_power_match[] = {
	{
		.compatible = "x-powers,axp517-usb-power-supply",
		.data = (void *)AXP517_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp517_usb_power_match);

static struct platform_driver axp517_usb_power_driver = {
	.driver = {
		.name = "axp517-usb-power-supply",
		.of_match_table = axp517_usb_power_match,
	},
	.probe = axp517_usb_probe,
	.remove = axp517_usb_remove,
	.shutdown = axp517_usb_shutdown,
	.suspend = axp517_usb_suspend,
	.resume = axp517_usb_resume,
};

module_platform_driver(axp517_usb_power_driver);

MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("axp517 usb driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.12");

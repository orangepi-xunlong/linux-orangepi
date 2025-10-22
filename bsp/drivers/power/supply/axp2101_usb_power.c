/* SPDX-License-Identifier: GPL-2.0-or-later */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp2101_charger.h"

/* #define DONOT_Correction */
/* #define POLL_READ */
#define SOC_RISE_INTERVAL (30)
#define POLL_INTERVAL     (1 * HZ)

#define AXP210X_MASK_VBUS_STATE (BIT(5))

struct axp2101_usb_power {
	char                      *name;
	struct device             *dev;
	struct axp_config_info     dts_info;
	struct task_struct        *poll_read;
	struct regmap             *regmap;
	struct power_supply       *usb_supply;
	struct delayed_work        usb_supply_mon;
	struct delayed_work        usb_chg_state;

	atomic_t set_current_limit;
};

static enum power_supply_property axp2101_usb_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
};

static int axp2101_get_vbus_state(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp2101_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = usb_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP2101_COMM_STAT0, &data);
	if (ret < 0)
		return ret;

	/* vbus is good when vbus state set */
	val->intval = !!(data & AXP210X_MASK_VBUS_STATE);

	return ret;
}

static int axp2101_usb_get_ihold(struct axp2101_usb_power *usb_power)
{
	int tmp;
	struct regmap *map = usb_power->regmap;

	regmap_read(map, AXP2101_IIN_LIM, &tmp);
	tmp &= 0x07;
	if (tmp == 0x00)
		return 100;
	else if (tmp == 0x01)
		return 500;
	else if (tmp == 0x02)
		return 900;
	else if (tmp == 0x03)
		return 1000;
	else if (tmp == 0x04)
		return 1500;
	else
		return 2000;

}

static int axp2101_usb_set_ihold(struct axp2101_usb_power *usb_power, int cur)
{
	struct regmap *map = usb_power->regmap;

	if (cur) {
		if (cur < 500)
			regmap_update_bits(map, AXP2101_IIN_LIM, 0x07, 0x00);
		else if (cur < 900)
			regmap_update_bits(map, AXP2101_IIN_LIM, 0x07, 0x01);
		else if (cur < 1000)
			regmap_update_bits(map, AXP2101_IIN_LIM, 0x07, 0x02);
		else if (cur < 1500)
			regmap_update_bits(map, AXP2101_IIN_LIM, 0x07, 0x03);
		else if (cur < 2000)
			regmap_update_bits(map, AXP2101_IIN_LIM, 0x07, 0x04);
		else
			regmap_update_bits(map, AXP2101_IIN_LIM, 0x07, 0x05);
	} else {
		regmap_update_bits(map, AXP2101_IIN_LIM, 0x07, 0x01);
	}
	return 0;
}

static int axp2101_usb_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct axp2101_usb_power *usb_power = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_PRESENT:
		ret = axp2101_get_vbus_state(psy, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = axp2101_usb_get_ihold(usb_power);
		if (ret < 0)
			return ret;
		val->intval = ret;
	default:
		break;
	}

	return ret;
}

static int axp2101_usb_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp2101_usb_power *usb_power = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = axp2101_usb_set_ihold(usb_power, val->intval);
		atomic_set(&usb_power->set_current_limit, 1);
		break;
	default:
		ret = -EINVAL;

	}
	return ret;
}

static int axp2101_usb_power_property_is_writeable(struct power_supply *psy,
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

static const struct power_supply_desc axp2101_usb_desc = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.get_property = axp2101_usb_get_property,
	.properties = axp2101_usb_props,
	.set_property = axp2101_usb_set_property,
	.num_properties = ARRAY_SIZE(axp2101_usb_props),
	.property_is_writeable = axp2101_usb_power_property_is_writeable,
};

static irqreturn_t axp210x_irq_handler_usb_in(int irq, void *data)
{
	struct axp2101_usb_power *usb_power = data;
	struct axp_config_info *axp_config = &usb_power->dts_info;

	power_supply_changed(usb_power->usb_supply);

	axp2101_usb_set_ihold(usb_power, axp_config->pmu_usbpc_cur);
	atomic_set(&usb_power->set_current_limit, 0);

	cancel_delayed_work_sync(&usb_power->usb_chg_state);
	schedule_delayed_work(&usb_power->usb_chg_state, msecs_to_jiffies(5 * 1000));

	return IRQ_HANDLED;
}

static irqreturn_t axp210x_irq_handler_usb_out(int irq, void *data)
{
	struct axp2101_usb_power *usb_power = data;

	power_supply_changed(usb_power->usb_supply);

	return IRQ_HANDLED;
}

enum axp2101_virq_index {
	AXP2101_VIRQ_USB_IN,
	AXP2101_VIRQ_USB_OUT,
};

static struct axp_interrupts axp_usb_irq[] = {
	[AXP2101_VIRQ_USB_IN] = { "usb in", axp210x_irq_handler_usb_in },
	[AXP2101_VIRQ_USB_OUT] = { "usb out", axp210x_irq_handler_usb_out },
};

static uint32_t iin_lim_tbl[] = {100, 500, 900, 1000, 1500, 2000};

static int axp2101_usb_power_init(struct axp2101_usb_power *usb_power)
{
	struct axp_config_info *axp_config = &usb_power->dts_info;

	axp2101_usb_set_ihold(usb_power, axp_config->pmu_usbpc_cur);

	return 0;
}

int axp2101_usb_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_usbpc_vol,                  4400);
	AXP_OF_PROP_READ(pmu_usbad_vol,                  4400);
	AXP_OF_PROP_READ(pmu_usbpc_cur,                     0);
	AXP_OF_PROP_READ(pmu_usbad_cur,                     0);

	axp_config->wakeup_usb_in =
		of_property_read_bool(node, "wakeup_usb_in");
	axp_config->wakeup_usb_out =
		of_property_read_bool(node, "wakeup_usb_out");

	/* axp_config_obj = axp_config; */
	return 0;
}

static void axp2101_usb_parse_device_tree(struct axp2101_usb_power *usb_power)
{
	int ret;
	uint32_t prop = 0, i;
	struct axp_config_info *cfg;

	/* set input current limit */
	if (!usb_power->dev->of_node) {
		pr_info("can not find device tree\n");
		return;
	}

	cfg = &usb_power->dts_info;
	ret = axp2101_usb_dt_parse(usb_power->dev->of_node, cfg);
	if (ret) {
		pr_info("can not parse device tree err\n");
		return;
	}

	prop = usb_power->dts_info.pmu_usbpc_cur;
	if (!of_property_read_u32(usb_power->dev->of_node, "iin_limit", &prop) ||
	    true) {
		for (i = 0; i < ARRAY_SIZE(iin_lim_tbl); i++) {
			if (prop < iin_lim_tbl[i])
				break;
		}

		i = i ? i - 1 : i;

		if (prop > iin_lim_tbl[ARRAY_SIZE(iin_lim_tbl) - 1])
			i = GENMASK(2, 0);

		regmap_update_bits(usb_power->regmap, AXP2101_IIN_LIM, GENMASK(2, 0), i);
	} else {
		axp2101_usb_power_init(usb_power);
	}

}

static void axp2101_usb_set_current_fsm(struct work_struct *work)
{
	struct axp2101_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_chg_state.work);
	struct axp_config_info *axp_config = &usb_power->dts_info;

	if (atomic_read(&usb_power->set_current_limit)) {
		pr_info("current limit setted: usb pc type\n");
	} else {
		axp2101_usb_set_ihold(usb_power, axp_config->pmu_usbad_cur);
		pr_info("current limit not set: usb adapter type\n");
	}
}

static void axp2101_usb_power_monitor(struct work_struct *work)
{
	struct axp2101_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_supply_mon.work);

	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));
}

static int axp2101_usb_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;
	struct axp2101_usb_power *usb_power;
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};

	if (!axp_dev->irq) {
		pr_err("can not register axp2101-charger without irq\n");
		return -EINVAL;
	}

	usb_power = devm_kzalloc(&pdev->dev, sizeof(*usb_power), GFP_KERNEL);
	if (usb_power == NULL) {
		axp210x_err("axp2101_usb_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	usb_power->name = "usb";
	usb_power->dev = &pdev->dev;
	usb_power->regmap = axp_dev->regmap;

	/* for device tree parse */
	axp2101_usb_parse_device_tree(usb_power);

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = usb_power;

	usb_power->usb_supply = devm_power_supply_register(usb_power->dev,
			&axp2101_usb_desc, &psy_cfg);
	if (ret < 0) {
		axp210x_err("axp210x register battery dev fail!\n");
		goto err;
	}

	INIT_DELAYED_WORK(&usb_power->usb_supply_mon, axp2101_usb_power_monitor);
	INIT_DELAYED_WORK(&usb_power->usb_chg_state, axp2101_usb_set_current_fsm);
#if ((defined DONOT_Correction) || (defined POLL_READ))
	usb_power->poll_read = kthread_run(thread_dosomthing, usb_power, "axp2101");

#else
	for (i = 0; i < ARRAY_SIZE(axp_usb_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_usb_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			dev_err(&pdev->dev, "can not get irq\n");
			return irq;
		}
		/* we use this variable to suspend irq */
		axp_usb_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_usb_irq[i].isr, 0,
						   axp_usb_irq[i].name, usb_power);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_usb_irq[i].name, irq, ret);
			return ret;
		} else {
			ret = 0;
		}

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_usb_irq[i].name, irq, ret);
	}
#endif
	platform_set_drvdata(pdev, usb_power);

	schedule_delayed_work(&usb_power->usb_chg_state, msecs_to_jiffies(20 * 1000));
	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(10 * 1000));

	return ret;

err:
	axp210x_err("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp2101_usb_remove(struct platform_device *pdev)
{
	struct axp2101_usb_power *usb_power = platform_get_drvdata(pdev);
#if ((defined DONOT_Correction) || (defined POLL_READ))
	kthread_stop(axp2101_usb_power->poll_read);
#endif

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);
	cancel_delayed_work_sync(&usb_power->usb_chg_state);

	dev_dbg(&pdev->dev, "==============AXP2101 usb unegister==============\n");
	if (usb_power->usb_supply)
		power_supply_unregister(usb_power->usb_supply);
	dev_dbg(&pdev->dev, "axp2101 teardown usb dev\n");

	return 0;
}

static inline void axp2101_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp2101_virq_dts_set(struct axp2101_usb_power *usb_power, bool enable)
{
	struct axp_config_info *dts_info = &usb_power->dts_info;

	if (!dts_info->wakeup_usb_in)
		axp2101_irq_set(axp_usb_irq[AXP2101_VIRQ_USB_IN].irq,
				enable);
	if (!dts_info->wakeup_usb_out)
		axp2101_irq_set(axp_usb_irq[AXP2101_VIRQ_USB_OUT].irq,
				enable);

}

static void axp2101_usb_shutdown(struct platform_device *p)
{
	struct axp2101_usb_power *usb_power = platform_get_drvdata(p);

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);
	cancel_delayed_work_sync(&usb_power->usb_chg_state);
}

static int axp2101_usb_suspend(struct platform_device *p, pm_message_t state)
{
	struct axp2101_usb_power *usb_power = platform_get_drvdata(p);

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);
	cancel_delayed_work_sync(&usb_power->usb_chg_state);

	axp2101_virq_dts_set(usb_power, false);
	return 0;
}

static int axp2101_usb_resume(struct platform_device *p)
{
	struct axp2101_usb_power *usb_power = platform_get_drvdata(p);

	schedule_delayed_work(&usb_power->usb_chg_state, 0);
	schedule_delayed_work(&usb_power->usb_supply_mon, 0);

	axp2101_virq_dts_set(usb_power, true);
	return 0;
}

static const struct of_device_id axp2101_usb_power_match[] = {
	{
		.compatible = "x-powers,axp2101-usb-power-supply",
		.data = (void *)AXP2101_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp2101_usb_power_match);

static struct platform_driver axp2101_usb_power_driver = {
	.driver = {
		.name = "axp2101-usb-power-supply",
		.of_match_table = axp2101_usb_power_match,
	},
	.probe = axp2101_usb_probe,
	.remove = axp2101_usb_remove,
	.shutdown = axp2101_usb_shutdown,
	.suspend = axp2101_usb_suspend,
	.resume = axp2101_usb_resume,
};

module_platform_driver(axp2101_usb_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp210x i2c driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

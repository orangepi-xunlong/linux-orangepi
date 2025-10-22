/* SPDX-License-Identifier: GPL-2.0-or-later */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp22x_charger.h"

struct axp22x_ac_power {
	char                      *name;
	struct device             *dev;
	struct axp_config_info     dts_info;
	struct regmap             *regmap;
	struct power_supply       *ac_supply;
	struct delayed_work        ac_supply_mon;
};


static enum power_supply_property axp22x_ac_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
};

static int axp22x_ac_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = 0;
	unsigned int reg_value;
	struct axp22x_ac_power *ac_power = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = regmap_read(ac_power->regmap, AXP22X_STATUS, &reg_value);
		if (ret)
			return ret;
		val->intval = !!(reg_value & AXP22X_STATUS_AC_PRESENT);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		ret = regmap_read(ac_power->regmap, AXP22X_STATUS, &reg_value);
		if (ret)
			return ret;
		val->intval = !!(reg_value & AXP22X_STATUS_AC_USED);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct power_supply_desc axp22x_ac_desc = {
	.name = "axp22x-ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.get_property = axp22x_ac_get_property,
	.properties = axp22x_ac_props,
	.num_properties = ARRAY_SIZE(axp22x_ac_props),
};

static int axp22x_ac_power_init(struct axp22x_ac_power *ac_power)
{
	return 0;
}

static irqreturn_t axp22x_ac_power_irq(int irq, void *data)
{
	struct axp22x_ac_power *ac_power = data;

	power_supply_changed(ac_power->ac_supply);

	return IRQ_HANDLED;
}

enum axp22x_ac_power_virqs {
	AXP22X_ACIN_PLUGIN,
	AXP22X_ACIN_REMOVAL,

	AXP22X_AC_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp22x_ac_irq[] = {
	[AXP22X_ACIN_PLUGIN] = {"ACIN_PLUGIN", axp22x_ac_power_irq},
	[AXP22X_ACIN_REMOVAL] = {"ACIN_REMOVAL", axp22x_ac_power_irq},
};

static void axp22x_ac_power_monitor(struct work_struct *work)
{
	struct axp22x_ac_power *ac_power =
		container_of(work, typeof(*ac_power), ac_supply_mon.work);

	schedule_delayed_work(&ac_power->ac_supply_mon, msecs_to_jiffies(500));
}

static int axp22x_ac_power_dt_parse(struct axp22x_ac_power *ac_power)
{
	struct axp_config_info *axp_config = &ac_power->dts_info;
	struct device_node *node = ac_power->dev->of_node;

	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_ac_vol,                     4400);
	AXP_OF_PROP_READ(pmu_ac_cur,                        0);

	axp_config->wakeup_ac_in =
		of_property_read_bool(node, "wakeup_ac_in");
	axp_config->wakeup_ac_out =
		of_property_read_bool(node, "wakeup_ac_out");

	return 0;
}

static int axp22x_ac_power_probe(struct platform_device *pdev)
{
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	struct axp22x_ac_power *ac_power;
	int i, irq;
	int ret = 0;

	if (!axp_dev->irq) {
		pr_err("can not register axp22x ac without irq\n");
		return -EINVAL;
	}

	ac_power = devm_kzalloc(&pdev->dev, sizeof(*ac_power), GFP_KERNEL);
	if (!ac_power) {
		pr_err("axp22x ac power alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	ac_power->name = "axp22x-ac-power";
	ac_power->dev = &pdev->dev;
	ac_power->regmap = axp_dev->regmap;

	platform_set_drvdata(pdev, ac_power);

	ret = axp22x_ac_power_dt_parse(ac_power);
	if (ret) {
		pr_err("%s parse device tree err\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = axp22x_ac_power_init(ac_power);
	if (ret < 0) {
		pr_err("axp210x init chip fail!\n");
		ret = -ENODEV;
		return ret;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = ac_power;

	ac_power->ac_supply = devm_power_supply_register(ac_power->dev,
			&axp22x_ac_desc, &psy_cfg);

	if (IS_ERR(ac_power->ac_supply)) {
		pr_err("axp22x failed to register ac power\n");
		ret = PTR_ERR(ac_power->ac_supply);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(axp22x_ac_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp22x_ac_irq[i].name);
		if (irq < 0) {
			dev_warn(&pdev->dev, "No IRQ for %s: %d\n",
				 axp22x_ac_irq[i].name, irq);
			continue;
		}
		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp22x_ac_irq[i].isr, 0,
						   axp22x_ac_irq[i].name, ac_power);
		if (ret < 0)
			dev_warn(&pdev->dev, "Error requesting %s IRQ %d: %d\n",
				axp22x_ac_irq[i].name, irq, ret);

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp22x_ac_irq[i].name, irq, ret);

		/* we use this variable to suspend irq */
		axp22x_ac_irq[i].irq = irq;
	}


	INIT_DELAYED_WORK(&ac_power->ac_supply_mon, axp22x_ac_power_monitor);
	schedule_delayed_work(&ac_power->ac_supply_mon, msecs_to_jiffies(500));

	return 0;
}

static int axp22x_ac_power_remove(struct platform_device *pdev)
{
	struct axp22x_ac_power *ac_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&ac_power->ac_supply_mon);

	return 0;
}

static inline void axp_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp22x_ac_virq_dts_set(struct axp22x_ac_power *ac_power, bool enable)
{
	struct axp_config_info *dts_info = &ac_power->dts_info;

	if (!dts_info->wakeup_usb_in)
		axp_irq_set(axp22x_ac_irq[AXP22X_ACIN_PLUGIN].irq,
				enable);
	if (!dts_info->wakeup_usb_out)
		axp_irq_set(axp22x_ac_irq[AXP22X_ACIN_REMOVAL].irq,
				enable);
}

static void axp22x_ac_power_shutdown(struct platform_device *pdev)
{
	struct axp22x_ac_power *ac_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&ac_power->ac_supply_mon);
}

static int axp22x_ac_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp22x_ac_power *ac_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&ac_power->ac_supply_mon);
	axp22x_ac_virq_dts_set(ac_power, false);

	return 0;
}

static int axp22x_ac_power_resume(struct platform_device *pdev)
{
	struct axp22x_ac_power *ac_power = platform_get_drvdata(pdev);

	schedule_delayed_work(&ac_power->ac_supply_mon, 0);
	axp22x_ac_virq_dts_set(ac_power, true);

	return 0;
}

static const struct of_device_id axp22x_ac_power_match[] = {
	{
		.compatible = "x-powers,axp22x-ac-power-supply",
		.data = (void *)AXP221_ID,
	}, {
		.compatible = "x-powers,axp22x-ac-power-supply",
		.data = (void *)AXP223_ID,
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp22x_ac_power_match);

static struct platform_driver axp22x_ac_power_driver = {
	.driver = {
		.name = "axp22x-ac-power-supply",
		.of_match_table = axp22x_ac_power_match,
	},
	.probe = axp22x_ac_power_probe,
	.remove = axp22x_ac_power_remove,
	.shutdown = axp22x_ac_power_shutdown,
	.suspend = axp22x_ac_power_suspend,
	.resume = axp22x_ac_power_resume,
};

module_platform_driver(axp22x_ac_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp22x ac power driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

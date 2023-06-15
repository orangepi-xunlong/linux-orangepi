#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/irq.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/gpio/consumer.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/err.h>
#include <linux/mfd/axp2101.h>
#include "axp803_charger.h"

struct axp803_ac_power {
	char                      *name;
	struct device             *dev;
	struct axp_config_info     dts_info;
	struct regmap             *regmap;
	struct power_supply       *ac_supply;
	struct delayed_work        ac_supply_mon;
};

static int axp803_ac_set_vhold(struct axp803_ac_power *ac_power, int vol)
{
	unsigned int tmp;
	struct regmap *map = ac_power->regmap;

	if (vol) {
		regmap_update_bits(map, AXP803_CHARGE_AC_SET, 0x40, 0x40);
		if (vol >= 4000 && vol <= 4700) {
			tmp = (vol - 4000)/100;
			regmap_update_bits(map, AXP803_CHARGE_AC_SET,
					0x7 << 3, tmp << 3);
		} else {
			pr_err("set ac limit voltage error, %d mV\n", vol);
		}
	} else {
		regmap_update_bits(map, AXP803_CHARGE_AC_SET, 0x40, 0x00);
	}

	return 0;
}

static int axp803_ac_set_ihold(struct axp803_ac_power *ac_power, int cur)
{
	unsigned int tmp;
	struct regmap *map = ac_power->regmap;

	if (cur) {
		if (cur >= 1500 && cur <= 4000) {
			tmp = (cur - 1500) / 500;
			regmap_update_bits(map, AXP803_CHARGE_AC_SET, 0x7, tmp);
		} else {
			pr_err("set ac limit current error, %d mA\n", cur);
		}
	} else {
		regmap_update_bits(map, AXP803_CHARGE_AC_SET, 0x40, 0x40);
	}

	return 0;
}

static enum power_supply_property axp803_ac_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
};

static int axp803_ac_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = 0;
	unsigned int reg_value;
	struct axp803_ac_power *ac_power = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = regmap_read(ac_power->regmap, AXP803_STATUS, &reg_value);
		if (ret)
			return ret;
		val->intval = !!(reg_value & AXP803_STATUS_AC_PRESENT);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		ret = regmap_read(ac_power->regmap, AXP803_STATUS, &reg_value);
		if (ret)
			return ret;
		val->intval = !!(reg_value & AXP803_STATUS_AC_USED);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct power_supply_desc axp803_ac_desc = {
	.name = "axp803-ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.get_property = axp803_ac_get_property,
	.properties = axp803_ac_props,
	.num_properties = ARRAY_SIZE(axp803_ac_props),
};

static int axp803_ac_power_init(struct axp803_ac_power *ac_power)
{
	struct axp_config_info *axp_config = &ac_power->dts_info;

	axp803_ac_set_vhold(ac_power, axp_config->pmu_ac_vol);
	axp803_ac_set_ihold(ac_power, axp_config->pmu_ac_cur);
	return 0;
}

static irqreturn_t axp803_ac_power_irq(int irq, void *data)
{
	struct axp803_ac_power *ac_power = data;

	power_supply_changed(ac_power->ac_supply);

	return IRQ_HANDLED;
}

enum axp803_ac_power_virqs {
	AXP803_VIRQ_ACIN,
	AXP803_VIRQ_ACRE,

	AXP803_AC_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp803_ac_irq[] = {
	[AXP803_VIRQ_ACIN] = {"ac in", axp803_ac_power_irq},
	[AXP803_VIRQ_ACRE] = {"ac out", axp803_ac_power_irq},
};

static void axp803_ac_power_monitor(struct work_struct *work)
{
	struct axp803_ac_power *ac_power =
		container_of(work, typeof(*ac_power), ac_supply_mon.work);

	schedule_delayed_work(&ac_power->ac_supply_mon, msecs_to_jiffies(500));
}

static int axp803_ac_power_dt_parse(struct axp803_ac_power *ac_power)
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

static int axp803_ac_power_probe(struct platform_device *pdev)
{
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	struct axp803_ac_power *ac_power;
	int i, irq;
	int ret = 0;

	if (!axp_dev->irq) {
		pr_err("can not register axp803 ac without irq\n");
		return -EINVAL;
	}

	ac_power = devm_kzalloc(&pdev->dev, sizeof(*ac_power), GFP_KERNEL);
	if (!ac_power) {
		pr_err("axp803 ac power alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	ac_power->name = "axp803-ac-power";
	ac_power->dev = &pdev->dev;
	ac_power->regmap = axp_dev->regmap;

	platform_set_drvdata(pdev, ac_power);

	ret = axp803_ac_power_dt_parse(ac_power);
	if (ret) {
		pr_err("%s parse device tree err\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = axp803_ac_power_init(ac_power);
	if (ret < 0) {
		pr_err("axp210x init chip fail!\n");
		ret = -ENODEV;
		return ret;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = ac_power;

	ac_power->ac_supply = devm_power_supply_register(ac_power->dev,
			&axp803_ac_desc, &psy_cfg);

	if (IS_ERR(ac_power->ac_supply)) {
		pr_err("axp803 failed to register ac power\n");
		ret = PTR_ERR(ac_power->ac_supply);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(axp803_ac_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp803_ac_irq[i].name);
		if (irq < 0) {
			dev_warn(&pdev->dev, "No IRQ for %s: %d\n",
				 axp803_ac_irq[i].name, irq);
			continue;
		}
		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp803_ac_irq[i].isr, 0,
						   axp803_ac_irq[i].name, ac_power);
		if (ret < 0)
			dev_warn(&pdev->dev, "Error requesting %s IRQ %d: %d\n",
				axp803_ac_irq[i].name, irq, ret);

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp803_ac_irq[i].name, irq, ret);

		/* we use this variable to suspend irq */
		axp803_ac_irq[i].irq = irq;
	}


	INIT_DELAYED_WORK(&ac_power->ac_supply_mon, axp803_ac_power_monitor);
	schedule_delayed_work(&ac_power->ac_supply_mon, msecs_to_jiffies(500));

	return 0;
}

static int axp803_ac_power_remove(struct platform_device *pdev)
{
	struct axp803_ac_power *ac_power = platform_get_drvdata(pdev);

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

static void axp803_ac_virq_dts_set(struct axp803_ac_power *ac_power, bool enable)
{
	struct axp_config_info *dts_info = &ac_power->dts_info;

	if (!dts_info->wakeup_usb_in)
		axp_irq_set(axp803_ac_irq[AXP803_VIRQ_ACIN].irq,
				enable);
	if (!dts_info->wakeup_usb_out)
		axp_irq_set(axp803_ac_irq[AXP803_VIRQ_ACRE].irq,
				enable);
}

static int axp803_ac_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp803_ac_power *ac_power = platform_get_drvdata(pdev);

	axp803_ac_virq_dts_set(ac_power, false);

	return 0;
}

static int axp803_ac_power_resume(struct platform_device *pdev)
{
	struct axp803_ac_power *ac_power = platform_get_drvdata(pdev);

	axp803_ac_virq_dts_set(ac_power, true);

	return 0;
}

static const struct of_device_id axp803_ac_power_match[] = {
	{
		.compatible = "x-powers,axp803-ac-power-supply",
		.data = (void *)AXP803_ID,
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp803_ac_power_match);

static struct platform_driver axp803_ac_power_driver = {
	.driver = {
		.name = "axp803-ac-power-supply",
		.of_match_table = axp803_ac_power_match,
	},
	.probe = axp803_ac_power_probe,
	.remove = axp803_ac_power_remove,
	.suspend = axp803_ac_power_suspend,
	.resume = axp803_ac_power_resume,
};

module_platform_driver(axp803_ac_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp803 ac power driver");
MODULE_LICENSE("GPL");

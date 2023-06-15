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

struct axp803_usb_power {
	char                      *name;
	struct device             *dev;
	struct axp_config_info     dts_info;
	struct regmap             *regmap;
	struct power_supply       *usb_supply;
	struct delayed_work        usb_supply_mon;
	struct delayed_work        usb_chg_state;

	atomic_t set_current_limit;
};

#ifdef CONFIG_AW_AXP_BC_EN
static int axp803_usb_set_ihold(struct axp803_usb_power *usb_power, int cur)
{
	struct regmap *map = usb_power->regmap;

	if (cur) {
		if (cur < 1500)
			regmap_update_bits(map, AXP803_IPS_SET, 0x03, 0x00);
		else if (cur >= 1500 && cur < 2000)
			regmap_update_bits(map, AXP803_IPS_SET, 0x03, 0x01);
		else if (cur >= 2000 && cur < 2500)
			regmap_update_bits(map, AXP803_IPS_SET, 0x03, 0x02);
		else
			regmap_update_bits(map, AXP803_IPS_SET, 0x03, 0x03);
	} else {
		regmap_update_bits(map, AXP803_IPS_SET, 0x03, 0x03);
	}

	return 0;
}

static int axp803_usb_get_ihold(struct axp803_usb_power *usb_power)
{
	unsigned int tmp;
	struct regmap *map = usb_power->regmap;

	regmap_read(map, AXP803_IPS_SET, &tmp);
	tmp = tmp & 0x3;
	if (tmp == 0x0)
		return 900;
	else if (tmp == 0x1)
		return 1500;
	else if (tmp == 0x2)
		return 2000;
	else
		return 2500;
}
#else
static int axp803_usb_set_ihold(struct axp803_usb_power *usb_power, int cur)
{
	struct regmap *map = usb_power->regmap;

	if (cur) {
		if (cur < 500)
			regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x00);
		else if (cur < 900)
			regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x10);
		else if (cur < 1500)
			regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x20);
		else if (cur < 2000)
			regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x30);
		else if (cur < 2500)
			regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x40);
		else if (cur < 3000)
			regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x50);
		else if (cur < 3500)
			regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x60);
		else if (cur < 4000)
			regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x70);
		else
			regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x80);
	} else {
		regmap_update_bits(map, AXP803_CHARGE3, 0xf0, 0x30);
	}

	return 0;
}

static int axp803_usb_get_ihold(struct axp803_usb_power *usb_power)
{
	unsigned int tmp;
	struct regmap *map = usb_power->regmap;

	regmap_read(map, AXP803_CHARGE3, &tmp);
	tmp = tmp & 0xf0;
	if (tmp == 0x00)
		return 100;
	else if (tmp == 0x10)
		return 500;
	else if (tmp == 0x20)
		return 900;
	else if (tmp == 0x30)
		return 1500;
	else if (tmp == 0x40)
		return 2000;
	else if (tmp == 0x50)
		return 2500;
	else if (tmp == 0x60)
		return 3000;
	else if (tmp == 0x70)
		return 3500;
	else
		return 4000;
}
#endif

static void axp803_usb_set_current_fsm(struct work_struct *work)
{
	struct axp803_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_chg_state.work);
	struct axp_config_info *axp_config = &usb_power->dts_info;

	if (atomic_read(&usb_power->set_current_limit)) {
		pr_info("current limit setted: usb pc type\n");
	} else {
		axp803_usb_set_ihold(usb_power, axp_config->pmu_usbad_cur);
		pr_info("current limit not set: usb adapter type\n");
	}
}

static enum power_supply_property axp803_usb_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
};

static int axp803_usb_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = 0;
	unsigned int reg_value;
	struct axp803_usb_power *usb_power = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = regmap_read(usb_power->regmap, AXP803_STATUS, &reg_value);
		if (ret)
			return ret;
		val->intval = !!(reg_value & AXP803_STATUS_VBUS_PRESENT);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		ret = regmap_read(usb_power->regmap, AXP803_STATUS, &reg_value);
		if (ret)
			return ret;
		val->intval = !!(reg_value & AXP803_STATUS_VBUS_USED);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = axp803_usb_get_ihold(usb_power);
		if (ret < 0)
			return ret;
		val->intval = ret;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int axp803_usb_set_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    const union power_supply_propval *val)
{
	int ret = 0;
	struct axp803_usb_power *usb_power = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = axp803_usb_set_ihold(usb_power, val->intval);
		atomic_set(&usb_power->set_current_limit, 1);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int axp803_usb_power_property_is_writeable(struct power_supply *psy,
		enum power_supply_property psp)
{
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = 1;
		break;
	default:
		ret = 0;
	}

	return ret;
}

static const struct power_supply_desc axp803_usb_desc = {
	.name = "axp803-usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.get_property = axp803_usb_get_property,
	.properties = axp803_usb_props,
	.set_property = axp803_usb_set_property,
	.num_properties = ARRAY_SIZE(axp803_usb_props),
	.property_is_writeable = axp803_usb_power_property_is_writeable,
};

static int axp803_usb_power_init(struct axp803_usb_power *usb_power)
{
	struct axp_config_info *axp_config = &usb_power->dts_info;

	axp803_usb_set_ihold(usb_power, axp_config->pmu_usbad_cur);

	return 0;
}

static irqreturn_t axp803_usb_power_in_irq(int irq, void *data)
{
	struct axp803_usb_power *usb_power = data;
	struct axp_config_info *axp_config = &usb_power->dts_info;

	power_supply_changed(usb_power->usb_supply);

	axp803_usb_set_ihold(usb_power, axp_config->pmu_usbpc_cur);
	atomic_set(&usb_power->set_current_limit, 0);

	cancel_delayed_work_sync(&usb_power->usb_chg_state);
	schedule_delayed_work(&usb_power->usb_chg_state, msecs_to_jiffies(5 * 1000));

	return IRQ_HANDLED;
}

static irqreturn_t axp803_usb_power_out_irq(int irq, void *data)
{
	struct axp803_usb_power *usb_power = data;

	power_supply_changed(usb_power->usb_supply);

	return IRQ_HANDLED;
}

enum axp803_usb_power_virqs {
	AXP803_VIRQ_USBIN,
	AXP803_VIRQ_USBRE,

	AXP803_USB_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp803_usb_irq[] = {
	[AXP803_VIRQ_USBIN] = {"usb in", axp803_usb_power_in_irq},
	[AXP803_VIRQ_USBRE] = {"usb out", axp803_usb_power_out_irq},
};

static void axp803_usb_power_monitor(struct work_struct *work)
{
	struct axp803_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_supply_mon.work);

	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));
}

static int axp803_usb_power_dt_parse(struct axp803_usb_power *usb_power)
{
	struct axp_config_info *axp_config = &usb_power->dts_info;
	struct device_node *node = usb_power->dev->of_node;

	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_usbpc_vol,                  4400);
	AXP_OF_PROP_READ(pmu_usbpc_cur,                     0);
	AXP_OF_PROP_READ(pmu_usbad_vol,                  4400);
	AXP_OF_PROP_READ(pmu_usbad_cur,                     0);

	axp_config->wakeup_usb_in =
		of_property_read_bool(node, "wakeup_usb_in");
	axp_config->wakeup_usb_out =
		of_property_read_bool(node, "wakeup_usb_out");

	return 0;
}

static int axp803_usb_power_probe(struct platform_device *pdev)
{
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	struct axp803_usb_power *usb_power;
	int i, irq;
	int ret = 0;

	if (!axp_dev->irq) {
		pr_err("can not register axp803 usb without irq\n");
		return -EINVAL;
	}

	usb_power = devm_kzalloc(&pdev->dev, sizeof(*usb_power), GFP_KERNEL);
	if (!usb_power) {
		pr_err("axp803 usb power alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	usb_power->name = "axp803-usb-power";
	usb_power->dev = &pdev->dev;
	usb_power->regmap = axp_dev->regmap;

	platform_set_drvdata(pdev, usb_power);

	ret = axp803_usb_power_dt_parse(usb_power);
	if (ret) {
		pr_err("%s parse device tree err\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = axp803_usb_power_init(usb_power);
	if (ret < 0) {
		pr_err("axp803 init usb power fail!\n");
		ret = -ENODEV;
		return ret;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = usb_power;

	usb_power->usb_supply = devm_power_supply_register(usb_power->dev,
			&axp803_usb_desc, &psy_cfg);

	if (IS_ERR(usb_power->usb_supply)) {
		pr_err("axp803 failed to register usb power\n");
		ret = PTR_ERR(usb_power->usb_supply);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(axp803_usb_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp803_usb_irq[i].name);
		if (irq < 0) {
			dev_warn(&pdev->dev, "No IRQ for %s: %d\n",
				 axp803_usb_irq[i].name, irq);
			continue;
		}
		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp803_usb_irq[i].isr, 0,
						   axp803_usb_irq[i].name, usb_power);
		if (ret < 0)
			dev_warn(&pdev->dev, "Error requesting %s IRQ %d: %d\n",
				axp803_usb_irq[i].name, irq, ret);

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp803_usb_irq[i].name, irq, ret);

		/* we use this variable to suspend irq */
		axp803_usb_irq[i].irq = irq;
	}

	INIT_DELAYED_WORK(&usb_power->usb_supply_mon, axp803_usb_power_monitor);
	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));

	INIT_DELAYED_WORK(&usb_power->usb_chg_state, axp803_usb_set_current_fsm);
	schedule_delayed_work(&usb_power->usb_chg_state, msecs_to_jiffies(20 * 1000));

	return 0;
}

static int axp803_usb_power_remove(struct platform_device *pdev)
{
	struct axp803_usb_power *usb_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&usb_power->usb_supply_mon);
	cancel_delayed_work_sync(&usb_power->usb_chg_state);

	return 0;
}

static inline void axp_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp803_usb_virq_dts_set(struct axp803_usb_power *usb_power, bool enable)
{
	struct axp_config_info *dts_info = &usb_power->dts_info;

	if (!dts_info->wakeup_usb_in)
		axp_irq_set(axp803_usb_irq[AXP803_VIRQ_USBIN].irq,
				enable);
	if (!dts_info->wakeup_usb_out)
		axp_irq_set(axp803_usb_irq[AXP803_VIRQ_USBRE].irq,
				enable);
}

static int axp803_usb_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp803_usb_power *usb_power = platform_get_drvdata(pdev);

	axp803_usb_virq_dts_set(usb_power, false);

	return 0;
}

static int axp803_usb_power_resume(struct platform_device *pdev)
{
	struct axp803_usb_power *usb_power = platform_get_drvdata(pdev);

	axp803_usb_virq_dts_set(usb_power, true);

	return 0;
}

static const struct of_device_id axp803_usb_power_match[] = {
	{
		.compatible = "x-powers,axp803-usb-power-supply",
		.data = (void *)AXP803_ID,
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp803_usb_power_match);

static struct platform_driver axp803_usb_power_driver = {
	.driver = {
		.name = "axp803-usb-power-supply",
		.of_match_table = axp803_usb_power_match,
	},
	.probe = axp803_usb_power_probe,
	.remove = axp803_usb_power_remove,
	.suspend = axp803_usb_power_suspend,
	.resume = axp803_usb_power_resume,
};

module_platform_driver(axp803_usb_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp803 usb power driver");
MODULE_LICENSE("GPL");

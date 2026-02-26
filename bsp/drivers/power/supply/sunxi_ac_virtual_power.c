/* SPDX-License-Identifier: GPL-2.0-or-later */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "sunxi-power-supply.h"

struct sunxi_virtual_ac_power {
	char                      *name;
	struct device             *dev;
	struct power_supply       *ac_supply;
};

static enum power_supply_property sunxi_virtual_ac_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
};

static int sunxi_virtual_ac_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		break;
	default:
		break;
	}

	return 0;
}

static const struct power_supply_desc sunxi_virtual_ac_desc = {
	.name = "sunxi-virtual-ac-power",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.get_property = sunxi_virtual_ac_get_property,
	.properties = sunxi_virtual_ac_props,
	.num_properties = ARRAY_SIZE(sunxi_virtual_ac_props),
};

static int sunxi_virtual_ac_power_probe(struct platform_device *pdev)
{
	struct power_supply_config psy_cfg = {};
	struct sunxi_virtual_ac_power *ac_power;
	int ret = 0;

	ac_power = devm_kzalloc(&pdev->dev, sizeof(*ac_power), GFP_KERNEL);
	if (!ac_power) {
		pr_err("sunxi_virtual ac power alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}
	ac_power->name = "sunxi-ac-power";
	ac_power->dev = &pdev->dev;

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = ac_power;

	platform_set_drvdata(pdev, ac_power);
	ac_power->ac_supply = devm_power_supply_register(ac_power->dev,
			&sunxi_virtual_ac_desc, &psy_cfg);
	if (IS_ERR(ac_power->ac_supply)) {
		pr_err("sunxi_virtual failed to register ac power\n");
		ret = PTR_ERR(ac_power->ac_supply);
		return ret;
	}
	return 0;
}

static int sunxi_virtual_ac_power_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id sunxi_virtual_ac_power_match[] = {
	{
		.compatible = "sunxi-virtual-ac-power-supply",
	}, { /* sentinel */ }
};

static struct platform_driver sunxi_virtual_ac_power_driver = {
	.driver = {
		.name = "sunxi-virtual-ac-power",
		.of_match_table = sunxi_virtual_ac_power_match,
	},
	.probe = sunxi_virtual_ac_power_probe,
	.remove = sunxi_virtual_ac_power_remove,
};

module_platform_driver(sunxi_virtual_ac_power_driver);
MODULE_AUTHOR("liufeng <liufeng@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi_virtual ac power driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

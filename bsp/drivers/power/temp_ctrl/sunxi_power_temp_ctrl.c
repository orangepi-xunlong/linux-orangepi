// SPDX-License-Identifier: GPL-2.0-only
/*
 * power supply notifier for sunxi.
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: xinouyang <xinouyang@allwinnertech.com>
 */

#include "sunxi-power-temp-ctrl.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 99)) && IS_ENABLED(CONFIG_THERMAL)
/* thermal cooling device callbacks */
static int ps_get_max_charge_cntl_limit(struct thermal_cooling_device *tcd,
					unsigned long *state)
{
	struct power_supply *psy;
	union power_supply_propval val;
	int ret;

	psy = tcd->devdata;
	ret = power_supply_get_property(psy,
			POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX, &val);
	if (ret)
		return ret;

	*state = val.intval;

	return ret;
}

static int ps_get_cur_charge_cntl_limit(struct thermal_cooling_device *tcd,
					unsigned long *state)
{
	struct power_supply *psy;
	union power_supply_propval val;
	int ret;

	psy = tcd->devdata;
	ret = power_supply_get_property(psy,
			POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, &val);
	if (ret)
		return ret;

	*state = val.intval;

	return ret;
}

static int ps_set_cur_charge_cntl_limit(struct thermal_cooling_device *tcd,
					unsigned long state)
{
	struct power_supply *psy;
	union power_supply_propval val;
	int ret;

	psy = tcd->devdata;
	val.intval = state;
	ret = psy->desc->set_property(psy,
		POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, &val);

	return ret;
}

static const struct thermal_cooling_device_ops psy_tcd_ops = {
	.get_max_state = ps_get_max_charge_cntl_limit,
	.get_cur_state = ps_get_cur_charge_cntl_limit,
	.set_cur_state = ps_set_cur_charge_cntl_limit,
};

int sunxi_power_register_cooler(struct power_supply *psy)
{
	psy->tcd = devm_thermal_of_cooling_device_register(&psy->dev,
		psy->of_node, (char *)psy->desc->name, psy, &psy_tcd_ops);

	return PTR_ERR_OR_ZERO(psy->tcd);
}
EXPORT_SYMBOL(sunxi_power_register_cooler);

void sunxi_power_unregister_cooler(struct power_supply *psy)
{
	if (IS_ERR_OR_NULL(psy->tcd))
		return;
	thermal_cooling_device_unregister(psy->tcd);
}
EXPORT_SYMBOL(sunxi_power_unregister_cooler);
#else
int sunxi_power_register_cooler(struct power_supply *psy)
{
	return 0;
}
EXPORT_SYMBOL(sunxi_power_register_cooler);

void sunxi_power_unregister_cooler(struct power_supply *psy)
{
	return;
}
EXPORT_SYMBOL(sunxi_power_unregister_cooler);
#endif

MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi power temp ctrl");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
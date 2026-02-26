// SPDX-License-Identifier: GPL-2.0-only
/*
 * Sunxi Power Supply Core Driver - Main power supply management framework
 *
 * This driver provides core functionalities for power supply management on Allwinner platforms
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: xinouyang <xinouyang@allwinnertech.com>
 */

#include "sunxi-power-supply.h"

static void _sunxi_power_supply_check_online(struct sunxi_supply_psy_status *power_psy,
						int list_id)
{
	enum sunxi_online_status status_var = SUNXI_STATUS_OFFLINE;
	union power_supply_propval temp;
	struct power_supply *psy;
	const char *phandle_name = power_psy[list_id].phandle_name;
	int ret = 0;

	if (power_psy[list_id].node_status != SUNXI_NODE_ENABLED)
		return;

	if (IS_ERR_OR_NULL(power_psy[list_id].psy))
		return;

	psy = power_psy[list_id].psy;

	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_PRESENT, &temp);
	if (ret < 0) {
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_ONLINE, &temp);
		if (ret < 0)
			temp.intval = 0;
	}

	status_var = temp.intval ? SUNXI_STATUS_ONLINE : SUNXI_STATUS_OFFLINE;
	power_psy[list_id].online_status = status_var;

	PMIC_DEBUG("%s online status is :%s\n",
			 phandle_name, SUNXI_ONLINE_STATUS_TEXT[status_var]);
}

static void _sunxi_power_supply_check_extcon_online(struct sunxi_supply_extcon_status *extcon,
							int list_id)
{
	enum sunxi_online_status status_var = SUNXI_STATUS_OFFLINE;
	const char *phandle_name = extcon[list_id].phandle_name;

	if (extcon[list_id].node_status != SUNXI_NODE_ENABLED)
		return;

	if (IS_ERR_OR_NULL(extcon[list_id].edev))
		return;

	status_var = extcon_get_state(extcon[list_id].edev, extcon[list_id].extcon_id) ?
				SUNXI_STATUS_ONLINE : SUNXI_STATUS_OFFLINE;

	extcon[list_id].online_status = status_var;

	PMIC_DEBUG("%s online status is :%s\n", phandle_name, SUNXI_ONLINE_STATUS_TEXT[status_var]);
}

static struct device_node *sunxi_power_supply_parse_dt_property(
			struct device_node *of_node, const char *phandle_name,
			enum sunxi_node_status *status_var)
{
	struct device_node *np = NULL;

	if (IS_ERR_OR_NULL(phandle_name)) {
		PMIC_INFO("error: phandle_name is NULL\n");
		return NULL;
	}

	np = of_parse_phandle(of_node, phandle_name, 0);
	if (np) {
		*status_var = SUNXI_NODE_DISABLED;
		if (of_device_is_available(np)) {
			PMIC_INFO("%s device is enabled\n", phandle_name);
			*status_var = SUNXI_NODE_ENABLED;
		} else {
			PMIC_INFO("%s device is not enabled\n", phandle_name);
		}
	} else {
		PMIC_INFO("%s device is not configed\n", phandle_name);
	}

	return np;
}

bool sunxi_power_supply_check_online(struct sunxi_supply_psy_status *power_psy,
					int list_id, int max_id)
{
	if (list_id < 0 || list_id >= max_id)
		return false;

	_sunxi_power_supply_check_online(power_psy, list_id);

	return power_psy[list_id].online_status == SUNXI_STATUS_ONLINE;
}
EXPORT_SYMBOL(sunxi_power_supply_check_online);

bool sunxi_power_supply_check_extcon_online(struct sunxi_supply_extcon_status *extcon,
						int list_id, int max_id)
{
	if (list_id < 0 || list_id >= max_id)
		return false;

	_sunxi_power_supply_check_extcon_online(extcon, list_id);

	return extcon[list_id].online_status == SUNXI_STATUS_ONLINE;
}
EXPORT_SYMBOL(sunxi_power_supply_check_extcon_online);

int sunxi_power_supply_init_dt_extcon(struct device *dev, struct device_node *of_node,
					struct sunxi_supply_extcon_status *extcon,
					int extcon_id, int list_id, int max_id,
					int (*notifier_call)(struct notifier_block *nb,
					unsigned long event, void *data))
{
	struct extcon_dev *edev = NULL;
	struct device_node *np = NULL;
	enum sunxi_node_status status_var = SUNXI_NODE_NOT_EXIST;
	const char *phandle_name = extcon[list_id].phandle_name;
	int ret = 0, status;

	if (list_id < 0 || list_id >= max_id) {
		PMIC_ERR("Invalid extcon list_id: %d (max allowed: %d)\n", list_id, max_id-1);
		return -EINVAL;
	}

	np = sunxi_power_supply_parse_dt_property(of_node, phandle_name, &status_var);
	extcon[list_id].extcon_id = extcon_id;
	extcon[list_id].nb.notifier_call = notifier_call;
	extcon[list_id].node_status = status_var;

	if (status_var != SUNXI_NODE_ENABLED) {
		extcon[list_id].online_status = SUNXI_STATUS_OFFLINE;
		return 0;
	}

	edev = extcon_find_edev_by_node(np);
	if (IS_ERR_OR_NULL(edev)) {
		PMIC_ERR("couldn't get extcon device for %s\n", phandle_name);
		return -EPROBE_DEFER;
	}

	ret = devm_extcon_register_notifier(dev, edev,
									  extcon_id, &extcon[list_id].nb);
	if (ret < 0) {
		PMIC_ERR("failed to register notifier for %s, ret:%d\n", phandle_name, ret);
		return -EPROBE_DEFER;
	}

	extcon[list_id].edev = edev;
	PMIC_INFO("%s extcon device initialized successfully\n", phandle_name);

	status = extcon_get_state(extcon[list_id].edev, extcon_id);
	extcon[list_id].online_status = status ? SUNXI_STATUS_ONLINE : SUNXI_STATUS_OFFLINE;

	return ret;
}
EXPORT_SYMBOL(sunxi_power_supply_init_dt_extcon);

int sunxi_power_supply_init_dt_supply(struct device *dev, struct device_node *of_node,
					struct sunxi_supply_psy_status *power_psy,
					int list_id, int max_id)
{
	struct power_supply *psy = NULL;
	struct device_node *np = NULL;
	enum sunxi_node_status status_var = SUNXI_NODE_NOT_EXIST;
	const char *phandle_name = power_psy[list_id].phandle_name;

	if (list_id < 0 || list_id >= max_id) {
		PMIC_ERR("Invalid supply list_id: %d (max allowed: %d)\n", list_id, max_id-1);
		return -EINVAL;
	}

	np = sunxi_power_supply_parse_dt_property(of_node, phandle_name, &status_var);
	power_psy[list_id].node_status = status_var;

	if (status_var != SUNXI_NODE_ENABLED) {
		power_psy[list_id].online_status = SUNXI_STATUS_OFFLINE;
		return 0;
	}

	psy = devm_power_supply_get_by_phandle(dev, phandle_name);
	if (IS_ERR_OR_NULL(psy)) {
		PMIC_ERR("%s supply is not ready\n", phandle_name);
		return -EPROBE_DEFER;
	}

	power_psy[list_id].psy = psy;
	PMIC_INFO("%s supply is :%s\n", phandle_name, psy->desc->name);

	_sunxi_power_supply_check_online(power_psy, list_id);

	return 0;
}
EXPORT_SYMBOL(sunxi_power_supply_init_dt_supply);

MODULE_VERSION("1.0.1");
MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi power supply core");
MODULE_LICENSE("GPL");

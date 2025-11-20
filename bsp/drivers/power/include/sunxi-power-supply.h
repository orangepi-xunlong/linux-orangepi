/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_POWER_SUPPLY_H_
#define _SUNXI_POWER_SUPPLY_H_

/*------------------------------
 * AW Power Core
 *------------------------------*/
#include "sunxi-power-core.h"

/*------------------------------
 * Kernel Thread Management and Freezer
 *------------------------------*/
#include <linux/kthread.h>
#include <linux/freezer.h>

/*------------------------------
 * Device Model and Platform Devices
 *------------------------------*/
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#include <linux/pinctrl/consumer.h>

/*------------------------------
 * Power Supply Management
 *------------------------------*/
#include <linux/power_supply.h>

/*------------------------------
 * AW Power Supply Struct
 *------------------------------*/

enum sunxi_supply_input_detect_type {
	DETEC_UNKNOWN = 0,
	DETEC_BY_VBUS,
	DETEC_BY_GPIO,
	DETEC_BY_EXTCON,
	DETEC_TYPE_MAX,
};

enum sunxi_node_status {
	SUNXI_NODE_UNKNOWN = 0,
	SUNXI_NODE_NOT_EXIST,
	SUNXI_NODE_DISABLED,
	SUNXI_NODE_ENABLED,
	SUNXI_NODE_MAX,
};

enum sunxi_online_status {
	SUNXI_STATUS_UNKNOWN = 0,
	SUNXI_STATUS_OFFLINE,
	SUNXI_STATUS_ONLINE,
	SUNXI_STATUS_MAX,
};

struct sunxi_supply_psy_status {
	const char *phandle_name;
	enum sunxi_node_status node_status;
	enum sunxi_online_status online_status;
	struct power_supply *psy;
};

struct sunxi_supply_extcon_status {
	const char *phandle_name;
	enum sunxi_node_status node_status;
	enum sunxi_online_status online_status;
	int extcon_id;
	struct extcon_dev *edev;
	struct notifier_block nb;
};

static const char * const SUNXI_NODE_STATUS_TEXT[] = {
	[SUNXI_NODE_UNKNOWN]		= "unknown",
	[SUNXI_NODE_NOT_EXIST]		= "node not config",
	[SUNXI_NODE_DISABLED]		= "node disabled",
	[SUNXI_NODE_ENABLED]		= "node enabled",
	[SUNXI_NODE_MAX]		= "node overflow",
};

static const char * const SUNXI_ONLINE_STATUS_TEXT[] = {
	[SUNXI_STATUS_UNKNOWN]		= "unknown",
	[SUNXI_STATUS_OFFLINE]		= "offline",
	[SUNXI_STATUS_ONLINE]		= "online",
	[SUNXI_STATUS_MAX]		= "online status overflow",
};

/**
 * sunxi_power_supply_check_online - Check if a power supply is online
 * @power_psy: Pointer to power supply status structure
 * @list_id: ID of the power supply to check
 * @max_id: Maximum allowed ID value for validation
 *
 * This function checks if a specified power supply is online by:
 * 1. Validating the list_id parameter
 * 2. Calling the internal status check function
 * 3. Returning the final online status
 *
 * Return: true if power supply is online, false otherwise
 */
bool sunxi_power_supply_check_online(struct sunxi_supply_psy_status *power_psy,
					int list_id, int max_id);

/**
 * sunxi_power_supply_check_extcon_online - Check if an extcon device is online
 * @extcon: Pointer to extcon status structure
 * @list_id: ID of the extcon device to check
 * @max_id: Maximum allowed ID value for validation
 *
 * This function checks if a specified extcon device is online by:
 * 1. Validating the list_id parameter
 * 2. Calling the internal status check function
 * 3. Returning the final online status
 *
 * Return: true if extcon device is online, false otherwise
 */
bool sunxi_power_supply_check_extcon_online(struct sunxi_supply_extcon_status *extcon,
						int list_id, int max_id);

/**
 * sunxi_power_supply_init_dt_extcon - Initialize extcon device from device tree
 * @dev: Pointer to device structure
 * @of_node: Pointer to device node
 * @extcon: Pointer to extcon status structure
 * @extcon_id: ID of the extcon device
 * @list_id: ID of the extcon device to initialize
 * @max_id: Maximum allowed ID value for validation
 * @notifier_call: Callback function for extcon notifier
 *
 * This function initializes an extcon device from device tree by:
 * 1. Parsing device tree properties
 * 2. Registering notifier for extcon events
 * 3. Storing the initialized extcon device
 *
 * Return: 0 on success, negative error code on failure
 */
int sunxi_power_supply_init_dt_extcon(struct device *dev, struct device_node *of_node,
					struct sunxi_supply_extcon_status *extcon,
					int extcon_id, int list_id, int max_id,
					int (*notifier_call)(struct notifier_block *nb,
					unsigned long event, void *data));

/**
 * sunxi_power_supply_init_dt_supply - Initialize power supply from device tree
 * @dev: Pointer to device structure
 * @of_node: Pointer to device node
 * @power_psy: Pointer to power supply status structure
 * @list_id: ID of the power supply to initialize
 * @max_id: Maximum allowed ID value for validation
 *
 * This function initializes a power supply from device tree by:
 * 1. Parsing device tree properties
 * 2. Getting power supply instance
 * 3. Checking initial online status
 *
 * Return: 0 on success, negative error code on failure
 */
int sunxi_power_supply_init_dt_supply(struct device *dev, struct device_node *of_node,
					struct sunxi_supply_psy_status *power_psy,
					int list_id, int max_id);

#endif /*  _SUNXI_POWER_SUPPLY_H_ */
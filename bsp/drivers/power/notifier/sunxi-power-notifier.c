// SPDX-License-Identifier: GPL-2.0-only
/*
 * power supply notifier for sunxi.
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: xinouyang <xinouyang@allwinnertech.com>
 */

#include "sunxi-power-notifier.h"

BLOCKING_NOTIFIER_HEAD(sunxi_power_supply_notifier);
EXPORT_SYMBOL_GPL(sunxi_power_supply_notifier);

int sunxi_power_supply_reg_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&sunxi_power_supply_notifier, nb);
}
EXPORT_SYMBOL(sunxi_power_supply_reg_notifier);

void sunxi_power_supply_unreg_notifier(struct notifier_block *nb)
{
	blocking_notifier_chain_unregister(&sunxi_power_supply_notifier, nb);
}
EXPORT_SYMBOL(sunxi_power_supply_unreg_notifier);

void sunxi_call_power_supply_notifier(unsigned long event)
{
	blocking_notifier_call_chain(&sunxi_power_supply_notifier, event, NULL);
}
EXPORT_SYMBOL(sunxi_call_power_supply_notifier);

void sunxi_call_power_supply_notifier_with_data(unsigned long event, void *data)
{
	blocking_notifier_call_chain(&sunxi_power_supply_notifier, event, data);
}
EXPORT_SYMBOL(sunxi_call_power_supply_notifier_with_data);

MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi power notifier");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

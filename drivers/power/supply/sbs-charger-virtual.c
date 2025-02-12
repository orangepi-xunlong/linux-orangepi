// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SBS Charger Virtual Driver
 *
 * Copyright (c) 2023, Your Name or Company
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define SBS_CHARGER_POLL_TIME 5000

struct sbs_charger_info {
	struct device *dev;
	struct power_supply *charger;
	struct power_supply *battery;
	struct delayed_work work;
	int last_status;
	bool is_present;
	bool is_online;
};

static enum power_supply_property sbs_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
};

static int sbs_charger_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	struct sbs_charger_info *info = power_supply_get_drvdata(psy);
	int ret;
	union power_supply_propval capacity_val;

	if (!info->battery) {
		dev_err(info->dev, "Battery power supply not available\n");
		return -ENODEV;
	}

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = power_supply_get_property(info->battery,
						POWER_SUPPLY_PROP_STATUS, val);
		if (ret == 0) {
			if (val->intval == POWER_SUPPLY_STATUS_CHARGING ||
			    val->intval == POWER_SUPPLY_STATUS_FULL) {
				info->is_online = true;
			} else {
				ret = power_supply_get_property(
					info->battery,
					POWER_SUPPLY_PROP_CAPACITY,
					&capacity_val);
				if (ret == 0 && capacity_val.intval >= 98 &&
				    val->intval ==
					    POWER_SUPPLY_STATUS_NOT_CHARGING) {
					info->is_online = true;
				} else {
					info->is_online = false;
				}
			}
		}
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = power_supply_get_property(info->battery, psp, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = info->is_present;
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = info->is_online;
		ret = 0;
		break;
	default:
		dev_err(info->dev, "Unsupported property %d\n", psp);
		return -EINVAL;
	}

	if (ret)
		dev_err(info->dev, "Failed to get property %d: %d\n", psp, ret);

	return ret;
}

static void sbs_charger_external_power_changed(struct power_supply *psy)
{
	struct sbs_charger_info *info = power_supply_get_drvdata(psy);

	dev_dbg(info->dev, "External power changed\n");
	cancel_delayed_work_sync(&info->work);
	schedule_delayed_work(&info->work, 0);
}

static void sbs_charger_work(struct work_struct *work)
{
	struct sbs_charger_info *info =
		container_of(work, struct sbs_charger_info, work.work);
	union power_supply_propval val;
	int ret;

	if (!info->battery) {
		dev_err(info->dev,
			"Battery power supply not available in work function\n");
		info->is_present = false;
		goto reschedule;
	}

	ret = power_supply_get_property(info->battery, POWER_SUPPLY_PROP_STATUS,
					&val);
	if (ret == 0 && info->last_status != val.intval) {
		info->last_status = val.intval;
		dev_info(info->dev, "Battery status changed to %d\n",
			 val.intval);
		power_supply_changed(info->charger);
	} else if (ret != 0) {
		dev_err(info->dev, "Failed to get battery status: %d\n", ret);
	}

	// Check battery presence
	ret = power_supply_get_property(info->battery,
					POWER_SUPPLY_PROP_PRESENT, &val);
	if (ret == 0) {
		info->is_present = val.intval;
	} else {
		dev_err(info->dev, "Failed to get battery presence: %d\n", ret);
		info->is_present = false;
	}

reschedule:
	schedule_delayed_work(&info->work,
			      msecs_to_jiffies(SBS_CHARGER_POLL_TIME));
}

static const struct power_supply_desc sbs_charger_desc = {
	.name = "sbs-charger",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = sbs_charger_props,
	.num_properties = ARRAY_SIZE(sbs_charger_props),
	.get_property = sbs_charger_get_property,
	.external_power_changed = sbs_charger_external_power_changed,
};

static int sbs_charger_probe(struct platform_device *pdev)
{
	struct sbs_charger_info *info;
	struct power_supply_config psy_cfg = {};
	const char *battery_name;
	int ret;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = &pdev->dev;

	ret = of_property_read_string(pdev->dev.of_node, "battery-name",
				      &battery_name);
	if (ret) {
		dev_err(&pdev->dev,
			"Failed to read battery-name from device tree\n");
		return ret;
	}

	info->battery = power_supply_get_by_name(battery_name);
	if (!info->battery) {
		dev_info(&pdev->dev,
			 "Battery power supply not ready, deferring probe\n");
		return -EPROBE_DEFER;
	}

	psy_cfg.drv_data = info;
	psy_cfg.of_node = pdev->dev.of_node;

	info->charger = devm_power_supply_register(&pdev->dev,
						   &sbs_charger_desc, &psy_cfg);
	if (IS_ERR(info->charger)) {
		ret = PTR_ERR(info->charger);
		dev_err(&pdev->dev, "Failed to register power supply: %d\n",
			ret);
		goto put_battery;
	}

	INIT_DELAYED_WORK(&info->work, sbs_charger_work);
	schedule_delayed_work(&info->work,
			      msecs_to_jiffies(SBS_CHARGER_POLL_TIME));

	platform_set_drvdata(pdev, info);

	dev_info(&pdev->dev,
		 "SBS charger virtual driver probed successfully\n");
	return 0;

put_battery:
	power_supply_put(info->battery);
	return ret;
}

static int sbs_charger_remove(struct platform_device *pdev)
{
	struct sbs_charger_info *info = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "Removing SBS charger virtual driver\n");

	cancel_delayed_work_sync(&info->work);
	if (info->battery) {
		power_supply_put(info->battery);
	}

	return 0;
}

static const struct of_device_id sbs_charger_of_match[] = {
	{ .compatible = "sbs,sbs-charger-virtual" },
	{},
};
MODULE_DEVICE_TABLE(of, sbs_charger_of_match);

static struct platform_driver sbs_charger_driver = {
	.driver = {
		.name = "sbs-charger-virtual",
		.of_match_table = of_match_ptr(sbs_charger_of_match),
	},
	.probe = sbs_charger_probe,
	.remove = sbs_charger_remove,
};

module_platform_driver(sbs_charger_driver);

MODULE_AUTHOR("goumin");
MODULE_DESCRIPTION("SBS Charger Virtual Driver");
MODULE_LICENSE("GPL v2");
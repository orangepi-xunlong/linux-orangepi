// SPDX-License-Identifier: GPL-2.0
/*
 * Power supply driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/platform_data/cros_ec_commands.h>
#include <linux/platform_data/cros_ec_proto.h>
#include <linux/of.h>

#define EC_BATTERY_PERCENT_FACTOR 100

struct cix_ec_battery_data {
	struct power_supply *battery;
	struct cros_ec_device *ec;
	uint16_t design_capacity;
	uint16_t design_voltage;
	uint32_t cycle_count;
	char manufacturer[EC_COMM_TEXT_MAX + 1];
	char model[EC_COMM_TEXT_MAX + 1];
	char serial[EC_COMM_TEXT_MAX + 1];
	char type[EC_COMM_TEXT_MAX + 1];
};

static int cix_ec_battery_get_static_info(struct cix_ec_battery_data *data,
					  int battery_index)
{
	int ret = 0;
	struct {
		struct cros_ec_command msg;
		union {
			struct ec_params_battery_static_info params;
			struct ec_response_battery_static_info resp;
		};
	} __packed buf;
	struct ec_params_battery_static_info *params = &buf.params;
	struct ec_response_battery_static_info *resp = &buf.resp;

	struct cros_ec_command *msg = &buf.msg;

	memset(&buf, 0, sizeof(buf));

	msg->version = 0;
	msg->command = EC_CMD_BATTERY_GET_STATIC;
	msg->insize = sizeof(*resp);
	msg->outsize = sizeof(*params);
	ret = cros_ec_cmd_xfer_status(data->ec, msg);
	if (ret < 0)
		return ret;

	data->design_capacity = ec_be16_to_cpu(resp->design_capacity);
	data->design_voltage = ec_be16_to_cpu(resp->design_voltage);
	data->cycle_count = ec_be32_to_cpu(resp->cycle_count);
	memcpy(data->manufacturer, resp->manufacturer, EC_COMM_TEXT_MAX);
	memcpy(data->model, resp->model, EC_COMM_TEXT_MAX);
	memcpy(data->serial, resp->serial, EC_COMM_TEXT_MAX);
	memcpy(data->type, resp->type, EC_COMM_TEXT_MAX);

	return 0;
}

static int cix_ec_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct cix_ec_battery_data *data = power_supply_get_drvdata(psy);
	struct cros_ec_device *ec = data->ec;
	int ret;
	struct {
		struct cros_ec_command msg;
		union {
			struct ec_params_battery_dynamic_info params;
			struct ec_response_battery_dynamic_info resp;
		};
	} __packed buf;
	struct ec_params_battery_dynamic_info *params = &buf.params;
	struct ec_response_battery_dynamic_info *resp = &buf.resp;

	struct cros_ec_command *msg = &buf.msg;
	memset(&buf, 0, sizeof(buf));
	msg->version = 0;
	msg->command = EC_CMD_BATTERY_GET_DYNAMIC;
	msg->insize = sizeof(*resp);
	msg->outsize = sizeof(*params);

	ret = cros_ec_cmd_xfer_status(ec, msg);
	if (ret < 0) {
		return -EINVAL;
	}

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = ec_be16_to_cpu(resp->actual_voltage);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = (int16_t)(ec_be16_to_cpu(resp->actual_current));
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		val->intval = ec_be16_to_cpu(resp->remaining_capacity);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL:
		val->intval = ec_be16_to_cpu(resp->full_capacity);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = ec_be16_to_cpu(resp->flags);
		if (!(val->intval & EC_BATT_FLAG_AC_PRESENT) ||
		    !(val->intval & EC_BATT_FLAG_BATT_PRESENT))
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (val->intval & EC_BATT_FLAG_CHARGING)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (val->intval & EC_BATT_FLAG_DISCHARGING)
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		val->intval = ec_be16_to_cpu(resp->desired_voltage);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		val->intval = ec_be16_to_cpu(resp->desired_current);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = data->design_capacity;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = data->design_voltage;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		val->intval = data->cycle_count;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = data->manufacturer;
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = data->model;
		break;
	case POWER_SUPPLY_PROP_SERIAL_NUMBER:
		val->strval = data->serial;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = EC_BATTERY_PERCENT_FACTOR *
                              ec_be16_to_cpu(resp->remaining_capacity) /
                              ec_be16_to_cpu(resp->full_capacity);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = ec_be16_to_cpu(resp->flags) &
                              EC_BATT_FLAG_BATT_PRESENT ? 1 : 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property cix_ec_battery_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_ENERGY_NOW,
	POWER_SUPPLY_PROP_ENERGY_FULL,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_PRESENT,
};

static const struct power_supply_desc battery_desc = {
	.properties = cix_ec_battery_props,
	.num_properties = ARRAY_SIZE(cix_ec_battery_props),
	.get_property = cix_ec_battery_get_property,
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
};

static int cix_ec_battery_probe(struct platform_device *pdev)
{
	int ret;
	struct cix_ec_battery_data *data;
	struct power_supply_config psy_cfg = {};
	struct cros_ec_device *ec = dev_get_drvdata(pdev->dev.parent);

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;
	data->ec = ec;
	psy_cfg.drv_data = data;
	platform_set_drvdata(pdev, data);

	ret = cix_ec_battery_get_static_info(data, 0);
	if (ret < 0) {
		printk("%s cix_ec_battery_get_static_info failed", __func__);
		return ret;
	}

	data->battery = power_supply_register_no_ws(&pdev->dev, &battery_desc,
						    &psy_cfg);
	if (IS_ERR(data->battery))
		return PTR_ERR(data->battery);

	return 0;
}

static int cix_ec_battery_remove(struct platform_device *pdev)
{
	struct cix_ec_battery_data *data = platform_get_drvdata(pdev);
	power_supply_unregister(data->battery);
	return 0;
}

static const struct of_device_id cix_ec_battery_of_match[] = {
	{ .compatible = "cix,cix-ec-battery", },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ec_battery_of_match);

static struct platform_driver cix_ec_battery_device = {
	.probe		= cix_ec_battery_probe,
	.remove		= cix_ec_battery_remove,
	.driver = {
		.name = "cix-ec-battery",
		.of_match_table = cix_ec_battery_of_match,
	}
};
module_platform_driver(cix_ec_battery_device);

MODULE_ALIAS("platform:cix-ec-battery");
MODULE_DESCRIPTION("CIX EC Battery driver");
MODULE_LICENSE("GPL v2");

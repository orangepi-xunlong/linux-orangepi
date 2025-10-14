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

struct cix_ec_charge_data {
	struct power_supply *charge;
	struct cros_ec_device *ec;
	char name[7];
};

struct ec_response_charger_info {
	char name[7];    // charger ic name. SC8886
	char type[3];    // PD
	bool online;     // 0
};

static int cix_ec_charge_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct cix_ec_charge_data *data = power_supply_get_drvdata(psy);
	struct cros_ec_device *ec = data->ec;
	int ret;
	struct {
		struct cros_ec_command msg;
		union {
			struct ec_response_charger_info resp;
		};
	} __packed buf;
	struct ec_response_charger_info *resp = &buf.resp;
	struct cros_ec_command *msg = &buf.msg;

	memset(&buf, 0, sizeof(buf));
	msg->version = 0;
	msg->command = EC_CMD_GET_CHARGER_INFO;
	msg->insize = sizeof(*resp);
	msg->outsize = 0;

	ret = cros_ec_cmd_xfer_status(ec, msg);
	if (ret < 0) {
		return -EINVAL;
	}
	strncpy(data->name, resp->name, 7);

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = data->name;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = resp->online;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property cix_ec_charge_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_ONLINE,
};

static const struct power_supply_desc charge_desc = {
	.properties = cix_ec_charge_props,
	.num_properties = ARRAY_SIZE(cix_ec_charge_props),
	.get_property = cix_ec_charge_get_property,
	.name = "charge",
	.type = POWER_SUPPLY_TYPE_USB_PD,
};

static int cix_ec_charge_probe(struct platform_device *pdev)
{
	struct cix_ec_charge_data *data;
	struct power_supply_config psy_cfg = {};
	struct cros_ec_device *ec = dev_get_drvdata(pdev->dev.parent);

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;
	data->ec = ec;
	psy_cfg.drv_data = data;
	platform_set_drvdata(pdev, data);
	data->charge = power_supply_register_no_ws(&pdev->dev, &charge_desc,
						    &psy_cfg);
	if (IS_ERR(data->charge))
		return PTR_ERR(data->charge);

	return 0;
}

static int cix_ec_charge_remove(struct platform_device *pdev)
{
	struct cix_ec_charge_data *data = platform_get_drvdata(pdev);
	power_supply_unregister(data->charge);
	return 0;
}

static const struct of_device_id cix_ec_charge_of_match[] = {
	{ .compatible = "cix,cix-ec-charge", },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ec_charge_of_match);

static struct platform_driver cix_ec_charge_device = {
	.probe		= cix_ec_charge_probe,
	.remove		= cix_ec_charge_remove,
	.driver = {
		.name = "cix-ec-charge",
		.of_match_table = cix_ec_charge_of_match,
	}
};
module_platform_driver(cix_ec_charge_device);

MODULE_ALIAS("platform:cix-ec-charge");
MODULE_DESCRIPTION("CIX EC Charge driver");
MODULE_LICENSE("GPL v2");

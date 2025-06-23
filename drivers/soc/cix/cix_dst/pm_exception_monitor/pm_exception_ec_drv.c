// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_data/cros_ec_proto.h>
#include <linux/platform_data/cros_ec_commands.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "pm_exception_dsm.h"
#include "../dst_print.h"

#define CHARGER_EXCEP_SHIFT 13

struct cros_ec_excp_data {
	struct cros_ec_device *ec_dev;
    struct delayed_work ec_excp_wq;
	struct notifier_block notifier;
};

static struct cros_ec_excp_data *g_ec_excp_data;

static uint8_t get_ec_poweroff_reason(struct cros_ec_device *ec_dev)
{
	uint8_t reason = 0;
	int ret;
	ret = cros_ec_cmd(ec_dev, 0, EC_CMD_GET_PWROFFRSN, NULL, 0,
			&reason, sizeof(reason));

	if (ret < 0) {
		DST_PRINT_ERR("Failed to get poweroff reason, ret = %d\n", ret);
		return 0;
	}

	return reason;
}

static int get_charger_state(struct cros_ec_device *ec_dev, struct ec_response_ic_error_info *pstate)
{
	int ret;

	ret = cros_ec_cmd(ec_dev, 0, EC_CMD_GET_IC_ERROR_STATE, NULL, 0,
			pstate, sizeof(*pstate));

	if (ret < 0) {
		DST_PRINT_ERR("Failed to get charger state, ret = %d\n", ret);
		return -1;
	}

	return 0;
}

static int ec_report_exception(void)
{
	uint8_t reason;
	struct ec_response_ic_error_info state;

	if (!g_ec_excp_data) {
		return -EINVAL;
	}

	reason = get_ec_poweroff_reason(g_ec_excp_data->ec_dev);
	DST_PRINT_PN("EC last poweroff reason is 0x%x\n", reason);

	if (!get_charger_state(g_ec_excp_data->ec_dev, &state)) {
		DST_PRINT_PN("Charger state: chgstate = 0x%x, prochotstate = 0x%x, ecstate=0x%x, reserved=0x%x\n",
			state.ChgState, state.ProchotState, state.ECState, state.Reserved);
	}

#if 0
	id = pm_calc_error_id(cmd.excp_src, cmd.excp_type, cmd.excp_id);

	pm_dsm_report_info(cmd.excp_src, id, "desc: %s", cmd.excp_desc);
#endif
	return 0;
}

static void cros_ec_check_exception(struct work_struct *work)
{
	ec_report_exception();
}

static int cros_charger_excep_notify(struct notifier_block *nb,
			unsigned long event, void *arg)
{
	struct cros_ec_device *ec_dev;
	uint32_t data;
	struct ec_response_ic_error_info state;

	ec_dev = (struct cros_ec_device *)arg;
	data = ec_dev->irq_info.data & (1 << CHARGER_EXCEP_SHIFT);
	if (!data)
		return NOTIFY_DONE;

	if (!get_charger_state(g_ec_excp_data->ec_dev, &state)) {
		DST_PRINT_PN("Charger state: chgstate = 0x%x, prochotstate = 0x%x, ecstate=0x%x, reserved=0x%x\n",
			state.ChgState, state.ProchotState, state.ECState, state.Reserved);
	}

	return NOTIFY_DONE;
}

static struct notifier_block cix_charger_excep_notifier = {
	.notifier_call = cros_charger_excep_notify,
};

static int cros_ec_excp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cros_ec_excp_data *drvdata;
	int ret;

	drvdata = devm_kzalloc(
		&pdev->dev, sizeof(struct cros_ec_excp_data), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->ec_dev = dev_get_drvdata(dev->parent);

	INIT_DELAYED_WORK(&drvdata->ec_excp_wq, cros_ec_check_exception);

	platform_set_drvdata(pdev, drvdata);
	g_ec_excp_data = drvdata;

	ret = blocking_notifier_chain_register(&drvdata->ec_dev->event_notifier,
					      &cix_charger_excep_notifier);

	if (ret < 0)
		dev_err(dev, "Failed to register notifier (err:%d)\n", ret);

	schedule_delayed_work(&drvdata->ec_excp_wq, msecs_to_jiffies(1));
	return 0;
}

static const struct of_device_id excp_cros_ec_of_match[] = {
	{ .compatible = "cix,cix-ec-excp", },
	{}
};
MODULE_DEVICE_TABLE(of, regulator_cros_ec_of_match);

static struct platform_driver cros_ec_excp_driver = {
	.probe		= cros_ec_excp_probe,
	.driver		= {
		.name		= "cix-ec-excp",
		.of_match_table = excp_cros_ec_of_match,
	},
};

module_platform_driver(cros_ec_excp_driver);

MODULE_AUTHOR("Vincent Wu<vincent.wu@cixtech.com>");
MODULE_DESCRIPTION("CIX EC Exception driver");
MODULE_LICENSE("GPL v2");

// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Power Interface (SCMI) based PM exception debug driver
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/scmi_protocol.h>
#include "../dst_print.h"
#include "pm_exception_drv.h"

static struct scmi_protocol_handle *ph;
static const struct scmi_pmexcp_proto_ops *pmexcp_ops;

static char *get_src_by_id(uint8_t id)
{
	switch (id) {
        case 0:
            return "NO";
        case 1:
            return "PMIC";
        case 2:
            return "Charger";
        case 3:
            return "EC";
        default:
            return "Unknown";
    }
}

void print_pm_exception_data(struct pm_exception_info *info)
{
	DST_PRINT_PN("%s, %s error: type=%d, id=%d, desc=%s...\n", __func__,
		get_src_by_id(info->excp_src), info->excp_type,
		info->excp_id, info->excp_desc);
}

static int scmi_pmexcp_notify(struct notifier_block *nb, unsigned long event, void *data)
{
	/* test for the flow */
	DST_PRINT_PN("%s start the flow of notify\n", __func__);

	pm_pmic_report_exception((csu_pm_state_report_t*)data);

	return NOTIFY_OK;
}

static int scmi_pm_excp_probe(struct scmi_device *sdev)
{
	int ret = 0;
	const struct scmi_handle *handle;
	struct notifier_block *nb;

	handle = sdev->handle;

	DST_PRINT_PN("%s, enter...\n", __func__);

	if (!handle)
		return -ENODEV;

	pmexcp_ops =
		handle->devm_protocol_get(sdev, SCMI_PROTOCOL_PM_EXCP, &ph);
	if (IS_ERR(pmexcp_ops))
		return PTR_ERR(pmexcp_ops);

	nb = devm_kzalloc(&sdev->dev, sizeof(*nb), GFP_KERNEL);
	nb->notifier_call = &scmi_pmexcp_notify;
	/* register notify */
	handle->notify_ops->devm_event_notifier_register(sdev, SCMI_PROTOCOL_PM_EXCP,
							 SCMI_EVENT_PMEXCP_REPORT, NULL,
							 nb);
	return ret;
}

void get_pm_exception_data(void)
{
	int ret = 0;

	if (ph && pmexcp_ops)
		ret = pmexcp_ops->get_exception(ph);

	if (ret)
		DST_PRINT_ERR("%s, get data failed...\n", __func__);
}

static void scmi_pm_excp_remove(struct scmi_device *sdev)
{
	/* Nothing need to be done now */
	return;
}

static const struct scmi_device_id scmi_id_table[] = {
	{ SCMI_PROTOCOL_PM_EXCP, "pm_excp" },
};
MODULE_DEVICE_TABLE(scmi, scmi_id_table);

static struct scmi_driver scmi_pm_excp_drv = {
	.name = "scmi-pm-excp",
	.probe = scmi_pm_excp_probe,
	.remove = scmi_pm_excp_remove,
	.id_table = scmi_id_table,
};
module_scmi_driver(scmi_pm_excp_drv);

MODULE_AUTHOR("Xinglong Yang<xinglong.yang@cixtech.com>");
MODULE_DESCRIPTION("CIX SCMI Debug interface driver");
MODULE_LICENSE("GPL v2");

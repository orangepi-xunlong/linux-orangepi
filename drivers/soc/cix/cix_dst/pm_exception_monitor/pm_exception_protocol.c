// SPDX-License-Identifier: GPL-2.0
/*
 * System Control and Management Interface (SCMI) Performance Protocol
 *
 * Copyright (C) 2018-2022 ARM Ltd.
 */

#include <linux/bits.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/scmi_protocol.h>
#include <linux/sort.h>
#include <trace/events/scmi.h>
#include "../../../../firmware/arm_scmi/protocols.h"
#include "../../../../firmware/arm_scmi/notify.h"
#include "pm_exception_dsm.h"
#include "pm_exception_drv.h"
#include "../dst_print.h"

enum scmi_pmexcp_protocol_cmd {
	EXCEPTION_GET = 0x0,
};

static bool translate_pm_to_pmic(csu_pm_state_report_t *in,
				 struct pm_exception_info *out)
{
	uint8_t i;
	if (in->pmic_events_flag & in->pmic_support_mask) {
		out->excp_src = PM_PMIC_ERROR;

		for (i = 0; i < MAX_PMIC_NUM; i++) {
			if (in->events_flag.pmic_flag[i].pmic_status8 &
				in->support_mask.pmic_mask[i].pmic_status8) {
				out->excp_type = i;
				out->excp_id = in->events_flag.pmic_flag[i].pmic_status8;
				memset(out->excp_desc, 0, sizeof(out->excp_desc));
				sprintf(out->excp_desc, "0x%x,0x%x,0x%x", in->pmic_status_word[0],
					in->pmic_status_word[1], in->pmic_status_word[2]);
				break;
			}
		}
		return true;
	}
	return false;
}

int pm_pmic_report_exception(csu_pm_state_report_t *info)
{
	struct pm_exception_info pmic_info;
	int id;

	if (!translate_pm_to_pmic(info, &pmic_info))
		return 0;

	id = pm_calc_error_id(pmic_info.excp_src, pmic_info.excp_type,
			      pmic_info.excp_id);

	print_pm_exception_data(&pmic_info);

	DST_PRINT_PN("%s: data=0x%x, 0x%x, 0x%x, 0x%x, 0x%x \n",
			__func__, info->pmic_events_flag,
			info->pmic_support_mask, info->pmic_status_word[0],
			info->pmic_status_word[1], info->pmic_status_word[2]);
	// report exception to dsm
	pm_dsm_report_info(pmic_info.excp_src - 1, id,
			"rawdata=0x%x, src=%d, type=%d, desc: %s",
			info->pmic_events_flag, pmic_info.excp_src,
			pmic_info.excp_type, pmic_info.excp_desc);

	return 0;
}

static int scmi_pm_exception_get(const struct scmi_protocol_handle *ph)
{
	int ret;
	struct scmi_xfer *t;
	csu_pm_state_report_t *exception_info;

	ret = ph->xops->xfer_get_init(ph, EXCEPTION_GET, 0,
				      sizeof(*exception_info), &t);

	if (ret)
		return ret;

	exception_info = t->rx.buf;

	ret = ph->xops->do_xfer(ph, t);
	if (!ret) {
		DST_PRINT_DBG("%s: data=0x%x, 0x%x, 0x%x, 0x%x, 0x%x \n", __func__,
			exception_info->pmic_events_flag,
			exception_info->pmic_support_mask,
			exception_info->pmic_status_word[0],
			exception_info->pmic_status_word[1],
			exception_info->pmic_status_word[2]);

		// report exception to dsm
		pm_pmic_report_exception(exception_info);
	}

	ph->xops->xfer_put(ph, t);

	return 0;
}

static int scmi_pmexcp_get_num_sources(const struct scmi_protocol_handle *ph)
{
	int num_sources;

	/* Can be modified in future to support multiple sources */
	num_sources = 1;

	return num_sources;
}

static int scmi_pmexcp_set_notify_enabled(const struct scmi_protocol_handle *ph,
					  u8 evt_id, u32 src_id, bool enable)
{
	int ret;

	/* the judge of enable may be flexible in future */
	ret = 0;

	return ret;
}

static void *scmi_pmexcp_fill_custom_report(const struct scmi_protocol_handle *ph,
					  u8 evt_id, ktime_t timestamp,
					  const void *payld, size_t payld_sz,
					  void *report, u32 *src_id)
{
	void *rep = NULL;

	switch (evt_id) {
	case SCMI_EVENT_PMEXCP_REPORT:
	{
		/* The report struct wiil be modify in future */
		const csu_pm_state_report_t *p = payld;
		csu_pm_state_report_t *r = report;

		memcpy(r, p, sizeof(*p));
		rep = r;
		break;
	}
	default:
		break;
	}

	return rep;
}

static const struct scmi_event pmexcp_events[] = {
	{
		.id = SCMI_EVENT_PMEXCP_REPORT,
		.max_payld_sz = sizeof(csu_pm_state_report_t),
		.max_report_sz= sizeof(csu_pm_state_report_t),
	},
};

static const struct scmi_event_ops pmexcp_event_ops = {
	.get_num_sources = scmi_pmexcp_get_num_sources,
	.set_notify_enabled = scmi_pmexcp_set_notify_enabled,
	.fill_custom_report = scmi_pmexcp_fill_custom_report,
};

static const struct scmi_protocol_events pmexcp_protocol_events = {
	.queue_sz = SCMI_PROTO_QUEUE_SZ,
	.ops = &pmexcp_event_ops,
	.evts = pmexcp_events,
	.num_events = ARRAY_SIZE(pmexcp_events),
};

static const struct scmi_pmexcp_proto_ops pmexcp_proto_ops = {
	.get_exception = scmi_pm_exception_get,
};

static int scmi_pmexcp_protocol_init(const struct scmi_protocol_handle *ph)
{
	/* nothing need to be done now */
	return 0;
}

static const struct scmi_protocol scmi_pm_excp = {
	.id = SCMI_PROTOCOL_PM_EXCP,
	.owner = THIS_MODULE,
	.instance_init = &scmi_pmexcp_protocol_init,
	.ops = &pmexcp_proto_ops,
	.events = &pmexcp_protocol_events,
};

DEFINE_SCMI_PROTOCOL_REGISTER_UNREGISTER(pm_excp, scmi_pm_excp)

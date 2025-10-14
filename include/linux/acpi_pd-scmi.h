/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */

#ifndef __ACPI_SCMI_POWER_H_
#define __ACPI_SCMI_POWER_H_

#define POWER_DOMAIN_LIST	"PDList"
#define POWER_DOMAIN_ID	"PdId"
#define MAX_STR_SIZE	16
#define MAX_NUM_RATES	16

#define ACPI_POWER_STATE_TYPE_SHIFT	30
#define ACPI_POWER_STATE_ID_MASK	(BIT(28) - 1)
#define SCMI_POWER_STATE_PARAM(type, id) \
	((((type) & BIT(0)) << ACPI_POWER_STATE_TYPE_SHIFT) | \
		((id) & ACPI_POWER_STATE_ID_MASK))
#define ACPI_PD_ON	SCMI_POWER_STATE_PARAM(0, 0)
#define ACPI_PD_OFF	SCMI_POWER_STATE_PARAM(1, 0)

struct power_domain_list
{
	const char *name[MAX_NUM_RATES];
	u32 pd_id[MAX_NUM_RATES];
};

#ifdef CONFIG_CIX_ACPI_POWER_DOMAIN
int acpi_pd_on(struct device *dev, u32 pd_id);
int acpi_pd_off(struct device *dev,u32 pd_id);
#else
static int acpi_pd_off(struct device *dev, u32 pd_id)
{
	return 0;
}

static u64 acpi_get_pd_state(struct device *dev, u32 pd_id)
{
	return 0;
}
#endif
#endif

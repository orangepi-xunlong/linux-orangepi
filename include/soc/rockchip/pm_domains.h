/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SOC_ROCKCHIP_PM_DOMAINS_H
#define __SOC_ROCKCHIP_PM_DOMAINS_H

#include <linux/errno.h>

struct device;

#ifdef CONFIG_ROCKCHIP_PM_DOMAINS
int rockchip_pmu_idle_request(struct device *dev, bool idle);
#else
static inline int rockchip_pmu_idle_request(struct device *dev, bool idle)
{
	return -ENOTSUPP;
}
#endif

#endif

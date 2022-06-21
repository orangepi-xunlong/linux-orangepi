/*
 * pm_domain.h - Definitions and headers related to device power domains.
 *
 * Copyright (C) 2017 Randy Li <ayaka@soulik.info>.
 *
 * This file is released under the GPLv2.
 */

#ifndef _LINUX_ROCKCHIP_PM_H
#define _LINUX_ROCKCHIP_PM_H
#include <linux/device.h>

int rockchip_pmu_idle_request(struct device *dev, bool idle);

#endif /* _LINUX_ROCKCHIP_PM_H */

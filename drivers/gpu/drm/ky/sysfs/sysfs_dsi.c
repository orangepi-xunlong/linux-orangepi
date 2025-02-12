// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include "../ky_mipi_panel.h"
#include "../ky_dsi.h"
#include "../ky_dpu.h"
#include "sysfs_display.h"


int ky_dsi_sysfs_init(struct device *dev)
{

	return 0;
}
EXPORT_SYMBOL(ky_dsi_sysfs_init);

MODULE_DESCRIPTION("Provide mipi dsi attribute nodes for userspace");
MODULE_LICENSE("GPL v2");


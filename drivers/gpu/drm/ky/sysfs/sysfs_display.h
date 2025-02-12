// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _SYSFS_DISPLAY_H_
#define _SYSFS_DISPLAY_H_

#include <linux/device.h>

extern struct class *display_class;

int ky_dpu_sysfs_init(struct device *dev);
int ky_dsi_sysfs_init(struct device *dev);
int ky_dphy_sysfs_init(struct device *dev);
int ky_mipi_panel_sysfs_init(struct device *dev);

#endif


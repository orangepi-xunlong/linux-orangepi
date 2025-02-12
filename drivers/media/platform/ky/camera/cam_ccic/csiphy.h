// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for KY CCIC MIPI D-PHY MODULE
 *
 * Copyright(C) 2023 KY Micro Limited.
 */
#ifndef __CSIPHY_H__
#define __CSIPHY_H__

#include "ccic_drv.h"
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/list.h>

struct csiphy_device;
struct mipi_csi2;

struct csiphy_device *csiphy_lookup_by_phandle(struct device *dev, const char *name);
int csiphy_stop(struct csiphy_device *csiphy_dev);
int csiphy_start(struct csiphy_device *csiphy_dev, struct mipi_csi2 *csi);

int ccic_csiphy_register(void);
void ccic_csiphy_unregister(void);
#endif /* ifndef __CSIPHY_H__ */

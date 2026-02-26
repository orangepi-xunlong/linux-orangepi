/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2023 - 2028 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Critical temperature handler for Allwinner SOC
 *
 * Copyright (c) 2022 Allwinnertech Ltd.
 */
#ifndef __SUNXI_CRITICAL_HANDLER_H__
#define __SUNXI_CRITICAL_HANDLER_H__
#include <linux/device.h>
#include "sunxi_thermal.h"

void sunxi_ths_critical_rewrite_ops(struct ths_device *tmdev);
void sunxi_ths_critical_handler(void *sensor_data);
void sunxi_ths_critical_suspend_handler(struct ths_device *tmdev);
void sunxi_ths_critical_resume_handler(struct ths_device *tmdev);

int sunxi_ths_critical_handler_init(struct device *dev, struct ths_device *tmdev);
void sunxi_ths_critical_handler_deinit(void);

#endif

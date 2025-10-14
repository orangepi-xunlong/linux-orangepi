/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ISP_HW_OPS__
#define __ISP_HW_OPS__
#include "armcb_isp.h"
#include "isp_hw_utils.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>

struct isp_hw_cmd_buf {
	struct list_head list;
	struct cmd_buf *cmd;
	void *client;
};

int armcb_isp_hw_read(struct isp_hw_req *req, void *argv);
int armcb_isp_hw_write(struct isp_hw_req *req, void *argv);
int armcb_isp_hw_apply_list(enum cmd_type type);
int armcb_isp_hw_apply(struct cmd_buf *cmd, void *client);
int armcb_sys_bus_test(struct perf_bus_params *puser_bus_params);

#endif

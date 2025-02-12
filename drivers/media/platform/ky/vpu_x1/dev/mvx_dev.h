/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

#ifndef _MVX_DEV_H_
#define _MVX_DEV_H_

#include <linux/reset.h>
#include "mvx_hwreg.h"
#include "mvx_scheduler.h"

/**
 * struct mvx_dev_ctx - Private context for the MVx dev device.
 */
struct mvx_dev_ctx {
	struct device *dev;
	struct mvx_if_ops *if_ops;
	struct mvx_client_ops client_ops;
	struct mvx_hwreg hwreg;
	struct mvx_sched scheduler;
	unsigned int irq;
	struct workqueue_struct *work_queue;
	struct work_struct work;
	unsigned long irqve;
	struct dentry *dentry;
	struct clk* clock;
	struct mutex pm_mutex;
	uint32_t fuses;
	uint32_t ncores;
	enum mvx_hw_id hw_id;
	uint32_t hw_revision;
	uint32_t hw_patch;
	struct reset_control *rst;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_dev_init() - Initialize the dev device.
 */
int mvx_dev_init(void);

/**
 * mvx_dev_exit() - Remove and exit the dev device.
 */
void mvx_dev_exit(void);

#endif /* _MVX_DEV_H_ */

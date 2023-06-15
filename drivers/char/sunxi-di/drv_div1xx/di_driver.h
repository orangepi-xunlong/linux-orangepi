/*
 * Copyright (c) 2020-2031 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DI_DRIVER_H_
#define _DI_DRIVER_H_

#include "di_client.h"

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>

enum di_drv_state {
	DI_DRV_STATE_IDLE = 0,
	DI_DRV_STATE_BUSY,
};

enum di_pm_state {
	DI_PM_STATE_RESUME = 0,
	DI_PM_STATE_SUSPEND,
	DI_PM_STATE_UNKNOWN,
};

struct di_driver_data {
	dev_t devt;
	struct cdev *pcdev;
	struct class *pclass;
	struct device *pdev;

	void __iomem *reg_base;
	u32  irq_no;
	struct clk *clk_source;
	struct clk *clk_bus;
	struct clk *iclk;
	struct reset_control *rst_bus_di;

	struct mutex mlock;
	struct list_head clients;
	int client_cnt;
	bool dev_enable;
	enum di_pm_state pm_state;

	spinlock_t queue_lock;
	u32 front;
	u32 task_cnt; /* client task count in queue */
	struct di_client *queue[DI_TASK_CNT_MAX];
	enum di_drv_state state;

};
#endif /* #ifndef _DI_DRIVER_H_ */

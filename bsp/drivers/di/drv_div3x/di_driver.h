/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
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

#define DI_TASK_CNT_MAX DI_CLIENT_CNT_MAX

typedef int amp_ctrl(void);

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
	struct device *dev;

	void __iomem *reg_base;
	u32  irq_no;
	struct clk *clk_source;
	struct clk *clk_bus;
	struct clk *iclk;
	int iclk_freq;	/* clk frequence */
	struct reset_control *rst_bus_di;
	struct reset_control *rst_bus_desys;

	struct mutex mlock;
	struct list_head clients;
	int client_cnt;
	bool dev_enable;
	enum di_pm_state pm_state;
	struct di_client *pre_client;
	bool need_apply_fixed_para;

	spinlock_t queue_lock;
	u32 front;
	u32 task_cnt; /* client task count in queue */
	struct di_client *queue[DI_TASK_CNT_MAX];
	enum di_drv_state state;
	int master_id;

};

struct di_amp_ctrl {
	amp_ctrl *rv_start;
	amp_ctrl *rv_stop;
	bool pm_state;
	bool rv_irq_state;
	bool iommu_need;
};


int di_drv_get_version(struct di_version *version);
int di_drv_client_inc(struct di_client *c);
int di_drv_client_dec(struct di_client *c);
bool di_drv_is_valid_client(struct di_client *c);
int di_drv_process_fb(struct di_client *c);
int di_drv_clients_set_tnrpara(struct di_client *client, struct tnr_module_param_pqd *data, int update);
int di_drv_get_global_tnr_pqpara(struct tnr_module_param_t *data);

void amp_deinterlace_init(void *amp_ops);
int amp_deinterlace_start_ack(void);
int amp_deinterlace_stop_ack(void);
void amp_deinterlace_exit(void);

#endif /* #ifndef _DI_DRIVER_H_ */

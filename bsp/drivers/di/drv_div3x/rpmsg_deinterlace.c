/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner rpmsg-deinterlace driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include "../common/di_debug.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/workqueue.h>
#include <linux/remoteproc.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/device.h>
#include "di_driver.h"

#define DEINTERLACE_PACKET_MAGIC 0x10244022

extern void sunxi_enable_device_iommu(unsigned int master_id, bool flag);

enum control_status {
	CONTROL_BY_RTOS,
	CONTROL_BY_ARM,
};

enum deinterlace_packet_type {
	DEINTERLACE_START,
	DEINTERLACE_START_ACK,
	DEINTERLACE_STOP,
	DEINTERLACE_STOP_ACK,
};

struct deinterlace_packet {
	u32 magic;
	u32 type;
};


struct rpmsg_deinterlace_private {
	struct di_amp_ctrl *di_ops;
	struct rpmsg_endpoint *ept;
	struct rpmsg_device *rpdev;
	struct mutex lock;
	enum control_status control;
};

struct rpmsg_deinterlace_private *deinterlace;


static int rpmsg_deinterlace_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct deinterlace_packet *pack;
	DI_DEBUG("rpmsg_deinterlace_cb \n");

	pack = (struct deinterlace_packet *)data;

	if (pack->magic != DEINTERLACE_PACKET_MAGIC || len != sizeof(*pack)) {
		DI_ERR("packet invalid magic or size %d %d %x\n",
				len, (int)sizeof(*pack), pack->magic);
		return 0;
	}

	mutex_lock(&deinterlace->lock);
	DI_DEBUG("pack_type %d \n", pack->type);
	if (deinterlace->di_ops == NULL) {
		DI_DEBUG("please pass di_ops to rpmsg \n");
		mutex_unlock(&deinterlace->lock);
		return 0;
	}

	if (pack->type == DEINTERLACE_START && deinterlace->control != CONTROL_BY_RTOS) {
		deinterlace->di_ops->rv_start();
		amp_deinterlace_start_ack();
		deinterlace->control = CONTROL_BY_RTOS;
	} else if (pack->type == DEINTERLACE_STOP && deinterlace->control != CONTROL_BY_ARM) {
		deinterlace->di_ops->rv_stop();
		amp_deinterlace_stop_ack();
		deinterlace->control = CONTROL_BY_ARM;
	}

	mutex_unlock(&deinterlace->lock);

	return 0;
}

static int amp_deinterlace_rpmsg_send(enum deinterlace_packet_type type)
{
	struct deinterlace_packet packet;
	int ret = 0;

	if (!deinterlace->ept)
		ret = -1;

	memset(&packet, 0, sizeof(packet));
	packet.magic = DEINTERLACE_PACKET_MAGIC;
	packet.type = type;

	ret = rpmsg_send(deinterlace->ept, &packet, sizeof(packet));
	DI_INFO("deinterlace send %s %s \n", type == DEINTERLACE_START_ACK ?
			"rtos start" : "rtos stop", ret == 0 ? "ok" : "fail");

	return ret;
}

static int rpmsg_deinterlace_probe(struct rpmsg_device *rpdev)
{
	mutex_lock(&deinterlace->lock);
	deinterlace->ept = rpdev->ept;
	deinterlace->rpdev = rpdev;
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;
	mutex_unlock(&deinterlace->lock);
	return 0;
}

static void rpmsg_deinterlace_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "%s is removed\n", dev_name(&rpdev->dev));
	mutex_lock(&deinterlace->lock);
	deinterlace->ept = NULL;
	mutex_unlock(&deinterlace->lock);
}


static struct rpmsg_device_id rpmsg_driver_hearbeat_id_table[] = {
	{ .name	= "sunxi,rpmsg_deinterlace" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_hearbeat_id_table);

static struct rpmsg_driver rpmsg_deinterlace_client = {
	.drv = {
		.name	= KBUILD_MODNAME,
	},
	.id_table	= rpmsg_driver_hearbeat_id_table,
	.probe		= rpmsg_deinterlace_probe,
	.callback	= rpmsg_deinterlace_cb,
	.remove		= rpmsg_deinterlace_remove,
};

void amp_deinterlace_init(void *ops)
{
	struct rpmsg_deinterlace_private *p;
	p = kzalloc(sizeof(*deinterlace), GFP_KERNEL);
	mutex_init(&p->lock);
	deinterlace = p;
	deinterlace->di_ops = (struct di_amp_ctrl *) ops;
	register_rpmsg_driver(&rpmsg_deinterlace_client);
	deinterlace->control = CONTROL_BY_ARM;
}

void amp_deinterlace_exit(void)
{
	unregister_rpmsg_driver(&rpmsg_deinterlace_client);
	kfree(deinterlace);
}

int amp_deinterlace_start_ack(void)
{
	return amp_deinterlace_rpmsg_send(DEINTERLACE_START_ACK);

}

int amp_deinterlace_stop_ack(void)
{
	return amp_deinterlace_rpmsg_send(DEINTERLACE_STOP_ACK);
}
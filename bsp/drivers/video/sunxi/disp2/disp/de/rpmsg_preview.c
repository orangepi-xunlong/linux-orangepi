/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner rpmsg-preview driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/
#include "include.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/workqueue.h>
#include <linux/remoteproc.h>
#include <linux/timer.h>
#include <linux/types.h>

#define PREVIEW_PACKET_MAGIC	0x10244021

struct rpmsg_preview_private {
	struct rpmsg_endpoint *ept;
	struct rpmsg_device *rpdev;
	struct mutex lock;
};

enum preview_packet_type {
	PREVIEW_START,
	PREVIEW_START_ACK,
	PREVIEW_STOP,
	PREVIEW_STOP_ACK,
};

struct preview_packet {
	u32 magic;
	u32 type;
};

struct rpmsg_preview_private *preview;

int disp_mgr_enter_preview(void);
int disp_mgr_exit_preview(void);

static int rpmsg_preview_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct preview_packet *pack = data;
//	struct disp_manager *mgr = disp_get_layer_manager(0);

	if (pack->magic != PREVIEW_PACKET_MAGIC || len != sizeof(*pack)) {
		DE_DEV_ERR(&rpdev->dev, "packet invalid magic or size %d %d %x\n",
				len, (int)sizeof(*pack), pack->magic);
		return 0;
	}
	dev_info(&rpdev->dev, "receive %s\n", pack->type == PREVIEW_START ?
		"preview start" : "preview stop");

	switch (pack->type) {
	case PREVIEW_START:
		disp_mgr_enter_preview();
		break;
	case PREVIEW_STOP:
		disp_mgr_exit_preview();
		break;
	default:
		DE_DEV_ERR(&rpdev->dev, "unknown packet, do nothing\n");
		break;
	}

	return 0;
}

static int rpmsg_preview_probe(struct rpmsg_device *rpdev)
{
	mutex_lock(&preview->lock);
	preview->ept = rpdev->ept;
	preview->rpdev = rpdev;
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;
	mutex_unlock(&preview->lock);
	return 0;
}

static void rpmsg_preview_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "%s is removed\n", dev_name(&rpdev->dev));
	mutex_lock(&preview->lock);
	preview->ept = NULL;
	mutex_unlock(&preview->lock);
}

static int amp_preview_rpmsg_send(enum preview_packet_type type)
{
	struct preview_packet packet;
	int ret = 0;

	mutex_lock(&preview->lock);
	if (!preview->ept)
		ret = -ENODEV;
	mutex_unlock(&preview->lock);
	if (ret)
		return ret;

	memset(&packet, 0, sizeof(packet));
	packet.magic = PREVIEW_PACKET_MAGIC;
	packet.type = type;
	ret = rpmsg_send(preview->ept, &packet, sizeof(packet));
	dev_info(&preview->rpdev->dev, "send %s %s\n", type == PREVIEW_START_ACK ?
		"preview start ack" : "preview stop ack", ret == 0 ? "ok" : "fail");

	return ret;
}

static struct rpmsg_device_id rpmsg_driver_hearbeat_id_table[] = {
	{ .name	= "sunxi,rpmsg_preview" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_hearbeat_id_table);

static struct rpmsg_driver rpmsg_preview_client = {
	.drv = {
		.name	= KBUILD_MODNAME,
	},
	.id_table	= rpmsg_driver_hearbeat_id_table,
	.probe		= rpmsg_preview_probe,
	.callback	= rpmsg_preview_cb,
	.remove		= rpmsg_preview_remove,
};

void amp_preview_init(void)
{
	struct rpmsg_preview_private *p;
	p = kzalloc(sizeof(*preview), GFP_KERNEL);
	mutex_init(&p->lock);
	preview = p;
	register_rpmsg_driver(&rpmsg_preview_client);
}

void amp_previe_exit(void)
{
	unregister_rpmsg_driver(&rpmsg_preview_client);
}

int amp_preview_start_ack(void)
{
	return amp_preview_rpmsg_send(PREVIEW_START_ACK);
}

int amp_preview_stop_ack(void)
{
	return amp_preview_rpmsg_send(PREVIEW_STOP_ACK);
}



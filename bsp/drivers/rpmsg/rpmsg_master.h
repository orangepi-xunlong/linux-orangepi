/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi's rpmsg ctrl driver
 *
 * the driver register the rpmsg_ctrl device node,which
 * controls the creation and release of rpmsg device nodes.
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef RPMSG_MASTER_H_
#define RPMSG_MASTER_H_

#include <uapi/linux/rpmsg.h>
#include "rpmsg_internal.h"

#define RPMSG_DEV_MAX	(MINORMASK + 1)

/* rpmsg ctrl cmd, arm and remote core need be consistent */
#define RPMSG_ACK_OK                0x13131411

#define RPMSG_ACK_FAILED            0x13131412
#define RPMSG_ACK_NOLISTEN          0x13131413
#define RPMSG_ACK_BUSY              0x13131414
#define RPMSG_ACK_NOMEM             0x13131415
#define RPMSG_ACK_NOENT             0x13131416

#define RPMSG_CREATE_CLIENT         0x13141413
#define RPMSG_CLOSE_CLIENT          0x13141414 /* host release */
#define RPMSG_RELEASE_CLIENT        0x13141415 /* client release */

/* Destroy all endpoint belonging to info.name */
#define RPMSG_RESET_GRP_CLIENT      0x12131516
#define RPMSG_RESET_ALL_CLIENT      0x14151617

#define __pack					__attribute__((__packed__))

struct rpmsg_ctrl_msg {
	char name[32];
	uint32_t id;
	uint32_t ctrl_id;
	uint32_t cmd;
} __pack;

struct rpmsg_ctrl_msg_ack {
	uint32_t id;
	uint32_t ack;
} __pack;

extern dev_t rpmsg_ctrldev_get_devt(void);
extern void rpmsg_ctrldev_put_devt(dev_t devt);
extern int rpmsg_ctrldev_notify(int ctrl_id, int id);
#if IS_ENABLED(CONFIG_AW_KERNEL_ORIGIN)
extern struct class *aw_rpmsg_class;
#endif
/**
 * RPMSG_CREATE_EPT_IOCTL:
 *     Create the endpoint specified by info.name,
 *     updates info.id.
 * RPMSG_DESTROY_EPT_IOCTL:
 *     Destroy the endpoint specified by info.id.
 * RPMSG_REST_EPT_GRP_IOCTL:
 *     Destroy all endpoint belonging to info.name
 * RPMSG_DESTROY_ALL_EPT_IOCTL:
 *     Destroy all endpoint
 */
#define RPMSG_CREATE_EPT_IOCTL	_IOW(0xb5, 0x1, struct rpmsg_endpoint_info)
#define RPMSG_DESTROY_EPT_IOCTL	_IO(0xb5, 0x2)
#define RPMSG_REST_EPT_GRP_IOCTL	_IO(0xb5, 0x3)
#define RPMSG_DESTROY_ALL_EPT_IOCTL	_IO(0xb5, 0x4)

/**
 * struct rpmsg_ctrl_msg - used by rpmsg_master.c
 * @name: user define
 * @id: update by driver
 * @cmd:only can RPMSG_CTRL_OPEN or RPMSG_CTRL_CLOSE
 * */
struct rpmsg_ept_info {
	char name[32];
	uint32_t id;
};

#endif

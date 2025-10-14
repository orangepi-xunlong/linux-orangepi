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

#ifndef __ARMCB_V4L_SD_H__
#define __ARMCB_V4L_SD_H__

#include "types_utils.h"
#include <linux/completion.h>
#include <linux/i2c.h>
#include <linux/iommu.h>
#include <linux/list.h>
#include <linux/pm_qos.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <media/v4l2-async.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-dma-contig.h>

#define ARMCB_IOCTL_CMD_PRIVATE_START \
	(BASE_VIDIOC_PRIVATE) /* 192-255 are private */

#define ARMCB_VIDIOC_S_REG_LIST (ARMCB_IOCTL_CMD_PRIVATE_START + 0)
#define ARMCB_VIDIOC_G_REG_LIST (ARMCB_IOCTL_CMD_PRIVATE_START + 1)

#define ARMCB_IMGSENS_S_INIT (ARMCB_IOCTL_CMD_PRIVATE_START + 8)
#define ARMCB_IMGSENS_S_SENSMODE (ARMCB_IOCTL_CMD_PRIVATE_START + 9)
#define ARMCB_IMGSENS_S_PARAMS_D (ARMCB_IOCTL_CMD_PRIVATE_START + 10)

#define ARMCB_CAMERA_SUBDEV_BASE (MEDIA_ENT_F_OLD_BASE + 0xF00)
#define ARMCB_CAMERA_SUBDEV_STATISTIC (ARMCB_CAMERA_SUBDEV_BASE + 0)
#define ARMCB_CAMERA_SUBDEV_ISP (ARMCB_CAMERA_SUBDEV_BASE + 1)
#define ARMCB_CAMERA_SUBDEV_CSI (ARMCB_CAMERA_SUBDEV_BASE + 2)
#define ARMCB_CAMERA_SUBDEV_IMGSENS (ARMCB_CAMERA_SUBDEV_BASE + 3)
#define ARMCB_CAMERA_SUBDEV_MOTOR (ARMCB_CAMERA_SUBDEV_BASE + 4)
#define ARMCB_HDMI_SUBDEV_DISPLAY (ARMCB_CAMERA_SUBDEV_BASE + 0x100)

/*
 * This is used to install Sequene in armchia_sd_register.
 * During armcb_close, properclose sequence will be triggered
 * For example:
 *
 *close_sequence = 0x00100001 (ISP)
 *close_sequence = 0x00100002 (ISP)
 *close_sequence = 0x00100003 (ISP)
 *close_sequence = 0x00200001 (Sensor)
 *close_sequence = 0x00200002 (Sensor)
 *close_sequence = 0x00200003 (Sensor)
 */
#define ARMCB_SD_CLOSE_1ST_CATEGORY 0x00010000
#define ARMCB_SD_CLOSE_2ND_CATEGORY 0x00020000
#define ARMCB_SD_CLOSE_3RD_CATEGORY 0x00030000
#define ARMCB_SD_CLOSE_4TH_CATEGORY 0x00040000

#define ARMCB_SUBDEV_NOTIFY_GET 0x1
#define ARMCB_SUBDEV_NOTIFY_PUT 0x2
#define ARMCB_SUBDEV_NOTIFY_REQ 0x3

struct armcb_sd_subdev {
	struct v4l2_subdev sd;
	int close_seq;
	struct list_head list;
};

struct armcb_subdev_req {
	char *name;
	struct v4l2_subdev *subdev;
};

enum ARMCB_SUBDEV_NODE_IDX {
	ARMCB_SUBDEV_NODE_HW_STATISTIC = 0,
	ARMCB_SUBDEV_NODE_HW_ISP,
	ARMCB_SUBDEV_NODE_HW_IMGSENS0,
	ARMCB_SUBDEV_NODE_HW_MOTOR0,
	ARMCB_SUBDEV_NODE_HW_CSI,
	ARMCB_SUBDEV_NODE_HW_DISP,
	ARMCB_SUBDEV_NODE_IDX_END
};

int armcb_subdev_register(struct armcb_sd_subdev *armcb_sdreg,
			  unsigned int cam_id);
int armcb_subdev_unregister(struct armcb_sd_subdev *armcb_sdreg);
void armcb_v4l2_subdev_notify(struct v4l2_subdev *sd, unsigned int notification,
			      void *arg);

int armcb_camera_async_complete(struct v4l2_async_notifier *notifier);
#endif //__ARMCB_V4L2_SUBDEV_H__

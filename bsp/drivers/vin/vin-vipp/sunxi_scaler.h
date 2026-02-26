/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _SUNXI_SCALER_H_
#define _SUNXI_SCALER_H_
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#if !defined VIPP_200
#include "vipp100/vipp_reg.h"
#else
#include "vipp200/vipp200_reg.h"
#endif

#define VIPP_ONLY_SHRINK 1

enum scaler_pad {
	SCALER_PAD_SINK,
	SCALER_PAD_SOURCE,
	SCALER_PAD_NUM,
};

enum vipp_work_mode {
	VIPP_ONLINE = 0,
	VIPP_OFFLINE = 1,
};

struct scaler_para {
	u32 xratio;
	u32 yratio;
	u32 w_shift;
	u32 width;
	u32 height;
};

struct scaler_dev {
	struct v4l2_subdev subdev;
	struct media_pad scaler_pads[SCALER_PAD_NUM];
	struct v4l2_mbus_framefmt formats[SCALER_PAD_NUM];
	struct platform_device *pdev;
	struct mutex subdev_lock;
	struct vin_mm vipp_reg;
	struct vin_mm osd_para;
	struct vin_mm osd_stat;
	struct {
		struct v4l2_rect request;
		struct v4l2_rect active;
	} crop;
	struct scaler_para para;
	void __iomem *base;
	unsigned char is_empty;
	unsigned char noneed_register;
	unsigned char id;
	spinlock_t slock;
	u8 large_image;
	unsigned int delay_init;
	unsigned int delay_para_ready;
#if defined VIPP_200
	struct vin_mm load_para[2];
	bool load_select; /*load_select = 0 select load_para[0], load_select = 1 select load_para[1]*/
	int irq;
	unsigned char is_irq_empty;
	unsigned int logic_top_stream_count;
	unsigned int work_mode;
#endif
	unsigned int stream_count;
};

int sunxi_scaler_subdev_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
struct v4l2_subdev *sunxi_scaler_get_subdev(int id);
int sunxi_vipp_get_osd_stat(int id, unsigned int *stat);
int sunxi_scaler_platform_register(void);
void sunxi_scaler_platform_unregister(void);

#endif /* _SUNXI_SCALER_H_ */

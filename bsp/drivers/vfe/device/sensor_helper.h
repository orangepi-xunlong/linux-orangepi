/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/device/sensor_helper.h
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

/*
 * sensor helper header file
 *
 */
#ifndef __SENSOR__HELPER__H__
#define __SENSOR__HELPER__H__

#include <media/v4l2-subdev.h>
#include <linux/videodev2.h>
#include "../vfe.h"
#include "../vfe_subdev.h"
#include "../csi_cci/cci_helper.h"
#include "camera_cfg.h"
#include "../platform_cfg.h"

#define REG_DLY  0xffff

struct regval_list {
	addr_type addr;
	data_type data;
};

extern int sensor_read(struct v4l2_subdev *sd, addr_type addr,  data_type *value);
extern int sensor_write(struct v4l2_subdev *sd, addr_type addr, data_type value);
extern int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *regs, int array_size);
extern int sensor_get_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt);
extern int sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt);
extern int sensor_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *parms);
extern int sensor_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *parms);
extern struct sensor_info *to_state(struct v4l2_subdev *sd);

/* extern int sensor_power(struct v4l2_subdev *sd, int on); */

#endif /* __SENSOR__HELPER__H__ */

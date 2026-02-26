/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/*
 * sensor_helper.h: helper function for sensors.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SENSOR__HELPER__H__
#define __SENSOR__HELPER__H__

#include <media/v4l2-subdev.h>
#include <linux/videodev2.h>
#include "../../vin-video/vin_core.h"
#include "../../utility/vin_supply.h"
#include "../../vin-cci/cci_helper.h"
#include "camera_cfg.h"
#include "camera.h"
#include "../flash/flash.h"
#include "../../platform/platform_cfg.h"

#define REG_DLY  0xffff

struct regval_list {
	addr_type addr;
	data_type data;
};

struct sensor_helper_dev {
	struct regulator *pmic[MAX_POW_NUM];
	char name[I2C_NAME_SIZE];
	unsigned int id;
};

extern int sensor_read(struct v4l2_subdev *sd, addr_type addr,
		       data_type *value);
extern int sensor_write(struct v4l2_subdev *sd, addr_type addr,
			data_type value);
extern int sensor_write_array(struct v4l2_subdev *sd,
				struct regval_list *regs,
				int array_size);
#if IS_ENABLED(CONFIG_COMPAT)
extern long sensor_compat_ioctl32(struct v4l2_subdev *sd,
		unsigned int cmd, unsigned long arg);
#endif
extern struct sensor_info *to_state(struct v4l2_subdev *sd);
extern void sensor_cfg_req(struct v4l2_subdev *sd, struct sensor_config *cfg);
extern void sensor_isp_input(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf);
extern unsigned int sensor_get_exp(struct v4l2_subdev *sd);
extern unsigned int sensor_get_clk(struct v4l2_subdev *sd, struct v4l2_mbus_config *cfg,
			unsigned long *top_clk, unsigned long *isp_clk);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
extern int sensor_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *state,
				struct v4l2_subdev_mbus_code_enum *code);
extern int sensor_enum_frame_size(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *state,
			struct v4l2_subdev_frame_size_enum *fse);
extern int sensor_enum_frame_interval(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *state,
			struct v4l2_subdev_frame_interval_enum *fie);
extern int sensor_get_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *state,
			struct v4l2_subdev_format *fmt);
extern int sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *state,
			struct v4l2_subdev_format *fmt);
extern int sensor_set_ir(struct v4l2_subdev *sd, struct ir_switch *ir_switch);
extern int sensor_ptn_init(struct v4l2_subdev *sd, struct vin_pattern_config *ptn);
#else
extern int sensor_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_mbus_code_enum *code);
extern int sensor_enum_frame_size(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_frame_size_enum *fse);
extern int sensor_enum_frame_interval(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_frame_interval_enum *fie);
extern int sensor_get_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt);
extern int sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt);
#endif
extern void sensor_get_resolution(struct v4l2_subdev *sd, struct sensor_resolution *sensor_resolution);
extern int sensor_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *parms);
extern int sensor_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *parms);
extern int sensor_try_ctrl(struct v4l2_ctrl *ctrl);

extern int actuator_init(struct v4l2_subdev *sd, struct actuator_para *range);
extern int actuator_set_code(struct v4l2_subdev *sd, struct actuator_ctrl *pos);
extern int flash_en(struct v4l2_subdev *sd, struct flash_para *para);

#endif

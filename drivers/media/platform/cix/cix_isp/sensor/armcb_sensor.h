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
#ifndef __ARMCHINA_SENSOR_INIT_H__
#define __ARMCHINA_SENSOR_INIT_H__

#include <linux/clk.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <media/videobuf2-v4l2.h>

#include "armcb_v4l2_core.h"
#include "armcb_v4l_sd.h"

// Resolution
enum armcb_imgsens_resolution_fps {
	ARMCB_IMGSENS_RES_START = 0,
	ARMCB_IMGSENS_RES720P30,
	ARMCB_IMGSENS_RES720P60,
	ARMCB_IMGSENS_RES1080P30,
	ARMCB_IMGSENS_RES1080P60,
	ARMCB_IMGSENS_RES1080I50,
	ARMCB_IMGSENS_RES1080P50,
	ARMCB_IMGSENS_RES960P30,

	ARMCB_IMGSENS_RESOLUTION_TOTAL,
};

enum armcb_imgsens_interface_channel {
	ARMCB_IMGSENS_INTERFACE_CHANNEL_0 = 0,
	ARMCB_IMGSENS_INTERFACE_CHANNEL_1,
	ARMCB_IMGSENS_INTERFACE_CHANNEL_2,
	ARMCB_IMGSENS_INTERFACE_CHANNEL_3,
	ARMCB_IMGSENS_INTERFACE_CHANNEL_MAX,
};

enum armcb_imgsens_scene_mode {
	ARMCB_IMGSENS_SCENE_MODE_NORMAL = 0,
	ARMCB_IMGSENS_SCENE_MODE_DOL2,
	ARMCB_IMGSENS_SCENE_MODE_DOL3,
	ARMCB_IMGSENS_SCENE_SWITCH_3MODE,
	ARMCB_IMGSENS_SCENE_SWITCH_2MODE,
	ARMCB_IMGSENS_SCENE_MODE_INVALID,

	ARMCB_IMGSENS_SCENE_MODE_NUM_MAX,
};

#define ARMCB_IMGSENS_NUM_MAX (1)
#define IMGSENS_I2C_DRVNAME "imgsensor0"

struct armcb_imgsen_info {
	u8 uchl;
	u8 slvaddr;
	u16 i2c_bus;
	u16 sensorid;
	u32 regtype;
};

enum armcb_imgsens_power_mode {
	ARMCB_IMGSENS_POWER_MODE_NULL = 0,
	ARMCB_IMGSENS_POWER_MODE_PMIC,
	ARMCB_IMGSENS_POWER_MODE_GPIO,
	ARMCB_IMGSENS_POWER_MODE_NUM_MAX,
};

struct armcb_camera_inst {
	struct i2c_client *pi2c_client;
	struct armcb_imgsen_info imgsen_info;

	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_gpio;
	struct clk *mclk;
	struct gpio_desc *pwn_gpio;
	struct gpio_desc *rst_gpio;
	struct gpio_desc *pwren_gpio;
	struct gpio_desc *pwren0_gpio;
};

struct armcb_imgsens_subdev {
	struct mutex imutex;
	spinlock_t sdlock;

	struct platform_device *ppdev;
	struct device *pdev;
	struct device_node *of_node;
	struct armcb_camera_inst imgs_inst;
	struct armcb_i2c *i2c_id;
	armcb_v4l2_dev_t *armcb_v4l2_dev;

	struct armcb_sd_subdev imgsens_sd;
	struct v4l2_subdev_ops *imgsens_subdev_ops;
	unsigned int dev_caps;
	unsigned int id;
	unsigned int cam_id;

	int module_init_status;
	wait_queue_head_t state_wait;
	struct regulator *vsupply_0;
	struct regulator *vsupply_1;
	BOOL power_on;
};

struct armcb_i2c_camsensor {
	u8 cursenhwchnl;
	struct armcb_camera_inst camera[ARMCB_IMGSENS_INTERFACE_CHANNEL_MAX];
};

#define MAX_SUBDEV_NUM (10)

#ifdef ARMCB_CAM_KO
void *armcb_get_sensor_driver_instance(void);
void armcb_sensor_driver_destroy(void);
#endif
#endif // __ARMCHINA_SENSOR_INIT_H_

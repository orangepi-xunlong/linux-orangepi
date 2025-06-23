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

#ifndef __ARMCB_ACTUATOR_H__
#define __ARMCB_ACTUATOR_H__

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/media.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/of_graph.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>

#include "armcb_v4l2_core.h"
#include "armcb_v4l_sd.h"
#include "types_utils.h"
#include <media/media-entity.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mediabus.h>

#define CIX_I2C_ACTUATOR
struct armcb_motor_subdev {
	struct mutex imutex;
	spinlock_t sdlock;

	struct platform_device *ppdev;
	struct device *pdev;
	struct device_node *of_node;

	struct armcb_sd_subdev motor_sd;
	struct v4l2_subdev_ops *motor_subdev_ops;
#ifdef CIX_I2C_ACTUATOR
	struct i2c_client *pcline_dev;
#else
	struct spi_device *pspi_dev;
#endif
	unsigned int dev_caps;
	unsigned int id;
	unsigned int cam_id;

	int module_init_status;
};

#ifdef ARMCB_CAM_KO
void *armcb_get_motor_driver_instance(void);
void armcb_motor_driver_destroy(void);
#endif

#endif //__ARMCB_ACTUATOR_H__

// SPDX-License-Identifier: GPL-2.0
/*
 * vsensor.h - virtual sensor funcitons
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _KY_VSENSOR_H_
#define _KY_VSENSOR_H_
#include <media/v4l2-device.h>
#include <linux/v4l2-subdev.h>
#include <media/media-entity.h>
#include "mlink.h"
#include "subdev.h"

enum {
	//SENSOR_PAD_RAWDUMP0 = 0,
	//SENSOR_PAD_RAWDUMP1,
	//SENSOR_PAD_PIPE0,
	//SENSOR_PAD_PIPE1,
	SENSOR_PAD_CSI_MAIN,
	SENSOR_PAD_CSI_VCDT,
	SENSOR_PAD_NUM
};

struct spm_camera_sensor {
	struct spm_camera_subdev sc_subdev;
	struct media_pad pads[SENSOR_PAD_NUM];
	struct v4l2_subdev_format pad_fmts[SENSOR_PAD_NUM];
	int idx;
	void *usr_data;
};

static inline struct spm_camera_sensor* media_entity_to_sc_sensor(struct media_entity *me)
{
	if (!is_sensor(me))
		return NULL;
	return (struct spm_camera_sensor *)me;
}

static inline struct spm_camera_sensor *v4l2_subdev_to_sc_sensor(struct v4l2_subdev *sd)
{
	if (!is_sensor(&(sd->entity)))
		return NULL;
	return (struct spm_camera_sensor *)sd;
}

struct spm_camera_sensor *spm_sensor_create(unsigned int grp_id, struct device *dev);
#endif

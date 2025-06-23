// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#define pr_fmt(fmt)	"<sensorlist> " fmt

#include <linux/module.h>

#include "sensor_list.h"
#include "hf_sensor_type.h"

int sensorlist_sensor_to_handle(int sensor)
{
	int handle = -1;

	switch (sensor) {
	case SENSOR_TYPE_ACCELEROMETER:
		handle = accel_handle;
		break;
	case SENSOR_TYPE_GYROSCOPE:
		handle = gyro_handle;
		break;
	case SENSOR_TYPE_MAGNETIC_FIELD:
		handle = mag_handle;
		break;
	case SENSOR_TYPE_LIGHT:
		handle = als_handle;
		break;
	case SENSOR_TYPE_PROXIMITY:
		handle = ps_handle;
		break;
	case SENSOR_TYPE_PRESSURE:
		handle = baro_handle;
		break;
	}
	return handle;
}

int sensorlist_handle_to_sensor(int handle)
{
	int type = -1;

	switch (handle) {
	case accel_handle:
		type = SENSOR_TYPE_ACCELEROMETER;
		break;
	case gyro_handle:
		type = SENSOR_TYPE_GYROSCOPE;
		break;
	case mag_handle:
		type = SENSOR_TYPE_MAGNETIC_FIELD;
		break;
	case als_handle:
		type = SENSOR_TYPE_LIGHT;
		break;
	case ps_handle:
		type = SENSOR_TYPE_PROXIMITY;
		break;
	case baro_handle:
		type = SENSOR_TYPE_PRESSURE;
		break;
	}
	return type;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("dynamic sensorlist driver");

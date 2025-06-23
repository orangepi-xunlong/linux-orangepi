// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef _SENSOR_LIST_H_
#define _SENSOR_LIST_H_

enum sensorlist {
	accel_handle,
	gyro_handle,
	mag_handle,
	als_handle,
	ps_handle,
	baro_handle,
	sar_handle,
	maxhandle,
};

int sensorlist_sensor_to_handle(int sensor);
int sensorlist_handle_to_sensor(int handle);

#endif

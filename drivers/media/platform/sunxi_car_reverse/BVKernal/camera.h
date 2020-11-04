/*
 * Fast car reverse head file
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Golden.Ye <golden@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

/*Define camera structure*/

/*Distoration table*/
typedef struct _Distoration_t {
	float angle;
	float realHeight;
	float refHeight;
	float distoration;
} Distoration_t, *pDistoration_t;

/*Lens define*/
typedef struct _Lens_t {
	float focal;
	float fovH;
	uint32 numDis;
	pDistoration_t pDis;
} Lens_t, *pLens_t;

/*Lens type*/
typedef enum _LensType_t {
	LensType_8255,
	LensType_4052,
	LensType_9005,
	LensType_8151,
	LensType_8192,
	LensType_6001,
	LensType_9090,
	LensType_6026,
	LensType_8077,       /* D1 */
	LensType_HaiLin4052, /* D1 */
	LensTypeAmount
} LensType_t;

/*Sensor define*/
typedef struct _Sensor_t {
	uint16 sensorH;
	uint16 sensorW;
	uint16 activeH;
	uint16 activeW;

	float pixH;
	float pixV;
} Sensor_t, *pSensor_t;

typedef enum _SensorType_t {
	SensorType_0130,
	SensorType_1133,
	SensorType_225,
	SensorType_H65,
	SensorType_1089,  /* D1 */
	SensorType_1099N, /* D1 */
	SensorTypeAmount
} SensorType_t;

/*Camera define*/
typedef struct _Camera_t {
	uint16 outH;
	uint16 outW;
	float centerX;
	float centerY;
	float scaleH;
	float scaleV;
	uint16 mirror;
	uint16 reserve;

	SensorType_t sensorType;
	LensType_t lensType;
} Camera_t, *pCamera_t;

/*public function for exterl used to get sensor and lens data*/
extern pSensor_t GetSensor(SensorType_t type);
extern pLens_t GetLens(LensType_t type);

#endif

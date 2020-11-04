/*
 * A V4L2 driver for nvp6158c yuv cameras.
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng Zequn<zequnzheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _MOTION_H_
#define _MOTION_H_

#include "common.h"

#define FUNC_ON      0x01
#define FUNC_OFF     0x00

typedef struct _motion_mode{
	unsigned char ch;
	unsigned char devnum;
	unsigned char set_val;

	unsigned char fmtdef;
} motion_mode;

void motion_nvp6158_onoff_set(motion_mode *motion_set);
void motion_nvp6158_display_onoff_set(motion_mode *motion_set);
void motion_nvp6158_pixel_all_onoff_set(motion_mode *motion_set);
void motion_nvp6158_pixel_onoff_set(motion_mode *motion_set);
void motion_nvp6158_pixel_onoff_get(motion_mode *motion_set);
void motion_nvp6158_tsen_set(motion_mode *motion_set);
void motion_nvp6158_psen_set(motion_mode *motion_set);
void motion_nvp6158_detection_get(motion_mode *motion_set);

#endif /* _MOTION_H_ */

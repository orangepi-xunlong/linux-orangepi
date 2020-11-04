/*
 * A V4L2 driver for nvp6324 cameras and AHD Coax protocol.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Li Huiyu <lihuiyu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MOTION_H_
#define _MOTION_H_

#include "jaguar1_common.h"

typedef struct _motion_mode{
	unsigned char ch;
	unsigned char devnum;
	unsigned char set_val;

	unsigned char fmtdef;
} motion_mode;

void motion_onoff_set(motion_mode *motion_set);
void motion_display_onoff_set(motion_mode *motion_set);
void motion_pixel_all_onoff_set(motion_mode *motion_set);
void motion_pixel_onoff_set(motion_mode *motion_set);
void motion_pixel_onoff_get(motion_mode *motion_set);
void motion_tsen_set(motion_mode *motion_set);
void motion_psen_set(motion_mode *motion_set);
void motion_detection_get(motion_mode *motion_set);

#endif /* _MOTION_H_ */

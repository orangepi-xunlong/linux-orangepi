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

#ifndef _JAGUAR1_CLOCK_
#define _JAGUAR1_CLOCK_

#include "jaguar1_video.h"

#define VD_DATA_TYPE_YUV422         (0x01)
#define VD_DATA_TYPE_YUV420         (0x02)
#define VD_DATA_TYPE_LEGACY420      (0x03)

typedef struct _mipi_vdfmt_set_s {
	unsigned char arb_scale;
	unsigned char mipi_frame_opt;
} mipi_vdfmt_set_s;

void arb_init(int dev_num);
int mipi_datatype_set(unsigned char data_type);
void mipi_tx_init(int dev_num);
void mipi_video_format_set(video_input_init *dev_ch_info);
void disable_parallel(int dev_num);
#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _FB_G2D_ROT_H
#define _FB_G2D_ROT_H

#include "disp_sys_intf.h"
#include <uapi/linux/sunxi-g2d.h>
#include "de/disp_features.h"
#include <uapi/video/sunxi_display2.h>
#include "dev_disp.h"
#include <linux/fb.h>

struct g2d_rot_create_info {
	unsigned int dst_width;
	unsigned int dst_height;
	unsigned int src_size;
	enum disp_pixel_format format;
	unsigned long long phy_addr;
	int rot_degree;
};

int fb_g2d_degree_to_int_degree(int in);
int fb_g2d_get_new_rot_degree(int rot_config, int rot_request);
void *fb_g2d_rot_init(struct g2d_rot_create_info *info, struct disp_layer_config *config);
int fb_g2d_rot_exit(void *rot);
int fb_g2d_rot_apply(void *rot, struct disp_layer_config *config, unsigned int yoffset);
int fb_g2d_set_degree(void *rot, int degree, struct disp_layer_config *config);
void fb_g2d_get_rot_size(int rot_degree, int *width, int *height);
bool fb_g2d_check_rotate(__u32 *rot);
#endif /* End of file */

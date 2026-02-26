/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * logo/logo.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
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
#ifndef _LOGO_H
#define _LOGO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "include.h"

struct bmp_pad_header {
	char data[2];		/* pading 2 byte */
	char signature[2];
	u32 file_size;
	u32 reserved;
	u32 data_offset;
	/* InfoHeader */
	u32 size;
	u32 width;
	u32 height;
	u16 planes;
	u16 bit_count;
	u32 compression;
	u32 image_size;
	u32 x_pixels_per_m;
	u32 y_pixels_per_m;
	u32 colors_used;
	u32 colors_important;
} __packed;

struct bmp_header {
	/* Header */
	char signature[2];
	u32 file_size;
	u32 reserved;
	u32 data_offset;
	/* InfoHeader */
	u32 size;
	u32 width;
	u32 height;
	u16 planes;
	u16 bit_count;
	u32 compression;
	u32 image_size;
	u32 x_pixels_per_m;
	u32 y_pixels_per_m;
	u32 colors_used;
	u32 colors_important;
	/* ColorTable */
} __packed;

int logo_parse(struct fb_info *info);

#ifdef __cplusplus
}
#endif

#endif /* End of file */

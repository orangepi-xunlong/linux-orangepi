/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __SUNXI_METADATA_H__
#define __SUNXI_METADATA_H__
#include <linux/types.h>

enum metadata_flag {
	/* hdr static metadata is available */
	DEC_METADATA_FLAG_HDR_SATIC_METADATA   = 0x00000001,
	/* hdr dynamic metadata is available */
	DEC_METADATA_FLAG_HDR_DYNAMIC_METADATA = 0x00000002,

	/* afbc header data is available */
	DEC_METADATA_FLAG_AFBC_HEADER          = 0x00000010,
};

struct afbc_header {
	unsigned int signature;
	unsigned short filehdr_size;
	unsigned short version;
	unsigned int body_size;
	unsigned char ncomponents;
	unsigned char header_layout;
	unsigned char yuv_transform;
	unsigned char block_split;
	unsigned char inputbits[4];
	unsigned short block_width;
	unsigned short block_height;
	unsigned short width;
	unsigned short height;
	unsigned char  left_crop;
	unsigned char  top_crop;
	unsigned short block_layout;
};

struct display_master_data {
	/* display primaries */
	unsigned short display_primaries_x[3];
	unsigned short display_primaries_y[3];

	/* white_point */
	unsigned short white_point_x;
	unsigned short white_point_y;

	/* max/min display mastering luminance */
	unsigned int max_display_mastering_luminance;
	unsigned int min_display_mastering_luminance;
};

/* static metadata type 1 */
struct hdr_static_metadata {
	struct display_master_data disp_master;

	unsigned short maximum_content_light_level;
	unsigned short maximum_frame_average_light_level;
};

/* sunxi video metadata for ve and de */
struct dec_metadata {
	struct hdr_static_metadata hdr_smetada;
	struct afbc_header afbc_head;
	unsigned short fps;
	unsigned short width;
	unsigned short height;
	unsigned short interlace;//1:interlace, 0:progressive
	unsigned int format;
	unsigned int colorspace;
	unsigned int reserve[20];
};

#endif /* #ifndef __SUNXI_METADATA_H__ */

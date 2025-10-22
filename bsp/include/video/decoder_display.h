/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * decoder_display.h
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
#ifndef _DECODER_DISPLAY_H
#define _DECODER_DISPLAY_H

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dec_rectz {
	unsigned int width;
	unsigned int height;
};

enum dec_pixel_format_t {
	/*NV12,yuv420 semi planer, uv combine*/
	DEC_FORMAT_YUV420P = 0,
	/*P010*/
	DEC_FORMAT_YUV420P_10BIT = 1,
	/*NV16, yuv422 semi planer, uv combine*/
	DEC_FORMAT_YUV422P = 2,
	/*P210*/
	DEC_FORMAT_YUV422P_10BIT = 3,
	DEC_FORMAT_YUV444P = 4,
	DEC_FORMAT_YUV444P_10BIT = 5,
	DEC_FORMAT_RGB888 = 6,

	/*
	 * Different form microsoft P010 which the lowest 6 bits set to zero.
	 * AV1 10bit use the bit0 ~ bit9 as valid data, and set the bit10 ~ bit15 as zero!
	 */
	DEC_FORMAT_YUV420P_10BIT_AV1 = 20,

	DEC_FORMAT_MAX = DEC_FORMAT_YUV420P_10BIT_AV1,
};

enum dec_interlace_field_mode_t {
	BOTTOM_FIELD_FIRST = 0,
	TOP_FIELD_FIRST = 1,
};

struct dec_frame_config {
	_Bool fbd_en;
	_Bool blue_en;
	/*body data without afbd header*/
	int image_fd;
	enum dec_pixel_format_t format;
	unsigned long long image_addr[3];
	struct dec_rectz image_size[3];
	unsigned int image_align[3];
	int metadata_fd;
	unsigned int metadata_size;
	unsigned int metadata_flag;
	unsigned long long metadata_addr;
	_Bool interlace_en;
	int field_mode;
	_Bool use_phy_addr;
};

struct dec_vsync_timestamp {
    int64_t timestamp;
};

struct dec_video_buffer_data {
	uint32_t phy_address;
	uint32_t size;
	uint64_t device_address;
};

struct overscan_info {
	uint32_t display_top_offset;
	uint32_t display_bottom_offset;
	uint32_t display_left_offset;
	uint32_t display_right_offset;

	struct window_type {
		uint32_t h_start;
		uint32_t h_size;
		uint32_t v_start;
		uint32_t v_size;
	} source_win;
};

struct afbd_wb_info {
	int dmafd;
	unsigned int ch_id;
	unsigned int width;
	unsigned int height;
};


/*decoder display ioctl command*/

#define DEC_IOC_MAGIC 'd'

#define DEC_FRMAE_SUBMIT    _IOW(DEC_IOC_MAGIC, 0x0, struct dec_frame_config)
#define DEC_ENABLE          _IOW(DEC_IOC_MAGIC, 0x1, unsigned int)
/* 0x2 ~ 0x6 reserved */
#define DEC_INTERLACE_SETUP _IOW(DEC_IOC_MAGIC, 0x7, struct dec_frame_config)
#define DEC_STREAM_STOP     _IOW(DEC_IOC_MAGIC, 0x8, unsigned int)
#define DEC_BYPASS_EN       _IOW(DEC_IOC_MAGIC, 0x9, unsigned int)
#define DEC_GET_VSYNC_TIMESTAMP _IOR(DEC_IOC_MAGIC, 0xA, struct dec_vsync_timestamp)
#define DEC_MAP_VIDEO_BUFFER _IOWR(DEC_IOC_MAGIC, 0xB, struct dec_video_buffer_data)
#define DEC_SET_OVERSCAN     _IOW(DEC_IOC_MAGIC, 0xC, struct overscan_info)
#define DEC_UNMAP_VIDEO_BUFFER _IOW(DEC_IOC_MAGIC, 0xD, struct dec_video_buffer_data)

#define DEC_ALLOC_VIDEO_BUFFER _IOWR(DEC_IOC_MAGIC, 0xE, struct dec_video_buffer_data)
#define DEC_FREE_VIDEO_BUFFER _IOW(DEC_IOC_MAGIC, 0xF, struct dec_video_buffer_data)

#define DEC_QUERY_VBI_BUFFER _IOWR(DEC_IOC_MAGIC, 0x10, struct dec_video_buffer_data)

#define DEC_START_WRITEBACK     _IOW(DEC_IOC_MAGIC, 0x11, struct afbd_wb_info)
#define DEC_WAIT_WB_FINISH _IOW(DEC_IOC_MAGIC, 0x12, struct afbd_wb_info)

#ifdef __cplusplus
}
#endif

#endif /*End of file*/

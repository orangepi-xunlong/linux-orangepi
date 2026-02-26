/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2007-2021 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _VIDEO_DECODER_DATA_TYPE_H_
#define _VIDEO_DECODER_DATA_TYPE_H_

#include <linux/types.h>

#ifndef u16_t
typedef unsigned short	u16_t;
#endif
#ifndef s32_t
typedef int32_t			s32_t;
#endif
#ifndef u32_t
typedef uint32_t		u32_t;
#endif

#define MAX_WINDOW_SIZE_H (1920 * 16)
#define MAX_WINDOW_SIZE_V (1080 * 16)

typedef struct _THalWin {
	s32_t h_start;
	u32_t h_size;
	s32_t v_start;
	u32_t v_size;
} THalWin;

typedef enum {
	kHalDisplayAspectRatio_Auto,		  // AFD, PAR, AR, WSS
	kHalDisplayAspectRatio_KeepSourceAr,  // display aspect ratio remains the same as source
	kHalDisplayAspectRatio_Full,
	kHalDisplayAspectRatio_16_9,
	kHalDisplayAspectRatio_4_3,
	kHalDisplayAspectRatio_Zoom,
	kHalDisplayAspectRatio_Max,
} THalDisplayAspectRatio;


typedef enum _VideoDecoderColorFormat {
	kVideoDecoderColorFormat_Yuv420_888,
	kVideoDecoderColorFormat_Yuv420_1088,
	kVideoDecoderColorFormat_Yuv420_101010,
	kVideoDecoderColorFormat_Yuv420_121212,
	kVideoDecoderColorFormat_Yuv422_888,
	kVideoDecoderColorFormat_Yuv422_1088,
	kVideoDecoderColorFormat_Yuv422_101010,
	kVideoDecoderColorFormat_Yuv422_121212,
	kVideoDecoderColorFormat_Yuv444_888,
	kVideoDecoderColorFormat_Yuv444_101010,
	kVideoDecoderColorFormat_Yuv444_121212,
	kVideoDecoderColorFormat_Rgb_888,
	kVideoDecoderColorFormat_Rgb_101010,
	kVideoDecoderColorFormat_Rgb_121212,
	kVideoDecoderColorFormat_Rgb_565,
	kVideoDecoderColorFormat_Yuv420_101010_AFBD_P010_High,
	kVideoDecoderColorFormat_Yuv420_101010_AFBD_P010_Low,
	kVideoDecoderColorFormat_Max,
} VideoDecoderColorFormat;

typedef enum _VideoDecoderColorSpace {
	kVideoDecoderColorSpace_Bt601,
	kVideoDecoderColorSpace_Bt709,
	kVideoDecoderColorSpace_Bt2020_Nclycc,
	kVideoDecoderColorSpace_Bt2020_Clycc,
	kVideoDecoderColorSpace_Xvycc,
	kVideoDecoderColorSpace_Rgb,
	kVideoDecoderColorSpace_Max,
} VideoDecoderColorSpace;

typedef enum _VideoDecoderHdrScheme{
	kVideoDecoderHdrScheme_Sdr,
	kVideoDecoderHdrScheme_Hdr10,
	kVideoDecoderHdrScheme_Hdr10Plus,
	kVideoDecoderHdrScheme_Sl_Hdr1,
	kVideoDecoderHdrScheme_Sl_Hdr2,
	kVideoDecoderHdrScheme_Hlg,
	kVideoDecoderHdrScheme_DolbyVison,
	kVideoDecoderHdrScheme_Max,
} VideoDecoderHdrScheme;

typedef struct _VideoDecoderHdrStaticMetadata {
	//ex: color gamut: BT.2020
	u16_t display_primaries_x[3];					// { 7080 , 1700, 1310 } *5, (divided by 5 on display side)
	u16_t display_primaries_y[3];					// { 2920 , 7970 , 460 } *5, (divided by 5 on display side)
	//ex: D65
	u16_t white_point_x;							// 3127 *5, (divided by 5 on display side)
	u16_t white_point_y;							// 3290 *5, (divided by 5 on display side)
	u32_t max_display_mastering_luma;				// 1000.00(typical value)*10000, (divided by 10000 on display side)
	u32_t min_display_mastering_luma;				// 0 or 0.0001((typical value))*10000, (divided by 10000 on display side)
	u16_t max_content_light_level;					// ex: 1000
	u16_t max_frame_average_light_level;			// ex: 100
} VideoDecoderHdrStaticMetadata;

typedef struct _VideoDecoderHdr10PlusMetadata {
	s32_t targeted_system_display_maximum_luminance;  // ex: 400, (-1: when dynamic metadata is not available)
	u16_t application_version;						// 0 or 1
	u16_t num_distributions;						// ex: 9
	u32_t maxscl[3];								// 0x00000-0x186A0, (divided by 10 on display side)
	u32_t average_maxrgb;							// 0x00000-0x186A0, (divided by 10 on display side)
	u32_t distribution_values[10];					// 0x00000-0x186A0(i=0,1,3-9), 0-100(i=2), (divided by 10 for i=0,1,3-9 on display side)
	u16_t knee_point_x;								// 0-4095, (divided by 4095 on display side)
	u16_t knee_point_y;								// 0-4095, (divided by 4095 on display side)
	u16_t num_bezier_curve_anchors;					// 0-9
	u16_t bezier_curve_anchors[9];					// 0-1023, (divided by 1023 on display side)
} VideoDecoderHdr10PlusMetadata;

typedef struct _VideoDisplayProperties{
	THalWin source;
	THalWin destination;
	THalDisplayAspectRatio aspect_ratio;
} VideoDisplayProperties;

typedef struct _VideoDecoderSignalInfo {
	u32_t interface_version_0;						// interface version = 'a','w',0,0
	u32_t interface_version_1;						// interface version = 2

	u32_t pic_hsize_before_scaler;
	u32_t pic_vsize_before_scaler;
	u32_t pic_hsize_after_scaler;
	u32_t pic_vsize_after_scaler;

	u32_t display_top_offset;
	u32_t display_bottom_offset;
	u32_t display_left_offset;
	u32_t display_right_offset;

	u32_t par_width;							  // Pixel Aspect Ratio. from SAR of H26x
	u32_t par_height;							  // for H26x
	u32_t active_format_description;			  // for DVB/ATSC

	u32_t frame_rate_before_frame_rate_change;	  // frame rate x 1000, for example: 23970 meaning frame rate 23.97Hz
	u32_t frame_rate_after_frame_rate_change;	  // frame rate x 1000, for example: 23970 meaning frame rate 23.97Hz
	u32_t b_interlace;							  // 1: interlace signal; 0: progressive signal.

	u32_t color_format;							  // use type define : VideoDecoderColorFormat
	u32_t color_space;							  // use type define : VideoDecoderColorSpace
	u32_t b_full_range;							  // 1: full range.

	u32_t color_primaries;						  // 0xffff0000 for undefined
	u32_t transfer_characteristics;				  // 0xffff0000 for undefined
	u32_t matrix_coefficients;					  // 0xffff0000 for undefined

	u32_t buffer_stride;						  // in byte.
	u32_t b_compress_en;						  // 1: enable compression. 0: disable compression.

	u32_t hdr_scheme;							// use type define : VideoDecoderHdrScheme
	u32_t hdr_static_metadata_offset;			// for Hdr10, Hdr10+, SL-HDR1, SL-HDR2, DolbyVison.
	u32_t hdr_dynamic_metadata_offset;			// for Hdr10+, SL-HDR1, SL-HDR2, DolbyVison.

	VideoDisplayProperties display_properties;
} VideoDecoderSignalInfo;

#endif

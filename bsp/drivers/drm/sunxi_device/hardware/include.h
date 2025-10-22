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

#ifndef _DISP_INCLUDE_H_
#define _DISP_INCLUDE_H_

#include <linux/types.h>

enum disp_color_space {
	DISP_UNDEF = 0x00,
	DISP_UNDEF_F = 0x01,
	DISP_GBR = 0x100,
	DISP_BT709 = 0x101,
	DISP_FCC = 0x102,
	DISP_BT470BG = 0x103,
	DISP_BT601 = 0x104,
	DISP_SMPTE240M = 0x105,
	DISP_YCGCO = 0x106,
	DISP_BT2020NC = 0x107,
	DISP_BT2020C = 0x108,
	DISP_GBR_F = 0x200,
	DISP_BT709_F = 0x201,
	DISP_FCC_F = 0x202,
	DISP_BT470BG_F = 0x203,
	DISP_BT601_F = 0x204,
	DISP_SMPTE240M_F = 0x205,
	DISP_YCGCO_F = 0x206,
	DISP_BT2020NC_F = 0x207,
	DISP_BT2020C_F = 0x208,
	DISP_RESERVED = 0x300,
	DISP_RESERVED_F = 0x301,
};

enum disp_csc_type {
	DISP_CSC_TYPE_RGB        = 0,
	DISP_CSC_TYPE_YUV444     = 1,
	DISP_CSC_TYPE_YUV422     = 2,
	DISP_CSC_TYPE_YUV420     = 3,
};

enum disp_data_bits {
	DISP_DATA_8BITS    = 0,
	DISP_DATA_10BITS   = 1,
	DISP_DATA_12BITS   = 2,
	DISP_DATA_16BITS   = 3,
};
enum disp_dvi_hdmi {
	DISP_DVI_HDMI_UNDEFINED = 0,
	DISP_DVI = 1,
	DISP_HDMI = 2,
};
enum disp_scan_info {
	DISP_SCANINFO_NO_DATA = 0,
	OVERSCAN = 1,
	UNDERSCAN = 2,
};
enum disp_color_range {
	DISP_COLOR_RANGE_DEFAULT = 0, /* default */
	DISP_COLOR_RANGE_0_255 = 1,
	DISP_COLOR_RANGE_16_235 = 2,
};

enum disp_tv_mode {
	DISP_TV_MOD_480I = 0,
	DISP_TV_MOD_576I = 1,
	DISP_TV_MOD_480P = 2,
	DISP_TV_MOD_576P = 3,
	DISP_TV_MOD_720P_50HZ = 4,
	DISP_TV_MOD_720P_60HZ = 5,
	DISP_TV_MOD_1080I_50HZ = 6,
	DISP_TV_MOD_1080I_60HZ = 7,
	DISP_TV_MOD_1080P_24HZ = 8,
	DISP_TV_MOD_1080P_50HZ = 9,
	DISP_TV_MOD_1080P_60HZ = 0xa,
	DISP_TV_MOD_1080P_24HZ_3D_FP = 0x17,
	DISP_TV_MOD_720P_50HZ_3D_FP = 0x18,
	DISP_TV_MOD_720P_60HZ_3D_FP = 0x19,
	DISP_TV_MOD_1080P_25HZ = 0x1a,
	DISP_TV_MOD_1080P_30HZ = 0x1b,
	DISP_TV_MOD_PAL = 0xb,
	DISP_TV_MOD_PAL_SVIDEO = 0xc,
	DISP_TV_MOD_NTSC = 0xe,
	DISP_TV_MOD_NTSC_SVIDEO = 0xf,
	DISP_TV_MOD_PAL_M = 0x11,
	DISP_TV_MOD_PAL_M_SVIDEO = 0x12,
	DISP_TV_MOD_PAL_NC = 0x14,
	DISP_TV_MOD_PAL_NC_SVIDEO = 0x15,
	DISP_TV_MOD_3840_2160P_30HZ = 0x1c,
	DISP_TV_MOD_3840_2160P_25HZ = 0x1d,
	DISP_TV_MOD_3840_2160P_24HZ = 0x1e,
	DISP_TV_MOD_4096_2160P_24HZ     = 0x1f,
	DISP_TV_MOD_4096_2160P_25HZ     = 0x20,
	DISP_TV_MOD_4096_2160P_30HZ     = 0x21,
	DISP_TV_MOD_3840_2160P_60HZ     = 0x22,
	DISP_TV_MOD_4096_2160P_60HZ     = 0x23,
	DISP_TV_MOD_3840_2160P_50HZ     = 0x24,
	DISP_TV_MOD_4096_2160P_50HZ     = 0x25,
	DISP_TV_MOD_2560_1440P_60HZ     = 0x26,
	DISP_TV_MOD_1440_2560P_70HZ     = 0x27,
	DISP_TV_MOD_1080_1920P_60HZ	= 0x28,
	DISP_TV_MOD_1280_1024P_60HZ     = 0x41,
	DISP_TV_MOD_1024_768P_60HZ      = 0x42,
	DISP_TV_MOD_900_540P_60HZ       = 0x43,
	DISP_TV_MOD_1920_720P_60HZ      = 0x44,
	DISP_TV_MOD_2560_1600P_60HZ     = 0x45,
	/* 2.5K reduce blanking */
	DISP_TV_MOD_2560_1600P_60HZ_RB  = 0x46,
	/* vga */
	DISP_VGA_MOD_640_480P_60 = 0x50,
	DISP_VGA_MOD_800_600P_60 = 0x51,
	DISP_VGA_MOD_1024_768P_60 = 0x52,
	DISP_VGA_MOD_1280_768P_60 = 0x53,
	DISP_VGA_MOD_1280_800P_60 = 0x54,
	DISP_VGA_MOD_1366_768P_60 = 0x55,
	DISP_VGA_MOD_1440_900P_60 = 0x56,
	DISP_VGA_MOD_1920_1080P_60 = 0x57,
	DISP_VGA_MOD_1920_1200P_60 = 0x58,
	DISP_TV_MOD_3840_1080P_30 = 0x59,
	DISP_VGA_MOD_1280_720P_60        = 0x5a,
	DISP_VGA_MOD_1600_900P_60        = 0x5b,
	DISP_VGA_MOD_MAX_NUM             = 0x5c,
	DISP_TV_MODE_NUM,
};

enum disp_eotf {
	DISP_EOTF_RESERVED = 0x000,
	DISP_EOTF_BT709 = 0x001,
	DISP_EOTF_UNDEF = 0x002,
	DISP_EOTF_GAMMA22 = 0x004, /* SDR */
	DISP_EOTF_GAMMA28 = 0x005,
	DISP_EOTF_BT601 = 0x006,
	DISP_EOTF_SMPTE240M = 0x007,
	DISP_EOTF_LINEAR = 0x008,
	DISP_EOTF_LOG100 = 0x009,
	DISP_EOTF_LOG100S10 = 0x00a,
	DISP_EOTF_IEC61966_2_4 = 0x00b,
	DISP_EOTF_BT1361 = 0x00c,
	DISP_EOTF_IEC61966_2_1 = 0X00d,
	DISP_EOTF_BT2020_0 = 0x00e,
	DISP_EOTF_BT2020_1 = 0x00f,
	DISP_EOTF_SMPTE2084 = 0x010, /* HDR10 */
	DISP_EOTF_SMPTE428_1 = 0x011,
	DISP_EOTF_ARIB_STD_B67 = 0x012, /* HLG */
};

/* disp_device_config - display deivce config
 *
 * @type: output type
 * @mode: output mode
 * @format: data format
 * @bits:   data bits
 * @eotf:   electro-optical transfer function
 *	    SDR  : DISP_EOTF_GAMMA22
 *	    HDR10: DISP_EOTF_SMPTE2084
 *	    HLG  : DISP_EOTF_ARIB_STD_B67
 * @cs:     color space type
 *	    DISP_BT601: SDR for SD resolution(< 720P)
 *	    DISP_BT709: SDR for HD resolution(>= 720P)
 *	    DISP_BT2020NC: HDR10 or HLG or wide-color-gamut
 * @dvi_hdmi: output mode
 *        DVI: DISP_DVI
 *        HDMI: DISP_HDMI
 * @range:    RGB/YUV quantization range
 *          DEFUALT: limited range when sending a CE video format
 *                   full range when sending an IT video format
 *          LIMITED: color limited range from 16 to 235
 *          FULL: color full range from 0 to 255
 * @scan info:
 *        DISP_SCANINFO_NO_DATA: overscan if it is a CE format,
 *                               underscan if it is an IT format
 *        OVERSCAN: composed for overscan display
 *        UNDERSCAN: composed for underscan display
 * @aspect_ratio: active format aspect ratio
 */
struct disp_device_config {
	enum disp_tv_mode			mode;
	enum disp_csc_type			format;
	enum disp_data_bits			bits;
	enum disp_eotf				eotf;
	enum disp_color_space		cs;
	enum disp_dvi_hdmi	        dvi_hdmi;
	enum disp_color_range		range;
	enum disp_scan_info			scan;
	unsigned int				aspect_ratio;
	unsigned int				reserve1;
};

struct disp_video_timings {
	unsigned int vic;	/* video information code */
	unsigned int tv_mode;
	unsigned int pixel_clk;
	unsigned int pixel_repeat; /* pixel repeat (pixel_repeat+1) times */
	unsigned int x_res;
	unsigned int y_res;
	unsigned int hor_total_time;
	unsigned int hor_back_porch;
	unsigned int hor_front_porch;
	unsigned int hor_sync_time;
	unsigned int ver_total_time;
	unsigned int ver_back_porch;
	unsigned int ver_front_porch;
	unsigned int ver_sync_time;
	unsigned int hor_sync_polarity;	/* 0: negative, 1: positive */
	unsigned int ver_sync_polarity;	/* 0: negative, 1: positive */
	bool b_interlace;
	unsigned int vactive_space;
	unsigned int trd_mode;
	unsigned long      dclk_rate_set; /* unit: hz */
	unsigned long long frame_period; /* unit: ns */
	int                start_delay; /* unit: line */
};

enum __lcd_irq_id_t {
	LCD_IRQ_TCON0_VBLK = 15,
	LCD_IRQ_TCON1_VBLK = 14,
	LCD_IRQ_TCON0_LINE = 13,
	LCD_IRQ_TCON1_LINE = 12,
	LCD_IRQ_TCON0_TRIF = 11,
	LCD_IRQ_TCON0_CNTR = 10,
	LCD_IRQ_FSYNC_INT = 9,
	LCD_IRQ_DATA_EN_INT = 8,
};

enum __lcd_src_t {
	LCD_SRC_DE = 0,
	LCD_SRC_COLOR_BAR = 1,
	LCD_SRC_GRAYSCALE = 2,
	LCD_SRC_BLACK_BY_WHITE = 3,
	LCD_SRC_BLACK = 4,
	LCD_SRC_WHITE = 5,
	LCD_SRC_GRID = 7,
	LCD_SRC_BLUE = 8
};

#endif

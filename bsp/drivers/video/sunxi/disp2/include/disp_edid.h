/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2022 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DISP_EDID_H__
#define __DISP_EDID_H__

#include <linux/types.h>

/* sunxi disp edid*/
#define EDID_LENGTH 128
#define EDID_ADDR 0x50
#define DDC_ADDR 0x50
#define DDC_ADDR2 0x52 /* E-DDC 1.2 - where DisplayID can hide */

#define CEA_EXT	    0x02
#define VTB_EXT	    0x10
#define DI_EXT	    0x40
#define LS_EXT	    0x50
#define MI_EXT	    0x60
#define DISPLAYID_EXT 0x70

#define DATA_BLOCK_CTA 0x81

struct est_timings {
	u8 t1;
	u8 t2;
	u8 mfg_rsvd;
} __attribute__((packed));

/* 00=16:10, 01=4:3, 10=5:4, 11=16:9 */
#define EDID_TIMING_ASPECT_SHIFT 6
#define EDID_TIMING_ASPECT_MASK  (0x3 << EDID_TIMING_ASPECT_SHIFT)

/* need to add 60 */
#define EDID_TIMING_VFREQ_SHIFT  0
#define EDID_TIMING_VFREQ_MASK   (0x3f << EDID_TIMING_VFREQ_SHIFT)

struct std_timing {
	u8 hsize; /* need to multiply by 8 then add 248 */
	u8 vfreq_aspect;
} __attribute__((packed));


/* If detailed data is pixel timing */
struct detailed_pixel_timing {
	u8 hactive_lo;
	u8 hblank_lo;
	u8 hactive_hblank_hi;
	u8 vactive_lo;
	u8 vblank_lo;
	u8 vactive_vblank_hi;
	u8 hsync_offset_lo;
	u8 hsync_pulse_width_lo;
	u8 vsync_offset_pulse_width_lo;
	u8 hsync_vsync_offset_pulse_width_hi;
	u8 width_mm_lo;
	u8 height_mm_lo;
	u8 width_height_mm_hi;
	u8 hborder;
	u8 vborder;
	u8 misc;
} __attribute__((packed));

/* If it's not pixel timing, it'll be one of the below */
struct detailed_data_string {
	u8 str[13];
} __attribute__((packed));

struct detailed_data_monitor_range {
	u8 min_vfreq;
	u8 max_vfreq;
	u8 min_hfreq_khz;
	u8 max_hfreq_khz;
	u8 pixel_clock_mhz; /* need to multiply by 10 */
	u8 flags;
	union {
		struct {
			u8 reserved;
			u8 hfreq_start_khz; /* need to multiply by 2 */
			u8 c; /* need to divide by 2 */
			__le16 m;
			u8 k;
			u8 j; /* need to divide by 2 */
		} __attribute__((packed)) gtf2;
		struct {
			u8 version;
			u8 data1; /* high 6 bits: extra clock resolution */
			u8 data2; /* plus low 2 of above: max hactive */
			u8 supported_aspects;
			u8 flags; /* preferred aspect and blanking support */
			u8 supported_scalings;
			u8 preferred_refresh;
		} __attribute__((packed)) cvt;
	} formula;
} __attribute__((packed));

struct detailed_data_wpindex {
	u8 white_yx_lo; /* Lower 2 bits each */
	u8 white_x_hi;
	u8 white_y_hi;
	u8 gamma; /* need to divide by 100 then add 1 */
} __attribute__((packed));

struct detailed_data_color_point {
	u8 windex1;
	u8 wpindex1[3];
	u8 windex2;
	u8 wpindex2[3];
} __attribute__((packed));

struct cvt_timing {
	u8 code[3];
} __attribute__((packed));

struct detailed_non_pixel {
	u8 pad1;
	u8 type; /* ff=serial, fe=string, fd=monitor range, fc=monitor name
		    fb=color point data, fa=standard timing data,
		    f9=undefined, f8=mfg. reserved */
	u8 pad2;
	union {
		struct detailed_data_string str;
		struct detailed_data_monitor_range range;
		struct detailed_data_wpindex color;
		struct std_timing timings[6];
		struct cvt_timing cvt[4];
	} data;
} __attribute__((packed));


struct detailed_timing {
	__le16 pixel_clock; /* need to multiply by 10 KHz */
	union {
		struct detailed_pixel_timing pixel_data;
		struct detailed_non_pixel other_data;
	} data;
} __attribute__((packed));

#define SUNXI_EDID_INPUT_SHIFT           (7)
#define SUNXI_EDID_INPUT_DIGITAL         (1 << 7)
#define SUNXI_EDID_DIGITAL_DEPTH_MASK    (7 << 4) /* 1.4 */
#define SUNXI_EDID_DIGITAL_DEPTH_UNDEF   (0 << 4) /* 1.4 */
#define SUNXI_EDID_DIGITAL_DEPTH_6       (1 << 4) /* 1.4 */
#define SUNXI_EDID_DIGITAL_DEPTH_8       (2 << 4) /* 1.4 */
#define SUNXI_EDID_DIGITAL_DEPTH_10      (3 << 4) /* 1.4 */
#define SUNXI_EDID_DIGITAL_DEPTH_12      (4 << 4) /* 1.4 */
#define SUNXI_EDID_DIGITAL_DEPTH_14      (5 << 4) /* 1.4 */
#define SUNXI_EDID_DIGITAL_DEPTH_16      (6 << 4) /* 1.4 */
#define SUNXI_EDID_DIGITAL_DEPTH_RSVD    (7 << 4) /* 1.4 */
#define SUNXI_EDID_DIGITAL_TYPE_MASK     (7 << 0) /* 1.4 */
#define SUNXI_EDID_DIGITAL_TYPE_UNDEF    (0 << 0) /* 1.4 */
#define SUNXI_EDID_DIGITAL_TYPE_DVI      (1 << 0) /* 1.4 */
#define SUNXI_EDID_DIGITAL_TYPE_HDMI_A   (2 << 0) /* 1.4 */
#define SUNXI_EDID_DIGITAL_TYPE_HDMI_B   (3 << 0) /* 1.4 */
#define SUNXI_EDID_DIGITAL_TYPE_MDDI     (4 << 0) /* 1.4 */
#define SUNXI_EDID_DIGITAL_TYPE_DP       (5 << 0) /* 1.4 */
#define SUNXI_EDID_DIGITAL_DFP_1_X       (1 << 0) /* 1.3 */

#define SUNXI_EDID_COLOR_FMT_MASK        (3 << 3)
#define SUNXI_EDID_COLOR_FMT_YCC444      (1 << 3)
#define SUNXI_EDID_COLOR_FMT_YCC422      (2 << 3)
#define SUNXI_EDID_COLOR_FMT_YCC444_422  (3 << 3)

#define SUNXI_CEA_BASIC_AUDIO_MASK       (1 << 6) /* Version3 */
#define SUNXI_CEA_YCC444_MASK	         (1 << 5) /* Version3 */
#define SUNXI_CEA_YCC422_MASK	         (1 << 4) /* Version3 */

#define SUNXI_EDID_MISC_VPOLAR_MASK      (7 << 2)
#define SUNXI_EDID_MISC_HPOLAR_MASK      ((1 << 1) | (1 << 4))
#define SUNXI_EDID_MISC_INTERLACE_MASK   (1 << 7)
#define SUNXI_EDID_MISC_VPOLAR_NEG	     0b110
#define SUNXI_EDID_MISC_VPOLAR_POS	     0b111
#define SUNXI_EDID_MISC_HPOLAR_NEG	     0b1000
#define SUNXI_EDID_MISC_HPOLAR_POS	     0b1001

struct edid {
	u8 header[8];
	/* Vendor & product info */
	u8 mfg_id[2];
	u8 prod_code[2];
	u32 serial; /* FIXME: byte order */
	u8 mfg_week;
	u8 mfg_year;
	/* EDID version */
	u8 version;
	u8 revision;
	/* Display info: */
	u8 input;
	u8 width_cm;
	u8 height_cm;
	u8 gamma;
	u8 features;
	/* Color characteristics */
	u8 red_green_lo;
	u8 black_white_lo;
	u8 red_x;
	u8 red_y;
	u8 green_x;
	u8 green_y;
	u8 blue_x;
	u8 blue_y;
	u8 white_x;
	u8 white_y;
	/* Est. timings and mfg rsvd timings*/
	struct est_timings established_timings;
	/* Standard timings 1-8*/
	struct std_timing standard_timings[8];
	/* Detailing timings 1-4 */
	struct detailed_timing detailed_timings[4];
	/* Number of 128 byte ext. blocks */
	u8 extensions;
	/* Checksum */
	u8 checksum;
} __attribute__((packed));

#define SUNXI_EDID_PRODUCT_ID(e) ((e)->prod_code[0] | ((e)->prod_code[1] << 8))

/*sunxi disp edid end*/

#endif

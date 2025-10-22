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
#include <linux/mm.h>

#define NUM_DIV 8
#define MAX_LUT_SIZE 729

enum {
	/* hdr static metadata is available */
	SUNXI_METADATA_FLAG_HDR_SATIC_METADATA   = 0x00000001,
	/* hdr dynamic metadata is available */
	SUNXI_METADATA_FLAG_HDR_DYNAMIC_METADATA = 0x00000002,
	/* hdr10+ metadata */
	SUNXI_METADATA_FLAG_HDR10P_METADATA      = 0x00000004,
	/* hdrvivid metadata */
	SUNXI_METADATA_FLAG_HDRVIVID_HEADER      = 0x00000008,
	/* afbc header data is available */
	SUNXI_METADATA_FLAG_AFBC_HEADER          = 0x00000010,
	/* gralloc set this when allocate buffer */
	SUNXI_METADATA_FLAG_UNINITIALIZED        = 0xffffffff,
};

struct afbc_header {
	u32 signature;
	u16 filehdr_size;
	u16 version;
	u32 body_size;
	u8 ncomponents;
	u8 header_layout;
	u8 yuv_transform;
	u8 block_split;
	u8 inputbits[4];
	u16 block_width;
	u16 block_height;
	u16 width;
	u16 height;
	u8  left_crop;
	u8  top_crop;
	u16 block_layout;
};

struct display_master_data {
	/* display primaries */
	u16 display_primaries_x[3];
	u16 display_primaries_y[3];

	/* white_point */
	u16 white_point_x;
	u16 white_point_y;

	/* max/min display mastering luminance */
	u32 max_display_mastering_luminance;
	u32 min_display_mastering_luminance;
};

/* static metadata type 1 */
#if IS_ENABLED(CONFIG_AW_DRM)
struct hdr_static_metadata_fake {
#else
struct hdr_static_metadata {
#endif
	struct display_master_data disp_master;

	u16 maximum_content_light_level;
	u16 maximum_frame_average_light_level;
};

struct hdr_10Plus_metadata {
	int32_t  targeted_system_display_maximum_luminance;  //ex: 400, (-1: not available)
	uint16_t application_version; // 0 or 1
	uint16_t num_distributions; //ex: 9
	uint32_t maxscl[3]; //0x00000-0x186A0, (display side will divided by 10)
	uint32_t average_maxrgb; //0x00000-0x186A0, (display side will divided by 10)
	uint32_t distribution_values[10]; //0x00000-0x186A0(will div by 10)(i=0,1,3-9), 0-100(i=2)
	uint16_t knee_point_x; //0-4095, (display side will divided by 4095)
	uint16_t knee_point_y; //0-4095, (display side will divided by 4095)
	uint16_t num_bezier_curve_anchors; //0-9
	uint16_t bezier_curve_anchors[9]; //0-1023, (display side will divided by 1023)
};

/**
 * Rational number (pair of numerator and denominator).
 */
typedef struct AVRational{
    int num; ///< Numerator
    int den; ///< Denominator
} AVRational;

/**
 * Color tone mapping parameters at a processing window in a dynamic metadata for
 * CUVA 005.1:2021.
 */
typedef struct AVHDRVividColorToneMappingParams {
    /**
     * The nominal maximum display luminance of the targeted system display,
     * in multiples of 1.0/4095 candelas per square metre. The value shall be in
     * the range of 0.0 to 1.0, inclusive.
     */
    AVRational targeted_system_display_maximum_luminance;

    /**
     * This flag indicates that transfer the base paramter(for value of 1)
     */
    int base_enable_flag;

    /**
     * base_param_m_p in the base parameter,
     * in multiples of 1.0/16383. The value shall be in
     * the range of 0.0 to 1.0, inclusive.
     */
    AVRational base_param_m_p;

    /**
     * base_param_m_m in the base parameter,
     * in multiples of 1.0/10. The value shall be in
     * the range of 0.0 to 6.3, inclusive.
     */
    AVRational base_param_m_m;

    /**
     * base_param_m_a in the base parameter,
     * in multiples of 1.0/1023. The value shall be in
     * the range of 0.0 to 1.0 inclusive.
     */
    AVRational base_param_m_a;

    /**
     * base_param_m_b in the base parameter,
     * in multiples of 1/1023. The value shall be in
     * the range of 0.0 to 1.0, inclusive.
     */
    AVRational base_param_m_b;

    /**
     * base_param_m_n in the base parameter,
     * in multiples of 1.0/10. The value shall be in
     * the range of 0.0 to 6.3, inclusive.
     */
    AVRational base_param_m_n;

    /**
     * indicates k1_0 in the base parameter,
     * base_param_k1 <= 1: k1_0 = base_param_k1
     * base_param_k1 > 1: reserved
     */
    int base_param_k1;

    /**
     * indicates k2_0 in the base parameter,
     * base_param_k2 <= 1: k2_0 = base_param_k2
     * base_param_k2 > 1: reserved
     */
    int base_param_k2;

    /**
     * indicates k3_0 in the base parameter,
     * base_param_k3 == 1: k3_0 = base_param_k3
     * base_param_k3 == 2: k3_0 = maximum_maxrgb
     * base_param_k3 > 2: reserved
     */
    int base_param_k3;

    /**
     * This flag indicates that delta mode of base paramter(for value of 1)
     */
    int base_param_Delta_enable_mode;

    /**
     * base_param_Delta in the base parameter,
     * in multiples of 1.0/127. The value shall be in
     * the range of 0.0 to 1.0, inclusive.
     */
    AVRational base_param_Delta;

    /**
     * indicates 3Spline_enable_flag in the base parameter,
     * This flag indicates that transfer three Spline of base paramter(for value of 1)
     */
    int three_Spline_enable_flag;

    /**
     * The number of three Spline. The value shall be in the range
     * of 1 to 2, inclusive.
     */
    int three_Spline_num;

    /**
     * The mode of three Spline. the value shall be in the range
     * of 0 to 3, inclusive.
     */
    int three_Spline_TH_mode[2];

    /**
     * three_Spline_TH_enable_MB is in the range of 0.0 to 1.0, inclusive
     * and in multiples of 1.0/255.
     *
     */
    AVRational three_Spline_TH_enable_MB[2];

    /**
     * 3Spline_TH_enable of three Spline.
     * The value shall be in the range of 0.0 to 1.0, inclusive.
     * and in multiples of 1.0/4095.
     */
    AVRational three_Spline_TH_enable[2];

    /**
     * 3Spline_TH_Delta1 of three Spline.
     * The value shall be in the range of 0.0 to 0.25, inclusive,
     * and in multiples of 0.25/1023.
     */
    AVRational three_Spline_TH_Delta1[2];

    /**
     * 3Spline_TH_Delta2 of three Spline.
     * The value shall be in the range of 0.0 to 0.25, inclusive,
     * and in multiples of 0.25/1023.
     */
    AVRational three_Spline_TH_Delta2[2];

    /**
     * 3Spline_enable_Strength of three Spline.
     * The value shall be in the range of 0.0 to 1.0, inclusive,
     * and in multiples of 1.0/255.
     */
    AVRational three_Spline_enable_Strength[2];
} AVHDRVividColorToneMappingParams;

/**
 * Color transform parameters at a processing window in a dynamic metadata for
 * CUVA 005.1:2021.
 */
typedef struct AVHDRVividColorTransformParams {
    /**
     * Indicates the minimum brightness of the displayed content.
     * The values should be in the range of 0.0 to 1.0,
     * inclusive and in multiples of 1/4095.
     */
    AVRational minimum_maxrgb;

    /**
     * Indicates the average brightness of the displayed content.
     * The values should be in the range of 0.0 to 1.0,
     * inclusive and in multiples of 1/4095.
     */
    AVRational average_maxrgb;

    /**
     * Indicates the variance brightness of the displayed content.
     * The values should be in the range of 0.0 to 1.0,
     * inclusive and in multiples of 1/4095.
     */
    AVRational variance_maxrgb;

    /**
     * Indicates the maximum brightness of the displayed content.
     * The values should be in the range of 0.0 to 1.0, inclusive
     * and in multiples of 1/4095.
     */
    AVRational maximum_maxrgb;

    /**
     * This flag indicates that the metadata for the tone mapping function in
     * the processing window is present (for value of 1).
     */
    int tone_mapping_mode_flag;

    /**
     * The number of tone mapping param. The value shall be in the range
     * of 1 to 2, inclusive.
     */
    int tone_mapping_param_num;

    /**
     * The color tone mapping parameters.
     */
    AVHDRVividColorToneMappingParams tm_params[2];

    /**
     * This flag indicates that the metadata for the color saturation mapping in
     * the processing window is present (for value of 1).
     */
    int color_saturation_mapping_flag;

    /**
     * The number of color saturation param. The value shall be in the range
     * of 0 to 7, inclusive.
     */
    int color_saturation_num;

    /**
     * Indicates the color correction strength parameter.
     * The values should be in the range of 0.0 to 2.0, inclusive
     * and in multiples of 1/128.
     */
    AVRational color_saturation_gain[8];
} AVHDRVividColorTransformParams;

typedef struct AVDynamicHDRVivid {
    int bHdrVividFlag;
    /**
     * The system start code. The value shall be set to 0x01.
     */
    uint8_t system_start_code;

    /**
     * The number of processing windows. The value shall be set to 0x01
     * if the system_start_code is 0x01.
     */
    uint8_t num_windows;

    /**
     * The color transform parameters for every processing window.
     */
    AVHDRVividColorTransformParams params[3];

} AVDynamicHDRVivid;

/* hdrvivid dynamic metadata type */
struct hdrvivid_dynamic_metadata {
    AVDynamicHDRVivid AvDynamicHdrVivid;
};

/* sunxi video metadata for ve and de */
struct sunxi_metadata {
#if IS_ENABLED(CONFIG_AW_DRM)
    struct hdr_static_metadata_fake hdr_smetada;
#else
    struct hdr_static_metadata hdr_smetada;
#endif
    struct afbc_header afbc_head;
    uint32_t flag;

    struct hdr_10Plus_metadata hdr10_plus_smetada;
    struct hdrvivid_dynamic_metadata hdrvivid_meta;
    /*cdc lut for driver*/
    uint32_t divLut[NUM_DIV][MAX_LUT_SIZE];
};

#endif /* #ifndef __SUNXI_METADATA_H__ */

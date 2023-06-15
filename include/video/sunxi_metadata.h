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

#define NUM_DIV 8
#define MAX_LUT_SIZE 729

enum {
	/* hdr static metadata is available */
	SUNXI_METADATA_FLAG_HDR_SATIC_METADATA   = 0x00000001,
	/* hdr dynamic metadata is available */
	SUNXI_METADATA_FLAG_HDR_DYNAMIC_METADATA = 0x00000002,
    /* hdr10+ metadata */
	SUNXI_METADATA_FLAG_HDR10P_METADATA      = 0x00000004,
    /* dolby vision */
	SUNXI_METADATA_FLAG_DV_HEADER            = 0x00000008,
	/* afbc header data is available */
	SUNXI_METADATA_FLAG_AFBC_HEADER          = 0x00000010,
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
struct hdr_static_metadata {
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
	unsigned int divLut[NUM_DIV][MAX_LUT_SIZE];
	//uint32_t flag;
};

/* sunxi video metadata for ve and de */
struct sunxi_metadata {
	struct hdr_static_metadata hdr_smetada;
	struct hdr_10Plus_metadata hdr10_plus_smetada;
	struct afbc_header afbc_head;


	// flag must be at last always
	uint32_t flag;
};

#endif /* #ifndef __SUNXI_METADATA_H__ */

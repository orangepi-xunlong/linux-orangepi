/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * NVIDIA Tegra Video Input Device Driver Core Helpers
 *
 * Copyright (c) 2015-2022, NVIDIA CORPORATION.  All rights reserved.
 */

#ifndef __TEGRA_CORE_H__
#define __TEGRA_CORE_H__

#include <media/v4l2-subdev.h>

/* Minimum and maximum width and height common to Tegra video input device. */
#define TEGRA_MIN_WIDTH		32U
#define TEGRA_MAX_WIDTH		32768U
#define TEGRA_MIN_HEIGHT	32U
#define TEGRA_MAX_HEIGHT	32768U
/* Width alignment */
#define TEGRA_WIDTH_ALIGNMENT	1
/* Stride alignment */
#define TEGRA_STRIDE_ALIGNMENT	1
/* Height alignment */
#define TEGRA_HEIGHT_ALIGNMENT	1
/* Size alignment */
#define TEGRA_SIZE_ALIGNMENT	0

/* 1080p resolution as default resolution for test pattern generator */
#define TEGRA_DEF_WIDTH		1920
#define TEGRA_DEF_HEIGHT	1080

#define TEGRA_VF_DEF		MEDIA_BUS_FMT_SRGGB10_1X10
#define TEGRA_IMAGE_FORMAT_DEF	32

enum tegra_image_dt {
	TEGRA_IMAGE_DT_YUV420_8 = 24,
	TEGRA_IMAGE_DT_YUV420_10,

	TEGRA_IMAGE_DT_YUV420CSPS_8 = 28,
	TEGRA_IMAGE_DT_YUV420CSPS_10,
	TEGRA_IMAGE_DT_YUV422_8,
	TEGRA_IMAGE_DT_YUV422_10,
	TEGRA_IMAGE_DT_RGB444,
	TEGRA_IMAGE_DT_RGB555,
	TEGRA_IMAGE_DT_RGB565,
	TEGRA_IMAGE_DT_RGB666,
	TEGRA_IMAGE_DT_RGB888,

	TEGRA_IMAGE_DT_RAW6 = 40,
	TEGRA_IMAGE_DT_RAW7,
	TEGRA_IMAGE_DT_RAW8,
	TEGRA_IMAGE_DT_RAW10,
	TEGRA_IMAGE_DT_RAW12,
	TEGRA_IMAGE_DT_RAW14,
};

/* Supported CSI to VI Data Formats */
enum tegra_vf_code {
	TEGRA_VF_RAW6 = 0,
	TEGRA_VF_RAW7,
	TEGRA_VF_RAW8,
	TEGRA_VF_RAW10,
	TEGRA_VF_RAW12,
	TEGRA_VF_RAW14,
	TEGRA_VF_EMBEDDED8,
	TEGRA_VF_RGB565,
	TEGRA_VF_RGB555,
	TEGRA_VF_RGB888,
	TEGRA_VF_RGB444,
	TEGRA_VF_RGB666,
	TEGRA_VF_YUV422,
	TEGRA_VF_YUV420,
	TEGRA_VF_YUV420_CSPS,
};

/**
 * struct tegra_frac
 * @numerator: numerator of the fraction
 * @denominator: denominator of the fraction
 */
struct tegra_frac {
	unsigned int numerator;
	unsigned int denominator;
};

/**
 * struct tegra_video_format - Tegra video format description
 * @vf_code: video format code
 * @width: format width in bits per component
 * @code: media bus format code
 * @bpp: bytes per pixel fraction (when stored in memory)
 * @img_fmt: image format
 * @img_dt: image data type
 * @fourcc: V4L2 pixel format FCC identifier
 * @description: format description, suitable for userspace
 */
struct tegra_video_format {
	enum tegra_vf_code vf_code;
	unsigned int width;
	unsigned int code;
	struct tegra_frac bpp;
	u32 img_fmt;
	enum tegra_image_dt img_dt;
	u32 fourcc;
	__u8 description[32];
};

#define	TEGRA_VIDEO_FORMAT(VF_CODE, BPP, MBUS_CODE, FRAC_BPP_NUM,	\
	FRAC_BPP_DEN, FORMAT, DATA_TYPE, FOURCC, DESCRIPTION)		\
{									\
	TEGRA_VF_##VF_CODE,						\
	BPP,								\
	MEDIA_BUS_FMT_##MBUS_CODE,					\
	{FRAC_BPP_NUM, FRAC_BPP_DEN},					\
	TEGRA_IMAGE_FORMAT_##FORMAT,					\
	TEGRA_IMAGE_DT_##DATA_TYPE,					\
	V4L2_PIX_FMT_##FOURCC,						\
	DESCRIPTION,							\
}

u32 tegra_core_get_word_count(unsigned int frame_width,
		const struct tegra_video_format *fmt);
u32 tegra_core_bytes_per_line(unsigned int width, unsigned int align,
		const struct tegra_video_format *fmt);
const struct tegra_video_format *tegra_core_get_default_format(void);

#endif

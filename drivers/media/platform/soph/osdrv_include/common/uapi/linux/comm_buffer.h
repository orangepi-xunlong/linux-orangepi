/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/comm_buffer.h
 * Description:
 *   The count defination of buffer size
 */

#ifndef __BUFFER_H__
#define __BUFFER_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <linux/common.h>
#include <linux/comm_math.h>
#include <linux/comm_video.h>

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define ROUND_UP(n, s) ((((n) + (s) - 1) / (s)) * (s))

#define CV183X_WA_FMT(x) (((x) == PIXEL_FORMAT_YUV_PLANAR_420) || ((x) == PIXEL_FORMAT_YUV_PLANAR_422) || \
			  ((x) == PIXEL_FORMAT_YUV_PLANAR_444) || ((x) == PIXEL_FORMAT_RGB_888_PLANAR) || \
			  ((x) == PIXEL_FORMAT_BGR_888_PLANAR) || ((x) == PIXEL_FORMAT_HSV_888_PLANAR))

static inline void common_getpicbufferconfig(unsigned int width, unsigned int height,
	pixel_format_e pixel_format, data_bitwidth_e bit_width,
	compress_mode_e cmp_mode, unsigned int align_width, vb_cal_config_s *cal_config)
{
	unsigned char bit_len = 0;
	unsigned int vb_size = 0;
	unsigned int align_height = 0;
	unsigned int main_stride = 0;
	unsigned int c_stride = 0;
	unsigned int main_size = 0;
	unsigned int y_size = 0;
	unsigned int c_size = 0;

	/* align: 0 is automatic mode, alignment size following system. non-0 for specified alignment size */
	if (align_width == 0)
		align_width = DEFAULT_ALIGN;
	else if (align_width > MAX_ALIGN)
		align_width = MAX_ALIGN;

	switch (bit_width) {
	case DATA_BITWIDTH_8: {
		bit_len = 8;
		break;
	}
	case DATA_BITWIDTH_10: {
		bit_len = 10;
		break;
	}
	case DATA_BITWIDTH_12: {
		bit_len = 12;
		break;
	}
	case DATA_BITWIDTH_14: {
		bit_len = 14;
		break;
	}
	case DATA_BITWIDTH_16: {
		bit_len = 16;
		break;
	}
	default: {
		bit_len = 0;
		break;
	}
	}

	if ((pixel_format == PIXEL_FORMAT_YUV_PLANAR_420)
	 || (pixel_format == PIXEL_FORMAT_NV12)
	 || (pixel_format == PIXEL_FORMAT_NV21)) {
		align_height = ALIGN(height, 2);
	} else
		align_height = height;

	if (cmp_mode == COMPRESS_MODE_NONE) {
		main_stride = ALIGN((width * bit_len + 7) >> 3, align_width);
		y_size = main_stride * align_height;

		if (pixel_format == PIXEL_FORMAT_YUV_PLANAR_420) {
			c_stride = ALIGN(((width >> 1) * bit_len + 7) >> 3, align_width);
			c_size = (c_stride * align_height) >> 1;

			main_stride = c_stride * 2;
			y_size = main_stride * align_height;
			main_size = y_size + (c_size << 1);
			cal_config->plane_num = 3;
		} else if (pixel_format == PIXEL_FORMAT_YUV_PLANAR_422) {
			c_stride = ALIGN(((width >> 1) * bit_len + 7) >> 3, align_width);
			c_size = c_stride * align_height;

			main_size = y_size + (c_size << 1);
			cal_config->plane_num = 3;
		} else if (pixel_format == PIXEL_FORMAT_RGB_888_PLANAR ||
			   pixel_format == PIXEL_FORMAT_BGR_888_PLANAR ||
			   pixel_format == PIXEL_FORMAT_HSV_888_PLANAR ||
			   pixel_format == PIXEL_FORMAT_YUV_PLANAR_444) {
			c_stride = main_stride;
			c_size = y_size;

			main_size = y_size + (c_size << 1);
			cal_config->plane_num = 3;
		} else if (pixel_format == PIXEL_FORMAT_RGB_BAYER_12BPP) {
			main_size = y_size;
			cal_config->plane_num = 1;
		} else if (pixel_format == PIXEL_FORMAT_YUV_400) {
			main_size = y_size;
			cal_config->plane_num = 1;
		} else if (pixel_format == PIXEL_FORMAT_NV12 || pixel_format == PIXEL_FORMAT_NV21) {
			c_stride = ALIGN((width * bit_len + 7) >> 3, align_width);
			c_size = (c_stride * align_height) >> 1;

			main_size = y_size + c_size;
			cal_config->plane_num = 2;
		} else if (pixel_format == PIXEL_FORMAT_NV16 || pixel_format == PIXEL_FORMAT_NV61) {
			c_stride = ALIGN((width * bit_len + 7) >> 3, align_width);
			c_size = c_stride * align_height;

			main_size = y_size + c_size;
			cal_config->plane_num = 2;
		} else if (pixel_format == PIXEL_FORMAT_YUYV || pixel_format == PIXEL_FORMAT_YVYU ||
			   pixel_format == PIXEL_FORMAT_UYVY || pixel_format == PIXEL_FORMAT_VYUY) {
			main_stride = ALIGN(((width * bit_len + 7) >> 3) * 2, align_width);
			y_size = main_stride * align_height;
			main_size = y_size;
			cal_config->plane_num = 1;
		} else if (pixel_format == PIXEL_FORMAT_ARGB_1555 || pixel_format == PIXEL_FORMAT_ARGB_4444) {
			// packed format
			main_stride = ALIGN((width * 16 + 7) >> 3, align_width);
			y_size = main_stride * align_height;
			main_size = y_size;
			cal_config->plane_num = 1;
		} else if (pixel_format == PIXEL_FORMAT_ARGB_8888) {
			// packed format
			main_stride = ALIGN((width * 32 + 7) >> 3, align_width);
			y_size = main_stride * align_height;
			main_size = y_size;
			cal_config->plane_num = 1;
		} else if (pixel_format == PIXEL_FORMAT_FP32_C3_PLANAR) {
			main_stride = ALIGN(((width * bit_len + 7) >> 3) * 4, align_width);
			y_size = main_stride * align_height;
			c_stride = main_stride;
			c_size = y_size;
			main_size = y_size + (c_size << 1);
			cal_config->plane_num = 3;
		} else if (pixel_format == PIXEL_FORMAT_FP16_C3_PLANAR ||
				pixel_format == PIXEL_FORMAT_BF16_C3_PLANAR) {
			main_stride = ALIGN(((width * bit_len + 7) >> 3) * 2, align_width);
			y_size = main_stride * align_height;
			c_stride = main_stride;
			c_size = y_size;
			main_size = y_size + (c_size << 1);
			cal_config->plane_num = 3;
		} else if (pixel_format == PIXEL_FORMAT_INT8_C3_PLANAR ||
				pixel_format == PIXEL_FORMAT_UINT8_C3_PLANAR) {
			c_stride = main_stride;
			c_size = y_size;
			main_size = y_size + (c_size << 1);
			cal_config->plane_num = 3;
		} else {
			// packed format
			main_stride = ALIGN(((width * bit_len + 7) >> 3) * 3, align_width);
			y_size = main_stride * align_height;
			main_size = y_size;
			cal_config->plane_num = 1;
		}

		vb_size = main_size;
	} else {
		// todo: compression mode
		cal_config->plane_num = 0;
	}

	cal_config->vb_size = vb_size;
	cal_config->main_stride = main_stride;
	cal_config->c_stride = c_stride;
	cal_config->main_y_size = y_size;
	cal_config->main_c_size = c_size;
	cal_config->main_size = main_size;
	cal_config->addr_align = align_width;
}

static inline unsigned int common_getpicbuffersize(unsigned int width,
			unsigned int height, pixel_format_e pixel_format,
			data_bitwidth_e bit_width, compress_mode_e cmp_mode, unsigned int align_width)
{
	vb_cal_config_s stcalconfig;

	common_getpicbufferconfig(width, height, pixel_format, bit_width, cmp_mode, align_width, &stcalconfig);

	return stcalconfig.vb_size;
}

static inline unsigned int common_getvencframebuffersize(int codec_type,
	unsigned int width, unsigned int height)
{
	unsigned int size = 0;

	if (codec_type == PT_H264) {
		unsigned int width_align = ALIGN(width, DEFAULT_ALIGN);
		unsigned int height_align = ALIGN(width, DEFAULT_ALIGN);

		size = (width_align * height_align * 2 * 3) / 2;
	} else if (codec_type == PT_H265) {
		size = ((width * height * 3) / 2);
		size = ROUND_UP(size, 4096) * 2;
	} else
		size = 0;

	return size;
}
static inline unsigned int vi_getrawbuffersize(unsigned int width,
		unsigned int height, pixel_format_e pixel_format,
		compress_mode_e cmp_mode, unsigned int align_width, unsigned char is_tile)
{
	unsigned int bit_width;
	unsigned int size = 0;
	unsigned int stride = 0;

	/* align: 0 is automatic mode, alignment size following system. non-0 for specified alignment size */
	if (align_width == 0)
		align_width = 16;
	else if (align_width > MAX_ALIGN)
		align_width = MAX_ALIGN;
	else
		align_width = ALIGN(align_width, 16);

	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_BAYER_8BPP: {
		bit_width = 8;
		break;
	}

	case PIXEL_FORMAT_RGB_BAYER_10BPP: {
		bit_width = 10;
		break;
	}

	case PIXEL_FORMAT_RGB_BAYER_12BPP: {
		bit_width = 12;
		break;
	}

	case PIXEL_FORMAT_RGB_BAYER_14BPP: {
		bit_width = 14;
		break;
	}

	case PIXEL_FORMAT_RGB_BAYER_16BPP: {
		bit_width = 16;
		break;
	}

	default: {
		bit_width = 0;
		break;
	}
	}

	if (cmp_mode == COMPRESS_MODE_NONE) {
		stride = ALIGN(ALIGN(width * bit_width, 8) / 8, align_width);
		size = stride * height;
	} else if (cmp_mode == COMPRESS_MODE_TILE || cmp_mode == COMPRESS_MODE_FRAME) {
		width = (is_tile) ? ((width + 8) >> 1) : (width >> 1);
		width = 3 * ((width + 1) >> 1);

		stride = ALIGN(width, 16);
		size = stride * height;
	}

	return size;
}

static inline void vdec_getpicbufferconfig(payload_type_e type,
	unsigned int width, unsigned int height,
	pixel_format_e pixel_format, data_bitwidth_e bit_width,
	compress_mode_e cmp_mode, vb_cal_config_s *vb_cfg)
{
	unsigned int align = 0, align_height = 0, align_width = 0;

	if (type == PT_H264) {
		align_width = ALIGN(width, H264D_ALIGN_W);
		align_height = ALIGN(height, H264D_ALIGN_H);
		align = H264D_ALIGN_W;
	} else if (type == PT_H265) {
		align_width = ALIGN(width, H265D_ALIGN_W);
		align_height = ALIGN(height, H265D_ALIGN_H);
		align = H265D_ALIGN_W;
	} else if ((type == PT_JPEG) || (type == PT_MJPEG)) {
		align_width = ALIGN(width, JPEGD_ALIGN_W);
		align_height = ALIGN(height, JPEGD_ALIGN_H);
		align = JPEGD_ALIGN_W;
	} else {
		align_width = ALIGN(width, DEFAULT_ALIGN);
		align_height = ALIGN(height, DEFAULT_ALIGN);
		align = DEFAULT_ALIGN;
	}

	common_getpicbufferconfig(align_width, align_height, pixel_format,
		bit_width, cmp_mode, align, vb_cfg);
}

static inline unsigned int vdec_getpicbuffersize(payload_type_e entype,
	unsigned int width, unsigned int height,
	pixel_format_e pixel_format, data_bitwidth_e bit_width,
	compress_mode_e cmp_mode)
{
	vb_cal_config_s stvbcfg;

	memset(&stvbcfg, 0, sizeof(stvbcfg));
	vdec_getpicbufferconfig(entype, width, height, pixel_format, bit_width, cmp_mode, &stvbcfg);
	return stvbcfg.vb_size;
}

static inline void venc_getpicbufferconfig(unsigned int width, unsigned int height,
	pixel_format_e pixel_format, data_bitwidth_e bit_width, compress_mode_e cmp_mode,
	vb_cal_config_s *vb_cfg)
{
	unsigned int alignwidth = ALIGN(width, VENC_ALIGN_W);
	unsigned int align_height = ALIGN(height, VENC_ALIGN_H);
	unsigned int align = VENC_ALIGN_W;

	common_getpicbufferconfig(alignwidth, align_height, pixel_format,
		bit_width, cmp_mode, align, vb_cfg);
}

static inline unsigned int venc_getpicbuffersize(unsigned int width, unsigned int height,
	pixel_format_e pixel_format, data_bitwidth_e bit_width, compress_mode_e cmp_mode)
{
	vb_cal_config_s vb_cfg;

	memset(&vb_cfg, 0, sizeof(vb_cfg));
	venc_getpicbufferconfig(width, height, pixel_format, bit_width, cmp_mode, &vb_cfg);
	return vb_cfg.vb_size;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __BUFFER_H__ */

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse preview enhancer process module
 *
 * Copyright (C) 2015-2023 AllwinnerTech, Inc.
 *
 * Authors:  Huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "include.h"
#if IS_ENABLED(CONFIG_SUNXI_DI_V2X) || IS_ENABLED(CONFIG_SUNXI_DI_V2X_MODULE)
#include "../../di/drv_div2x/sunxi-di.h"
#endif
#if IS_ENABLED(CONFIG_SUNXI_DI_V3X) || IS_ENABLED(CONFIG_SUNXI_DI_V3X_MODULE)
#include "../../di/drv_div3x/di_client.h"
#include "../../di/drv_div3x/sunxi_di.h"
#endif

static struct di_client *car_client;
#define ALIGN_16B(x) (((x) + (15)) & ~(15))

struct buffer_node pre_node;
struct buffer_node cur_node;
struct buffer_node ref_node;

enum __di_pixel_fmt_t {
	DI_FORMAT_NV12 = 0x00,	/* 2-plane */
	DI_FORMAT_NV21 = 0x01,	/* 2-plane */
	DI_FORMAT_MB32_12 = 0x02, /* NOT SUPPORTED, UV mapping like NV12 */
	DI_FORMAT_MB32_21 = 0x03, /* NOT SUPPORTED, UV mapping like NV21 */
	DI_FORMAT_YV12 = 0x04,	/* 3-plane */
	DI_FORMAT_YUV422_SP_UVUV = 0x08, /* 2-plane, New in DI_V2.2 */
	DI_FORMAT_YUV422_SP_VUVU = 0x09, /* 2-plane, New in DI_V2.2 */
	DI_FORMAT_YUV422P = 0x0c,	/* 3-plane, New in DI_V2.2 */
	DI_FORMAT_MAX,
};

#if IS_ENABLED(CONFIG_SUNXI_DI_V3X) || IS_ENABLED(CONFIG_SUNXI_DI_V3X_MODULE)
static unsigned int get_di_fb_format(unsigned int fmt)
{
	switch (fmt) {
	case DI_FORMAT_YUV422P:
		return DRM_FORMAT_YUV422;
	case DI_FORMAT_YV12:
		return DRM_FORMAT_YUV420;
	case DI_FORMAT_YUV422_SP_UVUV:
		return DRM_FORMAT_NV16;
	case DI_FORMAT_YUV422_SP_VUVU:
		return DRM_FORMAT_NV61;
	case DI_FORMAT_NV12:
		return DRM_FORMAT_NV12;
	case DI_FORMAT_NV21:
		return DRM_FORMAT_NV21;
	default:
		return 0;
	}

	return 0;
}

int preview_enhancer_init(struct preview_params *params)
{
	struct di_timeout_ns t;
	struct di_dit_mode dit_mode;
	struct di_fmd_enable fmd_en;
	unsigned int src_width, src_height;
	unsigned int dst_width, dst_height;
	int ret;
	struct di_size src_size;
	struct di_rect out_crop;

	car_client = (struct di_client *)di_client_create("car_reverse");
	if (!car_client) {
		CAR_REVERSE_ERR("di_client_create failed\n");
		return -1;
	}

	ret = di_client_reset(car_client, NULL);
	if (ret) {
		CAR_REVERSE_ERR("di_client_reset failed\n");
		return -1;
	}
	t.wait4start = 500000000ULL;
	t.wait4finish = 600000000ULL;
	ret = di_client_set_timeout(car_client, &t);
	if (ret) {
		CAR_REVERSE_ERR("di_client_set_timeout failed\n");
		return -1;
	}
	dit_mode.intp_mode = DI_DIT_INTP_MODE_MOTION;
	dit_mode.out_frame_mode = DI_DIT_OUT_1FRAME;
	ret = di_client_set_dit_mode(car_client, &dit_mode);
	if (ret) {
		CAR_REVERSE_ERR("di_client_set_dit_mode failed\n");
		return -1;
	}
	fmd_en.en = 0;
	ret = di_client_set_fmd_enable(car_client, &fmd_en);
	if (ret) {
		CAR_REVERSE_ERR("di_client_set_fmd_enable failed\n");
		return -1;
	}

	src_height = params->src_height;
	src_width = params->src_width;
	dst_width = src_width;
	dst_height = src_height;
	src_size.width = src_width;
	src_size.height = src_height;
	ret = di_client_set_video_size(car_client,
					&src_size);
	if (ret) {
		CAR_REVERSE_ERR("di_client_set_video_size failed\n");
		return -1;
	}

	out_crop.left = 0;
	out_crop.top = 0;
	out_crop.right = src_size.width;
	out_crop.bottom = src_size.height;
	ret = di_client_set_video_crop(car_client, &out_crop);
	if (ret) {
		CAR_REVERSE_ERR("di_client_set_crop failed\n");
		return -1;
	}

	PREVIEW_ENHANCER_DBG("src_width:%d, src_height:%d\n", \
			     src_size.width, src_size.height);

	PREVIEW_ENHANCER_DBG("crop_left:%d, crop_right:%d, crop_top:%d, crop_bottom:%d\n",
		out_crop.left, out_crop.right, out_crop.top, out_crop.bottom);

	ret = di_client_check_para(car_client, NULL);
	if (ret) {
		CAR_REVERSE_ERR("di_client_check_para failed\n");
		return -1;
	}

	memset(&pre_node, 0, sizeof(struct buffer_node));
	memset(&cur_node, 0, sizeof(struct buffer_node));
	memset(&ref_node, 0, sizeof(struct buffer_node));

	return 0;
}

int preview_enhancer_exit(void)
{
	if (car_client) {
		di_client_destroy(car_client);
		memset(&pre_node, 0, sizeof(struct buffer_node));
		memset(&cur_node, 0, sizeof(struct buffer_node));
		memset(&ref_node, 0, sizeof(struct buffer_node));
	}

	return 0;
}

int preview_image_enhance(struct buffer_node *frame, struct buffer_node **di_frame, int src_w,
			 int src_h, int format)
{
	struct buffer_node *pre_frame = &pre_node;
	struct buffer_node *cur_frame = &cur_node;
	struct buffer_node *zero_frame = &ref_node;
	unsigned int src_width, src_height;
	unsigned int dst_width, dst_height;
	int ret = 0;

	if (!frame || !di_frame) {
		CAR_REVERSE_ERR("di src/dst buffer is not available!\n");
		return -1;
	}

	memcpy(cur_frame, frame, sizeof(struct buffer_node));

	/* judge if frame has valid date */
	if ((memcmp(pre_frame, zero_frame, sizeof(struct buffer_node)) != 0) \
	    && (memcmp(cur_frame, zero_frame, sizeof(struct buffer_node)) != 0)) {

		struct di_process_fb_arg fb_arg;
		struct di_fb *fb;
		src_height = src_h;
		src_width = src_w;
		dst_width = src_width;
		dst_height = src_height;
		memset(&fb_arg, 0, sizeof(fb_arg));
		fb_arg.is_interlace = 1;
		fb_arg.is_pulldown = 0;
		fb_arg.top_field_first = 0;

		/* set fb0 */
		fb = &fb_arg.in_fb0;
		fb->size.width = src_width;
		fb->size.height = src_height;
		fb->buf.cstride = dst_width;
		fb->buf.ystride = ALIGN_16B(dst_width);
		if (format ==
			V4L2_PIX_FMT_NV61)
			fb->format =
			get_di_fb_format(DI_FORMAT_YUV422_SP_VUVU);
		else
			fb->format =
			get_di_fb_format(DI_FORMAT_NV21);
		fb->dma_buf_fd = pre_frame->dmabuf_fd;
		fb->buf.cb_addr = ALIGN_16B(src_width) * src_height;
		fb->buf.cr_addr = 0;

		PREVIEW_ENHANCER_DBG("pre_frame.height:%d, pre_frame.width:%d\n", \
				     fb->size.width, fb->size.height);
		PREVIEW_ENHANCER_DBG("pre_frame.fd = %d, pre_frame.cb_offset:0x%llx, pre_frame.cr_offset:0x%llx\n", \
			fb->dma_buf_fd, fb->buf.cb_addr, fb->buf.cr_addr);
		/* set fb1 */
		fb = &fb_arg.in_fb1;
		fb->size.width = src_width;
		fb->size.height = src_height;
		fb->buf.cstride = dst_width;
		fb->buf.ystride = ALIGN_16B(dst_width);
		if (format ==
			V4L2_PIX_FMT_NV61)
			fb->format =
			get_di_fb_format(DI_FORMAT_YUV422_SP_VUVU);
		else
			fb->format =
			get_di_fb_format(DI_FORMAT_NV21);
		fb->dma_buf_fd = cur_frame->dmabuf_fd;
		fb->buf.cb_addr = ALIGN_16B(src_width) * src_height;
		fb->buf.cr_addr = 0;
		PREVIEW_ENHANCER_DBG("cur_frame.height:%d, cur_frame.width:%d\n", \
				     fb->size.width, fb->size.height);
		PREVIEW_ENHANCER_DBG("cur_frame.fd = %d, cur_frame.cb_offset:0x%llx, cur_frame.cr_offset:0x%llx\n", \
			fb->dma_buf_fd, fb->buf.cb_addr, fb->buf.cr_addr);

		/* set out_fb0 */
		fb = &fb_arg.out_dit_fb0;
		fb->size.width = dst_width;
		fb->size.height = dst_height;
		fb->buf.cstride = dst_width;
		fb->buf.ystride = ALIGN_16B(dst_width);
		if (format ==
			V4L2_PIX_FMT_NV61)
			fb->format =
			get_di_fb_format(DI_FORMAT_YUV422_SP_VUVU);
		else
			fb->format =
			get_di_fb_format(DI_FORMAT_NV21);
		fb->dma_buf_fd = di_frame[0]->dmabuf_fd;
		fb->buf.cb_addr =
			ALIGN_16B(dst_width) * dst_height;
		fb->buf.cr_addr = 0;
		PREVIEW_ENHANCER_DBG("next_frame.height:%d, next_frame.width:%d\n", \
				     fb->size.width, fb->size.height);
		PREVIEW_ENHANCER_DBG("next_frame.fd = %d, next_frame.cb_offset:0x%llx, next_frame.cr_offset:0x%llx\n", \
			fb->dma_buf_fd, fb->buf.cb_addr, fb->buf.cr_addr);

		ret = di_client_process_fb(car_client, &fb_arg);
		if (ret < 0) {
			CAR_REVERSE_ERR("di_client_process_fb fail\n");
			ret = -1;
		}
	} else {
		CAR_REVERSE_ERR("first frame from di is not ready!\n");
		ret = -1;
	}

	memcpy(pre_frame, cur_frame, sizeof(struct buffer_node));
	return ret;
}

#if IS_ENABLED(CONFIG_SUNXI_DI_V2X) || IS_ENABLED(CONFIG_SUNXI_DI_V2X_MODULE)
int preview_enhancer_init(struct preview_params *params)
{
	CAR_REVERSE_ERR("interface for DI_V2X is not support yet!\n");
	return 0;
}

int preview_enhancer_exit(void)
{
	CAR_REVERSE_ERR("interface for DI_V2X is not support yet!\n");
	return 0;
}

int preview_image_enhance(struct buffer_node *frame, struct buffer_node **di_frame, int src_w,
			 int src_h, int format)
{
	CAR_REVERSE_ERR("interface for DI_V2X is not support yet!\n");
	return -1;
}
#endif

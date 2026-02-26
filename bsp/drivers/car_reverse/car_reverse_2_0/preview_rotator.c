/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse preview rotation process module
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
#include <uapi/linux/sunxi-g2d.h>

#define ALIGN_16B(x) (((x) + (15)) & ~(15))

#if IS_ENABLED(CONFIG_AW_G2D) || IS_ENABLED(CONFIG_AW_G2D_MODULE)
extern int g2d_blit_h(g2d_blt_h *para);
extern int g2d_stretchblit(g2d_stretchblt *para);
extern int g2d_open(struct inode *inode, struct file *file);
extern int g2d_release(struct inode *inode, struct file *file);
extern int g2d_get_layout_version(void);
#endif

static struct mutex g2d_mutex;

int preview_rotator_init(struct preview_params *params)
{
	mutex_init(&g2d_mutex);

	return 0;
}

int preview_rotator_exit(void)
{
	return 0;
}


#if defined(G2D_SCALER)
static int preview_g2d_scaler(struct buffer_node *frame, struct buffer_node *rotate,
		       int src_w, int src_h)
{
	struct file g2d_file;
	g2d_blt_h blit_para;
	int ret = -1;

	g2d_open(0, &g2d_file);
	if (!frame || !rotate) {
		CAR_REVERSE_ERR("g2d src/dst buffer is not available!\n");
		return -1;
	}
	mutex_lock(&g2d_mutex);
	blit_para.flag_h = G2D_BLT_NONE_H;
	blit_para.src_image_h.use_phy_addr = 0;
	blit_para.src_image_h.fd = frame->dmabuf_fd;
	blit_para.src_image_h.mode = G2D_GLOBAL_ALPHA;
	blit_para.src_image_h.clip_rect.x = 0;
	blit_para.src_image_h.clip_rect.y = 0;
	blit_para.src_image_h.clip_rect.w = src_w;
	blit_para.src_image_h.clip_rect.h = src_h;
	blit_para.src_image_h.width = src_w;
	blit_para.src_image_h.height = src_h;
	blit_para.src_image_h.alpha = 0xff;

	blit_para.dst_image_h.use_phy_addr = 0;
	blit_para.dst_image_h.fd = rotate->dmabuf_fd;
	blit_para.dst_image_h.mode = G2D_GLOBAL_ALPHA;
	blit_para.dst_image_h.clip_rect.x = 0;
	blit_para.dst_image_h.clip_rect.y = 0;
	blit_para.dst_image_h.clip_rect.w = src_h;
	blit_para.dst_image_h.clip_rect.h = src_h;
	blit_para.dst_image_h.width = src_w;
	blit_para.dst_image_h.height = src_h;
	blit_para.dst_image_h.alpha = 0xff;

	ret = g2d_blit_h(&blit_para);
	if (ret < 0)
		CAR_REVERSE_ERR("preview_g2d_scaler failed!\n");
	mutex_unlock(&g2d_mutex);

	g2d_release(0, &g2d_file);

	return ret;
}
#endif

static int image_rotate_dmabuf(struct buffer_node *frame, struct buffer_node **rotate, int src_w,
			 int src_h, int angle, int format)
{
	struct file g2d_file;
	g2d_blt_h blit_para;
	int ret = -1;

	g2d_open(0, &g2d_file);
	if (!frame || !rotate) {
		CAR_REVERSE_ERR("g2d src/dst buffer is not available!\n");
		return -1;
	}

	PREVIEW_ROTATOR_DBG("src_w = %d, src_h = %d, rotation = 0x%x, format = 0x%x\n", \
			src_w, src_h, angle, format);
	PREVIEW_ROTATOR_DBG("src_buf->dmabuf_fd:%d, dst_buf->dmabuf_fd:%d\n", \
			    frame->dmabuf_fd, rotate[0]->dmabuf_fd);

	mutex_lock(&g2d_mutex);
	blit_para.src_image_h.use_phy_addr = 0;
	blit_para.src_image_h.fd = frame->dmabuf_fd;
	blit_para.src_image_h.mode = G2D_GLOBAL_ALPHA;
	blit_para.src_image_h.clip_rect.x = 0;
	blit_para.src_image_h.clip_rect.y = 0;
	blit_para.src_image_h.clip_rect.w = src_w;
	blit_para.src_image_h.clip_rect.h = src_h;
	blit_para.src_image_h.width = src_w;
	blit_para.src_image_h.height = src_h;
	blit_para.src_image_h.alpha = 0xff;
	blit_para.src_image_h.align[0] = 0;
	blit_para.src_image_h.align[1] = 0;
	blit_para.src_image_h.align[2] = 0;

	blit_para.dst_image_h.use_phy_addr = 0;
	blit_para.dst_image_h.fd = rotate[0]->dmabuf_fd;
	blit_para.dst_image_h.mode = G2D_GLOBAL_ALPHA;
	blit_para.dst_image_h.clip_rect.x = 0;
	blit_para.dst_image_h.clip_rect.y = 0;
	blit_para.dst_image_h.alpha = 0xff;
	blit_para.dst_image_h.align[0] = 0;
	blit_para.dst_image_h.align[1] = 0;
	blit_para.dst_image_h.align[2] = 0;

	switch (format) {

	case V4L2_PIX_FMT_BGR24:
		blit_para.src_image_h.format = G2D_FORMAT_ARGB8888;
		blit_para.dst_image_h.format = G2D_FORMAT_ARGB8888;
		break;
	case V4L2_PIX_FMT_NV21:
		blit_para.src_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
		blit_para.dst_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
		break;
	case V4L2_PIX_FMT_NV12:
		blit_para.src_image_h.format = G2D_FORMAT_YUV420UVC_V1U1V0U0;
		blit_para.dst_image_h.format = G2D_FORMAT_YUV420UVC_V1U1V0U0;
		break;
	case V4L2_PIX_FMT_NV61:
		blit_para.src_image_h.format = G2D_FORMAT_YUV422UVC_U1V1U0V0;
		blit_para.dst_image_h.format = G2D_FORMAT_YUV422UVC_U1V1U0V0;
		break;
	case V4L2_PIX_FMT_NV16:
		blit_para.src_image_h.format = G2D_FORMAT_YUV422UVC_V1U1V0U0;
		blit_para.dst_image_h.format = G2D_FORMAT_YUV422UVC_V1U1V0U0;
		break;
	case V4L2_PIX_FMT_YUV420:
		blit_para.src_image_h.format = G2D_FORMAT_YUV420_PLANAR;
		blit_para.dst_image_h.format = G2D_FORMAT_YUV420_PLANAR;
		break;
	case V4L2_PIX_FMT_YUV422P:
		blit_para.src_image_h.format = G2D_FORMAT_YUV422_PLANAR;
		blit_para.dst_image_h.format = G2D_FORMAT_YUV422_PLANAR;
		break;
	default:
		blit_para.src_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
		blit_para.dst_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
	}

	switch (angle) {
	case 0x1:
		blit_para.flag_h = G2D_ROT_90;
		blit_para.dst_image_h.width = src_h;
		blit_para.dst_image_h.height = src_w;
		blit_para.dst_image_h.clip_rect.w = src_h;
		blit_para.dst_image_h.clip_rect.h = src_w;
		break;
	case 0x2:
		blit_para.flag_h = G2D_ROT_180;
		blit_para.dst_image_h.width = src_w;
		blit_para.dst_image_h.height = src_h;
		blit_para.dst_image_h.clip_rect.w = src_w;
		blit_para.dst_image_h.clip_rect.h = src_h;
		break;
	case 0x3:
		blit_para.flag_h = G2D_ROT_270;
		blit_para.dst_image_h.width = src_h;
		blit_para.dst_image_h.height = src_w;
		blit_para.dst_image_h.clip_rect.w = src_h;
		blit_para.dst_image_h.clip_rect.h = src_w;
		break;
	case 0xe:
		blit_para.flag_h = G2D_ROT_H;
		blit_para.dst_image_h.width = src_w;
		blit_para.dst_image_h.height = src_h;
		blit_para.dst_image_h.clip_rect.w = src_w;
		blit_para.dst_image_h.clip_rect.h = src_h;
		break;
	case 0xf:
		blit_para.flag_h = G2D_ROT_V;
		blit_para.dst_image_h.width = src_w;
		blit_para.dst_image_h.height = src_h;
		blit_para.dst_image_h.clip_rect.w = src_w;
		blit_para.dst_image_h.clip_rect.h = src_h;
		break;
	default:
		blit_para.flag_h = G2D_ROT_0;
		blit_para.dst_image_h.width = src_w;
		blit_para.dst_image_h.height = src_h;
		blit_para.dst_image_h.clip_rect.w = src_w;
		blit_para.dst_image_h.clip_rect.h = src_h;
	}

	ret = g2d_blit_h(&blit_para);
	if (ret < 0)
		CAR_REVERSE_ERR("preview g2d_rotate failed!\n");
	mutex_unlock(&g2d_mutex);

	g2d_release(0, &g2d_file);

	return ret;

}

static int image_rotate_passthrough(struct buffer_node *frame, struct buffer_node **rotate, int src_w,
			 int src_h, int angle, int format)
{
	int ret = -1;
#if defined(G2D_PASSTHOUGH)
	g2d_stretchblt blit_para;
	struct file g2d_file;
	struct buffer_node *node = rotate[1];

	g2d_open(0, &g2d_file);
	if (!node || !frame) {
		CAR_REVERSE_ERR("g2d src/dst buffer is not available!\n");
		return -1;
	}

	mutex_lock(&g2d_mutex);

	blit_para.src_image.addr[0] =
		(unsigned long)frame->phy_address;
	blit_para.src_image.addr[1] =
		(unsigned long)frame->phy_address +
		preview[screen_id].src.w * preview[screen_id].src.h;
	blit_para.src_image.w = src_w;
	blit_para.src_image.h = src_h;
	if (preview[screen_id].format == V4L2_PIX_FMT_NV21)
		blit_para.src_image.format = G2D_FMT_PYUV420UVC;
	else
		blit_para.src_image.format = G2D_FMT_PYUV422UVC;
	blit_para.src_image.pixel_seq = G2D_SEQ_NORMAL;
	blit_para.src_rect.x = 0;
	blit_para.src_rect.y = 0;
	blit_para.src_rect.w = src_w;
	blit_para.src_rect.h = src_h;

	blit_para.dst_image.addr[0] =
		(unsigned long)node->phy_address;
	blit_para.dst_image.addr[1] =
		(unsigned long)node->phy_address + src_w * src_h;
	if (preview[screen_id].format == V4L2_PIX_FMT_NV21)
		blit_para.dst_image.format = G2D_FMT_PYUV420UVC;
	else
		blit_para.dst_image.format = G2D_FMT_PYUV422UVC;
	blit_para.dst_image.pixel_seq = G2D_SEQ_NORMAL;
	blit_para.dst_rect.x = 0;
	blit_para.dst_rect.y = 0;
	if (preview[screen_id].rotation == 1 ||
		preview[screen_id].rotation == 3) {
		blit_para.dst_image.w = src_h;
		blit_para.dst_image.h = src_w;
		blit_para.dst_rect.w = src_h;
		blit_para.dst_rect.h = src_w;
	} else {
		blit_para.dst_image.w = src_w;
		blit_para.dst_image.h = src_h;
		blit_para.dst_rect.w = src_w;
		blit_para.dst_rect.h = src_h;
	}
	blit_para.color = 0xff;
	blit_para.alpha = 0xff;

	switch (angle) {
	case 0x1:
		blit_para.flag = G2D_BLT_ROTATE90;
		break;
	case 0x2:
		blit_para.flag = G2D_BLT_ROTATE180;
		break;
	case 0x3:
		blit_para.flag = G2D_BLT_ROTATE270;
		break;
	case 0xe:
		blit_para.flag = G2D_BLT_FLIP_HORIZONTAL;
		break;
	case 0xf:
		blit_para.flag = G2D_BLT_FLIP_VERTICAL;
		break;
	default:
		blit_para.flag = G2D_BLT_NONE;
	}
	ret = g2d_stretchblit(&blit_para);
	mutex_unlock(&g2d_mutex);
	if (ret < 0) {
		CAR_REVERSE_ERR("g2d_stretchblit fail!\n");
	}

	g2d_release(0, &g2d_file);
	return ret;
#endif
	CAR_REVERSE_ERR("not support yet!\n");
	return ret;

}


int preview_image_rotate(struct buffer_node *frame, struct buffer_node **rotate, int src_w,
			 int src_h, int angle, int format)
{
	int g2d_version;
	int ret;

	g2d_version = g2d_get_layout_version();

	if (g2d_version == 2)
		ret = image_rotate_dmabuf(frame, rotate, src_w, src_h, angle, format);
	else
		ret = image_rotate_passthrough(frame, rotate, src_w, src_h, angle, format);

	return ret;
}

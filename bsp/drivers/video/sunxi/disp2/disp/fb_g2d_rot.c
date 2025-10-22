/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
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
#include "fb_g2d_rot.h"
#include "fb_top.h"

enum fb_rot_degree {
	FB_ROTATION_HW_0 = 0,
	FB_ROTATION_HW_90 = 1,
	FB_ROTATION_HW_180 = 2,
	FB_ROTATION_HW_270 = 3,
};

struct fb_g2d_rot_t {
	unsigned int in_buffer_cnt;
	g2d_blt_h info;
	enum fb_rot_degree degree;
	void *dst_vir_addr;
	dma_addr_t dst_phy_addr;
	unsigned int dst_mem_len;
};

int platform_format_get_bpp(enum disp_pixel_format format);
extern int g2d_bsp_blit_h(g2d_blt_h *para);
extern  int g2d_open(struct inode *inode, struct file *file);
extern  int g2d_release(struct inode *inode, struct file *file);
extern void g2d_ioctl_mutex_lock(void);
extern void g2d_ioctl_mutex_unlock(void);

static inline void show_debug_info(g2d_image_enh *p)
{
	fb_debug_inf("buf w h %d %d clip x %d y %d w %d h %d addr %x\n",
			p->width, p->height, p->clip_rect.x,  p->clip_rect.y,
			p->clip_rect.w,  p->clip_rect.h, p->laddr[0]);
}

int fb_g2d_degree_to_int_degree(int in)
{
	int out = 0;
	switch (in) {
	case FB_ROTATION_HW_0:
		out = 0;
		break;
	case FB_ROTATION_HW_90:
		out = 90;
		break;
	case FB_ROTATION_HW_180:
		out = 180;
		break;
	case FB_ROTATION_HW_270:
		out = 270;
		break;
	default:
		DE_WARN("%s: g2d Not support rotate degree %d\n", __FUNCTION__, in);
		break;
	}
	return out;
}

bool fb_g2d_check_rotate(__u32 *rot)
{
	bool ret = true;
	switch (*rot) {
	case FB_ROTATION_HW_0:
	case FB_ROTATION_HW_90:
	case FB_ROTATION_HW_180:
	case FB_ROTATION_HW_270:
		break;
	default:
		DE_WARN("%s: g2d Not support rotate degree %d\n", __FUNCTION__, *rot);
		*rot = FB_ROTATION_HW_0;
		break;
	}
	return ret;
}

static inline g2d_blt_flags_h fb_rot_to_g2d_rot(enum fb_rot_degree in)
{
	g2d_blt_flags_h out = G2D_ROT_90;
	switch (in) {
	case FB_ROTATION_HW_90:
		out = G2D_ROT_90;
		break;
	case FB_ROTATION_HW_180:
		out = G2D_ROT_180;
		break;
	case FB_ROTATION_HW_270:
		out = G2D_ROT_270;
		break;
	case FB_ROTATION_HW_0:
	default:
		DE_WARN("%s: g2d Not support rotate degree %d\n", __FUNCTION__, in);
		break;
	}
	return out;
}

void fb_g2d_get_rot_size(int rot_degree, int *width, int *height)
{
	int tmp = *width;
	switch (rot_degree) {
	case FB_ROTATION_HW_90:
	case FB_ROTATION_HW_270:
		*width = *height;
		*height = tmp;
		break;
	case FB_ROTATION_HW_0:
	case FB_ROTATION_HW_180:
		break;
	default:
		DE_WARN("%s: g2d Not support rotate degree %d\n", __FUNCTION__, rot_degree);
		break;
	}
}

int fb_g2d_get_new_rot_degree(int rot_config, int rot_request)
{
	int new_degree = rot_request;
	if (rot_config != 0)
		new_degree += rot_config;
	new_degree %= 4;
	return new_degree;
}

int fb_g2d_rot_exit(void *rot)
{
	struct fb_g2d_rot_t *inst = rot;
	struct file g2d_file;

	if (inst) {
		disp_free((void *__force)inst->dst_vir_addr,
		  (void *)inst->dst_phy_addr, inst->dst_mem_len);
		kfree(inst);
		g2d_release(0, &g2d_file);
	}
	return 0;
}

static inline g2d_fmt_enh disp_format_to_g2d_format(enum disp_pixel_format format)
{
	g2d_fmt_enh out_format;

	switch (format) {
	case DISP_FORMAT_ARGB_8888:
		out_format = G2D_FORMAT_ARGB8888;
		break;
	case DISP_FORMAT_ABGR_8888:
		out_format = G2D_FORMAT_ABGR8888;
		break;
	case DISP_FORMAT_RGBA_8888:
		out_format = G2D_FORMAT_RGBA8888;
		break;
	case DISP_FORMAT_BGRA_8888:
		out_format = G2D_FORMAT_BGRA8888;
		break;
	case DISP_FORMAT_RGB_888:
		out_format = G2D_FORMAT_RGB888;
		break;
	case DISP_FORMAT_BGR_888:
		out_format = G2D_FORMAT_BGR888;
		break;
	case DISP_FORMAT_RGB_565:
		out_format = G2D_FORMAT_RGB565;
		break;
	case DISP_FORMAT_BGR_565:
		out_format = G2D_FORMAT_BGR565;
		break;
	default:
		out_format = G2D_FORMAT_ARGB8888;
		DE_WARN("g2d Not support pixel format %d\n", format);
		break;
	}

	return out_format;
}

enum fb_rot_degree fb_g2d_get_degree(void *rot)
{
	struct fb_g2d_rot_t *fb_rot = rot;
	return fb_rot->degree;
}

int fb_g2d_rot_apply(void *rot, struct disp_layer_config *config, unsigned int yoffset)
{
	g2d_blt_h g2d_para;
	struct fb_g2d_rot_t *inst = rot;
	int ret = -1;

	if (!inst || !config) {
		DE_WARN("Null pointer\n");
		return ret;
	}
	if (inst->degree == FB_ROTATION_HW_0)
		return 0;
	fb_debug_inf("src:\n");
	show_debug_info(&inst->info.src_image_h);
	fb_debug_inf("dst:\n");
	show_debug_info(&inst->info.dst_image_h);

	inst->info.src_image_h.clip_rect.y = yoffset;

	inst->info.dst_image_h.clip_rect.y = inst->info.dst_image_h.clip_rect.y == 0 ?
						inst->info.dst_image_h.clip_rect.h : 0;
	config->info.fb.crop.y = ((long long)inst->info.dst_image_h.clip_rect.y) << 32;

	/* g2d_blit_h will modify input param, so use a copy */
	memcpy(&g2d_para, &inst->info, sizeof(g2d_para));
	g2d_ioctl_mutex_lock();
	ret = g2d_bsp_blit_h(&g2d_para);
	if (ret)
		DE_WARN("g2d_blit_h fail!ret:%d\n", ret);
	g2d_ioctl_mutex_unlock();
	return ret;
}

int fb_g2d_set_degree(void *rot, int degree, struct disp_layer_config *config)
{
	struct fb_g2d_rot_t *fb_rot = rot;

	if (degree == FB_ROTATION_HW_90 || degree == FB_ROTATION_HW_270) {
		fb_rot->info.src_image_h.width = fb_rot->info.dst_image_h.clip_rect.h;
		fb_rot->info.src_image_h.height = fb_rot->info.dst_image_h.clip_rect.w * fb_rot->in_buffer_cnt;
		fb_rot->info.src_image_h.clip_rect.w = fb_rot->info.dst_image_h.clip_rect.h;
		fb_rot->info.src_image_h.clip_rect.h = fb_rot->info.dst_image_h.clip_rect.w;
	} else {
		fb_rot->info.src_image_h.width = fb_rot->info.dst_image_h.clip_rect.w;
		fb_rot->info.src_image_h.height = fb_rot->info.dst_image_h.clip_rect.h * fb_rot->in_buffer_cnt;
		fb_rot->info.src_image_h.clip_rect.w = fb_rot->info.dst_image_h.clip_rect.w;
		fb_rot->info.src_image_h.clip_rect.h = fb_rot->info.dst_image_h.clip_rect.h;
	}
	if (degree == FB_ROTATION_HW_0) {
		config->info.fb.addr[0] = fb_rot->info.src_image_h.laddr[0];
	} else {
		config->info.fb.addr[0] = fb_rot->dst_phy_addr;
	}
	fb_rot->info.flag_h = fb_rot_to_g2d_rot(degree);

	fb_rot->degree = degree;
	return 0;
}

void *fb_g2d_rot_init(struct g2d_rot_create_info *info, struct disp_layer_config *config)
{
	int ret = -1;
	struct fb_g2d_rot_t *fb_rot = NULL;
	struct file g2d_file;

	if (!info) {
		DE_WARN("Null pointer\n");
		return NULL;
	}
	fb_rot = kmalloc(sizeof(struct fb_g2d_rot_t), GFP_KERNEL | __GFP_ZERO);
	if (!fb_rot) {
		DE_WARN("kmalloc fail!\n");
		return NULL;
	}
	ret = g2d_open(0, &g2d_file);
	if (ret)
		goto ERROR;

	fb_debug_inf("%s\n", __FUNCTION__);

	fb_rot->in_buffer_cnt = 8 * info->src_size / info->dst_height / info->dst_width /
				  platform_format_get_bpp (info->format);
	fb_rot->dst_mem_len = info->dst_height * info->dst_width * 2 *
				platform_format_get_bpp (info->format) / 8;
	fb_rot->dst_vir_addr =
		disp_malloc(fb_rot->dst_mem_len, (u32 *)(&fb_rot->dst_phy_addr));
	if (!fb_rot->dst_vir_addr || !fb_rot->dst_phy_addr)
		goto G2D_RELEASE;

	fb_rot->info.dst_image_h.laddr[0] = (u32)fb_rot->dst_phy_addr;
	fb_rot->info.dst_image_h.align[0] = 0;
	fb_rot->info.dst_image_h.align[1] = 0;
	fb_rot->info.dst_image_h.align[2] = 0;
	fb_rot->info.dst_image_h.clip_rect.x = 0;
	fb_rot->info.dst_image_h.clip_rect.y = 0;
	fb_rot->info.dst_image_h.alpha = 1;
	fb_rot->info.dst_image_h.mode = 255;
	fb_rot->info.dst_image_h.use_phy_addr = 1;
	fb_rot->info.dst_image_h.format = disp_format_to_g2d_format(info->format);
	fb_rot->info.dst_image_h.width = info->dst_width;
	fb_rot->info.dst_image_h.height = info->dst_height * 2;
	fb_rot->info.dst_image_h.clip_rect.w = info->dst_width;
	fb_rot->info.dst_image_h.clip_rect.h = info->dst_height;

	config->info.fb.crop.width = ((long long)info->dst_width) << 32;
	config->info.fb.crop.height = ((long long)info->dst_height) << 32;
	config->info.fb.size[0].width = info->dst_width;
	config->info.fb.size[0].height = info->dst_height;

	fb_rot->info.src_image_h.format = disp_format_to_g2d_format(info->format);
	fb_rot->info.src_image_h.laddr[0] = info->phy_addr;
	fb_rot->info.src_image_h.align[0] = 0;
	fb_rot->info.src_image_h.align[1] = 0;
	fb_rot->info.src_image_h.align[2] = 0;
	fb_rot->info.src_image_h.clip_rect.x = 0;
	fb_rot->info.src_image_h.clip_rect.y = 0;
	fb_rot->info.src_image_h.alpha = 1;
	fb_rot->info.src_image_h.mode = 255;
	fb_rot->info.src_image_h.use_phy_addr = 1;

	fb_g2d_set_degree(fb_rot, info->rot_degree, config);

	return fb_rot;
G2D_RELEASE:
	g2d_release(0, &g2d_file);
ERROR:
	kfree(fb_rot);
	return NULL;
}

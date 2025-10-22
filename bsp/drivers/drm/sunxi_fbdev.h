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

#ifndef __SUNXI_FBDEV_H__
#define __SUNXI_FBDEV_H__

#include <drm/drm_print.h>
#include <uapi/linux/fb.h>
#include <sunxi-log.h>

#include "sunxi_drm_drv.h"

extern int fb_debug_val;

#define fb_debug_inf(fmt, args...) \
	do {\
		if (unlikely(fb_debug_val)) {\
			if (fb_debug_val > 1)\
				sunxi_info(NULL, "[FB]: " fmt, ## args);\
			else\
				sunxi_err(NULL, "[FB]: " fmt, ## args);\
		} \
	} while (0)

struct fb_output_map {
	u32 hw_display;
	u32 hw_channel;
};

enum fb_output_mode {
	ADAPTIVE_STRETCH,
	FULL_STRETCH,
};

enum fb_format {
	ARGB8888 = 0,
	RGB888 = 1,
};

struct fb_create_info {
	struct drm_device *drm;
	enum fb_format format;
	u32 width;
	u32 height;
	u32 scn_width;
	u32 scn_height;
	unsigned long logo_offset;
	/*TODO support an fb map to two device */
	struct fb_output_map map;
	enum fb_output_mode mode;
	unsigned int fb_output_cnt;
};

struct drm_fb_info {
	void *par;
	void *pseudo_palette;
	union {
		char __iomem *screen_base;
		char *screen_buffer;
	};
	u32 reserved[3];
	unsigned int xres;
	unsigned int yres;
	unsigned int yoffset;
};
/* platform */
struct fb_hw_info;
struct display_channel_state;

int platform_get_private_size(void);
int platform_update_fb_output(struct fb_hw_info *hw_info, void *info);
int platform_fb_mmap(struct fb_hw_info *hw_info, struct vm_area_struct *vma);
int platform_fb_memory_alloc(struct fb_hw_info *hw_info, void **vir_addr, unsigned long *device_addr,
			    unsigned int w, unsigned int h, int fmt);
int platform_fb_memory_free(struct fb_hw_info *hw_info);
int platform_fb_pan_display_post_proc(struct fb_hw_info *hw_info);
int platform_fb_set_blank(struct fb_hw_info *hw_info, bool is_blank);
int platform_fb_init_finish(struct fb_hw_info *hw_info, void *info,
			    struct display_channel_state *out_state);

int platform_fb_init(struct fb_create_info *create, struct fb_hw_info *info, void **pseudo_palette);
int platform_fb_exit(struct fb_create_info *create, struct fb_hw_info *info);
int platform_fb_get_dmabuf(struct fb_hw_info *hw_info, int *fd);

#endif

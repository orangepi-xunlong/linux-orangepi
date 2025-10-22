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


#include <linux/dma-buf.h>
#include "sunxi_fbdev.h"

/* double buffer */
#define FB_BUFFER_CNT		2
void sunxi_disp_notify_init(void);
struct drm_fb_info *__fb_info;

int fb_debug_val;
static struct fb_create_info create_info;

struct drm_fb_info *drm_framebuffer_alloc(size_t size, struct device *dev)
{
#define BYTES_PER_LONG (BITS_PER_LONG/8)
#define PADDING (BYTES_PER_LONG - (sizeof(struct drm_fb_info) % BYTES_PER_LONG))
	int drm_fb_info_size = sizeof(struct drm_fb_info);
	struct drm_fb_info *info;
	char *p;

	if (size)
		drm_fb_info_size += PADDING;

	p = kzalloc(drm_fb_info_size + size, GFP_KERNEL);

	if (!p)
		return NULL;

	info = (struct drm_fb_info *) p;

	if (size)
		info->par = p + drm_fb_info_size;

	return info;
#undef PADDING
#undef BYTES_PER_LONG
}

static int drm_fb_config(struct drm_device *drm, struct fb_create_info *info)
{
	struct sunxi_logo_info logo;
	unsigned int w, h;
	int ret;

	ret = sunxi_drm_get_logo_info(drm, &logo, &w, &h);
	if (ret < 0) {
		DRM_ERROR("get logo info err:%d\n", ret);
		return -1;
	}

	if (logo.phy_addr) {
		info->format = logo.bpp == 32 ?
				  ARGB8888 : RGB888;
		info->width = logo.width;
		info->height = logo.height;
		info->logo_offset = logo.phy_addr;
	} else {
		info->format = ARGB8888;
		info->width = logo.width;
		info->height = logo.height;
		info->logo_offset = 0;
	}
	info->scn_width = w;
	info->scn_height = h;
	info->drm = drm;
	info->map.hw_display = 0;
	info->map.hw_channel = 0;
	info->mode = FULL_STRETCH;
	info->fb_output_cnt = 1;
	return 0;
}

static int drm_fb_init(struct fb_create_info *create, struct display_channel_state *out_state)
{
	int ret;
	void *virtual_addr;
	unsigned long device_addr;
	unsigned int height = create->height;
	unsigned int width = create->width;
	struct drm_fb_info *info;

	sunxi_disp_notify_init();
	info = drm_framebuffer_alloc(platform_get_private_size(), create->drm->dev);
	if (!info) {
		fb_debug_inf("framebuffer_alloc fail\n");
		return -ENOMEM;
	}

	ret = platform_fb_init(create, info->par, &info->pseudo_palette);
	if (ret < 0) {
		fb_debug_inf("platform_fb_init fail\n");
		goto free_fb;
	}
	ret = platform_fb_memory_alloc(info->par, &virtual_addr, &device_addr, width,
						height * FB_BUFFER_CNT, create->format);
	if (ret < 0) {
		fb_debug_inf("platform_fb_memory_alloc fail\n");
		goto exit_fb;
	}

	platform_fb_init_finish(info->par, info, out_state);
	info->yoffset = 0;
	info->screen_base = virtual_addr;
	info->xres = width;
	info->yres = height;
	__fb_info = info;
	fb_debug_inf("drm_fb vir 0x%p phy 0x%8lx\n", virtual_addr, device_addr);

	return 0;

exit_fb:
	platform_fb_exit(create, info->par);
free_fb:
	kfree(info);

	return ret;
}

static int drm_fb_exit(struct fb_create_info *create)
{
	platform_fb_exit(create, __fb_info->par);
	platform_fb_memory_free(__fb_info->par);

	return 0;
}

int sunxi_fbdev_init(struct drm_device *drm, struct display_channel_state *out_state)
{
	int ret;

	ret = drm_fb_config(drm, &create_info);
	if (ret)
		goto OUT;
	ret = drm_fb_init(&create_info, out_state);
OUT:
	return ret;
}

int sunxi_drm_fb_exit(void)
{
	drm_fb_exit(&create_info);
	return 0;
}

struct drm_fb_info *sunxi_get_drmfb_info(int hw_id)
{
	return __fb_info;
}

int sunxi_drmfb_pan_display(struct drm_fb_info *info)
{
	platform_update_fb_output(info->par, info);

	return 0;
}

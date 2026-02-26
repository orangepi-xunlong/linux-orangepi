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

#include <linux/fb.h>
#include <linux/dma-buf.h>
#include "sunxi_fbdev.h"

/* double buffer */
#define FB_BUFFER_CNT		2
#define FBIOGET_DMABUF         _IOR('F', 0x21, struct fb_dmabuf_export)

struct fb_dmabuf_export {
	int fd;
	__u32 flags;
};

struct dsi_notify {
	struct work_struct dsi_work;
	int blank;
};
struct dsi_notify __dsi_notify;
struct fb_info *__fb_info;
int fb_debug_val;
static struct fb_create_info create_info;

int fb_core_init(struct fb_create_info *create, struct display_channel_state *out_state);
int fb_core_exit(struct fb_create_info *create);

static int fb_config_init(struct drm_device *drm, struct fb_create_info *info)
{
	struct sunxi_logo_info logo;
	unsigned int w, h;

	sunxi_drm_get_logo_info(drm, &logo, &w, &h);
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

int sunxi_fbdev_init(struct drm_device *drm, struct display_channel_state *out_state)
{
	int ret;

	ret = fb_config_init(drm, &create_info);
	if (ret)
		goto OUT;
	ret = fb_core_init(&create_info, out_state);
OUT:
	return ret;
}

int sunxi_fbdev_exit(void)
{
	fb_core_exit(&create_info);
	return 0;
}

static int sunxi_fb_release(struct fb_info *info, int user)
{
	return 0;
}

static int sunxi_fb_open(struct fb_info *info, int user)
{
	return 0;
}

int sunxi_fb_pan_display(struct fb_var_screeninfo *var,
				struct fb_info *info)
{
	fb_debug_inf("fb %d pan display start update\n", info->node);
	platform_update_fb_output(info->par, var);
	fb_debug_inf("fb %d pan display update ok\n", info->node);

	if (var->reserved[0] == FB_ACTIVATE_FORCE)
		return 0;

	platform_fb_pan_display_post_proc(info->par);
	fb_debug_inf("fb %d pan display ok\n", info->node);
	return 0;
}

int sunxi_fb_wait_for_vsync(struct fb_info *info)
{
	platform_fb_pan_display_post_proc(info->par);
	fb_debug_inf("fb %d pan display ok\n", info->node);
	return 0;
}

static int sunxi_fb_get_dmabuf(struct fb_info *info, int *fd)
{
	return platform_fb_get_dmabuf(info->par, fd);
}

static int sunxi_fb_ioctl(struct fb_info *info, unsigned int cmd,
			  unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	switch (cmd) {
	case FBIO_WAITFORVSYNC:
		ret = sunxi_fb_wait_for_vsync(info);
		break;
	case FBIOGET_DMABUF:
	{
		struct fb_dmabuf_export k_ret = {-1, 0};

		ret = sunxi_fb_get_dmabuf(info, &k_ret.fd);
		if (ret < 0) {
			DRM_ERROR("fb export dmabuf ret %d fd %d\n", ret, k_ret.fd);
			break;
		}

		if (copy_to_user(argp, &k_ret, sizeof(struct fb_dmabuf_export))) {
			DRM_ERROR("fb copy to user err\n");
		}
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int sunxi_fb_check_var(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	return 0;
}

static int sunxi_fb_set_par(struct fb_info *info)
{
	return 0;
}

static int sunxi_fb_blank(int blank_mode, struct fb_info *info)
{
	if (!(blank_mode == FB_BLANK_POWERDOWN ||
		blank_mode == FB_BLANK_UNBLANK))
		return -1;
	return platform_fb_set_blank(info->par,
					blank_mode == FB_BLANK_POWERDOWN ?
						1 : 0);
}

static inline u32 convert_bitfield(int val, struct fb_bitfield *bf)
{
	u32 mask = ((1 << bf->length) - 1) << bf->offset;

	return (val << bf->offset) & mask;
}

static int sunxi_fb_setcolreg(unsigned int regno, unsigned int red, unsigned int green,
			      unsigned int blue, unsigned int transp,
			      struct fb_info *info)
{
	u32 val;
	u32 ret = 0;
	if (regno < 16) {
		val = convert_bitfield(transp, &info->var.transp) |
		convert_bitfield(red, &info->var.red) |
		convert_bitfield(green, &info->var.green) |
		convert_bitfield(blue, &info->var.blue);
		fb_debug_inf("regno=%2d,a=%2X,r=%2X,g=%2X,b=%2X,result=%08X\n",
				regno, transp, red, green, blue, val);
		((u32 *) info->pseudo_palette)[regno] = val;
	} else {
		ret = 1;
	}

	return ret;
}

static int sunxi_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	return platform_fb_mmap(info->par, vma);
}

static struct fb_ops __fb_ops = {
	.owner = THIS_MODULE,
	.fb_open = sunxi_fb_open,
	.fb_release = sunxi_fb_release,
	.fb_pan_display = sunxi_fb_pan_display,
#if IS_ENABLED(CONFIG_COMPAT)
	.fb_compat_ioctl = sunxi_fb_ioctl,
#endif
	.fb_ioctl = sunxi_fb_ioctl,
	.fb_check_var = sunxi_fb_check_var,
	.fb_set_par = sunxi_fb_set_par,
	.fb_blank = sunxi_fb_blank,
	.fb_mmap = sunxi_fb_mmap,
#if IS_ENABLED(CONFIG_AW_FB_CONSOLE)
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
#endif
	.fb_setcolreg = sunxi_fb_setcolreg,
};

static int fb_format_to_var(enum fb_format format, struct fb_var_screeninfo *var)
{
	switch (format) {
	case RGB888:
		var->bits_per_pixel = 24;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->blue.offset = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;
		break;
	default:
	case ARGB8888:
		var->bits_per_pixel = 32;
		var->transp.length = 8;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->blue.offset = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;
		var->transp.offset = var->red.offset + var->red.length;
		break;
	}
	return 0;
}

static int fb_init_var(void *hw_info, struct fb_var_screeninfo *var, enum fb_format format, u32 fb_width, u32 fb_height)
{
	var->nonstd = 0;
	var->xoffset = 0;
	var->yoffset = 0;
	var->xres = fb_width;
	var->yres = fb_height;
	var->xres_virtual = fb_width;
	var->yres_virtual = fb_height * FB_BUFFER_CNT;
	var->activate = FB_ACTIVATE_FORCE;
	var->pixclock = 0;
	var->left_margin = var->right_margin = 0;
	var->upper_margin = var->lower_margin = 0;
	var->hsync_len = var->vsync_len = 0;
	fb_format_to_var(format, var);
	return 0;
}

static int fb_init_fix(struct fb_fix_screeninfo *fix, unsigned long device_addr, int buffer_width, int buffer_height)
{
	fix->line_length = buffer_width;
	fix->smem_len = buffer_width * buffer_height;
	fix->smem_start = device_addr;
	fix->type_aux = 0;
	fix->xpanstep = 1;
	fix->ypanstep = 1;
	fix->ywrapstep = 0;
	fix->mmio_start = 0;
	fix->mmio_len = 0;
	fix->accel = FB_ACCEL_NONE;
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->visual = FB_VISUAL_TRUECOLOR;
	return 0;
}
static void dsi_notify_tp_work(struct work_struct *work)
{
	struct fb_event event;
	struct dsi_notify *tmp_work = container_of(work, struct dsi_notify, dsi_work);

	if (!__fb_info)
		return;
	event.info = __fb_info;
	event.data = &tmp_work->blank;
	fb_notifier_call_chain(FB_EVENT_BLANK, &event);
}
void dsi_notify_call_chain(int cmd, int flag)
{
	struct fb_event event;

	if (flag) {
		if (!__fb_info)
			return;
		event.info = __fb_info;
		event.data = &cmd;
		fb_notifier_call_chain(FB_EVENT_BLANK, &event);
	} else {
		__dsi_notify.blank = cmd;
		schedule_work(&__dsi_notify.dsi_work);
	}
}

struct fb_info *sunxi_get_fb_info(int fb_id)
{
	return __fb_info;
}

int fb_core_init(struct fb_create_info *create, struct display_channel_state *out_state)
{
	int ret;
	void *virtual_addr;
	struct fb_info *info;
	unsigned long device_addr;
	unsigned int height = create->height;
	unsigned int width = create->width;

	INIT_WORK(&__dsi_notify.dsi_work, dsi_notify_tp_work);
	info = framebuffer_alloc(platform_get_private_size(), create->drm->dev);
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

	fb_init_var(info->par, &info->var, create->format, width, height);
	fb_init_fix(&info->fix, device_addr, width * info->var.bits_per_pixel / 8, height * FB_BUFFER_CNT);
	info->screen_base = virtual_addr;
	info->fbops = &__fb_ops;
	info->flags = 0;

	register_framebuffer(info);
	platform_fb_init_finish(info->par, &info->var, out_state);
	__fb_info = info;
	fb_debug_inf("fb vir 0x%p phy 0x%8lx\n", virtual_addr, device_addr);
	return 0;

exit_fb:
	platform_fb_exit(create, info->par);
free_fb:
	framebuffer_release(info);
	return ret;
}

int fb_core_exit(struct fb_create_info *create)
{
	unregister_framebuffer(__fb_info);
	platform_fb_exit(create, __fb_info->par);
	platform_fb_memory_free(__fb_info->par);
	framebuffer_release(__fb_info);
	return 0;
}

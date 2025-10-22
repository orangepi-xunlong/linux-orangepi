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

#include "fb_top.h"
#include "fb_platform.h"

struct fb_dmabuf_export {
	int fd;
	__u32 flags;
};

/* double buffer */
#define FB_BUFFER_CNT		2
#define FBIOPAN_DISPLAY_SUNXI	0x4650
#define FBIOGET_DMABUF         _IOR('F', 0x21, struct fb_dmabuf_export)
struct fb_info **fb_infos;

struct fb_info *sunxi_get_fb_info(int fb_id)
{
	if (fb_infos && fb_infos[fb_id]) {
		return fb_infos[fb_id];
	}
	return NULL;
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

static struct dma_buf *sunxi_fb_get_dmabuf(struct fb_info *info)
{
	return platform_fb_get_dmabuf(info->par);
}

static int sunxi_fb_ioctl(struct fb_info *info, unsigned int cmd,
			  unsigned long arg)
{
	int ret = 0;
	struct fb_var_screeninfo var;
	void __user *argp = (void __user *)arg;
	switch (cmd) {
	case FBIOPAN_DISPLAY_SUNXI:
		if (copy_from_user(&var, argp, sizeof(var)))
			return -EFAULT;
		ret = sunxi_fb_pan_display(&var, info);
		break;
	case FBIO_WAITFORVSYNC:
		ret = sunxi_fb_wait_for_vsync(info);
		break;
	case FBIOGET_DMABUF:
	{
		struct dma_buf *dmabuf;
		struct fb_dmabuf_export k_ret = {-1, 0};

		ret = -1;
		dmabuf = sunxi_fb_get_dmabuf(info);

		if (IS_ERR_OR_NULL(dmabuf))
			return PTR_ERR(dmabuf);

		k_ret.fd = dma_buf_fd(dmabuf, O_CLOEXEC);
		if (k_ret.fd < 0) {
			dma_buf_put(dmabuf);
			break;
		}

		if (copy_to_user(argp, &k_ret, sizeof(struct fb_dmabuf_export))) {
			DE_WARN("copy to user err\n");
		}
		ret = 0;
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
	platform_fb_check_rotate(&var->rotate);
	return 0;
}

static int sunxi_fb_set_par(struct fb_info *info)
{
	fb_debug_inf("fb %d set rot %d\n", info->node, info->var.rotate);
	return platform_fb_set_rotate(info->par, info->var.rotate);
}

void test_rot(int rot)
{
	struct fb_var_screeninfo tmp;
	memcpy(&tmp, &fb_infos[0]->var, sizeof(tmp));
	tmp.rotate = rot;

	sunxi_fb_check_var(&tmp, fb_infos[0]);
	memcpy(&fb_infos[0]->var, &tmp, sizeof(tmp));
	sunxi_fb_set_par(fb_infos[0]);
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

static struct fb_ops dispfb_ops = {
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

static int fb_init_var(void *hw_info, struct fb_var_screeninfo *var, enum disp_pixel_format format, u32 fb_width, u32 fb_height)
{
	var->nonstd = 0;
	var->xoffset = 0;
	var->yoffset = 0;
	var->xres = fb_width;
	var->yres = fb_height;
	var->xres_virtual = fb_width;
	var->yres_virtual = fb_height * FB_BUFFER_CNT;
	var->activate = FB_ACTIVATE_FORCE;

	platform_format_to_var(format, var);
	platform_get_timing(hw_info, var);
	platform_get_physical_size(hw_info, var);
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

/* for gpu usage */
s32 sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para)
{
	if (fb_addr_para && fb_infos[0]) {
		fb_addr_para->fb_paddr = fb_infos[0]->fix.smem_start;
		fb_addr_para->fb_size = fb_infos[0]->fix.smem_len;
		return 0;
	}
	return -1;
}
EXPORT_SYMBOL(sunxi_get_fb_addr_para);

int fb_core_init(struct fb_create_info *create)
{
	int i, j;
	int bpp;
	int ret = 0;
	unsigned int height;
	unsigned int width;
	char *virtual_addr;
	struct fb_info *info;
	unsigned long long device_addr;
	platform_fb_init(&create->fb_share);
	bpp = platform_format_get_bpp(create->fb_share.format);
	fb_infos = kmalloc(sizeof(*fb_infos) * create->fb_share.fb_num, GFP_KERNEL | __GFP_ZERO);
	if (!fb_infos) {
		fb_debug_inf("malloc memory fail\n");
		return -1;
	}

	for (i = 0; i < create->fb_share.fb_num; i++) {
		info = framebuffer_alloc(platform_get_private_size(), create->fb_share.disp_dev);
		if (!info) {
			ret = -1;
			fb_debug_inf("framebuffer_alloc fail\n");
			goto alloc_err;
		}
		fb_infos[i] = info;
		width = create->fb_each[i].width;
		height = create->fb_each[i].height;
		platform_fb_init_for_each(info->par, &create->fb_each[i], &info->pseudo_palette, i);
		ret = platform_fb_memory_alloc(info->par, &virtual_addr, &device_addr,
						width * height * FB_BUFFER_CNT * bpp / 8);
		if (ret == -1) {
			fb_debug_inf("platform_fb_memory_alloc fail\n");
			goto alloc_err;
		}
		info->screen_base = virtual_addr;
		info->fbops = &dispfb_ops;
		info->flags = 0;
		fb_init_var(info->par, &info->var, create->fb_share.format, width, height);
		fb_init_fix(&info->fix, device_addr, width * info->var.bits_per_pixel / 8, height * FB_BUFFER_CNT);
		register_framebuffer(info);
		platform_fb_g2d_rot_init(info->par);
		platform_fb_init_logo(info->par, &info->var);
		fb_debug_inf("fb %d vir 0x%p phy 0x%8llx\n", i, virtual_addr, device_addr);
	}
	platform_fb_post_init();
	return ret;

alloc_err:
	for (j = i - 1; j >= 0; j--)
		framebuffer_release(fb_infos[j]);
	kfree(fb_infos);
	return ret;
}

int fb_core_exit(struct fb_create_info *create)
{
	int i;
	fb_debug_inf("%s start\n", __FUNCTION__);
	platform_fb_exit();
	for (i = 0; i < create->fb_share.fb_num; i++) {
		platform_fb_g2d_rot_exit(fb_infos[i]->par);
		unregister_framebuffer(fb_infos[i]);
		platform_fb_memory_free(fb_infos[i]->par);
		platform_fb_exit_for_each(fb_infos[i]->par);
		framebuffer_release(fb_infos[i]);
	}
	platform_fb_post_exit();
	kfree(fb_infos);
	return 0;
}

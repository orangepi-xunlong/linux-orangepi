/* Copyright (C) 2023 MacroSilicon Technology Co., Ltd.
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
 * usbdisp_fbdev.c -- Framebuffer driver for MacroSilicon chip 913x and 912x
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kfifo.h>
#include <asm/unaligned.h>
#include <drm/drm_fourcc.h>

#include "usb_hal_event.h"
#include "usb_hal_interface.h"
#include "usbdisp_fbdev.h"
#include "msfb.h"

#define EDID_LENGTH 0x80
#define MOD_VER "3.0.2.8"
#define MAX_CUSTOM_MODE_CNT 16

struct custom_mode_stru
{
	unsigned short width;
	unsigned short height;
	unsigned char rate;
	unsigned char vic;
};

static int option_parserd = 0;
static char *custom_mode = NULL;
static int custom_mode_cnt = 0;
static struct custom_mode_stru mode_arr[MAX_CUSTOM_MODE_CNT];

module_param(custom_mode, charp, S_IRUGO);
MODULE_PARM_DESC(custom_mode, "custom mode: (<vic>_<x>x<y>@<refr>[,<vic>_<x>x<y>@<refr>...]");

static int g_hal_index = 0;

void parser_custom_mode(void)
{
	char *start, *cur, *ori;
	int width, height, rate, vic;

	custom_mode_cnt = 0;
	memset(&mode_arr, 0, sizeof(mode_arr));

	if (!custom_mode)
	{
		return;
	}

	ori = kstrdup(custom_mode, GFP_KERNEL);
	if (!ori)
	{
		printk("%s:duplicate custom mode failed!\n", __func__);
		return;
	}

	start = ori;
	while (start)
	{
		cur = strsep(&start, ",");
		if (!cur)
		{
			break;
		}

		width = 0;
		height = 0;
		rate = 0;
		vic = 0;
		sscanf(cur, "%d_%dx%d@%d", &vic, &width, &height, &rate);
		if ((width != 0) && (height != 0) && (rate != 0) && (vic != 0))
		{
			printk("%s:parsed custom mode: width:%d height %d rate:%d vic:%d\n", __func__, width, height, rate, vic);
			mode_arr[custom_mode_cnt].width = width;
			mode_arr[custom_mode_cnt].height = height;
			mode_arr[custom_mode_cnt].rate = rate;
			mode_arr[custom_mode_cnt].vic = vic;
			custom_mode_cnt++;
		}
	}

	kfree(ori);
}

#ifndef CONFIG_FB_MODE_HELPERS
const struct fb_videomode vesa_modes[] = {
    /* 0 640x350-85 VESA */
    {NULL, 85, 640, 350, 31746, 96, 32, 60, 32, 64, 3,
     FB_SYNC_HOR_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 1 640x400-85 VESA */
    {NULL, 85, 640, 400, 31746, 96, 32, 41, 01, 64, 3,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 2 720x400-85 VESA */
    {NULL, 85, 721, 400, 28169, 108, 36, 42, 01, 72, 3,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 3 640x480-60 VESA */
    {NULL, 60, 640, 480, 39682, 48, 16, 33, 10, 96, 2,
     0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 4 640x480-72 VESA */
    {NULL, 72, 640, 480, 31746, 128, 24, 29, 9, 40, 2,
     0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 5 640x480-75 VESA */
    {NULL, 75, 640, 480, 31746, 120, 16, 16, 01, 64, 3,
     0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 6 640x480-85 VESA */
    {NULL, 85, 640, 480, 27777, 80, 56, 25, 01, 56, 3,
     0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 7 800x600-56 VESA */
    {NULL, 56, 800, 600, 27777, 128, 24, 22, 01, 72, 2,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 8 800x600-60 VESA */
    {NULL, 60, 800, 600, 25000, 88, 40, 23, 01, 128, 4,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 9 800x600-72 VESA */
    {NULL, 72, 800, 600, 20000, 64, 56, 23, 37, 120, 6,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 10 800x600-75 VESA */
    {NULL, 75, 800, 600, 20202, 160, 16, 21, 01, 80, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 11 800x600-85 VESA */
    {NULL, 85, 800, 600, 17761, 152, 32, 27, 01, 64, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 12 1024x768i-43 VESA */
    {NULL, 43, 1024, 768, 22271, 56, 8, 41, 0, 176, 8,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_INTERLACED, FB_MODE_IS_VESA},
    /* 13 1024x768-60 VESA */
    {NULL, 60, 1024, 768, 15384, 160, 24, 29, 3, 136, 6,
     0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 14 1024x768-70 VESA */
    {NULL, 70, 1024, 768, 13333, 144, 24, 29, 3, 136, 6,
     0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 15 1024x768-75 VESA */
    {NULL, 75, 1024, 768, 12690, 176, 16, 28, 1, 96, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 16 1024x768-85 VESA */
    {NULL, 85, 1024, 768, 10582, 208, 48, 36, 1, 96, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 17 1152x864-75 VESA */
    {NULL, 75, 1152, 864, 9259, 256, 64, 32, 1, 128, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 18 1280x960-60 VESA */
    {NULL, 60, 1280, 960, 9259, 312, 96, 36, 1, 112, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 19 1280x960-85 VESA */
    {NULL, 85, 1280, 960, 6734, 224, 64, 47, 1, 160, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 20 1280x1024-60 VESA */
    {NULL, 60, 1280, 1024, 9259, 248, 48, 38, 1, 112, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 21 1280x1024-75 VESA */
    {NULL, 75, 1280, 1024, 7407, 248, 16, 38, 1, 144, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 22 1280x1024-85 VESA */
    {NULL, 85, 1280, 1024, 6349, 224, 64, 44, 1, 160, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 23 1600x1200-60 VESA */
    {NULL, 60, 1600, 1200, 6172, 304, 64, 46, 1, 192, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 24 1600x1200-65 VESA */
    {NULL, 65, 1600, 1200, 5698, 304, 64, 46, 1, 192, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 25 1600x1200-70 VESA */
    {NULL, 70, 1600, 1200, 5291, 304, 64, 46, 1, 192, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 26 1600x1200-75 VESA */
    {NULL, 75, 1600, 1200, 4938, 304, 64, 46, 1, 192, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 27 1600x1200-85 VESA */
    {NULL, 85, 1600, 1200, 4357, 304, 64, 46, 1, 192, 3,
     FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
     FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 28 1792x1344-60 VESA */
    {NULL, 60, 1792, 1344, 4882, 328, 128, 46, 1, 200, 3,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 29 1792x1344-75 VESA */
    {NULL, 75, 1792, 1344, 3831, 352, 96, 69, 1, 216, 3,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 30 1856x1392-60 VESA */
    {NULL, 60, 1856, 1392, 4580, 352, 96, 43, 1, 224, 3,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 31 1856x1392-75 VESA */
    {NULL, 75, 1856, 1392, 3472, 352, 128, 104, 1, 224, 3,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 32 1920x1440-60 VESA */
    {NULL, 60, 1920, 1440, 4273, 344, 128, 56, 1, 200, 3,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 33 1920x1440-75 VESA */
    {NULL, 75, 1920, 1440, 3367, 352, 144, 56, 1, 224, 3,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 34 1920x1200-60 RB VESA */
    {NULL, 60, 1920, 1200, 6493, 80, 48, 26, 3, 32, 6,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 35 1920x1200-60 VESA */
    {NULL, 60, 1920, 1200, 5174, 336, 136, 36, 3, 200, 6,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 36 1920x1200-75 VESA */
    {NULL, 75, 1920, 1200, 4077, 344, 136, 46, 3, 208, 6,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 37 1920x1200-85 VESA */
    {NULL, 85, 1920, 1200, 3555, 352, 144, 53, 3, 208, 6,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 38 2560x1600-60 RB VESA */
    {NULL, 60, 2560, 1600, 3724, 80, 48, 37, 3, 32, 6,
     FB_SYNC_HOR_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 39 2560x1600-60 VESA */
    {NULL, 60, 2560, 1600, 2869, 472, 192, 49, 3, 280, 6,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 40 2560x1600-75 VESA */
    {NULL, 75, 2560, 1600, 2256, 488, 208, 63, 3, 280, 6,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 41 2560x1600-85 VESA */
    {NULL, 85, 2560, 1600, 1979, 488, 208, 73, 3, 280, 6,
     FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
    /* 42 2560x1600-120 RB VESA */
    {NULL, 120, 2560, 1600, 1809, 80, 48, 85, 3, 32, 6,
     FB_SYNC_HOR_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
};
#endif

static const struct fb_fix_screeninfo msfb_fix = {
    .id = "usbdisp_fb",
    .type = FB_TYPE_PLANES,
    .visual = FB_VISUAL_TRUECOLOR,
    .xpanstep = 0,
    .ypanstep = 1,
    .ywrapstep = 0,
    .accel = FB_ACCEL_NONE,
};

static ssize_t edid_show(
    struct file *filp,
    struct kobject *kobj, struct bin_attribute *a,
    char *buf, loff_t off, size_t count)
{
	struct device *fbdev = kobj_to_dev(kobj);
	struct fb_info *fb_info = dev_get_drvdata(fbdev);
	struct msfb_data *msfb = fb_info->par;

	if (msfb->edid == NULL)
		return 0;

	if ((off >= msfb->edid_size) || (count > msfb->edid_size))
		return 0;

	if (off + count > msfb->edid_size)
		count = msfb->edid_size - off;

	memcpy(buf, msfb->edid, count);

	return count;
}

static const struct bin_attribute edid_attr = {
    .attr.name = "edid",
    .attr.mode = 0444,
    .size = 2 * EDID_LENGTH,
    .read = edid_show,
};

static const u32 msfb_info_flags = FBINFO_DEFAULT | FBINFO_MISC_ALWAYS_SETPAR;

static unsigned int msfb_bitfield_to_fourcc(struct fb_var_screeninfo *var)
{
	unsigned int fourcc = 0;

	switch (var->bits_per_pixel)
	{
	case 16:
		if ((5 == var->red.length) && (11 == var->red.offset) && (5 == var->blue.length) && (0 == var->blue.offset))
		{
			fourcc = DRM_FORMAT_RGB565;
		}
		break;
	case 24:
		if ((8 == var->red.length) && (16 == var->red.offset))
		{
			fourcc = DRM_FORMAT_RGB888;
		}
		else
		{
			fourcc = DRM_FORMAT_BGR888;
		}
		break;
	default:
		if ((8 == var->red.length) && (16 == var->red.offset))
		{
			fourcc = DRM_FORMAT_XRGB8888;
		}
		else
		{
			fourcc = DRM_FORMAT_XBGR8888;
		}
		break;
	}

	return fourcc;
}

static unsigned int msfb_get_fourcc(struct fb_var_screeninfo *var)
{
	if (var->grayscale > 1)
	{
		return var->grayscale;
	}

	return msfb_bitfield_to_fourcc(var);
}

static void msfb_var_color_format(struct fb_var_screeninfo *var)
{
	var->bits_per_pixel = 24;
	var->red = (struct fb_bitfield){16, 8, 0};
	var->green = (struct fb_bitfield){8, 8, 0};
	var->blue = (struct fb_bitfield){0, 8, 0};
	var->transp = (struct fb_bitfield){0, 0, 0};
}

static int msfb_enable_dev(struct msfb_data *msfb)
{
	struct usb_hal *hal = msfb->hal;
	struct usb_hal_video_mode mode;
	struct fb_info *info;
	unsigned int fourcc;
	int ret;

	info = msfb->info;
	mode.width = info->var.xres;
	mode.height = info->var.yres;
	mode.rate = 60;

	ret = usb_hal_get_vic(hal, mode.width, mode.height, mode.rate, &mode.vic);
	if (ret)
	{
		dev_err(info->device, "get vic failed! width:%d height:%d\n\n", mode.width, mode.height);
		return ret;
	}

	fourcc = msfb_get_fourcc(&info->var);
	dev_info(info->device, "get fourcc:%d\n", fourcc);

	return usb_hal_enable(hal, &mode, fourcc);
}

static void msfb_disable_dev(struct msfb_data *msfb)
{
	struct usb_hal *hal = msfb->hal;

	usb_hal_disable(hal);
}

static int msfb_ops_release(struct fb_info *info, int user)
{
	struct msfb_data *msfb = info->par;
	msfb->fb_count--;
	dev_info(info->device, "fbdev released! count:%d\n", msfb->fb_count);
	return 0;
}

static int msfb_ops_open(struct fb_info *info, int user)
{
	struct msfb_data *msfb = info->par;
	msfb->fb_count++;
	dev_info(info->device, "fbdev opened! count:%d\n", msfb->fb_count);
	return 0;
}

static int msfb_ops_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long page, pos;

	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	if (size > info->fix.smem_len)
		return -EINVAL;
	if (offset > info->fix.smem_len - size)
		return -EINVAL;

	pos = (unsigned long)info->fix.smem_start + offset;

	dev_dbg(info->dev, "mmap() framebuffer addr:%lu size:%lu\n",
		pos, size);

	while (size > 0)
	{
		page = vmalloc_to_pfn((void *)pos);
		if (remap_pfn_range(vma, start, page, PAGE_SIZE, PAGE_SHARED))
			return -EAGAIN;

		start += PAGE_SIZE;
		pos += PAGE_SIZE;
		if (size > PAGE_SIZE)
			size -= PAGE_SIZE;
		else
			size = 0;
	}

	return 0;
}

static void msfb_ops_destroy(struct fb_info *info)
{
	struct msfb_data *msfb = info->par;
	vfree(info->screen_base);
	usb_put_dev(msfb->udev);

	while (!list_empty(&msfb->deferred_free))
	{
		struct msfb_deferred_free *d = list_entry(msfb->deferred_free.next, struct msfb_deferred_free, list);
		list_del(&d->list);
		vfree(d->mem);
		kfree(d);
	}

	if (msfb->edid)
	{
		kfree(msfb->edid);
	}

	kfree(msfb);
	(void)kfifo_free(&msfb->fifo);

	/* Assume info structure is freed after this point */
	framebuffer_release(info);
	printk("distroy fb end!\n");
}

static void msfb_deferred_vfree(struct msfb_data *msfb, void *mem)
{
	struct msfb_deferred_free *d = kmalloc(sizeof(struct msfb_deferred_free), GFP_KERNEL);
	if (!d)
		return;
	d->mem = mem;
	list_add(&d->list, &msfb->deferred_free);
}

static int msfb_realloc_framebuffer(struct msfb_data *msfb, struct fb_info *info, u32 new_len)
{
	u32 old_len = info->fix.smem_len;
	const void *old_fb = (const void __force *)info->screen_base;
	unsigned char *new_fb;

	new_len = PAGE_ALIGN(new_len);

	if (new_len > old_len)
	{
		/*
		 * Alloc system memory for virtual framebuffer
		 */
		new_fb = vmalloc(new_len);
		if (!new_fb)
		{
			dev_err(info->dev, "Virtual framebuffer alloc failed\n");
			return -ENOMEM;
		}
		memset(new_fb, 0xff, new_len);

		if (info->screen_base)
		{
			memcpy(new_fb, old_fb, old_len);
			msfb_deferred_vfree(msfb, (void __force *)info->screen_base);
		}

		info->screen_base = (char __iomem *)new_fb;
		info->fix.smem_len = new_len;
		info->fix.smem_start = (unsigned long)new_fb;
		info->flags = msfb_info_flags;

		dev_info(info->device, "screen base:0x%p smem_len:%d\n", info->screen_base, info->fix.smem_len);
	}

	return 0;
}

static int msfb_ops_set_par(struct fb_info *info)
{
	struct msfb_data *msfb = info->par;
	int result;
	u8 *pix_framebuffer;
	int i;
	struct fb_var_screeninfo fvs;
	u32 line_length;
	u32 bpp, fourcc;

	fourcc = msfb_get_fourcc(&info->var);
	bpp = usb_hal_get_bpp_by_fourcc(fourcc);
	dev_info(info->device, "var: fourcc:0x%x bpp:%d\n", fourcc, bpp);
	line_length = (info->var.xres * bpp) / 8;

	// for grayscale is fourcc, we set the bpp
	if (info->var.grayscale > 1)
	{
		info->var.bits_per_pixel = bpp;
	}

	/* clear the activate field because it causes spurious miscompares */
	fvs = info->var;
	fvs.activate = 0;
	msfb->fourcc = fourcc;
	// fvs.vmode &= ~FB_VMODE_SMOOTH_XPAN;

	if (!memcmp(&msfb->current_mode, &fvs, sizeof(struct fb_var_screeninfo)))
	{
		dev_info(info->device, "Same as current, nothing will be changed!\n");
		return 0;
	}

	dev_info(info->device, "xres:%d yres:%d xres_vir:%d yres_vir:%d bpp:%d line_len:%d\n", info->var.xres,
		 info->var.yres, info->var.xres_virtual, info->var.yres_virtual, info->var.bits_per_pixel, line_length);
	result = msfb_realloc_framebuffer(msfb, info, info->var.yres_virtual * line_length);
	if (result)
		return result;

	// result = dlfb_set_video_mode(msfb, &info->var);

	msfb->current_mode = fvs;
	info->fix.line_length = line_length;

	if (msfb->fb_count == 0)
	{
		pix_framebuffer = (u8 *)info->screen_base;
		for (i = 0; i < info->var.xres * info->var.yres_virtual; i++)
		{
			pix_framebuffer[3 * i] = 0;
			pix_framebuffer[3 * i + 1] = 0;
			pix_framebuffer[3 * i + 2] = 0xff;
		}
	}

	msfb_disable_dev(msfb);
	msfb_enable_dev(msfb);

	return 0;
}

static int msfb_ops_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct msfb_data *msfb = (struct msfb_data *)info->par;
	struct usb_hal *hal = msfb->hal;
	struct usb_hal_video_mode mode;
	unsigned int fourcc;
	int ret;

	dev_info(info->dev, "check var! xres:%d yres:%d xres_virt:%d yres_virt:%d\n", var->xres, var->yres,
		 var->xres_virtual, var->yres_virtual);

	if (var->yres_virtual < var->yres * 2)
	{
		dev_err(info->dev, "Only support pan, yres_virtual must be double of yres!\n");
	}

	if (var->grayscale < 1)
	{
		if ((var->bits_per_pixel != 16) && (var->bits_per_pixel != 24) && (var->bits_per_pixel != 32))
		{
			dev_err(info->dev, "Only support 16 24 32 bits!\n");
			return -EINVAL;
		}
	}

	if ((var->xres > MSFB_MAX_WIDTH) || (var->yres > MSFB_MAX_HEIGHT))
	{
		dev_err(info->dev, "Inavlid width or height. Max Width:%d Max Height:%d width:%d height:%d\n",
			MSFB_MAX_WIDTH, MSFB_MAX_HEIGHT, var->xres, var->yres);
		return -ERANGE;
	}

	if ((var->xres < MSFB_MIN_WIDTH) || (var->yres < MSFB_MIN_HEIGHT))
	{
		dev_err(info->dev, "Inavlid width or height. Min Width:%d Min Height:%d width:%d height:%d\n",
			MSFB_MIN_WIDTH, MSFB_MIN_HEIGHT, var->xres, var->yres);
		return -ERANGE;
	}

	fourcc = msfb_get_fourcc(var);
	ret = usb_hal_is_support_fourcc(fourcc);
	if (!ret)
	{
		dev_err(info->dev, "invalid fourcc:0x%x! grayscale:0x%x red:%d %d blue:%d %d n", fourcc, var->grayscale,
			var->red.length, var->red.offset, var->blue.length, var->blue.offset);
		return -EINVAL;
	}

	mode.width = var->xres;
	mode.height = var->yres;
	mode.rate = 60;
	mode.vic = 0;

	return usb_hal_video_mode_valid(hal, &mode);
}

int msfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct msfb_data *msfb = info->par;
	struct usb_hal *hal = msfb->hal;
	int len = var->yres * info->fix.line_length;

	char *src = info->screen_base + var->yoffset * info->fix.line_length;

	return usb_hal_update_frame(hal, src, info->fix.line_length, len, msfb->fourcc, 0);
}

static char *msfb_get_edid(struct msfb_data *msfb, int *edid_size)
{
	struct fb_info *info = msfb->info;
	int ret = 0, tries = 3;
	char *edid;
	struct usb_hal *hal = msfb->hal;
	char buf[EDID_LENGTH];
	char block_cnt;

	while (tries--)
	{
		ret = usb_hal_get_edid(hal, 0, buf, EDID_LENGTH);
		if (!ret)
		{
			break;
		}
	}

	if (ret)
	{
		return NULL;
	}

	block_cnt = buf[0x7e];

	edid = kmalloc((block_cnt + 1) * EDID_LENGTH, GFP_KERNEL);
	if (!edid)
	{
		dev_err(info->device, "kmalloc for edid failed1\n");
		return NULL;
	}

	memcpy(edid, buf, EDID_LENGTH);

	for (tries = 1; tries < (block_cnt + 1); tries++)
	{
		ret = usb_hal_get_edid(hal, tries, edid + tries * EDID_LENGTH, EDID_LENGTH);
		if (ret)
		{
			dev_err(info->device, "get edid block%d failed! ret=%d\n", tries, ret);
			kfree(edid);
			return NULL;
		}
	}

	*edid_size = ((block_cnt + 1) * EDID_LENGTH);
	return edid;
}

static int msfb_is_valid_mode(struct fb_videomode *mode, struct msfb_data *msfb)
{
	struct usb_hal *hal = msfb->hal;
	struct usb_hal_video_mode hal_mode;
	int ret;

	hal_mode.width = mode->xres;
	hal_mode.height = mode->yres;
	hal_mode.rate = mode->refresh;
	ret = usb_hal_video_mode_valid(hal, &hal_mode);

	return (0 == ret) ? 1 : 0;
}

static int msfb_setup_modes(struct msfb_data *msfb,
			    struct fb_info *info,
			    char *default_edid, size_t default_edid_size)
{
	char *edid;
	int i, ret = 0, edid_size = 0;
	struct device *dev = info->device;
	struct fb_videomode *mode;
	const struct fb_videomode *default_vmode = NULL;

	if (info->dev)
	{
		/* only use mutex if info has been registered */
		mutex_lock(&info->lock);
		/* parent device is used otherwise */
		dev = info->dev;
	}

	fb_destroy_modelist(&info->modelist);
	memset(&info->monspecs, 0, sizeof(info->monspecs));

	edid = msfb_get_edid(msfb, &edid_size);
	if (!edid)
	{
		goto try_default;
	}

	fb_edid_to_monspecs(edid, &info->monspecs);
	if (info->monspecs.modedb_len > 0)
	{
		msfb->edid = edid;
		msfb->edid_size = edid_size;
	}

	if (info->monspecs.modedb_len > 0)
	{
		for (i = 0; i < info->monspecs.modedb_len; i++)
		{
			mode = &info->monspecs.modedb[i];
			if (msfb_is_valid_mode(mode, msfb))
			{
				fb_add_videomode(mode, &info->modelist);
			}
			else
			{
				dev_dbg(dev, "Specified mode %dx%d too big\n",
					mode->xres, mode->yres);
				if (i == 0)
					/* if we've removed top/best mode */
					info->monspecs.misc &= ~FB_MISC_1ST_DETAIL;
			}
		}

		default_vmode = fb_find_best_display(&info->monspecs,
						     &info->modelist);
	}

try_default:
	if (default_vmode == NULL)
	{
		struct fb_videomode fb_vmode = {0};

		/*
		 * Add the standard VESA modes to our modelist
		 * Since we don't have EDID, there may be modes that
		 * overspec monitor and/or are incorrect aspect ratio, etc.
		 * But at least the user has a chance to choose
		 */
		for (i = 0; i < VESA_MODEDB_SIZE; i++)
		{
			mode = (struct fb_videomode *)&vesa_modes[i];
			if (msfb_is_valid_mode(mode, msfb))
				fb_add_videomode(mode, &info->modelist);
			else
				dev_dbg(dev, "VESA mode %dx%d too big\n",
					mode->xres, mode->yres);
		}

		/*
		 * default to resolution safe for projectors
		 * (since they are most common case without EDID)
		 */
		fb_vmode.xres = 800;
		fb_vmode.yres = 600;
		fb_vmode.refresh = 60;
		default_vmode = fb_find_nearest_mode(&fb_vmode, &info->modelist);
	}

	if ((default_vmode != NULL) && (msfb->fb_count == 0))
	{
		dev_info(info->device, "default videomod: width:%d height:%d refresh:%d\n", default_vmode->xres,
			 default_vmode->yres, default_vmode->refresh);
		fb_videomode_to_var(&info->var, default_vmode);
		info->var.yres_virtual = (2 * info->var.yres);
		msfb_var_color_format(&info->var);

		/*
		 * with mode size info, we can now alloc our framebuffer.
		 */
		memcpy(&info->fix, &msfb_fix, sizeof(msfb_fix));
	}
	else
	{
		ret = -EINVAL;
	}

	if (edid && (msfb->edid != edid))
		kfree(edid);

	if (info->dev)
		mutex_unlock(&info->lock);

	return ret;
}

static int msfb_ops_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct msfb_data *msfb = info->par;

	if (!atomic_read(&msfb->usb_active))
	{
		return 0;
	}

	if (FBIOGET_HDMI_HPD_MSFB == cmd)
	{
		void __user *argp = (void __user *)arg;
		unsigned int tmp = 0, status;
		int ret;

		if (usb_hal_get_hpd_status(msfb->hal, &tmp))
		{
			return -EIO;
		}
		else
		{
			status = (tmp ? MSFB_HDMI_HPD_CONNECTED : MSFB_HDMI_HPD_DISCONNECTED);
		}

		ret = copy_to_user(argp, &status, sizeof(status));
		if (ret)
		{
			return -EFAULT;
		}
		return 0;
	}

	return -EINVAL;
}

static const struct fb_ops msfb_ops = {
    .owner = THIS_MODULE,
    .fb_mmap = msfb_ops_mmap,
    .fb_ioctl = msfb_ops_ioctl,
    .fb_open = msfb_ops_open,
    .fb_release = msfb_ops_release,
    .fb_check_var = msfb_ops_check_var,
    .fb_set_par = msfb_ops_set_par,
    .fb_pan_display = msfb_pan_display,
    .fb_destroy = msfb_ops_destroy,
};

static void msfb_usb_disconnect(struct usb_interface *interface)
{
	struct msfb_data *msfb = usb_get_intfdata(interface);
	struct usb_hal *hal = msfb->hal;
	struct fb_info *info = msfb->info;
	int i;

	msfb_disable_dev(msfb);
	for (i = 0; i < 100; i++)
	{
		if (!usb_hal_is_disabled(hal))
		{
			msleep(50);
		}
	}

	usb_hal_destroy(hal);

	/* we virtualize until all fb clients release. Then we free */
	msfb->virtualized = true;

	/* When non-active we'll update virtual framebuffer, but no new urbs */
	atomic_set(&msfb->usb_active, 0);

	device_remove_bin_file(info->dev, &edid_attr);
	sysfs_remove_link(&info->dev->kobj, "hal");
	unregister_framebuffer(info);
}

static void msfb_init_var_info(struct fb_info *info)
{
	info->var.xres = MSFB_DEFAULT_WIDTH;
	info->var.yres = MSFB_DEFAULt_HEIGHT;
	info->var.xres_virtual = MSFB_DEFAULT_WIDTH;
	info->var.yres_virtual = MSFB_DEFAULt_HEIGHT * 2;
	info->var.xoffset = info->var.yoffset = 0;

	msfb_var_color_format(&info->var);
}

struct usb_hal *usb_intf_device_to_hal_func(struct device *dev)
{
	struct msfb_data *msfb = dev_get_drvdata(dev);
	return msfb->hal;
}

void msfb_add_custom_mode(struct msfb_data *msfb)
{
	int i;
	int ret;
	struct usb_hal *hal = msfb->hal;

	for (i = 0; i < custom_mode_cnt; i++)
	{
		ret = usb_hal_add_custom_mode(hal, mode_arr[i].width, mode_arr[i].height, mode_arr[i].rate, mode_arr[i].vic);
		printk("%s: add custom mode:width:%d height:%d rate:%d vic:%d %s\n", __func__,
		       mode_arr[i].width, mode_arr[i].height, mode_arr[i].rate, mode_arr[i].vic, ret ? "failed" : "success");
	}
}

static int msfb_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct msfb_data *msfb;
	struct fb_info *info;
	int ret = 0;
	struct usb_device *usbdev = interface_to_usbdev(intf);
	struct usb_hal *hal;

	printk("FrameBuffer Driver Version is: V_%s\n", MOD_VER);

	if (!option_parserd)
	{
		parser_custom_mode();
		option_parserd = 1;
	}

	msfb = kzalloc(sizeof(*msfb), GFP_KERNEL);
	if (!msfb)
	{
		dev_err(&intf->dev, "%s: failed to allocate msfb\n", __func__);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&msfb->deferred_free);

	msfb->udev = usb_get_dev(usbdev);
	usb_set_intfdata(intf, msfb);

	msfb->sku_pixel_limit = (MSFB_MAX_WIDTH * MSFB_MAX_HEIGHT); /* default to maximum */

	ret = kfifo_alloc(&msfb->fifo, 1024, GFP_KERNEL);
	if (ret)
	{
		dev_err(&intf->dev, "alloc kfifo failed!\n ret = %d\n", ret);
		ret = -ENOMEM;
		goto error;
	}

	hal = usb_hal_init(intf, id, &msfb->fifo, g_hal_index);
	if (!hal)
	{
		dev_err(&intf->dev, "usb hal init failed!\n");
		goto error;
	}

	g_hal_index++;
	msfb->hal = hal;

	msfb_add_custom_mode(msfb);

	info = framebuffer_alloc(0, &msfb->udev->dev);
	if (!info)
		goto error;

	msfb->info = info;
	info->par = msfb;
	info->pseudo_palette = msfb->pseudo_palette;
	msfb->ops = msfb_ops;
	info->fbops = &msfb->ops;

	INIT_LIST_HEAD(&info->modelist);

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret < 0)
	{
		dev_err(info->device, "cmap allocation failed: %d\n", ret);
		goto error;
	}

	msfb_init_var_info(info);
	ret = msfb_setup_modes(msfb, info, NULL, 0);
	if (ret != 0)
	{
		dev_err(info->device,
			"unable to find common mode for display and adapter\n");
		goto error;
	}

	atomic_set(&msfb->usb_active, 1);

	msfb_ops_check_var(&info->var, info);
	ret = msfb_ops_set_par(info);
	if (ret)
		goto error;

	ret = register_framebuffer(info);
	if (ret < 0)
	{
		dev_err(info->device, "unable to register framebuffer: %d\n",
			ret);
		goto error;
	}

	ret = msfb_pan_display(&info->var, info);
	if (ret)
	{
		dev_err(info->device, "send first screen failed!\n");
	}

	ret = device_create_bin_file(info->dev, &edid_attr);
	if (ret)
		dev_warn(info->device, "failed to create '%s' attribute: %d\n",
			 edid_attr.attr.name, ret);

	ret = sysfs_create_link(&info->dev->kobj, &intf->dev.kobj, "hal");
	if (ret)
	{
		dev_err(info->dev, "create syslink failed!ret=%d\n", ret);
	}

	dev_info(info->device,
		 "%s is Usbdisp device (%dx%d, %dK framebuffer memory)\n",
		 dev_name(info->dev), info->var.xres, info->var.yres,
		 ((msfb->backing_buffer) ? info->fix.smem_len * 2 : info->fix.smem_len) >> 10);

	return 0;
error:
	if (msfb->info)
	{
		msfb_ops_destroy(msfb->info);
	}
	else
	{
		usb_put_dev(msfb->udev);
		kfree(msfb);
	}

	return ret;
}

static const struct usb_device_id id_table[] = {
    {
	.idVendor = 0x345f,
	.idProduct = 0x9132,
	.bInterfaceClass = 0xff,
	.bInterfaceSubClass = 0x00,
	.bInterfaceProtocol = 0x00,
	.match_flags = USB_DEVICE_ID_MATCH_VENDOR |
		       USB_DEVICE_ID_MATCH_PRODUCT |
		       USB_DEVICE_ID_MATCH_INT_CLASS |
		       USB_DEVICE_ID_MATCH_INT_SUBCLASS |
		       USB_DEVICE_ID_MATCH_INT_PROTOCOL,
    },
    {},
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver msfb_driver = {
    .name = "usbdisp_fb",
    .probe = msfb_usb_probe,
    .disconnect = msfb_usb_disconnect,
    .id_table = id_table,
};

module_usb_driver(msfb_driver);

MODULE_DESCRIPTION("MacroSilicon kernel framebuffer driver");
MODULE_VERSION(MOD_VER);
MODULE_LICENSE("GPL v2");

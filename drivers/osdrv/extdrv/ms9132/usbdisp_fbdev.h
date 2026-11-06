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
 * usbdisp_fbdev.h -- Framebuffer driver for MacroSilicon chip 913x and 912x
 */

#ifndef __MS_USB_FB_H__
#define __MS_USB_FB_H__

#define MSFB_DEFAULT_WIDTH 1920
#define MSFB_DEFAULt_HEIGHT 1080
#define MSFB_MIN_WIDTH 640
#define MSFB_MIN_HEIGHT 480
#define MSFB_MAX_WIDTH 1920
#define MSFB_MAX_HEIGHT 1080

struct kfifo;
struct usb_hal;

struct video_mode
{
	u8 vic;
	u8 refresh_rate;
	u16 width;
	u16 height;
};

struct msfb_deferred_free
{
	struct list_head list;
	void *mem;
};

struct msfb_data
{
	struct usb_device *udev;
	struct fb_info *info;
	// struct urb_list urbs;
	char *backing_buffer;
	int fb_count;
	bool virtualized;    /* true when physical usb device not present */
	atomic_t usb_active; /* 0 = update virtual buffer, but no usb traffic */
	char *edid;	     /* null until we read edid from hw or get from sysfs */
	size_t edid_size;
	int sku_pixel_limit;
	u32 pseudo_palette[256];
	struct fb_ops ops;
	struct fb_var_screeninfo current_mode;
	struct list_head deferred_free;
	struct usb_hal *hal;
	u32 fourcc;
	struct kfifo fifo;
};

#endif
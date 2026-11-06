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
 * usb_hal_interface.c -- Framebuffer driver for MacroSilicon chip 913x and 912x
 */

#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#include <linux/mm_types.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/scatterlist.h>
#include <linux/kfifo.h>

#include <drm/drm_fourcc.h>

#include "usb_hal_edid.h"
#include "usb_hal_chip.h"
#include "usb_hal_interface.h"
#include "usb_hal_event.h"
#include "usb_hal_dev.h"
#include "usb_hal_thread.h"
#include "hal_adaptor.h"
#include "usb_device_hid.h"

void usb_hal_sysfs_init(struct usb_interface *interface);
void usb_hal_sysfs_exit(struct usb_interface *interface);

#define USB_HAL_COLOR_FORMAT_RGB 0
#define USB_HAL_COLOR_FORMAT_YUV 1

struct fourcc_format_desc
{
	u32 fourcc;
	u8 color_fmt;
	u8 bpp;
	u8 vpack_in;
};

struct yuv422p_pix
{
	u8 u0;
	u8 y0;
	u8 v0;
	u8 y1;
};

static struct fourcc_format_desc g_support_arr[] = {
    {DRM_FORMAT_RGB565, USB_HAL_COLOR_FORMAT_RGB, 16, USB_HAL_COLOR_FORMAT_RGB565},
    {DRM_FORMAT_RGB888, USB_HAL_COLOR_FORMAT_RGB, 24, USB_HAL_COLOR_FORMAT_RGB888},
    {DRM_FORMAT_BGR888, USB_HAL_COLOR_FORMAT_RGB, 24, USB_HAL_COLOR_FORMAT_RGB888},
    {DRM_FORMAT_XRGB8888, USB_HAL_COLOR_FORMAT_RGB, 32, USB_HAL_COLOR_FORMAT_RGB888},
    {DRM_FORMAT_XBGR8888, USB_HAL_COLOR_FORMAT_RGB, 32, USB_HAL_COLOR_FORMAT_RGB888},
    {DRM_FORMAT_ARGB8888, USB_HAL_COLOR_FORMAT_RGB, 32, USB_HAL_COLOR_FORMAT_RGB888},
    {DRM_FORMAT_ABGR8888, USB_HAL_COLOR_FORMAT_RGB, 32, USB_HAL_COLOR_FORMAT_RGB888},
    {DRM_FORMAT_NV12, USB_HAL_COLOR_FORMAT_YUV, 12, USB_HAL_COLOR_FORMAT_YUV422},
    {DRM_FORMAT_YUV420, USB_HAL_COLOR_FORMAT_YUV, 12, USB_HAL_COLOR_FORMAT_YUV422}};

static int g_support_num = (sizeof(g_support_arr) / sizeof(struct fourcc_format_desc));

int usb_hal_get_hpd_status(struct usb_hal *hal, u32 *status)
{
	struct usb_hal_dev *usb_dev;
	if (!hal || !status)
	{
		return -EINVAL;
	}

	usb_dev = (struct usb_hal_dev *)hal->private;
	return usb_dev->hal_dev->funcs->get_hpd_status(usb_dev->udev, status);
}

int usb_hal_get_edid(struct usb_hal *hal, int block, u8 *buf, u32 len)
{
	struct usb_hal_dev *usb_dev;
	if (!hal || !buf)
	{
		return -EINVAL;
	}

	usb_dev = (struct usb_hal_dev *)hal->private;
	return usb_dev->hal_dev->funcs->get_edid(usb_dev->udev, hal->chip_id, hal->port_type, hal->sdram_type, block, buf, len);
}

int usb_hal_check_mode_for_custom_mode(struct usb_hal_dev *usb_dev, struct usb_hal_video_mode *mode)
{
	int ret = 1;
	int i;

	for (i = 0; i < usb_dev->custom_mode_cnt; i++)
	{
		if ((usb_dev->custom_mode[i].width = mode->width) && (usb_dev->custom_mode[i].height == mode->height) && (usb_dev->custom_mode[i].rate == mode->rate))
		{
			ret = 0;
			break;
		}
	}

	return ret;
}

/* may be called multi times, can't record log */
int usb_hal_video_mode_valid(struct usb_hal *hal, struct usb_hal_video_mode *mode)
{
	struct usb_hal_dev *usb_dev;
	u32 min_sram_size, sram_size;
	u8 rate, vic = 0;
	u16 width, height;
	int ret;

	if (!hal || !mode)
	{
		return -EINVAL;
	}

	usb_dev = (struct usb_hal_dev *)hal->private;

	// first check custom mode. if check success, return success, otherwise continue check
	ret = usb_hal_check_mode_for_custom_mode(usb_dev, mode);
	if (!ret)
	{
		return ret;
	}

	// check w&h is supported
	width = (u16)mode->width;
	height = (u16)mode->height;
	rate = (u8)mode->rate;
	ret = usb_dev->hal_dev->funcs->get_mode_vic(width, height, rate, &vic);
	if (ret)
	{
		return ret;
	}

	// check chip's sram size
	// min pix size is 2 bytes in mem, sdram is divided 2 frames, so min sram size is w * h * 2 * 2
	min_sram_size = (((u32)(mode->width * mode->height)) << 2);
	sram_size = SDRAM_TYPE_TO_SIZE(hal->sdram_type);
	if (min_sram_size > sram_size)
	{
		return -EINVAL;
	}

	return 0;
}

int usb_hal_get_vic_for_cunstom_mode(struct usb_hal_dev *usb_dev, u16 width, u16 height, u8 rate, u8 *vic)
{
	int ret = 1;
	int i;

	for (i = 0; i < usb_dev->custom_mode_cnt; i++)
	{
		if ((usb_dev->custom_mode[i].width = width) && (usb_dev->custom_mode[i].height == height) && (usb_dev->custom_mode[i].rate == rate))
		{
			*vic = usb_dev->custom_mode[i].vic;
			ret = 0;
			break;
		}
	}

	return ret;
}

int usb_hal_get_vic(struct usb_hal *hal, u16 width, u16 height, u8 rate, u8 *vic)
{
	int ret;
	struct usb_hal_dev *usb_dev;
	if (!hal || !vic)
	{
		return -EINVAL;
	}

	usb_dev = (struct usb_hal_dev *)hal->private;
	ret = usb_hal_get_vic_for_cunstom_mode(usb_dev, width, height, rate, vic);
	if (!ret)
	{
		return ret;
	}

	return usb_dev->hal_dev->funcs->get_mode_vic(width, height, rate, vic);
}

static struct fourcc_format_desc *usb_hal_find_desc(u32 fourcc)
{
	int i;
	struct fourcc_format_desc *desc = NULL;
	for (i = 0; i < g_support_num; i++)
	{
		if (g_support_arr[i].fourcc == fourcc)
		{
			desc = &g_support_arr[i];
			break;
		}
	}

	return desc;
}

unsigned int usb_hal_get_bpp_by_fourcc(u32 fourcc)
{
	struct fourcc_format_desc *desc = usb_hal_find_desc(fourcc);
	return desc ? desc->bpp : 0;
}

int usb_hal_is_support_fourcc(u32 fourcc)
{
	struct fourcc_format_desc *desc = usb_hal_find_desc(fourcc);

	return desc ? 1 : 0;
}

int usb_hal_enable(struct usb_hal *hal, struct usb_hal_video_mode *mode, u32 fourcc)
{
	struct usb_hal_dev *usb_dev;
	struct usb_hal_event event;
	u8 color_in;
	struct fourcc_format_desc *desc;

	if (!hal || !mode)
	{
		return -EINVAL;
	}

	usb_dev = (struct usb_hal_dev *)hal->private;

	if (usb_hal_video_mode_valid(hal, mode))
	{
		dev_err(&usb_dev->udev->dev, "mode is invalid! width:%d height:%d chip_type:%d\n", mode->width, mode->height, hal->sdram_type);
		return -EINVAL;
	}

	if (!usb_hal_is_support_fourcc(fourcc))
	{
		dev_err(&usb_dev->udev->dev, "invalid fourcc:0x%x!\n", fourcc);
		return -EINVAL;
	}

	usb_dev->mode = *mode;

	desc = usb_hal_find_desc(fourcc);
	usb_dev->color_out = ((desc->bpp > 16) ? USB_HAL_COLOR_FORMAT_RGB888 : USB_HAL_COLOR_FORMAT_RGB565);
	usb_dev->vpack_in = desc->vpack_in;

	color_in = ((usb_dev->vpack_out << 4) | usb_dev->vpack_in);

	memset(&event, 0, sizeof(event));
	event.base.type = USB_HAL_EVENT_TYPE_ENABLE;
	event.base.length = sizeof(event);
	event.para.enable.width = mode->width;
	event.para.enable.height = mode->height;
	event.para.enable.vic = mode->vic;
	event.para.enable.trans_mode = usb_dev->trans_mode;
	event.para.enable.color_in = color_in;
	event.para.enable.color_out = usb_dev->color_out;

	kfifo_in(usb_dev->fifo, &event, sizeof(event));
	up(&usb_dev->sema);

	return 0;
}

int usb_hal_disable(struct usb_hal *hal)
{
	struct usb_hal_dev *usb_dev;
	struct usb_hal_event event;
	if (!hal)
	{
		return -EINVAL;
	}

	usb_dev = (struct usb_hal_dev *)hal->private;

	memset(&event, 0, sizeof(event));
	event.base.type = USB_HAL_EVENT_TYPE_DISABLE;
	event.base.length = sizeof(event);
	kfifo_in(usb_dev->fifo, &event, sizeof(event));
	up(&usb_dev->sema);

	return 0;
}

int usb_hal_is_disabled(struct usb_hal *hal)
{
	struct usb_hal_dev *usb_dev;

	if (!hal)
	{
		return 1;
	}

	usb_dev = (struct usb_hal_dev *)hal->private;
	return (USB_HAL_DEV_STATE_DISABLED == usb_dev->state) ? 1 : 0;
}

static void usb_hal_rgb32_to_bgr888_line(u8 *dbuf, u8 *sbuf, unsigned int pixels, int is_rgb)
{
	unsigned int x;

	if (is_rgb)
	{
		for (x = 0; x < pixels; x++)
		{
			*dbuf++ = *sbuf;
			*dbuf++ = *(sbuf + 1);
			*dbuf++ = *(sbuf + 2);
			sbuf += 4;
		}
	}
	else
	{
		for (x = 0; x < pixels; x++)
		{
			*dbuf++ = *(sbuf + 2);
			*dbuf++ = *(sbuf + 1);
			*dbuf++ = *sbuf;
			sbuf += 4;
		}
	}
}

int usb_hal_cpy_rgb32_to_rgb24(char *src, char *dst, int pitch, int width, int height, int is_rgb)
{
	size_t linepixels = width;
	size_t dst_len = linepixels * 3;
	unsigned y, lines = height;
	int cpy_len = 0;

	for (y = 0; y < lines; y++)
	{
		usb_hal_rgb32_to_bgr888_line(dst, src, linepixels, is_rgb);
		cpy_len += dst_len;
		src += pitch;
		dst += dst_len;
	}

	return cpy_len;
}

static void usb_hal_cpy_bgr24_to_rgb24(char *src, char *dst, int pix_cnt)
{
	int i;
	for (i = 0; i < pix_cnt; i++)
	{
		dst[i * 3] = src[i * 3 + 2];
		dst[i * 3 + 1] = src[i * 3 + 1];
		dst[i * 3 + 2] = src[i * 3];
	}
}

static int usb_hal_rgb_copy(struct usb_hal_dev *usb_dev, u8 *buf, int pitch, u32 len, struct fourcc_format_desc *desc)
{
	int cpy_len;
	struct usb_hal_buffer *usb_buf = &usb_dev->usb_buf;

	if ((DRM_FORMAT_RGB565 == desc->fourcc) || (DRM_FORMAT_RGB888 == desc->fourcc))
	{
		memcpy(usb_buf->buf, buf, len);
		cpy_len = len;
	}
	else if (DRM_FORMAT_BGR888 == desc->fourcc)
	{
		usb_hal_cpy_bgr24_to_rgb24(buf, usb_buf->buf, usb_dev->mode.width * usb_dev->mode.height);
		cpy_len = len;
	}
	else
	{
		int is_rgb;
		is_rgb = ((DRM_FORMAT_XRGB8888 == desc->fourcc) || (DRM_FORMAT_ARGB8888 == desc->fourcc)) ? 1 : 0;
		cpy_len = usb_hal_cpy_rgb32_to_rgb24(buf, usb_buf->buf, pitch, usb_dev->mode.width, usb_dev->mode.height, is_rgb);
	}

	return cpy_len;
}

static void cpy_yplane_to_yuv422p(struct yuv422p_pix *dst, u8 *ybuf, int width, int height)
{
	int i, j;

	for (i = 0; i < height; i += 1)
	{
		for (j = 0; j < width; j += 2)
		{
			dst->y0 = *ybuf;
			dst->y1 = *(ybuf + 1);
			ybuf += 2;
			dst++;
		}
	}
}

static void cpy_uvplane_to_yuv422p(struct yuv422p_pix *dst, u8 *uvbuf, int width, int height, int dy)
{
	int i, j;

	for (i = 0; i < height / dy; i += 1)
	{
		for (j = 0; j < width / 2; j += 1)
		{
			dst->u0 = *uvbuf;
			dst->v0 = *(uvbuf + 1);
			uvbuf += 2;
			dst += 1;
		}
		dst += ((dy - 1) * width / 2);
	}
}

static void cpy_u_v_plane_to_yuv422p(struct yuv422p_pix *dst, u8 *ubuf, u8 *vbuf, int width, int height, int dy)
{
	int i, j;

	for (i = 0; i < height / dy; i += 1)
	{
		for (j = 0; j < width / 2; j += 1)
		{
			dst->u0 = *ubuf;
			dst->v0 = *vbuf;
			ubuf++;
			vbuf++;
			dst += 1;
		}
		dst += ((dy - 1) * width / 2);
	}
}

static void scale_up_linear(struct yuv422p_pix *dst, int width, int height)
{
	int i, j;
	struct yuv422p_pix *top, *bottom, *cur, *row, *prev_row, *next_row;

	for (i = 1; i < height - 1; i += 2)
	{
		prev_row = dst + (i - 1) * width / 2;
		row = dst + i * width / 2;
		next_row = dst + (i + 1) * width / 2;
		for (j = 0; j < width / 2; j++)
		{
			cur = row + j;
			top = prev_row + j;
			bottom = next_row + j;

			cur->u0 = ((u16)top->u0 + (u16)bottom->u0) / 2;
			cur->v0 = ((u16)top->v0 + (u16)bottom->v0) / 2;
		}
	}

	// process last row
	cur = dst + (height - 1) * width / 2;
	top = dst + (height - 2) * width / 2;
	for (i = 0; i < width / 2; i++)
	{
		cur->u0 = top->u0;
		cur->v0 = top->v0;
		cur++;
		top++;
	}
}

static int usb_hal_cpy_nv16_to_yuv422p(struct usb_hal_dev *usb_dev, u8 *buf, u32 len)
{
	int width, height;
	u8 *uv_start;
	struct usb_hal_buffer *usb_buf = &usb_dev->usb_buf;
	struct yuv422p_pix *dst;

	width = usb_dev->mode.width;
	height = usb_dev->mode.height;

	dst = (struct yuv422p_pix *)usb_buf->buf;
	cpy_yplane_to_yuv422p(dst, buf, width, height);

	uv_start = (buf + width * height);
	cpy_uvplane_to_yuv422p(dst, uv_start, width, height, 1);

	return len;
}

static int usb_hal_cpy_nv12_to_yuv422p(struct usb_hal_dev *usb_dev, u8 *buf, u32 len)
{
	int width, height;
	u8 *uv_start;
	struct usb_hal_buffer *usb_buf = &usb_dev->usb_buf;
	struct yuv422p_pix *dst;

	width = usb_dev->mode.width;
	height = usb_dev->mode.height;

	dst = (struct yuv422p_pix *)usb_buf->buf;
	cpy_yplane_to_yuv422p(dst, buf, width, height);

	uv_start = (buf + width * height);
	cpy_uvplane_to_yuv422p(dst, uv_start, width, height, 2);
	scale_up_linear(dst, width, height);

	return width * height * 2;
}

static int usb_hal_cpy_yuv420_to_yuv422p(struct usb_hal_dev *usb_dev, u8 *buf, u32 len)
{
	int width, height;
	u8 *ubuf, *vbuf;
	struct usb_hal_buffer *usb_buf = &usb_dev->usb_buf;
	struct yuv422p_pix *dst;

	width = usb_dev->mode.width;
	height = usb_dev->mode.height;

	dst = (struct yuv422p_pix *)usb_buf->buf;
	cpy_yplane_to_yuv422p(dst, buf, width, height);

	ubuf = (buf + width * height);
	vbuf = ubuf + width * height / 4;
	cpy_u_v_plane_to_yuv422p(dst, ubuf, vbuf, width, height, 2);
	scale_up_linear(dst, width, height);

	return width * height * 2;
}

static int usb_hal_yuv_copy(struct usb_hal_dev *usb_dev, u8 *buf, u32 len, struct fourcc_format_desc *desc)
{
	int cpy_len;

	switch (desc->fourcc)
	{
	case DRM_FORMAT_NV16:
		cpy_len = usb_hal_cpy_nv16_to_yuv422p(usb_dev, buf, len);
		break;
	case DRM_FORMAT_NV24:
		break;
	case DRM_FORMAT_NV12:
		cpy_len = usb_hal_cpy_nv12_to_yuv422p(usb_dev, buf, len);
		break;
	case DRM_FORMAT_YUV420:
		cpy_len = usb_hal_cpy_yuv420_to_yuv422p(usb_dev, buf, len);
		break;
	case DRM_FORMAT_YUV422:
		break;
	default:
		break;
	}

	return cpy_len;
}

int usb_hal_update_frame(struct usb_hal *hal, u8 *buf, int pitch, u32 len, u32 fourcc, int try_lock)
{
	struct usb_hal_dev *usb_dev;
	struct fourcc_format_desc *desc;
	struct usb_hal_event event;
	struct usb_hal_buffer *usb_buf;
	int cpy_len = 0;

	if (!hal || !buf)
	{
		return -EINVAL;
	}

	usb_dev = (struct usb_hal_dev *)hal->private;

	if (usb_dev->state != USB_HAL_DEV_STATE_ENABLED)
	{
		usb_dev->stat.state_error++;
		return -EPERM;
	}

	usb_buf = &usb_dev->usb_buf;
	desc = usb_hal_find_desc(fourcc);

	if (try_lock)
	{
		int ret;
		ret = mutex_trylock(&usb_buf->mutex);
		if (!ret)
		{
			usb_dev->stat.try_lock_fail++;
			return -EBUSY;
		}
	}
	else
	{
		mutex_lock(&usb_buf->mutex);
	}

	if (USB_HAL_COLOR_FORMAT_RGB == desc->color_fmt)
	{
		cpy_len = usb_hal_rgb_copy(usb_dev, buf, pitch, len, desc);
	}
	else
	{
		cpy_len = usb_hal_yuv_copy(usb_dev, buf, len, desc);
	}

	mutex_unlock(&usb_buf->mutex);

	memset(&event, 0, sizeof(event));
	event.base.type = USB_HAL_EVENT_TYPE_UPDATE;
	event.base.length = sizeof(event);
	event.para.update.len = cpy_len;
	kfifo_in(usb_dev->fifo, &event, sizeof(event));
	up(&usb_dev->sema);

	return 0;
}

int usb_hal_add_custom_mode(struct usb_hal *hal, int width, int height, int rate, unsigned char vic)
{
	struct usb_hal_dev *usb_dev;

	if (!hal)
	{
		return -EINVAL;
	}

	usb_dev = (struct usb_hal_dev *)hal->private;
	if (usb_dev->custom_mode_cnt >= USB_HAL_MAX_CUSTOM_MODE)
	{
		return -ERANGE;
	}

	usb_dev->custom_mode[usb_dev->custom_mode_cnt].width = width;
	usb_dev->custom_mode[usb_dev->custom_mode_cnt].height = height;
	usb_dev->custom_mode[usb_dev->custom_mode_cnt].rate = rate;
	usb_dev->custom_mode[usb_dev->custom_mode_cnt].vic = vic;
	usb_dev->custom_mode_cnt++;

	return 0;
}

static void usb_hal_free_buf(struct usb_hal_dev *usb_dev)
{
	if (!usb_dev->usb_buf.buf)
	{
		return;
	}

	switch (usb_dev->usb_buf.type)
	{
	case USB_HAL_BUF_TYPE_USB:
		usb_free_coherent(usb_dev->udev, usb_dev->usb_buf.size, usb_dev->usb_buf.buf, usb_dev->usb_buf.dma_addr);
		break;
	case USB_HAL_BUF_TYPE_VMALLOC:
		sg_free_table(usb_dev->usb_buf.sgt);
		kfree(usb_dev->usb_buf.sgt);
		vfree(usb_dev->usb_buf.buf);
		usb_dev->usb_buf.sgt = NULL;
		break;
	default:
		break;
	}
	usb_dev->usb_buf.buf = NULL;
}

static int usb_dev_vmalloc_buf(struct usb_hal_dev *usb_dev)
{
	int ret = 0;
	int i;
	struct page **pages = NULL;
	unsigned int num_pages;
	struct usb_device *udev = usb_dev->udev;

	usb_dev->usb_buf.buf = vmalloc(USB_HAL_BUF_SIZE);
	if (!usb_dev->usb_buf.buf)
	{
		dev_err(&udev->dev, "vmalloc failed!\n");
		ret = -ENOMEM;
		goto fail;
	}

	memset(usb_dev->usb_buf.buf, 0, USB_HAL_BUF_SIZE);

	usb_dev->usb_buf.sgt = (struct sg_table *)kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!usb_dev->usb_buf.sgt)
	{
		dev_err(&udev->dev, "kmalloc sgt failed!\n");
		ret = -ENOMEM;
		goto fail;
	}

	num_pages = (USB_HAL_BUF_SIZE >> PAGE_SHIFT);
	pages = kmalloc(sizeof(struct page *) * num_pages, GFP_KERNEL);
	if (!pages)
	{
		dev_err(&udev->dev, "kmalloc pages failed!\n");
		ret = -ENOMEM;
		goto fail;
	}

	for (i = 0; i < num_pages; i++)
	{
		pages[i] = vmalloc_to_page(usb_dev->usb_buf.buf + i * PAGE_SIZE);
	}

	ret = sg_alloc_table_from_pages(usb_dev->usb_buf.sgt, pages, num_pages, 0, USB_HAL_BUF_SIZE, 0);
	if (!ret)
	{
		goto success;
	}

	dev_err(&udev->dev, "alloc table from pages failed!\n");

fail:
	usb_dev->usb_buf.size = 0;
	usb_dev->usb_buf.len = 0;
	if (usb_dev->usb_buf.buf)
	{
		vfree(usb_dev->usb_buf.buf);
		usb_dev->usb_buf.buf = NULL;
	}

	if (usb_dev->usb_buf.sgt)
	{
		kfree(usb_dev->usb_buf.sgt);
		usb_dev->usb_buf.sgt = NULL;
	}

success:
	if (pages)
	{
		kfree(pages);
	}

	return ret;
}

static int usb_dev_alloc_buf(struct usb_hal_dev *usb_dev)
{
	struct usb_device *udev = usb_dev->udev;
	int ret = 0;
	usb_dev->usb_buf.size = USB_HAL_BUF_SIZE;
	usb_dev->usb_buf.len = USB_HAL_BUF_DEF_LEN;
	usb_dev->usb_buf.buf = usb_alloc_coherent(udev, USB_HAL_BUF_SIZE, GFP_KERNEL,
						  &usb_dev->usb_buf.dma_addr);
	if (usb_dev->usb_buf.buf)
	{
		usb_dev->usb_buf.type = USB_HAL_BUF_TYPE_USB;
		dev_info(&udev->dev, "buf type usb\n");
		goto success;
	}

	ret = usb_dev_vmalloc_buf(usb_dev);
	if (!ret)
	{
		usb_dev->usb_buf.type = USB_HAL_BUF_TYPE_VMALLOC;
		dev_info(&udev->dev, "buf type vmalloc\n");
		goto success;
	}

	usb_dev->usb_buf.buf = NULL;
	usb_dev->usb_buf.size = 0;
	usb_dev->usb_buf.len = 0;
	ret = -ENOMEM;

success:
	return ret;
}

static struct device *usb_hal_intf_get_dma_device(struct usb_interface *intf)
{
#if KERNEL_VERSION(4, 12, 0) <= LINUX_VERSION_CODE
	struct usb_device *udev = interface_to_usbdev(intf);
	struct device *dmadev;

	if (!udev->bus)
		return NULL;

	dmadev = get_device(udev->bus->sysdev);
	if (!dmadev || !dmadev->dma_mask)
	{
		put_device(dmadev);
		return NULL;
	}

	return dmadev;
#else
	return NULL;
#endif
}

static int usb_dev_hal_init(struct usb_hal *usb_hal)
{
	int ret = 0;
	struct usb_hal_dev *usb_dev = (struct usb_hal_dev *)usb_hal->private;
	struct usb_device *udev = usb_dev->udev;

	ret = usb_dev->hal_dev->funcs->get_chip_id(udev, &usb_hal->chip_id);
	if (ret)
	{
		dev_err(&udev->dev, "get chip id failed! ret=%d\n", ret);
		return -ENOENT;
	}

	ret = usb_dev->hal_dev->funcs->get_port_type(udev, &usb_hal->port_type);
	if (ret)
	{
		dev_err(&udev->dev, "get video port type failed! ret=%d\n", ret);
		return -ENOENT;
	}

	ret = usb_dev->hal_dev->funcs->get_sdram_type(udev, &usb_hal->sdram_type);
	if (ret)
	{
		dev_err(&udev->dev, "get sdram type failed! ret=%d\n", ret);
		return -ENOENT;
	}

	if (VIDEO_PORT_CVBS_SVIDEO == usb_hal->port_type)
	{
		ret = usb_hal_add_custom_mode(usb_hal, 720, 480, 60, _VFMT_CEA_02_720x480P_60HZ);
		if (ret)
		{
			dev_err(&udev->dev, "add 720*480 failed! ret=%d\n", ret);
		}
	}

	return usb_dev->hal_dev->funcs->init_dev(udev, usb_hal->chip_id, usb_hal->port_type, usb_hal->sdram_type);
}

struct usb_hal *usb_hal_init(struct usb_interface *interface, const struct usb_device_id *id, struct kfifo *fifo, u32 index)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	struct usb_hal *usb_hal = NULL;
	struct usb_hal_dev *usb_dev = NULL;
	char name[32];
	int ret = 0;

	if (!interface || !id || !fifo)
	{
		printk("%s: null pointer!\n", __func__);
		return NULL;
	}

	usb_hal = kzalloc(sizeof(*usb_hal), GFP_KERNEL);
	if (!usb_hal)
	{
		dev_err(&udev->dev, "kzalloc usb_hal failed!\n");
		goto err;
	}

	usb_dev = kzalloc(sizeof(*usb_dev), GFP_KERNEL);
	if (!usb_dev)
	{
		dev_err(&udev->dev, "kzalloc usb_hal_dev failed!\n");
		goto err;
	}

	usb_dev->hal_dev = msdisp_hal_find_dev(id, udev);
	if (!usb_dev->hal_dev)
	{
		dev_err(&udev->dev, "Can't find hal dev! vid=0x%x pid=0x%x\n", id->idVendor, id->idProduct);
		goto err;
	}

	usb_hal->private = usb_dev;
	usb_hal->interface = interface;

	usb_dev->udev = udev;
	usb_dev->hal = usb_hal;
	usb_dev->fifo = fifo;
	usb_dev->index = index;
	usb_dev->vpack_out = USB_HAL_COLOR_FORMAT_YUV422;
	usb_dev->trans_mode = USH_HAL_TRANS_MODE_FRAME;
	usb_dev->state = USB_HAL_DEV_STATE_UNKNOWN;

	usb_dev->dma_dev = usb_hal_intf_get_dma_device(interface);
	if (!usb_dev->dma_dev)
		dev_warn(&udev->dev, "buffer sharing not supported"); /* not an error */

	ret = usb_dev_hal_init(usb_hal);
	if (ret)
	{
		dev_err(&udev->dev, "usb dev hal init failed! ret=%d\n", ret);
		goto err;
	}
	else
	{
		dev_info(&udev->dev, "chip id:0x%x port:0x%x sdram:0x%x\n", usb_hal->chip_id, usb_hal->port_type, usb_hal->sdram_type);
	}

	ret = usb_dev_alloc_buf(usb_dev);
	if (ret)
	{
		dev_err(&udev->dev, "alloc buf failed!\n");
		goto err;
	}

	usb_dev->state = USB_HAL_DEV_STATE_UNKNOWN;
	usb_dev->bus_status = MS9132_USB_BUS_STATUS_NORMAL;

	mutex_init(&usb_dev->usb_buf.mutex);
	sema_init(&usb_dev->sema, 1);

	memset(name, 0, 32);
	snprintf(name, 32, "msdisp%d_send", index);
	usb_dev->thread_run_flag = 1;
	usb_dev->thread = kthread_run(usb_hal_state_machine_entry, usb_hal, name);

	usb_hal_sysfs_init(interface);
	goto out;

err:
	if (usb_dev && usb_dev->dma_dev)
	{
		put_device(usb_dev->dma_dev);
	}

	if (usb_dev)
	{
		kfree(usb_dev);
		usb_dev = NULL;
	}

	if (usb_hal)
	{
		kfree(usb_hal);
		usb_hal = NULL;
	}

out:
	printk("init usb_hal%d %s!\n", index, usb_hal ? "success" : "failed");
	return usb_hal;
}

void usb_hal_destroy(struct usb_hal *hal)
{
	struct usb_hal_dev *usb_dev = (struct usb_hal_dev *)hal->private;
	struct usb_interface *interface = hal->interface;
	int index = usb_dev->index;

	if (usb_dev->thread)
	{
		usb_hal_stop_thread(usb_dev);
		msleep(300);
		usb_dev->thread = NULL;
	}

	usb_hal_sysfs_exit(interface);
	usb_hal_free_buf(usb_dev);
	if (usb_dev->dma_dev)
	{
		put_device(usb_dev->dma_dev);
	}
	kfree(usb_dev);
	kfree(hal);

	printk("dstroy usb_hal%d success!\n", index);
}

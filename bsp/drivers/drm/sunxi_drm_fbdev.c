/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <drm/drm.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_probe_helper.h>
#include <linux/dma-buf.h>
#include <drm/drm_print.h>
#include <linux/version.h>

#include "sunxi_drm_drv.h"

#define PREFERRED_BPP		32

static int sunxi_drm_fbdev_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct drm_fb_helper *fb_helper = info->par;

	if (fb_helper->dev->driver->gem_prime_mmap)
		return fb_helper->dev->driver->gem_prime_mmap(fb_helper->buffer->gem, vma);
	else
		return -ENODEV;
}

static const struct fb_ops sunxi_drm_fbdev_ops = {
	.owner		= THIS_MODULE,
	DRM_FB_HELPER_DEFAULT_OPS,
	.fb_mmap	= sunxi_drm_fbdev_fb_mmap,
	.fb_fillrect	= drm_fb_helper_cfb_fillrect,
	.fb_copyarea	= drm_fb_helper_cfb_copyarea,
	.fb_imageblit	= drm_fb_helper_cfb_imageblit,
};

#if IS_ENABLED(CONFIG_AW_DRM_FBDEV_BOOTLOGO)
static void *fb_map_kernel_cache(unsigned long phys_addr, unsigned long size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr = NULL;
	int i;

	if (!pages)
		return NULL;

	for (i = 0, tmp = pages; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = PAGE_KERNEL;
	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	vfree(pages);
	return vaddr;
}

void fb_unmap_kernel(void *vaddr)
{
	vunmap(vaddr);
}

int sunxi_bootlogo_copy(struct drm_fb_helper *fb_helper)
{
	struct sunxi_drm_private *pri = to_sunxi_drm_private(fb_helper->dev);
	unsigned int src_phy_addr = pri->logo.phy_addr, src_width = pri->logo.width;
	unsigned int src_height = pri->logo.height, src_bpp = pri->logo.bpp;
	unsigned int src_stride = pri->logo.stride, src_crop_l = pri->logo.crop_l;
	unsigned int src_crop_t = pri->logo.crop_t, src_crop_r = pri->logo.crop_r;
	unsigned int src_crop_b = pri->logo.crop_b;
	unsigned long map_offset;
	char *dst_addr = (char *)fb_helper->fbdev->screen_buffer;
	unsigned int dst_width, dst_height, dst_bpp, dst_stride, src_cp_btyes;
	char *src_addr_b, *src_addr_e, *src_addr;

	dst_width = fb_helper->fbdev->var.xres;
	dst_height = fb_helper->fbdev->var.yres;
	dst_bpp = fb_helper->fbdev->var.bits_per_pixel;
	dst_stride = fb_helper->fbdev->fix.line_length;

	map_offset = (unsigned long)src_phy_addr + PAGE_SIZE
	    - PAGE_ALIGN((unsigned long)src_phy_addr + 1);
	src_addr = (char *)fb_map_kernel_cache((unsigned long)src_phy_addr -
					       map_offset,
					       src_stride * src_height +
					       map_offset);
	if (src_addr == NULL) {
		DRM_ERROR("fb_map_kernel_cache for src_addr failed\n");
		return -1;
	}

	/* only copy the crop area */
	src_addr_b = src_addr + map_offset;
	if ((src_crop_b > src_crop_t) &&
	    (src_height > src_crop_b - src_crop_t) &&
	    (src_crop_t >= 0) &&
	    (src_height >= src_crop_b)) {
		src_height = src_crop_b - src_crop_t;
		src_addr_b += (src_stride * src_crop_t);
	}
	if ((src_crop_r > src_crop_l)
	    && (src_width > src_crop_r - src_crop_l)
	    && (src_crop_l >= 0)
	    && (src_width >= src_crop_r)) {
		src_width = src_crop_r - src_crop_l;
		src_addr_b += (src_crop_l * src_bpp >> 3);
	}

	/* logo will be placed in the middle */
	if (src_height < dst_height) {
		int dst_crop_t = (dst_height - src_height) >> 1;

		dst_addr += (dst_stride * dst_crop_t);
	} else if (src_height > dst_height) {
		DRM_ERROR("logo src_height(%u) > dst_height(%u),please cut the height\n",
		      src_height,
		      dst_height);
		goto out_unmap;
	}
	if (src_width < dst_width) {
		int dst_crop_l = (dst_width - src_width) >> 1;

		dst_addr += (dst_crop_l * dst_bpp >> 3);
	} else if (src_width > dst_width) {
		DRM_ERROR("src_width(%u) > dst_width(%u),please cut the width\n",
		      src_width,
		      dst_width);
		goto out_unmap;
	}

	src_cp_btyes = src_width * src_bpp >> 3;
	src_addr_e = src_addr_b + src_stride * src_height;
	for (; src_addr_b != src_addr_e; src_addr_b += src_stride) {
		memcpy((void *)dst_addr, (void *)src_addr_b, src_cp_btyes);
		dst_addr += dst_stride;
	}

out_unmap:
	fb_unmap_kernel(src_addr);

	return 0;
}
#endif

static int sunxi_drm_fbdev_create(struct drm_fb_helper *fb_helper,
				       struct drm_fb_helper_surface_size *sizes)
{
	struct drm_client_dev *client = &fb_helper->client;
	struct drm_client_buffer *buffer;
	struct drm_framebuffer *fb;
	struct fb_info *fbi;
	u32 format;
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0)
	struct dma_buf_map map;
	int ret;
#else
	void *vaddr;
#endif
	//printk("=====csy: %d, %s\n", __LINE__, __func__);

	format = drm_mode_legacy_fb_format(sizes->surface_bpp, sizes->surface_depth);
	buffer = drm_client_framebuffer_create(client, sizes->surface_width,
					       sizes->surface_height, format);
	if (IS_ERR(buffer))
		return PTR_ERR(buffer);

	DRM_INFO("fbdev width(%d), height(%d) and bpp(%d) fmt(%d)\n",
		    sizes->surface_width, sizes->surface_height,
		    sizes->surface_bpp, format);

	fb_helper->buffer = buffer;
	fb_helper->fb = buffer->fb;
	fb = buffer->fb;

	fbi = drm_fb_helper_alloc_fbi(fb_helper);
	if (IS_ERR(fbi))
		return PTR_ERR(fbi);

	fbi->fbops = &sunxi_drm_fbdev_ops;
	fbi->screen_size = fb->height * fb->pitches[0];
	fbi->fix.smem_len = fbi->screen_size;

	drm_fb_helper_fill_info(fbi, fb_helper, sizes);

	/* buffer is mapped for HW framebuffer */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = drm_client_buffer_vmap(fb_helper->buffer, &map);
	if (ret)
		return ret;
	if (map.is_iomem) {
		fbi->screen_base = map.vaddr_iomem;
	} else {
		fbi->screen_buffer = map.vaddr;
	fbi->flags |= FBINFO_VIRTFB;
	}
#else
	vaddr = drm_client_buffer_vmap(fb_helper->buffer);
	if (IS_ERR(vaddr))
		return PTR_ERR(vaddr);

	fbi->screen_buffer = vaddr;
#endif
	/* Shamelessly leak the physical address to user-space */
#if IS_ENABLED(CONFIG_DRM_FBDEV_LEAK_PHYS_SMEM)
	if (drm_leak_fbdev_smem && fbi->fix.smem_start == 0)
		fbi->fix.smem_start =
			page_to_phys(virt_to_page(fbi->screen_buffer));
#endif

#if IS_ENABLED(CONFIG_AW_DRM_FBDEV_BOOTLOGO)
	sunxi_bootlogo_copy(fb_helper);
#endif

	return 0;
}

static const struct drm_fb_helper_funcs sunxi_drm_fb_helper_funcs = {
	.fb_probe = sunxi_drm_fbdev_create,
};

int sunxi_drm_fbdev_init(struct drm_device *dev)
{
	struct drm_fb_helper *helper;
	int ret;
	//printk("=====csy: %d, %s\n", __LINE__, __func__);
	helper = devm_kzalloc(dev->dev, sizeof(*helper), GFP_KERNEL);

	drm_fb_helper_prepare(dev, helper, &sunxi_drm_fb_helper_funcs);

	ret = drm_fb_helper_init(dev, helper);
	if (ret < 0) {
		DRM_DEV_ERROR(dev->dev,
			      "Failed to initialize drm fb helper - %d.\n",
			      ret);
		return ret;
	}

	//printk("=====csy: %d, %s\n", __LINE__, __func__);
	ret = drm_fb_helper_initial_config(helper, PREFERRED_BPP);
	if (ret < 0) {
		DRM_DEV_ERROR(dev->dev,
			      "Failed to set initial hw config - %d.\n",
			      ret);
		goto err_drm_fb_helper_fini;
	}
	//printk("=====csy: %d, %s\n", __LINE__, __func__);
	return 0;

err_drm_fb_helper_fini:
	//printk("=====csy: %d, %s\n", __LINE__, __func__);
	drm_fb_helper_fini(helper);
	return ret;
}

void sunxi_drm_fbdev_fini(struct drm_device *dev)
{
}

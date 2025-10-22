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
#include "de/disp_manager.h"
#include "fb_g2d_rot.h"
#include <linux/memblock.h>

int platform_format_get_bpp(enum disp_pixel_format format);
struct disp_dma_mem {
	char *virtual_addr;
	unsigned long long device_addr;
};

struct fb_hw_info {
	struct fb_init_info_each fb_info;
	int size;
	wait_queue_head_t *wait;
#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	struct disp_ion_mem *ion_mem;
#else
	struct disp_dma_mem dma_mem;
#endif
	int fb_id;
	void *fb_rot;
	struct mutex lock;
	/* protect by lock */
	struct disp_layer_config config;
	u32 pseudo_palette[16];
};

struct fb_vysnc_wait{
	wait_queue_head_t wait;
	atomic_t wait_count;
	bool init;
};

struct fb_hw_private {
	struct fb_info_share fb_share;
	struct fb_vysnc_wait *fb_wait;
	unsigned int num_screens;
	struct work_struct free_wq;
	/* only used to resize output */
	struct fb_hw_info **fbs;
};

struct fb_hw_private hw_private;

int platform_swap_rb_order(struct fb_info *info, bool swap)
{
	struct fb_hw_info *hw = NULL;
	int ret = -1;

	if (info) {
		hw = (struct fb_hw_info *)info->par;
		if (hw) {
			if (swap) {
				if (info->var.bits_per_pixel == 24) {
					hw->config.info.fb.format = DISP_FORMAT_BGR_888;
				} else if (info->var.bits_per_pixel == 32) {
					hw->config.info.fb.format = DISP_FORMAT_ABGR_8888;
				}
				ret = 0;
			} else {
				if (info->var.bits_per_pixel == 24) {
					hw->config.info.fb.format = DISP_FORMAT_RGB_888;
				} else if (info->var.bits_per_pixel == 32) {
					hw->config.info.fb.format = DISP_FORMAT_ARGB_8888;
				}
				ret = 0;
			}
		}
	}
	return ret;
}

static void fb_free_reserve_mem(struct work_struct *work)
{
	int ret;
	int hw_id;
	int fb_stride;
	int fb_height;
	unsigned long phy_addr;
	int count;
	int i;
	struct disp_manager *mgr;

	for (i = 0; i < hw_private.num_screens; i++) {
		if (!hw_private.fb_share.smooth_display[i])
			break;
		hw_id = hw_private.fb_share.smooth_display_hw_id[i];
		mgr = disp_get_layer_manager(hw_id);
		fb_stride = hw_private.fb_share.logo[i].logo_fb.stride;
		fb_height = hw_private.fb_share.logo[i].logo_fb.height;
		phy_addr = (unsigned long)hw_private.fb_share.logo[i].logo_fb.phy_addr;

		do {
				ret = wait_event_interruptible_timeout(hw_private.fb_wait[hw_id].wait,
							mgr->is_fb_replaced(mgr),
							msecs_to_jiffies(3000));
		} while (ret <= 0);

		count = atomic_read(&hw_private.fb_wait[hw_id].wait_count);
		do {
			ret = wait_event_interruptible_timeout(hw_private.fb_wait[hw_id].wait,
					count != atomic_read(&hw_private.fb_wait[hw_id].wait_count),
					msecs_to_jiffies(1000));
			/* if (ret <= 0) */
			/*         DE_WARN(" wait for sync timeout\n"); */
		} while (ret <= 0);

		/* FIXME free logo file reserve */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0) && !defined(MODULE)
		memblock_free(__va(phy_addr), fb_stride * fb_height);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
		memblock_free(phy_addr, fb_stride * fb_height);
#endif

#ifndef MODULE
		free_reserved_area(__va(phy_addr),
				__va(phy_addr + PAGE_ALIGN(fb_stride * fb_height)),
				0x100, "logo buffer");
#endif
	}
	fb_debug_inf("%s finish\n", __FUNCTION__);
}

static int set_layer_config(int disp, struct disp_layer_config *config)
{
	struct disp_layer_config tmp;
	struct disp_manager *mgr = disp_get_layer_manager(disp);
	if (mgr && mgr->set_layer_config) {
		memcpy(&tmp, config, sizeof(tmp));
		return mgr->set_layer_config(mgr, &tmp, 1);
	}
	return -1;
}

static void fb_vsync_process(u32 disp)
{
	if (!hw_private.fb_wait[disp].init)
		return;
	atomic_add(1, &hw_private.fb_wait[disp].wait_count);
	wake_up_interruptible_all(&hw_private.fb_wait[disp].wait);
}

static void get_fb_output_size(int hw_id, unsigned int *width, unsigned int *height)
{
	int i;
	for (i = 0; i < hw_private.num_screens; i++) {
		if (hw_private.fb_share.smooth_display[i] &&
				hw_id == hw_private.fb_share.smooth_display_hw_id[i]) {
			bsp_disp_get_display_size(hw_id, width, height);
			return;
		}
	}

	return platform_get_fb_buffer_size_from_config(hw_id, hw_private.fb_share.output_type[hw_id],
							hw_private.fb_share.output_mode[hw_id],
							(int *)width, (int *)height);
}

static int fb_layer_config_init(struct fb_hw_info *info,  const struct fb_map *map)
{
	unsigned int hw_output_width = 0;
	unsigned int hw_output_height = 0;

	memset(&info->config, 0, sizeof(info->config));
	info->config.enable = 1;
	info->config.info.mode = LAYER_MODE_BUFFER;
	info->config.info.zorder = 16;
	info->config.info.alpha_mode = 0;
	info->config.info.alpha_value = 0xff;

	get_fb_output_size(map->hw_display, &hw_output_width, &hw_output_height);
	info->config.info.screen_win.x = 0;
	info->config.info.screen_win.y = 0;
	info->config.info.screen_win.width = hw_output_width;
	info->config.info.screen_win.height = hw_output_height;

	info->config.info.fb.crop.x = (0LL) << 32;
	info->config.info.fb.crop.y = (0LL) << 32;
	info->config.info.fb.crop.width = ((long long)info->fb_info.width) << 32;
	info->config.info.fb.crop.height = ((long long)info->fb_info.height) << 32;
	info->config.info.fb.format = hw_private.fb_share.format;
	info->config.info.fb.addr[1] = 0;
	info->config.info.fb.addr[2] = 0;
	info->config.info.fb.flags = DISP_BF_NORMAL;
	info->config.info.fb.scan = DISP_SCAN_PROGRESSIVE;
	info->config.info.fb.size[0].width = info->fb_info.width;
	info->config.info.fb.size[0].height = info->fb_info.height;
	info->config.info.fb.size[1].width = 0;
	info->config.info.fb.size[1].height = 0;
	info->config.info.fb.size[2].width = 0;
	info->config.info.fb.size[2].height = 0;
	info->config.info.fb.color_space = DISP_BT601;

	info->config.channel = map->hw_channel;
	info->config.layer_id = map->hw_layer;
	info->config.info.zorder = map->hw_zorder;
	return 0;
}

void platform_get_fb_buffer_size_from_config(int hw_id, enum disp_output_type output_type,
						unsigned int output_mode, int *width, int *height)
{
	*width = bsp_disp_get_screen_width_from_output_type(hw_id,
							      output_type,
							      output_mode);
	*height = bsp_disp_get_screen_height_from_output_type(hw_id,
							       output_type,
							       output_mode);
}

int platform_fb_adjust_output_size(int disp, enum disp_output_type output_type, unsigned int output_mode)
{
	int width, height, i, ret = -1;
	struct disp_layer_config config_get;
	struct disp_manager *mgr;

	memset(&config_get, 0, sizeof(config_get));
	platform_get_fb_buffer_size_from_config(disp, output_type, output_mode, &width, &height);
	mgr = disp_get_layer_manager(disp);

	for (i = 0; i < hw_private.fb_share.fb_num; i++) {
		if (hw_private.fbs[i]->fb_info.map.hw_display != disp)
			continue;

		mutex_lock(&hw_private.fbs[i]->lock);
		hw_private.fbs[i]->config.info.screen_win.width = width;
		hw_private.fbs[i]->config.info.screen_win.height = height;
		mutex_unlock(&hw_private.fbs[i]->lock);

		config_get.channel = hw_private.fbs[i]->config.channel;
		config_get.layer_id = hw_private.fbs[i]->config.layer_id;
		if (mgr && mgr->set_layer_config)
			ret = mgr->get_layer_config(mgr, &config_get, 1);

		if (!ret && config_get.enable &&
			config_get.info.fb.addr[0] ==
				hw_private.fbs[i]->config.info.fb.addr[0]) {
			set_layer_config(disp, &hw_private.fbs[i]->config);
		}
	}

	return ret;
}

int platform_get_private_size(void)
{
	return sizeof(struct fb_hw_info);
}

int platform_format_get_bpp(enum disp_pixel_format format)
{
	int bpp;
	switch (format) {
	case DISP_FORMAT_RGB_888:
		bpp = 24;
		break;
	default:
	case DISP_FORMAT_ARGB_8888:
		bpp = 32;
		break;
	}
	return bpp;
}

int platform_format_to_var(enum disp_pixel_format format, struct fb_var_screeninfo *var)
{
	switch (format) {
	case DISP_FORMAT_RGB_888:
		var->bits_per_pixel = 24;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->blue.offset = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;
		break;
	default:
	case DISP_FORMAT_ARGB_8888:
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

int platform_get_timing(void *hw_info, struct fb_var_screeninfo *var)
{
	int ret = -1;
	struct disp_video_timings tt;
	struct fb_hw_info *info = hw_info;
	struct disp_manager *mgr = disp_get_layer_manager(info->fb_info.map.hw_display);
	if (mgr && mgr->device && mgr->device->get_timings) {
		ret = mgr->device->get_timings(mgr->device, &tt);
		var->pixclock = tt.pixel_clk ? 1000000000 / tt.pixel_clk : 0;
		var->left_margin = tt.hor_back_porch;
		var->right_margin = tt.hor_front_porch;
		var->upper_margin = tt.ver_back_porch;
		var->lower_margin = tt.ver_front_porch;
		var->hsync_len = tt.hor_sync_time;
		var->vsync_len = tt.ver_sync_time;
	}

	return ret;
}

int platform_get_physical_size(void *hw_info, struct fb_var_screeninfo *var)
{
	struct fb_hw_info *info = hw_info;
	var->width = bsp_disp_get_screen_physical_width(info->fb_info.map.hw_display);
	var->height = bsp_disp_get_screen_physical_height(info->fb_info.map.hw_display);

	return 0;
}

int platform_update_fb_output(void *hw_info, const struct fb_var_screeninfo *var)
{
	struct fb_hw_info *info = hw_info;
	struct disp_manager *mgr;
	mutex_lock(&info->lock);
	info->config.info.fb.crop.y = ((long long)var->yoffset) << 32;
#if IS_ENABLED(CONFIG_AW_DISP2_FB_HW_ROTATION_SUPPORT)
	if (info->fb_rot)
		fb_g2d_rot_apply(info->fb_rot, &info->config, var->yoffset);
#endif
	mutex_unlock(&info->lock);

	mgr = disp_get_layer_manager(info->fb_info.map.hw_display);
	set_layer_config(info->fb_info.map.hw_display, &info->config);
	if (var->reserved[0] == FB_ACTIVATE_FORCE) {
		if (mgr)
			mgr->sync(mgr, true);
	}
	return 0;
}

int platform_fb_mmap(void *hw_info, struct vm_area_struct *vma)
{
	struct fb_hw_info *info = hw_info;
	unsigned int off = vma->vm_pgoff << PAGE_SHIFT;

	if (off < info->size) {
#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
		return info->ion_mem->p_item->dmabuf->ops->mmap(
				info->ion_mem->p_item->dmabuf, vma);

#else
		return dma_mmap_wc(hw_private.fb_share.disp_dev, vma,
					info->dma_mem.virtual_addr,
					info->dma_mem.device_addr,
					info->size);
#endif
	}
	return -EINVAL;
}

struct dma_buf *platform_fb_get_dmabuf(void *hw_info)
{
#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	struct fb_hw_info *info = hw_info;
	get_dma_buf(info->ion_mem->p_item->dmabuf);
	return info->ion_mem->p_item->dmabuf;
#endif
	return NULL;
}

void *fb_map_kernel(unsigned long phys_addr, unsigned long size)
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

	pgprot = pgprot_noncached(PAGE_KERNEL);
	vaddr = vmap(pages, npages, VM_MAP, pgprot);

	vfree(pages);
	return vaddr;
}

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

void Fb_unmap_kernel(void *vaddr)
{
	vunmap(vaddr);
}

static int logo_display_file(struct fb_hw_info *info, unsigned long phy_addr)
{
/* TODO */
	return 0;
}

int platform_fb_set_blank(void *hw_info, bool is_blank)
{
	struct fb_hw_info *info = hw_info;
	mutex_lock(&info->lock);
	info->config.enable = is_blank ? 0 : 1;
	mutex_unlock(&info->lock);
	set_layer_config(info->fb_info.map.hw_display, &info->config);
	return platform_fb_pan_display_post_proc(info);
}

int logo_display_fb_no_rot(struct fb_hw_info *info, struct logo_info_fb *logo)
{
	unsigned long map_offset;
	char *dst_addr;
	int dst_width;
	int dst_height;
	int dst_bpp;
	int dst_stride;
	char *src_addr_b;
	char *src_addr_e;
	int src_cp_btyes;
	char *src_addr;

	unsigned long src_phy_addr = logo->phy_addr;
	int src_stride = logo->stride;
	int src_height = logo->height;
	int src_width = logo->width;
	int src_crop_l = logo->crop_l;
	int src_crop_t = logo->crop_t;
	int src_crop_r = logo->crop_r;
	int src_crop_b = logo->crop_b;
	int src_bpp = logo->bpp;

#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	dst_addr = (char *)(info->ion_mem->vaddr);
#else
	dst_addr = (char *)(info->dma_mem.virtual_addr);
#endif
	dst_width = info->fb_info.width;
	dst_height = info->fb_info.height;
	dst_bpp = platform_format_get_bpp(hw_private.fb_share.format);
	dst_stride = info->fb_info.width * dst_bpp / 8;

	map_offset = (unsigned long)src_phy_addr + PAGE_SIZE
	    - PAGE_ALIGN((unsigned long)src_phy_addr + 1);
	src_addr = (char *)fb_map_kernel_cache((unsigned long)src_phy_addr -
					       map_offset,
					       src_stride * src_height +
					       map_offset);
	if (src_addr == NULL) {
		DE_WARN("fb_map_kernel_cache for src_addr failed\n");
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
		DE_WARN("logo src_height(%d) > dst_height(%d),please cut the height\n",
		      src_height,
		      dst_height);
		goto out_unmap;
	}
	if (src_width < dst_width) {
		int dst_crop_l = (dst_width - src_width) >> 1;

		dst_addr += (dst_crop_l * dst_bpp >> 3);
	} else if (src_width > dst_width) {
		DE_WARN("src_width(%d) > dst_width(%d),please cut the width\n",
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
	Fb_unmap_kernel(src_addr);

	return 0;
}

#if IS_ENABLED(CONFIG_AW_DISP2_FB_HW_ROTATION_SUPPORT)
static int logo_display_fb_rot(struct fb_hw_info *info, struct logo_info_fb *logo, int degree_int)
{
	int i, j, ret = -1;
	int bpp;
	unsigned long map_offset;
	char *dst_addr;
	int dst_width;
	int dst_height;
	int dst_bpp;
	int dst_stride;
	char *src_addr;
	char *map_addr;

	char *tmp_src;
	char *tmp_dst;

	unsigned long src_phy_addr = logo->phy_addr;
	int src_stride = logo->stride;
	int src_height = logo->height;
	int src_crop_l = logo->crop_l;
	int src_crop_t = logo->crop_t;
	int src_crop_r = logo->crop_r;
	int src_crop_b = logo->crop_b;
	int src_bpp = logo->bpp;
	int src_crop_w = src_crop_r - src_crop_l;
	int src_crop_h = src_crop_b - src_crop_t;

	dst_bpp = platform_format_get_bpp(hw_private.fb_share.format);
	dst_height = info->fb_info.height;
	dst_width = info->fb_info.width;
	dst_stride = info->fb_info.width * dst_bpp / 8;
#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	dst_addr = (char *)(info->ion_mem->vaddr);
#else
	dst_addr = (char *)(info->dma_mem.virtual_addr);
#endif

	if (dst_bpp != src_bpp) {
		DE_WARN("%s: dst_bpp != src_bp\n", __FUNCTION__);
		return ret;
	}
	bpp = dst_bpp;
	map_offset = (unsigned long)src_phy_addr + PAGE_SIZE
	    - PAGE_ALIGN((unsigned long)src_phy_addr + 1);
	src_addr = (char *)fb_map_kernel_cache((unsigned long)src_phy_addr -
					       map_offset,
					       src_stride * src_height +
					       map_offset);
	if (src_addr == NULL) {
		DE_WARN("fb_map_kernel_cache for src_addr failed\n");
		return ret;
	}

	map_addr = src_addr;
	src_addr += src_stride * src_crop_t;
	src_addr += bpp / 8 * src_crop_l;

	if (degree_int == 180) { /* copy with rotate 180 */
		dst_addr += dst_stride * (dst_height - src_crop_t);
		dst_addr += bpp / 8 * (src_crop_w + src_crop_l);

		for (j = 0; j < src_crop_h; j++) {
			for (i = 0, tmp_dst = dst_addr, tmp_src = src_addr; i < src_crop_w; i++) {
				memcpy(dst_addr, src_addr, bpp / 8);
				src_addr += bpp / 8;
				dst_addr -= bpp / 8;
			}
			dst_addr = tmp_dst - src_stride;
			src_addr = tmp_src + src_stride;
		}
	} else if (degree_int == 270) { /* copy with rotate 90 */
		dst_addr += dst_stride * src_crop_l;
		dst_addr += bpp / 8 * (src_crop_t + src_crop_h);

		for (j = 0; j < src_crop_h; j++) {
			for (i = 0, tmp_dst = dst_addr, tmp_src = src_addr; i < src_crop_w; i++) {
				memcpy(dst_addr, src_addr, bpp / 8);
				src_addr += bpp / 8;
				dst_addr += dst_stride;
			}
			dst_addr = tmp_dst - bpp / 8;
			src_addr = tmp_src + src_stride;
		}
	} else if (degree_int == 90) { /* copy with rotate 270 */
		dst_addr += dst_stride * (src_crop_l + src_crop_w);
		dst_addr += bpp / 8 * src_crop_t;

		for (j = 0; j < src_crop_h; j++) {
			for (i = 0, tmp_dst = dst_addr, tmp_src = src_addr; i < src_crop_w; i++) {
				memcpy(dst_addr, src_addr, bpp / 8);
				src_addr += bpp / 8;
				dst_addr -= dst_stride;
			}
			dst_addr = tmp_dst + bpp / 8;
			src_addr = tmp_src + src_stride;
		}
	} else {
		DE_WARN("input rotate degree error %d\n", degree_int);
	}
	Fb_unmap_kernel(map_addr);
	return 0;
}
#endif

static int logo_display_fb(struct fb_hw_info *hw_info, struct logo_info_fb *logo)
{
	struct fb_hw_info *info = hw_info;
	int ret;
#if IS_ENABLED(CONFIG_AW_DISP2_FB_HW_ROTATION_SUPPORT)
	if (info->fb_rot) {
		ret = fb_g2d_degree_to_int_degree(hw_info->fb_info.rot_degree);
		ret = logo_display_fb_rot(info, logo, ret);
	} else {
		ret = logo_display_fb_no_rot(info, logo);
	}
#else
	ret = logo_display_fb_no_rot(info, logo);
#endif
	return ret;
}

bool platform_fb_check_rotate(__u32 *rot)
{
#if IS_ENABLED(CONFIG_AW_DISP2_FB_HW_ROTATION_SUPPORT)
	return fb_g2d_check_rotate(rot);
#endif
	return false;
}

int platform_fb_set_rotate(void *hw_info, int rot)
{
#if IS_ENABLED(CONFIG_AW_DISP2_FB_HW_ROTATION_SUPPORT)
	int new_degree;
	struct g2d_rot_create_info rot_create;
	struct fb_hw_info *info = hw_info;
	new_degree = fb_g2d_get_new_rot_degree(info->fb_info.rot_degree, rot);
	mutex_lock(&info->lock);
	 if (info->fb_rot == NULL) {
		rot_create.dst_width = info->fb_info.width;
		rot_create.dst_height = info->fb_info.height;
		rot_create.format = hw_private.fb_share.format;
		rot_create.rot_degree = new_degree;
		rot_create.phy_addr = info->config.info.fb.addr[0];
		rot_create.src_size = info->size;
		info->fb_rot = fb_g2d_rot_init(&rot_create, &info->config);
	} else {
		fb_g2d_set_degree(info->fb_rot, new_degree, &info->config);
	}
	mutex_unlock(&info->lock);
#endif
	return 0;
}

int platform_fb_init_logo(void *hw_info, const struct fb_var_screeninfo *var)
{
	struct fb_hw_info *info = hw_info;
	int disp = info->fb_info.map.hw_display;
	if (info->fb_info.logo_display) {
		fb_debug_inf("display logo on display %d channel %d layer %d type %d\n",
				disp, info->config.channel,
				info->config.layer_id,
				hw_private.fb_share.logo_type[disp]);

		if (hw_private.fb_share.logo_type[disp] == LOGO_FILE)
			logo_display_file(info, hw_private.fb_share.logo[disp].logo_file.phy_addr);
		else if (hw_private.fb_share.smooth_display[disp])
			logo_display_fb(info, &hw_private.fb_share.logo[disp].logo_fb);
	}

	platform_update_fb_output(hw_info, var);
	return 0;
}

int platform_fb_g2d_rot_init(void *hw_info)
{
#if IS_ENABLED(CONFIG_AW_DISP2_FB_HW_ROTATION_SUPPORT)
	struct fb_hw_info *info = hw_info;
	struct g2d_rot_create_info rot;
	if (info->fb_info.g2d_rot_used) {
		rot.dst_width = info->fb_info.width;
		rot.dst_height = info->fb_info.height;
		fb_g2d_get_rot_size(info->fb_info.rot_degree, &rot.dst_width, &rot.dst_height);
		rot.format = hw_private.fb_share.format;
		rot.rot_degree = info->fb_info.rot_degree;
		rot.phy_addr = info->config.info.fb.addr[0];
		rot.src_size = info->size;
		mutex_lock(&info->lock);
		info->fb_rot = fb_g2d_rot_init(&rot, &info->config);
		mutex_unlock(&info->lock);
	}
#endif
	return 0;
}

int platform_fb_memory_alloc(void *hw_info, char **vir_addr, unsigned long long *device_addr, unsigned int size)
{
	struct fb_hw_info *info = hw_info;
#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	info->ion_mem = disp_ion_malloc(size, device_addr);
	*vir_addr = info->ion_mem ? (char __iomem *)info->ion_mem->vaddr : NULL;
	if (!(*vir_addr))
		return -1;
	info->config.info.fb.addr[0] = (unsigned long long)info->ion_mem->p_item->dma_addr;
#else
	*vir_addr = disp_malloc(size, device_addr);
	if (!(*vir_addr))
		return -1;
	info->dma_mem.virtual_addr = *vir_addr;
	info->dma_mem.device_addr = *device_addr;
	info->config.info.fb.addr[0] = (unsigned long long)info->dma_mem.device_addr;
#endif
	info->size = size;

	return *vir_addr ? 0 : -1;
}

int platform_fb_memory_free(void *hw_info)
{
	struct fb_hw_info *info = hw_info;
#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	disp_ion_free((void *__force)info->ion_mem->vaddr,
		  (void *)info->ion_mem->p_item->dma_addr, info->size);
#else
	disp_free((void *__force)info->dma_mem.virtual_addr,
		  (void *)(unsigned long)info->dma_mem.device_addr, info->size);
#endif
	info->config.info.fb.addr[0] = 0;
	info->size = 0;
	return 0;
}

int platform_fb_pan_display_post_proc(void *hw_info)
{
	int count;
	int ret;
	struct fb_hw_info *info = hw_info;
	int hw_id = info->fb_info.map.hw_display;
	struct disp_manager *mgr = disp_get_layer_manager(hw_id);

	if (!mgr || !mgr->device ||
	    mgr->device->is_enabled == NULL ||
	    (mgr->device->is_enabled(mgr->device) == 0))
		return 0;

	count = atomic_read(&hw_private.fb_wait[hw_id].wait_count);
	ret = wait_event_interruptible_timeout(*info->wait,
					       count != atomic_read(&hw_private.fb_wait[hw_id].wait_count),
					       msecs_to_jiffies(50));
	fb_debug_inf("platform pan_display %d ret %d\n", hw_id, ret);
	return ret == 0 ? -ETIMEDOUT : 0;
}

static int disable_all_layer(void)
{
	int num_screens, num_layers, i, j, k;
	struct disp_manager *mgr = NULL;
	struct disp_layer_config *lyr_cfg = NULL;
	int max_layers = 16;
	int num_chn = 4;
	int num_lyr_per_chn = 4;

	num_screens = bsp_disp_feat_get_num_screens();
	for (i = 0; i < num_screens; ++i) {
		int num = bsp_disp_feat_get_num_layers(i);
		if (num > max_layers)
			max_layers = num;
	}
	lyr_cfg = kzalloc(sizeof(*lyr_cfg) * max_layers, GFP_KERNEL);

	for (i = 0; i < num_screens; ++i) {
		int tmp_num = 0;
		num_chn = bsp_disp_feat_get_num_channels(i);
		for (j = 0; j < num_chn; j++) {
			num_lyr_per_chn = bsp_disp_feat_get_num_layers_by_chn(i, j);
			for (k = 0; k < num_lyr_per_chn; k++) {
				if (tmp_num > max_layers - 1) {
					fb_debug_inf("out of size %d >= %d\n", tmp_num, max_layers);
					break;
				}
				lyr_cfg[tmp_num].enable = false;
				lyr_cfg[tmp_num].channel = j;
				lyr_cfg[tmp_num].layer_id = k;
				tmp_num++;
			}
		}
		num_layers = bsp_disp_feat_get_num_layers(i);
		mgr = disp_get_layer_manager(i);
		mgr->set_layer_config(mgr, lyr_cfg, num_layers);
	}

	kfree(lyr_cfg);
	return 0;
}

static bool is_smooth_display(struct fb_info_share fb_share)
{
	int i;
	for (i = 0; i < hw_private.num_screens; i++) {
		if (fb_share.smooth_display[i]) {
			return true;
		}
	}
	return false;
}

int platform_fb_init(struct fb_info_share *fb_init)
{
	int i;
	memcpy(&hw_private.fb_share, fb_init, sizeof(*fb_init));

	hw_private.num_screens = bsp_disp_feat_get_num_screens();
	hw_private.fb_wait = kzalloc(sizeof(*hw_private.fb_wait) * hw_private.num_screens, GFP_KERNEL);
	hw_private.fbs = kzalloc(sizeof(*hw_private.fbs)* fb_init->fb_num, GFP_KERNEL);
	for (i = 0; i < hw_private.num_screens; i++)
		atomic_set(&hw_private.fb_wait[i].wait_count, 0);
	disp_register_sync_finish_proc(fb_vsync_process);

	if (is_smooth_display(hw_private.fb_share)) {
		INIT_WORK(&hw_private.free_wq, fb_free_reserve_mem);
	} else {
		disable_all_layer();
	}
	return 0;
}

int platform_fb_init_for_each(void *hw_info, struct fb_init_info_each *fb, void **pseudo_palette, int fb_id)
{
	struct fb_hw_info *info = hw_info;
	int hw_id = fb->map.hw_display;

	memcpy(&info->fb_info, fb, sizeof(*fb));
	info->fb_id = fb_id;
	fb_layer_config_init(info, &fb->map);
	mutex_init(&info->lock);

	if (!hw_private.fb_wait[hw_id].init) {
		init_waitqueue_head(&hw_private.fb_wait[hw_id].wait);
		hw_private.fb_wait[hw_id].init = true;
	}
	info->wait = &hw_private.fb_wait[hw_id].wait;
	hw_private.fbs[fb_id] = hw_info;
	*pseudo_palette = hw_private.fbs[fb_id]->pseudo_palette;
	return 0;
}

int platform_fb_post_init(void)
{
	fb_debug_inf("%s start\n", __FUNCTION__);
	if (is_smooth_display(hw_private.fb_share))
		schedule_work(&hw_private.free_wq);

	return 0;
}

int platform_fb_g2d_rot_exit(void *hw_info)
{
#if IS_ENABLED(CONFIG_AW_DISP2_FB_HW_ROTATION_SUPPORT)
	struct fb_hw_info *info = hw_info;
	if (info->fb_rot) {
		fb_g2d_rot_exit(info->fb_rot);
	}
#endif
	return 0;
}

int disable_fb_output(struct fb_hw_info *info)
{
	int ret = 0;
	struct disp_manager *mgr;
	struct disp_layer_config config_get;
	int disp = info->fb_info.map.hw_display;

	memset(&config_get, 0, sizeof(config_get));
	mgr = disp_get_layer_manager(disp);

	config_get.channel = info->config.channel;
	config_get.layer_id = info->config.layer_id;
	if (mgr && mgr->set_layer_config)
		ret = mgr->get_layer_config(mgr, &config_get, 1);
	if (!ret && config_get.enable &&
		config_get.info.fb.addr[0] ==
			info->config.info.fb.addr[0]) {
			info->config.enable = 0;
		set_layer_config(disp, &info->config);
	}
	return ret;
}

int platform_fb_exit(void)
{
	int i;
	for (i = 0; i < hw_private.fb_share.fb_num; i++) {
		disable_fb_output(hw_private.fbs[i]);
	}
	disp_delay_ms(100);
	return 0;
}

int platform_fb_exit_for_each(void *hw_info)
{
	return 0;
}

int platform_fb_post_exit(void)
{
	if (is_smooth_display(hw_private.fb_share)) {
		cancel_work_sync(&hw_private.free_wq);
	}
	disp_unregister_sync_finish_proc(fb_vsync_process);
	memset(&hw_private, 0, sizeof(hw_private));
	fb_debug_inf("%s finshed\n", __FUNCTION__);
	return 0;
}

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
#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#else
#include <drm/drm_fb_dma_helper.h>
#include <drm/drm_gem_dma_helper.h>
#endif
#include <drm/drm_vblank.h>
#include <drm/drm_client.h>
#include <linux/memblock.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 1, 0)
#include <drm/drm_blend.h>
#endif
#include "sunxi_fbdev.h"
#include "sunxi_drm_crtc.h"
#include <drm/drm_fourcc.h>

struct fb_hw_info {
	struct fb_create_info create_info;
	unsigned int size;
	struct drm_client_dev client;
	struct work_struct free_wq;
	struct drm_client_buffer *buffer;
	struct display_channel_state state;
	u32 pseudo_palette[16];
};

static void fb_free_reserve_mem(struct work_struct *work)
{
	fb_debug_inf("%s finish\n", __FUNCTION__);
}

static int fb_layer_config_init(struct fb_hw_info *info, unsigned int w, unsigned int h)
{
	memset(&info->state, 0, sizeof(info->state));
	info->state.base.crtc_x = 0;//TODO add ADAPTIVE_STRETCH
	info->state.base.crtc_y = 0;
	info->state.base.crtc_w = w;
	info->state.base.crtc_h = h;
	info->state.base.src_x = (0LL) << 16;
	info->state.base.src_y = (0LL) << 16;
	info->state.base.src_w = ((long long)info->create_info.width) << 16;
	info->state.base.src_h = ((long long)info->create_info.height) << 16;
	info->state.base.alpha = 0xffff;
	info->state.base.pixel_blend_mode = DRM_MODE_BLEND_PIXEL_NONE;
	info->state.base.rotation = DRM_MODE_ROTATE_0;
	info->state.base.normalized_zpos = 0;/* force minimum zpos */
	info->state.base.visible = true;
	info->state.eotf = DE_EOTF_BT709;
	info->state.color_space = DE_COLOR_SPACE_BT709;
	info->state.color_range = DE_COLOR_RANGE_0_255;
	return 0;
}

int platform_get_private_size(void)
{
	return sizeof(struct fb_hw_info);
}

int platform_update_fb_output(struct fb_hw_info *hw_info, void *info)
{
	struct drm_device *drm = hw_info->create_info.drm;
	unsigned int de = hw_info->create_info.map.hw_display;
	unsigned int channel = hw_info->create_info.map.hw_channel;
	struct fbdev_config cfg;
#if IS_ENABLED (CONFIG_FB)
	struct fb_var_screeninfo *var = (struct fb_var_screeninfo *)info;
#else
	struct drm_fb_info *var = (struct drm_fb_info *)info;
#endif
	cfg.dev = drm;
	cfg.de_id = de;
	cfg.channel_id = channel;
	cfg.force = var->reserved[0] == FB_ACTIVATE_FORCE;
	cfg.fake_state = &hw_info->state;
	cfg.out_plane = &hw_info->state.base.plane;
	cfg.out_crtc = &hw_info->state.base.crtc;
	hw_info->state.base.src_y = ((long long)var->yoffset) << 16;

	/* TODO: if dual output, use two thread to call sunxi_fbdev_plane_update,
	 *	and block until two thread finish
	 */
	sunxi_fbdev_plane_update(&cfg);
	return 0;
}

int platform_fb_mmap(struct fb_hw_info *hw_info, struct vm_area_struct *vma)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	struct drm_device *drm = hw_info->create_info.drm;

	if (drm->driver->gem_prime_mmap)
		return drm->driver->gem_prime_mmap(hw_info->buffer->gem, vma);
	else
		return -ENODEV;
#else
	return drm_gem_prime_mmap(hw_info->buffer->gem, vma);
#endif
}

int platform_fb_get_dmabuf(struct fb_hw_info *hw_info, int *fd)
{
	struct fb_hw_info *info = hw_info;
	uint32_t handle;
	int ret = 0;

	if (!info->buffer && !info->buffer->gem) {
		DRM_ERROR("Failed get gem obj form fb\n");
		return -1;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	ret = drm_gem_handle_create(info->client.file, info->buffer->gem, &handle);
	if (ret < 0) {
		DRM_ERROR("Failed to create handle form gem obj\n");
		goto err_exit;
	}
#else
	if (!info->buffer->handle) {
		DRM_ERROR("Failed to get handle form buffer handle\n");
		goto err_exit;
	}
	handle = info->buffer->handle;
#endif
	ret = drm_gem_prime_handle_to_fd(info->client.dev, info->client.file,
					 handle, DRM_CLOEXEC, fd);

err_exit:
	return ret;
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

void *fb_map_kernel_cache(unsigned long phys_addr, unsigned long size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages /*= vmalloc(sizeof(struct page *) * npages)*/;
	struct page **tmp;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr = NULL;
	int i;

	pages = kvmalloc_array(npages, sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		DRM_ERROR("kvmalloc_array return failed ! out of memory ?\n");
		return NULL;
	}

	for (i = 0, tmp = pages; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = PAGE_KERNEL;
	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	kvfree(pages);
	return vaddr;
}

void Fb_unmap_kernel(void *vaddr)
{
	vunmap(vaddr);
}

int logo_display_file(struct fb_hw_info *info, unsigned long phy_addr)
{
	/* TODO */
	return 0;
}

int platform_fb_set_blank(struct fb_hw_info *hw_info, bool is_blank)
{
	/* TODO */
	return 0;
}

int platform_fb_init_finish(struct fb_hw_info *hw_info, void *info,
				struct display_channel_state *out_state)
{
	platform_update_fb_output(hw_info, info);
	schedule_work(&hw_info->free_wq);
	memcpy(out_state, &hw_info->state, sizeof(*out_state));
	return 0;
}

static int fb_fmt2_drm_fmt(enum fb_format fmt)
{
	if (fmt == ARGB8888)
		return DRM_FORMAT_ARGB8888;
	if (fmt == RGB888)
		return DRM_FORMAT_RGB888;
	WARN_ON(1);
	return DRM_FORMAT_ARGB8888;
}

int platform_fb_memory_alloc(struct fb_hw_info *hw_info, void **vir_addr, unsigned long *device_addr, unsigned int w, unsigned int h, int fmt)
{
	u64 addr;
	int size;
	void *tmp;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	int ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	struct iosys_map map;
#else
	struct dma_buf_map map;
#endif
#endif
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	struct drm_gem_cma_object *gem;
#else
	struct drm_gem_dma_object *gem;
#endif
	hw_info->buffer = drm_client_framebuffer_create(&hw_info->client, w, h, fb_fmt2_drm_fmt(fmt));
	if (IS_ERR_OR_NULL(hw_info->buffer))
		return PTR_ERR(hw_info->buffer);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	gem = drm_fb_cma_get_gem_obj(hw_info->buffer->fb, 0);
	if (gem) {
		addr = (u64)(gem->paddr) + hw_info->buffer->fb->offsets[0];
	}
#else
	gem = drm_fb_dma_get_gem_obj(hw_info->buffer->fb, 0);
	if (gem) {
		addr = (u64)(gem->dma_addr) + hw_info->buffer->fb->offsets[0];
	}
#endif
	if (!gem || !addr) {
		DRM_ERROR("Failed framebuffer alloc for fbdev fail\n");
		return -ENOMEM;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	*vir_addr = drm_client_buffer_vmap(hw_info->buffer);
#else
	ret = drm_client_buffer_vmap(hw_info->buffer, &map);
	if (ret) {
		DRM_ERROR("drm_client_buffer_vmap fail\n");
		return -ENOMEM;
	}
	*vir_addr = map.vaddr;
#endif
	hw_info->state.base.fb = hw_info->buffer->fb;
	*device_addr = (unsigned long)addr;

	if (hw_info->create_info.logo_offset &&
		  w == hw_info->create_info.width) {
		size = drm_format_info(fb_fmt2_drm_fmt(fmt))->depth / 8;/* be careful legacy deprecated api */
		size *= hw_info->create_info.width * hw_info->create_info.height;
		tmp = fb_map_kernel_cache(hw_info->create_info.logo_offset, size);
		if (tmp) {
			memcpy(*vir_addr, tmp, size);
			Fb_unmap_kernel(tmp);
		} else {
			DRM_ERROR("fb_map_kernel/vmap failed, skip logo copy!\n");
		}
	}
	return 0;
}

int platform_fb_memory_free(struct fb_hw_info *info)
{
	if (!info || !info->buffer)
		return 0;

	drm_client_buffer_vunmap(info->buffer);

	drm_client_framebuffer_delete(info->buffer);
	info->buffer = NULL;

	info->state.base.fb = NULL;

	return 0;
}

int platform_fb_pan_display_post_proc(struct fb_hw_info *info)
{
/* nothing to do becase we have wait finish in platform_update_fb_out */
/*
	int hw_id = info->create_info.map[0].hw_display;
	struct drm_device *drm = info->create_info.drm;
	drm_wait_one_vblank(drm, hw_id);
*/
	return 0;
}

static void drm_client_unregister(struct drm_device *dev, struct drm_client_dev *client_del)
{
	struct drm_client_dev *client, *tmp;

	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		return;

	mutex_lock(&dev->clientlist_mutex);
	list_for_each_entry_safe(client, tmp, &dev->clientlist, list) {
		if (client != client_del)
			continue;

		list_del(&client->list);
		if (client->funcs && client->funcs->unregister) {
			client->funcs->unregister(client);
		} else {
			drm_client_release(client);
		}
	}
	mutex_unlock(&dev->clientlist_mutex);
}

int platform_fb_init(struct fb_create_info *create, struct fb_hw_info *info, void **pseudo_palette)
{
	int ret;
	memcpy(&info->create_info, create, sizeof(*create));
	fb_layer_config_init(info, create->scn_width, create->scn_height);
	ret = drm_client_init(create->drm, &info->client, "sunxi_fbdev", NULL);
	if (ret) {
		DRM_ERROR("Failed to register client: %d\n", ret);
		return -ENOMEM;
	}
	drm_client_register(&info->client);
	INIT_WORK(&info->free_wq, fb_free_reserve_mem);
	*pseudo_palette = info->pseudo_palette;
	return 0;
}

int platform_fb_exit(struct fb_create_info *create, struct fb_hw_info *info)
{
	drm_client_unregister(create->drm, &info->client);
	return 0;
}

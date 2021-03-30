/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>

#include "sunxi_drm_drv.h"
#include "sunxi_drm_fb.h"
#include "sunxi_drm_gem.h"
#include "sunxi_drm_encoder.h"

static int sunxi_drm_framebuffer_create_handle(struct drm_framebuffer *fb,
					struct drm_file *file_priv,
					unsigned int *handle)
{
	struct sunxi_drm_framebuffer *sunxi_fb = to_sunxi_fb(fb);
	int i;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	for (i =0; i < MAX_FB_BUFFER; i++) {
		if (sunxi_fb->gem_obj[i] != NULL) {
			return drm_gem_handle_create(file_priv,
				sunxi_fb->gem_obj[i], handle);
		}
	}
	return -ENODEV;
}

static int sunxi_drm_framebuffer_dirty(struct drm_framebuffer *fb,
			struct drm_file *file_priv, unsigned flags,
			unsigned color, struct drm_clip_rect *clips,
			unsigned num_clips)
{
	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	/* TODO */

	return 0;
}

static void sunxi_drm_framebuffer_destroy(struct drm_framebuffer *fb)
{
	struct sunxi_drm_framebuffer *sunxi_fb = to_sunxi_fb(fb);
	unsigned int i;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	/* make sure that overlay data are updated before relesing fb. */

	drm_framebuffer_cleanup(fb);

	for (i = 0; i < MAX_FB_BUFFER; i++) {
		if (sunxi_fb->gem_obj[i] != NULL)
		drm_gem_object_unreference_unlocked(sunxi_fb->gem_obj[i]);
	}

	kfree(sunxi_fb);
	sunxi_fb = NULL;
}

static int sunxi_check_max_plane_num_format(__u32 pixel_format)
{
	int ret = 3;

	return ret;
}

static struct drm_framebuffer *sunxi_user_fb_create(struct drm_device *dev,
	struct drm_file *file_priv, struct drm_mode_fb_cmd2 *mode_cmd)
{
	struct drm_gem_object *obj[MAX_FB_BUFFER];
	struct drm_framebuffer *fb_obj;
	int i, ret, max_cnt, nr_gem;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);
	/* becareful for a buf but the yuv  in it,
	 * and you must set the y is 0,u is 1,v is 2,etc.
	 */
	for (i = 0, nr_gem = 0; i < MAX_FB_BUFFER; i++) {
		obj[i] = drm_gem_object_lookup(dev, file_priv,
			mode_cmd->handles[i]);
		if (!obj[i]) {
			DRM_DEBUG_KMS("failed to lookup gem object\n");
			continue;
		}

		nr_gem++;
		if (!sunxi_check_gem_memory_type(obj[i], SUNXI_BO_CONTIG)) {
			DRM_ERROR("cannot use this gem memory type for fb.\n");
			ret = -EFAULT;
			goto err_unreference;
		}
	}

	max_cnt = sunxi_check_max_plane_num_format(mode_cmd->pixel_format);

	if (nr_gem == 0 || nr_gem > max_cnt) {
		DRM_ERROR("cannot use this gem memory type for fb, gem[%d][%08x].\n",nr_gem,mode_cmd->pixel_format);
		ret = -EINVAL;
		goto err_unreference;
	}

	fb_obj = sunxi_drm_framebuffer_creat(dev, mode_cmd, obj, nr_gem, 0);
	if (NULL == fb_obj) {
		ret = -ENOMEM;
		DRM_ERROR("cannot creat drm_framebuffer.\n");
		goto err_unreference;
	}
	DRM_DEBUG_KMS("line[%d] fb:%d obj:%p\n", __LINE__, fb_obj->base.id, obj[0]);
	return fb_obj;

err_unreference:
	for (i = 0; i < MAX_FB_BUFFER; i++) {
		if (NULL != obj[i])
		drm_gem_object_unreference_unlocked(obj[i]);
	}
	return ERR_PTR(ret);
}

struct sunxi_drm_gem_buf *sunxi_drm_framebuffer_buffer(struct drm_framebuffer *fb, int index)
{
	struct sunxi_drm_framebuffer *sunxi_fb = to_sunxi_fb(fb);
	struct sunxi_drm_gem_buf *buffer;
	struct drm_gem_object	*gem_obj = sunxi_fb->gem_obj[index];

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	if (index >= MAX_FB_BUFFER)
		return NULL;
	if (NULL == gem_obj) {
		return NULL;
	}

	buffer = (struct sunxi_drm_gem_buf *)gem_obj->driver_private;
	if (!buffer)
		return NULL;

	DRM_DEBUG_KMS("dma_addr = 0x%llx\n", buffer->dma_addr);

	return buffer;
}

static struct drm_framebuffer_funcs sunxi_drm_framebuffer_funcs = {
	.destroy = sunxi_drm_framebuffer_destroy,
	.create_handle = sunxi_drm_framebuffer_create_handle,
	.dirty = sunxi_drm_framebuffer_dirty,
};

struct drm_framebuffer *sunxi_drm_framebuffer_creat(struct drm_device *dev,
		struct drm_mode_fb_cmd2 *mode_cmd,
		struct drm_gem_object *obj[MAX_FB_BUFFER], int nr_gem, int flags)
{
	struct sunxi_drm_framebuffer *sunxi_fb;
	int ret, i, nr;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	sunxi_fb = kzalloc(sizeof(*sunxi_fb), GFP_KERNEL);
	if (!sunxi_fb) {
		DRM_ERROR("failed to allocate sunxi drm framebuffer\n");
		return NULL;
	}

	drm_helper_mode_fill_fb_struct(&sunxi_fb->drm_fb, mode_cmd);
	/* fb->refcount = 2 */
	ret = drm_framebuffer_init(dev, &sunxi_fb->drm_fb, &sunxi_drm_framebuffer_funcs);
	if (ret) {
		kfree(sunxi_fb);
		DRM_ERROR("failed to initialize framebuffer\n");
		return NULL;
	}

	for (i = 0, nr = 0; i < MAX_FB_BUFFER && nr < nr_gem; i++) {
		if (obj[i] != NULL) {
			sunxi_fb->gem_obj[i] = obj[i];
			nr++;
		}
	}
	sunxi_fb->drm_fb.flags = mode_cmd->flags;
	sunxi_fb->nr_gem = nr_gem;
	sunxi_fb->fb_flag = flags;

	return &sunxi_fb->drm_fb;
}

static void sunxi_drm_output_poll_changed(struct drm_device *dev)
{
	struct sunxi_drm_private *private = dev->dev_private;
	struct drm_fb_helper *fb_helper = private->fb_helper;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	if (fb_helper)
		drm_fb_helper_hotplug_event(fb_helper);
}

static const struct drm_mode_config_funcs sunxi_drm_mode_config_funcs = {
	.fb_create = sunxi_user_fb_create,
	.output_poll_changed = sunxi_drm_output_poll_changed,
};

void sunxi_drm_mode_config_init(struct drm_device *dev)
{
	dev->mode_config.min_width = 0;
	dev->mode_config.min_height = 0;

	/* max_width be decided by the de bufferline */
	dev->mode_config.max_width = 4096;
	dev->mode_config.max_height = 4096;

	dev->mode_config.funcs = &sunxi_drm_mode_config_funcs;

}

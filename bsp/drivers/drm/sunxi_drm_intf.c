/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include "sunxi_drm_intf.h"
#include "sunxi_drm_crtc.h"

struct drm_property *
sunxi_drm_create_attach_property_enum(struct drm_device *drm,
					 struct drm_mode_object *base,
					 const char *name,
					 const struct drm_prop_enum_list *enums,
					 int num_enums,
					 uint64_t init_val)
{
	struct drm_property *prop;

	prop = drm_property_create_enum(drm, 0, name, enums, num_enums);
	if (!prop) {
		DRM_ERROR("sunxi drm fails to create enum property:%s\n", name);
		return NULL;
	}

	drm_object_attach_property(base, prop, init_val);

	return prop;
}

struct drm_property *
sunxi_drm_create_attach_property_range(struct drm_device *drm,
					 struct drm_mode_object *base,
					 const char *name,
					 uint64_t min, uint64_t max,
					 uint64_t init_val)
{
	struct drm_property *prop;

	prop = drm_property_create_range(drm, 0, name, min, max);
	if (!prop) {
		DRM_ERROR("sunxi drm fails to create range property:%s\n", name);
		return NULL;
	}

	drm_object_attach_property(base, prop, init_val);

	return prop;
}

struct drm_property *
sunxi_drm_create_attach_property_bitmask(struct drm_device *drm,
					 struct drm_mode_object *base,
					 const char *name,
					 const struct drm_prop_enum_list *list,
					 int num,
					 uint64_t support_bit,
					 uint64_t init_bit)
{
	struct drm_property *prop;

	prop = drm_property_create_bitmask(drm, 0, name, list, num, support_bit);
	if (IS_ERR_OR_NULL(prop)) {
		DRM_ERROR("sunxi drm fails to create bitmask property: %s\n", name);
		return NULL;
	}

	drm_object_attach_property(base, prop, init_bit);
	return prop;
}

int drm_mode_to_sunxi_video_timings(struct drm_display_mode *mode,
				    struct disp_video_timings *timings)
{
	if (!mode) {
		DRM_ERROR("drm mode invalid!\n");
		return -1;
	}

	if (!timings) {
		DRM_ERROR("sunxi video timings invalid!\n");
		return -1;
	}

	timings->vic = drm_match_cea_mode(mode);
	timings->pixel_clk = mode->clock * 1000;
	if (mode->clock < 27000)
		timings->pixel_repeat = 1;

	timings->b_interlace = mode->flags & DRM_MODE_FLAG_INTERLACE;
	timings->x_res = mode->hdisplay;
	timings->y_res = mode->vdisplay;
	timings->hor_total_time = mode->htotal;
	timings->hor_back_porch = mode->htotal - mode->hsync_end;
	timings->hor_front_porch = mode->hsync_start - mode->hdisplay;
	timings->hor_sync_time = mode->hsync_end - mode->hsync_start;
	timings->hor_sync_polarity = (mode->flags & DRM_MODE_FLAG_PHSYNC) ? 1 : 0;

	timings->ver_total_time = mode->vtotal;
	timings->ver_back_porch = (mode->vtotal - mode->vsync_end)
		/ (timings->b_interlace + 1);
	timings->ver_front_porch = (mode->vsync_start - mode->vdisplay)
		/ (timings->b_interlace + 1);
	timings->ver_sync_time = (mode->vsync_end - mode->vsync_start)
		/ (timings->b_interlace + 1);
	timings->ver_sync_polarity = (mode->flags & DRM_MODE_FLAG_PVSYNC) ? 1 : 0;

	return 0;
}

struct drm_connector *drm_device_to_connector(struct drm_device *dev, int drm_mode_connector)
{
	struct drm_connector_list_iter conn_iter;
	struct drm_connector *connector = NULL;

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(connector, &conn_iter) {
		if (connector->connector_type == drm_mode_connector) {
			DRM_ERROR("Get connector_type:%d\n", drm_mode_connector);
			break;
		}
	}

	drm_connector_list_iter_end(&conn_iter);
	return connector;
}

void print_physical_memory(struct drm_printer *p, struct resource *res,
						   size_t offset, size_t size)
{
	uintptr_t phys_addr = res->start;
	void __iomem *base;
	size_t i, j;

	base = ioremap(phys_addr + offset, size);

	for (i = 0; i < size; i += 16) {
		size_t line_size = min(size - i, (size_t)16);

		drm_printf(p, "0x%08lx: ", (unsigned long)(phys_addr + i + offset));
		for (j = 0; j < line_size; j += 4)
			drm_printf(p, "%08x ", readl(base + i + j));
		drm_printf(p, "\n");
	}
	iounmap(base);
}

int sunxi_parse_dump_string(const char *buf, size_t size,
		unsigned long *start, unsigned long *end)
{
	char *ptr = NULL;
	char *ptr2 = (char *)buf;
	int ret = 0, times = 0;

	/* Support single address mode, some time it haven't ',' */
next:

	/* Default dump only one register(*start =*end).
	If ptr is not NULL, we will cover the default value of end. */
	if (times == 1)
		*start = *end;

	if (!ptr2 || (ptr2 - buf) >= size)
		goto out;

	ptr = ptr2;
	ptr2 = strnchr(ptr, size - (ptr - buf), ',');
	if (ptr2) {
		*ptr2 = '\0';
		ptr2++;
	}

	ptr = strim(ptr);
	if (!strlen(ptr))
		goto next;

	ret = kstrtoul(ptr, 16, end);
	if (!ret) {
		times++;
		goto next;
	} else
		DRM_ERROR("String syntax errors: \"%s\"\n", ptr);

out:
	return ret;
}

/* FIXME:The original function was drm_atomic_helper_disable_all.
 * Due to GKI (Generic Kernel Image) compatibility issues, it had
 * to be temporarily exported for use. This is a provisional solution,
 * and the function should eventually be removed and replaced with
 * drm_atomic_helper_disable_all in the future.
 */
int sunxi_drm_atomic_helper_disable_all(struct drm_device *dev,
				  struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_atomic_state *state;
	struct drm_connector_state *conn_state;
	struct drm_connector *conn;
	struct drm_plane_state *plane_state;
	struct drm_plane *plane;
	struct drm_crtc_state *crtc_state;
	struct drm_crtc *crtc;
	int ret, i;

	state = drm_atomic_state_alloc(dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = ctx;

	drm_for_each_crtc(crtc, dev) {
		crtc_state = drm_atomic_get_crtc_state(state, crtc);
		if (IS_ERR(crtc_state)) {
			ret = PTR_ERR(crtc_state);
			goto free;
		}

		crtc_state->active = false;

		ret = drm_atomic_set_mode_prop_for_crtc(crtc_state, NULL);
		if (ret < 0)
			goto free;

		ret = drm_atomic_add_affected_planes(state, crtc);
		if (ret < 0)
			goto free;

		ret = drm_atomic_add_affected_connectors(state, crtc);
		if (ret < 0)
			goto free;
	}

	for_each_new_connector_in_state(state, conn, conn_state, i) {
		ret = drm_atomic_set_crtc_for_connector(conn_state, NULL);
		if (ret < 0)
			goto free;
	}

	for_each_new_plane_in_state(state, plane, plane_state, i) {
		ret = drm_atomic_set_crtc_for_plane(plane_state, NULL);
		if (ret < 0)
			goto free;

		drm_atomic_set_fb_for_plane(plane_state, NULL);
	}

	ret = drm_atomic_commit(state);
free:
	drm_atomic_state_put(state);
	return ret;
}

int sunxi_drm_mode_config_reset(struct drm_device *dev)
{
	int err;
	struct drm_modeset_acquire_ctx ctx;
	struct drm_atomic_state *state = NULL;

	if (!dev)
		return 0;

	if (WARN_ON(!!dev->mode_config.suspend_state))
		return -EINVAL;

	drm_kms_helper_poll_disable(dev);
	drm_fb_helper_set_suspend_unlocked(dev->fb_helper, 1);

	DRM_MODESET_LOCK_ALL_BEGIN(dev, ctx, 0, err);
	state = drm_atomic_helper_duplicate_state(dev, &ctx);
	if (IS_ERR(state)) {
		err = PTR_ERR(state);
		goto out;
	}

	/* FIXME: It needs to be fixed after adding it to the whitelist. */
	/* drm_atomic_helper_disable_all(dev, &ctx); */
	sunxi_drm_atomic_helper_disable_all(dev, &ctx);

	err = drm_atomic_helper_commit_duplicated_state(state, &ctx);

out:
	DRM_MODESET_LOCK_ALL_END(dev, ctx, err);
	drm_atomic_state_put(state);

	drm_fb_helper_set_suspend_unlocked(dev->fb_helper, 0);
	drm_kms_helper_poll_enable(dev);

	return err;
}

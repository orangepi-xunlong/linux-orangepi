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


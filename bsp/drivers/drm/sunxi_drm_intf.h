/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __SUNXI_DRM_INTF_H__
#define __SUNXI_DRM_INTF_H__

#include <drm/drm_modes.h>
#include <drm/drm_edid.h>
#include <drm/drm_property.h>
#include <drm/drm_plane.h>
#include <drm/drm_crtc.h>
#include <drm/drm_print.h>
#include <drm/drm_drv.h>
#include <drm/drm_atomic.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_modeset_lock.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_atomic_helper.h>

#include "include.h"

struct drm_property *
sunxi_drm_create_attach_property_enum(struct drm_device *drm,
					 struct drm_mode_object *base,
					 const char *name,
					 const struct drm_prop_enum_list *enums,
					 int num_enums,
					 uint64_t init_val);

struct drm_property *
sunxi_drm_create_attach_property_range(struct drm_device *drm,
					 struct drm_mode_object *base,
					 const char *name,
					 uint64_t min, uint64_t max,
					 uint64_t init_val);

struct drm_property *
sunxi_drm_create_attach_property_bitmask(struct drm_device *drm,
					 struct drm_mode_object *base,
					 const char *name,
					 const struct drm_prop_enum_list *list,
					 int num,
					 uint64_t support_bit,
					 uint64_t init_bit);

int drm_mode_to_sunxi_video_timings(struct drm_display_mode *mode,
				    struct disp_video_timings *timing);

struct drm_connector *drm_device_to_connector(struct drm_device *dev,
					int drm_mode_connector);

void print_physical_memory(struct drm_printer *p, struct resource *res,
						   size_t offset, size_t size);

int sunxi_parse_dump_string(const char *buf, size_t size,
		unsigned long *start, unsigned long *end);

int sunxi_drm_mode_config_reset(struct drm_device *dev);

#endif /*End of file*/

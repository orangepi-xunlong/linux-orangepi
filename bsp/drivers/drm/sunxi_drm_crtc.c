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
#include <linux/kthread.h>
#include <linux/version.h>
#include <drm/drm_blend.h>
#include <drm/drm_edid.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_vblank.h>
#include <drm/drm_vblank_work.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_writeback.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_uapi.h>
#include <linux/version.h>
#include <linux/sort.h>
#include <linux/completion.h>

#include "sunxi_drm_crtc.h"
#include "sunxi_drm_drv.h"
#include "sunxi_drm_trace.h"
#include "sunxi_drm_debug.h"
#include "sunxi_device/hardware/lowlevel_de/de_base.h"

#define WB_SIGNAL_MAX		2

/* wb finish after two vsync, use to signal work finish after vysnc, 2 struct wb_signal_wait is enough */
struct wb_signal_wait {
	bool active;
	unsigned int vsync_cnt;
};

struct sunxi_drm_wb {
	struct sunxi_de_wb *hw_wb;
	struct drm_writeback_connector wb_connector;
	spinlock_t signal_lock;
	struct wb_signal_wait signal[WB_SIGNAL_MAX];
};

struct sunxi_drm_crtc {
	struct drm_crtc crtc;
	struct sunxi_de_out *sunxi_de;
	unsigned int plane_cnt;
	unsigned int hw_id;
	/* protect by modeset_lock*/
	bool fbdev_output;
	/* protect by modeset_lock*/
	bool fbdev_flush_pending;
	/* protect by modeset_lock*/
	bool async_update_flush_pending;
	bool fbdev_init;
	struct completion flush_done;
	unsigned int fbdev_chn_id;
	bool gamma_dirty;
	bool enabled;
	unsigned long clk_freq;
	struct sunxi_drm_plane *plane;
	struct drm_pending_vblank_event *event;
	struct sunxi_drm_wb *wb;
	/* protect wb */
	spinlock_t wb_lock;
	/* output device info */
	vblank_enable_callback_t enable_vblank;
	fifo_status_check_callback_t check_status;
	is_sync_time_enough_callback_t is_sync_time_enough;
	get_cur_line_callback_t get_cur_line;
	is_support_backlight_callback_t is_support_backlight;
	set_backlight_value_callback_t set_backlight_value;
	get_backlight_value_callback_t get_backlight_value;
	void *output_dev_data;

	// protect by drm_device->event_lock
	int should_send_vblank;

	/* irq counter for systrace record */
	unsigned int irqcnt;
	unsigned int fifo_err;
	unsigned int vblank_trace;

	unsigned int hue_default_value;
	bool share_scaler;
};

struct sunxi_drm_plane {
	struct drm_plane plane;
	struct de_channel_handle *hdl;
	unsigned int index;
	unsigned int layer_cnt;
	struct sunxi_drm_crtc *crtc;
};

enum sunxi_plane_alpha_mode {
	PIXEL_ALPHA = 0,
	GLOBAL_ALPHA = 1,
	MIXED_ALPHA = 2,
};

#define to_sunxi_plane(x)			container_of(x, struct sunxi_drm_plane, plane)
#define to_sunxi_crtc(x)			container_of(x, struct sunxi_drm_crtc, crtc)
#define wb_connector_to_sunxi_wb(x)		container_of(x, struct sunxi_drm_wb, wb_connector.base)

struct task_struct *commit_task;

static int
sunxi_replace_property_blob_from_id(struct drm_device *dev,
					 struct drm_property_blob **blob,
					 uint64_t blob_id,
					 ssize_t expected_size,
					 bool *replaced)
{
	struct drm_property_blob *new_blob = NULL;

	if (blob_id != 0) {
		new_blob = drm_property_lookup_blob(dev, blob_id);
		if (new_blob == NULL) {
			DRM_ERROR("blob not found\n");
			return -EINVAL;
	}

		if (expected_size > 0 &&
		    new_blob->length != expected_size) {
			DRM_ERROR("blob size err %d %d\n", (int)new_blob->length, (int)expected_size);
			drm_property_blob_put(new_blob);
			return -EINVAL;
		}
	}

	*replaced |= drm_property_replace_blob(blob, new_blob);
	drm_property_blob_put(new_blob);

	return *replaced ? 0 : -EINVAL;
}

static int sunxi_crtc_pq_proc_locked(struct sunxi_drm_crtc *scrtc, enum sunxi_pq_type type, void *data)
{
	struct sunxi_crtc_state *state = to_sunxi_crtc_state(scrtc->crtc.state);
	struct de_backend_data *cur;
	struct csc_info *csc;
	drm_warn_on_modeset_not_all_locked(scrtc->crtc.dev);

	if (state->backend_blob) {
		state->backend_blob = drm_property_create_blob(scrtc->crtc.dev, sizeof(struct de_backend_data), NULL);
	}
	if (!data)
		return -EINVAL;

	cur = state->backend_blob->data;
	switch (type) {
	case PQ_DEBAND:
		cur->dirty |= DEBAND_DIRTY;
		cur->deband_para.dirty |= PQD_DIRTY_MASK;
		memcpy(&cur->deband_para.pqd, data, sizeof(cur->deband_para.pqd));
		if (cur->deband_para.pqd.cmd == PQ_READ) {
			sunxi_de_backend_get_pqd_config(scrtc->sunxi_de, cur);
			if (!(cur->dirty & DEBAND_DIRTY))
				memcpy(data, &cur->deband_para.pqd, sizeof(cur->deband_para.pqd));
		}
		break;
	case PQ_COLOR_MATRIX:
		csc = data;
		if (csc->dirty == MATRIX_DIRTY) {
			cur->dirty |= CSC_DIRTY;
			cur->csc_para.dirty |= PQD_DIRTY_MASK;
			memcpy(&cur->csc_para.pqd, data, sizeof(cur->csc_para.pqd));
			if (cur->csc_para.pqd.cmd == PQ_READ) {
				sunxi_de_backend_get_pqd_config(scrtc->sunxi_de, cur);
				if (!(cur->dirty & CSC_DIRTY))
					memcpy(data, &cur->csc_para.pqd, sizeof(cur->csc_para.pqd));
			} else {
				//tiger re-apply csc
				state->bcsh_changed = true;
			}
		} else if (csc->dirty == BCSH_DIRTY) {
			if (csc->cmd == PQ_READ) {
				csc->enhance.contrast = state->excfg.contrast;
				csc->enhance.brightness = state->excfg.brightness;
				csc->enhance.saturation = state->excfg.saturation;
				csc->enhance.hue = state->excfg.hue;
			} else {
				state->excfg.contrast = csc->enhance.contrast;
				state->excfg.brightness = csc->enhance.brightness;
				state->excfg.saturation = csc->enhance.saturation;
				state->excfg.hue = csc->enhance.hue;
				state->bcsh_changed = true;
			}
		}
		break;

	default:
		DRM_ERROR("pq cmd not support %d\n", type);
		return -EINVAL;
	}
	return 0;
}

static int sunxi_plane_pq_proc_locked(struct drm_plane *plane, enum sunxi_pq_type type, void *data)
{
	struct display_channel_state *cstate = to_display_channel_state(plane->state);
	struct de_frontend_data *cur;
	struct sunxi_drm_plane *splane = to_sunxi_plane(plane);

	drm_warn_on_modeset_not_all_locked(plane->dev);

	if (!cstate->frontend_blob) {
		cstate->frontend_blob = drm_property_create_blob(plane->dev, sizeof(struct de_frontend_data), NULL);
	}
	if (!data)
		return -EINVAL;

	cur = cstate->frontend_blob->data;

	switch (type) {
	case PQ_FCM:
		cur->dirty |= FCM_DIRTY;
		cur->fcm_para.dirty |= PQD_DIRTY_MASK;
		memcpy(&cur->fcm_para.pqd, data, sizeof(cur->fcm_para.pqd));
		if (cur->fcm_para.pqd.cmd == PQ_READ) {
			channel_get_pqd_config(splane->hdl, cstate);
			if (!(cur->fcm_para.dirty & PQD_DIRTY_MASK))
				memcpy(data, &cur->fcm_para.pqd, sizeof(cur->fcm_para.pqd));
		}
		break;
	case PQ_DCI:
		cur->dirty |= DCI_DIRTY;
		cur->dci_para.dirty |= PQD_DIRTY_MASK;
		memcpy(&cur->dci_para.pqd, data, sizeof(cur->dci_para.pqd));
		if (cur->dci_para.pqd.cmd == PQ_READ) {
			channel_get_pqd_config(splane->hdl, cstate);
			if (!(cur->dirty & DCI_DIRTY))
				memcpy(data, &cur->dci_para.pqd, sizeof(cur->dci_para.pqd));
		}
		break;
	case PQ_DLC:
		cur->dirty |= DLC_DIRTY;
		cur->dlc_para.dirty |= PQD_DIRTY_MASK;
		memcpy(&cur->dlc_para.pqd, data, sizeof(cur->dlc_para.pqd));
		if (cur->dlc_para.pqd.cmd == PQ_READ) {
			channel_get_pqd_config(splane->hdl, cstate);
			if (!(cur->dirty & DLC_DIRTY))
				memcpy(data, &cur->dlc_para.pqd, sizeof(cur->dlc_para.pqd));
		}
		break;
	case PQ_GTM:
		cur->dirty |= CDC_DIRTY;
		cur->cdc_para.dirty |= PQD_DIRTY_MASK;
		memcpy(&cur->cdc_para.pqd, data, sizeof(cur->cdc_para.pqd));
		if (cur->cdc_para.pqd.cmd == PQ_READ) {
			channel_get_pqd_config(splane->hdl, cstate);
			if (!(cur->dirty & CDC_DIRTY))
				memcpy(data, &cur->cdc_para.pqd, sizeof(cur->cdc_para.pqd));
		}
		break;

	case PQ_SHARP35X:
		cur->dirty |= SHARP_DIRTY;
		cur->sharp_para.dirty |= PQD_DIRTY_MASK;
		memcpy(&cur->sharp_para.pqd, data, sizeof(cur->sharp_para.pqd));
		if (cur->sharp_para.pqd.cmd == PQ_READ) {
			channel_get_pqd_config(splane->hdl, cstate);
			if (!(cur->dirty & SHARP_DIRTY))
				memcpy(data, &cur->sharp_para.pqd, sizeof(cur->sharp_para.pqd));
		}
		break;
	case PQ_ASU:
		cur->dirty |= ASU_DIRTY;
		cur->asu_para.dirty |= PQD_DIRTY_MASK;
		memcpy(&cur->asu_para.pqd, data, sizeof(cur->asu_para.pqd));
		if (cur->asu_para.pqd.cmd == PQ_READ) {
			channel_get_pqd_config(splane->hdl, cstate);
			if (!(cur->dirty & ASU_DIRTY))
				memcpy(data, &cur->asu_para.pqd, sizeof(cur->asu_para.pqd));
		}
		break;
	case PQ_SNR:
		cur->dirty |= SNR_DIRTY;
		cur->snr_para.dirty |= PQD_DIRTY_MASK;
		memcpy(&cur->snr_para.pqd, data, sizeof(cur->snr_para.pqd));
		if (cur->snr_para.pqd.cmd == PQ_READ) {
			channel_get_pqd_config(splane->hdl, cstate);
			if (!(cur->dirty & SNR_DIRTY))
				memcpy(data, &cur->snr_para.pqd, sizeof(cur->snr_para.pqd));
		}
		break;

	default:
		DRM_ERROR("pq cmd not support %d\n", type);
		return -EINVAL;
	}
	return 0;
}

static int sunxi_crtc_gamma_proc_locked(struct drm_crtc *crtc, u32 *lut, bool get)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct drm_crtc_state *state = crtc->state;
	struct drm_color_lut *cur;
	u32 r, g, b;
	int i;
	drm_warn_on_modeset_not_all_locked(crtc->dev);

	if (!state->gamma_lut) {
		state->gamma_lut = drm_property_create_blob(crtc->dev,
					  crtc->gamma_size * sizeof(struct drm_color_lut), lut);
	}

	if (!lut)
		return -EINVAL;

	cur = state->gamma_lut->data;
	if (get) {
		for (i = 0; i < crtc->gamma_size; i++) {
			if (crtc->gamma_size == 1024) {
				r = (cur[i].red >> 6) & 0x3ff;
				g = (cur[i].green >> 6) & 0x3ff;
				b = (cur[i].blue >> 6) & 0x3ff;
				lut[i] = (r << 20) | (g << 10) | b;
			} else {
				r = (cur[i].red >> 8) & 0xff;
				g = (cur[i].green >> 8) & 0xff;
				b = (cur[i].blue >> 8) & 0xff;
				lut[i] = (r << 16) | (g << 8) | b;
			}
		}
	} else {
		scrtc->gamma_dirty = true;
		for (i = 0; i < crtc->gamma_size; i++) {
			if (crtc->gamma_size == 1024) {
				cur[i].red = ((lut[i] >> 20) & 0x3ff) << 6;
				cur[i].green = ((lut[i] >> 10) & 0x3ff) << 6;
				cur[i].blue = (lut[i] & 0x3ff) << 6;
			} else {
				cur[i].red = ((lut[i] >> 16) & 0xff) << 8;
				cur[i].green = ((lut[i] >> 8) & 0xff) << 8;
				cur[i].blue = (lut[i] & 0xff) << 8;
			}
		}
	}
	return 0;
}

#define if_in_module(type_name, type)  ({			\
	bool __ret = false;					\
	int __i;						\
	for (__i = 0; __i < ARRAY_SIZE(__##type_name); __i++) {	\
		if (type == __##type_name[__i]) {		\
			__ret = true;				\
			break;					\
		}						\
	}							\
	__ret;							\
})

int sunxi_drm_crtc_pq_proc(struct drm_device *dev, int disp, enum sunxi_pq_type type, void *data)
{
	int __backend[] = {PQ_DEBAND, PQ_COLOR_MATRIX};
	int __frontend[] = {PQ_FCM, PQ_DCI, PQ_DLC, PQ_SHARP35X, PQ_SNR, PQ_ASU, PQ_GTM};//only channel cdc support gtm
	int __backend_and_frontend[] = {PQ_SET_REG, PQ_GET_REG, PQ_COLOR_MATRIX, PQ_CDC};
	struct gamma_para *gamma = data;
	bool in_backend;
	bool in_frontend;
	bool in_backend_and_frontend;
	struct drm_plane *plane;
	struct sunxi_drm_plane *splane;
	struct drm_crtc *crtc;
	struct sunxi_drm_crtc *scrtc;
	int ret = 0;

	in_backend = if_in_module(backend, type);
	in_frontend = if_in_module(frontend, type);
	in_backend_and_frontend = if_in_module(backend_and_frontend, type);

	drm_modeset_lock_all(dev);
	if (in_backend || in_backend_and_frontend) {
		drm_for_each_crtc(crtc, dev) {
			scrtc = to_sunxi_crtc(crtc);
			if (scrtc->hw_id != disp)
				continue;
			ret = sunxi_crtc_pq_proc_locked(scrtc, type, data);
			if (ret)
				goto OUT;;
		}
	} else if (in_frontend || in_backend_and_frontend) {
		drm_for_each_plane(plane, dev) {
			splane = to_sunxi_plane(plane);
			scrtc = splane->crtc;
			if (scrtc->hw_id != disp)
				continue;
			ret = sunxi_plane_pq_proc_locked(plane, type, data);
			if (ret)
				goto OUT;;
		}
	}
	if (type == PQ_GAMMA)
		drm_for_each_crtc(crtc, dev) {
			scrtc = to_sunxi_crtc(crtc);
			if (scrtc->hw_id != disp)
				continue;
			if (gamma->size != crtc->gamma_size)
				goto OUT;
			ret = sunxi_crtc_gamma_proc_locked(crtc, gamma->lut, gamma->cmd == PQ_READ);
			if (ret)
				goto OUT;
	}
OUT:
	drm_modeset_unlock_all(dev);
	return ret;
}

int sunxi_fbdev_plane_update(struct fbdev_config *config)
{
	struct drm_device *dev = config->dev;
	struct drm_plane *plane;
	struct sunxi_drm_plane *sunxi_plane;
	struct sunxi_drm_plane *fbdev_plane = NULL;
	struct sunxi_drm_crtc *scrtc;
	struct sunxi_drm_crtc *scrtc_fbdev;
	struct sunxi_de_channel_update info;
	struct display_channel_state disable;
	bool update;
	bool lock = !config->force;

	DRM_DEBUG_DRIVER("[SUNXI-DE] fbdev plane update\n");
	memset(&disable, 0, sizeof(disable));
	if (lock)
		drm_modeset_lock_all(dev);

	/* find fbdev channel, disable the remains channel if force */
	drm_for_each_plane(plane, dev) {
		sunxi_plane = to_sunxi_plane(plane);
		scrtc = sunxi_plane->crtc;
		if (scrtc->hw_id == config->de_id &&
		    sunxi_plane->index == config->channel_id) {
			fbdev_plane = sunxi_plane;
			scrtc_fbdev = scrtc;
			continue;
		} else if (config->force) {
			info.fbdev_output = true;
			info.force = true;
			info.hdl = sunxi_plane->hdl;
			info.hwde = sunxi_plane->crtc->sunxi_de;
			info.new_state = (void *)&disable;
			info.old_state = (void *)&disable;
			sunxi_de_channel_update(&info);
		}
		if (fbdev_plane && !config->force)
			break;
	}
	if (!fbdev_plane) {
		DRM_ERROR("plane fb %d %d for fb not found!\n", config->de_id, config->channel_id);
		if (lock)
			drm_modeset_unlock_all(dev);
		return -ENXIO;
	}

	plane = &fbdev_plane->plane;
	if ((plane->state->fb || plane->state->crtc) && !config->force && dev->master) {
		WARN_ON(scrtc_fbdev->fbdev_output);
		DRM_INFO("skip fbdev plane update because plane used by userspace\n");
		if (lock)
			drm_modeset_unlock_all(dev);
		return 0;
	}

	scrtc_fbdev->fbdev_output = true;
	scrtc_fbdev->fbdev_chn_id = config->channel_id;

	if (!scrtc_fbdev->fbdev_init) {
		/* commit_init_connecting() will commit fbdev channel */
		scrtc_fbdev->fbdev_init = true;
		*config->out_plane = plane;
		*config->out_crtc = &scrtc_fbdev->crtc;
		if (lock)
			drm_modeset_unlock_all(dev);
		return 0;
	}

	info.fbdev_output = scrtc_fbdev->fbdev_output;
	info.hdl = fbdev_plane->hdl;
	info.hwde = fbdev_plane->crtc->sunxi_de;
	info.new_state = (void *)config->fake_state;
	info.old_state = (void *)config->fake_state;
	info.is_fbdev = true;
	sunxi_de_channel_update(&info);

	if (lock)
		mutex_lock(&dev->master_mutex);
	update = dev->master ? false : true;
	update |= config->force;
	if (lock)
		mutex_unlock(&dev->master_mutex);
	if (update)
		sunxi_de_atomic_flush(scrtc_fbdev->sunxi_de, NULL,
			    config->force ? (void *)FORCE_ATOMIC_FLUSH : NULL);
	else
		scrtc_fbdev->fbdev_flush_pending = true;
	reinit_completion(&scrtc_fbdev->flush_done);
	if (lock)
		drm_modeset_unlock_all(dev);
	if (!update)
		wait_for_completion(&scrtc_fbdev->flush_done);
	DRM_DEBUG_DRIVER("[SUNXI-DE] fbdev plane update finish self_update :%d\n", update);
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static void sunxi_plane_atomic_update(struct drm_plane *plane,
				      struct drm_plane_state *old_state)
{

	struct display_channel_state *old_cstate = to_display_channel_state(old_state);
#else
static void sunxi_plane_atomic_update(struct drm_plane *plane,
				      struct drm_atomic_state *state)

{
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(state, plane);
	struct display_channel_state *old_cstate = to_display_channel_state(old_state);
#endif
	struct drm_plane_state *new_state = plane->state;
	struct display_channel_state *new_cstate = to_display_channel_state(new_state);
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct sunxi_drm_crtc *scrtc = sunxi_plane->crtc;
	struct sunxi_de_channel_update info;

	info.hdl = sunxi_plane->hdl;
	info.hwde = scrtc->sunxi_de;
	info.new_state = new_cstate;
	info.old_state = old_cstate;
	info.is_fbdev = false;
	info.fbdev_output = scrtc->fbdev_output;
	sunxi_de_channel_update(&info);

	if (new_cstate) {
		if (new_cstate->base.fb) {
			int i = 0;
			sunxidrm_debug_trace_frame(scrtc->hw_id, sunxi_plane->index, new_cstate->base.fb);
			for (i = 0; i < (MAX_LAYER_NUM_PER_CHN - 1); i++) {
				if (new_cstate->fb[i])
					sunxidrm_debug_trace_frame(scrtc->hw_id, sunxi_plane->index, new_cstate->fb[i]);
			}
		}
	}
}

static int __maybe_unused sunxi_plane_atomic_precheck(struct drm_plane *plane,
				      struct drm_plane_state *new_state)
{
	struct display_channel_state *cstate = to_display_channel_state(new_state);
	bool remain_enable = false;
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct sunxi_drm_crtc *scrtc = sunxi_plane->crtc;
	int i, ret;
	struct drm_framebuffer *fb = NULL;
	/* FIXME: add crtc enable when disable layer 123, when still remain layer is enabled */

	/* if layer_id is set, enable async update */
	if (cstate->layer_id != COMMIT_ALL_LAYER) {
		new_state->state->legacy_cursor_update = true;
		DRM_DEBUG_DRIVER("[SUNXI-DE] channel %s  %s %d\n", plane->name, __func__, __LINE__);
	}

	/* if async update, handle plane's fake fb and crtc */
	if (new_state->state->legacy_cursor_update) {
		if (cstate->layer_id == 0) {
			fb = new_state->fb;
			/* enable layer0: remove fake flag */
			if (fb) {
				cstate->fake_layer0 = false;
				DRM_DEBUG_DRIVER("[SUNXI-DE] channel %s 0 async update check enable\n", plane->name);
			/* disable layer0: only disable when all layer is disable */
			} else {
				for (i = 0; i < OVL_REMAIN; i++) {
					fb = cstate->fb[i];
					if (fb) {
						remain_enable = true;
						break;
					}
				}
				if (remain_enable) {
					drm_atomic_set_fb_for_plane(new_state, fb);
					ret = drm_atomic_set_crtc_for_plane(new_state, &scrtc->crtc);
					if (ret) {
						DRM_ERROR("[SUNXI-DE] async update check set crtc fail for fake layer0\n");
						return ret;
					}
					cstate->fake_layer0 = true;
					DRM_DEBUG_DRIVER("[SUNXI-DE] async update check plane disable plane %s by layer0 "
							  "with fake_layer0 add\n", plane->name);
				} else {
					DRM_DEBUG_DRIVER("[SUNXI-DE] async update check disable plane %s by layer %d\n",
							  plane->name, cstate->layer_id);
				}
			}
		} else {
			fb = cstate->fb[cstate->layer_id - 1];
			/* disable layer1/2/3: disable layer0 if all layer disable*/
			if (!fb && cstate->fake_layer0) {
				for (i = 0; i < OVL_REMAIN; i++) {
					if (cstate->fb[i]) {
						remain_enable = true;
						break;
					}
				}
				if (!remain_enable) {
					cstate->fake_layer0 = false;
					drm_atomic_set_fb_for_plane(new_state, NULL);
					ret = drm_atomic_set_crtc_for_plane(new_state, NULL);
					if (ret) {
						DRM_ERROR("[SUNXI-DE] async update check set crtc fail for remove fake layer0\n");
						return ret;
					}

					DRM_DEBUG_DRIVER("[SUNXI-DE] async update check plane disable plane %s by layer%d "
							  "with fake_layer0 add\n", plane->name, cstate->layer_id);
				}
			} else if (fb) {
				/* enable layer1/2/3: enable layer0 if layer0 is not enable */
				if (!cstate->fake_layer0 && !new_state->fb) {
						drm_atomic_set_fb_for_plane(new_state, fb);
						ret = drm_atomic_set_crtc_for_plane(new_state, &scrtc->crtc);
						if (ret) {
							DRM_ERROR("[SUNXI-DE] async update check set crtc fail for fake layer0\n");
							return ret;
						}
						DRM_DEBUG_DRIVER("[SUNXI-DE] channel %s %d async update check enable with fake layer0\n",
								  plane->name, cstate->layer_id);
						cstate->fake_layer0 = true;
				} else {
					DRM_DEBUG_DRIVER("[SUNXI-DE] channel %s %d async update check enable without fake layer0\n",
							  plane->name, cstate->layer_id);
				}
			} else {
				DRM_DEBUG_DRIVER("[SUNXI-DE] channel %s async update check layer%d  fb %lx\n",
							plane->name, cstate->layer_id, (unsigned long)(fb));
			}
		}
	}
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static int sunxi_plane_atomic_async_check(struct drm_plane *plane,
				      struct drm_plane_state *new_state)
#else

static int sunxi_plane_atomic_async_check(struct drm_plane *plane,
					struct drm_atomic_state *state)
#endif
{
	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static void sunxi_plane_atomic_async_update(struct drm_plane *plane,
				      struct drm_plane_state *new_state)
{
#else
static void sunxi_plane_atomic_async_update(struct drm_plane *plane,
					  struct drm_atomic_state *state)
{
	struct drm_plane_state *new_state = drm_atomic_get_new_plane_state(state,
									   plane);
#endif
	struct display_channel_state *cstate = to_display_channel_state(new_state);
	struct display_channel_state *old_cstate = to_display_channel_state(plane->state);
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(new_state->crtc);
	int i = cstate->layer_id - 1;

	old_cstate->fake_layer0 = cstate->fake_layer0;
	if (cstate->layer_id == 0) {
		plane->state->crtc_x = new_state->crtc_x;
		plane->state->crtc_y = new_state->crtc_y;
		plane->state->crtc_h = new_state->crtc_h;
		plane->state->crtc_w = new_state->crtc_w;
		plane->state->src_x = new_state->src_x;
		plane->state->src_y = new_state->src_y;
		plane->state->src_h = new_state->src_h;
		plane->state->src_w = new_state->src_w;
		swap(plane->state->fb, new_state->fb);
	} else {
		old_cstate->crtc_x[i] = cstate->crtc_x[i];
		old_cstate->crtc_y[i] = cstate->crtc_y[i];
		old_cstate->crtc_h[i] = cstate->crtc_h[i];
		old_cstate->crtc_w[i] = cstate->crtc_w[i];
		old_cstate->src_x[i] = cstate->src_x[i];
		old_cstate->src_y[i] = cstate->src_y[i];
		old_cstate->src_h[i] = cstate->src_h[i];
		old_cstate->src_w[i] = cstate->src_w[i];
		swap(old_cstate->fb[i], cstate->fb[i]);
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	sunxi_plane_atomic_update(plane, new_state);
#else
	sunxi_plane_atomic_update(plane, state);
#endif
	scrtc->async_update_flush_pending = true;
	DRM_DEBUG_DRIVER("[SUNXI-DE] channel %s %d async update\n", plane->name, cstate->layer_id);
}

static int sunxi_atomic_plane_set_property(struct drm_plane *plane,
					  struct drm_plane_state *state,
					  struct drm_property *property,
					  uint64_t val)
{
	struct drm_device *dev = plane->dev;
	struct sunxi_drm_private *private = to_sunxi_drm_private(plane->dev);
	struct display_channel_state *cstate = to_display_channel_state(state);
	int i, ret;
	bool replaced;

	for (i = 0; i < OVL_REMAIN; i++) {
		if (property == private->prop_blend_mode[i]) {
			cstate->pixel_blend_mode[i] = val;
			return 0;
		}

		if (property == private->prop_alpha[i]) {
			cstate->alpha[i] = val;
			return 0;
		}

		if (property == private->prop_src_x[i]) {
			cstate->src_x[i] = val;
			return 0;
		}

		if (property == private->prop_src_y[i]) {
			cstate->src_y[i] = val;
			return 0;
		}

		if (property == private->prop_src_w[i]) {
			cstate->src_w[i] = val;
			return 0;
		}

		if (property == private->prop_src_h[i]) {
			cstate->src_h[i] = val;
			return 0;
		}

		if (property == private->prop_crtc_x[i]) {
			cstate->crtc_x[i] = val;
			return 0;
		}

		if (property == private->prop_crtc_y[i]) {
			cstate->crtc_y[i] = val;
			return 0;
		}

		if (property == private->prop_crtc_w[i]) {
			cstate->crtc_w[i] = val;
			return 0;
		}

		if (property == private->prop_crtc_h[i]) {
			cstate->crtc_h[i] = val;
			return 0;
		}

		if (property == private->prop_fb_id[i]) {
			struct drm_framebuffer *fb = drm_framebuffer_lookup(dev, NULL, val);
			if (fb) {
				drm_framebuffer_assign(&cstate->fb[i], fb);
				drm_framebuffer_put(fb);
				return 0;
			} else {
				drm_framebuffer_assign(&cstate->fb[i], NULL);
				return 0;
			}
		}

		if (property == private->prop_color[i]) {
			cstate->color[i] = val;
			return 0;
		}
	}

	if (property == private->prop_color[OVL_MAX - 1]) {
		cstate->color[OVL_MAX - 1] = val;
		return 0;
	}

	/* how to set blob from userspace:
	 * use drmModeCreatePropertyBlob to request and fill a blob by ioctl,
	 * get the blob id and use add_property/add_property_optional to deliver
	 * to KMS. Finally the follow case would be called
	 */
	if (property == private->prop_frontend_data) {
		ret = sunxi_replace_property_blob_from_id(dev,
				&cstate->frontend_blob,
				val, sizeof(struct de_frontend_data),
				&replaced);
		return ret;
	}

	if (property == private->prop_eotf) {
		cstate->eotf = val;
		return 0;
	}

	if (property == private->prop_color_space) {
		cstate->color_space = val;
		return 0;
	}

	if (property == private->prop_color_range) {
		cstate->color_range = val;
		return 0;
	}

	/* the read val of this prop is meaningless for userspace */
	if (property == private->prop_layer_id) {
		cstate->layer_id = val;
		return 0;
	}

	if (property == private->prop_compressed_image_crop) {
		cstate->compressed_image_crop = val;
		return 0;
	}

	DRM_ERROR("plane property %d name%s not found!\n",
		  property->base.id, property->name);
	return -EINVAL;
}

static int sunxi_atomic_plane_get_property(struct drm_plane *plane,
					  const struct drm_plane_state *state,
					  struct drm_property *property,
					  uint64_t *val)
{
	struct sunxi_drm_private *private = to_sunxi_drm_private(plane->dev);
	struct display_channel_state *cstate = to_display_channel_state(state);
	int i;

	for (i = 0; i < OVL_REMAIN; i++) {
		if (property == private->prop_blend_mode[i]) {
			*val = cstate->pixel_blend_mode[i];
			return 0;
		}

		if (property == private->prop_alpha[i]) {
			*val = cstate->alpha[i];
			return 0;
		}

		if (property == private->prop_src_x[i]) {
			*val = cstate->src_x[i];
			return 0;
		}

		if (property == private->prop_src_y[i]) {
			*val = cstate->src_y[i];
			return 0;
		}

		if (property == private->prop_src_w[i]) {
			*val = cstate->src_w[i];
			return 0;
		}

		if (property == private->prop_src_h[i]) {
			*val = cstate->src_h[i];
			return 0;
		}

		if (property == private->prop_crtc_x[i]) {
			*val = cstate->crtc_x[i];
			return 0;
		}

		if (property == private->prop_crtc_y[i]) {
			*val = cstate->crtc_y[i];
			return 0;
		}

		if (property == private->prop_crtc_w[i]) {
			*val = cstate->crtc_w[i];
			return 0;
		}

		if (property == private->prop_crtc_h[i]) {
			*val = cstate->crtc_h[i];
			return 0;
		}

		if (property == private->prop_fb_id[i]) {
			*val = cstate->fb[i] ? cstate->fb[i]->base.id : 0;
			return 0;
		}

		if (property == private->prop_color[i]) {
			*val = cstate->color[i];
			return 0;
		}
	}

	if (property == private->prop_color[OVL_MAX - 1]) {
		*val = cstate->color[OVL_MAX - 1];
		return 0;
	}

	if (property == private->prop_frontend_data) {
		*val = (cstate->frontend_blob) ? cstate->frontend_blob->base.id : 0;
		return 0;
	}

	if (property == private->prop_eotf) {
		*val = cstate->eotf;
		return 0;
	}

	if (property == private->prop_color_space) {
		*val = cstate->color_space;
		return 0;
	}

	if (property == private->prop_color_range) {
		*val = cstate->color_range;
		return 0;
	}

	/* the read val of this prop is meaningless for userspace */
	if (property == private->prop_layer_id) {
		*val = cstate->layer_id;
		return 0;
	}

	if (property == private->prop_compressed_image_crop) {
		*val = cstate->compressed_image_crop;
		return 0;
	}

	DRM_ERROR("plane property %d name%s not found!\n",
		  property->base.id, property->name);
	return -EINVAL;
}

static struct drm_plane_state *sunxi_atomic_plane_duplicate_state(struct drm_plane *plane)
{

	struct display_channel_state *old;
	struct display_channel_state *new;
	int i;

	if (WARN_ON(!plane->state))
		return NULL;

	old = to_display_channel_state(plane->state);
	new = kmemdup(old, sizeof(*new), GFP_KERNEL);

	if (!new)
		return NULL;
	for (i = 0; i < OVL_REMAIN; i++) {
		if (new->fb[i])
			drm_framebuffer_get(new->fb[i]);
	}

	if (new->frontend_blob) {
		drm_property_blob_get(new->frontend_blob);
	}
	__drm_atomic_helper_plane_duplicate_state(plane, &new->base);
	return &new->base;
}

static void sunxi_atomic_plane_destroy_state(struct drm_plane *plane,
					    struct drm_plane_state *state)
{
	struct display_channel_state *cstate = to_display_channel_state(state);
	int i;

	for (i = 0; i < OVL_REMAIN; i++) {
		if (cstate->fb[i]) {
			drm_framebuffer_put(cstate->fb[i]);
			cstate->fb[i] = NULL;
		}
	}

	if (cstate->frontend_blob) {
		drm_property_blob_put(cstate->frontend_blob);
		cstate->frontend_blob = NULL;
	}
	__drm_atomic_helper_plane_destroy_state(state);
	kfree(cstate);
}

static void sunxi_atomic_plane_reset(struct drm_plane *plane)
{
	struct display_channel_state *state;
	int i;

	if (plane->state) {
		state = to_display_channel_state(plane->state);
		sunxi_atomic_plane_destroy_state(plane, plane->state);
	} else {
		kfree(state);
	}
	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state) {
		DRM_ERROR("%s fail, no mem\n", __FUNCTION__);
		return;
	}

	/* prepare for pqd */
	state->frontend_blob = drm_property_create_blob(plane->dev, sizeof(struct de_frontend_data), NULL);

	__drm_atomic_helper_plane_reset(plane, &state->base);
	for (i = 0; i < MAX_LAYER_NUM_PER_CHN - 1; i++) {
		state->alpha[i] = DRM_BLEND_ALPHA_OPAQUE;
		state->pixel_blend_mode[i] = DRM_MODE_BLEND_PREMULTI;
	}
	state->eotf = DE_EOTF_BT709;
	state->color_space = DE_COLOR_SPACE_BT709;
	state->color_range = DE_COLOR_RANGE_DEFAULT;
	state->fake_layer0 = false;
	state->layer_id = COMMIT_ALL_LAYER;
}

static bool sunxi_plane_format_mod_supported(struct drm_plane *plane, u32 format, u64 modifier)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct sunxi_drm_crtc *scrtc = sunxi_plane->crtc;
	return sunxi_de_format_mod_supported(scrtc->sunxi_de, sunxi_plane->hdl, format, modifier);
}

void sunxi_plane_print_state(struct drm_printer *p,
				   const struct drm_plane_state *state, bool state_only)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(state->plane);
	struct sunxi_drm_crtc *scrtc = sunxi_plane->crtc;
	struct display_channel_state *cstate = to_display_channel_state(state);
	sunxi_de_dump_channel_state(p, scrtc->sunxi_de, sunxi_plane->hdl, cstate, state_only);
}

static void sunxi_plane_atomic_print_state(struct drm_printer *p,
				   const struct drm_plane_state *state)
{
	sunxi_plane_print_state(p, state, true);
}

static const struct drm_plane_funcs sunxi_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = sunxi_atomic_plane_reset,
	.atomic_duplicate_state = sunxi_atomic_plane_duplicate_state,
	.atomic_destroy_state = sunxi_atomic_plane_destroy_state,
	.atomic_set_property = sunxi_atomic_plane_set_property,
	.atomic_get_property = sunxi_atomic_plane_get_property,
	.format_mod_supported = sunxi_plane_format_mod_supported,
	.atomic_print_state = sunxi_plane_atomic_print_state,
};

static const struct drm_plane_helper_funcs sunxi_plane_helper_funcs = {
	.atomic_update = sunxi_plane_atomic_update,
	.atomic_async_check = sunxi_plane_atomic_async_check,
	.atomic_async_update = sunxi_plane_atomic_async_update,
	/* atomic_precheck is a private function ptr add by sunxi,
	 * should be used with additonal patch for kernel drm framework
	 */
#ifdef SUNXI_DRM_PLANE_ASYNC
	.atomic_precheck = sunxi_plane_atomic_precheck,
#endif
};

static int create_extra_blend_mode_prop(struct sunxi_drm_private *pri, unsigned int index,
					 unsigned int supported_modes)
{
	char name[32];
	struct drm_device *dev = &pri->base;
	struct drm_property *prop;
	static const struct drm_prop_enum_list props[] = {
		{ DRM_MODE_BLEND_PIXEL_NONE, "None" },
		{ DRM_MODE_BLEND_PREMULTI, "Pre-multiplied" },
		{ DRM_MODE_BLEND_COVERAGE, "Coverage" },
	};
	unsigned int valid_mode_mask = BIT(DRM_MODE_BLEND_PIXEL_NONE) |
				       BIT(DRM_MODE_BLEND_PREMULTI)   |
				       BIT(DRM_MODE_BLEND_COVERAGE);
	int i;

	if (WARN_ON((supported_modes & ~valid_mode_mask) ||
		    ((supported_modes & BIT(DRM_MODE_BLEND_PREMULTI)) == 0)))
		return -EINVAL;

	sprintf(name, "pixel blend mode%d", index + 1);
	prop = drm_property_create(dev, DRM_MODE_PROP_ENUM,
				   name,
				   hweight32(supported_modes));
	if (!prop)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(props); i++) {
		int ret;

		if (!(BIT(props[i].type) & supported_modes))
			continue;

		ret = drm_property_add_enum(prop, props[i].type,
					    props[i].name);

		if (ret) {
			drm_property_destroy(dev, prop);

			return ret;
		}
	}

	pri->prop_blend_mode[index] = prop;

	return 0;
}

int sunxi_drm_plane_property_create(struct sunxi_drm_private *private)
{
	struct drm_device *dev = &private->base;
	struct drm_property *prop;
	char name[32];
	int i;

	prop = drm_property_create_range(dev, 0, "layer_id",
					 0, MAX_LAYER_NUM_PER_CHN);
	if (!prop)
		return -ENOMEM;
	private->prop_layer_id = prop;

	for (i = 0; i < OVL_REMAIN; i++) {
		if (create_extra_blend_mode_prop(private, i,
						  BIT(DRM_MODE_BLEND_PIXEL_NONE) |
						  BIT(DRM_MODE_BLEND_PREMULTI) |
						  BIT(DRM_MODE_BLEND_COVERAGE)))
			return -ENOMEM;

		sprintf(name, "alpha%d", i + 1);
		prop = drm_property_create_range(dev, 0, name,
						 0, DRM_BLEND_ALPHA_OPAQUE);
		if (!prop)
			return -ENOMEM;
		private->prop_alpha[i] = prop;

		sprintf(name, "SRC_X%d", i + 1);
		prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
				name, INT_MIN, INT_MAX);
		if (!prop)
			return -ENOMEM;
		private->prop_src_x[i] = prop;

		sprintf(name, "SRC_Y%d", i + 1);
		prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
				name, INT_MIN, INT_MAX);
		if (!prop)
			return -ENOMEM;
		private->prop_src_y[i] = prop;

		sprintf(name, "SRC_W%d", i + 1);
		prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
				name, INT_MIN, INT_MAX);
		if (!prop)
			return -ENOMEM;
		private->prop_src_w[i] = prop;

		sprintf(name, "SRC_H%d", i + 1);
		prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
				name, INT_MIN, INT_MAX);
		if (!prop)
			return -ENOMEM;
		private->prop_src_h[i] = prop;

		sprintf(name, "CRTC_X%d", i + 1);
		prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
				name, INT_MIN, INT_MAX);
		if (!prop)
			return -ENOMEM;
		private->prop_crtc_x[i] = prop;

		sprintf(name, "CRTC_Y%d", i + 1);
		prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
				name, INT_MIN, INT_MAX);
		if (!prop)
			return -ENOMEM;
		private->prop_crtc_y[i] = prop;

		sprintf(name, "CRTC_W%d", i + 1);
		prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
				name, INT_MIN, INT_MAX);
		if (!prop)
			return -ENOMEM;
		private->prop_crtc_w[i] = prop;

		sprintf(name, "CRTC_H%d", i + 1);
		prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
				name, INT_MIN, INT_MAX);
		if (!prop)
			return -ENOMEM;
		private->prop_crtc_h[i] = prop;

		sprintf(name, "FB_ID%d", i + 1);
		prop = drm_property_create_object(dev, DRM_MODE_PROP_ATOMIC,
				name, DRM_MODE_OBJECT_FB);
		if (!prop)
			return -ENOMEM;
		private->prop_fb_id[i] = prop;

		sprintf(name, "COLOR");
		if (i != 0)
			sprintf(name, "COLOR%d", i);
		prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
				name, 0, 0xffffffff);
		if (!prop)
			return -ENOMEM;
		private->prop_color[i] = prop;
	}

	sprintf(name, "COLOR%d", i);
	prop = drm_property_create_signed_range(dev, DRM_MODE_PROP_ATOMIC,
			name, 0, 0xffffffff);
	if (!prop)
		return -ENOMEM;
	private->prop_color[i] = prop;

	return 0;
}

static int plane_create_feature_blob(struct sunxi_drm_plane *plane)
{
	struct sunxi_drm_private *pri = to_sunxi_drm_private(plane->plane.dev);
	struct de_channel_handle *hdl = plane->hdl;
	struct drm_property_blob *blob;
	unsigned int size = 0;
	struct de_channel_feature ch;
	struct de_channel_linebuf_feature ch_line_buf;

	memset(&ch, 0, sizeof(ch));
	memcpy(&ch.support, &hdl->mod, sizeof(ch.support));
	ch.feature_cnt = 0;
	ch.layer_cnt = plane->layer_cnt;
	ch.hw_id = plane->index;
	size += sizeof(ch);

	memset(&ch_line_buf, 0, sizeof(ch_line_buf));
	memcpy(&ch_line_buf, &hdl->lbuf, sizeof(hdl->lbuf));
	ch.feature_cnt++;
	size += sizeof(ch_line_buf);

	blob = drm_property_create_blob(plane->plane.dev, size, NULL);
	if (IS_ERR(blob))
		return -1;
	memcpy(blob->data, &ch, sizeof(ch));
	memcpy(blob->data + sizeof(ch), &ch_line_buf, sizeof(ch_line_buf));

	drm_object_attach_property(&plane->plane.base, pri->prop_feature,
				   blob->base.id);
	return 0;
}

static void sunxi_drm_plane_property_init(struct sunxi_drm_plane *plane, unsigned int channel_cnt, bool afbc_rot_support)
{
	struct sunxi_drm_private *pri = to_sunxi_drm_private(plane->plane.dev);
#if DRM_OBJECT_MAX_PROPERTY >= 57
	int i;
#endif

	drm_plane_create_alpha_property(&plane->plane);
	drm_plane_create_blend_mode_property(&plane->plane,
					BIT(DRM_MODE_BLEND_PIXEL_NONE) |
					BIT(DRM_MODE_BLEND_PREMULTI) |
					BIT(DRM_MODE_BLEND_COVERAGE));
	drm_plane_create_zpos_property(&plane->plane, plane->index, 0, channel_cnt - 1);
	if (afbc_rot_support)
		drm_plane_create_rotation_property(&plane->plane, DRM_MODE_ROTATE_0, DRM_MODE_ROTATE_0 |
						    DRM_MODE_ROTATE_90 | DRM_MODE_ROTATE_180 | DRM_MODE_ROTATE_270 |
						    DRM_MODE_REFLECT_X | DRM_MODE_REFLECT_Y);

	/* { FB_ID CRTC_X(YWH) SRC_X(YWH) COLOR "pixel blend mode" alpha} * 4 = 48
	IN_FORMATS CRTC_ID type IN_FENCE_FD zpos EOTF COLOR_SPACE COLOR_RANGE rotation =  9 */

//FIXME remove maroc
#if DRM_OBJECT_MAX_PROPERTY >= 57
	for (i = 0; i < plane->layer_cnt - 1 && plane->layer_cnt > 1; i++) {
		drm_object_attach_property(&plane->plane.base, pri->prop_blend_mode[i], DRM_MODE_BLEND_PREMULTI);
		drm_object_attach_property(&plane->plane.base, pri->prop_alpha[i], DRM_BLEND_ALPHA_OPAQUE);
		drm_object_attach_property(&plane->plane.base, pri->prop_src_x[i], 0);
		drm_object_attach_property(&plane->plane.base, pri->prop_src_y[i], 0);
		drm_object_attach_property(&plane->plane.base, pri->prop_src_w[i], 0);
		drm_object_attach_property(&plane->plane.base, pri->prop_src_h[i], 0);
		drm_object_attach_property(&plane->plane.base, pri->prop_crtc_x[i], 0);
		drm_object_attach_property(&plane->plane.base, pri->prop_crtc_y[i], 0);
		drm_object_attach_property(&plane->plane.base, pri->prop_crtc_w[i], 0);
		drm_object_attach_property(&plane->plane.base, pri->prop_crtc_h[i], 0);
		drm_object_attach_property(&plane->plane.base, pri->prop_fb_id[i], 0);
		drm_object_attach_property(&plane->plane.base, pri->prop_color[i], 0);
	}
	drm_object_attach_property(&plane->plane.base, pri->prop_color[i], 0);
#else
	drm_object_attach_property(&plane->plane.base, pri->prop_color[0], 0);
#endif
#if DRM_OBJECT_MAX_PROPERTY > 24
	drm_object_attach_property(&plane->plane.base, pri->prop_layer_id, COMMIT_ALL_LAYER);
#endif
	drm_object_attach_property(&plane->plane.base, pri->prop_frontend_data, 0);
	drm_object_attach_property(&plane->plane.base, pri->prop_eotf, 0);
	drm_object_attach_property(&plane->plane.base, pri->prop_color_space, 0);
	drm_object_attach_property(&plane->plane.base, pri->prop_color_range, 0);
	drm_object_attach_property(&plane->plane.base, pri->prop_compressed_image_crop, 0);
	plane_create_feature_blob(plane);
}

static int sunxi_drm_plane_init(struct drm_device *dev,
				struct sunxi_drm_crtc *scrtc,
				uint32_t possible_crtc,
				struct sunxi_drm_plane *plane, int type,
				unsigned int de_id, const struct sunxi_plane_info *info)
{
	plane->crtc = scrtc;
	plane->hdl = info->hdl;
	plane->index = info->index;
	plane->layer_cnt = info->layer_cnt;

	if (drm_universal_plane_init(dev, &plane->plane, possible_crtc,
				     &sunxi_plane_funcs, info->formats, info->format_count,
				     info->format_modifiers, type,
				     "plane-%d-%s(%d)", plane->index, info->name, de_id)) {
		DRM_ERROR("drm_universal_plane_init failed\n");
		return -1;
	}

	drm_plane_helper_add(&plane->plane, &sunxi_plane_helper_funcs);
	sunxi_drm_plane_property_init(plane, scrtc->plane_cnt, info->afbc_rot_support);
	return 0;
}
/* plane end*/

static void wb_finish_proc(struct sunxi_drm_crtc *scrtc)
{
	int i;
	struct sunxi_drm_wb *wb;
	unsigned long flags;
	struct wb_signal_wait *wait;
	bool signal = false;

	spin_lock_irqsave(&scrtc->wb_lock, flags);
	wb = scrtc->wb;
	spin_unlock_irqrestore(&scrtc->wb_lock, flags);
	if (!wb) {
		return;
	}

	spin_lock_irqsave(&wb->signal_lock, flags);
	for (i = 0; i < WB_SIGNAL_MAX; i++) {
		wait = &wb->signal[i];
		if (wait->active) {
			wait->vsync_cnt++;
			if (wait->vsync_cnt == 2) {
				wait->active = 0;
				wait->vsync_cnt = 0;
				signal = true;
			}
		}
	}
	spin_unlock_irqrestore(&wb->signal_lock, flags);
	if (signal)
		drm_writeback_signal_completion(&wb->wb_connector, 0);
}

static int sunxi_wb_connector_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_mode_config *config = &dev->mode_config;
	struct drm_crtc *crtc;
	unsigned int cnt = 0;
	struct drm_display_mode *mode;

	list_for_each_entry(crtc, &config->crtc_list, head) {
		mode = NULL;
		if (crtc->state) {
			mode = drm_mode_duplicate(dev, &crtc->state->mode);
			if (mode) {
				drm_mode_probed_add(connector, mode);
				cnt++;
			}
		}
	}

	cnt += drm_add_modes_noedid(connector, dev->mode_config.max_width,
				    dev->mode_config.max_height);
	return cnt;
}

static void commit_new_wb_job(struct sunxi_drm_crtc *scrtc, struct sunxi_drm_wb *wb)
{
	int i;
	unsigned long flags;
	bool found = false;
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(scrtc->crtc.state);


	DRM_INFO("[SUNXI-DE] %s start\n", __FUNCTION__);
	/* find a free signal slot */
	spin_lock_irqsave(&wb->signal_lock, flags);
	for (i = 0; i < WB_SIGNAL_MAX; i++) {
		if (wb->signal[i].active == false) {
			DRM_DEBUG_DRIVER("[SUNXI-DE] set wb for crtc\n");
			wb->signal[i].active = true;
			found = true;
			break;
		}
	}
	spin_unlock_irqrestore(&wb->signal_lock, flags);

	/* add wb for isr to signal wb job */
	WARN(!found, "no free wb active signal slot\n");
	spin_lock_irqsave(&scrtc->wb_lock, flags);
	scrtc_state->wb = NULL;
	scrtc->wb = wb;
	spin_unlock_irqrestore(&scrtc->wb_lock, flags);
	sunxi_de_write_back(scrtc->sunxi_de, wb->hw_wb, wb->wb_connector.base.state->writeback_job->fb);
	drm_writeback_queue_job(&wb->wb_connector, wb->wb_connector.base.state);
}

static void disable_and_reset_wb(struct sunxi_drm_crtc *scrtc)
{
	bool all_finish = true;
	struct sunxi_drm_wb *wb = scrtc->wb;
	unsigned long flags;
	int i;

	if (wb) {
		/* check if all jobs finsh */
		spin_lock_irqsave(&wb->signal_lock, flags);
		for (i = 0; i < WB_SIGNAL_MAX; i++) {
			if (wb->signal[i].active == true) {
				all_finish = false;
				break;
			}
		}
		spin_unlock_irqrestore(&wb->signal_lock, flags);

		/* disable wb if all jobs finish  */
		if (all_finish) {
			DRM_DEBUG_DRIVER("[SUNXI-DE] rm wb for crtc\n");
			spin_lock_irqsave(&scrtc->wb_lock, flags);
			scrtc->wb = NULL;
			spin_unlock_irqrestore(&scrtc->wb_lock, flags);
			sunxi_de_write_back(scrtc->sunxi_de, wb->hw_wb, NULL);
		}
	}
}

static void sunxi_wb_commit(struct sunxi_drm_crtc *scrtc)
{
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(scrtc->crtc.state);
	struct sunxi_drm_wb *wb = scrtc_state->wb;
	struct drm_writeback_connector *wb_conn = wb ? &wb->wb_connector : NULL;
	struct drm_connector_state *conn_state = wb_conn ? wb_conn->base.state : NULL;
	struct drm_writeback_job *job = conn_state ? conn_state->writeback_job : NULL;
	struct drm_framebuffer *fb = job ? job->fb : NULL;

	if (fb)
		commit_new_wb_job(scrtc, wb);
	else
		disable_and_reset_wb(scrtc);
}

static int sunxi_wb_encoder_atomic_check(struct drm_encoder *encoder,
			       struct drm_crtc_state *crtc_state,
			       struct drm_connector_state *conn_state)

{
	int i, ret = 0;
	unsigned long flags;
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc_state);
	struct sunxi_drm_wb *wb = container_of(encoder, struct sunxi_drm_wb, wb_connector.encoder);
	if (!crtc_state->active) {
		DRM_ERROR("[SUNXI-DE] wb check fail, crtc is not enabled %s %d \n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	spin_lock_irqsave(&wb->signal_lock, flags);
	for (i = 0; i < WB_SIGNAL_MAX; i++) {
		if (wb->signal[i].active) {
			//ret = -EBUSY;
			DRM_ERROR("[SUNXI-DE] wb check fail, pending wb not finish %s %d \n", __FUNCTION__, __LINE__);
		}
	}
	spin_unlock_irqrestore(&wb->signal_lock, flags);
	/* user should make sure not to switch connector and request wb on the same commit*/
	if (crtc_state->mode_changed || crtc_state->connectors_changed) {
		crtc_state->mode_changed = false;
		crtc_state->connectors_changed = false;
		DRM_INFO("[SUNXI-DE] skip mode change and connector change for wb enable %s %d \n", __FUNCTION__, __LINE__);
	}
	if (!ret) {
		scrtc_state->wb = wb;
		DRM_DEBUG_DRIVER("[SUNXI-DE] %s add new fb\n", __func__);
	}
	return ret;
}

static const struct drm_connector_helper_funcs sunxi_wb_connector_helper_funcs = {
	.get_modes = sunxi_wb_connector_get_modes,
};

static const struct drm_connector_funcs sunxi_wb_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct drm_encoder_helper_funcs sunxi_wb_encoder_helper_funcs = {
	.atomic_check = sunxi_wb_encoder_atomic_check,
};

struct sunxi_drm_wb *sunxi_drm_wb_init_one(struct sunxi_de_wb_info *wb_info)
{
	int ret;
	struct drm_device *drm = wb_info->drm;
	struct sunxi_drm_wb *wb = devm_kzalloc(drm->dev, sizeof(*wb), GFP_KERNEL);
	static const u32 formats_wb[] = {
		DRM_FORMAT_ARGB8888,//TODO add more format base on hw feat
	};

	if (!wb) {
		DRM_ERROR("allocate memory for drm_wb fail\n");
		return ERR_PTR(-ENOMEM);
	}
	wb->hw_wb = wb_info->wb;
	spin_lock_init(&wb->signal_lock);
	wb->wb_connector.encoder.possible_crtcs = wb_info->support_disp_mask;
	drm_connector_helper_add(&wb->wb_connector.base, &sunxi_wb_connector_helper_funcs);

	ret = drm_writeback_connector_init(drm, &wb->wb_connector,
					   &sunxi_wb_connector_funcs,
					   &sunxi_wb_encoder_helper_funcs,
					   formats_wb, 1
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
		);
#else
		, wb_info->support_disp_mask);
#endif
	if (ret) {
		DRM_ERROR("writeback connector init failed\n");
		return NULL;
	} else
		return wb;
}

void sunxi_drm_wb_destory(struct sunxi_drm_wb *wb)
{
//TODO
}

static void sunxi_crtc_finish_page_flip(struct drm_device *dev,
					struct sunxi_drm_crtc *scrtc)
{
	unsigned long flags;

	/* send the vblank of drm_crtc_state->event */
	spin_lock_irqsave(&dev->event_lock, flags);
	if (scrtc->event && scrtc->should_send_vblank) {
		drm_crtc_send_vblank_event(&scrtc->crtc, scrtc->event);
		scrtc->vblank_trace++;
		SUNXIDRM_TRACE_INT2("crtc-vblank", scrtc->hw_id, scrtc->vblank_trace & 1);
		scrtc->event = NULL;
		scrtc->should_send_vblank = 0;
	}
	spin_unlock_irqrestore(&dev->event_lock, flags);
}

irqreturn_t sunxi_crtc_event_proc(int irq, void *crtc)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	bool timeout;
	bool busy = sunxi_de_query_de_busy(scrtc->sunxi_de);

	scrtc->irqcnt++;
	if (scrtc->check_status(scrtc->output_dev_data)) {
		scrtc->fifo_err++;
		SUNXIDRM_TRACE_INT2("crtc-ERR", scrtc->hw_id, scrtc->fifo_err & 1);
	}

	SUNXIDRM_TRACE_INT2("crtc-irq", scrtc->hw_id, scrtc->irqcnt & 1);
	SUNXIDRM_TRACE_INT2("crtc-busy", scrtc->hw_id, busy);

	timeout = !scrtc->is_sync_time_enough(scrtc->output_dev_data);
	sunxi_de_event_proc(scrtc->sunxi_de, timeout);

	wb_finish_proc(scrtc);
	/* vblank common process */
	drm_crtc_handle_vblank(&scrtc->crtc);

	if (!busy) {
		/*
		 * Ideally, the vsync interrupt should be processed during the vsync blanking area (where DE-busy is 0);
		 * however, due to system interrupt preemption or scheduling reasons,
		 * the vsync interrupt may be delayed to the active region of frame n+1.
		 *
		 * We need to identify these cases and prevent them from incorrectly triggering the fence of frame n+1.
		 */
		sunxi_crtc_finish_page_flip(scrtc->crtc.dev, scrtc);
	}
	return IRQ_HANDLED;
}

static int sunxi_crtc_atomic_set_property(struct drm_crtc *crtc,
					 struct drm_crtc_state *state,
					 struct drm_property *property,
					 uint64_t val)
{
	struct drm_device *drm = crtc->dev;
	bool replaced;
	struct sunxi_drm_private *priv = to_sunxi_drm_private(drm);
	struct drm_mode_config *config = &drm->mode_config;
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(state);
	int ret;

	if (property == config->tv_brightness_property) {
		scrtc_state->excfg.brightness = val;
		scrtc_state->bcsh_changed = true;
		return 0;
	} else if (property == config->tv_contrast_property) {
		scrtc_state->excfg.contrast = val;
		scrtc_state->bcsh_changed = true;
		return 0;
	} else if (property == config->tv_saturation_property) {
		scrtc_state->excfg.saturation = val;
		scrtc_state->bcsh_changed = true;
		return 0;
	} else if (property == config->tv_hue_property) {
		scrtc_state->excfg.hue = val;
		scrtc_state->bcsh_changed = true;
		return 0;
	} else if (property == priv->prop_backend_data) {
		return sunxi_replace_property_blob_from_id(drm,
				&scrtc_state->backend_blob,
				val, sizeof(struct de_backend_data),
				&replaced);
	} else if (property == priv->prop_sunxi_ctm) {
		ret = sunxi_replace_property_blob_from_id(drm,
				&scrtc_state->sunxi_ctm,
				val, sizeof(struct de_color_ctm),
				&replaced);
		scrtc_state->sunxi_ctm_changed |= replaced;
		return ret;
	} else if (property == priv->prop_frame_rate_change) {
		scrtc_state->frame_rate_change = val;
		DRM_DEBUG_DRIVER("[SUNXI-DE] set frame_rate_change for VRR\n");
		return 0;
	}

	return -EINVAL;
}

static int sunxi_crtc_atomic_get_property(struct drm_crtc *crtc,
					 const struct drm_crtc_state *state,
					 struct drm_property *property,
					 uint64_t *val)
{
	struct drm_device *drm = crtc->dev;
	struct sunxi_drm_private *priv = to_sunxi_drm_private(drm);
	struct drm_mode_config *config = &drm->mode_config;
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(state);

	if (property == config->tv_brightness_property) {
		*val = scrtc_state->excfg.brightness;
		return 0;
	} else if (property == config->tv_contrast_property) {
		*val = scrtc_state->excfg.contrast;
		return 0;
	} else if (property == config->tv_saturation_property) {
		*val = scrtc_state->excfg.saturation;
		return 0;
	} else if (property == config->tv_hue_property) {
		*val = scrtc_state->excfg.hue;
		return 0;
	} else if (property == priv->prop_backend_data) {
		*val = (scrtc_state->backend_blob) ? scrtc_state->backend_blob->base.id : 0;
		return 0;
	} else if (property == priv->prop_sunxi_ctm) {
		*val = (scrtc_state->sunxi_ctm) ? scrtc_state->sunxi_ctm->base.id : 0;
		return 0;
	} else if (property == priv->prop_frame_rate_change) {
		*val = false;
		return 0;
	}
	return -EINVAL;
}

static struct drm_crtc_state *sunxi_crtc_duplicate_state(struct drm_crtc *crtc)
{
	struct sunxi_crtc_state *state, *cur;

	if (WARN_ON(!crtc->state))
		return NULL;

	cur = to_sunxi_crtc_state(crtc->state);
	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return NULL;
	memcpy(state, cur, sizeof(*state));

	if (cur->backend_blob) {
		drm_property_blob_get(cur->backend_blob);
	}
	if (cur->sunxi_ctm) {
		drm_property_blob_get(cur->sunxi_ctm);
	}

	// frame_rate_change should be reset on next commit
	state->frame_rate_change = false;

	__drm_atomic_helper_crtc_duplicate_state(crtc, &state->base);
	return &state->base;
}

static void sunxi_crtc_destroy_state(struct drm_crtc *crtc,
				     struct drm_crtc_state *s)
{
	struct sunxi_crtc_state *state;

	state = to_sunxi_crtc_state(s);

	if (state->backend_blob) {
		drm_property_blob_put(state->backend_blob);
	}
	if (state->sunxi_ctm) {
		drm_property_blob_put(state->sunxi_ctm);
	}

	__drm_atomic_helper_crtc_destroy_state(s);
	kfree(state);
}

static void sunxi_crtc_reset(struct drm_crtc *crtc)
{
	struct sunxi_crtc_state *state = kzalloc(sizeof(*state), GFP_KERNEL);
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);

	if (crtc->state)
		sunxi_crtc_destroy_state(crtc, crtc->state);
	state->px_fmt_space = DE_FORMAT_SPACE_RGB;
	state->yuv_sampling = DE_YUV444;
	state->eotf = DE_EOTF_BT709;
	state->color_space = DE_COLOR_SPACE_BT709;
	state->color_range = DE_COLOR_RANGE_0_255;
	state->data_bits = DE_DATA_8BITS;
	state->clk_freq = scrtc->clk_freq;
	state->excfg.brightness = 50;
	state->excfg.contrast = 50;
	state->excfg.saturation = 50;
	state->excfg.hue = scrtc->hue_default_value;

	/* prepare for pqd */
	state->backend_blob = drm_property_create_blob(crtc->dev, sizeof(struct de_backend_data), NULL);

	__drm_atomic_helper_crtc_reset(crtc, &state->base);
}

void sunxi_crtc_atomic_print_state(struct drm_printer *p,
				   const struct drm_crtc_state *state)
{
	unsigned long flags;
	struct sunxi_drm_wb *wb;
	struct sunxi_crtc_state *cstate = to_sunxi_crtc_state(state);
	struct sunxi_drm_crtc *scrtc = (struct sunxi_drm_crtc *)state->crtc;
	int w = state->mode.hdisplay;
	int h = state->mode.vdisplay;
	int fps = drm_mode_vrefresh(&state->mode);

	drm_printf(p, "\t%s", scrtc->enabled ? "on: " : "off\n");
	if (scrtc->enabled) {
		drm_printf(p, "%dx%d@%d&%dMhz->tcon%d irqcnt=%d err=%d\n", w, h, fps,
			    (int)(scrtc->clk_freq / 1000000), cstate->tcon_id, scrtc->irqcnt, scrtc->fifo_err);
		drm_printf(p, "\t    format_space: %d yuv_sampling: %d eotf:%d cs: %d"
			    " color_range: %d data_bits: %d\n", cstate->px_fmt_space,
			    cstate->yuv_sampling, cstate->eotf, cstate->color_space,
			    cstate->color_range, cstate->data_bits);

		sunxi_de_dump_state(p, scrtc->sunxi_de);

		spin_lock_irqsave(&scrtc->wb_lock, flags);
		wb = scrtc->wb;
		if (!wb) {
			drm_printf(p, "\twb off\n");
		} else {
			drm_printf(p, "\twb on:\n\t\t[0]: %s %d\n\t\t[1]: %s %d\n",
				    wb->signal[0].active ? "waiting" : "finish", wb->signal[0].vsync_cnt,
				    wb->signal[1].active ? "waiting" : "finish", wb->signal[1].vsync_cnt);
		}
		spin_unlock_irqrestore(&scrtc->wb_lock, flags);
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static void sunxi_crtc_atomic_enable(struct drm_crtc *crtc,
				      struct drm_crtc_state *old_state)

#else
static void sunxi_crtc_atomic_enable(struct drm_crtc *crtc,
				     struct drm_atomic_state *state)
#endif
{
	//struct sunxi_drm_private *pri = to_sunxi_drm_private(crtc->dev);
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct drm_crtc_state *new_state = crtc->state;
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(new_state);
	struct drm_mode_modeinfo modeinfo;
	struct sunxi_de_out_cfg cfg;
	bool sw_enable = scrtc_state->sw_enable;

	if (scrtc_state->frame_rate_change) {
		drm_crtc_vblank_on(crtc);
		return;
	}

	DRM_INFO("[SUNXI-CRTC]%s\n", __func__);
	if (scrtc->enabled) {
		DRM_INFO("crtc has been enable, no need to enable again\n");
		return;
	}

	if ((!new_state->enable) || (!new_state->active)) {
		DRM_INFO("Warn: DRM do NOT want to enable or active crtc%d,"
			 " so can NOT be enabled\n",
			 scrtc->crtc.index);
		return;
	}

	SUNXIDRM_TRACE_BEGIN(__func__);

	scrtc->enable_vblank = scrtc_state->enable_vblank;
	scrtc->check_status = scrtc_state->check_status;
	scrtc->is_sync_time_enough = scrtc_state->is_sync_time_enough;
	scrtc->get_cur_line = scrtc_state->get_cur_line;
	scrtc->is_support_backlight = scrtc_state->is_support_backlight;
	scrtc->get_backlight_value = scrtc_state->get_backlight_value;
	scrtc->set_backlight_value = scrtc_state->set_backlight_value;
	scrtc->output_dev_data = scrtc_state->output_dev_data;

	drm_property_blob_get(new_state->mode_blob);
	memcpy(&modeinfo, new_state->mode_blob->data,
	       new_state->mode_blob->length);
	drm_property_blob_put(new_state->mode_blob);
	memset(&cfg, 0, sizeof(cfg));
	cfg.sw_enable = sw_enable;
	cfg.hwdev_index = scrtc_state->tcon_id;
	cfg.width = modeinfo.hdisplay;
	cfg.height = modeinfo.vdisplay;
	cfg.device_fps = modeinfo.vrefresh;
	cfg.max_device_fps = sunxi_drm_get_device_max_fps(crtc->dev);
	cfg.kHZ_pixelclk = modeinfo.clock;
	cfg.htotal = modeinfo.htotal;
	cfg.vtotal = modeinfo.vtotal;
	cfg.interlaced = !!(modeinfo.flags & DRM_MODE_FLAG_INTERLACE);
	cfg.px_fmt_space = scrtc_state->px_fmt_space;
	cfg.yuv_sampling = scrtc_state->yuv_sampling;
	cfg.eotf = scrtc_state->eotf;
	cfg.color_space = scrtc_state->color_space;
	cfg.color_range = scrtc_state->color_range;
	cfg.data_bits = scrtc_state->data_bits;

	if ((scrtc_state->pixel_mode != 0) && (scrtc_state->pixel_mode != 1)
	    && (scrtc_state->pixel_mode != 2) && (scrtc_state->pixel_mode != 4)
	    && (scrtc_state->pixel_mode != 8)) {
		DRM_ERROR("pixel_mode set for crtc is not support:%d, use default 1 pixel mode\n",
			  scrtc_state->pixel_mode);
		cfg.pixel_mode = 1;
	} else {
		cfg.pixel_mode = scrtc_state->pixel_mode;
	}

	if (sunxi_de_enable(scrtc->sunxi_de, &cfg) < 0)
		DRM_ERROR("sunxi_de_enable failed\n");

	scrtc->enabled = true;
	drm_crtc_vblank_on(crtc);
	if (sw_enable)
		sunxi_drm_signal_sw_enable_done(crtc);
	SUNXIDRM_TRACE_END(__func__);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static void sunxi_crtc_atomic_disable(struct drm_crtc *crtc,
				      struct drm_crtc_state *old_state)

#else
static void sunxi_crtc_atomic_disable(struct drm_crtc *crtc,
				      struct drm_atomic_state *state)
#endif

{
	unsigned long flags;
	struct sunxi_drm_wb *wb;
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc->state);

	drm_crtc_vblank_off(crtc);

	if (scrtc_state->frame_rate_change) {
		DRM_DEBUG_KMS("%s: skip disable for VRR\n", __func__);
		return;
	}

	DRM_INFO("[SUNXI-CRTC]%s\n", __func__);
	if (!scrtc->enabled) {
		DRM_ERROR("%s: crtc has been disabled\n", __func__);
		return;
	}

	/* remove not finish wb  */
	spin_lock_irqsave(&scrtc->wb_lock, flags);
	wb = scrtc->wb ? scrtc->wb : NULL;
	scrtc->wb = NULL;
	spin_unlock_irqrestore(&scrtc->wb_lock, flags);

//clear wb signal ??? drm_writeback_signal_completion??
	if (wb) {
		sunxi_de_write_back(scrtc->sunxi_de, wb->hw_wb, NULL);
	}

	scrtc->enabled = false;
	sunxi_de_disable(scrtc->sunxi_de);

	if (crtc->state->event && !crtc->state->active) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		scrtc->vblank_trace++;
		SUNXIDRM_TRACE_INT2("crtc-vblank", scrtc->hw_id, scrtc->vblank_trace & 1);
		spin_unlock_irq(&crtc->dev->event_lock);
		crtc->state->event = NULL;
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static void sunxi_crtc_atomic_begin(struct drm_crtc *crtc,
				      struct drm_crtc_state *old_state)

#else
static void sunxi_crtc_atomic_begin(struct drm_crtc *crtc,
			     struct drm_atomic_state *state)
#endif
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct drm_device *dev = crtc->dev;
	unsigned long flags;
	struct drm_plane *plane;
	struct sunxi_drm_plane *sunxi_plane;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	SUNXIDRM_TRACE_BEGIN(__func__);
	if (crtc->state->event) {
		drm_crtc_vblank_get(crtc);
		spin_lock_irqsave(&dev->event_lock, flags);
		scrtc->event = crtc->state->event;
		spin_unlock_irqrestore(&dev->event_lock, flags);
		crtc->state->event = NULL;
	}
	drm_atomic_crtc_for_each_plane(plane, crtc) {
		sunxi_plane = to_sunxi_plane(plane);
		if (sunxi_plane->index == scrtc->fbdev_chn_id) {
			if (plane->state && plane->state->fb)
				scrtc->fbdev_output = false;
			break;
		}
	}

	sunxi_de_atomic_begin(scrtc->sunxi_de);
	SUNXIDRM_TRACE_END(__func__);

    sunxidrm_debug_trace_begin(scrtc->hw_id);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static void sunxi_crtc_atomic_flush(struct drm_crtc *crtc,
				    struct drm_crtc_state *old_state)
#else
static void sunxi_crtc_atomic_flush(struct drm_crtc *crtc,
				    struct drm_atomic_state *state)
#endif
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct drm_crtc_state *new_state = crtc->state;
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(new_state);
	struct sunxi_de_flush_cfg cfg;
	void *backend_data;
	bool all_dirty = crtc->state->mode_changed || (crtc->state->active_changed && crtc->state->active);
	SUNXIDRM_TRACE_BEGIN(__func__);
	sunxi_wb_commit(scrtc);
	memset(&cfg, 0, sizeof(cfg));
	if (all_dirty || crtc->state->color_mgmt_changed) {
		if (crtc->state->gamma_lut) {
			cfg.gamma_lut = crtc->state->gamma_lut->data;
			cfg.gamma_dirty = true;
		}
	}

	if (all_dirty || scrtc_state->sunxi_ctm_changed) {
		if (scrtc_state->sunxi_ctm) {
			cfg.ctm = scrtc_state->sunxi_ctm->data;
			cfg.ctm_dirty = true;
			scrtc_state->sunxi_ctm_changed = false;
		}
	}

	if (all_dirty || scrtc_state->bcsh_changed) {
		cfg.brightness = scrtc_state->excfg.brightness;
		cfg.contrast = scrtc_state->excfg.contrast;
		cfg.saturation = scrtc_state->excfg.saturation;
		cfg.hue = scrtc_state->excfg.hue;
		cfg.bcsh_dirty = true;
		scrtc_state->bcsh_changed = false;
	}

	backend_data = scrtc_state->backend_blob ? scrtc_state->backend_blob->data : NULL;
	sunxi_de_atomic_flush(scrtc->sunxi_de, backend_data, &cfg);
	if (scrtc_state->atomic_flush)
		scrtc_state->atomic_flush(scrtc_state->output_dev_data);


	/*
	 * Ideally, the page_flip should be called within the vsync interrupt;
	 * but if the interrupt is delayed, we perform the page_flip after the rcq_finished
	 * to ensure that the fence of the current frame is signaled !
	 */
	sunxi_crtc_finish_page_flip(crtc->dev, scrtc);

	if (scrtc->fbdev_flush_pending) {
		scrtc->fbdev_flush_pending = false;
		complete_all(&scrtc->flush_done);
	}

	if (scrtc->async_update_flush_pending) {
		scrtc->async_update_flush_pending = false;
	}

	SUNXIDRM_TRACE_END(__func__);
	DRM_DEBUG_DRIVER("%s finish\n", __func__);
}

static int sunxi_drm_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);

	DRM_DEBUG_DRIVER("%s\n", __func__);
	if (scrtc->enable_vblank == NULL) {
		DRM_ERROR("enable vblank is not registerd!\n");
		return -1;
	}
	scrtc->enable_vblank(true, scrtc->output_dev_data);
	return 0;
}

static void sunxi_drm_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);

	DRM_DEBUG_DRIVER("%s\n", __func__);
	if (scrtc->enable_vblank == NULL) {
		DRM_ERROR("enable vblank is not registerd!\n");
		return;
	}
	scrtc->enable_vblank(false, scrtc->output_dev_data);
}

s32 commit_task_thread(void *parg)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(parg);
	struct drm_device *dev = scrtc->crtc.dev;
	if (!parg) {
		DRM_ERROR("NUll ndl\n");
		return -1;
	}

	while (1) {
		if (kthread_should_stop())
			break;

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(nsecs_to_jiffies(16666666));

		drm_modeset_lock_all(dev);
		if (scrtc->fbdev_flush_pending || scrtc->async_update_flush_pending) {
			DRM_DEBUG_DRIVER("[SUNXI-DE] thread flush fbdev:%d async%d\n",
					  scrtc->fbdev_flush_pending,
					  scrtc->async_update_flush_pending);

			sunxi_de_atomic_flush(scrtc->sunxi_de, NULL, NULL);
			if (scrtc->fbdev_flush_pending) {
				scrtc->fbdev_flush_pending = false;
				complete_all(&scrtc->flush_done);
			}
			if (scrtc->async_update_flush_pending) {
				scrtc->async_update_flush_pending = false;
			}
		}
		drm_modeset_unlock_all(dev);
	}
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static int sunxi_drm_crtc_atomic_check(struct drm_crtc *crtc,
				      struct drm_crtc_state *state)
{
	struct drm_crtc_state *crtc_state = state;
	struct drm_atomic_state *atomic_state = state->state;
#else

static int sunxi_drm_crtc_atomic_check(struct drm_crtc *crtc,
				       struct drm_atomic_state *state)
{
	struct drm_crtc_state *crtc_state =
		drm_atomic_get_new_crtc_state(state, crtc);
	struct drm_atomic_state *atomic_state = state;
#endif
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc_state);
	struct drm_plane *plane;
	struct drm_plane_state *new_plane_state;
	struct display_channel_state *cstate;
	uint32_t src_w, src_h;
	uint32_t scaler_en_num = 0;
	uint32_t ovl_en = 0, fill_color_en = 0;
	int i, j;

	if (crtc_state->enable && (!scrtc_state->output_dev_data ||
	    !scrtc_state->enable_vblank ||
	    !scrtc_state->check_status ||
	    !scrtc_state->get_cur_line)) {
		DRM_ERROR("invalid output device info\n");
		return -EINVAL;
	}
	if (scrtc->gamma_dirty == true) {
		scrtc->gamma_dirty = false;
		crtc_state->color_mgmt_changed = true;
	}

	if (scrtc->share_scaler) {
		for_each_new_plane_in_state(atomic_state, plane, new_plane_state, i) {
			/* force solid color to not use scaler in scaler share hardware */
			cstate = to_display_channel_state(new_plane_state);
			ovl_en = fill_color_en = 0;
			for (j = 0; j < OVL_MAX; j++) {
				if ((j == 0 && !new_plane_state->fb) ||
				    (j != 0 && !cstate->fb[j - 1]))
					continue;

				if (!cstate->color[j]) {
					ovl_en++;
					continue;
				}

				if (j == 0) {
					new_plane_state->src_x = 0;
					new_plane_state->src_y = 0;
					new_plane_state->src_w = new_plane_state->crtc_w << 16;
					new_plane_state->src_h = new_plane_state->crtc_h << 16;
				} else {
					cstate->src_x[j - 1] = 0;
					cstate->src_y[j - 1] = 0;
					cstate->src_w[j - 1] = cstate->crtc_w[j - 1] << 16;
					cstate->src_h[j - 1] = cstate->crtc_h[j - 1] << 16;
				}
				fill_color_en++;
			}

			/* solid color is forced to not use scaler,
			 * which may conflict with other layers with scaler.
			 */
			if (ovl_en && fill_color_en)
				DRM_INFO("[SUNXI-CRTC:%d:%s] not suitable for layer + solid color in the same ovl\n"
					 , crtc->base.id, crtc->name);

			if (fill_color_en)
				continue;

			src_w = new_plane_state->src_w >> 16;
			src_h = new_plane_state->src_h >> 16;
			if ((new_plane_state->crtc && new_plane_state->crtc == crtc) &&
			    (new_plane_state->fb && (src_w != new_plane_state->crtc_w ||
			     src_h != new_plane_state->crtc_h))) {
				scaler_en_num++;
			}
		}
	}
	if (scaler_en_num > 1) {
		DRM_ERROR("[SUNXI-CRTC:%d:%s] cannot set up more than one scaled "
			  "plane on shared scaler hardware.\n", crtc->base.id, crtc->name);
		return -EINVAL;
	}

	return 0;
}

static const struct drm_crtc_funcs sunxi_crtc_funcs = {
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.destroy = drm_crtc_cleanup,
	.reset = sunxi_crtc_reset,
	.atomic_print_state = sunxi_crtc_atomic_print_state,
	.atomic_duplicate_state = sunxi_crtc_duplicate_state,
	.atomic_destroy_state = sunxi_crtc_destroy_state,
	.atomic_get_property = sunxi_crtc_atomic_get_property,
	.atomic_set_property = sunxi_crtc_atomic_set_property,
	.enable_vblank = sunxi_drm_crtc_enable_vblank,
	.disable_vblank = sunxi_drm_crtc_disable_vblank,
};

static const struct drm_crtc_helper_funcs sunxi_crtc_helper_funcs = {
	.atomic_enable = sunxi_crtc_atomic_enable,
	.atomic_disable = sunxi_crtc_atomic_disable,
	.atomic_check = sunxi_drm_crtc_atomic_check,
	.atomic_begin = sunxi_crtc_atomic_begin,
	.atomic_flush = sunxi_crtc_atomic_flush,
};

int sunxi_drm_crtc_get_hw_id(struct drm_crtc *crtc)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	if (!crtc || !scrtc) {
		DRM_ERROR("crtc is NULL\n");
		return -EINVAL;
	}
	return scrtc->hw_id;
}

unsigned int sunxi_drm_crtc_get_clk_freq(struct drm_crtc *crtc)
{
	struct sunxi_drm_crtc *scrtc = to_sunxi_crtc(crtc);
	if (!crtc || !scrtc) {
		DRM_ERROR("crtc is NULL\n");
		return 0;
	}
	return scrtc->clk_freq;
}

static int crtc_create_feature_blob(struct sunxi_drm_crtc *crtc, const struct de_disp_feature *feat)
{
	struct sunxi_drm_private *pri = to_sunxi_drm_private(crtc->crtc.dev);
	struct drm_property_blob *blob;
	unsigned int size = 0;
	struct de_disp_feature feat_prop;

	memset(&feat_prop, 0, sizeof(feat_prop));
	memcpy(&feat_prop, feat, sizeof(*feat));
	feat_prop.feature_cnt = 0;
	feat_prop.hw_id = crtc->hw_id;
	size += sizeof(feat_prop);

	blob = drm_property_create_blob(crtc->crtc.dev, size, NULL);
	if (IS_ERR(blob)) {
		DRM_ERROR("crtc feature blob create fail\n");
		return -1;
	}
	memcpy(blob->data, &feat_prop, sizeof(feat_prop));

	drm_object_attach_property(&crtc->crtc.base, pri->prop_feature,
				   blob->base.id);
	return 0;
}

static void sunxi_drm_crtc_property_init(struct sunxi_drm_crtc *crtc, const struct de_disp_feature *feat,
					 int hue_default_value)
{
	struct sunxi_drm_private *pri = to_sunxi_drm_private(crtc->crtc.dev);
	struct drm_device *drm = crtc->crtc.dev;
	struct drm_mode_config *conf = &drm->mode_config;
	/* although for drm core bcsh is consider as connector's prop, we add
	 *  them for crtc, add handle them ourself.
	 */
	drm_object_attach_property(&crtc->crtc.base, conf->tv_brightness_property, 50);
	drm_object_attach_property(&crtc->crtc.base, conf->tv_contrast_property, 50);
	drm_object_attach_property(&crtc->crtc.base, conf->tv_saturation_property, 50);
	drm_object_attach_property(&crtc->crtc.base, conf->tv_hue_property, hue_default_value);
	drm_object_attach_property(&crtc->crtc.base, pri->prop_sunxi_ctm, 0);
	drm_object_attach_property(&crtc->crtc.base, pri->prop_backend_data, 0);
	drm_object_attach_property(&crtc->crtc.base, pri->prop_frame_rate_change, 0);
	crtc_create_feature_blob(crtc, feat);
}

void sunxi_drm_crtc_prepare_vblank_event(struct sunxi_drm_crtc *scrtc)
{
	unsigned long flags;
	struct drm_device *drmdev = scrtc->crtc.dev;

	spin_lock_irqsave(&drmdev->event_lock, flags);
	scrtc->should_send_vblank = 1;
	spin_unlock_irqrestore(&drmdev->event_lock, flags);
}

void sunxi_drm_crtc_wait_one_vblank(struct sunxi_drm_crtc *scrtc)
{
	drm_crtc_wait_one_vblank(&scrtc->crtc);
}

int sunxi_drm_crtc_get_output_current_line(struct sunxi_drm_crtc *scrtc)
{
	if (!scrtc) {
		DRM_ERROR("crtc is NULL\n");
		return -EINVAL;
	}
	return scrtc->get_cur_line(scrtc->output_dev_data);
}

bool sunxi_drm_crtc_is_support_backlight(struct sunxi_drm_crtc *scrtc)
{
	if (!scrtc) {
		DRM_ERROR("crtc is NULL\n");
		return -EINVAL;
	}

	return scrtc->is_support_backlight ? scrtc->is_support_backlight(scrtc->output_dev_data) : false;
}

int sunxi_drm_crtc_get_backlight(struct sunxi_drm_crtc *scrtc)
{
	if (!scrtc) {
		DRM_ERROR("crtc is NULL\n");
		return false;
	}
	return scrtc->get_backlight_value ? scrtc->get_backlight_value(scrtc->output_dev_data) : 0;
}

void sunxi_drm_crtc_set_backlight_value(struct sunxi_drm_crtc *scrtc, int backlight)
{
	if (!scrtc) {
		DRM_ERROR("crtc is NULL\n");
		return ;
	}
	if (scrtc->set_backlight_value)
		scrtc->set_backlight_value(scrtc->output_dev_data, backlight);
}

struct sunxi_drm_crtc *sunxi_drm_crtc_init_one(struct sunxi_de_info *info)
{
	struct sunxi_drm_crtc *scrtc;
	struct drm_device *drm = info->drm;
	const struct sunxi_plane_info *plane = NULL;
	int i, ret;
	int primary_cnt = 0;
	int primary_index = 0;

	scrtc = devm_kzalloc(drm->dev, sizeof(*scrtc), GFP_KERNEL);
	if (!scrtc) {
		DRM_ERROR("allocate memory for sunxi_crtc fail\n");
		return ERR_PTR(-ENOMEM);
	}
	spin_lock_init(&scrtc->wb_lock);
	init_completion(&scrtc->flush_done);

	scrtc->sunxi_de = info->de_out;
	scrtc->hw_id = info->hw_id;
	scrtc->plane_cnt = info->plane_cnt;
	scrtc->crtc.port = info->port;
	scrtc->clk_freq = info->clk_freq;
	scrtc->hue_default_value = info->hue_default_value;
	scrtc->share_scaler = info->feat.feat.share_scaler;
	scrtc->plane =
		devm_kzalloc(drm->dev,
			     sizeof(*scrtc->plane) * info->plane_cnt,
			     GFP_KERNEL);
	if (!scrtc->plane) {
		DRM_ERROR("allocate mem for planes fail\n");
		return ERR_PTR(-ENOMEM);
	}

	for (i = 0; i < info->plane_cnt; i++) {
		if (info->planes[i].is_primary) {
			plane = &info->planes[i];
			primary_index = i;
			primary_cnt++;
		}
	}

	if (!plane || primary_cnt > 1) {
		DRM_ERROR("primary plane for de %d cfg err cnt %d\n", info->hw_id, primary_cnt);
		goto err_out;
	}

	/* create primary plane for crtc */
	ret = sunxi_drm_plane_init(drm, scrtc, 0, &scrtc->plane[primary_index],
				   DRM_PLANE_TYPE_PRIMARY, info->hw_id,
				   plane);
	if (ret) {
		DRM_ERROR("plane init fail for de %d\n", info->hw_id);
		goto err_out;
	}

	/* create crtc with primary plane */
	drm_crtc_init_with_planes(drm, &scrtc->crtc,
				  &scrtc->plane[primary_index].plane, NULL,
				  &sunxi_crtc_funcs, "DE-%d", info->hw_id);
	/* create overlay planes with remain channels for the specified crtc */
	for (i = 0; i < info->plane_cnt; i++) {
		plane = &info->planes[i];
		if (plane->is_primary)
			continue;
		ret = sunxi_drm_plane_init(drm, scrtc, drm_crtc_mask(&scrtc->crtc),
				     &scrtc->plane[i], DRM_PLANE_TYPE_OVERLAY,
				     info->hw_id, plane);
		if (ret) {
			DRM_ERROR("sunxi plane init for %d fail\n", i);
			goto err_out;
		}
	}

	drm_mode_crtc_set_gamma_size(&scrtc->crtc, info->gamma_lut_len);
	drm_crtc_enable_color_mgmt(&scrtc->crtc, 0, false, info->gamma_lut_len);
	sunxi_drm_crtc_property_init(scrtc, &info->feat, info->hue_default_value);
	drm_crtc_helper_add(&scrtc->crtc, &sunxi_crtc_helper_funcs);

#ifdef SUNXI_DRM_PLANE_ASYNC
	if (!commit_task) {
		commit_task = kthread_create(commit_task_thread,
					      &scrtc->crtc, "commit_task");
		if (IS_ERR(commit_task)) {
			DRM_ERROR("create thread fail\n");
			return (void *)commit_task;
		}
		wake_up_process(commit_task);
	}
#endif
	return scrtc;

err_out:
	for (i = 0; i < scrtc->plane_cnt; i++) {
		if (scrtc->plane[i].plane.dev)
			drm_plane_cleanup(&scrtc->plane[i].plane);
	}
	if (scrtc->crtc.dev)
		drm_crtc_cleanup(&scrtc->crtc);
	return ERR_PTR(-EINVAL);
}

void sunxi_drm_crtc_destory(struct sunxi_drm_crtc *scrtc)
{
	int i;
#ifdef SUNXI_DRM_PLANE_ASYNC
	if (commit_task) {
		kthread_stop(commit_task);
		commit_task = NULL;
	}
#endif
	for (i = 0; i < scrtc->plane_cnt; i++) {
		drm_plane_cleanup(&scrtc->plane[i].plane);
	}
	drm_crtc_cleanup(&scrtc->crtc);
}

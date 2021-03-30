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
/*CRT -->Display Engine*/

#ifndef _SUNXI_DRM_CRTC_H_
#define _SUNXI_DRM_CRTC_H_

#include <drm/drm_crtc.h>

#define MANAGER_DEHZ_DIRTY  0x00000080
#define MANAGER_TCON_DIRTY  0x00000100

enum sunxi_crtc_mode {
	CRTC_MODE_3D,/* normal mode */
	CRTC_MODE_ENHANCE,	/*enhance the color */
	CRTC_MODE_ENHANCE_HF,
	CRTC_MODE_SMART_LIGHT,
	CRTC_MODE_LAST,
};

struct sunxi_drm_crtc {
	struct drm_crtc drm_crtc;
	/* crtc_id 0 start, make it careful for possible_crtcs from 1 start */
	int crtc_id;
	int dpms;
	/* Channel has zOrder,
	* and the layers in same channel must check zOrder
	* with other channel.we adjust zpos to the zOder of
	* the DE BSP 
	*/
	struct drm_property *plane_zpos_property; //0~15  0~7
	struct drm_property *fill_color_mode_property;
	/* You can set the zOrder  of the channel in pipe mux,
	* but you must know the de feature
	*/
	struct drm_property *channel_id_property;
	/* The id in the channel,
	* the zOrder id is that: 3>2>1>0
	*/
	struct drm_property *plane_id_chn_property;
	struct drm_property *_3d_property;

	int chn_count;
	int plane_of_de;
	struct drm_plane **plane_array;
	struct drm_plane *harware_cursor;
	struct drm_plane *fb_plane;
	struct disp_layer_config_data *plane_cfgs;

	struct disp_manager_data *global_cfgs;
	struct list_head pageflip_event_list;
	struct work_struct delayed_work;
	spinlock_t update_reg_lock;
	unsigned long update_frame_user_cnt;
	unsigned long user_skip_cnt; 
	unsigned long update_frame_ker_cnt;
	unsigned long ker_skip_cnt;
	bool user_update_frame;
	bool ker_update_frame;
	bool has_delayed_updata;
	bool chain_irq_enable;
	bool in_update_frame;
	struct sunxi_hardware_res *hw_res;

};

#define to_sunxi_crtc(x)	container_of(x, struct sunxi_drm_crtc, drm_crtc)

irqreturn_t sunxi_crtc_vsync_handle(int irq, void *data);

int  sunxi_drm_crtc_create(struct drm_device *dev,
	unsigned int nr, struct sunxi_hardware_res *hw_res);

int sunxi_drm_enable_vblank(struct drm_device *dev, int crtc);

void sunxi_drm_disable_vblank(struct drm_device *dev, int crtc);

struct sunxi_drm_crtc *get_sunxi_crt(struct drm_device *dev, int nr);

bool sunxi_set_global_cfg(struct drm_crtc *crtc, int type, void *data);

int init_crtc_array(int num);

enum disp_output_type get_sunxi_crtc_out_type(int nr);

void sunxi_drm_crtc_finish_pageflip(struct drm_device *dev,
	struct sunxi_drm_crtc *sunxi_crtc);

int sunxi_drm_flip_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);

int sunxi_drm_info_fb_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
#endif

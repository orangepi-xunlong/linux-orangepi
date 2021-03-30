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
/*Encoder --> Tcon*/

#ifndef _SUNXI_DRM_ENCODER_H_
#define _SUNXI_DRM_ENCODER_H_

#define ENCODER_UPDATA_DELAYED 3

enum encoder_ops_type {
    ENCODER_OPS_DSI = 0,
    ENCODER_OPS_LVDS,
    ENCODER_OPS_TV,
    ENCODER_OPS_HDMI,
    ENCODER_OPS_VGA,
    ENCODER_OPS_NUM,

};

struct sunxi_drm_encoder {
	struct drm_encoder	drm_encoder;
	int	enc_id;
	int 	part_id;
	int	dpms;
	struct sunxi_hardware_res *hw_res; 
};

#define to_sunxi_encoder(x)	(container_of(x, struct sunxi_drm_encoder, drm_encoder))

irqreturn_t sunxi_encoder_vsync_handle(int irq, void *data);

int sunxi_drm_encoder_create(struct drm_device *dev, unsigned int possible_crtcs,
			int fix_crtc, int id, struct sunxi_hardware_res *hw_res);
int  sunxi_encoder_updata_delayed(struct drm_encoder *encoder);

void sunxi_drm_encoder_enable(struct drm_encoder *encoder);

void sunxi_drm_encoder_disable(struct drm_encoder *encoder);

struct drm_connector *sunxi_get_conct(struct drm_encoder *encoder);

int sunxi_encoder_assign_ops(struct drm_device *dev, int nr,
		enum encoder_ops_type type, void *private);

#endif

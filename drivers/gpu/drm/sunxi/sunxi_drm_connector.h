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
/*Connector --> HDMI/LCD/MIPI/etc*/

#ifndef _SUNXI_DRM_CONNECTOR_H_
#define _SUNXI_DRM_CONNECTOR_H_
#include "de/include.h"
#include "sunxi_drm_panel.h"

enum chain_bit_mask{
    CHAIN_BIT_CONNECTER = 0x01,
    CHAIN_BIT_ENCODER = 0x02,
    CHAIN_BIT_CRTC = 0x04,
};

struct sunxi_drm_connector {
	struct drm_connector drm_connector;
	int connector_id;
	int dmps;
	struct drm_property *brigt_linght_property; //0~15  0~7
	struct sunxi_hardware_res *hw_res;
	enum disp_output_type disp_out_type;

	struct sunxi_panel *panel;
};

#define to_sunxi_connector(x)	container_of(x, struct sunxi_drm_connector, drm_connector)

irqreturn_t sunxi_connector_vsync_handle(int irq, void *data);

int sunxi_drm_connector_create(struct drm_device *dev, int possible_enc, int fix_enc, int id,
     struct sunxi_panel *panel, enum disp_output_type disp_out_type, struct sunxi_hardware_res *hw_res);

void sunxi_chain_disable(struct drm_connector *connector,
        enum chain_bit_mask id);

void sunxi_chain_enable(struct drm_connector *connector,
        enum chain_bit_mask id);

void sunxi_drm_connector_reset(struct drm_connector *connector);

void sunxi_drm_display_power(struct drm_connector *connector, int mode);

#endif

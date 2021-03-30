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
 
/* panel  for the lcd and hdmi panel*/
#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>

#include "sunxi_drm_panel.h"
#include "subdev/sunxi_lcd.h"
#include "subdev/sunxi_hdmi.h"
#include "sunxi_drm_connector.h"

struct sunxi_panel *sunxi_panel_creat(enum disp_output_type type, int pan_id)
{
	struct sunxi_panel *sunxi_panel;
	sunxi_panel = kzalloc(sizeof(struct sunxi_panel), GFP_KERNEL);
	if (!sunxi_panel) {
		DRM_ERROR("failed to allocate sunxi_panel.\n");
		goto out;
	}
	sunxi_panel->panel_id = pan_id;
	sunxi_panel->type = type;
out:
	return sunxi_panel;
}

void sunxi_panel_destroy(struct sunxi_panel *panel)
{
	struct sunxi_drm_connector *sunxi_connector; 
	sunxi_connector = to_sunxi_connector(panel->drm_connector);
	switch (panel->type){
	case DISP_OUTPUT_TYPE_LCD:
		sunxi_lcd_destroy(panel, sunxi_connector->hw_res);
		break;
	case DISP_OUTPUT_TYPE_TV:
		break;
	case DISP_OUTPUT_TYPE_HDMI:
		sunxi_hdmi_pan_destroy(panel, sunxi_connector->hw_res);
		break;
	case DISP_OUTPUT_TYPE_VGA:
		break;
	default:
		DRM_ERROR("give us a err type sunxi_panel.\n");
	}
	kfree(panel);
}


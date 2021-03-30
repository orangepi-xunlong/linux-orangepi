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
/*panel --> display panel(hdmi)*/

#ifndef _SUNXI_DRM_PANEL_H_
#define _SUNXI_DRM_PANEL_H_

#include "de/disp_lcd.h"

struct sunxi_panel;

struct panel_ops {
	bool (*init)(struct sunxi_panel *panel);
	bool (*open)(struct sunxi_panel *panel);
	bool (*close)(struct sunxi_panel *panel);
	bool (*reset)(struct sunxi_panel *panel);
	bool (*bright_light)(struct sunxi_panel *panel, unsigned int bright);
	enum drm_connector_status (*detect)(struct sunxi_panel *panel);
	enum drm_mode_status (*check_valid_mode)(struct sunxi_panel *panel, struct drm_display_mode *mode);
	enum drm_mode_status (*change_mode_to_timming)(void *timing, struct drm_display_mode *mode);
	unsigned int (*get_modes)(struct sunxi_panel *panel);
};

struct pwm_info_t{
	struct pwm_device *pwm_dev;
	u32 channel;
	u32 polarity;
	u32 period_ns;
	u32 duty_ns;
	bool enabled;
};

struct sunxi_panel {
	struct drm_connector *drm_connector;
	enum disp_output_type type;
	char *name;
	int panel_id;
	uint32_t dpms;
	struct panel_ops *panel_ops;
	struct clk *clk;
	unsigned long clk_rate;
	void *private;
};

struct sunxi_panel *sunxi_panel_creat(enum disp_output_type type, int pan_id);

void sunxi_panel_destroy(struct sunxi_panel *panel);

#endif


/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: lfdcn <hezexing@allwinnertech.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_LVDS_H_
#define __PANEL_LVDS_H_

#include <video/videomode.h>

#define POWER_MAX 3
#define GPIO_MAX 3

struct panel_lvds {
	struct drm_panel panel;
	struct device *dev;

	const char *label;
	unsigned int width;
	unsigned int height;
	struct videomode video_mode;
	unsigned int bus_format;
	bool data_mirror;

	unsigned int reset_num;
	struct {
		unsigned int power;
		unsigned int enable;
		unsigned int reset;
	} delay;

	struct backlight_device *backlight;
	struct regulator *supply[POWER_MAX];

	struct gpio_desc *enable_gpio[GPIO_MAX];
	struct gpio_desc *reset_gpio;
	enum drm_panel_orientation orientation;

	struct panel_lvds_helper_funcs *funcs;
};

struct panel_lvds_helper_funcs {
	int (*regulator_enable)(struct drm_panel *panel);
	bool (*is_support_backlight)(struct drm_panel *panel);

	int (*get_backlight_value)(struct drm_panel *panel);
	void (*set_backlight_value)(struct drm_panel *panel, int brightness);
};

static inline struct panel_lvds *to_panel_lvds(struct drm_panel *panel)
{
	return container_of(panel, struct panel_lvds, panel);
}

int panel_lvds_regulator_enable(struct drm_panel *panel);
bool panel_lvds_is_support_backlight(struct drm_panel *panel);
int panel_lvds_get_backlight_value(struct drm_panel *panel);
void panel_lvds_set_backlight_value(struct drm_panel *panel, int brightness);

#endif

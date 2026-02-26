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

#ifndef __PANEL_RGB_H_
#define __PANEL_RGB_H_

#include <video/videomode.h>

#define POWER_MAX 3
#define GPIO_MAX 3

struct panel_rgb {
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

	struct panel_rgb_helper_funcs *funcs;
};

struct panel_rgb_helper_funcs {
	int (*regulator_enable)(struct drm_panel *panel);
	bool (*is_support_backlight)(struct drm_panel *panel);

	int (*get_backlight_value)(struct drm_panel *panel);
	void (*set_backlight_value)(struct drm_panel *panel, int brightness);
};

static inline struct panel_rgb *to_panel_rgb(struct drm_panel *panel)
{
	return container_of(panel, struct panel_rgb, panel);
}

int panel_rgb_regulator_enable(struct drm_panel *panel);
bool panel_rgb_is_support_backlight(struct drm_panel *panel);
int panel_rgb_get_backlight_value(struct drm_panel *panel);
void panel_rgb_set_backlight_value(struct drm_panel *panel, int brightness);

#endif

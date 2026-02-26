/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __PANELS_H__
#define __PANELS_H__

#if IS_ENABLED(CONFIG_PANEL_DSI_GENERAL)
struct panel_dsi {
	struct drm_panel panel;
	struct device *dev;
	struct device *panel_dev;
	struct mipi_dsi_device *dsi;

	const struct panel_desc *desc;
	unsigned int bus_format;
	unsigned int vrr_setp;
	unsigned int pll_ss_permille;

	unsigned int power_num;
	unsigned int gpio_num;
	struct regulator *supply[10];
	struct regulator *avdd_supply;
	struct regulator *avee_supply;
	unsigned int avdd_output_voltage;
	unsigned int avee_output_voltage;
	struct gpio_desc *enable_gpio[10];
	struct gpio_desc *reset_gpio;

	struct drm_dsc_config *dsc;

	enum drm_panel_orientation orientation;
};

int panel_dsi_regulator_enable(struct drm_panel *panel);
bool panel_dsi_is_support_backlight(struct drm_panel *panel);
int panel_dsi_get_backlight_value(struct drm_panel *panel);
void panel_dsi_set_backlight_value(struct drm_panel *panel, int brightness);
#endif

#if IS_ENABLED(CONFIG_PANEL_LVDS_GENERAL)
int panel_lvds_regulator_enable(struct drm_panel *panel);
bool panel_lvds_is_support_backlight(struct drm_panel *panel);
int panel_lvds_get_backlight_value(struct drm_panel *panel);
void panel_lvds_set_backlight_value(struct drm_panel *panel, int brightness);
#endif

#if IS_ENABLED(CONFIG_PANEL_RGB_GENERAL)
int panel_rgb_regulator_enable(struct drm_panel *panel);
bool panel_rgb_is_support_backlight(struct drm_panel *panel);
int panel_rgb_get_backlight_value(struct drm_panel *panel);
void panel_rgb_set_backlight_value(struct drm_panel *panel, int brightness);
#endif

#if IS_ENABLED(CONFIG_PANEL_EDP_GENERAL)
bool general_panel_edp_is_support_backlight(struct drm_panel *panel);
int general_panel_edp_get_backlight_value(struct drm_panel *panel);
void general_panel_edp_set_backlight_value(struct drm_panel *panel, int brightness);
#else
static inline bool general_panel_edp_is_support_backlight(struct drm_panel *panel) { return false; }
static inline int general_panel_edp_get_backlight_value(struct drm_panel *panel) { return 0; }
static inline void general_panel_edp_set_backlight_value(struct drm_panel *panel, int brightness) {}
#endif

#define MIPI_DSI_MODE_VRR	BIT(24)

#endif

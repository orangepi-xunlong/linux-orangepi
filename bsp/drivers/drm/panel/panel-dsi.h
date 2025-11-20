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

#ifndef __PANEL_DSI_H_
#define __PANEL_DSI_H_

#include <video/videomode.h>
#include <drm/drm_mipi_dsi.h>

struct panel_cmd_header {
	u8 data_type;
	u8 delay;
	u8 payload_length;
} __packed;

struct panel_cmd_desc {
	struct panel_cmd_header header;
	u8 *payload;
};

struct panel_cmd_seq {
	struct panel_cmd_desc *cmds;
	unsigned int cmd_cnt;
};

struct gpio_timing {
	u32 level;
	u32 delay;
};

struct reset_sequence {
	u32 items;
	struct gpio_timing *timing;
};

struct panel_desc {
	const struct display_timings *timings;
	struct {
		unsigned int width;
		unsigned int height;
	} size;

	unsigned int reset_num;
	 struct {
		unsigned int power;
		unsigned int enable;
		unsigned int reset;
	} delay;
	struct reset_sequence rst_on_seq;
	struct reset_sequence rst_off_seq;

	struct panel_cmd_seq *init_seq;
	struct panel_cmd_seq *exit_seq;
	struct drm_dsc_config *dsc;
};

struct panel_desc_dsi {
	struct panel_desc desc;
	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	unsigned int lanes;
};

struct panel_dsi {
	struct drm_panel panel;
	struct device *dev;
	struct device *panel_dev;
	struct mipi_dsi_device *dsi;

	struct panel_desc *desc;
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

	struct panel_dsi_helper_funcs *funcs;
};

struct panel_dsi_helper_funcs {
	void (*esd_reset)(struct panel_dsi *dsi_panel, unsigned int items);

	int (*regulator_enable)(struct drm_panel *panel);
	bool (*is_support_backlight)(struct drm_panel *panel);

	int (*get_backlight_value)(struct drm_panel *panel);
	void (*set_backlight_value)(struct drm_panel *panel, int brightness);
};

static inline struct panel_dsi *to_panel_dsi(struct drm_panel *panel)
{
	return container_of(panel, struct panel_dsi, panel);
}

int panel_dsi_regulator_enable(struct drm_panel *panel);
bool panel_dsi_is_support_backlight(struct drm_panel *panel);
int panel_dsi_get_backlight_value(struct drm_panel *panel);
void panel_dsi_set_backlight_value(struct drm_panel *panel, int brightness);

#endif

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

#ifndef __PANEL_EDP_H__
#define __PANEL_EDP_H__

#if IS_ENABLED(CONFIG_PANEL_EDP_GENERAL)
bool general_panel_edp_is_support_backlight(struct drm_panel *panel);
int general_panel_edp_get_backlight_value(struct drm_panel *panel);
void general_panel_edp_set_backlight_value(struct drm_panel *panel, int brightness);
#else
static inline bool general_panel_edp_is_support_backlight(struct drm_panel *panel) { return false; }
static inline int general_panel_edp_get_backlight_value(struct drm_panel *panel) { return 0; }
static inline void general_panel_edp_set_backlight_value(struct drm_panel *panel, int brightness) {}
#endif

#endif

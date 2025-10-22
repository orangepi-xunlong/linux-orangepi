/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * edp panel function define
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __EDP_PANELS_H__
#define __EDP_PANELS_H__
#include "../../disp/de/bsp_display.h"

extern void EDP_OPEN_FUNC(u32 sel, EDP_FUNC func, u32 delay /* ms */);
extern void EDP_CLOSE_FUNC(u32 sel, EDP_FUNC func, u32 delay /* ms */);
extern struct __edp_panel *edp_panel_array[];
extern int sunxi_disp_get_edp_ops(struct sunxi_disp_edp_ops *src_ops);

struct __edp_panel {
	char name[32];
	struct disp_lcd_panel_fun func;
};

struct sunxi_edp_panel_drv {
	struct sunxi_disp_edp_ops edp_ops;
};

int edp_panels_init(void);
void edp_set_panel_funs(void);
void sunxi_edp_backlight_enable(u32 screen_id);
void sunxi_edp_backlight_disable(u32 screen_id);
void sunxi_edp_power_enable(u32 screen_id, u32 pwr_id);
void sunxi_edp_power_disable(u32 screen_id, u32 pwr_id);
s32 sunxi_edp_delay_ms(u32 ms);
s32 sunxi_edp_delay_us(u32 ms);
s32 sunxi_edp_pwm_enable(u32 screen_id);
s32 sunxi_edp_pwm_disable(u32 screen_id);
s32 sunxi_edp_set_panel_funs(char *name, struct disp_lcd_panel_fun *edp_cfg);
s32 sunxi_edp_pin_cfg(u32 screen_id, u32 bon);
s32 sunxi_edp_gpio_set_value(u32 screen_id, u32 io_index, u32 value);
s32 sunxi_edp_gpio_set_direction(u32 screen_id, u32 io_index, u32 direct);

#if IS_ENABLED(CONFIG_EDP_SUPPORT_VVX10T025J00_2560X1600) || \
	IS_ENABLED(CONFIG_EDP_SUPPORT_VVX10T025J00_2560X1600_MODULE)
extern struct __edp_panel VVX10T025J00_2560X1600_panel;
#endif

#if IS_ENABLED(CONFIG_EDP_SUPPORT_LM116X001A40_1920X1080) || \
	IS_ENABLED(CONFIG_EDP_SUPPORT_LM116X001A40_1920X1080_MODULE)
extern struct __edp_panel LM116X001A40_1920X1080_panel;
#endif

#if IS_ENABLED(CONFIG_EDP_SUPPORT_GENERAL_PANEL) || \
	IS_ENABLED(CONFIG_EDP_SUPPORT_GENERAL_PANEL_MODULE)
extern struct __edp_panel general_panel;
#endif

#endif

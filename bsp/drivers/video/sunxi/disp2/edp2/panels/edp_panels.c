/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * edp panel function for edp driver
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "edp_panels.h"

struct sunxi_edp_panel_drv g_edp_drv;

struct __edp_panel *edp_panel_array[] = {
//	&default_panel,
#if IS_ENABLED(CONFIG_EDP_SUPPORT_VVX10T025J00_2560X1600) || \
	IS_ENABLED(CONFIG_EDP_SUPPORT_VVX10T025J00_2560X1600_MODULE)
	&VVX10T025J00_2560X1600_panel,
#endif
#if IS_ENABLED(CONFIG_EDP_SUPPORT_LM116X001A40_1920X1080) || \
	IS_ENABLED(CONFIG_EDP_SUPPORT_LM116X001A40_1920X1080_MODULE)
	&LM116X001A40_1920X1080_panel,
#endif
#if IS_ENABLED(CONFIG_EDP_SUPPORT_GENERAL_PANEL) || \
	IS_ENABLED(CONFIG_EDP_SUPPORT_GENERAL_PANEL_MODULE)
	&general_panel,
#endif
	NULL,
};

s32 sunxi_edp_delay_ms(u32 ms)
{
	return g_edp_drv.edp_ops.sunxi_edp_delay_ms(ms);

	return -1;
}

s32 sunxi_edp_delay_us(u32 us)
{
	return g_edp_drv.edp_ops.sunxi_edp_delay_us(us);

	return -1;
}


void sunxi_edp_backlight_enable(u32 screen_id)
{
	if (g_edp_drv.edp_ops.sunxi_edp_backlight_enable)
		g_edp_drv.edp_ops.sunxi_edp_backlight_enable(screen_id);
}

void sunxi_edp_backlight_disable(u32 screen_id)
{
	if (g_edp_drv.edp_ops.sunxi_edp_backlight_disable)
		g_edp_drv.edp_ops.sunxi_edp_backlight_disable(screen_id);
}

void sunxi_edp_power_enable(u32 screen_id, u32 pwr_id)
{
	if (g_edp_drv.edp_ops.sunxi_edp_power_enable)
		g_edp_drv.edp_ops.sunxi_edp_power_enable(screen_id, pwr_id);
}

void sunxi_edp_power_disable(u32 screen_id, u32 pwr_id)
{
	if (g_edp_drv.edp_ops.sunxi_edp_power_disable)
		g_edp_drv.edp_ops.sunxi_edp_power_disable(screen_id, pwr_id);
}

s32 sunxi_edp_pwm_enable(u32 screen_id)
{
	if (g_edp_drv.edp_ops.sunxi_edp_pwm_enable)
		return g_edp_drv.edp_ops.sunxi_edp_pwm_enable(screen_id);

	return -1;
}

s32 sunxi_edp_pwm_disable(u32 screen_id)
{
	if (g_edp_drv.edp_ops.sunxi_edp_pwm_disable)
		return g_edp_drv.edp_ops.sunxi_edp_pwm_disable(screen_id);

	return -1;
}


s32 sunxi_edp_set_panel_funs(char *name, struct disp_lcd_panel_fun *edp_cfg)
{
	if (g_edp_drv.edp_ops.sunxi_edp_set_panel_funs)
		return g_edp_drv.edp_ops.sunxi_edp_set_panel_funs(name,
								  edp_cfg);

	return -1;
}

s32 sunxi_edp_pin_cfg(u32 screen_id, u32 bon)
{
	if (g_edp_drv.edp_ops.sunxi_edp_pin_cfg)
		return g_edp_drv.edp_ops.sunxi_edp_pin_cfg(screen_id, bon);

	return -1;
}

s32 sunxi_edp_gpio_set_value(u32 screen_id, u32 io_index, u32 value)
{
	if (g_edp_drv.edp_ops.sunxi_edp_gpio_set_value)
		return g_edp_drv.edp_ops.sunxi_edp_gpio_set_value(screen_id,
							io_index, value);
	return -1;
}

s32 sunxi_edp_gpio_set_direction(u32 screen_id, u32 io_index, u32 direct)
{
	if (g_edp_drv.edp_ops.sunxi_edp_gpio_set_direction)
		return g_edp_drv.edp_ops.sunxi_edp_gpio_set_direction(screen_id,
							io_index, direct);
	return -1;
}


void edp_set_panel_funs(void)
{
	int i;

	for (i = 0; edp_panel_array[i] != NULL; i++) {
		sunxi_edp_set_panel_funs(edp_panel_array[i]->name,
					 &edp_panel_array[i]->func);
	}
}

int edp_panels_init(void)
{
	sunxi_disp_get_edp_ops(&g_edp_drv.edp_ops);
	edp_set_panel_funs();

	return 0;
}
EXPORT_SYMBOL(edp_panels_init);

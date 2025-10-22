/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * general_panel.c
 * LM116X001A40 edp panel driver
 *
 * Copyright (c) 2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*
 * [edp0]
 * edp_panel_used = <1>;
 * edp_panel_driver = "general_panel";
 * edp_bl_en = <&pio PH 18 GPIO_ACTIVE_HIGH>;
 * edp_pwm_used = <1>;
 * edp_pwm_ch = <0>;
 * edp_pwm_freq = <50000>;
 * edp_pwm_pol = <0>;
 * edp_default_backlight = <200>;
 * edp_panel_power_0 = "edp-panel";
 * edp_bl_power = "edp-backlight";
 *
 * edid_timings_prefer = <1>;
 * sink_capacity_prefer = <1>
 * timings_fixed = <1>;
 *
 * vcc-edp-supply = <&reg_cldo1>;
 * vdd-edp-supply = <&reg_dcdc2>;
 * edp-panel-supply = <&reg_cldo4>;
 * edp-backlight-supply = <&reg_bl_power>;
 *
 */
#include "general_panel.h"

static void edp_power_on(u32 sel);
static void edp_power_off(u32 sel);
static void edp_bl_open(u32 sel);
static void edp_bl_close(u32 sel);

static void edp_panel_init(u32 sel);
static void edp_panel_exit(u32 sel);

#define panel_reset(sel, val) sunxi_edp_gpio_set_value(sel, 0, val)

static s32 edp_open_flow(u32 sel)
{
	EDP_OPEN_FUNC(sel, edp_power_on, 0);
	EDP_OPEN_FUNC(sel, edp_panel_init, 10);
	EDP_OPEN_FUNC(sel, edp_bl_open, 0);
	return 0;
}

static s32 edp_close_flow(u32 sel)
{
	EDP_CLOSE_FUNC(sel, edp_bl_close, 0);
	EDP_CLOSE_FUNC(sel, edp_panel_exit, 10);
	EDP_CLOSE_FUNC(sel, edp_power_off, 0);

	return 0;
}

static void edp_power_on(u32 sel)
{
	sunxi_edp_power_enable(sel, 0);
	sunxi_edp_delay_ms(40);
	sunxi_edp_power_enable(sel, 1);
	sunxi_edp_delay_ms(5);
	sunxi_edp_power_enable(sel, 2);
}

static void edp_power_off(u32 sel)
{
	panel_reset(sel, 0);
	sunxi_edp_delay_ms(1);
	sunxi_edp_power_disable(sel, 2);
	sunxi_edp_power_disable(sel, 1);
	sunxi_edp_power_disable(sel, 0);
}

static void edp_bl_open(u32 sel)
{
}

static void edp_bl_close(u32 sel)
{
	sunxi_edp_backlight_disable(sel);
	sunxi_edp_pwm_disable(sel);
}

static void edp_panel_init(u32 sel)
{
	sunxi_edp_delay_ms(10);
}

static void edp_panel_exit(u32 sel)
{
	sunxi_edp_delay_ms(10);
}

struct __edp_panel general_panel = {
	/* panel driver name, must mach the name of
	 * edp_drv_name in sys_config.fex
	 */
	.name = "general_panel",
	.func = {
			.cfg_open_flow = edp_open_flow,
			.cfg_close_flow = edp_close_flow,
	},
};

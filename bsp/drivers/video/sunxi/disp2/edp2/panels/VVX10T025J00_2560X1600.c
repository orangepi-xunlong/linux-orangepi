/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * VVX10T025J00_2560X1600.c
 * VVX10T025J00 edp panel driver
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
 * edp_panel_driver = "VVX10T025J00_2560X1600";
 * edp_bl_en = <&pio PB 14 GPIO_ACTIVE_HIGH>;
 * edp_pwm_used = <1>;
 * edp_pwm_ch = <4>;
 * edp_pwm_freq = <50000>;
 * edp_pwm_pol = <0>;
 * edp_default_backlight = <200>;
 * edp_panel_power_0 = "edp-panel";
 *
 * edp_timings_type = <0>;
 * edp_x = <2560>;
 * edp_ht = <2800>;
 * edp_hspw = <32>;
 * edp_hbp = <160>;
 * edp_hfp = <48>;
 * edp_y = <1600>;
 * edp_vt = <1646>;
 * edp_vspw = <6>;
 * edp_vbp = <37>;
 * edp_vfp = <3>;
 * edp_hpolar = <1>;
 * edp_vpolar = <1>;
 * edp_fps = <60>;
 *
 * vcc-edp-supply = <&reg_cldo1>;
 * vdd-edp-supply = <&reg_dcdc2>;
 * edp-panel-supply = <&reg_cldo4>;
 *
 */
#include "VVX10T025J00_2560X1600.h"

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
	//EDP_CLOSE_FUNC(sel, sunxi_edp_tcon_disable, 0);
	EDP_CLOSE_FUNC(sel, edp_power_off, 0);

	return 0;
}

static void edp_power_on(u32 sel)
{
	sunxi_edp_power_enable(sel, 0);
	sunxi_edp_delay_ms(40);
}

static void edp_power_off(u32 sel)
{
	panel_reset(sel, 0);
	sunxi_edp_delay_ms(1);
	sunxi_edp_power_disable(sel, 0);
}

static void edp_bl_open(u32 sel)
{
	sunxi_edp_pwm_enable(sel);
	sunxi_edp_backlight_enable(sel);
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

struct __edp_panel VVX10T025J00_2560X1600_panel = {
	/* panel driver name, must mach the name of
	 * edp_drv_name in sys_config.fex
	 */
	.name = "VVX10T025J00_2560X1600",
	.func = {
			.cfg_open_flow = edp_open_flow,
			.cfg_close_flow = edp_close_flow,
	},
};

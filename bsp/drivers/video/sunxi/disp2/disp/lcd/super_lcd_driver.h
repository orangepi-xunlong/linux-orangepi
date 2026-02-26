/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2021 Allwinnertech Co., Ltd.
 * Author: libairong <libairong@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __SUPER_LCD_PANEL_H__
#define  __SUPER_LCD_PANEL_H__

#include "panels.h"

extern struct __lcd_panel super_lcd_panel;

enum {
	/* void (*p_func)(u32 sel) */
	SUNXI_LCD_TCON_ENABLE             = 0,
	SUNXI_LCD_TCON_DISABLE,
	SUNXI_LCD_BACKLIGHT_ENABLE,
	SUNXI_LCD_BACKLIGHT_DISABLE,

	/* s32 (*p_func)(u32 sel) */
	SUNXI_LCD_PWM_ENABLE              = 10,
	SUNXI_LCD_PWM_DISABLE,
	SUNXI_LCD_CPU_SET_AUTO_MODE,
	SUNXI_LCD_DSI_CLK_ENABLE,
	SUNXI_LCD_DSI_CLK_DISABLE,

	/* void (*p_func)(u32 sel, u32, u32) */
	SUNXI_LCD_DSI_MODE_SWITCH         = 20,

	/* s32 (*p_func)(u32) */
	SUNXI_LCD_DELAY_MS                = 30,
	SUNXI_LCD_DELAY_US,

	/* void (*p_func)(u32 sel, u32) */
	SUNXI_LCD_POWER_ENABLE            = 40,
	SUNXI_LCD_POWER_DISABLE,
	SUNXI_LCD_CPU_WRITE_INDEX,
	SUNXI_LCD_CPU_WRITE_DATA,
	SUNXI_LCD_DSI_SET_MAX_RET_SIZE,
	SUNXI_LCD_PIN_CFG,

	/* s32 (*p_func)(u32 sel, u32, u32) */
	SUNXI_LCD_CPU_WRITE                = 50,
	SUNXI_LCD_GPIO_SET_VALUE,
	SUNXI_LCD_GPIO_SET_DIRECTION,

	/* s32 (*p_func)(u32 sel, u8, u8 *, u32 *) */
	SUNXI_LCD_DSI_DCS_READ             = 80,
#if defined(SUPPORT_DSI)
	/* s32 (*p_func)(u32 sel, u8, u8 *, u32) */
	DSI_DCS_WR                        = 90,
	SUNXI_LCD_DSI_DCS_WRITE,
	SUNXI_LCD_DSI_GEN_WRITE,

#endif

};

unsigned long lcd_func_table[][2] = {
	{SUNXI_LCD_TCON_ENABLE, (unsigned long)sunxi_lcd_tcon_enable},
	{SUNXI_LCD_TCON_DISABLE, (unsigned long)sunxi_lcd_tcon_disable},
	{SUNXI_LCD_BACKLIGHT_ENABLE, (unsigned long)sunxi_lcd_backlight_enable},
	{SUNXI_LCD_BACKLIGHT_DISABLE, (unsigned long)sunxi_lcd_backlight_disable},

	{SUNXI_LCD_PWM_ENABLE, (unsigned long)sunxi_lcd_pwm_enable},
	{SUNXI_LCD_PWM_DISABLE, (unsigned long)sunxi_lcd_pwm_disable},
	{SUNXI_LCD_CPU_SET_AUTO_MODE, (unsigned long)sunxi_lcd_cpu_set_auto_mode},
	{SUNXI_LCD_DSI_CLK_ENABLE, (unsigned long)sunxi_lcd_dsi_clk_enable},
	{SUNXI_LCD_DSI_CLK_DISABLE, (unsigned long)sunxi_lcd_dsi_clk_disable},

	{SUNXI_LCD_DSI_MODE_SWITCH, (unsigned long)sunxi_lcd_dsi_mode_switch},

	{SUNXI_LCD_DELAY_MS, (unsigned long)sunxi_lcd_delay_ms},
	{SUNXI_LCD_DELAY_US, (unsigned long)sunxi_lcd_delay_us},

	{SUNXI_LCD_POWER_ENABLE, (unsigned long)sunxi_lcd_power_enable},
	{SUNXI_LCD_POWER_DISABLE, (unsigned long)sunxi_lcd_power_disable},
	{SUNXI_LCD_CPU_WRITE_INDEX, (unsigned long)sunxi_lcd_cpu_write_index},
	{SUNXI_LCD_CPU_WRITE_DATA, (unsigned long)sunxi_lcd_cpu_write_data},
	{SUNXI_LCD_DSI_SET_MAX_RET_SIZE, (unsigned long)sunxi_lcd_dsi_set_max_ret_size},
	{SUNXI_LCD_PIN_CFG, (unsigned long)sunxi_lcd_pin_cfg},

	{SUNXI_LCD_CPU_WRITE, (unsigned long)sunxi_lcd_cpu_write},
	{SUNXI_LCD_GPIO_SET_VALUE, (unsigned long)sunxi_lcd_gpio_set_value},
	{SUNXI_LCD_GPIO_SET_DIRECTION, (unsigned long)sunxi_lcd_gpio_set_direction},

	{SUNXI_LCD_DSI_DCS_READ, (unsigned long)sunxi_lcd_dsi_dcs_read},
#if defined(SUPPORT_DSI)
	{DSI_DCS_WR, (unsigned long)dsi_dcs_wr},
	{SUNXI_LCD_DSI_DCS_WRITE, (unsigned long)sunxi_lcd_dsi_dcs_write},
	{SUNXI_LCD_DSI_GEN_WRITE, (unsigned long)sunxi_lcd_dsi_gen_write},
#endif
};

#endif

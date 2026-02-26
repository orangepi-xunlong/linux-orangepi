/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __PANEL_H__
#define __PANEL_H__
#include "../de/bsp_display.h"
#include "lcd_source.h"
#include "../de/disp_lcd.h"
extern void LCD_OPEN_FUNC(u32 sel, LCD_FUNC func, u32 delay /* ms */);
extern void LCD_CLOSE_FUNC(u32 sel, LCD_FUNC func, u32 delay /* ms */);

struct __lcd_panel {
	char name[32];
	struct disp_lcd_panel_fun func;
};

extern struct __lcd_panel *panel_array[];

struct sunxi_lcd_drv {
	struct sunxi_disp_source_ops src_ops;
};

#ifndef SUPPORT_DSI
enum __dsi_dcs_t {
	DSI_DCS_ENTER_IDLE_MODE = 0x39,	/* 01 */
	DSI_DCS_ENTER_INVERT_MODE = 0x21,	/* 02 */
	DSI_DCS_ENTER_NORMAL_MODE = 0x13,	/* 03 */
	DSI_DCS_ENTER_PARTIAL_MODE = 0x12,	/* 04 */
	DSI_DCS_ENTER_SLEEP_MODE = 0x10,	/* 05 */
	DSI_DCS_EXIT_IDLE_MODE = 0x38,	/* 06 */
	DSI_DCS_EXIT_INVERT_MODE = 0x20,	/* 07 */
	DSI_DCS_EXIT_SLEEP_MODE = 0x11,	/* 08 */
	DSI_DCS_GET_ADDRESS_MODE = 0x0b,	/* 09 */
	DSI_DCS_GET_BLUE_CHANNEL = 0x08,	/* 10 */
	DSI_DCS_GET_DIAGNOSTIC_RESULT = 0x0f,	/* 11 */
	DSI_DCS_GET_DISPLAY_MODE = 0x0d,	/* 12 */
	DSI_DCS_GET_GREEN_CHANNEL = 0x07,	/* 13 */
	DSI_DCS_GET_PIXEL_FORMAT = 0x0c,	/* 14 */
	DSI_DCS_GET_POWER_MODE = 0x0a,	/* 15 */
	DSI_DCS_GET_RED_CHANNEL = 0x06,	/* 16 */
	DSI_DCS_GET_SCANLINE = 0x45,	/* 17 */
	DSI_DCS_GET_SIGNAL_MODE = 0x0e,	/* 18 */
	DSI_DCS_NOP = 0x00,	/* 19 */
	DSI_DCS_READ_DDB_CONTINUE = 0xa8,	/* 20 */
	DSI_DCS_READ_DDB_START = 0xa1,	/* 21 */
	DSI_DCS_READ_MEMORY_CONTINUE = 0x3e,	/* 22 */
	DSI_DCS_READ_MEMORY_START = 0x2e,	/* 23 */
	DSI_DCS_SET_ADDRESS_MODE = 0x36,	/* 24 */
	DSI_DCS_SET_COLUMN_ADDRESS = 0x2a,	/* 25 */
	DSI_DCS_SET_DISPLAY_OFF = 0x28,	/* 26 */
	DSI_DCS_SET_DISPLAY_ON = 0x29,	/* 27 */
	DSI_DCS_SET_GAMMA_CURVE = 0x26,	/* 28 */
	DSI_DCS_SET_PAGE_ADDRESS = 0x2b,	/* 29 */
	DSI_DCS_SET_PARTIAL_AREA = 0x30,	/* 30 */
	DSI_DCS_SET_PIXEL_FORMAT = 0x3a,	/* 31 */
	DSI_DCS_SET_SCROLL_AREA = 0x33,	/* 32 */
	DSI_DCS_SET_SCROLL_START = 0x37,	/* 33 */
	DSI_DCS_SET_TEAR_OFF = 0x34,	/* 34 */
	DSI_DCS_SET_TEAR_ON = 0x35,	/* 35 */
	DSI_DCS_SET_TEAR_SCANLINE = 0x44,	/* 36 */
	DSI_DCS_SOFT_RESET = 0x01,	/* 37 */
	DSI_DCS_WRITE_LUT = 0x2d,	/* 38 */
	DSI_DCS_WRITE_MEMORY_CONTINUE = 0x3c,	/* 39 */
	DSI_DCS_WRITE_MEMORY_START = 0x2c,	/* 40 */
};
#endif

extern int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops);
int lcd_init(void);

extern struct __lcd_panel default_eink;
extern struct __lcd_panel default_panel;
extern struct __lcd_panel lt070me05000_panel;
extern struct __lcd_panel wtq05027d01_panel;
extern struct __lcd_panel t27p06_panel;
extern struct __lcd_panel dx0960be40a1_panel;
extern struct __lcd_panel tft720x1280_panel;
extern struct __lcd_panel S6D7AA0X01_panel;
extern struct __lcd_panel gg1p4062utsw_panel;
extern struct __lcd_panel ls029b3sx02_panel;
extern struct __lcd_panel he0801a068_panel;
#if IS_ENABLED(CONFIG_LCD_SUPPORT_INET_DSI_PANEL)
extern struct __lcd_panel inet_dsi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_LQ101R1SX03)
extern struct __lcd_panel lq101r1sx03_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_LQ079L1SX01)
extern struct __lcd_panel lq079l1sx01_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_WILLIAMLCD)
extern struct __lcd_panel WilliamLcd_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ILI9881C)
extern struct __lcd_panel ili9881c_dsi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_TM_DSI_PANEL)
extern struct __lcd_panel tm_dsi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_FD055HD003S)
extern struct __lcd_panel fd055hd003s_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_FRD450H40014)
extern struct __lcd_panel frd450h40014_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_H245QBN02)
extern struct __lcd_panel h245qbn02_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ILI9341)
extern struct __lcd_panel ili9341_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_LH219WQ1)
extern struct __lcd_panel lh219wq1_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ST7789V)
extern struct __lcd_panel st7789v_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ST7796S)
extern struct __lcd_panel st7796s_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ST7701S)
extern struct __lcd_panel st7701s_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_EQT371BYZ009G)
extern struct __lcd_panel eqt371byz009g_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_T30P106)
extern struct __lcd_panel t30p106_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_TO20T20000)
extern struct __lcd_panel to20t20000_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_S2003T46G)
extern struct __lcd_panel s2003t46g_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_WTL096601G03)
extern struct __lcd_panel wtl096601g03_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_RT13QV005D)
extern struct __lcd_panel rt13qv005d_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ST7789V_CPU)
extern struct __lcd_panel st7789v_cpu_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_CC08021801_310_800X1280)
extern struct __lcd_panel CC08021801_310_800X1280_mipi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_JD9366AB_3)
extern struct __lcd_panel jd9366ab_3_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_TFT08006)
extern struct __lcd_panel tft08006_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_BP101WX1_206)
extern struct __lcd_panel bp101wx1_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_K101IM2QA04)
extern struct __lcd_panel k101im2qa04_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_FX070)
extern struct __lcd_panel fx070_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_K101_IM2BYL02_L_800X1280)
extern struct __lcd_panel K101_IM2BYL02_L_800X1280_mipi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_K080_IM2HYL802R_800X1280)
extern struct __lcd_panel K080_IM2HYL802R_800X1280_mipi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_FX070_DHM18BOEL2_1024X600)
extern struct __lcd_panel FX070_DHM18BOEL2_1024X600_mipi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_FT8021_TV097WXM_LH0)
extern struct __lcd_panel FT8021_TV097WXM_LH0_mipi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_JD9365DA_SAT080BO31I21Y03_26114M018IB)
extern struct __lcd_panel JD9365DA_SAT080BO31I21Y03_26114M018IB_mipi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_JD9365DA_SAT080AT31I21Y03_26114M019IB)
extern struct __lcd_panel JD9365DA_SAT080AT31I21Y03_26114M019IB_mipi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_JD9365DA_SQ101A_B4EI313_39R501)
extern struct __lcd_panel SQ101A_B4EI313_39R501_mipi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_KD080D24)
extern struct __lcd_panel kd080d24_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_CC10132007_40A)
extern struct __lcd_panel CC10132007_40A_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_CC10127007_31AB)
extern struct __lcd_panel CC10127007_31AB_mipi_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_NT36523_INCELL_1200X1920)
extern struct __lcd_panel NT36523_INCELL_1200X1920_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_GM7321C)
extern struct __lcd_panel gm7123c_rgb2vga_panel;
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ILI9881T_INCELL_800X1280)
extern struct __lcd_panel ILI9881T_INCELL_800X1280_mipi_panel;
#endif
extern struct __lcd_panel SQ101D_Q5DI404_84H501_mipi_panel;
extern struct __lcd_panel super_lcd_panel;

#endif

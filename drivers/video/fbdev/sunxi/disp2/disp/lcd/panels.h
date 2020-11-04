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

extern void LCD_OPEN_FUNC(u32 sel, LCD_FUNC func, u32 delay /*ms */);
extern void LCD_CLOSE_FUNC(u32 sel, LCD_FUNC func, u32 delay /*ms */);

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
extern struct __lcd_panel inet_dsi_panel;
extern struct __lcd_panel lq101r1sx03_panel;
extern struct __lcd_panel WilliamLcd_panel;
extern struct __lcd_panel fx070_panel;
extern struct __lcd_panel FX070_DHM18BOEL2_1024X600_mipi_panel;
extern struct __lcd_panel bp101wx1_panel;

#endif

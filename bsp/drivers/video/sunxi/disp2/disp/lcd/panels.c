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

#include "panels.h"

struct sunxi_lcd_drv g_lcd_drv;

struct __lcd_panel *panel_array[] = {
#if IS_ENABLED(CONFIG_EINK_PANEL_USED)
	&default_eink,
#endif
	&default_panel,
#if IS_ENABLED(CONFIG_LCD_SUPPORT_FT8021_TV097WXM_LH0)
	&FT8021_TV097WXM_LH0_mipi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_JD9365DA_SAT080BO31I21Y03_26114M018IB)
	&JD9365DA_SAT080BO31I21Y03_26114M018IB_mipi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_JD9365DA_SAT080AT31I21Y03_26114M019IB)
	&JD9365DA_SAT080AT31I21Y03_26114M019IB_mipi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_JD9365DA_SQ101A_B4EI313_39R501)
	&SQ101A_B4EI313_39R501_mipi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_LT070ME05000)
	&lt070me05000_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_WTQ05027D01)
	&wtq05027d01_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_T27P06)
	&t27p06_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_DX0960BE40A1)
	&dx0960be40a1_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_TFT720X1280)
	&tft720x1280_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_S6D7AA0X01)
	&S6D7AA0X01_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_GG1P4062UTSW)
	&gg1p4062utsw_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_LS029B3SX02)
	&ls029b3sx02_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_HE0801A068)
	&he0801a068_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_INET_DSI_PANEL)
	&inet_dsi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_LQ101R1SX03)
	&lq101r1sx03_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_LQ079L1SX01)
	&lq079l1sx01_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_TM_DSI_PANEL)
	&tm_dsi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ILI9881C)
	&ili9881c_dsi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_FD055HD003S)
	&fd055hd003s_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_FRD450H40014)
	&frd450h40014_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_H245QBN02)
	&h245qbn02_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ILI9341)
	&ili9341_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_LH219WQ1)
	&lh219wq1_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ST7789V)
	&st7789v_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ST7796S)
	&st7796s_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ST7701S)
	&st7701s_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_EQT371BYZ009G)
	&eqt371byz009g_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_T30P106)
	&t30p106_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_TO20T20000)
	&to20t20000_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_S2003T46G)
	&s2003t46g_panel,
#endif

#if IS_ENABLED(CONFIG_LCD_SUPPORT_WILLIAMLCD)
	&WilliamLcd_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_WTL096601G03)
	&wtl096601g03_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_RT13QV005D)
	&rt13qv005d_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ST7789V_CPU)
	&st7789v_cpu_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_CC08021801_310_800X1280)
	&CC08021801_310_800X1280_mipi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_JD9366AB_3)
	&jd9366ab_3_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_TFT08006)
	&tft08006_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_BP101WX1_206)
	&bp101wx1_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_K101IM2QA04)
	&k101im2qa04_panel,
#endif

#if IS_ENABLED(CONFIG_LCD_SUPPORT_FX070)
	&fx070_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_K080_IM2HYL802R_800X1280)
	&K080_IM2HYL802R_800X1280_mipi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_K101_IM2BYL02_L_800X1280)
	&K101_IM2BYL02_L_800X1280_mipi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_FX070_DHM18BOEL2_1024X600)
	&FX070_DHM18BOEL2_1024X600_mipi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_KD080D24)
	&kd080d24_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_CC10132007_40A)
	&CC10132007_40A_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_CC10127007_31AB)
	&CC10127007_31AB_mipi_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_NT36523_INCELL_1200X1920)
	&NT36523_INCELL_1200X1920_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_GM7321C)
	&gm7123c_rgb2vga_panel,
#endif
#if IS_ENABLED(CONFIG_LCD_SUPPORT_ILI9881T_INCELL_800X1280)
	&ILI9881T_INCELL_800X1280_mipi_panel,
#endif
	&SQ101D_Q5DI404_84H501_mipi_panel,
	&super_lcd_panel,
	NULL,
};

void lcd_set_panel_funs(void)
{
	int i;

	for (i = 0; panel_array[i] != NULL; i++) {
		sunxi_lcd_set_panel_funs(panel_array[i]->name,
					 &panel_array[i]->func);
	}
}

int lcd_init(void)
{
	sunxi_disp_get_source_ops(&g_lcd_drv.src_ops);
	lcd_set_panel_funs();

	return 0;
}

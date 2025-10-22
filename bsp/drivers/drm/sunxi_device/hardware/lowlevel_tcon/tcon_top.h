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

#ifndef __TCON_TOP_H_
#define __TCON_TOP_H_

enum __tv_set_t {
	TV_TO_GPIO = 1,
	LCD_TO_GPIO = 0,
	TV_CLK_F_CCU = 0,
	TV_CLK_F_TVE = 1
};
void tcon_lcd_dsc_src_sel(void);
s32 tcon_lcd_dsi_clk_source(u32 sel, u32 dual_dsi_flag);
s32 tcon_lcd_rgb_src_sel(u32 src);
s32 tcon_lcd_dsi_src_sel(u32 sel, u32 src);
s32 tcon0_out_to_gpio(u32 sel);
s32 tcon1_out_to_gpio(u32 sel);
s32 tcon1_tv_clk_enable(u32 sel, u32 en);
s32 tcon1_edp_clk_enable(u32 sel, u32 en);
s32 tcon_top_hdmi_set_gate(u32 sel, u32 en);
s32 tcon_top_hdmi_set_clk_src(u32 sel, u32 src);
s32 tcon_lcd_dsi_clk_enable(u32 sel, u32 en);
s32 tcon_de_attach(u32 tcon_index, u32 de_index);
s32 edp_de_attach(u32 edp_index, u32 de_index);
s32 tcon_get_attach_by_de_index(u32 de_index);
s32 tcon_top_set_reg_base(u32 sel, uintptr_t base);
uintptr_t tcon_top_get_reg_base(u32 sel);

#endif

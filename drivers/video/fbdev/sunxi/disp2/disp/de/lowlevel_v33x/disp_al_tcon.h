/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DISP_AL_TCON_H_
#define _DISP_AL_TCON_H_

#include "../include.h"
#include "./tcon/de_lcd.h"
#if defined(SUPPORT_DSI)
#include "./tcon/de_dsi.h"
#endif

struct lcd_clk_info {
	enum disp_lcd_if lcd_if;
	int tcon_div;
	int lcd_div;
	int dsi_div;
	int dsi_rate;
};

int disp_al_lcd_cfg(u32 screen_id, struct disp_panel_para *panel,
	struct panel_extend_para *extend_panel);
int disp_al_lcd_cfg_ext(u32 screen_id, struct panel_extend_para *extend_panel);
int disp_al_lcd_enable(u32 screen_id, struct disp_panel_para *panel);
int disp_al_lcd_disable(u32 screen_id, struct disp_panel_para *panel);
int disp_al_lcd_query_irq(u32 screen_id, enum __lcd_irq_id_t irq_id,
	struct disp_panel_para *panel);
int disp_al_lcd_tri_busy(u32 screen_id, struct disp_panel_para *panel);
int disp_al_lcd_tri_start(u32 screen_id, struct disp_panel_para *panel);
int disp_al_lcd_io_cfg(u32 screen_id, u32 enable, struct disp_panel_para *panel);
int disp_al_lcd_get_cur_line(u32 screen_id, struct disp_panel_para *panel);
int disp_al_lcd_get_start_delay(u32 screen_id, struct disp_panel_para *panel);
int disp_al_lcd_get_clk_info(u32 screen_id, struct lcd_clk_info *info,
	struct disp_panel_para *panel);
int disp_al_lcd_enable_irq(u32 screen_id, enum __lcd_irq_id_t irq_id,
	struct disp_panel_para *panel);
int disp_al_lcd_disable_irq(u32 screen_id, enum __lcd_irq_id_t irq_id,
	struct disp_panel_para *panel);

int disp_al_hdmi_pad_sel(u32 screen_id, u32 pad);
int disp_al_hdmi_enable(u32 screen_id);
int disp_al_hdmi_disable(u32 screen_id);
int disp_al_hdmi_set_output_format(u32 screen_id, u32 output_format);
int disp_al_hdmi_cfg(u32 screen_id, struct disp_video_timings *video_info);
int disp_al_hdmi_irq_enable(u32 screen_id);
int disp_al_hdmi_irq_disable(u32 screen_id);
extern s32 bsp_disp_hdmi_get_color_format(void);

int disp_al_tv_enable(u32 screen_id);
int disp_al_tv_disable(u32 screen_id);
int disp_al_tv_cfg(u32 screen_id, struct disp_video_timings *video_info);
int disp_al_tv_irq_enable(u32 screen_id);
int disp_al_tv_irq_disable(u32 screen_id);

#if defined(SUPPORT_VGA)
int disp_al_vga_enable(u32 screen_id);
int disp_al_vga_disable(u32 screen_id);
int disp_al_vga_cfg(u32 screen_id, struct disp_video_timings *video_info);
int disp_al_vga_irq_enable(u32 screen_id);
int disp_al_vga_irq_disable(u32 screen_id);
#endif

int disp_al_vdevice_cfg(u32 screen_id, struct disp_video_timings *video_info,
	struct disp_vdevice_interface_para *para, u8 config_tcon_only);
int disp_al_vdevice_enable(u32 screen_id);
int disp_al_vdevice_disable(u32 screen_id);

int disp_al_device_get_cur_line(u32 screen_id);
int disp_al_device_get_start_delay(u32 screen_id);
int disp_al_device_query_irq(u32 screen_id);
int disp_al_device_enable_irq(u32 screen_id);
int disp_al_device_disable_irq(u32 screen_id);
int disp_al_device_get_status(u32 screen_id);
int disp_al_device_src_select(u32 screen_id, u32 src);
int disp_al_device_set_de_id(u32 screen_id, u32 de_id);
int disp_al_device_set_de_use_rcq(u32 screen_id, u32 use_rcq);
int disp_al_device_set_output_type(u32 screen_id, u32 output_type);

s32 disp_al_init_tcon(struct disp_bsp_init_para *para);
void disp_al_show_builtin_patten(u32 hwdev_index, u32 patten);
int disp_al_lcd_get_status(u32 screen_id, struct disp_panel_para *panel);

#endif /* #ifndef _DISP_AL_TCON_H_ */

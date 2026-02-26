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

#ifndef __TCON_TV_H_
#define __TCON_TV_H_

struct sunxi_tcon_tv {
	int tcon_index;
	volatile struct tcon_tv_reg *reg;
	//add new register layput as follow if need
	// struct tcon_tv_reg_version2 *reg2
};

s32 tcon_tv_init(struct sunxi_tcon_tv *tcon);
s32 tcon_tv_exit(struct sunxi_tcon_tv *tcon);
u32 tcon_tv_irq_query(struct sunxi_tcon_tv *tcon, enum __lcd_irq_id_t id);
u32 tcon_tv_get_start_delay(struct sunxi_tcon_tv *tcon);
s32 tcon_tv_get_status(struct sunxi_tcon_tv *tcon);
u32 tcon_tv_get_cur_line(struct sunxi_tcon_tv *tcon);

s32 tcon_tv_open(struct sunxi_tcon_tv *tcon);
s32 tcon_tv_close(struct sunxi_tcon_tv *tcon);
s32 tcon_tv_set_reg_base(struct sunxi_tcon_tv *tcon, uintptr_t base);
s32 tcon_tv_src_select(struct sunxi_tcon_tv *tcon, enum __lcd_src_t src, unsigned int de_no);
s32 tcon_tv_src_get(struct sunxi_tcon_tv *tcon);
s32 tcon_tv_set_timming(struct sunxi_tcon_tv *tcon, struct disp_video_timings *timming);
s32 tcon_tv_set_pixel_mode(struct sunxi_tcon_tv *tcon, unsigned int pixel_mode);
s32 tcon_tv_cfg(struct sunxi_tcon_tv *tcon, struct disp_video_timings *timing);
s32 tcon_tv_hdmi_color_remap(struct sunxi_tcon_tv *tcon, u32 onoff);
s32 tcon_tv_black_src(struct sunxi_tcon_tv *tcon, u32 on_off, u32 color);

s32 tcon_tv_volume_force(struct sunxi_tcon_tv *tcon, u32 value);
void tcon_tv_show_builtin_patten(struct sunxi_tcon_tv *tcon, u32 patten);
void tcon_tv_enable_vblank(struct sunxi_tcon_tv *tcon, bool enable);

#endif

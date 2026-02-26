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

#ifndef __TCON_LCD_H_
#define __TCON_LCD_H_

#include "dsi_v1.h"

struct sunxi_tcon_lcd {
	int tcon_index;
	volatile struct tcon_lcd_reg *reg;
};

enum lcd_hv_mode {
	HV_MODE_PRGB_1CYC       = 0,  //parallel hv
	HV_MODE_SRGB_3CYC	= 8,  //serial hv
	HV_MODE_DRGB_4CYC	= 10, //Dummy RGB
	HV_MODE_RGBD_4CYC	= 11, //RGB Dummy
	HV_MODE_CCIR656_2CYC	= 12,
};

enum lcd_hv_srgb_seq {
	HV_SRGB_SEQ_RGB_RGB = 0,
	HV_SRGB_SEQ_RGB_BRG = 1,
	HV_SRGB_SEQ_RGB_GBR = 2,
	HV_SRGB_SEQ_BRG_RGB = 4,
	HV_SRGB_SEQ_BRG_BRG = 5,
	HV_SRGB_SEQ_BRG_GBR = 6,
	HV_SRGB_SEQ_GRB_RGB = 8,
	HV_SRGB_SEQ_GRB_BRG = 9,
	HV_SRGB_SEQ_GRB_GBR = 10,
};
enum lcd_hv_syuv_seq {
	HV_SYUV_SEQ_YUYV    = 0,
	HV_SYUV_SEQ_YVYU    = 1,
	HV_SYUV_SEQ_UYUV    = 2,
	HV_SYUV_SEQ_VYUY    = 3,
};

enum lcd_hv_syuv_fdly {
	HV_SYUV_FDLY_0LINE  = 0,
	HV_SRGB_FDLY_2LINE  = 1, /*ccir pal*/
	HV_SRGB_FDLY_3LINE  = 2, /*ccir ntsc*/
};

enum lcd_lvds_colordepth {
	LVDS_8bit = 0,
	LVDS_6bit = 1,
};

struct disp_lvds_para {
	unsigned int channel;
	unsigned int lanes;
	unsigned int dual_lvds;
	enum lcd_lvds_colordepth lvds_colordepth;
	unsigned int lvds_data_mode;
	unsigned int fsync_en;
	unsigned int fsync_pol;
	unsigned int fsync_act_time;
	unsigned int fsync_dis_time;
	unsigned int rb_swap;
	unsigned int rgb_endian;
	enum disp_lcd_frm lcd_frm;
	struct disp_video_timings timings;
};

struct disp_rgb_para {
	enum lcd_hv_mode hv_mode;
	enum lcd_hv_srgb_seq hv_srgb_seq;
	enum lcd_hv_syuv_seq hv_syuv_seq;
	enum lcd_hv_syuv_fdly hv_syuv_fdly;
	unsigned int input_csc;
	unsigned int fsync_en;
	unsigned int fsync_pol;
	unsigned int fsync_act_time;
	unsigned int fsync_dis_time;
	unsigned int rb_swap;
	unsigned int rgb_endian;
	unsigned int lcd_interlace;
	unsigned int hv_sync_polarity;
	unsigned int hv_clk_phase;
	enum disp_lcd_frm lcd_frm;
	struct disp_video_timings timings;
};
void tcon_lcd_show_builtin_patten(struct sunxi_tcon_lcd *tcon, u32 patten);
s32 tcon_dsi_cfg(struct sunxi_tcon_lcd *tcon, struct disp_dsi_para *para);
s32 tcon_dsi_open(struct sunxi_tcon_lcd *tcon, struct disp_dsi_para *dsi_para);
s32 tcon_dsi_close(struct sunxi_tcon_lcd *tcon);
s32 tcon_lvds_cfg(struct sunxi_tcon_lcd *tcon, struct disp_lvds_para *para);
s32 tcon_lvds_open(struct sunxi_tcon_lcd *tcon);
s32 tcon_lvds_close(struct sunxi_tcon_lcd *tcon);
s32 tcon_rgb_cfg(struct sunxi_tcon_lcd *tcon, struct disp_rgb_para *para);
s32 tcon_rgb_open(struct sunxi_tcon_lcd *tcon);
s32 tcon_rgb_close(struct sunxi_tcon_lcd *tcon);
s32 tcon_cpu_open(struct sunxi_tcon_lcd *tcon);
s32 tcon_cpu_close(struct sunxi_tcon_lcd *tcon);
s32 tcon_lcd_set_dclk_div(struct sunxi_tcon_lcd *tcon, u8 div);
s32 lvds_open(struct sunxi_tcon_lcd *tcon, struct disp_lvds_para *panel);
s32 lvds_close(struct sunxi_tcon_lcd *tcon);
s32 tcon_lcd_set_reg_base(struct sunxi_tcon_lcd *tcon, uintptr_t address);
s32 tcon_lcd_init(struct sunxi_tcon_lcd *tcon);
s32 tcon_lcd_exit(struct sunxi_tcon_lcd *tcon);
s32 tcon_lcd_get_timing(struct sunxi_tcon_lcd *tcon, struct disp_video_timings *tt);
u32 tcon_lcd_irq_query(struct sunxi_tcon_lcd *tcon, enum __lcd_irq_id_t id);
u32 tcon_lcd_get_start_delay(struct sunxi_tcon_lcd *tcon);
u32 tcon_lcd_get_cur_line(struct sunxi_tcon_lcd *tcon);
s32 tcon_lcd_get_status(struct sunxi_tcon_lcd *tcon);
bool tcon_lcd_vrr_irq(struct sunxi_tcon_lcd *tcon, bool enable);
void tcon_lcd_enable_vblank(struct sunxi_tcon_lcd *tcon, bool enable);
s32 tcon_lcd_src_select(struct sunxi_tcon_lcd *tcon, enum __lcd_src_t src);
s32 tcon_lcd_src_get(struct sunxi_tcon_lcd *tcon);
int sunxi_tcon_updata_vt(struct sunxi_tcon_lcd *tcon,
		struct disp_video_timings *timings, u32 vrr_setp);
int sunxi_tcon_updata_vt_2(struct sunxi_tcon_lcd *tcon,
		struct disp_video_timings *timings);
/**
 * @name       :tcon_fsync_set_pol
 * @brief      :set fsync's polarity
 * @param[IN]  :sel:tcon index
 * @param[IN]  :pol:polarity. 1:positive;0:negetive
 *	positive:
 *           +---------+
 *  ---------+         +-----------
 *
 *	negative:
 *  ---------+         +------------
 *	     +---------+
 *
 * @return     :always 0
 */

/**
 * @name       :tcon_set_fsync_active_time
 * @brief      :set tcon fsync's active time
 * @param[IN]  :sel:tcon index
 * @param[IN]  :pixel_num:number of pixel time(Tpixel) to set
 *
 * Tpixel = 1/fps*1e9/vt/ht, unit:ns
 *
 * @return     :0 if success
 */
s32 tcon_set_fsync_active_time(struct sunxi_tcon_lcd *tcon, u32 pixel_num);


#endif

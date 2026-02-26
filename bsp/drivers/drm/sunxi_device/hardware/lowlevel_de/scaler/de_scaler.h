/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_SCALER_H_
#define _DE_SCALER_H_

#include "de_base.h"

struct de_scale_para {
	s32 hphase; /* initial phase of vsu/gsu in horizon */
	s32 vphase; /* initial phase of vsu/gsu in vertical */
	u32 hstep; /* scale step of vsu/gsu in horizon */
	u32 vstep; /* scale step of vsu/gsu in vertical */
};

enum {
	VSU_EXPAND_OUTWIDTH  = 0x00000001,
	VSU_EXPAND_OUTHEIGHT = 0x00000002,
	VSU_EXPAND_INWIDTH   = 0x00000004,
	VSU_EXPAND_INHEIGHT  = 0x00000008,
	VSU_RSHIFT_OUTWINDOW = 0x00000010,
	VSU_RSHIFT_INWINDOW  = 0x00000020,
	VSU_LSHIFT_OUTWINDOW = 0x00000040,
	VSU_LSHIFT_INWINDOW  = 0x00000080,
	VSU_USHIFT_OUTWINDOW = 0x00000100,
	VSU_USHIFT_INWINDOW  = 0x00000200,
	VSU_DSHIFT_OUTWINDOW = 0x00000400,
	VSU_DSHIFT_INWINDOW  = 0x00000800,
	VSU_CUT_OUTWIDTH     = 0x00001000,
	VSU_CUT_OUTHEIGHT    = 0x00002000,
	VSU_CUT_INWIDTH      = 0x00004000,
	VSU_CUT_INHEIGHT     = 0x00008000,
	RTMX_CUT_INHEIGHT    = 0x10000000,
};

struct de_scaler_handle {
	struct module_create_info cinfo;
	unsigned int linebuff_share_ids;
	unsigned int linebuff_yuv;
	unsigned int linebuff_rgb;
	unsigned int linebuff_yuv_ed;
	bool is_asu;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_scaler_private *private;
};

struct de_scaler_cal_lay_cfg {
	enum de_format_space fm_space;
	enum de_yuv_sampling yuv_sampling;
	enum de_pixel_format px_fmt;
	struct de_rect_s ovl_out_win;
	struct de_scale_para ovl_ypara;
	u8 snr_en;
};

struct de_scaler_apply_cfg {
	unsigned int disp;
	u8 scale_en;
	u8 glb_alpha;
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	enum de_pixel_format px_fmt;
	struct de_rect_s scn_win;
	struct de_rect_s ovl_out_win;
	struct de_scale_para ovl_ypara;
	struct de_rect_s c_win;
	struct de_scale_para ovl_cpara;
};

bool de_scaler_pq_is_enabled(struct de_scaler_handle *hdl);
s32 de_scaler_apply_asu_pq_config(struct de_scaler_handle *hdl, asu_module_param_t *para);
s32 de_scaler_asu_pq_enable(struct de_scaler_handle *hdl, u32 en);

s32 de_scaler_calc_lay_scale_para(struct de_scaler_handle *hdl,
			       const struct de_scaler_cal_lay_cfg *cfg,
			       const struct de_rect64_s *crop64,
			       const struct de_rect_s *scn_win,
			       struct de_rect_s *crop32,
			       struct de_scale_para *ypara,
			       struct de_scale_para *cpara);

s32 de_scaler_apply(struct de_scaler_handle *hdl, const struct de_scaler_apply_cfg *cfg);
struct de_scaler_handle *de_scaler_create(const struct module_create_info *info);
u32 de_scaler_fix_tiny_size(struct de_scaler_handle *hdl, struct de_rect_s *in_win,
			 struct de_rect_s *out_win, struct de_scale_para *ypara,
			 enum de_format_space fmt_space,
			 struct de_rect_s *lay_win, u32 lay_num, u32 scn_width,
			 u32 scn_height);
u32 de_scaler_fix_big_size(struct de_scaler_handle *hdl, struct de_rect_s *in_win,
			const struct de_rect_s *out_win,
			enum de_format_space fmt_space,
			enum de_yuv_sampling yuv_sampling);
s32 de_scaler_calc_scale_para(struct de_scaler_handle *hdl, u32 fix_size_result, enum de_format_space fmt_space,
			   enum de_yuv_sampling yuv_sampling,
			   struct de_rect_s *out_win, struct de_rect_s *ywin,
			   struct de_rect_s *cwin, struct de_scale_para *ypara,
			   struct de_scale_para *cpara);

u32 de_scaler_calc_ovl_coord(struct de_scaler_handle *hdl, u32 dst_coord, u32 scale_step);
s32 de_scaler_calc_ovl_scale_para(u32 layer_num, struct de_scale_para *ypara,
			       struct de_scale_para *cpara,
			       struct de_scale_para *ovl_ypara,
			       struct de_scale_para *ovl_cpara);

void dump_scaler_state(struct drm_printer *p, struct de_scaler_handle *hdl, const struct display_channel_state *state);

#endif

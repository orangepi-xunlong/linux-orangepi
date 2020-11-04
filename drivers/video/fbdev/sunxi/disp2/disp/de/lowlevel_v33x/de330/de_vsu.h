/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_VSU_H_
#define _DE_VSU_H_

#include <linux/types.h>
#include "de_rtmx.h"

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

struct de_vsu_sharp_config {
	/* peak2d */
	u32 level; /* detail level */

	/* size */
	s32 inw; /* overlay width */
	s32 inh; /* overlay height */
	s32 outw; /* overlay scale width */
	s32 outh; /* overlay scale height */

};

s32 de_vsu_calc_lay_scale_para(u32 disp, u32 chn,
	enum de_format_space fm_space, struct de_chn_info *chn_info,
	struct de_rect64_s *crop64, struct de_rect_s *scn_win,
	struct de_rect_s *crop32, struct de_scale_para *ypara,
	struct de_scale_para *cpara);

u32 de_vsu_calc_ovl_coord(u32 disp, u32 chn,
	u32 dst_coord, u32 scale_step);
s32 de_vsu_calc_ovl_scale_para(u32 layer_num,
	struct de_scale_para *ypara, struct de_scale_para *cpara,
	struct de_scale_para *ovl_ypara, struct de_scale_para *ovl_cpara);
u32 de_vsu_fix_tiny_size(u32 disp, u32 chn,
	struct de_rect_s *in_win, struct de_rect_s *out_win,
	struct de_scale_para *ypara, enum de_format_space fmt_space,
	struct de_rect_s *lay_win, u32 lay_num,
	u32 scn_width, u32 scn_height);
u32 de_vsu_fix_big_size(u32 disp, u32 chn,
	struct de_rect_s *in_win, struct de_rect_s *out_win,
	enum de_format_space fmt_space, enum de_yuv_sampling yuv_sampling);
s32 de_vsu_calc_scale_para(u32 fix_size_result,
	enum de_format_space fmt_space, enum de_yuv_sampling yuv_sampling,
	struct de_rect_s *out_win,
	struct de_rect_s *ywin, struct de_rect_s *cwin,
	struct de_scale_para *ypara, struct de_scale_para *cpara);

s32 de_vsu_set_para(u32 disp, u32 chn,
	struct de_chn_info *chn_info);
s32 de_vsu_disable(u32 disp, u32 chn);

s32 de_vsu_init_sharp_para(u32 disp, u32 chn);
s32 de_vsu_set_sharp_para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct de_vsu_sharp_config *para, u32 bypass);

s32 de_vsu_init(u32 disp, u8 __iomem *de_reg_base);
s32 de_vsu_exit(u32 disp);
s32 de_vsu_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);

#endif /* #ifndef _DE_CDC_H_ */

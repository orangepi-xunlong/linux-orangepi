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

#include "de_channel.h"
#include "de_scaler.h"
#include "de_scaler_platform.h"
#include "de_scaler_type.h"
#include "de_scaler_table.h"

#define VSU_STEP_VALID_START_BIT 1
#define VSU_STEP_FRAC_BITWIDTH 19
#define VSU_STEP_FIXED_BITWIDTH 4
#define VSU_PHASE_VALID_START_BIT 1
#define VSU_PHASE_FRAC_BITWIDTH 19
#define VSU_PHASE_FIXED_BITWIDTH 4
#define GSU_STEP_VALID_START_BIT 2
#define GSU_STEP_FRAC_BITWIDTH 18
#define GSU_STEP_FIXED_BITWIDTH 5
#define GSU_PHASE_VALID_START_BIT 2
#define GSU_PHASE_FRAC_BITWIDTH 18
#define GSU_PHASE_FIXED_BITWIDTH 4

#define VSU_MIN_INPUT_WIDTH 8
#define VSU_MIN_INPUT_HEIGHT 8

#define GSU_TAP_NUM_HORI 4
#define GSU_TAP_NUM_VERT 2
#define VSU8_TAP_NUM_HORI 4
#define VSU8_TAP_NUM_VERT 2
#define VSU10_TAP_NUM_HORI 8
#define VSU10_TAP_NUM_VERT 4
#define VSU_ED_TAP_NUM_HORI 8
#define VSU_ED_TAP_NUM_VERT 4
#define ASU_TAP_NUM_HORI 8
#define ASU_TAP_NUM_VERT 4

#define CHN_SCALER_OFFSET			(0x04000)

enum { GSU_REG_BLK_CTL = 0,
       GSU_REG_BLK_ATTR,
       GSU_REG_BLK_PARA,
       GSU_REG_BLK_COEFF0,
       GSU_REG_BLK_NUM,
};

enum { VSU8_REG_BLK_CTL = 0,
       VSU8_REG_BLK_ATTR,
       VSU8_REG_BLK_YPARA,
       VSU8_REG_BLK_CPARA,
       VSU8_REG_BLK_COEFF0,
       VSU8_REG_BLK_COEFF1,
       VSU8_REG_BLK_COEFF2,
       VSU8_REG_BLK_NUM,
};

enum { VSU10_REG_BLK_CTL = 0,
       VSU10_REG_BLK_ATTR,
       VSU10_REG_BLK_YPARA,
       VSU10_REG_BLK_CPARA,
       VSU10_REG_BLK_COEFF0,
       VSU10_REG_BLK_COEFF1,
       VSU10_REG_BLK_COEFF2,
       VSU10_REG_BLK_COEFF3,
       VSU10_REG_BLK_COEFF4,
       VSU10_REG_BLK_COEFF5,
       VSU10_REG_BLK_NUM,
};

enum { VSU_ED_REG_BLK_CTL = 0,
       VSU_ED_REG_BLK_DIR_SHP,
       VSU_ED_REG_BLK_ATTR,
       VSU_ED_REG_BLK_YPARA,
       VSU_ED_REG_BLK_CPARA,
       VSU_ED_REG_BLK_COEFF0,
       VSU_ED_REG_BLK_COEFF1,
       VSU_ED_REG_BLK_COEFF2,
       VSU_ED_REG_BLK_COEFF3,
       VSU_ED_REG_BLK_COEFF4,
       VSU_ED_REG_BLK_COEFF5,
       VSU_ED_REG_BLK_NUM,

       VSU_MAX_REG_BLK_NUM = VSU_ED_REG_BLK_NUM,
};

enum { ASU_REG_BLK_CTL = 0,
       ASU_REG_BLK_PEAKING,
       ASU_REG_BLK_ATTR,
       ASU_REG_BLK_YPARA,
       ASU_REG_BLK_CPARA,
       ASU_REG_BLK_COEFF0,
       ASU_REG_BLK_COEFF1,
       ASU_REG_BLK_COEFF2,
       ASU_REG_BLK_COEFF3,
       ASU_REG_BLK_COEFF4,
       ASU_REG_BLK_COEFF5,
       ASU_REG_BLK_NUM,
};

struct scaler_debug_info {
	bool enable;
	unsigned int in_w, in_h, out_w, out_h;
};

struct de_scaler_private {
	struct de_reg_mem_info reg_mem_info;
	struct scaler_debug_info debug;
	u32 reg_blk_num;
	bool pq_init;
	const struct de_scaler_dsc *dsc;
	union {
		struct de_reg_block gsu_reg_blks[GSU_REG_BLK_NUM];
		struct de_reg_block vsu8_reg_blks[VSU8_REG_BLK_NUM];
		struct de_reg_block vsu10_reg_blks[VSU10_REG_BLK_NUM];
		struct de_reg_block vsu_ed_reg_blks[VSU_ED_REG_BLK_NUM];
		struct de_reg_block asu_reg_blks[ASU_REG_BLK_NUM];
		struct de_reg_block reg_blks[VSU_MAX_REG_BLK_NUM];
	};
	u32 tap_num_hori;
	u32 tap_num_vert;
	u32 step_valid_start_bit;
	u32 step_frac_bit_width;
	u32 step_fixed_bit_width;
	u32 phase_valid_start_bit;
	u32 phase_frac_bit_width;
	u32 phase_fixed_bit_width;

};

static void scaler_set_block_dirty(struct de_scaler_private *priv, u32 blk_id,
				   u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static inline struct gsu_reg *get_gsu_reg(struct de_scaler_private *priv)
{
	return (struct gsu_reg *)(priv->gsu_reg_blks[0].vir_addr);
}

static inline struct vsu8_reg *get_vsu8_reg(struct de_scaler_private *priv)
{
	return (struct vsu8_reg *)(priv->vsu8_reg_blks[0].vir_addr);
}

static inline struct vsu10_reg *get_vsu10_reg(struct de_scaler_private *priv)
{
	return (struct vsu10_reg *)(priv->vsu10_reg_blks[0].vir_addr);
}

static inline struct vsu_ed_reg *get_vsu_ed_reg(struct de_scaler_private *priv)
{
	return (struct vsu_ed_reg *)(priv->vsu_ed_reg_blks[0].vir_addr);
}

static inline struct asu_reg *get_asu_reg(struct de_scaler_private *priv)
{
	return (struct asu_reg *)(priv->asu_reg_blks[0].vir_addr);
}

static u32 de_scaler_calc_fir_coef(enum scaler_type type, u32 step)
{
	u32 pt_coef;
	u32 scale_ratio, int_part, float_part, fir_coef_ofst;

	scale_ratio = step >> ((type == DE_SCALER_TYPE_GSU ? GSU_STEP_FRAC_BITWIDTH : VSU_PHASE_FRAC_BITWIDTH) - 3);
	int_part = scale_ratio >> 3;
	float_part = scale_ratio & 0x7;
	if (int_part == 0)
		fir_coef_ofst = VSU_ZOOM0_SIZE;
	else if (int_part == 1)
		fir_coef_ofst = VSU_ZOOM0_SIZE + float_part;
	else if (int_part == 2)
		fir_coef_ofst =
		    VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE + (float_part >> 1);
	else if (int_part == 3)
		fir_coef_ofst =
		    VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE + VSU_ZOOM2_SIZE;
	else if (int_part == 4)
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE +
				VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE;
	else
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE +
				VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE +
				VSU_ZOOM4_SIZE;

	pt_coef = fir_coef_ofst * (type == DE_SCALER_TYPE_GSU ? GSU_PHASE_NUM : VSU_PHASE_NUM);
	return pt_coef;
}

s32 de_scaler_calc_lay_scale_para(struct de_scaler_handle *hdl,
			       const struct de_scaler_cal_lay_cfg *cfg,
			       const struct de_rect64_s *crop64,
			       const struct de_rect_s *scn_win,
			       struct de_rect_s *crop32,
			       struct de_scale_para *ypara,
			       struct de_scale_para *cpara)
{
	u64 val;
	struct de_scaler_private *priv = hdl->private;
	u32 scale_mode = 0, linebuf = 0;

	if (scn_win->width) {
		val = crop64->width;
		do_div(val, scn_win->width);
	} else {
		val = 0;
	}
	ypara->hstep = (u32)(val >> (32 - priv->step_frac_bit_width));

	if (scn_win->height) {
		val = crop64->height;
		do_div(val, scn_win->height);
	} else {
		val = 0;
	}
	ypara->vstep = (u32)(val >> (32 - priv->step_frac_bit_width));

	ypara->hphase =
	    (crop64->left & 0xffffffff) >> (32 - priv->phase_frac_bit_width);
	ypara->vphase =
	    (crop64->top & 0xffffffff) >> (32 - priv->phase_frac_bit_width);

	crop32->left = (s32)(crop64->left >> 32);
	crop32->top = (s32)(crop64->top >> 32);

	val = (crop64->width & 0xffffffff) + ((u64)(crop64->left) & 0xffffffff);
	crop32->width =
	    (val >> 32) ? ((crop64->width >> 32) + 1) : (crop64->width >> 32);

	val = (crop64->height & 0xffffffff) + ((u64)(crop64->top) & 0xffffffff);
	crop32->height =
	    (val >> 32) ? ((crop64->height >> 32) + 1) : (crop64->height >> 32);

	if (cfg->fm_space == DE_FORMAT_SPACE_RGB) {
		cpara->hstep = ypara->hstep;
		cpara->vstep = ypara->vstep;
		cpara->hphase = ypara->hphase;
		cpara->vphase = ypara->vphase;
		return 0;
	} else if (cfg->fm_space != DE_FORMAT_SPACE_YUV) {
		DRM_ERROR("calc cpara for fm_space(%d)!\n", cfg->fm_space);
		return -1;
	}

	if (cfg->yuv_sampling == DE_YUV422) {
		/* horizon crop info fix */
		if (((crop32->left & 0x1) == 0x0) &&
		    ((crop32->width & 0x1) == 0x1)) {
			/* odd crop_w, crop down width, */
			/* last line may disappear */
			crop32->width--;
		} else if (((crop32->left & 0x1) == 0x1) &&
			   ((crop32->width & 0x1) == 0x0)) {
			/* odd crop_x, crop down x, and phase + 1 */
			ypara->hphase += (1U << priv->phase_frac_bit_width);
			crop32->left--;
		} else if (((crop32->left & 0x1) == 0x1) &&
			   ((crop32->width & 0x1) == 0x1)) {
			/* odd crop_x and crop_w, */
			/* crop_x - 1, and phase + 1, crop_w + 1 */
			ypara->hphase += (1U << priv->phase_frac_bit_width);
			crop32->left--;
			crop32->width++;
		}

		cpara->hstep = ypara->hstep >> 1;
		cpara->vstep = ypara->vstep;
		cpara->hphase = ypara->hphase;
		cpara->vphase = ypara->vphase;
	} else if (cfg->yuv_sampling == DE_YUV420) {
		/* horizon crop info fix */
		if (((crop32->left & 0x1) == 0x0) &&
		    ((crop32->width & 0x1) == 0x1)) {
			/* odd crop_w, crop down width, */
			/* last line may disappear */
			crop32->width--;
		} else if (((crop32->left & 0x1) == 0x1) &&
			   ((crop32->width & 0x1) == 0x0)) {
			/* odd crop_x, crop down x, and phase + 1 */
			ypara->hphase += (1 << priv->phase_frac_bit_width);
			crop32->left--;
		} else if (((crop32->left & 0x1) == 0x1) &&
			   ((crop32->width & 0x1) == 0x1)) {
			/* odd crop_x and crop_w, crop_x - 1, */
			/* and phase + 1, crop_w + 1 */
			ypara->hphase += (1 << priv->phase_frac_bit_width);
			crop32->left--;
			crop32->width++;
		}
		/* vertical crop info fix */
		if (((crop32->top & 0x1) == 0x0) &&
		    ((crop32->height & 0x1) == 0x1)) {
			/* odd crop_h, crop down height, */
			/* last line may disappear */
			crop32->height--;
		} else if (((crop32->top & 0x1) == 0x1) &&
			   ((crop32->height & 0x1) == 0x0)) {
			/* odd crop_y, crop down y, and phase + 1 */
			ypara->vphase += (1 << priv->phase_frac_bit_width);
			crop32->top--;
		} else if (((crop32->top & 0x1) == 0x1) &&
			   ((crop32->height & 0x1) == 0x1)) {
			/* odd crop_y and crop_h, crop_y - 1, */
			/* and phase + 1, crop_h + 1 */
			ypara->vphase += (1 << priv->phase_frac_bit_width);
			crop32->top--;
			crop32->height++;
		}

		cpara->hstep = ypara->hstep >> 1;
		cpara->vstep = ypara->vstep >> 1;
		/* H.261, H.263, MPEG-1 sample method */
		/* cpara->hphase = (ypara->hphase>>1) */
		/* - ((N2_POWER(1,VSU_PHASE_FRAC_BITWIDTH))>>2); */
		/* MPEG-2, MPEG-4.2, H264, VC-1 sample method (default choise)*/
		cpara->hphase = ypara->hphase >> 1;
		if (cfg->snr_en) {
			scale_mode = 1;
			if (cfg->px_fmt == DE_FORMAT_YUV420_P ||
			    cfg->px_fmt == DE_FORMAT_YVU420_P ||
			    cfg->px_fmt == DE_FORMAT_YUV420_SP_UVUV ||
			    cfg->px_fmt == DE_FORMAT_YUV420_SP_VUVU ||
			    cfg->px_fmt ==
				DE_FORMAT_YUV420_SP_UVUV_10BIT ||
			    cfg->px_fmt ==
				DE_FORMAT_YUV420_SP_VUVU_10BIT ||
			    cfg->px_fmt == DE_FORMAT_YUV420_SP_VUVU) {
				/*cpara->vphase = 0x7b3333;*/
				/* chorma vertical phase should -0.6 when input
				 * format
				 * is */
				/* yuv420 and snr on */
				if (priv->dsc->type == DE_SCALER_TYPE_VSU_ED ||
				    priv->dsc->type == DE_SCALER_TYPE_ASU) {
					linebuf = priv->dsc->line_buffer_yuv_ed;
					if (((cfg->ovl_out_win.width <=
					      linebuf) &&
					     (cfg->ovl_ypara.hstep <
					      (1 << priv->step_frac_bit_width)) &&
					     (cfg->ovl_ypara.vstep <
					      (1 << priv->step_frac_bit_width)))) {
						scale_mode = 2;
					}
				}

				if (scale_mode == 2) {
					/* chorma vertical phase should
					 * -0.25
					 * when input format is */
					/* yuv420 */
					cpara->vphase =
					    (ypara->vphase >> 1) -
					    (1
					     << (priv->phase_frac_bit_width - 2));
				} else { /*scale_mode == 1*/
					cpara->vphase =
					    (ypara->vphase >> 1) -
					    (((1 << priv->phase_frac_bit_width) *
					      153) >>
					     8);
				}
			} else {
				cpara->vphase = ypara->vphase;
			}
		} else {
			/* chorma vertical phase should -0.25 when input format
			 * is */
			/* yuv420 */
			cpara->vphase = (ypara->vphase >> 1) -
					(1 << (priv->phase_frac_bit_width - 2));
		}
	} else if (cfg->yuv_sampling == DE_YUV411) {
		/* horizon crop info */
		if (((crop32->left & 0x3) == 0x0) &&
		    ((crop32->width & 0x3) != 0x0)) {
			/* odd crop_w, crop down width, */
			/* last 1-3 lines may disappear */
			crop32->width = (crop32->width >> 2) << 2;
		} else if (((crop32->left & 0x3) != 0x0) &&
			   ((crop32->width & 0x3) == 0x0)) {
			/* odd crop_x, crop down x, and phase + 1 */
			ypara->hphase +=
			    ((crop32->left & 0x3) << priv->phase_frac_bit_width);
			crop32->left = (crop32->left >> 2) << 2;
		} else if (((crop32->left & 0x3) != 0x0) &&
			   ((crop32->width & 0x3) != 0x0)) {
			/* odd crop_x and crop_w, crop_x aligned to 4 pixel */
			ypara->hphase +=
			    ((crop32->left & 0x3) << priv->phase_frac_bit_width);
			crop32->width =
			    ((crop32->width + (crop32->left & 0x3)) >> 2) << 2;
			crop32->left = (crop32->left >> 2) << 2;
		}

		cpara->hstep = ypara->hstep >> 2;
		cpara->vstep = ypara->vstep;
		cpara->hphase = ypara->hphase;
		cpara->vphase = ypara->vphase;
	} else {
		DRM_ERROR("not support yuv_sampling(%d)!\n",
		       cfg->yuv_sampling);
		return -1;
	}

	return 0;
}

u32 de_scaler_calc_ovl_coord(struct de_scaler_handle *hdl, u32 dst_coord, u32 scale_step)
{
	u32 half, src_coord;
	struct de_scaler_private *priv = hdl->private;

	half = (1 << (priv->step_frac_bit_width - 1));
	src_coord = (dst_coord * scale_step + half) >> priv->step_frac_bit_width;

	return src_coord;
}

s32 de_scaler_calc_ovl_scale_para(u32 layer_num, struct de_scale_para *ypara,
			       struct de_scale_para *cpara,
			       struct de_scale_para *ovl_ypara,
			       struct de_scale_para *ovl_cpara)
{
	u32 i;

	if (layer_num == 1) {
		/*only one layer enabled in one overlay */
		/* set overlay scale para through this layer */
		ovl_ypara->hphase = ypara[0].hphase;
		ovl_ypara->vphase = ypara[0].vphase;
		ovl_ypara->hstep = ypara[0].hstep;
		ovl_ypara->vstep = ypara[0].vstep;

		ovl_cpara->hphase = cpara[0].hphase;
		ovl_cpara->vphase = cpara[0].vphase;
		ovl_cpara->hstep = cpara[0].hstep;
		ovl_cpara->vstep = cpara[0].vstep;
	} else if (layer_num > 1) {
		/* two or more layers enabled in one overlay */
		/* set overlay scale step through first enabled layer */
		ovl_ypara->hstep = ypara[0].hstep;
		ovl_ypara->vstep = ypara[0].vstep;
		ovl_cpara->hstep = cpara[0].hstep;
		ovl_cpara->vstep = cpara[0].vstep;

		/* set overlay phase through 1st enabled non-zero-phase layer */
		for (i = 0; i < layer_num; i++) {
			if (ypara[i].hphase != 0) {
				ovl_ypara->hphase = ypara[i].hphase;
				ovl_cpara->hphase = cpara[i].hphase;
				break;
			}
		}
		/* all layer phase equal to zero */
		if (i == layer_num) {
			ovl_ypara->hphase = ypara[0].hphase;
			ovl_cpara->hphase = cpara[0].hphase;
		}

		/* set overlay phase through first non-zero layer */
		for (i = 0; i < layer_num; i++) {
			if (ypara[i].vphase != 0) {
				ovl_ypara->vphase = ypara[i].vphase;
				ovl_cpara->vphase = cpara[i].vphase;
				break;
			}
		}
		/* all layer phase equal to zero */
		if (i == layer_num) {
			ovl_ypara->vphase = ypara[0].vphase;
			ovl_cpara->vphase = cpara[0].vphase;
		}
	}

	return 0;
}

s32 de_scaler_calc_scale_para(struct de_scaler_handle *hdl, u32 fix_size_result, enum de_format_space fmt_space,
			   enum de_yuv_sampling yuv_sampling,
			   struct de_rect_s *out_win, struct de_rect_s *ywin,
			   struct de_rect_s *cwin, struct de_scale_para *ypara,
			   struct de_scale_para *cpara)
{
	u32 wshift = 0;
	u32 hshift = 0;
	struct de_scaler_private *priv = hdl->private;

	if (fmt_space == DE_FORMAT_SPACE_YUV) {
		if (yuv_sampling == DE_YUV422) {
			wshift = 1;
		} else if (yuv_sampling == DE_YUV420) {
			wshift = 1;
			hshift = 1;
		} else if (yuv_sampling == DE_YUV411) {
			wshift = 2;
		}
	}
	cwin->width = ywin->width >> wshift;
	ywin->width = cwin->width << wshift;
	cwin->height = ywin->height >> hshift;
	ywin->height = cwin->height << hshift;

	if (fix_size_result & VSU_CUT_INWIDTH) {
		u64 val;

		val = (u64)ywin->width << priv->step_frac_bit_width;
		do_div(val, out_win->width);
		ypara->hstep = (u32)val;
		ypara->hphase = 0; /* no meaning when coarse scale using */

		cwin->width = ywin->width >> wshift;
		val = (u64)cwin->width << priv->step_frac_bit_width;
		do_div(val, out_win->width);
		cpara->hstep = (u32)val;
		cpara->hphase = 0; /* no meaning when coarse scale using */
	}
	if (fix_size_result & (VSU_CUT_INHEIGHT | RTMX_CUT_INHEIGHT)) {
		u64 val;

		val = (u64)ywin->height << VSU_STEP_FRAC_BITWIDTH;
		do_div(val, out_win->height);
		ypara->vstep = (u32)val;
		ypara->vphase = 0; /* no meaning when coarse scale using */

		cwin->height = ywin->height >> hshift;
		val = (u64)cwin->height << VSU_STEP_FRAC_BITWIDTH;
		do_div(val, out_win->height);
		cpara->vstep = (u32)val;
		cpara->vphase = 0; /* no meaning when coarse scale using */
	}

	return 0;
}

u32 de_scaler_fix_tiny_size(struct de_scaler_handle *hdl, struct de_rect_s *in_win,
			 struct de_rect_s *out_win, struct de_scale_para *ypara,
			 enum de_format_space fmt_space,
			 struct de_rect_s *lay_win, u32 lay_num, u32 scn_width,
			 u32 scn_height)
{
	u32 result = 0x0;
	struct de_scaler_private *priv = hdl->private;

	if (!in_win->width || !in_win->height || !out_win->width ||
	    !out_win->height || !ypara->hstep || !ypara->vstep)
		return result;

	/* horizon */
	if (in_win->width < VSU_MIN_INPUT_WIDTH ||
	    out_win->width < VSU_MIN_INPUT_WIDTH) {
		u32 org_out_win_width = out_win->width;
		u32 shift = priv->phase_frac_bit_width;

		if (ypara->hstep > (1 << shift)) {
			/* scale down */
			u64 val;
			out_win->width = VSU_MIN_INPUT_WIDTH;
			val = (u64)(in_win->width) << shift;
			do_div(val, VSU_MIN_INPUT_WIDTH);
			ypara->hstep = (u32)val;
			result |= VSU_EXPAND_OUTWIDTH;
		} else {
			/* scale up */
			in_win->width = VSU_MIN_INPUT_WIDTH;
			out_win->width =
			    VSU_MIN_INPUT_WIDTH * (1 << shift) / ypara->hstep;
			result |= (VSU_EXPAND_OUTWIDTH | VSU_EXPAND_OUTWIDTH);
		}

		if (out_win->width + out_win->left > scn_width) {
			u32 i;

			out_win->left -= (out_win->width - org_out_win_width);
			for (i = 0; i < lay_num; i++) {
				lay_win[i].left +=
				    ((ypara->hstep *
				      (out_win->width - org_out_win_width)) >>
				     shift);
			}
			result |= VSU_LSHIFT_OUTWINDOW;
		}
	}

	/* vertical */
	if (in_win->height < VSU_MIN_INPUT_HEIGHT ||
	    out_win->height < VSU_MIN_INPUT_HEIGHT) {
		u32 org_out_win_height = out_win->height;
		u32 shift = priv->phase_frac_bit_width;

		if (ypara->vstep > (1 << shift)) {
			/* scale down */
			u64 val;
			out_win->height = VSU_MIN_INPUT_HEIGHT;
			val = (u64)(in_win->height) << shift;
			do_div(val, VSU_MIN_INPUT_HEIGHT);
			ypara->vstep = (u32)val;
			result |= VSU_EXPAND_OUTHEIGHT;
		} else {
			/* scale up */
			in_win->height = VSU_MIN_INPUT_HEIGHT;
			out_win->height =
			    VSU_MIN_INPUT_HEIGHT * (1 << shift) / ypara->vstep;
			result |= (VSU_EXPAND_OUTHEIGHT | VSU_EXPAND_INHEIGHT);
		}

		if (out_win->height + out_win->top > scn_height) {
			u32 i;

			out_win->top -= (out_win->height - org_out_win_height);
			for (i = 0; i < lay_num; i++) {
				lay_win[i].top +=
				    ((ypara->vstep *
				      (out_win->height - org_out_win_height)) >>
				     shift);
			}
			result |= VSU_USHIFT_OUTWINDOW;
		}
	}

	return result;
}

u32 de_scaler_fix_big_size(struct de_scaler_handle *hdl, struct de_rect_s *in_win,
			const struct de_rect_s *out_win,
			enum de_format_space fmt_space,
			enum de_yuv_sampling yuv_sampling)
{
	struct de_scaler_private *priv = hdl->private;

	u32 result = 0;
	u32 wshift = 0;
	u32 hshift = 0;
	u32 in_width, in_height;
	u32 linebuf;
	u32 value;

	if (fmt_space == DE_FORMAT_SPACE_YUV) {
		if (yuv_sampling == DE_YUV422) {
			wshift = 1;
		} else if (yuv_sampling == DE_YUV420) {
			wshift = 1;
			hshift = 1;
		} else if (yuv_sampling == DE_YUV411) {
			wshift = 2;
		}
		linebuf = priv->dsc->line_buffer_yuv;
	} else if (fmt_space == DE_FORMAT_SPACE_RGB) {
		linebuf = priv->dsc->line_buffer_rgb;
	} else {
		linebuf = 2048;
	}

	in_width = in_win->width;
	in_height = in_win->height;

	if (in_width > linebuf) {
		in_width = linebuf;
	}
	value = priv->tap_num_hori * out_win->width;
	if (in_width > value)
		in_width = value;
	in_width &= (~((1U << wshift) - 1));
	if (in_width < in_win->width) {
		in_win->width = in_width;
		result |= VSU_CUT_INWIDTH;
	}

	value = priv->tap_num_vert * out_win->height;
	if (in_height > value)
		in_height = value;
	in_height &= (~((1 << hshift) - 1));
	if (in_height < in_win->height) {
		in_win->height = in_height;
		result |= VSU_CUT_INHEIGHT;
	}

	return result;
}

static s32 de_vsu8_set_para(struct de_scaler_handle *hdl, const struct de_scaler_apply_cfg *cfg)
{
	struct de_scaler_private *priv = hdl->private;
	struct vsu8_reg *reg = get_vsu8_reg(priv);
	u32 scale_mode = 0;
	u32 pt_coef;

	if (cfg->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		if (cfg->yuv_sampling == DE_YUV422) {
			switch (cfg->px_fmt) {
			case DE_FORMAT_YUV422_I_YVYU:
			case DE_FORMAT_YUV422_I_YUYV:
			case DE_FORMAT_YUV422_I_UYVY:
			case DE_FORMAT_YUV422_I_VYUY:
			case DE_FORMAT_YUV422_I_YVYU_10BIT:
			case DE_FORMAT_YUV422_I_YUYV_10BIT:
			case DE_FORMAT_YUV422_I_UYVY_10BIT:
			case DE_FORMAT_YUV422_I_VYUY_10BIT:
				scale_mode = 0;
				break;
			default:
				scale_mode = 1;
				break;
			}
		} else if (cfg->yuv_sampling == DE_YUV420) {
			scale_mode = 1;
		} else if (cfg->yuv_sampling == DE_YUV411) {
			scale_mode = 1;
		} else {
			DRM_ERROR("yuv_sampling=%d\n", cfg->yuv_sampling);
		}
	} else if (cfg->px_fmt_space != DE_FORMAT_SPACE_RGB) {
		DRM_ERROR("px_fmt_space=%d\n", cfg->px_fmt_space);
		return -1;
	}

	reg->ctl.bits.en = 1;
	reg->scale_mode.dwval = scale_mode;
	scaler_set_block_dirty(priv, VSU8_REG_BLK_CTL, 1);

	reg->out_size.bits.width =
	    cfg->scn_win.width ? (cfg->scn_win.width - 1) : 0;
	reg->out_size.bits.height =
	    cfg->scn_win.height ? (cfg->scn_win.height - 1) : 0;

	reg->glb_alpha.dwval = cfg->glb_alpha;
	scaler_set_block_dirty(priv, VSU8_REG_BLK_ATTR, 1);

	reg->y_in_size.bits.width =
	    cfg->ovl_out_win.width ? (cfg->ovl_out_win.width - 1) : 0;
	reg->y_in_size.bits.height = cfg->ovl_out_win.height
					 ? (cfg->ovl_out_win.height - 1)
					 : 0;
	reg->y_hstep.dwval = cfg->ovl_ypara.hstep
			     << VSU8_STEP_VALID_START_BIT;
	reg->y_vstep.dwval = cfg->ovl_ypara.vstep
			     << VSU8_STEP_VALID_START_BIT;
	reg->y_hphase.dwval = cfg->ovl_ypara.hphase
			      << VSU8_PHASE_VALID_START_BIT;
	reg->y_vphase.dwval = cfg->ovl_ypara.vphase
			      << VSU8_PHASE_VALID_START_BIT;
	scaler_set_block_dirty(priv, VSU8_REG_BLK_YPARA, 1);

	reg->c_in_size.bits.width =
	    cfg->c_win.width ? (cfg->c_win.width - 1) : 0;
	reg->c_in_size.bits.height =
	    cfg->c_win.height ? (cfg->c_win.height - 1) : 0;
	reg->c_hstep.dwval = cfg->ovl_cpara.hstep
			     << VSU8_STEP_VALID_START_BIT;
	reg->c_vstep.dwval = cfg->ovl_cpara.vstep
			     << VSU8_STEP_VALID_START_BIT;
	reg->c_hphase.dwval = cfg->ovl_cpara.hphase
			      << VSU8_PHASE_VALID_START_BIT;
	reg->c_vphase.dwval = cfg->ovl_cpara.vphase
			      << VSU8_PHASE_VALID_START_BIT;
	scaler_set_block_dirty(priv, VSU8_REG_BLK_CPARA, 1);

	/* fir coefficient */
	/* ch0 */
	pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_ypara.hstep);
	memcpy(reg->y_hori_coeff, lan2coefftab32 + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);
	pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_ypara.vstep);
	memcpy(reg->y_vert_coeff, lan2coefftab32 + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);

	/* ch1/2 */
	if (cfg->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff, lan2coefftab32 + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
	} else {
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff, bicubic4coefftab32 + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
	}
	scaler_set_block_dirty(priv, VSU8_REG_BLK_COEFF0, 1);
	scaler_set_block_dirty(priv, VSU8_REG_BLK_COEFF1, 1);
	scaler_set_block_dirty(priv, VSU8_REG_BLK_COEFF2, 1);

	return 0;
}

static s32 de_vsu10_set_para(struct de_scaler_handle *hdl, const struct de_scaler_apply_cfg *cfg)
{
	struct de_scaler_private *priv = hdl->private;
	struct vsu10_reg *reg = get_vsu10_reg(priv);
	u32 scale_mode = 0;
	u32 pt_coef;

	if (cfg->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		switch (cfg->yuv_sampling) {
		case DE_YUV444:
			scale_mode = 0;
			break;
		case DE_YUV422:
		case DE_YUV420:
		case DE_YUV411:
			scale_mode = 1;
			break;
		default:
			DRM_ERROR("yuv_sampling=%d\n", cfg->yuv_sampling);
			return -1;
		}
	} else if (cfg->px_fmt_space != DE_FORMAT_SPACE_RGB) {
		DRM_ERROR("px_fmt_space=%d\n", cfg->px_fmt_space);
		return -1;
	}

	reg->ctl.bits.en = 1;
	reg->scale_mode.dwval = scale_mode;
	if (priv->dsc->need_switch_en)
		reg->ctl.bits.switch_en = 1;
	scaler_set_block_dirty(priv, VSU10_REG_BLK_CTL, 1);

	reg->out_size.bits.width =
	    cfg->scn_win.width ? (cfg->scn_win.width - 1) : 0;
	reg->out_size.bits.height =
	    cfg->scn_win.height ? (cfg->scn_win.height - 1) : 0;

	reg->glb_alpha.dwval = cfg->glb_alpha;
	scaler_set_block_dirty(priv, VSU10_REG_BLK_ATTR, 1);

	reg->y_in_size.bits.width =
	    cfg->ovl_out_win.width ? (cfg->ovl_out_win.width - 1) : 0;
	reg->y_in_size.bits.height = cfg->ovl_out_win.height
					 ? (cfg->ovl_out_win.height - 1)
					 : 0;
	reg->y_hstep.dwval = cfg->ovl_ypara.hstep
			     << VSU10_STEP_VALID_START_BIT;
	reg->y_vstep.dwval = cfg->ovl_ypara.vstep
			     << VSU10_STEP_VALID_START_BIT;
	reg->y_hphase.dwval = cfg->ovl_ypara.hphase
			      << VSU10_PHASE_VALID_START_BIT;
	reg->y_vphase0.dwval = cfg->ovl_ypara.vphase
			       << VSU10_PHASE_VALID_START_BIT;
	scaler_set_block_dirty(priv, VSU10_REG_BLK_YPARA, 1);

	reg->c_in_size.bits.width =
	    cfg->c_win.width ? (cfg->c_win.width - 1) : 0;
	reg->c_in_size.bits.height =
	    cfg->c_win.height ? (cfg->c_win.height - 1) : 0;
	reg->c_hstep.dwval = cfg->ovl_cpara.hstep
			     << VSU10_STEP_VALID_START_BIT;
	reg->c_vstep.dwval = cfg->ovl_cpara.vstep
			     << VSU10_STEP_VALID_START_BIT;
	reg->c_hphase.dwval = cfg->ovl_cpara.hphase
			      << VSU10_PHASE_VALID_START_BIT;
	reg->c_vphase0.dwval = cfg->ovl_cpara.vphase
			       << VSU10_PHASE_VALID_START_BIT;
	scaler_set_block_dirty(priv, VSU10_REG_BLK_CPARA, 1);

	/* fir coefficient */
	/* ch0 */
	pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_ypara.hstep);
	memcpy(reg->y_hori_coeff0, lan3coefftab32_left + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);
	memcpy(reg->y_hori_coeff1, lan3coefftab32_right + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);
	pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_ypara.vstep);
	memcpy(reg->y_vert_coeff, lan2coefftab32 + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);

	/* ch1/2 */
	if (cfg->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff0, lan3coefftab32_left + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hori_coeff1, lan3coefftab32_right + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.vstep);
		memcpy(reg->c_vert_coeff, lan2coefftab32 + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
	} else {
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff0, bicubic8coefftab32_left + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hori_coeff1, bicubic8coefftab32_right + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.vstep);
		memcpy(reg->c_vert_coeff, bicubic4coefftab32 + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
	}
	scaler_set_block_dirty(priv, VSU10_REG_BLK_COEFF0, 1);
	scaler_set_block_dirty(priv, VSU10_REG_BLK_COEFF1, 1);
	scaler_set_block_dirty(priv, VSU10_REG_BLK_COEFF2, 1);
	scaler_set_block_dirty(priv, VSU10_REG_BLK_COEFF3, 1);
	scaler_set_block_dirty(priv, VSU10_REG_BLK_COEFF4, 1);
	scaler_set_block_dirty(priv, VSU10_REG_BLK_COEFF5, 1);

	return 0;
}

static s32 de_vsu_ed_set_para(struct de_scaler_handle *hdl, const struct de_scaler_apply_cfg *cfg)
{
	struct de_scaler_private *priv = hdl->private;
	struct vsu_ed_reg *reg = get_vsu_ed_reg(priv);
	u32 scale_mode = 0;
	u32 pt_coef;

	if (cfg->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		u32 linebuf = priv->dsc->line_buffer_yuv_ed;
		switch (cfg->yuv_sampling) {
		case DE_YUV444:
			scale_mode = 0;
			break;
		case DE_YUV422:
		case DE_YUV420:
		case DE_YUV411:
			if ((cfg->ovl_out_win.width <= linebuf) &&
			    (cfg->ovl_ypara.hstep <
			     (1 << VSU_STEP_FRAC_BITWIDTH)) &&
			    (cfg->ovl_ypara.vstep <
			     (1 << VSU_STEP_FRAC_BITWIDTH)))
				scale_mode = 2;
			else
				scale_mode = 1;
			break;
		default:
			DRM_ERROR("yuv_sampling=%d\n", cfg->yuv_sampling);
			return -1;
		}
	} else if (cfg->px_fmt_space != DE_FORMAT_SPACE_RGB) {
		DRM_ERROR("px_fmt_space=%d\n", cfg->px_fmt_space);
		return -1;
	}

	reg->ctl.bits.en = 1;
	reg->scale_mode.dwval = scale_mode;
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_CTL, 1);

	reg->dir_thr.dwval = 0x0000FF01;
	reg->edge_thr.dwval = 0x00080000;
	reg->dir_ctl.dwval = 0x00000000;
	reg->angle_thr.dwval = 0x00020000;
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_DIR_SHP, 1);

	reg->out_size.bits.width =
	    cfg->scn_win.width ? (cfg->scn_win.width - 1) : 0;
	reg->out_size.bits.height =
	    cfg->scn_win.height ? (cfg->scn_win.height - 1) : 0;

	reg->glb_alpha.dwval = cfg->glb_alpha;
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_ATTR, 1);

	reg->y_in_size.bits.width =
	    cfg->ovl_out_win.width ? (cfg->ovl_out_win.width - 1) : 0;
	reg->y_in_size.bits.height = cfg->ovl_out_win.height
					 ? (cfg->ovl_out_win.height - 1)
					 : 0;
	reg->y_hstep.dwval = cfg->ovl_ypara.hstep
			     << VSU_ED_STEP_VALID_START_BIT;
	reg->y_vstep.dwval = cfg->ovl_ypara.vstep
			     << VSU_ED_STEP_VALID_START_BIT;
	reg->y_hphase.dwval = cfg->ovl_ypara.hphase
			      << VSU_ED_PHASE_VALID_START_BIT;
	reg->y_vphase0.dwval = cfg->ovl_ypara.vphase
			       << VSU_ED_PHASE_VALID_START_BIT;
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_YPARA, 1);

	reg->c_in_size.bits.width =
	    cfg->c_win.width ? (cfg->c_win.width - 1) : 0;
	reg->c_in_size.bits.height =
	    cfg->c_win.height ? (cfg->c_win.height - 1) : 0;
	reg->c_hstep.dwval = cfg->ovl_cpara.hstep
			     << VSU_ED_STEP_VALID_START_BIT;
	reg->c_vstep.dwval = cfg->ovl_cpara.vstep
			     << VSU_ED_STEP_VALID_START_BIT;
	reg->c_hphase.dwval = cfg->ovl_cpara.hphase
			      << VSU_ED_PHASE_VALID_START_BIT;
	reg->c_vphase0.dwval = cfg->ovl_cpara.vphase
			       << VSU_ED_PHASE_VALID_START_BIT;
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_CPARA, 1);

	/* fir coefficient */
	/* ch0 */
	pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_ypara.hstep);
	memcpy(reg->y_hori_coeff0, lan3coefftab32_left + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);
	memcpy(reg->y_hori_coeff1, lan3coefftab32_right + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);
	pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_ypara.vstep);
	memcpy(reg->y_vert_coeff, lan2coefftab32 + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);

	/* ch1/2 */
	if (cfg->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff0, lan3coefftab32_left + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hori_coeff1, lan3coefftab32_right + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.vstep);
		memcpy(reg->c_vert_coeff, lan2coefftab32 + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
	} else {
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff0, bicubic8coefftab32_left + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hori_coeff1, bicubic8coefftab32_right + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.vstep);
		memcpy(reg->c_vert_coeff, bicubic4coefftab32 + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
	}
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_COEFF0, 1);
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_COEFF1, 1);
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_COEFF2, 1);
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_COEFF3, 1);
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_COEFF4, 1);
	scaler_set_block_dirty(priv, VSU_ED_REG_BLK_COEFF5, 1);

	return 0;
}

static s32 de_gsu_set_para(struct de_scaler_handle *hdl, const struct de_scaler_apply_cfg *cfg)
{
	struct de_scaler_private *priv = hdl->private;
	struct gsu_reg *reg = get_gsu_reg(priv);
	u32 pt_coef;

	if (cfg->px_fmt_space != DE_FORMAT_SPACE_RGB) {
		DRM_ERROR("px_fmt_space=%d\n", cfg->px_fmt_space);
		return -1;
	}

	reg->ctl.bits.en = 1;
	if (priv->dsc->need_switch_en)
		reg->ctl.bits.switch_en = 1;
	scaler_set_block_dirty(priv, GSU_REG_BLK_CTL, 1);

	reg->out_size.bits.width =
	    cfg->scn_win.width ? (cfg->scn_win.width - 1) : 0;
	reg->out_size.bits.height =
	    cfg->scn_win.height ? (cfg->scn_win.height - 1) : 0;

	/* reg->glb_alpha.dwval = cfg->glb_alpha; */
	scaler_set_block_dirty(priv, GSU_REG_BLK_ATTR, 1);

	reg->in_size.bits.width =
	    cfg->ovl_out_win.width ? (cfg->ovl_out_win.width - 1) : 0;
	reg->in_size.bits.height = cfg->ovl_out_win.height
					? (cfg->ovl_out_win.height - 1) : 0;
	reg->hstep.dwval = cfg->ovl_ypara.hstep << GSU_STEP_VALID_START_BIT;
	reg->vstep.dwval = cfg->ovl_ypara.vstep << GSU_STEP_VALID_START_BIT;
	reg->hphase.dwval = cfg->ovl_ypara.hphase << GSU_PHASE_VALID_START_BIT;
	reg->vphase0.dwval = cfg->ovl_ypara.vphase << GSU_PHASE_VALID_START_BIT;
	scaler_set_block_dirty(priv, GSU_REG_BLK_PARA, 1);

	/* fir coefficient */
	/* ch0 */
	pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_ypara.hstep);
	memcpy(reg->hcoeff, lan2coefftab16 + pt_coef, sizeof(u32) * GSU_PHASE_NUM);

	scaler_set_block_dirty(priv, GSU_REG_BLK_COEFF0, 1);
	return 0;
}



static s32 de_asu_set_para(struct de_scaler_handle *hdl, const struct de_scaler_apply_cfg *cfg)
{
	struct de_scaler_private *priv = hdl->private;
	struct asu_reg *reg = get_asu_reg(priv);
	u32 scale_mode = 0;
	u32 lb_mode = 0;
	u32 pt_coef;

	if (cfg->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		u32 linebuf = priv->dsc->line_buffer_yuv_ed;

		switch (cfg->yuv_sampling) {
		case DE_YUV444:
			scale_mode = 0;
			break;
		case DE_YUV422:
		case DE_YUV420:
		case DE_YUV411:
			if ((cfg->ovl_out_win.width <= linebuf) &&
			    (cfg->ovl_ypara.hstep <
			     (1 << VSU_STEP_FRAC_BITWIDTH)) &&
			    (cfg->ovl_ypara.vstep <
			     (1 << VSU_STEP_FRAC_BITWIDTH)))
				scale_mode = 2;
			else
				scale_mode = 1;
			break;
		default:
			DRM_ERROR("yuv_sampling=%d\n", cfg->yuv_sampling);
			return -1;
		}
	} else if (cfg->px_fmt_space != DE_FORMAT_SPACE_RGB) {
		DRM_ERROR("px_fmt_space=%d\n", cfg->px_fmt_space);
		return -1;
	}

	if (cfg->px_fmt_space == DE_FORMAT_SPACE_RGB ||
	    (cfg->px_fmt_space == DE_FORMAT_SPACE_YUV && cfg->yuv_sampling == DE_YUV444)) {
		u32 line_mode_size = priv->dsc->line_buffer_rgb / 2;

		if (cfg->ovl_out_win.width > line_mode_size)
			lb_mode = 1;
	}

	reg->ctl.dwval = 0x111;
	reg->scale_mode.dwval = ((lb_mode & 0x1) << 16) | (scale_mode & 0x3);
	reg->diret_thr.dwval = 0x280414;
	reg->g_alpha.dwval = cfg->glb_alpha;
	scaler_set_block_dirty(priv, ASU_REG_BLK_CTL, 1);

	reg->out_size.bits.width =
	    cfg->scn_win.width ? (cfg->scn_win.width - 1) : 0;
	reg->out_size.bits.height =
	    cfg->scn_win.height ? (cfg->scn_win.height - 1) : 0;

	scaler_set_block_dirty(priv, ASU_REG_BLK_ATTR, 1);

	reg->y_size.bits.y_width =
	    cfg->ovl_out_win.width ? (cfg->ovl_out_win.width - 1) : 0;
	reg->y_size.bits.y_height = cfg->ovl_out_win.height
					? (cfg->ovl_out_win.height - 1)
					: 0;
	reg->y_hstep.dwval = cfg->ovl_ypara.hstep
			     << priv->step_valid_start_bit;
	reg->y_vstep.dwval = cfg->ovl_ypara.vstep
			     << priv->step_valid_start_bit;
	reg->y_hphase.dwval = cfg->ovl_ypara.hphase
			      << VSU_PHASE_VALID_START_BIT;
	reg->y_vphase0.dwval = cfg->ovl_ypara.vphase
			       << VSU_PHASE_VALID_START_BIT;
	scaler_set_block_dirty(priv, ASU_REG_BLK_YPARA, 1);

	reg->c_size.bits.c_width =
	    cfg->c_win.width ? (cfg->c_win.width - 1) : 0;
	reg->c_size.bits.c_height =
	    cfg->c_win.height ? (cfg->c_win.height - 1) : 0;
	reg->c_hstep.dwval = cfg->ovl_cpara.hstep
			     << priv->step_valid_start_bit;
	reg->c_vstep.dwval = cfg->ovl_cpara.vstep
			     << priv->step_valid_start_bit;
	reg->c_hphase.dwval = cfg->ovl_cpara.hphase
			      << VSU_PHASE_VALID_START_BIT;
	reg->c_vphase0.dwval = cfg->ovl_cpara.vphase
			       << VSU_PHASE_VALID_START_BIT;
	/*if (cfg->px_fmt_space == DE_FORMAT_SPACE_YUV
	    && cfg->yuv_sampling == DE_YUV420
	    && (reg->y_size.bits.y_width * reg->y_size.bits.y_height
	    < reg->out_size.bits.width * reg->out_size.bits.height)) {
		reg->c_vphase0.dwval = 0xf80000;
		reg->c_vphase1.dwval = 0xf80000;
	} else if (cfg->snr_en) {
		reg->c_vphase0.dwval = 0x0;
		reg->c_vphase1.dwval = 0x0;
		reg->c_hphase.dwval = 0x0;
		reg->y_vphase0.dwval = 0x0;
		reg->y_vphase1.dwval = 0x0;
		reg->y_hphase.dwval = 0x0;
	}*/
	scaler_set_block_dirty(priv, ASU_REG_BLK_CPARA, 1);

	/* fir coefficient */
	/* ch0 */
	pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_ypara.hstep);
	memcpy(reg->y_hcoef0, lan3coefftab32_left + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);
	memcpy(reg->y_hcoef1, lan3coefftab32_right + pt_coef,
	       sizeof(u32) * VSU_PHASE_NUM);
	if (lb_mode) {
		memcpy(reg->y_vcoef, linearcoefftab32_4tab, sizeof(linearcoefftab32_4tab));
	} else {
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_ypara.vstep);
		memcpy(reg->y_vcoef, lan2coefftab32 + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
	}

	/* ch1/2 */
	if (cfg->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.hstep);
		memcpy(reg->c_hcoef0, lan3coefftab32_left + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hcoef1, lan3coefftab32_right + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);

		if (lb_mode) {
			memcpy(reg->c_vcoef, linearcoefftab32_4tab, sizeof(linearcoefftab32_4tab));
		} else {
			pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.vstep);
			memcpy(reg->c_vcoef, lan2coefftab32 + pt_coef,
			       sizeof(u32) * VSU_PHASE_NUM);
		}
	} else {
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.hstep);
		memcpy(reg->c_hcoef0, bicubic8coefftab32_left + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hcoef1, bicubic8coefftab32_right + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
		pt_coef = de_scaler_calc_fir_coef(hdl->private->dsc->type, cfg->ovl_cpara.vstep);
		memcpy(reg->c_vcoef, bicubic4coefftab32 + pt_coef,
		       sizeof(u32) * VSU_PHASE_NUM);
	}
	scaler_set_block_dirty(priv, ASU_REG_BLK_COEFF0, 1);
	scaler_set_block_dirty(priv, ASU_REG_BLK_COEFF1, 1);
	scaler_set_block_dirty(priv, ASU_REG_BLK_COEFF2, 1);
	scaler_set_block_dirty(priv, ASU_REG_BLK_COEFF3, 1);
	scaler_set_block_dirty(priv, ASU_REG_BLK_COEFF4, 1);
	scaler_set_block_dirty(priv, ASU_REG_BLK_COEFF5, 1);

	return 0;
}

static s32 de_scaler_disable(struct de_scaler_handle *hdl)
{
	struct de_scaler_private *priv = hdl->private;

	if (priv->dsc->type == DE_SCALER_TYPE_VSU8) {
		struct vsu8_reg *reg = get_vsu8_reg(priv);
		reg->ctl.bits.en = 0;
	} else if (priv->dsc->type == DE_SCALER_TYPE_VSU10) {
		struct vsu10_reg *reg = get_vsu10_reg(priv);
		reg->ctl.bits.en = 0;
	} else if (priv->dsc->type == DE_SCALER_TYPE_VSU_ED) {
		struct vsu_ed_reg *reg = get_vsu_ed_reg(priv);
		reg->ctl.bits.en = 0;
	} else if (priv->dsc->type == DE_SCALER_TYPE_ASU) {
		struct asu_reg *reg = get_asu_reg(priv);
		reg->ctl.dwval = 0;
	} else if (priv->dsc->type == DE_SCALER_TYPE_GSU) {
		struct gsu_reg *reg = get_gsu_reg(priv);
		reg->ctl.dwval = 0;
	}

	scaler_set_block_dirty(priv, VSU8_REG_BLK_CTL, 1);
	return 0;
}

void dump_scaler_state(struct drm_printer *p, struct de_scaler_handle *hdl, const struct display_channel_state *state)
{
	struct scaler_debug_info *debug = &hdl->private->debug;
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;
	struct de_scaler_private *priv = hdl->private;
	struct asu_reg *reg = get_asu_reg(priv);

	drm_printf(p, "\n\tscaler@%8x: %sable\n", (unsigned int)(base - de_base), debug->enable ? "en" : "dis");
	if (debug->enable) {
		drm_printf(p, "\t\t%4dx%4d ==> %4dx%4d\n", debug->in_w, debug->in_h, debug->out_w, debug->out_h);
		if (priv->dsc->type == DE_SCALER_TYPE_ASU)
			drm_printf(p, "\t\t asu_pq %sable\n", reg->peaking_en.bits.peaking_en ? "en" : "dis");
	}
}

bool de_scaler_pq_is_enabled(struct de_scaler_handle *hdl)
{
	struct de_scaler_private *priv = hdl->private;
	struct asu_reg *reg = get_asu_reg(priv);

	if (priv->dsc->type != DE_SCALER_TYPE_ASU)
		return false;
	return !!(reg->peaking_en.bits.peaking_en);
}

s32 de_scaler_asu_pq_enable(struct de_scaler_handle *hdl, u32 en)
{
	struct de_scaler_private *priv = hdl->private;
	struct asu_reg *reg = get_asu_reg(priv);
	if (priv->dsc->type != DE_SCALER_TYPE_ASU) {
		DRM_ERROR("scaler %d is not asu\n", hdl->cinfo.id);
		return -1;
	} else {
		/* restore all pq config */
		if (priv->pq_init && en)
			reg->peaking_en.bits.peaking_en = 1;
		else
			reg->peaking_en.bits.peaking_en = 0;

		DRM_DEBUG_DRIVER("%s %d\n", __func__, !!reg->peaking_en.bits.peaking_en);
		scaler_set_block_dirty(priv, ASU_REG_BLK_PEAKING, 1);
	}
	return 0;
}

s32 de_scaler_apply_asu_pq_config(struct de_scaler_handle *hdl, asu_module_param_t *para)
{
	struct de_scaler_private *priv = hdl->private;
	struct asu_reg *reg = get_asu_reg(priv);
	if (priv->dsc->type != DE_SCALER_TYPE_ASU) {
		DRM_ERROR("scaler %d is not asu\n", hdl->cinfo.id);
		return -1;
	} else {
		if (para->cmd == PQ_READ) {
			para->value[0] = reg->peaking_en.bits.peaking_en;
			para->value[1] = reg->peaking_en.bits.local_clamp_en;
			para->value[2] = reg->peaking_en.bits.cpeaking_en;
			para->value[3] = reg->peaking_stren.bits.peaking_strength;
			para->value[4] = reg->peaking_limit.bits.peaking_range_limit;
			para->value[5] = reg->peaking_para.bits.th_strong_edge;
			para->value[6] = reg->peaking_para.bits.peak_weights_strength;
			para->value[7] = reg->peaking_stren.bits.cpeaking_strength;
			para->value[8] = reg->peaking_limit.bits.cpeaking_range_limit;
		} else {
			reg->peaking_en.bits.peaking_en              = para->value[0];
			reg->peaking_en.bits.local_clamp_en          = para->value[1];
			reg->peaking_en.bits.cpeaking_en             = para->value[2];
			reg->peaking_stren.bits.peaking_strength     = para->value[3];
			reg->peaking_limit.bits.peaking_range_limit  = para->value[4];
			reg->peaking_para.bits.th_strong_edge        = para->value[5];
			reg->peaking_para.bits.peak_weights_strength = para->value[6];
			reg->peaking_stren.bits.cpeaking_strength    = para->value[7];
			reg->peaking_limit.bits.cpeaking_range_limit = para->value[8];
			priv->pq_init = true;
			scaler_set_block_dirty(priv, ASU_REG_BLK_PEAKING, 1);
		}
	}
	return 0;
}

s32 de_scaler_apply(struct de_scaler_handle *hdl, const struct de_scaler_apply_cfg *cfg)
{
	struct de_scaler_private *priv = hdl->private;

	if (!cfg->scale_en) {
		priv->debug.enable = false;
		priv->debug.in_w = 0;
		priv->debug.in_h = 0;
		priv->debug.out_w = 0;
		priv->debug.out_h = 0;
		return de_scaler_disable(hdl);
	}
	priv->debug.enable = true;
	priv->debug.in_w = cfg->ovl_out_win.width;
	priv->debug.in_h = cfg->ovl_out_win.height;
	priv->debug.out_w = cfg->scn_win.width;
	priv->debug.out_h = cfg->scn_win.height;

	if (priv->dsc->type == DE_SCALER_TYPE_VSU8) {
		return de_vsu8_set_para(hdl, cfg);
	} else if (priv->dsc->type == DE_SCALER_TYPE_VSU10) {
		return de_vsu10_set_para(hdl, cfg);
	} else if (priv->dsc->type == DE_SCALER_TYPE_VSU_ED) {
		return de_vsu_ed_set_para(hdl, cfg);
	} else if (priv->dsc->type == DE_SCALER_TYPE_ASU) {
		return de_asu_set_para(hdl, cfg);
	} else if (priv->dsc->type == DE_SCALER_TYPE_GSU) {
		return de_gsu_set_para(hdl, cfg);
	}

	return -1;
}

struct de_scaler_handle *de_scaler_create(const struct module_create_info *info)
{
	int i;
	struct de_scaler_handle *hdl;
	struct de_scaler_private *priv;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	unsigned int offset = CHN_SCALER_OFFSET;
	u8 __iomem *reg_base = (u8 __iomem *)(info->de_reg_base + info->reg_offset);
	const struct de_scaler_dsc *dsc;

	dsc = get_scaler_dsc(info);
	if (!dsc)
		return NULL;

	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));
	hdl->linebuff_share_ids = dsc->linebuff_share_ids;
	hdl->linebuff_yuv = dsc->line_buffer_yuv;
	hdl->linebuff_rgb = dsc->line_buffer_rgb;
	hdl->linebuff_yuv_ed = dsc->line_buffer_yuv_ed;
	hdl->private->dsc = dsc;
	if (dsc->offset)
		offset = dsc->offset;
	reg_base += offset;

	reg_mem_info = &(hdl->private->reg_mem_info);
	priv = hdl->private;

		switch (hdl->private->dsc->type) {
		case DE_SCALER_TYPE_GSU:
			reg_mem_info->size = sizeof(struct gsu_reg);
			reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
			    reg_mem_info->size,
			    (void *)&(reg_mem_info->phy_addr), info->update_mode == RCQ_MODE);
			if (NULL == reg_mem_info->vir_addr) {
				DRM_ERROR("alloc vsu[%d] mm fail!size=0x%x\n",
				       info->id, reg_mem_info->size);
				return ERR_PTR(-ENOMEM);
			}

			block = &(priv->gsu_reg_blks[GSU_REG_BLK_CTL]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x14;
			block->reg_addr = reg_base;

			block = &(priv->gsu_reg_blks[GSU_REG_BLK_ATTR]);
			block->phy_addr = reg_mem_info->phy_addr + 0x40;
			block->vir_addr = reg_mem_info->vir_addr + 0x40;
			block->size = 0x8;
			block->reg_addr = reg_base + 0x40;

			block = &(priv->gsu_reg_blks[GSU_REG_BLK_PARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0x80;
			block->vir_addr = reg_mem_info->vir_addr + 0x80;
			block->size = 0x20;
			block->reg_addr = reg_base + 0x80;

			block = &(priv->gsu_reg_blks[GSU_REG_BLK_COEFF0]);
			block->phy_addr = reg_mem_info->phy_addr + 0x200;
			block->vir_addr = reg_mem_info->vir_addr + 0x200;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x200;

			priv->reg_blk_num = GSU_REG_BLK_NUM;

			priv->tap_num_hori = GSU_TAP_NUM_HORI;
			priv->tap_num_vert = GSU_TAP_NUM_VERT;
			priv->step_valid_start_bit  = GSU_STEP_VALID_START_BIT;
			priv->step_frac_bit_width   = GSU_STEP_FRAC_BITWIDTH;
			priv->step_fixed_bit_width  = GSU_STEP_FIXED_BITWIDTH;
			priv->phase_valid_start_bit = GSU_PHASE_VALID_START_BIT;
			priv->phase_frac_bit_width  = GSU_PHASE_FRAC_BITWIDTH;
			priv->phase_fixed_bit_width = GSU_PHASE_FIXED_BITWIDTH;
			break;
		case DE_SCALER_TYPE_VSU8:
			reg_mem_info->size = sizeof(struct vsu8_reg);
			reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
			    reg_mem_info->size,
			    (void *)&(reg_mem_info->phy_addr), info->update_mode == RCQ_MODE);
			if (NULL == reg_mem_info->vir_addr) {
				DRM_ERROR("alloc vsu[%d] mm fail!size=0x%x\n",
				       info->id, reg_mem_info->size);
				return ERR_PTR(-ENOMEM);
			}

			block = &(priv->vsu8_reg_blks[VSU8_REG_BLK_CTL]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x14;
			block->reg_addr = reg_base;

			block = &(priv->vsu8_reg_blks[VSU8_REG_BLK_ATTR]);
			block->phy_addr = reg_mem_info->phy_addr + 0x40;
			block->vir_addr = reg_mem_info->vir_addr + 0x40;
			block->size = 0x8;
			block->reg_addr = reg_base + 0x40;

			block = &(priv->vsu8_reg_blks[VSU8_REG_BLK_YPARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0x80;
			block->vir_addr = reg_mem_info->vir_addr + 0x80;
			block->size = 0x1C;
			block->reg_addr = reg_base + 0x80;

			block = &(priv->vsu8_reg_blks[VSU8_REG_BLK_CPARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0xC0;
			block->vir_addr = reg_mem_info->vir_addr + 0xC0;
			block->size = 0x1C;
			block->reg_addr = reg_base + 0xC0;

			block = &(priv->vsu8_reg_blks[VSU8_REG_BLK_COEFF0]);
			block->phy_addr = reg_mem_info->phy_addr + 0x200;
			block->vir_addr = reg_mem_info->vir_addr + 0x200;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x200;

			block = &(priv->vsu8_reg_blks[VSU8_REG_BLK_COEFF1]);
			block->phy_addr = reg_mem_info->phy_addr + 0x400;
			block->vir_addr = reg_mem_info->vir_addr + 0x400;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x400;

			block = &(priv->vsu8_reg_blks[VSU8_REG_BLK_COEFF2]);
			block->phy_addr = reg_mem_info->phy_addr + 0x600;
			block->vir_addr = reg_mem_info->vir_addr + 0x600;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x600;

			priv->reg_blk_num = VSU8_REG_BLK_NUM;

			priv->tap_num_hori = VSU8_TAP_NUM_HORI;
			priv->tap_num_vert = VSU8_TAP_NUM_VERT;
			priv->step_valid_start_bit  = VSU_STEP_VALID_START_BIT;
			priv->step_frac_bit_width   = VSU_STEP_FRAC_BITWIDTH;
			priv->step_fixed_bit_width  = VSU_STEP_FIXED_BITWIDTH;
			priv->phase_valid_start_bit = VSU_PHASE_VALID_START_BIT;
			priv->phase_frac_bit_width  = VSU_PHASE_FRAC_BITWIDTH;
			priv->phase_fixed_bit_width = VSU_PHASE_FIXED_BITWIDTH;
			break;
		case DE_SCALER_TYPE_VSU10:
			reg_mem_info->size = sizeof(struct vsu10_reg);
			reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
			    reg_mem_info->size,
			    (void *)&(reg_mem_info->phy_addr), info->update_mode == RCQ_MODE);
			if (NULL == reg_mem_info->vir_addr) {
				DRM_ERROR("alloc vsu[%d] mm fail!size=0x%x\n",
				       info->id, reg_mem_info->size);
				return ERR_PTR(-ENOMEM);
			}

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_CTL]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x14;
			block->reg_addr = reg_base;

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_ATTR]);
			block->phy_addr = reg_mem_info->phy_addr + 0x40;
			block->vir_addr = reg_mem_info->vir_addr + 0x40;
			block->size = 0x8;
			block->reg_addr = reg_base + 0x40;

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_YPARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0x80;
			block->vir_addr = reg_mem_info->vir_addr + 0x80;
			block->size = 0x20;
			block->reg_addr = reg_base + 0x80;

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_CPARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0xC0;
			block->vir_addr = reg_mem_info->vir_addr + 0xC0;
			block->size = 0x20;
			block->reg_addr = reg_base + 0xC0;

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_COEFF0]);
			block->phy_addr = reg_mem_info->phy_addr + 0x200;
			block->vir_addr = reg_mem_info->vir_addr + 0x200;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x200;

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_COEFF1]);
			block->phy_addr = reg_mem_info->phy_addr + 0x300;
			block->vir_addr = reg_mem_info->vir_addr + 0x300;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x300;

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_COEFF2]);
			block->phy_addr = reg_mem_info->phy_addr + 0x400;
			block->vir_addr = reg_mem_info->vir_addr + 0x400;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x400;

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_COEFF3]);
			block->phy_addr = reg_mem_info->phy_addr + 0x600;
			block->vir_addr = reg_mem_info->vir_addr + 0x600;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x600;

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_COEFF4]);
			block->phy_addr = reg_mem_info->phy_addr + 0x700;
			block->vir_addr = reg_mem_info->vir_addr + 0x700;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x700;

			block = &(priv->vsu10_reg_blks[VSU10_REG_BLK_COEFF5]);
			block->phy_addr = reg_mem_info->phy_addr + 0x800;
			block->vir_addr = reg_mem_info->vir_addr + 0x800;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x800;

			priv->reg_blk_num = VSU10_REG_BLK_NUM;

			priv->tap_num_hori = VSU10_TAP_NUM_HORI;
			priv->tap_num_vert = VSU10_TAP_NUM_VERT;
			priv->step_valid_start_bit  = VSU_STEP_VALID_START_BIT;
			priv->step_frac_bit_width   = VSU_STEP_FRAC_BITWIDTH;
			priv->step_fixed_bit_width  = VSU_STEP_FIXED_BITWIDTH;
			priv->phase_valid_start_bit = VSU_PHASE_VALID_START_BIT;
			priv->phase_frac_bit_width  = VSU_PHASE_FRAC_BITWIDTH;
			priv->phase_fixed_bit_width = VSU_PHASE_FIXED_BITWIDTH;
			break;
		case DE_SCALER_TYPE_VSU_ED:
			reg_mem_info->size = sizeof(struct vsu_ed_reg);
			reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
			    reg_mem_info->size,
			    (void *)&(reg_mem_info->phy_addr), info->update_mode == RCQ_MODE);
			if (NULL == reg_mem_info->vir_addr) {
				DRM_ERROR("alloc vsu[%d] mm fail!size=0x%x\n",
				       info->id, reg_mem_info->size);
				return ERR_PTR(-ENOMEM);
			}

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_CTL]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x14;
			block->reg_addr = reg_base;

			block =
			    &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_DIR_SHP]);
			block->phy_addr = reg_mem_info->phy_addr + 0x20;
			block->vir_addr = reg_mem_info->vir_addr + 0x20;
			block->size = 0x20;
			block->reg_addr = reg_base + 0x20;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_ATTR]);
			block->phy_addr = reg_mem_info->phy_addr + 0x40;
			block->vir_addr = reg_mem_info->vir_addr + 0x40;
			block->size = 0x8;
			block->reg_addr = reg_base + 0x40;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_YPARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0x80;
			block->vir_addr = reg_mem_info->vir_addr + 0x80;
			block->size = 0x20;
			block->reg_addr = reg_base + 0x80;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_CPARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0xC0;
			block->vir_addr = reg_mem_info->vir_addr + 0xC0;
			block->size = 0x20;
			block->reg_addr = reg_base + 0xC0;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_COEFF0]);
			block->phy_addr = reg_mem_info->phy_addr + 0x200;
			block->vir_addr = reg_mem_info->vir_addr + 0x200;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x200;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_COEFF1]);
			block->phy_addr = reg_mem_info->phy_addr + 0x300;
			block->vir_addr = reg_mem_info->vir_addr + 0x300;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x300;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_COEFF2]);
			block->phy_addr = reg_mem_info->phy_addr + 0x400;
			block->vir_addr = reg_mem_info->vir_addr + 0x400;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x400;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_COEFF3]);
			block->phy_addr = reg_mem_info->phy_addr + 0x600;
			block->vir_addr = reg_mem_info->vir_addr + 0x600;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x600;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_COEFF4]);
			block->phy_addr = reg_mem_info->phy_addr + 0x700;
			block->vir_addr = reg_mem_info->vir_addr + 0x700;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x700;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_COEFF5]);
			block->phy_addr = reg_mem_info->phy_addr + 0x800;
			block->vir_addr = reg_mem_info->vir_addr + 0x800;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x800;

			priv->reg_blk_num = VSU_ED_REG_BLK_NUM;

			priv->tap_num_hori = VSU_ED_TAP_NUM_HORI;
			priv->tap_num_vert = VSU_ED_TAP_NUM_VERT;
			priv->step_valid_start_bit  = VSU_STEP_VALID_START_BIT;
			priv->step_frac_bit_width   = VSU_STEP_FRAC_BITWIDTH;
			priv->step_fixed_bit_width  = VSU_STEP_FIXED_BITWIDTH;
			priv->phase_valid_start_bit = VSU_PHASE_VALID_START_BIT;
			priv->phase_frac_bit_width  = VSU_PHASE_FRAC_BITWIDTH;
			priv->phase_fixed_bit_width = VSU_PHASE_FIXED_BITWIDTH;
			break;
		case DE_SCALER_TYPE_ASU:
			reg_mem_info->size = sizeof(struct asu_reg);
			reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
			    reg_mem_info->size,
			    (void *)&(reg_mem_info->phy_addr), info->update_mode == RCQ_MODE);
			if (NULL == reg_mem_info->vir_addr) {
				DRM_ERROR("alloc vsu[%d] mm fail!size=0x%x\n",
				       info->id, reg_mem_info->size);
				return ERR_PTR(-ENOMEM);
			}

			block = &(priv->asu_reg_blks[ASU_REG_BLK_CTL]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x14;
			block->reg_addr = reg_base;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_PEAKING]);
			block->phy_addr = reg_mem_info->phy_addr + 0x20;
			block->vir_addr = reg_mem_info->vir_addr + 0x20;
			block->size = 0x20;
			block->reg_addr = reg_base + 0x20;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_ATTR]);
			block->phy_addr = reg_mem_info->phy_addr + 0x40;
			block->vir_addr = reg_mem_info->vir_addr + 0x40;
			block->size = 0x10;
			block->reg_addr = reg_base + 0x40;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_YPARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0x50;
			block->vir_addr = reg_mem_info->vir_addr + 0x50;
			block->size = 0x20;
			block->reg_addr = reg_base + 0x50;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_CPARA]);
			block->phy_addr = reg_mem_info->phy_addr + 0x70;
			block->vir_addr = reg_mem_info->vir_addr + 0x70;
			block->size = 0x20;
			block->reg_addr = reg_base + 0x70;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_COEFF0]);
			block->phy_addr = reg_mem_info->phy_addr + 0x100;
			block->vir_addr = reg_mem_info->vir_addr + 0x100;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x100;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_COEFF1]);
			block->phy_addr = reg_mem_info->phy_addr + 0x200;
			block->vir_addr = reg_mem_info->vir_addr + 0x200;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x200;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_COEFF2]);
			block->phy_addr = reg_mem_info->phy_addr + 0x300;
			block->vir_addr = reg_mem_info->vir_addr + 0x300;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x300;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_COEFF3]);
			block->phy_addr = reg_mem_info->phy_addr + 0x400;
			block->vir_addr = reg_mem_info->vir_addr + 0x400;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x400;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_COEFF4]);
			block->phy_addr = reg_mem_info->phy_addr + 0x500;
			block->vir_addr = reg_mem_info->vir_addr + 0x500;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x500;

			block = &(priv->asu_reg_blks[ASU_REG_BLK_COEFF5]);
			block->phy_addr = reg_mem_info->phy_addr + 0x600;
			block->vir_addr = reg_mem_info->vir_addr + 0x600;
			block->size = 32 * 4;
			block->reg_addr = reg_base + 0x600;
			hdl->is_asu = true;

			priv->reg_blk_num = ASU_REG_BLK_NUM;

			priv->tap_num_hori = ASU_TAP_NUM_HORI;
			priv->tap_num_vert = ASU_TAP_NUM_VERT;
			priv->step_valid_start_bit  = VSU_STEP_VALID_START_BIT;
			priv->step_frac_bit_width   = VSU_STEP_FRAC_BITWIDTH;
			priv->step_fixed_bit_width  = VSU_STEP_FIXED_BITWIDTH;
			priv->phase_valid_start_bit = VSU_PHASE_VALID_START_BIT;
			priv->phase_frac_bit_width  = VSU_PHASE_FRAC_BITWIDTH;
			priv->phase_fixed_bit_width = VSU_PHASE_FIXED_BITWIDTH;
			break;

		default:
			DRM_ERROR("not support this vsu_type=%d\n",
			       hdl->private->dsc->type);
			break;
		}

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(block[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	return hdl;
}

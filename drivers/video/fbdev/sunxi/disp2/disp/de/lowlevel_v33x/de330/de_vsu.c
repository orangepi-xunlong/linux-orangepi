/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include "de_vsu_type.h"
#include "de_scaler_table.h"

#include "../../include.h"
#include "de_vsu.h"
#include "de_feat.h"
#include "de_top.h"

#define VSU_STEP_VALID_START_BIT 1
#define VSU_STEP_FRAC_BITWIDTH 19
#define VSU_STEP_FIXED_BITWIDTH 4
#define VSU_PHASE_VALID_START_BIT 1
#define VSU_PHASE_FRAC_BITWIDTH 19
#define VSU_PHASE_FIXED_BITWIDTH 4

#define VSU_MIN_INPUT_WIDTH 8
#define VSU_MIN_INPUT_HEIGHT 8

#define VSU8_TAP_NUM_HORI   4
#define VSU8_TAP_NUM_VERT   2
#define VSU10_TAP_NUM_HORI  8
#define VSU10_TAP_NUM_VERT  4
#define VSU_ED_TAP_NUM_HORI 8
#define VSU_ED_TAP_NUM_VERT 4

enum {
	VSU8_REG_BLK_CTL = 0,
	VSU8_REG_BLK_ATTR,
	VSU8_REG_BLK_YPARA,
	VSU8_REG_BLK_CPARA,
	VSU8_REG_BLK_COEFF0,
	VSU8_REG_BLK_COEFF1,
	VSU8_REG_BLK_COEFF2,
	VSU8_REG_BLK_NUM,
};

enum {
	VSU10_REG_BLK_CTL = 0,
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

enum {
	VSU_ED_REG_BLK_CTL = 0,
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

struct de_vsu_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	union {
		struct de_reg_block vsu8_reg_blks[VSU8_REG_BLK_NUM];
		struct de_reg_block vsu10_reg_blks[VSU10_REG_BLK_NUM];
		struct de_reg_block vsu_ed_reg_blks[VSU_ED_REG_BLK_NUM];
		struct de_reg_block reg_blks[VSU_MAX_REG_BLK_NUM];
	};
	u32 vsu_type;
	u32 tap_num_hori;
	u32 tap_num_vert;

	void (*set_blk_dirty)(struct de_vsu_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_vsu_private vsu_priv[DE_NUM][MAX_CHN_NUM];

static inline struct vsu8_reg *get_vsu8_reg(
	struct de_vsu_private *priv)
{
	return (struct vsu8_reg *)(priv->vsu8_reg_blks[0].vir_addr);
}

static inline struct vsu10_reg *get_vsu10_reg(
	struct de_vsu_private *priv)
{
	return (struct vsu10_reg *)(priv->vsu10_reg_blks[0].vir_addr);
}

static inline struct vsu_ed_reg *get_vsu_ed_reg(
	struct de_vsu_private *priv)
{
	return (struct vsu_ed_reg *)(priv->vsu_ed_reg_blks[0].vir_addr);
}

static void vsu_set_block_dirty(
	struct de_vsu_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void vsu_set_rcq_head_dirty(
	struct de_vsu_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

/*
* function    : de_vsu_calc_fir_coef(u32 step)
* description :
* parameters  :
*               step <horizontal scale ratio of vsu>
* return      :
*               offset (in word) of coefficient table
*/
static u32 de_vsu_calc_fir_coef(u32 step)
{
	u32 pt_coef;
	u32 scale_ratio, int_part, float_part, fir_coef_ofst;

	scale_ratio = step >> (VSU_PHASE_FRAC_BITWIDTH - 3);
	int_part = scale_ratio >> 3;
	float_part = scale_ratio & 0x7;
	if (int_part == 0)
		fir_coef_ofst = VSU_ZOOM0_SIZE;
	else if (int_part == 1)
		fir_coef_ofst = VSU_ZOOM0_SIZE + float_part;
	else if (int_part == 2)
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE
			+ (float_part >> 1);
	else if (int_part == 3)
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE
			+ VSU_ZOOM2_SIZE;
	else if (int_part == 4)
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE
			+ VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE;
	else
		fir_coef_ofst = VSU_ZOOM0_SIZE + VSU_ZOOM1_SIZE
			+ VSU_ZOOM2_SIZE + VSU_ZOOM3_SIZE + VSU_ZOOM4_SIZE;

	pt_coef = fir_coef_ofst * VSU_PHASE_NUM;

	return pt_coef;
}

s32 de_vsu_calc_lay_scale_para(u32 disp, u32 chn,
	enum de_format_space fm_space, struct de_chn_info *chn_info,
	struct de_rect64_s *crop64, struct de_rect_s *scn_win,
	struct de_rect_s *crop32, struct de_scale_para *ypara,
	struct de_scale_para *cpara)
{
	u64 val;
	struct de_vsu_private *priv = &(vsu_priv[disp][chn]);
	u32 scale_mode = 0, linebuf = 0;

	if (scn_win->width) {
		val = crop64->width;
		do_div(val, scn_win->width);
	} else {
		val = 0;
	}
	ypara->hstep = (u32)(val >> (32 - VSU_STEP_FRAC_BITWIDTH));

	if (scn_win->height) {
		val = crop64->height;
		do_div(val, scn_win->height);
	} else {
		val = 0;
	}
	ypara->vstep = (u32)(val >> (32 - VSU_STEP_FRAC_BITWIDTH));

	ypara->hphase =
		(crop64->left & 0xffffffff) >> (32 - VSU_PHASE_FRAC_BITWIDTH);
	ypara->vphase =
		(crop64->top & 0xffffffff) >> (32 - VSU_PHASE_FRAC_BITWIDTH);

	crop32->left = (s32)(crop64->left >> 32);
	crop32->top = (s32)(crop64->top >> 32);

	val = (crop64->width & 0xffffffff)
		+ ((u64)(crop64->left) & 0xffffffff);
	crop32->width = (val >> 32) ?
		((crop64->width >> 32) + 1) : (crop64->width >> 32);

	val = (crop64->height & 0xffffffff)
		+ ((u64)(crop64->top) & 0xffffffff);
	crop32->height = (val >> 32) ?
		((crop64->height >> 32) + 1) : (crop64->height >> 32);

	if (fm_space == DE_FORMAT_SPACE_RGB) {
		cpara->hstep = ypara->hstep;
		cpara->vstep = ypara->vstep;
		cpara->hphase = ypara->hphase;
		cpara->vphase = ypara->vphase;
		return 0;
	} else if (fm_space != DE_FORMAT_SPACE_YUV) {
		DE_WRN("calc cpara for fm_space(%d)!\n", fm_space);
		return -1;
	}

	if (chn_info->yuv_sampling == DE_YUV422) {
		/* horizon crop info fix */
		if (((crop32->left & 0x1) == 0x0)
			&& ((crop32->width & 0x1) == 0x1)) {
			/* odd crop_w, crop down width, */
			/* last line may disappear */
			crop32->width--;
		} else if (((crop32->left & 0x1) == 0x1)
			&& ((crop32->width & 0x1) == 0x0)) {
			/* odd crop_x, crop down x, and phase + 1 */
			ypara->hphase += (1U << VSU_PHASE_FRAC_BITWIDTH);
			crop32->left--;
		} else if (((crop32->left & 0x1) == 0x1)
			&& ((crop32->width & 0x1) == 0x1)) {
			/* odd crop_x and crop_w, */
			/* crop_x - 1, and phase + 1, crop_w + 1 */
			ypara->hphase += (1U << VSU_PHASE_FRAC_BITWIDTH);
			crop32->left--;
			crop32->width++;
		}

		cpara->hstep = ypara->hstep >> 1;
		cpara->vstep = ypara->vstep;
		cpara->hphase = ypara->hphase;
		cpara->vphase = ypara->vphase;
	} else if (chn_info->yuv_sampling == DE_YUV420) {
		/* horizon crop info fix */
		if (((crop32->left & 0x1) == 0x0)
			&& ((crop32->width & 0x1) == 0x1)) {
			/* odd crop_w, crop down width, */
			/* last line may disappear */
			crop32->width--;
		} else if (((crop32->left & 0x1) == 0x1)
			&& ((crop32->width & 0x1) == 0x0)) {
			/* odd crop_x, crop down x, and phase + 1 */
			ypara->hphase += (1 << VSU_PHASE_FRAC_BITWIDTH);
			crop32->left--;
		} else if (((crop32->left & 0x1) == 0x1)
			&& ((crop32->width & 0x1) == 0x1)) {
			/* odd crop_x and crop_w, crop_x - 1, */
			/* and phase + 1, crop_w + 1 */
			ypara->hphase += (1 << VSU_PHASE_FRAC_BITWIDTH);
			crop32->left--;
			crop32->width++;
		}
		/* vertical crop info fix */
		if (((crop32->top & 0x1) == 0x0)
			&& ((crop32->height & 0x1) == 0x1)) {
			/* odd crop_h, crop down height, */
			/* last line may disappear */
			crop32->height--;
		} else if (((crop32->height & 0x1) == 0x1)
			&& ((crop32->height & 0x1) == 0x0)) {
			/* odd crop_y, crop down y, and phase + 1 */
			ypara->vphase += (1 << VSU_PHASE_FRAC_BITWIDTH);
			crop32->top--;
		} else if (((crop32->top & 0x1) == 0x1)
			&& ((crop32->height & 0x1) == 0x1)) {
			/* odd crop_y and crop_h, crop_y - 1, */
			/* and phase + 1, crop_h + 1 */
			ypara->vphase += (1 << VSU_PHASE_FRAC_BITWIDTH);
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
		if (chn_info->snr_en) {
			scale_mode = 1;
			if (chn_info->px_fmt == DE_FORMAT_YUV420_P ||
			    chn_info->px_fmt == DE_FORMAT_YUV420_SP_UVUV ||
			    chn_info->px_fmt == DE_FORMAT_YUV420_SP_VUVU ||
			    chn_info->px_fmt == DE_FORMAT_YUV420_SP_UVUV_10BIT ||
			    chn_info->px_fmt == DE_FORMAT_YUV420_SP_VUVU_10BIT ||
			    chn_info->px_fmt == DE_FORMAT_YUV420_SP_VUVU) {
				/*cpara->vphase = 0x7b3333;*/
				/* chorma vertical phase should -0.6 when input
				 * format
				 * is */
				/* yuv420 and snr on */
				if (priv->vsu_type == DE_SCALER_TYPE_VSU_ED) {
					linebuf = de_feat_get_scale_linebuf_for_ed(disp, chn);
					if (((chn_info->ovl_out_win.width <= linebuf)
					     && (chn_info->ovl_ypara.hstep < (1 << VSU_STEP_FRAC_BITWIDTH))
					     && (chn_info->ovl_ypara.vstep < (1 << VSU_STEP_FRAC_BITWIDTH)))) {
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
						 << (VSU_PHASE_FRAC_BITWIDTH - 2));
				} else { /*scale_mode == 1*/
					cpara->vphase =
					    (ypara->vphase >> 1) -
					    (((1 << VSU_PHASE_FRAC_BITWIDTH) *
					      153) >>
					     8);
				}
			} else {
				cpara->vphase = ypara->vphase;
			}
		} else {
			/* chorma vertical phase should -0.25 when input format is */
			/* yuv420 */
			cpara->vphase = (ypara->vphase >> 1) -
				(1 << (VSU_PHASE_FRAC_BITWIDTH - 2));
		}
	} else if (chn_info->yuv_sampling == DE_YUV411) {
		/* horizon crop info */
		if (((crop32->left & 0x3) == 0x0)
			&& ((crop32->width & 0x3) != 0x0)) {
			/* odd crop_w, crop down width, */
			/* last 1-3 lines may disappear */
			crop32->width = (crop32->width >> 2) << 2;
		} else if (((crop32->left & 0x3) != 0x0)
			&& ((crop32->width & 0x3) == 0x0)) {
			/* odd crop_x, crop down x, and phase + 1 */
			ypara->hphase += ((crop32->left & 0x3) << VSU_PHASE_FRAC_BITWIDTH);
			crop32->left = (crop32->left >> 2) << 2;
		} else if (((crop32->left & 0x3) != 0x0)
			&& ((crop32->width & 0x3) != 0x0)) {
			/* odd crop_x and crop_w, crop_x aligned to 4 pixel */
			ypara->hphase += ((crop32->left & 0x3) << VSU_PHASE_FRAC_BITWIDTH);
			crop32->width = ((crop32->width + (crop32->left & 0x3)) >> 2) << 2;
			crop32->left = (crop32->left >> 2) << 2;
		}

		cpara->hstep = ypara->hstep >> 2;
		cpara->vstep = ypara->vstep;
		cpara->hphase = ypara->hphase;
		cpara->vphase = ypara->vphase;
	} else {
		DE_WRN("not support yuv_sampling(%d)!\n", chn_info->yuv_sampling);
		return -1;
	}

	return 0;
}

u32 de_vsu_calc_ovl_coord(u32 disp, u32 chn,
	u32 dst_coord, u32 scale_step)
{
	u32 half, src_coord;

	half = (1 << (VSU_STEP_FRAC_BITWIDTH - 1));
	src_coord = (dst_coord * scale_step + half)
		>> VSU_STEP_FRAC_BITWIDTH;
	DE_INF("half_shift_coord=<%x,%d,%d,%d>\n", half,
		VSU_STEP_FRAC_BITWIDTH, dst_coord, src_coord);

	return src_coord;
}

s32 de_vsu_calc_ovl_scale_para(u32 layer_num,
	struct de_scale_para *ypara,
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

u32 de_vsu_fix_tiny_size(u32 disp, u32 chn,
	struct de_rect_s *in_win, struct de_rect_s *out_win,
	struct de_scale_para *ypara, enum de_format_space fmt_space,
	struct de_rect_s *lay_win, u32 lay_num,
	u32 scn_width, u32 scn_height)
{
	u32 result = 0x0;

	if (!in_win->width || !in_win->height
		|| !out_win->width || !out_win->height
		|| !ypara->hstep || !ypara->vstep)
		return result;

	/* horizon */
	if (in_win->width < VSU_MIN_INPUT_WIDTH
		|| out_win->width < VSU_MIN_INPUT_WIDTH) {
		u32 org_out_win_width = out_win->width;
		u32 shift = VSU_PHASE_FRAC_BITWIDTH;

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
			out_win->width = VSU_MIN_INPUT_WIDTH * (1 << shift)
				/ ypara->hstep;
			result |= (VSU_EXPAND_OUTWIDTH | VSU_EXPAND_OUTWIDTH);
		}

		if (out_win->width + out_win->left > scn_width) {
			u32 i;

			out_win->left -= (out_win->width - org_out_win_width);
			for (i = 0; i < lay_num; i++) {
				lay_win[i].left += ((ypara->hstep
					* (out_win->width - org_out_win_width)) >> shift);
			}
			result |= VSU_LSHIFT_OUTWINDOW;
		}
	}

	/* vertical */
	if (in_win->height < VSU_MIN_INPUT_HEIGHT
		|| out_win->height < VSU_MIN_INPUT_HEIGHT) {
		u32 org_out_win_height = out_win->height;
		u32 shift = VSU_PHASE_FRAC_BITWIDTH;

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
			out_win->height = VSU_MIN_INPUT_HEIGHT * (1 << shift)
					/ ypara->vstep;
			result |= (VSU_EXPAND_OUTHEIGHT | VSU_EXPAND_INHEIGHT);
		}

		if (out_win->height + out_win->top > scn_height) {
			u32 i;

			out_win->top -= (out_win->height - org_out_win_height);
			for (i = 0; i < lay_num; i++) {
				lay_win[i].top += ((ypara->vstep
					* (out_win->height - org_out_win_height)) >> shift);
			}
			result |= VSU_USHIFT_OUTWINDOW;
		}
	}

	return result;
}

u32 de_vsu_fix_big_size(u32 disp, u32 chn,
	struct de_rect_s *in_win, struct de_rect_s *out_win,
	enum de_format_space fmt_space, enum de_yuv_sampling yuv_sampling)
{
	struct de_vsu_private *priv = &(vsu_priv[disp][chn]);

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
		linebuf = de_feat_get_scale_linebuf_for_yuv(disp, chn);
	} else if (fmt_space == DE_FORMAT_SPACE_RGB) {
		linebuf = de_feat_get_scale_linebuf_for_rgb(disp, chn);
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

s32 de_vsu_calc_scale_para(u32 fix_size_result,
	enum de_format_space fmt_space, enum de_yuv_sampling yuv_sampling,
	struct de_rect_s *out_win,
	struct de_rect_s *ywin, struct de_rect_s *cwin,
	struct de_scale_para *ypara, struct de_scale_para *cpara)
{
	u32 wshift = 0;
	u32 hshift = 0;

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

		val = (u64)ywin->width << VSU_STEP_FRAC_BITWIDTH;
		do_div(val, out_win->width);
		ypara->hstep = (u32)val;
		ypara->hphase = 0; /* no meaning when coarse scale using */

		cwin->width = ywin->width >> wshift;
		val = (u64)cwin->width << VSU_STEP_FRAC_BITWIDTH;
		do_div(val, out_win->width);
		cpara->hstep = (u32)val;
		cpara->hphase = 0; /* no meaning when coarse scale using */
	}
	if (fix_size_result
		& (VSU_CUT_INHEIGHT | RTMX_CUT_INHEIGHT)) {
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

static s32 de_vsu8_set_para(u32 disp, u32 chn,
	struct de_chn_info *chn_info)
{
	struct de_vsu_private *priv = &(vsu_priv[disp][chn]);
	struct vsu8_reg *reg = get_vsu8_reg(priv);
	u32 scale_mode = 0;
	u32 pt_coef;

	if (chn_info->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		if (chn_info->yuv_sampling == DE_YUV422) {
			switch (chn_info->px_fmt) {
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
		} else if (chn_info->yuv_sampling == DE_YUV420) {
			scale_mode = 1;
		} else if (chn_info->yuv_sampling == DE_YUV411) {
			scale_mode = 1;
		} else {
			DE_WRN("yuv_sampling=%d\n", chn_info->yuv_sampling);
		}
	} else if (chn_info->px_fmt_space != DE_FORMAT_SPACE_RGB) {
		DE_WRN("px_fmt_space=%d\n", chn_info->px_fmt_space);
		return -1;
	}

	reg->ctl.bits.en = 1;
	reg->scale_mode.dwval = scale_mode;
	priv->set_blk_dirty(priv, VSU8_REG_BLK_CTL, 1);

	reg->out_size.bits.width = chn_info->scn_win.width ?
		(chn_info->scn_win.width - 1) : 0;
	reg->out_size.bits.height = chn_info->scn_win.height ?
		(chn_info->scn_win.height - 1) : 0;

	reg->glb_alpha.dwval = chn_info->glb_alpha;
	priv->set_blk_dirty(priv, VSU8_REG_BLK_ATTR, 1);

	reg->y_in_size.bits.width = chn_info->ovl_out_win.width ?
		(chn_info->ovl_out_win.width - 1) : 0;
	reg->y_in_size.bits.height = chn_info->ovl_out_win.height ?
		(chn_info->ovl_out_win.height - 1) : 0;
	reg->y_hstep.dwval = chn_info->ovl_ypara.hstep
		<< VSU8_STEP_VALID_START_BIT;
	reg->y_vstep.dwval = chn_info->ovl_ypara.vstep
		<< VSU8_STEP_VALID_START_BIT;
	reg->y_hphase.dwval = chn_info->ovl_ypara.hphase
		<< VSU8_PHASE_VALID_START_BIT;
	reg->y_vphase.dwval = chn_info->ovl_ypara.vphase
		<< VSU8_PHASE_VALID_START_BIT;
	priv->set_blk_dirty(priv, VSU8_REG_BLK_YPARA, 1);

	reg->c_in_size.bits.width = chn_info->c_win.width ?
		(chn_info->c_win.width - 1) : 0;
	reg->c_in_size.bits.height = chn_info->c_win.height ?
		(chn_info->c_win.height - 1) : 0;
	reg->c_hstep.dwval = chn_info->ovl_cpara.hstep
		<< VSU8_STEP_VALID_START_BIT;
	reg->c_vstep.dwval = chn_info->ovl_cpara.vstep
		<< VSU8_STEP_VALID_START_BIT;
	reg->c_hphase.dwval = chn_info->ovl_cpara.hphase
		<< VSU8_PHASE_VALID_START_BIT;
	reg->c_vphase.dwval = chn_info->ovl_cpara.vphase
		<< VSU8_PHASE_VALID_START_BIT;
	priv->set_blk_dirty(priv, VSU8_REG_BLK_CPARA, 1);

	/* fir coefficient */
	/* ch0 */
	pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_ypara.hstep);
	memcpy(reg->y_hori_coeff, lan2coefftab32 + pt_coef,
		sizeof(u32) * VSU_PHASE_NUM);
	pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_ypara.vstep);
	memcpy(reg->y_vert_coeff, lan2coefftab32 + pt_coef,
		   sizeof(u32) * VSU_PHASE_NUM);

	/* ch1/2 */
	if (chn_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff, lan2coefftab32 + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
	} else {
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff, bicubic4coefftab32 + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
	}
	priv->set_blk_dirty(priv, VSU8_REG_BLK_COEFF0, 1);
	priv->set_blk_dirty(priv, VSU8_REG_BLK_COEFF1, 1);
	priv->set_blk_dirty(priv, VSU8_REG_BLK_COEFF2, 1);

	return 0;
}

static s32 de_vsu10_set_para(u32 disp, u32 chn,
	struct de_chn_info *chn_info)
{
	struct de_vsu_private *priv = &(vsu_priv[disp][chn]);
	struct vsu10_reg *reg = get_vsu10_reg(priv);
	u32 scale_mode = 0;
	u32 pt_coef;

	if (chn_info->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		switch (chn_info->yuv_sampling) {
		case DE_YUV444:
			scale_mode = 0;
			break;
		case DE_YUV422:
		case DE_YUV420:
		case DE_YUV411:
			scale_mode = 1;
			break;
		default:
			DE_WRN("yuv_sampling=%d\n", chn_info->yuv_sampling);
			return -1;
		}
	} else if (chn_info->px_fmt_space != DE_FORMAT_SPACE_RGB) {
		DE_WRN("px_fmt_space=%d\n", chn_info->px_fmt_space);
		return -1;
	}

	reg->ctl.bits.en = 1;
	reg->scale_mode.dwval = scale_mode;
	priv->set_blk_dirty(priv, VSU10_REG_BLK_CTL, 1);

	reg->out_size.bits.width = chn_info->scn_win.width ?
		(chn_info->scn_win.width - 1) : 0;
	reg->out_size.bits.height = chn_info->scn_win.height ?
		(chn_info->scn_win.height - 1) : 0;

	reg->glb_alpha.dwval = chn_info->glb_alpha;
	priv->set_blk_dirty(priv, VSU10_REG_BLK_ATTR, 1);

	reg->y_in_size.bits.width = chn_info->ovl_out_win.width ?
		(chn_info->ovl_out_win.width - 1) : 0;
	reg->y_in_size.bits.height = chn_info->ovl_out_win.height ?
		(chn_info->ovl_out_win.height - 1) : 0;
	reg->y_hstep.dwval = chn_info->ovl_ypara.hstep
		<< VSU10_STEP_VALID_START_BIT;
	reg->y_vstep.dwval = chn_info->ovl_ypara.vstep
		<< VSU10_STEP_VALID_START_BIT;
	reg->y_hphase.dwval = chn_info->ovl_ypara.hphase
		<< VSU10_PHASE_VALID_START_BIT;
	reg->y_vphase0.dwval = chn_info->ovl_ypara.vphase
		<< VSU10_PHASE_VALID_START_BIT;
	priv->set_blk_dirty(priv, VSU10_REG_BLK_YPARA, 1);

	reg->c_in_size.bits.width = chn_info->c_win.width ?
		(chn_info->c_win.width - 1) : 0;
	reg->c_in_size.bits.height = chn_info->c_win.height ?
		(chn_info->c_win.height - 1) : 0;
	reg->c_hstep.dwval = chn_info->ovl_cpara.hstep
		<< VSU10_STEP_VALID_START_BIT;
	reg->c_vstep.dwval = chn_info->ovl_cpara.vstep
		<< VSU10_STEP_VALID_START_BIT;
	reg->c_hphase.dwval = chn_info->ovl_cpara.hphase
		<< VSU10_PHASE_VALID_START_BIT;
	reg->c_vphase0.dwval = chn_info->ovl_cpara.vphase
		<< VSU10_PHASE_VALID_START_BIT;
	priv->set_blk_dirty(priv, VSU10_REG_BLK_CPARA, 1);

	/* fir coefficient */
	/* ch0 */
	pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_ypara.hstep);
	memcpy(reg->y_hori_coeff0, lan3coefftab32_left + pt_coef,
		sizeof(u32) * VSU_PHASE_NUM);
	memcpy(reg->y_hori_coeff1, lan3coefftab32_right + pt_coef,
		sizeof(u32) * VSU_PHASE_NUM);
	pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_ypara.vstep);
	memcpy(reg->y_vert_coeff, lan2coefftab32 + pt_coef,
		   sizeof(u32) * VSU_PHASE_NUM);

	/* ch1/2 */
	if (chn_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff0, lan3coefftab32_left + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hori_coeff1, lan3coefftab32_right + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.vstep);
		memcpy(reg->c_vert_coeff, lan2coefftab32 + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
	} else {
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff0,
			bicubic8coefftab32_left + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hori_coeff1,
			bicubic8coefftab32_right + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.vstep);
		memcpy(reg->c_vert_coeff,
			bicubic4coefftab32 + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
	}
	priv->set_blk_dirty(priv, VSU10_REG_BLK_COEFF0, 1);
	priv->set_blk_dirty(priv, VSU10_REG_BLK_COEFF1, 1);
	priv->set_blk_dirty(priv, VSU10_REG_BLK_COEFF2, 1);
	priv->set_blk_dirty(priv, VSU10_REG_BLK_COEFF3, 1);
	priv->set_blk_dirty(priv, VSU10_REG_BLK_COEFF4, 1);
	priv->set_blk_dirty(priv, VSU10_REG_BLK_COEFF5, 1);

	return 0;
}

static s32 de_vsu_ed_set_para(u32 disp, u32 chn,
	struct de_chn_info *chn_info)
{
	struct de_vsu_private *priv = &(vsu_priv[disp][chn]);
	struct vsu_ed_reg *reg = get_vsu_ed_reg(priv);
	u32 scale_mode = 0;
	u32 pt_coef;


	if (chn_info->px_fmt_space == DE_FORMAT_SPACE_YUV) {
		u32 linebuf = de_feat_get_scale_linebuf_for_ed(disp, chn);

		switch (chn_info->yuv_sampling) {
		case DE_YUV444:
			scale_mode = 0;
			break;
		case DE_YUV422:
		case DE_YUV420:
		case DE_YUV411:
			if ((chn_info->ovl_out_win.width <= linebuf)
				&& (chn_info->ovl_ypara.hstep < (1 << VSU_STEP_FRAC_BITWIDTH))
				&& (chn_info->ovl_ypara.vstep < (1 << VSU_STEP_FRAC_BITWIDTH)))
				scale_mode = 2;
			else
				scale_mode = 1;
			break;
		default:
			DE_WRN("yuv_sampling=%d\n", chn_info->yuv_sampling);
			return -1;
		}
	} else if (chn_info->px_fmt_space != DE_FORMAT_SPACE_RGB) {
		DE_WRN("px_fmt_space=%d\n", chn_info->px_fmt_space);
		return -1;
	}

	reg->ctl.bits.en = 1;
	reg->scale_mode.dwval = scale_mode;
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_CTL, 1);

	reg->dir_thr.dwval = 0x0000FF01;
	reg->edge_thr.dwval = 0x00080000;
	reg->dir_ctl.dwval = 0x00000000;
	reg->angle_thr.dwval = 0x00020000;
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_DIR_SHP, 1);

	reg->out_size.bits.width = chn_info->scn_win.width ?
		(chn_info->scn_win.width - 1) : 0;
	reg->out_size.bits.height = chn_info->scn_win.height ?
		(chn_info->scn_win.height - 1) : 0;

	reg->glb_alpha.dwval = chn_info->glb_alpha;
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_ATTR, 1);

	reg->y_in_size.bits.width = chn_info->ovl_out_win.width ?
		(chn_info->ovl_out_win.width - 1) : 0;
	reg->y_in_size.bits.height = chn_info->ovl_out_win.height ?
		(chn_info->ovl_out_win.height - 1) : 0;
	reg->y_hstep.dwval = chn_info->ovl_ypara.hstep
		<< VSU_ED_STEP_VALID_START_BIT;
	reg->y_vstep.dwval = chn_info->ovl_ypara.vstep
		<< VSU_ED_STEP_VALID_START_BIT;
	reg->y_hphase.dwval = chn_info->ovl_ypara.hphase
		<< VSU_ED_PHASE_VALID_START_BIT;
	reg->y_vphase0.dwval = chn_info->ovl_ypara.vphase
		<< VSU_ED_PHASE_VALID_START_BIT;
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_YPARA, 1);

	reg->c_in_size.bits.width = chn_info->c_win.width ?
		(chn_info->c_win.width - 1) : 0;
	reg->c_in_size.bits.height = chn_info->c_win.height ?
		(chn_info->c_win.height - 1) : 0;
	reg->c_hstep.dwval = chn_info->ovl_cpara.hstep
		<< VSU_ED_STEP_VALID_START_BIT;
	reg->c_vstep.dwval = chn_info->ovl_cpara.vstep
		<< VSU_ED_STEP_VALID_START_BIT;
	reg->c_hphase.dwval = chn_info->ovl_cpara.hphase
		<< VSU_ED_PHASE_VALID_START_BIT;
	reg->c_vphase0.dwval = chn_info->ovl_cpara.vphase
		<< VSU_ED_PHASE_VALID_START_BIT;
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_CPARA, 1);

	/* fir coefficient */
	/* ch0 */
	pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_ypara.hstep);
	memcpy(reg->y_hori_coeff0, lan3coefftab32_left + pt_coef,
		sizeof(u32) * VSU_PHASE_NUM);
	memcpy(reg->y_hori_coeff1, lan3coefftab32_right + pt_coef,
		sizeof(u32) * VSU_PHASE_NUM);
	pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_ypara.vstep);
	memcpy(reg->y_vert_coeff, lan2coefftab32 + pt_coef,
		   sizeof(u32) * VSU_PHASE_NUM);

	/* ch1/2 */
	if (chn_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff0, lan3coefftab32_left + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hori_coeff1, lan3coefftab32_right + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.vstep);
		memcpy(reg->c_vert_coeff, lan2coefftab32 + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
	} else {
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.hstep);
		memcpy(reg->c_hori_coeff0,
			bicubic8coefftab32_left + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
		memcpy(reg->c_hori_coeff1,
			bicubic8coefftab32_right + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
		pt_coef = de_vsu_calc_fir_coef(chn_info->ovl_cpara.vstep);
		memcpy(reg->c_vert_coeff,
			bicubic4coefftab32 + pt_coef,
			sizeof(u32) * VSU_PHASE_NUM);
	}
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_COEFF0, 1);
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_COEFF1, 1);
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_COEFF2, 1);
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_COEFF3, 1);
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_COEFF4, 1);
	priv->set_blk_dirty(priv, VSU_ED_REG_BLK_COEFF5, 1);

	return 0;
}


s32 de_vsu_set_para(u32 disp, u32 chn,
	struct de_chn_info *chn_info)
{
	struct de_vsu_private *priv = &(vsu_priv[disp][chn]);

	if (priv->vsu_type == DE_SCALER_TYPE_VSU8) {
		return de_vsu8_set_para(disp, chn, chn_info);
	} else if (priv->vsu_type == DE_SCALER_TYPE_VSU10) {
		return de_vsu10_set_para(disp, chn, chn_info);
	} else if (priv->vsu_type == DE_SCALER_TYPE_VSU_ED) {
		return de_vsu_ed_set_para(disp, chn, chn_info);
	}

	return -1;
}

s32 de_vsu_disable(u32 disp, u32 chn)
{
	struct de_vsu_private *priv = &(vsu_priv[disp][chn]);

	if (priv->vsu_type == DE_SCALER_TYPE_VSU8) {
		struct vsu8_reg *reg = get_vsu8_reg(priv);

		reg->ctl.bits.en = 0;
	} else if (priv->vsu_type == DE_SCALER_TYPE_VSU10) {
		struct vsu10_reg *reg = get_vsu10_reg(priv);

		reg->ctl.bits.en = 0;
	} else if (priv->vsu_type == DE_SCALER_TYPE_VSU_ED) {
		struct vsu_ed_reg *reg = get_vsu_ed_reg(priv);

		reg->ctl.bits.en = 0;
	}
	priv->set_blk_dirty(priv, VSU8_REG_BLK_CTL, 1);
	return 0;
}

s32 de_vsu_init_sharp_para(u32 disp, u32 chn)
{
	struct de_vsu_private *priv = &(vsu_priv[disp][chn]);

	if (priv->vsu_type == DE_SCALER_TYPE_VSU_ED) {
		struct vsu_ed_reg *reg = get_vsu_ed_reg(priv);

		reg->sharp_en.dwval = 0;
		reg->sharp_coring.dwval = 0x20;
		reg->sharp_gain0.dwval = 0x03200078;
		reg->sharp_gain1.dwval = 0x01080000;
		priv->set_blk_dirty(priv, VSU_ED_REG_BLK_DIR_SHP, 1);
	}
	return 0;
}

s32 de_vsu_set_sharp_para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct de_vsu_sharp_config *para, u32 bypass)
{
	struct de_vsu_private *priv = &(vsu_priv[disp][chn]);

	if (priv->vsu_type == DE_SCALER_TYPE_VSU_ED) {
		struct vsu_ed_reg *reg = get_vsu_ed_reg(priv);
		u32 level;
		u32 en;
		u32 gain;
		u32 linebuf;
		const u32 PEAK2D_PARA_NUM = 11;
		const s32 peak2d_para[][2] = {
			/* lcd,    tv */
			{0,   0}, /* gain for yuv */
			{5,   5}, /*	             */
			{10, 10}, /*	             */
			{15, 15}, /*	             */
			{20, 20}, /*	             */
			{25, 25}, /*	             */
			{30, 30}, /*	             */
			{35, 35}, /*	             */
			{40, 40}, /*	             */
			{45, 45}, /*	             */
			{50, 50}, /* gain for yuv */
		};

		linebuf = de_feat_get_scale_linebuf_for_ed(disp, chn);

		/*
		* enable condition:
		* 1. scale up;
		* 2. user level > 0
		* 3. yuv format
		* 4. inw <- lb
		*/
		if ((para->inw < para->outw) && (para->inw <= linebuf)
			&& (para->inh < para->outh) && (para->level > 0)
			&& (fmt == 0) && (bypass == 0)) {
			en = 1;
		} else {
			en = 0;
		}
		reg->sharp_en.dwval = en;
		if (en) {
			level = para->level > (PEAK2D_PARA_NUM - 1) ?
				(PEAK2D_PARA_NUM - 1) : para->level;
			gain  = peak2d_para[level][dev_type];
			reg->sharp_gain1.dwval = gain;
		}
		priv->set_blk_dirty(priv, VSU_ED_REG_BLK_DIR_SHP, 1);
	}
	return 0;
}

s32 de_vsu_init(u32 disp, u8 __iomem *de_reg_base)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		u32 phy_chn = de_feat_get_phy_chn_id(disp, chn);
		u8 __iomem *reg_base = (u8 __iomem *)(de_reg_base
			+ DE_CHN_OFFSET(phy_chn) + CHN_SCALER_OFFSET);
		u32 rcq_used = de_feat_is_using_rcq(disp);

		struct de_vsu_private *priv = &vsu_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
		struct de_reg_block *block;

		priv->vsu_type = de_feat_get_scaler_type(disp, chn);
		switch (priv->vsu_type) {
		case DE_SCALER_TYPE_VSU8:
			reg_mem_info->size = sizeof(struct vsu8_reg);
			reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				rcq_used);
			if (NULL == reg_mem_info->vir_addr) {
				DE_WRN("alloc vsu[%d][%d] mm fail!size=0x%x\n",
				     disp, chn, reg_mem_info->size);
				return -1;
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
			break;
		case DE_SCALER_TYPE_VSU10:
			reg_mem_info->size = sizeof(struct vsu10_reg);
			reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				rcq_used);
			if (NULL == reg_mem_info->vir_addr) {
				DE_WRN("alloc vsu[%d][%d] mm fail!size=0x%x\n",
				     disp, chn, reg_mem_info->size);
				return -1;
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
			break;
		case DE_SCALER_TYPE_VSU_ED:
			reg_mem_info->size = sizeof(struct vsu_ed_reg);
			reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				rcq_used);
			if (NULL == reg_mem_info->vir_addr) {
				DE_WRN("alloc vsu[%d][%d] mm fail!size=0x%x\n",
				     disp, chn, reg_mem_info->size);
				return -1;
			}

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_CTL]);
			block->phy_addr = reg_mem_info->phy_addr;
			block->vir_addr = reg_mem_info->vir_addr;
			block->size = 0x14;
			block->reg_addr = reg_base;

			block = &(priv->vsu_ed_reg_blks[VSU_ED_REG_BLK_DIR_SHP]);
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
			break;
		default:
			DE_WRN("not support this vsu_type=%d\n",
				priv->vsu_type);
			break;
		}

		if (rcq_used)
			priv->set_blk_dirty = vsu_set_rcq_head_dirty;
		else
			priv->set_blk_dirty = vsu_set_block_dirty;

	}

	return 0;
}

s32 de_vsu_exit(u32 disp)
{
	u32 chn_num, chn;

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		struct de_vsu_private *priv = &vsu_priv[disp][chn];
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);

		if (reg_mem_info->vir_addr != NULL)
			de_top_reg_memory_free(reg_mem_info->vir_addr,
				reg_mem_info->phy_addr, reg_mem_info->size);
	}

	return 0;
}

s32 de_vsu_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	u32 chn_num, chn;
	u32 total = 0;

	chn_num = de_feat_get_num_chns(disp);

	if (blks == NULL) {
		for (chn = 0; chn < chn_num; ++chn) {
			total += vsu_priv[disp][chn].reg_blk_num;
		}
		*blk_num = total;
		return 0;
	}

	for (chn = 0; chn < chn_num; ++chn) {
		struct de_vsu_private *priv = &(vsu_priv[disp][chn]);
		struct de_reg_block *blk_begin, *blk_end;
		u32 num;

		if (*blk_num >= priv->reg_blk_num) {
			num = priv->reg_blk_num;
		} else {
			DE_WRN("should not happen\n");
			num = *blk_num;
		}
		blk_begin = priv->vsu8_reg_blks;
		blk_end = blk_begin + num;
		for (; blk_begin != blk_end; ++blk_begin) {
			*blks++ = blk_begin;
		}
		total += num;
		*blk_num -= num;
	}
	*blk_num = total;
	return 0;
}

/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include "de_wb_type.h"
#include "de_wb.h"

#include "de_top.h"
#include "de_csc_table.h"
#include "de_cdc.h"

enum {
	WB_REG_BLK_CTL = 0,
	WB_REG_BLK_COEFF0,
	WB_REG_BLK_COEFF1,
	WB_REG_BLK_START, /* must be the last in rtwb */
	WB_REG_BLK_NUM,
};

struct de_wb_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[WB_REG_BLK_NUM];
	void *cdc_hdl;
	struct disp_capture_config user_cfg;

	void (*set_blk_dirty)(struct de_wb_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_wb_private wb_priv[DE_RTWB_NUM];
struct de_rcq_mem_info wb_rcq_info[DE_RTWB_NUM];
u8 __iomem *wb_de_base;

static inline struct wb_reg *get_wb_reg(struct de_wb_private *priv)
{
	return (struct wb_reg *)(priv->reg_blks[0].vir_addr);
}

static inline struct wb_reg *get_wb_hw_reg(struct de_wb_private *priv)
{
	return (struct wb_reg *)(priv->reg_blks[0].reg_addr);
}

static void wb_set_block_dirty(
	struct de_wb_private *priv, u32 blk_id, u32 dirty)
{
	struct de_reg_block *blk = &(priv->reg_blks[blk_id]);

	if (blk->vir_addr != blk->reg_addr)
		blk->dirty = dirty;
}

static void wb_set_rcq_head_dirty(
	struct de_wb_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static void de_wb_dump_user_cfg(struct disp_capture_config *cfg)
{
	struct disp_s_frame_inner *frame;

	DE_WRN("disp=%d, flags=%d\n",
		cfg->disp, cfg->flags);

	frame = &cfg->in_frame;
	DE_WRN("in frame: fmt=%d, fd=%d, sz[%d,%d,%d,%d,%d,%d]"
		", crop[%d,%d,%d,%d], addr[0x%llx,0x%llx,0x%llx]",
		frame->format, frame->fd,
		frame->size[0].width, frame->size[0].height,
		frame->size[1].width, frame->size[1].height,
		frame->size[2].width, frame->size[2].height,
		frame->crop.x, frame->crop.y,
		frame->crop.width, frame->crop.height,
		frame->addr[0], frame->addr[1], frame->addr[2]);

	frame = &cfg->out_frame;
	DE_WRN("in frame: fmt=%d, fd=%d, sz[%d,%d,%d,%d,%d,%d]"
		", crop[%d,%d,%d,%d], addr[0x%llx,0x%llx,0x%llx]",
		frame->format, frame->fd,
		frame->size[0].width, frame->size[0].height,
		frame->size[1].width, frame->size[1].height,
		frame->size[2].width, frame->size[2].height,
		frame->crop.x, frame->crop.y,
		frame->crop.width, frame->crop.height,
		frame->addr[0], frame->addr[1], frame->addr[2]);

}

static u8 is_valid_para(struct disp_capture_config *cfg)
{
	if ((cfg->in_frame.size[0].width < MININWIDTH)
		|| (cfg->in_frame.size[0].width > MAXINWIDTH)
		|| (cfg->in_frame.size[0].height < MININHEIGHT)
		|| (cfg->in_frame.size[0].height > MAXINHEIGHT)
		|| (cfg->in_frame.crop.width < MININWIDTH)
		|| (cfg->in_frame.crop.width > MAXINWIDTH)
		|| (cfg->in_frame.crop.height < MININHEIGHT)
		|| (cfg->in_frame.crop.height > MAXINHEIGHT)
		|| (cfg->in_frame.crop.x + cfg->in_frame.crop.width
			> cfg->in_frame.size[0].width)
		|| (cfg->in_frame.crop.y + cfg->in_frame.crop.height
			> cfg->in_frame.size[0].height)
		|| (cfg->out_frame.size[0].width
			< cfg->out_frame.crop.width)
		|| (cfg->out_frame.crop.width < MININWIDTH)
		|| (cfg->out_frame.crop.width > MAXINWIDTH)
		|| (cfg->out_frame.crop.width > cfg->in_frame.crop.width)
		|| (cfg->out_frame.crop.width > LINE_BUF_LEN)
		|| (cfg->out_frame.size[0].height
			< cfg->out_frame.crop.height)
		|| (cfg->out_frame.crop.height < MININHEIGHT)
		|| (cfg->out_frame.crop.height > MAXINHEIGHT)
		|| (cfg->out_frame.crop.height > cfg->in_frame.crop.height)) {
		return 0;
	}
	switch (cfg->out_frame.format) {
	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_BGRA_8888:
	case DISP_FORMAT_XRGB_8888:
	case DISP_FORMAT_XBGR_8888:
	case DISP_FORMAT_BGRX_8888:
	case DISP_FORMAT_RGBX_8888:
	case DISP_FORMAT_RGB_888:
	case DISP_FORMAT_BGR_888:
	case DISP_FORMAT_YUV420_P:
	case DISP_FORMAT_YUV420_SP_UVUV:
	case DISP_FORMAT_YUV420_SP_VUVU:
	case DISP_FORMAT_YUV444_I_AYUV:
		break;
	case DISP_FORMAT_A2R10G10B10: /* equal x2p10p10p10 */
		/* fixme: this fmt not support crop/scale/csc */
		break;
	default:
		DE_WRN("not support this fmt=%d\n",
			cfg->out_frame.format);
		return 0;
	}
	return 1;
}

static u32 convert_px_fmt_to_space(u32 px_fmt)
{
	switch (px_fmt) {
	case DE_FORMAT_YUV422_I_YVYU:
	case DE_FORMAT_YUV422_I_YUYV:
	case DE_FORMAT_YUV422_I_UYVY:
	case DE_FORMAT_YUV422_I_VYUY:
		return DE_FORMAT_SPACE_YUV;
	case DE_FORMAT_YUV422_P:
		return DE_FORMAT_SPACE_YUV;
	case DE_FORMAT_YUV422_SP_UVUV:
	case DE_FORMAT_YUV422_SP_VUVU:
	case DE_FORMAT_YUV422_SP_UVUV_10BIT:
	case DE_FORMAT_YUV422_SP_VUVU_10BIT:
		return DE_FORMAT_SPACE_YUV;
	case DE_FORMAT_YUV420_P:
		return DE_FORMAT_SPACE_YUV;
	case DE_FORMAT_YUV420_SP_UVUV:
	case DE_FORMAT_YUV420_SP_VUVU:
	case DE_FORMAT_YUV420_SP_UVUV_10BIT:
	case DE_FORMAT_YUV420_SP_VUVU_10BIT:
		return DE_FORMAT_SPACE_YUV;
	case DE_FORMAT_YUV411_P:
		return DE_FORMAT_SPACE_YUV;
	case DE_FORMAT_YUV411_SP_UVUV:
	case DE_FORMAT_YUV411_SP_VUVU:
		return DE_FORMAT_SPACE_YUV;
	case DE_FORMAT_YUV444_I_AYUV:
	case DE_FORMAT_YUV444_I_VUYA:
	case DE_FORMAT_YUV444_I_AYUV_10BIT:
	case DE_FORMAT_YUV444_I_VUYA_10BIT:
		return DE_FORMAT_SPACE_YUV;
	case DE_FORMAT_YUV444_P:
	case DE_FORMAT_YUV444_P_10BIT:
		return DE_FORMAT_SPACE_YUV;
	case DE_FORMAT_8BIT_GRAY:
		return DE_FORMAT_SPACE_GRAY;
	default:
		return DE_FORMAT_SPACE_RGB;
		break;
	}
}

s32 de_wb_writeback_enable(u32 wb, bool en)
{
	struct de_wb_private *priv = &wb_priv[wb];
	struct wb_reg *reg = get_wb_reg(priv);

	en = en ? 1 : 0;
	reg->gctrl.bits.fake_start = 0; /* fake_start is always 0 */
	priv->set_blk_dirty(priv, WB_REG_BLK_CTL, 1);

	reg->start.bits.wb_start = en;
	priv->set_blk_dirty(priv, WB_REG_BLK_START, 1);

	return 0;
}

static s32 de_wb_set_para(u32 wb)
{
	struct de_wb_private *priv = &wb_priv[wb];
	struct disp_capture_config *cfg = &priv->user_cfg;
	struct wb_reg *reg = get_wb_reg(priv);

	u32 in_w, in_h;
	u32 crop_x, crop_y, crop_w, crop_h;
	u32 out_addr[3];
	u32 out_buf_w, out_buf_h;
	u32 in_fmt;
	u32 out_fmt;
	u32 out_window_w, out_window_h, out_window_x, out_window_y;
	u32 cs_out_w0 = 0, cs_out_h0 = 0, cs_out_w1 = 0, cs_out_h1 = 0;
	u32 fs_out_w0, fs_out_h0, fs_out_w1, fs_out_h1;
	u32 step_h, step_v;
	u32 v_intg, v_frac, h_intg, h_frac;
	u32 down_scale_y, down_scale_c;
	u32 i;

	/* get para */
	in_fmt = cfg->in_frame.format;
	in_w = cfg->in_frame.size[0].width;
	in_h = cfg->in_frame.size[0].height;
	crop_x = (u32)cfg->in_frame.crop.x;
	crop_y = (u32)cfg->in_frame.crop.y;
	crop_w = cfg->in_frame.crop.width;
	crop_h = cfg->in_frame.crop.height;

	out_addr[0] = cfg->out_frame.addr[0];
	out_addr[1] = cfg->out_frame.addr[1];
	out_addr[2] = cfg->out_frame.addr[2];
	out_fmt = cfg->out_frame.format;
	out_buf_w = cfg->out_frame.size[0].width;
	out_buf_h = cfg->out_frame.size[0].height;
	out_window_x = cfg->out_frame.crop.x;
	out_window_y = cfg->out_frame.crop.y;
	out_window_w = cfg->out_frame.crop.width;
	out_window_h = cfg->out_frame.crop.height;

	/* input size */
	reg->size.dwval = (in_w - 1) | ((in_h - 1) << 16);
	/* input crop window */
	reg->crop_coord.dwval = crop_x | ((crop_y) << 16);
	reg->crop_size.dwval = (crop_w - 1) | ((crop_h - 1) << 16);
	reg->sftm.bits.sftm_vs = 0x20; /*default*/

	/* output fmt */
	if (out_fmt == DISP_FORMAT_ARGB_8888)
		reg->fmt.dwval = WB_FORMAT_ARGB_8888;
	else if (out_fmt == DISP_FORMAT_ABGR_8888)
		reg->fmt.dwval = WB_FORMAT_ABGR_8888;
	else if (out_fmt == DISP_FORMAT_RGBA_8888)
		reg->fmt.dwval = WB_FORMAT_RGBA_8888;
	else if (out_fmt == DISP_FORMAT_BGRA_8888)
		reg->fmt.dwval = WB_FORMAT_BGRA_8888;
	else if (out_fmt == DISP_FORMAT_RGB_888)
		reg->fmt.dwval = WB_FORMAT_RGB_888;
	else if (out_fmt == DISP_FORMAT_BGR_888)
		reg->fmt.dwval = WB_FORMAT_BGR_888;
	else if (out_fmt == DISP_FORMAT_YUV420_P)
		reg->fmt.dwval = WB_FORMAT_YUV420_P;
	else if (out_fmt == DISP_FORMAT_YUV420_SP_VUVU)
		reg->fmt.dwval = WB_FORMAT_YUV420_SP_UVUV;
	else if (out_fmt == DISP_FORMAT_YUV420_SP_UVUV)
		reg->fmt.dwval = WB_FORMAT_YUV420_SP_VUVU;
	else if (out_fmt == DISP_FORMAT_YUV444_I_AYUV)
		reg->fmt.dwval = WB_FORMAT_ARGB_8888;
	else {
		DE_WRN("unknow out fmt %d\n", out_fmt);
		return -1;
	}

	/* output addr and pitch for different fmt */
	switch (out_fmt) {
	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_BGRA_8888:
	case DISP_FORMAT_YUV444_I_AYUV:
		out_addr[0] += (out_window_y * out_buf_w + out_window_x) << 2;
		reg->wb_addr_a0.dwval = out_addr[0];
		reg->wb_pitch0.dwval = out_buf_w << 2;
		break;
	case DISP_FORMAT_RGB_888:
	case DISP_FORMAT_BGR_888:
		out_addr[0] += (out_window_y * out_buf_w + out_window_x) * 3;
		reg->wb_addr_a0.dwval = out_addr[0];
		reg->wb_pitch0.dwval = 3 * out_buf_w;
		break;
	case DISP_FORMAT_YUV420_SP_VUVU:
	case DISP_FORMAT_YUV420_SP_UVUV:
		out_addr[0] += (out_window_y * out_buf_w + out_window_x);
		out_addr[1] +=
			(((out_window_y * out_buf_w) >> 1) + out_window_x);
		reg->wb_addr_a0.dwval = out_addr[0];
		reg->wb_addr_a1.dwval = out_addr[1];
		reg->wb_pitch0.dwval = out_buf_w;
		reg->wb_pitch1.dwval = out_buf_w;
		break;
	case DISP_FORMAT_YUV420_P:
		out_addr[0] += (out_window_y * out_buf_w + out_window_x);
		out_addr[1] += (out_window_y * out_buf_w + out_window_x) >> 1;
		out_addr[2] += (out_window_y * out_buf_w + out_window_x) >> 1;
		reg->wb_addr_a0.dwval = out_addr[0];
		reg->wb_addr_a1.dwval = out_addr[1];
		reg->wb_addr_a2.dwval = out_addr[2];
		reg->wb_pitch0.dwval = out_buf_w;
		reg->wb_pitch1.dwval = out_buf_w >> 1;
		break;
	default:
		return -1;
	}

	/* Coarse scaling */
	if (crop_w > (out_window_w << 1)) {
		reg->cs_horz.dwval = crop_w | (out_window_w << 17);
		cs_out_w0 = out_window_w << 1;
	} else {
		reg->cs_horz.dwval = 0;
		cs_out_w0 = crop_w;
	}
	if (crop_h > (out_window_h << 1)) {
		reg->cs_vert.dwval = crop_h | (out_window_h << 17);
		cs_out_h0 = out_window_h << 1;
	} else {
		reg->cs_vert.dwval = 0;
		cs_out_h0 = crop_h;
	}
	if ((crop_w > (out_window_w << 1)) && (crop_h > (out_window_h << 1)))
		reg->bypass.dwval |= 0x00000002;
	else
		reg->bypass.dwval &= 0xfffffffd;

	/* Fine scaling */
	cs_out_w1 = cs_out_w0;
	cs_out_h1 = cs_out_h0;
	fs_out_w0 = out_window_w;
	fs_out_w1 =
	    ((out_fmt == DISP_FORMAT_YUV420_P)
	    | (out_fmt == DISP_FORMAT_YUV420_SP_VUVU)
	    | (out_fmt == DISP_FORMAT_YUV420_SP_UVUV)) ?
	    (out_window_w >> 1) : out_window_w;
	fs_out_h0 = out_window_h;
	fs_out_h1 =
	    ((out_fmt == DISP_FORMAT_YUV420_P)
	    | (out_fmt == DISP_FORMAT_YUV420_SP_VUVU)
	    | (out_fmt == DISP_FORMAT_YUV420_SP_UVUV)) ?
	    (out_window_h >> 1) : out_window_h;
	if ((cs_out_w0 == fs_out_w0) && (cs_out_h0 == fs_out_h0)
	    && (cs_out_w1 == fs_out_w1) && (cs_out_h1 == fs_out_h1)) {
		reg->bypass.dwval &= 0xfffffffb;
		reg->fs_hstep.dwval = 1 << 20;
		reg->fs_vstep.dwval = 1 << 20;
	} else {
		unsigned long long tmp;

		reg->bypass.dwval |= 0x00000004;
		tmp = ((long long)cs_out_w0 << LOCFRACBIT);
		do_div(tmp, (long long)out_window_w);
		step_h = (int)tmp;
		tmp = ((long long)cs_out_h0 << LOCFRACBIT);
		do_div(tmp, (long long)out_window_h);
		step_v = (int)tmp;
		h_intg = (step_h & (~((1 << LOCFRACBIT) - 1))) >> LOCFRACBIT;
		h_frac = step_h & ((1 << LOCFRACBIT) - 1);
		v_intg = (step_v & (~((1 << LOCFRACBIT) - 1))) >> LOCFRACBIT;
		v_frac = step_v & ((1 << LOCFRACBIT) - 1);
		reg->fs_hstep.dwval = (h_frac << 2) | (h_intg << 20);
		reg->fs_vstep.dwval = (v_frac << 2) | (v_intg << 20);
		if (cs_out_w0 <= fs_out_w0)
			down_scale_y = 0;
		else
			down_scale_y = 1;
		if (cs_out_w1 <= fs_out_w1)
			down_scale_c = 0;
		else
			down_scale_c = 1;
		for (i = 0; i < SCALERPHASE; i++) {
			unsigned int wb_lan2coefftab16[16] = {
				0x00004000, 0x00033ffe, 0x00063efc, 0x000a3bfb,
				0xff0f37fb, 0xfe1433fb, 0xfd192ffb, 0xfd1f29fb,
				0xfc2424fc, 0xfb291ffd, 0xfb2f19fd, 0xfb3314fe,
				0xfb370fff, 0xfb3b0a00, 0xfc3e0600, 0xfe3f0300
			};
			unsigned int wb_lan2coefftab16_down[16] = {
				0x000e240e, 0x0010240c, 0x0013230a, 0x00142309,
				0x00162208, 0x01182106, 0x011a2005, 0x021b1f04,
				0x031d1d03, 0x041e1c02, 0x05201a01, 0x06211801,
				0x07221601, 0x09231400, 0x0a231300, 0x0c231100
			};

			reg->yhcoeff[i].dwval =
			    down_scale_y ? wb_lan2coefftab16_down[i] :
			    wb_lan2coefftab16[i];
			reg->chcoeff[i].dwval =
			    down_scale_c ? wb_lan2coefftab16_down[i] :
			    wb_lan2coefftab16[i];
		}
		priv->set_blk_dirty(priv, WB_REG_BLK_COEFF0, 1);
		priv->set_blk_dirty(priv, WB_REG_BLK_COEFF1, 1);
	}
	reg->fs_insize.dwval =
	    (cs_out_w0 - 1) | ((cs_out_h0 - 1) << 16);
	reg->fs_outsize.dwval =
	    (out_window_w - 1) | ((out_window_h - 1) << 16);

	priv->set_blk_dirty(priv, WB_REG_BLK_CTL, 1);

	return 0;
}

static s32 de_wb_set_csc_para(u32 wb)
{
	struct de_rtmx_context *ctx;
	struct de_wb_private *priv = &wb_priv[wb];
	struct disp_capture_config *cfg = &priv->user_cfg;
	struct wb_reg *reg = get_wb_reg(priv);
	struct de_csc_info icsc_info;
	struct de_csc_info ocsc_info;
	u32 *csc_coeff = NULL;
	u32 px_fmt;

	ctx = de_rtmx_get_context(cfg->disp);

	px_fmt = cfg->in_frame.format;
	icsc_info.px_fmt_space = ctx->output.px_fmt_space;
	icsc_info.color_space = ctx->output.color_space;
	icsc_info.color_range = ctx->output.color_range;
	icsc_info.eotf = ctx->output.eotf;

	if (priv->cdc_hdl != NULL) {
		if (icsc_info.px_fmt_space == DE_FORMAT_SPACE_YUV) {
			ocsc_info.px_fmt_space = icsc_info.px_fmt_space;
			ocsc_info.color_range = icsc_info.color_range;
			ocsc_info.color_space = DE_COLOR_SPACE_BT709;
			ocsc_info.eotf = DE_EOTF_GAMMA22;
		} else {
			memcpy((void *)&ocsc_info,
				(void *)&icsc_info, sizeof(ocsc_info));
		}
		/* fixme: update reg right now at without rcq */
		de_cdc_set_para(priv->cdc_hdl, &icsc_info, &ocsc_info);
		memcpy((void *)&icsc_info,
			(void *)&ocsc_info, sizeof(ocsc_info));
	}

	px_fmt = cfg->out_frame.format;
	ocsc_info.px_fmt_space = convert_px_fmt_to_space(px_fmt);
	if ((cfg->out_frame.size[0].width <= 736)
		&& (cfg->out_frame.size[0].height <= 576))
		ocsc_info.color_space = DE_COLOR_SPACE_BT601;
	else
		ocsc_info.color_space = DE_COLOR_SPACE_BT709;
	if (ocsc_info.px_fmt_space == DE_FORMAT_SPACE_YUV)
		ocsc_info.color_range = DE_COLOR_RANGE_16_235;
	else
		ocsc_info.color_range = DE_COLOR_RANGE_0_255;
	ocsc_info.eotf = DE_EOTF_GAMMA22;
	de_csc_coeff_calc(&icsc_info, &ocsc_info, &csc_coeff);
	if (NULL != csc_coeff) {
		s32 dwval;

		reg->csc_ctl.dwval = 1;

		dwval = *(csc_coeff + 12);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->d0.dwval = dwval;
		dwval = *(csc_coeff + 13);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->d1.dwval = dwval;
		dwval = *(csc_coeff + 14);
		dwval = ((dwval & 0x80000000) ? (u32)(-(s32)dwval) : dwval) & 0x3FF;
		reg->d2.dwval = dwval;

		reg->c0[0].dwval = *(csc_coeff + 0);
		reg->c0[1].dwval = *(csc_coeff + 1);
		reg->c0[2].dwval = *(csc_coeff + 2);
		reg->c0[3].dwval = *(csc_coeff + 3);

		reg->c1[0].dwval = *(csc_coeff + 4);
		reg->c1[1].dwval = *(csc_coeff + 5);
		reg->c1[2].dwval = *(csc_coeff + 6);
		reg->c1[3].dwval = *(csc_coeff + 7);

		reg->c2[0].dwval = *(csc_coeff + 8);
		reg->c2[1].dwval = *(csc_coeff + 9);
		reg->c2[2].dwval = *(csc_coeff + 10);
		reg->c2[3].dwval = *(csc_coeff + 11);

		priv->set_blk_dirty(priv, WB_REG_BLK_CTL, 1);
	}

	return 0;
}

s32 de_wb_apply(u32 wb, struct disp_capture_config *cfg)
{
	struct de_wb_private *priv = &wb_priv[wb];

	memcpy((void *)&priv->user_cfg, (void *)cfg,
		sizeof(priv->user_cfg));

	if (!is_valid_para(&priv->user_cfg)) {
		de_wb_dump_user_cfg(&priv->user_cfg);
		return -1;
	}

	de_wb_set_para(wb);
	de_wb_set_csc_para(wb);
	de_wb_writeback_enable(wb, 1);

	return 0;
}

s32 de_wb_update_regs_ahb(u32 wb)
{
	struct de_rcq_mem_info *rcq_info = &wb_rcq_info[wb];
	struct de_reg_block **p_reg_blk = rcq_info->reg_blk;
	struct de_reg_block **p_reg_blk_end =
		p_reg_blk + rcq_info->cur_num;

	for (; p_reg_blk != p_reg_blk_end; ++p_reg_blk) {
		struct de_reg_block *reg_blk = *p_reg_blk;

		if (reg_blk->dirty) {
			memcpy((void *)reg_blk->reg_addr,
				(void *)reg_blk->vir_addr, reg_blk->size);
			reg_blk->dirty = 0;
		}
	}

	return 0;
}

u32 de_wb_get_status(u32 wb)
{
	struct de_wb_private *priv = &wb_priv[wb];
	struct wb_reg *reg = get_wb_hw_reg(priv);
	u32 status;

	status = reg->status.dwval & 0x71;
	reg->status.dwval = 0x71;

	if (status & 0x10)
		return 0;
	else if (status & 0x20)
		return 1;
	else if (status & 0x40)
		return 2;
	else
		return 3;
}

s32 de_wb_set_rcq_update(u32 wb, u32 en)
{
	if (en) {
		struct de_rcq_mem_info *rcq_info = &wb_rcq_info[wb];
		struct de_rcq_head *hd = rcq_info->vir_addr;
		struct de_rcq_head *hd_end = hd + rcq_info->cur_num;

		for (; hd != hd_end; ++hd) {
			if (hd->dirty.dwval)
				return de_top_wb_set_rcq_update(wb, en);
		}
	} else {
		de_top_wb_set_rcq_update(wb, en);
	}
	return 0;
}

static s32 de_wb_set_all_reg_block_dirty(u32 wb, u32 dirty)
{
	struct de_rcq_mem_info *rcq_info = &wb_rcq_info[wb];
	struct de_reg_block **pblk = rcq_info->reg_blk;
	struct de_reg_block **pblk_end = pblk + rcq_info->cur_num;

	if (pblk == NULL) {
		DE_WRN("p_reg_blk is null\n");
		return -1;
	}

	for (; pblk != pblk_end; pblk++)
		(*pblk)->dirty = dirty;

	return 0;
}

s32 de_wb_set_all_rcq_head_dirty(u32 wb, u32 dirty)
{
	struct de_rcq_mem_info *rcq_info = &wb_rcq_info[wb];
	struct de_rcq_head *hd = rcq_info->vir_addr;
	struct de_rcq_head *hd_end = hd + rcq_info->cur_num;

	if (hd == NULL) {
		DE_WRN("rcq head is null\n");
		return -1;
	}

	for (; hd != hd_end; hd++)
		hd->dirty.dwval = dirty;

	return 0;
}


s32 de_wb_init_rcq(u32 wb)
{
	struct de_rcq_mem_info *rcq_info = &wb_rcq_info[wb];
	struct de_rcq_head *rcq_hd;
	struct de_reg_block **p_reg_blks;
	u32 num;
	u32 rcq_used = de_feat_is_using_wb_rcq(wb);

	de_wb_get_reg_blocks(wb, NULL, &num);
	rcq_info->reg_blk = kmalloc(
		sizeof(*(rcq_info->reg_blk)) * num,
		GFP_KERNEL | __GFP_ZERO);
	if (rcq_info->reg_blk == NULL) {
		DE_WRN("kalloc for wb reg block failed, num=%d\n", num);
		return -1;
	}
	rcq_info->cur_num = num;

	p_reg_blks = rcq_info->reg_blk;
	de_wb_get_reg_blocks(wb, p_reg_blks, &num);
	if (num != rcq_info->cur_num)
		DE_WRN("checkit: num=%d, cur_num=%d\n",
			num, rcq_info->cur_num);

	if (rcq_used) {
		rcq_info->alloc_num = ((rcq_info->cur_num + 1) >> 1) << 1;
		rcq_info->vir_addr = (struct de_rcq_head *)
			de_top_reg_memory_alloc(
			 rcq_info->alloc_num * sizeof(*(rcq_info->vir_addr)),
			 &(rcq_info->phy_addr), rcq_used);
		if (rcq_info->vir_addr == NULL) {
			DE_WRN("alloc for de_rcq_head failed\n");
			return -1;
		}
		p_reg_blks	= rcq_info->reg_blk;
		rcq_hd = rcq_info->vir_addr;
		for (num = 0; num < rcq_info->cur_num; ++num) {
			struct de_reg_block *reg_blk = *p_reg_blks;

			rcq_hd->low_addr = (u32)((uintptr_t)(reg_blk->phy_addr));
			rcq_hd->dw0.bits.high_addr =
				(u8)((uintptr_t)(reg_blk->phy_addr) >> 32);
			rcq_hd->dw0.bits.len = reg_blk->size;
			rcq_hd->dirty.dwval = reg_blk->dirty;
			rcq_hd->reg_offset = (u32)(uintptr_t)(
				reg_blk->reg_addr - wb_de_base);
			reg_blk->rcq_hd = rcq_hd;
			rcq_hd++;
			p_reg_blks++;
		}
		for (; num < rcq_info->alloc_num; ++num) {
			rcq_hd->dirty.dwval = 0;
			rcq_hd++;
		}

	}

	return 0;
}

s32 de_wb_setup_rcq(u32 wb)
{
	struct de_rcq_mem_info *rcq_info = &wb_rcq_info[wb];

	de_top_wb_set_rcq_head(wb, (u64)(rcq_info->phy_addr),
		rcq_info->alloc_num * sizeof(*(rcq_info->vir_addr)));
	return 0;
}

s32 de_wb_irq_query_and_clear(u32 wb, enum wb_irq_state irq_state)
{
	struct de_wb_private *priv = &wb_priv[wb];
	struct wb_reg *hw_reg = get_wb_hw_reg(priv);
	u32 reg_val, state;

	reg_val = hw_reg->status.dwval;
	state = reg_val & irq_state & WB_IRQ_STATE_MASK;

	reg_val &= ~WB_IRQ_STATE_MASK;
	reg_val |= state;
	hw_reg->status.dwval = reg_val;

	return state;

}

s32 de_wb_enable_irq(u32 wb, u32 en)
{
	struct de_wb_private *priv = &wb_priv[wb];
	struct wb_reg *hw_reg = get_wb_hw_reg(priv);
	struct wb_reg *reg = get_wb_reg(priv);


	reg->intr.bits.int_en = en;
	hw_reg->intr.bits.int_en = en;
	return 0;
}

s32 de_wb_start(u32 wb, u32 wb_mux)
{
	struct de_wb_private *priv = &wb_priv[wb];
	struct wb_reg *reg = get_wb_reg(priv);
	struct wb_reg *hw_reg = get_wb_hw_reg(priv);

	if (de_feat_is_using_wb_rcq(wb))
		de_wb_set_all_rcq_head_dirty(wb, 0);
	else
		de_wb_set_all_reg_block_dirty(wb, 0);

	de_top_set_clk_enable(DE_CLK_WB, 1);

	reg->gctrl.dwval = 0x10000000;
	reg->start.dwval = 0x10000000;
	hw_reg->gctrl.dwval = 0x10000000;

	de_top_set_rtwb_mux(wb, wb_mux);
	de_wb_setup_rcq(wb);

	return 0;
}

void de_wb_rest(u32 wb, u32 reset)
{
	struct de_wb_private *priv = &wb_priv[wb];
	struct wb_reg *hw_reg = get_wb_hw_reg(priv);
	if (reset)
		hw_reg->gctrl.dwval = 0x20000010; /* stop now */
	else
		hw_reg->gctrl.dwval = 0x10000000;
}

s32 de_wb_stop(u32 wb)
{
	struct de_wb_private *priv = &wb_priv[wb];
	struct wb_reg *reg = get_wb_reg(priv);
	struct wb_reg *hw_reg = get_wb_hw_reg(priv);

	priv->set_blk_dirty(priv, WB_REG_BLK_CTL, 0);
	reg->gctrl.dwval = 0x20000010;
	reg->start.dwval = 0x20000010;
	hw_reg->gctrl.dwval = 0x20000010; /* stop now */
	de_top_start_rtwb(wb, 0);

	de_top_set_clk_enable(DE_CLK_WB, 0);
	return 0;
}

s32 de_wb_init(struct disp_bsp_init_para *para)
{
	u32 wb, wb_num;

	wb_de_base = (u8 __iomem *)para->reg_base[DISP_MOD_DE];

	wb_num = sizeof(wb_priv) / sizeof(wb_priv[0]);
	if (wb_num > 1) {
		DE_WRN("not support more than 1 wb at present\n");
		wb_num = 1;
	}
	for (wb = 0; wb < wb_num; ++wb) {
		struct de_wb_private *priv = &(wb_priv[wb]);
		struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
		struct de_reg_block *blk;
		u32 rcq_used = de_feat_is_using_wb_rcq(wb);
		u8 __iomem *reg_base = (u8 __iomem *)(
			para->reg_base[DISP_MOD_DE]
			+ DE_TOP_RTWB_OFFSET + RTWB_WB_OFFSET);

		reg_mem_info->size = sizeof(struct wb_reg);
		reg_mem_info->vir_addr = de_top_reg_memory_alloc(
		    reg_mem_info->size, &(reg_mem_info->phy_addr), rcq_used);
		if (reg_mem_info->vir_addr == NULL) {
			DE_WRN("alloc wb[%d] mm fail!size=0x%x\n", wb,
			       reg_mem_info->size);
			return -1;
		}

		blk = &(priv->reg_blks[WB_REG_BLK_CTL]);
		blk->phy_addr = reg_mem_info->phy_addr;
		blk->vir_addr = reg_mem_info->vir_addr;
		blk->size = 0xd0;
		blk->reg_addr = reg_base;

		blk = &(priv->reg_blks[WB_REG_BLK_COEFF0]);
		blk->phy_addr = reg_mem_info->phy_addr + 0x200;
		blk->vir_addr = reg_mem_info->vir_addr + 0x200;
		blk->size = 0x40;
		blk->reg_addr = reg_base + 0x200;

		blk = &(priv->reg_blks[WB_REG_BLK_COEFF1]);
		blk->phy_addr = reg_mem_info->phy_addr + 0x280;
		blk->vir_addr = reg_mem_info->vir_addr + 0x280;
		blk->size = 0x40;
		blk->reg_addr = reg_base + 0x280;

		blk = &(priv->reg_blks[WB_REG_BLK_START]);
		blk->phy_addr = reg_mem_info->phy_addr + 0xE0;
		blk->vir_addr = reg_mem_info->vir_addr + 0xE0;
		blk->size = 0x4;
		blk->reg_addr = reg_base;

		priv->reg_blk_num = WB_REG_BLK_NUM;

		reg_base = (u8 __iomem *)(para->reg_base[DISP_MOD_DE]
			+ DE_TOP_RTWB_OFFSET + RTWB_CDC_OFFSET);
		priv->cdc_hdl = de_cdc_create(reg_base, rcq_used, 1);
		if (priv->cdc_hdl == NULL) {
			DE_WRN("de_cdc_create for wb failed\n");
		}

		if (rcq_used)
			priv->set_blk_dirty = wb_set_rcq_head_dirty;
		else
			priv->set_blk_dirty = wb_set_block_dirty;

		de_wb_init_rcq(wb);

	}
	return 0;
}

s32 de_wb_exit(void)
{
	u32 wb;
	struct de_wb_private *priv;
	struct de_reg_mem_info *reg_mem_info;

	for (wb = 0; wb < DE_RTWB_NUM; ++wb) {
		priv = &(wb_priv[wb]);

		if (priv->cdc_hdl)
			de_cdc_destroy(priv->cdc_hdl);

		reg_mem_info = &priv->reg_mem_info;
		if (reg_mem_info->vir_addr)
			de_top_reg_memory_free(reg_mem_info->vir_addr,
				reg_mem_info->phy_addr, reg_mem_info->size);
		reg_mem_info->vir_addr = NULL;
	}
	return 0;
}

s32 de_wb_get_reg_blocks(u32 wb,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_wb_private *priv = &(wb_priv[wb]);
	u32 i = 0, num = 0;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		if (priv->cdc_hdl) {
			de_cdc_get_reg_blks(priv->cdc_hdl, NULL, &num);
			*blk_num += num;
		}
		return 0;
	}

	if (priv->cdc_hdl)
		de_cdc_get_reg_blks(priv->cdc_hdl, NULL, &num);
	num += priv->reg_blk_num;
	if (*blk_num < num) {
		DE_WRN("should not happen: blk_num=%d\n", *blk_num);
		*blk_num = 0;
		return -1;
	}

	for (i = 0; i < WB_REG_BLK_START; ++i)
		blks[i] = priv->reg_blks + i;

	if (priv->cdc_hdl) {
		num -= i;
		de_cdc_get_reg_blks(priv->cdc_hdl,
			blks + i, &num);
		i += num;
	}

	blks[i] = priv->reg_blks + WB_REG_BLK_START;
	i++;

	*blk_num = i;
	return 0;
}

/* *************************************************************** */

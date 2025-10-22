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

#include "include.h"
#include "tcon_tv_reg.h"
#include "tcon_top.h"
#include "tcon_tv.h"

s32 tcon_tv_set_reg_base(struct sunxi_tcon_tv *tcon, uintptr_t base)
{
	// we can choose any other tcon register struct here in another xxx_reg.c,
	// to compatible different platforms which has significant modify
	tcon->reg = (struct tcon_tv_reg *) (uintptr_t) (base);
	return 0;
}

s32 tcon_tv_init(struct sunxi_tcon_tv *tcon)
{
	/* FIXME:this reg is nolonger exist */
	tcon->reg->tcon_tv_ctl.bits.tcon_tv_en = 0;

	tcon->reg->tcon_gctl.bits.tcon_en = 0;
	tcon->reg->tcon_gint0.bits.tcon_irq_en = 0;
	tcon->reg->tcon_gint0.bits.tcon_irq_flag = 0;
	tcon->reg->tcon_gctl.bits.tcon_en = 1;
	tcon->reg->tv_data_io_pol0.dwval = 0x0;
	tcon->reg->tv_data_io_pol1.dwval = 0x0;
	tcon->reg->tv_data_io_tri0.dwval = 0x0;
	tcon->reg->tv_data_io_tri1.dwval = 0x0;
	tcon->reg->pixel_depth_mode.dwval = 0x0;

	return 0;
}

s32 tcon_tv_exit(struct sunxi_tcon_tv *tcon)
{
	tcon->reg->tcon_gctl.bits.tcon_en = 0;

	return 0;
}

s32 tcon_tv_irq_enable(struct sunxi_tcon_tv *tcon, enum __lcd_irq_id_t id)
{
	tcon->reg->tcon_gint0.bits.tcon_irq_en |= (1 << id);

	return 0;
}

s32 tcon_tv_irq_disable(struct sunxi_tcon_tv *tcon, enum __lcd_irq_id_t id)
{
	tcon->reg->tcon_gint0.bits.tcon_irq_en &= ~(1 << id);

	return 0;
}

u32 tcon_tv_irq_query(struct sunxi_tcon_tv *tcon, enum __lcd_irq_id_t id)
{
	u32 en, fl;

	en = tcon->reg->tcon_gint0.bits.tcon_irq_en;
	fl = tcon->reg->tcon_gint0.bits.tcon_irq_flag;
	if (en & fl & (((u32) 1) << id)) {
		tcon->reg->tcon_gint0.bits.tcon_irq_flag &=
		    ~(((u32) 1) << id);
		return 1;
	} else
		return 0;
}

u32 tcon_tv_get_start_delay(struct sunxi_tcon_tv *tcon)
{
	return tcon->reg->tcon_tv_ctl.bits.start_delay;
}

u32 tcon_tv_get_cur_line(struct sunxi_tcon_tv *tcon)
{
	return tcon->reg->tcon_debug.bits.tcon_tv_current_line;
}

s32 tcon_tv_get_status(struct sunxi_tcon_tv *tcon)
{
	if (tcon->reg->tcon_debug.bits.tcon_tv_fifo_under_flow) {
		tcon->reg->tcon_debug.bits.tcon_tv_fifo_under_flow = 0;
		return -1;
	}
	return 0;
}

s32 tcon_tv_open(struct sunxi_tcon_tv *tcon)
{
	tcon->reg->tcon_tv_ctl.bits.tcon_tv_en = 1;
	tcon_tv_irq_enable(tcon, LCD_IRQ_TCON1_VBLK);

	return 0;
}

s32 tcon_tv_close(struct sunxi_tcon_tv *tcon)
{
	tcon->reg->tcon_tv_ctl.bits.tcon_tv_en = 0;
	tcon_tv_irq_disable(tcon, LCD_IRQ_TCON0_VBLK);
	return 0;
}

s32 tcon_tv_cfg(struct sunxi_tcon_tv *tcon, struct disp_video_timings *timing)
{
	u32 start_delay;
#if (IS_ENABLED(CONFIG_ARCH_SUN60IW2)) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	tcon->reg->tcon_tv_basic1.bits.vic39 = timing->vic == 39 ? 0x1 : 0x0;
	tcon->reg->tcon_tv_basic1.bits.vt    = timing->b_interlace ?
			timing->ver_total_time : timing->ver_total_time * 2;
#else
	tcon->reg->tcon_tv_basic0.bits.x = timing->x_res - 1;
	tcon->reg->tcon_tv_basic0.bits.y =
	    timing->y_res / (timing->b_interlace + 1) - 1;
	tcon->reg->tcon_tv_basic1.bits.ls_xo = timing->x_res - 1;
	tcon->reg->tcon_tv_basic1.bits.ls_yo = timing->y_res
	    / (timing->b_interlace + 1) + timing->vactive_space - 1;
#endif

	tcon->reg->tcon_tv_basic2.bits.xo = timing->x_res - 1;
	tcon->reg->tcon_tv_basic2.bits.yo = timing->y_res
	    / (timing->b_interlace + 1) + timing->vactive_space - 1;
	tcon->reg->tcon_tv_basic3.bits.ht = timing->hor_total_time - 1;
	tcon->reg->tcon_tv_basic3.bits.hbp =
	    timing->hor_sync_time + timing->hor_back_porch - 1;
	tcon->reg->tcon_tv_basic4.bits.vt =
	    timing->ver_total_time * (2 - timing->b_interlace) *
	    ((timing->vactive_space != 0) ? 2 : 1);
	tcon->reg->tcon_tv_basic4.bits.vbp =
	    timing->ver_sync_time + timing->ver_back_porch - 1;
	tcon->reg->tcon_tv_basic5.bits.hspw = timing->hor_sync_time - 1;
	tcon->reg->tcon_tv_basic5.bits.vspw = timing->ver_sync_time - 1;

	tcon->reg->tcon_tv_io_pol.bits.io0_inv = timing->ver_sync_polarity;
	tcon->reg->tcon_tv_io_pol.bits.io1_inv = timing->hor_sync_polarity;

	tcon->reg->tcon_tv_ctl.bits.interlace_en = timing->b_interlace;
	tcon->reg->tcon_fill_start0.bits.fill_begin =
	    (timing->ver_total_time + 1) << 12;
	tcon->reg->tcon_fill_end0.bits.fill_end =
	    (timing->ver_total_time + timing->vactive_space) << 12;
	tcon->reg->tcon_fill_data0.bits.fill_value = 0;
	tcon->reg->tcon_fill_ctl.bits.tcon_tv_fill_en =
	    (timing->vactive_space != 0) ? 1 : 0;
	start_delay = (timing->ver_total_time - timing->y_res)
	    / (timing->b_interlace + 1) - 5;
	start_delay = (start_delay > 31) ? 31 : start_delay;
	tcon->reg->tcon_tv_ctl.bits.start_delay = start_delay;

	return 0;
}

s32 tcon_tv_hdmi_color_remap(struct sunxi_tcon_tv *tcon, u32 onoff)
{
	/*
	 * plane sequence:
	 * v: 16~240
	 * y: 16~235
	 * u: 16~240
	 */
	tcon->reg->tcon_ceu_coef_rr.bits.value = 0;
	tcon->reg->tcon_ceu_coef_rg.bits.value = 0;
	tcon->reg->tcon_ceu_coef_rb.bits.value = 0x100;
	tcon->reg->tcon_ceu_coef_rc.bits.value = 0;

	tcon->reg->tcon_ceu_coef_gr.bits.value = 0x100;
	tcon->reg->tcon_ceu_coef_gg.bits.value = 0;
	tcon->reg->tcon_ceu_coef_gb.bits.value = 0;
	tcon->reg->tcon_ceu_coef_gc.bits.value = 0;

	tcon->reg->tcon_ceu_coef_br.bits.value = 0;
	tcon->reg->tcon_ceu_coef_bg.bits.value = 0x100;
	tcon->reg->tcon_ceu_coef_bb.bits.value = 0;
	tcon->reg->tcon_ceu_coef_bc.bits.value = 0;

	tcon->reg->tcon_ceu_coef_rv.bits.max = 240;
	tcon->reg->tcon_ceu_coef_rv.bits.min = 16;
	tcon->reg->tcon_ceu_coef_gv.bits.max = 235;
	tcon->reg->tcon_ceu_coef_gv.bits.min = 16;
	tcon->reg->tcon_ceu_coef_bv.bits.max = 240;
	tcon->reg->tcon_ceu_coef_bv.bits.min = 16;

	if (onoff)
		tcon->reg->tcon_ceu_ctl.bits.ceu_en = 1;
	else
		tcon->reg->tcon_ceu_ctl.bits.ceu_en = 0;

	return 0;
}

s32 tcon_tv_set_pixel_mode(struct sunxi_tcon_tv *tcon, unsigned int pixel_mode)
{
	if (pixel_mode == 1)
		tcon->reg->tcon_gctl.bits.pixel_mode = 0;
	else if (pixel_mode == 2)
		tcon->reg->tcon_gctl.bits.pixel_mode = 1;
	else if (pixel_mode == 4)
		tcon->reg->tcon_gctl.bits.pixel_mode = 2;

	return 0;
}


s32 tcon_tv_set_timming(struct sunxi_tcon_tv *tcon, struct disp_video_timings *timming)
{
	tcon_tv_cfg(tcon, timming);

#if defined(HAVE_DEVICE_COMMON_MODULE)
	/* these register in tv_tcon1 maping to tcon1's tcon0 position */
	tcon->reg->tcon_tv_io_tri.bits.io0_output_tri_en = 0;
	tcon->reg->tcon_tv_io_tri.bits.io1_output_tri_en = 0;
	tcon->reg->tcon_tv_io_tri.bits.io2_output_tri_en = 1;
	tcon->reg->tcon_tv_io_tri.bits.io3_output_tri_en = 1;
	tcon->reg->tcon_tv_io_tri.bits.data_output_tri_en = 0xffffff;
#else
	tcon->reg->tcon_tv_io_pol.bits.io2_inv = 1;
	tcon->reg->tcon_tv_io_tri.bits.io0_output_tri_en = 1;
	tcon->reg->tcon_tv_io_tri.bits.io1_output_tri_en = 1;
	tcon->reg->tcon_tv_io_tri.bits.io2_output_tri_en = 1;
	tcon->reg->tcon_tv_io_tri.bits.io3_output_tri_en = 1;
	tcon->reg->tcon_tv_io_tri.bits.data_output_tri_en = 0xffffff;
#endif
	return 0;
}

s32 tcon_tv_src_select(struct sunxi_tcon_tv *tcon, enum __lcd_src_t src, unsigned int de_no)
{
	if (src == LCD_SRC_BLUE) {
		tcon->reg->tcon_tv_ctl.bits.src_sel = 2;
	} else {
		tcon->reg->tcon_tv_ctl.bits.src_sel = src;
		if (src == LCD_SRC_DE)
			tcon_de_attach(tcon->tcon_index, de_no);
	}
	return 0;
}

s32 tcon_tv_src_get(struct sunxi_tcon_tv *tcon)
{
	u32 src = 0;

	src = tcon->reg->tcon_tv_ctl.bits.src_sel;
	if (src == 2)
		return LCD_SRC_BLUE;

	src = tcon->reg->tcon_tv_ctl.bits.src_sel;

	return src;
}

static u32 tcon_ceu_range_cut(s32 *x_value, s32 x_min, s32 x_max)
{
	if (*x_value > x_max) {
		*x_value = x_max;
		return 1;
	} else if (*x_value < x_min) {
		*x_value = x_min;
		return 1;
	} else
		return 0;
}

static s32 tcon_ceu_reg_corr(s32 val, u32 bit)
{
	if (val >= 0)
		return val;
	else
		return (bit) | (u32) (-val);
}

static s32 tcon_ceu_rect_multi(s32 *dest, s32 *src1, s32 *src2)
{
	u32 x, y, z;
	__s64 val_int64;

	for (x = 0; x < 4; x++)
		for (y = 0; y < 4; y++) {
			val_int64 = 0;
			for (z = 0; z < 4; z++)
				val_int64 +=
				    (__s64) src1[x * 4 + z] * src2[z * 4 + y];
			val_int64 = (val_int64 + 512) >> 10;
			dest[x * 4 + y] = val_int64;
		}
	return 0;
}

static s32 tcon_ceu_rect_calc(s32 *p_rect, s32 b, s32 c, s32 s, s32 h)
{
	u8 const table_sin[91] = {
		0, 2, 4, 7, 9, 11, 13, 16, 18, 20,
		22, 24, 27, 29, 31, 33, 35, 37, 40, 42,
		44, 46, 48, 50, 52, 54, 56, 58, 60, 62,
		64, 66, 68, 70, 72, 73, 75, 77, 79, 81,
		82, 84, 86, 87, 89, 91, 92, 94, 95, 97,
		98, 99, 101, 102, 104, 105, 106, 107, 109, 110,
		111, 112, 113, 114, 115, 116, 117, 118, 119, 119,
		120, 121, 122, 122, 123, 124, 124, 125, 125, 126,
		126, 126, 127, 127, 127, 128, 128, 128, 128, 128,
		128
	};

	s32 const f_csh = 1024;
	s32 const f_sh = 8;
	s32 h1 = 0, h2 = 0, h3 = 0, h4 = 0;

	if (h >= 0 && h < 90) {
		h1 = table_sin[90 - h];
		h2 = table_sin[h];
		h3 = -table_sin[h];
		h4 = table_sin[90 - h];
	} else if (h >= 90 && h < 180) {
		h1 = -table_sin[h - 90];
		h2 = table_sin[180 - h];
		h3 = -table_sin[180 - h];
		h4 = -table_sin[h - 90];
	} else if (h >= 180 && h < 270) {
		h1 = -table_sin[270 - h];
		h2 = -table_sin[h - 180];
		h3 = table_sin[h - 180];
		h4 = -table_sin[270 - h];
	} else if (h >= 270 && h <= 360) {
		h1 = table_sin[h - 270];
		h2 = -table_sin[360 - h];
		h3 = table_sin[360 - h];
		h4 = table_sin[h - 270];
	}

	p_rect[0] = c * f_sh;
	p_rect[1] = 0;
	p_rect[2] = 0;
	p_rect[3] = -16 * c * f_sh + (b + 16) * f_csh;
	p_rect[4] = 0;
	p_rect[5] = (c * s * h1) >> 11;
	p_rect[6] = (c * s * h2) >> 11;
	p_rect[7] = 128 * (1 * f_csh - p_rect[5] - p_rect[6]);
	p_rect[8] = 0;
	p_rect[9] = (c * s * h3) >> 11;
	p_rect[10] = (c * s * h4) >> 11;
	p_rect[11] = 128 * (1 * f_csh - p_rect[9] - p_rect[10]);
	p_rect[12] = 0;
	p_rect[13] = 0;
	p_rect[14] = 0;
	p_rect[15] = 1024;
	return 0;
}

static s32 tcon_ceu_calc(u32 r2y_type, u32 cen_type, u32 y2r_type, s32 b, s32 c,
			 s32 s, s32 h, s32 *p_coff)
{
	const s32 rect_1[16] = {
		1024, 0, 0, 0,
		0, 1024, 0, 0,
		0, 0, 1024, 0,
		0, 0, 0, 1024
	};

	const s32 rect_r2y_sd[16] = {
		263, 516, 100, 16384,
		-152, -298, 450, 131072,
		450, -377, -73, 131072,
		0, 0, 0, 1024
	};

	const s32 rect_r2y_hd[16] = {
		187, 629, 63, 16384,
		-103, -346, 450, 131072,
		450, -409, -41, 131072,
		0, 0, 0, 1024
	};

	const s32 rect_y2r_sd[16] = {
		1192, 0, 1634, -228262,
		1192, -400, -833, 138740,
		1192, 2066, 0, -283574,
		0, 0, 0, 1024
	};

	const s32 rect_y2r_hd[16] = {
		1192, 0, 1836, -254083,
		1192, -218, -547, 78840,
		1192, 2166, 0, -296288,
		0, 0, 0, 1024
	};

	s32 rect_tmp0[16];
	s32 rect_tmp1[16];

	s32 *p_rect = NULL;
	s32 *p_r2y = NULL;
	s32 *p_y2r = NULL;
	s32 *p_ceu = NULL;
	u32 i = 0;

	if (r2y_type) {
		if (r2y_type == 1)
			p_r2y = (s32 *) rect_r2y_sd;
		else if (r2y_type == 2)
			p_r2y = (s32 *) rect_r2y_hd;
		p_rect = p_r2y;
	} else
		p_rect = (s32 *) rect_1;

	if (cen_type) {
		tcon_ceu_range_cut(&b, -600, 600);
		tcon_ceu_range_cut(&c, 0, 300);
		tcon_ceu_range_cut(&s, 0, 300);
		tcon_ceu_range_cut(&h, 0, 360);
		p_ceu = rect_tmp1;
		tcon_ceu_rect_calc(p_ceu, b, c, s, h);
		tcon_ceu_rect_multi(rect_tmp0, p_ceu, p_rect);
		p_rect = rect_tmp0;
	}

	if (y2r_type) {
		if (y2r_type == 1)
			p_y2r = (s32 *) rect_y2r_sd;
		else if (y2r_type == 2)
			p_y2r = (s32 *) rect_y2r_hd;
		tcon_ceu_rect_multi(rect_tmp1, p_y2r, p_rect);
		p_rect = rect_tmp1;
	}

	for (i = 0; i < 12; i++)
		*(p_coff + i) = *(p_rect + i);

	return 0;
}

s32 tcon_tv_ceu(struct sunxi_tcon_tv *tcon, u32 mode, s32 b, s32 c, s32 s, s32 h)
{
	s32 ceu_coff[12];
	u32 error;

	if (mode == 1) {
		tcon_ceu_calc(1, 1, 1, b, c, s, h, ceu_coff);
	} else if (mode == 2) {
		tcon_ceu_calc(0, 1, 0, b, c, s, h, ceu_coff);
	} else {
		tcon->reg->tcon_ceu_ctl.bits.ceu_en = 0;
		return 0;
	}

	ceu_coff[0] = (ceu_coff[0] + 2) >> 2;
	ceu_coff[1] = (ceu_coff[1] + 2) >> 2;
	ceu_coff[2] = (ceu_coff[2] + 2) >> 2;
	ceu_coff[3] = (ceu_coff[3] + 32) >> 6;
	ceu_coff[4] = (ceu_coff[4] + 2) >> 2;
	ceu_coff[5] = (ceu_coff[5] + 2) >> 2;
	ceu_coff[6] = (ceu_coff[6] + 2) >> 2;
	ceu_coff[7] = (ceu_coff[7] + 32) >> 6;
	ceu_coff[8] = (ceu_coff[8] + 2) >> 2;
	ceu_coff[9] = (ceu_coff[9] + 2) >> 2;
	ceu_coff[10] = (ceu_coff[10] + 2) >> 2;
	ceu_coff[11] = (ceu_coff[11] + 32) >> 6;

	error = 0;
	error |= tcon_ceu_range_cut(ceu_coff + 0, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 1, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 2, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 3, -262143, 262143);
	error |= tcon_ceu_range_cut(ceu_coff + 4, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 5, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 6, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 7, -262143, 262143);
	error |= tcon_ceu_range_cut(ceu_coff + 8, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 9, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 10, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 11, -262143, 262143);

	if (error) {
		tcon->reg->tcon_ceu_ctl.bits.ceu_en = 0;
		return -1;
	}

	if (mode == 1) {
		tcon->reg->tcon_ceu_coef_rv.bits.max = 255;
		tcon->reg->tcon_ceu_coef_rv.bits.min = 0;
		tcon->reg->tcon_ceu_coef_gv.bits.max = 255;
		tcon->reg->tcon_ceu_coef_gv.bits.min = 0;
		tcon->reg->tcon_ceu_coef_bv.bits.max = 255;
		tcon->reg->tcon_ceu_coef_bv.bits.min = 0;
	} else if (mode == 2) {
		tcon->reg->tcon_ceu_coef_rv.bits.max = 235;
		tcon->reg->tcon_ceu_coef_rv.bits.min = 16;
		tcon->reg->tcon_ceu_coef_gv.bits.max = 240;
		tcon->reg->tcon_ceu_coef_gv.bits.min = 16;
		tcon->reg->tcon_ceu_coef_bv.bits.max = 240;
		tcon->reg->tcon_ceu_coef_bv.bits.min = 16;
	}
	tcon->reg->tcon_ceu_coef_rr.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[0], 1 << 12);
	tcon->reg->tcon_ceu_coef_rg.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[1], 1 << 12);
	tcon->reg->tcon_ceu_coef_rb.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[2], 1 << 12);
	tcon->reg->tcon_ceu_coef_rc.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[3], 1 << 18);
	tcon->reg->tcon_ceu_coef_gr.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[0], 1 << 12);
	tcon->reg->tcon_ceu_coef_gg.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[1], 1 << 12);
	tcon->reg->tcon_ceu_coef_gb.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[2], 1 << 12);
	tcon->reg->tcon_ceu_coef_gc.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[3], 1 << 18);
	tcon->reg->tcon_ceu_coef_br.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[0], 1 << 12);
	tcon->reg->tcon_ceu_coef_bg.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[1], 1 << 12);
	tcon->reg->tcon_ceu_coef_bb.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[2], 1 << 12);
	tcon->reg->tcon_ceu_coef_bc.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[3], 1 << 18);
	tcon->reg->tcon_ceu_ctl.bits.ceu_en = 1;

	return 0;
}

s32 tcon_tv_black_src(struct sunxi_tcon_tv *tcon, u32 on_off, u32 color)
{
	tcon->reg->tcon_ceu_coef_rr.bits.value = 0x100;
	tcon->reg->tcon_ceu_coef_rg.bits.value = 0;
	tcon->reg->tcon_ceu_coef_rb.bits.value = 0;
	tcon->reg->tcon_ceu_coef_rc.bits.value = 0;

	tcon->reg->tcon_ceu_coef_gr.bits.value = 0;
	tcon->reg->tcon_ceu_coef_gg.bits.value = 0x100;
	tcon->reg->tcon_ceu_coef_gb.bits.value = 0;
	tcon->reg->tcon_ceu_coef_gc.bits.value = 0;

	tcon->reg->tcon_ceu_coef_br.bits.value = 0;
	tcon->reg->tcon_ceu_coef_bg.bits.value = 0;
	tcon->reg->tcon_ceu_coef_bb.bits.value = 0x100;
	tcon->reg->tcon_ceu_coef_bc.bits.value = 0;
	tcon->reg->tcon_ceu_coef_rv.bits.max = (color == 0) ? 0x000 :
						(color == 1) ? 0x040 : 0x200;
	tcon->reg->tcon_ceu_coef_rv.bits.min = (color == 0) ? 0x000 :
						(color == 1) ? 0x040 : 0x200;
	tcon->reg->tcon_ceu_coef_gv.bits.max = (color == 0) ? 0x000 :
						(color == 1) ? 0x200 : 0x040;
	tcon->reg->tcon_ceu_coef_gv.bits.min = (color == 0) ? 0x000 :
						(color == 1) ? 0x200 : 0x040;
	tcon->reg->tcon_ceu_coef_bv.bits.max = (color == 0) ? 0x000 :
						(color == 1) ? 0x200 : 0x040;
	tcon->reg->tcon_ceu_coef_bv.bits.min = (color == 0) ? 0x000 :
						(color == 1) ? 0x200 : 0x040;
	tcon->reg->tcon_ceu_ctl.bits.ceu_en = on_off ? 1 : 0;

	return 0;
}

s32 tcon_tv_volume_force(struct sunxi_tcon_tv *tcon, u32 value)
{
	tcon->reg->tcon_volume_ctl.dwval = value;
	return 0;
}

void tcon_tv_show_builtin_patten(struct sunxi_tcon_tv *tcon, u32 patten)
{
	tcon->reg->tcon_tv_src_ctl.bits.src_sel = patten;
}

void tcon_tv_enable_vblank(struct sunxi_tcon_tv *tcon, bool enable)
{
	if (enable)
		tcon_tv_irq_enable(tcon, LCD_IRQ_TCON1_VBLK);
	else
		tcon_tv_irq_disable(tcon, LCD_IRQ_TCON1_VBLK);
}

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
#include <linux/delay.h>
#include <linux/io.h>
#include "include.h"
#include "tcon_lcd_type.h"
#include "tcon_top.h"
#include "tcon_lcd.h"

static s32 tcon_lcd_frm(struct sunxi_tcon_lcd *tcon, u32 mode);
static s32 tcon_lcd_fsync_pol(struct sunxi_tcon_lcd *tcon, u32 pol);
static s32 tcon_lcd_fsync_active_time(struct sunxi_tcon_lcd *tcon, u32 pixel_num);
/*
static s32 disp_delay_ms(u32 ms)
{
	msleep(ms);
	return 0;
}
*/
static s32 disp_delay_us(u32 us)
{
	udelay(us);
	return 0;
}

s32 tcon_lcd_set_reg_base(struct sunxi_tcon_lcd *tcon, uintptr_t base)
{
	tcon->reg = (struct tcon_lcd_reg *) (uintptr_t) (base);
	return 0;
}

void lvds_1903_open(struct sunxi_tcon_lcd *tcon, struct disp_lvds_para *para)
{
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_en = 1;
	if (para->dual_lvds == 1) {
		tcon->reg->tcon0_lvds_ana[0].bits.c = 6;
		tcon->reg->tcon0_lvds_ana[0].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[0].bits.pd = 2;*/
		tcon->reg->tcon0_lvds_ana[1].bits.c = 6;
		tcon->reg->tcon0_lvds_ana[1].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[1].bits.pd = 2;*/

		tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 0;
		tcon->reg->tcon0_lvds_ana[1].bits.en_ldo = 0;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_mb = 1;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_drvc = 1;
		if (para->lvds_colordepth == LVDS_6bit) {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0x7;
		} else {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0xf;
		}
	} else if (para->dual_lvds == 2) {
		tcon->reg->tcon0_lvds_ana[0].bits.c = 6;
		tcon->reg->tcon0_lvds_ana[0].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[0].bits.pd = 2;*/
		tcon->reg->tcon0_lvds_ana[1].bits.c = 6;
		tcon->reg->tcon0_lvds_ana[1].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[1].bits.pd = 2;*/

		tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_ldo = 1;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_mb = 1;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_drvc = 1;
		if (para->lvds_colordepth == LVDS_6bit) {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0x7;
		} else {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0xf;
		}
	} else {
		if (tcon->tcon_index) {
			tcon->reg->tcon0_lvds_ana[0].bits.c = 6;
			tcon->reg->tcon0_lvds_ana[0].bits.v = 3;
			/*tcon->reg->tcon0_lvds_ana[0].bits.pd = 2;*/

			tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 0;
			/* 1200ns */
			disp_delay_us(5);
			tcon->reg->tcon0_lvds_ana[0].bits.en_24m = 1;
			tcon->reg->tcon0_lvds_ana[0].bits.en_lvds = 1;
			tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 1;
			/* 1200ns */
			disp_delay_us(5);
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 1;
			if (para->lvds_colordepth == LVDS_6bit)
				tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			else
				tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
		}
	}
}

void lvds_1919_open(struct sunxi_tcon_lcd *tcon, struct disp_lvds_para *para)
{
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_en = 1;
	if (para->dual_lvds == 1) {
		tcon->reg->tcon0_lvds_ana[0].bits.c = 5;
		tcon->reg->tcon0_lvds_ana[0].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[0].bits.pd = 2;*/
		tcon->reg->tcon0_lvds_ana[1].bits.c = 5;
		tcon->reg->tcon0_lvds_ana[1].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[1].bits.pd = 2;*/

		tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 0;
		tcon->reg->tcon0_lvds_ana[1].bits.en_ldo = 0;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_mb = 1;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_drvc = 1;
		if (para->lvds_colordepth == LVDS_6bit) {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0x7;
		} else {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0xf;
		}
	} else if (para->dual_lvds == 2) {
		tcon->reg->tcon0_lvds_ana[0].bits.c = 4;
		tcon->reg->tcon0_lvds_ana[0].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[0].bits.pd = 2;*/
		tcon->reg->tcon0_lvds_ana[1].bits.c = 4;
		tcon->reg->tcon0_lvds_ana[1].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[1].bits.pd = 2;*/

		tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_ldo = 1;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_mb = 1;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_drvc = 1;
		if (para->lvds_colordepth == LVDS_6bit) {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0x7;
		} else {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0xf;
		}
	} else {
		if (tcon->tcon_index) {
			tcon->reg->tcon0_lvds_ana[0].bits.c = 4;
			tcon->reg->tcon0_lvds_ana[0].bits.v = 3;
			/*tcon->reg->tcon0_lvds_ana[0].bits.pd = 2;*/

			tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 0;
			/* 1200ns */
			disp_delay_us(5);
			tcon->reg->tcon0_lvds_ana[0].bits.en_24m = 1;
			tcon->reg->tcon0_lvds_ana[0].bits.en_lvds = 1;
			tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 1;
			/* 1200ns */
			disp_delay_us(5);
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 1;
			if (para->lvds_colordepth == LVDS_6bit)
				tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			else
				tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
		}
	}
}

void lvds_default_open(struct sunxi_tcon_lcd *tcon, struct disp_lvds_para *para)
{
		tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_en = 1;
	if (para->dual_lvds == 1) {
		tcon->reg->tcon0_lvds_ana[0].bits.c = 5;
		tcon->reg->tcon0_lvds_ana[0].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[0].bits.pd = 2;*/
		tcon->reg->tcon0_lvds_ana[1].bits.c = 5;
		tcon->reg->tcon0_lvds_ana[1].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[1].bits.pd = 2;*/

		tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 0;
		tcon->reg->tcon0_lvds_ana[1].bits.en_ldo = 0;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_mb = 1;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_drvc = 1;
		if (para->lvds_colordepth == LVDS_6bit) {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0x7;
		} else {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0xf;
		}
	} else if (para->dual_lvds == 2) {
		tcon->reg->tcon0_lvds_ana[0].bits.c = 4;
		tcon->reg->tcon0_lvds_ana[0].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[0].bits.pd = 2;*/
		tcon->reg->tcon0_lvds_ana[1].bits.c = 4;
		tcon->reg->tcon0_lvds_ana[1].bits.v = 3;
		/*tcon->reg->tcon0_lvds_ana[1].bits.pd = 2;*/

		tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_ldo = 1;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_24m = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_lvds = 1;
		tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_mb = 1;
		/* 1200ns */
		disp_delay_us(5);
		tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 1;
		tcon->reg->tcon0_lvds_ana[1].bits.en_drvc = 1;
		if (para->lvds_colordepth == LVDS_6bit) {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0x7;
		} else {
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
			tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0xf;
		}
	} else {
		if (tcon->tcon_index) {
			tcon->reg->tcon0_lvds_ana[0].bits.c = 4;
			tcon->reg->tcon0_lvds_ana[0].bits.v = 3;
			/*tcon->reg->tcon0_lvds_ana[0].bits.pd = 2;*/

			tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 0;
			/* 1200ns */
			disp_delay_us(5);
			tcon->reg->tcon0_lvds_ana[0].bits.en_24m = 1;
			tcon->reg->tcon0_lvds_ana[0].bits.en_lvds = 1;
			tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 1;
			/* 1200ns */
			disp_delay_us(5);
			tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 1;
			if (para->lvds_colordepth == LVDS_6bit)
				tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			else
				tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
		}
	}
}

s32 lvds_open(struct sunxi_tcon_lcd *tcon, struct disp_lvds_para *para)
{
#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	lvds_1903_open(tcon, para);
#elif IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	lvds_1919_open(tcon, para);
#else
	lvds_default_open(tcon, para);
#endif

	return 0;
}

s32 lvds_close(struct sunxi_tcon_lcd *tcon)
{
	tcon->reg->tcon0_lvds_ana[0].bits.en_drvd = 0;
	tcon->reg->tcon0_lvds_ana[1].bits.en_drvd = 0;
	tcon->reg->tcon0_lvds_ana[0].bits.en_drvc = 0;
	tcon->reg->tcon0_lvds_ana[1].bits.en_drvc = 0;
	/* 1200ns */
	disp_delay_us(5);
	tcon->reg->tcon0_lvds_ana[0].bits.en_mb = 0;
	tcon->reg->tcon0_lvds_ana[1].bits.en_mb = 0;
	/* 1200ns */
	disp_delay_us(5);
	tcon->reg->tcon0_lvds_ana[0].bits.en_ldo = 0;
	tcon->reg->tcon0_lvds_ana[1].bits.en_ldo = 0;
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_en = 0;
	return 0;
}

s32 tcon_lcd_get_timing(struct sunxi_tcon_lcd *tcon, struct disp_video_timings *tt)
{
	u32 x, y, ht, hbp, vt, vbp, hspw, vspw;
	u32 lcd_if = 0, lcd_hv_if = 0;

	lcd_if = tcon->reg->tcon0_ctl.bits.tcon0_if;
	lcd_hv_if = tcon->reg->tcon0_hv_ctl.bits.hv_mode;
	x = tcon->reg->tcon0_basic0.bits.x;
	y = tcon->reg->tcon0_basic0.bits.y;
	ht = tcon->reg->tcon0_basic1.bits.ht;
	hbp = tcon->reg->tcon0_basic1.bits.hbp;
	vt = tcon->reg->tcon0_basic2.bits.vt;
	vbp = tcon->reg->tcon0_basic2.bits.vbp;
	hspw = tcon->reg->tcon0_basic3.bits.hspw;
	vspw = tcon->reg->tcon0_basic3.bits.vspw;

	tt->x_res = x;
	tt->hor_back_porch = (hbp + 1) - (hspw + 1);
	tt->hor_front_porch = (ht + 1) - (x + 1) - (hbp + 1);
	tt->hor_total_time = ht + 1;
	tt->y_res = y;
	tt->ver_back_porch = (vbp + 1) - (vspw + 1);
	tt->ver_front_porch = (vt / 2) - (y + 1) - (vbp + 1);
	tt->hor_sync_time = (hspw + 1);
	tt->ver_sync_time = (vspw + 1);
	tt->ver_total_time = (vt / 2);
/*
	if ((lcd_if == LCD_IF_HV) && (lcd_hv_if == HV_MODE_CCIR656_2CYC)) {
		tt->ver_total_time = vt;
		tt->hor_total_time = (ht + 1) / 2;
		tt->hor_back_porch = (hbp + 1) / 2;
		tt->hor_sync_time = (hspw + 1) / 2;
		tt->y_res = (y + 1) * 2;
	}
*/
	return 0;
}

s32 tcon_lcd_init(struct sunxi_tcon_lcd *tcon)
{
	tcon->reg->tcon0_ctl.bits.tcon0_en = 0;
	tcon->reg->tcon_gctl.bits.tcon_en = 0;
	tcon->reg->tcon_gint0.bits.tcon_irq_en = 0;
	tcon->reg->tcon_gint0.bits.tcon_irq_flag = 0;
	tcon->reg->tcon_gctl.bits.tcon_en = 1;

	return 0;
}

s32 tcon_lcd_exit(struct sunxi_tcon_lcd *tcon)
{
	tcon->reg->tcon_gctl.bits.tcon_en = 0;
	tcon->reg->tcon0_dclk.bits.tcon0_dclk_en = 0;

	return 0;
}

static s32 tcon_lcd_irq_enable(struct sunxi_tcon_lcd *tcon, enum __lcd_irq_id_t id)
{
	tcon->reg->tcon_gint0.bits.tcon_irq_en |= (1 << id);

	return 0;
}

static s32 tcon_lcd_irq_disable(struct sunxi_tcon_lcd *tcon, enum __lcd_irq_id_t id)
{
	tcon->reg->tcon_gint0.bits.tcon_irq_en &= ~(1 << id);

	return 0;
}

u32 tcon_lcd_irq_query(struct sunxi_tcon_lcd *tcon, enum __lcd_irq_id_t id)
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

u32 tcon_lcd_get_start_delay(struct sunxi_tcon_lcd *tcon)
{
	return tcon->reg->tcon0_ctl.bits.start_delay;
}

u32 tcon_lcd_get_cur_line(struct sunxi_tcon_lcd *tcon)
{
	return tcon->reg->tcon_debug.bits.tcon0_current_line;
}

/* 0: normal; -1:under flow; */
s32 tcon_lcd_get_status(struct sunxi_tcon_lcd *tcon)
{
	if (tcon->reg->tcon_debug.bits.tcon0_fifo_under_flow) {
		tcon->reg->tcon_debug.bits.tcon0_fifo_under_flow = 0;
		return -1;
	}
	return 0;
}

s32 tcon_lcd_src_select(struct sunxi_tcon_lcd *tcon, enum __lcd_src_t src)
{
	tcon->reg->tcon0_ctl.bits.src_sel = src;

	return 0;
}

s32 tcon_lcd_src_get(struct sunxi_tcon_lcd *tcon)
{
	return tcon->reg->tcon0_ctl.bits.src_sel;
}

s32 tcon_dsi_open(struct sunxi_tcon_lcd *tcon, struct disp_dsi_para *dsi_para)
{
	tcon->reg->tcon_gint0.bits.tcon_irq_flag &= 0x00000004;
//	tcon->reg->tcon0_ctl.bits.tcon0_en = 1;
	if (dsi_para->mode_flags & MIPI_DSI_MODE_COMMAND)
		tcon_lcd_irq_enable(tcon, LCD_IRQ_TCON0_CNTR);
	else if (dsi_para->mode_flags & MIPI_DSI_MODE_VIDEO
		 && dsi_para->dual_dsi) {
	//	tcon_lcd_irq_enable(tcon, LCD_IRQ_TCON0_VBLK);
		tcon->reg->tcon0_dclk.bits.tcon0_dclk_en = 0x2;
	} else
		tcon->reg->tcon0_dclk.bits.tcon0_dclk_en = 0xf;

	tcon->reg->tcon0_ctl.bits.tcon0_en = 1;
	msleep(100);

	return 0;
}

s32 tcon_dsi_close(struct sunxi_tcon_lcd *tcon)
{
	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_CNTR);
//	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_VBLK);
	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_TRIF);
	tcon->reg->tcon0_ctl.bits.tcon0_en = 0;
	tcon->reg->tcon0_dclk.bits.tcon0_dclk_en = 0x0;
	tcon_lcd_dsi_clk_enable(tcon->tcon_index, 0);
	if (tcon->reg->tcon_sync_ctl.bits.dsi_num &&
	    tcon->tcon_index + 1 < DEVICE_DSI_NUM)
		tcon_lcd_dsi_clk_enable(tcon->tcon_index, 0);

	return 1;
}
s32 tcon_lvds_open(struct sunxi_tcon_lcd *tcon)
{
	tcon->reg->tcon_gint0.bits.tcon_irq_flag &= 0x00000004;
	tcon->reg->tcon0_dclk.bits.tcon0_dclk_en = 0xf;
	tcon->reg->tcon0_ctl.bits.tcon0_en = 1;
//	tcon_lcd_irq_enable(tcon, LCD_IRQ_TCON0_VBLK);


	return 0;
}

s32 tcon_lvds_close(struct sunxi_tcon_lcd *tcon)
{
	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_CNTR);
//	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_VBLK);
	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_TRIF);
	tcon->reg->tcon0_ctl.bits.tcon0_en = 0;
	tcon->reg->tcon0_dclk.bits.tcon0_dclk_en = 0x0;

	return 1;
}

s32 tcon_rgb_open(struct sunxi_tcon_lcd *tcon)
{
	tcon->reg->tcon_gint0.bits.tcon_irq_flag &= 0x00000004;
	tcon->reg->tcon0_dclk.bits.tcon0_dclk_en = 0xf;
	tcon->reg->tcon0_ctl.bits.tcon0_en = 1;
//	tcon_lcd_irq_enable(tcon, LCD_IRQ_TCON0_VBLK);

	return 0;
}

s32 tcon_rgb_close(struct sunxi_tcon_lcd *tcon)
{
	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_CNTR);
//	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_VBLK);
	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_TRIF);
	tcon->reg->tcon0_ctl.bits.tcon0_en = 0;
	tcon->reg->tcon0_dclk.bits.tcon0_dclk_en = 0x0;

	return 1;
}
s32 tcon_cpu_open(struct sunxi_tcon_lcd *tcon)
{
	tcon->reg->tcon0_ctl.bits.tcon0_en = 1;
	tcon_lcd_irq_enable(tcon, LCD_IRQ_TCON0_CNTR);

	return 0;
}

s32 tcon_cpu_close(struct sunxi_tcon_lcd *tcon)
{
	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_CNTR);
//	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_VBLK);
	tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_TRIF);
	tcon->reg->tcon0_ctl.bits.tcon0_en = 0;
	tcon->reg->tcon0_dclk.bits.tcon0_dclk_en = 0x0;

	return 1;
}

static s32 tcon_lcd_timing(struct sunxi_tcon_lcd *tcon, struct disp_video_timings *timings)
{
	tcon->reg->tcon0_basic0.bits.x = timings->x_res - 1;
	tcon->reg->tcon0_basic0.bits.y = timings->y_res - 1;
	tcon->reg->tcon0_basic1.bits.ht = timings->hor_total_time - 1;
	tcon->reg->tcon0_basic1.bits.hbp =
		timings->hor_back_porch ? (timings->hor_back_porch +
				timings->hor_sync_time) - 1 : 0;
	tcon->reg->tcon0_basic2.bits.vt = timings->ver_total_time * 2;
	tcon->reg->tcon0_basic2.bits.vbp =
		timings->ver_back_porch ? (timings->ver_back_porch +
				timings->ver_sync_time) - 1 : 0;
	tcon->reg->tcon0_basic3.bits.hspw =
		timings->hor_sync_time ? timings->hor_sync_time - 1 : 0;
	tcon->reg->tcon0_basic3.bits.vspw =
		timings->ver_sync_time ? timings->ver_sync_time - 1 : 0;

	tcon->reg->tcon0_ctl.bits.start_delay = 2;

	return 0;
}

static s32 tcon_rgb_cfg_mode_auto(struct sunxi_tcon_lcd *tcon, struct disp_rgb_para *para)
{
	tcon->reg->tcon0_basic0.bits.x = para->timings.x_res - 1;
	tcon->reg->tcon0_basic0.bits.y = para->timings.y_res - 1;
	tcon->reg->tcon0_basic1.bits.ht = para->timings.hor_total_time - 1;
	tcon->reg->tcon0_basic1.bits.hbp =
	    para->timings.hor_back_porch ? (para->timings.hor_back_porch +
			    para->timings.hor_sync_time) - 1 : 0;
	tcon->reg->tcon0_basic2.bits.vt = para->timings.ver_total_time * 2;
	tcon->reg->tcon0_basic2.bits.vbp =
	    para->timings.ver_back_porch ? (para->timings.ver_back_porch +
			    para->timings.ver_sync_time) - 1 : 0;
	tcon->reg->tcon0_basic3.bits.hspw =
	    para->timings.hor_sync_time ? para->timings.hor_sync_time - 1 : 0;
	tcon->reg->tcon0_basic3.bits.vspw =
	    para->timings.ver_sync_time ? para->timings.ver_sync_time - 1 : 0;

	if (para->hv_mode == HV_MODE_CCIR656_2CYC) {
		if (para->lcd_interlace) {
			tcon->reg->tcon0_basic0.bits.y =
				para->timings.y_res / 2 - 1;
			tcon->reg->tcon0_basic2.bits.vt =
				(para->hv_syuv_fdly ==
				HV_SRGB_FDLY_2LINE) ? 625 : 525;
		} else {
			tcon->reg->tcon0_basic0.bits.y = para->timings.y_res - 1;
			tcon->reg->tcon0_basic2.bits.vt =
				(para->hv_syuv_fdly ==
				HV_SRGB_FDLY_2LINE) ? 1250 : 1050;
		}

		tcon->reg->tcon0_basic1.bits.ht =
			(para->timings.hor_total_time == 0) ? 0 : (para->timings.hor_total_time * 2 - 1);
		tcon->reg->tcon0_basic1.bits.hbp =
			(para->timings.hor_back_porch + para->timings.hor_sync_time) ? 0 :
			((para->timings.hor_back_porch + para->timings.hor_sync_time) * 2 - 1);
		tcon->reg->tcon0_basic3.bits.hspw =
			(para->timings.ver_back_porch + para->timings.ver_sync_time) ? 0 :
			((para->timings.ver_back_porch + para->timings.ver_sync_time) * 2 - 1);
	}

	tcon->reg->tcon0_ctl.bits.start_delay = 2;

	return 0;
}
static s32 tcon_lcd_cfg_tri(struct sunxi_tcon_lcd *tcon, struct disp_dsi_para *para)
{
	u32 start_delay = 0;
	u32 delay_line = 0;

	tcon->reg->tcon0_basic0.bits.x = para->timings.x_res - 1;
	tcon->reg->tcon0_basic0.bits.y = para->timings.y_res - 1;
	tcon->reg->tcon0_cpu_tri0.bits.block_size = para->timings.x_res - 1;
	tcon->reg->tcon0_cpu_tri1.bits.block_num = para->timings.y_res - 1;
	tcon->reg->tcon0_cpu_tri2.bits.trans_start_mode = 0;
	tcon->reg->tcon0_cpu_tri2.bits.sync_mode = 0;

	/**
	 * When the blanking area of LCD is too small, the following formula is
	 * not applicable, that calculates the start_ Delay is too large.
	 *
	 * The formula is
	 *	start_delay = (para->lcd_vt - para->lcd_y - 8 - 1)
	 *		* para->lcd_ht * de_clk_rate / para->lcd_dclk_freq / 8;
	 *
	 * Therefore, the following formula is obtained by balancing the
	 * requirements of DE, TCON and PANEL modules for pixel data speed
	 *
	 */
	if (para->timings.ver_back_porch > 3)
		delay_line = 3;
	else if (para->timings.ver_back_porch <= 3) {
		delay_line = para->timings.ver_back_porch;
		WARN(1, "vbp is too small, please readjust the timing parameters to increase vbp.\n");
	}

	start_delay = delay_line * para->timings.hor_total_time / 8 - 1;

	tcon->reg->tcon0_cpu_tri2.bits.start_delay = start_delay;

	tcon->reg->tcon0_cpu_ctl.bits.trigger_fifo_en = 1;
	tcon->reg->tcon0_cpu_ctl.bits.trigger_en = 1;
	tcon->reg->tcon0_cpu_ctl.bits.flush = 1;
	tcon->reg->tcon0_ctl.bits.tcon0_en = 1;
	tcon->reg->tcon_gctl.bits.tcon_en = 1;

	tcon->reg->tcon0_cpu_tri0.bits.block_space =
		    para->timings.hor_total_time * 24 /
		    (para->dsi_div * 4) - para->timings.x_res - 40;
	tcon->reg->tcon0_cpu_tri2.bits.trans_start_set = 10;

	return 0;
}

static void tcon_lcd_3d_fifo(struct sunxi_tcon_lcd *tcon, u32 x_res)
{
	tcon->reg->tcon0_3d_fifo.bits.fifo_3d_setting = 2;
	tcon->reg->tcon0_3d_fifo.bits.fifo_3d_half_line_size = x_res / 2 - 1;
}

static void tcon_lcd_rgb_if(struct sunxi_tcon_lcd *tcon, u32 hv_mode)
{
	tcon->reg->tcon0_ctl.bits.tcon0_if = 0;
	tcon->reg->tcon0_ctl.bits.start_delay = 2;
	tcon->reg->tcon0_hv_ctl.bits.hv_mode = hv_mode;
}

s32 tcon_dsi_cfg(struct sunxi_tcon_lcd *tcon, struct disp_dsi_para *para)
{
	if (para->mode_flags & MIPI_DSI_EN_3DFIFO)
		tcon_lcd_3d_fifo(tcon, para->timings.x_res);

	tcon_lcd_timing(tcon, &para->timings);
	if (para->dual_dsi) {
		tcon_lcd_rgb_if(tcon, 0);
		tcon->reg->tcon_sync_ctl.bits.dsi_num = para->dual_dsi;
		tcon->reg->tcon0_hv_ctl.bits.ccir_csc_dis = 1;

		tcon_lcd_dsi_src_sel(0, 0);
		tcon_lcd_dsi_src_sel(1, 1);
		tcon_lcd_dsi_clk_enable(0, 1);

	} else {
		if (para->mode_flags & MIPI_DSI_SLAVE_MODE)
			tcon_lcd_rgb_if(tcon, 0);
		else {
			tcon->reg->tcon0_ctl.bits.tcon0_if = 1;
			tcon->reg->tcon0_cpu_ctl.bits.cpu_mode = MODE_DSI;
			tcon_lcd_cfg_tri(tcon, para);
		}
		tcon_lcd_dsi_clk_enable(tcon->tcon_index, 1);
	}

/*
	tcon_lcd_frm(sel, para->lcd_frm);
*/
	/* fsync config */
/*
	tcon->reg->fsync_gen_ctrl.bits.fsync_gen_en = para->fsync_en;
	tcon_lcd_fsync_pol(sel, para->fsync_pol);


	if (para->lcd_fsync_act_time) {
		tcon_lcd_fsync_active_time(sel, para->lcd_fsync_act_time);
	} else {
		tcon->reg->fsync_gen_dly.bits.sensor_act0_time =
		    para->lcd_ht / 4;
		tcon->reg->fsync_gen_dly.bits.sensor_act1_time =
		    para->lcd_ht / 4;
	}

	tcon->reg->fsync_gen_ctrl.bits.sensor_dis_time =
	    (para->lcd_fsync_dis_time) ? para->lcd_fsync_dis_time : 1;
*/
	tcon->reg->tcon0_ctl.bits.rb_swap = 0; //para->lcd_rb_swap;
	tcon->reg->tcon0_io_tri.bits.rgb_endian = 0; //para->lcd_rgb_endian;
	tcon->reg->tcon_volume_ctl.bits.safe_period_mode = 3;
	tcon->reg->tcon_volume_ctl.bits.safe_period_fifo_num =
	    para->timings.pixel_clk * 15 / 1000000;

	tcon->reg->tcon0_io_tri.bits.io0_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io1_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io2_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io3_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.data_output_tri_en = 0;

	return 0;
}

s32 tcon_lvds_cfg(struct sunxi_tcon_lcd *tcon, struct disp_lvds_para *para)
{
	tcon->reg->tcon0_ctl.bits.tcon0_if = 0;
	tcon->reg->tcon0_hv_ctl.bits.hv_mode = 0;
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_link = para->dual_lvds == 1 ? 1 : 0;
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_bitwidth =
	    para->lvds_colordepth;
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_mode =
	    para->lvds_data_mode;
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_debug_en = 0;
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_correct_mode = 0;
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_dir = 0;
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_clk_sel = 1;
#if defined(LVDS_REVERT)
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_data_revert = 0xf;
	tcon->reg->tcon0_lvds_ctl.bits.tcon0_lvds_clk_revert = 0x1;
#endif
	tcon_lcd_timing(tcon, &para->timings);

	tcon_lcd_frm(tcon, para->lcd_frm);

	/* fsync config */
	tcon->reg->fsync_gen_ctrl.bits.fsync_gen_en = para->fsync_en;
//	tcon_lcd_fsync_pol(sel, para->fsync_pol);

/*
	if (para->fsync_act_time) {
		tcon_lcd_fsync_active_time(sel, para->fsync_act_time);
	} else {
		tcon->reg->fsync_gen_dly.bits.sensor_act0_time =
		    para->timings.hor_total_time / 4;
		tcon->reg->fsync_gen_dly.bits.sensor_act1_time =
		    para->timings.hor_total_time / 4;
	}
	*/
/*
	tcon->reg->fsync_gen_ctrl.bits.sensor_dis_time =
	    (para->fsync_dis_time) ? para->fsync_dis_time : 1;
*/
	tcon->reg->tcon0_ctl.bits.rb_swap = para->rb_swap;
	tcon->reg->tcon0_io_tri.bits.rgb_endian = para->rgb_endian;
	tcon->reg->tcon_volume_ctl.bits.safe_period_mode = 3;
	tcon->reg->tcon_volume_ctl.bits.safe_period_fifo_num =
		para->timings.pixel_clk * 15 / 1000000;
	tcon->reg->tcon0_io_pol.bits.sync_inv = 0;
	tcon->reg->fsync_gen_ctrl.bits.hsync_pol_sel = 0;

	tcon->reg->tcon0_io_tri.bits.io0_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io1_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io2_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io3_output_tri_en = 0;

	tcon->reg->tcon0_io_tri.bits.data_output_tri_en = 0;

	return 0;
}

s32 tcon_rgb_cfg(struct sunxi_tcon_lcd *tcon, struct disp_rgb_para *para)
{
	tcon->reg->tcon0_ctl.bits.tcon0_if = 0;
	tcon->reg->tcon0_hv_ctl.bits.hv_mode = para->hv_mode;
	tcon->reg->tcon0_hv_ctl.bits.srgb_seq =
	    para->hv_srgb_seq;
	tcon->reg->tcon0_hv_ctl.bits.syuv_seq =
	    para->hv_syuv_seq;
	tcon->reg->tcon0_hv_ctl.bits.syuv_fdly =
	    para->hv_syuv_fdly;
	tcon->reg->tcon0_hv_ctl.bits.ccir_csc_dis = para->input_csc;
	tcon_rgb_cfg_mode_auto(tcon, para);
	tcon_lcd_rgb_src_sel(tcon->tcon_index);

	tcon_lcd_frm(tcon, para->lcd_frm);

	tcon->reg->fsync_gen_ctrl.bits.fsync_gen_en = para->fsync_en;
	tcon_lcd_fsync_pol(tcon, para->fsync_pol);


	if (para->fsync_act_time) {
		tcon_lcd_fsync_active_time(tcon, para->fsync_act_time);
	} else {
		tcon->reg->fsync_gen_dly.bits.sensor_act0_time =
		    para->timings.hor_total_time / 4;
		tcon->reg->fsync_gen_dly.bits.sensor_act1_time =
		    para->timings.hor_total_time / 4;
	}

	tcon->reg->fsync_gen_ctrl.bits.sensor_dis_time =
	    (para->fsync_dis_time) ? para->fsync_dis_time : 1;

	tcon->reg->tcon0_ctl.bits.rb_swap = para->rb_swap;
	tcon->reg->tcon0_io_tri.bits.rgb_endian = para->rgb_endian;
	tcon->reg->tcon_volume_ctl.bits.safe_period_mode = 3;
	tcon->reg->tcon_volume_ctl.bits.safe_period_fifo_num =
	    para->timings.pixel_clk * 15 / 1000000;
	tcon->reg->tcon0_io_pol.bits.sync_inv = para->hv_sync_polarity;
	tcon->reg->fsync_gen_ctrl.bits.hsync_pol_sel =
	    !para->hv_sync_polarity;
	switch (para->hv_clk_phase) {
	case 0:
		tcon->reg->tcon0_io_pol.bits.clk_inv = 0;
		tcon->reg->tcon0_io_pol.bits.dclk_sel = 0;
		break;
	case 1:
		tcon->reg->tcon0_io_pol.bits.clk_inv = 0;
		tcon->reg->tcon0_io_pol.bits.dclk_sel = 2;
		break;
	case 2:
		tcon->reg->tcon0_io_pol.bits.clk_inv = 1;
		tcon->reg->tcon0_io_pol.bits.dclk_sel = 0;
		break;
	case 3:
		tcon->reg->tcon0_io_pol.bits.clk_inv = 1;
		tcon->reg->tcon0_io_pol.bits.dclk_sel = 2;
		break;
	default:
		tcon->reg->tcon0_io_pol.bits.clk_inv = 0;
		tcon->reg->tcon0_io_pol.bits.dclk_sel = 0;
		break;
	}

	tcon->reg->tcon0_io_tri.bits.io0_output_tri_en = 0;

	tcon->reg->tcon0_io_tri.bits.io1_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io2_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io3_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.data_output_tri_en = 0;

	return 0;
}

#ifdef DEBUG
s32 tcon_cpu_cfg(struct sunxi_tcon_lcd *tcon, struct disp_panel_para *para)
{
	tcon->reg->tcon0_ctl.bits.tcon0_if = 1;
	tcon->reg->tcon0_cpu_ctl.bits.cpu_mode = para->lcd_cpu_if;
	/*
	 * why ?
	 * tcon->reg->tcon0_cpu_ctl.bits.da = 1;
	 */
/*	if (para->lcd_cpu_mode == 0)
		tcon_lcd_timing(sel, &para->timings);
	else
		tcon_lcd_cfg_tri(sel, para); */
	tcon->reg->tcon0_cpu_tri4.bits.en = 0;

	tcon_lcd_frm(tcon, para->lcd_frm);

	/* fsync config */
	tcon->reg->fsync_gen_ctrl.bits.fsync_gen_en = para->fsync_en;
	tcon_lcd_fsync_pol(tcon, para->fsync_pol);


	if (para->lcd_fsync_act_time) {
		tcon_lcd_fsync_active_time(tcon, para->fsync_act_time);
	} else {
		tcon->reg->fsync_gen_dly.bits.sensor_act0_time =
		    para->lcd_ht / 4;
		tcon->reg->fsync_gen_dly.bits.sensor_act1_time =
		    para->lcd_ht / 4;
	}

	tcon->reg->fsync_gen_ctrl.bits.sensor_dis_time =
	    (para->lcd_fsync_dis_time) ? para->lcd_fsync_dis_time : 1;

	tcon->reg->tcon0_ctl.bits.rb_swap = para->rb_swap;
	tcon->reg->tcon0_io_tri.bits.rgb_endian = para->lcd_rgb_endian;
	tcon->reg->tcon_volume_ctl.bits.safe_period_mode = 3;
	tcon->reg->tcon_volume_ctl.bits.safe_period_fifo_num =
	    para->lcd_dclk_freq * 15;
	tcon->reg->tcon0_io_pol.bits.sync_inv = para->lcd_hv_sync_polarity;
	tcon->reg->fsync_gen_ctrl.bits.hsync_pol_sel =
	    !para->lcd_hv_sync_polarity;

	tcon->reg->tcon0_io_tri.bits.io0_output_tri_en = 0; // lcd_cpu_te;

	tcon->reg->tcon0_io_tri.bits.io1_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io2_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.io3_output_tri_en = 0;
	tcon->reg->tcon0_io_tri.bits.data_output_tri_en = 0;

	return 0;
}
#endif
/*
s32 tcon0_cfg_ext(struct sunxi_tcon_lcd *tcon, struct para_extend_para *extend_para)
{
	de_gamma_set_table(extend_para->mgr_id, extend_para->lcd_gamma_en,
		   extend_para->is_lcd_gamma_tbl_10bit, extend_para->lcd_gamma_tbl);
	tcon_cmap(sel, extend_para->lcd_cmap_en, extend_para->lcd_cmap_tbl);

	return 0;
}
*/
/*
s32 tcon0_tri_busy(struct sunxi_tcon_lcd *tcon)
{
	return tcon->reg->tcon0_cpu_ctl.bits.trigger_start;
}

s32 tcon0_tri_start(struct sunxi_tcon_lcd *tcon)
{
	tcon->reg->tcon0_cpu_ctl.bits.trigger_start = 1;
	return 0;
}
*/
static s32 tcon_lcd_frm(struct sunxi_tcon_lcd *tcon, u32 mode)
{
	if (mode == LCD_FRM_BYPASS) {
		tcon->reg->tcon_frm_ctl.bits.tcon0_frm_en = 0;
		return 0;
	}
	tcon->reg->tcon_frm_seed_pr.bits.seed_value = 1;
	tcon->reg->tcon_frm_seed_pg.bits.seed_value = 3;
	tcon->reg->tcon_frm_seed_pb.bits.seed_value = 5;
	tcon->reg->tcon_frm_seed_lr.bits.seed_value = 7;
	tcon->reg->tcon_frm_seed_lg.bits.seed_value = 11;
	tcon->reg->tcon_frm_seed_lb.bits.seed_value = 13;
	tcon->reg->tcon_frm_tbl_0.bits.frm_table_value = 0x01010000;
	tcon->reg->tcon_frm_tbl_1.bits.frm_table_value = 0x15151111;
	tcon->reg->tcon_frm_tbl_2.bits.frm_table_value = 0x57575555;
	tcon->reg->tcon_frm_tbl_3.bits.frm_table_value = 0x7f7f7777;
	if (mode == LCD_FRM_RGB666) {
		tcon->reg->tcon_frm_ctl.bits.tcon0_frm_mode_r = 0;
		tcon->reg->tcon_frm_ctl.bits.tcon0_frm_mode_g = 0;
		tcon->reg->tcon_frm_ctl.bits.tcon0_frm_mode_b = 0;
	} else if (mode == LCD_FRM_RGB565) {
		tcon->reg->tcon_frm_ctl.bits.tcon0_frm_mode_r = 1;
		tcon->reg->tcon_frm_ctl.bits.tcon0_frm_mode_g = 0;
		tcon->reg->tcon_frm_ctl.bits.tcon0_frm_mode_b = 1;
	}
	tcon->reg->tcon_frm_ctl.bits.tcon0_frm_en = 1;
	return 0;
}

s32 tcon_lcd_set_dclk_div(struct sunxi_tcon_lcd *tcon, u8 div)
{
	tcon->reg->tcon0_dclk.bits.tcon0_dclk_div = div;

	return 0;
}
/*
u32 tcon_lcd_get_dclk_div(struct sunxi_tcon_lcd *tcon)
{
	return tcon->reg->tcon0_dclk.bits.tcon0_dclk_div;
}
*/

u32 tcon0_get_cpu_tri2_start_delay(struct sunxi_tcon_lcd *tcon)
{
	return tcon->reg->tcon0_cpu_tri2.bits.start_delay;
}

void tcon_lcd_show_builtin_patten(struct sunxi_tcon_lcd *tcon, u32 patten)
{
	tcon->reg->tcon0_ctl.bits.src_sel = patten;
}

/**
 * @name       :tcon_lcd_fsync_pol
 * @brief      :set fsync's polarity
 * @param[IN]  :sel:tcon index
 * @param[IN]  :pol:polarity. 1:positive;0:negative
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

static s32 tcon_lcd_fsync_pol(struct sunxi_tcon_lcd *tcon, u32 pol)
{
	if (pol) {
		tcon->reg->tcon_gint0.bits.tcon_irq_flag |= 0x00000004;
		tcon->reg->fsync_gen_ctrl.bits.sensor_dis_value = 0;
		tcon->reg->fsync_gen_ctrl.bits.sensor_act0_value = 1;
		tcon->reg->fsync_gen_ctrl.bits.sensor_act1_value = 1;
	} else {
		tcon->reg->tcon_gint0.bits.tcon_irq_flag &= 0xfffffffb;
		tcon->reg->fsync_gen_ctrl.bits.sensor_dis_value = 1;
		tcon->reg->fsync_gen_ctrl.bits.sensor_act0_value = 0;
		tcon->reg->fsync_gen_ctrl.bits.sensor_act1_value = 0;
	}
	return 0;
}

/**
 * @name       :tcon_lcd_fsync_active_time
 * @brief      :set tcon fsync's active time
 * @param[IN]  :sel:tcon index
 * @param[IN]  :pixel_num:number of pixel time(Tpixel) to set
 *
 * Tpixel = 1/fps*1e9/vt/ht, unit:ns
 *
 * @return     :0 if success
 */

static s32 tcon_lcd_fsync_active_time(struct sunxi_tcon_lcd *tcon, u32 pixel_num)
{
	// 4095*2
	if (pixel_num > 8190 || pixel_num <= 0)
		return -1;

	if (pixel_num > 4095) {
		tcon->reg->fsync_gen_dly.bits.sensor_act0_time = 4095;
		tcon->reg->fsync_gen_dly.bits.sensor_act1_time =
		    pixel_num - 4095;
	} else {
		tcon->reg->fsync_gen_dly.bits.sensor_act0_time =
		    pixel_num / 2;
		tcon->reg->fsync_gen_dly.bits.sensor_act1_time =
		    pixel_num - pixel_num / 2;
	}

	return 0;
}

int sunxi_tcon_updata_vt(struct sunxi_tcon_lcd *tcon, struct disp_video_timings *timings,
				u32 vrr_setp)
{
	u32 vt_value;
	struct disp_video_timings timing_t;
	u32 step = vrr_setp ? vrr_setp : 30;
	u32 vbp_bit_h, bit_num = 0;
	u32 vbp = tcon->reg->tcon0_basic2.bits.vbp;

	tcon_lcd_get_timing(tcon, &timing_t);
	if (timings->ver_total_time > timing_t.ver_total_time) {
		if (timings->ver_back_porch <= timing_t.ver_back_porch + step)
			vbp_bit_h = timings->ver_back_porch + timing_t.ver_sync_time - 1;
		else
			vbp_bit_h = vbp + step;

		while (vbp_bit_h) {
			vbp_bit_h >>= 1;
			bit_num++;
		}
		bit_num--;
		vt_value = timing_t.ver_total_time + step;
		if (timings->ver_total_time <= vt_value) {
			tcon->reg->tcon0_basic2.bits.vt = timings->ver_total_time * 2;
			tcon->reg->tcon0_basic2.bits.vbp = (1 << bit_num) | vbp;
			tcon->reg->tcon0_basic2.bits.vbp = timings->ver_back_porch + timing_t.ver_sync_time - 1;
			tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_LINE);

			return 1;
		} else {
			tcon->reg->tcon0_basic2.bits.vt = vt_value * 2;
			tcon->reg->tcon0_basic2.bits.vbp = (1 << bit_num) | vbp;
			tcon->reg->tcon0_basic2.bits.vbp = vbp + step;
		}
	} else {
		if (timings->ver_back_porch >= timing_t.ver_back_porch - step)
			vbp_bit_h = timings->ver_back_porch + timing_t.ver_sync_time - 1;
		else
			vbp_bit_h = vbp - step;
		while (vbp_bit_h) {
			vbp_bit_h >>= 1;
			bit_num++;
		}
		bit_num--;
		vt_value = timing_t.ver_total_time - step;
		if (timings->ver_total_time >= vt_value) {
			tcon->reg->tcon0_basic2.bits.vt = timings->ver_total_time * 2;
			tcon->reg->tcon0_basic2.bits.vbp = (1 << bit_num) | vbp;
			tcon->reg->tcon0_basic2.bits.vbp = timings->ver_back_porch + timing_t.ver_sync_time - 1;
			tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_LINE);

			return 1;
		} else {
			tcon->reg->tcon0_basic2.bits.vt = vt_value * 2;
			tcon->reg->tcon0_basic2.bits.vbp = (1 << bit_num) | vbp;
			tcon->reg->tcon0_basic2.bits.vbp = vbp - step;
		}
	}

	return 0;
}

int sunxi_tcon_updata_vt_2(struct sunxi_tcon_lcd *tcon, struct disp_video_timings *timings)
{
	struct disp_video_timings timing_t;
	static u32 vrr_flag = 1;

	tcon_lcd_get_timing(tcon, &timing_t);
	if (timings->ver_total_time == timing_t.ver_total_time)
		return 0;

	if (timings->ver_total_time > timing_t.ver_total_time) {
		if (vrr_flag == 1) {
			tcon->reg->tcon0_basic2.bits.vt = (timing_t.ver_total_time + 1024) * 2;
			vrr_flag = 2;
		} else {
			tcon->reg->tcon0_basic2.bits.vt = timings->ver_total_time * 2;
			vrr_flag = 1;
		}
	} else {
		if (vrr_flag == 1) {
			tcon->reg->tcon0_basic2.bits.vt = (timings->ver_total_time + 1024) * 2;
			vrr_flag = 2;
		} else {
			tcon->reg->tcon0_basic2.bits.vt = timings->ver_total_time * 2;
			vrr_flag = 1;
		}
	}

	return 0;
}

bool tcon_lcd_vrr_irq(struct sunxi_tcon_lcd *tcon, bool enable)
{
	struct disp_video_timings timing_t;

	tcon_lcd_get_timing(tcon, &timing_t);
	if (enable) {
		tcon->reg->tcon_gint1.bits.tcon0_line_int_num =
					timing_t.ver_front_porch + 10;
		tcon_lcd_irq_enable(tcon, LCD_IRQ_TCON0_LINE);
	}

	return false;
}

void tcon_lcd_enable_vblank(struct sunxi_tcon_lcd *tcon, bool enable)
{
	if (enable)
		tcon_lcd_irq_enable(tcon, LCD_IRQ_TCON0_VBLK);
	else
		tcon_lcd_irq_disable(tcon, LCD_IRQ_TCON0_VBLK);
}

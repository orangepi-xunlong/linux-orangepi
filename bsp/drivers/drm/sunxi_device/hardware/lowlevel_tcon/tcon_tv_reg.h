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

#ifndef __DE_LCD_TYPE_H__
#define __DE_LCD_TYPE_H__

#include <linux/types.h>

/*
 * detail information of registers
 */

union tcon_tv_gctl_reg_t {
	u32 dwval;
	struct {
		u32 io_map_sel:1;
		u32 pad_sel:1;
		u32 res0:2;
		u32 pixel_mode:2;
		u32 res1:24;
		u32 tcon_gamma_en:1;
		u32 tcon_en:1;
	} bits;
};

union tcon_tv_gint0_reg_t {
	u32 dwval;
	struct {
		u32 tcon_irq_flag:16;
		u32 tcon_irq_en:16;
	} bits;
};

union tcon_tv_gint1_reg_t {
	u32 dwval;
	struct {
		u32 tcon_tv_line_int_num:12;
		u32 res0:4;
		u32 tcon0_line_int_num:12;
		u32 res1:4;
	} bits;
};

union tcon_tv_src_ctl_reg_t {
	u32 dwval;
	struct {
		u32 src_sel:3;
		u32 res0:29;
	} bits;
};

union tcon_tv_ctl_reg_t {
	u32 dwval;
	struct {
		u32 src_sel:2;
		u32 res0:2;
		u32 start_delay:5;
		u32 res1:11;
		u32 interlace_en:1;
		u32 res2:10;
		u32 tcon_tv_en:1;
	} bits;
};

union tcon_tv_basic0_reg_t {
	u32 dwval;
	struct {
		u32 y:12;
		u32 res0:4;
		u32 x:12;
		u32 res1:4;
	} bits;
};

union tcon_tv_basic1_reg_t {
	u32 dwval;
	struct {
#if (IS_ENABLED(CONFIG_ARCH_SUN60IW2)) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
		u32 vt:17;
		u32 res0:14;
		u32 vic39:1;
#else
		u32 ls_yo:12;
		u32 res0:4;
		u32 ls_xo:12;
		u32 res1:4;
#endif
	} bits;
};

union tcon_tv_basic2_reg_t {
	u32 dwval;
	struct {
		u32 yo:12;
		u32 res0:4;
		u32 xo:12;
		u32 res1:4;
	} bits;
};

union tcon_tv_basic3_reg_t {
	u32 dwval;
	struct {
		u32 hbp:12;
		u32 res0:4;
		u32 ht:13;
		u32 res1:3;
	} bits;
};

union tcon_tv_basic4_reg_t {
	u32 dwval;
	struct {
		u32 vbp:12;
		u32 res0:4;
		u32 vt:13;
		u32 res1:3;
	} bits;
};

union tcon_tv_basic5_reg_t {
	u32 dwval;
	struct {
		u32 vspw:10;
		u32 res0:6;
		u32 hspw:10;
		u32 res1:6;
	} bits;
};

union tcon_tv_ps_sync_reg_t {
	u32 dwval;
	struct {
		u32 sync_y:16;
		u32 sync_x:16;
	} bits;
};

union tcon_tv_io_pol_reg_t {
	u32 dwval;
	struct {
		u32 data_inv:24;
		u32 io0_inv:1;
		u32 io1_inv:1;
		u32 io2_inv:1;
		u32 io3_inv:1;
		u32 res0:4;
	} bits;
};

union tcon_tv_io_tri_reg_t {
	u32 dwval;
	struct {
		u32 data_output_tri_en:24;
		u32 io0_output_tri_en:1;
		u32 io1_output_tri_en:1;
		u32 io2_output_tri_en:1;
		u32 io3_output_tri_en:1;
		u32 res0:4;
	} bits;
};

union tcon_ecc_fifo_reg_t {
	u32 dwval;
	struct {
		u32 ecc_fifo_setting:8;
		u32 ecc_fifo_blank_en:1;
		u32 res0:7;
		u32 ecc_fifo_err_bits:8;
		u32 res1:6;
		u32 ecc_fifo_err_flag:1;
		u32 ecc_fifo_bist_en:1;
	} bits;
};

union tcon_debug_reg_t {
	u32 dwval;
	struct {
		u32 tcon_tv_current_line:12;
		u32 res0:1;
		u32 ecc_fifo_bypass:1;
		u32 res1:2;
		u32 tcon0_current_line:12;
		u32 tcon_tv_field_polarity:1;
		u32 tcon0_field_polarity:1;
		u32 tcon_tv_fifo_under_flow:1;
		u32 tcon0_fifo_under_flow:1;
	} bits;
};

union tcon_ceu_ctl_reg_t {
	u32 dwval;
	struct {
		u32 res0:31;
		u32 ceu_en:1;
	} bits;
};

union tcon_ceu_coef_mul_reg_t {
	u32 dwval;
	struct {
		u32 value:13;
		u32 res0:19;
	} bits;
};

union tcon_ceu_coef_add_reg_t {
	u32 dwval;
	struct {
		u32 value:19;
		u32 res0:13;
	} bits;
};

union tcon_ceu_coef_rang_reg_t {
	u32 dwval;
	struct {
		u32 max:8;
		u32 res0:8;
		u32 min:8;
		u32 res1:8;
	} bits;
};


union tcon_safe_period_reg_t {
	u32 dwval;
	struct {
		u32 safe_period_mode:2;
		u32 res0:14;
		u32 safe_period_fifo_num:13;
		u32 res1:3;
	} bits;
};

union tcon_tv_fill_ctl_reg_t {
	u32 dwval;
	struct {
		u32 res0:31;
		u32 tcon_tv_fill_en:1;
	} bits;
};

union tcon_tv_fill_begin_reg_t {
	u32 dwval;
	struct {
		u32 fill_begin:24;
		u32 res0:8;
	} bits;
};

union tcon_tv_fill_end_reg_t {
	u32 dwval;
	struct {
		u32 fill_end:24;
		u32 res0:8;
	} bits;
};

union tcon_tv_fill_data_reg_t {
	u32 dwval;
	struct {
		u32 fill_value:24;
		u32 res0:8;
	} bits;
};

union tcon_tv_reservd_reg_t {
	u32 dwval;
	struct {
		u32 res0;
	} bits;
};

union tv_data_io_pol0_reg_t {
	u32 dwval;
	struct {
		u32 g_y_ch_data_inv:10;
		u32 res0:6;
		u32 r_cb_ch_data_inv:10;
		u32 res1:6;
	} bits;
};

union tv_data_io_pol1_reg_t {
	u32 dwval;
	struct {
		u32 res0:16;
		u32 b_cr_ch_data_inv:10;
		u32 res1:6;
	} bits;
};

union tv_data_io_tri0_reg_t {
	u32 dwval;
	struct {
		u32 g_y_ch_data_out_tri_en:10;
		u32 res0:6;
		u32 r_cb_ch_data_out_tri_en:10;
		u32 res1:6;
	} bits;
};

union tv_data_io_tri1_reg_t {
	u32 dwval;
	struct {
		u32 res0:16;
		u32 b_cr_ch_data_out_tri_en:10;
		u32 res1:6;
	} bits;
};

union tv_pixel_depth_mode_reg_t {
	u32 dwval;
	struct {
		u32 pixel_depth:1;
		u32 res0:31;
	} bits;
};


struct tcon_tv_reg {
	/* 0x00 - 0x0c */
	union tcon_tv_gctl_reg_t tcon_gctl;
	union tcon_tv_gint0_reg_t tcon_gint0;
	union tcon_tv_gint1_reg_t tcon_gint1;
	union tcon_tv_reservd_reg_t tcon_reg00c;
	/* 0x10 - 0x1c */
	union tcon_tv_reservd_reg_t tcon_reg010[12];
	/* 0x40 - 0x4c */
	union tcon_tv_src_ctl_reg_t tcon_tv_src_ctl;
	union tcon_tv_reservd_reg_t tcon_reg044[3];
	/* 0x50 - 0x5c */
	union tcon_tv_reservd_reg_t tcon_reg050[12];
	/* 0x80 - 0x8c */
	union tcon_tv_reservd_reg_t tcon_reg080;
	union tcon_tv_reservd_reg_t tcon_reg084;
	union tcon_tv_io_pol_reg_t tcon_tv_io_pol;
	union tcon_tv_io_tri_reg_t tcon_tv_io_tri;
	/* 0x90 - 0x9c */
	/* regs which is from 0x90 to 0xfb are no longer exist */
	union tcon_tv_ctl_reg_t tcon_tv_ctl;
	union tcon_tv_basic0_reg_t tcon_tv_basic0;
	union tcon_tv_basic1_reg_t tcon_tv_basic1;
	union tcon_tv_basic2_reg_t tcon_tv_basic2;
	/* 0xa0 - 0xac */
	union tcon_tv_basic3_reg_t tcon_tv_basic3;
	union tcon_tv_basic4_reg_t tcon_tv_basic4;
	union tcon_tv_basic5_reg_t tcon_tv_basic5;
	union tcon_tv_reservd_reg_t tcon_reg0ac;
	/* 0xb0 - 0xec */
	union tcon_tv_ps_sync_reg_t tcon_tv_ps_ctl;
	union tcon_tv_reservd_reg_t tcon_reg0b4[15];
	/* 0xf0 - 0xfc */
	union tcon_tv_reservd_reg_t tcon_reg0f0[3];
	union tcon_debug_reg_t tcon_debug;
	/* 0x100 - 0x10c */
	union tcon_ceu_ctl_reg_t tcon_ceu_ctl;
	union tcon_tv_reservd_reg_t tcon_reg104[3];
	/* 0x110 - 0x11c */
	union tcon_ceu_coef_mul_reg_t tcon_ceu_coef_rr;
	union tcon_ceu_coef_mul_reg_t tcon_ceu_coef_rg;
	union tcon_ceu_coef_mul_reg_t tcon_ceu_coef_rb;
	union tcon_ceu_coef_add_reg_t tcon_ceu_coef_rc;
	/* 0x120 - 0x12c */
	union tcon_ceu_coef_mul_reg_t tcon_ceu_coef_gr;
	union tcon_ceu_coef_mul_reg_t tcon_ceu_coef_gg;
	union tcon_ceu_coef_mul_reg_t tcon_ceu_coef_gb;
	union tcon_ceu_coef_add_reg_t tcon_ceu_coef_gc;
	/* 0x130 - 0x13c */
	union tcon_ceu_coef_mul_reg_t tcon_ceu_coef_br;
	union tcon_ceu_coef_mul_reg_t tcon_ceu_coef_bg;
	union tcon_ceu_coef_mul_reg_t tcon_ceu_coef_bb;
	union tcon_ceu_coef_add_reg_t tcon_ceu_coef_bc;
	/* 0x140 - 0x14c */
	union tcon_ceu_coef_rang_reg_t tcon_ceu_coef_rv;
	union tcon_ceu_coef_rang_reg_t tcon_ceu_coef_gv;
	union tcon_ceu_coef_rang_reg_t tcon_ceu_coef_bv;
	union tcon_tv_reservd_reg_t tcon_reg14c;
	/* 0x150 - 0x15c */
	union tcon_tv_reservd_reg_t tcon_reg150[40];
	/* 0x1f0 - 0x1fc */
	union tcon_safe_period_reg_t tcon_volume_ctl;
	union tcon_tv_reservd_reg_t tcon_reg1f4[3];
	/* 0x200 - 0x21c */
	union tcon_tv_reservd_reg_t tcon_reg200[64];
	/* 0x300 - 0x30c */
	union tcon_tv_fill_ctl_reg_t tcon_fill_ctl;
	union tcon_tv_fill_begin_reg_t tcon_fill_start0;
	union tcon_tv_fill_end_reg_t tcon_fill_end0;
	union tcon_tv_fill_data_reg_t tcon_fill_data0;
	/* 0x310 - 0x31c */
	union tcon_tv_fill_begin_reg_t tcon_fill_start1;
	union tcon_tv_fill_end_reg_t tcon_fill_end1;
	union tcon_tv_fill_data_reg_t tcon_fill_data1;
	union tcon_tv_fill_begin_reg_t tcon_fill_start2;
	/* 0x320 - 0x32c */
	union tcon_tv_fill_end_reg_t tcon_fill_end2;
	union tcon_tv_fill_data_reg_t tcon_fill_data2;
	union tcon_tv_reservd_reg_t tcon_reg328[2];
	/* 0x330 - 0x340 */
	union tv_data_io_pol0_reg_t tv_data_io_pol0;
	union tv_data_io_pol1_reg_t tv_data_io_pol1;
	union tv_data_io_tri0_reg_t tv_data_io_tri0;
	union tv_data_io_tri1_reg_t tv_data_io_tri1;
	union tv_pixel_depth_mode_reg_t pixel_depth_mode;
};

#endif

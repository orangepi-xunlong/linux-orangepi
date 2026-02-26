/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2007-2021 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/io.h>
#include <../drivers/misc/sunxi-tvutils/afbd_hwspinlock.h>
#include <video/decoder_display.h>

#include "dec_reg.h"


/*
 * skip buffer select register control,
 * mips firmware is responsible for updating this register!!!
 */
#define DEC_SKIP_BUFFER_SELECT_OP (1)

static volatile uint32_t bugval;

#ifdef CONFIG_SUNXI_AFBD_WORKAROUND
#define PATCH_READ(x) { \
	bugval = readl((void *)x); \
	bugval = readl((void *)x); \
	bugval = readl((void *)x); \
	bugval = readl((void *)x); \
}
#else
#define PATCH_READ(x)
#endif

void dec_reg_top_enable(struct afbd_reg_t *p_reg, unsigned int en)
{
	int locked;
	unsigned long flags;

	p_reg->p_tvdisp_top->afbd_clk.bits.afbd_ahb_clk_gate = en;
	p_reg->p_tvdisp_top->afbd_clk.bits.afbd_mbus_clk_gate = en;
	p_reg->p_tvdisp_top->afbd_clk.bits.afbd_func_clk_gate = en;
	p_reg->p_tvdisp_top->afbd_clk.bits.afbd_pixel_clk_sel = en;
	p_reg->p_tvdisp_top->afbd_rst.bits.afbd_rst = en;
	p_reg->p_tvdisp_top->out_sel.dwval = 0xffffffff;
	p_reg->p_tvdisp_top->svp_clk.dwval = 0xffffffff;
	p_reg->p_tvdisp_top->svp_rst.dwval = 0xffffffff;
	p_reg->p_tvdisp_top->panel_rst.dwval = 0xffffffff;
	p_reg->p_tvdisp_top->panel_ext_gate.dwval = 0xffffffff;
	p_reg->p_tvdisp_top->panel_int_gate.dwval = 0xffffffff;

	locked = __afbd_hwspinlock_lock(HWLOCK_IRQSTATE, &flags);

	PATCH_READ(&(p_reg->p_top_reg->ctrl.dwval));
	p_reg->p_top_reg->ctrl.bits.en = en;

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IRQSTATE, &flags);
}

void dec_reg_enable(struct afbd_reg_t *p_reg, unsigned int en)
{
	int locked;
	unsigned long flags;
	locked = __afbd_hwspinlock_lock(HWLOCK_IRQSTATE, &flags);

#if (DEC_SKIP_BUFFER_SELECT_OP == 0)
	PATCH_READ(&(p_reg->p_dec_reg->ybuf_sel.dwval));
	PATCH_READ(&(p_reg->p_dec_reg->cbuf_sel.dwval));
	p_reg->p_dec_reg->ybuf_sel.bits.uncom_addr_ctrl = 1;
#endif
	PATCH_READ(&(p_reg->p_dec_reg->ctrl.dwval));
	p_reg->p_dec_reg->ctrl.bits.en = en;
	PATCH_READ(&(p_reg->p_dec_reg->int_ctrl.dwval));
	p_reg->p_dec_reg->int_ctrl.bits.vs_int_en = en;

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IRQSTATE, &flags);
}

void dec_reg_mux_select(struct afbd_reg_t *p_reg, unsigned int mux)
{
	int locked;
	unsigned long flags;
	locked = __afbd_hwspinlock_lock(HWLOCK_IRQSTATE, &flags);

#if (DEC_SKIP_BUFFER_SELECT_OP == 0)
	PATCH_READ(&(p_reg->p_dec_reg->ybuf_sel.dwval));
	PATCH_READ(&(p_reg->p_dec_reg->cbuf_sel.dwval));
	p_reg->p_dec_reg->ybuf_sel.bits.addr_cur_y_out_mux = mux;
	p_reg->p_dec_reg->cbuf_sel.bits.addr_cur_c_out_mux = mux;
#endif

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IRQSTATE, &flags);
}

void dec_reg_int_to_display(struct afbd_reg_t *p_reg)
{
	int locked;
	unsigned long flags;
	locked = __afbd_hwspinlock_lock(HWLOCK_IRQSTATE, &flags);

	PATCH_READ(&(p_reg->p_dec_reg->ctrl.dwval));
	p_reg->p_dec_reg->ctrl.bits.int_to_display = 1;

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IRQSTATE, &flags);
}

void dec_reg_int_to_display_atomic(struct afbd_reg_t *p_reg)
{
	int locked;
	unsigned long flags;
	locked = __afbd_hwspinlock_lock(HWLOCK_IN_ATOMIC, &flags);

	PATCH_READ(&(p_reg->p_dec_reg->ctrl.dwval));
	p_reg->p_dec_reg->ctrl.bits.int_to_display = 1;

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IN_ATOMIC, &flags);
}

void dec_reg_blue_en(struct afbd_reg_t *p_reg, unsigned int en)
{
	int locked;
	unsigned long flags;
	locked = __afbd_hwspinlock_lock(HWLOCK_IRQSTATE, &flags);

	PATCH_READ(&(p_reg->p_dec_reg->fild_mode.dwval));
	p_reg->p_dec_reg->fild_mode.bits.blue_en_out = en;

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IRQSTATE, &flags);
}

void dec_reg_set_address(struct afbd_reg_t *p_reg,
			  unsigned long long *frame_addr,
			  unsigned long long info_addr, unsigned int id)
{
	int locked;
	unsigned long flags;
	locked = __afbd_hwspinlock_lock(HWLOCK_IN_ATOMIC, &flags);

	switch (id) {
	case 0:
		p_reg->p_dec_reg->y_addr0 = frame_addr[0];
		p_reg->p_dec_reg->c_addr0 = frame_addr[1];
		PATCH_READ(&(p_reg->p_dec_reg->y_high_addr.dwval));
		p_reg->p_dec_reg->y_high_addr.bits.addr0 = frame_addr[0] >> 32;
		PATCH_READ(&(p_reg->p_dec_reg->c_high_addr.dwval));
		p_reg->p_dec_reg->c_high_addr.bits.addr0 = frame_addr[1] >> 32;
		p_reg->p_dec_reg->video_info0_addr = info_addr;
		PATCH_READ(&(p_reg->p_dec_reg->info_high_addr.dwval));
		p_reg->p_dec_reg->info_high_addr.bits.addr0 = info_addr >> 32;
		break;
	case 1:
		p_reg->p_dec_reg->y_addr1 = frame_addr[0];
		p_reg->p_dec_reg->c_addr1 = frame_addr[1];
		PATCH_READ(&(p_reg->p_dec_reg->y_high_addr.dwval));
		p_reg->p_dec_reg->y_high_addr.bits.addr1 = frame_addr[0] >> 32;
		PATCH_READ(&(p_reg->p_dec_reg->c_high_addr.dwval));
		p_reg->p_dec_reg->c_high_addr.bits.addr1 = frame_addr[1] >> 32;
		p_reg->p_dec_reg->video_info1_addr = info_addr;
		PATCH_READ(&(p_reg->p_dec_reg->info_high_addr.dwval));
		p_reg->p_dec_reg->info_high_addr.bits.addr1 = info_addr >> 32;
		break;
	case 2:
		p_reg->p_dec_reg->y_addr2 = frame_addr[0];
		p_reg->p_dec_reg->c_addr2 = frame_addr[1];
		PATCH_READ(&(p_reg->p_dec_reg->y_high_addr.dwval));
		p_reg->p_dec_reg->y_high_addr.bits.addr2 = frame_addr[0] >> 32;
		PATCH_READ(&(p_reg->p_dec_reg->c_high_addr.dwval));
		p_reg->p_dec_reg->c_high_addr.bits.addr2 = frame_addr[1] >> 32;
		p_reg->p_dec_reg->video_info2_addr = info_addr;
		PATCH_READ(&(p_reg->p_dec_reg->info_high_addr.dwval));
		p_reg->p_dec_reg->info_high_addr.bits.addr2 = info_addr >> 32;
		break;
	case 3:
		p_reg->p_dec_reg->y_addr3 = frame_addr[0];
		p_reg->p_dec_reg->c_addr3 = frame_addr[1];
		PATCH_READ(&(p_reg->p_dec_reg->y_high_addr.dwval));
		p_reg->p_dec_reg->y_high_addr.bits.addr3 = frame_addr[0] >> 32;
		PATCH_READ(&(p_reg->p_dec_reg->c_high_addr.dwval));
		p_reg->p_dec_reg->c_high_addr.bits.addr3 = frame_addr[1] >> 32;
		p_reg->p_dec_reg->video_info3_addr = info_addr;
		PATCH_READ(&(p_reg->p_dec_reg->info_high_addr.dwval));
		p_reg->p_dec_reg->info_high_addr.bits.addr3 = info_addr >> 32;
		break;
	default:
		break;
	}

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IN_ATOMIC, &flags);
}

void dec_reg_set_videoinfo(struct afbd_reg_t *p_reg,
		unsigned long long info_addr, unsigned int id)
{
	switch (id) {
	case 0:
		p_reg->p_dec_reg->video_info0_addr = info_addr;
		p_reg->p_dec_reg->info_high_addr.bits.addr0 = info_addr >> 32;
		break;
	case 1:
		p_reg->p_dec_reg->video_info1_addr = info_addr;
		p_reg->p_dec_reg->info_high_addr.bits.addr1 = info_addr >> 32;
		break;
	case 2:
		p_reg->p_dec_reg->video_info2_addr = info_addr;
		p_reg->p_dec_reg->info_high_addr.bits.addr2 = info_addr >> 32;
		break;
	case 3:
		p_reg->p_dec_reg->video_info3_addr = info_addr;
		p_reg->p_dec_reg->info_high_addr.bits.addr3 = info_addr >> 32;
		break;
	default:
		break;
	}
}

void dec_reg_set_filed_mode(struct afbd_reg_t *p_reg, unsigned int filed_mode)
{
	int locked;
	unsigned long flags;
	locked = __afbd_hwspinlock_lock(HWLOCK_IN_ATOMIC, &flags);

	PATCH_READ(&(p_reg->p_dec_reg->fild_mode.dwval));
	p_reg->p_dec_reg->fild_mode.bits.field_mode = filed_mode;

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IN_ATOMIC, &flags);
}

void dec_reg_set_filed_repeat(struct afbd_reg_t *p_reg, unsigned int enable)
{
	int locked;
	unsigned long flags;
	locked = __afbd_hwspinlock_lock(HWLOCK_IRQSTATE, &flags);

	PATCH_READ(&(p_reg->p_dec_reg->fild_mode.dwval));
	p_reg->p_dec_reg->fild_mode.bits.filed_rpt_mode_out = enable;

	//dec_reg_set_dirty(p_reg, 1);

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IRQSTATE, &flags);
}

static inline uint32_t dec_workaround_read_internal(
		struct afbd_reg_t *p_reg, uint32_t offset, int mode)
{
	int locked;
	unsigned long flags;
	volatile uint32_t a[5];
	void __iomem *addr = ((void *)p_reg->p_top_reg + offset);

	locked = __afbd_hwspinlock_lock(mode, &flags);

	a[0] = readl(addr);
	a[1] = readl(addr);
	a[2] = readl(addr);
	a[3] = readl(addr);
	a[4] = readl(addr);

	if (locked == 0)
		__afbd_hwspinlock_unlock(mode, &flags);
	else
		pr_err("read afbd without hwspinlock");

	return a[4];
}

uint32_t dec_workaround_read(struct afbd_reg_t *p_reg, uint32_t offset)
{
	return dec_workaround_read_internal(p_reg, offset, HWLOCK_IRQSTATE);
}

uint32_t dec_workaround_read_atomic(struct afbd_reg_t *p_reg, uint32_t offset)
{
	return dec_workaround_read_internal(p_reg, offset, HWLOCK_IN_ATOMIC);
}

unsigned int dec_irq_query(struct afbd_reg_t *p_reg)
{

	if (p_reg->p_dec_reg->vs_flag.bits.vs_flag &&
	    p_reg->p_dec_reg->int_ctrl.bits.vs_int_en) {
		p_reg->p_dec_reg->vs_flag.bits.vs_flag = 1;
		return 1;
	}
#if FPGA_DEBUG_ENABLE == 1
	return 1;
#endif
	return 0;

}

void dec_reg_set_dirty(struct afbd_reg_t *p_reg, _Bool dirty)
{
	p_reg->reg_dirty = dirty;
	p_reg->p_dec_reg->afbd_dec_reg_rdy = dirty;
#if FPGA_DEBUG_ENABLE == 1
	p_reg->p_top_reg->reg_ready = dirty;
#endif
}

_Bool dec_reg_is_dirty(struct afbd_reg_t *p_reg)
{
	return p_reg->reg_dirty;
}

unsigned int dec_reg_frame_cnt(struct afbd_reg_t *p_reg)
{
	return p_reg->p_dec_reg->frame_cnt;
}

unsigned int dec_reg_get_y_address(struct afbd_reg_t *p_reg, unsigned int id)
{
	switch (id) {
	case 0: return p_reg->p_dec_reg->y_addr0;
	case 1: return p_reg->p_dec_reg->y_addr1;
	default: return 0;
	}
}

unsigned int dec_reg_get_c_address(struct afbd_reg_t *p_reg, unsigned int id)
{
	switch (id) {
	case 0: return p_reg->p_dec_reg->c_addr0;
	case 1: return p_reg->p_dec_reg->c_addr1;
	default: return 0;
	}
}

unsigned int dec_reg_video_channel_attr_config(struct afbd_reg_t *p_reg,
		struct dec_buffer_attr *inattr)
{
	unsigned int shift_w = 0, shift_h = 0, block_w = 0, block_h = 0;
	struct afbd_top_reg *top = p_reg->p_top_reg;

	if (!p_reg || !inattr) {
		pr_err("Null parameter!\n");
		return -1;
	}

	if (inattr->is_compressed) {
		switch (inattr->sb_layout) {
		case 1:
		case 2:
			// 16*16
			shift_w = 4;
			block_w = 16;
			shift_h = 4;
			block_h = 16;
			break;
		case 3:
		case 5:
		case 6:
			// 32*8
			shift_w = 5;
			block_w = 32;
			shift_h = 3;
			block_h = 8;
			break;
		case 7:
			// 64*4
			shift_w = 6;
			block_w = 64;
			shift_h = 2;
			block_h = 4;
			break;
		default:
			pr_err("Invalid superblock layout:%u\n",
			       inattr->sb_layout);
			return -2;
			break;
		}
		top->block_size.bits.width	= (inattr->width  - 1) >> shift_w;
		top->block_size.bits.height = (inattr->height - 1) >> shift_h;
		top->source_crop.bits.top_crop = (inattr->height % block_h);
		top->source_crop.bits.left_crop = (inattr->width % block_w);
		top->out_crop.bits.width = inattr->width;
		top->out_crop.bits.height = inattr->height;
		top->y_stride = 0;
		top->c_stride = 0;
		top->y_crop_size.dwval = 0;
		top->c_crop_size.dwval = 0;
		top->attr.bits.source = 0;
		top->attr.bits.sb_layout = inattr->sb_layout;
		top->size.bits.width  = inattr->width - 1;
		top->size.bits.height = inattr->height - 1;
	} else {
		top->attr.bits.source = 1;
		switch (inattr->format) {
		case DEC_FORMAT_YUV420P_10BIT:
		case DEC_FORMAT_YUV420P_10BIT_AV1:
		case DEC_FORMAT_YUV422P_10BIT:
			top->y_stride = inattr->width * 2;
			top->c_stride = inattr->width * 2;
			top->y_crop_size.bits.width  = inattr->width * 2;
			top->y_crop_size.bits.height = inattr->height;
			top->c_crop_size.bits.width  = inattr->width * 2;
			top->c_crop_size.bits.height = inattr->height >> 1;
			break;
		case DEC_FORMAT_YUV420P:
		case DEC_FORMAT_YUV422P:
		case DEC_FORMAT_YUV444P:
			top->y_stride = inattr->width;
			top->c_stride = inattr->width;
			top->y_crop_size.bits.width  = inattr->width;
			top->y_crop_size.bits.height = inattr->height;
			top->c_crop_size.bits.width  = inattr->width;
			top->c_crop_size.bits.height = inattr->height >> 1;
			break;
		default:
			break;
		}
	}

	top->attr.bits.format = inattr->format;
	top->attr.bits.components = 3;
	top->attr.bits.is_compressed = inattr->is_compressed;



	top->attr.bits.y_en = 1;
	top->attr.bits.c_en = 1;
	return 0;
}

void dec_reg_bypass_config(struct afbd_reg_t *p_reg, int enable)
{
	int locked;
	unsigned long flags;
	locked = __afbd_hwspinlock_lock(HWLOCK_IRQSTATE, &flags);

	PATCH_READ(&(p_reg->p_dec_reg->ybuf_sel.dwval));
	p_reg->p_dec_reg->ybuf_sel.bits.uncom_addr_ctrl = enable ? 0 : 1;
	p_reg->p_dec_reg->afbd_dec_reg_rdy = 1;

	if (locked == 0)
		__afbd_hwspinlock_unlock(HWLOCK_IRQSTATE, &flags);
}

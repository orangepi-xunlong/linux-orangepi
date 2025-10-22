/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :  display engine 35x deband basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/
#include "de_deband_type.h"
#include "de_enhance.h"
#include "de_rtmx.h"

enum { DEBAND_PARA_REG_BLK = 0,
       DEBAND_REG_BLK_NUM,
};

struct de_deband_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[DEBAND_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_deband_private *priv, u32 blk_id,
			      u32 dirty);
};

static struct de_deband_private deband_priv[DE_NUM];
static enum enhance_init_state g_init_state;
static win_percent_t win_per;
static u32 pq_deband_ctr;

static inline struct deband_reg *get_deband_reg(struct de_deband_private *priv)
{
	return (
	    struct deband_reg *)(priv->reg_blks[DEBAND_PARA_REG_BLK].vir_addr);
}

static void deband_set_block_dirty(struct de_deband_private *priv, u32 blk_id,
				   u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void deband_set_rcq_head_dirty(struct de_deband_private *priv,
				      u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WARN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

static s32 deband_init(u32 disp, uintptr_t reg_base)
{
	struct de_deband_private *priv = &deband_priv[disp];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	/*u32 lcdgamma;
	s32 value = 1;
	s8 primary_key[20];
	s32 ret;*/

	base = reg_base + DE_DISP_OFFSET(disp) + DISP_DEBAND_OFFSET;

	reg_mem_info->size = sizeof(struct deband_reg);
	reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
	    reg_mem_info->size, (void *)&(reg_mem_info->phy_addr), rcq_used);
	if (reg_mem_info->vir_addr == NULL) {
		DE_WARN("alloc deband[%d] mm fail!size=0x%x\n", disp,
		       reg_mem_info->size);
		return -1;
	}

	priv->reg_blk_num = DEBAND_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[DEBAND_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = sizeof(struct deband_reg);
	reg_blk->reg_addr = (u8 __iomem *)base;
	if (rcq_used)
		priv->set_blk_dirty = deband_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = deband_set_block_dirty;

	return 0;
}

s32 de_deband_init(u32 disp, uintptr_t reg_base)
{
	if (de_feat_is_support_deband(disp))
		deband_init(disp, reg_base);

	g_init_state = ENHANCE_INVALID;
	return 0;
}

s32 de_deband_exit(u32 disp) { return 0; }

s32 de_deband_get_reg_blocks(u32 disp, struct de_reg_block **blks, u32 *blk_num)
{
	struct de_deband_private *priv = &(deband_priv[disp]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WARN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	return 0;
}

s32 de_deband_set_size(u32 disp, u32 width, u32 height)
{
	struct de_deband_private *priv = &(deband_priv[disp]);
	struct deband_reg *reg = get_deband_reg(priv);

	if (!de_feat_is_support_deband(disp))
		return -1;

	reg->size.bits.width = width - 1;
	reg->size.bits.height = height - 1;
	priv->set_blk_dirty(priv, DEBAND_PARA_REG_BLK, 1);

	return 0;
}

s32 de_deband_set_window(u32 disp, u32 win_enable, struct de_rect_o window)
{
	struct de_deband_private *priv = &(deband_priv[disp]);
	struct deband_reg *reg = get_deband_reg(priv);

	if (!de_feat_is_support_deband(disp))
		return -1;

	if (g_init_state >= ENHANCE_TIGERLCD_ON) {
		reg->demo_horz.bits.demo_horz_start =
		    window.x + reg->size.bits.width * win_per.hor_start / 100;
		reg->demo_horz.bits.demo_horz_end =
		    window.x + reg->size.bits.width * win_per.hor_end / 100;
		reg->demo_vert.bits.demo_vert_start =
		    window.y + reg->size.bits.height * win_per.ver_start / 100;
		reg->demo_vert.bits.demo_vert_end =
		    window.y + reg->size.bits.height * win_per.ver_end / 100;
	}
	if (win_enable) {
		reg->demo_horz.bits.demo_horz_start = window.x;
		reg->demo_horz.bits.demo_horz_end = window.x + window.w - 1;
		reg->demo_vert.bits.demo_vert_start = window.y;
		reg->demo_vert.bits.demo_vert_end = window.y + window.h - 1;
	}
	reg->ctl.bits.demo_en = win_enable | win_per.demo_en;
	DE_INFO("disp=%d, size=[%d,%d,%d,%d]\n", disp,  window.x, window.w, window.y, window.h);
	priv->set_blk_dirty(priv, DEBAND_PARA_REG_BLK, 1);

	return 0;
}

/*static s32 de_deband_apply(u32 disp, u32 deband_en)
{
	struct de_deband_private *priv = &deband_priv[disp];
	struct deband_reg *reg = get_deband_reg(priv);

    reg->ctl.dwval = deband_en ? 0x1111 : 0x0;
    priv->set_blk_dirty(priv, DEBAND_PARA_REG_BLK, 1);

	return 0;
}*/

s32 de_deband_info2para(u32 disp, u32 fmt, u32 dev_type, u32 bypass)
{
	struct de_deband_private *priv = &(deband_priv[disp]);
	struct deband_reg *reg = get_deband_reg(priv);
	u8 enable = 0;

	if (!de_feat_is_support_deband(disp))
		return -1;

	if (fmt == 1 || bypass)
		enable = 0;
	else
		enable = 1;

	if (enable)
		reg->ctl.dwval = 0x1111;
	else
		reg->ctl.dwval = 0x0;
	priv->set_blk_dirty(priv, DEBAND_PARA_REG_BLK, 1);

	return 0;
}

s32 de_deband_enable(u32 disp, u32 en)
{
	struct de_deband_private *priv = &(deband_priv[disp]);
	struct deband_reg *reg = get_deband_reg(priv);

	if (!de_feat_is_support_deband(disp))
		return -1;

	if (en)
		reg->ctl.dwval = 0x1111;
	else
		reg->ctl.dwval = 0x0;
	priv->set_blk_dirty(priv, DEBAND_PARA_REG_BLK, 1);

	return 0;
}

s32 de_deband_init_para(u32 disp)
{
	struct de_deband_private *priv = &(deband_priv[disp]);
	struct deband_reg *reg = get_deband_reg(priv);

	if (!de_feat_is_support_deband(disp))
		return -1;

	if (g_init_state >= ENHANCE_INITED) {
		return 0;
	}
	g_init_state = ENHANCE_INITED;
	reg->ctl.dwval = 0x1111;
	reg->cs.dwval = 1;
	reg->output_bits.dwval = 0x8;
	reg->step.dwval = 0x20002;
	reg->edge.dwval = 0x6e006e;
	reg->vmin_steps.dwval = 0x04040202;
	reg->vfilt_para0.dwval = 0x224444;
	reg->vfilt_para1.dwval = 0x20200e0e;
	reg->hmin_steps.dwval = 0x04040202;
	reg->hfilt_para0.dwval = 0x20200b0b;
	reg->hfilt_para1.dwval = 0x01000100;
	reg->err_diffuse_w.dwval = 0x02050207;
	reg->rand_dither.dwval = 0x80000003;
	reg->random_gen0.dwval = 0x0;
	reg->random_gen1.dwval = 0x0;
	reg->random_gen2.dwval = 0x0;

	priv->set_blk_dirty(priv, DEBAND_PARA_REG_BLK, 1);
	return 0;
}

s32 de_deband_set_outinfo(int disp, enum de_color_space cs,
			  enum de_data_bits bits, enum de_format_space fmt)
{
	struct de_deband_private *priv = &(deband_priv[disp]);
	struct deband_reg *reg = get_deband_reg(priv);
	u8 enable = 0;

	if (!de_feat_is_support_deband(disp))
		return -1;

	if (bits <= DE_DATA_10BITS &&
	    ((fmt == DE_FORMAT_SPACE_RGB &&
	      (cs == DE_COLOR_SPACE_BT601 || cs == DE_COLOR_SPACE_BT2020NC ||
	       cs == DE_COLOR_SPACE_BT2020C)) ||
	     fmt == DE_FORMAT_SPACE_YUV))
		enable = 1;
	else
		enable = 0;

	if (enable) {
		if (g_init_state < ENHANCE_INITED) {
			reg->ctl.dwval = 0x1111;
		} else if (g_init_state >= ENHANCE_TIGERLCD_ON) {
			reg->ctl.dwval = pq_deband_ctr;
		}
	} else {
		reg->ctl.dwval = 0x0;
		priv->set_blk_dirty(priv, DEBAND_PARA_REG_BLK, 1);
		return 0;
	}

	if (bits == DE_DATA_10BITS)
		reg->output_bits.bits.output_bits = 10;
	else if (bits == DE_DATA_8BITS)
		reg->output_bits.bits.output_bits = 8;

	if (cs == DE_COLOR_SPACE_BT601)
		reg->cs.bits.input_color_space = 0;
	else if (cs == DE_COLOR_SPACE_BT2020NC || cs == DE_COLOR_SPACE_BT2020C)
		reg->cs.bits.input_color_space = 2;
	else if (fmt == DE_FORMAT_SPACE_YUV)
		reg->cs.bits.input_color_space = 1;

	priv->set_blk_dirty(priv, DEBAND_PARA_REG_BLK, 1);
	return 0;
}

int de_deband_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data)
{
	struct de_deband_private *priv = &(deband_priv[sel]);
	struct deband_reg *reg = get_deband_reg(priv);
	deband_module_param_t *para = NULL;
	DE_INFO("sel=%d, cmd=%d, subcmd=%d, data=%px\n", sel, cmd, subcmd, data);

	if (!de_feat_is_support_deband(sel))
		return -1;

	para = (deband_module_param_t *)data;
	if (para == NULL) {
		DE_WARN("para NULL\n");
		return -1;
	}

	if (subcmd == 16) { /* read */
		para->value[0] = reg->ctl.bits.hdeband_en;
		para->value[1] = reg->ctl.bits.c_hdeband_en;
		para->value[2] = reg->ctl.bits.vdeband_en;
		para->value[3] = reg->ctl.bits.c_vdeband_en;
		/*para->value[4] = reg->ctl.bits.demo_en;*/
		para->value[5] = reg->rand_dither.bits.rand_dither_en;
		para->value[6] = reg->cs.bits.input_color_space;
		para->value[7] = reg->output_bits.bits.output_bits;
		para->value[8] = reg->step.bits.step_th;
		para->value[9] = reg->edge.bits.edge_th;
		para->value[10] = reg->vmin_steps.bits.vmin_steps;
		para->value[11] = reg->vmin_steps.bits.vmin_ratio;
		para->value[12] = reg->vfilt_para1.bits.adp_w_up;
		para->value[13] = reg->hmin_steps.bits.hmin_steps;
		para->value[14] = reg->hmin_steps.bits.hmin_ratio;
		para->value[15] = reg->hfilt_para0.bits.kmax;
		para->value[16] = reg->hfilt_para0.bits.hsigma;
		para->value[17] = reg->rand_dither.bits.rand_num_bits;
		/*para->value[18] = reg->demo_horz.bits.demo_horz_start;
		para->value[19] = reg->demo_horz.bits.demo_horz_end;
		para->value[20] = reg->demo_vert.bits.demo_vert_start;
		para->value[21] = reg->demo_vert.bits.demo_vert_end;*/
		para->value[18] = win_per.hor_start;
		para->value[19] = win_per.hor_end;
		para->value[20] = win_per.ver_start;
		para->value[21] = win_per.ver_end;
		para->value[4] = win_per.demo_en;
	} else {
		g_init_state = ENHANCE_TIGERLCD_ON;
		reg->ctl.bits.hdeband_en = para->value[0];
		reg->ctl.bits.c_hdeband_en = para->value[1];
		reg->ctl.bits.vdeband_en = para->value[2];
		reg->ctl.bits.c_vdeband_en = para->value[3];
		/*reg->ctl.bits.demo_en = para->value[4];*/
		reg->rand_dither.bits.rand_dither_en = para->value[5];
		reg->cs.bits.input_color_space = para->value[6];
		reg->output_bits.bits.output_bits = para->value[7];
		reg->step.bits.step_th = para->value[8];
		reg->edge.bits.edge_th = para->value[9];
		reg->vmin_steps.bits.vmin_steps = para->value[10];
		reg->vmin_steps.bits.vmin_ratio = para->value[11];
		reg->vfilt_para1.bits.adp_w_up = para->value[12];
		reg->hmin_steps.bits.hmin_steps = para->value[13];
		reg->hmin_steps.bits.hmin_ratio = para->value[14];
		reg->hfilt_para0.bits.kmax = para->value[15];
		reg->hfilt_para0.bits.hsigma = para->value[16];
		reg->rand_dither.bits.rand_num_bits = para->value[17];
		/*reg->demo_horz.bits.demo_horz_start = para->value[18];
		reg->demo_horz.bits.demo_horz_end = para->value[19];
		reg->demo_vert.bits.demo_vert_start = para->value[20];
		reg->demo_vert.bits.demo_vert_end = para->value[21];*/
		win_per.hor_start = para->value[18];
		win_per.hor_end = para->value[19];
		win_per.ver_start = para->value[20];
		win_per.ver_end = para->value[21];
		win_per.demo_en = para->value[4];
		reg->ctl.bits.demo_en = win_per.demo_en;
		pq_deband_ctr = reg->ctl.dwval;
		reg->demo_horz.bits.demo_horz_start =
		    reg->size.bits.width * win_per.hor_start / 100;
		reg->demo_horz.bits.demo_horz_end =
		    reg->size.bits.width * win_per.hor_end / 100;
		reg->demo_vert.bits.demo_vert_start =
		    reg->size.bits.height * win_per.ver_start / 100;
		reg->demo_vert.bits.demo_vert_end =
		    reg->size.bits.height * win_per.ver_end / 100;

		priv->set_blk_dirty(priv, DEBAND_PARA_REG_BLK, 1);
	}
	return 0;
}

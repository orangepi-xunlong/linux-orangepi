/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include "de_bld.h"
#include "de_bld_type.h"
#include "de_bld_platform.h"

enum de_blend_mode {
	/*
	* pixel color = sc * sa * cfs + dc * da * cfd
	* pixel alpha = sa * afs + da * afd
	* sc = source color
	* sa = source alpha
	* dc = destination color
	* da = destination alpha
	* cfs = source color factor for blend function
	* cfd = destination color factor for blend function
	* afs = source alpha factor for blend function
	* afd = destination alpha factor for blend function
	*/
	DE_BLD_MODE_CLEAR   = 0x00000000, /* cfs/afs: 0     cfd/afd: 0    */
	DE_BLD_MODE_SRC     = 0x00010001, /* cfs/afs: 1     cfd/afd: 0    */
	DE_BLD_MODE_DST     = 0x01000100, /* cfs/afs: 0     cfd/afd: 1    */
	DE_BLD_MODE_SRCOVER = 0x03010301, /* cfs/afs: 1     cfd/afd: 1-sa */
	DE_BLD_MODE_DSTOVER = 0x01030103, /* cfs/afs: 1-da  cfd/afd: 1    */
	DE_BLD_MODE_SRCIN   = 0x00020002, /* cfs/afs: da    cfd/afd: 0    */
	DE_BLD_MODE_DSTIN   = 0x02000200, /* cfs/afs: 0     cfd/afd: sa   */
	DE_BLD_MODE_SRCOUT  = 0x00030003, /* cfs/afs: 1-da  cfd/afd: 0    */
	DE_BLD_MODE_DSTOUT  = 0x03000300, /* cfs/afs: 0     cfd/afd: 1-sa */
	DE_BLD_MODE_SRCATOP = 0x03020302, /* cfs/afs: da    cfd/afd: 1-sa */
	DE_BLD_MODE_DSTATOP = 0x02030203, /* cfs/afs: 1-da  cfd/afd: sa   */
	DE_BLD_MODE_XOR     = 0x03030303, /* cfs/afs: 1-da  cfd/afd: 1-sa */
};

enum {
	BLD_REG_BLK_ATTR = 0,
	BLD_REG_BLK_CTL,
	BLD_REG_BLK_CK,
	BLD_REG_BLK_NUM,
};

struct bld_debug_info {
	bool enable;
	struct drm_rect crtc_pos;
	unsigned int port_id;
	bool is_premult;
};

struct de_bld_private {
	struct de_reg_mem_info reg_mem_info;
	struct bld_debug_info debug[BLD_PORT_MAX];
	const struct de_bld_desc *dsc;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[BLD_REG_BLK_NUM];
};

static inline struct bld_reg *get_bld_reg(struct de_bld_private *priv)
{
	return (struct bld_reg *)(priv->reg_blks[0].vir_addr);
}

static inline struct bld_reg *get_bld_hw_reg(struct de_bld_private *priv)
{
	return (struct bld_reg *)(priv->reg_blks[0].reg_addr);
}

static void bld_set_block_dirty(
	struct de_bld_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static int de_bld_set_pipe_fcolor(struct de_bld_handle *hdl, u32 pipe, u32 color, bool apply)
{
	struct de_bld_private *priv = hdl->private;
	struct bld_reg *reg = get_bld_reg(priv);
	struct bld_reg *hwreg = get_bld_hw_reg(priv);

	if (apply) {
		hwreg->pipe.en.bits.pipe0_fcolor_en = 1;
		hwreg->pipe.attr[0].fcolor.dwval = color;
	}
	reg->pipe.en.bits.pipe0_fcolor_en = 1;
	reg->pipe.attr[0].fcolor.dwval = color;

	bld_set_block_dirty(priv, BLD_REG_BLK_ATTR, 1);
	return 0;
}

int de_bld_output_set_attr(struct de_bld_handle *hdl, u32 width, u32 height, u32 fmt_space,
			   u32 interlaced, bool apply)
{
	struct de_bld_private *priv = hdl->private;
	struct bld_reg *reg = get_bld_reg(priv);
	struct bld_reg *hwreg = get_bld_hw_reg(priv);

	if (apply) {
		hwreg->out_size.dwval = (width ? ((width - 1) & 0x1FFF) : 0)
			| (height ? (((height - 1) & 0x1FFF) << 16) : 0);
		hwreg->out_ctl.bits.fmt_space = fmt_space;
		hwreg->out_ctl.bits.premul_en = 0;
		hwreg->out_ctl.bits.interlace_en = interlaced;
	}
	reg->out_size.dwval = (width ? ((width - 1) & 0x1FFF) : 0)
		| (height ? (((height - 1) & 0x1FFF) << 16) : 0);
	reg->out_ctl.bits.fmt_space = fmt_space;
	reg->out_ctl.bits.premul_en = 0;
	reg->out_ctl.bits.interlace_en = interlaced;

	if (fmt_space != DE_FORMAT_SPACE_RGB) {
		if (apply)
			hwreg->bg_color.dwval = 0x108080;
		reg->bg_color.dwval = 0x108080;
		de_bld_set_pipe_fcolor(hdl, 0, 0xff108080, apply);
	} else {
		if (apply)
			hwreg->bg_color.dwval = 0;
		reg->bg_color.dwval = 0;
		de_bld_set_pipe_fcolor(hdl, 0, 0xff000000, apply);
	}

	bld_set_block_dirty(priv, BLD_REG_BLK_CTL, 1);
	bld_set_block_dirty(priv, BLD_REG_BLK_CK, 1);
	return 0;
}

static int de_bld_set_premul(struct de_bld_handle *hdl, unsigned int pipe_id, bool is_premul)
{
	struct de_bld_private *priv = hdl->private;
	struct bld_reg *reg = get_bld_reg(priv);
	if (is_premul) {
		reg->premul_ctl.dwval = SET_BITS(pipe_id, 1, reg->premul_ctl.dwval, 1);
	} else {
		reg->premul_ctl.dwval = SET_BITS(pipe_id, 1, reg->premul_ctl.dwval, 0);
	}
	bld_set_block_dirty(priv, BLD_REG_BLK_CTL, 1);
	return 0;
}

static int de_bld_en_pipe(struct de_bld_handle *hdl, unsigned int pipe_id, bool en)
{
	struct de_bld_private *priv = hdl->private;
	struct bld_reg *reg = get_bld_reg(priv);

	priv->debug[pipe_id].enable = en;

	if (en)
		reg->pipe.en.dwval |= (1 << (pipe_id + 8));
	else
		reg->pipe.en.dwval &= ~(1 << (pipe_id + 8));
	bld_set_block_dirty(priv, BLD_REG_BLK_ATTR, 1);
	return 0;
}

static int de_bld_set_route(struct de_bld_handle *hdl, unsigned int pipe_id, unsigned int port_id)
{
	struct de_bld_private *priv = hdl->private;
	struct bld_reg *reg = get_bld_reg(priv);
	u32 dwval = reg->rout_ctl.dwval;

	priv->debug[pipe_id].port_id = port_id;
	reg->rout_ctl.dwval = SET_BITS(pipe_id * 4, 4, dwval, port_id);

	bld_set_block_dirty(priv, BLD_REG_BLK_CTL, 1);
	return 0;
}

int de_bld_pipe_reset(struct de_bld_handle *hdl, unsigned int pipe_id, int port_id)
{
	struct de_bld_private *priv = hdl->private;
	struct bld_reg *reg = get_bld_reg(priv);
	u32 dwval;
	dwval = GET_BITS(pipe_id * 4, 4, reg->rout_ctl.dwval);
	/* only reset if this pipe own by request chn */
	if (port_id < 0 && port_id != -255) {
		DRM_ERROR("invalid port_id for pipe\n");
		return -1;
	}
	if (dwval == port_id || port_id == -255) {
		de_bld_set_route(hdl, pipe_id, 0xf);
		de_bld_en_pipe(hdl, pipe_id, 0);
	}
	return 0;
}

int de_bld_get_chn_mux_port(struct de_bld_handle *hdl, unsigned int chn_mode, bool is_video, unsigned int type_id)
{
	bool found = false;
	int port;
	int i, j;
	unsigned long id;
	type_id = BIT(type_id) << (is_video ? VIDEO_CHANNEL_ID_SHIFT : UI_CHANNEL_ID_SHIFT);
	for (i = 0; i < hdl->private->dsc->mode_cnt && !found; i++) {
		if (hdl->private->dsc->mode[i].mode_id != chn_mode &&
			    hdl->private->dsc->mode[i].mode_id != CHN_MODE_FIX)
			continue;
		for (j = 0; j < hdl->private->dsc->mode[i].channel_cnt && !found; j++) {
			id = hdl->private->dsc->mode[i].channel_id[j];
			found = type_id == id;
			port = j;
		}
	}
	return found ? port : -1;
}

static int de_bld_set_blend_mode(struct de_bld_handle *hdl, u32 bld_id,
	enum de_blend_mode mode)
{
	struct de_bld_private *priv = hdl->private;
	struct bld_reg *reg = get_bld_reg(priv);
	reg->blend_ctl[bld_id].dwval = mode;

	bld_set_block_dirty(priv, BLD_REG_BLK_CTL, 1);
	return 0;
}

int de_bld_pipe_set_attr(struct de_bld_handle *hdl, unsigned int pipe_id, unsigned int port_id, const struct drm_rect *rect, bool is_premul)
{
	struct de_bld_private *priv = hdl->private;
	struct bld_reg *reg = get_bld_reg(priv);
	u32 x = rect->x1, y = rect->y1;
	u32 w = drm_rect_width(rect);
	u32 h = drm_rect_height(rect);
	u32 dwval;

	memcpy(&priv->debug[pipe_id].crtc_pos, rect, sizeof(*rect));
	priv->debug[pipe_id].is_premult = is_premul;
	de_bld_set_route(hdl, pipe_id, port_id);
	de_bld_set_premul(hdl, pipe_id, is_premul);
	de_bld_en_pipe(hdl, pipe_id, 1);
	de_bld_set_blend_mode(hdl, pipe_id, DE_BLD_MODE_SRCOVER);
	dwval = (w ? ((w - 1) & 0x1FFF) : 0) |
		    (h ? (((h - 1) & 0x1FFF) << 16) : 0);
	reg->pipe.attr[pipe_id].in_size.dwval = dwval;

	dwval = (x & 0xFFFF)
		| ((y & 0xFFFF) << 16);
	reg->pipe.attr[pipe_id].in_coord.dwval = dwval;


	bld_set_block_dirty(priv, BLD_REG_BLK_ATTR, 1);
	return 0;
}

void dump_bld_state(struct drm_printer *p, struct de_bld_handle *hdl)
{
	int i;
	struct bld_debug_info *info;
	unsigned long base = (unsigned long)hdl->private->reg_blks[0].reg_addr;
	unsigned long de_base = (unsigned long)hdl->cinfo.de_reg_base;

	drm_printf(p, "blender@%8x: %sable\n", (unsigned int)(base - de_base), hdl->private->debug[0].enable ? "en" : "dis");
	if (hdl->private->debug[0].enable) {
		drm_printf(p, "\t pipe_id | enable | channel | premult |       crtc-pos      \n");
		drm_printf(p, "\t---------+--------+---------+---------+---------------------\n");
	}
	for (i = 0; i < BLD_PORT_MAX; i++) {
		info = &hdl->private->debug[i];
		if (info->enable) {
		drm_printf(p, "\t    %2d   |  %5s |   %2d    |  %5s  | %4dx%4d+%4d+%4d \n", i, info->enable ? "true" : "false",
			    info->port_id, info->is_premult ? "true" : "false", drm_rect_width(&info->crtc_pos),
			    drm_rect_height(&info->crtc_pos), info->crtc_pos.x1, info->crtc_pos.y1);
		}
	}
	if (hdl->private->debug[0].enable) {
		drm_printf(p, "\n\n");
	}
}

struct de_bld_handle *de_blender_create(struct module_create_info *info,  unsigned int chn_mode)
{
	int i;
	struct de_bld_handle *hdl;
	struct de_reg_block *block;
	struct de_reg_mem_info *reg_mem_info;
	struct de_bld_private *priv;
	u8 __iomem *reg_base;
	const struct de_bld_desc *dsc;
	bool chnnels_not_zero = false;

	dsc = get_bld_dsc(info);
	if (!dsc)
		return NULL;

	for (i = 0; i < dsc->mode_cnt; i++) {
		if (dsc->mode[i].mode_id != chn_mode &&
		    dsc->mode[i].mode_id != CHN_MODE_FIX)
			continue;

		if (dsc->mode[i].channel_cnt > 0) {
			chnnels_not_zero = true;
			break;
		}
	}
	if (!chnnels_not_zero) {
		DRM_INFO("[SUNXI-DE] chn_cfg_mode %d de%d chnnels num is zero\n", chn_mode, info->id);
		return NULL;
	}

	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	hdl->disp_reg_base = dsc->disp_base;
	memcpy(&hdl->cinfo, info, sizeof(*info));

	hdl->private->dsc = dsc;
	reg_base = info->de_reg_base + dsc->disp_base + dsc->bld_offset;
	reg_mem_info = &(hdl->private->reg_mem_info);
	priv = hdl->private;
	reg_mem_info = &(priv->reg_mem_info);

	reg_mem_info->size = sizeof(struct bld_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
		reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
		info->update_mode == RCQ_MODE);
	if (NULL == reg_mem_info->vir_addr) {
		DRM_ERROR("alloc bld[%d] mm fail!size=0x%x\n",
		     info->id, reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
	}

	block = &(priv->reg_blks[BLD_REG_BLK_ATTR]);
	block->phy_addr = reg_mem_info->phy_addr;
	block->vir_addr = reg_mem_info->vir_addr;
	block->size = 0x60;
	block->reg_addr = reg_base;

	block = &(priv->reg_blks[BLD_REG_BLK_CTL]);
	block->phy_addr = reg_mem_info->phy_addr + 0x80;
	block->vir_addr = reg_mem_info->vir_addr + 0x80;
	block->size = 0x24;
	block->reg_addr = reg_base + 0x80;

	block = &(priv->reg_blks[BLD_REG_BLK_CK]);
	block->phy_addr = reg_mem_info->phy_addr + 0xA0;
	block->vir_addr = reg_mem_info->vir_addr + 0xA0;
	block->size = 0x60;
	block->reg_addr = reg_base + 0xA0;

	priv->reg_blk_num = BLD_REG_BLK_NUM;

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(block[0]) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	return hdl;
}

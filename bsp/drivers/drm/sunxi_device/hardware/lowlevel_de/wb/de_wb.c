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
#include <linux/version.h>
#include <linux/dma-direction.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#else
#include <drm/drm_fb_dma_helper.h>
#include <drm/drm_gem_dma_helper.h>
#endif
#include <drm/drm_framebuffer.h>
#include <drm/drm_fourcc.h>

#include "de_csc.h"
#include "de_csc_table.h"
#include "de_wb_type.h"
#include "de_wb.h"
#include "sunxi_drm_gem.h"

#define LINE_BUF_LEN			(2048)
#define LOCFRACBIT			(18)
#define SCALERPHASE			(16)
#define DE_TOP_RTWB_OFFSET		(0x010000)
#define RTWB_WB_OFFSET			(0x01000)

enum {
	WB_REG_BLK_CTL = 0,
	WB_REG_BLK_COEFF0,
	WB_REG_BLK_COEFF1,
	WB_REG_BLK_NUM,
};

struct de_wb_private {
	/* wb regs can be update by any one of display, we used the current mux disp to update it*/
	unsigned int current_disp;
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[WB_REG_BLK_NUM];
	void *cdc_hdl;
};

const unsigned int wb_formats[] = {
	DRM_FORMAT_ARGB8888, DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGBA8888, DRM_FORMAT_BGRA8888,

	DRM_FORMAT_RGB888,   DRM_FORMAT_BGR888,

	DRM_FORMAT_YUV420,   DRM_FORMAT_YVU420,

	DRM_FORMAT_NV12,     DRM_FORMAT_NV21,
};

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

	blk->dirty = dirty;
	if (priv->reg_blks[blk_id].rcq_hd)
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
}

static u32 wb_convert_px_fmt_to_space(u32 px_fmt)
{
	switch (px_fmt) {
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
		return DE_FORMAT_SPACE_YUV;
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
	default:
		return DE_FORMAT_SPACE_RGB;
	}
}

static s32 de_wb_writeback_enable(struct de_wb_handle *handle)
{
	struct de_wb_private *priv = handle->private;
	struct wb_reg *reg = get_wb_reg(priv);

	reg->gctrl.bits.wb_en = 1;
	wb_set_block_dirty(priv, WB_REG_BLK_CTL, 1);
	return 0;
}

static s32 de_wb_set_base_para(struct de_wb_handle *handle, unsigned int in_w, unsigned int in_h, struct drm_framebuffer *out_fb)
{
	struct de_wb_private *priv = handle->private;
	struct wb_reg *reg = get_wb_reg(priv);
	struct wb_reg *hw_reg = get_wb_hw_reg(priv);
	struct sunxi_gem_object *sgem_obj;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	struct drm_gem_cma_object *gem;
#else
	struct drm_gem_dma_object *gem;
#endif
	unsigned long out_addr[3] = {0};
	unsigned long tmp_addr;

	u32 i;
	u32 out_fmt;
	u32 crop_x = 0, crop_y = 0, crop_w = in_w, crop_h = in_h;
	u32 out_window_w, out_window_h;
	u32 cs_out_w0 = 0, cs_out_h0 = 0, cs_out_w1 = 0, cs_out_h1 = 0;
	u32 fs_out_w0, fs_out_h0, fs_out_w1, fs_out_h1;
	u32 step_h, step_v;
	u32 v_intg, v_frac, h_intg, h_frac;
	u32 down_scale_y, down_scale_c;
	u32 pitch[2] = {0};

	out_fmt = out_fb->format->format;
	out_window_w = out_fb->width;
	out_window_h = out_fb->height;

	reg->gctrl.dwval = 0x10000000;
	hw_reg->gctrl.dwval = 0x10000000;

	/* input size */
	reg->size.dwval = (in_w - 1) | ((in_h - 1) << 16);
	/* input crop window */
	reg->crop_coord.dwval = crop_x | ((crop_y) << 16);
	reg->crop_size.dwval = (crop_w - 1) | ((crop_h - 1) << 16);
	reg->sftm.bits.sftm_vs = 0x20; /*default*/

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	gem = drm_fb_cma_get_gem_obj(out_fb, 0);
	if (gem) {
		out_addr[0] = (unsigned long)(gem->paddr) + out_fb->offsets[0];
		pitch[0] = out_fb->pitches[0];
		// change sunxi_gem_object 's direction, ensure cache coherence
		// for data that might be read from ddr by cpu
		sgem_obj = to_sunxi_gem_obj(gem);
		sgem_obj->dir = DMA_FROM_DEVICE;
	}

	gem = drm_fb_cma_get_gem_obj(out_fb, 1);
	if (gem) {
		out_addr[1] = (u64)(gem->paddr) + out_fb->offsets[1];
		pitch[1] = out_fb->pitches[1];
		// change sunxi_gem_object 's direction, ensure cache coherence
		// for data that might be read from ddr by cpu
		sgem_obj = to_sunxi_gem_obj(gem);
		sgem_obj->dir = DMA_FROM_DEVICE;
	}

	gem = drm_fb_cma_get_gem_obj(out_fb, 2);
	if (gem) {
		out_addr[2] = (u64)(gem->paddr) + out_fb->offsets[2];
		// change sunxi_gem_object 's direction, ensure cache coherence
		// for data that might be read from ddr by cpu
		sgem_obj = to_sunxi_gem_obj(gem);
		sgem_obj->dir = DMA_FROM_DEVICE;
	}
#else
	gem = drm_fb_dma_get_gem_obj(out_fb, 0);
	if (gem) {
		out_addr[0] = (unsigned long)(gem->dma_addr) + out_fb->offsets[0];
		pitch[0] = out_fb->pitches[0];
		// change sunxi_gem_object 's direction, ensure cache coherence
		// for data that might be read from ddr by cpu
		sgem_obj = to_sunxi_gem_obj(gem);
		sgem_obj->dir = DMA_FROM_DEVICE;
	}

	gem = drm_fb_dma_get_gem_obj(out_fb, 1);
	if (gem) {
		out_addr[1] = (u64)(gem->dma_addr) + out_fb->offsets[1];
		pitch[1] = out_fb->pitches[1];
		// change sunxi_gem_object 's direction, ensure cache coherence
		// for data that might be read from ddr by cpu
		sgem_obj = to_sunxi_gem_obj(gem);
		sgem_obj->dir = DMA_FROM_DEVICE;
	}

	gem = drm_fb_dma_get_gem_obj(out_fb, 2);
	if (gem) {
		out_addr[2] = (u64)(gem->dma_addr) + out_fb->offsets[2];
		// change sunxi_gem_object 's direction, ensure cache coherence
		// for data that might be read from ddr by cpu
		sgem_obj = to_sunxi_gem_obj(gem);
		sgem_obj->dir = DMA_FROM_DEVICE;
	}
#endif

	switch (out_fmt) {
	case DRM_FORMAT_ARGB8888:
		reg->fmt.dwval = WB_FORMAT_ARGB_8888;
		break;
	case DRM_FORMAT_ABGR8888:
		reg->fmt.dwval = WB_FORMAT_ABGR_8888;
		break;
	case DRM_FORMAT_RGBA8888:
		reg->fmt.dwval = WB_FORMAT_RGBA_8888;
		break;
	case DRM_FORMAT_BGRA8888:
		reg->fmt.dwval = WB_FORMAT_BGRA_8888;
		break;
	case DRM_FORMAT_RGB888:
		reg->fmt.dwval = WB_FORMAT_RGB_888;
		break;
	case DRM_FORMAT_BGR888:
		reg->fmt.dwval = WB_FORMAT_BGR_888;
		break;
	case DRM_FORMAT_NV21:
		reg->fmt.dwval = WB_FORMAT_YUV420_SP_VUVU;
		break;
	case DRM_FORMAT_NV12:
		reg->fmt.dwval = WB_FORMAT_YUV420_SP_UVUV;
		break;
	case DRM_FORMAT_YUV420:
		reg->fmt.dwval = WB_FORMAT_YUV420_P;
		break;
	case DRM_FORMAT_YVU420:
		reg->fmt.dwval = WB_FORMAT_YUV420_P;
		tmp_addr = out_addr[1];
		out_addr[1] = out_addr[2];
		out_addr[2] = tmp_addr;
		break;
	default:
		DRM_ERROR("unknow out fmt %d\n", out_fmt);
		return -1;
	}

	reg->wb_pitch0.dwval = pitch[0];
	reg->wb_pitch1.dwval = pitch[1];
	reg->wb_addr_a0.dwval = out_addr[0];
	reg->wb_addr_a1.dwval = out_addr[1];
	reg->wb_addr_a2.dwval = out_addr[2];

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
	    ((out_fmt == DRM_FORMAT_NV12)
	    | (out_fmt == DRM_FORMAT_NV21)
	    | (out_fmt == DRM_FORMAT_YUV420)
	    | (out_fmt == DRM_FORMAT_YVU420)) ?
	    (out_window_w >> 1) : out_window_w;
	fs_out_h0 = out_window_h;
	fs_out_h1 =
	    ((out_fmt == DRM_FORMAT_NV12)
	    | (out_fmt == DRM_FORMAT_NV21)
	    | (out_fmt == DRM_FORMAT_YUV420)
	    | (out_fmt == DRM_FORMAT_YVU420)) ?
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
		wb_set_block_dirty(priv, WB_REG_BLK_COEFF0, 1);
		wb_set_block_dirty(priv, WB_REG_BLK_COEFF1, 1);
	}
	reg->fs_insize.dwval =
	    (cs_out_w0 - 1) | ((cs_out_h0 - 1) << 16);
	reg->fs_outsize.dwval =
	    (out_window_w - 1) | ((out_window_h - 1) << 16);

	wb_set_block_dirty(priv, WB_REG_BLK_CTL, 1);

	return 0;
}

static s32 de_wb_set_csc_para_inner(struct de_wb_handle *handle, struct de_csc_info *icsc_info, struct de_csc_info *ocsc_info)
{
	u32 *csc_coeff = NULL;
	struct de_wb_private *priv = handle->private;
	struct wb_reg *reg = get_wb_reg(priv);

	/* FIXME add cdc proc */

/*	if (priv->cdc_hdl != NULL) {
		if (icsc_info.px_fmt_space == DE_FORMAT_SPACE_YUV) {
			ocsc_info->px_fmt_space = icsc_info->px_fmt_space;
			ocsc_info->color_range = icsc_info->color_range;
			ocsc_info->color_space = DE_COLOR_SPACE_BT709;
			ocsc_info->eotf = DE_EOTF_GAMMA22;
		} else {
			memcpy((void *)&ocsc_info,
				(void *)&icsc_info, sizeof(ocsc_info));
		}
		//fixme: update reg right now at without rcq
		de_cdc_set_para(priv->cdc_hdl, &icsc_info, &ocsc_info);
		memcpy((void *)&icsc_info,
			(void *)&ocsc_info, sizeof(ocsc_info));
	}*/

	de_csc_coeff_calc(icsc_info, ocsc_info, &csc_coeff);
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

		wb_set_block_dirty(priv, WB_REG_BLK_CTL, 1);
	}

	return 0;
}

static s32 de_wb_set_csc_para(struct de_wb_handle *handle, struct de_csc_info *icsc_info, struct drm_framebuffer *out_fb)
{
	struct de_csc_info ocsc_info;

	/* TODO add user out csc config api */
	ocsc_info.px_fmt_space = wb_convert_px_fmt_to_space(out_fb->format->format);
	ocsc_info.eotf = DE_EOTF_GAMMA22;

	if ((out_fb->width <= 736) && (out_fb->height <= 576))
		ocsc_info.color_space = DE_COLOR_SPACE_BT601;
	else
		ocsc_info.color_space = DE_COLOR_SPACE_BT709;

	if (ocsc_info.px_fmt_space == DE_FORMAT_SPACE_YUV)
		ocsc_info.color_range = DE_COLOR_RANGE_16_235;
	else
		ocsc_info.color_range = DE_COLOR_RANGE_0_255;

	return de_wb_set_csc_para_inner(handle, icsc_info, &ocsc_info);
}

static s32 de_wb_stop(struct de_wb_handle *handle)
{
	struct de_wb_private *priv = handle->private;
	struct wb_reg *reg = get_wb_reg(priv);
	struct wb_reg *hw_reg = get_wb_hw_reg(priv);

	wb_set_block_dirty(priv, WB_REG_BLK_CTL, 0);
	reg->gctrl.dwval = 0x20000010;
	hw_reg->gctrl.dwval = 0x20000010; /* stop now */

	return 0;
}

void force_update(struct de_wb_handle *handle)
{
	int i, j;
	struct de_reg_block *blk;
	u32 *val;
	u32 *tmp;
	for (i = 0; i < handle->block_num; i++) {
		blk = handle->block[i];
		if (blk->dirty) {
			for (j = 0; j < blk->size; j++) {
				val = (u32 *)(blk->vir_addr);
				tmp = (u32 *)(blk->reg_addr);
				writel(val[j], &tmp[j]);
			}
			printk("update dirty\n");
		}
	}
}

int de_wb_apply(struct de_wb_handle *handle, struct wb_in_config *in, struct drm_framebuffer *out_fb)
{
	if (out_fb == NULL) {
		DRM_INFO("[SUNXI-CRTC]%s stop\n", __func__);
		return de_wb_stop(handle);
	}

	if (in->width < out_fb->width || in->height < out_fb->height || out_fb->width > LINE_BUF_LEN) {
		DRM_ERROR("invlid wb size %d %d %d %d\n",
		    in->width, out_fb->width, in->height, out_fb->height);
	}
	de_wb_set_base_para(handle, in->width, in->height, out_fb);

	de_wb_set_csc_para(handle, &in->csc_info, out_fb);
	de_wb_writeback_enable(handle);
	//force_update(handle);
	DRM_INFO("[SUNXI-CRTC]%s ok\n", __func__);
	return 0;
}

struct de_wb_handle *de_wb_create(struct module_create_info *info)
{
	int i;
	u8 __iomem *reg_base;
	struct de_wb_handle *hdl;
	struct de_reg_block *blk;
	struct de_wb_private *priv;
	struct de_reg_mem_info *reg_mem_info;

	reg_base = (u8 __iomem *)(info->de_reg_base + DE_TOP_RTWB_OFFSET + RTWB_WB_OFFSET);
	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, info, sizeof(*info));
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	priv = hdl->private;
	reg_mem_info = &(hdl->private->reg_mem_info);
	reg_mem_info->size = sizeof(struct wb_reg);
	reg_mem_info->vir_addr = (u8 *)sunxi_de_reg_buffer_alloc(hdl->cinfo.de,
				    reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				    info->update_mode == RCQ_MODE);

	if (reg_mem_info->vir_addr == NULL) {
		DRM_ERROR("alloc wb mm fail!size=0x%x\n", reg_mem_info->size);
		return ERR_PTR(-ENOMEM);
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

	priv->reg_blk_num = WB_REG_BLK_NUM;

/*  TODO add cdc
	reg_base = (u8 __iomem *)(para->reg_base[DISP_MOD_DE]
		+ DE_TOP_RTWB_OFFSET + RTWB_CDC_OFFSET);
	priv->cdc_hdl = de_cdc_create(reg_base, rcq_used, 1);
	if (priv->cdc_hdl == NULL) {
		DE_WRN("de_cdc_create for wb failed\n");
	}
*/

	hdl->block_num = priv->reg_blk_num;
	hdl->block = kmalloc(sizeof(*blk) * hdl->block_num, GFP_KERNEL | __GFP_ZERO);
	for (i = 0; i < hdl->private->reg_blk_num; i++)
		hdl->block[i] = &priv->reg_blks[i];

	return hdl;
}


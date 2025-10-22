/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "g2d_debug.h"

void dump_g2d_image_enh_info(const g2d_image_enh *p_image)
{
	G2D_DBG("image.bbuff           :%d\n", p_image->bbuff);
	G2D_DBG("image.color           :0x%x\n", p_image->color);
	G2D_DBG("image.use_phy_addr    :%d\n", p_image->use_phy_addr);
	G2D_DBG("image.fd              :%d\n", p_image->fd);
	G2D_DBG("image.laddr[0]        :0x%x\n", p_image->laddr[0]);
	G2D_DBG("image.laddr[1]        :0x%x\n", p_image->laddr[1]);
	G2D_DBG("image.laddr[2]        :0x%x\n", p_image->laddr[2]);
	G2D_DBG("image.haddr[0]        :0x%x\n", p_image->haddr[0]);
	G2D_DBG("image.haddr[1]        :0x%x\n", p_image->haddr[1]);
	G2D_DBG("image.haddr[2]        :0x%x\n", p_image->haddr[2]);
	G2D_DBG("image.format          :0x%x\n", p_image->format);
	G2D_DBG("image.width           :%d\n", p_image->width);
	G2D_DBG("image.height          :%d\n", p_image->height);
	G2D_DBG("image.align[0]        :%d\n", p_image->align[0]);
	G2D_DBG("image.align[1]        :%d\n", p_image->align[1]);
	G2D_DBG("image.align[2]        :%d\n", p_image->align[2]);
	G2D_DBG("image.clip_rect.x     :%d\n", p_image->clip_rect.x);
	G2D_DBG("image.clip_rect.y     :%d\n", p_image->clip_rect.y);
	G2D_DBG("image.clip_rect.w     :%d\n", p_image->clip_rect.w);
	G2D_DBG("image.clip_rect.h     :%d\n", p_image->clip_rect.h);
	G2D_DBG("image.resize.w        :%d\n", p_image->resize.w);
	G2D_DBG("image.resize.h        :%d\n", p_image->resize.h);
	G2D_DBG("image.coor.x          :%d\n", p_image->coor.x);
	G2D_DBG("image.coor.y          :%d\n", p_image->coor.y);
	G2D_DBG("image.gamut           :%d\n", p_image->gamut);
	G2D_DBG("image.alpha           :%d\n", p_image->alpha);
	G2D_DBG("image.bpremul         :%d\n", p_image->bpremul);
	G2D_DBG("image.mode            :%d\n", p_image->mode);
	G2D_DBG("image.color_range     :%d\n", p_image->color_range);
}

void dump_g2d_ck_info(const g2d_ck *g2d_ck)
{
	G2D_DBG("g2d_ck.match_rule     :%d\n", ((g2d_ck->match_rule) ? 1 : 0));
	G2D_DBG("g2d_ck.max_color      :%d\n", g2d_ck->max_color);
	G2D_DBG("g2d_ck.min_color      :%d\n", g2d_ck->min_color);
}

void dump_g2d_bld_info(const g2d_bld *g2d_bld_para)
{
	G2D_DBG("bld_cmd_flag:         :0x%x\n", g2d_bld_para->bld_cmd);
	G2D_DBG("dst_image para: \n");
	dump_g2d_image_enh_info(&(g2d_bld_para->dst_image));
	G2D_DBG("src0_image para: \n");
	dump_g2d_image_enh_info(&(g2d_bld_para->src_image[0]));
	G2D_DBG("src1_image para: \n");
	dump_g2d_image_enh_info(&(g2d_bld_para->src_image[1]));
}

void dump_mixer_para_info(const mixer_para *mixer_paras)
{
	G2D_DBG("op_flag:              :0x%x\n", mixer_paras->op_flag);
	G2D_DBG("flag_h:               :0x%x\n", mixer_paras->flag_h);
	G2D_DBG("back_flag:            :0x%x\n", mixer_paras->back_flag);
	G2D_DBG("fore_flag:            :0x%x\n", mixer_paras->fore_flag);
	G2D_DBG("bld_cmd:              :0x%x\n", mixer_paras->bld_cmd);
	G2D_DBG("src_image para: \n");
	dump_g2d_image_enh_info(&(mixer_paras->src_image_h));
	G2D_DBG("ptn_image para: \n");
	dump_g2d_image_enh_info(&(mixer_paras->ptn_image_h));
	G2D_DBG("mask_image para: \n");
	dump_g2d_image_enh_info(&(mixer_paras->mask_image_h));
	G2D_DBG("dst_image para: \n");
	dump_g2d_image_enh_info(&(mixer_paras->dst_image_h));
	G2D_DBG("g2d_ck para: \n");
	dump_g2d_ck_info(&(mixer_paras->ck_para));
}

void dump_g2d_maskblt_info(const g2d_maskblt *g2d_maskblt_para)
{
	G2D_DBG("back_flag:            :0x%x\n", g2d_maskblt_para->back_flag);
	G2D_DBG("fore_flag:            :0x%x\n", g2d_maskblt_para->fore_flag);
	G2D_DBG("src_image para: \n");
	dump_g2d_image_enh_info(&(g2d_maskblt_para->src_image_h));
	G2D_DBG("ptn_image para: \n");
	dump_g2d_image_enh_info(&(g2d_maskblt_para->ptn_image_h));
	G2D_DBG("mask_image para: \n");
	dump_g2d_image_enh_info(&(g2d_maskblt_para->mask_image_h));
	G2D_DBG("dst_image para: \n");
	dump_g2d_image_enh_info(&(g2d_maskblt_para->dst_image_h));
}

void dump_g2d_blt_h_info(const g2d_blt_h *g2d_blt_h_para)
{
	G2D_DBG("flag_h:               :0x%x\n", g2d_blt_h_para->flag_h);
	G2D_DBG("src_image para: \n");
	dump_g2d_image_enh_info(&(g2d_blt_h_para->src_image_h));
	G2D_DBG("dst_image para: \n");
	dump_g2d_image_enh_info(&(g2d_blt_h_para->dst_image_h));
}

void dump_g2d_fillrect_h_info(const g2d_fillrect_h *g2d_fillrect_h_para)
{
	G2D_DBG("dst_image para: \n");
	dump_g2d_image_enh_info(&(g2d_fillrect_h_para->dst_image_h));
}

void dump_g2d_lbc_rot_info(const g2d_lbc_rot *g2d_lbc_rot_para)
{
	G2D_DBG("g2d_blt_h: \n");
	dump_g2d_blt_h_info(&(g2d_lbc_rot_para->blt));
	G2D_DBG("lbc_cmp_ratio         :%d\n", g2d_lbc_rot_para->lbc_cmp_ratio);
	G2D_DBG("enc_is_lossy          :%d\n", g2d_lbc_rot_para->enc_is_lossy);
	G2D_DBG("dec_is_lossy          :%d\n", g2d_lbc_rot_para->dec_is_lossy);
}


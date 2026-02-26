/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *  File name   :       de_enhance.c
 *
 *  Description :       display engine 2.0 enhance basic function definition
 *
 *  History     :       2014/04/29  vito cheng  v0.1  Initial version
 *
 *****************************************************************************/
#include "de_rtmx.h"
#include "de_top.h"
#include "de_enhance.h"
#include "de_feat.h"
#include "de_vep_table.h"
#include "de_vsu.h"
#include "de_cdc.h"

#define de_readl(addr) readl((void __iomem *)(addr))
#define de_writel(val, addr) writel(val, (void __iomem *)(addr))

/*
 * only ONE parameters for one screen,
 * and all VEPs in this screen use SAME parameters
 */
#define ONE_SCREEN_ONE_PARA

struct de_enhance_private {
	struct de_reg_mem_info reg_mem_info;
};

static struct de_enhance_private enhance_priv[DE_NUM][VI_CHN_NUM];

/* enhance configuration */
static struct vep_config_info *g_cfg;
/* enhance channel layer information */
static struct disp_enhance_chn_info *g_chn_info[DE_NUM][VI_CHN_NUM];
/* enhance channel module information */
static struct vep_para *g_para[DE_NUM][VI_CHN_NUM];
/* enhance algorithm variable */
static struct vep_alg_var *g_alg[DE_NUM][VI_CHN_NUM];

static s32 de_enhance_init_para(u32 disp, u32 chn)
{

	if (g_cfg->dci_exist[disp][chn])
		de_dci_init_para(disp, chn);
	if (g_cfg->peaking_exist[disp][chn])
		de_vsu_init_peaking_para(disp, chn);
	if (g_cfg->fcm_exist[disp][chn])
		de_fcm_init_para(disp, chn);
	if (g_cfg->sharp_exist[disp][chn])
		de_sharp_init_para(disp, chn);

	/* process dep */
	if (chn == 0) {
		if (de_feat_is_support_gamma(disp))
			de_gamma_init_para(disp);
		if (de_feat_is_support_dither(disp))
			de_dither_init_para(disp);
		if (de_feat_is_support_deband(disp))
			de_deband_init_para(disp);
	}
	return 0;
}

static s32 de_enhance_set_size(u32 disp, u32 chn)
{
#define DEMO_SPEED 200
	struct de_rect_o tmp_win; /* bld size window */
	struct de_rect_o tmp_win2; /* ovl size window */
	struct de_rect_o tmp_win3; /* screen size window */
	struct disp_rectsz size; /* bld size */
	struct disp_rectsz size2; /* ovl size */
	struct disp_rectsz size3; /* screen size */
	static u32 percent;
	static u32 opposite;
	enum de_format_space chn_fmt;
	u32 demo_enable;
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);

	demo_enable = g_para[disp][chn]->demo_enable;
	size.width = g_chn_info[disp][chn]->bld_size.width;
	size.height = g_chn_info[disp][chn]->bld_size.height;
	size2.width = g_chn_info[disp][chn]->ovl_size.width;
	size2.height = g_chn_info[disp][chn]->ovl_size.height;
	size3.width = ctx->output.scn_width;
	size3.height = ctx->output.scn_height;
	chn_fmt = ctx->chn_info[chn].px_fmt_space;

	tmp_win.x = 0;
	tmp_win.y = 0;
	tmp_win2.x = 0;
	tmp_win2.y = 0;
	tmp_win3.x = 0;
	tmp_win3.y = 0;

	DE_INFO("disp=%d, chn=%d, demo_enable=%d, size=%d,%d, size2=%d,%d\n", disp, chn, demo_enable,
		   size.width, size.height, size2.width, size2.height);
	/* demo mode: rgb half, yuv scroll */
	if (demo_enable) {
		if (chn_fmt == DE_FORMAT_SPACE_YUV) {
			if (!opposite)
				percent >= DEMO_SPEED ? opposite = 1 : percent++;
			else
				percent <= 0 ? opposite = 0 : percent--;

			if (size.width > size.height) {
				tmp_win.w = percent ?  (size.width * percent / DEMO_SPEED) : 1;
				tmp_win.h = size.height;
				/* ovl window follow bld window */
				tmp_win2.w = percent ?  (size2.width * percent / DEMO_SPEED) : 1;
				tmp_win2.h = size2.height;
				/* screen window */
				tmp_win3.w = percent ?  (size3.width * percent / DEMO_SPEED) : 1;
				tmp_win3.h = size3.height;
			} else {
				tmp_win.w = size.width;
				tmp_win.h = percent ?  (size.height * percent / DEMO_SPEED) : 1;
				/* ovl window follow bld window */
				tmp_win2.w = size2.width;
				tmp_win2.h = percent ?  (size2.height * percent / DEMO_SPEED) : 1;
				/* screen window */
				tmp_win3.w = size3.width;
				tmp_win3.h = percent ?  (size3.height * percent / DEMO_SPEED) : 1;
			}
		} else {
			if (size.width > size.height) {
				tmp_win.w = size.width >> 1;
				tmp_win.h = size.height;
				/* ovl window follow bld window */
				tmp_win2.w = size2.width >> 1;
				tmp_win2.h = size2.height;
				/* screen window */
				tmp_win3.w = size3.width >> 1;
				tmp_win3.h = size3.height;
			} else {
				tmp_win.w = size.width;
				tmp_win.h = size.height >> 1;
				/* ovl window follow bld window */
				tmp_win2.w = size2.width;
				tmp_win2.h = size2.height >> 1;
				/* screen window */
				tmp_win3.w = size3.width;
				tmp_win3.h = size3.height >> 1;
			}
		}
	} else {
		tmp_win.w = size.width;
		tmp_win.h = size.height;
		tmp_win2.w = size2.width;
		tmp_win2.h = size2.height;
		tmp_win3.w = size3.width;
		tmp_win3.h = size3.height;
	}

	/* three vep here, snr handle by others*/
	/* dci */
	if (g_cfg->dci_exist[disp][chn]) {
		de_dci_set_size(disp, chn, size.width, size.height);
		de_dci_set_window(disp, chn, demo_enable, tmp_win);
	}

	/* fcm */
	if (g_cfg->fcm_exist[disp][chn]) {
		de_fcm_set_size(disp, chn, size.width, size.height);
		de_fcm_set_window(disp, chn, demo_enable, tmp_win);
	}

	/* asu peaking */
	if (g_cfg->peaking_exist[disp][chn]) {
		de_vsu_set_peaking_window(disp, chn, demo_enable, tmp_win);
	}

	/* sharp */
	if (g_cfg->sharp_exist[disp][chn]) {
		de_sharp_set_size(disp, chn, size.width, size.height);
		de_sharp_set_window(disp, chn, demo_enable, tmp_win);
	}

	/* process dep */
	if (chn == 0) {
		/* gamma */
		if (de_feat_is_support_gamma(disp)) {
			de_gamma_set_size(disp, size3.width, size3.height);
			de_gamma_set_window(disp, demo_enable, tmp_win3);
		}
		/* dither */
		if (de_feat_is_support_dither(disp)) {
			de_dither_set_size(disp, size3.width, size3.height);
		}
		/* deband */
		if (de_feat_is_support_deband(disp)) {
			de_deband_set_size(disp, size3.width, size3.height);
			de_deband_set_window(disp, demo_enable, tmp_win3);
		}
	}

	return 0;
}

static s32 de_enhance_apply_core(u32 disp)
{
	s32 chn, chno;
	u32 other_dirty;
	u32 dev_type;
	u32 fmt;

	chno = de_feat_get_num_vi_chns(disp);
	other_dirty = ENH_SIZE_DIRTY | ENH_FORMAT_DIRTY | ENH_ENABLE_DIRTY |
		      ENH_MODE_DIRTY;
	for (chn = 0; chn < chno; chn++) {
		if ((de_feat_is_support_vep_by_chn(disp, chn) <= 0))
			continue;
		fmt = g_para[disp][chn]->fmt;
		dev_type = g_para[disp][chn]->dev_type;

		/* 0. initial parameters */
		if (g_para[disp][chn]->flags & ENH_INIT_DIRTY)
			de_enhance_init_para(disp, chn);

		/* 1. size and demo window reg */
		if ((g_para[disp][chn]->flags &
		    (ENH_SIZE_DIRTY | ENH_FORMAT_DIRTY | ENH_MODE_DIRTY)) ||
		    g_para[disp][chn]->demo_enable)
			de_enhance_set_size(disp, chn);

		/* 2. module parameters */
		/*if ((g_para[disp][chn]->flags &
		     (other_dirty | ENH_CONTRAST_DIRTY | ENH_BYPASS_DIRTY)) &&
		    g_cfg->dci_exist[disp][chn])
			de_dci_info2para(disp, chn, fmt, dev_type,
					 &g_para[disp][chn]->dci_para,
					 g_para[disp][chn]->bypass);

		if ((g_para[disp][chn]->flags &
		     (other_dirty | ENH_BRIGHT_DIRTY | ENH_BYPASS_DIRTY))) {
			de_gamma_info2para(disp, fmt, dev_type,
					   &g_para[disp][chn]->gamma_para,
					   g_para[disp][chn]->bypass);
		}

		if (g_para[disp][chn]->flags &
		    (other_dirty | ENH_DETAIL_DIRTY | ENH_BYPASS_DIRTY)) {
			if (g_cfg->peaking_exist[disp][chn])
				de_vsu_set_peaking_para(
					disp, chn, fmt, dev_type,
					&g_para[disp][chn]->peaking_para,
					g_para[disp][chn]->bypass);
			if (g_cfg->sharp_exist[disp][chn])
				de_sharp_info2para(
					disp, chn, fmt, dev_type,
					&g_para[disp][chn]->sharp_para,
					g_para[disp][chn]->bypass);
		}

		if (g_para[disp][chn]->flags &
		    (other_dirty | ENH_EDGE_DIRTY | ENH_BYPASS_DIRTY))
			if (g_cfg->sharp_exist[disp][chn])
				de_sharp_info2para(
					disp, chn, fmt, dev_type,
					&g_para[disp][chn]->sharp_para,
					g_para[disp][chn]->bypass);

		if (g_para[disp][chn]->flags &
		    (ENH_FORMAT_DIRTY | ENH_ENABLE_DIRTY | ENH_MODE_DIRTY |
		     ENH_SAT_DIRTY | ENH_BYPASS_DIRTY))
			if (g_cfg->fcm_exist[disp][chn])
				de_fcm_info2para(disp, chn, fmt, dev_type,
						 &g_para[disp][chn]->fcm_para,
						 g_para[disp][chn]->bypass);

		if (g_para[disp][chn]->flags &
		    (ENH_FORMAT_DIRTY | ENH_ENABLE_DIRTY | ENH_MODE_DIRTY |
		     ENH_BYPASS_DIRTY))
			de_deband_info2para(disp, fmt, dev_type,
					    g_para[disp][chn]->bypass);*/

		/* clear dirty in g_config */
		g_para[disp][chn]->flags = 0x0;
	}

	return 0;
}

s32 de_enhance_layer_apply(u32 disp, struct disp_enhance_chn_info *info)
{
	s32 chn, chno, i, layno;
	s32 feid; /* first_enable_layer_id */
	u32 overlay_enable = 0;
	u32 format = 0xff;
	struct disp_enhance_chn_info *p_info;
	u32 bypass;
	enum disp_enhance_dirty_flags flags = { 0 };

	chno = de_feat_get_num_vi_chns(disp);

	DE_INFO("ovl_size=<%d,%d>, bld_size=<%d,%d>\n",
	      info->ovl_size.width, info->ovl_size.height, info->bld_size.width,
	      info->bld_size.height);

	p_info = kmalloc(sizeof(struct disp_enhance_chn_info),
			 GFP_KERNEL | __GFP_ZERO);
	if (p_info == NULL) {
		DE_WARN("malloc p_info memory fail! size=0x%x\n",
		      (u32)sizeof(struct disp_enhance_chn_info));
		return -1;
	}

	for (chn = 0; chn < chno; chn++) {
		if ((de_feat_is_support_vep_by_chn(disp, chn) <= 0))
			continue;
		memcpy(p_info, g_chn_info[disp][chn],
		       sizeof(struct disp_enhance_chn_info));
		/* ENABLE DIRTY */
		layno = de_feat_get_num_layers_by_chn(disp, chn);
		feid = -1;
		for (i = 0; i < layno; i++) {
			if (info[chn].layer_info[i].en !=
			    p_info->layer_info[i].en)
				flags |= ENH_ENABLE_DIRTY;

			if (info[chn].layer_info[i].en) {
				overlay_enable |= 0x1;
				if (feid == -1)
					feid = i;
			}
		}

		/* SIZE DIRTY */
		if (((info[chn].ovl_size.width != p_info->ovl_size.width) ||
		     (info[chn].ovl_size.height != p_info->ovl_size.height) ||
		     (info[chn].bld_size.width != p_info->bld_size.width) ||
		     (info[chn].bld_size.height != p_info->bld_size.height)) &&
		    (overlay_enable == 0x1)) {
			flags |= ENH_SIZE_DIRTY;
		} else {
			for (i = 0; i < layno; i++) {
				if (info[chn].layer_info[i].en &&
				    ((p_info->layer_info[i].fb_size.width !=
				      info[chn].layer_info[i].fb_size.width) ||
				     (p_info->layer_info[i].fb_size.height !=
				      info[chn].layer_info[i].fb_size.height) ||
				     (p_info->layer_info[i].fb_crop.x !=
				      info[chn].layer_info[i].fb_crop.x) ||
				     (p_info->layer_info[i].fb_crop.y !=
				      info[chn].layer_info[i].fb_crop.y)))
					flags |= ENH_SIZE_DIRTY;
			}
		}

		/* FORMAT DIRTY */
		for (i = 0; i < layno; i++) {
			if (info[chn].layer_info[i].en &&
			    (p_info->layer_info[i].format !=
			     info[chn].layer_info[i].format)) {
				flags |= ENH_FORMAT_DIRTY;
				format = (info[chn].layer_info[i].format >=
					  DE_FORMAT_YUV444_I_AYUV) ?
						 0 :
						 1;
				break;
			}
		}

		/* UPDATE g_chn_info */
		if (flags &
		    (ENH_ENABLE_DIRTY | ENH_SIZE_DIRTY | ENH_FORMAT_DIRTY))
			memcpy(g_chn_info[disp][chn], &info[chn],
			       sizeof(struct disp_enhance_chn_info));

		/* UPDATE g_para->foramt */
		if (flags & ENH_FORMAT_DIRTY) {
			g_para[disp][chn]->fmt = format;
			if (g_cfg->dci_exist[disp][chn])
				de_dci_enable(disp, chn, format == 0 ? 1 : 0);
			if (g_cfg->sharp_exist[disp][chn])
				de_sharp_enable(disp, chn, format == 0 ? 1 : 0);
		}

		/* UPDATE g_para->bypass */
		/* Old bypass */
		bypass = g_para[disp][chn]->bypass;
		if (flags & ENH_SIZE_DIRTY) {
			if (g_chn_info[disp][chn]->ovl_size.width <
				    ENHANCE_MIN_OVL_WIDTH ||
			    g_chn_info[disp][chn]->ovl_size.height <
				    ENHANCE_MIN_OVL_HEIGHT ||
			    g_chn_info[disp][chn]->bld_size.width <
				    ENHANCE_MIN_BLD_WIDTH ||
			    g_chn_info[disp][chn]->bld_size.height <
				    ENHANCE_MIN_BLD_HEIGHT)
				g_para[disp][chn]->bypass |= SIZE_BYPASS;
			else
				g_para[disp][chn]->bypass &= SIZE_BYPASS_MASK;
		}

		if (flags & ENH_ENABLE_DIRTY) {
			if (!overlay_enable)
				g_para[disp][chn]->bypass |= LAYER_BYPASS;
			else
				g_para[disp][chn]->bypass &= LAYER_BYPASS_MASK;
		}

		/* BYPASS_DIRTY */
		if (bypass != g_para[disp][chn]->bypass)
			flags |= ENH_BYPASS_DIRTY;

		g_para[disp][chn]->flags = (enum disp_manager_dirty_flags)flags;
	}

	de_enhance_apply_core(disp);
	kfree(p_info);
	return 0;
}

s32 de_enhance_apply(u32 disp, struct disp_enhance_config *config)
{
	s32 chn, chno;
	u32 bypass;

	chno = de_feat_get_num_vi_chns(disp);

	for (chn = 0; chn < chno; chn++) {
		if ((de_feat_is_support_vep_by_chn(disp, chn) <= 0))
			continue;
		/* copy dirty to g_para->flags */
		g_para[disp][chn]->flags |= (config[0].flags & ENH_USER_DIRTY);
		bypass = g_para[disp][chn]->bypass;
		if (config[0].flags & ENH_MODE_DIRTY) {
			/* enhance_mode */
			if (((config[0].info.mode & 0xffff0000) >> 16) == 2) {
				g_para[disp][chn]->demo_enable = config->info.demo_enable;
				g_para[disp][chn]->bypass &= USER_BYPASS_MASK;
			} else if (((config[0].info.mode & 0xffff0000) >> 16) == 1) {
				g_para[disp][chn]->demo_enable = config->info.demo_enable;
				g_para[disp][chn]->bypass &= USER_BYPASS_MASK;
			} else {
				g_para[disp][chn]->demo_enable = config->info.demo_enable;
				g_para[disp][chn]->bypass |= USER_BYPASS;
			}
			/* dev_type */
			g_para[disp][chn]->dev_type =
				config[0].info.mode & 0x0000ffff;

			if (bypass != g_para[disp][chn]->bypass)
				g_para[disp][chn]->flags |= ENH_BYPASS_DIRTY;
		}

		if (config[0].flags & ENH_BRIGHT_DIRTY) {
			g_para[disp][chn]->gamma_para.brighten_level =
				config[0].info.bright;
		}

		if (config[0].flags & ENH_CONTRAST_DIRTY) {
			g_para[disp][chn]->dci_para.contrast_level =
				config[0].info.contrast;
		}

		if (config[0].flags & ENH_EDGE_DIRTY) {
			g_para[disp][chn]->sharp_para.lti_level =
				config[0].info.edge;
		}

		if (config[0].flags & ENH_DETAIL_DIRTY) {
			g_para[disp][chn]->peaking_para.level =
				config[0].info.detail;
			g_para[disp][chn]->fcm_para.level =
				config[0].info.detail;
		}

		if (config[0].flags & ENH_SAT_DIRTY) {
			g_para[disp][chn]->fcm_para.level =
				config[0].info.saturation;
		}
	}

	de_enhance_apply_core(disp);
	return 0;
}

s32 de_enhance_sync(u32 disp)
{
	/* de_enhance_update_regs(disp); */
	return 0;
}

s32 de_enhance_rcq_finish_tasklet(u32 disp)
{
	s32 chn, chno;

	chno = de_feat_get_num_vi_chns(disp);

	for (chn = 0; chn < chno; chn++) {
		if ((de_feat_is_support_vep_by_chn(disp, chn) <= 0))
			continue;
		/* dci */
		if (g_cfg->dci_exist[disp][chn])
			de_dci_tasklet(disp, chn, g_alg[disp][chn]->frame_cnt);

		de_cdc_tasklet(disp, chn, g_alg[disp][chn]->frame_cnt);
		g_alg[disp][chn]->rcq_frame_cnt++;
	}
	return 0;
}

s32 de_enhance_tasklet(u32 disp)
{
	s32 chn, chno;

	chno = de_feat_get_num_vi_chns(disp);

	for (chn = 0; chn < chno; chn++) {
		if ((de_feat_is_support_vep_by_chn(disp, chn) <= 0))
			continue;
		g_alg[disp][chn]->frame_cnt++;
	}
	return 0;
}

s32 de_enhance_update_regs(u32 disp)
{
	return 0;
}

s32 de_enhance_init(struct disp_bsp_init_para *para)
{
	s32 disp, chn;
	s32 i;
	u32 chn_num[DE_NUM];

	g_cfg = kmalloc(sizeof(struct vep_config_info),
			GFP_KERNEL | __GFP_ZERO);
	if (g_cfg == NULL) {
		DE_WARN("malloc g_cfg memory fail! size=0x%x\n",
		      (u32)sizeof(struct vep_config_info));
		return -1;
	}

	g_cfg->device_num = de_feat_get_num_screens();

	for (disp = 0; disp < g_cfg->device_num; disp++) {
		chn_num[disp] = de_feat_get_num_vi_chns(disp);
	}

	for (disp = 0; disp < g_cfg->device_num; disp++) {
		for (chn = 0; chn < chn_num[disp]; chn++) {
			struct de_enhance_private *priv;
			struct de_reg_mem_info *reg_mem_info;
			u8 __iomem *phy_addr;
			u8 *vir_addr;
			u32 size;
			u32 rcq_used;
			if ((de_feat_is_support_vep_by_chn(disp, chn) <= 0))
				continue;
			priv = &enhance_priv[disp][chn];
			reg_mem_info = &(priv->reg_mem_info);
			rcq_used = de_feat_is_using_rcq(disp);
			reg_mem_info->size = DE_ENHANCE_REG_MEM_SIZE;
			reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size,
				(void *)&(reg_mem_info->phy_addr), rcq_used);
			if (NULL == reg_mem_info->vir_addr) {
				DE_WARN("alloc ovl[%d][%d] mm fail!size=0x%x\n",
				       disp, chn, reg_mem_info->size);
				return -1;
			}
			phy_addr = reg_mem_info->phy_addr;
			vir_addr = reg_mem_info->vir_addr;
			size = reg_mem_info->size;

			g_cfg->peaking_exist[disp][chn] =
				de_feat_is_support_asuscale_by_chn(disp, chn);
			g_cfg->sharp_exist[disp][chn] =
				de_feat_is_support_sharp_by_chn(disp, chn);
			g_cfg->dci_exist[disp][chn] =
				de_feat_is_support_dci_by_chn(disp, chn);
			g_cfg->fcm_exist[disp][chn] =
				de_feat_is_support_fcm_by_chn(disp, chn);
			if (g_cfg->dci_exist[disp][chn])
				de_dci_init(disp, chn,
					    para->reg_base[DISP_MOD_DE],
					    &phy_addr, &vir_addr, &size);
			if (g_cfg->sharp_exist[disp][chn])
				de_sharp_init(disp, chn,
					      para->reg_base[DISP_MOD_DE],
					      &phy_addr, &vir_addr, &size);
			if (g_cfg->fcm_exist[disp][chn])
				de_fcm_init(disp, chn,
					    para->reg_base[DISP_MOD_DE],
					    &phy_addr, &vir_addr, &size);

			/* initial local variable */
			g_chn_info[disp][chn] =
				kmalloc(sizeof(struct disp_enhance_chn_info),
					GFP_KERNEL | __GFP_ZERO);
			if (g_chn_info[disp][chn] == NULL) {
				DE_WARN("malloc g_chn_info[%d][%d] memory fail!",
				      disp, chn);
				DE_WARN("size=0x%x\n",
				      (u32)sizeof(
					      struct disp_enhance_chn_info));
				return -1;
			}
			for (i = 0; i < MAX_LAYER_NUM_PER_CHN; i++)
				g_chn_info[disp][chn]->layer_info[i].format =
					0xff;

			g_para[disp][chn] = kmalloc(sizeof(struct vep_para),
						    GFP_KERNEL | __GFP_ZERO);
			if (g_para[disp][chn] == NULL) {
				DE_WARN("malloc g_para[%d][%d] memory fail!",
				      disp, chn);
				DE_WARN("size=0x%x\n",
				      (u32)sizeof(struct vep_para));
				return -1;
			}

			g_alg[disp][chn] = kmalloc(sizeof(struct vep_alg_var),
						   GFP_KERNEL | __GFP_ZERO);
			if (g_alg[disp][chn] == NULL) {
				DE_WARN("malloc g_alg[%d][%d] memory fail!", disp,
				      chn);
				DE_WARN("size=0x%x\n",
				      (u32)sizeof(struct vep_alg_var));
				return -1;
			}
		}
		if (de_feat_is_support_gamma(disp))
			de_gamma_init(disp, para->reg_base[DISP_MOD_DE]);
		if (de_feat_is_support_dither(disp))
			de_dither_init(disp, para->reg_base[DISP_MOD_DE]);
		if (de_feat_is_support_deband(disp))
			de_deband_init(disp, para->reg_base[DISP_MOD_DE]);
	}
	return 0;
}

s32 de_enhance_exit(void)
{
	s32 disp, chn;
	u32 vi_chn_num;

	for (disp = 0; disp < g_cfg->device_num; disp++) {
		vi_chn_num = de_feat_get_num_vi_chns(disp);
		for (chn = 0; chn < vi_chn_num; chn++) {
			if ((de_feat_is_support_vep_by_chn(disp, chn) <= 0))
				continue;
			if (g_cfg->dci_exist[disp][chn])
				de_dci_exit(disp, chn);
			if (g_cfg->fcm_exist[disp][chn])
				de_fcm_exit(disp, chn);
			if (g_cfg->sharp_exist[disp][chn])
				de_sharp_exit(disp, chn);

			kfree(g_chn_info[disp][chn]);
			kfree(g_para[disp][chn]);
			kfree(g_alg[disp][chn]);
		}
		if (de_feat_is_support_deband(disp))
			de_deband_exit(disp);
		if (de_feat_is_support_dither(disp))
			de_dither_exit(disp);
		if (de_feat_is_support_gamma(disp))
			de_gamma_exit(disp);
	}
	kfree(g_cfg);
	return 0;
}

s32 de_enhance_get_reg_blocks(u32 disp, struct de_reg_block **blks,
			      u32 *blk_num)
{
	u32 chn_num, chn;
	u32 total = 0;
	u32 num;

	chn_num = de_feat_get_num_chns(disp);

	if (blks == NULL) {
		for (chn = 0; chn < chn_num; ++chn) {
			if (de_feat_is_support_vep_by_chn(disp, chn)) {
				if (g_cfg->dci_exist[disp][chn]) {
					de_dci_get_reg_blocks(disp, chn, blks,
							      &num);
					total += num;
				}
				if (g_cfg->fcm_exist[disp][chn]) {
					de_fcm_get_reg_blocks(disp, chn, blks,
							      &num);
					total += num;
				}
				if (g_cfg->sharp_exist[disp][chn]) {
					de_sharp_get_reg_blocks(disp, chn, blks,
								&num);
					total += num;
				}
			}
		}
		de_deband_get_reg_blocks(disp, blks, &num);
		total += num;
		de_dither_get_reg_blocks(disp, blks, &num);
		total += num;
		de_gamma_get_reg_blocks(disp, blks, &num);
		total += num;
		*blk_num = total;
		return 0;
	}

	for (chn = 0; chn < chn_num; ++chn) {
		if (de_feat_is_support_vep_by_chn(disp, chn)) {
			if (g_cfg->dci_exist[disp][chn]) {
				num = *blk_num;
				de_dci_get_reg_blocks(disp, chn, blks, &num);
				total += num;
				blks += num;
				*blk_num -= num;
			}
			if (g_cfg->fcm_exist[disp][chn]) {
				num = *blk_num;
				de_fcm_get_reg_blocks(disp, chn, blks, &num);
				total += num;
				blks += num;
				*blk_num -= num;
			}
			if (g_cfg->sharp_exist[disp][chn]) {
				num = *blk_num;
				de_sharp_get_reg_blocks(disp, chn, blks, &num);
				total += num;
				blks += num;
				*blk_num -= num;
			}

			if (chn > 0)
				continue;

			num = *blk_num;
			de_deband_get_reg_blocks(disp, blks, &num);
			total += num;
			blks += num;
			*blk_num -= num;

			num = *blk_num;
			de_dither_get_reg_blocks(disp, blks, &num);
			total += num;
			blks += num;
			*blk_num -= num;

			num = *blk_num;
			de_gamma_get_reg_blocks(disp, blks, &num);
			total += num;
			blks += num;
			*blk_num -= num;
		}
	}
	*blk_num = total;
	return 0;
}

void de_set_bits(u32 *reg_addr, u32 bits_val, u32 shift, u32 width)
{
	u32 reg_val;

	reg_val = de_readl(reg_addr);
	reg_val = SET_BITS(shift, width, reg_val, bits_val);
	de_writel(reg_val, reg_addr);
}

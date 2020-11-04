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
	if (g_cfg->dns_exist[disp][chn])
		de_dns_init_para(disp, chn);
	if (g_cfg->peak2d_exist[disp][chn])
		de_vsu_init_sharp_para(disp, chn);
	de_fce_init_para(disp, chn);
	de_peak_init_para(disp, chn);
	de_lti_init_para(disp, chn);
	de_fcc_init_para(disp, chn);
	de_bls_init_para(disp, chn);

	return 0;
}

static s32 de_enhance_set_size(u32 disp, u32 chn)
{
	struct de_rect_o tmp_win; /* bld size window */
	struct de_rect_o tmp_win2;/* ovl size window */
	struct disp_rectsz size; /* bld size */
	struct disp_rectsz size2; /* ovl size */
	u32 demo_enable;

	demo_enable = g_para[disp][chn]->demo_enable;
	size.width = g_chn_info[disp][chn]->bld_size.width;
	size.height = g_chn_info[disp][chn]->bld_size.height;
	size2.width = g_chn_info[disp][chn]->ovl_size.width;
	size2.height = g_chn_info[disp][chn]->ovl_size.height;

	tmp_win.x = 0;
	tmp_win.y = 0;
	tmp_win2.x = 0;
	tmp_win2.y = 0;

	if (demo_enable) {
		if (size.width > size.height) {
			tmp_win.w = size.width >> 1;
			tmp_win.h = size.height;
			/* ovl window follow bld window */
			tmp_win2.w = size2.width >> 1;
			tmp_win2.h = size2.height;
		} else {
			tmp_win.w = size.width;
			tmp_win.h = size.height >> 1;
			/* ovl window follow bld window */
			tmp_win2.w = size2.width;
			tmp_win2.h = size2.height >> 1;
		}
	} else {
		tmp_win.w = size.width;
		tmp_win.h = size.height;
		tmp_win2.w = size2.width;
		tmp_win2.h = size2.height;
	}

	/* dns */
	if (g_cfg->dns_exist[disp][chn]) {
		de_dns_set_size(disp, chn, size2.width, size2.height);
		de_dns_set_window(disp, chn, demo_enable, tmp_win2);
	}
	/* fce */
	de_fce_set_size(disp, chn, size.width, size.height);
	de_fce_set_window(disp, chn, demo_enable, tmp_win);

	/* peak */
	de_peak_set_size(disp, chn, size.width, size.height);
	de_peak_set_window(disp, chn, demo_enable, tmp_win);

	/* lti */
	de_lti_set_size(disp, chn, size.width, size.height);
	de_lti_set_window(disp, chn, demo_enable, tmp_win);

	/* bls */
	de_bls_set_size(disp, chn, size.width, size.height);
	de_bls_set_window(disp, chn, demo_enable, tmp_win);

	/* fcc */
	de_fcc_set_size(disp, chn, size.width, size.height);
	de_fcc_set_window(disp, chn, demo_enable, tmp_win);

	return 0;
}

static s32 de_enhance_apply_core(u32 disp)
{
	s32 chn, chno;
	u32 other_dirty;
	u32 dev_type;
	u32 fmt;

	chno = g_cfg->vep_num[disp];
	other_dirty = ENH_SIZE_DIRTY | ENH_FORMAT_DIRTY | ENH_ENABLE_DIRTY |
		      ENH_MODE_DIRTY;
	for (chn = 0; chn < chno; chn++) {
		fmt = g_para[disp][chn]->fmt;
		dev_type = g_para[disp][chn]->dev_type;

		/* 0. initial parameters */
		if (g_para[disp][chn]->flags & ENH_INIT_DIRTY)
			de_enhance_init_para(disp, chn);

		/* 1. size and demo window reg */
		if (g_para[disp][chn]->flags &
		    (ENH_SIZE_DIRTY | ENH_FORMAT_DIRTY | ENH_MODE_DIRTY))
			de_enhance_set_size(disp, chn);

		/* 2. module parameters */
		if (g_para[disp][chn]->flags & (other_dirty |
		    ENH_BRIGHT_DIRTY | ENH_CONTRAST_DIRTY | ENH_BYPASS_DIRTY))
			de_fce_info2para(disp, chn, fmt, dev_type,
					 &g_para[disp][chn]->fce_para,
					 g_para[disp][chn]->bypass);

		if (g_para[disp][chn]->flags &
		    (other_dirty | ENH_DETAIL_DIRTY | ENH_BYPASS_DIRTY)) {
			if (g_cfg->peak2d_exist[disp][chn])
				de_vsu_set_sharp_para(disp, chn, fmt,
					dev_type,
					&g_para[disp][chn]->peak2d_para,
					g_para[disp][chn]->bypass);
			de_peak_info2para(disp, chn, fmt, dev_type,
					&g_para[disp][chn]->peak_para,
					g_para[disp][chn]->bypass);
		}

		if (g_para[disp][chn]->flags &
		    (other_dirty | ENH_EDGE_DIRTY | ENH_BYPASS_DIRTY))
			de_lti_info2para(disp, chn, fmt, dev_type,
					  &g_para[disp][chn]->lti_para,
					  g_para[disp][chn]->bypass);

		if (g_para[disp][chn]->flags &
		    (ENH_FORMAT_DIRTY | ENH_ENABLE_DIRTY | ENH_MODE_DIRTY |
		     ENH_SAT_DIRTY | ENH_BYPASS_DIRTY))
			de_fcc_info2para(disp, chn, fmt, dev_type,
					  &g_para[disp][chn]->fcc_para,
					  g_para[disp][chn]->bypass);

		if (g_para[disp][chn]->flags &
		    (ENH_FORMAT_DIRTY | ENH_ENABLE_DIRTY | ENH_MODE_DIRTY |
		     ENH_SAT_DIRTY | ENH_BYPASS_DIRTY))
			de_bls_info2para(disp, chn, fmt, dev_type,
					 &g_para[disp][chn]->bls_para,
					 g_para[disp][chn]->bypass);

		if (g_para[disp][chn]->flags &
		    (other_dirty | ENH_DNS_DIRTY | ENH_BYPASS_DIRTY) &&
		    g_cfg->dns_exist[disp][chn])
			de_dns_info2para(disp, chn, fmt, dev_type,
					  &g_para[disp][chn]->dns_para,
					  g_para[disp][chn]->bypass);

		/* clear dirty in g_config */
		g_para[disp][chn]->flags = 0x0;
	}

	return 0;

}

s32 de_enhance_layer_apply(u32 disp,
			   struct disp_enhance_chn_info *info)
{
	s32 chn, chno, i, layno;
	s32 feid; /* first_enable_layer_id */
	u32 overlay_enable = 0;
	u32 format = 0xff;
	struct disp_enhance_chn_info *p_info;
	u32 bypass;
	enum disp_enhance_dirty_flags flags = {0};

	chno = g_cfg->vep_num[disp];

	__inf("%s, ovl_size=<%d,%d>, bld_size=<%d,%d>\n", __func__,
	    info->ovl_size.width, info->ovl_size.height, info->bld_size.width,
	    info->bld_size.height);

	p_info = kmalloc(sizeof(struct disp_enhance_chn_info), GFP_KERNEL |
		 __GFP_ZERO);
	if (p_info == NULL) {
		__wrn("malloc p_info memory fail! size=0x%x\n",
		      (u32)sizeof(struct disp_enhance_chn_info));
		return -1;
	}

	for (chn = 0; chn < chno; chn++) {
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
		    (info[chn].bld_size.height != p_info->bld_size.height))
		    && (overlay_enable == 0x1)) {
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
					  DE_FORMAT_YUV444_I_AYUV) ? 0 : 1;
				break;
			}
		}

		/* UPDATE g_chn_info */
		if (flags &
		    (ENH_ENABLE_DIRTY | ENH_SIZE_DIRTY | ENH_FORMAT_DIRTY))
			memcpy(g_chn_info[disp][chn], &info[chn],
				sizeof(struct disp_enhance_chn_info));

		/* UPDATE g_para->foramt */
		if (flags & ENH_FORMAT_DIRTY)
			g_para[disp][chn]->fmt = format;

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
				g_para[disp][chn]->bypass |=
					SIZE_BYPASS;
			else
				g_para[disp][chn]->bypass &=
					SIZE_BYPASS_MASK;
		}

		if (flags & ENH_ENABLE_DIRTY) {
			if (!overlay_enable)
				g_para[disp][chn]->bypass |=
					LAYER_BYPASS;
			else
				g_para[disp][chn]->bypass &=
					LAYER_BYPASS_MASK;
		}

		/* BYPASS_DIRTY */
		if (bypass != g_para[disp][chn]->bypass)
			flags |= ENH_BYPASS_DIRTY;

		g_para[disp][chn]->flags = flags;

		/* UPDATE g_para->module_para */
		if (flags & ENH_SIZE_DIRTY) {
			/* peak_para */
			g_para[disp][chn]->peak_para.inw =
				g_chn_info[disp][chn]->ovl_size.width;
			g_para[disp][chn]->peak_para.inh =
				g_chn_info[disp][chn]->ovl_size.height;
			g_para[disp][chn]->peak_para.outw =
				g_chn_info[disp][chn]->bld_size.width;
			g_para[disp][chn]->peak_para.outh =
				g_chn_info[disp][chn]->bld_size.height;
			/* peak2d_para */
			g_para[disp][chn]->peak2d_para.inw =
				g_chn_info[disp][chn]->ovl_size.width;
			g_para[disp][chn]->peak2d_para.inh =
				g_chn_info[disp][chn]->ovl_size.height;
			g_para[disp][chn]->peak2d_para.outw =
				g_chn_info[disp][chn]->bld_size.width;
			g_para[disp][chn]->peak2d_para.outh =
				g_chn_info[disp][chn]->bld_size.height;
			/* lti_para */
			g_para[disp][chn]->lti_para.inw =
				g_chn_info[disp][chn]->ovl_size.width;
			g_para[disp][chn]->lti_para.inh =
				g_chn_info[disp][chn]->ovl_size.height;
			g_para[disp][chn]->lti_para.outw =
				g_chn_info[disp][chn]->bld_size.width;
			g_para[disp][chn]->lti_para.outh =
				g_chn_info[disp][chn]->bld_size.height;
			/* fce_para */
			g_para[disp][chn]->fce_para.outw =
				g_chn_info[disp][chn]->bld_size.width;
			g_para[disp][chn]->fce_para.outh =
				g_chn_info[disp][chn]->bld_size.height;
			/* dns_para */
			if (feid >= 0) {
				g_para[disp][chn]->dns_para.inw =
					g_chn_info[disp][chn]->
						layer_info[feid].fb_size.width;
				g_para[disp][chn]->dns_para.inh =
					g_chn_info[disp][chn]->
						layer_info[feid].fb_size.height;
				memcpy(&g_para[disp][chn]->
				       dns_para.croprect,
				       &g_chn_info[disp][chn]->
				       layer_info[feid].fb_crop,
				       sizeof(struct disp_rect));
			} else {
				/* all layer disable */
				g_para[disp][chn]->dns_para.inw = 0;
				g_para[disp][chn]->dns_para.inh = 0;
				memset(&g_para[disp][chn]->
				       dns_para.croprect,
				       0, sizeof(struct disp_rect));
			}
		}
	}

	de_enhance_apply_core(disp);
	kfree(p_info);
	return 0;
}

s32 de_enhance_apply(u32 disp,
			   struct disp_enhance_config *config)
{
	s32 chn, chno;
	u32 bypass;

	chno = g_cfg->vep_num[disp];

	for (chn = 0; chn < chno; chn++) {
		/* copy dirty to g_para->flags */
		g_para[disp][chn]->flags |=
					(config[0].flags & ENH_USER_DIRTY);
		bypass = g_para[disp][chn]->bypass;
		if (config[0].flags & ENH_MODE_DIRTY) {
			/* enhance_mode */
			if (((config[0].info.mode & 0xffff0000) >> 16) == 2) {
				g_para[disp][chn]->demo_enable = 0x1;
				g_para[disp][chn]->bypass &=
							USER_BYPASS_MASK;
			} else if (((config[0].info.mode & 0xffff0000) >> 16)
				   == 1) {
				g_para[disp][chn]->demo_enable = 0x0;
				g_para[disp][chn]->bypass &=
							USER_BYPASS_MASK;
			} else {
				g_para[disp][chn]->demo_enable = 0x0;
				g_para[disp][chn]->bypass |= USER_BYPASS;
			}
			/* dev_type */
			g_para[disp][chn]->dev_type =
					config[0].info.mode & 0x0000ffff;

			if (bypass != g_para[disp][chn]->bypass)
				g_para[disp][chn]->flags |=
							       ENH_BYPASS_DIRTY;
		}

		if (config[0].flags & ENH_BRIGHT_DIRTY) {
			g_para[disp][chn]->fce_para.bright_level =
						config[0].info.bright;
		}

		if (config[0].flags & ENH_CONTRAST_DIRTY) {
			g_para[disp][chn]->fce_para.contrast_level =
						config[0].info.contrast;
		}

		if (config[0].flags & ENH_EDGE_DIRTY) {
			g_para[disp][chn]->lti_para.level =
						config[0].info.edge;
		}

		if (config[0].flags & ENH_DETAIL_DIRTY) {
			g_para[disp][chn]->peak_para.level =
						config[0].info.detail;
			g_para[disp][chn]->peak2d_para.level =
						config[0].info.detail;
		}

		if (config[0].flags & ENH_SAT_DIRTY) {
			g_para[disp][chn]->fcc_para.level =
						config[0].info.saturation;
			g_para[disp][chn]->bls_para.level =
						config[0].info.saturation;
		}

		if (config[0].flags & ENH_DNS_DIRTY) {
			g_para[disp][chn]->dns_para.level =
						config[0].info.denoise;
		}
	}

	de_enhance_apply_core(disp);
	return 0;
}

s32 de_enhance_sync(u32 disp)
{
	/* de_enhance_update_regs(id); */
	return 0;
}

s32 de_enhance_tasklet(u32 disp)
{
	s32 chn, chno;

	chno = g_cfg->vep_num[disp];

	for (chn = 0; chn < chno; chn++) {
		/* hist */
		de_hist_tasklet(disp, chn,
				g_alg[disp][chn]->frame_cnt);

		/* ce */
		de_ce_tasklet(disp, chn,
			      g_alg[disp][chn]->frame_cnt);

		/* dns */
		if (g_cfg->dns_exist[disp][chn])
			de_dns_tasklet(disp, chn,
				       g_alg[disp][chn]->frame_cnt);

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

	g_cfg = kmalloc(sizeof(struct vep_config_info), GFP_KERNEL |
		 __GFP_ZERO);
	if (g_cfg == NULL) {
		__wrn("malloc g_cfg memory fail! size=0x%x\n",
		      (u32)sizeof(struct vep_config_info));
		return -1;
	}

	g_cfg->device_num = de_feat_get_num_screens();

	for (disp = 0; disp < g_cfg->device_num; disp++)
		g_cfg->vep_num[disp] = de_feat_is_support_vep(disp);

	for (disp = 0; disp < g_cfg->device_num; disp++)
		for (chn = 0; chn < g_cfg->vep_num[disp]; chn++) {
			struct de_enhance_private *priv =
				&enhance_priv[disp][chn];
			struct de_reg_mem_info *reg_mem_info =
				&(priv->reg_mem_info);
			u8 __iomem *phy_addr;
			u8 *vir_addr;
			u32 size;
			u32 rcq_used = de_feat_is_using_rcq(disp);

			reg_mem_info->size = DE_ENHANCE_REG_MEM_SIZE;
			reg_mem_info->vir_addr = (u8 *)de_top_reg_memory_alloc(
				reg_mem_info->size, (void *)&(reg_mem_info->phy_addr),
				rcq_used);
			if (NULL == reg_mem_info->vir_addr) {
				DE_WRN("alloc ovl[%d][%d] mm fail!size=0x%x\n",
					 disp, chn, reg_mem_info->size);
				return -1;
			}
			phy_addr = reg_mem_info->phy_addr;
			vir_addr = reg_mem_info->vir_addr;
			size = reg_mem_info->size;

			g_cfg->dns_exist[disp][chn] =
			   de_feat_is_support_de_noise_by_chn(disp, chn);
			g_cfg->peak2d_exist[disp][chn] =
			    de_feat_is_support_edscale_by_chn(disp, chn);
			if (g_cfg->dns_exist[disp][chn])
				de_dns_init(disp, chn,
					para->reg_base[DISP_MOD_DE],
					&phy_addr, &vir_addr, &size);
			de_fce_init(disp, chn,
				para->reg_base[DISP_MOD_DE],
				&phy_addr, &vir_addr, &size);
			de_peak_init(disp, chn,
				para->reg_base[DISP_MOD_DE],
				&phy_addr, &vir_addr, &size);
			de_lti_init(disp, chn,
				para->reg_base[DISP_MOD_DE],
				&phy_addr, &vir_addr, &size);
			de_bls_init(disp, chn,
				para->reg_base[DISP_MOD_DE],
				&phy_addr, &vir_addr, &size);
			de_fcc_init(disp, chn,
				para->reg_base[DISP_MOD_DE],
				&phy_addr, &vir_addr, &size);

			/* initial local variable */
			g_chn_info[disp][chn] =
				kmalloc(sizeof(struct disp_enhance_chn_info),
					GFP_KERNEL | __GFP_ZERO);
			if (g_chn_info[disp][chn] == NULL) {
				__wrn("malloc g_chn_info[%d][%d] memory fail!",
				      disp, chn);
				__wrn("size=0x%x\n", (u32)
				      sizeof(struct disp_enhance_chn_info));
				return -1;
			}
			for (i = 0; i < MAX_LAYER_NUM_PER_CHN; i++)
				g_chn_info[disp][chn]->layer_info[i].format = 0xff;

			g_para[disp][chn] =
				kmalloc(sizeof(struct vep_para), GFP_KERNEL |
					__GFP_ZERO);
			if (g_para[disp][chn] == NULL) {
				__wrn("malloc g_para[%d][%d] memory fail!",
				      disp, chn);
				__wrn("size=0x%x\n",
				      (u32)sizeof(struct vep_para));
				return -1;
			}

			g_alg[disp][chn] =
				kmalloc(sizeof(struct vep_alg_var),
					GFP_KERNEL | __GFP_ZERO);
			if (g_alg[disp][chn] == NULL) {
				__wrn("malloc g_alg[%d][%d] memory fail!",
				      disp, chn);
				__wrn("size=0x%x\n",
				      (u32)sizeof(struct vep_alg_var));
				return -1;
			}

			g_para[disp][chn]->peak_para.peak2d_exist =
				g_cfg->peak2d_exist[disp][chn];
		}

	return 0;
}

s32 de_enhance_exit(void)
{
	s32 disp, chn;

	for (disp = 0; disp < g_cfg->device_num; disp++)
		for (chn = 0; chn < g_cfg->vep_num[disp]; chn++) {
			if (g_cfg->dns_exist[disp][chn])
				de_dns_exit(disp, chn);
			de_fce_exit(disp, chn);
			de_peak_exit(disp, chn);
			de_lti_exit(disp, chn);
			de_bls_exit(disp, chn);
			de_fcc_exit(disp, chn);

			kfree(g_chn_info[disp][chn]);
			kfree(g_para[disp][chn]);
			kfree(g_alg[disp][chn]);
		}

	kfree(g_cfg);

	return 0;
}

s32 de_enhance_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num)
{
	u32 chn_num, chn;
	u32 total = 0;
	u32 num;

	chn_num = de_feat_get_num_chns(disp);

	if (blks == NULL) {
		for (chn = 0; chn < chn_num; ++chn) {
			if (de_feat_is_support_vep_by_chn(disp, chn)) {
				if (g_cfg->dns_exist[disp][chn]) {
					de_dns_get_reg_blocks(disp, chn, blks, &num);
					total += num;
				}
				de_fce_get_reg_blocks(disp, chn, blks, &num);
				total += num;
				de_peak_get_reg_blocks(disp, chn, blks, &num);
				total += num;
				de_lti_get_reg_blocks(disp, chn, blks, &num);
				total += num;
				de_bls_get_reg_blocks(disp, chn, blks, &num);
				total += num;
				de_fcc_get_reg_blocks(disp, chn, blks, &num);
				total += num;
			}
		}
		*blk_num = total;
		return 0;
	}

	for (chn = 0; chn < chn_num; ++chn) {
		if (de_feat_is_support_vep_by_chn(disp, chn)) {
			if (g_cfg->dns_exist[disp][chn]) {
				num = *blk_num;
				de_dns_get_reg_blocks(disp, chn, blks, &num);
				total += num;
				blks += num;
				*blk_num -= num;
			}
			num = *blk_num;
			de_fce_get_reg_blocks(disp, chn, blks, &num);
			total += num;
			blks += num;
			*blk_num -= num;

			num = *blk_num;
			de_peak_get_reg_blocks(disp, chn, blks, &num);
			total += num;
			blks += num;
			*blk_num -= num;

			num = *blk_num;
			de_lti_get_reg_blocks(disp, chn, blks, &num);
			total += num;
			blks += num;
			*blk_num -= num;

			num = *blk_num;
			de_bls_get_reg_blocks(disp, chn, blks, &num);
			total += num;
			blks += num;
			*blk_num -= num;

			num = *blk_num;
			de_fcc_get_reg_blocks(disp, chn, blks, &num);
			total += num;
			blks += num;
			*blk_num -= num;
		}
	}
	*blk_num = total;

	return 0;
}

void de_set_bits(u32 *reg_addr, u32 bits_val,
			u32 shift, u32 width)
{
	u32 reg_val;

	reg_val = de_readl(reg_addr);
	reg_val = SET_BITS(shift, width, reg_val, bits_val);
	de_writel(reg_val, reg_addr);
}

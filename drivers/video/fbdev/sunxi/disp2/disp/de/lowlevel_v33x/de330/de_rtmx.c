/*
* Allwinner SoCs display driver.
*
* Copyright (C) 2017 Allwinner.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include "../../include.h"
#include "de_rtmx.h"
#include "de_feat.h"
#include "de_top.h"
#include "de_ovl.h"
#include "de_fbd_atw.h"
#include "de_vsu.h"
#include "de_cdc.h"
#include "de_ccsc.h"
#include "de_bld.h"
#include "de_fmt.h"
#include "de_enhance.h"
#include "de_smbl.h"
#include "de_snr.h"
#include "de_ksc.h"

struct de_rtmx_context *de_rtmx_get_context(u32 disp)
{
	static struct de_rtmx_context rtmx_ctx[DE_NUM];
	return &rtmx_ctx[disp];
}

static s32 de_rtmx_init_rcq(u32 disp)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	s32 (*get_reg_blocks[])(u32, struct de_reg_block **, u32 *) = {
		de_ovl_get_reg_blocks,
		de_fbd_atw_get_reg_blocks,
		de_vsu_get_reg_blocks,
		de_ccsc_get_reg_blocks,
		de_bld_get_reg_blocks,
		de_fmt_get_reg_blocks,
		de_enhance_get_reg_blocks,
		de_smbl_get_reg_blocks,
		de_snr_get_reg_blocks,
		de_ksc_get_reg_blocks,
	};

	s32 (*func)(u32, struct de_reg_block **, u32 *);
	const u32 func_num =
		sizeof(get_reg_blocks) / sizeof(get_reg_blocks[0]);

	struct de_rcq_mem_info *rcq_info = &ctx->rcq_info;
	struct de_rcq_head *rcq_hd = NULL;
	struct de_reg_block **p_reg_blks;
	u32 reg_blk_num, i;

	struct de_chn_info *chn_info;
	u32 chn;

	for (i = 0; i < func_num; ++i) {
		func = get_reg_blocks[i];
		func(disp, NULL, &reg_blk_num);
		rcq_info->cur_num += reg_blk_num;
	}
	for (chn = 0; chn < ctx->chn_num; chn++) {
		chn_info = ctx->chn_info + chn;
		if (chn_info->cdc_hdl) {
			de_cdc_get_reg_blks(chn_info->cdc_hdl, NULL, &reg_blk_num);
			rcq_info->cur_num += reg_blk_num;
		}
	}
	rcq_info->reg_blk = kmalloc(
		sizeof(*(rcq_info->reg_blk)) * rcq_info->cur_num,
		GFP_KERNEL | __GFP_ZERO);
	if (rcq_info->reg_blk == NULL) {
		DE_WRN("kalloc for de_reg_block failed\n");
		return -1;
	}
	p_reg_blks = rcq_info->reg_blk;
	reg_blk_num = rcq_info->cur_num;
	for (i = 0; i < func_num; ++i) {
		u32 num = reg_blk_num;

		func = get_reg_blocks[i];
		func(disp, p_reg_blks, &num);
		reg_blk_num -= num;
		p_reg_blks += num;
	}
	for (chn = 0; chn < ctx->chn_num; chn++) {
		chn_info = ctx->chn_info + chn;
		if (chn_info->cdc_hdl) {
			u32 num = reg_blk_num;

			de_cdc_get_reg_blks(chn_info->cdc_hdl,
				p_reg_blks, &num);
			reg_blk_num -= num;
			p_reg_blks += num;
		}
	}

#if RTMX_USE_RCQ
	rcq_info->alloc_num = ((rcq_info->cur_num + 1) >> 1) << 1;
	rcq_info->vir_addr = (struct de_rcq_head *)de_top_reg_memory_alloc(
		rcq_info->alloc_num * sizeof(*(rcq_info->vir_addr)),
		&(rcq_info->phy_addr), de_feat_is_using_rcq(disp));
	if (rcq_info->vir_addr == NULL) {
		DE_WRN("alloc for de_rcq_head failed\n");
		return -1;
	}
	p_reg_blks  = rcq_info->reg_blk;
	rcq_hd = rcq_info->vir_addr;
	for (reg_blk_num = 0; reg_blk_num < rcq_info->cur_num;
		++reg_blk_num) {
		struct de_reg_block *reg_blk = *p_reg_blks;

		rcq_hd->low_addr = (u32)((uintptr_t)(reg_blk->phy_addr));
		rcq_hd->dw0.bits.high_addr =
			(u8)((uintptr_t)(reg_blk->phy_addr) >> 32);
		rcq_hd->dw0.bits.len = reg_blk->size;
		rcq_hd->dirty.dwval = reg_blk->dirty;
		rcq_hd->reg_offset = (u32)(uintptr_t)
			(reg_blk->reg_addr - ctx->de_reg_base);
		reg_blk->rcq_hd = rcq_hd;
		rcq_hd++;
		p_reg_blks++;
	}
	if (rcq_info->alloc_num > rcq_info->cur_num) {
		rcq_hd = rcq_info->vir_addr + rcq_info->alloc_num;
		rcq_hd->dirty.dwval = 0;
	}

#else
	rcq_hd = NULL;
#endif

	return 0;
}

static s32 de_rtmx_exit_rcq(u32 disp)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_rcq_mem_info *rcq_info = &ctx->rcq_info;

	if (rcq_info->reg_blk)
		kfree(rcq_info->reg_blk);
	if (rcq_info->vir_addr)
		de_top_reg_memory_free(rcq_info->vir_addr, rcq_info->phy_addr,
			rcq_info->alloc_num * sizeof(*(rcq_info->vir_addr)));

	return 0;
}

s32 de_rtmx_setup_rcq(u32 disp)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_rcq_mem_info *rcq_info = &ctx->rcq_info;

	de_top_set_rcq_head(disp, (u64)(rcq_info->phy_addr),
		rcq_info->alloc_num * sizeof(*(rcq_info->vir_addr)));

	return 0;
}

s32 de_rtmx_set_all_rcq_head_dirty(u32 disp, u32 dirty)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_rcq_mem_info *rcq_info = &ctx->rcq_info;
	struct de_rcq_head *hd = rcq_info->vir_addr;
	struct de_rcq_head *hd_end = hd + rcq_info->cur_num;

	if (hd == NULL) {
		DE_WRN("rcq head is null\n");
		return -1;
	}

	for (; hd != hd_end; hd++)
		hd->dirty.dwval = dirty;

	return 0;
}

s32 de_rtmx_init(struct disp_bsp_init_para *para)
{
	u32 disp;
	u32 disp_num = de_feat_get_num_screens();

	de_top_set_reg_base(
		(u8 __iomem *)(para->reg_base[DISP_MOD_DE]));

	for (disp = 0; disp < disp_num; ++disp) {
		struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
		struct de_chn_info *chn_info;
		u32 chn, phy_chn, vi_chn_num;
		u8 __iomem *reg_base;
		u32 rcq_used = de_feat_is_using_rcq(disp);

		ctx->chn_num = de_feat_get_num_chns(disp);
		if (ctx->chn_num == 0) {
			DE_WRN("disp(%d) has no chn\n", disp);
			continue;
		}
		ctx->chn_info = kmalloc(
			ctx->chn_num * sizeof(*(ctx->chn_info)),
			GFP_KERNEL | __GFP_ZERO);
		if (ctx->chn_info == NULL) {
			DE_WRN("alloc fail! size=%d\n", ctx->chn_num);
			return -1;
		}

		ctx->de_reg_base =
			(u8 __iomem *)(para->reg_base[DISP_MOD_DE]);
		vi_chn_num = de_feat_get_num_vi_chns(disp);
		for (chn = 0; chn < ctx->chn_num; ++chn) {
			chn_info = ctx->chn_info + chn;
			phy_chn = de_feat_get_phy_chn_id(disp, chn);

			if (de_feat_is_support_cdc_by_chn(disp, chn)) {
				reg_base = ctx->de_reg_base + DE_CHN_OFFSET(phy_chn)
					+ CHN_CDC_OFFSET;
				chn_info->cdc_hdl = de_cdc_create(reg_base,
					rcq_used, ((phy_chn == 0) ? 1 : 0));
				if (chn_info->cdc_hdl == NULL) {
					DE_WRN("de_cdc_create fail!\n");
					return -1;
				}
			}
		}

		de_ovl_init(disp, ctx->de_reg_base);
		de_fbd_atw_init(disp, ctx->de_reg_base);
		de_snr_init(disp, ctx->de_reg_base);
		de_vsu_init(disp, ctx->de_reg_base);
		de_ccsc_init(disp, ctx->de_reg_base);
		de_bld_init(disp, ctx->de_reg_base);
		de_ksc_init(disp, ctx->de_reg_base);
		de_fmt_init(disp, ctx->de_reg_base);
		de_rtmx_init_rcq(disp);
	}

	return 0;
}

s32 de_rtmx_exit(void)
{
	u32 disp = 0;
	u32 disp_num = de_feat_get_num_screens();

	for (disp = 0; disp < disp_num; ++disp) {
		struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
		struct de_chn_info *chn_info;
		u32 chn;

		de_rtmx_exit_rcq(disp);

		de_fmt_exit(disp);
		de_bld_exit(disp);

		de_ccsc_exit(disp);
		de_vsu_exit(disp);
		de_fbd_atw_exit(disp);
		de_ovl_exit(disp);
		de_snr_exit(disp);
		de_ksc_exit(disp);

		for (chn = 0; chn < ctx->chn_num; ++chn) {
			chn_info = ctx->chn_info + chn;
			if (chn_info->cdc_hdl)
				de_cdc_destroy(chn_info->cdc_hdl);
		}

		if (ctx->chn_info != NULL)
			kfree(ctx->chn_info);
	}
	de_top_mem_pool_free();

	return 0;
}

static s32 de_rtmx_set_chn_mux(u32 disp)
{
	u32 chn, phy_chn, chn_num, v_chn_num;

	chn_num = de_feat_get_num_chns(disp);
	v_chn_num = de_feat_get_num_vi_chns(disp);
	for (chn = 0; chn < chn_num; ++chn) {
		phy_chn = de_feat_get_phy_chn_id(disp, chn);
		if (chn < v_chn_num) {
			de_top_set_vchn2core_mux(phy_chn, disp);
			de_top_set_port2vchn_mux(disp, chn, phy_chn);
		} else {
			de_top_set_uchn2core_mux(phy_chn, disp);
			de_top_set_port2uchn_mux(disp, chn, phy_chn);
		}
	}

	return 0;
}

s32 de_rtmx_start(u32 disp)
{
	de_top_set_clk_enable(DE_CLK_CORE0 + disp, 1);
	de_rtmx_set_chn_mux(disp);

#if RTMX_USE_RCQ
	de_rtmx_setup_rcq(disp);
#endif

	return 0;
}

s32 de_rtmx_stop(u32 disp)
{
	de_top_set_clk_enable(DE_CLK_CORE0 + disp, 0);
	return 0;
}

s32 de_rtmx_update_reg_ahb(u32 disp)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_rcq_mem_info *rcq_info = &ctx->rcq_info;
	struct de_reg_block **p_reg_blk = rcq_info->reg_blk;
	struct de_reg_block **p_reg_blk_end =
		p_reg_blk + rcq_info->cur_num;

	for (; p_reg_blk != p_reg_blk_end; ++p_reg_blk) {
		struct de_reg_block *reg_blk = *p_reg_blk;

		if (reg_blk->dirty) {
			memcpy((void *)reg_blk->reg_addr,
				(void *)reg_blk->vir_addr, reg_blk->size);
			reg_blk->dirty = 0;
		}
	}

	return 0;
}

s32 de_rtmx_set_rcq_update(u32 disp, u32 en)
{
	if (en) {
		struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
		struct de_rcq_mem_info *rcq_info = &ctx->rcq_info;
		struct de_rcq_head *hd = rcq_info->vir_addr;
		struct de_rcq_head *hd_end = hd + rcq_info->cur_num;

		for (; hd != hd_end; ++hd) {
			if (hd->dirty.dwval)
				return de_top_set_rcq_update(disp, en);
		}
	} else {
		de_top_set_rcq_update(disp, en);
	}
	return 0;
}

enum de_color_space de_rtmx_convert_color_space(enum de_color_space_u color_space_u)
{
	switch (color_space_u) {
	case DE_GBR:
	case DE_GBR_F:
		return DE_COLOR_SPACE_GBR;
	case DE_BT709:
	case DE_BT709_F:
		return DE_COLOR_SPACE_BT709;
	case DE_FCC:
	case DE_FCC_F:
		return DE_COLOR_SPACE_FCC;
	case DE_BT470BG:
	case DE_BT470BG_F:
		return DE_COLOR_SPACE_BT470BG;
	case DE_BT601:
	case DE_BT601_F:
		return DE_COLOR_SPACE_BT601;
	case DE_SMPTE240M:
	case DE_SMPTE240M_F:
		return DE_COLOR_SPACE_SMPTE240M;
	case DE_YCGCO:
	case DE_YCGCO_F:
		return DE_COLOR_SPACE_YCGCO;
	case DE_BT2020NC:
	case DE_BT2020NC_F:
		return DE_COLOR_SPACE_BT2020NC;
	case DE_BT2020C:
	case DE_BT2020C_F:
		return DE_COLOR_SPACE_BT2020C;
	case DE_RESERVED:
	case DE_RESERVED_F:
	case DE_UNDEF:
	case DE_UNDEF_F:
	default:
		return DE_COLOR_SPACE_GBR;
	}
}

s32 de_rtmx_mgr_apply(u32 disp, struct disp_manager_data *data)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);

	if (data->flag & MANAGER_BACK_COLOR_DIRTY) {
		ctx->bg_color = (data->config.back_color.alpha << 24)
			| (data->config.back_color.red << 16)
			| (data->config.back_color.green << 8)
			| (data->config.back_color.blue);
		if ((data->config.cs != DISP_CSC_TYPE_RGB)
			&& ((ctx->bg_color & 0xFFFFFF) == 0)) {
			ctx->bg_color |= ((0x10 << 16) | (0x80 << 8) | 0x80);
		}
		de_bld_set_bg_color(disp, ctx->bg_color);
		de_bld_set_pipe_fcolor(disp, 0, ctx->bg_color | 0xFF000000);
	}

	if (data->flag & MANAGER_SIZE_DIRTY) {
		ctx->output.scn_width = data->config.size.width;
		ctx->output.scn_height = data->config.size.height;
		de_bld_set_out_size(disp,
			ctx->output.scn_width,
			ctx->output.scn_height);
		de_top_set_out_size(disp,
			ctx->output.scn_width,
			ctx->output.scn_height);
	}

	if (data->flag & MANAGER_COLOR_SPACE_DIRTY) {
		/* set yuv or rgb blending space */
		switch (data->config.cs) {
		case DISP_CSC_TYPE_RGB:
			ctx->output.px_fmt_space = DE_FORMAT_SPACE_RGB;
			de_bld_set_fmt_space(disp, 0);
			break;
		case DISP_CSC_TYPE_YUV444:
			ctx->output.px_fmt_space = DE_FORMAT_SPACE_YUV;
			ctx->output.yuv_sampling = DE_YUV444;
			de_bld_set_fmt_space(disp, 1);
			break;
		case DISP_CSC_TYPE_YUV422:
			ctx->output.px_fmt_space = DE_FORMAT_SPACE_YUV;
			ctx->output.yuv_sampling = DE_YUV422;
			de_bld_set_fmt_space(disp, 1);
			break;
		case DISP_CSC_TYPE_YUV420:
			ctx->output.px_fmt_space = DE_FORMAT_SPACE_YUV;
			ctx->output.yuv_sampling = DE_YUV420;
			de_bld_set_fmt_space(disp, 1);
			break;
		default:
			DE_WRN("invalid config.cs=%d\n", data->config.cs);
			break;
		}
		ctx->output.color_range = data->config.color_range;
		ctx->output.color_space = de_rtmx_convert_color_space(
			(enum de_color_space_u)data->config.color_space);
		ctx->output.eotf = (enum de_eotf)data->config.eotf;
		ctx->output.data_bits = (enum de_data_bits)data->config.data_bits;
		ctx->output.hdr_type = data->config.hdr_type;
		DE_WRN("ctx->output.hdr_type=%d\n", ctx->output.hdr_type);
		de_fmt_set_para(disp);
	}

	if (data->flag & MANAGER_ENABLE_DIRTY) {
		de_top_set_rtmx_enable(disp, data->config.enable);

		ctx->output.tcon_id = data->config.hwdev_index;
		ctx->output.scan_mode = data->config.interlace;
		de_bld_set_out_scan_mode(disp, ctx->output.scan_mode);

		ctx->output.fps = data->config.device_fps;
		ctx->output.color_range = data->config.color_range;
		ctx->output.color_space = de_rtmx_convert_color_space(
			(enum de_color_space_u)data->config.color_space);
		ctx->output.eotf = (enum de_eotf)data->config.eotf;
		ctx->output.data_bits = (enum de_data_bits)data->config.data_bits;
		ctx->output.hdr_type = data->config.hdr_type;
		DE_WRN("ctx->output.hdr_type=%d\n", ctx->output.hdr_type);

		de_fmt_set_para(disp);

	}

	if (data->flag & MANAGER_KSC_DIRTY)
		de_ksc_set_para(disp, &data->config.ksc);

	ctx->clk_rate_hz = data->config.de_freq;
	ctx->be_blank = data->config.blank;

	return 0;
}

static s32 de_rtmx_chn_data_attr(
	struct de_chn_info *chn_info,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct disp_layer_info_inner *lay_info;
	u32 i;

	for (i = 0; i < layer_num; ++i) {
		lay_info = &(pdata[i]->config.info);
		if (lay_info->mode == LAYER_MODE_BUFFER)
			break;
	}
	if (i == layer_num)
		lay_info = &(pdata[0]->config.info);

	chn_info->px_fmt = (enum de_pixel_format)lay_info->fb.format;
	switch (chn_info->px_fmt) {
	case DE_FORMAT_YUV422_I_YVYU:
	case DE_FORMAT_YUV422_I_YUYV:
	case DE_FORMAT_YUV422_I_UYVY:
	case DE_FORMAT_YUV422_I_VYUY:
	case DE_FORMAT_YUV422_I_YVYU_10BIT:
	case DE_FORMAT_YUV422_I_YUYV_10BIT:
	case DE_FORMAT_YUV422_I_UYVY_10BIT:
	case DE_FORMAT_YUV422_I_VYUY_10BIT:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_YUV;
		chn_info->yuv_sampling = DE_YUV422;
		chn_info->planar_num = 1;
		break;
	case DE_FORMAT_YUV422_P:
	case DE_FORMAT_YUV422_P_10BIT:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_YUV;
		chn_info->yuv_sampling = DE_YUV422;
		chn_info->planar_num = 2;
		break;
	case DE_FORMAT_YUV422_SP_UVUV:
	case DE_FORMAT_YUV422_SP_VUVU:
	case DE_FORMAT_YUV422_SP_UVUV_10BIT:
	case DE_FORMAT_YUV422_SP_VUVU_10BIT:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_YUV;
		chn_info->yuv_sampling = DE_YUV422;
		chn_info->planar_num = 3;
		break;
	case DE_FORMAT_YUV420_P:
	case DE_FORMAT_YUV420_P_10BIT:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_YUV;
		chn_info->yuv_sampling = DE_YUV420;
		chn_info->planar_num = 3;
		break;
	case DE_FORMAT_YUV420_SP_UVUV:
	case DE_FORMAT_YUV420_SP_VUVU:
	case DE_FORMAT_YUV420_SP_UVUV_10BIT:
	case DE_FORMAT_YUV420_SP_VUVU_10BIT:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_YUV;
		chn_info->yuv_sampling = DE_YUV420;
		chn_info->planar_num = 2;
		break;
	case DE_FORMAT_YUV411_P:
	case DE_FORMAT_YUV411_P_10BIT:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_YUV;
		chn_info->yuv_sampling = DE_YUV411;
		chn_info->planar_num = 3;
		break;
	case DE_FORMAT_YUV411_SP_UVUV:
	case DE_FORMAT_YUV411_SP_VUVU:
	case DE_FORMAT_YUV411_SP_UVUV_10BIT:
	case DE_FORMAT_YUV411_SP_VUVU_10BIT:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_YUV;
		chn_info->yuv_sampling = DE_YUV411;
		chn_info->planar_num = 2;
		break;
	case DE_FORMAT_YUV444_I_AYUV:
	case DE_FORMAT_YUV444_I_VUYA:
	case DE_FORMAT_YUV444_I_AYUV_10BIT:
	case DE_FORMAT_YUV444_I_VUYA_10BIT:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_YUV;
		chn_info->yuv_sampling = DE_YUV444;
		chn_info->planar_num = 1;
		break;
	case DE_FORMAT_YUV444_P:
	case DE_FORMAT_YUV444_P_10BIT:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_YUV;
		chn_info->yuv_sampling = DE_YUV444;
		chn_info->planar_num = 3;
		break;
	case DE_FORMAT_8BIT_GRAY:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_GRAY;
		chn_info->planar_num = 1;
		break;
	default:
		chn_info->px_fmt_space = DE_FORMAT_SPACE_RGB;
		chn_info->planar_num = 1;
		break;
	}

	chn_info->eotf = (enum de_eotf)lay_info->fb.eotf;

	if (chn_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		/* rgb format will be always in full range */
		chn_info->color_space = DE_COLOR_SPACE_GBR;
		chn_info->color_range = DE_COLOR_RANGE_0_255;
	} else {
		enum de_color_space_u color_space_u =
			(enum de_color_space_u)lay_info->fb.color_space;

		switch (color_space_u) {
		case DE_UNDEF:
		case DE_RESERVED:
			chn_info->color_range = DE_COLOR_RANGE_16_235;
			if ((lay_info->fb.size[0].width <= 736)
				&& (lay_info->fb.size[0].height <= 576)) {
				chn_info->color_space = DE_COLOR_SPACE_BT601;
			} else {
				chn_info->color_space = DE_COLOR_SPACE_BT709;
			}
			break;
		case DE_UNDEF_F:
		case DE_RESERVED_F:
			chn_info->color_range = DE_COLOR_RANGE_0_255;
			if ((lay_info->fb.size[0].width <= 736)
				&& (lay_info->fb.size[0].height <= 576)) {
				chn_info->color_space = DE_COLOR_SPACE_BT601;
			} else {
				chn_info->color_space = DE_COLOR_SPACE_BT709;
			}
			break;
		default:
			chn_info->color_space = color_space_u & 0xF;
			chn_info->color_range =
				((color_space_u & 0xF00) == 0x100) ?
			    DE_COLOR_RANGE_16_235 : DE_COLOR_RANGE_0_255;
			break;
		}
	}

	return 0;
}

static void de_rtmx_chn_blend_attr(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	struct disp_layer_info_inner *lay_info;
	u32 i;
	u32 lay_premul_num = 0;

	if (chn_info->px_fmt_space == DE_FORMAT_SPACE_RGB) {
		chn_info->alpha_mode = DE_ALPHA_MODE_PIXEL_ALPHA;
	} else {
		chn_info->alpha_mode = DE_ALPHA_MODE_GLOBAL_ALPHA;
	}
	chn_info->glb_alpha = 0xFF;
	memset((void *)chn_info->lay_premul, 0, sizeof(chn_info->lay_premul));

	for (i = 0; i < layer_num; ++i) {
		lay_info = &(pdata[i]->config.info);
		if ((lay_info->alpha_mode == 1)
			|| (lay_info->alpha_mode == 2))
			chn_info->glb_alpha = lay_info->alpha_value;
		if (lay_info->mode == LAYER_MODE_BUFFER) {
			if (lay_info->fb.pre_multiply) {
				lay_premul_num++;
				chn_info->lay_premul[i] = DE_OVL_PREMUL_HAS_PREMUL;
			} else {
				chn_info->lay_premul[i] = DE_OVL_PREMUL_NON_TO_NON;
			}
		}
	}

	if ((lay_premul_num == 0)
		&& ((ctx->min_z_chn_id != chn)
		|| chn_info->fbd_en
		|| chn_info->atw_en)) {
		chn_info->alpha_mode &= ~DE_ALPHA_MODE_PREMUL;
	} else if ((lay_premul_num < layer_num)
		|| (ctx->min_z_chn_id == chn)) {
		for (i = 0; i < layer_num; ++i) {
			lay_info = &(pdata[i]->config.info);
			if ((lay_info->mode != LAYER_MODE_BUFFER)
				|| (!lay_info->fb.pre_multiply)) {
				chn_info->lay_premul[i] = DE_OVL_PREMUL_NON_TO_EXC;
			}
		}
		chn_info->alpha_mode |= DE_ALPHA_MODE_PREMUL;
	} else {
		chn_info->alpha_mode |= DE_ALPHA_MODE_PREMUL;
	}
}

static s32 de_rtmx_chn_calc_size(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;

	struct de_rect_s crop32[MAX_LAYER_NUM_PER_CHN];
	struct de_scale_para ypara[MAX_LAYER_NUM_PER_CHN];
	struct de_scale_para cpara[MAX_LAYER_NUM_PER_CHN];

	struct de_rect win;
	u32 i;

	win.left = win.top = 0x7FFFFFFF;
	win.right = win.bottom = 0x0;
	for (i = 0; i < layer_num; ++i) {
		struct de_rect_s *lay_scn_win = (struct de_rect_s *)
			&(pdata[i]->config.info.screen_win);
		s32 right, bottom;

		right = lay_scn_win->left + lay_scn_win->width;
		bottom = lay_scn_win->top + lay_scn_win->height;
		win.left = min(win.left, lay_scn_win->left);
		win.top = min(win.top, lay_scn_win->top);
		win.right = max(win.right, right);
		win.bottom = max(win.bottom, bottom);
	}
	chn_info->scn_win.left = win.left;
	chn_info->scn_win.top = win.top;
	chn_info->scn_win.width = win.right - win.left;
	chn_info->scn_win.height = win.bottom - win.top;

	win.left = win.top = 0x7FFFFFFF;
	win.right = win.bottom = 0x0;
	for (i = 0; i < layer_num; ++i) {
		struct disp_layer_info_inner *lay_info =
			&(pdata[i]->config.info);
		struct de_rect_s *lay_win = &(chn_info->lay_win[i]);
		struct de_rect64_s crop64;

		memcpy((void *)&crop64, (void *)&(lay_info->fb.crop), sizeof(crop64));

		de_vsu_calc_lay_scale_para(disp, chn,
			chn_info->px_fmt_space,	chn_info,
			(struct de_rect64_s *)&crop64,
			(struct de_rect_s *)&(lay_info->screen_win),
			&crop32[i], &ypara[i], &cpara[i]);

		lay_win->left = de_vsu_calc_ovl_coord(disp, chn,
			lay_info->screen_win.x - chn_info->scn_win.left,
			ypara[i].hstep);
		lay_win->top = de_vsu_calc_ovl_coord(disp, chn,
			lay_info->screen_win.y - chn_info->scn_win.top,
			ypara[i].vstep);
		lay_win->width = crop32[i].width;
		lay_win->height = crop32[i].height;
		win.left = min(lay_win->left, win.left);
		win.top = min(lay_win->top, win.top);
		win.right = max((s32)(lay_win->left + lay_win->width),
			win.right);
		win.bottom = max((s32)(lay_win->top + lay_win->height),
			win.bottom);
	}
	chn_info->ovl_out_win.left = win.left;
	chn_info->ovl_out_win.top = win.top;
	chn_info->ovl_out_win.width = win.right - win.left;
	chn_info->ovl_out_win.height = win.bottom - win.top;
	memcpy((void *)&(chn_info->ovl_win),
		(void *)&(chn_info->ovl_out_win),
		sizeof(chn_info->ovl_win));

	de_vsu_calc_ovl_scale_para(layer_num, ypara, cpara,
		&chn_info->ovl_ypara, &chn_info->ovl_cpara);

	return 0;
}

static u32 de_rtmx_chn_fix_size_for_ability(u32 disp, u32 chn)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	struct de_output_info *output = &(ctx->output);
	u32 tmp;
	u64 update_speed_ability, required_speed_ability;

	/*
	* lcd_freq_MHz = lcd_width*lcd_height*lcd_fps/1000000;
	* how many overlay line can be fetched during scanning
	*	one lcd line(consider 80% dram efficiency)
	* update_speed_ability = lcd_width*de_freq_mhz*125/(ovl_w*lcd_freq_MHz);
	*/
	tmp = output->scn_height * output->fps *
		((chn_info->ovl_win.width > chn_info->scn_win.width)
		? chn_info->ovl_win.width : chn_info->scn_win.width);
	if (tmp != 0) {
		update_speed_ability = ctx->clk_rate_hz * 80;
		do_div(update_speed_ability, tmp);
	} else {
		return 0;
	}

	/* how many overlay line need to fetch during scanning one lcd line */
	required_speed_ability = (chn_info->scn_win.height == 0) ? 0 :
		(chn_info->ovl_out_win.height * 100 / chn_info->scn_win.height);

	DE_INF("de_freq_hz=%lld, tmp=%d,vsu_outh=%d,ovl_h=%d\n",
		ctx->clk_rate_hz, tmp, chn_info->scn_win.height,
		chn_info->ovl_out_win.height);
	DE_INF("ability: %lld,%lld\n",
		update_speed_ability, required_speed_ability);

	if (update_speed_ability < required_speed_ability) {
		/* if ability < required, use coarse scale */
		u64 tmp2 = update_speed_ability * chn_info->scn_win.height;

		do_div(tmp2, 100);
		chn_info->ovl_out_win.height = (u32)tmp2;
		return RTMX_CUT_INHEIGHT;
	}

	return 0;
}

static s32 de_rtmx_chn_fix_size(u32 disp, u32 chn,
	struct disp_layer_config_data **const pdata, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	u32 fix_size_result = 0;

	if (((chn_info->px_fmt_space == DE_FORMAT_SPACE_RGB)
		|| ((chn_info->px_fmt_space == DE_FORMAT_SPACE_YUV)
		&& (chn_info->yuv_sampling == DE_YUV444)))
		&& (chn_info->ovl_out_win.height == chn_info->scn_win.height)
		&& (chn_info->ovl_out_win.width == chn_info->scn_win.width)) {
		chn_info->scale_en = 0;
	} else {
		chn_info->scale_en = 1;
	}

	if (chn_info->scale_en)
		fix_size_result |= de_vsu_fix_tiny_size(disp, chn,
			&(chn_info->ovl_out_win), &(chn_info->scn_win),
			&(chn_info->ovl_ypara), chn_info->px_fmt_space,
			chn_info->lay_win, layer_num,
			ctx->output.scn_width, ctx->output.scn_height);

	if (!chn_info->fbd_en) {
		fix_size_result |= de_vsu_fix_big_size(disp, chn,
			&(chn_info->ovl_out_win), &(chn_info->scn_win),
			chn_info->px_fmt_space, chn_info->yuv_sampling);
		fix_size_result |= de_rtmx_chn_fix_size_for_ability(disp, chn);
	}

	de_vsu_calc_scale_para(fix_size_result,
		chn_info->px_fmt_space, chn_info->yuv_sampling,
		&(chn_info->scn_win),
		&(chn_info->ovl_out_win), &(chn_info->c_win),
		&(chn_info->ovl_ypara), &(chn_info->ovl_cpara));

	return 0;
}

static s32 de_rtmx_chn_apply_csc(u32 disp, u32 chn,
	struct de_rtmx_context *ctx, struct de_chn_info *chn_info)
{
	struct de_csc_info icsc_info;
	struct de_csc_info ocsc_info;

	icsc_info.px_fmt_space = chn_info->px_fmt_space;
	icsc_info.color_space = chn_info->color_space;
	icsc_info.color_range = chn_info->color_range;
	icsc_info.eotf = chn_info->eotf;
	icsc_info.hdr_type = chn_info->hdr_type;
	icsc_info.pMeta = chn_info->pMeta;

	/* cdc */
	if (chn_info->cdc_hdl != NULL) {
		ocsc_info.px_fmt_space = icsc_info.px_fmt_space;
		ocsc_info.color_range = icsc_info.color_range;
		ocsc_info.color_space = ctx->output.color_space;
		ocsc_info.eotf = ctx->output.eotf;
		ocsc_info.hdr_type = ctx->output.hdr_type;
		de_cdc_set_para(chn_info->cdc_hdl, &icsc_info, &ocsc_info);
		memcpy((void *)&icsc_info, (void *)&ocsc_info,
			sizeof(icsc_info));
	}

	/* csc in vep */
	if (de_feat_is_support_vep_by_chn(disp, chn)) {
		/* iCSC in FCE */
		if (icsc_info.px_fmt_space == DE_FORMAT_SPACE_RGB) {
			ocsc_info.px_fmt_space = DE_FORMAT_SPACE_YUV;
			ocsc_info.color_space = DE_COLOR_SPACE_BT709;
		}
		ocsc_info.color_range = DE_COLOR_RANGE_0_255;
		de_fce_set_icsc_coeff(disp, chn, &icsc_info, &ocsc_info);
		memcpy((void *)&icsc_info, (void *)&ocsc_info,
			sizeof(icsc_info));

		/* iCSC & oCSC in FCC */
		ocsc_info.px_fmt_space = DE_FORMAT_SPACE_RGB;
		ocsc_info.color_space = DE_COLOR_SPACE_BT709;
		ocsc_info.color_range = DE_COLOR_RANGE_0_255;
		de_fcc_set_icsc_coeff(disp, chn, &icsc_info, &ocsc_info);
		de_fcc_set_ocsc_coeff(disp, chn, &ocsc_info, &icsc_info);
	}

	/* ccsc */
	ocsc_info.px_fmt_space = ctx->output.px_fmt_space;
	ocsc_info.color_range = ctx->output.color_range;
	ocsc_info.color_space = ctx->output.color_space;
	ocsc_info.eotf = ctx->output.eotf;
	de_ccsc_apply(disp, chn, &icsc_info, &ocsc_info);

	return 0;
}

int de_rtmx_get_3d_in_single_size(enum de_3d_in_mode inmode,
	struct disp_rect64 *size)
{

	switch (inmode) {
	case DE_3D_SRC_MODE_TB:
		/*size->width = size->width;*/
		size->height = size->height >> 1;
		/*size->x = size->x;
		size->y = size->y;*/
		break;
	case DE_3D_SRC_MODE_SSF:
	case DE_3D_SRC_MODE_SSH:
		size->width = size->width >> 1;
		/*size->height = size->height;
		size->x = size->x;
		size->y = size->y;*/
		break;
	case DE_3D_SRC_MODE_LI:
		/*size->width = size->width;*/
		size->height = size->height >> 1;
		/*size->x = size->x;*/
		size->y = size->y >> 1;
		break;
	case DE_3D_SRC_MODE_FP:
		/*size->width = size->width;
		size->height = size->height;
		size->x = size->x;
		size->y = size->y;*/
		break;
	default:
		/* undefine input mode */
		break;
	}

	return 0;
}

int de_rtmx_get_3d_out(struct disp_rect frame0, unsigned int scn_w,
			unsigned int scn_h, enum de_3d_out_mode trdoutmode,
			struct disp_rect *frame1)
{
	switch (trdoutmode) {
	case DE_3D_OUT_MODE_CI_1:
	case DE_3D_OUT_MODE_CI_2:
	case DE_3D_OUT_MODE_CI_3:
	case DE_3D_OUT_MODE_CI_4:
	case DE_3D_OUT_MODE_SSF:
	case DE_3D_OUT_MODE_SSH:
		{
			frame1->x = frame0.x + scn_w;
			frame1->y = frame0.y;
			frame1->width = frame0.width;
			frame1->height = frame0.height;
			break;
		}
	case DE_3D_OUT_MODE_TB:
	case DE_3D_OUT_MODE_FP:
		{
			frame1->x = frame0.x;
			frame1->y = frame0.y + (scn_h >> 1);
			frame1->width = frame0.width;
			frame1->height = frame0.height;
			break;
		}
	case DE_3D_OUT_MODE_LI:
		{
			frame1->x = frame0.x + scn_w;
			frame1->y = frame0.y;
			frame1->width = frame0.width;
			frame1->height = frame0.height;
			break;
		}
	case DE_3D_OUT_MODE_LIRGB:
		{
			frame1->x = frame0.x;
			frame1->y = frame0.y + scn_h;
			frame1->width = frame0.width;
			frame1->height = frame0.height;
			break;
		}
	default:
		break;
	}
	DE_INF("frame0=<%d,%d,%d,%d>\n", frame0.x, frame0.y, frame0.width,
			frame0.height);
	DE_INF("frame1=<%d,%d,%d,%d>\n", frame1->x, frame1->y, frame1->width,
			frame1->height);

	return 0;
}

int de_rtmx_get_3d_in(unsigned char fmt, struct disp_rect64 crop,
			struct disp_rectsz *size, unsigned int *align,
			enum de_3d_in_mode trdinmode, unsigned long long *addr,
			unsigned int *trd_addr, unsigned long long *lay_laddr)
{
	unsigned int ycnt, ucnt, data_type;
	unsigned int image_w0, image_w1, image_w2, image_h0, image_h1, image_h2;
	unsigned int pitch[3];
	unsigned int cropw = (unsigned int)(crop.width >> 32);
	unsigned int croph = (unsigned int)(crop.height >> 32);

	ycnt = ucnt = 0;
	if (trdinmode == DE_3D_SRC_NORMAL)
		return -1;

	if (fmt <= DE_FORMAT_BGRX_8888) {
		ycnt = 4;
		data_type = 0x0;
	} else if (fmt <= DE_FORMAT_BGR_888) {
		ycnt = 3;
		data_type = 0x0;
	} else if (fmt <= DE_FORMAT_BGRA_5551) {
		ycnt = 2;
		data_type = 0x0;
	} else if (fmt <= DE_FORMAT_BGRA_1010102) {
		ycnt = 4;
		data_type = 0x0;
	}
	/* YUV444 */
	else if (fmt <= DISP_FORMAT_YUV444_I_VUYA) {
		ycnt = 4;
		data_type = 0x0;
	}
	/* YUV422 */
	else if (fmt <= DE_FORMAT_YUV422_I_VYUY) {
		ycnt = 2;
		data_type = 0x0;
		ucnt = 0;
	} else if (fmt == DE_FORMAT_YUV422_P) {
		ycnt = 1;
		data_type = 0x2;
		ucnt = 1;
	}
	/* YUV422 */
	else if (fmt == DE_FORMAT_YUV420_P) {
		ycnt = 1;
		data_type = 0x2;
		ucnt = 1;
	}
	/* YUV420 */
	else if (fmt == DE_FORMAT_YUV411_P) {
		ycnt = 1;
		data_type = 0x2;
		ucnt = 1;
	}
	/* YUV411 */
	else if (fmt <= DE_FORMAT_YUV422_SP_VUVU) {
		ycnt = 1;
		data_type = 0x1;
		ucnt = 2;
	} else if (fmt <= DE_FORMAT_YUV420_SP_VUVU) {
		ycnt = 1;
		data_type = 0x1;
		ucnt = 2;
	} else if (fmt <= DE_FORMAT_YUV411_SP_VUVU) {
		ycnt = 1;
		data_type = 0x1;
		ucnt = 2;
	} else if ((fmt == DE_FORMAT_YUV420_SP_UVUV_10BIT) ||
		   (fmt == DE_FORMAT_YUV420_SP_VUVU_10BIT)) {
		ycnt = 2;
		data_type = 0x1;
		ucnt = 4;
	} else if ((fmt == DE_FORMAT_YUV422_SP_UVUV_10BIT) ||
		   (fmt == DE_FORMAT_YUV422_SP_VUVU_10BIT)) {
		ycnt = 2;
		data_type = 0x1;
		ucnt = 4;
	} else {
		ycnt = 4;
		data_type = 0x0;
	}

	switch (fmt) {
	case DE_FORMAT_YUV420_P:
	case DE_FORMAT_YUV420_SP_VUVU:
	case DE_FORMAT_YUV420_SP_UVUV:
	case DE_FORMAT_YUV420_SP_VUVU_10BIT:
	case DE_FORMAT_YUV420_SP_UVUV_10BIT:
		image_w0 = cropw;
		image_w2 = image_w1 = cropw >> 1;
		image_h0 = croph;
		image_h2 = image_h1 = croph >> 1;
		break;
	case DE_FORMAT_YUV422_P:
	case DE_FORMAT_YUV422_SP_VUVU:
	case DE_FORMAT_YUV422_SP_UVUV:
	case DE_FORMAT_YUV422_SP_VUVU_10BIT:
	case DE_FORMAT_YUV422_SP_UVUV_10BIT:
		image_w0 = cropw;
		image_w2 = image_w1 = cropw >> 1;
		image_h0 = croph;
		image_h2 = image_h1 = image_h0;
		break;
	case DE_FORMAT_YUV411_P:
	case DE_FORMAT_YUV411_SP_VUVU:
	case DE_FORMAT_YUV411_SP_UVUV:
		image_w0 = cropw;
		image_w2 = image_w1 = cropw >> 2;
		image_h0 = croph;
		image_h2 = image_h1 = image_h0;
		break;
	default:
		image_w0 = cropw;
		image_w2 = image_w1 = image_w0;
		image_h0 = croph;
		image_h2 = image_h1 = image_h0;
		break;
	}
	/* planar */
	if (data_type == 0x2) {
		if (trdinmode == DE_3D_SRC_MODE_TB) {
			pitch[0] = DISPALIGN(size[0].width, align[0]);
			pitch[1] = DISPALIGN(size[1].width, align[1]);
			pitch[2] = DISPALIGN(size[2].width, align[2]);

			lay_laddr[0] = pitch[0] * image_h0 + addr[0];
			lay_laddr[1] = pitch[1] * image_h1 + addr[1];
			lay_laddr[2] = pitch[2] * image_h2 + addr[2];

		} else if (trdinmode == DE_3D_SRC_MODE_FP) {
			pitch[0] = DISPALIGN(size[0].width, align[0]);
			pitch[1] = DISPALIGN(size[1].width, align[1]);
			pitch[2] = DISPALIGN(size[2].width, align[2]);

			lay_laddr[0] = trd_addr[0];
			lay_laddr[1] = trd_addr[1];
			lay_laddr[2] = trd_addr[2];

		} else if ((trdinmode == DE_3D_SRC_MODE_SSF)
			   || (trdinmode == DE_3D_SRC_MODE_SSH)) {
			pitch[0] = DISPALIGN(size[0].width, align[0]);
			pitch[1] = DISPALIGN(size[1].width, align[1]);
			pitch[2] = DISPALIGN(size[2].width, align[2]);

			lay_laddr[0] = image_w0 + addr[0];
			lay_laddr[1] = image_w1 + addr[1];
			lay_laddr[2] = image_w2 + addr[2];
		} else if (trdinmode == DE_3D_SRC_MODE_LI) {
			pitch[0] = (DISPALIGN(size[0].width, align[0])) << 1;
			pitch[1] = (DISPALIGN(size[1].width, align[1])) << 1;
			pitch[2] = (DISPALIGN(size[2].width, align[2])) << 1;

			lay_laddr[0] = DISPALIGN(size[0].width, align[0]) + addr[0];
			lay_laddr[1] = DISPALIGN(size[1].width, align[1]) + addr[1];
			lay_laddr[2] = DISPALIGN(size[2].width, align[2]) + addr[2];
		}
	} else if (data_type == 0x0)
		/* interleaved */
	{
		if (trdinmode == DE_3D_SRC_MODE_FP) {
			pitch[0] = DISPALIGN(size[0].width * ycnt, align[0]) / ycnt;
			pitch[1] = 0;
			pitch[2] = 0;

			lay_laddr[0] = trd_addr[0];
			lay_laddr[1] = 0;
			lay_laddr[2] = 0;
		} else if (trdinmode == DE_3D_SRC_MODE_TB) {
			pitch[0] = DISPALIGN(size[0].width * ycnt, align[0]) / ycnt;
			pitch[1] = 0;
			pitch[2] = 0;

			lay_laddr[0] = pitch[0] * image_h0 * ycnt + addr[0];
			lay_laddr[1] = 0;
			lay_laddr[2] = 0;
		} else if ((trdinmode == DE_3D_SRC_MODE_SSF)
			   || (trdinmode == DE_3D_SRC_MODE_SSH)) {

			pitch[0] = DISPALIGN(size[0].width * ycnt, align[0]) / ycnt;
			pitch[1] = 0;
			pitch[2] = 0;

			lay_laddr[0] = image_w0 * ycnt + addr[0];
			lay_laddr[1] = 0;
			lay_laddr[2] = 0;
		} else if (trdinmode == DE_3D_SRC_MODE_LI) {
			pitch[0] = (DISPALIGN(size[0].width, align[0])) << 1;
			pitch[1] = 0;
			pitch[2] = 0;

			lay_laddr[0] =
			    (DISPALIGN(size[0].width, align[0])) * ycnt + addr[0];
			lay_laddr[1] = 0;
			lay_laddr[2] = 0;
		}
	} else if (data_type == 0x1)
		/* semi-planar */
	{
		if ((trdinmode == DE_3D_SRC_MODE_SSF)
		    || (trdinmode == DE_3D_SRC_MODE_SSH)) {
			pitch[0] = DISPALIGN(size[0].width, align[0]);
			pitch[1] = DISPALIGN(size[1].width, align[1]);
			pitch[2] = 0;

			lay_laddr[0] = image_w0 + addr[0];
			lay_laddr[1] = (image_w1 * ucnt) + addr[1];
			lay_laddr[2] = 0;
		} else if (trdinmode == DE_3D_SRC_MODE_TB) {
			pitch[0] = DISPALIGN(size[0].width, align[0]);
			pitch[1] = DISPALIGN(size[1].width, align[1]);
			pitch[2] = 0;
			lay_laddr[0] = pitch[0] * image_h0 + addr[0];
			lay_laddr[1] = pitch[1] * image_h1 * ucnt + addr[1];
			lay_laddr[2] = 0;
		} else if (trdinmode == DE_3D_SRC_MODE_FP) {
			pitch[0] = DISPALIGN(size[0].width, align[0]);
			pitch[1] = DISPALIGN(size[1].width, align[1]);
			pitch[2] = 0;

			lay_laddr[0] = trd_addr[0];
			lay_laddr[1] = trd_addr[1];
			lay_laddr[2] = 0;
		} else if (trdinmode == DE_3D_SRC_MODE_LI) {
			pitch[0] = (DISPALIGN(size[0].width, align[0])) << 1;
			pitch[1] = (DISPALIGN(size[1].width, align[1])) << 1;
			pitch[2] = 0;

			lay_laddr[0] = addr[0] + DISPALIGN(size[1].width, align[1]);
			lay_laddr[1] =
			    addr[1] + (DISPALIGN(size[1].width, align[1])) * ucnt;
			lay_laddr[2] = 0;
		}
	}

	return 0;
}

static s32 de_rtmx_chn_layer_apply(u32 disp, u32 chn,
	struct disp_layer_config_data *data, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;
	u32 pdata_num = 0;
	struct disp_layer_config_data *pdata[MAX_LAYER_NUM_PER_CHN];
	struct disp_layer_config_data *data_end;
	u32 dirty_flag = 0;
	u8 enable = 0;
	u8 fbd_en = 0;
	u8 atw_en = 0;
	u8 snr_en = 0;
	enum disp_hdr_type hdr_type = HDR10;
	struct sunxi_metadata    *pMetadata = NULL;

	data_end = data + layer_num;
	for (; data != data_end; ++data) {
		if (data->config.channel != chn)
			continue;
		if (data->flag)
			dirty_flag = 1;
		if (data->config.enable) {
			if (pdata_num >= MAX_LAYER_NUM_PER_CHN) {
				DE_WRN("user layer num overflow on chn%d\n",
					chn);
				break;
			}
			pdata[pdata_num] = data;
			++pdata_num;

			enable++;
			if (data->config.info.fb.fbd_en)
				fbd_en++;
			if (data->config.info.atw.used)
				atw_en++;
			if (data->config.info.snr.en)
				snr_en++;
			if (data->config.info.fb.metadata_flag
				& SUNXI_METADATA_FLAG_HDR10P_METADATA) {
				hdr_type = HDR10P;
				pMetadata = data->config.info.fb.p_metadata;
			}
			/* 3d mode*/
			if (data->config.info.fb.flags) {
				memcpy(&(data + 1)->config.info.fb.crop,
						&data->config.info.fb.crop,
						sizeof(struct disp_rect64));
				de_rtmx_get_3d_in_single_size(
						(enum de_3d_in_mode)
						data->config.info.fb.flags,
						&data->config.info.fb.crop);
				if (data->config.info.b_trd_out) {
					de_rtmx_get_3d_in_single_size(
							(enum de_3d_in_mode)
							data->config.info.fb.flags,
							&(data + 1)->config.info.fb.crop);
					de_rtmx_get_3d_out(data->config.info.screen_win,
							ctx->output.scn_width,
							ctx->output.scn_height,
							(enum de_3d_out_mode)
							data->config.info.out_trd_mode,
							&(data + 1)->config.info.screen_win);
					(data + 1)->config.enable = data->config.enable;
					if ((data + 1)->config.enable)
						(data + 1)->config.info.fb.fbd_en =
								data->config.info.fb.fbd_en;
					(data + 1)->config.info.alpha_mode =
							data->config.info.alpha_mode;
					(data + 1)->config.info.alpha_value =
							data->config.info.alpha_value;
					(data + 1)->config.info.mode =
							data->config.info.mode;
					(data + 1)->config.info.fb.format =
							data->config.info.fb.format;
					(data + 1)->config.info.fb.pre_multiply =
							data->config.info.fb.pre_multiply;
					memcpy((data + 1)->config.info.fb.size,
							data->config.info.fb.size,
							sizeof(struct disp_rectsz) * 3);
					de_rtmx_get_3d_in(data->config.info.fb.format,
							(data + 1)->config.info.fb.crop,
							data->config.info.fb.size,
							data->config.info.fb.align,
							(enum de_3d_in_mode)data->config.
							info.fb.flags,
							data->config.info.fb.addr,
							data->config.info.fb.trd_right_addr,
							(data + 1)->config.info.fb.addr);

					(data + 1)->flag = data->flag;
				} else {
					/*(data + 1)->config.enable = 0;*/
				}
			}
		} else {
			if (data->flag)
				de_ovl_disable_lay(disp, chn, data->config.layer_id);
		}
	}
	chn_info->flag = dirty_flag;
	chn_info->enable = enable;
	chn_info->fbd_en = fbd_en;
	chn_info->atw_en = atw_en;
	chn_info->snr_en = snr_en;
	chn_info->hdr_type = hdr_type;
	if (HDR10P == hdr_type) {
		chn_info->pMeta = pMetadata;
	}
	if (!dirty_flag)
		return 0;
	if (!enable) {
		if (de_feat_is_support_fbd_by_layer(disp, chn, 0))
			de_fbd_atw_disable(disp, chn);
		de_vsu_disable(disp, chn);
		if (chn_info->cdc_hdl)
			de_cdc_disable(chn_info->cdc_hdl);
		if (de_feat_is_support_vep_by_chn(disp, chn)) {
			/* todo: disable vep */
		}
		de_ccsc_enable(disp, chn, 0);
		return 0;
	}


	de_rtmx_chn_data_attr(chn_info, pdata, pdata_num);
	de_rtmx_chn_blend_attr(disp, chn, pdata, pdata_num);
	de_rtmx_chn_calc_size(disp, chn, pdata, pdata_num);
	de_rtmx_chn_fix_size(disp, chn, pdata, pdata_num);
	ctx->output.color_range = chn_info->color_range;

	if (chn_info->fbd_en || chn_info->atw_en) {
		de_ovl_disable(disp, chn);
		de_fbd_atw_apply_lay(disp, chn, pdata, pdata_num);
	} else {
		de_fbd_atw_disable(disp, chn);
		de_ovl_apply_lay(disp, chn, pdata, pdata_num);
	}
	de_snr_set_para(disp, chn, pdata, pdata_num);

	if (chn_info->scale_en) {
		de_vsu_set_para(disp, chn, chn_info);
	} else {
		de_vsu_disable(disp, chn);
	}

	de_rtmx_chn_apply_csc(disp, chn, ctx, chn_info);
	if (ctx->output.cvbs_direct_show == true)
		de_ccsc_enable(disp, chn, 0);

	return 0;
}

static void de_rtmx_pipe_info(
	struct de_rtmx_context *ctx,
	struct de_bld_pipe_info *pipe_info, u32 *pipe_num)
{
	u32 num;
	u32 pipe_id = 0;
	u8 chn_routed[MAX_CHN_NUM];

	memset((void *)chn_routed, 0, sizeof(chn_routed));
	for (num = 0; num < *pipe_num; ++num) {
		struct de_chn_info *chn_info = ctx->chn_info;
		u8 min_zorder = 0xFF;
		u8 chn_finded = 0;
		u32 finded_chn_id;
		u32 chn;

		for (chn = 0; chn < ctx->chn_num; ++chn, ++chn_info) {
			if (!chn_info->enable || chn_routed[chn])
				continue;
			if (min_zorder > chn_info->min_zorder) {
				min_zorder = chn_info->min_zorder;
				chn_finded = 1;
				finded_chn_id = chn;
			}
		}
		if (chn_finded) {
			struct de_bld_pipe_info *pinfo = pipe_info + pipe_id;

			chn_info = ctx->chn_info + finded_chn_id;
			pinfo->enable = (ctx->be_blank) ? 0 : 1;
			pinfo->chn = finded_chn_id;
			pinfo->premul_ctl = (chn_info->alpha_mode
				& DE_ALPHA_MODE_PREMUL_MASK) ? 1 : 0;
			chn_routed[finded_chn_id] = 1;

			pipe_id++;
		} else {
			break;
		}
	}
	*pipe_num = pipe_id;
}

static s32 de_rtmx_chn_zorder(u32 disp,
	struct disp_layer_config_data *const data, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	u32 i;
	u8 min_zorder = 0xFF;

	for (i = 0; i < ctx->chn_num; ++i)
		ctx->chn_info[i].min_zorder = 0xFF;
	ctx->min_z_chn_id = ctx->chn_num;

	for (i = 0; i < layer_num; ++i) {
		struct disp_layer_config_data *p = data + i;
		u32 chn_id;

		if (p->config.enable == 0)
			continue;
		chn_id = p->config.channel;
		if (ctx->chn_info[chn_id].min_zorder > p->config.info.zorder)
			ctx->chn_info[chn_id].min_zorder = p->config.info.zorder;
		if (min_zorder > p->config.info.zorder) {
			min_zorder = p->config.info.zorder;
			ctx->min_z_chn_id = chn_id;
		}
	}

	return 0;
}

/**
 * @name       :disp_al_validate_direct_show
 * @brief      :whether open direct show function or not
 * @param[IN]  :disp:screen id
 * @param[IN]  :data:layer info
 * @param[IN]  :layer_num:number of layer
 * @return     :ture if open driect show function
 */
static bool disp_al_validate_direct_show(unsigned int disp,
			struct disp_layer_config_data *data,
			unsigned int layer_num)
{

	bool direct_show = true;
	unsigned char i, vi_chn;
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);

#define CVBS_PAL_WIDTH 720
#define CVBS_PAL_HEIGHT 576
#define CVBS_NTSC_WIDTH 720
#define CVBS_NTSC_HEIGHT 480
#define CVBS_NTSC_FPS 60
#define CVBS_PAL_FPS 50

#define P2P_FB_MIN_WIDTH 704
#define P2P_FB_MAX_WIDTH 736


	/*
	 * We need to check several conditions:
	 * 1, the output must be TV(PAL/NTSC) type
	 * 2, the output must be in yuv color space,
	 * 3, only video channels are used,
	 * 4, input format must be yuv format.
	 * 5, input layer size must be in the p2p limit range
	 * 6, the layer must be display on the fullscreen or non-scalely
	 */

	if ((ctx->output.scn_width != CVBS_PAL_WIDTH)
	    && (ctx->output.scn_width != CVBS_NTSC_WIDTH)) {
		direct_show = false;
		goto exit;
	}

	if ((ctx->output.scn_height != CVBS_PAL_HEIGHT)
	    && (ctx->output.scn_height != CVBS_NTSC_HEIGHT)) {
		direct_show = false;
		goto exit;
	}

	if ((ctx->output.fps != CVBS_PAL_FPS) &&
	    (ctx->output.fps != CVBS_NTSC_FPS)) {
		direct_show = false;
		goto exit;
	}

	/*check if the output csc of DE is yuv*/
	if (ctx->output.px_fmt_space != DE_FORMAT_SPACE_YUV) {
		direct_show = false;
		goto exit;
	}

	vi_chn = de_feat_get_num_vi_chns(disp);
	for (i = 0; i < layer_num; i++, data++) {
		struct disp_rect64 *crop64;
		struct disp_rect crop;
		struct disp_rect *screen_win;

		if (!data->config.enable)
			continue;

		if (data->config.channel >= vi_chn) {
			direct_show = false;
			goto exit;
		}

		if (data->config.info.fb.format < DISP_FORMAT_YUV444_I_AYUV) {
			direct_show = false;
			goto exit;
		}

		crop64 = &data->config.info.fb.crop;
		crop.x = crop64->x >> 32;
		crop.y = crop64->y >> 32;
		crop.width = crop64->width >> 32;
		crop.height = crop64->height >> 32;
		if ((crop.width < P2P_FB_MIN_WIDTH)
		    || (crop.width > P2P_FB_MAX_WIDTH)) {
			direct_show = false;
			goto exit;
		}

		if (crop.height != ctx->output.scn_height) {
			direct_show = false;
			goto exit;
		}

		screen_win = &data->config.info.screen_win;
		if ((screen_win->width != ctx->output.scn_width)
		    && (screen_win->width != crop.width)) {
			direct_show = false;
			goto exit;
		}

		if ((screen_win->height != ctx->output.scn_height)
		    && (screen_win->height != crop.height)) {
			direct_show = false;
			goto exit;
		}
	}

exit:

	return direct_show;
}

int de_rtmx_lyr_apply_direct_show(unsigned int screen_id,
			struct disp_layer_config_data *data,
			unsigned int layer_num, bool direct_show)
{
	unsigned int display_width, display_height;
	unsigned int i;
	struct de_rtmx_context *ctx = de_rtmx_get_context(screen_id);

	if (!direct_show)
		return 0;

	display_width = ctx->output.scn_width;
	display_height = ctx->output.scn_height;

	for (i = 0; i < layer_num; i++, data++) {
		struct disp_rect64 *crop64;
		struct disp_rect crop;
		struct disp_rect frame;
		struct disp_rect *screen_win;

		if (!data->config.enable)
			continue;

		crop64 = &data->config.info.fb.crop;
		screen_win = &data->config.info.screen_win;
		crop.x = crop64->x >> 32;
		crop.y = crop64->y >> 32;
		crop.width = crop64->width >> 32;
		crop.height = crop64->height >> 32;

		frame.x = 0;
		frame.y = 0;
		/*
		 * If source is larger than screen, crop the source.
		 * And if source is smaller than screen,
		 * make frame para center in the screen
		 */
		if (crop.width > display_width) {
			crop.x = (crop.width - display_width) >> 1;
			crop.width = display_width;
			frame.x = 0;
		} else {
			crop.x = 0;
			frame.x = (display_width - crop.width) >> 1;
		}

		if (crop.height > display_height) {
			crop.y = (crop.height - display_height) >> 1;
			crop.height = display_height;
			frame.y = 0;
		} else {
			crop.y = 0;
			crop.height = (crop.height >> 2) << 2;
			frame.y = (display_height - crop.height) >> 1;
		}

		frame.width = crop.width;
		frame.height = crop.height;

		crop64->x = (long long)crop.x << 32;
		crop64->y = (long long)crop.y << 32;
		crop64->width = (unsigned long long)crop.width
		    << 32;
		crop64->height = (unsigned long long)crop.height
		    << 32;
		screen_win->x = frame.x;
		screen_win->y = frame.y;
		screen_win->width = frame.width;
		screen_win->height = frame.height;
	}

	return 0;
}

s32 de_rtmx_layer_apply(u32 disp,
	struct disp_layer_config_data *const data, u32 layer_num)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	u32 chn_num, chn;
	u32 pipe_num, pipe;

	struct de_bld_pipe_info pipe_info[MAX_CHN_NUM];
	u8 dirty_flag = 0;
	u8 enable = 0;
	ctx->output.cvbs_direct_show = disp_al_validate_direct_show(disp, data, layer_num);

	de_rtmx_lyr_apply_direct_show(disp, data, layer_num, ctx->output.cvbs_direct_show);

	de_rtmx_chn_zorder(disp, data, layer_num);

	chn_num = de_feat_get_num_chns(disp);
	for (chn = 0; chn < chn_num; ++chn)
		de_rtmx_chn_layer_apply(disp, chn, data, layer_num);

	for (chn = 0; chn < chn_num; ++chn) {
		struct de_chn_info *chn_info = ctx->chn_info + chn;

		if (chn_info->flag)
			dirty_flag++;
		if (chn_info->enable)
			enable++;
	}
	if (dirty_flag == 0)
		DE_INF("rtmx%d is not dirty\n", disp);
	if (enable == 0) {
		de_bld_disable(disp);
		return 0;
	}

	pipe_num = sizeof(pipe_info) / sizeof(pipe_info[0]);
	memset((void *)pipe_info, 0, sizeof(pipe_info));
	de_rtmx_pipe_info(ctx, pipe_info, &pipe_num);
	de_bld_set_pipe_ctl(disp, pipe_info,
		sizeof(pipe_info) / sizeof(pipe_info[0]));
	for (pipe = 0; pipe < pipe_num; ++pipe)
		de_bld_set_pipe_attr(disp, pipe_info[pipe].chn, pipe);
	for (pipe = 0; pipe < chn_num - 1; ++pipe)
		de_bld_set_blend_mode(disp, pipe, DE_BLD_MODE_SRCOVER);

	return 0;
}

void de_rtmx_flush_layer_address(u32 disp, u32 chn, u32 layer_id)
{
	de_ovl_flush_layer_address(disp, chn, layer_id);
}

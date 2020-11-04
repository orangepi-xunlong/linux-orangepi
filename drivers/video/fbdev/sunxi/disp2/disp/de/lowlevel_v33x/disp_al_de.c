/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "disp_al_de.h"
#include "./de330/de_feat.h"
#include "./de330/de_rtmx.h"
#include "./de330/de_enhance.h"
#include "./de330/de_smbl.h"
#include "./de330/de_wb.h"
#include "./de330/de_bld.h"

static s32 disp_al_layer_enhance_apply(u32 disp,
	struct disp_layer_config_data *data, u32 layer_num)
{
	struct disp_enhance_chn_info *ehs_info;
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info;
	u32 vi_chn_num = de_feat_get_num_vi_chns(disp);
	u32 i;

	ehs_info = kmalloc(sizeof(struct disp_enhance_chn_info) * vi_chn_num,
			GFP_KERNEL | __GFP_ZERO);
	if (ehs_info == NULL) {
		pr_warn("%s failed to kmalloc!\n", __func__);
		return -1;
	}
	memset((void *)ehs_info, 0, sizeof(struct disp_enhance_chn_info)
			* vi_chn_num);
	for (i = 0; i < layer_num; ++i, ++data) {
		if (data->config.enable
			&& (data->config.channel < vi_chn_num)) {
			struct disp_enhance_layer_info *ehs_layer_info =
					&ehs_info[data->config.channel]
					.layer_info[data->config.layer_id];

			ehs_layer_info->fb_size.width =
				data->config.info.fb.size[0].width;
			ehs_layer_info->fb_size.height =
				data->config.info.fb.size[0].height;
			ehs_layer_info->fb_crop.x =
				data->config.info.fb.crop.x >> 32;
			ehs_layer_info->fb_crop.y =
				data->config.info.fb.crop.y >> 32;
			ehs_layer_info->en = 1;
			ehs_layer_info->format = data->config.info.fb.format;
		}
	}
	for (i = 0; i < vi_chn_num; i++) {
		ehs_info[i].ovl_size.width = chn_info->ovl_out_win.width;
		ehs_info[i].ovl_size.height = chn_info->ovl_out_win.height;
		ehs_info[i].bld_size.width = chn_info->scn_win.width;
		ehs_info[i].bld_size.height = chn_info->scn_win.height;

	}
	/* set enhance size */
	de_enhance_layer_apply(disp, ehs_info);
	kfree(ehs_info);
	return 0;
}

s32 disp_al_layer_apply(u32 disp,
	struct disp_layer_config_data *data, u32 layer_num)
{
#if 0
	pr_warn("flag:%d ch:%d lyr:%d en:%d mode:%d color:%d alpha:%d %d, "
		"zor:%d trd:%d atw_en:%d,scr[%d %d %d %d],fbd_en:%d\n",
		data->flag, data->config.channel, data->config.layer_id,
		data->config.enable, data->config.info.mode,
		data->config.info.color, data->config.info.alpha_mode,
		data->config.info.alpha_value, data->config.info.zorder,
		data->config.info.out_trd_mode, data->config.info.atw.used,
		data->config.info.screen_win.x, data->config.info.screen_win.y,
		data->config.info.screen_win.width,
		data->config.info.screen_win.height,
		data->config.info.fb.fbd_en
		);
	if (data->config.info.fb.fbd_en) {
		pr_warn("fbd header:sig:%d inputbits[%d %d %d %d] "
			"filehdr_size:%d,version:%d,body_size:%d ncompon:%d "
			"header_layout:%d yuv_transform:%d block_split:%d "
			"block_width:%d block_height:%d width:%d height:%d left_crop:%d top_crop:%d block_layout:%d\n",
			data->config.info.fb.p_afbc_header->signature,
			data->config.info.fb.p_afbc_header->inputbits[0],
			data->config.info.fb.p_afbc_header->inputbits[1],
			data->config.info.fb.p_afbc_header->inputbits[2],
			data->config.info.fb.p_afbc_header->inputbits[3],
			data->config.info.fb.p_afbc_header->filehdr_size,
			data->config.info.fb.p_afbc_header->version,
			data->config.info.fb.p_afbc_header->body_size,
			data->config.info.fb.p_afbc_header->ncomponents,
			data->config.info.fb.p_afbc_header->header_layout,
			data->config.info.fb.p_afbc_header->yuv_transform,
			data->config.info.fb.p_afbc_header->block_split,
			data->config.info.fb.p_afbc_header->block_width,
			data->config.info.fb.p_afbc_header->block_height,
			data->config.info.fb.p_afbc_header->width,
			data->config.info.fb.p_afbc_header->height,
			data->config.info.fb.p_afbc_header->left_crop,
			data->config.info.fb.p_afbc_header->top_crop,
			data->config.info.fb.p_afbc_header->block_layout);
	}
#endif
	de_rtmx_layer_apply(disp, data, layer_num);
	disp_al_layer_enhance_apply(disp, data, layer_num);

	return 0;
}

/* this function be called by disp_manager_en */
s32 disp_al_manager_init(u32 disp)
{
	de_rtmx_start(disp);
	return 0;
}

/* this function be called by disp_manager_disable */
s32 disp_al_manager_exit(u32 disp)
{
	de_rtmx_stop(disp);
	return 0;
}

s32 disp_al_manager_apply(u32 disp,
	struct disp_manager_data *data)
{
#if 0
	pr_warn("flag:%d enable:%d hwindex:%d cs:%d eotf:%d ksc_en:%d blank:%d "
		"[%d %d %d %d]\n",
		data->flag, data->config.enable, data->config.hwdev_index,
		data->config.color_space, data->config.eotf,
		data->config.ksc.enable, data->config.blank,
		data->config.size.x, data->config.size.y,
		data->config.size.width, data->config.size.height);
#endif
	de_rtmx_mgr_apply(disp, data);

	return 0;
}

s32 disp_al_manager_sync(u32 disp)
{
	return 0;
}

s32 disp_al_manager_update_regs(u32 disp)
{
	de_rtmx_update_reg_ahb(disp);
	return 0;
}

s32 disp_al_manager_set_rcq_update(u32 disp, u32 en)
{
	return de_rtmx_set_rcq_update(disp, en);
}

s32 disp_al_manager_set_all_rcq_head_dirty(u32 disp, u32 dirty)
{
	return de_rtmx_set_all_rcq_head_dirty(disp, dirty);
}

s32 disp_al_manager_set_irq_enable(
	u32 disp, u32 irq_flag, u32 en)
{
	return de_top_enable_irq(disp, irq_flag & DE_IRQ_FLAG_MASK, en);
}

u32 disp_al_manager_query_irq_state(u32 disp, u32 irq_state)
{
	return de_top_query_state_with_clear(disp, irq_state);
}

s32 disp_al_enhance_apply(u32 disp,
	struct disp_enhance_config *config)
{
	return de_enhance_apply(disp, config);
}

s32 disp_al_enhance_update_regs(u32 disp)
{
	return de_enhance_update_regs(disp);
}

s32 disp_al_enhance_sync(u32 disp)
{
	return de_enhance_sync(disp);
}

s32 disp_al_enhance_tasklet(u32 disp)
{
	return de_enhance_tasklet(disp);
}

s32 disp_al_capture_init(u32 disp)
{
	u32 rt_mux = 0;

	switch (disp) {
	case 0:
		rt_mux = RTWB_MUX_FROM_DSC0;
		break;
	case 1:
		rt_mux = RTWB_MUX_FROM_DSC1;
		break;
	case 2:
		rt_mux = RTWB_MUX_FROM_DSC2;
		break;
	case 3:
		rt_mux = RTWB_MUX_FROM_DSC3;
		break;
	default:
		rt_mux = RTWB_MUX_FROM_DSC0;
		break;
	}
	de_wb_start(0, rt_mux);
	return 0;
}

s32 disp_al_capture_exit(u32 disp)
{
	return de_wb_stop(0);
}

s32 disp_al_capture_apply(u32 disp,
	struct disp_capture_config *cfg)
{
	s32 ret = 0;

	ret = de_wb_apply(0, cfg);
	return ret;
}

s32 disp_al_capture_sync(u32 disp)
{
	if (disp_feat_is_using_wb_rcq(disp))
		de_top_start_rtwb(disp, 1);
	else {
		de_top_start_rtwb(disp, 1);
		de_wb_update_regs_ahb(0);
	}
	return 0;
}

s32 disp_al_capture_get_status(u32 disp)
{
	return de_wb_get_status(0);
}

s32 disp_al_capture_set_rcq_update(u32 disp, u32 en)
{
	return de_wb_set_rcq_update(0, en);
}

s32 disp_al_capture_set_all_rcq_head_dirty(u32 disp, u32 dirty)
{
	return de_wb_set_all_rcq_head_dirty(0, dirty);
}

s32 disp_al_capture_set_mode(u32 disp, enum de_rtwb_mode mode)
{
	return de_top_set_rtwb_mode(disp, mode);
}

s32 disp_al_capture_set_irq_enable(
	u32 disp, u32 irq_flag, u32 en)
{
	if (irq_flag == DISP_AL_CAPTURE_IRQ_FLAG_FRAME_END)
		return de_wb_enable_irq(0, en);
	else
		return de_top_wb_enable_irq(disp, irq_flag & DE_WB_IRQ_FLAG_MASK, en);
}

u32 disp_al_capture_query_irq_state(u32 disp, u32 irq_state)
{
	return de_top_wb_query_state_with_clear(disp, irq_state);
}

s32 disp_al_smbl_apply(u32 disp,
	struct disp_smbl_info *info)
{
	s32 ret = 0;

	ret = de_smbl_apply(disp, info);
	return ret;
}

s32 disp_al_smbl_update_regs(u32 disp)
{
	return de_smbl_update_regs(disp);
}

s32 disp_al_smbl_sync(u32 disp)
{
	return 0;
}

s32 disp_al_smbl_tasklet(u32 disp)
{
	return de_smbl_tasklet(disp);
}

s32 disp_al_smbl_get_status(u32 disp)
{
	return de_smbl_get_status(disp);
}

s32 disp_init_al(struct disp_bsp_init_para *para)
{
	if (de_top_mem_pool_alloc())
		return -1;
	de_enhance_init(para);
	de_smbl_init(para->reg_base[DISP_MOD_DE]);
	de_rtmx_init(para);
	de_wb_init(para);
	return 0;
}

/* ***************************************************** */

s32 disp_al_get_fb_info(u32 disp, struct disp_layer_info *info)
{
	return -1;
}

/* get display output resolution */
s32 disp_al_get_display_size(u32 disp, u32 *width, u32 *height)
{
	return de_bld_get_out_size(disp, width, height);
}

/* Get clk rate of de, unit hz. */
u32 de_get_clk_rate(void)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(0);

	return ctx->clk_rate_hz;
}

int disp_exit_al(void)
{
	/*TODO:free dma or kfree*/
	return 0;
}


bool disp_al_get_direct_show_state(unsigned int disp)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	return ctx->output.cvbs_direct_show;
}

void disp_al_flush_layer_address(u32 disp, u32 chn, u32 layer_id)
{
	de_rtmx_flush_layer_address(disp, chn, layer_id);
}


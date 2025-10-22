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

#include "disp_al_de.h"
#include "./de35x/de_feat.h"
#include "./de35x/de_rtmx.h"
#include "./de35x/de_enhance.h"
#include "./de35x/de_smbl.h"
#include "./de35x/de_wb.h"
#include "./de35x/de_bld.h"

static struct tasklet_struct rcqfinish_tasklet[DE_NUM];

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
		DE_WARN("failed to kmalloc\n");
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
/*
	DE_DEBUG("flag:%d ch:%d lyr:%d en:%d mode:%d color:%d alpha:%d %d, "
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
		DE_WARN("fbd header:sig:%d inputbits[%d %d %d %d] "
			"filehdr_size:%d,version:%d,body_size:%d ncompon:%d "
			"header_layout:%d yuv_transform:%d block_split:%d "
			"block_width:%d block_height:%d width:%d height:%d "
			"left_crop:%d top_crop:%d block_layout:%d\n",
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
*/
	de_rtmx_layer_apply(disp, data, layer_num);
	disp_al_layer_enhance_apply(disp, data, layer_num);

	return 0;
}

static void rcqfinish_tasklet_func(unsigned long data)
{
	u32 disp = (u32)data;
	de_enhance_rcq_finish_tasklet(disp);
	de_smbl_tasklet(disp);
}

/* this function be called by disp_manager_en */
s32 disp_al_manager_init(u32 disp)
{
	de_rtmx_start(disp);
	if (disp >= 0 && disp < DE_NUM) {
		tasklet_init(&rcqfinish_tasklet[disp], rcqfinish_tasklet_func, (unsigned long)disp);
	}
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
	struct disp_csc_config csc_config;
/*
	DE_WARN("flag:%d enable:%d hwindex:%d cs:%d eotf:%d ksc_en:%d blank:%d "
		"[%d %d %d %d]\n",
		data->flag, data->config.enable, data->config.hwdev_index,
		data->config.color_space, data->config.eotf,
		data->config.ksc.enable, data->config.blank,
		data->config.size.x, data->config.size.y,
		data->config.size.width, data->config.size.height);
*/
	de_rtmx_mgr_apply(disp, data);

	if (data->flag & MANAGER_CM_DIRTY) {
		de_dcsc_set_colormatrix(disp,
					data->config.color_matrix,
					data->config.color_matrix_identity);
		de_dcsc_get_config(disp, &csc_config);
		de_dcsc_apply(disp, &csc_config);
	}

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
	u32 disp, u32 irq_flag, u32 irq_type, u32 en)
{
	return de_top_enable_irq(disp, irq_flag & DE_IRQ_FLAG_MASK, en);
}

u32 disp_al_manager_query_irq_state(u32 disp, u32 irq_state, u32 irq_type)
{
	return de_top_query_state_with_clear(disp, irq_state);
}

u32 disp_al_manager_query_is_busy(u32 disp)
{
	return de_top_query_state_is_busy(disp);
}

s32 disp_al_enhance_apply(u32 disp,
	struct disp_enhance_config *config)
{
	struct disp_csc_config csc_config;

	de_dcsc_get_config(disp, &csc_config);
	csc_config.enhance_mode = (config->info.mode >> 16);
	de_dcsc_apply(disp, &csc_config);
	return de_enhance_apply(disp, config);
}

s32 disp_al_enhance_update_regs(u32 disp)
{
	de_rtmx_update_reg_ahb(disp);
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
	u32 hw_disp = de_feat_get_hw_disp(disp);
	switch (hw_disp) {
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
	if (disp_feat_is_using_wb_rcq(disp)) {
	if (SELF_GENERATED_TIMING == disp_al_capture_get_mode(disp))
		de_top_start_rtwb(disp, 1);
	} else {
		if (SELF_GENERATED_TIMING == disp_al_capture_get_mode(disp))
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

enum de_rtwb_mode disp_al_capture_get_mode(u32 disp)
{
	return de_top_get_rtwb_mode(disp);
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
	//return de_smbl_tasklet(disp);
	return 0;
}

s32 disp_al_smbl_get_status(u32 disp)
{
	return de_smbl_get_status(disp);
}

s32 disp_init_al(struct disp_bsp_init_para *para)
{
	if (de_top_mem_pool_alloc())
		return -1;
	/* if (de_top_pingpang_buf_alloc(s32 width, s32 height) < 0) */
		/* return -1; */ // need get screen_win.
	de_enhance_init(para);
	de_dcsc_init(para->reg_base[DISP_MOD_DE]);
	de_smbl_init(para->reg_base[DISP_MOD_DE]);
	de_rtmx_init(para);
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

/* get display chn sampling info*/
s32 disp_al_get_chn_ovl_win(u32 disp, u32 chn, struct de_rect_s *source, struct de_rect_s *output)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_chn_info *chn_info = ctx->chn_info + chn;

	memcpy(source, &chn_info->ovl_win, sizeof(chn_info->ovl_win));
	memcpy(output, &chn_info->ovl_out_win, sizeof(chn_info->ovl_out_win));
	return 0;
}

/* Get clk rate of de, unit hz. */
u32 de_get_clk_rate(void)
{
	struct de_rtmx_context *ctx = de_rtmx_get_context(0);

	return ctx->clk_rate_hz;
}

int disp_exit_al(void)
{
	de_dcsc_exit();
	/* TODO:free dma or kfree */
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
#ifndef CONFIG_AW_DRM
unsigned long de_get_reg_base(u32 sel, u32 *off, int need_update)
{
	unsigned long reg_base = 0x0;

	DE_INFO("invalid cmd for offset %x\n", *off);
	return reg_base;
}

unsigned long disp_al_get_reg_base(u32 sel, u32 *off, int updata)
{
	return de_get_reg_base(sel, off, updata);
}

int disp_al_fcm_proc(u32 screen_id, u32 cmd, fcm_hardware_data_t *data)
{
	int ret;
	switch (cmd) {
	case 0:
		ret = de_fcm_set_lut(screen_id, data, 1);
		break;
	case 1:
		ret = de_fcm_set_lut(screen_id, data, 0);
		break;
	case 2:
		ret = de_fcm_get_lut(screen_id, data);
		break;
	default:
		ret = -1;
		DE_WARN("invalid cmd for fcm %d\n", cmd);
		break;
	}
	return ret;
}
#endif

enum {
	PQ_SET_REG = 0x1,
	PQ_GET_REG = 0x2,
	PQ_ENABLE = 0x3,
	PQ_COLOR_MATRIX = 0x4,
	PQ_FCM = 0x5,
	PQ_CDC = 0x6,
	PQ_DCI = 0x7,
	PQ_DEBAND = 0x8,
	PQ_SHARP35X = 0x9,
	PQ_SNR = 0xa,
	PQ_GTM = 0xb,
	PQ_ASU = 0xc,
};

int disp_al_pq_proc(u32 sel, u32 cmd, u32 subcmd, void *data)
{
	int ret = 0;
	switch (cmd) {
	case PQ_DCI:
		ret = de_dci_pq_proc(sel, cmd, subcmd, data);
		break;
	case PQ_DEBAND:
		ret = de_deband_pq_proc(sel, cmd, subcmd, data);
		break;
	case PQ_SHARP35X:
		ret = de_sharp_pq_proc(sel, cmd, subcmd, data);
		break;
	case PQ_SNR:
		ret = de_snr_pq_proc(sel, cmd, subcmd, data);
		break;
	case PQ_GTM:
		ret = de_gtm_pq_proc(sel, cmd, subcmd, data);
		break;
	case PQ_ASU:
		ret = de_asu_pq_proc(sel, cmd, subcmd, data);
		break;
	}
	return ret;
}

s32 disp_al_rcqfinish_tasklet(u32 disp)
{
	if (disp >= 0 && disp < DE_NUM) {
		tasklet_schedule(&rcqfinish_tasklet[disp]);
	}
	return 0;
}

s32 disp_al_ahb_read_enable(u32 disp, bool enable)
{
	de_smbl_enable_ahb_read(disp, enable);
	de_dci_enable_ahb_read(disp, enable);
	return 0;
}

s32 disp_al_offline_enable(s32 disp, s32 width, s32 height, s32 enable)
{
	s32 size;

	if (disp != 0)
		return -1;

	if (!enable) {
		de_top_set_offline_enable(enable);
		return 0;
	}

	size = PAGE_ALIGN((width * height * 4 * 2));
	if (size != de_top_get_pingpang_buf_size()) {
		de_top_pingpang_buf_alloc(width, height);
	}
	de_top_set_offline_head();
	de_top_set_offline_enable(enable);

	return 0;
}

void *de_al_get_pingpang_buf_addr(void)
{
	return de_top_get_pingpang_vir_addr();
}

s32 de_al_get_pingpang_buf_size(void)
{
	return de_top_get_pingpang_buf_size();
}

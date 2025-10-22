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
#include <linux/mutex.h>
#include <drm/drm_blend.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#else
#include <drm/drm_fb_dma_helper.h>
#include <drm/drm_gem_dma_helper.h>
#endif
#include <drm/drm_fourcc.h>
#include <drm/drm_framebuffer.h>
#include "de_channel.h"
#include "de_ovl.h"
#include "de_scaler.h"
#include "de_csc.h"
#include "de_fbd_atw.h"
#include "de_tfbd.h"
#include "de_frontend.h"

/*
 *             +-  ovl0 -+
 *  channel0: -|- afbc0 -|-- SNR0 -- scaler0 -- frontend0
 *             +- tfbc0 -+
 *
 *
 *             +-  ovl1 -+
 *  channel1: -|- afbc1 -|-- SNR1 -- scaler1 -- frontend1
 *             +- tfbc1 -+
 *
 *             +-  ovlN -+
 *  channelN: -|- afbcN -|-- SNRN -- scalerN -- frontendN
 *             +- tfbcN -+
 *
 * +------------------ frontend0 -----------------------+
 * | CSC0_2 -- SHARP0 -- CDC0 -- DCI0 -- FCM0 -- CSC0_1 |
 * +----------------------------------------------------+
 *
 * +------------------ frontend1 -----------------------+
 * | CSC1_2 -- SHARP1 -- CDC1 -- DCI1 -- FCM1 -- CSC1_1 |
 * +----------------------------------------------------+
 *
 * +------------------ frontendN -----------------------+
 * | CSCN_2 -- sharpN -- CDCN -- DCIN-- FCMN -- CSCN_1  |
 * +----------------------------------------------------+
 *
 * note:
 * SNR is also considered as part of Frontend.
 */

enum de_alpha_mode {
/*	DE_ALPHA_MODE_PIXEL_ALPHA   = 0x00000000,
	DE_ALPHA_MODE_GLOBAL_ALPHA  = 0x00000001,
	DE_ALPHA_MODE_PXL_GLB_ALPHA = 0x00000002,*/

	DE_ALPHA_MODE_PREMUL_SHIFT  = 0x8,
	DE_ALPHA_MODE_PREMUL_MASK   = 0x1 << DE_ALPHA_MODE_PREMUL_SHIFT,
	DE_ALPHA_MODE_PREMUL        = 0x01 << DE_ALPHA_MODE_PREMUL_SHIFT,
};

struct de_chn_info {
	u8 enable;
	u8 fbd_en;
	u8 tfbd_en;
	u8 atw_en;
	u8 scale_en;
	u8 min_zorder;
	u8 glb_alpha;
	unsigned int layer_en_cnt;
	u8 layer_en[MAX_LAYER_NUM_PER_CHN];
	enum de_alpha_mode alpha_mode;
	u8 lay_premul[MAX_LAYER_NUM_PER_CHN];
	u32 flag;
	struct de_rect_s lay_win[MAX_LAYER_NUM_PER_CHN];
	struct de_rect_s ovl_win; /* layers input  */
	struct de_rect_s ovl_out_win; /* layers output win, after ovl down fetch */
	struct de_rect_s c_win; /* for chroma */
	struct de_rect_s scn_win; /* channel output, after scaler */
	struct de_scale_para ovl_ypara;
	struct de_scale_para ovl_cpara; /* for chroma */
	enum de_pixel_format px_fmt;
	enum de_format_space px_fmt_space;
	enum de_yuv_sampling yuv_sampling;
	u32 planar_num;
	enum de_eotf eotf;
	enum de_color_space color_space;
	enum de_color_range color_range;
	void *cdc_hdl;
	u8 snr_en;
};

struct de_channel_private {
	struct de_chn_info info;
	struct de_ovl_handle *ovl;
	struct de_afbd_handle *afbd;
	struct de_tfbd_handle *tfbd;
	struct de_scaler_handle *scaler;
	struct de_frontend_handle *frontend;
	struct mutex ch_lock;
};

static const uint64_t format_modifiers_common[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_INVALID,
};

int drm_to_de_format(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_ARGB8888:
		return DE_FORMAT_ARGB_8888;
	case DRM_FORMAT_ABGR8888:
		return DE_FORMAT_ABGR_8888;
	case DRM_FORMAT_RGBA8888:
		return DE_FORMAT_RGBA_8888;
	case DRM_FORMAT_BGRA8888:
		return DE_FORMAT_BGRA_8888;
	case DRM_FORMAT_XRGB8888:
		return DE_FORMAT_XRGB_8888;
	case DRM_FORMAT_XBGR8888:
		return DE_FORMAT_XBGR_8888;
	case DRM_FORMAT_RGBX8888:
		return DE_FORMAT_RGBX_8888;
	case DRM_FORMAT_BGRX8888:
		return DE_FORMAT_BGRX_8888;
	case DRM_FORMAT_RGB888:
		return DE_FORMAT_RGB_888;
	case DRM_FORMAT_BGR888:
		return DE_FORMAT_BGR_888;
	case DRM_FORMAT_RGB565:
		return DE_FORMAT_RGB_565;
	case DRM_FORMAT_BGR565:
		return DE_FORMAT_BGR_565;
	case DRM_FORMAT_ARGB4444:
		return DE_FORMAT_ARGB_4444;
	case DRM_FORMAT_ABGR4444:
		return DE_FORMAT_ABGR_4444;
	case DRM_FORMAT_RGBA4444:
		return DE_FORMAT_RGBA_4444;
	case DRM_FORMAT_BGRA4444:
		return DE_FORMAT_BGRA_4444;
	case DRM_FORMAT_ARGB1555:
		return DE_FORMAT_ARGB_1555;
	case DRM_FORMAT_ABGR1555:
		return DE_FORMAT_ABGR_1555;
	case DRM_FORMAT_RGBA5551:
		return DE_FORMAT_RGBA_5551;
	case DRM_FORMAT_BGRA5551:
		return DE_FORMAT_BGRA_5551;
	case DRM_FORMAT_ARGB2101010:
		return DE_FORMAT_ARGB_2101010;
	case DRM_FORMAT_ABGR2101010:
		return DE_FORMAT_ABGR_2101010;
	case DRM_FORMAT_RGBA1010102:
		return DE_FORMAT_RGBA_1010102;
	case DRM_FORMAT_BGRA1010102:
		return DE_FORMAT_BGRA_1010102;

	case DRM_FORMAT_AYUV:
		return DE_FORMAT_YUV444_I_AYUV;
	case DRM_FORMAT_YUV444:
		return DE_FORMAT_YUV444_P;
	case DRM_FORMAT_YUV422:
		return DE_FORMAT_YUV422_P;
	case DRM_FORMAT_YVU422:
		return DE_FORMAT_YVU422_P;
	case DRM_FORMAT_YUV420:
		return DE_FORMAT_YUV420_P;
	case DRM_FORMAT_YVU420:
		return DE_FORMAT_YVU420_P;
	case DRM_FORMAT_YUV411:
		return DE_FORMAT_YUV411_P;
	case DRM_FORMAT_YVU411:
		return DE_FORMAT_YVU411_P;
	case DRM_FORMAT_NV61:
		return DE_FORMAT_YUV422_SP_VUVU;
	case DRM_FORMAT_NV16:
		return DE_FORMAT_YUV422_SP_UVUV;
	case DRM_FORMAT_NV12:
		return DE_FORMAT_YUV420_SP_UVUV;
	case DRM_FORMAT_NV21:
		return DE_FORMAT_YUV420_SP_VUVU;
	case DRM_FORMAT_P010:
		return DE_FORMAT_YUV420_SP_UVUV_10BIT;
	case DRM_FORMAT_P210:
		return DE_FORMAT_YUV422_SP_UVUV_10BIT;
	}

	DRM_ERROR("get an unsupport drm format %d\n", format);
	return DE_FORMAT_ARGB_8888;
}

static void cal_channel_enable_layer_cnt(struct de_channel_handle *hdl, struct display_channel_state *state)
{
	int i;
	struct drm_framebuffer *fb;
	hdl->private->info.layer_en_cnt = 0;
	memset(hdl->private->info.layer_en, 0, sizeof(hdl->private->info.layer_en));
	for (i = 0; i < hdl->layer_cnt; i++) {
		fb = i == 0 ? state->base.fb : state->fb[i - 1];
		if (fb) {
			hdl->private->info.layer_en_cnt++;
			hdl->private->info.layer_en[i] = true;
		}
	}
//	WARN_ON(hdl->private->info.layer_en_cnt > hdl->layer_cnt);
}

static s32 de_rtmx_chn_data_attr(
	struct de_chn_info *chn_info,
	struct display_channel_state *state)
{
	struct drm_framebuffer *fb = state->base.fb;
	enum de_color_range range = state->color_range;

	chn_info->px_fmt = drm_to_de_format(fb->format->format);
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
	case DE_FORMAT_YVU422_P:
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
	case DE_FORMAT_YVU420_P:
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
	case DE_FORMAT_YVU411_P:
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

	chn_info->eotf = state->eotf;
	chn_info->color_space = state->color_space;
	if (range == DE_COLOR_RANGE_DEFAULT) {
		if (chn_info->px_fmt_space == DE_FORMAT_SPACE_YUV)
			range = DE_COLOR_RANGE_16_235;
		else
			range = DE_COLOR_RANGE_0_255;
	}
	chn_info->color_range = range;

	return 0;
}

/*
 * if any layer in a channel is premul, the remain not-premul layer should set chn_info->lay_premul[i]
 * to DE_OVL_PREMUL_NON_TO_EXC to tranform to a premul layer input, and premul layer should set to
 * DE_OVL_PREMUL_HAS_PREMUL, and  chn_info->alpha_mode should be set to DE_ALPHA_MODE_PREMUL.
 * if none layer is premul, chn_info->lay_premul[i] should be set to DE_OVL_PREMUL_NON_TO_NON, and
 * chn_info->alpha_mode should be set to ~DE_ALPHA_MODE_PREMUL.
 * blender pipe 0 must be ~DE_ALPHA_MODE_PREMUL.
 * afbc/tfbc/atw input should not be treat as premul layer.
 * refer to disp2 de_rtmx_chn_blend_attr() for detail.
 */

static void de_rtmx_chn_blend_attr(struct de_chn_info *chn_info, struct display_channel_state *state, unsigned int layer_cnt)
{
	u32 i;
	bool is_premul[MAX_LAYER_NUM_PER_CHN];

	/* FIXME maybe bug, this is used for scaler as chn's alpha, however every layer has its own alpha */
	chn_info->glb_alpha = (state->base.alpha >> 8);
	memset((void *)chn_info->lay_premul, DE_OVL_PREMUL_NON_TO_NON, sizeof(chn_info->lay_premul));
	chn_info->alpha_mode = ~DE_ALPHA_MODE_PREMUL;

	/* if any layer is premul, chn blending with premul */
	for (i = 0; i < layer_cnt; ++i) {
		if (!chn_info->layer_en[i])
			continue;
		is_premul[i] = i == 0 ? state->base.pixel_blend_mode == DRM_MODE_BLEND_PREMULTI :
				    state->pixel_blend_mode[i - 1] == DRM_MODE_BLEND_PREMULTI;
		if (is_premul[i]) {
			/* blender pipe0 has not alpha and premul is unavailable, state->base.normalized_zpos = 0 means this chn will mux as pipe0 */
			if (!state->base.normalized_zpos) {
				continue;
			}
			chn_info->alpha_mode = DE_ALPHA_MODE_PREMUL;
			chn_info->lay_premul[i] = DE_OVL_PREMUL_HAS_PREMUL;
		}
	}

	/* mark not premul layer need to convert to premul if chn blending with premul  */
	if (chn_info->alpha_mode == DE_ALPHA_MODE_PREMUL) {
		for (i = 0; i < layer_cnt; ++i) {
			if (!chn_info->layer_en[i])
				continue;
			if (chn_info->lay_premul[i] != DE_OVL_PREMUL_HAS_PREMUL)
				chn_info->lay_premul[i] = DE_OVL_PREMUL_NON_TO_EXC;
		}
	}

//	if (state->base.pixel_blend_mode == DRM_MODE_BLEND_PREMULTI && state->base.normalized_zpos) {
//		chn_info->alpha_mode |= DE_ALPHA_MODE_PREMUL;
//		for (i = 0; i < chn_info->layer_en_cnt; ++i) {
//			chn_info->lay_premul[i] = DE_OVL_PREMUL_HAS_PREMUL;/* FIXME */
//		}
//	} else {
//		chn_info->alpha_mode &= ~DE_ALPHA_MODE_PREMUL;
//		for (i = 0; i < chn_info->layer_en_cnt; ++i) {
//			chn_info->lay_premul[i] = DE_OVL_PREMUL_NON_TO_NON;/* FIXME */
//		}
//	}
}

static s32 de_rtmx_chn_calc_size(struct de_channel_handle *hdl, struct de_chn_info *chn_info, struct display_channel_state *state, unsigned int layer_cnt)
{
	struct de_scaler_handle *scaler = hdl->private->scaler;
	struct de_rect_s crop32[MAX_LAYER_NUM_PER_CHN];
	struct de_scale_para ypara[MAX_LAYER_NUM_PER_CHN];
	struct de_scale_para cpara[MAX_LAYER_NUM_PER_CHN];
	struct de_rect_s lay_scn_win[MAX_LAYER_NUM_PER_CHN];
	struct de_rect64_s crop64;
	struct de_rect win;
	struct de_scaler_cal_lay_cfg cal_lay_cfg;
	struct de_frontend_feedback frontend_data;
	s32 right, bottom;
	u32 i;

	win.left = win.top = 0x7FFFFFFF;
	win.right = win.bottom = 0x0;
	for (i = 0; i < layer_cnt; ++i) {
		if (!chn_info->layer_en[i])
			continue;

		if (i == 0) {
			lay_scn_win[i].left = state->base.crtc_x;
			lay_scn_win[i].top = state->base.crtc_y;
			lay_scn_win[i].width = state->base.crtc_w;
			lay_scn_win[i].height = state->base.crtc_h;
		} else {
			lay_scn_win[i].left = state->crtc_x[i - 1];
			lay_scn_win[i].top = state->crtc_y[i - 1];
			lay_scn_win[i].width = state->crtc_w[i - 1];
			lay_scn_win[i].height = state->crtc_h[i - 1];
		}
		right = lay_scn_win[i].left + lay_scn_win[i].width;
		bottom = lay_scn_win[i].top + lay_scn_win[i].height;
		win.left = min(win.left, lay_scn_win[i].left);
		win.top = min(win.top, lay_scn_win[i].top);
		win.right = max(win.right, right);
		win.bottom = max(win.bottom, bottom);
	}
	chn_info->scn_win.left = win.left;
	chn_info->scn_win.top = win.top;
	chn_info->scn_win.width = win.right - win.left;
	chn_info->scn_win.height = win.bottom - win.top;
	de_frontend_get_after_check_config(hdl->private->frontend, state, &frontend_data);
	chn_info->snr_en = frontend_data.snr_en;

	win.left = win.top = 0x7FFFFFFF;
	win.right = win.bottom = 0x0;

	memset(ypara, 0, sizeof(ypara));
	memset(cpara, 0, sizeof(cpara));

	cal_lay_cfg.fm_space = chn_info->px_fmt_space;
	cal_lay_cfg.yuv_sampling =  chn_info->yuv_sampling;
	cal_lay_cfg.px_fmt =  chn_info->px_fmt;
	memcpy(&cal_lay_cfg.ovl_out_win, &chn_info->ovl_out_win, sizeof(cal_lay_cfg.ovl_out_win));
	memcpy(&cal_lay_cfg.ovl_ypara, &chn_info->ovl_ypara, sizeof(cal_lay_cfg.ovl_ypara));
	cal_lay_cfg.snr_en =  chn_info->snr_en;


	for (i = 0; i < layer_cnt; ++i) {
		struct de_rect_s *lay_win = &(chn_info->lay_win[i]);

		if (!chn_info->layer_en[i])
			continue;

		if (i == 0) {
			crop64.left = (((unsigned long long)state->base.src_x) >> 16) << 32;
			crop64.top = (((unsigned long long)state->base.src_y) >> 16) << 32;
			if ((state->base.fb->format->format == DRM_FORMAT_YVU420 ||
			     state->base.fb->format->format == DRM_FORMAT_YUV420 ||
			     state->base.fb->format->format == DRM_FORMAT_NV12 ||
			     state->base.fb->format->format == DRM_FORMAT_NV21) &&
			    ((state->base.rotation & DRM_MODE_ROTATE_90) == DRM_MODE_ROTATE_90 ||
			     (state->base.rotation & DRM_MODE_ROTATE_270) == DRM_MODE_ROTATE_270)) {
				crop64.width = (((unsigned long long)state->base.src_h) >> 16) << 32;
				crop64.height = (((unsigned long long)state->base.src_w) >> 16) << 32;
			} else {
				crop64.width = (((unsigned long long)state->base.src_w) >> 16) << 32;
				crop64.height = (((unsigned long long)state->base.src_h) >> 16) << 32;
			}
		} else {
			crop64.left = (((unsigned long long)state->src_x[i - 1]) >> 16) << 32;
			crop64.top = (((unsigned long long)state->src_y[i - 1]) >> 16) << 32;
			crop64.width = (((unsigned long long)state->src_w[i - 1]) >> 16) << 32;
			crop64.height = (((unsigned long long)state->src_h[i - 1]) >> 16) << 32;
		}

		de_scaler_calc_lay_scale_para(scaler,
			&cal_lay_cfg,
			&crop64,
			&lay_scn_win[i],
			&crop32[i], &ypara[i], &cpara[i]);

		lay_win->left = de_scaler_calc_ovl_coord(scaler,
			lay_scn_win[i].left - chn_info->scn_win.left,
			ypara[i].hstep);
		lay_win->top = de_scaler_calc_ovl_coord(scaler,
			lay_scn_win[i].top - chn_info->scn_win.top,
			ypara[i].vstep);
		if (chn_info->yuv_sampling == DE_YUV420 &&
			 ((lay_win->top + crop32[i].height) % 2)) {
			lay_win->top -= 1;
		}
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

	de_scaler_calc_ovl_scale_para(layer_cnt, ypara, cpara,
		&chn_info->ovl_ypara, &chn_info->ovl_cpara);

	return 0;
}

__attribute__((unused))
static u32 de_rtmx_chn_fix_size_for_ability(const struct de_chn_info *chn_info, unsigned int de_ouput_height, unsigned int device_fps, unsigned long de_freq, u32 *height_out)
{
	u32 tmp;
	u64 update_speed_ability, required_speed_ability;

	/*
	* lcd_freq_MHz = lcd_width*lcd_height*lcd_fps/1000000;
	* how many overlay line can be fetched during scanning
	*	one lcd line(consider 80% dram efficiency)
	* update_speed_ability = lcd_width*de_freq_mhz*125/(ovl_w*lcd_freq_MHz);
	*/
	tmp = de_ouput_height * device_fps *
		((chn_info->ovl_win.width > chn_info->scn_win.width)
		? chn_info->ovl_win.width : chn_info->scn_win.width);
	if (tmp != 0) {
		update_speed_ability = de_freq * 80;
		do_div(update_speed_ability, tmp);
	} else {
		return 0;
	}

	/* how many overlay line need to fetch during scanning one lcd line */
	required_speed_ability = (chn_info->scn_win.height == 0) ? 0 :
		(chn_info->ovl_out_win.height * 100 / chn_info->scn_win.height);

	if (update_speed_ability < required_speed_ability) {
		/* if ability < required, use coarse scale */
		u64 tmp2 = update_speed_ability * chn_info->scn_win.height;

		do_div(tmp2, 100);
		*height_out = (u32)tmp2;
		return RTMX_CUT_INHEIGHT;
	}

	return 0;
}

/* de_performance:
 * Pde = 1 pixel/clk
 * lay_w * lay_h / Pde * Tde(1 / de_freq) <= htotal * disp_h * (1 / dclk) -->
 * lay_w * lay_h / Pde * dclk <= htotal * disp_h * de_freq
 * NOTE: maybe not suitable for compositing multiple layers in one ovl
 */
static u32 de_rtmx_chn_fix_size_for_hw_performance(const struct de_chn_info *chn_info, unsigned int htotal, unsigned long dclk_khz, unsigned long de_freq_khz, u32 *fix_height)
{
	u64 frame_piexl_process_required_time, refresh_zone_time;
	u32 efficiency = 90; // 90% efficiency

	frame_piexl_process_required_time = chn_info->ovl_win.width * chn_info->ovl_win.height * dclk_khz * 100 / efficiency;
	refresh_zone_time = htotal * chn_info->scn_win.height * de_freq_khz;

	if (frame_piexl_process_required_time > refresh_zone_time) {
		u64 tmp;

		tmp = chn_info->ovl_win.height * refresh_zone_time / frame_piexl_process_required_time;
		*fix_height = (u32)tmp;
		DRM_DEBUG_DRIVER("de_performance: in(%dx%d), dclk %ld, htotal %d, disp_height %d, de_clk %ld, height_out %d\n",
				chn_info->ovl_win.width, chn_info->ovl_win.height, dclk_khz, htotal, chn_info->scn_win.height, de_freq_khz, *fix_height);
		return RTMX_CUT_INHEIGHT;
	}

	return 0;
}

//chn_info->scale_en
static s32 de_rtmx_chn_fix_size(struct de_scaler_handle *hdl, struct de_chn_info *chn_info,
				    unsigned int de_ouput_width, unsigned int de_ouput_height,
				    unsigned int device_fps, unsigned long de_freq,
					unsigned int htotal, unsigned long dclk_khz)
{
	u32 fix_size_result = 0;

	if (((chn_info->px_fmt_space == DE_FORMAT_SPACE_RGB)
		|| ((chn_info->px_fmt_space == DE_FORMAT_SPACE_YUV)
		&& (chn_info->yuv_sampling == DE_YUV444)))
		&& (chn_info->ovl_out_win.height == chn_info->scn_win.height)
		&& (chn_info->ovl_out_win.width == chn_info->scn_win.width)) {
		chn_info->scale_en = 0;
		return 0;
	} else {
		chn_info->scale_en = 1;
	}

	if (chn_info->scale_en)
		fix_size_result |= de_scaler_fix_tiny_size(hdl,
			&(chn_info->ovl_out_win), &(chn_info->scn_win),
			&(chn_info->ovl_ypara), chn_info->px_fmt_space,
			chn_info->lay_win, chn_info->layer_en_cnt,
			de_ouput_width, de_ouput_height);

	if (!chn_info->fbd_en && !chn_info->tfbd_en) {
		fix_size_result |= de_scaler_fix_big_size(hdl,
			&(chn_info->ovl_out_win), &(chn_info->scn_win),
			chn_info->px_fmt_space, chn_info->yuv_sampling);
		// fix_size_result |= de_rtmx_chn_fix_size_for_ability(chn_info, de_ouput_height,
								// device_fps, de_freq, &chn_info->ovl_out_win.height);
		fix_size_result |= de_rtmx_chn_fix_size_for_hw_performance(chn_info, htotal, dclk_khz,
				de_freq / 1000, &chn_info->ovl_out_win.height);
	}

	de_scaler_calc_scale_para(hdl, fix_size_result,
		chn_info->px_fmt_space, chn_info->yuv_sampling,
		&(chn_info->scn_win),
		&(chn_info->ovl_out_win), &(chn_info->c_win),
		&(chn_info->ovl_ypara), &(chn_info->ovl_cpara));

	return 0;
}


void channel_update_regs(struct de_channel_handle *hdl)
{
	mutex_lock(&hdl->private->ch_lock);
	de_frontend_update_regs(hdl->private->frontend);
	mutex_unlock(&hdl->private->ch_lock);
}

void channel_process_late(struct de_channel_handle *hdl)
{
	de_frontend_process_late(hdl->private->frontend);
}

bool channel_format_mod_supported(struct de_channel_handle *hdl, uint32_t format, uint64_t modifier)
{
	bool is_support = false;

	if (modifier == DRM_FORMAT_MOD_LINEAR)
		return true;
	if (hdl->private->afbd)
		is_support |= de_afbc_format_mod_supported(hdl->private->afbd, format, modifier);
	if (hdl->private->tfbd)
		is_support |= de_tfbd_format_mod_supported(hdl->private->tfbd, format, modifier);
	return is_support;
}

int channel_get_pqd_config(struct de_channel_handle *hdl, struct display_channel_state *cstate)
{
	if (hdl->private->frontend)
		return de_frontend_get_pqd_config(hdl->private->frontend, cstate);
	return -EINVAL;
}

int channel_apply(struct de_channel_handle *hdl, struct display_channel_state *state,
		    const struct de_output_info *de_info, struct de_channel_output_info *output,
		    bool rgb_out)
{
	bool afbd_en = false, tfbd_en = false;
	struct de_ovl_cfg ovl_cfg;
	struct de_afbd_cfg afbd_cfg;
	struct de_tfbd_cfg tfbd_cfg;
	struct de_scaler_apply_cfg scaler_cfg;
	struct de_frontend_apply_cfg frontend_cfg;

	memset(&ovl_cfg, 0, sizeof(ovl_cfg));
	memset(&afbd_cfg, 0, sizeof(afbd_cfg));
	memset(&tfbd_cfg, 0, sizeof(tfbd_cfg));
	memset(&scaler_cfg, 0, sizeof(scaler_cfg));
	memset(&frontend_cfg, 0, sizeof(frontend_cfg));
	memset(&hdl->private->info, 0, sizeof(hdl->private->info));
	hdl->private->info.enable = false;

	/* disable channel */
	if (state->base.fb == NULL) {
		if (hdl->private->afbd)
			de_afbd_apply_lay(hdl->private->afbd, state, &afbd_cfg, &afbd_en);
		if (hdl->private->tfbd)
			de_tfbd_apply_lay(hdl->private->tfbd, state, &tfbd_cfg, &tfbd_en);
		de_ovl_apply_lay(hdl->private->ovl, state, &ovl_cfg);
		de_scaler_apply(hdl->private->scaler, &scaler_cfg);
		de_frontend_disable(hdl->private->frontend);
		DRM_DEBUG_DRIVER("[SUNXI-DE] %s %s disbale\n", __FUNCTION__, hdl->name);
		return 0;
	}
	hdl->private->info.enable = true;

	if (hdl->private->afbd)
		hdl->private->info.fbd_en = de_afbd_should_enable(hdl->private->afbd, state);
	if (hdl->private->tfbd)
		hdl->private->info.tfbd_en = de_tfbd_should_enable(hdl->private->tfbd, state);

	/* cal chn info  */
	cal_channel_enable_layer_cnt(hdl, state);
	de_rtmx_chn_data_attr(&hdl->private->info, state);
	de_rtmx_chn_blend_attr(&hdl->private->info, state, hdl->layer_cnt);
	de_rtmx_chn_calc_size(hdl, &hdl->private->info, state, hdl->layer_cnt);
	de_rtmx_chn_fix_size(hdl->private->scaler, &hdl->private->info, de_info->width, de_info->height,
			de_info->max_device_fps, de_info->de_clk_freq, de_info->htotal, de_info->pclk_khz);

	/* config afbd  */
	if (hdl->private->afbd) {
		afbd_cfg.lay_premul = hdl->private->info.lay_premul[0];
		memcpy(&afbd_cfg.ovl_win, &hdl->private->info.ovl_win, sizeof(afbd_cfg.ovl_win));
		de_afbd_apply_lay(hdl->private->afbd, state, &afbd_cfg, &afbd_en);
	}

	/* config tfbd  */
	if (hdl->private->tfbd) {
		memcpy(&tfbd_cfg.ovl_win, &hdl->private->info.ovl_win, sizeof(tfbd_cfg.ovl_win));
		/* if state is afbc format, it will return that modifier is not supported and disable it. */
		de_tfbd_apply_lay(hdl->private->tfbd, state, &tfbd_cfg, &tfbd_en);
	}

	/* config ovl */
	/* disable ovl when afbd enable */
	ovl_cfg.layer_en_cnt = (afbd_en | tfbd_en) ? 0 : hdl->private->info.layer_en_cnt;

	if (!afbd_en && !tfbd_en)
		memcpy(ovl_cfg.layer_en, &hdl->private->info.layer_en, sizeof(ovl_cfg.layer_en));
	memcpy(ovl_cfg.lay_premul, &hdl->private->info.lay_premul, sizeof(ovl_cfg.lay_premul));
	memcpy(ovl_cfg.lay_win, &hdl->private->info.lay_win, sizeof(ovl_cfg.lay_win));
	memcpy(&ovl_cfg.ovl_win, &hdl->private->info.ovl_win, sizeof(ovl_cfg.ovl_win));
	memcpy(&ovl_cfg.ovl_out_win, &hdl->private->info.ovl_out_win, sizeof(ovl_cfg.ovl_out_win));
	de_ovl_apply_lay(hdl->private->ovl, state, &ovl_cfg);

	scaler_cfg.scale_en = hdl->private->info.scale_en;
	scaler_cfg.glb_alpha = hdl->private->info.glb_alpha;
	scaler_cfg.px_fmt_space = hdl->private->info.px_fmt_space;
	scaler_cfg.yuv_sampling = hdl->private->info.yuv_sampling;
	scaler_cfg.px_fmt = hdl->private->info.px_fmt;
	memcpy(&scaler_cfg.scn_win, &hdl->private->info.scn_win, sizeof(scaler_cfg.scn_win));
	memcpy(&scaler_cfg.ovl_out_win, &hdl->private->info.ovl_out_win, sizeof(scaler_cfg.ovl_out_win));
	memcpy(&scaler_cfg.ovl_ypara, &hdl->private->info.ovl_ypara, sizeof(scaler_cfg.ovl_ypara));
	memcpy(&scaler_cfg.c_win, &hdl->private->info.c_win, sizeof(scaler_cfg.c_win));
	memcpy(&scaler_cfg.ovl_cpara, &hdl->private->info.ovl_cpara, sizeof(scaler_cfg.ovl_cpara));
	de_scaler_apply(hdl->private->scaler, &scaler_cfg);


	/* fill frontend apply cfg from chn_info */
	frontend_cfg.layer_en_cnt = hdl->private->info.layer_en_cnt;
	frontend_cfg.color_range = hdl->private->info.color_range;
	frontend_cfg.px_fmt_space = hdl->private->info.px_fmt_space;
	frontend_cfg.color_space = hdl->private->info.color_space;
	frontend_cfg.eotf = hdl->private->info.eotf;
	frontend_cfg.rgb_out = rgb_out;
	frontend_cfg.de_out_cfg.eotf = de_info->eotf;
	frontend_cfg.de_out_cfg.color_space = de_info->color_space;
	frontend_cfg.de_out_cfg.color_range = de_info->color_range;
	frontend_cfg.de_out_cfg.px_fmt_space = de_info->px_fmt_space;
	frontend_cfg.de_out_cfg.width = de_info->width;
	frontend_cfg.de_out_cfg.height = de_info->height;
	memcpy(&frontend_cfg.scn_win, &hdl->private->info.scn_win, sizeof(frontend_cfg.scn_win));
	memcpy(&frontend_cfg.ovl_out_win, &hdl->private->info.ovl_out_win, sizeof(frontend_cfg.ovl_out_win));
	de_frontend_apply(hdl->private->frontend, state, &frontend_cfg);

	/* fill output info */
	output->is_premul = (hdl->private->info.alpha_mode & DE_ALPHA_MODE_PREMUL) ? 1 : 0;
	drm_rect_init(&output->disp_win, hdl->private->info.scn_win.left, hdl->private->info.scn_win.top,
			    hdl->private->info.scn_win.width, hdl->private->info.scn_win.height);
	return 0;
}

static void drm_framebuffer_print_info(struct drm_printer *p, unsigned int indent,
				struct drm_framebuffer **fb, unsigned int fb_cnt)
{
	int i;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
	struct drm_gem_cma_object *obj[MAX_LAYER_NUM_PER_CHN] = {0};
#else
	struct drm_gem_dma_object *obj[MAX_LAYER_NUM_PER_CHN] = {0};
#endif
//FIXME
//	if (fb_cnt <= 0)
		return;

	drm_printf_indent(p, indent, "fb_id    : %20d |%20d |%20d |%20d\n",
		    fb[0] ? fb[0]->base.id : -1, fb[1] ? fb[1]->base.id : -1,
		    fb[2] ? fb[2]->base.id : -1, fb[3] ? fb[3]->base.id : -1);

	drm_printf_indent(p, indent, "ref_cnt  : %20d |%20d |%20d |%20d\n",
			    fb[0] ? drm_framebuffer_read_refcount(fb[0]) : 0,
			    fb[1] ? drm_framebuffer_read_refcount(fb[1]) : 0,
			    fb[2] ? drm_framebuffer_read_refcount(fb[2]) : 0,
			    fb[3] ? drm_framebuffer_read_refcount(fb[3]) : 0);



	drm_printf_indent(p, indent, "format   :                 %c%c%c%c |                %c%c%c%c |"
						"                %c%c%c%c |                %c%c%c%c \n",
			    fb[0] ? fb[0]->format->format & 0xff : ' ',
			    fb[0] ? fb[0]->format->format >> 8 & 0xff : 'N',
			    fb[0] ? fb[0]->format->format >> 16 & 0xff : '/',
			    fb[0] ? fb[0]->format->format >> 24 & 0xff : 'A',
			    fb[1] ? fb[1]->format->format & 0xff : ' ',
			    fb[1] ? fb[1]->format->format >> 8 & 0xff : 'N',
			    fb[1] ? fb[1]->format->format >> 16 & 0xff : '/',
			    fb[1] ? fb[1]->format->format >> 24 & 0xff : 'A',
			    fb[2] ? fb[2]->format->format & 0xff : ' ',
			    fb[2] ? fb[2]->format->format >> 8 & 0xff : 'N',
			    fb[2] ? fb[2]->format->format >> 16 & 0xff : '/',
			    fb[2] ? fb[2]->format->format >> 24 & 0xff : 'A',
			    fb[3] ? fb[3]->format->format & 0xff : ' ',
			    fb[3] ? fb[3]->format->format >> 8 & 0xff : 'N',
			    fb[3] ? fb[3]->format->format >> 16 & 0xff : '/',
			    fb[3] ? fb[3]->format->format >> 24 & 0xff : 'A');

	drm_printf_indent(p, indent, "modifier : 0x%18llx |0x%18llx |0x%18llx |0x%18llx\n",
		    fb[0] ? fb[0]->modifier : 0, fb[1] ? fb[1]->modifier : 0,
		    fb[2] ? fb[2]->modifier : 0, fb[3] ? fb[3]->modifier : 0);

	drm_printf_indent(p, indent, "size     : %4d        x   %4d |%4d        x   %4d |%4d        x   %4d |%4d        x   %4d\n",
		    fb[0] ? fb[0]->width : 0, fb[0] ? fb[0]->height : 0,
		    fb[1] ? fb[1]->width : 0, fb[1] ? fb[1]->height : 0,
		    fb[2] ? fb[1]->width : 0, fb[2] ? fb[2]->height : 0,
		    fb[3] ? fb[1]->width : 0, fb[3] ? fb[3]->height : 0);

	drm_printf_indent(p, indent, "p0 size  : %4d(%4d)  x   %4d |%4d(%4d)  x   %4d |%4d(%4d)  x   %4d |%4d(%4d)  x   %4d\n",
		    drm_framebuffer_plane_width(fb[0]->width,  fb[0], 0), fb[0]->pitches[0],
		    drm_framebuffer_plane_height(fb[0]->height, fb[0], 0),
		    fb[1] ? drm_framebuffer_plane_width(fb[1]->width,  fb[1], 0) : 0,
		    fb[1] ? fb[1]->pitches[0] : 0,
		    fb[1] ? drm_framebuffer_plane_height(fb[1]->height, fb[1], 0) : 0,
		    fb[2] ? drm_framebuffer_plane_width(fb[2]->width,  fb[2], 0) : 0,
		    fb[2] ? fb[2]->pitches[0] : 0,
		    fb[2] ? drm_framebuffer_plane_height(fb[2]->height, fb[2], 0) : 0,
		    fb[3] ? drm_framebuffer_plane_width(fb[3]->width,  fb[3], 0) : 0,
		    fb[3] ? fb[3]->pitches[0] : 0,
		    fb[3] ? drm_framebuffer_plane_height(fb[3]->height, fb[3], 0) : 0);

	for (i = 0; i < fb_cnt; i++) {
		if (!fb[i])
			continue;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
		obj[i] = to_drm_gem_cma_obj(fb[i]->obj[0]);
#else
		obj[i] = to_drm_gem_dma_obj(fb[i]->obj[0]);
#endif
	}
	drm_printf_indent(p, indent, "p0 paddr : 0x%18llx |0x%18llx |0x%18llx |0x%18llx\n",
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
		    obj[0] ? (unsigned long long)(obj[0]->paddr) : 0, obj[1] ? (unsigned long long)(obj[1]->paddr) : 0,
		    obj[2] ? (unsigned long long)(obj[2]->paddr) : 0, obj[3] ? (unsigned long long)(obj[3]->paddr) : 0);
#else
		    obj[0] ? (unsigned long long)(obj[0]->dma_addr) : 0, obj[1] ? (unsigned long long)(obj[1]->dma_addr) : 0,
		    obj[2] ? (unsigned long long)(obj[2]->dma_addr) : 0, obj[3] ? (unsigned long long)(obj[3]->dma_addr) : 0);
#endif

	drm_printf_indent(p, indent, "p0 offset: 0x%18x |0x%18x |0x%18x |0x%18x\n",
		    fb[0] ? fb[0]->offsets[0] : 0, fb[1] ? fb[1]->offsets[0] : 0,
		    fb[2] ? fb[2]->offsets[0] : 0, fb[3] ? fb[3]->offsets[0] : 0);

	if (fb[0]->format->num_planes > 1 || (fb[1] && fb[1]->format->num_planes > 1) ||
		  (fb[2] && fb[2]->format->num_planes > 1) || (fb[3] && fb[3]->format->num_planes > 1)) {
		drm_printf_indent(p, indent, "p1 size  : %4d(%4d)  x   %4d |%4d(%4d)  x   %4d |%4d(%4d)  x   %4d |%4d(%4d)  x   %4d\n",
			    fb[0]->format->num_planes > 1 ? drm_framebuffer_plane_width(fb[0]->width,  fb[0], 1) : 0, fb[0]->pitches[1],
			    fb[0]->format->num_planes > 1 ? drm_framebuffer_plane_height(fb[0]->height, fb[0], 1) : 0,
			    (fb[1] && fb[1]->format->num_planes > 1) ? drm_framebuffer_plane_width(fb[1]->width,  fb[1], 1) : 0,
			    (fb[1] && fb[1]->format->num_planes > 1) ? fb[1]->pitches[1] : 0,
			    (fb[1] && fb[1]->format->num_planes > 1) ? drm_framebuffer_plane_height(fb[1]->height, fb[1], 1) : 0,
			    (fb[2] && fb[2]->format->num_planes > 1) ? drm_framebuffer_plane_width(fb[2]->width,  fb[2], 1) : 0,
			    (fb[2] && fb[2]->format->num_planes > 1) ? fb[2]->pitches[1] : 0,
			    (fb[2] && fb[2]->format->num_planes > 1) ? drm_framebuffer_plane_height(fb[2]->height, fb[2], 1) : 0,
			    (fb[3] && fb[3]->format->num_planes > 1) ? drm_framebuffer_plane_width(fb[3]->width,  fb[3], 1) : 0,
			    (fb[3] && fb[3]->format->num_planes > 1) ? fb[3]->pitches[1] : 0,
			    (fb[3] && fb[3]->format->num_planes > 1) ? drm_framebuffer_plane_height(fb[3]->height, fb[3], 1) : 0);
		for (i = 0; i < fb_cnt; i++) {
			if (!fb[i])
				continue;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
			obj[i] = to_drm_gem_cma_obj(fb[i]->obj[1]);
#else
			obj[i] = to_drm_gem_dma_obj(fb[i]->obj[1]);
#endif
		}
		drm_printf_indent(p, indent, "p1 paddr : 0x%18llx |0x%18llx |0x%18llx |0x%18llx\n",
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
			    obj[0] ? (unsigned long long)(obj[0]->paddr) : 0, obj[1] ? (unsigned long long)(obj[1]->paddr) : 0,
			    obj[2] ? (unsigned long long)(obj[2]->paddr) : 0, obj[3] ? (unsigned long long)(obj[3]->paddr) : 0);
#else
			    obj[0] ? (unsigned long long)(obj[0]->dma_addr) : 0, obj[1] ? (unsigned long long)(obj[1]->dma_addr) : 0,
			    obj[2] ? (unsigned long long)(obj[2]->dma_addr) : 0, obj[3] ? (unsigned long long)(obj[3]->dma_addr) : 0);
#endif

		drm_printf_indent(p, indent, "p1 offset: 0x%18x |0x%18x |0x%18x |0x%18x\n",
			    fb[0] ? fb[0]->offsets[1] : 0, fb[1] ? fb[1]->offsets[1] : 0,
			    fb[2] ? fb[2]->offsets[1] : 0, fb[3] ? fb[3]->offsets[1] : 0);

	}

	if (fb[0]->format->num_planes > 2 || (fb[1] && fb[1]->format->num_planes > 2) ||
		  (fb[2] && fb[2]->format->num_planes > 2) || (fb[3] && fb[3]->format->num_planes > 2)) {
		drm_printf_indent(p, indent, "p2 size  : %4d(%4d)  x   %4d |%4d(%4d)  x   %4d |%4d(%4d)  x   %4d |%4d(%4d)  x   %4d\n",
			    (fb[0] && fb[0]->format->num_planes > 2) ? drm_framebuffer_plane_width(fb[0]->width,   fb[0], 2) : 0, fb[0]->pitches[2],
			    (fb[0] && fb[0]->format->num_planes > 2) ? drm_framebuffer_plane_height(fb[0]->height, fb[0], 2) : 0,
			    (fb[1] && fb[1]->format->num_planes > 2) ? drm_framebuffer_plane_width(fb[1]->width,   fb[1], 2) : 0,
			    (fb[1] && fb[1]->format->num_planes > 2) ? fb[1]->pitches[2] : 0,
			    (fb[1] && fb[1]->format->num_planes > 2) ? drm_framebuffer_plane_height(fb[1]->height, fb[1], 2) : 0,
			    (fb[2] && fb[2]->format->num_planes > 2) ? drm_framebuffer_plane_width(fb[2]->width,   fb[2], 2) : 0,
			    (fb[2] && fb[2]->format->num_planes > 2) ? fb[2]->pitches[2] : 0,
			    (fb[2] && fb[2]->format->num_planes > 2) ? drm_framebuffer_plane_height(fb[2]->height, fb[2], 2) : 0,
			    (fb[3] && fb[3]->format->num_planes > 2) ? drm_framebuffer_plane_width(fb[3]->width,   fb[3], 2) : 0,
			    (fb[3] && fb[3]->format->num_planes > 2) ? fb[3]->pitches[2] : 0,
			    (fb[3] && fb[3]->format->num_planes > 2) ? drm_framebuffer_plane_height(fb[3]->height, fb[3], 2) : 0);

		for (i = 0; i < fb_cnt; i++) {
			if (!fb[i])
				continue;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
			obj[i] = to_drm_gem_cma_obj(fb[i]->obj[2]);
#else
			obj[i] = to_drm_gem_dma_obj(fb[i]->obj[2]);
#endif
		}
		 drm_printf_indent(p, indent, "p2 paddr : 0x%18llx |0x%18llx |0x%18llx |0x%18llx\n",
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
			    obj[0] ? (unsigned long long)(obj[0]->paddr) : 0, obj[1] ? (unsigned long long)(obj[1]->paddr) : 0,
			    obj[2] ? (unsigned long long)(obj[2]->paddr) : 0, obj[3] ? (unsigned long long)(obj[3]->paddr) : 0);
#else
			    obj[0] ? (unsigned long long)(obj[0]->dma_addr) : 0, obj[1] ? (unsigned long long)(obj[1]->dma_addr) : 0,
			    obj[2] ? (unsigned long long)(obj[2]->dma_addr) : 0, obj[3] ? (unsigned long long)(obj[3]->dma_addr) : 0);
#endif

		drm_printf_indent(p, indent, "p2 offset: 0x%18x |0x%18x |0x%18x |0x%18x\n",
			    fb[0] ? fb[0]->offsets[2] : 0, fb[1] ? fb[1]->offsets[2] : 0,
			    fb[2] ? fb[2]->offsets[2] : 0, fb[3] ? fb[3]->offsets[2] : 0);
	}

}

void dump_channel_state(struct drm_printer *p, struct de_channel_handle *hdl, const struct display_channel_state *state, bool state_only)
{
	int i;
	unsigned int cnt = 0;
	struct drm_framebuffer *fb;
	struct drm_framebuffer *fbs[MAX_LAYER_NUM_PER_CHN] = {0};
	struct drm_plane_state *s = (struct drm_plane_state *)&state->base;
	char *blending_name[] = {
		[DRM_MODE_BLEND_PREMULTI] = "premult",
		[DRM_MODE_BLEND_COVERAGE] = "coverage",
		[DRM_MODE_BLEND_PIXEL_NONE] = "none",
	};

	for (i = 0; i < hdl->layer_cnt; i++) {
		fb = i == 0 ? state->base.fb : state->fb[i - 1];
		if (fb)
			cnt++;
	}

	if (cnt) {
		drm_printf(p, "\t\t layer_id | fb_id |       src-pos       |       crtc-pos      | blend_mode |  alpha |   color \n");
		drm_printf(p, "\t\t ---------+-------+---------------------+---------------------+------------+--------+---------\n");
	}
	for (i = 0; i < cnt; i++) {
		if (i == 0) {
			drm_printf(p, "\t\t   %3d    |"" %4d  |"" %4dx%4d+%4d+%4d | %4dx%4d+%4d+%4d |  %8s  |   %3d  | 0x%08x\n",
					i,  s->fb ? s->fb->base.id : 0, s->src_w >> 16, s->src_h >> 16,
					s->src_x >> 16, s->src_y >> 16, s->crtc_w, s->crtc_h, s->crtc_x, s->crtc_y,
					blending_name[s->pixel_blend_mode], s->alpha >> 8, state->color[i]);
			if (s->fb)
				fbs[0] = s->fb;
		} else {
			drm_printf(p, "\t\t   %3d    |"" %4d  |"" %4dx%4d+%4d+%4d | %4dx%4d+%4d+%4d |  %8s  |   %3d  | 0x%08x\n",
					i, state->fb[i - 1] ? state->fb[i - 1]->base.id : 0, state->src_w[i - 1] >> 16, state->src_h[i - 1] >> 16,
					state->src_x[i - 1] >> 16, state->src_y[i - 1] >> 16, state->crtc_w[i - 1], state->crtc_h[i - 1],
					state->crtc_x[i - 1], state->crtc_y[i - 1], blending_name[state->pixel_blend_mode[i - 1]],
					state->alpha[i - 1] >> 8, state->color[i]);
			if (state->fb[i - 1])
				fbs[i] = state->fb[i - 1];
		}
	}

	if (cnt)
		drm_framebuffer_print_info(p, 3, fbs, cnt);

	if (!state_only) {
		de_dump_ovl_state(p, hdl->private->ovl, state);
		if (hdl->private->afbd)
			de_dump_afbd_state(p, hdl->private->afbd, state);
		if (hdl->private->tfbd)
			de_dump_tfbd_state(p, hdl->private->tfbd, state);
		if (hdl->private->scaler)
			dump_scaler_state(p, hdl->private->scaler, state);
		if (hdl->private->frontend)
			de_frontend_dump_state(p, hdl->private->frontend);
	}
}

int get_size_by_chn(struct de_channel_handle *hdl, int *width, int *height)
{
	bool in_enable;
	in_enable = hdl->private->info.enable;
	if (in_enable) {
		*width = hdl->private->info.ovl_win.width;
		*height = hdl->private->info.ovl_win.height;
	} else {
		*width = 0;
		*height = 0;
	}
	return 0;
}

unsigned int get_chn_size_on_scn(struct de_channel_handle *hdl, int *width, int *height)
{
	*width = hdl->private->info.scn_win.width;
	*height = hdl->private->info.scn_win.height;
	return 0;
}

struct de_channel_handle *de_channel_create(struct module_create_info *cinfo)
{
	struct de_channel_handle *hdl;
	unsigned int format_modifiers_num = 0;
	uint64_t *modifier_ptr;
	unsigned int block_num = 0;
	int cur_block = 0;
	int i;
	struct module_create_info info;

	memcpy(&info, cinfo, sizeof(*cinfo));
	hdl = kmalloc(sizeof(*hdl), GFP_KERNEL | __GFP_ZERO);
	hdl->private = kmalloc(sizeof(*hdl->private), GFP_KERNEL | __GFP_ZERO);
	memcpy(&hdl->cinfo, cinfo, sizeof(*cinfo));

	/* create ovl */
	hdl->private->ovl = de_ovl_create(&info);
	if (!hdl->private->ovl) {
		kfree(hdl->private);
		kfree(hdl);
		return NULL;
	}

	/* add channel reg base */
	info.reg_offset = hdl->private->ovl->channel_reg_base;

	/* create scaler */
	hdl->private->scaler = de_scaler_create(&info);

	/* create afbd */
	hdl->private->afbd = de_afbd_create(&info);

	/* create tfbd */
	hdl->private->tfbd = de_tfbd_create(&info);

	hdl->name = hdl->private->ovl->name;

	/* create front process module */
	if (hdl->private->scaler && hdl->private->scaler->is_asu)
		info.extra = hdl->private->scaler;
	hdl->private->frontend = de_frontend_create(&info);
	if (hdl->private->frontend)
		hdl->routine_job = hdl->private->frontend->routine_job;
	info.extra = NULL;

	hdl->formats = hdl->private->ovl->formats;
	if (hdl->private->scaler) {
		hdl->exclusive_scaler_ids = hdl->private->scaler->linebuff_share_ids;
		hdl->lbuf.scaler_lbuffer_yuv = hdl->private->scaler->linebuff_yuv;
		hdl->lbuf.scaler_lbuffer_rgb = hdl->private->scaler->linebuff_rgb;
		hdl->lbuf.scaler_lbuffer_yuv_ed = hdl->private->scaler->linebuff_yuv_ed;
	}
	hdl->format_count = hdl->private->ovl->format_count;
	hdl->layer_cnt = hdl->private->ovl->layer_cnt;

	if (hdl->private->afbd || hdl->private->tfbd)
		format_modifiers_num = (hdl->private->afbd ? hdl->private->afbd->format_modifiers_num : 0) +
				       (hdl->private->tfbd ? hdl->private->tfbd->format_modifiers_num : 0);

	/* for each channel ,should reserve for DRM_FORMAT_MOD_LINEAR and others */
	format_modifiers_num += ARRAY_SIZE(format_modifiers_common) - 1;

	hdl->format_modifiers_comb = kmalloc(sizeof(*(hdl->format_modifiers_comb)) *
					(format_modifiers_num + 1), GFP_KERNEL | __GFP_ZERO);
	modifier_ptr = hdl->format_modifiers_comb;
	if (hdl->private->afbd || hdl->private->tfbd) {
		if (hdl->private->afbd) {
			hdl->afbc_rot_support = hdl->private->afbd->rotate_support;
			hdl->lbuf.afbc_rotate_support = hdl->private->afbd->rotate_support;
			hdl->lbuf.limit_afbc_rotate_height = hdl->private->afbd->rotate_limit_height;
			memcpy(modifier_ptr, hdl->private->afbd->format_modifiers,
			       hdl->private->afbd->format_modifiers_num *
			       sizeof(*(hdl->format_modifiers_comb)));
			modifier_ptr += hdl->private->afbd->format_modifiers_num;
		}
		if (hdl->private->tfbd) {
			memcpy(modifier_ptr, hdl->private->tfbd->format_modifiers,
			       hdl->private->tfbd->format_modifiers_num *
			       sizeof(*(hdl->format_modifiers_comb)));
			modifier_ptr += hdl->private->tfbd->format_modifiers_num;
		}

		/* add default DRM_FORMAT_MOD_LINEAR */
		memcpy(modifier_ptr, format_modifiers_common,
		       ARRAY_SIZE(format_modifiers_common) - 1);
		modifier_ptr += ARRAY_SIZE(format_modifiers_common) - 1;
	} else {
		memcpy(modifier_ptr, format_modifiers_common,
		       format_modifiers_num);
		modifier_ptr += format_modifiers_num;
	}

	*modifier_ptr = DRM_FORMAT_MOD_INVALID;

	hdl->is_video = hdl->private->ovl->is_video;
	hdl->type_hw_id = hdl->private->ovl->type_hw_id;

	/* block info */

	if (hdl->private->afbd)
		block_num += hdl->private->afbd->block_num;
	if (hdl->private->tfbd)
		block_num += hdl->private->tfbd->block_num;
	if (hdl->private->ovl)
		block_num += hdl->private->ovl->block_num;

	if (hdl->private->scaler)
		block_num += hdl->private->scaler->block_num;

	if (hdl->private->frontend) {
		memcpy(&hdl->mod, &hdl->private->frontend->mod, sizeof(hdl->mod));
		block_num += hdl->private->frontend->block_num;
	}

	hdl->block = kmalloc(sizeof(*hdl->block) * block_num, GFP_KERNEL | __GFP_ZERO);

	if (hdl->private->ovl) {
		for (i = 0; i < hdl->private->ovl->block_num; i++, cur_block++) {
			hdl->block[cur_block] = hdl->private->ovl->block[i];
		}
	}

	if (hdl->private->scaler) {
		for (i = 0; i < hdl->private->scaler->block_num; i++, cur_block++) {
			hdl->block[cur_block] = hdl->private->scaler->block[i];
		}
	}

	if (hdl->private->afbd) {
		for (i = 0; i < hdl->private->afbd->block_num; i++, cur_block++) {
			hdl->block[cur_block] = hdl->private->afbd->block[i];
		}
	}

	if (hdl->private->tfbd) {
		for (i = 0; i < hdl->private->tfbd->block_num; i++, cur_block++) {
			hdl->block[cur_block] = hdl->private->tfbd->block[i];
		}
	}

	if (hdl->private->frontend) {
		for (i = 0; i < hdl->private->frontend->block_num; i++, cur_block++) {
			hdl->block[cur_block] = hdl->private->frontend->block[i];
		}
	}

	hdl->block_num = block_num;

	mutex_init(&hdl->private->ch_lock);

	return hdl;
}

#include <linux/module.h>
#include <linux/kernel.h>

#include "vpss_debug.h"
#include "vpss_common.h"
#include "scaler.h"
#include "vpss_core.h"
#include "vpss_hal.h"
#include "vpss_platform.h"


enum sclr_format user_fmt_to_hw(u32 fmt)
{
	switch (fmt) {
	case PIXEL_FORMAT_YUV_PLANAR_420:
		return SCL_FMT_YUV420;
	case PIXEL_FORMAT_YUV_PLANAR_422:
		return SCL_FMT_YUV422;
	case PIXEL_FORMAT_YUV_PLANAR_444:
		return SCL_FMT_RGB_PLANAR;
	case PIXEL_FORMAT_NV12:
		return SCL_FMT_NV12;
	case PIXEL_FORMAT_NV21:
		return SCL_FMT_NV21;
	case PIXEL_FORMAT_NV16:
		return SCL_FMT_YUV422SP1;
	case PIXEL_FORMAT_NV61:
		return SCL_FMT_YUV422SP2;
	case PIXEL_FORMAT_YUYV:
		return SCL_FMT_YUYV;
	case PIXEL_FORMAT_YVYU:
		return SCL_FMT_YVYU;
	case PIXEL_FORMAT_UYVY:
		return SCL_FMT_UYVY;
	case PIXEL_FORMAT_VYUY:
		return SCL_FMT_VYUY;
	case PIXEL_FORMAT_YUV_444:
		return SCL_FMT_RGB_PACKED;
	case PIXEL_FORMAT_RGB_888_PLANAR:
		return SCL_FMT_RGB_PLANAR;
	case PIXEL_FORMAT_BGR_888_PLANAR:
		return SCL_FMT_RGB_PLANAR;
	case PIXEL_FORMAT_RGB_888:
		return SCL_FMT_RGB_PACKED;
	case PIXEL_FORMAT_BGR_888:
		return SCL_FMT_BGR_PACKED;
	case PIXEL_FORMAT_YUV_400:
		return SCL_FMT_Y_ONLY;
	case PIXEL_FORMAT_HSV_888:
		return SCL_FMT_RGB_PACKED;
	case PIXEL_FORMAT_HSV_888_PLANAR:
		return SCL_FMT_RGB_PLANAR;
	case PIXEL_FORMAT_FP32_C1:
	case PIXEL_FORMAT_FP32_C3_PLANAR:
	case PIXEL_FORMAT_FP16_C1:
	case PIXEL_FORMAT_FP16_C3_PLANAR:
	case PIXEL_FORMAT_BF16_C1:
	case PIXEL_FORMAT_BF16_C3_PLANAR:
	case PIXEL_FORMAT_INT8_C1:
	case PIXEL_FORMAT_INT8_C3_PLANAR:
	case PIXEL_FORMAT_UINT8_C1:
	case PIXEL_FORMAT_UINT8_C3_PLANAR:
		return SCL_FMT_BF16;
	default:
		return SCL_FMT_YUV420;
	}

	return SCL_FMT_YUV420;
}

enum sclr_odma_datatype user_fmt_to_datatype(u32 fmt)
{
	switch (fmt) {
	case PIXEL_FORMAT_FP32_C1:
	case PIXEL_FORMAT_FP32_C3_PLANAR:
		return SCL_DATATYPE_FP32;
	case PIXEL_FORMAT_FP16_C1:
	case PIXEL_FORMAT_FP16_C3_PLANAR:
		return SCL_DATATYPE_FP16;
	case PIXEL_FORMAT_BF16_C1:
	case PIXEL_FORMAT_BF16_C3_PLANAR:
		return SCL_DATATYPE_BF16;
	case PIXEL_FORMAT_INT8_C1:
	case PIXEL_FORMAT_INT8_C3_PLANAR:
		return SCL_DATATYPE_INT8;
	case PIXEL_FORMAT_UINT8_C1:
	case PIXEL_FORMAT_UINT8_C3_PLANAR:
		return SCL_DATATYPE_U8;
	default:
		return SCL_DATATYPE_DISABLE;
	}

	return SCL_DATATYPE_DISABLE;
}

u8 _gop_get_bpp(enum sclr_gop_format fmt)
{
	return (fmt == SCL_GOP_FMT_ARGB8888) ? 4 :
		(fmt == SCL_GOP_FMT_256LUT) ? 1 : 2;
}

int _sc_ext_set_rgn_cfg(const u8 inst, u8 layer, const struct rgn_cfg *rgn_cfg,
	const struct sclr_size *size)
{
	struct sclr_gop_cfg *gop_cfg = sclr_gop_get_cfg(inst, layer);
	struct sclr_gop_odec_cfg *odec_cfg = &gop_cfg->odec_cfg;
	struct rgn_lut_cfg *rgn_lut_cfg = (struct rgn_lut_cfg *)&rgn_cfg->rgn_lut_cfg;
	struct sclr_gop_ow_cfg *ow_cfg;
	u8 bpp, ow_idx;

	gop_cfg->gop_ctrl.raw &= ~0xfff;
	gop_cfg->gop_ctrl.b.hscl_en = rgn_cfg->hscale_x2;
	gop_cfg->gop_ctrl.b.vscl_en = rgn_cfg->vscale_x2;
	gop_cfg->gop_ctrl.b.colorkey_en = rgn_cfg->colorkey_en;
	gop_cfg->gop_ctrl.b.burst = 0xf;
	gop_cfg->colorkey = rgn_cfg->colorkey;

	if (rgn_lut_cfg->is_updated) {
		sclr_gop_setup_256LUT(inst, layer, rgn_lut_cfg->lut_length, rgn_lut_cfg->lut_addr);
	}

	if (rgn_cfg->odec.enable) { // odec enable
		ow_idx = rgn_cfg->odec.attached_ow;
		ow_cfg = &gop_cfg->ow_cfg[ow_idx];
		gop_cfg->gop_ctrl.raw |= BIT(ow_idx);

		if (rgn_cfg->param[ow_idx].fmt > RGN_FMT_MAX) {
			TRACE_VPSS(DBG_ERR, "RGN FMT Error\n");
		} else {
			ow_cfg->fmt = (enum sclr_gop_format)rgn_cfg->param[ow_idx].fmt;
		}
		ow_cfg->addr = rgn_cfg->param[ow_idx].phy_addr;
		ow_cfg->pitch = rgn_cfg->param[ow_idx].stride;
		ow_cfg->start.x = rgn_cfg->param[ow_idx].rect.left;
		ow_cfg->start.y = rgn_cfg->param[ow_idx].rect.top;
		ow_cfg->img_size.w = rgn_cfg->param[ow_idx].rect.width;
		ow_cfg->img_size.h = rgn_cfg->param[ow_idx].rect.height;
		ow_cfg->end.x = ow_cfg->start.x + ow_cfg->img_size.w;
		ow_cfg->end.y = ow_cfg->start.y + ow_cfg->img_size.h;

		//while reg_odec_en=1, odec_stream_size = {reg_ow_dram_vsize,reg_ow_dram_hsize}
		ow_cfg->mem_size.w = ALIGN(rgn_cfg->odec.bso_sz, 16) & 0x7fff;
		ow_cfg->mem_size.h = ALIGN(rgn_cfg->odec.bso_sz, 16) >> 15;

		// set odec cfg
		odec_cfg->odec_ctrl.b.odec_en = true;
		odec_cfg->odec_ctrl.b.odec_int_en = true;
		odec_cfg->odec_ctrl.b.odec_int_clr = true;
		//odec_cfg->odec_ctrl.b.odec_wdt_en = true;
		odec_cfg->odec_ctrl.b.odec_attached_idx = ow_idx;
#if 0
		TRACE_VPSS(DBG_INFO, "inst(%d) gop(%d) fmt(%d) rect(%d %d %d %d) addr(0x%llx) pitch(%d).\n",
				inst, layer, ow_cfg->fmt,
				ow_cfg->start.x, ow_cfg->start.y, ow_cfg->img_size.w, ow_cfg->img_size.h,
				ow_cfg->addr, ow_cfg->pitch);
		TRACE_VPSS(DBG_INFO, "odec:enable(%d) attached_ow(%d) size(%d).\n",
				odec_cfg->odec_ctrl.b.odec_en, odec_cfg->odec_ctrl.b.odec_attached_idx,
				rgn_cfg->odec.bso_sz);
#endif

		sclr_gop_ow_set_cfg(inst, layer, ow_idx, ow_cfg, true);
	} else { //normal rgn w/o odec enabled
		for (ow_idx = 0; ow_idx < rgn_cfg->num_of_rgn; ++ow_idx) {
			ow_cfg = &gop_cfg->ow_cfg[ow_idx];
			gop_cfg->gop_ctrl.raw |= BIT(ow_idx);

			if (rgn_cfg->param[ow_idx].fmt > RGN_FMT_MAX) {
				TRACE_VPSS(DBG_ERR, "RGN FMT Error\n");
			} else {
				bpp = _gop_get_bpp((enum sclr_gop_format)rgn_cfg->param[ow_idx].fmt);
				ow_cfg->fmt = (enum sclr_gop_format)rgn_cfg->param[ow_idx].fmt;
			}
			ow_cfg->addr = rgn_cfg->param[ow_idx].phy_addr;
			ow_cfg->pitch = rgn_cfg->param[ow_idx].stride;
			if (rgn_cfg->param[ow_idx].rect.left < 0) {
				ow_cfg->start.x = 0;
				ow_cfg->addr -= bpp * rgn_cfg->param[ow_idx].rect.left;
				ow_cfg->img_size.w
					= rgn_cfg->param[ow_idx].rect.width + rgn_cfg->param[ow_idx].rect.left;
			} else if ((rgn_cfg->param[ow_idx].rect.left + rgn_cfg->param[ow_idx].rect.width) > size->w) {
				ow_cfg->start.x = rgn_cfg->param[ow_idx].rect.left;
				ow_cfg->img_size.w = size->w - rgn_cfg->param[ow_idx].rect.left;
				ow_cfg->mem_size.w = rgn_cfg->param[ow_idx].stride;
			} else {
				ow_cfg->start.x = rgn_cfg->param[ow_idx].rect.left;
				ow_cfg->img_size.w = rgn_cfg->param[ow_idx].rect.width;
				ow_cfg->mem_size.w = rgn_cfg->param[ow_idx].stride;
			}

			if (rgn_cfg->param[ow_idx].rect.top < 0) {
				ow_cfg->start.y = 0;
				ow_cfg->addr -= ow_cfg->pitch * rgn_cfg->param[ow_idx].rect.top;
				ow_cfg->img_size.h
					= rgn_cfg->param[ow_idx].rect.height + rgn_cfg->param[ow_idx].rect.top;
			} else if ((rgn_cfg->param[ow_idx].rect.top + rgn_cfg->param[ow_idx].rect.height) > size->h) {
				ow_cfg->start.y = rgn_cfg->param[ow_idx].rect.top;
				ow_cfg->img_size.h = size->h - rgn_cfg->param[ow_idx].rect.top;
			} else {
				ow_cfg->start.y = rgn_cfg->param[ow_idx].rect.top;
				ow_cfg->img_size.h = rgn_cfg->param[ow_idx].rect.height;
			}

			ow_cfg->end.x = ow_cfg->start.x +
					(ow_cfg->img_size.w << gop_cfg->gop_ctrl.b.hscl_en) -
					gop_cfg->gop_ctrl.b.hscl_en;
			ow_cfg->end.y = ow_cfg->start.y +
					(ow_cfg->img_size.h << gop_cfg->gop_ctrl.b.vscl_en) -
					gop_cfg->gop_ctrl.b.vscl_en;
			ow_cfg->mem_size.w = ALIGN(ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
			ow_cfg->mem_size.h = ow_cfg->img_size.h;

#if 0
			TRACE_VPSS(DBG_INFO, "inst(%d) gop(%d) fmt(%d)\n", inst, layer, ow_cfg->fmt);
			TRACE_VPSS(DBG_INFO, "rect(%d %d %d %d) addr(0x%llx) pitch(%d).\n",
				ow_cfg->start.x, ow_cfg->start.y, ow_cfg->img_size.w, ow_cfg->img_size.h,
				ow_cfg->addr, ow_cfg->pitch);
#endif

			sclr_gop_ow_set_cfg(inst, layer, ow_idx, ow_cfg, true);
		}

		// set odec enable to false
		odec_cfg->odec_ctrl.b.odec_en = false;
	}
	sclr_gop_set_cfg(inst, layer, gop_cfg, true);

	return 0;
}

static int _sc_ext_set_fbd(u8 dev_idx, const struct vpss_hal_grp_cfg *grp_cfg){
	struct sclr_fbd_cfg fbd_cfg;
	int height;
	memset(&fbd_cfg, 0, sizeof(struct sclr_fbd_cfg));

	fbd_cfg.enable = grp_cfg->fbd_enable;
	if(fbd_cfg.enable){
		height = ((grp_cfg->src_size.height + 15) >> 4) << 2;
		fbd_cfg.crop.x = grp_cfg->crop.left;
		fbd_cfg.crop.y = grp_cfg->crop.top;
		fbd_cfg.crop.w = grp_cfg->crop.width;
		fbd_cfg.crop.h = grp_cfg->crop.height;
		fbd_cfg.stride_y = ALIGN(ALIGN(grp_cfg->src_size.width, 16) << 2, 32);
		fbd_cfg.stride_c = ALIGN(ALIGN(grp_cfg->src_size.width >> 1, 16) << 2, 32);
		fbd_cfg.height_y = ALIGN(grp_cfg->src_size.height, 16);
		fbd_cfg.height_c = ALIGN(grp_cfg->src_size.height >> 1, 8);
		fbd_cfg.offset_base_y = grp_cfg->addr[0] + (((fbd_cfg.crop.x >> 8) * height) << 5) +
					((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.offset_base_c = grp_cfg->addr[1] + (((fbd_cfg.crop.x >> 9) * height) << 5) +
					((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.comp_base_y = grp_cfg->addr[2] + (fbd_cfg.crop.y >> 2) * fbd_cfg.stride_y;
		fbd_cfg.comp_base_c = grp_cfg->addr[3] + (fbd_cfg.crop.y >> 2) * ALIGN(ALIGN(((grp_cfg->src_size.width + 1) >> 1), 16) << 2, 32);
		fbd_cfg.out_mode_y = 0;
		fbd_cfg.out_mode_c = 7;
		fbd_cfg.endian = 0;
	}
	sclr_set_fbd(dev_idx, &fbd_cfg, true);

	return 0;
}

static int _sc_ext_set_quant(u8 dev_idx, const struct sc_quant_param *param)
{
	struct sclr_odma_cfg *odma_cfg = sclr_odma_get_cfg(dev_idx);

	if (!param->enable)
		return 0;

	memcpy(odma_cfg->csc_cfg.quant_form.sc_frac, param->sc_frac, sizeof(param->sc_frac));
	memcpy(odma_cfg->csc_cfg.quant_form.sub, param->sub, sizeof(param->sub));
	memcpy(odma_cfg->csc_cfg.quant_form.sub_frac, param->sub_frac, sizeof(param->sub_frac));

	odma_cfg->csc_cfg.mode = SCL_OUT_QUANT;
	odma_cfg->csc_cfg.quant_round = (enum sclr_quant_rounding)param->rounding;
	odma_cfg->csc_cfg.work_on_border = false;

	sclr_ctrl_set_output(dev_idx, &odma_cfg->csc_cfg, odma_cfg->fmt);

	// if fmt is yuv, try use img'csc to convert rgb to yuv.
	if (IS_YUV_FMT(odma_cfg->fmt)) {
		struct sclr_img_cfg *img_cfg = sclr_img_get_cfg(dev_idx);

		img_cfg->csc = (IS_YUV_FMT(img_cfg->fmt))
			     ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_RGB2YUV;

		//if (idev->is_online_from_isp) {
		//	TRACE_VPSS(DBG_ERR, "quant for yuv not work in online.\n");
		//	return -EINVAL;
		//}
		sclr_img_set_cfg(dev_idx, img_cfg);
	}

	return 0;
}

static int _sc_ext_set_convertto(u8 dev_idx, const struct convertto_param *param)
{
	struct sclr_odma_cfg *odma_cfg = sclr_odma_get_cfg(dev_idx);

	if (!param->enable)
		return 0;

	if ((odma_cfg->fmt != SCL_FMT_RGB_PLANAR) &&
		(odma_cfg->fmt != SCL_FMT_BF16)){
		TRACE_VPSS(DBG_ERR, "convertto only support rgb planar + bf16.\n");
		return -1;
	}
	memcpy(odma_cfg->csc_cfg.convert_to_cfg.a_frac, param->a_frac, sizeof(param->a_frac));
	memcpy(odma_cfg->csc_cfg.convert_to_cfg.b_frac, param->b_frac, sizeof(param->b_frac));

	odma_cfg->csc_cfg.mode = SCL_OUT_CONVERT_TO;
	odma_cfg->csc_cfg.work_on_border = false;

	sclr_ctrl_set_output(dev_idx, &odma_cfg->csc_cfg, odma_cfg->fmt);

	return 0;
}

static void _sc_ext_set_border(u8 dev_idx, const struct sc_border_param *param)
{
	struct sclr_border_cfg cfg;
	struct sclr_odma_cfg *odma_cfg;

	if (param->enable) {
		// full-size odma for border
		odma_cfg = sclr_odma_get_cfg(dev_idx);
		odma_cfg->mem.start_x = odma_cfg->mem.start_y = 0;
		odma_cfg->mem.width = odma_cfg->frame_size.w;
		odma_cfg->mem.height = odma_cfg->frame_size.h;
		sclr_odma_set_mem(dev_idx, &odma_cfg->mem);
	}

	cfg.cfg.b.enable = param->enable;
	cfg.cfg.b.bd_color_r = param->bg_color[0];
	cfg.cfg.b.bd_color_g = param->bg_color[1];
	cfg.cfg.b.bd_color_b = param->bg_color[2];
	cfg.start.x = param->offset_x;
	cfg.start.y = param->offset_y;
	sclr_border_set_cfg(dev_idx, &cfg);
}

static void _sc_ext_set_border_vpp(u8 dev_idx, const struct sc_border_vpp_param *param)
{
	int i;
	struct sclr_border_vpp_cfg border_cfg;

	for (i = 0; i < VPSS_RECT_NUM; i++) {
		border_cfg.cfg.b.enable = param[i].enable;
		border_cfg.cfg.b.bd_color_r = param[i].bg_color[0];
		border_cfg.cfg.b.bd_color_g = param[i].bg_color[1];
		border_cfg.cfg.b.bd_color_b = param[i].bg_color[2];
		border_cfg.inside_start.x = param[i].inside.start_x;
		border_cfg.inside_start.y = param[i].inside.start_y;
		border_cfg.inside_end.x = param[i].inside.end_x;
		border_cfg.inside_end.y = param[i].inside.end_y;
		border_cfg.outside_start.x = param[i].outside.start_x;
		border_cfg.outside_start.y = param[i].outside.start_y;
		border_cfg.outside_end.x = param[i].outside.end_x;
		border_cfg.outside_end.y = param[i].outside.end_y;

		sclr_border_vpp_set_cfg(dev_idx, i, &border_cfg, true);
	}
}


static void _sc_ext_set_coverex(u8 dev_idx, const struct rgn_coverex_cfg *cfg)
{
	int i;
	struct sclr_cover_cfg sc_cover_cfg;

	for (i = 0; i < RGN_COVEREX_MAX_NUM; i++) {
		if (cfg->rgn_coverex_param[i].enable) {
			sc_cover_cfg.start.raw = 0;
			sc_cover_cfg.color.raw = 0;
			sc_cover_cfg.start.b.enable = 1;
			sc_cover_cfg.start.b.x = cfg->rgn_coverex_param[i].rect.left;
			sc_cover_cfg.start.b.y = cfg->rgn_coverex_param[i].rect.top;
			sc_cover_cfg.img_size.w = cfg->rgn_coverex_param[i].rect.width;
			sc_cover_cfg.img_size.h = cfg->rgn_coverex_param[i].rect.height;
			sc_cover_cfg.color.b.cover_color_r = (cfg->rgn_coverex_param[i].color >> 16) & 0xff;
			sc_cover_cfg.color.b.cover_color_g = (cfg->rgn_coverex_param[i].color >> 8) & 0xff;
			sc_cover_cfg.color.b.cover_color_b = cfg->rgn_coverex_param[i].color & 0xff;
		} else {
			sc_cover_cfg.start.raw = 0;
		}
		sclr_cover_set_cfg(dev_idx, i, &sc_cover_cfg, true);
	}
}

static void _sc_ext_set_mask(u8 dev_idx, const struct rgn_mosaic_cfg *cfg)
{
	struct sclr_privacy_cfg mask_cfg = {0};
	//struct sclr_img_cfg *img_cfg;
	//struct vip_dev *bdev = container_of(sdev, struct vip_dev, sc_vdev[dev_idx]);
	//struct img_vdev *idev = &bdev->img_vdev[sdev->img_src];

	if (cfg->enable) {
		mask_cfg.cfg.raw = 0;
		mask_cfg.cfg.b.enable = 1;
		mask_cfg.cfg.b.mode = 0;
		mask_cfg.cfg.b.grid_size = cfg->blk_size;
		mask_cfg.cfg.b.fit_picture = 0;
		mask_cfg.cfg.b.force_alpha = 0;
		mask_cfg.cfg.b.mask_rgb332 = 0;
		//mask_cfg.cfg.b.blend_y = 1;
		//mask_cfg.cfg.b.y2r_enable = 1;
		mask_cfg.map_cfg.alpha_factor = 256;
		mask_cfg.map_cfg.no_mask_idx = 0;
		mask_cfg.map_cfg.base = cfg->phy_addr;
		mask_cfg.map_cfg.axi_burst = 7;
		mask_cfg.start.x = cfg->start_x;
		mask_cfg.end.x = cfg->end_x - 1;
		mask_cfg.start.y = cfg->start_y;
		mask_cfg.end.y = cfg->end_y - 1;

		//img_cfg= sclr_img_get_cfg(idev->img_type);
		//img_cfg->csc = SCL_CSC_NONE;
		//sclr_img_set_cfg(idev->img_type, img_cfg);
	} else {
		mask_cfg.cfg.raw = 0;
	}
	sclr_pri_set_cfg(dev_idx, &mask_cfg);
}

static void _sc_ext_set_y_ratio(u8 dev_idx, u32 y_ratio, enum sclr_csc csc_type)
{
	int i;
	struct sclr_csc_matrix csc_matrix;
	struct sclr_csc_matrix *def_matrix = sclr_get_csc_mtrx(csc_type);

	csc_matrix = *def_matrix;
	for (i = 0; i < 3; i++)
		csc_matrix.coef[0][i] = (def_matrix->coef[0][i] * y_ratio) / YRATIO_SCALE;

	sclr_set_csc(dev_idx, &csc_matrix);
}

bool img_left_tile_cfg(u8 dev_idx, u16 online_l_width)
{
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(dev_idx);
	struct sclr_mem mem = cfg->mem;
	struct sclr_fbd_cfg fbd_cfg = *sclr_fbd_get_cfg(dev_idx);
	struct sclr_core_cfg *sc = sclr_get_cfg(dev_idx);

	TRACE_VPSS(DBG_DEBUG, "img-%d: tile on left.\n", dev_idx);

#ifdef TILE_ON_IMG
	if (online_l_width)
		mem.width = online_l_width;
	else
		mem.width = (sc->sc.tile.in_mem.w >> 1) + TILE_GUARD_PIXEL;
	sclr_img_set_mem(dev_idx, &mem, true);
	if(fbd_cfg.enable){
		int fbd_height = ((sc->sc.tile.in_mem.h + 15) >> 4) << 2;
		fbd_cfg.crop.x = mem.start_x;
		fbd_cfg.crop.y = mem.start_y;
		fbd_cfg.crop.w = mem.width;
		fbd_cfg.crop.h = mem.height;
		fbd_cfg.offset_base_y += (((fbd_cfg.crop.x >> 8) * fbd_height) << 5) + ((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.offset_base_c += (((fbd_cfg.crop.x >> 9) * fbd_height) << 5) + ((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.comp_base_y += (fbd_cfg.crop.y >> 2) * fbd_cfg.stride_y;
		fbd_cfg.comp_base_c += (fbd_cfg.crop.y >> 2) * ALIGN(ALIGN(((sc->sc.tile.in_mem.w + 1) >> 1), 16) << 2, 32);
		sclr_set_fbd(dev_idx, &fbd_cfg, false);
	}
	TRACE_VPSS(DBG_DEBUG, "img-%d start_x(%d) width(%d).\n", dev_idx, mem.start_x, mem.width);
#endif
	return sclr_left_tile(dev_idx, mem.width);
}

bool img_right_tile_cfg(u8 dev_idx, u16 online_r_start, u16 online_r_end)
{
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(dev_idx);
	struct sclr_mem mem = cfg->mem;
	struct sclr_fbd_cfg fbd_cfg = *sclr_fbd_get_cfg(dev_idx);
	struct sclr_core_cfg *sc_cfg = sclr_get_cfg(dev_idx);
	u32 sc_offset;
	u16 src_width = sc_cfg->sc.tile.in_mem.w;

	TRACE_VPSS(DBG_DEBUG, "img-%d: tile on right.\n", dev_idx);

#ifdef TILE_ON_IMG
	if (online_r_start && online_r_end) {
		mem.width = online_r_end - online_r_start + 1;
		sc_offset = online_r_start;
	} else {
		sc_offset = (sc_cfg->sc.tile.in_mem.w >> 1) - TILE_GUARD_PIXEL;
		mem.start_x += sc_offset;
		mem.width = src_width - sc_offset;
	}
	sclr_img_set_mem(dev_idx, &mem, true);
	if(fbd_cfg.enable){
		int fbd_height = ((sc_cfg->sc.tile.in_mem.h + 15) >> 4) << 2;
		fbd_cfg.crop.x = mem.start_x;
		fbd_cfg.crop.y = mem.start_y;
		fbd_cfg.crop.w = mem.width;
		fbd_cfg.crop.h = mem.height;
		fbd_cfg.offset_base_y += (((fbd_cfg.crop.x >> 8) * fbd_height) << 5) + ((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.offset_base_c += (((fbd_cfg.crop.x >> 9) * fbd_height) << 5) + ((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.comp_base_y += (fbd_cfg.crop.y >> 2) * fbd_cfg.stride_y;
		fbd_cfg.comp_base_c += (fbd_cfg.crop.y >> 2) * ALIGN(ALIGN(((sc_cfg->sc.tile.in_mem.w + 1) >> 1), 16) << 2, 32);
		sclr_set_fbd(dev_idx, &fbd_cfg, false);
	}
	TRACE_VPSS(DBG_DEBUG, "img-%d start_x(%d) width(%d).\n", dev_idx, mem.start_x, mem.width);
#endif
	return sclr_right_tile(dev_idx, sc_offset);
}

bool img_top_tile_cfg(u8 dev_idx, u8 is_right)
{
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(dev_idx);
	struct sclr_mem mem = cfg->mem;
	struct sclr_fbd_cfg fbd_cfg = *sclr_fbd_get_cfg(dev_idx);
	struct sclr_core_cfg *sc = sclr_get_cfg(dev_idx);
	TRACE_VPSS(DBG_DEBUG, "img-%d: tile on top.\n", dev_idx);

#ifdef TILE_ON_IMG
	mem.height = (sc->sc.v_tile.in_mem.h >> 1) + TILE_GUARD_PIXEL;
	sclr_img_set_mem(dev_idx, &mem, false);
	if(fbd_cfg.enable){
		int fbd_height = ((sc->sc.v_tile.in_mem.h + 15) >> 4) << 2;
		fbd_cfg.crop.x = mem.start_x;
		fbd_cfg.crop.y = mem.start_y;
		fbd_cfg.crop.w = mem.width;
		fbd_cfg.crop.h = mem.height;
		fbd_cfg.offset_base_y += (((fbd_cfg.crop.x >> 8) * fbd_height) << 5) + ((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.offset_base_c += (((fbd_cfg.crop.x >> 9) * fbd_height) << 5) + ((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.comp_base_y += (fbd_cfg.crop.y >> 2) * fbd_cfg.stride_y;
		fbd_cfg.comp_base_c += (fbd_cfg.crop.y >> 2) * ALIGN(ALIGN(((sc->sc.v_tile.in_mem.w + 1) >> 1), 16) << 2, 32);
		sclr_set_fbd(dev_idx, &fbd_cfg, false);
	}
	TRACE_VPSS(DBG_DEBUG, "img-%d start_y(%d) height(%d).\n", dev_idx, mem.start_y, mem.height);
#endif
	return sclr_top_tile(dev_idx, mem.height, is_right);
}

bool img_down_tile_cfg(u8 dev_idx, u8 is_right)
{
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(dev_idx);
	struct sclr_mem mem = cfg->mem;
	struct sclr_fbd_cfg fbd_cfg = *sclr_fbd_get_cfg(dev_idx);
	struct sclr_core_cfg *sc = sclr_get_cfg(dev_idx);
	u32 sc_offset;

	TRACE_VPSS(DBG_DEBUG, "img-%d: tile on down.\n", dev_idx);

#ifdef TILE_ON_IMG
	sc_offset = (sc->sc.v_tile.in_mem.h >> 1) - TILE_GUARD_PIXEL;
	mem.start_y += sc_offset;
	mem.height = mem.height - sc_offset;
	sclr_img_set_mem(dev_idx, &mem, false);
	if(fbd_cfg.enable){
		int fbd_height = ((sc->sc.v_tile.in_mem.h + 15) >> 4) << 2;
		fbd_cfg.crop.x = mem.start_x;
		fbd_cfg.crop.y = mem.start_y;
		fbd_cfg.crop.w = mem.width;
		fbd_cfg.crop.h = mem.height;
		fbd_cfg.offset_base_y += (((fbd_cfg.crop.x >> 8) * fbd_height) << 5) + ((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.offset_base_c += (((fbd_cfg.crop.x >> 9) * fbd_height) << 5) + ((fbd_cfg.crop.y >> 2) << 5);
		fbd_cfg.comp_base_y += (fbd_cfg.crop.y >> 2) * fbd_cfg.stride_y;
		fbd_cfg.comp_base_c += (fbd_cfg.crop.y >> 2) * ALIGN(ALIGN(((sc->sc.v_tile.in_mem.w + 1) >> 1), 16) << 2, 32);
		sclr_set_fbd(dev_idx, &fbd_cfg, false);
	}
	TRACE_VPSS(DBG_DEBUG, "img-%d start_y(%d) height(%d).\n", dev_idx, mem.start_y, mem.height);
#endif
	return sclr_down_tile(dev_idx, sc_offset, is_right);
}

enum sclr_csc get_img_csc(u32 pixelformat)
{
	enum sclr_format fmt;
	enum sclr_csc csc_type;

	fmt = user_fmt_to_hw(pixelformat);

	if (pixelformat == PIXEL_FORMAT_YUV_PLANAR_444 || pixelformat == PIXEL_FORMAT_YUV_444)
		csc_type = SCL_CSC_601_LIMIT_YUV2RGB;
	else if (IS_YUV_FMT(fmt))
		csc_type = (fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_YUV2RGB;
	else
		csc_type = SCL_CSC_NONE;

	return csc_type;
}

enum sclr_csc get_sc_csc(u32 pixelformat)
{
	enum sclr_format fmt;
	enum sclr_csc csc_type;

	fmt = user_fmt_to_hw(pixelformat);

	if (pixelformat == PIXEL_FORMAT_YUV_PLANAR_444 || pixelformat == PIXEL_FORMAT_YUV_444) {
		csc_type = SCL_CSC_601_LIMIT_RGB2YUV;
	} else if (IS_YUV_FMT(fmt)) {
		csc_type = (fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_RGB2YUV;
	} else {
		csc_type = SCL_CSC_NONE;
	}

	return csc_type;
}

int vpss_stitch_platform(u8 dev_idx, struct vpss_cmdq_buf *cmdq_buf, struct vpss_stitch_cfg *cfg)
{
	int i;
	u8 stitch_num = cfg->num;
	struct sclr_ctrl_cfg *ctrl_cfg = NULL;
	union sclr_intr mask;
	struct vpss_window *window;

	ctrl_cfg = (struct sclr_ctrl_cfg *)vzalloc(sizeof(struct sclr_ctrl_cfg) * stitch_num);
	if (!ctrl_cfg) {
		TRACE_VPSS(DBG_ERR, "vzalloc fail.\n");
		return ERR_VPSS_NOMEM;
	}

	for (i = 0; i < stitch_num; i++) {
		window = &cfg->chn_cfg[i].window;

		if ((window->rect_in.left < window->rect_out.left) ||
			(window->rect_in.top < window->rect_out.top) ||
			((window->rect_out.left + window->rect_out.width) <
			(window->rect_in.left + window->rect_in.width)) ||
			((window->rect_out.top + window->rect_out.height) <
			(window->rect_in.top + window->rect_in.height))) {
			TRACE_VPSS(DBG_ERR, "inside rectangle invalid.\n");
			vfree(ctrl_cfg);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		if (((window->top_width + window->bottom_width) >= window->rect_out.height) ||
			((window->left_width + window->right_width) >= window->rect_out.width)) {
			TRACE_VPSS(DBG_ERR, "border width invalid.\n");
			vfree(ctrl_cfg);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		ctrl_cfg[i].img_inst = dev_idx;
		ctrl_cfg[i].input = SCL_INPUT_MEM;
		ctrl_cfg[i].src_fmt = user_fmt_to_hw(cfg->chn_cfg[i].pixelformat);
		ctrl_cfg[i].src_csc = get_img_csc(cfg->chn_cfg[i].pixelformat);
		ctrl_cfg[i].src.w = cfg->chn_cfg[i].src_size.width;
		ctrl_cfg[i].src.h = cfg->chn_cfg[i].src_size.height;
		ctrl_cfg[i].src_crop.x = cfg->chn_cfg[i].rect_crop.left;
		ctrl_cfg[i].src_crop.y = cfg->chn_cfg[i].rect_crop.top;
		ctrl_cfg[i].src_crop.w = cfg->chn_cfg[i].rect_crop.width;
		ctrl_cfg[i].src_crop.h = cfg->chn_cfg[i].rect_crop.height;
		ctrl_cfg[i].pitch_y = cfg->chn_cfg[i].bytesperline[0];
		ctrl_cfg[i].pitch_c = cfg->chn_cfg[i].bytesperline[1];
		ctrl_cfg[i].src_addr0 = cfg->chn_cfg[i].addr[0];
		ctrl_cfg[i].src_addr1 = cfg->chn_cfg[i].addr[1];
		ctrl_cfg[i].src_addr2 = cfg->chn_cfg[i].addr[2];

		ctrl_cfg[i].dst.inst = dev_idx;
		ctrl_cfg[i].dst.flip_mode = window->flip;
		ctrl_cfg[i].dst.fmt = user_fmt_to_hw(cfg->dst_cfg.pixelformat);
		ctrl_cfg[i].dst.csc = get_sc_csc(cfg->dst_cfg.pixelformat);
		ctrl_cfg[i].dst.addr0 = cfg->dst_cfg.addr[0];
		ctrl_cfg[i].dst.addr1 = cfg->dst_cfg.addr[1];
		ctrl_cfg[i].dst.addr2 = cfg->dst_cfg.addr[2];

		if (i == 0) {
			ctrl_cfg[i].border_cfg.cfg.b.enable = 1;
			ctrl_cfg[i].border_cfg.cfg.b.bd_color_r = cfg->dst_cfg.color[0];
			ctrl_cfg[i].border_cfg.cfg.b.bd_color_g = cfg->dst_cfg.color[1];
			ctrl_cfg[i].border_cfg.cfg.b.bd_color_b = cfg->dst_cfg.color[2];
			ctrl_cfg[i].border_cfg.start.x = window->rect_in.left;
			ctrl_cfg[i].border_cfg.start.y = window->rect_in.top;
			ctrl_cfg[i].dst.offset.x = 0;
			ctrl_cfg[i].dst.offset.y = 0;
			ctrl_cfg[i].dst.window.w = cfg->dst_cfg.dst_size.width;
			ctrl_cfg[i].dst.window.h = cfg->dst_cfg.dst_size.height;
			ctrl_cfg[i].dst.frame.w = window->rect_in.width;
			ctrl_cfg[i].dst.frame.h = window->rect_in.height;

			//background: vpp border[0]
			if ((window->rect_in.left != window->rect_out.left) ||
				(window->rect_in.top != window->rect_out.top) ||
				(window->rect_in.width != window->rect_out.width) ||
				(window->rect_in.height != window->rect_out.height)) {
				ctrl_cfg[i].border_vpp_cfg[0].cfg.b.enable = 1;
				ctrl_cfg[i].border_vpp_cfg[0].cfg.b.bd_color_r = window->bgcolor[0];
				ctrl_cfg[i].border_vpp_cfg[0].cfg.b.bd_color_g = window->bgcolor[1];
				ctrl_cfg[i].border_vpp_cfg[0].cfg.b.bd_color_b = window->bgcolor[2];
				ctrl_cfg[i].border_vpp_cfg[0].inside_start.x = window->rect_in.left;
				ctrl_cfg[i].border_vpp_cfg[0].inside_start.y = window->rect_in.top;
				ctrl_cfg[i].border_vpp_cfg[0].inside_end.x = window->rect_in.left + window->rect_in.width;
				ctrl_cfg[i].border_vpp_cfg[0].inside_end.y = window->rect_in.top + window->rect_in.height;
				ctrl_cfg[i].border_vpp_cfg[0].outside_start.x = window->rect_out.left;
				ctrl_cfg[i].border_vpp_cfg[0].outside_start.y = window->rect_out.top;
				ctrl_cfg[i].border_vpp_cfg[0].outside_end.x = window->rect_out.left + window->rect_out.width;
				ctrl_cfg[i].border_vpp_cfg[0].outside_end.y = window->rect_out.top + window->rect_out.height;
			}

			//border vpp border[1]
			if (window->top_width || window->left_width ||
				window->right_width || window->bottom_width) {
				ctrl_cfg[i].border_vpp_cfg[1].cfg.b.enable = 1;
				ctrl_cfg[i].border_vpp_cfg[1].cfg.b.bd_color_r = window->border_color[0];
				ctrl_cfg[i].border_vpp_cfg[1].cfg.b.bd_color_g = window->border_color[1];
				ctrl_cfg[i].border_vpp_cfg[1].cfg.b.bd_color_b = window->border_color[2];
				ctrl_cfg[i].border_vpp_cfg[1].inside_start.x = window->rect_out.left + window->left_width;
				ctrl_cfg[i].border_vpp_cfg[1].inside_start.y = window->rect_out.top + window->top_width;
				ctrl_cfg[i].border_vpp_cfg[1].inside_end.x = window->rect_out.left + window->rect_out.width -
															window->right_width;
				ctrl_cfg[i].border_vpp_cfg[1].inside_end.y = window->rect_out.top + window->rect_out.height -
															window->bottom_width;
				ctrl_cfg[i].border_vpp_cfg[1].outside_start.x = window->rect_out.left;
				ctrl_cfg[i].border_vpp_cfg[1].outside_start.y = window->rect_out.top;
				ctrl_cfg[i].border_vpp_cfg[1].outside_end.x = window->rect_out.left + window->rect_out.width;
				ctrl_cfg[i].border_vpp_cfg[1].outside_end.y = window->rect_out.top + window->rect_out.height;
			}
		} else {
			ctrl_cfg[i].border_cfg.cfg.b.enable = 1;
			ctrl_cfg[i].border_cfg.cfg.b.bd_color_r = window->bgcolor[0];
			ctrl_cfg[i].border_cfg.cfg.b.bd_color_g = window->bgcolor[1];
			ctrl_cfg[i].border_cfg.cfg.b.bd_color_b = window->bgcolor[2];
			ctrl_cfg[i].border_cfg.start.x = window->rect_in.left - window->rect_out.left;
			ctrl_cfg[i].border_cfg.start.y = window->rect_in.top - window->rect_out.top;
			ctrl_cfg[i].dst.offset.x = window->rect_out.left;
			ctrl_cfg[i].dst.offset.y = window->rect_out.top;
			ctrl_cfg[i].dst.window.w = window->rect_out.width;
			ctrl_cfg[i].dst.window.h = window->rect_out.height;
			ctrl_cfg[i].dst.frame.w = window->rect_in.width;
			ctrl_cfg[i].dst.frame.h = window->rect_in.height;

			//border vpp border[0]
			if (window->top_width || window->left_width ||
				window->right_width || window->bottom_width) {
				ctrl_cfg[i].border_vpp_cfg[0].cfg.b.enable = 1;
				ctrl_cfg[i].border_vpp_cfg[0].cfg.b.bd_color_r = window->border_color[0];
				ctrl_cfg[i].border_vpp_cfg[0].cfg.b.bd_color_g = window->border_color[1];
				ctrl_cfg[i].border_vpp_cfg[0].cfg.b.bd_color_b = window->border_color[2];
				ctrl_cfg[i].border_vpp_cfg[0].inside_start.x = window->left_width;
				ctrl_cfg[i].border_vpp_cfg[0].inside_start.y = window->top_width;
				ctrl_cfg[i].border_vpp_cfg[0].inside_end.x = window->rect_out.width - window->right_width;
				ctrl_cfg[i].border_vpp_cfg[0].inside_end.y = window->rect_out.height - window->bottom_width;
				ctrl_cfg[i].border_vpp_cfg[0].outside_start.x = 0;
				ctrl_cfg[i].border_vpp_cfg[0].outside_start.y = 0;
				ctrl_cfg[i].border_vpp_cfg[0].outside_end.x = window->rect_out.width;
				ctrl_cfg[i].border_vpp_cfg[0].outside_end.y = window->rect_out.height;
			}
		}

		ctrl_cfg[i].dst.pitch_y = cfg->dst_cfg.bytesperline[0];
		ctrl_cfg[i].dst.pitch_c = cfg->dst_cfg.bytesperline[1];
	}

	mask.raw = 0;
	mask.b.cmdq = true;
	sclr_set_intr_mask(dev_idx, mask);
	//TODO:recover intr

	sclr_engine_cmdq(dev_idx, ctrl_cfg, stitch_num, cmdq_buf->cmdq_phy_addr,
						cmdq_buf->cmdq_vir_addr, cmdq_buf->cmdq_buf_size);
	vfree(ctrl_cfg);
	return 0;
}

void sc_update(u8 dev_idx, const struct vpss_hal_chn_cfg *chn_cfg)
{
	enum sclr_format fmt;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_core_cfg *core_cfg;
	struct sclr_cir_cfg cir_cfg;
	const struct rgn_cfg *rgn_cfg;
	u8 i, ow_idx;
	bool cir_bypass = true, gop_bypass = true;

	TRACE_VPSS(DBG_DEBUG, "-- sc%d --\n", dev_idx);
	TRACE_VPSS(DBG_DEBUG, "update sc(%d) buf 0x%llx-0x%llx-0x%llx\n",
		dev_idx, chn_cfg->addr[0], chn_cfg->addr[1], chn_cfg->addr[2]);
	TRACE_VPSS(DBG_DEBUG, "%10s(%4d * %4d)%10s(%4d)\n", "src size",
		chn_cfg->src_size.width, chn_cfg->src_size.height,
		"sc coef", chn_cfg->sc_coef);
	TRACE_VPSS(DBG_DEBUG, "%10s(%4d %4d %4d %4d)\n", "crop rect",
		chn_cfg->crop.left, chn_cfg->crop.top,
		chn_cfg->crop.width, chn_cfg->crop.height);
	TRACE_VPSS(DBG_DEBUG, "%10s(%4d %4d %4d %4d)\n", "dst_rect",
		chn_cfg->dst_rect.left, chn_cfg->dst_rect.top,
		chn_cfg->dst_rect.width, chn_cfg->dst_rect.height);
	TRACE_VPSS(DBG_DEBUG, "%10s(%4d)%10s(%4d)\n", "pitch_y", chn_cfg->bytesperline[0],
		"pitch_c", chn_cfg->bytesperline[1]);

	/*core config*/
	core_cfg = sclr_get_cfg(dev_idx);
	odma_cfg = sclr_odma_get_cfg(dev_idx);

	core_cfg->sc.src.w = chn_cfg->src_size.width;
	core_cfg->sc.src.h = chn_cfg->src_size.height;
	core_cfg->sc.crop.x = chn_cfg->crop.left;
	core_cfg->sc.crop.y = chn_cfg->crop.top;
	core_cfg->sc.crop.w = chn_cfg->crop.width;
	core_cfg->sc.crop.h = chn_cfg->crop.height;
	core_cfg->sc.dst.w = chn_cfg->dst_rect.width;
	core_cfg->sc.dst.h = chn_cfg->dst_rect.height;
	core_cfg->sc.algorithm = (enum sclr_algorithm)chn_cfg->sc_coef;
	core_cfg->sc.tile_enable = 0;
	core_cfg->sc.mir_enable = 0;
	sclr_core_set_cfg(dev_idx, core_cfg);
	sclr_core_checksum_en(dev_idx, true);

	/*odma config*/
	fmt = user_fmt_to_hw(chn_cfg->pixelformat);
	odma_cfg->fmt = fmt;
	odma_cfg->flip = (enum sclr_flip_mode)chn_cfg->flip;
	odma_cfg->frame_size.w = chn_cfg->dst_size.width;
	odma_cfg->frame_size.h = chn_cfg->dst_size.height;
	odma_cfg->mem.addr0 = chn_cfg->addr[0];
	odma_cfg->mem.addr1 = chn_cfg->addr[1];
	odma_cfg->mem.addr2 = chn_cfg->addr[2];
	odma_cfg->mem.pitch_y = chn_cfg->bytesperline[0];
	odma_cfg->mem.pitch_c = chn_cfg->bytesperline[1];
	odma_cfg->mem.width = chn_cfg->dst_rect.width;
	odma_cfg->mem.height = chn_cfg->dst_rect.height;
	// update out-2-mem's pos
	if ((odma_cfg->flip == SCL_FLIP_HFLIP) || (odma_cfg->flip == SCL_FLIP_HVFLIP))
		odma_cfg->mem.start_x = chn_cfg->dst_size.width - chn_cfg->dst_rect.width - chn_cfg->dst_rect.left;
	else
		odma_cfg->mem.start_x = chn_cfg->dst_rect.left;
	if ((odma_cfg->flip == SCL_FLIP_VFLIP) || (odma_cfg->flip == SCL_FLIP_HVFLIP))
		odma_cfg->mem.start_y = chn_cfg->dst_size.height - chn_cfg->dst_rect.height - chn_cfg->dst_rect.top;
	else
		odma_cfg->mem.start_y = chn_cfg->dst_rect.top;

	odma_cfg->csc_cfg.datatype = user_fmt_to_datatype(chn_cfg->pixelformat);
	odma_cfg->csc_cfg.work_on_border = true;
	if ((chn_cfg->pixelformat == PIXEL_FORMAT_HSV_888) || (chn_cfg->pixelformat == PIXEL_FORMAT_HSV_888_PLANAR))
		odma_cfg->csc_cfg.mode = SCL_OUT_HSV;
	else if ((chn_cfg->pixelformat == PIXEL_FORMAT_YUV_PLANAR_444) || (chn_cfg->pixelformat == PIXEL_FORMAT_YUV_444)) {
		odma_cfg->csc_cfg.mode = SCL_OUT_CSC;
		odma_cfg->csc_cfg.csc_type = SCL_CSC_601_LIMIT_RGB2YUV;
	} else if (IS_YUV_FMT(odma_cfg->fmt)) {
		odma_cfg->csc_cfg.mode = SCL_OUT_CSC;
		odma_cfg->csc_cfg.csc_type = (odma_cfg->fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_RGB2YUV;
	} else {
		odma_cfg->csc_cfg.mode = SCL_OUT_DISABLE;
		odma_cfg->csc_cfg.csc_type = SCL_CSC_NONE;
	}
	sclr_odma_set_cfg(dev_idx, odma_cfg);
	//user csc
	if (odma_cfg->csc_cfg.mode == SCL_OUT_CSC)
		sclr_set_csc(dev_idx, (struct sclr_csc_matrix *)&chn_cfg->csc_cfg);

	/*y rotio*/
	if ((odma_cfg->csc_cfg.mode == SCL_OUT_CSC) && (chn_cfg->y_ratio != YRATIO_SCALE))
		_sc_ext_set_y_ratio(dev_idx, chn_cfg->y_ratio, odma_cfg->csc_cfg.csc_type);

	TRACE_VPSS(DBG_DEBUG, "%10s(%4d)%10s(%4d)%10s(%4d)\n", "fmt",
		odma_cfg->fmt, "csc mode", odma_cfg->csc_cfg.mode,
		"csc_type", odma_cfg->csc_cfg.csc_type);

	/*cir*/
	if (chn_cfg->mute_cfg.enable) {
		cir_bypass = false;
		cir_cfg.mode = SCL_CIR_SHAPE;
		cir_cfg.rect.x = 0;
		cir_cfg.rect.y = 0;
		cir_cfg.rect.w = chn_cfg->dst_size.width;
		cir_cfg.rect.h = chn_cfg->dst_size.height;
		cir_cfg.center.x = chn_cfg->dst_size.width >> 1;
		cir_cfg.center.y = chn_cfg->dst_size.height >> 1;
		cir_cfg.radius = MAX(cir_cfg.rect.w, cir_cfg.rect.h);
		cir_cfg.color_r = chn_cfg->mute_cfg.color[0];
		cir_cfg.color_g = chn_cfg->mute_cfg.color[1];
		cir_cfg.color_b = chn_cfg->mute_cfg.color[2];
	} else {
		cir_cfg.mode = SCL_CIR_DISABLE;
	}
	sclr_cir_set_cfg(dev_idx, &cir_cfg);

	/*border*/
	_sc_ext_set_border(dev_idx, &chn_cfg->border_cfg);
	TRACE_VPSS(DBG_DEBUG, "%10s(%4d)%10s(%4d %4d)%10s(%4d %4d %4d)\n",
		"border enable", chn_cfg->border_cfg.enable,
		"offset", chn_cfg->border_cfg.offset_x, chn_cfg->border_cfg.offset_y,
		"bgcolor", chn_cfg->border_cfg.bg_color[0], chn_cfg->border_cfg.bg_color[1],
		chn_cfg->border_cfg.bg_color[2]);

	/*border vpp*/
	_sc_ext_set_border_vpp(dev_idx, chn_cfg->border_vpp_cfg);

	/*convertto*/
	_sc_ext_set_convertto(dev_idx, &chn_cfg->convert_to_cfg);
	TRACE_VPSS(DBG_DEBUG, "%15s(%4d)%10s(%4d %4d %4d)%10s(%4d %4d %4d)\n",
		"convert enable", chn_cfg->convert_to_cfg.enable,
		"a_frac", chn_cfg->convert_to_cfg.a_frac[0], chn_cfg->convert_to_cfg.a_frac[1],
		chn_cfg->convert_to_cfg.a_frac[2],
		"b_frac", chn_cfg->convert_to_cfg.b_frac[0], chn_cfg->convert_to_cfg.b_frac[1],
		chn_cfg->convert_to_cfg.b_frac[2]);

	/*quant*/
	_sc_ext_set_quant(dev_idx, &chn_cfg->quant_cfg);
	TRACE_VPSS(DBG_DEBUG, "%15s(%4d)%10s(%4d)%10s(%4d %4d %4d)%10s(%4d %4d %4d)%10s(%4d %4d %4d)\n",
		"quant enable", chn_cfg->quant_cfg.enable,
		"rounding", chn_cfg->quant_cfg.rounding,
		"factor", chn_cfg->quant_cfg.sc_frac[0], chn_cfg->quant_cfg.sc_frac[1], chn_cfg->quant_cfg.sc_frac[2],
		"sub", chn_cfg->quant_cfg.sub[0], chn_cfg->quant_cfg.sub[1], chn_cfg->quant_cfg.sub[2],
		"sub_frac", chn_cfg->quant_cfg.sub_frac[0], chn_cfg->quant_cfg.sub_frac[1],
		chn_cfg->quant_cfg.sub_frac[2]);

	/*cover*/
	_sc_ext_set_coverex(dev_idx, &chn_cfg->rgn_coverex_cfg);
	/*mosaic*/
	_sc_ext_set_mask(dev_idx, &chn_cfg->rgn_mosaic_cfg);

	/*gop*/
	for (i = 0; i < RGN_MAX_LAYER_VPSS; ++i) {
		rgn_cfg = &chn_cfg->rgn_cfg[i];
		_sc_ext_set_rgn_cfg(dev_idx, i, rgn_cfg, &sclr_get_cfg(dev_idx)->sc.dst);
		TRACE_VPSS(DBG_DEBUG, "gop%d:%10s(%4d)%10s(%4d)%10s(%4d)%10s(%4d)%10s(0x%08x)\n", i,
			"rgn number", rgn_cfg->num_of_rgn,
			"hscale_x2", rgn_cfg->hscale_x2,
			"vscale_x2", rgn_cfg->vscale_x2,
			"colorkey_en", rgn_cfg->colorkey_en,
			"colorkey", rgn_cfg->colorkey);
		if (rgn_cfg->odec.enable) {
			gop_bypass = false;
			TRACE_VPSS(DBG_DEBUG, "gop%d odec:%10s(%4d)%10s(%4d)%10s(%4d)\n", i,
				       "enable", rgn_cfg->odec.enable,
				       "attached_ow", rgn_cfg->odec.attached_ow,
				       "bso_sz", rgn_cfg->odec.bso_sz);

			ow_idx = rgn_cfg->odec.attached_ow;
			TRACE_VPSS(DBG_DEBUG,
				"ow%d:%11s(%4d)%10s(%4d %4d %4d %4d)%10s(%6d)%10s(0x%llx)\n", ow_idx,
				"fmt", rgn_cfg->param[ow_idx].fmt,
				"rect", rgn_cfg->param[ow_idx].rect.left, rgn_cfg->param[ow_idx].rect.top,
					rgn_cfg->param[ow_idx].rect.width, rgn_cfg->param[ow_idx].rect.height,
				"stride", rgn_cfg->param[ow_idx].stride,
				"phy_addr", rgn_cfg->param[ow_idx].phy_addr);
		} else {
			for (ow_idx = 0; ow_idx < rgn_cfg->num_of_rgn; ++ow_idx){
				gop_bypass = false;
				TRACE_VPSS(DBG_DEBUG,
					"ow%d:%11s(%4d)%10s(%4d %4d %4d %4d)%10s(%4d)%10s(0x%llx)\n", ow_idx,
					"fmt", rgn_cfg->param[ow_idx].fmt,
					"rect", rgn_cfg->param[ow_idx].rect.left,
						rgn_cfg->param[ow_idx].rect.top,
						rgn_cfg->param[ow_idx].rect.width,
						rgn_cfg->param[ow_idx].rect.height,
					"stride", rgn_cfg->param[ow_idx].stride,
					"phy_addr", rgn_cfg->param[ow_idx].phy_addr);
			}
		}
	}

	sclr_set_cfg(dev_idx, false, gop_bypass, cir_bypass, false);
}

void img_update(u8 dev_idx, bool is_master, const struct vpss_hal_grp_cfg *grp_cfg)
{
	struct sclr_img_cfg *cfg;
	enum sclr_format fmt;

	fmt = user_fmt_to_hw(grp_cfg->pixelformat);
	cfg = sclr_img_get_cfg(dev_idx);

	if (grp_cfg->online_from_isp) {
		cfg->src = is_master ? SCL_INPUT_ISP : SCL_INPUT_SHARE;
		cfg->fmt = SCL_FMT_YUV422;
		cfg->csc = SCL_CSC_601_LIMIT_YUV2RGB;
	} else {
		cfg->src = is_master ? SCL_INPUT_MEM : SCL_INPUT_SHARE; //todo:dwa-vpss online
		cfg->fmt = fmt;
		if (grp_cfg->pixelformat == PIXEL_FORMAT_YUV_PLANAR_444 || grp_cfg->pixelformat == PIXEL_FORMAT_YUV_444)
			cfg->csc = SCL_CSC_601_LIMIT_YUV2RGB;
		else if (IS_YUV_FMT(cfg->fmt))
			cfg->csc = (cfg->fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_YUV2RGB;
		else
			cfg->csc = SCL_CSC_NONE;
	}

	cfg->burst = 15;
	cfg->outstanding = 15;
	cfg->fifo_y = 128;
	cfg->fifo_c = (cfg->fmt == SCL_FMT_YUV420 || cfg->fmt == SCL_FMT_YUV422) ? 64 : 128;

	cfg->mem.addr0 = grp_cfg->addr[0];
	cfg->mem.addr1 = grp_cfg->addr[1];
	cfg->mem.addr2 = grp_cfg->addr[2];
	cfg->mem.pitch_y = grp_cfg->bytesperline[0];
	cfg->mem.pitch_c = grp_cfg->bytesperline[1];
	cfg->mem.start_x = grp_cfg->crop.left;
	cfg->mem.start_y = grp_cfg->crop.top;
	cfg->mem.width	 = grp_cfg->crop.width;
	cfg->mem.height  = grp_cfg->crop.height;
	cfg->dup2fancy_enable = grp_cfg->upsample;

	if (!is_master) {
		cfg->fmt = SCL_FMT_RGB_PLANAR;
		cfg->csc = SCL_CSC_NONE;
		cfg->mem.pitch_y = ALIGN(cfg->mem.width, IMG_IN_PITCH_ALIGN);
		cfg->mem.pitch_c = ALIGN(cfg->mem.width, IMG_IN_PITCH_ALIGN);
		cfg->mem.start_x = 0;
		cfg->mem.start_y = 0;
	} else if (grp_cfg->fbd_enable) {
		cfg->src = SCL_INPUT_ISP;
		cfg->fmt = SCL_FMT_YUV420;
		cfg->csc = SCL_CSC_601_LIMIT_YUV2RGB;
		cfg->mem.pitch_y = ALIGN(grp_cfg->src_size.width, 16);
		cfg->mem.pitch_c = ALIGN(grp_cfg->src_size.width / 2, 16);
		cfg->mem.start_x = 0;
		cfg->mem.start_y = 0;
		cfg->mem.addr0 = 0;
		cfg->mem.addr1 = 0;
		cfg->mem.addr2 = 0;
	}

	_sc_ext_set_fbd(dev_idx, grp_cfg);

	TRACE_VPSS(DBG_DEBUG, "update img(%d) online_from_isp=%d, fmt=%d, csc=%d, fbd_enable=%d\n",
		dev_idx, grp_cfg->online_from_isp, cfg->fmt, cfg->csc, grp_cfg->fbd_enable);

	TRACE_VPSS(DBG_DEBUG,
		"input: w(%d) h(%d) x(%d) y(%d) pitch_y(%d) pitch_c(%d) addr(0x%llx-0x%llx-0x%llx-0x%llx).\n",
		cfg->mem.width, cfg->mem.height, cfg->mem.start_x, cfg->mem.start_y, cfg->mem.pitch_y,
		cfg->mem.pitch_c, cfg->mem.addr0, cfg->mem.addr1, cfg->mem.addr2, grp_cfg->addr[3]);

	sclr_img_checksum_en(dev_idx, true);
	sclr_img_set_cfg(dev_idx, cfg);
	//user csc
	if (cfg->csc != SCL_CSC_NONE)
		sclr_img_set_csc(dev_idx, (struct sclr_csc_matrix *)&grp_cfg->csc_cfg);
}

void top_update(u8 dev_idx, bool is_share, bool fbd_enable)
{
	sclr_top_set_src_share(dev_idx, is_share);
	sclr_top_set_cfg(dev_idx, true, fbd_enable);
}

void img_start(u8 dev_idx, u8 chn_num)
{
	int i;

	for (i = dev_idx; i < (dev_idx + chn_num); i++)
		sclr_slave_ready(i);

	sclr_img_start(dev_idx);
}

void img_reset(u8 dev_idx)
{
	sclr_vpss_sw_top_reset(dev_idx);
}

void vpss_stauts(u8 dev_idx)
{
	sclr_dump_top_register(dev_idx);
	sclr_dump_img_in_register(dev_idx);
	sclr_dump_core_register(dev_idx);
	sclr_dump_odma_register(dev_idx);
}

void vpss_error_stauts(u8 dev_idx)
{
	sclr_dump_register(dev_idx);
}


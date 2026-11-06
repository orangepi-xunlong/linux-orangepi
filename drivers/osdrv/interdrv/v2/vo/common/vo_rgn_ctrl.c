#include <linux/types.h>
#include <linux/mm.h>
#include <linux/defines.h>
#include <linux/common.h>
#include <linux/comm_buffer.h>

#include <vo.h>
#include <vo_cb.h>
#include "disp.h"

static u8 _gop_get_bpp(enum disp_gop_format fmt)
{
	return (fmt == DISP_GOP_FMT_ARGB8888) ? 4 :
		(fmt == DISP_GOP_FMT_256LUT) ? 1 : 2;
}

static int vo_set_rgn_cfg(const u8 inst, const u8 layer, const struct rgn_cfg *cfg, const struct disp_size *size)
{
	u8 i;
	struct disp_gop_cfg *gop_cfg = disp_gop_get_cfg(inst, layer);
	struct disp_gop_ow_cfg *ow_cfg;

	gop_cfg->gop_ctrl.raw &= ~0xfff;
	gop_cfg->gop_ctrl.b.hscl_en = cfg->hscale_x2;
	gop_cfg->gop_ctrl.b.vscl_en = cfg->vscale_x2;
	gop_cfg->gop_ctrl.b.colorkey_en = cfg->colorkey_en;
	gop_cfg->colorkey = cfg->colorkey;

	for (i = 0; i < cfg->num_of_rgn; ++i) {
		u8 bpp = _gop_get_bpp((enum disp_gop_format)cfg->param[i].fmt);

		ow_cfg = &gop_cfg->ow_cfg[i];
		gop_cfg->gop_ctrl.raw |= BIT(i);

		ow_cfg->fmt = (enum disp_gop_format)cfg->param[i].fmt;
		ow_cfg->addr = cfg->param[i].phy_addr;
		ow_cfg->pitch = cfg->param[i].stride;
		if (cfg->param[i].rect.left < 0) {
			ow_cfg->start.x = 0;
			ow_cfg->addr -= bpp * cfg->param[i].rect.left;
			ow_cfg->img_size.w = cfg->param[i].rect.width + cfg->param[i].rect.left;
		} else if ((cfg->param[i].rect.left + cfg->param[i].rect.width) > size->w) {
			ow_cfg->start.x = cfg->param[i].rect.left;
			ow_cfg->img_size.w = size->w - cfg->param[i].rect.left;
			ow_cfg->mem_size.w = cfg->param[i].stride;
		} else {
			ow_cfg->start.x = cfg->param[i].rect.left;
			ow_cfg->img_size.w = cfg->param[i].rect.width;
			ow_cfg->mem_size.w = cfg->param[i].stride;
		}

		if (cfg->param[i].rect.top < 0) {
			ow_cfg->start.y = 0;
			ow_cfg->addr -= ow_cfg->pitch * cfg->param[i].rect.top;
			ow_cfg->img_size.h = cfg->param[i].rect.height + cfg->param[i].rect.top;
		} else if ((cfg->param[i].rect.top + cfg->param[i].rect.height) > size->h) {
			ow_cfg->start.y = cfg->param[i].rect.top;
			ow_cfg->img_size.h = size->h - cfg->param[i].rect.top;
		} else {
			ow_cfg->start.y = cfg->param[i].rect.top;
			ow_cfg->img_size.h = cfg->param[i].rect.height;
		}

		ow_cfg->end.x = ow_cfg->start.x + (ow_cfg->img_size.w << gop_cfg->gop_ctrl.b.hscl_en)
						- gop_cfg->gop_ctrl.b.hscl_en;
		ow_cfg->end.y = ow_cfg->start.y + (ow_cfg->img_size.h << gop_cfg->gop_ctrl.b.vscl_en)
						- gop_cfg->gop_ctrl.b.vscl_en;
		ow_cfg->mem_size.w = ALIGN(ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
		ow_cfg->mem_size.h = ow_cfg->img_size.h;
#if 0
		TRACE_VO(DBG_INFO, "gop(%d) fmt(%d) rect(%d %d %d %d) addr(%llx) pitch(%d).\n", inst
			, ow_cfg->fmt, ow_cfg->start.x, ow_cfg->start.y, ow_cfg->img_size.w, ow_cfg->img_size.h
			, ow_cfg->addr, ow_cfg->pitch);
#endif
		disp_gop_ow_set_cfg(inst, layer, i, ow_cfg, true);
	}

	disp_gop_set_cfg(inst, layer, gop_cfg, true);

	return 0;
}

static int _check_vo_status(vo_layer layer)
{
	int ret = 0;

	ret = check_graphic_layer_valid(layer);
	if (ret != 0)
		return ret;

	return ret;
}

int vo_cb_get_rgn_hdls(vo_layer layer, rgn_type_e enType, rgn_handle hdls[])
{
	int ret, i;
	struct vo_overlay_ctx *overlay_ctx;

	ret = check_vo_null_ptr(ID_VO, hdls);
	if (ret != 0)
		return ret;

	ret = _check_vo_status(layer);
	if (ret != 0)
		return ret;

	overlay_ctx = &g_vo_ctx->overlay_ctx[layer - VO_MAX_VIDEO_LAYER_NUM];
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VO; ++i)
			hdls[i] = overlay_ctx->rgn_handle[i];
	} else {
		ret = ERR_VO_NOT_SUPPORT;
	}

	return ret;
}

int vo_cb_set_rgn_hdls(vo_layer layer, rgn_type_e enType, rgn_handle hdls[])
{
	int ret, i;
	struct vo_overlay_ctx *overlay_ctx;

	ret = check_vo_null_ptr(ID_VO, hdls);
	if (ret != 0)
		return ret;

	ret = _check_vo_status(layer);
	if (ret != 0)
		return ret;

	overlay_ctx = &g_vo_ctx->overlay_ctx[layer - VO_MAX_VIDEO_LAYER_NUM];
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VO; ++i)
			overlay_ctx->rgn_handle[i] = hdls[i];
	} else {
		ret = ERR_VO_NOT_SUPPORT;
	}

	return ret;
}

int vo_cb_set_rgn_cfg(vo_layer layer, struct rgn_cfg *cfg)
{
	int ret, i;
	struct vo_overlay_ctx *overlay_ctx;
	struct vo_dev_ctx *dev_ctx;
	struct disp_timing *timing;
	struct disp_size size;
	vo_dev dev;
	vo_layer overlay;
	int vgop_index = -1;

	ret = check_vo_null_ptr(ID_VO, cfg);
	if (ret != 0)
		return ret;

	ret = _check_vo_status(layer);
	if (ret != 0)
		return ret;

	overlay = layer - VO_MAX_VIDEO_LAYER_NUM;
	overlay_ctx = &g_vo_ctx->overlay_ctx[overlay];
	memcpy(&overlay_ctx->rgn_cfg, cfg, sizeof(*cfg));

	dev = overlay_ctx->bind_dev_id;
	if (dev == -1) {
		TRACE_VO(DBG_ERR, "layer(%d) not bind any Dev\n", layer);
		return ERR_VO_DEV_NOT_BINDED;
	}

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	mutex_lock(&g_vo_ctx->dev_ctx[dev].dev_lock);
	for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i)
		if (dev_ctx->bind_overlay_id[i] == layer)
			vgop_index = i;

	mutex_unlock(&g_vo_ctx->dev_ctx[dev].dev_lock);

	if (vgop_index == -1)
		return ERR_VO_DEV_NOT_BINDED;

	timing = disp_get_timing(dev);
	size.w = timing->hfde_end - timing->hfde_start + 1;
	size.h = timing->vfde_end - timing->vfde_start + 1;

	vo_set_rgn_cfg(overlay_ctx->bind_dev_id, vgop_index, cfg, &size);

	return 0;
}

int vo_cb_get_chn_size(vo_layer layer, rect_s *rect)
{
	int ret;
	struct vo_overlay_ctx *overlay_ctx;
	vo_dev dev;
	vo_layer video_layer;

	ret = check_vo_null_ptr(ID_VO, rect);
	if (ret != 0)
		return ret;

	ret = _check_vo_status(layer);
	if (ret != 0)
		return ret;

	overlay_ctx = &g_vo_ctx->overlay_ctx[layer - VO_MAX_VIDEO_LAYER_NUM];

	dev = overlay_ctx->bind_dev_id;
	if (dev == -1) {
		TRACE_VO(DBG_ERR, "layer(%d) not bind any Dev\n", layer);
		return ERR_VO_DEV_NOT_BINDED;
	}

	mutex_lock(&g_vo_ctx->dev_ctx[dev].dev_lock);
	video_layer = g_vo_ctx->dev_ctx[dev].bind_layer_id;
	mutex_unlock(&g_vo_ctx->dev_ctx[dev].dev_lock);

	memcpy(rect, &g_vo_ctx->layer_ctx[video_layer].layer_attr.disp_rect, sizeof(*rect));

	return 0;
}

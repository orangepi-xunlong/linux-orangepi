#ifndef __VO_SDK_LAYER_H__
#define __VO_SDK_LAYER_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/comm_errno.h>
#include "vo_common.h"
#include "vo_defines.h"

extern struct vo_ctx *g_vo_ctx;

static inline int check_video_layer_disable(vo_layer layer)
{
	if (g_vo_ctx->layer_ctx[layer].is_layer_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) already enabled.\n", layer);
		return ERR_VO_VIDEO_NOT_DISABLED;
	}

	return 0;
}

static inline int check_video_layer_valid(vo_layer layer)
{
	if (layer >= VO_MAX_VIDEO_LAYER_NUM) {
		TRACE_VO(DBG_ERR, "layer(%d) invalid.\n", layer);
		return ERR_VO_INVALID_LAYERID;
	}

	return 0;
}

static inline int check_video_layer_enable(vo_layer layer)
{
	if (!g_vo_ctx->layer_ctx[layer].is_layer_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) not enable.\n", layer);
		return ERR_VO_VIDEO_NOT_ENABLED;
	}

	return 0;
}

static inline int check_graphic_layer_valid(vo_layer layer)
{
	if (layer < VO_MAX_VIDEO_LAYER_NUM || layer >= VO_MAX_LAYER_NUM) {
		TRACE_VO(DBG_ERR, "layer(%d) invalid.\n", layer);
		return ERR_VO_INVALID_LAYERID;
	}

	return 0;
}

static inline int check_vo_dev_valid(vo_dev dev)
{
	if (dev >= VO_MAX_DEV_NUM || dev < 0) {
		TRACE_VO(DBG_ERR, "dev(%d) invalid.\n", dev);
		return ERR_VO_INVALID_DEVID;
	}

	return 0;
}

static inline int check_vo_chn_valid(vo_layer layer, vo_chn chn)
{
	if (layer >= VO_MAX_VIDEO_LAYER_NUM || layer < 0) {
		TRACE_VO(DBG_ERR, "layer(%d) invalid.\n", layer);
		return ERR_VO_INVALID_LAYERID;
	}
	if (chn >= VO_MAX_CHN_NUM || chn < 0) {
		TRACE_VO(DBG_ERR, "chn(%d) invalid.\n", chn);
		return ERR_VO_INVALID_CHNID;
	}

	return 0;
}

static inline int check_vo_chn_enable(vo_layer layer, vo_chn chn)
{
	if (!g_vo_ctx->layer_ctx[layer].chn_ctx[chn].is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) isn't enabled yet.\n", layer, chn);
		return ERR_VO_CHN_NOT_ENABLED;
	}

	return 0;
}

static inline int check_vo_null_ptr(mod_id_e mod, const void *ptr)
{
	if (mod >= ID_BUTT)
		return -1;

	if (!ptr) {
		TRACE_VO(DBG_ERR, "NULL pointer\n");
		return ERR_VO_NULL_PTR;
	}
	return 0;
}

static inline int check_vo_wbc_valid(vo_wbc wbc_dev)
{
	if (wbc_dev >= VO_MAX_WBC_NUM || wbc_dev < 0) {
		TRACE_VO(DBG_ERR, "VoWbc(%d) invalid.\n", wbc_dev);
		return ERR_VO_INVALID_WBCID;
	}

	return 0;
}

long vo_sdk_ctrl(struct vo_core_dev *vdev, struct vo_ext_control *p);
int vo_disable(vo_dev dev);
int vo_disablevideolayer(vo_layer layer);
int vo_disable_chn(vo_layer layer, vo_chn chn);
int vo_disable_wbc(vo_wbc wbc_dev);
int vo_get_chnrotation(vo_layer layer, vo_chn chn, rotation_e *rotation);
int vo_wbc_qbuf(struct vo_wbc_ctx *wbc_ctx);
struct vo_fmt *vo_sdk_get_format(u32 pixelformat);

#ifdef __cplusplus
}
#endif

#endif //__VO_SDK_LAYER_H__

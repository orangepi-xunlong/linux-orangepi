/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui 
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/*plane --> Layer*/

#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include "sunxi_drm_plane.h"
#include <video/sunxi_display2.h>
#include <drm/sunxi_drm.h>
#include <asm-generic/dma-mapping-common.h>


#include "sunxi_drm_drv.h"
#include "sunxi_drm_core.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_plane.h"
#include "drm_de/drm_al.h"

#include "subdev/sunxi_common.h"
#include "sunxi_drm_gem.h"
#include "sunxi_drm_plane.h"
#include "sunxi_drm_fb.h"

struct pixel_info_t {
	enum disp_pixel_format format;
	unsigned char plane;
	unsigned char plane_bpp[3];
	unsigned char wscale[3];
	unsigned char hscale[3];
	bool    swap_uv;
};

static const uint32_t ui_formats[] = {
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,

	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGBA8888,
	DRM_FORMAT_BGRA8888,

	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBX8888,
	DRM_FORMAT_BGRX8888,

	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,

	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_ABGR4444,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_BGRA4444,

	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_ABGR1555,
	DRM_FORMAT_RGBA5551,
	DRM_FORMAT_BGRA5551,
};

static const uint32_t vi_formats[] = {
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,

	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGBA8888,
	DRM_FORMAT_BGRA8888,

	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBX8888,
	DRM_FORMAT_BGRX8888,

	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,

	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_ABGR4444,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_BGRA4444,

	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_ABGR1555,
	DRM_FORMAT_RGBA5551,
	DRM_FORMAT_BGRA5551,

	DRM_FORMAT_AYUV,
	DRM_FORMAT_YUV444,
	DRM_FORMAT_YUV422,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YUV411,

	DRM_FORMAT_NV61,
	DRM_FORMAT_NV16,
	DRM_FORMAT_NV21,
	DRM_FORMAT_NV12,

};

uint32_t disp_to_drm_format(enum disp_pixel_format format)
{
	switch (format) {
	case DISP_FORMAT_ARGB_8888:
		return DRM_FORMAT_ARGB8888;

	case DISP_FORMAT_ABGR_8888:
		return DRM_FORMAT_ABGR8888;

	case DISP_FORMAT_RGBA_8888:
		return DRM_FORMAT_RGBA8888;

	case DISP_FORMAT_BGRA_8888:
		return DRM_FORMAT_BGRA8888;

	case DISP_FORMAT_XRGB_8888:
		return DRM_FORMAT_XRGB8888;

	case DISP_FORMAT_XBGR_8888:
		return DRM_FORMAT_XBGR8888;

	case DISP_FORMAT_RGBX_8888:
		return DRM_FORMAT_RGBX8888;

	case DISP_FORMAT_BGRX_8888:
		return DRM_FORMAT_BGRX8888;

	case DISP_FORMAT_RGB_888:
		return DRM_FORMAT_RGB888;

	case DISP_FORMAT_BGR_888:
		return DRM_FORMAT_BGR888;

	case DISP_FORMAT_RGB_565:
		return DRM_FORMAT_RGB565;

	case DISP_FORMAT_BGR_565:
		return DRM_FORMAT_BGR565;

	case DISP_FORMAT_ARGB_4444:
		return DRM_FORMAT_ARGB4444;

	case DISP_FORMAT_ABGR_4444:
		return DRM_FORMAT_ABGR4444;

	case DISP_FORMAT_RGBA_4444:
		return DRM_FORMAT_RGBA4444;

	case DISP_FORMAT_BGRA_4444:
		return DRM_FORMAT_BGRA4444;

	case DISP_FORMAT_ARGB_1555:
		return DRM_FORMAT_ARGB1555;

	case DISP_FORMAT_ABGR_1555:
		return DRM_FORMAT_ABGR1555;

	case DISP_FORMAT_RGBA_5551:
		return DRM_FORMAT_RGBA5551;

	case DISP_FORMAT_BGRA_5551:
		return DRM_FORMAT_BGRA5551;

	/* SP: semi-planar, P:planar, I:interleaved
	* UVUV: U in the LSBs;     VUVU: V in the LSBs */

	case DISP_FORMAT_YUV444_I_AYUV:
		return DRM_FORMAT_AYUV;

	case DISP_FORMAT_YUV444_I_VUYA:
		return 0;

	case DISP_FORMAT_YUV422_I_YVYU:
		return 0;

	case DISP_FORMAT_YUV422_I_YUYV:
		return 0;

	case DISP_FORMAT_YUV422_I_UYVY:
		return 0;

	case DISP_FORMAT_YUV422_I_VYUY:
		return 0;

	case DISP_FORMAT_YUV444_P:
		return DRM_FORMAT_YUV444;

	case DISP_FORMAT_YUV422_P:
		return DRM_FORMAT_YUV422;

	case DISP_FORMAT_YUV420_P:
		return DRM_FORMAT_YUV420;

	case DISP_FORMAT_YUV411_P:
		return DRM_FORMAT_YUV411;

	case DISP_FORMAT_YUV422_SP_UVUV:
		return DRM_FORMAT_NV61;

	case DISP_FORMAT_YUV422_SP_VUVU:
		return DRM_FORMAT_NV16;

	case DISP_FORMAT_YUV420_SP_UVUV:
		return DRM_FORMAT_NV21;

	case DISP_FORMAT_YUV420_SP_VUVU:
		return DRM_FORMAT_NV12;

	case DISP_FORMAT_YUV411_SP_UVUV:
		return 0;

	case DISP_FORMAT_YUV411_SP_VUVU:
		return 0;

	default:
		DRM_ERROR("get a err display format.\n");
	}
	return 0;
}

bool drm_to_disp_format(struct pixel_info_t *pixel_info, uint32_t format)
{
	pixel_info->plane = 1;
	pixel_info->wscale[0] = 1;
	pixel_info->hscale[0] = 1;
	pixel_info->plane_bpp[0] = 32;
	pixel_info->swap_uv = 0;
	/*  */
	switch (format) {
	case DRM_FORMAT_ARGB8888:
		pixel_info->format = DISP_FORMAT_ARGB_8888;
		return true;

	case DRM_FORMAT_ABGR8888:
		pixel_info->format = DISP_FORMAT_ABGR_8888;
		return true;

	case DRM_FORMAT_RGBA8888:
		pixel_info->format = DISP_FORMAT_RGBA_8888;
		return true;

	case DRM_FORMAT_BGRA8888:
		pixel_info->format = DISP_FORMAT_BGRA_8888;
		return true;

	case DRM_FORMAT_XRGB8888:
		pixel_info->format = DISP_FORMAT_XRGB_8888;
		return true;

	case DRM_FORMAT_XBGR8888:
		pixel_info->format = DISP_FORMAT_XBGR_8888;
		return true;

	case DRM_FORMAT_RGBX8888:
		pixel_info->format = DISP_FORMAT_RGBX_8888;
		return true;

	case DRM_FORMAT_BGRX8888:
		pixel_info->format = DISP_FORMAT_BGRX_8888;
		return true;

	case DRM_FORMAT_RGB888:
		pixel_info->format = DISP_FORMAT_RGB_888;
		pixel_info->plane_bpp[0] = 24;
		return true;

	case DISP_FORMAT_BGR_888:
		pixel_info->format = DRM_FORMAT_BGR888;
		pixel_info->plane_bpp[0] = 24;
		return true;

	case DRM_FORMAT_RGB565:
		pixel_info->format = DISP_FORMAT_RGB_565;
		pixel_info->plane_bpp[0] = 16;
		return true;

	case DRM_FORMAT_BGR565:
		pixel_info->format = DISP_FORMAT_BGR_565;
		pixel_info->plane_bpp[0] = 16;
		return true;

	case DRM_FORMAT_ARGB4444:
		pixel_info->format = DISP_FORMAT_ARGB_4444;
		return true;

	case DRM_FORMAT_ABGR4444:
		pixel_info->format = DISP_FORMAT_ABGR_4444;
		pixel_info->plane_bpp[0] = 16;
		return true;

	case DRM_FORMAT_RGBA4444:
		pixel_info->format = DISP_FORMAT_RGBA_4444;
		pixel_info->plane_bpp[0] = 16;
		return true;

	case DRM_FORMAT_BGRA4444:
		pixel_info->format = DISP_FORMAT_BGRA_4444;
		pixel_info->plane_bpp[0] = 16;
		return true;

	case DRM_FORMAT_ARGB1555:
		pixel_info->format = DISP_FORMAT_ARGB_1555;
		pixel_info->plane_bpp[0] = 16;
		return true;

	case DRM_FORMAT_ABGR1555:
		pixel_info->format = DISP_FORMAT_ABGR_1555;
		pixel_info->plane_bpp[0] = 16;
		return true;

	case DRM_FORMAT_RGBA5551:
		pixel_info->format = DISP_FORMAT_RGBA_5551;
		pixel_info->plane_bpp[0] = 16;
		return true;

	case DRM_FORMAT_BGRA5551:
		pixel_info->format = DISP_FORMAT_BGRA_5551;
		pixel_info->plane_bpp[0] = 16;
		return true;

	/* SP: semi-planar, P:planar, I:interleaved
	* UVUV: U in the LSBs;     VUVU: V in the LSBs */

	case DRM_FORMAT_AYUV:
		pixel_info->format = DISP_FORMAT_YUV444_I_AYUV;
		pixel_info->plane_bpp[0] = 32;
		return true;

	case DRM_FORMAT_YUV444:
		pixel_info->format = DISP_FORMAT_YUV444_P;
		pixel_info->plane = 3;
		pixel_info->wscale[1] = 1;
		pixel_info->hscale[1] = 1;
		pixel_info->wscale[2] = 1;
		pixel_info->hscale[2] = 1;
		pixel_info->plane_bpp[0] = 8;
		pixel_info->plane_bpp[1] = 8;
		pixel_info->plane_bpp[2] = 8;
		return true;

	case DRM_FORMAT_YUV422:
		pixel_info->format = DISP_FORMAT_YUV422_P;
		pixel_info->plane = 3;
		pixel_info->wscale[1] = 2;
		pixel_info->hscale[1] = 1;
		pixel_info->wscale[2] = 2;
		pixel_info->hscale[2] = 1;
		pixel_info->plane_bpp[0] = 8;
		pixel_info->plane_bpp[1] = 8;
		pixel_info->plane_bpp[2] = 8;
		return true;

	case DRM_FORMAT_YUV420:
		pixel_info->format = DISP_FORMAT_YUV420_P;
		pixel_info->plane = 3;
		pixel_info->wscale[1] = 2;
		pixel_info->hscale[1] = 2;
		pixel_info->wscale[2] = 2;
		pixel_info->hscale[2] = 2;
		pixel_info->plane_bpp[0] = 8;
		pixel_info->plane_bpp[1] = 8;
		pixel_info->plane_bpp[2] = 8;
		return true;

	case DRM_FORMAT_YUV411:
		pixel_info->format = DISP_FORMAT_YUV411_P;
		pixel_info->plane = 3;
		pixel_info->wscale[1] = 4;
		pixel_info->hscale[1] = 1;
		pixel_info->wscale[2] = 4;
		pixel_info->hscale[2] = 1;
		pixel_info->plane_bpp[0] = 8;
		pixel_info->plane_bpp[1] = 8;
		pixel_info->plane_bpp[2] = 8;
		return true;

	case DRM_FORMAT_NV61:
		pixel_info->format = DISP_FORMAT_YUV422_SP_UVUV;
		pixel_info->plane = 2;
		pixel_info->wscale[1] = 2;
		pixel_info->hscale[1] = 1;
		pixel_info->plane_bpp[0] = 8;
		pixel_info->plane_bpp[1] = 16;
		return true;

	case DRM_FORMAT_NV16:
		pixel_info->format = DISP_FORMAT_YUV422_SP_VUVU;
		pixel_info->plane = 2;
		pixel_info->wscale[1] = 2;
		pixel_info->hscale[1] = 1;
		pixel_info->plane_bpp[0] = 8;
		pixel_info->plane_bpp[1] = 16;
		return true;

	case DRM_FORMAT_NV21:
		pixel_info->format = DISP_FORMAT_YUV420_SP_UVUV;
		pixel_info->plane = 2;
		pixel_info->wscale[1] = 2;
		pixel_info->hscale[1] = 2;
		pixel_info->plane_bpp[0] = 8;
		pixel_info->plane_bpp[1] = 16;
		return true;

	case DRM_FORMAT_NV12:
		pixel_info->format = DISP_FORMAT_YUV420_SP_VUVU;
		pixel_info->plane = 2;
		pixel_info->wscale[1] = 2;
		pixel_info->hscale[1] = 2;
		pixel_info->plane_bpp[0] = 8;
		pixel_info->plane_bpp[1] = 16;
		return true;

	default:
		DRM_ERROR("get a err drm format.\n");
	}
	return false;
}

static struct drm_plane *sunxi_find_fb_plane(struct drm_crtc *crtc)
{
	struct sunxi_drm_crtc  *sunxi_crtc = to_sunxi_crtc(crtc);
	struct drm_plane *plane = NULL;
	struct sunxi_drm_plane *ui_plane, *fb_plane;
	int video_chn, i;

	video_chn = sunxi_drm_get_vi_pipe_by_crtc(sunxi_crtc->crtc_id);
	for (i = 0; i < sunxi_crtc->plane_of_de; i++) {
		plane = sunxi_crtc->plane_array[i];
		ui_plane = to_sunxi_plane(plane);
		if(ui_plane->chn_id > (video_chn - 1)) {
			break;
		}
		ui_plane = NULL;
	}

	if (crtc->fb) {
		switch (crtc->fb->pixel_format) {
		case DRM_FORMAT_RGB888:
		case DRM_FORMAT_BGR888:
		case DRM_FORMAT_ARGB8888:
		case DRM_FORMAT_ABGR8888:
		case DRM_FORMAT_RGBA8888:
		case DRM_FORMAT_BGRA8888:
		case DRM_FORMAT_XRGB8888:
		case DRM_FORMAT_XBGR8888:
		case DRM_FORMAT_RGBX8888:
		case DRM_FORMAT_BGRX8888:
		case DRM_FORMAT_RGB565:
		case DRM_FORMAT_BGR565:
		case DRM_FORMAT_ARGB4444:
		case DRM_FORMAT_ABGR4444:
		case DRM_FORMAT_RGBA4444:
		case DRM_FORMAT_BGRA4444:
		case DRM_FORMAT_ARGB1555:
		case DRM_FORMAT_ABGR1555:
		case DRM_FORMAT_RGBA5551:
		case DRM_FORMAT_BGRA5551:
			if(sunxi_crtc->fb_plane) {
				fb_plane = to_sunxi_plane(sunxi_crtc->fb_plane);
				if(fb_plane->chn_id >= video_chn) {
					return sunxi_crtc->fb_plane;
				}
			}
			if(ui_plane)
				return &ui_plane->drm_plane;
			break;

		case DRM_FORMAT_AYUV:
		case DRM_FORMAT_YUV444:
		case DRM_FORMAT_YUV422:
		case DRM_FORMAT_YUV420:
		case DRM_FORMAT_YUV411:

		case DRM_FORMAT_NV61:
		case DRM_FORMAT_NV16:
		case DRM_FORMAT_NV21:
		case DRM_FORMAT_NV12:
			if(video_chn != 0)
				return sunxi_crtc->plane_array[0];
			break;
		default:
			DRM_ERROR("get fb_plane got a error fb format.\n");
		}
	}else {
		if (sunxi_crtc->fb_plane) {
			return sunxi_crtc->fb_plane;
		}
		if (ui_plane) {
			return &ui_plane->drm_plane;
		}
		DRM_ERROR("must init err,there is not ui_plane.\n");
	}
	DRM_ERROR("must  err,there is not fb_plane return.\n");
	return NULL;
}

/*
 * sunxi_update_plane()
 * Only  update the args to the register,
 * but no use it for the next frame(vsync).
 * so you must call sunxi_plane_commit() to 
 * validate the register.
 */
static int sunxi_plane_mode_set(struct disp_layer_config_data *cfg, struct drm_crtc *crtc,
		struct drm_framebuffer *fb, int crtc_x, int crtc_y,
		unsigned int crtc_w, unsigned int crtc_h,
		uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h)
{
	int  i, j;
	bool buf_packed = 0;
	struct sunxi_drm_framebuffer *sunxi_fb = to_sunxi_fb(fb);
	struct sunxi_drm_gem_buf *buffer;
	struct drm_gem_object *gem_obj;
	struct pixel_info_t pixel_info;
	struct disp_layer_config *config = &cfg->config;

	if (!drm_to_disp_format(&pixel_info, fb->pixel_format)) {
		DRM_ERROR("got a bad pixel format.\n");
		return -EINVAL;
	}

	if (sunxi_fb->nr_gem > pixel_info.plane ||
		sunxi_fb->nr_gem > 3) {
		DRM_ERROR("have too manny gem for plane.\n");
		return -EINVAL;
	}

	if (pixel_info.plane > sunxi_fb->nr_gem) {
		buf_packed = 1;
	}

	for (i = 0, j = 0; i < MAX_FB_BUFFER; i++) {
		gem_obj = sunxi_fb->gem_obj[i];
		if (gem_obj) {
			buffer = (struct sunxi_drm_gem_buf *)gem_obj->driver_private;
			if (!buffer)
				continue;
			if (buffer->flags & SUNXI_BO_CACHABLE) {
				/* SUNXI_BO_CACHABLE must dma_sync_sg_for_device , becareful*/
				DRM_DEBUG_KMS("plane set dma_addr(0x%lx)(%lx), size(%lu)\n",
						(unsigned long)buffer->dma_addr, (unsigned long)buffer->kvaddr,
						buffer->size);

				sunxi_sync_buf(buffer);
			}

			if (!(buffer->flags & SUNXI_BO_CONTIG)) {
				memset(config, 0, sizeof(*config));
				cfg->flag |= BLEND_ENABLE_DIRTY;
				DRM_ERROR("no_contig mem.\n");
				return -EINVAL;
			}
			/* mem offset */
			config->info.fb.addr[j++] = buffer->dma_addr + fb->offsets[i];
		}
	}

	config->info.fb.format = pixel_info.format;
	config->enable = 1;
	config->info.screen_win.x = crtc_x;
	config->info.screen_win.y = crtc_y;
	config->info.screen_win.width = crtc_w;
	config->info.screen_win.height = crtc_h;
	/* 3D mode set in property */
	config->info.alpha_mode = 1;
	config->info.alpha_value = 0xff;
	/* screen crop set */
	config->info.fb.crop.x = ((long long)src_x)<<32;
	config->info.fb.crop.y = ((long long)src_y)<<32;
	config->info.fb.crop.height = ((long long)src_h)<<32;
	config->info.fb.crop.width = ((long long)src_w)<<32;
	/* set in property  TODO*/
	config->info.fb.pre_multiply = 0;
	/* fb stride/bpp , is pixel */
	if (buf_packed) {
		config->info.fb.size[0].width = fb->pitches[0] /
						(fb->bits_per_pixel>>3) /
						pixel_info.wscale[0];
	}else {
		config->info.fb.size[0].width = fb->pitches[0] /
						(pixel_info.plane_bpp[0]>>3) /
						pixel_info.wscale[0];
	}

	config->info.fb.size[0].height = fb->height / pixel_info.hscale[0];

	config->info.fb.align[0] = 0;

	for (i = 1; i < pixel_info.plane; i++) {
		/* set align will in  property for GPU / g2d / video,
		* all use the plane , currently set it use fb.pitches, 
		* we control it by fb.pitch which user set, so  set it 0.
		*/
		config->info.fb.align[i] = 0;
		config->info.fb.size[i].width =
		config->info.fb.size[0].width / pixel_info.wscale[i];
		config->info.fb.size[i].height =
		config->info.fb.size[0].height / pixel_info.hscale[i];

		if (buf_packed) {
			config->info.fb.addr[i] =
			config->info.fb.size[i-1].width * config->info.fb.size[i-1].height;
		}
	}

	if (pixel_info.swap_uv) {
		unsigned long long  swap_addr;
		swap_addr = config->info.fb.addr[1];
		config->info.fb.addr[1] = config->info.fb.addr[2];
		config->info.fb.addr[2] = swap_addr;
	}
	DRM_DEBUG_KMS("plane cfg:"
			"crt_id:%d\nchn[%d] lyr[%d] z[%d] from[%lld, %lld, %lld, %lld]"
			" to [%d, %d, %d, %d] pitch[%d,%d] addr:0x%llx (%d)\n",
	crtc->base.id,
	config->channel,
	config->layer_id,
	config->info.zorder,
	config->info.fb.crop.x>>32,
	config->info.fb.crop.y>>32,
	config->info.fb.crop.width>>32,
	config->info.fb.crop.height>>32,
	config->info.screen_win.x,
	config->info.screen_win.y,
	config->info.screen_win.width,
	config->info.screen_win.height,
	config->info.fb.size[0].width,
	config->info.fb.size[0].height,
	config->info.fb.addr[0],
	config->info.fb.format);
	cfg->flag |= LAYER_ALL_DIRTY;

	return 0;
}

int sunxi_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
		     struct drm_framebuffer *fb, int crtc_x, int crtc_y,
		     unsigned int crtc_w, unsigned int crtc_h,
		     uint32_t src_x, uint32_t src_y,
		     uint32_t src_w, uint32_t src_h)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct disp_layer_config_data *cfg;
	struct sunxi_hardware_ops *plane_ops = sunxi_plane->hw_res->ops;
	int ret = 0;

	DRM_DEBUG_KMS("[%d]crtc_id:%d\n", __LINE__,crtc->base.id);

	cfg = sunxi_plane->plane_cfg;
	/* TODO for 3D */
	ret = sunxi_plane_mode_set(cfg, crtc, fb, crtc_x, crtc_y,
				crtc_w, crtc_h, src_x >> 16, src_y >> 16,
				src_w >> 16, src_h >> 16);
	if (ret) {
		DRM_ERROR("updata_plane err.\n");
		return -EINVAL; 
	}
	/* protect the current layer to vsync period */
	mutex_lock(&sunxi_plane->delayed_work_lock);
	if (sunxi_plane->old_fb) {
		/* for many update */
		drm_framebuffer_unreference(sunxi_plane->old_fb);
	}
	sunxi_plane->old_fb = plane->fb;
	mutex_unlock(&sunxi_plane->delayed_work_lock);

	drm_framebuffer_reference(fb);

	cfg = sunxi_plane->plane_cfg;
	cfg->config.enable = 1;
	cfg->flag |= LAYER_ALL_DIRTY;
	if (plane_ops->updata_reg != NULL) {
		if (!plane_ops->updata_reg(cfg)) {
			return -EINVAL;
		}
	}

	return 0;
}

void sunxi_plane_dpms(struct drm_plane *plane, int status)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct disp_layer_config_data *cfg;
	struct sunxi_hardware_ops *plane_ops = sunxi_plane->hw_res->ops;
	bool changed = 0;

	cfg = sunxi_plane->plane_cfg;
	DRM_DEBUG_KMS("[%d] status[%d]\n", __LINE__, status);

	switch(status){
		case  DRM_MODE_DPMS_ON:
			if (cfg->config.enable == 0) {
				cfg->config.enable = 1;
				cfg->flag = BLEND_ENABLE_DIRTY;
			}
			break;
		case  DRM_MODE_DPMS_STANDBY:
		case  DRM_MODE_DPMS_SUSPEND:
		case  DRM_MODE_DPMS_OFF:
			if (cfg->config.enable == 1) {
				cfg->config.enable = 0;
				cfg->flag = BLEND_ENABLE_DIRTY;
				changed = 1;
			}
			if (changed && plane_ops->disable != NULL) {
				plane_ops->disable(cfg);
			}
			break;
		default:
		DRM_ERROR("sunxi_plane_dpms error arg.\n");
	}
}

static int sunxi_disable_plane(struct drm_plane *plane)
{
	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	sunxi_plane_dpms(plane, DRM_MODE_DPMS_OFF);
	return 0;
}

static void sunxi_plane_destroy(struct drm_plane *plane)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	sunxi_disable_plane(plane);
	drm_plane_cleanup(plane);
	kfree(sunxi_plane);
}

static int sunxi_plane_set_property(struct drm_plane *plane,
				     struct drm_property *property,
				     uint64_t val)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(plane);
	struct sunxi_drm_crtc  *sunxi_crtc = NULL;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	sunxi_crtc = get_sunxi_crt(plane->dev, sunxi_plane->crtc_id);
	if(!sunxi_crtc) {
		DRM_ERROR("failed to get the sunxi_crtc.\n");
		goto err;
	}

	if (property == sunxi_crtc->plane_zpos_property) {
		sunxi_plane->plane_cfg->config.info.zorder = val;
		return 0;
	}
err:
	return -EINVAL;
}

static struct drm_plane_funcs sunxi_plane_funcs = {
	.update_plane	= sunxi_update_plane,
	.disable_plane	= sunxi_disable_plane,
	.destroy	= sunxi_plane_destroy,
	.set_property	= sunxi_plane_set_property,
};

/* 
 * if the DE BSP modify the set de_al_lyr_apply()func 1 layer
 * or 1 channel set mode, will modify the plane_ops
 * updata_reg and disable functions.  
 */
void sunxi_plane_delayed(void *data)
{
	struct sunxi_drm_plane *plane = to_sunxi_plane(data);
	/* protect the current layer to vsync period
	* can only run one times when n vsync occure.
	*/
	mutex_lock(&plane->delayed_work_lock);
	if (plane->old_fb) {
		drm_framebuffer_unreference(plane->old_fb);
		plane->old_fb = NULL;
	}
	mutex_unlock(&plane->delayed_work_lock);
}

bool sunxi_plane_reset(void *data)
{
	struct drm_plane *plane = (struct drm_plane *)data;
	sunxi_plane_dpms(plane, DRM_MODE_DPMS_OFF);
	return true;
}

bool sunxi_plane_updata_reg(void *data)
{
	struct sunxi_drm_plane *sunxi_plane = to_sunxi_plane(data);
	struct sunxi_drm_crtc *sunxi_crtc;
	bool    changed = 0;

	sunxi_crtc = get_sunxi_crt(sunxi_plane->drm_plane.dev,
			sunxi_plane->crtc_id);
	if (!sunxi_crtc) {
		DRM_ERROR("failed get sunxi crt.\n");
		return false;
	}

	if (!sunxi_crtc->user_update_frame) {
		spin_lock(&sunxi_crtc->update_reg_lock);
		sunxi_crtc->user_update_frame = 1;
		spin_lock(&sunxi_crtc->update_reg_lock);
		changed = 1;
	}

	if (changed) {
		/* waite for the update reg ready of double buffer */
		if (sunxi_crtc->ker_update_frame) {
			sunxi_plane->delayed_updata = 1;
			sunxi_crtc->has_delayed_updata = 1;
			return true;
		}
	}
	/* do update the reg */

	return true;
}

struct sunxi_hardware_ops plane_ops = {
	.reset          = NULL,
	.enable         = NULL,
	.disable        = NULL,
	.updata_reg     = NULL,
	.vsync_proc   = NULL,
	.vsync_delayed_do = sunxi_plane_delayed,
};

static const struct drm_prop_enum_list layer_mode_names[] = {
	{ CRTC_MODE_3D,     "normal_mode" },
	{ CRTC_MODE_ENHANCE, "enhance" },
	{ CRTC_MODE_ENHANCE_HF, "enhance_half" },
	{ CRTC_MODE_SMART_LIGHT, "smart_light" },//ToDo
};

void sunxi_reset_plane(struct drm_crtc *crtc)
{
	int i;
	struct sunxi_drm_crtc  *sunxi_crtc = to_sunxi_crtc(crtc);

	for (i = 0; i < sunxi_crtc->plane_of_de; i++) {
		sunxi_plane_reset(sunxi_crtc->plane_array[i]);
	}
}

struct drm_plane *sunxi_plane_init(struct drm_device *dev,struct drm_crtc *crtc,
            struct disp_layer_config_data *cfg, int chn, int plane_id, bool priv)
{
	struct sunxi_drm_plane  *sunxi_plane;
	struct drm_plane	    *plane;
	struct sunxi_drm_crtc   *sunxi_crtc = to_sunxi_crtc(crtc);
	bool video = 0;
	int vi_chn;

	int err;
	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	sunxi_plane = kzalloc(sizeof(struct sunxi_drm_plane), GFP_KERNEL);
	if (!sunxi_plane) {
		DRM_ERROR("failed to allocate plane\n");
		return NULL;
	}

	vi_chn = sunxi_drm_get_vi_pipe_by_crtc(sunxi_crtc->crtc_id);
	if (vi_chn > chn)
		video = 1;

	err = drm_plane_init(dev, &sunxi_plane->drm_plane, 1 << sunxi_crtc->crtc_id,
				&sunxi_plane_funcs, video ? vi_formats : ui_formats,
				video ? ARRAY_SIZE(vi_formats) : ARRAY_SIZE(ui_formats),
				priv);
	if (err) {
		DRM_ERROR("failed to initialize plane\n");
		kfree(sunxi_plane);
		return NULL;
	}

	sunxi_plane->hw_res = kzalloc(sizeof(struct sunxi_hardware_res), GFP_KERNEL);
	if (!sunxi_plane->hw_res) {
		DRM_ERROR("failed to allocate sunxi_hardware_res\n");
		kfree(sunxi_plane);
		return NULL;
	}

	sunxi_plane->hw_res->ops = &plane_ops;
	sunxi_plane->chn_id = chn;
	sunxi_plane->isvideo = video;
	sunxi_plane->crtc_id = sunxi_crtc->crtc_id;
	sunxi_plane->plane_cfg = cfg;
	cfg->config.channel = chn;
	cfg->config.layer_id = plane_id;
	mutex_init(&sunxi_plane->delayed_work_lock);
	plane = &sunxi_plane->drm_plane;
	if (priv) {
	/* for the Hardware Cursor if have */
		sunxi_crtc->harware_cursor = &sunxi_plane->drm_plane;
	}else{
		drm_object_attach_property(&plane->base, sunxi_crtc->channel_id_property, chn);
		drm_object_attach_property(&plane->base, sunxi_crtc->plane_zpos_property, 0);
		drm_object_attach_property(&plane->base, sunxi_crtc->plane_id_chn_property, plane_id);
	}

	return &sunxi_plane->drm_plane;
}

int sunxi_set_fb_plane(struct drm_crtc *crtc, unsigned int plane_id, unsigned int zoder)
{
	struct sunxi_drm_plane *sunxi_plane;
	struct drm_mode_object *obj;
	struct drm_plane *plane = NULL;
	struct sunxi_drm_crtc *sunxi_crtc = to_sunxi_crtc(crtc);

	obj = drm_mode_object_find(crtc->dev, plane_id, DRM_MODE_OBJECT_PLANE);
	if (!obj) {
		plane = sunxi_find_fb_plane(crtc);
	}else{
		plane = obj_to_plane(obj);
	}

	DRM_DEBUG_KMS("setfb: crtc_id[%d] plane[%d]\n", crtc->base.id, plane->base.id);

	if (plane != NULL) {
		sunxi_plane = to_sunxi_plane(plane);
		if (sunxi_plane->crtc_id == sunxi_crtc->crtc_id) {
			sunxi_crtc->fb_plane = plane;
			if (sunxi_crtc->plane_of_de > zoder)
				sunxi_plane->plane_cfg->config.info.zorder = zoder;
			else
				/* default FB zorder or 0~sunxi_crtc->plane_of_de-1 */
				sunxi_plane->plane_cfg->config.info.zorder = 0;
			return 0;
		}
	} else {
		if (sunxi_plane->crtc_id == sunxi_crtc->crtc_id)
			sunxi_crtc->fb_plane = NULL;
		return 0;
	}
	return -EINVAL;
}

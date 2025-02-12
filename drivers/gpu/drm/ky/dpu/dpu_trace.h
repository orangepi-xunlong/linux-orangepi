// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM dpu

#if !defined(_DPU_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _DPU_TRACE_H_

#include <linux/stringify.h>
#include <linux/types.h>
#include <linux/tracepoint.h>

#include "dpu_saturn.h"

TRACE_EVENT(drm_display_mode_info,
	TP_PROTO(struct drm_display_mode *mode),
	TP_ARGS(mode),
	TP_STRUCT__entry(
		__field(u16, hdisplay)
		__field(u16, hbp)
		__field(u16, hfp)
		__field(u16, hsync)
		__field(u16, vdisplay)
		__field(u16, vbp)
		__field(u16, vfp)
		__field(u16, vsync)
	),
	TP_fast_assign(
		__entry->hdisplay = mode->hdisplay;
		__entry->hbp = mode->htotal - mode->hsync_end;
		__entry->hfp = mode->hsync_start - mode->hdisplay;
		__entry->hsync = mode->hsync_end - mode->hsync_start;
		__entry->vdisplay = mode->vdisplay;
		__entry->vbp = mode->vtotal - mode->vsync_end;
		__entry->vfp = mode->vsync_start - mode->vdisplay;
		__entry->vsync = mode->vsync_end - mode->vsync_start;
	),
	TP_printk("drm_display_mode: hdisplay=%d vdisplay=%d hsync=%d vsync=%d hbp=%d hfp=%d vbp=%d vfp=%d",
		  __entry->hdisplay, __entry->vdisplay,  __entry->hsync, __entry->vsync,
		  __entry->hbp, __entry->hfp,  __entry->vbp, __entry->vfp
	)
);

TRACE_EVENT(dpu_plane_info,
	TP_PROTO(struct drm_plane_state *state, struct drm_framebuffer *fb,
		u32 rdma_id, u32 alpha, u32 rotation, u32 afbc),
	TP_ARGS(state, fb, rdma_id, alpha, rotation, afbc),
	TP_STRUCT__entry(
			__field(u32, rdma_id)
			__field(u32, src_w)
			__field(u32, src_h)
			__field(u32, src_x)
			__field(u32, src_y)
			__field(u32, crtc_w)
			__field(u32, crtc_h)
			__field(u32, crtc_x)
			__field(u32, crtc_y)
			__field(u32, width)
			__field(u32, height)
			__field(u32, format)
			__field(u32, blend_mode)
			__field(u32, alpha)
			__field(u32, zpos)
			__field(u32, rotation)
			__field(u32, afbc)
	),
	TP_fast_assign(
			__entry->rdma_id = rdma_id;
			__entry->src_w = state->src_w >> 16;
			__entry->src_h = state->src_h >> 16;
			__entry->src_x = state->src_x >> 16;
			__entry->src_y = state->src_y >> 16;
			__entry->crtc_w = state->crtc_w;
			__entry->crtc_h = state->crtc_h;
			__entry->crtc_x = state->crtc_x;
			__entry->crtc_y = state->crtc_y;
			__entry->width = fb->width;
			__entry->height = fb->height;
			__entry->format = fb->format->format;
			__entry->blend_mode = state->pixel_blend_mode;
			__entry->alpha = alpha;
			__entry->zpos = state->zpos;
			__entry->rotation = rotation;
			__entry->afbc = afbc;
	),
	TP_printk("rdma_id=%d src: w=%d h=%d x=%d y=%d crtc: w=%d h=%d x=%d y=%d "
		  "width=%d height=%d fmt=0x%x blend=%d alpha:%d "
		  "zpos=%d rot:%d afbc:%d",
		  __entry->rdma_id, __entry->src_w,
		  __entry->src_h, __entry->src_x,
		  __entry->src_y, __entry->crtc_w,
		  __entry->crtc_h, __entry->crtc_x,
		  __entry->crtc_y, __entry->width,
		  __entry->height, __entry->format,
		  __entry->blend_mode, __entry->alpha,
		  __entry->zpos, __entry->rotation,
		  __entry->afbc)
);

DECLARE_EVENT_CLASS(dpu_status_template,
	TP_PROTO(const char *name, int status),
	TP_ARGS(name, status),
	TP_STRUCT__entry(
		__string(name_str, name)
		__field(int, status)
	),
	TP_fast_assign(
		__assign_str(name_str, name);
		__entry->status = status;
	),
	TP_printk("%s: 0x%x",
		  __get_str(name_str), __entry->status)
);

DEFINE_EVENT(dpu_status_template, dpuctrl_scaling,
	TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);
DEFINE_EVENT(dpu_status_template, dpuctrl,
	TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);
DEFINE_EVENT(dpu_status_template, ky_plane_update_hw_channel,
	TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);
DEFINE_EVENT(dpu_status_template, saturn_irq_enable,
	TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);
DEFINE_EVENT(dpu_status_template, saturn_enable_vsync,
	TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);
DEFINE_EVENT(dpu_status_template, dpu_isr_status,
	TP_PROTO(const char *name, int status),
	TP_ARGS(name, status)
);

DECLARE_EVENT_CLASS(dpu_ul_data_template,
	TP_PROTO(const char *name, unsigned long ul_data),
	TP_ARGS(name, ul_data),
	TP_STRUCT__entry(
		__string(name_str, name)
		__field(unsigned long, ul_data)
	),
	TP_fast_assign(
		__assign_str(name_str, name);
		__entry->ul_data = ul_data;
	),
	TP_printk("%s: %ld",
		  __get_str(name_str), __entry->ul_data)
);

DEFINE_EVENT(dpu_ul_data_template, dpu_isr_ul_data,
	TP_PROTO(const char *name, unsigned long ul_data),
	TP_ARGS(name, ul_data)
);

DECLARE_EVENT_CLASS(dpu_uint64_t_data_template,
	TP_PROTO(const char *name, uint64_t data),
	TP_ARGS(name, data),
	TP_STRUCT__entry(
		__string(name_str, name)
		__field(uint64_t, data)
	),
	TP_fast_assign(
		__assign_str(name_str, name);
		__entry->data = data;
	),
	TP_printk("%s: %lld",
		  __get_str(name_str), __entry->data)
);

DEFINE_EVENT(dpu_uint64_t_data_template, u64_data,
        TP_PROTO(const char *name, uint64_t data),
        TP_ARGS(name, data)
);

TRACE_EVENT(ky_plane_disable_hw_channel,
	TP_PROTO(uint32_t layer_id, uint32_t rdma_id),
	TP_ARGS(layer_id, rdma_id),
	TP_STRUCT__entry(
		__field(uint32_t, layer_id)
		__field(uint32_t, rdma_id)
	),
	TP_fast_assign(
		__entry->layer_id = layer_id;
		__entry->rdma_id = rdma_id;
	),
	TP_printk("layer_id:%d rdma_id:%d",
		  __entry->layer_id, __entry->rdma_id)
);

TRACE_EVENT(dpu_mclk_scl,
	TP_PROTO(uint64_t in_width, uint64_t in_height, uint64_t out_width, uint64_t out_height, uint64_t bpp, uint64_t C, uint64_t width_ratio, uint64_t height_ratio),
	TP_ARGS(in_width, in_height, out_width, out_height, bpp, C, width_ratio, height_ratio),
	TP_STRUCT__entry(
		__field(uint64_t, in_width)
		__field(uint64_t, in_height)
		__field(uint64_t, out_width)
		__field(uint64_t, out_height)
		__field(uint64_t, bpp)
		__field(uint64_t, C)
		__field(uint64_t, width_ratio)
		__field(uint64_t, height_ratio)
	),
	TP_fast_assign(
		__entry->in_width = in_width;
		__entry->in_height = in_height;
		__entry->out_width = out_width;
		__entry->out_height = out_height;
		__entry->bpp = bpp;
		__entry->C = C;
		__entry->width_ratio = width_ratio;
		__entry->height_ratio = height_ratio;
	),
	TP_printk("scl_in_width = %lld, scl_in_height = %lld, scl_out_width = %lld, scl_out_height = %lld, bpp = %lld, C = %lld, width_ratio = %lld, height_ratio = %lld \n",
		__entry->in_width, __entry->in_height, __entry->out_width,
		__entry->out_height, __entry->bpp, __entry->C,
		__entry->width_ratio, __entry->height_ratio)
);

TRACE_EVENT(dpu_fpixclk_hblk,
	TP_PROTO(uint64_t hact, uint64_t fps, uint64_t vtotal, uint64_t fpixclk_hblk),
	TP_ARGS(hact, fps, vtotal, fpixclk_hblk),
	TP_STRUCT__entry(
		__field(uint64_t, hact)
		__field(uint64_t, fps)
		__field(uint64_t, vtotal)
		__field(uint64_t, fpixclk_hblk)
	),
	TP_fast_assign(
		__entry->hact = hact;
		__entry->fps = fps;
		__entry->vtotal = vtotal;
		__entry->fpixclk_hblk = fpixclk_hblk;
	),
	TP_printk("hact = %lld, fps = %lld, vtotal = %lld, Fpixclk_hblk = %lld\n",
		  __entry->hact, __entry->fps, __entry->vtotal, __entry->fpixclk_hblk)
);

TRACE_EVENT(dpuctrl_scaling_setting,
	TP_PROTO(uint32_t id, int in_use, int rdma_id),
	TP_ARGS(id, in_use, rdma_id),
	TP_STRUCT__entry(
		__field(uint32_t, id)
		__field(int, in_use)
		__field(int, rdma_id)
	),
	TP_fast_assign(
		__entry->id = id;
		__entry->in_use = in_use;
		__entry->rdma_id = rdma_id;
	),
	TP_printk("scaler%d: in_use:0x%x rdma_id:%d",
		  __entry->id, __entry->in_use, __entry->rdma_id)
);

TRACE_EVENT(dpu_reg_info,
	TP_PROTO(u8* mod_name, phys_addr_t p_addr, uint32_t reg_num),
	TP_ARGS(mod_name, p_addr, reg_num),
	TP_STRUCT__entry(
		__string(name_str, mod_name)
		__field(phys_addr_t, p_addr)
		__field(uint32_t, reg_num)
	),
	TP_fast_assign(
		__assign_str(name_str, mod_name);
		__entry->p_addr = p_addr;
		__entry->reg_num = reg_num;
	),
	TP_printk("%s(0x%08llx), num:%d",
		__get_str(name_str), __entry->p_addr, __entry->reg_num)
);

TRACE_EVENT(dpu_reg_dump,
	TP_PROTO(uint32_t reg, uint32_t val),
	TP_ARGS(reg, val),
	TP_STRUCT__entry(
		__field(uint32_t, reg)
		__field(uint32_t, val)
	),
	TP_fast_assign(
		__entry->reg = reg;
		__entry->val = val;
	),
	TP_printk("0x%08x: 0x%08x",
		  __entry->reg, __entry->val)
);

DECLARE_EVENT_CLASS(dpu_func_template,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id),
	TP_STRUCT__entry(
		__field(uint32_t, dev_id)
	),
	TP_fast_assign(
		__entry->dev_id = dev_id;
	),
	TP_printk("dev_id=%d", __entry->dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_plane_check_rdma,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_plane_atomic_check,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_plane_atomic_update,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_plane_reset,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_plane_atomic_duplicate_state,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_plane_atomic_destroy_state,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_plane_init,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_enable_clocks,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_disable_clocks,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, saturn_wait_for_cfg_ready,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, saturn_wait_for_eof,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, saturn_wait_for_wb,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, saturn_run_display,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_init,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_uninit,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_isr,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_run,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, dpu_stop,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_crtc_mode_set_nofb,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_crtc_atomic_enable,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_crtc_atomic_disable,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_crtc_atomic_check,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_crtc_atomic_begin,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_crtc_atomic_flush,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_crtc_enable_vblank,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_crtc_disable_vblank,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_dpu_run,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_dpu_stop,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_dpu_init,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);
DEFINE_EVENT(dpu_func_template, ky_dpu_uninit,
	TP_PROTO(uint32_t dev_id),
	TP_ARGS(dev_id)
);

DECLARE_EVENT_CLASS(dpu_ctrl_template,
	TP_PROTO(int id, int enable),
	TP_ARGS(id, enable),
	TP_STRUCT__entry(
		__field(uint32_t, id)
		__field(uint32_t, enable)
	),
	TP_fast_assign(
		__entry->id = id;
		__entry->enable = enable;
	),
	TP_printk("id=%d enable:%d",
		  __entry->id, __entry->enable)
);
DEFINE_EVENT(dpu_ctrl_template, saturn_ctrl_cfg_ready,
	TP_PROTO(int id, int enable),
	TP_ARGS(id, enable)
);
DEFINE_EVENT(dpu_ctrl_template, saturn_ctrl_sw_start,
	TP_PROTO(int id, int enable),
	TP_ARGS(id, enable)
);


TRACE_EVENT(saturn_conf_scaler_x,
	TP_PROTO(struct ky_plane_state *state),
	TP_ARGS(state),
	TP_STRUCT__entry(
		__field(int, id)
	),
	TP_fast_assign(
		__entry->id = state->scaler_id;
	),
	TP_printk("scaler_id:%d", __entry->id)
);


#endif /* _DPU_TRACE_H_ */

/* This part must be outside protection */
#undef TRACE_INCLUDE_FILE
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../../drivers/gpu/drm/ky/dpu/

#define TRACE_INCLUDE_FILE dpu_trace
#include <trace/define_trace.h>

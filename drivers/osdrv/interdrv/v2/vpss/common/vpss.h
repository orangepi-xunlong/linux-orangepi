#ifndef _VPSS_H_vpss_g
#define _VPSS_H_

#include <linux/common.h>
#include <linux/comm_video.h>
#include <linux/comm_sys.h>
#include <linux/comm_vpss.h>
#include <linux/comm_region.h>
#include <linux/comm_errno.h>
#include "base_ctx.h"
#include <linux/vpss_uapi.h>
#include <vpss_ctx.h>
#include <base_ctx.h>
#include <base_cb.h>
#include <ldc_cb.h>
#include "vpss_debug.h"

typedef void (*vpss_timer_cb)(void *data);

/* Configured from user, IOCTL */
signed int vpss_create_grp(vpss_grp grp_id, const vpss_grp_attr_s *grp_attr);
signed int vpss_destroy_grp(vpss_grp grp_id);
vpss_grp vpss_get_available_grp(void);

signed int vpss_start_grp(vpss_grp grp_id);
signed int vpss_stop_grp(vpss_grp grp_id);

signed int vpss_reset_grp(vpss_grp grp_id);

signed int vpss_set_grp_attr(vpss_grp grp_id, const vpss_grp_attr_s *grp_attr);
signed int vpss_get_grp_attr(vpss_grp grp_id, vpss_grp_attr_s *grp_attr);

signed int vpss_set_grp_crop(vpss_grp grp_id, const vpss_crop_info_s *crop_info);
signed int vpss_get_grp_crop(vpss_grp grp_id, vpss_crop_info_s *crop_info);

signed int vpss_get_grp_frame(vpss_grp grp_id, video_frame_info_s *video_frame);
signed int vpss_release_grp_frame(vpss_grp grp_id, const video_frame_info_s *video_frame);

signed int vpss_send_frame(vpss_grp grp_id, const video_frame_info_s *video_frame, signed int milli_sec);
signed int vpss_send_chn_frame(vpss_grp grp_id, vpss_chn chn_id
	, const video_frame_info_s *video_frame, signed int milli_sec);

signed int vpss_set_chn_attr(vpss_grp grp_id, vpss_chn chn_id, const vpss_chn_attr_s *chn_attr);
signed int vpss_get_chn_attr(vpss_grp grp_id, vpss_chn chn_id, vpss_chn_attr_s *chn_attr);

signed int vpss_enable_chn(vpss_grp grp_id, vpss_chn chn_id);
signed int vpss_disable_chn(vpss_grp grp_id, vpss_chn chn_id);

signed int vpss_set_chn_crop(vpss_grp grp_id, vpss_chn chn_id, const vpss_crop_info_s *crop_info);
signed int vpss_get_chn_crop(vpss_grp grp_id, vpss_chn chn_id, vpss_crop_info_s *crop_info);

signed int vpss_set_chn_rotation(vpss_grp grp_id, vpss_chn chn_id, rotation_e rotation);
signed int vpss_get_chn_rotation(vpss_grp grp_id, vpss_chn chn_id, rotation_e *rotation);

signed int vpss_set_chn_ldc_attr(vpss_grp grp_id, vpss_chn chn_id, const vpss_ldc_attr_s *ldc_attr,
			u64 mesh_addr);
signed int vpss_get_chn_ldc_attr(vpss_grp grp_id, vpss_chn chn_id, vpss_ldc_attr_s *ldc_attr);

signed int vpss_set_chn_fisheye_attr(vpss_grp grp_id, vpss_chn chn_id, rotation_e rotation,
			const fisheye_attr_s *fish_eye_attr, u64 mesh_addr);
signed int vpss_get_chn_fisheye_attr(vpss_grp grp_id, vpss_chn chn_id, fisheye_attr_s *fish_eye_attr);

signed int vpss_get_chn_frame(vpss_grp grp_id, vpss_chn chn_id, video_frame_info_s *frame_info,
			 signed int milli_sec);
signed int vpss_release_chn_frame(vpss_grp grp_id, vpss_chn chn_id, const video_frame_info_s *video_frame);

signed int vpss_set_chn_align(vpss_grp grp_id, vpss_chn chn_id, u32 align);
signed int vpss_get_chn_align(vpss_grp grp_id, vpss_chn chn_id, u32 *align);

signed int vpss_set_chn_yratio(vpss_grp grp_id, vpss_chn chn_id, u32 y_ratio);
signed int vpss_get_chn_yratio(vpss_grp grp_id, vpss_chn chn_id, u32 *y_ratio);

signed int vpss_set_chn_scale_coef_level(vpss_grp grp_id, vpss_chn chn_id, vpss_scale_coef_e coef);
signed int vpss_get_chn_scale_coef_level(vpss_grp grp_id, vpss_chn chn_id, vpss_scale_coef_e *coef);

signed int vpss_set_chn_draw_rect(vpss_grp grp_id, vpss_chn chn_id, const vpss_draw_rect_s *draw_rect);
signed int vpss_get_chn_draw_rect(vpss_grp grp_id, vpss_chn chn_id, vpss_draw_rect_s *draw_rect);

signed int vpss_set_chn_convert(vpss_grp grp_id, vpss_chn chn_id, const vpss_convert_s *convert);
signed int vpss_get_chn_convert(vpss_grp grp_id, vpss_chn chn_id, vpss_convert_s *convert);

signed int vpss_show_chn(vpss_grp grp_id, vpss_chn chn_id);
signed int vpss_hide_chn(vpss_grp grp_id, vpss_chn chn_id);

signed int vpss_attach_vb_pool(vpss_grp grp_id, vpss_chn chn_id, vb_pool vb_pool);
signed int vpss_detach_vb_pool(vpss_grp grp_id, vpss_chn chn_id);

signed int vpss_trigger_snap_frame(vpss_grp grp_id, vpss_chn chn_id, u32 frame_cnt);

signed int vpss_stitch(u32 chn_num, vpss_stitch_chn_attr_s *input,
			vpss_stitch_output_attr_s *output, video_frame_info_s *video_frame);

signed int vpss_set_mod_param(const vpss_mod_param_s *mod_param);
signed int vpss_get_mod_param(vpss_mod_param_s *mod_param);

signed int vpss_bm_send_frame(bm_vpss_cfg *vpss_cfg);

/* INTERNAL */
signed int vpss_set_vivpss_mode(const vi_vpss_mode_s *mode);
signed int vpss_set_grp_csc(struct vpss_grp_csc_cfg *cfg);
signed int vpss_set_chn_csc(struct vpss_chn_csc_cfg *cfg);
signed int vpss_get_proc_amp_ctrl(proc_amp_e type, proc_amp_ctrl_s *ctrl);
signed int vpss_get_proc_amp(vpss_grp grp_id, signed int *proc_amp);
signed int vpss_get_all_proc_amp(struct vpss_all_proc_amp_cfg *cfg);

void vpss_set_mlv_info(u8 snr_num, struct mlv_i_s *p_m_lv_i);
void vpss_get_mlv_info(u8 snr_num, struct mlv_i_s *p_m_lv_i);

int _vpss_call_cb(u32 m_id, u32 cmd_id, void *data);
void vpss_init(void);
void vpss_deinit(void);
s32 vpss_suspend_handler(void);
s32 vpss_resume_handler(void);

void vpss_gdc_callback(void *param, vb_blk blk);

signed int check_vpss_id(vpss_grp grp_id, vpss_chn chn_id);

void vpss_mode_init(void);
void vpss_mode_deinit(void);

void register_timer_fun(vpss_timer_cb cb, void *data);

struct vpss_ctx **vpss_get_ctx(void);

void vpss_release_grp(void);

//Check GRP and CHN VALID, CREATED and FMT
#define vpss_grp_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY) ||				\
	 (fmt == PIXEL_FORMAT_YUV_444))

#define VPSS_CHN_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_HSV_888) || (fmt == PIXEL_FORMAT_HSV_888_PLANAR) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY) ||				\
	 (fmt == PIXEL_FORMAT_YUV_444) ||							\
	 (fmt == PIXEL_FORMAT_FP32_C3_PLANAR) || (fmt == PIXEL_FORMAT_FP16_C3_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_BF16_C3_PLANAR) || (fmt == PIXEL_FORMAT_INT8_C3_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_UINT8_C3_PLANAR))

#define VPSS_UPSAMPLE(fmt_grp, fmt_chn) \
	((IS_FMT_YUV420(fmt_grp) \
	&& (IS_FMT_YUV422(fmt_chn) || IS_FMT_RGB(fmt_chn) || (fmt_chn == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt_chn == PIXEL_FORMAT_YUV_444))) \
	|| (IS_FMT_YUV422(fmt_grp) && (IS_FMT_RGB(fmt_chn) || (fmt_chn == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt_chn == PIXEL_FORMAT_YUV_444))))

#define FRC_INVALID(frame_rate)	\
	(frame_rate.dst_frame_rate <= 0 || frame_rate.src_frame_rate <= 0 ||	\
		frame_rate.dst_frame_rate >= frame_rate.src_frame_rate)

static inline signed int mod_check_null_ptr(mod_id_e mod, const void *ptr)
{
	if (mod >= ID_BUTT)
		return -1;
	if (!ptr) {
		TRACE_VPSS(DBG_ERR, "NULL pointer\n");
		return ERR_VPSS_NULL_PTR;
	}
	return 0;
}

static inline signed int check_vpss_grp_valid(vpss_grp grp)
{
	if ((grp >= VPSS_MAX_GRP_NUM) || (grp < 0)) {
		TRACE_VPSS(DBG_ERR, "vpss_grp(%d) exceeds Max(%d)\n", grp, VPSS_MAX_GRP_NUM);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

static inline signed int check_yuv_param(pixel_format_e fmt, u32 w, u32 h)
{
	if (fmt == PIXEL_FORMAT_YUV_PLANAR_422) {
		if (w & 0x01) {
			TRACE_VPSS(DBG_ERR, "YUV_422 width(%d) should be even.\n", w);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	} else if ((fmt == PIXEL_FORMAT_YUV_PLANAR_420)
		   || (fmt == PIXEL_FORMAT_NV12)
		   || (fmt == PIXEL_FORMAT_NV21)) {
		if (w & 0x01) {
			TRACE_VPSS(DBG_ERR, "YUV_420 width(%d) should be even.\n", w);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		if (h & 0x01) {
			TRACE_VPSS(DBG_ERR, "YUV_420 height(%d) should be even.\n", h);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	return 0;
}

static inline signed int check_vpss_grp_fmt(vpss_grp grp, pixel_format_e fmt)
{
	if (!vpss_grp_SUPPORT_FMT(fmt)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) enPixelFormat(%d) unsupported\n"
		, grp, fmt);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

static inline signed int check_vpss_chn_fmt(vpss_grp grp, vpss_chn chn, pixel_format_e fmt)
{
	if (!VPSS_CHN_SUPPORT_FMT(fmt)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) enPixelFormat(%d) unsupported\n"
		, grp, chn, fmt);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

static inline signed int check_vpss_gdc_fmt(vpss_grp grp, vpss_chn chn, pixel_format_e fmt)
{
	if (!GDC_SUPPORT_FMT(fmt)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) invalid PixFormat(%d) for GDC.\n"
		, grp, chn, (fmt));
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

static inline signed int check_vpss_dwa_fmt(vpss_grp grp, vpss_chn chn, pixel_format_e fmt)
{
	if (!DWA_SUPPORT_FMT(fmt)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) invalid PixFormat(%d) for DWA.\n"
		, grp, chn, (fmt));
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

#endif /* _VPSS_H_ */

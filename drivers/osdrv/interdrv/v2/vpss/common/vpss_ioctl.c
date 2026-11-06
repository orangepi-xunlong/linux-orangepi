#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
//#include <linux/module.h>

#include <vpss_cb.h>

#include "vpss_debug.h"
#include "vpss_core.h"
#include "vpss.h"
#include "vpss_ioctl.h"
#include "vpss_rgn_ctrl.h"
#include "base_ctx.h"

long vpss_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	char stack_kdata[128];
	char *kdata = stack_kdata;
	int ret = 0;
	unsigned int in_size, out_size, drv_size, ksize;

	/* Figure out the delta between user cmd size and kernel cmd size */
	drv_size = _IOC_SIZE(cmd);
	out_size = _IOC_SIZE(cmd);
	in_size = out_size;
	if ((cmd & IOC_IN) == 0)
		in_size = 0;
	if ((cmd & IOC_OUT) == 0)
		out_size = 0;
	ksize = max(max(in_size, out_size), drv_size);

	/* If necessary, allocate buffer for ioctl argument */
	if (ksize > sizeof(stack_kdata)) {
		kdata = kmalloc(ksize, GFP_KERNEL);
		if (!kdata)
			return -ENOMEM;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (!access_ok((void __user *)arg, in_size)) {
		TRACE_VPSS(DBG_ERR, "access_ok failed\n");
	}
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size)) {
		TRACE_VPSS(DBG_ERR, "access_ok failed\n");
	}
#endif

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		TRACE_VPSS(DBG_INFO, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
	case VPSS_CREATE_GROUP:
	{
		struct vpss_crt_grp_cfg *cfg =
			(struct vpss_crt_grp_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_crt_grp_cfg);
		ret = vpss_create_grp(cfg->vpss_grp, &cfg->grp_attr);
		break;
	}

	case VPSS_DESTROY_GROUP:
	{
		vpss_grp grp_id = *((vpss_grp *)kdata);

		CHECK_IOCTL_CMD(cmd, vpss_grp);
		ret = vpss_destroy_grp(grp_id);
		break;
	}

	case VPSS_START_GROUP:
	{
		struct vpss_str_grp_cfg *cfg = (struct vpss_str_grp_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_str_grp_cfg);
		ret = vpss_start_grp(cfg->vpss_grp);
		break;
	}

	case VPSS_STOP_GROUP:
	{
		vpss_grp grp_id = *((vpss_grp *)kdata);

		CHECK_IOCTL_CMD(cmd, vpss_grp);
		ret = vpss_stop_grp(grp_id);
		break;
	}

	case VPSS_RESET_GROUP:
	{
		vpss_grp grp_id = *((vpss_grp *)kdata);

		CHECK_IOCTL_CMD(cmd, vpss_grp);
		ret = vpss_reset_grp(grp_id);
		break;
	}

	case VPSS_GET_AVAIL_GROUP:
	{
		vpss_grp grp_id;

		CHECK_IOCTL_CMD(cmd, vpss_grp);
		grp_id = vpss_get_available_grp();
		*((vpss_grp *)kdata) = grp_id;
		ret = 0;
		break;
	}

	case VPSS_SET_GRP_ATTR:
	{
		struct vpss_grp_attr *cfg = (struct vpss_grp_attr *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;

		const vpss_grp_attr_s *grp_attr = &cfg->grp_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_attr);
		ret = vpss_set_grp_attr(vpss_grp, grp_attr);
		break;
	}

	case VPSS_GET_GRP_ATTR:
	{
		struct vpss_grp_attr *cfg = (struct vpss_grp_attr *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_grp_attr_s *grp_attr = &cfg->grp_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_attr);
		ret = vpss_get_grp_attr(vpss_grp, grp_attr);
		break;
	}

	case VPSS_SET_GRP_CROP:
	{
		struct vpss_grp_crop_cfg *cfg = (struct vpss_grp_crop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;

		const vpss_crop_info_s *crop_info = &cfg->crop_info;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_crop_cfg);
		ret = vpss_set_grp_crop(vpss_grp, crop_info);
		break;
	}

	case VPSS_GET_GRP_CROP:
	{
		struct vpss_grp_crop_cfg *cfg = (struct vpss_grp_crop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_crop_info_s *crop_info = &cfg->crop_info;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_crop_cfg);
		ret = vpss_get_grp_crop(vpss_grp, crop_info);
		break;
	}

	case VPSS_GET_GRP_FRAME:
	{
		struct vpss_grp_frame_cfg *cfg = (struct vpss_grp_frame_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		video_frame_info_s *video_frame = &cfg->video_frame;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_frame_cfg);
		ret = vpss_get_grp_frame(vpss_grp, video_frame);
		break;
	}

	case VPSS_SET_RELEASE_GRP_FRAME:
	{
		struct vpss_grp_frame_cfg *cfg = (struct vpss_grp_frame_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		const video_frame_info_s *video_frame = &cfg->video_frame;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_frame_cfg);
		ret = vpss_release_grp_frame(vpss_grp, video_frame);
		break;
	}

	case VPSS_SEND_FRAME:
	{
		struct vpss_snd_frm_cfg *cfg = (struct vpss_snd_frm_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		const video_frame_info_s *video_frame = &cfg->video_frame;
		s32 milli_sec = cfg->milli_sec;

		CHECK_IOCTL_CMD(cmd, struct vpss_snd_frm_cfg);
		ret = vpss_send_frame(vpss_grp, video_frame, milli_sec);
		break;
	}

	case VPSS_SET_GRP_CSC_CFG:
	{
		struct vpss_grp_csc_cfg *cfg = (struct vpss_grp_csc_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_csc_cfg);
		ret = vpss_set_grp_csc(cfg);
		break;
	}

	case VPSS_SET_CHN_CSC_CFG:
	{
		struct vpss_chn_csc_cfg *cfg = (struct vpss_chn_csc_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_csc_cfg);
		ret = vpss_set_chn_csc(cfg);
		break;
	}

	case VPSS_SEND_CHN_FRAME:
	{
		struct vpss_chn_frm_cfg *cfg = (struct vpss_chn_frm_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		const video_frame_info_s *video_frame = &cfg->video_frame;
		s32 milli_sec = cfg->milli_sec;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_frm_cfg);
		ret = vpss_send_chn_frame(vpss_grp, vpss_chn, video_frame, milli_sec);
		break;
	}

	case VPSS_SET_CHN_ATTR:
	{
		struct vpss_chn_attr *attr = (struct vpss_chn_attr *)kdata;
		vpss_grp vpss_grp = attr->vpss_grp;
		vpss_chn vpss_chn = attr->vpss_chn;

		const vpss_chn_attr_s *chn_attr_s = &attr->chn_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_attr);
		ret = vpss_set_chn_attr(vpss_grp, vpss_chn, chn_attr_s);
		break;
	}

	case VPSS_GET_CHN_ATTR:
	{
		struct vpss_chn_attr *attr = (struct vpss_chn_attr *)kdata;
		vpss_grp vpss_grp = attr->vpss_grp;
		vpss_chn vpss_chn = attr->vpss_chn;
		vpss_chn_attr_s *chn_attr_s = &attr->chn_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_attr);
		ret = vpss_get_chn_attr(vpss_grp, vpss_chn, chn_attr_s);
		break;
	}

	case VPSS_ENABLE_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_en_chn_cfg);
		ret = vpss_enable_chn(vpss_grp, vpss_chn);
		break;
	}

	case VPSS_DISABLE_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_en_chn_cfg);
		ret = vpss_disable_chn(cfg->vpss_grp, cfg->vpss_chn);
		break;
	}

	case VPSS_SET_CHN_CROP:
	{
		struct vpss_chn_crop_cfg *cfg = (struct vpss_chn_crop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		const vpss_crop_info_s *crop_info = &cfg->crop_info;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_crop_cfg);
		ret = vpss_set_chn_crop(vpss_grp, vpss_chn, crop_info);
		break;
	}

	case VPSS_GET_CHN_CROP:
	{
		struct vpss_chn_crop_cfg *cfg = (struct vpss_chn_crop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_crop_info_s *crop_info = &cfg->crop_info;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_crop_cfg);
		ret = vpss_get_chn_crop(vpss_grp, vpss_chn, crop_info);
		break;
	}

	case VPSS_SET_CHN_ROTATION:
	{
		struct vpss_chn_rot_cfg *cfg = (struct vpss_chn_rot_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		rotation_e rotation = cfg->rotation;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_rot_cfg);
		ret = vpss_set_chn_rotation(vpss_grp, vpss_chn, rotation);
		break;
	}

	case VPSS_GET_CHN_ROTATION:
	{
		struct vpss_chn_rot_cfg *cfg = (struct vpss_chn_rot_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		rotation_e *rotation = &cfg->rotation;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_rot_cfg);
		ret = vpss_get_chn_rotation(vpss_grp, vpss_chn, rotation);
		break;
	}

	case VPSS_SET_CHN_LDC:
	{
		struct vpss_chn_ldc_cfg *cfg = (struct vpss_chn_ldc_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u64 mesh_addr = cfg->mesh_handle;

		const vpss_ldc_attr_s *ldc_attr = &cfg->ldc_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_ldc_cfg);
		ret = vpss_set_chn_ldc_attr(vpss_grp, vpss_chn, ldc_attr, mesh_addr);
		break;
	}

	case VPSS_GET_CHN_LDC:
	{
		struct vpss_chn_ldc_cfg *cfg = (struct vpss_chn_ldc_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_ldc_attr_s *ldc_attr = &cfg->ldc_attr;

		// vpss_get_chn_rotation(vpss_grp, vpss_chn, &cfg->rotation);
		CHECK_IOCTL_CMD(cmd, struct vpss_chn_ldc_cfg);
		ret = vpss_get_chn_ldc_attr(vpss_grp, vpss_chn, ldc_attr);
		break;
	}

	case VPSS_SET_CHN_FISHEYE:
	{
		struct vpss_chn_fisheye_cfg *cfg = (struct vpss_chn_fisheye_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		rotation_e rotation = cfg->rotation;
		u64 mesh_addr = cfg->mesh_handle;

		const fisheye_attr_s *fisheye_attr = &cfg->fisheye_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_fisheye_cfg);
		ret = vpss_set_chn_fisheye_attr(vpss_grp, vpss_chn, rotation, fisheye_attr, mesh_addr);
		break;
	}

	case VPSS_GET_CHN_FISHEYE:
	{
		struct vpss_chn_fisheye_cfg *cfg = (struct vpss_chn_fisheye_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		fisheye_attr_s *fisheye_attr = &cfg->fisheye_attr;

		vpss_get_chn_rotation(vpss_grp, vpss_chn, &cfg->rotation);

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_fisheye_cfg);
		ret = vpss_get_chn_fisheye_attr(vpss_grp, vpss_chn, fisheye_attr);
		break;
	}

	case VPSS_GET_CHN_FRAME:
	{
		struct vpss_chn_frm_cfg *cfg = (struct vpss_chn_frm_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		video_frame_info_s *video_frame = &cfg->video_frame;
		s32 milli_sec = cfg->milli_sec;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_frm_cfg);
		ret = vpss_get_chn_frame(vpss_grp, vpss_chn, video_frame, milli_sec);
		break;
	}

	case VPSS_RELEASE_CHN_RAME:
	{
		struct vpss_chn_frm_cfg *cfg = (struct vpss_chn_frm_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		const video_frame_info_s *video_frame = &cfg->video_frame;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_frm_cfg);
		ret = vpss_release_chn_frame(vpss_grp, vpss_chn, video_frame);
		break;
	}

	case VPSS_SET_CHN_ALIGN:
	{
		struct vpss_chn_align_cfg *cfg = (struct vpss_chn_align_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 align = cfg->align;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_align_cfg);
		ret = vpss_set_chn_align(vpss_grp, vpss_chn, align);
		break;
	}

	case VPSS_GET_CHN_ALIGN:
	{
		struct vpss_chn_align_cfg *cfg = (struct vpss_chn_align_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 *align = &cfg->align;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_align_cfg);
		ret = vpss_get_chn_align(vpss_grp, vpss_chn, align);
		break;
	}

	case VPSS_SET_CHN_YRATIO:
	{
		struct vpss_chn_yratio_cfg *cfg = (struct vpss_chn_yratio_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 y_ratio = cfg->y_ratio;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_yratio_cfg);
		ret = vpss_set_chn_yratio(vpss_grp, vpss_chn, y_ratio);
		break;
	}

	case VPSS_GET_CHN_YRATIO:
	{
		struct vpss_chn_yratio_cfg *cfg = (struct vpss_chn_yratio_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 *y_ratio = &cfg->y_ratio;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_yratio_cfg);
		ret = vpss_get_chn_yratio(vpss_grp, vpss_chn, y_ratio);
		break;
	}

	case VPSS_SET_CHN_SCALE_COEFF_LEVEL:
	{
		struct vpss_chn_coef_level_cfg *cfg = (struct vpss_chn_coef_level_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_scale_coef_e coef = cfg->coef;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_coef_level_cfg);
		ret = vpss_set_chn_scale_coef_level(vpss_grp, vpss_chn, coef);
		break;
	}

	case VPSS_GET_CHN_SCALE_COEFF_LEVEL:
	{
		struct vpss_chn_coef_level_cfg *cfg = (struct vpss_chn_coef_level_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_scale_coef_e *coef = &cfg->coef;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_coef_level_cfg);
		ret = vpss_get_chn_scale_coef_level(vpss_grp, vpss_chn, coef);
		break;
	}

	case VPSS_SET_CHN_DRAW_RECT:
	{
		struct vpss_chn_draw_rect_cfg *cfg = (struct vpss_chn_draw_rect_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_draw_rect_s *draw_rect = &cfg->draw_rect;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_draw_rect_cfg);
		ret = vpss_set_chn_draw_rect(vpss_grp, vpss_chn, draw_rect);
		break;
	}

	case VPSS_GET_CHN_DRAW_RECT:
	{
		struct vpss_chn_draw_rect_cfg *cfg = (struct vpss_chn_draw_rect_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_draw_rect_s *draw_rect = &cfg->draw_rect;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_draw_rect_cfg);
		ret = vpss_get_chn_draw_rect(vpss_grp, vpss_chn, draw_rect);
		break;
	}

	case VPSS_SET_CHN_CONVERT:
	{
		struct vpss_chn_convert_cfg *cfg = (struct vpss_chn_convert_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_convert_s *convert = &cfg->convert;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_convert_cfg);
		ret = vpss_set_chn_convert(vpss_grp, vpss_chn, convert);
		break;
	}

	case VPSS_GET_CHN_CONVERT:
	{
		struct vpss_chn_convert_cfg *cfg = (struct vpss_chn_convert_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_convert_s *convert = &cfg->convert;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_convert_cfg);
		ret = vpss_get_chn_convert(vpss_grp, vpss_chn, convert);
		break;
	}

	case VPSS_SET_COVEREX_CFG:
	{
		struct vpss_coverex_cfg *cfg = (struct vpss_coverex_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_coverex_cfg);
		ret = vpss_set_rgn_coverex_cfg(vpss_grp, vpss_chn, &cfg->rgn_coverex_cfg);
		break;
	}

	case VPSS_SET_MOSAIC_CFG:
	{
		struct vpss_mosaic_cfg *cfg = (struct vpss_mosaic_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_mosaic_cfg);
		ret = vpss_set_rgn_mosaic_cfg(vpss_grp, vpss_chn, &cfg->rgn_mosaic_cfg);
		break;
	}

	case VPSS_SET_GOP_CFG:
	{
		struct vpss_gop_cfg *cfg = (struct vpss_gop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 layer = cfg->layer;

		CHECK_IOCTL_CMD(cmd, struct vpss_gop_cfg);
		ret = vpss_set_rgn_cfg(vpss_grp, vpss_chn, layer, &cfg->rgn_cfg);
		break;
	}

	case VPSS_SHOW_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_en_chn_cfg);
		ret = vpss_show_chn(vpss_grp, vpss_chn);
		break;
	}

	case VPSS_HIDE_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_en_chn_cfg);
		ret = vpss_hide_chn(vpss_grp, vpss_chn);
		break;
	}

	case VPSS_ATTACH_VB_POOL:
	{
		struct vpss_vb_pool_cfg *cfg = (struct vpss_vb_pool_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vb_pool pool_id = (vb_pool)cfg->vb_pool;

		CHECK_IOCTL_CMD(cmd, struct vpss_vb_pool_cfg);
		ret = vpss_attach_vb_pool(vpss_grp, vpss_chn, pool_id);
		break;
	}

	case VPSS_DETACH_VB_POOL:
	{
		struct vpss_vb_pool_cfg *cfg = (struct vpss_vb_pool_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_vb_pool_cfg);
		ret = vpss_detach_vb_pool(vpss_grp, vpss_chn);
		break;
	}

	case VPSS_TRIGGER_SNAP_FRAME:
	{
		struct vpss_snap_cfg *cfg = (struct vpss_snap_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 frame_cnt = cfg->frame_cnt;

		CHECK_IOCTL_CMD(cmd, struct vpss_snap_cfg);
		ret = vpss_trigger_snap_frame(vpss_grp, vpss_chn, frame_cnt);
		break;
	}

	case VPSS_SET_MOD_PARAM:
	{
		const vpss_mod_param_s *cfg = (vpss_mod_param_s *)kdata;

		CHECK_IOCTL_CMD(cmd, vpss_mod_param_s);
		ret = vpss_set_mod_param(cfg);
		break;
	}

	case VPSS_GET_MOD_PARAM:
	{
		vpss_mod_param_s *cfg = (vpss_mod_param_s *)kdata;

		CHECK_IOCTL_CMD(cmd, vpss_mod_param_s);
		ret = vpss_get_mod_param(cfg);
		break;
	}

	case VPSS_GET_AMP_CTRL:
	{
		struct vpss_proc_amp_ctrl_cfg *cfg = (struct vpss_proc_amp_ctrl_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_proc_amp_ctrl_cfg);
		ret = vpss_get_proc_amp_ctrl(cfg->type, &cfg->ctrl);
		break;
	}

	case VPSS_GET_AMP_CFG:
	{
		struct vpss_proc_amp_cfg *cfg = (struct vpss_proc_amp_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;

		CHECK_IOCTL_CMD(cmd, struct vpss_proc_amp_cfg);
		ret = vpss_get_proc_amp(vpss_grp, cfg->proc_amp);
		break;
	}

	case VPSS_GET_ALL_AMP:
	{
		struct vpss_all_proc_amp_cfg *cfg = (struct vpss_all_proc_amp_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_all_proc_amp_cfg);
		ret = vpss_get_all_proc_amp(cfg);
		break;
	}

	case VPSS_STITCH:
	{
		u32 chn_num;
		vpss_stitch_chn_attr_s *input;
		struct _vpss_stitch_cfg *cfg = (struct _vpss_stitch_cfg *)kdata;

		chn_num = cfg->chn_num;
		input = (vpss_stitch_chn_attr_s *)vmalloc(sizeof(vpss_stitch_chn_attr_s) * chn_num);

		ret = copy_from_user(input, cfg->input, sizeof(vpss_stitch_chn_attr_s) * chn_num);
		if (ret != 0) {
			TRACE_VPSS(DBG_INFO, "input copy_from_user failed: ret=%d\n", ret);
			vfree(input);
			break;
		}

		CHECK_IOCTL_CMD(cmd, struct _vpss_stitch_cfg);
		ret = vpss_stitch(chn_num, input, &cfg->output, &cfg->video_frame);

		vfree(input);
		break;
	}

	case VPSS_BM_SEND_FRAME:
	{
		bm_vpss_cfg *cfg = (bm_vpss_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, bm_vpss_cfg);
		ret = vpss_bm_send_frame(cfg);
		break;
	}

	default:
		TRACE_VPSS(DBG_DEBUG, "unknown cmd(0x%x)\n", cmd);
		break;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}

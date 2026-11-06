#include <proc/vo_proc.h>
#include <linux/version.h>

#define VO_PRC_NAME	"soph/vo"

/*************************************************************************
 *	VO proc functions
 *************************************************************************/
static void intf_type_to_string(vo_intf_type_e intf_type, char *str, int len)
{
	switch (intf_type) {
	case VO_INTF_MIPI:
		strncpy(str, "MIPI", len);
		break;
	case VO_INTF_LVDS:
		strncpy(str, "LVDS", len);
		break;
	case VO_INTF_BT656:
		strncpy(str, "BT656", len);
		break;
	case VO_INTF_BT1120:
		strncpy(str, "BT1120", len);
		break;
	case VO_INTF_PARALLEL_RGB:
		strncpy(str, "PARALLEL_RGB", len);
		break;
	case VO_INTF_SERIAL_RGB:
		strncpy(str, "SERIAL_RGB", len);
		break;
	case VO_INTF_I80:
		strncpy(str, "I80", len);
		break;
	case VO_INTF_HW_MCU:
		strncpy(str, "HW_MCU", len);
		break;
	case VO_INTF_HDMI:
		strncpy(str, "HDMI", len);
		break;
	default:
		strncpy(str, "Unknown Type", len);
		break;
	}
}

static void intf_sync_to_string(vo_intf_sync_e intf_sync, char *str, int len)
{
	switch (intf_sync) {
	case VO_OUTPUT_PAL:
		strncpy(str, "PAL", len);
		break;
	case VO_OUTPUT_NTSC:
		strncpy(str, "NTSC", len);
		break;
	case VO_OUTPUT_1080P24:
		strncpy(str, "1080P@24", len);
		break;
	case VO_OUTPUT_1080P25:
		strncpy(str, "1080P@25", len);
		break;
	case VO_OUTPUT_1080P30:
		strncpy(str, "1080P@30", len);
		break;
	case VO_OUTPUT_720P50:
		strncpy(str, "720P@50", len);
		break;
	case VO_OUTPUT_720P60:
		strncpy(str, "720P@60", len);
		break;
	case VO_OUTPUT_1080P50:
		strncpy(str, "1080P@50", len);
		break;
	case VO_OUTPUT_1080P60:
		strncpy(str, "1080P@60", len);
		break;
	case VO_OUTPUT_576P50:
		strncpy(str, "576P@50", len);
		break;
	case VO_OUTPUT_480P60:
		strncpy(str, "480P@60", len);
		break;
	case VO_OUTPUT_800x600_60:
		strncpy(str, "800x600@60", len);
		break;
	case VO_OUTPUT_1024x768_60:
		strncpy(str, "1024x768@60", len);
		break;
	case VO_OUTPUT_1280x1024_60:
		strncpy(str, "1280x1024@60", len);
		break;
	case VO_OUTPUT_1366x768_60:
		strncpy(str, "1366x768@60", len);
		break;
	case VO_OUTPUT_1440x900_60:
		strncpy(str, "1440x900@60", len);
		break;
	case VO_OUTPUT_1280x800_60:
		strncpy(str, "1280x800@60", len);
		break;
	case VO_OUTPUT_1600x1200_60:
		strncpy(str, "1600x1200@60", len);
		break;
	case VO_OUTPUT_1680x1050_60:
		strncpy(str, "1680x1050@60", len);
		break;
	case VO_OUTPUT_1920x1200_60:
		strncpy(str, "1920x1200@60", len);
		break;
	case VO_OUTPUT_640x480_60:
		strncpy(str, "640x480@60", len);
		break;
	case VO_OUTPUT_720x1280_60:
		strncpy(str, "720x1280@60", len);
		break;
	case VO_OUTPUT_1080x1920_60:
		strncpy(str, "1080x1920@60", len);
		break;
	case VO_OUTPUT_480x800_60:
		strncpy(str, "480x800@60", len);
		break;
	case VO_OUTPUT_1440P60:
		strncpy(str, "1440P@60", len);
		break;
	case VO_OUTPUT_USER:
		strncpy(str, "User timing", len);
		break;
	default:
		strncpy(str, "Unknown Timing", len);
		break;
	}
}

static void pix_fmt_to_string(pixel_format_e pixfmt, char *str, int len)
{
	switch (pixfmt) {
	case PIXEL_FORMAT_RGB_888:
		strncpy(str, "RGB_888", len);
		break;
	case PIXEL_FORMAT_BGR_888:
		strncpy(str, "BGR_888", len);
		break;
	case PIXEL_FORMAT_RGB_888_PLANAR:
		strncpy(str, "RGB_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_BGR_888_PLANAR:
		strncpy(str, "BGR_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_ARGB_1555:
		strncpy(str, "ARGB_1555", len);
		break;
	case PIXEL_FORMAT_ARGB_4444:
		strncpy(str, "ARGB_4444", len);
		break;
	case PIXEL_FORMAT_ARGB_8888:
		strncpy(str, "ARGB_8888", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_8BPP:
		strncpy(str, "RGB_BAYER_8BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_10BPP:
		strncpy(str, "RGB_BAYER_10BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_12BPP:
		strncpy(str, "RGB_BAYER_12BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_14BPP:
		strncpy(str, "RGB_BAYER_14BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_16BPP:
		strncpy(str, "RGB_BAYER_16BPP", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_422:
		strncpy(str, "YUV_PLANAR_422", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_420:
		strncpy(str, "YUV_PLANAR_420", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_444:
		strncpy(str, "YUV_PLANAR_444", len);
		break;
	case PIXEL_FORMAT_YUV_400:
		strncpy(str, "YUV_400", len);
		break;
	case PIXEL_FORMAT_HSV_888:
		strncpy(str, "HSV_888", len);
		break;
	case PIXEL_FORMAT_HSV_888_PLANAR:
		strncpy(str, "HSV_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_NV12:
		strncpy(str, "NV12", len);
		break;
	case PIXEL_FORMAT_NV21:
		strncpy(str, "NV21", len);
		break;
	case PIXEL_FORMAT_NV16:
		strncpy(str, "NV16", len);
		break;
	case PIXEL_FORMAT_NV61:
		strncpy(str, "NV61", len);
		break;
	case PIXEL_FORMAT_YUYV:
		strncpy(str, "YUYV", len);
		break;
	case PIXEL_FORMAT_UYVY:
		strncpy(str, "UYVY", len);
		break;
	case PIXEL_FORMAT_YVYU:
		strncpy(str, "YVYU", len);
		break;
	case PIXEL_FORMAT_VYUY:
		strncpy(str, "VYUY", len);
		break;
	case PIXEL_FORMAT_FP32_C1:
		strncpy(str, "FP32_C1", len);
		break;
	case PIXEL_FORMAT_FP32_C3_PLANAR:
		strncpy(str, "FP32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT32_C1:
		strncpy(str, "INT32_C1", len);
		break;
	case PIXEL_FORMAT_INT32_C3_PLANAR:
		strncpy(str, "INT32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT32_C1:
		strncpy(str, "UINT32_C1", len);
		break;
	case PIXEL_FORMAT_UINT32_C3_PLANAR:
		strncpy(str, "UINT32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_BF16_C1:
		strncpy(str, "BF16_C1", len);
		break;
	case PIXEL_FORMAT_BF16_C3_PLANAR:
		strncpy(str, "BF16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT16_C1:
		strncpy(str, "INT16_C1", len);
		break;
	case PIXEL_FORMAT_INT16_C3_PLANAR:
		strncpy(str, "INT16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT16_C1:
		strncpy(str, "UINT16_C1", len);
		break;
	case PIXEL_FORMAT_UINT16_C3_PLANAR:
		strncpy(str, "UINT16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT8_C1:
		strncpy(str, "INT8_C1", len);
		break;
	case PIXEL_FORMAT_INT8_C3_PLANAR:
		strncpy(str, "INT8_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT8_C1:
		strncpy(str, "UINT8_C1", len);
		break;
	case PIXEL_FORMAT_UINT8_C3_PLANAR:
		strncpy(str, "UINT8_C3_PLANAR", len);
		break;
	default:
		strncpy(str, "Unknown Fmt", len);
		break;
	}
}

static void wbc_src_to_string(vo_wbc_src_type_e src_type, char *str, int len)
{
	switch (src_type) {
	case VO_WBC_SRC_DEV:
		strncpy(str, "DEV", len);
		break;
	case VO_WBC_SRC_VIDEO:
		strncpy(str, "VIDEO", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static void wbc_mode_to_string(vo_wbc_mode_e wbc_mode, char *str, int len)
{
	switch (wbc_mode) {
	case VO_WBC_MODE_NORM:
		strncpy(str, "NORM", len);
		break;
	case VO_WBC_MODE_DROP_REPEAT:
		strncpy(str, "REPEAT", len);
		break;
	case VO_WBC_MODE_PROGRESSIVE_TO_INTERLACED:
		strncpy(str, "INTERLACED", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static void chn_aspect_ratio_to_string(aspect_ratio_e mode, char *str, int len)
{
	switch (mode) {
	case ASPECT_RATIO_NONE:
		strncpy(str, "NONE", len);
		break;
	case ASPECT_RATIO_AUTO:
		strncpy(str, "AUTO", len);
		break;
	case ASPECT_RATIO_MANUAL:
		strncpy(str, "MANUAL", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static void chn_zoom_type_to_string(vo_chn_zoom_type_e zoom_type, char *str, int len)
{
	switch (zoom_type) {
	case VO_CHN_ZOOM_IN_RECT:
		strncpy(str, "RECT", len);
		break;
	case VO_CHN_ZOOM_IN_RATIO:
		strncpy(str, "RATIO", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static void chn_mirror_to_string(vo_chn_mirror_type_e mirror, char *str, int len)
{
	switch (mirror) {
	case VO_CHN_MIRROR_NONE:
		strncpy(str, "NONE", len);
		break;
	case VO_CHN_MIRROR_HOR:
		strncpy(str, "HOR", len);
		break;
	case VO_CHN_MIRROR_VER:
		strncpy(str, "VER", len);
		break;
	case VO_CHN_MIRROR_BOTH:
		strncpy(str, "BOTH", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static int vo_proc_show(struct seq_file *m, void *v)
{
	int i, j, cnt;
	char c[32], d[32];
	struct vo_ctx *vo_ctx_p = (struct vo_ctx *)m->private;

#if 0//TODO: UTS_VERSION
	seq_printf(m, "\nModule: [VO], Build Time[%s]\n", UTS_VERSION);
#endif
	// Device Config
	seq_puts(m, "\n-------------------------------DEVICE CONFIG------------------------------\n");
	seq_printf(m, "%10s%10s%20s%20s%10s%10s\n", "DevID", "DevEn", "intf_type", "IntfSync", "BkClr", "DevFrt");
	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		memset(c, 0, sizeof(c));
		intf_type_to_string(vo_ctx_p->dev_ctx[i].pub_attr.intf_type, c, sizeof(c));

		memset(d, 0, sizeof(d));
		intf_sync_to_string(vo_ctx_p->dev_ctx[i].pub_attr.intf_sync, d, sizeof(d));
		seq_printf(m, "%8s%2d%10s%20s%20s%10X%10d\n",
				"#",
				i,
				(vo_ctx_p->dev_ctx[i].is_dev_enable) ? "Y" : "N",
				c,
				d,
				vo_ctx_p->dev_ctx[i].pub_attr.bgcolor,
				vo_ctx_p->dev_ctx[i].pub_attr.sync_info.frame_rate);
	}

	// video layer status 1
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 1-----------------------\n");
	seq_printf(m, "%10s%10s%20s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "VideoEn", "pixfmt", "ImgW", "ImgH", "DispX", "DispY", "DispW", "DispH");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		memset(c, 0, sizeof(c));
		pix_fmt_to_string(vo_ctx_p->layer_ctx[i].layer_attr.pixformat, c, sizeof(c));

		seq_printf(m, "%8s%2d%10s%20s%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				(vo_ctx_p->layer_ctx[i].is_layer_enable) ? "Y" : "N",
				c,
				vo_ctx_p->layer_ctx[i].layer_attr.img_size.width,
				vo_ctx_p->layer_ctx[i].layer_attr.img_size.height,
				vo_ctx_p->layer_ctx[i].layer_attr.disp_rect.x,
				vo_ctx_p->layer_ctx[i].layer_attr.disp_rect.y,
				vo_ctx_p->layer_ctx[i].layer_attr.disp_rect.width,
				vo_ctx_p->layer_ctx[i].layer_attr.disp_rect.height);
	}

	// video layer status 2
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 2 (continue)------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%15s%20s%20s\n",
		"LayerId", "BindDevId", "EnChNum", "Luma", "Cont", "Hue", "Satu", "Toleration", "DispalyPts", "PreDonePts");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {

		cnt = 0;
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				cnt++;
		}

		seq_printf(m, "%8s%2d%10d%10d%10d%10d%10d%10d%15d%20lld%20lld\n",
				"#",
				i,
				vo_ctx_p->layer_ctx[i].bind_dev_id,
				cnt,
				vo_ctx_p->layer_ctx[i].proc_amp[PROC_AMP_BRIGHTNESS],
				vo_ctx_p->layer_ctx[i].proc_amp[PROC_AMP_CONTRAST],
				vo_ctx_p->layer_ctx[i].proc_amp[PROC_AMP_HUE],
				vo_ctx_p->layer_ctx[i].proc_amp[PROC_AMP_SATURATION],
				vo_ctx_p->layer_ctx[i].toleration,
				vo_ctx_p->layer_ctx[i].display_pts,
				vo_ctx_p->layer_ctx[i].predone_pts);
	}

	// video layer status 3
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 3 (continue)------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "BufLen", "Depth", "SrcFrt", "RealFrt", "BwFail", "OsdBwFail");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {

		cnt = 0;
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				cnt++;
		}

		seq_printf(m, "%8s%2d%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				vo_ctx_p->layer_ctx[i].display_buflen,
				vo_ctx_p->layer_ctx[i].layer_attr.depth,
				vo_ctx_p->layer_ctx[i].src_frame_rate,
				vo_ctx_p->layer_ctx[i].frame_rate,
				vo_ctx_p->layer_ctx[i].bw_fail,
				vo_ctx_p->layer_ctx[i].vgop_bw_fail);
	}

	// gragphic layer status 1
	seq_puts(m, "\n-------------------------------GRAPHIC LAYER STATUS 1-----------------------\n");
	seq_printf(m, "%10s%10s%10s\n",
		"LayerId", "BindDevId", "Prio");
	for (i = VO_MAX_VIDEO_LAYER_NUM; i < VO_MAX_LAYER_NUM; ++i) {

		seq_printf(m, "%8s%2d%10d%10d\n",
				"#",
				i,
				vo_ctx_p->overlay_ctx[i - VO_MAX_VIDEO_LAYER_NUM].bind_dev_id,
				vo_ctx_p->overlay_ctx[i - VO_MAX_VIDEO_LAYER_NUM].priority);
	}

	// chn basic info
	seq_puts(m, "\n-------------------------------CHN BASIC INFO 1---------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "ChnEn", "Prio", "SrcW", "SrcH", "ChnX", "ChnY", "ChnW", "ChnH", "RotAngle", "Thrshd", "Depth");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				continue;
			memset(c, 0, sizeof(c));
			if (vo_ctx_p->layer_ctx[i].chn_ctx[j].rotation == ROTATION_0)
				strncpy(c, "0", sizeof(c));
			else if (vo_ctx_p->layer_ctx[i].chn_ctx[j].rotation == ROTATION_90)
				strncpy(c, "90", sizeof(c));
			else if (vo_ctx_p->layer_ctx[i].chn_ctx[j].rotation == ROTATION_180)
				strncpy(c, "180", sizeof(c));
			else if (vo_ctx_p->layer_ctx[i].chn_ctx[j].rotation == ROTATION_270)
				strncpy(c, "270", sizeof(c));
			else
				strncpy(c, "Invalid", sizeof(c));

			seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%10d%10d%10d%10d%10d%10s%10d%10d\n",
				"#",
				i,
				"#",
				j,
				(vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable) ? "Y" : "N",
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.priority,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].src_width,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].src_height,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.rect.x,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.rect.y,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.rect.width,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.rect.height,
				c,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].threshold,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.depth);
		}
	}

	seq_puts(m, "\n-------------------------------CHN ZOOM INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "ZoomType", "RectX", "RectY", "RectW", "RectH", "Xratio", "Yratio", "WRatio", "HRatio");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				continue;
			memset(c, 0, sizeof(c));
			chn_zoom_type_to_string(vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_type,
						c, sizeof(c));

			seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				"#",
				j,
				c,
				(!vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_type) ?
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.rect.x : 0,
				(!vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_type) ?
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.rect.y : 0,
				(!vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_type) ?
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.rect.width : 0,
				(!vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_type) ?
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.rect.height : 0,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_type ?
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_ratio.x_ratio : 0,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_type ?
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_ratio.y_ratio : 0,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_type ?
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_ratio.width_ratio : 0,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_type ?
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_zoom_attr.zoom_ratio.height_ratio : 0);
		}
	}

	seq_puts(m, "\n-------------------------------CHN ASPECTRATIO INFO-----------------------\n");
	seq_printf(m, "%10s%10s%15s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "AspectRatio", "BkClrEn", "BkClr", "RectX", "RectY", "RectW", "RectH", "Mirror");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				continue;
			memset(c, 0, sizeof(c));
			chn_aspect_ratio_to_string(vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_param.aspect_ratio.mode,
						   c, sizeof(c));
			memset(d, 0, sizeof(d));
			chn_mirror_to_string(vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_mirror, d, sizeof(d));

			seq_printf(m, "%8s%2d%8s%2d%15s%10s%10X%10d%10d%10d%10d%10s\n",
				"#",
				i,
				"#",
				j,
				c,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_param.aspect_ratio.enable_bgcolor ? "Y" : "N",
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_param.aspect_ratio.bgcolor,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_param.aspect_ratio.video_rect.x,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_param.aspect_ratio.video_rect.y,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_param.aspect_ratio.video_rect.width,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_param.aspect_ratio.video_rect.height,
				d);
		}
	}

	seq_puts(m, "\n-------------------------------CHN BORDER INFO----------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "BorderEn", "TopW", "BottomW", "LeftW", "RightW", "Color");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				continue;

			seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%10d%10d%10X\n",
				"#",
				i,
				"#",
				j,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_border_attr.enable ? "Y" : "N",
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_border_attr.border.top_width,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_border_attr.border.bottom_width,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_border_attr.border.left_width,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_border_attr.border.right_width,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_border_attr.border.color);
		}
	}

	// chn play info
	seq_puts(m, "\n-------------------------------CHN PLAY INFO------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%20s%20s%20s\n",
		"LayerId", "ChnId", "Show", "Pause", "Step", "ChnSrcFrt", "ChnFrt", "ChnGap(us)", "DispalyPts", "PreDonePts");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				continue;
			seq_printf(m, "%8s%2d%8s%2d%10s%10s%10s%10d%10d%20d%20lld%20lld\n",
				"#",
				i,
				"#",
				j,
				(vo_ctx_p->layer_ctx[i].chn_ctx[j].hide) ? "N" : "Y",
				(vo_ctx_p->layer_ctx[i].chn_ctx[j].pause) ? "Y" : "N",
				(vo_ctx_p->layer_ctx[i].chn_ctx[j].step) ? "Y" : "N",
				vo_ctx_p->layer_ctx[i].chn_ctx[j].src_frame_rate,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].frame_rate,
				(vo_ctx_p->layer_ctx[i].chn_ctx[j].frame_rate == 0) ?
				0 : (1000000 / vo_ctx_p->layer_ctx[i].chn_ctx[j].frame_rate),
				vo_ctx_p->layer_ctx[i].chn_ctx[j].display_pts,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].predone_pts);
		}
	}

	// wbc info
	seq_puts(m, "\n-------------------------------WBC INFO 1---------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%15s%10s\n",
		"WbcId", "WbcEn",  "SrcType", "SrcId", "WbcW", "WbcH", "pixfmt", "Depth");
	for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
		memset(c, 0, sizeof(c));
		wbc_src_to_string(vo_ctx_p->wbc_ctx[i].wbc_src.src_type, c, sizeof(c));
		memset(d, 0, sizeof(d));
		pix_fmt_to_string(vo_ctx_p->wbc_ctx[i].wbc_attr.pixformat, d, sizeof(d));

		seq_printf(m, "%8s%2d%10s%10s%10d%10d%10d%15s%10d\n",
				"#",
				i,
				(vo_ctx_p->wbc_ctx[i].is_wbc_enable) ? "Y" : "N",
				c,
				(vo_ctx_p->wbc_ctx[i].wbc_src.src_id),
				vo_ctx_p->wbc_ctx[i].wbc_attr.target_size.width,
				vo_ctx_p->wbc_ctx[i].wbc_attr.target_size.height,
				d,
				vo_ctx_p->wbc_ctx[i].depth);
	}

	seq_puts(m, "\n-------------------------------WBC INFO 2 (continue)----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%20s\n",
		"WbcId", "WbcMode", "WbcFrt", "RealFrt", "OdmafifoFullCnt");
	for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
		memset(c, 0, sizeof(c));
		wbc_mode_to_string(vo_ctx_p->wbc_ctx[i].wbc_mode, c, sizeof(c));

		seq_printf(m, "%8s%2d%10s%10d%10d%20d\n",
				"#",
				i,
				c,
				vo_ctx_p->wbc_ctx[i].wbc_attr.frame_rate,
				vo_ctx_p->wbc_ctx[i].frame_rate,
				vo_ctx_p->wbc_ctx[i].odma_fifofull);
	}

	return 0;
}

static int vo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vo_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _vo_proc_fops = {
	.proc_open = vo_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _vo_proc_fops = {
	.owner = THIS_MODULE,
	.open = vo_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif


int vo_proc_init(struct vo_ctx *ctx)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VO_PRC_NAME, 0644, NULL, &_vo_proc_fops, ctx) == NULL) {
		pr_err("vo proc creation failed\n");
		rc = -1;
	}

	return rc;
}

int vo_proc_remove(void)
{
	remove_proc_entry(VO_PRC_NAME, NULL);

	return 0;
}

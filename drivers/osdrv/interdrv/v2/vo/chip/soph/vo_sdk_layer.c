#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <linux/comm_buffer.h>
#include <linux/defines.h>
#include "vb.h"
#include "sys.h"
#include "ion.h"
#include "vbq.h"
#include "base_common.h"
#include "disp.h"
#include "dsi_phy.h"
#include "vo.h"
#include "vo_sdk_layer.h"
#include "vo_interfaces.h"

/****************************************************************************
 * Global parameters
 ****************************************************************************/
struct vo_fmt vo_sdk_formats[] = {
	{
	.fourcc	 = PIXEL_FORMAT_YUV_PLANAR_420,
	.fmt		 = DISP_FMT_YUV420,
	.bit_depth	 = { 8, 4, 4 },
	.buffers	 = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUV_PLANAR_422,
	.fmt		 = DISP_FMT_YUV422,
	.bit_depth	 = { 8, 4, 4 },
	.buffers	 = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUV_PLANAR_444,
	.fmt		 = DISP_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV12,
	.fmt		 = DISP_FMT_NV12,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV21,
	.fmt		 = DISP_FMT_NV21,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV16,
	.fmt		 = DISP_FMT_YUV422SP1,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV61,
	.fmt		 = DISP_FMT_YUV422SP2,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUYV,
	.fmt		 = DISP_FMT_YUYV,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc		 = PIXEL_FORMAT_YVYU,
	.fmt		 = DISP_FMT_YVYU,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_UYVY,
	.fmt		 = DISP_FMT_UYVY,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_VYUY,
	.fmt		 = DISP_FMT_VYUY,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_BGR_888_PLANAR, /* rgb */
	.fmt		 = DISP_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_RGB_888_PLANAR, /* rgb */
	.fmt		 = DISP_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},

	{
	.fourcc	 = PIXEL_FORMAT_RGB_888, /* rgb */
	.fmt		 = DISP_FMT_RGB_PACKED,
	.bit_depth	 = { 24 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_BGR_888, /* bgr */
	.fmt		 = DISP_FMT_BGR_PACKED,
	.bit_depth	 = { 24 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUV_400, /* Y-Only */
	.fmt		 = DISP_FMT_Y_ONLY,
	.bit_depth	 = { 8 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_HSV_888, /* hsv */
	.fmt		 = DISP_FMT_RGB_PACKED,
	.bit_depth	 = { 24 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_HSV_888_PLANAR, /* hsv */
	.fmt		 = DISP_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
};

vo_sync_info_s sync_info[VO_OUTPUT_BUTT] = {
	[VO_OUTPUT_800x600_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 600, .vbb = 24, .vfb = 1
		, .hact = 800, .hbb = 88, .hfb = 40
		, .vpw = 4, .hpw = 128, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P24] = {.synm = 1, .iop = 1, .frame_rate = 24
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 638
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P25] = {.synm = 1, .iop = 1, .frame_rate = 25
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 528
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P30] = {.synm = 1, .iop = 1, .frame_rate = 30
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 88
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_720P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 720, .vbb = 20, .vfb = 5
		, .hact = 1280, .hbb = 220, .hfb = 440
		, .vpw = 5, .hpw = 40, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_720P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 720, .vbb = 20, .vfb = 5
		, .hact = 1280, .hbb = 220, .hfb = 110
		, .vpw = 5, .hpw = 40, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 528
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_1080P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 1080, .vbb = 36, .vfb = 4
		, .hact = 1920, .hbb = 148, .hfb = 88
		, .vpw = 5, .hpw = 44, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_576P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 576, .vbb = 39, .vfb = 5
		, .hact = 720, .hbb = 68, .hfb = 12
		, .vpw = 5, .hpw = 64, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_480P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 480, .vbb = 30, .vfb = 9
		, .hact = 720, .hbb = 60, .hfb = 16
		, .vpw = 6, .hpw = 62, .idv = 0, .ihs = 0, .ivs = 0},
	[VO_OUTPUT_720x1280_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 1280, .vbb = 4, .vfb = 6
		, .hact = 720, .hbb = 36, .hfb = 128
		, .vpw = 16, .hpw = 64, .idv = 0, .ihs = 0, .ivs = 1},
	[VO_OUTPUT_1080x1920_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 1920, .vbb = 36, .vfb = 6
		, .hact = 1080, .hbb = 148, .hfb = 88
		, .vpw = 16, .hpw = 64, .idv = 0, .ihs = 0, .ivs = 1},
	[VO_OUTPUT_480x800_60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 800, .vbb = 20, .vfb = 20
		, .hact = 480, .hbb = 50, .hfb = 50
		, .vpw = 10, .hpw = 10, .idv = 0, .ihs = 0, .ivs = 1},
	[VO_OUTPUT_1440P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 1440, .vbb = 33, .vfb = 3
		, .hact = 2560, .hbb = 80, .hfb = 32
		, .vpw = 5, .hpw = 48, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P24] = {.synm = 1, .iop = 1, .frame_rate = 24
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 1276
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P25] = {.synm = 1, .iop = 1, .frame_rate = 25
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 1056
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P30] = {.synm = 1, .iop = 1, .frame_rate = 30
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 176
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 1056
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_2160P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 3840, .hbb = 296, .hfb = 176
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 0},
	[VO_OUTPUT_4096x2160P24] = {.synm = 1, .iop = 1, .frame_rate = 24
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 296, .hfb = 1020
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
	[VO_OUTPUT_4096x2160P25] = {.synm = 1, .iop = 1, .frame_rate = 25
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 128, .hfb = 968
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
	[VO_OUTPUT_4096x2160P30] = {.synm = 1, .iop = 1, .frame_rate = 30
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 128, .hfb = 88
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
	[VO_OUTPUT_4096x2160P50] = {.synm = 1, .iop = 1, .frame_rate = 50
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 128, .hfb = 968
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
	[VO_OUTPUT_4096x2160P60] = {.synm = 1, .iop = 1, .frame_rate = 60
		, .vact = 2160, .vbb = 72, .vfb = 8
		, .hact = 4096, .hbb = 128, .hfb = 88
		, .vpw = 10, .hpw = 88, .idv = 0, .ihs = 1, .ivs = 1},
};

/****************************************************************************
 * SDK layer Defines
 ****************************************************************************/
#define DEFAULT_MESH_PADDR	0x80000000

#define VO_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY))

#define GDC_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||           \
	 (fmt == PIXEL_FORMAT_YUV_400))


/****************************************************************************
 *internal APIs
 ****************************************************************************/
static int vo_get_panelstatus(vo_dev dev, u32 *is_init)
{
	if (disp_mux_get(dev) == DISP_VO_SEL_I80)
		*is_init = disp_check_i80_enable(dev);
	else
		*is_init = disp_check_tgen_enable(dev);

	return 0;
}

struct vo_fmt *vo_sdk_get_format(u32 pixelformat)
{
	struct vo_fmt *fmt;
	u32 k;

	for (k = 0; k < ARRAY_SIZE(vo_sdk_formats); k++) {
		fmt = &vo_sdk_formats[k];
		if (fmt->fourcc == pixelformat)
			return fmt;
	}

	return NULL;
}

static void _vo_sdk_fill_disp_cfg(struct disp_cfg *cfg,
		struct vo_video_format *video_format)
{
	struct vo_fmt *vo_sdk_fmt;
	struct vo_plane_info *plane_info = video_format->plane_info;

	vo_sdk_fmt = vo_sdk_get_format(video_format->pixelformat);

	cfg->fmt = vo_sdk_fmt->fmt;

	if (video_format->colorspace == VO_COLORSPACE_SRGB)
		cfg->in_csc = DISP_CSC_NONE;
	else if (video_format->colorspace == VO_COLORSPACE_SMPTE170M)
		cfg->in_csc = DISP_CSC_601_FULL_YUV2RGB;
	else
		cfg->in_csc = DISP_CSC_709_FULL_YUV2RGB;

	TRACE_VO(DBG_DEBUG, "bytesperline 0(%d))\n", plane_info[0].bytesperline);
	TRACE_VO(DBG_DEBUG, "bytesperline 1(%d))\n", plane_info[1].bytesperline);
	cfg->mem.pitch_y = plane_info[0].bytesperline;
	cfg->mem.pitch_c = plane_info[1].bytesperline;

	TRACE_VO(DBG_DEBUG, " width(%d), heigh(%d)\n", video_format->width, video_format->height);
	cfg->mem.width = video_format->width;
	cfg->mem.height = video_format->height;
	cfg->mem.start_x = 0;
	cfg->mem.start_y = 0;
}

static int _vo_sdk_setfmt(int width, int height, u32 pxlfmt, vo_dev dev)
{
	int p = 0;
	struct vo_video_format video_fmt;
	const struct vo_fmt *vo_sdk_fmt;
	struct disp_cfg *cfg;
	u32 bytesperline;

	memset(&video_fmt, 0, sizeof(struct vo_video_format));

	video_fmt.width = width;
	video_fmt.height = height;
	video_fmt.pixelformat = pxlfmt;
	video_fmt.field = 0;

	switch (pxlfmt) {
	case PIXEL_FORMAT_HSV_888_PLANAR:
	case PIXEL_FORMAT_YUV_PLANAR_420:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_VYUY:
		video_fmt.colorspace = VO_COLORSPACE_SMPTE170M;
		break;
	default:
		break;
	}
	switch (pxlfmt) {
	default:
	case PIXEL_FORMAT_HSV_888_PLANAR:
	case PIXEL_FORMAT_YUV_PLANAR_420:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
		video_fmt.num_planes = 3;
		break;
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
		video_fmt.num_planes = 2;
		break;
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_VYUY:
		video_fmt.num_planes = 1;
		break;
	}

	vo_sdk_fmt = vo_sdk_get_format(pxlfmt);

	for (p = 0; p < video_fmt.num_planes; p++) {
		u8 plane_sub_v = (p == 0) ? 1 : vo_sdk_fmt->plane_sub_v;
		/* Calculate the minimum supported bytesperline value */
		bytesperline = ALIGN((video_fmt.width * vo_sdk_fmt->bit_depth[p]) >> 3, DISP_ALIGNMENT);

		if (video_fmt.plane_info[p].bytesperline < bytesperline)
			video_fmt.plane_info[p].bytesperline = bytesperline;

		video_fmt.plane_info[p].sizeimage = video_fmt.plane_info[p].bytesperline
		* video_fmt.height / plane_sub_v;

		TRACE_VO(DBG_DEBUG, "plane-%d: bytesperline(%d) sizeimage(%x)\n", p,
			 video_fmt.plane_info[p].bytesperline, video_fmt.plane_info[p].sizeimage);
	}

	cfg = disp_get_cfg(dev);
	_vo_sdk_fill_disp_cfg(cfg, &video_fmt);
	disp_set_bw_cfg(dev, cfg->fmt);
	disp_set_cfg(dev, cfg);

	return 0;
}

static void _release_buffer(struct vo_layer_ctx *layer_ctx, struct list_head *head)
{
	unsigned long flags;
	struct disp_buffer *b = NULL;

	while (!list_empty(head)) {
		spin_lock_irqsave(&layer_ctx->list_lock, flags);
		b = list_first_entry(head,
			struct disp_buffer, list);
		list_del_init(&b->list);
		spin_unlock_irqrestore(&layer_ctx->list_lock, flags);

		if (b == NULL)
			return;

		if (b->blk != VB_INVALID_HANDLE)
			vb_release_block(b->blk);

		TRACE_VO(DBG_DEBUG, "relase vb(0x%llx).\n", b->buf.planes[0].addr);
		vfree(b);
		b = NULL;
	}
}

/****************************************************************************
 * SDK device APIs
 ****************************************************************************/
static int vo_set_pub_attr(vo_dev dev, vo_pub_attr_s *pub_attr)
{
	struct vo_dv_timings dv_timings;
	u16 rgb[3];
	u32 panel_status = 0;
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->is_dev_enable) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) should be disabled.\n", dev);
		return ERR_VO_DEV_HAS_ENABLED;
	}

	if (pub_attr->intf_sync == VO_OUTPUT_USER) {
		dv_timings.bt.interlaced = !pub_attr->sync_info.iop;
		dv_timings.bt.height = pub_attr->sync_info.vact << dv_timings.bt.interlaced;
		dv_timings.bt.vbackporch = pub_attr->sync_info.vbb;
		dv_timings.bt.vfrontporch = pub_attr->sync_info.vfb;
		dv_timings.bt.width = pub_attr->sync_info.hact;
		dv_timings.bt.hbackporch = pub_attr->sync_info.hbb;
		dv_timings.bt.hfrontporch = pub_attr->sync_info.hfb;
		dv_timings.bt.il_vbackporch = 0;
		dv_timings.bt.il_vfrontporch = 0;
		dv_timings.bt.il_vsync = 0;
		dv_timings.bt.hsync = pub_attr->sync_info.hpw;
		dv_timings.bt.vsync = pub_attr->sync_info.vpw;
		dv_timings.bt.polarities = ((pub_attr->sync_info.ivs) ? 0 : 0x1)
					| ((pub_attr->sync_info.ihs) ? 0 : 0x2);
		dv_timings.bt.pixelclock = pub_attr->sync_info.frame_rate
					* (dv_timings.bt.vbackporch + dv_timings.bt.height
					   + dv_timings.bt.vfrontporch + dv_timings.bt.vsync)
					* (dv_timings.bt.hbackporch + dv_timings.bt.width
					   + dv_timings.bt.hfrontporch + dv_timings.bt.hsync);
	} else if (pub_attr->intf_sync < VO_OUTPUT_USER) {
		dv_timings.bt.interlaced = !sync_info[pub_attr->intf_sync].iop;
		dv_timings.bt.height = sync_info[pub_attr->intf_sync].vact << dv_timings.bt.interlaced;
		dv_timings.bt.vbackporch = sync_info[pub_attr->intf_sync].vbb;
		dv_timings.bt.vfrontporch = sync_info[pub_attr->intf_sync].vfb;
		dv_timings.bt.width = sync_info[pub_attr->intf_sync].hact;
		dv_timings.bt.hbackporch = sync_info[pub_attr->intf_sync].hbb;
		dv_timings.bt.hfrontporch = sync_info[pub_attr->intf_sync].hfb;
		dv_timings.bt.il_vbackporch = 0;
		dv_timings.bt.il_vfrontporch = 0;
		dv_timings.bt.il_vsync = 0;
		dv_timings.bt.hsync = sync_info[pub_attr->intf_sync].hpw;
		dv_timings.bt.vsync = sync_info[pub_attr->intf_sync].vpw;
		dv_timings.bt.polarities = ((sync_info[pub_attr->intf_sync].ivs) ? 0 : 0x1)
					| ((sync_info[pub_attr->intf_sync].ihs) ? 0 : 0x2);
		dv_timings.bt.pixelclock = sync_info[pub_attr->intf_sync].frame_rate
					* (dv_timings.bt.vbackporch + dv_timings.bt.height
					   + dv_timings.bt.vfrontporch + dv_timings.bt.vsync)
					* (dv_timings.bt.hbackporch + dv_timings.bt.width
					   + dv_timings.bt.hfrontporch + dv_timings.bt.hsync);
		pub_attr->sync_info = sync_info[pub_attr->intf_sync];
	} else {
		TRACE_VO(DBG_ERR, "VO Sync Info(%d) invalid.\n", pub_attr->intf_sync);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (dv_timings.bt.interlaced) {
		TRACE_VO(DBG_ERR, "VO not support interlaced timing.\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	if ((dv_timings.bt.pixelclock == 0) || (dv_timings.bt.height == 0) || (dv_timings.bt.width == 0)) {
		TRACE_VO(DBG_ERR, "VO Sync timing invalid. width(%d) height(%d) pixelclock(%llu)\n",
			 dv_timings.bt.width, dv_timings.bt.height, dv_timings.bt.pixelclock);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (pub_attr->intf_type >= VO_INTF_BUTT) {
		TRACE_VO(DBG_ERR, "VO invalid INTF type(0x%x)\n", pub_attr->intf_type);
		return ERR_VO_ILLEGAL_PARAM;
	}

	vo_get_panelstatus(dev, &panel_status);
	TRACE_VO(DBG_INFO, "panel_status[%d], intf_type[%d]\n", panel_status, pub_attr->intf_type);

	if (pub_attr->intf_type != VO_INTF_MIPI && pub_attr->intf_type != VO_INTF_HDMI && !panel_status) {
		struct disp_timing timing;
		vo_fill_disp_timing(&timing, &dv_timings.bt);
		disp_set_timing(dev, &timing);
	}

	rgb[2] = pub_attr->bgcolor & 0x3ff;
	rgb[1] = (pub_attr->bgcolor >> 10) & 0x3ff;
	rgb[0] = (pub_attr->bgcolor >> 20) & 0x3ff;

	disp_set_frame_bgcolor(dev, rgb[0], rgb[1], rgb[2]);

	memcpy(&dev_ctx->pub_attr, pub_attr, sizeof(*pub_attr));

	return 0;
}

static int vo_get_pub_attr(vo_dev dev, vo_pub_attr_s *pub_attr)
{
	enum disp_vo_sel vo_sel;
	struct disp_timing *timing = disp_get_timing(dev);
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	dev_ctx->pub_attr.sync_info.hact = timing->hfde_end - timing->hfde_start + 1;
	dev_ctx->pub_attr.sync_info.vact = timing->vfde_end - timing->vfde_start + 1;

	vo_sel = disp_mux_get(dev);

	switch (vo_sel) {
	case DISP_VO_SEL_RGB:
		dev_ctx->pub_attr.intf_type = VO_INTF_PARALLEL_RGB;
		break;

	case DISP_VO_SEL_SERIAL_RGB:
		dev_ctx->pub_attr.intf_type = VO_INTF_SERIAL_RGB;
		break;

	case DISP_VO_SEL_HW_MCU:
		dev_ctx->pub_attr.intf_type = VO_INTF_HW_MCU;
		break;

	case DISP_VO_SEL_BT656:
		dev_ctx->pub_attr.intf_type = VO_INTF_BT656;
		break;

	case DISP_VO_SEL_BT1120:
		dev_ctx->pub_attr.intf_type = VO_INTF_BT1120;
		break;

	default:
		if (dphy_get_dsi_clk_lane_status(dev)) {
			if (dphy_is_lvds(dev))
				dev_ctx->pub_attr.intf_type = VO_INTF_LVDS;
			else
				dev_ctx->pub_attr.intf_type = VO_INTF_MIPI;
		} else {
			dev_ctx->pub_attr.intf_type = VO_INTF_HDMI;
		}
		break;
	}

	memcpy(pub_attr, &dev_ctx->pub_attr, sizeof(vo_pub_attr_s));

	return 0;
}

static int vo_set_lvds_param(vo_dev dev, vo_lvds_attr_s *lvds_param)
{
	struct vo_dev_ctx *dev_ctx;
	struct vo_disp_intf_cfg cfg;
	struct vo_dv_timings dv_timings;
	u16 i;
	int ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_sync == VO_OUTPUT_USER) {
		dv_timings.bt.pixelclock =
			dev_ctx->pub_attr.sync_info.frame_rate *
			(dev_ctx->pub_attr.sync_info.vbb +
			(dev_ctx->pub_attr.sync_info.vact <<
			!dev_ctx->pub_attr.sync_info.iop) +
			dev_ctx->pub_attr.sync_info.vfb +
			dev_ctx->pub_attr.sync_info.vpw) *
			(dev_ctx->pub_attr.sync_info.hbb +
			dev_ctx->pub_attr.sync_info.hact +
			dev_ctx->pub_attr.sync_info.hfb +
			dev_ctx->pub_attr.sync_info.hpw);
	} else if (dev_ctx->pub_attr.intf_sync < VO_OUTPUT_USER) {
		dv_timings.bt.pixelclock =
			sync_info[dev_ctx->pub_attr.intf_sync].frame_rate *
			(sync_info[dev_ctx->pub_attr.intf_sync].vbb +
			(sync_info[dev_ctx->pub_attr.intf_sync].vact <<
			!sync_info[dev_ctx->pub_attr.intf_sync].iop) +
			sync_info[dev_ctx->pub_attr.intf_sync].vfb +
			sync_info[dev_ctx->pub_attr.intf_sync].vpw) *
			(sync_info[dev_ctx->pub_attr.intf_sync].hbb +
			sync_info[dev_ctx->pub_attr.intf_sync].hact +
			sync_info[dev_ctx->pub_attr.intf_sync].hfb +
			sync_info[dev_ctx->pub_attr.intf_sync].hpw);
	}

	if (dev_ctx->pub_attr.intf_type == VO_INTF_LVDS) {
		cfg.intf_type = VO_DISP_INTF_LVDS;
		cfg.lvds_cfg.out_bits = lvds_param->out_bits;
		cfg.lvds_cfg.mode = lvds_param->lvds_vesa_mode;
		cfg.lvds_cfg.chn_num = lvds_param->chn_num;
		if (cfg.lvds_cfg.chn_num > 1) {
			TRACE_VO(DBG_ERR, "lvds only surpports single link!\n");
			return ERR_VO_ILLEGAL_PARAM;
		}
		cfg.lvds_cfg.vs_out_en = 1;
		cfg.lvds_cfg.hs_out_en = 1;
		cfg.lvds_cfg.hs_blk_en = 1;
		cfg.lvds_cfg.msb_lsb_data_swap = 1;
		cfg.lvds_cfg.serial_msb_first = lvds_param->data_big_endian;
		cfg.lvds_cfg.even_odd_link_swap = 0;
		cfg.lvds_cfg.enable = 1;

		do_div(dv_timings.bt.pixelclock, 1000);
		cfg.lvds_cfg.pixelclock = dv_timings.bt.pixelclock;

		for (i = 0; i < VO_LVDS_LANE_MAX; ++i) {
			cfg.lvds_cfg.lane_id[i] = lvds_param->lane_id[i];
			cfg.lvds_cfg.lane_pn_swap[i] = lvds_param->lane_pn_swap[i];
		}

		if (vo_set_interface(dev, &cfg) != 0) {
			TRACE_VO(DBG_ERR, "VO INTF configure failured.\n");
			return -1;
		}
	} else {
		TRACE_VO(DBG_ERR, "not working under th lvds interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	memcpy(&dev_ctx->lvds_param, lvds_param, sizeof(*lvds_param));

	return 0;
}

static int vo_get_lvds_param(vo_dev dev, vo_lvds_attr_s *lvds_param)
{
	struct vo_dev_ctx *dev_ctx;
	int ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_type != VO_INTF_LVDS) {
		TRACE_VO(DBG_ERR, "not working under th lvds interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	memcpy(lvds_param, &dev_ctx->lvds_param, sizeof(*lvds_param));

	return 0;
}

static int vo_set_bt_param(vo_dev dev, vo_bt_attr_s *bt_param)
{
	struct vo_dev_ctx *dev_ctx;
	struct vo_disp_intf_cfg cfg;
	struct vo_dv_timings dv_timings;
	int ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_sync == VO_OUTPUT_USER) {
		dv_timings.bt.pixelclock =
			dev_ctx->pub_attr.sync_info.frame_rate *
			(dev_ctx->pub_attr.sync_info.vbb +
			(dev_ctx->pub_attr.sync_info.vact <<
			!dev_ctx->pub_attr.sync_info.iop) +
			dev_ctx->pub_attr.sync_info.vfb +
			dev_ctx->pub_attr.sync_info.vpw) *
			(dev_ctx->pub_attr.sync_info.hbb +
			dev_ctx->pub_attr.sync_info.hact +
			dev_ctx->pub_attr.sync_info.hfb +
			dev_ctx->pub_attr.sync_info.hpw);
	} else if (dev_ctx->pub_attr.intf_sync < VO_OUTPUT_USER) {
		dv_timings.bt.pixelclock =
			sync_info[dev_ctx->pub_attr.intf_sync].frame_rate *
			(sync_info[dev_ctx->pub_attr.intf_sync].vbb +
			(sync_info[dev_ctx->pub_attr.intf_sync].vact <<
			!sync_info[dev_ctx->pub_attr.intf_sync].iop) +
			sync_info[dev_ctx->pub_attr.intf_sync].vfb +
			sync_info[dev_ctx->pub_attr.intf_sync].vpw) *
			(sync_info[dev_ctx->pub_attr.intf_sync].hbb +
			sync_info[dev_ctx->pub_attr.intf_sync].hact +
			sync_info[dev_ctx->pub_attr.intf_sync].hfb +
			sync_info[dev_ctx->pub_attr.intf_sync].hpw);
	}

	if (dev_ctx->pub_attr.intf_type == VO_INTF_BT656 ||
		dev_ctx->pub_attr.intf_type == VO_INTF_BT1120) {
		cfg.bt_cfg.data_seq = bt_param->data_seq;
		cfg.bt_cfg.bt_clk_inv= bt_param->bt_clk_inv;
		cfg.bt_cfg.bt_vs_inv= bt_param->bt_vs_inv;
		cfg.bt_cfg.bt_hs_inv= bt_param->bt_hs_inv;
		do_div(dv_timings.bt.pixelclock, 1000);
		cfg.bt_cfg.pixelclock = dv_timings.bt.pixelclock;
		cfg.bt_cfg.pins.pin_num = bt_param->pin_num;
		memcpy(&cfg.bt_cfg.pins.d_pins, bt_param->d_pins, sizeof(bt_param->d_pins));

		if(dev_ctx->pub_attr.intf_type == VO_INTF_BT656) {
			cfg.bt_cfg.mode = BT_MODE_656;
			cfg.intf_type = VO_DISP_INTF_BT656;
		} else if (dev_ctx->pub_attr.intf_type == VO_INTF_BT1120) {
			cfg.bt_cfg.mode = BT_MODE_1120;
			cfg.intf_type = VO_DISP_INTF_BT1120;
		}

		if (vo_set_interface(dev, &cfg) != 0) {
			TRACE_VO(DBG_ERR, "VO INTF configure failured.\n");
			return -1;
		}
	} else {
		TRACE_VO(DBG_ERR, "not working under th bt interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	memcpy(&dev_ctx->bt_param, bt_param, sizeof(*bt_param));

	return 0;
}

static int vo_get_bt_param(vo_dev dev, vo_bt_attr_s *bt_param)
{
	struct vo_dev_ctx *dev_ctx;
	int ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_type != VO_INTF_BT656 &&
		dev_ctx->pub_attr.intf_type != VO_INTF_BT1120) {
		TRACE_VO(DBG_ERR, "not working under th bt interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	memcpy(bt_param, &dev_ctx->bt_param, sizeof(*bt_param));

	return 0;
}

static int vo_set_hdmi_param(vo_dev dev, vo_hdmi_param_s *hdmi_param)
{
	struct vo_dev_ctx *dev_ctx;

	if (dev != VO_HDMI_DEVICE) {
		TRACE_VO(DBG_ERR, "Only device 1 has hdmi interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_type != VO_INTF_HDMI) {
		TRACE_VO(DBG_ERR, "not working under th hdmi interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	if ((hdmi_param->hdmi_csc.csc_matrix >= VO_CSC_MATRIX_601_LIMIT_RGB2YUV &&
	     hdmi_param->hdmi_csc.csc_matrix <= VO_CSC_MATRIX_709_FULL_RGB2YUV) ||
	     hdmi_param->hdmi_csc.csc_matrix == VO_CSC_MATRIX_IDENTITY) {
		disp_set_out_csc(dev, (enum disp_csc)hdmi_param->hdmi_csc.csc_matrix);
	} else {
		TRACE_VO(DBG_ERR, "dev(%d) HDMI CscMatrix(%d) invalid.\n", dev, hdmi_param->hdmi_csc.csc_matrix);
		return ERR_VO_ILLEGAL_PARAM;
	}

	return 0;
}

static int vo_get_hdmi_param(vo_dev dev, vo_hdmi_param_s *hdmi_param)
{
	struct disp_cfg *disp_cfg;
	struct vo_dev_ctx *dev_ctx;

	if (dev != VO_HDMI_DEVICE) {
		TRACE_VO(DBG_ERR, "Only device 1 has hdmi interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	if (dev_ctx->pub_attr.intf_type != VO_INTF_HDMI) {
		TRACE_VO(DBG_ERR, "not working under th hdmi interface!\n");
		return ERR_VO_ILLEGAL_PARAM;
	}

	disp_cfg = disp_get_cfg(dev);
	hdmi_param->hdmi_csc.csc_matrix = (vo_csc_matrix_e)disp_cfg->out_csc;

	return 0;
}

static int vo_enable(vo_dev dev)
{
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];
	if (dev_ctx->pub_attr.intf_type == 0) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) isn't correctly configured.\n", dev);
		return ERR_VO_DEV_NOT_CONFIG;
	}
	if (dev_ctx->bind_layer_id == -1) {
		TRACE_VO(DBG_DEBUG, "dev(%d) unbind layer", dev);
		return ERR_VO_SYS_NOTREADY;
	}

	if (dev_ctx->is_dev_enable) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) should be disabled.\n", dev);
		return ERR_VO_DEV_HAS_ENABLED;
	}

	dev_ctx->is_dev_enable = true;

	if (vo_start_streaming(dev)) {
		TRACE_VO(DBG_ERR, "Failed to vo start streaming\n");
		return ERR_VO_SYS_NOTREADY;
	}

	return 0;

}

int vo_disable(vo_dev dev)
{
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];
	if (!dev_ctx->is_dev_enable) {
		TRACE_VO(DBG_ERR, "vo_dev(%d) already disabled.\n", dev);
		return ERR_VO_DEV_NOT_ENABLED;
	}

	dev_ctx->is_dev_enable = false;

	if (vo_stop_streaming(dev)) {
		TRACE_VO(DBG_ERR, "Failed to vo stop streaming\n");
		return ERR_VO_SYS_NOTREADY;
	}

	return 0;
}


/****************************************************************************
 * SDK layer APIs
 ****************************************************************************/
static int vo_set_displaybuflen(vo_layer layer, u32 buflen)
{
	int ret = -1;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	g_vo_ctx->layer_ctx[layer].display_buflen = buflen;
	mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);

	return 0;
}

static int vo_get_displaybuflen(vo_layer layer, u32 *buflen)
{
	int ret = -1;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	*buflen = g_vo_ctx->layer_ctx[layer].display_buflen;

	return 0;
}

static int vo_enablevideolayer(vo_layer layer)
{
	int ret = -1;
	int i;
	struct vo_layer_ctx *layer_ctx;
	vb_cal_config_s vb_cal_config;
	vb_blk blk = VB_INVALID_HANDLE;
	struct disp_buffer *disp_buf;
	unsigned long flags;
	vo_dev dev;
	struct vb_s *vb;
	mmf_chn_s chn;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_disable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (layer_ctx->bind_dev_id == -1) {
		TRACE_VO(DBG_DEBUG, "layer(%d) unbind device", layer);
		return ERR_VO_SYS_NOTREADY;
	}

	dev = layer_ctx->bind_dev_id;
	chn.mod_id = ID_VO;
	chn.dev_id = dev;
	chn.chn_id = 0;

	common_getpicbufferconfig(layer_ctx->layer_attr.img_size.width,
				  layer_ctx->layer_attr.img_size.height,
				  layer_ctx->layer_attr.pixformat, DATA_BITWIDTH_8,
				  COMPRESS_MODE_NONE, DEFAULT_ALIGN, &vb_cal_config);

	base_mod_jobs_init(&layer_ctx->layer_jobs, 0, 0, layer_ctx->layer_attr.depth);

	for (i = 0; i < layer_ctx->display_buflen; i++) {
		blk = vb_get_block_with_id(VB_INVALID_POOLID, vb_cal_config.vb_size, ID_VO);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_VO(DBG_ERR, "get vb block fail.\n");
			ret = ERR_VO_NO_MEM;
			goto err;
		}

		vb = (struct vb_s *)blk;
		base_get_frame_info(layer_ctx->layer_attr.pixformat
					, layer_ctx->layer_attr.img_size
					, &vb->buf
					, vb_handle2phys_addr(blk)
					, DEFAULT_ALIGN);

		vb->buf.offset_top = 0;
		vb->buf.offset_right = 0;
		vb->buf.offset_left = 0;
		vb->buf.offset_bottom = 0;

		disp_buf = vzalloc(sizeof(*disp_buf));
		if (!disp_buf) {
			TRACE_VO(DBG_ERR, "vzalloc size(%zu) fail\n", sizeof(struct disp_buffer));
			vb_release_block(blk);
			ret = ERR_VO_NO_MEM;
			goto err;
		}

		disp_buf->buf.length = 3;
		disp_buf->buf.index  = i;
		disp_buf->sequence = i;
		disp_buf->blk = blk;

		spin_lock_irqsave(&layer_ctx->list_lock, flags);
		list_add_tail(&disp_buf->list, &layer_ctx->list_done);
		spin_unlock_irqrestore(&layer_ctx->list_lock, flags);
	}


	mutex_lock(&layer_ctx->layer_lock);
	layer_ctx->is_layer_enable = true;
	layer_ctx->event = 0;
	init_waitqueue_head(&layer_ctx->wq);
	mutex_unlock(&layer_ctx->layer_lock);

	ret = vo_create_thread(layer);
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to create thread, layer(%d).\n", layer);
		goto err;
	}

	return 0;

err:
	base_mod_jobs_exit(&layer_ctx->layer_jobs);
	_release_buffer(layer_ctx, &layer_ctx->list_done);

	return ret;
}

int vo_disablevideolayer(vo_layer layer)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (!layer_ctx->is_layer_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) isn't enabled yet.\n", layer);
		return ERR_VO_VIDEO_NOT_ENABLED;
	}

	mutex_lock(&layer_ctx->layer_lock);
	layer_ctx->is_layer_enable = false;
	layer_ctx->frame_num = 0;
	layer_ctx->frame_rate = 0;
	layer_ctx->src_frame_num = 0;
	layer_ctx->src_frame_rate = 0;
	layer_ctx->frame_index = 0;
	layer_ctx->display_pts = 0;
	layer_ctx->predone_pts = 0;
	layer_ctx->bw_fail = 0;
	layer_ctx->vgop_bw_fail = 0;
	mutex_unlock(&layer_ctx->layer_lock);

	vo_destroy_thread(layer);

	base_mod_jobs_exit(&layer_ctx->layer_jobs);

	_release_buffer(layer_ctx, &layer_ctx->list_done);
	_release_buffer(layer_ctx, &layer_ctx->list_work);
	_release_buffer(layer_ctx, &layer_ctx->list_wait);

	return 0;
}

static void vo_sort_layer_priority(int *priority, u32 length, int *overlay_id)
{
	int i, j, t1, t2;

	for (j = 0; j < length; j++) {
		for (i = 0; i < (length - 1 - j); i++) {
			if (overlay_id[i] > overlay_id[i + 1] && (overlay_id[i + 1] != -1)) {
				t2 = priority[i];
				priority[i] = priority[i + 1];
				priority[i + 1] = t2;
				t1 = overlay_id[i];
				overlay_id[i] = overlay_id[i + 1];
				overlay_id[i + 1] = t1;
			}
			if (priority[i] > priority[i + 1]) {
				t2 = priority[i];
				priority[i] = priority[i + 1];
				priority[i + 1] = t2;
				t1 = overlay_id[i];
				overlay_id[i] = overlay_id[i + 1];
				overlay_id[i + 1] = t1;
			}
		}
	}
}

static int vo_bind_layer(vo_layer layer, vo_dev dev)
{
	int i, ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (layer >= 0 && layer < VO_MAX_VIDEO_LAYER_NUM) {
		ret = check_video_layer_disable(layer);
		if (ret != 0)
			return ret;

		if (g_vo_ctx->dev_ctx[dev].bind_layer_id != -1) {
			TRACE_VO(DBG_ERR, "dev(%d) already bind with layer(%d).",
				 dev, layer);
			return ERR_VO_DEV_HAS_BINDED;
		}

		mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
		g_vo_ctx->dev_ctx[dev].bind_layer_id = layer;
		g_vo_ctx->layer_ctx[layer].bind_dev_id = dev;
		mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	} else if ((layer >= VO_MAX_VIDEO_LAYER_NUM) &&
		(layer < VO_MAX_LAYER_NUM)) {

		vo_layer overlay = layer - VO_MAX_VIDEO_LAYER_NUM;
		u8 bind_overlay_num = 0;
		int priority[VO_MAX_GRAPHIC_LAYER_IN_DEV];
		int overlay_id[VO_MAX_GRAPHIC_LAYER_IN_DEV];
		int bind_overlay_id_next[VO_MAX_GRAPHIC_LAYER_IN_DEV];

		if (g_vo_ctx->overlay_ctx[overlay].bind_dev_id != -1) {
			TRACE_VO(DBG_ERR, "layer(%d) already bind on dev(%d)."
				 "please unbind first.\n",
				 layer, g_vo_ctx->overlay_ctx[overlay].bind_dev_id);
			return ERR_VO_DEV_HAS_BINDED;
		}

		for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i) {
			if (g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] == -1) {
				TRACE_VO(DBG_ERR, "overlay(%d) invalid.\n", overlay);
				mutex_lock(&g_vo_ctx->dev_ctx[dev].dev_lock);
				g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] = layer;
				g_vo_ctx->overlay_ctx[overlay].bind_dev_id = dev;
				mutex_unlock(&g_vo_ctx->dev_ctx[dev].dev_lock);
				break;
			} else {
				bind_overlay_num++;
			}
		}

		if (bind_overlay_num == VO_MAX_GRAPHIC_LAYER_IN_DEV) {
			TRACE_VO(DBG_ERR, "dev(%d) already bind %d overlays.\n"
				 "please unbind first.\n", dev, VO_MAX_GRAPHIC_LAYER_IN_DEV);
			return ERR_VO_INVALID_LAYERID;
		}

		for (bind_overlay_num = 0, i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i) {
			if (g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] != -1) {
				overlay = g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] - VO_MAX_VIDEO_LAYER_NUM;
				TRACE_VO(DBG_ERR, "overlay(%d) invalid.\n", overlay);
				mutex_lock(&g_vo_ctx->dev_ctx[dev].dev_lock);
				priority[bind_overlay_num] = g_vo_ctx->overlay_ctx[overlay].priority;
				overlay_id[bind_overlay_num] = g_vo_ctx->dev_ctx[dev].bind_overlay_id[i];
				mutex_unlock(&g_vo_ctx->dev_ctx[dev].dev_lock);
				bind_overlay_num++;
			}
		}

		vo_sort_layer_priority(priority, bind_overlay_num, overlay_id);

		for (i = 0; i < bind_overlay_num; ++i) {
			bind_overlay_id_next[i] = overlay_id[i];
			TRACE_VO(DBG_ERR, "overlay_id(%d).\n", overlay_id[i]);
		}

		mutex_lock(&g_vo_ctx->dev_ctx[dev].dev_lock);
		for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i) {
			if (i < bind_overlay_num)
				g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] = bind_overlay_id_next[i];
			else
				g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] = -1;
		}
		mutex_unlock(&g_vo_ctx->dev_ctx[dev].dev_lock);

		for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i)
			TRACE_VO(DBG_ERR, "dev(%d) bind layer(%d).\n",
				 dev, g_vo_ctx->dev_ctx[dev].bind_overlay_id[i]);
	} else {
		TRACE_VO(DBG_ERR, "layer(%d) invalid.\n", layer);
		return ERR_VO_INVALID_LAYERID;
	}

	return 0;
}

static int vo_unbind_layer(vo_layer layer, vo_dev dev)
{
	int i, ret = -1;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (layer >= 0 && layer < VO_MAX_VIDEO_LAYER_NUM) {
		ret = check_video_layer_disable(layer);
		if (ret != 0)
			return ret;

		if (g_vo_ctx->dev_ctx[dev].bind_layer_id != layer ||
		    g_vo_ctx->layer_ctx[layer].bind_dev_id != dev) {
			TRACE_VO(DBG_ERR, "layer(%d) not bind on dev(%d).",
				 layer, dev);
			return ERR_VO_DEV_NOT_BINDED;
		}

		mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
		g_vo_ctx->dev_ctx[dev].bind_layer_id = -1;
		g_vo_ctx->layer_ctx[layer].bind_dev_id = -1;
		mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	} else if ((layer >= VO_MAX_VIDEO_LAYER_NUM) &&
		(layer < VO_MAX_LAYER_NUM)) {

		vo_layer overlay = layer - VO_MAX_VIDEO_LAYER_NUM;

		if (g_vo_ctx->overlay_ctx[overlay].bind_dev_id != dev) {
			TRACE_VO(DBG_ERR, "layer(%d) not bind on dev(%d).",
				 layer, dev);
			return ERR_VO_DEV_NOT_BINDED;
		}

		for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i) {
			if (g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] == layer) {
				mutex_lock(&g_vo_ctx->dev_ctx[dev].dev_lock);
				g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] = -1;
				g_vo_ctx->overlay_ctx[overlay].bind_dev_id = -1;
				mutex_unlock(&g_vo_ctx->dev_ctx[dev].dev_lock);
				break;
			}
		}
	} else {
		TRACE_VO(DBG_ERR, "layer(%d) invalid.\n", layer);
		return ERR_VO_INVALID_LAYERID;
	}

	return 0;
}

static int vo_set_layer_priority(vo_layer layer, u32 value)
{
	int i;

	if (layer >= VO_MAX_VIDEO_LAYER_NUM &&
	    layer < VO_MAX_LAYER_NUM) {

		vo_layer overlay = layer - VO_MAX_VIDEO_LAYER_NUM;
		vo_dev dev = g_vo_ctx->overlay_ctx[overlay].bind_dev_id;
		int priority[VO_MAX_GRAPHIC_LAYER_IN_DEV];
		int overlay_id[VO_MAX_GRAPHIC_LAYER_IN_DEV];
		int bind_overlay_id_next[VO_MAX_GRAPHIC_LAYER_IN_DEV];
		u8 bind_overlay_num = 0;

		g_vo_ctx->overlay_ctx[overlay].priority = value;

		if (dev != -1) {
			for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i) {
				if (g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] != -1) {
					overlay = g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] - VO_MAX_VIDEO_LAYER_NUM;
					mutex_lock(&g_vo_ctx->dev_ctx[dev].dev_lock);
					priority[bind_overlay_num] = g_vo_ctx->overlay_ctx[overlay].priority;
					overlay_id[bind_overlay_num] = g_vo_ctx->dev_ctx[dev].bind_overlay_id[i];
					mutex_unlock(&g_vo_ctx->dev_ctx[dev].dev_lock);
					bind_overlay_num++;
				}
			}

			vo_sort_layer_priority(priority, bind_overlay_num, overlay_id);

			for (i = 0; i < bind_overlay_num; ++i) {
				bind_overlay_id_next[i] = overlay_id[i];
			}

			mutex_lock(&g_vo_ctx->dev_ctx[dev].dev_lock);
			for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i) {
				if (i < bind_overlay_num)
					g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] = bind_overlay_id_next[i];
				else
					g_vo_ctx->dev_ctx[dev].bind_overlay_id[i] = -1;
			}
			mutex_unlock(&g_vo_ctx->dev_ctx[dev].dev_lock);
		}
	} else {
		TRACE_VO(DBG_ERR, "layer(%d) invalid.\n", layer);
		return ERR_VO_INVALID_LAYERID;
	}

	return 0;
}

static int vo_get_layer_priority(vo_layer layer, u32 *priority)
{
	if (layer >= VO_MAX_VIDEO_LAYER_NUM &&
	    layer < VO_MAX_LAYER_NUM) {
		vo_layer overlay = layer - VO_MAX_VIDEO_LAYER_NUM;
		vo_dev dev = g_vo_ctx->overlay_ctx[overlay].bind_dev_id;

		mutex_lock(&g_vo_ctx->dev_ctx[dev].dev_lock);
		*priority = g_vo_ctx->overlay_ctx[overlay].priority;
		mutex_unlock(&g_vo_ctx->dev_ctx[dev].dev_lock);
	} else {
		TRACE_VO(DBG_ERR, "layer(%d) invalid.\n", layer);
		return ERR_VO_INVALID_LAYERID;
	}

	return 0;
}

static int vo_get_videolayerattr(vo_layer layer, vo_video_layer_attr_s *layer_attr)
{
	int ret = -1;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	memcpy(layer_attr, &g_vo_ctx->layer_ctx[layer].layer_attr, sizeof(*layer_attr));

	return 0;
}

static int vo_set_videolayerattr(vo_layer layer, const vo_video_layer_attr_s *layer_attr)
{
	struct disp_rect rect;
	u16 rgb[3] = {0, 0, 0};
	int ret = -1;
	vo_dev dev;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_disable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (!VO_SUPPORT_FMT(layer_attr->pixformat)) {
		TRACE_VO(DBG_DEBUG, "layer(%d) pixformat(%d) unsupported\n",
			 layer, layer_attr->pixformat);
		return ERR_VO_ILLEGAL_PARAM;
	}

	dev = layer_ctx->bind_dev_id;
	if (dev == -1) {
		TRACE_VO(DBG_DEBUG, "layer(%d) unbind device", layer);
		return ERR_VO_SYS_NOTREADY;
	}

	if (layer_attr->img_size.width != layer_attr->disp_rect.width ||
	    layer_attr->img_size.height != layer_attr->disp_rect.height) {
		TRACE_VO(DBG_ERR, "layer(%d) img_size(%d %d) disp_rect(%d %d) isn't the same.\n",
			 layer, layer_attr->img_size.width, layer_attr->img_size.height,
			 layer_attr->disp_rect.width, layer_attr->disp_rect.height);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (layer_attr->img_size.width < VO_MIN_CHN_WIDTH ||
	    layer_attr->img_size.height < VO_MIN_CHN_HEIGHT) {
		TRACE_VO(DBG_ERR, "layer(%d) Size(%d %d) too small.\n",
			 layer, layer_attr->img_size.width, layer_attr->img_size.height);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (layer_attr->depth > VO_MAX_LAYER_DEPTH) {
		TRACE_VO(DBG_ERR, "layer(%d) Depth(%d) invalid.\n",
			 layer, layer_attr->depth);
		return ERR_VO_ILLEGAL_PARAM;
	}

	_vo_sdk_setfmt(layer_attr->img_size.width, layer_attr->img_size.height,
		       layer_attr->pixformat, dev);

	disp_set_window_bgcolor(dev, rgb[0], rgb[1], rgb[2]);

	rect.w = layer_attr->disp_rect.width;
	rect.h = layer_attr->disp_rect.height;
	rect.x = layer_attr->disp_rect.x;
	rect.y = layer_attr->disp_rect.y;

	//vo_set_tgt_compose(d->fd, &area);
	disp_set_rect(dev, rect);

	mutex_lock(&layer_ctx->layer_lock);
	layer_ctx->layer_attr.disp_rect = layer_attr->disp_rect;
	layer_ctx->layer_attr.img_size = layer_attr->img_size;
	layer_ctx->layer_attr.frame_rate = layer_attr->frame_rate;
	layer_ctx->layer_attr.pixformat = layer_attr->pixformat;
	if (!layer_ctx->is_layer_enable)
		layer_ctx->layer_attr.depth = layer_attr->depth;
	mutex_unlock(&layer_ctx->layer_lock);

	TRACE_VO(DBG_DEBUG, "layer(%d) image-size(%d * %d) disp-rect(%d-%d-%d-%d).\n", layer,
		 layer_attr->img_size.width, layer_attr->img_size.height,
		 layer_attr->disp_rect.x, layer_attr->disp_rect.y,
		 layer_attr->disp_rect.width, layer_attr->disp_rect.height);

	return 0;
}

static int vo_get_layer_proc_amp(vo_layer layer, int *proc_amp)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	memcpy(proc_amp, layer_ctx->proc_amp, sizeof(layer_ctx->proc_amp));

	return 0;
}

static int vo_set_layer_proc_amp(vo_layer layer, const int *proc_amp)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (!IS_FMT_YUV(layer_ctx->layer_attr.pixformat)) {
		TRACE_VO(DBG_ERR, "layer(%d) Only YUV format support.\n", layer);
		return ERR_VO_NOT_SUPPORT;
	}

	mutex_lock(&layer_ctx->layer_lock);
	memcpy(layer_ctx->proc_amp, proc_amp, sizeof(layer_ctx->proc_amp));
	mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_set_layer_csc(vo_layer layer, vo_csc_s video_csc)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	vo_dev dev;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	dev = layer_ctx->bind_dev_id;

	if (!IS_FMT_YUV(layer_ctx->layer_attr.pixformat)) {
		TRACE_VO(DBG_ERR, "layer(%d) Only YUV format support set csc.\n", layer);
		return ERR_VO_NOT_SUPPORT;
	}

	if (video_csc.csc_matrix >= VO_CSC_MATRIX_601_LIMIT_YUV2RGB &&
	    video_csc.csc_matrix <= VO_CSC_MATRIX_709_FULL_YUV2RGB) {
		disp_set_in_csc(dev, (enum disp_csc)video_csc.csc_matrix);
	} else {
		TRACE_VO(DBG_ERR, "layer(%d) CscMatrix(%d) invalid.\n", layer, video_csc.csc_matrix);
		return ERR_VO_ILLEGAL_PARAM;
	}

	return 0;
}

static int vo_get_layer_csc(vo_layer layer, vo_csc_s *video_csc)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	struct disp_cfg *disp_cfg;
	vo_dev dev;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	dev = layer_ctx->bind_dev_id;
	disp_cfg = disp_get_cfg(dev);
	video_csc->csc_matrix = (vo_csc_matrix_e)disp_cfg->in_csc;

	return 0;
}

static int vo_get_screen_frame(vo_layer layer, video_frame_info_s *video_frame, int millisec)
{
	int ret = -1;
	vb_blk blk;
	vo_dev dev = 0;
	mmf_chn_s chn;
	struct vb_s *vb;
	struct vo_layer_ctx *layer_ctx;
	int i = 0;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	dev = layer_ctx->bind_dev_id;

	chn.mod_id = ID_VO;
	chn.dev_id = dev;
	chn.chn_id = 0;
	memset(video_frame, 0, sizeof(*video_frame));

	ret = base_get_chn_buffer(chn, &layer_ctx->layer_jobs, &blk, millisec);
	if (ret != 0) {
		TRACE_VO(DBG_ERR, "vo get screen buf fail\n");
		return ret;
	}

	vb = (struct vb_s *)blk;

	video_frame->video_frame.pixel_format = vb->buf.pixel_format;
	video_frame->video_frame.width = vb->buf.size.width;
	video_frame->video_frame.height = vb->buf.size.height;
	video_frame->video_frame.time_ref = vb->buf.frm_num;
	video_frame->video_frame.pts = vb->buf.pts;
	for (i = 0; i < 3; ++i) {
		video_frame->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
		video_frame->video_frame.length[i] = vb->buf.length[i];
		video_frame->video_frame.stride[i] = vb->buf.stride[i];
	}

	video_frame->video_frame.offset_top = vb->buf.offset_top;
	video_frame->video_frame.offset_bottom = vb->buf.offset_bottom;
	video_frame->video_frame.offset_left = vb->buf.offset_left;
	video_frame->video_frame.offset_right = vb->buf.offset_right;
	video_frame->video_frame.private_data = vb;

	TRACE_VO(DBG_DEBUG, "pixfmt(%d), w(%d), h(%d), pts(%lld), addr(0x%llx, 0x%llx, 0x%llx)\n",
		 video_frame->video_frame.pixel_format, video_frame->video_frame.width,
		 video_frame->video_frame.height, video_frame->video_frame.pts,
		 video_frame->video_frame.phyaddr[0], video_frame->video_frame.phyaddr[1],
		 video_frame->video_frame.phyaddr[2]);
	TRACE_VO(DBG_DEBUG, "length(%d, %d, %d), stride(%d, %d, %d)\n",
		 video_frame->video_frame.length[0], video_frame->video_frame.length[1],
		 video_frame->video_frame.length[2], video_frame->video_frame.stride[0],
		 video_frame->video_frame.stride[1], video_frame->video_frame.stride[2]);

	return 0;
}

static int vo_release_screen_frame(vo_layer layer, video_frame_info_s *video_frame, int millisec)
{
	vb_blk blk;
	int ret = 0;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;


	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VO(DBG_ERR, "Invalid phy-address(%llx) in video_frame. Can't find vb_blk.\n",
			 video_frame->video_frame.phyaddr[0]);
		return -1;
	}

	if (vb_release_block(blk) != 0)
		return -1;

	TRACE_VO(DBG_DEBUG, "release layer frame, addr(0x%llx)\n",
		 video_frame->video_frame.phyaddr[0]);

	return 0;
}

static int vo_set_layer_toleration(vo_layer layer, u32 toleration)
{
	int ret = 0;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	if (toleration < VO_MIN_LAYER_TOLERATE || toleration > VO_MAX_LAYER_TOLERATE) {
		TRACE_VO(DBG_ERR, "layer(%d) toleration(%d) invalid.\n", layer, toleration);
		return ERR_VO_ILLEGAL_PARAM;
	}

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	layer_ctx->toleration = toleration;

	return 0;
}

static int vo_get_layer_toleration(vo_layer layer, u32 *toleration)
{
	int ret = 0;
	struct vo_layer_ctx *layer_ctx;

	ret = check_video_layer_valid(layer);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	*toleration = layer_ctx->toleration;

	return 0;
}

/****************************************************************************
 * SDK chn APIs
 ****************************************************************************/
static int vo_clear_chnbuf(vo_layer layer, vo_chn chn, bool clear)
{
	int ret = -1;
	vb_blk blk;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_vo_chn_enable(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	//clear chn waitq vb
	while (!base_mod_jobs_waitq_empty(&chn_ctx->chn_jobs)) {
		blk = base_mod_jobs_waitq_pop(&chn_ctx->chn_jobs);
		if (blk != VB_INVALID_HANDLE)
			vb_release_block(blk);
	}

	//clear chn workq vb
	while (clear && !base_mod_jobs_workq_empty(&chn_ctx->chn_jobs)) {
		blk = base_mod_jobs_workq_pop(&chn_ctx->chn_jobs);
		if (blk != VB_INVALID_HANDLE)
			vb_release_block(blk);
	}

	return 0;
}

static int vo_send_frame(vo_layer layer, vo_chn chn, video_frame_info_s *video_frame, int millisec)
{
	mmf_chn_s mmf_chn = {.mod_id = ID_VO, .dev_id = layer, .chn_id = chn};
	vb_blk blk;
	//size_s stSize;
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;

	UNUSED(millisec);

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_vo_chn_enable(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (layer_ctx->layer_attr.pixformat != video_frame->video_frame.pixel_format) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) PixelFormat(%d) mismatch.\n",
			 layer, chn, video_frame->video_frame.pixel_format);
		return ERR_VO_ILLEGAL_PARAM;
	}

#if 0
	if (chn_ctx->rotation == K_ROTATION_90 ||
	    chn_ctx->rotation == K_ROTATION_270) {
		stSize.width = chn_ctx->chn_attr.rect.height;
		stSize.height = chn_ctx->chn_attr.rect.width;
	} else {
		stSize.width = chn_ctx->chn_attr.rect.width;
		stSize.height = chn_ctx->chn_attr.rect.height;
	}

	if ((stSize.width != (video_frame->video_frame.width -
		video_frame->video_frame.offset_left - video_frame->video_frame.offset_right) ||
		(stSize.height != (video_frame->video_frame.height -
		video_frame->video_frame.offset_top - video_frame->video_frame.offset_bottom))) {
		TRACE_VO(DBG_ERR,
			     "layer(%d) chn(%d) Size(%d * %d) frame width(%d %d %d) height(%d %d %d)mismatch.\n"
			     , layer, chn, stSize.width, stSize.height
			     , video_frame->video_frame.offset_left, video_frame->video_frame.offset_right
			     , video_frame->video_frame.width, video_frame->video_frame.offset_top
			     , video_frame->video_frame.offset_bottom, video_frame->video_frame.height);
		return ERR_VO_ILLEGAL_PARAM;
	}
#endif

	if (IS_FRAME_OFFSET_INVALID(video_frame->video_frame)) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) frame offset (%d %d %d %d) invalid\n",
			 layer, chn,
			 video_frame->video_frame.offset_left, video_frame->video_frame.offset_right,
			 video_frame->video_frame.offset_top, video_frame->video_frame.offset_bottom);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (IS_FMT_YUV420(layer_ctx->layer_attr.pixformat)) {
		if ((video_frame->video_frame.width - video_frame->video_frame.offset_left -
		     video_frame->video_frame.offset_right) & 0x01) {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) YUV420 can't accept odd frame valid width\n",
				 layer, chn);
			TRACE_VO(DBG_ERR, "width(%d) offset_left(%d) offset_right(%d)\n",
				 video_frame->video_frame.width, video_frame->video_frame.offset_left,
				 video_frame->video_frame.offset_right);
			return ERR_VO_ILLEGAL_PARAM;
		}
		if ((video_frame->video_frame.height - video_frame->video_frame.offset_top -
		     video_frame->video_frame.offset_bottom) & 0x01) {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) YUV420 can't accept odd frame valid height\n",
				 layer, chn);
			TRACE_VO(DBG_ERR, "height(%d) offset_top(%d) offset_bottom(%d)\n",
				 video_frame->video_frame.height, video_frame->video_frame.offset_top,
				 video_frame->video_frame.offset_bottom);
			return ERR_VO_ILLEGAL_PARAM;
		}
	}
	if (IS_FMT_YUV422(layer_ctx->layer_attr.pixformat)) {
		if ((video_frame->video_frame.width - video_frame->video_frame.offset_left -
		     video_frame->video_frame.offset_right) & 0x01) {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) YUV422 can't accept odd frame valid width\n",
				 layer, chn);
			TRACE_VO(DBG_ERR, "width(%d) offset_left(%d) offset_right(%d)\n",
				 video_frame->video_frame.width, video_frame->video_frame.offset_left,
				 video_frame->video_frame.offset_right);
			return ERR_VO_ILLEGAL_PARAM;
		}
	}

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Invalid phy-addr(%llx). Can't locate vb_blk.\n",
			 layer, chn, video_frame->video_frame.phyaddr[0]);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (base_fill_videoframe2buffer(mmf_chn, video_frame, &((struct vb_s *)blk)->buf) != 0) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Invalid parameter\n", layer, chn);
		return ERR_VO_ILLEGAL_PARAM;
	}

	vo_recv_frame(mmf_chn, blk);

	return ret;
}

static int vo_get_chn_attr(vo_layer layer, vo_chn chn, vo_chn_attr_s *chn_attr)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	memcpy(chn_attr, &chn_ctx->chn_attr, sizeof(*chn_attr));

	return 0;
}

static int vo_set_chn_attr(vo_layer layer, vo_chn chn, const vo_chn_attr_s *chn_attr)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	if (chn_attr->rect.width < VO_MIN_CHN_WIDTH || chn_attr->rect.height < VO_MIN_CHN_HEIGHT ||
	    (chn_attr->rect.width + chn_attr->rect.x > layer_ctx->layer_attr.img_size.width) ||
	    (chn_attr->rect.height + chn_attr->rect.y > layer_ctx->layer_attr.img_size.height) ||
	    chn_attr->rect.x < 0 || chn_attr->rect.y < 0) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) rect(%d %d %d %d) invalid.\n",
			 layer, chn, chn_attr->rect.x, chn_attr->rect.y,
			 chn_attr->rect.width, chn_attr->rect.height);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (chn_attr->depth > VO_MAX_CHN_DEPTH) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Depth(%d) too big.\n",
			 layer, chn, chn_attr->depth);
		return ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&layer_ctx->layer_lock);
	chn_ctx->chn_attr.priority = chn_attr->priority;
	chn_ctx->chn_attr.rect = chn_attr->rect;
	if (!chn_ctx->is_chn_enable)
		chn_ctx->chn_attr.depth = chn_attr->depth;
	//when chn rect change need disable
	memset(&chn_ctx->chn_zoom_attr, 0, sizeof(chn_ctx->chn_zoom_attr));
	memset(&chn_ctx->chn_param, 0, sizeof(chn_ctx->chn_param));
	mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_set_chn_param(vo_layer layer, vo_chn chn, const vo_chn_param_s *chn_param)
{
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;
	vo_dev dev;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	dev = layer_ctx->bind_dev_id;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (dev_ctx->pub_attr.intf_type == 0) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) isn't correctly configured.\n", dev);
		return ERR_VO_DEV_NOT_CONFIG;
	}

	if (!dev_ctx->is_dev_enable) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) should be enabled.\n", dev);
		return ERR_VO_DEV_NOT_ENABLED;
	}

	if (layer_ctx->layer_attr.img_size.width == 0 || layer_ctx->layer_attr.img_size.height == 0) {
		TRACE_VO(DBG_ERR, "layer(%d) isn't correctly configured.\n", layer);
		return ERR_VO_VIDEO_NOT_CONFIG;
	}

	if (chn_param->aspect_ratio.mode >= ASPECT_RATIO_MAX || chn_param->aspect_ratio.mode < 0) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) AspectRatio Mode(%d) invalid.\n",
			 layer, chn, chn_param->aspect_ratio.mode);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if ((chn_param->aspect_ratio.video_rect.x < 0 || chn_param->aspect_ratio.video_rect.y < 0 ||
	     ((chn_param->aspect_ratio.video_rect.x + chn_param->aspect_ratio.video_rect.width) >
	     chn_ctx->chn_attr.rect.width) || ((chn_param->aspect_ratio.video_rect.y +
	     chn_param->aspect_ratio.video_rect.height) > chn_ctx->chn_attr.rect.height)) &&
	     chn_param->aspect_ratio.mode == ASPECT_RATIO_MANUAL) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) AspectRatio rect(%d %d %d %d) invalid.\n",
			 layer, chn, chn_param->aspect_ratio.video_rect.x, chn_param->aspect_ratio.video_rect.y,
			 chn_param->aspect_ratio.video_rect.width, chn_param->aspect_ratio.video_rect.height);
		return ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&layer_ctx->layer_lock);
	memcpy(&chn_ctx->chn_param, chn_param, sizeof(*chn_param));
	mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_get_chn_param(vo_layer layer, vo_chn chn, vo_chn_param_s *chn_param)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	memcpy(chn_param, &chn_ctx->chn_param, sizeof(*chn_param));
	return 0;
}

static int vo_set_chn_zoom(vo_layer layer, vo_chn chn, const vo_chn_zoom_attr_s *chn_zoom_attr)
{
	int ret = -1;
	struct vo_dev_ctx *dev_ctx;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;
	vo_dev dev;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	dev = layer_ctx->bind_dev_id;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	if (dev_ctx->pub_attr.intf_type == 0) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) isn't correctly configured.\n", dev);
		return ERR_VO_DEV_NOT_CONFIG;
	}

	if (!dev_ctx->is_dev_enable) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) should be enabled.\n", dev);
		return ERR_VO_DEV_NOT_ENABLED;
	}

	if (layer_ctx->layer_attr.img_size.width == 0 || layer_ctx->layer_attr.img_size.height == 0) {
		TRACE_VO(DBG_ERR, "layer(%d) isn't correctly configured.\n", layer);
		return ERR_VO_VIDEO_NOT_CONFIG;
	}

	//notice : rect/zoom_ratio is for src pic
	if (chn_zoom_attr->zoom_type == VO_CHN_ZOOM_IN_RECT) {
		if ((chn_zoom_attr->rect.x < 0) || (chn_zoom_attr->rect.y < 0) ||
			(chn_zoom_attr->rect.width < VO_MIN_CHN_WIDTH && chn_zoom_attr->rect.width != 0) ||
		    (chn_zoom_attr->rect.height < VO_MIN_CHN_HEIGHT && chn_zoom_attr->rect.height != 0) ||
		    (chn_zoom_attr->rect.width & 0x01) || (chn_zoom_attr->rect.height & 0x01)) {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Zoom rect(%d %d %d %d) invalid.\n",
				 layer, chn, chn_zoom_attr->rect.x, chn_zoom_attr->rect.y,
			chn_zoom_attr->rect.width, chn_zoom_attr->rect.height);
			return ERR_VO_ILLEGAL_PARAM;
		}
		if (((chn_ctx->chn_attr.rect.width / chn_zoom_attr->rect.width) > VO_MAX_CHN_SCALE) ||
		    ((chn_ctx->chn_attr.rect.height / chn_zoom_attr->rect.height) > VO_MAX_CHN_SCALE)) {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Zoom rect(%d %d %d %d) More than 16x scaling.\n",
				 layer, chn, chn_zoom_attr->rect.x, chn_zoom_attr->rect.y,
			chn_zoom_attr->rect.width, chn_zoom_attr->rect.height);
			return ERR_VO_ILLEGAL_PARAM;
		}
	} else if (chn_zoom_attr->zoom_type == VO_CHN_ZOOM_IN_RATIO) {
		if (chn_zoom_attr->zoom_ratio.x_ratio > VO_MAX_CHN_ZOOM_RATIO ||
		    chn_zoom_attr->zoom_ratio.y_ratio > VO_MAX_CHN_ZOOM_RATIO ||
		    chn_zoom_attr->zoom_ratio.height_ratio > VO_MAX_CHN_ZOOM_RATIO ||
		    chn_zoom_attr->zoom_ratio.height_ratio > VO_MAX_CHN_ZOOM_RATIO) {
			TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Zoom Ratio(%d %d %d %d) invalid.\n",
				 layer, chn, chn_zoom_attr->zoom_ratio.x_ratio, chn_zoom_attr->zoom_ratio.y_ratio,
				 chn_zoom_attr->zoom_ratio.width_ratio, chn_zoom_attr->zoom_ratio.height_ratio);
			return ERR_VO_ILLEGAL_PARAM;
		}
	} else {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) ZoomType(%d) invalid.\n",
			 layer, chn, chn_zoom_attr->zoom_type);
		return ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&layer_ctx->layer_lock);
	memcpy(&chn_ctx->chn_zoom_attr, chn_zoom_attr, sizeof(*chn_zoom_attr));
	mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_get_chn_zoom(vo_layer layer, vo_chn chn, vo_chn_zoom_attr_s *chn_zoom_attr)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	memcpy(chn_zoom_attr, &chn_ctx->chn_zoom_attr, sizeof(*chn_zoom_attr));
	return 0;
}

static int vo_set_chn_border(vo_layer layer, vo_chn chn, const vo_chn_border_attr_s *chn_border_attr)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	if (layer_ctx->layer_attr.img_size.width == 0 || layer_ctx->layer_attr.img_size.height == 0) {
		TRACE_VO(DBG_ERR, "layer(%d) isn't correctly configured.\n", layer);
		return ERR_VO_VIDEO_NOT_CONFIG;
	}

	if (chn_border_attr->border.top_width > VO_MAX_CHN_BORDER_WIDTH ||
	    chn_border_attr->border.bottom_width > VO_MAX_CHN_BORDER_WIDTH ||
	    chn_border_attr->border.right_width > VO_MAX_CHN_BORDER_WIDTH ||
	    chn_border_attr->border.right_width > VO_MAX_CHN_BORDER_WIDTH) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Border(%d %d %d %d) invalid.\n",
			 layer, chn, chn_border_attr->border.top_width,
			 chn_border_attr->border.bottom_width,
		chn_border_attr->border.left_width, chn_border_attr->border.right_width);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (chn_border_attr->border.top_width & 0x01 || chn_border_attr->border.bottom_width & 0x01 ||
	    chn_border_attr->border.left_width & 0x01 || chn_border_attr->border.right_width & 0x01) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Border(%d %d %d %d) 2-byte alignment required\n",
			 layer, chn, chn_border_attr->border.top_width,
			 chn_border_attr->border.bottom_width,
			 chn_border_attr->border.left_width,
			 chn_border_attr->border.right_width);
		return ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&layer_ctx->layer_lock);
	memcpy(&chn_ctx->chn_border_attr, chn_border_attr, sizeof(*chn_border_attr));
	mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_get_chn_border(vo_layer layer, vo_chn chn, vo_chn_border_attr_s *chn_border_attr)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	memcpy(chn_border_attr, &chn_ctx->chn_border_attr, sizeof(*chn_border_attr));
	return 0;
}

static int vo_set_chn_mirror(vo_layer layer, vo_chn chn, vo_chn_mirror_type_e chn_mirror)
{
	int ret = -1;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	if (layer_ctx->layer_attr.img_size.width == 0 || layer_ctx->layer_attr.img_size.height == 0) {
		TRACE_VO(DBG_ERR, "layer(%d) isn't correctly configured.\n", layer);
		return ERR_VO_VIDEO_NOT_CONFIG;
	}

	if (chn_mirror >= VO_CHN_MIRROR_BUTT || chn_mirror < 0) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) Mirror(%d) invalid.\n",
			 layer, chn, chn_mirror);
		return ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&layer_ctx->layer_lock);
	chn_ctx->chn_mirror = chn_mirror;
	mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int vo_get_chn_mirror(vo_layer layer, vo_chn chn, vo_chn_mirror_type_e *chn_mirror)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	*chn_mirror = chn_ctx->chn_mirror;

	return 0;
}

static int vo_get_chn_frame(vo_layer layer, vo_chn chn, video_frame_info_s *video_frame, int millisec)
{
	int ret = -1;
	vb_blk blk;
	vo_dev dev = 0;
	mmf_chn_s mmf_chn;
	struct vb_s *vb;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;
	int i = 0;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_vo_chn_enable(layer, chn);
	if (ret != 0)
		return ret;

	layer_ctx = &g_vo_ctx->layer_ctx[layer];
	dev = layer_ctx->bind_dev_id;
	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	mmf_chn.mod_id = ID_VO;
	mmf_chn.dev_id = dev;
	mmf_chn.chn_id = chn;
	memset(video_frame, 0, sizeof(*video_frame));

	ret = base_get_chn_buffer(mmf_chn, &chn_ctx->chn_jobs, &blk, millisec);
	if (ret != 0) {
		TRACE_VO(DBG_ERR, "vo get chn buf fail\n");
		return ret;
	}

	vb = (struct vb_s *)blk;

	video_frame->video_frame.pixel_format = vb->buf.pixel_format;
	video_frame->video_frame.width = vb->buf.size.width;
	video_frame->video_frame.height = vb->buf.size.height;
	video_frame->video_frame.time_ref = vb->buf.frm_num;
	video_frame->video_frame.pts = vb->buf.pts;
	for (i = 0; i < 3; ++i) {
		video_frame->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
		video_frame->video_frame.length[i] = vb->buf.length[i];
		video_frame->video_frame.stride[i] = vb->buf.stride[i];
	}

	video_frame->video_frame.offset_top = vb->buf.offset_top;
	video_frame->video_frame.offset_bottom = vb->buf.offset_bottom;
	video_frame->video_frame.offset_left = vb->buf.offset_left;
	video_frame->video_frame.offset_right = vb->buf.offset_right;
	video_frame->video_frame.private_data = vb;

	TRACE_VO(DBG_DEBUG, "pixfmt(%d), w(%d), h(%d), pts(%lld), addr(0x%llx, 0x%llx, 0x%llx)\n",
		 video_frame->video_frame.pixel_format, video_frame->video_frame.width,
		 video_frame->video_frame.height, video_frame->video_frame.pts,
		 video_frame->video_frame.phyaddr[0], video_frame->video_frame.phyaddr[1],
		 video_frame->video_frame.phyaddr[2]);
	TRACE_VO(DBG_DEBUG, "length(%d, %d, %d), stride(%d, %d, %d)\n",
		 video_frame->video_frame.length[0], video_frame->video_frame.length[1],
		 video_frame->video_frame.length[2], video_frame->video_frame.stride[0],
		 video_frame->video_frame.stride[1], video_frame->video_frame.stride[2]);

	return 0;
}

static int vo_release_chn_frame(vo_layer layer, vo_chn chn, video_frame_info_s *video_frame, int millisec)
{
	vb_blk blk;
	int ret = 0;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VO(DBG_ERR, "Invalid phy-address(%llx) in video_frame. Can't find vb_blk.\n",
			 video_frame->video_frame.phyaddr[0]);
		return -1;
	}

	if (vb_release_block(blk) != 0)
		return -1;

	TRACE_VO(DBG_DEBUG, "release chn frame, addr(0x%llx)\n",
		 video_frame->video_frame.phyaddr[0]);

	return 0;
}

static int vo_set_chn_framerate(vo_layer layer, vo_chn chn, u32 frame_rate)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	if (frame_rate > chn_ctx->frame_rate) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) FrameRate(%d) invalid.\n",
			 layer, chn, frame_rate);
	}
	//ueser set framerate
	chn_ctx->frame_rate_user_set = frame_rate;

	return 0;
}

static int vo_get_chn_framerate(vo_layer layer, vo_chn chn, u32 *frame_rate)
{
	int ret = 0;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	//realframerate
	*frame_rate = chn_ctx->frame_rate;

	return 0;
}

static int vo_get_chn_pts(vo_layer layer, vo_chn chn, u64 *chn_pts)
{
	int ret = 0;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_vo_chn_enable(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	*chn_pts = chn_ctx->display_pts;

	return 0;
}

static int vo_get_chn_status(vo_layer layer, vo_chn chn, vo_query_status_s *status)
{
	int ret = 0;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_vo_chn_enable(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	//threshold + VO_CHN_WORKQ + depth
	status->chn_buf_used = chn_ctx->threshold + chn_ctx->chn_attr.depth;

	return 0;
}

static int vo_set_chn_threshold(vo_layer layer, vo_chn chn, u32 threshold)
{
	int ret = 0;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	if (g_vo_ctx->layer_ctx[layer].chn_ctx[chn].is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) must be disable.\n", layer, chn);
		return ERR_VO_CHN_NOT_DISABLED;
	}

	if (threshold < 2 || threshold > 8) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) threshold(%d) invalid.\n", layer, chn, threshold);
		return ERR_VO_ILLEGAL_PARAM;
	}

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	chn_ctx->threshold = threshold;

	return 0;
}

static int vo_get_chn_threshold(vo_layer layer, vo_chn chn, u32 *threshold)
{
	int ret = 0;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	*threshold = chn_ctx->threshold ? chn_ctx->threshold : VO_CHN_THRESHOLD;

	return 0;
}

static int vo_enable_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;
	struct vo_layer_ctx *layer_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	ret = check_video_layer_enable(layer);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already enabled.\n", layer, chn);
		return ERR_VO_CHN_NOT_DISABLED;
	}

	mutex_lock(&layer_ctx->layer_lock);
	chn_ctx->is_chn_enable = true;
	if (chn_ctx->threshold)
		base_mod_jobs_init(&chn_ctx->chn_jobs, chn_ctx->threshold - VO_CHN_WORKQ,
				   VO_CHN_WORKQ, chn_ctx->chn_attr.depth);
	else {
		base_mod_jobs_init(&chn_ctx->chn_jobs, VO_CHN_THRESHOLD - VO_CHN_WORKQ, VO_CHN_WORKQ,
				   chn_ctx->chn_attr.depth);
		chn_ctx->threshold = VO_CHN_THRESHOLD;
	}
	mutex_unlock(&layer_ctx->layer_lock);

	return ret;
}

int vo_disable_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;
	struct vo_layer_ctx *layer_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n", layer, chn);
		return 0;
	}

	mutex_lock(&layer_ctx->layer_lock);
	base_mod_jobs_exit(&chn_ctx->chn_jobs);
	chn_ctx->is_chn_enable = false;
	chn_ctx->frame_num = 0;
	chn_ctx->frame_rate = 0;
	chn_ctx->src_frame_num = 0;
	chn_ctx->src_frame_rate = 0;
	chn_ctx->frame_index = 0;
	chn_ctx->frame_rate_user_set = 0;
	chn_ctx->display_pts = 0;
	chn_ctx->predone_pts = 0;
	chn_ctx->chn_mirror = 0;
	chn_ctx->pause = false;
	chn_ctx->refresh = false;
	chn_ctx->step = false;
	chn_ctx->step_trigger = false;
	chn_ctx->rotation = ROTATION_0;
	memset(&chn_ctx->chn_zoom_attr, 0, sizeof(chn_ctx->chn_zoom_attr));
	memset(&chn_ctx->chn_border_attr, 0, sizeof(chn_ctx->chn_border_attr));
	memset(&chn_ctx->chn_param, 0, sizeof(chn_ctx->chn_param));
	mutex_unlock(&layer_ctx->layer_lock);
	TRACE_VO(DBG_INFO, "layer(%d) chn(%d) disabled.\n", layer, chn);

	return 0;
}

static int vo_hide_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vb_s *vb;
	struct vo_chn_ctx *chn_ctx;
	struct vb_jobs_t *jobs;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n", layer, chn);
		return 0;
	}

	mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].hide = true;
	g_vo_ctx->layer_ctx[layer].is_layer_update = true;
	mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);

	jobs = &chn_ctx->chn_jobs;
	mutex_lock(&jobs->lock);
	while (!FIFO_EMPTY(&jobs->waitq)) {
		FIFO_POP(&jobs->waitq, &vb);
		vb_release_block((vb_blk)vb);
	}
	mutex_unlock(&jobs->lock);

	return 0;
}

static int vo_show_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	if (!g_vo_ctx->layer_ctx[layer].chn_ctx[chn].is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n", layer, chn);
		return 0;
	}

	mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].hide = false;
	g_vo_ctx->layer_ctx[layer].is_layer_update = true;
	mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);

	return 0;
}

static int vo_pause_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vb_s *vb;
	struct vo_chn_ctx *chn_ctx;
	struct vb_jobs_t *jobs;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n", layer, chn);
		return 0;
	}

	mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].pause = true;
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].step = false;
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].step_trigger = false;
	mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);

	jobs = &chn_ctx->chn_jobs;
	mutex_lock(&jobs->lock);
	while (!FIFO_EMPTY(&jobs->waitq)) {
		FIFO_POP(&jobs->waitq, &vb);
		vb_release_block((vb_blk)vb);
	}
	mutex_unlock(&jobs->lock);

	return 0;
}

static int vo_step_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n", layer, chn);
		return 0;
	}
	if (chn_ctx->step && chn_ctx->step_trigger) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) The last time step trigger was not finish.\n", layer, chn);
		return 0;
	}

	mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].pause = false;
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].refresh = false;
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].step = true;
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].step_trigger = true;
	mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);

	return 0;
}

static int vo_resume_chn(vo_layer layer, vo_chn chn)
{
	int ret = -1;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	if (!g_vo_ctx->layer_ctx[layer].chn_ctx[chn].is_chn_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) already disabled.\n", layer, chn);
		return 0;
	}

	mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].pause = false;
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].refresh = false;
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].step = false;
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].step_trigger = false;
	mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);

	return 0;
}

static int vo_refresh_chn(vo_layer layer, vo_chn chn)
{
	struct vo_chn_ctx *chn_ctx;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];

	if (!chn_ctx->pause) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) can only refresh in paused state.\n", layer, chn);
		return 0;
	}

	if (chn_ctx->refresh) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) The last time refresh was not finish.\n", layer, chn);
		return 0;
	}

	mutex_lock(&g_vo_ctx->layer_ctx[layer].layer_lock);
	g_vo_ctx->layer_ctx[layer].chn_ctx[chn].refresh = true;
	mutex_unlock(&g_vo_ctx->layer_ctx[layer].layer_lock);

	return 0;
}

int vo_get_chnrotation(vo_layer layer, vo_chn chn, rotation_e *rotation)
{
	int ret = -1;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	*rotation = g_vo_ctx->layer_ctx[layer].chn_ctx[chn].rotation;

	return 0;
}

static int vo_set_chnrotation(vo_layer layer, vo_chn chn, rotation_e rotation)
{
	int ret = -1;
	struct vo_chn_ctx *chn_ctx;
	struct vo_layer_ctx *layer_ctx;

	ret = check_vo_chn_valid(layer, chn);
	if (ret != 0)
		return ret;

	chn_ctx = &g_vo_ctx->layer_ctx[layer].chn_ctx[chn];
	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (rotation >= ROTATION_MAX) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) invalid rotation(%d).\n", layer, chn, rotation);
		return ERR_VO_ILLEGAL_PARAM;
	} else if (rotation == ROTATION_0) {
		chn_ctx->rotation = rotation;
		return 0;
	}

	if (!GDC_SUPPORT_FMT(layer_ctx->layer_attr.pixformat)) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) invalid PixFormat(%d).\n",
			 layer, chn, layer_ctx->layer_attr.pixformat);
		return ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&layer_ctx->layer_lock);
	// TODO: dummy settings
	chn_ctx->mesh.paddr = DEFAULT_MESH_PADDR;
	chn_ctx->rotation = rotation;
	mutex_unlock(&layer_ctx->layer_lock);

	return 0;
}

static int _vo_wbc_qbuf(struct vo_wbc_ctx *wbc_ctx, struct video_buffer *buf)
{
	struct wbc_buffer *qbuf;

	qbuf = kzalloc(sizeof(*qbuf), GFP_ATOMIC);
	if (qbuf == NULL) {
		TRACE_VO(DBG_ERR, "qbuf kzalloc size(%zu) failed\n", sizeof(struct wbc_buffer));
		return -ENOMEM;
	}

	qbuf->buf.addr[0] = buf->phy_addr[0];
	qbuf->buf.addr[1] = buf->phy_addr[1];
	qbuf->buf.addr[2] = buf->phy_addr[2];

	qbuf->buf.pitch_y = buf->stride[0];
	qbuf->buf.pitch_c = buf->stride[1];

	qbuf->buf.width = buf->size.width;
	qbuf->buf.height = buf->size.height;

	vo_wbc_rdy_buf_queue(wbc_ctx, qbuf);

	return 0;
}

int vo_wbc_qbuf(struct vo_wbc_ctx *wbc_ctx)
{
	int ret;
	vb_blk blk;
	struct vb_s *vb;
	vb_cal_config_s vb_cal_config;
	vo_dev dev = wbc_ctx->wbc_src.src_id;
	mmf_chn_s chn = {.mod_id = ID_VO, .dev_id = dev, .chn_id = 0};
	size_s stSize = wbc_ctx->wbc_attr.target_size;
	pixel_format_e pixformat = wbc_ctx->wbc_attr.pixformat;
	// only support SDR8 COMPRESS_NONE
	common_getpicbufferconfig(stSize.width, stSize.height, pixformat
			, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DISP_ALIGNMENT, &vb_cal_config);
	// get vb for odma write
	blk = vb_get_block_with_id(VB_INVALID_POOLID, vb_cal_config.vb_size, ID_VO);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VO(DBG_ERR, "Can't acquire vb block for wbc, size(%d)\n", vb_cal_config.vb_size);
		return ERR_VO_NO_MEM;
	}

	vb = (struct vb_s *)blk;

	base_get_frame_info(pixformat
				, stSize
				, &vb->buf
				, vb_handle2phys_addr(blk)
				, DEFAULT_ALIGN);

	// not support scale/crop
	vb->buf.offset_top = 0;
	vb->buf.offset_right = 0;
	vb->buf.offset_left = 0;
	vb->buf.offset_bottom = 0;

	ret = vb_qbuf(chn, CHN_TYPE_OUT, &wbc_ctx->wbc_jobs, blk);
	if (ret != 0) {
		TRACE_VO(DBG_ERR, "wbc vb_qbuf failed\n");
		return ret;
	}

	ret = _vo_wbc_qbuf(wbc_ctx, &vb->buf);
	if (ret != 0) {
		TRACE_VO(DBG_ERR, "_vo_wbc_qbuf failed\n");
		return ret;
	}

	ret = vb_release_block(blk);
	if (ret != 0) {
		TRACE_VO(DBG_ERR, "wbc vb_release_block failed\n");
		return ret;
	}

	return ret;
}

static int vo_set_wbc_src(vo_wbc wbc_dev, vo_wbc_src_s *wbc_src)
{
	int ret = -1;

	struct vo_wbc_ctx *wbc_ctx;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];

	if (wbc_src->src_type < VO_WBC_SRC_DEV || wbc_src->src_type >= VO_WBC_SRC_BUTT ||
	    wbc_src->src_id >= VO_MAX_DEV_NUM) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) SrcType(%d) SrcId(%d) invalid.\n",
			 wbc_dev, wbc_src->src_type, wbc_src->src_id);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (wbc_src->src_id == 0 && wbc_src->src_type == VO_WBC_SRC_DEV) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) SrcType(%d) SrcId(%d) invalid. "
			 "only device 1 support VO_WBC_SRC_DEV\n",
			 wbc_dev, wbc_src->src_type, wbc_src->src_id);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (wbc_ctx->is_wbc_enable) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) already enabled.\n", wbc_dev);
		return ERR_VO_WBC_NOT_DISABLED;
	}

	mutex_lock(&wbc_ctx->wbc_lock);
	memcpy(&wbc_ctx->wbc_src, wbc_src, sizeof(*wbc_src));
	wbc_ctx->is_wbc_src_cfg = true;
	mutex_unlock(&wbc_ctx->wbc_lock);

	return 0;
}

static int vo_get_wbc_src(vo_wbc wbc_dev, vo_wbc_src_s *wbc_src)
{
	int ret = -1;
	struct vo_wbc_ctx *wbc_ctx;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];
	memcpy(wbc_src, &wbc_ctx->wbc_src, sizeof(*wbc_src));

	return 0;
}

static int vo_enable_wbc(vo_wbc wbc_dev)
{
	int ret = -1;
	vo_dev dev;
	vo_layer layer;
	struct vo_dev_ctx *dev_ctx;
	struct vo_layer_ctx *layer_ctx;
	struct vo_wbc_ctx *wbc_ctx;
	int i = 0;
	struct wbc_buffer *wbc_qbuf, *tmp;
	unsigned long flags;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];
	dev = wbc_ctx->wbc_src.src_id;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];
	layer = dev_ctx->bind_layer_id;

	if (dev_ctx->bind_layer_id == -1) {
		TRACE_VO(DBG_DEBUG, "dev(%d) unbind layer", dev);
		return ERR_VO_SYS_NOTREADY;
	}

	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (!dev_ctx->is_dev_enable) {
		TRACE_VO(DBG_ERR, "VO DEV(%d) isn't enabled yet.\n", dev);
		return ERR_VO_DEV_NOT_ENABLED;
	}

	if (!layer_ctx->is_layer_enable) {
		TRACE_VO(DBG_ERR, "layer(%d) isn't enabled yet.\n", layer);
		return ERR_VO_VIDEO_NOT_ENABLED;
	}

	if (!wbc_ctx->is_wbc_attr_cfg) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) attr has not been set yet.\n", wbc_dev);
		return ERR_VO_WBC_ATTR_NOT_CONFIG;
	}

	if (wbc_ctx->is_wbc_enable) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) already enabled.\n", wbc_dev);
		return 0;
	}

	if (wbc_ctx->wbc_src.src_type == VO_WBC_SRC_DEV) {
		base_mod_jobs_init(&wbc_ctx->wbc_jobs, 0, VO_WBC_WORKQ, wbc_ctx->depth);
		INIT_LIST_HEAD(&wbc_ctx->qbuf_list);
		INIT_LIST_HEAD(&wbc_ctx->dqbuf_list);
		for (i = 0; i < VO_WBC_WORKQ; i++) {
			ret = vo_wbc_qbuf(wbc_ctx);
			if (ret != 0) {
				TRACE_VO(DBG_ERR, "vo_wbc_sdk_qbuf error (%d)", ret);
				goto ERR_QBUF;
			}
		}
		wbc_ctx->event = 0;
		init_waitqueue_head(&wbc_ctx->wq);
		ret = vo_wbc_create_thread(wbc_dev);
		if (ret) {
			TRACE_VO(DBG_ERR, "Failed to create thread, wbc_dev(%d).\n", wbc_dev);
			return ret;
		}
	}

	mutex_lock(&wbc_ctx->wbc_lock);
	wbc_ctx->is_wbc_enable = true;
	mutex_unlock(&wbc_ctx->wbc_lock);

	return ret;

ERR_QBUF:
	spin_lock_irqsave(&wbc_ctx->qbuf_lock, flags);
	list_for_each_entry_safe(wbc_qbuf, tmp, &wbc_ctx->qbuf_list, list) {
		kfree(wbc_qbuf);
	}
	wbc_ctx->qbuf_num = 0;
	INIT_LIST_HEAD(&wbc_ctx->qbuf_list);
	spin_unlock_irqrestore(&wbc_ctx->qbuf_lock, flags);

	spin_lock_irqsave(&wbc_ctx->dqbuf_lock, flags);
	list_for_each_entry_safe(wbc_qbuf, tmp, &wbc_ctx->dqbuf_list, list) {
		kfree(wbc_qbuf);
	}
	INIT_LIST_HEAD(&wbc_ctx->dqbuf_list);
	spin_unlock_irqrestore(&wbc_ctx->dqbuf_lock, flags);

	base_mod_jobs_exit(&wbc_ctx->wbc_jobs);

	return ret;
}

int vo_disable_wbc(vo_wbc wbc_dev)
{
	int ret = -1;
	struct vo_wbc_ctx *wbc_ctx;
	struct wbc_buffer *wbc_qbuf, *tmp;
	unsigned long flags;
	vo_dev dev;
	union disp_odma_intr_sel online_odma_mask;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];
	dev = wbc_ctx->wbc_src.src_id;

	if (!wbc_ctx->is_wbc_enable) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) already disabled.\n", wbc_dev);
		return 0;
	}

	if (wbc_ctx->wbc_src.src_type == VO_WBC_SRC_DEV) {
		vo_wbc_destroy_thread(wbc_dev);

		spin_lock_irqsave(&wbc_ctx->qbuf_lock, flags);
		list_for_each_entry_safe(wbc_qbuf, tmp, &wbc_ctx->qbuf_list, list) {
			kfree(wbc_qbuf);
		}
		wbc_ctx->qbuf_num = 0;
		INIT_LIST_HEAD(&wbc_ctx->qbuf_list);
		spin_unlock_irqrestore(&wbc_ctx->qbuf_lock, flags);

		spin_lock_irqsave(&wbc_ctx->dqbuf_lock, flags);
		list_for_each_entry_safe(wbc_qbuf, tmp, &wbc_ctx->dqbuf_list, list) {
			kfree(wbc_qbuf);
		}
		INIT_LIST_HEAD(&wbc_ctx->dqbuf_list);
		spin_unlock_irqrestore(&wbc_ctx->dqbuf_lock, flags);

		base_mod_jobs_exit(&wbc_ctx->wbc_jobs);

		if (wbc_ctx->is_odma_enable) {
			// odma disable
			disp_get_odma_intr_mask(wbc_ctx->wbc_src.src_id, &online_odma_mask);
			online_odma_mask.b.disp_online_frame_end = false; //true means disable
			online_odma_mask.b.disp_odma_frame_end = true; //true means disable
			disp_set_odma_intr_mask(wbc_ctx->wbc_src.src_id, online_odma_mask);
			disp_odma_enable(wbc_ctx->wbc_src.src_id, false);
			wbc_ctx->is_odma_enable = false;
		}
	}

	mutex_lock(&wbc_ctx->wbc_lock);
	wbc_ctx->is_wbc_enable = false;
	wbc_ctx->is_wbc_src_cfg = false;
	wbc_ctx->is_wbc_attr_cfg = false;
	wbc_ctx->wbc_src.src_type = VO_WBC_SRC_DEV;
	wbc_ctx->wbc_src.src_id = 0;
	wbc_ctx->wbc_attr.target_size.width = 0;
	wbc_ctx->wbc_attr.target_size.height = 0;
	wbc_ctx->wbc_attr.pixformat = PIXEL_FORMAT_NV21;
	wbc_ctx->wbc_attr.frame_rate = 0;
	wbc_ctx->wbc_attr.dynamic_range = DYNAMIC_RANGE_SDR8;
	wbc_ctx->wbc_attr.compress_mode = COMPRESS_MODE_NONE;
	wbc_ctx->wbc_mode = VO_WBC_MODE_NORM;
	wbc_ctx->depth = VO_WBC_DONEQ;
	wbc_ctx->done_cnt = 0;
	wbc_ctx->frame_num = 0;
	wbc_ctx->frame_rate = 0;
	wbc_ctx->odma_fifofull = 0;
	mutex_unlock(&wbc_ctx->wbc_lock);

	return 0;
}

static int vo_set_wbc_attr(vo_wbc wbc_dev, vo_wbc_attr_s *wbc_attr)
{
	int ret = -1;
	vo_dev dev;
	vo_layer layer;
	struct vo_dev_ctx *dev_ctx;
	struct vo_layer_ctx *layer_ctx;
	struct vo_wbc_ctx *wbc_ctx;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];
	dev = wbc_ctx->wbc_src.src_id;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];
	layer = dev_ctx->bind_layer_id;
	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (!wbc_ctx->is_wbc_src_cfg) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) src has not been set yet.\n", wbc_dev);
		return ERR_VO_WBC_SRC_NOT_CONFIG;
	}

	if (wbc_attr->target_size.width != layer_ctx->layer_attr.img_size.width ||
	    wbc_attr->target_size.height != layer_ctx->layer_attr.img_size.height) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) Size (%d %d) must be same to Layer Size (%d %d).\n",
			 wbc_dev, wbc_attr->target_size.width, wbc_attr->target_size.height,
			 layer_ctx->layer_attr.img_size.width, layer_ctx->layer_attr.img_size.height);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (!VO_SUPPORT_FMT(wbc_attr->pixformat)) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) pixformat(%d) unsupported\n",
			 wbc_dev,  wbc_attr->pixformat);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (wbc_attr->dynamic_range != DYNAMIC_RANGE_SDR8) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) dynamic_range(%d) unsupported\n",
			 wbc_dev,  wbc_attr->dynamic_range);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (wbc_attr->compress_mode != COMPRESS_MODE_NONE) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) compress_mode(%d) unsupported\n",
			 wbc_dev, wbc_attr->compress_mode);
		return ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&wbc_ctx->wbc_lock);
	if (wbc_ctx->wbc_src.src_type == VO_WBC_SRC_VIDEO) {
		TRACE_VO(DBG_INFO, "wbc_dev(%d) src_type(%d) use the LayerAttr\n",
			 wbc_dev,  wbc_ctx->wbc_src.src_type);
		wbc_ctx->wbc_attr.target_size = layer_ctx->layer_attr.img_size;
		wbc_ctx->wbc_attr.pixformat = layer_ctx->layer_attr.pixformat;
		wbc_ctx->wbc_attr.frame_rate = layer_ctx->layer_attr.frame_rate;
		wbc_ctx->wbc_attr.dynamic_range = DYNAMIC_RANGE_SDR8;
		wbc_ctx->wbc_attr.compress_mode = COMPRESS_MODE_NONE;
	} else {
		wbc_ctx->wbc_attr.target_size = wbc_attr->target_size;
		wbc_ctx->wbc_attr.pixformat = wbc_attr->pixformat;
		wbc_ctx->wbc_attr.frame_rate = wbc_attr->frame_rate;
		wbc_ctx->wbc_attr.dynamic_range = DYNAMIC_RANGE_SDR8;
		wbc_ctx->wbc_attr.compress_mode = COMPRESS_MODE_NONE;
	}

	wbc_ctx->is_wbc_attr_cfg = true;

	mutex_unlock(&wbc_ctx->wbc_lock);

	return 0;
}

static int vo_get_wbc_attr(vo_wbc wbc_dev, vo_wbc_attr_s *wbc_attr)
{
	int ret = -1;
	struct vo_wbc_ctx *wbc_ctx;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];
	memcpy(wbc_attr, &wbc_ctx->wbc_attr, sizeof(*wbc_attr));

	return 0;
}

static int vo_set_wbc_mode(vo_wbc wbc_dev, vo_wbc_mode_e wbc_mode)
{
	int ret = -1;
	struct vo_wbc_ctx *wbc_ctx;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];

	if (wbc_ctx->is_wbc_enable) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) must be in disabled state.\n", wbc_dev);
		return ERR_VO_WBC_NOT_DISABLED;
	}

	// if (wbc_mode < VO_WBC_MODE_NORM || wbc_mode >= VO_WBC_MODE_BUTT) {
	//	TRACE_VO(DBG_ERR, "wbc_dev(%d) Mode(%d) not support.\n", wbc_dev, wbc_mode);
	//	return ERR_VO_ILLEGAL_PARAM;
	// }

	// now only support VO_WBC_MODE_NORM
	if (wbc_mode != VO_WBC_MODE_NORM) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) Mode(%d) not support.\n", wbc_dev, wbc_mode);
		return ERR_VO_ILLEGAL_PARAM;
	}

	mutex_lock(&wbc_ctx->wbc_lock);
	wbc_ctx->wbc_mode = wbc_mode;
	mutex_unlock(&wbc_ctx->wbc_lock);

	return 0;
}

static int vo_get_wbc_mode(vo_wbc wbc_dev, vo_wbc_mode_e *wbc_mode)
{
	int ret = -1;
	struct vo_wbc_ctx *wbc_ctx;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];

	*wbc_mode = wbc_ctx->wbc_mode;

	return 0;
}

static int vo_set_wbc_depth(vo_wbc wbc_dev, u32 depth)
{
	int ret = -1;
	vo_dev dev;
	vo_layer layer;
	struct vo_dev_ctx *dev_ctx;
	struct vo_layer_ctx *layer_ctx;
	struct vo_wbc_ctx *wbc_ctx;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;


	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];
	dev = wbc_ctx->wbc_src.src_id;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];
	layer = dev_ctx->bind_layer_id;
	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	if (wbc_ctx->is_wbc_enable) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) must be in disabled state.\n", wbc_dev);
		return ERR_VO_WBC_NOT_DISABLED;
	}

	mutex_lock(&wbc_ctx->wbc_lock);
	if (wbc_ctx->wbc_src.src_type == VO_WBC_SRC_VIDEO) {
		TRACE_VO(DBG_INFO, "wbc_dev(%d) depth(%d) is Useless for src_type(%d)\n",
			 wbc_dev, depth, wbc_ctx->wbc_src.src_type);
		wbc_ctx->depth = layer_ctx->layer_attr.depth;
	} else {
		wbc_ctx->depth = depth;
	}
	mutex_unlock(&wbc_ctx->wbc_lock);

	return 0;
}

static int vo_get_wbc_depth(vo_wbc wbc_dev, u32 *depth)
{
	int ret = -1;
	struct vo_wbc_ctx *wbc_ctx;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];

	*depth = wbc_ctx->depth;

	return 0;
}

static int vo_get_wbc_frame(vo_wbc wbc_dev, video_frame_info_s *video_frame, int millisec)
{
	int ret = -1;
	struct vo_wbc_ctx *wbc_ctx;
	struct vo_dev_ctx *dev_ctx;
	struct vo_layer_ctx *layer_ctx;
	vb_blk blk;
	vo_dev dev;
	vo_layer layer;
	struct vb_s *vb;
	int i = 0;
	mmf_chn_s chn;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	wbc_ctx = &g_vo_ctx->wbc_ctx[wbc_dev];
	dev = wbc_ctx->wbc_src.src_id;
	dev_ctx = &g_vo_ctx->dev_ctx[dev];
	layer = dev_ctx->bind_layer_id;
	layer_ctx = &g_vo_ctx->layer_ctx[layer];

	chn.mod_id = ID_VO;
	chn.dev_id = dev;
	chn.chn_id = 0;

	if (!wbc_ctx->is_wbc_enable) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) isn't enabled yet.\n", wbc_dev);
		return ERR_VO_WBC_NOT_ENABLED;
	}

	if (wbc_ctx->depth <= 0) {
		TRACE_VO(DBG_ERR, "wbc_dev(%d) depth not enough.\n", wbc_dev);
		return ERR_VO_ILLEGAL_PARAM;
	}

	memset(video_frame, 0, sizeof(*video_frame));

	if (wbc_ctx->wbc_src.src_type == VO_WBC_SRC_DEV) {
		ret = base_get_chn_buffer(chn, &wbc_ctx->wbc_jobs, &blk, millisec);
		if (ret != 0) {
			TRACE_VO(DBG_ERR, "wbc get buf fail\n");
			return ret;
		}
	} else {
		ret = base_get_chn_buffer(chn, &layer_ctx->layer_jobs, &blk, millisec);
		if (ret != 0) {
			TRACE_VO(DBG_ERR, "wbc get buf fail\n");
			return ret;
		}
	}

	vb = (struct vb_s *)blk;

	video_frame->video_frame.pixel_format = vb->buf.pixel_format;
	video_frame->video_frame.width = vb->buf.size.width;
	video_frame->video_frame.height = vb->buf.size.height;
	video_frame->video_frame.time_ref = vb->buf.frm_num;
	video_frame->video_frame.pts = vb->buf.pts;
	for (i = 0; i < 3; ++i) {
		video_frame->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
		video_frame->video_frame.length[i] = vb->buf.length[i];
		video_frame->video_frame.stride[i] = vb->buf.stride[i];
	}

	video_frame->video_frame.offset_top = vb->buf.offset_top;
	video_frame->video_frame.offset_bottom = vb->buf.offset_bottom;
	video_frame->video_frame.offset_left = vb->buf.offset_left;
	video_frame->video_frame.offset_right = vb->buf.offset_right;
	video_frame->video_frame.private_data = vb;

	TRACE_VO(DBG_DEBUG, "pixfmt(%d), w(%d), h(%d), pts(%lld), addr(0x%llx, 0x%llx, 0x%llx)\n",
		 video_frame->video_frame.pixel_format, video_frame->video_frame.width,
		 video_frame->video_frame.height, video_frame->video_frame.pts,
		 video_frame->video_frame.phyaddr[0], video_frame->video_frame.phyaddr[1],
		 video_frame->video_frame.phyaddr[2]);
	TRACE_VO(DBG_DEBUG, "length(%d, %d, %d), stride(%d, %d, %d)\n",
		 video_frame->video_frame.length[0], video_frame->video_frame.length[1],
		 video_frame->video_frame.length[2], video_frame->video_frame.stride[0],
		 video_frame->video_frame.stride[1], video_frame->video_frame.stride[2]);

	return 0;
}

static int vo_release_wbc_frame(vo_wbc wbc_dev, video_frame_info_s *video_frame, int millisec)
{
	vb_blk blk;
	int ret = 0;

	ret = check_vo_wbc_valid(wbc_dev);
	if (ret != 0)
		return ret;

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VO(DBG_ERR, "Invalid phy-address(%llx) in video_frame. Can't find vb_blk.\n",
			 video_frame->video_frame.phyaddr[0]);
		return -1;
	}

	if (vb_release_block(blk) != 0)
		return -1;

	TRACE_VO(DBG_DEBUG, "release wbc frame, addr(0x%llx)\n",
		 video_frame->video_frame.phyaddr[0]);

	return 0;
}

static int vo_resume(struct vo_core_dev *vdev)
{
	int ret = -1;
	vo_wbc wbc_dev;
	vo_layer layer;
	vo_dev dev = 0;

	for (layer = 0; layer < VO_MAX_VIDEO_LAYER_NUM; ++layer)
		if (g_vo_ctx->layer_ctx[layer].is_layer_enable && g_vo_ctx->suspend) {
			ret = vo_create_thread(layer);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to vo create thread\n");
				ret = -EAGAIN;
			}
		}

	for (wbc_dev = 0; wbc_dev < VO_MAX_WBC_NUM; ++wbc_dev)
		if (g_vo_ctx->wbc_ctx[wbc_dev].is_wbc_enable && g_vo_ctx->suspend) {
			ret = vo_wbc_create_thread(wbc_dev);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to wbc create thread\n");
				ret = -EAGAIN;
			}
		}


	for (dev = 0; dev < VO_MAX_DEV_NUM; ++dev)
		if (g_vo_ctx->dev_ctx[dev].is_dev_enable && g_vo_ctx->suspend) {
			ret = vo_start_streaming(dev);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to vo start streaming\n");
				return -EAGAIN;
			}
		}

	g_vo_ctx->suspend = false;

	return ret;
}

static int vo_suspend(struct vo_core_dev *vdev)
{
	int ret = -1;
	vo_wbc wbc_dev;
	vo_layer layer;
	vo_dev dev = 0;

	g_vo_ctx->suspend = true;

	for (layer = 0; layer < VO_MAX_VIDEO_LAYER_NUM; ++layer)
		if (g_vo_ctx->layer_ctx[layer].is_layer_enable) {
			ret = vo_destroy_thread(layer);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to vo destroy thread\n");
				ret = -EAGAIN;
			}
		}

	for (wbc_dev = 0; wbc_dev < VO_MAX_WBC_NUM; ++wbc_dev)
		if (g_vo_ctx->wbc_ctx[wbc_dev].is_wbc_enable) {
			ret = vo_wbc_destroy_thread(wbc_dev);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to wbc destroy thread\n");
				ret = -EAGAIN;
			}
		}

	for (dev = 0; dev < VO_MAX_DEV_NUM; ++dev)
		if (g_vo_ctx->dev_ctx[dev].is_dev_enable) {
			ret = vo_stop_streaming(dev);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to vo stop streaming\n");
				return -EAGAIN;
			}
		}


	return ret;
}


/*****************************************************************************
 *  SDK layer ioctl operations for vi.c
 ****************************************************************************/
long vo_sdk_ctrl(struct vo_core_dev *vdev, struct vo_ext_control *p)
{
	u32 id = p->sdk_id;
	long rc = 0;

	switch (id) {
	case VO_SDK_SET_CHNATTR: {
		struct vo_chn_attr_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_attr_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_CHNATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_attr(cfg.layer, cfg.chn, &cfg.chn_attr);
	}
	break;

	case VO_SDK_GET_CHNATTR: {
		struct vo_chn_attr_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_attr_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_attr(cfg.layer, cfg.chn, &cfg.chn_attr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNATTR copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNPARAM: {
		struct vo_chn_param_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_param_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_CHNPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_param(cfg.layer, cfg.chn, &cfg.chn_param);
	}
	break;

	case VO_SDK_GET_CHNPARAM: {
		struct vo_chn_param_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_param_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_param(cfg.layer, cfg.chn, &cfg.chn_param);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNPARAM copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNZOOM: {
		struct vo_chn_zoom_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_zoom_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_zoom_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_CHNZOOM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_zoom(cfg.layer, cfg.chn, &cfg.chn_zoom_attr);
	}
	break;

	case VO_SDK_GET_CHNZOOM: {
		struct vo_chn_zoom_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_zoom_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_zoom_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNZOOM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_zoom(cfg.layer, cfg.chn, &cfg.chn_zoom_attr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_zoom_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNZOOM copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNBORDER: {
		struct vo_chn_border_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_border_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_border_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_CHNBORDER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_border(cfg.layer, cfg.chn, &cfg.chn_border_attr);
	}
	break;

	case VO_SDK_GET_CHNBORDER: {
		struct vo_chn_border_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_border_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_border_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNBORDER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_border(cfg.layer, cfg.chn, &cfg.chn_border_attr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_border_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNBORDER copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNMIRROR: {
		struct vo_chn_mirror_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_mirror_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_mirror_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_CHNMIRROR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_mirror(cfg.layer, cfg.chn, cfg.chn_mirror);
	}
	break;

	case VO_SDK_GET_CHNMIRROR: {
		struct vo_chn_mirror_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_mirror_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_mirror_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNMIRROR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_mirror(cfg.layer, cfg.chn, &cfg.chn_mirror);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_mirror_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNMIRROR copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNFRAME : {
		struct vo_chn_frame_cfg vo_chn_frame;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_frame_cfg);
		if (copy_from_user(&vo_chn_frame, p->ptr, sizeof(struct vo_chn_frame_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_frame(vo_chn_frame.layer, vo_chn_frame.chn, &vo_chn_frame.video_frame,
				      vo_chn_frame.millisec);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_get_chn_frame failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &vo_chn_frame, sizeof(struct vo_chn_frame_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNFRAME copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_RELEASE_CHNFRAME : {
		struct vo_chn_frame_cfg vo_chn_frame;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_frame_cfg);
		if (copy_from_user(&vo_chn_frame, p->ptr, sizeof(struct vo_chn_frame_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_RELEASE_CHNFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_release_chn_frame(vo_chn_frame.layer, vo_chn_frame.chn, &vo_chn_frame.video_frame,
					  vo_chn_frame.millisec);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_release_chn_frame failed with ret(%lx).\n", rc);
			break;
		}
	}
	break;

	case VO_SDK_SET_CHNFRAMERATE : {
		struct vo_chn_frmrate_cfg vo_chn_frame_rate;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_frmrate_cfg);
		if (copy_from_user(&vo_chn_frame_rate, p->ptr, sizeof(struct vo_chn_frmrate_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_CHNFRAMERATE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_framerate(vo_chn_frame_rate.layer, vo_chn_frame_rate.chn, vo_chn_frame_rate.frame_rate);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_set_chn_framerate failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &vo_chn_frame_rate, sizeof(struct vo_chn_frmrate_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_CHNFRAMERATE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNFRAMERATE : {
		struct vo_chn_frmrate_cfg vo_chn_frame_rate;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_frmrate_cfg);
		if (copy_from_user(&vo_chn_frame_rate, p->ptr, sizeof(struct vo_chn_frmrate_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNFRAMERATE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_framerate(vo_chn_frame_rate.layer, vo_chn_frame_rate.chn,
					  &vo_chn_frame_rate.frame_rate);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_get_chn_framerate failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &vo_chn_frame_rate, sizeof(struct vo_chn_frmrate_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNFRAMERATE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNPTS : {
		struct vo_chn_pts_cfg vo_chn_pts;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_pts_cfg);
		if (copy_from_user(&vo_chn_pts, p->ptr, sizeof(struct vo_chn_pts_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNPTS copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_pts(vo_chn_pts.layer, vo_chn_pts.chn, &vo_chn_pts.chn_pts);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_get_chn_pts failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &vo_chn_pts, sizeof(struct vo_chn_pts_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNPTS copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNSTATUS : {
		struct vo_chn_status_cfg vo_chn_status;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_status_cfg);
		if (copy_from_user(&vo_chn_status, p->ptr, sizeof(struct vo_chn_status_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNSTATUS copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_status(vo_chn_status.layer, vo_chn_status.chn, &vo_chn_status.status);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_get_chn_status failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &vo_chn_status, sizeof(struct vo_chn_status_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNSTATUS copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNTHRESHOLD: {
		struct vo_chn_threshold_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_chn_threshold_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_threshold_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_CHNTHRESHOLD copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chn_threshold(cfg.layer, cfg.chn, cfg.threshold);
	}
	break;

	case VO_SDK_GET_CHNTHRESHOLD: {
		struct vo_chn_threshold_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_chn_threshold_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_threshold_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNTHRESHOLD copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chn_threshold(cfg.layer, cfg.chn, &cfg.threshold);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_chn_threshold_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNTHRESHOLD copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_PUBATTR: {
		struct vo_pub_attr_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_pub_attr_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_pub_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_PUBATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_pub_attr(cfg.dev, &cfg.pub_attr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_pub_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_PUBATTR copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_PUBATTR: {
		struct vo_pub_attr_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_pub_attr_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_pub_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_PUBATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_pub_attr(cfg.dev, &cfg.pub_attr);
	}
	break;

	case VO_SDK_SET_LVDSPARAM: {
		struct vo_lvds_param_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_lvds_param_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_lvds_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_LVDSPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_lvds_param(cfg.dev, &cfg.lvds_param);
	}
	break;

	case VO_SDK_GET_LVDSPARAM: {
		struct vo_lvds_param_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_lvds_param_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_lvds_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_LVDSPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_lvds_param(cfg.dev, &cfg.lvds_param);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_lvds_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_LVDSPARAM copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_BTPARAM: {
		struct vo_bt_param_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_bt_param_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_bt_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_BTPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_bt_param(cfg.dev, &cfg.bt_param);
	}
	break;

	case VO_SDK_GET_BTPARAM: {
		struct vo_bt_param_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_bt_param_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_bt_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_BTPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_bt_param(cfg.dev, &cfg.bt_param);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_bt_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_BTPARAM copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_HDMIPARAM: {
		struct vo_hdmi_param_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_hdmi_param_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_hdmi_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_HDMIPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_hdmi_param(cfg.dev, &cfg.hdmi_param);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_hdmi_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_HDMIPARAM copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_HDMIPARAM: {
		struct vo_hdmi_param_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_hdmi_param_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_hdmi_param_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_HDMIPARAM copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_hdmi_param(cfg.dev, &cfg.hdmi_param);
	}
	break;

	case VO_SDK_SUSPEND: {
		rc = vo_suspend(vdev);
	}
	break;

	case VO_SDK_RESUME: {
		rc = vo_resume(vdev);
	}
	break;

	case VO_SDK_GET_PANELSTATUE: {
		struct vo_panel_status_cfg cfg;
		vo_dev dev;

		CHECK_STRUCT_SIZE(p->size, struct vo_panel_status_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_panel_status_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_PANELSTATUE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		dev = g_vo_ctx->layer_ctx[cfg.layer].bind_dev_id;
		vo_get_panelstatus(dev, &cfg.is_init);

		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_panel_status_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_PANELSTATUE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_ENABLE_CHN: {
		struct vo_chn_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_chn_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_ENABLE_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_enable_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_DISABLE_CHN: {
		struct vo_chn_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_chn_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_DISABLE_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_disable_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_SHOW_CHN: {
		struct vo_chn_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_chn_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SHOW_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_show_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_HIDE_CHN: {
		struct vo_chn_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_chn_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_HIDE_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_hide_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_ENABLE: {
		struct vo_dev_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_dev_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_dev_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_ENABLE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_enable(cfg.dev);
	}
	break;

	case VO_SDK_DISABLE: {
		struct vo_dev_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_dev_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_dev_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_DISABLE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_disable(cfg.dev);
	}
	break;

	case VO_SDK_ISENABLE: {
		struct vo_dev_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_dev_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_dev_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_ISENABLE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		cfg.enable = g_vo_ctx->dev_ctx[cfg.dev].is_dev_enable;

		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_dev_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_ISENABLE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SEND_FRAME: {
		struct vo_snd_frm_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_snd_frm_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_snd_frm_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SEND_FRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_send_frame(cfg.layer, cfg.chn, &cfg.video_frame, cfg.millisec);
	}
	break;

	case VO_SDK_CLEAR_CHNBUF: {
		struct vo_clear_chn_buf_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_clear_chn_buf_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_clear_chn_buf_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_CLEAR_CHNBUF copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_clear_chnbuf(cfg.layer, cfg.chn, cfg.clear);
	}
	break;

	case VO_SDK_SET_DISPLAYBUFLEN: {
		struct vo_display_buflen_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_display_buflen_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_display_buflen_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_DISPLAYBUFLEN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_displaybuflen(cfg.layer, cfg.buflen);
	}
	break;

	case VO_SDK_GET_DISPLAYBUFLEN: {
		struct vo_display_buflen_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_display_buflen_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_display_buflen_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_DISPLAYBUFLEN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_displaybuflen(cfg.layer, &cfg.buflen);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_display_buflen_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_DISPLAYBUFLEN copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_CHNROTATION: {
		struct vo_chn_rotation_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_chn_rotation_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_rotation_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNROTATION copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_chnrotation(cfg.layer, cfg.chn, &cfg.rotation);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_display_buflen_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_CHNROTATION copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_CHNROTATION: {
		struct vo_chn_rotation_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_chn_rotation_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_rotation_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_CHNROTATION copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_chnrotation(cfg.layer, cfg.chn, cfg.rotation);
	}
	break;

	case VO_SDK_SET_VIDEOLAYERATTR: {
		struct vo_video_layer_attr_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_video_layer_attr_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_VIDEOLAYERATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_videolayerattr(cfg.layer, &cfg.layer_attr);
	}
	break;

	case VO_SDK_GET_VIDEOLAYERATTR: {
		struct vo_video_layer_attr_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_video_layer_attr_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_VIDEOLAYERATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_videolayerattr(cfg.layer, &cfg.layer_attr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_video_layer_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_VIDEOLAYERATTR copy_to_user failed.\n");
			rc = -EFAULT;

		}
	}
	break;

	case VO_SDK_SET_LAYER_PROC_AMP: {
		struct vo_layer_proc_amp_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_layer_proc_amp_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_proc_amp_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_LAYER_PROC_AMP copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_layer_proc_amp(cfg.layer, cfg.proc_amp);
	}
	break;

	case VO_SDK_GET_LAYER_PROC_AMP: {
		struct vo_layer_proc_amp_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_layer_proc_amp_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_proc_amp_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_LAYER_PROC_AMP copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_layer_proc_amp(cfg.layer, cfg.proc_amp);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_layer_proc_amp_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_LAYER_PROC_AMP copy_to_user failed.\n");
			rc = -EFAULT;

		}
	}
	break;

	case VO_SDK_SET_LAYERCSC: {
		struct vo_layer_csc_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_layer_csc_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_csc_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_LAYERCSC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_layer_csc(cfg.layer, cfg.video_csc);
	}
	break;

	case VO_SDK_GET_LAYERCSC: {
		struct vo_layer_csc_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_layer_csc_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_csc_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_LAYERCSC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_layer_csc(cfg.layer, &cfg.video_csc);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_layer_csc_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_LAYERCSC copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_ENABLE_VIDEOLAYER: {
		struct vo_video_layer_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_video_layer_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_ENABLE_VIDEOLAYER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_enablevideolayer(cfg.layer);
	}
	break;

	case VO_SDK_DISABLE_VIDEOLAYER: {
		struct vo_video_layer_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_video_layer_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_DISABLE_VIDEOLAYER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_disablevideolayer(cfg.layer);
	}
	break;

	case VO_SDK_SET_LAYERTOLERATION: {
		struct vo_layer_toleration_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_layer_toleration_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_toleration_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_LAYERTOLERATION copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_layer_toleration(cfg.layer, cfg.toleration);
	}
	break;

	case VO_SDK_GET_LAYERTOLERATION: {
		struct vo_layer_toleration_cfg cfg;
		CHECK_STRUCT_SIZE(p->size, struct vo_layer_toleration_cfg);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_toleration_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_LAYERTOLERATION copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_layer_toleration(cfg.layer, &cfg.toleration);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_layer_toleration_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_LAYERTOLERATION copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_SCREENFRAME : {
		struct vo_screen_frame vo_screen_frame;

		CHECK_STRUCT_SIZE(p->size, struct vo_screen_frame);
		if (copy_from_user(&vo_screen_frame, p->ptr, sizeof(struct vo_screen_frame))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_SCREENFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_screen_frame(vo_screen_frame.layer, &vo_screen_frame.video_frame,
					 vo_screen_frame.millisec);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_get_screen_frame failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &vo_screen_frame, sizeof(struct vo_screen_frame))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_SCREENFRAME copy_to_user failed.\n");
			rc = -EFAULT;
		}

	}
	break;

	case VO_SDK_RELEASE_SCREENFRAME : {
		struct vo_screen_frame vo_screen_frame;

		CHECK_STRUCT_SIZE(p->size, struct vo_screen_frame);
		if (copy_from_user(&vo_screen_frame, p->ptr, sizeof(struct vo_screen_frame))) {
			TRACE_VO(DBG_ERR, "VO_SDK_RELEASE_SCREENFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_release_screen_frame(vo_screen_frame.layer, &vo_screen_frame.video_frame,
					     vo_screen_frame.millisec);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_release_screen_frame failed with ret(%lx).\n", rc);
			break;
		}
	}
	break;

	case VO_SDK_SET_LAYERPRRIORITY: {
		struct vo_layer_priority_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_layer_priority_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_priority_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_LAYERPRRIORITY copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_layer_priority(cfg.layer, cfg.priority);
	}
	break;

	case VO_SDK_GET_LAYERPRRIORITY: {
		struct vo_layer_priority_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_layer_priority_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_layer_priority_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_LAYERPRRIORITY copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_layer_priority(cfg.layer, &cfg.priority);
	}
	break;

	case VO_SDK_BIND_LAYER: {
		struct vo_video_layer_bind_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_video_layer_bind_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_bind_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_BIND_LAYER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_bind_layer(cfg.layer, cfg.dev);
	}
	break;

	case VO_SDK_UNBIND_LAYER: {
		struct vo_video_layer_bind_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_video_layer_bind_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_video_layer_bind_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_UNBIND_LAYER copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_unbind_layer(cfg.layer, cfg.dev);
	}
	break;

	case VO_SDK_PAUSE_CHN: {
		struct vo_chn_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_PAUSE_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_pause_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_STEP_CHN: {
		struct vo_chn_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_STEP_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_step_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_REFRESH_CHN: {
		struct vo_chn_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_REFRESH_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_refresh_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_RESUME_CHN: {
		struct vo_chn_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_chn_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_RESUME_CHN copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_resume_chn(cfg.layer, cfg.chn);
	}
	break;

	case VO_SDK_SET_WBCSRC: {
		struct vo_wbc_src_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_src_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_src_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_WBCSRC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_wbc_src(cfg.wbc_dev, &cfg.wbc_src);
	}
	break;

	case VO_SDK_GET_WBCSRC: {
		struct vo_wbc_src_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_src_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_src_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCSRC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_src(cfg.wbc_dev, &cfg.wbc_src);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_src_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCSRC copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_ENABLE_WBC: {
		struct vo_wbc_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_ENABLE_WBC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_enable_wbc(cfg.wbc_dev);
	}
	break;

	case VO_SDK_DISABLE_WBC: {
		struct vo_wbc_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_DISABLE_WBC copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_disable_wbc(cfg.wbc_dev);
	}
	break;

	case VO_SDK_SET_WBCATTR: {
		struct vo_wbc_attr_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_attr_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_WBCATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_wbc_attr(cfg.wbc_dev, &cfg.wbc_attr);
	}
	break;

	case VO_SDK_GET_WBCATTR: {
		struct vo_wbc_attr_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_attr_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCATTR copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_attr(cfg.wbc_dev, &cfg.wbc_attr);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_attr_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCATTR copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_WBCMODE: {
		struct vo_wbc_mode_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_mode_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_mode_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_WBCMODE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_wbc_mode(cfg.wbc_dev, cfg.wbc_mode);
	}
	break;

	case VO_SDK_GET_WBCMODE: {
		struct vo_wbc_mode_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_mode_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_mode_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCMODE copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_mode(cfg.wbc_dev, &cfg.wbc_mode);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_mode_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCMODE copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_SET_WBCDEPTH: {
		struct vo_wbc_depth_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_depth_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_depth_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_SET_WBCDEPTH copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_set_wbc_depth(cfg.wbc_dev, cfg.depth);
	}
	break;

	case VO_SDK_GET_WBCDEPTH: {
		struct vo_wbc_depth_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_depth_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_depth_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCDEPTH copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_depth(cfg.wbc_dev, &cfg.depth);
		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_depth_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCDEPTH copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_GET_WBCFRAME: {
		struct vo_wbc_frame_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_frame_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_frame_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_get_wbc_frame(cfg.wbc_dev, &cfg.video_frame, cfg.millisec);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_get_wbc_frame failed with ret(%lx).\n", rc);
			break;
		}

		if (copy_to_user(p->ptr, &cfg, sizeof(struct vo_wbc_frame_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_GET_WBCFRAME copy_to_user failed.\n");
			rc = -EFAULT;
		}
	}
	break;

	case VO_SDK_RELEASE_WBCFRAME: {
		struct vo_wbc_frame_cfg cfg;

		CHECK_STRUCT_SIZE(p->size, struct vo_wbc_frame_cfg);
		if (copy_from_user(&cfg, p->ptr, sizeof(struct vo_wbc_frame_cfg))) {
			TRACE_VO(DBG_ERR, "VO_SDK_RELEASE_WBCFRAME copy_from_user failed.\n");
			rc = -EFAULT;
			break;
		}

		rc = vo_release_wbc_frame(cfg.wbc_dev, &cfg.video_frame, cfg.millisec);
		if (rc) {
			TRACE_VO(DBG_ERR, "vo_release_wbc_frame failed with ret(%lx).\n", rc);
			break;
		}
	}
	break;

	default:
		break;
	}

	return rc;
}
